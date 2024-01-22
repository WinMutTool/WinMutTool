#ifndef LLVM_LOG_H
#define LLVM_LOG_H

#include <cstdio>
#include <llvm/WinMutRuntime/mutations/MutationIDDecl.h>
#include <unistd.h>

static char _errmsg_buf[20001];

#define LOG(...)                                                               \
  do {                                                                         \
    if (MUTATION_ID == 0) {                                                    \
      int num = snprintf(_errmsg_buf, 20000, "%s->%s():%d\tMID: %d\t",         \
                         __FILE__, __FUNCTION__, __LINE__, MUTATION_ID);       \
      int num1 = snprintf(_errmsg_buf + num, 20000 - num, __VA_ARGS__);        \
      if (write(STDERR_FILENO, _errmsg_buf, num + num1) < 0) {                 \
      }                                                                        \
    }                                                                          \
  } while (0)

#define ALWAYS_LOG(...)                                                        \
  do {                                                                         \
    int num = snprintf(_errmsg_buf, 20000, "%s->%s():%d\tMID: %d\t", __FILE__, \
                       __FUNCTION__, __LINE__, MUTATION_ID);                   \
    int num1 = snprintf(_errmsg_buf + num, 20000 - num, __VA_ARGS__);          \
    if (write(STDERR_FILENO, _errmsg_buf, num + num1) < 0) {                   \
    }                                                                          \
  } while (0)

#endif // LLVM_LOG_H
