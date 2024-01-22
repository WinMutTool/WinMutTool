#ifndef LLVM_REGULARFILE_H
#define LLVM_REGULARFILE_H

#include "../../memory/memory.h"
#include "BaseFile.h"
#include <llvm/WinMutRuntime/signal/ExitCodes.h>
#include <unistd.h>
#include <vector>

namespace accmut {

extern bool _write_called;
inline bool write_called() { return _write_called; }

class RegularFile : public BaseFile {
  std::vector<char> data;
  size_t len;

public:
  inline RegularFile(std::vector<char> data, size_t len)
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

  inline size_t getSize() { return len; }
};
} // namespace accmut

#endif // LLVM_REGULARFILE_H
