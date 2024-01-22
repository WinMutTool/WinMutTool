#include "llvm/Pass.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include <iostream>
#include <vector>
using namespace llvm;

#define DEBUG_TYPE "TestInst"

namespace {
    struct TestInst : public FunctionPass {
        static char ID;
        TestInst() : FunctionPass(ID) {

        }
        bool runOnFunction(Function &F) override {
            Module *module = F.getParent();
            StructType *t = module->getTypeByName("struct.A");
            StructType *t1 = StructType::create(
                    module->getContext(),
                    {IntegerType::get(module->getContext(), 32),
                     IntegerType::get(module->getContext(), 64)},
                     "struct.B"
                    );
            t->dump();
            t1->dump();
            std::cout << t1->getNumElements() << std::endl;

            std::vector<Constant *> vec;
            for (int i = 0; i < 10; i++) {
                vec.push_back((ConstantStruct::get(t1, {
                                ConstantInt::get(IntegerType::get(module->getContext(), 32), 2),
                                ConstantInt::get(IntegerType::get(module->getContext(), 64), 4)
                        })));
            }
            ConstantArray *a = dyn_cast<ConstantArray>(ConstantArray::get(ArrayType::get(t1, 10),
                    ArrayRef<Constant *>(vec)));

            GlobalVariable *gv1 = new GlobalVariable(
                    *module,
                    ArrayType::get(t1, 10),
                    false,
                    llvm::GlobalValue::LinkageTypes::InternalLinkage,
                    a,
                    F.getName() + ".mut",
                    nullptr,
                    llvm::GlobalValue::ThreadLocalMode::NotThreadLocal,
                    0
                    );
            gv1->setAlignment(16);
            module->dump();

            std::cout << "\n\n\n" << std::endl;

            for (auto i = module->global_begin(); i != module->global_end(); ++i) {
                GlobalValue &gv = *i;
                if (isa<GlobalVariable>(gv)) {
                    std::cout << "ok" << std::endl;
                    i->dump();
                }
                if (isa<Function>(gv)) {
                    std::cout << "func" << std::endl;
                }
                if (isa<Constant>(gv)) {
                    std::cout << "cons" << std::endl;
                }
                if (isa<GlobalIFunc>(gv)) {
                    std::cout << "ifunc" << std::endl;
                }
            }
            return true;
        }
    };
}

char TestInst::ID = 0;
static RegisterPass<TestInst>
        Y("testinst", "Test Inst");
