#pragma once
#include "k2stdint.h"

// K2's CExoString::GetLength, AsINT and AsFLOAT are inline header functions (the armv7 image calls
// them, the arm64 image has no symbol for them). IsEmpty is spelled the same way; neither K2 image
// names it. K1 keeps all four out of line in exostring.cpp. A TU includes this after its system headers, since
// port/libcstring.h declares strlen with bionic's own exception specification left out.
#if !KOTOR1
#include "exostring.h"
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"

inline int CExoString::GetLength() const
{
    return m_sString != 0 ? static_cast<int>(strlen(m_sString)) : 0;
}

inline int CExoString::AsINT() const
{
    return m_sString != 0 ? atoi(m_sString) : 0;
}

inline float CExoString::AsFLOAT() const
{
    return m_sString != 0 ? static_cast<float>(atof(m_sString)) : 0.0f;
}

inline int CExoString::IsEmpty() const
{
    return m_sString == 0 || m_sString[0] == '\0';
}
#endif
