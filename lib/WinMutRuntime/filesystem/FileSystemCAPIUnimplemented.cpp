// "pthread_create", "pthread_cancel", "pthread_detach", "pthread_equal",
// "pthread_exit", "pthread_join", "pthread_kill", "pthread_once",
// "pthread_self", "pthread_setcancelstate", "pthread_setcanceltype",
// "pthread_testcancel", "pthread_attr_destroy", "pthread_attr_getinheritsched",
// "pthread_attr_getschedparam", "pthread_attr_getschedpolicy",
// "pthread_attr_getscope", "pthread_attr_getstacksize",
// "pthread_attr_getstackaddr", "pthread_attr_getdetachstate",
// "pthread_attr_init", "pthread_attr_setinheritsched",
// "pthread_attr_setschedparam", "pthread_attr_setschedpolicy",
// "pthread_attr_setscope", "pthread_attr_setstacksize",
// "pthread_attr_setstackaddr", "pthread_attr_setdetachstate",
// "pthread_mutexattr_destroy", "pthread_mutexattr_getprioceiling",
// "pthread_mutexattr_getprotocol", "pthread_mutexattr_gettype",
// "pthread_mutexattr_init", "pthread_mutexattr_setprioceiling",
// "pthread_mutexattr_setprotocol", "pthread_mutexattr_settype",
// "pthread_mutex_destroy", "pthread_mutex_init", "pthread_mutex_lock",
// "pthread_mutex_trylock", "pthread_mutex_unlock", "pthread_condattr_destroy",
// "pthread_condattr_init", "pthread_cond_broadcast", "pthread_cond_destroy",
// "pthread_cond_init", "pthread_cond_signal", "pthread_cond_timedwait",
// "pthread_cond_wait", "pthread_rwlock_destroy", "pthread_rwlock_init",
// "pthread_rwlock_rdlock", "pthread_rwlock_tryrdlock",
// "pthread_rwlock_trywrlock", "pthread_rwlock_unlock", "pthread_rwlock_wrlock",
// "pthread_rwlockattr_destroy", "pthread_rwlockattr_getpshared",
// "pthread_rwlockattr_init", "pthread_rwlockattr_setpshared",
// "pthread_key_create", "pthread_key_delete", "pthread_getspecific",
// "pthread_setspecific", "pthread_atfork"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wignored-attributes"
#include <dlfcn.h>
#include <fcntl.h>
#include <llvm/WinMutRuntime/checking/panic.h>
#include <llvm/WinMutRuntime/filesystem/Dir.h>
#include <llvm/WinMutRuntime/filesystem/ErrorCode.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/DirectoryFile.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/filesystem/MirrorFileSystem.h>
#include <llvm/WinMutRuntime/filesystem/OpenFileTable.h>
#include <llvm/WinMutRuntime/init/init.h>
#include <llvm/WinMutRuntime/sysdep/macros.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" {

int pthread_create(pthread_t *a0, const pthread_attr_t *a1, void *(*a2)(void *),
                   void *a3) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_create(a0, a1, a2, a3);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_create(a0, a1, a2, a3);
  }
  panic("pthread_create not implemented");
}
int pthread_cancel(pthread_t a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_cancel(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_cancel(a0);
  }
  panic("pthread_cancel not implemented");
}
int pthread_detach(pthread_t a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_detach(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_detach(a0);
  }
  panic("pthread_detach not implemented");
}
int pthread_equal(pthread_t a0, pthread_t a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_equal(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_equal(a0, a1);
  }
  panic("pthread_equal not implemented");
}
void pthread_exit(void *a0) {
  if (unlikely(!system_disabled())) {
    __accmut_libc_pthread_exit(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    __accmut_libc_pthread_exit(a0);
  }
  panic("pthread_exit not implemented");
}
int pthread_join(pthread_t a0, void **a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_join(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_join(a0, a1);
  }
  panic("pthread_join not implemented");
}
int pthread_kill(pthread_t a0, int a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_kill(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_kill(a0, a1);
  }
  panic("pthread_kill not implemented");
}
/*
int pthread_once(pthread_once_t *a0, void (*a1)(void)) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_once(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_once(a0, a1);
  }
  panic("pthread_once not implemented");
}
*/
pthread_t pthread_self() {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_self();
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_self();
  }
  panic("pthread_self not implemented");
}
int pthread_setcancelstate(int a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_setcancelstate(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_setcancelstate(a0, a1);
  }
  panic("pthread_setcancelstate not implemented");
}
int pthread_setcanceltype(int a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_setcanceltype(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_setcanceltype(a0, a1);
  }
  panic("pthread_setcanceltype not implemented");
}
void pthread_testcancel() {
  if (unlikely(!system_disabled())) {
    __accmut_libc_pthread_testcancel();
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    __accmut_libc_pthread_testcancel();
  }
  panic("pthread_testcancel not implemented");
}
int pthread_attr_destroy(pthread_attr_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_destroy(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_destroy(a0);
  }
  panic("pthread_attr_destroy not implemented");
}
int pthread_attr_getinheritsched(const pthread_attr_t *a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_getinheritsched(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_getinheritsched(a0, a1);
  }
  panic("pthread_attr_getinheritsched not implemented");
}
int pthread_attr_getschedparam(const pthread_attr_t *a0,
                               struct sched_param *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_getschedparam(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_getschedparam(a0, a1);
  }
  panic("pthread_attr_getschedparam not implemented");
}
int pthread_attr_getschedpolicy(const pthread_attr_t *a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_getschedpolicy(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_getschedpolicy(a0, a1);
  }
  panic("pthread_attr_getschedpolicy not implemented");
}
int pthread_attr_getscope(const pthread_attr_t *a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_getscope(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_getscope(a0, a1);
  }
  panic("pthread_attr_getscope not implemented");
}
int pthread_attr_getstacksize(const pthread_attr_t *a0, size_t *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_getstacksize(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_getstacksize(a0, a1);
  }
  panic("pthread_attr_getstacksize not implemented");
}
int pthread_attr_getstackaddr(const pthread_attr_t *a0, void **a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_getstackaddr(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_getstackaddr(a0, a1);
  }
  panic("pthread_attr_getstackaddr not implemented");
}
int pthread_attr_getdetachstate(const pthread_attr_t *a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_getdetachstate(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_getdetachstate(a0, a1);
  }
  panic("pthread_attr_getdetachstate not implemented");
}
int pthread_attr_init(pthread_attr_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_init(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_init(a0);
  }
  panic("pthread_attr_init not implemented");
}
int pthread_attr_setinheritsched(pthread_attr_t *a0, int a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_setinheritsched(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_setinheritsched(a0, a1);
  }
  panic("pthread_attr_setinheritsched not implemented");
}
int pthread_attr_setschedparam(pthread_attr_t *a0,
                               const struct sched_param *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_setschedparam(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_setschedparam(a0, a1);
  }
  panic("pthread_attr_setschedparam not implemented");
}
int pthread_attr_setschedpolicy(pthread_attr_t *a0, int a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_setschedpolicy(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_setschedpolicy(a0, a1);
  }
  panic("pthread_attr_setschedpolicy not implemented");
}
int pthread_attr_setscope(pthread_attr_t *a0, int a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_setscope(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_setscope(a0, a1);
  }
  panic("pthread_attr_setscope not implemented");
}
int pthread_attr_setstacksize(pthread_attr_t *a0, size_t a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_setstacksize(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_setstacksize(a0, a1);
  }
  panic("pthread_attr_setstacksize not implemented");
}
int pthread_attr_setstackaddr(pthread_attr_t *a0, void *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_setstackaddr(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_setstackaddr(a0, a1);
  }
  panic("pthread_attr_setstackaddr not implemented");
}
int pthread_attr_setdetachstate(pthread_attr_t *a0, int a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_attr_setdetachstate(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_attr_setdetachstate(a0, a1);
  }
  panic("pthread_attr_setdetachstate not implemented");
}
int pthread_mutexattr_destroy(pthread_mutexattr_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutexattr_destroy(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutexattr_destroy(a0);
  }
  panic("pthread_mutexattr_destroy not implemented");
}
int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutexattr_getprioceiling(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutexattr_getprioceiling(a0, a1);
  }
  panic("pthread_mutexattr_getprioceiling not implemented");
}
int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutexattr_getprotocol(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutexattr_getprotocol(a0, a1);
  }
  panic("pthread_mutexattr_getprotocol not implemented");
}
int pthread_mutexattr_gettype(const pthread_mutexattr_t *a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutexattr_gettype(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutexattr_gettype(a0, a1);
  }
  panic("pthread_mutexattr_gettype not implemented");
}
int pthread_mutexattr_init(pthread_mutexattr_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutexattr_init(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutexattr_init(a0);
  }
  panic("pthread_mutexattr_init not implemented");
}
int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *a0, int a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutexattr_setprioceiling(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutexattr_setprioceiling(a0, a1);
  }
  panic("pthread_mutexattr_setprioceiling not implemented");
}
int pthread_mutexattr_setprotocol(pthread_mutexattr_t *a0, int a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutexattr_setprotocol(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutexattr_setprotocol(a0, a1);
  }
  panic("pthread_mutexattr_setprotocol not implemented");
}
int pthread_mutexattr_settype(pthread_mutexattr_t *a0, int a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutexattr_settype(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutexattr_settype(a0, a1);
  }
  panic("pthread_mutexattr_settype not implemented");
}
int pthread_mutex_destroy(pthread_mutex_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutex_destroy(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutex_destroy(a0);
  }
  panic("pthread_mutex_destroy not implemented");
}
int pthread_mutex_init(pthread_mutex_t *a0, const pthread_mutexattr_t *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutex_init(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutex_init(a0, a1);
  }
  panic("pthread_mutex_init not implemented");
}
int pthread_mutex_lock(pthread_mutex_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutex_lock(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutex_lock(a0);
  }
  panic("pthread_mutex_lock not implemented");
}
int pthread_mutex_trylock(pthread_mutex_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutex_trylock(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutex_trylock(a0);
  }
  panic("pthread_mutex_trylock not implemented");
}
int pthread_mutex_unlock(pthread_mutex_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_mutex_unlock(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_mutex_unlock(a0);
  }
  panic("pthread_mutex_unlock not implemented");
}
int pthread_condattr_destroy(pthread_condattr_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_condattr_destroy(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_condattr_destroy(a0);
  }
  panic("pthread_condattr_destroy not implemented");
}
int pthread_condattr_init(pthread_condattr_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_condattr_init(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_condattr_init(a0);
  }
  panic("pthread_condattr_init not implemented");
}
int pthread_cond_broadcast(pthread_cond_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_cond_broadcast(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_cond_broadcast(a0);
  }
  panic("pthread_cond_broadcast not implemented");
}
int pthread_cond_destroy(pthread_cond_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_cond_destroy(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_cond_destroy(a0);
  }
  panic("pthread_cond_destroy not implemented");
}
int pthread_cond_init(pthread_cond_t *a0, const pthread_condattr_t *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_cond_init(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_cond_init(a0, a1);
  }
  panic("pthread_cond_init not implemented");
}
int pthread_cond_signal(pthread_cond_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_cond_signal(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_cond_signal(a0);
  }
  panic("pthread_cond_signal not implemented");
}
int pthread_cond_timedwait(pthread_cond_t *a0, pthread_mutex_t *a1,
                           const struct timespec *a2) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_cond_timedwait(a0, a1, a2);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_cond_timedwait(a0, a1, a2);
  }
  panic("pthread_cond_timedwait not implemented");
}
int pthread_cond_wait(pthread_cond_t *a0, pthread_mutex_t *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_cond_wait(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_cond_wait(a0, a1);
  }
  panic("pthread_cond_wait not implemented");
}
int pthread_rwlock_destroy(pthread_rwlock_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlock_destroy(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlock_destroy(a0);
  }
  panic("pthread_rwlock_destroy not implemented");
}
int pthread_rwlock_init(pthread_rwlock_t *a0, const pthread_rwlockattr_t *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlock_init(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlock_init(a0, a1);
  }
  panic("pthread_rwlock_init not implemented");
}
int pthread_rwlock_rdlock(pthread_rwlock_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlock_rdlock(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlock_rdlock(a0);
  }
  panic("pthread_rwlock_rdlock not implemented");
}
int pthread_rwlock_tryrdlock(pthread_rwlock_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlock_tryrdlock(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlock_tryrdlock(a0);
  }
  panic("pthread_rwlock_tryrdlock not implemented");
}
int pthread_rwlock_trywrlock(pthread_rwlock_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlock_trywrlock(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlock_trywrlock(a0);
  }
  panic("pthread_rwlock_trywrlock not implemented");
}
int pthread_rwlock_unlock(pthread_rwlock_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlock_unlock(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlock_unlock(a0);
  }
  panic("pthread_rwlock_unlock not implemented");
}
int pthread_rwlock_wrlock(pthread_rwlock_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlock_wrlock(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlock_wrlock(a0);
  }
  panic("pthread_rwlock_wrlock not implemented");
}
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlockattr_destroy(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlockattr_destroy(a0);
  }
  panic("pthread_rwlockattr_destroy not implemented");
}
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *a0, int *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlockattr_getpshared(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlockattr_getpshared(a0, a1);
  }
  panic("pthread_rwlockattr_getpshared not implemented");
}
int pthread_rwlockattr_init(pthread_rwlockattr_t *a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlockattr_init(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlockattr_init(a0);
  }
  panic("pthread_rwlockattr_init not implemented");
}
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *a0, int a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_rwlockattr_setpshared(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_rwlockattr_setpshared(a0, a1);
  }
  panic("pthread_rwlockattr_setpshared not implemented");
}
int pthread_key_create(pthread_key_t *a0, void (*a1)(void *)) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_key_create(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_key_create(a0, a1);
  }
  panic("pthread_key_create not implemented");
}
int pthread_key_delete(pthread_key_t a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_key_delete(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_key_delete(a0);
  }
  panic("pthread_key_delete not implemented");
}
void *pthread_getspecific(pthread_key_t a0) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_getspecific(a0);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_getspecific(a0);
  }
  panic("pthread_getspecific not implemented");
}
int pthread_setspecific(pthread_key_t a0, const void *a1) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_setspecific(a0, a1);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_setspecific(a0, a1);
  }
  panic("pthread_setspecific not implemented");
}
int pthread_atfork(void (*a0)(void), void (*a1)(void), void (*a2)(void)) {
  if (unlikely(!system_disabled())) {
    return __accmut_libc_pthread_atfork(a0, a1, a2);
  }
  if (unlikely(!system_initialized())) {
    disable_system();
    return __accmut_libc_pthread_atfork(a0, a1, a2);
  }
  panic("pthread_atfork not implemented");
}
}
    #pragma GCC diagnostic pop
    
