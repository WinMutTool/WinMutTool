#include <algorithm>
#include <chrono>
#include <llvm/WinMutRuntime/filesystem/MirrorFileSystem.h>
#include <llvm/WinMutRuntime/filesystem/OpenFileTable.h>
#include <llvm/WinMutRuntime/filesystem/Path.h>
#include <llvm/WinMutRuntime/checking/panic.h>
#include <llvm/WinMutRuntime/init/init.h>
#include <llvm/WinMutRuntime/logging/LogFilePrefix.h>
#include <llvm/WinMutRuntime/memory/memory.h>
#include <llvm/WinMutRuntime/mutations/MutationCAPI.h>
#include <llvm/WinMutRuntime/mutations/MutationManager.h>
#include <llvm/WinMutRuntime/protocol/protocol.h>
#include <llvm/WinMutRuntime/signal/ExitFunc.h>
#include <llvm/WinMutRuntime/signal/Handlers.h>
#include <llvm/WinMutRuntime/timers/TimerDefault.h>
#include <map>
#include <sstream>
#include <sys/wait.h>

#include <llvm/Transforms/WinMut/DebugMacro.h>

// ------------------- mut output for demo site ---------------------
#include <llvm/WinMutRuntime/filesystem/MutOutput.h>
// ------------------- mut output for demo site ---------------------

// ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------
#ifdef MUT_TOOL
#include <llvm/WinMutRuntime/logging/LogForMutTool.h>
#endif
// ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------

extern "C" {
extern void init_stdio();
}

static int initialized = false;
static int disabled = false;
int system_initialized() { return initialized; }
int system_disabled() { return disabled; }
static std::string get_print_line(const char *prefix) {
  auto d = std::chrono::steady_clock::now().time_since_epoch();
  return prefix +
         std::to_string(
             std::chrono::duration_cast<std::chrono::milliseconds>(d).count()) +
         " " + getPrintLine() + "\n";
}
static void print_time_line(const char *prefix) {
  for (const auto *filename :
       {"forked", "forked-simple", "timing", "timeout", "panic", "trace"}) 
  {
    writeToLogFile(filename, get_print_line(prefix).c_str());
  }

#ifdef PROC_TREE
  writeToLogFile("proc_tree", get_print_line(prefix).c_str());
#endif

  // ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------
#ifdef MUT_TOOL
  writeToMutToolLogFile("proc_tree",  get_print_line(prefix).c_str());
#endif
// ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------

// ------------ dump all mutaiont for mut tool to calculate mutation score -------------
#ifdef DUMP_ALL_MUTATION
    writeToMutToolLogFile("all_mutation",  get_print_line(prefix).c_str());
#endif
// ------------ dump all mutaiont for mut tool to calculate mutation score -------------

}
static void print_single_time_line(const char *prefix, const char *filename) {
  auto d = std::chrono::steady_clock::now().time_since_epoch();
  writeToLogFile(filename, get_print_line(prefix).c_str());
}

// ------------ dump all mutaiont for mut tool to calculate mutation score -------------
#ifdef DUMP_ALL_MUTATION
static void dump_all_mutation(){
  auto mm = MutationManager::getInstance();
  std::vector<Mutation> all_mutation = mm->get_all_mutation();
  for (auto m : all_mutation) {
    char buf[1000];
    sprintf(buf, "%d:%d:%d:%d:%d\n", m.type, m.sop, m.op_0, m.op_1, m.op_2);
    writeToMutToolLogFile("all_mutation", buf);
  }
}
#endif
// ------------ dump all mutaiont for mut tool to calculate mutation score -------------


static void exit_time_printer() {
  if (MUTATION_ID == 0) {
    static bool done = false;
    if (!done) {
      done = true;
    } else {
      return;
    }

// ------------ dump all mutaiont for mut tool to calculate mutation score -------------
#ifdef DUMP_ALL_MUTATION
  dump_all_mutation();  // 已弃用，改为在 register 时递增输出
#endif
// ------------ dump all mutaiont for mut tool to calculate mutation score -------------

    const char *prefix = "++ ";
    print_time_line(prefix);
    writeToLogFile("ran", "#");
  }
}
static std::chrono::time_point<std::chrono::steady_clock> globalstart;
static void exit_time_printer_only_timing() {
  auto end = std::chrono::steady_clock::now();
  auto refusec = std::chrono::duration_cast<std::chrono::microseconds>(
		  end - globalstart).count();

  char buf[1024];
  snprintf(buf, 1024, "%ld\n", refusec);
  writeToLogFile("timing", buf);
  print_single_time_line("++ ", "timing");
  writeToLogFile("ran", "#");
}
static void initialize_print_line(int argc, char **argv) {
  std::stringstream toPrintSS;
  if (argc < 0) {
    toPrintSS << "-- #NO ARGC#";
  } else if (argv == nullptr) {
    toPrintSS << "-- #NO ARGV#";
  } else {
    toPrintSS << "--";
    for (int i = 0; i < argc; ++i) {
      toPrintSS << " " << argv[i];
    }
    toPrintSS << "";
  }
  std::string str = toPrintSS.str();
  std::map<char, const char *> escape = {
      {'\a', "\\a"}, {'\b', "\\b"}, {'\f', "\\f"}, {'\n', "\\n"},
      {'\r', "\\r"}, {'\t', "\\t"}, {'\v', "\\v"}, {'\\', "\\\\"},
  };
  for (const auto &p : escape) {
    int pos = str.find(p.first);
    while (pos != -1) {
      str.replace(pos, 1, p.second);
      pos = str.find(p.first, pos + 2);
    }
  }
  setPrintLine(str);
}
void initialize_only_timing(int argc, char **argv) {
  char *logFilePrefix = getenv("WINMUT_LOG_FILE_PREFIX");
  if (logFilePrefix) {
    setLogFilePrefix(logFilePrefix);
  }
  disable_system();
  initialize_print_line(argc, argv);
  print_single_time_line("-- ", "timing");
  globalstart = std::chrono::steady_clock::now();
  atexit(exit_time_printer_only_timing);
}
void initialize_system(int argc, char **argv) {

#ifdef __linux__
  /*
if (mcheck(abort_func) < 0) {
  exit(MALLOC_ERR);
}*/
  init_memory_hook();
#endif
  char *isDisabled = getenv("WINMUT_DISABLED");
  if (isDisabled && !strcmp(isDisabled, "YES")) {
    disable_system();
    return;
  }

  char *logFilePrefix = getenv("WINMUT_LOG_FILE_PREFIX");
  if (logFilePrefix) {
    setLogFilePrefix(logFilePrefix);
  }

  char *maxRunCasesStr = getenv("WINMUT_MAX_RUN_CASES");
  int maxRunCases = INT_MAX;
  if (maxRunCasesStr) {

    maxRunCases = strtol(maxRunCasesStr, NULL, 10);
    accmut::Path runnedFile = accmut::Path(getLogFilePrefix()) + accmut::Path("ran");
    int alreadyRunned = 0;
    struct stat st;
    if (__accmut_libc_lstat(runnedFile.c_str(), &st) != -1) {
      alreadyRunned = st.st_size;
    }

    if (alreadyRunned >= maxRunCases) {
      disable_system();
      return;
    }

// ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------
// 只有在指定了 WINMUT_MAX_RUN_CASES 时才起效
#ifdef MUT_TOOL
    mkMutToolLogDir(alreadyRunned + 1);
#endif
// ---------------- 构建为 run的 case 输出目录 ： WINMUT_LOG_FILE_PREFIX/run/case_xx -----------------
  }

  char *measureCount = getenv("WINMUT_MEASURE_COUNT");
  int measure_times = 3;
  if (measureCount) {
    measure_times = strtol(measureCount, NULL, 10);
  }


  accmut_set_handlers();
  MutationManager::getInstance()->goodvar_mutant_specs_stack_init();
  /*
  if (TIME < 0) {
    LOG("TIME NOT INIT\n");
    exit(ENV_ERR);
  }*/

  accmut::MirrorFileSystem::hold();
  accmut::OpenFileTable::hold();
  setlinebuf(stdout);
  setvbuf(stderr, nullptr, _IONBF, 0);

  initialize_print_line(argc, argv);

  const char *prefix = "-- ";
  print_time_line(prefix);

  if (disabled) {
    writeToLogFile("timing", "0:earlypanic");
    exit_time_printer();
    return;
  }
  initialized = 1;

  init_stdio();

  // ------------------- mut output for demo site ---------------------
#ifdef MUT_OUTPUT_FOR_DEMO_SITE
  accmut::MutOutput::hold();
  // accmut::MutOutput::getInstance()->createStdoutFileFor0();
#endif
  // ------------------- mut output for demo site ---------------------
  
  bool panic = false;

  long usecmin = std::numeric_limits<long>::max();
  long refusecmin = std::numeric_limits<long>::max();
  for (int i = 0; i < measure_times; ++i) {
    int fd[2];
    if (i == 0) {
      if (__accmut_libc_pipe(fd) < 0) {
        __accmut_libc_perror("pipe");
        exit(-1);
      }
    }
    auto start = std::chrono::steady_clock::now();
    pid_t pid = __accmut_libc_fork();
    if (pid == -1) {
      __accmut_libc_perror("fork");
      exit(-1);
    }

    if (pid == 0) {
      MUTATION_ID = std::numeric_limits<int>::max();
      accmut::MirrorFileSystem::getInstance()->setVirtual();
      accmut::OpenFileTable::getInstance()->setVirtual();
      if (i == 0) {
        __accmut_libc_close(fd[0]);
        accmut::MirrorFileSystem::getInstance()->setPipeFd(fd[1]);
      }
      i = measure_times;
    } else {
      if (i == 0) {
        __accmut_libc_close(fd[1]);
        MessageHdr hdr;
        while (true) {
          if (readHdr(fd[0], &hdr) != 0)
            break;
          char *buf = new char[hdr.length + 1];
          if (readMsg(fd[0], hdr, buf) != 0) {
            delete[] buf;
            break;
          }
          switch (hdr.type) {
          case MessageHdr::CACHE:
            accmut::MirrorFileSystem::getInstance()->precache(buf);
            break;
          case MessageHdr::PANIC:
            panic = true;
            disable_system();
            writeToLogFile("panic", buf);
            break;
          default:
            assert(false);
          }
          if (panic) {
            break;
          }
        }
        if (!panic) {
          atexit(exit_time_printer);
        } else {
          i = measure_times - 1; // jump out
        }
        __accmut_libc_close(fd[0]);
      }

      int status;
      int r = waitpid(pid, &status, 0);
      if (r < 0) {
        __accmut_libc_perror("waitpid");
        exit(-1);
      }
      auto end = std::chrono::steady_clock::now();
      auto refusec =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start)
              .count();

      if (WIFEXITED(status)) {
        setOriginalReturnVal(WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
        setOriginalReturnVal(128 + WTERMSIG(status));
      } else {
        const char *str = "UNHANDLED EXIT STATUS FOR ORIGINAL PROGRAM\n";
        __accmut_libc_write(STDERR_FILENO, str, strlen(str));
        exit(-1);
      }

      // auto usec = std::min(refusec + 2000000, refusec * 5);
      auto usec = refusec * 3;
      if (i != 0) {
        usecmin = std::min(usec, usecmin);
	refusecmin = std::min(refusec, refusecmin);
      }

      if (i == measure_times - 1) {
        char buf[1024];
        if (panic)
          snprintf(buf, 1024, "%ld:panic\n", refusecmin);
        else
          snprintf(buf, 1024, "%ld:%d\n", refusecmin, getOriginalReturnVal());
        writeToLogFile("timing", buf);
        set_default_timer(usecmin);
        if (panic)
          exit_time_printer();
      }
    }
  }
}
extern "C" {
extern void reset_stdio();
extern void remove_memory_hook(void);
}
void disable_system() {
  if (disabled)
    return;
  if (MUTATION_ID != 0 && MUTATION_ID != INT_MAX) {
    error("disabling in sub-process", true);
  }
  disabled = true;
  if (initialized) {
    reset_stdio();
    remove_memory_hook();
  } else {
    error("Disabling system before initialize", false);
  }
}
