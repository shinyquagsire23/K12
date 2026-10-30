#pragma once
#include "k2stdint.h"

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
    // Win32's CRITICAL_SECTION record, which the port layer initializes and locks.
    struct CriticalSection {
        void*     pDebugInfo;
        int       nLockCount;
        int       nRecursionCount;
        void*     hOwningThread;
        void*     hLockSemaphore;
        uintptr_t nSpinCount;
    };
    CriticalSection m_criticalSection;
#endif
};
