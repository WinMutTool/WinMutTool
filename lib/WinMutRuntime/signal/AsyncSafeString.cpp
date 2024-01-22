#include "llvm/WinMutRuntime/signal/AsyncSafeString.h"

unsigned int strlen_async_safe(const char *s) {
  unsigned int len = 0;
  while (*s++)
    ++len;
  return len;
}

char *strcat_async_safe(char *dest, const char *src) {
  char *cp = dest;
  while (*cp)
    ++cp;
  while (((*cp++) = (*src++)))
    ;
  return dest;
}

char *itoa_async_safe(long n, int base) {
  static char buf[36];
  char *p = &buf[36];
  *(--p) = '\0';
  int minus;
  if (n < 0) {
    minus = 1;
    n = 0 - n;
  } else
    minus = 0;

  if (n == 0) {
    *(--p) = '0';
  } else
    while (n > 0) {
      *(--p) = "01234567890abcdef"[n % base];
      n /= base;
    }
  if (minus)
    *(--p) = '-';
  return p;
}
