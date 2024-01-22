#ifndef LLVM_FILESYSTEM_CAPI_H
#define LLVM_FILESYSTEM_CAPI_H

#ifndef _USE_OWN_STDIO
#include <cstdio>
#endif
#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
// no special data structure needed
int access(const char *pathname, int mode);
int chdir(const char *path);
int chmod(const char *pathname, mode_t mode);
char *getcwd(char *buf, size_t size);
char *getwd(char *buf);
int lstat(const char *pathname, struct stat *buf);
int mkdir(const char *pathname, mode_t mode);
char *mkdtemp(char *templ);
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
int rename(const char *oldpath, const char *newpath);
int rmdir(const char *pathname);
int unlink(const char *pathname);
int remove(const char *pathname);
int stat(const char *pathname, struct stat *buf);

int closedir(DIR *dirp);
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);

// file descriptor
int close(int fd);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int dup3(int oldfd, int newfd, int flags);
int fchdir(int fd);
int fchown(int fd, uid_t owner, gid_t group);
int fcntl(int fd, int cmd, ...);
int fstat(int fd, struct stat *buf);
off_t lseek(int fd, off_t offset, int whence);
int open(const char *pathname, int flags, ...);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

FILE *fdopen(int fd, const char *mode);
FILE *fopen(const char *pathname, const char *mode);
FILE *freopen(const char *__restrict pathname, const char *__restrict mode,
              FILE *stream);
void perror(const char *s);

// at functions
int openat(int dirfd, const char *pathname, int flags, ...);
int mkdirat(int dirfd, const char *path, mode_t mode);
int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group,
             int flags);
int fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags);
ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
int renameat(int olddirfd, const char *oldpath, int newdirfd,
             const char *newpath);
int renameat2(int olddirfd, const char *oldpath, int newdirfd,
              const char *newpath, unsigned int flags);
int unlinkat(int dirfd, const char *pathname, int flags);
DIR *fdopendir(int fd);

int fstatat64(int dirfd, const char *pathname, struct stat64 *statbuf,
              int flags);
int stat64(const char *pathname, struct stat64 *buf);
int fstat64(int fd, struct stat64 *buf);
int lstat64(const char *pathname, struct stat64 *buf);
// unimplemented / need to be banned
pid_t fork(void);
int pipe(int pipefd[2]);
int pipe2(int pipefd[2], int flags);
int socket(int domain, int type, int protocol);

#ifdef __cplusplus
}
#endif

#endif // LLVM_FILESYSTEM_CAPI_H
