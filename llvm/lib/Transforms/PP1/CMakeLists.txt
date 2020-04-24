add_llvm_library( PP1 MODULE BUILDTREE_ONLY
  ScalarReplacement.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
