#include "nwrules.h"
#include "exostring.h"
#include "exolocstringinternal.h"
#include "nwspell.h"
#include "nwitem.h"
#include "nwclass.h"
#include "nwrace.h"
#include "nwskill.h"
#include "CExoArrayListDtors.h"
#include "CExoArrayListImpl.h"
#include "resgff.h"
#include "nwsrules.h"
#include "gameobjectarray.h"

#include "port/crtapi.h"
#include "nwscriptconst.h"

CSWRules::~CSWRules()
{
    if (m_p2DArrays) {
        m_p2DArrays->UnLoad2DArrays();
        delete m_p2DArrays;
        m_p2DArrays = 0;
    }
    if (m_pBaseItemArray) {
        delete m_pBaseItemArray;
        m_pBaseItemArray = 0;
    }
    if (m_pSpellArray) {
        delete m_pSpellArray;
        m_pSpellArray = 0;
    }
    if (m_paFeats) {
        delete[] m_paFeats;
        m_paFeats = 0;
    }
    if (m_paFeatIndices) {
        delete[] m_paFeatIndices;
        m_paFeatIndices = 0;
    }
    if (m_paClasses) {
        delete[] m_paClasses;
        m_paClasses = 0;
    }
    if (m_paRaces) {
        delete[] m_paRaces;
        m_paRaces = 0;
    }
    if (m_paSkills) {
        delete[] m_paSkills;
        m_paSkills = 0;
    }
    if (m_paMasterFeatNameStrRefs) {
        delete[] m_paMasterFeatNameStrRefs;
        m_paMasterFeatNameStrRefs = 0;
    }
    if (m_paMasterFeatDescriptionStrRefs) {
        delete[] m_paMasterFeatDescriptionStrRefs;
        m_paMasterFeatDescriptionStrRefs = 0;
    }
    if (m_paMasterFeatIcons) {
        delete[] m_paMasterFeatIcons;
        m_paMasterFeatIcons = 0;
    }
}

CSWRules::CSWRules()
    : m_pBaseItemArray(0)
    , m_aExpTable()
{
    m_paSkills = 0;
    m_paRaces = 0;
    m_paClasses = 0;
    m_nNumSkills = 0;
    m_nNumRaces = 0;
    m_nNumClasses = 0;
    m_nFeats = 0;
    m_paFeatIndices = 0;
    m_paMasterFeatIcons = 0;
    m_paMasterFeatDescriptionStrRefs = 0;
    m_paMasterFeatNameStrRefs = 0;
    m_paFeats = 0;
    m_pSpellArray = 0;

    // m_aExpTable is value-initialized in the initializer list above; its zero run merges with
    // m_pBaseItemArray's.

    // For the spell ranges, rows 0..4 of ranges.2da fill the first five, row 19 the sixth (the
    // wizard range).
    C2DA* pRanges = new C2DA(CResRef("Ranges"), 0);
    pRanges->Load2DArray();

    int i;
    float fRange;
    m_fSpellRange[0] = 0.0f;
    for (i = 0; i != 5; i++, m_fSpellRange[i] = 0.0f)
    {
        if (pRanges->GetFLOATEntry(i, CExoString("PrimaryRange"), &fRange))
            m_fSpellRange[i] = fRange;

        m_fMinSpellRange[i] = 0.0f;
        if (pRanges->GetFLOATEntry(i, CExoString("SecondaryRange"), &fRange))
            m_fMinSpellRange[i] = fRange;
    }

    {
        if (pRanges->GetFLOATEntry(19, CExoString("PrimaryRange"), &fRange))
            m_fSpellRange[i] = fRange;

        m_fMinSpellRange[i] = 0.0f;
        if (pRanges->GetFLOATEntry(19, CExoString("SecondaryRange"), &fRange))
            m_fMinSpellRange[i] = fRange;
    }

    delete pRanges;

    // Every other gameplay 2DA table, then the tables built from those.
    m_p2DArrays = new CTwoDimArrays();
    m_p2DArrays->Load2DArrays();

    m_pSpellArray = new CSWSpellArray();
    m_pSpellArray->Load();

    LoadFeatInfo();

    m_pBaseItemArray = new CSWBaseItemArray();
    m_pBaseItemArray->Load();

    // The XP table.
    C2DA* pExpTable = new C2DA(CResRef("EXPTABLE"), 0);
    pExpTable->Load2DArray();

    int nXP;
#if KOTOR1
    for (i = 0; i < 21; i++)
#else
    for (i = 0; i < 51; i++)
#endif
    {
        pExpTable->GetINTEntry(i, CExoString("XP"), &nXP);
        m_aExpTable[i] = nXP;
    }

    delete pExpTable;

    // The record-array readers, in dependency order.
    LoadSkillInfo();
    LoadClassInfo();
    LoadRaceInfo();
    LoadDifficultyInfo();
}

int CSWRules::GetDifficultyOption(int nOption, int nDifficulty)
{
    return m_difficultyOptions[nDifficulty][nOption];
}

CResRef CSWRules::GetMasterFeatIcon(char nMasterFeat)
{
    return m_paMasterFeatIcons[static_cast<int>(nMasterFeat)];
}

// The two master-feat text getters share one body, differing only in which strref array they
// index; both return a copy of the fetched string.
CExoString CSWRules::GetMasterFeatNameText(char nMasterFeat)
{
    STR_RES res;
    g_pTlkTable->Fetch(m_paMasterFeatNameStrRefs[static_cast<int>(nMasterFeat)], res, CTlkTable::GENDER_MALE);
    return res.text;
}

CExoString CSWRules::GetMasterFeatDescriptionText(char nMasterFeat)
{
    STR_RES res;
    g_pTlkTable->Fetch(m_paMasterFeatDescriptionStrRefs[static_cast<int>(nMasterFeat)], res, CTlkTable::GENDER_MALE);
    return res.text;
}

CSWFeat* CSWRules::GetFeat(unsigned short nFeat)
{
    if (m_nFeats <= nFeat)
        return 0;

    CSWFeat* pFeat = &m_paFeats[nFeat];
    return (pFeat->m_nFlags & CSWFeat::FLAG_PRESENT) ? pFeat : 0;
}

int CSWRules::IsFeatUseable(unsigned char nWisdom, unsigned short nFeat)
{
    // Unlike GetFeat, there is no bounds check here, only the present-bit test.
#ifdef K2_FIX_ENGINE_BUGS
    if (m_nFeats <= nFeat)
        return 0;
#endif
    CSWFeat* pFeat = &m_paFeats[nFeat];
    if (!(pFeat->m_nFlags & CSWFeat::FLAG_PRESENT))
        pFeat = 0;

    // sic: the pointer is nulled above and then dereferenced anyway, so a feat row absent from
    // feat.2da (present-bit clear) reads through a null pointer.
#ifdef K2_FIX_ENGINE_BUGS
    if (pFeat == 0)
        return 0;
#endif
    return pFeat->m_nMinWIS <= nWisdom;
}

unsigned short CSWRules::RollDice(unsigned char nDice, unsigned char nSides)
{
    int nTotal = 0;
    for (int i = 0; i < nDice; i++)
        nTotal += rand_wincompatible() % nSides + 1;

    return static_cast<unsigned short>(nTotal);
}

// Weights an already-rolled damage amount by a percentage, in one of two mutually exclusive modes:
// - mode A scales the damage up toward the cap: it adds a random slice between nPercent and 100
//   percent of nMaxDamage, then clamps to nMaxDamage, and returns the cap outright when nPercent
//   already scales to it.
// - mode B treats nPercent as a difficulty scalar around a neutral 100: below it the damage loses
//   the shortfall and floors at 1, above it a random bonus is added, and at 100 it passes through.
// The flag parameters are placeholders; the one caller that can be read never passes 1 for the
// first.
int CSWRules::GetWeightedDamageAmount(int nDamage, int nMaxDamage, int nFlagA, int nFlagB, int nPercent)
{
    if (nFlagA == 1) {
        if (nPercent == 0)
            return nDamage;

        if (nPercent * nMaxDamage / 100 == nMaxDamage)
            return nMaxDamage;

        int nWeighted = (rand_wincompatible() % (100 - nPercent) + nPercent) * nMaxDamage / 100 + nDamage;
        return nWeighted < nMaxDamage ? nWeighted : nMaxDamage;
    }

    if (nFlagB == 1) {
        if (nPercent == 0)
            return nDamage;

        if (nPercent <= 99) {
            int nWeighted = nDamage - (100 - nPercent) * nMaxDamage / 100;
            return nWeighted > 1 ? nWeighted : 1;
        }

        nPercent -= 100;
        if (nPercent == 0)
            return nDamage;

        return rand_wincompatible() % nPercent * nMaxDamage / 100 + nDamage;
    }

    return nDamage;
}

int CSWRules::GetMetaMagicLevelCost(unsigned char nMetaMagic)
{
    // A single-bit metamagic flag, so the cases are the powers of two from 1 to 32, and any other
    // value costs nothing.
    switch (nMetaMagic) {
    case 1:  return 2;
    case 2:  return 1;
    case 4:  return 3;
    case 8:  return 4;
    case 16: return 1;
    case 32: return 1;
    }

    return 0;
}

void CSWRules::LoadDifficultyInfo()
{
    C2DA* p2DA = new C2DA(CResRef("DiffSettings"), 0);
    p2DA->Load2DArray();

    // The 2DA is transposed. Columns 1..5 are the difficulty levels and rows 0..5 the options,
    // hence the [difficulty][option] indexing.
    for (int nDifficulty = 0; nDifficulty < 5; nDifficulty++) {
        for (int nOption = 0; nOption < 6; nOption++) {
            // sic: a one-byte local handed to an int out-parameter, so GetINTEntry writes three
            // bytes past its end.
#ifdef K2_FIX_ENGINE_BUGS
            // Engine bug (stack overwrite): give the `int*` write its four bytes. On a host -O0
            // build the three stray bytes land on the loop counters and the loop never ends.
            unsigned int nValueSlot;
            unsigned char& nValue = *reinterpret_cast<unsigned char*>(&nValueSlot);
#else
            unsigned char nValue;
#endif
            m_difficultyOptions[nDifficulty][nOption] =
                p2DA->GetINTEntry(nOption, nDifficulty + 1, reinterpret_cast<int*>(&nValue)) ? nValue : 0;
        }
    }

    delete p2DA;
}

// Loads skill.2da into m_paSkills. The "KeyAbility" column names an ability, which is mapped onto
// an index by compare; the flag columns each pack a bit into the flags byte.
void CSWRules::LoadSkillInfo()
{
    C2DA* p2DA = new C2DA(CResRef("Skills"), 0);
    p2DA->Load2DArray();

    m_nNumSkills = static_cast<unsigned char>(p2DA->m_nRows);
    m_paSkills = new CSWSkill[m_nNumSkills];

    for (int i = 0; i < m_nNumSkills; i++) {
        CExoString sValue;
        int nValue;

        p2DA->GetINTEntry(i, CExoString("Name"), &nValue);
        m_paSkills[i].m_nNameStrRef = nValue;
        p2DA->GetINTEntry(i, CExoString("Description"), &nValue);
        m_paSkills[i].m_nDescriptionStrRef = nValue;

        p2DA->GetCExoStringEntry(i, CExoString("Icon"), &sValue);
        m_paSkills[i].m_icon = sValue;

        p2DA->GetINTEntry(i, CExoString("Untrained"), &nValue);
        m_paSkills[i].m_flags = ((nValue & 1) << 1) | (m_paSkills[i].m_flags & ~CSWSkill::FLAG_UNTRAINED);

        // An unknown "KeyAbility" leaves the default 0 (strength).
        p2DA->GetCExoStringEntry(i, CExoString("KeyAbility"), &sValue);
        m_paSkills[i].m_nKeyAbility = ABILITY_STRENGTH;
        if (sValue == "STR")
            m_paSkills[i].m_nKeyAbility = ABILITY_STRENGTH;
        else if (sValue == "DEX")
            m_paSkills[i].m_nKeyAbility = ABILITY_DEXTERITY;
        else if (sValue == "CON")
            m_paSkills[i].m_nKeyAbility = ABILITY_CONSTITUTION;
        else if (sValue == "INT")
            m_paSkills[i].m_nKeyAbility = ABILITY_INTELLIGENCE;
        else if (sValue == "WIS")
            m_paSkills[i].m_nKeyAbility = ABILITY_WISDOM;
        else if (sValue == "CHA")
            m_paSkills[i].m_nKeyAbility = ABILITY_CHARISMA;

        p2DA->GetINTEntry(i, CExoString("ArmorCheckPenalty"), &nValue);
        m_paSkills[i].m_flags = ((nValue & 1) << 2) | (m_paSkills[i].m_flags & ~CSWSkill::FLAG_ARMOR_CHECK_PENALTY);
        p2DA->GetINTEntry(i, CExoString("AllClassesCanUse"), &nValue);
        m_paSkills[i].m_flags = ((nValue & 1) << 3) | (m_paSkills[i].m_flags & ~CSWSkill::FLAG_ALL_CLASSES_CAN_USE);

        p2DA->GetINTEntry(i, CExoString("Category"), &nValue);
        m_paSkills[i].m_nCategory = nValue;
        p2DA->GetINTEntry(i, CExoString("MaxCR"), &nValue);
        m_paSkills[i].m_nMaxCR = nValue;

        p2DA->GetINTEntry(i, CExoString("HostileSkill"), &nValue);
        m_paSkills[i].m_flags = (m_paSkills[i].m_flags & ~CSWSkill::FLAG_HOSTILE) | (nValue & 1);
        p2DA->GetINTEntry(i, CExoString("NPCCanUse"), &nValue);
        m_paSkills[i].m_flags = ((nValue & 1) << 4) | (m_paSkills[i].m_flags & ~CSWSkill::FLAG_NPC_CAN_USE);
        p2DA->GetINTEntry(i, CExoString("DroidCanUse"), &nValue);
        m_paSkills[i].m_flags = ((nValue & 1) << 5) | (m_paSkills[i].m_flags & ~CSWSkill::FLAG_DROID_CAN_USE);
    }

    delete p2DA;
}

// Loads racialtypes.2da into m_paRaces. The "FeatsTable" column names a second 2DA, read by the
// row's own CSWRace::LoadFeatsTable.
void CSWRules::LoadRaceInfo()
{
    C2DA* p2DA = new C2DA(CResRef("RacialTypes"), 0);
    p2DA->Load2DArray();

    m_nNumRaces = static_cast<unsigned char>(p2DA->m_nRows);
    m_paRaces = new CSWRace[m_nNumRaces];

    for (int i = 0; i < m_nNumRaces; i++) {
        CExoString sValue;
        int nValue;

        p2DA->GetINTEntry(i, CExoString("Name"), &nValue);
        m_paRaces[i].m_nNameStrRef = nValue;
        p2DA->GetINTEntry(i, CExoString("ConverName"), &nValue);
        m_paRaces[i].m_nConverNameStrRef = nValue;
        p2DA->GetINTEntry(i, CExoString("ConverNameLower"), &nValue);
        m_paRaces[i].m_nConverNameLowerStrRef = nValue;
        p2DA->GetINTEntry(i, CExoString("NamePlural"), &nValue);
        m_paRaces[i].m_nNamePluralStrRef = nValue;
        p2DA->GetINTEntry(i, CExoString("Description"), &nValue);
        m_paRaces[i].m_nDescriptionStrRef = nValue;
        if (p2DA->GetINTEntry(i, CExoString("Biography"), &nValue))
            m_paRaces[i].m_nDefaultBiographyStrRef = nValue;

        int nByte;
        if (p2DA->GetINTEntry(i, CExoString("Favored"), &nByte))
            m_paRaces[i].m_nFavoredClass = static_cast<unsigned char>(nByte);
        if (p2DA->GetINTEntry(i, CExoString("PlayerRace"), &nByte))
            m_paRaces[i].m_bPlayerRace = (nByte != 0);

        p2DA->GetINTEntry(i, CExoString("STRAdjust"), &nByte);
        m_paRaces[i].m_anAbilityAdjust[0] = static_cast<signed char>(nByte);
        p2DA->GetINTEntry(i, CExoString("DEXAdjust"), &nByte);
        m_paRaces[i].m_anAbilityAdjust[1] = static_cast<signed char>(nByte);
        p2DA->GetINTEntry(i, CExoString("INTAdjust"), &nByte);
        m_paRaces[i].m_anAbilityAdjust[2] = static_cast<signed char>(nByte);
        p2DA->GetINTEntry(i, CExoString("CHAAdjust"), &nByte);
        m_paRaces[i].m_anAbilityAdjust[3] = static_cast<signed char>(nByte);
        p2DA->GetINTEntry(i, CExoString("WISAdjust"), &nByte);
        m_paRaces[i].m_anAbilityAdjust[4] = static_cast<signed char>(nByte);
        p2DA->GetINTEntry(i, CExoString("CONAdjust"), &nByte);
        m_paRaces[i].m_anAbilityAdjust[5] = static_cast<signed char>(nByte);
        p2DA->GetINTEntry(i, CExoString("Endurance"), &nByte);
        m_paRaces[i].m_nEndurance = static_cast<unsigned char>(nByte);

        p2DA->GetINTEntry(i, CExoString("Age"), &nByte);
        m_paRaces[i].m_nAge = nByte;
        p2DA->GetINTEntry(i, CExoString("Appearance"), &nByte);
        m_paRaces[i].m_nAppearance = static_cast<unsigned short>(nByte);

        if (p2DA->GetCExoStringEntry(i, CExoString("FeatsTable"), &sValue))
            m_paRaces[i].LoadFeatsTable(sValue);
    }

    delete p2DA;
}

// Loads classes.2da into m_paClasses. Eight "...Table" columns each name a second 2DA, read by a
// CSWClass member; the spells table is loaded unconditionally.
void CSWRules::LoadClassInfo()
{
    C2DA* p2DA = new C2DA(CResRef("CLASSES"), 0);
    p2DA->Load2DArray();

    m_nNumClasses = static_cast<unsigned char>(p2DA->m_nRows);
    m_paClasses = new CSWClass[m_nNumClasses];

    for (int i = 0; i < m_nNumClasses; i++) {
        CExoString sValue;
        int nValue;
        int nHitDie;
        int nSkillPointBase;
        int nField;

        p2DA->GetINTEntry(i, CExoString("Name"), &nValue);
        m_paClasses[i].m_nNameStrRef = nValue;
        p2DA->GetINTEntry(i, CExoString("SkillPointBase"), &nSkillPointBase);
        m_paClasses[i].m_nSkillPointsBase = static_cast<unsigned char>(nSkillPointBase);
        p2DA->GetINTEntry(i, CExoString("Lower"), &nValue);
        m_paClasses[i].m_nNameLowerStrRef = nValue;
        p2DA->GetINTEntry(i, CExoString("Plural"), &nValue);
        m_paClasses[i].m_nNamePluralStrRef = nValue;
        p2DA->GetINTEntry(i, CExoString("Description"), &nValue);
        m_paClasses[i].m_nDescriptionStrRef = nValue;

        p2DA->GetCExoStringEntry(i, CExoString("Icon"), &sValue);
        m_paClasses[i].m_sIcon = sValue;

        p2DA->GetINTEntry(i, CExoString("HitDie"), &nHitDie);
        m_paClasses[i].m_nHitDie = static_cast<unsigned char>(nHitDie);
        p2DA->GetINTEntry(i, CExoString("ForceDie"), &nHitDie);
        m_paClasses[i].m_nForcePointGain = static_cast<unsigned char>(nHitDie);

        if (p2DA->GetCExoStringEntry(i, CExoString("AttackBonusTable"), &sValue))
            m_paClasses[i].LoadAttackBonusTable(sValue);
        if (p2DA->GetCExoStringEntry(i, CExoString("FeatsTable"), &sValue))
            m_paClasses[i].LoadFeatsTable(sValue, this);
        if (p2DA->GetCExoStringEntry(i, CExoString("SavingThrowTable"), &sValue))
            m_paClasses[i].LoadSavingThrowTable(sValue);
        if (p2DA->GetCExoStringEntry(i, CExoString("SkillsTable"), &sValue))
            m_paClasses[i].LoadSkillsTable(sValue);
        if (p2DA->GetCExoStringEntry(i, CExoString("SpellGainTable"), &sValue))
            m_paClasses[i].LoadSpellGainTable(sValue);
        if (p2DA->GetCExoStringEntry(i, CExoString("SpellKnownTable"), &sValue))
            m_paClasses[i].LoadSpellKnownTable(sValue);
        if (p2DA->GetCExoStringEntry(i, CExoString("FeatGain"), &sValue))
            m_paClasses[i].LoadFeatGain(sValue);
        if (p2DA->GetCExoStringEntry(i, CExoString("ArmorClassColumn"), &sValue))
            m_paClasses[i].LoadArmorClassTable(sValue);

        m_paClasses[i].LoadSpellsTable();

        if (p2DA->GetINTEntry(i, CExoString("PlayerClass"), &nField))
            m_paClasses[i].m_bPlayerClass = (nField != 0);
        if (p2DA->GetINTEntry(i, CExoString("SpellCaster"), &nField))
            m_paClasses[i].m_bSpellCaster = (nField != 0);
        if (p2DA->GetINTEntry(i, CExoString("AlignRestrict"), &nField))
            m_paClasses[i].m_alignRestrict = static_cast<unsigned char>(nField);
        if (p2DA->GetINTEntry(i, CExoString("AlignRstrctType"), &nField))
            m_paClasses[i].m_nAlignRestrictType = static_cast<unsigned char>(nField);

        if (p2DA->GetCExoStringEntry(i, CExoString("PrimaryAbil"), &sValue)) {
            if (sValue == "STR")
                m_paClasses[i].m_nPrimaryAbility = ABILITY_STRENGTH;
            else if (sValue == "DEX")
                m_paClasses[i].m_nPrimaryAbility = ABILITY_DEXTERITY;
            else if (sValue == "CON")
                m_paClasses[i].m_nPrimaryAbility = ABILITY_CONSTITUTION;
            else if (sValue == "INT")
                m_paClasses[i].m_nPrimaryAbility = ABILITY_INTELLIGENCE;
            else if (sValue == "WIS")
                m_paClasses[i].m_nPrimaryAbility = ABILITY_WISDOM;
            else if (sValue == "CHA")
                m_paClasses[i].m_nPrimaryAbility = ABILITY_CHARISMA;
        }

        p2DA->GetINTEntry(i, CExoString("Str"), &nField);
        m_paClasses[i].m_abRecommendedScores[0] = static_cast<unsigned char>(nField);
        p2DA->GetINTEntry(i, CExoString("Dex"), &nField);
        m_paClasses[i].m_abRecommendedScores[1] = static_cast<unsigned char>(nField);
        p2DA->GetINTEntry(i, CExoString("Con"), &nField);
        m_paClasses[i].m_abRecommendedScores[2] = static_cast<unsigned char>(nField);
        p2DA->GetINTEntry(i, CExoString("Int"), &nField);
        m_paClasses[i].m_abRecommendedScores[3] = static_cast<unsigned char>(nField);
        p2DA->GetINTEntry(i, CExoString("Wis"), &nField);
        m_paClasses[i].m_abRecommendedScores[4] = static_cast<unsigned char>(nField);
        p2DA->GetINTEntry(i, CExoString("Cha"), &nField);
        m_paClasses[i].m_abRecommendedScores[5] = static_cast<unsigned char>(nField);

        // Challenge ratings are per level and start at 1 (level 0 is never a CR). K2 has no CR
        // columns and fills every level from 1 to 50 with 1.
#if KOTOR1  // K1 reads levels 1..20 from the EffCRLvl%02d columns
        {
            CExoString sColumn;
            for (int nLevel = 0; nLevel < 20; nLevel++) {
                sColumn.Format(const_cast<char*>("EffCRLvl%02d"), nLevel + 1);
                p2DA->GetINTEntry(i, CExoString(sColumn.CStr()), &nField);
                m_paClasses[i].m_anEffCRByLevel[nLevel + 1] = static_cast<unsigned char>(nField);
            }
        }
#else
        {
            CExoString sUnused;
            for (int nLevel = 1; nLevel != 51; nLevel++) {
                nField = 1;
                m_paClasses[i].m_anEffCRByLevel[nLevel] = static_cast<unsigned char>(nField);
            }
        }
#endif
    }

    delete p2DA;
}

// Loads feat.2da into m_paFeats and records the index of every row it accepted in m_paFeatIndices,
// then loads MasterFeats.2da into the master-feat name, description and icon arrays.
//
// A row is accepted when its first column reads back; a rejected row keeps whatever
// CSWFeat::CSWFeat seeded, with only its present bit cleared.
// EFFECTIVE: length-exact, landing pads exact; register tie-breaks in the prereq-flag bitfield stores.
void CSWRules::LoadFeatInfo()
{
    C2DA* p2DA = new C2DA(CResRef("Feat"), 0);
    p2DA->Load2DArray();

    m_nFeats = static_cast<unsigned short>(p2DA->m_nRows);
    m_paFeats = new CSWFeat[m_nFeats];
    m_paFeatIndices = new short[m_nFeats];
    m_nFeatIndices = 0;

    for (int i = 0; i < m_nFeats; i++) {
        int nValue;
        int nField;
        CExoString sValue;
        CExoString sColumn("Name");
        if (p2DA->GetColumnIndex(sColumn) == -1)
            sColumn = "Feat";

        if (!p2DA->GetINTEntry(i, sColumn, &nValue)) {
            m_paFeats[i].m_nFlags &= ~CSWFeat::FLAG_PRESENT;
        } else {
            m_paFeatIndices[m_nFeatIndices] = static_cast<short>(i);
            ++m_nFeatIndices;
            m_paFeats[i].m_nNameStrRef = nValue;

            p2DA->GetINTEntry(i, CExoString("Description"), &nValue);
            m_paFeats[i].m_nDescriptionStrRef = nValue;

            if (p2DA->GetCExoStringEntry(i, CExoString("Icon"), &sValue))
                m_paFeats[i].SetIcon(CResRef(sValue.CStr()));

            p2DA->GetINTEntry(i, CExoString("Category"), &nField);
            m_paFeats[i].m_nCategory = nField;
            p2DA->GetINTEntry(i, CExoString("MaxCR"), &nField);
            m_paFeats[i].m_nMaxCR = nField;
            p2DA->GetINTEntry(i, CExoString("MinAttackBonus"), &nField);
            m_paFeats[i].m_nMinAttackBonus = static_cast<unsigned char>(nField);
            p2DA->GetINTEntry(i, CExoString("MinCharLevel"), &nField);
            m_paFeats[i].m_nMinCharLevel = static_cast<unsigned char>(nField);
            p2DA->GetINTEntry(i, CExoString("MinSTR"), &nField);
            m_paFeats[i].m_nMinSTR = static_cast<unsigned char>(nField);
            p2DA->GetINTEntry(i, CExoString("MinDEX"), &nField);
            m_paFeats[i].m_nMinDEX = static_cast<unsigned char>(nField);
            p2DA->GetINTEntry(i, CExoString("MinINT"), &nField);
            m_paFeats[i].m_nMinINT = static_cast<unsigned char>(nField);
            p2DA->GetINTEntry(i, CExoString("MinWIS"), &nField);
            m_paFeats[i].m_nMinWIS = static_cast<unsigned char>(nField);

            if (p2DA->GetINTEntry(i, CExoString("PrereqFeat1"), &nField))
                m_paFeats[i].m_nPrereqFeat1 = static_cast<unsigned short>(nField);
            if (p2DA->GetINTEntry(i, CExoString("PrereqFeat2"), &nField))
                m_paFeats[i].m_nPrereqFeat2 = static_cast<unsigned short>(nField);

            CExoString sOrReqFeat;
            for (int j = 0; j < 5; j++) {
                sOrReqFeat = "OrReqFeat";
                sOrReqFeat = sOrReqFeat + CExoString(j);
                if (p2DA->GetINTEntry(i, sOrReqFeat, &nField))
                    m_paFeats[i].m_anOrReqFeat[j] = static_cast<unsigned short>(nField);
            }

            if (p2DA->GetINTEntry(i, CExoString("REQSKILL"), &nField))
                m_paFeats[i].m_nReqSkill = static_cast<short>(nField);
            if (p2DA->GetINTEntry(i, CExoString("SUCCESSOR"), &nField))
                m_paFeats[i].m_nSuccessor = nField;
#if !KOTOR1
            if (p2DA->GetINTEntry(i, CExoString("HANDMAIDEN"), &nField))
                m_paFeats[i].m_nHandmaidenGrantLevel = static_cast<unsigned char>(nField);
            if (p2DA->GetINTEntry(i, CExoString("BAODUR"), &nField))
                m_paFeats[i].m_nBaodurGrantLevel = static_cast<unsigned char>(nField);
            if (p2DA->GetINTEntry(i, CExoString("HANHARR"), &nField))
                m_paFeats[i].m_nHanharrGrantLevel = static_cast<unsigned char>(nField);
            if (p2DA->GetINTEntry(i, CExoString("HK47"), &nField))
                m_paFeats[i].m_nHK47GrantLevel = static_cast<unsigned char>(nField);
            if (p2DA->GetINTEntry(i, CExoString("G0T0"), &nField))
                m_paFeats[i].m_nG0T0GrantLevel = static_cast<unsigned char>(nField);
            if (p2DA->GetINTEntry(i, CExoString("ATTON"), &nField))
                m_paFeats[i].m_nAttonGrantLevel = static_cast<unsigned char>(nField);
            if (p2DA->GetINTEntry(i, CExoString("KREIA"), &nField))
                m_paFeats[i].m_nKreiaGrantLevel = static_cast<unsigned char>(nField);
#endif

            p2DA->GetINTEntry(i, CExoString("GainMultiple"), &nField);
            m_paFeats[i].m_nFlags = (nField == 1) | (m_paFeats[i].m_nFlags & ~CSWFeat::FLAG_GAIN_MULTIPLE);

            p2DA->GetINTEntry(i, CExoString("MinSpellLvl"), &nField);
            m_paFeats[i].m_nMinSpellLvl = 0;

            p2DA->GetINTEntry(i, CExoString("AllClassesCanUse"), &nField);
            m_paFeats[i].m_nFlags = ((nField & 1) << 1) | (m_paFeats[i].m_nFlags & ~CSWFeat::FLAG_ALL_CLASSES_CAN_USE);

            if (p2DA->GetINTEntry(i, CExoString("SpellId"), &nField))
                m_paFeats[i].m_nSpellId = nField;
            if (p2DA->GetINTEntry(i, CExoString("UsesPerDay"), &nField))
                m_paFeats[i].m_maxUses = static_cast<unsigned char>(nField);

            // The master-feat column is signed, with -1 for a feat that belongs to none.
            if (p2DA->GetINTEntry(i, CExoString("MASTERFEAT"), &nField))
                m_paFeats[i].m_nMasterFeat = static_cast<char>(nField);
            else
                m_paFeats[i].m_nMasterFeat = -1;

            if (p2DA->GetINTEntry(i, CExoString("TargetSelf"), &nField))
                m_paFeats[i].m_nFlags = ((nField & 1) << 2) | (m_paFeats[i].m_nFlags & ~CSWFeat::FLAG_TARGET_SELF);

            p2DA->GetINTEntry(i, CExoString("HostileFeat"), &nField);
            m_paFeats[i].m_nFlags = ((nField & 1) << 3) | (m_paFeats[i].m_nFlags & ~CSWFeat::FLAG_HOSTILE);
        }
    }

    delete p2DA;

    C2DA* pMasterFeats = new C2DA(CResRef("MasterFeats"), 0);
    pMasterFeats->Load2DArray();

    char nMasterFeatCount = static_cast<char>(pMasterFeats->m_nRows);
    m_nNumMasterFeats = nMasterFeatCount;
    m_paMasterFeatNameStrRefs = new int[nMasterFeatCount];
    m_paMasterFeatDescriptionStrRefs = new int[nMasterFeatCount];
    m_paMasterFeatIcons = new CResRef[nMasterFeatCount];

    CExoString sIcon;
    if (m_nNumMasterFeats > 0) {
        for (int i = 0; i < m_nNumMasterFeats; i++) {
            int nMasterValue;
            pMasterFeats->GetINTEntry(i, CExoString("STRREF"), &nMasterValue);
            m_paMasterFeatNameStrRefs[i] = nMasterValue;
            pMasterFeats->GetINTEntry(i, CExoString("DESCRIPTION"), &nMasterValue);
            m_paMasterFeatDescriptionStrRefs[i] = nMasterValue;

            // sic: the icon read's result is ignored.
            pMasterFeats->GetCExoStringEntry(i, CExoString("ICON"), &sIcon);
            m_paMasterFeatIcons[i] = sIcon.CStr();
        }
    }

    delete pMasterFeats;
}

// The per-creature computed combat summary. Both node lists are CExoArrayList members used
// directly, so their contents are only freed by the destructors below.

// The race requirement comes from the rules singleton's race count.
CCombatInformationNode::CCombatInformationNode()
{
    m_cValue = 0;
    m_nModifierType = 0;
    m_nType = 0;
    m_nFlag = 0;
    m_nClassReq = g_pRules->m_nNumRaces;
}

// Compares all five fields.
int CCombatInformationNode::operator==(CCombatInformationNode& other)
{
    if (m_cValue == other.m_cValue
        && m_nModifierType == other.m_nModifierType
        && m_nType == other.m_nType
        && m_nFlag == other.m_nFlag
        && m_nClassReq == other.m_nClassReq)
        return 1;
    return 0;
}

int CCombatInformationNode::operator!=(CCombatInformationNode& other)
{
    if (m_cValue == other.m_cValue
        && m_nModifierType == other.m_nModifierType
        && m_nType == other.m_nType
        && m_nFlag == other.m_nFlag
        && m_nClassReq == other.m_nClassReq)
        return 0;
    return 1;
}

CCombatInformationNode& CCombatInformationNode::operator=(const CCombatInformationNode& other)
{
    if (this == &other)
        return *this;

    m_cValue = other.m_cValue;
    m_nModifierType = other.m_nModifierType;
    m_nType = other.m_nType;
    m_nFlag = other.m_nFlag;
    m_nClassReq = other.m_nClassReq;
    return *this;
}

// Sets the unarmed defaults. Both crit lines start at 20/x2, the damage defaults to 1d4, and both
// weapon slots hold the invalid-object-id sentinel with empty display names. The off-hand name is
// assigned first.
CCombatInformation::CCombatInformation()
{
    __builtin_memset(&m_nNumOnHandAttacks, 0, sizeof(long long));
    m_nDefaultDamageDie = 1;
#if KOTOR1
    m_nDefaultDamageDiceCount = 3;
#else
    m_nDefaultDamageDiceCount = 4;
#endif
    m_nCritRangeOnHand = 0;
    m_nCritMultiplierOnHand = 2;
    m_bOffHandWeaponEquipped = 0;
    m_nCritRangeOffHand = 0;
    m_nCritMultiplierOffHand = 2;
    m_onHandWeaponId = OBJECT_INVALID;
    m_offHandWeaponId = OBJECT_INVALID;
    m_offHandWeaponName = "";
    m_onHandWeaponName = "";
    m_nOverrideDie = 0;
    m_nOverrideNumDice = 0;
}

// Clears the unarmed defaults, then frees both node lists. The lists do not own their elements,
// so this is the only place the nodes are freed.
CCombatInformation::~CCombatInformation()
{
    __builtin_memset(&m_nNumOnHandAttacks, 0, sizeof(long long));
    m_nDefaultDamageDie = 1;
#if KOTOR1
    m_nDefaultDamageDiceCount = 3;
#else
    m_nDefaultDamageDiceCount = 4;
#endif
    m_nCritRangeOnHand = 0;
    m_nCritMultiplierOnHand = 0;
    m_bOffHandWeaponEquipped = 0;
    m_nCritRangeOffHand = 0;
    m_nCritMultiplierOffHand = 2;

    CExoArrayList<CCombatInformationNode*>& aAttackMods = m_attackBonusMods;
    while (aAttackMods.m_count > 0)
    {
        CCombatInformationNode* pNode = aAttackMods.m_pArray[0];
        aAttackMods.DelIndex(0);
        delete pNode;
    }
    CExoArrayList<CCombatInformationNode*>& aDamageMods = m_damageBonusMods;
    while (aDamageMods.m_count > 0)
    {
        CCombatInformationNode* pNode = aDamageMods.m_pArray[0];
        aDamageMods.DelIndex(0);
        delete pNode;
    }
}

// Writes the CombatInfo struct: the scalar fields in GFF-label order, then the two node lists.

// The GFF struct ids of the saved combat information and its attack and damage bonus lists.
enum {
    GFF_STRUCT_ID_COMBAT_INFO  = 0xcaaa,
    GFF_STRUCT_ID_ATTACK_BONUS = 0xaaee,
    GFF_STRUCT_ID_DAMAGE_BONUS = 0xddee,
};

// Several GFF labels are the engine's own names for the fields (m_nType is "WeaponWield",
// m_nFlag is "VersusGoodEvil", m_nClassReq is "VersusRace"), and "UnarmedDamDice" carries
// m_nDefaultDamageDie while "UnarmedDamDie" carries m_nDefaultDamageDiceCount.
int CCombatInformation::SaveData(CResGFF* pGFF, CResStruct* pStruct)
{
    CResStruct sub;
    pGFF->AddStructToStruct(&sub, pStruct, const_cast<char*>("CombatInfo"), GFF_STRUCT_ID_COMBAT_INFO);
    pGFF->WriteFieldBYTE(&sub, m_nNumOnHandAttacks, const_cast<char*>("NumAttacks"));
    pGFF->WriteFieldCHAR(&sub, m_nAttackModOnHand, const_cast<char*>("OnHandAttackMod"));
    pGFF->WriteFieldCHAR(&sub, m_nStrDamageModOnHand, const_cast<char*>("OnHandDamageMod"));
    pGFF->WriteFieldCHAR(&sub, m_nAttackModOffHand, const_cast<char*>("OffHandAttackMod"));
    pGFF->WriteFieldCHAR(&sub, m_nStrDamageModOffHand, const_cast<char*>("OffHandDamageMod"));
    pGFF->WriteFieldBYTE(&sub, m_nForceResistance, const_cast<char*>("ForceResistance"));
    pGFF->WriteFieldBYTE(&sub, m_nArcaneSpellFailure, const_cast<char*>("ArcaneSpellFail"));
    pGFF->WriteFieldBYTE(&sub, m_bArmorCheckPenalty, const_cast<char*>("ArmorCheckPen"));
    pGFF->WriteFieldBYTE(&sub, m_nDefaultDamageDie, const_cast<char*>("UnarmedDamDice"));
    pGFF->WriteFieldBYTE(&sub, m_nDefaultDamageDiceCount, const_cast<char*>("UnarmedDamDie"));
    pGFF->WriteFieldBYTE(&sub, m_nCritRangeOnHand, const_cast<char*>("OnHandCritRng"));
    pGFF->WriteFieldBYTE(&sub, m_nCritMultiplierOnHand, const_cast<char*>("OnHandCritMult"));
    pGFF->WriteFieldBYTE(&sub, static_cast<unsigned char>(m_bOffHandWeaponEquipped), const_cast<char*>("OffHandWeaponEq"));
    pGFF->WriteFieldBYTE(&sub, m_nCritRangeOffHand, const_cast<char*>("OffHandCritRng"));
    pGFF->WriteFieldBYTE(&sub, m_nCritMultiplierOffHand, const_cast<char*>("OffHandCritMult"));
    pGFF->WriteFieldDWORD(&sub, static_cast<unsigned int>(m_offHandWeaponId), const_cast<char*>("LeftEquip"));
    pGFF->WriteFieldDWORD(&sub, static_cast<unsigned int>(m_onHandWeaponId), const_cast<char*>("RightEquip"));
    pGFF->WriteFieldCExoString(&sub, m_offHandWeaponName, const_cast<char*>("LeftString"));
    pGFF->WriteFieldCExoString(&sub, m_onHandWeaponName, const_cast<char*>("RightString"));
    pGFF->WriteFieldBYTE(&sub, m_nOverrideDie, const_cast<char*>("DamageDice"));
    pGFF->WriteFieldBYTE(&sub, m_nOverrideNumDice, const_cast<char*>("DamageDie"));

    CResList list;
    pGFF->AddList(&list, &sub, const_cast<char*>("AttackList"));
    if (m_attackBonusMods.m_count >= 1)
    {
        for (int i = 0; i < m_attackBonusMods.m_count; i++)
        {
            CResStruct st;
            pGFF->AddListElement(&st, &list, GFF_STRUCT_ID_ATTACK_BONUS);
            pGFF->WriteFieldCHAR(&st, m_attackBonusMods.m_pArray[i]->m_cValue, const_cast<char*>("Modifier"));
            pGFF->WriteFieldBYTE(&st, m_attackBonusMods.m_pArray[i]->m_nType, const_cast<char*>("WeaponWield"));
            pGFF->WriteFieldBYTE(&st, m_attackBonusMods.m_pArray[i]->m_nFlag, const_cast<char*>("VersusGoodEvil"));
            pGFF->WriteFieldBYTE(&st, m_attackBonusMods.m_pArray[i]->m_nClassReq, const_cast<char*>("VersusRace"));
        }
    }

    pGFF->AddList(&list, &sub, const_cast<char*>("DamageList"));
    if (m_damageBonusMods.m_count >= 1)
    {
        for (int i = 0; i < m_damageBonusMods.m_count; i++)
        {
            CResStruct st;
            pGFF->AddListElement(&st, &list, GFF_STRUCT_ID_DAMAGE_BONUS);
            pGFF->WriteFieldCHAR(&st, m_damageBonusMods.m_pArray[i]->m_cValue, const_cast<char*>("Modifier"));
            pGFF->WriteFieldBYTE(&st, m_damageBonusMods.m_pArray[i]->m_nModifierType, const_cast<char*>("ModifierType"));
            pGFF->WriteFieldBYTE(&st, m_damageBonusMods.m_pArray[i]->m_nType, const_cast<char*>("WeaponWield"));
            pGFF->WriteFieldBYTE(&st, m_damageBonusMods.m_pArray[i]->m_nFlag, const_cast<char*>("VersusGoodEvil"));
            pGFF->WriteFieldBYTE(&st, m_damageBonusMods.m_pArray[i]->m_nClassReq, const_cast<char*>("VersusRace"));
        }
    }
    return 1;
}

// Reads the CombatInfo struct written by SaveData: the same labels in the same order, then the two
// node lists. Returns 0 when the sub-struct is absent.
//
// Each scalar read falls back to the member's current value, apart from the override bytes and the
// node fields, whose defaults are literals, and "VersusRace", which falls back to the race count.
int CCombatInformation::LoadData(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    CResStruct st;
    CResStruct sub;
    int bSuccess;
    if (!pGFF->GetStructFromStruct(&sub, pStruct, const_cast<char*>("CombatInfo")))
        return 0;

    m_nNumOnHandAttacks = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("NumAttacks"), bSuccess, m_nNumOnHandAttacks);
    m_nAttackModOnHand = pGFF->ReadFieldCHAR(&sub, const_cast<char*>("OnHandAttackMod"), bSuccess, m_nAttackModOnHand);
    m_nStrDamageModOnHand = pGFF->ReadFieldCHAR(&sub, const_cast<char*>("OnHandDamageMod"), bSuccess, m_nStrDamageModOnHand);
    m_nAttackModOffHand = pGFF->ReadFieldCHAR(&sub, const_cast<char*>("OffHandAttackMod"), bSuccess, m_nAttackModOffHand);
    m_nStrDamageModOffHand = pGFF->ReadFieldCHAR(&sub, const_cast<char*>("OffHandDamageMod"), bSuccess, m_nStrDamageModOffHand);
    m_nForceResistance = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("ForceResistance"), bSuccess, m_nForceResistance);
    // sic: the "ArcaneSpellFail" read's result is discarded (no store follows).
    pGFF->ReadFieldBYTE(&sub, const_cast<char*>("ArcaneSpellFail"), bSuccess, m_nArcaneSpellFailure);
    m_bArmorCheckPenalty = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("ArmorCheckPen"), bSuccess, m_bArmorCheckPenalty);
    m_nDefaultDamageDie = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("UnarmedDamDice"), bSuccess, m_nDefaultDamageDie);
    m_nDefaultDamageDiceCount = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("UnarmedDamDie"), bSuccess, m_nDefaultDamageDiceCount);
    m_nCritRangeOnHand = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("OnHandCritRng"), bSuccess, m_nCritRangeOnHand);
    m_nCritMultiplierOnHand = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("OnHandCritMult"), bSuccess, m_nCritMultiplierOnHand);
    m_bOffHandWeaponEquipped = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("OffHandWeaponEq"), bSuccess, static_cast<unsigned char>(m_bOffHandWeaponEquipped));
    m_nCritRangeOffHand = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("OffHandCritRng"), bSuccess, m_nCritRangeOffHand);
    m_nCritMultiplierOffHand = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("OffHandCritMult"), bSuccess, m_nCritMultiplierOffHand);
    m_offHandWeaponId = pGFF->ReadFieldDWORD(&sub, const_cast<char*>("LeftEquip"), bSuccess, static_cast<unsigned int>(m_offHandWeaponId));
    m_onHandWeaponId = pGFF->ReadFieldDWORD(&sub, const_cast<char*>("RightEquip"), bSuccess, static_cast<unsigned int>(m_onHandWeaponId));
    m_offHandWeaponName = pGFF->ReadFieldCExoString(&sub, const_cast<char*>("LeftString"), bSuccess, "");
    m_onHandWeaponName = pGFF->ReadFieldCExoString(&sub, const_cast<char*>("RightString"), bSuccess, "");
    m_nOverrideDie = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("DamageDice"), bSuccess, 0);
    m_nOverrideNumDice = pGFF->ReadFieldBYTE(&sub, const_cast<char*>("DamageDie"), bSuccess, 0);

    // The casts keep the loop bound signed, as this reader's loops are.
    if (pGFF->GetList(&list, pStruct, const_cast<char*>("AttackList")) == 1)
    {
        for (int i = 0; i < static_cast<int>(pGFF->GetListCount(&list)); i++)
        {
            pGFF->GetListElement(&st, &list, i);
            CCombatInformationNode* pNode = new CCombatInformationNode();
            pNode->m_cValue = pGFF->ReadFieldCHAR(&st, const_cast<char*>("Modifier"), bSuccess, 0);
            pNode->m_nType = pGFF->ReadFieldBYTE(&st, const_cast<char*>("WeaponWield"), bSuccess, 0);
            pNode->m_nFlag = pGFF->ReadFieldBYTE(&st, const_cast<char*>("VersusGoodEvil"), bSuccess, 0);
            pNode->m_nClassReq = pGFF->ReadFieldBYTE(&st, const_cast<char*>("VersusRace"), bSuccess,
                                                     g_pRules->m_nNumRaces);
            m_attackBonusMods.Add(pNode);
        }
    }

    if (pGFF->GetList(&list, pStruct, const_cast<char*>("DamageList")) == 1)
    {
        for (int i = 0; i < static_cast<int>(pGFF->GetListCount(&list)); i++)
        {
            pGFF->GetListElement(&st, &list, i);
            CCombatInformationNode* pNode = new CCombatInformationNode();
            pNode->m_cValue = pGFF->ReadFieldCHAR(&st, const_cast<char*>("Modifier"), bSuccess, 0);
            pNode->m_nModifierType = pGFF->ReadFieldBYTE(&st, const_cast<char*>("ModifierType"), bSuccess, 0);
            pNode->m_nType = pGFF->ReadFieldBYTE(&st, const_cast<char*>("WeaponWield"), bSuccess, 0);
            pNode->m_nFlag = pGFF->ReadFieldBYTE(&st, const_cast<char*>("VersusGoodEvil"), bSuccess, 0);
            pNode->m_nClassReq = pGFF->ReadFieldBYTE(&st, const_cast<char*>("VersusRace"), bSuccess,
                                                     g_pRules->m_nNumRaces);
            m_damageBonusMods.Add(pNode);
        }
    }
    return 1;
}

// Compares every field except the two weapon names and the override die and dice count.
int CCombatInformation::operator==(CCombatInformation& other)
{
    if (m_nNumOnHandAttacks != other.m_nNumOnHandAttacks
        || m_nAttackModOnHand != other.m_nAttackModOnHand
        || m_nStrDamageModOnHand != other.m_nStrDamageModOnHand
        || m_nAttackModOffHand != other.m_nAttackModOffHand
        || m_nStrDamageModOffHand != other.m_nStrDamageModOffHand
        || m_bOffHandWeaponEquipped != other.m_bOffHandWeaponEquipped
        || m_nForceResistance != other.m_nForceResistance
        || m_nArcaneSpellFailure != other.m_nArcaneSpellFailure
        || m_bArmorCheckPenalty != other.m_bArmorCheckPenalty
        || m_nDefaultDamageDie != other.m_nDefaultDamageDie
        || m_nDefaultDamageDiceCount != other.m_nDefaultDamageDiceCount
        || m_nCritRangeOnHand != other.m_nCritRangeOnHand
        || m_nCritMultiplierOnHand != other.m_nCritMultiplierOnHand
        || m_nCritRangeOffHand != other.m_nCritRangeOffHand
        || m_nCritMultiplierOffHand != other.m_nCritMultiplierOffHand
        || m_offHandWeaponId != other.m_offHandWeaponId
        || m_onHandWeaponId != other.m_onHandWeaponId
        || m_attackBonusMods.m_count != other.m_attackBonusMods.m_count
        || m_damageBonusMods.m_count != other.m_damageBonusMods.m_count)
        return 0;

    for (int i = 0; i < m_attackBonusMods.m_count; i++)
        if (m_attackBonusMods.m_pArray[i] != other.m_attackBonusMods.m_pArray[i])
            return 0;
    for (int i = 0; i < m_damageBonusMods.m_count; i++)
        if (m_damageBonusMods.m_pArray[i] != other.m_damageBonusMods.m_pArray[i])
            return 0;
    return 1;
}

// Unlike operator==, which compares the node pointers, this compares each node's fields.
int CCombatInformation::operator!=(CCombatInformation& other)
{
    if (m_nNumOnHandAttacks != other.m_nNumOnHandAttacks
        || m_nAttackModOnHand != other.m_nAttackModOnHand
        || m_nStrDamageModOnHand != other.m_nStrDamageModOnHand
        || m_nAttackModOffHand != other.m_nAttackModOffHand
        || m_nStrDamageModOffHand != other.m_nStrDamageModOffHand
        || m_bOffHandWeaponEquipped != other.m_bOffHandWeaponEquipped
        || m_nForceResistance != other.m_nForceResistance
        || m_nArcaneSpellFailure != other.m_nArcaneSpellFailure
        || m_bArmorCheckPenalty != other.m_bArmorCheckPenalty
        || m_nDefaultDamageDie != other.m_nDefaultDamageDie
        || m_nDefaultDamageDiceCount != other.m_nDefaultDamageDiceCount
        || m_nCritRangeOnHand != other.m_nCritRangeOnHand
        || m_nCritMultiplierOnHand != other.m_nCritMultiplierOnHand
        || m_nCritRangeOffHand != other.m_nCritRangeOffHand
        || m_nCritMultiplierOffHand != other.m_nCritMultiplierOffHand
        || m_offHandWeaponId != other.m_offHandWeaponId
        || m_onHandWeaponId != other.m_onHandWeaponId
        || m_attackBonusMods.m_count != other.m_attackBonusMods.m_count
        || m_damageBonusMods.m_count != other.m_damageBonusMods.m_count)
        return 1;

    for (int i = 0; i < m_attackBonusMods.m_count; i++)
    {
        CCombatInformationNode* pNode = m_attackBonusMods.m_pArray[i];
        CCombatInformationNode* pOther = other.m_attackBonusMods.m_pArray[i];
        if (*pNode != *pOther)
            return 1;
    }
    for (int i = 0; i < m_damageBonusMods.m_count; i++)
    {
        CCombatInformationNode* pNode = m_damageBonusMods.m_pArray[i];
        CCombatInformationNode* pOther = other.m_damageBonusMods.m_pArray[i];
        if (*pNode != *pOther)
            return 1;
    }
    return 0;
}

// Deep-copies the scalar fields, clears both node lists and then appends a fresh node per source
// element.
CCombatInformation& CCombatInformation::operator=(CCombatInformation& other)
{
    if (this == &other)
        return *this;

    m_nNumOnHandAttacks = other.m_nNumOnHandAttacks;
    m_nAttackModOnHand = other.m_nAttackModOnHand;
    m_nStrDamageModOnHand = other.m_nStrDamageModOnHand;
    m_nAttackModOffHand = other.m_nAttackModOffHand;
    m_nStrDamageModOffHand = other.m_nStrDamageModOffHand;
    m_bOffHandWeaponEquipped = other.m_bOffHandWeaponEquipped;
    m_nForceResistance = other.m_nForceResistance;
    m_nArcaneSpellFailure = other.m_nArcaneSpellFailure;
    m_bArmorCheckPenalty = other.m_bArmorCheckPenalty;
    m_nDefaultDamageDie = other.m_nDefaultDamageDie;
    m_nDefaultDamageDiceCount = other.m_nDefaultDamageDiceCount;
    m_nCritRangeOnHand = other.m_nCritRangeOnHand;
    m_nCritMultiplierOnHand = other.m_nCritMultiplierOnHand;
    m_nCritRangeOffHand = other.m_nCritRangeOffHand;
    m_nCritMultiplierOffHand = other.m_nCritMultiplierOffHand;
    m_offHandWeaponId = other.m_offHandWeaponId;
    m_onHandWeaponId = other.m_onHandWeaponId;

    m_attackBonusMods.SetSize(0);
    m_damageBonusMods.SetSize(0);

    for (int i = 0; i < other.m_attackBonusMods.m_count; i++)
    {
        CCombatInformationNode* pNode = new CCombatInformationNode();
        *pNode = *other.m_attackBonusMods.m_pArray[i];
        m_attackBonusMods.Add(pNode);
    }
    for (int i = 0; i < other.m_damageBonusMods.m_count; i++)
    {
        CCombatInformationNode* pNode = new CCombatInformationNode();
        *pNode = *other.m_damageBonusMods.m_pArray[i];
        m_damageBonusMods.Add(pNode);
    }
    return *this;
}
