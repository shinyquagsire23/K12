#include "restpc.h"

// The TPC header, and K1's format bit for a packed format whose data size covers the whole mip
// chain.
enum {
    TPC_HEADER_SIZE = 128,
    TPC_FORMAT_PACKED_CHAIN = 0x10,
};

CResTPC::CResTPC()
{
    m_bAllocTrailingData = 1;
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_nUncompressedSize = 0;
    m_nCompressedSize = 0;
    m_nTXISize = 0;
    m_pTPCData = 0;
    m_pTPCHeader = 0;
    m_pTPCTXI = 0;
}

CResTPC::~CResTPC()
{
}

int CResTPC::GetTPCAttrib(int* pWidth, int* pHeight, char* pFormat, int* pnCompressedSize, int* pnUncompressedSize,
                          int* pTXISize, float* pAlphaTest, char* pMipCount)
{
    TPCHeader* h = static_cast<TPCHeader*>(m_pTPCHeader);
    if (h == 0)
        return 0;
    *pWidth = h->m_width;
    *pHeight = h->m_height;
    *reinterpret_cast<unsigned int*>(pAlphaTest) = *reinterpret_cast<unsigned int*>(&h->m_fAlphaTest);
    *pMipCount = static_cast<char>(h->m_nMipCount);
    *pnCompressedSize = m_nCompressedSize;
    *pnUncompressedSize = m_nUncompressedSize;
    *pTXISize = m_nTXISize;
    unsigned char fmt = static_cast<TPCHeader*>(m_pTPCHeader)->m_formatBits;
    if (fmt & 1)
        *pFormat = 1;
    else if (fmt & 2)
        *pFormat = 3;
    else if (fmt & 4)
        *pFormat = 4;
#if KOTOR1
    // K1: format bit 4 selects 5, or 6 together with bit 2
    fmt = static_cast<TPCHeader*>(m_pTPCHeader)->m_formatBits;
    if (fmt & TPC_FORMAT_PACKED_CHAIN)
        *pFormat = (fmt & 4) ? 6 : 5;
#endif
    return 1;
}

void* CResTPC::GetTPCDataPtr()
{
    return m_pTPCData;
}

void* CResTPC::GetTPCHeaderPtr()
{
    return m_pTPCHeader;
}

void* CResTPC::GetTPCTXIPtr()
{
    return m_pTPCTXI;
}

int CResTPC::GetTPCSize()
{
    return m_bCompressed ? m_nCompressedSize : m_nUncompressedSize;
}

int CResTPC::GetTPCTXISize()
{
    return m_nTXISize;
}

int CResTPC::IsCompressed()
{
    return m_bCompressed;
}

int CResTPC::IsLoaded()
{
    return m_bLoaded;
}

int CResTPC::OnResourceFreed()
{
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_nUncompressedSize = 0;
    m_nCompressedSize = 0;
    m_nTXISize = 0;
    m_pTPCTXI = 0;
    m_pTPCData = 0;
    m_pTPCHeader = 0;
    return 1;
}

// Publishes the parsed texture: the file buffer is the header, pixel data starts past it, and the
// remaining bytes become the TXI view. The pixel span comes from the header's own data size when
// compressed, otherwise from a walk over the mip chain.
#if KOTOR1
void ByteSwap(unsigned int* p);
void ByteSwap(float* p);
void ByteSwap(unsigned short* p);
#endif
int CResTPC::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    if (m_pTPCHeader != 0 || m_pTPCData != 0)
        return 0;

#if KOTOR1  // K1 byte-swaps the header's four leading fields first
    m_pTPCHeader = m_pFileData;
    ByteSwap(&static_cast<TPCHeader*>(m_pTPCHeader)->m_nDataSize);
    ByteSwap(&static_cast<TPCHeader*>(m_pTPCHeader)->m_fAlphaTest);
    ByteSwap(&static_cast<TPCHeader*>(m_pTPCHeader)->m_width);
    ByteSwap(&static_cast<TPCHeader*>(m_pTPCHeader)->m_height);
    TPCHeader* pHeader = static_cast<TPCHeader*>(m_pTPCHeader);
    m_pTPCData = static_cast<TPCHeader*>(m_pFileData) + 1;
#else
    TPCHeader* pHeader = static_cast<TPCHeader*>(m_pFileData);
    m_pTPCData = pHeader + 1;
    m_pTPCHeader = m_pFileData;
#endif

    int nBytesPerPixel;
    if (pHeader->m_formatBits & 1)
        nBytesPerPixel = 1;
    else
        nBytesPerPixel = (pHeader->m_formatBits & 2) ? 3 : 4;

    if (pHeader->m_nDataSize == 0) {
        int nWidth = pHeader->m_width;
        int nHeight = pHeader->m_height;
        m_nUncompressedSize = m_nCompressedSize = 0;
        unsigned int nTotal = 0;
        for (int i = 0; i < pHeader->m_nMipCount; i++) {
            nTotal += nWidth * nBytesPerPixel * nHeight;
            nWidth >>= 1;
            nHeight >>= 1;
        }
        m_nUncompressedSize = nTotal;
        m_pTPCTXI = static_cast<unsigned char*>(m_pTPCData) + nTotal;
        m_nTXISize = m_nSize - nTotal - TPC_HEADER_SIZE;
    } else {
        m_nUncompressedSize = 0;
        m_nCompressedSize = pHeader->m_nDataSize;
        m_bCompressed = 1;
        int nWidth = pHeader->m_width;
        int nHeight = pHeader->m_height;
        unsigned int nMipCount = pHeader->m_nMipCount;
        int nRatio = nHeight / nWidth;
        int nFaceCount = (nRatio == 6) ? 6 : 1;
        int nBlockBytes = (nBytesPerPixel == 4) ? 16 : 8;
        int nMipWidth = nWidth;
        int nMipHeight = (nRatio == 6) ? nHeight / 6 : nHeight;
        int nTotal = pHeader->m_nDataSize * nFaceCount;
        for (unsigned int i = 1; i < nMipCount; i++) {
            nMipWidth >>= 1;
            nMipHeight >>= 1;
            nTotal += (nMipWidth + 3) / 4 * ((nMipHeight + 3) / 4) * nBlockBytes * nFaceCount;
        }
#if KOTOR1  // K1: format bit 4 means the data size already covers the whole chain
        if (pHeader->m_formatBits & TPC_FORMAT_PACKED_CHAIN)
            nTotal = pHeader->m_nDataSize;
#endif
        m_pTPCTXI = static_cast<unsigned char*>(m_pTPCData) + nTotal;
        m_nTXISize = m_nSize - nTotal - TPC_HEADER_SIZE;
    }

    m_bLoaded = 1;
    return 1;
}
