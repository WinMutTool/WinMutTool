#ifndef LLVM_BASEDIRECTORYFILE_H
#define LLVM_BASEDIRECTORYFILE_H

#include "BaseFile.h"
#include <cstring>
#include <dirent.h>
#include <list>
#include <string>
#include <vector>

namespace accmut {
class BaseDirectoryFile : public BaseFile {
  std::list<std::string> deletedFiles;
  std::list<dirent> newFiles;
  typedef std::list<std::string>::iterator deletedIterator;
  typedef std::list<dirent>::iterator newIterator;
  typedef std::list<std::string>::const_iterator deletedConstIterator;
  typedef std::list<dirent>::const_iterator newConstIterator;

public:
  inline void dump(FILE *f) override { fprintf(f, "uncached directory\n"); }

  inline bool isCached() override { return false; }

  inline deletedIterator deletedBegin() { return deletedFiles.begin(); };

  inline deletedIterator deletedEnd() { return deletedFiles.end(); };

  inline deletedConstIterator deletedBegin() const {
    return deletedFiles.begin();
  };

  inline deletedConstIterator deletedEnd() const { return deletedFiles.end(); };

  inline deletedIterator deletedErase(deletedIterator pos) {
    return deletedFiles.erase(pos);
  }

  inline deletedIterator deletedErase(deletedConstIterator pos) {
    return deletedFiles.erase(pos);
  }

  inline void deletedPushBack(std::string d) {
    deletedFiles.push_back(std::move(d));
  }

  inline deletedIterator deletedFind(const std::string &name) {
    for (auto b = deletedBegin(); b != deletedEnd(); ++b) {
      if (name == *b)
        return b;
    }
    return deletedEnd();
  }

  inline newIterator newBegin() { return newFiles.begin(); };

  inline newIterator newEnd() { return newFiles.end(); };

  inline newConstIterator newBegin() const { return newFiles.begin(); };

  inline newConstIterator newEnd() const { return newFiles.end(); };

  inline newIterator newErase(newIterator pos) { return newFiles.erase(pos); }

  inline newIterator newErase(newConstIterator pos) {
    return newFiles.erase(pos);
  }

  inline void newPushBack(dirent d) { newFiles.push_back(d); }

  inline newIterator newFind(const std::string &name) {
    for (auto b = newBegin(); b != newEnd(); ++b) {
      if (strcmp(name.c_str(), b->d_name) == 0)
        return b;
    }
    return newEnd();
  }
};
} // namespace accmut

#endif // LLVM_BASEDIRECTORYFILE_H
