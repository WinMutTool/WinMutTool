#ifndef LLVM_INODE_H
#define LLVM_INODE_H

#include "../ErrorCode.h"
#include "../Utils/Permission.h"
#include "BacktrackPos.h"
#include "BaseFile.h"
#include "RegularFile.h"
#include <list>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <sys/stat.h>
#include <system_error>
#include <vector>

namespace accmut {
class INode {
  friend class MirrorFileSystemTest;
  struct stat meta;
  std::shared_ptr<BaseFile> content;
  std::set<std::shared_ptr<BacktrackPos>, compareBTPosPtr> btpos;

public:
  inline std::shared_ptr<BacktrackPos> getBTPos(ino_t ino,
                                                const std::string &name) {
    for (auto &ptr : btpos) {
      if (ptr->ino == ino && ptr->name == name)
        return ptr;
    }
    return nullptr;
  }

  inline std::shared_ptr<BacktrackPos> addBTPos(ino_t ino,
                                                const std::string &name) {
    auto ptr = BacktrackPos::get(ino, name);
    btpos.insert(ptr);
    return ptr;
  }

  void deleteBTPos(ino_t ino, const std::string &name);
  inline const std::set<std::shared_ptr<BacktrackPos>, compareBTPosPtr> &
  getBTPosSet() const {
    return btpos;
  }

  virtual ~INode() = default;

  void dump(FILE *f);

  // meta-data
  inline struct stat getStat() { return meta; }

  void trunc();
  inline bool isDir() { return S_ISDIR(meta.st_mode); }

  inline bool isLnk() { return S_ISLNK(meta.st_mode); }

  inline bool isReg() { return S_ISREG(meta.st_mode); }

  inline bool isCharSpecial() { return S_ISCHR(meta.st_mode); }

  inline bool isPipe() { return S_ISFIFO(meta.st_mode); }

  inline bool canRead() { return check_read_perm(meta); }

  inline bool canWrite() { return check_write_perm(meta); }

  inline bool canExecute() { return check_execute_perm(meta); }

  inline ino_t getIno() { return meta.st_ino; }

  inline bool cached() {
    if (content) {
      return content->isCached();
    }
    return false;
  }

  inline std::shared_ptr<BaseFile> getUnderlyingFile() { return content; }

  inline void incRef() { meta.st_nlink++; }

  inline void decRef() { meta.st_nlink--; }

  inline void setMod(mode_t mode) {
    meta.st_mode = (meta.st_mode & (mode_t)S_IFMT) | (mode & (~S_IFMT));
  }

  inline void setUid(uid_t uid) { meta.st_uid = uid; }

  inline void setGid(gid_t gid) { meta.st_gid = gid; }

  inline INode(const struct stat &meta, std::shared_ptr<BaseFile> content,
               const std::set<std::shared_ptr<BacktrackPos>, compareBTPosPtr>
                   *pbtpos = nullptr)
      : meta(meta), content(std::move(content)) {
    if (pbtpos) {
      btpos = *pbtpos;
    }
  }

  ssize_t write(const void *buf, size_t nbyte, size_t off,
                       std::error_code &err);
  ssize_t read(void *buf, size_t nbyte, size_t off,
                      std::error_code &err);
};
} // namespace accmut

#endif // LLVM_INODE_H
