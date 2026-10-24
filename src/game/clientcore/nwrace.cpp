// One row of racialtypes.2da.

#include "k2stdint.h"
#include "nwrace.h"
#include "exostring.h"
#include "exolocstringinternal.h"
#include "twodimarray.h"

const unsigned short CSWRace::CRACE_DROID = 5;
const unsigned short CSWRace::CRACE_HUMAN = 6;
const unsigned short CSWRace::CRACE_ALL_RACES = 7;
const unsigned short CSWRace::CRACE_HUMAN_MASK = 1;
const unsigned short CSWRace::CRACE_DROID_MASK = 2;
const unsigned short CSWRace::CRACE_PLACEABLE_MASK = 4;

#include "port/libcstring.h"
#include "nwscriptconst.h"

CSWRace::CSWRace()
{
    m_paFirstLevelGrantedFeats = 0;
    m_nNameStrRef = -1;
    m_nNamePluralStrRef = -1;
    m_nDescriptionStrRef = -1;
    m_nDefaultBiographyStrRef = -1;
    memset(&m_anAbilityAdjust, 0, 7);  // idiom-exempt: 7 == the six ability bytes + the +0x1e pad
    m_nFavoredClass = CLASS_TYPE_INVALID;
    m_bPlayerRace = 0;
}

CSWRace::~CSWRace()
{
    if (m_paFirstLevelGrantedFeats) {
        delete[] m_paFirstLevelGrantedFeats;
        m_paFirstLevelGrantedFeats = 0;
    }
}

CExoString CSWRace::GetDescriptionText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nDescriptionStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

CExoString CSWRace::GetDefaultBiographyText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nDefaultBiographyStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

CExoString CSWRace::GetNameText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nNameStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

CExoString CSWRace::GetConverNameText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nConverNameStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

CExoString CSWRace::GetConverNameLowerText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nConverNameLowerStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

CExoString CSWRace::GetNamePluralText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nNamePluralStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

int CSWRace::IsFirstLevelGrantedFeat(unsigned short nFeat)
{
    if (!m_paFirstLevelGrantedFeats || !m_nNumFirstLevelGrantedFeats)
        return 0;

    for (unsigned int i = 0; i < m_nNumFirstLevelGrantedFeats; i++) {
        if (m_paFirstLevelGrantedFeats[i] == nFeat)
            return 1;
    }
    return 0;
}

// Builds the member array from a racialtypes.2da FeatIndex column.
void CSWRace::LoadFeatsTable(const CExoString& sTable)
{
    C2DA* p2DA = new C2DA(CResRef(sTable), 0);
    if (!p2DA->Load2DArray()) {
        // Only built for a diagnostic that is compiled out.
        CExoString sMessage;
        sMessage = CExoString("CSWRace::LoadFeatTable: Can't load") + sTable;
        sMessage = sMessage + CExoString(" 2DA file");
    }

    if (m_paFirstLevelGrantedFeats) {
        delete[] m_paFirstLevelGrantedFeats;
        m_paFirstLevelGrantedFeats = 0;
    }

    m_nNumFirstLevelGrantedFeats = static_cast<unsigned short>(p2DA->m_nRows);
    m_paFirstLevelGrantedFeats = new unsigned short[m_nNumFirstLevelGrantedFeats];

    for (int i = 0; i < m_nNumFirstLevelGrantedFeats; i++) {
        int nValue;
        m_paFirstLevelGrantedFeats[i] = 0;
        if (p2DA->GetINTEntry(i, CExoString("FeatIndex"), &nValue))
            m_paFirstLevelGrantedFeats[i] = static_cast<unsigned short>(nValue);
    }

    delete p2DA;
}
