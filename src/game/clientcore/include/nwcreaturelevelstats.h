#pragma once

#include "CExoArrayListDtors.h"

// One level's worth of pending and applied character advancement. CSWSCreatureStats keeps one of
// these per character level in a CSWLevelStats** array, and LevelUp/LevelDown fold a row into or
// out of the live stats block.
//
// Only the fields the engine uses are named; everything else is padding.
class CSWLevelStats {
public:
    // The first three are what the level granted (the gained lists) and what it replaced (the lost
    // spell list, re-added by LevelDown).
    CExoArrayList<unsigned int>   m_spellsGained;
    CExoArrayList<unsigned int>   m_spellsLost;
    CExoArrayList<unsigned short> m_featsGained;
    // The per-skill rank-delta array, heap-allocated by the ctor and sized by the rules' skill
    // count (skills.2da rows); GetSkillRankChange indexes it.
    char*          m_paSkillRankChanges;
    unsigned short m_nSkillPoints;                // purpose unknown on both ends
    unsigned char  m_nAbilityScoreIncrease;  // the score the level raises: 0..5 = STR..CHA, 6 = none
    unsigned char  m_nHitPointGain;          // folded into CSWSObject::m_hitPoints
    unsigned char  m_nClass;                 // the class id the level was taken in
    unsigned char  m_nForcePointGain;        // accumulated into CSWSCreatureStats::m_nBaseForcePoints
    unsigned char _pad3e[0x40 - 0x3e];       // sizeof is a guess here

    void ClearFeats();
    void SetSkillRankChange(unsigned short nSkill, char nRank);
    void AddFeat(unsigned short nFeat);

    // nClass indexes the per-class known-spell lists, of which a level has exactly one.
    void AddAddedKnownSpell(unsigned char nClass, unsigned int nSpell) { (&m_spellsGained)[nClass].Add(nSpell); }
    void AddRemovedKnownSpell(unsigned char nClass, unsigned int nSpell) { (&m_spellsLost)[nClass].Add(nSpell); }

    // The level's per-skill rank delta; both LevelUp and LevelDown call it twice per skill, once
    // as the gate and once for the delta itself.
    unsigned char GetSkillRankChange(unsigned short nSkill);

    // Defined outside this header: CSWSCreatureStats allocates one per level.
    CSWLevelStats();

    // Defined outside this header: CSWSCreatureStats deletes the level array's entries.
    ~CSWLevelStats();
};
