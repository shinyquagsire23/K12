#include "k2stdint.h"
#include "exotlktable.h"

#include "Achievements.h"
#include "swguiingame.h"
#include "twodimarray.h"
#include "appmanager.h"
#include "CExoArrayListImpl.h"
#include "exobase.h"
#include "exolocstringinternal.h"
#include "exostring.h"
#include "guiingame.h"
#include "resgff.h"
#include "exoresman.h"
#include "swguimaininterfacepc2.h"
#include "clientexoapp.h"
#include "exostringinline.h"

const int CTlkTable::CUSTOMTOKEN_RESERVED_END = 9;
const unsigned char CTlkTable::GENDER_MALE = 0;
const unsigned char CTlkTable::GENDER_FEMALE = 1;
const unsigned char CTlkTable::NUM_CONTENT_PKGS = 7;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/win32api.h"

#include "exoresgffread.h"
#include "restype.h"
#include "strref.h"

// The TLK file layout: the header, the string-entry size of a V3.0 file and of the older one, the
// "TLK " tag, and an entry's flags.
enum {
    TLK_HEADER_SIZE = 20,
    TLK_ENTRY_SIZE_V3_0 = 40,
    TLK_ENTRY_SIZE_OLD = 36,
    TLK_FILE_TYPE = 0x204b4c54,
    TEXT_PRESENT = 0x1,
    SND_PRESENT = 0x2,
    SNDLENGTH_PRESENT = 0x4,
    STRING_NOT_IN_PROJECT = 0x8000,
};

// AspyrFormatString's buffer.
enum {
    ASPYR_FORMAT_BUFFER_SIZE = 1024,
};

namespace {

// The on-disk TLK string-entry record. FetchInternal reads nEntrySize bytes of it, which is longer
// in the current format than in the older one, so m_fSoundLength is pre-zeroed and simply stays 0
// for the short record.
struct TLKStringEntry {
    unsigned int m_nFlags;           // bit0 text, bit1 sound resref, bit2 sound length, bit15 deleted
    char         m_szSoundResRef[16];// written straight into STR_RES::soundResRef
    unsigned int m_nVolumeVariance;           // byte-swapped but never read
    unsigned int m_nPitchVariance;           // byte-swapped but never read
    unsigned int m_nStringOffset;    // relative to CTlkFile::m_nStringTableOffset
    unsigned int m_nStringSize;      // byte count of the text that follows
    float        m_fSoundLength;     // only present in the 0x28 record
};

#if !KOTOR1
// Formats into a fixed 1024-byte buffer through the fortified vsprintf.
void AspyrFormatString(char* pDst, int, int, const char* pFmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, pFmt);
    __vsprintf_chk(pDst, 0, ASPYR_FORMAT_BUFFER_SIZE, pFmt, ap);
    __builtin_va_end(ap);
}
#endif

}  // namespace

// The accepted header version, compared by FetchInternal as a 4-byte load of its first four
// characters rather than as a CExoString.
const char* CTlkFile::TLK_FILE_VERSION = "V3.0";

CTlkFile::CTlkFile(const CExoString& resRef)
    : CExoFile(resRef, RESTYPE_TLK, CExoString("rb")),
      m_nFileType(0),
      m_nVersion(0),
      m_nLanguageID(0),
      m_nStringCount(0),
      m_nStringTableOffset(0)
{
}

int CTlkFile::ReadHeader()
{
    SeekBeginning();
    if (Read(&m_nFileType, 1, TLK_HEADER_SIZE) != TLK_HEADER_SIZE)
        return 0;

    ByteSwap(&m_nFileType);
    ByteSwap(&m_nVersion);
    ByteSwap(&m_nLanguageID);
    ByteSwap(&m_nStringCount);
    ByteSwap(&m_nStringTableOffset);

    return m_nFileType == TLK_FILE_TYPE;
}

CTlkTable::CTlkTable()
    : m_pStringTokens(0),
      m_nStringTokenCount(0),
      m_pCustomTokens(0),
      m_nCustomTokenCount(0),
      m_bShowStringRefs(0)
{
    // One fill per block, since a per-member memset would merge into scalar pairs rather than wide
    // stores.
#if KOTOR1  // K1: no override tables, only the file grid is zeroed
    __builtin_memset(m_pTlkFiles, 0, sizeof(m_pTlkFiles));  // idiom-exempt: measured codegen, see above
#else
    // The override tables and the file grid, cleared as one block.
    __builtin_memset(reinterpret_cast<char*>(this) + K2_OFFSETOF(CTlkTable, m_overrideStrRefs), 0,
                     K2_OFFSETOF(CTlkTable, m_pTlkFiles) + sizeof(m_pTlkFiles)
                     - K2_OFFSETOF(CTlkTable, m_overrideStrRefs));  // idiom-exempt: measured codegen, see above
#endif
    m_gender = 0;
}

CTlkTable::~CTlkTable()
{
    delete[] m_pCustomTokens;
    delete[] m_pStringTokens;

    for (int i = 0; i < 7; i++)
    {
        if (m_pTlkFiles[i][1] != 0 && m_pTlkFiles[i][1] != m_pTlkFiles[i][0])
            delete m_pTlkFiles[i][1];
        delete m_pTlkFiles[i][0];
    }
}

int CTlkTable::ExoIsDBCSLeadByte(unsigned char c)
{
    return (IsDBCSLeadByte(c) != 0) | ((c & 0x80) >> 7);
}

char* CTlkTable::RemapFontName(char* pName)
{
    return pName;
}

int CTlkTable::FetchInternal(unsigned int strRef, STR_RES& res, int bParse)
{
    CExoString sText;
    CExoString sError;

    sError.Format(const_cast<char*>("Invalid STRREF %d passed to Fetch"), strRef);

    if (strRef == STRREF_INVALID || m_gender > 1)
    {
        res.text = "";
        res.soundResRef = const_cast<char*>("");
        return 1;
    }

    for (int i = 0; i < 7; i++)
    {
        CTlkFile* pFile = m_pTlkFiles[i][m_gender];
        if (pFile == 0)
            continue;

        unsigned int nEntrySize =
            (pFile->m_nVersion == *reinterpret_cast<const unsigned int*>(CTlkFile::TLK_FILE_VERSION)) ? TLK_ENTRY_SIZE_V3_0 : TLK_ENTRY_SIZE_OLD;

        strRef &= STRREF_INDEX_MASK;
        if (strRef >= pFile->m_nStringCount)
            continue;
        if (nEntrySize * strRef + TLK_HEADER_SIZE > static_cast<unsigned int>(pFile->GetSize()))
            continue;
        if (!pFile->Seek(nEntrySize * strRef + TLK_HEADER_SIZE, 0))
            continue;

        TLKStringEntry strResHeader;
        strResHeader.m_fSoundLength = 0.0f;
        if (pFile->Read(&strResHeader, 1, nEntrySize) != nEntrySize)
            continue;

        ByteSwap(&strResHeader.m_nFlags);
        ByteSwap(&strResHeader.m_nVolumeVariance);
        ByteSwap(&strResHeader.m_nPitchVariance);
        ByteSwap(&strResHeader.m_nStringOffset);
        ByteSwap(&strResHeader.m_nStringSize);
        ByteSwap(&strResHeader.m_fSoundLength);

        if (strResHeader.m_nFlags & STRING_NOT_IN_PROJECT)
            continue;

        if (strResHeader.m_nFlags & SNDLENGTH_PRESENT)
            res.fSoundLength = strResHeader.m_fSoundLength;
        else
            res.fSoundLength = 0.0f;

        if (strResHeader.m_nFlags & SND_PRESENT)
            res.soundResRef = reinterpret_cast<unsigned char*>(strResHeader.m_szSoundResRef);
        else
            res.soundResRef = const_cast<char*>("");

        if (strResHeader.m_nFlags & TEXT_PRESENT)
        {
            if (pFile->m_nStringTableOffset + strResHeader.m_nStringOffset > static_cast<unsigned int>(pFile->GetSize()))
            {
                res.text = strRef;
                res.soundResRef = const_cast<char*>("");
                return 0;
            }

            if (!pFile->Seek(pFile->m_nStringTableOffset + strResHeader.m_nStringOffset, 0))
            {
                res.text = "";
                res.soundResRef = const_cast<char*>("");
                return 0;
            }

            char* pBuffer = new char[strResHeader.m_nStringSize];
            pFile->Read(pBuffer, 1, strResHeader.m_nStringSize);
            sText = CExoString(pBuffer, strResHeader.m_nStringSize);
            operator delete[](pBuffer);

            if (bParse)
                ParseStr(sText);

            res.text = sText;
        }

#if !KOTOR1  // K1 has no override tables
        // In the Aspyr override table for the current input mode, a strref match replaces the text.
        int nSet = IsJoystickConnected() & 1;
        if (m_overrideStrRefs[nSet].m_count >= 1)
        {
            int j = 0;
            do
            {
                if (m_overrideStrRefs[nSet].m_pArray[j] == static_cast<int>(strRef))
                    res.text = m_overrideStrings[nSet].m_pArray[j];
                j++;
            } while (j < m_overrideStrRefs[nSet].m_count);
        }
#endif
        return 1;
    }

    if (m_gender == 1)
    {
        m_gender = 0;
        return FetchInternal(strRef, res, bParse);
    }

    res.text = "";
    res.soundResRef = const_cast<char*>("");
    return 0;
}

// Substitutes token tags and strips {...} runs. m_bSuppressText, set by a re-entrant GetTokenValue,
// suppresses the escapes and every literal-run flush.
void CTlkTable::ParseStr(CExoString& sText)
{
    CExoString sResult;
    CExoString sToken;

    m_bSuppressText = 0;

    char* pStr = static_cast<char*>(sText.CStr());
    if (pStr[0] == '\0')
        return;

    int nStart = 0;
    int i = 0;
    do
    {
#if !KOTOR1  // K1 has no {}-run discard: only <> tokens are parsed
        if (pStr[i] == '{')
        {
            int j = i + 1;
            if (pStr[j] == '{')
            {
                // The doubled bracket is an escape, so flush the run, then skip both characters.
                if (m_bSuppressText == 0)
                    sResult = sResult + CExoString(&pStr[nStart], j - nStart);
                i++;
                nStart = i + 1;
                continue;
            }

            if (m_bSuppressText == 0)
                sResult = sResult + CExoString(&pStr[nStart], i - nStart);

            // A braced run is discarded, closing bracket included (there is no {}-family token).
            int n = j;
            while (pStr[n] != '\0' && pStr[n] != '}')
                n++;

            i = n;
            nStart = i + 1;
            continue;
        }

#endif
        if (pStr[i] == '<')
        {
            int j = i + 1;
            if (pStr[j] == '<')
            {
                if (m_bSuppressText == 0)
                    sResult = sResult + CExoString(&pStr[nStart], j - nStart);
                i++;
                nStart = i + 1;
                continue;
            }

            if (m_bSuppressText == 0)
                sResult = sResult + CExoString(&pStr[nStart], i - nStart);

            sToken = "<UNRECOGNIZED TOKEN>";

            // Find the closing bracket.
            int nEnd = j;
            while (pStr[nEnd] != '\0' && pStr[nEnd] != '>')
                nEnd++;

            unsigned int nNameLen = static_cast<unsigned int>(nEnd - j);
            if (nEnd > j)
            {
                if (nNameLen >= 7 && memcmp(&pStr[j], "CUSTOM", 6) == 0)
                {
                    // <CUSTOMnnn>: parse the digits backwards from the closing bracket.
                    unsigned int nCustom = 0;
                    int nDigit = 1;
                    bool bDigits = true;
                    int k = nEnd - 1;
                    while (k > i + 6)
                    {
                        int d = pStr[k] - '0';
                        if (static_cast<unsigned int>(d) > 9)
                        {
                            bDigits = false;
                            break;
                        }
                        nCustom += static_cast<unsigned int>(d) * nDigit;
                        nDigit *= 10;
                        k--;
                    }

                    if (bDigits && nCustom != static_cast<unsigned int>(-1))
                    {
                        // Sorted binary search over the custom-token table by id.
                        unsigned int nLow = 0;
                        unsigned int nHigh = m_nCustomTokenCount;
                        while (nHigh > nLow)
                        {
                            unsigned int nMid = (nLow + nHigh) >> 1;
                            unsigned int nId = static_cast<unsigned int>(m_pCustomTokens[nMid].m_nId);
                            if (nId > nCustom)
                                nHigh = nMid;
                            else if (nId < nCustom)
                                nLow = nMid + 1;
                            else
                            {
                                sToken = m_pCustomTokens[nMid].m_sValue;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    // Hashed lookup in the sorted StringTokens table.
                    unsigned int nHash = CResGFFFieldIDHash::Hash(&pStr[j], nNameLen);
                    unsigned int nLow = 0;
                    unsigned int nHigh = m_nStringTokenCount;
                    while (nHigh > nLow)
                    {
                        unsigned int nMid = (nLow + nHigh) >> 1;
                        unsigned int nMidHash = m_pStringTokens[nMid].m_nNameHash;
                        if (nMidHash > nHash)
                            nHigh = nMid;
                        else if (nMidHash < nHash)
                            nLow = nMid + 1;
                        else
                        {
#if KOTOR1  // K1: CExoString::GetLength is out of line
                            unsigned int nNameSize = m_pStringTokens[nMid].m_sName.GetLength();
#else
                            unsigned int nNameSize = 0;
                            if (m_pStringTokens[nMid].m_sName.m_sString != 0)
                                nNameSize = static_cast<unsigned int>(strlen(m_pStringTokens[nMid].m_sName.m_sString));
#endif
                            if (nNameSize != nNameLen ||
                                strncmp(m_pStringTokens[nMid].m_sName.CStr(), &pStr[j], nNameLen) != 0)
                                break;

                            GetTokenValue(m_pStringTokens[nMid], sToken);
                            break;
                        }
                    }
                }
            }

            i = nEnd;
            sResult = sResult + sToken;
            nStart = i + 1;
            continue;
        }
} while (pStr[++i] != '\0');
    if (nStart == 0)
        return;

    if (i > nStart && m_bSuppressText == 0)
        sResult = sResult + CExoString(&pStr[nStart]);

    sText = sResult;
}

CExoString CTlkTable::GetSimpleString(uint64_t strRef)
{
    STR_RES res;
    Fetch(static_cast<unsigned int>(strRef), res, 0);
    return res.text;
}

int CTlkTable::Fetch(unsigned int strRef, STR_RES& res, unsigned char gender)
{
#if KOTOR1  // K1: no achievement hook, no help popup, the gender stored as given
    m_gender = gender;
#else
    Achievements::EvaluateAchievement(strRef);

#ifndef K2_UI_PC  // PC UI: the macOS Fetch has no help-overlay hook (the overlays are touch/pad art)
    if (g_pAppManager->m_pClientExoApp->GetInGameGui() != 0 &&
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->ShowHelpForString(strRef);
#endif

    m_gender = gender > 1 ? 0 : gender;
#endif

    int nRet = FetchInternal(strRef, res, 0);

    if (m_bShowStringRefs)
        res.text.Format(const_cast<char*>("[%d]%s"), strRef, res.text.CStr());

    return nRet;
}

#ifdef K2_UI_PC
const char* k2pcstrings_line(int nIndex);
#endif

// Opens the base table, then, if the resolved name is long enough, loads the two Aspyr input-mode
// override files alongside it, and finally rebuilds the StringTokens table from its 2DA. Each override
// row is "<strref>,<text>"; both tables are kept insertion-sorted by strref.
int CTlkTable::OpenFile(const CExoString& sPath)
{
    if (!AddFile(sPath, 0))
        return 0;

#if !KOTOR1  // K1: no Aspyr input-mode override files
    CExoString sResolved = g_pExoBase->m_pAliasList->ResolveFileName(sPath, RESTYPE_TLK);
    const char* pszName = sResolved.CStr();

    if (strlen(pszName) >= 6)
    {
        for (int i = 0; i < 2; i++)
        {
            char szOverride[ASPYR_FORMAT_BUFFER_SIZE];
            AspyrFormatString(szOverride, 0, 0, "%s_%s", pszName, i == 0 ? "touch" : "control");

            AspyrLocalizedStrings localized(szOverride, true);

            for (int j = 0; ; j++)
            {
                const char* pszLine = localized.GetString(j, 0);
#ifdef K2_UI_PC
                // PC UI: set 0 is the keyboard-and-mouse set, built in (there is no touch screen).
                if (i == 0)
                    pszLine = k2pcstrings_line(j);
#endif

                int nStrRef = 0;
                static_cast<void>(sscanf(pszLine, "%d,%*s", &nStrRef));

                if (nStrRef != 0)
                {
                    char* pszText = new char[strlen(pszLine)];
                    pszText[0] = '\0';

                    // Skip past the first comma, then strip one leading and one trailing '"'.
                    const char* p = pszLine;
                    char c;
                    do
                        c = *p++;
                    while (c != ',' && c != '\0');
                    if (c == '\0')
                        p--;
                    if (*p == '"')
                        p++;
                    int n = 0;
                    while (p[n] != '\0' && p[n] != '"')
                    {
                        pszText[n] = p[n];
                        n++;
                    }
                    pszText[n] = '\0';

                    m_overrideStrRefs[i].Add(nStrRef);
                    m_overrideStrings[i].Add(pszText);
                }

                if (*pszLine == '\0')
                    break;
            }
        }
    }

#endif
    delete[] m_pStringTokens;

    CExoString sValue;

    CResRef refTokens("StringTokens");
    C2DA tokens(refTokens, 0);
    if (!tokens.Load2DArray())
        return 0;

    m_nStringTokenCount = tokens.m_nRows;
    m_pStringTokens = new CTlkTableToken[m_nStringTokenCount];

    for (int row = 0; static_cast<unsigned int>(row) < m_nStringTokenCount; row++)
    {
        CTlkTableToken* pToken = &m_pStringTokens[row];
        int nValue = 0;

        tokens.GetCExoStringEntry(row, 0, &pToken->m_sName);
        tokens.GetINTEntry(row, 1, &nValue);
        pToken->m_nLanguage = nValue;
        tokens.GetINTEntry(row, 2, &nValue);
        pToken->m_nStringRef = nValue;
        tokens.GetINTEntry(row, 3, &nValue);
        pToken->m_StrRefs[0] = nValue;
        tokens.GetINTEntry(row, 4, &nValue);
        pToken->m_StrRefs[1] = nValue;
        tokens.GetINTEntry(row, 5, &nValue);
        pToken->m_StrRefs[2] = nValue;
        tokens.GetINTEntry(row, 6, &nValue);
        pToken->m_StrRefs[3] = nValue;

#if KOTOR1  // K1: CExoString::GetLength is out of line
        pToken->m_nNameHash = CResGFFFieldIDHash::Hash(pToken->m_sName.CStr(),
            pToken->m_sName.GetLength());
#else
        pToken->m_nNameHash = CResGFFFieldIDHash::Hash(pToken->m_sName.CStr(),
            static_cast<unsigned int>(pToken->m_sName.GetLength()));
#endif

        // Insertion sort by name hash (the table is binary-searched).
        for (int k = row; k > 0; k--)
        {
            if (m_pStringTokens[k].m_nNameHash >= m_pStringTokens[k - 1].m_nNameHash)
                break;
            CTlkTableToken& a = m_pStringTokens[k];
            CTlkTableToken& b = m_pStringTokens[k - 1];

            unsigned int nHash = b.m_nNameHash;
            sValue = b.m_sName;
            unsigned int nLang = b.m_nLanguage;
            unsigned int nRefs[4];
            nRefs[0] = b.m_StrRefs[0];
            nRefs[1] = b.m_StrRefs[1];
            nRefs[2] = b.m_StrRefs[2];
            nRefs[3] = b.m_StrRefs[3];
            int nStringRef = b.m_nStringRef;

            b.m_nNameHash = a.m_nNameHash;
            b.m_sName = a.m_sName;
            b.m_nLanguage = a.m_nLanguage;
            b.m_StrRefs[0] = a.m_StrRefs[0];
            b.m_StrRefs[1] = a.m_StrRefs[1];
            b.m_StrRefs[2] = a.m_StrRefs[2];
            b.m_StrRefs[3] = a.m_StrRefs[3];
            b.m_nStringRef = a.m_nStringRef;

            a.m_nNameHash = nHash;
            a.m_sName = sValue;
            a.m_nLanguage = nLang;
            a.m_StrRefs[0] = nRefs[0];
            a.m_StrRefs[1] = nRefs[1];
            a.m_StrRefs[2] = nRefs[2];
            a.m_StrRefs[3] = nRefs[3];
            a.m_nStringRef = nStringRef;
        }
    }

    return 1;
}

int CTlkTable::AddFile(const CExoString& sPath, int nIndex)
{
#ifdef K2_FIX_ENGINE_BUGS
    if (static_cast<unsigned int>(nIndex) >= 7)
#else
    if (static_cast<unsigned int>(nIndex) > 7)
#endif
        return 0;
    if (sPath == "")
        return 0;

    // Replace whatever is registered at this index. The two slots may alias (the "F" file falls
    // back to the base file below), so never delete the same pointer twice.
    if (m_pTlkFiles[nIndex][1] != m_pTlkFiles[nIndex][0])
        delete m_pTlkFiles[nIndex][1];
    else
        m_pTlkFiles[nIndex][1] = 0;
    delete m_pTlkFiles[nIndex][0];

    m_pTlkFiles[nIndex][0] = new CTlkFile(sPath);
    if (!m_pTlkFiles[nIndex][0]->FileOpened())
        return 0;
    if (!m_pTlkFiles[nIndex][0]->ReadHeader())
    {
        delete m_pTlkFiles[nIndex][0];
        m_pTlkFiles[nIndex][0] = 0;
        return 0;
    }

    m_pTlkFiles[nIndex][1] = new CTlkFile(sPath + "F");
    if (!m_pTlkFiles[nIndex][1]->FileOpened())
    {
        delete m_pTlkFiles[nIndex][1];
        m_pTlkFiles[nIndex][1] = m_pTlkFiles[nIndex][0];
        return 1;
    }
    if (!m_pTlkFiles[nIndex][1]->ReadHeader())
    {
        delete m_pTlkFiles[nIndex][0];
        m_pTlkFiles[nIndex][0] = 0;
        delete m_pTlkFiles[nIndex][1];
        m_pTlkFiles[nIndex][1] = 0;
        return 0;
    }

    return 1;
}

int CTlkTable::RemoveFile(int nIndex)
{
    if (static_cast<unsigned int>(nIndex - 1) > 6)
        return 0;

    if (m_pTlkFiles[nIndex][1] != m_pTlkFiles[nIndex][0])
    {
        if (m_pTlkFiles[nIndex][1] != 0)
        {
            delete m_pTlkFiles[nIndex][1];
            m_pTlkFiles[nIndex][1] = 0;
        }
    }
    else
    {
        m_pTlkFiles[nIndex][1] = 0;
    }

    if (m_pTlkFiles[nIndex][0] != 0)
    {
        delete m_pTlkFiles[nIndex][0];
        m_pTlkFiles[nIndex][0] = 0;
    }

    return 1;
}

// Inserts or replaces into the id-sorted custom-token table with a grow-and-shift, not a
// CExoArrayList, because the element carries a CExoString and the grow re-runs every element's
// destructor.
void CTlkTable::SetCustomToken(int nId, const CExoString& sValue)
{
    if (nId < 0)
        return;

    unsigned int nCount = m_nCustomTokenCount;
    unsigned int nIndex = 0;

    if (nCount != 0)
    {
        // Binary search over the id-sorted table.
        unsigned int nLow = 0;
        unsigned int nHigh = nCount;
        while (nHigh > nLow)
        {
            unsigned int nMid = (nLow + nHigh) >> 1;
            if (static_cast<unsigned int>(m_pCustomTokens[nMid].m_nId) > static_cast<unsigned int>(nId))
                nHigh = nMid;
            else if (static_cast<unsigned int>(m_pCustomTokens[nMid].m_nId) < static_cast<unsigned int>(nId))
                nLow = nMid + 1;
            else
            {
                m_pCustomTokens[nMid].m_sValue = sValue;
                return;
            }
        }
        nIndex = nLow;
    }

    const unsigned int nNewCount = nCount + 1;
    CTlkTableCustomToken* pNew = new CTlkTableCustomToken[nNewCount];

    // nIndex is the search's insertion point, at most nCount, and the table holds nCount tokens.
    unsigned int i;
    for (i = 0; i < nIndex; i++)
        pNew[i] = m_pCustomTokens[i];

    K2_ASSUME(i < nNewCount);
    pNew[i].m_nId = nId;
    pNew[i].m_sValue = sValue;

    for (; i < m_nCustomTokenCount; i++)
    {
        K2_ASSUME(i + 1 < nNewCount);
        pNew[i + 1] = m_pCustomTokens[i];
    }

    delete[] m_pCustomTokens;
    m_pCustomTokens = pNew;
    m_nCustomTokenCount++;
}

void CTlkTable::ClearCustomTokens()
{
    if (m_pCustomTokens != 0)
    {
        delete[] m_pCustomTokens;
        m_pCustomTokens = 0;
    }
    m_nCustomTokenCount = 0;
}

int CTlkTable::GetLanguageVersion()
{
    if (m_pTlkFiles[0][0] != 0)
        return m_pTlkFiles[0][0]->m_nLanguageID;
    return 0;
}

// The base class has no token semantics of its own; CSWTlkTable overrides this.
void CTlkTable::GetTokenValue(const CTlkTableToken& token, CExoString& sValue)
{
    K2_UNUSED(token); K2_UNUSED(sValue);
}
