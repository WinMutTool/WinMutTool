#include <llvm/WinMutRuntime/filesystem/FileSystemData/RegularFile.h>
#include <llvm/WinMutRuntime/checking/panic.h>

namespace accmut {
bool _write_called = false;
  ssize_t RegularFile::write(const void *buf, size_t nbyte, size_t off,
                       std::error_code &err) {
    err.clear();
    if (off + nbyte >= len) {
      if (off + nbyte >= data.size()) {
        if (memory_func_called() || write_called())
          exit(MALLOC_ERR);
        _write_called = true;
        data.resize(std::max<size_t>(data.size() * 2, off + nbyte));
        _write_called = false;
      }
      len = off + nbyte;
    } else if (off + nbyte >= data.size()) {
      panic("Should not happen");
    }
    memcpy(data.data() + off, buf, nbyte);
    return nbyte;
  }
}
