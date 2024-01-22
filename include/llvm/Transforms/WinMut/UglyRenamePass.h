#ifndef LLVM_UGLYRENAMEPASS_H
#define LLVM_UGLYRENAMEPASS_H

#include "llvm/IR/Module.h"
#include <map>

using namespace llvm;

class UglyRenamePass : public ModulePass {
public:
  static char ID;

  bool runOnModule(Module &M) override;

  UglyRenamePass();
};

#endif // LLVM_UGLYRENAMEPASS_H
