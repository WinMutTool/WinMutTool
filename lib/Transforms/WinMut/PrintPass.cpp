#include <llvm/Transforms/WinMut/PrintPass.h>

PrintPass::PrintPass(const char *suffix) : ModulePass(ID), suffix(suffix){}
PrintPass::PrintPass() : PrintPass(".ll") {}
char PrintPass::ID = 0;

bool PrintPass::runOnModule(Module &M) {
  printIR(M, suffix);
  return false;
}
static RegisterPass<PrintPass> X("WinMut-PrintPass", "WinMut - Print IR");
