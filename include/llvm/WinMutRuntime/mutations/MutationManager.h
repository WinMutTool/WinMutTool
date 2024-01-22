#ifndef LLVM_MUTATIONMANAGER_H
#define LLVM_MUTATIONMANAGER_H

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <llvm/WinMutRuntime/init/init.h>
#include <llvm/WinMutRuntime/logging/Log.h>
#include <llvm/WinMutRuntime/mutations/MutationIDDecl.h>
#include <llvm/WinMutRuntime/mutations/MutationStorage.h>
#include <llvm/WinMutRuntime/signal/ExitCodes.h>
#include <llvm/WinMutRuntime/sysdep/macros.h>
#include <map>
#include <memory>
#include <stack>
#include <sys/mman.h>
#include <type_traits>
#include <vector>

// #define DEBUG_OUTPUT

#include <llvm/Transforms/WinMut/DebugMacro.h>

struct EqClass {
  int64_t value{};
  std::vector<int> mut_id;
  EqClass() = default;
  EqClass(EqClass &&other) noexcept {
    value = other.value;
    mut_id = std::move(other.mut_id);
  }
  EqClass &operator=(EqClass &&other) noexcept {
    value = other.value;
    mut_id = std::move(other.mut_id);
    return *this;
  }
};

struct MutSpec {
  int from;
  int to;
};

struct MutSpecs {
  int length;
  int totlength;
  MutSpec mutSpecs[1];
};

#define MAX_EQ_CLASS_NUM 200000

// manually copy enum definitions from llvm 6.0
enum Instruction {
  Add = 11,
  Sub = 13,
  Mul = 15,
  UDiv = 17,
  SDiv = 18,
  URem = 20,
  SRem = 21,
  Shl = 23,
  LShr = 24,
  AShr = 25,
  And = 26,
  Or = 27,
  Xor = 28,
};

enum CmpInst {
  ICMP_EQ = 32,  ///< equal
  ICMP_NE = 33,  ///< not equal
  ICMP_UGT = 34, ///< unsigned greater than
  ICMP_UGE = 35, ///< unsigned greater or equal
  ICMP_ULT = 36, ///< unsigned less than
  ICMP_ULE = 37, ///< unsigned less or equal
  ICMP_SGT = 38, ///< signed greater than
  ICMP_SGE = 39, ///< signed greater or equal
  ICMP_SLT = 40, ///< signed less than
  ICMP_SLE = 41, ///< signed less or equal
};

class GoodvarArgInBlock;

struct GoodvarArg {
  int status;
  int from;
  int to;
  int from_local;
  int to_local;
  int good_from;
  int good_to;
  int ret_id;
  int left_id;
  int right_id;
  int op;
  MutSpecs *specs;
  MutSpecs *dep;
  MutSpecs *notdep;
  GoodvarArgInBlock *inblock;
  RegMutInfo *rmi;
};

struct GoodvarArgInBlock {
  int len;
  GoodvarArg *goodvarArgs[1];
};

typedef struct PrepareCallParam {
  int type;
  uint64_t address;
} PrepareCallParam;

struct BlockRegMutBound {
  int left;
  int right;
};

enum class CalcType { I32Arith, I64Arith, I32Bool, I64Bool };

template <typename OpType, typename RetType, CalcType calcType> struct CalcFunc;

class MutationManager {
public:

// ------------ dump all mutaiont for mut tool to calculate mutation score -------------
#ifdef DUMP_ALL_MUTATION
  std::vector<Mutation> get_all_mutation(){
    return this->all_mutation;
  }
#endif
// ------------ dump all mutaiont for mut tool to calculate mutation score -------------


private:
  template <typename OpType, typename RetType, CalcType calcType>
  friend struct CalcFunc;

  std::vector<Mutation> all_mutation;
  std::vector<int> default_active_set;
  enum GoodvarStatus : unsigned char { NORMAL, BADVARLIKE, SKIP, EXECUTE };
  std::vector<int> recent_set;
  std::vector<int> forked_active_set;
  std::vector<int64_t> temp_result;
  EqClass eq_class[MAX_EQ_CLASS_NUM];
  int eq_num = 0;
  inline void filter_variant(int from, int to);
  inline void filter_variant_goodvar(int from, int to);
  void dump_eq_class();
  void divide_eqclass_cmp(int all_or, int all_and);
  void divide_eqclass_cl_st();
  void divide_eqclass();
  void divide_eqclass(int64_t zeroval);
  void divide_eqclass_goodvar(int from, int to, int64_t zeroval);
  template <class T, class UT>
  static T cal_T_arith(int32_t op, T a, T b, T undefValue);
  static int32_t cal_i32_arith(int32_t op, int32_t a, int32_t b);
  static int64_t cal_i64_arith(int32_t op, int64_t a, int64_t b);
  template <class T, class UT> static T cal_T_bool(int32_t pred, T a, T b);
  static int32_t cal_i32_bool(int32_t pred, int32_t a, int32_t b);
  static int32_t cal_i64_bool(int32_t pred, int64_t a, int64_t b);
  int apply_call_mut(Mutation *m, PrepareCallParam params[]);
  int apply_store_mut(Mutation *m, int64_t tobestore, uint64_t addr,
                      int is_i32);

  template <typename T> class MemVecNonHolding {
    T *cur;
    size_t _size;

  public:
    explicit MemVecNonHolding(void *ptr)
        : cur(static_cast<T *>(ptr)), _size(0) {}
    void push_back(const T &p) { cur[_size++] = p; }
    void push_back(T &&p) { cur[_size++] = p; }
    T *begin() { return cur; }
    T *end() { return cur + _size; }
    const T *begin() const { return cur; }
    const T *end() const { return cur + _size; }
    void clear() { _size = 0; }
    size_t size() const { return _size; }
  };

  class MmapMemoryPool {
    std::vector<std::pair<void *, size_t>> pointers;
    void *cur{};
    size_t offset{};
    size_t size;
    void getNew() {
      if (offset == 0 && !pointers.empty()) {
        // not using
        if (pointers.back().second != size) {
          munmap(pointers.back().first, pointers.back().second);
          pointers.pop_back();
        } else {
          return;
        }
      }
      cur = mmap(nullptr, size, PROT_WRITE | PROT_READ,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (pointers.empty()) {
        char *b = (char *)cur;
        char *e = b + size;
        while (b < e) {
          *b = 1;
          b += PAGE_SIZE / 2;
        }
      }
      offset = 0;
      pointers.emplace_back(cur, size);
    }

  public:
    void dump() {
      char buf[1024];
      snprintf(buf, 1024, "pointers: %ld, offset: %ld, size: %ld\n",
               pointers.size(), offset, size);
      if (__accmut_libc_write(STDERR_FILENO, buf, strlen(buf)) < 0)
        exit(-1);
    }
    MmapMemoryPool() : MmapMemoryPool(1024) {}
    explicit MmapMemoryPool(size_t size) : size(size) { getNew(); }
    ~MmapMemoryPool() {
      for (const auto &p : pointers) {
        munmap(p.first, p.second);
      }
    }
    template <typename T> T *getRawPtr(size_t rsize) {
      size_t realsize = rsize * sizeof(T);
      if (unlikely(realsize > size)) {
        size = realsize * 64;
        getNew();
      } else {
        if (unlikely(realsize + offset > size)) {
          getNew();
        }
      }
      T *ret = reinterpret_cast<T *>(static_cast<int8_t *>(cur) + offset);
      offset += realsize;
      return ret;
    }
    void resize(size_t rsize) {
      if (this->size < rsize) {
        this->size = rsize;
        getNew();
      }
    }
  };

  MmapMemoryPool pool;
  size_t maxNumOfGoodVar = 0;
  size_t maxNumOfMutants = 0;

  using GoodVarEntryType = std::pair<int, int64_t>;
  using GoodVarTableType = MemVecNonHolding<GoodVarEntryType>;

  class GoodVarTable {
  public:
    friend class MutationManager;

  private:
    GoodVarTableType *goodVarTableList;

  public:
    GoodVarTable(int maxNumOfGoodVar, int maxNumOfMutants,
                 GoodVarTableType *tbllst, GoodVarEntryType *ptr) {
      // assume available size of table is
      // sizeof(GoodVarTableType) * maxNumOfGoodVar
      // assume available size of ptr is
      // sizeof(GoodVarEntryType) * maxNumOfMutants * maxNumOfGoodVar
      goodVarTableList = tbllst;
      for (int i = 0; i < maxNumOfGoodVar; ++i) {
        goodVarTableList[i] =
            MemVecNonHolding<GoodVarEntryType>(ptr + maxNumOfMutants * i);
      }
    }
    GoodVarTableType &operator[](size_t id) { return goodVarTableList[id]; }
    const GoodVarTableType &operator[](size_t id) const {
      return goodVarTableList[id];
    }
  };

  class GoodVarStack {
  private:
    std::vector<GoodVarTable> base;
    size_t size{};

  public:
    GoodVarTable &top() { return base[size - 1]; }
    ~GoodVarStack() {
      char buf[1024];
      snprintf(buf, 1024, "%ld", size);
      __accmut_libc_write(STDERR_FILENO, buf, strlen(buf));
    }
    void push(size_t numOfGoodVar, size_t numOfMutants,
              MmapMemoryPool &mmapPool) {
      if (size == base.size()) {
        auto tbllst = mmapPool.getRawPtr<GoodVarTableType>(numOfGoodVar);
        auto ptr =
            mmapPool.getRawPtr<GoodVarEntryType>(numOfMutants * numOfGoodVar);
        base.emplace_back(numOfGoodVar, numOfMutants, tbllst, ptr);
        ++size;
      } else {
        ++size;
        for (size_t i = 0; i < numOfGoodVar; ++i)
          top()[i].clear();
      }
    }
    void pop() { size--; }
  };

  GoodVarStack goodvar_table_stack;
  inline GoodVarTable &get_goodvar_table() { return goodvar_table_stack.top(); }

public:
  inline void goodvar_mutant_specs_stack_init() {
    goodvar_table_push(maxNumOfGoodVar, maxNumOfMutants);
    // pool.dump();
  }

private:
  using goodvar_mutant_specs_type = MutSpecs *;
  void dump_spec(MutSpecs *);
  void filter_mutants(const goodvar_mutant_specs_type &depSpec, int classid);
  int64_t fork_eqclass(const char *moduleName,
                       const goodvar_mutant_specs_type &depSpec, int offset);

  template <typename OpType>
  int process_T_cmp(RegMutInfo *rmi, int from, int to, OpType left,
                    OpType right, int (*calc)(int, OpType, OpType));

  template <typename OpType, typename RetType>
  inline RetType process_T_arith(RegMutInfo *rmi, int from, int to, OpType left,
                                 OpType right, int op,
                                 RetType (*calc)(int, OpType, OpType));

  template <typename OpType, class RetType, bool needInit, CalcType calcType>
  inline RetType process_T_arith_goodvar(OpType left, OpType right,
                                         GoodvarArg *arg);

  template <typename OpType, typename RetType>
  inline OpType process_T_calc_mutation(int op, OpType left, OpType right,
                                        RetType (*f)(int, OpType, OpType),
                                        int mut_id);
  template <typename OpType, typename RetType>
  inline void process_T_calc_goodvar(int op, OpType left, OpType right,
                                     RetType (*f)(int, OpType, OpType),
                                     int left_id, int right_id);
  template <typename OpType, typename RetType>
  inline void process_T_calc_goodvar_for_badvar_and_forked(
      int op, OpType left, OpType right, RetType (*f)(int, OpType, OpType),
      int left_id, int right_id, int from);
  bool is_active(int id);

  MutationManager();

public:
  static inline MutationManager *getInstance() {
    static MutationManager *singleton = nullptr;
    if (likely(singleton != nullptr)) {
      return singleton;
    }
    singleton = new MutationManager();
    return singleton;
  }
  inline void setMaxNum(size_t numOfGoodVar, size_t numOfMutants) {
    bool updated = false;
    if (numOfGoodVar > maxNumOfGoodVar) {
      maxNumOfGoodVar = numOfGoodVar + 1;
      updated = true;
    }
    if (numOfMutants > maxNumOfMutants) {
      maxNumOfMutants = numOfMutants + 1;
      updated = true;
    }
    if (updated) {
      pool.resize(static_cast<size_t>(maxNumOfGoodVar * maxNumOfMutants * 100));
    }
  }

  void register_muts(RegMutInfo *rmi, BlockRegMutBound **bound, int boundnum,
                     MutSpecs **specs, int specsnum, GoodvarArg **args,
                     int argsnum);
  int process_i32_arith(RegMutInfo *rmi, int from, int to, int left, int right);
  int64_t process_i64_arith(RegMutInfo *rmi, int from, int to, int64_t left,
                            int64_t right);
  int process_i32_cmp(RegMutInfo *rmi, int from, int to, int left, int right);
  int process_i64_cmp(RegMutInfo *rmi, int from, int to, int64_t left,
                      int64_t right);
  int prepare_call(RegMutInfo *rmi, int from, int to, int opnum, va_list ap);
  int stdcall_i32();
  int64_t stdcall_i64();
  void stdcall_void();
  void std_store();
  int prepare_st_i32(RegMutInfo *rmi, int from, int to, int tobestore,
                     int *addr);
  int prepare_st_i64(RegMutInfo *rmi, int from, int to, int64_t tobestore,
                     int64_t *addr);

  void goodvar_basic_block_init(RegMutInfo *rmi, int from, int to);
  void goodvar_table_push(size_t numOfGoodVar, size_t numOfMutants);
  void goodvar_table_push_max();
  void goodvar_table_pop();
  inline int process_i32_arith_goodvar(int left, int right, GoodvarArg *arg);
  inline int64_t process_i64_arith_goodvar(int64_t left, int64_t right,
                                           GoodvarArg *arg);
  inline int32_t process_i32_cmp_goodvar(int left, int right, GoodvarArg *arg);
  inline int32_t process_i64_cmp_goodvar(int64_t left, int64_t right,
                                         GoodvarArg *arg);
  inline int process_i32_arith_goodvar_init(int left, int right,
                                            GoodvarArg *arg);
  inline int64_t process_i64_arith_goodvar_init(int64_t left, int64_t right,
                                                GoodvarArg *arg);
  inline int32_t process_i32_cmp_goodvar_init(int left, int right,
                                              GoodvarArg *arg);
  inline int32_t process_i64_cmp_goodvar_init(int64_t left, int64_t right,
                                              GoodvarArg *arg);
};

void MutationManager::filter_variant(int from, int to) {
  recent_set.clear();
  if (MUTATION_ID == 0) {
    recent_set.push_back(0);
    for (int i = from; i <= to; ++i) {
      if (default_active_set[i] == 1) {
        recent_set.push_back(i);
      }
    }
  } else {
    for (int m : forked_active_set) {
      if (m >= from && m <= to) {
        recent_set.push_back(m);
      }
    }
    if (recent_set.empty()) {
      exit(MALLOC_ERR);
    }
  }
}

void MutationManager::filter_variant_goodvar(int from, int to) {
  // recent_set.clear();
  // don't clear for the order
  if (MUTATION_ID == 0) {
    for (int i = from; i <= to; ++i) {
      if (default_active_set[i] == 1) {
        recent_set.push_back(i);
      }
    }
  } else {
    for (int m : forked_active_set) {
      if (m >= from && m <= to)
        recent_set.push_back(m);
      /*
      if (ret_id == 0) {
        if (m >= from) {
          recent_set.push_back(m);
        }
      } else {
        if (m >= from && m <= to) {
          recent_set.push_back(m);
        }
      }
       */
    }
  }
}

template <> struct CalcFunc<int32_t, int32_t, CalcType::I32Arith> {
  using FuncPointerType = int32_t (*)(int, int32_t, int32_t);
  constexpr static FuncPointerType funcPointer =
      &(MutationManager::cal_i32_arith);
};

template <> struct CalcFunc<int64_t, int64_t, CalcType::I64Arith> {
  using FuncPointerType = int64_t (*)(int, int64_t, int64_t);
  constexpr static FuncPointerType funcPointer =
      &(MutationManager::cal_i64_arith);
};

template <> struct CalcFunc<int32_t, int32_t, CalcType::I32Bool> {
  using FuncPointerType = int32_t (*)(int, int32_t, int32_t);
  constexpr static FuncPointerType funcPointer =
      &(MutationManager::cal_i32_bool);
};

template <> struct CalcFunc<int64_t, int32_t, CalcType::I64Bool> {
  using FuncPointerType = int32_t (*)(int, int64_t, int64_t);
  constexpr static FuncPointerType funcPointer =
      &(MutationManager::cal_i64_bool);
};

template <class T, class UT>
T MutationManager::cal_T_arith(int32_t op, T a, T b, T undefValue) {
  // TODO:: add float point
  switch (op) {
  case Instruction::Add:
    return a + b;

  case Instruction::Sub:
    return a - b;

  case Instruction::Mul:
    return a * b;

  case Instruction::UDiv:
    return b == 0 ? undefValue : (T)((UT)a / (UT)b);

  case Instruction::SDiv:
    if (sizeof(T) == 4) {
      if (unlikely(b == -1 && a == INT32_MIN)) {
        return undefValue;
      }
    } else {
      if (unlikely(b == -1 && a == INT64_MIN)) {
        return undefValue;
      }
    }
    return b == 0 ? undefValue : a / b;

  case Instruction::URem:
    return b == 0 ? undefValue : (T)((UT)a % (UT)b);

  case Instruction::SRem:
    if (sizeof(T) == 4) {
      if (unlikely(b == -1 && a == INT32_MIN)) {
        return undefValue;
      }
    } else {
      if (unlikely(b == -1 && a == INT64_MIN)) {
        return undefValue;
      }
    }
    return b == 0 ? undefValue : a % b;

  case Instruction::Shl:
    return a << b;

  case Instruction::LShr:
    return (T)((UT)a >> b);

  case Instruction::AShr:
    return a >> b;

  case Instruction::And:
    return a & b;

  case Instruction::Or:
    return a | b;

  case Instruction::Xor:
    return a ^ b;

  default:
    LOG("OpCode: %d\n", op);
    if (MUTATION_ID == 0) {
      raise(SIGSTOP);
    }
    exit(MUT_TP_ERR);
  }
}
template <class T, class UT>
T MutationManager::cal_T_bool(int32_t pred, T a, T b) {
  switch (pred) {
  case CmpInst::ICMP_EQ:
    return a == b;

  case CmpInst::ICMP_NE:
    return a != b;

  case CmpInst::ICMP_UGT:
    return (UT)a > (UT)b;

  case CmpInst::ICMP_UGE:
    return (UT)a >= (UT)b;

  case CmpInst::ICMP_ULT:
    return (UT)a < (UT)b;

  case CmpInst::ICMP_ULE:
    return (UT)a <= (UT)b;

  case CmpInst::ICMP_SGT:
    return a > b;

  case CmpInst::ICMP_SGE:
    return a >= b;

  case CmpInst::ICMP_SLT:
    return a < b;

  case CmpInst::ICMP_SLE:
    return a <= b;

  default:
    LOG("PredCode: %d\n", pred);
    exit(MUT_TP_ERR);
  }
}

template <typename OpType, typename RetType>
RetType MutationManager::process_T_arith(RegMutInfo *rmi, int from, int to,
                                         OpType left, OpType right, int op,
                                         RetType (*calc)(int, OpType, OpType)) {
  using namespace std::chrono;
  if (unlikely(system_disabled() || !system_initialized()))
    return calc(op, left, right);
#ifdef CLOCK
  auto start = high_resolution_clock::now();
#endif
  from += rmi->offset;
  to += rmi->offset;

#if 0
  if (MUTATION_ID != 0) {
    char buf[1024];
    snprintf(buf, 1024, "%d %d %d\n", MUTATION_ID, from, to);
    write(STDERR_FILENO, buf, strlen(buf));
    sleep(1);
  }
#endif

  if (likely(MUTATION_ID != 0)) {
    if (likely(MUTATION_ID > to || MUTATION_ID < from)) {
      return calc(op, left, right);
    }
  }
  filter_variant(from, to);
  temp_result.clear();
  for (int &i : recent_set) {
    auto mut_res = process_T_calc_mutation(op, left, right, calc, i);
    temp_result.push_back(mut_res);
  }
  if (recent_set.size() == 1) {
    if (MUTATION_ID < from || MUTATION_ID > to) {
      return calc(op, left, right);
    }
    return temp_result[0];
  }

  divide_eqclass();

  /*
    MutSpec spec;
    spec.from = from;
    spec.to = to;
    return fork_eqclass(goodvar_mutant_specs_type{spec}, rmi->offset);
    */
  MutSpecs specs{};
  specs.length = 1;
  specs.totlength = 1;
  specs.mutSpecs[0].from = from;
  specs.mutSpecs[0].to = to;

#ifdef CLOCK
  auto end = high_resolution_clock::now();
  char buf[1024];
  snprintf(buf, 1024, "3: %ld %ld\n",
           duration_cast<nanoseconds>(end - start).count(), recent_set.size());
  write(STDERR_FILENO, buf, strlen(buf));
#endif

  auto ret = fork_eqclass(rmi->moduleName,
                          &specs
                          // goodvar_mutant_specs_type{spec}
                          ,
                          rmi->offset);
  /*
  if (ret != ori) {
    char buf[1024];
    snprintf(buf, 1024, "%lx %lx %lx\n", (int64_t)ret, (int64_t)ori,
  eq_class[0].value); write(STDERR_FILENO, "FUCK\n", 5); write(STDERR_FILENO,
  buf, strlen(buf)); for (int i = 0; i < recent_set.size(); ++i) { snprintf(buf,
  1024, "%d:%lx ", recent_set[i], temp_result[i]); write(STDERR_FILENO, buf,
  strlen(buf));
    }
    write(STDERR_FILENO, "\n", 1);
  }*/
  return ret;
}

template <typename OpType, class RetType, bool needInit, CalcType calcType>
RetType MutationManager::process_T_arith_goodvar(OpType left, OpType right,
                                                 GoodvarArg *arg) {
  typedef RetType CalcFuncType(int, OpType, OpType);

  CalcFuncType *f = CalcFunc<OpType, RetType, calcType>::funcPointer;

  if (unlikely(system_disabled() || !system_initialized()))
    return f(arg->op, left, right);
  int from = arg->from;
  int to = arg->to;
  int op = arg->op;
#ifdef DEBUG_OUTPUT
  char buf[1024];
#endif

  switch (arg->status) {
  case EXECUTE: {
#ifdef DEBUG_OUTPUT
    int fd = open("eq_class", O_CREAT | O_APPEND | O_WRONLY, 0644);
    snprintf(buf, 1024, "EXECUTE: %d %d\n", MUTATION_ID, from);
    write(fd, buf, strlen(buf));
    close(fd);
#endif
    return process_T_calc_mutation(op, left, right, f, MUTATION_ID);
  }

  case BADVARLIKE: {
#ifdef DEBUG_OUTPUT
    int fd = open("eq_class", O_CREAT | O_APPEND | O_WRONLY, 0644);
    snprintf(buf, 1024, "BVL: %d %d\n", MUTATION_ID, from);
    write(fd, buf, strlen(buf));
    close(fd);
#endif
    return process_T_arith(arg->rmi, arg->from_local, arg->to_local, left,
                           right, op, f);
  }
  /*
  case SKIP: {
      write(STDERR_FILENO, "= =\n", 4);
      raise(SIGSTOP);
#ifdef DEBUG_OUTPUT
    int fd = open("eq_class", O_CREAT | O_APPEND | O_WRONLY, 0644);
    snprintf(buf, 1024, "SKIP: %d %d\n", MUTATION_ID, from);
    write(fd, buf, strlen(buf));
    close(fd);
#endif
    return f(op, left, right);
  }
   */
  default: {
#ifdef DEBUG_OUTPUT
    int fd = open("eq_class", O_CREAT | O_APPEND | O_WRONLY, 0644);
    snprintf(buf, 1024, "RUN: %d %d\n", MUTATION_ID, from);
    write(fd, buf, strlen(buf));
    close(fd);
#endif
    if (MUTATION_ID != 0) {
      if (unlikely(forked_active_set.size() == 0))
        return f(op, left, right);
      if (forked_active_set.front() >= from && forked_active_set.back() <= to) {
        for (int i = 0; i < arg->inblock->len; ++i) {
          arg->inblock->goodvarArgs[i]->status = SKIP;
        }
        arg->status = BADVARLIKE;
#ifdef DEBUG_OUTPUT
        int fd = open("eq_class", O_CREAT | O_APPEND | O_WRONLY, 0644);
        write(fd, "BBVVLL\n", 7);
        close(fd);
#endif
        return process_T_arith(arg->rmi, arg->from_local, arg->to_local, left,
                               right, op, f);
      }
    }
    // #define CLOCK
    using namespace std::chrono;
#ifdef CLOCK
    auto start = high_resolution_clock::now();
#endif
    int goodvar_from = arg->good_from;
    int goodvar_to = arg->good_to;

    if (unlikely(MUTATION_ID != 0) &&
        unlikely(MUTATION_ID > goodvar_to || MUTATION_ID < goodvar_from)) {
      return f(op, left, right);
    }

    int ret_id = arg->ret_id;

    recent_set.clear();
    temp_result.clear();
// #define BUGGY
#ifndef BUGGY
    if (MUTATION_ID != 0 && ret_id == 0) {
      process_T_calc_goodvar_for_badvar_and_forked(
          op, left, right, f, arg->left_id, arg->right_id, from);
    } else {
#endif
      process_T_calc_goodvar(op, left, right, f, arg->left_id, arg->right_id);
#ifndef BUGGY
    }
#endif
#undef BUGGY
    filter_variant_goodvar(from, to);

    /*
    char buf[1024];
    snprintf(buf, 1024, "%d %ld %d %d %d\n", MUTATION_ID, recent_set.size(),
             arg->left_id, arg->right_id, arg->ret_id);
    write(STDERR_FILENO, buf, strlen(buf));
     */

#ifdef CLOCK
    if (MUTATION_ID != 0) {
      auto end = high_resolution_clock::now();
      char buf[1024];
      snprintf(buf, 1024, "??: %ld %ld\n",
               duration_cast<nanoseconds>(end - start).count(),
               recent_set.size());
      write(STDERR_FILENO, buf, strlen(buf));
      start = high_resolution_clock::now();
    }
#endif
    for (auto i = temp_result.size(); i != recent_set.size(); ++i) {
      int mut_id = recent_set[i];
      temp_result.push_back(
          process_T_calc_mutation(op, left, right, f, mut_id));
    }

    if (ret_id == 0) {
      if (recent_set.empty()) {
        arg->status = SKIP;
        return f(op, left, right);
      }

#ifdef CLOCK
      if (MUTATION_ID != 0) {
        auto end = high_resolution_clock::now();
        char buf[1024];
        snprintf(buf, 1024, "11: %ld %ld\n",
                 duration_cast<nanoseconds>(end - start).count(),
                 recent_set.size());
        write(STDERR_FILENO, buf, strlen(buf));
        start = high_resolution_clock::now();
      }
#endif
#ifdef DEBUG_OUTPUT
      char buf[1024];
      snprintf(buf, 1024, "id: %d %d %d\n", arg->left_id, arg->right_id,
               ret_id);
      int fd = open("eq_class", O_APPEND | O_CREAT | O_WRONLY, 0644);
      write(fd, buf, strlen(buf));
      close(fd);
      dump_spec(arg->specs);
#endif
      if (MUTATION_ID == 0)
        divide_eqclass(f(op, left, right));
      else {
        divide_eqclass_goodvar(from, to, f(op, left, right));
      }
      if (eq_num == 1) {
        return eq_class[0].value;
      }
#ifdef DEBUG_OUTPUT
      dump_eq_class();
#endif

#ifdef CLOCK
      if (MUTATION_ID != 0) {
        auto end = high_resolution_clock::now();
        char buf[1024];
        snprintf(buf, 1024, "1: %ld %ld\n",
                 duration_cast<nanoseconds>(end - start).count(),
                 recent_set.size());
        write(STDERR_FILENO, buf, strlen(buf));
      }
#endif
      auto ret = (RetType)fork_eqclass(arg->rmi->moduleName,
      // goodvar_get_mutant_spec()
#ifdef BUGGY
                                       arg->specs,
#else
                                       arg->dep,
#endif
                                       arg->rmi->offset);
      if (MUTATION_ID != 0) {
        if (forked_active_set.size() == 1) {
          for (int i = 0; i < arg->inblock->len; ++i) {
            auto x = arg->inblock->goodvarArgs[i];
            if (x->from <= MUTATION_ID && x->to >= MUTATION_ID) {
              arg->inblock->goodvarArgs[i]->status = EXECUTE;
            } else {
              arg->inblock->goodvarArgs[i]->status = SKIP;
            }
          }
        }
      }
      return ret;
    } else {

#ifdef CLOCK
      if (MUTATION_ID != 0) {
        auto end = high_resolution_clock::now();
        char buf[1024];
        snprintf(buf, 1024, "22: %d %d %d %d %d %ld %ld\n", MUTATION_ID, from,
                 to, goodvar_from, goodvar_to,
                 duration_cast<nanoseconds>(end - start).count(),
                 recent_set.size());
        write(STDERR_FILENO, buf, strlen(buf));
        start = high_resolution_clock::now();
      }
#endif
      GoodVarTableType &ret_goodvar = get_goodvar_table()[ret_id];
      ret_goodvar.clear();

      if (recent_set.empty()) {
        arg->status = SKIP;
        return f(op, left, right);
      }

      for (size_t i = 0; i != recent_set.size(); ++i) {
        ret_goodvar.push_back(std::make_pair(recent_set[i], temp_result[i]));
      }
#ifdef DEBUG_OUTPUT
      char buf[1024];
      snprintf(buf, 1024, "id: %d %d %d\n", arg->left_id, arg->right_id,
               ret_id);
      int fd = open("eq_class", O_APPEND | O_CREAT | O_WRONLY, 0644);
      write(fd, buf, strlen(buf));
      write(fd, "goodvar table:\n", strlen("goodvar table:\n"));
      for (auto &x : ret_goodvar) {
        snprintf(buf, 1024, "%d:%ld ", x.first, x.second);
        write(fd, buf, strlen(buf));
      }
      write(fd, "\n", 1);
      close(fd);
#endif

#ifdef CLOCK
      if (MUTATION_ID != 0) {
        auto end = high_resolution_clock::now();
        char buf[1024];
        snprintf(buf, 1024, "2: %ld %ld\n",
                 duration_cast<nanoseconds>(end - start).count(),
                 ret_goodvar.size());
        write(STDERR_FILENO, buf, strlen(buf));
      }
#endif

      return f(op, left, right);
    }
  }
  }
}

template <typename OpType, typename RetType>
OpType
MutationManager::process_T_calc_mutation(int op, OpType left, OpType right,
                                         RetType (*f)(int, OpType, OpType),
                                         int mut_id) {
#ifdef DEBUG_OUTPUT
  /*
  int fd = open("eq_class", O_APPEND | O_WRONLY | O_CREAT, 0644);
  char buf[1024];
  snprintf(buf, 1024, "%d %ld %ld %d\n", op, (int64_t)left, (int64_t)right,
           mut_id);
  write(fd, buf, strlen(buf));
  close(fd);
   */
#endif
  if (mut_id == 0) {
    return f(op, left, right);
  }
  Mutation m = all_mutation[mut_id];
  switch (m.type) {
  case LVR:
    if (m.op_0 == 0) {
      return f(op, (OpType)m.op_2, right);
    } else if (m.op_0 == 1) {
      return f(op, left, (OpType)m.op_2);
    } else {
      assert(false);
    }
  case UOI: {
    if (m.op_1 == 0) {
      OpType u_left;
      if (m.op_2 == 0) {
        u_left = left + 1;
      } else if (m.op_2 == 1) {
        u_left = left - 1;
      } else if (m.op_2 == 2) {
        u_left = -left;
      } else {
        assert(false);
      }
      return f(op, u_left, right);
    } else if (m.op_1 == 1) {
      long u_right;
      if (m.op_2 == 0) {
        u_right = right + 1;
      } else if (m.op_2 == 1) {
        u_right = right - 1;
      } else if (m.op_2 == 2) {
        u_right = -right;
      } else {
        assert(false);
      }
      return f(op, left, u_right);
    } else {
      assert(false);
    }
  }

  case ROV:
    return f(op, right, left);

  case ABV:
    if (m.op_0 == 0) {
      return f(op, abs(left), right);
    } else if (m.op_0 == 1) {
      return f(op, left, abs(right));
    } else {
      assert(false);
    }

  case AOR:
  case LOR:
  case COR:
  case SOR:
    return f(m.op_0, left, right);

  case ROR:
    return f(static_cast<OpType>(m.op_2), left, right);

  default:
    assert(false);
  }
}
#ifdef BUGGY
template <typename OpType, typename RetType>
void MutationManager::process_T_calc_goodvar(int op, OpType left, OpType right,
                                             RetType (*f)(int, OpType, OpType),
                                             int left_id, int right_id) {
  GoodVarTableType &left_goodvar = get_goodvar_table()[left_id];
  GoodVarTableType &right_goodvar = get_goodvar_table()[right_id];

  auto left_it = left_goodvar.begin();
  auto right_it = right_goodvar.begin();

  for (; left_it != left_goodvar.end() && right_it != right_goodvar.end();) {
    if (left_it->first < right_it->first) {
      if (is_active(left_it->first)) {
        recent_set.push_back(left_it->first);
        temp_result.push_back(f(op, left_it->second, right));
      }
      ++left_it;
    } else if (left_it->first > right_it->first) {
      if (is_active(right_it->first)) {
        recent_set.push_back(right_it->first);
        temp_result.push_back(f(op, left, right_it->second));
      }
      ++right_it;
    } else {
      if (is_active(left_it->first)) {
        recent_set.push_back(left_it->first);
        temp_result.push_back(f(op, left_it->second, right_it->second));
      }
      ++left_it;
      ++right_it;
    }
  }
  while (left_it != left_goodvar.end()) {
    if (is_active(left_it->first)) {
      recent_set.push_back(left_it->first);
      temp_result.push_back(f(op, left_it->second, right));
    }
    ++left_it;
  }
  while (right_it != right_goodvar.end()) {
    if (is_active(right_it->first)) {
      recent_set.push_back(right_it->first);
      temp_result.push_back(f(op, left, right_it->second));
    }
    ++right_it;
  }
}
#else
template <typename OpType, typename RetType>
void MutationManager::process_T_calc_goodvar(int op, OpType left, OpType right,
                                             RetType (*f)(int, OpType, OpType),
                                             int left_id, int right_id) {
  GoodVarTableType &left_goodvar = get_goodvar_table()[left_id];
  GoodVarTableType &right_goodvar = get_goodvar_table()[right_id];

  auto left_it = left_goodvar.begin();
  auto right_it = right_goodvar.begin();

  if (MUTATION_ID == 0) {
    for (; left_it != left_goodvar.end() && right_it != right_goodvar.end();) {
      if (left_it->first < right_it->first) {
        if (default_active_set[left_it->first] == 1) {
          recent_set.push_back(left_it->first);
          temp_result.push_back(f(op, left_it->second, right));
        }
        ++left_it;
      } else if (left_it->first > right_it->first) {
        if (default_active_set[right_it->first] == 1) {
          recent_set.push_back(right_it->first);
          temp_result.push_back(f(op, left, right_it->second));
        }
        ++right_it;
      } else {
        if (default_active_set[left_it->first] == 1) {
          recent_set.push_back(left_it->first);
          temp_result.push_back(f(op, left_it->second, right_it->second));
        }
        ++left_it;
        ++right_it;
      }
    }
    while (left_it != left_goodvar.end()) {
      if (default_active_set[left_it->first] == 1) {
        recent_set.push_back(left_it->first);
        temp_result.push_back(f(op, left_it->second, right));
      }
      ++left_it;
    }
    while (right_it != right_goodvar.end()) {
      if (default_active_set[right_it->first] == 1) {
        recent_set.push_back(right_it->first);
        temp_result.push_back(f(op, left, right_it->second));
      }
      ++right_it;
    }
  } else {
    auto active_it = forked_active_set.begin();
    while (left_it != left_goodvar.end() && right_it != right_goodvar.end()) {
      if (*active_it > left_it->first) {
        ++left_it;
        continue;
      }
      if (*active_it > right_it->first) {
        ++right_it;
        continue;
      }
      // invariant: min(left_it->first, right_it->first) == *active_it
      if (left_it->first < right_it->first) {
        recent_set.push_back(left_it->first);
        temp_result.push_back(f(op, left_it->second, right));
        ++left_it;
      } else if (left_it->first > right_it->first) {
        recent_set.push_back(right_it->first);
        temp_result.push_back(f(op, left, right_it->second));
        ++right_it;
      } else {
        recent_set.push_back(left_it->first);
        temp_result.push_back(f(op, left_it->second, right_it->second));
        ++left_it;
        ++right_it;
      }
    }
    while (left_it != left_goodvar.end() &&
           active_it != forked_active_set.end()) {
      if (left_it->first == *active_it) {
        recent_set.push_back(left_it->first);
        temp_result.push_back(f(op, left_it->second, right));
        ++active_it;
        ++left_it;
      } else if (left_it->first < *active_it) {
        ++left_it;
      } else {
        assert(false);
      }
    }
    while (right_it != right_goodvar.end()) {
      if (right_it->first == *active_it) {
        recent_set.push_back(right_it->first);
        temp_result.push_back(f(op, left, right_it->second));
        ++active_it;
        ++right_it;
      } else if (right_it->first < *active_it) {
        ++right_it;
      } else {
        assert(false);
      }
    }
  }
}
#endif

template <typename OpType, typename RetType>
void MutationManager::process_T_calc_goodvar_for_badvar_and_forked(
    int op, OpType left, OpType right, RetType (*f)(int, OpType, OpType),
    int left_id, int right_id, int from) {
  GoodVarTableType &left_goodvar = get_goodvar_table()[left_id];
  GoodVarTableType &right_goodvar = get_goodvar_table()[right_id];

  auto left_it = left_goodvar.begin();
  auto right_it = right_goodvar.begin();

  auto active_it = forked_active_set.begin();
  while (active_it != forked_active_set.end() && *active_it < from) {
    bool run = false;
    int cur = *active_it;
    OpType leftOperand = left;
    OpType rightOperand = right;

    if (left_it != left_goodvar.end() && left_it->first == cur) {
      run = true;
      leftOperand = left_it->second;
      ++left_it;
    }
    if (right_it != right_goodvar.end() && right_it->first == cur) {
      run = true;
      rightOperand = right_it->second;
      ++right_it;
    }
    if (run) {
      recent_set.push_back(cur);
      temp_result.push_back(f(op, leftOperand, rightOperand));
    }
    ++active_it;
  }
}

int MutationManager::process_i32_arith_goodvar(int left, int right,
                                               GoodvarArg *arg) {
  return process_T_arith_goodvar<int32_t, int32_t, false, CalcType::I32Arith>(
      left, right, arg);
}

int64_t MutationManager::process_i64_arith_goodvar(int64_t left, int64_t right,
                                                   GoodvarArg *arg) {
  return process_T_arith_goodvar<int64_t, int64_t, false, CalcType::I64Arith>(
      left, right, arg);
}

int32_t MutationManager::process_i32_cmp_goodvar(int left, int right,
                                                 GoodvarArg *arg) {
  return process_T_arith_goodvar<int32_t, int32_t, false, CalcType::I32Bool>(
      left, right, arg);
}

int32_t MutationManager::process_i64_cmp_goodvar(int64_t left, int64_t right,
                                                 GoodvarArg *arg) {
  return process_T_arith_goodvar<int64_t, int32_t, false, CalcType::I64Bool>(
      left, right, arg);
}

int MutationManager::process_i32_arith_goodvar_init(int left, int right,
                                                    GoodvarArg *arg) {
  return process_T_arith_goodvar<int32_t, int32_t, true, CalcType::I32Arith>(
      left, right, arg);
}

int64_t MutationManager::process_i64_arith_goodvar_init(int64_t left,
                                                        int64_t right,
                                                        GoodvarArg *arg) {
  return process_T_arith_goodvar<int64_t, int64_t, true, CalcType::I64Arith>(
      left, right, arg);
}

int32_t MutationManager::process_i32_cmp_goodvar_init(int left, int right,
                                                      GoodvarArg *arg) {
  return process_T_arith_goodvar<int32_t, int32_t, true, CalcType::I32Bool>(
      left, right, arg);
}

int32_t MutationManager::process_i64_cmp_goodvar_init(int64_t left,
                                                      int64_t right,
                                                      GoodvarArg *arg) {
  return process_T_arith_goodvar<int64_t, int32_t, true, CalcType::I64Bool>(
      left, right, arg);
}

#endif // LLVM_MUTATIONMANAGER_H
