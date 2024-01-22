#include <llvm/Transforms/WinMut/InstrumentMainPass.h>

#include <llvm/IR/Constants.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

InstrumentMainPass::InstrumentMainPass(const char *func_name)
    : ModulePass(ID), func_name(func_name) {}

char InstrumentMainPass::ID = 0;

bool InstrumentMainPass::runOnModule(Module &M) {
  for (Function &F : M) {
    if (F.getName() == "main") {
      IntegerType *i32ty = IntegerType::get(M.getContext(), 32);
      IntegerType *i8ty = IntegerType::get(M.getContext(), 8);
      PointerType *i8pty = PointerType::get(i8ty, 0);
      PointerType *i8ptypty = PointerType::get(i8pty, 0);

      int idx = 0;
      Value *argc = nullptr;
      Value *argv = nullptr;
      for (auto &arg : F.args()) {
        if (idx == 0)
          argc = &arg;
        if (idx == 1)
          argv = &arg;
        ++idx;
      }
      if (!argc) {
        argc = ConstantInt::get(M.getContext(), APInt(32, "-1", 10));
      }
      if (!argv) {
        argv = ConstantPointerNull::get(i8ptypty);
      }

      Function *initfunc = M.getFunction(func_name);
      if (!initfunc) {
        FunctionType *initty = FunctionType::get(
            Type::getVoidTy(M.getContext()), {i32ty, i8ptypty}, false);
        initfunc = Function::Create(initty, GlobalValue::ExternalLinkage,
                                    func_name, &M);
        initfunc->setCallingConv(CallingConv::C);

        AttributeList initattr;
        {
          SmallVector<AttributeList, 4> Attrs;
          AttributeList PAS;
          {
            AttrBuilder B;
            PAS = AttributeList::get(M.getContext(), ~0U, B);
          }

          Attrs.push_back(PAS);
          initattr = AttributeList::get(M.getContext(), Attrs);
        }
        initfunc->setAttributes(initattr);
      }

      BasicBlock *front = &F.front();
      BasicBlock *bb = BasicBlock::Create(M.getContext(), "", &F, front);

      auto *callinit = CallInst::Create(initfunc, {argc, argv}, "", bb);

      callinit->setTailCallKind(CallInst::TailCallKind::TCK_Tail);

      /*auto *br = */ BranchInst::Create(front, bb);

#ifdef OUTPUT
      llvm::errs() << F << "\n";
#endif
#ifdef VERIFY
      if (verifyModule(M, &(llvm::errs()))) {
        llvm::errs() << "FATAL!!!! failed to verify\n";
        exit(-1);
      }
#endif

      // work on returns

      auto *exit_func = M.getFunction("exit");
      if (!exit_func) {
        auto *voidTy = Type::getVoidTy(M.getContext());
        auto *i32 = IntegerType::get(M.getContext(), 32);
        auto *functype = FunctionType::get(voidTy, {i32}, false);
        exit_func = Function::Create(
            functype, GlobalValue::LinkageTypes::ExternalLinkage, "exit", &M);

        exit_func->addFnAttr(Attribute::NoReturn);
      }

      for (auto &BB : F) {
        auto it = BB.begin();
        for (; it != BB.end();) {
          auto *I = &*it;
          ++it;
          if (auto *ret = dyn_cast<ReturnInst>(I)) {
            auto *retval = ret->getReturnValue();
            CallInst::Create(exit_func, {retval}, "", ret);
            ReplaceInstWithInst(ret, new UnreachableInst(M.getContext()));
          }
        }
      }

      return true;
    }
  }
  return false;
}

void InstrumentMainPass::instrument(Function &F) {}

static RegisterPass<InstrumentMainPass> X("WinMut-InstrumentMainPass",
                                          "WinMut - InstrumentMain");