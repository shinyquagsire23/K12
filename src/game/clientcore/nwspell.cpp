#include "k2stdint.h"
#include "nwspell.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "exolocstringinternal.h"
#include "exotlktable.h"
#include "twodimarray.h"
#include "exoresman.h"
#include "nwscriptconst.h"

// The good-evil scale's light-side band (60 and up) and its midpoint: a power costs less on the
// light side and more below the midpoint by the room's Force rating.
enum {
    ALIGNMENT_LIGHT_SIDE_LOWEST  = 60,
    ALIGNMENT_GOOD_EVIL_MIDPOINT = 50,
};

const int CSWSpell::USERTYPE_NONE = 0;
const int CSWSpell::USERTYPE_FORCEPOWER = 1;
const int CSWSpell::USERTYPE_MONSTERPOWER = 2;
const int CSWSpell::USERTYPE_FEAT = 3;
const int CSWSpell::USERTYPE_ITEM = 4;
const int CSWSpell::USERTYPE_MASTER = 5;
const int CSWSpell::USERTYPE_FORM = 6;

CSWSpell::CSWSpell()
{
    m_nConjureTimeMS = 3000;
    m_conjSoundMale = const_cast<char*>("amb_e08b");
    m_conjSoundFemale = const_cast<char*>("amb_e09a");
    m_conjSoundVFX = const_cast<char*>("");
    m_castSound = const_cast<char*>("");
    m_nTargetType = 0;
    m_sImpactScript = "default";
    m_bHasProjectile = 0;
    m_projectileResRef = const_cast<char*>("sp_sumfire");
    m_nProjectileType = 1;
    m_projSoundResRef = const_cast<char*>("amb_e14");
    m_castHeadVisual = const_cast<char*>("");
    m_castHandVisual = const_cast<char*>("");
    m_castGrndVisual = const_cast<char*>("");
    m_conjHeadVisual = const_cast<char*>("");
    m_conjHandVisual = const_cast<char*>("");
    m_conjGrndVisual = const_cast<char*>("");
    m_nCastAttachNode = 0;
    m_nCastPauseMS = 0;
    m_nProjOrientation = 0;
    m_nReserved1 = 0;
    m_nReserved2 = 0;
    m_paSubRadialSpells = NULL;
    m_nSubRadialCount = 0;
#if KOTOR1  // K1: the row defaults to 1 (K2 0).
    m_bUseConcentration = 1;
#else
    m_bUseConcentration = 0;
#endif
    m_nCounter1 = -1;
    m_nCounter2 = -1;
    m_nUserType = 0;
    m_nAltMessage = 0;
    m_bHostileActivity = 0;
    m_nCastRecoveryMS = 0;
    m_nCatchAnimation = 0;
    m_nItemTargeting = 0;
    m_nExclusionMask = 0;
    m_nForbidItemMask = 0;
    m_nRequireItemMask = 0;
#if !KOTOR1
    m_nSpellFormMask = 0;
#endif
}

#include "port/libcstring.h"

CSWSpell::~CSWSpell()
{
    delete[] m_paSubRadialSpells;
}

unsigned int CSWSpell::GetSubRadialSpell(unsigned char idx)
{
    if (idx < m_nSubRadialCount)
        return m_paSubRadialSpells[idx];
    return static_cast<unsigned int>(-1);
}

void CSWSpell::SetSubRadialSpell(unsigned char idx, unsigned int spellId)
{
    if (idx < m_nSubRadialCount)
        m_paSubRadialSpells[idx] = spellId;
}

unsigned char CSWSpell::GetSpellLevel(unsigned char nClassId)
{
#if KOTOR1  // K1: only classes 0-2 are non-Force; no prestige-class slots.
    switch (nClassId) {
    case CLASS_TYPE_SOLDIER: case CLASS_TYPE_SCOUT: case CLASS_TYPE_SCOUNDREL:
        return SPELL_LEVEL_NONE;
    case CLASS_TYPE_JEDIGUARDIAN:  return m_nSpellLevelClass3;
    case CLASS_TYPE_JEDICONSULAR:  return m_nSpellLevelClass4;
    case CLASS_TYPE_JEDISENTINEL:  return m_nSpellLevelClass5;
    default:                       return m_nSpellLevelOther;
    }
#else
    switch (nClassId) {
    case CLASS_TYPE_SOLDIER: case CLASS_TYPE_SCOUT: case CLASS_TYPE_SCOUNDREL:
    case CLASS_TYPE_TECHSPECIALIST: case CLASS_TYPE_BOUNTYHUNTER:
        return SPELL_LEVEL_NONE;
    case CLASS_TYPE_JEDIGUARDIAN:     return m_nSpellLevelClass3;
    case CLASS_TYPE_JEDICONSULAR:     return m_nSpellLevelClass4;
    case CLASS_TYPE_JEDISENTINEL:     return m_nSpellLevelClass5;
    case CLASS_TYPE_JEDIWEAPONMASTER: return m_nSpellLevelClass11;
    case CLASS_TYPE_JEDIMASTER:       return m_nSpellLevelClass12;
    case CLASS_TYPE_JEDIWATCHMAN:     return m_nSpellLevelClass13;
    case CLASS_TYPE_SITHMARAUDER:     return m_nSpellLevelClass14;
    case CLASS_TYPE_SITHLORD:         return m_nSpellLevelClass15;
    case CLASS_TYPE_SITHASSASSIN:     return m_nSpellLevelClass16;
    default:                          return m_nSpellLevelOther;
    }
#endif
}

void CSWSpell::SetSubRadialSpellCount(unsigned char nCount)
{
    if (m_paSubRadialSpells != NULL)
    {
        delete[] m_paSubRadialSpells;
        m_paSubRadialSpells = NULL;
    }
    if (nCount != 0)
    {
        m_paSubRadialSpells = new unsigned int[nCount];
    }
    // The count store is unconditional, so a zero count still resets the array.
    m_nSubRadialCount = nCount;
}

CExoString CSWSpell::GetSpellNameText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nNameStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

CSWSpellArray::CSWSpellArray() : m_nCount(0), m_paSpells(NULL)
{
}

CSWSpellArray::~CSWSpellArray()
{
    if (m_paSpells != NULL)
    {
        delete[] m_paSpells;
        m_paSpells = NULL;
        m_nCount = 0;
    }
}

// Reads every row of spells.2da into a freshly allocated m_paSpells array, pulling each column with its
// own Get*Entry call straight into the row.
void CSWSpellArray::Load()
{
    C2DA* pTable = new C2DA(CResRef("SPELLS"), 0);
    pTable->Load2DArray();

    m_nCount = pTable->m_nRows;
    m_paSpells = new CSWSpell[m_nCount];

    CExoString sValue;
    for (int nRow = 0; nRow < m_nCount; nRow++)
    {
        int nValue;

#if !KOTOR1
        m_paSpells[nRow].m_nSpellId = nRow;
#endif

        pTable->GetINTEntry(nRow, CExoString("Category"), &nValue);
        m_paSpells[nRow].m_nCategory = nValue;

        pTable->GetINTEntry(nRow, CExoString("MaxCR"), &nValue);
        m_paSpells[nRow].m_nMaxCR = nValue;

        pTable->GetCExoStringEntry(nRow, CExoString("Prerequisites"), &sValue);
        if (sValue.Left(1) != "_" && sValue.Left(1) != ":")
            sValue = CExoString("_") + sValue;
        if (sValue.Right(1) != "_" && sValue.Right(1) != ":")
            sValue = sValue + "_";
        m_paSpells[nRow].SetPrerequisites(sValue);

        pTable->GetCExoStringEntry(nRow, CExoString("MasterSpell"), &sValue);
        if (sValue.Left(1) != "_" && sValue.Left(1) != ":")
            sValue = CExoString("_") + sValue;
        if (sValue.Right(1) != "_" && sValue.Right(1) != ":")
            sValue = sValue + "_";
        m_paSpells[nRow].SetMasterSpells(sValue);

        pTable->GetCExoStringEntry(nRow, CExoString("Label"), &sValue);
        m_paSpells[nRow].m_sName = sValue;

        pTable->GetINTEntry(nRow, CExoString("Name"), &nValue);
        m_paSpells[nRow].m_nNameStrRef = nValue;

        pTable->GetINTEntry(nRow, CExoString("ForcePoints"), &nValue);
        m_paSpells[nRow].m_nBaseForcePoints = static_cast<unsigned char>(nValue);

        pTable->GetCExoStringEntry(nRow, CExoString("GoodEvil"), &sValue);
        m_paSpells[nRow].SetSpellAlignment(sValue[0]);

        pTable->GetINTEntry(nRow, CExoString("SpellDesc"), &nValue);
        m_paSpells[nRow].m_nDescriptionStrRef = nValue;

        pTable->GetCExoStringEntry(nRow, CExoString("IconResRef"), &sValue);
        m_paSpells[nRow].SetSpellIcon(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("Range"), &sValue);
        m_paSpells[nRow].SetSpellRange(sValue.Left(1));

        pTable->GetINTEntry(nRow, CExoString("TargetType"), &nValue);
        m_paSpells[nRow].m_nTargetType = static_cast<unsigned short>(nValue);

        pTable->GetCExoStringEntry(nRow, CExoString("ImpactScript"), &sValue);
        m_paSpells[nRow].m_sImpactScript = sValue;

        m_paSpells[nRow].m_nSpellLevelClass3 = CSWSpell::SPELL_LEVEL_NONE;
        m_paSpells[nRow].m_nSpellLevelClass4 = CSWSpell::SPELL_LEVEL_NONE;
        m_paSpells[nRow].m_nSpellLevelClass5 = CSWSpell::SPELL_LEVEL_NONE;
        m_paSpells[nRow].m_nSpellLevelOther = CSWSpell::SPELL_LEVEL_NONE;
#if !KOTOR1  // K1: four level columns, no prestige classes.
        m_paSpells[nRow].m_nSpellLevelClass11 = CSWSpell::SPELL_LEVEL_NONE;
        m_paSpells[nRow].m_nSpellLevelClass12 = CSWSpell::SPELL_LEVEL_NONE;
        m_paSpells[nRow].m_nSpellLevelClass13 = CSWSpell::SPELL_LEVEL_NONE;
        m_paSpells[nRow].m_nSpellLevelClass14 = CSWSpell::SPELL_LEVEL_NONE;
        m_paSpells[nRow].m_nSpellLevelClass15 = CSWSpell::SPELL_LEVEL_NONE;
        m_paSpells[nRow].m_nSpellLevelClass16 = CSWSpell::SPELL_LEVEL_NONE;
#endif

        if (pTable->GetINTEntry(nRow, CExoString("Guardian"), &nValue))
            m_paSpells[nRow].m_nSpellLevelClass3 = static_cast<unsigned char>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("Consular"), &nValue))
            m_paSpells[nRow].m_nSpellLevelClass4 = static_cast<unsigned char>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("Sentinel"), &nValue))
            m_paSpells[nRow].m_nSpellLevelClass5 = static_cast<unsigned char>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("Inate"), &nValue))
            m_paSpells[nRow].m_nSpellLevelOther = static_cast<unsigned char>(nValue);

#if !KOTOR1
        if (pTable->GetINTEntry(nRow, CExoString("WeapMstr"), &nValue))
            m_paSpells[nRow].m_nSpellLevelClass11 = static_cast<unsigned char>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("JediMaster"), &nValue))
            m_paSpells[nRow].m_nSpellLevelClass12 = static_cast<unsigned char>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("Watchman"), &nValue))
            m_paSpells[nRow].m_nSpellLevelClass13 = static_cast<unsigned char>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("Marauder"), &nValue))
            m_paSpells[nRow].m_nSpellLevelClass14 = static_cast<unsigned char>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("SithLord"), &nValue))
            m_paSpells[nRow].m_nSpellLevelClass15 = static_cast<unsigned char>(nValue);

        if (pTable->GetINTEntry(nRow, CExoString("Assassin"), &nValue))
            m_paSpells[nRow].m_nSpellLevelClass16 = static_cast<unsigned char>(nValue);
#endif

        pTable->GetINTEntry(nRow, CExoString("ConjTime"), &nValue);
        m_paSpells[nRow].m_nConjureTimeMS = nValue;

        pTable->GetCExoStringEntry(nRow, CExoString("ConjSoundMale"), &sValue);
        m_paSpells[nRow].SetSpellConjureSoundMale(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("ConjSoundFemale"), &sValue);
        m_paSpells[nRow].SetSpellConjureSoundFemale(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("ConjSoundVFX"), &sValue);
        m_paSpells[nRow].SetSpellConjureSoundVFX(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("ConjHeadVisual"), &sValue);
        m_paSpells[nRow].SetSpellConjureHeadVisual(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("ConjHandVisual"), &sValue);
        m_paSpells[nRow].SetSpellConjureHandVisual(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("ConjGrndVisual"), &sValue);
        m_paSpells[nRow].SetSpellConjureGroundVisual(CResRef(sValue));

        pTable->GetINTEntry(nRow, CExoString("CastTime"), &nValue);
        m_paSpells[nRow].m_nCastPauseMS = nValue;

        pTable->GetCExoStringEntry(nRow, CExoString("CastAnim"), &sValue);
        sValue = sValue.LowerCase();
        if (sValue == "dark")             nValue = CSWSpell::CAST_ANIMATION_DARK;
        else if (sValue == "self")        nValue = CSWSpell::CAST_ANIMATION_SELF;
        else if (sValue == "up")          nValue = CSWSpell::CAST_ANIMATION_UP;
        else if (sValue == "area")        nValue = CSWSpell::CAST_ANIMATION_AREA;
        else if (sValue == "point")       nValue = CSWSpell::CAST_ANIMATION_POINT;
        else if (sValue == "touch")       nValue = CSWSpell::CAST_ANIMATION_POINT;
        else if (sValue == "throw")       nValue = CSWSpell::CAST_ANIMATION_THROW;
        else if (sValue == "jump")        nValue = CSWSpell::CAST_ANIMATION_JUMP;
        else if (sValue == "monster")     nValue = CSWSpell::CAST_ANIMATION_MONSTER;
#if !KOTOR1  // K1: the chain ends at "monster".
        else if (sValue == "fury")        nValue = CSWSpell::CAST_ANIMATION_FURY;
        else if (sValue == "monsterfury") nValue = CSWSpell::CAST_ANIMATION_MONSTER_FURY;
        else if (sValue == "crush")       nValue = CSWSpell::CAST_ANIMATION_CRUSH;
#endif
        else                              nValue = CSWSpell::CAST_ANIMATION_NONE;
        m_paSpells[nRow].m_nCastAnimationKind = static_cast<unsigned short>(nValue);
        m_paSpells[nRow].m_nConjureAnimationKind = static_cast<unsigned short>(nValue);

        pTable->GetCExoStringEntry(nRow, CExoString("CastHeadVisual"), &sValue);
        m_paSpells[nRow].SetSpellCastHeadVisual(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("CastHandVisual"), &sValue);
        m_paSpells[nRow].SetSpellCastHandVisual(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("CastSound"), &sValue);
        m_paSpells[nRow].SetSpellCastSound(CResRef(sValue));

        pTable->GetCExoStringEntry(nRow, CExoString("CastGrndVisual"), &sValue);
        m_paSpells[nRow].SetSpellCastGroundVisual(CResRef(sValue));

        pTable->GetINTEntry(nRow, CExoString("CatchTime"), &nValue);
        m_paSpells[nRow].m_nCastRecoveryMS = nValue;

        pTable->GetCExoStringEntry(nRow, CExoString("CatchAnim"), &sValue);
        sValue = sValue.LowerCase();
        if (sValue == "catch")
            nValue = 0;

        int anTimes[5] = { 0 };
        int nCount = 0;
        if (pTable->GetINTEntry(nRow, CExoString("FORCEFRIENDLY"), &nValue))
        {
            anTimes[nCount] = nValue * 10 + 3000;
            pTable->GetINTEntry(nRow, CExoString("FORCEPRIORITY"), &nValue);
            anTimes[nCount] += nValue;
            nCount++;
        }
        if (pTable->GetINTEntry(nRow, CExoString("FORCEHOSTILE"), &nValue))
        {
            anTimes[nCount] = nValue * 10 + 1000;
            pTable->GetINTEntry(nRow, CExoString("FORCEPRIORITY"), &nValue);
            anTimes[nCount] += nValue;
            nCount++;
        }
        if (pTable->GetINTEntry(nRow, CExoString("FORCEPASSIVE"), &nValue))
        {
            anTimes[nCount] = nValue * 10 + 2000;
            pTable->GetINTEntry(nRow, CExoString("FORCEPRIORITY"), &nValue);
            anTimes[nCount] += nValue;
            nCount++;
        }

        m_paSpells[nRow].SetSubRadialSpellCount(static_cast<unsigned char>(nCount));
        for (int i = 0; i < nCount; i++)
            m_paSpells[nRow].SetSubRadialSpell(static_cast<unsigned char>(i), anTimes[i]);

        if (pTable->GetINTEntry(nRow, CExoString("Counter2"), &nValue))
            m_paSpells[nRow].m_nCounter2 = nValue;
        else
            m_paSpells[nRow].m_nCounter2 = -1;

        pTable->GetINTEntry(nRow, CExoString("Proj"), &nValue);
        m_paSpells[nRow].m_bHasProjectile = nValue;
        if (nValue != 0)
        {
            pTable->GetCExoStringEntry(nRow, CExoString("ProjModel"), &sValue);
            m_paSpells[nRow].SetSpellProjectileModel(CResRef(sValue));

            pTable->GetCExoStringEntry(nRow, CExoString("ProjType"), &sValue);
            sValue = sValue.LowerCase();
            nValue = 0;
            if (sValue == "homing")            nValue = 1;
            else if (sValue == "ballistic")    nValue = 2;
            else if (sValue == "highballistic") nValue = 3;
            else if (sValue == "burst")        nValue = 9;
            else if (sValue == "accelerating") nValue = 5;
            else if (sValue == "spiral")       nValue = 6;
            else if (sValue == "linked")       nValue = 7;
            else if (sValue == "grenade")      nValue = 11;
            else if (sValue == "bounce")       nValue = 8;
            else if (sValue == "grenade")      nValue = 11;
            else if (sValue == "test")         nValue = 50;
            m_paSpells[nRow].m_nProjectileType = nValue;

            pTable->GetCExoStringEntry(nRow, CExoString("ProjSpwnPoint"), &sValue);
            sValue = sValue.LowerCase();
            if (sValue == "hand")       nValue = 1;
            else if (sValue == "head")  nValue = 2;
            else if (sValue == "throw") nValue = 3;
            else                        nValue = 0;
            m_paSpells[nRow].m_nCastAttachNode = static_cast<unsigned char>(nValue);

            pTable->GetCExoStringEntry(nRow, CExoString("ProjSound"), &sValue);
            m_paSpells[nRow].SetSpellProjectileSound(CResRef(sValue));

            pTable->GetCExoStringEntry(nRow, CExoString("ProjOrientation"), &sValue);
            sValue = sValue.LowerCase();
            nValue = 0;
            if (sValue == "target")      nValue = 1;
            else if (sValue == "path")   nValue = 2;
            m_paSpells[nRow].m_nProjOrientation = static_cast<unsigned char>(nValue);
        }
        else
        {
            sValue = "sp_sumfire";
            m_paSpells[nRow].SetSpellProjectileModel(CResRef(sValue));
            m_paSpells[nRow].m_nProjectileType = 1;
            m_paSpells[nRow].SetSpellProjectileSound(CResRef("amb_e14"));
        }

        pTable->GetINTEntry(nRow, CExoString("UserType"), &nValue);
        m_paSpells[nRow].m_nUserType = static_cast<unsigned char>(nValue);

        pTable->GetINTEntry(nRow, CExoString("AltMessage"), &nValue);
        m_paSpells[nRow].m_nAltMessage = nValue;

        pTable->GetINTEntry(nRow, CExoString("HostileSetting"), &nValue);
        m_paSpells[nRow].m_bHostileActivity = nValue != 0;

        pTable->GetINTEntry(nRow, CExoString("FeatID"), &nValue);
        m_paSpells[nRow].m_nFeatId = static_cast<unsigned short>(nValue);

        pTable->GetINTEntry(nRow, CExoString("Exclusion"), &nValue);
        m_paSpells[nRow].m_nExclusionMask = nValue;

        pTable->GetINTEntry(nRow, CExoString("RequireItemMask"), &nValue);
        m_paSpells[nRow].m_nRequireItemMask = nValue;

        pTable->GetINTEntry(nRow, CExoString("ForbidItemMask"), &nValue);
        m_paSpells[nRow].m_nForbidItemMask = nValue;

        if (pTable->GetINTEntry(nRow, CExoString("ItemTargeting"), &nValue))
            m_paSpells[nRow].m_nItemTargeting = static_cast<unsigned char>(nValue);
        else
            m_paSpells[nRow].m_nItemTargeting = 0;

#if !KOTOR1  // K1: no FormMask column.
        pTable->GetINTEntry(nRow, CExoString("FormMask"), &nValue);
        m_paSpells[nRow].m_nSpellFormMask = nValue;
#endif
    }

    delete pTable;
}

// The power's force-point cost for a given caster state; 0 when the spell has no base cost. With
// bInParty set, the base cost is scaled by the ForceAdjust 2DA float for the caster's alignment bucket
// (nAlignment / 10), picking the good or evil column from m_nAlignmentCode. The room adjustment
// subtracts nRoomForceRating / 20 for good alignments and adds it for dark ones; two Force forms
// scale the result by 1.2. The fourth argument is nStanceId, which CSWSCreature::m_stanceId feeds
// end to end.
#if KOTOR1  // K1: the alignment column only, with no room rating, charisma discount or stance
             // surcharge, and a zero base cost is not special-cased
int CSWSpell::GetAdjustedForcePoints(unsigned int nAlignment, int bInParty)
{
    int nCost = m_nBaseForcePoints;
    if (bInParty != 0) {
        float fCost = 1.0f;
        if (m_nAlignmentCode == ALIGNMENT_CODE_GOOD) {
            g_pRules->m_p2DArrays->m_pForceAdjust->GetFLOATEntry(
                nAlignment / 10, CTwoDimArrays::COL2DA_FORCEADJUST_GOODCOST, &fCost);
        } else if (m_nAlignmentCode == ALIGNMENT_CODE_EVIL) {
            g_pRules->m_p2DArrays->m_pForceAdjust->GetFLOATEntry(
                nAlignment / 10, CTwoDimArrays::COL2DA_FORCEADJUST_EVILCOST, &fCost);
        }
        nCost = static_cast<int>(fCost * static_cast<float>(nCost));
    }
    return nCost;
}
#endif
#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSpell::GetAdjustedForcePoints(unsigned int nAlignment, int bInParty, int nRoomForceRating,
                                     int nStanceId, char cCHAMod)
{
    int nCost = m_nBaseForcePoints;
    if (nCost == 0)
        return 0;
    if (bInParty != 0) {
        float fCost = 1.0f;
        if (m_nAlignmentCode == ALIGNMENT_CODE_GOOD) {
            g_pRules->m_p2DArrays->m_pForceAdjust->GetFLOATEntry(
                nAlignment / 10, CTwoDimArrays::COL2DA_FORCEADJUST_GOODCOST, &fCost);
        } else if (m_nAlignmentCode == ALIGNMENT_CODE_EVIL) {
            g_pRules->m_p2DArrays->m_pForceAdjust->GetFLOATEntry(
                nAlignment / 10, CTwoDimArrays::COL2DA_FORCEADJUST_EVILCOST, &fCost);
        }
        if (cCHAMod > 0 && fCost > 1.0f) {
            fCost += static_cast<float>(cCHAMod) * -0.05f;
            if (fCost < 1.0f)
                fCost = 1.0f;
        }
        nCost = static_cast<int>(fCost * static_cast<float>(nCost));
    }
    int nAdjust = nRoomForceRating / 20;
    int nResult;
    if (nAlignment >= ALIGNMENT_LIGHT_SIDE_LOWEST)
        nResult = nCost - nAdjust;
    else
        nResult = nCost + (nAlignment < ALIGNMENT_GOOD_EVIL_MIDPOINT ? nAdjust : 0);
    if (nResult < 2)
        nResult = 1;
    if (nStanceId == FORM_FORCE_IV_MASTERY || nStanceId == FORM_FORCE_II_POTENCY)
        nResult = static_cast<int>(static_cast<float>(nResult) * 1.2f);
    return nResult;
}
#endif // !KOTOR1

// Parses the spells.2da prerequisites column into m_anPrereq1: decimal ids separated by runs of '_' or
// ':', closed with -1. A short list leaves the tail slots at -1 and a long one is truncated at five.
void CSWSpell::SetPrerequisites(const CExoString& sPrereq)
{
    m_sPrerequisites = sPrereq;
#if KOTOR1  // K1 calls the out-of-line CExoString::GetLength.
    int nLen = sPrereq.GetLength();
#else
    int nLen = sPrereq.m_sString != NULL ? static_cast<int>(strlen(sPrereq.m_sString)) : 0;
#endif
    int i = 0;
    while (i < nLen && (sPrereq[i] == '_' || sPrereq[i] == ':'))
        i++;
    for (int n = 0; n < 5; n++)
    {
        m_anPrereq1[n] = -1;
        if (i < nLen)
        {
            int nId = 0;
            while (sPrereq[i] != '_' && sPrereq[i] != ':')
            {
                nId = nId * 10 + static_cast<unsigned char>(sPrereq[i]) - '0';
                i++;
            }
            i++;
            m_anPrereq1[n] = nId;
        }
    }
}

void CSWSpell::SetMasterSpells(const CExoString& sMaster)
{
    m_sMasterSpells = sMaster;
#if KOTOR1  // K1 calls the out-of-line CExoString::GetLength.
    int nLen = sMaster.GetLength();
#else
    int nLen = sMaster.m_sString != NULL ? static_cast<int>(strlen(sMaster.m_sString)) : 0;
#endif
    int i = 0;
    while (i < nLen && (sMaster[i] == '_' || sMaster[i] == ':'))
        i++;
    for (int n = 0; n < 5; n++)
    {
        m_anPrereq2[n] = -1;
        if (i < nLen)
        {
            int nId = 0;
            while (sMaster[i] != '_' && sMaster[i] != ':')
            {
                nId = nId * 10 + static_cast<unsigned char>(sMaster[i]) - '0';
                i++;
            }
            i++;
            m_anPrereq2[n] = nId;
        }
    }
}

CSWSpell CSWSpellArray::operator[](int index) const
{
    if (index < 0 || m_paSpells == NULL || index >= m_nCount)
    {
        CSWSpell spell;
        return CSWSpell(spell);
    }
    return m_paSpells[index];
}

CSWSpell* CSWSpellArray::GetSpell(int spellId) const
{
    if (m_paSpells == NULL || spellId < 0)
        return NULL;
    if (spellId >= m_nCount)
        return NULL;
    return &m_paSpells[spellId];
}
