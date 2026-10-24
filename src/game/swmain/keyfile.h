#ifndef KOTOR2_KEYFILE_H
#define KOTOR2_KEYFILE_H

// Obfuscated copy-protection key file: one on-disk record, obscured by a
// pseudo-random byte stream.

#include "port/libcbase.h"

struct KeyFileDate {
    int  m_nMonth;        // expiry date, compared year-major by KeyFileExpired
    int  m_nDay;
    int  m_nYear;
    int  m_nVersion;        // compared against KeyFileValidate's third argument
};

struct KeyFileData {
    KeyFileDate m_date;
    char m_szName[16];    // user name, matched case-insensitively by KeyFileValidate
    char m_szKey[988];    // payload string
};

// Reads one record into *pData. Returns 1 on success, 0 when the file is short,
// the checksum fails, or the payload length is wrong.
int  KeyFileRead(FILE* pFile, KeyFileData* pData);

// Writes *pData as one record, re-padding the tail with a fresh time-seeded stream.
void KeyFileWrite(FILE* pFile, KeyFileData* pData);

// Writes a record of pure pseudo-random bytes, the destroy / no-key-yet state.
void KeyFileDestroy(FILE* pFile);

// True when the record's date is before (nYear, nMonth, nDay), or that day.
bool KeyFileExpired(KeyFileData* pData, int nMonth, int nDay, int nYear);

// Today's date from GetSystemTime, in the order KeyFileExpired takes.
void KeyFileGetCurrentTime(int* pnMonth, int* pnDay, int* pnYear);

// Opens pszFileName, reads a record and checks it. Returns 0 when valid, 1 when the
// file cannot be opened, 2 when the read fails, 3 when expired, 4 when
// pData->m_nVersion < nMinVersion, 5 on a name mismatch.
int  KeyFileValidate(char* pszFileName, char* pszName, int nMinVersion, KeyFileData* pData);

#endif  // KOTOR2_KEYFILE_H
