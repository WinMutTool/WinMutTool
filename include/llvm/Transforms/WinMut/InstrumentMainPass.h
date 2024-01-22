#ifndef LLVM_INSTRUMENTMAIN_H
#define LLVM_INSTRUMENTMAIN_H

#include "llvm/Transforms/WinMut/Mutation.h"
#include "llvm/Transforms/WinMut/printIR.h"

#include "llvm/IR/Module.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;
using namespace std;

class InstrumentMainPass : public ModulePass {
  const char *func_name;
  void instrument(Function &F);

public:
  static char ID;

  InstrumentMainPass(const char *func_name);
  inline InstrumentMainPass() : InstrumentMainPass("__accmut__init") {}

  bool runOnModule(Module &M) override;
};

#endif // LLVM_INSTRUMENTMAIN_H
