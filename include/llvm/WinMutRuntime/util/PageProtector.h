#ifndef LLVM_PAGEPROTECTOR_H
#define LLVM_PAGEPROTECTOR_H

#include <cstdio>
#include <cstdlib>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/signal/ExitCodes.h>
#include <llvm/WinMutRuntime/sysdep/macros.h>
#include <sys/mman.h>

class PageProtector {
  void *base;
  size_t page_num;

public:
  PageProtector(void *base, size_t page_num) : base(base), page_num(page_num) {
    if (mprotect(base, PAGE_SIZE * page_num, PROT_READ | PROT_WRITE)) {
      __accmut_libc_perror("mprotect ERR : PROT_READ | PROT_WRITE");
      exit(ENV_ERR);
    }
  }
  ~PageProtector() {
    if (mprotect(base, PAGE_SIZE * page_num, PROT_READ)) {
      __accmut_libc_perror("mprotect ERR : PROT_READ");
      exit(ENV_ERR);
    }
  }
};

#endif // LLVM_PAGEPROTECTOR_H
