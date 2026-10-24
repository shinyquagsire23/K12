#include "k2stdint.h"
#include "nwsaiaction.h"
#include "serverexoappinternal.h"

CSWSModule* CServerExoApp::GetModule()
{
    return m_pInternal->GetModule();
}

CExoString CServerExoApp::GetModuleName()
{
    return m_pInternal->GetModuleName();
}

#if KOTOR1
int CServerExoApp::SaveGame(unsigned int a, const CExoString& b, CSWSPlayer* c, int d)
{
    return m_pInternal->SaveGame(a, b, c, d);
}
#endif

int CServerExoApp::SaveGame(unsigned int a, const CExoString& b, CSWSPlayer* c, int d, int e)
{
    return m_pInternal->SaveGame(a, b, c, d, e);
}

void CServerExoApp::SendCharacterQuery(CSWSPlayer* a)
{
    m_pInternal->SendCharacterQuery(a);
}

int CServerExoApp::LoadGame(unsigned int a, const CExoString& b, const CExoString& c, CSWSPlayer* d)
{
    return m_pInternal->LoadGame(a, b, c, d);
}

void CServerExoApp::StartNewModule(const CExoString& a)
{
    m_pInternal->StartNewModule(a);
}

void CServerExoApp::ExportAllPlayers()
{
    m_pInternal->ExportAllPlayers();
}

void CServerExoApp::LoadCharacterFinish(CSWSPlayer* a, int b)
{
    m_pInternal->LoadCharacterFinish(a, b);
}

void CServerExoApp::LoadPrimaryPlayer(CSWSPlayer* a)
{
    m_pInternal->LoadPrimaryPlayer(a);
}

CSWSItem* CServerExoApp::GetItemByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetItemByGameObjectID(a);
}

CSWSCreature* CServerExoApp::GetCreatureByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetCreatureByGameObjectID(a);
}

void CServerExoApp::GetModuleByGameObjectID(unsigned long long a)
{
    m_pInternal->GetModuleByGameObjectID(a);
}

CSWSArea* CServerExoApp::GetAreaByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetAreaByGameObjectID(a);
}

CSWSTrigger* CServerExoApp::GetTriggerByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetTriggerByGameObjectID(a);
}

CSWSPlaceable* CServerExoApp::GetPlaceableByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetPlaceableByGameObjectID(a);
}

CSWSStore* CServerExoApp::GetStoreByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetStoreByGameObjectID(a);
}

CSWSDoor* CServerExoApp::GetDoorByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetDoorByGameObjectID(a);
}

CSWSAreaOfEffectObject* CServerExoApp::GetAreaOfEffectByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetAreaOfEffectByGameObjectID(a);
}

CSWSWaypoint* CServerExoApp::GetWaypointByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetWaypointByGameObjectID(a);
}

CSWSEncounter* CServerExoApp::GetEncounterByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetEncounterByGameObjectID(a);
}

CSWSSoundObject* CServerExoApp::GetSoundObjectByGameObjectID(unsigned long long a)
{
    return m_pInternal->GetSoundObjectByGameObjectID(a);
}

void CServerExoApp::GetPlayerIDByGameObjectID(unsigned long long a)
{
    m_pInternal->GetPlayerIDByGameObjectID(a);
}

unsigned int CServerExoApp::GetPlayerLanguage(unsigned int a)
{
    return m_pInternal->GetPlayerLanguage(a);
}

int CServerExoApp::GetModuleLanguage()
{
    return m_pInternal->GetModuleLanguage();
}

CWorldTimer* CServerExoApp::GetActiveTimer(unsigned long long a)
{
    return static_cast<CWorldTimer*>(m_pInternal->GetActiveTimer(a));
}

void CServerExoApp::HandleMessage(unsigned int a, unsigned char* b, unsigned int c, int d)
{
    m_pInternal->HandleMessage(a, b, c, d);
}

void CServerExoApp::Initialize()
{
    m_pInternal->Initialize();
}

int CServerExoApp::MainLoop()
{
    return m_pInternal->MainLoop();
}

void CServerExoApp::PlayerListChange(unsigned int a, int b, int c)
{
    m_pInternal->PlayerListChange(a, b, c);
}

void CServerExoApp::DumpServerOptionsToLog()
{
    m_pInternal->DumpServerOptionsToLog();
}

void CServerExoApp::ContinueMessageProcessing()
{
    m_pInternal->ContinueMessageProcessing();
}

void CServerExoApp::Shutdown(int a, int b)
{
    m_pInternal->Shutdown(a, b);
}

void CServerExoApp::UnloadModule()
{
    m_pInternal->UnloadModule();
}

void CServerExoApp::LoadModule(const CExoString& a, CSWSPlayer* b)
{
    m_pInternal->LoadModule(a, b);
}

void CServerExoApp::RunModule()
{
    m_pInternal->RunModule();
}

CExoString CServerExoApp::GetPlayerListString()
{
    return m_pInternal->GetPlayerListString();
}

void CServerExoApp::OnCDChange()
{
    m_pInternal->OnCDChange();
}

void CServerExoApp::OnExit()
{
    m_pInternal->OnExit();
}

void CServerExoApp::OnGainFocus()
{
    m_pInternal->OnGainFocus();
}

void CServerExoApp::LoadCharacterStart(unsigned char a, CSWSPlayer* b, CResRef c, void* d, unsigned int e)
{
    m_pInternal->LoadCharacterStart(a, b, c, d, e);
}

void CServerExoApp::OnLostFocus()
{
    m_pInternal->OnLostFocus();
}

void CServerExoApp::OnVideoChange()
{
    m_pInternal->OnVideoChange();
}

void CServerExoApp::Uninitialize()
{
    m_pInternal->Uninitialize();
}

void CServerExoApp::StartServices(int a)
{
    m_pInternal->StartServices(a);
}

void CServerExoApp::StopServices()
{
    m_pInternal->StopServices();
}

CSWSPlayer* CServerExoApp::GetClientObjectByObjectId(unsigned long long a)
{
    return m_pInternal->GetClientObjectByObjectId(a);
}

void* CServerExoApp::GetClientObjectByPlayerId(unsigned int a, unsigned char b)
{
    return m_pInternal->GetClientObjectByPlayerId(a, b);
}

void CServerExoApp::ValidatePlayerLogin(void* a)
{
    m_pInternal->ValidatePlayerLogin(a);
}

void CServerExoApp::InitiateModuleForPlayer(void* a)
{
    m_pInternal->InitiateModuleForPlayer(a);
}

void CServerExoApp::MovePlayerToArea(void* a)
{
    m_pInternal->MovePlayerToArea(a);
}

int CServerExoApp::GetFactionOfObject(unsigned long long a, int* b)
{
    return m_pInternal->GetFactionOfObject(a, b);
}

void CServerExoApp::TogglePauseState(unsigned char a)
{
    m_pInternal->TogglePauseState(a);
}

void CServerExoApp::SkipPastDialog(unsigned long long a)
{
    m_pInternal->SkipPastDialog(a);
}

unsigned long long CServerExoApp::ClientToServerObjectId(unsigned long long a)
{
    return m_pInternal->ClientToServerObjectId(a);
}

int CServerExoApp::GetPauseState(unsigned char a)
{
    return m_pInternal->GetPauseState(a);
}

unsigned char CServerExoApp::GetActivePauseState()
{
    return m_pInternal->GetActivePauseState();
}

void CServerExoApp::SetPauseState(unsigned char a, int b)
{
    m_pInternal->SetPauseState(a, b);
}

void CServerExoApp::AddToExclusionList(unsigned long long a, unsigned char b)
{
    m_pInternal->AddToExclusionList(a, b);
}

void CServerExoApp::RemoveFromExclusionList(unsigned long long a, unsigned char b)
{
    m_pInternal->RemoveFromExclusionList(a, b);
}

CExoLinkedList<unsigned long long>* CServerExoApp::GetActiveExclusionList()
{
    return m_pInternal->GetActiveExclusionList();
}

void CServerExoApp::IsOnActiveExclusionList(unsigned long long a)
{
    m_pInternal->IsOnActiveExclusionList(a);
}

unsigned int CServerExoApp::ResolvePlayerByFirstName(const CExoString& a)
{
    return m_pInternal->ResolvePlayerByFirstName(a);
}

int CServerExoApp::GetDifficultyOption(int a)
{
    return m_pInternal->GetDifficultyOption(a);
}

void CServerExoApp::AddCharListRequest(unsigned int a)
{
    m_pInternal->AddCharListRequest(a);
}

void CServerExoApp::IsOnExclusionList(unsigned long long a)
{
    m_pInternal->IsOnExclusionList(a);
}

unsigned long long CServerExoApp::GetFirstPCObject()
{
    return m_pInternal->GetFirstPCObject();
}

unsigned long long CServerExoApp::GetNextPCObject()
{
    return m_pInternal->GetNextPCObject();
}

void CServerExoApp::DebugValidatePosition(unsigned long long a, unsigned long long b, const Vector& c)
{
    m_pInternal->DebugValidatePosition(a, b, c);
}

void CServerExoApp::DebugValidatePath(unsigned long long a, int b)
{
    m_pInternal->DebugValidatePath(a, b);
}

void CServerExoApp::DebugValidatePath_Segment(CSWSArea* a, CPathfindInformation* b, const Vector& c, const Vector& d)
{
    m_pInternal->DebugValidatePath_Segment(a, b, c, d);
}

void CServerExoApp::ChangePlayerGameObject(unsigned long long a)
{
    m_pInternal->ChangePlayerGameObject(a);
}

unsigned long long CServerExoApp::GetPlayerCreatureId()
{
    return m_pInternal->GetPlayerCreatureId();
}

void CServerExoApp::MoveCreatureToPosition(unsigned long long a, const Vector& b, unsigned long long c)
{
    m_pInternal->MoveCreatureToPosition(a, b, c);
}

void CServerExoApp::UpdateSubareasOnMoveTo(unsigned long long a, const Vector& b, const Vector& c)
{
    m_pInternal->UpdateSubareasOnMoveTo(a, b, c);
}

int CServerExoApp::IsActionsPending(unsigned long long a)
{
    return m_pInternal->IsActionsPending(a);
}

void CServerExoApp::CreatureAcquireItem(unsigned long long a, const CResRef& b)
{
    m_pInternal->CreatureAcquireItem(a, b);
}

void CServerExoApp::RunDeathSequence()
{
    m_pInternal->RunDeathSequence();
}

void CServerExoApp::KillActiveCharacter()
{
    m_pInternal->KillActiveCharacter();
}

int CServerExoApp::TestSafeLocation(unsigned long long a, const Vector& b)
{
    return m_pInternal->TestSafeLocation(a, b);
}

void CServerExoApp::ComputeSafeLocation(const Vector& a, float b, unsigned long long c, int d, Vector* e)
{
    m_pInternal->ComputeSafeLocation(a, b, c, d, e);
}

Vector CServerExoApp::GetNormalOfPoint(const Vector& a)
{
    return m_pInternal->GetNormalOfPoint(a);
}

void CServerExoApp::ClearLastModuleFound()
{
    m_pInternal->ClearLastModuleFound();
}

void CServerExoApp::ClearAreaCache()
{
    m_pInternal->ClearAreaCache();
}

int CServerExoApp::ShrinkLineByCreatures(const Vector& a, const Vector& b, Vector& c)
{
    return m_pInternal->ShrinkLineByCreatures(a, b, c);
}

void CServerExoApp::SetMoveToWorldTime(unsigned int a, unsigned char b, unsigned char c, unsigned char d, unsigned short e, unsigned short f, unsigned short g)
{
    m_pInternal->SetMoveToWorldTime(a, b, c, d, e, f, g);
}

void CServerExoApp::GetMoveToWorldTime(unsigned int& a, unsigned char& b, unsigned char& c, unsigned char& d, unsigned short& e, unsigned short& f, unsigned short& g)
{
    m_pInternal->GetMoveToWorldTime(a, b, c, d, e, f, g);
}

int CServerExoApp::GetModulePreviouslyVisited(const CExoString& a)
{
    return m_pInternal->GetModulePreviouslyVisited(a);
}

void CServerExoApp::HasEnoughDiskSpaceForSaveGame()
{
    m_pInternal->HasEnoughDiskSpaceForSaveGame();
}

void CServerExoApp::ExecuteLoadModule(const CExoString& a)
{
    m_pInternal->ExecuteLoadModule(a);
}

unsigned long long CServerExoApp::GetSystemTimeAsDWORD64()
{
    return m_pInternal->GetSystemTimeAsDWORD64();
}

void CServerExoApp::DebugValidatePath_Point(CSWSArea* a, const Vector& b)
{
#if KOTOR1
    m_pInternal->DebugValidatePath_Point(a, b);
#else
    m_pInternal->DebugValidatePath_Point(a, b, 0);
#endif
}

int CServerExoApp::GetApplicationId()
{
    return g_pAppManager->m_appId;
}

float CServerExoApp::GetFPS()
{
    return 100.0f;
}

void CServerExoApp::ShutDownToMainMenu()
{
}

void CServerExoApp::SetApplicationIdsMatch(int a)
{
    K2_UNUSED(a);
}

unsigned short CServerExoApp::GetServerMode()
{
    return m_pInternal->m_serverMode;
}

CSWSMessage* CServerExoApp::GetSWSMessage()
{
    return m_pInternal->m_pSWSMessage;
}

CNetLayer* CServerExoApp::GetNetLayer()
{
    return m_pInternal->m_pNetLayer;
}

CServerAIMaster* CServerExoApp::GetServerAIMaster()
{
    return m_pInternal->m_pServerAIMaster;
}

CWorldTimer* CServerExoApp::GetWorldTimer()
{
    return m_pInternal->m_pWorldTimer;
}

CWorldTimer* CServerExoApp::GetTimestopTimer()
{
    return static_cast<CWorldTimer*>(m_pInternal->m_pTimestopTimer);
}

CWorldTimer* CServerExoApp::GetPauseTimer()
{
    return m_pInternal->m_pPauseTimer;
}

CFactionManager* CServerExoApp::GetFactionManager()
{
    return m_pInternal->m_pFactionManager;
}

void* CServerExoApp::GetConnectionLib()
{
    return m_pInternal->m_pConnectionLib;
}

CGameObjectArray* CServerExoApp::GetObjectArray()
{
    return m_pInternal->m_pObjectArray;
}

CExoLinkedList<CSWSPlayerListEntry>* CServerExoApp::GetPlayerList()
{
    return m_pInternal->m_pPlayerList;
}

CExoLinkedList<CSWSPlayerListEntry>* CServerExoApp::GetSysAdminList()
{
    return m_pInternal->m_pSysAdminList;
}

int CServerExoApp::GetDebugMode()
{
    return m_pInternal->m_debugMode;
}

int CServerExoApp::GetMoveToModulePending()
{
    return m_pInternal->m_moveToModulePending;
}

unsigned int CServerExoApp::GetMoveToPausedTimeOfDay()
{
    return m_pInternal->m_moveToPausedTimeOfDay;
}

unsigned int CServerExoApp::GetMoveToPausedCalendarDay()
{
    return m_pInternal->m_moveToPausedCalendarDay;
}

int CServerExoApp::GetLoadingModule()
{
    return m_pInternal->m_loadingModule;
}

int CServerExoApp::GetAutoSavePending()
{
    return m_pInternal->m_autoSavePending;
}

int CServerExoApp::GetExportAllCharactersPending()
{
    return m_pInternal->m_exportAllCharactersPending;
}

int CServerExoApp::GetLoadFromSaveGame()
{
    return m_pInternal->m_loadFromSaveGame;
}

int CServerExoApp::GetButtonMashCheck()
{
    return m_pInternal->m_buttonMashCheck;
}

int CServerExoApp::GetActionConversation()
{
    return m_pInternal->m_actionConversation;
}

int CServerExoApp::GetIgnoreConversationStartRange()
{
    return m_pInternal->m_ignoreConversationStartRange;
}

int CServerExoApp::GetAutoSaveTimer()
{
    return m_pInternal->m_autoSaveTimerDay;
}

void CServerExoApp::SetDebugMode(int a)
{
    m_pInternal->m_debugMode = a;
}

void CServerExoApp::SetMoveToModulePending(int a)
{
    m_pInternal->m_moveToModulePending = a;
}

void CServerExoApp::SetMoveToPausedTimeOfDay(unsigned int a)
{
    m_pInternal->m_moveToPausedTimeOfDay = a;
}

void CServerExoApp::SetMoveToPausedCalendarDay(unsigned int a)
{
    m_pInternal->m_moveToPausedCalendarDay = a;
}

void CServerExoApp::SetLoadingModule(int a)
{
    m_pInternal->m_loadingModule = a;
}

void CServerExoApp::SetAutoSavePending(int a)
{
    m_pInternal->m_autoSavePending = a;
}

void CServerExoApp::SetExportAllCharactersPending(int a)
{
    m_pInternal->m_exportAllCharactersPending = a;
}

void CServerExoApp::SetLoadFromSaveGame(int a)
{
    m_pInternal->m_loadFromSaveGame = a;
}

void CServerExoApp::SetButtonMashCheck(int a)
{
    m_pInternal->m_buttonMashCheck = a;
}

void CServerExoApp::SetActionConversation(int a)
{
    m_pInternal->m_actionConversation = a;
}

void CServerExoApp::SetIgnoreConversationStartRange(int a)
{
    m_pInternal->m_ignoreConversationStartRange = a;
}

void CServerExoApp::SetDoingStartNewModule(int a)
{
    m_pInternal->m_doingStartNewModule = a;
}

void CServerExoApp::SetForceUpdate()
{
    m_pInternal->m_forceUpdate = 1;
}

CExoString CServerExoApp::GetMoveToModuleString()
{
    return m_pInternal->m_moveToModuleString;
}

CExoString CServerExoApp::GetMoveToModuleStartWaypoint()
{
    return m_pInternal->m_moveToModuleStartWaypoint;
}

CExoString CServerExoApp::GetLastConversation()
{
    return m_pInternal->m_lastConversation;
}

void CServerExoApp::SetMoveToModuleString(const CExoString& a)
{
    m_pInternal->m_moveToModuleString = a;
}

void CServerExoApp::SetMoveToModuleStartWaypoint(const CExoString& a)
{
    m_pInternal->m_moveToModuleStartWaypoint = a;
}

void CServerExoApp::SetLastConversation(const CExoString& a)
{
    m_pInternal->m_lastConversation = a;
}

void CServerExoApp::SetSaveGameName(const CExoString& a)
{
    m_pInternal->m_saveGameName = a;
}

CSWGlobalVariableTable* CServerExoApp::GetGlobalVariableTable()
{
    return &m_pInternal->m_globalVariableTable;
}

CSWPartyTable* CServerExoApp::GetPartyTable()
{
    return &m_pInternal->m_partyTable;
}

void CServerExoApp::SetAutoSaveTimer(unsigned int a, int b)
{
    m_pInternal->m_autoSaveTimerDay = a;
    m_pInternal->m_autoSaveTimerFlag = b;
}

CServerInfo* CServerExoApp::GetServerInfo()
{
    return m_pInternal->m_pServerInfo;
}

// The interface types this as CExoString*, but the field it points at is a CExoLocString.
CExoString* CServerExoApp::GetModuleDescription()
{
    if (m_pInternal->GetModule() == 0)
        return 0;
    return reinterpret_cast<CExoString*>(&m_pInternal->GetModule()->m_sDescription);
}

CGameObject* CServerExoApp::GetGameObject(unsigned long long a)
{
#if KOTOR1
    return m_pInternal->GetGameObject(a);
#else
    CServerExoAppInternal* pInternal = m_pInternal;
    CGameObject* pObject = 0;
    pInternal->m_pObjectArray->GetGameObject(a, &pObject);
    return pObject;
#endif
}

int CServerExoApp::GetIsControlledByPlayer(unsigned long long a)
{
    return m_pInternal->GetClientObjectByObjectId(a) != 0;
}

int CServerExoApp::IsInjured(unsigned long long a)
{
    unsigned long long id = m_pInternal->ClientToServerObjectId(a);
    return m_pInternal->IsInjured(id);
}

void CServerExoApp::ClearAllQueuedCombatActions(unsigned long long a)
{
    CSWSCreature* pCreature = m_pInternal->GetCreatureByGameObjectID(a);
    if (pCreature != 0)
        pCreature->m_pCombatRound->RemoveAllActions();
}

CPathfindInformation* CServerExoApp::GetCreaturePathfindInformation(unsigned long long a)
{
    unsigned long long id = m_pInternal->ClientToServerObjectId(a);
    CSWSCreature* pCreature = m_pInternal->GetCreatureByGameObjectID(id);
    if (pCreature == 0)
        return 0;
    return pCreature->m_pPathfindInfo;
}

void CServerExoApp::UpDatePlayerPosition(unsigned long long a, const Vector& b, const Vector& c)
{
    unsigned long long id = m_pInternal->ClientToServerObjectId(a);
    m_pInternal->UpDatePlayerPosition(id, b, c);
}

CSWSRoom* CServerExoApp::GetRoom(const CResRef& a, const Vector& b)
{
    K2_UNUSED(a);
    return m_pInternal->GetModule()->GetArea()->GetRoom(b, 0, 0);
}

int CServerExoApp::GetRoomIndex(const CResRef& a, const Vector& b, int c)
{
    K2_UNUSED(a);
    if (m_pInternal->GetModule() == 0)
        return -1;
    CSWSArea* pArea = m_pInternal->GetModule()->GetArea();
    if (pArea == 0)
        return -1;
    return pArea->GetRoomIndex(b, c);
}

float CServerExoApp::ComputeHeight(const CResRef& a, const Vector& b, int c)
{
    K2_UNUSED(a);
    if (m_pInternal->GetModule() == 0)
        return 0.0f;
    CSWSArea* pArea = m_pInternal->GetModule()->GetArea();
    if (pArea == 0)
        return 0.0f;
    return pArea->ComputeHeight(b, c, 0, 0);
}

void CServerExoApp::ClearAllActions(unsigned long long a)
{
    CSWSCreature* pCreature = m_pInternal->GetCreatureByGameObjectID(a);
    if (pCreature != 0)
    {
        pCreature->ClearAllActions(1);
        pCreature->m_pCombatRound->RemoveAllActions();
        pCreature->m_pCombatRound->m_oidAttackTarget = OBJECT_INVALID;
        pCreature->SetAttemptedAttackTarget(OBJECT_INVALID);
        pCreature->m_oidAttackTarget = OBJECT_INVALID;
        pCreature->m_oidAttemptedSpellTarget = OBJECT_INVALID;
        pCreature->m_oidSpellTarget = OBJECT_INVALID;
    }
}

int CServerExoApp::GetCreatureMovedByPowerOrFeat(unsigned long long a, Vector& b)
{
    unsigned long long id = m_pInternal->ClientToServerObjectId(a);
    CGameObject* pObject = m_pInternal->GetGameObject(id);
    // AsSWSCreature() is called without testing pObject for null.
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    int result = 0;
    if (pCreature != 0)
    {
        if (pCreature->m_bMovedByPowerOrFeat != 0)
        {
            b = pCreature->m_position;
            pCreature->m_bMovedByPowerOrFeat = 0;
            result = 1;
        }
    }
    return result;
}

int CServerExoApp::TestDirectLine(unsigned long long a, const CResRef& b, const Vector& c, const Vector& d,
                                   const float& e, float f, int g, CWalkHitInfo* h)
{
    K2_UNUSED(b);
    CSWSArea* pArea; // pArea stays uninitialized when GetModule() is null
#ifdef K2_FIX_ENGINE_BUGS
    pArea = 0;
#endif
    if (m_pInternal->GetModule() != 0)
        pArea = m_pInternal->GetModule()->GetArea();
    unsigned long long id = m_pInternal->ClientToServerObjectId(a);
    CSWSCreature* pCreature = m_pInternal->GetCreatureByGameObjectID(id);
#if KOTOR1  // K1 has no minigame fallback: no area or no creature is a plain 0
    if (pArea != 0 && pCreature != 0)
    {
        pArea->m_pActivePathfindInfo = pCreature->m_pPathfindInfo;
        return pArea->TestDirectLine(c, d, e, f, g, h);
    }
    return 0;
#else
    if (pArea != 0 && pCreature != 0)
    {
        pArea->m_pActivePathfindInfo = pCreature->m_pPathfindInfo;
    }
    else
    {
        void* miniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
        if (miniGame == 0)
            return static_cast<int>(reinterpret_cast<uintptr_t>(miniGame));  // miniGame is null here
    }
    return pArea->TestDirectLine(c, d, e, f, g, h);
#endif
}

void CServerExoApp::SetCreatureFollowing(unsigned long long a, unsigned long long b, int c, const Vector& d)
{
    unsigned long long id_b = b;
    int actionInt = c;
    CSWSCreature* pCreature = m_pInternal->GetCreatureByGameObjectID(a);
    if (pCreature != 0)
    {
        pCreature->ClearAllActions(1);
        CGameObject* pObject = m_pInternal->GetGameObject(b);
        if (pObject != 0)
        {
            int zeroInt = 0;
            Vector pos = pCreature->m_position;
            pCreature->AddAction(AIACTION_FORCE_FOLLOW_OBJECT, ACTION_GROUP_NEW,
                ACTION_PARAM_OBJECT, &id_b, ACTION_PARAM_INT, &actionInt,
                ACTION_PARAM_FLOAT, const_cast<Vector*>(&d),
                ACTION_PARAM_FLOAT, const_cast<float*>(&d.y), ACTION_PARAM_INT, &zeroInt,
                ACTION_PARAM_FLOAT, &pos,
                ACTION_PARAM_FLOAT, reinterpret_cast<void*>(reinterpret_cast<unsigned long long>(&pos) | 4),
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }
    }
}

CServerExoApp::CServerExoApp()
{
    m_pInternal = new CServerExoAppInternal;
}

CServerExoApp::~CServerExoApp()
{
    delete m_pInternal;
}
