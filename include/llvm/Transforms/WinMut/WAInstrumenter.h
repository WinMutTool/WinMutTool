#ifndef LLVM_WAINSTRUMENT_H
#define LLVM_WAINSTRUMENT_H

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"

#include "llvm/ADT/Optional.h"
#include "llvm/Transforms/WinMut/Mutation.h"
#include <llvm/Transforms/Utils/ValueMapper.h>

using namespace llvm;

#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using std::map;
using std::set;
using std::string;
using std::vector;

struct SplittedBlocks {
  BasicBlock *fullBB;
  BasicBlock *goodOnlyBB;
  BasicBlock *badOnlyBB;
  BasicBlock *noBB;
  std::vector<ValueToValueMapTy> vmap;
  int goodOnlyVmapIdx;
  int badOnlyVmapIdx;
};

struct DuplicatedBlockResult {
  BasicBlock *orig;
  BasicBlock *phiguard;
  std::vector<ValueToValueMapTy> &vmapList;
  std::vector<BasicBlock *> duplicated;
  DuplicatedBlockResult(BasicBlock *orig, BasicBlock *phiguard,
                        std::vector<ValueToValueMapTy> &vmapList)
      : orig(orig), phiguard(phiguard), vmapList(vmapList) {}
};

struct IDTuple {
  int left_id;
  int right_id;
  int ret_id;
  IDTuple(int left_id, int right_id, int ret_id)
      : left_id(left_id), right_id(right_id), ret_id(ret_id) {}
};

class WAInstrumenter : public ModulePass {
  bool useWindowAnalysis;
  bool optimizedInstrumentation;
  std::vector<std::string> skipFunctionList{"main", "mlp_filter_channel_x86"};
  std::string name;
  FILE *statFile = nullptr;

public:
  static char ID; // Pass identification, replacement for typeid
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  WAInstrumenter() : WAInstrumenter(true, true) {}

  WAInstrumenter(bool useWindowAnalysis, bool optimizedInstrumentation);

  bool runOnModule(Module &M);

private:
  Module *TheModule = nullptr;
  GlobalVariable *rmigv = nullptr;

  int numOfGoodVar = 0;
  int numOfMutants = 0;
  int maxNumOfGoodVar = 0;
  int maxNumOfMutants = 0;

  bool runOnFunction(Function &F);

  std::map<std::string, Type *> typeMapping;
  std::map<std::string, Function *> funcMapping;

  std::vector<GlobalVariable *> blockBoundGVs;
  std::map<GlobalVariable *, GlobalVariable *> constBlockBoundGVs;

  StructType *buildBlockBoundType(int length);

  GlobalVariable *
  buildBlockBoundGV(const std::list<std::pair<int, int>> &gvspec,
                    bool exactBound = true, int left = -1, int right = -1);

  GlobalVariable *blockGVArray;

  void genGlobalBlockGVArray();

  std::map<int, GlobalVariable *> mutSpecsGVs;
  StructType *buildMutSpecsType(int length);
  GlobalVariable *
  getMutSpecsGVFromAllList(int length,
                           const std::list<std::pair<int, int>> &all);
  void buildMutSpecsGV(std::list<std::pair<int, int>> gvspec);
  void printMutSpecsGV(const std::list<std::pair<int, int>> gvspec);
  std::map<int, GlobalVariable *> mutSpecsDepGVs;
  std::map<int, GlobalVariable *> mutSpecsNoDepGVs;
  void buildMutDepSpecsGV(std::list<std::pair<int, int>> gvspec,
                          std::list<IDTuple> idtuple);
  GlobalVariable *mutSpecsArray;
  void genGlobalMutSpecsArray();

  std::list<std::pair<int, int>>
  collapseGVSpec(const std::list<std::pair<int, int>> &gvspec);

  std::list<std::pair<int, int>>
  mergeGVSpec(const std::list<std::pair<int, int>> &gvspec1,
              const std::list<std::pair<int, int>> &gvspec2);
  std::list<std::pair<int, int>>
  complementaryGVSpec(const std::list<std::pair<int, int>> &all,
                      const std::list<std::pair<int, int>> &mask);

  std::map<int, GlobalVariable *> goodvarargGVs;
  GlobalVariable *goodvarargArray;
  void genGoodvarargArray();
  StructType *buildGoodvarArgInBlockType(int length);

  std::vector<BasicBlock *> cloneFunctionBlocks(Function *F, int left,
                                                int right);

  DuplicatedBlockResult duplicateBlock(BasicBlock *BB,
                                       std::vector<ValueToValueMapTy> &vmap);

  SplittedBlocks splitBasicBlock(BasicBlock *BB);

  Constant *getMutationIDConstant(bool isLocal);

  void generateRegisterCtor();

  void generateSetMaxCtor();

  void initialTypes();

  void initialFuncs();

  bool readInMuts();

  map<Instruction *, vector<Mutation *>> instMutsMap;

  void getInstMutsMap(vector<Mutation *> *v, Function &F);

  map<Instruction *, int> goodVariables;

  void getGoodVariables(BasicBlock &BB);

  void filterRealGoodVariables();

  bool hasUsedPreviousGoodVariables(Instruction *I);

  void pushGoodVarIdInfo(vector<Value *> &params, Instruction *I, int from,
                         int to, int good_from, int good_to, int left_id,
                         int right_id, int ret_id);

  int getGoodVarId(Value *V);

  ConstantInt *getI32Constant(int value);

  ConstantInt *getI64Constant(int64_t value);

  ConstantInt *getI8Constant(int8_t value);

  ConstantInt *getI1Constant(bool value);

  bool instrumentMulti(BasicBlock *BB, vector<ValueToValueMapTy> &mappings,
                       int good_idx = -1, int bad_idx = -1);

  void instrumentAboutGoodVariable(Instruction &I, int mut_from, int mut_to,
                                   int good_from, int good_to, bool is_first,
                                   int left_id, int right_id, int ret_id);

  void instrumentAsDMA(Instruction &I, int mut_from, int mut_to);

  void instrumentInst(Instruction &I, bool aboutGoodVariable, int mut_from,
                      int mut_to, int good_from, int good_to, bool is_first,
                      int left_id, int right_id, int ret_id);

  void instrumentCallInst(CallInst *call, int mut_from, int mut_to);
  void instrumentStoreInst(StoreInst *st, int mut_from, int mut_to);
  void instrumentArithInst(Instruction *cur_it, int mut_from, int mut_to,
                           bool aboutGoodVariable, int good_from, int good_to,
                           bool is_first, int left_id, int right_id,
                           int ret_id);
  void instrumentCmpInst(Instruction *cur_it, int mut_from, int mut_to,
                         bool aboutGoodVariable, int good_from, int good_to,
                         bool is_first, int left_id, int right_id, int ret_id);

  static bool isSupportedOpcode(Instruction *I);

  static bool isSupportedOp(Instruction *I);

  static bool isSupportedInstruction(Instruction *I);

  static bool isSupportedBoolInstruction(Instruction *I);

  static bool isSupportedType(Value *V);

  bool canFuseGoodVarInst(BasicBlock *BB);
};

#endif // LLVM_WAINSTRUMENT_H
