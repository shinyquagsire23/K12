#include "k2stdint.h"
#include "keyfile.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"
#include "port/libctime.h"
#include "port/crtapi.h"

#include "port/win32api.h"

// The generator the module uses throughout, with the obfuscation byte taken from the middle of the
// new state.
static const unsigned long long kLcgMul = 214013;
static const unsigned long long kLcgAdd = 2531011;

// The obfuscation streams' seeds (the write path reseeds for everything after byte 0), and the salt
// added to the checksum's low byte.
enum : unsigned long long {
    KEYFILE_READ_SEED = 0x6b47,
    KEYFILE_WRITE_SEED = 0x15e7938ee,
};
enum {
    KEYFILE_CHECKSUM_SALT = 0x79,
};

// The on-disk record is an 8-byte checksum word followed by the image.
static const int kKeyFileImageSize = sizeof(KeyFileData);
static const int kKeyFileRecordSize = kKeyFileImageSize + 8;

int KeyFileRead(FILE* pFile, KeyFileData* pData)
{
    char szBuf[kKeyFileRecordSize];
    KeyFileData kfTemp;               // the record image

    int nRead = static_cast<int>(fread(szBuf, 1, kKeyFileRecordSize, pFile));
    int nSize = nRead - 8;

    // Un-obfuscate every byte that was actually read, from a fixed seed.
    if (nRead >= 1) {
        unsigned int nCount = static_cast<unsigned int>(nRead);
        unsigned long long nSeed = KEYFILE_READ_SEED;
        for (uint64_t i = 0; i < nCount; i++) {
            nSeed = nSeed * kLcgMul + kLcgAdd;
            szBuf[i] -= static_cast<char>(nSeed >> 16);
        }
    }

    if (nSize < 0) {
        return 0;
    }

    // The 8-byte header is the byte-sum of the payload that follows it.
    unsigned long long nStored = *reinterpret_cast<unsigned long long*>(szBuf);
    unsigned long long nSum = 0;
    for (int i = 0; i < nSize; i++) {
        nSum += static_cast<unsigned char>(szBuf[8 + i]);
    }
    if (nStored != nSum) {
        return 0;
    }

    __memcpy_chk(&kfTemp, szBuf + 8, nSize, sizeof(kfTemp));
    if (nSize != kKeyFileImageSize) {
        return 0;
    }

    pData->m_date.m_nMonth = kfTemp.m_date.m_nMonth;
    pData->m_date.m_nDay = kfTemp.m_date.m_nDay;
    pData->m_date.m_nYear = kfTemp.m_date.m_nYear;
    pData->m_date.m_nVersion = kfTemp.m_date.m_nVersion;

    __strncpy_chk2(pData->m_szName, kfTemp.m_szName, sizeof(pData->m_szName), sizeof(pData->m_szName),
                   sizeof(kfTemp.m_szName) + sizeof(kfTemp.m_szKey));
    __strncpy_chk2(pData->m_szKey, kfTemp.m_szKey, sizeof(pData->m_szKey), static_cast<uint64_t>(-1),
                   sizeof(kfTemp.m_szKey));
    pData->m_szName[15] = 0;
    // The engine terminates the key here, sixteen bytes short of the field's end; the write path
    // uses the last byte instead.
    pData->m_szKey[sizeof(pData->m_szKey) - 17] = 0;
    return 1;
}

void KeyFileWrite(FILE* pFile, KeyFileData* pData)
{
    char szFile[kKeyFileRecordSize];  // the on-disk record
    KeyFileData kfTemp;               // the record image

    kfTemp.m_date = pData->m_date;

    time_t nSeed = time(0);
    for (int i = sizeof(KeyFileDate); i < kKeyFileImageSize; i++) {
        nSeed = nSeed * kLcgMul + kLcgAdd;
        reinterpret_cast<char*>(&kfTemp)[i] = static_cast<char>(nSeed >> 16);  // raw-offset: KEY file record bytes
    }

    pData->m_szName[15] = 0;
    __strcpy_chk(kfTemp.m_szName, pData->m_szName, sizeof(kfTemp.m_szName) + sizeof(kfTemp.m_szKey));
    pData->m_szKey[sizeof(pData->m_szKey) - 1] = 0;
    __strcpy_chk(kfTemp.m_szKey, pData->m_szKey, sizeof(kfTemp.m_szKey));

    *reinterpret_cast<unsigned long long*>(szFile) = 0;
    memcpy(szFile + 8, &kfTemp, sizeof(kfTemp));

    uint64_t nSum = 0;
    for (int i = 0; i < kKeyFileImageSize; i++) {
        nSum += static_cast<unsigned char>(szFile[8 + i]);
    }

    *reinterpret_cast<unsigned long long*>(szFile) = nSum;
    szFile[0] = static_cast<char>(static_cast<unsigned char>(nSum) + KEYFILE_CHECKSUM_SALT);

    // The tail is re-obfuscated with a different seed, leaving byte 0 (the salted checksum).
    unsigned long long nObf = KEYFILE_WRITE_SEED;
    for (int i = 1; i < kKeyFileRecordSize; i++) {
        nObf = nObf * kLcgMul + kLcgAdd;
        szFile[i] += static_cast<char>(nObf >> 16);
    }

    fwrite(szFile, 1, kKeyFileRecordSize, pFile);
}

void KeyFileDestroy(FILE* pFile)
{
    char szBuf[kKeyFileRecordSize];   // the on-disk record

    time_t nSeed = time(0);
    for (int i = 0; i < kKeyFileRecordSize; i++) {
        nSeed = nSeed * kLcgMul + kLcgAdd;
        szBuf[i] = static_cast<char>(nSeed >> 16);
    }

    fwrite(szBuf, 1, kKeyFileRecordSize, pFile);
}

bool KeyFileExpired(KeyFileData* pData, int nMonth, int nDay, int nYear)
{
    if (pData->m_date.m_nYear < nYear) {
        return true;
    }
    if (pData->m_date.m_nYear == nYear) {
        if (pData->m_date.m_nMonth < nMonth) {
            return true;
        }
        if (pData->m_date.m_nMonth == nMonth) {
            return pData->m_date.m_nDay <= nDay;
        }
    }
    return false;
}

void KeyFileGetCurrentTime(int* pnMonth, int* pnDay, int* pnYear)
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    *pnMonth = st.wMonth;
    *pnDay = st.wDay;
    *pnYear = st.wYear;
}

int KeyFileValidate(char* pszFileName, char* pszName, int nMinVersion, KeyFileData* pData)
{
    FILE* pFile = fopen(pszFileName, "rb");
    if (pFile == 0) {
        return 1;
    }

    int nRead = KeyFileRead(pFile, pData);

    SYSTEMTIME st;
    GetSystemTime(&st);
    int nMonth = st.wMonth;
    int nDay = st.wDay;
    int nYear = st.wYear;

    fclose(pFile);

    if (nRead == 0) {
        return 2;
    }
    if (KeyFileExpired(pData, nMonth, nDay, nYear)) {
        return 3;
    }
    if (pData->m_date.m_nVersion < nMinVersion) {
        return 4;
    }
    return _stricmp(pszName, pData->m_szName) == 0 ? 0 : 5;
}
