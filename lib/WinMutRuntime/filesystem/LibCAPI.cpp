#include <dlfcn.h>
#include <fcntl.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/sysdep/macros.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/select.h>
#include <sys/stat.h>
static char *(*__accmut_libc_ptr_realpath)(const char *, char *);
char *__accmut_libc_realpath(const char *a0, char *a1) {
  if (unlikely(__accmut_libc_ptr_realpath == nullptr)) {
    __accmut_libc_ptr_realpath =
        reinterpret_cast<decltype(__accmut_libc_ptr_realpath)>(
            dlsym(RTLD_NEXT, "realpath"));
  }
  return __accmut_libc_ptr_realpath(a0, a1);
}
static int (*__accmut_libc_ptr_select)(int, fd_set *, fd_set *, fd_set *,
                                       struct timeval *);
int __accmut_libc_select(int a0, fd_set *a1, fd_set *a2, fd_set *a3,
                         struct timeval *a4) {
  if (unlikely(__accmut_libc_ptr_select == nullptr)) {
    __accmut_libc_ptr_select =
        reinterpret_cast<decltype(__accmut_libc_ptr_select)>(
            dlsym(RTLD_NEXT, "select"));
  }
  return __accmut_libc_ptr_select(a0, a1, a2, a3, a4);
}
static int (*__accmut_libc_ptr_fcntl)(int, int, ...);
int __accmut_libc_fcntl(int fd, int cmd, ...) {
  if (unlikely(__accmut_libc_ptr_fcntl == nullptr)) {
    __accmut_libc_ptr_fcntl =
        reinterpret_cast<decltype(__accmut_libc_ptr_fcntl)>(
            dlsym(RTLD_NEXT, "fcntl"));
  }
  int var = 0;
  if (cmd == F_DUPFD_CLOEXEC || cmd == F_DUPFD || cmd == F_SETFD ||
      cmd == F_SETFL) {
    va_list ap;
    va_start(ap, cmd);
    var = va_arg(ap, int);
    va_end(ap);
  }
  return __accmut_libc_ptr_fcntl(fd, cmd, var);
}
static int (*__accmut_libc_ptr_open)(const char *, int, ...);
int __accmut_libc_open(const char *pathname, int flags, ...) {
  if (unlikely(__accmut_libc_ptr_open == nullptr)) {
    __accmut_libc_ptr_open = reinterpret_cast<decltype(__accmut_libc_ptr_open)>(
        dlsym(RTLD_NEXT, "open"));
  }
  int mode = 0;
  if (flags & O_CREAT) {
    va_list lst;
    va_start(lst, flags);
    mode = va_arg(lst, int);
    va_end(lst);
  }
  return __accmut_libc_ptr_open(pathname, flags, mode);
}
static int (*__accmut_libc_ptr_openat)(int, const char *, int, ...);
int __accmut_libc_openat(int dirfd, const char *pathname, int flags, ...) {
  if (unlikely(__accmut_libc_ptr_openat == nullptr)) {
    __accmut_libc_ptr_openat =
        reinterpret_cast<decltype(__accmut_libc_ptr_openat)>(
            dlsym(RTLD_NEXT, "openat"));
  }
  int mode = 0;
  if (flags & O_CREAT) {
    va_list lst;
    va_start(lst, flags);
    mode = va_arg(lst, int);
    va_end(lst);
  }
  return __accmut_libc_ptr_openat(dirfd, pathname, flags, mode);
}
static int (*__accmut_libc_ptr_access)(const char *, int) = nullptr;
int __accmut_libc_access(const char *a0, int a1) {
  if (unlikely(__accmut_libc_ptr_access == nullptr)) {
    __accmut_libc_ptr_access =
        reinterpret_cast<decltype(__accmut_libc_ptr_access)>(
            dlsym(RTLD_NEXT, "access"));
  }
  return __accmut_libc_ptr_access(a0, a1);
}
static int (*__accmut_libc_ptr_chdir)(const char *) = nullptr;
int __accmut_libc_chdir(const char *a0) {
  if (unlikely(__accmut_libc_ptr_chdir == nullptr)) {
    __accmut_libc_ptr_chdir =
        reinterpret_cast<decltype(__accmut_libc_ptr_chdir)>(
            dlsym(RTLD_NEXT, "chdir"));
  }
  return __accmut_libc_ptr_chdir(a0);
}
static int (*__accmut_libc_ptr_chmod)(const char *, mode_t) = nullptr;
int __accmut_libc_chmod(const char *a0, mode_t a1) {
  if (unlikely(__accmut_libc_ptr_chmod == nullptr)) {
    __accmut_libc_ptr_chmod =
        reinterpret_cast<decltype(__accmut_libc_ptr_chmod)>(
            dlsym(RTLD_NEXT, "chmod"));
  }
  return __accmut_libc_ptr_chmod(a0, a1);
}
static int (*__accmut_libc_ptr_fchmod)(int, mode_t) = nullptr;
int __accmut_libc_fchmod(int a0, mode_t a1) {
  if (unlikely(__accmut_libc_ptr_fchmod == nullptr)) {
    __accmut_libc_ptr_fchmod =
        reinterpret_cast<decltype(__accmut_libc_ptr_fchmod)>(
            dlsym(RTLD_NEXT, "fchmod"));
  }
  return __accmut_libc_ptr_fchmod(a0, a1);
}
static char *(*__accmut_libc_ptr_getcwd)(char *, size_t) = nullptr;
char *__accmut_libc_getcwd(char *a0, size_t a1) {
  if (unlikely(__accmut_libc_ptr_getcwd == nullptr)) {
    __accmut_libc_ptr_getcwd =
        reinterpret_cast<decltype(__accmut_libc_ptr_getcwd)>(
            dlsym(RTLD_NEXT, "getcwd"));
  }
  return __accmut_libc_ptr_getcwd(a0, a1);
}
static char *(*__accmut_libc_ptr_getwd)(char *) = nullptr;
char *__accmut_libc_getwd(char *a0) {
  if (unlikely(__accmut_libc_ptr_getwd == nullptr)) {
    __accmut_libc_ptr_getwd =
        reinterpret_cast<decltype(__accmut_libc_ptr_getwd)>(
            dlsym(RTLD_NEXT, "getwd"));
  }
  return __accmut_libc_ptr_getwd(a0);
}
static int (*__accmut_libc_ptr_lstat)(int, const char *,
                                      struct stat *) = nullptr;
int __accmut_libc_lstat(const char *a0, struct stat *a1) {
  if (unlikely(__accmut_libc_ptr_lstat == nullptr)) {
    __accmut_libc_ptr_lstat =
        reinterpret_cast<decltype(__accmut_libc_ptr_lstat)>(
            dlsym(RTLD_NEXT, "__lxstat"));
  }
  return __accmut_libc_ptr_lstat(1, a0, a1);
}
static int (*__accmut_libc_ptr_mkdir)(const char *, mode_t) = nullptr;
int __accmut_libc_mkdir(const char *a0, mode_t a1) {
  if (unlikely(__accmut_libc_ptr_mkdir == nullptr)) {
    __accmut_libc_ptr_mkdir =
        reinterpret_cast<decltype(__accmut_libc_ptr_mkdir)>(
            dlsym(RTLD_NEXT, "mkdir"));
  }
  return __accmut_libc_ptr_mkdir(a0, a1);
}
static char *(*__accmut_libc_ptr_mkdtemp)(char *) = nullptr;
char *__accmut_libc_mkdtemp(char *a0) {
  if (unlikely(__accmut_libc_ptr_mkdtemp == nullptr)) {
    __accmut_libc_ptr_mkdtemp =
        reinterpret_cast<decltype(__accmut_libc_ptr_mkdtemp)>(
            dlsym(RTLD_NEXT, "mkdtemp"));
  }
  return __accmut_libc_ptr_mkdtemp(a0);
}
static char *(*__accmut_libc_ptr_mktemp)(char *) = nullptr;
char *__accmut_libc_mktemp(char *a0) {
  if (unlikely(__accmut_libc_ptr_mktemp == nullptr)) {
    __accmut_libc_ptr_mktemp =
        reinterpret_cast<decltype(__accmut_libc_ptr_mktemp)>(
            dlsym(RTLD_NEXT, "mktemp"));
  }
  return __accmut_libc_ptr_mktemp(a0);
}
static int (*__accmut_libc_ptr_mkstemp)(char *) = nullptr;
int __accmut_libc_mkstemp(char *a0) {
  if (unlikely(__accmut_libc_ptr_mkstemp == nullptr)) {
    __accmut_libc_ptr_mkstemp =
        reinterpret_cast<decltype(__accmut_libc_ptr_mkstemp)>(
            dlsym(RTLD_NEXT, "mkstemp"));
  }
  return __accmut_libc_ptr_mkstemp(a0);
}
static int (*__accmut_libc_ptr_mkostemp)(char *, int oflags) = nullptr;
int __accmut_libc_mkostemp(char *a0, int a1) {
  if (unlikely(__accmut_libc_ptr_mkostemp == nullptr)) {
    __accmut_libc_ptr_mkostemp =
        reinterpret_cast<decltype(__accmut_libc_ptr_mkostemp)>(
            dlsym(RTLD_NEXT, "mkostemp"));
  }
  return __accmut_libc_ptr_mkostemp(a0, a1);
}
static int (*__accmut_libc_ptr_mkstemps)(char *, int slen) = nullptr;
int __accmut_libc_mkstemps(char *a0, int a1) {
  if (unlikely(__accmut_libc_ptr_mkstemps == nullptr)) {
    __accmut_libc_ptr_mkstemps =
        reinterpret_cast<decltype(__accmut_libc_ptr_mkstemps)>(
            dlsym(RTLD_NEXT, "mkstemps"));
  }
  return __accmut_libc_ptr_mkstemps(a0, a1);
}
static int (*__accmut_libc_ptr_mkostemps)(char *, int slen,
                                          int oflags) = nullptr;
int __accmut_libc_mkostemps(char *a0, int a1, int a2) {
  if (unlikely(__accmut_libc_ptr_mkostemps == nullptr)) {
    __accmut_libc_ptr_mkostemps =
        reinterpret_cast<decltype(__accmut_libc_ptr_mkostemps)>(
            dlsym(RTLD_NEXT, "mkostemps"));
  }
  return __accmut_libc_ptr_mkostemps(a0, a1, a2);
}
static FILE *(*__accmut_libc_ptr_tmpfile)() = nullptr;
FILE *__accmut_libc_tmpfile() {
  if (unlikely(__accmut_libc_ptr_tmpfile == nullptr)) {
    __accmut_libc_ptr_tmpfile =
        reinterpret_cast<decltype(__accmut_libc_ptr_tmpfile)>(
            dlsym(RTLD_NEXT, "tmpfile"));
  }
  return __accmut_libc_ptr_tmpfile();
}
static ssize_t (*__accmut_libc_ptr_readlink)(const char *, char *,
                                             size_t) = nullptr;
ssize_t __accmut_libc_readlink(const char *a0, char *a1, size_t a2) {
  if (unlikely(__accmut_libc_ptr_readlink == nullptr)) {
    __accmut_libc_ptr_readlink =
        reinterpret_cast<decltype(__accmut_libc_ptr_readlink)>(
            dlsym(RTLD_NEXT, "readlink"));
  }
  return __accmut_libc_ptr_readlink(a0, a1, a2);
}
static int (*__accmut_libc_ptr_rename)(const char *, const char *) = nullptr;
int __accmut_libc_rename(const char *a0, const char *a1) {
  if (unlikely(__accmut_libc_ptr_rename == nullptr)) {
    __accmut_libc_ptr_rename =
        reinterpret_cast<decltype(__accmut_libc_ptr_rename)>(
            dlsym(RTLD_NEXT, "rename"));
  }
  return __accmut_libc_ptr_rename(a0, a1);
}
static int (*__accmut_libc_ptr_rmdir)(const char *) = nullptr;
int __accmut_libc_rmdir(const char *a0) {
  if (unlikely(__accmut_libc_ptr_rmdir == nullptr)) {
    __accmut_libc_ptr_rmdir =
        reinterpret_cast<decltype(__accmut_libc_ptr_rmdir)>(
            dlsym(RTLD_NEXT, "rmdir"));
  }
  return __accmut_libc_ptr_rmdir(a0);
}
static int (*__accmut_libc_ptr_unlink)(const char *) = nullptr;
int __accmut_libc_unlink(const char *a0) {
  if (unlikely(__accmut_libc_ptr_unlink == nullptr)) {
    __accmut_libc_ptr_unlink =
        reinterpret_cast<decltype(__accmut_libc_ptr_unlink)>(
            dlsym(RTLD_NEXT, "unlink"));
  }
  return __accmut_libc_ptr_unlink(a0);
}
static int (*__accmut_libc_ptr_remove)(const char *) = nullptr;
int __accmut_libc_remove(const char *a0) {
  if (unlikely(__accmut_libc_ptr_remove == nullptr)) {
    __accmut_libc_ptr_remove =
        reinterpret_cast<decltype(__accmut_libc_ptr_remove)>(
            dlsym(RTLD_NEXT, "remove"));
  }
  return __accmut_libc_ptr_remove(a0);
}
static int (*__accmut_libc_ptr_stat)(int, const char *,
                                     struct stat *) = nullptr;
int __accmut_libc_stat(const char *a0, struct stat *a1) {
  if (unlikely(__accmut_libc_ptr_stat == nullptr)) {
    __accmut_libc_ptr_stat = reinterpret_cast<decltype(__accmut_libc_ptr_stat)>(
        dlsym(RTLD_NEXT, "__xstat"));
  }
  return __accmut_libc_ptr_stat(1, a0, a1);
}
static int (*__accmut_libc_ptr_closedir)(DIR *) = nullptr;
int __accmut_libc_closedir(DIR *a0) {
  if (unlikely(__accmut_libc_ptr_closedir == nullptr)) {
    __accmut_libc_ptr_closedir =
        reinterpret_cast<decltype(__accmut_libc_ptr_closedir)>(
            dlsym(RTLD_NEXT, "closedir"));
  }
  return __accmut_libc_ptr_closedir(a0);
}
static DIR *(*__accmut_libc_ptr_opendir)(const char *) = nullptr;
DIR *__accmut_libc_opendir(const char *a0) {
  if (unlikely(__accmut_libc_ptr_opendir == nullptr)) {
    __accmut_libc_ptr_opendir =
        reinterpret_cast<decltype(__accmut_libc_ptr_opendir)>(
            dlsym(RTLD_NEXT, "opendir"));
  }
  return __accmut_libc_ptr_opendir(a0);
}
static struct dirent *(*__accmut_libc_ptr_readdir)(DIR *) = nullptr;
struct dirent *__accmut_libc_readdir(DIR *a0) {
  if (unlikely(__accmut_libc_ptr_readdir == nullptr)) {
    __accmut_libc_ptr_readdir =
        reinterpret_cast<decltype(__accmut_libc_ptr_readdir)>(
            dlsym(RTLD_NEXT, "readdir"));
  }
  return __accmut_libc_ptr_readdir(a0);
}
static int (*__accmut_libc_ptr_close)(int) = nullptr;
int __accmut_libc_close(int a0) {
  if (unlikely(__accmut_libc_ptr_close == nullptr)) {
    __accmut_libc_ptr_close =
        reinterpret_cast<decltype(__accmut_libc_ptr_close)>(
            dlsym(RTLD_NEXT, "close"));
  }
  return __accmut_libc_ptr_close(a0);
}
static int (*__accmut_libc_ptr_dup)(int) = nullptr;
int __accmut_libc_dup(int a0) {
  if (unlikely(__accmut_libc_ptr_dup == nullptr)) {
    __accmut_libc_ptr_dup = reinterpret_cast<decltype(__accmut_libc_ptr_dup)>(
        dlsym(RTLD_NEXT, "dup"));
  }
  return __accmut_libc_ptr_dup(a0);
}
static int (*__accmut_libc_ptr_dup2)(int, int) = nullptr;
int __accmut_libc_dup2(int a0, int a1) {
  if (unlikely(__accmut_libc_ptr_dup2 == nullptr)) {
    __accmut_libc_ptr_dup2 = reinterpret_cast<decltype(__accmut_libc_ptr_dup2)>(
        dlsym(RTLD_NEXT, "dup2"));
  }
  return __accmut_libc_ptr_dup2(a0, a1);
}
static int (*__accmut_libc_ptr_dup3)(int, int, int) = nullptr;
int __accmut_libc_dup3(int a0, int a1, int a2) {
  if (unlikely(__accmut_libc_ptr_dup3 == nullptr)) {
    __accmut_libc_ptr_dup3 = reinterpret_cast<decltype(__accmut_libc_ptr_dup3)>(
        dlsym(RTLD_NEXT, "dup3"));
  }
  return __accmut_libc_ptr_dup3(a0, a1, a2);
}
static int (*__accmut_libc_ptr_fchdir)(int) = nullptr;
int __accmut_libc_fchdir(int a0) {
  if (unlikely(__accmut_libc_ptr_fchdir == nullptr)) {
    __accmut_libc_ptr_fchdir =
        reinterpret_cast<decltype(__accmut_libc_ptr_fchdir)>(
            dlsym(RTLD_NEXT, "fchdir"));
  }
  return __accmut_libc_ptr_fchdir(a0);
}
static int (*__accmut_libc_ptr_fchown)(int, uid_t, gid_t) = nullptr;
int __accmut_libc_fchown(int a0, uid_t a1, gid_t a2) {
  if (unlikely(__accmut_libc_ptr_fchown == nullptr)) {
    __accmut_libc_ptr_fchown =
        reinterpret_cast<decltype(__accmut_libc_ptr_fchown)>(
            dlsym(RTLD_NEXT, "fchown"));
  }
  return __accmut_libc_ptr_fchown(a0, a1, a2);
}
static int (*__accmut_libc_ptr_fstat)(int, int, struct stat *) = nullptr;
int __accmut_libc_fstat(int a0, struct stat *a1) {
  if (unlikely(__accmut_libc_ptr_fstat == nullptr)) {
    __accmut_libc_ptr_fstat =
        reinterpret_cast<decltype(__accmut_libc_ptr_fstat)>(
            dlsym(RTLD_NEXT, "__fxstat"));
  }
  return __accmut_libc_ptr_fstat(1, a0, a1);
}
static off_t (*__accmut_libc_ptr_lseek)(int, off_t, int) = nullptr;
off_t __accmut_libc_lseek(int a0, off_t a1, int a2) {
  if (unlikely(__accmut_libc_ptr_lseek == nullptr)) {
    __accmut_libc_ptr_lseek =
        reinterpret_cast<decltype(__accmut_libc_ptr_lseek)>(
            dlsym(RTLD_NEXT, "lseek"));
  }
  return __accmut_libc_ptr_lseek(a0, a1, a2);
}
static ssize_t (*__accmut_libc_ptr_read)(int, void *, size_t) = nullptr;
ssize_t __accmut_libc_read(int a0, void *a1, size_t a2) {
  if (unlikely(__accmut_libc_ptr_read == nullptr)) {
    __accmut_libc_ptr_read = reinterpret_cast<decltype(__accmut_libc_ptr_read)>(
        dlsym(RTLD_NEXT, "read"));
  }
  return __accmut_libc_ptr_read(a0, a1, a2);
}
static ssize_t (*__accmut_libc_ptr_write)(int, const void *, size_t) = nullptr;
ssize_t __accmut_libc_write(int a0, const void *a1, size_t a2) {
  if (unlikely(__accmut_libc_ptr_write == nullptr)) {
    __accmut_libc_ptr_write =
        reinterpret_cast<decltype(__accmut_libc_ptr_write)>(
            dlsym(RTLD_NEXT, "write"));
  }
  return __accmut_libc_ptr_write(a0, a1, a2);
}
static FILE *(*__accmut_libc_ptr_fdopen)(int, const char *) = nullptr;
FILE *__accmut_libc_fdopen(int a0, const char *a1) {
  if (unlikely(__accmut_libc_ptr_fdopen == nullptr)) {
    __accmut_libc_ptr_fdopen =
        reinterpret_cast<decltype(__accmut_libc_ptr_fdopen)>(
            dlsym(RTLD_NEXT, "fdopen"));
  }
  return __accmut_libc_ptr_fdopen(a0, a1);
}
static FILE *(*__accmut_libc_ptr_fopen)(const char *, const char *) = nullptr;
FILE *__accmut_libc_fopen(const char *a0, const char *a1) {
  if (unlikely(__accmut_libc_ptr_fopen == nullptr)) {
    __accmut_libc_ptr_fopen =
        reinterpret_cast<decltype(__accmut_libc_ptr_fopen)>(
            dlsym(RTLD_NEXT, "fopen"));
  }
  return __accmut_libc_ptr_fopen(a0, a1);
}
static FILE *(*__accmut_libc_ptr_freopen)(const char *__restrict,
                                          const char *__restrict,
                                          FILE *) = nullptr;
FILE *__accmut_libc_freopen(const char *__restrict a0,
                            const char *__restrict a1, FILE *a2) {
  if (unlikely(__accmut_libc_ptr_freopen == nullptr)) {
    __accmut_libc_ptr_freopen =
        reinterpret_cast<decltype(__accmut_libc_ptr_freopen)>(
            dlsym(RTLD_NEXT, "freopen"));
  }
  return __accmut_libc_ptr_freopen(a0, a1, a2);
}
static void (*__accmut_libc_ptr_perror)(const char *) = nullptr;
void __accmut_libc_perror(const char *a0) {
  if (unlikely(__accmut_libc_ptr_perror == nullptr)) {
    __accmut_libc_ptr_perror =
        reinterpret_cast<decltype(__accmut_libc_ptr_perror)>(
            dlsym(RTLD_NEXT, "perror"));
  }
  __accmut_libc_ptr_perror(a0);
}
static int (*__accmut_libc_ptr_mkdirat)(int, const char *, mode_t) = nullptr;
int __accmut_libc_mkdirat(int a0, const char *a1, mode_t a2) {
  if (unlikely(__accmut_libc_ptr_mkdirat == nullptr)) {
    __accmut_libc_ptr_mkdirat =
        reinterpret_cast<decltype(__accmut_libc_ptr_mkdirat)>(
            dlsym(RTLD_NEXT, "mkdirat"));
  }
  return __accmut_libc_ptr_mkdirat(a0, a1, a2);
}
static int (*__accmut_libc_ptr_fchmodat)(int, const char *, mode_t,
                                         int) = nullptr;
int __accmut_libc_fchmodat(int a0, const char *a1, mode_t a2, int a3) {
  if (unlikely(__accmut_libc_ptr_fchmodat == nullptr)) {
    __accmut_libc_ptr_fchmodat =
        reinterpret_cast<decltype(__accmut_libc_ptr_fchmodat)>(
            dlsym(RTLD_NEXT, "fchmodat"));
  }
  return __accmut_libc_ptr_fchmodat(a0, a1, a2, a3);
}
static int (*__accmut_libc_ptr_fchownat)(int, const char *, uid_t, gid_t,
                                         int) = nullptr;
int __accmut_libc_fchownat(int a0, const char *a1, uid_t a2, gid_t a3, int a4) {
  if (unlikely(__accmut_libc_ptr_fchownat == nullptr)) {
    __accmut_libc_ptr_fchownat =
        reinterpret_cast<decltype(__accmut_libc_ptr_fchownat)>(
            dlsym(RTLD_NEXT, "fchownat"));
  }
  return __accmut_libc_ptr_fchownat(a0, a1, a2, a3, a4);
}
static int (*__accmut_libc_ptr_fstatat)(int, int, const char *, struct stat *,
                                        int) = nullptr;
int __accmut_libc_fstatat(int a0, const char *a1, struct stat *a2, int a3) {
  if (unlikely(__accmut_libc_ptr_fstatat == nullptr)) {
    __accmut_libc_ptr_fstatat =
        reinterpret_cast<decltype(__accmut_libc_ptr_fstatat)>(
            dlsym(RTLD_NEXT, "__fxstatat"));
  }
  return __accmut_libc_ptr_fstatat(1, a0, a1, a2, a3);
}
static int (*__accmut_libc_ptr_readlinkat)(int, const char *, char *,
                                           size_t) = nullptr;
int __accmut_libc_readlinkat(int a0, const char *a1, char *a2, size_t a3) {
  if (unlikely(__accmut_libc_ptr_readlinkat == nullptr)) {
    __accmut_libc_ptr_readlinkat =
        reinterpret_cast<decltype(__accmut_libc_ptr_readlinkat)>(
            dlsym(RTLD_NEXT, "readlinkat"));
  }
  return __accmut_libc_ptr_readlinkat(a0, a1, a2, a3);
}
static int (*__accmut_libc_ptr_renameat)(int, const char *, int,
                                         const char *) = nullptr;
int __accmut_libc_renameat(int a0, const char *a1, int a2, const char *a3) {
  if (unlikely(__accmut_libc_ptr_renameat == nullptr)) {
    __accmut_libc_ptr_renameat =
        reinterpret_cast<decltype(__accmut_libc_ptr_renameat)>(
            dlsym(RTLD_NEXT, "renameat"));
  }
  return __accmut_libc_ptr_renameat(a0, a1, a2, a3);
}
static int (*__accmut_libc_ptr_renameat2)(int, const char *, int, const char *,
                                          unsigned int) = nullptr;
int __accmut_libc_renameat2(int a0, const char *a1, int a2, const char *a3,
                            unsigned int a4) {
  if (unlikely(__accmut_libc_ptr_renameat2 == nullptr)) {
    __accmut_libc_ptr_renameat2 =
        reinterpret_cast<decltype(__accmut_libc_ptr_renameat2)>(
            dlsym(RTLD_NEXT, "renameat2"));
  }
  return __accmut_libc_ptr_renameat2(a0, a1, a2, a3, a4);
}
static int (*__accmut_libc_ptr_unlinkat)(int, const char *, int) = nullptr;
int __accmut_libc_unlinkat(int a0, const char *a1, int a2) {
  if (unlikely(__accmut_libc_ptr_unlinkat == nullptr)) {
    __accmut_libc_ptr_unlinkat =
        reinterpret_cast<decltype(__accmut_libc_ptr_unlinkat)>(
            dlsym(RTLD_NEXT, "unlinkat"));
  }
  return __accmut_libc_ptr_unlinkat(a0, a1, a2);
}
static DIR *(*__accmut_libc_ptr_fdopendir)(int) = nullptr;
DIR *__accmut_libc_fdopendir(int a0) {
  if (unlikely(__accmut_libc_ptr_fdopendir == nullptr)) {
    __accmut_libc_ptr_fdopendir =
        reinterpret_cast<decltype(__accmut_libc_ptr_fdopendir)>(
            dlsym(RTLD_NEXT, "fdopendir"));
  }
  return __accmut_libc_ptr_fdopendir(a0);
}
static pid_t (*__accmut_libc_ptr_fork)() = nullptr;
pid_t __accmut_libc_fork() {
  if (unlikely(__accmut_libc_ptr_fork == nullptr)) {
    __accmut_libc_ptr_fork = reinterpret_cast<decltype(__accmut_libc_ptr_fork)>(
        dlsym(RTLD_NEXT, "fork"));
  }
  return __accmut_libc_ptr_fork();
}
static int (*__accmut_libc_ptr_pipe)(int[2]) = nullptr;
int __accmut_libc_pipe(int a0[2]) {
  if (unlikely(__accmut_libc_ptr_pipe == nullptr)) {
    __accmut_libc_ptr_pipe = reinterpret_cast<decltype(__accmut_libc_ptr_pipe)>(
        dlsym(RTLD_NEXT, "pipe"));
  }
  return __accmut_libc_ptr_pipe(a0);
}
static int (*__accmut_libc_ptr_pipe2)(int[2], int) = nullptr;
int __accmut_libc_pipe2(int a0[2], int a1) {
  if (unlikely(__accmut_libc_ptr_pipe2 == nullptr)) {
    __accmut_libc_ptr_pipe2 =
        reinterpret_cast<decltype(__accmut_libc_ptr_pipe2)>(
            dlsym(RTLD_NEXT, "pipe2"));
  }
  return __accmut_libc_ptr_pipe2(a0, a1);
}
static int (*__accmut_libc_ptr_socket)(int, int, int) = nullptr;
int __accmut_libc_socket(int a0, int a1, int a2) {
  if (unlikely(__accmut_libc_ptr_socket == nullptr)) {
    __accmut_libc_ptr_socket =
        reinterpret_cast<decltype(__accmut_libc_ptr_socket)>(
            dlsym(RTLD_NEXT, "socket"));
  }
  return __accmut_libc_ptr_socket(a0, a1, a2);
}
static int (*__accmut_libc_ptr_vfork)(void) = nullptr;
vfork_func_ty __accmut_get_libc_vfork_func(void) {
  if (unlikely(__accmut_libc_ptr_vfork == nullptr)) {
    __accmut_libc_ptr_vfork =
        reinterpret_cast<decltype(__accmut_libc_ptr_vfork)>(
            dlsym(RTLD_NEXT, "vfork"));
  }
  return __accmut_libc_ptr_vfork;
}
pid_t __accmut_libc_vfork(void) {
  if (unlikely(__accmut_libc_ptr_vfork == nullptr)) {
    __accmut_libc_ptr_vfork =
        reinterpret_cast<decltype(__accmut_libc_ptr_vfork)>(
            dlsym(RTLD_NEXT, "vfork"));
  }
  return __accmut_libc_ptr_vfork();
}
static int (*__accmut_libc_ptr_execve)(const char *, char *const[],
                                       char *const[]) = nullptr;
int __accmut_libc_execve(const char *path, char *const argv[],
                         char *const envp[]) {
  if (unlikely(__accmut_libc_ptr_execve == nullptr)) {
    __accmut_libc_ptr_execve =
        reinterpret_cast<decltype(__accmut_libc_ptr_execve)>(
            dlsym(RTLD_NEXT, "execve"));
  }
  return __accmut_libc_ptr_execve(path, argv, envp);
}
static int (*__accmut_libc_ptr_execv)(const char *, char *const[]) = nullptr;
int __accmut_libc_execv(const char *path, char *const argv[]) {
  if (unlikely(__accmut_libc_ptr_execv == nullptr)) {
    __accmut_libc_ptr_execv =
        reinterpret_cast<decltype(__accmut_libc_ptr_execv)>(
            dlsym(RTLD_NEXT, "execv"));
  }
  return __accmut_libc_ptr_execv(path, argv);
}
static int (*__accmut_libc_ptr_execvp)(const char *, char *const[]) = nullptr;
int __accmut_libc_execvp(const char *filename, char *const argv[]) {
  if (unlikely(__accmut_libc_ptr_execvp == nullptr)) {
    __accmut_libc_ptr_execvp =
        reinterpret_cast<decltype(__accmut_libc_ptr_execvp)>(
            dlsym(RTLD_NEXT, "execvp"));
  }
  return __accmut_libc_ptr_execvp(filename, argv);
}
static int (*__accmut_libc_ptr_fexecve)(int, char *const[],
                                        char *const[]) = nullptr;
int __accmut_libc_fexecve(int fd, char *const argv[], char *const envp[]) {
  if (unlikely(__accmut_libc_ptr_fexecve == nullptr)) {
    __accmut_libc_ptr_fexecve =
        reinterpret_cast<decltype(__accmut_libc_ptr_fexecve)>(
            dlsym(RTLD_NEXT, "fexecve"));
  }
  return __accmut_libc_ptr_fexecve(fd, argv, envp);
}
static int (*__accmut_libc_ptr_pthread_create)(pthread_t *,
                                               const pthread_attr_t *,
                                               void *(*)(void *),
                                               void *) = nullptr;
int __accmut_libc_pthread_create(pthread_t *a0, const pthread_attr_t *a1,
                                 void *(*a2)(void *), void *a3) {
  if (unlikely(__accmut_libc_ptr_pthread_create == nullptr)) {
    __accmut_libc_ptr_pthread_create =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_create)>(
            dlsym(RTLD_NEXT, "pthread_create"));
  }
  return __accmut_libc_ptr_pthread_create(a0, a1, a2, a3);
}
static int (*__accmut_libc_ptr_pthread_cancel)(pthread_t) = nullptr;
int __accmut_libc_pthread_cancel(pthread_t a0) {
  if (unlikely(__accmut_libc_ptr_pthread_cancel == nullptr)) {
    __accmut_libc_ptr_pthread_cancel =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_cancel)>(
            dlsym(RTLD_NEXT, "pthread_cancel"));
  }
  return __accmut_libc_ptr_pthread_cancel(a0);
}
static int (*__accmut_libc_ptr_pthread_detach)(pthread_t) = nullptr;
int __accmut_libc_pthread_detach(pthread_t a0) {
  if (unlikely(__accmut_libc_ptr_pthread_detach == nullptr)) {
    __accmut_libc_ptr_pthread_detach =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_detach)>(
            dlsym(RTLD_NEXT, "pthread_detach"));
  }
  return __accmut_libc_ptr_pthread_detach(a0);
}
static int (*__accmut_libc_ptr_pthread_equal)(pthread_t, pthread_t) = nullptr;
int __accmut_libc_pthread_equal(pthread_t a0, pthread_t a1) {
  if (unlikely(__accmut_libc_ptr_pthread_equal == nullptr)) {
    __accmut_libc_ptr_pthread_equal =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_equal)>(
            dlsym(RTLD_NEXT, "pthread_equal"));
  }
  return __accmut_libc_ptr_pthread_equal(a0, a1);
}
static void (*__accmut_libc_ptr_pthread_exit)(void *) = nullptr;
void __accmut_libc_pthread_exit(void *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_exit == nullptr)) {
    __accmut_libc_ptr_pthread_exit =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_exit)>(
            dlsym(RTLD_NEXT, "pthread_exit"));
  }
  __accmut_libc_ptr_pthread_exit(a0);
}
static int (*__accmut_libc_ptr_pthread_join)(pthread_t, void **) = nullptr;
int __accmut_libc_pthread_join(pthread_t a0, void **a1) {
  if (unlikely(__accmut_libc_ptr_pthread_join == nullptr)) {
    __accmut_libc_ptr_pthread_join =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_join)>(
            dlsym(RTLD_NEXT, "pthread_join"));
  }
  return __accmut_libc_ptr_pthread_join(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_kill)(pthread_t, int) = nullptr;
int __accmut_libc_pthread_kill(pthread_t a0, int a1) {
  if (unlikely(__accmut_libc_ptr_pthread_kill == nullptr)) {
    __accmut_libc_ptr_pthread_kill =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_kill)>(
            dlsym(RTLD_NEXT, "pthread_kill"));
  }
  return __accmut_libc_ptr_pthread_kill(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_once)(pthread_once_t *,
                                             void (*)(void)) = nullptr;
int __accmut_libc_pthread_once(pthread_once_t *a0, void (*a1)(void)) {
  if (unlikely(__accmut_libc_ptr_pthread_once == nullptr)) {
    __accmut_libc_ptr_pthread_once =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_once)>(
            dlsym(RTLD_NEXT, "pthread_once"));
  }
  return __accmut_libc_ptr_pthread_once(a0, a1);
}
static pthread_t (*__accmut_libc_ptr_pthread_self)() = nullptr;
pthread_t __accmut_libc_pthread_self() {
  if (unlikely(__accmut_libc_ptr_pthread_self == nullptr)) {
    __accmut_libc_ptr_pthread_self =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_self)>(
            dlsym(RTLD_NEXT, "pthread_self"));
  }
  return __accmut_libc_ptr_pthread_self();
}
static int (*__accmut_libc_ptr_pthread_setcancelstate)(int, int *) = nullptr;
int __accmut_libc_pthread_setcancelstate(int a0, int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_setcancelstate == nullptr)) {
    __accmut_libc_ptr_pthread_setcancelstate =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_setcancelstate)>(
            dlsym(RTLD_NEXT, "pthread_setcancelstate"));
  }
  return __accmut_libc_ptr_pthread_setcancelstate(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_setcanceltype)(int, int *) = nullptr;
int __accmut_libc_pthread_setcanceltype(int a0, int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_setcanceltype == nullptr)) {
    __accmut_libc_ptr_pthread_setcanceltype =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_setcanceltype)>(
            dlsym(RTLD_NEXT, "pthread_setcanceltype"));
  }
  return __accmut_libc_ptr_pthread_setcanceltype(a0, a1);
}
static void (*__accmut_libc_ptr_pthread_testcancel)() = nullptr;
void __accmut_libc_pthread_testcancel() {
  if (unlikely(__accmut_libc_ptr_pthread_testcancel == nullptr)) {
    __accmut_libc_ptr_pthread_testcancel =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_testcancel)>(
            dlsym(RTLD_NEXT, "pthread_testcancel"));
  }
  __accmut_libc_ptr_pthread_testcancel();
}
static int (*__accmut_libc_ptr_pthread_attr_destroy)(pthread_attr_t *) =
    nullptr;
int __accmut_libc_pthread_attr_destroy(pthread_attr_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_destroy == nullptr)) {
    __accmut_libc_ptr_pthread_attr_destroy =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_attr_destroy)>(
            dlsym(RTLD_NEXT, "pthread_attr_destroy"));
  }
  return __accmut_libc_ptr_pthread_attr_destroy(a0);
}
static int (*__accmut_libc_ptr_pthread_attr_getinheritsched)(
    const pthread_attr_t *, int *) = nullptr;
int __accmut_libc_pthread_attr_getinheritsched(const pthread_attr_t *a0,
                                               int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_getinheritsched == nullptr)) {
    __accmut_libc_ptr_pthread_attr_getinheritsched = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_attr_getinheritsched)>(
        dlsym(RTLD_NEXT, "pthread_attr_getinheritsched"));
  }
  return __accmut_libc_ptr_pthread_attr_getinheritsched(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_getschedparam)(
    const pthread_attr_t *, struct sched_param *) = nullptr;
int __accmut_libc_pthread_attr_getschedparam(const pthread_attr_t *a0,
                                             struct sched_param *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_getschedparam == nullptr)) {
    __accmut_libc_ptr_pthread_attr_getschedparam = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_attr_getschedparam)>(
        dlsym(RTLD_NEXT, "pthread_attr_getschedparam"));
  }
  return __accmut_libc_ptr_pthread_attr_getschedparam(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_getschedpolicy)(
    const pthread_attr_t *, int *) = nullptr;
int __accmut_libc_pthread_attr_getschedpolicy(const pthread_attr_t *a0,
                                              int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_getschedpolicy == nullptr)) {
    __accmut_libc_ptr_pthread_attr_getschedpolicy = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_attr_getschedpolicy)>(
        dlsym(RTLD_NEXT, "pthread_attr_getschedpolicy"));
  }
  return __accmut_libc_ptr_pthread_attr_getschedpolicy(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_getscope)(const pthread_attr_t *,
                                                      int *) = nullptr;
int __accmut_libc_pthread_attr_getscope(const pthread_attr_t *a0, int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_getscope == nullptr)) {
    __accmut_libc_ptr_pthread_attr_getscope =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_attr_getscope)>(
            dlsym(RTLD_NEXT, "pthread_attr_getscope"));
  }
  return __accmut_libc_ptr_pthread_attr_getscope(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_getstacksize)(
    const pthread_attr_t *, size_t *) = nullptr;
int __accmut_libc_pthread_attr_getstacksize(const pthread_attr_t *a0,
                                            size_t *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_getstacksize == nullptr)) {
    __accmut_libc_ptr_pthread_attr_getstacksize =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_attr_getstacksize)>(
            dlsym(RTLD_NEXT, "pthread_attr_getstacksize"));
  }
  return __accmut_libc_ptr_pthread_attr_getstacksize(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_getstackaddr)(
    const pthread_attr_t *, void **) = nullptr;
int __accmut_libc_pthread_attr_getstackaddr(const pthread_attr_t *a0,
                                            void **a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_getstackaddr == nullptr)) {
    __accmut_libc_ptr_pthread_attr_getstackaddr =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_attr_getstackaddr)>(
            dlsym(RTLD_NEXT, "pthread_attr_getstackaddr"));
  }
  return __accmut_libc_ptr_pthread_attr_getstackaddr(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_getdetachstate)(
    const pthread_attr_t *, int *) = nullptr;
int __accmut_libc_pthread_attr_getdetachstate(const pthread_attr_t *a0,
                                              int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_getdetachstate == nullptr)) {
    __accmut_libc_ptr_pthread_attr_getdetachstate = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_attr_getdetachstate)>(
        dlsym(RTLD_NEXT, "pthread_attr_getdetachstate"));
  }
  return __accmut_libc_ptr_pthread_attr_getdetachstate(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_init)(pthread_attr_t *) = nullptr;
int __accmut_libc_pthread_attr_init(pthread_attr_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_init == nullptr)) {
    __accmut_libc_ptr_pthread_attr_init =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_attr_init)>(
            dlsym(RTLD_NEXT, "pthread_attr_init"));
  }
  return __accmut_libc_ptr_pthread_attr_init(a0);
}
static int (*__accmut_libc_ptr_pthread_attr_setinheritsched)(pthread_attr_t *,
                                                             int) = nullptr;
int __accmut_libc_pthread_attr_setinheritsched(pthread_attr_t *a0, int a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_setinheritsched == nullptr)) {
    __accmut_libc_ptr_pthread_attr_setinheritsched = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_attr_setinheritsched)>(
        dlsym(RTLD_NEXT, "pthread_attr_setinheritsched"));
  }
  return __accmut_libc_ptr_pthread_attr_setinheritsched(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_setschedparam)(
    pthread_attr_t *, const struct sched_param *) = nullptr;
int __accmut_libc_pthread_attr_setschedparam(pthread_attr_t *a0,
                                             const struct sched_param *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_setschedparam == nullptr)) {
    __accmut_libc_ptr_pthread_attr_setschedparam = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_attr_setschedparam)>(
        dlsym(RTLD_NEXT, "pthread_attr_setschedparam"));
  }
  return __accmut_libc_ptr_pthread_attr_setschedparam(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_setschedpolicy)(pthread_attr_t *,
                                                            int) = nullptr;
int __accmut_libc_pthread_attr_setschedpolicy(pthread_attr_t *a0, int a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_setschedpolicy == nullptr)) {
    __accmut_libc_ptr_pthread_attr_setschedpolicy = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_attr_setschedpolicy)>(
        dlsym(RTLD_NEXT, "pthread_attr_setschedpolicy"));
  }
  return __accmut_libc_ptr_pthread_attr_setschedpolicy(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_setscope)(pthread_attr_t *,
                                                      int) = nullptr;
int __accmut_libc_pthread_attr_setscope(pthread_attr_t *a0, int a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_setscope == nullptr)) {
    __accmut_libc_ptr_pthread_attr_setscope =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_attr_setscope)>(
            dlsym(RTLD_NEXT, "pthread_attr_setscope"));
  }
  return __accmut_libc_ptr_pthread_attr_setscope(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_setstacksize)(pthread_attr_t *,
                                                          size_t) = nullptr;
int __accmut_libc_pthread_attr_setstacksize(pthread_attr_t *a0, size_t a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_setstacksize == nullptr)) {
    __accmut_libc_ptr_pthread_attr_setstacksize =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_attr_setstacksize)>(
            dlsym(RTLD_NEXT, "pthread_attr_setstacksize"));
  }
  return __accmut_libc_ptr_pthread_attr_setstacksize(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_setstackaddr)(pthread_attr_t *,
                                                          void *) = nullptr;
int __accmut_libc_pthread_attr_setstackaddr(pthread_attr_t *a0, void *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_setstackaddr == nullptr)) {
    __accmut_libc_ptr_pthread_attr_setstackaddr =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_attr_setstackaddr)>(
            dlsym(RTLD_NEXT, "pthread_attr_setstackaddr"));
  }
  return __accmut_libc_ptr_pthread_attr_setstackaddr(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_attr_setdetachstate)(pthread_attr_t *,
                                                            int) = nullptr;
int __accmut_libc_pthread_attr_setdetachstate(pthread_attr_t *a0, int a1) {
  if (unlikely(__accmut_libc_ptr_pthread_attr_setdetachstate == nullptr)) {
    __accmut_libc_ptr_pthread_attr_setdetachstate = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_attr_setdetachstate)>(
        dlsym(RTLD_NEXT, "pthread_attr_setdetachstate"));
  }
  return __accmut_libc_ptr_pthread_attr_setdetachstate(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_mutexattr_destroy)(
    pthread_mutexattr_t *) = nullptr;
int __accmut_libc_pthread_mutexattr_destroy(pthread_mutexattr_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_mutexattr_destroy == nullptr)) {
    __accmut_libc_ptr_pthread_mutexattr_destroy =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_mutexattr_destroy)>(
            dlsym(RTLD_NEXT, "pthread_mutexattr_destroy"));
  }
  return __accmut_libc_ptr_pthread_mutexattr_destroy(a0);
}
static int (*__accmut_libc_ptr_pthread_mutexattr_getprioceiling)(
    const pthread_mutexattr_t *, int *) = nullptr;
int __accmut_libc_pthread_mutexattr_getprioceiling(
    const pthread_mutexattr_t *a0, int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_mutexattr_getprioceiling == nullptr)) {
    __accmut_libc_ptr_pthread_mutexattr_getprioceiling =
        reinterpret_cast<decltype(
            __accmut_libc_ptr_pthread_mutexattr_getprioceiling)>(
            dlsym(RTLD_NEXT, "pthread_mutexattr_getprioceiling"));
  }
  return __accmut_libc_ptr_pthread_mutexattr_getprioceiling(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_mutexattr_getprotocol)(
    const pthread_mutexattr_t *, int *) = nullptr;
int __accmut_libc_pthread_mutexattr_getprotocol(const pthread_mutexattr_t *a0,
                                                int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_mutexattr_getprotocol == nullptr)) {
    __accmut_libc_ptr_pthread_mutexattr_getprotocol = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_mutexattr_getprotocol)>(
        dlsym(RTLD_NEXT, "pthread_mutexattr_getprotocol"));
  }
  return __accmut_libc_ptr_pthread_mutexattr_getprotocol(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_mutexattr_gettype)(
    const pthread_mutexattr_t *, int *) = nullptr;
int __accmut_libc_pthread_mutexattr_gettype(const pthread_mutexattr_t *a0,
                                            int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_mutexattr_gettype == nullptr)) {
    __accmut_libc_ptr_pthread_mutexattr_gettype =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_mutexattr_gettype)>(
            dlsym(RTLD_NEXT, "pthread_mutexattr_gettype"));
  }
  return __accmut_libc_ptr_pthread_mutexattr_gettype(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_mutexattr_init)(pthread_mutexattr_t *) =
    nullptr;
int __accmut_libc_pthread_mutexattr_init(pthread_mutexattr_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_mutexattr_init == nullptr)) {
    __accmut_libc_ptr_pthread_mutexattr_init =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_mutexattr_init)>(
            dlsym(RTLD_NEXT, "pthread_mutexattr_init"));
  }
  return __accmut_libc_ptr_pthread_mutexattr_init(a0);
}
static int (*__accmut_libc_ptr_pthread_mutexattr_setprioceiling)(
    pthread_mutexattr_t *, int) = nullptr;
int __accmut_libc_pthread_mutexattr_setprioceiling(pthread_mutexattr_t *a0,
                                                   int a1) {
  if (unlikely(__accmut_libc_ptr_pthread_mutexattr_setprioceiling == nullptr)) {
    __accmut_libc_ptr_pthread_mutexattr_setprioceiling =
        reinterpret_cast<decltype(
            __accmut_libc_ptr_pthread_mutexattr_setprioceiling)>(
            dlsym(RTLD_NEXT, "pthread_mutexattr_setprioceiling"));
  }
  return __accmut_libc_ptr_pthread_mutexattr_setprioceiling(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_mutexattr_setprotocol)(
    pthread_mutexattr_t *, int) = nullptr;
int __accmut_libc_pthread_mutexattr_setprotocol(pthread_mutexattr_t *a0,
                                                int a1) {
  if (unlikely(__accmut_libc_ptr_pthread_mutexattr_setprotocol == nullptr)) {
    __accmut_libc_ptr_pthread_mutexattr_setprotocol = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_mutexattr_setprotocol)>(
        dlsym(RTLD_NEXT, "pthread_mutexattr_setprotocol"));
  }
  return __accmut_libc_ptr_pthread_mutexattr_setprotocol(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_mutexattr_settype)(pthread_mutexattr_t *,
                                                          int) = nullptr;
int __accmut_libc_pthread_mutexattr_settype(pthread_mutexattr_t *a0, int a1) {
  if (unlikely(__accmut_libc_ptr_pthread_mutexattr_settype == nullptr)) {
    __accmut_libc_ptr_pthread_mutexattr_settype =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_mutexattr_settype)>(
            dlsym(RTLD_NEXT, "pthread_mutexattr_settype"));
  }
  return __accmut_libc_ptr_pthread_mutexattr_settype(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_mutex_destroy)(pthread_mutex_t *) =
    nullptr;
int __accmut_libc_pthread_mutex_destroy(pthread_mutex_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_mutex_destroy == nullptr)) {
    __accmut_libc_ptr_pthread_mutex_destroy =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_mutex_destroy)>(
            dlsym(RTLD_NEXT, "pthread_mutex_destroy"));
  }
  return __accmut_libc_ptr_pthread_mutex_destroy(a0);
}
static int (*__accmut_libc_ptr_pthread_mutex_init)(
    pthread_mutex_t *, const pthread_mutexattr_t *) = nullptr;
int __accmut_libc_pthread_mutex_init(pthread_mutex_t *a0,
                                     const pthread_mutexattr_t *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_mutex_init == nullptr)) {
    __accmut_libc_ptr_pthread_mutex_init =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_mutex_init)>(
            dlsym(RTLD_NEXT, "pthread_mutex_init"));
  }
  return __accmut_libc_ptr_pthread_mutex_init(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_mutex_lock)(pthread_mutex_t *) = nullptr;
int __accmut_libc_pthread_mutex_lock(pthread_mutex_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_mutex_lock == nullptr)) {
    __accmut_libc_ptr_pthread_mutex_lock =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_mutex_lock)>(
            dlsym(RTLD_NEXT, "pthread_mutex_lock"));
  }
  return __accmut_libc_ptr_pthread_mutex_lock(a0);
}
static int (*__accmut_libc_ptr_pthread_mutex_trylock)(pthread_mutex_t *) =
    nullptr;
int __accmut_libc_pthread_mutex_trylock(pthread_mutex_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_mutex_trylock == nullptr)) {
    __accmut_libc_ptr_pthread_mutex_trylock =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_mutex_trylock)>(
            dlsym(RTLD_NEXT, "pthread_mutex_trylock"));
  }
  return __accmut_libc_ptr_pthread_mutex_trylock(a0);
}
static int (*__accmut_libc_ptr_pthread_mutex_unlock)(pthread_mutex_t *) =
    nullptr;
int __accmut_libc_pthread_mutex_unlock(pthread_mutex_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_mutex_unlock == nullptr)) {
    __accmut_libc_ptr_pthread_mutex_unlock =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_mutex_unlock)>(
            dlsym(RTLD_NEXT, "pthread_mutex_unlock"));
  }
  return __accmut_libc_ptr_pthread_mutex_unlock(a0);
}
static int (*__accmut_libc_ptr_pthread_condattr_destroy)(pthread_condattr_t *) =
    nullptr;
int __accmut_libc_pthread_condattr_destroy(pthread_condattr_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_condattr_destroy == nullptr)) {
    __accmut_libc_ptr_pthread_condattr_destroy =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_condattr_destroy)>(
            dlsym(RTLD_NEXT, "pthread_condattr_destroy"));
  }
  return __accmut_libc_ptr_pthread_condattr_destroy(a0);
}
static int (*__accmut_libc_ptr_pthread_condattr_init)(pthread_condattr_t *) =
    nullptr;
int __accmut_libc_pthread_condattr_init(pthread_condattr_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_condattr_init == nullptr)) {
    __accmut_libc_ptr_pthread_condattr_init =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_condattr_init)>(
            dlsym(RTLD_NEXT, "pthread_condattr_init"));
  }
  return __accmut_libc_ptr_pthread_condattr_init(a0);
}
static int (*__accmut_libc_ptr_pthread_cond_broadcast)(pthread_cond_t *) =
    nullptr;
int __accmut_libc_pthread_cond_broadcast(pthread_cond_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_cond_broadcast == nullptr)) {
    __accmut_libc_ptr_pthread_cond_broadcast =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_cond_broadcast)>(
            dlsym(RTLD_NEXT, "pthread_cond_broadcast"));
  }
  return __accmut_libc_ptr_pthread_cond_broadcast(a0);
}
static int (*__accmut_libc_ptr_pthread_cond_destroy)(pthread_cond_t *) =
    nullptr;
int __accmut_libc_pthread_cond_destroy(pthread_cond_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_cond_destroy == nullptr)) {
    __accmut_libc_ptr_pthread_cond_destroy =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_cond_destroy)>(
            dlsym(RTLD_NEXT, "pthread_cond_destroy"));
  }
  return __accmut_libc_ptr_pthread_cond_destroy(a0);
}
static int (*__accmut_libc_ptr_pthread_cond_init)(
    pthread_cond_t *, const pthread_condattr_t *) = nullptr;
int __accmut_libc_pthread_cond_init(pthread_cond_t *a0,
                                    const pthread_condattr_t *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_cond_init == nullptr)) {
    __accmut_libc_ptr_pthread_cond_init =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_cond_init)>(
            dlsym(RTLD_NEXT, "pthread_cond_init"));
  }
  return __accmut_libc_ptr_pthread_cond_init(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_cond_signal)(pthread_cond_t *) = nullptr;
int __accmut_libc_pthread_cond_signal(pthread_cond_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_cond_signal == nullptr)) {
    __accmut_libc_ptr_pthread_cond_signal =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_cond_signal)>(
            dlsym(RTLD_NEXT, "pthread_cond_signal"));
  }
  return __accmut_libc_ptr_pthread_cond_signal(a0);
}
static int (*__accmut_libc_ptr_pthread_cond_timedwait)(
    pthread_cond_t *, pthread_mutex_t *, const struct timespec *) = nullptr;
int __accmut_libc_pthread_cond_timedwait(pthread_cond_t *a0,
                                         pthread_mutex_t *a1,
                                         const struct timespec *a2) {
  if (unlikely(__accmut_libc_ptr_pthread_cond_timedwait == nullptr)) {
    __accmut_libc_ptr_pthread_cond_timedwait =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_cond_timedwait)>(
            dlsym(RTLD_NEXT, "pthread_cond_timedwait"));
  }
  return __accmut_libc_ptr_pthread_cond_timedwait(a0, a1, a2);
}
static int (*__accmut_libc_ptr_pthread_cond_wait)(pthread_cond_t *,
                                                  pthread_mutex_t *) = nullptr;
int __accmut_libc_pthread_cond_wait(pthread_cond_t *a0, pthread_mutex_t *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_cond_wait == nullptr)) {
    __accmut_libc_ptr_pthread_cond_wait =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_cond_wait)>(
            dlsym(RTLD_NEXT, "pthread_cond_wait"));
  }
  return __accmut_libc_ptr_pthread_cond_wait(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_rwlock_destroy)(pthread_rwlock_t *) =
    nullptr;
int __accmut_libc_pthread_rwlock_destroy(pthread_rwlock_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlock_destroy == nullptr)) {
    __accmut_libc_ptr_pthread_rwlock_destroy =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_rwlock_destroy)>(
            dlsym(RTLD_NEXT, "pthread_rwlock_destroy"));
  }
  return __accmut_libc_ptr_pthread_rwlock_destroy(a0);
}
static int (*__accmut_libc_ptr_pthread_rwlock_init)(
    pthread_rwlock_t *, const pthread_rwlockattr_t *) = nullptr;
int __accmut_libc_pthread_rwlock_init(pthread_rwlock_t *a0,
                                      const pthread_rwlockattr_t *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlock_init == nullptr)) {
    __accmut_libc_ptr_pthread_rwlock_init =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_rwlock_init)>(
            dlsym(RTLD_NEXT, "pthread_rwlock_init"));
  }
  return __accmut_libc_ptr_pthread_rwlock_init(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_rwlock_rdlock)(pthread_rwlock_t *) =
    nullptr;
int __accmut_libc_pthread_rwlock_rdlock(pthread_rwlock_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlock_rdlock == nullptr)) {
    __accmut_libc_ptr_pthread_rwlock_rdlock =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_rwlock_rdlock)>(
            dlsym(RTLD_NEXT, "pthread_rwlock_rdlock"));
  }
  return __accmut_libc_ptr_pthread_rwlock_rdlock(a0);
}
static int (*__accmut_libc_ptr_pthread_rwlock_tryrdlock)(pthread_rwlock_t *) =
    nullptr;
int __accmut_libc_pthread_rwlock_tryrdlock(pthread_rwlock_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlock_tryrdlock == nullptr)) {
    __accmut_libc_ptr_pthread_rwlock_tryrdlock =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_rwlock_tryrdlock)>(
            dlsym(RTLD_NEXT, "pthread_rwlock_tryrdlock"));
  }
  return __accmut_libc_ptr_pthread_rwlock_tryrdlock(a0);
}
static int (*__accmut_libc_ptr_pthread_rwlock_trywrlock)(pthread_rwlock_t *) =
    nullptr;
int __accmut_libc_pthread_rwlock_trywrlock(pthread_rwlock_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlock_trywrlock == nullptr)) {
    __accmut_libc_ptr_pthread_rwlock_trywrlock =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_rwlock_trywrlock)>(
            dlsym(RTLD_NEXT, "pthread_rwlock_trywrlock"));
  }
  return __accmut_libc_ptr_pthread_rwlock_trywrlock(a0);
}
static int (*__accmut_libc_ptr_pthread_rwlock_unlock)(pthread_rwlock_t *) =
    nullptr;
int __accmut_libc_pthread_rwlock_unlock(pthread_rwlock_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlock_unlock == nullptr)) {
    __accmut_libc_ptr_pthread_rwlock_unlock =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_rwlock_unlock)>(
            dlsym(RTLD_NEXT, "pthread_rwlock_unlock"));
  }
  return __accmut_libc_ptr_pthread_rwlock_unlock(a0);
}
static int (*__accmut_libc_ptr_pthread_rwlock_wrlock)(pthread_rwlock_t *) =
    nullptr;
int __accmut_libc_pthread_rwlock_wrlock(pthread_rwlock_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlock_wrlock == nullptr)) {
    __accmut_libc_ptr_pthread_rwlock_wrlock =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_rwlock_wrlock)>(
            dlsym(RTLD_NEXT, "pthread_rwlock_wrlock"));
  }
  return __accmut_libc_ptr_pthread_rwlock_wrlock(a0);
}
static int (*__accmut_libc_ptr_pthread_rwlockattr_destroy)(
    pthread_rwlockattr_t *) = nullptr;
int __accmut_libc_pthread_rwlockattr_destroy(pthread_rwlockattr_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlockattr_destroy == nullptr)) {
    __accmut_libc_ptr_pthread_rwlockattr_destroy = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_rwlockattr_destroy)>(
        dlsym(RTLD_NEXT, "pthread_rwlockattr_destroy"));
  }
  return __accmut_libc_ptr_pthread_rwlockattr_destroy(a0);
}
static int (*__accmut_libc_ptr_pthread_rwlockattr_getpshared)(
    const pthread_rwlockattr_t *, int *) = nullptr;
int __accmut_libc_pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *a0,
                                                int *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlockattr_getpshared == nullptr)) {
    __accmut_libc_ptr_pthread_rwlockattr_getpshared = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_rwlockattr_getpshared)>(
        dlsym(RTLD_NEXT, "pthread_rwlockattr_getpshared"));
  }
  return __accmut_libc_ptr_pthread_rwlockattr_getpshared(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_rwlockattr_init)(
    pthread_rwlockattr_t *) = nullptr;
int __accmut_libc_pthread_rwlockattr_init(pthread_rwlockattr_t *a0) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlockattr_init == nullptr)) {
    __accmut_libc_ptr_pthread_rwlockattr_init =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_rwlockattr_init)>(
            dlsym(RTLD_NEXT, "pthread_rwlockattr_init"));
  }
  return __accmut_libc_ptr_pthread_rwlockattr_init(a0);
}
static int (*__accmut_libc_ptr_pthread_rwlockattr_setpshared)(
    pthread_rwlockattr_t *, int) = nullptr;
int __accmut_libc_pthread_rwlockattr_setpshared(pthread_rwlockattr_t *a0,
                                                int a1) {
  if (unlikely(__accmut_libc_ptr_pthread_rwlockattr_setpshared == nullptr)) {
    __accmut_libc_ptr_pthread_rwlockattr_setpshared = reinterpret_cast<decltype(
        __accmut_libc_ptr_pthread_rwlockattr_setpshared)>(
        dlsym(RTLD_NEXT, "pthread_rwlockattr_setpshared"));
  }
  return __accmut_libc_ptr_pthread_rwlockattr_setpshared(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_key_create)(pthread_key_t *,
                                                   void (*)(void *)) = nullptr;
int __accmut_libc_pthread_key_create(pthread_key_t *a0, void (*a1)(void *)) {
  if (unlikely(__accmut_libc_ptr_pthread_key_create == nullptr)) {
    __accmut_libc_ptr_pthread_key_create =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_key_create)>(
            dlsym(RTLD_NEXT, "pthread_key_create"));
  }
  return __accmut_libc_ptr_pthread_key_create(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_key_delete)(pthread_key_t) = nullptr;
int __accmut_libc_pthread_key_delete(pthread_key_t a0) {
  if (unlikely(__accmut_libc_ptr_pthread_key_delete == nullptr)) {
    __accmut_libc_ptr_pthread_key_delete =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_key_delete)>(
            dlsym(RTLD_NEXT, "pthread_key_delete"));
  }
  return __accmut_libc_ptr_pthread_key_delete(a0);
}
static void *(*__accmut_libc_ptr_pthread_getspecific)(pthread_key_t) = nullptr;
void *__accmut_libc_pthread_getspecific(pthread_key_t a0) {
  if (unlikely(__accmut_libc_ptr_pthread_getspecific == nullptr)) {
    __accmut_libc_ptr_pthread_getspecific =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_getspecific)>(
            dlsym(RTLD_NEXT, "pthread_getspecific"));
  }
  return __accmut_libc_ptr_pthread_getspecific(a0);
}
static int (*__accmut_libc_ptr_pthread_setspecific)(pthread_key_t,
                                                    const void *) = nullptr;
int __accmut_libc_pthread_setspecific(pthread_key_t a0, const void *a1) {
  if (unlikely(__accmut_libc_ptr_pthread_setspecific == nullptr)) {
    __accmut_libc_ptr_pthread_setspecific =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_setspecific)>(
            dlsym(RTLD_NEXT, "pthread_setspecific"));
  }
  return __accmut_libc_ptr_pthread_setspecific(a0, a1);
}
static int (*__accmut_libc_ptr_pthread_atfork)(void (*)(void), void (*)(void),
                                               void (*)(void)) = nullptr;
int __accmut_libc_pthread_atfork(void (*a0)(void), void (*a1)(void),
                                 void (*a2)(void)) {
  if (unlikely(__accmut_libc_ptr_pthread_atfork == nullptr)) {
    __accmut_libc_ptr_pthread_atfork =
        reinterpret_cast<decltype(__accmut_libc_ptr_pthread_atfork)>(
            dlsym(RTLD_NEXT, "pthread_atfork"));
  }
  return __accmut_libc_ptr_pthread_atfork(a0, a1, a2);
}
void init_libc_api_to_avoid_dead_loop() {
  if (unlikely(__accmut_libc_ptr_open == nullptr)) {
    __accmut_libc_ptr_open = reinterpret_cast<decltype(__accmut_libc_ptr_open)>(
        dlsym(RTLD_NEXT, "open"));
  }
  if (unlikely(__accmut_libc_ptr_dup2 == nullptr)) {
    __accmut_libc_ptr_dup2 = reinterpret_cast<decltype(__accmut_libc_ptr_dup2)>(
        dlsym(RTLD_NEXT, "dup2"));
  }
  if (unlikely(__accmut_libc_ptr_close == nullptr)) {
    __accmut_libc_ptr_close =
        reinterpret_cast<decltype(__accmut_libc_ptr_close)>(
            dlsym(RTLD_NEXT, "close"));
  }
}
