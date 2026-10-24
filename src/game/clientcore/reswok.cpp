#include "reswok.h"

CResWOK::CResWOK()
{
    m_bLoaded = 0;
    m_nWOKSize = 0;
    m_pWOKData = 0;
}

CResWOK::~CResWOK()
{
}

void* CResWOK::GetWOKDataPtr()
{
    return m_pWOKData;
}

int CResWOK::GetWOKSize()
{
    return m_nWOKSize;
}

int CResWOK::IsLoaded()
{
    return m_bLoaded;
}

int CResWOK::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nWOKSize = 0;
    m_pWOKData = 0;
    return 1;
}

// Publishes a view into the base's raw file buffer rather than copying it.
int CResWOK::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pWOKData = m_pFileData;
    m_bLoaded = 1;
    m_nWOKSize = m_nSize;
    return 1;
}
