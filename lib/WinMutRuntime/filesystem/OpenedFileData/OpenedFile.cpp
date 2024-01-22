#include "llvm/WinMutRuntime/filesystem/OpenedFileData/OpenedFile.h"
#include "llvm/WinMutRuntime/checking/panic.h"

ssize_t accmut::OpenedFile::read(void *buf, size_t nbyte,
                                 std::error_code &err) {
  auto *mfs = MirrorFileSystem::getInstance();
  ssize_t ret = mfs->read(ino, buf, nbyte, off, err);
  if (mfs->isReg(ino) || mfs->isPipe(ino)) {
    if (ret != -1)
      off += ret;
  }
  return ret;
}

ssize_t accmut::OpenedFile::write(const void *buf, size_t nbyte,
                                  std::error_code &err) {
  auto *mfs = MirrorFileSystem::getInstance();
  ssize_t ret = mfs->write(ino, buf, nbyte, off, err);
  if (mfs->isReg(ino)) {
    if (ret != -1) {
      off += ret;
    }
  }
  return ret;
}

off_t accmut::OpenedFile::lseek(off_t offset, int whence,
                                std::error_code &err) {
  auto *mfs = MirrorFileSystem::getInstance();
  if (mfs->isReg(ino)) {
    off_t newpos;
    if (whence == SEEK_CUR) {
      newpos = off + offset;
    } else if (whence == SEEK_END) {
      newpos = mfs->getStat(ino).st_size + offset;
    } else if (whence == SEEK_SET) {
      newpos = offset;
    } else {
      err = make_system_error_code(std::errc::invalid_argument);
      return -1;
    }
    if (newpos < 0) {
      err = make_system_error_code(std::errc::invalid_argument);
      return -1;
    }
    off = newpos;
    return off;
  } else {
#ifdef __APPLE__
    panic("Undefined");
#else
    err = make_system_error_code(std::errc::invalid_seek);
    return -1;
#endif
  }
}
