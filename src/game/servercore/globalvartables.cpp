// The global script-variable table is a hash table over variable names plus four typed value
// arrays, each with its own count. Every accessor resolves the name through FindIdentifierIndex and
// checks the entry's two-bit type; the failure paths build diagnostics that are then discarded.
#include "k2stdint.h"
#include "globalvartables.h"
#include "resgff.h"
#include "exoresman.h"
#include "twodimarray.h"
#include "crc32poly.h"

// GFF load-error reporter, defined in another translation unit. Takes an unsigned long strref, not a
// short.
void HandleLoadError(uint64_t strRef);

#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "strref.h"
#include "exostringinline.h"

// Builds the CRC-32 table the hash walks, then clears the identifier table and the values.
CSWGlobalVariableTable::CSWGlobalVariableTable()
{
    for (int i = 0; i < 256; i++)
    {
        unsigned int crc = static_cast<unsigned int>(i);
        for (int k = 0; k < 8; k++)
            crc = (crc & 1) ? ((crc >> 1) ^ CRC32_POLYNOMIAL) : (crc >> 1);
        m_aHashTable[i] = crc;
    }
#if KOTOR1  // K1 resets through ClearIdentifiers
    ClearIdentifiers();
#else
    __builtin_memset(m_aIdentifiers, 0, sizeof(m_aIdentifiers));
    m_nCollisions = 0;
    m_nProbes = 0;
    m_nMaxProbes = 0;
    m_nBooleanCount = 0;
    m_nNumberCount = 0;
    m_nLocationCount = 0;
    m_nStringCount = 0;
    ClearValues();
#endif
}

// Clears the identifier table and every counter and statistic, then the values.
#if KOTOR1  // K1 calls this from the constructor's tail, so it clears the values too
__attribute__((noinline))
#endif
void CSWGlobalVariableTable::ClearIdentifiers()
{
    __builtin_memset(m_aIdentifiers, 0, sizeof(m_aIdentifiers));
    m_nCollisions = 0;
    m_nProbes = 0;
    m_nMaxProbes = 0;
    m_nBooleanCount = 0;
    m_nNumberCount = 0;
    m_nLocationCount = 0;
    m_nStringCount = 0;
    ClearValues();
}

CSWGlobalVariableTable::~CSWGlobalVariableTable()
{
}

// Zeroes the four value arrays.
// EFFECTIVE: the boolean and number zeroings are adjacent, so the backend merges them into one
// 1113-byte memset where the original makes the number array its own call.
#if !KOTOR1  // K1 inlines this into ClearIdentifiers
__attribute__((noinline))
#endif
void CSWGlobalVariableTable::ClearValues()
{
    __builtin_memset(m_aBooleanBits, 0, sizeof(m_aBooleanBits));
    __builtin_memset(m_aNumbers, 0, sizeof(m_aNumbers));
    __builtin_memset(m_aLocations, 0, sizeof(m_aLocations));
    __builtin_memset(m_aStrings, 0, sizeof(m_aStrings));
}

// Hashes the name with a CRC-32 walk that case-folds each character, xor's it into the low byte of
// the running value, then replaces the value with the table entry and shifts right by 8. The result
// is reduced modulo the table size, so the identifier array and the hash modulus agree.
unsigned int CSWGlobalVariableTable::Hash(const CExoString& sName)
{
    unsigned int nHash = 0;
#if KOTOR1  // K1 calls the out-of-line CExoString::GetLength
    for (int i = 0; i < sName.GetLength(); i++)
#else
    for (int i = 0; i < static_cast<int>(sName.m_sString ? strlen(sName.m_sString) : 0); i++)
#endif
        nHash = m_aHashTable[(toupper(sName[i]) ^ nHash) & 0xff] ^ (nHash >> 8);
    return nHash % NUM_IDENTIFIERS;
}

// Open-addressed lookup. CompareNoCase returns non-zero when the strings are equal, so the walk runs
// while the slot is occupied and the name differs. Returns -1 when the name is not in the table.
int CSWGlobalVariableTable::FindIdentifierIndex(const CExoString& sName)
{
    unsigned int nHash = Hash(sName) % NUM_IDENTIFIERS;
    int nIndex = static_cast<int>(nHash);
    if (m_aIdentifiers[nIndex].m_szName[0] != 0 &&
        sName.CompareNoCase(m_aIdentifiers[nIndex].m_szName) == 0)
    {
        for (;;)
        {
            nIndex = (nIndex + 1) % NUM_IDENTIFIERS;
            if (nHash == static_cast<unsigned int>(nIndex))
                break;
            if (m_aIdentifiers[nIndex].m_szName[0] == 0)
                break;
            if (sName.CompareNoCase(m_aIdentifiers[nIndex].m_szName) != 0)
                break;
        }
    }
    return sName.CompareNoCase(m_aIdentifiers[nIndex].m_szName) ? nIndex : -1;
}

// Adds an entry and returns its slot. There is no duplicate check: an occupied primary slot turns this
// into a linear probe for the first free slot, which is how two entries can end up sharing a name. The
// statistics are only maintained on that path, and `probes` counts every step including the one that
// wraps back to the primary slot (the table-full case, where the primary slot is overwritten).
int CSWGlobalVariableTable::AddIdentifier(const CExoString& sName, unsigned int nType,
                                          unsigned int nIndex)
{
    unsigned int nHash = Hash(sName) % NUM_IDENTIFIERS;
    int nSlot = static_cast<int>(nHash);
    int nProbe = 0;
    if (m_aIdentifiers[nSlot].m_szName[0] != 0)
    {
        int nNext = (nSlot + 1) % NUM_IDENTIFIERS;
        m_nCollisions++;
        nProbe = 1;
        if (nNext != nSlot)
        {
            for (;;)
            {
                if (m_aIdentifiers[nNext].m_szName[0] == 0)
                {
                    nSlot = nNext;
                    break;
                }
                nNext = (nNext + 1) % NUM_IDENTIFIERS;
                nProbe++;
                if (nSlot == nNext)
                    break;
            }
        }
        m_nProbes += nProbe;
        if (nProbe > m_nMaxProbes)
            m_nMaxProbes = nProbe;
    }
    __builtin___strcpy_chk(m_aIdentifiers[nSlot].m_szName, sName.CStr(),
                           sizeof(m_aIdentifiers[nSlot].m_szName));
    m_aIdentifiers[nSlot].m_nIndex = nIndex;
    m_aIdentifiers[nSlot].m_nType = nType;
    return nSlot;
}

// The four typed wrappers each hand their own count over as the slot to use and bump that count
// when the add succeeded. The bounds are the array sizes in the header.
int CSWGlobalVariableTable::AddIdentifierBoolean(const CExoString& sName)
{
    if (m_nBooleanCount <= 899)
    {
        int nSlot = AddIdentifier(sName, GVT_BOOLEAN, m_nBooleanCount);
        m_nBooleanCount++;
        return nSlot;
    }
    return -1;
}

int CSWGlobalVariableTable::AddIdentifierNumber(const CExoString& sName)
{
    if (m_nNumberCount < NUM_NUMBERS)
    {
        int nSlot = AddIdentifier(sName, GVT_NUMBER, m_nNumberCount);
        m_nNumberCount++;
        return nSlot;
    }
    return -1;
}

int CSWGlobalVariableTable::AddIdentifierLocation(const CExoString& sName)
{
    if (m_nLocationCount <= 99)
    {
        int nSlot = AddIdentifier(sName, GVT_LOCATION, m_nLocationCount);
        m_nLocationCount++;
        return nSlot;
    }
    return -1;
}

int CSWGlobalVariableTable::AddIdentifierString(const CExoString& sName)
{
    if (m_nStringCount <= 4)
    {
        int nSlot = AddIdentifier(sName, GVT_STRING, m_nStringCount);
        m_nStringCount++;
        return nSlot;
    }
    return -1;
}

// Reads one boolean out of a packed bit array. Bit 7 of each byte is bit 0 of the range, so the bit
// order is most-significant-first. Non-static so the array pointer can arrive as an argument.
int CSWGlobalVariableTable::GetBit(unsigned char* pBits, int nBit)
{
    int nByte = nBit / 8;
    int nRem = nBit - nByte * 8;
    return (pBits[nByte] & (1 << (7 - nRem))) != 0;
}

// Writes one boolean into a packed bit array, same bit order as GetBit.
void CSWGlobalVariableTable::SetBit(unsigned char* pBits, int nBit, int nValue)
{
    int nByte = nBit / 8;
    int nRem = nBit - nByte * 8;
    unsigned int nMask = 1u << (7 - nRem);
    unsigned char* pByte = &pBits[nByte];
    unsigned char nSet   = static_cast<unsigned char>(*pByte | nMask);
    unsigned char nClear = *pByte & ~nMask;
    *pByte = nValue ? nSet : nClear;
}

// The typed accessors all have one shape: resolve the identifier, gate on its two-bit type, then use
// the value array or build the discarded diagnostic.
void CSWGlobalVariableTable::GetValueBoolean(const CExoString& sName, int& nValue)
{
    int nIndex = FindIdentifierIndex(sName);
    nValue = 0;
    if (nIndex < 0)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var BOOLEAN '%s' not in catalogue!"), sName.CStr());
    }
    else if (m_aIdentifiers[nIndex].m_nType != GVT_BOOLEAN)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var '%s' is not BOOLEAN!"), sName.CStr());
    }
    else
    {
        nValue = GetBit(m_aBooleanBits, m_aIdentifiers[nIndex].m_nIndex);
    }
}

void CSWGlobalVariableTable::GetValueNumber(const CExoString& sName, char& nValue)
{
    int nIndex = FindIdentifierIndex(sName);
    nValue = 0;
    if (nIndex < 0)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var NUMBER '%s' not in catalogue!"), sName.CStr());
    }
    else if (m_aIdentifiers[nIndex].m_nType != GVT_NUMBER)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var '%s' not a NUMBER!"), sName.CStr());
    }
    else
    {
        nValue = m_aNumbers[m_aIdentifiers[nIndex].m_nIndex];
    }
}

// Leaves the out-parameter untouched when the lookup fails.
void CSWGlobalVariableTable::GetValueLocation(const CExoString& sName, CScriptLocation& out)
{
    int nIndex = FindIdentifierIndex(sName);
    if (nIndex < 0)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var LOCATION '%s' not in catalogue!"), sName.CStr());
    }
    else if (m_aIdentifiers[nIndex].m_nType != GVT_LOCATION)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var '%s' not a LOCATION!"), sName.CStr());
    }
    else
    {
        out = m_aLocations[m_aIdentifiers[nIndex].m_nIndex];
    }
}

void CSWGlobalVariableTable::GetValueString(const CExoString& sName, CExoString& out)
{
    int nIndex = FindIdentifierIndex(sName);
    if (nIndex < 0)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var STRING '%s' not in catalogue!"), sName.CStr());
    }
    else if (m_aIdentifiers[nIndex].m_nType != GVT_STRING)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var '%s' not a STRING!"), sName.CStr());
    }
    else
    {
        out = m_aStrings[m_aIdentifiers[nIndex].m_nIndex];
    }
}

void CSWGlobalVariableTable::SetValueBoolean(const CExoString& sName, int nValue)
{
    int nIndex = FindIdentifierIndex(sName);
    if (nIndex < 0)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var BOOLEAN '%s' not in catalogue!"), sName.CStr());
    }
    else if (m_aIdentifiers[nIndex].m_nType != GVT_BOOLEAN)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var '%s' not a BOOLEAN!"), sName.CStr());
    }
    else
    {
        SetBit(m_aBooleanBits, m_aIdentifiers[nIndex].m_nIndex, nValue);
    }
}

void CSWGlobalVariableTable::SetValueNumber(const CExoString& sName, char nValue)
{
    int nIndex = FindIdentifierIndex(sName);
    if (nIndex < 0)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var NUMBER '%s' not in catalogue!"), sName.CStr());
    }
    else if (m_aIdentifiers[nIndex].m_nType != GVT_NUMBER)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var '%s' not a NUMBER!"), sName.CStr());
    }
    else
    {
        m_aNumbers[m_aIdentifiers[nIndex].m_nIndex] = nValue;
    }
}

void CSWGlobalVariableTable::SetValueLocation(const CExoString& sName, const CScriptLocation& value)
{
    int nIndex = FindIdentifierIndex(sName);
    if (nIndex < 0)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var LOCATION '%s' not in catalogue!"), sName.CStr());
    }
    else if (m_aIdentifiers[nIndex].m_nType != GVT_LOCATION)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var '%s' not a LOCATION!"), sName.CStr());
    }
    else
    {
        m_aLocations[m_aIdentifiers[nIndex].m_nIndex] = value;
    }
}

void CSWGlobalVariableTable::SetValueString(const CExoString& sName, const CExoString& value)
{
    int nIndex = FindIdentifierIndex(sName);
    if (nIndex < 0)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var STRING '%s' not in catalogue!"), sName.CStr());
    }
    else if (m_aIdentifiers[nIndex].m_nType != GVT_STRING)
    {
        CExoString sError;
        sError.Format(const_cast<char*>("Script var '%s' not a STRING!"), sName.CStr());
    }
    else
    {
        m_aStrings[m_aIdentifiers[nIndex].m_nIndex] = value;
    }
}

// Reads back the hash statistics. The identifier total sums the three non-string counts, so strings
// are deliberately excluded, and the average is the total probe count over that total.
void CSWGlobalVariableTable::GetPerformanceInfo(int& nIdentifiers, int& nCollisions,
                                                int& nAvgProbes, int& nMaxProbes)
{
    nIdentifiers = m_nBooleanCount + m_nNumberCount + m_nLocationCount;
    nCollisions = m_nCollisions;
    nMaxProbes = m_nMaxProbes;
    nAvgProbes = m_nProbes / nIdentifiers;
}

// Seeds the identifier table from globalcat.2da, which lists each script variable's name and type. The
// .2da also carries a per-type slot column, which this pass ignores: it appends in row order and lets
// AddIdentifier assign the slot.
//
// CompareNoCase is a boolean-equal here, not a strcmp: it returns non-zero when the two strings are
// equal, so the type chain tests it directly. The diagnostics are built and then discarded.
void CSWGlobalVariableTable::ReadCatalogue(const CResRef& resRef)
{
    C2DA cat(resRef, 1);
    CExoString sName;
    CExoString sType;
    if (!cat.Load2DArray())
        return;
    if (cat.m_nRows < 1)
        return;

    for (int i = 0; i < cat.m_nRows; i++)
    {
        if (!cat.GetCExoStringEntry(i, CExoString("Name"), &sName))
            continue;
        if (sName.GetLength() > 21)
        {
            CExoString sMsg;
            sMsg.Format(const_cast<char*>("Script var '%s' longer than %d letters!"), sName.CStr(), 21);
            continue;
        }
        if (!cat.GetCExoStringEntry(i, CExoString("Type"), &sType))
            continue;
        int nSlot;
        if (FindIdentifierIndex(sName) >= 0)
        {
            CExoString sMsg;
            sMsg.Format(const_cast<char*>("Script var '%s' duplicated in catalogue!"), sName.CStr());
            continue;
        }

        if (sType.CompareNoCase("BOOLEAN"))
            nSlot = AddIdentifierBoolean(sName);
        else if (sType.CompareNoCase("NUMBER"))
            nSlot = AddIdentifierNumber(sName);
        else if (sType.CompareNoCase("LOCATION"))
            nSlot = AddIdentifierLocation(sName);
        else if (sType.CompareNoCase("STRING"))
            nSlot = AddIdentifierString(sName);
        else
        {
            CExoString sMsg;
            sMsg.Format(const_cast<char*>("Script var '%s' has bad type in catalogue!"), sName.CStr());
            continue;
        }

        if (nSlot < 0)
        {
            CExoString sMsg;
            sMsg.Format(const_cast<char*>("Script var '%s' '%s' won't fit in table!"), sType.CStr(), sName.CStr());
        }
    }
}

// GFF loader for a table with a catalogue list: globalcat.2da says which script variables exist and
// what type each is, so this pass seeds the identifier table from the /Catalogue/ GFF and then copies
// the per-type value arrays in. ReadTableWithoutCatalogue finds the identifiers the other way, by
// probing each value list for a matching name.
void CSWGlobalVariableTable::ReadTableWithCatalogue(CResGFF* pGFF, CResStruct& sTop)
{
    unsigned char   aBooleans[NUM_BOOLEAN_BYTES];
    char            aNumbers[NUM_NUMBERS];

    CScriptLocation aLocations[NUM_LOCATIONS];
    CExoString      aStrings[5];
    CExoString      sName;
    unsigned char   nDefault = 0;

    ClearValues();

#if KOTOR1  // K1 expands the two local zeroings inline (this TU's K1 build is -fno-builtin)
    __builtin_memset(aBooleans, 0, sizeof(aBooleans));
    __builtin_memset(aNumbers, 0, sizeof(aNumbers));
#else
    memset(aBooleans, 0, sizeof(aBooleans));
    memset(aNumbers, 0, sizeof(aNumbers));
#endif

    CResStruct sStruct;
    CResStruct sStringStruct;
    CResList   list1;
    CResList   list2;
    int        bSuccess;
    int        nSlot;

    pGFF->ReadFieldVOID(&sTop, aBooleans, sizeof(aBooleans), const_cast<char*>("ValBoolean"), bSuccess, &nDefault);
    if (bSuccess)
    {
        pGFF->GetList(&list1, &sTop, const_cast<char*>("CatBoolean"));
        for (int i = 0; i < static_cast<int>(pGFF->GetListCount(&list1)); i++)
        {
            pGFF->GetListElement(&sStruct, &list1, i);
            sName = pGFF->ReadFieldCExoString(&sStruct, const_cast<char*>("Name"), bSuccess,
                                              CExoString(""));
            if (bSuccess && sName != "")
            {
                nSlot = FindIdentifierIndex(sName);
                if (nSlot < 0)
                {
                    nSlot = AddIdentifierBoolean(sName);
                    if (nSlot < 0)
                    {
                        CExoString sMsg;
                        sMsg.Format(const_cast<char*>("ReadTableWithCat(): BOOLEAN '%s' won't fit!"), sName.CStr());
#ifdef K2_FIX_ENGINE_BUGS
                        continue;
#endif
                    }
                }
                SetBit(m_aBooleanBits, m_aIdentifiers[nSlot].m_nIndex, GetBit(aBooleans, i));
            }
        }
    }

    pGFF->ReadFieldVOID(&sTop, aNumbers, sizeof(aNumbers), const_cast<char*>("ValNumber"), bSuccess, &nDefault);
    if (bSuccess)
    {
        pGFF->GetList(&list1, &sTop, const_cast<char*>("CatNumber"));
        for (int i = 0; i < static_cast<int>(pGFF->GetListCount(&list1)); i++)
        {
            pGFF->GetListElement(&sStruct, &list1, i);
            sName = pGFF->ReadFieldCExoString(&sStruct, const_cast<char*>("Name"), bSuccess,
                                              CExoString(""));
            if (bSuccess && sName != "")
            {
                nSlot = FindIdentifierIndex(sName);
                if (nSlot < 0)
                {
                    nSlot = AddIdentifierNumber(sName);
                    if (nSlot < 0)
                    {
                        CExoString sMsg;
                        sMsg.Format(const_cast<char*>("ReadTableWithCat(): NUMBER '%s' won't fit!"), sName.CStr());
#ifdef K2_FIX_ENGINE_BUGS
                        continue;
#endif
                    }
                }
                m_aNumbers[m_aIdentifiers[nSlot].m_nIndex] = aNumbers[i];
            }
        }
    }

    pGFF->ReadFieldVOID(&sTop, aLocations, sizeof(aLocations), const_cast<char*>("ValLocation"), bSuccess, &nDefault);
    if (bSuccess)
    {
        pGFF->GetList(&list1, &sTop, const_cast<char*>("CatLocation"));
        for (int i = 0; i < static_cast<int>(pGFF->GetListCount(&list1)); i++)
        {
            pGFF->GetListElement(&sStruct, &list1, i);
            sName = pGFF->ReadFieldCExoString(&sStruct, const_cast<char*>("Name"), bSuccess,
                                              CExoString(""));
            if (bSuccess && sName != "")
            {
                nSlot = FindIdentifierIndex(sName);
                if (nSlot < 0)
                {
                    nSlot = AddIdentifierLocation(sName);
                    if (nSlot < 0)
                    {
                        CExoString sMsg;
                        sMsg.Format(const_cast<char*>("ReadTableWithCat(): LOCATION '%s' won't fit!"), sName.CStr());
#ifdef K2_FIX_ENGINE_BUGS
                        continue;
#endif
                    }
                }
                m_aLocations[m_aIdentifiers[nSlot].m_nIndex] = aLocations[i];
            }
        }
    }

    if (pGFF->GetList(&list2, &sTop, const_cast<char*>("ValString")))
    {
#if KOTOR1  // K1 reads the string count once
        unsigned int nCount = pGFF->GetListCount(&list2);
        if (nCount > 5)
            nCount = 5;
        if (static_cast<int>(nCount) >= 1)
        {
            for (unsigned int i = 0; i < nCount; i++)
            {
                pGFF->GetListElement(&sStringStruct, &list2, i);
                aStrings[i] = pGFF->ReadFieldCExoString(&sStringStruct, const_cast<char*>("String"),
                                                        bSuccess, CExoString(""));
            }
        }
#else
        int nCount = pGFF->GetListCount(&list2) > 5 ? 5 : pGFF->GetListCount(&list2);
        if (nCount >= 1)
        {
            for (int i = 0; i < nCount; i++)
            {
                pGFF->GetListElement(&sStringStruct, &list2, i);
                aStrings[i] = pGFF->ReadFieldCExoString(&sStringStruct, const_cast<char*>("String"),
                                                        bSuccess, CExoString(""));
            }
        }
#endif

        pGFF->GetList(&list1, &sTop, const_cast<char*>("CatString"));
        for (int i = 0; i < static_cast<int>(pGFF->GetListCount(&list1)); i++)
        {
            pGFF->GetListElement(&sStruct, &list1, i);
            sName = pGFF->ReadFieldCExoString(&sStruct, const_cast<char*>("Name"), bSuccess,
                                              CExoString(""));
            if (bSuccess && sName != "")
            {
                nSlot = FindIdentifierIndex(sName);
                if (nSlot < 0)
                {
                    nSlot = AddIdentifierString(sName);
                    if (nSlot < 0)
                    {
                        CExoString sMsg;
                        sMsg.Format(const_cast<char*>("ReadTableWithCat(): STRING '%s' won't fit!"), sName.CStr());
#ifdef K2_FIX_ENGINE_BUGS
                        continue;
#endif
                    }
                }
#ifdef K2_FIX_ENGINE_BUGS
                if (i >= 5)
                    continue;
#endif
                m_aStrings[m_aIdentifiers[nSlot].m_nIndex] = aStrings[i];
            }
        }
    }
}


// Writes the table as GFF: builds a V2.0 GVT, walks the identifier table once filling the four
// positional arrays from the typed value arrays, writes the three raw value blobs, then the
// CatString list back out of the strings. bBinary gates both the four list creations and the
// per-element name field; with it zero only the three raw value blobs are written.
//
// sPath is only used for the alias comparison against the reboot-data table; the GFF write gets the
// full file name.
void CSWGlobalVariableTable::WriteTable(const CExoString& sFileName, int bBinary)
{
    unsigned char aBooleans[NUM_BOOLEAN_BYTES];
    char          aNumbers[NUM_NUMBERS];

    CScriptLocation aLocations[NUM_LOCATIONS];
    CExoString      aStrings[5];
    CExoString      sPath;

    int nPos = sFileName.Find(CExoString(":"), 0);
    if (nPos != -1)
        sPath = sFileName.Left(nPos + 1);

    CResGFF* pGFF = new CResGFF();
    CResStruct topStruct;
    pGFF->CreateGFFFile(&topStruct, CExoString("GVT "), CExoString("V2.0"));

    CResList listBoolean;
    CResList listNumber;
    CResList listLocation;
    CResList listString;
    if (bBinary)
    {
        pGFF->AddList(&listBoolean,  &topStruct, const_cast<char*>("CatBoolean"));
        pGFF->AddList(&listNumber,   &topStruct, const_cast<char*>("CatNumber"));
        pGFF->AddList(&listLocation, &topStruct, const_cast<char*>("CatLocation"));
        pGFF->AddList(&listString,   &topStruct, const_cast<char*>("CatString"));
    }

#if KOTOR1  // K1 expands the two local zeroings inline (this TU's K1 build is -fno-builtin)
    __builtin_memset(aBooleans, 0, sizeof(aBooleans));
    __builtin_memset(aNumbers, 0, sizeof(aNumbers));
#else
    memset(aBooleans, 0, sizeof(aBooleans));
    memset(aNumbers, 0, sizeof(aNumbers));
#endif

    CResStruct sElem;
    int nBoolean  = 0;
    int nNumber   = 0;
    int nLocation = 0;
    int nString   = 0;
    for (int i = 0; i < NUM_IDENTIFIERS; i++)
    {
        if (m_aIdentifiers[i].m_szName[0] == 0)
            continue;

        CResList* pList = 0;
        switch (m_aIdentifiers[i].m_nType)
        {
        case GVT_BOOLEAN:
            SetBit(aBooleans, nBoolean++, GetBit(m_aBooleanBits, m_aIdentifiers[i].m_nIndex));
            pList = &listBoolean;
            break;
        case GVT_NUMBER:
            aNumbers[nNumber++] = m_aNumbers[m_aIdentifiers[i].m_nIndex];
            pList = &listNumber;
            break;
        case GVT_LOCATION:
            aLocations[nLocation++] = m_aLocations[m_aIdentifiers[i].m_nIndex];
            pList = &listLocation;
            break;
        case GVT_STRING:
            aStrings[nString++] = m_aStrings[m_aIdentifiers[i].m_nIndex];
            pList = &listString;
            break;
        }

        if (bBinary)
        {
            pGFF->AddListElement(&sElem, pList, 0);
            pGFF->WriteFieldCExoString(&sElem, CExoString(m_aIdentifiers[i].m_szName),
                                       const_cast<char*>("Name"));
        }
    }

    pGFF->WriteFieldVOID(&topStruct, aBooleans, m_nBooleanCount / 8 + 1, const_cast<char*>("ValBoolean"));
    pGFF->WriteFieldVOID(&topStruct, aNumbers, m_nNumberCount, const_cast<char*>("ValNumber"));
    pGFF->WriteFieldVOID(&topStruct, aLocations, sizeof(aLocations), const_cast<char*>("ValLocation"));

    CResList listStringOut;
    pGFF->AddList(&listStringOut, &topStruct, const_cast<char*>("ValString"));
    if (nString >= 1)
    {
        for (int i = 0; i < nString; i++)
        {
            pGFF->AddListElement(&sElem, &listStringOut, 0);
            pGFF->WriteFieldCExoString(&sElem, aStrings[i], const_cast<char*>("String"));
        }
    }

    pGFF->WriteGFFFile(sFileName, 0);
    delete pGFF;

    if (sPath == "REBOOTDATA:")
        g_pExoResMan->CreateSignatureFile(sFileName, 0, 0, 0);
}

// Loads a GFF table that has no catalogue list. The value arrays are positional, so this pass reads
// the three bulk value fields, prefetches up to five strings, then walks the identifier table once
// and dispatches on each occupied slot's type. It shares its prologue with ReadTableWithCatalogue
// but does not call ClearValues, since the walk overwrites every occupied slot.
//
// The string counter doubles as the pre-read bound: it starts at the number of strings just read (or
// zero when the list is absent) and the walk continues from there, so a string slot consumes
// aStrings[1] after one string was read.
void CSWGlobalVariableTable::ReadTableWithoutCatalogue(CResGFF* pGFF, CResStruct& sTop)
{
    unsigned char   aBooleans[NUM_BOOLEAN_BYTES];
    char            aNumbers[NUM_NUMBERS];

    CScriptLocation aLocations[NUM_LOCATIONS];
    CExoString      aStrings[5];
    unsigned char   nDefault = 0;

#if KOTOR1  // K1 expands the two local zeroings inline (this TU's K1 build is -fno-builtin)
    __builtin_memset(aBooleans, 0, sizeof(aBooleans));
    __builtin_memset(aNumbers, 0, sizeof(aNumbers));
#else
    memset(aBooleans, 0, sizeof(aBooleans));
    memset(aNumbers, 0, sizeof(aNumbers));
#endif

    CResStruct sStruct;
    CResList   list1;
    int        bSuccess;

    pGFF->ReadFieldVOID(&sTop, aBooleans, sizeof(aBooleans), const_cast<char*>("ValBoolean"), bSuccess, &nDefault);
    pGFF->ReadFieldVOID(&sTop, aNumbers, sizeof(aNumbers), const_cast<char*>("ValNumber"), bSuccess, &nDefault);
    pGFF->ReadFieldVOID(&sTop, aLocations, sizeof(aLocations), const_cast<char*>("ValLocation"), bSuccess, &nDefault);

    int nString = 0;
    if (pGFF->GetList(&list1, &sTop, const_cast<char*>("ValString")))
    {
#if KOTOR1  // K1 reads the string count once
        nString = pGFF->GetListCount(&list1);
        if (static_cast<unsigned int>(nString) > 5)
            nString = 5;
#else
        nString = pGFF->GetListCount(&list1) > 5 ? 5 : pGFF->GetListCount(&list1);
#endif
        if (nString >= 1)
        {
            for (int i = 0; i < nString; i++)
            {
                pGFF->GetListElement(&sStruct, &list1, i);
                aStrings[i] = pGFF->ReadFieldCExoString(&sStruct, const_cast<char*>("String"), bSuccess,
                                                        CExoString(""));
            }
        }
    }

    int nBoolean  = 0;
    int nNumber   = 0;
    int nLocation = 0;
#ifdef K2_FIX_ENGINE_BUGS
    nString = 0;
#endif
    for (int i = 0; i < NUM_IDENTIFIERS; i++)
    {
        if (m_aIdentifiers[i].m_szName[0] != 0)
        {
            switch (m_aIdentifiers[i].m_nType)
            {
            case GVT_BOOLEAN:
                SetBit(m_aBooleanBits, m_aIdentifiers[i].m_nIndex, GetBit(aBooleans, nBoolean++));
                break;
            case GVT_NUMBER:
                m_aNumbers[m_aIdentifiers[i].m_nIndex] = aNumbers[nNumber++];
                break;
            case GVT_LOCATION:
                m_aLocations[m_aIdentifiers[i].m_nIndex] = aLocations[nLocation++];
                break;
            case GVT_STRING:
#ifdef K2_FIX_ENGINE_BUGS
                if (nString >= 5)
                    break;
#endif
                m_aStrings[m_aIdentifiers[i].m_nIndex] = aStrings[nString++];
                break;
            }
        }
    }
}

// Backs Load: allocates a GFF object over the resref, screens it on m_nValid, then picks the reader
// by whether the catalogue list is present.
int CSWGlobalVariableTable::ReadTable(const CExoString& sFileName)
{
    CResStruct topStruct;
    CResList list1;
    CResGFF* pGff = new CResGFF(0, const_cast<char*>("GVT "), CResRef(sFileName));
    if (pGff->m_nValid == 0)
    {
        HandleLoadError(STRREF_UNABLE_TO_LOAD);
        delete pGff;
        return 0;
    }

    pGff->GetTopLevelStruct(&topStruct);

    if (pGff->GetList(&list1, &topStruct, const_cast<char*>("CatBoolean")))
        ReadTableWithCatalogue(pGff, topStruct);
    else
        ReadTableWithoutCatalogue(pGff, topStruct);
    delete pGff;
    return 1;
}

// Self test that fills the table with 900 booleans, 1000 numbers, 100 locations and 5 strings,
// writes it to a save, reads it back and reads every value out again.
void CSWGlobalVariableTable::RegressionTest()
{
    CScriptLocation aLocations[NUM_LOCATIONS];
    CExoString      aStrings[5];
    CExoString      sName;
    CScriptLocation loc;
    CExoString      sValue;
    int             nBooleanValue;
    char            nNumberValue;

    ClearIdentifiers();

    for (int i = 0; i < 900; i++)
    {
        sName.Format(const_cast<char*>("b%04d"), i);
        AddIdentifierBoolean(sName);
        SetValueBoolean(sName, i & 1);
    }

#if KOTOR1  // K1 holds 500 number globals
    for (int i = 0; i < 500; i++)
#else
    for (int i = 0; i < 1000; i++)
#endif
    {
        sName.Format(const_cast<char*>("n%04d"), i);
        AddIdentifierNumber(sName);
        SetValueNumber(sName, static_cast<char>(-i));
    }

    for (int i = 0; i < 100; i++)
    {
        aLocations[i].m_vPosition    = Vector(static_cast<float>(i),  static_cast<float>(i),  static_cast<float>(i));
        aLocations[i].m_vOrientation = Vector(static_cast<float>(-i), static_cast<float>(-i), static_cast<float>(-i));
        sName.Format(const_cast<char*>("sl%04d"), i);
        AddIdentifierLocation(sName);
        SetValueLocation(sName, aLocations[i]);
    }

    for (int i = 0; i < 5; i++)
    {
        aStrings[i] = CExoString(i);
        sName.Format(const_cast<char*>("s%04d"), i);
        AddIdentifierString(sName);
        SetValueString(sName, aStrings[i]);
    }

    WriteTable(CExoString("save"), 1);
    ReadTable(CExoString("save"));

    for (int i = 0; i < 900; i++)
    {
        sName.Format(const_cast<char*>("b%04d"), i);
        GetValueBoolean(sName, nBooleanValue);
    }
#if KOTOR1
    for (int i = 0; i < 500; i++)
#else
    for (int i = 0; i < 1000; i++)
#endif
    {
        sName.Format(const_cast<char*>("n%04d"), i);
        GetValueNumber(sName, nNumberValue);
    }
    for (int i = 0; i < 100; i++)
    {
        sName.Format(const_cast<char*>("sl%04d"), i);
        GetValueLocation(sName, loc);
    }
    // "sl%04d", not "s%04d": the string read-back reuses the location prefix and looks up
    // "sl0000".."sl0004" in the string table.
    for (int i = 0; i < 5; i++)
    {
        sName.Format(const_cast<char*>("sl%04d"), i);
        GetValueString(sName, sValue);
    }
}

// Loads the global-variables table out of the current module and reports ReadTable's result.
int CSWGlobalVariableTable::Load()
{
    CExoString sFileName("GLOBALVARS");
    return ReadTable(sFileName);
}

// Writes the table into the module folder as binary.
void CSWGlobalVariableTable::Save(const CExoString& sPath)
{
#if KOTOR1  // K1 (Android port): "/" separators
    CExoString sFileName = sPath + CExoString("/") + CExoString("GLOBALVARS");
#else
    CExoString sFileName = sPath + CExoString("\\") + CExoString("GLOBALVARS");
#endif
    WriteTable(sFileName, 1);
}
