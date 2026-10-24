#include "exobase.h"
#include "exorandinternal.h"

CExoRand::CExoRand()
{
    m_pInternal = new CExoRandInternal();
}

CExoRand::~CExoRand()
{
    if (m_pInternal)
    {
        delete m_pInternal;
        m_pInternal = 0;
    }
}

int CExoRand::Rand()
{
    return m_pInternal->Rand();
}

CExoString CExoRand::GetString(unsigned short nID)
{
    return m_pInternal->GetString(nID);
}

void CExoRand::SignalEvent()
{
    m_pInternal->SignalEvent();
}

void CExoRand::SignalEvent(unsigned int nEvent)
{
    m_pInternal->SignalEvent(nEvent);
}
