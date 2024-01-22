#ifndef LLVM_MEMORY_H
#define LLVM_MEMORY_H

extern "C" {
void init_memory_hook(void);
bool memory_func_called();
}

#endif // LLVM_MEMORY_H
