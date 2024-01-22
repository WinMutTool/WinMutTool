#ifndef LLVM_PANIC_H
#define LLVM_PANIC_H

#include "check.h"
#include <cstdio>
#include <limits>
#include <llvm/WinMutRuntime/filesystem/MirrorFileSystem.h>
#include <llvm/WinMutRuntime/logging/StackTrace.h>
#include <llvm/WinMutRuntime/mutations/MutationIDDecl.h>

#define error(_str, _abort)                                                    \
  do {                                                                         \
    if (MUTATION_ID == 0) {                                                    \
      const char *trace = getStackTrace(80);                                   \
      FILE *_f = __accmut_libc_fopen("/tmp/accmut_check.log", "a");            \
      fprintf(_f, "%s:%s:%d(MUTATION_ID %d) panic: %s\n", __FILE__,            \
                      __FUNCTION__, __LINE__, MUTATION_ID, _str);              \
      fprintf(_f, "%s\n", trace);                                              \
      fprintf(_f, "%d\n", getpid());                                           \
      fclose(_f);                                                              \
    } else if (MUTATION_ID == std::numeric_limits<int>::max()) {               \
      const char *trace = getStackTrace(80);                                   \
      auto mfs = accmut::MirrorFileSystem::getInstance();                      \
      char buf[1024];                                                          \
      snprintf(buf, 1024, "%s:%s:%d panic: %s\n", __FILE__, __FUNCTION__,      \
               __LINE__, _str);                                                \
      mfs->sendPanicMsg((std::string(buf) + trace).c_str());                   \
    }                                                                          \
    if (_abort) abort();                                                       \
  } while (0)

#define panic(_str) error(_str, true)

#endif // LLVM_PANIC_H
