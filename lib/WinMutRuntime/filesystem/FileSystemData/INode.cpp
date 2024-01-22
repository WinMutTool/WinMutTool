#include <llvm/WinMutRuntime/filesystem/FileSystemData/INode.h>
#include <llvm/WinMutRuntime/checking/panic.h>

namespace accmut {
void INode::dump(FILE *f) {
  fprintf(f, "ACCESS: %o\n", meta.st_mode & (~S_IFMT));
  fprintf(f, "INODE: %lu\n", meta.st_ino);
  if (content)
    content->dump(f);
  else
    fprintf(f, "Unknown content\n");
}
void INode::deleteBTPos(ino_t ino, const std::string &name) {
  for (auto b = btpos.begin(); b != btpos.end(); ++b) {
    if ((*b)->ino == ino && (*b)->name == name) {
      (*b)->setExpire();
      btpos.erase(b);
      return;
    }
  }
  panic("Should not get here");
}
void INode::trunc() {
  if (!isReg())
    panic("Should not call this");
  meta.st_size = 0;
  std::static_pointer_cast<RegularFile>(content)->trunc();
}
  ssize_t INode::write(const void *buf, size_t nbyte, size_t off,
                       std::error_code &err) {
    if (isDir() || isLnk()) {
      err = make_system_error_code(std::errc::permission_denied);
    }
    if (isReg())
      if (!cached())
        panic("Failed to write to uncached file");
    ssize_t realbytes = content->write(buf, nbyte, off, err);
    if (realbytes != -1) {
      if (realbytes + off > (size_t)meta.st_size)
        meta.st_size = realbytes + off;
    }
    return realbytes;
  }

  ssize_t INode::read(void *buf, size_t nbyte, size_t off,
                      std::error_code &err) {
    if (isDir()) {
      err = make_system_error_code(std::errc::is_a_directory);
      return -1;
    }
    if (isLnk()) {
      err = make_system_error_code(std::errc::permission_denied);
      return -1;
    }
    if (!cached())
      panic("Failed to read from uncached file");
    return content->read(buf, nbyte, off, err);
  }

} // namespace accmut
