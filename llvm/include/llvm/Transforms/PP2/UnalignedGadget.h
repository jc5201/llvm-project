
#ifndef LLVM_TRANSFORMS_PP2_H
#define LLVM_TRANSFORMS_PP2_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Casting.h"
#include "llvm/ADT/Statistic.h"

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

#include <queue>
#include <vector>

using namespace llvm;

namespace llvm{
  struct UnalignedGadgetRemoval : public MachineFunctionPass {
    static char ID; // Pass identification
    UnalignedGadgetRemoval() : MachineFunctionPass(ID) { };

    // Entry point for the overall scalar-replacement pass
    bool runOnMachineFunction(MachineFunction &MF);

  private:
    // Add fields and helper functions for this pass here.
  };
}


#endif

