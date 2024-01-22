#ifndef LLVM_LOGTHEFUNC_H
#define LLVM_LOGTHEFUNC_H

#include "LogFilePrefix.h"
#include <functional>
#include <llvm/WinMutRuntime/mutations/MutationIDDecl.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <type_traits>
#include <unistd.h>
#include <vector>
#include <any>

#ifdef ACCMUT_LOG_FUNCTION_TRACE
template <typename T> struct dependent_false {
  static constexpr bool value = false;
};
template <typename T>
inline constexpr bool dependent_false_v = dependent_false<T>::value;
#endif

enum class WhenToLogThisArg { PRE = 1, POST = 2, BOTH = 3 };
#ifdef ACCMUT_LOG_FUNCTION_TRACE
class LogArgument {
public:
  using InternalToStringType = std::function<std::string(const std::any &)>;

private:
  std::any internal;
  InternalToStringType fpre, fpost;
  WhenToLogThisArg wtp;

public:
  template <typename T>
  LogArgument(T &&v, InternalToStringType fpre, InternalToStringType fpost,
              WhenToLogThisArg w);
  template <typename T>
  LogArgument(T &&v, InternalToStringType f,
              WhenToLogThisArg w = WhenToLogThisArg::BOTH);
  template <typename T>
  LogArgument(T &&v, size_t maxLenForResolvingArrays,
              WhenToLogThisArg w = WhenToLogThisArg::BOTH);
  template <typename T>
  LogArgument(T &&v, WhenToLogThisArg w = WhenToLogThisArg::BOTH,
              std::enable_if_t<!std::is_same_v<
                  std::remove_const_t<std::remove_reference_t<T>>, LogArgument>>
                  *_enable = nullptr);
  LogArgument(const LogArgument &) = default;
  LogArgument(LogArgument &&) = default;
  LogArgument &operator=(const LogArgument &) = default;
  LogArgument &operator=(LogArgument &&) = default;
  ~LogArgument() = default;
  bool shouldPrint(WhenToLogThisArg w);
  std::string toString(WhenToLogThisArg w = WhenToLogThisArg::PRE);

private:
  template <typename T> constexpr static bool shouldSavePointer();
  template <typename T> static auto getPointer(T &&arg);
  template <typename T> static std::any getAny(T &&arg);

public:
  template <typename T> static std::string toStringGenericValue(const T &x);
  static std::string toStringCharArray(const char *ptr, size_t extent);
  template <typename T>
  static std::string toStringGenericArray(const T *ptr, size_t extent,
                                          size_t maxLenForResolvingArrays);
  static std::string toStringCharPtr(const char *ptr,
                                     size_t maxLenForResolvingArrays);
  template <typename T>
  static std::string toStringGenericPtr(const T *ptr,
                                        size_t maxLenForResolvingArrays);
  template <typename T>
  static std::string anyToString(const std::any &a,
                                 size_t maxLenForResolvingArrays);
};
#else
class LogArgument {
public:
  template <typename... Arg> LogArgument(Arg &&... arg);
};
#endif

class LogTheFuncCommon {
#ifdef ACCMUT_LOG_FUNCTION_TRACE
protected:
  static int depth;
#endif
};

template <typename RetType> class LogTheFuncBase : public LogTheFuncCommon {
#ifdef ACCMUT_LOG_FUNCTION_TRACE
private:
  struct first_tag {};
  struct not_first_tag {};
  std::string funcName;
  std::deque<LogArgument> arguments;
  LogTheFuncBase(not_first_tag tag);
  template <typename T> LogTheFuncBase(not_first_tag tag, T &&arg);
  template <typename T, typename... Args>
  LogTheFuncBase(not_first_tag tag, T &&arg, Args &&... args);
  template <typename... Args>
  LogTheFuncBase(first_tag tag, const char *funcName, Args &&... args);

protected:
  std::string argString(WhenToLogThisArg w);
#endif
protected:
  std::function<RetType(void)> f;
  template <typename... Args>
  LogTheFuncBase(std::function<RetType(void)> func, const char *funcName,
                 Args &&... args);
};

template <typename RetType> class LogTheFunc : public LogTheFuncBase<RetType> {
private:
  std::function<std::string(const RetType &ret)> retToString;

public:
  RetType call();
  template <typename... Args>
  LogTheFunc(std::function<RetType(void)> func,
             std::function<std::string(const RetType &ret)> retToString,
             const char *funcname, Args &&... args);
  template <typename... Args>
  LogTheFunc(std::function<RetType(void)> func, const char *funcname,
             Args &&... args);
};

template <> class LogTheFunc<void> : public LogTheFuncBase<void> {
public:
  void call();
  template <typename... Args>
  LogTheFunc(std::function<void(void)> func, const char *funcname,
             Args &&... args);
};

// implemetation
#ifdef ACCMUT_LOG_FUNCTION_TRACE
template <typename T>
LogArgument::LogArgument(T &&v, LogArgument::InternalToStringType fpre,
                         LogArgument::InternalToStringType fpost,
                         WhenToLogThisArg w)
    : internal(getAny(std::forward<T>(v))), fpre(fpre), fpost(fpost), wtp(w) {}
template <typename T>
LogArgument::LogArgument(T &&v, LogArgument::InternalToStringType f,
                         WhenToLogThisArg w)
    : LogArgument(std::forward<T>(v), f, f, w) {}
template <typename T>
LogArgument::LogArgument(T &&v, size_t maxLenForResolvingArrays,
                         WhenToLogThisArg w)
    : LogArgument(
          std::forward<T>(v),
          [len = maxLenForResolvingArrays](const std::any &a) -> std::string {
            return anyToString<T>(a, len);
          },
          w) {}
template <typename T>
LogArgument::LogArgument(
    T &&v, WhenToLogThisArg w,
    std::enable_if_t<!std::is_same_v<
        std::remove_const_t<std::remove_reference_t<T>>, LogArgument>> *_enable)
    : LogArgument(std::forward<T>(v), [](const std::any &a) -> std::string {
        using Tnoref = std::remove_reference_t<T>;
        if constexpr (std::is_array_v<Tnoref>) {
          return anyToString<T>(a, 10);
        } else {
          return anyToString<T>(a, 1);
        }
      }) {}
template <typename T> constexpr bool LogArgument::shouldSavePointer() {
  using Tnoref = std::remove_reference_t<T>;
  return std::is_array_v<Tnoref> || std::is_pointer_v<Tnoref> ||
         std::is_lvalue_reference_v<T>;
}
template <typename T> auto LogArgument::getPointer(T &&arg) {
  using Tnoref = std::remove_reference_t<T>;
  if constexpr (std::is_array_v<Tnoref>) {
    return static_cast<std::remove_extent_t<Tnoref> *>(arg);
  } else if constexpr (std::is_pointer_v<Tnoref>) {
    return arg;
  } else if constexpr (std::is_lvalue_reference_v<T>) {
    return &arg;
  } else {
    static_assert(dependent_false_v<T>);
  }
}
template <typename T> std::any LogArgument::getAny(T &&arg) {
  if constexpr (shouldSavePointer<T>()) {
    return getPointer(std::forward<T>(arg));
  } else {
    return arg;
  }
}
template <typename T>
std::string LogArgument::toStringGenericValue(const T &x) {
  if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
    return std::to_string(x);
  } else {
    std::stringstream ss;
    ss << x;
    return ss.str();
  }
}
template <typename T>
std::string LogArgument::toStringGenericArray(const T *ptr, size_t extent,
                                              size_t maxLenForResolvingArrays) {
  if (!ptr) {
    return "<<null>>";
  }
  std::string ret = "{";
  size_t i = 0;
  for (; i < std::min(extent, maxLenForResolvingArrays); ++i) {
    if (i != 0)
      ret += ", ";
    ret += toStringGenericValue(ptr[i]);
  }
  if (i != extent) {
    ret += ", ...";
  }
  ret += "}";
  return ret;
}
template <typename T>
std::string LogArgument::toStringGenericPtr(const T *ptr,
                                            size_t maxLenForResolvingArrays) {
  if (!ptr) {
    return "<<null>>";
  }
  if (maxLenForResolvingArrays != 1) {
    std::string ret = "{";
    size_t i = 0;
    for (; i < maxLenForResolvingArrays; ++i) {
      if (i != 0)
        ret += ", ";
      ret += toStringGenericValue(ptr[i]);
    }
    ret += ", ...}";
  }
  return toStringGenericValue(ptr[0]);
}
template <typename T>
std::string LogArgument::anyToString(const std::any &a,
                                     size_t maxLenForResolvingArrays) {
  using Tnoref = std::remove_reference_t<T>;
  if constexpr (std::is_array_v<Tnoref>) {
    auto ptr = std::any_cast<std::remove_extent_t<Tnoref> *>(a);
    if constexpr (std::is_same_v<
                      std::remove_const_t<std::remove_extent_t<Tnoref>>,
                      char>) {
      // maxLenForResolvingArrays is ignored
      return toStringCharArray(ptr, std::extent_v<Tnoref>);
    } else {
      return toStringGenericArray(ptr, std::extent_v<Tnoref>,
                                  maxLenForResolvingArrays);
    }
  } else if constexpr (std::is_pointer_v<Tnoref>) {
    auto ptr = std::any_cast<Tnoref>(a);
    if constexpr (std::is_same_v<
                      std::remove_const_t<std::remove_pointer_t<Tnoref>>,
                      char>) {
      return toStringCharPtr(ptr, maxLenForResolvingArrays);
    } else {
      return toStringGenericPtr(ptr, maxLenForResolvingArrays);
    }
  } else if constexpr (std::is_lvalue_reference_v<T>) {
    return toStringGenericValue(
        *std::any_cast<std::remove_reference_t<T> *>(a));
  } else {
    return toStringGenericValue(std::any_cast<std::remove_reference_t<T>>(a));
  }
}
template <typename RetType>
LogTheFuncBase<RetType>::LogTheFuncBase(LogTheFuncBase::not_first_tag tag) {}
template <typename RetType>
template <typename T>
LogTheFuncBase<RetType>::LogTheFuncBase(LogTheFuncBase::not_first_tag tag,
                                        T &&arg) {
  arguments.emplace_front(std::forward<T>(arg));
}
template <typename RetType>
template <typename T, typename... Args>
LogTheFuncBase<RetType>::LogTheFuncBase(LogTheFuncBase::not_first_tag tag,
                                        T &&arg, Args &&... args)
    : LogTheFuncBase(not_first_tag{}, std::forward<Args>(args)...) {
  arguments.emplace_front(std::forward<T>(arg));
}
template <typename RetType>
template <typename... Args>
LogTheFuncBase<RetType>::LogTheFuncBase(LogTheFuncBase::first_tag tag,
                                        const char *funcName, Args &&... args)
    : LogTheFuncBase(not_first_tag{}, std::forward<Args>(args)...) {
  this->funcName = funcName;
}
template <typename RetType>
template <typename... Args>
LogTheFuncBase<RetType>::LogTheFuncBase(std::function<RetType(void)> func,
                                        const char *funcName, Args &&... args)
    : LogTheFuncBase(first_tag{}, funcName, std::forward<Args>(args)...) {
  f = func;
}
template <typename RetType>
std::string LogTheFuncBase<RetType>::argString(WhenToLogThisArg w) {
  std::string ret;
  ret.reserve(1024);
  for (int i = 0; i < this->depth; ++i)
    ret += "|   ";
  ret += funcName;
  ret += "(";
  for (size_t i = 0; i < arguments.size(); ++i) {
    if (i != 0) {
      ret += ", ";
    }
    if (arguments[i].shouldPrint(w)) {
      ret += arguments[i].toString(w);
    } else {
      ret += "<arg>";
    }
  }
  ret += ")";
  return ret;
}
template <typename RetType> RetType LogTheFunc<RetType>::call() {
#ifndef ACCMUT_LOG_ALL_FUNCTION_TRACE
  if (MUTATION_ID == 0) {
#endif
    std::string preStr = this->argString(WhenToLogThisArg::PRE);
    preStr += " errno = " + std::to_string(errno) + "\n";
    writeToLogFile("trace", preStr);
#ifndef ACCMUT_LOG_ALL_FUNCTION_TRACE
  }
#endif
  ++this->depth;
  RetType ret = this->f();
  --this->depth;
#ifndef ACCMUT_LOG_ALL_FUNCTION_TRACE
  if (MUTATION_ID == 0) {
#endif
    std::string postStr = this->argString(WhenToLogThisArg::POST);
    postStr += " = " + retToString(ret);
    postStr += " errno = " + std::to_string(errno) + "\n";
    writeToLogFile("trace", postStr);
#ifndef ACCMUT_LOG_ALL_FUNCTION_TRACE
  }
#endif
  return ret;
}
#else
template <typename... Arg> LogArgument::LogArgument(Arg &&... arg) {}
template <typename RetType> RetType LogTheFunc<RetType>::call() {
  return this->f();
}
template <typename RetType>
template <typename... Args>
LogTheFuncBase<RetType>::LogTheFuncBase(std::function<RetType(void)> func,
                                        const char *funcName, Args &&... args)
    : f(func) {}
#endif
template <typename RetType>
template <typename... Args>
LogTheFunc<RetType>::LogTheFunc(
    std::function<RetType(void)> func,
    std::function<std::string(const RetType &)> retToString,
    const char *funcname, Args &&... args)
    : LogTheFuncBase<RetType>(func, funcname, args...) {
  this->retToString = retToString;
}
template <typename RetType>
template <typename... Args>
LogTheFunc<RetType>::LogTheFunc(std::function<RetType(void)> func,
                                const char *funcname, Args &&... args)
    : LogTheFunc<RetType>(
          func,
          [](const RetType &ret) {
#ifdef ACCMUT_LOG_FUNCTION_TRACE
            return LogArgument(ret).toString();
#else
            return "No Logging";
#endif
          },
          funcname, args...) {
}

template <typename... Args>
LogTheFunc<void>::LogTheFunc(std::function<void(void)> func,
                             const char *funcname, Args &&... args)
    : LogTheFuncBase<void>(func, funcname, args...) {}

#endif // LLVM_LOGTHEFUNC_H
