#include "llvm/WinMutRuntime/protocol/protocol.h"
#include "llvm/WinMutRuntime/filesystem/LibCAPI.h"

int readHdr(int fd, MessageHdr *buf) {
  int remaining = 8;
  int num = 0;
  char *hdr = (char *)buf;
  while (remaining != 0 && (num = __accmut_libc_read(fd, buf + (8 - remaining),
                                                     remaining)) != 0) {
    if (num == -1)
      return -1;
    remaining -= num;
  }
  if (remaining == 0)
    return 0;
  return -1;
}

int readMsg(int fd, MessageHdr header, char *buf) {
  int remaining = header.length;
  int num = 0;
  while (remaining != 0 &&
         (num = __accmut_libc_read(fd, buf + (header.length - remaining),
                                   remaining)) != 0) {
    if (num == -1)
      return -1;
    remaining -= num;
  }
  buf[header.length] = 0;
  if (remaining == 0)
    return 0;
  return 1;
}
