#include "llvm/Transforms/WinMut/AddMainArgcArgvPass.h"
#include "llvm/Transforms/Utils/Cloning.h"

AddMainArgcArgvPass::AddMainArgcArgvPass() : ModulePass(ID) {}

char AddMainArgcArgvPass::ID = 0;

bool AddMainArgcArgvPass::runOnModule(Module &M) {
  for (Function &F : M) {
    if (F.getName() == "main") {
      if (F.arg_size() < 2) {
        auto i32 = IntegerType::get(M.getContext(), 32);
        auto i8 = IntegerType::get(M.getContext(), 8);
        auto mainType = FunctionType::get(
            i32, {i32, PointerType::get(PointerType::get(i8, 0), 0)}, false);

        auto newMain =
            Function::Create(mainType, F.getLinkage(), F.getName(), &M);
        ValueToValueMapTy vmap;
        for (auto it1 = F.arg_begin(), it2 = newMain->arg_begin();
             it1 != F.arg_end(); ++it1, ++it2) {
          vmap[&*it1] = &*it2;
        }
        SmallVector<ReturnInst *, 8> returns;
        CloneFunctionInto(newMain, &F, vmap, true, returns);

        F.deleteBody();
        F.removeFromParent();
        RemapFunction(F, vmap);
        newMain->setName("main");
        return true;
      }
    }
  }
  return false;
}

static RegisterPass<AddMainArgcArgvPass> X("WinMut-AddMainArgcArgvPass",
                                           "WinMut - Add argc & argv to main");
