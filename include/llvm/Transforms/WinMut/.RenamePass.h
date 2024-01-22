#ifndef LLVM_RENAMEPASS_H
#define LLVM_RENAMEPASS_H

#include "llvm/IR/Module.h"
#include <map>

using namespace llvm;

class RenamePass : public ModulePass {
public:
    static char ID;

    bool runOnModule(Module &M) override;

    RenamePass();

private:
    void renameType();

    Type *rename(Type *ty);

    void renameGlobals();

    void rewriteFunctions();

    void rewriteGlobalInitalizers();

    void renameBack();

    Module *theModule;
    StructType *accmut_file_ty;
    StructType *file_ty;
    std::map<StructType *, StructType *> tymap;
    std::map<Function *, Function *> funcmap;
    std::map<GlobalVariable *, GlobalVariable *> gvmap;
    std::vector<std::string> toSkip;

    void initSkip();

    void initFunc();

private:
    Value *rewriteValue(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteBinaryOperator(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteBranchInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteCallInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteCmpInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteGetElementPtrInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewritePHINode(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteReturnInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteStoreInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteAllocaInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteCastInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteLoadInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteConstantExpr(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteGlobalObject(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteConstantData(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteSwitchInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteSelectInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteShuffleVectorInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteInsertElementInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteExtractElementInst(Value *arg, std::map<Value *, Value *> &valmap);

    Value *rewriteConstantAggregate(Value *arg, std::map<Value *, Value *> &valmap);
};

#endif //LLVM_RENAMEPASS_H
