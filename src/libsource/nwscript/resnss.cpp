#include "resnss.h"

CResNSS::CResNSS()
{
    m_bLoaded = 0;
    m_nNSSSize = 0;
    m_pNSSData = 0;
}

CResNSS::~CResNSS()
{
}

void* CResNSS::GetNSSDataPtr()
{
    return m_pNSSData;
}

int CResNSS::GetNSSSize()
{
    return m_nNSSSize;
}

int CResNSS::IsLoaded()
{
    return m_bLoaded;
}

int CResNSS::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nNSSSize = 0;
    m_pNSSData = 0;
    return 1;
}

int CResNSS::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pNSSData = m_pFileData;
    m_bLoaded = 1;
    m_nNSSSize = m_nSize;
    return 1;
}
