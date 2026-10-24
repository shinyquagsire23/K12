#include "res2da.h"

#if KOTOR1
void ByteSwap(unsigned int* p);
#endif

// Header tags, packed little-endian.
static const unsigned int RES2DA_FILE_TYPE     = '2' | ('D' << 8) | ('A' << 16) | (' ' << 24);
static const unsigned int RES2DA_VERSION_TEXT  = 'V' | ('2' << 8) | ('.' << 16) | ('0' << 24);
static const unsigned int RES2DA_VERSION_BINARY = 'V' | ('2' << 8) | ('.' << 16) | ('b' << 24);

CRes2DA::~CRes2DA()
{
}

// Unlike CResMDX and CRes4PC, CRes::m_bAllocHeaderData is left at the base's zero here: .2da
// resources are not encapsulated.
CRes2DA::CRes2DA()
{
    m_p2DAData = 0;
    m_p2DAHeader = 0;
    m_bLoaded = 0;
    m_n2DASize = 0;
    m_b2daBinary = 0;
}

// Validates the 8-byte header and publishes the payload view. The size and header-pointer stores are
// unconditional once a buffer exists, so they sit above the tag guards.
int CRes2DA::OnResourceServiced()
{
#if KOTOR1
    // K1 byte-swaps both header words in place before the tag checks, and reads the tags and the
    // payload back through the members.
    if (m_pFileData == 0)
        return 0;

    if (m_bLoaded)
        return 1;

    m_p2DAHeader = static_cast<Header2DA*>(m_pFileData);
    m_n2DASize = m_nSize - sizeof(Header2DA);
    ByteSwap(reinterpret_cast<unsigned int*>(static_cast<Header2DA*>(m_p2DAHeader)->szFileType));  // raw-offset: 2DA file header four-character codes
    ByteSwap(reinterpret_cast<unsigned int*>(static_cast<Header2DA*>(m_p2DAHeader)->szFileVersion));  // raw-offset: 2DA file header four-character codes

    if (*reinterpret_cast<unsigned int*>(static_cast<Header2DA*>(m_p2DAHeader)->szFileType) != RES2DA_FILE_TYPE) {  // raw-offset: 2DA file header four-character codes
        OnResourceFreed();
        return 0;
    }

    if (*reinterpret_cast<unsigned int*>(static_cast<Header2DA*>(m_p2DAHeader)->szFileVersion) != RES2DA_VERSION_TEXT) {  // raw-offset: 2DA file header four-character codes
        if (*reinterpret_cast<unsigned int*>(static_cast<Header2DA*>(m_p2DAHeader)->szFileVersion) != RES2DA_VERSION_BINARY) {  // raw-offset: 2DA file header four-character codes
            OnResourceFreed();
            return 0;
        }
        m_b2daBinary = 1;
    }

    m_bLoaded = 1;
    m_p2DAData = static_cast<Header2DA*>(m_pFileData) + 1;
    return 1;
#else
    if (m_pFileData == 0)
        return 0;

    if (m_bLoaded)
        return 1;

    Header2DA* pHeader = static_cast<Header2DA*>(m_pFileData);

    m_p2DAHeader = pHeader;
    m_n2DASize = m_nSize - sizeof(Header2DA);

    if (*reinterpret_cast<unsigned int*>(pHeader->szFileType) != RES2DA_FILE_TYPE) {  // raw-offset: 2DA file header four-character codes
        OnResourceFreed();
        return 0;
    }

    if (*reinterpret_cast<unsigned int*>(pHeader->szFileVersion) != RES2DA_VERSION_TEXT) {  // raw-offset: 2DA file header four-character codes
        if (*reinterpret_cast<unsigned int*>(pHeader->szFileVersion) != RES2DA_VERSION_BINARY) {  // raw-offset: 2DA file header four-character codes
            OnResourceFreed();
            return 0;
        }
        m_b2daBinary = 1;
    }

    m_p2DAData = pHeader + 1;
    m_bLoaded = 1;
    return 1;
#endif
}

int CRes2DA::OnResourceFreed()
{
#if KOTOR1
    if (m_bLoaded) {
        ByteSwap(reinterpret_cast<unsigned int*>(static_cast<Header2DA*>(m_p2DAHeader)->szFileType));  // raw-offset: 2DA file header four-character codes
        ByteSwap(reinterpret_cast<unsigned int*>(static_cast<Header2DA*>(m_p2DAHeader)->szFileVersion));  // raw-offset: 2DA file header four-character codes
    }
#endif
    m_bLoaded = 0;
    m_n2DASize = 0;
    m_p2DAData = 0;
    m_b2daBinary = 0;
    m_p2DAHeader = 0;
    return 1;
}

void* CRes2DA::Get2DAHeaderPtr()
{
    return m_p2DAHeader;
}

void* CRes2DA::Get2DADataPtr()
{
    return m_p2DAData;
}

int CRes2DA::Get2DASize()
{
    return m_n2DASize;
}

int CRes2DA::IsLoaded()
{
    return m_bLoaded;
}
