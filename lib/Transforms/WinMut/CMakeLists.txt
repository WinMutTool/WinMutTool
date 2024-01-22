set(WinMutSourceFiles
        WAInstrumenter.cpp
        MSInstrumenter.cpp
        MutationGen.cpp
        MutUtil.cpp
        StatisticsUtils.cpp
        PrintPass.cpp
        InstrumentMainPass.cpp
        AddMainArgcArgvPath.cpp
        NaiveSplitBlockPass.cpp
        #RenamePass.cpp
        UglyRenamePass.cpp)
set(LLVM_LINK_COMPONENTS Analysis Core IPO Support TransformUtils)

#add_llvm_loadable_module(WinMut
#        ${WinMutSourceFiles}
#
#        ADDITIONAL_HEADER_DIRS
#        ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/WinMut
#
#        DEPENDS
#        intrinsics_gen
#        PLUGIN_TOOL
#        opt
#        )

add_llvm_library(LLVMWinMut
        ${WinMutSourceFiles}

        ADDITIONAL_HEADER_DIRS
        ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/WinMut

        DEPENDS
        intrinsics_gen
        )
