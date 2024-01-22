#include "llvm/Transforms/WinMut/UglyRenamePass.h"
#include "llvm/Transforms/WinMut/printIR.h"
#include <map>
#include <set>
#include <string>
#include <vector>

UglyRenamePass::UglyRenamePass() : ModulePass(ID) {}

bool UglyRenamePass::runOnModule(Module &M) {
  printIR(M, ".ori.ll");

  std::set<std::string> accmut_catched_func{
      "access",
      "chdir",
      "chmod",
      "getcwd",
      "getwd",
      "lstat",
      "mkdir",
      "mkdirat",
      "mkdtemp",
      "readlink",
      "rename",
      "rmdir",
      "unlink",
      "stat",
      "closedir",
      "opendir",
      "readdir",
      "close",
      "dup",
      "dup2",
      "dup3",
      "fchdir",
      "fchown",
      "fcntl",
      "fstat",
      "lseek",
      "open",
      "read",
      "write",
      "fdopen",
      "fopen",
      "freopen",
      "perror",
      "fork",
      "pipe",
      "pipe2",
      "socket",
      "pthread_create",
      "remove",
      "openat",
      "mkdirat",
      "fchmodat",
      "fchmodat",
      "fstatat",
      "readlinkat",
      "renameat",
      "renameat2",
      "unlinkat",
      "fdopendir",
      "pthread_create",
      "pthread_cancel",
      "pthread_detach",
      "pthread_equal",
      "pthread_exit",
      "pthread_join",
      "pthread_kill",
      "pthread_once",
      "pthread_self",
      "pthread_setcancelstate",
      "pthread_setcanceltype",
      "pthread_testcancel",
      "pthread_attr_destroy",
      "pthread_attr_getinheritsched",
      "pthread_attr_getschedparam",
      "pthread_attr_getschedpolicy",
      "pthread_attr_getscope",
      "pthread_attr_getstacksize",
      "pthread_attr_getstackaddr",
      "pthread_attr_getdetachstate",
      "pthread_attr_init",
      "pthread_attr_setinheritsched",
      "pthread_attr_setschedparam",
      "pthread_attr_setschedpolicy",
      "pthread_attr_setscope",
      "pthread_attr_setstacksize",
      "pthread_attr_setstackaddr",
      "pthread_attr_setdetachstate",
      "pthread_mutexattr_destroy",
      "pthread_mutexattr_getprioceiling",
      "pthread_mutexattr_getprotocol",
      "pthread_mutexattr_gettype",
      "pthread_mutexattr_init",
      "pthread_mutexattr_setprioceiling",
      "pthread_mutexattr_setprotocol",
      "pthread_mutexattr_settype",
      "pthread_mutex_destroy",
      "pthread_mutex_init",
      "pthread_mutex_lock",
      "pthread_mutex_trylock",
      "pthread_mutex_unlock",
      "pthread_condattr_destroy",
      "pthread_condattr_init",
      "pthread_cond_broadcast",
      "pthread_cond_destroy",
      "pthread_cond_init",
      "pthread_cond_signal",
      "pthread_cond_timedwait",
      "pthread_cond_wait",
      "pthread_rwlock_destroy",
      "pthread_rwlock_init",
      "pthread_rwlock_rdlock",
      "pthread_rwlock_tryrdlock",
      "pthread_rwlock_trywrlock",
      "pthread_rwlock_unlock",
      "pthread_rwlock_wrlock",
      "pthread_rwlockattr_destroy",
      "pthread_rwlockattr_getpshared",
      "pthread_rwlockattr_init",
      "pthread_rwlockattr_setpshared",
      "pthread_key_create",
      "pthread_key_delete",
      "pthread_getspecific",
      "pthread_setspecific",
      "pthread_atfork",
      "vfork",
      "execve",
      "execvl",
      "execv",
      "execle",
      "execvp",
      "execlp",
      "fexecve",
  };

#ifdef __APPLE__
  /*
  std::map<std::string, std::string> gvmp = {
          {"__stdinp",  "accmutv2_stdin"},
          {"__stdoutp", "accmutv2_stdout"},
          {"__stderrp", "accmutv2_stderr"}
  };*/
  std::map<std::string, std::string> accmut_catched_func_mac_alias;
  for (auto &s : accmut_catched_func) {
    accmut_catched_func_mac_alias["\01_" + s] = s;
    llvm::errs() << "\\01_" + s << "\n";
  }
  for (auto &s : accmut_catched_func) {
    accmut_catched_func_mac_alias["\01_" + s + "$INODE64"] = s;
    llvm::errs() << "\\01_" + s << "\n";
  }
#else
#endif

  for (Function &F : M) {
    if (accmut_catched_func.find(F.getName().str()) !=
        accmut_catched_func.end()) {
      F.setName("__accmut_v3_" + F.getName());
    } else if (F.getName().endswith("64")) {
      auto it = accmut_catched_func.find(
          F.getName().substr(0, F.getName().size() - 2).str());
      if (it != accmut_catched_func.end()) {
        F.setName("__accmut_v3_" + *it);
      }
    }
#ifdef __APPLE__
    else {
      auto it = accmut_catched_func_mac_alias.find(F.getName().str());
      if (it != accmut_catched_func_mac_alias.end()) {
        F.setName("__accmut_v3_" + it->second);
      }
    }
#endif
  }
  /*
  for (GlobalVariable &gv : M.globals()) {
      if (gvmp.find(gv.getName()) != gvmp.end())
          gv.setName(gvmp[gv.getName()]);
  }*/
  printIR(M, ".new.ll");
  return true;
}

char UglyRenamePass::ID = 0;
static RegisterPass<UglyRenamePass> X("WinMut-UglyRenamePass",
                                      "WinMut - Rename FILE");
