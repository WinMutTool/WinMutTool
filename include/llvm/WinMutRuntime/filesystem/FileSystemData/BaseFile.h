#ifndef LLVM_BASEFILE_H
#define LLVM_BASEFILE_H

#include "../ErrorCode.h"
#include <cstdio>
#include <system_error>

namespace accmut {
class BaseFile {
public:
  virtual ~BaseFile() = default;

  virtual void dump(FILE *f) = 0;

  virtual bool isCached() = 0;

  inline virtual ssize_t write(const void *buf, size_t nbyte, size_t off,
                               std::error_code &err) {
    err = make_system_error_code(std::errc::operation_not_permitted);
    return -1;
  }

  inline virtual ssize_t read(void *buf, size_t nbyte, size_t off,
                              std::error_code &err) {
    err = make_system_error_code(std::errc::operation_not_permitted);
    return -1;
  }
};
} // namespace accmut

#endif // LLVM_BASEFILE_H
