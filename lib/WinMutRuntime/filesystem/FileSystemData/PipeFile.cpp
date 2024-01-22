#include <llvm/WinMutRuntime/filesystem/FileSystemData/PipeFile.h>
#include <llvm/WinMutRuntime/checking/panic.h>

namespace accmut {

  ssize_t PipeFile::write(const void *buf, size_t nbyte, size_t off,
                       std::error_code &err) {
    panic("Write to pipe is not supported");
  }

} // namespace accmut
