#define DEBUG_TYPE "pp2"

#include "llvm/Transforms/PP2/UnalignedGadget.h"

using namespace llvm;

char UnalignedGadgetRemoval::ID = 0;
static RegisterPass<UnalignedGadgetRemoval> X("pp2",
          "Scalar Replacement of Aggregates (PP1)",
          false /* does not modify the CFG */,
          false /* transformation, not just analysis */);


bool UnalignedGadgetRemoval::runOnMachineFunction(MachineFunction &MF) {
  for (auto &MBB : MF) {
    for (MachineInstr &MI : MBB.instrs()) {
      if (isVulnerableJmp(MI)) {
        NumJmp++;
      }
    }
  }
  return true;

}

bool UnalignedGadgetRemoval::isVulnerableJmp(MachineInstr &MI) {
  return false;
}
