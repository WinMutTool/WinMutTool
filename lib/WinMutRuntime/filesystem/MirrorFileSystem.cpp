#include <dirent.h>
#include <fcntl.h>
#include <llvm/WinMutRuntime/checking/check.h>
#include <llvm/WinMutRuntime/checking/panic.h>
#include <llvm/WinMutRuntime/filesystem/ErrorCode.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/DirectoryFile.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/RegularFile.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/SymbolicLinkFile.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/UnsupportedFile.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <llvm/WinMutRuntime/filesystem/MirrorFileSystem.h>
#include <llvm/WinMutRuntime/sysdep/macros.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sysmacros.h>

namespace accmut {
class PermissionExemptObject {
  static int pending_num;

public:
  PermissionExemptObject() {
	  /*
    if (pending_num == 0) {
      errno = 0;
      if (seteuid(0) < 0) {
      }
    }
    ++pending_num;
*/
  }

  ~PermissionExemptObject() {
	  /*
    --pending_num;
    if (pending_num == 0) {
      errno = 0;
      if (seteuid(getuid())) {
      }
    }
    */
  }
};

int PermissionExemptObject::pending_num = 0;

MirrorFileSystem *MirrorFileSystem::holdptr = nullptr;

MirrorFileSystem *MirrorFileSystem::getInstance() {
  if (likely(holdptr != nullptr)) {
    return holdptr;
  }
  holdptr = new MirrorFileSystem();
  return holdptr;
}

void MirrorFileSystem::hold() { holdptr = getInstance(); }

void MirrorFileSystem::release() { holdptr = nullptr; }

MirrorFileSystem::MirrorFileSystem() {
  PermissionExemptObject peo;
  struct stat st;
  if (__accmut_libc_lstat("/", &st) < 0) {
    panic("Failed to initialize mirror file system: root");
  }
  DIR *root = __accmut_libc_opendir("/");
  if (!root) {
    panic("Failed to initialize mirror file system: root");
  }
  std::list<dirent> dlist;
  struct dirent *pdir;
  while ((pdir = __accmut_libc_readdir(root)) != nullptr) {
    dlist.push_back(*pdir);
  }
  __accmut_libc_closedir(root);
  auto pdf = std::make_shared<DirectoryFile>(dlist);
  auto pdf1 = std::make_shared<DirectoryFile>(std::move(dlist));
  auto pinode = std::make_shared<INode>(st, pdf);
  auto pinode1 = std::make_shared<INode>(st, pdf1);
  real_mapping[st.st_ino] = pinode;
  fake_mapping[st.st_ino] = pinode1;
  pinode->addBTPos(st.st_ino, "/");
  pinode1->addBTPos(st.st_ino, "/");
  rootino = st.st_ino;
  rootdev = st.st_dev;

  char buf[MAXPATHLEN];
  if (!__accmut_libc_getcwd(buf, MAXPATHLEN))
    panic("Failed to initialize mirror file system: current working directory");
  struct stat cwdst;
  if (__accmut_libc_stat(buf, &cwdst) < 0) {
    panic("Failed to initialize mirror file system: current working directory "
          "stat");
  }
  cwdino = cwdst.st_ino;
  blksize = cwdst.st_blksize;

  std::error_code err;
  findINode(Path(buf), X_OK, true, nullptr, err);
  if (err) {
    panic("Failed to initialize mirror file system: current working directory "
          "realpath");
  }
}

Path MirrorFileSystem::realPathOfDir(ino_t ino, std::error_code &err) {
  err.clear();
  if (ino == rootino)
    return Path("/");
  ino_t nxtino = ino;
  std::deque<std::string> dq;
  int num = 0;
  while (true) {
    if (++num == 500)
      panic("Too deep directory");
    auto iter = fake_mapping.find(nxtino);
    if (iter == fake_mapping.end()) {
      if (nxtino == ino)
        err = make_system_error_code(std::errc::no_such_file_or_directory);
      else
        panic("Failed to cache the whole tree");
      return Path(".");
    }
    if (!iter->second->isDir()) {
      err = make_system_error_code(std::errc::not_a_directory);
      return Path(".");
    }
    if (iter->second->getBTPosSet().size() != 1)
      panic("Directories should have exactly one father");
    auto p = *(iter->second->getBTPosSet().begin());
    dq.push_front(p->name);
    nxtino = p->ino;
    if (nxtino == rootino) {
      // found root
      return Path::fromDeque(false, std::move(dq), false);
    }
  }
}

ino_t MirrorFileSystem::cache(ino_t base, const Path &path,
                              std::error_code &err) {
  PermissionExemptObject peo;
  err.clear();
  // Path toPath = cwd + path;
  Path baseDir = realPathOfDir(base, err);
  if (err.value() != 0)
    return 0;
  Path toPath = baseDir + path;
  const char *cpath = toPath.c_str();
  struct stat st;
  if (__accmut_libc_lstat(cpath, &st) < 0) {
    err = make_system_error_code(std::errc::no_such_file_or_directory);
    return 0;
  }
  if (major(st.st_dev) == 0 && st.st_dev != rootdev)
    panic("Cannot handle no device files");
  auto iter = real_mapping.find(st.st_ino);
  bool found = iter != real_mapping.end();
  if (found) {
    if (iter->second->cached()) {
      if (!iter->second->isDir()) {
        ino_t fatherino = cache(rootino, toPath.baseDir(), err);
        if (err.value() != 0)
          return 0;
        real_mapping[st.st_ino]->addBTPos(fatherino, toPath.lastName());
        fake_mapping[st.st_ino]->addBTPos(fatherino, toPath.lastName());
      }
      return st.st_ino;
    }
    // found but no perm
    if (!check_read_perm(st))
      return st.st_ino;
  } else if (!check_read_perm(st)) {
    if (pipefd) {
      MessageHdr hdr;
      hdr.type = MessageHdr::CACHE;
      hdr.length = strlen(cpath);
      __accmut_libc_write(pipefd, &hdr, sizeof(MessageHdr));
      __accmut_libc_write(pipefd, cpath, hdr.length);
    }
    ino_t fatherino = cache(rootino, toPath.baseDir(), err);
    if (err.value() != 0)
      return 0;
    if (S_ISLNK(st.st_mode)) {
      panic("No read perm for link");
    } else if (S_ISDIR(st.st_mode)) {
      if (toPath.lastName() == ".." || toPath.lastName() == ".")
        panic("Should not end with ../.");

      fake_mapping[st.st_ino] =
          std::make_shared<INode>(st, std::make_shared<BaseDirectoryFile>());
      real_mapping[st.st_ino] =
          std::make_shared<INode>(st, std::make_shared<BaseDirectoryFile>());
    } else if (S_ISREG(st.st_mode)) {
      real_mapping[st.st_ino] = std::make_shared<INode>(st, nullptr);
      fake_mapping[st.st_ino] = std::make_shared<INode>(st, nullptr);
    } else {
      real_mapping[st.st_ino] =
          std::make_shared<INode>(st, std::make_shared<UnsupportedFile>());
      fake_mapping[st.st_ino] =
          std::make_shared<INode>(st, std::make_shared<UnsupportedFile>());
    }

    real_mapping[st.st_ino]->addBTPos(fatherino, toPath.lastName());
    fake_mapping[st.st_ino]->addBTPos(fatherino, toPath.lastName());
    if (S_ISDIR(st.st_mode)) {
      if (real_mapping[st.st_ino]->getBTPosSet().size() != 1)
        panic("should be exactly one");
      if (fake_mapping[st.st_ino]->getBTPosSet().size() != 1)
        panic("should be exactly one");
    }
    return st.st_ino;
  }
  if (pipefd) {
    MessageHdr hdr;
    hdr.type = MessageHdr::CACHE;
    hdr.length = strlen(cpath);
    __accmut_libc_write(pipefd, &hdr, sizeof(MessageHdr));
    __accmut_libc_write(pipefd, cpath, hdr.length);
  }
  ino_t fatherino = cache(rootino, toPath.baseDir(), err);
  if (err.value() != 0)
    return 0;

  // not found or not cached, but can read
  if (S_ISDIR(st.st_mode)) {
    // for BaseDirectoryFile
    if (check_read_perm(st)) {
      if (toPath.lastName() == ".." || toPath.lastName() == ".")
        panic("Should not end with ../.");

      DIR *dir = __accmut_libc_opendir(cpath);
      if (!dir) {
        panic("Failed to open");
      }
      std::list<dirent> dlist;
      struct dirent *pdir;
      while ((pdir = __accmut_libc_readdir(dir)) != nullptr) {
        dlist.push_back(*pdir);
      }
      __accmut_libc_closedir(dir);

      auto pdf = std::make_shared<DirectoryFile>(dlist);
      // fake should check delete
      if (found) {
        real_mapping[st.st_ino] = std::make_shared<INode>(
            st, pdf, &real_mapping[st.st_ino]->getBTPosSet());
        auto fake_iter = fake_mapping.find(st.st_ino);
        if (fake_iter == fake_mapping.end())
          panic("Failed to find in fake map");
        auto pbdf = std::static_pointer_cast<BaseDirectoryFile>(
            fake_iter->second->getUnderlyingFile());
        for (auto b = pbdf->deletedBegin(); b != pbdf->deletedEnd(); ++b) {
          auto niter =
              std::find_if(dlist.begin(), dlist.end(), [&b](const dirent &d) {
                return strcmp(d.d_name, b->c_str()) == 0;
              });
          if (niter != dlist.end())
            dlist.erase(niter);
        }
        for (auto b = pbdf->newBegin(); b != pbdf->newEnd(); ++b) {
          auto diter =
              std::find_if(dlist.begin(), dlist.end(), [&b](const dirent &d) {
                return strcmp(d.d_name, b->d_name) == 0;
              });
          if (diter != dlist.end())
            dlist.erase(diter);
          dlist.push_back(*b);
        }
        auto pdf1 = std::make_shared<DirectoryFile>(std::move(dlist));
        fake_mapping[st.st_ino] =
            std::make_shared<INode>(fake_mapping[st.st_ino]->getStat(), pdf1,
                                    &fake_mapping[st.st_ino]->getBTPosSet());
        // should already be linked
        if (S_ISDIR(st.st_mode)) {
          if (real_mapping[st.st_ino]->getBTPosSet().size() != 1)
            panic("should be exactly one");
          if (fake_mapping[st.st_ino]->getBTPosSet().size() != 1)
            panic("should be exactly one");
        }
        return st.st_ino;
      } else {
        auto pdf1 = std::make_shared<DirectoryFile>(std::move(dlist));
        real_mapping[st.st_ino] = std::make_shared<INode>(st, pdf);
        fake_mapping[st.st_ino] = std::make_shared<INode>(st, pdf1);
      }
    }
  } else if (S_ISREG(st.st_mode)) {
    if (check_read_perm(st)) {
      int fd = __accmut_libc_open(cpath, O_RDONLY);
      if (!fd)
        panic("Failed to open");
      auto buf = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      if (!buf)
        panic("Failed to mmap");
      std::vector<char> tmp;
      tmp.resize(st.st_size + 10);
      memcpy(tmp.data(), buf, st.st_size);
      if (__accmut_libc_close(fd) < 0)
        panic("Failed to close");
      auto prf = std::make_shared<RegularFile>(tmp, st.st_size);
      auto prf1 = std::make_shared<RegularFile>(std::move(tmp), st.st_size);
      if (found) {
        real_mapping[st.st_ino] = std::make_shared<INode>(
            st, prf1, &real_mapping[st.st_ino]->getBTPosSet());
        fake_mapping[st.st_ino] =
            std::make_shared<INode>(fake_mapping[st.st_ino]->getStat(), prf,
                                    &fake_mapping[st.st_ino]->getBTPosSet());
        // should already be linked
        if (S_ISDIR(st.st_mode)) {
          if (real_mapping[st.st_ino]->getBTPosSet().size() != 1)
            panic("should be exactly one");
          if (fake_mapping[st.st_ino]->getBTPosSet().size() != 1)
            panic("should be exactly one");
        }
        return st.st_ino;
      } else {
        real_mapping[st.st_ino] = std::make_shared<INode>(st, prf1);
        fake_mapping[st.st_ino] = std::make_shared<INode>(st, prf);
      }
    }
  } else if (S_ISLNK(st.st_mode)) {
    char buf[MAXPATHLEN];
    ssize_t n = __accmut_libc_readlink(cpath, buf, MAXPATHLEN);
    if (n == -1) {
      panic("Failed to open");
    }
    buf[n] = 0;

    real_mapping[st.st_ino] =
        std::make_shared<INode>(st, std::make_shared<SymbolicLinkFile>(buf));
    fake_mapping[st.st_ino] =
        std::make_shared<INode>(st, std::make_shared<SymbolicLinkFile>(buf));
  } else {
    real_mapping[st.st_ino] =
        std::make_shared<INode>(st, std::make_shared<UnsupportedFile>());
    fake_mapping[st.st_ino] =
        std::make_shared<INode>(st, std::make_shared<UnsupportedFile>());
  }
  // only not found can get here
  // fake and real are the same
  real_mapping[st.st_ino]->addBTPos(fatherino, toPath.lastName());
  fake_mapping[st.st_ino]->addBTPos(fatherino, toPath.lastName());
  if (S_ISDIR(st.st_mode)) {
    if (real_mapping[st.st_ino]->getBTPosSet().size() != 1)
      panic("should be exactly one");
    if (fake_mapping[st.st_ino]->getBTPosSet().size() != 1)
      panic("should be exactly one");
  }
  return st.st_ino;
}

void MirrorFileSystem::dump(FILE *f, ino_t ino, bool is_real) {
  fprintf(f, "DUMP: %lu\n", ino);
  if (is_real) {
    auto iter = real_mapping.find(ino);
    if (iter == real_mapping.end()) {
      fprintf(f, "NOT FOUND");
      return;
    }
    iter->second->dump(f);
  } else {
    auto iter = fake_mapping.find(ino);
    if (iter == fake_mapping.end()) {
      fprintf(f, "NOT FOUND");
      return;
    }
    iter->second->dump(f);
  }
}

std::shared_ptr<INode> MirrorFileSystem::findINodeProc(const Path &path,
                                                       int checkmode,
                                                       std::error_code &err) {
  PermissionExemptObject peo;
  err.clear();
  const char *cpath = path.c_str();
  struct stat st;
  if (__accmut_libc_stat(cpath, &st) < 0) {
    err = make_system_error_code(std::errc::no_such_file_or_directory);
    return nullptr;
  }
  if (major(st.st_dev) == 0 && st.st_dev != rootdev)
    panic("Cannot handle no device files");
  auto iter = real_mapping.find(st.st_ino);
  bool found = iter != real_mapping.end();
  if (found) {
    if (iter->second->cached()) {
      return fake_mapping[st.st_ino];
    }
    // found but no perm
    if (!check_read_perm(st))
      return fake_mapping[st.st_ino];
  } else if (!check_read_perm(st)) {
    if (S_ISLNK(st.st_mode)) {
      panic("No read perm for link");
    } else if (S_ISDIR(st.st_mode)) {
      fake_mapping[st.st_ino] =
          std::make_shared<INode>(st, std::make_shared<BaseDirectoryFile>());
      real_mapping[st.st_ino] =
          std::make_shared<INode>(st, std::make_shared<BaseDirectoryFile>());
    } else if (S_ISREG(st.st_mode)) {
      real_mapping[st.st_ino] = std::make_shared<INode>(st, nullptr);
      fake_mapping[st.st_ino] = std::make_shared<INode>(st, nullptr);
    } else {
      real_mapping[st.st_ino] =
          std::make_shared<INode>(st, std::make_shared<UnsupportedFile>());
      fake_mapping[st.st_ino] =
          std::make_shared<INode>(st, std::make_shared<UnsupportedFile>());
    }
    return fake_mapping[st.st_ino];
  }
  // not found or not cached, but can read
  if (S_ISDIR(st.st_mode)) {
    // for BaseDirectoryFile
    if (check_read_perm(st)) {
      DIR *dir = __accmut_libc_opendir(cpath);
      if (!dir) {
        panic("Failed to open");
      }
      std::list<dirent> dlist;
      struct dirent *pdir;
      while ((pdir = __accmut_libc_readdir(dir)) != nullptr) {
        dlist.push_back(*pdir);
      }
      __accmut_libc_closedir(dir);

      auto pdf = std::make_shared<DirectoryFile>(dlist);
      // fake should check delete
      if (found) {
        real_mapping[st.st_ino] = std::make_shared<INode>(
            st, pdf, &real_mapping[st.st_ino]->getBTPosSet());
        auto fake_iter = fake_mapping.find(st.st_ino);
        if (fake_iter == fake_mapping.end())
          panic("Failed to find in fake map");
        auto pbdf = std::static_pointer_cast<BaseDirectoryFile>(
            fake_iter->second->getUnderlyingFile());
        for (auto b = pbdf->deletedBegin(); b != pbdf->deletedEnd(); ++b) {
          auto niter =
              std::find_if(dlist.begin(), dlist.end(), [&b](const dirent &d) {
                return strcmp(d.d_name, b->c_str()) == 0;
              });
          if (niter != dlist.end())
            dlist.erase(niter);
        }
        for (auto b = pbdf->newBegin(); b != pbdf->newEnd(); ++b) {
          auto diter =
              std::find_if(dlist.begin(), dlist.end(), [&b](const dirent &d) {
                return strcmp(d.d_name, b->d_name) == 0;
              });
          if (diter != dlist.end())
            dlist.erase(diter);
          dlist.push_back(*b);
        }
        auto pdf1 = std::make_shared<DirectoryFile>(std::move(dlist));
        fake_mapping[st.st_ino] =
            std::make_shared<INode>(fake_mapping[st.st_ino]->getStat(), pdf1,
                                    &fake_mapping[st.st_ino]->getBTPosSet());
      } else {
        auto pdf1 = std::make_shared<DirectoryFile>(std::move(dlist));
        real_mapping[st.st_ino] = std::make_shared<INode>(st, pdf);
        fake_mapping[st.st_ino] = std::make_shared<INode>(st, pdf1);
      }
    }
  } else if (S_ISREG(st.st_mode)) {
    if (check_read_perm(st)) {
      int fd = __accmut_libc_open(cpath, O_RDONLY);
      if (!fd)
        panic("Failed to open");
      auto buf = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      if (!buf)
        panic("Failed to mmap");
      std::vector<char> tmp;
      tmp.resize(st.st_size + 10);
      memcpy(tmp.data(), buf, st.st_size);
      if (__accmut_libc_close(fd) < 0)
        panic("Failed to close");
      auto prf = std::make_shared<RegularFile>(tmp, st.st_size);
      auto prf1 = std::make_shared<RegularFile>(std::move(tmp), st.st_size);
      if (found) {
        real_mapping[st.st_ino] = std::make_shared<INode>(
            st, prf1, &real_mapping[st.st_ino]->getBTPosSet());
        fake_mapping[st.st_ino] =
            std::make_shared<INode>(fake_mapping[st.st_ino]->getStat(), prf,
                                    &fake_mapping[st.st_ino]->getBTPosSet());
      } else {
        real_mapping[st.st_ino] = std::make_shared<INode>(st, prf1);
        fake_mapping[st.st_ino] = std::make_shared<INode>(st, prf);
      }
    }
  } else if (S_ISLNK(st.st_mode)) {
    char buf[MAXPATHLEN];
    ssize_t n = __accmut_libc_readlink(cpath, buf, MAXPATHLEN);
    if (n == -1) {
      panic("Failed to open");
    }
    buf[n] = 0;
    real_mapping[st.st_ino] =
        std::make_shared<INode>(st, std::make_shared<SymbolicLinkFile>(buf));
    fake_mapping[st.st_ino] =
        std::make_shared<INode>(st, std::make_shared<SymbolicLinkFile>(buf));
  } else {
    real_mapping[st.st_ino] =
        std::make_shared<INode>(st, std::make_shared<UnsupportedFile>());
    fake_mapping[st.st_ino] =
        std::make_shared<INode>(st, std::make_shared<UnsupportedFile>());
  }
  return fake_mapping[st.st_ino];
}

ino_t MirrorFileSystem::queryProc(const Path &path, int checkmode,
                                  std::error_code &err) {
  auto inode = findINodeProc(path, checkmode, err);
  if (!inode)
    return 0;
  return inode->getIno();
}

std::shared_ptr<INode> MirrorFileSystem::findINode(ino_t ino, const Path &path,
                                                   int checkmode,
                                                   bool follow_symlink,
                                                   Path *real_relapath,
                                                   std::error_code &err) {
  err.clear();
  std::deque<std::string> deque = path.getDeque();
  for (int i = 0; i < path.getDdnum(); ++i) {
    deque.push_front("..");
  }
  if (path.isEndWithSlash()) {
    follow_symlink = true;
    // deque.push_back(".");
  }
  int follownum = 0;
  std::vector<ino_t> lastbasestack;
  if (path.isRelative()) {
    lastbasestack.push_back(ino);
  } else {
    lastbasestack.push_back(rootino);
  }
  if (follow_symlink)
    deque.push_back(".");
  std::deque<std::string> realpath_deque;
  while (!deque.empty()) {
    std::string str = deque.front();
    deque.pop_front();
    auto iter = fake_mapping.find(lastbasestack.back());
    if (iter == fake_mapping.end())
      panic((std::string(path.c_str()) + "Base not found").c_str());
    auto inodeptr = iter->second;
    if (!inodeptr->cached()) {
      // read it out
      if (inodeptr->isDir()) {
        // try to bypass it
        if (inodeptr->canExecute()) {
          auto pbdf = std::static_pointer_cast<BaseDirectoryFile>(
              inodeptr->getUnderlyingFile());
          if (!pbdf)
            panic("Should be base directory file");
          auto niter = pbdf->newFind(str);
          if (niter != pbdf->newEnd()) {
            ino_t nxtino = niter->d_ino;
            lastbasestack.push_back(nxtino);
            if (real_relapath)
              realpath_deque.push_back(std::move(str));
            continue;
          }
          auto diter = pbdf->deletedFind(str);
          if (diter != pbdf->deletedEnd()) {
            // this time there's no new file
            err = make_system_error_code(std::errc::no_such_file_or_directory);
            return nullptr;
          }

          ino_t nxtino = cache(lastbasestack.back(), Path(str), err);
          if (err.value() != 0)
            return nullptr;
          lastbasestack.push_back(nxtino);
          if (real_relapath)
            realpath_deque.push_back(std::move(str));
          continue;
        } else if (deque.empty() && follow_symlink) {
          break;
        }
      } else if (inodeptr->isLnk()) {
        panic("Link must be cached");
      } else if (deque.empty() && follow_symlink) {
        if (path.isEndWithSlash()) {
          err = make_system_error_code(std::errc::not_a_directory);
          return nullptr;
        }
        break;
      } else {
        err = make_system_error_code(std::errc::not_a_directory);
        return nullptr;
      }
      err = make_system_error_code(std::errc::permission_denied);
      return nullptr;
    }
    if (inodeptr->isDir()) {
      if (deque.empty() && follow_symlink)
        break;
      ino_t foundino;
      if (!iter->second->canExecute()) {
        err = make_system_error_code(std::errc::permission_denied);
        return nullptr;
      }
      auto dirfile = std::static_pointer_cast<DirectoryFile>(
          inodeptr->getUnderlyingFile());
      for (auto &d : *dirfile) {
        if (strcmp(d.d_name, str.c_str()) == 0) {
          foundino = d.d_ino;
          goto ok;
        }
      }
      err = make_system_error_code(std::errc::no_such_file_or_directory);
      return nullptr;
    ok:
      /* ino_t cached_ino = */ cache(lastbasestack.back(), Path(str), err);
      auto fake_iter = fake_mapping.find(foundino);
      if (fake_iter == fake_mapping.end() || !fake_iter->second->cached()) {
        if (!(deque.size() == 1 && follow_symlink)) {
          if (err.value() != 0)
            return nullptr;
        }
      }
      err.clear();
      lastbasestack.push_back(foundino);
      if (real_relapath)
        realpath_deque.push_back(std::move(str));
      continue;
    } else if (inodeptr->isLnk()) {
      if (++follownum == 100) {
        err = make_system_error_code(std::errc::too_many_symbolic_link_levels);
        return nullptr;
      }
      auto linkto = std::static_pointer_cast<SymbolicLinkFile>(
                        inodeptr->getUnderlyingFile())
                        ->getTarget();
      Path p = Path(linkto, false);
      deque.push_front(str);
      auto dp = p.getDeque();
      for (auto rb = dp.rbegin(); rb != dp.rend(); ++rb) {
        deque.push_front(*rb);
      }
      for (int i = 0; i < p.getDdnum(); ++i) {
        deque.push_front("..");
      }
      if (p.isRelative()) {
        if (real_relapath)
          realpath_deque.pop_back();
        lastbasestack.pop_back();
      } else {
        if (real_relapath)
          realpath_deque.clear();
        lastbasestack.clear();
        lastbasestack.push_back(rootino);
      }
      continue;
    } else if (deque.empty() && follow_symlink) {
      if (path.isEndWithSlash()) {
        err = make_system_error_code(std::errc::not_a_directory);
        return nullptr;
      }
      if (!inodeptr->isReg())
        panic("Not implemented");
      break;
    } else {
      err = make_system_error_code(std::errc::not_a_directory);
      return nullptr;
    }
    err = make_system_error_code(std::errc::no_such_file_or_directory);
    return nullptr;
  }
  auto iter = fake_mapping.find(lastbasestack.back());
  if (iter == fake_mapping.end()) {
    panic("Failing to cache");
  }
  if ((checkmode & R_OK) && !iter->second->canRead())
    goto noaccess;
  if ((checkmode & W_OK) && !iter->second->canWrite())
    goto noaccess;
  if ((checkmode & X_OK) && !iter->second->canExecute())
    goto noaccess;
  if (real_relapath) {
    Path p = Path::fromDeque(true, realpath_deque);
    p.c_str();
    Path realpath = realPathOfDir(lastbasestack.front(), err) + p;
    realpath.c_str();
    if (err.value() != 0)
      panic("Should not fail");
    *real_relapath = std::move(realpath);
  }
  return iter->second;
noaccess:
  err = make_system_error_code(std::errc::permission_denied);
  return nullptr;
}

ino_t MirrorFileSystem::query(ino_t ino, const Path &path, int checkmode,
                              bool follow_symlink, Path *real_relapath,
                              std::error_code &err) {
  auto inode =
      findINode(ino, path, checkmode, follow_symlink, real_relapath, err);
  if (!inode)
    return 0;
  return inode->getIno();
}

std::shared_ptr<INode> MirrorFileSystem::findINode(const Path &path,
                                                   int checkmode,
                                                   bool follow_symlink,
                                                   Path *real_relapath,
                                                   std::error_code &err) {
  return findINode(cwdino, path, checkmode, follow_symlink, real_relapath, err);
}

ino_t MirrorFileSystem::query(const Path &path, int checkmode,
                              bool follow_symlink, Path *real_relapath,
                              std::error_code &err) {
  return query(cwdino, path, checkmode, follow_symlink, real_relapath, err);
}

int MirrorFileSystem::openDirByIno(ino_t ino) {
  PermissionExemptObject peo;
  if (!isReal())
    panic("Should only be called by father");
  std::error_code err;
  Path p = realPathOfDir(ino, err);
  if (err.value() != 0)
    panic("Should not fail");
  errno = 0;
  int fd = __accmut_libc_open(p.c_str(), O_RDONLY);
  if (fd < 0)
    panic("Should not fail");
  int newfd = __accmut_libc_dup(fd);
  __accmut_libc_close(fd);
  return newfd;
}

Path MirrorFileSystem::getwd(std::error_code &err) {
  err.clear();
  char buff[MAXPATHLEN];
  char *oriret = only_origin(__accmut_libc_getcwd(buff, MAXPATHLEN));
  Path ret = realPathOfDir(cwdino, err);
  if (err.value() != 0) {
    if (oriret == nullptr) {
    }
    panic("Should not fail");
  }
  check_streq(buff, ret.c_str());
  return ret;
}

void MirrorFileSystem::chdir(ino_t ino, std::error_code &err) {
  err.clear();
  auto inode = extractExistingINode(ino);
  if (inode->isDir()) {
    if (inode->canExecute()) {
      cwdino = ino;
    } else {
      err = make_system_error_code(std::errc::permission_denied);
    }
  } else {
    err = make_system_error_code(std::errc::not_a_directory);
  }
}

void MirrorFileSystem::unlink_check(
    ino_t baseino, const Path &path, ino_t father_ino, ino_t this_ino,
    const std::string &filename, std::error_code &errori,
    int check_mode /* only useful when errori == 0 */) {
  if (isReal()) {
    if (errori.value() == 0) {
      auto father_iter = real_mapping.find(father_ino);
      if (father_iter == real_mapping.end()) {
        panic("Not found in real");
      }
      auto this_iter = real_mapping.find(this_ino);
      if (this_iter == real_mapping.end()) {
        panic("Not found in real");
      }
      this_iter->second->deleteBTPos(father_ino, path.lastName());
      if (!father_iter->second->isDir())
        panic("real not a dir");
      if (check_mode == 0) {
        if (!father_iter->second->cached())
          panic("Should be cached");
        auto father_file = std::static_pointer_cast<DirectoryFile>(
            father_iter->second->getUnderlyingFile());
        auto iter = father_file->find(filename);
        if (iter == father_file->end()) {
          panic("dirent not found in real");
        }
        father_file->erase(iter);
      } else if (check_mode == 1) {
        if (father_iter->second->cached())
          panic("Should not be cached");
        auto father_file = std::static_pointer_cast<BaseDirectoryFile>(
            father_iter->second->getUnderlyingFile());
        auto diter = father_file->deletedFind(filename);
        if (diter != father_file->deletedEnd())
          panic("Should not be found in deleted list");
        auto niter = father_file->newFind(filename);
        if (niter != father_file->newEnd())
          panic("Should not be found in new list");
        father_file->deletedPushBack(filename);
      } else if (check_mode == 2) {
        if (father_iter->second->cached())
          panic("Should not be cached");
        auto father_file = std::static_pointer_cast<BaseDirectoryFile>(
            father_iter->second->getUnderlyingFile());
        auto niter = father_file->newFind(filename);
        if (niter == father_file->newEnd())
          panic("Should be found in real map");
        father_file->newErase(niter);
        auto diter = father_file->deletedFind(filename);
        if (diter == father_file->deletedEnd())
          father_file->deletedPushBack(filename);
      }
    }
    int fd = openDirByIno(baseino);
    errno = 0;
    int ori_ret = __accmut_libc_unlinkat(fd, path.c_str(), 0);
    int err = errno;
    __accmut_libc_close(fd);
    check_eq(err, errori.value());
    if (errori.value() != 0)
      check_eq(ori_ret, -1);
  }
}

void MirrorFileSystem::unlink_check(ino_t baseino, const Path &path,
                                    std::error_code err) {
  if (err.value() == 0)
    panic("Should supply father_ino and filename");
  unlink_check(baseino, path, 0, 0, "", err, 0);
}

void MirrorFileSystem::unlink_precheck(ino_t baseino, const Path &path,
                                       Path &rpath, std::shared_ptr<INode> &ptr,
                                       std::shared_ptr<INode> &fatherptr,
                                       std::error_code &err) {
  err.clear();
  findINode(baseino, path, F_OK, false, &rpath, err);
  if (err.value() != 0) {
    unlink_check(baseino, path, err);
    return;
  }
  ptr = findINode(baseino, rpath, F_OK, false, nullptr, err);
  if (err.value() != 0)
    panic("No sync");
  if (ptr->isDir()) {
    err = make_system_error_code(APPLE_OR_GLIBC(
        std::errc::operation_not_permitted, std::errc::is_a_directory));
#ifndef __APPLE__
    std::error_code err1;
    auto ptrlinux = findINode(baseino, path.getNoEndWithSlash(), F_OK, false,
                              nullptr, err1);
    if (ptrlinux->isLnk())
      err = make_system_error_code(std::errc::not_a_directory);
#endif
    unlink_check(baseino, path, err);
    return;
  }
  fatherptr =
      findINode(baseino, path.baseDir(), W_OK | X_OK, true, nullptr, err);
  if (err.value() != 0) {
    unlink_check(baseino, path, err);
    return;
  }
}

void MirrorFileSystem::unlink_precheck(ino_t baseino, const Path &path,
                                       std::error_code &err) {
  Path rpath;
  std::shared_ptr<INode> ptr, fatherptr;
  unlink_precheck(baseino, path, rpath, ptr, fatherptr, err);
}

void MirrorFileSystem::unlink(ino_t baseino, const Path &path,
                              std::error_code &err) {
  err.clear();
  Path rpath;
  std::shared_ptr<INode> ptr, fatherptr;
  unlink_precheck(baseino, path, rpath, ptr, fatherptr, err);
  if (err.value() != 0) {
    return;
  }

  if (!fatherptr->isDir())
    panic("Should be directory");
  ptr->deleteBTPos(fatherptr->getIno(), rpath.lastName());
  if (fatherptr->cached()) {
    auto fatherfile =
        std::static_pointer_cast<DirectoryFile>(fatherptr->getUnderlyingFile());
    auto iter = fatherfile->find(rpath.lastName());
    if (iter == fatherfile->end()) {
      panic("Not found in fake");
    }
    fatherfile->erase(iter);
    unlink_check(baseino, path, fatherptr->getIno(), ptr->getIno(),
                 rpath.lastName(), err, 0);
  } else {
    auto fatherfile = std::static_pointer_cast<BaseDirectoryFile>(
        fatherptr->getUnderlyingFile());
    auto iter = fatherfile->newFind(rpath.lastName());
    if (iter == fatherfile->newEnd()) {
      // not found in new
      auto niter = fatherfile->deletedFind(rpath.lastName());
      if (niter != fatherfile->deletedEnd())
        panic("Should not be found in deleted");
      fatherfile->deletedPushBack(rpath.lastName());
      unlink_check(baseino, path, fatherptr->getIno(), ptr->getIno(),
                   rpath.lastName(), err, 1);
    } else {
      // found in new
      fatherfile->newErase(iter);
      unlink_check(baseino, path, fatherptr->getIno(), ptr->getIno(),
                   rpath.lastName(), err, 2);
    }
  }
}

void MirrorFileSystem::unlink(const Path &path, std::error_code &err) {
  unlink(cwdino, path, err);
}

void MirrorFileSystem::newreg_check_fail(ino_t ino, const Path &path,
                                         mode_t mode, std::error_code err) {
  if (err.value() == 0)
    panic("Should only check fail");
  if (isReal()) {
    errno = 0;
    int fd = openDirByIno(ino);
    int ret = __accmut_libc_openat(fd, path.c_str(), O_CREAT | O_EXCL, mode);
    __accmut_libc_close(fd);
    check_eq(errno, err.value());
    check_eq(ret, -1);
  }
}

static ino_t get_fresh_ino() {
  static ino_t ori = 0x6000000000000000;
  return --ori;
}

ino_t MirrorFileSystem::newRegularFile(ino_t ino, const Path &path, mode_t mode,
                                       std::error_code &err,
                                       int additionalFlags, int *fdp) {
  if (path.isEndWithSlash()) {
    panic("Don't pass directory to the function");
  }
  auto r1 = findINode(ino, path, F_OK, false, nullptr, err);
  if (err.value() == 0) {
    err = make_system_error_code(std::errc::file_exists);
    newreg_check_fail(ino, path, mode | additionalFlags, err);
    return 0;
  }
  err.clear();
  auto fakeFatherInode = findINode(ino, path.baseDir().getNoEndWithSlash(),
                                   F_OK, true, nullptr, err);
  if (err.value() != 0) {
    newreg_check_fail(ino, path, mode | additionalFlags, err);
    return 0;
  }

  if (!fakeFatherInode->isDir()) {
    err = make_system_error_code(std::errc::not_a_directory);
    newreg_check_fail(ino, path, mode | additionalFlags, err);
    return 0;
  }
  if (!(fakeFatherInode->canWrite() && fakeFatherInode->canExecute())) {
    err = make_system_error_code(std::errc::permission_denied);
    newreg_check_fail(ino, path, mode | additionalFlags, err);
    return 0;
  }

  ino_t newino;
  if (isReal()) {
    errno = 0;
    int pfd = openDirByIno(ino);
    int fd = __accmut_libc_openat(pfd, path.c_str(),
                                  O_CREAT | O_EXCL | additionalFlags, mode);
    __accmut_libc_close(pfd);
    check_eq(errno, 0);
    struct stat st;
    __accmut_libc_fstat(fd, &st);
    check_eq(errno, 0);
    newino = st.st_ino;
    std::shared_ptr<RegularFile> rf =
        std::make_shared<RegularFile>(std::vector<char>(1), 0);
    std::shared_ptr<RegularFile> rf1 =
        std::make_shared<RegularFile>(std::vector<char>(1), 0);

    real_mapping[newino] = std::make_shared<INode>(st, rf);
    fake_mapping[newino] = std::make_shared<INode>(st, rf1);

    /*
    dirent d{.d_ino = st.st_ino,
#ifdef __APPLE__
             .d_namlen = (uint16_t)strlen(path.lastName().c_str()),
#endif
             .d_type = DT_REG};
             */
    dirent d;
    d.d_ino = st.st_ino;
    d.d_type = DT_REG;
#ifdef __APPLE__
    d.d_namlen = (uint16_t)strlen(path.lastName().c_str());
#endif

    strcpy(d.d_name, path.lastName().c_str());
    auto realFatherInodeIter = real_mapping.find(fakeFatherInode->getIno());
    if (realFatherInodeIter == real_mapping.end())
      panic("No sync");
    auto realFatherInode = realFatherInodeIter->second;
    if (!realFatherInode->isDir())
      panic("No sync");
    if (!realFatherInode->canWrite() || !realFatherInode->canExecute())
      panic("No sync");

    if (fakeFatherInode->cached()) {
      if (!realFatherInode->cached())
        panic("No sync");
      auto fakeFatherFile = std::static_pointer_cast<DirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      auto realFatherFile = std::static_pointer_cast<DirectoryFile>(
          realFatherInode->getUnderlyingFile());
      fakeFatherFile->push_back(d);
      realFatherFile->push_back(d);
    } else {
      if (realFatherInode->cached())
        panic("No sync");
      auto fakeFatherFile = std::static_pointer_cast<BaseDirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      auto realFatherFile = std::static_pointer_cast<BaseDirectoryFile>(
          realFatherInode->getUnderlyingFile());
      fakeFatherFile->newPushBack(d);
      realFatherFile->newPushBack(d);
    }
    real_mapping[newino]->addBTPos(realFatherInode->getIno(), path.lastName());
    fake_mapping[newino]->addBTPos(fakeFatherInode->getIno(), path.lastName());
    if (!fdp)
      __accmut_libc_close(fd);
    else
      *fdp = fd;
  } else {

    struct stat st;
    st.st_mode = (mode_t)S_IFREG | mode;
    st.st_ino = get_fresh_ino();
    st.st_nlink = 1;
    st.st_uid = getuid();
    st.st_gid = getgid();
    st.st_size = 0;
    st.st_blksize = blksize;
#ifdef __APPLE__
    clock_gettime(CLOCK_REALTIME, &st.st_atimespec);
    st.st_mtimespec = st.st_atimespec;
    st.st_ctimespec = st.st_atimespec;
#else
    clock_gettime(CLOCK_REALTIME, &st.st_atim);
    st.st_mtim = st.st_atim;
    st.st_ctim = st.st_atim;
#endif
    newino = st.st_ino;

    std::shared_ptr<RegularFile> rf =
        std::make_shared<RegularFile>(std::vector<char>(1), 0);
    fake_mapping[newino] = std::make_shared<INode>(st, rf);
    /*
    dirent d{.d_ino = st.st_ino,
#ifdef __APPLE__
             .d_namlen = (uint16_t)strlen(path.lastName().c_str()),
#endif
             .d_type = DT_REG};*/
    dirent d;
    d.d_ino = st.st_ino;
    d.d_type = DT_REG;
#ifdef __APPLE__
    d.d_namlen = (uint16_t)strlen(path.lastName().c_str());
#endif

    strcpy(d.d_name, path.lastName().c_str());

    if (fakeFatherInode->cached()) {
      auto fakeFatherFile = std::static_pointer_cast<DirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      fakeFatherFile->push_back(d);
    } else {
      auto fakeFatherFile = std::static_pointer_cast<BaseDirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      fakeFatherFile->newPushBack(d);
    }
    fake_mapping[newino]->addBTPos(fakeFatherInode->getIno(), path.lastName());
  }
  return newino;
}

ino_t MirrorFileSystem::newRegularFile(const Path &path, mode_t mode,
                                       std::error_code &err,
                                       int additionalFlags, int *fdp) {
  return newRegularFile(cwdino, path, mode, err, additionalFlags, fdp);
}

void MirrorFileSystem::rmdir_check(
    ino_t baseino, const Path &path, ino_t father_ino,
    const std::string &filename, std::error_code &errori,
    int check_mode /* only useful when errori == 0 */) {
  if (isReal()) {
    if (errori.value() == 0) {
      auto father_iter = real_mapping.find(father_ino);
      if (father_iter == real_mapping.end()) {
        panic("Not found in real");
      }
      if (!father_iter->second->isDir())
        panic("real not a dir");
      if (check_mode == 0) {
        if (!father_iter->second->cached())
          panic("Should be cached");
        auto father_file = std::static_pointer_cast<DirectoryFile>(
            father_iter->second->getUnderlyingFile());
        auto iter = father_file->find(filename);
        if (iter == father_file->end()) {
          panic("dirent not found in real");
        }
        father_file->erase(iter);
      } else if (check_mode == 1) {
        if (father_iter->second->cached())
          panic("Should not be cached");
        auto father_file = std::static_pointer_cast<BaseDirectoryFile>(
            father_iter->second->getUnderlyingFile());
        auto diter = father_file->deletedFind(filename);
        if (diter != father_file->deletedEnd())
          panic("Should not be found in deleted list");
        auto niter = father_file->newFind(filename);
        if (niter != father_file->newEnd())
          panic("Should not be found in new list");
        father_file->deletedPushBack(filename);
      } else if (check_mode == 2) {
        if (father_iter->second->cached())
          panic("Should not be cached");
        auto father_file = std::static_pointer_cast<BaseDirectoryFile>(
            father_iter->second->getUnderlyingFile());
        auto niter = father_file->newFind(filename);
        if (niter == father_file->newEnd())
          panic("Should be found in real map");
        father_file->newErase(niter);
        auto diter = father_file->deletedFind(filename);
        if (diter == father_file->deletedEnd())
          father_file->deletedPushBack(filename);
      }
    }
    int fd = openDirByIno(baseino);
    errno = 0;
    int ori_ret = __accmut_libc_unlinkat(fd, path.c_str(), AT_REMOVEDIR);
    int err = errno;
    __accmut_libc_close(fd);
    check_eq(err, errori.value());
    if (errori.value() != 0)
      check_eq(ori_ret, -1);
  }
}

void MirrorFileSystem::rmdir_check(ino_t baseino, const Path &path,
                                   std::error_code err) {
  if (err.value() == 0)
    panic("Should supply father_ino and filename");
  rmdir_check(baseino, path, 0, "", err, 0);
}

void MirrorFileSystem::rmdir_precheck(ino_t baseino, const Path &path,
                                      Path &rpath,
                                      std::shared_ptr<INode> &fatherptr,
                                      std::error_code &err) {
  err.clear();
#ifndef __APPLE__
  if (path.isEndWithSlash()) {
    std::error_code err1;
    auto linuxptr = findINode(baseino, path.getNoEndWithSlash(), F_OK, false,
                              nullptr, err1);
    if (!linuxptr->isDir()) {
      err = make_system_error_code(std::errc::not_a_directory);
      rmdir_check(baseino, path, err);
      return;
    }
  };
#endif
  findINode(baseino, path, F_OK, false, &rpath, err);
  if (err.value() != 0) {
    rmdir_check(baseino, path, err);
    return;
  }
  auto ptr = findINode(baseino, rpath, F_OK, false, nullptr, err);
  if (err.value() != 0)
    panic("No sync");
  if (!ptr->isDir()) {
    err = make_system_error_code(std::errc::not_a_directory);
    rmdir_check(baseino, path, err);
    return;
  }
  if (!ptr->cached()) {
    panic("Not implemented");
  }
  auto nowfile =
      std::static_pointer_cast<DirectoryFile>(ptr->getUnderlyingFile());
  for (auto &d : *nowfile) {
    if (d.d_name[0] != '.')
      goto notempty;
    if (d.d_name[1] == '.') {
      if (d.d_name[2] != 0)
        goto notempty;
    } else if (d.d_name[1] != 0) {
      goto notempty;
    }
    continue;
  notempty:
    err = make_system_error_code(std::errc::directory_not_empty);
    rmdir_check(baseino, path, err);
    return;
  }
  fatherptr = findINode(rpath.baseDir(), W_OK | X_OK, true, nullptr, err);
  if (err.value() != 0) {
    rmdir_check(baseino, path, err);
    return;
  }
}

void MirrorFileSystem::rmdir_precheck(ino_t baseino, const Path &path,
                                      std::error_code &err) {
  Path rpath;
  std::shared_ptr<INode> fatherptr;
  rmdir_precheck(baseino, path, rpath, fatherptr, err);
}

void MirrorFileSystem::rmdir(ino_t baseino, const Path &path,
                             std::error_code &err) {
  err.clear();
  Path bpath = path;
  Path rpath;
  std::shared_ptr<INode> fatherptr;
  rmdir_precheck(baseino, path, rpath, fatherptr, err);
  if (err.value() != 0)
    return;

  if (!fatherptr->isDir())
    panic("Should be directory");
  if (fatherptr->cached()) {
    auto fatherfile =
        std::static_pointer_cast<DirectoryFile>(fatherptr->getUnderlyingFile());
    auto iter = fatherfile->find(rpath.lastName());
    if (iter == fatherfile->end()) {
      panic("Not found in fake");
    }
    fatherfile->erase(iter);
    rmdir_check(baseino, path, fatherptr->getIno(), rpath.lastName(), err, 0);
  } else {
    auto fatherfile = std::static_pointer_cast<BaseDirectoryFile>(
        fatherptr->getUnderlyingFile());
    auto iter = fatherfile->newFind(rpath.lastName());
    if (iter == fatherfile->newEnd()) {
      // not found in new
      auto niter = fatherfile->deletedFind(rpath.lastName());
      if (niter != fatherfile->deletedEnd())
        panic("Should not be found in deleted");
      fatherfile->deletedPushBack(rpath.lastName());
      rmdir_check(baseino, path, fatherptr->getIno(), rpath.lastName(), err, 1);
    } else {
      // found in new
      fatherfile->newErase(iter);
      rmdir_check(baseino, path, fatherptr->getIno(), rpath.lastName(), err, 2);
    }
  }
}

void MirrorFileSystem::rmdir(const Path &path, std::error_code &err) {
  rmdir(cwdino, path, err);
}

void MirrorFileSystem::remove(const Path &path, std::error_code &err) {
  auto ino = findINode(path, F_OK, false, nullptr, err);
  if (!ino || ino->isDir()) {
    return rmdir(path, err);
  } else {
    return unlink(path, err);
  }
}

void MirrorFileSystem::newdir_check_fail(ino_t ino, const Path &path,
                                         mode_t mode, std::error_code err) {
  if (err.value() == 0)
    panic("Should only check fail");
  if (isReal()) {
    errno = 0;
    int fd = openDirByIno(ino);
    int ret = __accmut_libc_mkdirat(fd, path.c_str(), mode);
    __accmut_libc_close(fd);
    check_eq(errno, err.value());
    check_eq(ret, -1);
  }
}

std::shared_ptr<INode> MirrorFileSystem::newDirectory(ino_t ino,
                                                      const Path &path,
                                                      mode_t mode,
                                                      std::error_code &err) {
  auto pnoslash = path.getNoEndWithSlash();
  auto r1 = findINode(ino, pnoslash, F_OK, false, nullptr, err);
  if (err.value() == 0) {
    err = make_system_error_code(std::errc::file_exists);
    newdir_check_fail(ino, path, mode, err);
    return nullptr;
  }
  err.clear();
  auto fakeFatherInode = findINode(ino, path.baseDir().getNoEndWithSlash(),
                                   F_OK, true, nullptr, err);
  if (err.value() != 0) {
    newdir_check_fail(ino, path, mode, err);
    return nullptr;
  }

  if (!fakeFatherInode->isDir()) {
    err = make_system_error_code(std::errc::not_a_directory);
    newdir_check_fail(ino, path, mode, err);
    return nullptr;
  }
  if (!(fakeFatherInode->canWrite() && fakeFatherInode->canExecute())) {
    err = make_system_error_code(std::errc::permission_denied);
    newdir_check_fail(ino, path, mode, err);
    return nullptr;
  }

  ino_t newino;
  if (isReal()) {
    errno = 0;
    int fd = openDirByIno(ino);
    int ret = __accmut_libc_mkdirat(fd, path.c_str(), mode);
    __accmut_libc_close(fd);
    check_eq(errno, 0);
    check_eq(ret, 0);
    struct stat st;
    __accmut_libc_lstat(path.c_str(), &st);
    check_eq(errno, 0);
    newino = st.st_ino;
    /*
     DIR *dir = opendir(path.c_str());
     check_neq(dir, nullptr);

     struct dirent *pdir;*/
    ino_t fatherino = fakeFatherInode->getIno();
    std::list<dirent> dlist;
    /*
    dirent dot{.d_ino = st.st_ino,
#ifdef __APPLE__
               .d_namlen = 1,
#endif
               .d_type = DT_DIR};*/
    dirent dot;
    dot.d_ino = st.st_ino;
    dot.d_type = DT_DIR;
#ifdef __APPLE__
    dot.d_namlen = 1;
#endif
    strcpy(dot.d_name, ".");
    dlist.push_back(dot);

    /*
    dirent ddot{.d_ino = fakeFatherInode->getIno(),
#ifdef __APPLE__
                .d_namlen = 2,
#endif
                .d_type = DT_DIR};*/
    dirent ddot;
    ddot.d_ino = fakeFatherInode->getIno();
    ddot.d_type = DT_DIR;
#ifdef __APPLE__
    ddot.d_namlen = 2;
#endif
    strcpy(ddot.d_name, "..");
    dlist.push_back(ddot);
    /*

    while ((pdir = readdir(dir)) != nullptr) {
      dlist.push_back(*pdir);
      if (strcmp(pdir->d_name, "..") == 0)
        fatherino = pdir->d_ino;
    }
    __accmut_libc_closedir(dir);*/

    auto pdf = std::make_shared<DirectoryFile>(dlist);
    auto pdf1 = std::make_shared<DirectoryFile>(std::move(dlist));

    real_mapping[newino] = std::make_shared<INode>(st, pdf1);
    fake_mapping[newino] = std::make_shared<INode>(st, pdf);
    real_mapping[newino]->addBTPos(fatherino, path.lastName());
    fake_mapping[newino]->addBTPos(fatherino, path.lastName());

    /*
    dirent d{.d_ino = st.st_ino,
#ifdef __APPLE__
             .d_namlen = (uint16_t)strlen(path.lastName().c_str()),
#endif
             .d_type = DT_DIR};*/
    dirent d;
    d.d_ino = st.st_ino;
    d.d_type = DT_DIR;
#ifdef __APPLE__
    d.d_namlen = (uint16_t)strlen(path.lastName().c_str());
#endif

    strcpy(d.d_name, path.lastName().c_str());
    auto realFatherInodeIter = real_mapping.find(fakeFatherInode->getIno());
    if (realFatherInodeIter == real_mapping.end())
      panic("No sync");
    auto realFatherInode = realFatherInodeIter->second;
    if (!realFatherInode->isDir())
      panic("No sync");
    if (!realFatherInode->canWrite() || !realFatherInode->canExecute())
      panic("No sync");

    if (fakeFatherInode->cached()) {
      if (!realFatherInode->cached())
        panic("No sync");
      auto fakeFatherFile = std::static_pointer_cast<DirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      auto realFatherFile = std::static_pointer_cast<DirectoryFile>(
          realFatherInode->getUnderlyingFile());
      fakeFatherFile->push_back(d);
      realFatherFile->push_back(d);
    } else {
      if (realFatherInode->cached())
        panic("No sync");
      auto fakeFatherFile = std::static_pointer_cast<BaseDirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      auto realFatherFile = std::static_pointer_cast<BaseDirectoryFile>(
          realFatherInode->getUnderlyingFile());
      fakeFatherFile->newPushBack(d);
      realFatherFile->newPushBack(d);
    }
  } else {

    struct stat st;
    st.st_mode = (mode_t)S_IFDIR | mode;
    st.st_ino = get_fresh_ino();
    st.st_nlink = 2;
    st.st_uid = getuid();
    st.st_gid = getgid();
    st.st_size = 0;
    st.st_blksize = blksize;
#ifdef __APPLE__
    clock_gettime(CLOCK_REALTIME, &st.st_atimespec);
    st.st_mtimespec = st.st_atimespec;
    st.st_ctimespec = st.st_atimespec;
#else
    clock_gettime(CLOCK_REALTIME, &st.st_atim);
    st.st_mtim = st.st_atim;
    st.st_ctim = st.st_atim;
#endif
    newino = st.st_ino;

    std::list<dirent> dirents;
    /*
    dirent dot{.d_ino = st.st_ino,
#ifdef __APPLE__
               .d_namlen = 1,
#endif
               .d_type = DT_DIR};*/
    dirent dot;
    dot.d_ino = st.st_ino;
    dot.d_type = DT_DIR;
#ifdef __APPLE__
    dot.d_namlen = 1;
#endif
    strcpy(dot.d_name, ".");
    dirents.push_back(dot);

    /*
    dirent ddot{.d_ino = fakeFatherInode->getIno(),
#ifdef __APPLE__
                .d_namlen = 2,
#endif
                .d_type = DT_DIR};*/
    dirent ddot;
    ddot.d_ino = fakeFatherInode->getIno();
    ddot.d_type = DT_DIR;
#ifdef __APPLE
    ddot.d_namlen = 2;
#endif
    strcpy(ddot.d_name, "..");
    dirents.push_back(ddot);

    std::shared_ptr<DirectoryFile> rf =
        std::make_shared<DirectoryFile>(std::move(dirents));

    fake_mapping[newino] = std::make_shared<INode>(st, rf);
    fake_mapping[newino]->addBTPos(fakeFatherInode->getIno(), path.lastName());
    /*
    dirent d{.d_ino = st.st_ino,
#ifdef __APPLE__
             .d_namlen = (uint16_t)strlen(path.lastName().c_str()),
#endif
             .d_type = DT_DIR};*/
    dirent d;
    d.d_ino = st.st_ino;
    d.d_type = DT_DIR;
#ifdef __APPLE__
    d.d_namlen = (uint16_t)strlen(path.lastName().c_str());
#endif

    strcpy(d.d_name, path.lastName().c_str());

    if (fakeFatherInode->cached()) {
      auto fakeFatherFile = std::static_pointer_cast<DirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      fakeFatherFile->push_back(d);
    } else {
      auto fakeFatherFile = std::static_pointer_cast<BaseDirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      fakeFatherFile->newPushBack(d);
    }
  }
  return fake_mapping[newino];
}

std::shared_ptr<INode> MirrorFileSystem::newDirectory(const Path &path,
                                                      mode_t mode,
                                                      std::error_code &err) {
  return newDirectory(cwdino, path, mode, err);
}

void MirrorFileSystem::newsymlink_check_fail(ino_t ino, const std::string &str,
                                             const Path &path,
                                             std::error_code err) {
  if (err.value() == 0)
    panic("Should only check fail");
  if (isReal()) {
    errno = 0;
    int fd = openDirByIno(ino);
    int ret = symlinkat(str.c_str(), fd, path.c_str());
    __accmut_libc_close(fd);
    check_eq(errno, err.value());
    check_eq(ret, -1);
  }
}

std::shared_ptr<INode> MirrorFileSystem::newSymlink(const std::string &content,
                                                    ino_t ino, const Path &path,
                                                    std::error_code &err) {
  err.clear();
#ifndef __APPLE__
  if (content.size() == 0) {
    err = make_system_error_code(std::errc::no_such_file_or_directory);
    newsymlink_check_fail(ino, content, path, err);
    return nullptr;
  }

  if (path.isEndWithSlash()) {
    auto r1 =
        findINode(ino, path.getNoEndWithSlash(), F_OK, false, nullptr, err);
    if (err.value() == 0) {
      err = make_system_error_code(std::errc::file_exists);
    } else {
      err = make_system_error_code(std::errc::no_such_file_or_directory);
    }
    newsymlink_check_fail(ino, content, path, err);
    return nullptr;
  } else {
#endif
    auto r1 = findINode(ino, path, F_OK, false, nullptr, err);
    if (err.value() == 0) {
      err = make_system_error_code(std::errc::file_exists);
      newsymlink_check_fail(ino, content, path, err);
      return nullptr;
    }
#ifndef __APPLE__
  }
#endif
  err.clear();
  auto fakeFatherInode = findINode(ino, path.baseDir().getNoEndWithSlash(),
                                   F_OK, true, nullptr, err);
  if (err.value() != 0) {
    newsymlink_check_fail(ino, content, path, err);
    return nullptr;
  }

  if (!fakeFatherInode->isDir()) {
    err = make_system_error_code(std::errc::not_a_directory);
    newsymlink_check_fail(ino, content, path, err);
    return nullptr;
  }
  if (!(fakeFatherInode->canWrite() && fakeFatherInode->canExecute())) {
    err = make_system_error_code(std::errc::permission_denied);
    newsymlink_check_fail(ino, content, path, err);
    return nullptr;
  }
#ifdef __APPLE__
  if (path.isEndWithSlash()) {
    auto r2 =
        findINode(ino, path.getNoEndWithSlash(), F_OK, false, nullptr, err);
    if (err.value() == 0) {
      if (r2->isReg())
        err = make_system_error_code(std::errc::not_a_directory);
      else
        err = make_system_error_code(std::errc::file_exists);
    } else {
      err = make_system_error_code(std::errc::no_such_file_or_directory);
    }
    newsymlink_check_fail(ino, content, path, err);
    return nullptr;
  }
  if (content.size() == 0) {
    err = make_system_error_code(std::errc::invalid_argument);
    newsymlink_check_fail(ino, content, path, err);
    return nullptr;
  }
#endif

  ino_t newino;
  if (isReal()) {
    errno = 0;
    int fd = openDirByIno(ino);
    int ret = symlinkat(content.c_str(), fd, path.c_str());
    __accmut_libc_close(fd);
    check_eq(errno, 0);
    check_eq(ret, 0);
    struct stat st;
    ret = __accmut_libc_lstat(path.c_str(), &st);
    check_eq(errno, 0);
    check_eq(ret, 0);
    newino = st.st_ino;

    std::shared_ptr<SymbolicLinkFile> slf =
        std::make_shared<SymbolicLinkFile>(content);
    std::shared_ptr<SymbolicLinkFile> slf1 =
        std::make_shared<SymbolicLinkFile>(content);

    real_mapping[newino] = std::make_shared<INode>(st, slf);
    fake_mapping[newino] = std::make_shared<INode>(st, slf1);

    /*
    dirent d{.d_ino = st.st_ino,
#ifdef __APPLE__
             .d_namlen = (uint16_t)strlen(path.lastName().c_str()),
#endif
             .d_type = DT_LNK};*/
    dirent d;
    d.d_ino = st.st_ino;
    d.d_type = DT_LNK;
#ifdef __APPLE__
    d.d_namlen = (uint16_t)strlen(path.lastName().c_str());
#endif

    strcpy(d.d_name, path.lastName().c_str());
    auto realFatherInodeIter = real_mapping.find(fakeFatherInode->getIno());
    if (realFatherInodeIter == real_mapping.end())
      panic("No sync");
    auto realFatherInode = realFatherInodeIter->second;
    if (!realFatherInode->isDir())
      panic("No sync");
    if (!realFatherInode->canWrite() || !realFatherInode->canExecute())
      panic("No sync");

    if (fakeFatherInode->cached()) {
      if (!realFatherInode->cached())
        panic("No sync");
      auto fakeFatherFile = std::static_pointer_cast<DirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      auto realFatherFile = std::static_pointer_cast<DirectoryFile>(
          realFatherInode->getUnderlyingFile());
      fakeFatherFile->push_back(d);
      realFatherFile->push_back(d);
    } else {
      if (realFatherInode->cached())
        panic("No sync");
      auto fakeFatherFile = std::static_pointer_cast<BaseDirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      auto realFatherFile = std::static_pointer_cast<BaseDirectoryFile>(
          realFatherInode->getUnderlyingFile());
      fakeFatherFile->newPushBack(d);
      realFatherFile->newPushBack(d);
    }
    real_mapping[newino]->addBTPos(realFatherInode->getIno(), path.lastName());
    fake_mapping[newino]->addBTPos(fakeFatherInode->getIno(), path.lastName());
  } else {

    struct stat st;
    st.st_mode = (mode_t)S_IFLNK | 0755;
    st.st_ino = get_fresh_ino();
    st.st_nlink = 1;
    st.st_uid = getuid();
    st.st_gid = getgid();
    st.st_size = 0;
    st.st_blksize = blksize;
#ifdef __APPLE__
    clock_gettime(CLOCK_REALTIME, &st.st_atimespec);
    st.st_mtimespec = st.st_atimespec;
    st.st_ctimespec = st.st_atimespec;
#else
    clock_gettime(CLOCK_REALTIME, &st.st_atim);
    st.st_mtim = st.st_atim;
    st.st_ctim = st.st_atim;
#endif
    newino = st.st_ino;

    std::shared_ptr<SymbolicLinkFile> slf =
        std::make_shared<SymbolicLinkFile>(content);
    fake_mapping[newino] = std::make_shared<INode>(st, slf);
    /*
    dirent d{.d_ino = st.st_ino,
#ifdef __APPLE__
             .d_namlen = (uint16_t)strlen(path.lastName().c_str()),
#endif
             .d_type = DT_LNK};*/
    dirent d;
    d.d_ino = st.st_ino;
    d.d_type = DT_LNK;
#ifdef __APPLE__
    d.d_namlen = (uint16_t)strlen(path.lastName().c_str());
#endif

    strcpy(d.d_name, path.lastName().c_str());

    if (fakeFatherInode->cached()) {
      auto fakeFatherFile = std::static_pointer_cast<DirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      fakeFatherFile->push_back(d);
    } else {
      auto fakeFatherFile = std::static_pointer_cast<BaseDirectoryFile>(
          fakeFatherInode->getUnderlyingFile());
      fakeFatherFile->newPushBack(d);
    }
    fake_mapping[newino]->addBTPos(fakeFatherInode->getIno(), path.lastName());
  }
  return fake_mapping[newino];
}

std::shared_ptr<INode> MirrorFileSystem::newSymlink(const std::string &content,
                                                    const Path &path,
                                                    std::error_code &err) {
  return newSymlink(content, cwdino, path, err);
}

void MirrorFileSystem::chmod(ino_t ino, mode_t mode, std::error_code &err) {
  err.clear();

  auto iter = fake_mapping.find(ino);
  if (iter == fake_mapping.end())
    panic("Inexisting ino");
  auto r1 = iter->second;
  if (r1->isDir()) {
    if (r1->getBTPosSet().size() != 1)
      panic("Directory should have a parent directory");
  }
#ifndef __APPLE__
  if (r1->isLnk()) {
    err = make_system_error_code(std::errc::not_supported);
    return;
  }
#endif
  if (r1->getStat().st_uid != getuid()) {
    err = make_system_error_code(std::errc::operation_not_permitted);
    return;
  }
#ifdef __APPLE__
  if (mode & S_ISGID) {
    err = make_system_error_code(std::errc::operation_not_permitted);
    return;
  }
#endif
  r1->setMod(mode);
  if (isReal()) {
    auto riter = real_mapping.find(ino);
    if (riter == real_mapping.end())
      panic("Not found in real");
    riter->second->setMod(mode);
  }
}

void MirrorFileSystem::rename_check(ino_t ino1, const Path &path, ino_t ino2,
                                    const Path &path1, std::error_code err) {
  if (isReal()) {
    int fd1 = openDirByIno(ino1);
    int fd2 = openDirByIno(ino2);
    errno = 0;
    int ret = __accmut_libc_renameat(fd1, path.c_str(), fd2, path1.c_str());
    check_eq(errno, err.value());
    __accmut_libc_close(fd1);
    __accmut_libc_close(fd2);
    if (err.value() == 0) {
      check_eq(ret, 0);
    } else {
      check_eq(ret, -1);
    }
  }
}

static bool check_name_ddot(const Path &path, std::error_code &err) {
  if (path.lastName() == "." || path.lastName() == "..") {
#ifdef __APPLE__
    err = make_system_error_code(std::errc::invalid_argument);
#else
    err = make_system_error_code(std::errc::device_or_resource_busy);
#endif
    return false;
  }
  return true;
}

void MirrorFileSystem::rename(ino_t ino1, const accmut::Path &path, ino_t ino2,
                              const accmut::Path &path1, std::error_code &err) {
  err.clear();
  Path r, r1;
  auto ptr = findINode(ino1, path, F_OK, false, &r, err);
  if (err.value() != 0) {
    rename_check(ino1, path, ino2, path1, err);
    return;
  }

  auto ptr1 = findINode(ino2, path1, F_OK, false, &r1, err);
  if (err.value() == 0) {
#ifndef __APPLE__
    if (ptr->getIno() == ptr1->getIno()) {
      if (isReal()) {
        int fd1 = openDirByIno(ino1);
        int fd2 = openDirByIno(ino2);
        errno = 0;
        int ret = ::renameat(fd1, path.c_str(), fd2, path1.c_str());
        check_eq(errno, 0);
        __accmut_libc_close(fd1);
        __accmut_libc_close(fd2);
        check_eq(ret, 0);
      }
      return;
    }
#endif
    // need to remove
    if ((ptr->isDir() && (!ptr1->isDir()))) {
      err = make_system_error_code(std::errc::not_a_directory);
      rename_check(ino1, path, ino2, path1, err);
      return;
    }
    if ((!ptr->isDir()) && ptr1->isDir()) {
      err = make_system_error_code(std::errc::is_a_directory);
      rename_check(ino1, path, ino2, path1, err);
      return;
    }
    if ((ptr->isDir())) {
#ifdef __APPLE__
      if (!ptr->canWrite()) {
        err = make_system_error_code(std::errc::permission_denied);
        rename_check(ino1, path, ino2, path1, err);
        return;
      }
#endif
      if (!(check_name_ddot(path, err) && check_name_ddot(path1, err))) {
        rename_check(ino1, path, ino2, path1, err);
        return;
      }
      rmdir_precheck(ino2, path1, err);
      if (err.value() != 0) {
        rename_check(ino1, path, ino2, path1, err);
        return;
      }
#ifndef __APPLE__
      if (!(check_name_ddot(path, err) && check_name_ddot(path1, err))) {
        rename_check(ino1, path, ino2, path1, err);
        return;
      }
#endif
      rmdir(ino2, path1, err);
      if (err.value() != 0) {
        rename_check(ino1, path, ino2, path1, err);
        return;
      }
    } else {
#ifdef __APPLE__
#endif
      unlink_precheck(ino2, path1, err);
      if (err.value() != 0) {
        rename_check(ino1, path, ino2, path1, err);
        return;
      }

      if (ptr->getIno() == ptr1->getIno()) {
        if (isReal()) {
          int fd1 = openDirByIno(ino1);
          int fd2 = openDirByIno(ino2);
          errno = 0;
          int ret =
              __accmut_libc_renameat(fd1, path.c_str(), fd2, path1.c_str());
          check_eq(errno, 0);
          __accmut_libc_close(fd1);
          __accmut_libc_close(fd2);
          check_eq(ret, 0);
        }
        return;
      }
      unlink(ino2, path1, err);
      if (err.value() != 0) {
        rename_check(ino1, path, ino2, path1, err);
        return;
      }
    }
  } else if (err.value() == ENOTDIR) {
    rename_check(ino1, path, ino2, path1, err);
    return;
  }

  auto pf = findINode(ino1, r.baseDir().getNoEndWithSlash(), W_OK | X_OK, true,
                      nullptr, err);
  if (err.value() != 0) {
    rename_check(ino1, path, ino2, path1, err);
    return;
  }
  auto pf1 = findINode(ino2, path1.baseDir().getNoEndWithSlash(), W_OK | X_OK,
                       true, nullptr, err);
  if (err.value() != 0) {
    rename_check(ino1, path, ino2, path1, err);
    return;
  }

  if (!(check_name_ddot(path, err) && check_name_ddot(path1, err))) {
    rename_check(ino1, path, ino2, path1, err);
    return;
  }

  auto btpos = ptr->getBTPos(pf->getIno(), r.lastName());
  if (btpos == nullptr)
    panic("Failed to find btpos");

  if (pf->cached()) {
    auto pfather =
        std::static_pointer_cast<DirectoryFile>(pf->getUnderlyingFile());
    auto iter = pfather->find(r.lastName());
    if (iter == pfather->end())
      panic("Not found in father");
    auto d = *iter;
    strcpy(d.d_name, path1.lastName().c_str());
    pfather->erase(iter);
    if (pf1->cached()) {
      auto pfather1 =
          std::static_pointer_cast<DirectoryFile>(pf1->getUnderlyingFile());
      pfather1->push_back(d);
    } else {
      auto pfather1 =
          std::static_pointer_cast<BaseDirectoryFile>(pf1->getUnderlyingFile());
      pfather1->newPushBack(d);
    }
    btpos->ino = pf1->getIno();
    btpos->name = path1.lastName();
  } else {
    auto pfather =
        std::static_pointer_cast<BaseDirectoryFile>(pf->getUnderlyingFile());
    auto iter = pfather->newFind(r.lastName());
    if (iter == pfather->newEnd())
      panic("Not implemented");
    auto d = *iter;
    strcpy(d.d_name, path1.lastName().c_str());
    pfather->newErase(iter);
    auto diter = pfather->deletedFind(r.lastName());
    if (diter == pfather->deletedEnd())
      pfather->deletedPushBack(r.lastName());
    if (pf1->cached()) {
      auto pfather1 =
          std::static_pointer_cast<DirectoryFile>(pf1->getUnderlyingFile());
      pfather1->push_back(d);
    } else {
      auto pfather1 =
          std::static_pointer_cast<BaseDirectoryFile>(pf1->getUnderlyingFile());
      pfather1->newPushBack(d);
    }
    btpos->ino = pf1->getIno();
    btpos->name = path1.lastName();
  }

  // final processing
  if (isReal()) {
    auto realpfit = real_mapping.find(pf->getIno());
    auto realpf1it = real_mapping.find(pf1->getIno());
    auto realfileiter = real_mapping.find(ptr->getIno());
    if (realpfit == real_mapping.end())
      panic("Failed to find in real");
    if (realpf1it == real_mapping.end())
      panic("Failed to find in real");
    if (realfileiter == real_mapping.end())
      panic("File not found in real");
    auto realpf = realpfit->second;
    auto realpf1 = realpf1it->second;
    auto realptr = realfileiter->second;
    auto realbtpos = realptr->getBTPos(realpf->getIno(), r.lastName());
    if (realbtpos == nullptr)
      panic("Failed to find realbtpos");
    if (pf->cached()) {
      if (realpf->cached()) {
        auto realpfather = std::static_pointer_cast<DirectoryFile>(
            realpf->getUnderlyingFile());
        auto iter = realpfather->find(r.lastName());
        if (iter == realpfather->end())
          panic("Not found in real father");
        auto d = *iter;
        strcpy(d.d_name, path1.lastName().c_str());
        realpfather->erase(iter);
        if (realpf1->cached()) {
          auto realpfather1 = std::static_pointer_cast<DirectoryFile>(
              realpf1->getUnderlyingFile());
          realpfather1->push_back(d);
        } else {
          auto realpfather1 = std::static_pointer_cast<BaseDirectoryFile>(
              realpf1->getUnderlyingFile());
          realpfather1->newPushBack(d);
        }
        realbtpos->ino = realpf1->getIno();
        realbtpos->name = path1.lastName();
      } else {
        panic("real not cached");
      }
    } else {
      if (!realpf->cached()) {
        auto realpfather = std::static_pointer_cast<BaseDirectoryFile>(
            realpf->getUnderlyingFile());
        auto iter = realpfather->newFind(r.lastName());
        if (iter == realpfather->newEnd())
          panic("Not implemented");
        auto d = *iter;
        strcpy(d.d_name, path1.lastName().c_str());
        realpfather->newErase(iter);
        auto diter = realpfather->deletedFind(r.lastName());
        if (diter == realpfather->deletedEnd())
          realpfather->deletedPushBack(r.lastName());
        if (realpf1->cached()) {
          auto realpfather1 = std::static_pointer_cast<DirectoryFile>(
              realpf1->getUnderlyingFile());
          realpfather1->push_back(d);
        } else {
          auto realpfather1 = std::static_pointer_cast<BaseDirectoryFile>(
              realpf1->getUnderlyingFile());
          realpfather1->newPushBack(d);
        }
        realbtpos->ino = realpf1->getIno();
        realbtpos->name = path1.lastName();
      } else {
        panic("real cached");
      }
    }
  }

  rename_check(ino1, path, ino2, path1, err);
}

void MirrorFileSystem::rename(ino_t ino, const accmut::Path &path,
                              const accmut::Path &path1, std::error_code &err) {
  return rename(ino, path, cwdino, path1, err);
}

void MirrorFileSystem::rename(const accmut::Path &path, ino_t ino1,
                              const accmut::Path &path1, std::error_code &err) {
  return rename(cwdino, path, ino1, path1, err);
}

void MirrorFileSystem::rename(const accmut::Path &path,
                              const accmut::Path &path1, std::error_code &err) {
  return rename(cwdino, path, cwdino, path1, err);
}
Path MirrorFileSystem::bt2path(std::shared_ptr<BacktrackPos> bt,
                               std::error_code &err) {
  err.clear();
  if (bt->name == "/")
    return Path("/");
  Path base = realPathOfDir(bt->ino, err);
  if (err.value() != 0)
    return Path();
  return base + Path(bt->name);
}

void MirrorFileSystem::trunc(ino_t ino) { extractExistingINode(ino)->trunc(); }

bool MirrorFileSystem::isDir(ino_t ino) {
  return extractExistingINode(ino)->isDir();
}

bool MirrorFileSystem::isLnk(ino_t ino) {
  return extractExistingINode(ino)->isLnk();
}

bool MirrorFileSystem::isReg(ino_t ino) {
  return extractExistingINode(ino)->isReg();
}

bool MirrorFileSystem::isCharSpecial(ino_t ino) {
  return extractExistingINode(ino)->isCharSpecial();
}

bool MirrorFileSystem::isPipe(ino_t ino) {
  return extractExistingINode(ino)->isPipe();
}

bool MirrorFileSystem::canRead(ino_t ino) {
  return extractExistingINode(ino)->canRead();
}

bool MirrorFileSystem::canWrite(ino_t ino) {
  return extractExistingINode(ino)->canWrite();
}

bool MirrorFileSystem::canExecute(ino_t ino) {
  return extractExistingINode(ino)->canExecute();
}

void MirrorFileSystem::incRef(ino_t ino) {
  extractExistingINode(ino)->incRef();
}

void MirrorFileSystem::decRef(ino_t ino) {
  extractExistingINode(ino)->decRef();
}

void MirrorFileSystem::setMod(ino_t ino, mode_t mode) {
  extractExistingINode(ino)->setMod(mode);
}

struct stat MirrorFileSystem::getStat(ino_t ino) {
  return extractExistingINode(ino)->getStat();
}

void MirrorFileSystem::setUid(ino_t ino, uid_t uid) {
  extractExistingINode(ino)->setUid(uid);
}

void MirrorFileSystem::setGid(ino_t ino, gid_t gid) {
  extractExistingINode(ino)->setGid(gid);
}

ssize_t MirrorFileSystem::read(ino_t ino, void *buf, size_t nbyte, size_t off,
                               std::error_code &err) {
  return extractExistingINode(ino)->read(buf, nbyte, off, err);
}

ssize_t MirrorFileSystem::write(ino_t ino, const void *buf, size_t nbyte,
                                size_t off, std::error_code &err) {
  return extractExistingINode(ino)->write(buf, nbyte, off, err);
}

std::shared_ptr<BacktrackPos>
MirrorFileSystem::getBTPos(ino_t ino, ino_t fatherIno,
                           const std::string &name) {
  return extractExistingINode(ino)->getBTPos(fatherIno, name);
}

std::vector<struct dirent> MirrorFileSystem::getDirentList(ino_t ino) {
  auto inode = extractExistingINode(ino);
  if (!inode->cached())
    panic("Not implemented");
  auto df = std::static_pointer_cast<DirectoryFile>(inode->getUnderlyingFile());
  return std::vector<struct dirent>(df->begin(), df->end());
}

std::shared_ptr<INode> MirrorFileSystem::extractExistingINode(ino_t ino) {
  auto inode = fake_mapping[ino];
  if (!inode)
    panic("Should not happen");
  return inode;
}

} // namespace accmut
