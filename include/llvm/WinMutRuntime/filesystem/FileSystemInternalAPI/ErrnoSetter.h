#ifndef LLVM_ERRNOSETTER_H
#define LLVM_ERRNOSETTER_H

#include <errno.h>

namespace accmut {
class ErrnoSetter {
  int saved_errno;
  bool enabled = false;

public:
  ErrnoSetter() : saved_errno(errno) {}
  void set(int err) { saved_errno = err; }
  void enable() { enabled = true; }
  void disable() { enabled = false; }
  ~ErrnoSetter() {
    if (enabled)
      errno = saved_errno;
  }
};
} // namespace accmut

#endif // LLVM_ERRNOSETTER_H
