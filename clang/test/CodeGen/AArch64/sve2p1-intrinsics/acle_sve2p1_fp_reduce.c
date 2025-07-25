// NOTE: Assertions have been autogenerated by utils/update_cc_test_checks.py
// REQUIRES: aarch64-registered-target
// RUN: %clang_cc1 -triple aarch64 -target-feature +sve -target-feature +sve2 -target-feature +sve2p1 -O1 -Werror -Wall -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple aarch64 -target-feature +sme -target-feature +sme2 -target-feature +sme2p1 -O1 -Werror -Wall -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple aarch64 -target-feature +sve -target-feature +sme -target-feature +sve2p1 -O1 -Werror -Wall -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple aarch64 -target-feature +sve -target-feature +sme -target-feature +sme2p1 -O1 -Werror -Wall -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple aarch64 -target-feature +sve -target-feature +sve2 -target-feature +sve2p1 -O1 -Werror -Wall -emit-llvm -o - -x c++ %s | FileCheck %s -check-prefix=CPP-CHECK
// RUN: %clang_cc1 -DSVE_OVERLOADED_FORMS -triple aarch64 -target-feature +sve -target-feature +sve2 -target-feature +sve2p1 -O1 -Werror -Wall -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -DSVE_OVERLOADED_FORMS -triple aarch64 -target-feature +sve -target-feature +sve2 -target-feature +sve2p1 -O1 -Werror -Wall -emit-llvm -o - -x c++ %s | FileCheck %s -check-prefix=CPP-CHECK
// RUN: %clang_cc1 -triple aarch64 -target-feature +sve -target-feature +sve2 -target-feature +sve2p1 -S -disable-O0-optnone -Werror -Wall -o /dev/null %s
#include <arm_neon.h>
#include <arm_sve.h>

#ifdef SVE_OVERLOADED_FORMS
// A simple used,unused... macro, long enough to represent any SVE builtin.
#define SVE_ACLE_FUNC(A1,A2_UNUSED,A3,A4_UNUSED) A1##A3
#else
#define SVE_ACLE_FUNC(A1,A2,A3,A4) A1##A2##A3##A4
#endif

#if defined(__ARM_FEATURE_SME) && defined(__ARM_FEATURE_SVE)
#define ATTR __arm_streaming_compatible
#elif defined(__ARM_FEATURE_SME)
#define ATTR __arm_streaming
#else
#define ATTR
#endif

// FADDQV

// CHECK-LABEL: @test_svaddqv_f16(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.faddqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z16test_svaddqv_f16u10__SVBool_tu13__SVFloat16_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.faddqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
float16x8_t test_svaddqv_f16(svbool_t pg, svfloat16_t op) ATTR
{
  return SVE_ACLE_FUNC(svaddqv,,_f16,)(pg, op);
}

// CHECK-LABEL: @test_svaddqv_f32(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.faddqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z16test_svaddqv_f32u10__SVBool_tu13__SVFloat32_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.faddqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
float32x4_t test_svaddqv_f32(svbool_t pg, svfloat32_t op) ATTR
{
  return SVE_ACLE_FUNC(svaddqv,,_f32,)(pg, op);
}

// CHECK-LABEL: @test_svaddqv_f64(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.faddqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z16test_svaddqv_f64u10__SVBool_tu13__SVFloat64_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.faddqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
float64x2_t test_svaddqv_f64(svbool_t pg, svfloat64_t op) ATTR
{
  return SVE_ACLE_FUNC(svaddqv,,_f64,)(pg, op);
}


// FMAXQV

// CHECK-LABEL: @test_svmaxqv_f16(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.fmaxqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z16test_svmaxqv_f16u10__SVBool_tu13__SVFloat16_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.fmaxqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
float16x8_t test_svmaxqv_f16(svbool_t pg, svfloat16_t op) ATTR
{
  return SVE_ACLE_FUNC(svmaxqv,,_f16,)(pg, op);
}

// CHECK-LABEL: @test_svmaxqv_f32(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.fmaxqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z16test_svmaxqv_f32u10__SVBool_tu13__SVFloat32_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.fmaxqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
float32x4_t test_svmaxqv_f32(svbool_t pg, svfloat32_t op) ATTR
{
  return SVE_ACLE_FUNC(svmaxqv,,_f32,)(pg, op);
}

// CHECK-LABEL: @test_svmaxqv_f64(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.fmaxqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z16test_svmaxqv_f64u10__SVBool_tu13__SVFloat64_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.fmaxqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
float64x2_t test_svmaxqv_f64(svbool_t pg, svfloat64_t op) ATTR
{
  return SVE_ACLE_FUNC(svmaxqv,,_f64,)(pg, op);
}


// FMINQV

// CHECK-LABEL: @test_svminqv_f16(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.fminqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z16test_svminqv_f16u10__SVBool_tu13__SVFloat16_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.fminqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
float16x8_t test_svminqv_f16(svbool_t pg, svfloat16_t op) ATTR
{
  return SVE_ACLE_FUNC(svminqv,,_f16,)(pg, op);
}

// CHECK-LABEL: @test_svminqv_f32(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.fminqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z16test_svminqv_f32u10__SVBool_tu13__SVFloat32_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.fminqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
float32x4_t test_svminqv_f32(svbool_t pg, svfloat32_t op) ATTR
{
  return SVE_ACLE_FUNC(svminqv,,_f32,)(pg, op);
}

// CHECK-LABEL: @test_svminqv_f64(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.fminqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z16test_svminqv_f64u10__SVBool_tu13__SVFloat64_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.fminqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
float64x2_t test_svminqv_f64(svbool_t pg, svfloat64_t op) ATTR
{
  return SVE_ACLE_FUNC(svminqv,,_f64,)(pg, op);
}


// FMAXNMQV

// CHECK-LABEL: @test_svmaxnmqv_f16(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.fmaxnmqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z18test_svmaxnmqv_f16u10__SVBool_tu13__SVFloat16_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.fmaxnmqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
float16x8_t test_svmaxnmqv_f16(svbool_t pg, svfloat16_t op) ATTR
{
  return SVE_ACLE_FUNC(svmaxnmqv,,_f16,)(pg, op);
}

// CHECK-LABEL: @test_svmaxnmqv_f32(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.fmaxnmqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z18test_svmaxnmqv_f32u10__SVBool_tu13__SVFloat32_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.fmaxnmqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
float32x4_t test_svmaxnmqv_f32(svbool_t pg, svfloat32_t op) ATTR
{
  return SVE_ACLE_FUNC(svmaxnmqv,,_f32,)(pg, op);
}

// CHECK-LABEL: @test_svmaxnmqv_f64(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.fmaxnmqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z18test_svmaxnmqv_f64u10__SVBool_tu13__SVFloat64_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.fmaxnmqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
float64x2_t test_svmaxnmqv_f64(svbool_t pg, svfloat64_t op) ATTR
{
  return SVE_ACLE_FUNC(svmaxnmqv,,_f64,)(pg, op);
}


// FMINNMQV

// CHECK-LABEL: @test_svminnmqv_f16(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.fminnmqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z18test_svminnmqv_f16u10__SVBool_tu13__SVFloat16_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 8 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv8i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <8 x half> @llvm.aarch64.sve.fminnmqv.v8f16.nxv8f16(<vscale x 8 x i1> [[TMP0]], <vscale x 8 x half> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <8 x half> [[TMP1]]
//
float16x8_t test_svminnmqv_f16(svbool_t pg, svfloat16_t op) ATTR
{
  return SVE_ACLE_FUNC(svminnmqv,,_f16,)(pg, op);
}

// CHECK-LABEL: @test_svminnmqv_f32(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.fminnmqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z18test_svminnmqv_f32u10__SVBool_tu13__SVFloat32_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 4 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv4i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <4 x float> @llvm.aarch64.sve.fminnmqv.v4f32.nxv4f32(<vscale x 4 x i1> [[TMP0]], <vscale x 4 x float> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <4 x float> [[TMP1]]
//
float32x4_t test_svminnmqv_f32(svbool_t pg, svfloat32_t op) ATTR
{
  return SVE_ACLE_FUNC(svminnmqv,,_f32,)(pg, op);
}

// CHECK-LABEL: @test_svminnmqv_f64(
// CHECK-NEXT:  entry:
// CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.fminnmqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
// CPP-CHECK-LABEL: @_Z18test_svminnmqv_f64u10__SVBool_tu13__SVFloat64_t(
// CPP-CHECK-NEXT:  entry:
// CPP-CHECK-NEXT:    [[TMP0:%.*]] = tail call <vscale x 2 x i1> @llvm.aarch64.sve.convert.from.svbool.nxv2i1(<vscale x 16 x i1> [[PG:%.*]])
// CPP-CHECK-NEXT:    [[TMP1:%.*]] = tail call <2 x double> @llvm.aarch64.sve.fminnmqv.v2f64.nxv2f64(<vscale x 2 x i1> [[TMP0]], <vscale x 2 x double> [[OP:%.*]])
// CPP-CHECK-NEXT:    ret <2 x double> [[TMP1]]
//
float64x2_t test_svminnmqv_f64(svbool_t pg, svfloat64_t op) ATTR
{
  return SVE_ACLE_FUNC(svminnmqv,,_f64,)(pg, op);
}
