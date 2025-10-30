//===- VPlanSimplifier.cpp - Simplify VPlan recipes -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "VPlanSimplifier.h"
#include "VPlan.h"
#include "VPlanCFG.h"
#include "VPlanDominatorTree.h"
#include "VPlanHelpers.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/TypeSwitch.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/PatternMatch.h"

using namespace llvm;

#define DEBUG_TYPE "vplan"

VPRecipeSimplifier::VPRecipeSimplifier(const VPlan &Plan) {
  if (auto LoopRegion = Plan.getVectorLoopRegion()) {
    if (const auto *CanIV = dyn_cast<VPCanonicalIVPHIRecipe>(
            &LoopRegion->getEntryBasicBlock()->front())) {
      CanonicalIVTy = CanIV->getScalarType();
      return;
    }
  }

  // If there's no canonical IV, retrieve the type from the trip count
  // expression.
  auto *TC = Plan.getTripCount();
  if (TC->isLiveIn()) {
    CanonicalIVTy = TC->getLiveInIRValue()->getType();
    return;
  }
  CanonicalIVTy = cast<VPExpandSCEVRecipe>(TC)->getSCEV()->getType();
}

VPRecipeBase *VPRecipeSimplifier::SimplifyRecipe(VPRecipeBase *R) {
  Type *ResultTy =
      TypeSwitch<const VPRecipeBase *, Type *>(V->getDefiningRecipe())
          .Case<VPActiveLaneMaskPHIRecipe, VPCanonicalIVPHIRecipe,
                VPFirstOrderRecurrencePHIRecipe, VPReductionPHIRecipe,
                VPWidenPointerInductionRecipe, VPEVLBasedIVPHIRecipe>(
              [this](const auto *R) {
                // Handle header phi recipes, except VPWidenIntOrFpInduction
                // which needs special handling due it being possibly truncated.
                // TODO: consider inferring/caching type of siblings, e.g.,
                // backedge value, here and in cases below.
                return inferScalarType(R->getStartValue());
              })
          .Case<VPWidenIntOrFpInductionRecipe, VPDerivedIVRecipe>(
              [](const auto *R) { return R->getScalarType(); })
          .Case<VPReductionRecipe, VPPredInstPHIRecipe, VPWidenPHIRecipe,
                VPScalarIVStepsRecipe, VPWidenGEPRecipe, VPVectorPointerRecipe,
                VPVectorEndPointerRecipe, VPWidenCanonicalIVRecipe,
                VPPartialReductionRecipe>([this](const VPRecipeBase *R) {
            return inferScalarType(R->getOperand(0));
          })
          // VPInstructionWithType must be handled before VPInstruction.
          .Case<VPInstructionWithType, VPWidenIntrinsicRecipe,
                VPWidenCastRecipe>(
              [](const auto *R) { return R->getResultType(); })
          .Case<VPBlendRecipe, VPInstruction, VPWidenRecipe, VPReplicateRecipe,
                VPWidenCallRecipe, VPWidenMemoryRecipe, VPWidenSelectRecipe>(
              [this](const auto *R) { return inferScalarTypeForRecipe(R); })
          .Case<VPInterleaveBase>([V](const auto *R) {
            // TODO: Use info from interleave group.
            return V->getUnderlyingValue()->getType();
          })
          .Case<VPExpandSCEVRecipe>([](const VPExpandSCEVRecipe *R) {
            return R->getSCEV()->getType();
          })
          .Case<VPReductionRecipe>([this](const auto *R) {
            return inferScalarType(R->getChainOp());
          })
          .Case<VPExpressionRecipe>([this](const auto *R) {
            return inferScalarType(R->getOperandOfResultType());
          });

  assert(ResultTy && "could not infer type for the given VPValue");
  CachedTypes[V] = ResultTy;
  return ResultTy;
}
