#include "resmdl.h"

CResMDL::CResMDL()
{
    m_bAllocHeaderData = 1;
    m_bLoaded = 0;
    m_nMDLSize = 0;
    m_pMDLData = 0;
}

CResMDL::~CResMDL()
{
}

void* CResMDL::GetMDLDataPtr()
{
    return m_pMDLData;
}

int CResMDL::GetMDLSize()
{
    return m_nMDLSize;
}

int CResMDL::IsLoaded()
{
    return m_bLoaded;
}

int CResMDL::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nMDLSize = 0;
    m_pMDLData = 0;
    return 1;
}

int CResMDL::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pMDLData = m_pFileData;
    m_bLoaded = 1;
    m_nMDLSize = m_nSize;
    return 1;
}
