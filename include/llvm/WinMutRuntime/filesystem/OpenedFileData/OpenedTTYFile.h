#ifndef LLVM_OPENEDTTYFILE_H
#define LLVM_OPENEDTTYFILE_H

#include <llvm/WinMutRuntime/checking/panic.h>
#include <llvm/WinMutRuntime/filesystem/OpenedFileData/OpenedFile.h>

namespace accmut {

class OpenedTTYFile : public OpenedFile {
  struct stat st;

public:
  inline OpenedTTYFile(int flag, struct stat st)
      : OpenedFile(flag, 0, nullptr), st(st) {}
  ~OpenedTTYFile() = default;
  inline ssize_t read(void *buf, size_t nbyte, std::error_code &err) override {
    panic("Read on TTY file");
  }
  inline ssize_t write(const void *buf, size_t nbyte,
                       std::error_code &err) override {
    return nbyte;
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

#endif // LLVM_OPENEDTTYFILE_H
