// High and low resolution timers. CExoTimers is the facade the engine holds in g_pExoBase; the
// internal class wraps QueryPerformanceFrequency / QueryPerformanceCounter / GetTickCount.

#ifndef CEXOTIMERS_H
#define CEXOTIMERS_H

class CExoTimersInternal {
public:
    CExoTimersInternal();
    ~CExoTimersInternal();

    unsigned int GetLowResolutionTimer();
    // Microseconds, not nanoseconds.
    unsigned long long GetHighResolutionTimer();

private:
    int m_bHighResAvailable;
    int m_bFreqBelowThreshold;
    long long m_nFrequency;
    long long m_nFreqDivMillion;
    double m_dMicrosecondsPerTick;
};

class CExoTimers {
public:
    CExoTimers();
    ~CExoTimers();

    unsigned int GetLowResolutionTimer();
    unsigned long long GetHighResolutionTimer();

private:
    CExoTimersInternal* m_pInternal;
};

#endif
