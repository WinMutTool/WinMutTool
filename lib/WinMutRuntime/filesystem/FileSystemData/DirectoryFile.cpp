#include <dirent.h>
#include <llvm/WinMutRuntime/filesystem/FileSystemData/DirectoryFile.h>

namespace accmut {
void DirectoryFile::dump(FILE *f) {
  for (auto d : dirents) {
    fprintf(f, "%ld\t%s", (long)d.d_ino, d.d_name);
    if (d.d_type == DT_LNK)
      fprintf(f, "->\n");
    else if (d.d_type == DT_DIR)
      fprintf(f, "/\n");
    else if (d.d_type == DT_REG)
      fprintf(f, "\n");
    else
      fprintf(f, "?\n");
  }
}
} // namespace accmut
