#ifndef LLVM_SYMBOLICLINKFILE_H
#define LLVM_SYMBOLICLINKFILE_H

#include <utility>

#include "BaseFile.h"

namespace accmut {
class SymbolicLinkFile : public BaseFile {
  std::string target;

public:
  inline explicit SymbolicLinkFile(std::string target)
      : target(std::move(target)) {}

  inline std::string getTarget() { return target; }

  inline void dump(FILE *f) override {
    fprintf(f, "linkto\t%s\n", target.c_str());
  }

  inline bool isCached() override { return true; }
};
} // namespace accmut

#endif // LLVM_SYMBOLICLINKFILE_H
