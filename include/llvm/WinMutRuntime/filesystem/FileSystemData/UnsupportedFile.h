#ifndef LLVM_UNSUPPORTEDFILE_H
#define LLVM_UNSUPPORTEDFILE_H

#include "NoMemDataFile.h"

namespace accmut {
class UnsupportedFile : public NoMemDataFile {
  inline void dump(FILE *f) override { fprintf(f, "unsupported\n"); }

  inline bool isCached() override { return false; }
};
} // namespace accmut

#endif // LLVM_UNSUPPORTEDFILE_H
