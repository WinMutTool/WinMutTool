#ifndef LLVM_INITIALIZEMACROS_H
#define LLVM_INITIALIZEMACROS_H

#include <llvm/WinMutRuntime/filesystem/FileSystemInternalAPI/ErrnoSetter.h>

#define FS_CAPI_SETUP(errnoSetter, errCode, libcFuncName, ...)                 \
  accmut::ErrnoSetter errnoSetter;                                             \
  if (unlikely(system_disabled()))                                             \
    return libcFuncName(__VA_ARGS__);                                          \
  if (unlikely(!system_initialized())) {                                       \
    disable_system();                                                          \
    return libcFuncName(__VA_ARGS__);                                          \
  }                                                                            \
  errnoSetter.enable();                                                        \
  std::error_code errCode;

#define FS_CAPI_FINISH(errnoSetter, retVal, errCode, libcFuncName, ...)        \
  do {                                                                         \
    if (errCode.value() != 0) {                                                \
      errnoSetter.set(err.value());                                            \
    }                                                                          \
    if (MUTATION_ID == 0) {                                                    \
      errno = 0;                                                               \
      auto __ori_ret = libcFuncName(__VA_ARGS__);                              \
      check_eq(errno, errCode.value());                                        \
      check_eq(__ori_ret, retVal);                                             \
    }                                                                          \
  } while (0)

#define FS_CAPI_FINISH_VOID(errnoSetter, errCode, libcFuncName, ...)           \
  do {                                                                         \
    if (errCode.value() != 0)                                                  \
      errnoSetter.set(err.value());                                            \
    if (MUTATION_ID == 0) {                                                    \
      errno = 0;                                                               \
      libcFuncName(__VA_ARGS__);                                               \
      check_eq(errno, errCode.vaue());                                         \
    }                                                                          \
  } while (0)

#endif // LLVM_INITIALIZEMACROS_H
