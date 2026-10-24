#include "k2stdint.h"
#include "nwrules.h"
#include "exostring.h"
#include "exolocstringinternal.h"

#include "port/libcstring.h"

CSWFeat::CSWFeat()
{
    m_nNameStrRef = -1;
    m_nDescriptionStrRef = -1;
    m_nMinAttackBonus = 0;
    m_nMinSTR = 0;
    m_nMinDEX = 0;
    m_nMinINT = 0;
    m_nMinWIS = 0;
    memset(&m_nPrereqFeat1, -1, 16);  // idiom-exempt: 16 == the eight sentinel shorts +0x34..+0x43
    m_nMinSpellLvl = 0;
    m_maxUses = 0;
    m_nSuccessor = -1;
    m_nSpellId = -1;
    m_nFlags = (m_nFlags & ~FLAG_MASK) | FLAG_PRESENT;
#if !KOTOR1
    memset(&m_nHandmaidenGrantLevel, 0, 7);  // idiom-exempt: 7 == the seven grant bytes +0x44..+0x4a
#endif
}

// Same lookup as CSWRules::GetMasterFeatNameText and GetMasterFeatDescriptionText, but keyed by
// the row's own strref rather than an array subscript.
CExoString CSWFeat::GetDescriptionText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nDescriptionStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

CExoString CSWFeat::GetNameText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nNameStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

CSWFeat::~CSWFeat()
{
}
