#pragma once

// The render interface's own capability probes, the two cached queries that are not part of the
// extension-word table in aurglext.cpp.

#include "aurglext.h"

extern int WGL_ARB_MULTISAMPLE_BIT;

int AurPixelShadersAvailable();
int AurIsAAliasingAvailable();
