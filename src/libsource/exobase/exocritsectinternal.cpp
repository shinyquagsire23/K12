#include "exocritsectinternal.h"
#include "port/sdlapi.h"

#if KOTOR1
#else
#include "port/win32api.h"
#endif

CExoCriticalSectionInternal::CExoCriticalSectionInternal()
{
#if KOTOR1
    m_pMutex = SDL_CreateMutex();
#else
    InitializeCriticalSection(&m_criticalSection);
#endif
}

CExoCriticalSectionInternal::~CExoCriticalSectionInternal()
{
#if KOTOR1
    SDL_DestroyMutex(m_pMutex);
#else
    DeleteCriticalSection(&m_criticalSection);
#endif
}

void CExoCriticalSectionInternal::EnterCriticalSection()
{
#if KOTOR1
    SDL_LockMutex(m_pMutex);
#else
    ::EnterCriticalSection(&m_criticalSection);
#endif
}

void CExoCriticalSectionInternal::LeaveCriticalSection()
{
#if KOTOR1
    SDL_UnlockMutex(m_pMutex);
#else
    ::LeaveCriticalSection(&m_criticalSection);
#endif
}
