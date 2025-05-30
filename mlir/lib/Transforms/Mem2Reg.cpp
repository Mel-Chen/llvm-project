//===- Mem2Reg.cpp - Promotes memory slots into values ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mlir/Transforms/Mem2Reg.h"
#include "mlir/Analysis/DataLayoutAnalysis.h"
#include "mlir/Analysis/SliceAnalysis.h"
#include "mlir/Analysis/TopologicalSortUtils.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Dominance.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/RegionKindInterface.h"
#include "mlir/IR/Value.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Interfaces/MemorySlotInterfaces.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/GenericIteratedDominanceFrontier.h"

namespace mlir {
#define GEN_PASS_DEF_MEM2REG
#include "mlir/Transforms/Passes.h.inc"
} // namespace mlir

#define DEBUG_TYPE "mem2reg"

using namespace mlir;

/// mem2reg
///
/// This pass turns unnecessary uses of automatically allocated memory slots
/// into direct Value-based operations. For example, it will simplify storing a
/// constant in a memory slot to immediately load it to a direct use of that
/// constant. In other words, given a memory slot addressed by a non-aliased
/// "pointer" Value, mem2reg removes all the uses of that pointer.
///
/// Within a block, this is done by following the chain of stores and loads of
/// the slot and replacing the results of loads with the values previously
/// stored. If a load happens before any other store, a poison value is used
/// instead.
///
/// Control flow can create situations where a load could be replaced by
/// multiple possible stores depending on the control flow path taken. As a
/// result, this pass must introduce new block arguments in some blocks to
/// accommodate for the multiple possible definitions. Each predecessor will
/// populate the block argument with the definition reached at its end. With
/// this, the value stored can be well defined at block boundaries, allowing
/// the propagation of replacement through blocks.
///
/// This pass computes this transformation in four main steps. The two first
/// steps are performed during an analysis phase that does not mutate IR.
///
/// The two steps of the analysis phase are the following:
/// - A first step computes the list of operations that transitively use the
/// memory slot we would like to promote. The purpose of this phase is to
/// identify which uses must be removed to promote the slot, either by rewiring
/// the user or deleting it. Naturally, direct uses of the slot must be removed.
/// Sometimes additional uses must also be removed: this is notably the case
/// when a direct user of the slot cannot rewire its use and must delete itself,
/// and thus must make its users no longer use it. If any of those uses cannot
/// be removed by their users in any way, promotion cannot continue: this is
/// decided at this step.
/// - A second step computes the list of blocks where a block argument will be
/// needed ("merge points") without mutating the IR. These blocks are the blocks
/// leading to a definition clash between two predecessors. Such blocks happen
/// to be the Iterated Dominance Frontier (IDF) of the set of blocks containing
/// a store, as they represent the point where a clear defining dominator stops
/// existing. Computing this information in advance allows making sure the
/// terminators that will forward values are capable of doing so (inability to
/// do so aborts promotion at this step).
///
/// At this point, promotion is guaranteed to happen, and the mutation phase can
/// begin with the following steps:
/// - A third step computes the reaching definition of the memory slot at each
/// blocking user. This is the core of the mem2reg algorithm, also known as
/// load-store forwarding. This analyses loads and stores and propagates which
/// value must be stored in the slot at each blocking user.  This is achieved by
/// doing a depth-first walk of the dominator tree of the function. This is
/// sufficient because the reaching definition at the beginning of a block is
/// either its new block argument if it is a merge block, or the definition
/// reaching the end of its immediate dominator (parent in the dominator tree).
/// We can therefore propagate this information down the dominator tree to
/// proceed with renaming within blocks.
/// - The final fourth step uses the reaching definition to remove blocking uses
/// in topological order.
///
/// For further reading, chapter three of SSA-based Compiler Design [1]
/// showcases SSA construction, where mem2reg is an adaptation of the same
/// process.
///
/// [1]: Rastello F. & Bouchez Tichadou F., SSA-based Compiler Design (2022),
///      Springer.

namespace {

using BlockingUsesMap =
    llvm::MapVector<Operation *, SmallPtrSet<OpOperand *, 4>>;

/// Information computed during promotion analysis used to perform actual
/// promotion.
struct MemorySlotPromotionInfo {
  /// Blocks for which at least two definitions of the slot values clash.
  SmallPtrSet<Block *, 8> mergePoints;
  /// Contains, for each operation, which uses must be eliminated by promotion.
  /// This is a DAG structure because if an operation must eliminate some of
  /// its uses, it is because the defining ops of the blocking uses requested
  /// it. The defining ops therefore must also have blocking uses or be the
  /// starting point of the blocking uses.
  BlockingUsesMap userToBlockingUses;
};

/// Computes information for basic slot promotion. This will check that direct
/// slot promotion can be performed, and provide the information to execute the
/// promotion. This does not mutate IR.
class MemorySlotPromotionAnalyzer {
public:
  MemorySlotPromotionAnalyzer(MemorySlot slot, DominanceInfo &dominance,
                              const DataLayout &dataLayout)
      : slot(slot), dominance(dominance), dataLayout(dataLayout) {}

  /// Computes the information for slot promotion if promotion is possible,
  /// returns nothing otherwise.
  std::optional<MemorySlotPromotionInfo> computeInfo();

private:
  /// Computes the transitive uses of the slot that block promotion. This finds
  /// uses that would block the promotion, checks that the operation has a
  /// solution to remove the blocking use, and potentially forwards the analysis
  /// if the operation needs further blocking uses resolved to resolve its own
  /// uses (typically, removing its users because it will delete itself to
  /// resolve its own blocking uses). This will fail if one of the transitive
  /// users cannot remove a requested use, and should prevent promotion.
  LogicalResult computeBlockingUses(BlockingUsesMap &userToBlockingUses);

  /// Computes in which blocks the value stored in the slot is actually used,
  /// meaning blocks leading to a load. This method uses `definingBlocks`, the
  /// set of blocks containing a store to the slot (defining the value of the
  /// slot).
  SmallPtrSet<Block *, 16>
  computeSlotLiveIn(SmallPtrSetImpl<Block *> &definingBlocks);

  /// Computes the points in which multiple re-definitions of the slot's value
  /// (stores) may conflict.
  void computeMergePoints(SmallPtrSetImpl<Block *> &mergePoints);

  /// Ensures predecessors of merge points can properly provide their current
  /// definition of the value stored in the slot to the merge point. This can
  /// notably be an issue if the terminator used does not have the ability to
  /// forward values through block operands.
  bool areMergePointsUsable(SmallPtrSetImpl<Block *> &mergePoints);

  MemorySlot slot;
  DominanceInfo &dominance;
  const DataLayout &dataLayout;
};

using BlockIndexCache = DenseMap<Region *, DenseMap<Block *, size_t>>;

/// The MemorySlotPromoter handles the state of promoting a memory slot. It
/// wraps a slot and its associated allocator. This will perform the mutation of
/// IR.
class MemorySlotPromoter {
public:
  MemorySlotPromoter(MemorySlot slot, PromotableAllocationOpInterface allocator,
                     OpBuilder &builder, DominanceInfo &dominance,
                     const DataLayout &dataLayout, MemorySlotPromotionInfo info,
                     const Mem2RegStatistics &statistics,
                     BlockIndexCache &blockIndexCache);

  /// Actually promotes the slot by mutating IR. Promoting a slot DOES
  /// invalidate the MemorySlotPromotionInfo of other slots. Preparation of
  /// promotion info should NOT be performed in batches.
  /// Returns a promotable allocation op if a new allocator was created, nullopt
  /// otherwise.
  std::optional<PromotableAllocationOpInterface> promoteSlot();

private:
  /// Computes the reaching definition for all the operations that require
  /// promotion. `reachingDef` is the value the slot should contain at the
  /// beginning of the block. This method returns the reached definition at the
  /// end of the block. This method must only be called at most once per block.
  Value computeReachingDefInBlock(Block *block, Value reachingDef);

  /// Computes the reaching definition for all the operations that require
  /// promotion. `reachingDef` corresponds to the initial value the
  /// slot will contain before any write, typically a poison value.
  /// This method must only be called at most once per region.
  void computeReachingDefInRegion(Region *region, Value reachingDef);

  /// Removes the blocking uses of the slot, in topological order.
  void removeBlockingUses();

  /// Lazily-constructed default value representing the content of the slot when
  /// no store has been executed. This function may mutate IR.
  Value getOrCreateDefaultValue();

  MemorySlot slot;
  PromotableAllocationOpInterface allocator;
  OpBuilder &builder;
  /// Potentially non-initialized default value. Use `getOrCreateDefaultValue`
  /// to initialize it on demand.
  Value defaultValue;
  /// Contains the reaching definition at this operation. Reaching definitions
  /// are only computed for promotable memory operations with blocking uses.
  DenseMap<PromotableMemOpInterface, Value> reachingDefs;
  DenseMap<PromotableMemOpInterface, Value> replacedValuesMap;
  DominanceInfo &dominance;
  const DataLayout &dataLayout;
  MemorySlotPromotionInfo info;
  const Mem2RegStatistics &statistics;

  /// Shared cache of block indices of specific regions.
  BlockIndexCache &blockIndexCache;
};

} // namespace

MemorySlotPromoter::MemorySlotPromoter(
    MemorySlot slot, PromotableAllocationOpInterface allocator,
    OpBuilder &builder, DominanceInfo &dominance, const DataLayout &dataLayout,
    MemorySlotPromotionInfo info, const Mem2RegStatistics &statistics,
    BlockIndexCache &blockIndexCache)
    : slot(slot), allocator(allocator), builder(builder), dominance(dominance),
      dataLayout(dataLayout), info(std::move(info)), statistics(statistics),
      blockIndexCache(blockIndexCache) {
#ifndef NDEBUG
  auto isResultOrNewBlockArgument = [&]() {
    if (BlockArgument arg = dyn_cast<BlockArgument>(slot.ptr))
      return arg.getOwner()->getParentOp() == allocator;
    return slot.ptr.getDefiningOp() == allocator;
  };

  assert(isResultOrNewBlockArgument() &&
         "a slot must be a result of the allocator or an argument of the child "
         "regions of the allocator");
#endif // NDEBUG
}

Value MemorySlotPromoter::getOrCreateDefaultValue() {
  if (defaultValue)
    return defaultValue;

  OpBuilder::InsertionGuard guard(builder);
  builder.setInsertionPointToStart(slot.ptr.getParentBlock());
  return defaultValue = allocator.getDefaultValue(slot, builder);
}

LogicalResult MemorySlotPromotionAnalyzer::computeBlockingUses(
    BlockingUsesMap &userToBlockingUses) {
  // The promotion of an operation may require the promotion of further
  // operations (typically, removing operations that use an operation that must
  // delete itself). We thus need to start from the use of the slot pointer and
  // propagate further requests through the forward slice.

  // Because this pass currently only supports analysing the parent region of
  // the slot pointer, if a promotable memory op that needs promotion is within
  // a graph region, the slot may only be used in a graph region and should
  // therefore be ignored.
  Region *slotPtrRegion = slot.ptr.getParentRegion();
  auto slotPtrRegionOp =
      dyn_cast<RegionKindInterface>(slotPtrRegion->getParentOp());
  if (slotPtrRegionOp &&
      slotPtrRegionOp.getRegionKind(slotPtrRegion->getRegionNumber()) ==
          RegionKind::Graph)
    return failure();

  // First insert that all immediate users of the slot pointer must no longer
  // use it.
  for (OpOperand &use : slot.ptr.getUses()) {
    SmallPtrSet<OpOperand *, 4> &blockingUses =
        userToBlockingUses[use.getOwner()];
    blockingUses.insert(&use);
  }

  // Then, propagate the requirements for the removal of uses. The
  // topologically-sorted forward slice allows for all blocking uses of an
  // operation to have been computed before it is reached. Operations are
  // traversed in topological order of their uses, starting from the slot
  // pointer.
  SetVector<Operation *> forwardSlice;
  mlir::getForwardSlice(slot.ptr, &forwardSlice);
  for (Operation *user : forwardSlice) {
    // If the next operation has no blocking uses, everything is fine.
    auto it = userToBlockingUses.find(user);
    if (it == userToBlockingUses.end())
      continue;

    SmallPtrSet<OpOperand *, 4> &blockingUses = it->second;

    SmallVector<OpOperand *> newBlockingUses;
    // If the operation decides it cannot deal with removing the blocking uses,
    // promotion must fail.
    if (auto promotable = dyn_cast<PromotableOpInterface>(user)) {
      if (!promotable.canUsesBeRemoved(blockingUses, newBlockingUses,
                                       dataLayout))
        return failure();
    } else if (auto promotable = dyn_cast<PromotableMemOpInterface>(user)) {
      if (!promotable.canUsesBeRemoved(slot, blockingUses, newBlockingUses,
                                       dataLayout))
        return failure();
    } else {
      // An operation that has blocking uses must be promoted. If it is not
      // promotable, promotion must fail.
      return failure();
    }

    // Then, register any new blocking uses for coming operations.
    for (OpOperand *blockingUse : newBlockingUses) {
      assert(llvm::is_contained(user->getResults(), blockingUse->get()));

      SmallPtrSetImpl<OpOperand *> &newUserBlockingUseSet =
          userToBlockingUses[blockingUse->getOwner()];
      newUserBlockingUseSet.insert(blockingUse);
    }
  }

  // Because this pass currently only supports analysing the parent region of
  // the slot pointer, if a promotable memory op that needs promotion is outside
  // of this region, promotion must fail because it will be impossible to
  // provide a valid `reachingDef` for it.
  for (auto &[toPromote, _] : userToBlockingUses)
    if (isa<PromotableMemOpInterface>(toPromote) &&
        toPromote->getParentRegion() != slot.ptr.getParentRegion())
      return failure();

  return success();
}

SmallPtrSet<Block *, 16> MemorySlotPromotionAnalyzer::computeSlotLiveIn(
    SmallPtrSetImpl<Block *> &definingBlocks) {
  SmallPtrSet<Block *, 16> liveIn;

  // The worklist contains blocks in which it is known that the slot value is
  // live-in. The further blocks where this value is live-in will be inferred
  // from these.
  SmallVector<Block *> liveInWorkList;

  // Blocks with a load before any other store to the slot are the starting
  // points of the analysis. The slot value is definitely live-in in those
  // blocks.
  SmallPtrSet<Block *, 16> visited;
  for (Operation *user : slot.ptr.getUsers()) {
    if (!visited.insert(user->getBlock()).second)
      continue;

    for (Operation &op : user->getBlock()->getOperations()) {
      if (auto memOp = dyn_cast<PromotableMemOpInterface>(op)) {
        // If this operation loads the slot, it is loading from it before
        // ever writing to it, so the value is live-in in this block.
        if (memOp.loadsFrom(slot)) {
          liveInWorkList.push_back(user->getBlock());
          break;
        }

        // If we store to the slot, further loads will see that value.
        // Because we did not meet any load before, the value is not live-in.
        if (memOp.storesTo(slot))
          break;
      }
    }
  }

  // The information is then propagated to the predecessors until a def site
  // (store) is found.
  while (!liveInWorkList.empty()) {
    Block *liveInBlock = liveInWorkList.pop_back_val();

    if (!liveIn.insert(liveInBlock).second)
      continue;

    // If a predecessor is a defining block, either:
    // - It has a load before its first store, in which case it is live-in but
    // has already been processed in the initialisation step.
    // - It has a store before any load, in which case it is not live-in.
    // We can thus at this stage insert to the worklist only predecessors that
    // are not defining blocks.
    for (Block *pred : liveInBlock->getPredecessors())
      if (!definingBlocks.contains(pred))
        liveInWorkList.push_back(pred);
  }

  return liveIn;
}

using IDFCalculator = llvm::IDFCalculatorBase<Block, false>;
void MemorySlotPromotionAnalyzer::computeMergePoints(
    SmallPtrSetImpl<Block *> &mergePoints) {
  if (slot.ptr.getParentRegion()->hasOneBlock())
    return;

  IDFCalculator idfCalculator(dominance.getDomTree(slot.ptr.getParentRegion()));

  SmallPtrSet<Block *, 16> definingBlocks;
  for (Operation *user : slot.ptr.getUsers())
    if (auto storeOp = dyn_cast<PromotableMemOpInterface>(user))
      if (storeOp.storesTo(slot))
        definingBlocks.insert(user->getBlock());

  idfCalculator.setDefiningBlocks(definingBlocks);

  SmallPtrSet<Block *, 16> liveIn = computeSlotLiveIn(definingBlocks);
  idfCalculator.setLiveInBlocks(liveIn);

  SmallVector<Block *> mergePointsVec;
  idfCalculator.calculate(mergePointsVec);

  mergePoints.insert_range(mergePointsVec);
}

bool MemorySlotPromotionAnalyzer::areMergePointsUsable(
    SmallPtrSetImpl<Block *> &mergePoints) {
  for (Block *mergePoint : mergePoints)
    for (Block *pred : mergePoint->getPredecessors())
      if (!isa<BranchOpInterface>(pred->getTerminator()))
        return false;

  return true;
}

std::optional<MemorySlotPromotionInfo>
MemorySlotPromotionAnalyzer::computeInfo() {
  MemorySlotPromotionInfo info;

  // First, find the set of operations that will need to be changed for the
  // promotion to happen. These operations need to resolve some of their uses,
  // either by rewiring them or simply deleting themselves. If any of them
  // cannot find a way to resolve their blocking uses, we abort the promotion.
  if (failed(computeBlockingUses(info.userToBlockingUses)))
    return {};

  // Then, compute blocks in which two or more definitions of the allocated
  // variable may conflict. These blocks will need a new block argument to
  // accommodate this.
  computeMergePoints(info.mergePoints);

  // The slot can be promoted if the block arguments to be created can
  // actually be populated with values, which may not be possible depending
  // on their predecessors.
  if (!areMergePointsUsable(info.mergePoints))
    return {};

  return info;
}

Value MemorySlotPromoter::computeReachingDefInBlock(Block *block,
                                                    Value reachingDef) {
  SmallVector<Operation *> blockOps;
  for (Operation &op : block->getOperations())
    blockOps.push_back(&op);
  for (Operation *op : blockOps) {
    if (auto memOp = dyn_cast<PromotableMemOpInterface>(op)) {
      if (info.userToBlockingUses.contains(memOp))
        reachingDefs.insert({memOp, reachingDef});

      if (memOp.storesTo(slot)) {
        builder.setInsertionPointAfter(memOp);
        Value stored = memOp.getStored(slot, builder, reachingDef, dataLayout);
        assert(stored && "a memory operation storing to a slot must provide a "
                         "new definition of the slot");
        reachingDef = stored;
        replacedValuesMap[memOp] = stored;
      }
    }
  }

  return reachingDef;
}

void MemorySlotPromoter::computeReachingDefInRegion(Region *region,
                                                    Value reachingDef) {
  assert(reachingDef && "expected an initial reaching def to be provided");
  if (region->hasOneBlock()) {
    computeReachingDefInBlock(&region->front(), reachingDef);
    return;
  }

  struct DfsJob {
    llvm::DomTreeNodeBase<Block> *block;
    Value reachingDef;
  };

  SmallVector<DfsJob> dfsStack;

  auto &domTree = dominance.getDomTree(slot.ptr.getParentRegion());

  dfsStack.emplace_back<DfsJob>(
      {domTree.getNode(&region->front()), reachingDef});

  while (!dfsStack.empty()) {
    DfsJob job = dfsStack.pop_back_val();
    Block *block = job.block->getBlock();

    if (info.mergePoints.contains(block)) {
      BlockArgument blockArgument =
          block->addArgument(slot.elemType, slot.ptr.getLoc());
      builder.setInsertionPointToStart(block);
      allocator.handleBlockArgument(slot, blockArgument, builder);
      job.reachingDef = blockArgument;

      if (statistics.newBlockArgumentAmount)
        (*statistics.newBlockArgumentAmount)++;
    }

    job.reachingDef = computeReachingDefInBlock(block, job.reachingDef);
    assert(job.reachingDef);

    if (auto terminator = dyn_cast<BranchOpInterface>(block->getTerminator())) {
      for (BlockOperand &blockOperand : terminator->getBlockOperands()) {
        if (info.mergePoints.contains(blockOperand.get())) {
          terminator.getSuccessorOperands(blockOperand.getOperandNumber())
              .append(job.reachingDef);
        }
      }
    }

    for (auto *child : job.block->children())
      dfsStack.emplace_back<DfsJob>({child, job.reachingDef});
  }
}

/// Gets or creates a block index mapping for `region`.
static const DenseMap<Block *, size_t> &
getOrCreateBlockIndices(BlockIndexCache &blockIndexCache, Region *region) {
  auto [it, inserted] = blockIndexCache.try_emplace(region);
  if (!inserted)
    return it->second;

  DenseMap<Block *, size_t> &blockIndices = it->second;
  SetVector<Block *> topologicalOrder = getBlocksSortedByDominance(*region);
  for (auto [index, block] : llvm::enumerate(topologicalOrder))
    blockIndices[block] = index;
  return blockIndices;
}

/// Sorts `ops` according to dominance. Relies on the topological order of basic
/// blocks to get a deterministic ordering. Uses `blockIndexCache` to avoid the
/// potentially expensive recomputation of a block index map.
static void dominanceSort(SmallVector<Operation *> &ops, Region &region,
                          BlockIndexCache &blockIndexCache) {
  // Produce a topological block order and construct a map to lookup the indices
  // of blocks.
  const DenseMap<Block *, size_t> &topoBlockIndices =
      getOrCreateBlockIndices(blockIndexCache, &region);

  // Combining the topological order of the basic blocks together with block
  // internal operation order guarantees a deterministic, dominance respecting
  // order.
  llvm::sort(ops, [&](Operation *lhs, Operation *rhs) {
    size_t lhsBlockIndex = topoBlockIndices.at(lhs->getBlock());
    size_t rhsBlockIndex = topoBlockIndices.at(rhs->getBlock());
    if (lhsBlockIndex == rhsBlockIndex)
      return lhs->isBeforeInBlock(rhs);
    return lhsBlockIndex < rhsBlockIndex;
  });
}

void MemorySlotPromoter::removeBlockingUses() {
  llvm::SmallVector<Operation *> usersToRemoveUses(
      llvm::make_first_range(info.userToBlockingUses));

  // Sort according to dominance.
  dominanceSort(usersToRemoveUses, *slot.ptr.getParentBlock()->getParent(),
                blockIndexCache);

  llvm::SmallVector<Operation *> toErase;
  // List of all replaced values in the slot.
  llvm::SmallVector<std::pair<Operation *, Value>> replacedValuesList;
  // Ops to visit with the `visitReplacedValues` method.
  llvm::SmallVector<PromotableOpInterface> toVisit;
  for (Operation *toPromote : llvm::reverse(usersToRemoveUses)) {
    if (auto toPromoteMemOp = dyn_cast<PromotableMemOpInterface>(toPromote)) {
      Value reachingDef = reachingDefs.lookup(toPromoteMemOp);
      // If no reaching definition is known, this use is outside the reach of
      // the slot. The default value should thus be used.
      if (!reachingDef)
        reachingDef = getOrCreateDefaultValue();

      builder.setInsertionPointAfter(toPromote);
      if (toPromoteMemOp.removeBlockingUses(
              slot, info.userToBlockingUses[toPromote], builder, reachingDef,
              dataLayout) == DeletionKind::Delete)
        toErase.push_back(toPromote);
      if (toPromoteMemOp.storesTo(slot))
        if (Value replacedValue = replacedValuesMap[toPromoteMemOp])
          replacedValuesList.push_back({toPromoteMemOp, replacedValue});
      continue;
    }

    auto toPromoteBasic = cast<PromotableOpInterface>(toPromote);
    builder.setInsertionPointAfter(toPromote);
    if (toPromoteBasic.removeBlockingUses(info.userToBlockingUses[toPromote],
                                          builder) == DeletionKind::Delete)
      toErase.push_back(toPromote);
    if (toPromoteBasic.requiresReplacedValues())
      toVisit.push_back(toPromoteBasic);
  }
  for (PromotableOpInterface op : toVisit) {
    builder.setInsertionPointAfter(op);
    op.visitReplacedValues(replacedValuesList, builder);
  }

  for (Operation *toEraseOp : toErase)
    toEraseOp->erase();

  assert(slot.ptr.use_empty() &&
         "after promotion, the slot pointer should not be used anymore");
}

std::optional<PromotableAllocationOpInterface>
MemorySlotPromoter::promoteSlot() {
  computeReachingDefInRegion(slot.ptr.getParentRegion(),
                             getOrCreateDefaultValue());

  // Now that reaching definitions are known, remove all users.
  removeBlockingUses();

  // Update terminators in dead branches to forward default if they are
  // succeeded by a merge points.
  for (Block *mergePoint : info.mergePoints) {
    for (BlockOperand &use : mergePoint->getUses()) {
      auto user = cast<BranchOpInterface>(use.getOwner());
      SuccessorOperands succOperands =
          user.getSuccessorOperands(use.getOperandNumber());
      assert(succOperands.size() == mergePoint->getNumArguments() ||
             succOperands.size() + 1 == mergePoint->getNumArguments());
      if (succOperands.size() + 1 == mergePoint->getNumArguments())
        succOperands.append(getOrCreateDefaultValue());
    }
  }

  LLVM_DEBUG(llvm::dbgs() << "[mem2reg] Promoted memory slot: " << slot.ptr
                          << "\n");

  if (statistics.promotedAmount)
    (*statistics.promotedAmount)++;

  return allocator.handlePromotionComplete(slot, defaultValue, builder);
}

LogicalResult mlir::tryToPromoteMemorySlots(
    ArrayRef<PromotableAllocationOpInterface> allocators, OpBuilder &builder,
    const DataLayout &dataLayout, DominanceInfo &dominance,
    Mem2RegStatistics statistics) {
  bool promotedAny = false;

  // A cache that stores deterministic block indices which are used to determine
  // a valid operation modification order. The block index maps are computed
  // lazily and cached to avoid expensive recomputation.
  BlockIndexCache blockIndexCache;

  SmallVector<PromotableAllocationOpInterface> workList(allocators);

  SmallVector<PromotableAllocationOpInterface> newWorkList;
  newWorkList.reserve(workList.size());
  while (true) {
    bool changesInThisRound = false;
    for (PromotableAllocationOpInterface allocator : workList) {
      bool changedAllocator = false;
      for (MemorySlot slot : allocator.getPromotableSlots()) {
        if (slot.ptr.use_empty())
          continue;

        MemorySlotPromotionAnalyzer analyzer(slot, dominance, dataLayout);
        std::optional<MemorySlotPromotionInfo> info = analyzer.computeInfo();
        if (info) {
          std::optional<PromotableAllocationOpInterface> newAllocator =
              MemorySlotPromoter(slot, allocator, builder, dominance,
                                 dataLayout, std::move(*info), statistics,
                                 blockIndexCache)
                  .promoteSlot();
          changedAllocator = true;
          // Add newly created allocators to the worklist for further
          // processing.
          if (newAllocator)
            newWorkList.push_back(*newAllocator);

          // A break is required, since promoting a slot may invalidate the
          // remaining slots of an allocator.
          break;
        }
      }
      if (!changedAllocator)
        newWorkList.push_back(allocator);
      changesInThisRound |= changedAllocator;
    }
    if (!changesInThisRound)
      break;
    promotedAny = true;

    // Swap the vector's backing memory and clear the entries in newWorkList
    // afterwards. This ensures that additional heap allocations can be avoided.
    workList.swap(newWorkList);
    newWorkList.clear();
  }

  return success(promotedAny);
}

namespace {

struct Mem2Reg : impl::Mem2RegBase<Mem2Reg> {
  using impl::Mem2RegBase<Mem2Reg>::Mem2RegBase;

  void runOnOperation() override {
    Operation *scopeOp = getOperation();

    Mem2RegStatistics statistics{&promotedAmount, &newBlockArgumentAmount};

    bool changed = false;

    auto &dataLayoutAnalysis = getAnalysis<DataLayoutAnalysis>();
    const DataLayout &dataLayout = dataLayoutAnalysis.getAtOrAbove(scopeOp);
    auto &dominance = getAnalysis<DominanceInfo>();

    for (Region &region : scopeOp->getRegions()) {
      if (region.getBlocks().empty())
        continue;

      OpBuilder builder(&region.front(), region.front().begin());

      SmallVector<PromotableAllocationOpInterface> allocators;
      // Build a list of allocators to attempt to promote the slots of.
      region.walk([&](PromotableAllocationOpInterface allocator) {
        allocators.emplace_back(allocator);
      });

      // Attempt promoting as many of the slots as possible.
      if (succeeded(tryToPromoteMemorySlots(allocators, builder, dataLayout,
                                            dominance, statistics)))
        changed = true;
    }
    if (!changed)
      markAllAnalysesPreserved();
  }
};

} // namespace
