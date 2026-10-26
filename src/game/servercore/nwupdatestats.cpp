#include "k2stdint.h"
#include "nwupdatestats.h"

#include "nwrules.h"
#include "nwsrules.h"

#include "port/libcstring.h"

void CSWCreatureStatsUpdate::SetCombatInformation(CCombatInformation* pCombatInformation) {
    *m_pCombatInformation = *pCombatInformation;
}

int CSWCreatureStatsUpdate::HasFeat(unsigned short nFeat) {
    for (int i = 0; i < m_feats.m_count; i++) {
        if (m_feats.m_pArray[i] == nFeat)
            return 1;
    }
    return 0;
}

CSWCreatureStatsUpdate::CSWCreatureStatsUpdate() {
    // Every seed is the -20 "no change" sentinel; fields not seeded here are left uninitialised.
    __builtin_memset(&m_anAbilityStats, -20,
                     sizeof(m_anAbilityStats) + sizeof(m_anAbilityBases) +
                         sizeof(m_anAbilityEffectBonuses));
    m_nDexACBonus = -20;
    m_nFortSave = static_cast<unsigned char>(-20);
    m_nWillSave = static_cast<unsigned char>(-20);
    m_nReflexSave = static_cast<unsigned char>(-20);
    m_nReserved2 = static_cast<unsigned short>(-20);
    m_nReserved3 = static_cast<unsigned short>(-20);
    m_nExperience = -20;
    m_bMonkUnarmed = 0;
    m_nArmorClass = static_cast<unsigned short>(-20);
    m_paSkillRanks = 0;
    m_pCombatInformation = new CCombatInformation();
    // The rank array is sized by skills.2da's row count and pre-filled with the same sentinel. An
    // empty table leaves m_paSkillRanks at the 0 written above rather than allocating a
    // zero-length block.
    if (g_pRules->m_nNumSkills != 0) {
        unsigned char* pRanks = new unsigned char[g_pRules->m_nNumSkills];
        m_paSkillRanks = pRanks;
        __builtin_memset(pRanks, 0xec, g_pRules->m_nNumSkills);  // idiom-exempt: runtime length -- the skills.2da row count; the orig emits a real tail `b memset`
    }
}

void CSWCreatureStatsUpdate::ClearEffectIcons() {
    // Each iteration erases its own slot, so the walk runs back to front. DelIndex leaves the
    // vacated slot alone when it is the last one, so it is nulled here.
    for (int i = m_effectIcons.m_count - 1; i >= 0; i--) {
        if (m_effectIcons.m_pArray[i] != 0) {
            delete m_effectIcons.m_pArray[i];
            m_effectIcons.m_pArray[i] = 0;
        }
        m_effectIcons.DelIndex(i);
    }
}

CSWCreatureStatsUpdate::~CSWCreatureStatsUpdate() {
    // The feat lists are only cleared; their arrays are released by the members' own destructors.
    m_feats.m_count = 0;
    m_bonusFeats.m_count = 0;
    delete[] m_paSkillRanks;
    if (m_pCombatInformation != 0) {
        delete m_pCombatInformation;
        m_pCombatInformation = 0;
    }
    ClearEffectIcons();
}
