// What the engine imports that iOS does not have.
#include <stdio.h>

// AndroidJNI.cpp's `rm -rf` of a cloud-save folder (ios/ios_nosystem.h). The Java side that calls
// it does not exist on iOS; iOS cannot spawn processes either.
extern "C" int k2_system(const char* pszCommand)
{
    fprintf(stderr, "k2: system(\"%s\") is not available on iOS\n", pszCommand ? pszCommand : "");
    return -1;
}
