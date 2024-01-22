#ifndef LLVM_EXITFUNC_H
#define LLVM_EXITFUNC_H

#include <string>

void setOriginalReturnVal(int val);
int getOriginalReturnVal();
const std::string &getPrintLine();
void setPrintLine(const std::string &str);

#endif // LLVM_EXITFUNC_H
