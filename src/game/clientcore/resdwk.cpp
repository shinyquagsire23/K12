#include "resdwk.h"

CResDWK::CResDWK()
{
    m_bAllocHeaderData = 1;
    m_bLoaded = 0;
    m_nDWKSize = 0;
    m_pDWKData = 0;
}

CResDWK::~CResDWK()
{
}

void* CResDWK::GetDWKDataPtr()
{
    return m_pDWKData;
}

int CResDWK::GetDWKSize()
{
    return m_nDWKSize;
}

int CResDWK::IsLoaded()
{
    return m_bLoaded;
}

int CResDWK::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nDWKSize = 0;
    m_pDWKData = 0;
    return 1;
}

int CResDWK::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pDWKData = m_pFileData;
    m_bLoaded = 1;
    m_nDWKSize = m_nSize;
    return 1;
}
