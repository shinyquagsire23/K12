#include "k2stdint.h"
#include "ClientExoAppInternal.h"
#include "clientoptions.h"
#include "globalvartables.h"
#include "exoresman.h"
#include "aurcamera.h"
#include "swplayercontrol.h"
#include "Achievements.h"
#include "gameobjectarray.h"
#include "swguimainmenu.h"
#include "exoinput.h"
#include "swgui.h"
#include "port/crtapi.h"
#include "include/aurbehavior.h"

CSWCJournal* CClientExoApp::GetQuestJournal()
{
    return m_pInternal->m_pQuestJournal;
}

CSWCWorldJournal* CClientExoApp::GetWorldJournal()
{
    return m_pInternal->m_pWorldJournal;
}

void* CClientExoApp::GetCharacterFileBuffer()
{
    return m_pInternal->m_pCharacterFileBuffer;
}

int CClientExoApp::GetCharacterFileSize()
{
    return m_pInternal->m_characterFileSize;
}

int CClientExoApp::GetCharacterFileSubDir()
{
    return m_pInternal->m_characterFileSubDir;
}

void* CClientExoApp::GetCaptureEventFile()
{
    return m_pInternal->m_pCaptureEventFile;
}

int CClientExoApp::GetCaptureEvents()
{
    return m_pInternal->m_captureEvents;
}

int CClientExoApp::GetCaptureEventPlayBack()
{
    return m_pInternal->m_captureEventPlayBack;
}

int CClientExoApp::GetClientLanguage()
{
    return m_pInternal->m_clientLanguage;
}

void CClientExoApp::SetClientLanguage(int lang)
{
    m_pInternal->m_clientLanguage = lang;
}

int CClientExoApp::IsClientImeUI()
{
    return m_pInternal->m_bClientImeUI;
}

CImeUI* CClientExoApp::GetImeUI()
{
    return m_pInternal->m_pImeUI;
}

CWorldTimer* CClientExoApp::GetWorldTimer()
{
    return m_pInternal->m_pWorldTimer;
}

CWorldTimer* CClientExoApp::GetPauseTimer()
{
    return m_pInternal->m_pPauseTimer;
}

CWorldTimer* CClientExoApp::GetAnimationTimer()
{
    return m_pInternal->m_pAnimationTimer;
}

CConnectionLib* CClientExoApp::GetConnectionLib()
{
    return m_pInternal->m_pConnectionLib;
}

#if KOTOR1
int CClientExoApp::DontUseThisSetPlayerCreature(unsigned long long oid)
{
    return m_pInternal->DontUseThisSetPlayerCreature(oid);
}
#endif

unsigned long long CClientExoApp::GetPlayerCreatureId()
{
    return m_pInternal->m_playerCreatureId;
}

CGameObjectArray* CClientExoApp::GetGameObjectArray()
{
    return m_pInternal->m_pGameObjectArray;
}

int CClientExoApp::GetInStartNewModule()
{
    return m_pInternal->m_inStartNewModule;
}

void CClientExoApp::SetInStartNewModule(int b)
{
    m_pInternal->m_inStartNewModule = b;
}

int CClientExoApp::GetNumCDKeys()
{
    return m_pInternal->m_sCDKeys.m_count;
}

CGuiInGame* CClientExoApp::GetInGameGui()
{
    return m_pInternal->m_pInGameGui;
}

CGuiPanel* CClientExoApp::GetPlayModulePanel()
{
    return m_pInternal->m_pPlayModulePanel;
}

CGuiPanel* CClientExoApp::GetLoadModulePanel()
{
    return m_pInternal->m_pLoadModulePanel;
}

CClientAIMaster* CClientExoApp::GetClientAIMaster()
{
    return m_pInternal->m_pClientAIMaster;
}

CExoLinkedList<CSWCPlayer>* CClientExoApp::GetPlayerList()
{
    return &m_pInternal->m_playerList;
}

CNetLayer* CClientExoApp::GetNetLayer()
{
    return m_pInternal->m_pNetLayer;
}

void CClientExoApp::DestroyMainMenuPanel()
{
    m_pInternal->DestroyMainMenuPanel();
}

CAurCamera* CClientExoApp::GetModuleCamera()
{
    return m_pInternal->GetModuleCamera();
}

int CClientExoApp::GetActivePauseState()
{
    return m_pInternal->GetActivePauseState();
}

CSWCModule* CClientExoApp::GetModule()
{
    return m_pInternal->GetModule();
}

int CClientExoApp::GetPauseState(unsigned char mask)
{
    return m_pInternal->GetPauseState(mask);
}

CSWGuiStore* CClientExoApp::GetStorePanel()
{
    return m_pInternal->GetStorePanel();
}

CSWCMessage* CClientExoApp::GetSWCMessage()
{
    return m_pInternal->m_pSWCMessage;
}

CClientOptions* CClientExoApp::GetClientOptions()
{
    return m_pInternal->m_pClientOptions;
}

#if KOTOR1
int CClientExoApp::IsValidResolution(unsigned int nWidth, unsigned int nHeight)
{
    return m_pInternal->IsValidResolution(nWidth, nHeight);
}

int CClientExoApp::GetAllowHighMonitorFrequency()
{
    return m_pInternal->m_bAllowHighMonitorFrequency;
}
#endif

CServerInfo* CClientExoApp::GetServerInfo()
{
    return m_pInternal->m_pServerInfo;
}

unsigned short CClientExoApp::GetServerMode()
{
    return m_pInternal->m_serverModeChange;
}

int CClientExoApp::IsClientInitialized()
{
    return m_pInternal->m_bClientInitialized;
}

CSWCPlayer* CClientExoApp::GetPlayerByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetPlayerByGameObjectID(id);
}

CSWCItem* CClientExoApp::GetItemByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetItemByGameObjectID(id);
}

CSWCCreature* CClientExoApp::GetCreatureByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetCreatureByGameObjectID(id);
}

CSWCModule* CClientExoApp::GetModuleByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetModuleByGameObjectID(id);
}

CSWCArea* CClientExoApp::GetAreaByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetAreaByGameObjectID(id);
}

CSWCTrigger* CClientExoApp::GetTriggerByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetTriggerByGameObjectID(id);
}

CSWCProjectile* CClientExoApp::GetProjectileByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetProjectileByGameObjectID(id);
}

CSWCPlaceable* CClientExoApp::GetPlaceableByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetPlaceableByGameObjectID(id);
}

CSWCDoor* CClientExoApp::GetDoorByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetDoorByGameObjectID(id);
}

CSWCAreaOfEffectObject* CClientExoApp::GetAreaOfEffectByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetAreaOfEffectByGameObjectID(id);
}

CSWCSoundObject* CClientExoApp::GetSoundObjectByGameObjectID(unsigned long long id)
{
    return m_pInternal->GetSoundObjectByGameObjectID(id);
}

void CClientExoApp::EndLoadBar()
{
    m_pInternal->EndLoadBar();
}

void CClientExoApp::CreateQuestJournal()
{
    m_pInternal->CreateQuestJournal();
}

void CClientExoApp::DestroyQuestJournal()
{
    m_pInternal->DestroyQuestJournal();
}

void CClientExoApp::CreateWorldJournal()
{
    m_pInternal->CreateWorldJournal();
}

void CClientExoApp::DestroyWorldJournal()
{
    m_pInternal->DestroyWorldJournal();
}

void CClientExoApp::DisplayMainMenu(int a)
{
    m_pInternal->DisplayMainMenu(a);
}

void CClientExoApp::DisplayStartScreen()
{
    m_pInternal->DisplayStartScreen();
}

void CClientExoApp::UnloadModule()
{
    m_pInternal->UnloadModule();
}

CSWCModule* CClientExoApp::CreateModule()
{
    return m_pInternal->CreateModule();
}

void CClientExoApp::ExitProgram()
{
    m_pInternal->ExitProgram();
}

CSWCCreature* CClientExoApp::GetPlayerCreature()
{
    return m_pInternal->GetPlayerCreature();
}

void CClientExoApp::ReprocessExclusionList()
{
    m_pInternal->ReprocessExclusionList();
}

CWorldTimer* CClientExoApp::GetActiveTimer(unsigned long long id)
{
    return m_pInternal->GetActiveTimer(id);
}

void CClientExoApp::ClearPauseExclusionList()
{
    m_pInternal->ClearPauseExclusionList();
}

void CClientExoApp::AddToPauseExclusionList(unsigned long long id)
{
    m_pInternal->AddToPauseExclusionList(id);
}

void CClientExoApp::AttachPlayerGameObjectToArea()
{
    m_pInternal->AttachPlayerGameObjectToArea();
}

void CClientExoApp::InitializeInGameGUI()
{
    m_pInternal->InitializeInGameGUI();
}

void CClientExoApp::ServerModeChange(unsigned char n)
{
    m_pInternal->ServerModeChange(n);
}

void CClientExoApp::AddObjectToHitcheckIgnoreList(unsigned long long id)
{
    m_pInternal->AddObjectToHitcheckIgnoreList(id);
}

void CClientExoApp::RemoveObjectFromHitcheckIgnoreList(unsigned long long id)
{
    m_pInternal->RemoveObjectFromHitcheckIgnoreList(id);
}

void CClientExoApp::EmptyHitcheckIgnoreList()
{
    m_pInternal->EmptyHitcheckIgnoreList();
}

void CClientExoApp::SetPlayerCreature(unsigned long long id)
{
    m_pInternal->SetPlayerCreature(id);
}

void CClientExoApp::ContinueMessageProcessing()
{
    m_pInternal->ContinueMessageProcessing();
}

void CClientExoApp::Initialize()
{
    m_pInternal->Initialize();
}

int CClientExoApp::MainLoop()
{
    return m_pInternal->MainLoop();
}

void CClientExoApp::OnCDChange()
{
    m_pInternal->OnCDChange();
}

void CClientExoApp::SendCharacterResponse(int a)
{
    m_pInternal->SendCharacterResponse(a);
}

CSWCPlayer* CClientExoApp::GetPlayerByPlayerID(unsigned int playerId)
{
    return m_pInternal->GetPlayerByPlayerID(playerId);
}

CSWCPlayer* CClientExoApp::GetPlayerByPlayerName(const CExoString& name)
{
    return m_pInternal->GetPlayerByPlayerName(name);
}

void CClientExoApp::SetPauseState(unsigned char state, int a, CExoLinkedList<unsigned long long>* pList)
{
    m_pInternal->SetPauseState(state, a, pList);
}

void CClientExoApp::SendSaveGameRequest(unsigned int a, const CExoString& b, const CExoString& c)
{
    m_pInternal->SendSaveGameRequest(a, b, c);
}

void CClientExoApp::SendLoadGameRequest(unsigned int a, const CExoString& b, const CExoString& c)
{
    m_pInternal->SendLoadGameRequest(a, b, c);
}

// sic: shipped returning NULL unconditionally.
CModuleDescription* CClientExoApp::GetModuleDescription()
{
    return 0;
}

void CClientExoApp::JournalUpdated(int a, int b, const CExoString& s)
{
    m_pInternal->JournalUpdated(a, b, s);
}

int CClientExoApp::GetObjectName(unsigned long long id, CExoString& out)
{
    return m_pInternal->GetObjectName(id, out);
}

void CClientExoApp::SetCharacterFile(unsigned int a, unsigned char* pData, int size)
{
    m_pInternal->SetCharacterFile(a, pData, size);
}

void CClientExoApp::HandleMessage(unsigned int a, unsigned char* pData, unsigned int b, int c)
{
    m_pInternal->HandleMessage(a, pData, b, c);
}

// The internal overload takes an extra trailing int (0 here).
void CClientExoApp::WalkPlayerToPoint(float x, float y, float z, int a, unsigned long long id)
{
    m_pInternal->WalkPlayerToPoint(x, y, z, a, id, 0);
}

CExoString CClientExoApp::GetClientName()
{
    return m_pInternal->m_clientName;
}

void CClientExoApp::SetClientName(const CExoString& name)
{
    m_pInternal->m_clientName = name;
}

CExoString CClientExoApp::GetCDKey(int index)
{
    return m_pInternal->m_sCDKeys.m_pArray[index];
}

void CClientExoApp::OnExit()
{
    m_pInternal->OnExit();
}

void CClientExoApp::OnGainFocus()
{
    m_pInternal->OnGainFocus();
}

void CClientExoApp::OnLostFocus()
{
    m_pInternal->OnLostFocus();
}

void CClientExoApp::OnPlayModulePanelExit(int a)
{
    m_pInternal->OnPlayModulePanelExit(a);
}

void CClientExoApp::OnVideoChange()
{
    m_pInternal->OnVideoChange();
}

void CClientExoApp::Uninitialize()
{
    m_pInternal->Uninitialize();
}

void CClientExoApp::SetCaptureEvents(const CExoString& s)
{
    m_pInternal->SetCaptureEvents(s);
}

void CClientExoApp::SetCaptureEventPlayBack(const CExoString& s)
{
    m_pInternal->SetCaptureEventPlayBack(s);
}

void CClientExoApp::SetCaptureEventPlayBackRate(int a)
{
    m_pInternal->SetCaptureEventPlayBackRate(a);
}

void CClientExoApp::SetDebugMode(int a)
{
    m_pInternal->SetDebugMode(a);
}

int CClientExoApp::GetDebugMode()
{
    return m_pInternal->GetDebugMode();
}

void CClientExoApp::SetViewTriggers(int a)
{
    m_pInternal->SetViewTriggers(a);
}

int CClientExoApp::GetViewTriggers()
{
    return m_pInternal->GetViewTriggers();
}

int CClientExoApp::StartServices(int a, const CExoString& b, const CExoString& c, const CExoString& d, uint64_t e)
{
    return m_pInternal->StartServices(a, b, c, d, e);
}

void CClientExoApp::SetEventDescriptions()
{
    m_pInternal->SetEventDescriptions();
}

void CClientExoApp::StopServices()
{
    m_pInternal->StopServices();
}

void CClientExoApp::CleanUpModuleServices()
{
    m_pInternal->CleanUpModuleServices();
}

void CClientExoApp::ShutDownLegalScreen()
{
    m_pInternal->ShutDownLegalScreen();
}

void CClientExoApp::ShutDownToMainMenu()
{
    m_pInternal->ShutDownToMainMenu();
}

void CClientExoApp::ShowServerMem()
{
    m_pInternal->ShowServerMem();
}

int CClientExoApp::IsHostingServer()
{
    return m_pInternal->IsHostingServer();
}

void CClientExoApp::NextPartyFormation()
{
    m_pInternal->NextPartyFormation();
}

void CClientExoApp::ReadAndSetVideoMode(int a, int b)
{
    m_pInternal->ReadAndSetVideoMode(a, b);
}

int CClientExoApp::GetNearestVideoMode(unsigned int a, unsigned int b, unsigned char c, unsigned int d)
{
    return m_pInternal->GetNearestVideoMode(a, b, c, d);
}

int CClientExoApp::GetVideoMode(unsigned int a, unsigned int b, unsigned char c, unsigned int d)
{
    return m_pInternal->GetVideoMode(a, b, c, d);
}

void CClientExoApp::SetContainerOpened(unsigned long long a)
{
    m_pInternal->m_containerOpened = a;
}

unsigned long long CClientExoApp::GetContainerOpened()
{
    return m_pInternal->m_containerOpened;
}

void CClientExoApp::SetApplicationIdsMatch(int a)
{
    m_pInternal->m_bApplicationIdsMatch = a;
}

CSWParty* CClientExoApp::GetSWParty()
{
    return m_pInternal->m_pParty;
}

int CClientExoApp::GetHitCheckMaterialMask()
{
    return m_pInternal->m_hitCheckMaterialMask;
}

int CClientExoApp::GetCurrentVideoMode()
{
    return m_pInternal->m_currentVideoMode;
}

int CClientExoApp::SetVideoMode(unsigned int a)
{
    return m_pInternal->SetVideoMode(a, 0);
}

void CClientExoApp::BeginIntro()
{
    m_pInternal->BeginIntro();
}

void CClientExoApp::PlayerListChange(unsigned int a, int b, int c)
{
    K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c);
    // sic: shipped as an empty stub.
}

void CClientExoApp::GetVideoModeSpec(unsigned int a, unsigned int* b, unsigned int* c, unsigned char* d, unsigned int* e)
{
    m_pInternal->GetVideoModeSpec(a, b, c, d, e);
}

void CClientExoApp::SetTexturePack(unsigned char a)
{
    m_pInternal->SetTexturePack(a, 1);
}

unsigned char CClientExoApp::GetCurrentTexturePack()
{
    return m_pInternal->m_currentTexturePack;
}

CClientMsgSysAdmin* CClientExoApp::GetClientSysAdminMessenger()
{
    return m_pInternal->m_pSysAdminMessenger;
}

CSoundSetList* CClientExoApp::GetSoundSetList()
{
    return m_pInternal->m_pSoundSetList;
}

int CClientExoApp::GetLoadBarState()
{
    return m_pInternal->m_loadBarState;
}

void CClientExoApp::SetEndMovie(const CExoString& s)
{
    m_pInternal->m_endMovie = s;
}

CExoString CClientExoApp::GetEndMovie()
{
    return m_pInternal->m_endMovie;
}

int CClientExoApp::GetDisableMovies()
{
    return m_pInternal->m_bDisableMovies;
}

int CClientExoApp::GetDisableIntroMovies()
{
    return m_pInternal->m_bDisableIntroMovies;
}

#if KOTOR1
void CClientExoApp::InformMoviePlayerOfWindowsPosChanging(int a)
#else
void CClientExoApp::InformMoviePlayerOfWindowsPosChanging(int64_t a)
#endif
{
    m_pInternal->InformMoviePlayerOfWindowsPosChanging(a);
}

void CClientExoApp::InformMoviePlayerOfWindowsPosChanged()
{
    m_pInternal->InformMoviePlayerOfWindowsPosChanged();
}

void CClientExoApp::RestartMoviesIfNecessary()
{
    m_pInternal->RestartMoviesIfNecessary();
}

void CClientExoApp::ClearMovies()
{
    m_pInternal->ClearMovies();
}

void CClientExoApp::PauseMovie()
{
    m_pInternal->PauseMovie();
}

void CClientExoApp::UnpauseMovie()
{
    m_pInternal->UnpauseMovie();
}

void CClientExoApp::SetSendAreaLoadedMessageAfterMovie()
{
    m_pInternal->m_bSendAreaLoadedAfterMovie = 1;
}

void CClientExoApp::ReloadDynamicEventDescriptions()
{
    m_pInternal->ReloadDynamicEventDescriptions();
}

CGuiAreaLoadscreen* CClientExoApp::StartLoadBar(int a, char* b, int c)
{
    return m_pInternal->StartLoadBar(a, b, c);
}

void CClientExoApp::AddObjectModelDirectlyToIgnoreList(unsigned long long a, List<CAurObject*>* b)
{
    m_pInternal->AddObjectModelDirectlyToIgnoreList(a, b);
}

unsigned long long CClientExoApp::GetLastDialogSpeaker()
{
    return m_pInternal->m_lastDialogSpeaker;
}

void CClientExoApp::SetLastDialogSpeaker(unsigned long long a)
{
    m_pInternal->m_lastDialogSpeaker = a;
}

void CClientExoApp::StartMenuMusic(int a, int b, int c)
{
    m_pInternal->StartMenuMusic(a, b, c);
}

void CClientExoApp::StopMenuMusic()
{
    m_pInternal->StopMenuMusic();
}

void CClientExoApp::StopLoadMusic()
{
    m_pInternal->StopLoadMusic();
}

CExoStreamingSoundSource* CClientExoApp::AquireLoadMusic(const CResRef& r)
{
    return m_pInternal->AquireLoadMusic(r);
}

void CClientExoApp::SetStartMenuMusicOnMovieEnd(int a)
{
    m_pInternal->m_bStartMenuMusicOnMovieEnd = a;
}

void CClientExoApp::RenderTilePathNodes(int a)
{
    m_pInternal->RenderTilePathNodes(a);
}

int CClientExoApp::GetRenderTilePathNodes()
{
    return m_pInternal->m_bRenderTilePathNodes;
}

int CClientExoApp::GetApplicationId()
{
    return g_pAppManager->m_appId;
}

void CClientExoApp::ResetCharacterFile()
{
    m_pInternal->ResetCharacterFile();
}

void* CClientExoApp::GetWalkToRingVFX()
{
    return m_pInternal->m_pWalkToRingVFX;
}

int CClientExoApp::GetShowWalkToRingVFX()
{
    return m_pInternal->m_bShowWalkToRingVFX;
}

int CClientExoApp::GetQuitClicked()
{
    return m_pInternal->m_bQuitClicked;
}

void CClientExoApp::SetQuitClicked(int b)
{
    m_pInternal->m_bQuitClicked = b;
}

int CClientExoApp::GetWMCLOSEQuery()
{
    return m_pInternal->m_bWMCLOSEQuery;
}

void CClientExoApp::SetWMCLOSEQuery(int b)
{
    m_pInternal->m_bWMCLOSEQuery = b;
}

int CClientExoApp::GetReachedMainMenu()
{
    return m_pInternal->m_bReachedMainMenu;
}

void CClientExoApp::SetOptionsPanelOpen(int b)
{
    m_pInternal->m_bOptionsPanelOpen = b;
}

void CClientExoApp::SetPointerToCharGenPanel(CCharacterGenerationPanel* p)
{
    m_pInternal->m_pCharGenPanel = p;
}

CSWGuiManager* CClientExoApp::GetSWGuiManager()
{
    return m_pInternal->m_pSWGuiManager;
}

int CClientExoApp::GetInputClass()
{
    return m_pInternal->m_inputClass;
}

int CClientExoApp::GetPlayerCharacterSet()
{
    return m_pInternal->m_playerCharacterSet;
}

int CClientExoApp::GetAreaNotReadyToDisplay()
{
    return m_pInternal->m_bAreaNotReadyToDisplay;
}

void CClientExoApp::SetAreaNotReadyToDisplay(int b)
{
    m_pInternal->m_bAreaNotReadyToDisplay = b;
}

CSWMiniGameObjectArray* CClientExoApp::GetMGOArray()
{
    return m_pInternal->m_pMGOArray;
}

CExoArrayList<CExoString>* CClientExoApp::GetMoveToModuleMovies()
{
    return &m_pInternal->m_moveToModuleMovies;
}

void CClientExoApp::RemoveMoveToModuleMovies()
{
    m_pInternal->m_moveToModuleMovies.m_count = 0;
}

int CClientExoApp::GetGameOver()
{
    return m_pInternal->m_nGameOverReason;
}

void CClientExoApp::SetDialogSent(int b)
{
    m_pInternal->m_bDialogSent = b;
}

int CClientExoApp::GetDialogSent()
{
    return m_pInternal->m_bDialogSent;
}

void CClientExoApp::SetDialogModelsSent(int b)
{
    m_pInternal->m_bDialogModelsSent = b;
}

int CClientExoApp::GetDialogModelsSent()
{
    return m_pInternal->m_bDialogModelsSent;
}

void CClientExoApp::SetLoadFromIFO(int b)
{
    m_pInternal->m_bLoadFromIFO = b;
}

int CClientExoApp::GetLoadFromIFO()
{
    return m_pInternal->m_bLoadFromIFO;
}

unsigned long long CClientExoApp::GetLastTarget()
{
    return m_pInternal->m_lastTarget;
}

void CClientExoApp::SetGuiGameSlow(int b)
{
    m_pInternal->m_bGuiGameSlow = b;
}

int CClientExoApp::GetGuiGameSlow()
{
    return m_pInternal->m_bGuiGameSlow;
}

void CClientExoApp::SetFutureLeader(unsigned long long id)
{
    m_pInternal->m_futureLeader = id;
}

unsigned long long CClientExoApp::GetFutureLeader()
{
    return m_pInternal->m_futureLeader;
}

int CClientExoApp::GetEndGamePending()
{
    return m_pInternal->m_bEndGamePending;
}

int CClientExoApp::GetDisplayEndGameGui()
{
    return m_pInternal->m_bDisplayEndGameGui;
}

int CClientExoApp::GetCombatMode()
{
    return m_pInternal->m_combatMode;
}

int CClientExoApp::GetVideoEffectType()
{
    return m_pInternal->m_videoEffectType;
}

void CClientExoApp::SetAttackMashTutorialTimer(unsigned int t)
{
    m_pInternal->m_attackMashTutorialTimer = t;
}

unsigned int CClientExoApp::GetAttackMashTutorialTimer()
{
    return m_pInternal->m_attackMashTutorialTimer;
}

int CClientExoApp::GetLoadingSaveGame()
{
    return m_pInternal->m_bLoadingSaveGame;
}

void CClientExoApp::SetLoadingSaveGame(int b)
{
    m_pInternal->m_bLoadingSaveGame = b;
}

void CClientExoApp::ClearEnemySighted()
{
    m_pInternal->m_bEnemySighted = 0;
}

unsigned int CClientExoApp::GetLoadScreenHintUpdateTimer()
{
    return m_pInternal->m_loadScreenHintUpdateTimer;
}

void CClientExoApp::SetLoadScreenHintUpdateTimer(unsigned int t)
{
    m_pInternal->m_loadScreenHintUpdateTimer = t;
}

unsigned char CClientExoApp::GetGamePlayHint()
{
    return m_pInternal->m_gamePlayHint;
}

void CClientExoApp::SetGamePlayHint(unsigned char h)
{
    m_pInternal->m_gamePlayHint = h;
}

void CClientExoApp::ResetModuleResourcesLoaded()
{
    m_pInternal->m_moduleResourceFlags = 0;
}

int CClientExoApp::GetPazaakIsRunning()
{
    return m_pInternal->m_bPazaakIsRunning;
}

int CClientExoApp::GetCharacterChangeInProgress()
{
    return m_pInternal->m_bCharacterChangeInProgress;
}

void CClientExoApp::SetCharacterChangeInProgress(int b)
{
    m_pInternal->m_bCharacterChangeInProgress = b;
}

int CClientExoApp::GetCreditSequenceInProgress()
{
    return m_pInternal->m_creditSequence;
}

void CClientExoApp::SetCreditSequenceInProgress(int b)
{
    m_pInternal->m_creditSequence = b;
}

void CClientExoApp::SetLastClickedOnTarget(unsigned long long id)
{
    m_pInternal->m_lastClickedOnTarget = id;
}

unsigned long long CClientExoApp::GetLastClickedOnTarget()
{
    return m_pInternal->m_lastClickedOnTarget;
}

unsigned long long CClientExoApp::GetObjectMouseOver()
{
    return m_pInternal->m_objectMouseOver;
}

int CClientExoApp::GetSoundPausedByCombat()
{
    return m_pInternal->m_bSoundPausedByCombat;
}

void CClientExoApp::SetSoundPausedByCombat(int b)
{
    m_pInternal->m_bSoundPausedByCombat = b;
}

int CClientExoApp::GetAutoPaused()
{
    return m_pInternal->m_bAutoPaused;
}

void CClientExoApp::SetLoadStep(int index, unsigned char v)
{
    m_pInternal->m_loadStep[index] = v;
}

unsigned char CClientExoApp::GetLoadStep(int index)
{
    return m_pInternal->m_loadStep[index];
}

#if KOTOR1
unsigned char CClientExoApp::GetStoryHint()
{
    return m_pInternal->m_storyHint[0];
}

void CClientExoApp::SetStoryHint(unsigned char value)
{
    m_pInternal->m_storyHint[0] = value;
}

void CClientExoApp::HandleExitGameButton()
{
    m_pInternal->HandleExitGameButton(0);
}

void MacHack::HandleExitGameButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (g_pAppManager && g_pAppManager->m_pClientExoApp)
        g_pAppManager->m_pClientExoApp->HandleExitGameButton();
}

#ifdef K2_UI_PC
extern volatile bool g_done;

// pc: closing the window (SDL_QUIT, MacMain.cpp) asks first, as KOTOR II's WM_CLOSE does and the
// Steam build of KOTOR 1 does. Before the in-game GUI exists there is nothing to ask with.
void K1PcQueryExit()
{
    if (g_pAppManager && g_pAppManager->m_pClientExoApp
        && g_pAppManager->m_pClientExoApp->GetInGameGui())
        g_pAppManager->m_pClientExoApp->QueryExitProgram();
    else
        g_done = true;
}
#endif
#else
unsigned char CClientExoApp::GetStoryHint(unsigned char index)
{
    return m_pInternal->m_storyHint[index];
}

void CClientExoApp::SetStoryHint(unsigned char value, unsigned char index)
{
    m_pInternal->m_storyHint[index] = value;
}
#endif

int CClientExoApp::GetTogglePauseAtEndOfFrame()
{
    return m_pInternal->m_bTogglePauseAtEndOfFrame;
}

int CClientExoApp::ReattachCreatureToScene(unsigned long long id)
{
    return m_pInternal->m_sceneCreatureId != id;
}

void CClientExoApp::SetEndGamePending(int a, float b, int c)
{
    m_pInternal->m_bEndGamePending = a;
    m_pInternal->m_fEndGameDelay = b;
    m_pInternal->m_bDisplayEndGameGui = c;
}

const CExoString& CClientExoApp::GetLiveContentName(int index)
{
    return m_pInternal->m_liveContent[index].name;
}

int CClientExoApp::GetLiveContentExists(int index)
{
    return m_pInternal->m_liveContent[index].exists;
}

int CClientExoApp::GetLiveContentCorrupt(int index)
{
    return m_pInternal->m_liveContent[index].corrupt;
}

void CClientExoApp::SetModuleResourceLoaded(unsigned char mask)
{
    m_pInternal->m_moduleResourceFlags |= mask;
}

int CClientExoApp::GetModuleResourceLoaded(unsigned char mask)
{
    return (m_pInternal->m_moduleResourceFlags & mask) != 0;
}

int CClientExoApp::GetTargetChanging()
{
    return m_pInternal->m_fTargetChanging > 0.0f;
}

void CClientExoApp::GetPazaakResults(int* a, int* b)
{
    *a = m_pInternal->m_pazaakResultA;
    *b = m_pInternal->m_pazaakResultB;
}

int CClientExoApp::GetControlledCharacterDead()
{
    return m_pInternal->GetControlledCharacterDead();
}

void CClientExoApp::PlayerFlourishWeapons()
{
    m_pInternal->PlayerFlourishWeapons();
}

void CClientExoApp::HandleInputEvent(int a, int b)
{
    m_pInternal->HandleInputEvent(a, b);
}

int CClientExoApp::ShowTutorialWindow(unsigned char a, unsigned long long b, unsigned long long c, unsigned int d)
{
    return m_pInternal->ShowTutorialWindow(a, b, c, d);
}

uint64_t CClientExoApp::GetNextLoadScreenHintSTRREF(const CExoString& a, unsigned char b)
{
    return m_pInternal->GetNextLoadScreenHintSTRREF(a, b);
}

void CClientExoApp::QueryExitProgram()
{
    m_pInternal->QueryExitProgram();
}

int CClientExoApp::StopRumblePattern(int a)
{
    return m_pInternal->StopRumblePattern(a);
}

void CClientExoApp::EnableVideoEffect(int a)
{
    m_pInternal->EnableVideoEffect(a);
}

int CClientExoApp::IsMoviePlaying()
{
    return m_pInternal->IsMoviePlaying();
}

void CClientExoApp::SetPlayerCharacterName(const CExoString& a)
{
    m_pInternal->SetPlayerCharacterName(a);
}

void CClientExoApp::AddMoveToModuleMovie(const CExoString& a)
{
    m_pInternal->AddMoveToModuleMovie(a);
}

CExoString* CClientExoApp::GetPlayerCharacterName()
{
    return m_pInternal->GetPlayerCharacterName();
}

int CClientExoApp::GetClientMeleeAnimation(unsigned long long a, unsigned long long b, unsigned short c, int d, int e, int f)
{
    return m_pInternal->GetClientMeleeAnimation(a, b, c, d, e, f);
}

void CClientExoApp::StartPazaakGame(int a, const CExoString& b, int c, int d, unsigned long long e)
{
    m_pInternal->StartPazaakGame(a, b, c, d, e);
}

void CClientExoApp::PlayLoadModuleMovies()
{
    m_pInternal->PlayLoadModuleMovies();
}

int CClientExoApp::ChangeCharacterToNextLivingPartyMember(int a, int b, unsigned char c)
{
    return m_pInternal->ChangeCharacterToNextLivingPartyMember(a, b, c);
}

int CClientExoApp::GetLoadBarProgress()
{
    return m_pInternal->GetLoadBarProgress();
}

void CClientExoApp::TogglePartyFollow()
{
    m_pInternal->TogglePartyFollow();
}

void CClientExoApp::FadeAndStopStreamingSoundSource(CExoStreamingSoundSource* a, unsigned int b)
{
    m_pInternal->FadeAndStopStreamingSoundSource(a, b);
}

void CClientExoApp::BusyWaitMenuMusic()
{
    m_pInternal->BusyWaitMenuMusic();
}

void CClientExoApp::SetCanSendDialog(int a)
{
    m_pInternal->SetCanSendDialog(a);
}

int CClientExoApp::DismissInGameGUI()
{
    return m_pInternal->DismissInGameGUI();
}

int CClientExoApp::GetLoadScreenVisible()
{
    return m_pInternal->GetLoadScreenVisible();
}

void CClientExoApp::CancelMovie()
{
    m_pInternal->CancelMovie();
}

void CClientExoApp::HideLoadScreen(int a, int b, int c)
{
    m_pInternal->HideLoadScreen(a, b, c);
}

void CClientExoApp::BarkString(unsigned long long a, const CExoString& b, const CResRef& c, int d)
{
    m_pInternal->BarkString(a, b, c, d);
}

void CClientExoApp::AddFloatyText(unsigned long long a, unsigned char b, int c)
{
    m_pInternal->AddFloatyText(a, b, c);
}

void CClientExoApp::DisableVideoEffect()
{
    m_pInternal->DisableVideoEffect();
}

void CClientExoApp::UpdateRoom(const Vector& a)
{
    m_pInternal->UpdateRoom(a);
}

void CClientExoApp::StopCreditSequence()
{
    m_pInternal->StopCreditSequence();
}

void CClientExoApp::SetInputClass(int a, int b)
{
    m_pInternal->SetInputClass(a, b);
}

CExoString CClientExoApp::GetLoadMusicByModuleName(const CExoString& a)
{
    return m_pInternal->GetLoadMusicByModuleName(a);
}

void CClientExoApp::ChangeCharacter(int a, int b)
{
    m_pInternal->ChangeCharacter(a, b);
}

void CClientExoApp::SetCombatMode(int a)
{
    m_pInternal->SetCombatMode(a);
}

void CClientExoApp::SetMouseMode(unsigned char a)
{
    m_pInternal->SetMouseMode(a);
}

void CClientExoApp::TurnOffKeyboardModal()
{
    m_pInternal->TurnOffKeyboardModal();
}

int CClientExoApp::IsPlayerCreature(CSWCCreature* a)
{
    return m_pInternal->IsPlayerCreature(a);
}

int CClientExoApp::GetCanSendDialog(unsigned long long a, unsigned long long b, CSWSDialog* c)
{
    return m_pInternal->GetCanSendDialog(a, b, c);
}

void CClientExoApp::SetLoadScreenByModuleName(const CExoString& a)
{
    m_pInternal->SetLoadScreenByModuleName(a);
}

void CClientExoApp::HurryDeathSequence()
{
    m_pInternal->HurryDeathSequence();
}

#if KOTOR1
void CClientExoApp::LoadOptions()
{
    m_pInternal->LoadOptions();
}
#else
void CClientExoApp::LoadOptions(int a)
{
    m_pInternal->LoadOptions(a);
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
void CClientExoApp::DoScreenShot()
{
    m_pInternal->DoScreenShot();
}
#endif // !KOTOR1

void CClientExoApp::SaveOptions()
{
    m_pInternal->SaveOptions();
}

void CClientExoApp::SetPausedByCombat(int a, unsigned char b, int c)
{
    m_pInternal->SetPausedByCombat(a, b, c);
}

void CClientExoApp::StartCreditSequence(const Vector& a, const CExoString& b)
{
    m_pInternal->StartCreditSequence(a, b);
}

uint64_t CClientExoApp::GetLoadScreenHint()
{
    return m_pInternal->GetLoadScreenHint();
}

CSWCPlayer* CClientExoApp::GetPlayerByCreatureName(const CExoString& a)
{
    return m_pInternal->GetPlayerByCreatureName(a);
}

void CClientExoApp::ResetDriveAcceleration()
{
    m_pInternal->ResetDriveAcceleration();
}

void CClientExoApp::EndPazaakGame(int a, int b, int c, unsigned long long d)
{
    m_pInternal->EndPazaakGame(a, b, c, d);
}

void CClientExoApp::SetGameOver(int a)
{
    m_pInternal->SetGameOver(a);
}

void CClientExoApp::PlayMovieQueue(int a)
{
    m_pInternal->PlayMovieQueue(a);
}

void CClientExoApp::StopSoundPlayingInDialog()
{
    m_pInternal->StopSoundPlayingInDialog();
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CClientExoApp::ResumeStreamMusicForMovie()
{
    m_pInternal->ResumeStreamMusicForMovie();
}
#endif // !KOTOR1

void CClientExoApp::HideMouse(int a)
{
#if KOTOR1
    if (a == CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE)
        return;
#endif
    m_pInternal->HideMouse(a);
}

void CClientExoApp::IndicateObject(CSWCObject* a, int b)
{
    m_pInternal->IndicateObject(a, b);
}

void CClientExoApp::ClearCachedCreatures(CSWCCreature* a)
{
    m_pInternal->ClearCachedCreatures(a);
}

void CClientExoApp::SetLoadScreenHint(uint64_t a)
{
    m_pInternal->SetLoadScreenHint(a);
}

void CClientExoApp::StartMovie(const CResRef& a)
{
    m_pInternal->StartMovie(a);
}

void CClientExoApp::DisableInput()
{
    m_pInternal->DisableInput();
}

int CClientExoApp::IsSoundPlayingInDialog()
{
    return m_pInternal->IsSoundPlayingInDialog();
}

void CClientExoApp::LookUpAndPerformRumbleWithCutOff(int a, unsigned char b, const Vector& c)
{
    m_pInternal->LookUpAndPerformRumbleWithCutOff(a, b, c);
}

void CClientExoApp::CrownFutureLeader()
{
    m_pInternal->CrownFutureLeader();
}

void CClientExoApp::EnableInput()
{
    m_pInternal->EnableInput();
}

void CClientExoApp::EnableMouseAtEndOfLoadModuleMovie()
{
    m_pInternal->EnableMouseAtEndOfLoadModuleMovie();
}

void CClientExoApp::ShowMouse(int a)
{
    m_pInternal->ShowMouse(a);
}

CExoString CClientExoApp::GetDisplayCharacterForKey(int a)
{
    return m_pInternal->GetDisplayCharacterForKey(a);
}

void CClientExoApp::SetLastPartyMemberTempKilled(unsigned long long a)
{
    m_pInternal->SetLastPartyMemberTempKilled(a);
}

void CClientExoApp::InitializeClientOptions()
{
    m_pInternal->InitializeClientOptions();
}

void CClientExoApp::StopMovie()
{
    m_pInternal->StopMovie();
}

int CClientExoApp::GetPausedByCombat()
{
    return m_pInternal->GetPausedByCombat();
}

void CClientExoApp::DelayNextEnemyAutoPause()
{
    m_pInternal->DelayNextEnemyAutoPause();
}

void CClientExoApp::ShowLoadScreen(int a, int b, int c, int d)
{
    m_pInternal->ShowLoadScreen(a, b, c, d);
}

#if KOTOR1
void CClientExoApp::ShowLoadScreen(int a, int b, int c)
{
    m_pInternal->ShowLoadScreen(a, b, c);
}

void CClientExoApp::HideLoadScreen(int a, int b)
{
    m_pInternal->HideLoadScreen(a, b);
}
#endif

unsigned long long CClientExoApp::ServerToClientObjectId(unsigned long long a)
{
    return m_pInternal->ServerToClientObjectId(a);
}

void CClientExoApp::SetLastTarget(unsigned long long a, int b)
{
    m_pInternal->SetLastTarget(a, b);
}

void CClientExoApp::SetLoadBarProgress(int a, int b)
{
    m_pInternal->SetLoadBarProgress(a, b);
}

void CClientExoApp::SetInputActive(int a, int b)
{
    m_pInternal->SetInputActive(a, b);
}

void CClientExoApp::SetAutoPaused(int a, unsigned char b)
{
    m_pInternal->SetAutoPaused(a, b);
}

void CClientExoApp::SetGameSpeed(float a)
{
    m_pInternal->SetGameSpeed(a);
}

void CClientExoApp::BusyWaitMuteSound()
{
    m_pInternal->BusyWaitMuteSound();
}

void CClientExoApp::RunDeathSequence()
{
    m_pInternal->RunDeathSequence();
}

CExoString CClientExoApp::GetGUIString(uint64_t a)
{
    return m_pInternal->GetGUIString(a);
}

#if !KOTOR1
bool CClientExoApp::CanFlourish()
{
    return m_pInternal->CanFlourish();
}
#endif

void CClientExoApp::PauseAllSoundsForMovieLostFocus()
{
    m_pInternal->PauseAllSoundsForMovieLostFocus();
}

int CClientExoApp::PlayRumblePattern(int a)
{
    return m_pInternal->PlayRumblePattern(a);
}

void CClientExoApp::SelectNearestObject(int a)
{
    m_pInternal->SelectNearestObject(a, 0);
}

int CClientExoApp::GetClientRangedAnimation(unsigned long long a, int b, int c, int d)
{
    K2_UNUSED(c); K2_UNUSED(d);
    return m_pInternal->GetClientRangedAnimation(a, b, 0, 0);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CClientExoApp::SetSuperModelSet(unsigned char b)
{
    if (m_pInternal)
        m_pInternal->m_superModelSet = b;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
unsigned char CClientExoApp::GetSuperModelSet()
{
    if (m_pInternal)
        return m_pInternal->m_superModelSet;
    return 1;
}
#endif // !KOTOR1

void CClientExoApp::ClearMainMenuPointer()
{
    if (m_pInternal)
        m_pInternal->m_pMainMenuPanel = 0;
}

void CClientExoApp::SetLoadMusic(const CExoString& music)
{
    m_pInternal->m_loadMusic = music;
}

void CClientExoApp::SetRunScript(const CExoString& script)
{
    m_pInternal->m_bRunScriptSet = 1;
    m_pInternal->m_runScript = script;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CClientExoApp::UnloadSuperModelSet()
{
    if (m_pInternal)
        m_pInternal->UnloadSuperModelSet();
}
#endif // !KOTOR1

// End-of-campaign hook: bumps the profile's completion count, publishes it as the script-visible
// GBL_GAME_COMPLETE, and flushes the options file so the count survives.
#if !KOTOR1
void OnGameComplete()
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->m_pInternal->m_pClientOptions;
    char nGamesCompleted = pOptions->m_nGamesCompleted + 1;
    pOptions->m_nGamesCompleted = nGamesCompleted;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
        CExoString("GBL_GAME_COMPLETE"), nGamesCompleted);
    g_pAppManager->m_pClientExoApp->m_pInternal->SaveOptions();
}
#endif

void CClientExoApp::AddFeedbackMessage(unsigned short nMsg, CSWCCMessageData* pData,
                                       const CExoString& sMsg)
{
    int bEmpty = sMsg == "";
    CClientExoAppInternal* pInternal = m_pInternal;
    if (bEmpty) {
        CExoString sEmpty("");
        pInternal->AddFeedbackMessage(nMsg, pData, sEmpty);
    } else {
        pInternal->AddFeedbackMessage(nMsg, pData, sMsg);
    }
}

void CClientExoApp::AddCDKey(const CExoString& sKey)
{
    m_pInternal->m_sCDKeys.Add(sKey.UpperCase());
}

// Free function OnGameComplete fires only for the post-game "malmov12"/"malmov14" end-roll.
void CClientExoApp::AddMovieToMovieQueue(const CExoString& sMovie, int nFlags)
{
#if !KOTOR1
    Achievements::EvaluateAchievementMovie(sMovie.CStr());
    if (_stricmp(sMovie.CStr(), "malmov12") == 0 || _stricmp(sMovie.CStr(), "malmov14") == 0)
        OnGameComplete();
#endif
    m_pInternal->AddMovieToMovieQueue(sMovie, nFlags);
}

#if KOTOR1
// K1 has no achievement hook and no skippable flag: a plain tail into the internal player.
void CClientExoApp::PlayMovie(const CExoString& sMovie, int& nResult, int a, int b)
{
    m_pInternal->PlayMovie(sMovie, nResult, a, b);
}
#else
// The achievement hook fires only when the fourth argument is 0, then the call forwards.
void CClientExoApp::PlayMovie(const CExoString& sMovie, int& nResult, int a, int b, int c)
{
    if (b == 0)
        Achievements::EvaluateAchievementMovie(sMovie.CStr());
    m_pInternal->PlayMovie(sMovie, nResult, a, b, c);
}
#endif

void CClientExoApp::ClearAllCDKeys()
{
    m_pInternal->m_sCDKeys.Clear();
}

CGameObject* CClientExoApp::GetGameObject(unsigned long long id)
{
    return m_pInternal->GetGameObject(id);
}

CResRef CClientExoApp::GetItemDropSound(unsigned char nSound)
{
    return m_pInternal->GetItemDropSound(nSound);
}

void CClientExoApp::CreatureAcquireItem(unsigned long long id, const CResRef& r)
{
    unsigned long long nServerId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(id);
    g_pAppManager->m_pServerExoApp->CreatureAcquireItem(nServerId, r);
}

CSWMiniGame* CClientExoApp::GetMiniGame()
{
    CSWCModule* pModule = m_pInternal->GetModule();
    if (pModule)
        return pModule->GetMiniGame();
    return 0;
}

int CClientExoApp::GetInFreeLook()
{
    if (!m_pInternal->GetModule())
        return 0;
    if (!m_pInternal->GetModule()->m_pCamera)
        return 0;
    return m_pInternal->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_FREE_LOOK) != 0;
}

int CClientExoApp::GetPlayerMovementInput()
{
    CClientExoAppInternal* pInternal = m_pInternal;
    CSWPlayerControl* pCtl = pInternal->m_pPlayerControl;
    if (pCtl) {
        if (pCtl->GetLeftRight() != 0.0f)
            return 1;
        pCtl = pInternal->m_pPlayerControl;
        return pCtl->GetUpDown() != 0.0f;
    }
    return 0;
}

#if !KOTOR1  // K1 has no main-menu idle timer
void CClientExoApp::EnableMainMenuIdleTimer()
{
    if (m_pInternal && m_pInternal->m_pMainMenuPanel)
        m_pInternal->m_pMainMenuPanel->m_bIdleTimerDisabled = 0;
}
#endif

void CClientExoApp::DebugValidatePath(unsigned long long id, int n)
{
    g_pAppManager->m_pServerExoApp->DebugValidatePath(id, n);
}

void CClientExoApp::DebugValidatePosition(unsigned long long a, unsigned long long b, Vector vPos)
{
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    unsigned long long nServerA = pServer->ClientToServerObjectId(a);
    unsigned long long nServerB = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(b);
    pServer->DebugValidatePosition(nServerA, nServerB, vPos);
}

void CClientExoApp::DebugValidatePath_Point(CSWSArea* pArea, Vector vPos)
{
    g_pAppManager->m_pServerExoApp->DebugValidatePath_Point(pArea, vPos);
}

void CClientExoApp::DebugValidatePath_Segment(CSWSArea* pArea, CPathfindInformation* pInfo,
                                              Vector vStart, Vector vEnd)
{
    g_pAppManager->m_pServerExoApp->DebugValidatePath_Segment(pArea, pInfo, vStart, vEnd);
}

extern float g_fFPS;

float CClientExoApp::GetFPS()
{
    return g_fFPS;
}

extern int g_bUseJoystick;

// True when a gamepad is attached and the player has asked for one: the gate the input code reads
// before showing any gamepad hint.
inline bool CClientExoAppInternal::GetIsJoystickConnected()
{
    return (g_pExoInput->GetNumberJoysticks() != 0) & (g_bUseJoystick != 0);
}

bool IsJoystickConnected()
{
    return g_pAppManager->m_pClientExoApp->m_pInternal->GetIsJoystickConnected();
}

CClientExoApp::CClientExoApp()
{
    m_pInternal = new CClientExoAppInternal;
}

CClientExoApp::~CClientExoApp()
{
    delete m_pInternal;
}
