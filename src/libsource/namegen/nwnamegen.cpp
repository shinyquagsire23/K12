#include "nwnamegen.h"
#include "exostring.h"
#include "twodimarray.h"
#include "exoresman.h"

#include "port/crtapi.h"
#if KOTOR1
static const float kRandMax = 2147483647.0f;  // K1 draws from the C library, whose RAND_MAX is two billion
#else
static const float kRandMax = 32767.0f;
#endif
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "exostringinline.h"

// The name tables' letter codes past 'a'..'z', and the species that picks the animal table.
enum {
    NAMECHAR_APOSTROPHE = 26,
    NAMECHAR_HYPHEN = 27,
    SPECIES_ANIMAL = 0xffff,
};

// Codes 0..25 are 'a'..'z'; the two extra codes the tables carry are apostrophe and hyphen.
static char NameChar(int nIndex)
{
    if (nIndex == NAMECHAR_APOSTROPHE)
        return '\'';
    return (nIndex == NAMECHAR_HYPHEN) ? '-' : static_cast<char>(nIndex + 'a');
}

static int NameIndex(char c)
{
    if (c == '\'')
        return NAMECHAR_APOSTROPHE;
    if (c == '-')
        return NAMECHAR_HYPHEN;
    return c - 'a';
}

CSWNameGen::CSWNameGen()
{
    m_bTableLoaded = 0;
    m_nLoadedSpecies = 0;
    m_nLoadedGender = 1;
}

CSWNameGen::~CSWNameGen()
{
    if (m_bTableLoaded) {
        m_bTableLoaded = 0;
        m_pRes->Release();
    }
}

void CSWNameGen::UnloadNameTable()
{
    if (m_bTableLoaded) {
        m_bTableLoaded = 0;
        m_pRes->Release();
    }
}

// Walks the loaded .ltr table: the first three letters come from the unigram, the bigram row it
// selects and that pair's trigram leaf, every later letter from the leaf's continue or end entry.
// Each draw scans the entry's cumulative distribution; a draw that runs past the entry backs the
// name up a letter or restarts it, and after five restarts the empty string comes back.
CExoString CSWNameGen::GetRandomName(unsigned char nLength)
{
    CExoString sName;
    int nCodes = m_pRes->m_pHeader->m_nCodeCount;
    char szName[64];
    int nLen = 0;
    int i1 K2_UNSET, i2 K2_UNSET, i3 K2_UNSET;
    unsigned char nTries = 0;
    bool bEnd = false;
    bool bFirst = true;

    for (;;) {
        if (nTries >= 5)
            return CExoString("");
        if (bEnd)
            break;
        if (bFirst) {
            for (;;) {
                float f1 = static_cast<float>(rand_wincompatible()) / kRandMax;
                for (i1 = 0; i1 < nCodes; i1++)
                    if (f1 < m_pRes->m_pUnigram[0][i1])
                        break;
                if (i1 == nCodes)
                    continue;

                float f2 = static_cast<float>(rand_wincompatible()) / kRandMax;
                for (i2 = 0; i2 < nCodes; i2++)
                    if (f2 < m_pRes->m_pBigram[0][i1][i2])
                        break;
                if (i2 == nCodes)
                    continue;

                float f3 = static_cast<float>(rand_wincompatible()) / kRandMax;
                for (i3 = 0; i3 < nCodes; i3++)
                    if (f3 < m_pRes->m_pTrigram[0][i1][i2][i3])
                        break;
                if (i3 == nCodes)
                    continue;
                break;
            }
            szName[0] = NameChar(i1);
            szName[1] = NameChar(i2);
            szName[2] = NameChar(i3);
            nLen = 3;
        }

        if (i3 < nCodes) {
            i1 = i2;
            i2 = i3;
        }
        int nDraw = rand_wincompatible();
        bool bHasLength = (nLength & 0xff) != 0;
        bool bAtTarget = (nLen == nLength - 1);
        int nMod = rand_wincompatible() % 12;
        float f = static_cast<float>(nDraw) / kRandMax;
        if (nMod <= nLen || (bHasLength && bAtTarget)) {
            for (i3 = 0; i3 < nCodes; i3++)
                if (f < m_pRes->m_pTrigram[2][i1][i2][i3])
                    break;
            if (i3 >= nCodes) {
                nTries++;
                bFirst = true;
                bEnd = false;
                continue;
            }
            bEnd = true;
        } else {
            for (i3 = 0; i3 < nCodes; i3++)
                if (f < m_pRes->m_pTrigram[1][i1][i2][i3])
                    break;
            bEnd = false;
            if (i3 >= nCodes) {
                if (nLen < 4) {
                    nTries++;
                    bFirst = true;
                    continue;
                }
                i2 = NameIndex(szName[nLen - 2]);
                i1 = NameIndex(szName[nLen - 3]);
                bFirst = false;
                nLen--;
                continue;
            }
        }
        szName[nLen] = NameChar(i3);
        nLen++;
        bFirst = false;
    }

    szName[nLen] = '\0';
    szName[0] = static_cast<char>(toupper(szName[0]));
    sName = szName;
    {
        CExoString sEntry;
        C2DA* p2da = new C2DA(CResRef("NameFilter"), 0);
        if (p2da->Load2DArray()) {
            int bOK = 1;
            for (int i = 0; i < p2da->m_nRows; i++) {
                int bFound = p2da->GetCExoStringEntry(i, CExoString("Name"), &sEntry);
                if (bFound && sEntry.CompareNoCase(sName) != 0) {
                    bOK = 0;
                    break;
                }
            }
            p2da->Unload2DArray();
            delete p2da;
            if (bOK)
                return sName;
            return GetRandomName(0);
        }
        delete p2da;
        return sName;
    }
}

// nGender is a part bitmask: 1 and 2 request a first name ("m" or "f" table suffix), 4 a last
// name ("l"); asking for both first-name bits is rejected. nLength is the length budget, split
// between the two parts when both are requested (0: no budget).
CExoString CSWNameGen::GetRandomName(unsigned short nSpecies, unsigned char nGender,
                                    unsigned char nLength)
{
    CExoString sTable;
    CExoString sFirst;
    CExoString sLast;

    if (nSpecies == SPECIES_ANIMAL) {
        sTable = "animal";
    } else {
        switch (nSpecies) {
        case 0: sTable = "dwarf";    break;
        case 1: sTable = "elf";      break;
        case 2: sTable = "gnome";    break;
        case 3: sTable = "halfling"; break;
        case 4:
            // A half-elf takes either the elf or the human table.
            if (rand_wincompatible() & 1)
                sTable = "elf";
            else
                sTable = "human";
            break;
        case 5: sTable = "halforc";  break;
        case 6: sTable = "human";    break;
        default: return CExoString("");
        }

        if (nGender == 4) {
            sLast = sTable + CExoString("l");
        } else if (nGender == 2) {
            sFirst = sTable + CExoString("f");
        } else if (nGender == 1) {
            sFirst = sTable + CExoString("m");
        } else {
            // A name cannot be both first and last, so the two first-name bits are exclusive.
            if ((nGender & 2) && (nGender & 1))
                return CExoString("");
            if (nGender & 1)
                sFirst = sTable + CExoString("m");
            else if (nGender & 2)
                sFirst = sTable + CExoString("f");
            if (nGender & 4)
                sLast = sTable + CExoString("l");

            if (sFirst.GetLength() != 0 &&
                sLast.GetLength() != 0) {
                CExoString sFirstPart;
                CExoString sLastPart;

                UnloadNameTable();

                if (nLength == 0) {
                    if (!LoadNameTable(sFirst))
                        return CExoString("");
                    sFirstPart = GetRandomName(0);
                    UnloadNameTable();
                    if (!LoadNameTable(sLast))
                        return CExoString("");
                    sLastPart = GetRandomName(0);
                    UnloadNameTable();
                    if (sFirstPart.GetLength() >= 1 &&
                        sLastPart.GetLength() >= 1)
                        return sFirstPart + CExoString(" ") + sLastPart;
                    return CExoString("");
                }

                int nLimit = (nLength * 2) / 3;
                int j;
                int nSplit = nLength / 3 + rand_wincompatible() % (nLength / 3);
                if (nSplit >= 1) {
                    for (; nSplit > 0; nSplit--) {
                        for (j = nSplit; j < nLimit; j++) {
                            if (!LoadNameTable(sFirst))
                                return CExoString("");
                            sFirstPart = GetRandomName(static_cast<unsigned char>(j));
                            UnloadNameTable();
                            if (!LoadNameTable(sLast))
                                return CExoString("");
                            sLastPart = GetRandomName(static_cast<unsigned char>(nLength - j));
                            UnloadNameTable();
                            if (sFirstPart.GetLength() >= 1 &&
                                sLastPart.GetLength() > 0)
                                break;
                        }
                        if (sFirstPart.GetLength() >= 1 &&
                            sLastPart.GetLength() >= 1)
                            return sFirstPart + CExoString(" ") + sLastPart;
                    }
                }
                return CExoString("");
            }
            return CExoString("");
        }
    }

    // The cached table is reused only for the same species and gender.
    if (m_bTableLoaded) {
        if (m_nLoadedSpecies == nSpecies && m_nLoadedGender == (nGender & 0xfb))
            return CExoString("");
        m_bTableLoaded = 0;
        m_pRes->Release();
    }

    if (sFirst.GetLength() >= 1) {
        if (!LoadNameTable(sFirst))
            return CExoString("");
    } else if (sLast.GetLength() >= 1) {
        if (!LoadNameTable(sLast))
            return CExoString("");
    } else {
        return CExoString("");
    }

    m_nLoadedSpecies = nSpecies;
    m_bTableLoaded = 1;
    m_nLoadedGender = nGender & ~4;
    return GetRandomName(nLength);
}

int CSWNameGen::LoadNameTable(const CExoString& sTableName)
{
    CResRef ref(sTableName);
    SetResRef(ref, 0);
    if (m_pRes->Demand()) {
        m_bTableLoaded = 1;
        return 1;
    }
    return 0;
}
