#ifndef LLVM_TTYFILE_H
#define LLVM_TTYFILE_H

#include "NoMemDataFile.h"

namespace accmut {

class TTYFile : public NoMemDataFile {
public:
  inline ssize_t write(const void *buf, size_t nbyte, size_t off,
                       std::error_code &err) override {
    err.clear();
    return nbyte;
  }
};
} // namespace accmut

#endif // LLVM_TTYFILE_H
