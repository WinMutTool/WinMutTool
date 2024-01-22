#ifndef LLVM_BASEOPENFILE_H
#define LLVM_BASEOPENFILE_H

#include "../FileSystemData/INode.h"
#include "../MirrorFileSystem.h"
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

namespace accmut {
class OpenedFile {
  friend class OpenFileTable;
  int flag;
  ino_t ino;
  std::shared_ptr<BacktrackPos> btptr;

protected:
  off_t off = 0;

public:
  inline OpenedFile(int flag, ino_t ino, std::shared_ptr<BacktrackPos> btptr)
      : flag(flag), ino(std::move(ino)), btptr(std::move(btptr)) {}
  virtual ~OpenedFile() = default;
  virtual ssize_t read(void *buf, size_t nbyte, std::error_code &err);
  virtual ssize_t write(const void *buf, size_t nbyte, std::error_code &err);
  virtual inline struct stat getStat() {
    return MirrorFileSystem::getInstance()->getStat(ino);
  }
  virtual inline void setUid(uid_t uid) {
    MirrorFileSystem::getInstance()->setUid(ino, uid);
  }
  virtual inline void setGid(gid_t gid) {
    MirrorFileSystem::getInstance()->setGid(ino, gid);
  }
  virtual off_t lseek(off_t offset, int whence, std::error_code &err);
  virtual inline void setFlag(int newflag) {
    flag = (flag & O_ACCMODE) | (newflag & (~O_ACCMODE));
  }
  virtual inline int getFlag() { return flag; }
  virtual inline std::shared_ptr<BacktrackPos> getBTPos() { return btptr; }
};

} // namespace accmut

#endif // LLVM_BASEOPENFILE_H
