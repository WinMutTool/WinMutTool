#ifndef LLVM_NOMEMDATAFILE_H
#define LLVM_NOMEMDATAFILE_H

#include "BaseFile.h"

namespace accmut {
/// Just for making Clion happy, we are currently not planning to support it
/// Should not be put into the mirror file system, only
class NoMemDataFile : public BaseFile {
public:
  void dump(FILE *f) override;
  bool isCached() override;
  ssize_t write(const void *buf, size_t nbyte, size_t off,
                std::error_code &err);
  ssize_t read(void *buf, size_t nbyte, size_t off,
               std::error_code &err);
};
} // namespace accmut
#endif // LLVM_NOMEMDATAFILE_H
