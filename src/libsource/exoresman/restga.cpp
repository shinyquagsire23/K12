#include "k2stdint.h"
#include "restga.h"
#include "exofile.h"

#include "port/libcstring.h"

// The slack the RLE decoders allocate past the decoded image.
enum {
    RLE_DECODE_SLACK = 14,
};
#if KOTOR1
void ByteSwap(unsigned short* p);
#endif

CResTGA::CResTGA()
{
    m_bAllocTrailingData = 1;
    m_bDataAllocated = 0;
    m_bColorMapped = 0;
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_nTGASize = 0;
    m_nTGABlobSize = 0;
    m_pTGAColorMap = 0;
    m_pTGAData = 0;
    m_pTGAHeader = 0;
}

CResTGA::~CResTGA()
{
}

int CResTGA::CopyTGAData(unsigned char* pDst)
{
    if (!m_bLoaded)
        return 0;
    memcpy(pDst, m_pTGAData, m_nTGASize);  // idiom-exempt: runtime length
    return 1;
}

int CResTGA::GetTGAAttrib(unsigned int* pWidth, unsigned int* pHeight,
                          unsigned char* pBitsPerPixel)
{
    TGAHeader* h = static_cast<TGAHeader*>(m_pTGAHeader);
    if (h == 0)
        return 0;
    *pWidth = h->m_width;
    *pHeight = h->m_height;
    *pBitsPerPixel = h->m_bitsPerPixel;
    return 1;
}

void* CResTGA::GetTGAColorMapPtr()
{
    return m_pTGAColorMap;
}

void* CResTGA::GetTGADataPtr()
{
    return m_pTGAData;
}

void* CResTGA::GetTGAHeaderPtr()
{
    return m_pTGAHeader;
}

int CResTGA::GetTGASize()
{
    return m_nTGASize;
}

int CResTGA::IsColorMapped()
{
    return m_bColorMapped;
}

int CResTGA::IsCompressed()
{
    return m_bCompressed;
}

int CResTGA::IsLoaded()
{
    return m_bLoaded;
}

int CResTGA::OnResourceFreed()
{
    if (m_bDataAllocated && m_pTGAData != 0)
        operator delete[](m_pTGAData);
    m_bDataAllocated = 0;
    m_bColorMapped = 0;
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_nTGASize = 0;
    m_nTGABlobSize = 0;
    m_pTGAColorMap = 0;
    m_pTGAData = 0;
    m_pTGAHeader = 0;
    return 1;
}

// TGA pixels are BGR(A); every path that publishes pixel data swaps bytes 0 and 2 of each pixel.
// Image type 3 is greyscale, which has no colour to swap.
static void SwapRGB(TGAHeader* h, unsigned char* pData)
{
    if (h->m_imageType == 3)
        return;
    unsigned int nPixels = h->m_width * h->m_height;
    if (nPixels == 0)
        return;
    unsigned int nStride = h->m_bitsPerPixel >> 3;
    unsigned char* p = pData + 2;
    do {
        unsigned char nTmp = p[-2];
        p[-2] = p[0];
        p[0] = nTmp;
        p += nStride;
    } while (--nPixels);
}

int CResTGA::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    if (m_pTGAHeader != 0 || m_pTGAData != 0 || m_pTGAColorMap != 0)
        return 0;

#if KOTOR1
    // K1 byte-swaps the six 16-bit header fields in place.
    m_pTGAHeader = m_pFileData;
    ByteSwap(reinterpret_cast<unsigned short*>(&static_cast<TGAHeader*>(m_pTGAHeader)->m_abColorMapSpec[0]));  // raw-offset: TGA file header
    ByteSwap(reinterpret_cast<unsigned short*>(&static_cast<TGAHeader*>(m_pTGAHeader)->m_abColorMapSpec[2]));  // raw-offset: TGA file header
    ByteSwap(&static_cast<TGAHeader*>(m_pTGAHeader)->m_xOrigin);
    ByteSwap(&static_cast<TGAHeader*>(m_pTGAHeader)->m_yOrigin);
    ByteSwap(&static_cast<TGAHeader*>(m_pTGAHeader)->m_width);
    ByteSwap(&static_cast<TGAHeader*>(m_pTGAHeader)->m_height);
    TGAHeader* h = static_cast<TGAHeader*>(m_pTGAHeader);
    unsigned int nIDLength = h->m_idLength;
    m_pTGAColorMap = static_cast<unsigned char*>(m_pFileData) + nIDLength + TGA_HEADER_SIZE;
#else
    TGAHeader* h = static_cast<TGAHeader*>(m_pFileData);
    m_pTGAHeader = h;
    unsigned int nIDLength = h->m_idLength;
    m_pTGAColorMap = reinterpret_cast<unsigned char*>(h) + nIDLength + TGA_HEADER_SIZE;  // raw-offset: TGA file header
#endif
    unsigned int nColorMapLength = *reinterpret_cast<const unsigned short*>(&h->m_abColorMapSpec[2]);  // raw-offset: TGA file header

    switch (h->m_imageType) {
    case 1:
        {
            unsigned int nColorMapSize = (h->m_abColorMapSpec[4] >> 3) * nColorMapLength;
            m_nTGASize = m_nSize - TGA_HEADER_SIZE - nIDLength - nColorMapSize;
            m_pTGAData = static_cast<unsigned char*>(m_pTGAColorMap) + nColorMapSize;
            m_bColorMapped = 1;
        }
        break;
    case 2:
    case 3:
        {
            unsigned int nColorMapSize = (h->m_abColorMapSpec[4] >> 3) * nColorMapLength;
            m_nTGASize = m_nSize - TGA_HEADER_SIZE - nIDLength - nColorMapSize;
            m_pTGAData = static_cast<unsigned char*>(m_pTGAColorMap) + nColorMapSize;
        }
        break;
    case 9:
        {
            unsigned int nColorMapSize = (h->m_abColorMapSpec[4] >> 3) * nColorMapLength;
            m_nTGABlobSize = m_nSize - TGA_HEADER_SIZE - nIDLength - nColorMapSize;
            m_nTGASize = h->m_width * h->m_height * (h->m_bitsPerPixel >> 3);
            m_pTGAData = static_cast<unsigned char*>(m_pTGAColorMap) + nColorMapSize;
        }
        m_bColorMapped = 1;
        m_bCompressed = 1;
        ReadColorMappedRLETGA();
        break;
    case 10:
        {
            unsigned int nColorMapSize = (h->m_abColorMapSpec[4] >> 3) * nColorMapLength;
            m_nTGABlobSize = m_nSize - TGA_HEADER_SIZE - nIDLength - nColorMapSize;
            m_nTGASize = h->m_width * h->m_height * (h->m_bitsPerPixel >> 3);
            m_pTGAData = static_cast<unsigned char*>(m_pTGAColorMap) + nColorMapSize;
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

    SwapRGB(static_cast<TGAHeader*>(m_pTGAHeader), static_cast<unsigned char*>(m_pTGAData));

    m_bLoaded = 1;
    return 1;
}

// PackBits-style RLE decode of an 8-bit colour-mapped image: allocate an owned buffer of
// m_nTGASize (+14 slack) and expand into it. Control byte: bit 7 set means a run (repeat one byte
// count+1 times), clear means a literal (copy count+1 bytes); the count is the low 7 bits.
int CResTGA::ReadColorMappedRLETGA()
{
    uint64_t size = m_nTGASize;
    unsigned char* src = static_cast<unsigned char*>(m_pTGAData);
    m_pTGAData = new unsigned char[size + RLE_DECODE_SLACK];
    if (size != 0) {
        unsigned int dstPos = 0;
        unsigned int srcPos = 0;
        do {
            unsigned char ctrl = src[srcPos++];
            unsigned int newPos = dstPos + (ctrl & RLE_COUNT_MASK) + 1;
            if (ctrl & RLE_RUN_FLAG) {
                unsigned char runByte = src[srcPos++];
                while (dstPos < newPos)
                    static_cast<unsigned char*>(m_pTGAData)[dstPos++] = runByte;
            } else {
                while (dstPos < newPos)
                    static_cast<unsigned char*>(m_pTGAData)[dstPos++] = src[srcPos++];
            }
        } while (dstPos < m_nTGASize);
    }
    m_bDataAllocated = 1;
    return 1;
}

// The same PackBits decode for a direct-colour (24- or 32-bit) image; every other depth fails.
// A packet is `count+1` whole pixels, so the run arm repeats one pixel-strided group and the
// literal arm walks `count+1` groups forward.
int CResTGA::ReadUnmappedRLETGA()
{
    uint64_t size = m_nTGASize;
    unsigned char* src = static_cast<unsigned char*>(m_pTGAData);
    m_pTGAData = new unsigned char[size + RLE_DECODE_SLACK];
    TGAHeader* h = static_cast<TGAHeader*>(m_pTGAHeader);

    if (h->m_bitsPerPixel == 24) {
        if (size != 0) {
            unsigned int dstPos = 0;
            unsigned int srcPos = 0;
            do {
                unsigned char ctrl = src[srcPos++];
                unsigned int newPos = dstPos + 3 * ((ctrl & RLE_COUNT_MASK) + 1);
                if (ctrl & RLE_RUN_FLAG) {
                    while (dstPos < newPos) {
                        static_cast<unsigned char*>(m_pTGAData)[dstPos] = src[srcPos];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 1] = src[srcPos + 1];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 2] = src[srcPos + 2];
                        dstPos += 3;
                    }
                    srcPos += 3;
                } else {
                    while (dstPos < newPos) {
                        static_cast<unsigned char*>(m_pTGAData)[dstPos] = src[srcPos];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 1] = src[srcPos + 1];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + 2] = src[srcPos + 2];
                        dstPos += 3;
                        srcPos += 3;
                    }
                }
            } while (dstPos < m_nTGASize);
        }
    } else if (h->m_bitsPerPixel == 32) {
        if (size != 0) {
            unsigned int dstPos = 0;
            unsigned int srcPos = 0;
            do {
                // srcPos stays on the control byte here; the pixel bytes are srcPos+1..+4.
                unsigned char ctrl = src[srcPos];
                unsigned int newPos = dstPos + 4 * ((ctrl & RLE_COUNT_MASK) + 1);
                if (ctrl & RLE_RUN_FLAG) {
                    while (dstPos < newPos) {
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
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + nIdx] = src[srcPos + nIdx + 1];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + nIdx + 1] = src[srcPos + nIdx + 2];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + nIdx + 2] = src[srcPos + nIdx + 3];
                        static_cast<unsigned char*>(m_pTGAData)[dstPos + nIdx + 3] = src[srcPos + nIdx + 4];
                        nIdx += 4;
                    }
                    dstPos += nIdx;
                    srcPos += nIdx + 1;
                }
            } while (dstPos < m_nTGASize);
        }
    } else {
        return 0;
    }

    m_bDataAllocated = 1;
    return 1;
}

int CResTGA::Write(const CExoString& path)
{
    CExoFile file(path, 3, CExoString("wb"));
    unsigned char* pBuffer;
    TGAHeader* h;
    if (m_nTGASize == 0 || !file.FileOpened())
        return 0;

    pBuffer = new unsigned char[m_nTGASize];
    if (m_bLoaded)
        memcpy(pBuffer, m_pTGAData, m_nTGASize);  // idiom-exempt: runtime length

    h = static_cast<TGAHeader*>(m_pTGAHeader);
    if (h->m_imageType != 3) {
        unsigned int nPixels = h->m_width * h->m_height;
        if (nPixels != 0) {
            unsigned int nStride = h->m_bitsPerPixel >> 3;
            unsigned char* pPixel = pBuffer + h->m_idLength + GetTGAColorMapSize(h) + TGA_HEADER_SIZE;
            unsigned int nOffset = 0;
            for (unsigned int n = nPixels; n != 0; n--) {
                unsigned char nTmp = pPixel[nOffset];
                pPixel[nOffset] = pPixel[nOffset + 2];
                pPixel[nOffset + 2] = nTmp;
                nOffset += nStride;
            }
        }
    }

    file.Write(h, TGA_HEADER_SIZE, 1);
    file.Write(pBuffer, m_nTGASize, 1);
    operator delete[](pBuffer);
    return 1;
}
