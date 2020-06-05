
set(LLVM_LINK_COMPONENTS 
  Support 
  Passes 
  Core
  MIRParser
  CodeGen
  Target
  X86AsmParser
  X86CodeGen
  X86Desc
  X86Info
  MC
)

add_llvm_unittest(EmitTests
  EmitTest.cpp
  )

