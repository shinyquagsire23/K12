// The in-game HUD/GUI surface, holding the main interface, pause GUI, dialog state, bark bubbles,
// the tutorial box, area-transition state and the per-placeable computer cameras. Owned sub-objects
// are opaque here; the per-translation-unit models live in CGuiInGameInternal.h, which includes
// this back.
#pragma once
#include "k2stdint.h"

#include "exostring.h"
class CResGFF;
class CStatusSummary;

#include "exoresman.h"

#include "CExoLinkedList.h"
#include "CExoArrayListDtors.h"
#include "exolocstring.h"
#include "Vector.h"
#include "Quaternion.h"
#include "serveranimation.h"

// One placeable computer-camera shot, indexed by SetPlaceableCamera and GetPlaceableCamera.
struct CGuiPlaceableCamera {
    int        m_charId;
    Vector     m_position;
    Quaternion m_rotation;
    float      m_a;
    float      m_b;
    float      m_c;
    float      m_d;
};

class CSWGuiInGameMenu;
class CSWGuiMessageBox;
class CSWGuiContainer;
class CSWGuiDialogCinematic;
class CSWGuiExamine;
class CSWGuiPanel;
class CSWGuiStore;
class CSWGuiManager;
class CSWGuiDialogComputer;
class CSWGuiDialogComputerCamera;
class CSWGuiBarkBubble;
class CSWGuiCreateItemSubMenu;
class CSWGuiCreateMedicalItem;
class CSWGuiSwoopUpgrade;
class CSWGuiDialogLetterbox;
class CSWGuiFade;
struct CSWGuiColor;
class CSWGuiMainInterface;
class CSWGuiDeathDisplay;
class CSWGuiTutorialBox;
class CSWGuiSkillInfoBox;
struct CSWGuiExtent;
class CSWGuiStatusSummary;
class CExoStreamingSoundSource;
class CExoLinkedListInternal;
struct CGuiInGameMessage;
struct CGuiInGameDialogMessage;

// Packed to 4 bytes because two 8-byte members sit at 4-mod-8 offsets and are written unaligned,
// and changing the packing part-way through the class would not move the bases.
#pragma pack(push, 4)
class CGuiInGame {
public:
    int                     m_compassEnabled;
    char                    _pad004[4];
    CSWGuiInGameMenu*       m_pInGameMenu;
    CSWGuiPanel*            m_apScreenPanels[8];
    int                     m_lastGuiPanel;
    int                     m_bInGameMenuShown;
    int                     m_swGuiToggleState;
    char                    _pad05c[0x60-0x5c];
    CSWGuiManager*          m_pGuiManager;
    void*                   m_pActiveDialogPanel;
    CSWGuiDialogCinematic*  m_pDialogCinematicPanel;
    CSWGuiDialogComputer*   m_pDialogComputerPanel;
    CSWGuiDialogComputerCamera* m_pDialogComputerCameraPanel;
    CSWGuiBarkBubble*       m_pBarkBubble;
    CSWGuiExamine*          m_pExamineBoxPanel;
    CSWGuiContainer*        m_pContainerGuiPanel;
    CSWGuiPanel*            m_pItemCreatePanel;
    CSWGuiCreateItemSubMenu* m_pSubItemCreateMenuPanel;
    CSWGuiDialogLetterbox*  m_pTopLetterbox;
    CSWGuiDialogLetterbox*  m_pBottomLetterbox;
    CSWGuiDialogLetterbox*  m_pDialogLetterbox;
    CSWGuiFade*             m_pGlobalFade;
    CSWGuiPanel*            m_pLoadModuleDebugPanel;
    CSWGuiPanel*            m_pPowersFeatsSkillsDebugPanel;
    CSWGuiPanel*            m_pMessagesPanel;
    CSWGuiPanel*            m_pPauseGuiPanel;
    CSWGuiPanel*            m_pGalaxyMapPanel;
    // Typed as the concrete store panel because one consumer requires that type; the rest of this
    // file only ever touches it through the panel base.
    CSWGuiStore*            m_pStorePanel;
    CSWGuiPanel*             m_pUpgradeScreenPanel;
#if !KOTOR1  // K1 has no chemical or swoop upgrade screens
    CSWGuiCreateMedicalItem* m_pChemicalUpgradeScreenPanel;
    CSWGuiSwoopUpgrade*      m_pSwoopUpgradeScreenPanel;
#endif
    CSWGuiPanel*            m_pSoloModePanel;
    CSWGuiMainInterface*    m_pMainInterface;
    CSWGuiPanel*            m_pAreaTransitionPanel;
    CSWGuiMessageBox*       m_pSoloModeQueryBox;
#if !KOTOR1  // K1 has no death panel
    CSWGuiPanel*            m_pDatapadMessageBox;
    CSWGuiDeathDisplay*     m_pDeathMessagePanel;
#endif
    CSWGuiSkillInfoBox*     m_pSkillInfoBoxPanel;
    CSWGuiTutorialBox*      m_pTutorialBox;
    CSWGuiPanel*            m_pControllerPausePanel;
    CSWGuiStatusSummary*      m_pStatusSummary;
#if !KOTOR1  // K1 has no demo screen
    char                    _pad168[0x170-0x168];
    CSWGuiPanel*            m_pDemoScreenPanel;
#endif
    CStatusSummary*         m_pStatusFallbackCache;
    int                     m_dialogInputActive;
    unsigned int          m_nGlobalDialogState;
    char                    _pad188[0x190-0x188];
    int                     m_bAnimatedCut;
    int                     m_bOldHitCheck;
    int                     m_dialogSelectionA;
    int                     m_dialogSelectionB;
    int                     m_dialogSelectionActive;
    int                     m_dialogSelectionC;
    int                     m_unpauseableRefCount;
    int                     m_bControllerPauseActive;
    int                     m_bPausedBeforeControllerPause;
    int                     m_lastShotValid;
    Vector                  m_lastShotPos;
    float                   m_lastShotD;
    CGuiInGameMessage*       m_pMessageBuffer;
    CGuiInGameDialogMessage* m_pDialogMessageBuffer;
#if !KOTOR1  // K1 has no combat message log
    CGuiInGameMessage*       m_pCombatMessageBuffer;
#endif
    int                     m_messageBufferSize;
    int                     m_dialogMessageBufferSize;
#if !KOTOR1
    int                     m_combatMessageBufferSize;
#endif
    int                     m_bInitialized;
    int                     m_levelUpMode;
    int                     m_toggledOff;
    unsigned int            m_repliesCount;
#if !KOTOR1
    char                    _pad1fc[4];
#endif
    CExoString*             m_paReplyText;
    unsigned int*           m_paReplyLinkIndices;
    unsigned long long*     m_paReplyListenerIds;
    unsigned int*           m_paReplyCameraAngles;
    int*                    m_paReplyCameraIds;
    float*                  m_paReplyHeightOffsets;
    float*                  m_paReplyTargetHeightOffsets;
    unsigned int*           m_paReplyWaitFlags;
    unsigned short*         m_paReplyCameraAnimations;
    float*                  m_paReplyFOVs;
    int*                    m_paReplyCamVidEffects;
    unsigned char*          m_paReplyFadeTypes;
    Vector*                 m_paReplyPositions;
    float*                  m_paReplyFadeDelays;
    float*                  m_paReplyFadeLengths;
    unsigned int*           m_paReplyFlags;
    void**                  m_paReplyAnimObjectIds;
    void**                  m_paReplyAnimations;
    unsigned int            m_nWaitFlags;
    unsigned short          m_nCameraAnimation;
    char                    _pad296[2];
    int                     m_nReserved1;
    int                     m_bDialogSoundPolled;
    unsigned long long      m_dialogParticipantIdA;
    unsigned long long      m_dialogParticipantIdB;
    unsigned long long      m_oidPrevDialogParticipantA;
    unsigned long long      m_oidPrevDialogParticipantB;
    unsigned long long      m_oidDialogSoundSpeaker;
    unsigned long long      m_oidDialogPlayer;
    unsigned long long      m_oidDialogOwner;    // a server object id (CSWSObject::StartDialog)
    unsigned int            m_stuntModelCount;
    char                    _pad2dc[0x2e0-0x2dc];
    unsigned long long*     m_paStuntModelObjIds;
    CResRef*                m_paStuntModelResRefs;
    unsigned int            m_dialogAnimCount;
    unsigned int            m_lastDialogAnimCount;
    unsigned long long*     m_paDialogAnimData;
    unsigned short*         m_paDialogAnimFlags;
    unsigned long long*     m_paLastDialogAnimData;
    unsigned short*         m_paLastDialogAnimFlags;
    int                     m_placeableCameraCount;
#if KOTOR1  // K1 keeps 50 placeable cameras, K2 70
    CGuiPlaceableCamera     m_placeableCameras[50];
#else
    CGuiPlaceableCamera     m_placeableCameras[70];
#endif
    int                     m_bShowBarkOnDialogEnd;
    CExoString              m_sBarkOnDialogEnd;
    int                     m_bDialogCameraSet;
    int                     m_bAnimatedCameraActive;
    int                     m_bAnimatedCameraLoaded;
    float                   m_fAnimatedCameraLength;
    char                    _pad1060[0x1064-0x1060];
    int                     m_startConversationCount;
    int                     m_bPaused;
    int                     m_bPauseGuiShown;
    CExoStreamingSoundSource* m_pDialogSound;
    CExoLinkedList<unsigned long long> m_orientExclusionList;
    CExoLinkedList<unsigned long long> m_headFollowExclusionList;
    CExoLinkedList<CExoString>         m_dialogNonDependantList;
    int                     m_dialogSpanRemaining;
    int                     m_dialogSpanCount;
    CExoString              m_dialogSpanNodes[8];
    int                     m_bHoldFadeInForDialog;
    char                    _pad111c[0x1128-0x111c];
    unsigned long long      m_oidPostDialogCharacterSwitch;
#if KOTOR1  // 6 bytes here against 33, and no credit-sequence flag
    unsigned char           m_tutorialWindowShown[6];
#else
    unsigned char           m_tutorialWindowShown[33];
    char                    _pad1151[0x1154-0x1151];
    int                     m_creditSequenceInProgress;
#endif
    int                     m_bForceDialogText;
    int                     m_dialogScrollDoneFlag;
    float                   m_fCloseUpPullBackRange;
    char                    _pad1164[0x1168-0x1164];
    CExoStreamingSoundSource* m_pAmbientSound;
    unsigned char           m_nAbilitiesMode;
    unsigned char           m_nInventoryFilter;
    struct {
        unsigned char bShowDoneQuests : 1;
        unsigned char _rest           : 7;
    } m_journalFlags;
    unsigned char           m_nMessageDisplay;
    unsigned int            m_nNoClickDay;
    unsigned int            m_nNoClickTime;
    unsigned int            m_nNoClickDuration;
    int                     m_bConversationUseLeader;
    char                    _pad1184[0x1188-0x1184];
    unsigned long long      m_dialogLipSyncObjId;
    CResRef                 m_dialogLipSyncResRef;
    unsigned long long      m_paLastPartyCharacterIds[3];
    int                     m_bDeferredFOVCall;
    float                   m_deferredFOV;
    int                     m_bRunningGuiScript;
    int                     m_bInputActiveBeforeControllerPause;
    Vector                  m_areaTransitionPosition;
    char                    _pad11d4[4];
    unsigned long long      m_areaTransitionObjId;
    int                     m_bAreaTransitionUseCameraFacing;
    int                     m_bDanceDanceMalak;
#if !KOTOR1
    int                     m_bFadeUntilScript;
    int                     m_bScriptVideoEffect;
#endif
#if !KOTOR1  // K1 has none of these three lists
    int                     m_bKeepStealthInDialog;
    char                    _pad11f4[0x11f8-0x11f4];
    CExoArrayList<CExoLocString>   m_aReceivedItemNames;
    CExoArrayList<unsigned short>  m_aSortedFeats;
    CExoArrayList<unsigned int>    m_aSortedPowers;
#endif

    // This model is truncated; nothing allocates or array-indexes this class, so its real size is
    // unknown.

    CGuiInGame();
    ~CGuiInGame();

    // Kept out of line because the single caller must keep its call.
    void ShutDown() __attribute__((noinline));
    void ShutDownGlobal();
    int  Initialize();
    void InitializeGlobal();

    int GetCanClick();
    void* GetMessageBuffer();
    int GetMessageBufferSize();
    void PauseBarkBubble();
    void ResumeBarkBubble();
    void ChangeCharacter();
    void SetupPauseGuiExtent(CSWGuiExtent* pExtent);
    void ShowGameMenuChangeButtons(int show);
    void IncrementStartConversation();
    void UnToggleOff();
    void SetDialogFade(int a, float b, float c, const Vector& d);
    void StopBarkBubble(int a);
    void SetMainInterfaceTarget(unsigned long long objId, int flag);
    void AddFloatyText(unsigned long long objId, const CExoString& text, const Vector& pos, float f);
    void InitializeMap();
    void ShowFlashingStatus(unsigned char status);
    int  SetTutorialReason(unsigned char reason);  // int, not bool: callers branch on the whole value
    int  GetAreaTransitionVisible();
    void HideAreaTransition();
    void UpdateAreaTransitionGUI();
    void ShowAreaTransition(unsigned long long objId, const Vector& pos);
    int  IsPauseGUIVisible();
    void SetCombatMessage(uint64_t msg);

    // Dialog stunt animation ids, also the camera animation ids GetCameraAnimationName names
    // ("cut001" up): 1000-1327 play once, 1400-1727 loop. Ids from 10000 up are
    // dialoganimations.2da rows.
    enum {
        STUNT_ANIM_FIREFORGET_FIRST = 1000,
        STUNT_ANIM_LOOPING_FIRST = 1400,
        STUNT_ANIM_COUNT = 328,              // ids of each kind
        STUNT_ANIM_SPAN = 728,               // FIREFORGET_FIRST through the last looping id
        DIALOG_ANIM_2DA_BASE = 10000,
        CAMERA_ANIMATION_NONE = SERVER_ANIMATION_NONE,   // m_nCameraAnimation with no camera animation running
    };
    // Lines each message log keeps.
    enum { MESSAGE_BUFFER_SIZE = 64 };

    // m_swGuiToggleState: the HUD is up, the player toggled it off, or a menu screen covers it.
    enum SWGuiToggleState {
        SWGUI_STATE_SHOWN = 1,
        SWGUI_STATE_TOGGLED_OFF = 2,
        SWGUI_STATE_COVERED = 3,
    };
    // SetSWGuiStatus's requests: toggle the HUD on or off, cover it with a screen, uncover it.
    enum SWGuiStatusRequest {
        SWGUI_TOGGLE_ON = 1,
        SWGUI_TOGGLE_OFF = 2,
        SWGUI_COVER = 3,
        SWGUI_UNCOVER = 4,
    };
    // The in-game menu's screens, m_apScreenPanels' index and m_lastGuiPanel.
    enum InGameScreen {
        SCREEN_EQUIP = 0,
        SCREEN_INVENTORY = 1,
        SCREEN_CHARACTER = 2,
        SCREEN_ABILITIES = 3,
#if KOTOR1
        SCREEN_MESSAGES = 4,
#else
        SCREEN_PARTY_SELECTION = 4,
#endif
        SCREEN_JOURNAL = 5,
        SCREEN_MAP = 6,
        SCREEN_OPTIONS = 7,
    };
    // UpdateStatus's status ids: the status summary's rows, then the three notification sounds.
    enum GuiStatus {
        GUI_STATUS_JOURNAL = 0,
        GUI_STATUS_CREDITS = 1,
        GUI_STATUS_XP = 2,
        GUI_STATUS_STEALTH_XP = 3,
        GUI_STATUS_ALIGNMENT_SHIFT = 4,  // signed: light side up, dark side down
        GUI_STATUS_ALIGNMENT_SHIFT_UNUSED = 5,  // same handler as 4; nothing sends it
        GUI_STATUS_ITEM_RECEIVED = 7,
        GUI_STATUS_ITEM_LOST = 8,
#if KOTOR1
        GUI_STATUS_LEVEL_UP_SOUND = 9,
        GUI_STATUS_NEW_QUEST_SOUND = 10,
        GUI_STATUS_COMPLETE_SOUND = 11,
#else
        GUI_STATUS_INFLUENCE_GAINED = 9,
        GUI_STATUS_INFLUENCE_LOST = 10,
        GUI_STATUS_MAX_FP_GAINED = 11,
        GUI_STATUS_MAX_FP_LOST = 12,
        GUI_STATUS_LEVEL_UP_SOUND = 100,
        GUI_STATUS_NEW_QUEST_SOUND = 101,
        GUI_STATUS_COMPLETE_SOUND = 102,
#endif
    };

    static Vector COLOR_NEW_HIGHLIGHT;
    static Vector COLOR_LIGHT_GOLD;
    static Vector COLOR_NEW_TEXT;
    static CSWGuiColor COLOR_NEW_BONUS;
    static Vector COLOR_DARK_TAN;
    static Vector COLOR_RED;
    static Vector COLOR_GREEN;
    static Vector COLOR_WHITE;
    static Vector COLOR_TRANSPARENT;
    static Vector COLOR_BLACK;
    static Vector COLOR_PURPLE;
    static Vector COLOR_ORANGE;
    static Vector COLOR_BLUE;
    static Vector COLOR_YELLOW;
    static Vector COLOR_UPGRADE_BORDER;
    static Vector COLOR_UPGRADE_TEXT;
    static const Vector DISABLED_TEXT;
    static const Vector DISABLED_ICON_COLOR;
    static const Vector ENABLED_ICON_COLOR;
    static const Vector COLOR_PAZAAK_GOLD;
#if KOTOR1
    static const Vector STANDARD_BLUE;
#endif
    static Vector COLOR_NEW_GREEN;
    void SetDeferredFOVCallForAnimatedCutscenes(float fov);
    void SetLevelUpMode(int mode);
    void ShowDialogEntry(const CExoString& msg, unsigned long long a, unsigned long long b);
    int  ShowLevelUpGUI(int mode);  // int, not bool: callers use the whole value
    int IsCameraAnimationPlayingInDialog();
    void RePopulateMainInterface();
    void SuppressStatusSummary(int flag);
    void SetTopMenuOverrideCharacter(int charId);
    void ToggleOff();
    int ShowSWInGameGui(int panelIndex);
    void ShowDialogReplies(unsigned int a, unsigned int b);
    void SetDialogSelection(int a, int b, int c);
    void UpdateDialog();
    void HandleDialogSelection(int index, int hasText);
    bool HandleDialogEntry(unsigned int a1, unsigned long long objIdA, unsigned long long objIdC,
                            unsigned long long objIdB, unsigned long long objIdD, unsigned char a6,
                            const CExoString& text, unsigned int animCount, unsigned long long* paAnimData,
                            unsigned short* paAnimFlags, unsigned int nCameraAngle, int charId,
                            float floatC, float floatD, unsigned int arg, unsigned short cameraId,
                            float speed, int effectFlag, unsigned char letterboxFadeType,
                            const Vector& fadePos, float fadeB, float fadeC, int bForceText);
    void SetDialogAnimations();
    void UpdateShotCamera(unsigned int a, int b, float c, float d, unsigned int e, int f);
    int  HandleDialogSound(unsigned long long objId, const CResRef& resRef);
    void SetDialogLipSync(unsigned long long objId, const CResRef& resRef);
    void UpdateDialogLipSync();
    CExoString DialogNodeSpanningTextParse(const CExoString& text);
    void SetAreaTransition(unsigned long long objId, const Vector& pos);
    void ResetTutorialWindowTracker();
    int InitializePlaceableCameras(int count);
    void* GetPlaceableCamera(int index);
    unsigned char GetInterfaceOffset();
    uint64_t GetCombatMessage();
    void EnableCharacterChangeButtons(int flag);
    int GetTopMenuOverrideCharacter();
    void* GetGuiInGameScreenPanel(unsigned char index);
    void GetStatusSummary(class CStatusSummary& out);
    void UpdateStatus(unsigned char status, int value);  // the status is a single byte
    void SetGlobalDialogState(int state);
    int CloseDialog();
    int  IsCameraDialog();
    int  IsInDialog(unsigned long long objId);
    void SetGuiInGameInventoryDirty(int flag);
    unsigned int GetTutorialWindowHasBeenShown(unsigned char index);
    void ShowDialogMessage();
    void SetDialogAnimationData(unsigned int index, unsigned long long data, unsigned short flags);
    int  SetAnimatedCamera(unsigned short cameraId, float speed);
    void DialogGuiDoneScroll();
    Vector GetAreaTranstionPosition();
    Vector GetDialogCameraPosition(int bOffsetByOrientation);

    static Vector COLOR_CYAN;
    static Vector COLOR_DARK_GOLD;
    void SetGlobalUnpauseableState(int flag);
    int            GetPendingStatusSummary();
    int IsGlobalFadeObscuring();  // int, not bool: callers test the whole value
    bool InitializeStuntModelData(unsigned int count);
    void SetStuntModelData(unsigned int index, unsigned long long objId, const CResRef& resRef);
    int IsFadingInDialog();
    int IsGlobalFading();  // int, not bool: callers test the whole value
    void SetTutorialWindowHasBeenShown(unsigned char index, int flag);
    void ClearDialogAnimationData();
    void ClearLastDialogAnimationData();
    int  IsFireForgetDialogAnimation(unsigned short a);
    int  IsLoopingDialogAnimation(unsigned short a);
    void StopGlobalFade();
    void ShowBarkBubble(unsigned long long objId, const CExoString& text, const CResRef& resRef, int flag);
    bool HandleDialogReplyChosen(unsigned int a1, unsigned int a2, unsigned int a3, const CExoString& text,
                                  unsigned long long objId, unsigned char b, int c);
    void HideBarkBubble();
    void SetPlaceableCamera(int index, int charId, const Vector& pos, const Quaternion& rot, float a, float b, float c, float d);
    int InitializeShotCamera();
    void SetShotFromScript(int charId, int effectFlag);
    // Declared, never defined.
    void SetComputerCameraShot(int charId, unsigned int arg, int effectFlag);
    void HandleDialogClose(int flag);
    int  LoadStuntModelsForDialog();
    void UnloadStuntModelsForDialog();
    int  HandleDialogModels(unsigned int count, unsigned int count2, unsigned long long* paObjIds,
                             CResRef* paResRefs, const CResRef& cameraResRef,
                             const CResRef& ambientResRef);
    void ShowItemCreateMenu();
    void ShowLoadModuleDebugMenu();
    void ShowPowersFeatsSkillsDebugMenu();
    void MarkNoClickEvent(unsigned int reason);
    void ClearDialogNonDependantList();
    void UpdateMessageGUI(CSWGuiPanel* pPanel);
#if KOTOR1  // K1 always uses the message-log screen slot, with no combat log
    void UpdateMessageGUI();
#endif
    void UpdateAbilitiesCharacter();
    void UpdateInventory();
    void UpdateCreatedInGameGUI(int oldPanel, int newPanel);
    int  HideSWInGameGui(int notify);  // int, not bool: callers branch on the whole value
    void ResetTalkAnimations();
    void ResetDialogAnimations();
    int CanPlayDialogAnimations(unsigned long long objId);
    void NextSWInGameGui();
    void PrevSWInGameGui();
    void SwitchToSWInGameGui(int newPanel);
    int  GetBarkBubbleVisible(int& out);
    void FadeAndStopDialogAmbientTrack();
    int  GetGlobalFadeActive();

    void ResetComputerCameraShot();
    void LoadCameraModelForDialog(const CResRef& resRef);
    void LoadAmbientTrackForDialog(const CResRef& resRef);
    void EndDialogCamera();
    void ShowSubItemCreateMenu(unsigned int flag);
    void SetStatusSummary(const CStatusSummary& in);
    void ShowStatusSummary();
#if !KOTOR1
    void RemoveDeathMessage(bool removePanel);
#endif
    void SetPaused(int pause, unsigned char reason);
    void UpdateControllerPauseState();
    void ShowExamineBox(uint64_t objId, int flag);
    void ShowExamineBox(const CExoString& text, int flag);
    bool InitializeDialogAnimationData(unsigned int count);
    int GetMiniMapVisible();  // int, not bool: callers test the whole value
    void ShowContainerGui(unsigned long long objId, int flag);
#if !KOTOR1
    void DisplayDeathMessage();
#endif
    void EndControllerPauseState();
    void HideContainerGui();
    void SetConversationType(unsigned int type, unsigned char flag);
    void HideSubItemCreateMenu();
    void HideExamineBox();
    void HideCreateItemScreen();
    void EnterControllerPauseState();
    void HideItemCreateMenu();
    void HideLoadModuleDebugMenu();
    void HidePowersFeatsSkillsDebugMenu();
    int  TutorialReasonWillShowWindow(unsigned char reason);  // int, not bool, for the same reason
    bool ShowTutorialWindow(unsigned char reason, unsigned long long instigatorId,
                             unsigned long long targetId, unsigned int type);

    void AppendToDialogBuffer(const CExoString& text, const CExoString& speaker);
    void AppendToDialogBuffer(const CExoString& text, unsigned long long objId);
    void AppendToCombatBuffer(const CExoString& text, unsigned int a, unsigned char b);
    void AppendToMsgBuffer(const CExoString& text, unsigned int a, unsigned char b);
    void ResetInterfaceFont();
    void RemoveGUIPopUps();
    void HideSoloMode();
    void ShowSoloModeQuery(int text);
    int  GetFadeOverridePanelVisible();  // int, not bool: callers test the whole value
    int IsAnimationPlayingInDialog();
    void UnlockDialogParticipantOrientations();
    void StartGlobalFade(int a, float b, float c, const Vector& d);
    int IsOnDialogNonDependantList(const CExoString& text);  // int, not bool: stored into an int unmodified
    void AddToDialogNonDependantList(const CExoString& text);
    void SetSWGuiStatus(int a, int b);
    void ShowGalaxyMapGui(int planetIndex);
    void HideGalaxyMapGui();
    void ShowStoreGui(unsigned long long a, unsigned long long b);
    void HideStoreGui();
    void ClearRepliesData();
#if KOTOR1  // returns 0 in K1
    int  InitializeRepliesData(unsigned int count);
#else
    void InitializeRepliesData(unsigned int count);
#endif
    void SetReplyData(unsigned int index, const CExoString& text, unsigned int a, unsigned long long b,
                       unsigned int c, int d, float e, float f, unsigned int g, unsigned short h,
                       float i, int j, unsigned char k, const Vector& pos, float m, float n,
                       unsigned int count, unsigned long long* paU, unsigned short* paT);
    bool HandleDialogReplies(unsigned int a1, unsigned long long a2, unsigned char a3,
                              CExoString* paText, unsigned int* paA, unsigned long long* paB,
                              unsigned int* paC, int* paD, float* paE, float* paF, unsigned int* paG,
                              unsigned short* paH, float* paI, int* paJ, unsigned char* paK,
                              Vector* paPos, float* paM, float* paN, unsigned int a4, unsigned int a5,
                              unsigned int a6, unsigned int* paCount, unsigned long long** paU,
                              unsigned short** paT, int a7);
    void CleanUpEntryNode();
    void CleanUpReplyNode();
    bool GetCanOrientInDialog(unsigned long long objId);
    bool GetCanHeadFollowInDialog(unsigned long long objId);
    void SetLockObjectOrientationInDialog(unsigned long long objId, int lock);
    void SetLockHeadFollowInDialog(unsigned long long objId, int lock);
    void HideUpgradeScreen();
    void HideChemicalUpgradeScreen();
    void HideSwoopUpgradeScreen();
    void HideDemoScreen();
    void ShowDemoScreen(CExoString name, int a, int b, int c, int d);
#if KOTOR1  // K1 takes only the item id
    void ShowUpgradeScreen(unsigned long long itemObjId);
    void ShowUpgradeScreen(unsigned long long itemObjId, unsigned long long creatureObjId,
                           int a, int b, CExoString desc) { K2_UNUSED(creatureObjId); K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(desc); ShowUpgradeScreen(itemObjId); }
#else
    void ShowUpgradeScreen(unsigned long long itemObjId, unsigned long long creatureObjId,
                           int a, int b, CExoString desc);
#endif
    void ShowChemicalUpgradeScreen(unsigned long long objId);
    void ShowSwoopUpgradeScreen(unsigned long long objId);

    void StoreLastDialogAnimationData();
    void DoQuickLoad();
    bool UnpackQuickSaveGame(unsigned int a, const CExoString& b);
    bool CopyQuickSaveGameToFutureGame(unsigned int a, const CExoString& b);
    void ResetInterfaceForSize();
#if !KOTOR1  // K1's variant takes no extra flag
    void ShowPartySelection(const CExoString& text, int a, int forceNpc1, int forceNpc2, int d);
#else
    void ShowPartySelection(const CExoString& text, int a, int forceNpc1, int forceNpc2);
    int  GetPartyAccessPanelUp(int bEquipOnly);
    int  GetCurrentEquipScreenNPC();
#endif

    CExoString GetCameraAnimationName(unsigned short nState);

    void ShowReplyChosen(unsigned int a, unsigned int b, const CExoString& c);
    int  HandleInventoryClose(int flag);
    void CreateDeathMenuGUI();
    void ClearMapDisplays();
    void StartCreditSequence(const Vector& pos, const CExoString& text);
    void PreloadMaps();
    void HideDebugMenu();
    void ShowDebugMenu();
    void DestroyDeathMenuGUI();

    int  IsLoopingDialogStuntAnimation(unsigned short anim);  // int: compared against zero whole
    bool IsFireForgetDialogStuntAnimation(unsigned short anim);
    bool IsDialogStuntAnimation(unsigned short anim);
    bool IsOverlayDialogAnimation(unsigned short anim);

    void ToggleSWGui();
};
#pragma pack(pop)

#if KOTOR1  // K1 size and field offsets
K2_LAYOUT_ASSERT(sizeof(CGuiInGame) == 0xdc8, "CGuiInGame K1 size drift");
K2_LAYOUT_ASSERT(__builtin_offsetof(CGuiInGame, m_pSoloModeQueryBox) == 0x120, "K1 CGuiInGame layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CGuiInGame, m_pTutorialBox) == 0x130, "K1 CGuiInGame layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CGuiInGame, m_bInitialized) == 0x1b0, "K1 CGuiInGame layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CGuiInGame, m_bShowBarkOnDialogEnd) == 0xc3c, "K1 CGuiInGame layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CGuiInGame, m_tutorialWindowShown) == 0xd30, "K1 CGuiInGame layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CGuiInGame, m_bForceDialogText) == 0xd38, "K1 CGuiInGame layout");
#endif
