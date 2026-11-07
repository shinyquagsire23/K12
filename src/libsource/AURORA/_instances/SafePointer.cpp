// Explicit-instantiation home for the watched-pointer pair.

#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "AURORA/aurmodel.h"
#include "aurtexman.h"

#if KOTOR1
template class SafePointer<CAurObject>;

template class SafePointer<CAurTexture>;
#else
// K2 instantiates the assignments and the destructor; a constructor's copy comes from its users.
template SafePointer<CAurObject>::~SafePointer();
template SafePointer<CAurObject>& SafePointer<CAurObject>::operator=(CAurObject*);
template SafePointer<CAurObject>& SafePointer<CAurObject>::operator=(const SafePointer<CAurObject>&);

template SafePointer<CAurTexture>::~SafePointer();
template SafePointer<CAurTexture>& SafePointer<CAurTexture>::operator=(CAurTexture*);
template SafePointer<CAurTexture>& SafePointer<CAurTexture>::operator=(const SafePointer<CAurTexture>&);
#endif
