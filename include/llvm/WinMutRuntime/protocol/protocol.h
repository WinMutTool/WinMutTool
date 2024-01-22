#ifndef LLVM_PROTOCOL_H
#define LLVM_PROTOCOL_H

#include <unistd.h>

struct MessageHdr {
  enum MessageType { CACHE, PANIC } type;
  int length;
};

int readHdr(int fd, MessageHdr *buf);

int readMsg(int fd, MessageHdr header, char *buf);

#endif // LLVM_PROTOCOL_H
