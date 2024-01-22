#include <fcntl.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/filesystem/Path.h>
#include <llvm/WinMutRuntime/logging/LogFilePrefix.h>
#include <string.h>
#include <string>
#include <unistd.h>

using namespace accmut;

static char logFilePrefix[MAXPATHLEN];

void setLogFilePrefix(const char *input) {
  strcpy(logFilePrefix, Path(input, true).getEndWithSlash().c_str());
}

const char *getLogFilePrefix() { return logFilePrefix; }

void writeToLogFile(const char *filename, const char *contents, size_t size) {
  char buff[MAXPATHLEN];
  strcpy(buff, logFilePrefix);
  strcat(buff, filename);
  int fd =
      __accmut_libc_open(buff, O_APPEND | O_CREAT | O_WRONLY, 0644);
  __accmut_libc_write(fd, contents, size);
  __accmut_libc_close(fd);
}

void writeToLogFile(const char *filename, const char *contents) {
  writeToLogFile(filename, contents, strlen(contents));
}

void writeToLogFile(const char *filename, const std::string &contents) {
  writeToLogFile(filename, contents.c_str(), contents.size());
}
