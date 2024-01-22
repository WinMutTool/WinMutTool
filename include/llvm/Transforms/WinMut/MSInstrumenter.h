#ifndef ACCMUT_MS_INSTRUMENTER_H
#define ACCMUT_MS_INSTRUMENTER_H

#include "llvm/IR/Module.h"
#include "llvm/Transforms/WinMut/Mutation.h"

#include <map>
#include <string>
#include <vector>

using namespace llvm;
using namespace std;

class MSInstrumenter : public FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  virtual bool runOnFunction(Function &F);

  MSInstrumenter(/*Module *M*/);

private:
  void filtMutsByIndex(Function &F, vector<Mutation *> *v);

  int instrument(Function &F, vector<Mutation *> *v, int index, int mut_from,
                 int mut_to, int instrumented_insts);

  Module *TheModule;
};

#endif
