#ifndef CSWPARTYTABLE_H
#define CSWPARTYTABLE_H
#include "k2stdint.h"

// Save and roster-persistence party table: NPC and puppet availability, influence, planet
// selectability, the party journal and the shared item repository. Distinct from CSWParty, the
// live in-module roster.

#include "exostring.h"
#include "Vector.h"

class CResGFF;
struct CResStruct;
class CItemRepository;
class CSWSCreature;
class CSWSJournal;

class CSWPartyTable {
public:
    // The NPC slots (KOTOR 1 has nine), the puppet slots and the galaxy map's planets.
    enum {
#if KOTOR1
        NUM_NPCS    = 9,
#else
        NUM_NPCS    = 12,
#endif
        NUM_PUPPETS = 3,
        NUM_PLANETS = 16,
    };

    CSWPartyTable();
    ~CSWPartyTable();
    void CreateParty();
    int  LoadTableInfo();
    void PositionParty();
    void ClearTable();
    void ClearMembers();
#if !KOTOR1
    void ClearPuppets();
    void ClearPuppet(int index);
#endif
    void ClearMember(int index);
    void ClearInventory();
    int  GetIsNPCAvailable(int index);
#if !KOTOR1
    int  GetIsPUPAvailable(int index);
    int  GetInfluence(int index);
    void SetInfluence(int index, int value);
#endif
    int  GetPlanetSelectable(unsigned int index);
    int  GetPlanetAvailable(unsigned int index);
    void SetPlanetSelectable(unsigned int index, int value);
    void SetPlanetAvailable(unsigned int index, int value);
    void SetSelectedPlanet(int index);
    void SetReturnStrref(int a, uint64_t b, uint64_t c);
    void SetNPCObject(int index, unsigned long long id);
#if !KOTOR1
    void SetPUPObject(int index, unsigned long long id);
#endif
    void SetNPCSelectability(int index, unsigned char selectable);
    unsigned char GetNPCSelectability(int index);
    int  RemoveNPC(int index);
#if !KOTOR1
    int  RemovePUP(int index);
#endif
    void SetPartyFollowState(int state);
    void SetSoloMode(int solo, int unstealth);
    void UnstealthParty(int a);
    int  GetIsMember(int position);
#if !KOTOR1
    int  GetIsPuppet(int position);
    unsigned long long GetNextPC();
    CExoString   GetPupFilename(int index);
#endif
    CExoString   GetFilename(int index);
    CSWSJournal* GetPartyJournal();
    void         SaveJournal(CResGFF* pGff, CResStruct* pStruct);
    void         LoadJournal(CResGFF* pGff, CResStruct* pStruct);
#if !KOTOR1
    int          GetPUPID(unsigned long long id);
#endif
    static void  ResetCurrentSessionStartTime();
    CItemRepository* GetPartyItems();
    unsigned char GetNumNPCAvailable();
    int          GetTotalPlayedSeconds();
    int          GetIsAvailable(unsigned long long id);
#if !KOTOR1
    int          AssignPUP(int pupIndex, int npcIndex);
#endif
    int          GetTotalPotentialLevels();
    int          GetTotalEffectiveCRForPotentialLevels();
    void         AutoLevelUpNPCs();
#if !KOTOR1
    int          KillPUPObject(int pupIndex, int notify);
#endif
    int          GetIsMember(unsigned long long id);
    void         RemoveGameInProgress();
    void         AddGameInProgress();
#if !KOTOR1
    CSWSCreature* GetPUPOwner(int pupIndex);
#endif
    int          GetNPCID(unsigned long long id);
    void         SetLeader(unsigned long long id);
    int          GetIsLeader(unsigned long long id);
    void         UpdateMembers(int value);
    void         SaveMember(int index, int value);
#if !KOTOR1
    void         SavePuppet(int index, int value);
    void         SaveMemberByObject(int index, unsigned long long id);
    void         SavePuppetByObject(int index, unsigned long long id);
#endif
    void         UpdateInventory(int value);
    void         SaveTableInfo(const CExoString& name);
    void         Save(const CExoString& name, int refresh);
    void         GivePlotXP(unsigned int row, int percent);
    void         GivePlotXP(const CExoString& row, int percent);
    void         DistributeExperience(int xp, int flag);
#if !KOTOR1
    int          AddPuppet(int index, unsigned long long id);
#endif
    int          AddMember(int index, unsigned long long id);
    int          AddNPC(int index, CSWSCreature* pCreature);
#if !KOTOR1
    int          AddPUP(int index, CSWSCreature* pCreature);
#endif
    int          AddNPC(int index, const CExoString& name);
#if !KOTOR1
    int          AddPUP(int index, const CExoString& name);
    CExoString   GetPCFirstName();
    CExoString   GetPCLastName();
#endif
    int          KillNPCObject(int npcIndex, int notify);
#if !KOTOR1
    int          RemovePuppet(int pupIndex);
#endif
    int          RemoveMember(int npcIndex);
    void         TransferInventory(CSWSCreature* pCreature);
    unsigned long long GetNPCObject(int index, int create, int notify);
#if !KOTOR1
    unsigned long long GetPUPObject(int index, int create, int notify);
    unsigned long long SpawnPUP(int index, int usePos, const Vector& pos, const Vector& orient, int notify);
#endif
    unsigned long long SpawnNPC(int index, int usePos, const Vector& pos, const Vector& orient, int notify);
    int          SwitchPlayerCharacter(int index);

#if KOTOR1
    // K1: no puppets, no influence, no components or chemicals, no swoop slots, and 9 NPC slots
    // instead of 12.
    int                 m_nNumPartyMembers;
    int                 m_partyMemberIndex[2];
    unsigned char       m_pad0c[4];
    unsigned long long  m_partyMemberID[9];
    int                 m_npcAvailable[9];
    unsigned char       m_npcSelectability[9];
    // The galaxy map's per-planet flags.
    struct {
        int             m_aAvailable[16];
        int             m_aSelectable[16];
    }                   m_planets;
    int                 m_selectedPlanet;
    int                 m_aiState;
    int                 m_followState;
    int                 m_nLeaderIndex;
    int                 m_nControlledNPC;
    int                 m_gameInProgressRefCount;
    int                 m_nXPPool;
    int                 m_nGold;
    int                 m_bShowReturn;
    unsigned long long  m_nReturnStrref;
    unsigned long long  m_nReturnQueryStrref;
    int                 m_bKeepSavedPositions;
    int                 m_bPartyCreated;
    int                 m_nTotalPlayedSecondsAccum;
    CItemRepository*    m_pItemRepository;
    CSWSJournal*        m_pJournal;
    int                 m_pazaakCards[18];
    int                 m_pazaakSideDeck[10];
    int                 m_soloMode;
    unsigned int        m_cheatUsedFlags;
#else
    int                 m_nNumPartyMembers;
    int                 m_nNumPuppets;
    int                 m_partyMemberIndex[2];
    int                 m_puppetIndex[2];
    int                 m_nThirdPuppetIndex;
    unsigned char       m_pad1c[4];
    unsigned long long  m_partyMemberID[12];
    int                 m_npcAvailable[12];
    unsigned char       m_npcSelectability[12];
    // The galaxy map's per-planet flags.
    struct {
        int             m_aAvailable[16];
        int             m_aSelectable[16];
    }                   m_planets;
    int                 m_selectedPlanet;
    int                 m_aiState;
    int                 m_followState;
    int                 m_nLeaderIndex;
    int                 m_nControlledNPC;
    int                 m_influence[12];
    unsigned long long  m_puppetID[3];
    int                 m_pupAvailable[3];
    unsigned char       m_pupSelectability[3];
    unsigned char       m_pad1a7;
    int                 m_gameInProgressRefCount;
    int                 m_nXPPool;
    int                 m_nGold;
    int                 m_nItemComponent;
    int                 m_nItemChemical;
    int                 m_swoop[6];
    int                 m_bShowReturn;
    unsigned long long  m_nReturnStrref;
    unsigned long long  m_nReturnQueryStrref;
    int                 m_bKeepSavedPositions;
    int                 m_bPartyCreated;
    int                 m_nTotalPlayedSecondsAccum;
    unsigned char       m_pad1f4[4];
    CItemRepository*    m_pItemRepository;
    CSWSJournal*        m_pJournal;
    int                 m_pazaakCards[23];
    int                 m_pazaakSideDeck[10];
    int                 m_soloMode;
    unsigned int        m_cheatUsedFlags;
    unsigned char       m_pad294[4];
    CExoString          m_saveTableName;
    int                 m_saveNumber;
    int                 m_forfeitViolations;
    int                 m_forfeitConditions;
    int                 m_nCurrentPCIndex;
    int                 m_disableMap;
    int                 m_disableRegen;
#endif
};

#endif
