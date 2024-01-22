#ifndef LLVM_OPENEDPIPEFILE_H
#define LLVM_OPENEDPIPEFILE_H

#include <llvm/WinMutRuntime/checking/panic.h>
#include <llvm/WinMutRuntime/filesystem/OpenedFileData/OpenedFile.h>

namespace accmut {

class OpenedPipeFile : public OpenedFile {
  struct stat st;
  std::vector<char> data;

public:
  inline OpenedPipeFile(int flag, struct stat st, std::vector<char> data)
      : OpenedFile(flag, 0, nullptr), st(st), data(std::move(data)) {}
  ~OpenedPipeFile() = default;
  inline ssize_t read(void *buf, size_t nbyte, std::error_code &err) override {
    err.clear();
    if (off >= data.size())
      return 0;
    size_t s = std::min(nbyte, data.size() - (size_t)off);
    memcpy(buf, data.data() + off, s);
    off += s;
    return s;
  }
  inline ssize_t write(const void *buf, size_t nbyte,
                       std::error_code &err) override {
    panic("Write to pipe is not supported");
  }
  inline struct stat getStat() { return st; }
  inline void setUid(uid_t uid) { st.st_uid = uid; }
  inline void setGid(gid_t gid) { st.st_gid = gid; }
  off_t lseek(off_t offset, int whence, std::error_code &err) override {
    err = make_system_error_code(std::errc::invalid_seek);
    return -1;
  }
};

} // namespace accmut

#endif // LLVM_OPENEDPIPEFILE_H
