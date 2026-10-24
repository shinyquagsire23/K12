#pragma once

// Win32 CRITICAL_SECTION shim behind CExoCriticalSection, reachable only through it.

class CExoCriticalSectionInternal {
public:
    CExoCriticalSectionInternal();
    ~CExoCriticalSectionInternal();
    void EnterCriticalSection();
    void LeaveCriticalSection();
private:
#if KOTOR1
    struct SDL_mutex* m_pMutex;
#else
    unsigned char m_criticalSection[0x28];
#endif
};
