//===- ComplexToStandard.cpp - conversion from Complex to Standard dialect ===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mlir/Conversion/ComplexToStandard/ComplexToStandard.h"

#include "mlir/Conversion/ComplexCommon/DivisionConverter.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Complex/IR/Complex.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/IR/ImplicitLocOpBuilder.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/DialectConversion.h"
#include <type_traits>

namespace mlir {
#define GEN_PASS_DEF_CONVERTCOMPLEXTOSTANDARDPASS
#include "mlir/Conversion/Passes.h.inc"
} // namespace mlir

using namespace mlir;

namespace {

enum class AbsFn { abs, sqrt, rsqrt };

// Returns the absolute value, its square root or its reciprocal square root.
Value computeAbs(Value real, Value imag, arith::FastMathFlags fmf,
                 ImplicitLocOpBuilder &b, AbsFn fn = AbsFn::abs) {
  Value one = b.create<arith::ConstantOp>(real.getType(),
                                          b.getFloatAttr(real.getType(), 1.0));

  Value absReal = b.create<math::AbsFOp>(real, fmf);
  Value absImag = b.create<math::AbsFOp>(imag, fmf);

  Value max = b.create<arith::MaximumFOp>(absReal, absImag, fmf);
  Value min = b.create<arith::MinimumFOp>(absReal, absImag, fmf);

  // The lowering below requires NaNs and infinities to work correctly.
  arith::FastMathFlags fmfWithNaNInf = arith::bitEnumClear(
      fmf, arith::FastMathFlags::nnan | arith::FastMathFlags::ninf);
  Value ratio = b.create<arith::DivFOp>(min, max, fmfWithNaNInf);
  Value ratioSq = b.create<arith::MulFOp>(ratio, ratio, fmfWithNaNInf);
  Value ratioSqPlusOne = b.create<arith::AddFOp>(ratioSq, one, fmfWithNaNInf);
  Value result;

  if (fn == AbsFn::rsqrt) {
    ratioSqPlusOne = b.create<math::RsqrtOp>(ratioSqPlusOne, fmfWithNaNInf);
    min = b.create<math::RsqrtOp>(min, fmfWithNaNInf);
    max = b.create<math::RsqrtOp>(max, fmfWithNaNInf);
  }

  if (fn == AbsFn::sqrt) {
    Value quarter = b.create<arith::ConstantOp>(
        real.getType(), b.getFloatAttr(real.getType(), 0.25));
    // sqrt(sqrt(a*b)) would avoid the pow, but will overflow more easily.
    Value sqrt = b.create<math::SqrtOp>(max, fmfWithNaNInf);
    Value p025 = b.create<math::PowFOp>(ratioSqPlusOne, quarter, fmfWithNaNInf);
    result = b.create<arith::MulFOp>(sqrt, p025, fmfWithNaNInf);
  } else {
    Value sqrt = b.create<math::SqrtOp>(ratioSqPlusOne, fmfWithNaNInf);
    result = b.create<arith::MulFOp>(max, sqrt, fmfWithNaNInf);
  }

  Value isNaN = b.create<arith::CmpFOp>(arith::CmpFPredicate::UNO, result,
                                        result, fmfWithNaNInf);
  return b.create<arith::SelectOp>(isNaN, min, result);
}

struct AbsOpConversion : public OpConversionPattern<complex::AbsOp> {
  using OpConversionPattern<complex::AbsOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::AbsOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    ImplicitLocOpBuilder b(op.getLoc(), rewriter);

    arith::FastMathFlags fmf = op.getFastMathFlagsAttr().getValue();

    Value real = b.create<complex::ReOp>(adaptor.getComplex());
    Value imag = b.create<complex::ImOp>(adaptor.getComplex());
    rewriter.replaceOp(op, computeAbs(real, imag, fmf, b));

    return success();
  }
};

// atan2(y,x) = -i * log((x + i * y)/sqrt(x**2+y**2))
struct Atan2OpConversion : public OpConversionPattern<complex::Atan2Op> {
  using OpConversionPattern<complex::Atan2Op>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::Atan2Op op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    mlir::ImplicitLocOpBuilder b(op.getLoc(), rewriter);

    auto type = cast<ComplexType>(op.getType());
    Type elementType = type.getElementType();
    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();

    Value lhs = adaptor.getLhs();
    Value rhs = adaptor.getRhs();

    Value rhsSquared = b.create<complex::MulOp>(type, rhs, rhs, fmf);
    Value lhsSquared = b.create<complex::MulOp>(type, lhs, lhs, fmf);
    Value rhsSquaredPlusLhsSquared =
        b.create<complex::AddOp>(type, rhsSquared, lhsSquared, fmf);
    Value sqrtOfRhsSquaredPlusLhsSquared =
        b.create<complex::SqrtOp>(type, rhsSquaredPlusLhsSquared, fmf);

    Value zero =
        b.create<arith::ConstantOp>(elementType, b.getZeroAttr(elementType));
    Value one = b.create<arith::ConstantOp>(elementType,
                                            b.getFloatAttr(elementType, 1));
    Value i = b.create<complex::CreateOp>(type, zero, one);
    Value iTimesLhs = b.create<complex::MulOp>(i, lhs, fmf);
    Value rhsPlusILhs = b.create<complex::AddOp>(rhs, iTimesLhs, fmf);

    Value divResult = b.create<complex::DivOp>(
        rhsPlusILhs, sqrtOfRhsSquaredPlusLhsSquared, fmf);
    Value logResult = b.create<complex::LogOp>(divResult, fmf);

    Value negativeOne = b.create<arith::ConstantOp>(
        elementType, b.getFloatAttr(elementType, -1));
    Value negativeI = b.create<complex::CreateOp>(type, zero, negativeOne);

    rewriter.replaceOpWithNewOp<complex::MulOp>(op, negativeI, logResult, fmf);
    return success();
  }
};

template <typename ComparisonOp, arith::CmpFPredicate p>
struct ComparisonOpConversion : public OpConversionPattern<ComparisonOp> {
  using OpConversionPattern<ComparisonOp>::OpConversionPattern;
  using ResultCombiner =
      std::conditional_t<std::is_same<ComparisonOp, complex::EqualOp>::value,
                         arith::AndIOp, arith::OrIOp>;

  LogicalResult
  matchAndRewrite(ComparisonOp op, typename ComparisonOp::Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    auto type = cast<ComplexType>(adaptor.getLhs().getType()).getElementType();

    Value realLhs = rewriter.create<complex::ReOp>(loc, type, adaptor.getLhs());
    Value imagLhs = rewriter.create<complex::ImOp>(loc, type, adaptor.getLhs());
    Value realRhs = rewriter.create<complex::ReOp>(loc, type, adaptor.getRhs());
    Value imagRhs = rewriter.create<complex::ImOp>(loc, type, adaptor.getRhs());
    Value realComparison =
        rewriter.create<arith::CmpFOp>(loc, p, realLhs, realRhs);
    Value imagComparison =
        rewriter.create<arith::CmpFOp>(loc, p, imagLhs, imagRhs);

    rewriter.replaceOpWithNewOp<ResultCombiner>(op, realComparison,
                                                imagComparison);
    return success();
  }
};

// Default conversion which applies the BinaryStandardOp separately on the real
// and imaginary parts. Can for example be used for complex::AddOp and
// complex::SubOp.
template <typename BinaryComplexOp, typename BinaryStandardOp>
struct BinaryComplexOpConversion : public OpConversionPattern<BinaryComplexOp> {
  using OpConversionPattern<BinaryComplexOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(BinaryComplexOp op, typename BinaryComplexOp::Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto type = cast<ComplexType>(adaptor.getLhs().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    mlir::ImplicitLocOpBuilder b(op.getLoc(), rewriter);
    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();

    Value realLhs = b.create<complex::ReOp>(elementType, adaptor.getLhs());
    Value realRhs = b.create<complex::ReOp>(elementType, adaptor.getRhs());
    Value resultReal = b.create<BinaryStandardOp>(elementType, realLhs, realRhs,
                                                  fmf.getValue());
    Value imagLhs = b.create<complex::ImOp>(elementType, adaptor.getLhs());
    Value imagRhs = b.create<complex::ImOp>(elementType, adaptor.getRhs());
    Value resultImag = b.create<BinaryStandardOp>(elementType, imagLhs, imagRhs,
                                                  fmf.getValue());
    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, resultReal,
                                                   resultImag);
    return success();
  }
};

template <typename TrigonometricOp>
struct TrigonometricOpConversion : public OpConversionPattern<TrigonometricOp> {
  using OpAdaptor = typename OpConversionPattern<TrigonometricOp>::OpAdaptor;

  using OpConversionPattern<TrigonometricOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(TrigonometricOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    auto type = cast<ComplexType>(adaptor.getComplex().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();

    Value real =
        rewriter.create<complex::ReOp>(loc, elementType, adaptor.getComplex());
    Value imag =
        rewriter.create<complex::ImOp>(loc, elementType, adaptor.getComplex());

    // Trigonometric ops use a set of common building blocks to convert to real
    // ops. Here we create these building blocks and call into an op-specific
    // implementation in the subclass to combine them.
    Value half = rewriter.create<arith::ConstantOp>(
        loc, elementType, rewriter.getFloatAttr(elementType, 0.5));
    Value exp = rewriter.create<math::ExpOp>(loc, imag, fmf);
    Value scaledExp = rewriter.create<arith::MulFOp>(loc, half, exp, fmf);
    Value reciprocalExp = rewriter.create<arith::DivFOp>(loc, half, exp, fmf);
    Value sin = rewriter.create<math::SinOp>(loc, real, fmf);
    Value cos = rewriter.create<math::CosOp>(loc, real, fmf);

    auto resultPair =
        combine(loc, scaledExp, reciprocalExp, sin, cos, rewriter, fmf);

    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, resultPair.first,
                                                   resultPair.second);
    return success();
  }

  virtual std::pair<Value, Value>
  combine(Location loc, Value scaledExp, Value reciprocalExp, Value sin,
          Value cos, ConversionPatternRewriter &rewriter,
          arith::FastMathFlagsAttr fmf) const = 0;
};

struct CosOpConversion : public TrigonometricOpConversion<complex::CosOp> {
  using TrigonometricOpConversion<complex::CosOp>::TrigonometricOpConversion;

  std::pair<Value, Value> combine(Location loc, Value scaledExp,
                                  Value reciprocalExp, Value sin, Value cos,
                                  ConversionPatternRewriter &rewriter,
                                  arith::FastMathFlagsAttr fmf) const override {
    // Complex cosine is defined as;
    //   cos(x + iy) = 0.5 * (exp(i(x + iy)) + exp(-i(x + iy)))
    // Plugging in:
    //   exp(i(x+iy)) = exp(-y + ix) = exp(-y)(cos(x) + i sin(x))
    //   exp(-i(x+iy)) = exp(y + i(-x)) = exp(y)(cos(x) + i (-sin(x)))
    // and defining t := exp(y)
    // We get:
    //   Re(cos(x + iy)) = (0.5/t + 0.5*t) * cos x
    //   Im(cos(x + iy)) = (0.5/t - 0.5*t) * sin x
    Value sum =
        rewriter.create<arith::AddFOp>(loc, reciprocalExp, scaledExp, fmf);
    Value resultReal = rewriter.create<arith::MulFOp>(loc, sum, cos, fmf);
    Value diff =
        rewriter.create<arith::SubFOp>(loc, reciprocalExp, scaledExp, fmf);
    Value resultImag = rewriter.create<arith::MulFOp>(loc, diff, sin, fmf);
    return {resultReal, resultImag};
  }
};

struct DivOpConversion : public OpConversionPattern<complex::DivOp> {
  DivOpConversion(MLIRContext *context, complex::ComplexRangeFlags target)
      : OpConversionPattern<complex::DivOp>(context), complexRange(target) {}

  using OpConversionPattern<complex::DivOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::DivOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    auto type = cast<ComplexType>(adaptor.getLhs().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();

    Value lhsReal =
        rewriter.create<complex::ReOp>(loc, elementType, adaptor.getLhs());
    Value lhsImag =
        rewriter.create<complex::ImOp>(loc, elementType, adaptor.getLhs());
    Value rhsReal =
        rewriter.create<complex::ReOp>(loc, elementType, adaptor.getRhs());
    Value rhsImag =
        rewriter.create<complex::ImOp>(loc, elementType, adaptor.getRhs());

    Value resultReal, resultImag;

    if (complexRange == complex::ComplexRangeFlags::basic ||
        complexRange == complex::ComplexRangeFlags::none) {
      mlir::complex::convertDivToStandardUsingAlgebraic(
          rewriter, loc, lhsReal, lhsImag, rhsReal, rhsImag, fmf, &resultReal,
          &resultImag);
    } else if (complexRange == complex::ComplexRangeFlags::improved) {
      mlir::complex::convertDivToStandardUsingRangeReduction(
          rewriter, loc, lhsReal, lhsImag, rhsReal, rhsImag, fmf, &resultReal,
          &resultImag);
    }

    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, resultReal,
                                                   resultImag);

    return success();
  }

private:
  complex::ComplexRangeFlags complexRange;
};

struct ExpOpConversion : public OpConversionPattern<complex::ExpOp> {
  using OpConversionPattern<complex::ExpOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::ExpOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    auto type = cast<ComplexType>(adaptor.getComplex().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();

    Value real =
        rewriter.create<complex::ReOp>(loc, elementType, adaptor.getComplex());
    Value imag =
        rewriter.create<complex::ImOp>(loc, elementType, adaptor.getComplex());
    Value expReal = rewriter.create<math::ExpOp>(loc, real, fmf.getValue());
    Value cosImag = rewriter.create<math::CosOp>(loc, imag, fmf.getValue());
    Value resultReal =
        rewriter.create<arith::MulFOp>(loc, expReal, cosImag, fmf.getValue());
    Value sinImag = rewriter.create<math::SinOp>(loc, imag, fmf.getValue());
    Value resultImag =
        rewriter.create<arith::MulFOp>(loc, expReal, sinImag, fmf.getValue());

    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, resultReal,
                                                   resultImag);
    return success();
  }
};

Value evaluatePolynomial(ImplicitLocOpBuilder &b, Value arg,
                         ArrayRef<double> coefficients,
                         arith::FastMathFlagsAttr fmf) {
  auto argType = mlir::cast<FloatType>(arg.getType());
  Value poly =
      b.create<arith::ConstantOp>(b.getFloatAttr(argType, coefficients[0]));
  for (unsigned i = 1; i < coefficients.size(); ++i) {
    poly = b.create<math::FmaOp>(
        poly, arg,
        b.create<arith::ConstantOp>(b.getFloatAttr(argType, coefficients[i])),
        fmf);
  }
  return poly;
}

struct Expm1OpConversion : public OpConversionPattern<complex::Expm1Op> {
  using OpConversionPattern<complex::Expm1Op>::OpConversionPattern;

  // e^(a+bi)-1 = (e^a*cos(b)-1)+e^a*sin(b)i
  //            [handle inaccuracies when a and/or b are small]
  //            = ((e^a - 1) * cos(b) + cos(b) - 1) + e^a*sin(b)i
  //            = (expm1(a) * cos(b) + cosm1(b)) + e^a*sin(b)i
  LogicalResult
  matchAndRewrite(complex::Expm1Op op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto type = op.getType();
    auto elemType = mlir::cast<FloatType>(type.getElementType());

    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();
    ImplicitLocOpBuilder b(op.getLoc(), rewriter);
    Value real = b.create<complex::ReOp>(adaptor.getComplex());
    Value imag = b.create<complex::ImOp>(adaptor.getComplex());

    Value zero = b.create<arith::ConstantOp>(b.getFloatAttr(elemType, 0.0));
    Value one = b.create<arith::ConstantOp>(b.getFloatAttr(elemType, 1.0));

    Value expm1Real = b.create<math::ExpM1Op>(real, fmf);
    Value expReal = b.create<arith::AddFOp>(expm1Real, one, fmf);

    Value sinImag = b.create<math::SinOp>(imag, fmf);
    Value cosm1Imag = emitCosm1(imag, fmf, b);
    Value cosImag = b.create<arith::AddFOp>(cosm1Imag, one, fmf);

    Value realResult = b.create<arith::AddFOp>(
        b.create<arith::MulFOp>(expm1Real, cosImag, fmf), cosm1Imag, fmf);

    Value imagIsZero = b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, imag,
                                               zero, fmf.getValue());
    Value imagResult = b.create<arith::SelectOp>(
        imagIsZero, zero, b.create<arith::MulFOp>(expReal, sinImag, fmf));

    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, realResult,
                                                   imagResult);
    return success();
  }

private:
  Value emitCosm1(Value arg, arith::FastMathFlagsAttr fmf,
                  ImplicitLocOpBuilder &b) const {
    auto argType = mlir::cast<FloatType>(arg.getType());
    auto negHalf = b.create<arith::ConstantOp>(b.getFloatAttr(argType, -0.5));
    auto negOne = b.create<arith::ConstantOp>(b.getFloatAttr(argType, -1.0));

    // Algorithm copied from cephes cosm1.
    SmallVector<double, 7> kCoeffs{
        4.7377507964246204691685E-14, -1.1470284843425359765671E-11,
        2.0876754287081521758361E-9,  -2.7557319214999787979814E-7,
        2.4801587301570552304991E-5,  -1.3888888888888872993737E-3,
        4.1666666666666666609054E-2,
    };
    Value cos = b.create<math::CosOp>(arg, fmf);
    Value forLargeArg = b.create<arith::AddFOp>(cos, negOne, fmf);

    Value argPow2 = b.create<arith::MulFOp>(arg, arg, fmf);
    Value argPow4 = b.create<arith::MulFOp>(argPow2, argPow2, fmf);
    Value poly = evaluatePolynomial(b, argPow2, kCoeffs, fmf);

    auto forSmallArg =
        b.create<arith::AddFOp>(b.create<arith::MulFOp>(argPow4, poly, fmf),
                                b.create<arith::MulFOp>(negHalf, argPow2, fmf));

    // (pi/4)^2 is approximately 0.61685
    Value piOver4Pow2 =
        b.create<arith::ConstantOp>(b.getFloatAttr(argType, 0.61685));
    Value cond = b.create<arith::CmpFOp>(arith::CmpFPredicate::OGE, argPow2,
                                         piOver4Pow2, fmf.getValue());
    return b.create<arith::SelectOp>(cond, forLargeArg, forSmallArg);
  }
};

struct LogOpConversion : public OpConversionPattern<complex::LogOp> {
  using OpConversionPattern<complex::LogOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::LogOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto type = cast<ComplexType>(adaptor.getComplex().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();
    mlir::ImplicitLocOpBuilder b(op.getLoc(), rewriter);

    Value abs = b.create<complex::AbsOp>(elementType, adaptor.getComplex(),
                                         fmf.getValue());
    Value resultReal = b.create<math::LogOp>(elementType, abs, fmf.getValue());
    Value real = b.create<complex::ReOp>(elementType, adaptor.getComplex());
    Value imag = b.create<complex::ImOp>(elementType, adaptor.getComplex());
    Value resultImag =
        b.create<math::Atan2Op>(elementType, imag, real, fmf.getValue());
    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, resultReal,
                                                   resultImag);
    return success();
  }
};

struct Log1pOpConversion : public OpConversionPattern<complex::Log1pOp> {
  using OpConversionPattern<complex::Log1pOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::Log1pOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto type = cast<ComplexType>(adaptor.getComplex().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    arith::FastMathFlags fmf = op.getFastMathFlagsAttr().getValue();
    mlir::ImplicitLocOpBuilder b(op.getLoc(), rewriter);

    Value real = b.create<complex::ReOp>(adaptor.getComplex());
    Value imag = b.create<complex::ImOp>(adaptor.getComplex());

    Value half = b.create<arith::ConstantOp>(elementType,
                                             b.getFloatAttr(elementType, 0.5));
    Value one = b.create<arith::ConstantOp>(elementType,
                                            b.getFloatAttr(elementType, 1));
    Value realPlusOne = b.create<arith::AddFOp>(real, one, fmf);
    Value absRealPlusOne = b.create<math::AbsFOp>(realPlusOne, fmf);
    Value absImag = b.create<math::AbsFOp>(imag, fmf);

    Value maxAbs = b.create<arith::MaximumFOp>(absRealPlusOne, absImag, fmf);
    Value minAbs = b.create<arith::MinimumFOp>(absRealPlusOne, absImag, fmf);

    Value useReal = b.create<arith::CmpFOp>(arith::CmpFPredicate::OGT,
                                            realPlusOne, absImag, fmf);
    Value maxMinusOne = b.create<arith::SubFOp>(maxAbs, one, fmf);
    Value maxAbsOfRealPlusOneAndImagMinusOne =
        b.create<arith::SelectOp>(useReal, real, maxMinusOne);
    arith::FastMathFlags fmfWithNaNInf = arith::bitEnumClear(
        fmf, arith::FastMathFlags::nnan | arith::FastMathFlags::ninf);
    Value minMaxRatio = b.create<arith::DivFOp>(minAbs, maxAbs, fmfWithNaNInf);
    Value logOfMaxAbsOfRealPlusOneAndImag =
        b.create<math::Log1pOp>(maxAbsOfRealPlusOneAndImagMinusOne, fmf);
    Value logOfSqrtPart = b.create<math::Log1pOp>(
        b.create<arith::MulFOp>(minMaxRatio, minMaxRatio, fmfWithNaNInf),
        fmfWithNaNInf);
    Value r = b.create<arith::AddFOp>(
        b.create<arith::MulFOp>(half, logOfSqrtPart, fmfWithNaNInf),
        logOfMaxAbsOfRealPlusOneAndImag, fmfWithNaNInf);
    Value resultReal = b.create<arith::SelectOp>(
        b.create<arith::CmpFOp>(arith::CmpFPredicate::UNO, r, r, fmfWithNaNInf),
        minAbs, r);
    Value resultImag = b.create<math::Atan2Op>(imag, realPlusOne, fmf);
    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, resultReal,
                                                   resultImag);
    return success();
  }
};

struct MulOpConversion : public OpConversionPattern<complex::MulOp> {
  using OpConversionPattern<complex::MulOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::MulOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    mlir::ImplicitLocOpBuilder b(op.getLoc(), rewriter);
    auto type = cast<ComplexType>(adaptor.getLhs().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();
    auto fmfValue = fmf.getValue();
    Value lhsReal = b.create<complex::ReOp>(elementType, adaptor.getLhs());
    Value lhsImag = b.create<complex::ImOp>(elementType, adaptor.getLhs());
    Value rhsReal = b.create<complex::ReOp>(elementType, adaptor.getRhs());
    Value rhsImag = b.create<complex::ImOp>(elementType, adaptor.getRhs());
    Value lhsRealTimesRhsReal =
        b.create<arith::MulFOp>(lhsReal, rhsReal, fmfValue);
    Value lhsImagTimesRhsImag =
        b.create<arith::MulFOp>(lhsImag, rhsImag, fmfValue);
    Value real = b.create<arith::SubFOp>(lhsRealTimesRhsReal,
                                         lhsImagTimesRhsImag, fmfValue);
    Value lhsImagTimesRhsReal =
        b.create<arith::MulFOp>(lhsImag, rhsReal, fmfValue);
    Value lhsRealTimesRhsImag =
        b.create<arith::MulFOp>(lhsReal, rhsImag, fmfValue);
    Value imag = b.create<arith::AddFOp>(lhsImagTimesRhsReal,
                                         lhsRealTimesRhsImag, fmfValue);
    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, real, imag);
    return success();
  }
};

struct NegOpConversion : public OpConversionPattern<complex::NegOp> {
  using OpConversionPattern<complex::NegOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::NegOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    auto type = cast<ComplexType>(adaptor.getComplex().getType());
    auto elementType = cast<FloatType>(type.getElementType());

    Value real =
        rewriter.create<complex::ReOp>(loc, elementType, adaptor.getComplex());
    Value imag =
        rewriter.create<complex::ImOp>(loc, elementType, adaptor.getComplex());
    Value negReal = rewriter.create<arith::NegFOp>(loc, real);
    Value negImag = rewriter.create<arith::NegFOp>(loc, imag);
    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, negReal, negImag);
    return success();
  }
};

struct SinOpConversion : public TrigonometricOpConversion<complex::SinOp> {
  using TrigonometricOpConversion<complex::SinOp>::TrigonometricOpConversion;

  std::pair<Value, Value> combine(Location loc, Value scaledExp,
                                  Value reciprocalExp, Value sin, Value cos,
                                  ConversionPatternRewriter &rewriter,
                                  arith::FastMathFlagsAttr fmf) const override {
    // Complex sine is defined as;
    //   sin(x + iy) = -0.5i * (exp(i(x + iy)) - exp(-i(x + iy)))
    // Plugging in:
    //   exp(i(x+iy)) = exp(-y + ix) = exp(-y)(cos(x) + i sin(x))
    //   exp(-i(x+iy)) = exp(y + i(-x)) = exp(y)(cos(x) + i (-sin(x)))
    // and defining t := exp(y)
    // We get:
    //   Re(sin(x + iy)) = (0.5*t + 0.5/t) * sin x
    //   Im(cos(x + iy)) = (0.5*t - 0.5/t) * cos x
    Value sum =
        rewriter.create<arith::AddFOp>(loc, scaledExp, reciprocalExp, fmf);
    Value resultReal = rewriter.create<arith::MulFOp>(loc, sum, sin, fmf);
    Value diff =
        rewriter.create<arith::SubFOp>(loc, scaledExp, reciprocalExp, fmf);
    Value resultImag = rewriter.create<arith::MulFOp>(loc, diff, cos, fmf);
    return {resultReal, resultImag};
  }
};

// The algorithm is listed in https://dl.acm.org/doi/pdf/10.1145/363717.363780.
struct SqrtOpConversion : public OpConversionPattern<complex::SqrtOp> {
  using OpConversionPattern<complex::SqrtOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::SqrtOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    ImplicitLocOpBuilder b(op.getLoc(), rewriter);

    auto type = cast<ComplexType>(op.getType());
    auto elementType = cast<FloatType>(type.getElementType());
    arith::FastMathFlags fmf = op.getFastMathFlagsAttr().getValue();

    auto cst = [&](APFloat v) {
      return b.create<arith::ConstantOp>(elementType,
                                         b.getFloatAttr(elementType, v));
    };
    const auto &floatSemantics = elementType.getFloatSemantics();
    Value zero = cst(APFloat::getZero(floatSemantics));
    Value half = b.create<arith::ConstantOp>(elementType,
                                             b.getFloatAttr(elementType, 0.5));

    Value real = b.create<complex::ReOp>(elementType, adaptor.getComplex());
    Value imag = b.create<complex::ImOp>(elementType, adaptor.getComplex());
    Value absSqrt = computeAbs(real, imag, fmf, b, AbsFn::sqrt);
    Value argArg = b.create<math::Atan2Op>(imag, real, fmf);
    Value sqrtArg = b.create<arith::MulFOp>(argArg, half, fmf);
    Value cos = b.create<math::CosOp>(sqrtArg, fmf);
    Value sin = b.create<math::SinOp>(sqrtArg, fmf);
    // sin(atan2(0, inf)) = 0, sqrt(abs(inf)) = inf, but we can't multiply
    // 0 * inf.
    Value sinIsZero =
        b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, sin, zero, fmf);

    Value resultReal = b.create<arith::MulFOp>(absSqrt, cos, fmf);
    Value resultImag = b.create<arith::SelectOp>(
        sinIsZero, zero, b.create<arith::MulFOp>(absSqrt, sin, fmf));
    if (!arith::bitEnumContainsAll(fmf, arith::FastMathFlags::nnan |
                                            arith::FastMathFlags::ninf)) {
      Value inf = cst(APFloat::getInf(floatSemantics));
      Value negInf = cst(APFloat::getInf(floatSemantics, true));
      Value nan = cst(APFloat::getNaN(floatSemantics));
      Value absImag = b.create<math::AbsFOp>(elementType, imag, fmf);

      Value absImagIsInf =
          b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, absImag, inf, fmf);
      Value absImagIsNotInf =
          b.create<arith::CmpFOp>(arith::CmpFPredicate::ONE, absImag, inf, fmf);
      Value realIsInf =
          b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, real, inf, fmf);
      Value realIsNegInf =
          b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, real, negInf, fmf);

      resultReal = b.create<arith::SelectOp>(
          b.create<arith::AndIOp>(realIsNegInf, absImagIsNotInf), zero,
          resultReal);
      resultReal = b.create<arith::SelectOp>(
          b.create<arith::OrIOp>(absImagIsInf, realIsInf), inf, resultReal);

      Value imagSignInf = b.create<math::CopySignOp>(inf, imag, fmf);
      resultImag = b.create<arith::SelectOp>(
          b.create<arith::CmpFOp>(arith::CmpFPredicate::UNO, absSqrt, absSqrt),
          nan, resultImag);
      resultImag = b.create<arith::SelectOp>(
          b.create<arith::OrIOp>(absImagIsInf, realIsNegInf), imagSignInf,
          resultImag);
    }

    Value resultIsZero =
        b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, absSqrt, zero, fmf);
    resultReal = b.create<arith::SelectOp>(resultIsZero, zero, resultReal);
    resultImag = b.create<arith::SelectOp>(resultIsZero, zero, resultImag);

    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, resultReal,
                                                   resultImag);
    return success();
  }
};

struct SignOpConversion : public OpConversionPattern<complex::SignOp> {
  using OpConversionPattern<complex::SignOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::SignOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto type = cast<ComplexType>(adaptor.getComplex().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    mlir::ImplicitLocOpBuilder b(op.getLoc(), rewriter);
    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();

    Value real = b.create<complex::ReOp>(elementType, adaptor.getComplex());
    Value imag = b.create<complex::ImOp>(elementType, adaptor.getComplex());
    Value zero =
        b.create<arith::ConstantOp>(elementType, b.getZeroAttr(elementType));
    Value realIsZero =
        b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, real, zero);
    Value imagIsZero =
        b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, imag, zero);
    Value isZero = b.create<arith::AndIOp>(realIsZero, imagIsZero);
    auto abs = b.create<complex::AbsOp>(elementType, adaptor.getComplex(), fmf);
    Value realSign = b.create<arith::DivFOp>(real, abs, fmf);
    Value imagSign = b.create<arith::DivFOp>(imag, abs, fmf);
    Value sign = b.create<complex::CreateOp>(type, realSign, imagSign);
    rewriter.replaceOpWithNewOp<arith::SelectOp>(op, isZero,
                                                 adaptor.getComplex(), sign);
    return success();
  }
};

template <typename Op>
struct TanTanhOpConversion : public OpConversionPattern<Op> {
  using OpConversionPattern<Op>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(Op op, typename Op::Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    ImplicitLocOpBuilder b(op.getLoc(), rewriter);
    auto loc = op.getLoc();
    auto type = cast<ComplexType>(adaptor.getComplex().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    arith::FastMathFlags fmf = op.getFastMathFlagsAttr().getValue();
    const auto &floatSemantics = elementType.getFloatSemantics();

    Value real =
        b.create<complex::ReOp>(loc, elementType, adaptor.getComplex());
    Value imag =
        b.create<complex::ImOp>(loc, elementType, adaptor.getComplex());
    Value negOne = b.create<arith::ConstantOp>(
        elementType, b.getFloatAttr(elementType, -1.0));

    if constexpr (std::is_same_v<Op, complex::TanOp>) {
      // tan(x+yi) = -i*tanh(-y + xi)
      std::swap(real, imag);
      real = b.create<arith::MulFOp>(real, negOne, fmf);
    }

    auto cst = [&](APFloat v) {
      return b.create<arith::ConstantOp>(elementType,
                                         b.getFloatAttr(elementType, v));
    };
    Value inf = cst(APFloat::getInf(floatSemantics));
    Value four = b.create<arith::ConstantOp>(elementType,
                                             b.getFloatAttr(elementType, 4.0));
    Value twoReal = b.create<arith::AddFOp>(real, real, fmf);
    Value negTwoReal = b.create<arith::MulFOp>(negOne, twoReal, fmf);

    Value expTwoRealMinusOne = b.create<math::ExpM1Op>(twoReal, fmf);
    Value expNegTwoRealMinusOne = b.create<math::ExpM1Op>(negTwoReal, fmf);
    Value realNum =
        b.create<arith::SubFOp>(expTwoRealMinusOne, expNegTwoRealMinusOne, fmf);

    Value cosImag = b.create<math::CosOp>(imag, fmf);
    Value cosImagSq = b.create<arith::MulFOp>(cosImag, cosImag, fmf);
    Value twoCosTwoImagPlusOne = b.create<arith::MulFOp>(cosImagSq, four, fmf);
    Value sinImag = b.create<math::SinOp>(imag, fmf);

    Value imagNum = b.create<arith::MulFOp>(
        four, b.create<arith::MulFOp>(cosImag, sinImag, fmf), fmf);

    Value expSumMinusTwo =
        b.create<arith::AddFOp>(expTwoRealMinusOne, expNegTwoRealMinusOne, fmf);
    Value denom =
        b.create<arith::AddFOp>(expSumMinusTwo, twoCosTwoImagPlusOne, fmf);

    Value isInf = b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ,
                                          expSumMinusTwo, inf, fmf);
    Value realLimit = b.create<math::CopySignOp>(negOne, real, fmf);

    Value resultReal = b.create<arith::SelectOp>(
        isInf, realLimit, b.create<arith::DivFOp>(realNum, denom, fmf));
    Value resultImag = b.create<arith::DivFOp>(imagNum, denom, fmf);

    if (!arith::bitEnumContainsAll(fmf, arith::FastMathFlags::nnan |
                                            arith::FastMathFlags::ninf)) {
      Value absReal = b.create<math::AbsFOp>(real, fmf);
      Value zero = b.create<arith::ConstantOp>(
          elementType, b.getFloatAttr(elementType, 0.0));
      Value nan = cst(APFloat::getNaN(floatSemantics));

      Value absRealIsInf =
          b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, absReal, inf, fmf);
      Value imagIsZero =
          b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, imag, zero, fmf);
      Value absRealIsNotInf = b.create<arith::XOrIOp>(
          absRealIsInf, b.create<arith::ConstantIntOp>(true, /*width=*/1));

      Value imagNumIsNaN = b.create<arith::CmpFOp>(arith::CmpFPredicate::UNO,
                                                   imagNum, imagNum, fmf);
      Value resultRealIsNaN =
          b.create<arith::AndIOp>(imagNumIsNaN, absRealIsNotInf);
      Value resultImagIsZero = b.create<arith::OrIOp>(
          imagIsZero, b.create<arith::AndIOp>(absRealIsInf, imagNumIsNaN));

      resultReal = b.create<arith::SelectOp>(resultRealIsNaN, nan, resultReal);
      resultImag =
          b.create<arith::SelectOp>(resultImagIsZero, zero, resultImag);
    }

    if constexpr (std::is_same_v<Op, complex::TanOp>) {
      // tan(x+yi) = -i*tanh(-y + xi)
      std::swap(resultReal, resultImag);
      resultImag = b.create<arith::MulFOp>(resultImag, negOne, fmf);
    }

    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, resultReal,
                                                   resultImag);
    return success();
  }
};

struct ConjOpConversion : public OpConversionPattern<complex::ConjOp> {
  using OpConversionPattern<complex::ConjOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::ConjOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    auto type = cast<ComplexType>(adaptor.getComplex().getType());
    auto elementType = cast<FloatType>(type.getElementType());
    Value real =
        rewriter.create<complex::ReOp>(loc, elementType, adaptor.getComplex());
    Value imag =
        rewriter.create<complex::ImOp>(loc, elementType, adaptor.getComplex());
    Value negImag = rewriter.create<arith::NegFOp>(loc, elementType, imag);

    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, real, negImag);

    return success();
  }
};

/// Converts lhs^y = (a+bi)^(c+di) to
///    (a*a+b*b)^(0.5c) * exp(-d*atan2(b,a)) * (cos(q) + i*sin(q)),
///    where q = c*atan2(b,a)+0.5d*ln(a*a+b*b)
static Value powOpConversionImpl(mlir::ImplicitLocOpBuilder &builder,
                                 ComplexType type, Value lhs, Value c, Value d,
                                 arith::FastMathFlags fmf) {
  auto elementType = cast<FloatType>(type.getElementType());

  Value a = builder.create<complex::ReOp>(lhs);
  Value b = builder.create<complex::ImOp>(lhs);

  Value abs = builder.create<complex::AbsOp>(lhs, fmf);
  Value absToC = builder.create<math::PowFOp>(abs, c, fmf);

  Value negD = builder.create<arith::NegFOp>(d, fmf);
  Value argLhs = builder.create<math::Atan2Op>(b, a, fmf);
  Value negDArgLhs = builder.create<arith::MulFOp>(negD, argLhs, fmf);
  Value expNegDArgLhs = builder.create<math::ExpOp>(negDArgLhs, fmf);

  Value coeff = builder.create<arith::MulFOp>(absToC, expNegDArgLhs, fmf);
  Value lnAbs = builder.create<math::LogOp>(abs, fmf);
  Value cArgLhs = builder.create<arith::MulFOp>(c, argLhs, fmf);
  Value dLnAbs = builder.create<arith::MulFOp>(d, lnAbs, fmf);
  Value q = builder.create<arith::AddFOp>(cArgLhs, dLnAbs, fmf);
  Value cosQ = builder.create<math::CosOp>(q, fmf);
  Value sinQ = builder.create<math::SinOp>(q, fmf);

  Value inf = builder.create<arith::ConstantOp>(
      elementType,
      builder.getFloatAttr(elementType,
                           APFloat::getInf(elementType.getFloatSemantics())));
  Value zero = builder.create<arith::ConstantOp>(
      elementType, builder.getFloatAttr(elementType, 0.0));
  Value one = builder.create<arith::ConstantOp>(
      elementType, builder.getFloatAttr(elementType, 1.0));
  Value complexOne = builder.create<complex::CreateOp>(type, one, zero);
  Value complexZero = builder.create<complex::CreateOp>(type, zero, zero);
  Value complexInf = builder.create<complex::CreateOp>(type, inf, zero);

  // Case 0:
  // d^c is 0 if d is 0 and c > 0. 0^0 is defined to be 1.0, see
  // Branch Cuts for Complex Elementary Functions or Much Ado About
  // Nothing's Sign Bit, W. Kahan, Section 10.
  Value absEqZero =
      builder.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, abs, zero, fmf);
  Value dEqZero =
      builder.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, d, zero, fmf);
  Value cEqZero =
      builder.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, c, zero, fmf);
  Value bEqZero =
      builder.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, b, zero, fmf);

  Value zeroLeC =
      builder.create<arith::CmpFOp>(arith::CmpFPredicate::OLE, zero, c, fmf);
  Value coeffCosQ = builder.create<arith::MulFOp>(coeff, cosQ, fmf);
  Value coeffSinQ = builder.create<arith::MulFOp>(coeff, sinQ, fmf);
  Value complexOneOrZero =
      builder.create<arith::SelectOp>(cEqZero, complexOne, complexZero);
  Value coeffCosSin =
      builder.create<complex::CreateOp>(type, coeffCosQ, coeffSinQ);
  Value cutoff0 = builder.create<arith::SelectOp>(
      builder.create<arith::AndIOp>(
          builder.create<arith::AndIOp>(absEqZero, dEqZero), zeroLeC),
      complexOneOrZero, coeffCosSin);

  // Case 1:
  // x^0 is defined to be 1 for any x, see
  // Branch Cuts for Complex Elementary Functions or Much Ado About
  // Nothing's Sign Bit, W. Kahan, Section 10.
  Value rhsEqZero = builder.create<arith::AndIOp>(cEqZero, dEqZero);
  Value cutoff1 =
      builder.create<arith::SelectOp>(rhsEqZero, complexOne, cutoff0);

  // Case 2:
  // 1^(c + d*i) = 1 + 0*i
  Value lhsEqOne = builder.create<arith::AndIOp>(
      builder.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, a, one, fmf),
      bEqZero);
  Value cutoff2 =
      builder.create<arith::SelectOp>(lhsEqOne, complexOne, cutoff1);

  // Case 3:
  // inf^(c + 0*i) = inf + 0*i, c > 0
  Value lhsEqInf = builder.create<arith::AndIOp>(
      builder.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, a, inf, fmf),
      bEqZero);
  Value rhsGt0 = builder.create<arith::AndIOp>(
      dEqZero,
      builder.create<arith::CmpFOp>(arith::CmpFPredicate::OGT, c, zero, fmf));
  Value cutoff3 = builder.create<arith::SelectOp>(
      builder.create<arith::AndIOp>(lhsEqInf, rhsGt0), complexInf, cutoff2);

  // Case 4:
  // inf^(c + 0*i) = 0 + 0*i, c < 0
  Value rhsLt0 = builder.create<arith::AndIOp>(
      dEqZero,
      builder.create<arith::CmpFOp>(arith::CmpFPredicate::OLT, c, zero, fmf));
  Value cutoff4 = builder.create<arith::SelectOp>(
      builder.create<arith::AndIOp>(lhsEqInf, rhsLt0), complexZero, cutoff3);

  return cutoff4;
}

struct PowOpConversion : public OpConversionPattern<complex::PowOp> {
  using OpConversionPattern<complex::PowOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::PowOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    mlir::ImplicitLocOpBuilder builder(op.getLoc(), rewriter);
    auto type = cast<ComplexType>(adaptor.getLhs().getType());
    auto elementType = cast<FloatType>(type.getElementType());

    Value c = builder.create<complex::ReOp>(elementType, adaptor.getRhs());
    Value d = builder.create<complex::ImOp>(elementType, adaptor.getRhs());

    rewriter.replaceOp(op, {powOpConversionImpl(builder, type, adaptor.getLhs(),
                                                c, d, op.getFastmath())});
    return success();
  }
};

struct RsqrtOpConversion : public OpConversionPattern<complex::RsqrtOp> {
  using OpConversionPattern<complex::RsqrtOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::RsqrtOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    mlir::ImplicitLocOpBuilder b(op.getLoc(), rewriter);
    auto type = cast<ComplexType>(adaptor.getComplex().getType());
    auto elementType = cast<FloatType>(type.getElementType());

    arith::FastMathFlags fmf = op.getFastMathFlagsAttr().getValue();

    auto cst = [&](APFloat v) {
      return b.create<arith::ConstantOp>(elementType,
                                         b.getFloatAttr(elementType, v));
    };
    const auto &floatSemantics = elementType.getFloatSemantics();
    Value zero = cst(APFloat::getZero(floatSemantics));
    Value inf = cst(APFloat::getInf(floatSemantics));
    Value negHalf = b.create<arith::ConstantOp>(
        elementType, b.getFloatAttr(elementType, -0.5));
    Value nan = cst(APFloat::getNaN(floatSemantics));

    Value real = b.create<complex::ReOp>(elementType, adaptor.getComplex());
    Value imag = b.create<complex::ImOp>(elementType, adaptor.getComplex());
    Value absRsqrt = computeAbs(real, imag, fmf, b, AbsFn::rsqrt);
    Value argArg = b.create<math::Atan2Op>(imag, real, fmf);
    Value rsqrtArg = b.create<arith::MulFOp>(argArg, negHalf, fmf);
    Value cos = b.create<math::CosOp>(rsqrtArg, fmf);
    Value sin = b.create<math::SinOp>(rsqrtArg, fmf);

    Value resultReal = b.create<arith::MulFOp>(absRsqrt, cos, fmf);
    Value resultImag = b.create<arith::MulFOp>(absRsqrt, sin, fmf);

    if (!arith::bitEnumContainsAll(fmf, arith::FastMathFlags::nnan |
                                            arith::FastMathFlags::ninf)) {
      Value negOne = b.create<arith::ConstantOp>(
          elementType, b.getFloatAttr(elementType, -1));

      Value realSignedZero = b.create<math::CopySignOp>(zero, real, fmf);
      Value imagSignedZero = b.create<math::CopySignOp>(zero, imag, fmf);
      Value negImagSignedZero =
          b.create<arith::MulFOp>(negOne, imagSignedZero, fmf);

      Value absReal = b.create<math::AbsFOp>(real, fmf);
      Value absImag = b.create<math::AbsFOp>(imag, fmf);

      Value absImagIsInf =
          b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, absImag, inf, fmf);
      Value realIsNan =
          b.create<arith::CmpFOp>(arith::CmpFPredicate::UNO, real, real, fmf);
      Value realIsInf =
          b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, absReal, inf, fmf);
      Value inIsNanInf = b.create<arith::AndIOp>(absImagIsInf, realIsNan);

      Value resultIsZero = b.create<arith::OrIOp>(inIsNanInf, realIsInf);

      resultReal =
          b.create<arith::SelectOp>(resultIsZero, realSignedZero, resultReal);
      resultImag = b.create<arith::SelectOp>(resultIsZero, negImagSignedZero,
                                             resultImag);
    }

    Value isRealZero =
        b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, real, zero, fmf);
    Value isImagZero =
        b.create<arith::CmpFOp>(arith::CmpFPredicate::OEQ, imag, zero, fmf);
    Value isZero = b.create<arith::AndIOp>(isRealZero, isImagZero);

    resultReal = b.create<arith::SelectOp>(isZero, inf, resultReal);
    resultImag = b.create<arith::SelectOp>(isZero, nan, resultImag);

    rewriter.replaceOpWithNewOp<complex::CreateOp>(op, type, resultReal,
                                                   resultImag);
    return success();
  }
};

struct AngleOpConversion : public OpConversionPattern<complex::AngleOp> {
  using OpConversionPattern<complex::AngleOp>::OpConversionPattern;

  LogicalResult
  matchAndRewrite(complex::AngleOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto loc = op.getLoc();
    auto type = op.getType();
    arith::FastMathFlagsAttr fmf = op.getFastMathFlagsAttr();

    Value real =
        rewriter.create<complex::ReOp>(loc, type, adaptor.getComplex());
    Value imag =
        rewriter.create<complex::ImOp>(loc, type, adaptor.getComplex());

    rewriter.replaceOpWithNewOp<math::Atan2Op>(op, imag, real, fmf);

    return success();
  }
};

} // namespace

void mlir::populateComplexToStandardConversionPatterns(
    RewritePatternSet &patterns, complex::ComplexRangeFlags complexRange) {
  // clang-format off
  patterns.add<
      AbsOpConversion,
      AngleOpConversion,
      Atan2OpConversion,
      BinaryComplexOpConversion<complex::AddOp, arith::AddFOp>,
      BinaryComplexOpConversion<complex::SubOp, arith::SubFOp>,
      ComparisonOpConversion<complex::EqualOp, arith::CmpFPredicate::OEQ>,
      ComparisonOpConversion<complex::NotEqualOp, arith::CmpFPredicate::UNE>,
      ConjOpConversion,
      CosOpConversion,
      ExpOpConversion,
      Expm1OpConversion,
      Log1pOpConversion,
      LogOpConversion,
      MulOpConversion,
      NegOpConversion,
      SignOpConversion,
      SinOpConversion,
      SqrtOpConversion,
      TanTanhOpConversion<complex::TanOp>,
      TanTanhOpConversion<complex::TanhOp>,
      PowOpConversion,
      RsqrtOpConversion
  >(patterns.getContext());

    patterns.add<DivOpConversion>(patterns.getContext(), complexRange);

  // clang-format on
}

namespace {
struct ConvertComplexToStandardPass
    : public impl::ConvertComplexToStandardPassBase<
          ConvertComplexToStandardPass> {
  using Base::Base;

  void runOnOperation() override;
};

void ConvertComplexToStandardPass::runOnOperation() {
  // Convert to the Standard dialect using the converter defined above.
  RewritePatternSet patterns(&getContext());
  populateComplexToStandardConversionPatterns(patterns, complexRange);

  ConversionTarget target(getContext());
  target.addLegalDialect<arith::ArithDialect, math::MathDialect>();
  target.addLegalOp<complex::CreateOp, complex::ImOp, complex::ReOp>();
  if (failed(
          applyPartialConversion(getOperation(), target, std::move(patterns))))
    signalPassFailure();
}
} // namespace
