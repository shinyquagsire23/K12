// Force-included into src/port/android/AndroidJNI.cpp on iOS (the root CMakeLists.txt). iOS's SDK
// marks system() unavailable, and the Android cloud-save callbacks there spell `rm -rf` through it.
// <stdlib.h> is read first, so the SDK's declaration keeps its own name; every later `system` in the
// TU is the stub in ios/ios_compat.cpp.
#include <stdlib.h>
#ifdef __cplusplus
extern "C"
#endif
int k2_system(const char* pszCommand);
#define system k2_system
