#ifndef LLVM_TIMERS_H
#define LLVM_TIMERS_H

void accmut_set_timer(long usecond);
void accmut_set_real_timer(long usecond);
void accmut_disable_timer();
void accmut_disable_real_timer();
void accmut_set_saved_timer();

#endif // LLVM_TIMERS_H
