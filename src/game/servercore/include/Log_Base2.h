#pragma once

#include "port/libcmath.h"

// log2(n), rounded to the nearest int. Used for the damage-flag-bits to node sub-type mapping.
inline int Log_Base2(int n)
{
    static int nRetVal;
    nRetVal = static_cast<int>(log10f(static_cast<float>(n)) * 3.321928f + 0.5f);
    return nRetVal;
}
