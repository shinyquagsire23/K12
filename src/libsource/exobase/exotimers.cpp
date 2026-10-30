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

uint64_ll_t CExoTimers::GetHighResolutionTimer()
{
    return m_pInternal->GetHighResolutionTimer();
}
