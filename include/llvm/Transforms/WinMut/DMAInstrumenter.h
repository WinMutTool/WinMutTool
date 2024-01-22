#ifndef ACCMUT_DMA_INSTRUMENTER_H
#define ACCMUT_DMA_INSTRUMENTER_H

#include "llvm/Transforms/WinMut/Mutation.h"

#include "llvm/IR/Module.h"

#include <vector>
#include <unordered_map>
#include <string>

using namespace llvm;
using namespace std;

class DMAInstrumenter : public ModulePass {
public:
    static char ID;// Pass identification, replacement for typeid
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    bool runOnModule(Module &M);

    //DMAInstrumenter(Module *M);
    DMAInstrumenter();

private:
    bool runOnFunction(Function &F);

    void collectCanMove(Function &F, vector<Mutation *> *v);

    void moveLiteralForFunc(Function &F, vector<Mutation *> *v);

    void instrument(Function &F, vector<Mutation *> *v);

    BasicBlock::iterator getLocation(Function &F, int instrumented_insts, int index);

    bool hasMutation(Instruction *inst, vector<Mutation *> *v);

    bool needInstrument(Instruction *I, vector<Mutation *> *v);

    Module *TheModule;
    bool firstTime = true;
    GlobalVariable *rmigv;
    StructType *regmutinfo;
    std::unordered_map<Value *, bool> canMove;
};

#endif
