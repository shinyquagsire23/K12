#include "nwanimbase.h"

CSWAnimBase::CSWAnimBase()
    : m_fParam0(0.0f),
      m_fParam1(0.0f)
{
}

CSWAnimBase::~CSWAnimBase()
{
}

int CSWAnimBase::Get(int nIndex, float* pfValue)
{
    switch (nIndex) {
    case 0:
        *pfValue = m_fParam0;
        return 1;
    case 1:
        *pfValue = m_fParam1;
        return 1;
    case 2:
        *pfValue = m_fParam2;
        return 1;
    case 3:
        *pfValue = m_fParam3;
        return 1;
    }
    return 0;
}

int CSWAnimBase::Set(int nIndex, float fValue)
{
    switch (nIndex) {
    case 0:
        m_fParam0 = fValue;
        return 1;
    case 1:
        m_fParam1 = fValue;
        return 1;
    case 2:
        m_fParam2 = fValue;
        return 1;
    case 3:
        m_fParam3 = fValue;
        return 1;
    }
    return 0;
}
