//===- Assembler.h - X86 Assembler ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
/// \file This file contains the X86 assembler.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_X86_ASSEMBLER_H
#define LLVM_LIB_TARGET_X86_ASSEMBLER_H

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/TargetRegistry.h"
#include <vector>

namespace llvm {

class PP2Assembler {
public:
  PP2Assembler();
  ~PP2Assembler() = default;
  std::vector<unsigned char> getMC(MachineFunction &MF);
};

} // end namespace llvm

#endif
