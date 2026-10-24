#include "k2stdint.h"
#include "resplt.h"

#include "port/libcstring.h"
#include "restga.h"

// The PLT header and its "PLT " tag.
enum {
    PLT_HEADER_SIZE = 24,
    FOURCC_PLT = 0x20544c50,
};
extern void ByteSwap(unsigned int* p);

CResPLT::CResPLT()
{
    m_bAllocTrailingData = 1;
    m_bOwnsData = 0;
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_nPLTSize = 0;
    m_nSrcSize = 0;
    m_pPLTData = 0;
    m_pPLTHeader = 0;
}

CResPLT::~CResPLT()
{
}

int CResPLT::CopyPLTData(unsigned char* pDst)
{
    if (!m_bLoaded)
        return 0;
    memcpy(pDst, m_pPLTData, m_nPLTSize);  // idiom-exempt: runtime length
    return 1;
}

int CResPLT::GetPLTAttrib(unsigned int* pWidth, unsigned int* pHeight, unsigned int* pNumLayers)
{
    PLTHeader* h = static_cast<PLTHeader*>(m_pPLTHeader);
    if (h == 0)
        return 0;
    *pWidth = h->m_width;
    *pHeight = h->m_height;
    *pNumLayers = h->m_nNumLayers;
    return 1;
}

void* CResPLT::GetPLTDataPtr()
{
    return m_pPLTData;
}

void* CResPLT::GetPLTHeaderPtr()
{
    return m_pPLTHeader;
}

int CResPLT::GetPLTSize()
{
    return m_nPLTSize;
}

int CResPLT::IsCompressed()
{
    return m_bCompressed;
}

int CResPLT::IsLoaded()
{
    return m_bLoaded;
}

int CResPLT::OnResourceFreed()
{
    if (m_bOwnsData && m_pPLTData)
        operator delete[](m_pPLTData);
    m_bOwnsData = 0;
    m_bCompressed = 0;
    m_bLoaded = 0;
    m_nPLTSize = 0;
    m_nSrcSize = 0;
    m_pPLTData = 0;
    m_pPLTHeader = 0;
    return 1;
}

int CResPLT::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    if (m_pPLTHeader != 0 || m_pPLTData != 0)
        return 0;
    m_pPLTHeader = m_pFileData;
    ByteSwap(&static_cast<PLTHeader*>(m_pPLTHeader)->m_magic);
    ByteSwap(&static_cast<PLTHeader*>(m_pPLTHeader)->m_nVersion);
    ByteSwap(&static_cast<PLTHeader*>(m_pPLTHeader)->m_nNumLayers);
    ByteSwap(&static_cast<PLTHeader*>(m_pPLTHeader)->m_bCompressed);
    ByteSwap(&static_cast<PLTHeader*>(m_pPLTHeader)->m_width);
    ByteSwap(&static_cast<PLTHeader*>(m_pPLTHeader)->m_height);
    if (static_cast<PLTHeader*>(m_pPLTHeader)->m_magic != FOURCC_PLT)
        return 0;
    if (static_cast<PLTHeader*>(m_pPLTHeader)->m_bCompressed) {
        unsigned char* src = reinterpret_cast<unsigned char*>(static_cast<PLTHeader*>(m_pFileData) + 1);
        m_bCompressed = 1;
        m_nSrcSize = m_nSize - PLT_HEADER_SIZE;
        m_pPLTData = src;
        unsigned int size = m_nPLTSize;
        m_pPLTData = new unsigned char[size];
        if (size != 0) {
            unsigned int dstPos = 0;
            unsigned int srcPos = 0;
            do {
                unsigned char ctrl = src[srcPos++];  // raw-offset: PLT file bytes
                unsigned int newPos = dstPos + (ctrl & RLE_COUNT_MASK) + 1;
#ifdef K2_FIX_ENGINE_BUGS
                if (newPos > size)
                    newPos = size;
#endif
                if (ctrl & RLE_RUN_FLAG) {
                    unsigned char runByte = src[srcPos++];  // raw-offset: PLT file bytes
                    while (dstPos < newPos)
                        static_cast<unsigned char*>(m_pPLTData)[dstPos++] = runByte;
                } else {
                    while (dstPos < newPos)
                        static_cast<unsigned char*>(m_pPLTData)[dstPos++] = src[srcPos++];  // raw-offset: PLT file bytes
                }
            } while (dstPos < m_nPLTSize);
        }
        m_bOwnsData = 1;
    } else {
        m_bOwnsData = 0;
        m_bCompressed = 0;
        m_nPLTSize = m_nSize - PLT_HEADER_SIZE;
        m_nSrcSize = m_nSize - PLT_HEADER_SIZE;
        m_pPLTData = static_cast<PLTHeader*>(m_pFileData) + 1;
    }
    m_bLoaded = 1;
    return 1;
}

// PackBits-style RLE decode.
int CResPLT::ReadRLEPLT()
{
    unsigned char* src = static_cast<unsigned char*>(m_pPLTData);
    unsigned int size = m_nPLTSize;
    m_pPLTData = new unsigned char[size];
    if (size != 0) {
        unsigned int dstPos = 0;
        unsigned int srcPos = 0;
        do {
            unsigned char ctrl = src[srcPos++];
            unsigned int newPos = dstPos + (ctrl & RLE_COUNT_MASK) + 1;
#ifdef K2_FIX_ENGINE_BUGS
            if (newPos > size)
                newPos = size;
#endif
            if (ctrl & RLE_RUN_FLAG) {
                unsigned char runByte = src[srcPos++];
                while (dstPos < newPos)
                    static_cast<unsigned char*>(m_pPLTData)[dstPos++] = runByte;
            } else {
                while (dstPos < newPos)
                    static_cast<unsigned char*>(m_pPLTData)[dstPos++] = src[srcPos++];
            }
        } while (dstPos < m_nPLTSize);
    }
    m_bOwnsData = 1;
    return 1;
}
