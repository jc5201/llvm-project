#include "Assembler.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "X86.h"

#include <vector>

using namespace llvm;

PP2Assembler::PP2Assembler() {}

std::unique_ptr<LLVMTargetMachine> createX86TargetMachine() {
  auto TT(Triple::normalize("x86_64--"));
  std::string Error;
  const Target *T = TargetRegistry::lookupTarget(TT, Error);
  if (!T)
    printf("debug: nullptr target %s \n", Error.c_str());
  return std::unique_ptr<LLVMTargetMachine>(static_cast<LLVMTargetMachine*>(
      T->createTargetMachine(TT, "", "", TargetOptions(), None, None,
                                     CodeGenOpt::Default)));
}

std::vector<unsigned char> PP2Assembler::getMC(MachineFunction &MF) {
  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86AsmParser();
  LLVMInitializeX86AsmPrinter();
  LLVMContext Context;
  std::unique_ptr<LLVMTargetMachine> TM = createX86TargetMachine();
  if (!TM)
    return std::vector<unsigned char>();

  std::unique_ptr<MCStreamer> AsmStreamer;
  TargetOptions Options = TargetOptions();
  const MCSubtargetInfo &STI = *(TM->getMCSubtargetInfo());
  const MCRegisterInfo &MRI = *(TM->getMCRegisterInfo());
  const MCInstrInfo &MII = *(TM->getMCInstrInfo());

  MachineModuleInfoWrapperPass *MMIWP = new MachineModuleInfoWrapperPass(&*TM);
  MCContext& MCcxt = MMIWP->getMMI().getContext();

  MCCodeEmitter *MCE = TM->getTarget().createMCCodeEmitter(MII, MRI, MCcxt);
  MCAsmBackend *MAB = TM->getTarget().createMCAsmBackend(STI, MRI, Options.MCOptions);
  if (!MCE || !MAB)
    return std::vector<unsigned char>();

  SmallVector<char, 10000 > *buf = new SmallVector<char, 10000 >();
  raw_svector_ostream&& out = raw_svector_ostream(*buf);

  Triple T(TM->getTargetTriple().str());
  AsmStreamer.reset(TM->getTarget().createMCObjectStreamer(
      T, MCcxt, std::unique_ptr<MCAsmBackend>(MAB),
      MAB->createObjectWriter(out), std::unique_ptr<MCCodeEmitter>(MCE),
      STI, Options.MCOptions.MCRelaxAll,
      Options.MCOptions.MCIncrementalLinkerCompatible,
      /*DWARFMustBeAtTheEnd*/ true));

  AsmPrinter* AsmPrinter = TM->getTarget().createAsmPrinter(*TM, std::move(AsmStreamer));
  AsmPrinter->runOnMachineFunction(MF);

  std::vector<unsigned char> ret;
  for (auto itr = buf->begin(), end = buf->end(); itr != end; ++itr) {
    ret.push_back(*(itr));
  }
  return ret;
}
