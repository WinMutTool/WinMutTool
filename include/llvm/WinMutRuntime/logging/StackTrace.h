#ifndef LLVM_STACKTRACE_H
#define LLVM_STACKTRACE_H

#include <algorithm>
#include <execinfo.h>
#include <fcntl.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <string.h>
#include <unistd.h>

inline char *getStackTrace(int num) {
  static char buf[102400];
  size_t size = 102400;
  int i;
  void *buffer[1024];
  int n = std::min(num, backtrace(buffer, 1024));
  char **symbols = backtrace_symbols(buffer, n);
  buf[0] = 0;
  char *bufp = buf;
  for (i = 0; i < n; i++) {
    bufp = strncat(buf, symbols[i], size);
    size -= (bufp - buf);
    bufp = strncat(buf, symbols[i], size);
    size -= (bufp - buf);
    if (size <= 10)
      break;
  }
  return buf;
}

#endif // LLVM_STACKTRACE_H
