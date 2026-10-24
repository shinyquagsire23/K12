#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "AURORA/aurmodel.h"
#include "aurtexman.h"

template void SafePointerList<CAurObject>::AddPointer(SafePointer<CAurObject>*);

template class SafePointerList<CAurTexture>;
