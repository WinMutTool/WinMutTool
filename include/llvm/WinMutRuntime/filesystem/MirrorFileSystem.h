#ifndef LLVM_MIRRORFILESYSTEM_H
#define LLVM_MIRRORFILESYSTEM_H

#include <fcntl.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/INode.h>
#include <llvm/WinMutRuntime/filesystem/Path.h>
#include <llvm/WinMutRuntime/mutations/MutationIDDecl.h>
#include <llvm/WinMutRuntime/protocol/protocol.h>
#include <map>
#include <memory>
#include <system_error>

namespace accmut {
class MirrorFileSystem {
  blksize_t blksize = 0;
  int pipefd = -1;
  bool is_real = true;

  std::map<ino_t, std::shared_ptr<INode>> real_mapping;
  std::map<ino_t, std::shared_ptr<INode>> fake_mapping;
  // Path cwd;
  ino_t cwdino;
  ino_t rootino;
  dev_t rootdev;

  Path realPathOfDir(ino_t ino, std::error_code &err);

  ino_t cache(ino_t base, const Path &path, std::error_code &err);

  void dump(FILE *f, ino_t ino, bool is_real = true);

  MirrorFileSystem();

  void unlink_precheck(ino_t baseino, const Path &path, Path &rpath,
                       std::shared_ptr<INode> &ptr,
                       std::shared_ptr<INode> &fatherptr, std::error_code &err);

  void unlink_precheck(ino_t baseino, const Path &path, std::error_code &err);

  void unlink_check(ino_t baseino, const Path &path, ino_t father_ino,
                    ino_t this_ino, const std::string &filename,
                    std::error_code &errori, int check_mode);

  void unlink_check(ino_t baseino, const Path &path, std::error_code err);

  void newreg_check_fail(ino_t ino, const Path &path, mode_t mode,
                         std::error_code err);

  void rmdir_precheck(ino_t baseino, const Path &path, Path &rpath,
                      std::shared_ptr<INode> &fatherptr, std::error_code &err);

  void rmdir_precheck(ino_t baseino, const Path &path, std::error_code &err);

  void rmdir_check(ino_t baseino, const Path &path, ino_t ino,
                   const std::string &filename, std::error_code &errori,
                   int check_mode);

  void rmdir_check(ino_t baseino, const Path &path, std::error_code err);

  void newdir_check_fail(ino_t ino, const Path &path, mode_t mode,
                         std::error_code err);

  void newsymlink_check_fail(ino_t ino, const std::string &str,
                             const Path &path, std::error_code err);

  void rename_check(ino_t ino1, const Path &path, ino_t ino2, const Path &path1,
                    std::error_code err);

  static MirrorFileSystem *holdptr;

  int openDirByIno(ino_t ino);

  std::shared_ptr<INode> extractExistingINode(ino_t ino);

  std::shared_ptr<INode> findINode(const Path &path, int checkmode,
                                   bool follow_symlink, Path *real_relapath,
                                   std::error_code &err);

  std::shared_ptr<INode> findINode(ino_t ino, const Path &path, int checkmode,
                                   bool follow_symlink, Path *real_relapath,
                                   std::error_code &err);

  std::shared_ptr<INode> findINodeProc(const Path &path, int checkmode,
                                       std::error_code &err);

public:
  static MirrorFileSystem *getInstance();

  static void hold();

  static void release();

  ino_t query(const Path &path, int checkmode, bool follow_symlink,
              Path *real_relapath, std::error_code &err);

  ino_t query(ino_t ino, const Path &path, int checkmode, bool follow_symlink,
              Path *real_relapath, std::error_code &err);

  ino_t queryProc(const Path &path, int checkmode, std::error_code &err);

  inline ino_t getCwdIno() { return cwdino; }

  inline ino_t getRootIno() { return rootino; }

  void chdir(ino_t ino, std::error_code &err);

  Path getwd(std::error_code &err);

  void unlink(const Path &path, std::error_code &err);

  void unlink(ino_t ino, const Path &path, std::error_code &err);

  ino_t newRegularFile(const Path &path, mode_t mode, std::error_code &err,
                       int additionalFlags = 0, int *fdp = nullptr);

  ino_t newRegularFile(ino_t ino, const Path &path, mode_t mode,
                       std::error_code &err, int additionalFlags = 0,
                       int *fdp = nullptr);

  void rmdir(const Path &path, std::error_code &err);

  void rmdir(ino_t ino, const Path &path, std::error_code &err);

  void remove(const Path &path, std::error_code &err);

  std::shared_ptr<INode> newDirectory(const Path &path, mode_t mode,
                                      std::error_code &err);

  std::shared_ptr<INode> newDirectory(ino_t ino, const Path &path, mode_t mode,
                                      std::error_code &err);

  std::shared_ptr<INode> newSymlink(const std::string &content,
                                    const Path &path, std::error_code &err);

  std::shared_ptr<INode> newSymlink(const std::string &content, ino_t ino,
                                    const Path &path, std::error_code &err);

  void chmod(ino_t ino, mode_t mode, std::error_code &err);

  void rename(const Path &path, const Path &path1, std::error_code &err);

  void rename(ino_t ino, const Path &path, const Path &path1,
              std::error_code &err);

  void rename(const Path &path, ino_t ino1, const Path &path1,
              std::error_code &err);

  void rename(ino_t ino, const Path &path, ino_t ino1, const Path &path1,
              std::error_code &err);

  // delegated functions
  struct stat getStat(ino_t ino);
  void trunc(ino_t ino);
  bool isDir(ino_t ino);
  bool isLnk(ino_t ino);
  bool isReg(ino_t ino);
  bool isCharSpecial(ino_t ino);
  bool isPipe(ino_t ino);
  bool canRead(ino_t ino);
  bool canWrite(ino_t ino);
  bool canExecute(ino_t ino);
  void incRef(ino_t ino);
  void decRef(ino_t ino);
  void setMod(ino_t ino, mode_t mode);
  void setUid(ino_t ino, uid_t uid);
  void setGid(ino_t ino, gid_t gid);
  ssize_t read(ino_t ino, void *buf, size_t nbyte, size_t off,
               std::error_code &err);
  ssize_t write(ino_t ino, const void *buf, size_t nbyte, size_t off,
                std::error_code &err);
  std::shared_ptr<BacktrackPos> getBTPos(ino_t ino, ino_t fatherIno,
                                         const std::string &name);
  std::vector<struct dirent> getDirentList(ino_t ino);

  Path bt2path(std::shared_ptr<BacktrackPos> bt, std::error_code &err);

  static inline bool isReal() { return MUTATION_ID == 0; }

  inline void setVirtual() { is_real = false; }

  inline void setPipeFd(int fd) { pipefd = fd; }

  inline void precache(const char *path) {
    std::error_code err;
    cache(rootino, Path(path), err);
  }

  inline void sendPanicMsg(const char *path) {
    if (pipefd != -1) {
      MessageHdr hdr;
      hdr.type = MessageHdr::PANIC;
      hdr.length = strlen(path);
      __accmut_libc_write(pipefd, &hdr, sizeof(MessageHdr));
      __accmut_libc_write(pipefd, path, hdr.length);
    }
  }
};
} // namespace accmut

#endif // LLVM_MIRRORFILESYSTEM_H
