#pragma once
// idiom-exempt: canonical

#include "exostring.h"
#include "CExoArrayListDtors.h"

// Server-side per-module faction and reputation table. Only pointers and ints, so forward
// declarations suffice for everything it names.
class CSWSFaction;
class CResGFF;
struct CResList;

class CFactionManager {
public:
    CFactionManager();
    ~CFactionManager();

    // Reputation bounds: below 11 is hostile, above 89 friendly, and 11 to 89 neutral.
    enum {
        REPUTATION_HOSTILE_BELOW  = 11,
        REPUTATION_FRIENDLY_ABOVE = 89,
    };

    // The seven built-in factions, resolved by name and allocated by CreateDefaultFactions.
    // "neutral" is the one dead and surrendering creatures join.
    CSWSFaction* m_pFactionNeutral;
    CSWSFaction* m_pFactionInsane;
    CSWSFaction* m_pFactionFriendly2;
    CSWSFaction* m_pFactionFriendly1;
    CSWSFaction* m_pFactionHostile1;
    CSWSFaction* m_pFactionHostile2;
    CSWSFaction* m_pFactionPlayer;

    int m_nNPCFactionCount;
    // The NPC faction-pair reputation matrix.
    CExoArrayList<int>*     m_paReputations;
    // Every faction, indexed by id.
    CExoArrayList<CSWSFaction*>* m_paFactions;

    CSWSFaction* GetFaction(int nFactionId);
    int  GetIsNPCFaction(int nFactionId);
    void SetNPCFactionReputation(int nFactionId1, int nFactionId2, int nRep);
    int  GetNPCFactionReputation(int nFactionId1, int nFactionId2);
    void DeleteFaction(int nFactionId);
    void ResolveFactionName(const CExoString& sName, CSWSFaction* pFaction);
    int GetFactionIdByName(const CExoString& sName);

    int SaveFactions(CResGFF* pGFF, CResList* pList);
    int  LoadReputations();
    int LoadReputationsFromSaveGame(CResGFF* pGFF, CResList* pList);
    int SaveReputations(CResGFF* pGFF, CResList* pList);
    int LoadFactions(int bLoadSpecial);
    int LoadFactionsFromSaveGame(CResGFF* pGFF, CResList* pList);
    void CreateDefaultFactions();
};
