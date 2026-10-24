#include "resbmp.h"
#include <stddef.h>

// The BMP file header and its "BM" tag, the info header that follows it (the colour table comes
// next), and a 4-bit pixel's half of its byte.
enum {
    BMP_FILE_TYPE = 0x4d42,
    BMP_FILE_HEADER_SIZE = 14,
    BMP_INFO_HEADER_SIZE = 40,
    LOW_NIBBLE = 0xf,
};

// The derived-field block is cleared as one span, bounded so no trailing padding is touched.
#define BMP_CLEAR(p) __builtin_memset(&(p)->m_nColorCount, 0, static_cast<size_t>( \
    reinterpret_cast<char*>(&(p)->m_bLoaded + 1) - reinterpret_cast<char*>(&(p)->m_nColorCount)))

CResBmp::CResBmp()
{
    BMP_CLEAR(this);
}

// Same body as the default constructor; the argument is ignored.
CResBmp::CResBmp(unsigned int)
{
    BMP_CLEAR(this);
}

CResBmp::~CResBmp()
{
}

int CResBmp::GetBitDepth()
{
    if (!m_bLoaded)
        return -1;
    return static_cast<BitmapInfoHeader*>(m_pInfoHeader)->m_biBitCount;
}

int CResBmp::GetColorCount()
{
    if (!m_bLoaded)
        return -1;
    m_nColorCount = 0;
    return 0;
}

void* CResBmp::GetColorTable()
{
    return m_pColorTable;
}

void* CResBmp::GetImageData()
{
    if (!m_bLoaded)
        return 0;
    return m_pImageData;
}

void CResBmp::GetImageDimensions(int& rWidth, int& rHeight)
{
    if (m_bLoaded) {
        BitmapInfoHeader* h = static_cast<BitmapInfoHeader*>(m_pInfoHeader);
        rWidth = h->m_biWidth;
        rHeight = h->m_biHeight;
    } else {
        rWidth = 0;
        rHeight = 0;
    }
}

// Reads one pixel through the palette at 4- or 8-bit depth, or as a straight word at 24-bit. An
// out-of-range coordinate on the palette path is not an error: it yields colour-table entry 0 and
// still reports success.
int CResBmp::GetPixelColor(int x, int y, tagRGBQUAD& rQuad)
{
    if (!m_bLoaded)
        return 0;

    BitmapInfoHeader* pInfo = static_cast<BitmapInfoHeader*>(m_pInfoHeader);
    int nBitCount = pInfo->m_biBitCount;

    switch (nBitCount)
    {
    case 4:
    case 8:
    {
        tagRGBQUAD* paColorTable = static_cast<tagRGBQUAD*>(m_pColorTable);
        int nIndex = 0;
        if (x >= 0 && x < pInfo->m_biWidth && y >= 0 && y < pInfo->m_biHeight)
        {
            switch (nBitCount)
            {
            case 8:
            {
                unsigned char* pPixels = static_cast<unsigned char*>(m_pImageData);
                int nRow = pInfo->m_biHeight - 1 - y;
                nIndex = pPixels[nRow * m_nStride * 4 + x];
                break;
            }
            case 4:
            {
                unsigned char* pPixels = static_cast<unsigned char*>(m_pImageData);
                int nRow = pInfo->m_biHeight - 1 - y;
                unsigned char nByte = pPixels[nRow * m_nStride * 4 + (x >> 1)];
                if (x & 1)
                    nIndex = nByte & LOW_NIBBLE;
                else
                    nIndex = nByte >> 4;
                break;
            }
            }
        }
        rQuad = paColorTable[nIndex];
        return 1;
    }
    case 24:
    {
        unsigned char* pPixels = static_cast<unsigned char*>(m_pImageData);
        int nRow = pInfo->m_biHeight - 1 - y;
        rQuad = *reinterpret_cast<tagRGBQUAD*>(pPixels + nRow * m_nStride * 4 + x * 3);
        return 1;
    }
    }
    return 0;
}

// The paletted index behind GetPixelColor; 4-bit pixels are packed two per byte, high nibble first.
int CResBmp::GetPixelValue(int x, int y)
{
    int nValue = 0;
    if (m_bLoaded)
    {
        if (x >= 0 && x < static_cast<BitmapInfoHeader*>(m_pInfoHeader)->m_biWidth &&
            y >= 0 && y < static_cast<BitmapInfoHeader*>(m_pInfoHeader)->m_biHeight)
        {
            switch (static_cast<BitmapInfoHeader*>(m_pInfoHeader)->m_biBitCount)
            {
            case 8:
            {
                unsigned char* pPixels = static_cast<unsigned char*>(m_pImageData);
                int nRow = static_cast<BitmapInfoHeader*>(m_pInfoHeader)->m_biHeight - 1 - y;
                return pPixels[nRow * m_nStride * 4 + x];
            }
            case 4:
            {
                unsigned char* pPixels = static_cast<unsigned char*>(m_pImageData);
                int nRow = static_cast<BitmapInfoHeader*>(m_pInfoHeader)->m_biHeight - 1 - y;
                unsigned char nByte = pPixels[nRow * m_nStride * 4 + (x >> 1)];
                if (x & 1)
                    return nByte & LOW_NIBBLE;
                return nByte >> 4;
            }
            default:
                return 0;
            }
        }
    }
    return nValue;
}

int CResBmp::IsLoaded()
{
    return m_bLoaded;
}

int CResBmp::OnResourceFreed()
{
    BMP_CLEAR(this);
    return 1;
}

// Parses the file: keeps the raw buffer, points the info header past the file header, then
// dispatches on the bit count. 24-bit has no palette; 4- and 8-bit publish one. Every arm records
// the image offset and a row stride rounded up to whole dwords.
int CResBmp::OnResourceServiced()
{
    if (m_bLoaded)
        return 1;
    unsigned char* pData = static_cast<unsigned char*>(m_pFileData);
    if (pData == 0)
        return 0;
    m_pFileHeader = pData;
    if (reinterpret_cast<BitmapFileHeader*>(pData)->m_bfType != BMP_FILE_TYPE) {
        m_pFileHeader = 0;
        return 0;
    }
    BitmapInfoHeader* pInfoHeader = reinterpret_cast<BitmapInfoHeader*>(pData + BMP_FILE_HEADER_SIZE);
    m_pInfoHeader = pInfoHeader;
    if (pInfoHeader->m_biCompression != 0)
        return 0;

    switch (pInfoHeader->m_biBitCount) {
    case 4:
        m_nColorCount = pInfoHeader->m_biClrUsed;
        m_pColorTable = pData + BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE;
        m_pImageData = pData + reinterpret_cast<BitmapFileHeader*>(pData)->m_bfOffBits;
        m_nStride = pInfoHeader->m_biWidth / 8;
        if (pInfoHeader->m_biWidth % 8)
            m_nStride++;
        break;
    case 8:
        m_nColorCount = pInfoHeader->m_biClrUsed;
        m_pColorTable = pData + BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE;
        m_pImageData = pData + reinterpret_cast<BitmapFileHeader*>(pData)->m_bfOffBits;
        m_nStride = pInfoHeader->m_biWidth / 4;
        if (pInfoHeader->m_biWidth % 4)
            m_nStride++;
        break;
    case 24:
        m_pColorTable = 0;
        m_nColorCount = -1;
        m_pImageData = pData + reinterpret_cast<BitmapFileHeader*>(pData)->m_bfOffBits;
        m_nStride = (pInfoHeader->m_biWidth * 3) / 4;
        if ((pInfoHeader->m_biWidth * 3) % 4)
            m_nStride++;
        break;
    default:
        m_pFileHeader = 0;
        m_pInfoHeader = 0;
        return 0;
    }
    m_bLoaded = 1;
    return 1;
}
