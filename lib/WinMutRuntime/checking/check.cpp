#include <cstdarg>
#include <llvm/WinMutRuntime/checking/check.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>

FILE *__check_fopen(const char *__restrict path, const char *__restrict mode) {
  return __accmut_libc_fopen(path, mode);
}

int __check_fclose(FILE *stream) { return fclose(stream); }

int __check_fprintf(FILE *__restrict stream, const char *__restrict format,
                    ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vfprintf(stream, format, ap);
  va_end(ap);
  return ret;
}

FILE *__check_stderr = stderr;
