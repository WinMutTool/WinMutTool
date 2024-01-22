#ifndef LLVM_MUTATIONSTORAGE_H
#define LLVM_MUTATIONSTORAGE_H

enum MType {
  AOR, /* 0 */
  LOR, /* 1 */
  COR, /* 2 */
  ROR, /* 3 */
  SOR, /* 4 */
  STD, /* 5 */
  LVR, /* 6 */
  UOI, /* 7 */
  ROV, /* 8 */
  ABV  /* 9 */
};

struct Mutation {
  MType type;

#if ACCMUT_STATIC_ANALYSIS_EVAL
  int location;
#endif

  // src operand, for all muts
  int sop;

  // AOR,LOR->t_op
  // LVR,ABV->index;
  int op_0;

  // ROR->s_pre & t_pre
  // LVR->src_const & tar_const
  // ROV->op1 & op2
  // STD->func_type & retval
  // UOI->idx & tp
  long op_1;
  long op_2;
};

struct RegMutInfo {
  Mutation *ptr;
  int num;
  int offset;
  int isReg;
  const char *moduleName;
};

#endif // LLVM_MUTATIONSTORAGE_H
