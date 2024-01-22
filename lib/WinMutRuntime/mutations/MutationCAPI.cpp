#include <chrono>
#include <llvm/WinMutRuntime/filesystem/MirrorFileSystem.h>
#include <llvm/WinMutRuntime/filesystem/OpenFileTable.h>
#include <llvm/WinMutRuntime/init/init.h>
#include <llvm/WinMutRuntime/logging/LogFilePrefix.h>
#include <llvm/WinMutRuntime/memory/memory.h>
#include <llvm/WinMutRuntime/mutations/MutationCAPI.h>
#include <llvm/WinMutRuntime/mutations/MutationManager.h>
#include <llvm/WinMutRuntime/protocol/protocol.h>
#include <llvm/WinMutRuntime/signal/ExitFunc.h>
#include <llvm/WinMutRuntime/signal/Handlers.h>
#include <llvm/WinMutRuntime/timers/TimerDefault.h>
#include <malloc.h>
#include <sstream>
#include <wait.h>
extern "C" {
void reset_stdio();
}
#ifdef __linux__
#include <mcheck.h>
#endif

int __accmut__process_i32_arith(RegMutInfo *rmi, int from, int to, int left,
                                int right) {

  auto mm = MutationManager::getInstance();
  return mm->process_i32_arith(rmi, from, to, left, right);
}

int64_t __accmut__process_i64_arith(RegMutInfo *rmi, int from, int to,
                                    int64_t left, int64_t right) {

  auto mm = MutationManager::getInstance();
  return mm->process_i64_arith(rmi, from, to, left, right);
}

int __accmut__process_i32_cmp(RegMutInfo *rmi, int from, int to, int left,
                              int right) {

  auto mm = MutationManager::getInstance();
  return mm->process_i32_cmp(rmi, from, to, left, right);
}

int __accmut__process_i64_cmp(RegMutInfo *rmi, int from, int to, int64_t left,
                              int64_t right) {

  auto mm = MutationManager::getInstance();
  return mm->process_i64_cmp(rmi, from, to, left, right);
}

int __accmut__prepare_call(RegMutInfo *rmi, int from, int to, int opnum, ...) {

  auto mm = MutationManager::getInstance();
  va_list ap;
  va_start(ap, opnum);
  int ret = mm->prepare_call(rmi, from, to, opnum, ap);
  va_end(ap);
  return ret;
}

int __accmut__stdcall_i32() {

  auto mm = MutationManager::getInstance();
  return mm->stdcall_i32();
}

int64_t __accmut__stdcall_i64() {

  auto mm = MutationManager::getInstance();
  return mm->stdcall_i64();
}

void __accmut__stdcall_void() {

  auto mm = MutationManager::getInstance();
  mm->stdcall_void();
}

void __accmut__std_store() {

  auto mm = MutationManager::getInstance();
  mm->std_store();
}

int __accmut__prepare_st_i32(RegMutInfo *rmi, int from, int to, int tobestore,
                             int *addr) {

  auto mm = MutationManager::getInstance();
  return mm->prepare_st_i32(rmi, from, to, tobestore, addr);
}

int __accmut__prepare_st_i64(RegMutInfo *rmi, int from, int to,
                             int64_t tobestore, int64_t *addr) {

  auto mm = MutationManager::getInstance();
  return mm->prepare_st_i64(rmi, from, to, tobestore, addr);
}
void __accmut__init(int argc, char *argv[]) {
  init_libc_api_to_avoid_dead_loop();
  initialize_system(argc, argv);
}
void __accmut__init_timing(int argc, char *argv[]) {
  initialize_only_timing(argc, argv);
}

void __accmut__GoodVar_BBinit(RegMutInfo *rmi, int from, int to) {
  auto mm = MutationManager::getInstance();
  mm->goodvar_basic_block_init(rmi, from, to);
}

void __accmut__GoodVar_TablePush(int numOfGoodVar, int numOfMutants) {
  /*if (MUTATION_ID == 0) {
    char buf[1024];
    snprintf(buf, 1024, "push: %d %d\n", numOfGoodVar, numOfMutants);
    write(STDERR_FILENO, buf, strlen(buf));
  }*/
  auto mm = MutationManager::getInstance();
  mm->goodvar_table_push(numOfGoodVar, numOfMutants);
}

void __accmut__GoodVar_TablePush_max() {
  /*if (MUTATION_ID == 0) {
    write(STDERR_FILENO, "max\n", 4);
  }*/
  auto mm = MutationManager::getInstance();
  mm->goodvar_table_push_max();
}

void __accmut__GoodVar_TablePop() {
  /*if (MUTATION_ID == 0) {
    write(STDERR_FILENO, "pop\n", 4);
  }*/
  auto mm = MutationManager::getInstance();
  mm->goodvar_table_pop();
}

int32_t __accmut__process_i32_arith_GoodVar(int32_t left, int32_t right,
                                            GoodvarArg *arg) {

  auto mm = MutationManager::getInstance();
  return mm->process_i32_arith_goodvar(left, right, arg);
}

int64_t __accmut__process_i64_arith_GoodVar(int64_t left, int64_t right,
                                            GoodvarArg *arg) {

  auto mm = MutationManager::getInstance();
  return mm->process_i64_arith_goodvar(left, right, arg);
}

int32_t __accmut__process_i32_cmp_GoodVar(int32_t left, int32_t right,
                                          GoodvarArg *arg) {

  auto mm = MutationManager::getInstance();
  return mm->process_i32_cmp_goodvar(left, right, arg);
}

int32_t __accmut__process_i64_cmp_GoodVar(int64_t left, int64_t right,
                                          GoodvarArg *arg) {

  auto mm = MutationManager::getInstance();
  return mm->process_i64_cmp_goodvar(left, right, arg);
}

int32_t __accmut__process_i32_arith_GoodVar_init(int32_t left, int32_t right,
                                                 GoodvarArg *arg) {

  auto mm = MutationManager::getInstance();
  return mm->process_i32_arith_goodvar_init(left, right, arg);
}

int64_t __accmut__process_i64_arith_GoodVar_init(int64_t left, int64_t right,
                                                 GoodvarArg *arg) {

  auto mm = MutationManager::getInstance();
  return mm->process_i64_arith_goodvar_init(left, right, arg);
}

int32_t __accmut__process_i32_cmp_GoodVar_init(int32_t left, int32_t right,
                                               GoodvarArg *arg) {

  auto mm = MutationManager::getInstance();
  return mm->process_i32_cmp_goodvar_init(left, right, arg);
}

int32_t __accmut__process_i64_cmp_GoodVar_init(int64_t left, int64_t right,
                                               GoodvarArg *arg) {

  auto mm = MutationManager::getInstance();
  return mm->process_i64_cmp_goodvar_init(left, right, arg);
}

void __accmut__register(RegMutInfo *rmi, BlockRegMutBound **bound, int boundnum,
                        MutSpecs **specs, int specsnum, GoodvarArg **args,
                        int argsnum) {
  auto mm = MutationManager::getInstance();
  return mm->register_muts(rmi, bound, boundnum, specs, specsnum, args,
                           argsnum);
}

void __accmut__set_max_num(size_t numOfGoodVar, size_t numOfMutants) {
  auto mm = MutationManager::getInstance();
  mm->setMaxNum(numOfGoodVar, numOfMutants);
}

void __accmut__log(int mut_id, int mut_id_local, int rmi_off, int left,
                   int right, bool choose, int id) {
  if (mut_id != 0 && mut_id != INT_MAX) {
    if (choose) {
      char buf[1024];
      snprintf(buf, 1024, "%d %d %d %d %d %dCHOOSE\n", mut_id, mut_id_local,
               rmi_off, left, right, id);
      if (__accmut_libc_write(STDERR_FILENO, buf, strlen(buf)))
        exit(-1);
    } else {
      char buf[1024];
      snprintf(buf, 1024, "%d %d %d %d %d %dNOT CHOOSE\n", mut_id, mut_id_local,
               rmi_off, left, right, id);
      if (__accmut_libc_write(STDERR_FILENO, buf, strlen(buf)))
        exit(-1);
    }
  }
}
