#ifndef LLVM_ERRORCODE_H
#define LLVM_ERRORCODE_H

#include "llvm/WinMutRuntime/logging/StackTrace.h"
#include <system_error>

// #define LOG_STACK_TRACE

namespace accmut {
inline std::error_code make_system_error_code(std::errc ec) {
#ifdef LOG_STACK_TRACE
  const char *buf = getStackTrace(3);
  __accmut_libc_write(STDERR_FILENO, buf, strlen(buf));
#endif
  return {static_cast<int>(ec), std::system_category()};
}

inline std::error_code make_system_error_code(int ec) {
#ifdef LOG_STACK_TRACE
  const char *buf = getStackTrace(3);
  __accmut_libc_write(STDERR_FILENO, buf, strlen(buf));
#endif
  return {ec, std::system_category()};
}

inline std::error_code make_system_error_code() {
#ifdef LOG_STACK_TRACE
  const char *buf = getStackTrace(3);
  __accmut_libc_write(STDERR_FILENO, buf, strlen(buf));
#endif
  return {errno, std::system_category()};
}
} // namespace accmut

#endif // LLVM_ERRORCODE_H
