#include "resmdx.h"

CResMDX::~CResMDX()
{
}

// Sets the base flag, since .mdx resources carry an encapsulated header.
CResMDX::CResMDX()
{
    m_bAllocHeaderData = 1;
    m_bLoaded = 0;
    m_nMDXSize = 0;
    m_pMDXData = 0;
}

// Publishes the whole loaded file as the .mdx view; there is no header to parse, so the only check
// is that the resource manager delivered a buffer.
int CResMDX::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;

    if (!m_bLoaded)
    {
        m_pMDXData = m_pFileData;
        m_bLoaded = 1;
        m_nMDXSize = m_nSize;
    }

    return 1;
}

int CResMDX::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nMDXSize = 0;
    m_pMDXData = 0;
    return 1;
}

void* CResMDX::GetMDXDataPtr()
{
    return m_pMDXData;
}

int CResMDX::GetMDXSize()
{
    return m_nMDXSize;
}

int CResMDX::IsLoaded()
{
    return m_bLoaded;
}
