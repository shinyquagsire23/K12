
// The ASL GL shim's one free entry point.

#include "aslgl.h"

// Writes slot 0 of the vec4 the vertex shader reads.
void SetBumpedOutShellIos(float f)
{
    s_vertexShaderParams[0] = f;
}
