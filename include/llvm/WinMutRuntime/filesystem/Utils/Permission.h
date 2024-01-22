#ifndef LLVM_PERMISSION_H
#define LLVM_PERMISSION_H

#include <sys/stat.h>
#include <unistd.h>

namespace accmut {
inline bool check_read_perm(struct stat &s) {
  if (geteuid() == 0)
    return true;
  if (getuid() == s.st_uid) {
    return static_cast<bool>(s.st_mode & S_IRUSR);
  } else if (getgid() == s.st_gid) {
    return static_cast<bool>(s.st_mode & S_IRGRP);
  } else {
    return static_cast<bool>(s.st_mode & S_IROTH);
  }
}

inline bool check_write_perm(struct stat &s) {
  if (geteuid() == 0)
    return true;
  if (getuid() == s.st_uid) {
    return static_cast<bool>(s.st_mode & S_IWUSR);
  } else if (getgid() == s.st_gid) {
    return static_cast<bool>(s.st_mode & S_IWGRP);
  } else {
    return static_cast<bool>(s.st_mode & S_IWOTH);
  }
}

inline bool check_execute_perm(struct stat &s) {
  if (geteuid() == 0)
    return true;
  if (getuid() == s.st_uid) {
    return static_cast<bool>(s.st_mode & S_IXUSR);
  } else if (getgid() == s.st_gid) {
    return static_cast<bool>(s.st_mode & S_IXGRP);
  } else {
    return static_cast<bool>(s.st_mode & S_IXOTH);
  }
}
} // namespace accmut

#endif // LLVM_PERMISSION_H
