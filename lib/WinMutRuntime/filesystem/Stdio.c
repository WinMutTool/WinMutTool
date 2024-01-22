#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include "llvm/WinMutRuntime/filesystem/Stdio.h"
#include "llvm/WinMutRuntime/init/init.h"
#include <fcntl.h>
#include <llvm/WinMutRuntime/filesystem/LibCAPI.h>
#include <unistd.h>
static _unix_read_t _saved_read = __accmut_libc_read;

static _unix_seek_t _saved_seek = __accmut_libc_lseek;

static _unix_close_t _saved_close = __accmut_libc_close;

static _unix_write_t _saved_write = __accmut_libc_write;

static _unix_open_t _saved_open = __accmut_libc_open;

static _unix_fcntl_t _saved_fcntl = __accmut_libc_fcntl;

static _unix_dup2_t _saved_dup2 = __accmut_libc_dup2;

static _unix_fstat_t _saved_fstat = __accmut_libc_fstat;

#ifndef __APPLE__
static _unix_dup3_t _saved_dup3 = __accmut_libc_dup3;
#endif

void inject_read(_unix_read_t func) { _saved_read = func; }

void inject_write(_unix_write_t func) { _saved_write = func; }

void inject_seek(_unix_seek_t func) { _saved_seek = func; }

void inject_close(_unix_close_t func) { _saved_close = func; }

void inject_open(_unix_open_t func) { _saved_open = func; }

void inject_fcntl(_unix_fcntl_t func) { _saved_fcntl = func; }

void inject_dup2(_unix_dup2_t func) { _saved_dup2 = func; }

#ifndef __APPLE__
void inject_dup3(_unix_dup3_t func) { _saved_dup3 = func; }
#endif

void inject_fstat(_unix_fstat_t func) { _saved_fstat = func; }

#ifndef __APPLE__

ssize_t _read_vfunc(FILE *cookie, void *buf, ssize_t n) {
  FILE *fp = cookie;
  return (_saved_read(fp->_fileno, buf, (size_t)n));
}

ssize_t _write_vfunc(FILE *cookie, const void *buf, ssize_t n) {
  FILE *fp = cookie;
  return (_saved_write(fp->_fileno, buf, (size_t)n));
}

off64_t _seek_vfunc(FILE *cookie, off64_t offset, int whence) {
  FILE *fp = cookie;
  return (_saved_seek(fp->_fileno, (off_t)offset, whence));
}

int _close_vfunc(FILE *cookie) {
  return _saved_close(((FILE *)cookie)->_fileno);
}

int _fstat_vfunc(FILE *cookie, void *buf) {
  return _saved_fstat(cookie->_fileno, buf);
}

ssize_t _read_cookie_func(void *cookie, char *buf, size_t n) {
  return _read_vfunc((FILE *)cookie, buf, n);
}

ssize_t _write_cookie_func(void *cookie, const char *buf, size_t n) {
  return _write_vfunc((FILE *)cookie, buf, n);
}

int _seek_cookie_func(void *cookie, off64_t *offset, int whence) {
  if (offset == NULL) {
    return -1;
  }
  off64_t result = _seek_vfunc((FILE *)cookie, *offset, whence);
  *offset = result;
  return 0;
}

int _close_cookie_func(void *cookie) { return _close_vfunc((FILE *)cookie); }

#else

int _read_vfunc(void *cookie, char *buf, int n) {
  FILE *fp = cookie;
  return (_saved_read(fp->_file, buf, (size_t)n));
}

int _write_vfunc(void *cookie, const char *buf, int n) {
  FILE *fp = cookie;
  return (_saved_write(fp->_file, buf, (size_t)n));
}

fpos_t _seek_vfunc(void *cookie, fpos_t offset, int whence) {
  FILE *fp = cookie;
  return (_saved_seek(fp->_file, (off_t)offset, whence));
}

int _close_vfunc(void *cookie) { return _saved_close(((FILE *)cookie)->_file); }
#endif

#ifndef __APPLE__
#define _IO_lock_initializer                                                   \
  { 0, 0, NULL }

#define _IO_lock_init(_name)                                                   \
  ((void)((_name) = (_IO_lock_t)_IO_lock_initializer))

#include "llvm/WinMutRuntime/filesystem/Stdio.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define _IO_pos_BAD ((off64_t)-1)

#define _IO_seek_set 0
#define _IO_seek_cur 1
#define _IO_seek_end 2

/* THE JUMPTABLE FUNCTIONS.

 * The _IO_FILE type is used to implement the FILE type in GNU libc,
 * as well as the streambuf class in GNU iostreams for C++.
 * These are all the same, just used differently.
 * An _IO_FILE (or FILE) object is allows followed by a pointer to
 * a jump table (of pointers to functions).  The pointer is accessed
 * with the _IO_JUMPS macro.  The jump table has an eccentric format,
 * so as to be compatible with the layout of a C++ virtual function table.
 * (as implemented by g++).  When a pointer to a streambuf object is
 * coerced to an (FILE*), then _IO_JUMPS on the result just
 * happens to point to the virtual function table of the streambuf.
 * Thus the _IO_JUMPS function table used for C stdio/libio does
 * double duty as the virtual function table for C++ streambuf.
 *
 * The entries in the _IO_JUMPS function table (and hence also the
 * virtual functions of a streambuf) are described below.
 * The first parameter of each function entry is the _IO_FILE/streambuf
 * object being acted on (i.e. the 'this' parameter).
 */

/* Setting this macro to 1 enables the use of the _vtable_offset bias
   in _IO_JUMPS_FUNCS, below.  This is only needed for new-format
   _IO_FILE in libc that must support old binaries (see oldfileops.c).  */
#define _IO_JUMPS_OFFSET 0

/* Type of MEMBER in struct type TYPE.  */
#define _IO_MEMBER_TYPE(TYPE, MEMBER) __typeof__(((TYPE){}).MEMBER)
#define offsetof(type, ident) ((size_t) & (((type *)0)->ident))
/* Essentially ((TYPE *) THIS)->MEMBER, but avoiding the aliasing
   violation in case THIS has a different pointer type.  */
#define _IO_CAST_FIELD_ACCESS(THIS, TYPE, MEMBER)                              \
  (*(_IO_MEMBER_TYPE(TYPE, MEMBER) *)(((char *)(THIS)) +                       \
                                      offsetof(TYPE, MEMBER)))

#define _IO_JUMPS(THIS) (THIS)->vtable
#define _IO_JUMPS_FILE_plus(THIS)                                              \
  _IO_CAST_FIELD_ACCESS((THIS), struct _IO_FILE_plus, vtable)
#define _IO_WIDE_JUMPS(THIS)                                                   \
  _IO_CAST_FIELD_ACCESS((THIS), struct _IO_FILE, _wide_data)->_wide_vtable
#define _IO_CHECK_WIDE(THIS)                                                   \
  (_IO_CAST_FIELD_ACCESS((THIS), struct _IO_FILE, _wide_data) != NULL)

#if _IO_JUMPS_OFFSET
#define _IO_JUMPS_FUNC(THIS)                                                   \
  (IO_validate_vtable(                                                         \
      *(struct _IO_jump_t **)((void *)&_IO_JUMPS_FILE_plus(THIS) +             \
                              (THIS)->_vtable_offset)))
#define _IO_vtable_offset(THIS) (THIS)->_vtable_offset
#else
#define _IO_JUMPS_FUNC(THIS) (_IO_JUMPS_FILE_plus(THIS))
#define _IO_vtable_offset(THIS) 0
#endif
#define _IO_WIDE_JUMPS_FUNC(THIS) _IO_WIDE_JUMPS(THIS)
#define JUMP_FIELD(TYPE, NAME) TYPE NAME
#define JUMP0(FUNC, THIS) (_IO_JUMPS_FUNC(THIS)->FUNC)(THIS)
#define JUMP1(FUNC, THIS, X1) (_IO_JUMPS_FUNC(THIS)->FUNC)(THIS, X1)
#define JUMP2(FUNC, THIS, X1, X2) (_IO_JUMPS_FUNC(THIS)->FUNC)(THIS, X1, X2)
#define JUMP3(FUNC, THIS, X1, X2, X3)                                          \
  (_IO_JUMPS_FUNC(THIS)->FUNC)(THIS, X1, X2, X3)
#define JUMP_INIT(NAME, VALUE) VALUE
#define JUMP_INIT_DUMMY JUMP_INIT(dummy, 0), JUMP_INIT(dummy2, 0)

#define WJUMP0(FUNC, THIS) (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC)(THIS)
#define WJUMP1(FUNC, THIS, X1) (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC)(THIS, X1)
#define WJUMP2(FUNC, THIS, X1, X2)                                             \
  (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC)(THIS, X1, X2)
#define WJUMP3(FUNC, THIS, X1, X2, X3)                                         \
  (_IO_WIDE_JUMPS_FUNC(THIS)->FUNC)(THIS, X1, X2, X3)

/* The 'finish' function does any final cleaning up of an _IO_FILE object.
   It does not delete (free) it, but does everything else to finalize it.
   It matches the streambuf::~streambuf virtual destructor.  */
#define _IO_FINISH(FP) JUMP1(__finish, FP, 0)
#define _IO_WFINISH(FP) WJUMP1(__finish, FP, 0)

/* The 'overflow' hook flushes the buffer.
   The second argument is a character, or EOF.
   It matches the streambuf::overflow virtual function. */
#define _IO_OVERFLOW(FP, CH) JUMP1(__overflow, FP, CH)
#define _IO_WOVERFLOW(FP, CH) WJUMP1(__overflow, FP, CH)

/* The 'underflow' hook tries to fills the get buffer.
   It returns the next character (as an unsigned char) or EOF.  The next
   character remains in the get buffer, and the get position is not changed.
   It matches the streambuf::underflow virtual function. */
#define _IO_UNDERFLOW(FP) JUMP0(__underflow, FP)
#define _IO_WUNDERFLOW(FP) WJUMP0(__underflow, FP)

/* The 'uflow' hook returns the next character in the input stream
   (cast to unsigned char), and increments the read position;
   EOF is returned on failure.
   It matches the streambuf::uflow virtual function, which is not in the
   cfront implementation, but was added to C++ by the ANSI/ISO committee. */
#define _IO_UFLOW(FP) JUMP0(__uflow, FP)
#define _IO_WUFLOW(FP) WJUMP0(__uflow, FP)

/* The 'pbackfail' hook handles backing up.
   It matches the streambuf::pbackfail virtual function. */
#define _IO_PBACKFAIL(FP, CH) JUMP1(__pbackfail, FP, CH)
#define _IO_WPBACKFAIL(FP, CH) WJUMP1(__pbackfail, FP, CH)

/* The 'xsputn' hook writes upto N characters from buffer DATA.
   Returns EOF or the number of character actually written.
   It matches the streambuf::xsputn virtual function. */
#define _IO_XSPUTN(FP, DATA, N) JUMP2(__xsputn, FP, DATA, N)
#define _IO_WXSPUTN(FP, DATA, N) WJUMP2(__xsputn, FP, DATA, N)

/* The 'xsgetn' hook reads upto N characters into buffer DATA.
   Returns the number of character actually read.
   It matches the streambuf::xsgetn virtual function. */
#define _IO_XSGETN(FP, DATA, N) JUMP2(__xsgetn, FP, DATA, N)
#define _IO_WXSGETN(FP, DATA, N) WJUMP2(__xsgetn, FP, DATA, N)

/* The 'seekoff' hook moves the stream position to a new position
   relative to the start of the file (if DIR==0), the current position
   (MODE==1), or the end of the file (MODE==2).
   It matches the streambuf::seekoff virtual function.
   It is also used for the ANSI fseek function. */
#define _IO_SEEKOFF(FP, OFF, DIR, MODE) JUMP3(__seekoff, FP, OFF, DIR, MODE)
#define _IO_WSEEKOFF(FP, OFF, DIR, MODE) WJUMP3(__seekoff, FP, OFF, DIR, MODE)

/* The 'seekpos' hook also moves the stream position,
   but to an absolute position given by a fpos64_t (seekpos).
   It matches the streambuf::seekpos virtual function.
   It is also used for the ANSI fgetpos and fsetpos functions.  */
/* The _IO_seek_cur and _IO_seek_end options are not allowed. */
#define _IO_SEEKPOS(FP, POS, FLAGS) JUMP2(__seekpos, FP, POS, FLAGS)
#define _IO_WSEEKPOS(FP, POS, FLAGS) WJUMP2(__seekpos, FP, POS, FLAGS)

/* The 'setbuf' hook gives a buffer to the file.
   It matches the streambuf::setbuf virtual function. */
#define _IO_SETBUF(FP, BUFFER, LENGTH) JUMP2(__setbuf, FP, BUFFER, LENGTH)
#define _IO_WSETBUF(FP, BUFFER, LENGTH) WJUMP2(__setbuf, FP, BUFFER, LENGTH)

/* The 'sync' hook attempts to synchronize the internal data structures
   of the file with the external state.
   It matches the streambuf::sync virtual function. */
#define _IO_SYNC(FP) JUMP0(__sync, FP)
#define _IO_WSYNC(FP) WJUMP0(__sync, FP)

/* The 'doallocate' hook is used to tell the file to allocate a buffer.
   It matches the streambuf::doallocate virtual function, which is not
   in the ANSI/ISO C++ standard, but is part traditional implementations. */
#define _IO_DOALLOCATE(FP) JUMP0(__doallocate, FP)
#define _IO_WDOALLOCATE(FP) WJUMP0(__doallocate, FP)

/* The following four hooks (sysread, syswrite, sysclose, sysseek, and
   sysstat) are low-level hooks specific to this implementation.
   There is no correspondence in the ANSI/ISO C++ standard library.
   The hooks basically correspond to the Unix system functions
   (read, write, close, lseek, and stat) except that a FILE*
   parameter is used instead of an integer file descriptor;  the default
   implementation used for normal files just calls those functions.
   The advantage of overriding these functions instead of the higher-level
   ones (underflow, overflow etc) is that you can leave all the buffering
   higher-level functions.  */

/* The 'sysread' hook is used to read data from the external file into
   an existing buffer.  It generalizes the Unix read(2) function.
   It matches the streambuf::sys_read virtual function, which is
   specific to this implementation. */
#define _IO_SYSREAD(FP, DATA, LEN) JUMP2(__read, FP, DATA, LEN)
#define _IO_WSYSREAD(FP, DATA, LEN) WJUMP2(__read, FP, DATA, LEN)

/* The 'syswrite' hook is used to write data from an existing buffer
   to an external file.  It generalizes the Unix write(2) function.
   It matches the streambuf::sys_write virtual function, which is
   specific to this implementation. */
#define _IO_SYSWRITE(FP, DATA, LEN) JUMP2(__write, FP, DATA, LEN)
#define _IO_WSYSWRITE(FP, DATA, LEN) WJUMP2(__write, FP, DATA, LEN)

/* The 'sysseek' hook is used to re-position an external file.
   It generalizes the Unix lseek(2) function.
   It matches the streambuf::sys_seek virtual function, which is
   specific to this implementation. */
#define _IO_SYSSEEK(FP, OFFSET, MODE) JUMP2(__seek, FP, OFFSET, MODE)
#define _IO_WSYSSEEK(FP, OFFSET, MODE) WJUMP2(__seek, FP, OFFSET, MODE)

/* The 'sysclose' hook is used to finalize (close, finish up) an
   external file.  It generalizes the Unix close(2) function.
   It matches the streambuf::sys_close virtual function, which is
   specific to this implementation. */
#define _IO_SYSCLOSE(FP) JUMP0(__close, FP)
#define _IO_WSYSCLOSE(FP) WJUMP0(__close, FP)

/* The 'sysstat' hook is used to get information about an external file
   into a struct stat buffer.  It generalizes the Unix fstat(2) call.
   It matches the streambuf::sys_stat virtual function, which is
   specific to this implementation. */
#define _IO_SYSSTAT(FP, BUF) JUMP1(__stat, FP, BUF)
#define _IO_WSYSSTAT(FP, BUF) WJUMP1(__stat, FP, BUF)

/* The 'showmany' hook can be used to get an image how much input is
   available.  In many cases the answer will be 0 which means unknown
   but some cases one can provide real information.  */
#define _IO_SHOWMANYC(FP) JUMP0(__showmanyc, FP)
#define _IO_WSHOWMANYC(FP) WJUMP0(__showmanyc, FP)

/* The 'imbue' hook is used to get information about the currently
   installed locales.  */
#define _IO_IMBUE(FP, LOCALE) JUMP1(__imbue, FP, LOCALE)
#define _IO_WIMBUE(FP, LOCALE) WJUMP1(__imbue, FP, LOCALE)

/* We always allocate an extra word following an _IO_FILE.
   This contains a pointer to the function jump table used.
   This is for compatibility with C++ streambuf; the word can
   be used to smash to a pointer to a virtual function table. */
#define JUMP_FIELD(TYPE, NAME) TYPE NAME
struct _IO_jump_t {
  JUMP_FIELD(size_t, __dummy);
  JUMP_FIELD(size_t, __dummy2);
  JUMP_FIELD(_IO_finish_t, __finish);
  JUMP_FIELD(_IO_overflow_t, __overflow);
  JUMP_FIELD(_IO_underflow_t, __underflow);
  JUMP_FIELD(_IO_underflow_t, __uflow);
  JUMP_FIELD(_IO_pbackfail_t, __pbackfail);
  /* showmany */
  JUMP_FIELD(_IO_xsputn_t, __xsputn);
  JUMP_FIELD(_IO_xsgetn_t, __xsgetn);
  JUMP_FIELD(_IO_seekoff_t, __seekoff);
  JUMP_FIELD(_IO_seekpos_t, __seekpos);
  JUMP_FIELD(_IO_setbuf_t, __setbuf);
  JUMP_FIELD(_IO_sync_t, __sync);
  JUMP_FIELD(_IO_doallocate_t, __doallocate);
  JUMP_FIELD(_IO_read_t, __read);
  JUMP_FIELD(_IO_write_t, __write);
  JUMP_FIELD(_IO_seek_t, __seek);
  JUMP_FIELD(_IO_close_t, __close);
  JUMP_FIELD(_IO_stat_t, __stat);
  JUMP_FIELD(_IO_showmanyc_t, __showmanyc);
  JUMP_FIELD(_IO_imbue_t, __imbue);
};

struct _IO_FILE_plus {
  FILE file;
  const struct _IO_jump_t *vtable;
};

typedef ssize_t cookie_read_function_t(void *, char *, size_t);
typedef ssize_t cookie_write_function_t(void *, const char *, size_t);
typedef int cookie_seek_function_t(void *, off64_t *, int whence);
typedef int cookie_close_function_t(void *);
typedef struct {
  cookie_read_function_t *read;
  cookie_write_function_t *write;
  cookie_seek_function_t *seek;
  cookie_close_function_t *close;
} cookie_io_functions_t;
#include <fcntl.h>
#include <gconv.h>

#define _IO_JUMPS(THIS) (THIS)->vtable
#define __set_errno(val) (errno = (val))
#define _IO_mask_flags(fp, f, mask)                                            \
  ((fp)->_flags = ((fp)->_flags & ~(mask)) | ((f) & (mask)))
#define _IO_MAGIC 0xFBAD0000 /* Magic number */
#define _IO_MAGIC_MASK 0xFFFF0000
#define _IO_USER_BUF 0x0001 /* Don't deallocate buffer on close. */
#define _IO_UNBUFFERED 0x0002
#define _IO_NO_READS 0x0004  /* Reading not allowed.  */
#define _IO_NO_WRITES 0x0008 /* Writing not allowed.  */
#define _IO_EOF_SEEN 0x0010
#define _IO_ERR_SEEN 0x0020
#define _IO_DELETE_DONT_CLOSE 0x0040 /* Don't call close(_fileno) on close. */
#define _IO_LINKED 0x0080            /* In the list of all open files.  */
#define _IO_IN_BACKUP 0x0100
#define _IO_LINE_BUF 0x0200
#define _IO_TIED_PUT_GET 0x0400 /* Put and get pointer move in unison.  */
#define _IO_CURRENTLY_PUTTING 0x0800
#define _IO_IS_APPENDING 0x1000
#define _IO_IS_FILEBUF 0x2000
/* 0x4000  No longer used, reserved for compat.  */
#define _IO_USER_LOCK 0x8000

extern const struct _IO_jump_t _IO_wfile_jumps;
extern const struct _IO_jump_t _IO_file_jumps;
struct _IO_jump_t _IO_file_jumps_injected;
struct _IO_jump_t _IO_file_jumps_donothing_close;

ssize_t (*_orig_read_vfunc)(FILE *, void *, ssize_t);
ssize_t (*_orig_write_vfunc)(FILE *, const void *, ssize_t);
off64_t (*_orig_seek_vfunc)(FILE *, off64_t, int);
int (*_orig_close_vfunc)(FILE *);
int (*_orig_fstat_vfunc)(FILE *, void *);

int _nothing_close(FILE *f) { return 0; }

/* Standard streams.  */
extern FILE *stdin;  /* Standard input stream.  */
extern FILE *stdout; /* Standard output stream.  */
extern FILE *stderr; /* Standard error output stream.  */
/* C89/C99 say they're macros.  Make them happy.  */
#define stdin stdin
#define stdout stdout
#define stderr stderr
#define EOF (-1)
const struct _IO_jump_t *stdin_vtable;
const struct _IO_jump_t *stdout_vtable;
const struct _IO_jump_t *stderr_vtable;

void set_injected_table() {
  // bypass vtable check
  struct locked_FILE {
    struct _IO_FILE_plus cfile;
    _IO_lock_t lock;
  } * new_f;

  new_f = (struct locked_FILE *)malloc(sizeof(struct locked_FILE));

  new_f->cfile.file._lock = &new_f->lock;

  extern void _IO_init(FILE * fp, int flags);
  _IO_init(&new_f->cfile.file, 0);
  free(new_f);

  // build injected table
  memcpy(&_IO_file_jumps_injected, &_IO_file_jumps, sizeof(struct _IO_jump_t));
  memcpy(&_IO_file_jumps_donothing_close, &_IO_file_jumps,
         sizeof(struct _IO_jump_t));
  _IO_file_jumps_donothing_close.__close = _nothing_close;
  _orig_close_vfunc = _IO_file_jumps_injected.__close;
  _orig_write_vfunc = _IO_file_jumps_injected.__write;
  _orig_read_vfunc = _IO_file_jumps_injected.__read;
  _orig_seek_vfunc = _IO_file_jumps_injected.__seek;
  _orig_fstat_vfunc = _IO_file_jumps_injected.__stat;
  _IO_file_jumps_injected.__close = _close_vfunc;
  _IO_file_jumps_injected.__write = _write_vfunc;
  _IO_file_jumps_injected.__read = _read_vfunc;
  _IO_file_jumps_injected.__seek = _seek_vfunc;
  _IO_file_jumps_injected.__stat = _fstat_vfunc;

  // set stdio table
  stdin_vtable = ((struct _IO_FILE_plus *)stdin)->vtable;
  ((struct _IO_FILE_plus *)stdin)->vtable = &_IO_file_jumps_injected;
  stdout_vtable = ((struct _IO_FILE_plus *)stdout)->vtable;
  ((struct _IO_FILE_plus *)stdout)->vtable = &_IO_file_jumps_injected;
  stderr_vtable = ((struct _IO_FILE_plus *)stderr)->vtable;
  ((struct _IO_FILE_plus *)stderr)->vtable = &_IO_file_jumps_injected;
}

void unload_stdio() {
  _IO_file_jumps_injected.__close = _orig_close_vfunc;
  _IO_file_jumps_injected.__write = _orig_write_vfunc;
  _IO_file_jumps_injected.__read = _orig_read_vfunc;
  _IO_file_jumps_injected.__seek = _orig_seek_vfunc;
  _IO_file_jumps_injected.__stat = _orig_fstat_vfunc;
}

extern int fclose(FILE *__stream);
extern FILE *fopencookie(void *__restrict __magic_cookie,
                         const char *__restrict __modes,
                         cookie_io_functions_t __io_funcs) __THROW __wur;
extern int fileno(FILE *__stream) __THROW __wur;

void reset_stdio() {
  ((struct _IO_FILE_plus *)stdin)->vtable = stdin_vtable;
  ((struct _IO_FILE_plus *)stdout)->vtable = stdout_vtable;
  ((struct _IO_FILE_plus *)stderr)->vtable = stderr_vtable;
}

FILE *fdopen(int fd, const char *mode) {
  if (system_disabled())
    return __accmut_libc_fdopen(fd, mode);
  if (!system_initialized()) {
    disable_system();
    return __accmut_libc_fdopen(fd, mode);
  }
  int read_write;
  int i;
  bool do_seek = false;
  const char *orimode = mode;

  switch (*mode) {
  case 'r':
    read_write = _IO_NO_WRITES;
    break;
  case 'w':
    read_write = _IO_NO_READS;
    break;
  case 'a':
    read_write = _IO_NO_READS | _IO_IS_APPENDING;
    break;
  default:
    __set_errno(EINVAL);
    return NULL;
  }
  for (i = 1; i < 5; ++i) {
    switch (*++mode) {
    case '\0':
      break;
    case '+':
      read_write &= _IO_IS_APPENDING;
      break;
    default:
      /* Ignore */
      continue;
    }
    break;
  }
  int fd_flags = _saved_fcntl(fd, F_GETFL);
  if (fd_flags == -1)
    return NULL;
  if (((fd_flags & O_ACCMODE) == O_RDONLY && !(read_write & _IO_NO_WRITES)) ||
      ((fd_flags & O_ACCMODE) == O_WRONLY && !(read_write & _IO_NO_READS))) {
    __set_errno(EINVAL);
    return NULL;
  }

  if ((read_write & _IO_IS_APPENDING) && !(fd_flags & O_APPEND)) {
    do_seek = true;
    if (_saved_fcntl(fd, F_SETFL, fd_flags | O_APPEND) == -1)
      return NULL;
  }

  cookie_io_functions_t io_functions = {.read = _read_cookie_func,
                                        .write = _write_cookie_func,
                                        .seek = _seek_cookie_func,
                                        .close = _close_cookie_func};
  FILE *new_f = fopencookie(NULL, orimode, io_functions);
  if (new_f == NULL)
    return NULL;

  new_f->_fileno = fd;
  new_f->_flags &= ~_IO_DELETE_DONT_CLOSE;
  ((struct _IO_FILE_plus *)new_f)->vtable = &_IO_file_jumps_injected;

  _IO_mask_flags(new_f, read_write,
                 _IO_NO_READS + _IO_NO_WRITES + _IO_IS_APPENDING);

  /* For append mode, set the file offset to the end of the file if we added
     O_APPEND to the file descriptor flags.  Don't update the offset cache
     though, since the file handle is not active.  */
  if (do_seek && ((read_write & (_IO_IS_APPENDING | _IO_NO_READS)) ==
                  (_IO_IS_APPENDING | _IO_NO_READS))) {
    off64_t new_pos = _saved_seek(fd, 0, _IO_seek_end);
    if (new_pos == _IO_pos_BAD && errno != ESPIPE)
      return NULL;
  }
  return new_f;
}

struct _IO_cookie_file {
  struct _IO_FILE_plus __fp;
  void *__cookie;
  cookie_io_functions_t __io_functions;
};

#define _IO_mask_flags(fp, f, mask)                                            \
  ((fp)->_flags = ((fp)->_flags & ~(mask)) | ((f) & (mask)))

static FILE *_new_file_open(FILE *file, const char *filename,
                            const char *mode) {
  int oflags = 0, omode;
  int oprot = 0666;
  int i;
  int read_write;
  switch (*mode) {
  case 'r':
    omode = O_RDONLY;
    read_write = _IO_NO_WRITES;
    break;
  case 'w':
    omode = O_WRONLY;
    oflags = O_CREAT | O_TRUNC;
    read_write = _IO_NO_READS;
    break;
  case 'a':
    omode = O_WRONLY;
    oflags = O_CREAT | O_APPEND;
    read_write = _IO_NO_READS | _IO_IS_APPENDING;
    break;
  default:
    __set_errno(EINVAL);
    return NULL;
  }
  for (i = 1; i < 7; ++i) {
    switch (*++mode) {
    case '\0':
      break;
    case '+':
      omode = O_RDWR;
      read_write &= _IO_IS_APPENDING;
      continue;
    default:
      continue;
    }
  }

  int fd = _saved_open(filename, omode | oflags, oprot);
  if (fd < 0)
    return NULL;
  file->_fileno = fd;
  _IO_mask_flags(file, read_write,
                 _IO_NO_READS + _IO_NO_WRITES + _IO_IS_APPENDING);
  if (file->_flags &
      (_IO_IS_APPENDING | _IO_NO_READS) == (_IO_IS_APPENDING | _IO_NO_READS)) {
    off64_t new_pos = _saved_seek(fd, 0, _IO_seek_end);
    if (new_pos == _IO_pos_BAD && errno != ESPIPE) {
      _saved_close(fd);
      return NULL;
    }
  }
  return file;
}

FILE *fopen(const char *filename, const char *mode) {
  if (system_disabled())
    return __accmut_libc_fopen(filename, mode);
  if (!system_initialized()) {
    disable_system();
    return __accmut_libc_fopen(filename, mode);
  }
  cookie_io_functions_t io_functions = {.read = _read_cookie_func,
                                        .write = _write_cookie_func,
                                        .seek = _seek_cookie_func,
                                        .close = _close_cookie_func};
  FILE *file = fopencookie(NULL, mode, io_functions);
  ((struct _IO_cookie_file *)file)->__cookie = file;

  if (!_new_file_open(file, filename, mode)) {
    ((struct _IO_FILE_plus *)file)->vtable = &_IO_file_jumps_donothing_close;
    fclose(file);
    return NULL;
  }
  ((struct _IO_FILE_plus *)file)->vtable = &_IO_file_jumps_injected;
  return file;
}

FILE *fopen64(const char *filename, const char *mode) {
  if (system_disabled())
    return __accmut_libc_fopen(filename, mode);
  if (!system_initialized()) {
    disable_system();
    return __accmut_libc_fopen(filename, mode);
  }
  cookie_io_functions_t io_functions = {.read = _read_cookie_func,
                                        .write = _write_cookie_func,
                                        .seek = _seek_cookie_func,
                                        .close = _close_cookie_func};
  FILE *file = fopencookie(NULL, mode, io_functions);
  ((struct _IO_cookie_file *)file)->__cookie = file;

  if (!_new_file_open(file, filename, mode)) {
    ((struct _IO_FILE_plus *)file)->vtable = &_IO_file_jumps_donothing_close;
    fclose(file);
    return NULL;
  }
  ((struct _IO_FILE_plus *)file)->vtable = &_IO_file_jumps_injected;
  return file;
}

unsigned _IO_adjust_column(unsigned start, const char *line, int count) {
  const char *ptr = line + count;
  while (ptr > line)
    if (*--ptr == '\n')
      return line + count - ptr - 1;
  return start + count;
}

#define _IO_setg(fp, eb, g, eg)                                                \
  ((fp)->_IO_read_base = (eb), (fp)->_IO_read_ptr = (g),                       \
   (fp)->_IO_read_end = (eg))
#define _IO_setp(__fp, __p, __ep)                                              \
  ((__fp)->_IO_write_base = (__fp)->_IO_write_ptr = __p,                       \
   (__fp)->_IO_write_end = (__ep))
#define _IO_have_backup(fp) ((fp)->_IO_save_base != NULL)
#define _IO_in_backup(fp) ((fp)->_flags & _IO_IN_BACKUP)
static size_t new_do_write(FILE *fp, const char *data, size_t to_do) {
  size_t count;
  if (fp->_flags & _IO_IS_APPENDING)
    /* On a system without a proper O_APPEND implementation,
       you would need to sys_seek(0, SEEK_END) here, but is
       not needed nor desirable for Unix- or Posix-like systems.
       Instead, just indicate that offset (before and after) is
       unpredictable. */
    fp->_offset = _IO_pos_BAD;
  else if (fp->_IO_read_end != fp->_IO_write_base) {
    off64_t new_pos = _IO_SYSSEEK(fp, fp->_IO_write_base - fp->_IO_read_end, 1);
    if (new_pos == _IO_pos_BAD)
      return 0;
    fp->_offset = new_pos;
  }
  count = _IO_SYSWRITE(fp, data, to_do);
  if (fp->_cur_column && count)
    fp->_cur_column = _IO_adjust_column(fp->_cur_column - 1, data, count) + 1;
  _IO_setg(fp, fp->_IO_buf_base, fp->_IO_buf_base, fp->_IO_buf_base);
  fp->_IO_write_base = fp->_IO_write_ptr = fp->_IO_buf_base;
  fp->_IO_write_end =
      (fp->_mode <= 0 && (fp->_flags & (_IO_LINE_BUF | _IO_UNBUFFERED))
           ? fp->_IO_buf_base
           : fp->_IO_buf_end);
  return count;
}

int _IO_do_write(FILE *fp, const char *data, size_t to_do) {
  return (to_do == 0 || (size_t)new_do_write(fp, data, to_do) == to_do) ? 0
                                                                        : EOF;
}

#define _IO_file_is_open(__fp) ((__fp)->_fileno != -1)
#define _IO_do_flush(_f)                                                       \
  _IO_do_write(_f, (_f)->_IO_write_base,                                       \
               (_f)->_IO_write_ptr - (_f)->_IO_write_base)

void _IO_switch_to_main_get_area(FILE *fp) {
  char *tmp;
  fp->_flags &= ~_IO_IN_BACKUP;
  /* Swap _IO_read_end and _IO_save_end. */
  tmp = fp->_IO_read_end;
  fp->_IO_read_end = fp->_IO_save_end;
  fp->_IO_save_end = tmp;
  /* Swap _IO_read_base and _IO_save_base. */
  tmp = fp->_IO_read_base;
  fp->_IO_read_base = fp->_IO_save_base;
  fp->_IO_save_base = tmp;
  /* Set _IO_read_ptr. */
  fp->_IO_read_ptr = fp->_IO_read_base;
}

void _IO_free_backup_area(FILE *fp) {
  if (_IO_in_backup(fp))
    _IO_switch_to_main_get_area(fp); /* Just in case. */
  free(fp->_IO_save_base);
  fp->_IO_save_base = NULL;
  fp->_IO_save_end = NULL;
  fp->_IO_backup_base = NULL;
}

void _IO_unsave_markers(FILE *fp) {
  struct _IO_marker *mark = fp->_markers;
  if (mark) {
    fp->_markers = 0;
  }

  if (_IO_have_backup(fp))
    _IO_free_backup_area(fp);
}

#define _IO_FLAGS2_NOCLOSE 32
#define _IO_FLAGS2_CLOEXEC 64
#define FREE_BUF(_B, _S) free(_B)
#define _IO_blen(fp) ((fp)->_IO_buf_end - (fp)->_IO_buf_base)
#define CLOSED_FILEBUF_FLAGS                                                   \
  (_IO_IS_FILEBUF + _IO_NO_READS + _IO_NO_WRITES + _IO_TIED_PUT_GET)

void _IO_setb(FILE *f, char *b, char *eb, int a) {
  if (f->_IO_buf_base && !(f->_flags & _IO_USER_BUF))
    FREE_BUF(f->_IO_buf_base, _IO_blen(f));
  f->_IO_buf_base = b;
  f->_IO_buf_end = eb;
  if (a)
    f->_flags &= ~_IO_USER_BUF;
  else
    f->_flags |= _IO_USER_BUF;
}

int _IO_new_file_close_it(FILE *fp) {
  int write_status;
  if (!_IO_file_is_open(fp))
    return EOF;

  if ((fp->_flags & _IO_NO_WRITES) == 0 &&
      (fp->_flags & _IO_CURRENTLY_PUTTING) != 0)
    write_status = _IO_do_flush(fp);
  else
    write_status = 0;

  _IO_unsave_markers(fp);

  int close_status =
      ((fp->_flags2 & _IO_FLAGS2_NOCLOSE) == 0 ? _IO_SYSCLOSE(fp) : 0);

  _IO_setb(fp, NULL, NULL, 0);
  _IO_setg(fp, NULL, NULL, NULL);
  _IO_setp(fp, NULL, NULL);

  //_IO_un_link((struct _IO_FILE_plus *)fp);
  fp->_flags = _IO_MAGIC | CLOSED_FILEBUF_FLAGS | _IO_LINKED;
  fp->_fileno = -1;
  fp->_offset = _IO_pos_BAD;

  return close_status ? close_status : write_status;
}

FILE *freopen(const char *filename, const char *mode, FILE *fp) {
  if (system_disabled())
    return __accmut_libc_freopen(filename, mode, fp);
  if (!system_initialized()) {
    disable_system();
    return __accmut_libc_freopen(filename, mode, fp);
  }
  FILE *result = NULL;
  // no acquire lock here
  _IO_SYNC(fp);

  if (!(fp->_flags & _IO_IS_FILEBUF))
    goto end;
  int fd = fileno(fp);
  const char *gfilename = filename; // should handle filename == NULL
  if (filename == NULL)
    goto end;
  fp->_flags2 |= _IO_FLAGS2_NOCLOSE;
  _IO_new_file_close_it(fp);

  result = _new_file_open(fp, gfilename, mode);
  fp->_flags2 &= ~_IO_FLAGS2_NOCLOSE;
  if (result != NULL) {
    result->_mode = -1;

    if (fd != -1) {
      _saved_dup3(result->_fileno, fd,
                  (result->_flags2 & _IO_FLAGS2_CLOEXEC) != 0 ? O_CLOEXEC : 0);
      _saved_close(result->_fileno);
      result->_fileno = fd;
    }
  } else {
    fclose(fp);
    if (fd != -1) {
      _saved_close(fd);
    }
  }
end:
  return result;
}

#else

#include "llvm/WinMutRuntime/filesystem/Stdio.h"
#include <errno.h>
#include <fcntl.h>
#include <libkern/OSAtomic.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>

extern pthread_once_t __sdidinit;
extern void __sinit(void);
#define FLOCKFILE(fp) flockfile(fp)
#define FUNLOCKFILE funlockfile(fp);
extern int __sflush(FILE *fp);

/* hold a buncha junk that would grow the ABI */
struct __sFILEX {
  unsigned char *up;        /* saved _p when _p is doing ungetc data */
  pthread_mutex_t fl_mutex; /* used for MT-safety */
  int orientation : 2;      /* orientation for fwide() */
  int counted : 1;          /* stream counted against STREAM_MAX */
  mbstate_t mbstate;        /* multibyte conversion state */
};

#define _up _extra->up
#define _fl_mutex _extra->fl_mutex
#define _orientation _extra->orientation
#define _mbstate _extra->mbstate
#define _counted _extra->counted
/*
 * Test whether the given stdio file has an active ungetc buffer;
 * release such a buffer, without restoring ordinary unread data.
 */
#define HASUB(fp) ((fp)->_ub._base != NULL)
#define FREEUB(fp)                                                             \
  {                                                                            \
    if ((fp)->_ub._base != (fp)->_ubuf)                                        \
      free((char *)(fp)->_ub._base);                                           \
    (fp)->_ub._base = NULL;                                                    \
  }

/*
 * test for an fgetln() buffer.
 */
#define HASLB(fp) ((fp)->_lb._base != NULL)
#define FREELB(fp)                                                             \
  {                                                                            \
    free((char *)(fp)->_lb._base);                                             \
    (fp)->_lb._base = NULL;                                                    \
  }

extern int __sflags(const char *, int *);

extern int _sread(FILE *, char *, int);
extern int _swrite(FILE *, const char *, int);
extern int _sseek(FILE *, fpos_t, int);

FILE *_stdfiles[3];

void _initstdfile() {
  if (system_disabled())
    return;
  _stdfiles[0] = stdin;
  _stdfiles[1] = stdout;
  _stdfiles[2] = stderr;
  pthread_once(&__sdidinit, __sinit);
  for (int i = 0; i < 3; ++i) {
    _stdfiles[i]->_read = _read_vfunc;
    _stdfiles[i]->_write = _write_vfunc;
    _stdfiles[i]->_seek = _seek_vfunc;
    _stdfiles[i]->_close = _close_vfunc;
  }
}

FILE *fopen(const char *filename, const char *mode) {
  FILE *fp;
  int f;
  int flags, oflags;

  if ((flags = __sflags(mode, &oflags)) == 0)
    return (NULL);
  if ((f = _saved_open(filename, oflags, DEFFILEMODE)) < 0) {
    return (NULL);
  }
  /*
   * File descriptors are a full int, but _file is only a short.
   * If we get a valid file descriptor that is greater than
   * SHRT_MAX, then the fd will get sign-extended into an
   * invalid file descriptor.  Handle this case by failing the
   * open.
   */
  if (f > SHRT_MAX) {
    _saved_close(f);
    errno = EMFILE;
    return (NULL);
  }
  fp = funopen(NULL, _read_vfunc, _write_vfunc, _seek_vfunc, _close_vfunc);
  fp->_file = f;
  fp->_flags = flags;
  fp->_cookie = fp;
  /*
   * When opening in append mode, even though we use O_APPEND,
   * we need to seek to the end so that ftell() gets the right
   * answer.  If the user then alters the seek pointer, or
   * the file extends, this will fail, but there is not much
   * we can do about this.  (We could set __SAPP and check in
   * fseek and ftell.)
   */
  if (oflags & O_APPEND)
    (void)_sseek(fp, (fpos_t)0, SEEK_END);
  return (fp);
}

FILE *fdopen(int fd, const char *mode) {
  FILE *fp;
  int flags, oflags, fdflags, tmp;

  if (fd > SHRT_MAX) {
    errno = EMFILE;
    return (NULL);
  }

  if ((flags = __sflags(mode, &oflags)) == 0)
    return (NULL);

  if ((fdflags = _saved_fcntl(fd, F_GETFL, 0)) < 0)
    return (NULL);
  tmp = fdflags & O_ACCMODE;
  if (tmp != O_RDWR && (tmp != (oflags & O_ACCMODE))) {
    errno = EINVAL;
    return (NULL);
  }

  if ((fp = funopen(NULL, _read_vfunc, _write_vfunc, _seek_vfunc,
                    _close_vfunc)) == NULL)
    return (NULL);
  fp->_flags = flags;

  if ((oflags & O_APPEND) && !(fdflags & O_APPEND))
    fp->_flags |= __SAPP;
  fp->_file = fd;
  fp->_cookie = fp;
  return (fp);
}

/*
 * Re-direct an existing, open (probably) file to some other file.
 * ANSI is written such that the original file gets closed if at
 * all possible, no matter what.
 */
FILE *freopen(const char *__restrict file, const char *__restrict mode,
              FILE *fp) {
  int f;
  int dflags, flags, oflags, sverrno, wantfd;

  if ((flags = __sflags(mode, &oflags)) == 0) {
    sverrno = errno;
    (void)fclose(fp);
    errno = sverrno;
    return (NULL);
  }

  pthread_once(&__sdidinit, __sinit);

  FLOCKFILE(fp);

  /*
   * If the filename is a NULL pointer, the caller is asking us to
   * re-open the same file with a different mode. We allow this only
   * if the modes are compatible.
   */
  if (file == NULL) {
    /* See comment below regarding freopen() of closed files. */
    if (fp->_flags == 0) {
      FUNLOCKFILE(fp);
      errno = EINVAL;
      return (NULL);
    }
    if ((dflags = _saved_fcntl(fp->_file, F_GETFL)) < 0) {
      sverrno = errno;
      fclose(fp);
      FUNLOCKFILE(fp);
      errno = sverrno;
      return (NULL);
    }
    if ((dflags & O_ACCMODE) != O_RDWR &&
        (dflags & O_ACCMODE) != (oflags & O_ACCMODE)) {
      fclose(fp);
      FUNLOCKFILE(fp);
      errno = EBADF;
      return (NULL);
    }
    if (fp->_flags & __SWR)
      (void)__sflush(fp);
    if ((oflags ^ dflags) & O_APPEND) {
      dflags &= ~O_APPEND;
      dflags |= oflags & O_APPEND;
      if (_saved_fcntl(fp->_file, F_SETFL, dflags) < 0) {
        sverrno = errno;
        fclose(fp);
        FUNLOCKFILE(fp);
        errno = sverrno;
        return (NULL);
      }
    }
    if (oflags & O_TRUNC)
      (void)ftruncate(fp->_file, (off_t)0);
    if (!(oflags & O_APPEND))
      (void)_sseek(fp, (fpos_t)0, SEEK_SET);
    f = fp->_file;
  } else {
    int isopen;

    /*
     * There are actually programs that depend on being able to "freopen"
     * descriptors that weren't originally open.  Keep this from breaking.
     * Remember whether the stream was open to begin with, and which file
     * descriptor (if any) was associated with it.  If it was attached to
     * a descriptor, defer closing it; freopen("/dev/stdin", "r", stdin)
     * should work.  This is unnecessary if it was not a Unix file.
     *
     * For UNIX03, we always close if it was open.
     */
    if (fp->_flags == 0) {
      fp->_flags = __SEOF; /* hold on to it */
      isopen = 0;
      wantfd = -1;
    } else {
      /* flush the stream; ANSI doesn't require this. */
      if (fp->_flags & __SWR)
        (void)__sflush(fp);
        /* if close is NULL, closing is a no-op, hence pointless */
#if __DARWIN_UNIX03
      if (fp->_close)
        (void)(*fp->_close)(fp->_cookie);
      isopen = 0;
      wantfd = -1;
#else  /* !__DARWIN_UNIX03 */
      isopen = fp->_close != NULL;
      if ((wantfd = fp->_file) < 0 && isopen) {
        (void)(*fp->_close)(fp->_cookie);
        isopen = 0;
      }
#endif /* __DARWIN_UNIX03 */
    }

    /* Get a new descriptor to refer to the new file. */
    f = _saved_open(file, oflags, DEFFILEMODE);
    if (f < 0 && isopen) {
      /* If out of fd's close the old one and try again. */
      if (errno == ENFILE || errno == EMFILE) {
        (void)(*fp->_close)(fp->_cookie);
        isopen = 0;
        f = _saved_open(file, oflags, DEFFILEMODE);
      }
    }
    sverrno = errno;

    /*
     * Finish closing fp.  Even if the open succeeded above, we cannot
     * keep fp->_base: it may be the wrong size.  This loses the effect
     * of any setbuffer calls, but stdio has always done this before.
     */
    if (isopen)
      (void)(*fp->_close)(fp->_cookie);

    if (f < 0) { /* did not get it after all */
      FUNLOCKFILE(fp);
      fclose(fp);
      errno = sverrno; /* restore in case _close clobbered */
      return (NULL);
    } else {

      /*
       * If reopening something that was open before on a real file, try
       * to maintain the descriptor.  Various C library routines (perror)
       * assume stderr is always fd STDERR_FILENO, even if being freopen'd.
       */
      if (wantfd >= 0 && f != wantfd) {
        if (_saved_dup2(f, wantfd) >= 0) {
          (void)_saved_close(f);
          f = wantfd;
        }
      }

      /*
       * File descriptors are a full int, but _file is only a short.
       * If we get a valid file descriptor that is greater than
       * SHRT_MAX, then the fd will get sign-extended into an
       * invalid file descriptor.  Handle this case by failing the
       * open.
       */
      if (f > SHRT_MAX) {
        FUNLOCKFILE(fp);
        fclose(fp);
        errno = EMFILE;
        return (NULL);
      }
    }
  }

  /*
   * Finish closing fp.  Even if the open succeeded above, we cannot
   * keep fp->_base: it may be the wrong size.  This loses the effect
   * of any setbuffer calls, but stdio has always done this before.
   */
  if (fp->_flags & __SMBF)
    free((char *)fp->_bf._base);
  fp->_w = 0;
  fp->_r = 0;
  fp->_p = NULL;
  fp->_bf._base = NULL;
  fp->_bf._size = 0;
  fp->_lbfsize = 0;
  if (HASUB(fp))
    FREEUB(fp);
  fp->_ub._size = 0;
  if (HASLB(fp))
    FREELB(fp);
  fp->_lb._size = 0;
  fp->_orientation = 0;
  memset(&fp->_mbstate, 0, sizeof(mbstate_t));

  fp->_flags = flags;
  fp->_file = f;
  fp->_cookie = fp;
  fp->_read = _read_vfunc;
  fp->_write = _write_vfunc;
  fp->_seek = _seek_vfunc;
  fp->_close = _close_vfunc;
  /*
   * When opening in append mode, even though we use O_APPEND,
   * we need to seek to the end so that ftell() gets the right
   * answer.  If the user then alters the seek pointer, or
   * the file extends, this will fail, but there is not much
   * we can do about this.  (We could set __SAPP and check in
   * fseek and ftell.)
   */
  if (oflags & O_APPEND)
    (void)_sseek(fp, (fpos_t)0, SEEK_END);
  FUNLOCKFILE(fp);
  return (fp);
}

#endif

void init_stdio() {
  if (system_disabled())
    return;
  inject_write(write);
  inject_open(open);
  inject_close(close);
  inject_read(read);
  inject_fcntl(fcntl);
  inject_fstat(fstat);
  inject_seek(lseek);
  inject_dup2(dup2);
#ifndef __APPLE__
  inject_dup3(dup3);
#endif
#ifndef __APPLE__
  set_injected_table();
#else
  _initstdfile();
#endif
}
extern int fflush(FILE *);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
