#ifndef LLVM_INIT_H
#define LLVM_INIT_H

#ifdef __cplusplus
extern "C" {
#endif
int system_initialized();
int system_disabled();
void initialize_system(int argc, char **argv);
void disable_system();
void initialize_only_timing(int argc, char **argv);
#ifdef __cplusplus
}
#endif

#endif // LLVM_INIT_H
