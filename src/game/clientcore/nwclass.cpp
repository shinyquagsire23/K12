#include "k2stdint.h"
#include "nwclass.h"
#include "nwsrules.h"
#include "nwskill.h"
#include "exostring.h"
#include "exolocstringinternal.h"
#include "twodimarray.h"
#include "nwscriptconst.h"

const unsigned char CSWClass::CCLASS_SOLDIER = 0;
const unsigned char CSWClass::CCLASS_SCOUT = 1;
const unsigned char CSWClass::CCLASS_SCOUNDREL = 2;
const unsigned char CSWClass::CCLASS_JEDIGUARDIAN = 3;
const unsigned char CSWClass::CCLASS_JEDICONSULAR = 4;
const unsigned char CSWClass::CCLASS_JEDISENTINEL = 5;
#if !KOTOR1
const unsigned char CSWClass::CCLASS_JEDIWEAPONMASTER = 11;
const unsigned char CSWClass::CCLASS_JEDIMASTER = 12;
const unsigned char CSWClass::CCLASS_JEDIWATCHMAN = 13;
const unsigned char CSWClass::CCLASS_SITHMARAUDER = 14;
const unsigned char CSWClass::CCLASS_SITHLORD = 15;
const unsigned char CSWClass::CCLASS_SITHASSASSIN = 16;
#endif

#include "port/libcstring.h"
#include "exostringinline.h"

// Seeds the row's defaults. The bonus-feat clear starts one entry in, since the normal-feat clear
// owns that slot.
CSWClass::CSWClass()
{
    m_nNameStrRef = STRREF_NONE;
    m_nNamePluralStrRef = STRREF_NONE;
    m_nDescriptionStrRef = STRREF_NONE;
#if KOTOR1
    __builtin_memset(m_aAttackBonus, 0, sizeof(m_aAttackBonus));
#else
    __builtin_memset(m_aAttackBonus, 0x30, sizeof(m_aAttackBonus));
#endif
    __builtin_memset(m_aSpellGain, 0x30, sizeof(m_aSpellGain));
    __builtin_memset(&m_bonusFeats.m_aByLevel[1], 0, sizeof(m_bonusFeats.m_aByLevel) - 2);
    __builtin_memset(m_normalFeats.m_aByLevel, 0, sizeof(m_normalFeats.m_aByLevel));
    __builtin_memset(m_abRecommendedScores, 0, sizeof(m_abRecommendedScores));
#if KOTOR1
    __builtin_memset(m_anEffCRByLevel, 0, 20);  // idiom-exempt: K1 skips the 21st byte the same way
#else
    __builtin_memset(m_anEffCRByLevel, 0, 50);  // idiom-exempt: 50 == +0x223..+0x254, one byte short of the 51-entry array; the ORIG skips +0x255
#endif
    __builtin_memset(m_aFortSaveBonus, 0, sizeof(m_aFortSaveBonus));
    __builtin_memset(m_aWillSaveBonus, 0, sizeof(m_aWillSaveBonus));
    __builtin_memset(m_aSpellGainMinLevel, 0, sizeof(m_aSpellGainMinLevel));
    __builtin_memset(m_aSpellGain, 0, sizeof(m_aSpellGain));
    __builtin_memset(m_aRefSaveBonus, 0, sizeof(m_aRefSaveBonus));
    m_paClassSkills = 0;
    m_paRecommendedSpells = 0;
    m_nHitDie = 0;
    m_nNumClassSkills = 0;
    m_bPlayerClass = 0;
    m_paGrantedFeats = 0;
    m_nNumGrantedFeats = 0;
    m_bSpellCaster = 0;
    m_nPrimaryAbility = 0;
    m_alignRestrict = 0;
    m_nAlignRestrictType = 0;
    m_pArmorBonusTable = 0;
    m_paSpellsKnownPerLevel = 0;
}

// Per-level bonus tables, 1-based class level 1..60; 0 means out of range.
unsigned char CSWClass::GetAttackBonus(unsigned char nLevel)
{
    if (nLevel < 1 || nLevel > 60)
        return 0;
    return m_aAttackBonus[nLevel - 1];
}

unsigned char CSWClass::GetFortSaveBonus(unsigned char nLevel)
{
    if (nLevel < 1 || nLevel > 60)
        return 0;
    return m_aFortSaveBonus[nLevel - 1];
}

unsigned char CSWClass::GetRefSaveBonus(unsigned char nLevel)
{
    if (nLevel < 1 || nLevel > 60)
        return 0;
    return m_aRefSaveBonus[nLevel - 1];
}

unsigned char CSWClass::GetWillSaveBonus(unsigned char nLevel)
{
    if (nLevel < 1 || nLevel > 60)
        return 0;
    return m_aWillSaveBonus[nLevel - 1];
}

// Armor bonus comes from a heap table indexed directly by level, entry 0 unused.
unsigned char CSWClass::GetArmorBonus(int nLevel)
{
#if KOTOR1
#else
    if (nLevel < 1 || nLevel > 60)
        return 0;
#endif
    return m_pArmorBonusTable[nLevel];
}

// Force-power slots at a caster level, gated by the row's own minimum caster level.
int CSWClass::GetSpellGain(unsigned char nLevel, unsigned char nMinLevel)
{
#if KOTOR1
    if (static_cast<unsigned char>(nLevel - 1) < 20 && nMinLevel < m_aSpellGainMinLevel[static_cast<uint64_t>(nLevel) - 1])
#else
    if (static_cast<unsigned char>(nLevel - 1) < 50 && nMinLevel < m_aSpellGainMinLevel[static_cast<uint64_t>(nLevel) - 1])
#endif
        return m_aSpellGain[static_cast<uint64_t>(nLevel) - 1];
    return LEVEL_NONE;
}

// Nth recommended chargen skill; a generic entry (order 0) is the fallback answer.
unsigned int CSWClass::GetRecommendedSkill(int nOrder)
{
    if (m_nNumClassSkills == 0)
        return RECOMMENDED_NONE;
    CSWClassSkill* pEntry = m_paClassSkills;
    unsigned short i = 0;
    unsigned int nResult = RECOMMENDED_NONE;
    int nLastFallbackOrder = -1;
    while (i < m_nNumClassSkills) {
        if (pEntry->m_nRecommendedOrder == nOrder)
            return pEntry->m_nSkill;
        if (nLastFallbackOrder != nOrder && pEntry->m_nRecommendedOrder == 0) {
            nResult = pEntry->m_nSkill;
            nLastFallbackOrder++;
        }
        i++;
        pEntry++;
    }
    return nResult;
}

// Nth recommended chargen spell for one alignment side. Only user types 1 and 5 count, and a
// generic entry (order 0) is the fallback answer.
unsigned int CSWClass::GetRecommendedSpell(int bGoodAligned, int nOrder)
{
    if (m_nNumRecommendedSpells == 0)
        return RECOMMENDED_NONE;
    CSWClassSpell* pEntry = m_paRecommendedSpells;
    unsigned short i = 0;
    unsigned int nResult = RECOMMENDED_NONE;
    int nLastFallbackCount = 0;
    while (i < m_nNumRecommendedSpells) {
        if (pEntry->m_nUserType == 1 || pEntry->m_nUserType == 5) {
            if (bGoodAligned) {
                if (pEntry->m_nLightRecommended == nOrder)
                    return pEntry->m_nSpellId;
                if (nLastFallbackCount != nOrder && pEntry->m_nLightRecommended == 0) {
                    nResult = pEntry->m_nSpellId;
                    nLastFallbackCount++;
                }
            } else {
                if (pEntry->m_nDarkRecommended == nOrder)
                    return pEntry->m_nSpellId;
                if (nLastFallbackCount != nOrder && pEntry->m_nDarkRecommended == 0) {
                    nResult = pEntry->m_nSpellId;
                    nLastFallbackCount++;
                }
            }
        }
        i++;
        pEntry++;
    }
    return nResult;
}

// Nth recommended chargen feat, taken from the granted-feat table's own order column.
unsigned int CSWClass::GetRecommendedFeat(int nOrder)
{
    if (m_nNumGrantedFeats == 0)
        return RECOMMENDED_NONE;
    CSWClassGrantedFeat* pEntry = m_paGrantedFeats;
    unsigned short i = 0;
    unsigned int nResult = RECOMMENDED_NONE;
    int nLastFallbackOrder = -1;
    while (i < m_nNumGrantedFeats) {
        if (pEntry->m_nRecommendedOrder == nOrder)
            return pEntry->m_nFeat;
        if (nLastFallbackOrder != nOrder && pEntry->m_nRecommendedOrder == 0) {
            nResult = pEntry->m_nFeat;
            nLastFallbackOrder++;
        }
        i++;
        pEntry++;
    }
    return nResult;
}

// Granted level for nFeat, or 255 when the feat is absent or on a selectable list.
unsigned char CSWClass::GetLevelFeatGranted(unsigned short nFeat)
{
    for (unsigned short i = 0; i < m_nNumGrantedFeats; i++) {
        CSWClassGrantedFeat* pRow = &m_paGrantedFeats[i];
        if (pRow->m_nFeat == nFeat && !pRow->m_nFeatList)
            return pRow->m_nLevel;
    }
    return LEVEL_NONE;
}

// Known-spell slots at (nLevel, nSpellLevel), gated by the level's own minimum spell level.
// nClassId, nRace and nCHA are unused.
unsigned char CSWClass::GetSpellsKnownPerLevel(unsigned char nLevel, unsigned char nSpellLevel,
                                                unsigned char nClassId, unsigned short nRace,
                                                unsigned char nCHA)
{
    K2_UNUSED(nClassId); K2_UNUSED(nRace); K2_UNUSED(nCHA);
#if KOTOR1
    if (static_cast<unsigned char>(nLevel - 1) >= 20)
        return 0;
#else
    if (static_cast<unsigned char>(nLevel - 1) >= 50)
        return 0;
#endif
    if (m_aSpellGainMinLevel[static_cast<uint64_t>(nLevel) - 1] <= nSpellLevel)
        return 0;
    if (!m_paSpellsKnownPerLevel)
        return 0;
    return m_paSpellsKnownPerLevel[static_cast<uint64_t>(nLevel) - 1][nSpellLevel];
}

int CSWClass::IsSkillClassSkill(unsigned short nSkill)
{
    if (nSkill >= g_pRules->m_nNumSkills || m_nNumClassSkills == 0)
        return 0;
    for (unsigned short i = 0; i < m_nNumClassSkills; i++)
        if (m_paClassSkills[i].m_nSkill == nSkill)
            return m_paClassSkills[i].m_bClassSkill;
    return 0;
}

// Whether this class grants use of a given skill. A skill flagged as useable by every class returns
// true outright; otherwise the class's own skill table is scanned for a match.
int CSWClass::IsSkillUseable(unsigned short nSkill)
{
    CSWSkill* pSkill;
    if (g_pRules->m_nNumSkills <= nSkill)
        pSkill = 0;
    else
        pSkill = &g_pRules->m_paSkills[nSkill];
    if (pSkill == 0)
        return 0;
    if (pSkill->m_flags & 8)
        return 1;
    for (int64_t i = 0; i < m_nNumClassSkills; i++)
        if (m_paClassSkills[i].m_nSkill == nSkill)
            return 1;
    return 0;
}

// The TLK-text accessors fetch the row's text in the male gender form and return it by value.
CExoString CSWClass::GetNameText()
{
    STR_RES strRes;
    g_pTlkTable->Fetch(m_nNameStrRef, strRes, CTlkTable::GENDER_MALE);
    return strRes.text;
}

CExoString CSWClass::GetNameLowerText()
{
    STR_RES strRes;
    g_pTlkTable->Fetch(m_nNameLowerStrRef, strRes, CTlkTable::GENDER_MALE);
    return strRes.text;
}

CExoString CSWClass::GetNamePluralText()
{
    STR_RES strRes;
    g_pTlkTable->Fetch(m_nNamePluralStrRef, strRes, CTlkTable::GENDER_MALE);
    return strRes.text;
}

CExoString CSWClass::GetDescriptionText()
{
    STR_RES strRes;
    g_pTlkTable->Fetch(m_nDescriptionStrRef, strRes, CTlkTable::GENDER_MALE);
    return strRes.text;
}

// The two feat-list predicates are identical scans, each reporting a different bit of the row's
// list field.
int CSWClass::IsNormalFeat(unsigned short nFeat)
{
    for (unsigned short i = 0; i < m_nNumGrantedFeats; i++)
        if (m_paGrantedFeats[i].m_nFeat == nFeat)
            return m_paGrantedFeats[i].m_nFeatList & 1;
    return 0;
}

int CSWClass::IsBonusFeat(unsigned short nFeat)
{
    for (unsigned short i = 0; i < m_nNumGrantedFeats; i++)
        if (m_paGrantedFeats[i].m_nFeat == nFeat)
            return (m_paGrantedFeats[i].m_nFeatList >> 1) & 1;
    return 0;
}

// Granted only when the feat is auto-granted, not on either selectable list. nLevel is armed with
// 0xff up front, so a caller that ignores the return still sees "no level".
int CSWClass::IsGrantedFeat(unsigned short nFeat, unsigned char& nLevel)
{
    nLevel = LEVEL_NONE;
    for (unsigned short i = 0; i < m_nNumGrantedFeats; i++) {
        CSWClassGrantedFeat* pRow = &m_paGrantedFeats[i];
        if (pRow->m_nFeat == nFeat) {
            if (pRow->m_nFeatList)
                return 0;
            nLevel = pRow->m_nLevel;
            return 1;
        }
    }
    return 0;
}

// Same scan, reporting the row's PC-only flag instead.
#if !KOTOR1
int CSWClass::IsPCGrantedFeat(unsigned short nFeat)
{
    for (unsigned short i = 0; i < m_nNumGrantedFeats; i++)
        if (m_paGrantedFeats[i].m_nFeat == nFeat)
            return m_paGrantedFeats[i].m_bPCOnly == 1;
    return 0;
}
#endif

// Answers whether this class can use nFeat. The feat must exist globally and appear anywhere in the
// class's granted-feat table, on a list or not.
int CSWClass::IsFeatUseable(unsigned short nFeat)
{
    if (g_pRules->GetFeat(nFeat) == 0)
        return 0;
    for (unsigned short i = 0; i < m_nNumGrantedFeats; i++)
        if (m_paGrantedFeats[i].m_nFeat == nFeat)
            return 1;
    return 0;
}

// The two class-family predicates, taking a raw classes.2da row id.
int CSWClass::IsJedi(int nClass)
{
#if KOTOR1
    return nClass == CLASS_TYPE_JEDIGUARDIAN || nClass == CLASS_TYPE_JEDICONSULAR ||
           nClass == CLASS_TYPE_JEDISENTINEL;
#else
    return nClass == CLASS_TYPE_JEDIGUARDIAN || nClass == CLASS_TYPE_JEDICONSULAR ||
           nClass == CLASS_TYPE_JEDISENTINEL || nClass == CLASS_TYPE_JEDIWEAPONMASTER ||
           nClass == CLASS_TYPE_JEDIWATCHMAN || nClass == CLASS_TYPE_SITHMARAUDER ||
           nClass == CLASS_TYPE_SITHLORD || nClass == CLASS_TYPE_SITHASSASSIN ||
           nClass == CLASS_TYPE_JEDIMASTER;
#endif
}

int CSWClass::IsDroid(int nClass)
{
    return nClass == CLASS_TYPE_COMBATDROID || nClass == CLASS_TYPE_EXPERTDROID;
}

// Rebuilds m_paRecommendedSpells from spells.2da, one entry per row carrying the row index plus its
// Light_Recom, Dark_Recom and UserType columns. A failed load only builds a diagnostic string and
// falls through, so the table is still sized and filled from the empty array.
//
// The table loaders below all follow the same pattern: open a 2DA, build a diagnostic string on
// failure (never printed), replace the member array, then fill it row by row.

// classes.2da "AttackBonusTable"; the column inside it is "BAB".
// 60 one-based levels, each written unconditionally: a failed lookup stores 0.
void CSWClass::LoadAttackBonusTable(const CExoString& sTable)
{
    int nValue;
    CExoString sError;
    C2DA* p2DA = new C2DA(CResRef(sTable), 0);
    if (!p2DA->Load2DArray()) {
        CExoString sMessage;
        sMessage = CExoString("CSWClass::LoadAttackBonusTable: Can't load") + sTable;
        sMessage = sMessage + CExoString(" 2DA file");
    }

    for (int i = 0; i < 60; i++) {
        int bFound = p2DA->GetINTEntry(i, CExoString("BAB"), &nValue);
        m_aAttackBonus[i] = bFound ? static_cast<unsigned char>(nValue) : static_cast<unsigned char>(0);
    }

    delete p2DA;
}

// featgain.2da, opened by literal: the column names are the argument plus a "_REG"/"_BON" suffix.
// Writes are conditional here (a missing entry leaves the slot alone) and the slots are one-based,
// while the readers index with the raw class level.
void CSWClass::LoadFeatGain(const CExoString& sTable)
{
    C2DA* p2DA = new C2DA(CResRef("featgain"), 0);
    if (!p2DA->Load2DArray()) {
        CExoString sError("CSWClass::LoadFeatGain: can't load featgain.2da");
    }

    CExoString sNormal = sTable + CExoString("_REG");
    CExoString sBonus = sTable + CExoString("_BON");

#if KOTOR1
    for (int i = 0; i < 20; i++) {
#else
    for (int i = 0; i < 50; i++) {
#endif
        int nValue;
        if (p2DA->GetINTEntry(i, sNormal, &nValue))
            m_normalFeats.m_aByLevel[i + 1] = static_cast<unsigned char>(nValue);
        if (p2DA->GetINTEntry(i, sBonus, &nValue))
            m_bonusFeats.m_aByLevel[i + 1] = static_cast<unsigned char>(nValue);
    }

    delete p2DA;
}

// feat.2da: one CSWClassGrantedFeat per row the rules resolve to a feat; the row index is the feat
// id. Four columns are built from the argument as a prefix: "_List" (a 0..3 selector mapped onto
// m_nFeatList), "_Granted" (the level), "_PC_Granted" (a flag) and "_Recom" (the chargen
// recommendation order). An unresolved feat skips the row, so the destination index lags the row.
void CSWClass::LoadFeatsTable(const CExoString& sTable, CSWRules* pRules)
{
    C2DA* p2DA = new C2DA(CResRef("Feat"), 0);
    if (!p2DA->Load2DArray()) {
        CExoString sError;
        sError = CExoString("CSWClass::LoadFeatTable: Can't load feat.2da");
        sError = sError + CExoString(" 2DA file");
    }

    if (m_paGrantedFeats) {
        delete[] m_paGrantedFeats;
        m_paGrantedFeats = 0;
    }

    m_nNumGrantedFeats = static_cast<unsigned short>(p2DA->m_nRows);
    m_paGrantedFeats = new CSWClassGrantedFeat[m_nNumGrantedFeats];

    int nValue;
    int nEntry = 0;
    for (unsigned short i = 0; i < m_nNumGrantedFeats; i++) {
        // Both slots are cleared because they diverge once a feat fails to resolve: the destination
        // slot so a skipped row still reads as empty, the source slot because the columns below only
        // fill it on success.
#if KOTOR1
        // K1 names the source slot's feat before the resolution test.
        m_paGrantedFeats[i].m_nFeatList = 0;
        m_paGrantedFeats[i].m_nLevel = 0;
        m_paGrantedFeats[i].m_nFeat = 0;
        m_paGrantedFeats[nEntry].m_nFeatList = 0;
        m_paGrantedFeats[nEntry].m_nLevel = 0;
        m_paGrantedFeats[nEntry].m_nFeat = i;
#else
        m_paGrantedFeats[nEntry].m_bPCOnly = 0;
        m_paGrantedFeats[nEntry].m_nFeatList = 0;
        m_paGrantedFeats[nEntry].m_nLevel = 0;
        m_paGrantedFeats[nEntry].m_nFeat = 0;
        m_paGrantedFeats[i].m_bPCOnly = 0;
        m_paGrantedFeats[i].m_nFeatList = 0;
        m_paGrantedFeats[i].m_nLevel = 0;
        m_paGrantedFeats[i].m_nFeat = 0;
#endif

        if (pRules->GetFeat(i) == 0)
            continue;

        p2DA->GetINTEntry(i, sTable + CExoString("_List"), &nValue);
        switch (static_cast<unsigned char>(nValue)) {
        case 0:
            m_paGrantedFeats[nEntry].m_nFeatList = 1;
            break;
        case 1:
            m_paGrantedFeats[nEntry].m_nFeatList = 3;
            break;
        case 2:
            m_paGrantedFeats[nEntry].m_nFeatList = 2;
            break;
        case 3:
            m_paGrantedFeats[nEntry].m_nFeatList = 0;
            break;
        case 4:
            // List value 4 skips the row without consuming a slot.
            continue;
        }

#if !KOTOR1
        m_paGrantedFeats[nEntry].m_nFeat = i;
#endif

        p2DA->GetINTEntry(i, sTable + CExoString("_Granted"), &nValue);
        m_paGrantedFeats[nEntry].m_nLevel = static_cast<unsigned char>(nValue);

#if !KOTOR1
        p2DA->GetINTEntry(i, sTable + CExoString("_PC_Granted"), &nValue);
        m_paGrantedFeats[nEntry].m_bPCOnly = static_cast<unsigned char>(nValue);
#endif

        p2DA->GetINTEntry(i, sTable + CExoString("_Recom"), &nValue);
        m_paGrantedFeats[nEntry].m_nRecommendedOrder = nValue;

        nEntry++;
    }

    delete p2DA;
}

// classes.2da "SavingThrowTable"; the three columns are literal. 60 one-based levels, every write
// unconditional.
void CSWClass::LoadSavingThrowTable(const CExoString& sTable)
{
    int nValue;
    C2DA* p2DA = new C2DA(CResRef(sTable), 0);
    if (!p2DA->Load2DArray()) {
        CExoString sError;
        sError = CExoString("CSWClass::LoadSavingThrowTable: Can't load") + sTable;
        sError = sError + CExoString(" 2DA file");
    }

    for (int i = 0; i < 60; i++) {
        p2DA->GetINTEntry(i, CExoString("FortSave"), &nValue);
        m_aFortSaveBonus[i] = static_cast<unsigned char>(nValue);
        p2DA->GetINTEntry(i, CExoString("WillSave"), &nValue);
        m_aWillSaveBonus[i] = static_cast<unsigned char>(nValue);
        p2DA->GetINTEntry(i, CExoString("RefSave"), &nValue);
        m_aRefSaveBonus[i] = static_cast<unsigned char>(nValue);
    }

    delete p2DA;
}

// skills.2da, opened by literal. The entry's leading row index is the skill id; "_Class" fills
// m_bClassSkill and "_Reco" fills the chargen recommendation order, which defaults to -1 when the column
// has no entry for the row.
void CSWClass::LoadSkillsTable(const CExoString& sTable)
{
    C2DA* p2DA = new C2DA(CResRef("skills"), 0);
    if (!p2DA->Load2DArray()) {
        CExoString sError;
        sError = CExoString("CSWClass::LoadSkillsTable: Can't load skills.2da");
    }

    if (m_paClassSkills) {
        delete[] m_paClassSkills;
        m_paClassSkills = 0;
    }

    m_nNumClassSkills = static_cast<unsigned short>(p2DA->m_nRows);
    m_paClassSkills = new CSWClassSkill[m_nNumClassSkills];

    for (unsigned short i = 0; i < m_nNumClassSkills; i++) {
        int nValue;
        m_paClassSkills[i].m_nSkill = i;
        p2DA->GetINTEntry(i, sTable + CExoString("_Class"), &nValue);
        m_paClassSkills[i].m_bClassSkill = nValue;
        if (p2DA->GetINTEntry(i, sTable + CExoString("_Reco"), &nValue))
            m_paClassSkills[i].m_nRecommendedOrder = nValue;
        else
            m_paClassSkills[i].m_nRecommendedOrder = -1;
    }

    delete p2DA;
}

// Loads two 2DAs in one body. One is cls_spgn_jedi, whose "NumSpellLevels" column is the per-level
// count of spell levels, the other classpowergain, read with the argument as the column name. The
// argument must be a non-empty string or the whole loader is skipped.
void CSWClass::LoadSpellGainTable(const CExoString& sTable)
{
    int nValue;
    if (sTable.GetLength() < 1)
        return;

    C2DA* p2DAJedi = new C2DA(CResRef("CLS_SPGN_JEDI"), 0);
    if (!p2DAJedi->Load2DArray()) {
        CExoString sError;
        sError = CExoString("CSWClass::LoadSpellGainTable: Can't load CLS_SPGN_JEDI");
        sError = sError + CExoString(" 2DA file");
    }

    C2DA* p2DAGain = new C2DA(CResRef("ClassPowerGain"), 0);
    if (!p2DAGain->Load2DArray()) {
        CExoString sError;
        sError = CExoString("CSWClass::LoadSpellGainTable: Can't load ClassPowerGain");
        sError = sError + CExoString(" 2DA file");
    }

#if KOTOR1
    for (int i = 0; i < 20; i++) {
#else
    for (int i = 0; i < 50; i++) {
#endif
        p2DAJedi->GetINTEntry(i, CExoString("NumSpellLevels"), &nValue);
        m_aSpellGainMinLevel[i] = static_cast<unsigned char>(nValue);
        for (int j = 0; j < m_aSpellGainMinLevel[i]; j++) {
            if (p2DAGain->GetINTEntry(i, sTable, &nValue))
                m_aSpellGain[i] = static_cast<unsigned char>(nValue);
            else
                m_aSpellGain[i] = static_cast<unsigned char>(-1);
        }
    }

    delete p2DAGain;
    delete p2DAJedi;
}

// classes.2da "SpellsKnownTable". The per-row allocation is sized and bounded by
// m_aSpellGainMinLevel, and each slot's column name is formatted in place.
void CSWClass::LoadSpellKnownTable(const CExoString& sTable)
{
    CExoString sLevel;
    int nValue;
    C2DA* p2DA = new C2DA(CResRef(sTable), 0);
    if (!p2DA->Load2DArray()) {
        CExoString sError;
        sError = CExoString("CSWClass::LoadSpellKnownTable: Can't load") + sTable;
        sError = sError + CExoString(" 2DA file");
    }

    if (m_paSpellsKnownPerLevel) {
#if KOTOR1
        for (int i = 0; i < 20; i++) {
#else
        for (int i = 0; i < 50; i++) {
#endif
            if (m_paSpellsKnownPerLevel[i]) {
                delete[] m_paSpellsKnownPerLevel[i];
                m_paSpellsKnownPerLevel[i] = 0;
            }
        }
        delete[] m_paSpellsKnownPerLevel;
        m_paSpellsKnownPerLevel = 0;
    }

#if KOTOR1
    m_paSpellsKnownPerLevel = new unsigned char*[20];
#else
    m_paSpellsKnownPerLevel = new unsigned char*[50];
#endif

#if KOTOR1
    for (int i = 0; i < 20; i++) {
#else
    for (int i = 0; i < 50; i++) {
#endif
        if (m_aSpellGainMinLevel[i] == 0)
            continue;
        m_paSpellsKnownPerLevel[i] = new unsigned char[m_aSpellGainMinLevel[i]];
        for (int j = 0; j < m_aSpellGainMinLevel[i]; j++) {
            sLevel.Format(const_cast<char*>("SpellLevel%d"), j);
            m_paSpellsKnownPerLevel[i][j] = p2DA->GetINTEntry(i, sLevel, &nValue)
                                                ? static_cast<unsigned char>(nValue)
                                                : static_cast<unsigned char>(0);
        }
    }

    delete p2DA;
}

// acbonus.2da, opened by literal: the argument is the column (one class's own AC progression) and the
// table is a flat 51-entry per-level run.
void CSWClass::LoadArmorClassTable(const CExoString& sTable)
{
    int nValue;
    C2DA* p2DA = new C2DA(CResRef("ACBonus"), 0);
    if (!p2DA->Load2DArray()) {
        CExoString sError;
        sError = CExoString("CSWClass::LoadArmorClassTable: Can't load ACBonus");
        sError = sError + CExoString(" 2DA file");
    }

    if (m_pArmorBonusTable) {
        delete[] m_pArmorBonusTable;
        m_pArmorBonusTable = 0;
    }

#if KOTOR1
    m_pArmorBonusTable = new unsigned char[21];

    for (int i = 0; i < 21; i++)
#else
    m_pArmorBonusTable = new unsigned char[51];

    for (int i = 0; i < 51; i++)
#endif
        m_pArmorBonusTable[i] = p2DA->GetINTEntry(i, sTable, &nValue)
                                   ? static_cast<unsigned char>(nValue)
                                   : static_cast<unsigned char>(0);

    delete p2DA;
}

void CSWClass::LoadSpellsTable()
{
    C2DA* p2DA = new C2DA(CResRef("spells"), 0);
    if (!p2DA->Load2DArray()) {
        CExoString sError;
        sError = CExoString("CSWClass::LoadSpellsTable: Can't load spells.2da");
    }

    if (m_paRecommendedSpells) {
        delete[] m_paRecommendedSpells;
        m_paRecommendedSpells = 0;
    }

    m_nNumRecommendedSpells = static_cast<unsigned short>(p2DA->m_nRows);
    m_paRecommendedSpells = new CSWClassSpell[m_nNumRecommendedSpells];

    for (unsigned short i = 0; i < m_nNumRecommendedSpells; i++) {
        int nValue;
        m_paRecommendedSpells[i].m_nSpellId = i;
        p2DA->GetINTEntry(i, CExoString("Light_Recom"), &nValue);
        m_paRecommendedSpells[i].m_nLightRecommended = nValue;
        p2DA->GetINTEntry(i, CExoString("Dark_Recom"), &nValue);
        m_paRecommendedSpells[i].m_nDarkRecommended = nValue;
        p2DA->GetINTEntry(i, CExoString("UserType"), &nValue);
        m_paRecommendedSpells[i].m_nUserType = static_cast<unsigned char>(nValue);
    }

    delete p2DA;
}

// Frees the five owned allocations; m_paSpellsKnownPerLevel is two-level, so each non-null row is
// freed before the array that holds them.
CSWClass::~CSWClass()
{
    if (m_paSpellsKnownPerLevel) {
#if KOTOR1
        for (int i = 0; i < 20; i++) {
#else
        for (int i = 0; i < 50; i++) {
#endif
            if (m_paSpellsKnownPerLevel[i]) {
                delete[] m_paSpellsKnownPerLevel[i];
                m_paSpellsKnownPerLevel[i] = 0;
            }
        }
        delete[] m_paSpellsKnownPerLevel;
        m_paSpellsKnownPerLevel = 0;
    }
    if (m_paClassSkills) {
        delete[] m_paClassSkills;
        m_paClassSkills = 0;
    }
    if (m_paGrantedFeats) {
        delete[] m_paGrantedFeats;
        m_paGrantedFeats = 0;
    }
    if (m_pArmorBonusTable) {
        delete[] m_pArmorBonusTable;
        m_pArmorBonusTable = 0;
    }
    if (m_paRecommendedSpells) {
        delete[] m_paRecommendedSpells;
        m_paRecommendedSpells = 0;
    }
}
