#include "llvm/Support/raw_ostream.h"

#include "llvm/Transforms/WinMut/MutUtil.h"

#include <sstream>

#include "llvm/Transforms/WinMut/DebugMacro.h"

using namespace llvm;
using namespace std;

bool MutUtil::allMutsGeted = false;
map<string, vector<Mutation *> *> MutUtil::AllMutsMap;
vector<Mutation *> MutUtil::AllMutsVec;

void MutUtil::dumpAllMuts() {
  map<string, vector<Mutation *> *>::iterator it;
  errs() << "--------- DUMP ALL MUTATIONS ----------\n";
  for (it = AllMutsMap.begin(); it != AllMutsMap.end(); it++) {
    errs() << "FUNCTION : " << it->first << '\n';
    vector<Mutation *> *v = it->second;
    vector<Mutation *>::iterator vit;
    for (vit = v->begin(); vit != v->end(); vit++) {
      Mutation *m = (*vit);
      errs() << "\tID: " << m->id << "\tTYPE: " << m->type
             << " \tFUNC: " << m->func << "\tINDEX: " << m->index
             << "\tS_OP: " << m->src_op << '\n';
    }
  }
  errs() << "--------------- END DUMP -----------------\n";
}

void MutUtil::getAllMutations(const string &path) {
  if (allMutsGeted) {
    return;
  }
  string buf;
  // string path = getenv("HOME");
  // path += "/tmp/accmut/mutations.txt";
#ifdef TMP_CTRL
  string pathmut = path + ".mut.tmpCtrl";
#else
  string pathmut = path + ".mut";
#endif

  std::ifstream fin(pathmut, ios::in);

  if (!fin.is_open()) {
    errs() << "FILE ERROR : mutations.txt @ " << path << "\n";
    allMutsGeted = true;
    return;
    // exit(-1);
  }

  int id = 1;
  while (fin >> buf) {
    Mutation *m = getMutation(buf, id);
    id++;
    if (AllMutsMap.count(m->func) == 0) {
      AllMutsMap[m->func] = new vector<Mutation *>();
    }
    AllMutsMap[m->func]->push_back(m);
    AllMutsVec.push_back(m);
  }
  fin.close();
  allMutsGeted = true;

#if 0
    dumpAllMuts();
#endif
}

Mutation *MutUtil::getMutation(string line, int id) {
#ifdef OUTPUT
  llvm::errs() << line << std::endl;
#endif
  stringstream ss;
  ss << line;
  string mtype, func;
  int index, sop;
  char colon;
  getline(ss, mtype, ':');
  getline(ss, func, ':');
  ss >> index;
  ss >> colon;
  ss >> sop;
  ss >> colon;

  Mutation *m;
  if (mtype == "AOR") {
    AORMut *aor = new AORMut();
    int top;
    ss >> top;
    aor->tar_op = top;
    m = dyn_cast<Mutation>(aor);
  } else if (mtype == "LOR") {
    LORMut *lor = new LORMut();
    int top;
    ss >> top;
    lor->tar_op = top;
    m = dyn_cast<Mutation>(lor);
  } else if (mtype == "COR") {
    std::abort();
  } else if (mtype == "ROR") {
    RORMut *ror = new RORMut();
    int sp, tp;
    ss >> sp;
    ss >> colon;
    ss >> tp;
    ror->src_pre = sp;
    ror->tar_pre = tp;
    m = dyn_cast<Mutation>(ror);
  } else if (mtype == "SOR") {
    std::abort();
  } else if (mtype == "STD") {
    STDMut *std = new STDMut();
    int type;
    ss >> type;
    int retval;
    std->func_ty = type;
    if (ss.peek() == ':') {
      ss >> colon;
      ss >> retval;
      std->retval = retval;
    }
    m = dyn_cast<Mutation>(std);
  } else if (mtype == "LVR") {
    LVRMut *lvr = new LVRMut();
    int oi, sc, tc;
    ss >> oi;
    ss >> colon;
    ss >> sc;
    ss >> colon;
    ss >> tc;
    lvr->oper_index = oi;
    lvr->src_const = sc;
    lvr->tar_const = tc;
    m = dyn_cast<Mutation>(lvr);
  } else if (mtype == "UOI") {
    UOIMut *uoi = new UOIMut();
    int oi, ut;
    ss >> oi;
    ss >> colon;
    ss >> ut;
    uoi->oper_index = oi;
    uoi->ury_tp = ut;
    m = dyn_cast<Mutation>(uoi);
  } else if (mtype == "ROV") {
    ROVMut *rov = new ROVMut();
    int op1, op2;
    ss >> op1;
    ss >> colon;
    ss >> op2;
    rov->op1 = op1;
    rov->op2 = op2;
    m = dyn_cast<Mutation>(rov);
  } else if (mtype == "ABV") {
    ABVMut *abv = new ABVMut();
    int opindex;
    ss >> opindex;
    // abv->oper_index;
    abv->oper_index = opindex;
    m = dyn_cast<Mutation>(abv);
  } else {
    errs() << "WRONG MUT TYPE !\n";
    exit(-1);
  }
  m->id = id;
  m->type = mtype;
  m->func = func;
  m->index = abs(index);
  m->src_op = sop;
  return m;
}

BasicBlock::iterator MutUtil::getLocation(Function &F, int instrumented_insts,
                                          int index) {
  int cur = 0;
  // errs()<<"GETLOCATION : "<<instrumented_insts<<"    "<<index<<"\n";
  for (Function::iterator FI = F.begin(); FI != F.end(); ++FI) {
    Function::iterator BB = FI;
    for (BasicBlock::iterator BI = BB->begin(); BI != BB->end(); ++BI, cur++) {
      // errs()<<"\t"<<" CUR: "<<cur<<*BI<<"\n";
      if (index + instrumented_insts == cur) {
        return BI;
      }
    }
  }
  return F.back().end();
}

int MutUtil::getOperandPtrDimension(Value *v) { // TODO: check for getElemPtr !
  if (LoadInst *ld = dyn_cast<LoadInst>(v)) {
    Value *ptr_of_ld = ld->getPointerOperand();
    if (dyn_cast<Constant>(ptr_of_ld) || dyn_cast<AllocaInst>(ptr_of_ld) ||
        dyn_cast<GetElementPtrInst>(ptr_of_ld)) {
      return 1;
    }

    LoadInst *lev_2_ld = dyn_cast<LoadInst>(ptr_of_ld);

    if (lev_2_ld) {
      Value *ptr_of_lev_2_ld = lev_2_ld->getPointerOperand();
      if (dyn_cast<Constant>(ptr_of_lev_2_ld) ||
          dyn_cast<AllocaInst>(ptr_of_lev_2_ld) ||
          dyn_cast<GetElementPtrInst>(ptr_of_ld)) {
        return 2;
      }
    }
  }
  // other type can not handle
  return -1;
}
