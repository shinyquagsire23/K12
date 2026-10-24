// Out-of-line placement new/delete for the host build. compat/include/__new/placement_new_delete.h
// only declares them, so engine TUs that bring their own definition still compile; every other TU
// links against these.
#include <cstddef>
void* operator new(std::size_t, void* p) noexcept { return p; }
void* operator new[](std::size_t, void* p) noexcept { return p; }
void operator delete(void*, void*) noexcept {}
void operator delete[](void*, void*) noexcept {}
