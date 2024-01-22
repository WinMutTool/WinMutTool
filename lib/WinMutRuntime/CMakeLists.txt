set(LLVM_LINK_COMPONENTS) # Support)

#if (CMAKE_BUILD_TYPE MATCHES "Debug"
#    OR CMAKE_BUILD_TYPE MATCHES "None")
#    if (NOT CMAKE_VERSION VERSION_LESS 3.13.4)
#        add_compile_options(-ftest-coverage -fprofile-arcs)
#        add_link_options(-ftest-coverage -fprofile-arcs)
#    endif()
#endif()
file(GLOB_RECURSE _source_files "./[^_]*.c" "./[^_]*.cpp" "./[^_]*.S")
message("Found ${_source_files}")

add_link_options(-export-dynamic -Wl,--no-as-needed -ldl)
add_llvm_library(LLVMWinMutRuntime SHARED
        ${_source_files}
        MutOutput.cpp
        logging/LogForMutTool.cpp
        ADDITIONAL_HEADER_DIRS
        ${LLVM_MAIN_INCLUDE_DIR}/llvm/WinMutRuntime
        )

