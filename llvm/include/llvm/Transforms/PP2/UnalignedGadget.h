#ifndef LLVM_TRANSFORMS_PP2_H
#define LLVM_TRANSFORMS_PP2_H

#include "llvm/Support/Debug.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TargetOpcodes.def"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Triple.h"

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"

#include <queue>
#include <vector>

using namespace llvm;

STATISTIC(NumJmp,  "Number of vulnerable jmp instructions");

namespace llvm{
  struct UnalignedGadgetRemoval : public MachineFunctionPass {
    static char ID; // Pass identification
    UnalignedGadgetRemoval() : MachineFunctionPass(ID) { };

    // Entry point for the overall scalar-replacement pass
    bool runOnMachineFunction(MachineFunction &MF);

  private:
  	bool isVulnerableJmp(MachineInstr *MI);
    // Add fields and helper functions for this pass here.
  };
}

FunctionPass *createUnalignedGadgetRemovalPass() { return new UnalignedGadgetRemoval(); }


#endif

