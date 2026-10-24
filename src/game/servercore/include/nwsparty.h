#pragma once

#include "CExoArrayListDtors.h"
#include "exostring.h"

class CFactionManager;

// One faction record, holding a parent faction id, a name, a global flag and a member list. Covers
// the fields the faction manager's save and load path touches.
class CSWSFaction {
public:
    CSWSFaction();
    CSWSFaction(unsigned long long gameObjectId);
    ~CSWSFaction();

    CExoArrayList<unsigned long long> m_members;
    int          m_nFactionId;
    int          m_nParentFactionID;
    int          m_nMemberIterator;
    char         _pad1c[0x04];
    CExoString   m_sName;
    int          m_nGlobal;
    char         _pad34[0x04];

    // Fans the text out to every connected member's client as a party chat message.
    void SendChatMessage(unsigned long long speakerId, const CExoString& sText);
    void AddMember(unsigned long long gameObjectId, int a);
    void RemoveMember(unsigned long long gameObjectId);
    // Handlers for the player-party messages.
    int  TransferLeadership(unsigned long long partyId, unsigned long long creatureId);
    int  InviteMember(unsigned long long partyId, unsigned long long creatureId);
    int  AttemptRemove(unsigned long long partyId, unsigned long long creatureId);
    int  GetMemberList(unsigned long long** ppMembers);
    // The object id of the member carrying the player-character latch, or the invalid sentinel when
    // there is none.
    unsigned long long GetLeader();
    // Faction-wide aggregate queries.
    int  GetAverageGoodEvilAlignment();
    int  GetAverageLevel();
    // The one aggregate query measured towards another object.
    int  GetAverageReputation(unsigned long long oidTarget);
    int  GetAverageXP();
    int  GetGold();
    int  GetMostFrequentClass();
    // Each member-picking query scans the member list with a visibility filter and answers a
    // member's object id, or the invalid sentinel when the scan finds nobody.
    unsigned long long GetBestAC(unsigned long long oidCreature, int bMustBeVisible);
    unsigned long long GetWorstAC(unsigned long long oidCreature, int bMustBeVisible);
    unsigned long long GetLeastDamagedMember(unsigned long long oidCreature, int bMustBeVisible);
    unsigned long long GetMostDamagedMember(unsigned long long oidCreature, int bMustBeVisible);
    unsigned long long GetStrongestMember(unsigned long long oidCreature, int bMustBeVisible);
    unsigned long long GetWeakestMember(unsigned long long oidCreature, int bMustBeVisible);
    // Iterates the faction members, returning the object id of the next member at or after
    // nStartIndex, optionally restricted to player characters.
    unsigned long long GetFactionMember(int nStartIndex, int bPCOnly);
    // Nonzero when the faction resolves to a single-member party.
    int  GetSingletonParty();
    // Whether the given object id is a member of this faction.
    int  GetCreatureInParty(unsigned long long gameObjectId);

    // The live faction manager.
    CFactionManager* GetFactionManager();
    // Ships a Party_List update to every roster member that resolves to a live client object.
    void SendFactionUpdateAdd(unsigned long long gameObjectId);
    void SendFactionUpdateRemove(unsigned long long gameObjectId);
    void SendFactionUpdateList();
    // Re-seeds the record to its just-created state: empty name, iterator at 0, global on.
    void Initialise();
    int  GetIsInvited(unsigned long long gameObjectId);
    void AttemptJoin(unsigned long long gameObjectId);
};
