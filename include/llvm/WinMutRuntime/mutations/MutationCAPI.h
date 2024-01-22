#ifndef LLVM_MUTATIONCAPI_H
#define LLVM_MUTATIONCAPI_H

#include "MutationManager.h"
#include "MutationStorage.h"
#include <cstdint>
extern "C" {
int __accmut__process_i32_arith(RegMutInfo *rmi, int from, int to, int left,
                                int right);

int64_t __accmut__process_i64_arith(RegMutInfo *rmi, int from, int to,
                                    int64_t left, int64_t right);

int __accmut__process_i32_cmp(RegMutInfo *rmi, int from, int to, int left,
                              int right);

int __accmut__process_i64_cmp(RegMutInfo *rmi, int from, int to, int64_t left,
                              int64_t right);

int __accmut__prepare_call(RegMutInfo *rmi, int from, int to, int opnum, ...);

int __accmut__stdcall_i32();

int64_t __accmut__stdcall_i64();

void __accmut__stdcall_void();

void __accmut__std_store();

int __accmut__prepare_st_i32(RegMutInfo *rmi, int from, int to, int tobestore,
                             int *addr);

int __accmut__prepare_st_i64(RegMutInfo *rmi, int from, int to,
                             int64_t tobestore, int64_t *addr);

void __accmut__init(int argc, char *argv[]);
void __accmut__init_timing(int argc, char *argv[]);

void __accmut__GoodVar_BBinit(RegMutInfo *rmi, int from, int to);

void __accmut__GoodVar_TablePush(int numOfGoodVar, int numOfMutants);

void __accmut__GoodVar_TablePush_max();

void __accmut__GoodVar_TablePop();

int32_t __accmut__process_i32_arith_GoodVar(int32_t left, int32_t right,
                                            GoodvarArg *arg);

int64_t __accmut__process_i64_arith_GoodVar(int64_t left, int64_t right,
                                            GoodvarArg *arg);

int32_t __accmut__process_i32_cmp_GoodVar(int32_t left, int32_t right,
                                          GoodvarArg *arg);

int32_t __accmut__process_i64_cmp_GoodVar(int64_t left, int64_t right,
                                          GoodvarArg *arg);

int32_t __accmut__process_i32_arith_GoodVar_init(int32_t left, int32_t right,
                                                 GoodvarArg *arg);

int64_t __accmut__process_i64_arith_GoodVar_init(int64_t left, int64_t right,
                                                 GoodvarArg *arg);

int32_t __accmut__process_i32_cmp_GoodVar_init(int32_t left, int32_t right,
                                               GoodvarArg *arg);

int32_t __accmut__process_i64_cmp_GoodVar_init(int64_t left, int64_t right,
                                               GoodvarArg *arg);

void __accmut__register(RegMutInfo *rmi, BlockRegMutBound **bound, int boundnum,
                        MutSpecs **specs, int specsnum, GoodvarArg **args,
                        int argsnum);

void __accmut__set_max_num(size_t numOfGoodVar, size_t numOfMutants);

void __accmut__log(int mut_id, int mut_id_local, int rmi_off, int left,
                   int right, bool choose, int id);
}

#endif // LLVM_MUTATIONCAPI_H
