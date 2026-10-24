
// The render interface's two capability probes. See aurrenderinterfacecore.h.

#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurrenderinterfacecore.h"
#include "port/aslglapi.h"


int envmapon;
int tex1on;

int AurPixelShadersAvailable()
{
    static int returnval = -1;
    if (returnval == -1) {
        if (AurNV1xLevelShadersAvailable()) {
            if (AurATIFragmentShaderAvailable()) {
                returnval = 1;
                return returnval;
            }
            ASLgl::glGetIntegerv(queryTranslationTable[1], &returnval);
            if (returnval > 7) {
                returnval = 1;
                return returnval;
            }
        }
        returnval = 0;
        returnval = AurATIFragmentShaderAvailable();
    }
    return returnval;
}

int AurIsAAliasingAvailable()
{
#if defined(K2_QOL_IMPROVEMENTS) || defined(K2_QOL_K1_ANTIALIAS)
    // Anti-aliasing supersamples the scene buffer, which needs no multisample extension, and the
    // port's GL_EXTENSIONS never lists GL_ARB_multisample.
    return 1;
#else
    static int returnval = -1;
    if (returnval == -1)
        returnval = (WGL_ARB_MULTISAMPLE_BIT & existingextensions) == WGL_ARB_MULTISAMPLE_BIT;
    return returnval;
#endif
}
