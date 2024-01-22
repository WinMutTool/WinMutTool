#ifndef LLVM_PRINTIR_H
#define LLVM_PRINTIR_H

#include "llvm/IR/Module.h"
#include <llvm/Support/raw_ostream.h>
using namespace llvm;
using namespace std;

inline void printIR(Module &M, const char *suffix) {
  std::string ir;
  raw_string_ostream os(ir);
  os << M;
  os.flush();
  auto name = std::string(M.getName()) + suffix;
  FILE *f = fopen(name.c_str(), "w");
  if (f) {
    fputs(ir.c_str(), f);
    fclose(f);
  } else {
    llvm::errs() << "Cannot open " << name << "\n";
  }
}

#endif // LLVM_PRINTIR_H
