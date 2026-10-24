#include "reslip.h"

CResLIP::CResLIP()
{
    m_bLoaded = 0;
    m_nLIPSize = 0;
    m_pLIPData = 0;
}

CResLIP::~CResLIP()
{
}

void* CResLIP::GetLIPDataPtr()
{
    return m_pLIPData;
}

int CResLIP::GetLIPSize()
{
    return m_nLIPSize;
}

int CResLIP::IsLoaded()
{
    return m_bLoaded;
}

int CResLIP::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nLIPSize = 0;
    m_pLIPData = 0;
    return 1;
}

int CResLIP::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pLIPData = m_pFileData;
    m_bLoaded = 1;
    m_nLIPSize = m_nSize;
    return 1;
}
