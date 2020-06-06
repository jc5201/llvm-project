include_directories(
  ${CMAKE_SOURCE_DIR}/lib/Target/X86
  ${CMAKE_BINARY_DIR}/lib/Target/X86
)

set(LLVM_LINK_COMPONENTS 
  Support 
  Passes 
  Core
  MIRParser
  CodeGen
  Target
  X86CodeGen
  X86AsmParser
  X86Desc
  X86Info
  MC
)

add_llvm_unittest(EmitTests
  EmitTest.cpp
  )

