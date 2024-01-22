#include <llvm/WinMutRuntime/checking/panic.h>
#include <llvm/WinMutRuntime/filesystem/ErrorCode.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/DirectoryFile.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/PipeFile.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/RegularFile.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/TTYFile.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/filesystem/OpenFileTable.h>
#include <llvm/WinMutRuntime/filesystem/OpenedFileData/OpenedFile.h>
#include <llvm/WinMutRuntime/filesystem/OpenedFileData/OpenedNullFile.h>
#include <llvm/WinMutRuntime/filesystem/OpenedFileData/OpenedPipeFile.h>
#include <llvm/WinMutRuntime/filesystem/OpenedFileData/OpenedTTYFile.h>
#include <llvm/WinMutRuntime/logging/LogFilePrefix.h>
#include <llvm/WinMutRuntime/sysdep/macros.h>

namespace accmut {

static constexpr int openSupportedFlags = O_APPEND | O_CREAT | O_TRUNC |
                                          O_EXCL | O_NOFOLLOW | O_CLOEXEC |
                                          O_NONBLOCK | O_NOCTTY | O_DIRECTORY;

static constexpr int fcntlSupportedFlags = O_APPEND | O_NONBLOCK | O_DIRECTORY
#ifndef __APPLE__
                                           | 0x8000 | O_NOFOLLOW
#endif
    ;

static constexpr int fcntlCannotChangeFlags = 0
#ifndef __APPLE__
                                              | 0x8000 | O_NOFOLLOW
#endif
    ;

OpenFileTable *OpenFileTable::holdptr = nullptr;

static std::pair<std::shared_ptr<OpenedFile>, int> initFile(int fd) {
  auto mfs = MirrorFileSystem::getInstance();
  int flag = __accmut_libc_fcntl(fd, F_GETFL);
  if (flag < 0)
    panic("Failed to get fl");
  int dflag = __accmut_libc_fcntl(fd, F_GETFD);
  if (dflag < 0)
    panic("Failed to get fd");
  if ((dflag & flag) != 0)
    panic("BUG! Assume not intersected");
  int acc = flag & O_ACCMODE;
  int other = flag & (~O_ACCMODE);
  if (other & (~fcntlSupportedFlags)) {
    panic("Not supported");
  }
  // flag |= dflag;
  struct stat st;
  __accmut_libc_fstat(fd, &st);
  struct stat nullst;
  __accmut_libc_lstat("/dev/null", &nullst);
  if (st.st_dev == nullst.st_dev && st.st_ino == nullst.st_ino) {
    return std::make_pair(std::make_shared<OpenedNullFile>(flag, st), dflag);
  }
  if (isatty(fd) || S_ISCHR(st.st_mode)) {
    return std::make_pair(std::make_shared<OpenedTTYFile>(flag, st), dflag);
  } else {
    // find out where the file is
    // should be called very early
    if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) {
      // determine path
      char buff[MAXPATHLEN];
      int ckflag = 0;
      switch (acc) {
      case O_RDONLY:
        ckflag = R_OK;
        break;
      case O_WRONLY:
        ckflag = W_OK;
        break;
      case O_RDWR:
        ckflag = R_OK | W_OK;
        break;
      default:
        panic("Not supported");
        break;
      }
      std::error_code err;
      Path real;
#ifdef __APPLE__
      int ret = __accmut_libc_fcntl(fd, F_GETPATH, buff);
      if (ret < 0) {
        panic("Failed to get path");
      }
#else
      char buf1[MAXPATHLEN];
      snprintf(buf1, MAXPATHLEN, "/proc/self/fd/%d", fd);
      if (!__accmut_libc_realpath(buf1, buff)) {
        goto failed;
      }
#endif
      {
        auto ino = mfs->query(Path(buff), ckflag, false, &real, err);
        if (!err.value()) {
          auto name = real.lastName();
          auto fatherino =
              mfs->query(real.baseDir(), F_OK, false, nullptr, err);
          if (err.value())
            panic("Failed to get father");
          auto btptr = mfs->getBTPos(ino, fatherino, name);
          if (!btptr)
            panic("Failed to get father");
          auto of = std::make_shared<OpenedFile>(flag, std::move(ino), btptr);
          return std::make_pair(of, dflag);
        }
      }

#ifndef __APPLE__
    failed:
      auto ino = mfs->queryProc(Path(buf1), ckflag, err);
      if (err.value())
        panic("Failed to read proc files");
      auto of = std::make_shared<OpenedFile>(flag, ino, nullptr);
      return std::make_pair(of, dflag);
#endif
    } else if (S_ISFIFO(st.st_mode)) {
      std::vector<char> vec;
      if (fd == 0) {
        int num = 0;
        char buf[1024];
        while ((num = __accmut_libc_read(fd, buf, 1024)) != 0) {
          for (int i = 0; i < num; ++i)
            vec.push_back(buf[i]);
        }
      }
      return std::make_pair(
          std::make_shared<OpenedPipeFile>(flag, st, std::move(vec)), dflag);
    } else {
      panic("Not implemented");
    }
  }
}

ACCMUT_V3_DIR *OpenFileTable::fdopendir(int fd, std::error_code &err) {
  if (fd < 0 || isFree(fd)) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    return nullptr;
  }
  auto ino = vec[fd]->ino;
  if (!fsptr->isDir(ino)) {
    err = make_system_error_code(std::errc::not_a_directory);
    return nullptr;
  }
  return new ACCMUT_V3_DIR(fd, fsptr->getDirentList(ino));
}

bool OpenFileTable::isFree(int fd) {
  if (fd < 0)
    return true;
  if ((int)vec.size() <= fd)
    return true;
  if (std::find(free_fd.begin(), free_fd.end(), fd) != free_fd.end())
    return true;
  return false;
}

int OpenFileTable::getFreeFileDes(int fdbase, std::error_code &err) {
  static int maxval = getdtablesize();
  if (fdbase >= maxval) {
    err = make_system_error_code(std::errc::too_many_files_open);
    return -1;
  }
  auto iter = std::lower_bound(free_fd.begin(), free_fd.end(), fdbase);
  if (iter != free_fd.end()) {
    return *iter;
  } else {
    return std::max(fdbase, (int)vec.size());
  }
}

void OpenFileTable::place(int fd, std::shared_ptr<OpenedFile> pt, int flag) {
  if ((int)vec.size() <= fd) {
    // resize
    for (int i = vec.size(); i < fd; ++i) {
      free_fd.push_back(i);
    }
    vec.resize(fd + 1);
    fdflags.resize(fd + 1);
  } else {
    auto iter = std::find(free_fd.begin(), free_fd.end(), fd);
    if (iter == free_fd.end())
      panic("Not free");
    if (vec[fd])
      panic("Not free");
    free_fd.erase(iter);
  }
  vec[fd] = std::move(pt);
  fdflags[fd] = flag;
}

void OpenFileTable::free(int fd) {
  if ((int)vec.size() <= fd) {
    panic("Should not be freed");
  } else {
    auto iter = std::lower_bound(free_fd.begin(), free_fd.end(), fd);
    if (iter != free_fd.end() && *iter == fd)
      panic("Should not be freed");
    free_fd.insert(iter, fd);
    vec[fd] = nullptr;
  }
}

extern "C" {
extern void init_stdio();
}

OpenFileTable::OpenFileTable() {
  fsptr = MirrorFileSystem::getInstance();
  int dfd;
  DIR *proc;
  if ((proc = __accmut_libc_fdopendir(
           (dfd = __accmut_libc_open("/proc/self/fd", O_RDONLY)))) == nullptr)
    panic("Failed to open /proc/self/fd");
  struct dirent *d = nullptr;
  while ((d = __accmut_libc_readdir(proc)) != nullptr) {
    if (d->d_name[0] == '.' || d->d_name[0] == 0)
      continue;
    errno = 0;
    int fd = strtol(d->d_name, nullptr, 10);
    if (errno == 0) {
      if (fd == dfd)
        continue;
      auto x = initFile(fd);
      place(fd, x.first, x.second);
    }
  }
  __accmut_libc_closedir(proc);
}

OpenFileTable *OpenFileTable::getInstance() {
  if (likely(holdptr != nullptr)) {
    return holdptr;
  }
  holdptr = new OpenFileTable();
  return holdptr;
}

void OpenFileTable::hold() { holdptr = getInstance(); }

void OpenFileTable::release() { holdptr = nullptr; }

bool OpenFileTable::openPrecheck(ino_t dirino, const Path &path, int acc,
                                 int other, std::error_code &err) {
  if (other & (~openSupportedFlags))
    panic(("Not supported flags for open: " +
           std::to_string(other & (~openSupportedFlags)) +
           ", path: " + path.c_str())
              .c_str());
  err.clear();
  ino_t ino;
  ino_t fatherino =
      fsptr->query(dirino, path.baseDir(), X_OK, true, nullptr, err);
  if (err.value())
    return false;
  if (!fsptr->isDir(fatherino)) {
    err = make_system_error_code(std::errc::not_a_directory);
    return false;
  }
  path.c_str();
  if (other & O_NOFOLLOW) {
    ino = fsptr->query(dirino, path, F_OK, false, nullptr, err);
    if (!err.value()) {
      if (fsptr->isLnk(ino)) {
        err = make_system_error_code(std::errc::too_many_symbolic_link_levels);
        return false;
      }
    }
  } else {
    ino = fsptr->query(dirino, path, F_OK, true, nullptr, err);
  }
  if (!err.value()) {
    if (other & O_DIRECTORY) {
      if (!fsptr->isDir(ino)) {
        err = make_system_error_code(std::errc::not_a_directory);
        return false;
      }
    }
    // found
    if ((other & O_CREAT) && (other & O_EXCL)) {
      err = make_system_error_code(std::errc::file_exists);
      return false;
    }
    if (fsptr->isReg(ino)) {

      switch (acc) {
      case O_RDONLY:
        if (!fsptr->canRead(ino))
          goto permission;
        break;
      case O_WRONLY:
        if (!fsptr->canWrite(ino))
          goto permission;
        break;
      case O_RDWR:
        if (!fsptr->canRead(ino) || !fsptr->canWrite(ino))
          goto permission;
        break;
      default:
        err = make_system_error_code(std::errc::invalid_argument);
        return false;
      }
      if (other & O_TRUNC)
        if (!fsptr->canWrite(ino))
          goto permission;
      return false;
    } else if (fsptr->isDir(ino)) {
      if (acc == O_RDONLY) {
        if (!fsptr->canRead(ino)) {
          goto permission;
        }
        if (other & O_TRUNC)
          goto isdir;
        return false;
      }
    isdir:
      err = make_system_error_code(std::errc::is_a_directory);
      return false;
    } else {
      panic(("Not implemented " + std::string(path.c_str())).c_str());
    }
  permission:
    err = make_system_error_code(std::errc::permission_denied);
    return false;
  } else {
    err.clear();
    // not found
    if (!(other & O_CREAT)) {
      err = make_system_error_code(std::errc::no_such_file_or_directory);
      return false;
    }
    if (!fsptr->canWrite(fatherino)) {
      err = make_system_error_code(std::errc::permission_denied);
    }
    return true; // should create
  }
}

int OpenFileTable::openat(int dirfd, const char *path, int oflag, int mode,
                          std::error_code &err) {
  err.clear();
  ino_t baseino;

  if (dirfd == AT_FDCWD) {
    baseino = fsptr->getCwdIno();
  } else {
    if (isFree(dirfd)) {
      err = make_system_error_code(std::errc::bad_file_descriptor);
      if (isReal()) {
        errno = 0;
        int ret = __accmut_libc_openat(dirfd, path, oflag, mode);
        check_eq(ret, -1);
        check_eq(errno, err.value());
      }
      return -1;
    }
    baseino = vec[dirfd]->getStat().st_ino;
  }

  int acc = oflag & O_ACCMODE;
  int otherflag = oflag & (~O_ACCMODE);
  int fdflag = (oflag & O_CLOEXEC) ? FD_CLOEXEC : 0;
  bool shouldCreate;
  bool is_null = (strcmp(path, "/dev/null") == 0);
  int fd;
  ino_t ino;
  std::shared_ptr<BacktrackPos> btptr;
  if (!is_null) {
    shouldCreate =
        openPrecheck(baseino, Path(path, false), acc, otherflag, err);
    if (err.value() != 0) {
      if (isReal()) {
        errno = 0;
        __accmut_libc_openat(dirfd, path, oflag, mode);
        check_eq(errno, err.value());
      }
      return -1;
    }

    if (shouldCreate) {
      auto pathname = Path(path, false);
      if (isReal()) {
        ino = fsptr->newRegularFile(baseino, pathname, mode, err, oflag, &fd);
        if (err.value())
          panic("Should not fail");
      } else {
        ino =
            fsptr->newRegularFile(baseino, pathname, mode, err, oflag, nullptr);
        if (err.value())
          panic("Should not fail");
        fd = getFreeFileDes(0, err);
        if (err.value()) {
          return -1;
        }
      }
      auto name = pathname.lastName();
      auto fatherino =
          fsptr->query(baseino, pathname.baseDir(), F_OK, true, nullptr, err);
      if (err.value())
        panic("Should not fail");
      btptr = fsptr->getBTPos(ino, fatherino, name);
      if (!btptr)
        panic("Should not fail");
    } else {
      if (isReal()) {
        errno = 0;
        fd = __accmut_libc_openat(dirfd, path, oflag, mode);
        if (errno != 0) {
          panic("Should not fail");
        }
      } else {
        fd = getFreeFileDes(0, err);
        if (err.value()) {
          return -1;
        }
      }
      if (!isFree(fd))
        panic("Should be free");
      Path real;
      if (otherflag & O_NOFOLLOW) {
        ino = fsptr->query(baseino, Path(path, false), F_OK, false, &real, err);
      } else {
        ino = fsptr->query(baseino, Path(path, false), F_OK, true, &real, err);
      }
      if (err.value() != 0) {
        panic("Should not fail");
      }
      if (!fsptr->isReg(ino) && !fsptr->isDir(ino)) {
        panic("Should not get here");
      }
      if (otherflag & O_TRUNC) {
        fsptr->trunc(ino);
      }
      auto fatherino =
          fsptr->query(baseino, real.baseDir(), F_OK, true, nullptr, err);
      if (err.value() != 0)
        panic("Should not fail");
      auto name = real.lastName();
      btptr = fsptr->getBTPos(ino, fatherino, name);
      if (!btptr)
        panic("Should not fail");
    }
  } else {
    if (isReal()) {
      errno = 0;
      fd = __accmut_libc_openat(dirfd, path, oflag, mode);
      if (errno != 0) {
        panic("Should not fail");
      }
    } else {
      fd = getFreeFileDes(0, err);
      if (err.value() != 0)
        return -1;
    }
    struct stat st;
    __accmut_libc_lstat("/dev/null", &st);
    auto openFile = std::make_shared<OpenedNullFile>(
        acc | (otherflag & (O_APPEND | O_NONBLOCK)) | APPLE_OR_GLIBC(0, 0x8000),
        st);
    place(fd, openFile, fdflag);
    return fd;
  }
  auto openFile = std::make_shared<OpenedFile>(
      acc | (otherflag & (O_APPEND | O_NONBLOCK)) | APPLE_OR_GLIBC(0, 0x8000) |
          APPLE_OR_GLIBC(0, oflag & O_NOFOLLOW),
      ino, btptr);
  place(fd, openFile, fdflag);
  return fd;
}

void OpenFileTable::close(int fildes, std::error_code &err) {
  err.clear();
  if (isFree(fildes)) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    if (isReal()) {
      check_eq(err.value(), EBADF);
    }
    return;
  }
  free(fildes);
  if (isReal()) {
    int ret = __accmut_libc_close(fildes);
    check_eq(ret, 0);
  }
}

int OpenFileTable::dup(int oldfs, std::error_code &err) {
  return fcntl(oldfs, F_DUPFD, 0, err);
}

int OpenFileTable::dup2(int oldfs, int newfs, std::error_code &err) {
  err.clear();
  if (isFree(oldfs)) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    return -1;
  }
  if (oldfs == newfs)
    return newfs;
  if (!isFree(newfs))
    close(newfs, err);
  if (err.value())
    return -1;
  int ret = fcntl(oldfs, F_DUPFD, newfs, err);
  if (ret != -1)
    check_eq(ret, newfs);
  return ret;
}

int OpenFileTable::dup3(int oldfs, int newfs, int flags, std::error_code &err) {
  err.clear();
  if (isFree(oldfs)) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    return -1;
  }
  if (oldfs == newfs) {
    err = make_system_error_code(std::errc::invalid_argument);
    return -1;
  }
  if (flags & ~(O_CLOEXEC)) {
    err = make_system_error_code(std::errc::invalid_argument);
    return -1;
  }
  if (!isFree(newfs))
    close(newfs, err);
  if (err.value())
    return -1;
  int ret =
      fcntl(oldfs, flags == O_CLOEXEC ? F_DUPFD_CLOEXEC : F_DUPFD, newfs, err);
  if (ret != -1)
    check_eq(ret, newfs);
  return ret;
}

int OpenFileTable::chdir(const char *path, std::error_code &err) {
  err.clear();
  if (path == nullptr) {
    err = make_system_error_code(std::errc::bad_address);
    return -1;
  }
  ino_t ino = fsptr->query(Path(path, false), F_OK, true, nullptr, err);
  if (err.value())
    return -1;
  fsptr->chdir(ino, err);
  return err.value() ? -1 : 0;
}

int OpenFileTable::fchdir(int fd, std::error_code &err) {
  err.clear();
  ino_t ino = resolveFdToIno(fd, err);
  if (err.value())
    return -1;
  fsptr->chdir(ino, err);
  return err.value() ? -1 : 0;
}

void OpenFileTable::fchown(int fd, uid_t owner, gid_t group,
                           std::error_code &err) {
  err.clear();
  if ((long)vec.size() <= fd || !vec[fd]) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    if (isReal()) {
      check_eq(err.value(), EBADF);
    }
    return;
  }
  vec[fd]->setUid(owner);
  vec[fd]->setGid(group);
  if (isReal()) {
    errno = 0;
    int ret = __accmut_libc_fchown(fd, owner, group);
    check_eq(ret, 0);
  }
  return;
}

void OpenFileTable::check_fcntl(int fd, int cmd, int var, int ret,
                                std::error_code &err) {
  if (isReal()) {
    errno = 0;
    int oriret = __accmut_libc_fcntl(fd, cmd, var);
    check_eq(ret, oriret);
    check_eq(err.value(), errno);
  }
}

int OpenFileTable::fcntl(int fd, int cmd, int var, std::error_code &err) {
  err.clear();

  if (isFree(fd)) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    check_fcntl(fd, cmd, var, -1, err);
    return -1;
  }
  switch (cmd) {
  case F_DUPFD:
  case F_DUPFD_CLOEXEC: {
    static int maxval = getdtablesize();
    if (var >= maxval || var < 0) {
      err = make_system_error_code(std::errc::invalid_argument);
      check_fcntl(fd, cmd, var, -1, err);
      return -1;
    }
    int newfd = getFreeFileDes(var, err);
    if (err.value() != 0) {
      check_fcntl(fd, cmd, var, -1, err);
      return -1;
    }
    place(newfd, vec[fd], cmd == F_DUPFD ? 0 : FD_CLOEXEC);
    check_fcntl(fd, cmd, var, newfd, err);
    return newfd;
  }
  case F_GETFD: {
    int ret = fdflags[fd];
    check_fcntl(fd, cmd, var, ret, err);
    return ret;
  }
  case F_SETFD: {
    int var1 = var & FD_CLOEXEC;
    fdflags[fd] = var1;
    check_fcntl(fd, cmd, var, 0, err);
    // if (MUTATION_ID) {
    std::error_code err1;
    // failing would panic
    fcntl(fd, F_GETFD, 0, err1);
    //}
    return 0;
  }
  case F_GETFL: {
    int ret = vec[fd]->getFlag();
    check_fcntl(fd, cmd, var, ret, err);
    return ret;
  }
  case F_SETFL: {
    if (var & (O_NONBLOCK | O_ASYNC)) {
      panic("Not implemented");
    }
    int var1 = ((var & ~fcntlCannotChangeFlags) & fcntlSupportedFlags) |
               (vec[fd]->getFlag() & fcntlCannotChangeFlags);
    // int var1 = (var & fcntlSupportedFlags) | APPLE_OR_GLIBC(0, 0x8000);
    vec[fd]->setFlag(var1);
    check_fcntl(fd, cmd, var, 0, err);
    // if (MUTATION_ID) {
    std::error_code err1;
    // failing would panic
    fcntl(fd, F_GETFL, 0, err1);
    //}
    return 0;
  }
  default:
    panic("Not implemented");
  }
}
void OpenFileTable::fstat(int fd, struct stat *buf, std::error_code &err) {
  err.clear();
  if (isFree(fd)) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    return;
  }
  *buf = vec[fd]->getStat();
  if (isReal()) {
    struct stat buf1;
    errno = 0;
    int oriret = __accmut_libc_fstat(fd, &buf1);
    check_eq(errno, err.value());
    if (err.value())
      check_eq(oriret, -1);
    else
      check_eq(oriret, 0);
    check_eq(buf->st_ino, buf1.st_ino);
    check_eq(buf->st_size, buf1.st_size);
    check_eq(buf->st_mode, buf1.st_mode);
    check_eq(buf->st_uid, buf1.st_uid);
    check_eq(buf->st_gid, buf1.st_gid);
  }
}

int OpenFileTable::fstatat(int dirfd, const char *pathname,
                           struct stat *statbuf, int flags,
                           std::error_code &err) {
  bool follow_symlink = true;
  // not implement AT_NO_AUTOMOUNT
  ino_t ino;
  int ret = 0;
  if (flags & AT_SYMLINK_NOFOLLOW)
    follow_symlink = false;
  if ((flags & AT_EMPTY_PATH) && pathname[0] == 0) {
    if (dirfd == AT_FDCWD) {
      ino = fsptr->query(Path("."), F_OK, follow_symlink, nullptr, err);
    } else {
      if (isFree(dirfd)) {
        err = make_system_error_code(std::errc::bad_file_descriptor);
      } else {
        *statbuf = vec[dirfd]->getStat();
      }
    }
  } else {
    ino_t baseino;
    if (dirfd == AT_FDCWD) {
      baseino = fsptr->getCwdIno();
    } else {
      if (isFree(dirfd)) {
        err = make_system_error_code(std::errc::bad_file_descriptor);
        ret = -1;
        goto check;
      }
      baseino = vec[dirfd]->getStat().st_ino;
    }
    ino = fsptr->query(baseino, Path(pathname, false), F_OK, follow_symlink,
                       nullptr, err);
  }
  if (err.value()) {
    ret = -1;
    goto check;
  }
  *statbuf = fsptr->getStat(ino);

check:
  if (isReal()) {
    struct stat st;
    errno = 0;
    int oriret = __accmut_libc_fstatat(dirfd, pathname, &st, flags);
    if (ret != oriret || errno != err.value()) {
      panic("No sync");
    }
    if (ret == 0) {
      if (st.st_mode != statbuf->st_mode || st.st_uid != statbuf->st_uid ||
          st.st_gid != statbuf->st_gid || st.st_ino != statbuf->st_ino ||
          st.st_size != statbuf->st_size) {
        panic("No sync");
      }
    }
  }
  return ret;
}

off_t OpenFileTable::lseek(int fd, off_t offset, int whence,
                           std::error_code &err) {
  err.clear();
  if ((long)vec.size() <= fd || !vec[fd]) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    if (isReal()) {
      check_eq(err.value(), EBADF);
    }
    return -1;
  }
  return vec[fd]->lseek(offset, whence, err);
}
ssize_t OpenFileTable::read(int fd, void *buf, size_t count,
                            std::error_code &err) {
  err.clear();
  if ((long)vec.size() <= fd || !vec[fd]) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    if (isReal()) {
      check_eq(err.value(), EBADF);
    }
    return -1;
  }
  return vec[fd]->read(buf, count, err);
}
ssize_t OpenFileTable::write(int fd, const void *buf, size_t count,
                             std::error_code &err) {
  err.clear();
  if ((long)vec.size() <= fd || !vec[fd]) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    if (isReal()) {
      check_eq(err.value(), EBADF);
    }
    return -1;
  }
  if (vec[fd]->getFlag() & O_APPEND)
    lseek(fd, 0, SEEK_END, err);
  if (err.value())
    panic("Lost sync for append");
  return vec[fd]->write(buf, count, err);
}
Path OpenFileTable::fgetpath(int fd, std::error_code &err) {
  err.clear();
  if ((long)vec.size() <= fd || !vec[fd]) {
    err = make_system_error_code(std::errc::bad_file_descriptor);
    if (isReal()) {
      check_eq(err.value(), EBADF);
    }
    return Path();
  }
  auto btptr = vec[fd]->getBTPos();
  if (!btptr) {
    err = make_system_error_code(std::errc::no_such_file_or_directory);
    return Path();
  }
  return fsptr->bt2path(vec[fd]->getBTPos(), err);
}

int OpenFileTable::mkdirat(int fd, const char *path, mode_t mode,
                           std::error_code &err) {
  err.clear();
  if (fd == AT_FDCWD) {
    fsptr->newDirectory(Path(path, false), mode, err);
  } else {
    if (isFree(fd)) {
      err = make_system_error_code(std::errc::bad_file_descriptor);
      return -1;
    }
    fsptr->newDirectory(vec[fd]->getStat().st_ino, Path(path, false), mode,
                        err);
  }
  return 0;
}

int OpenFileTable::unlinkat(int fd, const char *pathname, int flag,
                            std::error_code &err) {
  err.clear();
  if (fd == AT_FDCWD) {
    if (flag & AT_REMOVEDIR) {
      fsptr->rmdir(Path(pathname, false), err);
    } else {
      fsptr->unlink(Path(pathname, false), err);
    }
  } else {
    if (isFree(fd)) {
      err = make_system_error_code(std::errc::bad_file_descriptor);
      return -1;
    }
    if (flag & AT_REMOVEDIR) {
      fsptr->rmdir(vec[fd]->getStat().st_ino, Path(pathname, false), err);
    } else {
      fsptr->unlink(vec[fd]->getStat().st_ino, Path(pathname, false), err);
    }
  }
  return 0;
}

int OpenFileTable::fchmod(int fd, mode_t mode, std::error_code &err) {
  err.clear();
  ino_t ino = resolveFdToIno(fd, err);
  if (err.value())
    return -1;
  fsptr->chmod(ino, mode, err);
  if (err.value()) {
    return -1;
  } else {
    return 0;
  }
}

int OpenFileTable::fchmodat(int dirfd, const char *pathname, mode_t mode,
                            int flags, std::error_code &err) {
  err.clear();
  if (pathname == nullptr) {
    err = make_system_error_code(std::errc::bad_address);
    return -1;
  }
  ino_t baseino = resolveFdToIno(dirfd, err);
  if (err.value())
    return -1;
  bool follow_symlink = !(flags & AT_SYMLINK_NOFOLLOW);
  ino_t ino = fsptr->query(baseino, Path(pathname, false), F_OK, follow_symlink,
                           nullptr, err);
  if (err.value())
    return -1;
  fsptr->chmod(ino, mode, err);
  if (err.value()) {
    return -1;
  } else {
    return 0;
  }
}

char *OpenFileTable::realpath(const char *path, char *resolved_path,
                              std::error_code &err) {
  err.clear();
  char *allocated = nullptr;
  if (resolved_path == nullptr) {
    allocated = (char *)malloc(MAXPATHLEN);
    if (!allocated) {
      err = make_system_error_code(std::errc::not_enough_memory);
      return nullptr;
    }
  }
  Path ret;
  fsptr->query(Path(path, false), F_OK, true, &ret, err);
  if (err.value())
    return nullptr;
  if (resolved_path) {
    strcpy(resolved_path, ret.c_str());
    return resolved_path;
  } else {
    strcpy(allocated, ret.c_str());
    return allocated;
  }
}

ino_t OpenFileTable::resolveFdToIno(int fd, std::error_code &err) {
  if (fd == AT_FDCWD) {
    return fsptr->getCwdIno();
  } else {
    if (isFree(fd)) {
      err = make_system_error_code(std::errc::bad_file_descriptor);
      return -1;
    }
    return vec[fd]->getStat().st_ino;
  }
}

bool OpenFileTable::shouldOrigin(int fd) {
  if (isFree(fd))
    return true;
  auto open = vec[fd];
  auto st = open->getStat();
  if (S_ISCHR(st.st_mode))
    return false;
  return true;
}

bool OpenFileTable::isPipe(int fd) {
  if (isFree(fd))
    return false;
  return S_ISFIFO(vec[fd]->getStat().st_mode);
}

} // namespace accmut
