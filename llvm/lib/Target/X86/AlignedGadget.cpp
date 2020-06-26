#ifndef LLVM_TRANSFORMS_PP2_ALIGNED_H
#define LLVM_TRANSFORMS_PP2_ALIGNED_H

#define DEBUG_TYPE "pp2-aligned"

#include "llvm/Support/Debug.h"
#include "llvm/Support/Casting.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Triple.h"

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/VirtRegMap.h"
#include "llvm/CodeGen/RegisterClassInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"

#include "X86.h"
#include "X86Subtarget.h"
#include "X86InstrInfo.h"
#include "X86InstrBuilder.h"

#include <queue>
#include <vector>
#include <cstdio>

using namespace llvm;

namespace {
  struct AlignedGadgetRemoval : public MachineFunctionPass {
    static char ID; 
    AlignedGadgetRemoval() : MachineFunctionPass(ID) { };

    bool runOnMachineFunction(MachineFunction &MF);


    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesCFG();
      MachineFunctionPass::getAnalysisUsage(AU);
    }

  private:
  };
}

//------------------------------------------------------
namespace llvm{
  FunctionPass *createAlignedGadgetRemovalPass() { 
    return new AlignedGadgetRemoval(); 
  }
}


char AlignedGadgetRemoval::ID = 0;


bool AlignedGadgetRemoval::runOnMachineFunction(MachineFunction &MF) {
  for (auto &MBB : MF) {
    if (MBB.empty()) continue;
    MachineInstr &MI = MBB.begin();
    AddXorBefore(MI);
    break;
  }

  for (auto &MBB : MF) {
    for (MachineInstr &MI : MBB.instrs()) {
      if (MI.isReturn()) {
        AddXorBefore(MI);
        break;
      }
    }
  }

  return true;
}

void AlignedGadgetRemoval::AddXorBefore(MachineInstr &MI) {
  MachineBasicBlock *MBB =  MI.getParent();
  MachineFunction *MF = MBB->getParent();
  const X86Subtarget &STI = MF->getSubtarget<X86Subtarget>();
  const X86InstrInfo &TII = *STI.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  MachineInstrBuilder MIB; 


  MachineOperand r11_def = MachineOperand::CreateReg(X86::R11, true);
  MachineOperand r11_use = MachineOperand::CreateReg(X86::R11, false);

  MIB = BuildMI(*MBB, MI, DL, TII.get(X86::MOV64rm)).addOperand(r11_def)
    .addReg(0).addImm(1).addReg(0).addImm(0x28).addReg(X86::FS);

  MIB = BuildMI(*MBB, MI, DL, TII.get(X86::XOR64mr));
  addRegOffset(MIB, retAddrRegister, false, retAddrOffset);
  MIB.addOperand(r11_use);
}

#endif


