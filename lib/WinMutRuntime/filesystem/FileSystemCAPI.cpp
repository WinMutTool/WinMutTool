#include <dlfcn.h>
#include <fcntl.h>
#include <llvm/WinMutRuntime/checking/panic.h>
#include <llvm/WinMutRuntime/filesystem/Dir.h>
#include <llvm/WinMutRuntime/filesystem/ErrorCode.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemCAPI.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/DirectoryFile.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemInternalAPI/WrapperMacros.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/filesystem/MirrorFileSystem.h>
#include <llvm/WinMutRuntime/filesystem/OpenFileTable.h>
#include <llvm/WinMutRuntime/init/init.h>
#include <llvm/WinMutRuntime/logging/LogTheFunc.h>
#include <llvm/WinMutRuntime/sysdep/macros.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <llvm/Transforms/WinMut/DebugMacro.h>
// ------------------- mut output for demo site ---------------------
#include <llvm/WinMutRuntime/filesystem/MutOutput.h>
// ------------------- mut output for demo site ---------------------

// #define LOGGING

using namespace accmut;

std::ostream& operator << (std::ostream& os, const struct stat &st) {
  auto f = os.flags();
  os << "stat { st_dev = 0h" << std::hex << st.st_dev << ", st_ino = " << std::dec << st.st_ino
     << ", st_mode = " << std::oct << st.st_mode << ", st_nlink = " << std::dec << st.st_nlink
     << ", st_uid = " << st.st_uid << ", st_gid = " << st.st_gid << " }";
  os.flags(f);
  return os;
}

std::ostream& operator << (std::ostream& os, const struct stat64 &st) {
  auto f = os.flags();
  os << "stat { st_dev = 0h" << std::hex << st.st_dev << ", st_ino = " << std::dec << st.st_ino
     << ", st_mode = " << std::oct << st.st_mode << ", st_nlink = " << std::dec << st.st_nlink
     << ", st_uid = " << st.st_uid << ", st_gid = " << st.st_gid << " }";
  os.flags(f);
  return os;
}

std::ostream& operator << (std::ostream &os, const dirent &d) {
  auto f = os.flags();
  os << "dirent { d_ino = " << std::dec << d.d_ino << ", d_off = " << d.d_off
     << ", d_reclen = " << d.d_reclen << ", d_type = " << (unsigned int)d.d_type
     << ", d_name = " << d.d_name << " }";
  os.flags(f);
  return os;
}

std::ostream& operator << (std::ostream &os, const FILE &f) {
  os << "FILE@" << (const void*)&f;
  return os;
}

std::string DIRp2str(const DIR *p) {
  std::stringstream ss;
  ss << "DIR@" << (const void*)p;
  return ss.str();
}

std::string DIRany2str(const std::any &a) {
  return DIRp2str(std::any_cast<DIR*>(a));
}

extern "C" {

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds,
           struct timeval *timeout) {
  auto resolveFDSet = [](int nfds, fd_set *s) {
    if (!s)
      return std::string("NULL");
    std::stringstream ss;
    ss << "[";
    bool is_first = true;
    for (int i = 0; i < nfds; ++i) {
      if (FD_ISSET(i, s)) {
        if (is_first)
          is_first = false;
        else
          ss << ", ";
        ss << i;
      }
    }
    ss << "]";
    return ss.str();
  };
  auto resolveTimeout = [](struct timeval *t) {
    if (!t)
      return std::string("NULL");
    std::stringstream ss;
    ss << t->tv_sec * 1000000 + t->tv_usec << "us";
    return ss.str();
  };
  return LogTheFunc<int>(
             [&]() {
               return __accmut_libc_select(nfds, readfds, writefds, errorfds,
                                           timeout);
             },
             "select", nfds, resolveFDSet(nfds, readfds),
             resolveFDSet(nfds, writefds), resolveFDSet(nfds, errorfds),
             resolveTimeout(timeout))
      .call();
}

/*
void abort(void) {
raise(SIGSTOP);
}*/

int access(const char *pathname, int mode) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_access(pathname, mode);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_access(pathname, mode);
               }
               errnoSetter.enable();
               MirrorFileSystem::hold();
               auto mfs = MirrorFileSystem::getInstance();
#ifdef __APPLE__
               mode = mode & (F_OK | X_OK | W_OK | R_OK);
#else
               if (mode & (~(F_OK | X_OK | W_OK | R_OK))) {
                 errno = EINVAL;
                 return -1;
               }
#endif
               std::error_code err;
               // ino_t ino =
                   mfs->query(Path(pathname, false), mode, true, nullptr, err);
               if (err.value() != 0) {
                 errnoSetter.set(err.value());
                 if (mfs->isReal()) {
                   int ret = __accmut_libc_access(pathname, mode);
                   if (ret != -1 || errno != err.value()) {
                     panic("No sync");
                   }
                 }
                 return -1;
               }
               if (mfs->isReal()) {
                 int ret = __accmut_libc_access(pathname, mode);
                 if (ret != 0) {
                   panic("No sync");
                 }
               }
               return 0;
             },
             "access", pathname, mode)
      .call();
}

int chdir(const char *path) {
  return LogTheFunc<int>(
             [&]() {
               FS_CAPI_SETUP(errnoSetter, err, __accmut_libc_chdir, path);
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               int ret = oft->chdir(path, err);
               FS_CAPI_FINISH(errnoSetter, ret, err, __accmut_libc_chdir, path);
               if (MUTATION_ID == 0) {
                 // checking inside
                 char buf[MAXPATHLEN];
                 getcwd(buf, MAXPATHLEN);
               }
               return ret;
             },
             __FUNCTION__, path)
      .call();
}

char *getcwd(char *buf, size_t size) {
  return LogTheFunc<char *>(
             [&]() -> char * {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_getcwd(buf, size);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_getcwd(buf, size);
               }
               errnoSetter.enable();
               MirrorFileSystem::hold();
               auto mfs = MirrorFileSystem::getInstance();
               std::error_code err;
               auto p = mfs->getwd(err);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 return nullptr;
               }
               auto len = strlen(p.c_str());
               if (len + 1 > size) {
                 errnoSetter.set(ERANGE);
                 return nullptr;
               }
               strcpy(buf, p.c_str());
               return buf;
             },
             "getcwd", buf, size)
      .call();
}

char *getwd(char *buf) {
  return LogTheFunc<char *>(
             [&]() -> char * {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_getwd(buf);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_getwd(buf);
               }
               errnoSetter.enable();
               MirrorFileSystem::hold();
               auto mfs = MirrorFileSystem::getInstance();
               bool should_free = false;
               if (buf == nullptr) {
                 buf = static_cast<char *>(malloc(MAXPATHLEN));
                 should_free = true;
               }
               std::error_code err;
               auto p = mfs->getwd(err);
               if (err.value()) {
                 if (should_free)
                   free(buf);
                 errnoSetter.set(err.value());
                 return nullptr;
               }
               strcpy(buf, p.c_str());
               return buf;
             },
             "getwd", buf)
      .call();
}

int lstat(const char *pathname, struct stat *buf) {
  return LogTheFunc<int>(
             [&]() -> int {
               if (unlikely(system_disabled()))
                 return __accmut_libc_lstat(pathname, buf);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_lstat(pathname, buf);
               }
               return fstatat(AT_FDCWD, pathname, buf, AT_SYMLINK_NOFOLLOW);
             },
             "lstat", pathname, buf)
      .call();
}

int mkdir(const char *pathname, mode_t mode) {
  return LogTheFunc<int>(
             [&]() -> int {
               if (unlikely(system_disabled()))
                 return __accmut_libc_mkdir(pathname, mode);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_mkdir(pathname, mode);
               }
               return mkdirat(AT_FDCWD, pathname, mode);
             },
             "mkdir", pathname, mode)
      .call();
}

int mkdirat(int fd, const char *path, mode_t mode) {
  return LogTheFunc<int>(
             [&]() -> int {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_mkdirat(fd, path, mode);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_mkdirat(fd, path, mode);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               std::error_code err;
               oft->mkdirat(fd, path, mode, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 return -1;
               }
               return 0;
             },
             "mkdirat", fd, path, mode)
      .call();
}

static const char padchar[] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
typedef enum { FTPP_DONE, FTPP_TRY_NEXT, FTPP_ERROR } find_temp_path_progress_t;
typedef find_temp_path_progress_t (*find_temp_path_action_t)(int dfd,
                                                             char *path,
                                                             void *ctx,
                                                             void *result);

#ifndef __APPLE__
#define __unused
#endif

/* For every path matching a given template, invoke an action. Depending on
 * the progress reported by action, stops or tries the next path.
 * Returns 1 if succeeds, 0 and sets errno if fails.
 */
static int find_temp_path(int dfd, char *path, int slen, bool stat_base_dir,
                          find_temp_path_action_t action, void *action_ctx,
                          void *action_result) {
  char *start, *trv, *suffp, *carryp;
  const char *pad;
  struct stat sbuf;
  int rval;
  uint32_t rand;
  char carrybuf[MAXPATHLEN];

  if (slen < 0) {
    errno = EINVAL;
    return (0);
  }

  for (trv = path; *trv != '\0'; ++trv)
    ;
  if (trv - path >= MAXPATHLEN) {
    errno = ENAMETOOLONG;
    return (0);
  }
  trv -= slen;
  suffp = trv;
  --trv;
  if (trv < path || NULL != strchr(suffp, '/')) {
    errno = EINVAL;
    return (0);
  }

  /* Fill space with random characters */
  while (trv >= path && *trv == 'X') {
#ifdef __APPLE__
    rand = arc4random_uniform(sizeof(padchar) - 1);
#else
    rand = random() % (sizeof(padchar) - 1);
#endif
    *trv-- = padchar[rand];
  }
  start = trv + 1;

  /* save first combination of random characters */
  memcpy(carrybuf, start, suffp - start);

  /*
   * check the target directory.
   */
  if (stat_base_dir) {
    for (; trv > path; --trv) {
      if (*trv == '/') {
        *trv = '\0';
        rval = fstatat(dfd, path, &sbuf, 0);
        *trv = '/';
        if (rval != 0)
          return (0);
        if (!S_ISDIR(sbuf.st_mode)) {
          errno = ENOTDIR;
          return (0);
        }
        break;
      }
    }
  }

  for (;;) {
    switch (action(dfd, path, action_ctx, action_result)) {
    case FTPP_DONE:
      return (1);
    case FTPP_ERROR:
      return (0); // errno must be set by the action
    default:;     // FTPP_TRY_NEXT, fall-through
    }

    /* If we have a collision, cycle through the space of filenames */
    for (trv = start, carryp = carrybuf;;) {
      /* have we tried all possible permutations? */
      if (trv == suffp) {
        /* yes - exit with EEXIST */
        errno = EEXIST;
        return (0);
      }
      pad = strchr(padchar, *trv);
      if (pad == NULL) {
        /* this should never happen */
        errno = EIO;
        return (0);
      }
      /* increment character */
      *trv = (*++pad == '\0') ? padchar[0] : *pad;
      /* carry to next position? */
      if (*trv == *carryp) {
        /* increment position and loop */
        ++trv;
        ++carryp;
      } else {
        /* try with new name */
        break;
      }
    }
  }
  /*NOTREACHED*/
}

static find_temp_path_progress_t _mkdtemp_action(int dfd, char *path,
                                                 void *ctx __unused,
                                                 void *result __unused) {
  if (mkdirat(dfd, path, 0700) == 0)
    return FTPP_DONE;
  return (errno == EEXIST) ? FTPP_TRY_NEXT : FTPP_ERROR; // errno is set already
}

static find_temp_path_progress_t
_mktemp_action(int dfd, char *path, void *ctx __unused, void *result __unused) {
  struct stat sbuf;
  if (fstatat(dfd, path, &sbuf, AT_SYMLINK_NOFOLLOW)) {
    return (errno == ENOENT) ? FTPP_DONE : FTPP_ERROR;
  }
  return FTPP_TRY_NEXT;
}

static find_temp_path_progress_t _mkostemps_action(int dfd, char *path,
                                                   void *ctx, void *result) {
  int oflags = (ctx != NULL) ? *((int *)ctx) : 0;
  int fd = openat(dfd, path, O_CREAT | O_EXCL | O_RDWR | oflags, 0600);
  if (fd >= 0) {
    *((int *)result) = fd;
    return FTPP_DONE;
  }
  return (errno == EEXIST) ? FTPP_TRY_NEXT : FTPP_ERROR;
}

char *mkdtemp64(char *path) {
  return mkdtemp(path);
}

char *mkdtemp(char *path) {
  return LogTheFunc<char *>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_mkdtemp(path);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_mkdtemp(path);
               }
               return (find_temp_path(AT_FDCWD, path, 0, true, _mkdtemp_action,
                                      nullptr, nullptr)
                           ? path
                           : (char *)NULL);
             },
             "mkdtemp", path)
      .call();
}

char *mktemp64(char *path) {
  return mktemp(path);
}

char *mktemp(char *path) {
  return LogTheFunc<char *>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_mktemp(path);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_mktemp(path);
               }
               return (find_temp_path(AT_FDCWD, path, 0, false, _mktemp_action,
                                      nullptr, nullptr)
                           ? path
                           : (char *)nullptr);
             },
             "mktemp", path)
      .call();
}

int mkstemp64(char *path) {
  return mkstemp(path);
}

int mkstemp(char *path) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_mkstemp(path);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_mkstemp(path);
               }
               int fd;
               return (find_temp_path(AT_FDCWD, path, 0, true,
                                      _mkostemps_action, nullptr, &fd)
                           ? fd
                           : -1);
             },
             "mkstemp", path)
      .call();
}

#define ALLOWED_MKOSTEMP_FLAGS (O_APPEND | O_CLOEXEC)

int mkostemp64(char *path, int oflags) {
  return mkostemp(path, oflags);
}

int mkostemp(char *path, int oflags) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_mkostemp(path, oflags);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_mkostemp(path, oflags);
               }
               int fd;
               if (oflags & ~ALLOWED_MKOSTEMP_FLAGS) {
                 errno = EINVAL;
                 return -1;
               }
               return (find_temp_path(AT_FDCWD, path, 0, true,
                                      _mkostemps_action, &oflags, &fd)
                           ? fd
                           : -1);
             },
             "mkostemp", path, oflags)
      .call();
}

int mkstemps64(char *path, int slen) {
  return mkstemps(path, slen);
}

int mkstemps(char *path, int slen) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_mkstemps(path, slen);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_mkstemps(path, slen);
               }
               int fd;
               return (find_temp_path(AT_FDCWD, path, slen, true,
                                      _mkostemps_action, nullptr, &fd)
                           ? fd
                           : -1);
             },
             "mkstemps", path, slen)
      .call();
}

int mkostemps64(char *path, int slen, int oflags) {
  return mkostemps(path, slen, oflags);
}

int mkostemps(char *path, int slen, int oflags) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_mkostemps(path, slen, oflags);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_mkostemps(path, slen, oflags);
               }
               int fd;
               if (oflags & ~ALLOWED_MKOSTEMP_FLAGS) {
                 errno = EINVAL;
                 return -1;
               }
               return (find_temp_path(AT_FDCWD, path, slen, true,
                                      _mkostemps_action, &oflags, &fd)
                           ? fd
                           : -1);
             },
             "mkostemps", path, slen, oflags)
      .call();
}

#ifndef __APPLE__
static bool issetugid() {
  if (getuid() != geteuid())
    return true;
  if (getgid() != getegid())
    return true;
  return false;
}
#endif

FILE *tmpfile64(void) {
  return tmpfile();
}

FILE *tmpfile(void) {
  return LogTheFunc<FILE *>(
             [&]() -> FILE * {
               if (unlikely(system_disabled()))
                 return __accmut_libc_tmpfile();
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_tmpfile();
               }
               sigset_t set, oset;
               FILE *fp;
               int fd, sverrno;
               char *buf;
               const char *tmpdir;
               tmpdir = nullptr;
               if (issetugid() == 0)
                 tmpdir = getenv("TMPDIR");
               if (tmpdir == nullptr)
                 tmpdir = "/tmp";
               if (*tmpdir == '\0')
                 return nullptr;
               (void)asprintf(&buf, "%s%s%s", tmpdir,
                              (tmpdir[strlen(tmpdir) - 1] == '/') ? "" : "/",
                              "tmp.XXXXXX");
               if (buf == nullptr)
                 return nullptr;
               sigfillset(&set);
               (void)sigprocmask(SIG_BLOCK, &set, &oset);
               fd = mkstemp(buf);
               if (fd != -1)
                 (void)unlink(buf);
               free(buf);
               (void)sigprocmask(SIG_SETMASK, &oset, nullptr);
               if (fd == -1)
                 return nullptr;
               if ((fp = fdopen(fd, "w+")) == nullptr) {
                 sverrno = errno;
                 (void)close(fd);
                 errno = sverrno;
                 return nullptr;
               }
               return fp;
             },
             "tmpfile")
      .call();
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
  return LogTheFunc<ssize_t>(
             [&]() -> ssize_t {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_readlink(pathname, buf, bufsiz);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_readlink(pathname, buf, bufsiz);
               }
               errnoSetter.enable();
               MirrorFileSystem::hold();
               auto mfs = MirrorFileSystem::getInstance();
               std::error_code err;
               auto ino =
                   mfs->query(Path(pathname, false), F_OK, false, nullptr, err);
               if (!err.value() && !mfs->isLnk(ino))
                 err = make_system_error_code(std::errc::invalid_argument);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 goto check;
               }

  check:
               return 0;
             },
             "readlink", pathname, buf, bufsiz)
      .call();
}

int rename(const char *oldpath, const char *newpath) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_rename(oldpath, newpath);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_rename(oldpath, newpath);
               }
               errnoSetter.enable();
               MirrorFileSystem::hold();
               auto mfs = MirrorFileSystem::getInstance();
               std::error_code err;
               mfs->rename(Path(oldpath, false), Path(newpath, false), err);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 return -1;
               }
               return 0;
             },
             "rename", oldpath, newpath)
      .call();
}

int rmdir(const char *pathname) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_rmdir(pathname);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_rmdir(pathname);
               }
               errnoSetter.enable();
               MirrorFileSystem::hold();
               auto mfs = MirrorFileSystem::getInstance();
               std::error_code err;
               mfs->rmdir(Path(pathname, false), err);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 return -1;
               }
               return 0;
             },
             "rmdir", pathname)
      .call();
}

int unlink(const char *pathname) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_unlink(pathname);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_unlink(pathname);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               std::error_code err;
               oft->unlinkat(AT_FDCWD, pathname, 0, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 return -1;
               }
               return 0;
             },
             "unlink", pathname)
      .call();
}

int unlinkat(int fd, const char *pathname, int flag) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_unlinkat(fd, pathname, flag);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_unlinkat(fd, pathname, flag);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               std::error_code err;
               oft->unlinkat(fd, pathname, flag, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 return -1;
               }
               return 0;
             },
             "unlinkat", fd, pathname, flag)
      .call();
}

int remove(const char *pathname) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_remove(pathname);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_remove(pathname);
               }
               errnoSetter.enable();
               MirrorFileSystem::hold();
               auto mfs = MirrorFileSystem::getInstance();
               std::error_code err;
               mfs->remove(Path(pathname, false), err);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 return -1;
               }
               return 0;
             },
             "remove", pathname)
      .call();
}

int stat(const char *pathname, struct stat *buf) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_stat(pathname, buf);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_stat(pathname, buf);
               }
               return fstatat(AT_FDCWD, pathname, buf, 0);
             },
             "stat", pathname, buf)
      .call();
}

int closedir(DIR *_dirp) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled())) {
                 return __accmut_libc_closedir(_dirp);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_closedir(_dirp);
               }
               auto *dirp = reinterpret_cast<ACCMUT_V3_DIR *>(_dirp);
               close(dirp->fd);
               delete dirp;
               return 0;
             },
             "closedir", LogArgument(_dirp, DIRany2str))
      .call();
}

DIR *opendir(const char *name) {
  return LogTheFunc<DIR *>(
             [&]() -> DIR * {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled())) {
                 return __accmut_libc_opendir(name);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_opendir(name);
               }
               errnoSetter.enable();
               int fd = open(name, O_RDONLY | O_EXCL | O_CLOEXEC);
               if (fd < 0) {
                 errnoSetter.disable();
                 return nullptr;
               }
               auto mfs = MirrorFileSystem::getInstance();
               std::error_code err;
               ino_t ino =
                   mfs->query(Path(name, false), R_OK, true, nullptr, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 return nullptr;
               }
               if (!mfs->isDir(ino)) {
                 close(fd);
                 return nullptr;
               }
               return reinterpret_cast<DIR *>(
                   new ACCMUT_V3_DIR(fd, mfs->getDirentList(ino)));
             },
             [](DIR * const(&ret)) {
               return DIRp2str(ret);
             },
             "opendir", name)
      .call();
}

DIR *fdopendir(int fd) {
  return LogTheFunc<DIR *>(
             [&]() -> DIR * {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled())) {
                 return __accmut_libc_fdopendir(fd);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_fdopendir(fd);
               }
               errnoSetter.enable();
               auto oft = OpenFileTable::getInstance();
               std::error_code err;

               auto *ret = oft->fdopendir(fd, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
               }
               return reinterpret_cast<DIR *>(ret);
             },
             [](DIR * const(&ret)) {
               return DIRp2str(ret);
             },
             "fdopendir", fd)
      .call();
}

struct dirent *readdir(DIR *_dirp) {
  return LogTheFunc<struct dirent *>(
             [&]() -> struct dirent * {
               if (unlikely(system_disabled())) {
                 return __accmut_libc_readdir(_dirp);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_readdir(_dirp);
               }
               auto *dirp = reinterpret_cast<ACCMUT_V3_DIR *>(_dirp);
               if ((size_t)dirp->idx >= dirp->dirents.size())
                 return nullptr;
               return &dirp->dirents[dirp->idx++];
             },
             "readdir", LogArgument(_dirp, DIRany2str))
      .call();
}

int close(int fd) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled())) {
                 return __accmut_libc_close(fd);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_close(fd);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();

               std::error_code err;
               oft->close(fd, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
                 return -1;
               }
               return 0;
             },
             "close", fd)
      .call();
}
int dup(int oldfd) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled())) {
                 return __accmut_libc_dup(oldfd);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_dup(oldfd);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();

               std::error_code err;
               int ret = oft->dup(oldfd, err);

               if (err.value()) {
                 errnoSetter.set(err.value());
               }
               return ret;
             },
             "dup", oldfd)
      .call();
}

int dup2(int oldfd, int newfd) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled())) {
                 return __accmut_libc_dup2(oldfd, newfd);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_dup2(oldfd, newfd);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               std::error_code err;
               int ret = oft->dup2(oldfd, newfd, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
               }
               return ret;
             },
             "dup2", oldfd, newfd)
      .call();
}

int dup3(int oldfd, int newfd, int flags) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled())) {
                 return __accmut_libc_dup3(oldfd, newfd, flags);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_dup3(oldfd, newfd, flags);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               std::error_code err;
               int ret = oft->dup3(oldfd, newfd, flags, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
               }
               return ret;
             },
             "dup3", oldfd, newfd, flags)
      .call();
}

int fchdir(int fd) {
  return LogTheFunc<int>(
             [&]() {
               FS_CAPI_SETUP(errnoSetter, err, __accmut_libc_fchdir, fd);
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               int ret = oft->fchdir(fd, err);
               FS_CAPI_FINISH(errnoSetter, ret, err, __accmut_libc_fchdir, fd);
               if (MUTATION_ID == 0) {
                 // checking inside
                 char buf[MAXPATHLEN];
                 getcwd(buf, MAXPATHLEN);
               }
               return ret;
             },
             __FUNCTION__, fd)
      .call();
}

int fchown(int fd, uid_t owner, gid_t group) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled())) {
                 return __accmut_libc_fchown(fd, owner, group);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_fchown(fd, owner, group);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();

               std::error_code err;
               oft->fchown(fd, owner, group, err);

               if (err.value()) {
                 errnoSetter.set(err.value());
                 return -1;
               }
               return 0;
             },
             "fchown", fd, owner, group)
      .call();
}
int fcntl(int fd, int cmd, ...) {
  int var = 0;
  if (cmd == F_DUPFD_CLOEXEC || cmd == F_DUPFD || cmd == F_SETFD ||
      cmd == F_SETFL) {
    va_list ap;
    va_start(ap, cmd);
    var = va_arg(ap, int);
    va_end(ap);
  }
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled())) {
                 return __accmut_libc_fcntl(fd, cmd, var);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_fcntl(fd, cmd, var);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();

               std::error_code err;
               int ret = oft->fcntl(fd, cmd, var, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
               }
               return ret;
             },
             "fcntl", fd, cmd, var)
      .call();
}
int fstat(int fd, struct stat *buf) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_fstat(fd, buf);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_fstat(fd, buf);
               }
               return fstatat(AT_FDCWD, "", buf, AT_EMPTY_PATH);
             },
             "fstat", fd, buf)
      .call();
}
int fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_fstatat(dirfd, pathname, statbuf, flags);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_fstatat(dirfd, pathname, statbuf, flags);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               std::error_code err;
               int ret = oft->fstatat(dirfd, pathname, statbuf, flags, err);
               if (err.value())
                 errnoSetter.set(err.value());
               return ret;
             },
             "fstatat", dirfd, pathname, statbuf, flags)
      .call();
}
off_t lseek64(int fd, off_t offset, int whence) {
  return lseek(fd, offset, whence);
}
off_t lseek(int fd, off_t offset, int whence) {
  return LogTheFunc<int>(
             [&]() {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_lseek(fd, offset, whence);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_lseek(fd, offset, whence);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();

               std::error_code err;
               off_t ret = oft->lseek(fd, offset, whence, err);

               if (oft->isReal()) {
                 errno = 0;
                 ssize_t oriret = __accmut_libc_lseek(fd, offset, whence);
                 int errsave = errno;
                 check_eq(errsave, err.value());
                 check_eq(oriret, ret);
               }
               if (err.value()) {
                 errnoSetter.set(err.value());
               }
               return ret;
             },
             "lseek", fd, offset, whence)
      .call();
}
static bool __accmut_is_unsupported_file(const char *pathname) {
  if (pathname[0] == '/' && pathname[1] == 'd' && pathname[2] == 'e' &&
      pathname[3] == 'v' && pathname[4] == '/' && pathname[5] != 0) {
    if (pathname[5] == 'n' && pathname[6] == 'u' && pathname[7] == 'l' && pathname[8] == 'l' &&
        pathname[9] == 0) {
      return false;
    }
    return true;
  }
  return false;
}
static int doopenat(int dirfd, const char *pathname, int flags, int mode) {
  if (unlikely(__accmut_is_unsupported_file(pathname))) {
    disable_system();
    return __accmut_libc_openat(dirfd, pathname, flags, mode);
  }
  ErrnoSetter errnoSetter;
  errnoSetter.enable();
  OpenFileTable::hold();
  auto oft = OpenFileTable::getInstance();
  std::error_code err;
  int fd = oft->openat(dirfd, pathname, flags, mode, err);
  if (err.value()) {
    errnoSetter.set(err.value());
    return -1;
  }
  return fd;
}
int open(const char *pathname, int flags, ...) {
  int mode = 0;
  if (flags & O_CREAT) {
    va_list lst;
    va_start(lst, flags);
    mode = va_arg(lst, int);
    va_end(lst);
  }
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_open(pathname, flags, mode);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_open(pathname, flags, mode);
               }
               int fd = doopenat(AT_FDCWD, pathname, flags, mode);
                // ------------------- mut output for demo site ---------------------
#ifdef MUT_OUTPUT_FOR_DEMO_SITE
                // if (accmut::MutOutput::isStdout(fd))
                //   accmut::MutOutput::getInstance()->writeStdoutFile(buf, count);
                accmut::MutOutput::getInstance()->open_and_register_MutOutputFile(pathname, fd, flags, mode);
#endif
                // ------------------- mut output for demo site ---------------------
               return fd;
             },
             "open", pathname, flags, mode)
      .call();
}
int open64(const char *pathname, int flags, ...) {
  int mode = 0;
  if (flags & O_CREAT) {
    va_list lst;
    va_start(lst, flags);
    mode = va_arg(lst, int);
    va_end(lst);
  }
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_open(pathname, flags, mode);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_open(pathname, flags, mode);
               }
               return doopenat(AT_FDCWD, pathname, flags, mode);
             },
             "open64", pathname, flags, mode)
      .call();
}
int creat(const char *pathname, mode_t mode) {
  return LogTheFunc<int>(
             [&]() {
               return open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
             },
             "creat", pathname, mode)
      .call();
}
int creat64(const char *pathname, mode_t mode) {
  return LogTheFunc<int>(
             [&]() {
               return open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
             },
             "creat64", pathname, mode)
      .call();
}
int openat(int dirfd, const char *pathname, int flags, ...) {
  int mode = 0;
  if (flags & O_CREAT) {
    va_list lst;
    va_start(lst, flags);
    mode = va_arg(lst, int);
    va_end(lst);
  }
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_openat(dirfd, pathname, flags, mode);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_openat(dirfd, pathname, flags, mode);
               }
               return doopenat(dirfd, pathname, flags, mode);
             },
             "openat", dirfd, pathname, flags, mode)
      .call();
}
int openat64(int dirfd, const char *pathname, int flags, ...) {
  int mode = 0;
  if (flags & O_CREAT) {
    va_list lst;
    va_start(lst, flags);
    mode = va_arg(lst, int);
    va_end(lst);
  }

  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_openat(dirfd, pathname, flags, mode);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_openat(dirfd, pathname, flags, mode);
               }
               return doopenat(dirfd, pathname, flags, mode);
             },
             "openat64", dirfd, pathname, flags, mode)
      .call();
}
ssize_t read(int fd, void *buf, size_t count) {
  return LogTheFunc<ssize_t>(
             [&]() -> ssize_t {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_read(fd, buf, count);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_read(fd, buf, count);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();

               std::error_code err;
               if (oft->shouldOrigin(fd)) {
                 ssize_t ret = oft->read(fd, buf, count, err);

                 if (oft->isReal()) {
                   const int smallbufSize = 4096;
                   char smallbuf[smallbufSize];
                   static char *largebuf;
                   static size_t largebufSize = 0;

                   char *buf1;
                   if (count < 4096) {
                     buf1 = smallbuf;
                   } else if (largebufSize < count) {
                     if (largebufSize != 0) {
                       delete[] largebuf;
                     }
                     largebuf = new char[2 * count];
                     largebufSize = 2 * count;
                     buf1 = largebuf;
                   } else {
                     buf1 = largebuf;
                   } 

                   if (!oft->isPipe(fd)) {
                     errno = 0;
                     ssize_t oriret = __accmut_libc_read(fd, buf1, count);
                     int errsave = errno;
		     if (oriret != ret) raise(SIGSTOP);
                     check_eq(errsave, err.value());
                     check_eq(oriret, ret);
                     if (oriret != -1)
                       check_mem(buf, buf1, oriret);
                   }
                 }
                 if (err.value())
                   errnoSetter.set(err.value());
                 return ret;
               } else {
                 if (oft->isReal()) {
                   errnoSetter.disable();
                   // for pipe or tty
                   return __accmut_libc_read(fd, buf, count);
                 } else {
                   if (oft->isFree(fd)) {
                     errnoSetter.set(EBADF);
                   }
                   return 0;
                 }
               }
             },
             "read", fd, (char *)buf, count)
      .call();
}
ssize_t write(int fd, const void *buf, size_t count) {
  return LogTheFunc<ssize_t>(
             [&]() -> ssize_t {
               ErrnoSetter errnoSetter;
               if (unlikely(system_disabled()))
                 return __accmut_libc_write(fd, buf, count);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_write(fd, buf, count);
               }
               errnoSetter.enable();
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();

               std::error_code err;
               if (oft->isFree(fd)) {
                 errnoSetter.set(EBADF);
                 return -1;
               }
               if (oft->isPipe(fd)) {
                 if (MUTATION_ID == 0) {
                   errnoSetter.disable();
                   return __accmut_libc_write(fd, buf, count);
                 }
                 return count;
               }
               if (oft->shouldOrigin(fd)) {

                  // ------------------- mut output for demo site ---------------------
#ifdef MUT_OUTPUT_FOR_DEMO_SITE
                  // if (accmut::MutOutput::isStdout(fd))
                  //   accmut::MutOutput::getInstance()->writeStdoutFile(buf, count);

                  accmut::MutOutput::getInstance()->write_registered_MutOutputFile(fd, buf, count);
#endif
                  // ------------------- mut output for demo site ---------------------

                 ssize_t ret = oft->write(fd, buf, count, err);
                 if (oft->isReal()) {
                   errno = 0;
                   ssize_t oriret = __accmut_libc_write(fd, buf, count);
                   int errsave = errno;
                   check_eq(errsave, err.value());
                   check_eq(oriret, ret);
                 }
                 if (err.value())
                   errnoSetter.set(err.value());
                 return ret;
               } else {
                  // ------------------- mut output for demo site ---------------------
#ifdef MUT_OUTPUT_FOR_DEMO_SITE
                  // if (accmut::MutOutput::isStdout(fd))
                  //   accmut::MutOutput::getInstance()->writeStdoutFile(buf, count);
                  if (fd == 1)
                    accmut::MutOutput::getInstance()->write_registered_MutOutputFile(fd, buf, count);
#endif
                  // ------------------- mut output for demo site ---------------------
                 if (oft->isReal()) {
                   errnoSetter.disable();
                   return __accmut_libc_write(fd, buf, count);
                 }
                  return count;
               }
             },
             "write", fd, (const char *)buf, count)
      .call();
}
void perror(const char *s) {
  LogTheFunc<void>(
      [&]() {
        if (unlikely(system_disabled()))
          return __accmut_libc_perror(s);
        if (unlikely(!system_initialized())) {
          disable_system();
          return __accmut_libc_perror(s);
        }
        fprintf(stderr, "%s: %s\n", s, strerror(errno));
      },
      "perror", s);
}


int chmod(const char *pathname, mode_t mode) {
  return LogTheFunc<int>(
             [&]() {
               FS_CAPI_SETUP(errnoSetter, err, __accmut_libc_chmod, pathname,
                             mode);
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               int ret = oft->fchmodat(AT_FDCWD, pathname, mode, 0, err);
               FS_CAPI_FINISH(errnoSetter, ret, err, __accmut_libc_chmod,
                              pathname, mode);
               if (MUTATION_ID == 0) {
                 struct stat buf;
                 stat(pathname, &buf);
               }
               return ret;
             },
             __FUNCTION__, pathname, mode)
      .call();
}

int fchmod(int fd, mode_t mode) {
  return LogTheFunc<int>(
             [&]() {
               FS_CAPI_SETUP(errnoSetter, err, __accmut_libc_fchmod, fd, mode);
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               int ret = oft->fchmod(fd, mode, err);
               FS_CAPI_FINISH(errnoSetter, ret, err, __accmut_libc_fchmod, fd,
                              mode);
               if (MUTATION_ID == 0) {
                 struct stat buf;
                 fstat(fd, &buf);
               }
               return ret;
             },
             __FUNCTION__, fd, mode)
      .call();
}

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
  return LogTheFunc<int>(
             [&]() {
               FS_CAPI_SETUP(errnoSetter, err, __accmut_libc_fchmodat, dirfd,
                             pathname, mode, flags);
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               int ret = oft->fchmodat(dirfd, pathname, mode, flags, err);
               FS_CAPI_FINISH(errnoSetter, ret, err, __accmut_libc_fchmodat,
                              dirfd, pathname, mode, flags);
               if (MUTATION_ID == 0) {
                 struct stat buf;
                 fstatat(dirfd, pathname, &buf, flags & AT_SYMLINK_NOFOLLOW);
               }
               return ret;
             },
             __FUNCTION__, dirfd, pathname, mode, flags)
      .call();
}
int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group,
             int flags) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_fchownat(dirfd, pathname, owner, group,
                                               flags);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_fchownat(dirfd, pathname, owner, group,
                                               flags);
               }
               panic("Not implemented");
             },
             "fchownat", dirfd, pathname, owner, group, flags)
      .call();
}
ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
  return LogTheFunc<ssize_t>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_readlinkat(dirfd, pathname, buf, bufsiz);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_readlinkat(dirfd, pathname, buf, bufsiz);
               }
               panic("Not implemented");
             },
             "readlinkat", dirfd, pathname, buf, bufsiz)
      .call();
}
int renameat(int olddirfd, const char *oldpath, int newdirfd,
             const char *newpath) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled()))
                 return __accmut_libc_renameat(olddirfd, oldpath, newdirfd,
                                               newpath);
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_renameat(olddirfd, oldpath, newdirfd,
                                               newpath);
               }
               panic("Not implemented");
             },
             "renameat", olddirfd, oldpath, newdirfd, newpath)
      .call();
}
/*
int renameat2(int olddirfd, const char *oldpath, int newdirfd,
                          const char *newpath, unsigned int flags) {
  if (unlikely(system_disabled()))
    return __accmut_libc_renameat2(olddirfd, oldpath, newdirfd, newpath, flags);
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_renameat2(olddirfd, oldpath, newdirfd, newpath, flags);
  }
  panic("Not implemented");
}
*/
pid_t fork(void) {
  return LogTheFunc<pid_t>(
             [&]() {
               if (unlikely(system_disabled())) {
                 return __accmut_libc_fork();
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_fork();
               }
               panic("fork not implemented");
             },
             "fork")
      .call();
}
int pipe(int pipefd[2]) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled())) {
                 return __accmut_libc_pipe(pipefd);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_pipe(pipefd);
               }
               panic("pipe not implemented");
             },
             "pipe", pipefd)
      .call();
}
int pipe2(int pipefd[2], int flags) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled())) {
                 return __accmut_libc_pipe2(pipefd, flags);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_pipe2(pipefd, flags);
               }
               panic("pipe2 not implemented");
             },
             "pipe2", pipefd, flags)
      .call();
}
int socket(int domain, int type, int protocol) {
  return LogTheFunc<int>(
             [&]() {
               if (unlikely(system_disabled())) {
                 return __accmut_libc_socket(domain, type, protocol);
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_socket(domain, type, protocol);
               }
               panic("socket not implemented");
             },
             "socket", domain, type, protocol)
      .call();
}

pid_t vfork_panic(void) {
  panic("vfork not implemented");
}

vfork_func_ty vfork_impl(void) {
  if (unlikely(system_disabled())) {
    return __accmut_get_libc_vfork_func();
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_get_libc_vfork_func();
  }
  return vfork_panic;
}

/*
pid_t vfork(void) {
  return LogTheFunc<pid_t>(
             [&]() {
               if (unlikely(system_disabled())) {
                 return __accmut_libc_vfork();
               }
               if (unlikely(!system_initialized())) {
                 disable_system();
                 return __accmut_libc_vfork();
               }
               panic("vfork not implemented");
             },
             "vfork")
      .call();
}
*/

int execve(const char *path, char *const argv[], char *const envp[]) {
  if (unlikely(system_disabled())) {
    return __accmut_libc_execve(path, argv, envp);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_execve(path, argv, envp);
  }
  panic("execve not implemented");
}
int execvl(const char *path, const char *arg0, ...) {
  if (unlikely(system_disabled())) {
    std::vector<const char *> argv;
    va_list ap;
    va_start(ap, arg0);
    while (arg0 != nullptr) {
      argv.push_back(arg0);
      arg0 = va_arg(ap, const char *);
    }
    va_end(ap);
    argv.push_back(nullptr);

    return __accmut_libc_execv(path, const_cast<char *const *>(argv.data()));
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    std::vector<const char *> argv;
    va_list ap;
    va_start(ap, arg0);
    while (arg0 != nullptr) {
      argv.push_back(arg0);
      arg0 = va_arg(ap, const char *);
    }
    va_end(ap);
    argv.push_back(nullptr);

    return __accmut_libc_execv(path, const_cast<char *const *>(argv.data()));
  }
  panic("execvl not implemented");
}
int execv(const char *path, char *const argv[]) {
  if (unlikely(system_disabled())) {
    return __accmut_libc_execv(path, argv);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_execv(path, argv);
  }
  panic("execv not implemented");
}
int execle(const char *path, const char *arg0, ...) {
  if (unlikely(system_disabled())) {
    char **envp;
    std::vector<const char *> argv;
    va_list ap;
    va_start(ap, arg0);
    while (arg0 != nullptr) {
      argv.push_back(arg0);
      arg0 = va_arg(ap, const char *);
    }
    envp = va_arg(ap, char **);
    va_end(ap);
    argv.push_back(nullptr);

    return __accmut_libc_execve(path, const_cast<char *const *>(argv.data()),
                                envp);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    char **envp;
    std::vector<const char *> argv;
    va_list ap;
    va_start(ap, arg0);
    while (arg0 != nullptr) {
      argv.push_back(arg0);
      arg0 = va_arg(ap, const char *);
    }
    envp = va_arg(ap, char **);
    va_end(ap);
    argv.push_back(nullptr);

    return __accmut_libc_execve(path, const_cast<char *const *>(argv.data()),
                                envp);
  }
  panic("execvle not implemented");
}
int execvp(const char *filename, char *const argv[]) {
  if (unlikely(system_disabled())) {
    return __accmut_libc_execvp(filename, argv);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_execvp(filename, argv);
  }
  panic("execvp not implemented");
}
int execlp(const char *filename, const char *arg0, ...) {
  if (unlikely(system_disabled())) {
    std::vector<const char *> argv;
    va_list ap;
    va_start(ap, arg0);
    while (arg0 != nullptr) {
      argv.push_back(arg0);
      arg0 = va_arg(ap, const char *);
    }
    va_end(ap);
    argv.push_back(nullptr);
    return __accmut_libc_execvp(filename,
                                const_cast<char *const *>(argv.data()));
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    std::vector<const char *> argv;
    va_list ap;
    va_start(ap, arg0);
    while (arg0 != nullptr) {
      argv.push_back(arg0);
      arg0 = va_arg(ap, const char *);
    }
    va_end(ap);
    argv.push_back(nullptr);
    return __accmut_libc_execvp(filename,
                                const_cast<char *const *>(argv.data()));
  }
  panic("execlp not implemented");
}
int fexecve(int fd, char *const argv[], char *const envp[]) {
  if (unlikely(system_disabled())) {
    return __accmut_libc_fexecve(fd, argv, envp);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_fexecve(fd, argv, envp);
  }
  panic("fexecve not implemented");
}
// assume 64 bit
static_assert(sizeof(ino_t) == sizeof(ino64_t),
              "assume ino_t has the same size as ino64_t");

int fstatat64(int dirfd, const char *pathname, struct stat64 *statbuf,
              int flags) {
  return LogTheFunc<int>(
             [&]() {
               return fstatat(dirfd, pathname, (struct stat *)statbuf, flags);
             },
             "fstatat64", dirfd, pathname, statbuf, flags)
      .call();
}
int stat64(const char *pathname, struct stat64 *buf) {
  return LogTheFunc<int>([&]() { return stat(pathname, (struct stat *)buf); },
                         "stat64", pathname, buf)
      .call();
}
int fstat64(int fd, struct stat64 *buf) {
  return LogTheFunc<int>([&]() { return fstat(fd, (struct stat *)buf); },
                         "fstat64", fd, buf)
      .call();
}
int lstat64(const char *pathname, struct stat64 *buf) {
  return LogTheFunc<int>([&]() { return lstat(pathname, (struct stat *)buf); },
                         "lstat64", pathname, buf)
      .call();
}
char *realpath(const char *path, char *resolved_path) {
  return LogTheFunc<char *>(
             [&]() {
               FS_CAPI_SETUP(errnoSetter, err, __accmut_libc_realpath, path,
                             resolved_path);
               OpenFileTable::hold();
               auto oft = OpenFileTable::getInstance();
               char *ret = oft->realpath(path, resolved_path, err);
               if (err.value()) {
                 errnoSetter.set(err.value());
               }
               if (MUTATION_ID == 0) {
                 errno = 0;
                 char buf[MAXPATHLEN];
                 auto *oriRet = __accmut_libc_realpath(path, buf);
                 check_eq(errno, err.value());
                 if (ret) {
                   check_streq(oriRet, ret);
                 } else {
                   check_eq(oriRet, (char *)nullptr);
                 }
               }
               return ret;
             },
             "realpath", path, resolved_path)
      .call();
}
}
