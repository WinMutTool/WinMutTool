#ifndef LLVM_PIPEFILE_H
#define LLVM_PIPEFILE_H

#include "BaseFile.h"
#include <vector>
#include <fcntl.h>
#include <unistd.h>

namespace accmut {
// Just for making Clion happy, we are currently not planning to support it
// class PipeFile : public NoMemDataFile {};
// experimental supporting, only pipe for stdin is supported
class PipeFile : public BaseFile {
  std::vector<char> data;
  size_t len;

public:
  inline PipeFile(std::vector<char> data, size_t len)
  : data(std::move(data)), len(len) {}

  inline void dump(FILE *f) override { fprintf(f, "size: %lu\n", len); }

  inline bool isCached() override { return true; }

  inline void trunc() { len = 0; }

  ssize_t write(const void *buf, size_t nbyte, size_t off,
                       std::error_code &err) override;

  inline ssize_t read(void *buf, size_t nbyte, size_t off,
                      std::error_code &err) override {
    err.clear();
    if (off >= len)
      return 0;
    size_t s = std::min(nbyte, len - off);
    memcpy(buf, data.data() + off, s);
    return s;
  }

  inline size_t getSize() {
    return len;
  }
};
} // namespace accmut

#endif // LLVM_PIPEFILE_H
