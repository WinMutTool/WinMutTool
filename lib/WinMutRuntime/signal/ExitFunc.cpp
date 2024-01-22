#include <cassert>
#include <chrono>
#include <dlfcn.h>
#include <limits>
#include <llvm/WinMutRuntime/logging/LogFilePrefix.h>
#include <llvm/WinMutRuntime/mutations/MutationIDDecl.h>
#include <llvm/WinMutRuntime/signal/ExitFunc.h>
#include <llvm/WinMutRuntime/init/init.h>
#include <string>
#include <unistd.h>

#include <llvm/Transforms/WinMut/DebugMacro.h>
#include <llvm/WinMutRuntime/logging/LogForMutTool.h>

void (*exit_func)(int) = nullptr;
void (*_exit_func)(int) = nullptr;
int originalReturnVal = 0;
void setOriginalReturnVal(int val) { originalReturnVal = val; }
int getOriginalReturnVal() { return originalReturnVal; }
static std::string printLine;
const std::string &getPrintLine() {
  return printLine;
}
void setPrintLine(const std::string &str) {
  printLine = str;
}


extern "C" {
extern void exit(int __status) {
  if (exit_func == nullptr) {
    exit_func = (void (*)(int))dlsym(RTLD_NEXT, "exit");
  }
if (system_initialized() && !system_disabled()) {
  if (MUTATION_ID != 0 && MUTATION_ID != std::numeric_limits<int>::max() &&
      __status != originalReturnVal) {
    _exit(__status);
  }
  if (MUTATION_ID == 0)
    fflush(nullptr);
}

  // ------------ log ori exit value for mut tool to calculate mutation score -------------
#ifdef MUT_TOOL
  if (MUTATION_ID == 0){
    char buf[1000];
    sprintf(buf, "accmut::ori_exit_val: %d\n", __status);
    writeToMutToolLogFile("proc_tree", buf);
  }
 
#endif
// ------------ log ori exit value for mut tool to calculate mutation score -------------

  exit_func(__status);
  assert(false);
}
}
