#include "k2stdint.h"
#include "resset.h"

#include <string.h>
#include "port/libcbase.h"
#include "port/libcstring.h"

// The GENERAL section's Type value of a set file, "SET", read as a little-endian word.
enum {
    SET_FILE_TYPE = 'S' | ('E' << 8) | ('T' << 16),
};


CResSET::CResSET()
{
    m_bLoaded = 0;
    m_nSETSize = 0;
    m_pSectionTable = 0;
    m_nSectionCount = 0;
}

CResSET::~CResSET()
{
}

int CResSET::CopySETData(unsigned char* pDst)
{
    if (!m_bLoaded)
        return 0;
    memcpy(pDst, m_pFileData, m_nSETSize);  // idiom-exempt: runtime length
    return 1;
}

void* CResSET::GetSETDataPtr()
{
    return m_pFileData;
}

int CResSET::GetSETSize()
{
    return m_nSETSize;
}

int CResSET::IsLoaded()
{
    return m_bLoaded;
}

// Parses the .set file, requiring the general section to declare Type="SET" and Version="V1.0"
// before the resource counts as loaded.
int CResSET::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_nSETSize = m_nSize;
    if (CreateSectionTable()) {
        char buf[256];
        GetSectionEntryValue(const_cast<char*>("GENERAL"), const_cast<char*>("Type"), buf);
        if (*reinterpret_cast<unsigned int*>(buf) == SET_FILE_TYPE) {
            GetSectionEntryValue(const_cast<char*>("GENERAL"), const_cast<char*>("Version"), buf);
            if (memcmp(buf, "V1.0", 5) == 0) {
                m_bLoaded = 1;
                return 1;
            }
        }
        m_nSETSize = 0;
    }
    return 0;
}

// Two-pass Windows-INI parser over the raw file buffer: pass 1 counts the [Section] headers, pass 2
// takes each section's name from its header line, counts its key=value lines, allocates that many
// SETEntry and parses each line into m_key/m_value. A parsed entry count that does not match the
// pre-count rejects the whole table.
int CResSET::CreateSectionTable()
{
    char buf[256];

    unsigned int sectionCount = 0;
    {
        unsigned int idx = 0;
        char* p = static_cast<char*>(m_pFileData);
        while (idx < m_nSETSize) {
            p = SkipWhiteSpace(p);
            if (*p == '[' && strchr(p, ']') != 0)
                sectionCount++;
            p = GetNextLine(idx, &idx);
        }
    }

    m_pSectionTable = new SETSection[sectionCount];

    unsigned int sectionIndex = 0;
    unsigned int idx = 0;
    char* p = static_cast<char*>(m_pFileData);
    while (idx < m_nSETSize) {
        p = SkipWhiteSpace(p);
        if (*p == '[') {
            char* pClose = strchr(p, ']');
            if (pClose != 0) {
                K2_ASSUME(sectionIndex < sectionCount);   // pass 1 counted these same header lines
                *pClose = 0;
                m_pSectionTable[sectionIndex].m_name = p + 1;
                *pClose = ']';

                // The count runs on its own index `j`, leaving `idx` on the header line for the
                // parse below.
                unsigned int nEntries = 0;
                unsigned int j = 0;
                char* q = GetNextLine(idx, &j);
                while (j < m_nSETSize) {
                    q = SkipWhiteSpace(q);
                    if (*q == '[')
                        break;
                    nEntries++;
                    q = GetNextLine(j, &j);
                }
                m_pSectionTable[sectionIndex].m_pEntries = new SETEntry[nEntries];

                p = GetNextLine(idx, &idx);
                unsigned int n = 0;
                while (idx < m_nSETSize) {
                    p = SkipWhiteSpace(p);
                    if (*p == '[')
                        break;

                    char* pNL = strchr(p, '\n');
                    char* pCR = strchr(p, '\r');
#ifdef K2_FIX_ENGINE_BUGS
                    bool bNL = (pNL != 0 && (pCR == 0 || pCR >= pNL));   // the nearer terminator
#else
                    bool bNL = (pNL != 0 && pCR >= pNL);
#endif
                    char* pEol = bNL ? pNL : pCR;
                    if (pEol != 0) {
                        *pEol = 0;
                        __strcpy_chk(buf, p, sizeof(buf));
                    } else {
                        __strncpy_chk(buf, p, m_nSETSize - idx, sizeof(buf));
                        buf[m_nSETSize - idx] = 0;
                    }

                    char* pEq = __strchr_chk(buf, '=', sizeof(buf));
                    if (pEq == 0) {
                        p = GetNextLine(idx, &idx);
                        continue;
                    }

                    *pEq = 0;
                    char* pKeyEnd = pEq;
                    while (pKeyEnd[-1] == ' ' || pKeyEnd[-1] == '\t')
                        pKeyEnd--;
                    char* pKey = buf;
                    while (*pKey == ' ' || *pKey == '\t')
                        pKey++;
                    K2_ASSUME(n < nEntries);   // n counts a subset of the lines nEntries counted
                    m_pSectionTable[sectionIndex].m_pEntries[n].m_key = pKey;

                    if (pEol != 0)
                        __strcpy_chk(buf, p, sizeof(buf));
                    else
                        __strncpy_chk(buf, p, m_nSETSize - idx, sizeof(buf));

                    char* pTrail = buf;
                    pTrail += __strlen_chk(buf, sizeof(buf));
                    while (pTrail[-1] == ' ' || pTrail[-1] == '\t')
                        pTrail--;
                    char* pVal = __strchr_chk(buf, '=', sizeof(buf)) + 1;
                    while (*pVal == ' ' || *pVal == '\t')
                        pVal++;
                    m_pSectionTable[sectionIndex].m_pEntries[n].m_value = pVal;
                    n++;

                    if (pEol != 0)
                        *pEol = bNL ? '\n' : '\r';

                    p = GetNextLine(idx, &idx);
                }

                if (n != nEntries)
                    return 0;
                m_pSectionTable[sectionIndex].m_nEntries = nEntries;
                sectionIndex++;
                continue;
            }
        }
        p = GetNextLine(idx, &idx);
    }

    if (sectionIndex != sectionCount)
        return 0;
    m_nSectionCount = sectionCount;
    return 1;
}

int CResSET::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nSETSize = 0;
    if (m_pSectionTable) {
        for (unsigned int i = 0; i < m_nSectionCount; i++)
            delete[] m_pSectionTable[i].m_pEntries;
        delete[] m_pSectionTable;
        m_pSectionTable = 0;
    }
    m_nSectionCount = 0;
    return 1;
}

// Copies the value of `key` in `sectionName` into `out`, or an empty string on a miss.
void CResSET::GetSectionEntryValue(char* sectionName, char* key, char* out)
{
    if (m_nSectionCount == 0) {
        *out = 0;
        return;
    }
    unsigned int i = 0;
    while (i < m_nSectionCount) {
        if (m_pSectionTable[i].m_name == sectionName)
            break;
        i++;
    }
    if (i == m_nSectionCount) {
        *out = 0;
        return;
    }
    unsigned int sectionIndex = i;
    i = 0;
    while (i < static_cast<unsigned>(m_pSectionTable[sectionIndex].m_nEntries)) {
        if (m_pSectionTable[sectionIndex].m_pEntries[i].m_key == key) {
            strcpy(out, m_pSectionTable[sectionIndex].m_pEntries[i].m_value.CStr());
            break;
        }
        i++;
    }
    if (i == static_cast<unsigned>(m_pSectionTable[sectionIndex].m_nEntries))
        *out = 0;
}

// EFFECTIVE (K1): ours folds the latch's `add` into a pre-indexed `ldrb [x0, #1]!` (the loop test is
// tail-duplicated into the latch); K1 keeps `add` + `ldrb`. Same loads and tests, armv7 -O3 exact.
char* CResSET::SkipWhiteSpace(char* p)
{
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

// Returns the start of the line following the end-of-line run at `start`, storing the index just
// past it in *pEnd.
char* CResSET::GetNextLine(unsigned int start, unsigned int* pEnd)
{
    unsigned int size = m_nSETSize;
    char* data = static_cast<char*>(m_pFileData);
    unsigned int idx = start;
    char* p = data + idx;
    while (idx < size) {
        if (*p == '\n' || *p == '\r')
            break;
        idx++;
        p++;
    }
    while (idx < size && (*p == '\r' || *p == '\n')) {
        idx++;
        p++;
    }
    *pEnd = idx;
    return p;
}
