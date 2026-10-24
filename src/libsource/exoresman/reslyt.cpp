#include "reslyt.h"

CResLYT::CResLYT()
{
    m_bLoaded = 0;
    m_nLYTSize = 0;
    m_pLYTData = 0;
}

CResLYT::~CResLYT()
{
}

void* CResLYT::GetLYTDataPtr()
{
    return m_pLYTData;
}

int CResLYT::GetLYTSize()
{
    return m_nLYTSize;
}

int CResLYT::IsLoaded()
{
    return m_bLoaded;
}

int CResLYT::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nLYTSize = 0;
    m_pLYTData = 0;
    return 1;
}

int CResLYT::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pLYTData = m_pFileData;
    m_bLoaded = 1;
    m_nLYTSize = m_nSize;
    return 1;
}
