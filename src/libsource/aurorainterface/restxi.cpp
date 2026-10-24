#include "restxi.h"

CResTXI::CResTXI()
{
    m_bAllocHeaderData = 1;
    m_bLoaded = 0;
    m_nTXISize = 0;
    m_pTXIData = 0;
}

CResTXI::~CResTXI()
{
}

void* CResTXI::GetTXIDataPtr()
{
    return m_pTXIData;
}

int CResTXI::GetTXISize()
{
    return m_nTXISize;
}

int CResTXI::IsLoaded()
{
    return m_bLoaded;
}

int CResTXI::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nTXISize = 0;
    m_pTXIData = 0;
    return 1;
}

int CResTXI::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pTXIData = m_pFileData;
    m_bLoaded = 1;
    m_nTXISize = m_nSize;
    return 1;
}
