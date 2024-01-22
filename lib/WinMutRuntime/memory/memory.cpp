#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/memory/memory.h>
#include <llvm/WinMutRuntime/mutations/MutationIDDecl.h>
#include <llvm/WinMutRuntime/signal/ExitCodes.h>
#include <malloc.h>
#include <new>
#include <unistd.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#define DISABLE_MALLOC_CHECK_OUTPUT

extern "C" {

static volatile bool _memory_func_called = false;
static volatile bool _cpp_memory_func_called = false;

bool memory_func_called() {
  return _memory_func_called || _cpp_memory_func_called;
}

static void *(*old_malloc_hook)(size_t __size, const void *);
static void (*old_free_hook)(void *__ptr, const void *);
static void *(*old_realloc_hook)(void *__ptr, size_t __size, const void *);

static void *my_malloc_hook(size_t size, const void *caller);

static void my_free_hook(void *ptr, const void *caller);

static void *my_realloc_hook(void *__ptr, size_t __size, const void *);

static int disabled = 0;

static void *my_malloc_hook(size_t size, const void *caller) {
#ifdef DISABLE_MALLOC_CHECK_OUTPUT
  if (MUTATION_ID != 0 && !disabled) {
    int nullfd = __accmut_libc_open("/dev/null", O_WRONLY);
    __accmut_libc_dup2(nullfd, STDERR_FILENO);
    __accmut_libc_close(nullfd);
    disabled = 1;
  }
#endif

  _memory_func_called = true;
  void *result;
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;
  __realloc_hook = old_realloc_hook;
  result = malloc(size);
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  old_realloc_hook = __realloc_hook;

  __malloc_hook = my_malloc_hook;
  __free_hook = my_free_hook;
  __realloc_hook = my_realloc_hook;

  _memory_func_called = false;
  return result;
}

static void my_free_hook(void *ptr, const void *caller) {
#ifdef DISABLE_MALLOC_CHECK_OUTPUT
  if (MUTATION_ID != 0 && !disabled) {
    int nullfd = __accmut_libc_open("/dev/null", O_WRONLY);
    __accmut_libc_dup2(nullfd, STDERR_FILENO);
    __accmut_libc_close(nullfd);
    disabled = 1;
  }
#endif
  _memory_func_called = true;
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;
  __realloc_hook = old_realloc_hook;
  free(ptr);
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  old_realloc_hook = __realloc_hook;

  __malloc_hook = my_malloc_hook;
  __free_hook = my_free_hook;
  __realloc_hook = my_realloc_hook;

  _memory_func_called = false;
}

static void *my_realloc_hook(void *ptr, size_t size, const void *caller) {
#ifdef DISABLE_MALLOC_CHECK_OUTPUT
  if (MUTATION_ID != 0 && !disabled) {
    int nullfd = __accmut_libc_open("/dev/null", O_WRONLY);
    __accmut_libc_dup2(nullfd, STDERR_FILENO);
    __accmut_libc_close(nullfd);
    disabled = 1;
  }
#endif
  _memory_func_called = true;
  void *result;
  __malloc_hook = old_malloc_hook;
  __free_hook = old_free_hook;
  __realloc_hook = old_realloc_hook;
  result = realloc(ptr, size);
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  old_realloc_hook = __realloc_hook;

  __malloc_hook = my_malloc_hook;
  __free_hook = my_free_hook;
  __realloc_hook = my_realloc_hook;

  _memory_func_called = false;
  return result;
}

static void *(*orig_malloc_hook)(size_t __size, const void *);
static void (*orig_free_hook)(void *__ptr, const void *);
static void *(*orig_realloc_hook)(void *__ptr, size_t __size, const void *);

void init_memory_hook(void) {
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  old_realloc_hook = __realloc_hook;
  orig_malloc_hook = __malloc_hook;
  orig_free_hook = __free_hook;
  orig_realloc_hook = __realloc_hook;
  __malloc_hook = my_malloc_hook;
  __free_hook = my_free_hook;
  __realloc_hook = my_realloc_hook;
}

void remove_memory_hook(void) {
  __malloc_hook = orig_malloc_hook;
  __free_hook = orig_free_hook;
  __realloc_hook = orig_realloc_hook;
}

void __attribute__((noreturn)) __stack_chk_fail(void) { abort(); }
}

void *operator new(std::size_t sz) {
  _cpp_memory_func_called = true;
  void *ret = malloc(sz);
  _cpp_memory_func_called = false;
  return ret;
}

void *operator new[](std::size_t sz) {
  _cpp_memory_func_called = true;
  void *ret = malloc(sz);
  _cpp_memory_func_called = false;
  return ret;
}

void operator delete(void *ptr) noexcept {
  _cpp_memory_func_called = true;
  free(ptr);
  _cpp_memory_func_called = false;
}

void operator delete[](void *ptr) noexcept {
  _cpp_memory_func_called = true;
  free(ptr);
  _cpp_memory_func_called = false;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
