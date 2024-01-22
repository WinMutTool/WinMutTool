#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <type_traits>

template <bool v> struct bool_constant { static const bool value = v; };

struct true_type : bool_constant<true> {};

struct false_type : bool_constant<false> {};

template <class...> struct conjunction : true_type {};
template <class B1> struct conjunction<B1> : B1 {};
template <class B1, class... Bn>
struct conjunction<B1, Bn...>
    : std::conditional<bool(B1::value), conjunction<Bn...>, B1>::type {};

template <class B> struct negation : bool_constant<!bool(B::value)> {};

template <typename _U, typename _T>
struct accmut_same : public std::false_type {};

#define DECLARE_CONSIDER(a)                                                    \
  template <> struct accmut_same<a, a> : public std::true_type {}
#define DECLARE_SAME(a, b)                                                     \
  template <> struct accmut_same<a, b> : public std::true_type {};             \
  template <> struct accmut_same<b, a> : public std::true_type {}

DECLARE_CONSIDER(uint8_t);
DECLARE_CONSIDER(uint16_t);
DECLARE_CONSIDER(uint32_t);
DECLARE_CONSIDER(uint64_t);
DECLARE_CONSIDER(int8_t);
DECLARE_CONSIDER(int16_t);
DECLARE_CONSIDER(int32_t);
DECLARE_CONSIDER(int64_t);
DECLARE_CONSIDER(void);
DECLARE_CONSIDER(char);
#ifdef __APPLE__
DECLARE_CONSIDER(unsigned long);
DECLARE_CONSIDER(long);
#endif
DECLARE_CONSIDER(struct stat);
DECLARE_CONSIDER(va_list);
DECLARE_CONSIDER(FILE);

// DECLARE_SAME(FILE, ACCMUTV2_FILE);

template <typename _T, typename _U>
struct accmut_same<_T *, _U *> : public accmut_same<_T, _U> {};

template <typename _T, typename _U>
struct accmut_same<const _T, const _U> : public accmut_same<_T, _U> {};

template <typename _T, typename _U>
struct accmut_same<_T(), _U()> : public accmut_same<_T, _U> {};

template <typename _T, typename _U>
struct accmut_same<_T(...), _U(...)> : public accmut_same<_T, _U> {};

template <typename _T, typename _T1, typename _U, typename _U1>
struct accmut_same<_T(_T1), _U(_U1)>
    : conjunction<accmut_same<_T, _U>, accmut_same<_T1, _U1>> {};

template <typename _T, typename _T1, typename _U, typename _U1>
struct accmut_same<_T(_T1, ...), _U(_U1, ...)> : accmut_same<_T(_T1), _U(_U1)> {
};

template <typename _T, typename _T1, typename _T2, typename _U, typename _U1,
          typename _U2>
struct accmut_same<_T(_T1, _T2), _U(_U1, _U2)>
    : conjunction<accmut_same<_T(_T1), _U(_U1)>, accmut_same<_T2, _U2>> {};

template <typename _T, typename _T1, typename _T2, typename _U, typename _U1,
          typename _U2>
struct accmut_same<_T(_T1, _T2, ...), _U(_U1, _U2, ...)>
    : accmut_same<_T(_T1, _T2), _U(_U1, _U2)> {};

template <typename _T, typename _T1, typename _T2, typename _T3, typename _U,
          typename _U1, typename _U2, typename _U3>
struct accmut_same<_T(_T1, _T2, _T3), _U(_U1, _U2, _U3)>
    : conjunction<accmut_same<_T(_T1, _T2), _U(_U1, _U2)>,
                  accmut_same<_T3, _U3>> {};

template <typename _T, typename _T1, typename _T2, typename _T3, typename _U,
          typename _U1, typename _U2, typename _U3>
struct accmut_same<_T(_T1, _T2, _T3, ...), _U(_U1, _U2, _U3, ...)>
    : accmut_same<_T(_T1, _T2, _T3), _U(_U1, _U2, _U3)> {};

template <typename _T, typename _T1, typename _T2, typename _T3, typename _T4,
          typename _U, typename _U1, typename _U2, typename _U3, typename _U4>
struct accmut_same<_T(_T1, _T2, _T3, _T4), _U(_U1, _U2, _U3, _U4)>
    : conjunction<accmut_same<_T(_T1, _T2, _T3), _U(_U1, _U2, _U3)>,
                  accmut_same<_T4, _U4>> {};

template <typename _T, typename _T1, typename _T2, typename _T3, typename _T4,
          typename _U, typename _U1, typename _U2, typename _U3, typename _U4>
struct accmut_same<_T(_T1, _T2, _T3, _T4, ...), _U(_U1, _U2, _U3, _U4, ...)>
    : accmut_same<_T(_T1, _T2, _T3, _T4), _U(_U1, _U2, _U3, _U4)> {};
/*
static_assert(accmut_same<int(const char), int(const char)>::value, "open");
static_assert(accmut_same<FILE ****, ACCMUTV2_FILE ****>::value, "test");
static_assert(!accmut_same<FILE ***, ACCMUTV2_FILE ****>::value, "test");
static_assert(!accmut_same<const FILE ****, ACCMUTV2_FILE ****>::value, "test");
static_assert(accmut_same<FILE *const, ACCMUTV2_FILE *const>::value, "test");
static_assert(accmut_same<FILE *(), ACCMUTV2_FILE *()>::value, "test");
static_assert(accmut_same<FILE *(...), ACCMUTV2_FILE *(...)>::value, "test");
static_assert(!accmut_same<int *(), ACCMUTV2_FILE *()>::value, "test");
static_assert(!accmut_same<int *(...), ACCMUTV2_FILE *(...)>::value, "test");
#define TEST_TYPE(func)                                                        \
  static_assert(                                                               \
      accmut_same<decltype(func), decltype(__accmutv2__##func)>::value, #func)

TEST_TYPE(open);
TEST_TYPE(creat);
TEST_TYPE(close);
TEST_TYPE(lseek);
TEST_TYPE(read);
TEST_TYPE(write);
TEST_TYPE(fopen);
TEST_TYPE(fclose);
TEST_TYPE(freopen);
TEST_TYPE(feof);
TEST_TYPE(fileno);
TEST_TYPE(fflush);
TEST_TYPE(ferror);
TEST_TYPE(fseek);
TEST_TYPE(ftell);
TEST_TYPE(fseeko);
TEST_TYPE(ftello);
TEST_TYPE(rewind);
TEST_TYPE(fread);
TEST_TYPE(fgets);
TEST_TYPE(fgetc);
TEST_TYPE(getc);
TEST_TYPE(ungetc);
// TEST_TYPE(vfscanf);
TEST_TYPE(fscanf);
TEST_TYPE(scanf);
TEST_TYPE(fputc);
TEST_TYPE(fputs);
TEST_TYPE(fwrite);
// TEST_TYPE(vfprintf);
TEST_TYPE(fprintf);
TEST_TYPE(printf);
TEST_TYPE(perror);

// seems that va_list cannot be verified

#define TEST_SAME(a, b)                                                        \
  static_assert(accmut_same<decltype(a), decltype(b)>::value, #a "/" #b)
*/
/*
TEST_SAME(access, fs_access);
TEST_SAME(chmod, fs_chmod);
TEST_SAME(chown, fs_chown);
TEST_SAME(mkdir, fs_mkdir);
TEST_SAME(stat, fs_stat);
TEST_SAME(lstat, fs_lstat);
TEST_SAME(realpath, fs_realpath);
*/
