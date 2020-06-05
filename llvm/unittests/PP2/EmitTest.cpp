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

#define INITIALIZE(SIZE) LLVMInitializeX86TargetInfo(); \
  LLVMInitializeX86Target(); \
  LLVMInitializeX86TargetMC(); \
  LLVMInitializeX86AsmParser(); \
  LLVMInitializeX86AsmPrinter(); \
  LLVMContext Context; \
  std::unique_ptr<LLVMTargetMachine> TM = createX86TargetMachine(); \
  if (!TM) {ASSERT_TRUE(false);} \
  StringRef str = StringRef(code); \
  std::unique_ptr<MemoryBuffer> MBuffer = MemoryBuffer::getMemBuffer(str); \
  std::unique_ptr<MIRParser> MIR = createMIRParser(std::move(MBuffer), Context); \
  std::unique_ptr<Module> M = MIR->parseIRModule(); \
  M->setTargetTriple(TM->getTargetTriple().getTriple()); \
  DataLayout DL = TM->createDataLayout(); \
  M->setDataLayout(DL); \
  MachineModuleInfoWrapperPass *MMIWP = new MachineModuleInfoWrapperPass(&*TM); \
  if (MIR->parseMachineFunctions(*M, MMIWP->getMMI())) { ASSERT_TRUE(false);} \
  legacy::PassManager PM; \
  SmallVector<char, SIZE > *buf = new SmallVector<char, SIZE >(); \
  raw_svector_ostream&& out = raw_svector_ostream(*buf); \
  if (TM->addPassesToEmitFile(PM, out, nullptr, CGFT_ObjectFile)) {printf("failed addPassesToEmitFile\n"); } \
  PM.add(MMIWP); \
  PM.add(new UnalignedGadgetRemoval()); \
  PM.run(*M);

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

TEST(EMIT_TEST, MOVNTI) {
  const char * code = "--- |\n\
  ; ModuleID = 'b.ll'\n\
  source_filename = \"a.c\"\n\
  target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n\
  target triple = \"x86_64-unknown-linux-gnu\"\n\
  \n\
  ; Function Attrs: nounwind\n\
  define i32 @f(<4 x float> %A, i8* %B, <2 x double> %C, i32 %D, <2 x i64> %E, <4 x i32> %F, <8 x i16> %G, <16 x i8> %H, i64 %I, i32* %loadptr) #0 {\n\
    %v0 = load i32, i32* %loadptr, align 1\n\
    %cast = bitcast i8* %B to <4 x float>*\n\
    %A2 = fadd <4 x float> %A, <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00>\n\
    store <4 x float> %A2, <4 x float>* %cast, align 16, !nontemporal !0\n\
    %v1 = load i32, i32* %loadptr, align 1\n\
    %cast1 = bitcast i8* %B to <2 x i64>*\n\
    %E2 = add <2 x i64> %E, <i64 1, i64 2>\n\
    store <2 x i64> %E2, <2 x i64>* %cast1, align 16, !nontemporal !0\n\
    %v2 = load i32, i32* %loadptr, align 1\n\
    %cast2 = bitcast i8* %B to <2 x double>*\n\
    %C2 = fadd <2 x double> %C, <double 1.000000e+00, double 2.000000e+00>\n\
    store <2 x double> %C2, <2 x double>* %cast2, align 16, !nontemporal !0\n\
    %v3 = load i32, i32* %loadptr, align 1\n\
    %cast3 = bitcast i8* %B to <4 x i32>*\n\
    %F2 = add <4 x i32> %F, <i32 1, i32 2, i32 3, i32 4>\n\
    store <4 x i32> %F2, <4 x i32>* %cast3, align 16, !nontemporal !0\n\
    %v4 = load i32, i32* %loadptr, align 1\n\
    %cast4 = bitcast i8* %B to <8 x i16>*\n\
    %G2 = add <8 x i16> %G, <i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8>\n\
    store <8 x i16> %G2, <8 x i16>* %cast4, align 16, !nontemporal !0\n\
    %v5 = load i32, i32* %loadptr, align 1\n\
    %cast5 = bitcast i8* %B to <16 x i8>*\n\
    %H2 = add <16 x i8> %H, <i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8, i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8>\n\
    store <16 x i8> %H2, <16 x i8>* %cast5, align 16, !nontemporal !0\n\
    %v6 = load i32, i32* %loadptr, align 1\n\
    %cast6 = bitcast i8* %B to i32*\n\
    store i32 %D, i32* %cast6, align 1, !nontemporal !0\n\
    %v7 = load i32, i32* %loadptr, align 1\n\
    %cast7 = bitcast i8* %B to i64*\n\
    store i64 %I, i64* %cast7, align 1, !nontemporal !0\n\
    %v8 = load i32, i32* %loadptr, align 1\n\
    %sum1 = add i32 %v0, %v1\n\
    %sum2 = add i32 %sum1, %v2\n\
    %sum3 = add i32 %sum2, %v3\n\
    %sum4 = add i32 %sum3, %v4\n\
    %sum5 = add i32 %sum4, %v5\n\
    %sum6 = add i32 %sum5, %v6\n\
    %sum7 = add i32 %sum6, %v7\n\
    %sum8 = add i32 %sum7, %v8\n\
    ret i32 %sum8\n\
  }\n\
  \n\
  attributes #0 = { nounwind }\n\
  \n\
  !llvm.module.flags = !{!0}\n\
  !llvm.ident = !{!1}\n\
  \n\
  !0 = !{i32 1, !\"wchar_size\", i32 4}\n\
  !1 = !{!\"clang version 10.0.0 (https://github.com/jc5201/llvm-project.git e2bc554d840d533d76650083c4e76af7ccc02963)\"}\n\
\n\
...\n\
---\n\
name:            f\n\
alignment:       16\n\
tracksRegLiveness: true\n\
liveins:\n\
  - { reg: '$xmm0' }\n\
  - { reg: '$rdi' }\n\
  - { reg: '$xmm1' }\n\
  - { reg: '$esi' }\n\
  - { reg: '$xmm2' }\n\
  - { reg: '$xmm3' }\n\
  - { reg: '$xmm4' }\n\
  - { reg: '$xmm5' }\n\
  - { reg: '$rdx' }\n\
  - { reg: '$rcx' }\n\
frameInfo:\n\
  maxAlignment:    1\n\
  maxCallFrameSize: 0\n\
constants:\n\
  - id:              0\n\
    value:           '<4 x float> <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00>'\n\
    alignment:       16\n\
  - id:              1\n\
    value:           '<2 x i64> <i64 1, i64 2>'\n\
    alignment:       16\n\
  - id:              2\n\
    value:           '<2 x double> <double 1.000000e+00, double 2.000000e+00>'\n\
    alignment:       16\n\
  - id:              3\n\
    value:           '<4 x i32> <i32 1, i32 2, i32 3, i32 4>'\n\
    alignment:       16\n\
  - id:              4\n\
    value:           '<8 x i16> <i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8>'\n\
    alignment:       16\n\
  - id:              5\n\
    value:           '<16 x i8> <i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8, i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8>'\n\
    alignment:       16\n\
machineFunctionInfo: {}\n\
body:             |\n\
  bb.0 (%ir-block.0):\n\
    liveins: $esi, $rcx, $rdi, $rdx, $xmm0, $xmm1, $xmm2, $xmm3, $xmm4, $xmm5\n\
  \n\
    renamable $eax = MOV32rm renamable $rcx, 1, $noreg, 0, $noreg :: (load 4 from %ir.loadptr, align 1)\n\
    renamable $xmm0 = nofpexcept ADDPSrm killed renamable $xmm0, $rip, 1, $noreg, %const.0, $noreg, implicit $mxcsr :: (load 16 from constant-pool)\n\
    MOVNTPSmr renamable $rdi, 1, $noreg, 0, $noreg, killed renamable $xmm0 :: (non-temporal store 16 into %ir.cast)\n\
    renamable $xmm2 = PADDQrm killed renamable $xmm2, $rip, 1, $noreg, %const.1, $noreg :: (load 16 from constant-pool)\n\
    renamable $eax = ADD32rm killed renamable $eax, renamable $rcx, 1, $noreg, 0, $noreg, implicit-def dead $eflags :: (load 4 from %ir.loadptr, align 1)\n\
    MOVNTDQmr renamable $rdi, 1, $noreg, 0, $noreg, killed renamable $xmm2 :: (non-temporal store 16 into %ir.cast1)\n\
    renamable $xmm1 = nofpexcept ADDPDrm killed renamable $xmm1, $rip, 1, $noreg, %const.2, $noreg, implicit $mxcsr :: (load 16 from constant-pool)\n\
    renamable $eax = ADD32rm killed renamable $eax, renamable $rcx, 1, $noreg, 0, $noreg, implicit-def dead $eflags :: (load 4 from %ir.loadptr, align 1)\n\
    MOVNTPDmr renamable $rdi, 1, $noreg, 0, $noreg, killed renamable $xmm1 :: (non-temporal store 16 into %ir.cast2)\n\
    renamable $xmm3 = PADDDrm killed renamable $xmm3, $rip, 1, $noreg, %const.3, $noreg :: (load 16 from constant-pool)\n\
    renamable $eax = ADD32rm killed renamable $eax, renamable $rcx, 1, $noreg, 0, $noreg, implicit-def dead $eflags :: (load 4 from %ir.loadptr, align 1)\n\
    MOVNTDQmr renamable $rdi, 1, $noreg, 0, $noreg, killed renamable $xmm3 :: (non-temporal store 16 into %ir.cast3)\n\
    renamable $xmm4 = PADDWrm killed renamable $xmm4, $rip, 1, $noreg, %const.4, $noreg :: (load 16 from constant-pool)\n\
    renamable $eax = ADD32rm killed renamable $eax, renamable $rcx, 1, $noreg, 0, $noreg, implicit-def dead $eflags :: (load 4 from %ir.loadptr, align 1)\n\
    MOVNTDQmr renamable $rdi, 1, $noreg, 0, $noreg, killed renamable $xmm4 :: (non-temporal store 16 into %ir.cast4)\n\
    renamable $xmm5 = PADDBrm killed renamable $xmm5, $rip, 1, $noreg, %const.5, $noreg :: (load 16 from constant-pool)\n\
    renamable $eax = ADD32rm killed renamable $eax, renamable $rcx, 1, $noreg, 0, $noreg, implicit-def dead $eflags :: (load 4 from %ir.loadptr, align 1)\n\
    MOVNTDQmr renamable $rdi, 1, $noreg, 0, $noreg, killed renamable $xmm5 :: (non-temporal store 16 into %ir.cast5)\n\
    renamable $eax = ADD32rm killed renamable $eax, renamable $rcx, 1, $noreg, 0, $noreg, implicit-def dead $eflags :: (load 4 from %ir.loadptr, align 1)\n\
    MOVNTImr renamable $rdi, 1, $noreg, 0, $noreg, killed renamable $esi :: (non-temporal store 4 into %ir.cast6, align 1)\n\
    renamable $eax = ADD32rm killed renamable $eax, renamable $rcx, 1, $noreg, 0, $noreg, implicit-def dead $eflags :: (load 4 from %ir.loadptr, align 1)\n\
    MOVNTI_64mr killed renamable $rdi, 1, $noreg, 0, $noreg, killed renamable $rdx :: (non-temporal store 8 into %ir.cast7, align 1)\n\
    renamable $eax = ADD32rm killed renamable $eax, killed renamable $rcx, 1, $noreg, 0, $noreg, implicit-def dead $eflags :: (load 4 from %ir.loadptr, align 1)\n\
    RETQ $eax\n\
\n\
...\n\
";

  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86AsmParser();
  LLVMInitializeX86AsmPrinter();
  LLVMContext Context;
  std::unique_ptr<LLVMTargetMachine> TM = createX86TargetMachine();
  if (!TM) 
    ASSERT_TRUE(false);

  StringRef str = StringRef(code);
  std::unique_ptr<MemoryBuffer> MBuffer = MemoryBuffer::getMemBuffer(str);
  std::unique_ptr<MIRParser> MIR = createMIRParser(std::move(MBuffer), Context);
  std::unique_ptr<Module> M = MIR->parseIRModule();
  M->setTargetTriple(TM->getTargetTriple().getTriple());
  DataLayout DL = TM->createDataLayout();
  M->setDataLayout(DL);
  MachineModuleInfoWrapperPass *MMIWP = new MachineModuleInfoWrapperPass(&*TM);
  if (MIR->parseMachineFunctions(*M, MMIWP->getMMI()))
    ASSERT_TRUE(false);
  legacy::PassManager PM;

  SmallVector<char, 10000 > *buf = new SmallVector<char, 10000 >();
  raw_svector_ostream&& out = raw_svector_ostream(*buf);

  // if (TM->addPassesToEmitFile(PM, out, nullptr, CGFT_AssemblyFile))
  if (TM->addPassesToEmitFile(PM, out, nullptr, CGFT_ObjectFile))
    printf("failed addPassesToEmitFile\n");
  PM.add(MMIWP);
  PM.add(new UnalignedGadgetRemoval());

  PM.run(*M);

  // Don't add flush or delete
  StringRef s1 = out.str();

  printf("[Info] The size of the object is %d bytes.\n", s1.size());
  // printf("%s\n", s1.str().c_str());
  int cnt = 0;
  unsigned char ch[12800];
  for (auto itr = buf->begin(), end = buf->end(); itr != end; ++itr) {
    ch[cnt]=*(itr);
    cnt++;
  }

  for (int i = 0; i < cnt / 16; i++) {
    for (int j = 0; j < 4; j++) {
      int base = 16 * i + 4 * j;
      printf("0x%02X%02X%02X%02X ", ch[base], ch[base + 1], ch[base + 2], ch[base + 3]);
    }
    printf("\n");
  }
  for (int i = 0; i < cnt % 16 / 4; i++) {
    int base = cnt - (cnt % 16 ) + 4 * i;
    printf("0x%02X%02X%02X%02X ", ch[base], ch[base + 1], ch[base + 2], ch[base + 3]);
  }
  printf("\n");
  if (cnt % 4 != 0) {
    printf("0x");
    for (int i = 0; i < cnt % 4; i++) {
      printf("%02X", ch[cnt - 4 * (cnt % 4) + i]);
    }
  }
}

TEST(TEST_TEST, TEST_TEST) {
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

  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86Target();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86AsmParser();
  LLVMInitializeX86AsmPrinter();
  LLVMContext Context;
  std::unique_ptr<LLVMTargetMachine> TM = createX86TargetMachine();
  if (!TM) 
    ASSERT_TRUE(false);

  StringRef str = StringRef(code);
  std::unique_ptr<MemoryBuffer> MBuffer = MemoryBuffer::getMemBuffer(str);
  std::unique_ptr<MIRParser> MIR = createMIRParser(std::move(MBuffer), Context);
  std::unique_ptr<Module> M = MIR->parseIRModule();
  M->setTargetTriple(TM->getTargetTriple().getTriple());
  DataLayout DL = TM->createDataLayout();
  M->setDataLayout(DL);
  MachineModuleInfoWrapperPass *MMIWP = new MachineModuleInfoWrapperPass(&*TM);
  if (MIR->parseMachineFunctions(*M, MMIWP->getMMI()))
    ASSERT_TRUE(false);
  legacy::PassManager PM;

  SmallVector<char, 2000 > *buf = new SmallVector<char, 2000 >();
  raw_svector_ostream&& out = raw_svector_ostream(*buf);

  // if (TM->addPassesToEmitFile(PM, out, nullptr, CGFT_AssemblyFile))
  if (TM->addPassesToEmitFile(PM, out, nullptr, CGFT_ObjectFile))
    printf("failed addPassesToEmitFile\n");
  PM.add(MMIWP);
  PM.add(new UnalignedGadgetRemoval());

  PM.run(*M);

  // Don't add flush or delete
  StringRef s1 = out.str();

  printf("[Info] The size of the object is %d bytes.\n", s1.size());
  // printf("%s\n", s1.str().c_str());

  // unsigned char ch[2000];
  // int cnt = 0;
  bool prev_e9 = false;
  unsigned char ch, e9, c3;
  e9 = 233;
  c3 = 195;
  for (auto itr = buf->begin(), end = buf->end(); itr != end; ++itr) {
    // ch[cnt]=*(itr);
    // cnt++;
    ch = *(itr);
    if (ch == c3) {
      ASSERT_TRUE (!prev_e9);
    } else if (ch == e9) {
      prev_e9 = true;
    } else {
      prev_e9 = false;
    }
  }

  // Below code omit the last part of the SVector.
  /*
  for (int i = 0; i < cnt / 16; i++) {
    for (int j = 0; j < 4; j++) {
      int base = 16 * i + 4 * j;
      printf("0x%02X%02X%02X%02X ", ch[base], ch[base + 1], ch[base + 2], ch[base + 3]);
    }
    printf("\n");
  }
  for (int i = 0; i < cnt % 16 / 4; i++) {
    int base = cnt - (cnt % 16 ) + 4 * i;
    printf("0x%02X%02X%02X%02X ", ch[base], ch[base + 1], ch[base + 2], ch[base + 3]);
  }
  printf("\n");
  if (cnt % 4 != 0) {
    printf("0x");
    for (int i = 0; i < cnt % 4; i++) {
      printf("%02X", ch[cnt - 4 * (cnt % 4) + i]);
    }
  }
  */
}
}

