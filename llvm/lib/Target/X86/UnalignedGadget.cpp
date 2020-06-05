#ifndef LLVM_TRANSFORMS_PP2_H
#define LLVM_TRANSFORMS_PP2_H

#define DEBUG_TYPE "pp2"

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

#include "X86.h"
#include "X86Subtarget.h"
#include "X86InstrInfo.h"
#include "X86InstrBuilder.h"

#include <queue>
#include <vector>

using namespace llvm;

STATISTIC(NumJmp,  "Number of vulnerable jmp instructions");

namespace {
  struct UnalignedGadgetRemoval : public MachineFunctionPass {
    static char ID; // Pass identification
    UnalignedGadgetRemoval() : MachineFunctionPass(ID) { };

    // Entry point for the overall scalar-replacement pass
    bool runOnMachineFunction(MachineFunction &MF);


    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<MachineModuleInfoWrapperPass>();
      AU.addPreserved<MachineModuleInfoWrapperPass>();
    }

  private:
    bool isVulnerableJmp(MachineInstr &MI);
    bool isVulnerableBswap(MachineInstr &MI);
    bool changeVulnerableBswap(MachineInstr &MI);
    bool isVulnerableMovnti(MachineInstr &MI);
    bool changeVulnerableMovnti(MachineInstr &MI);
    // Add fields and helper functions for this pass here.
  };
}

//------------------------------------------------------
namespace llvm{
  FunctionPass *createUnalignedGadgetRemovalPass() { 
    return new UnalignedGadgetRemoval(); 
  }
}


char UnalignedGadgetRemoval::ID = 0;


bool UnalignedGadgetRemoval::runOnMachineFunction(MachineFunction &MF) {
  bool changed = false;

  MachineModuleInfo &MMI =
      getAnalysis<MachineModuleInfoWrapperPass>().getMMI();
  for (auto &MBB : MF) {
    for (MachineInstr &MI : MBB.instrs()) {
      if (isVulnerableJmp(MI)) {
        errs() << "Found vulnerable jmp op\n" ;
      }
      else if (isVulnerableBswap(MI)) {
        changeVulnerableBswap(MI);
        changed = true;
        errs() << "Found vulnerable bswap op\n" ;
      }
      else if (isVulnerableMovnti(MI)) {
        changeVulnerableMovnti(MI);
        changed = true;
        errs() << "Found vulnerable movnti op\n" ;
      }
    }
  }
  return changed;
}

bool UnalignedGadgetRemoval::isVulnerableJmp(MachineInstr &MI) {
  if (MI.getOpcode() == X86::JMP_1 || MI.getOpcode() == X86::JCC_1) {
    int length;
    if (MI.getOpcode() == X86::JMP_1)
      length = 5;
    else if (MI.getOpcode() == X86::JCC_1)
      length = 6;

    if (MI.getOperand(0).isMBB()) {
      //TODO
      return false;
    }
    else if (MI.getOperand(0).isBlockAddress() || MI.getOperand(0).isGlobal() || MI.getOperand(0).isMCSymbol() || MI.getOperand(0).isSymbol() || MI.getOperand(0).isTargetIndex()) {
      int64_t offset = MI.getOperand(0).getOffset();
      if ((offset - length) & 0xff == 0xc3)
        return true;
      else
        return false;
    }
  }
  return false;
}

bool UnalignedGadgetRemoval::isVulnerableBswap(MachineInstr &MI) {
  if (MI.getOpcode() == X86::BSWAP32r || MI.getOpcode() == X86::BSWAP64r) {
    if (MI.getOperand(0).getReg() == X86::RDX || MI.getOperand(0).getReg() == X86::EDX 
      || MI.getOperand(0).getReg() == X86::RBX || MI.getOperand(0).getReg() == X86::EBX
      || MI.getOperand(0).getReg() == X86::R10 || MI.getOperand(0).getReg() == X86::R11)
      return true;
    else 
      return false;
  }
  else
    return false;
}


bool UnalignedGadgetRemoval::changeVulnerableBswap(MachineInstr &MI) {
  MachineBasicBlock *MBB =  MI.getParent();
  MachineFunction *MF = MBB->getParent();
  const X86Subtarget &STI = MF->getSubtarget<X86Subtarget>();
  const X86InstrInfo &TII = *STI.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  MachineInstrBuilder MIB; 

  unsigned int oldReg = MI.getOperand(0).getReg();
  bool is32 = MI.getOpcode() == X86::BSWAP32r;
  unsigned int newReg = is32 ? X86::ECX : X86::RCX;
  unsigned int moveOp = is32 ? X86::MOV32rr : X86::MOV64rr;

  MIB = BuildMI(*MBB, &MI, DL, TII.get(X86::PUSH64r)).addReg(X86::RCX, RegState::Kill);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(X86::MOV32rr)).addReg(newReg).addReg(oldReg);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(MI.getOpcode())).addReg(newReg, RegState::Define).addReg(newReg, RegState::Kill);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(X86::MOV32rr)).addReg(oldReg).addReg(newReg);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(X86::POP64r)).addReg(X86::RCX, RegState::Define);

  MI.removeFromParent();
}

bool UnalignedGadgetRemoval::isVulnerableMovnti(MachineInstr &MI) {
  if (MI.getOpcode() == X86::MOVNTImr || MI.getOpcode() == X86::MOVNTI_64mr)
    return true;
  else
    return false;
}

bool UnalignedGadgetRemoval::changeVulnerableMovnti(MachineInstr &MI) {
  MachineBasicBlock *MBB =  MI.getParent();
  MachineFunction *MF = MBB->getParent();
  const X86Subtarget &STI = MF->getSubtarget<X86Subtarget>();
  const X86InstrInfo &TII = *STI.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  MachineInstrBuilder MIB; 

  bool is32 = MI.getOpcode() == X86::MOVNTImr;
  unsigned int moveOp = is32 ? X86::MOV32mr : X86::MOV64mr;

  MIB = BuildMI(*MBB, &MI, DL, TII.get(moveOp));
  MachineOperand *MO = new MachineOperand(MI.getOperand(0));
  MIB.addOperand(*MO);
  *MO = new MachineOperand(MI.getOperand(1));
  MIB.addOperand(*MO);

  MI.removeFromParent();
}

#endif

