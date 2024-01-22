#ifndef LLVM_DIRECTORYFILE_H
#define LLVM_DIRECTORYFILE_H

#include "BaseDirectoryFile.h"
#include <cstring>
#include <dirent.h>
#include <list>
#include <string>

namespace accmut {
class DirectoryFile : public BaseDirectoryFile {
  std::list<dirent> dirents;
  typedef std::list<dirent>::iterator iterator;
  typedef std::list<dirent>::const_iterator const_iterator;

public:
  inline explicit DirectoryFile(std::list<dirent> dirents)
      : dirents(std::move(dirents)) {}

  inline iterator begin() { return dirents.begin(); };

  inline iterator end() { return dirents.end(); };

  inline const_iterator begin() const { return dirents.begin(); };

  inline const_iterator end() const { return dirents.end(); };

  inline iterator erase(iterator pos) { return dirents.erase(pos); }

  inline iterator erase(const_iterator pos) { return dirents.erase(pos); }

  inline void push_back(dirent d) { dirents.push_back(d); }

  inline iterator find(const std::string &name) {
    for (auto b = begin(); b != end(); ++b) {
      if (strcmp(name.c_str(), b->d_name) == 0)
        return b;
    }
    return end();
  }

  void dump(FILE *f) override;

  inline bool isCached() override { return true; }
};
} // namespace accmut

#endif // LLVM_DIRECTORYFILE_H
