#ifndef LLVM_NAIVESPLITBLOCKPASS_H
#define LLVM_NAIVESPLITBLOCKPASS_H

#include "llvm/IR/Module.h"

using namespace llvm;

class NaiveSplitBlockPass : public ModulePass {
public:
  static char ID;

  bool runOnModule(Module &M) override;

  NaiveSplitBlockPass();
};
#endif // LLVM_NAIVESPLITBLOCKPASS_H
