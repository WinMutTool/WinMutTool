#include "sys/mman.h"
#include "llvm/WinMutRuntime/logging/Log.h"
#include "llvm/WinMutRuntime/mutations/MutationIDDecl.h"
#include "llvm/WinMutRuntime/signal/ExitCodes.h"
#include "llvm/WinMutRuntime/sysdep/macros.h"
#include "llvm/WinMutRuntime/util/PageProtector.h"
#include <stdlib.h>
#include <sys/time.h>

static struct itimerval ticks[PAGE_SIZE / sizeof(itimerval)]
    __attribute__((aligned(0x1000)));

#define saved_prof_tick (ticks[0])
#define saved_real_tick (ticks[1])

void accmut_set_timer(long usec) {
  long vsec = 0;
  long vusec = 0;
  vsec = usec / 1000000;
  vusec = usec % 1000000;

  const int INTERVAL_SEC = 0;
  const int INTERVAL_USEC = 50;

  struct itimerval prof_tick;
  prof_tick.it_value.tv_sec = vsec;
  prof_tick.it_value.tv_usec = vusec;
  prof_tick.it_interval.tv_sec = INTERVAL_SEC;
  prof_tick.it_interval.tv_usec = INTERVAL_USEC;

  {
    PageProtector p(ticks, 1);
    int r2 = setitimer(ITIMER_PROF, &prof_tick, &saved_prof_tick);
    if (/*r1 < 0 || */ r2 < 0) {
      LOG("Failed to set timer");
      exit(ENV_ERR);
    }
  }
}

void accmut_set_real_timer(long usec) {
  long vsec = 0;
  long vusec = 0;
  vsec = usec / 1000000;
  vusec = usec % 1000000;

  const int INTERVAL_SEC = 0;
  const int INTERVAL_USEC = 50;

  struct itimerval tick;
  tick.it_value.tv_sec = vsec;
  tick.it_value.tv_usec = vusec;
  tick.it_interval.tv_sec = INTERVAL_SEC;
  tick.it_interval.tv_usec = INTERVAL_USEC;

  {
    PageProtector p(ticks, 1);
    int r1 = setitimer(ITIMER_REAL, &tick, &saved_real_tick);
    if (r1 < 0) {
      LOG("Failed to set timer");
      exit(ENV_ERR);
    }
  }
}

void accmut_disable_timer() { accmut_set_timer(0); }

void accmut_disable_real_timer() { accmut_set_real_timer(0); }

void accmut_set_saved_timer() {
  PageProtector p(ticks, 1);
  int r1 = setitimer(ITIMER_REAL, &saved_prof_tick, nullptr);
  int r2 = setitimer(ITIMER_PROF, &saved_real_tick, nullptr);
  if (r1 < 0 || r2 < 0) {
    LOG("Failed to set timer");
    exit(ENV_ERR);
  }
}
