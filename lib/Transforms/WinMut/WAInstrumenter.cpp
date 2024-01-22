#include "llvm/Transforms/WinMut/WAInstrumenter.h"
#include "llvm/Transforms/WinMut/MutUtil.h"
#include <sstream>

#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>

#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <llvm/Transforms/Utils/ValueMapper.h>

#include <signal.h>
#include <unistd.h>

using namespace llvm;

WAInstrumenter::WAInstrumenter(bool useWindowAnalysis, bool optimizedInstrumentation) : ModulePass(ID), useWindowAnalysis(useWindowAnalysis), optimizedInstrumentation(optimizedInstrumentation) {}

void WAInstrumenter::initialTypes() {
  auto i1 = IntegerType::get(TheModule->getContext(), 1);
  auto i8 = IntegerType::get(TheModule->getContext(), 8);
  auto i32 = IntegerType::get(TheModule->getContext(), 32);
  auto i64 = IntegerType::get(TheModule->getContext(), 64);
  auto voidTy = Type::getVoidTy(TheModule->getContext());

  typeMapping["i8"] = i8;
  typeMapping["i32"] = i32;
  typeMapping["i64"] = i64;
  typeMapping["void"] = voidTy;
  typeMapping["i8p"] = PointerType::get(i8, 0);

  // struct.Mutation
  StructType *mutationTy = StructType::create(
      TheModule->getContext(), {i32, i32, i32, i64, i64}, "struct.Mutation");
  typeMapping["Mutation"] = mutationTy;

  // struct.RegMutInfo

  StructType *regmutinfoTy = StructType::create(
      TheModule->getContext(),
      {PointerType::get(mutationTy, 0), i32, i32, i32, PointerType::get(i8, 0)},
      "struct.RegMutInfo");
  typeMapping["RegMutInfo"] = regmutinfoTy;

  // struct.BlockRegMutBound
  StructType *blockRegMutBound = StructType::create(
      TheModule->getContext(), {i32, i32}, "struct.BlockRegMutBound");
  typeMapping["BlockRegMutBound"] = blockRegMutBound;

  // struct.MutSpec
  auto *mutspecTy =
      StructType::create(TheModule->getContext(), {i32, i32}, "struct.MutSpec");
  typeMapping["MutSpec"] = mutspecTy;

  // struct.MutSpecs
  auto *mutspecsTy = StructType::create(
      TheModule->getContext(), {i32, i32, ArrayType::get(mutspecTy, 1)},
      "struct.MutSpec");
  typeMapping["MutSpecs"] = mutspecsTy;

  // struct.GoodvarArg & struct.GoodvarArgInBlock
  auto *goodvarargInBlockTy =
      StructType::create(TheModule->getContext(), "struct.GoodvarArgInBlock");
  auto *goodvarargTy = StructType::create(
      TheModule->getContext(),
      {i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32,
       PointerType::get(mutspecsTy, 0), PointerType::get(mutspecsTy, 0),
       PointerType::get(mutspecsTy, 0),
       PointerType::get(goodvarargInBlockTy, 0),
       PointerType::get(regmutinfoTy, 0)},
      "struct.GoodvarArg");
  typeMapping["GoodvarArg"] = goodvarargTy;

  goodvarargInBlockTy->setBody(
      {i32, ArrayType::get(PointerType::get(goodvarargTy, 0), 1)});
  typeMapping["GoodvarArgInBlock"] = goodvarargInBlockTy;

  // registerFuncTy
  vector<Type *> paramTypes{
      PointerType::get(regmutinfoTy, 0),
      PointerType::get(PointerType::get(blockRegMutBound, 0), 0),
      i32,
      PointerType::get(PointerType::get(mutspecsTy, 0), 0),
      i32,
      PointerType::get(PointerType::get(goodvarargTy, 0), 0),
      i32};
  typeMapping["registerFuncTy"] = FunctionType::get(voidTy, paramTypes, false);

  typeMapping["setMaxNumTy"] = FunctionType::get(voidTy, {i64, i64}, false);

  typeMapping["logFuncTy"] =
      FunctionType::get(voidTy, {i32, i32, i32, i32, i32, i1, i32}, false);

  typeMapping["tablePushFuncTy"] = FunctionType::get(voidTy, {i64, i64}, false);

  // goodvarVoidFuncTy
  typeMapping["returnvoidFuncTy"] = FunctionType::get(voidTy, false);
  typeMapping["returni32FuncTy"] = FunctionType::get(i32, false);
  typeMapping["returni64FuncTy"] = FunctionType::get(i64, false);

  // goodvarFuncTy
  auto getGoodvarBinaryFuncTy = [&](Type *retType, Type *paramType) {
    std::vector<Type *> paramTypes{paramType, paramType,
                                   PointerType::get(goodvarargTy, 0)};
    return FunctionType::get(retType, paramTypes, false);
  };

  typeMapping["goodvari32i32FuncTy"] = getGoodvarBinaryFuncTy(i32, i32);
  typeMapping["goodvari64i64FuncTy"] = getGoodvarBinaryFuncTy(i64, i64);
  typeMapping["goodvari64i32FuncTy"] = getGoodvarBinaryFuncTy(i32, i64);

  // normalFuncTy
  auto getNormalBinaryFuncTy = [&](Type *retType, Type *paramType) {
    std::vector<Type *> paramTypes{PointerType::get(regmutinfoTy, 0), i32, i32,
                                   paramType, paramType};
    return FunctionType::get(retType, paramTypes, false);
  };

  typeMapping["normali32i32FuncTy"] = getNormalBinaryFuncTy(i32, i32);
  typeMapping["normali64i64FuncTy"] = getNormalBinaryFuncTy(i64, i64);
  typeMapping["normali64i32FuncTy"] = getNormalBinaryFuncTy(i32, i64);

  typeMapping["prepareCallFuncTy"] = FunctionType::get(
      i32, {PointerType::get(regmutinfoTy, 0), i32, i32, i32}, true);

  typeMapping["prepareStorei32FuncTy"] =
      FunctionType::get(i32,
                        {PointerType::get(regmutinfoTy, 0), i32, i32, i32,
                         PointerType::get(i32, 0)},
                        false);
  typeMapping["prepareStorei64FuncTy"] =
      FunctionType::get(i32,
                        {PointerType::get(regmutinfoTy, 0), i32, i32, i64,
                         PointerType::get(i64, 0)},
                        false);
}

void WAInstrumenter::initialFuncs() {
  auto registerFuncTy = dyn_cast<FunctionType>(typeMapping["registerFuncTy"]);
  auto setMaxNumTy = dyn_cast<FunctionType>(typeMapping["setMaxNumTy"]);
  auto logFuncTy = dyn_cast<FunctionType>(typeMapping["logFuncTy"]);
  auto returnvoidFuncTy =
      dyn_cast<FunctionType>(typeMapping["returnvoidFuncTy"]);
  auto returni32FuncTy = dyn_cast<FunctionType>(typeMapping["returni32FuncTy"]);
  auto returni64FuncTy = dyn_cast<FunctionType>(typeMapping["returni64FuncTy"]);
  auto goodvari32i32FuncTy =
      dyn_cast<FunctionType>(typeMapping["goodvari32i32FuncTy"]);
  auto goodvari64i64FuncTy =
      dyn_cast<FunctionType>(typeMapping["goodvari64i64FuncTy"]);
  auto goodvari64i32FuncTy =
      dyn_cast<FunctionType>(typeMapping["goodvari64i32FuncTy"]);
  auto normali32i32FuncTy =
      dyn_cast<FunctionType>(typeMapping["normali32i32FuncTy"]);
  auto normali64i64FuncTy =
      dyn_cast<FunctionType>(typeMapping["normali64i64FuncTy"]);
  auto normali64i32FuncTy =
      dyn_cast<FunctionType>(typeMapping["normali64i32FuncTy"]);
  auto prepareCallFuncTy =
      dyn_cast<FunctionType>(typeMapping["prepareCallFuncTy"]);
  auto prepareStorei32FuncTy =
      dyn_cast<FunctionType>(typeMapping["prepareStorei32FuncTy"]);
  auto prepareStorei64FuncTy =
      dyn_cast<FunctionType>(typeMapping["prepareStorei64FuncTy"]);
  auto tablePushFuncTy = dyn_cast<FunctionType>(typeMapping["tablePushFuncTy"]);
  std::vector<std::pair<std::string, FunctionType *>> toCreate{
      {"__accmut__register", registerFuncTy},
      {"__accmut__set_max_num", setMaxNumTy},
      {"__accmut__log", logFuncTy},
      {"__accmut__stdcall_i32", returni32FuncTy},
      {"__accmut__stdcall_i64", returni64FuncTy},
      {"__accmut__stdcall_void", returnvoidFuncTy},
      {"__accmut__std_store", returnvoidFuncTy},
      {"__accmut__process_i32_arith", normali32i32FuncTy},
      {"__accmut__process_i64_arith", normali64i64FuncTy},
      {"__accmut__process_i32_cmp", normali32i32FuncTy},
      {"__accmut__process_i64_cmp", normali64i32FuncTy},
      {"__accmut__prepare_call", prepareCallFuncTy},
      {"__accmut__prepare_st_i32", prepareStorei32FuncTy},
      {"__accmut__prepare_st_i64", prepareStorei64FuncTy},
      {"__accmut__GoodVar_TablePush", tablePushFuncTy},
      {"__accmut__GoodVar_TablePop", returnvoidFuncTy},
      {"__accmut__GoodVar_TablePush_max", returnvoidFuncTy},
      {"__accmut__process_i32_arith_GoodVar", goodvari32i32FuncTy},
      {"__accmut__process_i32_arith_GoodVar_init", goodvari32i32FuncTy},
      {"__accmut__process_i64_arith_GoodVar", goodvari64i64FuncTy},
      {"__accmut__process_i64_arith_GoodVar_init", goodvari64i64FuncTy},
      {"__accmut__process_i32_cmp_GoodVar", goodvari32i32FuncTy},
      {"__accmut__process_i32_cmp_GoodVar_init", goodvari32i32FuncTy},
      {"__accmut__process_i64_cmp_GoodVar", goodvari64i32FuncTy},
      {"__accmut__process_i64_cmp_GoodVar_init", goodvari64i32FuncTy},
  };

  for (auto &p : toCreate) {
    auto F = TheModule->getFunction(p.first);
    if (F == nullptr) {
      F = Function::Create(p.second, GlobalValue::ExternalLinkage, p.first,
                           TheModule);
    }
    funcMapping[p.first] = F;
  }
}

StructType *WAInstrumenter::buildBlockBoundType(int length) {
  stringstream ss;
  ss << "struct.BlockRegMutBound" << length;
  string name = ss.str();
  Type *i8 = typeMapping["i8"];
  Type *i32 = typeMapping["i32"];
  // struct.BlockRegMutBound
  StructType *blockRegMutBound = TheModule->getTypeByName(name);
  if (blockRegMutBound == nullptr) {
    blockRegMutBound = StructType::create(
        TheModule->getContext(), {i32, i32, ArrayType::get(i8, length)}, name);
  }
  return blockRegMutBound;
}

StructType *WAInstrumenter::buildMutSpecsType(int length) {
  stringstream ss;
  ss << "struct.MutSpecs" << length;
  string name = ss.str();
  StructType *mutspecsTy = TheModule->getTypeByName(name);
  if (mutspecsTy == nullptr) {
    Type *i32 = typeMapping["i32"];
    mutspecsTy = StructType::create(
        TheModule->getContext(),
        {i32, i32, ArrayType::get(typeMapping["MutSpec"], length)}, name);
  }
  return mutspecsTy;
}

StructType *WAInstrumenter::buildGoodvarArgInBlockType(int length) {
  stringstream ss;
  ss << "struct.GoodvarArgInBlockType" << length;
  string name = ss.str();
  StructType *gvArgInBlockTy = TheModule->getTypeByName(name);
  if (gvArgInBlockTy == nullptr) {
    Type *i32 = typeMapping["i32"];
    gvArgInBlockTy = StructType::create(
        TheModule->getContext(),
        {i32, ArrayType::get(PointerType::get(typeMapping["GoodvarArg"], 0),
                             length)},
        name);
  }
  return gvArgInBlockTy;
}

std::list<std::pair<int, int>>
WAInstrumenter::collapseGVSpec(const std::list<std::pair<int, int>> &gvspec) {
  if (gvspec.size() <= 1)
    return gvspec;
  auto localSpec = gvspec;
  localSpec.sort(
      [](const std::pair<int, int> &p1, const std::pair<int, int> &p2) {
        return p1.first < p2.first;
      });
  auto b1 = localSpec.begin();
  auto b2 = localSpec.begin();
  b2++;
  while (b2 != localSpec.end()) {
    auto &p1 = *b1;
    auto &p2 = *b2;
    if (p1.second + 1 >= p2.first) {
      p1.second = p2.second;
      b2 = localSpec.erase(b2);
    } else {
      b1++;
      b2++;
    }
  }
  return localSpec;
}

std::list<std::pair<int, int>>
WAInstrumenter::mergeGVSpec(const std::list<std::pair<int, int>> &gvspec1,
                            const std::list<std::pair<int, int>> &gvspec2) {
  auto local1 = gvspec1;
  auto local2 = gvspec2;
  local1.merge(local2);
  auto cmp = [](const std::pair<int, int> &p1, const std::pair<int, int> &p2) {
    return p1.first < p2.first;
  };
  local1.sort(cmp);
  local2.sort(cmp);
  local1.merge(local2);
  local1.unique(
      [](const std::pair<int, int> &p1, const std::pair<int, int> &p2) {
        return p1.first == p2.first;
      });
  return local1;
}

std::list<std::pair<int, int>> WAInstrumenter::complementaryGVSpec(
    const std::list<std::pair<int, int>> &all,
    const std::list<std::pair<int, int>> &mask) {
  std::list<std::pair<int, int>> ret;
  auto itall = all.begin();
  auto itmask = mask.begin();
  while (itall != all.end()) {
    if (itall->first == itmask->first) {
      ++itall;
      ++itmask;
      continue;
    }
    ret.push_back(*itall);
    ++itall;
  }
  assert(itmask == mask.end());
  return ret;
}

GlobalVariable *WAInstrumenter::getMutSpecsGVFromAllList(
    int length, const std::list<std::pair<int, int>> &all) {
  int totlength = all.size();

  auto *mutspecTy = cast<StructType>(this->typeMapping["MutSpec"]);
  std::vector<Constant *> initvec;

  for (auto &p : all) {
    initvec.push_back(
        ConstantStruct::get(mutspecTy, {this->getI32Constant(p.first),
                                        this->getI32Constant(p.second)}));
  }
  Constant *initarr =
      ConstantArray::get(ArrayType::get(mutspecTy, initvec.size()), initvec);

  auto *ty = this->buildMutSpecsType(initvec.size());
  auto init =
      ConstantStruct::get(ty, {this->getI32Constant(length),
                               this->getI32Constant(totlength), initarr});
  auto *gv = new GlobalVariable(
      *this->TheModule, ty, false,
      llvm::GlobalValue::LinkageTypes::InternalLinkage, init);
  return gv;
}

void WAInstrumenter::printMutSpecsGV(
    const std::list<std::pair<int, int>> gvspec) {
  for (auto &x : gvspec) {
    llvm::errs() << "(" << x.first << ", " << x.second << ") ";
  }
}

void WAInstrumenter::buildMutSpecsGV(std::list<std::pair<int, int>> gvspec) {
  std::list<std::pair<int, int>> head;
  std::list<std::pair<int, int>> tail;
  while (!gvspec.empty()) {
    head.push_back(gvspec.front());
    gvspec.pop_front();
    head = collapseGVSpec(head);
    tail = collapseGVSpec(gvspec);
    int length = head.size();
    std::list<std::pair<int, int>> all = head;
    for (auto &x : tail) {
      all.push_back(x);
    }
    mutSpecsGVs[head.back().second] = getMutSpecsGVFromAllList(length, all);
  }
}

void WAInstrumenter::buildMutDepSpecsGV(std::list<std::pair<int, int>> gvspec,
                                        std::list<IDTuple> idtuple) {
  std::list<std::pair<int, int>> seen;
  std::map<int, std::list<std::pair<int, int>>> dep;
  std::map<int, std::list<std::pair<int, int>>> notdep;
  std::map<int, std::pair<int, int>> idToMutPair;
  auto specit = gvspec.begin();
  auto idit = idtuple.begin();

  for (; specit != gvspec.end(); ++specit, ++idit) {
    seen.push_back(*specit);
    if (idit->ret_id != 0) {
      idToMutPair[idit->ret_id] = *specit;
      dep[idit->ret_id] = mergeGVSpec(dep[idit->left_id], dep[idit->right_id]);
      dep[idit->ret_id].push_back(*specit);
      notdep[idit->ret_id] = complementaryGVSpec(seen, dep[idit->ret_id]);
    } else {
      auto currdep = mergeGVSpec(dep[idit->left_id], dep[idit->right_id]);
      currdep.push_back(*specit);
      auto currnotdep = complementaryGVSpec(seen, currdep);
      /*
      llvm::errs() << "Building " << specit->second << "\n";
      printMutSpecsGV(seen);
      llvm::errs() << "\n";
      printMutSpecsGV(currdep);
      llvm::errs() << "\n";
      printMutSpecsGV(currnotdep);
      llvm::errs() << "\n";
       */
      currdep = collapseGVSpec(currdep);
      currnotdep = collapseGVSpec(currnotdep);

      mutSpecsDepGVs[specit->second] =
          getMutSpecsGVFromAllList(currdep.size(), currdep);
      mutSpecsNoDepGVs[specit->second] =
          getMutSpecsGVFromAllList(currnotdep.size(), currnotdep);
    }
  }
}

GlobalVariable *
WAInstrumenter::buildBlockBoundGV(const std::list<std::pair<int, int>> &gvspec,
                                  bool exactBound, int left, int right) {
  auto mergedGVSpec = collapseGVSpec(gvspec);
  Type *i8 = typeMapping["i8"];
  auto num0 = getI8Constant(0);
  auto num1 = getI8Constant(1);
  if (exactBound) {
    left = mergedGVSpec.front().first;
    right = mergedGVSpec.back().second;
  }

  if (mergedGVSpec.empty()) {
    assert(false);
    exit(-1);
  } else if (mergedGVSpec.size() == 1 && left == mergedGVSpec.front().first &&
             right == mergedGVSpec.back().second) {
    auto ty = dyn_cast<StructType>(typeMapping["BlockRegMutBound"]);
    auto init = dyn_cast<ConstantStruct>(
        ConstantStruct::get(ty, {getI32Constant(left), getI32Constant(right)}));
    auto gv = new GlobalVariable(
        *TheModule, ty, false, llvm::GlobalValue::LinkageTypes::InternalLinkage,
        init);
    auto constgv = new GlobalVariable(
        *TheModule, ty, true, llvm::GlobalValue::LinkageTypes::InternalLinkage,
        init);
    blockBoundGVs.push_back(gv);
    constBlockBoundGVs[gv] = constgv;
    return gv;
  } else {
    auto len = right - left + 1;
    auto ty = buildBlockBoundType(len);
    std::vector<Constant *> vec(len, num0);
    for (const auto &p : mergedGVSpec) {
      for (int i = p.first; i <= p.second; ++i) {
        vec[i - left] = num1;
      }
    }
    auto innerVal =
        ConstantArray::get(ArrayType::get(i8, len), ArrayRef<Constant *>(vec));
    auto init = ConstantStruct::get(
        ty, {getI32Constant(left), getI32Constant(right), innerVal});

    auto gv = new GlobalVariable(
        *TheModule, ty, false, llvm::GlobalValue::LinkageTypes::InternalLinkage,
        init);
    auto constgv = new GlobalVariable(
        *TheModule, ty, true, llvm::GlobalValue::LinkageTypes::InternalLinkage,
        init);
    blockBoundGVs.push_back(gv);
    constBlockBoundGVs[gv] = constgv;
    return gv;
  }
}

void WAInstrumenter::genGlobalBlockGVArray() {
  ArrayType *ty =
      ArrayType::get(PointerType::get(typeMapping["BlockRegMutBound"], 0),
                     blockBoundGVs.size());
  Type *blockRegMutBound = typeMapping["BlockRegMutBound"];
  Type *blockRegMutBoundPtr = PointerType::get(blockRegMutBound, 0);
  std::vector<Constant *> init;
  for (auto gv : blockBoundGVs) {
    if (gv->getType() == blockRegMutBound) {
      init.push_back(gv);
    } else {
      init.push_back(ConstantExpr::getPointerCast(gv, blockRegMutBoundPtr));
    }
  }
  blockGVArray = new GlobalVariable(
      *TheModule, ty, true, llvm::GlobalValue::LinkageTypes::InternalLinkage,
      ConstantArray::get(ty, ArrayRef<Constant *>(init)), "BlockBoundArray");
}

void WAInstrumenter::genGlobalMutSpecsArray() {
  Type *eleTy = PointerType::get(typeMapping["MutSpecs"], 0);
  ArrayType *ty = ArrayType::get(eleTy, mutSpecsGVs.size());
  std::vector<Constant *> init;
  for (auto &p : mutSpecsGVs) {
    init.push_back(ConstantExpr::getPointerCast(p.second, eleTy));
  }
  mutSpecsArray = new GlobalVariable(
      *TheModule, ty, true, llvm::GlobalVariable::LinkageTypes::InternalLinkage,
      ConstantArray::get(ty, ArrayRef<Constant *>(init)), "MutSpecsArray");
}

void WAInstrumenter::genGoodvarargArray() {
  Type *eleTy = PointerType::get(typeMapping["GoodvarArg"], 0);
  ArrayType *ty = ArrayType::get(eleTy, goodvarargGVs.size());
  std::vector<Constant *> init;
  for (auto &p : goodvarargGVs) {
    init.push_back(p.second);
  }
  goodvarargArray = new GlobalVariable(
      *TheModule, ty, true, llvm::GlobalVariable::LinkageTypes::InternalLinkage,
      ConstantArray::get(ty, ArrayRef<Constant *>(init)), "GoodvarArgArray");
}

Constant *WAInstrumenter::getMutationIDConstant(bool isLocal) {
  static Constant *ret = nullptr;
  static Constant *retlocal = nullptr;
  if (ret != nullptr) {
    if (isLocal) {
      return retlocal;
    } else {
      return ret;
    }
  }
  auto i32 = typeMapping["i32"];
  GlobalVariable *holder = new GlobalVariable(
      *TheModule, ArrayType::get(i32, 1024), false,
      llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr, "HOLDER");
  ret = ConstantExpr::getInBoundsGetElementPtr(
      holder->getValueType(), holder,
      ArrayRef<Constant *>({getI32Constant(0), getI32Constant(0)}));
  retlocal = ConstantExpr::getInBoundsGetElementPtr(
      holder->getValueType(), holder,
      ArrayRef<Constant *>({getI32Constant(0), getI32Constant(1)}));
  return ret;
}

std::vector<BasicBlock *>
WAInstrumenter::cloneFunctionBlocks(Function *F, int left, int right) {
  std::vector<BasicBlock *> ret;
  std::vector<BasicBlock *> cloned;
  ValueToValueMapTy VMap;
  BasicBlock *entry = nullptr, *mirrorEntry = nullptr;
  for (Argument &I : F->args()) {
    if (VMap.count(&I) == 0) { // Is this argument preserved?
      VMap[&I] = &I;
    }
  }
  for (auto BI = F->begin(), BE = F->end(); BI != BE; ++BI) {
    BasicBlock &BB = *BI;
    ret.push_back(&BB);

    // Create a new basic block and copy instructions into it!
    BasicBlock *CBB = CloneBasicBlock(&BB, VMap, ".mirror");
    if (entry == nullptr) {
      entry = &BB;
      mirrorEntry = CBB;
    }
    cloned.push_back(CBB);

    // Add basic block mapping.
    VMap[&BB] = CBB;
  }
  for (auto BB : cloned) {
    BB->insertInto(F);
  }

  for (auto BB : cloned) {
    for (Instruction &II : *BB) {
      RemapInstruction(&II, VMap);
    }
  }

  auto guard = BasicBlock::Create(F->getContext(), F->getName() + ".func.guard",
                                  F, &*F->begin());
  IRBuilder<> irbuilder(guard);
  auto mutationId = irbuilder.CreateLoad(getMutationIDConstant(false), "mutid");
  // auto mutationIdLocal =
  //    irbuilder.CreateLoad(getMutationIDConstant(true), "mutid.local");
  auto notzero =
      irbuilder.CreateICmpNE(mutationId, getI32Constant(0), "notzero");
  //  auto rmioff = irbuilder.CreateLoad(
  //      ConstantExpr::getInBoundsGetElementPtr(
  //          rmigv->getValueType(), rmigv,
  //          ArrayRef<Constant *>({getI32Constant(0), getI32Constant(2)})),
  //      "rmioff");

  auto gv = buildBlockBoundGV({{left, right}});

  auto leftLoad = irbuilder.CreateLoad(
      ConstantExpr::getInBoundsGetElementPtr(
          gv->getValueType(), gv,
          ArrayRef<Constant *>({getI32Constant(0), getI32Constant(0)})),
      "left");
  auto cmp1 = irbuilder.CreateICmpSLT(mutationId, leftLoad);

  auto rightLoad = irbuilder.CreateLoad(
      ConstantExpr::getInBoundsGetElementPtr(
          gv->getValueType(), gv,
          ArrayRef<Constant *>({getI32Constant(0), getI32Constant(1)})),
      "right");
  auto cmp2 = irbuilder.CreateICmpSGT(mutationId, rightLoad);
  auto orcmp = irbuilder.CreateOr(cmp1, cmp2, "or");
  auto andcmp = irbuilder.CreateAnd(orcmp, notzero, "and");

  /*irbuilder.CreateCall(
      funcMapping["__accmut__log"],
      ArrayRef<Value*>({mutationId, mutationIdLocal, rmioff, leftLoad,
     rightLoad, andcmp, getI32Constant(0)}));*/
  auto br = irbuilder.CreateCondBr(andcmp, mirrorEntry, entry);
  auto N = MDNode::get(br->getContext(),
                       {MDString::get(br->getContext(), "branch_weights"),
                        ConstantAsMetadata::get(getI32Constant(2000)),
                        ConstantAsMetadata::get(getI32Constant(4))});
  br->setMetadata(LLVMContext::MD_prof, N);
  return ret;
}

DuplicatedBlockResult
WAInstrumenter::duplicateBlock(BasicBlock *BB,
                               std::vector<ValueToValueMapTy> &vmapList) {
  int num = vmapList.size();
  auto phiguardBB =
      BasicBlock::Create(BB->getContext(), "", BB->getParent(), BB);
  if (BB->hasName())
    phiguardBB->setName(BB->getName() + ".phiguard");

  auto succBB = BasicBlock::Create(BB->getContext(), "", BB->getParent(),
                                   BB->getNextNode());
  if (BB->hasName())
    succBB->setName(BB->getName() + ".succ");

  BB->replaceSuccessorsPhiUsesWith(succBB);
  BB->replaceAllUsesWith(phiguardBB);

  for (auto it = BB->begin(); it != BB->end();) {
    auto cur_it = &*it;
    it++;
    if (auto *phi = dyn_cast<PHINode>(cur_it)) {
      phi->removeFromParent();
      phiguardBB->getInstList().push_back(phi);
    }
  }

  auto terminator = BB->getTerminator();
  terminator->removeFromParent();
  succBB->getInstList().push_back(terminator);

  IRBuilder<> irbuilder(BB);
  irbuilder.CreateBr(succBB);

  auto duplicateOne = [&vmapList](BasicBlock *BB1, int idx) {
    // BB1 have no phi node
    // The final instruction is br
    // no block use now

    auto collectBB = BasicBlock::Create(BB1->getContext(), "", BB1->getParent(),
                                        BB1->getNextNode());
    if (BB1->hasName())
      collectBB->setName(BB1->getName() + ".collect." + std::to_string(idx));
    BB1->replaceSuccessorsPhiUsesWith(collectBB);

    auto terminator = BB1->getTerminator();
    terminator->removeFromParent();
    collectBB->getInstList().push_back(terminator);
    IRBuilder<> irbuilder(BB1);
    irbuilder.CreateBr(collectBB);

    ValueToValueMapTy &vmap = vmapList[idx];
    auto clonedBB =
        llvm::CloneBasicBlock(BB1, vmap, ".cloned." + std::to_string(idx));
    clonedBB->insertInto(BB1->getParent(), collectBB);

    irbuilder.SetInsertPoint(collectBB, collectBB->begin());
    for (auto it = BB1->begin(); it != BB1->end(); ++it) {
      auto inst = &*it;
      auto clonedInst = dyn_cast<Instruction>(vmap[inst]);
      inst->replaceUsesOutsideBlock(clonedInst, BB1);
      if (clonedInst->isUsedOutsideOfBlock(clonedBB)) {
        auto phi = irbuilder.CreatePHI(inst->getType(), 2);
        if (inst->hasName()) {
          phi->setName(inst->getName() + ".phi");
        }
        clonedInst->replaceUsesOutsideBlock(phi, clonedBB);
        phi->addIncoming(inst, BB1);
        phi->addIncoming(clonedInst, clonedBB);
      }
    }

    return clonedBB;
  };

  DuplicatedBlockResult ret(BB, phiguardBB, vmapList);

  for (int i = 0; i < num; ++i) {
    ret.duplicated.push_back(duplicateOne(BB, i));
  }
  return ret;

  // for (auto )
}

SplittedBlocks WAInstrumenter::splitBasicBlock(BasicBlock *BB) {
  SplittedBlocks ret;
  ret.goodOnlyVmapIdx = -1;
  ret.badOnlyVmapIdx = -1;

  std::vector<Instruction *> goodInst;
  std::vector<Instruction *> badInst;
  std::list<std::pair<int, int>> goodSpec;
  std::list<std::pair<int, int>> badSpec;
  std::list<std::pair<int, int>> totSpec;

  totSpec = collapseGVSpec(totSpec);

  for (auto it = BB->rbegin(), end = BB->rend(); it != end;) {
    Instruction &I = *it;
    ++it;

    if (goodVariables.count(&I) == 1 || hasUsedPreviousGoodVariables(&I)) {
      if (instMutsMap.count(&I) == 1) {
        goodInst.push_back(&I);
        vector<Mutation *> &tmp = instMutsMap[&I];
        goodSpec.emplace_back(tmp.front()->id, tmp.back()->id);
        totSpec.emplace_back(tmp.front()->id, tmp.back()->id);
      }
    } else if (instMutsMap.count(&I) == 1) {
      badInst.push_back(&I);
      vector<Mutation *> &tmp = instMutsMap[&I];
      badSpec.emplace_back(tmp.front()->id, tmp.back()->id);
      totSpec.emplace_back(tmp.front()->id, tmp.back()->id);
    }
  }

  auto checkBB = [&](BasicBlock *bb) {
    for (auto &inst : *bb) {
      if (instMutsMap.count(&inst) == 1) {
        assert(false);
        // assert(false);
      }
    }
  };

  IRBuilder<> irbuilder(BB);
  if (goodInst.empty() && badInst.empty()) {
    // noBB
    ret.fullBB = nullptr;
    ret.badOnlyBB = nullptr;
    ret.goodOnlyBB = nullptr;
    ret.noBB = BB;
  } else if (goodInst.empty()) {
    ret.vmap = std::vector<ValueToValueMapTy>(1);
    auto dup = duplicateBlock(BB, ret.vmap);
    auto clonedBB = dup.duplicated[0];
    checkBB(clonedBB);
    auto phiguard = dup.phiguard;
    irbuilder.SetInsertPoint(BB);

    auto guard = BasicBlock::Create(BB->getContext(), "", BB->getParent(), BB);
    if (BB->hasName())
      guard->setName(BB->getName() + ".guard");

    irbuilder.SetInsertPoint(phiguard);
    irbuilder.CreateBr(guard);

    irbuilder.SetInsertPoint(guard);

    auto mutationIdLocal =
        irbuilder.CreateLoad(getMutationIDConstant(true), "mutid.local");
    auto notzero =
        irbuilder.CreateICmpNE(mutationIdLocal, getI32Constant(0), "notzero");

    badSpec = collapseGVSpec(badSpec);
    if (badSpec.size() != 1) {
      assert(false);
      exit(-1);
    }

    auto left = getI32Constant(badSpec.front().first);
    auto cmp1 = irbuilder.CreateICmpSLT(mutationIdLocal, left);

    auto right = getI32Constant(badSpec.front().second);
    auto cmp2 = irbuilder.CreateICmpSGT(mutationIdLocal, right);
    auto orcmp = irbuilder.CreateOr(cmp1, cmp2, "or");
    auto andcmp = irbuilder.CreateAnd(orcmp, notzero, "and");

    auto br = irbuilder.CreateCondBr(andcmp, clonedBB, BB);
    auto N = MDNode::get(br->getContext(),
                         {MDString::get(br->getContext(), "branch_weights"),
                          ConstantAsMetadata::get(getI32Constant(2000)),
                          ConstantAsMetadata::get(getI32Constant(4))});
    br->setMetadata(LLVMContext::MD_prof, N);
    ret.fullBB = BB;
    ret.badOnlyBB = nullptr;
    ret.goodOnlyBB = nullptr;
    ret.noBB = clonedBB;
  } else if (badInst.empty()) {
    ret.vmap = std::vector<ValueToValueMapTy>(2);
    auto dup = duplicateBlock(BB, ret.vmap);
    auto goodOnlyBB = dup.duplicated[0];
    // auto &goodOnlyVMap = ret.vmap[0];
    ret.goodOnlyVmapIdx = 0;
    auto clonedBB = dup.duplicated[1];
    auto phiguard = dup.phiguard;
    checkBB(goodOnlyBB);
    checkBB(clonedBB);

    /*
      addCounterPartMutants(BB, goodOnlyVMap, true);
      */

    irbuilder.SetInsertPoint(BB);

    auto zeroGuard =
        BasicBlock::Create(BB->getContext(), "", BB->getParent(), BB);
    auto rangeGuard =
        BasicBlock::Create(BB->getContext(), "", BB->getParent(), BB);
    if (BB->hasName()) {
      zeroGuard->setName(BB->getName() + ".guard.zero");
      rangeGuard->setName(BB->getName() + ".guard.range");
    }

    irbuilder.SetInsertPoint(phiguard);
    irbuilder.CreateBr(zeroGuard);

    irbuilder.SetInsertPoint(zeroGuard);

    auto mutationIdLocal =
        irbuilder.CreateLoad(getMutationIDConstant(true), "mutid.local");
    auto notzero =
        irbuilder.CreateICmpNE(mutationIdLocal, getI32Constant(0), "notzero");
    irbuilder.CreateCondBr(
        notzero, rangeGuard, BB,
        MDNode::get(zeroGuard->getContext(),
                    {MDString::get(zeroGuard->getContext(), "branch_weights"),
                     ConstantAsMetadata::get(getI32Constant(2000)),
                     ConstantAsMetadata::get(getI32Constant(4))}));

    irbuilder.SetInsertPoint(rangeGuard);

    goodSpec = collapseGVSpec(goodSpec);

    if (goodSpec.size() != 1) {
      assert(false);
      exit(-1);
    }

    auto left = getI32Constant(goodSpec.front().first);
    auto cmp1 = irbuilder.CreateICmpSLT(mutationIdLocal, left);

    auto right = getI32Constant(goodSpec.front().second);
    auto cmp2 = irbuilder.CreateICmpSGT(mutationIdLocal, right);
    auto orcmp = irbuilder.CreateOr(cmp1, cmp2, "or");

    irbuilder.CreateCondBr(
        orcmp, clonedBB, goodOnlyBB,
        MDNode::get(rangeGuard->getContext(),
                    {MDString::get(rangeGuard->getContext(), "branch_weights"),
                     ConstantAsMetadata::get(getI32Constant(2000)),
                     ConstantAsMetadata::get(getI32Constant(4))}));

    ret.fullBB = BB;
    ret.badOnlyBB = nullptr;
    ret.goodOnlyBB = goodOnlyBB;
    ret.noBB = clonedBB;
  } else {
    ret.vmap = std::vector<ValueToValueMapTy>(3);
    auto dup = duplicateBlock(BB, ret.vmap);
    auto goodOnlyBB = dup.duplicated[0];
    // auto &goodOnlyVMap = ret.vmap[0];
    ret.goodOnlyVmapIdx = 0;
    auto badOnlyBB = dup.duplicated[1];
    // auto &badOnlyVMap = ret.vmap[1];
    ret.badOnlyVmapIdx = 1;
    auto clonedBB = dup.duplicated[2];
    auto phiguard = dup.phiguard;
    checkBB(badOnlyBB);
    checkBB(goodOnlyBB);
    checkBB(clonedBB);

    /*
    addCounterPartMutants(BB, goodOnlyVMap, true);
    addCounterPartMutants(BB, badOnlyVMap, false);
     */

    irbuilder.SetInsertPoint(BB);

    auto zeroGuard =
        BasicBlock::Create(BB->getContext(), "", BB->getParent(), BB);
    auto rangeGuard =
        BasicBlock::Create(BB->getContext(), "", BB->getParent(), BB);
    auto selectGuard =
        BasicBlock::Create(BB->getContext(), "", BB->getParent(), BB);
    if (BB->hasName()) {
      zeroGuard->setName(BB->getName() + ".guard.zero");
      rangeGuard->setName(BB->getName() + ".guard.range");
      selectGuard->setName(BB->getName() + ".guard.select");
    }

    irbuilder.SetInsertPoint(phiguard);
    irbuilder.CreateBr(zeroGuard);

    irbuilder.SetInsertPoint(zeroGuard);

    auto mutationIdLocal =
        irbuilder.CreateLoad(getMutationIDConstant(true), "mutid.local");
    auto notzero =
        irbuilder.CreateICmpNE(mutationIdLocal, getI32Constant(0), "notzero");
    irbuilder.CreateCondBr(
        notzero, rangeGuard, BB,
        MDNode::get(zeroGuard->getContext(),
                    {MDString::get(zeroGuard->getContext(), "branch_weights"),
                     ConstantAsMetadata::get(getI32Constant(2000)),
                     ConstantAsMetadata::get(getI32Constant(4))}));

    irbuilder.SetInsertPoint(rangeGuard);

    totSpec = collapseGVSpec(totSpec);

    if (totSpec.size() != 1) {
      assert(false);
      exit(-1);
    }

    auto left = getI32Constant(totSpec.front().first);
    auto cmp1 = irbuilder.CreateICmpSLT(mutationIdLocal, left);

    auto right = getI32Constant(totSpec.front().second);
    auto cmp2 = irbuilder.CreateICmpSGT(mutationIdLocal, right);
    auto orcmp = irbuilder.CreateOr(cmp1, cmp2, "or");

    irbuilder.CreateCondBr(
        orcmp, clonedBB, selectGuard,
        MDNode::get(rangeGuard->getContext(),
                    {MDString::get(rangeGuard->getContext(), "branch_weights"),
                     ConstantAsMetadata::get(getI32Constant(2000)),
                     ConstantAsMetadata::get(getI32Constant(4))}));

    goodSpec = collapseGVSpec(goodSpec);
    badSpec = collapseGVSpec(badSpec);
    irbuilder.SetInsertPoint(selectGuard);

    if (goodSpec.size() == 1) {
      auto leftg = getI32Constant(goodSpec.front().first);
      auto cmp1g = irbuilder.CreateICmpSLT(mutationIdLocal, leftg);
      auto rightg = getI32Constant(goodSpec.front().second);
      auto cmp2g = irbuilder.CreateICmpSGT(mutationIdLocal, rightg);
      auto orcmpg = irbuilder.CreateOr(cmp1g, cmp2g, "or");

      irbuilder.CreateCondBr(orcmpg, badOnlyBB, goodOnlyBB);
    } else if (badSpec.size() == 1) {
      auto leftb = getI32Constant(badSpec.front().first);
      auto cmp1b = irbuilder.CreateICmpSLT(mutationIdLocal, leftb);
      auto rightb = getI32Constant(badSpec.front().second);
      auto cmp2b = irbuilder.CreateICmpSGT(mutationIdLocal, rightb);
      auto orcmpb = irbuilder.CreateOr(cmp1b, cmp2b, "or");

      irbuilder.CreateCondBr(orcmpb, goodOnlyBB, badOnlyBB);
    } else {
      auto gv = constBlockBoundGVs[buildBlockBoundGV(
          goodSpec, false, totSpec.front().first, totSpec.front().second)];

      if (gv->getValueType() == typeMapping["BlockRegMutBound"]) {
        assert(false);
        exit(-1);
      } else {

        auto sub = irbuilder.CreateSub(mutationIdLocal,
                                       getI32Constant(totSpec.front().first));
        auto gep = irbuilder.CreateInBoundsGEP(
            gv, ArrayRef<Value *>({getI32Constant(0), getI32Constant(2), sub}));
        auto ok = irbuilder.CreateLoad(gep);
        auto cmp = irbuilder.CreateICmpEQ(ok, getI8Constant(1));
        irbuilder.CreateCondBr(cmp, goodOnlyBB, badOnlyBB);
      }
    }

    ret.fullBB = BB;
    ret.badOnlyBB = badOnlyBB;
    ret.goodOnlyBB = goodOnlyBB;
    ret.noBB = clonedBB;
  }

  return ret;
}

bool WAInstrumenter::readInMuts() {

  MutUtil::getAllMutations(TheModule->getName());

  vector<Mutation *> &AllMutsVec = MutUtil::AllMutsVec;

  if (AllMutsVec.size() == 0)
    return false;

  auto i8 = typeMapping["i8"];
  auto i32 = typeMapping["i32"];
  auto i64 = typeMapping["i64"];

  auto *mutationTy = dyn_cast<StructType>(typeMapping["Mutation"]);

  std::vector<Constant *> vec;
  vec.reserve(AllMutsVec.size());
  for (uint64_t i = 0; i < AllMutsVec.size(); ++i) {
    Mutation *mut = AllMutsVec[i];
    int type = mut->getKind();
    int sop = mut->src_op;
    int op_0 = 0;
    long op_1 = 0;
    long op_2 = 0;
    switch (type) {
    case Mutation::MK_AOR:
      op_0 = dyn_cast<AORMut>(mut)->tar_op;
      break;
    case Mutation::MK_LOR:
      op_0 = dyn_cast<LORMut>(mut)->tar_op;
      break;
    case Mutation::MK_ROR:
      op_1 = dyn_cast<RORMut>(mut)->src_pre;
      op_2 = dyn_cast<RORMut>(mut)->tar_pre;
      break;
    case Mutation::MK_STD:
      op_1 = dyn_cast<STDMut>(mut)->func_ty;
      op_2 = dyn_cast<STDMut>(mut)->retval;
      break;
    case Mutation::MK_LVR:
      op_0 = dyn_cast<LVRMut>(mut)->oper_index;
      op_1 = dyn_cast<LVRMut>(mut)->src_const;
      op_2 = dyn_cast<LVRMut>(mut)->tar_const;
      break;
    case Mutation::MK_UOI:
      op_1 = dyn_cast<UOIMut>(mut)->oper_index;
      op_2 = dyn_cast<UOIMut>(mut)->ury_tp;
      break;
    case Mutation::MK_ROV:
      op_1 = dyn_cast<ROVMut>(mut)->op1;
      op_2 = dyn_cast<ROVMut>(mut)->op2;
      break;
    case Mutation::MK_ABV:
      op_0 = dyn_cast<ABVMut>(mut)->oper_index;
      break;
    default:
      break;
    }
    vec.push_back((ConstantStruct::get(
        mutationTy,
        {ConstantInt::get(i32, type, true), ConstantInt::get(i32, sop, true),
         ConstantInt::get(i32, op_0, true), ConstantInt::get(i64, op_1, true),
         ConstantInt::get(i64, op_2, true)})));
  }
  auto *a = dyn_cast<ConstantArray>(ConstantArray::get(
      ArrayType::get(mutationTy, vec.size()), ArrayRef<Constant *>(vec)));
  GlobalVariable *gv1 = new GlobalVariable(
      *TheModule, ArrayType::get(mutationTy, vec.size()), false,
      llvm::GlobalValue::LinkageTypes::InternalLinkage, a, "mutarray", nullptr,
      llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, 0);

  auto *regmutinfo = dyn_cast<StructType>(typeMapping["RegMutInfo"]);

  Constant *gep = ConstantExpr::getInBoundsGetElementPtr(
      nullptr, gv1,
      ArrayRef<Constant *>({
          ConstantInt::get(i32, 0, false),
          ConstantInt::get(i32, 0, false),
      }));
  auto *str = ConstantDataArray::getString(TheModule->getContext(),
                                           TheModule->getName(), true);

  auto *gvstr =
      new GlobalVariable(*TheModule, str->getType(), str,
                         llvm::GlobalValue::LinkageTypes::InternalLinkage, str);

  auto *strptr = ConstantExpr::getInBoundsGetElementPtr(
      gvstr->getValueType(), gvstr,
      ArrayRef<Constant *>{ConstantInt::get(i32, 0, false),
                           ConstantInt::get(i32, 0, false)});
  auto rmi = dyn_cast<ConstantStruct>(ConstantStruct::get(
      regmutinfo, {gep, ConstantInt::get(i32, vec.size(), false),
                   ConstantInt::get(i32, 0, false),
                   ConstantInt::get(i32, 0, false), strptr}));
  rmigv = new GlobalVariable(
      *TheModule, regmutinfo, false,
      llvm::GlobalValue::LinkageTypes::InternalLinkage, rmi, "mutrmi", nullptr,
      llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, 0);

  return true;
}

bool WAInstrumenter::runOnModule(Module &M) {

  TheModule = &M;
  name = M.getName();

  statFile = fopen((name + ".stat").c_str(), "w");
  initialTypes();
  initialFuncs();

  if (!readInMuts()) {
    return false;
  }

  for (Function &F : M) {
    if (!F.isDeclaration()) {
      runOnFunction(F);
    }
  }
  genGlobalBlockGVArray();
  genGlobalMutSpecsArray();
  genGoodvarargArray();
  generateRegisterCtor();
  generateSetMaxCtor();

  /*
  for (Function &F : M) {
    if (!F.isDeclaration()) {
      if (F.getName().startswith("__accmut__")) {
        continue;
      }

      if (F.getName().equals("main")) {
        continue;
      }
      generateRegister(F);
    }
  }
   */

  return true;
}

bool WAInstrumenter::runOnFunction(Function &F) {
  if (F.getName().startswith("__accmut__")) {
    return false;
  }

  for (const auto &name : skipFunctionList) {
    if (F.getName().equals(name)) {
      return true;
    }
  }

  vector<Mutation *> *v = MutUtil::AllMutsMap[F.getName()];

  if (v == nullptr || v->empty()) {
    return false;
  }
#ifdef OUTPUT
  errs() << "\n######## WA INSTRUMTNTING MUT @ " << F.getName()
         << "()  ########\n\n";
#endif

  getInstMutsMap(v, F);

  if (optimizedInstrumentation) {
    auto toInstrumentBBs =
        cloneFunctionBlocks(&F, v->front()->id, v->back()->id);

    for (auto it = toInstrumentBBs.begin(), end = toInstrumentBBs.end();
         it != end;) {
      // avoid iterator invalidation
      BasicBlock *BB = *it;
      ++it;

      if (useWindowAnalysis) {
        getGoodVariables(*BB);
        filterRealGoodVariables();
      }

      /// split here
      auto splitted = splitBasicBlock(BB);

      if (splitted.fullBB) {
        if (splitted.goodOnlyBB) {
          std::vector<BasicBlock *> blocks;
          blocks.push_back(splitted.fullBB);
          blocks.push_back(splitted.goodOnlyBB);

          CallInst::Create(funcMapping["__accmut__GoodVar_TablePush_max"], "",
                           splitted.fullBB->getFirstNonPHI());
          std::vector<Value *> params;
          params.push_back(getI64Constant(numOfGoodVar));
          params.push_back(getI64Constant(numOfMutants));
          CallInst::Create(funcMapping["__accmut__GoodVar_TablePush"], params,
                           "", splitted.goodOnlyBB->getFirstNonPHI());
          for (auto *BB : blocks) {
            CallInst::Create(funcMapping["__accmut__GoodVar_TablePop"], "",
                             BB->getTerminator());
          }
        }
        instrumentMulti(splitted.fullBB, splitted.vmap,
                        splitted.goodOnlyVmapIdx, splitted.badOnlyVmapIdx);
        if (splitted.goodOnlyBB) {
          canFuseGoodVarInst(splitted.goodOnlyBB);
        }
      }
    }
  } else {
    for (auto it = F.begin(), end = F.end(); it != end;) {
      auto &BB = *it;
      ++it;
      if (useWindowAnalysis) {
        getGoodVariables(BB);
        filterRealGoodVariables();
      }
      if (!goodVariables.empty()) {
        CallInst::Create(funcMapping["__accmut__GoodVar_TablePush_max"], "",
                         BB.getFirstNonPHI());
        std::vector<Value *> params;
        params.push_back(getI64Constant(numOfGoodVar));
        params.push_back(getI64Constant(numOfMutants));
        CallInst::Create(funcMapping["__accmut__GoodVar_TablePop"], "",
                         BB.getTerminator());
      }
      auto mapping = std::vector<ValueToValueMapTy>{};
      instrumentMulti(&BB, mapping, -1, -1);
    }
  }

  return true;
}

void WAInstrumenter::generateRegisterCtor() {
  auto *func =
      Function::Create(dyn_cast<FunctionType>(typeMapping["returnvoidFuncTy"]),
                       GlobalValue::LinkageTypes::InternalLinkage,
                       "__accmut_register_ctor", TheModule);
  auto *bb = BasicBlock::Create(TheModule->getContext(), "", func);
  ArrayRef<Constant *>({getI32Constant(0), getI32Constant(0)});
  CallInst::Create(
      funcMapping["__accmut__register"],
      {rmigv,
       ConstantExpr::getInBoundsGetElementPtr(
           blockGVArray->getValueType(), blockGVArray,
           ArrayRef<Constant *>({getI32Constant(0), getI32Constant(0)})),
       getI32Constant(blockBoundGVs.size()),
       ConstantExpr::getInBoundsGetElementPtr(
           mutSpecsArray->getValueType(), mutSpecsArray,
           ArrayRef<Constant *>({getI32Constant(0), getI32Constant(0)})),
       getI32Constant(mutSpecsGVs.size()),
       ConstantExpr::getInBoundsGetElementPtr(
           goodvarargArray->getValueType(), goodvarargArray,
           ArrayRef<Constant *>({getI32Constant(0), getI32Constant(0)})),
       getI32Constant(goodvarargGVs.size())},
      "", bb);
  ReturnInst::Create(TheModule->getContext(), bb);

  appendToGlobalCtors(*TheModule, func, 100);
}

void WAInstrumenter::generateSetMaxCtor() {
  auto *func =
      Function::Create(dyn_cast<FunctionType>(typeMapping["returnvoidFuncTy"]),
                       GlobalValue::LinkageTypes::InternalLinkage,
                       "__accmut_set_max_ctor", TheModule);
  auto *bb = BasicBlock::Create(TheModule->getContext(), "", func);
  ArrayRef<Constant *>({getI32Constant(0), getI32Constant(0)});
  CallInst::Create(
      funcMapping["__accmut__set_max_num"],
      {getI64Constant(maxNumOfGoodVar), getI64Constant(maxNumOfMutants)}, "",
      bb);
  ReturnInst::Create(TheModule->getContext(), bb);

  appendToGlobalCtors(*TheModule, func, 100);
}

bool WAInstrumenter::instrumentMulti(BasicBlock *BB,
                                     std::vector<ValueToValueMapTy> &mappings,
                                     int good_idx, int bad_idx) {
  bool aboutGoodVariables = false;
  int good_from = -1, good_to = -1;
  Instruction *first_goodvar_inst = nullptr;
#ifdef OUTPUT
  bool should_exit = false;
#endif
  std::list<std::pair<int, int>> mutSpecList;
  std::list<IDTuple> mutIDList;

  int numOfGoodVarCaching = 0;
  int numOfGoodVarForking = 0;
  int numOfBadVar = 0;

  for (auto &I : *BB) {
    if (instMutsMap.count(&I) == 1) {
      if (goodVariables.count(&I) == 1 || hasUsedPreviousGoodVariables(&I)) {
        auto &muts = instMutsMap[&I];
        mutSpecList.emplace_back(muts.front()->id, muts.back()->id);
        int left_id = getGoodVarId(I.getOperand(0));
        int right_id = getGoodVarId(I.getOperand(1));
        int ret_id = getGoodVarId(&I);
        mutIDList.emplace_back(left_id, right_id, ret_id);
        if (ret_id == 0)
          ++numOfGoodVarForking;
        else
          ++numOfGoodVarCaching;
        if (!muts.empty()) {
          if (good_from == -1) {
            good_from = muts.front()->id;
            good_to = muts.back()->id;
          } else {
#ifdef OUTPUT
            if (muts.back()->id + 1 != good_from) {
              should_exit = true;
            }
#endif
            good_to = muts.back()->id;
          }
        }
        if (!first_goodvar_inst)
          first_goodvar_inst = &I;
      } else {
        ++numOfBadVar;
      }
    }
  }

  if (statFile) {
    fprintf(statFile, "%d:%d:%d\n", numOfGoodVarCaching, numOfGoodVarForking,
            numOfBadVar);
  }

  if (!mutSpecList.empty()) {
    buildMutSpecsGV(mutSpecList);
    buildMutDepSpecsGV(mutSpecList, mutIDList);

    std::vector<GlobalVariable *> gvs;
    for (auto &I : *BB) {
      if (goodVariables.count(&I) == 1 || hasUsedPreviousGoodVariables(&I)) {
        if (instMutsMap.count(&I) == 1) {
          auto &muts = instMutsMap[&I];
          int from = muts.front()->id;
          int to = muts.back()->id;
          auto arggv = new GlobalVariable(
              *TheModule, typeMapping["GoodvarArg"], false,
              llvm::GlobalValue::LinkageTypes::InternalLinkage, nullptr,
              "goodvararg_" + std::to_string(from) + "_" + std::to_string(to),
              nullptr, llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, 0);
          goodvarargGVs[to] = arggv;
          gvs.push_back(arggv);
        }
      }
    }

    Type *eleTy = PointerType::get(typeMapping["GoodvarArg"], 0);
    ArrayType *ty = ArrayType::get(eleTy, gvs.size());
    std::vector<Constant *> initvec;
    for (auto *p : gvs) {
      initvec.push_back(p);
    }
    Constant *initarr = ConstantArray::get(ty, initvec);

    auto gvtype = buildGoodvarArgInBlockType(gvs.size());
    auto gvsgv = new GlobalVariable(
        *TheModule, gvtype, true,
        llvm::GlobalVariable::LinkageTypes::InternalLinkage,
        ConstantStruct::get(gvtype, {getI32Constant(gvs.size()), initarr}),
        "GoodvarArgInBlock_" + std::to_string(good_from) + "_" +
            std::to_string(good_to));

    for (auto &I : *BB) {
      if (goodVariables.count(&I) == 1 || hasUsedPreviousGoodVariables(&I)) {
        if (instMutsMap.count(&I) == 1) {
          auto &muts = instMutsMap[&I];
          int from = muts.front()->id;
          int to = muts.back()->id;
          auto arggv = goodvarargGVs[to];
          int left_id = getGoodVarId(I.getOperand(0));
          int right_id = getGoodVarId(I.getOperand(1));
          int ret_id = getGoodVarId(&I);
          int op = I.isBinaryOp() ? I.getOpcode()
                                  : (int)dyn_cast<ICmpInst>(&I)->getPredicate();
          auto *mutspecsPtrTy = PointerType::get(typeMapping["MutSpecs"], 0);
          auto *dep =
              dyn_cast<Constant>(ConstantPointerNull::get(mutspecsPtrTy));
          auto *notdep =
              dyn_cast<Constant>(ConstantPointerNull::get(mutspecsPtrTy));
          if (ret_id == 0) {
            dep =
                ConstantExpr::getPointerCast(mutSpecsDepGVs[to], mutspecsPtrTy);
            notdep = ConstantExpr::getPointerCast(mutSpecsNoDepGVs[to],
                                                  mutspecsPtrTy);
          }

          auto arg = dyn_cast<ConstantStruct>(ConstantStruct::get(
              dyn_cast<StructType>(typeMapping["GoodvarArg"]),
              {getI32Constant(0), getI32Constant(from), getI32Constant(to),
               getI32Constant(from), getI32Constant(to),
               getI32Constant(good_from), getI32Constant(good_to),
               getI32Constant(ret_id), getI32Constant(left_id),
               getI32Constant(right_id), getI32Constant(op),
               ConstantExpr::getPointerCast(mutSpecsGVs[to], mutspecsPtrTy),
               dep, notdep,
               ConstantExpr::getPointerCast(
                   gvsgv,
                   PointerType::get(typeMapping["GoodvarArgInBlock"], 0)),
               rmigv}));
          arggv->setInitializer(arg);
        }
      }
    }
  }

  for (auto it = BB->rbegin(), end = BB->rend(); it != end;) {
    // errs() << it.getNodePtr() << "---" << it->getPrevNode() << "\n";
    // errs() << "\n---F---" << *BB.getParent() << "---F---\n";

    // avoid iterator invalidation
    Instruction &I = *it;
    ++it;

    auto cur_it = &I;
    if (instMutsMap.count(cur_it) == 1) {
      vector<Mutation *> &tmp = instMutsMap[cur_it];
      int mut_from, mut_to;

      mut_from = tmp.front()->id;
      mut_to = tmp.back()->id;

      if (tmp.size() >= MAX_MUT_NUM_PER_LOCATION) {
        llvm::errs() << I << "\n";
        llvm::errs() << tmp.size() << "\n";
        assert(false);
        exit(-1);
      }

      if (goodVariables.count(&I) == 1 || hasUsedPreviousGoodVariables(&I)) {
        int left_id = getGoodVarId(I.getOperand(0));
        int right_id = getGoodVarId(I.getOperand(1));
        int ret_id = getGoodVarId(&I);
        if (good_idx != -1) {
          /*
          llvm::errs() << "FROM" << I << "\n";
          for (auto m : mappings[good_idx]) {
            llvm::errs() << *m.first << " to ";
            llvm::errs() << *m.second << "\n";
          }*/
          auto inst = dyn_cast<Instruction>(mappings[good_idx][&I]);
          assert(inst);
          instrumentAboutGoodVariable(*inst, mut_from, mut_to, good_from,
                                      good_to, &I == first_goodvar_inst,
                                      left_id, right_id, ret_id);
        }
        // llvm::errs() << "TO" << I << "\n";
        instrumentAboutGoodVariable(I, mut_from, mut_to, good_from, good_to,
                                    &I == first_goodvar_inst, left_id, right_id,
                                    ret_id);
        aboutGoodVariables = true;
      } else {
        if (bad_idx != -1) {
          /*
          llvm::errs() << "FROM" << I << "\n";
          for (auto m : mappings[bad_idx]) {
            llvm::errs() << *m.first << " to ";
            llvm::errs() << *m.second << "\n";
          }*/
          auto inst = dyn_cast<Instruction>(mappings[bad_idx][&I]);
          assert(inst);
          instrumentAsDMA(*inst, mut_from, mut_to);
        }
        // llvm::errs() << "TO" << I << "\n";
        instrumentAsDMA(I, mut_from, mut_to);
      }
      instMutsMap.erase(&I);

    } else {
      if (goodVariables.count(&I) == 1 || hasUsedPreviousGoodVariables(&I)) {
        assert(false);
      }
    }
  }
  return aboutGoodVariables;
}

void WAInstrumenter::instrumentAboutGoodVariable(Instruction &I, int mut_from,
                                                 int mut_to, int good_from,
                                                 int good_to, bool is_first,
                                                 int left_id, int right_id,
                                                 int ret_id) {
  instrumentInst(I, true, mut_from, mut_to, good_from, good_to, is_first,
                 left_id, right_id, ret_id);
}

void WAInstrumenter::instrumentAsDMA(Instruction &I, int mut_from, int mut_to) {
  instrumentInst(I, false, mut_from, mut_to, 0, 0, false, 0, 0, 0);
}

#ifdef OUTPUT

#define VALERRMSG(it, msg, cp)                                                 \
  llvm::errs() << "\tCUR_IT:\t" << (*(it)) << "\n\t" << (msg) << ":\t"         \
               << (*(cp)) << "\n"

#define ERRMSG(msg)                                                            \
  llvm::errs() << (msg) << " @ " << __FILE__ << "->" << __FUNCTION__           \
               << "():" << __LINE__ << "\n"
#else
#define VALERRMSG(it, msg, cp) (cp)
#define ERRMSG(msg)
#endif

// TYPE BITS OF SIGNATURE
#define CHAR_TP 0
#define SHORT_TP 1
#define INT_TP 2
#define LONG_TP 3

static int getTypeMacro(Type *t) {
  int res = -1;
  if (t->isIntegerTy()) {
    unsigned width = t->getIntegerBitWidth();
    switch (width) {
    case 8:
      res = CHAR_TP;
      break;
    case 16:
      res = SHORT_TP;
      break;
    case 32:
      res = INT_TP;
      break;
    case 64:
      res = LONG_TP;
      break;
    default:
      ERRMSG("OMMITNG PARAM TYPE");
      llvm::errs() << *t << '\n';
      assert(false);
      exit(-1);
    }
  }
  return res;
}

static bool isHandledCoveInst(Instruction *inst) {
  return inst->getOpcode() == Instruction::Trunc ||
         inst->getOpcode() == Instruction::ZExt ||
         inst->getOpcode() == Instruction::SExt ||
         inst->getOpcode() == Instruction::BitCast;
  // TODO:: PtrToInt, IntToPtr, AddrSpaceCast and float related Inst are not
  // handled
}

static bool pushPreparecallParam(std::vector<Value *> &params, int index,
                                 Value *OI, Module *TheModule) {
  Type *OIType = (dyn_cast<Value>(&*OI))->getType();
  int tp = getTypeMacro(OIType);
  if (tp < 0) {
    return false;
  }

  std::stringstream ss;
  // push type
  ss.str("");
  unsigned short tp_and_idx = ((unsigned short)tp) << 8;
  tp_and_idx = tp_and_idx | index;
  ss << tp_and_idx;
  ConstantInt *c_t_a_i = ConstantInt::get(TheModule->getContext(),
                                          APInt(16, StringRef(ss.str()), 10));

  // now push the pointer of idx'th param
  if (LoadInst *ld = dyn_cast<LoadInst>(&*OI)) { // is a local var
    Value *ptr_of_ld = ld->getPointerOperand();
    // if the pointer of loadInst dose not point to an integer
    if (SequentialType *t = dyn_cast<SequentialType>(ptr_of_ld->getType())) {
      if (!t->getElementType()->isIntegerTy()) { // TODO: for i32**
        ERRMSG("WARNNING ! Trying to push a none-i32* !! ");
        return false;
      }
    }
    params.push_back(c_t_a_i);
    params.push_back(ptr_of_ld);
  } /*else if(AllocaInst *alloca = dyn_cast<AllocaInst>(&*OI)){// a param of
   the F, fetch it by alloca params.push_back(c_t_a_i);
   params.push_back(alloca); }else if(GetElementPtrInst *ge =
   dyn_cast<GetElementPtrInst>(&*OI)){
       // TODO: test
       params.push_back(c_t_a_i);
       params.push_back(ge);
   }*/
  // TODO:: for Global Pointer ?!
  else {
    ERRMSG("CAN NOT GET A POINTER");
#ifdef OUTPUT
    Value *v = dyn_cast<Value>(&*OI);
    llvm::errs() << "\tCUR_OPREAND:\t" << v << "\n";
#endif
    // v->dump();
    assert(false);
    exit(-1);
  }
  return true;
}

void WAInstrumenter::instrumentCallInst(CallInst *cur_it, int mut_from,
                                        int mut_to) {
  // move all constant literal and SSA value to repalce to alloca, e.g
  // foo(a+5)->b = a+5;foo(b)

  for (auto OI = cur_it->op_begin(), OE = cur_it->op_end(); OI != OE; ++OI) {
    Value *V = OI->get();
    Type *type = V->getType();
    if (type->isIntegerTy(32) || type->isIntegerTy(64)) {
      AllocaInst *alloca =
          new AllocaInst(type, 0, (V->getName().str() + ".alias"), &*cur_it);
      /*StoreInst *str = */ new StoreInst(V, alloca, &*cur_it);
      LoadInst *ld =
          new LoadInst(alloca, (V->getName().str() + ".ld"), &*cur_it);
      *OI = (Value *)ld;
    }
  }

  Function *precallfunc = funcMapping["__accmut__prepare_call"];

  std::vector<Value *> params;
  params.push_back(rmigv);
  std::stringstream ss;
  ss << mut_from;
  ConstantInt *from_i32 = ConstantInt::get(TheModule->getContext(),
                                           APInt(32, StringRef(ss.str()), 10));
  params.push_back(from_i32);
  ss.str("");
  ss << mut_to;
  ConstantInt *to_i32 = ConstantInt::get(TheModule->getContext(),
                                         APInt(32, StringRef(ss.str()), 10));
  params.push_back(to_i32);

  int index = 0;
  int record_num = 0;
  std::vector<int> pushed_param_idx;

  // get signature info
  for (auto OI = cur_it->op_begin(), OE = cur_it->op_end() - 1; OI != OE;
       ++OI, ++index) {

    Value *v = OI->get();
    Type *type = v->getType();
    if (!(type->isIntegerTy(32) || type->isIntegerTy(64))) {
      continue;
    }
    bool succ = pushPreparecallParam(params, index, v, TheModule);

    if (succ) {
      pushed_param_idx.push_back(index);
      record_num++;
    } else {
      assert(false);
      // ERRMSG("---- WARNNING : PUSH PARAM FAILURE ");
      // VALERRMSG(cur_it,"CUR_OPRAND",v);
    }
  }

#ifdef OUTPUT
  if (!pushed_param_idx.empty()) {
    llvm::errs() << "---- PUSH PARAM : ";
    for (auto it = pushed_param_idx.begin(), ie = pushed_param_idx.end();
         it != ie; ++it) {
      llvm::errs() << *it << "'th\t";
    }
    llvm::errs() << "\n";
    llvm::errs() << *cur_it << "\n";
  }
#endif

  // insert num of param-records
  ss.str("");
  ss << record_num;
  ConstantInt *rcd = ConstantInt::get(TheModule->getContext(),
                                      APInt(32, StringRef(ss.str()), 10));
  params.insert(params.begin() + 3, rcd);

  CallInst *pre = CallInst::Create(precallfunc, params, "", &*cur_it);
  pre->setCallingConv(CallingConv::C);
  pre->setTailCall(false);
  AttributeList preattrset;
  pre->setAttributes(preattrset);

  ConstantInt *zero =
      ConstantInt::get(Type::getInt32Ty(TheModule->getContext()), 0);

  ICmpInst *hasstd =
      new ICmpInst(&*cur_it, ICmpInst::ICMP_EQ, pre, zero, "hasstd.call");

  BasicBlock *cur_bb = cur_it->getParent();

  Instruction *oricall = cur_it->clone();

  BasicBlock *label_if_end = cur_bb->splitBasicBlock(cur_it, "stdcall.if.end");

  BasicBlock *label_if_then =
      BasicBlock::Create(TheModule->getContext(), "stdcall.if.then",
                         cur_bb->getParent(), label_if_end);
  BasicBlock *label_if_else =
      BasicBlock::Create(TheModule->getContext(), "stdcall.if.else",
                         cur_bb->getParent(), label_if_end);

  ReplaceInstWithInst(&cur_bb->back(),
                      BranchInst::Create(label_if_then, label_if_else, hasstd));

  // label_if_then
  // move the loadinsts of params into if_then_block
  index = 0;
  for (auto OI = oricall->op_begin(), OE = oricall->op_end() - 1; OI != OE;
       ++OI, ++index) {

    // only move pushed parameters
    if (find(pushed_param_idx.begin(), pushed_param_idx.end(), index) ==
        pushed_param_idx.end()) {
      continue;
    }

    if (LoadInst *ld = dyn_cast<LoadInst>(&*OI)) {
      ld->removeFromParent();
      label_if_then->getInstList().push_back(ld);
    } else if (/*Constant *con = */ dyn_cast<Constant>(&*OI)) {
      // TODO::  test
      assert(false);
      continue;
    } else if (/*GetElementPtrInst *ge = */ dyn_cast<GetElementPtrInst>(&*OI)) {
      assert(false);
      // TODO: test
      // ge->removeFromParent();
      // label_if_then->getInstList().push_back(ge);
    } else {
      assert(false);
      // TODO:: check
      // TODO:: instrumented_insts !!!
      Instruction *coversion = dyn_cast<Instruction>(&*OI);
      if (isHandledCoveInst(coversion)) {
        Instruction *op_of_cov =
            dyn_cast<Instruction>(coversion->getOperand(0));
        if (dyn_cast<LoadInst>(&*op_of_cov) ||
            dyn_cast<AllocaInst>(&*op_of_cov)) {
          op_of_cov->removeFromParent();
          label_if_then->getInstList().push_back(op_of_cov);
          coversion->removeFromParent();
          label_if_then->getInstList().push_back(coversion);
        } else {
          ERRMSG("CAN MOVE GET A POINTER INTO IF.THEN");
#ifdef OUTPUT
          Value *v = dyn_cast<Value>(&*OI);
          VALERRMSG(cur_it, "CUR_OPRAND", v);
#endif
          assert(false);
          exit(-1);
        }
      } else {
        ERRMSG("CAN MOVE GET A POINTER INTO IF.THEN");
#ifdef OUTPUT
        Value *v = dyn_cast<Value>(&*OI);
        VALERRMSG(cur_it, "CUR_OPRAND", v);
#endif
        assert(false);
        exit(-1);
      }
    }
  }
  label_if_then->getInstList().push_back(oricall);
  BranchInst::Create(label_if_end, label_if_then);
  // errs() << "if_then end\n";

  // label_if_else
  Function *std_handle;
  if (oricall->getType()->isIntegerTy(32)) {
    std_handle = funcMapping["__accmut__stdcall_i32"];
  } else if (oricall->getType()->isIntegerTy(64)) {
    std_handle = funcMapping["__accmut__stdcall_i64"];
  } else if (oricall->getType()->isVoidTy()) {
    std_handle = funcMapping["__accmut__stdcall_void"];
  } else {
    ERRMSG("ERR CALL TYPE ");
    // oricall->dump();
    // oricall->getType()->dump();
    assert(false);
    exit(-1);
  } //}else if(oricall->getType()->isPointerTy()){

  CallInst *stdcall = CallInst::Create(std_handle, "", label_if_else);
  stdcall->setCallingConv(CallingConv::C);
  stdcall->setTailCall(false);
  AttributeList stdcallPAL;
  stdcall->setAttributes(stdcallPAL);
  BranchInst::Create(label_if_end, label_if_else);

  // errs() << "if_else end\n";
  // label_if_end
  if (oricall->getType()->isVoidTy()) {
    cur_it->eraseFromParent();
  } else {
    PHINode *call_res = PHINode::Create(oricall->getType(), 2, "call.phi");
    call_res->addIncoming(oricall, label_if_then);
    call_res->addIncoming(stdcall, label_if_else);
    ReplaceInstWithInst(&*cur_it, call_res);
  }
}

void WAInstrumenter::instrumentStoreInst(StoreInst *st, int mut_from,
                                         int mut_to) {
  // TODO:: add or call inst?
  if (ConstantInt *cons = dyn_cast<ConstantInt>(st->getValueOperand())) {
    AllocaInst *alloca = new AllocaInst(cons->getType(), 0, "cons_alias", st);
    /*StoreInst *str = */ new StoreInst(cons, alloca, st);
    LoadInst *ld = new LoadInst(alloca, "const_load", st);
    User::op_iterator OI = st->op_begin();
    *OI = (Value *)ld;
  }

  Function *prestfunc;
  if (st->getValueOperand()->getType()->isIntegerTy(32)) {
    prestfunc = funcMapping["__accmut__prepare_st_i32"];
  } else if (st->getValueOperand()->getType()->isIntegerTy(64)) {
    prestfunc = funcMapping["__accmut__prepare_st_i64"];
  } else {
    ERRMSG("ERR STORE TYPE ");
    VALERRMSG(st, "CUR_TYPE", st->getValueOperand()->getType());
    assert(false);
    exit(-1);
  }

  std::vector<Value *> params;
  params.push_back(rmigv);
  std::stringstream ss;
  ss << mut_from;
  ConstantInt *from_i32 = ConstantInt::get(TheModule->getContext(),
                                           APInt(32, StringRef(ss.str()), 10));
  params.push_back(from_i32);
  ss.str("");
  ss << mut_to;
  ConstantInt *to_i32 = ConstantInt::get(TheModule->getContext(),
                                         APInt(32, StringRef(ss.str()), 10));
  params.push_back(to_i32);

  Value *tobestored = dyn_cast<Value>(st->op_begin());
  params.push_back(tobestored);

  auto addr = st->op_begin() + 1; // the pointer of the storeinst
  if (LoadInst *ld = dyn_cast<LoadInst>(&*addr)) { // is a local var
    params.push_back(ld);
  } else if (AllocaInst *alloca = dyn_cast<AllocaInst>(&*addr)) {
    params.push_back(alloca);
  } else if (Constant *con = dyn_cast<Constant>(&*addr)) {
    params.push_back(con);
  } else if (GetElementPtrInst *gete = dyn_cast<GetElementPtrInst>(&*addr)) {
    params.push_back(gete);
  } else if (PHINode *phi = dyn_cast<PHINode>(&*addr)) {
    params.push_back(phi);
  } else {
    ERRMSG("NOT A POINTER ");
    // cur_it->dump();
    Value *v = dyn_cast<Value>(&*addr);
    llvm::errs() << *st << "\t" << *v << "\n";
    // v->dump();
    assert(false);
    exit(-1);
  }
  CallInst *pre = CallInst::Create(prestfunc, params, "", &*st);
  pre->setCallingConv(CallingConv::C);
  pre->setTailCall(false);
  AttributeList attrset;
  pre->setAttributes(attrset);

  ConstantInt *zero =
      ConstantInt::get(Type::getInt32Ty(TheModule->getContext()), 0);

  ICmpInst *hasstd =
      new ICmpInst(&*st, ICmpInst::ICMP_EQ, pre, zero, "hasstd.st");

  BasicBlock *cur_bb = st->getParent();

  BasicBlock *label_if_end = cur_bb->splitBasicBlock(st, "stdst.if.end");

  BasicBlock *label_if_else = BasicBlock::Create(
      TheModule->getContext(), "std.st", cur_bb->getParent(), label_if_end);

  cur_bb->back().eraseFromParent();

  BranchInst::Create(label_if_end, label_if_else, hasstd, cur_bb);

  // label_if_else
  Function *std_handle = funcMapping["__accmut__std_store"];

  CallInst *stdcall = CallInst::Create(std_handle, "", label_if_else);
  stdcall->setCallingConv(CallingConv::C);
  stdcall->setTailCall(false);
  AttributeList stdcallPAL;
  stdcall->setAttributes(stdcallPAL);
  BranchInst::Create(label_if_end, label_if_else);

  // label_if_end
  st->eraseFromParent();
}

void WAInstrumenter::instrumentArithInst(Instruction *cur_it, int mut_from,
                                         int mut_to, bool aboutGoodVariable,
                                         int good_from, int good_to,
                                         bool is_first, int left_id,
                                         int right_id, int ret_id) {
  Type *ori_ty = cur_it->getType();
  Function *f_process;
  if (ori_ty->isIntegerTy(32)) {
    if (aboutGoodVariable) {
      if (is_first) {
        f_process = funcMapping["__accmut__process_i32_arith_GoodVar_init"];
      } else {
        f_process = funcMapping["__accmut__process_i32_arith_GoodVar"];
      }
    } else {
      f_process = funcMapping["__accmut__process_i32_arith"];
    }
  } else if (ori_ty->isIntegerTy(64)) {
    if (aboutGoodVariable) {
      if (is_first) {
        f_process = funcMapping["__accmut__process_i64_arith_GoodVar_init"];
      } else {
        f_process = funcMapping["__accmut__process_i64_arith_GoodVar"];
      }
    } else {
      f_process = funcMapping["__accmut__process_i64_arith"];
    }
  } else {
    ERRMSG("ArithInst TYPE ERROR ");
    // cur_it->dump();
    llvm::errs() << *ori_ty << "\n";
    llvm::errs() << *cur_it << "\n";
    llvm::errs() << *(cur_it->getParent()) << "\n";
    // TODO:: handle i1, i8, i64 ... type
    assert(false);
    exit(-1);
  }

  std::vector<Value *> int_call_params;
  if (!aboutGoodVariable) {
    int_call_params.push_back(rmigv);
    std::stringstream ss;
    ss << mut_from;
    ConstantInt *from_i32 = ConstantInt::get(
        TheModule->getContext(), APInt(32, StringRef(ss.str()), 10));
    int_call_params.push_back(from_i32);
    ss.str("");
    ss << mut_to;
    ConstantInt *to_i32 = ConstantInt::get(TheModule->getContext(),
                                           APInt(32, StringRef(ss.str()), 10));
    int_call_params.push_back(to_i32);
  }
  int_call_params.push_back(cur_it->getOperand(0));
  int_call_params.push_back(cur_it->getOperand(1));

  if (aboutGoodVariable) {
    GlobalVariable *arggv = goodvarargGVs[mut_to];
    int_call_params.push_back(arggv);

    BasicBlock *cur_bb = cur_it->getParent();
    BasicBlock *original = cur_bb->splitBasicBlock(cur_it, "goodvar.orig");

    cur_bb->back().eraseFromParent();
    IRBuilder<> irBuilder(cur_bb);
    auto *statusptr = irBuilder.CreateInBoundsGEP(
        arggv, {getI32Constant(0), getI32Constant(0)});
    auto *load = irBuilder.CreateLoad(statusptr);
    auto *runorig = irBuilder.CreateICmpEQ(load, getI32Constant(2));
    auto b = original->begin();
    ++b;
    BasicBlock *end = original->splitBasicBlock(b, "goodvar.end");
    BasicBlock *goodvar = BasicBlock::Create(
        TheModule->getContext(), "goodvar.inst", cur_bb->getParent(), end);
    irBuilder.CreateCondBr(runorig, original, goodvar);

    irBuilder.SetInsertPoint(goodvar);
    auto *call = irBuilder.CreateCall(f_process, int_call_params);
    irBuilder.CreateBr(end);

    irBuilder.SetInsertPoint(&*end->begin());
    auto *phi = irBuilder.CreatePHI(call->getType(), 2);
    original->begin()->replaceUsesOutsideBlock(phi, original);
    phi->addIncoming(&*(original->begin()), original);
    phi->addIncoming(call, goodvar);

    // ReplaceInstWithInst(&*cur_it, call);

  } else {
    CallInst *call = CallInst::Create(f_process, int_call_params);
    ReplaceInstWithInst(&*cur_it, call);
  }
}

void WAInstrumenter::instrumentCmpInst(Instruction *cur_it, int mut_from,
                                       int mut_to, bool aboutGoodVariable,
                                       int good_from, int good_to,
                                       bool is_first, int left_id, int right_id,
                                       int ret_id) {
  Function *f_process;

  if (cur_it->getOperand(0)->getType()->isIntegerTy(32)) {

    if (aboutGoodVariable) {
      if (is_first) {
        f_process = funcMapping["__accmut__process_i32_cmp_GoodVar_init"];
      } else {
        f_process = funcMapping["__accmut__process_i32_cmp_GoodVar"];
      }
    } else {
      f_process = funcMapping["__accmut__process_i32_cmp"];
    }
  } else if (cur_it->getOperand(0)->getType()->isIntegerTy(64)) {

    if (aboutGoodVariable) {
      if (is_first) {
        f_process = funcMapping["__accmut__process_i64_cmp_GoodVar_init"];
      } else {
        f_process = funcMapping["__accmut__process_i64_cmp_GoodVar"];
      }
    } else {
      f_process = funcMapping["__accmut__process_i64_cmp"];
    }
  } else {
    ERRMSG("ICMP TYPE ERROR ");
    assert(false);
    exit(-1);
  }

  std::vector<Value *> int_call_params;
  if (!aboutGoodVariable) {
    int_call_params.push_back(rmigv);
    std::stringstream ss;
    ss << mut_from;
    ConstantInt *from_i32 = ConstantInt::get(
        TheModule->getContext(), APInt(32, StringRef(ss.str()), 10));
    int_call_params.push_back(from_i32);
    ss.str("");
    ss << mut_to;
    ConstantInt *to_i32 = ConstantInt::get(TheModule->getContext(),
                                           APInt(32, StringRef(ss.str()), 10));
    int_call_params.push_back(to_i32);
  }
  int_call_params.push_back(cur_it->getOperand(0));
  int_call_params.push_back(cur_it->getOperand(1));

  if (aboutGoodVariable) {
    GlobalVariable *arggv = goodvarargGVs[mut_to];
    int_call_params.push_back(arggv);

    BasicBlock *cur_bb = cur_it->getParent();
    BasicBlock *original = cur_bb->splitBasicBlock(cur_it, "goodvar.orig");

    cur_bb->back().eraseFromParent();
    IRBuilder<> irBuilder(cur_bb);
    auto *statusptr = irBuilder.CreateInBoundsGEP(
        arggv, {getI32Constant(0), getI32Constant(0)});
    auto *load = irBuilder.CreateLoad(statusptr);
    auto *runorig = irBuilder.CreateICmpEQ(load, getI32Constant(2));
    auto b = original->begin();
    ++b;
    BasicBlock *end = original->splitBasicBlock(b, "goodvar.end");
    BasicBlock *goodvar = BasicBlock::Create(
        TheModule->getContext(), "goodvar.inst", cur_bb->getParent(), end);
    irBuilder.CreateCondBr(runorig, original, goodvar);

    irBuilder.SetInsertPoint(goodvar);
    auto *call = irBuilder.CreateCall(f_process, int_call_params);
    auto *i32_conv = irBuilder.CreateTrunc(
        call, IntegerType::get(TheModule->getContext(), 1));
    irBuilder.CreateBr(end);

    irBuilder.SetInsertPoint(&*end->begin());
    auto *phi = irBuilder.CreatePHI(i32_conv->getType(), 2);
    original->begin()->replaceUsesOutsideBlock(phi, original);
    phi->addIncoming(&*(original->begin()), original);
    phi->addIncoming(i32_conv, goodvar);

    // ReplaceInstWithInst(&*cur_it, call);

  } else {
    CallInst *call = CallInst::Create(f_process, int_call_params, "", &*cur_it);
    CastInst *i32_conv =
        new TruncInst(call, IntegerType::get(TheModule->getContext(), 1), "");
    ReplaceInstWithInst(&*cur_it, i32_conv);
  }
}

void WAInstrumenter::instrumentInst(Instruction &I, bool aboutGoodVariable,
                                    int mut_from, int mut_to, int good_from,
                                    int good_to, bool is_first, int left_id,
                                    int right_id, int ret_id) {
  // TODO: test without it first

  Instruction *cur_it = &I;

#ifdef OUTPUT
  llvm::errs() << "CUR_INST: " << tmp.front()->index << "  (FROM:" << mut_from
               << "  TO:" << mut_to << "  AboutGoodVar: " << aboutGoodVariable
               << ")\t" << *cur_it << "\n";
#endif

  // CallInst Navigation
  if (auto call = dyn_cast<CallInst>(&*cur_it)) {
    instrumentCallInst(call, mut_from, mut_to);
  }
  // StoreInst Navigation
  else if (auto *st = dyn_cast<StoreInst>(&*cur_it)) {
    instrumentStoreInst(st, mut_from, mut_to);
  }
  // BinaryInst Navigation
  else {
    // FOR ARITH INST
    if (cur_it->getOpcode() >= Instruction::Add &&
        cur_it->getOpcode() <= Instruction::Xor) {
      instrumentArithInst(cur_it, mut_from, mut_to, aboutGoodVariable,
                          good_from, good_to, is_first, left_id, right_id,
                          ret_id);
    }
    // FOR ICMP INST
    // Cmp Navigation
    else if (cur_it->getOpcode() == Instruction::ICmp) {
      instrumentCmpInst(cur_it, mut_from, mut_to, aboutGoodVariable, good_from,
                        good_to, is_first, left_id, right_id, ret_id);
    }
  }
}

void WAInstrumenter::getInstMutsMap(vector<Mutation *> *v, Function &F) {
  instMutsMap.clear();

  int instIndex = 0;
  auto mutIter = v->begin();

  for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I, ++instIndex) {
    while ((*mutIter)->index == instIndex) {
      instMutsMap[&*I].push_back(*mutIter);
      ++mutIter;
      if (mutIter == v->end()) {
        return;
      }
    }
  }
}

void WAInstrumenter::getGoodVariables(BasicBlock &BB) {
  goodVariables.clear();

  int goodVariableCount = 0;

  for (auto I = BB.rbegin(), E = BB.rend(); I != E; ++I) {
    if (!isSupportedInstruction(&*I) || I->isUsedOutsideOfBlock(&BB)) {
      continue;
    }

    bool checkUser = true;

    for (User *U : I->users()) {
      if (Instruction *userInst = dyn_cast<Instruction>(U)) {
        if ( // goodVariables.count(userInst) == 0/
            !isSupportedBoolInstruction(userInst) &&
            !isSupportedInstruction(userInst)) {
          checkUser = false;
          break;
        }
      } else {
        assert(false);
      }
    }

    if (checkUser/* || (userCount == 1 && isSupportedInstruction(dyn_cast<Instruction>(*I->users().begin())))*/) {
      // assign an index to each good variable for instrument
      // TODO: comment this line to get DMA
      goodVariables[&*I] = ++goodVariableCount;
    }
  }

  // bool nb = false;

  /*
  if (goodVariableCount >= 17) {
    llvm::errs() << "TOO MANY\n";
    goodVariables.clear();
    assert(false); exit(-1);
    return;
  }
   */
  // errs() << "GV: " << goodVariableCount << "\n";

  /*
  for (auto it = BB.rbegin(), end = BB.rend(); it != end;) {
    //errs() << it.getNodePtr() << "---" << it->getPrevNode() << "\n";
    //errs() << "\n---F---" << *BB.getParent() << "---F---\n";

    // avoid iterator invalidation
    Instruction &I = *it;
    ++it;

    if (goodVariables.count(&I) == 1 || hasUsedPreviousGoodVariables(&I)) {
      if (isSupportedBoolInstruction(&I)) {
        if (nb == false)
          llvm::errs() << "BOOL\n";
        else
          llvm::errs() << "NOT LAST\n";
        return;
      } else {
        nb = false;
      }
    }
  }

  llvm::errs() << "NONE\n";
  goodVariables.clear();*/
}

void WAInstrumenter::filterRealGoodVariables() {
  std::vector<int> erased;
  for (auto it = goodVariables.begin(), end = goodVariables.end(); it != end;) {
    // avoid iterator invalidation
    Instruction *I = it->first;
    ++it;

    if (instMutsMap.count(I) == 0 && !hasUsedPreviousGoodVariables(I)) {
      erased.push_back(goodVariables[I]);
      goodVariables.erase(I);
    }
  }
  if (goodVariables.size() == 1)
    goodVariables.clear();

  if (!goodVariables.empty()) {
    std::sort(erased.begin(), erased.end());
    for (auto it = goodVariables.begin(), end = goodVariables.end();
         it != end;) {
      Instruction *I = it->first;
      int cur = it->second;
      ++it;
      goodVariables[I] =
          cur - (std::lower_bound(erased.begin(), erased.end(), cur) -
                 erased.begin());
    }
    std::set<int> s;
    for (auto it = goodVariables.begin(), end = goodVariables.end(); it != end;
         ++it) {
      s.insert(it->second);
    }
    if (s.size() != goodVariables.size()) {
      llvm::errs() << "Wrong size\n";
      abort();
    }
    int cur = 1;
    for (auto it = s.begin(); it != s.end(); ++it) {
      if (*it != cur++) {
        llvm::errs() << "Wrong elem\n";
        abort();
      }
    }
    int numOfGoodVar = s.size();
    int numOfMutants = 0;
    for (auto it = goodVariables.begin(); it != goodVariables.end(); ++it) {
      numOfMutants += instMutsMap[it->first].size();
    }
    this->numOfGoodVar = numOfGoodVar;
    this->numOfMutants = numOfMutants;
    this->maxNumOfGoodVar = std::max(numOfGoodVar, this->maxNumOfGoodVar);
    this->maxNumOfMutants = std::max(numOfMutants, this->maxNumOfMutants);
  }

#ifdef OUTPUT
  llvm::errs() << "GVC: " << goodVariables.size() << "\n";
#endif
}

void WAInstrumenter::pushGoodVarIdInfo(vector<Value *> &params, Instruction *I,
                                       int from, int to, int good_from,
                                       int good_to, int left_id, int right_id,
                                       int ret_id) {
  /*
  auto arg = dyn_cast<ConstantStruct>(ConstantStruct::get(
      dyn_cast<StructType>(typeMapping["GoodvarArg"]),
      {getI32Constant(from), getI32Constant(to), getI32Constant(from),
  getI32Constant(to), getI32Constant(ret_id), getI32Constant(left_id),
  getI32Constant(right_id), ConstantExpr::getPointerCast( mutSpecsGVs[to],
  PointerType::get(typeMapping["MutSpecs"], 0))}));*/

  GlobalVariable *arggv = goodvarargGVs[to];
  /*
  if (it != goodvarargGVs.end()) {
    arggv = it->second;
  } else {
    arggv = new GlobalVariable(
        *TheModule, typeMapping["GoodvarArg"], false,
        llvm::GlobalValue::LinkageTypes::InternalLinkage, arg,
        "goodvararg_" + std::to_string(from) + "_" + std::to_string(to),
        nullptr, llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, 0);
    goodvarargGVs[to] = arggv;
  }*/

  // errs() << "in push\n";
  // params.push_back(getI32Constant(good_from));
  // params.push_back(getI32Constant(good_to));
  params.push_back(arggv);
  // errs() << "out push\n";
}

int WAInstrumenter::getGoodVarId(Value *V) {
  Instruction *I = dyn_cast<Instruction>(V);
  auto it = goodVariables.find(I);
  int id = it != goodVariables.end() ? it->second : 0;
  return id;
}

ConstantInt *WAInstrumenter::getI32Constant(int value) {
  return ConstantInt::get(TheModule->getContext(),
                          APInt(32, std::to_string(value), 10));
}

ConstantInt *WAInstrumenter::getI64Constant(int64_t value) {
  return ConstantInt::get(TheModule->getContext(),
                          APInt(64, std::to_string(value), 10));
}

ConstantInt *WAInstrumenter::getI8Constant(int8_t value) {
  return ConstantInt::get(TheModule->getContext(),
                          APInt(8, std::to_string(value), 10));
}

ConstantInt *WAInstrumenter::getI1Constant(bool value) {
  return ConstantInt::get(TheModule->getContext(),
                          APInt(1, value ? "1" : "0", 10));
}

bool WAInstrumenter::isSupportedOpcode(Instruction *I) {
  unsigned opcode = I->getOpcode();
  return (Instruction::isBinaryOp(opcode) &&
          Instruction::getOpcodeName(opcode)[0] != 'F') ||
         opcode == Instruction::ICmp
      //|| opcode == Instruction::Select
      ;
}

bool WAInstrumenter::isSupportedOp(Instruction *I) {
  // Opcode and Operands both supported?
  return isSupportedOpcode(I) && isSupportedType(I->getOperand(1));
}

bool WAInstrumenter::isSupportedInstruction(Instruction *I) {
  return isSupportedType(I) && isSupportedOp(I);
}

bool WAInstrumenter::isSupportedBoolInstruction(Instruction *I) {
  return (I->getType()->isIntegerTy(1) && isSupportedOp(I))
      //|| I->getOpcode() == Instruction::Select
      ;
}

bool WAInstrumenter::isSupportedType(Value *V) {
  // TODO: only support i32 and i64 good variables now
  Type *T = V->getType();
  return T->isIntegerTy(32) || T->isIntegerTy(64);
}

bool WAInstrumenter::hasUsedPreviousGoodVariables(Instruction *I) {
  for (Use &U : I->operands()) {
    if (Instruction *usedInst = dyn_cast<Instruction>(U.get())) {
      if (goodVariables.count(usedInst) == 1) {
        // errs() << *usedInst << "\n";
        return true;
      }
    }
  }
  return false;
}

bool WAInstrumenter::canFuseGoodVarInst(BasicBlock *BB) {
  /*
  std::set<Instruction*> postset;
  std::set<Instruction*> goodset;
  std::set<Instruction*> preset;
  for (auto it = BB->begin(); it != BB->end(); ++it) {
    auto inst = dyn_cast<Instruction>(&*it);
    if (auto call = dyn_cast<CallInst>(inst)) {
      auto name = call->getCalledFunction()->getName();
      if (name.startswith("__accmut__process") && (name.endswith("GoodVar") ||
  name.endswith("GoodVar_init"))) { goodset.insert(call);
      }
    }
  }
  std::set<Instruction*> tovisit = goodset;
  while (!tovisit.empty()) {
    auto it = tovisit.begin();
    auto *next = *it;
    llvm::errs() << *next << "\n";
    tovisit.erase(it);


    for (auto *v : next->users()) {
      auto *vinst = dyn_cast<Instruction>(v);
      llvm::errs() << "   " << *vinst << "\n";
      if (vinst->getParent() != BB)
        continue;
      if (goodset.count(vinst) != 0) {
        if (goodset.count(next) == 0)
          return false;
      } else if (postset.count(vinst) != 0) {
      } else {
        postset.insert(vinst);
        tovisit.insert(vinst);
      }
    }
  }
  llvm::errs() << "--------\n";
  llvm::errs() << *BB << "\n";
  llvm::errs() << "goodset:\n";
  for (auto i : goodset) {
    llvm::errs() << *i << "\n";
  }
  llvm::errs() << "postset:\n";
  for (auto i : postset) {
    llvm::errs() << *i << "\n";
  }*/
  return true;
}

void WAInstrumenter::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<AAResultsWrapperPass>();
  AU.addRequired<MemoryDependenceWrapperPass>();
}

char WAInstrumenter::ID = 0;
static RegisterPass<WAInstrumenter>
    X("WinMut", "WinMut instrument");
