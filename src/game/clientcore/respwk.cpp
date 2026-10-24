#include "respwk.h"

CResPWK::CResPWK()
{
    m_bAllocHeaderData = 1;
    m_bLoaded = 0;
    m_nPWKSize = 0;
    m_pPWKData = 0;
}

CResPWK::~CResPWK()
{
}

void* CResPWK::GetPWKDataPtr()
{
    return m_pPWKData;
}

int CResPWK::GetPWKSize()
{
    return m_nPWKSize;
}

int CResPWK::IsLoaded()
{
    return m_bLoaded;
}

int CResPWK::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nPWKSize = 0;
    m_pPWKData = 0;
    return 1;
}

int CResPWK::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pPWKData = m_pFileData;
    m_bLoaded = 1;
    m_nPWKSize = m_nSize;
    return 1;
}
