#include "exotimersinternal.h"
#include "port/sdlapi.h"

#if KOTOR1
#endif
#include "port/win32api.h"

CExoTimersInternal::CExoTimersInternal()
{
#if KOTOR1
    // K1 reads SDL's performance counter, which is always present.
    long long freq = SDL_GetPerformanceFrequency();
    m_bHighResAvailable = 1;
    m_nFrequency = freq;
    m_bFreqBelowThreshold = 0;
    if (static_cast<unsigned long long>(freq) <= 199999999ULL)
        m_bFreqBelowThreshold = 1;
    else
        m_nFreqDivMillion = freq / 1000000LL;
    m_dMicrosecondsPerTick = 1.0 / (static_cast<double>(freq) / 1000000.0);
#else
    long long freq;
    m_bHighResAvailable = QueryPerformanceFrequency(&freq);
    if (m_bHighResAvailable != 1)
        return;

    m_nFrequency = freq;
    m_bFreqBelowThreshold = 0;
    if (static_cast<unsigned long long>(freq) <= 199999999ULL)
    {
        m_bFreqBelowThreshold = 1;
        m_dMicrosecondsPerTick = 1.0 / (static_cast<double>(freq) / 1000000.0);
    }
    else
    {
        m_dMicrosecondsPerTick = 1.0 / (static_cast<double>(freq) / 1000000.0);
        m_nFreqDivMillion = freq / 1000000LL;
    }
#endif
}

CExoTimersInternal::~CExoTimersInternal()
{
}

unsigned int CExoTimersInternal::GetLowResolutionTimer()
{
    return GetTickCount();
}

unsigned long long CExoTimersInternal::GetHighResolutionTimer()
{
    if (m_bHighResAvailable != 1)
        return static_cast<unsigned long long>(GetTickCount()) * 1000;

#if KOTOR1
    long long counter = SDL_GetPerformanceCounter();
#else
    long long counter;
    QueryPerformanceCounter(&counter);
#endif
    return static_cast<long long>(static_cast<double>(counter) * m_dMicrosecondsPerTick);
}
