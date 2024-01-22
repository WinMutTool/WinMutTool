#!/usr/bin/env python3

def ptype(arglst):
    first = True
    for i in range(len(arglst)):
        if first:
            first = False
        else:
            print(', ', end='')
        if (arglst[i].find('{}') != -1):
            print(arglst[i].format(''), end='')
        else:
            print(arglst[i], end='')


def pdef(arglst):
    first = True
    for i in range(len(arglst)):
        if first:
            first = False
        else:
            print(', ', end='')
        if (arglst[i].find('{}') != -1):
            print(arglst[i].format('a' + str(i)), end='')
        else:
            print(arglst[i], 'a' + str(i), end='')


def parg(arglst):
    first = True
    for i in range(len(arglst)):
        if first:
            first = False
        else:
            print(', ', end='')
        print('a' + str(i), end='')


def pret(rettype, name, arglst, prefix='__accmut_libc_ptr_'):
    if rettype == 'void':
        print('    {}{}('.format(prefix, name), end='')
    else:
        print('  return {}{}('.format(prefix, name), end='')
    parg(arglst)
    print(');')


def plibc(rettype, name, arglst):
    print('static {} (*__accmut_libc_ptr_{})('.format(rettype, name), end='')
    ptype(arglst)
    print(') = nullptr;')
    print('{} __accmut_libc_{}('.format(rettype, name), end='')
    pdef(arglst)
    print(') {')
    print('  if (unlikely(__accmut_libc_ptr_{} == nullptr))'.format(name), '{')
    print('    __accmut_libc_ptr_{} = reinterpret_cast<decltype(__accmut_libc_ptr_{})>(dlsym(RTLD_NEXT, "{}"));'.format(
        name, name,
        name))
    print('  }')
    pret(rettype, name, arglst)
    print('}')


def plibcdecl(rettype, name, arglst):
    print('{} __accmut_libc_{}('.format(rettype, name), end='')
    pdef(arglst)
    print(');')


def pfunc(rettype, name, arglst):
    print('{} {}('.format(rettype, name), end='')
    pdef(arglst)
    print(') {')
    print('  if (unlikely(!system_disabled())) {')
    pret(rettype, name, arglst, '__accmut_libc_')
    print('  }')
    print('  if (unlikely(!system_initialized())) {')
    print('    disable_system();')
    pret(rettype, name, arglst, '__accmut_libc_')
    print('  }')
    print('  panic("{} not implemented");'.format(name))
    print('}')


lst = [['int', 'pthread_create',
        'pthread_t *',
        'const pthread_attr_t *',
        'void *(*{})(void *)',
        'void *'],
       ['int', 'pthread_cancel',
        'pthread_t'],
       ['int', 'pthread_detach',
        'pthread_t'],
       ['int', 'pthread_equal',
        'pthread_t',
        'pthread_t'],
       ['void', 'pthread_exit',
        'void *'],
       ['int', 'pthread_join',
        'pthread_t',
        'void **'],
       ['int', 'pthread_kill',
        'pthread_t',
        'int'],
       ['int', 'pthread_once',
        'pthread_once_t *',
        'void (*{})(void)'],
       ['pthread_t', 'pthread_self'],
       ['int', 'pthread_setcancelstate',
        'int',
        'int *'],
       ['int', 'pthread_setcanceltype',
        'int',
        'int *'],
       ['void', 'pthread_testcancel'],
       ['int', 'pthread_attr_destroy',
        'pthread_attr_t *'],
       ['int', 'pthread_attr_getinheritsched',
        'const pthread_attr_t *',
        'int *'],
       ['int', 'pthread_attr_getschedparam',
        'const pthread_attr_t *',
        'struct sched_param *'],
       ['int', 'pthread_attr_getschedpolicy',
        'const pthread_attr_t *',
        'int *'],
       ['int', 'pthread_attr_getscope',
        'const pthread_attr_t *',
        'int *'],
       ['int', 'pthread_attr_getstacksize',
        'const pthread_attr_t *',
        'size_t *'],
       ['int', 'pthread_attr_getstackaddr',
        'const pthread_attr_t *',
        'void **'],
       ['int', 'pthread_attr_getdetachstate',
        'const pthread_attr_t *',
        'int *'],
       ['int', 'pthread_attr_init',
        'pthread_attr_t *'],
       ['int', 'pthread_attr_setinheritsched',
        'pthread_attr_t *',
        'int '],
       ['int', 'pthread_attr_setschedparam',
        'pthread_attr_t *',
        'const struct sched_param *'],
       ['int', 'pthread_attr_setschedpolicy',
        'pthread_attr_t *',
        'int'],
       ['int', 'pthread_attr_setscope',
        'pthread_attr_t *',
        'int'],
       ['int', 'pthread_attr_setstacksize',
        'pthread_attr_t *',
        'size_t'],
       ['int', 'pthread_attr_setstackaddr',
        'pthread_attr_t *',
        'void *'],
       ['int', 'pthread_attr_setdetachstate',
        'pthread_attr_t *',
        'int'],
       ['int', 'pthread_mutexattr_destroy',
        'pthread_mutexattr_t *'],
       ['int', 'pthread_mutexattr_getprioceiling',
        'const pthread_mutexattr_t *',
        'int *'],
       ['int', 'pthread_mutexattr_getprotocol',
        'const pthread_mutexattr_t *',
        'int *'],
       ['int', 'pthread_mutexattr_gettype',
        'const pthread_mutexattr_t *',
        'int *'],
       ['int', 'pthread_mutexattr_init',
        'pthread_mutexattr_t *'],
       ['int', 'pthread_mutexattr_setprioceiling',
        'pthread_mutexattr_t *',
        'int'],
       ['int', 'pthread_mutexattr_setprotocol',
        'pthread_mutexattr_t *',
        'int'],
       ['int', 'pthread_mutexattr_settype',
        'pthread_mutexattr_t *',
        'int'],
       ['int', 'pthread_mutex_destroy',
        'pthread_mutex_t *'],
       ['int', 'pthread_mutex_init',
        'pthread_mutex_t *',
        'const pthread_mutexattr_t *'],
       ['int', 'pthread_mutex_lock',
        'pthread_mutex_t *'],
       ['int', 'pthread_mutex_trylock',
        'pthread_mutex_t *'],
       ['int', 'pthread_mutex_unlock',
        'pthread_mutex_t *'],
       ['int', 'pthread_condattr_destroy',
        'pthread_condattr_t *'],
       ['int', 'pthread_condattr_init',
        'pthread_condattr_t *'],
       ['int', 'pthread_cond_broadcast',
        'pthread_cond_t *'],
       ['int', 'pthread_cond_destroy',
        'pthread_cond_t *'],
       ['int', 'pthread_cond_init',
        'pthread_cond_t *',
        'const pthread_condattr_t *'],
       ['int', 'pthread_cond_signal',
        'pthread_cond_t *'],
       ['int', 'pthread_cond_timedwait',
        'pthread_cond_t *',
        'pthread_mutex_t *',
        'const struct timespec *'],
       ['int', 'pthread_cond_wait',
        'pthread_cond_t *',
        'pthread_mutex_t *'],
       ['int', 'pthread_rwlock_destroy',
        'pthread_rwlock_t *'],
       ['int', 'pthread_rwlock_init',
        'pthread_rwlock_t *',
        'const pthread_rwlockattr_t *'],
       ['int', 'pthread_rwlock_rdlock',
        'pthread_rwlock_t *'],
       ['int', 'pthread_rwlock_tryrdlock',
        'pthread_rwlock_t *'],
       ['int', 'pthread_rwlock_trywrlock',
        'pthread_rwlock_t *'],
       ['int', 'pthread_rwlock_unlock',
        'pthread_rwlock_t *'],
       ['int', 'pthread_rwlock_wrlock',
        'pthread_rwlock_t *'],
       ['int', 'pthread_rwlockattr_destroy',
        'pthread_rwlockattr_t *'],
       ['int', 'pthread_rwlockattr_getpshared',
        'const pthread_rwlockattr_t *',
        'int *'],
       ['int', 'pthread_rwlockattr_init',
        'pthread_rwlockattr_t *'],
       ['int', 'pthread_rwlockattr_setpshared',
        'pthread_rwlockattr_t *',
        'int'],
       ['int', 'pthread_key_create',
        'pthread_key_t *',
        'void (*{})(void *)'],
       ['int', 'pthread_key_delete',
        'pthread_key_t'],
       ['void *', 'pthread_getspecific',
        'pthread_key_t'],
       ['int', 'pthread_setspecific',
        'pthread_key_t',
        'const void *'],
       ['int', 'pthread_atfork',
        'void (*{})(void)',
        'void (*{})(void)',
        'void (*{})(void)'],
       # ['void', 'pthread_cleanup_pop',
       #  'int'], # buggy in stdlib?
       # ['void', 'pthread_cleanup_push',
       #  'void (*{})(void *)',
       #  'void *'] # buggy in stdlib?
       ]

fslst = [['int', 'access', 'const char *', 'int'],
         ['int', 'chdir', 'const char *'],
         ['int', 'chmod', 'const char *', 'mode_t'],
         ['char *', 'getcwd', 'char *', 'size_t'],
         ['char *', 'getwd', 'char *'],
         ['int', 'lstat', 'const char *', 'struct stat *'],
         ['int', 'mkdir', 'const char *', 'mode_t'],
         ['char *', 'mkdtemp', 'char *'],
         ['ssize_t', 'readlink', 'const char *', 'char *', 'size_t'],
         ['int', 'rename', 'const char *', 'const char *'],
         ['int', 'rmdir', 'const char *'],
         ['int', 'unlink', 'const char *'],
         ['int', 'remove', 'const char *'],
         ['int', 'stat', 'const char *', 'struct stat *'],
         ['int', 'closedir', 'DIR *'],
         ['DIR *', 'opendir', 'const char *'],
         ['struct dirent *', 'readdir', 'DIR *'],
         ['int', 'close', 'int'],
         ['int', 'dup', 'int'],
         ['int', 'dup2', 'int', 'int'],
         ['int', 'dup3', 'int', 'int', 'int'],
         ['int', 'fchdir', 'int'],
         ['int', 'fchown', 'int', 'uid_t', 'gid_t'],
         ['int', 'fstat', 'int', 'struct stat *'],
         ['off_t', 'lseek', 'int', 'off_t', 'int'],
         ['ssize_t', 'read', 'int', 'void *', 'size_t'],
         ['ssize_t', 'write', 'int', 'const void *', 'size_t'],
         ['FILE *', 'fdopen', 'int', 'const char *'],
         ['FILE *', 'fopen', 'const char *', 'const char *'],
         ['FILE *', 'freopen', 'const char *__restrict', 'const char *__restrict', 'FILE *'],
         ['void', 'perror', 'const char *'],
         ['int', 'mkdirat', 'int', 'const char *', 'mode_t'],
         ['int', 'fchmodat', 'int', 'const char *', 'mode_t', 'int'],
         ['int', 'fchownat', 'int', 'const char *', 'uid_t', 'gid_t', 'int'],
         ['int', 'fstatat', 'int', 'const char *', 'struct stat *', 'int'],
         ['int', 'readlinkat', 'int', 'const char *', 'char *', 'size_t'],
         ['int', 'renameat', 'int', 'const char *', 'int', 'const char *'],
         ['int', 'renameat2', 'int', 'const char *', 'int', 'const char *', 'unsigned int'],
         ['int', 'unlinkat', 'int', 'const char *', 'int'],
         ['DIR *', 'fdopendir', 'int'],
         ['pid_t', 'fork'],
         ['int', 'pipe', 'int {}[2]'],
         ['int', 'pipe2', 'int {}[2]', 'int'],
         ['int', 'socket', 'int', 'int', 'int']]
print('//', end=' ')
first = True
for l in map(lambda x: x[1], lst):
    if first:
        first = False
    else:
        print(', ', end='')
    print('"{}"'.format(l), end='')

print(
    '''
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wignored-attributes"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <llvm/WinMutRuntime/checking/panic.h>
#include <llvm/WinMutRuntime/filesystem/Dir.h>
#include <llvm/WinMutRuntime/filesystem/ErrorCode.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/DirectoryFile.h>
#include <llvm/WinMutRuntime/filesystem/MirrorFileSystem.h>
#include <llvm/WinMutRuntime/filesystem/OpenFileTable.h>
#include <llvm/WinMutRuntime/init/init.h>
#include <llvm/WinMutRuntime/sysdep/macros.h>
#include <dlfcn.h>

extern "C" {
'''
)
for l in lst:
    plibc(l[0], l[1], l[2:])
    pfunc(l[0], l[1], l[2:])
print(
    '''
    }
    #pragma GCC diagnostic pop
    '''
)
