#ifndef LLVM_LOG_FOR_MUT_TOOL_H
#define LLVM_LOG_FOR_MUT_TOOL_H

#include <llvm/WinMutRuntime/filesystem/Path.h>

int directory_exists(const char *path);
void mkMutToolLogDir(int cur_case);

void setMutToolLogFilePrefix(const char *input);

const char *getMutToolLogFilePrefix();

void writeToMutToolLogFile(const char *filename, const char *contents, size_t size);
void writeToMutToolLogFile(const char *filename, const char *contents);
void writeToMutToolLogFile(const char *filename, const std::string &contents);

#endif // LLVM_LOG_FOR_MUT_TOOL_H
