#include <llvm/WinMutRuntime/checking/panic.h>
#include <llvm/WinMutRuntime/filesystem/Path.h>

namespace accmut {
Path::Path(const std::string &str, bool remove_redundancy) {
  if (str.empty()) {
    panic("Should not be zero length");
  }
  size_t b = 0, e = 0;
  is_relative = str[0] != '/';
  ends_with_slash = str.back() == '/';
  ddnum = 0;
  if (remove_redundancy) {
    while (e < str.size()) {
      if (str[e] == '/') {
        if (b == e) {
          b = ++e;
          continue;
        }
        std::string s = str.substr(b, e - b);
        if (s == ".") {
          // do nothing
        } else if (s == "..") {
          if (base.empty()) {
            if (is_relative)
              ddnum++;
          } else {
            base.pop_back();
          }
        } else {
          base.push_back(std::move(s));
        }
        b = ++e;
      } else {
        ++e;
      }
    }
    if (b != e) {
      std::string s = str.substr(b, e - b);
      if (s == ".") {
        ends_with_slash = true;
        // do nothing
      } else if (s == "..") {
        ends_with_slash = true;
        if (base.empty()) {
          if (is_relative)
            ddnum++;
        } else {
          base.pop_back();
        }
      } else {
        base.push_back(std::move(s));
      }
    }
  } else {
    while (e < str.size()) {
      if (str[e] == '/') {
        if (b == e) {
          b = ++e;
          continue;
        }
        base.push_back(str.substr(b, e - b));
        b = ++e;
      } else {
        ++e;
      }
    }
    if (b != e)
      base.push_back(str.substr(b, e - b));
  }
}

const char *Path::c_str() const {
  if (unix_computed)
    return unix_path.c_str();
  if (!is_relative)
    unix_path = "/";
  for (size_t i = 0; i < ddnum; ++i)
    unix_path += "../";
  for (const auto &s : base) {
    unix_path += s;
    unix_path += "/";
  }
  if (!base.empty() && !ends_with_slash)
    unix_path.pop_back();
  if (unix_path.empty())
    unix_path = "./";
  unix_computed = true;
  return unix_path.c_str();
}

Path::operator const char *() const { return c_str(); }

Path Path::operator+(const Path &rhs) const {
  if (!rhs.is_relative)
    return rhs;
  std::deque<std::string> que;
  size_t len1 = base.size();
  if (len1 <= rhs.ddnum) {
    return Path(is_relative, rhs.base, ddnum + rhs.ddnum - len1,
                rhs.ends_with_slash);
  } else {
    que.insert(que.end(), base.begin(), base.end() - rhs.ddnum);
    que.insert(que.end(), rhs.base.begin(), rhs.base.end());
    return Path(is_relative, std::move(que), ddnum, rhs.ends_with_slash);
  }
}

std::string Path::lastName() const {
  if (base.empty()) {
    if (is_relative)
      return ".";
    else
      return "/";
  }
  return base.back();
}

Path Path::baseDir() const {
  auto d1 = base;
  if (d1.empty()) {
    if (is_relative)
      return Path(is_relative, d1, ddnum + 1, true);
    return Path("/");
  }
  d1.pop_back();
  return Path(is_relative, d1, ddnum, true);
}

Path Path::fromDeque(bool is_relative, const std::deque<std::string> &base,
                     size_t ddnum, bool ends_with_slash,
                     bool remove_redundancy) {
  std::deque<std::string> base1;
  for (const auto &s : base) {
    if (s == ".")
      continue;
    if (s == "..") {
      if (base1.empty()) {
        if (is_relative)
          ++ddnum;
      } else {
        base1.pop_back();
      }
    } else {
      base1.push_back(s);
    }
  }
  return Path(is_relative, std::move(base1), ddnum, ends_with_slash);
}
} // namespace accmut
