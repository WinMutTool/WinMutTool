#ifndef LLVM_ASYNCSAFESTRING_H
#define LLVM_ASYNCSAFESTRING_H

unsigned int strlen_async_safe(const char *s);

char *strcat_async_safe(char *dest, const char *src);

char *itoa_async_safe(long n, int base);

#endif // LLVM_ASYNCSAFESTRING_H
