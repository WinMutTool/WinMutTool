#ifndef LLVM_PATH_H
#define LLVM_PATH_H

#include <deque>
#include <memory>
#include <string>
#include <sys/param.h>

namespace accmut {
// immutable
class Path {
  bool is_relative{};
  std::deque<std::string> base;
  size_t ddnum{};
  bool ends_with_slash{};
  mutable std::string unix_path;
  mutable bool unix_computed = false;

  Path(bool is_relative, std::deque<std::string> base, size_t ddnum,
       bool ends_with_slash)
      : is_relative(is_relative), base(std::move(base)), ddnum(ddnum),
        ends_with_slash(ends_with_slash) {}

public:
  inline Path() = default;

  explicit Path(const std::string &str, bool remove_redundancy = true);

  const char *c_str() const;

  explicit operator const char *() const;

  Path operator+(const Path &rhs) const;

  static Path fromDeque(bool is_relative, const std::deque<std::string> &base,
                        size_t ddnum = 0, bool ends_with_slash = false,
                        bool remove_redundancy = true);

  std::string lastName() const;

  Path baseDir() const;

  inline bool isRelative() const { return is_relative; }

  inline std::deque<std::string> getDeque() const { return base; }

  inline size_t getDdnum() const { return ddnum; }

  inline bool isEndWithSlash() const { return ends_with_slash; }

  inline Path getNoEndWithSlash() const {
    return Path(is_relative, base, ddnum, false);
  }

  inline Path getEndWithSlash() const {
    return Path(is_relative, base, ddnum, true);
  }
};
} // namespace accmut

#endif // LLVM_PATH_H
