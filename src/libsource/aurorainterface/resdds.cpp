#include "k2stdint.h"
#include "resdds.h"

#include "port/libcstring.h"
extern void ByteSwap(int* p);

CResDDS::CResDDS()
{
    m_bAllocHeaderData = 1;
    m_pDDSData = 0;
    m_pDDSHeader = 0;
    m_bLoaded = 0;
    m_nDDSSize = 0;
}

CResDDS::~CResDDS()
{
}

int CResDDS::CopyDDSData(unsigned char* pDst)
{
    if (!m_bLoaded)
        return 0;
    memcpy(pDst, m_pDDSData, m_nDDSSize);  // idiom-exempt: runtime length
    return 1;
}

// The out-parameters may alias m_pDDSHeader, so the member is read again rather than cached.
int CResDDS::GetDDSAttrib(unsigned int* pWidth, unsigned int* pHeight,
                          unsigned char* pBytesPerPixel, unsigned int* pDataSize, float* pAlphaTest)
{
    DDSHeader* h = static_cast<DDSHeader*>(m_pDDSHeader);
    if (h == 0)
        return 0;
    *pWidth = h->m_width;
    *pHeight = h->m_height;
    *pBytesPerPixel = static_cast<unsigned char>(h->m_nBytesPerPixel);
    h = static_cast<DDSHeader*>(m_pDDSHeader);
    *pDataSize = h->m_nDataSize;
    *reinterpret_cast<unsigned int*>(pAlphaTest) = h->m_fAlphaTest;
    return 1;
}

void* CResDDS::GetDDSDataPtr()
{
    return m_pDDSData;
}

void* CResDDS::GetDDSHeaderPtr()
{
    return m_pDDSHeader;
}

int CResDDS::GetDDSSize()
{
    return m_nDDSSize;
}

int CResDDS::IsLoaded()
{
    return m_bLoaded;
}

int CResDDS::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nDDSSize = 0;
    m_pDDSData = 0;
    m_pDDSHeader = 0;
    return 1;
}

// Publishes a view into the base buffer, byte-swapping the first four header dwords in place.
int CResDDS::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    if (m_pDDSHeader != 0 || m_pDDSData != 0)
        return 0;
    m_pDDSHeader = m_pFileData;
    ByteSwap(reinterpret_cast<int*>(&static_cast<DDSHeader*>(m_pDDSHeader)->m_width));
    ByteSwap(reinterpret_cast<int*>(&static_cast<DDSHeader*>(m_pDDSHeader)->m_height));
    ByteSwap(reinterpret_cast<int*>(&static_cast<DDSHeader*>(m_pDDSHeader)->m_nBytesPerPixel));
    ByteSwap(reinterpret_cast<int*>(&static_cast<DDSHeader*>(m_pDDSHeader)->m_nDataSize));
    m_bLoaded = 1;
    m_pDDSData = static_cast<DDSHeader*>(m_pFileData) + 1;
    return 1;
}
