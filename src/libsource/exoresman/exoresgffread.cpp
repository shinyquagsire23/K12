#include "k2stdint.h"
#include "exoresgffread.h"
#include "crc32poly.h"

unsigned int CResGFFFieldIDHash::m_nCRC32[256];

// The standard reflected CRC-32 table builder, four lanes wide.
void CResGFFFieldIDHash::Initialize()
{
    for (int i = 0; i < 256; i++)
    {
        unsigned int nValue = static_cast<unsigned int>(i);

        for (int j = 0; j < 8; j++)
        {
            if (nValue & 1)
                nValue = (nValue >> 1) ^ CRC32_POLYNOMIAL;
            else
                nValue = nValue >> 1;
        }

        m_nCRC32[i] = nValue;
    }
}

// Hashes a name of nLength bytes.
unsigned int CResGFFFieldIDHash::Hash(const char* pszName, unsigned int nLength)
{
    unsigned int nHash = 0;
    for (unsigned int i = 0; i < nLength; i++)
    {
        if (pszName[i] == 0)
            break;
        nHash = m_nCRC32[(nHash & 0xff) ^ static_cast<unsigned char>(pszName[i])] ^ (nHash >> 8);
    }
    return nHash;
}
