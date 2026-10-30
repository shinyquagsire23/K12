#pragma once

// The per-creature "what changed" snapshot the server fills in and ships to the client.
#include "CExoArrayListDtors.h"

class CCombatInformation;
class CEffectIconObject;

class CSWCreatureStatsUpdate {
public:
    CExoArrayList<unsigned short> m_feats;
    CExoArrayList<unsigned short> m_bonusFeats;
    unsigned long long m_objectId;
    int m_nSkillsChanged;
    CExoArrayList<CEffectIconObject*> m_effectIcons;
    unsigned char m_anAbilityStats[6];
    unsigned char m_anAbilityBases[6];
    unsigned char m_anAbilityEffectBonuses[6];
    char m_nDexACBonus;
    unsigned char m_nFortSave;
    unsigned char m_nWillSave;
    unsigned char m_nReflexSave;
    unsigned short m_nReserved2;
    unsigned short m_nReserved3;
    unsigned char _pad5a[0x5c - 0x5a];
    int  m_nExperience;
    int  m_bMonkUnarmed;
    unsigned char m_nBaseAttackBonus;
    unsigned char _pad65;
    unsigned short m_nArmorClass;
    CCombatInformation* m_pCombatInformation;
    unsigned char* m_paSkillRanks;

    CSWCreatureStatsUpdate();
    ~CSWCreatureStatsUpdate();
    void SetCombatInformation(CCombatInformation* pCombatInformation);
    int  HasFeat(unsigned short nFeat);
    void ClearEffectIcons();
};

// One row of the creature's effect-icon strip, the status icons drawn next to the portrait.
#include "exoresman.h"

class CEffectIconObject {
public:
    short          m_nIconId;
    CResRef        m_resIcon;
    unsigned char  _pad12[0x14 - 0x12];
    int            m_bGood;
    unsigned short m_nPriority;
    unsigned char  _pad1a[0x1c - 0x1a];
    int            m_nGood;
    unsigned int   m_nNameStrRef;
};
