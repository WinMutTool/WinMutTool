#ifndef ACCMUT_MUTATION_GEN_H
#define ACCMUT_MUTATION_GEN_H

#include "llvm/IR/Module.h"

#include <fstream>
#include <string>

using namespace llvm;

class MutationGen : public ModulePass {
public:
  static char ID; // Pass identification, replacement for typeid
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  Module *TheModule;
  static std::string filename;
  static std::ofstream ofresult;

  MutationGen(/*Module *M*/);

  virtual bool runOnModule(Module &M);

  static void genMutationFile(Function &F);

  static std::string getMutationFilePath();

private:
  bool runOnFunction(Function &F);

  static void genAOR(Instruction *inst, StringRef fname, int index);

  static void genLOR(Instruction *inst, StringRef fname, int index);

  static void genCOR();

  static void genROR(Instruction *inst, StringRef fname, int index);

  static void genSOR();

  static void genSTDCall(Instruction *inst, StringRef fname, int index);

  static void genSTDStore(Instruction *inst, StringRef fname, int index);

  static void genLVR(Instruction *inst, StringRef fname, int index);

  static void genUOI(Instruction *inst, StringRef fname, int index);

  static void genROV(Instruction *inst, StringRef fname, int index);

  static void genABV(Instruction *inst, StringRef fname, int index);
};

#endif
