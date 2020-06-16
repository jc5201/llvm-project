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
#include "llvm/CodeGen/VirtRegMap.h"
#include "llvm/CodeGen/RegisterClassInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"

#include "X86.h"
#include "X86Subtarget.h"
#include "X86InstrInfo.h"
#include "X86InstrBuilder.h"

#include "GFreeAssembler.h"

#include <queue>
#include <vector>
#include <cstdio>

using namespace llvm;

STATISTIC(NumJmp,  "Number of vulnerable jmp instructions");

namespace {
  struct UnalignedGadgetRemoval : public MachineFunctionPass {
    static char ID; 
    UnalignedGadgetRemoval() : MachineFunctionPass(ID) { };

    bool runOnMachineFunction(MachineFunction &MF);


    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesCFG();
      AU.addRequired<VirtRegMap>();
      AU.addRequired<MachineModuleInfoWrapperPass>();
      AU.addPreserved<MachineModuleInfoWrapperPass>();
      MachineFunctionPass::getAnalysisUsage(AU);
    }

  private:
    // TODO: checking vulnerable jmp should be handled in a separated pass
    //   because it should be checked at the last part.
    bool isVulnerableJmp(MachineInstr &MI, const std::vector<unsigned char> MIBytes);
    void changeVulnerableJmp(MachineInstr &MI);
    bool isVulnerableBswap(MachineInstr &MI);
    void changeVulnerableBswap(MachineInstr &MI);
    bool isVulnerableMovnti(MachineInstr &MI);
    void changeVulnerableMovnti(MachineInstr &MI);
    bool isVulnerableModrm(MachineInstr &MI);
    void changeVulnerableModrm(MachineInstr &MI);
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

  VirtRegMap * VRM = &getAnalysis<VirtRegMap>();
  GFreeAssembler *Assembler = new GFreeAssembler(MF, VRM);

  for (auto &MBB : MF) {
    bool repeatLoop = true;
    while (repeatLoop) {
      repeatLoop = false;
      for (MachineInstr &MI : MBB.instrs()) {
        std::vector<unsigned char> MIBytes = Assembler->MachineInstrToBytes(&MI);
        for (auto ch : MIBytes) {errs() << '|' << (unsigned int)ch;} errs() << "\n";
        if (isVulnerableJmp(MI, MIBytes)) {
          errs() << "Found vulnerable jmp op\n" ;
          changeVulnerableJmp(MI);
          changed = true;
          repeatLoop = true;
          break;
        }
        else if (isVulnerableBswap(MI)) {
          errs() << "Found vulnerable bswap op\n" ;
          changeVulnerableBswap(MI);
          changed = true;
          repeatLoop = true;
          break;
        }
        else if (isVulnerableMovnti(MI)) {
          errs() << "Found vulnerable movnti op\n" ;
          changeVulnerableMovnti(MI);
          changed = true;
          repeatLoop = true;
          break;
        }
        if (isVulnerableModrm(MI)) {
          errs() << "Found vulnerable modrm\n" ;
          changeVulnerableModrm(MI);
          changed = true;
          repeatLoop = true;
          break;
        }
      }
    }
  }
  return changed;
}

bool UnalignedGadgetRemoval::isVulnerableJmp(MachineInstr &MI, const std::vector<unsigned char> MIBytes) {
  if (MI.getOpcode() == X86::JMP_1 || MI.getOpcode() == X86::JCC_1) {
    for (char ch : MIBytes) {
      if (ch == 0xc2 || ch == 0xc3 || ch == 0xca || ch == 0xcb) //TODO: if 0xc2 is not last byte of offset?
        return true;
      else continue;
    }
    return false;
  }
  return false;
}

void UnalignedGadgetRemoval::changeVulnerableJmp(MachineInstr &MI) {
  MachineBasicBlock *MBB =  MI.getParent();
  MachineFunction *MF = MBB->getParent();
  const X86Subtarget &STI = MF->getSubtarget<X86Subtarget>();
  const X86InstrInfo &TII = *STI.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  MachineInstrBuilder MIB; 

  MachineInstr* NMI;
  for(auto MIIter = MBB->instr_begin(); ; MIIter++) {
    if (&(*MIIter) != &MI)
      continue;
    MIIter++;
    NMI = &(*MIIter);
  }

  MIB = BuildMI(*MBB, NMI, DL, TII.get(X86::NOOP));
  MIB = BuildMI(*MBB, NMI, DL, TII.get(X86::NOOP));

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


void UnalignedGadgetRemoval::changeVulnerableBswap(MachineInstr &MI) {
  MachineBasicBlock *MBB =  MI.getParent();
  MachineFunction *MF = MBB->getParent();
  const X86Subtarget &STI = MF->getSubtarget<X86Subtarget>();
  const X86InstrInfo &TII = *STI.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  MachineInstrBuilder MIB; 

  unsigned int oldReg = MI.getOperand(0).getReg();
  bool is32 = MI.getOpcode() == X86::BSWAP32r;
  unsigned int newReg = is32 ? X86::EDI : X86::RDI;
  unsigned int newReg64 = X86::RDI;
  unsigned int moveOp = is32 ? X86::MOV32rr : X86::MOV64rr;

  MIB = BuildMI(*MBB, &MI, DL, TII.get(X86::PUSH64r)).addReg(newReg64, RegState::Kill);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(moveOp)).addReg(newReg).addReg(oldReg);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(MI.getOpcode())).addReg(newReg, RegState::Define).addReg(newReg, RegState::Kill);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(moveOp)).addReg(oldReg).addReg(newReg);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(X86::POP64r)).addReg(newReg64, RegState::Define);

  MI.eraseFromParent();
}

bool UnalignedGadgetRemoval::isVulnerableMovnti(MachineInstr &MI) {
  if (MI.getOpcode() == X86::MOVNTImr || MI.getOpcode() == X86::MOVNTI_64mr)
    return true;
  else
    return false;
}

void UnalignedGadgetRemoval::changeVulnerableMovnti(MachineInstr &MI) {
  MachineBasicBlock *MBB =  MI.getParent();
  MachineFunction *MF = MBB->getParent();
  const X86Subtarget &STI = MF->getSubtarget<X86Subtarget>();
  const X86InstrInfo &TII = *STI.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  MachineInstrBuilder MIB; 

  bool is32 = MI.getOpcode() == X86::MOVNTImr;
  unsigned int moveOp = is32 ? X86::MOV32mr : X86::MOV64mr;

  MIB = BuildMI(*MBB, &MI, DL, TII.get(moveOp));
  for (int i=0; i < MI.getNumOperands(); i++) {  
    MachineOperand *MO = new MachineOperand(MI.getOperand(i));
    MIB.add(*MO);
  }

  MI.eraseFromParent();
}

bool UnalignedGadgetRemoval::isVulnerableModrm(MachineInstr &MI) {
  if (MI.getNumOperands() == 2) {
    if (MI.getOperand(0).isReg() && MI.getOperand(1).isReg()){
      if (MI.getOperand(0).getReg() == X86::RAX || MI.getOperand(0).getReg() == X86::EAX
        || MI.getOperand(0).getReg() == X86::AX || MI.getOperand(0).getReg() == X86::AL
        || MI.getOperand(0).getReg() == X86::RCX || MI.getOperand(0).getReg() == X86::ECX
        || MI.getOperand(0).getReg() == X86::CX || MI.getOperand(0).getReg() == X86::CL) {
        if (MI.getOperand(1).getReg() == X86::RDX || MI.getOperand(1).getReg() == X86::EDX
          || MI.getOperand(1).getReg() == X86::DX || MI.getOperand(1).getReg() == X86::DL
          || MI.getOperand(1).getReg() == X86::RBX || MI.getOperand(1).getReg() == X86::EBX
          || MI.getOperand(1).getReg() == X86::BX || MI.getOperand(1).getReg() == X86::BL) {
          return true;
        }
        else 
          return false;
      }
      else 
        return false;
    }
    else
      return false;
  }
  else
    return false;
}

void UnalignedGadgetRemoval::changeVulnerableModrm(MachineInstr &MI) {
  MachineBasicBlock *MBB =  MI.getParent();
  MachineFunction *MF = MBB->getParent();
  const X86Subtarget &STI = MF->getSubtarget<X86Subtarget>();
  const X86InstrInfo &TII = *STI.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  MachineInstrBuilder MIB;

  unsigned int srcReg = MI.getOperand(1).getReg();
  unsigned int srcReg64 = (srcReg == X86::RDX || srcReg == X86::EDX || srcReg == X86::DX || srcReg == X86::DL) ? X86::RDX : X86::RBX;
  unsigned int destReg = MI.getOperand(0).getReg();
  unsigned int newReg64 = X86::RDI;
  unsigned int newReg;
  if (srcReg == X86::RDX || srcReg == X86::RBX) newReg = X86::RDI;
  else if (srcReg == X86::EDX || srcReg == X86::EBX) newReg = X86::EDI;
  else newReg = X86::DI;

  MIB = BuildMI(*MBB, &MI, DL, TII.get(X86::PUSH64r)).addReg(newReg64, RegState::Kill);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(X86::MOV64rr)).addReg(newReg64).addReg(srcReg64);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(MI.getOpcode())).addReg(destReg, RegState::Define).addReg(newReg, RegState::Kill);
  MIB = BuildMI(*MBB, &MI, DL, TII.get(X86::POP64r)).addReg(newReg64, RegState::Define);

  MI.eraseFromParent();
}

#endif

