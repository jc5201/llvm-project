#define DEBUG_TYPE "pp2"

#include "llvm/Transforms/PP2/UnalignedGadget.h"

using namespace llvm;

char UnalignedGadgetRemoval::ID = 0;
static RegisterPass<UnalignedGadgetRemoval> X("pp2",
          "Scalar Replacement of Aggregates (PP1)",
          false /* does not modify the CFG */,
          false /* transformation, not just analysis */);


bool UnalignedGadgetRemoval::runOnMachineFunction(MachineFunction &MF) {
  assert (MF.getTarget().getTargetTriple().getArch() == Triple::ArchType::x86 || MF.getTarget().getTargetTriple().getArch() == Triple::ArchType::x86_64);
  for (ato &MBB : MF) {
    for (MachineBasicBlock::instr_iterator II : MF.instrs()) {
      MachineInstr *MI = dyn_cast<MachineInstr> (II);
      if (isVulnerableJmp(MI)) {
        NumJmp++;
      }

    }
  }
  return true;

}

bool UnalignedGadgetRemoval::isVulnerableJmp(MachineInstr *MI) {
  return false;
}
