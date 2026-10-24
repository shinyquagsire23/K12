#include "exocritsect.h"
#include "exocritsectinternal.h"

CExoCriticalSection::CExoCriticalSection()
{
    m_pInternal = new CExoCriticalSectionInternal();
}

CExoCriticalSection::~CExoCriticalSection()
{
    delete m_pInternal;
    m_pInternal = 0;
}

void CExoCriticalSection::EnterCriticalSection()
{
    m_pInternal->EnterCriticalSection();
}

void CExoCriticalSection::LeaveCriticalSection()
{
    m_pInternal->LeaveCriticalSection();
}
