#include "k2stdint.h"
#include "nwccreaturestats.h"
#include "nwsrules.h"
#include "nwskill.h"
#include "nwrace.h"
#include "nwclass.h"
#include "nwccreature.h"
#include "nwscreature.h"
#include "nwsplaceable.h"
#include "exoresman.h"
#include "CExoArrayListImpl.h"
#include "nwcobject.h"
#include "exolocstringinternal.h"
#include "resgff.h"
#include "nwrules.h"
#include "globalvartables.h"
#include "serverexoapp.h"
#include "nwcitem.h"
#include "nwitem.h"
#include "nwsitem.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwsmodule.h"
#include "nwsarea.h"
#include "Achievements.h"

#include "port/libcstring.h"
#include "port/libcmath.h"

#include "nwspell.h"
#include "nwscriptconst.h"
#include "nwmessagetypes.h"
#include "nwportrait.h"
#include "strref.h"
#include "exostringinline.h"

// CSWGuiInterfaceAction::m_nFlags: bit 0 enables the entry, bits 1-4 say why it is disabled (the
// main interface's hint strings: force depleted, restricted by armor, missing item, target too close,
// full health, party leader unavailable).
enum {
    ACTION_FLAG_ENABLED                       = 0x01,
    ACTION_FLAG_REASON_MASK                   = 0x1e,
    ACTION_FLAG_REASON_FORCE_DEPLETED         = 0x02,
    ACTION_FLAG_REASON_RESTRICTED_BY_ARMOR    = 0x04,
    ACTION_FLAG_REASON_MISSING_ITEM           = 0x06,
    ACTION_FLAG_REASON_TARGET_TOO_CLOSE       = 0x08,
    ACTION_FLAG_REASON_FULL_HEALTH            = 0x0a,
    ACTION_FLAG_REASON_PARTY_LEADER_UNAVAILABLE = 0x0c,
    ACTION_FLAG_REASON_NOT_ENOUGH_HEALTH      = 0x0e,
};

// CCombatInformationNode::m_nType, the attack a modifier applies to (the server writes it from the
// effect's ATTACK_BONUS_* / DAMAGE_BONUS_* kind), and the kinds the character sheet lists.
enum {
    COMBAT_INFO_ONHAND  = 1,
    COMBAT_INFO_OFFHAND = 2,
    COMBAT_INFO_MISC    = 6,
    COMBAT_INFO_UNARMED = 7,
    COMBAT_INFO_LISTED  = (1 << COMBAT_INFO_ONHAND) | (1 << COMBAT_INFO_OFFHAND) |
                          (1 << COMBAT_INFO_MISC) | (1 << COMBAT_INFO_UNARMED),
};

// The feat.2da grant level of a feat a companion never receives.
enum {
    GRANT_LEVEL_NEVER = 0xff,
};

#if KOTOR1
// The class-skill feat rows HasClassSkillFeat tests; KOTOR 1's nwscript.nss has no names for them
// (CSWSCreatureStats carries the same values class-scoped).
enum {
    FEAT_CLASS_SKILL_AWARENESS    = 183,
    FEAT_CLASS_SKILL_COMPUTER_USE = 184,
    FEAT_CLASS_SKILL_DEMOLITIONS  = 185,
    FEAT_CLASS_SKILL_REPAIR       = 186,
    FEAT_CLASS_SKILL_SECURITY     = 187,
    FEAT_CLASS_SKILL_STEALTH      = 188,
    FEAT_CLASS_SKILL_TREAT_INJURY = 189,
};
#endif

#if KOTOR1
CSWCCreatureStats::CSWCCreatureStats()
#else
CSWCCreatureStats::CSWCCreatureStats(CSWCCreature* pCreature)
#endif
{
    m_name = "";
    m_sLastName = "";
    m_nStartingPackage = 0;
    m_nAge = 0;
    m_nRace = CSWRace::CRACE_HUMAN;
    m_raceString = "";
    m_nSubraceIndex = 0;
    m_conversation = const_cast<char*>("");

    m_nGender = 0;
    m_nNumClasses = 1;
    m_classes[0].m_nLevel = 1;
    m_classes[1].m_nClass = CLASS_TYPE_INVALID;
    m_classes[1].m_nLevel = 0;

    m_abScores[0] = 8;
    m_abScores[1] = 8;
    m_abScores[2] = 8;
    m_abScores[3] = 8;
    m_abScores[4] = 8;
    m_abScores[5] = 8;
    m_abScoresBase[0] = 0;
    m_abScoresBase[1] = 0;
    m_abScoresBase[2] = 0;
    m_abScoresBase[3] = 0;
    m_abScoresBase[4] = 0;
    m_abScoresBase[5] = 0;
    m_nDEXArmorAdjust = 0;
    m_abScoreBonuses[0] = 0;
    m_abScoreBonuses[1] = 0;
    m_abScoreBonuses[2] = 0;
    m_abScoreBonuses[3] = 0;
    m_abScoreBonuses[4] = 0;
    m_abScoreBonuses[5] = 0;
    m_nHitPointsBase = 0;
    m_nHitPointBonus = 0;
    m_nCurrentHitPoints = 0;
    m_nMaxHitPoints = 0;
    m_nArmorClass = 0;
    m_nArmorClassSent = 0;

    m_fChallengeRatingOverride = 6.0f;
    m_nExperience = 0;
    m_nFortSavingThrow = 0;
    m_nWillSavingThrow = 0;
    m_nReflexSavingThrow = 0;
    m_bUseMonkAttackTables = 0;
    m_alignmentGoodEvil = 50;
#if !KOTOR1
    m_baseCNPCAlignment = -1;
#endif
    m_nBaseAttackBonus = 0;

    m_nAppearanceType = m_nRace;
    m_classes[0].m_nClass = CSWClass::CCLASS_SOLDIER;

    m_pSkillRanks = new unsigned char[g_pRules->m_nNumSkills];
    m_pSkillRanksBase = new unsigned char[g_pRules->m_nNumSkills];
    memset(m_pSkillRanks, 0, g_pRules->m_nNumSkills);  // idiom-exempt: dynamic byte count == the new[] alloc above, not a struct copy
    memset(m_pSkillRanksBase, 0, g_pRules->m_nNumSkills);  // idiom-exempt: dynamic byte count == the new[] alloc above, not a struct copy

    m_pCombatInformation = new CCombatInformation();
    m_bCombatInformationSet = 0;

    m_bTail = 0;
    m_bWings = 0;
    m_nColorSkin = 0;
    m_nColorHair = 0;
    m_nColorTattoo1 = 0;
    m_nColorTattoo2 = 0;
    m_nPhenotype = 0;
    m_nAppearanceHead = 0;
    m_portraitId = CSWPortrait::PORTRAIT_ID_NONE;
    m_nMaxForcePoints = 0;
    m_nForcePoints = 0;
    m_nCurrentForcePoints = 0;
    m_nTemporaryForcePoints = 0;
    m_bDuplicatingHead = 0xff;
    m_nUseBackupHead = 0;
#if !KOTOR1
    m_pCreature = pCreature;
#endif
}

CSWCCreatureStats::~CSWCCreatureStats()
{
    if (m_pSkillRanks) {
        delete[] m_pSkillRanks;
        m_pSkillRanks = 0;
    }
    if (m_pSkillRanksBase) {
        delete[] m_pSkillRanksBase;
        m_pSkillRanksBase = 0;
    }
    ClearFeats();
    if (m_pCombatInformation) {
        delete m_pCombatInformation;
        m_pCombatInformation = 0;
    }
    // m_classes[2] is left to its own destructor.
}

unsigned short CSWCCreatureStats::GetMaxHitPoints()
{
    return m_nMaxHitPoints;
}

int CSWCCreatureStats::CheckCustomPortrait(CResRef* pResRef, int nGender)
{
    CSWPortrait portrait;
    portrait.SetPortrait(*pResRef);
    if (g_pExoResMan->Exists(portrait.GetPortrait(3), 3, static_cast<unsigned int*>(0)) == 0) {
        if (nGender == GENDER_MALE)
            *pResRef = const_cast<char*>("po_hu_m_99_");
        else
            *pResRef = const_cast<char*>("po_hu_f_99_");
        return 0;
    }
    return 1;
}

void CSWCCreatureStats::SetPortraitId(unsigned short a)
{
    CSWPortrait portrait;
    if (portrait.SetPortraitId(a))
        m_portrait.SetPortrait(portrait.GetPortrait(0));
    m_portraitId = a;
}

int CSWCCreatureStats::CalculateCriticalHitRange()
{
    return 1;
}

int CSWCCreatureStats::GetArmorClass(int nType)
{
#if KOTOR1
    K2_UNUSED(nType);
    return m_nArmorClass;
#else
    signed char nBonus = 0;
    if (nType == 1 && m_nNumClasses != 0) {
        uint64_t i = 0;
        do {
            K2_ASSUME(i < 2);   // m_nNumClasses is at most 2, the class slots (server and GFF clamp it)
            uint64_t nNumKnown = static_cast<unsigned short>(m_classes[i].m_spellLevels[0].m_count);
            if (nNumKnown != 0) {
                unsigned int* pKnown = m_classes[i].m_spellLevels[0].m_pArray;
                do {
                    if (*pKnown == FORCE_POWER_BATTLE_PRECOGNITION) {
                        CSWSCreature* pServer = m_pCreature->GetServerCreature();
                        if (pServer != 0) {
                            CSWItem* pItem = pServer->m_pInventory->GetItemInSlot(CSWInventory::SLOT_BODY);
                            CSWBaseItem* pBase = (pItem != 0) ? pItem->GetBaseItem() : 0;
                            if (pItem == 0 || (pBase != 0 && pBase->m_nBaseAC == 0)) {
                                nBonus = (GetWISMod() < 0) ? 0 : GetWISMod();
                            }
                        }
                        goto done;
                    }
                    pKnown++;
                } while (--nNumKnown != 0);
            }
            i++;
        } while (i != m_nNumClasses);
    }
done:
    return m_nArmorClass + static_cast<unsigned char>(nBonus);
#endif
}

int CSWCCreatureStats::GetMenuInfoCount(unsigned int nType)
{
    int nCount = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++)
        nCount += m_classes[i].GetMenuInfoCount(nType);
    return nCount;
}

unsigned char CSWCCreatureStats::GetLevel()
{
    int nLevel = 0;
#if KOTOR1
    for (unsigned char i = 0; i < m_nNumClasses; i++)
        nLevel = nLevel + GetClassLevel(i);
#else
    for (unsigned char i = 0; i < m_nNumClasses; i++)
        nLevel += i < 2 ? m_classes[i].m_nLevel : 0;

    if (m_pCreature) {
        CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
        if (pServerCreature && pServerCreature->m_pStats->m_bIsPC == 0 &&
            pServerCreature->m_bInParty == 0 && pServerCreature->m_nMultiplierSet != 0)
        {
            float fLevelMult;
            g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(pServerCreature->m_nMultiplierSet, CExoString("LevelMult"), &fLevelMult);

            unsigned char nPCLevel = pServerCreature->m_nPCLevelAtSpawn;
            if (nPCLevel == 0) {
                CSWGlobalVariableTable* pTable = g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
                CExoString name("G_PC_LEVEL");
                // Only the char& overload of GetValueNumber exists, so the cast is required.
                pTable->GetValueNumber(name, reinterpret_cast<char&>(nPCLevel));
            }

            unsigned char nMult = static_cast<signed char>(nPCLevel) < 2 ? 1 : nPCLevel;
            nLevel = nLevel + static_cast<int>(fLevelMult * static_cast<float>(nMult)) - 1;
            if (nLevel < 0)
                nLevel = 1;
        }
    }
#endif
    return static_cast<unsigned char>(nLevel);
}

int CSWCCreatureStats::GetExpNeededForLevelUp()
{
    return g_pRules->m_aExpTable[GetLevel()];
}

int CSWCCreatureStats::GetCasterLevel(unsigned char nClassPos)
{
    if (nClassPos > 1)
        return 0;
    if (g_pRules->m_paClasses[GetClass(nClassPos)].m_bSpellCaster == 0)
        return 0;
    return GetClassLevel(nClassPos);
}

int CSWCCreatureStats::CalculateUnarmedDamageDice()
{
    int nSizeCategory;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        g_pRules->m_paRaces[m_nRace].m_nAppearance,
        CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSizeCategory);
    return 0;
}

int CSWCCreatureStats::CalculateUnarmedDamageDie()
{
    int nSizeCategory;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        g_pRules->m_paRaces[m_nRace].m_nAppearance,
        CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSizeCategory);
    return 0;
}

signed char CSWCCreatureStats::CalcStatModifier(unsigned char nStat)
{
    return static_cast<signed char>(static_cast<int>(floor(nStat * 0.5)) - 5);
}

int CSWCCreatureStats::CalculateAttackModifier()
{
    return GetSTRMod();
}

int CSWCCreatureStats::CalculateDamageModifier()
{
    return GetSTRMod();
}

unsigned char CSWCCreatureStats::GetClass(unsigned char nClassPos)
{
    if (nClassPos > 1)
        return CLASS_TYPE_INVALID;
    return m_classes[nClassPos].m_nClass;
}

unsigned char CSWCCreatureStats::GetClassLevel(unsigned char nClassPos)
{
    if (nClassPos > 1)
        return 0;
    return m_classes[nClassPos].m_nLevel;
}

void CSWCCreatureStats::SetClass(unsigned char nClassPos, unsigned char nClass)
{
    if (nClassPos > 1)
        return;
    m_classes[nClassPos].m_nClass = nClass;
}

void CSWCCreatureStats::SetClassLevel(unsigned char nClassPos, unsigned char nLevel)
{
    if (nClassPos > 1)
        return;
    m_classes[nClassPos].m_nLevel = nLevel;
}

unsigned short CSWCCreatureStats::GetFeat(unsigned short nFeat)
{
    if (m_lstFeats.m_count <= nFeat)
        return 0xffff;
    return m_lstFeats.m_pArray[nFeat];
}

int CSWCCreatureStats::HasFeat(unsigned short nFeat)
{
    for (int i = 0; i < m_lstFeats.m_count; i++)
        if (m_lstFeats.m_pArray[i] == nFeat)
            return 1;
    return 0;
}

int CSWCCreatureStats::HasClassSkillFeat(unsigned short nSkill)
{
    unsigned short nFeat = 0xffff;
    switch (nSkill) {
        case SKILL_COMPUTER_USE: nFeat = FEAT_CLASS_SKILL_COMPUTER_USE; break;
        case SKILL_DEMOLITIONS:  nFeat = FEAT_CLASS_SKILL_DEMOLITIONS; break;
        case SKILL_STEALTH:      nFeat = FEAT_CLASS_SKILL_STEALTH; break;
        case SKILL_AWARENESS:    nFeat = FEAT_CLASS_SKILL_AWARENESS; break;
        case SKILL_REPAIR:       nFeat = FEAT_CLASS_SKILL_REPAIR; break;
        case SKILL_SECURITY:     nFeat = FEAT_CLASS_SKILL_SECURITY; break;
        case SKILL_TREAT_INJURY: nFeat = FEAT_CLASS_SKILL_TREAT_INJURY; break;
    }
    return FeatAcquired(nFeat);
}

int CSWCCreatureStats::GetFeatRemainingUses(unsigned short nFeat)
{
    for (int64_t i = 0; i < m_lstFeatUses.m_count; i++)
        if (m_lstFeatUses.m_pArray[i]->m_nFeat == nFeat)
            return m_lstFeatUses.m_pArray[i]->m_nRemainingUses;
    if (FeatAcquired(nFeat))
        return 100;
    return 0;
}

void CSWCCreatureStats::SetFeatRemainingUses(unsigned short nFeat, unsigned char nUses)
{
    for (int64_t i = 0; i < m_lstFeatUses.m_count; i++) {
        if (m_lstFeatUses.m_pArray[i]->m_nFeat == nFeat) {
            m_lstFeatUses.m_pArray[i]->m_nRemainingUses = nUses;
            return;
        }
    }
    CSWCCreatureStats_FeatUse* pFeatUse = new CSWCCreatureStats_FeatUse;
    pFeatUse->m_nFeat = nFeat;
    pFeatUse->m_nRemainingUses = nUses;

    m_lstFeatUses.Add(pFeatUse);
}

unsigned short CSWCCreatureStats::GetBonusFeat(unsigned short nFeat)
{
    if (m_lstBonusFeats.m_count <= nFeat)
        return 0xffff;
    return m_lstBonusFeats.m_pArray[nFeat];
}

int CSWCCreatureStats::GetCanUseSkill(unsigned short nSkill)
{
    if (g_pRules->m_nNumSkills <= nSkill)
        return 0;
    CSWSkill* paSkills = g_pRules->m_paSkills;
#if !KOTOR1  // K1 has no power-based Stealth unlock
    if (nSkill == SKILL_STEALTH) {
        if (m_nNumClasses != 0) {
            for (int64_t i = 0; i < m_nNumClasses; i++) {
                unsigned int* pKnown = m_classes[i].m_spellLevels[0].m_pArray;
                for (int64_t n = static_cast<unsigned short>(m_classes[i].m_spellLevels[0].m_count); n != 0; n--)
                    if (*pKnown++ == FORCE_POWER_FORCE_CAMOUFLAGE)
                        return 1;
            }
            for (int64_t i = 0; i < m_nNumClasses; i++) {
                unsigned int* pKnown = m_classes[i].m_spellLevels[0].m_pArray;
                for (int64_t n = static_cast<unsigned short>(m_classes[i].m_spellLevels[0].m_count); n != 0; n--)
                    if (*pKnown++ == FORCE_POWER_IMPROVED_FORCE_CAMOUFLAGE)
                        return 1;
            }
            for (int64_t i = 0; i < m_nNumClasses; i++) {
                unsigned int* pKnown = m_classes[i].m_spellLevels[0].m_pArray;
                for (int64_t n = static_cast<unsigned short>(m_classes[i].m_spellLevels[0].m_count); n != 0; n--)
                    if (*pKnown++ == FORCE_POWER_MASTER_FORCE_CAMOUFLAGE)
                        return 1;
            }
        }
        for (int64_t i = 0; i < m_lstFeats.m_count; i++)
            if (m_lstFeats.m_pArray[i] == FEAT_PERSONAL_CLOAKING_SHIELD)
                return 1;
    }
#endif
    if ((paSkills[nSkill].m_flags & 2) == 0)
        return static_cast<signed char>(m_pSkillRanksBase[nSkill]) > 0;
    if (m_nNumClasses == 0)
        return 0;
    for (int64_t i = 0; i < m_nNumClasses; i++) {
        unsigned int nClass = i <= 1 ? m_classes[i].m_nClass : CLASS_TYPE_INVALID;
        if (g_pRules->m_paClasses[nClass].IsSkillUseable(nSkill))
            return 1;
    }
    return 0;
}

int CSWCCreatureStats::SpontaneousCastUsesLeft(unsigned char nClassPos, unsigned char nSpellLevel)
{
    K2_UNUSED(nClassPos); K2_UNUSED(nSpellLevel);
    return 0;
}

int CSWCCreatureStats::GetSpellMinAbilityMet(unsigned char nClassPos, unsigned char nSpellLevel)
{
    K2_UNUSED(nClassPos); K2_UNUSED(nSpellLevel);
    return 0;
}

int CSWCCreatureStats::GetAlignmentStringRef(int nAlignment)
{
    K2_UNUSED(nAlignment);
    return 0;
}

void CSWCCreatureStats::PayoutPowerAchievements()
{
    static const int aLightPowers[] = { 17, 137, 180, 36, 134, 175, 44, 13, -1 };
    static const int aDarkPowers[]  = { 155, 11, 31, 177, 161, 25, 38, 32, -1 };
    const int* apPowers[2] = { aLightPowers, aDarkPowers };

    // An achievement pays out when either class slot knows every power on its list.
    for (int nList = 0; nList < 2; nList++) {
        const int* pPowers = apPowers[nList];
        int i;
        for (i = 0; pPowers[i] != -1; i++) {
            if (!m_classes[0].m_spellLevels[0].ContainsFast(static_cast<unsigned int>(pPowers[i])) &&
                !m_classes[1].m_spellLevels[0].ContainsFast(static_cast<unsigned int>(pPowers[i])))
                break;
        }
        if (pPowers[i] == -1) {
            switch (nList) {
            case 0:
                Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_ALL_LIGHT_SIDE_POWERS);
                break;
            case 1:
                Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_ALL_DARK_SIDE_POWERS);
                break;
            }
        }
    }
}


CExoString CSWCCreatureStats::GetCreatureChallengeRatingColor(CSWCCreature *pCreature)
{
    K2_UNUSED(pCreature);
    return "";
}

CExoString CSWCCreatureStats::GetClassesString(char cSeparator)
{
    CExoString strReturn;
    char szSeparator[2] = { 0, 0 };

    for (unsigned char i = 0; i < m_nNumClasses; i++) {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClass(i)];
        if (pClass) {
            strReturn = strReturn + szSeparator + pClass->GetNameText() + " (" + CExoString(GetClassLevel(i)) + ")";
            szSeparator[0] = cSeparator;
        }
    }
    return strReturn;
}

int CSWCCreatureStats::GetChallengeRatingbyCR(float fCR)
{
    unsigned char nLevel = GetLevel();
    int nTier;

    // A flat chain of half-open brackets: a NaN CR fails every test and lands on tier 5.
    if (fCR >= 1.0f)
        nTier = static_cast<int>(fCR) + 5;
    else if (fCR < 0.15f)
        nTier = 1;
    else if (fCR >= 0.15f && fCR < 0.2f)
        nTier = 2;
    else if (fCR >= 0.2f && fCR < 0.3f)
        nTier = 3;
    else
        nTier = (fCR >= 0.3f && fCR < 0.45f) ? 4 : 5;
    if (nTier < nLevel)
        return 0;

    if (nTier < nLevel + 2)
        return 1;
    if (nTier < nLevel + 4)
        return 2;
    if (nTier < nLevel + 6)
        return 3;
    if (nTier < nLevel + 8)
        return 4;
    if (nTier >= nLevel + 10)
        return 6;
    return 5;
}

int CSWCCreatureStats::GetCreatureChallengeRating(CSWCCreature *pCreature)
{
    CSWCCreatureStats *pStats = pCreature->m_pStats;
    float fCR = pStats->m_fChallengeRatingOverride;
    if (fCR == 0.0f)
        fCR = static_cast<float>(pStats->GetLevel()) + 5.0f;
    return GetChallengeRatingbyCR(fCR);
}

#if KOTOR1
CExoString CSWCCreatureStats::GetFullName()
{
    if (m_name.GetLength() > 0 && m_sLastName.GetLength() > 0)
        return m_name + CExoString(" ") + m_sLastName;
    if (m_name.GetLength() > 0)
        return m_name;
    if (m_sLastName.GetLength() > 0)
        return m_sLastName;
    return CExoString("");
}
#endif

CExoString CSWCCreatureStats::GetFirstName()
{
    CExoString sName = m_name;
    if (m_name.GetLength() >= 1) {
        int nSpace = m_name.Find(' ', 0);
        if (nSpace != -1)
            sName = m_name.SubString(0, nSpace);
    }
    return sName;
}

CExoString CSWCCreatureStats::GetLastName()
{
    CExoString sName;
    if (m_name.GetLength() >= 1) {
        int nSpace = m_name.Find(' ', 0);
        if (nSpace != -1)
            sName = m_name.SubString(nSpace + 1, -1);
    }
    return sName;
}

CExoString CSWCCreatureStats::GetRaceString()
{
    if (m_raceString == CExoString(""))
        return g_pRules->m_paRaces[m_nRace].GetNameText();
    return m_raceString;
}

CExoString CSWCCreatureStats::GetAlignmentString(int nAlignment)
{
    STR_RES strRes;
    g_pTlkTable->Fetch(GetAlignmentStringRef(nAlignment), strRes, CTlkTable::GENDER_MALE);
    return strRes.text;
}

float CSWCCreatureStats_ClassInfo::GetMinSpellRange(const CExoString& sRange)
{
    float fUnknown;  // intentionally uninitialized
#ifdef K2_FIX_ENGINE_BUGS
    fUnknown = 0.0f;
#endif
    float fRange;
    if (sRange == "T" || sRange == "P")
        fRange = g_pRules->m_fMinSpellRange[1];
    else if (sRange == "S")
        fRange = g_pRules->m_fMinSpellRange[2];
    else if (sRange == "M")
        fRange = g_pRules->m_fMinSpellRange[3];
    else if (sRange == "L")
        fRange = g_pRules->m_fMinSpellRange[4];
    else if (sRange == "W")
        fRange = g_pRules->m_fMinSpellRange[5];
    else
        return fUnknown;
    return fRange;
}

float CSWCCreatureStats::GetMinSpellRange(const CExoString& sRange)
{
    float fUnknown;  // intentionally uninitialized
#ifdef K2_FIX_ENGINE_BUGS
    fUnknown = 0.0f;
#endif
    float* pRange;
    if (sRange == "T" || sRange == "P")
        pRange = &g_pRules->m_fMinSpellRange[1];
    else if (sRange == "S")
        pRange = &g_pRules->m_fMinSpellRange[2];
    else if (sRange == "M")
        pRange = &g_pRules->m_fMinSpellRange[3];
    else if (sRange == "L")
        pRange = &g_pRules->m_fMinSpellRange[4];
    else if (sRange == "W")
        pRange = &g_pRules->m_fMinSpellRange[5];
    else
        return fUnknown;
    return *pRange;
}

CResRef CSWCCreatureStats::GetPortrait(int nType)
{
    CExoString sVariant;
    C2DA* pPortraits = g_pRules->m_p2DArrays->m_pPortrait;
    if (pPortraits != 0 && nType != 0) {
        int bFound = 0;
        short nAlignment = m_alignmentGoodEvil;
        sVariant = "";
        if (nAlignment <= 10) {
            CExoString sColumn("BaseResRefVVVE");
            bFound = pPortraits->GetCExoStringEntry(m_portraitId, sColumn, &sVariant);
        } else if (nAlignment <= 20) {
            CExoString sColumn("BaseResRefVVE");
            bFound = pPortraits->GetCExoStringEntry(m_portraitId, sColumn, &sVariant);
        } else if (nAlignment <= 30) {
            CExoString sColumn("BaseResRefVE");
            bFound = pPortraits->GetCExoStringEntry(m_portraitId, sColumn, &sVariant);
        } else if (nAlignment <= 40) {
            CExoString sColumn("BaseResRefE");
            bFound = pPortraits->GetCExoStringEntry(m_portraitId, sColumn, &sVariant);
        }
        if (bFound)
            return sVariant;
    }
    return m_portrait.m_baseResRef;
}

int CSWCCreatureStats::GetBaseAttackBonus()
{
    if (m_bCombatInformationSet == 1)
        return m_nBaseAttackBonus;
    int nBonus = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++) {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClass(i)];
        if (pClass)
            nBonus += pClass->GetAttackBonus(GetClassLevel(i));
    }
    return nBonus;
}

int CSWCCreatureStats::GetModifiedAttackValue(int nAttack)
{
    if (!m_pCombatInformation)
        return 0;
    if (m_bUseMonkAttackTables != 0)
        return static_cast<unsigned char>(GetBaseAttackBonus()) - nAttack * 3;
    return static_cast<unsigned char>(GetBaseAttackBonus()) - nAttack * 5;
}

int CSWCCreatureStats::CalculateNumAttacks()
{
    int nBonus = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++)
        nBonus += g_pRules->m_paClasses[GetClass(i)].GetAttackBonus(GetClassLevel(i));
    int nAttacks = (nBonus + 4) / 5;
    if (nAttacks < 1)
        nAttacks = 1;
    return nAttacks;
}

CExoString CSWCCreatureStats::GetForceResistanceInfo()
{
    CExoString s("");
    if (m_pCombatInformation)
        s.Format(const_cast<char*>("%d"), m_pCombatInformation->m_nForceResistance);
    return s;
}

CExoString CSWCCreatureStats::GetArcaneSpellFailureInfo()
{
    CExoString s("");
    if (m_pCombatInformation)
        s.Format(const_cast<char*>("%d%%"), m_pCombatInformation->m_nArcaneSpellFailure);
    return s;
}

CExoString CSWCCreatureStats::GetArmorCheckPenaltyInfo()
{
    CExoString s("");
    if (m_pCombatInformation) {
        if (m_pCombatInformation->m_bArmorCheckPenalty)
            s.Format(const_cast<char*>("-%d"), m_pCombatInformation->m_bArmorCheckPenalty);
        else
            s.Format(const_cast<char*>("%d"), m_pCombatInformation->m_bArmorCheckPenalty);
    }
    return s;
}

CExoString CSWCCreatureStats::GetCriticalHitInfo(int bOffHand)
{
    CExoString s("");
    STR_RES strRes;
    if (m_pCombatInformation) {
        unsigned char nRange = m_pCombatInformation->m_nCritRangeOffHand;
        unsigned char nMultiplier = m_pCombatInformation->m_nCritMultiplierOffHand;
        if (bOffHand == 0) {
            nRange = m_pCombatInformation->m_nCritRangeOnHand;
            nMultiplier = m_pCombatInformation->m_nCritMultiplierOnHand;
        }
        g_pTlkTable->Fetch(0, strRes, CTlkTable::GENDER_MALE);
        if (static_cast<unsigned int>(21 - nRange) == 20 || nRange == 0) {
            s.Format(const_cast<char*>(" (%s 20 / x%d)"), strRes.text.CStr(), nMultiplier);
        } else {
            s.Format(const_cast<char*>(" (%s %d-20 / x%d)"), strRes.text.CStr(), 21 - nRange, nMultiplier);
        }
    }
    return s;
}

CExoString CSWCCreatureStats::GetAlignmentInfo()
{
    CExoString s;
    STR_RES strRes;
    int nCategory;
    if (m_alignmentGoodEvil <= 40)
        nCategory = ALIGNMENT_DARK_SIDE;
    else if (m_alignmentGoodEvil > 59)
        nCategory = ALIGNMENT_LIGHT_SIDE;
    else
        nCategory = ALIGNMENT_NEUTRAL;
    if (nCategory == ALIGNMENT_NEUTRAL)
        g_pTlkTable->Fetch(STRREF_NEUTRAL, strRes, CTlkTable::GENDER_MALE);
    else if (nCategory == ALIGNMENT_DARK_SIDE)
        g_pTlkTable->Fetch(STRREF_DARK_SIDE, strRes, CTlkTable::GENDER_MALE);
    else if (nCategory == ALIGNMENT_LIGHT_SIDE)
        g_pTlkTable->Fetch(STRREF_LIGHT_SIDE, strRes, CTlkTable::GENDER_MALE);
    s.Format(const_cast<char*>("%s (%d)"), strRes.text.CStr(), m_alignmentGoodEvil);
    return s;
}

void CSWCCreatureStats::InitializeCombatInformation()
{
    CCombatInformation* pInfo = m_pCombatInformation;
    if (!pInfo)
        return;

    pInfo->m_nNumOnHandAttacks = static_cast<unsigned char>(CalculateNumAttacks());

    signed char nMod = CalcStatModifier(m_abScores[0]);
    pInfo->m_nAttackModOnHand = nMod;
    pInfo->m_nStrDamageModOnHand = nMod;

    int nSizeCategory;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        g_pRules->m_paRaces[m_nRace].m_nAppearance,
        CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSizeCategory);
    pInfo->m_nDefaultDamageDie = 0;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        g_pRules->m_paRaces[m_nRace].m_nAppearance,
        CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSizeCategory);
    pInfo->m_nDefaultDamageDiceCount = 0;
    pInfo->m_nCritRangeOnHand = 1;
}

int CSWCCreatureStats::GetNumOnHandAttacks()
{
    if (m_pCombatInformation)
        return m_pCombatInformation->m_nNumOnHandAttacks;
    return 0;
}

int CSWCCreatureStats::GetNumOffHandAttacks()
{
    if (m_pCombatInformation && m_pCombatInformation->m_bOffHandWeaponEquipped == 1)
        return 1;
    return 0;
}

void CSWCCreatureStats::SetCombatInformation(CCombatInformation* pCombatInformation)
{
    delete m_pCombatInformation;
    if (m_bCombatInformationSet == 0)
        m_bCombatInformationSet = 1;
    m_pCombatInformation = pCombatInformation;
}

CSWCCreatureStats_ClassInfo::CSWCCreatureStats_ClassInfo()
    : m_spellLevels()
{
    m_nClass = 0;
    m_nLevel = 0;
    m_nSpellsPerDayLeft = 0;
    m_nReserved1 = 0;
    m_nReserved2 = 0xFFFF;
}

unsigned char CSWCCreatureStats_ClassInfo::GetSpellsPerDayLeft(unsigned char nSpellLevel)
{
    if (nSpellLevel != 0)
        return 0;
    return m_nSpellsPerDayLeft;
}

void CSWCCreatureStats_ClassInfo::SetSpellsPerDayLeft(unsigned char nSpellLevel, unsigned char nValue)
{
    if (nSpellLevel == 0)
        m_nSpellsPerDayLeft = nValue;
}

int CSWCCreatureStats_ClassInfo::GetKnownSpell(unsigned char nLevel, unsigned char nIndex)
{
    if (nLevel != 0)
        return -1;
    if (m_spellLevels[0].m_count <= static_cast<int>(nIndex))
        return -1;
    return m_spellLevels[0].m_pArray[nIndex];
}

int CSWCCreatureStats_ClassInfo::HasSpell(unsigned char nLevel, unsigned int nSpellId)
{
    unsigned short nCount = static_cast<unsigned short>(m_spellLevels[nLevel].m_count);
    if (nCount == 0)
        return 0;
    for (int64_t i = 0; i < nCount; i++)
        if (m_spellLevels[nLevel].m_pArray[i] == nSpellId)
            return 1;
    return 0;
}

void CSWCCreatureStats_ClassInfo::ClearKnownSpells()
{
    for (int i = m_spellLevels[0].m_count - 1; i >= 0; i--)
        m_spellLevels[0].DelIndex(i);
}


void CSWCCreatureStats_ClassInfo::RemoveKnownSpell(unsigned char nLevel, unsigned int nSpellId)
{
    if (nLevel != 0)
        return;
    CExoArrayList<unsigned int>& lvl = m_spellLevels[0];
    if (lvl.IndexOf(nSpellId) < 0)
        return;
    lvl.Remove(nSpellId);
}

void CSWCCreatureStats_ClassInfo::AddKnownSpell(unsigned char nLevel, unsigned int nSpellId)
{
    if (nLevel != 0)
        return;
    CExoArrayList<unsigned int>& lvl = m_spellLevels[0];
    if (lvl.IndexOf(nSpellId) != -1)
        return;
    lvl.Add(nSpellId);
}

CSWCCreatureStats_ClassInfo::~CSWCCreatureStats_ClassInfo()
{
    ClearKnownSpells();
}

int CSWCCreatureStats::GetCombatModeAttackModifier(CSWCCreature* pCreature)
{
    if (pCreature == 0)
        return 0;
#if KOTOR1
    if (pCreature->m_bPowerAttackMode == 1)
        return -5;
    if (pCreature->m_bImprovedPowerAttackMode == 1)
        return -10;
#else
    if (pCreature->m_bPowerAttackMode == 1)
        return -3;
    if (pCreature->m_bImprovedPowerAttackMode == 1)
        return -5;
#endif
    if (pCreature->m_bRapidShotMode == 1)
        return -2;
    return pCreature->m_bFlurryMode == 1 ? -2 : 0;
}

CExoString CSWCCreatureStats::GetGeneralAttackBonusString(CSWCCreature* pCreature, int nHand)
{
    CExoString strReturn("");
    CCombatInformation* pInfo = m_pCombatInformation;
    if (!pInfo)
        return strReturn;

    int nAbilityMod;
    unsigned int nNumAttacks;
    if (nHand != 0) {
        nNumAttacks = GetNumOffHandAttacks();
        nAbilityMod = pInfo->m_nAttackModOffHand;
    } else {
        nNumAttacks = GetNumOnHandAttacks();
        nAbilityMod = pInfo->m_nAttackModOnHand;
    }

    int nCombatModeMod = GetCombatModeAttackModifier(pCreature);

    int nFeatSum = 0;
    if (pInfo->m_attackBonusMods.m_count >= 1) {
        for (int i = 0; i < pInfo->m_attackBonusMods.m_count; i++) {
            CCombatInformationNode* pMod = pInfo->m_attackBonusMods.m_pArray[i];
            unsigned char nType = pMod->m_nType;
            if (nType > COMBAT_INFO_UNARMED || ((1 << nType) & COMBAT_INFO_LISTED) == 0)
                continue;
            if (pMod->m_nClassReq != g_pRules->m_nNumRaces)
                continue;
            if (pMod->m_nFlag != 0)
                continue;
            if (nHand == 0 ? nType == COMBAT_INFO_OFFHAND : nType == COMBAT_INFO_ONHAND)
                continue;
            if (pCreature->GetEquippedItemID(MSG_SLOT_RIGHT_WEAPON) == OBJECT_INVALID || nType != 7)
                nFeatSum += pMod->m_cValue;
        }
    }

    if (nNumAttacks != 0) {
        int nFlat = nFeatSum + nAbilityMod + nCombatModeMod;
        for (unsigned int uAttack = 0; uAttack != static_cast<unsigned int>(nNumAttacks); uAttack++) {
            if (uAttack != 0)
                strReturn = strReturn + "/";

            if (GetModifiedAttackValue(uAttack) + nFlat < 0)
                strReturn = strReturn + "-" + CExoString(-(GetModifiedAttackValue(uAttack) + nFlat));
            else if (GetModifiedAttackValue(uAttack) + nFlat != 0)
                strReturn = strReturn + "+" + CExoString(GetModifiedAttackValue(uAttack) + nFlat);
            else
                strReturn = strReturn + CExoString(GetModifiedAttackValue(uAttack) + nFlat);
        }
    }
    return strReturn;
}

int CSWCCreatureStats::GetCombatModeDamageModifier(CSWCCreature* pCreature)
{
    if (pCreature == 0)
        return 0;
#if KOTOR1
    if (pCreature->m_bPowerAttackMode == 1)
        return 5;
    return pCreature->m_bImprovedPowerAttackMode == 1 ? 10 : 0;
#else
    if (pCreature->m_bPowerAttackMode == 1)
        return 3;
    return pCreature->m_bImprovedPowerAttackMode == 1 ? 7 : 0;
#endif
}

CExoString CSWCCreatureStats::GetGeneralDamageBonusString(CSWCCreature* pCreature, int nHand)
{
    CExoString strReturn("");
    if (pCreature == 0)
        return strReturn;
    CCombatInformation* pInfo = m_pCombatInformation;
    if (pInfo == 0)
        return strReturn;

    unsigned int nNumDice = pInfo->m_nDefaultDamageDiceCount;
    unsigned int nDie = pInfo->m_nDefaultDamageDie;
    unsigned char nOverrideNumDice = pInfo->m_nOverrideNumDice;
    char cBonus;

    if (nHand == 0) {
        if (nOverrideNumDice == 0) {
            CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
            unsigned long long id = pCreature->GetEquippedItemID(MSG_SLOT_RIGHT_WEAPON);
            CSWCItem* pItem = pApp->GetItemByGameObjectID(id);
            if (pItem) {
                CSWBaseItem* pBase = g_pRules->GetBaseItem(pItem->m_baseItem);
                if (pBase) {
                    nNumDice = pBase->m_nDieToRoll;
                    nDie = pBase->m_nNumDice;
                }
            }
        } else {
            nDie = pInfo->m_nOverrideDie;
            nNumDice = nOverrideNumDice;
        }
        cBonus = pInfo->m_nStrDamageModOnHand;
    } else {
        if (nOverrideNumDice == 0) {
            CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
            unsigned long long id = pCreature->GetEquippedItemID(MSG_SLOT_LEFT_WEAPON);
            CSWCItem* pItem = pApp->GetItemByGameObjectID(id);
            if (pItem) {
                CSWBaseItem* pBase = g_pRules->GetBaseItem(pItem->m_baseItem);
                if (pBase) {
                    nNumDice = pBase->m_nDieToRoll;
                    nDie = pBase->m_nNumDice;
                }
            }
        } else {
            nDie = pInfo->m_nOverrideDie;
            nNumDice = nOverrideNumDice;
        }
        cBonus = pInfo->m_nStrDamageModOffHand;
    }

    int nModifier = GetCombatModeDamageModifier(pCreature) + cBonus;

    strReturn = CExoString(nDie) + "-" + CExoString(nDie * nNumDice);

    if (nModifier < 0)
        strReturn = strReturn + " - " + CExoString(-nModifier);
    else if (nModifier > 0)
        strReturn = strReturn + " + " + CExoString(nModifier);

    return strReturn;
}

CExoString CSWCCreatureStats::GetDamageBonusString(CSWCCreature* pCreature, int nHand, int bOffHand)
{
    unsigned int nStrRef;
    CExoString strReturn("");
    STR_RES strTemp;

    CCombatInformation* pInfo = m_pCombatInformation;
    if (pInfo != 0) {
        CCombatInformationNode* pMod = pInfo->m_damageBonusMods.m_pArray[nHand];
        if (pMod->m_nType < 8 && ((1 << pMod->m_nType) & COMBAT_INFO_LISTED) != 0) {
            if (bOffHand != 0) {
                if (pMod->m_nType == COMBAT_INFO_ONHAND)
                    return CExoString("");
            } else if (pMod->m_nType == COMBAT_INFO_OFFHAND) {
                return CExoString("");
            }

            unsigned long long nItemID = pCreature->GetEquippedItemID(MSG_SLOT_RIGHT_WEAPON);
            if (nItemID != OBJECT_INVALID && pMod->m_nType == COMBAT_INFO_UNARMED)
                return CExoString("");

            int iVar7 = pMod->m_cValue + GetCombatModeDamageModifier(pCreature);
            unsigned char nDamageSubType = pMod->m_nModifierType;

            if (iVar7 < 6) {
                if (iVar7 >= 0)
                    strReturn = strReturn + "    +" + CExoString(iVar7);
                else
                    strReturn = CExoString("    -") + CExoString(-iVar7);
            } else {
                g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(iVar7, CExoString("GameString"), reinterpret_cast<int*>(&nStrRef));
                g_pTlkTable->Fetch(nStrRef, strTemp, CTlkTable::GENDER_MALE);
                strReturn = strReturn + "    +" + strTemp.text;
            }

            switch (nDamageSubType) {  // every subtype fetches strref 0
            case 0:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 1:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 2:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 3:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 4:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 5:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 6:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 7:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 8:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 9:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 10:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 11:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 12:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 13:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            case 14:
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                break;
            default:
                strTemp.text = "";
            }

            strReturn = strReturn + " " + strTemp.text;

            int bAppendedExtra = 0;
            if (pMod->m_nFlag != 0) {
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                strReturn = strReturn + " " + strTemp.text + " ";

                if (pMod->m_nFlag == 3)
                    g_pTlkTable->Fetch(STRREF_DARK_SIDE, strTemp, CTlkTable::GENDER_MALE);
                else if (pMod->m_nFlag == 2)
                    g_pTlkTable->Fetch(STRREF_LIGHT_SIDE, strTemp, CTlkTable::GENDER_MALE);
                else if (pMod->m_nFlag == 1)
                    g_pTlkTable->Fetch(STRREF_NEUTRAL, strTemp, CTlkTable::GENDER_MALE);

                strReturn = strReturn + strTemp.text;
                bAppendedExtra = 1;
            }

            if (pMod->m_nClassReq != g_pRules->m_nNumRaces) {
                if (bAppendedExtra) {
                    strReturn = strReturn + " ";
                } else {
                    g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                    strReturn = strReturn + " " + strTemp.text + " ";
                }
                g_pTlkTable->Fetch(g_pRules->m_paRaces[pMod->m_nClassReq].m_nNamePluralStrRef, strTemp, CTlkTable::GENDER_MALE);
                strReturn = strReturn + strTemp.text;
            }
        }
    }
    return strReturn;
}

CExoString CSWCCreatureStats::GetAttackBonusString(CSWCCreature* pCreature, int nHand, int bOffHand)
{
    STR_RES strTemp;
    CExoString strReturn("");

    CCombatInformation* pInfo = m_pCombatInformation;
    if (pInfo != 0) {
        CCombatInformationNode* pMod = pInfo->m_attackBonusMods.m_pArray[nHand];
        unsigned char nType = pMod->m_nType;
        if (nType < 8 && ((1 << nType) & COMBAT_INFO_LISTED) != 0 &&
            (pMod->m_nClassReq != g_pRules->m_nNumRaces || pMod->m_nFlag != 0)) {

            if (bOffHand != 0) {
                if (nType == COMBAT_INFO_ONHAND)
                    return CExoString("");
            } else if (nType == COMBAT_INFO_OFFHAND) {
                return CExoString("");
            }

            unsigned long long nItemID = pCreature->GetEquippedItemID(MSG_SLOT_RIGHT_WEAPON);
            if (nItemID != OBJECT_INVALID && nType == COMBAT_INFO_UNARMED)
                return CExoString("");

            int nAbilityMod;
            int nNumAttacks;
            if (bOffHand != 0) {
                nNumAttacks = GetNumOffHandAttacks();
                nAbilityMod = m_pCombatInformation->m_nAttackModOffHand;
            } else {
                nNumAttacks = GetNumOnHandAttacks();
                nAbilityMod = m_pCombatInformation->m_nAttackModOnHand;
            }

            int nCombatModeMod = GetCombatModeAttackModifier(pCreature);

            if (nNumAttacks > 0) {
                int nFlat = nAbilityMod + pMod->m_cValue + nCombatModeMod;
                strReturn = "    ";
                for (unsigned int uAttack = 0; uAttack != static_cast<unsigned int>(nNumAttacks); uAttack++) {
                    if (uAttack != 0)
                        strReturn = strReturn + "/";

                    if (GetModifiedAttackValue(uAttack) + nFlat < 0)
                        strReturn = strReturn + "-" + CExoString(-(GetModifiedAttackValue(uAttack) + nFlat));
                    else
                        strReturn = strReturn + "+" + CExoString(GetModifiedAttackValue(uAttack) + nFlat);
                }
            }

            bool bAppendedExtra = false;
            if (pMod->m_nFlag != 0) {
                g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                strReturn = strReturn + " " + strTemp.text + " ";

                if (pMod->m_nFlag == 1)
                    g_pTlkTable->Fetch(STRREF_NEUTRAL, strTemp, CTlkTable::GENDER_MALE);
                else if (pMod->m_nFlag == 2)
                    g_pTlkTable->Fetch(STRREF_LIGHT_SIDE, strTemp, CTlkTable::GENDER_MALE);
                else if (pMod->m_nFlag == 3)
                    g_pTlkTable->Fetch(STRREF_DARK_SIDE, strTemp, CTlkTable::GENDER_MALE);

                strReturn = strReturn + strTemp.text;
                bAppendedExtra = true;
            }

            if (pMod->m_nClassReq != g_pRules->m_nNumRaces) {
                if (bAppendedExtra) {
                    strReturn = strReturn + " ";
                } else {
                    g_pTlkTable->Fetch(0, strTemp, CTlkTable::GENDER_MALE);
                    strReturn = strReturn + " " + strTemp.text + " ";
                }
                g_pTlkTable->Fetch(g_pRules->m_paRaces[pMod->m_nClassReq].m_nNamePluralStrRef, strTemp, CTlkTable::GENDER_MALE);
                strReturn = strReturn + strTemp.text;
            }
        }
    }
    return strReturn;
}

void CSWCCreatureStats::UpdateAbilityModifiedSkillRanks()
{
    for (unsigned short i = 0; i < g_pRules->m_nNumSkills; i++) {
        unsigned char nRank = (g_pRules->m_paSkills[i].m_flags & 2) | m_pSkillRanksBase[i];
        if (nRank != 0)
            nRank = static_cast<unsigned char>(GetSkillModifier(i) + m_pSkillRanksBase[i]);
        m_pSkillRanks[i] = nRank;
    }
}

void CSWCCreatureStats::ClearKnownSpells()
{
    for (unsigned char c = 0; c < m_nNumClasses; c++)
        m_classes[c].ClearKnownSpells();
}

void CSWCCreatureStats::ClearKnownSpells(unsigned char nClassPos)
{
    if (nClassPos < 2)
        m_classes[nClassPos].ClearKnownSpells();
}

#if KOTOR1  // K1 keeps this out of line
__attribute__((noinline))
#endif
signed char CSWCCreatureStats::GetSkillModifier(unsigned short nSkill)
{
    int nAbility = g_pRules->m_paSkills[nSkill].m_nKeyAbility;
    switch (nAbility) {
    case ABILITY_STRENGTH: return GetSTRMod();
    case ABILITY_DEXTERITY: return GetDEXMod();
    case ABILITY_CONSTITUTION: return GetCONMod();
    case ABILITY_INTELLIGENCE: return GetINTMod();
    case ABILITY_WISDOM: return GetWISMod();
    case ABILITY_CHARISMA: return GetCHAMod();
    default: return 0;
    }
}

int CSWCCreatureStats::HasSpell(unsigned char nSpellLevel, unsigned int nSpellId)
{
    for (int i = 0; i < m_nNumClasses; i++) {
        // The known-spell count is effectively 16-bit, so the counter is compared zero-extended.
        int64_t nNum = static_cast<unsigned short>(m_classes[i].m_spellLevels[nSpellLevel].m_count);
        if (nNum != 0) {
            const unsigned int* pSpells = m_classes[i].m_spellLevels[nSpellLevel].m_pArray;
            do {
                if (static_cast<unsigned int>(*pSpells) == nSpellId)
                    return 1;
                pSpells++;
            } while (--nNum != 0);
        }
    }
    return 0;
}

int CSWCCreatureStats::GetSkillRank(unsigned short nSkill)
{
    if (g_pRules->m_nNumSkills <= nSkill)
        return 0;
    return m_pSkillRanks[nSkill];
}

void CSWCCreatureStats::ClearFeats()
{
    if (m_lstFeats.m_count != 0)
        m_lstFeats.m_count = 0;
    while (m_lstBonusFeats.m_count != 0)
        m_lstBonusFeats.DelIndex(0);
    while (m_lstFeatUses.m_count != 0) {
        delete m_lstFeatUses[0];
        m_lstFeatUses.DelIndex(0);
    }
}

void CSWCCreatureStats::ClearSkills()
{
    memset(m_pSkillRanks, 0, g_pRules->m_nNumSkills);  // idiom-exempt: byte array, size is a runtime element count (element==1 byte)
    memset(m_pSkillRanksBase, 0, g_pRules->m_nNumSkills);  // idiom-exempt: byte array, size is a runtime element count (element==1 byte)
}

unsigned char CSWCCreatureStats::GetSpellsPerDayLeft(unsigned char nClassPos, unsigned char nSpellLevel)
{
    if (nClassPos <= 1 && nSpellLevel == 0)
        return m_classes[nClassPos].m_nSpellsPerDayLeft;
    return 0;
}

void CSWCCreatureStats::SetSpellsPerDayLeft(unsigned char nClassPos, unsigned char nSpellLevel, unsigned char nValue)
{
    if (nClassPos <= 1 && nSpellLevel == 0)
        m_classes[nClassPos].m_nSpellsPerDayLeft = nValue;
}

unsigned short CSWCCreatureStats::GetNumberKnownSpells(unsigned char nClassPos, unsigned char nSpellLevel)
{
    if (nClassPos > 1)
        return 0;
    return static_cast<unsigned short>(m_classes[nClassPos].m_spellLevels[nSpellLevel].m_count);
}

int CSWCCreatureStats::GetKnownSpell(unsigned char nClassPos, unsigned char nSpellLevel, unsigned char nIndex)
{
    if (nClassPos > 1)
        return -1;
    return m_classes[nClassPos].GetKnownSpell(nSpellLevel, nIndex);
}

void CSWCCreatureStats::RemoveKnownSpell(unsigned char nClassPos, unsigned int nSpellId)
{
    if (nClassPos > 1)
        return;
    if (g_pRules->GetSpell(nSpellId) == 0)
        return;
    m_classes[nClassPos].RemoveKnownSpell(0, nSpellId);
}

// inlined CExoArrayList<unsigned int>::Add(nValue)
static inline void AppendKnownSpell(CExoArrayList<unsigned int>& lvl, unsigned int nValue)
{
    int idx = lvl.m_count;
    if (lvl.m_count == lvl.m_capacity) {
        int newcap = lvl.m_count != 0 ? lvl.m_count * 2 : 16;
        unsigned int* pOld = lvl.m_pArray;
        lvl.m_capacity = newcap;
        lvl.m_pArray = new unsigned int[newcap];
        for (int64_t i = 0; i < lvl.m_count; i++)
            lvl.m_pArray[i] = pOld[i];
        delete[] pOld;
        idx = lvl.m_count;
    }
    unsigned int* pSlot = &lvl.m_pArray[idx];
    lvl.m_count = idx + 1;
    *pSlot = nValue;
}

void CSWCCreatureStats::AddKnownSpell(unsigned char nClassPos, unsigned int nSpellId)
{
    if (nClassPos > 1)
        return;
    if (g_pRules->GetSpell(nSpellId) == 0)
        return;
    // inlined CSWCCreatureStats_ClassInfo::AddKnownSpell(0, nSpellId)
    CExoArrayList<unsigned int>& lvl = m_classes[nClassPos].m_spellLevels[0];
    if (lvl.IndexOf(nSpellId) != -1)
        return;
    AppendKnownSpell(lvl, nSpellId);
}

CSWCStats_SpellLikeAbility::CSWCStats_SpellLikeAbility()
{
    m_nSpellId = 0;
    m_nReady = 1;
    m_nCasterLevel = 0;
}

template void CExoArrayList<CSWGuiInterfaceAction>::Allocate(int);

void CSWCLevelUpStats::SetINTBase(unsigned char nScore)
{
    m_abScoresBase[3] = nScore;
    m_abScores[3] = static_cast<unsigned char>(g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[2] + nScore);
}

void CSWCLevelUpStats::SetCHABase(unsigned char nScore)
{
    m_abScoresBase[5] = nScore;
    m_abScores[5] = static_cast<unsigned char>(g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[3] + nScore);
}

void CSWCLevelUpStats::SetSTRBase(unsigned char nScore)
{
    m_abScoresBase[0] = nScore;
    m_abScores[0] = static_cast<unsigned char>(g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[0] + nScore);
    InitializeCombatInformation();
}

void CSWCLevelUpStats::SetWISBase(unsigned char nScore)
{
    m_abScoresBase[4] = nScore;
    m_abScores[4] = static_cast<unsigned char>(g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[4] + nScore);
    SetWillSavingThrow();
}

void CSWCLevelUpStats::SetCONBase(unsigned char nScore)
{
    m_abScoresBase[2] = nScore;
    m_abScores[2] = static_cast<unsigned char>(g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[5] + nScore);

    if (GetLevel() == 1 && m_nExperience == 0) {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClassLeveledUpIn()];
        if (pClass) {
            short nHP = m_nHitPointsBase + GetCONMod();
            if (nHP < 2)
                nHP = 1;
            m_nMaxHitPoints = nHP;
            m_nCurrentHitPoints = nHP;
        }
    }

    SetFortSavingThrow();
}

void CSWCLevelUpStats::SetDEXBase(unsigned char nScore, int bAdjustAC)
{
    unsigned short nOldAC = m_nArmorClass;
    char nOldDexMod = GetDEXMod();
    m_abScoresBase[1] = nScore;
    m_abScores[1] = static_cast<unsigned char>(g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[1] + nScore);

    SetReflexSavingThrow();

    if (bAdjustAC == 0)
        m_nArmorClass = GetDEXMod() + 10;
    else
        m_nArmorClass = nOldAC - nOldDexMod + GetDEXMod();

    InitializeCombatInformation();
}

void CSWCLevelUpStats::SetReflexSavingThrow()
{
    char nBonus = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++) {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClass(i)];
        if (pClass)
            nBonus += pClass->GetRefSaveBonus(GetClassLevel(i));
    }
    m_nReflexSavingThrow = nBonus + GetDEXMod();
}

void CSWCLevelUpStats::SetFortSavingThrow()
{
    char nBonus = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++) {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClass(i)];
        if (pClass)
            nBonus += pClass->GetFortSaveBonus(GetClassLevel(i));
    }
    m_nFortSavingThrow = nBonus + GetCONMod();
}

void CSWCLevelUpStats::SetWillSavingThrow()
{
    char nBonus = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++) {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClass(i)];
        if (pClass)
            nBonus += pClass->GetWillSaveBonus(GetClassLevel(i));
    }
    m_nWillSavingThrow = nBonus + GetWISMod();
}

int CSWCLevelUpStats::FeatRequirementsMet(unsigned short nFeat, CExoArrayList<unsigned short>* pFeats)
{
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if (!pFeat)
        return 0;

    CSWClass* pHighestClass = &g_pRules->m_paClasses[GetClassLeveledUpIn()];
    unsigned char nGrantLevel = pHighestClass->GetLevelFeatGranted(nFeat);
    if (nGrantLevel != CSWClass::LEVEL_NONE && nGrantLevel > GetLevel())
        return 0;

    if (pFeat->m_nMinSpellLvl != 0) {
        for (unsigned char i = 0;; i++) {
            if (i >= m_nNumClasses)
                return 0;
            unsigned char nClassId = GetClass(i);
            CSWClass* pClass = &g_pRules->m_paClasses[nClassId];
            if (pClass->m_bSpellCaster == 0)
                continue;
            if (pClass->GetSpellsKnownPerLevel(GetClassLevel(i), pFeat->m_nMinSpellLvl, nClassId,
                                                m_nRace, m_abScoresBase[5]) != 0)
                break;
        }
    }

    if (pFeat->m_nMinCharLevel > GetLevel())
        return 0;

    unsigned char nMinAB = pFeat->m_nMinAttackBonus;
    unsigned char nAttackBonus = GetBaseAttackBonus();
    if (nAttackBonus < nMinAB)
        return 0;

    if (pFeat->m_nMinSTR > m_abScoresBase[0])
        return 0;
    if (pFeat->m_nMinDEX > m_abScoresBase[1])
        return 0;
    if (pFeat->m_nMinINT > m_abScoresBase[3])
        return 0;
    if (pFeat->m_nMinWIS > m_abScoresBase[4])
        return 0;

    if (!HasFeatPrereq(pFeat, pFeats))
        return 0;

    if (pFeat->m_nReqSkill != -1 && !GetCanUseSkill(pFeat->m_nReqSkill))
        return 0;

    return 1;
}

char CSWCLevelUpStats::GetBaseAttackBonus()
{
    unsigned char nBonus = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++)
        nBonus += g_pRules->m_paClasses[GetClass(i)].GetAttackBonus(GetClassLevel(i));
    return nBonus;
}

__attribute__((noinline))
int CSWCLevelUpStats::HasFeatPrereq(CSWFeat* pFeat, CExoArrayList<unsigned short>* pFeats)
{
    unsigned short* p;
    int n;
    int nCount;
    int nBonusCount;

    unsigned short nPrereq1 = pFeat->m_nPrereqFeat1;
    if (nPrereq1 != 0xffff) {
        nCount = 0;
        p = m_lstFeats.m_pArray;
        n = m_lstFeats.m_count;
        for (; n > 0; n--, p++)
            if (*p == nPrereq1)
                nCount++;
        if (nCount == 0) {
            nBonusCount = 0;
            p = m_lstBonusFeats.m_pArray;
            n = m_lstBonusFeats.m_count;
            for (; n > 0; n--, p++)
                if (*p == nPrereq1)
                    nBonusCount++;
            if (nBonusCount == 0) {
                if (!pFeats)
                    return 0;
                if (pFeats->m_count < 1)
                    return 0;
                unsigned short i = 0;
                while (pFeats->m_pArray[i] != nPrereq1) {
                    i++;
                    if (static_cast<int>(i) >= pFeats->m_count)
                        return 0;
                }
            }
        }
    }

    unsigned short nPrereq2 = pFeat->m_nPrereqFeat2;
    if (nPrereq2 != 0xffff) {
        nCount = 0;
        p = m_lstFeats.m_pArray;
        n = m_lstFeats.m_count;
        for (; n > 0; n--, p++)
            if (*p == nPrereq2)
                nCount++;
        if (nCount == 0) {
            nBonusCount = 0;
            p = m_lstBonusFeats.m_pArray;
            n = m_lstBonusFeats.m_count;
            for (; n > 0; n--, p++)
                if (*p == nPrereq2)
                    nBonusCount++;
            if (nBonusCount == 0) {
                if (!pFeats)
                    return 0;
                if (pFeats->m_count < 1)
                    return 0;
                unsigned short i = 0;
                while (pFeats->m_pArray[i] != nPrereq2) {
                    i++;
                    if (static_cast<int>(i) >= pFeats->m_count)
                        return 0;
                }
            }
        }
    }

    bool bAnyOrReq = false;
    bool bMatched = false;
    for (int slot = 0; slot < 5; slot++) {
        unsigned short nOrReq = pFeat->m_anOrReqFeat[slot];
        if (nOrReq == 0xffff)
            continue;
        bAnyOrReq = true;

        nCount = 0;
        p = m_lstFeats.m_pArray;
        n = m_lstFeats.m_count;
        for (; n > 0; n--, p++)
            if (*p == nOrReq)
                nCount++;
        if (nCount != 0) {
            bMatched = true;
            break;
        }

        nBonusCount = 0;
        p = m_lstBonusFeats.m_pArray;
        n = m_lstBonusFeats.m_count;
        for (; n > 0; n--, p++)
            if (*p == nOrReq)
                nBonusCount++;
        if (nBonusCount != 0) {
            bMatched = true;
            break;
        }

        if (pFeats) {
            bool bFoundInList = false;
            for (int i = 0; i < pFeats->m_count; i++)
                if (pFeats->m_pArray[i] == nOrReq) {
                    bFoundInList = true;
                    break;
                }
            if (bFoundInList) {
                bMatched = true;
                break;
            }
        }
    }
    return !bAnyOrReq || bMatched;
}

void CSWCLevelUpStats::SetSkillRankBase(unsigned short nSkill, char cRank)
{
    if (nSkill >= g_pRules->m_nNumSkills)
        return;
    m_pSkillRanksBase[nSkill] = cRank;
    if ((g_pRules->m_paSkills[nSkill].m_flags & 2) == 0) {
        if (nSkill >= g_pRules->m_nNumSkills)
            return;
        if (m_pSkillRanksBase[nSkill] == 0) {
            m_pSkillRanks[nSkill] = 0;
            return;
        }
    }
    // Out-of-range ability ids fall through the switch, leaving the raw id as the modifier.
    int nKeyAbility = g_pRules->m_paSkills[nSkill].m_nKeyAbility;
    char nModifier = static_cast<char>(nKeyAbility);
    switch (nKeyAbility) {
    case ABILITY_STRENGTH: nModifier = GetSTRMod(); break;
    case ABILITY_DEXTERITY: nModifier = GetDEXMod(); break;
    case ABILITY_CONSTITUTION: nModifier = GetCONMod(); break;
    case ABILITY_INTELLIGENCE: nModifier = GetINTMod(); break;
    case ABILITY_WISDOM: nModifier = GetWISMod(); break;
    case ABILITY_CHARISMA: nModifier = GetCHAMod(); break;
    }
    if (nSkill < g_pRules->m_nNumSkills)
        m_pSkillRanks[nSkill] = static_cast<unsigned char>(m_pSkillRanksBase[nSkill] + nModifier);
}

#define NWCCREATURESTATS_MAX(a, b) ((a) > (b) ? (a) : (b))

void CSWCLevelUpStats::AddSkillPointsPerLevel()
{
    CSWClass* pClass = &g_pRules->m_paClasses[GetClassLeveledUpIn()];
#if KOTOR1
    // K1: level 1 pays (SkillPointBase/2 + INT mod) x4, later levels (SkillPointBase + INT mod)/2,
    // each at least 1.
    if (pClass) {
        unsigned char nLevel = GetLevel();
        if (nLevel == 1) {
            int nPoints = (pClass->m_nSkillPointsBase >> 1) + GetBaseINTMod();
            if (nPoints < 2)
                nPoints = 1;
            m_nSkillPointsRemaining = static_cast<unsigned short>(nPoints * 4);
        } else {
            int nPoints = (pClass->m_nSkillPointsBase + GetBaseINTMod()) / 2;
            if (nPoints < 2)
                nPoints = 1;
            m_nSkillPointsRemaining = static_cast<unsigned short>(m_nSkillPointsRemaining + nPoints);
        }
    }
#else
    if (pClass) {
        if (GetLevel() == 1)
            m_nSkillPointsRemaining = static_cast<unsigned short>(
                NWCCREATURESTATS_MAX(pClass->m_nSkillPointsBase + GetBaseINTMod(), 1) * 4);
        else
            m_nSkillPointsRemaining += NWCCREATURESTATS_MAX(pClass->m_nSkillPointsBase + GetBaseINTMod(), 1);
    }
#endif
}

int CSWCLevelUpStats::CanChooseFeat(unsigned short nFeat, unsigned char nNormalLeft,
                                      unsigned char nBonusLeft, CExoArrayList<unsigned short>* pChosen)
{
#if !KOTOR1
    if (nFeat == CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS ||
        nFeat == CSWSCreatureStats::FEAT_MINE_IMMUNITY)
        return false;
#endif
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if ((nBonusLeft | nNormalLeft) == 0)
        return false;
    if (!pFeat)
        return false;

#if !KOTOR1
    if (CExoString(m_sTag).CompareNoCase("baodur") && pFeat->m_nBaodurGrantLevel == GRANT_LEVEL_NEVER)
        return false;
    if (CExoString(m_sTag).CompareNoCase("kreia") && pFeat->m_nKreiaGrantLevel == GRANT_LEVEL_NEVER)
        return false;
#endif

    for (int i = 0; i < static_cast<unsigned short>(m_lstFeats.m_count); i++)
        if (GetFeat(static_cast<unsigned short>(i)) == nFeat)
            return false;

    for (unsigned short i = 0; i < pChosen->m_count; i++)
        if (pChosen->m_pArray[i] == nFeat)
            return false;

    if (!FeatRequirementsMet(nFeat, pChosen))
        return false;

    int nNormal, nBonus;
    SetNormalBonusFlags(nFeat, nNormal, nBonus);
    if (!nNormal && !nBonus)
        return false;

    CExoArrayList<int> lstConsumed;
    lstConsumed.SetSize(pChosen->m_count);
    int* paConsumed = lstConsumed.m_pArray;
    int nChosenCount = pChosen->m_count;
    if (nChosenCount != 0) {
        for (int i = 0; i < nChosenCount; i++)
            paConsumed[i] = 0;

        for (unsigned short i = 0; i < pChosen->m_count; i++) {
            int bOtherNormal, bOtherBonus;
            SetNormalBonusFlags(pChosen->m_pArray[i], bOtherNormal, bOtherBonus);
            if (bOtherBonus) {
                if (nBonusLeft == 0 || bOtherNormal)
                    continue;
                paConsumed[i] = 1;
                nBonusLeft--;
            } else {
                if (!bOtherNormal)
                    continue;
                if (nNormalLeft == 0)
                    return false;
                paConsumed[i] = 1;
                nNormalLeft--;
            }
        }
    }

    bool bSelfConsumed = false;
    SetNormalBonusFlags(nFeat, nNormal, nBonus);
    if (nBonus) {
        if (!nNormal) {
            if (nBonusLeft == 0)
                return false;
            nBonusLeft--;
            bSelfConsumed = true;
        }
    } else if (nNormal) {
        if (nNormalLeft == 0)
            return false;
        nNormalLeft--;
        bSelfConsumed = true;
    }

    for (unsigned short i = 0; i < pChosen->m_count; i++) {
        if (paConsumed[i])
            continue;
        if (nNormalLeft != 0)
            nNormalLeft--;
        else if (nBonusLeft != 0)
            nBonusLeft--;
        else
            return false;
    }

    return bSelfConsumed | ((nNormalLeft | nBonusLeft) != 0);
}

void CSWCLevelUpStats::SetNormalBonusFlags(unsigned short nFeat, int& nNormal, int& nBonus)
{
    nNormal = 0;
    nBonus = 0;
    if (g_pRules->GetFeat(nFeat) == 0)
        return;
    CSWRules* pRules = g_pRules;
    unsigned char nClassId = GetClass(static_cast<unsigned char>(m_nNumClasses - 1));
    CSWClass* pClass = &pRules->m_paClasses[nClassId];
    if (pClass == 0)
        return;
    if (pClass->IsNormalFeat(nFeat) != 0)
        nNormal = 1;
    if (pClass->IsBonusFeat(nFeat) != 0)
        nBonus = 1;
}

#if KOTOR1
CSWCLevelUpStats::CSWCLevelUpStats()
#else
CSWCLevelUpStats::CSWCLevelUpStats(CSWCCreature* pCreature)
    : CSWCCreatureStats(pCreature)
#endif
{
    m_nAbilityPointsRemaining = 30;
    m_nSkillPointsRemaining = 0;

    SetSTRBase(8);
    SetDEXBase(8, 0);
    SetINTBase(8);
    SetWISBase(8);
    SetCHABase(8);
    SetCONBase(8);
}

CSWCLevelUpStats::~CSWCLevelUpStats()
{
}

unsigned int CSWCLevelUpStats::ComputeNumberKnownSpellsLeft()
{
    unsigned char nClassPos = static_cast<unsigned char>(m_nNumClasses - 1);
    unsigned char nClassId = GetClass(nClassPos);
    CSWClass* pClass = &g_pRules->m_paClasses[nClassId];
    if (pClass &&
        (nClassId == CSWClass::CCLASS_JEDICONSULAR ||
         nClassId == CSWClass::CCLASS_JEDIGUARDIAN ||
#if KOTOR1  // K1 has only the three base Jedi classes
         nClassId == CSWClass::CCLASS_JEDISENTINEL)) {
#else
         nClassId == CSWClass::CCLASS_JEDISENTINEL ||
         nClassId == CSWClass::CCLASS_JEDIWATCHMAN ||
         nClassId == CSWClass::CCLASS_JEDIWEAPONMASTER ||
         nClassId == CSWClass::CCLASS_JEDIMASTER ||
         nClassId == CSWClass::CCLASS_SITHASSASSIN ||
         nClassId == CSWClass::CCLASS_SITHMARAUDER ||
         nClassId == CSWClass::CCLASS_SITHLORD)) {
#endif
        unsigned int nGain = pClass->GetSpellGain(GetClassLevel(nClassPos), 0);
        return (static_cast<unsigned char>(nGain) == CSWClass::LEVEL_NONE) ? 0 : nGain;
    }
    return 0;
}

int CSWCLevelUpStats::CanChooseSpell(unsigned int nSpell, unsigned char nLeft, CExoArrayList<unsigned int>* pChosen)
{
#if KOTOR1
    // K1: no per-class spell-level gate, and only the last class slot's known list is scanned;
    // a read past the count returns -1.
    unsigned char nLastPos = static_cast<unsigned char>(m_nNumClasses - 1);
    CSWSpell* pSpell = g_pRules->GetSpell(nSpell);
    if (pSpell == 0)
        return false;
    if (pSpell->m_nUserType != 1 && pSpell->m_nUserType != 5)
        return false;
    if (nLeft == 0)
        return false;
    if (nLastPos < 2) {
        CExoArrayList<unsigned int>& known = m_classes[nLastPos].m_spellLevels[0];
        for (unsigned short i = 0; i < static_cast<unsigned short>(known.m_count); i++) {
            unsigned char j = static_cast<unsigned char>(i);
            int nKnown = j < known.m_count ? known.m_pArray[j] : -1;
            if (static_cast<unsigned int>(nKnown) == nSpell)
                return false;
        }
    }
    for (unsigned short i = 0; i < pChosen->m_count; i++)
        if (static_cast<unsigned int>(pChosen->m_pArray[i]) == nSpell)
            return false;
    if (!HasSpellPrereq(pSpell, pChosen))
        return false;
    return pChosen->m_count < nLeft;
#else
    int nClassPos = m_nNumClasses - 1;
    CSWSpell* pSpell = g_pRules->GetSpell(nSpell);
    if (pSpell == 0)
        return false;
    if ((pSpell->m_nUserType | 4) != 5)
        return false;

    unsigned char nSpellLevel = pSpell->GetSpellLevel(GetClass(static_cast<unsigned char>(nClassPos)));
    if (nLeft == 0 || nSpellLevel == CSWSpell::SPELL_LEVEL_NONE)
        return false;

    for (int i = 0; i != m_nNumClasses; i++) {
        unsigned short nCount = static_cast<unsigned short>(m_classes[i].m_spellLevels[0].m_count);
        for (int j = 0; j < nCount; j++)
            if (m_classes[i].m_spellLevels[0].m_pArray[j] == nSpell)
                return false;
    }
    for (unsigned short i = 0; i < pChosen->m_count; i++)
        if (static_cast<unsigned int>(pChosen->m_pArray[i]) == nSpell)
            return false;

    if (!HasSpellPrereq(pSpell, pChosen))
        return false;
    return pChosen->m_count < nLeft;
#endif
}

int CSWCLevelUpStats::HasSpellPrereq(CSWSpell* pSpell, CExoArrayList<unsigned int>* pChosen)
{
    for (int k = 0; pSpell->m_anPrereq2[k] != -1; k++) {
        int nId = pSpell->m_anPrereq2[k];
        for (int i = 0; i != m_nNumClasses; i++) {
            unsigned short nCount = static_cast<unsigned short>(m_classes[i].m_spellLevels[0].m_count);
            for (int j = 0; j < nCount; j++)
                if (m_classes[i].m_spellLevels[0].m_pArray[j] == nId)
                    goto next2;
        }
        if (pChosen->IndexOf(static_cast<unsigned int>(nId)) == -1)
            return false;
    next2: ;
    }
    for (int k = 0; pSpell->m_anPrereq1[k] != -1; k++) {
        int nId = pSpell->m_anPrereq1[k];
        for (int i = 0; i != m_nNumClasses; i++) {
            unsigned short nCount = static_cast<unsigned short>(m_classes[i].m_spellLevels[0].m_count);
            for (int j = 0; j < nCount; j++)
                if (m_classes[i].m_spellLevels[0].m_pArray[j] == nId)
                    goto next1;
        }
        if (pChosen->IndexOf(static_cast<unsigned int>(nId)) == -1)
            return false;
    next1: ;
    }
    unsigned char nLevel = GetLevel();
    unsigned char nSpellLevel = pSpell->GetSpellLevel(GetClassLeveledUpIn());
    return nLevel >= nSpellLevel;
}

int CSWCLevelUpStats::FeatAcquired(unsigned short nFeat, CExoArrayList<unsigned short>* pChosen)
{
    if (nFeat == 0xffff)
        return 0;
    if (CSWCCreatureStats::FeatAcquired(nFeat))
        return 1;
    if (pChosen != 0) {
        for (unsigned short i = 0; i < pChosen->m_count; i++)
            if (pChosen->m_pArray[i] == nFeat)
                return 1;
    }
    return 0;
}

void CSWCLevelUpStats::SetClass(unsigned char nClassPos, unsigned char nClass)
{
    if (nClassPos > 1)
        return;
    m_classes[nClassPos].m_nClass = nClass;

    SetReflexSavingThrow();
    SetWillSavingThrow();
    SetFortSavingThrow();

#if KOTOR1
    // K1: a creature with no base hit points yet takes the class's hit die
    if (m_nHitPointsBase == 0) {
        unsigned char nHitDie = g_pRules->m_paClasses[nClass].m_nHitDie;
        short nHP = nHitDie + GetCONMod();
        if (nHP < 2)
            nHP = 1;
        m_nHitPointsBase = nHitDie;
        m_nMaxHitPoints = nHP;
        m_nCurrentHitPoints = nHP;
    }
#endif

    InitializeCombatInformation();
}

void CSWCLevelUpStats::SetClassLevel(unsigned char nClassPos, unsigned char nLevel)
{
    if (nClassPos > 1)
        return;
    m_classes[nClassPos].m_nLevel = nLevel;

    SetReflexSavingThrow();
    SetWillSavingThrow();
    SetFortSavingThrow();

    InitializeCombatInformation();
}

int CSWCLevelUpStats::GetSkillPointCostForSkillRank(unsigned short nSkill, int bAsPositive)
{
    CSWSkill* pSkill = &g_pRules->m_paSkills[nSkill];
    if (!pSkill)
        return 0;
    bool bIsClassSkill = false;
    int bCanBuy = (pSkill->m_flags >> 3) & 1;
    for (unsigned char i = 0; !bIsClassSkill && i < m_nNumClasses; i++) {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClass(i)];
        if (pClass && pClass->IsSkillUseable(nSkill)) {
            bCanBuy = 1;
            if (pClass->IsSkillClassSkill(nSkill)) {
                bIsClassSkill = true;
                continue;
            }
#if !KOTOR1  // K1 has no class-skill-granting feats
            if (HasClassSkillFeat(nSkill))
                bIsClassSkill = true;
#endif
        }
    }
    int nCost;
    if (bIsClassSkill) {
        nCost = 1;
    } else if (bCanBuy) {
        nCost = 2;
    } else {
        return 0;
    }
    return bAsPositive ? nCost : -nCost;
}

int CSWCLevelUpStats::IncreaseSkillRank(unsigned short nSkill)
{
    CSWSkill* pSkill = &g_pRules->m_paSkills[nSkill];
    if (!pSkill)
        return 0;
    bool bIsClassSkill = false;
    int bCanBuy = (pSkill->m_flags >> 3) & 1;
    for (unsigned char i = 0; !bIsClassSkill && i < m_nNumClasses; i++) {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClass(i)];
        if (pClass && pClass->IsSkillUseable(nSkill)) {
            bCanBuy = 1;
            if (pClass->IsSkillClassSkill(nSkill)) {
                bIsClassSkill = true;
                continue;
            }
#if !KOTOR1
            if (HasClassSkillFeat(nSkill))
                bIsClassSkill = true;
#endif
        }
    }

#if KOTOR1
    if (bIsClassSkill) {
        if (m_nSkillPointsRemaining == 0)
            return 0;
    } else {
        if (!bCanBuy || m_nSkillPointsRemaining <= 1)
            return 0;
    }
    unsigned char nMaxRank = static_cast<unsigned char>(GetLevel() + 3);
    if (!bIsClassSkill)
        nMaxRank = nMaxRank / 2;
    short nCost = bIsClassSkill ? -1 : -2;
#else
    unsigned char nMaxRank;
    short nCost;
    if (bIsClassSkill) {
        if (m_nSkillPointsRemaining == 0)
            return 0;
        nMaxRank = static_cast<unsigned char>(GetLevel() + 3);
        nCost = -1;
    } else {
        if (!bCanBuy || m_nSkillPointsRemaining <= 1)
            return 0;
        nMaxRank = static_cast<unsigned char>(((GetLevel() + 3) >> 1) & 0x7f);
        nCost = -2;
    }
#endif

    if (nSkill < g_pRules->m_nNumSkills) {
        if (static_cast<int>(static_cast<signed char>(m_pSkillRanksBase[nSkill])) >= static_cast<int>(nMaxRank))
            return 0;
    } else if (nMaxRank == 0) {
        return 0;
    }
    signed char nNewRank = (nSkill < g_pRules->m_nNumSkills) ? static_cast<signed char>(m_pSkillRanksBase[nSkill] + 1) : 1;
    SetSkillRankBase(nSkill, nNewRank);
    m_nSkillPointsRemaining += nCost;
    return 1;
}

void CSWCLevelUpStats::DecreaseSkillRank(unsigned short nSkill, unsigned char nRank)
{
    CSWSkill* pSkill = &g_pRules->m_paSkills[nSkill];
    if (!pSkill)
        return;
    bool bIsClassSkill = false;
    int bCanBuy = (pSkill->m_flags >> 3) & 1;
    for (unsigned char i = 0; !bIsClassSkill && i < m_nNumClasses; i++) {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClass(i)];
        if (pClass && pClass->IsSkillUseable(nSkill)) {
            bCanBuy = 1;
            if (pClass->IsSkillClassSkill(nSkill)) {
                bIsClassSkill = true;
                continue;
            }
#if !KOTOR1
            if (HasClassSkillFeat(nSkill))
                bIsClassSkill = true;
#endif
        }
    }

    short nCost;
    if (bIsClassSkill) {
        nCost = 1;
    } else {
        if (!bCanBuy)
            return;
        nCost = 2;
    }

    signed char nCurrentRank = (nSkill < g_pRules->m_nNumSkills) ? static_cast<signed char>(m_pSkillRanksBase[nSkill]) : 0;
    if (nCurrentRank == nRank)
        return;

    signed char nNewRank = (nSkill < g_pRules->m_nNumSkills) ? static_cast<signed char>(m_pSkillRanksBase[nSkill] - 1) : -1;
    SetSkillRankBase(nSkill, nNewRank);
    m_nSkillPointsRemaining += nCost;
}

void CSWCLevelUpStats::AddDefaultPackageSkills(int nClassId, int bSkipFlagCheck)
{
    unsigned char nNumSkills = g_pRules->m_nNumSkills;
    if (nNumSkills == 0)
        return;
    int nPoints = m_nSkillPointsRemaining;
    if (nPoints == 0)
        return;
    CSWClass* pClass = &g_pRules->m_paClasses[static_cast<unsigned char>(nClassId)];
    unsigned char nOrder = 1;
    do {
        unsigned int nSkill = pClass->GetRecommendedSkill(nOrder);
        if (nSkill != 0xffffffff) {
            bool bDo = true;
            if (bSkipFlagCheck == 0) {
                CSWSkill* pSkill = &g_pRules->m_paSkills[static_cast<unsigned short>(nSkill)];
                int bNotGoto = 0;
#if !KOTOR1
                if (nSkill == SKILL_STEALTH) {
                    CSWSCreature* pServer = m_pCreature->GetServerCreature();
                    bNotGoto = pServer->m_sTag.CompareNoCase("g0t0") != 0;
                }
#endif
                unsigned char nFlags = pSkill->m_flags;
                bDo = (nFlags & CSWSkill::FLAG_NPC_CAN_USE) != 0 && (m_nRace != CSWRace::CRACE_DROID ||
                (bNotGoto | ((nFlags >> 5) & 1)));
            }
            if (bDo) {
                while (IncreaseSkillRank(static_cast<unsigned short>(nSkill))) { }
                nPoints = m_nSkillPointsRemaining;
            }
        }
    } while (nOrder++ < nNumSkills && nPoints > 0);
}

int CSWCLevelUpStats::AddDefaultPackageFeats(int nClassId, CExoArrayList<unsigned short>* pFeats,
                                              int bAllowNormal, int bAllowBonus)
{
    unsigned char nNormalCount, nBonusCount;
    CalcNumberFeats(nNormalCount, nBonusCount);

    int nNormalLeft = bAllowNormal ? nNormalCount : 0;
    int nBonusLeft = bAllowBonus ? nBonusCount : 0;
    int nTotal = nNormalLeft + nBonusLeft;

    unsigned short nNumFeats = g_pRules->m_nFeats;
    if (nNumFeats == 0 || static_cast<unsigned char>(nTotal) == 0)
        return nTotal;
    CSWClass* pClass = &g_pRules->m_paClasses[static_cast<unsigned char>(nClassId)];
    int nOrder = 1;
    do {
        unsigned short nFeat = static_cast<unsigned short>(pClass->GetRecommendedFeat(nOrder));
        if (CanChooseFeat(nFeat, static_cast<unsigned char>(nNormalLeft), static_cast<unsigned char>(nBonusLeft), pFeats)) {
            pFeats->Add(nFeat);
            nTotal--;
        }
        nOrder++;
    } while (nOrder <= nNumFeats && static_cast<unsigned char>(nTotal) != 0);
    return nTotal;
}

unsigned int CSWCLevelUpStats::AddDefaultPackageSpells(CExoArrayList<unsigned int>* pSpells)
{
#if KOTOR1
    // K1: the class-count helper is expanded in place.
    unsigned int nLeft = 0;
    {
        unsigned char nClassPos = static_cast<unsigned char>(m_nNumClasses - 1);
        unsigned char nClassId = GetClass(nClassPos);
        CSWClass* pClass = &g_pRules->m_paClasses[nClassId];
        if (pClass &&
            (nClassId == CSWClass::CCLASS_JEDICONSULAR ||
             nClassId == CSWClass::CCLASS_JEDIGUARDIAN ||
             nClassId == CSWClass::CCLASS_JEDISENTINEL)) {
            unsigned int nGain = pClass->GetSpellGain(GetClassLevel(nClassPos), 0);
            nLeft = (static_cast<unsigned char>(nGain) == CSWClass::LEVEL_NONE) ? 0 : nGain;
        }
    }
#else
    unsigned int nLeft = ComputeNumberKnownSpellsLeft();
#endif
    CSWClass* pClass = &g_pRules->m_paClasses[GetClassLeveledUpIn()];
    if (pClass->m_bSpellCaster == 0)
        return nLeft;
    int bGoodAligned = m_alignmentGoodEvil > 40;
    int nNumSpells = g_pRules->m_pSpellArray->m_nCount;
    if (nNumSpells <= 0 || static_cast<unsigned char>(nLeft) == 0)
        return nLeft;
    int nAdded = 0;
    int nOrder = 1;
    for (;;) {
        unsigned int nSpellId = pClass->GetRecommendedSpell(bGoodAligned, nOrder);
        if (CanChooseSpell(nSpellId & 0xffff, static_cast<unsigned char>(nLeft), pSpells)) {
            pSpells->Add(nSpellId & 0xffff);
            nAdded++;
        }
        if (nOrder >= nNumSpells)
            break;
        nOrder++;
        if (nAdded >= static_cast<int>(static_cast<unsigned char>(nLeft)))
            break;
    }
    return nLeft;
}

void CSWCLevelUpStats::CalcNumberFeats(unsigned char& nNormal, unsigned char& nBonus)
{
    CSWRules* pRules = g_pRules;
    unsigned char nClassPos = static_cast<unsigned char>(m_nNumClasses - 1);
    unsigned char nLevel;
    CSWClass* pClass;
    if (nClassPos > 1) {
#ifdef K2_FIX_ENGINE_BUGS
        nNormal = 0;
        nBonus = 0;
        return;
#else
        nLevel = 0;
        pClass = &pRules->m_paClasses[CLASS_TYPE_INVALID];
#endif
    } else {
        CSWCCreatureStats_ClassInfo* pInfo = &m_classes[nClassPos];
        nLevel = pInfo->m_nLevel;
        if (nLevel < static_cast<int>(sizeof(CSWClass::m_normalFeats.m_aByLevel))) {
            pClass = &pRules->m_paClasses[pInfo->m_nClass];
        } else {
            nNormal = 0;
            nBonus = 0;
            return;
        }
    }
    nNormal = pClass->m_normalFeats.m_aByLevel[nLevel];
    nBonus = pClass->m_bonusFeats.m_aByLevel[nLevel];
}

// Walks every feat.2da row and grants the feats this creature qualifies for at its current level:
// first-level racial bonus feats, the last class slot's class-progression grants (the
// Jedi-multiclass range gated separately), PC-only grants, and a fixed companion cascade keyed on
// the party tag. Every grant site repeats the dedup/grow/append block.
// inlined CExoArrayList<unsigned short>::AddUnique(nId), as its deferred-inline form
#define KOTOR2_GRANT_FEAT(nId, bAlsoAddToList)                                 \
    do {                                                                      \
        if (m_lstFeats.Contains(nId) == 0)                                    \
            m_lstFeats.Add(nId);                                              \
        if (pFeats && (bAlsoAddToList))                                      \
            pFeats->Add(nId);                                               \
    } while (0)

#if KOTOR1
void CSWCLevelUpStats::AddGrantedFeats(CExoArrayList<unsigned int>* pFeats)
{
    for (unsigned short nFeatIdx = 0; nFeatIdx < g_pRules->m_nFeats; nFeatIdx++) {
        CSWFeat* pFeat = g_pRules->GetFeat(nFeatIdx);
        if (!pFeat)
            continue;

        if (GetLevel() == 1 && m_nExperience == 0 &&
            g_pRules->m_paRaces[m_nRace].IsFirstLevelGrantedFeat(nFeatIdx)) {
            KOTOR2_GRANT_FEAT(nFeatIdx, true);
            continue;
        }

        unsigned char nClassPos = static_cast<unsigned char>(m_nNumClasses - 1);
        unsigned char nClassId = (nClassPos < 2) ? m_classes[nClassPos].m_nClass : CLASS_TYPE_INVALID;
        CSWClass* pClass = &g_pRules->m_paClasses[nClassId];
        if (pClass == 0)
            continue;

        unsigned char nGrantLevel;
        if (pClass->IsGrantedFeat(nFeatIdx, nGrantLevel)) {
            unsigned char nLastLevel = (nClassPos < 2) ? m_classes[nClassPos].m_nLevel : 0;
            if (nGrantLevel == nLastLevel)
                KOTOR2_GRANT_FEAT(nFeatIdx, true);
        }
    }
}
#else
void CSWCLevelUpStats::AddGrantedFeats(CExoArrayList<unsigned int>* pFeats, bool bIsPC)
{
    unsigned char nTotalLevel = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++)
        nTotalLevel += GetClassLevel(i);

    unsigned char nTotalJediLevel = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++) {
        unsigned char nClassId = (i < 2) ? m_classes[i].m_nClass : CLASS_TYPE_INVALID;
        if (CSWClass::IsJedi(nClassId))
            nTotalJediLevel += (i < 2) ? m_classes[i].m_nLevel : 0;
    }

    for (unsigned short nFeatIdx = 0; nFeatIdx < g_pRules->m_nFeats; nFeatIdx++) {
        CSWFeat* pFeat = g_pRules->GetFeat(nFeatIdx);
        if (!pFeat)
            continue;

        if (GetLevel() == 1 && m_nExperience == 0 &&
            g_pRules->m_paRaces[m_nRace].IsFirstLevelGrantedFeat(nFeatIdx)) {
            KOTOR2_GRANT_FEAT(nFeatIdx, true);
            continue;
        }

        unsigned char nClassPos = static_cast<unsigned char>(m_nNumClasses - 1);
        unsigned char nClassId = (nClassPos < 2) ? m_classes[nClassPos].m_nClass : CLASS_TYPE_INVALID;
        CSWClass* pClass = &g_pRules->m_paClasses[nClassId];
        if (pClass == 0)
            continue;

        unsigned char nGrantLevel;
        if (pClass->IsGrantedFeat(nFeatIdx, nGrantLevel)) {
            if (static_cast<unsigned>(nFeatIdx - CSWSCreatureStats::FEAT_UNARMED_SPECIALIST_I) < 8) {
                // Call IsJedi separately per arm rather than reusing nClassId.
                bool bJedi = (nClassPos < 2) ? CSWClass::IsJedi(m_classes[nClassPos].m_nClass)
                                              : CSWClass::IsJedi(CLASS_TYPE_INVALID);
                if (bJedi && nGrantLevel == nTotalJediLevel) {
                    KOTOR2_GRANT_FEAT(nFeatIdx, true);
                }
            } else {
                unsigned char nLastLevel = (nClassPos < 2) ? m_classes[nClassPos].m_nLevel : 0;
                if (nGrantLevel == nLastLevel) {
                    if (CExoString(m_sTag).CompareNoCase("baodur") &&
                        pFeat->m_nBaodurGrantLevel == static_cast<unsigned char>(-1))
                        continue;
                    if (CExoString(m_sTag).CompareNoCase("kreia") &&
                        pFeat->m_nKreiaGrantLevel == static_cast<unsigned char>(-1))
                        continue;
                    KOTOR2_GRANT_FEAT(nFeatIdx, nFeatIdx != CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS &&
                                                    nFeatIdx != CSWSCreatureStats::FEAT_MINE_IMMUNITY);
                }
            }
        } else if (pClass->IsPCGrantedFeat(nFeatIdx) && bIsPC) {
            KOTOR2_GRANT_FEAT(nFeatIdx, true);
        } else if (CExoString(m_sTag).CompareNoCase("handmaiden")) {
            if (pFeat->m_nHandmaidenGrantLevel != 0 && pFeat->m_nHandmaidenGrantLevel == nTotalLevel)
                KOTOR2_GRANT_FEAT(nFeatIdx, true);
        } else if (CExoString(m_sTag).CompareNoCase("baodur")) {
            if (pFeat->m_nBaodurGrantLevel != 0 && pFeat->m_nBaodurGrantLevel == nTotalLevel)
                KOTOR2_GRANT_FEAT(nFeatIdx, true);
        } else if (CExoString(m_sTag).CompareNoCase("hanharr")) {
            if (pFeat->m_nHanharrGrantLevel != 0 && pFeat->m_nHanharrGrantLevel == nTotalLevel)
                KOTOR2_GRANT_FEAT(nFeatIdx, true);
        } else if (CExoString(m_sTag).CompareNoCase("hk47")) {
            if (pFeat->m_nHK47GrantLevel != 0 && pFeat->m_nHK47GrantLevel == nTotalLevel)
                KOTOR2_GRANT_FEAT(nFeatIdx, true);
        } else if (CExoString(m_sTag).CompareNoCase("g0t0")) {
            if (pFeat->m_nG0T0GrantLevel != 0 && pFeat->m_nG0T0GrantLevel == nTotalLevel)
                KOTOR2_GRANT_FEAT(nFeatIdx, true);
        } else if (CExoString(m_sTag).CompareNoCase("atton")) {
            if (pFeat->m_nAttonGrantLevel != 0 && pFeat->m_nAttonGrantLevel == nTotalLevel)
                KOTOR2_GRANT_FEAT(nFeatIdx, true);
        }
    }

    // bIsPC-gated hardcoded feat 206; a duplicate hit returns early, as at the other grant sites.
    if (bIsPC) {
        for (int i = 0; i < m_lstFeats.m_count; i++)
            if (m_lstFeats.m_pArray[i] == FEAT_WAR_VETERAN)
                return;
        KOTOR2_GRANT_FEAT(FEAT_WAR_VETERAN, true);
    }
}
#endif
#undef KOTOR2_GRANT_FEAT

// Grants class-specific bonus force powers to classes[0]/classes[1] only, always targeting the
// highest class slot via AddKnownSpell. Per class id (3..16 only): 13/16 and 14 pick fixed ids
// from the highest slot's level (1/7/13 and 1/5/9), 3/4/5 gate on the Jedi level minus 11 and
// index a per-class table, and 12/15 gate on (highest level - 1) being a multiple of 4 in
// [0,20).
#if !KOTOR1
void CSWCLevelUpStats::AddGrantedPowers(CExoArrayList<unsigned int>* pPowers)
{
    if (m_nNumClasses == 0)
        return;

    unsigned char nClassPos = m_nNumClasses - 1;
    unsigned char nHighestLevel = (nClassPos <= 1) ? m_classes[nClassPos].m_nLevel : 0;

    int nJediLevel = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++)
        if (CSWClass::IsJedi(GetClass(i)))
            nJediLevel += GetClassLevel(i);

    if (m_nNumClasses == 0)
        return;

    static const unsigned int KOTOR2_JEDI_POWERS_3[4] = {258, 259, 260, 265};
    static const unsigned int KOTOR2_JEDI_POWERS_4[4] = {258, 265, 259, 260};
    static const unsigned int KOTOR2_JEDI_POWERS_5[4] = {258, 260, 265, 259};

    unsigned char nJediTier = static_cast<unsigned char>(nJediLevel - 11);
    unsigned char nLevelRem = (nHighestLevel - 1) & 3;
    unsigned char nLevelTier = (nHighestLevel - 1) >> 2;

    for (unsigned char nSlot = 0; nSlot < m_nNumClasses && nSlot < 2; nSlot++) {
        unsigned char nClassId = m_classes[nSlot].m_nClass;
        if (nClassId < CLASS_TYPE_JEDIGUARDIAN || nClassId > CLASS_TYPE_SITHASSASSIN)
            continue;

        unsigned int nPowerId;
        switch (nClassId) {
        case CLASS_TYPE_JEDIWATCHMAN:
        case CLASS_TYPE_SITHASSASSIN:
            if (nHighestLevel == 1)
                nPowerId = FORCE_POWER_FORCE_CAMOUFLAGE;
            else if (nHighestLevel == 13)
                nPowerId = FORCE_POWER_MASTER_FORCE_CAMOUFLAGE;
            else if (nHighestLevel == 7)
                nPowerId = FORCE_POWER_IMPROVED_FORCE_CAMOUFLAGE;
            else
                continue;
            break;
        case CLASS_TYPE_JEDIGUARDIAN:
            if (nJediTier >= 4)
                continue;
            nPowerId = KOTOR2_JEDI_POWERS_3[nJediTier];
            break;
        case CLASS_TYPE_JEDICONSULAR:
            if (nJediTier >= 4)
                continue;
            nPowerId = KOTOR2_JEDI_POWERS_4[nJediTier];
            break;
        case CLASS_TYPE_JEDISENTINEL:
            if (nJediTier >= 4)
                continue;
            nPowerId = KOTOR2_JEDI_POWERS_5[nJediTier];
            break;
        case CLASS_TYPE_JEDIMASTER:
            if (nLevelRem != 0 || nLevelTier >= 5)
                continue;
            nPowerId = nLevelTier + FORCE_POWER_INSPIRE_FOLLOWERS_I;
            break;
        case CLASS_TYPE_SITHMARAUDER:
            if (nHighestLevel == 1)
                nPowerId = FORCE_POWER_FURY;
            else if (nHighestLevel == 9)
                nPowerId = FORCE_POWER_MASTER_FURY;
            else if (nHighestLevel == 5)
                nPowerId = FORCE_POWER_IMPROVED_FURY;
            else
                continue;
            break;
        case CLASS_TYPE_SITHLORD:
            if (nLevelRem != 0 || nLevelTier >= 5)
                continue;
            nPowerId = nLevelTier + FORCE_POWER_CRUSH_OPPOSITION_I;
            break;
        default:
            continue;
        }

        AddKnownSpell(m_nNumClasses - 1, nPowerId);
        if (pPowers)
            pPowers->Add(nPowerId);
    }
}
#endif

void CSWCLevelUpStats::ClearPackage()
{
    m_nSkillPointsRemaining = 0;
    ClearSkills();
    UpdateAbilityModifiedSkillRanks();
    ClearFeats();
    ClearKnownSpells();
#if KOTOR1
    AddGrantedFeats(0);
#else
    AddGrantedFeats(0, false);
#endif
    // Resets the count only; the backing array and capacity stay allocated.
    // inlined CExoArrayList<CResRef>::DelIndex(m_count - 1), popped until empty
    if (m_lstPackageResRefs.m_count != 0)
        m_lstPackageResRefs.m_count = 0;
}

void CSWCCreatureStats::ReadStatsFromGff(CResGFF* pGFF, CResStruct* pStruct)
{
    int success = 1;

    CExoString strTemp;
    CExoLocString locTemp;
    CResList classList;
    CResList skillList;
    CResList featList;

    {
        CExoLocString emptyLoc;
        locTemp = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("FirstName"), success, emptyLoc);
    }
    locTemp.GetStringLoc(0, &strTemp, 0);
    m_name = strTemp;

    {
        CExoLocString emptyLoc;
        locTemp = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LastName"), success, emptyLoc);
    }
    locTemp.GetStringLoc(0, &strTemp, 0);
    m_sLastName = strTemp;

    {
        CExoLocString emptyLoc;
        locTemp = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("Description"), success, emptyLoc);
    }
    locTemp.GetStringLoc(0, &strTemp, 0);
    m_sDescription = strTemp;

    m_nGender = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Gender"), success, m_nGender);
    m_nRace = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Race"), success, static_cast<unsigned char>(m_nRace));
    {
        CExoString result = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Subrace"), success, m_raceString);
        m_raceString = result;
    }
    m_nSubraceIndex = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("SubraceIndex"), success, m_nSubraceIndex);
    m_nStartingPackage = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("StartingPackage"), success, m_nStartingPackage);
    m_nArmorClass = pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("ArmorClass"), success, static_cast<short>(m_nArmorClass));

    if (pGFF->GetList(&classList, pStruct, const_cast<char*>("ClassList")) &&
        static_cast<unsigned char>(pGFF->GetListCount(&classList)) != 0) {
        unsigned char nCount = static_cast<unsigned char>(pGFF->GetListCount(&classList));
        m_nNumClasses = (nCount > 2) ? 2 : nCount;
        if (m_nNumClasses != 0) {
            for (unsigned char i = 0; i < m_nNumClasses; i++) {
                CResStruct classElem;
                if (pGFF->GetListElement(&classElem, &classList, i)) {
                    unsigned char defaultClass = (i < 2) ? m_classes[i].m_nClass : CLASS_TYPE_INVALID;
                    unsigned int nClass = pGFF->ReadFieldINT(&classElem, const_cast<char*>("Class"), success, defaultClass);
                    if (nClass == 0xffffffff || success != 1) {
                        if (i < 2)
                            nClass = m_classes[i].m_nClass;
                    } else if (i < 2) {
                        m_classes[i].m_nClass = static_cast<unsigned char>(nClass);
                    }
                    if (i < 2 && static_cast<unsigned char>(nClass) != CLASS_TYPE_INVALID) {
                        unsigned short nLevel = pGFF->ReadFieldSHORT(&classElem, const_cast<char*>("ClassLevel"), success, m_classes[i].m_nLevel);
                        if (static_cast<unsigned char>(nLevel) != 0)
                            m_classes[i].m_nLevel = static_cast<unsigned char>(nLevel);
                    }
                }
            }
            if (m_nNumClasses >= 3)
                m_nNumClasses = 2;
            if (m_nNumClasses == 2 && m_classes[1].m_nClass == CLASS_TYPE_INVALID)
                m_nNumClasses = 1;
        }
    }

    CSWRace* pRace = &g_pRules->m_paRaces[m_nRace];
    m_abScoresBase[0] = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Str"), success, m_abScoresBase[0]);
    m_abScoresBase[1] = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Dex"), success, m_abScoresBase[1]);
    m_abScoresBase[3] = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Int"), success, m_abScoresBase[3]);
    m_abScoresBase[4] = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Wis"), success, m_abScoresBase[4]);
    m_abScoresBase[2] = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Con"), success, m_abScoresBase[2]);
    m_abScoresBase[5] = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Cha"), success, m_abScoresBase[5]);
    m_abScores[0] = static_cast<unsigned char>(m_abScoresBase[0] + pRace->m_anAbilityAdjust[0]);
    m_abScores[1] = static_cast<unsigned char>(m_abScoresBase[1] + pRace->m_anAbilityAdjust[1]);
    m_abScores[3] = static_cast<unsigned char>(m_abScoresBase[3] + pRace->m_anAbilityAdjust[2]);
    m_abScores[4] = static_cast<unsigned char>(m_abScoresBase[4] + pRace->m_anAbilityAdjust[4]);
    m_abScores[2] = static_cast<unsigned char>(m_abScoresBase[2] + pRace->m_anAbilityAdjust[5]);
    m_abScores[5] = static_cast<unsigned char>(pRace->m_anAbilityAdjust[3] + m_abScoresBase[5]);

    m_nHitPointsBase = pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("HitPoints"), success, m_nHitPointsBase);
    m_nMaxHitPoints = pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("MaxHitPoints"), success, m_nMaxHitPoints);
    m_nCurrentHitPoints = pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("PregameCurrent"), success, 0);
    m_nExperience = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Experience"), success, m_nExperience);

    unsigned short nPortraitId = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("PortraitId"), success, CSWPortrait::PORTRAIT_ID_NONE);
    SetPortraitId(nPortraitId);
    if (nPortraitId > CSWPortrait::PORTRAIT_ID_LAST) {
        { CExoString unused; }
        CResRef newResRef = pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("Portrait"), success, m_portrait.m_baseResRef);
        CheckCustomPortrait(&newResRef, static_cast<unsigned>(m_nGender == GENDER_FEMALE));
        m_portrait.SetPortrait(newResRef);
    }

    unsigned short nGoodEvil = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("GoodEvil"), success, static_cast<unsigned char>(m_alignmentGoodEvil));
    if (nGoodEvil > 99)
        nGoodEvil = 100;
    m_alignmentGoodEvil = nGoodEvil;

#if !KOTOR1
    char nBaseCNPCAlignment = pGFF->ReadFieldCHAR(pStruct, const_cast<char*>("BaseCNPCAlignment"), success, -1);
    if (static_cast<unsigned char>(nBaseCNPCAlignment) == 0xff)
        nBaseCNPCAlignment = static_cast<char>(m_alignmentGoodEvil);
    int nClampedAlignment = nBaseCNPCAlignment;
    if (nClampedAlignment < 0)
        nClampedAlignment = -1;
    if (nClampedAlignment > 99)
        nClampedAlignment = 100;
    m_baseCNPCAlignment = static_cast<short>(nClampedAlignment);
#endif

    m_nReflexSavingThrow = pGFF->ReadFieldCHAR(pStruct, const_cast<char*>("RefSaveThrow"), success, m_nReflexSavingThrow);
    m_nWillSavingThrow = pGFF->ReadFieldCHAR(pStruct, const_cast<char*>("WillSaveThrow"), success, m_nWillSavingThrow);
    m_nFortSavingThrow = pGFF->ReadFieldCHAR(pStruct, const_cast<char*>("FortSaveThrow"), success, m_nFortSavingThrow);

    if (pGFF->GetList(&skillList, pStruct, const_cast<char*>("SkillList"))) {
        unsigned short nSkillCount = static_cast<unsigned short>(pGFF->GetListCount(&skillList));
        if (nSkillCount != 0) {
            for (unsigned short i = 0; i < nSkillCount; i++) {
                CResStruct skillElem;
                if (pGFF->GetListElement(&skillElem, &skillList, i)) {
                    unsigned char defaultRank = (i < g_pRules->m_nNumSkills) ? m_pSkillRanksBase[i] : 0;
                    unsigned char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success, defaultRank);
                    if (i < g_pRules->m_nNumSkills)
                        m_pSkillRanksBase[i] = nRank;
                }
            }
        }
    }
    if (g_pRules->m_nNumSkills != 0) {
        for (unsigned short i = 0; i < g_pRules->m_nNumSkills; i++) {
            signed char nBaseRank = m_pSkillRanksBase[i];
            signed char nTotal = 0;
            if (nBaseRank != 0 || (g_pRules->m_paSkills[i].m_flags & 2) != 0)
                nTotal = static_cast<signed char>(GetSkillModifier(i) + nBaseRank);
            m_pSkillRanks[i] = nTotal;
        }
    }

    if (pGFF->GetList(&featList, pStruct, const_cast<char*>("FeatList"))) {
        unsigned short nFeatCount = static_cast<unsigned short>(pGFF->GetListCount(&featList));
        if (nFeatCount != 0) {
            for (unsigned short i = 0; i < nFeatCount; i++) {
                CResStruct featElem;
                if (pGFF->GetListElement(&featElem, &featList, i)) {
                    unsigned short nFeat = pGFF->ReadFieldWORD(&featElem, const_cast<char*>("Feat"), success, 0);
                    if (success != 0) {
                        m_lstFeats.AddUnique(nFeat);
                    }
                }
            }
        }
    }

    if (m_nNumClasses != 0) {
        for (unsigned char i = 0; i < m_nNumClasses; i++) {
            CResStruct classElem;
            if (pGFF->GetListElement(&classElem, &classList, i)) {
                CExoString label;
                label.Format(const_cast<char*>("%s%d"), const_cast<char*>("KnownList"), 0);
                CResList spellList;
                if (pGFF->GetList(&spellList, &classElem, static_cast<char*>(label.CStr()))) {
                    unsigned short j = 0;
                    while (true) {
                        if (pGFF->GetListCount(&spellList) <= j)
                            break;
                        CResStruct spellElem;
                        if (pGFF->GetListElement(&spellElem, &spellList, j)) {
                            unsigned short nSpell = pGFF->ReadFieldWORD(&spellElem, const_cast<char*>("Spell"), success, 0xffff);
                            AddKnownSpell(i, nSpell);
                        }
                        j++;
                    }
                }
            }
        }
    }

    InitializeCombatInformation();
    m_pCombatInformation->LoadData(pGFF, pStruct);
}

// Builds the radial-menu entry list for one call's worth of a creature's known spells/powers, for
// CSWCCreature::GetStances, GetSelfForcePowers and GetHostileForcePowers (nMenuType 1 =
// hostile-targeted, 3 = self/stance). Entries are appended to menuList from its current count,
// which callers share across several producers per menu build.
//
// K1 delegates to CSWCCreatureStats_ClassInfo::GetMenuInfo once per class; K2 does the work here,
// with nExcludeMask matched against CSWSpell::m_nExclusionMask, nForceItemMask against
// m_nForbidItemMask/m_nRequireItemMask, fRangeSq flagging entries beyond the max range,
// nDiscountTier -1/0/1/2 for none/50/40/30 percent off, nCurrentHitPoints the minimum-discount
// threshold, and nStanceOnly restricting the candidates to the stance id block instead of
// excluding it.

#if KOTOR1
void CSWCCreatureStats::GetMenuInfo(CExoArrayList<CSWGuiInterfaceAction>& menuList, unsigned int nMenuType,
                                     unsigned int nForcePoints, int nExcludeMask, unsigned int nForceItemMask,
                                     float fRangeSq)
{
    for (int i = 0; i < m_nNumClasses; i++)
        m_classes[i].GetMenuInfo(menuList, nMenuType, nForcePoints, m_alignmentGoodEvil, nExcludeMask,
                                 nForceItemMask, fRangeSq);
}
#else
void CSWCCreatureStats::GetMenuInfo(CExoArrayList<CSWGuiInterfaceAction>& menuList, unsigned int nMenuType,
                                     unsigned int nForcePoints, int nExcludeMask, unsigned int nForceItemMask,
                                     float fRangeSq, int nRoomForceRating, int nDiscountTier, int nCurrentHitPoints,
                                     int nStanceOnly, int nStanceId, int bTargetIsDroid, char cCHAMod)
{
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule->GetArea()->m_nPlayerRestrictMode != 0 || m_nNumClasses == 0)
        return;

    // Phase 1: concatenate every class's known level-0 spell/power ids.
    CExoArrayList<unsigned int> spellIds;
    int nSpellCount = 0;
    for (unsigned char i = 0; i < m_nNumClasses; i++) {
        int nThisCount = m_classes[i].m_spellLevels[0].m_count;
        spellIds.SetSize(nThisCount + nSpellCount);
        for (int k = 0; k < nThisCount; k++)
            spellIds.m_pArray[nSpellCount + k] = m_classes[i].m_spellLevels[0].m_pArray[k];
        nSpellCount = spellIds.m_count;
    }

    if (nSpellCount > 0) {
        // Phase 2: filter and decode into candidate (subgroup, rank, spell, id) records, deduped
        // per subgroup so only the highest-rank power in each menu slot survives.
        CExoArrayList<CSWSpell*>    candSpells;
        CExoArrayList<short>        candRank;
        CExoArrayList<short>        candSubgroup;
        CExoArrayList<unsigned int> candIds;
        int nAlignment = m_alignmentGoodEvil;

        for (int i = 0; i < nSpellCount; i++) {
            int nSpellId = spellIds.m_pArray[i];
            CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
            if (pSpell == 0)
                continue;

            bool bExcludeMaskOk;
            if (bTargetIsDroid != 0 && (nSpellId == FORCE_POWER_MIND_TRICK ||
                nSpellId == FORCE_POWER_CONFUSION)) {
                bExcludeMaskOk = false;
            } else {
                bExcludeMaskOk = (bTargetIsDroid != 0 ||
                (nSpellId != FORCE_POWER_BEAST_TRICK && nSpellId != FORCE_POWER_BEAST_CONFUSION)) &&
                                 (pSpell->m_nExclusionMask & nExcludeMask) == 0;
            }

            // One shared alignment test for all four arms below.
            bool bGateA;
            if (nSpellId == FORCE_POWER_CONFUSION) {
                char bConfActive = 0;
                CSWGlobalVariableTable* pTable = g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
                CExoString name("000_Human_Conf_Active");
                pTable->GetValueNumber(name, bConfActive);
                bGateA = (bConfActive > 0) ? false : bExcludeMaskOk;
            } else if (nSpellId == FORCE_POWER_DROID_CONFUSION) {
                char bConfActive = 0;
                CSWGlobalVariableTable* pTable = g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
                CExoString name("000_Droid_Conf_Active");
                pTable->GetValueNumber(name, bConfActive);
                bGateA = (bConfActive > 0) ? false : bExcludeMaskOk;
            } else if (nSpellId == FORCE_POWER_BEAST_CONFUSION) {
                char bConfActive = 0;
                CSWGlobalVariableTable* pTable = g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
                CExoString name("000_Beast_Conf_Active");
                pTable->GetValueNumber(name, bConfActive);
                bGateA = (bConfActive > 0) ? false : bExcludeMaskOk;
            } else {
                bGateA = bExcludeMaskOk;
            }

            bool bFinalGate;
            if (static_cast<unsigned int>(nSpellId - FORCE_POWER_CRUSH_OPPOSITION_I) < 6)
                bFinalGate = (nAlignment > 40) ? false : bGateA;
            else
                bFinalGate = (static_cast<unsigned int>(nSpellId - FORCE_POWER_INSPIRE_FOLLOWERS_I) < 6 &&
                              nAlignment < 60) ? false : bGateA;

            bool bInStanceRange = static_cast<unsigned int>(nSpellId - FORM_SABER_I_SHII_CHO) <=
                                  FORM_FORCE_IV_MASTERY - FORM_SABER_I_SHII_CHO;
            bool bStanceGate = bInStanceRange ? bFinalGate : false;
            bool bNonStanceGate = !bInStanceRange ? bFinalGate : false;
            bool bGate = bNonStanceGate;
            if (nStanceOnly != 0)
                bGate = bStanceGate;

            if (bGate && pSpell->m_nSubRadialCount != 0) {
                for (unsigned char s = 0; s < pSpell->m_nSubRadialCount; s++) {
                    short nRank = static_cast<short>(pSpell->GetPriority(s));
                    short nSubgroup = static_cast<short>(pSpell->GetSlot(s));
                    if (pSpell->GetMenu(s) == static_cast<int>(nMenuType)) {

                        bool bDone = false;
                        for (int64_t idx = 0; idx < candSpells.m_count; idx++) {
                            short nExisting = candSubgroup.m_pArray[idx];
                            if (nSubgroup == 17 && nExisting == 16) {
                                candRank.m_pArray[idx] = nRank;
                                candSpells.m_pArray[idx] = pSpell;
                                candIds.m_pArray[idx] = static_cast<unsigned int>(nSpellId);
                                candSubgroup.m_pArray[idx] = 17;
                                bDone = true;
                                break;
                            }
                            if (nSubgroup == 21 && nExisting == 20) {
                                candRank.m_pArray[idx] = nRank;
                                candSpells.m_pArray[idx] = pSpell;
                                candIds.m_pArray[idx] = static_cast<unsigned int>(nSpellId);
                                candSubgroup.m_pArray[idx] = 21;
                                bDone = true;
                                break;
                            }
                            if (nExisting == nSubgroup) {
                                if (candRank.m_pArray[idx] < nRank) {
                                    candRank.m_pArray[idx] = nRank;
                                    candSpells.m_pArray[idx] = pSpell;
                                    candIds.m_pArray[idx] = static_cast<unsigned int>(nSpellId);
                                }
                                bDone = true;
                                break;
                            }
                            if (nSubgroup < nExisting) {
                                candRank.Insert(nRank, static_cast<int>(idx));
                                candSubgroup.Insert(nSubgroup, static_cast<int>(idx));
                                candSpells.Insert(pSpell, static_cast<int>(idx));
                                candIds.Insert(static_cast<unsigned int>(nSpellId), static_cast<int>(idx));
                                bDone = true;
                                break;
                            }
                        }
                        if (!bDone) {
                            candSpells.Add(pSpell);
                            candRank.Add(nRank);
                            candSubgroup.Add(nSubgroup);
                            candIds.Add(static_cast<unsigned int>(nSpellId));
                        }
                    }
                }
            }
        }

        // Phase 3: emit one CSWGuiInterfaceAction per surviving candidate.
        if (candSpells.m_count > 0) {
            int nBase = menuList.m_count;
            int nDiscountPct = (2 < static_cast<unsigned int>(nDiscountTier)) ? 0 : (nDiscountTier * -10 + 50);
            CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;

            for (int i = 0; i < candSpells.m_count; i++) {
                CSWSpell* pSpell = candSpells.m_pArray[i];
                menuList.SetSize(nBase + i + 1);

                CSWGuiInterfaceAction& entry = menuList.m_pArray[nBase + i];
                entry.m_sName = pApp->GetGUIString(pSpell->m_nNameStrRef);
                entry.m_nActionId = candIds.m_pArray[i] | CSWGuiInterfaceAction::ACTION_ID_SPELL;
                entry.m_resRef = pSpell->m_iconResRef;
                entry.m_nFlags = entry.m_nFlags & ~ACTION_FLAG_REASON_MASK;

                bool bAffordable;
                if (nDiscountTier == -1) {
                    int nCost = pSpell->GetAdjustedForcePoints(nAlignment, 1, nRoomForceRating, nStanceId, cCHAMod);
                    if (nForcePoints < static_cast<unsigned int>(nCost)) {
                        entry.m_nFlags =
                            (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_FORCE_DEPLETED;
                        bAffordable = false;
                    } else {
                        bAffordable = true;
                    }
                } else {
                    int nCost = pSpell->GetAdjustedForcePoints(nAlignment, 1, nRoomForceRating, nStanceId, cCHAMod);
                    int nDiscount = (nDiscountPct * nCost) / 100;
                    if (nForcePoints < static_cast<unsigned int>(nCost - nDiscount)) {
                        entry.m_nFlags =
                            (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_FORCE_DEPLETED;
                        bAffordable = false;
                    } else if (nDiscount < nCurrentHitPoints) {
                        bAffordable = true;
                    } else {
                        entry.m_nFlags =
                            (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_NOT_ENOUGH_HEALTH;
                        bAffordable = false;
                    }
                }

                float fRange = GetMinSpellRange(pSpell->m_sRangeCode);
                if (fRangeSq <= fRange * fRange) {
                    entry.m_nFlags =
                        (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_TARGET_TOO_CLOSE;
                    bAffordable = false;
                }

                unsigned char bBit0;
                if ((pSpell->m_nForbidItemMask & nForceItemMask) == 0) {
                    if (bAffordable) {
                        if ((pSpell->m_nRequireItemMask & ~nForceItemMask) != 0) {
                            entry.m_nFlags =
                                (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_MISSING_ITEM;
                            bBit0 = 0;
                        } else {
                            bBit0 = 1;
                        }
                    } else {
                        bBit0 = 0;
                    }
                } else {
                    entry.m_nFlags =
                        (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_RESTRICTED_BY_ARMOR;
                    bBit0 = 0;
                }
                entry.m_nFlags = (entry.m_nFlags & ~ACTION_FLAG_ENABLED) | bBit0;
            }
        }
    }
}
#endif // KOTOR1

#if KOTOR1
void CSWCCreatureStats_ClassInfo::GetMenuInfo(CExoArrayList<CSWGuiInterfaceAction>& menuList,
                                               unsigned int nMenuType, unsigned int nForcePoints,
                                               unsigned int nAlignment, int nExcludeMask,
                                               unsigned int nForceItemMask, float fRangeSq)
{
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule->GetArea()->m_nPlayerRestrictMode != 0)
        return;

    int nSpellCount = m_spellLevels[0].m_count;
    if (nSpellCount < 1)
        return;

    CExoArrayList<CSWSpell*>    candSpells;
    CExoArrayList<short>        candRank;
    CExoArrayList<short>        candSubgroup;
    CExoArrayList<unsigned int> candIds;

    for (int i = 0; i < nSpellCount; i++) {
        int nSpellId = m_spellLevels[0].m_pArray[i];
        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
        if (pSpell == 0 || (pSpell->m_nExclusionMask & nExcludeMask) != 0)
            continue;

        for (unsigned char s = 0; s < pSpell->m_nSubRadialCount; s++) {
            short nRank = static_cast<short>(pSpell->GetPriority(s));
            short nSubgroup = static_cast<short>(pSpell->GetSlot(s));
            if (pSpell->GetMenu(s) == static_cast<int>(nMenuType)) {

                bool bDone = false;
                for (int64_t idx = 0; idx < candSpells.m_count; idx++) {
                    short nExisting = candSubgroup.m_pArray[idx];
                    if (nExisting == nSubgroup) {
                        if (candRank.m_pArray[idx] < nRank) {
                            candRank.m_pArray[idx] = nRank;
                            candSpells.m_pArray[idx] = pSpell;
                            candIds.m_pArray[idx] = static_cast<unsigned int>(nSpellId);
                        }
                        bDone = true;
                        break;
                    }
                    if (nSubgroup < nExisting) {
                        candRank.Insert(nRank, static_cast<int>(idx));
                        candSubgroup.Insert(nSubgroup, static_cast<int>(idx));
                        candSpells.Insert(pSpell, static_cast<int>(idx));
                        candIds.Insert(static_cast<unsigned int>(nSpellId), static_cast<int>(idx));
                        bDone = true;
                        break;
                    }
                }
                if (!bDone) {
                    candSpells.Add(pSpell);
                    candRank.Add(nRank);
                    candSubgroup.Add(nSubgroup);
                    candIds.Add(static_cast<unsigned int>(nSpellId));
                }
            }
        }
    }

    if (candSpells.m_count > 0) {
        int nBase = menuList.m_count;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;

        for (int i = 0; i < candSpells.m_count; i++) {
            CSWSpell* pSpell = candSpells.m_pArray[i];
            menuList.SetSize(nBase + i + 1);

            CSWGuiInterfaceAction& entry = menuList.m_pArray[nBase + i];
            entry.m_sName = pApp->GetGUIString(pSpell->m_nNameStrRef);
            entry.m_nActionId = candIds.m_pArray[i] | CSWGuiInterfaceAction::ACTION_ID_SPELL;
            entry.m_resRef = pSpell->m_iconResRef;
            entry.m_nFlags = entry.m_nFlags & ~ACTION_FLAG_REASON_MASK;

            bool bAffordable = true;
            int nCost = pSpell->GetAdjustedForcePoints(nAlignment, 1);
            if (nForcePoints < static_cast<unsigned int>(nCost)) {
                entry.m_nFlags =
                    (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_FORCE_DEPLETED;
                bAffordable = false;
            }

            float fRange = GetMinSpellRange(pSpell->m_sRangeCode);
            if (fRangeSq <= fRange * fRange) {
                entry.m_nFlags =
                    (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_TARGET_TOO_CLOSE;
                bAffordable = false;
            }

            unsigned char bBit0;
            if ((pSpell->m_nForbidItemMask & nForceItemMask) == 0) {
                if (bAffordable) {
                    if ((pSpell->m_nRequireItemMask & ~nForceItemMask) != 0) {
                        entry.m_nFlags =
                            (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_MISSING_ITEM;
                        bBit0 = 0;
                    } else {
                        bBit0 = 1;
                    }
                } else {
                    bBit0 = 0;
                }
            } else {
                entry.m_nFlags =
                    (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_RESTRICTED_BY_ARMOR;
                bBit0 = 0;
            }
            entry.m_nFlags = (entry.m_nFlags & ~ACTION_FLAG_ENABLED) | bBit0;
        }
    }
}
#else
void CSWCCreatureStats_ClassInfo::GetMenuInfo(CExoArrayList<CSWGuiInterfaceAction>& menuList,
                                               unsigned int nMenuType, unsigned int nForcePoints,
                                               unsigned int nAlignment, int nExcludeMask,
                                               unsigned int nForceItemMask, float fRangeSq,
                                               int nRoomForceRating, int nDiscountTier,
                                               int nCurrentHitPoints, int nStanceOnly, int nStanceId,
                                               int bTargetIsDroid, char cCHAMod)
{
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule->GetArea()->m_nPlayerRestrictMode != 0)
        return;

    // Phase 2: filter and decode into candidate (subgroup, rank, spell, id) records, deduped per
    // subgroup so only the highest-rank power in each menu slot survives.
    int nSpellCount = m_spellLevels[0].m_count;
    if (nSpellCount < 1)
        return;

    CExoArrayList<CSWSpell*>    candSpells;
    CExoArrayList<short>        candRank;
    CExoArrayList<short>        candSubgroup;
    CExoArrayList<unsigned int> candIds;

    for (int i = 0; i < nSpellCount; i++) {
        int nSpellId = m_spellLevels[0].m_pArray[i];
        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
        if (pSpell == 0)
            continue;

        bool bExcludeMaskOk;
        if (bTargetIsDroid != 0 && (nSpellId == FORCE_POWER_MIND_TRICK ||
            nSpellId == FORCE_POWER_CONFUSION)) {
            bExcludeMaskOk = false;
        } else {
            bExcludeMaskOk = (bTargetIsDroid != 0 ||
            (nSpellId != FORCE_POWER_BEAST_TRICK && nSpellId != FORCE_POWER_BEAST_CONFUSION)) &&
                             (pSpell->m_nExclusionMask & nExcludeMask) == 0;
        }

        // One shared alignment test for all four arms below.
        bool bGateA;
        if (nSpellId == FORCE_POWER_CONFUSION) {
            char bConfActive = 0;
            CSWGlobalVariableTable* pTable = g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
            CExoString name("000_Human_Conf_Active");
            pTable->GetValueNumber(name, bConfActive);
            bGateA = (bConfActive > 0) ? false : bExcludeMaskOk;
        } else if (nSpellId == FORCE_POWER_DROID_CONFUSION) {
            char bConfActive = 0;
            CSWGlobalVariableTable* pTable = g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
            CExoString name("000_Droid_Conf_Active");
            pTable->GetValueNumber(name, bConfActive);
            bGateA = (bConfActive > 0) ? false : bExcludeMaskOk;
        } else if (nSpellId == FORCE_POWER_BEAST_CONFUSION) {
            char bConfActive = 0;
            CSWGlobalVariableTable* pTable = g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
            CExoString name("000_Beast_Conf_Active");
            pTable->GetValueNumber(name, bConfActive);
            bGateA = (bConfActive > 0) ? false : bExcludeMaskOk;
        } else {
            bGateA = bExcludeMaskOk;
        }

        bool bFinalGate;
        if (static_cast<unsigned int>(nSpellId - FORCE_POWER_CRUSH_OPPOSITION_I) < 6)
            bFinalGate = (nAlignment > 40) ? false : bGateA;
        else
            bFinalGate = (static_cast<unsigned int>(nSpellId - FORCE_POWER_INSPIRE_FOLLOWERS_I) < 6 &&
                          nAlignment < 60) ? false : bGateA;

        bool bInStanceRange = static_cast<unsigned int>(nSpellId - FORM_SABER_I_SHII_CHO) <=
                              FORM_FORCE_IV_MASTERY - FORM_SABER_I_SHII_CHO;
        bool bStanceGate = bInStanceRange ? bFinalGate : false;
        bool bNonStanceGate = !bInStanceRange ? bFinalGate : false;
        bool bGate = bNonStanceGate;
        if (nStanceOnly != 0)
            bGate = bStanceGate;

        if (bGate && pSpell->m_nSubRadialCount != 0) {
            for (unsigned char s = 0; s < pSpell->m_nSubRadialCount; s++) {
                // All three decoders run before the menu test, as in the class-record twin above.
                short nRank = static_cast<short>(pSpell->GetPriority(s));
                short nSubgroup = static_cast<short>(pSpell->GetSlot(s));
                if (pSpell->GetMenu(s) == static_cast<int>(nMenuType)) {

                    bool bDone = false;
                    for (int64_t idx = 0; idx < candSpells.m_count; idx++) {
                        short nExisting = candSubgroup.m_pArray[idx];
                        if (nExisting == nSubgroup) {
                            if (candRank.m_pArray[idx] < nRank) {
                                candRank.m_pArray[idx] = nRank;
                                candSpells.m_pArray[idx] = pSpell;
                                candIds.m_pArray[idx] = static_cast<unsigned int>(nSpellId);
                            }
                            bDone = true;
                            break;
                        }
                        if (nSubgroup < nExisting) {
                            candRank.Insert(nRank, static_cast<int>(idx));
                            candSubgroup.Insert(nSubgroup, static_cast<int>(idx));
                            candSpells.Insert(pSpell, static_cast<int>(idx));
                            candIds.Insert(static_cast<unsigned int>(nSpellId), static_cast<int>(idx));
                            bDone = true;
                            break;
                        }
                    }
                    if (!bDone) {
                        candSpells.Add(pSpell);
                        candRank.Add(nRank);
                        candSubgroup.Add(nSubgroup);
                        candIds.Add(static_cast<unsigned int>(nSpellId));
                    }
                }
            }
        }
    }

    // Phase 3: emit one CSWGuiInterfaceAction per surviving candidate.
    if (candSpells.m_count > 0) {
        int nBase = menuList.m_count;
        int nDiscountPct = (2 < static_cast<unsigned int>(nDiscountTier)) ? 0 : (nDiscountTier * -10 + 50);
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;

        for (int i = 0; i < candSpells.m_count; i++) {
            CSWSpell* pSpell = candSpells.m_pArray[i];
            menuList.SetSize(nBase + i + 1);

            CSWGuiInterfaceAction& entry = menuList.m_pArray[nBase + i];
            entry.m_sName = pApp->GetGUIString(pSpell->m_nNameStrRef);
            entry.m_nActionId = candIds.m_pArray[i] | CSWGuiInterfaceAction::ACTION_ID_SPELL;
            entry.m_resRef = pSpell->m_iconResRef;
            entry.m_nFlags = entry.m_nFlags & ~ACTION_FLAG_REASON_MASK;

            bool bAffordable;
            if (nDiscountTier == -1) {
                int nCost = pSpell->GetAdjustedForcePoints(nAlignment, 1, nRoomForceRating, nStanceId, cCHAMod);
                if (nForcePoints < static_cast<unsigned int>(nCost)) {
                    entry.m_nFlags =
                        (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_FORCE_DEPLETED;
                    bAffordable = false;
                } else {
                    bAffordable = true;
                }
            } else {
                int nCost = pSpell->GetAdjustedForcePoints(nAlignment, 1, nRoomForceRating, nStanceId, cCHAMod);
                int nDiscount = (nDiscountPct * nCost) / 100;
                if (nForcePoints < static_cast<unsigned int>(nCost - nDiscount)) {
                    entry.m_nFlags =
                        (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_FORCE_DEPLETED;
                    bAffordable = false;
                } else if (nDiscount < nCurrentHitPoints) {
                    bAffordable = true;
                } else {
                    entry.m_nFlags =
                        (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_NOT_ENOUGH_HEALTH;
                    bAffordable = false;
                }
            }

            float fRange = GetMinSpellRange(pSpell->m_sRangeCode);
            if (fRangeSq <= fRange * fRange) {
                entry.m_nFlags =
                    (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_TARGET_TOO_CLOSE;
                bAffordable = false;
            }

            unsigned char bBit0;
            if ((pSpell->m_nForbidItemMask & nForceItemMask) == 0) {
                if (bAffordable) {
                    if ((pSpell->m_nRequireItemMask & ~nForceItemMask) != 0) {
                        entry.m_nFlags =
                            (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_MISSING_ITEM;
                        bBit0 = 0;
                    } else {
                        bBit0 = 1;
                    }
                } else {
                    bBit0 = 0;
                }
            } else {
                entry.m_nFlags =
                    (entry.m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_RESTRICTED_BY_ARMOR;
                bBit0 = 0;
            }
            entry.m_nFlags = (entry.m_nFlags & ~ACTION_FLAG_ENABLED) | bBit0;
        }
    }
}
#endif // KOTOR1

int CSWCCreatureStats_ClassInfo::GetMenuInfoCount(unsigned int nType)
{
    if (g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->m_nPlayerRestrictMode)
        return 0;

    int nCount = 0;
    for (int i = 0; i < m_spellLevels[0].m_count; i++) {
        int nSpellId = m_spellLevels[0].m_pArray[i];
        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
        if (pSpell && pSpell->m_nSubRadialCount != 0) {
            unsigned char nNumSub = pSpell->m_nSubRadialCount;
            for (int j = 0; j < nNumSub; j++) {
                if (static_cast<short>(pSpell->GetSubRadialSpell(static_cast<unsigned char>(j))) / 1000 == static_cast<int>(nType))
                    nCount++;
            }
        }
    }
    return nCount;
}

CSWCStats_Spell::CSWCStats_Spell()
{
    m_nCasterLevel = 0;
    m_pReserved1 = NULL;
    m_nReady = 1;
}
