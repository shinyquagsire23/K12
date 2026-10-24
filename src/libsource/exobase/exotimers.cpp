#include "exotimersinternal.h"

CExoTimers::CExoTimers()
{
    m_pInternal = new CExoTimersInternal();
}

CExoTimers::~CExoTimers()
{
    CExoTimersInternal* pInternal = m_pInternal;
    if (pInternal != 0)
    {
        pInternal->~CExoTimersInternal();
        operator delete(pInternal);
    }
}

unsigned int CExoTimers::GetLowResolutionTimer()
{
    return m_pInternal->GetLowResolutionTimer();
}

unsigned long long CExoTimers::GetHighResolutionTimer()
{
    return m_pInternal->GetHighResolutionTimer();
}
