
// Explicit-instantiation home for the watched-pointer pair.

#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "AURORA/aurmodel.h"
#include "aurtexman.h"

template class SafePointer<CAurObject>;

template class SafePointer<CAurTexture>;
