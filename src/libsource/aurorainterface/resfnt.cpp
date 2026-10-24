#include "k2stdint.h"
#include "resfnt.h"
#include "resgff.h"

#include "port/libcstring.h"
#include "restga.h"

// Empty body. It does not free m_pTGAData, which is OnResourceFreed's job, driven by
// m_bTGADataOwned.
CResFNT::~CResFNT()
{
}

// Seeds CRes::m_bAllocTrailingData rather than m_bAllocHeaderData: .fnt buffers are allocated with 14
// bytes of slack instead of an encapsulated header.
CResFNT::CResFNT()
{
    m_bAllocTrailingData = 1;
    m_bTGADataOwned = 0;
    m_bColorMapped = 0;
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_fntSize = 0;
    m_tgaSize = 0;
    m_nTGABlobSize = 0;
    m_pTGAColorMap = 0;
    m_pTGAData = 0;
    m_pTGAHeader = 0;
    m_pFNTData = 0;
    m_pFNTHeader = 0;
}

// Releases the decompressed TGA buffer (only the RLE path allocates one) and resets every view.
int CResFNT::OnResourceFreed()
{
    if (m_bTGADataOwned && m_pTGAData != 0)
        delete[] static_cast<unsigned char*>(m_pTGAData);

    m_bTGADataOwned = 0;
    m_bColorMapped = 0;
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_fntSize = 0;
    m_tgaSize = 0;
    m_nTGABlobSize = 0;
    m_pTGAColorMap = 0;
    m_pTGAData = 0;
    m_pTGAHeader = 0;
    m_pFNTData = 0;
    m_pFNTHeader = 0;
    return 1;
}

// Parses the file buffer into the split TGA and FNT halves. The .fnt container is a TGA whose pixel
// data is followed immediately by the FNT blob, so every arm ends by pointing m_pFNTHeader at the
// far end of the TGA payload.
//
// Every ByteSwap re-reads its member rather than using a cached local.
int CResFNT::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    if (m_pFNTHeader != 0 || m_pTGAHeader != 0 || m_pFNTData != 0
        || m_pTGAData != 0 || m_pTGAColorMap != 0)
        return 0;

    m_pTGAHeader = static_cast<HeaderTGA*>(m_pFileData);
    ByteSwap(reinterpret_cast<unsigned short*>(&static_cast<HeaderTGA*>(m_pFileData)->aColorMapSpec[0]));  // raw-offset: TGA file header
    ByteSwap(reinterpret_cast<unsigned short*>(&static_cast<HeaderTGA*>(m_pTGAHeader)->aColorMapSpec[2]));  // raw-offset: TGA file header
    ByteSwap(&static_cast<HeaderTGA*>(m_pTGAHeader)->nXOrigin);
    ByteSwap(&static_cast<HeaderTGA*>(m_pTGAHeader)->nYOrigin);
    ByteSwap(&static_cast<HeaderTGA*>(m_pTGAHeader)->nWidth);
    ByteSwap(&static_cast<HeaderTGA*>(m_pTGAHeader)->nHeight);

    HeaderTGA* h = static_cast<HeaderTGA*>(m_pTGAHeader);
    unsigned int nIDLength = h->nIdLength;
    unsigned int nColorMapLength = *reinterpret_cast<const unsigned short*>(&h->aColorMapSpec[2]);  // raw-offset: TGA file header
    m_pTGAColorMap = static_cast<unsigned char*>(m_pFileData) + nIDLength + TGA_HEADER_SIZE;

    switch (h->nImageType) {
    case 1:
        {
            unsigned int nColorMapSize = (h->aColorMapSpec[4] >> 3) * nColorMapLength;
            m_tgaSize = h->nWidth * h->nHeight * (h->nBitsPerPixel >> 3);
            m_bColorMapped = 1;
            m_pTGAData = static_cast<unsigned char*>(m_pTGAColorMap) + nColorMapSize;
            m_pFNTHeader = static_cast<unsigned char*>(m_pTGAData) + m_tgaSize;
        }
        break;
    case 2:
        {
            unsigned int nColorMapSize = (h->aColorMapSpec[4] >> 3) * nColorMapLength;
            m_tgaSize = h->nWidth * h->nHeight * (h->nBitsPerPixel >> 3);
            m_pTGAData = static_cast<unsigned char*>(m_pTGAColorMap) + nColorMapSize;
            m_pFNTHeader = static_cast<unsigned char*>(m_pTGAData) + m_tgaSize;
        }
        break;
    case 9:
        {
            unsigned int nColorMapSize = (h->aColorMapSpec[4] >> 3) * nColorMapLength;
            m_nTGABlobSize = m_nSize - TGA_HEADER_SIZE - nIDLength - nColorMapSize;
            m_tgaSize = h->nWidth * h->nHeight * (h->nBitsPerPixel >> 3);
            m_pTGAData = static_cast<unsigned char*>(m_pTGAColorMap) + nColorMapSize;
            m_pFNTHeader = static_cast<unsigned char*>(m_pTGAData) + m_nTGABlobSize;
        }
        m_bColorMapped = 1;
        m_bCompressed = 1;
        ReadColorMappedRLETGA();
        break;
    case 10:
        {
            unsigned int nColorMapSize = (h->aColorMapSpec[4] >> 3) * nColorMapLength;
            m_nTGABlobSize = m_nSize - TGA_HEADER_SIZE - nIDLength - nColorMapSize;
            m_tgaSize = h->nWidth * h->nHeight * (h->nBitsPerPixel >> 3);
            m_pTGAData = static_cast<unsigned char*>(m_pTGAColorMap) + nColorMapSize;
            m_pFNTHeader = static_cast<unsigned char*>(m_pTGAData) + m_nTGABlobSize;
        }
        m_bCompressed = 1;
        if (!ReadUnmappedRLETGA()) {
            OnResourceFreed();
            return 0;
        }
        break;
    default:
        m_pTGAColorMap = 0;
        m_pTGAHeader = 0;
        return 0;
    }

    // The FNT blob's own header is endian-round-tripped in place -- one int then five floats.
    ByteSwap(&static_cast<HeaderFNT*>(m_pFNTHeader)->nGlyphCount);
    ByteSwap(&static_cast<HeaderFNT*>(m_pFNTHeader)->afMetrics[0]);
    ByteSwap(&static_cast<HeaderFNT*>(m_pFNTHeader)->afMetrics[1]);
    ByteSwap(&static_cast<HeaderFNT*>(m_pFNTHeader)->afMetrics[2]);
    ByteSwap(&static_cast<HeaderFNT*>(m_pFNTHeader)->afMetrics[3]);
    ByteSwap(&static_cast<HeaderFNT*>(m_pFNTHeader)->afMetrics[4]);
    m_bLoaded = 1;
    m_pFNTData = reinterpret_cast<unsigned char*>(static_cast<HeaderFNT*>(m_pFNTHeader) + 1);
    return 1;
}

// PackBits-style RLE decode of an 8-bit colour-mapped image into an owned buffer: control bit 7
// set means run (repeat one byte, count+1 times), clear means literal (copy count+1 bytes), with
// count taken from the control byte's low seven bits.
//
// The allocation size and the loop bound come from two different fields. Each arm writes m_tgaSize
// (the decoded size and the right bound), but the new[] argument reads m_fntSize, which nothing in
// this class ever assigns. A colour-mapped .fnt therefore allocates a zero-byte buffer and decodes
// past it.
int CResFNT::ReadColorMappedRLETGA()
{
    unsigned char* src = static_cast<unsigned char*>(m_pTGAData);
#ifdef K2_FIX_ENGINE_BUGS
    uint64_t size = m_tgaSize;
#else
    uint64_t size = m_fntSize;
#endif
    m_pTGAData = new unsigned char[size];
    if (m_tgaSize != 0) {
        unsigned int dstPos = 0;
        unsigned int srcPos = 0;
        do {
            unsigned char ctrl = src[srcPos++];
            unsigned int newPos = dstPos + (ctrl & RLE_COUNT_MASK) + 1;
#ifdef K2_FIX_ENGINE_BUGS
            if (newPos > m_tgaSize)   // a final packet longer than the image
                newPos = m_tgaSize;
#endif
            if (ctrl & RLE_RUN_FLAG) {
                unsigned char runByte = src[srcPos++];
                while (dstPos < newPos)
                    static_cast<unsigned char*>(m_pTGAData)[dstPos++] = runByte;
            } else {
                while (dstPos < newPos)
                    static_cast<unsigned char*>(m_pTGAData)[dstPos++] = src[srcPos++];
            }
        } while (dstPos < m_tgaSize);
    }
    m_bTGADataOwned = 1;
    return 1;
}

// The same PackBits decode for a direct-colour (24- or 32-bit) image; every other depth fails and
// the resource is torn down. A packet is count+1 whole pixels.
int CResFNT::ReadUnmappedRLETGA()
{
    uint64_t size = m_tgaSize;
    unsigned char* src = static_cast<unsigned char*>(m_pTGAData);
    m_pTGAData = new unsigned char[size];
    HeaderTGA* h = static_cast<HeaderTGA*>(m_pTGAHeader);

    if (h->nBitsPerPixel == 24) {
        if (size != 0) {
            unsigned int dstPos = 0;
            unsigned int srcPos = 0;
            do {
                unsigned char ctrl = src[srcPos++];
                unsigned int newPos = dstPos + 3 * ((ctrl & RLE_COUNT_MASK) + 1);
#ifdef K2_FIX_ENGINE_BUGS
                if (newPos > m_tgaSize)   // a final packet longer than the image
                    newPos = m_tgaSize;
#endif
                if (ctrl & RLE_RUN_FLAG) {
                    while (dstPos < newPos) {
#ifdef K2_FIX_ENGINE_BUGS
                        K2_ASSUME(dstPos + 2 < size);   // dstPos and the clamped newPos are whole pixels
#endif
                        static_cast<unsigned char*>(m_pTGAData)[dstPos] = src[srcPos];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 1] = src[srcPos + 1];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 2] = src[srcPos + 2];
                        dstPos += 3;
                    }
                    srcPos += 3;
                } else {
                    while (dstPos < newPos) {
#ifdef K2_FIX_ENGINE_BUGS
                        K2_ASSUME(dstPos + 2 < size);   // dstPos and the clamped newPos are whole pixels
#endif
                        static_cast<unsigned char*>(m_pTGAData)[dstPos] = src[srcPos];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 1] = src[srcPos + 1];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 2] = src[srcPos + 2];
                        dstPos += 3;
                        srcPos += 3;
                    }
                }
            } while (dstPos < m_tgaSize);
        }
    } else if (h->nBitsPerPixel == 32) {
        if (size != 0) {
            unsigned int dstPos = 0;
            unsigned int srcPos = 0;
            do {
                // The control byte's own position is kept here, unlike the 24-bit arm which steps
                // past it: the pixel bytes are at ctrl+1..+4, a run packet is 5 bytes and a
                // literal is 1 + 4*count.
                unsigned char ctrl = src[srcPos];
                unsigned int newPos = dstPos + 4 * ((ctrl & RLE_COUNT_MASK) + 1);
#ifdef K2_FIX_ENGINE_BUGS
                if (newPos > m_tgaSize)   // a final packet longer than the image
                    newPos = m_tgaSize;
#endif
                if (ctrl & RLE_RUN_FLAG) {
                    while (dstPos < newPos) {
#ifdef K2_FIX_ENGINE_BUGS
                        K2_ASSUME(dstPos + 3 < size);   // dstPos and the clamped newPos are whole pixels
#endif
                        static_cast<unsigned char*>(m_pTGAData)[dstPos] = src[srcPos + 1];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 1] = src[srcPos + 2];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 2] = src[srcPos + 3];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 3] = src[srcPos + 4];
                        dstPos += 4;
                    }
                    srcPos += 5;
                } else {
                    unsigned int nIdx = 0;
                    while (dstPos + nIdx < newPos) {
#ifdef K2_FIX_ENGINE_BUGS
                        K2_ASSUME(dstPos + nIdx + 3 < size);   // dstPos, nIdx and the clamped newPos are whole pixels
#endif
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + nIdx] = src[srcPos + nIdx + 1];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + nIdx + 1] = src[srcPos + nIdx + 2];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + nIdx + 2] = src[srcPos + nIdx + 3];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + nIdx + 3] = src[srcPos + nIdx + 4];
                        nIdx += 4;
                    }
                    dstPos += nIdx;
                    srcPos += nIdx + 1;
                }
            } while (dstPos < m_tgaSize);
        }
    } else {
        return 0;
    }

    m_bTGADataOwned = 1;
    return 1;
}

int CResFNT::CopyFNTData(unsigned char* pDst)
{
    if (!m_bLoaded)
        return 0;
    memcpy(pDst, m_pFNTData, m_fntSize);  // idiom-exempt: runtime length
    return 1;
}

// The TGA blob's length is m_fntSize, not m_tgaSize, the same field CopyFNTData loads, while
// GetTGASize hands out m_tgaSize. A shipped copy-paste bug, kept as it is.
int CResFNT::CopyTGAData(unsigned char* pDst)
{
    if (!m_bLoaded)
        return 0;
    memcpy(pDst, m_pTGAData, m_fntSize);  // idiom-exempt: runtime length
    return 1;
}

int CResFNT::GetTGAAttrib(unsigned int* pWidth, unsigned int* pHeight, unsigned char* pBitsPerPixel)
{
    if (m_pFNTHeader == 0)
        return 0;

    HeaderTGA* pHeader = static_cast<HeaderTGA*>(m_pTGAHeader);
    *pWidth = pHeader->nWidth;
    *pHeight = pHeader->nHeight;
    *pBitsPerPixel = pHeader->nBitsPerPixel;
    return 1;
}

int CResFNT::IsColorMapped() {
    return m_bColorMapped;
}

int CResFNT::IsCompressed() {
    return m_bCompressed;
}

int CResFNT::IsLoaded() {
    return m_bLoaded;
}

int CResFNT::GetFNTSize() {
    return m_fntSize;
}

int CResFNT::GetTGASize() {
    return m_tgaSize;
}

void* CResFNT::GetTGAColorMapPtr() {
    return m_pTGAColorMap;
}

void* CResFNT::GetTGADataPtr() {
    return m_pTGAData;
}

void* CResFNT::GetTGAHeaderPtr() {
    return m_pTGAHeader;
}

void* CResFNT::GetFNTDataPtr() {
    return m_pFNTData;
}

void* CResFNT::GetFNTHeaderPtr() {
    return m_pFNTHeader;
}
