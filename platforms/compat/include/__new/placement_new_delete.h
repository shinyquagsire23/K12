// Host-build shadow of libc++'s <__new/placement_new_delete.h> (found first via -I).
//
// Engine TUs define placement new themselves, because the NDK build never pulled in <new>. libc++'s
// own definitions carry an abi_tag, which a redeclaration cannot add, so any engine TU that defines
// placement new and then includes a libc++ header fails to compile. Here the four forms are only
// declared, without the abi_tag; compat/placement_new.cpp provides the definitions for every TU
// that does not bring its own.
#ifndef _LIBCPP___NEW_PLACEMENT_NEW_DELETE_H
#define _LIBCPP___NEW_PLACEMENT_NEW_DELETE_H
#include <__config>
#include <__cstddef/size_t.h>
#pragma GCC system_header
void* operator new(std::size_t, void* __p) _NOEXCEPT;
void* operator new[](std::size_t, void* __p) _NOEXCEPT;
void operator delete(void*, void*) _NOEXCEPT;
void operator delete[](void*, void*) _NOEXCEPT;
#endif
