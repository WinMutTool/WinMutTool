#include <algorithm>
#include <chrono>
#include <fcntl.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/filesystem/MirrorFileSystem.h>
#include <llvm/WinMutRuntime/filesystem/OpenFileTable.h>
#include <llvm/WinMutRuntime/logging/LogFilePrefix.h>
#include <llvm/WinMutRuntime/logging/LogForMutTool.h>
#include <llvm/WinMutRuntime/mutations/MutationManager.h>
#include <llvm/WinMutRuntime/signal/Handlers.h>
#include <llvm/WinMutRuntime/timers/TimerDefault.h>
#include <llvm/WinMutRuntime/timers/Timers.h>
#include <llvm/WinMutRuntime/util/PageProtector.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/wait.h>


#include <llvm/Transforms/WinMut/DebugMacro.h>
// ------------------- mut output for demo site ---------------------
#include <llvm/WinMutRuntime/filesystem/MutOutput.h>
// ------------------- mut output for demo site ---------------------

void MutationManager::dump_eq_class() {
  int fd = __accmut_libc_open("eq_class", O_WRONLY | O_CREAT | O_APPEND, 0644);
  char buf[1024];
  snprintf(buf, 1024, "MUTATION_ID: %d, eq_num: %d\n", MUTATION_ID, eq_num);
  if (__accmut_libc_write(fd, buf, strlen(buf)) < 0)
    exit(-1);
  for (int i = 0; i < eq_num; ++i) {
    snprintf(buf, 1024, "%ld: ", eq_class[i].value);
    if (__accmut_libc_write(fd, buf, strlen(buf)) < 0)
      exit(-1);
    for (int j : eq_class[i].mut_id) {
      snprintf(buf, 1024, "%d ", j);
      if (__accmut_libc_write(fd, buf, strlen(buf)) < 0)
        exit(-1);
    }
    if (__accmut_libc_write(fd, "\n", 1) < 0)
      exit(-1);
  }
  __accmut_libc_close(fd);
}

void MutationManager::dump_spec(MutSpecs *spec) {
  int fd = __accmut_libc_open("eq_class", O_WRONLY | O_CREAT | O_APPEND, 0644);
  char buf[1024];
  if (__accmut_libc_write(fd, "spec: ", 6) < 0)
    exit(-1);
  for (int i = 0; i < spec->length; ++i) {
    snprintf(buf, 1024, "%d-%d ", spec->mutSpecs[i].from, spec->mutSpecs[i].to);
    if (__accmut_libc_write(fd, buf, strlen(buf)) < 0)
      exit(-1);
  }
  if (__accmut_libc_write(fd, "\n", 1))
    exit(-1);
  __accmut_libc_close(fd);
}

void MutationManager::divide_eqclass_cmp(int all_or, int all_and) {
  if (all_or == 0) {
    eq_class[0].mut_id.clear();
    eq_class[0].value = 0;
    for (int m : recent_set) {
      eq_class[0].mut_id.push_back(m);
    }
    eq_num = 1;
    return;
  } else if (all_and == 1) {
    eq_class[0].mut_id.clear();
    eq_class[0].value = 1;
    for (int m : recent_set) {
      eq_class[0].mut_id.push_back(m);
    }
    eq_num = 1;
    return;
  }

  int res_0 = temp_result[0];

  eq_class[0].mut_id.clear();
  eq_class[0].mut_id.push_back(MUTATION_ID);
  eq_class[0].value = res_0;

  eq_class[1].mut_id.clear();
  eq_class[1].value = 1 - res_0;

  for (unsigned i = 1; i < recent_set.size(); ++i) {
    if (temp_result[i] == res_0) {
      eq_class[0].mut_id.push_back(recent_set[i]);
    } else {
      eq_class[1].mut_id.push_back(recent_set[i]);
    }
  }
  eq_num = 2;
}
void MutationManager::divide_eqclass_cl_st() {
  if (recent_set.size() == 1) {
    eq_num = 1;
    eq_class[0].mut_id.clear();
    eq_class[0].mut_id.push_back(recent_set[0]);
    return;
  }

  int cur_zero_num = 0;
  for (unsigned i = 0; i < recent_set.size(); ++i) {
    int64_t result = temp_result[i];
    if (result == 0) {
      if (cur_zero_num == 0) {
        eq_class[0].mut_id.clear();
        eq_class[0].mut_id.push_back(recent_set[i]);
        cur_zero_num = 1;
      } else {
        eq_class[0].mut_id.push_back(recent_set[i]);
        cur_zero_num++;
      }
    } else {
      int idx = i - cur_zero_num + 1;
      eq_class[idx].mut_id.clear();
      eq_class[idx].mut_id.push_back(recent_set[i]);
    }
  }
  if (cur_zero_num > 0) {
    eq_num = recent_set.size() - cur_zero_num + 1;
  } else {
    eq_num = 1;
  }
}
void MutationManager::divide_eqclass() {
  eq_num = 0;
  for (unsigned i = 0; i < recent_set.size(); ++i) {
    int64_t result = temp_result[i];
    int flag = 0;
    for (int j = 0; j < eq_num; ++j) {
      if (eq_class[j].value == result) {
        eq_class[j].mut_id.push_back(recent_set[i]);
        flag = 1;
        break;
      }
    }
    if (flag == 0) {
      eq_class[eq_num].value = result;
      eq_class[eq_num].mut_id.clear();
      eq_class[eq_num].mut_id.push_back(recent_set[i]);
      ++eq_num;
    }
  }
}
void MutationManager::divide_eqclass(int64_t zeroval) {
  eq_class[0].value = zeroval;
  eq_class[0].mut_id.clear();
  eq_class[0].mut_id.push_back(0);
  eq_num = 1;
  for (unsigned i = 0; i < recent_set.size(); ++i) {
    int64_t result = temp_result[i];
    int flag = 0;
    for (int j = 0; j < eq_num; ++j) {
      if (eq_class[j].value == result) {
        eq_class[j].mut_id.push_back(recent_set[i]);
        flag = 1;
        break;
      }
    }
    if (flag == 0) {
      eq_class[eq_num].value = result;
      eq_class[eq_num].mut_id.clear();
      eq_class[eq_num].mut_id.push_back(recent_set[i]);
      ++eq_num;
    }
  }
}

void MutationManager::divide_eqclass_goodvar(int from, int to,
                                             int64_t zeroval) {
  eq_num = 0;
  int eq_pos = -1;
  for (unsigned i = 0; i < recent_set.size(); ++i) {
    int64_t result = temp_result[i];
    int flag = 0;
    for (int j = 0; j < eq_num; ++j) {
      if (eq_class[j].value == result) {
        eq_class[j].mut_id.push_back(recent_set[i]);
        flag = 1;
        break;
      }
    }
    if (flag == 0) {
      if (result == zeroval)
        eq_pos = eq_num;
      eq_class[eq_num].value = result;
      eq_class[eq_num].mut_id.clear();
      eq_class[eq_num].mut_id.push_back(recent_set[i]);
      ++eq_num;
    }
  }
  // MUTATION_ID must not be zero
  auto lb =
      std::upper_bound(forked_active_set.begin(), forked_active_set.end(), to);
  if (lb != forked_active_set.end()) {
    if (eq_pos == -1) {
      eq_class[eq_num].value = zeroval;
      eq_class[eq_num].mut_id.clear();
      eq_pos = eq_num;
      ++eq_num;
    }
    for (; lb != forked_active_set.end(); ++lb) {
      eq_class[eq_pos].mut_id.push_back(*lb);
    }
  }
}

void MutationManager::filter_mutants(const goodvar_mutant_specs_type &depSpec,
                                     int classid) {
  if (eq_class[classid].mut_id[0] == 0) {
    for (int i = 0; i < depSpec->length; ++i) {
      for (int j = depSpec->mutSpecs[i].from; j <= depSpec->mutSpecs[i].to;
           ++j) {
        default_active_set[j] = 0;
      }
    }
    /*
    for (auto mutspec : spec) {
      for (int j = mutspec.from; j <= mutspec.to; ++j) {
        default_active_set[j] = 0;
      }
    }
    */
    for (int m : eq_class[classid].mut_id) {
      default_active_set[m] = 1;
    }
  } else {
    forked_active_set.clear();
    forked_active_set = eq_class[classid].mut_id;
  }
}
int64_t MutationManager::fork_eqclass(const char *moduleName,
                                      const goodvar_mutant_specs_type &depSpec,
                                      int offset) { 
  if (eq_num == 0) {
    ALWAYS_LOG("eq_num is 0");
    exit(-1);
  }

  // {
  //   char buf[1000];
  //   sprintf(buf, "fork_eqclass\n");
  //   writeToLogFile("proc_tree", buf);
  // }

  int64_t result = eq_class[0].value;

  // -------------- proc_tree: reduced in ori -----------------
#ifdef PROC_TREE
  if (eq_class[0].mut_id[0] == 0){
    char buf[2000];
    sprintf(buf, " reduced:");

    for (int eq_class_mut_id :  eq_class[0].mut_id){
      sprintf(buf, "%s [%2d : %2ld]", buf, eq_class_mut_id, eq_class[0].value);
    }
    sprintf(buf, "%s\n", buf);
    writeToLogFile("proc_tree", buf);

    // ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------
#ifdef MUT_TOOL
    writeToMutToolLogFile("proc_tree", buf);
#endif
// ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------
  }
#endif
  // -------------- proc_tree: reduced in ori -----------------

  if (get_default_timer() != 0) {
    if (likely(MUTATION_ID != 0))
      accmut_disable_real_timer();
    for (int i = 1; i < eq_num; ++i) {

      auto start = std::chrono::steady_clock::now();

      sigset_t mask, orig_mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGCHLD);
      if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
        exit(-1);
      }
      // ------------------- mut output for demo site ---------------------
#ifdef MUT_OUTPUT_FOR_DEMO_SITE
      // accmut::MutOutput::getInstance()->createStdoutFileForN0(eq_class[i].mut_id);
      accmut::MutOutput::getInstance()->prepare_copy(MUTATION_ID, eq_class[i].mut_id);
#endif
      // ------------------- mut output for demo site ---------------------
      int pid = __accmut_libc_fork();
      if (pid < 0) {
        LOG("fork FAILED");
        exit(ENV_ERR);
      }

      if (pid == 0) {
        accmut_set_handlers();
        accmut_set_timer(get_default_timer());
        if (sigprocmask(SIG_SETMASK, &orig_mask, nullptr) < 0) {
          exit(-1);
        }

        filter_mutants(depSpec, i);
        {
          PageProtector pg(&MUTATION_ID, 1);
          MUTATION_ID = eq_class[i].mut_id[0];
          MUTATION_ID_LOCAL = eq_class[i].mut_id[0] - offset;

          accmut::MirrorFileSystem::getInstance()->setVirtual();
          accmut::OpenFileTable::getInstance()->setVirtual();
        }
        // ------------------- mut output for demo site ---------------------
#ifdef MUT_OUTPUT_FOR_DEMO_SITE
        // accmut::MutOutput::getInstance()->createStdoutFileForN0(eq_class[i].mut_id);
        accmut::MutOutput::getInstance()->copy_and_register_MutOutputFile(eq_class[i].mut_id);
#endif
        // ------------------- mut output for demo site ---------------------
        return eq_class[i].value;
      } else {
        struct timespec timeout;
        auto setusec = get_default_timer() * eq_class[i].mut_id.size() * 2;

        timeout.tv_sec = setusec / 1000000;
        timeout.tv_nsec = (setusec % 1000000) * 1000;
        do {
          if (sigtimedwait(&mask, nullptr, &timeout) < 0) {
            if (errno == EINTR) {
              continue;
            } else if (errno == EAGAIN) {
              kill(pid, SIGKILL);
            } else {
              __accmut_libc_perror("t");
              fprintf(stderr, "%ld %ld\n", timeout.tv_sec, timeout.tv_nsec);
              if (__accmut_libc_write(STDERR_FILENO, "ttimedwait err\n", 15))
                exit(-1);
              exit(-1);
            }
          }
          break;
        } while (true);
        if (sigprocmask(SIG_SETMASK, &orig_mask, nullptr) < 0) {
          if (__accmut_libc_write(STDERR_FILENO, "remove mask err\n", 5))
            exit(-1);
          exit(-1);
        }

        int status;

        int pr = waitpid(pid, &status, 0);
        auto end = std::chrono::steady_clock::now();
        auto usec =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count();

        char buf[1000];
        {
          int from = depSpec->mutSpecs[0].from;
          int to = depSpec->mutSpecs[depSpec->totlength - 1].to;
                   
          // ------------- proc_tree -----------------
#ifdef PROC_TREE
          char buf1[2000];
          sprintf(buf1, "%2d=>%2d:%2d:%2d(%s-%2d)", MUTATION_ID, 
                      from, to, eq_class[i].mut_id[0], 
                      moduleName, eq_class[i].mut_id[0] - offset);
          if (WIFEXITED(status)) {
            sprintf(buf1, "%s: /r%2d", buf1, WEXITSTATUS(status));
          } else if (WIFSIGNALED(status)) {
            sprintf(buf1, "%s: /s%2d", buf1, WTERMSIG(status));
          } else {
            sprintf(buf1, "%s: /e", buf1);
          }

          sprintf(buf1, "%s accmut::eq_class:", buf1);
          for (int eq_class_mut_id :  eq_class[i].mut_id)
            sprintf(buf1, "%s [%2d : %2ld]", buf1, eq_class_mut_id, eq_class[i].value);
          sprintf(buf1, "%s\n", buf1);
          writeToLogFile("proc_tree", buf1);
// ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------
#ifdef MUT_TOOL
          writeToMutToolLogFile("proc_tree", buf1);
#endif
// ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------
#endif
          // ------------- proc_tree -----------------

          if (WIFEXITED(status)) {
            sprintf(buf, "%d=>%d:%d:%d(%s-%d): %ld/r%d\n", MUTATION_ID, from,
                    to, eq_class[i].mut_id[0], moduleName,
                    eq_class[i].mut_id[0] - offset, usec, WEXITSTATUS(status));
          } else if (WIFSIGNALED(status)) {
            sprintf(buf, "%d=>%d:%d:%d(%s-%d): %ld/s%d\n", MUTATION_ID, from,
                    to, eq_class[i].mut_id[0], moduleName,
                    eq_class[i].mut_id[0] - offset, usec, WTERMSIG(status));
          } else {
            sprintf(buf, "%d=>%d:%d:%d(%s-%d): %ld\n", MUTATION_ID, from, to,
                    eq_class[i].mut_id[0], moduleName,
                    eq_class[i].mut_id[0] - offset, usec);
          }
        }
        writeToLogFile("forked", buf);
        sprintf(buf, "%s-%d\n", moduleName, eq_class[i].mut_id[0] - offset);
        writeToLogFile("forked-simple", buf);

        if (pr < 0) {
          kill(pid, SIGKILL);
          LOG("waitpid ERR\n");
        }
      }
    }
    if (likely(MUTATION_ID != 0))
      accmut_set_real_timer(get_default_timer() * 2);
  }
  filter_mutants(depSpec, 0);
  return result;
}
int32_t MutationManager::cal_i32_arith(int32_t op, int32_t a, int32_t b) {
  return cal_T_arith<int32_t, uint32_t>(op, a, b, INT32_MAX);
}
int64_t MutationManager::cal_i64_arith(int32_t op, int64_t a, int64_t b) {
  return cal_T_arith<int64_t, uint64_t>(op, a, b, INT64_MAX);
}
int32_t MutationManager::cal_i32_bool(int32_t pred, int32_t a, int32_t b) {
  return cal_T_bool<int32_t, uint32_t>(pred, a, b);
}
int32_t MutationManager::cal_i64_bool(int32_t pred, int64_t a, int64_t b) {
  return cal_T_bool<int64_t, uint64_t>(pred, a, b);
}
MutationManager::MutationManager() {
  all_mutation.resize(1);
  default_active_set.push_back(1);
}

void MutationManager::register_muts(RegMutInfo *rmi, BlockRegMutBound **bound,
                                    int boundnum, MutSpecs **specs,
                                    int specsnum, GoodvarArg **args,
                                    int argsnum) {
    // writeToMutToolLogFile("all_mutation", "dump:");

  if (likely(rmi->isReg))
    return;
  rmi->isReg = 1;
  rmi->offset = all_mutation.size() - 1;
  all_mutation.insert(all_mutation.end(), rmi->ptr, rmi->ptr + rmi->num);

  // ------------ dump all mutaiont for mut tool to calculate mutation score -------------
  // 将 all_mutation 设置为和 proc_tree 一样的递增式，原因见 grep case_185
// #ifdef DUMP_ALL_MUTATION
//     writeToMutToolLogFile("all_mutation", "dump:");

//   for (int i = 0; i < rmi->num; ++i) {
//     Mutation* m = (rmi->ptr + i);
//     char buf[1000];
//     sprintf(buf, "%d:%d:%d:%d:%d\n", m->type, m->sop, m->op_0, m->op_1, m->op_2);
//     writeToMutToolLogFile("all_mutation", buf);
//   }
// #endif
  // ------------ dump all mutaiont for mut tool to calculate mutation score -------------

  default_active_set.insert(default_active_set.end(), rmi->num, 1);
  for (int i = 0; i < boundnum; ++i) {
    bound[i]->left += rmi->offset;
    bound[i]->right += rmi->offset;
  }
  for (int i = 0; i < specsnum; ++i) {
    for (int j = 0; j < specs[i]->totlength; ++j) {
      /*
      auto ptr = &(specs[i]->mutSpecs[j]);
      ptr->from += rmi->offset;
      ptr->to += rmi->offset;
       */
    }
  }
  for (int i = 0; i < argsnum; ++i) {
    args[i]->from += rmi->offset;
    args[i]->to += rmi->offset;
    args[i]->good_from += rmi->offset;
    args[i]->good_to += rmi->offset;
    for (auto *spec : {args[i]->specs, args[i]->dep, args[i]->notdep}) {
      if (spec == nullptr)
        continue;
      for (int j = 0; j < spec->totlength; ++j) {
        auto ptr = &(spec->mutSpecs[j]);
        ptr->from += rmi->offset;
        ptr->to += rmi->offset;
      }
    }
  }
}

int MutationManager::process_i32_arith(RegMutInfo *rmi, int from, int to,
                                       int left, int right) {
  return process_T_arith(rmi, from, to, left, right,
                         all_mutation[to + rmi->offset].sop,
                         &(MutationManager::cal_i32_arith));
}

int64_t MutationManager::process_i64_arith(RegMutInfo *rmi, int from, int to,
                                           int64_t left, int64_t right) {
  return process_T_arith(rmi, from, to, left, right,
                         all_mutation[to + rmi->offset].sop,
                         &(MutationManager::cal_i64_arith));
}

template <typename T> T tabs(T x) {
  if (x < 0)
    return -x;
  return x;
}

template <typename OpType>
int MutationManager::process_T_cmp(RegMutInfo *rmi, int from, int to,
                                   OpType left, OpType right,
                                   int (*calc)(int, OpType, OpType)) {
  // register_muts(rmi);
  from += rmi->offset;
  to += rmi->offset;

  int s_pre = all_mutation[to].op_1;
  if (unlikely(system_disabled()) || !system_initialized())
    return calc(s_pre, left, right);

  if (likely(MUTATION_ID != 0)) {
    if (likely(MUTATION_ID > to || MUTATION_ID < from)) {
      return calc(s_pre, left, right);
    }
  }
  filter_variant(from, to);

  // generate recent_set
  int onlyhas_1 = 1;
  int onlyhas_0 = 0;

  temp_result.clear();
  for (size_t i = 0; i < recent_set.size(); ++i) {
    int mut_res =
        process_T_calc_mutation(s_pre, left, right, calc, recent_set[i]);
    temp_result.push_back(mut_res);
    onlyhas_1 &= mut_res;
    onlyhas_0 |= mut_res;
  } // end for i

  if (recent_set.size() == 1) {
    if (MUTATION_ID < from || MUTATION_ID > to) {
      return calc(s_pre, left, right);
    }
    return temp_result[0];
  }

  divide_eqclass_cmp(onlyhas_0, onlyhas_1);

  /*
    MutSpec spec;
    spec.from = from;
    spec.to = to;
    return fork_eqclass(goodvar_mutant_specs_type{spec}, rmi->offset);
    */
  MutSpecs specs;
  specs.length = 1;
  specs.totlength = 1;
  specs.mutSpecs[0].from = from;
  specs.mutSpecs[0].to = to;
  return fork_eqclass(rmi->moduleName,
                      &specs
                      // goodvar_mutant_specs_type{spec}
                      ,
                      rmi->offset);
}

int MutationManager::process_i32_cmp(RegMutInfo *rmi, int from, int to,
                                     int left, int right) {
  return process_T_cmp(rmi, from, to, left, right,
                       &(MutationManager::cal_i32_bool));
}
int MutationManager::process_i64_cmp(RegMutInfo *rmi, int from, int to,
                                     int64_t left, int64_t right) {
  return process_T_cmp(rmi, from, to, left, right,
                       &(MutationManager::cal_i64_bool));
}

/****************** PREPARE CALL **************************/
// TYPE BITS OF SIGNATURE
#define CHAR_TP 0
#define SHORT_TP 1
#define INT_TP 2
#define LONG_TP 3

int MutationManager::apply_call_mut(Mutation *m, PrepareCallParam *params) {

  if (m->type == STD) {
    return 1;
  }

  switch (m->type) {
  case LVR: {
    int idx = m->op_0;

    switch (params[idx].type) {
    case CHAR_TP: {
      char *ptr = (char *)params[idx].address;
      *ptr = m->op_2;
      break;
    }
    case SHORT_TP: {
      short *ptr = (short *)params[idx].address;
      *ptr = m->op_2;
      break;
    }
    case INT_TP: {
      int *ptr = (int *)params[idx].address;
      *ptr = m->op_2;
      break;
    }
    case LONG_TP: {
      int64_t *ptr = (int64_t *)params[idx].address;
      *ptr = m->op_2;
      break;
    }
    default: {
      LOG("ERR LVR params[idx].type ");
      exit(MUT_TP_ERR);
    }
    } // end switch(params[idx].type)
    break;
  }
  case UOI: {
    int idx = m->op_1;
    int uoi_tp = m->op_2;

    switch (params[idx].type) {
    case CHAR_TP: {
      char *ptr = (char *)params[idx].address;
      if (uoi_tp == 0) {
        *ptr = *ptr + 1;
      } else if (uoi_tp == 1) {
        *ptr = *ptr - 1;
      } else if (uoi_tp == 2) {
        *ptr = 0 - *ptr;
      }
      break;
    }
    case SHORT_TP: {
      short *ptr = (short *)params[idx].address;
      if (uoi_tp == 0) {
        *ptr = *ptr + 1;
      } else if (uoi_tp == 1) {
        *ptr = *ptr - 1;
      } else if (uoi_tp == 2) {
        *ptr = 0 - *ptr;
      }
      break;
    }
    case INT_TP: {
      int *ptr = (int *)params[idx].address;
      if (uoi_tp == 0) {
        *ptr = *ptr + 1;
      } else if (uoi_tp == 1) {
        *ptr = *ptr - 1;
      } else if (uoi_tp == 2) {
        *ptr = 0 - *ptr;
      }
      break;
    }
    case LONG_TP: {
      int64_t *ptr = (int64_t *)params[idx].address;
      if (uoi_tp == 0) {
        *ptr = *ptr + 1;
      } else if (uoi_tp == 1) {
        *ptr = *ptr - 1;
      } else if (uoi_tp == 2) {
        *ptr = 0 - *ptr;
      }
      break;
    }
    default: {
      LOG("ERR UOI params[idx].type ");
      exit(MUT_TP_ERR);
    }
    } // end switch(params[idx].type)
    break;
  }
  case ROV: {
    int idx1 = m->op_1;
    int idx2 = m->op_2;
    // op1
    switch (params[idx1].type) {
    case CHAR_TP: {
      char *ptr1 = (char *)params[idx1].address;
      // op2
      switch (params[idx2].type) {
      case CHAR_TP: {
        char *ptr2 = (char *)params[idx2].address;
        char tmp = *ptr1;
        *ptr1 = *ptr2;
        *ptr2 = tmp;
        break;
      }
      case SHORT_TP: {
        short *ptr2 = (short *)params[idx2].address;
        char tmp = (char)*ptr2;
        *ptr2 = *ptr1;
        *ptr1 = tmp;
        break;
      }
      case INT_TP: {
        int *ptr2 = (int *)params[idx2].address;
        char tmp = (char)*ptr2;
        *ptr2 = *ptr1;
        *ptr1 = tmp;
        break;
      }
      case LONG_TP: {
        int64_t *ptr2 = (int64_t *)params[idx2].address;
        char tmp = (char)*ptr2;
        *ptr2 = *ptr1;
        *ptr1 = tmp;
        break;
      }
      default: {
        LOG("ERR ROV params[idx2].type ");
        exit(MUT_TP_ERR);
      }
      } // end switch(params[idx2].type)
      break;
    }
    case SHORT_TP: {
      short *ptr1 = (short *)params[idx1].address;
      // op2
      switch (params[idx2].type) {
      case CHAR_TP: {
        char *ptr2 = (char *)params[idx2].address;
        char tmp = (short)*ptr1;
        *ptr1 = *ptr2;
        *ptr2 = tmp;
        break;
      }
      case SHORT_TP: {
        short *ptr2 = (short *)params[idx2].address;
        short tmp = *ptr2;
        *ptr2 = *ptr1;
        *ptr1 = tmp;
        break;
      }
      case INT_TP: {
        int *ptr2 = (int *)params[idx2].address;
        short tmp = (short)*ptr2;
        *ptr2 = *ptr1;
        *ptr1 = tmp;
        break;
      }
      case LONG_TP: {
        int64_t *ptr2 = (int64_t *)params[idx2].address;
        short tmp = (short)*ptr2;
        *ptr2 = *ptr1;
        *ptr1 = tmp;
        break;
      }
      default: {
        LOG("ERR ROV params[idx2].type ");
        exit(MUT_TP_ERR);
      }
      } // end switch(params[idx2].type)
      break;
    }
    case INT_TP: {
      int *ptr1 = (int *)params[idx1].address;
      // op2
      switch (params[idx2].type) {
      case CHAR_TP: {
        char *ptr2 = (char *)params[idx2].address;
        char tmp = (char)*ptr1;
        *ptr1 = *ptr2;
        *ptr2 = tmp;
        break;
      }
      case SHORT_TP: {
        short *ptr2 = (short *)params[idx2].address;
        short tmp = *ptr2;
        *ptr1 = *ptr2;
        *ptr2 = tmp;
        break;
      }
      case INT_TP: {
        int *ptr2 = (int *)params[idx2].address;
        int tmp = *ptr2;
        *ptr2 = *ptr1;
        *ptr1 = tmp;
        break;
      }
      case LONG_TP: {
        int64_t *ptr2 = (int64_t *)params[idx2].address;
        int tmp = (int)*ptr2;
        *ptr2 = *ptr1;
        *ptr1 = tmp;
        break;
      }
      default: {
        LOG("ERR ROV params[idx2].type ");
        exit(MUT_TP_ERR);
      }
      } // end switch(params[idx2].type)
      break;
    }
    case LONG_TP: {
      int64_t *ptr1 = (int64_t *)params[idx1].address;
      // op2
      switch (params[idx2].type) {
      case CHAR_TP: {
        char *ptr2 = (char *)params[idx2].address;
        char tmp = (char)*ptr1;
        *ptr1 = *ptr2;
        *ptr2 = tmp;
        break;
      }
      case SHORT_TP: {
        short *ptr2 = (short *)params[idx2].address;
        short tmp = (short)*ptr1;
        *ptr1 = *ptr2;
        *ptr2 = tmp;
        break;
      }
      case INT_TP: {
        int *ptr2 = (int *)params[idx2].address;
        int tmp = (int)*ptr2;
        *ptr1 = *ptr2;
        *ptr2 = tmp;
        break;
      }
      case LONG_TP: {
        int64_t *ptr2 = (int64_t *)params[idx2].address;
        int64_t tmp = *ptr2;
        *ptr2 = *ptr1;
        *ptr1 = tmp;
        break;
      }
      default: {
        LOG("ERR ROV params[idx2].type ");
        exit(MUT_TP_ERR);
      }
      } // end switch(params[idx2].type)
      break;
    }
    default: {
      LOG("ERR ROV params[idx1].type ");
      exit(MUT_TP_ERR);
    }
    } // end switch(params[idx1].type)
    break;
  }
  case ABV: {
    int idx = m->op_0;

    switch (params[idx].type) {
    case CHAR_TP: {
      char *ptr = (char *)params[idx].address;
      *ptr = abs(*ptr);
      break;
    }
    case SHORT_TP: {
      short *ptr = (short *)params[idx].address;
      *ptr = abs(*ptr);
      break;
    }
    case INT_TP: {
      int *ptr = (int *)params[idx].address;
      *ptr = abs(*ptr);
      break;
    }
    case LONG_TP: {
      int64_t *ptr = (int64_t *)params[idx].address;
      *ptr = labs(*ptr);
      break;
    }
    default: {
      LOG("ERR ABV params[idx].type ");
      exit(MUT_TP_ERR);
    }
    } // end switch(params[idx].type)
    break;
  }
  default: {
    LOG("ERR m->type ");
    exit(MUT_TP_ERR);
  }
  } // end switch(m->type)
  return 0;
}

#define MAX_PARAM_NUM 256

int MutationManager::prepare_call(RegMutInfo *rmi, int from, int to, int opnum,
                                  va_list ap) {
  if (unlikely(system_disabled() || !system_initialized()))
    return 0;
  // register_muts(rmi);
  from += rmi->offset;
  to += rmi->offset;

  if (likely(MUTATION_ID != 0)) {
    if (likely(MUTATION_ID > to || MUTATION_ID < from)) {
      return 0;
    }
  }

  filter_variant(from, to);

  // va_list ap;
  // va_start(ap, opnum);

  PrepareCallParam params[MAX_PARAM_NUM]; // max param num limited to 256

  for (int i = 0; i < opnum; i++) {
    int tp_and_idx = va_arg(ap, int);
    int idx = tp_and_idx & 0x00FF;

    params[idx].type = tp_and_idx >> 8;
    params[idx].address = va_arg(ap, uint64_t);

  } // end for i
  // va_end(ap);

  temp_result.clear();
  for (size_t i = 0; i < recent_set.size(); i++) {

    if (recent_set[i] == 0) {
      temp_result.push_back(0);
      continue;
    }

    Mutation m = all_mutation[recent_set[i]];

    int mut_res = recent_set[i];

    switch (m.type) {
    case UOI: {
      int idx = m.op_1;
      int uoi_tp = m.op_2;

      switch (params[idx].type) {
      case CHAR_TP: {
        char *ptr = (char *)params[idx].address;
        if (uoi_tp == 2 && (*ptr == 0)) {
          mut_res = 0;
        }
        break;
      }
      case SHORT_TP: {
        short *ptr = (short *)params[idx].address;
        if (uoi_tp == 2 && (*ptr == 0)) {
          mut_res = 0;
        }
        break;
      }
      case INT_TP: {
        int *ptr = (int *)params[idx].address;

        if (uoi_tp == 2 && (*ptr == 0)) {
          mut_res = 0;
        }
        break;
      }
      case LONG_TP: {
        int64_t *ptr = (int64_t *)params[idx].address;
        if (uoi_tp == 2 && (*ptr == 0)) {
          mut_res = 0;
        }
        break;
      }
      default: {
        LOG("UOI params[idx].type ");
        exit(MUT_TP_ERR);
      }
      } // end switch(params[idx].type)
      break;
    } // end case UOI
    case ROV: {
      int idx1 = m.op_1;
      int idx2 = m.op_2;
      // op1
      switch (params[idx1].type) {
      case CHAR_TP: {
        char *ptr1 = (char *)params[idx1].address;
        // op2
        switch (params[idx2].type) {
        case CHAR_TP: {
          char *ptr2 = (char *)params[idx2].address;
          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case SHORT_TP: {
          short *ptr2 = (short *)params[idx2].address;
          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case INT_TP: {
          int *ptr2 = (int *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case LONG_TP: {
          int64_t *ptr2 = (int64_t *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        default: {
          LOG("ROV params[idx2].type ");
          exit(MUT_TP_ERR);
        }
        } // end switch(params[idx2].type)
        break;
      }
      case SHORT_TP: {
        short *ptr1 = (short *)params[idx1].address;
        // op2
        switch (params[idx2].type) {
        case CHAR_TP: {
          char *ptr2 = (char *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case SHORT_TP: {
          short *ptr2 = (short *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case INT_TP: {
          int *ptr2 = (int *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case LONG_TP: {
          int64_t *ptr2 = (int64_t *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        default: {
          LOG("ROV params[idx2].type ");
          exit(MUT_TP_ERR);
        }
        } // end switch(params[idx2].type)
        break;
      }
      case INT_TP: {
        int *ptr1 = (int *)params[idx1].address;
        // op2
        switch (params[idx2].type) {
        case CHAR_TP: {
          char *ptr2 = (char *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case SHORT_TP: {
          short *ptr2 = (short *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case INT_TP: {
          int *ptr2 = (int *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case LONG_TP: {
          int64_t *ptr2 = (int64_t *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        default: {
          LOG("ROV params[idx2].type ");
          exit(MUT_TP_ERR);
        }
        } // end switch(params[idx2].type)
        break;
      }
      case LONG_TP: {
        int64_t *ptr1 = (int64_t *)params[idx1].address;
        // op2
        switch (params[idx2].type) {
        case CHAR_TP: {
          char *ptr2 = (char *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case SHORT_TP: {
          short *ptr2 = (short *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case INT_TP: {
          int *ptr2 = (int *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        case LONG_TP: {
          int64_t *ptr2 = (int64_t *)params[idx2].address;

          if (*ptr1 == *ptr2) {
            mut_res = 0;
          }
          break;
        }
        default: {
          LOG("ROV params[idx2].type ");
          exit(MUT_TP_ERR);
        }
        } // end switch(params[idx2].type)
        break;
      }
      default: {
        LOG("ROV params[idx1].type ");
        exit(MUT_TP_ERR);
      }
      } // end switch(params[idx1].type)
      break;
    } // end case ROV
    case ABV: {
      int idx = m.op_0;

      switch (params[idx].type) {
      case CHAR_TP: {
        char *ptr = (char *)params[idx].address;
        if (*ptr >= 0) {
          mut_res = 0;
        }
        break;
      }
      case SHORT_TP: {
        short *ptr = (short *)params[idx].address;
        if (*ptr >= 0) {
          mut_res = 0;
        }
        break;
      }
      case INT_TP: {
        int *ptr = (int *)params[idx].address;
        if (*ptr >= 0) {
          mut_res = 0;
        }
        break;
      }
      case LONG_TP: {
        int64_t *ptr = (int64_t *)params[idx].address;
        if (*ptr >= 0) {
          mut_res = 0;
        }
        break;
      }
      default: {
        LOG("ABV params[idx].type ");
        exit(MUT_TP_ERR);
      }
      } // end switch(params[idx].type)
      break;
    } // end case ABV
    default: {
    }
    } // end switch(m.type)

    temp_result.push_back(mut_res);

  } // end for i
  /*

      fprintf(stderr, "calc finish\n");
      fflush(stderr);
  */

  if (recent_set.size() == 1) {
    if (MUTATION_ID < from || MUTATION_ID > to) {
      return 0;
    }

    int tmpmid = temp_result[0]; // TODO :: CHECK !

    // printf("MID %d tmpmid %d\n", MUTATION_ID, tmpmid);

    Mutation m = all_mutation[tmpmid];

    if (tmpmid != 0 && m.type == STD) {
      return 1;
    } else {
      return apply_call_mut(&m, params);
    }
  }
  /* divide */
  divide_eqclass_cl_st();

  /* fork */
  /*
    MutSpec spec;
    spec.from = from;
    spec.to = to;
    fork_eqclass(goodvar_mutant_specs_type{spec}, rmi->offset);
    */

  MutSpecs specs;
  specs.length = 1;
  specs.totlength = 1;
  specs.mutSpecs[0].from = from;
  specs.mutSpecs[0].to = to;
  fork_eqclass(rmi->moduleName,
               &specs
               // goodvar_mutant_specs_type{spec}
               ,
               rmi->offset);

  /* apply the represent mutant*/

  if (MUTATION_ID == 0)
    return 0;

  return apply_call_mut(&all_mutation[MUTATION_ID], params);
}
int MutationManager::stdcall_i32() { return all_mutation[MUTATION_ID].op_2; }
int64_t MutationManager::stdcall_i64() {
  return all_mutation[MUTATION_ID].op_2;
}
void MutationManager::stdcall_void() { /*do nothing*/
}
void MutationManager::std_store() { /*do nothing*/
}
int MutationManager::apply_store_mut(Mutation *m, int64_t tobestore,
                                     uint64_t addr, int is_i32) {

  if (m->type == STD) {
    return 1;
  }

  switch (m->type) {
  case LVR: {
    tobestore = m->op_2;
    break;
  }
  case UOI: {
    int uoi_tp = m->op_2;
    if (uoi_tp == 0) {
      tobestore = tobestore + 1;
    } else if (uoi_tp == 1) {
      tobestore = tobestore - 1;
    } else if (uoi_tp == 2) {
      tobestore = 0 - tobestore;
    }
    break;
  }
  case ABV: {
    tobestore = labs(tobestore);
    break;
  }
  default: {
    LOG("m->type ERR ");
    exit(MUT_TP_ERR);
  }
  } // end switch(m->type)

  if (is_i32 == 1) {
    int *ptr = (int *)addr;
    *ptr = tobestore;
  } else {
    int64_t *ptr = (int64_t *)addr;
    *ptr = tobestore;
  }
  return 0;
}
int MutationManager::prepare_st_i32(RegMutInfo *rmi, int from, int to,
                                    int tobestore, int *addr) {
  if (unlikely(system_disabled() || !system_initialized())) {
    *addr = tobestore;
    return 0;
  }
  // register_muts(rmi);
  from += rmi->offset;
  to += rmi->offset;

  if (likely(MUTATION_ID != 0)) {
    if (likely(MUTATION_ID > to || MUTATION_ID < from)) {
      *addr = tobestore;
      return 0;
    }
  }
  filter_variant(from, to);

  temp_result.clear();
  for (size_t i = 0; i < recent_set.size(); ++i) {

    if (recent_set[i] == 0) {
      temp_result.push_back(0);
      continue;
    }

    Mutation m = all_mutation[recent_set[i]];

    int mut_res = recent_set[i];

    switch (m.type) {
    case UOI: {
      int uoi_tp = m.op_2;
      if (tobestore == 0 && uoi_tp == 2) {
        mut_res = 0;
      }
      break;
    } // end case UOI
    case ABV: {
      if (tobestore >= 0) { // TODO::check signed or unsigned are the same
        mut_res = 0;
      }
      break;
    } // end case ABV
    default:
      break;
    } // end switch(m->type)

    temp_result.push_back(mut_res);

  } // end for i

  if (recent_set.size() == 1) {

    if (MUTATION_ID < from || MUTATION_ID > to) {
      *addr = tobestore;
      return 0;
    }

    int tmpmid = temp_result[0];

    Mutation m = all_mutation[tmpmid];

    if (tmpmid != 0 && m.type == STD) {
      return 1;
    } else {
      return apply_store_mut(&m, tobestore, (uint64_t)addr, 1);
    }
  }

  divide_eqclass_cl_st();

  /* fork */
  /*
    MutSpec spec;
    spec.from = from;
    spec.to = to;
    fork_eqclass(goodvar_mutant_specs_type{spec}, rmi->offset);
    */

  MutSpecs specs;
  specs.length = 1;
  specs.totlength = 1;
  specs.mutSpecs[0].from = from;
  specs.mutSpecs[0].to = to;
  fork_eqclass(rmi->moduleName,
               &specs
               // goodvar_mutant_specs_type{spec}
               ,
               rmi->offset);

  if (MUTATION_ID == 0) {
    *addr = tobestore;
    return 0;
  }

  // printf("CURR_MID: %d   PID: %d\n", MUTATION_ID, getpid());

  /* apply the mutation */
  Mutation m = all_mutation[MUTATION_ID];

  if (m.type == STD) {
    return 1;
  }

  return apply_store_mut(&m, tobestore, (uint64_t)addr, 1);
}
int MutationManager::prepare_st_i64(RegMutInfo *rmi, int from, int to,
                                    int64_t tobestore, int64_t *addr) {
  if (unlikely(system_disabled() || !system_initialized())) {
    *addr = tobestore;
    return 0;
  }
  // register_muts(rmi);
  from += rmi->offset;
  to += rmi->offset;

  if (likely(MUTATION_ID != 0)) {
    if (likely(MUTATION_ID > to || MUTATION_ID < from)) {
      *addr = tobestore;
      return 0;
    }
  }

  filter_variant(from, to);

  temp_result.clear();
  for (size_t i = 0; i < recent_set.size(); ++i) {

    if (recent_set[i] == 0) {
      temp_result.push_back(0);
      continue;
    }

    Mutation m = all_mutation[recent_set[i]];

    int mut_res = recent_set[i];

    switch (m.type) {
    case UOI: {
      int uoi_tp = m.op_2;
      if (tobestore == 0 && uoi_tp == 2) {
        mut_res = 0;
      }
      break;
    } // end case UOI
    case ABV: {
      if (tobestore >= 0) { // TODO::check signed or unsigned are the same
        mut_res = 0;
      }
      break;
    } // end case ABV
    default: {
    }
    } // end switch(m->type)

    temp_result.push_back(mut_res);

  } // end for i

  if (recent_set.size() == 1) {

    if (MUTATION_ID < from || MUTATION_ID > to) {
      *addr = tobestore;
      return 0;
    }

    int tmpmid = temp_result[0];

    Mutation m = all_mutation[tmpmid];

    if (tmpmid != 0 && m.type == STD) {
      return 1;
    } else {
      return apply_store_mut(&m, tobestore, (uint64_t)addr, 0);
    }
  }

  /* divide */
  divide_eqclass_cl_st();

  /* fork */
  /*
    MutSpec spec;
    spec.from = from;
    spec.to = to;
    fork_eqclass(goodvar_mutant_specs_type{spec}, rmi->offset);
    */

  MutSpecs specs;
  specs.length = 1;
  specs.totlength = 1;
  specs.mutSpecs[0].from = from;
  specs.mutSpecs[0].to = to;
  fork_eqclass(rmi->moduleName,
               &specs
               // goodvar_mutant_specs_type{spec}
               ,
               rmi->offset);

  if (MUTATION_ID == 0) {
    *addr = tobestore;
    return 0;
  }

  /* apply the mutation */
  Mutation m = all_mutation[MUTATION_ID];

  if (m.type == STD) {
    return 1;
  }

  return apply_store_mut(&m, tobestore, (uint64_t)addr, 0);
}

void MutationManager::goodvar_basic_block_init(RegMutInfo *rmi, int from,
                                               int to) {
  /*
  from += rmi->offset;
  to += rmi->offset;

  if (likely(MUTATION_ID != 0) && likely(MUTATION_ID > to || MUTATION_ID <
  from)) { } else { get_goodvar_table().clear(); goodvar_mutant_specs.clear();
  }
  MutSpec &mutSpec = get_goodvar_bb_mutant_info();
  mutSpec.from = from;
  mutSpec.to = to;
   */
}
void MutationManager::goodvar_table_push(size_t numOfGoodVar,
                                         size_t numOfMutants) {
  using namespace std::chrono;
#ifdef DEBUG_OUTPUT
  char buf[1024];
  snprintf(buf, 1024, "push: %d %d %d\n", MUTATION_ID, numOfGoodVar,
           numOfMutants);
  int fd = open("eq_class", O_APPEND | O_CREAT | O_WRONLY, 0644);
  write(fd, buf, strlen(buf));
  auto s = high_resolution_clock::now();
#endif
  goodvar_table_stack.push(numOfGoodVar + 1, numOfMutants + 1, pool);

#ifdef DEBUG_OUTPUT
  snprintf(
      buf, 1024, "push time %ld\n",
      duration_cast<nanoseconds>(high_resolution_clock::now() - s).count());
  write(fd, buf, strlen(buf));
  __accmut_libc_close(fd);
#endif
}
void MutationManager::goodvar_table_push_max() {
  using namespace std::chrono;
#ifdef DEBUG_OUTPUT
  char buf[1024];
  snprintf(buf, 1024, "push: %d %d %d\n", MUTATION_ID, maxNumOfGoodVar,
           maxNumOfMutants);
  int fd = open("eq_class", O_APPEND | O_CREAT | O_WRONLY, 0644);
  write(fd, buf, strlen(buf));
  auto s = high_resolution_clock::now();
#endif
  goodvar_table_stack.push(maxNumOfGoodVar + 1, maxNumOfMutants + 1, pool);
#ifdef DEBUG_OUTPUT
  snprintf(
      buf, 1024, "push time max %ld\n",
      duration_cast<nanoseconds>(high_resolution_clock::now() - s).count());
  write(fd, buf, strlen(buf));
  __accmut_libc_close(fd);
#endif
}
void MutationManager::goodvar_table_pop() {
  using namespace std::chrono;
#ifdef DEBUG_OUTPUT
  char buf[1024];
  int fd = open("eq_class", O_APPEND | O_CREAT | O_WRONLY, 0644);
  write(fd, "pop\n", 4);
  auto s = high_resolution_clock::now();
#endif
  goodvar_table_stack.pop();
  // goodvar_pop_mutant_spec_stack();
#ifdef DEBUG_OUTPUT
  snprintf(
      buf, 1024, "pop time %ld\n",
      duration_cast<nanoseconds>(high_resolution_clock::now() - s).count());
  write(fd, buf, strlen(buf));
  __accmut_libc_close(fd);
#endif
}

bool MutationManager::is_active(int id) {
  if (MUTATION_ID == 0) {
    return default_active_set[id] == 1;
  } else {
    return std::find(forked_active_set.begin(), forked_active_set.end(), id) !=
           forked_active_set.end();
  }
}
