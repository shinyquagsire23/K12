#include "k2stdint.h"
#include "nwcreaturelevelstats.h"
#include "nwsrules.h"
#include "CExoArrayListImpl.h"

#include "port/libcstring.h"

CSWLevelStats::CSWLevelStats()
{
    m_paSkillRankChanges = new char[g_pRules->m_nNumSkills];
    memset(m_paSkillRankChanges, 0, g_pRules->m_nNumSkills * sizeof(*m_paSkillRankChanges));
    m_nAbilityScoreIncrease = 6;
    m_nHitPointGain = 0;
    m_nClass = 0;
    m_nForcePointGain = 0;
    m_nSkillPoints = 0;
}

CSWLevelStats::~CSWLevelStats()
{
    if (m_paSkillRankChanges)
    {
        delete[] m_paSkillRankChanges;
        m_paSkillRankChanges = 0;
    }
    ClearFeats();
    m_spellsGained.SetSize(0);
    m_spellsLost.SetSize(0);
}

void CSWLevelStats::ClearFeats()
{
    m_featsGained.SetSize(0);
}

unsigned char CSWLevelStats::GetSkillRankChange(unsigned short nSkill)
{
    if (nSkill < g_pRules->m_nNumSkills)
        return m_paSkillRankChanges[nSkill];
    return 0;
}

void CSWLevelStats::AddFeat(unsigned short nFeat)
{
    if (!m_featsGained.Contains(nFeat))
        m_featsGained.Add(nFeat);
}

void CSWLevelStats::SetSkillRankChange(unsigned short nSkill, char nRank)
{
    if (nSkill < g_pRules->m_nNumSkills)
        m_paSkillRankChanges[nSkill] = nRank;
}
