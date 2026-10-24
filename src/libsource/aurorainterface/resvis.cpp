#include "resvis.h"

CResVIS::CResVIS()
{
    m_bAllocHeaderData = 1;
    m_bLoaded = 0;
    m_nVISSize = 0;
    m_pVISData = 0;
}

CResVIS::~CResVIS()
{
}

void* CResVIS::GetVISDataPtr()
{
    return m_pVISData;
}

int CResVIS::GetVISSize()
{
    return m_nVISSize;
}

int CResVIS::IsLoaded()
{
    return m_bLoaded;
}

int CResVIS::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nVISSize = 0;
    m_pVISData = 0;
    return 1;
}

int CResVIS::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pVISData = m_pFileData;
    m_bLoaded = 1;
    m_nVISSize = m_nSize;
    return 1;
}
