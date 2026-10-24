#include "soundset.h"

#include "twodimarray.h"
#include "exolocstringinternal.h"
#include "exotlktable.h"
#include "strref.h"

extern void ByteSwap(unsigned int* p);

struct SSFHeader {
    char         szFileType[4];
    char         szVersion[4];
    unsigned int nStrRefTableOffset;
};

// A .ssf holds 28 sound slots, numbered from 1.
enum { SSF_STRRES_COUNT = 28 };

CSoundSet::CSoundSet(const CResRef& resRef)
{
    SetResRef(resRef, 0);
}

CSoundSet::~CSoundSet()
{
    if (m_pRes != 0 && m_pRes->GetDemands())
        m_pRes->Release();
}

int CSoundSet::GetStrres(unsigned short nStrres, STR_RES& strRes)
{
    unsigned char* pData = static_cast<unsigned char*>(m_pRes->GetData());

    unsigned int nTableOffset = reinterpret_cast<SSFHeader*>(pData)->nStrRefTableOffset;
    ByteSwap(&nTableOffset);

    if (static_cast<unsigned short>(nStrres - 1) >= SSF_STRRES_COUNT)
        return 0;

    unsigned int nStrRef = *reinterpret_cast<unsigned int*>(pData + nTableOffset + nStrres * sizeof(unsigned int)
                                            - sizeof(unsigned int));
    ByteSwap(&nStrRef);
    if (nStrRef == STRREF_NONE)
        return 0;

    g_pTlkTable->Fetch(nStrRef, strRes, CTlkTable::GENDER_MALE);
    return 1;
}

CSoundSetList::CSoundSetList()
{
    m_paResRefs = 0;
    m_nCount = 0;
    m_panGenders = 0;
    m_panNameStrRefs = 0;

    C2DA* p2daSoundSet = new C2DA(CResRef("SoundSet"), 0);
    if (p2daSoundSet->Load2DArray()) {
        C2DA* p2daSoundSetType = new C2DA(CResRef("SoundSetType"), 0);
        if (p2daSoundSetType->Load2DArray()) {
            m_nCount = static_cast<unsigned short>(p2daSoundSet->m_nRows);
            m_paResRefs = new CResRef[m_nCount];
            m_panGenders = new unsigned char[m_nCount];
            m_panNameStrRefs = new int[m_nCount];

            for (unsigned int i = 0; i < m_nCount; i++) {
                CExoString sResRef;
                p2daSoundSet->GetCExoStringEntry(i, CExoString("RESREF"), &sResRef);
                m_paResRefs[i] = sResRef;

                int nValue;
                p2daSoundSet->GetINTEntry(i, CExoString("STRREF"), &nValue);
                m_panNameStrRefs[i] = nValue;

                p2daSoundSet->GetINTEntry(i, CExoString("GENDER"), &nValue);
                m_panGenders[i] = static_cast<unsigned char>(nValue);
            }

            delete p2daSoundSet;
            delete p2daSoundSetType;
        } else {
            delete p2daSoundSet;
        }
    }
}

CSoundSetList::~CSoundSetList()
{
    if (m_paResRefs != 0) {
        delete[] m_paResRefs;
        m_paResRefs = 0;
    }
    if (m_panGenders != 0) {
        delete[] m_panGenders;
        m_panGenders = 0;
    }
    if (m_panNameStrRefs != 0) {
        delete[] m_panNameStrRefs;
        m_panNameStrRefs = 0;
    }
}

CResRef CSoundSetList::GetSoundSet(unsigned short nSoundSet)
{
    if (nSoundSet >= m_nCount)
        return CResRef();
    return m_paResRefs[nSoundSet];
}

unsigned char CSoundSetList::GetGender(unsigned short nSoundSet)
{
    if (nSoundSet >= m_nCount)
        return 0;
    return m_panGenders[nSoundSet];
}

CExoString CSoundSetList::GetName(unsigned short nSoundSet)
{
    if (nSoundSet >= m_nCount)
        return CExoString();

    STR_RES strRes;
    g_pTlkTable->Fetch(m_panNameStrRefs[nSoundSet], strRes, CTlkTable::GENDER_MALE);
    return strRes.text;
}
