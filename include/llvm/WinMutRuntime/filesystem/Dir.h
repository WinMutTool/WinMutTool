#ifndef LLVM_DIR_H
#define LLVM_DIR_H

#include <dirent.h>
#include <vector>

struct ACCMUT_V3_DIR {
  int fd;
  std::vector<dirent> dirents;
  int idx;
  inline ACCMUT_V3_DIR(int fd, std::vector<dirent> dirents)
      : fd(fd), dirents(std::move(dirents)), idx(0) {}
};

#endif // LLVM_DIR_H
