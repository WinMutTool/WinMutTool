#ifndef LLVM_OPENFILETABLE_H
#define LLVM_OPENFILETABLE_H

#include "Dir.h"
#include "MirrorFileSystem.h"
#include "llvm/WinMutRuntime/filesystem/OpenedFileData/OpenedFile.h"
#include <list>
#include <map>
#include <memory>
#include <system_error>
#include <vector>
extern "C" {
extern void unload_stdio();
}

namespace accmut {
class OpenFileTable {
  bool is_real = true;

  std::vector<std::shared_ptr<OpenedFile>> vec;
  std::vector<int> fdflags;
  std::list<int> free_fd; // according to POSIX.1, the allocated fd should be
                          // the smallest fd available
  MirrorFileSystem *fsptr;
  static OpenFileTable *holdptr;

  int getFreeFileDes(int fdbase, std::error_code &err);

  void place(int fd, std::shared_ptr<OpenedFile> pt, int flag);

  void free(int fd);

  OpenFileTable();

  bool openPrecheck(ino_t dirino, const Path &path, int acc, int other,
                    std::error_code &err);

  void check_fcntl(int fd, int cmd, int var, int ret, std::error_code &err);

public:
  ~OpenFileTable() {
    if (isReal()) {
      unload_stdio();
    }
  }

  bool isFree(int fd);

  static OpenFileTable *getInstance();

  static void hold();

  static void release();

  int openat(int dirfd, const char *path, int oflag, int mode,
             std::error_code &err);

  void close(int fildes, std::error_code &err);

  int dup(int oldfs, std::error_code &err);

  int dup2(int oldfs, int newfs, std::error_code &err);

  int dup3(int oldfs, int newfs, int flags, std::error_code &err);

  int chdir(const char *path, std::error_code &err);

  int fchdir(int fd, std::error_code &err);

  void fchown(int fd, uid_t owner, gid_t group, std::error_code &err);

  int fcntl(int fd, int cmd, int var, std::error_code &err);

  void fstat(int fd, struct stat *buf, std::error_code &err);

  off_t lseek(int fd, off_t offset, int whence, std::error_code &err);

  ssize_t read(int fd, void *buf, size_t count, std::error_code &err);

  ssize_t write(int fd, const void *buf, size_t count, std::error_code &err);

  Path fgetpath(int fd, std::error_code &err);

  char *realpath(const char *path, char *resolved_path, std::error_code &err);

  int mkdirat(int fd, const char *path, mode_t mode, std::error_code &err);

  int fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags,
              std::error_code &err);

  int unlinkat(int dirfd, const char *pathname, int flags,
               std::error_code &err);

  int fchmod(int fd, mode_t mode, std::error_code &err);

  int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags,
               std::error_code &err);

  ino_t resolveFdToIno(int fd, std::error_code &err);

  ACCMUT_V3_DIR *fdopendir(int fd, std::error_code &err);

  bool shouldOrigin(int fd);

  bool isPipe(int fd);

  static inline bool isReal() { return MUTATION_ID == 0; }

  inline void setVirtual() { is_real = false; }
};
} // namespace accmut

#endif // LLVM_OPENFILETABLE_H
