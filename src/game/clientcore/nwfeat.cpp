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
    m_nPrereqFeat1 = 0xffff;
    m_nPrereqFeat2 = 0xffff;
    m_nReqSkill = -1;
    for (int i = 0; i < 5; i++)
        m_anOrReqFeat[i] = 0xffff;
    m_nMinSpellLvl = 0;
    m_nSuccessor = -1;
    m_maxUses = 0;
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
