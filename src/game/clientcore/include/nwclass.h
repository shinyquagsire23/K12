#pragma once

#include "exostring.h"
#include "strref.h"

class CExoString;
class CSWRules;

// One classes.2da SkillsTable entry.
struct CSWClassSkill {
    unsigned short m_nSkill;
    unsigned char  _pad02[0x4 - 0x2];
    int            m_bClassSkill;
    int            m_nRecommendedOrder;
};

// One recommended-spell row, scanned by GetRecommendedSpell.
struct CSWClassSpell {
    unsigned short m_nSpellId;
    unsigned char  _pad02[0x4 - 0x2];
    int            m_nLightRecommended;
    int            m_nDarkRecommended;
    unsigned char  m_nUserType;
};

// One classes.2da granted-feat table row.
struct CSWClassGrantedFeat {
    unsigned short m_nFeat;
    unsigned char  m_nLevel;
    // 2-bit set: 0 grants the feat automatically, bit 0 is the normal feat list and bit 1 the bonus
    // list.
    unsigned char  m_nFeatList;
#if KOTOR1  // K1 has no "_PC_Granted" column, so the record is 8 bytes
    int            m_nRecommendedOrder;
#else
    unsigned char  m_bPCOnly;
    int            m_nRecommendedOrder;
#endif
};

// One row of classes.2da, owned by CSWRules as `new CSWClass[]`.
class CSWClass {
public:
    // The unset strrefs, the GetRecommended* answer when the table is empty, and the level
    // answers (spell gain, feat granted) for none.
    enum : unsigned int {
        RECOMMENDED_NONE = 0xffffffffu,
    };
    enum {
        LEVEL_NONE = 0xff,
    };

    // Row ids of classes.2da.
    static const unsigned char CCLASS_SOLDIER;
    static const unsigned char CCLASS_SCOUT;
    static const unsigned char CCLASS_SCOUNDREL;
    static const unsigned char CCLASS_JEDIGUARDIAN;
    static const unsigned char CCLASS_JEDICONSULAR;
    static const unsigned char CCLASS_JEDISENTINEL;
    static const unsigned char CCLASS_COMBATDROID;
    static const unsigned char CCLASS_EXPERTDROID;
    static const unsigned char CCLASS_MINION;
    static const unsigned char CCLASS_TECHSPECIALIST;
    static const unsigned char CCLASS_BOUNTYHUNTER;
    static const unsigned char CCLASS_JEDIWEAPONMASTER;
    static const unsigned char CCLASS_JEDIMASTER;
    static const unsigned char CCLASS_JEDIWATCHMAN;
    static const unsigned char CCLASS_SITHMARAUDER;
    static const unsigned char CCLASS_SITHLORD;
    static const unsigned char CCLASS_SITHASSASSIN;

    CSWClass();
    ~CSWClass();

    // The class's base attack bonus at the given level, from classes.2da AttackBonusTable.
    unsigned char GetAttackBonus(unsigned char nLevel);
    unsigned char GetFortSaveBonus(unsigned char nLevel);
    unsigned char GetRefSaveBonus(unsigned char nLevel);
    unsigned char GetWillSaveBonus(unsigned char nLevel);
    unsigned char GetArmorBonus(int nLevel);
    // The four text accessors, one per strref field at the head of the row; each returns the fetched
    // string by value.
    CExoString GetNameText();
    CExoString GetNameLowerText();
    CExoString GetNamePluralText();
    CExoString GetDescriptionText();

    // Whether the class grants use of a skill (classes.2da SkillsTable).
    int IsSkillUseable(unsigned short nSkill);
    // The class-skill row's own m_bClassSkill, 0 when the skill is not in this class's table.
    int IsSkillClassSkill(unsigned short nSkill);
    // Scans the granted-feat table and returns one bit of the matched row's m_nFeatList.
    int IsNormalFeat(unsigned short nFeat);
    int IsBonusFeat(unsigned short nFeat);
    // Whether the class can use the feat at all, from feats.2da and this class's granted-feat table.
    int IsFeatUseable(unsigned short nFeat);

    int IsGrantedFeat(unsigned short nFeat, unsigned char& nLevel);
#if !KOTOR1
    int IsPCGrantedFeat(unsigned short nFeat);
#endif
    static int IsDroid(int nClass);
    void LoadSpellsTable();
    void LoadAttackBonusTable(const CExoString& sTable);
    void LoadFeatsTable(const CExoString& sTable, CSWRules* pRules);
    void LoadSavingThrowTable(const CExoString& sTable);
    void LoadSkillsTable(const CExoString& sTable);
    void LoadSpellGainTable(const CExoString& sTable);
    void LoadSpellKnownTable(const CExoString& sTable);
    void LoadFeatGain(const CExoString& sTable);
    void LoadArmorClassTable(const CExoString& sTable);
    static int IsJedi(int nClass);

    // Per-(level, spell level) force-power slot count from classes.2da SpellGainTable; nMinLevel
    // gates a minimum caster level.
    int GetSpellGain(unsigned char nLevel, unsigned char nMinLevel);
    unsigned int GetRecommendedSkill(int nOrder);
    unsigned int GetRecommendedSpell(int bGoodAligned, int nOrder);
    unsigned int GetRecommendedFeat(int nOrder);
    unsigned char GetLevelFeatGranted(unsigned short nFeat);
    unsigned char GetSpellsKnownPerLevel(unsigned char nLevel, unsigned char nSpellLevel,
                                          unsigned char nClassId, unsigned short nRace,
                                          unsigned char nCHA);

    unsigned int   m_nNameStrRef;
    unsigned int   m_nNameLowerStrRef;
    unsigned int   m_nNamePluralStrRef;
    unsigned int   m_nDescriptionStrRef;
    CExoString     m_sIcon;
    unsigned char  m_aAttackBonus[60];
    unsigned char  m_nHitDie;
    unsigned char  m_nForcePointGain;
    unsigned char  m_nSkillPointsBase;
    unsigned char  m_aFortSaveBonus[60];
    unsigned char  m_aWillSaveBonus[60];
    unsigned char  m_aRefSaveBonus[60];
    unsigned char* m_pArmorBonusTable;
    unsigned char** m_paSpellsKnownPerLevel;
#if KOTOR1
    // K1's level cap is 20, so these four tables have 20/20/20/21 entries.
    unsigned char  m_aSpellGainMinLevel[20];
#else
    unsigned char  m_aSpellGainMinLevel[50];
#endif
    // The featgain.2da per-level counts follow the spell-gain table. Both are indexed with the class
    // level itself, 1 to the cap, so each one's unused entry 0 is the byte before it: the last
    // spell-gain entry for the bonus table, and the bonus table's last entry for the normal one.
    union {
#if KOTOR1
        unsigned char m_aSpellGain[20];
        struct { unsigned char _pad[19]; unsigned char _bonus[20]; unsigned char m_aByLevel[21]; } m_normalFeats;
        struct { unsigned char _pad[19]; unsigned char m_aByLevel[21]; } m_bonusFeats;
#else
        unsigned char m_aSpellGain[50];
        struct { unsigned char _pad[49]; unsigned char _bonus[50]; unsigned char m_aByLevel[51]; } m_normalFeats;
        struct { unsigned char _pad[49]; unsigned char m_aByLevel[51]; } m_bonusFeats;
#endif
    };
    CSWClassSkill* m_paClassSkills;
    unsigned short m_nNumClassSkills;
    CSWClassGrantedFeat* m_paGrantedFeats;
    unsigned short m_nNumGrantedFeats;
    CSWClassSpell* m_paRecommendedSpells;
    unsigned short m_nNumRecommendedSpells;
    unsigned char  m_nPrimaryAbility;
    unsigned char  m_abRecommendedScores[6];
    unsigned char  m_alignRestrict;
    unsigned char  m_nAlignRestrictType;
    // Per-level challenge-rating bytes, indexed by class level.
#if KOTOR1
    unsigned char  m_anEffCRByLevel[21];
#else
    unsigned char  m_anEffCRByLevel[51];
#endif
    int            m_bPlayerClass;           // nonzero for a selectable PC class
    int            m_bSpellCaster;           // classes.2da "SpellCaster"
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWClass) == 0x1c8, "CSWClass K1 stride (CSWRules::~CSWRules @0x2eb0a0)");
#else
K2_LAYOUT_ASSERT(sizeof(CSWClass) == 0x260, "CSWClass K2 stride");
#endif
