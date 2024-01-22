add_llvm_loadable_module( LLVMTestInst
        TestInst.cpp


        DEPENDS
        intrinsics_gen
        PLUGIN_TOOL
        opt)