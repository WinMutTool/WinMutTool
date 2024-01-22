#include "llvm/WinMutRuntime/filesystem/LibCAPI.h"
#include "llvm/WinMutRuntime/logging/LogFilePrefix.h"
#include "llvm/WinMutRuntime/mutations/MutationIDDecl.h"
#include "llvm/WinMutRuntime/signal/AsyncSafeString.h"
#include "llvm/WinMutRuntime/signal/ExitCodes.h"
#include <execinfo.h>
#include <fcntl.h>
#include <llvm/WinMutRuntime/timers/Timers.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void ShowStack() {
  int i;
  void *buffer[1024];
  int n = backtrace(buffer, 1024);
  char **symbols = backtrace_symbols(buffer, n);
  int fd = __accmut_libc_open("eq_class", O_APPEND | O_WRONLY | O_CREAT, 0644);
  char buf[1024];
  snprintf(buf, 1024, "mut: %d\n", MUTATION_ID);
  __accmut_libc_write(fd, buf, strlen_async_safe(buf));
  for (i = 0; i < n; i++) {
    if (__accmut_libc_write(fd, symbols[i], strlen_async_safe(symbols[i])) < 0)
      _exit(ILL_STATE_ERR);
    if (__accmut_libc_write(fd, "\n", 1) < 0)
      _exit(ILL_STATE_ERR);
  }
}

static void accmut_omitdump_handler(int sig) {
  int exitcd = 0;

  switch (sig) {
  case SIGSEGV:
    exitcd = SIGSEGV_ERR;
    break;
  case SIGABRT:
    exitcd = SIGABRT_ERR;
    break;
  case SIGFPE:
    exitcd = SIGFPE_ERR;
    break;
  default:
    break;
  }
  /*
    auto *x = itoa_async_safe(MUTATION_ID, 10);
    __accmut_libc_write(STDERR_FILENO, x, strlen_async_safe(x));
    __accmut_libc_write(STDERR_FILENO, "\n", 1);
    ShowStack();
    */

  _exit(exitcd);
}
static bool called = false;
static void accmut_timeout_handler(int sig) {
  char msg[128] = {0};

  strcat_async_safe(msg, "TIMEOUT: ");
  strcat_async_safe(msg, itoa_async_safe(MUTATION_ID, 10));
  strcat_async_safe(msg, "\n");

  writeToLogFile("timeout", msg);

  if (!called)
    _exit(TIMEOUT_ERR);
}

void accmut_set_timeout_handlers() {
  signal(SIGPROF, accmut_timeout_handler);

  signal(SIGALRM, accmut_timeout_handler);
}

static void sigusr1(int a) {
  accmut_disable_real_timer();
  accmut_disable_timer();
  called = true;
  raise(SIGSTOP);
}

void accmut_set_handlers() {
  accmut_set_timeout_handlers();
  char signals[] = {SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGFPE};
  for (int sig : signals) {
    signal(sig, accmut_omitdump_handler);
  }
  signal(SIGUSR1, sigusr1);
}