
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

  BasicBlock &GeneratedAllocas = F.getEntryBlock();
  GeneratedAllocas.splitBasicBlock(GeneratedAllocas.begin());
  Instruction *T = &GeneratedAllocas.getInstList().back();

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

  if (!PromotableAllocas.empty()) {
    PromoteMemToReg(PromotableAllocas, DT, &AC);
    NumPromoted += PromotableAllocas.size();
    Changed = true;
  }

  while(!ReplaceableAllocas.empty()) {
    PromotableAllocas.clear();
    AllocaInst *AI = ReplaceableAllocas.front();
    ReplaceableAllocas.pop();

    StructType *ST = dyn_cast<StructType>(AI->getAllocatedType());
    assert (ST != NULL);
    for (unsigned i=0; i < ST->getNumElements(); i++) {
      Type *ElementType = ST->getElementType(i);

      AllocaInst *NewAlloca = new AllocaInst(ElementType, 0);
      NewAlloca->insertBefore(T);
      Value *NewAllocated = dyn_cast<Value>(NewAlloca);

      bool AIChanged = true;
      while(AIChanged) {
        AIChanged = false;
        assert (isAllocaReplaceable(AI));
        for (User* U : AI->users()) {
          Instruction* I = dyn_cast<Instruction>(U);
          assert (I != NULL);
          if (SROA::isInstU1(I)) {
            GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(I);
            ConstantInt *C = dyn_cast<ConstantInt>(GEPI->idx_begin() + 1);
            if (C->getZExtValue() == i) {
              std::vector<Value *> IdxArray = std::vector<Value*>();
              for (int j = 0; j < GEPI->getNumIndices(); j++) {
                if (j != 1) {
                  IdxArray.push_back(dyn_cast<Value>(GEPI->idx_begin() + 1));
                }
              }
              GetElementPtrInst *NewGEPI = GetElementPtrInst::Create(ElementType, NewAllocated, ArrayRef<Value*>(IdxArray));
              ReplaceInstWithInst(GEPI, NewGEPI);
              AIChanged = true;
              break;
            }
          }
          else if (SROA::isInstU2(I)) {
            ICmpInst *CMP = dyn_cast<ICmpInst>(I);
            Value* NullPtr = ConstantPointerNull::get(dyn_cast<PointerType>(CMP->getOperand(0)->getType()));
            Instruction* NewCMPI = CmpInst::Create(Instruction::ICmp, CMP->getInversePredicate(), NullPtr, NullPtr);
            ReplaceInstWithInst(CMP, NewCMPI);
            AIChanged = true;
            break;
          }
        }
      }

      if (SROA::isAllocaPromotable(NewAlloca))
        PromotableAllocas.push_back(NewAlloca);
      else if (SROA::isAllocaReplaceable(NewAlloca))
        ReplaceableAllocas.push(NewAlloca);
    }

    if (!PromotableAllocas.empty()) {
      PromoteMemToReg(PromotableAllocas, DT, &AC);
      NumPromoted += PromotableAllocas.size();
    }

    NumReplaced += 1;
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
  if (!dyn_cast<StructType>(AI->getAllocatedType()))
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

bool SROA::isInstU1(const Instruction *I) {
  if (const GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(I)) {
    if (!GEPI->hasAllConstantIndices())
      return false;
    if (const ConstantInt *C = dyn_cast<ConstantInt>(GEPI->idx_begin())) {
      if (C->getZExtValue() != 0)
        return false;
    }
    else return false;

    for (const User *U : GEPI->users()) {
      if (const Instruction *I = dyn_cast<Instruction>(U)) {
        if (isInstU1(I)) continue;
        else if (isInstU2(I)) continue;
        else if (const LoadInst *LI = dyn_cast<LoadInst>(U)) continue;
        else if (const StoreInst *SI = dyn_cast<StoreInst>(U)){
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


