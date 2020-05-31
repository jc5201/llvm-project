#include "gtest/gtest.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/PP2/UnalignedGadget.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/CodeGen/MIRParser/MIRParser.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
  
#include <cstdio>

using namespace llvm;

namespace {

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


TEST(TEST_TEST, TEST_TEST) {
  printf("debug1\n");
  const char * code = "--- |\n\
  ; ModuleID = 'asd.ll'\n\
  source_filename = \"a.c\"\n\
  target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n\
  target triple = \"x86_64-unknown-linux-gnu\"\n\
  \n\
  ; Function Attrs: noinline nounwind optnone uwtable\n\
  define dso_local i32 @main() #0 {\n\
    %1 = alloca i32, align 4\n\
    %2 = alloca i32, align 4\n\
    %3 = alloca i32, align 4\n\
    store i32 0, i32* %1, align 4\n\
    store i32 3, i32* %2, align 4\n\
    store i32 5, i32* %3, align 4\n\
    %4 = load i32, i32* %2, align 4\n\
    %5 = icmp sgt i32 %4, 5\n\
    br i1 %5, label %6, label %7\n\
  \n\
  6:                                                ; preds = %0\n\
    store i32 4, i32* %2, align 4\n\
    br label %9\n\
  \n\
  7:                                                ; preds = %0\n\
    %8 = load i32, i32* %3, align 4\n\
    store i32 %8, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    store i32 5, i32* %2, align 4\n\
    br label %9\n\
  \n\
  9:                                                ; preds = %7, %6\n\
    %10 = load i32, i32* %2, align 4\n\
    %11 = load i32, i32* %3, align 4\n\
    %12 = add nsw i32 %10, %11\n\
    ret i32 %12\n\
  }\n\
  \n\
  attributes #0 = { noinline nounwind optnone uwtable \"correctly-rounded-divide-sqrt-fp-math\"=\"false\" \"disable-tail-calls\"=\"false\" \"frame-pointer\"=\"all\" \"less-precise-fpmad\"=\"false\" \"min-legal-vector-width\"=\"0\" \"no-infs-fp-math\"=\"false\" \"no-jump-tables\"=\"false\" \"no-nans-fp-math\"=\"false\" \"no-signed-zeros-fp-math\"=\"false\" \"no-trapping-math\"=\"false\" \"stack-protector-buffer-size\"=\"8\" \"target-cpu\"=\"x86-64\" \"target-features\"=\"+cx8,+fxsr,+mmx,+sse,+sse2,+x87\" \"unsafe-fp-math\"=\"false\" \"use-soft-float\"=\"false\" }\n\
  \n\
  !llvm.module.flags = !{!0}\n\
  !llvm.ident = !{!1}\n\
  \n\
  !0 = !{i32 1, !\"wchar_size\", i32 4}\n\
  !1 = !{!\"clang version 10.0.0 (https://github.com/llvm/llvm-project.git d32170dbd5b0d54436537b6b75beaf44324e0c28)\"}\n\
\n\
...\n\
---\n\
name:            main\n\
alignment:       16\n\
tracksRegLiveness: true\n\
frameInfo:\n\
  stackSize:       8\n\
  offsetAdjustment: -8\n\
  maxAlignment:    4\n\
  maxCallFrameSize: 0\n\
fixedStack:\n\
  - { id: 0, type: spill-slot, offset: -16, size: 8, alignment: 16 }\n\
stack:\n\
  - { id: 0, name: '', offset: -28, size: 4, alignment: 4 }\n\
  - { id: 1, name: '', offset: -20, size: 4, alignment: 4 }\n\
  - { id: 2, name: '', offset: -24, size: 4, alignment: 4 }\n\
machineFunctionInfo: {}\n\
body:             |\n\
  bb.0 (%ir-block.0):\n\
    frame-setup PUSH64r killed $rbp, implicit-def $rsp, implicit $rsp\n\
    CFI_INSTRUCTION def_cfa_offset 16\n\
    CFI_INSTRUCTION offset $rbp, -16\n\
    $rbp = frame-setup MOV64rr $rsp\n\
    CFI_INSTRUCTION def_cfa_register $rbp\n\
    MOV32mi $rbp, 1, $noreg, -12, $noreg, 0 :: (store 4 into %ir.1)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 3 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -8, $noreg, 5 :: (store 4 into %ir.3)\n\
    CMP32mi8 $rbp, 1, $noreg, -4, $noreg, 5, implicit-def $eflags :: (load 4 from %ir.2)\n\
    JCC_1 %bb.2, 14, implicit killed $eflags\n\
  \n\
  bb.1 (%ir-block.6):\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 4 :: (store 4 into %ir.2)\n\
    JMP_1 %bb.3\n\
  \n\
  bb.2 (%ir-block.7):\n\
    renamable $eax = MOV32rm $rbp, 1, $noreg, -8, $noreg :: (load 4 from %ir.3)\n\
    MOV32mr $rbp, 1, $noreg, -4, $noreg, killed renamable $eax :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
    MOV32mi $rbp, 1, $noreg, -4, $noreg, 5 :: (store 4 into %ir.2)\n\
  \n\
  bb.3 (%ir-block.9):\n\
    renamable $eax = MOV32rm $rbp, 1, $noreg, -4, $noreg :: (load 4 from %ir.2)\n\
    renamable $eax = ADD32rm killed renamable $eax, $rbp, 1, $noreg, -8, $noreg, implicit-def dead $eflags :: (load 4 from %ir.3)\n\
    $rbp = frame-destroy POP64r implicit-def $rsp, implicit $rsp\n\
    CFI_INSTRUCTION def_cfa $rsp, 8\n\
    RETQ implicit $eax\n\
\n\
...\n\
";

  printf("debug2\n");

  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();
  printf("debug2.1\n");
  LLVMContext Context;
  std::unique_ptr<LLVMTargetMachine> TM = createX86TargetMachine();
  if (!TM) 
    ASSERT_TRUE(false);

  printf("debug3\n");
  StringRef str = StringRef(code);
  printf("debug4\n");
  std::unique_ptr<MemoryBuffer> MBuffer = MemoryBuffer::getMemBuffer(str);
  printf("debug4.1\n");
  std::unique_ptr<MIRParser> MIR = createMIRParser(std::move(MBuffer), Context);
  printf("debug4.2\n");
  std::unique_ptr<Module> M = MIR->parseIRModule();
  M->setTargetTriple(TM->getTargetTriple().getTriple());
  printf("debug4.3\n");
  DataLayout DL = TM->createDataLayout();
  printf("debug4.4\n");
  M->setDataLayout(DL);
  printf("debug5\n");
  MachineModuleInfoWrapperPass *MMIWP = new MachineModuleInfoWrapperPass(&*TM);
  printf("debug6\n");
  if (MIR->parseMachineFunctions(*M, MMIWP->getMMI()))
    ASSERT_TRUE(false);
  printf("debug7\n");
  legacy::PassManager PM;


  std::string s;

  std::error_code EC;

  SmallVector<char, 2000> *buf = new SmallVector<char, 2000>();
  raw_svector_ostream&& out = raw_svector_ostream(*buf);
  printf("debug10\n");

  // if (TM->addPassesToEmitFile(PM, *out, nullptr, CGFT_ObjectFile))
  if (TM->addPassesToEmitFile(PM, out, nullptr, CGFT_AssemblyFile))
    printf("failed addPassesToEmitFile\n");
  printf("debug11\n");
  PM.add(MMIWP);
  printf("debug8\n");
  PM.add(new UnalignedGadgetRemoval());
  printf("debug9\n");

  PM.run(*M);

  printf("debug12\n");  

  // Don't add flush or delete
  StringRef s1 = out.str();

  printf("string len : %d\n", s1.size());
  printf("%s\n", s1.str().c_str());


  ASSERT_TRUE(true);
  printf("Finish!!!!!\n");
}
}

