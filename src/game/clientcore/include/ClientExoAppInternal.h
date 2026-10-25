#pragma once
#include "k2stdint.h"

#ifndef CSWCPLAYER_H
#define CSWCPLAYER_H

#include "exostring.h"
#include "nwportrait.h"

// Roster entry for one connected player, holding the player id, the display name and, once that
// player is driving a character, that character's object id, joined name and portrait.
class CSWCPlayer {
public:
    virtual void* AsSWCDungeonMaster() { return 0; }

    unsigned int         m_nPlayerId;
    int                  m_nReserved1;
    CExoString           m_sPlayerName;
    CSWPortrait          m_portrait;
    unsigned short       m_nPortraitId;
    char                 _pad32[0x38-0x32];
    unsigned long long   m_gameObjectId;
    unsigned long long   m_characterObjectId;
    CExoString           m_sCharacterName;

    CSWCPlayer(unsigned int nPlayerId)
        : m_nPlayerId(nPlayerId), m_nReserved1(0), m_nPortraitId(0xffff) {}

    void SetPrimaryPCPortraitId(unsigned short nPortraitId);
};

#endif // CSWCPLAYER_H

#include "nwcmodule.h"
#include "clientexoapp.h"
#include "tutorial.h"

// Internals of the client app object, the pimpl behind CClientExoApp, plus slim views of the client
// classes its methods reach into. Only this directory's own sources include it; everyone else uses
// ClientExoApp.h.

class CSWCModule;
class CAurCamera;
struct CClientOptions;
class CServerInfo;
class CSWCMessage;
class CSWMiniGameObjectArray;
class CSWCJournal;
class CSWCWorldJournal;
class CSWGuiStore;
class CClientAIMaster;
class CNetLayer;
class CGameObjectArray;
struct CWorldTimer;
class CGuiAreaLoadscreen;
class CGuiInGame;
class CGuiPanel;
class CImeUI;
class CConnectionLib;
class CModuleDescription;
#include "serverexoapp.h"
#include "appmanager.h"
class CResRef;
class CSWGuiLegalScreen;
class CSWGuiLoadScreen;
class CSWGuiControl;
class CAurObject;
class CClientMsgSysAdmin;
class CSoundSetList;
template<class T> class List;
#include "exomovieplayer.h"
#include "swparty.h"
template<class T> class CExoLinkedList;
class CExoFile;
class CSWCObject; class CSWSDialog; class CExoStreamingSoundSource;
struct Vector;
class CSWCPlayer; class CSWCItem; class CSWCCreature; class CSWCArea; class CSWCTrigger;
class CSWCProjectile; class CSWCPlaceable; class CSWCDoor; class CSWCAreaOfEffectObject; class CSWCSoundObject;
class CSWCStore;
class CSWGuiMainMenu;
class CGuiStringStruct;
class CClientExoAppInternal;
class CSWGuiManager;
class CCharacterGenerationPanel;

#include "CExoArrayListDtors.h"
#include "nwcobject.h"
struct LiveContentEntry {
    CExoString name;
    int        corrupt;
    int        exists;
};

#include "CExoLinkedList.h"


// One queued animation, holding the name, its float and int parameters, and an unused halfword.
struct CQueuedAnim {
    CExoString     m_sName;
    float          m_fParam;
    int            m_nParam;
    unsigned short m_nAnimation;
};

// One entry of the idle-rumble table, holding a pattern index, the playback time into it, and a
// latch that restarts the pattern (the frame it clears does not advance the time).
struct SRumblePattern {
    int   nPattern;
    float fPhase;
    int   nState;
};

// One captured input event. The tag byte selects which payload fields are meaningful:
// 'K' key-down, 'C' char, 'L' net playback link, 'M' message, 'I' input, 'Z' move-to,
// 'V' camera state, 'Y' teleport.
struct CPlayBackEventNode {
    int                m_nTime;
    unsigned char      m_type;
    char               _pad05[1];
    unsigned short     m_wmParam;
    int                m_nCX;
    int                m_nCY;
    unsigned int       m_nMsgA;
    int                m_nMsgC;
    unsigned char*     m_pMsgData;
    unsigned int       m_nMsgB;
    float              m_fA;
    float              m_fB;
    float              m_fC;
    unsigned long long m_oid;

    // The time and the tag are written after construction.
    CPlayBackEventNode()
        : m_wmParam(0), m_nCX(0), m_nCY(0), m_nMsgA(0), m_nMsgC(0), m_pMsgData(0),
          m_nMsgB(0), m_fA(0), m_fB(0), m_fC(0) {}
};

#include "swpattern.h"

#include "PortDevMode.h"

// One row of the hostile-target highlight list, holding the object id and its hilite scale.
struct CHostileHilite {
    unsigned long long  m_dwObjectId;
    float               m_fHiliteSize;
    char                _pad0c[0x10-0xc];
};

// One row of the per-frame visible-object table, holding the object id, a visibility cache that
// starts out unresolved, and the scan flags.
struct CNearestStruct {
    enum { CANSEE_UNKNOWN = 0xff };   // m_canSee before the line-of-sight test has run
    unsigned long long  m_objectId;
    unsigned char       m_canSee;
    unsigned char       m_flags;
    char                _pad0a[6];
};

// Client application object, the pimpl behind CClientExoApp. It holds the timer set, the
// connection, the module and every piece of per-frame input, movie and GUI state.
class CClientExoAppInternal {
public:
    CClientExoAppInternal();
    ~CClientExoAppInternal();
    CGameObject* GetGameObject(unsigned long long id);

    enum DoDLResDirParam { DoDLOnlineDir = 0, DoDLModulesDir = 1 };
    // m_mouseHideMask's bits, the reasons the mouse is hidden: HideMouse sets one, ShowMouse clears
    // it, and the cursor comes back once none is left.
    enum {
        MOUSEHIDE_MINIGAME = 0x1,
        MOUSEHIDE_LOAD_SCREEN = 0x2,
        MOUSEHIDE_MOUSE_MODE = 0x4,
        MOUSEHIDE_MOVIE = 0x8,
        MOUSEHIDE_DIALOG_CUTSCENE = 0x10,
        MOUSEHIDE_CAMERA_ROTATE = 0x20,
        MOUSEHIDE_REINIT_AURORA = 0x40,
        MOUSEHIDE_EVIL_ENDING = 0x80,
        MOUSEHIDE_ALL = 0xffff,
    };
    // Why the game is paused (SetPausedByCombat's mode, SetAutoPaused's level, m_combatPauseMode).
    // The auto-pause ones each have a client option.
    enum {
        PAUSE_MODE_NONE = 0,
        PAUSE_MODE_ENEMY_SIGHTED = 1,
        PAUSE_MODE_TUTORIAL = 2,
        PAUSE_MODE_SILENT = 3,               // no rumble or sound pause
        PAUSE_MODE_PLAYER = 4,
        PAUSE_MODE_END_OF_COMBAT_ROUND = 5,
        PAUSE_MODE_ACTION_MENU = 7,
        PAUSE_MODE_NEW_TARGET = 8,
        PAUSE_MODE_PARTY_MEMBER_DOWN = 9,
        PAUSE_MODE_MINE_SIGHTED = 11,
        PAUSE_MODE_MESSAGE_BOX = 12,
        PAUSE_MODE_HELP = 13,
        PAUSE_MODE_NO_PENDING = 0xff,        // m_nPendingAutoPauseReason: nothing waiting
    };
    // SetVideoMode's screen mode: keep the current one, windowed or full screen.
    enum {
        VIDEO_SCREEN_CURRENT = 0,
        VIDEO_SCREEN_WINDOWED = 1,
        VIDEO_SCREEN_FULLSCREEN = 2,
    };
    CSWMiniGameObjectArray* m_pMGOArray;
    CClientOptions*     m_pClientOptions;
    CServerInfo*        m_pServerInfo;
    CClientAIMaster*    m_pClientAIMaster;
    CNetLayer*    m_pNetLayer;
    CGameObjectArray*   m_pGameObjectArray;
    CSWCModule*         m_pModule;
    CExoLinkedList<CSWCPlayer> m_playerList;
    unsigned long long  m_playerCreatureId;
    CWorldTimer*        m_pWorldTimer;
    CWorldTimer*        m_pExclusionTimer;
    CWorldTimer*        m_pPauseTimer;
    CWorldTimer*        m_pAnimationTimer;
    CConnectionLib*     m_pConnectionLib;
    CSWCJournal*    m_pQuestJournal;
    CSWCWorldJournal*    m_pWorldJournal;
    CGuiInGame*         m_pInGameGui;
    unsigned long long  m_nReserved1;
    unsigned long long  m_nReserved2;
    unsigned long long  m_nReserved3;
    CGuiPanel*          m_pPlayModulePanel;
    CGuiPanel*          m_pLoadModulePanel;
    unsigned long long  m_containerOpened;
    char                _padb8[0xc0-0xb8];
    unsigned long long  m_nReserved4;
    char                _padc8[0xd0-0xc8];
    unsigned long long  m_nReserved5;
    CCharacterGenerationPanel* m_pCharGenPanel;
    int                 m_bPazaakIsRunning;
    char                _pade4[0xe8-0xe4];
    CExoString          m_sPazaakEndScript;
    int                 m_pazaakResultB;
    int                 m_pazaakResultA;
    int                 m_nPazaakOpponentDeck;
    char                _pad104[4];
    CExoFile*           m_pCaptureEventFile;
    int                 m_captureEvents;
    int                 m_captureEventPlayBack;
    int                 m_captureEventPlayBackRate;
    char                _pad11c[0x120-0x11c];
    CExoLinkedList<CPlayBackEventNode> m_playbackEvents;
    int                 m_inputClass;
    char                _pad12c[0x130-0x12c];
    int                 m_hitCheckMaterialMask;
    unsigned char       m_moveTypeMode;
    char                _pad135[0x138-0x135];
    float               m_fCameraCollision;
    int                 m_nReserved6;
    int                 m_nReserved7;
    unsigned int        m_aReserved1[6];
    int                 m_nScrollSpeedX;
    int                 m_nScrollSpeedY;
    char                _pad164[0x170-0x164];
    int                 m_bClientImeUI;
    char                _pad174[0x178-0x174];
    CImeUI*             m_pImeUI;
#if !KOTOR1
    char                _pad180[0x188-0x180];
#endif
    unsigned long long  m_nLastDriveTime;
    long long           m_drivingTimerReset;
    unsigned long long  m_nDriveKeyPressTime;   // only ever zeroed, in both games: UpdateDriveMode's tap test never passes
    int                 m_bForceDriveUpdate;
    int                 m_keyboardModalFlags[7];
    unsigned long long  m_nReserved8;
    unsigned long long  m_nReserved9;
    CExoMoviePlayer*    m_pMoviePlayer;
    char                _pad1d8[0x1e8-0x1d8];
    CExoString          m_clientName;
    int                 m_clientLanguage;
    unsigned short      m_serverModeChange;
    char                _pad1fe[0x200-0x1fe];
    CSWCMessage*        m_pSWCMessage;
    int                 m_bScreenShotPending;
#if KOTOR1
    int                 m_nScreenShotIndex;
    char                m_sScreenShotFile[16];
#else
    char                m_sScreenShotFile[20];
#endif
    CExoLinkedList<CQueuedAnim> m_queuedAnims;
    float               m_fQueuedAnimDelay;
    char                _pad22c[0x230-0x22c];
    unsigned long long  m_oidReserved1;
    unsigned long long  m_nReserved10;
    unsigned char       m_nReserved11;
    char                _pad241[0x250-0x241];
    void*               m_pLoadGameLoadBar;
    int                 m_loadBarState;
    CExoLinkedList<unsigned long long> m_hitCheckIgnoreList;
    int                 m_debugMode;
    char                _pad26c[0x278-0x26c];
    int                 m_nReserved12;
    int                 m_currentVideoMode;
    unsigned char       m_currentTexturePack;
    char                _pad281[0x284-0x281];
    int                 m_bEndGamePending;
    int                 m_bDisplayEndGameGui;
    int                 m_bGuiGameSlow;
    unsigned char*      m_pCharacterFileBuffer;
    int                 m_characterFileSize;
    int                 m_characterFileSubDir;
    CClientMsgSysAdmin* m_pSysAdminMessenger;
    CSoundSetList*      m_pSoundSetList;
    CResRef*            m_pItemDropSounds;
    unsigned char       m_pauseFlags;
    CExoLinkedList<unsigned long long>* m_pauseExclusionList;
    int                 m_viewTriggers;
    char                _pad2cc[0x2d0-0x2cc];
    CExoArrayList<CExoString> m_sCDKeys;
    CExoString          m_endMovie;
    int                 m_bStartMenuMusicOnMovieEnd;
    int                 m_bSendAreaLoadedAfterMovie;
    char                _pad2f8[0x300-0x2f8];
    int                 m_bDisableMovies;
    int                 m_bDisableIntroMovies;
    int                 m_bShowMouseAtMovieEnd;
    int                 m_bClientInitialized;
    int                 m_bApplicationIdsMatch;
    Vector              m_vLastMovePoint;
    int                 m_nLastMoveTimer;
    int                 m_nLastMoveBusy;
    int                 m_inStartNewModule;
    char                _pad32c[0x330-0x32c];
    unsigned long long  m_lastDialogSpeaker;
    int                 m_bCanSendDialog;
    int                 m_nCanSendDialogTime;
    int                 m_bDialogSent;
    int                 m_bDialogModelsSent;
    CExoStreamingSoundSource* m_pMenuMusic;
    CExoStreamingSoundSource* m_pLoadMusic;
    int                 m_bLoadGameLoadBarActive;
    int                 m_bRenderTilePathNodes;
    void*               m_pWalkToRingVFX;
    unsigned long long  m_nReserved13;
    int                 m_bQuitClicked;
    int                 m_bWMCLOSEQuery;
    int                 m_bReachedMainMenu;
    int                 m_bShowWalkToRingVFX;
    int                 m_nExportRequestState;
    int                 m_nLoadGameError;
    int                 m_bOptionsPanelOpen;
    int                 m_bStartSoundsAfterLoadBar;
    CSWParty*           m_pParty;
    CSWGuiManager*      m_pSWGuiManager;
    CSWGuiLoadScreen*   m_pLoadScreenObj;
    CSWGuiLegalScreen*  m_pLegalScreen;
    class CSWGuiMainMenu*    m_pMainMenuPanel;
    void*               m_pStartScreen;
    int                 m_bAreaNotReadyToDisplay;
    char                _pad3c4[0x3c8-0x3c4];
    unsigned long long  m_sceneCreatureId;
    int                 m_playerCharacterSet;
    char                _pad3d4[0x3d8-0x3d4];
    CExoString          m_playerCharacterName;
    int                 m_controlType;
    char                _pad3ec[0x3f0-0x3ec];
    class CSWPlayerControl* m_pPlayerControl;
    float               m_fDriveInterruptDelay;
    char                _pad3fc[4];
    CExoArrayList<CNearestStruct> m_visibleTable;
    unsigned long long  m_lastTarget;
    unsigned long long  m_lastClickedOnTarget;
    int                 m_bInputDisabled;
    int                 m_nGameOverReason;
    int                 m_bGameOver;
    int                 m_deathSeqLastMs;
    float               m_fDeathSeqAccum;
    float               m_fDeathSeqFade;
    unsigned long long  m_lastPartyMemberTempKilled;
    CExoArrayList<CExoString> m_moveToModuleMovies;
    CExoArrayList<CExoString> m_moviesToPlay;
    CExoArrayList<int>        m_movieFlags;
    CExoString          m_loadScreenName;
    CExoString          m_loadMusic;
    int                 m_bLoadScreenNameSet;
    int                 m_bLoadFromIFO;
    CExoArrayList<CHostileHilite> m_hiliteList;
    int                 m_combatMode;
    int                 m_bEnemySighted;
    int                 m_bTrapSighted;
    int                 m_bRunScriptSet;
    CExoString          m_runScript;
    unsigned long long  m_futureLeader;
    int                 m_bLoadingSaveGame;
    char                _pad4d4[0x4d8-0x4d4];
    CExoArrayList<CSWRumblePattern> m_rumblePatternList;
    CExoArrayList<SRumblePattern> m_rumblePatterns;
    CExoArrayList<CGuiStringStruct> m_stringStructs;
    unsigned long long  m_oidReserved2;
    float               m_fTargetHiddenTime;
    float               m_fCombatStreak;
    float               m_fStatusSummaryDelay;
    float               m_fEndGameDelay;
    float               m_fTargetChanging;
    unsigned char       m_bLastTargetChanged : 1;
    unsigned char       m_flags524_bit1      : 1;
    unsigned char       m_bTogglePauseAtEndOfFrame : 1;
    unsigned char       m_flags524Rest       : 5;
    char                _pad525[0x528-0x525];
    int                 m_combatPauseRequest;
    unsigned char       m_bAutoPaused        : 1;
    unsigned char       m_flags52cRest       : 7;
    unsigned char       m_combatPauseMode;
    char                _pad52e[0x530-0x52e];
    float               m_fEnemyAutoPauseDelay;
    float               m_fEnemyAutoPauseTimer;
    float               m_fEnemySightedTimer;
    unsigned char       m_nPendingAutoPauseReason;
    char                _pad53d[0x540-0x53d];
    float               m_fPartyDownPauseCooldown;
    float               m_fTrapSightedTimer;
    unsigned int        m_nReserved14;
    // The active videoeffects.2da row, or NONE.
    enum { VIDEO_EFFECT_NONE = -2 };
    int                 m_videoEffectType;
    unsigned long long  m_tutorialObjA;
    unsigned long long  m_tutorialObjB;
    unsigned int        m_tutorialTimerSpan;
    unsigned char       m_tutorialReason;
    unsigned long long  m_oidCreatureCache;
    void*               m_pCreatureCache;
    CExoStreamingSoundSource* m_pEvilEndingMusic;
    int                 m_restoreMouseX;
    int                 m_restoreMouseY;
    unsigned long long  m_nReserved15;
    int                 m_mouseHideMask;
    float               m_fMouseShowTimer;
    unsigned int        m_attackMashTutorialTimer;
    unsigned char       m_loadStep[8];
    int             m_actionKeyQueue[10];
    unsigned char   m_actionKeyCount;
    char            _pad5cd[0x5d0-0x5cd];
    LiveContentEntry    m_liveContent[7];
    unsigned int        m_loadScreenHintUpdateTimer;
    unsigned int        m_nLoadScreenHintLastTime;
    unsigned char       m_gamePlayHint;
#if KOTOR1
    unsigned char       m_storyHint[1];
    char                _pad67a[0x67c-0x67a];
    int                 m_bMainMenuReset;       // nothing sets it in either game; DisplayMainMenu clears it
    unsigned char       m_moduleResourceFlags;
    char                _pad681[0x684-0x681];
    unsigned int        m_nReserved16;
    unsigned int        m_nReserved17;
    unsigned long long  m_objectMouseOver;
    Vector              m_mouseOverPos;
    int                 m_bCharacterChangeInProgress;
    int                 m_creditSequence;
    int                 m_bEvilEndingHackInProgress;
    int                 m_bAllowHighMonitorFrequency;
    int                 m_bSoundPausedByCombat;
    CExoArrayList<CSWGuiInterfaceAction> m_actions;
#else
    unsigned char       m_storyHint[11];
    int                 m_bMainMenuReset;       // nothing sets it in either game; DisplayMainMenu clears it
    unsigned char       m_moduleResourceFlags;
    char                _pad691[0x694-0x691];
    unsigned int        m_nReserved16;
    unsigned int        m_nReserved17;
    int                 m_bCharacterChangeInProgress;
    unsigned long long  m_objectMouseOver;
    Vector              m_mouseOverPos;
    int                 m_creditSequence;
    int                 m_bEvilEndingHackInProgress;
    int                 m_bSoundPausedByCombat;
    CExoArrayList<CSWGuiInterfaceAction> m_actions;
    unsigned char       m_superModelSet;
#endif

    void             EnableInput();
    void             DisableInput();
    CSWCModule*      GetModule() __attribute__((noinline));
    void             SetDebugMode(int mode);
    int              GetDebugMode();
    void             SetViewTriggers(int b);
    int              GetViewTriggers();
    void             StopCreditSequence();
    void             ServerModeChange(unsigned char n);
    void             RestoreMousePositionTo(int x, int y);
    int              GetPauseState(unsigned char mask);
    CSWGuiStore*  GetStorePanel();
    int              GetActivePauseState();
    CAurCamera*   GetModuleCamera();
    void             UpdateMoviePlayer();
    void             PerformMButtonDownAction();
    void             PerformMButtonUpAction();
    void             DisplayOptionsLoadError();
    void             DestroyMainMenuPanel();
    int              AdmitNetworkAddress(unsigned int addr, const CExoString& s);
    void             ExitProgram();
    void             SetGameOver(int reason);
    void             DelayNextEnemyAutoPause();
    int              IsMoviePlaying();
    CExoString*      GetPlayerCharacterName();
    __attribute__((noinline)) int GetPausedByCombat();
    void             SetLastTarget(unsigned long long id, int b);
    void BeginIntro();
    int GetVideoModeSpec(unsigned int a, unsigned int* b, unsigned int* c, unsigned char* d, unsigned int* e);
    int ReadVideoModeSettings(unsigned int* a, unsigned int* b, unsigned char* c, unsigned int* d);
    int  SetTexturePack(unsigned char a, int b);
#if KOTOR1
    void InformMoviePlayerOfWindowsPosChanging(int a);   // K1 passes an int, K2 a long
#else
    void InformMoviePlayerOfWindowsPosChanging(int64_t a);
#endif
    void InformMoviePlayerOfWindowsPosChanged();
    void RestartMoviesIfNecessary();
    void SetMouseCursor(int nCursorId, int yPos, int bButtonDown);
    void ResetDrivingTimers(float speed);
    void ClearMovies();
    void PauseMovie();
    void UnpauseMovie();
    void ReloadDynamicEventDescriptions();
    // Always returns null: every area load runs without a load screen.
    CGuiAreaLoadscreen* StartLoadBar(int a, char* b, int c);
    void AddObjectModelDirectlyToIgnoreList(unsigned long long a, List<CAurObject*>* b);
    void AddSimpleObjectModelDirectlyToIgnoreList(unsigned long long a, List<CAurObject*>* b);
    void StartMenuMusic(int a, int b, int c);
    void StopMenuMusic();
    void StopLoadMusic();
    // Adopts the current load-music stream when the requested sound differs from it, else null.
    CExoStreamingSoundSource* AquireLoadMusic(const CResRef& r);
    void RenderTilePathNodes(int a);
    void OnExit();
    void OnGainFocus();
    void OnLostFocus();
    void OnPlayModulePanelExit(int a);
    void OnVideoChange();
    void Uninitialize();
    void SetCaptureEvents(const CExoString& s);
    void SetCaptureEventPlayBack(const CExoString& s);
    void SetCaptureEventPlayBackRate(int a);
    int  StartServices(int a, const CExoString& b, const CExoString& c, const CExoString& d, uint64_t e);
    // Returns whether the event was consumed.
    int HandleInputEvent(int a, int b);
    void PlayBackInputEvents();
    void ReadPlayBackInputEvents();
    void SetEventDescriptions();
    void StopServices();
    void CleanUpModuleServices();
    void ShutDownLoadGameLoadBar();
    void ShutDownLegalScreen();
    void ShutDownToMainMenu();
    void ShowServerMem();
    int IsHostingServer();
    void NextPartyFormation();
    void ReadAndSetVideoMode(int a, int b);
    void LoadItemDropSoundList();
    int              GetNearestVideoMode(unsigned int a, unsigned int b, unsigned char c, unsigned int d);
    int GetVideoMode(unsigned int a, unsigned int b, unsigned char c, unsigned int d);
    // The second argument is tri-state: 1 windowed, 2 fullscreen, anything else follows Aurora.
    int  SetVideoMode(unsigned int a, unsigned char b);
    CSWCPlayer* GetPlayerByGameObjectID(unsigned long long id);
    CSWCStore*         GetStoreByGameObjectID(unsigned long long id);
    CSWCItem*          GetItemByGameObjectID(unsigned long long id);
    CSWCCreature*      GetCreatureByGameObjectID(unsigned long long id);
    CSWCModule*        GetModuleByGameObjectID(unsigned long long id);
    CSWCArea*          GetAreaByGameObjectID(unsigned long long id);
    CSWCTrigger*       GetTriggerByGameObjectID(unsigned long long id);
    CSWCProjectile*    GetProjectileByGameObjectID(unsigned long long id);
    CSWCPlaceable*     GetPlaceableByGameObjectID(unsigned long long id);
    CSWCDoor*          GetDoorByGameObjectID(unsigned long long id);
    CSWCAreaOfEffectObject*  GetAreaOfEffectByGameObjectID(unsigned long long id);
    CSWCSoundObject*   GetSoundObjectByGameObjectID(unsigned long long id);
    void EndLoadBar();
    void CreateQuestJournal();
    void DestroyQuestJournal();
    void CreateWorldJournal();
    void DestroyWorldJournal();
    void DisplayMainMenu(int a);
    void DisplayStartScreen();
    void UnloadModule();
    CSWCModule* CreateModule();
    CSWCCreature* GetPlayerCreature();
    void ReprocessExclusionList();
    CWorldTimer* GetActiveTimer(unsigned long long id);
    void ClearPauseExclusionList();
    void AddToPauseExclusionList(unsigned long long id);
    void AttachPlayerGameObjectToArea() __attribute__((noinline));
    void InitializeInGameGUI();
    void AddObjectToHitcheckIgnoreList(unsigned long long id);
    void RemoveObjectFromHitcheckIgnoreList(unsigned long long id);
    void HandleMouseClickInWorld();
    void HandleMouseOver(unsigned long long id, const Vector& rPos, int b);  // b: pick mode
    void EmptyHitcheckIgnoreList();
    void SetPlayerCreature(unsigned long long id) __attribute__((noinline));
    int  ContinueMessageProcessing();
    int  Initialize();
    int  MainLoop();
    void OnCDChange();
    void SendCharacterResponse(int a);
    CSWCPlayer* GetPlayerByPlayerID(unsigned int playerId);
    CSWCPlayer* GetPlayerByPlayerName(const CExoString& name);
    void SetPauseState(unsigned char state, int a, CExoLinkedList<unsigned long long>* pList);
    void SendSaveGameRequest(unsigned int a, const CExoString& b, const CExoString& c);
    void SendLoadGameRequest(unsigned int a, const CExoString& b, const CExoString& c);
    void JournalUpdated(int a, int b, const CExoString& s);
    int GetObjectName(unsigned long long id, CExoString& out);
    void SetCharacterFile(unsigned int a, unsigned char* pData, int size);
    int  HandleMessage(unsigned int a, unsigned char* pData, unsigned int b, int c);
    bool WalkPlayerToPoint(float x, float y, float z, int a, unsigned long long id, int b);
    void UnloadSuperModelSet();
    CSWCObject* SelectNearestObject(int a, unsigned char b);
    int GetClientRangedAnimation(unsigned long long a, int b, int c, int d);
    __attribute__((noinline)) int GetControlledCharacterDead();
    void PlayerFlourishWeapons();
    void GetDefaultActions();   // builds the radial menu for the current target
    int ShowTutorialWindow(unsigned char a, unsigned long long b, unsigned long long c, unsigned int d);
    uint64_t GetNextLoadScreenHintSTRREF(const CExoString& a, unsigned char b);
    void QueryExitProgram();
    int  StopRumblePattern(int a);
    void EnableVideoEffect(int a);
    void SetPlayerCharacterName(const CExoString& a);
    int AddMoveToModuleMovie(const CExoString& a);   // returns the append's result
    int GetClientMeleeAnimation(unsigned long long a, unsigned long long b, unsigned short c, int d, int e, int f);
    void StartPazaakGame(int a, const CExoString& b, int c, int d, unsigned long long e);
    int PlayLoadModuleMovies();   // 1 when the move-to-module movies were queued or played
    int ChangeCharacterToNextLivingPartyMember(int a, int b, unsigned char c);
    int GetLoadBarProgress();
    void TogglePartyFollow();
    void FadeAndStopStreamingSoundSource(CExoStreamingSoundSource* a, unsigned int b);
    void BusyWaitMenuMusic();
    void SetCanSendDialog(int a);
    int DismissInGameGUI();
    void InitializeMoviePlayer();
    int GetLoadScreenVisible();
    void CancelMovie();
    void HideLoadScreen(int a, int b, int c);
#if KOTOR1
    void HideLoadScreen(int a, int b);
#endif
    void BarkString(unsigned long long a, const CExoString& b, const CResRef& c, int d);
    void AddFloatyText(unsigned long long a, unsigned char b, int c);
    void DisableVideoEffect();
    void UpdateRoom(const Vector& a);
    void SetInputClass(int a, int b);
    CExoString GetLoadMusicByModuleName(const CExoString& a);
    void ChangeCharacter(int a, int b);
    void SetCombatMode(int a);
    void SetMouseMode(unsigned char a);
    void TurnOffKeyboardModal();
    int IsPlayerCreature(CSWCCreature* a);
    int GetCanSendDialog(unsigned long long a, unsigned long long b, CSWSDialog* c);
    void SetLoadScreenByModuleName(const CExoString& a);
    __attribute__((noinline)) void HurryDeathSequence();
#if KOTOR1
    int LoadOptions();
#else
    int LoadOptions(int a);
#endif
    __attribute__((noinline)) void DoScreenShot();
    void SaveOptions();
    void SetPausedByCombat(int a, unsigned char b, int c);
    void StartCreditSequence(const Vector& a, const CExoString& b);
    uint64_t GetLoadScreenHint();
    CSWCPlayer* GetPlayerByCreatureName(const CExoString& a);
    void ResetDriveAcceleration();
    void UpdateDriveMode();
    bool DrivePlayer(unsigned char b);
    void EndPazaakGame(int a, int b, int c, unsigned long long d);
    void PlayMovieQueue(int a);
    void StopSoundPlayingInDialog();
    void ResumeStreamMusicForMovie();
    void HideMouse(int a) __attribute__((noinline));
    void IndicateObject(CSWCObject* a, int b) __attribute__((noinline));
    void ClearCachedCreatures(CSWCCreature* a);
    void SetLoadScreenHint(uint64_t a);
    int  PerformXMouseMoveAction(int nX);
    int  PerformYMouseMoveAction(int nY);
    void PerformRButtonUpAction();
    void PerformRButtonDownAction();
    void PerformLButtonUpAction();
    void PerformLButtonDownAction(int a);       // a is the double-click flag
    void HandleExitGameButton(CSWGuiControl* pControl);
    void PlayCharacterSelectSound(CSWCCreature* pCreature);
    int StartMovie(const CResRef& a);   // 0 when there is no movie player
#if KOTOR1
    int PlayMovie(const CExoString& a, int& b, int c, int d);
    int IsValidResolution(unsigned int nWidth, unsigned int nHeight);
    int PlayMovie(const CExoString& a, int& b, int c, int d, int) { return PlayMovie(a, b, c, d); }
#else
    int PlayMovie(const CExoString& a, int& b, int c, int d, int e);
#endif
    void AddMovieToMovieQueue(const CExoString& sMovie, int nFlags);
    void AddFeedbackMessage(unsigned short nMsg, CSWCCMessageData* pData,
                            const CExoString& sMsg);
    void ShowObject(CSWCObject* pObject, int nPartyIndex, int nMode, int nUnused);
    int IsSoundPlayingInDialog();
    void LookUpAndPerformRumbleWithCutOff(int a, unsigned char b, const Vector& c);
    int  CrownFutureLeader();
    void EnableMouseAtEndOfLoadModuleMovie();
    void ShowMouse(int a) __attribute__((noinline));
    CExoString GetDisplayCharacterForKey(int a);
    void SetLastPartyMemberTempKilled(unsigned long long a);
    void ShutDownMoviePlayer();
    int  ActionIconToMouseCursor(int a, int b);
    CResRef ActionIconToCursorResref(int a, int b);
    void HandleIntroDelete();
    void DoDownloadedResourceDirectories(DoDLResDirParam param, int a);
    void AddDownloadedResources();
    int  AttachPatchFiles(int a);
    void PerformZMouseMoveAction(int zDelta);
    void InitializeClientOptions();
    void InitializeSoundOptions();
    void StopMovie();
    void ShowLoadScreen(int a, int b, int c, int d);
#if KOTOR1
    void ShowLoadScreen(int a, int b, int c);
#endif
    unsigned long long ServerToClientObjectId(unsigned long long a);
#if KOTOR1
    int DontUseThisSetPlayerCreature(unsigned long long oid);
#endif
    void UpdateLoadingScreenHint();
    void SetLoadBarProgress(int a, int b);
    void SetInputActive(int a, int b);
    void StartLoadMusic();
    // Returns the resulting control type; -1 cycles between the first two.
    int  ChangeControlType(int nType);
    void SetAutoPaused(int a, unsigned char b);
    void SetGameSpeed(float a);
    void ProcessInput(float fDeltaT);
    void DoPassiveSelection(float a);
    void UpdateCombatMode();
    void UpdateHostileHilite(float a);
    void UpdateRumble(float a);
    void UpdateSoundEngine();
    void BusyWaitMuteSound();
    void RunDeathSequence();
    CExoString GetGUIString(uint64_t a);
#if !KOTOR1
    bool CanFlourish();
#endif
    void PauseAllSoundsForMovieLostFocus();
    int  PlayRumblePattern(int a);
    void             PlayerListChange(unsigned int a, int b, int c);
    void             HandleCDKeyPanelExit(int a);
    void             HandleLoadBarExit(int a);
    void             HideMovieFadePanel();
    void             AddQueuedAnim(const CExoString& a, unsigned short b, float c, int d);
    void             ClearCameraInput();
    void             DoQuickSave();
    void             UpdateDeathSequence();
    void             UpdateCamera(float f);
    int              IsOnExclusionList(unsigned long long id);
    void             SetEvilEndingHackInProgress(int a);
    void             SetHitcheckIgnoreList(List<CAurObject*>* pList);
    void             AddCreatureModelDirectlyToIgnoreList(unsigned long long creatureId, List<CAurObject*>* pList);
    void             AddItemModelDirectlyToIgnoreList(unsigned long long itemId, List<CAurObject*>* pList);
    __attribute__((noinline)) void SetToolTipInstantMode(int a);
    void             StallServerCode();
    int              AddMovieToExoArrayList(const CExoString& a, CExoArrayList<CExoString>& b);
    int              WaitForMovie(int a);
#if KOTOR1
    int              PlayMoviesInExoArrayList(CExoArrayList<CExoString>& a, int& b, int c, int d, CExoArrayList<int>* e);
    int              PlayMoviesInExoArrayList(CExoArrayList<CExoString>& a, int& b, int c, int d, CExoArrayList<int>* e, int)
                     { return PlayMoviesInExoArrayList(a, b, c, d, e); }
#else
    int              PlayMoviesInExoArrayList(CExoArrayList<CExoString>& a, int& b, int c, int d, CExoArrayList<int>* e, int f);
#endif
};

#if KOTOR1
// K1's exit-game message-box callback holder.
class MacHack {
public:
    void HandleExitGameButton(CSWGuiControl* pControl);
};

K2_LAYOUT_ASSERT(sizeof(CClientExoAppInternal) == 0x6c8, "K1 CClientExoAppInternal size");
K2_LAYOUT_ASSERT(__builtin_offsetof(CClientExoAppInternal, m_nLastDriveTime) == 0x180, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CClientExoAppInternal, m_pLoadScreenObj) == 0x398, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CClientExoAppInternal, m_liveContent) == 0x5c8, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CClientExoAppInternal, m_loadScreenHintUpdateTimer) == 0x670, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CClientExoAppInternal, m_gamePlayHint) == 0x678, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CClientExoAppInternal, m_objectMouseOver) == 0x690, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CClientExoAppInternal, m_bSoundPausedByCombat) == 0x6b4, "K1 layout");
#endif

#include "guiingame.h"

// idiom-exempt: canonical
#include "exostring.h"

// A GUI string bound to the object that owns it.
class CGuiStringStruct {
public:
    uint64_t m_strRef;
    CExoString    m_sText;
};

#include "gameobjectarray.h"

#if !KOTOR1  // K1 defines this out of line
inline CGameObject* CClientExoAppInternal::GetGameObject(unsigned long long id)
{
    CGameObject* pObject = 0;
    m_pGameObjectArray->GetGameObject(id, &pObject);
    return pObject;
}
#endif
