
#define DEBUG_TYPE "pp2"

#include "llvm/Transforms/PP2/UnalignedGadget.h"

using namespace llvm;

char UnalignedGadgetRemoval::ID = 0;
static RegisterPass<UnalignedGadgetRemoval> X("pp2",
			    "Scalar Replacement of Aggregates (PP1)",
			    false /* does not modify the CFG */,
			    false /* transformation, not just analysis */);


// Public interface to create the ScalarReplAggregates pass.
// This function is provided to you.
FunctionPass *createMyScalarReplAggregatesPass() { return new UnalignedGadgetRemoval(); }


//===----------------------------------------------------------------------===//
//                      SKELETON FUNCTION TO BE IMPLEMENTED
//===----------------------------------------------------------------------===//
//
// Function runOnFunction:
// Entry point for the overall ScalarReplAggregates function pass.
// This function is provided to you.
bool UnalignedGadgetRemoval::runOnMachineFunction(MachineFunction &MF) {

  return true;

}

