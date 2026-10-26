#include "k2stdint.h"
#include "Achievements.h"
#include "nwccreature.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwsplayer.h"
#include "nwsarea.h"
#include "nwsmodule.h"
#include "nwcobject.h"
#include "nwsitem.h"
#include "nwsparty.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#ifndef CSWPARTYTABLE_INTERNAL_H
#define CSWPARTYTABLE_INTERNAL_H

// Minimal views of the classes this unit touches but does not own; only the members used here are
// declared.
#include "partytables.h"

#include "exostring.h"

#include "Vector.h"
class CSWSRoom;
class CPathfindInformation;

#include "port/win32api.h"

void HandleLoadError(uint64_t strRef);

#include "gameobjectarray.h"
#include "nwsjournal.h"
#include "swparty.h"

#include "exoresman.h"

#include "swguiexamine.h"
#include "clientexoapp.h"

#include "nwsrules.h"

#include "nwitem.h"

#include "resgff.h"
#include "restype.h"
#include "nwscreature.h"
#include "nwscriptconst.h"
#include "strref.h"

// The type a feedback or combat-log line is filed under (CGuiInGame's message buffers).
enum {
    kMessageTypeDebug = 0x10000000,
};

// The galaxy map's saved planet mask: bit i is planet i's availability, bit 16 + i its
// selectability.
enum {
    PLANET_MASK_SELECTABLE_BIT0 = 1 << 16,
};

#include "factionmanager.h"

#include "SWCollisionMesh.h"

float Yaw(const Vector& orientation);
float DegToRad(float degrees);

#include "serveraimaster.h"
#include "serverexoapp.h"

#include "appmanager.h"

extern unsigned long long g_nCurrentSessionStartFILETIME;

extern int g_nJournalSortType;

#endif
#include "nwmessage.h"
#include "clientoptions.h"
#include <math.h>

#include "port/libcstring.h"
#include "exostringinline.h"

extern CAppManager* g_pAppManager;
CSWRules* g_pRules;

CSWPartyTable::CSWPartyTable()
{
    m_pItemRepository = 0;
    m_pJournal = 0;
    ClearTable();
}

CSWPartyTable::~CSWPartyTable()
{
    CItemRepository* pItemRepo = m_pItemRepository;
    if (pItemRepo)
    {
        pItemRepo->~CItemRepository();
        operator delete(pItemRepo);
        m_pItemRepository = 0;
    }
    CSWSJournal* pJournal = m_pJournal;
    if (pJournal)
    {
        pJournal->~CSWSJournal();
        operator delete(pJournal);
        m_pJournal = 0;
    }
}

// Resets every field to its spawn value, ending with the member and puppet lists.
void CSWPartyTable::ClearTable()
{
#if KOTOR1
    __builtin_memset(&m_planets, 0, sizeof(m_planets));
    m_nLeaderIndex = -1;
    m_nControlledNPC = -1;
    m_gameInProgressRefCount = 0;
    m_nXPPool = 0;
    m_nGold = 0;
    m_bShowReturn = 1;
    m_nReturnStrref = STRREF_RETURN_TO_SPACEPORT;
    m_nReturnQueryStrref = STRREF_RETURN_TO_EBON_HAWK;
    m_bKeepSavedPositions = 0;
    m_nTotalPlayedSecondsAccum = 0;
    m_selectedPlanet = -1;
    m_soloMode = 0;
    m_cheatUsedFlags = 0;

    CItemRepository* pItemRepo = m_pItemRepository;
    if (pItemRepo)
    {
        pItemRepo->~CItemRepository();
        operator delete(pItemRepo);
        m_pItemRepository = 0;
    }

    m_bPartyCreated = 0;
    m_nNumPartyMembers = 0;
    m_aiState = 0;
    m_followState = 0;
    for (int i = 0; i < 9; i++)
    {
        m_npcSelectability[i] = 1;
        m_npcAvailable[i] = 0;
    }
    ClearMembers();
    for (int i = 0; i < 5; i++)
        m_pazaakCards[i] = 2;
    for (int i = 5; i < 18; i++)
        m_pazaakCards[i] = 0;
    __builtin_memset(m_pazaakSideDeck, 0xff, sizeof(m_pazaakSideDeck));

    CSWSJournal* pJournal = m_pJournal;
    if (pJournal)
    {
        pJournal->~CSWSJournal();
        operator delete(pJournal);
        m_pJournal = 0;
    }
#else
    __builtin_memset(m_planets.m_aAvailable, 0, sizeof(m_planets.m_aAvailable));
    __builtin_memset(m_planets.m_aSelectable, 0, sizeof(m_planets.m_aSelectable));
    m_nLeaderIndex = -1;
    m_gameInProgressRefCount = 0;
    m_nXPPool = 0;
    m_nGold = 0;
    m_nItemComponent = 0;
    m_nItemChemical = 0;
    m_bShowReturn = 1;
    m_nReturnStrref = STRREF_RETURN_TO_SPACEPORT;
    m_nReturnQueryStrref = STRREF_RETURN_TO_EBON_HAWK;
    m_bKeepSavedPositions = 0;
    m_nTotalPlayedSecondsAccum = 0;
    m_nCurrentPCIndex = 0;
    m_selectedPlanet = -1;
    m_soloMode = 0;
    m_cheatUsedFlags = 0;

    CItemRepository* pItemRepo = m_pItemRepository;
    if (pItemRepo)
    {
        pItemRepo->~CItemRepository();
        operator delete(pItemRepo);
        m_pItemRepository = 0;
    }

    m_nControlledNPC = -1;
    m_bPartyCreated = 0;
    m_npcAvailable[2] = 0;
    m_nNumPartyMembers = 0;
    m_nNumPuppets = 0;
    m_aiState = 0;
    m_followState = 0;
    m_npcSelectability[0] = 1;
    m_npcSelectability[1] = 1;
    m_npcAvailable[0] = 0;
    m_npcAvailable[1] = 0;
    m_influence[0] = -1;
    m_influence[1] = -1;
    m_npcSelectability[2] = 1;
    m_npcSelectability[3] = 1;
    m_npcSelectability[4] = 1;
    m_npcSelectability[5] = 1;
    ClearMembers();
    m_influence[2] = -1;
    m_influence[3] = -1;
    m_influence[4] = -1;
    m_influence[5] = -1;
    m_influence[6] = -1;
    m_influence[7] = -1;
    m_influence[8] = -1;
    m_influence[9] = -1;
    m_npcSelectability[6] = 1;
    m_npcSelectability[7] = 1;
    m_npcSelectability[8] = 1;
    m_npcSelectability[9] = 1;
    m_npcAvailable[3] = 0;
    m_npcAvailable[4] = 0;
    m_npcAvailable[5] = 0;
    m_npcAvailable[6] = 0;
    m_npcAvailable[7] = 0;
    m_npcAvailable[8] = 0;
    m_npcAvailable[9] = 0;
    m_npcAvailable[10] = 0;
    m_npcSelectability[10] = 1;
    m_npcSelectability[11] = 1;
    m_npcAvailable[11] = 0;
    m_influence[10] = -1;
    m_influence[11] = -1;
    m_pupSelectability[0] = 1;
    m_pupAvailable[0] = 0;
    m_pupAvailable[1] = 0;
    m_pupSelectability[1] = 1;
    m_pupSelectability[2] = 1;
    ClearPuppets();
    m_pupAvailable[2] = 0;
    m_puppetIndex[0] = -1;
    m_puppetIndex[1] = -1;
    m_nThirdPuppetIndex = -1;
    for (int i = 1; i < 5; i++)
        m_pazaakCards[i] = 1;
    for (int i = 5; i < 9; i++)
        m_pazaakCards[i] = 1;
    m_pazaakCards[0] = 1;
    m_pazaakCards[9] = 1;
    __builtin_memset(&m_pazaakCards[10], 0, 8 * sizeof(m_pazaakCards[0]));
    for (int i = 18; i < 22; i++)
        m_pazaakCards[i] = 0;
    m_pazaakCards[22] = 0;
    __builtin_memset(m_pazaakSideDeck, 0xff, sizeof(m_pazaakSideDeck));

    CSWSJournal* pJournal = m_pJournal;
    if (pJournal)
    {
        pJournal->~CSWSJournal();
        operator delete(pJournal);
        m_pJournal = 0;
    }

    m_forfeitConditions = 0;
    m_saveNumber = 0;
    m_forfeitViolations = 0;
    m_disableMap = 0;
    m_disableRegen = 0;
#endif
}

void CSWPartyTable::ClearMembers()
{
    m_partyMemberID[0] = OBJECT_INVALID;
    m_partyMemberID[1] = OBJECT_INVALID;
    m_partyMemberID[2] = OBJECT_INVALID;
    m_partyMemberID[3] = OBJECT_INVALID;
    m_partyMemberID[4] = OBJECT_INVALID;
    m_partyMemberID[5] = OBJECT_INVALID;
    m_partyMemberID[6] = OBJECT_INVALID;
    m_partyMemberID[7] = OBJECT_INVALID;
    m_partyMemberID[8] = OBJECT_INVALID;
#if !KOTOR1
    m_partyMemberID[9] = OBJECT_INVALID;
    m_partyMemberID[10] = OBJECT_INVALID;
    m_partyMemberID[11] = OBJECT_INVALID;
#endif
}

#if !KOTOR1
void CSWPartyTable::ClearPuppets()
{
    m_puppetID[0] = OBJECT_INVALID;
    m_puppetID[1] = OBJECT_INVALID;
    m_puppetID[2] = OBJECT_INVALID;
}
#endif

#if !KOTOR1
void CSWPartyTable::ClearPuppet(int index)
{
    if (static_cast<unsigned int>(index) <= 2)
        m_puppetID[index] = OBJECT_INVALID;
}
#endif

void CSWPartyTable::ClearMember(int index)
{
    if (static_cast<unsigned int>(index) < NUM_NPCS)
        m_partyMemberID[index] = OBJECT_INVALID;
}

void CSWPartyTable::ClearInventory()
{
    if (m_pItemRepository)
        m_pItemRepository->ClearItems();
    m_bPartyCreated = 0;
}

int CSWPartyTable::GetIsNPCAvailable(int index)
{
    if (static_cast<unsigned int>(index) >= NUM_NPCS)
        return 0;
    return m_npcAvailable[index];
}

#if !KOTOR1
int CSWPartyTable::GetIsPUPAvailable(int index)
{
    if (static_cast<unsigned int>(index) > 2)
        return 0;
    return m_pupAvailable[index];
}
#endif

#if !KOTOR1
int CSWPartyTable::GetInfluence(int index)
{
    if (static_cast<unsigned int>(index) >= NUM_NPCS)
        return 0;
    return m_influence[index];
}
#endif

#if !KOTOR1
void CSWPartyTable::SetInfluence(int index, int value)
{
    if (static_cast<unsigned int>(index) < NUM_NPCS)
    {
        m_influence[index] = value;
        if (value == 0)
            Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_INFLUENCE_ZERO);
    }
}
#endif

int CSWPartyTable::GetPlanetSelectable(unsigned int index)
{
    if (index < NUM_PLANETS)
        return m_planets.m_aSelectable[index];
    return 0;
}

int CSWPartyTable::GetPlanetAvailable(unsigned int index)
{
    if (index < NUM_PLANETS)
        return m_planets.m_aAvailable[index];
    return 0;
}

void CSWPartyTable::SetPlanetSelectable(unsigned int index, int value)
{
    if (index < NUM_PLANETS)
        m_planets.m_aSelectable[index] = value;
}

void CSWPartyTable::SetPlanetAvailable(unsigned int index, int value)
{
    if (index < NUM_PLANETS)
        m_planets.m_aAvailable[index] = value;
}

void CSWPartyTable::SetSelectedPlanet(int index)
{
#if KOTOR1
    if (static_cast<unsigned int>(index) < NUM_PLANETS && m_planets.m_aAvailable[index] != 0 && m_planets.m_aSelectable[index] != 0)
#else
    if (static_cast<unsigned int>(index) < NUM_PLANETS && m_planets.m_aAvailable[index] != 0)
#endif
        m_selectedPlanet = index;
}

void CSWPartyTable::SetReturnStrref(int a, uint64_t b, uint64_t c)
{
    m_bShowReturn = a;
    m_nReturnStrref = b;
    m_nReturnQueryStrref = c;
}

void CSWPartyTable::SetNPCObject(int index, unsigned long long id)
{
    if (static_cast<unsigned int>(index) < NUM_NPCS && m_npcAvailable[index] != 0)
        m_partyMemberID[index] = id;
}

#if !KOTOR1
void CSWPartyTable::SetPUPObject(int index, unsigned long long id)
{
    if (static_cast<unsigned int>(index) <= 2 && m_pupAvailable[index] != 0)
        m_puppetID[index] = id;
}
#endif

void CSWPartyTable::SetNPCSelectability(int index, unsigned char selectable)
{
    if (static_cast<unsigned int>(index) < NUM_NPCS && m_npcAvailable[index] != 0)
        m_npcSelectability[index] = selectable;
}

unsigned char CSWPartyTable::GetNPCSelectability(int index)
{
    if (static_cast<unsigned int>(index) < NUM_NPCS && m_npcAvailable[index] != 0)
        return m_npcSelectability[index];
    return 0xff;
}

int CSWPartyTable::RemoveNPC(int index)
{
    if (static_cast<unsigned int>(index) < NUM_NPCS && m_npcAvailable[index] != 0)
    {
        m_npcAvailable[index] = 0;
        return 1;
    }
    return 0;
}

#if !KOTOR1
int CSWPartyTable::RemovePUP(int index)
{
    if (static_cast<unsigned int>(index) <= 2 && m_pupAvailable[index] != 0)
    {
        m_pupAvailable[index] = 0;
        return 1;
    }
    return 0;
}
#endif

void CSWPartyTable::SetPartyFollowState(int state)
{
    K2_UNUSED(state);
    m_followState = 0;
    g_pAppManager->m_pClientExoApp->GetSWParty()->FollowStateChanged();
}

void CSWPartyTable::UnstealthParty(int a)
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    unsigned long long playerCreatureId = pServerApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer = pServerApp->GetClientObjectByObjectId(playerCreatureId);
    if (pPlayer != 0)
    {
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
        if (pCreature != 0)
            pCreature->SetActivity(CSWSCreature::ACTIVITY_STEALTH, 0, a);
    }
    for (unsigned char i = 0; i < m_nNumPartyMembers; i++)
    {
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[m_partyMemberIndex[i]]);
        if (pCreature != 0)
            pCreature->SetActivity(CSWSCreature::ACTIVITY_STEALTH, 0, a);
    }
}

void CSWPartyTable::SetSoloMode(int solo, int unstealth)
{
    if (solo == 0 && unstealth != 0)
        UnstealthParty(0);
    m_soloMode = solo;
}

int CSWPartyTable::GetIsMember(int position)
{
    for (int i = 0; i < m_nNumPartyMembers; i++)
    {
        if (m_partyMemberIndex[i] == position)
            return 1;
    }
    return 0;
}

#if !KOTOR1
int CSWPartyTable::GetIsPuppet(int position)
{
    for (int i = 0; i < m_nNumPuppets; i++)
    {
        if (m_puppetIndex[i] == position)
            return 1;
    }
    return 0;
}
#endif

#if !KOTOR1
CExoString CSWPartyTable::GetPupFilename(int index)
{
    CExoString result;
    result.Format(const_cast<char*>("AVAILPUP%d"), index);
    return result;
}
#endif

CExoString CSWPartyTable::GetFilename(int index)
{
    CExoString result;
    result.Format(const_cast<char*>("AVAILNPC%d"), index);
    return result;
}

CSWSJournal* CSWPartyTable::GetPartyJournal()
{
    if (m_pJournal == 0)
        m_pJournal = new CSWSJournal();
    return m_pJournal;
}

void CSWPartyTable::SaveJournal(CResGFF* pGff, CResStruct* pStruct)
{
    CSWSJournal* pJournal = m_pJournal;
    if (pJournal == 0)
    {
        pJournal = new CSWSJournal();
        m_pJournal = pJournal;
    }
    if (pJournal->m_entries.m_count != 0)
    {
        pGff->WriteFieldINT(pStruct, g_nJournalSortType, const_cast<char*>("JNL_SortOrder"));

        CResList entryList;
        pGff->AddList(&entryList, pStruct, const_cast<char*>("JNL_Entries"));

        for (int i = 0; i < pJournal->m_entries.m_count; i++)
        {
            CResStruct elemStruct;
            pGff->AddListElement(&elemStruct, &entryList, 0);
            pGff->WriteFieldCExoString(&elemStruct, pJournal->m_entries.m_pArray[i].m_sPlotId, const_cast<char*>("JNL_PlotID"));
            pGff->WriteFieldINT(&elemStruct, pJournal->m_entries.m_pArray[i].m_nState, const_cast<char*>("JNL_State"));
            pGff->WriteFieldDWORD(&elemStruct, pJournal->m_entries.m_pArray[i].m_nDate, const_cast<char*>("JNL_Date"));
            pGff->WriteFieldDWORD(&elemStruct, pJournal->m_entries.m_pArray[i].m_nTime, const_cast<char*>("JNL_Time"));
        }
    }
}

void CSWPartyTable::LoadJournal(CResGFF* pGff, CResStruct* pStruct)
{
    int state;
    unsigned int date;
    unsigned int time;
    GetPartyJournal();

    CResList entryList;
    if (pGff->GetList(&entryList, pStruct, const_cast<char*>("JNL_Entries")))
    {
        int bSuccess;
        g_nJournalSortType = pGff->ReadFieldINT(pStruct, const_cast<char*>("JNL_SortOrder"), bSuccess, g_nJournalSortType);

        CExoString plotId;
        int count = pGff->GetListCount(&entryList);
        for (int i = 0; i < count; i++)
        {
            CResStruct elemStruct;
            pGff->GetListElement(&elemStruct, &entryList, i);

            plotId = pGff->ReadFieldCExoString(&elemStruct, const_cast<char*>("JNL_PlotID"), bSuccess, CExoString(""));

            state = pGff->ReadFieldINT(&elemStruct, const_cast<char*>("JNL_State"), bSuccess, 0);
            GetPartyJournal()->SetState(plotId, state, 1);

            date = pGff->ReadFieldDWORD(&elemStruct, const_cast<char*>("JNL_Date"), bSuccess, 0);
            GetPartyJournal()->SetDate(plotId, date, 1);

            time = pGff->ReadFieldDWORD(&elemStruct, const_cast<char*>("JNL_Time"), bSuccess, 0);
            GetPartyJournal()->SetTime(plotId, time, 1);
        }
    }
}

#if !KOTOR1
int CSWPartyTable::GetPUPID(unsigned long long id)
{
    if (id == OBJECT_INVALID)
        return -1;
    if (m_puppetID[0] == id)
        return 0;
    if (m_puppetID[1] == id)
        return 1;
    return (m_puppetID[2] == id) ? 2 : -1;
}
#endif

void CSWPartyTable::ResetCurrentSessionStartTime()
{
    unsigned long long fileTime;
    GetSystemTimeAsFileTime(&fileTime);
    g_nCurrentSessionStartFILETIME = fileTime;
}

CItemRepository* CSWPartyTable::GetPartyItems()
{
    if (m_pItemRepository == 0)
        m_pItemRepository = new CItemRepository(g_pAppManager->m_pServerExoApp->GetPlayerCreatureId(), 1);
    return m_pItemRepository;
}

unsigned char CSWPartyTable::GetNumNPCAvailable()
{
    int count = 0;
#if KOTOR1
    for (int i = 0; i < 9; i++)
#else
    for (int i = 0; i < 12; i++)
#endif
        if (m_npcAvailable[i] != 0)
            count++;
    return static_cast<unsigned char>(count);
}

int CSWPartyTable::GetTotalPlayedSeconds()
{
    unsigned long long now;
    GetSystemTimeAsFileTime(&now);
    long long elapsed = static_cast<long long>(now - g_nCurrentSessionStartFILETIME);
    if (elapsed < 0)
        elapsed = 0;
    return m_nTotalPlayedSecondsAccum + static_cast<int>((elapsed + 5000000) / 10000000);
}

int CSWPartyTable::GetIsAvailable(unsigned long long id)
{
#if KOTOR1
    for (int i = 0; i < 9; i++)
#else
    for (int i = 0; i < 12; i++)
#endif
        if (m_partyMemberID[i] == id)
            return 1;
    return 0;
}

#if !KOTOR1
int CSWPartyTable::AssignPUP(int pupIndex, int npcIndex)
{
    if (static_cast<unsigned>(pupIndex) >= 3 || static_cast<unsigned>(npcIndex) >= 12)
        return 0;
    if (m_npcAvailable[npcIndex] == 0)
        return 0;
    if (m_pupAvailable[pupIndex] == 0)
        return 0;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[npcIndex]);
    if (pCreature == 0)
        return 0;
    pCreature->m_nAssignedPup = pupIndex;
    return 1;
}
#endif

int CSWPartyTable::GetTotalPotentialLevels()
{
    int total = 0;
    for (int i = 0; i < m_nNumPartyMembers; i++)
    {
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[m_partyMemberIndex[i]]);
        if (pCreature != 0)
            total += pCreature->m_pStats->GetPotentialLevel();
    }
    return total;
}

int CSWPartyTable::GetTotalEffectiveCRForPotentialLevels()
{
    int total = 0;
    for (int i = 0; i < m_nNumPartyMembers; i++)
    {
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[m_partyMemberIndex[i]]);
        if (pCreature != 0)
            total += pCreature->m_pStats->GetEffectiveCRForPotentialLevel();
    }
    return total;
}

void CSWPartyTable::AutoLevelUpNPCs()
{
    for (int i = 0; i < m_nNumPartyMembers; i++)
    {
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[m_partyMemberIndex[i]]);
        if (pCreature != 0 && pCreature->m_pStats->CanLevelUp() != 0)
            pCreature->m_pStats->AutoLevelup(1);
    }
}

#if !KOTOR1
int CSWPartyTable::KillPUPObject(int pupIndex, int notify)
{
    if (static_cast<unsigned>(pupIndex) >= 3)
        return 0;
    long long puppetCount = m_nNumPuppets;
    if (puppetCount >= 1)
    {
        int* pIndex = m_puppetIndex;
        do
        {
            if (*pIndex == pupIndex)
                return 0;
            puppetCount--;
            pIndex++;
        } while (puppetCount != 0);
    }

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_puppetID[pupIndex]);
    m_puppetID[pupIndex] = OBJECT_INVALID;
    if (pCreature == 0)
        return 0;

    if (notify != 0)
    {
        CSWCObject* pClientObj = static_cast<CSWCObject*>(pCreature->GetClientCreature());
        if (pClientObj != 0)
        {
            pClientObj->SetFadeState(0);
            pClientObj->SetDesiredFadeState(0, 1, 0, 0);
            pClientObj->m_bPendingRemoval = 1;
        }
    }
    delete pCreature;
    return 1;
}
#endif

int CSWPartyTable::GetIsMember(unsigned long long id)
{
    int index;
    if (m_partyMemberID[0] == id) index = 0;
    else if (m_partyMemberID[1] == id) index = 1;
    else if (m_partyMemberID[2] == id) index = 2;
    else if (m_partyMemberID[3] == id) index = 3;
    else if (m_partyMemberID[4] == id) index = 4;
    else if (m_partyMemberID[5] == id) index = 5;
    else if (m_partyMemberID[6] == id) index = 6;
    else if (m_partyMemberID[7] == id) index = 7;
#if KOTOR1
    else if (m_partyMemberID[8] == id) index = 8;
    else return 0;
#else
    else if (m_partyMemberID[8] == id) index = 8;
    else if (m_partyMemberID[9] == id) index = 9;
    else if (m_partyMemberID[10] == id) index = 10;
    else if (m_partyMemberID[11] == id) index = 11;
    else return 0;
#endif

    for (int i = 0; i < m_nNumPartyMembers; i++)
        if (m_partyMemberIndex[i] == index)
            return 1;
    return 0;
}

void CSWPartyTable::RemoveGameInProgress()
{
    if (--m_gameInProgressRefCount == 0)
    {
        CExoResMan* pResMan = g_pExoResMan;
        CExoString dir("GAMEINPROGRESS:");
        pResMan->RemoveResourceDirectory(dir);
    }
}

void CSWPartyTable::AddGameInProgress()
{
    if (m_gameInProgressRefCount == 0)
    {
        CExoResMan* pResMan = g_pExoResMan;
        CExoString dir("GAMEINPROGRESS:");
        pResMan->AddResourceDirectory(dir);
    }
    m_gameInProgressRefCount++;
}

#if !KOTOR1
// Which creature owns puppet slot pupIndex: the two party slots first, then the player.
// An empty slot (-1) is skipped; a slot holding an out-of-range or unavailable NPC is
// still looked up, as OBJECT_INVALID.
CSWSCreature* CSWPartyTable::GetPUPOwner(int pupIndex)
{
    if (static_cast<unsigned>(pupIndex) > 2)
        return 0;

    CSWSCreature* pCreature;
    int pos0 = m_partyMemberIndex[0];
    if (pos0 != -1)
    {
        unsigned long long id0 = OBJECT_INVALID;
        if (static_cast<unsigned>(pos0) < NUM_NPCS && m_npcAvailable[pos0] != 0)
            id0 = m_partyMemberID[pos0];
        pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id0);
        if (pCreature != 0 && pCreature->m_nAssignedPup == pupIndex)
            return pCreature;
    }

    int pos1 = m_partyMemberIndex[1];
    if (pos1 != -1)
    {
        unsigned long long id1 = OBJECT_INVALID;
        if (static_cast<unsigned>(pos1) < NUM_NPCS && m_npcAvailable[pos1] != 0)
            id1 = m_partyMemberID[pos1];
        pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id1);
        if (pCreature != 0 && pCreature->m_nAssignedPup == pupIndex)
            return pCreature;
    }

    pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    if (pCreature != 0 && pCreature->m_nAssignedPup == pupIndex)
        return pCreature;

    return 0;
}
#endif

void CSWPartyTable::SaveTableInfo(const CExoString& name)
{
#if KOTOR1
    // K1 has no planets, puppets, influence or status summary fields, and its path
    // separator is a backslash.
    CExoString saveName;
    CResGFF* pGff;
    CResStruct topStruct;
    CResStruct struct1;
    CResList list1;
    unsigned int planetMask;
    int i;
    int created;

    pGff = new CResGFF();
    {
        CExoString type("PT  ");
        CExoString version("V2.0");
        created = pGff->CreateGFFFile(&topStruct, type, version);
    }
    if (created == 1)
    {
        saveName = name + "/" + "PARTYTABLE";

        m_nTotalPlayedSecondsAccum = GetTotalPlayedSeconds();
        ResetCurrentSessionStartTime();

        pGff->WriteFieldDWORD(&topStruct, m_nGold, const_cast<char*>("PT_GOLD"));
        pGff->WriteFieldINT(&topStruct, m_nXPPool, const_cast<char*>("PT_XP_POOL"));
        pGff->WriteFieldDWORD(&topStruct, m_nTotalPlayedSecondsAccum, const_cast<char*>("PT_PLAYEDSECONDS"));
        pGff->WriteFieldINT(&topStruct, m_nControlledNPC, const_cast<char*>("PT_CONTROLLED_NPC"));
        pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_soloMode), const_cast<char*>("PT_SOLOMODE"));
        pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_cheatUsedFlags), const_cast<char*>("PT_CHEAT_USED"));
        pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_nNumPartyMembers), const_cast<char*>("PT_NUM_MEMBERS"));

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_MEMBERS"));
        for (i = 0; i < m_nNumPartyMembers; i++)
        {
            int memberIndex = m_partyMemberIndex[i];
            int leaderIndex = m_nLeaderIndex;
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldINT(&struct1, m_partyMemberIndex[i], const_cast<char*>("PT_MEMBER_ID"));
            pGff->WriteFieldBYTE(&struct1, memberIndex == leaderIndex, const_cast<char*>("PT_IS_LEADER"));
        }

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_AVAIL_NPCS"));
        for (i = 0; i < 9; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldBYTE(&struct1, static_cast<unsigned char>(m_npcAvailable[i]), const_cast<char*>("PT_NPC_AVAIL"));
            pGff->WriteFieldBYTE(&struct1, m_npcSelectability[i], const_cast<char*>("PT_NPC_SELECT"));
        }

        pGff->WriteFieldINT(&topStruct, m_aiState, const_cast<char*>("PT_AISTATE"));
        pGff->WriteFieldINT(&topStruct, m_followState, const_cast<char*>("PT_FOLLOWSTATE"));

        planetMask = 0;
        for (i = 0; i < NUM_PLANETS; i++)
        {
            if (m_planets.m_aAvailable[i] != 0)
                planetMask |= 1 << i;
            if (m_planets.m_aSelectable[i] != 0)
                planetMask |= PLANET_MASK_SELECTABLE_BIT0 << i;
        }
        pGff->AddStructToStruct(&struct1, &topStruct, const_cast<char*>("GlxyMap"), 0);
        pGff->WriteFieldDWORD(&struct1, NUM_PLANETS, const_cast<char*>("GlxyMapNumPnts"));
        pGff->WriteFieldDWORD(&struct1, planetMask, const_cast<char*>("GlxyMapPlntMsk"));
        pGff->WriteFieldINT(&struct1, m_selectedPlanet, const_cast<char*>("GlxyMapSelPnt"));

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_PAZAAKCARDS"));
        for (i = 0; i < 18; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldINT(&struct1, m_pazaakCards[i], const_cast<char*>("PT_PAZAAKCOUNT"));
        }
        pGff->AddListElement(&struct1, &list1, 0);
        pGff->WriteFieldBYTE(&struct1, static_cast<unsigned char>(m_cheatUsedFlags), const_cast<char*>("PT_PAZAAKCOUNT"));

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_PAZSIDELIST"));
        for (i = 0; i < 10; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldINT(&struct1, m_pazaakSideDeck[i], const_cast<char*>("PT_PAZSIDECARD"));
        }

        pGff->WriteFieldVOID(&topStruct, g_pAppManager->m_pClientExoApp->GetInGameGui()->m_tutorialWindowShown, sizeof(CGuiInGame::m_tutorialWindowShown), const_cast<char*>("PT_TUT_WND_SHOWN"));
        pGff->WriteFieldINT(&topStruct, g_pAppManager->m_pClientExoApp->GetInGameGui()->m_lastGuiPanel, const_cast<char*>("PT_LAST_GUI_PNL"));

        CGuiInGameMessage* pMsgBuffer;
        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_FB_MSG_LIST"));
        pMsgBuffer = static_cast<CGuiInGameMessage*>(g_pAppManager->m_pClientExoApp->GetInGameGui()->GetMessageBuffer());
        for (i = 0; i < g_pAppManager->m_pClientExoApp->GetInGameGui()->GetMessageBufferSize(); i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldCExoString(&struct1, pMsgBuffer[i].m_text, const_cast<char*>("PT_FB_MSG_MSG"));
            pGff->WriteFieldDWORD(&struct1, pMsgBuffer[i].m_param3, const_cast<char*>("PT_FB_MSG_TYPE"));
            pGff->WriteFieldBYTE(&struct1, pMsgBuffer[i].m_param4, const_cast<char*>("PT_FB_MSG_COLOR"));
        }

        CGuiInGameDialogMessage* pDlgBuffer;
        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_DLG_MSG_LIST"));
        pDlgBuffer = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pDialogMessageBuffer;
        for (i = 0; i < g_pAppManager->m_pClientExoApp->GetInGameGui()->m_dialogMessageBufferSize; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldCExoString(&struct1, pDlgBuffer[i].m_speaker, const_cast<char*>("PT_DLG_MSG_SPKR"));
            pGff->WriteFieldCExoString(&struct1, pDlgBuffer[i].m_text, const_cast<char*>("PT_DLG_MSG_MSG"));
        }

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_COST_MULT_LIST"));
        for (i = 0; i < g_pRules->m_pBaseItemArray->m_count; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldFLOAT(&struct1, g_pRules->GetBaseItem(i)->m_costMultiplier, const_cast<char*>("PT_COST_MULT_VALUE"));
        }

        SaveJournal(pGff, &topStruct);

        pGff->WriteGFFFile(saveName, 0);
    }
    if (name == "REBOOTDATA:")
        g_pExoResMan->CreateSignatureFile(saveName, 0, 0, 0);
    delete pGff;
#else
    CExoString saveName;
    CResGFF* pGff;
    CResStruct topStruct;
    CResStruct struct1;
    CResList list1;
    unsigned int planetMask;
    int i;
    int created;

    pGff = new CResGFF();
    {
        CExoString type("PT  ");
        CExoString version("V2.0");
        created = pGff->CreateGFFFile(&topStruct, type, version);
    }
    if (created == 1)
    {
        saveName = name + "\\" + "PARTYTABLE";

        m_nTotalPlayedSecondsAccum = GetTotalPlayedSeconds();
        ResetCurrentSessionStartTime();

        pGff->WriteFieldCExoString(&topStruct, m_saveTableName, const_cast<char*>("PT_PCNAME"));
        pGff->WriteFieldDWORD(&topStruct, m_nGold, const_cast<char*>("PT_GOLD"));
        pGff->WriteFieldDWORD(&topStruct, m_nItemComponent, const_cast<char*>("PT_ITEM_COMPONENT"));
        pGff->WriteFieldDWORD(&topStruct, m_nItemChemical, const_cast<char*>("PT_ITEM_CHEMICAL"));
        pGff->WriteFieldDWORD(&topStruct, m_swoop[0], const_cast<char*>("PT_SWOOP1"));
        pGff->WriteFieldDWORD(&topStruct, m_swoop[1], const_cast<char*>("PT_SWOOP2"));
        pGff->WriteFieldDWORD(&topStruct, m_swoop[2], const_cast<char*>("PT_SWOOP3"));
        pGff->WriteFieldINT(&topStruct, m_nXPPool, const_cast<char*>("PT_XP_POOL"));
        pGff->WriteFieldDWORD(&topStruct, m_nTotalPlayedSecondsAccum, const_cast<char*>("PT_PLAYEDSECONDS"));
        pGff->WriteFieldINT(&topStruct, m_nControlledNPC, const_cast<char*>("PT_CONTROLLED_NPC"));
        pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_soloMode), const_cast<char*>("PT_SOLOMODE"));
        pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_cheatUsedFlags), const_cast<char*>("PT_CHEAT_USED"));
        pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_nNumPartyMembers), const_cast<char*>("PT_NUM_MEMBERS"));

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_MEMBERS"));
        for (i = 0; i < m_nNumPartyMembers; i++)
        {
            int memberIndex = m_partyMemberIndex[i];
            int leaderIndex = m_nLeaderIndex;
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldINT(&struct1, m_partyMemberIndex[i], const_cast<char*>("PT_MEMBER_ID"));
            pGff->WriteFieldBYTE(&struct1, memberIndex == leaderIndex, const_cast<char*>("PT_IS_LEADER"));
        }

        pGff->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_nNumPuppets), const_cast<char*>("PT_NUM_PUPPETS"));
        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_PUPPETS"));
        for (i = 0; i < m_nNumPuppets; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldINT(&struct1, m_puppetIndex[i], const_cast<char*>("PT_PUPPET_ID"));
        }

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_AVAIL_PUPS"));
        for (i = 0; i < 3; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldBYTE(&struct1, static_cast<unsigned char>(m_pupAvailable[i]), const_cast<char*>("PT_PUP_AVAIL"));
            pGff->WriteFieldBYTE(&struct1, m_pupSelectability[i], const_cast<char*>("PT_PUP_SELECT"));
        }

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_AVAIL_NPCS"));
        for (i = 0; i < 12; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldBYTE(&struct1, static_cast<unsigned char>(m_npcAvailable[i]), const_cast<char*>("PT_NPC_AVAIL"));
            pGff->WriteFieldBYTE(&struct1, m_npcSelectability[i], const_cast<char*>("PT_NPC_SELECT"));
        }

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_INFLUENCE"));
        for (i = 0; i < 12; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldINT(&struct1, m_influence[i], const_cast<char*>("PT_NPC_INFLUENCE"));
        }

        pGff->WriteFieldINT(&topStruct, m_aiState, const_cast<char*>("PT_AISTATE"));
        pGff->WriteFieldINT(&topStruct, m_followState, const_cast<char*>("PT_FOLLOWSTATE"));

        planetMask = 0;
        for (i = 0; i < NUM_PLANETS; i++)
        {
            if (m_planets.m_aAvailable[i] != 0)
                planetMask |= 1 << i;
            if (m_planets.m_aSelectable[i] != 0)
                planetMask |= PLANET_MASK_SELECTABLE_BIT0 << i;
        }
        pGff->AddStructToStruct(&struct1, &topStruct, const_cast<char*>("GlxyMap"), 0);
        pGff->WriteFieldDWORD(&struct1, NUM_PLANETS, const_cast<char*>("GlxyMapNumPnts"));
        pGff->WriteFieldDWORD(&struct1, planetMask, const_cast<char*>("GlxyMapPlntMsk"));
        pGff->WriteFieldINT(&struct1, m_selectedPlanet, const_cast<char*>("GlxyMapSelPnt"));

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_PAZAAKCARDS"));
        for (i = 0; i < 23; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldINT(&struct1, m_pazaakCards[i], const_cast<char*>("PT_PAZAAKCOUNT"));
        }
        pGff->AddListElement(&struct1, &list1, 0);
        pGff->WriteFieldBYTE(&struct1, static_cast<unsigned char>(m_cheatUsedFlags), const_cast<char*>("PT_PAZAAKCOUNT"));

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_PAZSIDELIST"));
        for (i = 0; i < 10; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldINT(&struct1, m_pazaakSideDeck[i], const_cast<char*>("PT_PAZSIDECARD"));
        }

        pGff->WriteFieldVOID(&topStruct, g_pAppManager->m_pClientExoApp->GetInGameGui()->m_tutorialWindowShown, sizeof(CGuiInGame::m_tutorialWindowShown), const_cast<char*>("PT_TUT_WND_SHOWN"));
        pGff->WriteFieldINT(&topStruct, g_pAppManager->m_pClientExoApp->GetInGameGui()->m_lastGuiPanel, const_cast<char*>("PT_LAST_GUI_PNL"));
        pGff->WriteFieldINT(&struct1, m_forfeitViolations, const_cast<char*>("FORFEITVIOL"));
        pGff->WriteFieldINT(&struct1, m_forfeitConditions, const_cast<char*>("FORFEITCONDS"));

        CGuiInGameMessage* pMsgBuffer;
        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_FB_MSG_LIST"));
        pMsgBuffer = static_cast<CGuiInGameMessage*>(g_pAppManager->m_pClientExoApp->GetInGameGui()->GetMessageBuffer());
        int msgCount = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetMessageBufferSize();
        for (i = 0; i < msgCount; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldCExoString(&struct1, pMsgBuffer[i].m_text, const_cast<char*>("PT_FB_MSG_MSG"));
            pGff->WriteFieldDWORD(&struct1, pMsgBuffer[i].m_param3, const_cast<char*>("PT_FB_MSG_TYPE"));
            pGff->WriteFieldBYTE(&struct1, pMsgBuffer[i].m_param4, const_cast<char*>("PT_FB_MSG_COLOR"));
        }

        CGuiInGameDialogMessage* pDlgBuffer;
        int dlgCount;
        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_DLG_MSG_LIST"));
        pDlgBuffer = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pDialogMessageBuffer;
        dlgCount = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_dialogMessageBufferSize;
        for (i = 0; i < dlgCount; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldCExoString(&struct1, pDlgBuffer[i].m_speaker, const_cast<char*>("PT_DLG_MSG_SPKR"));
            pGff->WriteFieldCExoString(&struct1, pDlgBuffer[i].m_text, const_cast<char*>("PT_DLG_MSG_MSG"));
        }

        CGuiInGameMessage* pComBuffer;
        int comCount;
        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_COM_MSG_LIST"));
        pComBuffer = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pCombatMessageBuffer;
        comCount = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_combatMessageBufferSize;
        for (i = 0; i < comCount; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldCExoString(&struct1, pComBuffer[i].m_text, const_cast<char*>("PT_COM_MSG_MSG"));
            pGff->WriteFieldDWORD(&struct1, pComBuffer[i].m_param3, const_cast<char*>("PT_COM_MSG_TYPE"));
            pGff->WriteFieldBYTE(&struct1, pComBuffer[i].m_param4, const_cast<char*>("PT_COM_MSG_COOR"));
        }

        pGff->AddList(&list1, &topStruct, const_cast<char*>("PT_COST_MULT_LIST"));
        for (i = 0; i < g_pRules->m_pBaseItemArray->m_count; i++)
        {
            pGff->AddListElement(&struct1, &list1, 0);
            pGff->WriteFieldFLOAT(&struct1, g_pRules->GetBaseItem(i)->m_costMultiplier, const_cast<char*>("PT_COST_MULT_VALUE"));
        }

        pGff->WriteFieldINT(&struct1, m_disableMap, const_cast<char*>("PT_DISABLEMAP"));
        pGff->WriteFieldINT(&struct1, m_disableRegen, const_cast<char*>("PT_DISABLEREGEN"));

        CStatusSummary statusSummary;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->GetStatusSummary(statusSummary);
        statusSummary.SaveToGFF(pGff, struct1);

        SaveJournal(pGff, &topStruct);

        pGff->WriteGFFFile(saveName, 0);
    }
    if (name == "REBOOTDATA:")
        g_pExoResMan->CreateSignatureFile(saveName, 0, 0, 0);
    delete pGff;
#endif
}

int CSWPartyTable::LoadTableInfo()
{
#if KOTOR1
    // K1 writes a smaller PT_* subset and reports its own load-error string reference.
    CResGFF* pGff = new CResGFF(0, const_cast<char*>("PT  "), CResRef("PARTYTABLE"));
    if (pGff->m_nValid == 0)
    {
        HandleLoadError(STRREF_UNABLE_TO_LOAD_PARTY);
        delete pGff;
        return 0;
    }

    ClearTable();

    CResStruct topStruct;
    pGff->GetTopLevelStruct(&topStruct);

    CResStruct struct1;
    CResList list1;
    int bSuccess;

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_MEMBERS"));

    m_nGold = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_GOLD"), bSuccess, 0);
    m_nXPPool = pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_XP_POOL"), bSuccess, 0);
    bSuccess = 1;
    m_nTotalPlayedSecondsAccum = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_PLAYEDSECONDS"), bSuccess, 0);
    if (bSuccess == 0)
        m_nTotalPlayedSecondsAccum = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_PLAYEDMINUTES"), bSuccess, 0) * 60;
    m_nControlledNPC = pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_CONTROLLED_NPC"), bSuccess, -1);
    int soloMode = pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("PT_SOLOMODE"), bSuccess, static_cast<unsigned char>(m_soloMode));
    if (soloMode == 0)
        UnstealthParty(0);
    m_soloMode = soloMode;
    m_cheatUsedFlags = pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("PT_CHEAT_USED"), bSuccess, 0);
    m_nNumPartyMembers = pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("PT_NUM_MEMBERS"), bSuccess, 0);
    if (pGff->GetListCount(&list1) < static_cast<unsigned int>(m_nNumPartyMembers))
        m_nNumPartyMembers = pGff->GetListCount(&list1);
#ifdef K2_FIX_ENGINE_BUGS
    if (m_nNumPartyMembers > static_cast<int>(sizeof(m_partyMemberIndex) / sizeof(m_partyMemberIndex[0])))
        m_nNumPartyMembers = static_cast<int>(sizeof(m_partyMemberIndex) / sizeof(m_partyMemberIndex[0]));
#endif
    for (int i = 0; i < m_nNumPartyMembers; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_partyMemberIndex[i] = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_MEMBER_ID"), bSuccess, -1);
        if (pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_IS_LEADER"), bSuccess, 0))
            m_nLeaderIndex = m_partyMemberIndex[i];
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_AVAIL_NPCS"));
    unsigned int listCount = pGff->GetListCount(&list1);
    int availCount = listCount < 9 ? listCount : 9;
    for (int i = 0; i < availCount; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_npcAvailable[i] = pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_NPC_AVAIL"), bSuccess, 0);
        m_npcSelectability[i] = pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_NPC_SELECT"), bSuccess, 0);
    }

    m_aiState = pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_AISTATE"), bSuccess, 0);
    m_followState = pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_FOLLOWSTATE"), bSuccess, 0);

    if (pGff->GetStructFromStruct(&struct1, &topStruct, const_cast<char*>("GlxyMap")))
    {
        unsigned int numPlanets = pGff->ReadFieldDWORD(&struct1, const_cast<char*>("GlxyMapNumPnts"), bSuccess, 0);
        unsigned int planetMask = pGff->ReadFieldDWORD(&struct1, const_cast<char*>("GlxyMapPlntMsk"), bSuccess, 0);
        m_selectedPlanet = pGff->ReadFieldINT(&struct1, const_cast<char*>("GlxyMapSelPnt"), bSuccess, -1);
        if (numPlanets == NUM_PLANETS)
        {
            for (int i = 0; i < NUM_PLANETS; i++)
            {
                if ((1 << i) & planetMask)
                    m_planets.m_aAvailable[i] = 1;
                if ((PLANET_MASK_SELECTABLE_BIT0 << i) & planetMask)
                    m_planets.m_aSelectable[i] = 1;
            }
        }
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_PAZAAKCARDS"));
    for (int i = 0; i < 18; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_pazaakCards[i] = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_PAZAAKCOUNT"), bSuccess, 0);
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_PAZSIDELIST"));
    for (int i = 0; i < 10; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_pazaakSideDeck[i] = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_PAZSIDECARD"), bSuccess, 0);
    }

    unsigned char defaultTutShown[6] = {};
    pGff->ReadFieldVOID(&topStruct, g_pAppManager->m_pClientExoApp->GetInGameGui()->m_tutorialWindowShown, sizeof(CGuiInGame::m_tutorialWindowShown), const_cast<char*>("PT_TUT_WND_SHOWN"), bSuccess, defaultTutShown);
    CGuiInGame* pGuiInGame = g_pAppManager->m_pClientExoApp->GetInGameGui();
    pGuiInGame->m_lastGuiPanel =
        pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_LAST_GUI_PNL"), bSuccess, g_pAppManager->m_pClientExoApp->GetInGameGui()->m_lastGuiPanel);

    CExoString text;
    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_FB_MSG_LIST"));
    for (int i = 0; i < static_cast<int>(pGff->GetListCount(&list1)); i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        text = pGff->ReadFieldCExoString(&struct1, const_cast<char*>("PT_FB_MSG_MSG"), bSuccess, CExoString(""));
        unsigned int msgType = pGff->ReadFieldDWORD(&struct1, const_cast<char*>("PT_FB_MSG_TYPE"), bSuccess, kMessageTypeDebug);
        unsigned char msgColor = pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_FB_MSG_COLOR"), bSuccess, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(text, msgType, msgColor);
    }

    CExoString speaker;
    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_DLG_MSG_LIST"));
    for (int i = 0; i < static_cast<int>(pGff->GetListCount(&list1)); i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        speaker = pGff->ReadFieldCExoString(&struct1, const_cast<char*>("PT_DLG_MSG_SPKR"), bSuccess, CExoString(""));
        text = pGff->ReadFieldCExoString(&struct1, const_cast<char*>("PT_DLG_MSG_MSG"), bSuccess, CExoString(""));
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToDialogBuffer(text, speaker);
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_COST_MULT_LIST"));
    for (int i = 0; i < g_pRules->m_pBaseItemArray->m_count; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(i);
        pBaseItem->m_costMultiplier =
            pGff->ReadFieldFLOAT(&struct1, const_cast<char*>("PT_COST_MULT_VALUE"), bSuccess, 1.0f);
    }

    LoadJournal(pGff, &topStruct);

    delete pGff;
    return 1;
#else
    CResGFF* pGff = new CResGFF(0, const_cast<char*>("PT  "), CResRef("PARTYTABLE"));
    if (pGff->m_nValid == 0)
    {
        HandleLoadError(STRREF_UNABLE_TO_LOAD_PARTY);
        delete pGff;
        return 0;
    }

    ClearTable();

    CResStruct topStruct;
    pGff->GetTopLevelStruct(&topStruct);

    CResStruct struct1;
    CResList list1;
    int bSuccess;
    int soloMode;
    int availCount;

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_MEMBERS"));

    CExoString defaultStr("");
    m_saveTableName = pGff->ReadFieldCExoString(&topStruct, const_cast<char*>("PT_PCNAME"), bSuccess, defaultStr);

    m_nGold = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_GOLD"), bSuccess, 0);
    m_nItemComponent = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_ITEM_COMPONENT"), bSuccess, 0);
    m_nItemChemical = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_ITEM_CHEMICAL"), bSuccess, 0);
    m_swoop[0] = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_SWOOP1"), bSuccess, 0);
    m_swoop[1] = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_SWOOP2"), bSuccess, 0);
    m_swoop[2] = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_SWOOP3"), bSuccess, 0);
    m_nXPPool = pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_XP_POOL"), bSuccess, 0);
    bSuccess = 1;
    m_nTotalPlayedSecondsAccum = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_PLAYEDSECONDS"), bSuccess, 0);
    if (bSuccess == 0)
        m_nTotalPlayedSecondsAccum = pGff->ReadFieldDWORD(&topStruct, const_cast<char*>("PT_PLAYEDMINUTES"), bSuccess, 0) * 60;
    m_nControlledNPC = pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_CONTROLLED_NPC"), bSuccess, -1);
    soloMode = pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("PT_SOLOMODE"), bSuccess, static_cast<unsigned char>(m_soloMode));
    if (soloMode == 0)
        UnstealthParty(0);
    m_soloMode = soloMode;
    m_cheatUsedFlags = pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("PT_CHEAT_USED"), bSuccess, 0);
    m_nNumPartyMembers = pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("PT_NUM_MEMBERS"), bSuccess, 0);
    if (pGff->GetListCount(&list1) < static_cast<unsigned int>(m_nNumPartyMembers))
        m_nNumPartyMembers = pGff->GetListCount(&list1);
#ifdef K2_FIX_ENGINE_BUGS
    if (m_nNumPartyMembers > static_cast<int>(sizeof(m_partyMemberIndex) / sizeof(m_partyMemberIndex[0])))
        m_nNumPartyMembers = static_cast<int>(sizeof(m_partyMemberIndex) / sizeof(m_partyMemberIndex[0]));
#endif
    for (int i = 0; i < m_nNumPartyMembers; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_partyMemberIndex[i] = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_MEMBER_ID"), bSuccess, -1);
        if (pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_IS_LEADER"), bSuccess, 0))
            m_nLeaderIndex = m_partyMemberIndex[i];
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_PUPPETS"));
    m_nNumPuppets = pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("PT_NUM_PUPPETS"), bSuccess, 0);
    if (pGff->GetListCount(&list1) < static_cast<unsigned int>(m_nNumPuppets))
        m_nNumPuppets = pGff->GetListCount(&list1);
#ifdef K2_FIX_ENGINE_BUGS
    if (m_nNumPuppets > static_cast<int>(sizeof(m_puppetIndex) / sizeof(m_puppetIndex[0])))
        m_nNumPuppets = static_cast<int>(sizeof(m_puppetIndex) / sizeof(m_puppetIndex[0]));
#endif
    for (int i = 0; i < m_nNumPuppets; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_puppetIndex[i] = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_PUPPET_ID"), bSuccess, -1);
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_AVAIL_PUPS"));
    availCount = 3;
    if (pGff->GetListCount(&list1) <= 2)
        availCount = pGff->GetListCount(&list1);
    for (int i = 0; i < availCount; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_pupAvailable[i] = pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_PUP_AVAIL"), bSuccess, 0);
        m_pupSelectability[i] = pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_PUP_SELECT"), bSuccess, 0);
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_AVAIL_NPCS"));
    availCount = 12;
    if (pGff->GetListCount(&list1) <= 11)
        availCount = pGff->GetListCount(&list1);
    for (int i = 0; i < availCount; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_npcAvailable[i] = pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_NPC_AVAIL"), bSuccess, 0);
        m_npcSelectability[i] = pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_NPC_SELECT"), bSuccess, 0);
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_INFLUENCE"));
    availCount = 12;
    if (pGff->GetListCount(&list1) <= 11)
        availCount = pGff->GetListCount(&list1);
    for (int i = 0; i < availCount; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_influence[i] = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_NPC_INFLUENCE"), bSuccess, -1);
    }

    m_aiState = pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_AISTATE"), bSuccess, 0);
    m_followState = pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_FOLLOWSTATE"), bSuccess, 0);

    if (pGff->GetStructFromStruct(&struct1, &topStruct, const_cast<char*>("GlxyMap")))
    {
        unsigned int numPlanets = pGff->ReadFieldDWORD(&struct1, const_cast<char*>("GlxyMapNumPnts"), bSuccess, 0);
        unsigned int planetMask = pGff->ReadFieldDWORD(&struct1, const_cast<char*>("GlxyMapPlntMsk"), bSuccess, 0);
        m_selectedPlanet = pGff->ReadFieldINT(&struct1, const_cast<char*>("GlxyMapSelPnt"), bSuccess, -1);
        if (numPlanets == NUM_PLANETS)
        {
            for (int i = 0; i < NUM_PLANETS; i++)
            {
                if ((1 << i) & planetMask)
                    m_planets.m_aAvailable[i] = 1;
                if ((PLANET_MASK_SELECTABLE_BIT0 << i) & planetMask)
                    m_planets.m_aSelectable[i] = 1;
            }
        }
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_PAZAAKCARDS"));
    for (int i = 0; i < 23; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_pazaakCards[i] = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_PAZAAKCOUNT"), bSuccess, 0);
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_PAZSIDELIST"));
    for (int i = 0; i < 10; i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        m_pazaakSideDeck[i] = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_PAZSIDECARD"), bSuccess, 0);
    }

    unsigned char defaultTutShown[33] = {};
    CGuiInGame* pGuiInGame;
    pGff->ReadFieldVOID(&topStruct, g_pAppManager->m_pClientExoApp->GetInGameGui()->m_tutorialWindowShown, sizeof(CGuiInGame::m_tutorialWindowShown), const_cast<char*>("PT_TUT_WND_SHOWN"), bSuccess, defaultTutShown);
    pGuiInGame = g_pAppManager->m_pClientExoApp->GetInGameGui();
    pGuiInGame->m_lastGuiPanel =
        pGff->ReadFieldINT(&topStruct, const_cast<char*>("PT_LAST_GUI_PNL"), bSuccess, g_pAppManager->m_pClientExoApp->GetInGameGui()->m_lastGuiPanel);
    m_forfeitViolations = pGff->ReadFieldINT(&struct1, const_cast<char*>("FORFEITVIOL"), bSuccess, 0);
    m_forfeitConditions = pGff->ReadFieldINT(&struct1, const_cast<char*>("FORFEITCONDS"), bSuccess, 0);

    CExoString text;
    unsigned int msgType;
    unsigned char msgColor;
    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_FB_MSG_LIST"));
    for (int i = 0; i < static_cast<int>(pGff->GetListCount(&list1)); i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        text = pGff->ReadFieldCExoString(&struct1, const_cast<char*>("PT_FB_MSG_MSG"), bSuccess, CExoString(""));
        msgType = pGff->ReadFieldDWORD(&struct1, const_cast<char*>("PT_FB_MSG_TYPE"), bSuccess, kMessageTypeDebug);
        msgColor = pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_FB_MSG_COLOR"), bSuccess, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(text, msgType, msgColor);
    }

    CExoString speaker;
    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_DLG_MSG_LIST"));
    for (int i = 0; i < static_cast<int>(pGff->GetListCount(&list1)); i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        speaker = pGff->ReadFieldCExoString(&struct1, const_cast<char*>("PT_DLG_MSG_SPKR"), bSuccess, CExoString(""));
        text = pGff->ReadFieldCExoString(&struct1, const_cast<char*>("PT_DLG_MSG_MSG"), bSuccess, CExoString(""));
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToDialogBuffer(text, speaker);
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_COM_MSG_LIST"));
    for (int i = 0; i < static_cast<int>(pGff->GetListCount(&list1)); i++)
    {
        pGff->GetListElement(&struct1, &list1, i);
        text = pGff->ReadFieldCExoString(&struct1, const_cast<char*>("PT_COM_MSG_MSG"), bSuccess, CExoString(""));
        msgType = pGff->ReadFieldDWORD(&struct1, const_cast<char*>("PT_COM_MSG_TYPE"), bSuccess, kMessageTypeDebug);
        msgColor = pGff->ReadFieldBYTE(&struct1, const_cast<char*>("PT_COM_MSG_COOR"), bSuccess, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToCombatBuffer(text, msgType, msgColor);
    }

    pGff->GetList(&list1, &topStruct, const_cast<char*>("PT_COST_MULT_LIST"));
    for (int i = 0; i < g_pRules->m_pBaseItemArray->m_count; i++)
    {
        CSWBaseItem* pBaseItem;
        pGff->GetListElement(&struct1, &list1, i);
        pBaseItem = g_pRules->GetBaseItem(i);
        pBaseItem->m_costMultiplier =
            pGff->ReadFieldFLOAT(&struct1, const_cast<char*>("PT_COST_MULT_VALUE"), bSuccess, 1.0f);
    }

    m_disableMap = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_DISABLEMAP"), bSuccess, 0);
    m_disableRegen = pGff->ReadFieldINT(&struct1, const_cast<char*>("PT_DISABLEREGEN"), bSuccess, 0);

    CStatusSummary statusSummary;
    statusSummary.LoadFromGFF(pGff, struct1);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetStatusSummary(statusSummary);

    LoadJournal(pGff, &topStruct);

    delete pGff;
    return 1;
#endif
}

void CSWPartyTable::Save(const CExoString& name, int refresh)
{
    if (refresh != 0)
    {
        UpdateMembers(0);
        UpdateInventory(0);
    }
    SaveTableInfo(name);
}

int CSWPartyTable::GetNPCID(unsigned long long id)
{
    if (id == OBJECT_INVALID) return -1;
    if (m_partyMemberID[0] == id) return 0;
    if (m_partyMemberID[1] == id) return 1;
    if (m_partyMemberID[2] == id) return 2;
    if (m_partyMemberID[3] == id) return 3;
    if (m_partyMemberID[4] == id) return 4;
    if (m_partyMemberID[5] == id) return 5;
    if (m_partyMemberID[6] == id) return 6;
    if (m_partyMemberID[7] == id) return 7;
#if KOTOR1
    return (m_partyMemberID[8] == id) ? 8 : -1;
#else
    if (m_partyMemberID[8] == id) return 8;
    if (m_partyMemberID[9] == id) return 9;
    if (m_partyMemberID[10] == id) return 10;
    return (m_partyMemberID[11] == id) ? 11 : -1;
#endif
}

void CSWPartyTable::SetLeader(unsigned long long id)
{
    m_nLeaderIndex = -1;
    if (id == OBJECT_INVALID)
        return;
    int index;
    if (m_partyMemberID[0] == id) index = 0;
    else if (m_partyMemberID[1] == id) index = 1;
    else if (m_partyMemberID[2] == id) index = 2;
    else if (m_partyMemberID[3] == id) index = 3;
    else if (m_partyMemberID[4] == id) index = 4;
    else if (m_partyMemberID[5] == id) index = 5;
    else if (m_partyMemberID[6] == id) index = 6;
    else if (m_partyMemberID[7] == id) index = 7;
#if KOTOR1
    else if (m_partyMemberID[8] != id) return;
    else index = 8;
#else
    else if (m_partyMemberID[8] == id) index = 8;
    else if (m_partyMemberID[9] == id) index = 9;
    else if (m_partyMemberID[10] == id) index = 10;
    else if (m_partyMemberID[11] != id) return;
    else index = 11;
#endif
    m_nLeaderIndex = index;
}

int CSWPartyTable::GetIsLeader(unsigned long long id)
{
    int index;
    if (id == OBJECT_INVALID) return 0;
    else if (m_partyMemberID[0] == id) index = 0;
    else if (m_partyMemberID[1] == id) index = 1;
    else if (m_partyMemberID[2] == id) index = 2;
    else if (m_partyMemberID[3] == id) index = 3;
    else if (m_partyMemberID[4] == id) index = 4;
    else if (m_partyMemberID[5] == id) index = 5;
    else if (m_partyMemberID[6] == id) index = 6;
    else if (m_partyMemberID[7] == id) index = 7;
#if KOTOR1
    else if (m_partyMemberID[8] != id) return 0;
    else index = 8;
#else
    else if (m_partyMemberID[8] == id) index = 8;
    else if (m_partyMemberID[9] == id) index = 9;
    else if (m_partyMemberID[10] == id) index = 10;
    else if (m_partyMemberID[11] != id) return 0;
    else index = 11;
#endif
    return m_nLeaderIndex == index;
}

void CSWPartyTable::SaveMember(int index, int value)
{
    if (static_cast<unsigned>(index) >= NUM_NPCS)
        return;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[index]);
    if (pCreature == 0)
        return;

    if (value != 0)
        pCreature->ClearAllActions(1);

    CResStruct topStruct;
    CExoString name;
    name.Format(const_cast<char*>("AVAILNPC%d"), index);

    CResGFF* pGff = new CResGFF();
    {
        CExoString type("UTC ");
        CExoString version("V2.0");
        pGff->CreateGFFFile(&topStruct, type, version);
    }
    pCreature->SaveCreature(pGff, &topStruct);

    CExoString gameInProgressPath = CExoString("GAMEINPROGRESS:") + name;
    pGff->WriteGFFFile(gameInProgressPath.CStr(), RESTYPE_UTC);

    delete pGff;
}

#if !KOTOR1
void CSWPartyTable::SavePuppet(int index, int value)
{
    if (static_cast<unsigned>(index) > 2)
        return;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_puppetID[index]);
    if (pCreature == 0)
        return;

    if (value != 0)
        pCreature->ClearAllActions(1);

    CResStruct topStruct;
    CExoString name;
    name.Format(const_cast<char*>("AVAILPUP%d"), index);

    CResGFF* pGff = new CResGFF();
    {
        CExoString type("UTC ");
        CExoString version("V2.0");
        pGff->CreateGFFFile(&topStruct, type, version);
    }
    pCreature->SaveCreature(pGff, &topStruct);

    CExoString gameInProgressPath = CExoString("GAMEINPROGRESS:") + name;
    pGff->WriteGFFFile(gameInProgressPath.CStr(), RESTYPE_UTC);

    delete pGff;
}
#endif

#if !KOTOR1
void CSWPartyTable::SaveMemberByObject(int index, unsigned long long id)
{
    if (static_cast<unsigned>(index) >= NUM_NPCS)
        return;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id);
    if (pCreature == 0)
        return;

    pCreature->ClearAllActions(1);

    CResStruct topStruct;
    CExoString name;
    name.Format(const_cast<char*>("AVAILNPC%d"), index);

    CResGFF* pGff = new CResGFF();
    {
        CExoString type("UTC ");
        CExoString version("V2.0");
        pGff->CreateGFFFile(&topStruct, type, version);
    }
    pCreature->SaveCreature(pGff, &topStruct);

    CExoString gameInProgressPath = CExoString("GAMEINPROGRESS:") + name;
    pGff->WriteGFFFile(gameInProgressPath.CStr(), RESTYPE_UTC);

    delete pGff;
}
#endif

#if !KOTOR1
void CSWPartyTable::SavePuppetByObject(int index, unsigned long long id)
{
    if (static_cast<unsigned>(index) >= 3)
        return;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id);
    if (pCreature == 0)
        return;

    pCreature->ClearAllActions(1);

    CResStruct topStruct;
    CExoString name;
    name.Format(const_cast<char*>("AVAILPUP%d"), index);

    CResGFF* pGff = new CResGFF();
    {
        CExoString type("UTC ");
        CExoString version("V2.0");
        pGff->CreateGFFFile(&topStruct, type, version);
    }
    pCreature->SaveCreature(pGff, &topStruct);

    CExoString gameInProgressPath = CExoString("GAMEINPROGRESS:") + name;
    pGff->WriteGFFFile(gameInProgressPath.CStr(), RESTYPE_UTC);

    delete pGff;
}
#endif

void CSWPartyTable::UpdateInventory(int value)
{
    if (m_pItemRepository == 0)
        return;

    CResGFF* pGff = new CResGFF();
    CResStruct topStruct;
    CResList itemList;
    CResStruct elemStruct;
    pGff->CreateGFFFile(&topStruct, CExoString("INV "), CExoString("V2.0"));
    pGff->AddList(&itemList, &topStruct, const_cast<char*>("ItemList"));

    if (m_pItemRepository->ItemListGetCount() != 0)
    {
        unsigned int i = 0;
        do
        {
            CSWSItem* pItem = m_pItemRepository->ItemListGetItem(i);
            if (pItem != 0 && pGff->AddListElement(&elemStruct, &itemList, 0))
                pItem->SaveItem(pGff, &elemStruct);
            i++;
        } while (i < m_pItemRepository->ItemListGetCount());
    }

    CExoString gameInProgressPath("GAMEINPROGRESS:INVENTORY");
    pGff->WriteGFFFile(CExoString(gameInProgressPath.CStr()), 0);

    if (value != 0)
        m_pItemRepository->ClearItems();

    delete pGff;
}

void CSWPartyTable::UpdateMembers(int value)
{
    SaveMember(0, value);
    SaveMember(1, value);
    SaveMember(2, value);
    SaveMember(3, value);
    SaveMember(4, value);
    SaveMember(5, value);
    SaveMember(6, value);
    SaveMember(7, value);
    SaveMember(8, value);
#if !KOTOR1
    SaveMember(9, value);
    SaveMember(10, value);
    SaveMember(11, value);
    SavePuppet(0, value);
    SavePuppet(1, value);
    SavePuppet(2, value);
#endif
}

#if !KOTOR1
CExoString CSWPartyTable::GetPCFirstName()
{
    CExoString result(m_saveTableName);
    if (m_saveTableName.GetLength() >= 1)
    {
        int spaceIndex = m_saveTableName.Find(' ', 0);
        if (spaceIndex != -1)
        {
            CExoString tmp = m_saveTableName.SubString(0, spaceIndex);
            result = tmp;
        }
    }
    return result;
}
#endif

#if !KOTOR1
CExoString CSWPartyTable::GetPCLastName()
{
    CExoString lastName(m_saveTableName);
    if (m_saveTableName.GetLength() >= 1)
    {
        int spaceIndex = m_saveTableName.Find(' ', 0);
        if (spaceIndex != -1)
        {
            CExoString tmp = m_saveTableName.SubString(spaceIndex + 1, -1);
            lastName = tmp;
        }
    }
    if (lastName.GetLength() <= 0)
    {
        return m_saveTableName;
    }
    return lastName;
}
#endif

int CSWPartyTable::KillNPCObject(int npcIndex, int notify)
{
    if (static_cast<unsigned>(npcIndex) >= NUM_NPCS)
        return 0;

    long long memberCount = m_nNumPartyMembers;
    if (memberCount >= 1)
    {
        int* pIndex = m_partyMemberIndex;
        do
        {
            if (*pIndex == npcIndex)
                return 0;
            memberCount--;
            pIndex++;
        } while (memberCount != 0);
    }

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[npcIndex]);
    m_partyMemberID[npcIndex] = OBJECT_INVALID;
    if (pCreature == 0)
        return 0;

    if (notify != 0)
    {
        CSWCObject* pClientObj = static_cast<CSWCObject*>(pCreature->GetClientCreature());
        if (pClientObj != 0)
        {
            pClientObj->SetFadeState(0);
            pClientObj->SetDesiredFadeState(0, 1, 0, 0);
            pClientObj->m_bPendingRemoval = 1;
        }
    }

#if !KOTOR1 || defined(K2_FIX_ENGINE_BUGS)
    unsigned long long futureLeader = g_pAppManager->m_pClientExoApp->GetFutureLeader();
    unsigned long long serverObjId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(futureLeader);
    if (pCreature->m_objectId == serverObjId)
        g_pAppManager->m_pClientExoApp->SetFutureLeader(OBJECT_INVALID);
#endif

    delete pCreature;
    return 1;
}

#if !KOTOR1
int CSWPartyTable::RemovePuppet(int pupIndex)
{
    if (static_cast<unsigned>(pupIndex) > 2)
        return 0;
    if (m_pupAvailable[pupIndex] == 0)
        return 0;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_puppetID[pupIndex]);
    if (pCreature != 0)
    {
        pCreature->ClearAllEffects(0);
        pCreature->SetIsPuppet(0);
        if (pCreature->GetClientCreature() != 0)
            pCreature->SetIsPuppet(0);
        SavePuppet(pupIndex, 0);
    }

    int numPuppets = m_nNumPuppets;
    if (numPuppets < 1)
        return 0;

    for (int i = 0; i < numPuppets; i++)
    {
        if (m_puppetIndex[i] == pupIndex)
        {
            for (int j = i + 1; j < numPuppets; j++)
            {
                K2_ASSUME(j < 2);
                m_puppetIndex[j - 1] = m_puppetIndex[j];
            }
            m_nNumPuppets = numPuppets - 1;
            return 1;
        }
    }
    return 0;
}
#endif

int CSWPartyTable::RemoveMember(int npcIndex)
{
#if KOTOR1
    if (m_nControlledNPC != -1)
        return 0;
    if (static_cast<unsigned>(npcIndex) > 8)
        return 0;
    if (m_nNumPartyMembers < 1)
        return 0;
    if (m_npcAvailable[npcIndex] == 0)
        return 0;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[npcIndex]);
    if (pCreature != 0)
        pCreature->ClearAllEffects(0);

    int numMembers = m_nNumPartyMembers;
    for (int i = 0; i < numMembers; i++)
    {
        if (m_partyMemberIndex[i] == npcIndex)
        {
            for (int j = i + 1; j < numMembers; j++)
            {
                K2_ASSUME(j < 2);
                m_partyMemberIndex[j - 1] = m_partyMemberIndex[j];
            }
            numMembers--;
            m_nNumPartyMembers = numMembers;
            if (numMembers == 0 && m_soloMode != 0)
                m_soloMode = 0;
            return 1;
        }
    }
    return 0;
#else
    if (static_cast<unsigned>(npcIndex) >= NUM_NPCS)
        return 0;
    if (m_nNumPartyMembers < 1)
        return 0;
    if (m_npcAvailable[npcIndex] == 0)
        return 0;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[npcIndex]);
    if (pCreature != 0)
    {
        pCreature->ClearAllEffects(0);
        int pupIndex = pCreature->m_nAssignedPup;
        if (pupIndex != -1 && m_nNumPuppets >= 1)
        {
            for (int k = 0; k < m_nNumPuppets; k++)
            {
                if (m_puppetIndex[k] == pupIndex)
                {
                    RemovePuppet(pupIndex);
                    KillPUPObject(pupIndex, 1);
                    break;
                }
            }
        }
        SaveMember(npcIndex, 0);
    }

    int numMembers = m_nNumPartyMembers;
    for (int i = 0; i < numMembers; i++)
    {
        if (m_partyMemberIndex[i] == npcIndex)
        {
            for (int j = i + 1; j < numMembers; j++)
            {
                K2_ASSUME(j < 2);
                m_partyMemberIndex[j - 1] = m_partyMemberIndex[j];
            }
            numMembers--;
            m_nNumPartyMembers = numMembers;
            if (numMembers == 0 && m_soloMode != 0)
                m_soloMode = 0;
            return 1;
        }
    }
    return 0;
#endif
}

#if !KOTOR1
unsigned long long CSWPartyTable::GetNextPC()
{
    int i = m_nCurrentPCIndex;
    if (i > 1)
        return OBJECT_INVALID;
    int position = m_partyMemberIndex[i];
    if (static_cast<unsigned int>(position) >= NUM_NPCS)
        return OBJECT_INVALID;
    unsigned long long id = m_partyMemberID[position];
    m_nCurrentPCIndex = i + 1;
    return id;
}
#endif

void CSWPartyTable::GivePlotXP(unsigned int row, int percent)
{
    int xpValue = 0;
    C2DA* pTable = g_pRules->m_p2DArrays->m_pPlotXP;
    if (pTable == 0)
        return;

    {
        CExoString col("XP");
        pTable->GetINTEntry(row, col, &xpValue);
    }

    int xp = static_cast<int>(__builtin_ceilf(static_cast<float>(xpValue * percent) / 100.0f));
    if (xp > 0)
    {
        DistributeExperience(xp, 1);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_XP, xp);
    }
}

void CSWPartyTable::GivePlotXP(const CExoString& row, int percent)
{
    int xpValue = 0;
    C2DA* pTable = g_pRules->m_p2DArrays->m_pPlotXP;
    if (pTable == 0)
        return;

    {
        CExoString col("XP");
        pTable->GetINTEntry(row, col, &xpValue);
    }

    int xp = static_cast<int>(__builtin_ceilf(static_cast<float>(xpValue * percent) / 100.0f));
    if (xp > 0)
    {
        DistributeExperience(xp, 1);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_XP, xp);
    }
}

// Shares an XP award across the active party: adds it to the running total, gives the first
// member holding the leadership feat a bonus on top of the shared amount, hands the amount
// to every member (auto-leveling them when the client option is on), then gives it to the
// player character and, when flag is set, sends the feedback message carrying the amount.
// The leadership bonus accrues to the shared pool before the per-member handout, so the
// feat-holder's discovery inflates everyone.
void CSWPartyTable::DistributeExperience(int xp, int flag)
{
    if (xp < 1)
        return;
    m_nXPPool += xp;
#if !KOTOR1
    for (int i = 0; i < m_nNumPartyMembers; i++)
    {
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[m_partyMemberIndex[i]]);
        if (pCreature != 0 && pCreature->m_pStats->HasFeat(FEAT_MENTOR))
        {
            unsigned char level = pCreature->m_pStats->GetLevel(0);
            if (level != 0)
            {
                float share = (level > 6) ? 0.05f : 0.03f;
                if (level > 12)
                    share = 0.07f;
                xp += static_cast<int>(__builtin_ceilf(share * static_cast<float>(xp)));
            }
            break;
        }
    }
#endif
    for (int i = 0; i < m_nNumPartyMembers; i++)
    {
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[m_partyMemberIndex[i]]);
        if (pCreature != 0)
        {
            pCreature->m_sTag.CStr();  // sic: return value unused (leftover from a debug print?)
            pCreature->ReceiveExperience(xp);
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bAutoLevelUpNPCs &&
                pCreature->m_pStats->CanLevelUp() != 0)
                pCreature->m_pStats->AutoLevelup(1);
        }
    }
    CExoLinkedList<CSWSClient>* ppList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    CSWSPlayer* pPlayer = static_cast<CSWSClient*>(ppList->m_pInternal->m_pHead->m_pData)->AsSWSPlayer();
    unsigned long long playerId = pPlayer->m_gameObjectId;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(playerId);
    if (pCreature != 0)
    {
        pCreature->ReceiveExperience(xp);
        if (flag != 0)
        {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetInteger(0, xp);
            pCreature->SendFeedbackMessage(FEEDBACK_EXPERIENCE_RECEIVED, pData);
        }
    }
}

// Adds roster NPC `index` (object `id`) to the active party: rejects a slot out of range, a
// full party, an unavailable NPC, a missing object, a slot already in the party, or an object
// already in any party slot; then records the slot, marks both creatures in-party, moves the
// NPC's inventory to the shared repository, force-equips clothing, seeds solo mode from the
// player on the first member, and re-adds the creature's assigned puppet when it has one.
int CSWPartyTable::AddMember(int index, unsigned long long id)
{
#if KOTOR1
    // K1 refuses while a controlled NPC is set, and has no influence or puppet re-add.
    if (m_nControlledNPC != -1)
        return 0;
    int result = 0;
    if (static_cast<unsigned>(index) > 8)
        return result;
    if (m_nNumPartyMembers == 2)
        return result;
    if (m_npcAvailable[index] == 0)
        return result;
    if (g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id) == 0)
        return result;
    if (GetIsMember(index))
        return result;
    for (int k = 0; k < 9; k++)
    {
        if (k != index && m_partyMemberID[k] == id)
            return result;
    }
    m_partyMemberIndex[m_nNumPartyMembers] = index;
    m_partyMemberID[index] = id;
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
    CSWCCreature* pCCreature = pClientApp->GetCreatureByGameObjectID(pClientApp->ServerToClientObjectId(id));
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id);
    if (pCCreature != 0)
        pCCreature->SetInParty(1);
    if (pCreature != 0)
    {
        pCreature->SetInParty(1, 1);
        TransferInventory(pCreature);
        pCreature->ForceEquipClothing();
    }
    if (m_nNumPartyMembers == 0)
    {
        CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
        CSWSCreature* pPC = pServerApp->GetPlayerCreature();
        if (pPC != 0 && pPC->m_stealthMode == 1)
            m_soloMode = 1;
    }
    m_nNumPartyMembers = m_nNumPartyMembers + 1;
    return 1;
#else
    int result = 0;
    if (static_cast<unsigned>(index) >= NUM_NPCS)
        return result;
    if (m_nNumPartyMembers == 2)
        return result;
    if (m_npcAvailable[index] == 0)
        return result;
    if (g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id) == 0)
        return result;
    if (GetIsMember(index))
        return result;
    for (int k = 0; k < 12; k++)
    {
        if (k != index && m_partyMemberID[k] == id)
            return result;
    }
    m_partyMemberIndex[m_nNumPartyMembers] = index;
    m_partyMemberID[index] = id;
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
    CSWCCreature* pCCreature = pClientApp->GetCreatureByGameObjectID(pClientApp->ServerToClientObjectId(id));
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id);
    if (pCCreature != 0)
        pCCreature->SetInParty(1);
    if (pCreature != 0)
    {
        pCreature->SetInParty(1, 1);
        TransferInventory(pCreature);
        pCreature->ForceEquipClothing();
    }
    if (m_nNumPartyMembers == 0)
    {
        CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
        CSWSCreature* pPC = pServerApp->GetPlayerCreature();
        if (pPC != 0 && pPC->m_stealthMode == 1)
            m_soloMode = 1;
    }
    m_nNumPartyMembers = m_nNumPartyMembers + 1;
    K2_ASSUME(pCreature != 0);
    pCreature->HandleAlignmentInfluence();
    int pupIndex = pCreature->m_nAssignedPup;
    if (pupIndex == -1)
        return 1;
    result = 1;
    unsigned long long pupId = SpawnPUP(pupIndex, 1, pCreature->m_position, pCreature->m_orientation, 1);
    AddPuppet(pupIndex, pupId);
    return result;
#endif
}

#if !KOTOR1
int CSWPartyTable::AddPuppet(int index, unsigned long long id)
{
    if (static_cast<unsigned>(index) > 2)
        return 0;
    if (m_nNumPuppets == 2)
        return 0;
    if (m_pupAvailable[index] == 0)
        return 0;

    if (g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id) == 0)
        return 0;

    long long numPuppets = m_nNumPuppets;
    if (numPuppets >= 1)
    {
        for (long long i = 0; i < numPuppets; i++)
        {
            if (m_puppetIndex[i] == index)
                return 0;
        }
    }

    if (index == 0)
    {
    checkSlot1:
        if (m_puppetID[1] == id)
            return 0;
        if (index == 2)
            goto doWrite;
    }
    else
    {
        if (m_puppetID[0] == id)
            return 0;
        if (index != 1)
            goto checkSlot1;
    }
    if (m_puppetID[2] == id)
        return 0;

doWrite:
    K2_ASSUME(numPuppets >= 0 && numPuppets < 2);
    m_puppetIndex[numPuppets] = index;
    m_puppetID[index] = id;

    CSWCCreature* pClientCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(id));
    CSWSCreature* pServerCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(id);
    if (pClientCreature != 0)
        pClientCreature->m_bIsPuppet = 1;
    if (pServerCreature != 0)
        pServerCreature->SetIsPuppet(1);

    m_nNumPuppets++;
    return 1;
}
#endif

int CSWPartyTable::AddNPC(int index, CSWSCreature* pCreature)
{
#if KOTOR1
    if (static_cast<unsigned>(index) > 8 || pCreature == 0)
#else
    if (static_cast<unsigned>(index) > 11 || pCreature == 0)
#endif
        return 0;
    if (m_npcAvailable[index] != 0)
        return 0;
    m_npcAvailable[index] = 1;

    TransferInventory(pCreature);

    C2DA* pXPTable = g_pRules->m_p2DArrays->m_pNPC;
    int found;
    int percent;
    {
        CExoString col("PercentXP");
        found = pXPTable->GetINTEntry(index, col, &percent);
    }
    float pctXP = 1.0f;
    if (found != 0 && percent != 0)
        pctXP = static_cast<float>(percent) / 100.0f;
    int baseXP = static_cast<int>(pctXP * static_cast<float>(m_nXPPool));

    unsigned char lvl = pCreature->m_pStats->GetLevel(0);
    int totalXP;
    if (lvl != 0)
    {
        int i = 0;
        int step = 0;
        totalXP = 0;
        unsigned char curLevel;
        do
        {
            totalXP += step;
            curLevel = pCreature->m_pStats->GetLevel(0);
            i++;
            step += 1000;
        } while (i < curLevel);
    }
    else
    {
        totalXP = 0;
    }

#if KOTOR1
    pCreature->m_nJoiningXP = (totalXP > baseXP) ? totalXP - baseXP : 0;
#else
    int xpToGive = (totalXP > baseXP) ? totalXP - baseXP : baseXP;

    pCreature->m_nJoiningXP = 0;
    pCreature->ReceiveExperience(xpToGive);
#endif

    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pPlayer = pServerApp->GetPlayerCreature();
    if (pPlayer != 0 && pPlayer->GetFaction() != 0)
        pPlayer->GetFaction()->AddMember(pCreature->m_objectId, 0);

    pCreature->m_pStats->SetMovementRate(0);
#if !KOTOR1
    pCreature->HandleAlignmentInfluence();
#endif

    CExoString gipPath;
    {
        CExoString availName;
        availName.Format(const_cast<char*>("AVAILNPC%d"), index);
        char* pszAvailName = availName.CStr();
        gipPath.Format(const_cast<char*>("GAMEINPROGRESS:%s"), pszAvailName);
    }

    CResStruct topStruct;
    CResGFF* pGff = new CResGFF();
    {
        CExoString fileType("UTC ");
        CExoString version("V2.0");
        pGff->CreateGFFFile(&topStruct, fileType, version);
    }
    pCreature->SaveCreature(pGff, &topStruct);

    int result;
    {
        CExoString finalPath(gipPath.CStr());
        result = pGff->WriteGFFFile(finalPath, RESTYPE_UTC);
    }
    delete pGff;
    return result;
}

int CSWPartyTable::AddNPC(int index, const CExoString& name)
{
#if KOTOR1
    if (static_cast<unsigned>(index) > 8 || m_npcAvailable[index] != 0)
#else
    if (static_cast<unsigned>(index) > 11 || m_npcAvailable[index] != 0)
#endif
        return 0;

    CSWSCreature* pCreature = new CSWSCreature(OBJECT_INVALID, 0);
    CResRef resRef(name);
    if (pCreature->LoadFromTemplate(resRef, 0))
    {
        int result = AddNPC(index, pCreature);
        delete pCreature;
        return result;
    }
    delete pCreature;
    return 0;
}

#if !KOTOR1
int CSWPartyTable::AddPUP(int index, CSWSCreature* pCreature)
{
    if (static_cast<unsigned>(index) > 2 || pCreature == 0)
        return 0;
    if (m_pupAvailable[index] != 0)
        return 0;
    m_pupAvailable[index] = 1;

    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    pServerApp->GetPlayerCreature();
    CSWSFaction* pFaction = g_pAppManager->m_pServerExoApp->GetFactionManager()->GetFaction(STANDARD_FACTION_PARTYPUPPET);
    pFaction->AddMember(pCreature->m_objectId, 0);

    pCreature->m_pStats->SetMovementRate(6);

    CExoString gipPath;
    {
        CExoString availName;
        availName.Format(const_cast<char*>("AVAILPUP%d"), index);
        char* pszAvailName = availName.CStr();
        gipPath.Format(const_cast<char*>("GAMEINPROGRESS:%s"), pszAvailName);
    }

    CResStruct topStruct;
    CResGFF* pGff = new CResGFF();
    {
        CExoString fileType("UTC ");
        CExoString version("V2.0");
        pGff->CreateGFFFile(&topStruct, fileType, version);
    }
    pCreature->SaveCreature(pGff, &topStruct);

    int result;
    {
        CExoString finalPath(gipPath.CStr());
        result = pGff->WriteGFFFile(finalPath, RESTYPE_UTC);
    }
    delete pGff;
    return result;
}
#endif

#if !KOTOR1
int CSWPartyTable::AddPUP(int index, const CExoString& name)
{
    if (static_cast<unsigned>(index) > 2 || m_pupAvailable[index] != 0)
        return 0;

    CSWSCreature* pCreature = new CSWSCreature(OBJECT_INVALID, 0);
    CResRef resRef(name);
    if (pCreature->LoadFromTemplate(resRef, 0))
    {
        int result = AddPUP(index, pCreature);
        delete pCreature;
        return result;
    }
    delete pCreature;
    return 0;
}
#endif

void CSWPartyTable::TransferInventory(CSWSCreature* pCreature)
{
    if (pCreature == 0)
        return;

    int origPartyFlag = pCreature->m_bInParty;
    pCreature->SetInParty(0, 0);

    m_nGold += pCreature->GetGold();
#if !KOTOR1
    m_nItemComponent += pCreature->GetComponent();
    m_nItemChemical += pCreature->GetChemical();
#endif

    CItemRepository* pSrcRepo = pCreature->GetItemRepository(1);
    pCreature->SetInParty(origPartyFlag, 0);

    if (m_pItemRepository == 0)
        m_pItemRepository = new CItemRepository(g_pAppManager->m_pServerExoApp->GetPlayerCreatureId(), 1);

    for (unsigned int i = 0; i < pSrcRepo->ItemListGetCount(); i++)
    {
        CSWSItem* pItem = pSrcRepo->ItemListGetItem(i);
        if (pItem == 0)
            continue;
        pItem->SetPossessor(CGameObjectArray::MAXCHAROBJID, 0, 0, OBJECT_INVALID);
        m_pItemRepository->AddItem(&pItem, 1, 0, 1);
    }
    pSrcRepo->ClearItems();
}

unsigned long long CSWPartyTable::GetNPCObject(int index, int create, int notify)
{
#if KOTOR1
    if (static_cast<unsigned>(index) >= 9 || m_npcAvailable[index] == 0)
#else
    if (static_cast<unsigned>(index) >= 12 || m_npcAvailable[index] == 0)
#endif
        return OBJECT_INVALID;

    if (m_partyMemberID[index] != OBJECT_INVALID)
        return m_partyMemberID[index];

    if (create == 0)
        return OBJECT_INVALID;

    CSWSCreature* pCreature = new CSWSCreature(OBJECT_INVALID, 0);

    AddGameInProgress();

    int loaded;
    {
        CExoString availName;
        availName.Format(const_cast<char*>("AVAILNPC%d"), index);
        CResRef resRef(availName);
        loaded = pCreature->LoadFromTemplate(resRef, 0);
    }

    if (loaded == 0)
    {
        delete pCreature;
        return OBJECT_INVALID;
    }

    RemoveGameInProgress();

    if (notify != 0)
    {
        short liveness = pCreature->GetCurrentHitPoints(0);
        if (liveness < 1)
        {
            pCreature->m_bRaiseable = 1;
            CGameEffect* pEffect = new CGameEffect(1);
            pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RESURRECTION);
            pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | CGameEffect::DURATION_TYPE_INSTANTANEOUS;
            pCreature->ApplyEffect(pEffect, 0, 0);
        }
    }

    m_partyMemberID[index] = pCreature->m_objectId;

#if !KOTOR1
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    CSWSObject* pObj = static_cast<CSWSObject*>(pCreature->AsSWSObject());
    pAIMaster->SetAILevel(pObj, 3);
#endif

    return pCreature->m_objectId;
}

#if !KOTOR1
unsigned long long CSWPartyTable::GetPUPObject(int index, int create, int notify)
{
    if (static_cast<unsigned>(index) > 2 || m_pupAvailable[index] == 0)
        return OBJECT_INVALID;

    if (m_puppetID[index] != OBJECT_INVALID)
        return m_puppetID[index];

    if (create == 0)
        return OBJECT_INVALID;

    CSWSCreature* pCreature = new CSWSCreature(OBJECT_INVALID, 0);

    AddGameInProgress();

    int loaded;
    {
        CExoString availName;
        availName.Format(const_cast<char*>("AVAILPUP%d"), index);
        CResRef resRef(availName);
        loaded = pCreature->LoadFromTemplate(resRef, 0);
    }

    if (loaded == 0)
    {
        delete pCreature;
        return OBJECT_INVALID;
    }

    RemoveGameInProgress();

    if (notify != 0)
    {
        short liveness = pCreature->GetCurrentHitPoints(0);
        if (liveness < 1)
        {
            pCreature->m_bRaiseable = 1;
            CGameEffect* pEffect = new CGameEffect(1);
            pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RESURRECTION);
            pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | CGameEffect::DURATION_TYPE_INSTANTANEOUS;
            pCreature->ApplyEffect(pEffect, 0, 0);
        }
    }

    m_puppetID[index] = pCreature->m_objectId;

    return pCreature->m_objectId;
}
#endif

#if !KOTOR1
unsigned long long CSWPartyTable::SpawnPUP(int index, int usePos, const Vector& pos, const Vector& orient, int notify)
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    unsigned long long pupId = GetPUPObject(index, 1, notify);
    CSWSCreature* pCreature = pServerApp->GetCreatureByGameObjectID(pupId);
    if (pCreature != 0)
    {
        int savedFlag = pCreature->m_bCommandable;
        pCreature->m_bCommandable = 1;
        pCreature->ClearAllActions(1);
        pCreature->m_bCommandable = savedFlag;

        CSWSArea* pArea = pServerApp->GetModule()->GetArea();

        Vector safePos = {};
        Vector posVec = {};
        Vector orientVec = {};

        const Vector* pOrientSrc = &orient;
        if (usePos != 0)
        {
            posVec = pos;
        }
        else
        {
            posVec = pCreature->m_position;
            pOrientSrc = &pCreature->m_orientation;
        }
        orientVec = *pOrientSrc;

        if (pArea->GetRoom(posVec, 0, 0) != 0)
            pArea->ComputeSafeLocation(posVec, 20.0f, pCreature->m_pPathfindInfo, 1, &safePos);

        pCreature->AddToArea(pArea, safePos.x, safePos.y, safePos.z, 1, 0);
        pCreature->SetOrientation(orientVec);
    }
    return pupId;
}
#endif

unsigned long long CSWPartyTable::SpawnNPC(int index, int usePos, const Vector& pos, const Vector& orient, int notify)
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    unsigned long long npcId = GetNPCObject(index, 1, notify);
    CSWSCreature* pCreature = pServerApp->GetCreatureByGameObjectID(npcId);
    if (pCreature != 0)
    {
#if !KOTOR1
        CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
        if (pPlayer != 0 && pPlayer->GetFaction() != 0)
            pPlayer->GetFaction()->AddMember(pCreature->m_objectId, 0);
#endif

        int savedFlag = pCreature->m_bCommandable;
        pCreature->m_bCommandable = 1;
        pCreature->ClearAllActions(1);
        pCreature->m_bCommandable = savedFlag;

        CSWSArea* pArea = pServerApp->GetModule()->GetArea();

        Vector safePos = {};
        Vector posVec = {};
        Vector orientVec = {};

        const Vector* pOrientSrc = &orient;
        if (usePos != 0)
        {
            posVec = pos;
        }
        else
        {
            posVec = pCreature->m_position;
            pOrientSrc = &pCreature->m_orientation;
        }
        orientVec = *pOrientSrc;

        if (pArea->GetRoom(posVec, 0, 0) != 0)
            pArea->ComputeSafeLocation(posVec, 20.0f, pCreature->m_pPathfindInfo, 1, &safePos);

        pCreature->AddToArea(pArea, safePos.x, safePos.y, safePos.z, 1, 0);
        pCreature->SetOrientation(orientVec);

        C2DA* pXPTable = g_pRules->m_p2DArrays->m_pNPC;
        int found;
        int percent;
        {
            CExoString col("PercentXP");
            found = pXPTable->GetINTEntry(index, col, &percent);
        }
        float pctXP = 1.0f;
        if (found != 0 && percent != 0)
            pctXP = static_cast<float>(percent) / 100.0f;

        int earnedGap = pCreature->m_pStats->m_nExperience - pCreature->m_nJoiningXP;
        if (earnedGap < static_cast<int>(pctXP * static_cast<float>(m_nXPPool)))
        {
            pCreature->ReceiveExperience(m_nXPPool - static_cast<int>((static_cast<float>(earnedGap) / static_cast<float>(percent)) * 100.0f));

            CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
            if (pOptions->m_bAutoLevelUpNPCs && pCreature->m_pStats->CanLevelUp() != 0)
                pCreature->m_pStats->AutoLevelup(1);
        }
#if !KOTOR1
        pCreature->HandleAlignmentInfluence();
#endif
    }
    return npcId;
}

// Re-seats the live party at the player's position, for example after a module transition:
// resets every member's follow-path data off the player's position and facing, then places
// each companion at its formation slot around its follow point, rotated by the follow path
// sample's heading, snapped to walkable ground, and finally re-seats each puppet on its
// owner. Clears solo mode and unstealths the party.
void CSWPartyTable::PositionParty()
{
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pPlayer = pServerApp->GetPlayerCreature();
    Vector orient = pPlayer->m_orientation;
    pParty->ResetAllFollowData(pPlayer->m_position, Yaw(orient), 0);
    if (m_bKeepSavedPositions == 0)
    {
        Vector safePos = {};
#if !KOTOR1
        pParty->ResetAllFollowData(pPlayer->m_position, Yaw(orient), 0);
#endif
        for (int i = 0; i < m_nNumPartyMembers; i++)
        {
            Vector offset = pParty->m_members[i + 1].m_formationOffset;
            Vector followPoint = pParty->m_members[i + 1].m_followPoint;
            CSWPartyFollowData* pPathData = pParty->m_followData.GetPathData(pParty->m_members[i + 1].m_bFollowPointValid);
            Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(pPathData->m_fLeaderYaw));
            float w = q.w;
            if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f))
            {
                float x = q.x, y = q.y, z = q.z;
                float xx = x * x, yy = y * y, zz = z * z;
                float xy = x * y, xz = x * z, yz = y * z;
                float wx = w * x, wy = w * y, wz = w * z;
                Vector r;
                r.x = offset.x * (1.0f - ((yy + zz) + (yy + zz))) + offset.y * ((xy - wz) + (xy - wz)) + offset.z * ((xz + wy) + (xz + wy));
                r.y = offset.x * ((xy + wz) + (xy + wz)) + offset.y * (1.0f - ((xx + zz) + (xx + zz))) + offset.z * ((yz - wx) + (yz - wx));
                r.z = offset.x * ((xz - wy) + (xz - wy)) + offset.y * ((yz + wx) + (yz + wx)) + offset.z * (1.0f - ((xx + yy) + (xx + yy)));
                offset = r;
            }
            safePos.x = followPoint.x + offset.x;
            safePos.y = followPoint.y + offset.y;
            safePos.z = followPoint.z + offset.z;
            int index = m_partyMemberIndex[i];
            unsigned long long memberId;
            if (static_cast<unsigned int>(index) < NUM_NPCS && m_npcAvailable[index] != 0)
                memberId = m_partyMemberID[index];
            else
                memberId = OBJECT_INVALID;
            g_pAppManager->m_pServerExoApp->ComputeSafeLocation(safePos, 10.0f, memberId, 0, &safePos);
            CSWSCreature* pMember = pServerApp->GetCreatureByGameObjectID(memberId);
            pMember->SetPosition(safePos, 1, 1, 0);
            pParty->m_members[i + 1].m_safePosition = safePos;
            pMember->SetOrientation(orient);
        }
#if !KOTOR1
        for (int i = 0; i < m_nNumPuppets; i++)
        {
            CSWSCreature* pOwner = GetPUPOwner(m_puppetIndex[i]);
            if (pOwner != 0)
            {
                CSWSCreature* pPuppet = pServerApp->GetCreatureByGameObjectID(m_puppetID[m_puppetIndex[i]]);
                if (pPuppet != 0)
                {
                    pServerApp->ComputeSafeLocation(pOwner->m_position, 10.0f, m_puppetID[m_puppetIndex[i]], 0, &safePos);
                    pPuppet->SetPosition(safePos, 1, 1, 0);
                    pPuppet->SetOrientation(pOwner->m_orientation);
                }
            }
        }
#endif
        UnstealthParty(0);
        m_soloMode = 0;
    }
    m_bKeepSavedPositions = 0;
}

// Switches which creature is the player character. index >= 0 makes that NPC table slot the
// new player: the current one is written out to the in-progress template when it is the real
// player, or un-flagged and saved back to its slot when it is another NPC; the new creature
// is resolved or template-created, installed with the party and is-player flags, handed the
// shared items, moved to the old position when freshly spawned, and made the player object;
// every remaining member is then removed and killed. index == -1 switches back to the real
// player, template-loading it under the in-progress mount that the other path wrote.
int CSWPartyTable::SwitchPlayerCharacter(int index)
{
    CExoString saveName;
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CExoLinkedList<CSWSClient>* ppList = pServerApp->GetPlayerList();
    CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppList->m_pInternal->m_pHead ? ppList->m_pInternal->m_pHead->m_pData : 0);
    K2_ASSUME(pPlayer != 0);
    CSWSCreature* pOldPC = pServerApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
    CSWSArea* pArea = static_cast<CSWSArea*>(pOldPC->GetArea());
    Vector orient = {};
    Vector pos = {};
    bool bFromTemplate = false;
    CSWSCreature* pNewPC;
    unsigned long long newId;
    CSWParty* pParty;
    int curPCIndex = m_nControlledNPC;
    if (index == -1)
    {
        if (curPCIndex == -1)
            return 1;
#if KOTOR1
        CSWSCreature* pCurNPC = pServerApp->GetPlayerCreature();
#else
        CSWSCreature* pCurNPC = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_partyMemberID[curPCIndex]);
#endif
        pCurNPC->m_pStats->m_bIsPC = 0;
        pCurNPC->m_bPlayerCharacter = 0;
        SaveMember(m_nControlledNPC, 0);
        pNewPC = new CSWSCreature(OBJECT_INVALID, 0);
        AddGameInProgress();
        if (pNewPC->LoadFromTemplate(CResRef("PC"), 0) == 0)
        {
            delete pNewPC;
            return 0;
        }
        RemoveGameInProgress();
        pos = pOldPC->m_position;
        orient = pOldPC->m_orientation;
        bFromTemplate = true;
        KillNPCObject(m_nControlledNPC, 1);
    }
    else
    {
        if (static_cast<unsigned>(index) >= NUM_NPCS || m_npcAvailable[index] == 0)
            return 0;
        if (curPCIndex == index)
            return 1;
        if (curPCIndex == -1)
        {
            saveName = "GAMEINPROGRESS:PC";
            CResStruct topStruct;
            CResGFF* pGff = new CResGFF();
            {
                CExoString fileType("UTC ");
                CExoString version("V2.0");
                pGff->CreateGFFFile(&topStruct, fileType, version);
            }
            pOldPC->SaveCreature(pGff, &topStruct);
            int written;
            {
                CExoString finalPath(saveName.CStr());
                written = pGff->WriteGFFFile(finalPath, RESTYPE_UTC);
            }
            if (written == 0)
                return 0;
        }
        else
        {
            CSWSCreature* pCurPC = pServerApp->GetPlayerCreature();
            pCurPC->m_pStats->m_bIsPC = 0;
            pCurPC->m_bPlayerCharacter = 0;
            SaveMember(m_nControlledNPC, 0);
        }
        if (m_npcAvailable[index] != 0)
            newId = m_partyMemberID[index];
        else
            newId = OBJECT_INVALID;
        pNewPC = pServerApp->GetCreatureByGameObjectID(newId);
        if (pNewPC == 0)
        {
            pNewPC = pServerApp->GetCreatureByGameObjectID(GetNPCObject(index, 1, 1));
            pos = pOldPC->m_position;
            orient = pOldPC->m_orientation;
            bFromTemplate = true;
        }
        if (m_nControlledNPC == -1)
        {
            CSWCObject* pClientObj = static_cast<CSWCObject*>(pOldPC->GetClientCreature());
            if (pClientObj != 0)
            {
                pClientObj->SetFadeState(0);
                pClientObj->SetDesiredFadeState(0, 1, 0, 0);
                pClientObj->m_bPendingRemoval = 1;
            }
            if (pOldPC != 0)
                delete pOldPC;
        }
        else
        {
            KillNPCObject(m_nControlledNPC, 1);
        }
        if (pNewPC == 0)
            return 0;
    }
    pParty = pClientApp->GetSWParty();
    pParty->ClearParty();
    pNewPC->SetInParty(1, 1);
    pNewPC->m_pStats->m_bIsPC = 1;
    pNewPC->m_bPlayerCharacter = 1;
    m_nLeaderIndex = -1;
    CItemRepository* pRepo = pNewPC->GetItemRepository(1);
    if (pRepo != 0 && pRepo->ItemListGetCount() != 0)
    {
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
        {
            CSWSItem* pItem = m_pItemRepository->ItemListGetItem(i);
            if (pItem != 0)
                pItem->SetPossessor(pNewPC->m_objectId, 0, 0, OBJECT_INVALID);
        }
    }
    if (bFromTemplate)
    {
        pNewPC->AddToArea(pArea, pos.x, pos.y, pos.z, 0, 0);
        pNewPC->SetOrientation(orient);
    }
    pPlayer->m_gameObjectId = pNewPC->m_objectId;
    pPlayer->SetGameObject(pNewPC);
    for (int64_t i = static_cast<int64_t>(m_nNumPartyMembers); i > 0; i--)
    {
        int memberIndex = m_partyMemberIndex[i - 1];
        RemoveMember(memberIndex);
        KillNPCObject(memberIndex, 1);
    }
    m_nControlledNPC = index;
    return 1;
}

// Materializes the party after a new game or module load, one-shot and gated on m_bPartyCreated:
// mounts the in-progress resource directory, respawns every member and puppet at the default
// position (notifying only when this is not a save load), re-points the current-player slot
// at the real player, rebuilds the shared item repository from the in-progress inventory
// when one is present, defaults the save-table name to the player's full name, then re-runs
// the inventory transfer and the clothing force-equip for the player and every roster slot.
void CSWPartyTable::CreateParty()
{
#if KOTOR1
    // K1: a controlled NPC replaces the member respawn rather than preceding it, the
    // load-from-save flag is re-read per member, and there are no puppets or save-table name.
    if (m_bPartyCreated != 0)
        return;
    AddGameInProgress();
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    pClientApp->GetSWParty();  // sic: return value unused
    if (m_nControlledNPC != -1)
        m_partyMemberID[m_nControlledNPC] = pServerApp->GetPlayerCreatureId();
    else
    {
        for (uint64_t i = 1; i - 1 < static_cast<unsigned>(m_nNumPartyMembers); i++)
        {
            unsigned long long npcId = SpawnNPC(m_partyMemberIndex[i - 1], 0, Vector{}, Vector{}, g_pAppManager->m_pServerExoApp->GetLoadFromSaveGame() == 0);
            CSWCCreature* pClient = pClientApp->GetCreatureByGameObjectID(pClientApp->ServerToClientObjectId(npcId));
            CSWSCreature* pServer = pServerApp->GetCreatureByGameObjectID(npcId);
            if (pClient != 0)
                pClient->SetInParty(1);
            if (pServer != 0)
                pServer->SetInParty(1, 1);
        }
    }
    CResStruct topStruct;
    CResList itemList;
    CResStruct elemStruct;
    CResGFF* pGff = new CResGFF(0, const_cast<char*>("INV "), CResRef("INVENTORY"));
    if (pGff->GetData() != 0)
    {
        if (m_pItemRepository == 0)
            m_pItemRepository = new CItemRepository(pServerApp->GetPlayerCreatureId(), 1);
        else
        {
            m_pItemRepository->ClearItems();
            m_bPartyCreated = 0;
        }
        pGff->GetTopLevelStruct(&topStruct);
        pGff->GetList(&itemList, &topStruct, const_cast<char*>("ItemList"));
        for (unsigned int i = 0; i < pGff->GetListCount(&itemList); i++)
        {
            CSWSItem* pItem = 0;
            pGff->GetListElement(&elemStruct, &itemList, i);
            pItem = new CSWSItem(OBJECT_INVALID);
            if (pItem->LoadItem(pGff, &elemStruct) != 0)
            {
                if (m_pItemRepository->AddItem(&pItem, 1, 0, 0) != 0 && pItem != 0)
                    pItem->SetPossessor(pServerApp->GetPlayerCreatureId(), 0, 0, OBJECT_INVALID);
            }
            else if (pItem != 0)
                delete pItem;
        }
    }
    delete pGff;
    RemoveGameInProgress();
    CSWSCreature* pPlayer = pServerApp->GetPlayerCreature();
    if (pPlayer != 0)
        TransferInventory(pPlayer);
    for (int i = 0; i < 9; i++)
    {
        CSWSCreature* pMember = pServerApp->GetCreatureByGameObjectID(m_partyMemberID[i]);
        if (pMember != 0)
            TransferInventory(pMember);
    }
    CSWSCreature* pPC = pServerApp->GetPlayerCreature();
    if (pPC != 0)
        pPC->ForceEquipClothing();
    for (int i = 0; i < 9; i++)
    {
        CSWSCreature* pMember = pServerApp->GetCreatureByGameObjectID(m_partyMemberID[i]);
        if (pMember != 0)
            pMember->ForceEquipClothing();
    }
    m_bPartyCreated = 1;
#else
    if (m_bPartyCreated != 0)
        return;
    AddGameInProgress();
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    pClientApp->GetSWParty();  // sic: return value unused
    int loadFromSaveGame = g_pAppManager->m_pServerExoApp->GetLoadFromSaveGame();
    if (m_nControlledNPC != -1)
        m_partyMemberID[m_nControlledNPC] = pServerApp->GetPlayerCreatureId();
    for (uint64_t i = 2; i - 2 < static_cast<unsigned>(m_nNumPartyMembers); i++)
    {
        unsigned long long npcId = SpawnNPC(m_partyMemberIndex[i - 2], 0, Vector{}, Vector{}, loadFromSaveGame == 0);
        CSWCCreature* pClient = pClientApp->GetCreatureByGameObjectID(pClientApp->ServerToClientObjectId(npcId));
        CSWSCreature* pServer = pServerApp->GetCreatureByGameObjectID(npcId);
        if (pClient != 0)
            pClient->SetInParty(1);
        if (pServer != 0)
            pServer->SetInParty(1, 1);
    }
    for (uint64_t i = 4; i - 4 < static_cast<unsigned>(m_nNumPuppets); i++)
    {
        unsigned long long pupId = SpawnPUP(m_puppetIndex[i - 4], 0, Vector{}, Vector{}, loadFromSaveGame == 0);
        CSWCCreature* pClient = pClientApp->GetCreatureByGameObjectID(pClientApp->ServerToClientObjectId(pupId));
        CSWSCreature* pServer = pServerApp->GetCreatureByGameObjectID(pupId);
        if (pClient != 0)
            pClient->m_bIsPuppet = 1;
        if (pServer != 0)
            pServer->SetIsPuppet(1);
    }
    CResStruct topStruct;
    CResList itemList;
    CResStruct elemStruct;
    CResGFF* pGff = new CResGFF(0, const_cast<char*>("INV "), CResRef("INVENTORY"));
    if (pGff->GetData() != 0)
    {
        if (m_pItemRepository == 0)
            m_pItemRepository = new CItemRepository(pServerApp->GetPlayerCreatureId(), 1);
        else
        {
            m_pItemRepository->ClearItems();
            m_bPartyCreated = 0;
        }
        pGff->GetTopLevelStruct(&topStruct);
        pGff->GetList(&itemList, &topStruct, const_cast<char*>("ItemList"));
        for (unsigned int i = 0; i < pGff->GetListCount(&itemList); i++)
        {
            CSWSItem* pItem = 0;
            pGff->GetListElement(&elemStruct, &itemList, i);
            pItem = new CSWSItem(OBJECT_INVALID);
            if (pItem->LoadItem(pGff, &elemStruct) != 0)
            {
                if (m_pItemRepository->AddItem(&pItem, 1, 0, 0) != 0 && pItem != 0)
                    pItem->SetPossessor(pServerApp->GetPlayerCreatureId(), 0, 0, OBJECT_INVALID);
            }
            else if (pItem != 0)
                delete pItem;
        }
    }
    delete pGff;
    RemoveGameInProgress();
    CSWSCreature* pPlayer = pServerApp->GetPlayerCreature();
    if (pPlayer != 0)
    {
        TransferInventory(pPlayer);
        if (m_saveTableName == "")
        {
            CExoString firstName;
            CExoString lastName;
            CExoString fullName;
            pPlayer->GetFirstName().GetString(0, &firstName, 0);
            pPlayer->GetLastName().GetString(0, &lastName, 0);
            if (lastName.GetLength() >= 1)
                fullName = firstName + " " + lastName;
            else
                fullName = firstName;
            m_saveTableName = fullName;
        }
    }
    for (int i = 0; i < 12; i++)
    {
        CSWSCreature* pMember = pServerApp->GetCreatureByGameObjectID(m_partyMemberID[i]);
        if (pMember != 0)
            TransferInventory(pMember);
    }
    CSWSCreature* pPC = pServerApp->GetPlayerCreature();
    if (pPC != 0)
        pPC->ForceEquipClothing();
    for (int i = 0; i < 12; i++)
    {
        CSWSCreature* pMember = pServerApp->GetCreatureByGameObjectID(m_partyMemberID[i]);
        if (pMember != 0)
            pMember->ForceEquipClothing();
    }
    m_bPartyCreated = 1;
#endif
}
