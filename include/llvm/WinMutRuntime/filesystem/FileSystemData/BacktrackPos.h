#ifndef LLVM_BACKTRACKPOS_H
#define LLVM_BACKTRACKPOS_H

#include "sys/stat.h"
#include <memory>
#include <string>

namespace accmut {
struct BacktrackPos {
  ino_t ino;
  std::string name;
  static inline std::shared_ptr<BacktrackPos> get(ino_t ino, std::string name) {
    return std::shared_ptr<BacktrackPos>(
        new BacktrackPos(ino, std::move(name)));
  }
  inline bool operator==(const BacktrackPos &rhs) const {
    return ino == rhs.ino && name == rhs.name;
  }
  inline bool operator!=(const BacktrackPos &rhs) const {
    return !(*this == rhs);
  }

  inline bool operator<(const BacktrackPos &rhs) const {
    return ino < rhs.ino || (ino == rhs.ino && name < rhs.name);
  }

  inline void setExpire() { expired = true; }

  inline bool isExpired() { return expired; }

private:
  bool expired = false;
  inline BacktrackPos(ino_t ino, std::string name)
      : ino(ino), name(std::move(name)) {}
};

struct compareBTPosPtr {
  bool operator()(const std::shared_ptr<BacktrackPos> &lhs,
                  const std::shared_ptr<BacktrackPos> &rhs) const {
    return *lhs < *rhs;
  }
};

} // namespace accmut

#endif // LLVM_BACKTRACKPOS_H
