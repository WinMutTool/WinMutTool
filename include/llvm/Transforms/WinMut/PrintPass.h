#ifndef LLVM_PRINTPASS_H
#define LLVM_PRINTPASS_H

#include "llvm/Transforms/WinMut/Mutation.h"
#include "llvm/Transforms/WinMut/printIR.h"

#include "llvm/IR/Module.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;
using namespace std;

class PrintPass : public ModulePass {
  const char *suffix;

public:
  static char ID;
  PrintPass();
  PrintPass(const char *suffix);
  bool runOnModule(Module &M) override;
};

#endif // LLVM_PRINTPASS_H
