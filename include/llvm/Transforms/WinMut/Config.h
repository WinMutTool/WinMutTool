#ifndef ACCMUT_CONFIG_H
#define ACCMUT_CONFIG_H

// SWITCH FOR IR-LEVEL MUTATION GENERATION
#define ACCMUT_GEN_MUT 1

// SWITCH FOR MUTATION SCHEMATA
#define ACCMUT_MUTATION_SCHEMATA 2

// SWITCH FOR DYNAMIC ANALYSIS
#define ACCMUT_DYNAMIC_ANALYSIS_INSTRUMENT 3

// SWITCH FOR WINDOW ANALYSIS
#define ACCMUT_WINDOW_ANALYSIS_INSTRUMENT 4

// SWITCH FOR SOME STATISTICS
#define ACCMUT_STATISTICS_INSTRUEMENT 5

#define ACCMUT_MODE_SELECT ACCMUT_WINDOW_ANALYSIS_INSTRUMENT

/*
static_assert(ACCMUT_GEN_MUT + ACCMUT_MUTATION_SCHEMATA +
ACCMUT_DYNAMIC_ANALYSIS_INSTRUEMENT
              + ACCMUT_WINDOW_ANALYSIS_INSTRUMENT +
ACCMUT_STATISTICS_INSTRUEMENT
              <= 1, "WinMut - too many options in Config.h!");*/
static_assert(ACCMUT_MODE_SELECT <= 5 && ACCMUT_MODE_SELECT >= 0,
              "WinMut - Unknown type");

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

class FunctionPassToModulePassWrapper : public ModulePass {
public:
  static char ID;

  FunctionPassToModulePassWrapper(FunctionPass *workPass) : ModulePass(ID) {
    pass = workPass;
  }

  ~FunctionPassToModulePassWrapper() override { delete pass; }

  bool runOnModule(Module &M) override {
    bool changed = false;
    for (Function &F : M) {
      changed |= pass->runOnFunction(F);
    }
    return changed;
  }

private:
  FunctionPass *pass;
};

char FunctionPassToModulePassWrapper::ID = 0;

#endif
