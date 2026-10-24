#include "restxb.h"

// The TXB header, the boundary each cube face's data is padded to, and a DXT block's bytes.
enum {
    TXB_HEADER_SIZE = 128,
    TXB_FACE_ALIGN = 128,
    DXT1_BLOCK_BYTES = 8,
    DXT5_BLOCK_BYTES = 16,
};

CResTXB::CResTXB()
{
    m_bAllocTrailingData = 1;
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_nUncompressedSize = 0;
    m_nCompressedSize = 0;
    m_nTXISize = 0;
    m_pTXBData = 0;
    m_pTXBHeader = 0;
    m_pTXBTXI = 0;
}

CResTXB::~CResTXB()
{
}

// Unpacks the header: 16-bit dimensions, the alpha-test value and mip count copied raw, and the
// pixel format decoded from the header's format bits (left unwritten when no bit is set).
int CResTXB::GetTXBAttrib(int* pWidth, int* pHeight, char* pFormat, int* pnCompressedSize, int* pnUncompressedSize,
                          int* pTXISize, float* pAlphaTest, char* pMipCount)
{
    TXBHeader* h = static_cast<TXBHeader*>(m_pTXBHeader);
    if (h == 0)
        return 0;
    *pWidth = h->m_width;
    *pHeight = h->m_height;
    *reinterpret_cast<unsigned int*>(pAlphaTest) = *reinterpret_cast<unsigned int*>(&h->m_fAlphaTest);
    *pMipCount = static_cast<char>(h->m_nMipCount);
    *pnCompressedSize = m_nCompressedSize;
    *pnUncompressedSize = m_nUncompressedSize;
    *pTXISize = m_nTXISize;
    unsigned char fmt = static_cast<TXBHeader*>(m_pTXBHeader)->m_formatBits;
    if (fmt & 1)
        *pFormat = 1;
    else if (fmt & 2)
        *pFormat = 3;
    else if (fmt & 4)
        *pFormat = 4;
    return 1;
}

void* CResTXB::GetTXBDataPtr()
{
    return m_pTXBData;
}

void* CResTXB::GetTXBHeaderPtr()
{
    return m_pTXBHeader;
}

void* CResTXB::GetTXBTXIPtr()
{
    return m_pTXBTXI;
}

int CResTXB::GetTXBSize()
{
    return m_bCompressed ? m_nCompressedSize : m_nUncompressedSize;
}

int CResTXB::GetTXBTXISize()
{
    return m_nTXISize;
}

int CResTXB::IsCompressed()
{
    return m_bCompressed;
}

int CResTXB::IsLoaded()
{
    return m_bLoaded;
}

int CResTXB::OnResourceFreed()
{
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_nUncompressedSize = 0;
    m_nCompressedSize = 0;
    m_nTXISize = 0;
    m_pTXBTXI = 0;
    m_pTXBData = 0;
    m_pTXBHeader = 0;
    return 1;
}

// Parses the header, then walks the mip chain to total the pixel-data size, once for the
// uncompressed case and once for the DXT block-compressed case. A cubemap (height six times the
// width) is totalled per face and multiplied by six; the total is rounded up to a 128-byte
// boundary.
int CResTXB::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    if (m_pTXBHeader != 0 || m_pTXBData != 0)
        return 0;

    TXBHeader* h = static_cast<TXBHeader*>(m_pFileData);
    m_pTXBData = reinterpret_cast<unsigned char*>(static_cast<TXBHeader*>(m_pFileData) + 1);
    m_pTXBHeader = h;

    int nFormat;
    if (h->m_formatBits & 1)
        nFormat = 1;
    else if (h->m_formatBits & 2)
        nFormat = 3;
    else
        nFormat = 4;

    if (h->m_nDataSize != 0)
    {
        m_bCompressed = 1;
        int nWidth = h->m_width;
        int nHeight = h->m_height;
        m_nUncompressedSize = 0;
        m_nCompressedSize = h->m_nDataSize;
        int nBlockBytes = (nFormat == 4) ? DXT5_BLOCK_BYTES : DXT1_BLOCK_BYTES;
        int nAspect = nHeight / nWidth;
        int nFaces = (nAspect == 6) ? nAspect : 1;
        int nCurH = (nAspect == 6) ? nHeight / 6 : nHeight;
        int nSize = h->m_nDataSize;
        for (int i = 0; i < h->m_nMipCount - 1; i++)
        {
            nWidth >>= 1;
            nCurH >>= 1;
            nSize += ((nWidth + 3) / 4) * nBlockBytes * ((nCurH + 3) / 4);
        }
        if (nAspect == 6)
        {
            nSize = nSize - nSize % TXB_FACE_ALIGN + TXB_FACE_ALIGN;
            nSize = nSize * nFaces;
        }
        m_pTXBTXI = static_cast<unsigned char*>(m_pTXBData) + nSize;
        m_nTXISize = m_nSize - nSize - TXB_HEADER_SIZE;
        m_bLoaded = 1;
        return 1;
    }
    else
    {
        int nWidth = h->m_width;
        int nHeight = h->m_height;
        m_nUncompressedSize = 0;
        m_nCompressedSize = 0;
        int nAspect = nHeight / nWidth;
        int nFaces = (nAspect == 6) ? nAspect : 1;
        int nCurH = (nAspect == 6) ? nHeight / 6 : nHeight;
        unsigned int nSize = 0;
        for (unsigned int i = 0; i < h->m_nMipCount; i++)
        {
            nSize += nWidth * nFormat * nCurH;
            nWidth >>= 1;
            nCurH >>= 1;
        }
        m_nUncompressedSize = nSize;
        if (nAspect == 6)
        {
            nSize = nSize + TXB_FACE_ALIGN - nSize % TXB_FACE_ALIGN;
            nSize = nSize * nFaces;
            m_nUncompressedSize = nSize;
        }
        m_pTXBTXI = static_cast<unsigned char*>(m_pTXBData) + nSize;
        m_nTXISize = m_nSize - nSize - TXB_HEADER_SIZE;
        m_bLoaded = 1;
        return 1;
    }
}
