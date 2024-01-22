#ifndef LLVM_LIBCAPI_H
#define LLVM_LIBCAPI_H
#include <dirent.h>
#include <unistd.h>
#ifndef _USE_OWN_STDIO
#include <stdio.h>
#endif
#include <pthread.h>
#include <sys/stat.h>
#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif
char *__accmut_libc_realpath(const char *a0, char *a1);
int __accmut_libc_select(int nfds, fd_set *readfds, fd_set *writefds,
                         fd_set *errorfds, struct timeval *timeout);
int __accmut_libc_access(const char *pathname, int mode);
int __accmut_libc_chdir(const char *path);
int __accmut_libc_chmod(const char *pathname, mode_t mode);
int __accmut_libc_fchmod(int fd, mode_t mode);
char *__accmut_libc_getcwd(char *buf, size_t size);
char *__accmut_libc_getwd(char *buf);
int __accmut_libc_lstat(const char *pathname, struct stat *buf);
int __accmut_libc_mkdir(const char *pathname, mode_t mode);
char *__accmut_libc_mkdtemp(char *templ);
char *__accmut_libc_mktemp(char *templ);
int __accmut_libc_mkstemp(char *templ);
int __accmut_libc_mkostemp(char *path, int oflags);
int __accmut_libc_mkstemps(char *path, int slen);
int __accmut_libc_mkostemps(char *path, int slen, int oflags);
FILE *__accmut_libc_tmpfile();
ssize_t __accmut_libc_readlink(const char *pathname, char *buf, size_t bufsiz);
int __accmut_libc_rename(const char *oldpath, const char *newpath);
int __accmut_libc_rmdir(const char *pathname);
int __accmut_libc_unlink(const char *pathname);
int __accmut_libc_remove(const char *pathname);
int __accmut_libc_stat(const char *pathname, struct stat *buf);
int __accmut_libc_closedir(DIR *dirp);
DIR *__accmut_libc_opendir(const char *name);
struct dirent *__accmut_libc_readdir(DIR *dirp);
int __accmut_libc_close(int fd);
int __accmut_libc_dup(int oldfd);
int __accmut_libc_dup2(int oldfd, int newfd);
int __accmut_libc_dup3(int oldfd, int newfd, int flags);
int __accmut_libc_fchdir(int fd);
int __accmut_libc_fchown(int fd, uid_t owner, gid_t group);
int __accmut_libc_fcntl(int fd, int cmd, ...);
int __accmut_libc_fstat(int fd, struct stat *buf);
off_t __accmut_libc_lseek(int fd, off_t offset, int whence);
int __accmut_libc_open(const char *pathname, int flags, ...);
ssize_t __accmut_libc_read(int fd, void *buf, size_t count);
ssize_t __accmut_libc_write(int fd, const void *buf, size_t count);

FILE *__accmut_libc_fdopen(int fd, const char *mode);
FILE *__accmut_libc_fopen(const char *pathname, const char *mode);
FILE *__accmut_libc_freopen(const char *__restrict pathname,
                            const char *__restrict mode, FILE *stream);
void __accmut_libc_perror(const char *s);

int __accmut_libc_openat(int dirfd, const char *pathname, int flags, ...);
int __accmut_libc_mkdirat(int dirfd, const char *path, mode_t mode);
int __accmut_libc_fchmodat(int dirfd, const char *pathname, mode_t mode,
                           int flags);
int __accmut_libc_fchownat(int dirfd, const char *pathname, uid_t owner,
                           gid_t group, int flags);
int __accmut_libc_fstatat(int dirfd, const char *pathname, struct stat *statbuf,
                          int flags);
int __accmut_libc_readlinkat(int dirfd, const char *pathname, char *buf,
                             size_t bufsiz);
int __accmut_libc_renameat(int olddirfd, const char *oldpath, int newdirfd,
                           const char *newpath);
int __accmut_libc_renameat2(int olddirfd, const char *oldpath, int newdirfd,
                            const char *newpath, unsigned int flags);
int __accmut_libc_unlinkat(int dirfd, const char *pathname, int flags);
DIR *__accmut_libc_fdopendir(int fd);

// unimplemented / need to be banned
pid_t __accmut_libc_fork(void);
int __accmut_libc_pipe(int pipefd[2]);
int __accmut_libc_pipe2(int pipefd[2], int flags);
int __accmut_libc_socket(int domain, int type, int protocol);

typedef pid_t (*vfork_func_ty)(void);
vfork_func_ty __accmut_get_libc_vfork_func(void);
pid_t __accmut_libc_vfork(void);
pid_t __accmut_libc_vfork(void);
int __accmut_libc_execve(const char *path, char *const argv[],
                         char *const envp[]);
int __accmut_libc_execv(const char *path, char *const argv[]);
int __accmut_libc_execvp(const char *filename, char *const argv[]);
int __accmut_libc_fexecve(int fd, char *const argv[], char *const envp[]);

int __accmut_libc_pthread_create(pthread_t *a0, const pthread_attr_t *a1,
                                 void *(*a2)(void *), void *a3);
int __accmut_libc_pthread_cancel(pthread_t a0);
int __accmut_libc_pthread_detach(pthread_t a0);
int __accmut_libc_pthread_equal(pthread_t a0, pthread_t a1);
void __accmut_libc_pthread_exit(void *a0);
int __accmut_libc_pthread_join(pthread_t a0, void **a1);
int __accmut_libc_pthread_kill(pthread_t a0, int a1);
int __accmut_libc_pthread_once(pthread_once_t *a0, void (*a1)(void));
pthread_t __accmut_libc_pthread_self();
int __accmut_libc_pthread_setcancelstate(int a0, int *a1);
int __accmut_libc_pthread_setcanceltype(int a0, int *a1);
void __accmut_libc_pthread_testcancel();
int __accmut_libc_pthread_attr_destroy(pthread_attr_t *a0);
int __accmut_libc_pthread_attr_getinheritsched(const pthread_attr_t *a0,
                                               int *a1);
int __accmut_libc_pthread_attr_getschedparam(const pthread_attr_t *a0,
                                             struct sched_param *a1);
int __accmut_libc_pthread_attr_getschedpolicy(const pthread_attr_t *a0,
                                              int *a1);
int __accmut_libc_pthread_attr_getscope(const pthread_attr_t *a0, int *a1);
int __accmut_libc_pthread_attr_getstacksize(const pthread_attr_t *a0,
                                            size_t *a1);
int __accmut_libc_pthread_attr_getstackaddr(const pthread_attr_t *a0,
                                            void **a1);
int __accmut_libc_pthread_attr_getdetachstate(const pthread_attr_t *a0,
                                              int *a1);
int __accmut_libc_pthread_attr_init(pthread_attr_t *a0);
int __accmut_libc_pthread_attr_setinheritsched(pthread_attr_t *a0, int a1);
int __accmut_libc_pthread_attr_setschedparam(pthread_attr_t *a0,
                                             const struct sched_param *a1);
int __accmut_libc_pthread_attr_setschedpolicy(pthread_attr_t *a0, int a1);
int __accmut_libc_pthread_attr_setscope(pthread_attr_t *a0, int a1);
int __accmut_libc_pthread_attr_setstacksize(pthread_attr_t *a0, size_t a1);
int __accmut_libc_pthread_attr_setstackaddr(pthread_attr_t *a0, void *a1);
int __accmut_libc_pthread_attr_setdetachstate(pthread_attr_t *a0, int a1);
int __accmut_libc_pthread_mutexattr_destroy(pthread_mutexattr_t *a0);
int __accmut_libc_pthread_mutexattr_getprioceiling(
    const pthread_mutexattr_t *a0, int *a1);
int __accmut_libc_pthread_mutexattr_getprotocol(const pthread_mutexattr_t *a0,
                                                int *a1);
int __accmut_libc_pthread_mutexattr_gettype(const pthread_mutexattr_t *a0,
                                            int *a1);
int __accmut_libc_pthread_mutexattr_init(pthread_mutexattr_t *a0);
int __accmut_libc_pthread_mutexattr_setprioceiling(pthread_mutexattr_t *a0,
                                                   int a1);
int __accmut_libc_pthread_mutexattr_setprotocol(pthread_mutexattr_t *a0,
                                                int a1);
int __accmut_libc_pthread_mutexattr_settype(pthread_mutexattr_t *a0, int a1);
int __accmut_libc_pthread_mutex_destroy(pthread_mutex_t *a0);
int __accmut_libc_pthread_mutex_init(pthread_mutex_t *a0,
                                     const pthread_mutexattr_t *a1);
int __accmut_libc_pthread_mutex_lock(pthread_mutex_t *a0);
int __accmut_libc_pthread_mutex_trylock(pthread_mutex_t *a0);
int __accmut_libc_pthread_mutex_unlock(pthread_mutex_t *a0);
int __accmut_libc_pthread_condattr_destroy(pthread_condattr_t *a0);
int __accmut_libc_pthread_condattr_init(pthread_condattr_t *a0);
int __accmut_libc_pthread_cond_broadcast(pthread_cond_t *a0);
int __accmut_libc_pthread_cond_destroy(pthread_cond_t *a0);
int __accmut_libc_pthread_cond_init(pthread_cond_t *a0,
                                    const pthread_condattr_t *a1);
int __accmut_libc_pthread_cond_signal(pthread_cond_t *a0);
int __accmut_libc_pthread_cond_timedwait(pthread_cond_t *a0,
                                         pthread_mutex_t *a1,
                                         const struct timespec *a2);
int __accmut_libc_pthread_cond_wait(pthread_cond_t *a0, pthread_mutex_t *a1);
int __accmut_libc_pthread_rwlock_destroy(pthread_rwlock_t *a0);
int __accmut_libc_pthread_rwlock_init(pthread_rwlock_t *a0,
                                      const pthread_rwlockattr_t *a1);
int __accmut_libc_pthread_rwlock_rdlock(pthread_rwlock_t *a0);
int __accmut_libc_pthread_rwlock_tryrdlock(pthread_rwlock_t *a0);
int __accmut_libc_pthread_rwlock_trywrlock(pthread_rwlock_t *a0);
int __accmut_libc_pthread_rwlock_unlock(pthread_rwlock_t *a0);
int __accmut_libc_pthread_rwlock_wrlock(pthread_rwlock_t *a0);
int __accmut_libc_pthread_rwlockattr_destroy(pthread_rwlockattr_t *a0);
int __accmut_libc_pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *a0,
                                                int *a1);
int __accmut_libc_pthread_rwlockattr_init(pthread_rwlockattr_t *a0);
int __accmut_libc_pthread_rwlockattr_setpshared(pthread_rwlockattr_t *a0,
                                                int a1);
int __accmut_libc_pthread_key_create(pthread_key_t *a0, void (*a1)(void *));
int __accmut_libc_pthread_key_delete(pthread_key_t a0);
void *__accmut_libc_pthread_getspecific(pthread_key_t a0);
int __accmut_libc_pthread_setspecific(pthread_key_t a0, const void *a1);
int __accmut_libc_pthread_atfork(void (*a0)(void), void (*a1)(void),
                                 void (*a2)(void));
void init_libc_api_to_avoid_dead_loop();

#ifdef __cplusplus
}
#endif

#endif // LLVM_LIBCAPI_H
