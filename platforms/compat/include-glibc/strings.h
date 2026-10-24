// glibc's <strings.h> without its C++ index/rindex const overloads: see string.h beside this.
#include <features.h>
#pragma push_macro("__glibc_clang_prereq")
#undef __glibc_clang_prereq
#define __glibc_clang_prereq(maj, min) 0
#include_next <strings.h>
#pragma pop_macro("__glibc_clang_prereq")
