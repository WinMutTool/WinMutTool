#include <llvm/WinMutRuntime/filesystem/FileSystemData/NoMemDataFile.h>
#include <llvm/WinMutRuntime/checking/panic.h>

namespace accmut {
  void NoMemDataFile::dump(FILE *f) { panic("Don't call this"); }
  bool NoMemDataFile::isCached() { panic("Don't call this"); }
  ssize_t NoMemDataFile::write(const void *buf, size_t nbyte, size_t off,
                std::error_code &err) {
    panic("Don't call this");
  }
  ssize_t NoMemDataFile::read(void *buf, size_t nbyte, size_t off,
               std::error_code &err) {
    panic("Don't call this");
  }
}
