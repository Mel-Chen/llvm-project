//===- VPlanSimplifier.h - Simplify VPlan recipes ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_VECTORIZE_VPLANSIMPLIFIER_H
#define LLVM_TRANSFORMS_VECTORIZE_VPLANSIMPLIFIER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/IR/Type.h"

namespace llvm {

class LLVMContext;
class VPValue;
class VPBlendRecipe;
class VPInstruction;
class VPWidenRecipe;
class VPWidenCallRecipe;
class VPWidenIntOrFpInductionRecipe;
class VPWidenMemoryRecipe;
struct VPWidenSelectRecipe;
class VPReplicateRecipe;
class VPRecipeBase;
class VPlan;
class Value;
class TargetTransformInfo;
class Type;

class VPRecipeSimplifier {
public:
  VPRecipeSimplifier(const VPlan &Plan);

  VPRecipeBase *SimplifyRecipe(VPRecipeBase *R);
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_VECTORIZE_VPLANSIMPLIFIER_H
