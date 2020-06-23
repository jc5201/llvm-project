#ifndef LLVM_LIB_TARGET_X86_GFREEASSEMBLER_H
#define LLVM_LIB_TARGET_X86_GFREEASSEMBLER_H

#include "X86.h"
#include "llvm/MC/MCStreamer.h"
#include "X86AsmPrinter.h"
#include "X86MCInstLower.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/VirtRegMap.h"
#include "X86GFreeUtils.h"
#include "llvm/IR/LegacyPassManager.h"

namespace llvm {
  class LLVM_LIBRARY_VISIBILITY GFreeAssembler{
  public:
    std::unique_ptr<MCCodeEmitter> CodeEmitter;
    MCStreamer *S;
    X86AsmPrinter *Printer;
    gfree::X86MCInstLower *MCInstLower;
    const MCSubtargetInfo *STI;
    const TargetRegisterInfo *TRI;
    const TargetInstrInfo *TII;
    MachineBasicBlock *tmpMBB;
    VirtRegMap *VRM;
    legacy::PassManager *PM;

    void temporaryRewriteRegister(MachineInstr *MI);
    std::vector<unsigned char> lowerEncodeInstr(MachineInstr *RegRewMI);
    bool expandPseudo(MachineInstr *MI);
    bool LowerSubregToReg(MachineInstr *MI);
    bool LowerCopy(MachineInstr *MI);

    GFreeAssembler(MachineFunction &MF, VirtRegMap *VRMap=nullptr);
    std::vector<unsigned char> MachineInstrToBytes(MachineInstr *MI);
    ~GFreeAssembler();
  };

}

#endif
