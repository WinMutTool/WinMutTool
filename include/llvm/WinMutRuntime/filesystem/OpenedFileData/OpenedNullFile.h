#ifndef LLVM_OPENEDNULLFILE_H
#define LLVM_OPENEDNULLFILE_H

#include "OpenedFile.h"

namespace accmut {

class OpenedNullFile : public OpenedFile {
  struct stat st;

public:
  inline OpenedNullFile(int flag, struct stat st)
      : OpenedFile(flag, 0, nullptr), st(st) {}
  ~OpenedNullFile() = default;
  inline ssize_t read(void *buf, size_t nbyte, std::error_code &err) override {
    return 0;
  }
  inline ssize_t write(const void *buf, size_t nbyte,
                       std::error_code &err) override {
    err.clear();
    return nbyte;
  }
  inline struct stat getStat() { return st; }
  inline void setUid(uid_t uid) { st.st_uid = uid; }
  inline void setGid(gid_t gid) { st.st_gid = gid; }
  off_t lseek(off_t offset, int whence, std::error_code &err) override {
    return 0;
  }
};
} // namespace accmut

#endif // LLVM_OPENEDNULLFILE_H
