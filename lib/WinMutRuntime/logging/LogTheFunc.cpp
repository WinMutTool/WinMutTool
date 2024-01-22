#include <llvm/WinMutRuntime/logging/LogTheFunc.h>

#ifdef LOGGING
int LogTheFuncCommon::depth = 0;

void LogTheFunc<void>::call() {
#ifndef LOG_ALL
  if (MUTATION_ID == 0) {
#endif
    std::string preStr = this->argString(WhenToLogThisArg::PRE);
    preStr += " errno = " + std::to_string(errno) + "\n";
    writeToLogFile("trace", preStr);
#ifndef LOG_ALL
  }
#endif
  ++this->depth;
  this->f();
  --this->depth;
#ifndef LOG_ALL
  if (MUTATION_ID == 0) {
#endif
    std::string postStr = this->argString(WhenToLogThisArg::POST);
    postStr += " = <void>";
    postStr += " errno = " + std::to_string(errno) + "\n";
    writeToLogFile("trace", postStr);
#ifndef LOG_ALL
  }
#endif
}
bool LogArgument::shouldPrint(WhenToLogThisArg w) {
  return (int(w) & int(wtp));
}
std::string LogArgument::toString(WhenToLogThisArg w) {
  if (w == WhenToLogThisArg::PRE) {
    return fpre(internal);
  } else {
    return fpost(internal);
  }
}
std::string LogArgument::toStringCharArray(const char *ptr, size_t extent) {
  if (!ptr) {
    return "<<null>>";
  }
  if (!memchr(ptr, 0, extent)) {
    std::string ret;
    for (size_t i = 0; i < extent; ++i)
      ret.push_back(ptr[i]);
    return "\"" + ret + "\"<<no null character found>>";
  }
  return "\"" + std::string(ptr) + "\"";
}

std::string LogArgument::toStringCharPtr(const char *ptr,
                                         size_t maxLenForResolvingArrays) {
  if (!ptr) {
    return "<<null>>";
  }
  if (!memchr(ptr, 0, maxLenForResolvingArrays)) {
    std::string ret;
    for (size_t i = 0; i < maxLenForResolvingArrays; ++i)
      ret.push_back(ptr[i]);
    return "\"" + ret + "\"<<no null character found>>";
  }
  return "\"" + std::string(ptr) + "\"";
}
#else
void LogTheFunc<void>::call() { f(); }

#endif
