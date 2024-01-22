#ifndef LLVM_LOGFILEPREFIX_H
#define LLVM_LOGFILEPREFIX_H

#include <llvm/WinMutRuntime/filesystem/Path.h>

void setLogFilePrefix(const char *input);

const char *getLogFilePrefix();

void writeToLogFile(const char *filename, const char *contents, size_t size);
void writeToLogFile(const char *filename, const char *contents);
void writeToLogFile(const char *filename, const std::string &contents);

#endif // LLVM_LOGFILEPREFIX_H
