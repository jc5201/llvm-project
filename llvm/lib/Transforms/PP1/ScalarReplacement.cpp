
//===- ScalarReplAggregates.cpp - Scalar Replacement of Aggregates --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This transformation implements the well known scalar replacement of
// aggregates transformation.  This xform breaks up alloca instructions of
// structure type into individual alloca instructions for
// each member (if possible).  Then, if possible, it transforms the individual
// alloca instructions into nice clean scalar SSA form.
//
// This combines an SRoA algorithm with Mem2Reg because they
// often interact, especially for C++ programs.  As such, this code
// iterates between SRoA and Mem2Reg until we run out of things to promote.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pp1-scalarreplace"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Casting.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AssumptionCache.h"

#include <queue>
#include <vector>

using namespace llvm;

STATISTIC(NumReplaced,  "Number of aggregate allocas broken up");
STATISTIC(NumPromoted,  "Number of scalar allocas promoted to register");

namespace {
  struct SROA : public FunctionPass {
    static char ID; // Pass identification
    SROA() : FunctionPass(ID) { }

    // Entry point for the overall scalar-replacement pass
    bool runOnFunction(Function &F);

    // getAnalysisUsage - List passes required by this pass.  We also know it
    // will not alter the CFG, so say so.
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<AssumptionCacheTracker>();
      AU.addRequired<DominatorTreeWrapperPass>();
      AU.setPreservesCFG();
    }

  private:
    // Add fields and helper functions for this pass here.
    bool isAllocaPromotable(const AllocaInst*);
    bool isAllocaReplaceable(const AllocaInst*);
    void replaceAlloca(AllocaInst* InstToBeReplaced, std::vector<AllocaInst *>& PromotableAllocas, std::queue<AllocaInst *>& ReplaceableAllocas, Instruction *InsertBefore);
    bool isInstU1(const Instruction*);
    bool isInstU2(const Instruction*);
  };
}

char SROA::ID = 0;
static RegisterPass<SROA> X("pp1-scalarreplace",
			    "Scalar Replacement of Aggregates (PP1)",
			    false /* does not modify the CFG */,
			    false /* transformation, not just analysis */);


// Public interface to create the ScalarReplAggregates pass.
// This function is provided to you.
FunctionPass *createMyScalarReplAggregatesPass() { return new SROA(); }


//===----------------------------------------------------------------------===//
//                      SKELETON FUNCTION TO BE IMPLEMENTED
//===----------------------------------------------------------------------===//
//
// Function runOnFunction:
// Entry point for the overall ScalarReplAggregates function pass.
// This function is provided to you.
bool SROA::runOnFunction(Function &F) {
  bool Changed = false;

  DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  AssumptionCache &AC = getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);

  std::vector<AllocaInst *> PromotableAllocas;
  std::queue<AllocaInst *> ReplaceableAllocas;
  PromotableAllocas.clear();

  BasicBlock &NewAllocas = F.getEntryBlock();
  NewAllocas.splitBasicBlock(NewAllocas.begin());
  Instruction *InsertTarget = &NewAllocas.getInstList().back();

  for (BasicBlock &BB : F.getBasicBlockList()) {
    for (BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I) {
      if (AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
        if (SROA::isAllocaPromotable(AI))
          PromotableAllocas.push_back(AI);
        else if (SROA::isAllocaReplaceable(AI))
          ReplaceableAllocas.push(AI);
      }
    }
  }

  while(!ReplaceableAllocas.empty()) {
    AllocaInst *AI = ReplaceableAllocas.front();
    ReplaceableAllocas.pop();

    SROA::replaceAlloca(AI, PromotableAllocas, ReplaceableAllocas, InsertTarget);
    
    NumReplaced += 1;
    Changed = true;
  }

  if (!PromotableAllocas.empty()) {
    PromoteMemToReg(PromotableAllocas, DT, &AC);
    NumPromoted += PromotableAllocas.size();
    Changed = true;
  }

  return Changed;

}

bool SROA::isAllocaPromotable(const AllocaInst *AI) {
  bool isTypeFirstClass = false;
  Type* t = AI->getAllocatedType();
  if (t->isFPOrFPVectorTy() || t->isIntOrIntVectorTy() || t->isPtrOrPtrVectorTy()) {
    isTypeFirstClass = true;
  }
  else return false;

  for (const User *U : AI->users()) {
    if (const StoreInst *SI = dyn_cast<StoreInst>(U)) {
      if (SI->isVolatile()) return false;
    }
    else if (const LoadInst *LI = dyn_cast<LoadInst>(U)) {
      if (LI->isVolatile()) return false;
    }
    else {
      return false;
    }
  }

  return true;
}

bool SROA::isAllocaReplaceable(const AllocaInst *AI) {
  if (!isa<StructType>(AI->getAllocatedType()))
    return false;
  for (const User *U : AI->users()) {
    if (const Instruction *I = dyn_cast<Instruction>(U)) {
      if (isInstU1(I) || isInstU2(I))
        continue;
      else 
        return false;
    }
  }

  return true;
}

void SROA::replaceAlloca(AllocaInst* InstToBeReplaced, std::vector<AllocaInst *>& PromotableAllocas, std::queue<AllocaInst *>& ReplaceableAllocas, Instruction *InsertBefore) {
  StructType *ST = dyn_cast<StructType>(InstToBeReplaced->getAllocatedType());
  assert (ST != NULL);
  int N = ST->getNumElements();
  std::vector<AllocaInst *> NewAllocas = std::vector<AllocaInst *>(N);

  for (int i=0; i < N; i++) {
    Type *ElementType = ST->getElementType(i);
    AllocaInst *NewAlloca = new AllocaInst(ElementType, 0);
    NewAlloca->insertBefore(InsertBefore);
    NewAllocas[i] = NewAlloca;
  }

  while(!InstToBeReplaced->user_empty()) {
    Instruction* I = dyn_cast<Instruction>(*InstToBeReplaced->user_begin());
    assert (I != NULL);
    if (SROA::isInstU1(I)) {
      GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(I);
      ConstantInt *C = dyn_cast<ConstantInt>(GEPI->idx_begin() + 1);
      int ind = C->getZExtValue();
      if (GEPI->getNumIndices() == 2) {
        GEPI->replaceAllUsesWith(NewAllocas[ind]);
        GEPI->eraseFromParent();
      }
      else {
        std::vector<Value *> IdxArray = std::vector<Value*>();
        for (int j = 0; j < GEPI->getNumIndices(); j++) {
          if (j != 1) {
            IdxArray.push_back(dyn_cast<Value>(GEPI->idx_begin() + j));
          }
        }
        GetElementPtrInst *NewGEPI = GetElementPtrInst::Create(NewAllocas[ind]->getAllocatedType(), NewAllocas[ind], ArrayRef<Value*>(IdxArray));
        ReplaceInstWithInst(GEPI, NewGEPI);
      }
    }
    else if (SROA::isInstU2(I)) {
      ICmpInst *CMP = dyn_cast<ICmpInst>(I);
      Value* NullPtr = ConstantPointerNull::get(dyn_cast<PointerType>(CMP->getOperand(0)->getType()));
      Instruction* NewCMPI = CmpInst::Create(Instruction::ICmp, CMP->getInversePredicate(), NullPtr, NullPtr);
      ReplaceInstWithInst(CMP, NewCMPI);
    }
  }

  for (int i=0; i < N; i++) {
    if (SROA::isAllocaPromotable(NewAllocas[i]))
      PromotableAllocas.push_back(NewAllocas[i]);
    else if (SROA::isAllocaReplaceable(NewAllocas[i]))
      ReplaceableAllocas.push(NewAllocas[i]);
  }
}

bool SROA::isInstU1(const Instruction *I) {
  if (const GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(I)) {
    if (!GEPI->hasAllConstantIndices())
      return false;
    if (GEPI->getNumIndices() < 2)
      return false;
    if (const ConstantInt *C = dyn_cast<ConstantInt>(GEPI->idx_begin())) {
      if (C->getZExtValue() != 0)
        return false;
    }
    else return false;

    for (const User *U : GEPI->users()) {
      if (const Instruction *UI = dyn_cast<Instruction>(U)) {
        if (isInstU1(UI)) continue;
        else if (isInstU2(UI)) continue;
        else if (isa<LoadInst>(UI)) continue;
        else if (const StoreInst *SI = dyn_cast<StoreInst>(UI)){
          if (SI->getValueOperand() == I) 
            return false;
        }
        else return false;
      }
      else return false;
    }
  }
  else return false;

  return true;
}

bool SROA::isInstU2(const Instruction *I) {
  if (const ICmpInst *CMP = dyn_cast<ICmpInst>(I))
    if (CMP->isEquality())
      if (dyn_cast<ConstantPointerNull>(CMP->getOperand(0)) || dyn_cast<ConstantPointerNull>(CMP->getOperand(1)))
        return true;

  return false;
}


