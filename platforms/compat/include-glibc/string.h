// glibc's <string.h>, without its C++ const overloads (Linux host builds only; compat/include-glibc
// comes first on the engine's include path there).
//
// In C++ glibc declares strchr / strstr / strpbrk / ... as const/non-const overload pairs
// (__CORRECT_ISO_CPP_STRING_H_PROTO). The engine TUs declare the plain C prototypes themselves
// (`extern "C" char* strstr(const char*, const char*)`, as bionic and Darwin spell them), which the
// const pair turns into "differ only in their return type". Hiding glibc's clang check for the
// header leaves the C prototypes, and libc++'s <string.h> adds its own preferred overloads instead,
// exactly as on macOS.
#include <features.h>
#pragma push_macro("__glibc_clang_prereq")
#undef __glibc_clang_prereq
#define __glibc_clang_prereq(maj, min) 0
#include_next <string.h>
#pragma pop_macro("__glibc_clang_prereq")
