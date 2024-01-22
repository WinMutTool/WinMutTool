#ifndef LLVM_MACROS_H
#define LLVM_MACROS_H

#ifdef __APPLE__
#define APPLE_OR_GLIBC(apple, glibc) apple
#else
#define APPLE_OR_GLIBC(apple, glibc) (glibc)
#endif

#define PAGE_SIZE 4096

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#endif // LLVM_MACROS_H
