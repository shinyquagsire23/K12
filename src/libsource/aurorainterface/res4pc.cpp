#include "k2stdint.h"
#include "res4pc.h"

#include "port/libcstring.h"
extern void ByteSwap(int* p);
extern void ByteSwap(float* p);

CRes4PC::CRes4PC()
{
    m_bAllocHeaderData = 1;
    m_p4PCData = 0;
    m_p4PCHeader = 0;
    m_bLoaded = 0;
    m_n4PCSize = 0;
}

CRes4PC::~CRes4PC()
{
}

int CRes4PC::Copy4PCData(unsigned char* pDst)
{
    if (!m_bLoaded)
        return 0;
    memcpy(pDst, m_p4PCData, m_n4PCSize);  // idiom-exempt: runtime length
    return 1;
}

int CRes4PC::Get4PCAttrib(unsigned int* pnWidth, unsigned int* pnHeight, float* pfAlphaTest, int* pnFormat)
{
    Header4PC* h = static_cast<Header4PC*>(m_p4PCHeader);
    if (h == 0)
        return 0;
    *pnWidth = h->m_nWidth;
    *pnHeight = h->m_nHeight;
    *reinterpret_cast<unsigned int*>(pfAlphaTest) = *reinterpret_cast<unsigned int*>(&h->m_fAlphaTest);
    *pnFormat = static_cast<int>(h->m_nFormat);
    return 1;
}

void* CRes4PC::Get4PCDataPtr()
{
    return m_p4PCData;
}

void* CRes4PC::Get4PCHeaderPtr()
{
    return m_p4PCHeader;
}

int CRes4PC::Get4PCSize()
{
    return m_n4PCSize;
}

int CRes4PC::IsLoaded()
{
    return m_bLoaded;
}

int CRes4PC::OnResourceFreed()
{
    m_bLoaded = 0;
    m_n4PCSize = 0;
    m_p4PCData = 0;
    m_p4PCHeader = 0;
    return 1;
}

int CRes4PC::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    if (m_p4PCHeader != 0 || m_p4PCData != 0)
        return 0;
    m_p4PCHeader = m_pFileData;
    ByteSwap(reinterpret_cast<int*>(&static_cast<Header4PC*>(m_p4PCHeader)->m_nWidth));
    ByteSwap(reinterpret_cast<int*>(&static_cast<Header4PC*>(m_p4PCHeader)->m_nHeight));
    ByteSwap(&static_cast<Header4PC*>(m_p4PCHeader)->m_fAlphaTest);
    ByteSwap(reinterpret_cast<int*>(&static_cast<Header4PC*>(m_p4PCHeader)->m_nFormat));
    m_bLoaded = 1;
    m_p4PCData = static_cast<Header4PC*>(m_pFileData) + 1;
    return 1;
}
