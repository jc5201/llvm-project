add_llvm_component_library( LLVMPP2
  UnalignedGadget.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/PP2

  DEPENDS
  intrinsics_gen
  )
