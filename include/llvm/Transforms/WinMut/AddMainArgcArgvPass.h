#ifndef LLVM_ADDMAINARGCARGVPASS_H
#define LLVM_ADDMAINARGCARGVPASS_H

#include "llvm/Transforms/WinMut/Mutation.h"
#include "llvm/Transforms/WinMut/printIR.h"

#include "llvm/IR/Module.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;
using namespace std;

class AddMainArgcArgvPass : public ModulePass {
public:
  static char ID;

  AddMainArgcArgvPass();

  bool runOnModule(Module &M) override;
};
#endif // LLVM_ADDMAINARGCARGVPASS_H
