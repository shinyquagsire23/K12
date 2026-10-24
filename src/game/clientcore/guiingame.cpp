#include "k2stdint.h"
#include "port/Mac/MacMain.h"
#include "swguiexamine.h"
#include "serverexoappinternal.h"
#include <stddef.h>
#include "swguiingameequip.h"
#include "swguiingameinventory.h"
#include "swguidynamic.h"
#include "exoresman.h"
#include "guiingame.h"
#include "nwsobject.h"
#include "nwsplaceable.h"
#include "nwsrules.h"
#include "nwscriptconst.h"
#include "serveranimation.h"
#include "restype.h"
#include "include/aurbehavior.h"
#include "soundprioritygroup.h"

int g_bCreateAllGUIs = 1;

int bDisableLetterbox;

#if !KOTOR1
bool AspyrHackCarthDialog;
float AspyrHackCameraFocus;
float AspyrHackCameraDistance;
float AspyrHackCameraYaw;
#endif

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"
#include "port/crtapi.h"

CSWGuiDynamic* g_pDynamicGUI;

// CSWCAnimBase::GetModel's selectors for the body model and the head model.
enum {
    ANIMBASE_MODEL_BODY = 0xff,
    ANIMBASE_MODEL_HEAD = 0xfe,
};

// A dialog shot's video effect beside the videoeffects.2da rows (nwscript's VIDEO_EFFECT_*):
// none, and none without the live-feed caption.
enum {
    VIDEO_EFFECT_NONE_NO_CAPTION = -2,
};

#if !KOTOR1
// appearance.2da rows the Aspyr dialog-camera fixes single out.
enum {
    APPEARANCE_REPUBLIC_OFFICER_MALE_OLD_WHITE = 289,
    APPEARANCE_REPUBLIC_OFFICER_MALE_OLD_BLACK = 290,
    APPEARANCE_REPUBLIC_OFFICER_FEMALE_OLD_BLACK = 363,
    APPEARANCE_ALIEN_GRAN_01 = 565,
};
#endif

// The pause reasons (CClientExoAppInternal::PAUSE_MODE_*) that put up the pause banner; 10 is
// "Action added to queue.".
enum {
    PAUSE_REASONS_WITH_BANNER = (1 << CClientExoAppInternal::PAUSE_MODE_ENEMY_SIGHTED) |
                                (1 << CClientExoAppInternal::PAUSE_MODE_PLAYER) |
                                (1 << CClientExoAppInternal::PAUSE_MODE_END_OF_COMBAT_ROUND) |
                                (1 << CClientExoAppInternal::PAUSE_MODE_ACTION_MENU) |
                                (1 << CClientExoAppInternal::PAUSE_MODE_NEW_TARGET) |
                                (1 << CClientExoAppInternal::PAUSE_MODE_PARTY_MEMBER_DOWN) |
                                (1 << 10) |
                                (1 << CClientExoAppInternal::PAUSE_MODE_MINE_SIGHTED),
};

extern int quaternionidentityoptimization;
#if KOTOR1
bool HasTouchScreen();
void IosTemporaryDisableInteraction(int nDisable);
#endif

#if KOTOR1
const Vector CGuiInGame::ENABLED_ICON_COLOR(1.0f, 1.0f, 1.0f);
#endif
const Vector CGuiInGame::DISABLED_ICON_COLOR(0.35f, 0.35f, 0.35f);
Vector CGuiInGame::COLOR_TRANSPARENT(-1.0f, -1.0f, -1.0f);
#if KOTOR1
const Vector CGuiInGame::STANDARD_BLUE(0.0f, 0.64f, 0.96f);
#endif
Vector CGuiInGame::COLOR_BLACK(0.0f, 0.0f, 0.0f);
Vector CGuiInGame::COLOR_WHITE(1.0f, 1.0f, 1.0f);
Vector CGuiInGame::COLOR_BLUE(0.0f, 0.66f, 0.98f);
Vector CGuiInGame::COLOR_YELLOW(0.98f, 1.0f, 0.0f);
Vector CGuiInGame::COLOR_CYAN(0.4f, 1.0f, 1.0f);
Vector CGuiInGame::COLOR_RED(0.74f, 0.11f, 0.0f);
Vector CGuiInGame::COLOR_GREEN(0.28f, 0.92f, 0.11f);
Vector CGuiInGame::COLOR_ORANGE(0.98f, 0.45f, 0.0f);
Vector CGuiInGame::COLOR_PURPLE(0.95f, 0.0f, 0.85f);
#if KOTOR1
const Vector CGuiInGame::DISABLED_TEXT(0.0f, 0.33f, 0.49f);
#else
const Vector CGuiInGame::DISABLED_TEXT(0.4f, 0.4f, 0.3f);
#endif
Vector CGuiInGame::COLOR_NEW_GREEN(0.05f, 0.35f, 0.27f);
Vector CGuiInGame::COLOR_NEW_HIGHLIGHT(0.7f, 0.7f, 0.6f);
Vector CGuiInGame::COLOR_NEW_TEXT(0.1f, 0.7f, 0.55f);
CSWGuiColor CGuiInGame::COLOR_NEW_BONUS(0.95f, 0.66f, 0.3f);
Vector CGuiInGame::COLOR_LIGHT_GOLD(0.64f, 0.53f, 0.18f);
Vector CGuiInGame::COLOR_DARK_GOLD(0.31f, 0.25f, 0.09f);
Vector CGuiInGame::COLOR_DARK_TAN(0.4f, 0.4f, 0.3f);
const Vector CGuiInGame::COLOR_PAZAAK_GOLD(0.8f, 0.65f, 0.2f);
Vector CGuiInGame::COLOR_UPGRADE_TEXT(0.0f, 0.66f, 0.98f);
Vector CGuiInGame::COLOR_UPGRADE_BORDER(0.0f, 0.33f, 0.5f);


#if !KOTOR1  // K2-only (no K1 counterpart)
bool IsPlayerControllingT3M4Tutorial()
{
    if (g_pAppManager == 0)
        return false;
    if (g_pAppManager->m_pServerExoApp == 0)
        return false;
    if (g_pAppManager->m_pServerExoApp->GetPartyTable() == 0)
        return false;
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nControlledNPC != 8)
        return false;
    return g_pAppManager->m_pServerExoApp->GetPartyTable()->m_selectedPlanet == -1;
}
#endif // !KOTOR1

char* showcredits(char* pszArgs)
{
    Vector vPos = { 0.0f, 0.0f, 0.0f };
    static_cast<void>(sscanf(pszArgs, "%f %f %f", &vPos.x, &vPos.y, &vPos.z));
    g_pAppManager->m_pClientExoApp->StartCreditSequence(vPos, CExoString(""));
    return const_cast<char*>("Success");
}

char* creditsinprogress(char*)
{
    return const_cast<char*>(g_pAppManager->m_pClientExoApp->GetCreditSequenceInProgress()
                       ? "True"
                       : "False");
}

char* HideGUI(char*)
{
    if (g_pDynamicGUI != 0) {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->RemovePanel(g_pDynamicGUI);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
        delete g_pDynamicGUI;
        g_pDynamicGUI = 0;
    }
    return const_cast<char*>("");
}

char* ShowGUI(char* pszName)
{
    if (*pszName == 0)
        return const_cast<char*>("need to specify the GUI to create");

    if (g_pDynamicGUI != 0) {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->RemovePanel(g_pDynamicGUI);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
        delete g_pDynamicGUI;
        g_pDynamicGUI = 0;
    }

    g_pDynamicGUI = new CSWGuiDynamic(g_pAppManager->m_pClientExoApp->GetSWGuiManager(),
                                      CResRef(pszName));
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(g_pDynamicGUI, 0, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    return const_cast<char*>("");
}

CGuiInGame::CGuiInGame()
{
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCameraMode(CClientOptions::CAMERA_MODE_ON_A_STICK);

    m_pMessageBuffer = new CGuiInGameMessage[MESSAGE_BUFFER_SIZE];
    m_pDialogMessageBuffer = new CGuiInGameDialogMessage[MESSAGE_BUFFER_SIZE];
#if !KOTOR1
    m_pCombatMessageBuffer = new CGuiInGameMessage[MESSAGE_BUFFER_SIZE];
#endif

    m_dialogParticipantIdA = OBJECT_INVALID;
    m_dialogParticipantIdB = OBJECT_INVALID;
    m_journalFlags.bShowDoneQuests = 0;
    m_nAbilitiesMode = 0;
    m_nInventoryFilter = 0;
    m_nMessageDisplay = 0;

    m_dialogInputActive = 0;
    m_pStatusFallbackCache = 0;
    m_startConversationCount = 0;
    m_nReserved1 = 0;
    m_dialogAnimCount = 0;
    m_lastDialogAnimCount = 0;

    m_pInGameMenu = 0;
    m_apScreenPanels[0] = 0;
    m_apScreenPanels[1] = 0;
    m_apScreenPanels[2] = 0;
    m_apScreenPanels[3] = 0;
    m_apScreenPanels[4] = 0;
    m_apScreenPanels[5] = 0;
    m_apScreenPanels[6] = 0;
    m_apScreenPanels[7] = 0;
    m_lastGuiPanel = SCREEN_EQUIP;
    m_bInGameMenuShown = 0;
    m_swGuiToggleState = 0;

    m_pActiveDialogPanel = 0;
    m_pDialogCinematicPanel = 0;
    m_pDialogComputerPanel = 0;
    m_pDialogComputerCameraPanel = 0;
    m_pBarkBubble = 0;
    m_pExamineBoxPanel = 0;
    m_pContainerGuiPanel = 0;
    m_pItemCreatePanel = 0;
    m_pSubItemCreateMenuPanel = 0;
    m_pTopLetterbox = 0;
    m_pBottomLetterbox = 0;
    m_pDialogLetterbox = 0;
    m_pGlobalFade = 0;
    m_pLoadModuleDebugPanel = 0;
    m_pPowersFeatsSkillsDebugPanel = 0;
    m_pMessagesPanel = 0;
#if !KOTOR1
    m_pSwoopUpgradeScreenPanel = 0;
#endif
    m_pGalaxyMapPanel = 0;
    m_pStorePanel = 0;
    m_pUpgradeScreenPanel = 0;
#if !KOTOR1
    m_pChemicalUpgradeScreenPanel = 0;
#endif
    m_pMainInterface = 0;
    m_pAreaTransitionPanel = 0;
    m_pSoloModeQueryBox = 0;
#if !KOTOR1
    m_pDatapadMessageBox = 0;
    m_pDeathMessagePanel = 0;
#endif
    m_pSkillInfoBoxPanel = 0;
    m_pControllerPausePanel = 0;
    m_pStatusSummary = 0;

    m_levelUpMode = 0;
    m_toggledOff = 0;
    m_repliesCount = 0;
    m_messageBufferSize = 0;
    m_dialogMessageBufferSize = 0;
#if !KOTOR1
    m_combatMessageBufferSize = 0;
#endif
    m_bInitialized = 0;

    m_paReplyText = 0;
    m_paReplyLinkIndices = 0;
    m_paReplyListenerIds = 0;
    m_paReplyCameraAngles = 0;
    m_paReplyCameraIds = 0;
    m_paReplyHeightOffsets = 0;
    m_paReplyTargetHeightOffsets = 0;
    m_paReplyWaitFlags = 0;
    m_paReplyCameraAnimations = 0;
    m_paReplyFOVs = 0;
    m_paReplyCamVidEffects = 0;
    m_paReplyFadeTypes = 0;
    m_paReplyPositions = 0;
    m_paReplyFadeDelays = 0;
    m_paReplyFadeLengths = 0;
    m_paReplyFlags = 0;
    m_paReplyAnimObjectIds = 0;
    m_paReplyAnimations = 0;

    m_nWaitFlags = 0;
    m_bDialogCameraSet = 0;
    m_bAnimatedCameraActive = 0;
    m_bAnimatedCameraLoaded = 0;
    m_fAnimatedCameraLength = 0;
    m_nCameraAnimation = CAMERA_ANIMATION_NONE;
    m_stuntModelCount = 0;
    m_pGuiManager = 0;
    m_paStuntModelObjIds = 0;
    m_paStuntModelResRefs = 0;
    m_paDialogAnimData = 0;
    m_paDialogAnimFlags = 0;
    m_paLastDialogAnimData = 0;
    m_paLastDialogAnimFlags = 0;

    m_oidPrevDialogParticipantA = OBJECT_INVALID;
    m_oidPrevDialogParticipantB = OBJECT_INVALID;
    m_oidDialogSoundSpeaker = 0xffffffff;
    m_oidDialogPlayer = OBJECT_INVALID;

    m_bShowBarkOnDialogEnd = 0;
    m_sBarkOnDialogEnd = "";
    m_nGlobalDialogState = 0;
    m_bAnimatedCut = 0;
    m_bOldHitCheck = 0;
    m_compassEnabled = 1;

#if !KOTOR1  // K1: the compass is always on
    CExoIni ini;
    CExoString value;
    if (ini.ReadIniEntry(value, ".\\swkotor2.ini", "Display Options", "Compass") != 0) {
        if (value.m_sString != 0)
            m_compassEnabled = atoi(value.m_sString) != 0;
        else
            m_compassEnabled = 0;
    }
#endif

    m_placeableCameraCount = 0;
    m_lastShotValid = 0;
    m_lastShotD = 0;
    m_pDialogSound = 0;
    m_bDialogSoundPolled = 0;
    m_bPaused = 0;
    m_dialogSelectionA = 0;
    m_dialogSelectionB = 0;
    m_dialogSelectionActive = 0;
    m_dialogSelectionC = 0;
#if !KOTOR1
    Ios_minigamePaused = 0;
#endif
    m_bPauseGuiShown = 0;
    m_dialogSpanRemaining = 0;
    m_dialogSpanCount = 0;
    m_bHoldFadeInForDialog = 0;
    m_oidPostDialogCharacterSwitch = OBJECT_INVALID;
#if KOTOR1
    for (int i = 0; i < 6; i++)
        m_tutorialWindowShown[i] = 0;
#else
    for (int i = 0; i < 33; i++)
        m_tutorialWindowShown[i] = 0;
#endif
    m_bPausedBeforeControllerPause = 0;
    m_bForceDialogText = 0;
    m_dialogScrollDoneFlag = 0;
    m_pAmbientSound = 0;
    m_fCloseUpPullBackRange = 0;
    m_unpauseableRefCount = 1;
    m_bControllerPauseActive = 0;
    m_nNoClickDay = 0;
    m_nNoClickTime = 0;
    m_nNoClickDuration = 0;
    m_bConversationUseLeader = 0;
    m_dialogLipSyncObjId = OBJECT_INVALID;
    m_dialogLipSyncResRef = const_cast<char*>("");
#if !KOTOR1
    m_bFadeUntilScript = 0;
    m_bScriptVideoEffect = 0;
#endif
    m_paLastPartyCharacterIds[2] = OBJECT_INVALID;
    m_oidDialogOwner = OBJECT_INVALID;
    m_areaTransitionObjId = OBJECT_INVALID;
    m_bDeferredFOVCall = 0;
    m_deferredFOV = 0.0f;
    m_bRunningGuiScript = 0;
    m_bInputActiveBeforeControllerPause = 0;
    m_areaTransitionPosition = Vector(0.0f, 0.0f, 0.0f);
#if !KOTOR1
    m_bKeepStealthInDialog = 0;
#endif
    m_bAreaTransitionUseCameraFacing = 1;
#if KOTOR1
    m_bDanceDanceMalak = 0;
#endif
    m_paLastPartyCharacterIds[0] = OBJECT_INVALID;
    m_paLastPartyCharacterIds[1] = OBJECT_INVALID;
#if !KOTOR1
    m_pDemoScreenPanel = 0;
#endif
}

CGuiInGame::~CGuiInGame()
{
    ShutDown();
    ShutDownGlobal();

    delete[] m_pDialogMessageBuffer;
    delete[] m_pMessageBuffer;
#if !KOTOR1  // K1 has no combat message log
    delete[] m_pCombatMessageBuffer;
#endif

    // Only the orient list is drained; its elements are raw objIds.
    while (m_orientExclusionList.GetHeadPos() != 0)
        delete m_orientExclusionList.RemoveHead();

    {
        CExoStreamingSoundSource* pAmbient = m_pAmbientSound;
        if (pAmbient != 0) {
            pAmbient->~CExoStreamingSoundSource();
            operator delete(pAmbient);
            m_pAmbientSound = 0;
        }
    }

    if (m_pDialogSound != 0) {
        m_pDialogSound->Stop(0);
        CExoStreamingSoundSource* pDialogSound = m_pDialogSound;
        if (pDialogSound != 0) {
            pDialogSound->~CExoStreamingSoundSource();
            operator delete(pDialogSound);
        }
        m_pDialogSound = 0;
    }
}

void CGuiInGame::ShutDown()
{
    if (m_bInitialized == 0) return;

    m_pGuiManager = 0;

    if (m_pBarkBubble != 0)                  { delete m_pBarkBubble; m_pBarkBubble = 0; }
    if (m_pExamineBoxPanel != 0)             { delete m_pExamineBoxPanel;               m_pExamineBoxPanel = 0; }
    if (m_pContainerGuiPanel != 0)           { delete m_pContainerGuiPanel;           m_pContainerGuiPanel = 0; }
    if (m_pItemCreatePanel != 0)             { delete m_pItemCreatePanel; m_pItemCreatePanel = 0; }
    if (m_pLoadModuleDebugPanel != 0)        { delete m_pLoadModuleDebugPanel; m_pLoadModuleDebugPanel = 0; }
    if (m_pPowersFeatsSkillsDebugPanel != 0) { delete m_pPowersFeatsSkillsDebugPanel; m_pPowersFeatsSkillsDebugPanel = 0; }
    if (m_pSubItemCreateMenuPanel != 0)      { delete m_pSubItemCreateMenuPanel; m_pSubItemCreateMenuPanel = 0; }
    if (m_pInGameMenu != 0)                  { delete m_pInGameMenu; m_pInGameMenu = 0; }
    if (m_pTopLetterbox != 0)                { delete m_pTopLetterbox; m_pTopLetterbox = 0; }
    if (m_pBottomLetterbox != 0)     { delete m_pBottomLetterbox; m_pBottomLetterbox = 0; }
    if (m_pDialogLetterbox != 0)             { delete m_pDialogLetterbox; m_pDialogLetterbox = 0; }
    if (m_pDialogCinematicPanel != 0)           { delete m_pDialogCinematicPanel;          m_pDialogCinematicPanel = 0; }
    if (m_pDialogComputerPanel != 0)         { delete m_pDialogComputerPanel; m_pDialogComputerPanel = 0; }
    if (m_pDialogComputerCameraPanel != 0)                     { delete m_pDialogComputerCameraPanel; m_pDialogComputerCameraPanel = 0; }
    if (m_pGlobalFade != 0)                  { delete m_pGlobalFade; m_pGlobalFade = 0; }
    if (m_pMessagesPanel != 0)                     { delete m_pMessagesPanel; m_pMessagesPanel = 0; }
    if (m_pPauseGuiPanel != 0)               { delete m_pPauseGuiPanel; m_pPauseGuiPanel = 0; }
    if (m_pSoloModePanel != 0)               { delete m_pSoloModePanel; m_pSoloModePanel = 0; }
    if (m_pAreaTransitionPanel != 0)         { delete m_pAreaTransitionPanel; m_pAreaTransitionPanel = 0; }
    if (m_pStatusSummary != 0)               { delete m_pStatusSummary; m_pStatusSummary = 0; }
    if (m_pStatusFallbackCache != 0)         { operator delete(m_pStatusFallbackCache);                          m_pStatusFallbackCache = 0; }
    if (m_pStorePanel != 0)                  { delete m_pStorePanel; m_pStorePanel = 0; }
    if (m_pMainInterface != 0)               { delete m_pMainInterface; m_pMainInterface = 0; }

#if !KOTOR1  // K1 has no CSWGuiInGameEquip::ShutdownCleanup
    static_cast<CSWGuiInGameEquip*>(m_apScreenPanels[SCREEN_EQUIP])->ShutdownCleanup();
#endif

    if (m_apScreenPanels[0] != 0) { delete m_apScreenPanels[0]; m_apScreenPanels[0] = 0; }
    if (m_apScreenPanels[1] != 0) { delete m_apScreenPanels[1]; m_apScreenPanels[1] = 0; }
    if (m_apScreenPanels[2] != 0) { delete m_apScreenPanels[2]; m_apScreenPanels[2] = 0; }
    if (m_apScreenPanels[3] != 0) { delete m_apScreenPanels[3]; m_apScreenPanels[3] = 0; }
    if (m_apScreenPanels[4] != 0) { delete m_apScreenPanels[4]; m_apScreenPanels[4] = 0; }
    if (m_apScreenPanels[5] != 0) { delete m_apScreenPanels[5]; m_apScreenPanels[5] = 0; }
    if (m_apScreenPanels[6] != 0) { delete m_apScreenPanels[6]; m_apScreenPanels[6] = 0; }
    if (m_apScreenPanels[7] != 0) { delete m_apScreenPanels[7]; m_apScreenPanels[7] = 0; }

    if (m_pGalaxyMapPanel != 0) { delete m_pGalaxyMapPanel; m_pGalaxyMapPanel = 0; }

    if (m_paStuntModelObjIds != 0) {
        operator delete[](m_paStuntModelObjIds);
        m_paStuntModelObjIds = 0;
    }
    if (m_paStuntModelResRefs != 0) {
        operator delete[](m_paStuntModelResRefs);
        m_paStuntModelResRefs = 0;
    }
    if (m_paDialogAnimData != 0) {
        operator delete[](m_paDialogAnimData);
        m_paDialogAnimData = 0;
    }
    if (m_paDialogAnimFlags != 0) {
        operator delete[](m_paDialogAnimFlags);
        m_paDialogAnimFlags = 0;
    }
    if (m_paLastDialogAnimData != 0) {
        if (m_paDialogAnimData != 0) {
            operator delete[](m_paDialogAnimData);
        }
        m_paDialogAnimData = 0;
    }
    if (m_paLastDialogAnimFlags != 0) {
        operator delete[](m_paLastDialogAnimFlags);
        m_paLastDialogAnimFlags = 0;
    }

    m_stuntModelCount = 0;
    ClearRepliesData();
    m_dialogAnimCount = 0;
    if (m_paDialogAnimData != 0) {
        operator delete[](m_paDialogAnimData);
        m_paDialogAnimData = 0;
    }
    if (m_paDialogAnimFlags != 0) {
        operator delete[](m_paDialogAnimFlags);
        m_paDialogAnimFlags = 0;
    }
    m_lastDialogAnimCount = 0;
    if (m_paLastDialogAnimData != 0) {
        operator delete[](m_paLastDialogAnimData);
        m_paLastDialogAnimData = 0;
    }
    if (m_paLastDialogAnimFlags != 0) {
        operator delete[](m_paLastDialogAnimFlags);
        m_paLastDialogAnimFlags = 0;
    }

#if !KOTOR1  // K1 has no Ios_minigamePaused
    Ios_minigamePaused = false;
#endif
    m_bPaused = 0;
    m_bPauseGuiShown = 0;
    m_bInitialized = 0;
}

void CGuiInGame::ShutDownGlobal()
{
    // Six popups, each released through its own destructor.
    if (m_pSoloModeQueryBox != 0)             { delete m_pSoloModeQueryBox; m_pSoloModeQueryBox = 0; }
#if !KOTOR1  // K1: four popups only
    if (m_pDatapadMessageBox != 0)             { delete m_pDatapadMessageBox; m_pDatapadMessageBox = 0; }
    if (m_pDeathMessagePanel != 0)    { delete m_pDeathMessagePanel; m_pDeathMessagePanel = 0; }
#endif
    if (m_pSkillInfoBoxPanel != 0)    { delete m_pSkillInfoBoxPanel; m_pSkillInfoBoxPanel = 0; }
    if (m_pTutorialBox != 0)          { delete m_pTutorialBox; m_pTutorialBox = 0; }
    if (m_pControllerPausePanel != 0) { delete m_pControllerPausePanel; m_pControllerPausePanel = 0; }

    m_pGuiManager = 0;
}

int CGuiInGame::Initialize()
{
    if (m_bInitialized != 0)
        return m_bInitialized;

    CSWGuiManager* pGuiManager = m_pGuiManager;
    if (pGuiManager == 0) {
        pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        m_pGuiManager = pGuiManager;
    }

    m_pItemCreatePanel = new CSWGuiCreateItemMenu(pGuiManager);
    m_pLoadModuleDebugPanel = new CSWGuiLoadModuleDebugMenu(m_pGuiManager);
    m_pPowersFeatsSkillsDebugPanel = new CSWGuiPowersFeatsSkillsDebugMenu(m_pGuiManager);

    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    pClientExoApp->SetLoadBarProgress(static_cast<int>(static_cast<float>(pClientExoApp->GetLoadStep(3)) * 0.2f +
                                            static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(1)) +
                                            static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(0))),
                                      1);
    UpdateScreen(0.033333335f, 0, 0);

    m_pSubItemCreateMenuPanel = new CSWGuiCreateItemSubMenu(m_pGuiManager);
    m_pExamineBoxPanel = new CSWGuiExamine(m_pGuiManager);
    m_pBarkBubble = new CSWGuiBarkBubble(m_pGuiManager);
    m_pContainerGuiPanel = new CSWGuiContainer(m_pGuiManager);
    m_pDialogCinematicPanel = new CSWGuiDialogCinematic(m_pGuiManager);
    if (g_bCreateAllGUIs != 0) {
        m_pDialogComputerPanel = new CSWGuiDialogComputer(m_pGuiManager);
    }
    m_pDialogComputerCameraPanel = new CSWGuiDialogComputerCamera(m_pGuiManager);
    m_pTopLetterbox = new CSWGuiDialogLetterbox(m_pGuiManager);

    if (m_pSoloModeQueryBox == 0) {
        m_pSoloModeQueryBox = new CSWGuiMessageBox(m_pGuiManager);
    }
#if !KOTOR1  // K1 has no datapad-style box
    if (m_pDatapadMessageBox == 0) {
        m_pDatapadMessageBox = new CSWGuiMessageBox(m_pGuiManager, 1);
    }
#endif
    if (m_pSkillInfoBoxPanel == 0) {
        m_pSkillInfoBoxPanel = new CSWGuiSkillInfoBox(m_pGuiManager);
    }
    if (m_pTutorialBox == 0) {
        m_pTutorialBox = new CSWGuiTutorialBox(m_pGuiManager);
    }
    if (m_pControllerPausePanel == 0) {
        m_pControllerPausePanel = new CSWGuiControllerLossBox(m_pGuiManager);
    }

    pClientExoApp = g_pAppManager->m_pClientExoApp;
    pClientExoApp->SetLoadBarProgress(static_cast<int>(static_cast<float>(pClientExoApp->GetLoadStep(3)) * 0.4f +
                                            static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(1)) +
                                            static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(0))),
                                      1);
    UpdateScreen(0.033333335f, 0, 0);

    m_pBottomLetterbox = new CSWGuiDialogLetterbox(m_pGuiManager);
    m_pDialogLetterbox = new CSWGuiDialogLetterbox(m_pGuiManager);
    m_pGlobalFade = new CSWGuiFade(m_pGuiManager);
    m_pInGameMenu = new CSWGuiInGameMenu(m_pGuiManager);
    m_pPauseGuiPanel = new CSWGuiInGamePause(m_pGuiManager);
    m_pSoloModePanel = new CSWGuiInGameSoloModeQuery(m_pGuiManager);
    m_pAreaTransitionPanel = static_cast<CSWGuiPanel*>(new CSWGuiInGameAreaTransition(m_pGuiManager));
#if KOTOR1
    // K1: no fade-until-script latch; the party selection fills a different slot.
    if (g_bCreateAllGUIs != 0) {
        m_pMessagesPanel = static_cast<CSWGuiPanel*>(new CSWGuiPartySelection(m_pGuiManager));
        m_pStorePanel = new CSWGuiStore(m_pGuiManager);
        m_apScreenPanels[SCREEN_EQUIP] = static_cast<CSWGuiPanel*>(new CSWGuiInGameEquip(m_pGuiManager));
        m_apScreenPanels[SCREEN_INVENTORY] = new CSWGuiInGameInventory(m_pGuiManager);
    }
#else
    if (m_bFadeUntilScript != 0) {
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pGlobalFade), 0, 1);
        static_cast<CSWGuiPanel*>(m_pGlobalFade)->SetVisible(1);
        Vector pos;
        m_pGlobalFade->SetFade(0, 0.0f, 0.1f, pos);
    }
    if (g_bCreateAllGUIs != 0) {
        CSWGuiInGameMessages* pMessages = new CSWGuiInGameMessages(m_pGuiManager);
        m_pMessagesPanel = static_cast<CSWGuiPanel*>(pMessages);
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        static_cast<CSWGuiPanel*>(pMessages)->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif

        m_pStorePanel = new CSWGuiStore(m_pGuiManager);

        CSWGuiInGameEquip* pEquip = new CSWGuiInGameEquip(m_pGuiManager);
        pGuiManager = m_pGuiManager;
        m_apScreenPanels[SCREEN_EQUIP] = static_cast<CSWGuiPanel*>(pEquip);
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        static_cast<CSWGuiPanel*>(pEquip)->AddExtraBorderPanel(pGuiManager, CExoString("bg_replacement"));
#endif

        CSWGuiInGameInventory* pInventory = new CSWGuiInGameInventory(m_pGuiManager);
        pGuiManager = m_pGuiManager;
        m_apScreenPanels[SCREEN_INVENTORY] = pInventory;
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        pInventory->AddExtraBorderPanel(pGuiManager, CExoString("bg_replacement"));
#endif
    }
#endif // KOTOR1

    pClientExoApp = g_pAppManager->m_pClientExoApp;
    pClientExoApp->SetLoadBarProgress(static_cast<int>(static_cast<float>(pClientExoApp->GetLoadStep(3)) * 0.6f +
                                            static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(1)) +
                                            static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(0))),
                                      1);
    UpdateScreen(0.033333335f, 0, 0);

    if (g_bCreateAllGUIs != 0) {
        CSWGuiInGameCharacter* pCharacter = new CSWGuiInGameCharacter(m_pGuiManager);
        m_apScreenPanels[SCREEN_CHARACTER] = pCharacter;
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        pCharacter->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
    }

    m_pStatusSummary = new CSWGuiStatusSummary(m_pGuiManager);
    if (m_pStatusFallbackCache != 0)
        m_pStatusSummary->SetSummary(*m_pStatusFallbackCache);
    m_pMainInterface = new CSWGuiMainInterface(m_pGuiManager);

    pClientExoApp = g_pAppManager->m_pClientExoApp;
    pClientExoApp->SetLoadBarProgress(static_cast<int>(static_cast<float>(pClientExoApp->GetLoadStep(3)) * 0.8f +
                                            static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(1)) +
                                            static_cast<float>(g_pAppManager->m_pClientExoApp->GetLoadStep(0))),
                                      1);
    UpdateScreen(0.033333335f, 0, 0);

    if (g_bCreateAllGUIs != 0) {
        CSWGuiInGameMap* pMap = new CSWGuiInGameMap(m_pGuiManager);
        m_apScreenPanels[SCREEN_MAP] = pMap;
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        pMap->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif

        CSWGuiInGameAbilities* pAbilities = new CSWGuiInGameAbilities(m_pGuiManager);
        pGuiManager = m_pGuiManager;
        m_apScreenPanels[SCREEN_ABILITIES] = pAbilities;
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        pAbilities->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif

        CSWGuiInGameJournal* pJournal = new CSWGuiInGameJournal(m_pGuiManager);
        pGuiManager = m_pGuiManager;
        m_apScreenPanels[SCREEN_JOURNAL] = pJournal;
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        pJournal->AddExtraBorderPanel(pGuiManager, CExoString("bg_replacement"));
#endif

        CSWGuiInGameOptions* pOptions = new CSWGuiInGameOptions(m_pGuiManager);
        pGuiManager = m_pGuiManager;
        m_apScreenPanels[SCREEN_OPTIONS] = pOptions;
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        pOptions->AddExtraBorderPanel(pGuiManager, CExoString("bg_replacement"));
#endif

#if KOTOR1
        m_apScreenPanels[SCREEN_MESSAGES] = static_cast<CSWGuiPanel*>(new CSWGuiInGameMessages(m_pGuiManager));
#else
        CSWGuiPartySelection* pPartySelection = new CSWGuiPartySelection(m_pGuiManager);
        pGuiManager = m_pGuiManager;
        m_apScreenPanels[SCREEN_PARTY_SELECTION] = pPartySelection;
        pPartySelection->AddExtraBorderPanel(pGuiManager, CExoString("bg_replacement"));
#endif

        m_pGalaxyMapPanel = new CSWGuiInGameGalaxyMap(m_pGuiManager);
    }

    pClientExoApp = g_pAppManager->m_pClientExoApp;
    pClientExoApp->SetLoadBarProgress(pClientExoApp->GetLoadStep(3) +
                                          g_pAppManager->m_pClientExoApp->GetLoadStep(1) +
                                          g_pAppManager->m_pClientExoApp->GetLoadStep(0),
                                      1);
    UpdateScreen(0.033333335f, 0, 0);

    m_bInitialized = 1;
    m_pActiveDialogPanel = m_pDialogCinematicPanel;
    return 1;
}

void CGuiInGame::InitializeGlobal()
{
    CSWGuiManager* pGuiManager = m_pGuiManager;
    if (pGuiManager == 0) {
        pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        m_pGuiManager = pGuiManager;
    }

    m_pSoloModeQueryBox = new CSWGuiMessageBox(pGuiManager);
#if !KOTOR1  // K1 has no datapad-style box
    m_pDatapadMessageBox = new CSWGuiMessageBox(m_pGuiManager, 1);
#endif
    m_pSkillInfoBoxPanel = new CSWGuiSkillInfoBox(m_pGuiManager);
    m_pTutorialBox = new CSWGuiTutorialBox(m_pGuiManager);
    m_pControllerPausePanel = new CSWGuiControllerLossBox(m_pGuiManager);
}

void CGuiInGame::ToggleOff()
{
    if (m_toggledOff == 0) {
        m_toggledOff = 1;
    }
}

void CGuiInGame::SetLevelUpMode(int mode)
{
    if (m_levelUpMode != mode) {
        m_levelUpMode = mode;
    }
}

int CGuiInGame::ShowLevelUpGUI(int mode)
{
    if (m_bInitialized == 0) return false;

    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pInGameGui->m_levelUpMode == 0) return false;

    if (mode == 0) {
        if (g_pAppManager->m_pClientExoApp->GetCharacterChangeInProgress() != 0) return false;
    } else {
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
#endif
    }

    CSWGuiInGameCharacter* pCharPanel = static_cast<CSWGuiInGameCharacter*>(m_apScreenPanels[SCREEN_CHARACTER]);
    if (pCharPanel == 0) {
        pCharPanel = new CSWGuiInGameCharacter(m_pGuiManager);
        m_apScreenPanels[SCREEN_CHARACTER] = pCharPanel;
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        pCharPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pCharPanel = static_cast<CSWGuiInGameCharacter*>(m_apScreenPanels[SCREEN_CHARACTER]);
    }
    pCharPanel->ShowLevelUpGUI(mode);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    return true;
}

void CGuiInGame::IncrementStartConversation()
{
    m_startConversationCount = m_startConversationCount + 1;
}

void CGuiInGame::ShowDialogEntry(const CExoString& msg, unsigned long long a, unsigned long long b)
{
    K2_UNUSED(b);
    if (m_repliesCount < 2) {
        g_pAppManager->m_pClientExoApp->m_pInternal->HideMouse(CClientExoAppInternal::MOUSEHIDE_DIALOG_CUTSCENE);
#if KOTOR1  // K1: input is held off briefly after the mouse is hidden
        IosTemporaryDisableInteraction(60);
#endif
    }
    if (m_dialogInputActive == 0) {
        m_dialogInputActive = 1;
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_DIALOG, 1);
        g_pAppManager->m_pClientExoApp->ResetDriveAcceleration();
        m_pBarkBubble->StopBark(1);
    } else {
        static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->Reset();
    }
    static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->SetOwner(a);
    if (!m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pActiveDialogPanel))) {
        CSWGuiPanel* pPanel = static_cast<CSWGuiPanel*>(m_pActiveDialogPanel);
        int zone;
        if (m_pActiveDialogPanel == m_pDialogCinematicPanel) {
            if (!bDisableLetterbox) {
                m_pDialogLetterbox->SetMiddle();
                m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pDialogLetterbox), 0, 1);
                m_pTopLetterbox->SetTop();
                m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pTopLetterbox), 0, 1);
                m_pBottomLetterbox->SetBottom();
                m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pBottomLetterbox), 0, 1);
                pPanel = static_cast<CSWGuiPanel*>(m_pActiveDialogPanel);
            }
            zone = 0;
        } else {
            zone = (m_pActiveDialogPanel == m_pDialogComputerPanel) ? 2 : 0;
        }
        m_pGuiManager->AddPanel(pPanel, zone, 1);
        SetSWGuiStatus(SWGUI_COVER, 1);
    }
    static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->SetDialogMessage(msg);
}

void CGuiInGame::SetSWGuiStatus(int status, int notify)
{
    int newState;
    switch (status) {
    case SWGUI_TOGGLE_ON:
        if (m_swGuiToggleState != SWGUI_STATE_COVERED) goto showHud;
        newState = SWGUI_STATE_COVERED;
        goto checkDialogGate;
    case SWGUI_TOGGLE_OFF:
        newState = SWGUI_STATE_TOGGLED_OFF;
        break;
    case SWGUI_COVER:
        if (m_swGuiToggleState == SWGUI_STATE_TOGGLED_OFF) {
stayToggledOff:
            newState = SWGUI_STATE_TOGGLED_OFF;
            goto checkDialogGate;
        }
        newState = SWGUI_STATE_COVERED;
        break;
    case SWGUI_UNCOVER:
        if (m_swGuiToggleState == SWGUI_STATE_TOGGLED_OFF) goto stayToggledOff;
showHud:
        newState = SWGUI_STATE_SHOWN;
        break;
    default:
        return;
    }
    m_swGuiToggleState = newState;
checkDialogGate:
    if (m_bInitialized == 0) return;
    if (m_pGuiManager == 0) return;
    {
        int mainExists = m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pMainInterface));
        if (newState == SWGUI_STATE_SHOWN) {
            if (mainExists == 0) {
                m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pMainInterface), 4, 1);
                m_pGuiManager->SendPanelToBack(static_cast<CSWGuiPanel*>(m_pMainInterface));
            }
            if (m_bPauseGuiShown != 0 && m_pPauseGuiPanel != 0 &&
                m_pGuiManager->PanelExists(m_pPauseGuiPanel) == 0) {
                m_pGuiManager->AddPanel(m_pPauseGuiPanel, 4, 1);
                m_pGuiManager->SendPanelToBack(m_pPauseGuiPanel);
            }
        } else {
            if (mainExists != 0) {
                m_pGuiManager->RemovePanel(m_pMainInterface);
            }
            if (notify != 0 && m_pGuiManager->PanelExists(m_pPauseGuiPanel) != 0) {
                unsigned short flags = m_pPauseGuiPanel->m_stateFlags;
                unsigned short cleared = flags & ~PANEL_REMOVE_DEFERRED;
                unsigned short removeMode = flags & PANEL_REMOVE_MASK;
                unsigned short removeRequest = (flags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
                m_pPauseGuiPanel->m_stateFlags = (removeMode == PANEL_REMOVE_AND_DELETE) ? cleared : removeRequest;
            }
        }
    }
}

void CGuiInGame::ShowDialogReplies(unsigned int a, unsigned int b)
{
    if ((m_repliesCount > 1) ||
        ((m_repliesCount == 1) && (*m_paReplyText != ""))) {
        g_pAppManager->m_pClientExoApp->m_pInternal->ShowMouse(CClientExoAppInternal::MOUSEHIDE_ALL);
    }
    if (m_pActiveDialogPanel == 0) {
        return;
    }
    unsigned long long playerCreatureId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerCreatureId);
    unsigned long long speakerObjId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_gameObjectId);
    if ((m_repliesCount == 1) && (*m_paReplyText == "")) {
        static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->SetReplies(m_repliesCount, m_paReplyText, m_paReplyLinkIndices, a, 0);
        static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->m_nCurrentEntry = b;
        static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->SetDialogMessage(*m_paReplyText);
#if KOTOR1  // K1: IsEmpty is out of line
        unsigned int hasText = !m_paReplyText->IsEmpty();
#else
        unsigned int hasText = 0;
        if (m_paReplyText->m_sString != 0) {
            hasText = (m_paReplyText->m_sString[0] != 0);
        }
#endif
        m_dialogSelectionA = 0;
        m_dialogSelectionB = hasText;
        m_dialogSelectionActive = 1;
        m_dialogSelectionC = 0;
        UpdateDialog();
        static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->SetActiveControl(0, 0);
        return;
    }
    static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->SetReplies(m_repliesCount, m_paReplyText, m_paReplyLinkIndices, a, 1);
    static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->m_nCurrentEntry = b;
    UnlockDialogParticipantOrientations();
    CSWCCreature* pSpeakerCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(speakerObjId);
    bool bKeepSpeaker = (pSpeakerCreature != 0) && (pSpeakerCreature->m_bInParty != 0);
    m_oidPrevDialogParticipantB = m_dialogParticipantIdB;
    m_dialogParticipantIdB = m_dialogParticipantIdA;
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CSWCCreature* pPartyLeader = pParty->GetCharacter(0);
    if ((!bKeepSpeaker || (m_bConversationUseLeader != 0)) && (pPartyLeader != 0)) {
        speakerObjId = pPartyLeader->m_objectId;
    }
    unsigned long long prevParticipantA = m_dialogParticipantIdA;
    m_dialogParticipantIdA = speakerObjId;
    m_oidPrevDialogParticipantA = prevParticipantA;
    if ((m_pActiveDialogPanel != 0) && (m_pActiveDialogPanel == m_pDialogCinematicPanel)) {
        InitializeShotCamera();
        UpdateShotCamera(m_paReplyCameraAngles[0], m_paReplyCameraIds[0], m_paReplyHeightOffsets[0],
                          m_paReplyTargetHeightOffsets[0], m_paReplyWaitFlags[0], m_paReplyCamVidEffects[0]);
    }
}

void CGuiInGame::SetDialogSelection(int a, int b, int c)
{
    m_dialogSelectionA = a;
    m_dialogSelectionB = b;
    m_dialogSelectionC = c;
    m_dialogSelectionActive = 1;
}

void CGuiInGame::UpdateDialog()
{
    if (m_nGlobalDialogState == 0) {
        return;
    }
    if (m_bDeferredFOVCall == 1) {
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule->m_pCamera != 0) {
            pModule->m_pCamera->SetViewAngle(m_deferredFOV);
            m_bDeferredFOVCall = 0;
        }
    }
    if ((m_dialogSelectionActive != 0) && (m_paReplyListenerIds != 0)) {
        int index = m_dialogSelectionA;
        unsigned long long speakerObjId = m_oidDialogPlayer;
        unsigned long long replyTargetId = m_paReplyListenerIds[index];
        unsigned long long objIdA = m_dialogParticipantIdA;
        if (replyTargetId != OBJECT_INVALID) {
            objIdA = replyTargetId;
        }
        CGameObject* pGameObjA = g_pAppManager->m_pClientExoApp->GetGameObject(objIdA);
        if ((pGameObjA != 0) && (pGameObjA->AsSWCObject() != 0) &&
            ((pGameObjA = g_pAppManager->m_pClientExoApp->GetGameObject(speakerObjId)), pGameObjA != 0) &&
            (pGameObjA->AsSWCObject() != 0)) {
            if (0 < static_cast<int>(m_paReplyFlags[index])) {
                int i = 0;
                do {
                    CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(
                        static_cast<unsigned long long*>(m_paReplyAnimObjectIds[index])[i]);
                    if (pGameObj == 0) {
                        return;
                    }
                    if (pGameObj->AsSWCObject() == 0) {
                        return;
                    }
                    i = i + 1;
                } while (i < static_cast<int>(m_paReplyFlags[index]));
            }
            AppendToDialogBuffer(m_paReplyText[m_dialogSelectionA], speakerObjId);
            int replySelection = m_dialogSelectionA;
            if (m_dialogSelectionC == 0) {
                HandleDialogSelection(replySelection, m_dialogSelectionB);
                return;
            }
            unsigned int replyIndex;
            if (replySelection < 0) {
                replyIndex = 0xffffffff;
            } else {
                replyIndex = static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->GetReplyIndex(replySelection);
                replySelection = m_dialogSelectionA;
            }
            int bAbort = (replySelection == -3);
            unsigned long long playerCreatureId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
            CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerCreatureId);
            unsigned long long ownerServerId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->m_ownerId);
            CGameObject* pServerObj = g_pAppManager->m_pServerExoApp->GetGameObject(ownerServerId);
            if (m_bAnimatedCut == 0) {
                unsigned int animCount = m_paReplyFlags[index];
                if (InitializeDialogAnimationData(animCount)) {
                    unsigned long long* paSrcData = static_cast<unsigned long long*>(m_paReplyAnimObjectIds[index]);
                    unsigned short* paSrcFlags = static_cast<unsigned short*>(m_paReplyAnimations[index]);
                    for (int j = 0; j < static_cast<int>(animCount); j++) {
                        SetDialogAnimationData(j, paSrcData[j], paSrcFlags[j]);
                    }
                }
                SetDialogAnimations();
            }
            if ((pServerObj != 0) && (pServerObj->AsSWSObject() != 0)) {
                CSWSObject* pServerSWSObj = pServerObj->AsSWSObject();
                pServerSWSObj->ReplyDialog(pPlayer->m_nPlayerId, replyIndex, bAbort,
                                            static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->m_nCurrentEntry, 1);
            }
            m_dialogSelectionActive = 0;
        }
    }
}

int CGuiInGame::IsCameraDialog()
{
    if (m_pActiveDialogPanel == 0) {
        goto notEqual;
    }
    if (m_pActiveDialogPanel == m_pDialogCinematicPanel) {
        return true;
    }
notEqual:
    return false;
}

void CGuiInGame::UnToggleOff()
{
    if (m_toggledOff != 0) {
        m_toggledOff = 0;
    }
}

void* CGuiInGame::GetMessageBuffer()
{
    return m_pMessageBuffer;
}

int CGuiInGame::GetMessageBufferSize()
{
    return m_messageBufferSize;
}

void CGuiInGame::SetDialogFade(int a, float b, float c, const Vector& d)
{
    if (m_pDialogLetterbox != 0) {
        m_pDialogLetterbox->SetFade(a, b, c, d);
    }
}

void CGuiInGame::PauseBarkBubble()
{
    m_pBarkBubble->Pause();
}

void CGuiInGame::ResumeBarkBubble()
{
    m_pBarkBubble->Resume();
}

int CGuiInGame::IsCameraAnimationPlayingInDialog()
{
    return static_cast<unsigned>(m_nCameraAnimation - STUNT_ANIM_FIREFORGET_FIRST) < STUNT_ANIM_SPAN;
}

int CGuiInGame::IsInDialog(unsigned long long objId)
{
    if (m_dialogParticipantIdA == objId) {
        return true;
    }
    return m_dialogParticipantIdB == objId;
}

int CGuiInGame::InitializePlaceableCameras(int count)
{
#if KOTOR1  // K1: 50 placeable cameras
    if (count > 50) {
#else
    if (count > 70) {
#endif
        return 0;
    }
    m_placeableCameraCount = count;
    return 1;
}

void* CGuiInGame::GetPlaceableCamera(int index)
{
    void* addr = &m_placeableCameras[index];
    return (index <= m_placeableCameraCount) ? addr : 0;
}

int CGuiInGame::HandleDialogSound(unsigned long long objId, const CResRef& resRef)
{
    CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogParticipantIdA);
    if (pGameObj != 0 && pGameObj->AsSWCObject() != 0) {
        CSWCObject* pObj = static_cast<CSWCObject*>(pGameObj->AsSWCObject());
        int bValid = resRef.IsValid();
        if (pObj != 0 && bValid != 0) {
            if (m_pDialogSound != 0) {
                m_pDialogSound->Stop(0);
                CExoStreamingSoundSource* pDialogSound = m_pDialogSound;
                if (pDialogSound != 0) {
                    pDialogSound->~CExoStreamingSoundSource();
                    operator delete(pDialogSound);
                }
            }
            m_pDialogSound = new CExoStreamingSoundSource(1);
            m_pDialogSound->SetPriorityGroup(PRIORITY_GROUP_NON_PLAYER_CHAT);
            if (m_pDialogSound->Play(resRef, 8, 1, 0, 0) == 0) {
#if !KOTOR1  // K1 has no VO debug trace
                if (g_bVODebugEnabled) {
                    char buf[20];
                    resRef.CopyToString(buf);
                    g_pExoSound->GetSoundMode();
                }
#endif
                if (m_pDialogSound->Play(resRef, 4, 1, 0, 0) == 0) {
#if !KOTOR1
                    if (g_bVODebugEnabled) {
                        char buf[20];
                        resRef.CopyToString(buf);
                        g_pExoSound->GetSoundMode();
                    }
#endif
                    pObj->LipSync(resRef);
                    return 0;
                }
            }
            m_dialogLipSyncObjId = objId;
            m_dialogLipSyncResRef = resRef;
        }
        K2_ASSUME(pObj != 0);   // the object's AsSWCObject() was tested non-null above
        m_oidDialogSoundSpeaker = pObj->m_objectId;
        return 1;
    }
    return 0;
}

int CGuiInGame::CanPlayDialogAnimations(unsigned long long objId)
{
    if (objId == OBJECT_INVALID) return 1;

    unsigned long long serverId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(objId);
    CGameObject* pServerObj = g_pAppManager->m_pServerExoApp->GetGameObject(serverId);
    if (pServerObj == 0) return 0;

    if (pServerObj->AsSWSCreature() == 0) return 1;

    CSWSCreature* pCreature = pServerObj->AsSWSCreature();
    if (pCreature->IsDebilitated(0) != 0) return 0;

    if (pCreature->GetDead() != 0) return 0;

#if KOTOR1  // K1: GetDeadTemp instead of the in-party hit-point test
    if (pCreature->GetDeadTemp() != 0) return 0;
    return 1;
#else
    if (pCreature->m_bInParty == 0) return 1;

    if (pCreature->GetCurrentHitPoints(0) >= 1) return 1;
    return 0;
#endif
}

// Resets the previous frame's animated objects to idle.
void CGuiInGame::ResetDialogAnimations()
{
    for (unsigned int i = 0; i < m_lastDialogAnimCount; i++) {
        unsigned long long objId = m_paLastDialogAnimData[i];
        CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
        if (pGameObj == 0) continue;

        if (pGameObj->AsSWCCreature() != 0) {
            CSWCCreature* pCreature = static_cast<CSWCCreature*>(pGameObj->AsSWCCreature());
            int bCanPlay = CanPlayDialogAnimations(pCreature->m_objectId);
            if (pCreature != 0 && bCanPlay != 0) {
                pCreature->m_bPlayingCutsceneAnimation = 0;
                if (static_cast<CSWCObject*>(pCreature)->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                    static_cast<CSWCObject*>(pCreature)->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                }
                static_cast<CSWCObject*>(pCreature)->m_pModel->SetInCutscene(0);
            }
        }

        if (pGameObj->AsSWCPlaceable() != 0) {
            CSWCPlaceable* pPlaceable = static_cast<CSWCPlaceable*>(pGameObj->AsSWCPlaceable());
            CSWSPlaceable* pServerPlaceable = pPlaceable->GetServerPlaceable();
            if (pPlaceable != 0) {
                pPlaceable->m_bPlayingCutsceneAnimation = 0;
                static_cast<CSWCObject*>(pPlaceable)->SetAnimation(static_cast<unsigned short>(pServerPlaceable->m_nAnimation), 0, 0);
                static_cast<CSWCObject*>(pPlaceable)->m_pModel->SetInCutscene(0);
            }
        }
    }
}

// Resets both dialog participants' talk animations.
void CGuiInGame::ResetTalkAnimations()
{
    CGameObject* pObjA = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidPrevDialogParticipantA);
    if (pObjA != 0 && pObjA->AsSWCObject() != 0) {
#if KOTOR1  // K1: no door exemption
        if (pObjA->AsSWCCreature() != 0) {
            CSWCObject* pCreature = static_cast<CSWCObject*>(pObjA->AsSWCCreature());
            if (!CanPlayDialogAnimations(pCreature->m_objectId)) goto tailA;
        }
        if (static_cast<CSWCObject*>(pObjA->AsSWCObject())->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
            static_cast<CSWCObject*>(pObjA->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
        }
#else
        bool needsReset = false;
        if (pObjA->AsSWCCreature() != 0) {
            CSWCObject* pCreature = static_cast<CSWCObject*>(pObjA->AsSWCCreature());
            if (!CanPlayDialogAnimations(pCreature->m_objectId)) goto tailA;
            CSWCObject* pObj = static_cast<CSWCObject*>(pObjA->AsSWCObject());
            needsReset = pObj->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE;
        } else {
            CSWCObject* pObj = static_cast<CSWCObject*>(pObjA->AsSWCObject());
            if (pObj->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                needsReset = pObjA->AsSWCDoor() == 0;
            }
        }
        if (needsReset) {
            static_cast<CSWCObject*>(pObjA->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
        }
#endif
        static_cast<CSWCObject*>(pObjA->AsSWCObject())->m_pModel->SetInCutscene(0);
    }
tailA:;

    CGameObject* pObjB = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidPrevDialogParticipantB);
    if (pObjB != 0 && pObjB->AsSWCObject() != 0) {
#if KOTOR1  // K1: no door exemption
        if (pObjB->AsSWCCreature() != 0) {
            CSWCObject* pCreature = static_cast<CSWCObject*>(pObjB->AsSWCCreature());
            if (!CanPlayDialogAnimations(pCreature->m_objectId)) goto tailB;
        }
        if (static_cast<CSWCObject*>(pObjB->AsSWCObject())->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
            static_cast<CSWCObject*>(pObjB->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
        }
#else
        bool needsReset = false;
        if (pObjB->AsSWCCreature() != 0) {
            CSWCObject* pCreature = static_cast<CSWCObject*>(pObjB->AsSWCCreature());
            if (!CanPlayDialogAnimations(pCreature->m_objectId)) goto tailB;
            CSWCObject* pObj = static_cast<CSWCObject*>(pObjB->AsSWCObject());
            needsReset = pObj->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE;
        } else {
            CSWCObject* pObj = static_cast<CSWCObject*>(pObjB->AsSWCObject());
            if (pObj->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                needsReset = pObjB->AsSWCDoor() == 0;
            }
        }
        if (needsReset) {
            static_cast<CSWCObject*>(pObjB->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
        }
#endif
        static_cast<CSWCObject*>(pObjB->AsSWCObject())->m_pModel->SetInCutscene(0);
    }
tailB:;
}

void CGuiInGame::SetDialogLipSync(unsigned long long objId, const CResRef& resRef)
{
    m_dialogLipSyncObjId = objId;
    m_dialogLipSyncResRef = resRef;
}

void CGuiInGame::UpdateDialogLipSync()
{
    if (m_dialogLipSyncObjId != OBJECT_INVALID && m_pDialogSound != 0 &&
        m_pDialogSound->IsPlaying() != 0 && m_pDialogSound->GetHasStartedStreaming() != 0) {
        CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogLipSyncObjId);
        if (pGameObj != 0 && pGameObj->AsSWCObject() != 0) {
            CSWCObject* pObj = static_cast<CSWCObject*>(pGameObj->AsSWCObject());
            pObj->LipSync(m_dialogLipSyncResRef);
            m_dialogLipSyncResRef = const_cast<char*>("");
            m_dialogLipSyncObjId = OBJECT_INVALID;
        }
    }
}

// Splits dialog text on "##" markers, one segment per call.
CExoString CGuiInGame::DialogNodeSpanningTextParse(const CExoString& text)
{
    const CExoString* src;

    if (m_dialogSpanRemaining >= 1) {
        int remaining = m_dialogSpanRemaining;
        int index = m_dialogSpanCount - remaining;
        m_dialogSpanRemaining = remaining - 1;
        src = &m_dialogSpanNodes[index];
        goto returnSrc;
    }

    int hashPos;
    {
        CExoString needle("##");
        hashPos = text.Find(needle, 0);
    }
    if (hashPos == -1) {
        src = &text;
        goto returnSrc;
    }

    m_dialogSpanCount = 0;

    char* origBuf;
#if KOTOR1  // K1: GetLength is out of line
    origBuf = new char[text.GetLength() + 1];
    strcpy(origBuf, text.CStr());
    origBuf[text.GetLength()] = '\0';
#else
    origBuf = text.m_sString ? new char[strlen(text.m_sString) + 1] : new char[1];
    strcpy(origBuf, text.CStr());
    origBuf[text.m_sString ? static_cast<int>(strlen(text.m_sString)) : 0] = '\0';
#endif

    char* buf;
    buf = origBuf;
    int i;
    i = 0;
    bool isDelim;

    for (;;) {
        char c = buf[i];
        if (c == '#') {
            if (buf[i + 1] == '#') {
                isDelim = true;
                goto checkStart;
            }
            i = i + 1;
            if (buf[i] != '\0') continue;
        } else if (c == '\0') {
            goto finish;
        } else {
            i = i + 1;
            if (buf[i] != '\0') continue;
        }
        isDelim = false;

    checkStart:
        if (i == 0 && buf[1] != '\0') {
            goto abandonParse;
        }

        {
            int segLen = isDelim ? (i - 1) : (i + 1);
            char* tmp = new char[segLen + 1];
            strncpy(tmp, buf, segLen);
            tmp[segLen] = '\0';
            CExoString token(tmp);
            delete[] tmp;

            if (m_dialogSpanCount == 8) {
                goto abandonParse;
            }
            m_dialogSpanNodes[m_dialogSpanCount++] = token;

#ifdef K2_FIX_ENGINE_BUGS
            if (!isDelim) goto finish;   // buf[i] is the terminator
#endif
            if (buf[i + 1] == '\0') {
                if (!isDelim) goto finish;
                continue;
            }
            if (buf[i + 2] == '\0') goto finish;
            buf = buf + i + 3;
            i = 0;
        }
    }

abandonParse:
    delete[] origBuf;
    src = &text;
    goto returnSrc;

finish:
    delete[] origBuf;
    m_dialogSpanRemaining = m_dialogSpanCount - 1;
    src = &m_dialogSpanNodes[0];

returnSrc:
    return CExoString(*src);
}

void CGuiInGame::SetLockObjectOrientationInDialog(unsigned long long objId, int lock)
{
    if (lock == 1) {
        CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
        if (pGameObj != 0 && pGameObj->AsSWCObject() != 0) {
            CExoLinkedListNode* pos = m_orientExclusionList.m_pInternal->m_pHead;
            void* pData = m_orientExclusionList.m_pInternal->m_pHead ?
                          m_orientExclusionList.m_pInternal->m_pHead->m_pData : 0;
            while (pos != 0) {
                if (*static_cast<unsigned long long*>(pData) == objId) return;
                pData = m_orientExclusionList.m_pInternal->GetNext(pos);
            }
            unsigned long long* pNew = static_cast<unsigned long long*>(operator new(8));
            *pNew = objId;
            m_orientExclusionList.m_pInternal->AddHead(pNew);
        }
    } else {
        CExoLinkedListNode* pos = m_orientExclusionList.m_pInternal->m_pHead;
        void* pData = m_orientExclusionList.m_pInternal->m_pHead ?
                      m_orientExclusionList.m_pInternal->m_pHead->m_pData : 0;
        if (pos != 0) {
            unsigned long long val = *static_cast<unsigned long long*>(pData);
            while (val != objId) {
                pData = m_orientExclusionList.m_pInternal->GetNext(pos);
                if (pos == 0) return;
                val = *static_cast<unsigned long long*>(pData);
            }
            m_orientExclusionList.m_pInternal->Remove(pos);
        }
    }
}

void CGuiInGame::SetLockHeadFollowInDialog(unsigned long long objId, int lock)
{
    if (lock == 1) {
        CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
        if (pGameObj != 0 && pGameObj->AsSWCObject() != 0) {
            CExoLinkedListNode* pos = m_headFollowExclusionList.m_pInternal->m_pHead;
            void* pData = m_headFollowExclusionList.m_pInternal->m_pHead ?
                          m_headFollowExclusionList.m_pInternal->m_pHead->m_pData : 0;
            while (pos != 0) {
                if (*static_cast<unsigned long long*>(pData) == objId) return;
                pData = m_headFollowExclusionList.m_pInternal->GetNext(pos);
            }
            unsigned long long* pNew = static_cast<unsigned long long*>(operator new(8));
            *pNew = objId;
            m_headFollowExclusionList.m_pInternal->AddHead(pNew);
            if (pGameObj->AsSWCCreature() != 0) {
                static_cast<CSWCCreature*>(pGameObj->AsSWCCreature())->LookAt(OBJECT_INVALID, 0.0f);
            }
        }
    } else {
        CExoLinkedListNode* pos = m_headFollowExclusionList.m_pInternal->m_pHead;
        void* pData = m_headFollowExclusionList.m_pInternal->m_pHead ?
                      m_headFollowExclusionList.m_pInternal->m_pHead->m_pData : 0;
        if (pos != 0) {
            unsigned long long val = *static_cast<unsigned long long*>(pData);
            while (val != objId) {
                pData = m_headFollowExclusionList.m_pInternal->GetNext(pos);
                if (pos == 0) return;
                val = *static_cast<unsigned long long*>(pData);
            }
            m_headFollowExclusionList.m_pInternal->Remove(pos);
        }
    }
}

void CGuiInGame::ShowSoloModeQuery(int text)
{
    if (m_bInitialized == 0 || m_pGuiManager == 0 || m_pSoloModePanel == 0) return;

    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    bool bMultiPartyMember = pParty != 0 && pParty->m_nCharacterCount > 1;

    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    CSWSCreature* pServerCreature = pPlayerCreature != 0 ? pPlayerCreature->GetServerCreature() : 0;

    CSWSArea* pServerArea = 0;
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule != 0) {
        pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule->m_pArea != 0) {
            pServerArea = pModule->m_pArea->GetServerArea();
        }
    }

    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pInGameGui->m_nGlobalDialogState == 0 &&
        (pServerArea == 0 || pServerArea->m_bTransitionPending == 0) &&
        g_pAppManager->m_pClientExoApp->GetAreaNotReadyToDisplay() == 0) {
        if (pServerCreature != 0) {
            if (pServerCreature->GetDead() != 0) goto playSound;
#if KOTOR1  // K1: GetDeadTemp instead of the in-party hit-point test
            int bDeadTemp = pServerCreature->GetDeadTemp();
            if (!bMultiPartyMember) goto playSound;
            if (bDeadTemp != 0) goto playSound;
            goto doQuery;
#else
            if (pServerCreature->m_bInParty != 0) {
                short hp = pServerCreature->GetCurrentHitPoints(0);
                if (bMultiPartyMember && hp >= 1) goto doQuery;
                goto playSound;
            }
#endif
        }
        if (bMultiPartyMember) {
doQuery:
            if (m_bPaused == 0) {
                g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
            }
            static_cast<CSWGuiInGameSoloModeQuery*>(m_pSoloModePanel)->SetQueryText(text);
            if (m_pGuiManager->PanelExists(m_pSoloModePanel) == 0) {
                m_pGuiManager->AddPanel(m_pSoloModePanel, 1, 1);
            }
            g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
            return;
        }
    }
playSound:
    m_pGuiManager->PlayGuiSound(GUISOUND_ERROR_DEFAULT);
}

void CGuiInGame::StopBarkBubble(int a)
{
    if (m_pBarkBubble != 0) {
        m_pBarkBubble->StopBark(a);
    }
}

void CGuiInGame::SetMainInterfaceTarget(unsigned long long objId, int flag)
{
    if (m_pMainInterface != 0) {
        m_pMainInterface->SetTarget(objId, flag);
    }
}

void CGuiInGame::SetAreaTransition(unsigned long long objId, const Vector& pos)
{
    m_areaTransitionPosition = pos;
    m_areaTransitionObjId = objId;
}

void CGuiInGame::RePopulateMainInterface()
{
    if (m_pMainInterface != 0) {
        m_pMainInterface->PopulateMenus(1);
    }
}

void* CGuiInGame::GetGuiInGameScreenPanel(unsigned char index)
{
    if (index > 7) {
        return 0;
    }
    return static_cast<void*>(m_apScreenPanels[index]);
}

void CGuiInGame::AddFloatyText(unsigned long long objId, const CExoString& text, const Vector& pos, float f)
{
    if (m_pMainInterface != 0) {
        m_pMainInterface->AddFloatyText(objId, text, pos, f);
    }
}

void CGuiInGame::InitializeMap()
{
    if (m_pMainInterface != 0) {
        m_pMainInterface->InitializeMap();
    }
}

void CGuiInGame::UpdateStatus(unsigned char status, int value)
{
    CSWGuiStatusSummary* pSummary = m_pStatusSummary;
    if (pSummary == 0) {
        return;
    }
    if (pSummary->m_suppressFlag > 0) {
        pSummary->m_suppressFlag = pSummary->m_suppressFlag - 1;
        return;
    }

    switch (status) {
    case GUI_STATUS_JOURNAL:
        pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_DISPLAY_PENDING | CSWGuiStatusSummary::SUMMARY_JOURNAL;
        break;
    case GUI_STATUS_CREDITS:
        pSummary->AddCredits(value);
        break;
    case GUI_STATUS_XP:
        if (value != 0) {
            pSummary->m_nXP += value;
            pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_DISPLAY_PENDING;
        }
        break;
    case GUI_STATUS_STEALTH_XP:
        if (value != 0) {
            pSummary->m_nStealthXP += value;
            pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_DISPLAY_PENDING;
        }
        break;
    case GUI_STATUS_ALIGNMENT_SHIFT:
    case GUI_STATUS_ALIGNMENT_SHIFT_UNUSED:
        pSummary->AddAlignmentShift(static_cast<char>(value));
        break;
    case GUI_STATUS_ITEM_RECEIVED:
        pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_DISPLAY_PENDING | CSWGuiStatusSummary::SUMMARY_ITEM_RECEIVED;
        break;
    case GUI_STATUS_ITEM_LOST:
        pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_DISPLAY_PENDING | CSWGuiStatusSummary::SUMMARY_ITEM_LOST;
        break;
#if KOTOR1  // K1: no influence or max-FP statuses
    case GUI_STATUS_LEVEL_UP_SOUND:
        pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_SOUND_PENDING | CSWGuiStatusSummary::SUMMARY_LEVEL_UP_SOUND;
        break;
    case GUI_STATUS_NEW_QUEST_SOUND:
        pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_SOUND_PENDING | CSWGuiStatusSummary::SUMMARY_NEW_QUEST_SOUND;
        break;
    case GUI_STATUS_COMPLETE_SOUND:
        pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_SOUND_PENDING | CSWGuiStatusSummary::SUMMARY_COMPLETE_SOUND;
        break;
#else
    case GUI_STATUS_INFLUENCE_GAINED:
        pSummary->AddInfluenceGained(static_cast<char>(value));
        break;
    case GUI_STATUS_INFLUENCE_LOST:
        pSummary->AddInfluenceLost(static_cast<char>(value));
        break;
    case GUI_STATUS_MAX_FP_GAINED:
        pSummary->AddMaxFPGained(value);
        break;
    case GUI_STATUS_MAX_FP_LOST:
        pSummary->AddMaxFPLost(value);
        break;
    case GUI_STATUS_LEVEL_UP_SOUND:
        pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_SOUND_PENDING | CSWGuiStatusSummary::SUMMARY_LEVEL_UP_SOUND;
        break;
    case GUI_STATUS_NEW_QUEST_SOUND:
        pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_SOUND_PENDING | CSWGuiStatusSummary::SUMMARY_NEW_QUEST_SOUND;
        break;
    case GUI_STATUS_COMPLETE_SOUND:
        pSummary->m_flags |= CSWGuiStatusSummary::SUMMARY_SOUND_PENDING | CSWGuiStatusSummary::SUMMARY_COMPLETE_SOUND;
        break;
#endif
    default:
        break;
    }
}

void CGuiInGame::GetStatusSummary(CStatusSummary& out)
{
    if (m_pStatusSummary != 0) {
        m_pStatusSummary->GetSummary(out);
    }
}

void CGuiInGame::SuppressStatusSummary(int flag)
{
    if (flag < 0) {
        return;
    }
    if (m_pStatusSummary == 0) {
        return;
    }
    m_pStatusSummary->m_suppressFlag = flag;
}

void CGuiInGame::ShowFlashingStatus(unsigned char status)
{
    if (m_pMainInterface != 0) {
        m_pMainInterface->ShowFlashingStatus(status);
    }
}

int CGuiInGame::SetTutorialReason(unsigned char reason)
{
    if (m_pTutorialBox != 0)
        return m_pTutorialBox->SetTutorialReason(reason);
    return 0;
}

int CGuiInGame::GetAreaTransitionVisible()
{
    return m_pGuiManager->PanelExists(m_pAreaTransitionPanel);
}

void CGuiInGame::ChangeCharacter()
{
    m_pMainInterface->StartRotation();
}

int CGuiInGame::IsPauseGUIVisible()
{
    return m_pGuiManager->PanelExists(m_pPauseGuiPanel);
}

void CGuiInGame::SetupPauseGuiExtent(CSWGuiExtent* pExtent)
{
    m_pMainInterface->SetupPauseGuiExtent(pExtent);
}

unsigned char CGuiInGame::GetInterfaceOffset()
{
    if (m_pMainInterface != 0) {
        return m_pMainInterface->m_interfaceOffset;
    }
    return 0;
}

void CGuiInGame::SetCombatMessage(uint64_t msg)
{
    if (m_pMainInterface != 0) {
        m_pMainInterface->SetCombatMessage(msg);
    }
}

uint64_t CGuiInGame::GetCombatMessage()
{
    if (m_pMainInterface != 0) {
        return m_pMainInterface->m_combatMessage;
    }
    return 0;
}

void CGuiInGame::SetDeferredFOVCallForAnimatedCutscenes(float fov)
{
    m_bDeferredFOVCall = 1;
    m_deferredFOV = fov;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::ShowGameMenuChangeButtons(int show)
{
    m_pInGameMenu->ShowChangeButtons(show);
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::EnableCharacterChangeButtons(int flag)
{
    if (m_pInGameMenu != 0) {
        m_pInGameMenu->m_bChangeButtonsDisabled = (flag == 0);
    }
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::ResetTutorialWindowTracker()
{
    __builtin_memset(m_tutorialWindowShown, 0, sizeof(m_tutorialWindowShown));
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::SetTopMenuOverrideCharacter(int charId)
{
    if (m_pInGameMenu != 0) {
        m_pInGameMenu->m_nTopMenuOverrideChar = charId;
    }
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
int CGuiInGame::GetTopMenuOverrideCharacter()
{
    if (m_pInGameMenu != 0) {
        return m_pInGameMenu->m_nTopMenuOverrideChar;
    }
    return -1;
}
#endif // !KOTOR1

void CGuiInGame::SetDialogAnimationData(unsigned int index, unsigned long long data, unsigned short flags)
{
    if (index >= m_dialogAnimCount) {
        return;
    }
    if (m_paDialogAnimData == 0) {
        return;
    }
    if (m_paDialogAnimFlags == 0) {
        return;
    }
    m_paDialogAnimData[index] = data;
    m_paDialogAnimFlags[index] = flags;
}

// Reconciles this frame's animated objects with the previous frame's.
void CGuiInGame::SetDialogAnimations()
{
    for (int i = 0; i < 3; i++) {
        CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_paLastPartyCharacterIds[i]);
        if (pObj != 0 && pObj->AsSWCCreature() != 0) {
            CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObj->AsSWCCreature());
            static_cast<CSWCObject*>(pCreature)->m_pModel->SetInCutscene(0);
            m_paLastPartyCharacterIds[i] = OBJECT_INVALID;
        }
    }

    bool bFoundParticipantB = false;
    bool bFoundParticipantA = false;
    if (static_cast<int>(m_lastDialogAnimCount) < 1) {
        bFoundParticipantA = false;
    } else {
        bFoundParticipantB = false;
        bFoundParticipantA = false;
        for (int i = 0; i < static_cast<int>(m_lastDialogAnimCount); i++) {
            unsigned long long objId = m_paLastDialogAnimData[i];
            CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
            if (pGameObj == 0) continue;

            bool bStillAnimating = false;
            if (0 < static_cast<int>(m_dialogAnimCount)) {
                for (int j = 0; j < static_cast<int>(m_dialogAnimCount); j++) {
                    if (objId == m_paDialogAnimData[j]) {
                        bStillAnimating = true;
                        break;
                    }
                }
            }
            if (objId == m_oidPrevDialogParticipantB) bFoundParticipantB = true;
            if (objId == m_oidPrevDialogParticipantA) bFoundParticipantA = true;
            if (bStillAnimating) continue;

            if (pGameObj->AsSWCCreature() != 0) {
                CSWCCreature* pCreature = static_cast<CSWCCreature*>(pGameObj->AsSWCCreature());
                int bCanPlay = CanPlayDialogAnimations(pCreature->m_objectId);
                if (pCreature != 0 && bCanPlay != 0) {
                    pCreature->m_bPlayingCutsceneAnimation = 0;
                    if (static_cast<CSWCObject*>(pCreature)->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                        static_cast<CSWCObject*>(pCreature)->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                    }
                    static_cast<CSWCObject*>(pCreature)->m_pModel->SetInCutscene(0);
                }
            }
            if (pGameObj->AsSWCPlaceable() != 0) {
                CSWCPlaceable* pPlaceable = static_cast<CSWCPlaceable*>(pGameObj->AsSWCPlaceable());
                CSWSPlaceable* pServerPlaceable = pPlaceable->GetServerPlaceable();
                if (pPlaceable != 0) {
                    pPlaceable->m_bPlayingCutsceneAnimation = 0;
                    static_cast<CSWCObject*>(pPlaceable)->SetAnimation(static_cast<unsigned short>(pServerPlaceable->m_nAnimation), 0, 0);
                    static_cast<CSWCObject*>(pPlaceable)->m_pModel->SetInCutscene(0);
                }
            }
        }
        if (bFoundParticipantB) goto skipParticipantB;
    }
    {
        CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidPrevDialogParticipantB);
        if (pObj != 0 && pObj->AsSWCObject() != 0) {
#if KOTOR1  // K1: no door exemption
            if (pObj->AsSWCCreature() == 0 ||
                CanPlayDialogAnimations(static_cast<CSWCCreature*>(pObj->AsSWCCreature())->m_objectId) != 0) {
                if (static_cast<CSWCObject*>(pObj->AsSWCObject())->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                }
                static_cast<CSWCObject*>(pObj->AsSWCObject())->m_pModel->SetInCutscene(0);
            }
#else
            bool bNeedsReset = false;
            bool bCanPlay = true;
            if (pObj->AsSWCCreature() == 0) {
                CSWCObject* pPlain = static_cast<CSWCObject*>(pObj->AsSWCObject());
                if (pPlain->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE && pObj->AsSWCDoor() == 0) {
                    bNeedsReset = true;
                }
            } else {
                CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObj->AsSWCCreature());
                bCanPlay = CanPlayDialogAnimations(pCreature->m_objectId) != 0;
                if (bCanPlay) {
                    CSWCObject* pPlain = static_cast<CSWCObject*>(pObj->AsSWCObject());
                    bNeedsReset = pPlain->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE;
                }
            }
            if (bCanPlay) {
                if (bNeedsReset) {
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                }
                static_cast<CSWCObject*>(pObj->AsSWCObject())->m_pModel->SetInCutscene(0);
            }
#endif
        }
    }
skipParticipantB:;

    if (!bFoundParticipantA) {
        CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidPrevDialogParticipantA);
        if (pObj != 0 && pObj->AsSWCObject() != 0) {
#if KOTOR1  // K1: no door exemption
            if (pObj->AsSWCCreature() == 0 ||
                CanPlayDialogAnimations(static_cast<CSWCCreature*>(pObj->AsSWCCreature())->m_objectId) != 0) {
                if (static_cast<CSWCObject*>(pObj->AsSWCObject())->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                }
                static_cast<CSWCObject*>(pObj->AsSWCObject())->m_pModel->SetInCutscene(0);
            }
#else
            bool bNeedsReset = false;
            bool bCanPlay = true;
            if (pObj->AsSWCCreature() == 0) {
                CSWCObject* pPlain = static_cast<CSWCObject*>(pObj->AsSWCObject());
                if (pPlain->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE && pObj->AsSWCDoor() == 0) {
                    bNeedsReset = true;
                }
            } else {
                CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObj->AsSWCCreature());
                bCanPlay = CanPlayDialogAnimations(pCreature->m_objectId) != 0;
                if (bCanPlay) {
                    CSWCObject* pPlain = static_cast<CSWCObject*>(pObj->AsSWCObject());
                    bNeedsReset = pPlain->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE;
                }
            }
            if (bCanPlay) {
                if (bNeedsReset) {
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                }
                static_cast<CSWCObject*>(pObj->AsSWCObject())->m_pModel->SetInCutscene(0);
            }
#endif
        }
    }

    bool bFoundDialogB = false;
    bool bFoundDialogA = false;
    if (0 < static_cast<int>(m_dialogAnimCount)) {
        for (int i = 0; i < static_cast<int>(m_dialogAnimCount); i++) {
            unsigned long long objId = m_paDialogAnimData[i];
            unsigned short flags = m_paDialogAnimFlags[i];
            CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
            if (pGameObj == 0) continue;

            bool bFoundLast = false;
            unsigned short lastFlags = 0;
            if (0 < static_cast<int>(m_lastDialogAnimCount)) {
                for (int j = 0; j < static_cast<int>(m_lastDialogAnimCount); j++) {
                    if (objId == m_paLastDialogAnimData[j]) {
                        lastFlags = m_paLastDialogAnimFlags[j];
                        bFoundLast = true;
                        break;
                    }
                }
            }

            if (objId == m_dialogParticipantIdB) bFoundDialogB = true;
            if (objId == m_dialogParticipantIdA) bFoundDialogA = true;

            if (!bFoundLast || flags != lastFlags) {
                if (pGameObj->AsSWCCreature() != 0) {
                    CSWCCreature* pCreature = static_cast<CSWCCreature*>(pGameObj->AsSWCCreature());
                    if (pCreature != 0 && CanPlayDialogAnimations(pCreature->m_objectId) != 0) {
                        if (IsLoopingDialogAnimation(flags) != 0) {
                            pCreature->m_bPlayingCutsceneAnimation = 0;
                            static_cast<CSWCObject*>(pCreature)->m_pModel->SetInCutscene(1);
                            pCreature->LookAt(OBJECT_INVALID, 0.0f);
                            if (static_cast<CSWCObject*>(pCreature)->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                                if (static_cast<unsigned short>(flags >> 4) >= DIALOG_ANIM_2DA_BASE / 16 &&
                                    g_pRules->m_p2DArrays->m_pDialogAnimations->m_nRows + DIALOG_ANIM_2DA_BASE > flags) {
                                    unsigned short row = static_cast<unsigned short>(flags - DIALOG_ANIM_2DA_BASE);
                                    int overlayEntry, dialogEntry;
                                    if (g_pRules->m_p2DArrays->m_pDialogAnimations->GetINTEntry(
                                            row, CTwoDimArrays::COL2DA_DIALOGANIMATIONS_OVERLAY, &overlayEntry) &&
                                        g_pRules->m_p2DArrays->m_pDialogAnimations->GetINTEntry(
                                            row, CTwoDimArrays::COL2DA_DIALOGANIMATIONS_DIALOG, &dialogEntry) &&
                                        overlayEntry != 0 && dialogEntry != 0) {
                                        static_cast<CSWCObject*>(pCreature)->AbortFireForgetAnimation(0);
                                    }
                                }
                                static_cast<CSWCObject*>(pCreature)->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                            }
                            static_cast<CSWCObject*>(pCreature)->SetAnimation(flags, 0, 0);
                        }
                        if (IsFireForgetDialogAnimation(flags) != 0) {
                            pCreature->m_bPlayingCutsceneAnimation = 0;
                            static_cast<CSWCObject*>(pCreature)->m_pModel->SetInCutscene(1);
                            pCreature->LookAt(OBJECT_INVALID, 0.0f);
                            if (static_cast<CSWCObject*>(pCreature)->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                                static_cast<CSWCObject*>(pCreature)->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                            }
                            if (static_cast<unsigned short>(flags - STUNT_ANIM_FIREFORGET_FIRST) < STUNT_ANIM_COUNT) {
                                static_cast<CSWCObject*>(pCreature)->AbortFireForgetAnimation(0);
                                static_cast<CSWCObject*>(pCreature)->RemoveAllFireForgetAnimations();
                                static_cast<CSWCObject*>(pCreature)->AddFireForgetAnimation(flags, 0);
                                pCreature->AnimateFireAndForget();
                            } else {
                                static_cast<CSWCObject*>(pCreature)->AddFireForgetAnimation(flags, 0);
                            }
                        }
                    }
                }
                if (pGameObj->AsSWCPlaceable() != 0) {
                    CSWCPlaceable* pPlaceable = static_cast<CSWCPlaceable*>(pGameObj->AsSWCPlaceable());
                    if (pPlaceable != 0) {
                        if (IsLoopingDialogAnimation(flags) != 0) {
                            pPlaceable->m_bPlayingCutsceneAnimation = 0;
                            static_cast<CSWCObject*>(pPlaceable)->m_pModel->SetInCutscene(1);
                            if (static_cast<CSWCObject*>(pPlaceable)->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                                static_cast<CSWCObject*>(pPlaceable)->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 0);
                            }
                            static_cast<CSWCObject*>(pPlaceable)->SetAnimation(flags, 0, 0);
                        }
                        if (IsFireForgetDialogAnimation(flags) != 0) {
                            pPlaceable->m_bPlayingCutsceneAnimation = 0;
                            static_cast<CSWCObject*>(pPlaceable)->m_pModel->SetInCutscene(1);
                            if (static_cast<CSWCObject*>(pPlaceable)->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                                static_cast<CSWCObject*>(pPlaceable)->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 0);
                            }
                            if (static_cast<unsigned short>(flags - STUNT_ANIM_FIREFORGET_FIRST) < STUNT_ANIM_COUNT) {
                                static_cast<CSWCObject*>(pPlaceable)->AbortFireForgetAnimation(0);
                                static_cast<CSWCObject*>(pPlaceable)->RemoveAllFireForgetAnimations();
                            }
                            static_cast<CSWCObject*>(pPlaceable)->AddFireForgetAnimation(flags, 0);
                        }
                    }
                }
            }
        }
        if (bFoundDialogB) goto skipDialogB;
    }
    {
        CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogParticipantIdB);
        if (pObj != 0 && pObj->AsSWCObject() != 0 && pObj->AsSWCCreature() != 0) {
            CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObj->AsSWCCreature());
            if (CanPlayDialogAnimations(pCreature->m_objectId) != 0) {
                static_cast<CSWCObject*>(pObj->AsSWCObject())->m_pModel->SetInCutscene(1);
                CSWCObject* pPlain = static_cast<CSWCObject*>(pObj->AsSWCObject());
                if (pPlain->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->AbortFireForgetAnimation(0);
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                }
                unsigned int nAnimation = static_cast<CSWCCreature*>(pObj->AsSWCCreature())->IsInjured() == 0 ? SERVER_ANIMATION_LISTEN : SERVER_ANIMATION_LISTEN_INJURED;
                static_cast<CSWCObject*>(pObj->AsSWCObject())->SetAnimation(static_cast<unsigned short>(nAnimation), 0, 0);
            }
        }
    }
skipDialogB:;

    if (!bFoundDialogA) {
        CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogParticipantIdA);
        if (pObj != 0 && pObj->AsSWCObject() != 0 && pObj->AsSWCCreature() != 0) {
            CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObj->AsSWCCreature());
            if (CanPlayDialogAnimations(pCreature->m_objectId) != 0) {
                static_cast<CSWCObject*>(pObj->AsSWCObject())->m_pModel->SetInCutscene(1);
                CSWCObject* pPlain = static_cast<CSWCObject*>(pObj->AsSWCObject());
                if (pPlain->GetCurrentAnimation() != SERVER_ANIMATION_PAUSE) {
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->AbortFireForgetAnimation(0);
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->SetAnimation(SERVER_ANIMATION_PAUSE, 0, 1);
                }
                unsigned int nAnimation = static_cast<CSWCCreature*>(pObj->AsSWCCreature())->IsInjured() == 0 ? SERVER_ANIMATION_TALK_NORMAL : SERVER_ANIMATION_TALK_INJURED;
                static_cast<CSWCObject*>(pObj->AsSWCObject())->SetAnimation(static_cast<unsigned short>(nAnimation), 0, 0);
            }
        }
    }

    CSWCCreature* pChar0 = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pChar0 == 0) {
        m_paLastPartyCharacterIds[0] = OBJECT_INVALID;
    } else {
        static_cast<CSWCObject*>(pChar0)->m_pModel->SetInCutscene(1);
        m_paLastPartyCharacterIds[0] = pChar0->m_objectId;
    }

    CSWCCreature* pChar1 = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(1);
    if (pChar1 == 0) {
        m_paLastPartyCharacterIds[1] = OBJECT_INVALID;
    } else {
        static_cast<CSWCObject*>(pChar1)->m_pModel->SetInCutscene(1);
        m_paLastPartyCharacterIds[1] = pChar1->m_objectId;
    }

    CSWCCreature* pChar2 = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(2);
    if (pChar2 == 0) {
        m_paLastPartyCharacterIds[2] = OBJECT_INVALID;
    } else {
        static_cast<CSWCObject*>(pChar2)->m_pModel->SetInCutscene(1);
        m_paLastPartyCharacterIds[2] = pChar2->m_objectId;
    }
}

int CGuiInGame::SetAnimatedCamera(unsigned short cameraId, float speed)
{
    if (static_cast<unsigned short>(cameraId - STUNT_ANIM_FIREFORGET_FIRST) >= STUNT_ANIM_SPAN) return false;
    if (m_bAnimatedCameraLoaded == 0) return false;
    CSWCModule* pModule = static_cast<CSWCModule*>(g_pAppManager->m_pClientExoApp->GetModule());
    if (pModule == 0) return false;
    m_nCameraAnimation = cameraId;
    m_bDialogCameraSet = 0;
    m_bAnimatedCameraActive = 1;
    CGameObject* pObj = g_pAppManager->m_pServerExoApp->GetGameObject(m_oidDialogOwner);
    if (pObj != 0 && pObj->AsSWSObject() != 0) {
        pObj->AsSWSObject()->TimeStampCameraAnimation();
    }
    m_fAnimatedCameraLength = pModule->SetAnimatedCamera(cameraId, speed, m_dialogParticipantIdA);
    return true;
}

void CGuiInGame::DialogGuiDoneScroll()
{
    if (m_pBottomLetterbox != 0) {
        m_dialogScrollDoneFlag = 1;
        static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->ShowDialogMessage();
    }
}

void CGuiInGame::SetGuiInGameInventoryDirty(int flag)
{
    if (m_apScreenPanels[SCREEN_INVENTORY] != 0) {
        static_cast<CSWGuiInGameInventory*>(m_apScreenPanels[SCREEN_INVENTORY])->m_bRebuildItemList = static_cast<unsigned char>(flag);
    }
}

Vector CGuiInGame::GetAreaTranstionPosition()
{
    if (m_pAreaTransitionPanel != 0) {
        return static_cast<CSWGuiInGameAreaTransition*>(m_pAreaTransitionPanel)->m_position;
    }
    Vector zero;
    zero.z = 0;
    zero.y = 0;
    zero.x = 0;
    return zero;
}

Vector CGuiInGame::GetDialogCameraPosition(int bOffsetByOrientation)
{
    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    Vector vPos;
    if (pPlayerCreature != 0) {
        vPos = pPlayerCreature->m_desiredPosition;
    } else {
        vPos.x = 0;
        vPos.y = 0;
        vPos.z = 0;
    }

    if (m_bDialogCameraSet == 0 && m_bAnimatedCameraActive == 0) {
        return vPos;
    }

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule == 0) {
        return vPos;
    }
    pModule = g_pAppManager->m_pClientExoApp->GetModule();
    CAurCamera* pCamera = pModule->m_pCamera;
    g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pCamera == 0) {
        return vPos;
    }

    vPos = pCamera->GetPosition();
    if (bOffsetByOrientation != 0) {
        Quaternion q = pCamera->GetOrientation();
        Vector vDir = {
            -2.0f * (q.x * q.z + q.w * q.y),
            -2.0f * (q.z * q.y - q.x * q.w),
            -(1.0f - 2.0f * (q.x * q.x + q.y * q.y))
        };
        vDir = normalize(vDir);
        vPos.x += vDir.x * 5.0f;
        vPos.y += vDir.y * 5.0f;
        vPos.z += vDir.z * 5.0f;
    }
    return vPos;
}

void CGuiInGame::HideAreaTransition()
{
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule == 0) {
        goto doAreaTransition;
    }
    pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule->m_pArea == 0) {
        goto doAreaTransition;
    }
    {
        CSWSArea* pServerArea = pModule->m_pArea->GetServerArea();
        if (pServerArea == 0 || pServerArea->m_bTransitionPending == 0) {
            goto doAreaTransition;
        }
        if (m_pGuiManager->PanelExists(m_pAreaTransitionPanel) != 0) {
            m_pGuiManager->SendPanelToBack(m_pAreaTransitionPanel);
        }
        return;
    }

doAreaTransition:
    if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pAreaTransitionPanel != 0) {
        Vector zero2 = {0, 0, 0};
        static_cast<CSWGuiInGameAreaTransition*>(m_pAreaTransitionPanel)->SetTransitionObject(OBJECT_INVALID, zero2);
        if (m_pGuiManager->PanelExists(m_pAreaTransitionPanel) != 0) {
            m_pGuiManager->RemovePanel(m_pAreaTransitionPanel);
        }
    }
}

void CGuiInGame::UpdateAreaTransitionGUI()
{
    if (m_bInitialized == 0 || m_pGuiManager == 0 || m_pAreaTransitionPanel == 0) return;

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule == 0) {
        goto doHide;
    }
    pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule->m_pArea == 0) {
        goto doHide;
    }
    {
        CSWSArea* pServerArea = pModule->m_pArea->GetServerArea();
        if (pServerArea == 0) {
            goto doHide;
        }
        if (m_swGuiToggleState != SWGUI_STATE_SHOWN) {
            goto doHide;
        }

        unsigned long long objId = m_areaTransitionObjId;
        if (static_cast<CSWGuiInGameAreaTransition*>(m_pAreaTransitionPanel)->m_objId == objId) {
            Vector pos1 = static_cast<CSWGuiInGameAreaTransition*>(m_pAreaTransitionPanel)->m_position;
            Vector pos2 = m_areaTransitionPosition;
            Vector delta;
            delta.x = pos1.x - pos2.x;
            delta.y = pos1.y - pos2.y;
            delta.z = pos1.z - pos2.z;
            if (magnitude(delta) < 0.0001f) return;
            objId = m_areaTransitionObjId;
        }

        if (objId == OBJECT_INVALID) {
            goto doHide;
        }

        if (pServerArea->m_bTransitionPending != 0) {
            if (m_pGuiManager->PanelExists(m_pAreaTransitionPanel) != 0) goto sendToBack;
            return;
        }
        static_cast<CSWGuiInGameAreaTransition*>(m_pAreaTransitionPanel)->SetTransitionObject(objId, m_areaTransitionPosition);
        if (m_pGuiManager->PanelExists(m_pAreaTransitionPanel) != 0) return;
        m_pGuiManager->AddPanel(m_pAreaTransitionPanel, 4, 1);
sendToBack:
        m_pGuiManager->SendPanelToBack(m_pAreaTransitionPanel);
        return;
    }

doHide:
    HideAreaTransition();
}

void CGuiInGame::ShowAreaTransition(unsigned long long objId, const Vector& pos)
{
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule == 0) {
        goto doHide;
    }
    pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule->m_pArea == 0) {
        goto doHide;
    }
    {
        CSWSArea* pServerArea = pModule->m_pArea->GetServerArea();
        if (objId == OBJECT_INVALID) {
            goto doHide;
        }
        if (pServerArea == 0) {
            goto doHide;
        }
        if (m_swGuiToggleState != SWGUI_STATE_SHOWN) {
            goto doHide;
        }
        if (pServerArea->m_bTransitionPending == 0) {
            if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pAreaTransitionPanel != 0) {
                static_cast<CSWGuiInGameAreaTransition*>(m_pAreaTransitionPanel)->SetTransitionObject(objId, pos);
                if (m_pGuiManager->PanelExists(m_pAreaTransitionPanel) == 0) {
                    m_pGuiManager->AddPanel(m_pAreaTransitionPanel, 4, 1);
                    goto sendToBack;
                }
            }
        } else {
            if (m_pGuiManager->PanelExists(m_pAreaTransitionPanel) != 0) {
sendToBack:
                m_pGuiManager->SendPanelToBack(m_pAreaTransitionPanel);
            }
        }
        return;
    }
doHide:
    HideAreaTransition();
}

void CGuiInGame::SetGlobalDialogState(int state)
{
    m_nGlobalDialogState = state;
#if !KOTOR1  // K1 has no letterbox-mode global
    g_bLetterBoxMode = (state == 1);
#endif
    if (state != 0) {
        return;
    }

    m_bAnimatedCut = 0;
    g_pAppManager->m_pServerExoApp->SetIgnoreConversationStartRange(0);

    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pInGameGui->m_unpauseableRefCount != 0) {
        pInGameGui->m_unpauseableRefCount = pInGameGui->m_unpauseableRefCount - 1;
    }

    if (m_bHoldFadeInForDialog != 0) {
        m_bHoldFadeInForDialog = 0;
        Vector zero = {0.0f, 0.0f, 0.0f};
        StartGlobalFade(1, 0.0f, 1.0f, zero);
    }

    g_pAppManager->m_pClientExoApp->m_pInternal->ShowMouse(CClientExoAppInternal::MOUSEHIDE_DIALOG_CUTSCENE);
}

int CGuiInGame::CloseDialog()
{
    if (m_bInitialized == 0) {
        return 0;
    }

    g_pAppManager->m_pClientExoApp->DisableVideoEffect();
    while (m_dialogNonDependantList.m_pInternal->m_pHead != 0) {
        void* pData = m_dialogNonDependantList.m_pInternal->RemoveHead();
        if (pData != 0) {
            static_cast<CExoString*>(pData)->~CExoString();
            operator delete(pData);
        }
    }
    m_bConversationUseLeader = 0;

    if (m_oidPostDialogCharacterSwitch != OBJECT_INVALID) {
#if KOTOR1  // K1: the party-selection screen is a different slot
        if (!m_pGuiManager->PanelExists(m_pMessagesPanel)) {
#else
        if (!m_pGuiManager->PanelExists(m_apScreenPanels[SCREEN_PARTY_SELECTION])) {
#endif
            CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
            if (pParty->m_nCharacterCount >= 1) {
                int i = 0;
                do {
                    pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
                    CSWCCreature* pChar = pParty->GetCharacter(0);
                    if (pChar != 0 &&
                        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pChar->m_objectId) == m_oidPostDialogCharacterSwitch) {
                        break;
                    }
                    g_pAppManager->m_pClientExoApp->ChangeCharacter(-2, 0);
                    i = i + 1;
                    pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
                } while (i < pParty->m_nCharacterCount);
            }
        }
        m_oidPostDialogCharacterSwitch = OBJECT_INVALID;
    }

    CGameObject* pObjA = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogParticipantIdA);
    if (pObjA != 0 && pObjA->AsSWCCreature() != 0) {
        CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObjA->AsSWCCreature());
        pCreature->LookAt(OBJECT_INVALID, 0.0f);
    }
    CGameObject* pObjB = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogParticipantIdB);
    if (pObjB != 0 && pObjB->AsSWCCreature() != 0) {
        CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObjB->AsSWCCreature());
        pCreature->LookAt(OBJECT_INVALID, 0.0f);
    }
    unsigned long long oldIdB = m_dialogParticipantIdB;
    unsigned long long oldIdA = m_dialogParticipantIdA;
    m_dialogParticipantIdB = OBJECT_INVALID;
    m_dialogParticipantIdA = OBJECT_INVALID;
    m_oidPrevDialogParticipantB = oldIdB;
    m_oidPrevDialogParticipantA = oldIdA;
    ResetDialogAnimations();
    ResetTalkAnimations();
    m_dialogInputActive = 0;
    SetGlobalDialogState(0);

    void* pActiveCam = m_pActiveDialogPanel;
    if (pActiveCam == m_pDialogComputerPanel) {
        CSWGuiPanel* pPanel = static_cast<CSWGuiPanel*>(pActiveCam);
#if KOTOR1  // K1: no panel chain, one stamp
        if (g_bCreateAllGUIs == 0) {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_MASK) | PANEL_REMOVE_AND_DELETE | PANEL_REMOVE_DEFERRED;
        } else {
            unsigned short flags = pPanel->m_stateFlags;
            unsigned short newFlags = flags | PANEL_REMOVE_DEFERRED;
            if ((flags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE) {
                newFlags = (newFlags & ~PANEL_REMOVE_AND_DELETE) | PANEL_REMOVE;
            }
            pPanel->m_stateFlags = newFlags;
        }
#else
        if (g_bCreateAllGUIs == 0) {
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_MASK) | PANEL_REMOVE_AND_DELETE | PANEL_REMOVE_DEFERRED;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel != 0);
        } else {
            unsigned short flags = pPanel->m_stateFlags;
            unsigned short newFlags = flags | PANEL_REMOVE_DEFERRED;
            if ((flags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE) {
                newFlags = (newFlags & ~PANEL_REMOVE_AND_DELETE) | PANEL_REMOVE;
            }
            pPanel->m_stateFlags = newFlags;
        }
#endif
        m_pDialogComputerPanel = 0;
    } else {
        m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(pActiveCam));
    }
    m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pDialogComputerCameraPanel));

    void* pActiveCam2 = m_pActiveDialogPanel;
    m_dialogScrollDoneFlag = 0;
    if (pActiveCam2 == m_pDialogCinematicPanel) {
        if (bDisableLetterbox == 0) {
            m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pTopLetterbox));
            m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pBottomLetterbox));
            m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pDialogLetterbox));
        }
        m_pTopLetterbox->m_bScrollDone = 0;
        m_pTopLetterbox->ResetFade();
        m_pBottomLetterbox->m_bScrollDone = 0;
        m_pBottomLetterbox->ResetFade();
        m_pDialogLetterbox->m_bScrollDone = 0;
        m_pDialogLetterbox->ResetFade();
        pActiveCam2 = m_pActiveDialogPanel;
    }
    static_cast<CSWGuiDialog*>(pActiveCam2)->EndDialog();
    m_pActiveDialogPanel = m_pDialogCinematicPanel;
    g_pAppManager->m_pClientExoApp->GetModule()->RestoreCamera();

    m_bDialogCameraSet = 0;
    m_bAnimatedCameraActive = 0;
    m_bAnimatedCameraLoaded = 0;
    m_fAnimatedCameraLength = 0.0f;
    m_startConversationCount = 0;
    m_nCameraAnimation = CAMERA_ANIMATION_NONE;
    m_oidDialogPlayer = OBJECT_INVALID;
    m_dialogParticipantIdA = OBJECT_INVALID;
    m_dialogParticipantIdB = OBJECT_INVALID;
    m_oidPrevDialogParticipantA = OBJECT_INVALID;
    m_oidPrevDialogParticipantB = OBJECT_INVALID;
    UnloadStuntModelsForDialog();
    ClearRepliesData();
    ClearDialogAnimationData();
    ClearLastDialogAnimationData();
    m_dialogSpanRemaining = 0;
    m_dialogSpanCount = 0;

    if (g_pAppManager->m_pClientExoApp->GetInputClass() == 3) {
#if KOTOR1  // K1: back to input class 0
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
#else
        void* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
        g_pAppManager->m_pClientExoApp->SetInputClass(pMiniGame != 0, 1);
#endif
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
    }
    g_pAppManager->m_pClientExoApp->ResetDriveAcceleration();

    if (m_bShowBarkOnDialogEnd != 0) {
        CResRef emptyResRef;
        ShowBarkBubble(OBJECT_INVALID, m_sBarkOnDialogEnd, emptyResRef, 0);
        m_bShowBarkOnDialogEnd = 0;
        m_sBarkOnDialogEnd = "";
    }

    g_pAppManager->m_pClientExoApp->SetLastClickedOnTarget(OBJECT_INVALID);
    return m_dialogInputActive;
}

void CGuiInGame::SetGlobalUnpauseableState(int flag)
{
    int delta;
    if (flag == 1) {
        delta = 1;
    } else {
        if (m_unpauseableRefCount == 0) {
            return;
        }
        delta = -1;
    }
    m_unpauseableRefCount = m_unpauseableRefCount + delta;
}

int CGuiInGame::GetPendingStatusSummary()
{
    if (m_pStatusSummary == 0) {
        return 0;
    }
    if ((m_pStatusSummary->m_flags & CSWGuiStatusSummary::SUMMARY_DISPLAY_PENDING) != 0) {
        return 1;
    }
    return (m_pStatusSummary->m_flags >> 1) & 1;
}

unsigned int CGuiInGame::GetTutorialWindowHasBeenShown(unsigned char index)
{
#if KOTOR1  // K1: 43 windows; one counts as shown without a touch screen
    if (index == 42) {
        if (!HasTouchScreen()) return 1;
    } else if (index > 42) {
        return 0;
    }
#endif
    unsigned char byteIndex = index >> 3;
    unsigned char bitIndex = index & 7;
    return (1 << bitIndex) & m_tutorialWindowShown[byteIndex];
}

void CGuiInGame::ShowDialogMessage()
{
    if (m_pActiveDialogPanel == m_pDialogCinematicPanel) {
        static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->ShowDialogMessage();
    }
}

int CGuiInGame::IsGlobalFadeObscuring()
{
#if !KOTOR1  // K1: the fade object is dereferenced unconditionally
    if (m_pGlobalFade == 0) return false;
#endif
    return m_pGlobalFade->m_fAlpha > 0.001f;
}

bool CGuiInGame::InitializeStuntModelData(unsigned int count)
{
    if (count == 0) return false;
    if (m_paStuntModelObjIds != 0) {
        operator delete[](m_paStuntModelObjIds);
        m_paStuntModelObjIds = 0;
    }
    if (m_paStuntModelResRefs != 0) {
        operator delete[](m_paStuntModelResRefs);
        m_paStuntModelResRefs = 0;
    }
    m_stuntModelCount = count;
    m_paStuntModelObjIds = static_cast<unsigned long long*>(operator new[](static_cast<uint64_t>(count) << 3));
    m_paStuntModelResRefs = new CResRef[count];
    if (m_paStuntModelObjIds == 0) {
        m_paStuntModelObjIds = 0;
        operator delete[](m_paStuntModelResRefs);
        m_paStuntModelResRefs = 0;
        m_stuntModelCount = 0;
        return false;
    }
    return true;
}

int CGuiInGame::LoadStuntModelsForDialog()
{
    if (m_stuntModelCount != 0) {
        for (uint64_t i = 0; i < m_stuntModelCount; i++) {
            CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_paStuntModelObjIds[i]);
            if (pObj != 0) {
                CSWCObject* pSWCObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
                if (pSWCObj != 0) {
                    char buf[20];
                    m_paStuntModelResRefs[i].CopyToString(buf);
                    if (buf[0] != '\0') {
                        Gob* pModel = pSWCObj->GetModel(ANIMBASE_MODEL_BODY, 0);
                        if (pModel == 0) return false;
                        pModel->LoadAddInAnimations(buf);
                        Gob* pModel2 = pSWCObj->GetModel(ANIMBASE_MODEL_HEAD, 0);
                        if (pModel2 != 0) {
                            pModel2->LoadAddInAnimations(buf);
                        }
                    }
                }
            }
        }
    }
    return true;
}

// Stops the stunt models' add-in animations and restores their idle animation.
void CGuiInGame::UnloadStuntModelsForDialog()
{
    if (m_stuntModelCount != 0) {
        unsigned int animId K2_UNSET;
        for (uint64_t i = 0; i < m_stuntModelCount; i++) {
            CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_paStuntModelObjIds[i]);
            if (pGameObj == 0) continue;

            CSWCObject* pObj = static_cast<CSWCObject*>(pGameObj->AsSWCObject());
            if (pObj == 0) continue;

            Gob* pModelA = pObj->GetModel(ANIMBASE_MODEL_BODY, 0);
            if (pModelA != 0) {
                pModelA->PlayAnimation(0, 1.0f, 0, 0.0f);
                pModelA->RemoveAddInAnimations();
            }
            Gob* pModelB = pObj->GetModel(ANIMBASE_MODEL_HEAD, 0);
            if (pModelB != 0) {
                pModelB->PlayAnimation(0, 1.0f, 0, 0.0f);
                pModelB->RemoveAddInAnimations();
            }

            CExoString animName;
            if (pObj->AsSWCCreature() != 0) {
                animName = pObj->GetAnimationBase()->GetAnimationName(SERVER_ANIMATION_PAUSE);
                animId = SERVER_ANIMATION_PAUSE;
            }
            if (pObj->AsSWCPlaceable() != 0) {
                CSWSPlaceable* pServerPlaceable = pObj->AsSWCPlaceable()->GetServerPlaceable();
                if (pServerPlaceable == 0) {
                    animName = static_cast<CSWCObject*>(pObj->AsSWCPlaceable())->GetAnimationBase()
                                   ->GetAnimationName(SERVER_ANIMATION_PAUSE);
                    animId = SERVER_ANIMATION_PAUSE;
                } else {
                    animId = pServerPlaceable->m_nAnimation;
                    animName = static_cast<CSWCObject*>(pObj->AsSWCPlaceable())->GetAnimationBase()
                                   ->GetAnimationName(static_cast<unsigned short>(animId));
                }
            }

            if (pModelA != 0) {
                pModelA->PlayAnimation(static_cast<char*>(animName.CStr()), 1.0f, 2, 0.0f);
            }
            if (pModelB != 0) {
                pModelB->PlayAnimation(static_cast<char*>(animName.CStr()), 1.0f, 2, 0.0f);
            }

            if (pObj->AsSWCCreature() != 0) {
                pObj->SetAnimation(static_cast<unsigned short>(animId), 0, 0);
            }
            if (pObj->AsSWCPlaceable() != 0) {
                static_cast<CSWCObject*>(pObj->AsSWCPlaceable())->SetAnimation(static_cast<unsigned short>(animId), 0, 0);
            }
        }
    }

    if (m_paStuntModelObjIds != 0) {
        operator delete[](m_paStuntModelObjIds);
        m_paStuntModelObjIds = 0;
    }
    if (m_paStuntModelResRefs != 0) {
        operator delete[](m_paStuntModelResRefs);
        m_paStuntModelResRefs = 0;
    }
    m_stuntModelCount = 0;
}

void CGuiInGame::SetStuntModelData(unsigned int index, unsigned long long objId, const CResRef& resRef)
{
    if (index >= m_stuntModelCount) return;
    if (m_paStuntModelObjIds == 0) return;
    if (m_paStuntModelResRefs == 0) return;
    m_paStuntModelObjIds[index] = objId;
    m_paStuntModelResRefs[index] = resRef;
}

int CGuiInGame::IsFadingInDialog()
{
    if (m_pDialogLetterbox == 0) {
        goto isFalse;
    }
    if (m_pDialogLetterbox->IsFadeDone() != 0) {
        goto isFalse;
    }
    return true;
isFalse:
    return false;
}

int CGuiInGame::IsGlobalFading()
{
    if (m_pGlobalFade == 0) {
        goto isFalse;
    }
    if (m_pGlobalFade->IsFadeDone() != 0) {
        goto isFalse;
    }
    return 1;
isFalse:
    return 0;
}

void CGuiInGame::SetTutorialWindowHasBeenShown(unsigned char index, int flag)
{
#if KOTOR1  // K1: higher ids are ignored
    if (index > 42) return;
#endif
    unsigned char byteIndex = index >> 3;
    unsigned char bitIndex = index & 7;
    unsigned char bit = m_tutorialWindowShown[byteIndex];
    unsigned char set = static_cast<unsigned char>(bit | (1 << bitIndex));
    unsigned char cleared = bit & ~(1 << bitIndex);
    m_tutorialWindowShown[byteIndex] = flag ? set : cleared;
}

void CGuiInGame::ClearDialogAnimationData()
{
    void* pData = m_paDialogAnimData;
    m_dialogAnimCount = 0;
    if (pData != 0) {
        operator delete[](pData);
        m_paDialogAnimData = 0;
    }
    if (m_paDialogAnimFlags != 0) {
        operator delete[](m_paDialogAnimFlags);
        m_paDialogAnimFlags = 0;
    }
}

void CGuiInGame::ClearLastDialogAnimationData()
{
    void* pData = m_paLastDialogAnimData;
    m_lastDialogAnimCount = 0;
    if (pData != 0) {
        operator delete[](pData);
        m_paLastDialogAnimData = 0;
    }
    if (m_paLastDialogAnimFlags != 0) {
        operator delete[](m_paLastDialogAnimFlags);
        m_paLastDialogAnimFlags = 0;
    }
}

void CGuiInGame::StopGlobalFade()
{
    if (m_pGlobalFade == 0) return;
#if !KOTOR1
    if (m_bFadeUntilScript != 0) return;
#endif
    m_pGlobalFade->ResetFade();
    m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pGlobalFade));
}

void CGuiInGame::ShowBarkBubble(unsigned long long objId, const CExoString& text, const CResRef& resRef, int flag)
{
    if (m_bInitialized == 0) return;
    m_pBarkBubble->SetBark(objId, text, resRef, flag);
    m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pBarkBubble), 4, 1);
}

bool CGuiInGame::HandleDialogReplyChosen(unsigned int a1, unsigned int a2, unsigned int a3, const CExoString& text,
                                          unsigned long long objId, unsigned char b, int c)
{
    K2_UNUSED(a1); K2_UNUSED(a2); K2_UNUSED(a3); K2_UNUSED(objId); K2_UNUSED(b); K2_UNUSED(c);
    CExoString localText = text;
#if KOTOR1  // K1: IsEmpty is out of line
    if (!text.IsEmpty()) {
#else
    if (text.m_sString != 0 && *text.m_sString != 0) {
#endif
        unsigned long long playerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerId);
        unsigned long long clientId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_gameObjectId);
        CGameObject* pGameObj;
        CSWTlkTable* pTlk;
        pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(clientId);
        pTlk = g_pSWTlkTable;
        pTlk->m_pSpeakerObj = pGameObj;
        pTlk->m_gender = pTlk->GetObjectGender();
        pTlk->m_bSuppressEnabled &= ~1;
        pTlk->ParseStr(localText);
    }
    return true;
}

void CGuiInGame::HandleDialogSelection(int index, int hasText)
{
    m_dialogSelectionActive = 0;
    if (index == -2) {
        CloseDialog();
        return;
    }

    unsigned int replyIndex;
    if (index < 0) {
        replyIndex = 0xffffffff;
    } else {
        replyIndex = static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->GetReplyIndex(index);
    }
    int isEndDialog = (index == -3);
    CSWCMessage msg;

    unsigned long long playerCreatureId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayerObj = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerCreatureId);
    unsigned long long serverOwnerId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
        static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->m_ownerId);
    CGameObject* pGameObj = g_pAppManager->m_pServerExoApp->GetGameObject(serverOwnerId);

    static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->SetDialogMessage(m_paReplyText[index]);

    if (hasText != 0)
        InitializeDialogAnimationData(m_paReplyFlags[index]);

    for (int i = 0; i < static_cast<int>(m_paReplyFlags[index]); i++)
        SetDialogAnimationData(i, static_cast<unsigned long long*>(m_paReplyAnimObjectIds[index])[i],
                               static_cast<unsigned short*>(m_paReplyAnimations[index])[i]);

    if (m_paReplyFadeTypes[index] != 0) {
        int fadeArg = 1;
        if (m_paReplyFadeTypes[index] == 1 || m_paReplyFadeTypes[index] == 4)
            fadeArg = 0;
        SetDialogFade(fadeArg, m_paReplyFadeDelays[index], m_paReplyFadeLengths[index], m_paReplyPositions[index]);
    }

    if (hasText != 0) {
        UnlockDialogParticipantOrientations();

        if (m_paReplyListenerIds[index] == OBJECT_INVALID) {
            m_oidPrevDialogParticipantB = m_dialogParticipantIdB;
            m_dialogParticipantIdB = m_dialogParticipantIdA;
        } else {
            m_oidPrevDialogParticipantB = m_dialogParticipantIdB;
            m_dialogParticipantIdB = m_paReplyListenerIds[index];
        }
        m_oidPrevDialogParticipantA = m_dialogParticipantIdA;
        m_dialogParticipantIdA = m_oidDialogPlayer;

        SetDialogAnimations();

        if (m_pActiveDialogPanel != 0 && m_pActiveDialogPanel == m_pDialogCinematicPanel) {
            if (!SetAnimatedCamera(m_paReplyCameraAnimations[index], m_paReplyFOVs[index])) {
                InitializeShotCamera();
                UpdateShotCamera(m_paReplyCameraAngles[index], m_paReplyCameraIds[index], m_paReplyHeightOffsets[index],
                                  m_paReplyTargetHeightOffsets[index], m_paReplyWaitFlags[index], m_paReplyCamVidEffects[index]);
            }
        } else if (m_paReplyCameraAngles[index] == 6) {
            InitializeShotCamera();
            SetComputerCameraShot(m_paReplyCameraIds[index], m_paReplyWaitFlags[index], m_paReplyCamVidEffects[index]);
#if !KOTOR1  // K1 has no ApplyCameraShot after the computer shot
            CSWBehaviorCameraDialog* pDialog =
                static_cast<CSWBehaviorCameraDialog*>(g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_DIALOG));
            pDialog->ApplyCameraShot();
#endif
        }
    }

    if (isEndDialog) {
        CloseDialog();
    }
    if (pGameObj != 0 && pGameObj->AsSWSObject() != 0) {
        pGameObj->AsSWSObject()->ReplyDialog(pPlayerObj->m_nPlayerId, replyIndex, isEndDialog,
                                              static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->m_nCurrentEntry, 0);
    }
}

bool CGuiInGame::HandleDialogEntry(unsigned int a1, unsigned long long objIdA, unsigned long long objIdC,
                                    unsigned long long objIdB, unsigned long long objIdD, unsigned char a6,
                                    const CExoString& text, unsigned int animCount, unsigned long long* paAnimData,
                                    unsigned short* paAnimFlags, unsigned int nCameraAngle, int charId,
                                    float floatC, float floatD, unsigned int arg, unsigned short cameraId,
                                    float speed, int effectFlag, unsigned char letterboxFadeType,
                                    const Vector& fadePos, float fadeB, float fadeC, int bForceText)
{
    K2_UNUSED(a1); K2_UNUSED(a6);
#if KOTOR1  // K1: GetInGameGui is re-fetched for each use
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog != 0) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog = 0;
        Vector zero = {0.0f, 0.0f, 0.0f};
        g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(1, 0.0f, 1.0f, zero);
    }
#else
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pInGameGui->m_bHoldFadeInForDialog != 0) {
        pInGameGui->m_bHoldFadeInForDialog = 0;
        Vector zero = {0.0f, 0.0f, 0.0f};
        pInGameGui->StartGlobalFade(1, 0.0f, 1.0f, zero);
    }
#endif

    if (InitializeDialogAnimationData(animCount)) {
        for (unsigned int i = 0; i != animCount; i++) {
            SetDialogAnimationData(i, paAnimData[i], paAnimFlags[i]);
        }
    }

    CExoString localText = text;

    unsigned long long playerCreatureId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerCreatureId);
    unsigned long long clientId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_gameObjectId);
    CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(clientId);
    CSWTlkTable* pTlk = g_pSWTlkTable;
    pTlk->m_pSpeakerObj = pGameObj;
    pTlk->m_gender = pTlk->GetObjectGender();
    pTlk->m_bSuppressEnabled &= ~1;
    pTlk->ParseStr(localText);

    localText = DialogNodeSpanningTextParse(localText);
    AppendToDialogBuffer(localText, objIdC);
    CleanUpReplyNode();

    CClientOptions* pOpts = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOpts->m_bFeedback2 == 0) {
        if (g_bDisableSound != 0) {
            m_bForceDialogText = 1;
        } else if (m_bAnimatedCut == 0) {
            int val = bForceText;
            if (m_pActiveDialogPanel == 0 || m_pActiveDialogPanel != m_pDialogCinematicPanel) {
                val = 1;
            }
            m_bForceDialogText = val;
        }
    }

    ShowDialogEntry(localText, objIdA, objIdC);
    UnlockDialogParticipantOrientations();

    if (m_oidDialogPlayer == OBJECT_INVALID) {
        m_oidDialogPlayer = objIdD;
        m_oidPrevDialogParticipantB = m_dialogParticipantIdB;
        if (objIdB != OBJECT_INVALID) {
            m_dialogParticipantIdB = objIdB;
        } else {
            m_dialogParticipantIdB = objIdD;
        }
    } else {
        if (objIdB != OBJECT_INVALID) {
            unsigned long long oldB = m_dialogParticipantIdB;
            m_dialogParticipantIdB = objIdB;
            m_oidPrevDialogParticipantB = oldB;
        } else {
            m_oidPrevDialogParticipantB = m_dialogParticipantIdB;
            if (m_dialogParticipantIdA != objIdC) {
                m_dialogParticipantIdB = m_dialogParticipantIdA;
            } else {
                m_dialogParticipantIdB = objIdD;
            }
        }
    }
    m_oidPrevDialogParticipantA = m_dialogParticipantIdA;
    m_dialogParticipantIdA = objIdC;

    SetDialogAnimations();

    if (letterboxFadeType != 0) {
        int flag;
        if (letterboxFadeType == 4 || letterboxFadeType == 1) {
            flag = 0;
        } else {
            flag = 1;
        }
        CSWGuiDialogLetterbox* pLetterbox = m_pDialogLetterbox;
        if (pLetterbox != 0) {
            pLetterbox->SetFade(flag, fadeB, fadeC, fadePos);
        }
    }

    if (m_pActiveDialogPanel == 0 || m_pActiveDialogPanel != m_pDialogCinematicPanel) {
        if (nCameraAngle == 6) {
            if (m_bDialogCameraSet == 0) {
                CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
                if (pModule != 0) {
                    pModule->SetDialogCamera(m_bOldHitCheck);
                }
                m_bDialogCameraSet = 1;
            }
            SetComputerCameraShot(charId, arg, effectFlag);
#if !KOTOR1  // K1 has no ApplyCameraShot after the computer shot
            CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
            CSWBehaviorCameraDialog* pDialog = static_cast<CSWBehaviorCameraDialog*>(pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_DIALOG));
            pDialog->ApplyCameraShot();
#endif
        }
    } else {
        if (!SetAnimatedCamera(cameraId, speed)) {
            if (m_bDialogCameraSet == 0) {
                CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
                if (pModule != 0) {
                    pModule->SetDialogCamera(m_bOldHitCheck);
                }
                m_bDialogCameraSet = 1;
            }
            UpdateShotCamera(nCameraAngle, charId, floatC, floatD, arg, effectFlag);
        }
    }

    return true;
}

void CGuiInGame::HideBarkBubble()
{
    if (m_bInitialized == 0) return;
    if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pBarkBubble)) != 0) {
        m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pBarkBubble));
    }
}

void CGuiInGame::SetPlaceableCamera(int index, int charId, const Vector& pos, const Quaternion& rot, float a, float b, float c, float d)
{
    if (index >= m_placeableCameraCount) return;
    CGuiPlaceableCamera* p = &m_placeableCameras[index];
    p->m_charId = charId;
    p->m_position = pos;
    p->m_rotation = rot;
    p->m_a = a;
    p->m_b = b;
    p->m_c = c;
    p->m_d = d;
}

int CGuiInGame::InitializeShotCamera()
{
    if (m_bDialogCameraSet == 0) {
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule != 0) {
            pModule->SetDialogCamera(m_bOldHitCheck);
        }
        m_bDialogCameraSet = 1;
    }
    return m_bDialogCameraSet;
}

void CGuiInGame::SetShotFromScript(int charId, int effectFlag)
{
    if (m_nGlobalDialogState == 0) return;

    if (m_pActiveDialogPanel == m_pDialogComputerPanel) {
        InitializeShotCamera();
        SetComputerCameraShot(charId, m_nWaitFlags, effectFlag);
#if !KOTOR1  // K1 has no ApplyCameraShot after the computer shot
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        CSWBehaviorCameraDialog* pDialog = static_cast<CSWBehaviorCameraDialog*>(pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_DIALOG));
        pDialog->ApplyCameraShot();
#endif
    } else {
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        CSWBehaviorCameraDialog* pDialog = static_cast<CSWBehaviorCameraDialog*>(pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_DIALOG));
        if (pDialog != 0) {
            int idx;
            bool bFound = false;
            for (idx = 0; idx < m_placeableCameraCount; idx++) {
                if (m_placeableCameras[idx].m_charId == charId) {
                    bFound = true;
                    break;
                }
            }
            Quaternion rot;
            if (bFound) {
                CGuiPlaceableCamera& cam = m_placeableCameras[idx];
                Vector pos = { cam.m_position.x, cam.m_position.y, cam.m_position.z + cam.m_b };
                rot = cam.m_rotation;
                float fYaw = Yaw(rot);
                float fPitch = Pitch(rot) + cam.m_a;
                float fRoll = Roll(rot);
                rot = YawPitchRoll(fYaw, fPitch, fRoll);
                pDialog->SetShot(6, pos, rot, cam.m_c);

                m_lastShotPos = pos;
                m_lastShotValid = 1;
                m_lastShotD = cam.m_d;

                if (static_cast<unsigned int>(effectFlag) < static_cast<unsigned int>(VIDEO_EFFECT_NONE_NO_CAPTION)) {
                    g_pAppManager->m_pClientExoApp->EnableVideoEffect(effectFlag);
                }
            }
        }
    }
}

void CGuiInGame::SetComputerCameraShot(int charId, unsigned int arg, int effectFlag)
{
    if (m_placeableCameraCount == 0) return;

#if !KOTOR1  // K1 has no live-feed caption
    CSWGuiDialogComputerCamera* pComputerCameraPanel = m_pDialogComputerCameraPanel;
    int bLiveFeedVisible;
    if (effectFlag == VIDEO_EFFECT_NONE_NO_CAPTION) {
        bLiveFeedVisible = 0;
    } else if (effectFlag == VIDEO_FFECT_SECURITY_NO_LABEL) {
        bLiveFeedVisible = 0;
    } else {
        bLiveFeedVisible = 1;
    }
    pComputerCameraPanel->SetLiveFeedMessageVisible(bLiveFeedVisible);
#endif
    m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pActiveDialogPanel));
    m_pGuiManager->AddPanel(m_pDialogComputerCameraPanel, 0, 1);
    m_pDialogComputerCameraPanel->SetOwner(static_cast<CSWGuiDialog*>(m_pActiveDialogPanel)->m_ownerId);

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    CSWBehaviorCameraDialog* pDialog = static_cast<CSWBehaviorCameraDialog*>(pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_DIALOG));
    if (pDialog != 0) {
        int idx;
        bool bFound = false;
        for (idx = 0; idx < m_placeableCameraCount; idx++) {
            if (m_placeableCameras[idx].m_charId == charId) {
                bFound = true;
                break;
            }
        }
        Vector pos;
        Quaternion rot;
        float fC, fD;
        if (bFound) {
            CGuiPlaceableCamera& cam = m_placeableCameras[idx];
            pos = { cam.m_position.x, cam.m_position.y, cam.m_position.z + cam.m_b };
            rot = cam.m_rotation;
            float fYaw = Yaw(rot);
            float fPitch = Pitch(rot) + cam.m_a;
            float fRoll = Roll(rot);
            rot = YawPitchRoll(fYaw, fPitch, fRoll);
            fC = cam.m_c;
            fD = cam.m_d;
        } else {
            pos = { 0.0f, 0.0f, 0.0f };
            rot.w = 1.0f;
            rot.x = 0.0f;
            rot.y = 0.0f;
            rot.z = 0.0f;
            fC = 55.0f;
            fD = 0.0f;
        }
        pDialog->SetShot(6, pos, rot, fC);

        m_lastShotPos = pos;
        m_lastShotValid = 1;
        m_lastShotD = fD;

        int iVar4 = (effectFlag != VIDEO_EFFECT_NONE) ? effectFlag : 0;
#if KOTOR1  // K1: -2 leaves the video effect alone
        if (iVar4 != -2) {
            g_pAppManager->m_pClientExoApp->EnableVideoEffect(iVar4);
        }
#ifdef K2_FIX_ENGINE_BUGS
        else {
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
        }
#endif
#else
        if (iVar4 == -2) {
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
        } else {
            g_pAppManager->m_pClientExoApp->EnableVideoEffect(iVar4);
        }
#endif
    }
    m_nWaitFlags = arg;
}

void CGuiInGame::HandleDialogClose(int flag)
{
    K2_UNUSED(flag);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->RemovePanel(static_cast<CSWGuiPanel*>(m_pActiveDialogPanel));
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->RemovePanel(static_cast<CSWGuiPanel*>(m_pDialogComputerCameraPanel));
}

int CGuiInGame::HandleDialogModels(unsigned int count, unsigned int count2, unsigned long long* paObjIds,
                                     CResRef* paResRefs, const CResRef& cameraResRef,
                                     const CResRef& ambientResRef)
{
    K2_UNUSED(count);
    InitializeStuntModelData(count2);
    for (unsigned int i = 0; i != count2; i++)
        SetStuntModelData(i, paObjIds[i], paResRefs[i]);
    if (!LoadStuntModelsForDialog()) return false;
    LoadCameraModelForDialog(cameraResRef);
    LoadAmbientTrackForDialog(ambientResRef);
    return true;
}

void CGuiInGame::ShowItemCreateMenu()
{
    if (m_bInitialized == 0) return;
    m_pGuiManager->AddPanel(m_pItemCreatePanel, 1, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}

void CGuiInGame::ShowLoadModuleDebugMenu()
{
    if (m_bInitialized == 0) return;
    m_pGuiManager->AddPanel(m_pLoadModuleDebugPanel, 1, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}

void CGuiInGame::ShowPowersFeatsSkillsDebugMenu()
{
    if (m_bInitialized == 0) return;
    m_pGuiManager->AddPanel(m_pPowersFeatsSkillsDebugPanel, 1, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}

void CGuiInGame::MarkNoClickEvent(unsigned int reason)
{
    CWorldTimer* pTimer = g_pAppManager->m_pClientExoApp->GetWorldTimer();
    pTimer->GetWorldTime(&m_nNoClickDay, &m_nNoClickTime);
    m_nNoClickDuration = reason;
}

int CGuiInGame::GetCanClick()
{
    unsigned int nowA;
    unsigned int nowB;
    unsigned int diffDay;
    unsigned int diffTod = 0;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nowA, &nowB);
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(nowA, nowB, m_nNoClickDay, m_nNoClickTime, &diffDay, &diffTod);
    if (diffDay == 0) {
        return diffTod >= m_nNoClickDuration;
    }
    return 1;
}

void CGuiInGame::ClearDialogNonDependantList()
{
    if (m_dialogNonDependantList.m_pInternal->m_pHead == 0) return;
    do {
        void* data = m_dialogNonDependantList.m_pInternal->RemoveHead();
        if (data != 0) {
            static_cast<CExoString*>(data)->~CExoString();
            operator delete(data);
        }
    } while (m_dialogNonDependantList.m_pInternal->m_pHead != 0);
}

#if KOTOR1
void CGuiInGame::UpdateMessageGUI()
{
    static_cast<CSWGuiInGameMessages*>(m_apScreenPanels[SCREEN_MESSAGES])->AddMessages(static_cast<CGuiInGameMessage*>(m_pMessageBuffer), m_messageBufferSize);
    static_cast<CSWGuiInGameMessages*>(m_apScreenPanels[SCREEN_MESSAGES])->AddDialogMessages(static_cast<CGuiInGameDialogMessage*>(m_pDialogMessageBuffer), m_dialogMessageBufferSize);
}
#else
void CGuiInGame::UpdateMessageGUI(CSWGuiPanel* pPanel)
{
    if (pPanel == 0) return;
    CSWGuiInGameMessages* p = static_cast<CSWGuiInGameMessages*>(pPanel);
    p->AddMessages(static_cast<CGuiInGameMessage*>(m_pMessageBuffer), m_messageBufferSize);
    p->AddDialogMessages(static_cast<CGuiInGameDialogMessage*>(m_pDialogMessageBuffer), m_dialogMessageBufferSize);
    p->AddCombatMessages(static_cast<CGuiInGameMessage*>(m_pCombatMessageBuffer), m_combatMessageBufferSize);
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::UpdateAbilitiesCharacter()
{
    if (m_apScreenPanels[SCREEN_ABILITIES] == 0) return;
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PanelExists(m_apScreenPanels[SCREEN_ABILITIES]) == 0) return;
    static_cast<CSWGuiInGameAbilities*>(m_apScreenPanels[SCREEN_ABILITIES])->UpdateCharacter();
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::UpdateInventory()
{
    if (m_apScreenPanels[SCREEN_INVENTORY] == 0) return;
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PanelExists(m_apScreenPanels[SCREEN_INVENTORY]) == 0) return;
    static_cast<CSWGuiInGameInventory*>(m_apScreenPanels[SCREEN_INVENTORY])->PopulateItemListBox();
}
#endif // !KOTOR1

int CGuiInGame::GetBarkBubbleVisible(int& out)
{
    if (m_pBarkBubble == 0) return 0;
    if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pBarkBubble)) == 0) return 0;
    out = m_pBarkBubble->m_extent.y + m_pBarkBubble->m_extent.h;
    return 1;
}

void CGuiInGame::FadeAndStopDialogAmbientTrack()
{
    if (m_pAmbientSound == 0) return;
    g_pAppManager->m_pClientExoApp->FadeAndStopStreamingSoundSource(m_pAmbientSound, 500);
    CExoStreamingSoundSource* p = m_pAmbientSound;
    if (p != 0) {
        p->~CExoStreamingSoundSource();
        operator delete(p);
    }
    m_pAmbientSound = 0;
}

#if !KOTOR1 || defined(K2_FIX_ENGINE_BUGS)  // K2-only (no K1 counterpart)
int CGuiInGame::GetGlobalFadeActive()
{
    if (m_pGlobalFade == 0) {
        goto isFalse;
    }
    if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pGlobalFade)) == 0) {
        goto isFalse;
    }
    if (m_pGlobalFade->m_fAlpha >= 0.01f) {
        return 1;
    }
isFalse:
    return 0;
}
#endif // !KOTOR1 || K2_FIX_ENGINE_BUGS

void CGuiInGame::ResetComputerCameraShot()
{
    if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pDialogComputerCameraPanel)) != 0) {
        m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pDialogComputerCameraPanel));
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pActiveDialogPanel), 2, 1);
#if KOTOR1  // K1: no override flag, the video effect goes off unconditionally
        g_pAppManager->m_pClientExoApp->DisableVideoEffect();
#else
        if (m_bScriptVideoEffect == 0) {
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
        }
#endif
    }
}

void CGuiInGame::LoadCameraModelForDialog(const CResRef& resRef)
{
    if (!resRef.IsValid()) return;
    if (m_bAnimatedCameraLoaded != 0) return;
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule == 0) return;
    pModule->LoadAnimatedCamera(resRef);
    m_bAnimatedCameraLoaded = 1;
}

void CGuiInGame::LoadAmbientTrackForDialog(const CResRef& resRef)
{
    CExoStreamingSoundSource* pAmbient = m_pAmbientSound;
    if (pAmbient != 0) {
        pAmbient->~CExoStreamingSoundSource();
        operator delete(pAmbient);
        m_pAmbientSound = 0;
    }
    if (resRef.IsValid()) {
        CExoStreamingSoundSource* pSound = new CExoStreamingSoundSource(1);
        m_pAmbientSound = pSound;
        pSound->SetPriorityGroup(PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS);
        m_pAmbientSound->Play(resRef, 8, 0, 1, 0);
    }
}

void CGuiInGame::EndDialogCamera()
{
    g_pAppManager->m_pClientExoApp->GetModule()->RestoreCamera();
    __builtin_memset(&m_bDialogCameraSet, 0, offsetof(CGuiInGame, _pad1060) - offsetof(CGuiInGame, m_bDialogCameraSet));
    m_startConversationCount = 0;
    m_nCameraAnimation = CAMERA_ANIMATION_NONE;
    m_oidDialogPlayer = OBJECT_INVALID;
    m_dialogParticipantIdB = OBJECT_INVALID;
    m_dialogParticipantIdA = OBJECT_INVALID;
    m_oidPrevDialogParticipantB = OBJECT_INVALID;
    m_oidPrevDialogParticipantA = OBJECT_INVALID;
}

void CGuiInGame::ShowSubItemCreateMenu(unsigned int flag)
{
    if (m_bInitialized == 0) return;
    m_pSubItemCreateMenuPanel->LoadItems(flag);
    m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pSubItemCreateMenuPanel), 1, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}

void CGuiInGame::SetStatusSummary(const CStatusSummary& in)
{
    if (m_pStatusSummary != 0) {
        m_pStatusSummary->SetSummary(in);
        return;
    }
    if (m_pStatusFallbackCache == 0) {
        m_pStatusFallbackCache = static_cast<CStatusSummary*>(operator new(sizeof(CStatusSummary)));
    }
    *m_pStatusFallbackCache = in;
}

void CGuiInGame::ShowStatusSummary()
{
    if (m_pStatusSummary == 0) {
        return;
    }
    if ((m_pStatusSummary->m_flags & CSWGuiStatusSummary::SUMMARY_DISPLAY_PENDING) != 0 &&
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFeedback5) {
        CSWGuiStatusSummary* pStatus = m_pStatusSummary;
        bool bPause;
        if (m_bPaused == 0) {
            bPause = g_pAppManager->m_pClientExoApp->GetTogglePauseAtEndOfFrame() != 0;
        } else {
            bPause = true;
        }
        pStatus->m_bTogglePauseAtEndOfFrame =
            static_cast<unsigned char>((pStatus->m_bTogglePauseAtEndOfFrame & ~1) | static_cast<int>(bPause));
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 6, 0);
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pStatusSummary), 1, 1);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 0);
        return;
    }
#if !KOTOR1  // K1: straight to DisplayStatus
    m_pStatusSummary->HandleAlignmentMovie();
    m_pStatusSummary->HandleLevelUpDLG();
    m_aReceivedItemNames.m_count = 0;
#endif
    m_pStatusSummary->DisplayStatus();
}

#if !KOTOR1  // K1 has no RemoveDeathMessage
void CGuiInGame::RemoveDeathMessage(bool removePanel)
{
    if (m_pDeathMessagePanel == 0) return;
    if (removePanel && m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pDeathMessagePanel)) != 0) {
        m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pDeathMessagePanel));
    }
    CSWGuiPanel* p = static_cast<CSWGuiPanel*>(m_pDeathMessagePanel);
    do {
        p->m_stateFlags = (p->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
        p = p->m_pNextInChain;
    } while (p != 0);
    m_pDeathMessagePanel = 0;
}
#endif // !KOTOR1

void CGuiInGame::SetPaused(int pause, unsigned char reason)
{
    if (m_bInitialized == 0) return;

    int result;
    if (pause == 0) {
        if (m_pPauseGuiPanel == 0) {
            result = 0;
            goto done;
        }
        result = 0;
        if (!m_pGuiManager->PanelExists(m_pPauseGuiPanel)) goto done;
    } else {
        result = 0;
        if (reason > CClientExoAppInternal::PAUSE_MODE_MINE_SIGHTED || ((1 << reason) & PAUSE_REASONS_WITH_BANNER) == 0) goto done;
        if (static_cast<CSWGuiInGamePause*>(m_pPauseGuiPanel) != 0) {
            static_cast<CSWGuiInGamePause*>(m_pPauseGuiPanel)->SetPauseReason(reason);
        }
        if (m_swGuiToggleState == SWGUI_STATE_SHOWN || g_pAppManager->m_pClientExoApp->GetMiniGame() != 0) {
            if (m_pPauseGuiPanel != 0 &&
                (m_pGuiManager->PanelExists(m_pPauseGuiPanel) == 0 && g_bInScreenshotMode == 0)) {
                result = 1;
                m_pGuiManager->AddPanel(m_pPauseGuiPanel, 4, 1);
#if KOTOR1  // K1: the pause panel goes to the back
                m_pGuiManager->SendPanelToBack(m_pPauseGuiPanel);
#endif
                goto done;
            }
        } else if (m_pPauseGuiPanel != 0 && m_pGuiManager->PanelExists(m_pPauseGuiPanel) != 0) {
            result = 1;
            goto toggle;
        }
        result = 1;
        goto done;
    }
toggle:
    {
        unsigned short flags = m_pPauseGuiPanel->m_stateFlags;
        unsigned short cleared = flags & ~PANEL_REMOVE_DEFERRED;
        unsigned short removeMode = flags & PANEL_REMOVE_MASK;
        unsigned short removeRequest = (flags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
        m_pPauseGuiPanel->m_stateFlags = (removeMode == PANEL_REMOVE_AND_DELETE) ? cleared : removeRequest;
    }
done:
    m_bPauseGuiShown = result;
    m_bPaused = pause;
#if !KOTOR1  // K1 has no Ios_minigamePaused
    Ios_minigamePaused = pause != 0;
#endif
}

void CGuiInGame::UpdateControllerPauseState()
{
    if (g_pExoInput->GetMainDeviceLost() != 0 && m_unpauseableRefCount == 0 && m_bControllerPauseActive == 0) {
        m_bInputActiveBeforeControllerPause = g_pExoInput->GetActive();
        g_pExoInput->SetActive(1);
        EnterControllerPauseState();
        m_bControllerPauseActive = 1;
    }
}

void CGuiInGame::ShowExamineBox(uint64_t objId, int flag)
{
    if (m_bInitialized == 0) return;
    m_pExamineBoxPanel->SetMessage(objId);
    m_pExamineBoxPanel->m_examineMode = flag;
    m_pGuiManager->AddPanel(m_pExamineBoxPanel, 1, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}

void CGuiInGame::ShowExamineBox(const CExoString& text, int flag)
{
    if (m_bInitialized == 0) return;
    m_pExamineBoxPanel->SetMessage(text);
    m_pExamineBoxPanel->m_examineMode = flag;
    m_pGuiManager->AddPanel(m_pExamineBoxPanel, 1, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}

void CGuiInGame::StoreLastDialogAnimationData()
{
    ClearLastDialogAnimationData();
    unsigned int count = m_dialogAnimCount;
    m_lastDialogAnimCount = count;
    if (count != 0) {
        m_paLastDialogAnimData = static_cast<unsigned long long*>(operator new[](static_cast<uint64_t>(count) << 3));
        m_paLastDialogAnimFlags = static_cast<unsigned short*>(operator new[](static_cast<uint64_t>(count) << 1));
        for (int i = 0; i < static_cast<int>(count); i++) {
            m_paLastDialogAnimData[i] = m_paDialogAnimData[i];
            m_paLastDialogAnimFlags[i] = m_paDialogAnimFlags[i];
        }
    }
}

bool CGuiInGame::InitializeDialogAnimationData(unsigned int count)
{
    StoreLastDialogAnimationData();
    m_dialogAnimCount = 0;
    if (m_paDialogAnimData != 0) {
        operator delete[](m_paDialogAnimData);
        m_paDialogAnimData = 0;
    }
    if (m_paDialogAnimFlags != 0) {
        operator delete[](m_paDialogAnimFlags);
        m_paDialogAnimFlags = 0;
    }
    if (count == 0) {
        return false;
    }
    m_dialogAnimCount = count;
    m_paDialogAnimData = static_cast<unsigned long long*>(operator new[](static_cast<uint64_t>(count) << 3));
    m_paDialogAnimFlags = static_cast<unsigned short*>(operator new[](static_cast<uint64_t>(count) << 1));
    return true;
}

int CGuiInGame::GetMiniMapVisible()
{
    CClientOptions* pOpts = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOpts->m_bFeedback3 == 0) {
        goto isFalse;
    }
    if (g_pAppManager->m_pClientExoApp->GetMiniGame() != 0) {
        goto isFalse;
    }
    if (m_pMainInterface == 0) {
        return true;
    }
    return m_pMainInterface->m_miniMap.m_tail.bVisible;
isFalse:
    return false;
}

void CGuiInGame::ShowContainerGui(unsigned long long objId, int flag)
{
    if (m_bInitialized == 0) return;
    m_pContainerGuiPanel->SetContainer(objId, flag);
    m_pGuiManager->AddPanel(m_pContainerGuiPanel, 1, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 3, 0);
}

#if !KOTOR1  // K1: CSWGuiMessageBox::DisplayDeathMessage instead
void CGuiInGame::DisplayDeathMessage()
{
    if (m_pDeathMessagePanel == 0) {
        m_pDeathMessagePanel = new CSWGuiDeathDisplay(m_pGuiManager);
    }
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 0);
    m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pDeathMessagePanel), 1, 1);
}
#endif // !KOTOR1

void CGuiInGame::EndControllerPauseState()
{
    m_bControllerPauseActive = 0;
#if !KOTOR1  // K1: the input queue is not flushed
    g_pExoInput->ClearEvents();
#endif
    g_pExoInput->SetActive(m_bInputActiveBeforeControllerPause == 1);
    if (g_pAppManager != 0 && g_pAppManager->m_pServerExoApp != 0 && m_bPausedBeforeControllerPause == 0) {
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 1);
    }
}

void CGuiInGame::HideContainerGui()
{
    if (m_bInitialized == 0) return;
    if (m_pGuiManager->PanelExists(m_pContainerGuiPanel) == 0) return;
    m_pGuiManager->PopModalPanel();
    m_pGuiManager->RemovePanel(m_pContainerGuiPanel);
#if KOTOR1  // K1: input and pause are left alone while the dialog panel is up
    if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pDialogCinematicPanel)) != 0) return;
#endif
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
    g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
}

void CGuiInGame::SetConversationType(unsigned int type, unsigned char flag)
{
    if (type == 1) {
        if (m_pDialogComputerPanel == 0) {
            m_pDialogComputerPanel = new CSWGuiDialogComputer(m_pGuiManager);
        }
        m_pActiveDialogPanel = m_pDialogComputerPanel;
        m_pDialogComputerPanel->SetType(flag);
        return;
    }
    m_pActiveDialogPanel = m_pDialogCinematicPanel;
}

void CGuiInGame::HideSubItemCreateMenu()
{
    if (m_bInitialized == 0) return;
    if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pSubItemCreateMenuPanel)) == 0) return;
    m_pGuiManager->PopModalPanel();
    m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pSubItemCreateMenuPanel));
    int inputClass = (g_pAppManager->m_pClientExoApp->GetMiniGame() != 0) ? 1 : 0;
    g_pAppManager->m_pClientExoApp->SetInputClass(inputClass, 1);
    m_pSubItemCreateMenuPanel->ClearItems();
}

void CGuiInGame::HideExamineBox()
{
    if (m_bInitialized == 0) return;
    if (m_pGuiManager->PanelExists(m_pExamineBoxPanel) == 0) return;
    m_pGuiManager->PopModalPanel();
    m_pGuiManager->RemovePanel(m_pExamineBoxPanel);
    if (m_pExamineBoxPanel->m_examineMode == 1) {
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
        return;
    }
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::HideCreateItemScreen()
{
    if (m_bInitialized == 0) return;
    if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pSubItemCreateMenuPanel)) == 0) return;
    m_pGuiManager->PopModalPanel();
    m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pSubItemCreateMenuPanel));
    int inputClass = (g_pAppManager->m_pClientExoApp->GetMiniGame() != 0) ? 1 : 0;
    g_pAppManager->m_pClientExoApp->SetInputClass(inputClass, 1);
    m_pSubItemCreateMenuPanel->ClearItems();
}
#endif // !KOTOR1

void CGuiInGame::EnterControllerPauseState()
{
    void* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    CSWGuiPanel* pPanel = static_cast<CGuiInGame*>(pInGameGui)->m_pControllerPausePanel;
    if (pPanel != 0) {
        m_pGuiManager->AddPanel(pPanel, 1, 1);
    }
    if (g_pAppManager != 0 && g_pAppManager->m_pServerExoApp != 0) {
        int paused = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
        m_bPausedBeforeControllerPause = paused;
        if (paused == 0) {
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 0, 1);
        }
    }
}

void CGuiInGame::HideItemCreateMenu()
{
    if (m_bInitialized == 0) return;
    if (m_pGuiManager->PanelExists(m_pItemCreatePanel) == 0) return;
    m_pGuiManager->PopModalPanel();
    m_pGuiManager->RemovePanel(m_pItemCreatePanel);
    if (g_pAppManager->m_pClientExoApp->GetMiniGame() != 0) {
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_MINIGAME, 1);
        return;
    }
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
}

void CGuiInGame::HideLoadModuleDebugMenu()
{
    if (m_bInitialized == 0) return;
    if (m_pGuiManager->PanelExists(m_pLoadModuleDebugPanel) == 0) return;
    m_pGuiManager->PopModalPanel();
    m_pGuiManager->RemovePanel(m_pLoadModuleDebugPanel);
    if (g_pAppManager->m_pClientExoApp->GetMiniGame() != 0) {
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_MINIGAME, 1);
        return;
    }
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
}

void CGuiInGame::HidePowersFeatsSkillsDebugMenu()
{
    if (m_bInitialized == 0) return;
    if (m_pGuiManager->PanelExists(m_pPowersFeatsSkillsDebugPanel) == 0) return;
    m_pGuiManager->PopModalPanel();
    m_pGuiManager->RemovePanel(m_pPowersFeatsSkillsDebugPanel);
    if (g_pAppManager->m_pClientExoApp->GetMiniGame() != 0) {
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_MINIGAME, 1);
        return;
    }
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
}

int CGuiInGame::TutorialReasonWillShowWindow(unsigned char reason)
{
#if KOTOR1  // K1: the shown test is GetTutorialWindowHasBeenShown
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bTutorialWindows == 0 ||
        GetTutorialWindowHasBeenShown(reason) != 0 || m_nGlobalDialogState != 0) {
        return false;
    }
    return m_pTutorialBox->TutorialReasonIsValid(reason) != 0;
#else
    unsigned char byteIndex = reason >> 3;
    unsigned char bitIndex = reason & 7;
    CClientOptions* pOpts = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOpts->m_bTutorialWindows == 0) {
        goto isFalse;
    }
    if (((1 << bitIndex) & m_tutorialWindowShown[byteIndex]) != 0) {
        goto isFalse;
    }
    if (m_nGlobalDialogState != 0) {
        goto isFalse;
    }
    return m_pTutorialBox->TutorialReasonIsValid(reason) != 0;
isFalse:
    return false;
#endif
}

bool CGuiInGame::ShowTutorialWindow(unsigned char reason, unsigned long long instigatorId,
                                     unsigned long long targetId, unsigned int type)
{
    CClientOptions* pOpts = g_pAppManager->m_pClientExoApp->GetClientOptions();
#if KOTOR1  // K1: higher ids are refused
    if (reason > 42) {
        return false;
    }
    if (pOpts->m_bTutorialWindows == 0) {
        return false;
    }
    if (GetTutorialWindowHasBeenShown(reason) != 0) {
        return false;
    }
    unsigned char byteIndex = reason >> 3;
    unsigned char bitIndex = reason & 7;
#else
    if (pOpts->m_bTutorialWindows == 0) {
        return false;
    }
    unsigned char byteIndex = reason >> 3;
    unsigned char bitIndex = reason & 7;
    if (((1 << bitIndex) & m_tutorialWindowShown[byteIndex]) != 0) {
        return false;
    }
#endif

    m_tutorialWindowShown[byteIndex] |= (1 << bitIndex);
    m_pTutorialBox->SetTutorialInstigator(instigatorId, targetId, type);

    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PanelExists(m_pTutorialBox) != 0) {
        return true;
    }

    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(m_pTutorialBox, 1, 1);
    return true;
}

void CGuiInGame::HideUpgradeScreen()
{
    if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pUpgradeScreenPanel != 0) {
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
        CSWGuiPanel* p = m_pUpgradeScreenPanel;
#if KOTOR1  // K1: no panel chain, one stamp
        p->m_stateFlags = (p->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
#else
        do {
            p->m_stateFlags = (p->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
            p = p->m_pNextInChain;
        } while (p != 0);
#endif
        m_pUpgradeScreenPanel = 0;
    }
}

#if !KOTOR1  // K1 has no HideChemicalUpgradeScreen
void CGuiInGame::HideChemicalUpgradeScreen()
{
    if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pChemicalUpgradeScreenPanel != 0) {
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
        CSWGuiPanel* p = static_cast<CSWGuiPanel*>(m_pChemicalUpgradeScreenPanel);
        do {
            p->m_stateFlags = (p->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
            p = p->m_pNextInChain;
        } while (p != 0);
        m_pChemicalUpgradeScreenPanel = 0;
    }
}
#endif // !KOTOR1

#if !KOTOR1  // K1 has no HideSwoopUpgradeScreen
void CGuiInGame::HideSwoopUpgradeScreen()
{
    if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pSwoopUpgradeScreenPanel != 0) {
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
        CSWGuiPanel* p = static_cast<CSWGuiPanel*>(m_pSwoopUpgradeScreenPanel);
        do {
            p->m_stateFlags = (p->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
            p = p->m_pNextInChain;
        } while (p != 0);
        m_pSwoopUpgradeScreenPanel = 0;
    }
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::ShowDemoScreen(CExoString name, int a, int b, int c, int d)
{
    if (m_bInitialized == 0 || m_pGuiManager == 0 || m_pDemoScreenPanel != 0) return;

#if KOTOR1
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
#else
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
#endif
    if (m_bPaused == 0) {
        g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
    }
    CSWDemoScreen* pDemoScreen = new CSWDemoScreen(m_pGuiManager, a, b, c, d);
    m_pDemoScreenPanel = pDemoScreen;
    m_pGuiManager->AddPanel(m_pDemoScreenPanel, 3, 1);
    CSWDemoScreen* pDemoScreen2 = static_cast<CSWDemoScreen*>(m_pDemoScreenPanel);
    CResRef resRef(static_cast<char*>(name.CStr()));
    pDemoScreen2->SetImage(resRef);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}
#endif // !KOTOR1

#if !KOTOR1  // K1 has no HideDemoScreen
void CGuiInGame::HideDemoScreen()
{
    if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pDemoScreenPanel != 0) {
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
        CSWGuiPanel* p = m_pDemoScreenPanel;
        do {
            p->m_stateFlags = (p->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
            p = p->m_pNextInChain;
        } while (p != 0);
        m_pDemoScreenPanel = 0;
    }
}
#endif // !KOTOR1

void CGuiInGame::ResetInterfaceFont()
{
    if (m_pMainInterface != 0) static_cast<CSWGuiPanel*>(m_pMainInterface)->ResetFont();
    if (m_apScreenPanels[0] != 0) m_apScreenPanels[0]->ResetFont();
    if (m_apScreenPanels[1] != 0) m_apScreenPanels[1]->ResetFont();
    if (m_apScreenPanels[2] != 0) m_apScreenPanels[2]->ResetFont();
    if (m_apScreenPanels[3] != 0) m_apScreenPanels[3]->ResetFont();
    if (m_apScreenPanels[4] != 0) m_apScreenPanels[4]->ResetFont();
    if (m_apScreenPanels[5] != 0) m_apScreenPanels[5]->ResetFont();
    if (m_apScreenPanels[6] != 0) m_apScreenPanels[6]->ResetFont();
    if (m_apScreenPanels[7] != 0) m_apScreenPanels[7]->ResetFont();
}

void CGuiInGame::RemoveGUIPopUps()
{
#if KOTOR1  // K1: nothing to do without a GUI manager
    if (m_pGuiManager == 0) return;
#endif
    if (m_pGuiManager->PanelExists(m_pSoloModeQueryBox) != 0) {
        m_pSoloModeQueryBox->HandleInputEvent(INPUTEVENT_GUI_B, 1);
    }
    if (m_pGuiManager->PanelExists(m_pTutorialBox) != 0) {
        m_pTutorialBox->HandleInputEvent(INPUTEVENT_GUI_B, 1);
    }
    if (m_pGuiManager->PanelExists(m_pSoloModePanel) != 0) {
        m_pSoloModePanel->HandleInputEvent(INPUTEVENT_GUI_B, 1);
    }
    if (m_pGuiManager->PanelExists(m_pContainerGuiPanel) != 0) {
        m_pContainerGuiPanel->HandleInputEvent(INPUTEVENT_GUI_B, 1);
    }
#if !KOTOR1
    if (m_pGuiManager->PanelExists(m_pDatapadMessageBox) != 0) {
        m_pDatapadMessageBox->HandleInputEvent(INPUTEVENT_GUI_B, 1);
    }
#endif
}

void CGuiInGame::HideSoloMode()
{
    if (m_pGuiManager->PanelExists(m_pSoloModePanel) != 0) {
        if (m_bPaused == 0) {
#if KOTOR1  // K1: unpause through the server toggle-pause request
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
#else
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
#endif
        }
        m_pGuiManager->PopModalPanel();
        unsigned short flags = m_pSoloModePanel->m_stateFlags;
        unsigned short a = flags & ~PANEL_REMOVE_DEFERRED;
        unsigned short cond = flags & PANEL_REMOVE_MASK;
        unsigned short b = (flags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
        m_pSoloModePanel->m_stateFlags = (cond == PANEL_REMOVE_AND_DELETE) ? a : b;
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
    }
}

int CGuiInGame::GetFadeOverridePanelVisible()
{
    if (m_bInitialized == 0) return false;
    if (m_pGuiManager == 0) return false;
    if (m_pGalaxyMapPanel != 0) {
        if (m_pGuiManager->PanelExists(m_pGalaxyMapPanel) != 0) {
            if ((*reinterpret_cast<unsigned char*>(&m_pGalaxyMapPanel->m_stateFlags) & 4) != 0) return true;
        }
    }
    if (m_pMessagesPanel != 0) {
        if (m_pGuiManager->PanelExists(m_pMessagesPanel) != 0) {
            if ((*reinterpret_cast<unsigned char*>(&m_pMessagesPanel->m_stateFlags) & 4) != 0) return true;
        }
    }
    if (m_pStorePanel != 0) {
        if (m_pGuiManager->PanelExists(m_pStorePanel) != 0) {
            if ((*reinterpret_cast<unsigned char*>(&m_pStorePanel->m_stateFlags) & 4) != 0) return true;
        }
    }
    if (m_pInGameMenu != 0) {
        if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pInGameMenu)) != 0) {
            if ((*reinterpret_cast<unsigned char*>(&static_cast<CSWGuiPanel*>(m_pInGameMenu)->m_stateFlags) & 4) != 0) return true;
        }
    }
    return g_pAppManager->m_pClientExoApp->GetPazaakIsRunning() != 0;
}

int CGuiInGame::IsAnimationPlayingInDialog()
{
#if KOTOR1  // K1: true when any participant plays a dialog animation
    for (unsigned int i = 0; i < m_dialogAnimCount; i++) {
        CSWCObject* pSWC = static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(m_paDialogAnimData[i])->AsSWCCreature());
        if (pSWC != 0 && pSWC->AnimationDialog(pSWC->m_currentAnimation) != 0) return true;
    }
    return false;
#else
    unsigned int i, count;
    int found;
    if (m_dialogAnimCount == 0) {
        found = 0;
        i = 0;
        count = 0;
    } else {
        found = 0;
        i = 0;
        do {
            CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_paDialogAnimData[i]);
            CSWCObject* pSWC = static_cast<CSWCObject*>(pObj->AsSWCCreature());
            if (pSWC != 0) {
                int result = pSWC->AnimationDialog(pSWC->m_currentAnimation);
                if (result == 0) return false;
                found = result;
            }
            count = m_dialogAnimCount;
            i = i + 1;
        } while (i < count);
    }
    return (i != count) ? found : 1;
#endif
}

void CGuiInGame::UnlockDialogParticipantOrientations()
{
    CGameObject* pObjA = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogParticipantIdA);
    if (pObjA != 0 && pObjA->AsSWCCreature() != 0) {
        CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObjA->AsSWCCreature());
        pCreature->SetLockOrientationToObject(OBJECT_INVALID, 0);
    }
    CGameObject* pObjB = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogParticipantIdB);
    if (pObjB != 0 && pObjB->AsSWCCreature() != 0) {
        CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObjB->AsSWCCreature());
        pCreature->SetLockOrientationToObject(OBJECT_INVALID, 0);
    }
}

void CGuiInGame::StartGlobalFade(int a, float b, float c, const Vector& d)
{
#if KOTOR1
    // K1: no demo-screen or override gates, and the fade is re-added without a PanelExists test
    if (m_pGlobalFade != 0) {
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pGlobalFade), 4, 1);
        static_cast<CSWGuiPanel*>(m_pGlobalFade)->SetVisible(1);
        m_pGlobalFade->SetFade(a, b, c, d);
    }
#else
    if ((m_pDemoScreenPanel == 0 || static_cast<CSWDemoScreen*>(m_pDemoScreenPanel)->m_bSuppressGlobalFade == 0) &&
        m_pGlobalFade != 0 && m_bFadeUntilScript == 0) {
        if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pGlobalFade)) == 0) {
            m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pGlobalFade), 0, 1);
        }
        static_cast<CSWGuiPanel*>(m_pGlobalFade)->SetVisible(1);
        m_pGlobalFade->SetFade(a, b, c, d);
    }
#endif
}

int CGuiInGame::IsOnDialogNonDependantList(const CExoString& text)
{
    if (text.CompareNoCase("") != 0) return false;
    void* pData = m_dialogNonDependantList.m_pInternal->m_pHead ? m_dialogNonDependantList.m_pInternal->m_pHead->m_pData : 0;
    CExoLinkedListNode* pos = m_dialogNonDependantList.m_pInternal->m_pHead;
    if (pos == 0) return false;
    do {
        const char* str = static_cast<CExoString*>(pData)->CStr();
        if (text.CompareNoCase(str) != 0) return true;
        pData = m_dialogNonDependantList.m_pInternal->GetNext(pos);
    } while (pos != 0);
    return false;
}

void CGuiInGame::AddToDialogNonDependantList(const CExoString& text)
{
#if KOTOR1  // K1: IsEmpty is out of line
    if (text.CompareNoCase("") == 0 && !text.IsEmpty()) {
#else
    if (text.CompareNoCase("") == 0 && text.m_sString != 0 && *text.m_sString != '\0') {
#endif
        if (!IsOnDialogNonDependantList(text)) {
            CExoString* pNew = new CExoString();
            *pNew = text;
            m_dialogNonDependantList.m_pInternal->AddHead(pNew);
        }
    }
}

void CGuiInGame::ShowGalaxyMapGui(int planetIndex)
{
    if ((m_bInitialized != 0) && (m_bInGameMenuShown == 0)) {
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0) {
            if (g_pAppManager->m_pClientExoApp->GetPausedByCombat() == 0) {
                g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 0, 0);
            }
            g_pAppManager->m_pClientExoApp->IndicateObject(0, 0);
#if KOTOR1
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
#else
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
#endif
            SetSWGuiStatus(SWGUI_COVER, 1);
            m_pBarkBubble->SetBark(OBJECT_INVALID, CExoString(""), CResRef(""), 0);
            g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
            if (m_pGalaxyMapPanel == 0) {
                m_pGalaxyMapPanel = new CSWGuiInGameGalaxyMap(m_pGuiManager);
            }
            CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
            pPartyTable->SetSelectedPlanet(planetIndex);
            m_pGuiManager->AddPanel(m_pGalaxyMapPanel, 2, 1);
            m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_ADDED);
        }
    }
}

void CGuiInGame::ClearRepliesData()
{
    if (m_paReplyText != 0) {
        delete[] m_paReplyText;
        m_paReplyText = 0;
    }
    if (m_paReplyLinkIndices != 0) {
        operator delete[](m_paReplyLinkIndices);
        m_paReplyLinkIndices = 0;
    }
    if (m_paReplyListenerIds != 0) {
        operator delete[](m_paReplyListenerIds);
        m_paReplyListenerIds = 0;
    }
    if (m_paReplyCameraAngles != 0) {
        operator delete[](m_paReplyCameraAngles);
        m_paReplyCameraAngles = 0;
    }
    if (m_paReplyCameraIds != 0) {
        operator delete[](m_paReplyCameraIds);
        m_paReplyCameraIds = 0;
    }
    if (m_paReplyHeightOffsets != 0) {
        operator delete[](m_paReplyHeightOffsets);
        m_paReplyHeightOffsets = 0;
    }
    if (m_paReplyTargetHeightOffsets != 0) {
        operator delete[](m_paReplyTargetHeightOffsets);
        m_paReplyTargetHeightOffsets = 0;
    }
    if (m_paReplyWaitFlags != 0) {
        operator delete[](m_paReplyWaitFlags);
        m_paReplyWaitFlags = 0;
    }
    if (m_paReplyCameraAnimations != 0) {
        operator delete[](m_paReplyCameraAnimations);
        m_paReplyCameraAnimations = 0;
    }
    if (m_paReplyFOVs != 0) {
        operator delete[](m_paReplyFOVs);
        m_paReplyFOVs = 0;
    }
    if (m_paReplyCamVidEffects != 0) {
        operator delete[](m_paReplyCamVidEffects);
        m_paReplyCamVidEffects = 0;
    }
    if (m_paReplyFadeTypes != 0) {
        operator delete[](m_paReplyFadeTypes);
        m_paReplyFadeTypes = 0;
    }
    if (m_paReplyPositions != 0) {
        operator delete[](m_paReplyPositions);
        m_paReplyPositions = 0;
    }
    if (m_paReplyFadeDelays != 0) {
        operator delete[](m_paReplyFadeDelays);
        m_paReplyFadeDelays = 0;
    }
    if (m_paReplyFadeLengths != 0) {
        operator delete[](m_paReplyFadeLengths);
        m_paReplyFadeLengths = 0;
    }
    for (int i = 0; i < static_cast<int>(m_repliesCount); i++) {
        if (m_paReplyFlags[i] != 0) {
            void* p1 = m_paReplyAnimObjectIds[i];
            void* p2 = m_paReplyAnimations[i];
            if (p1 != 0) operator delete[](p1);
            if (p2 != 0) operator delete[](p2);
        }
    }
    if (m_paReplyAnimObjectIds != 0) {
        operator delete[](m_paReplyAnimObjectIds);
        m_paReplyAnimObjectIds = 0;
    }
    if (m_paReplyAnimations != 0) {
        operator delete[](m_paReplyAnimations);
        m_paReplyAnimations = 0;
    }
    if (m_paReplyFlags != 0) {
        operator delete[](m_paReplyFlags);
        m_paReplyFlags = 0;
    }
    m_repliesCount = 0;
}

#if KOTOR1  // K1: returns 0
int CGuiInGame::InitializeRepliesData(unsigned int count)
#else
void CGuiInGame::InitializeRepliesData(unsigned int count)
#endif
{
    ClearRepliesData();
    m_repliesCount = count;
    m_paReplyText = new CExoString[count];
    unsigned int n = m_repliesCount;
    uint64_t sz4 = static_cast<uint64_t>(n) << 2;
    m_paReplyLinkIndices = static_cast<unsigned int*>(operator new[](sz4));
    uint64_t sz8 = static_cast<uint64_t>(n) << 3;
    m_paReplyListenerIds = static_cast<unsigned long long*>(operator new[](sz8));
    m_paReplyCameraAngles = static_cast<unsigned int*>(operator new[](sz4));
    m_paReplyCameraIds = static_cast<int*>(operator new[](sz4));
    m_paReplyHeightOffsets = static_cast<float*>(operator new[](sz4));
    m_paReplyTargetHeightOffsets = static_cast<float*>(operator new[](sz4));
    m_paReplyWaitFlags = static_cast<unsigned int*>(operator new[](sz4));
    m_paReplyCameraAnimations = static_cast<unsigned short*>(operator new[](static_cast<uint64_t>(n) << 1));
    m_paReplyFOVs = static_cast<float*>(operator new[](sz4));
    m_paReplyCamVidEffects = static_cast<int*>(operator new[](sz4));
    m_paReplyFadeTypes = static_cast<unsigned char*>(operator new[](static_cast<uint64_t>(n)));
    uint64_t szVec = static_cast<uint64_t>(n) * sizeof(Vector);
    Vector* pVecArr = static_cast<Vector*>(operator new[](szVec));
    if (n != 0) {
        memset(pVecArr, 0, ((szVec - sizeof(Vector)) / sizeof(Vector)) * sizeof(Vector) + sizeof(Vector));
    }
    m_paReplyPositions = pVecArr;
    m_paReplyFadeDelays = static_cast<float*>(operator new[](sz4));
    m_paReplyFadeLengths = static_cast<float*>(operator new[](sz4));
    m_paReplyFlags = static_cast<unsigned int*>(operator new[](sz4));
    m_paReplyAnimObjectIds = static_cast<void**>(operator new[](sz8));
    m_paReplyAnimations = static_cast<void**>(operator new[](sz8));
#if KOTOR1
    return 0;
#endif
}

void CGuiInGame::SetReplyData(unsigned int index, const CExoString& text, unsigned int a, unsigned long long b,
                               unsigned int c, int d, float e, float f, unsigned int g, unsigned short h,
                               float i, int j, unsigned char k, const Vector& pos, float m, float n,
                               unsigned int count, unsigned long long* paU, unsigned short* paT)
{
    if (index < m_repliesCount) {
        m_paReplyText[index] = text;
        m_paReplyLinkIndices[index] = a;
        m_paReplyListenerIds[index] = b;
        m_paReplyCameraAngles[index] = c;
        m_paReplyCameraIds[index] = d;
        m_paReplyHeightOffsets[index] = e;
        m_paReplyTargetHeightOffsets[index] = f;
        m_paReplyWaitFlags[index] = g;
        m_paReplyCameraAnimations[index] = h;
        m_paReplyFOVs[index] = i;
        m_paReplyCamVidEffects[index] = j;
        m_paReplyFadeTypes[index] = k;
        m_paReplyPositions[index] = pos;
        m_paReplyFadeDelays[index] = m;
        m_paReplyFadeLengths[index] = n;
        m_paReplyFlags[index] = count;
        if (count == 0) {
            m_paReplyAnimObjectIds[index] = 0;
            m_paReplyAnimations[index] = 0;
        } else {
            m_paReplyAnimObjectIds[index] = operator new[](static_cast<uint64_t>(count) << 3);
            m_paReplyAnimations[index] = operator new[](static_cast<uint64_t>(count) << 1);
            for (int idx2 = 0; idx2 < static_cast<int>(count); idx2++) {
                static_cast<unsigned long long*>(m_paReplyAnimObjectIds[index])[idx2] = paU[idx2];
                static_cast<unsigned short*>(m_paReplyAnimations[index])[idx2] = paT[idx2];
            }
        }
    }
}

bool CGuiInGame::HandleDialogReplies(unsigned int a1, unsigned long long a2, unsigned char a3,
                                      CExoString* paText, unsigned int* paA, unsigned long long* paB,
                                      unsigned int* paC, int* paD, float* paE, float* paF, unsigned int* paG,
                                      unsigned short* paH, float* paI, int* paJ, unsigned char* paK,
                                      Vector* paPos, float* paM, float* paN, unsigned int a4, unsigned int a5,
                                      unsigned int a6, unsigned int* paCount, unsigned long long** paU,
                                      unsigned short** paT, int a7)
{
    K2_UNUSED(a1); K2_UNUSED(a2); K2_UNUSED(a3); K2_UNUSED(a7);
    unsigned int count = a5 + a4;
    if (count != 0) {
        CExoString text;
        InitializeRepliesData(count);
        for (unsigned int i = 0; i < count; i++) {
            text = paText[i];
#if KOTOR1  // K1: IsEmpty; the parsed reply is not trimmed
            if (!text.IsEmpty()) {
#else
            if (text.m_sString != 0 && *text.m_sString != 0) {
#endif
                unsigned long long playerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
                CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerId);
                unsigned long long clientId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_gameObjectId);
                CGameObject* pGameObj = g_pAppManager->m_pClientExoApp->GetGameObject(clientId);
                CSWTlkTable* pTlk = g_pSWTlkTable;
                pTlk->m_pSpeakerObj = pGameObj;
                pTlk->m_gender = pTlk->GetObjectGender();
                pTlk->m_bSuppressEnabled &= ~1;
                pTlk->ParseStr(text);
#if !KOTOR1
                while (text.m_sString != 0) {
                    if (static_cast<int>(strlen(text.m_sString)) < 1) break;
                    if (text[0] != ' ') break;
                    int len = text.m_sString ? static_cast<int>(strlen(text.m_sString)) : 0;
                    text = text.SubString(1, len - 1);
                }
                // An empty reply is not trimmed either.
                while (text.m_sString != 0) {
                    if (static_cast<int>(strlen(text.m_sString)) < 1) break;
                    int lenA = text.m_sString ? static_cast<int>(strlen(text.m_sString)) : 0;
                    if (text[lenA - 1] != ' ') break;
                    int lenB = text.m_sString ? static_cast<int>(strlen(text.m_sString)) : 0;
                    text = text.SubString(0, lenB - 1);
                }
#endif
            }
            SetReplyData(i, text, paA[i], paB[i], paC[i], paD[i], paE[i], paF[i], paG[i], paH[i],
                         paI[i], paJ[i], paK[i], paPos[i], paM[i], paN[i], paCount[i], paU[i], paT[i]);
        }
    }
    CleanUpEntryNode();
    ShowDialogReplies(a5, a6);
    return true;
}

void CGuiInGame::HideGalaxyMapGui()
{
    if (m_bInitialized != 0) {
        if (g_pAppManager->m_pClientExoApp->GetPausedByCombat() != 0) {
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
        }
        SetSWGuiStatus(SWGUI_UNCOVER, 1);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
        m_pGuiManager->RemovePanel(m_pGalaxyMapPanel);
        if (g_bCreateAllGUIs == 0) {
            if (m_pGalaxyMapPanel != 0) delete m_pGalaxyMapPanel;
            m_pGalaxyMapPanel = 0;
        }
        m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_REMOVED);
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
    }
}

void CGuiInGame::ShowStoreGui(unsigned long long a, unsigned long long b)
{
    if ((m_bInitialized != 0) && (m_bInGameMenuShown == 0) && (m_nGlobalDialogState == 0)) {
        if (g_pAppManager->m_pClientExoApp->GetPausedByCombat() == 0) {
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 0, 0);
        }
        g_pAppManager->m_pClientExoApp->IndicateObject(0, 0);
        SetSWGuiStatus(SWGUI_COVER, 1);
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
#endif
        m_pBarkBubble->SetBark(OBJECT_INVALID, CExoString(""), CResRef(""), 0);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
        if (m_pStorePanel == 0) {
            m_pStorePanel = new CSWGuiStore(m_pGuiManager);
        }
        m_pStorePanel->m_oidStore = a;
        m_pStorePanel->m_oidPlayer = b;
        m_pGuiManager->AddPanel(m_pStorePanel, 3, 1);
        m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_ADDED);
    }
}

void CGuiInGame::HideStoreGui()
{
    if (m_bInitialized != 0) {
        if (g_pAppManager->m_pClientExoApp->GetPausedByCombat() != 0) {
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
        }
        SetSWGuiStatus(SWGUI_UNCOVER, 1);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
        if (g_bCreateAllGUIs == 0) {
            CSWGuiPanel* p = m_pStorePanel;
#if KOTOR1  // K1: no panel chain, one stamp
            p->m_stateFlags = (p->m_stateFlags & ~PANEL_REMOVE_MASK) | PANEL_REMOVE_AND_DELETE | PANEL_REMOVE_DEFERRED;
#else
            do {
                p->m_stateFlags = (p->m_stateFlags & ~PANEL_REMOVE_MASK) | PANEL_REMOVE_AND_DELETE | PANEL_REMOVE_DEFERRED;
                p = p->m_pNextInChain;
            } while (p != 0);
#endif
            m_pStorePanel = 0;
        } else {
            m_pGuiManager->RemovePanel(m_pStorePanel);
        }
        m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_REMOVED);
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
    }
}

void CGuiInGame::AppendToDialogBuffer(const CExoString& text, const CExoString& speaker)
{
    if (text != "") {
        unsigned int size = m_dialogMessageBufferSize;
        if (size > MESSAGE_BUFFER_SIZE - 1) {
            m_dialogMessageBufferSize = MESSAGE_BUFFER_SIZE - 1;
            unsigned int i = 0;
            do {
                unsigned int next = i + 1;
                m_pDialogMessageBuffer[static_cast<unsigned short>(i)] = m_pDialogMessageBuffer[next];
                i = next;
                size = m_dialogMessageBufferSize;
            } while (static_cast<unsigned short>(i) < size);
        }
        m_pDialogMessageBuffer[size].m_text = text;
        m_pDialogMessageBuffer[static_cast<unsigned int>(m_dialogMessageBufferSize)].m_speaker = speaker;
        m_dialogMessageBufferSize = m_dialogMessageBufferSize + 1;
    }
}

void CGuiInGame::AppendToDialogBuffer(const CExoString& text, unsigned long long objId)
{
    if (text != "") {
        CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(objId);
        if (pObj != 0) {
            CExoString name;
            CSWCObject* pSWCObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
            name = pSWCObj->GetName();
            AppendToDialogBuffer(text, name);
        }
    }
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::AppendToCombatBuffer(const CExoString& text, unsigned int a, unsigned char b)
{
    if (text != "") {
        unsigned int size = m_combatMessageBufferSize;
        if (size > MESSAGE_BUFFER_SIZE - 1) {
            m_combatMessageBufferSize = MESSAGE_BUFFER_SIZE - 1;
            uint64_t i = 0;
            do {
                CGuiInGameMessage* dest = &m_pCombatMessageBuffer[i];
                CGuiInGameMessage* src = &m_pCombatMessageBuffer[i + 1];
                dest->m_text = src->m_text;
                dest->m_param4 = src->m_param4;
                dest->m_param3 = src->m_param3;
                i++;
                size = m_combatMessageBufferSize;
            } while (i < size);
        }
        m_pCombatMessageBuffer[size].m_text = text;
        m_pCombatMessageBuffer[static_cast<unsigned int>(m_combatMessageBufferSize)].m_param3 = a;
        m_pCombatMessageBuffer[static_cast<unsigned int>(m_combatMessageBufferSize)].m_param4 = b;
        m_combatMessageBufferSize = m_combatMessageBufferSize + 1;
    }
}
#endif // !KOTOR1

void CGuiInGame::AppendToMsgBuffer(const CExoString& text, unsigned int a, unsigned char b)
{
    if (text != "") {
        unsigned int size = m_messageBufferSize;
        if (size > MESSAGE_BUFFER_SIZE - 1) {
            m_messageBufferSize = MESSAGE_BUFFER_SIZE - 1;
            unsigned short i = 0;
            do {
                CGuiInGameMessage* dest = &m_pMessageBuffer[i];
                unsigned int next = i + 1;
                CGuiInGameMessage* src = &m_pMessageBuffer[next];
                dest->m_text = src->m_text;
                dest->m_param4 = src->m_param4;
                dest->m_param3 = src->m_param3;
                i = static_cast<unsigned short>(next);
                size = m_messageBufferSize;
            } while (i < size);
        }
        m_pMessageBuffer[size].m_text = text;
        m_pMessageBuffer[static_cast<unsigned int>(m_messageBufferSize)].m_param3 = a;
        m_pMessageBuffer[static_cast<unsigned int>(m_messageBufferSize)].m_param4 = b;
        m_messageBufferSize = m_messageBufferSize + 1;
    }
}

// Reads the list head twice: once to seed the cursor, once for the entry.
bool CGuiInGame::GetCanOrientInDialog(unsigned long long objId)
{
    if (objId == OBJECT_INVALID) return false;
    CExoLinkedListNode* pos = m_orientExclusionList.m_pInternal->m_pHead;
    void* pData = m_orientExclusionList.m_pInternal->m_pHead ? m_orientExclusionList.m_pInternal->m_pHead->m_pData : 0;
    while (pos != 0) {
        if (*static_cast<unsigned long long*>(pData) == objId) return false;
        pData = m_orientExclusionList.m_pInternal->GetNext(pos);
    }
    return true;
}

bool CGuiInGame::GetCanHeadFollowInDialog(unsigned long long objId)
{
    if (objId == OBJECT_INVALID) return false;
    CExoLinkedListNode* pos = m_headFollowExclusionList.m_pInternal->m_pHead;
    void* pData = m_headFollowExclusionList.m_pInternal->m_pHead ? m_headFollowExclusionList.m_pInternal->m_pHead->m_pData : 0;
    while (pos != 0) {
        if (*static_cast<unsigned long long*>(pData) == objId) return false;
        pData = m_headFollowExclusionList.m_pInternal->GetNext(pos);
    }
    return true;
}

void CGuiInGame::CleanUpEntryNode()
{
    __builtin_memset(&m_lastShotValid, 0, offsetof(CGuiInGame, m_pMessageBuffer) - offsetof(CGuiInGame, m_lastShotValid));
    m_pDialogLetterbox->ResetFade();
    if ((m_pActiveDialogPanel == 0 || m_pActiveDialogPanel != m_pDialogCinematicPanel) &&
        m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pDialogComputerCameraPanel)) != 0) {
        m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pDialogComputerCameraPanel));
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pActiveDialogPanel), 2, 1);
#if KOTOR1  // K1: no override flag, the video effect goes off unconditionally
        g_pAppManager->m_pClientExoApp->DisableVideoEffect();
#else
        if (m_bScriptVideoEffect == 0) {
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
        }
#endif
    }
    m_fAnimatedCameraLength = 0;
    g_pExoInput->CoolDownEvent(INPUTEVENT_MOUSE_LEFT, 750);
    g_pExoInput->CoolDownEvent(INPUTEVENT_KEY_RETURN, 750);
}

void CGuiInGame::CleanUpReplyNode()
{
    __builtin_memset(&m_lastShotValid, 0, offsetof(CGuiInGame, m_pMessageBuffer) - offsetof(CGuiInGame, m_lastShotValid));
    m_pDialogLetterbox->ResetFade();
    ClearRepliesData();
    if ((m_pActiveDialogPanel == 0 || m_pActiveDialogPanel != m_pDialogCinematicPanel) &&
        m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pDialogComputerCameraPanel)) != 0) {
        m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pDialogComputerCameraPanel));
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pActiveDialogPanel), 2, 1);
#if KOTOR1  // K1: no override flag, the video effect goes off unconditionally
        g_pAppManager->m_pClientExoApp->DisableVideoEffect();
#else
        if (m_bScriptVideoEffect == 0) {
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
        }
#endif
    }
    m_fAnimatedCameraLength = 0;
    g_pExoInput->CoolDownEvent(INPUTEVENT_MOUSE_LEFT, 750);
    g_pExoInput->CoolDownEvent(INPUTEVENT_KEY_RETURN, 750);
}

#if KOTOR1
void CGuiInGame::ShowUpgradeScreen(unsigned long long itemObjId)
{
    if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pUpgradeScreenPanel == 0 &&
        (itemObjId == OBJECT_INVALID || g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemObjId) != 0)) {
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
        if (m_bPaused == 0)
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        CSWGuiUpgradeSelection* pPanel = new CSWGuiUpgradeSelection(m_pGuiManager);
        m_pUpgradeScreenPanel = pPanel;
        pPanel->m_itemObjId = itemObjId;
        m_pGuiManager->AddPanel(pPanel, 3, 1);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    }
}
#else
// Description taken by value.
static inline void SetUpgradeDescription(CSWGuiUpgradeSelection* pPanel, CExoString sDesc)
{
    pPanel->m_description = sDesc;
}

void CGuiInGame::ShowUpgradeScreen(unsigned long long itemObjId, unsigned long long creatureObjId,
                                    int a, int b, CExoString desc)
{
    if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pUpgradeScreenPanel == 0 &&
        (itemObjId == OBJECT_INVALID || g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemObjId) != 0)) {
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
#endif
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
        CSWGuiUpgradeSelection* pPanel = new CSWGuiUpgradeSelection(m_pGuiManager, creatureObjId);
        m_pUpgradeScreenPanel = pPanel;
        pPanel->m_bParam3 = (a != 0);
        pPanel->m_itemObjId = itemObjId;
        pPanel->m_creatureObjId = creatureObjId;
        pPanel->m_bParam4 = (b != 0);
        if (desc.m_sString != 0 && desc.m_sString[0] != '\0')
            SetUpgradeDescription(pPanel, desc);
        else
            SetUpgradeDescription(pPanel, CExoString(""));
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pUpgradeScreenPanel), 3, 1);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    }
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::ShowChemicalUpgradeScreen(unsigned long long objId)
{
    if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pChemicalUpgradeScreenPanel == 0) {
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
#endif
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
        CSWGuiCreateMedicalItem* pPanel = new CSWGuiCreateMedicalItem(m_pGuiManager, objId);
        m_pChemicalUpgradeScreenPanel = pPanel;
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pPanel), 3, 1);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    }
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::ShowSwoopUpgradeScreen(unsigned long long objId)
{
    if (m_bInitialized != 0 && m_pGuiManager != 0 && m_pSwoopUpgradeScreenPanel == 0 &&
        (objId == OBJECT_INVALID || g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(objId) != 0)) {
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
#endif
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
        CSWGuiSwoopUpgrade* pPanel = new CSWGuiSwoopUpgrade(m_pGuiManager);
        m_pSwoopUpgradeScreenPanel = pPanel;
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pPanel), 3, 1);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    }
}
#endif // !KOTOR1

void CGuiInGame::ShowReplyChosen(unsigned int a, unsigned int b, const CExoString& c)
{
    K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c);
}

int CGuiInGame::HandleInventoryClose(int flag)
{
    K2_UNUSED(flag);
    return static_cast<int>(reinterpret_cast<long long>(this));
}

void CGuiInGame::CreateDeathMenuGUI()
{
}

void CGuiInGame::ClearMapDisplays()
{
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CGuiInGame::StartCreditSequence(const Vector& pos, const CExoString& text)
{
    K2_UNUSED(pos); K2_UNUSED(text);
}
#endif // !KOTOR1

void CGuiInGame::PreloadMaps()
{
}

void CGuiInGame::HideDebugMenu()
{
}

void CGuiInGame::ShowDebugMenu()
{
}

void CGuiInGame::DestroyDeathMenuGUI()
{
}

void CGuiInGame::UpdateCreatedInGameGUI(int oldPanel, int newPanel)
{
    if (oldPanel == newPanel || g_bCreateAllGUIs != 0) return;

    if (oldPanel != -1) {
        CSWGuiPanel* pPanel = m_apScreenPanels[oldPanel];
        if (pPanel != 0) {
#if KOTOR1  // K1: no panel chain, one stamp
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
#else
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel != 0);
#endif
            m_apScreenPanels[oldPanel] = 0;
        }
    }

    if (m_apScreenPanels[newPanel] == 0) {
        CSWGuiPanel* pNewPanel = reinterpret_cast<CSWGuiPanel*>(this);  // idiom-exempt: the cast is unreachable, newPanel is always 0-7.
        switch (newPanel) {
        case SCREEN_EQUIP: pNewPanel = static_cast<CSWGuiPanel*>(new CSWGuiInGameEquip(m_pGuiManager)); break;
        case SCREEN_INVENTORY: pNewPanel = new CSWGuiInGameInventory(m_pGuiManager); break;
        case SCREEN_CHARACTER: pNewPanel = new CSWGuiInGameCharacter(m_pGuiManager); break;
        case SCREEN_ABILITIES: pNewPanel = new CSWGuiInGameAbilities(m_pGuiManager); break;
#if KOTOR1  // K1: screen 4 is the message log
        case SCREEN_MESSAGES: pNewPanel = new CSWGuiInGameMessages(m_pGuiManager); break;
#else
        case SCREEN_PARTY_SELECTION: pNewPanel = new CSWGuiPartySelection(m_pGuiManager); break;
#endif
        case SCREEN_JOURNAL: pNewPanel = new CSWGuiInGameJournal(m_pGuiManager); break;
        case SCREEN_MAP: pNewPanel = new CSWGuiInGameMap(m_pGuiManager); break;
        case SCREEN_OPTIONS: pNewPanel = new CSWGuiInGameOptions(m_pGuiManager); break;
        }
        m_apScreenPanels[newPanel] = pNewPanel;
    }
}

int CGuiInGame::ShowSWInGameGui(int panelIndex)
{
    if (m_bInitialized != 0) {
        CSWGuiManager* pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
#if !KOTOR1  // K1 has no ResetPartyMemberItemUse
        CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
        pInGameGui->m_pMainInterface->ResetPartyMemberItemUse();
#endif
        if (pGuiManager != 0) {
#if KOTOR1
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
#else
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
#endif
            g_pAppManager->m_pClientExoApp->IndicateObject(0, 0);
            SetSWGuiStatus(SWGUI_COVER, 1);
            CExoString scriptName("k_sup_guiopen");
            g_pVirtualMachine->RunScript(&scriptName, 0, 0);

            if (static_cast<unsigned int>(panelIndex) < 8) {
                m_lastGuiPanel = panelIndex;
            } else {
                CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
                CSWCCreature* pPlayerChar = pParty->GetPlayerCharacter();
                if (pPlayerChar != 0) {
                    pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
                    pPlayerChar = pParty->GetPlayerCharacter();
                    CSWSCreature* pServerCreature = pPlayerChar->GetServerCreature();
                    if (pServerCreature->m_pStats->CanLevelUp() != 0) {
                        m_lastGuiPanel = SCREEN_CHARACTER;
                    }
                }
            }

            UpdateCreatedInGameGUI(-1, m_lastGuiPanel);
            pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pInGameMenu), 0, 1);
            pGuiManager->AddPanel(m_apScreenPanels[m_lastGuiPanel], 2, 1);
            pGuiManager->SendPanelToBack(m_apScreenPanels[m_lastGuiPanel]);
            m_pInGameMenu->SetActiveControlID(m_lastGuiPanel, 0);
            m_bInGameMenuShown = 1;
            if (m_bPaused == 0) {
                g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
            }
            m_pBarkBubble->Pause();
            if (g_pVirtualMachine != 0) {
                CExoString scriptName2("k_pend_screenchg");
                g_pVirtualMachine->RunScript(&scriptName2, OBJECT_INVALID, 0);
            }
            m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_ADDED);
        }
    }
    return m_bInGameMenuShown;
}

int CGuiInGame::HideSWInGameGui(int notify)
{
    if (m_bInitialized == 0) return false;

    CSWGuiManager* pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    if (pGuiManager == 0) return false;

#if !KOTOR1  // K1 has no action-bar revert or pause-panel raise over the fade
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pInGameGui->m_pMainInterface != 0) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->RevertActionBarStates();
    }
#endif

    if (pGuiManager->m_modalPanels.m_count != 0) return false;

    SetSWGuiStatus(SWGUI_UNCOVER, 1);
    pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(m_pInGameMenu));

    bool removedScreenPanel = false;
    if (g_bCreateAllGUIs != 0) {
        pGuiManager->RemovePanel(m_apScreenPanels[m_lastGuiPanel]);
        removedScreenPanel = g_bCreateAllGUIs != 0;
    }

    m_bInGameMenuShown = 0;
    if (!removedScreenPanel && m_lastGuiPanel != -1) {
        CSWGuiPanel* p = m_apScreenPanels[m_lastGuiPanel];
        if (p != 0) {
#if KOTOR1  // K1: no panel chain, one stamp
            p->m_stateFlags = (p->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
#else
            do {
                p->m_stateFlags = (p->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
                p = p->m_pNextInChain;
            } while (p != 0);
#endif
            m_apScreenPanels[m_lastGuiPanel] = 0;
        }
    }

    if (m_bPaused == 0) {
        g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
    } else {
        if (g_bInScreenshotMode == 0 && notify == 0 && m_pGuiManager->PanelExists(m_pPauseGuiPanel) == 0) {
            m_pGuiManager->AddPanel(m_pPauseGuiPanel, 4, 1);
            m_pGuiManager->SendPanelToBack(m_pPauseGuiPanel);
        }
#if !KOTOR1
        if (m_pGuiManager->PanelExists(m_pPauseGuiPanel) != 0 && m_pGlobalFade != 0 &&
            m_pGlobalFade->m_fAlpha > 0.001f) {
            m_pGuiManager->BringPanelToFront(m_pPauseGuiPanel);
        }
#endif
    }

    if (notify != 0 && m_pGuiManager->PanelExists(m_pPauseGuiPanel) != 0) {
        unsigned short flags = m_pPauseGuiPanel->m_stateFlags;
        unsigned short cleared = flags & ~PANEL_REMOVE_DEFERRED;
        unsigned short removeMode = flags & PANEL_REMOVE_MASK;
        unsigned short removeRequest = (flags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
        m_pPauseGuiPanel->m_stateFlags = (removeMode == PANEL_REMOVE_AND_DELETE) ? cleared : removeRequest;
    }

    m_pBarkBubble->Resume();
    m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_REMOVED);
#if KOTOR1
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#else
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
    return true;
}

void CGuiInGame::NextSWInGameGui()
{
    if (m_bInitialized == 0) return;

    CSWGuiManager* pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    if (g_bCreateAllGUIs != 0 && pGuiManager != 0 && m_bInGameMenuShown != 0) {
        pGuiManager->RemovePanel(m_apScreenPanels[m_lastGuiPanel]);
    }

    int oldPanel = m_lastGuiPanel;
    int newPanel = (oldPanel > 6) ? 0 : oldPanel + 1;
    int updateArg = (m_bInGameMenuShown == 0 || pGuiManager == 0) ? -1 : newPanel;
    m_lastGuiPanel = newPanel;
    UpdateCreatedInGameGUI(oldPanel, updateArg);

    if (pGuiManager != 0 && m_bInGameMenuShown != 0) {
        pGuiManager->AddPanel(m_apScreenPanels[m_lastGuiPanel], 2, 1);
        pGuiManager->SendPanelToBack(m_apScreenPanels[m_lastGuiPanel]);
        m_pInGameMenu->SetActiveControlID(m_lastGuiPanel, 1);
        if (g_pVirtualMachine != 0) {
            CExoString scriptName("k_pend_screenchg");
            g_pVirtualMachine->RunScript(&scriptName, OBJECT_INVALID, 0);
        }
    }
}

void CGuiInGame::PrevSWInGameGui()
{
    if (m_bInitialized == 0) return;

    CSWGuiManager* pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    if (g_bCreateAllGUIs != 0 && pGuiManager != 0 && m_bInGameMenuShown != 0) {
        pGuiManager->RemovePanel(m_apScreenPanels[m_lastGuiPanel]);
    }

    int oldPanel = m_lastGuiPanel;
    int newPanel = (oldPanel < 1) ? 7 : oldPanel - 1;
    int updateArg = (m_bInGameMenuShown == 0 || pGuiManager == 0) ? -1 : newPanel;
    m_lastGuiPanel = newPanel;
    UpdateCreatedInGameGUI(oldPanel, updateArg);

    if (pGuiManager != 0 && m_bInGameMenuShown != 0) {
        pGuiManager->AddPanel(m_apScreenPanels[m_lastGuiPanel], 2, 1);
        pGuiManager->SendPanelToBack(m_apScreenPanels[m_lastGuiPanel]);
        m_pInGameMenu->SetActiveControlID(m_lastGuiPanel, 1);
        if (g_pVirtualMachine != 0) {
            CExoString scriptName("k_pend_screenchg");
            g_pVirtualMachine->RunScript(&scriptName, OBJECT_INVALID, 0);
        }
    }
}

void CGuiInGame::SwitchToSWInGameGui(int newPanel)
{
    if (static_cast<unsigned int>(newPanel) >= 8) return;
    if (m_lastGuiPanel == newPanel) return;
    if (m_bInitialized == 0) return;

    CSWGuiManager* pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    if (g_bCreateAllGUIs != 0 && pGuiManager != 0 && m_bInGameMenuShown != 0) {
        K2_ASSUME(static_cast<unsigned int>(m_lastGuiPanel) < 8);   // only ever set to a panel index 0..7
        pGuiManager->RemovePanel(m_apScreenPanels[m_lastGuiPanel]);
    }

    int oldPanel = m_lastGuiPanel;
    int updateArg = (m_bInGameMenuShown == 0 || pGuiManager == 0) ? -1 : newPanel;
    UpdateCreatedInGameGUI(oldPanel, updateArg);
    m_lastGuiPanel = newPanel;

    if (pGuiManager != 0 && m_bInGameMenuShown != 0) {
        pGuiManager->AddPanel(m_apScreenPanels[newPanel], 2, 1);
        pGuiManager->SendPanelToBack(m_apScreenPanels[m_lastGuiPanel]);
        m_pInGameMenu->SetActiveControlID(m_lastGuiPanel, 1);
        if (g_pVirtualMachine != 0) {
            CExoString scriptName("k_pend_screenchg");
            g_pVirtualMachine->RunScript(&scriptName, OBJECT_INVALID, 0);
        }
    }
}

void CGuiInGame::ToggleSWGui()
{
    int state = m_swGuiToggleState;
#if KOTOR1  // K1: the same states, spelled as a switch
    switch (state) {
    case SWGUI_STATE_SHOWN:
        m_swGuiToggleState = SWGUI_STATE_TOGGLED_OFF;
        if (m_bInitialized != 0 && m_pGuiManager != 0) {
            if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pMainInterface))) {
                m_pGuiManager->RemovePanel(m_pMainInterface);
            }
            if (m_pGuiManager->PanelExists(m_pPauseGuiPanel)) {
                unsigned short flags = m_pPauseGuiPanel->m_stateFlags;
                unsigned short cleared = flags & ~PANEL_REMOVE_DEFERRED;
                unsigned short removeMode = flags & PANEL_REMOVE_MASK;
                unsigned short removeRequest = (flags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
                m_pPauseGuiPanel->m_stateFlags = (removeMode == PANEL_REMOVE_AND_DELETE) ? cleared : removeRequest;
            }
        }
        return;
    case SWGUI_STATE_TOGGLED_OFF:
        state = SWGUI_TOGGLE_ON;
        break;
    case SWGUI_STATE_COVERED:
        state = SWGUI_UNCOVER;
        break;
    default:
        return;
    }
    SetSWGuiStatus(state, 1);
#else
    if (state == SWGUI_STATE_COVERED) {
        state = SWGUI_UNCOVER;
    }
    else if (state != SWGUI_STATE_TOGGLED_OFF) {
        if (state == SWGUI_STATE_SHOWN) {
            m_swGuiToggleState = SWGUI_STATE_TOGGLED_OFF;
            if (m_bInitialized != 0 && m_pGuiManager != 0) {
                if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pMainInterface))) {
                    m_pGuiManager->RemovePanel(m_pMainInterface);
                }
                if (m_pGuiManager->PanelExists(m_pPauseGuiPanel)) {
                    unsigned short flags = m_pPauseGuiPanel->m_stateFlags;
                    unsigned short cleared = flags & ~PANEL_REMOVE_DEFERRED;
                    unsigned short removeMode = flags & PANEL_REMOVE_MASK;
                    unsigned short removeRequest = (flags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
                    m_pPauseGuiPanel->m_stateFlags = (removeMode == PANEL_REMOVE_AND_DELETE) ? cleared : removeRequest;
                }
            }
        }
        return;
    }
    else {
        state = SWGUI_TOGGLE_ON;
    }
    SetSWGuiStatus(state, 1);
#endif
}

int CGuiInGame::IsLoopingDialogStuntAnimation(unsigned short anim)
{
    return static_cast<unsigned short>(anim - STUNT_ANIM_LOOPING_FIRST) < STUNT_ANIM_COUNT;
}

bool CGuiInGame::IsFireForgetDialogStuntAnimation(unsigned short anim)
{
    return static_cast<unsigned short>(anim - STUNT_ANIM_FIREFORGET_FIRST) < STUNT_ANIM_COUNT;
}

bool CGuiInGame::IsDialogStuntAnimation(unsigned short anim)
{
    return static_cast<unsigned short>(anim - STUNT_ANIM_FIREFORGET_FIRST) < STUNT_ANIM_SPAN;
}

bool CGuiInGame::IsOverlayDialogAnimation(unsigned short anim)
{
    if (static_cast<unsigned short>(anim >> 4) < DIALOG_ANIM_2DA_BASE / 16) {
        return false;
    }

    if (g_pRules->m_p2DArrays->m_pDialogAnimations->m_nRows + DIALOG_ANIM_2DA_BASE <= anim) {
        return false;
    }
    unsigned short row = static_cast<unsigned short>(anim - DIALOG_ANIM_2DA_BASE);

    int overlayEntry;
    if (!g_pRules->m_p2DArrays->m_pDialogAnimations->GetINTEntry(
            row, CTwoDimArrays::COL2DA_DIALOGANIMATIONS_OVERLAY, &overlayEntry)) {
        return false;
    }

    int dialogEntry;
    if (!g_pRules->m_p2DArrays->m_pDialogAnimations->GetINTEntry(
            row, CTwoDimArrays::COL2DA_DIALOGANIMATIONS_DIALOG, &dialogEntry)) {
        return false;
    }

    return overlayEntry != 0 && dialogEntry != 0;
}

// Per-shot dialog camera update. The other modes re-orient the two participants toward each other
// (honouring the orient and head-follow exclusion lists) and then pick the shot: nShotType 6 comes
// from the placeable cameras, anything else from the 2D range table.
void CGuiInGame::UpdateShotCamera(unsigned int nShotType, int nCharId, float fC, float fD, unsigned int nArg, int nVideoEffect)
{
    CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidPrevDialogParticipantA);
    if (pObj != 0 && pObj->AsSWCCreature() != 0) {
        pObj->AsSWCCreature()->LookAt(OBJECT_INVALID, 0.0f);
    }
    pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidPrevDialogParticipantB);
    if (pObj != 0 && pObj->AsSWCCreature() != 0) {
        pObj->AsSWCCreature()->LookAt(OBJECT_INVALID, 0.0f);
    }

    if (nShotType == 5 && m_oidPrevDialogParticipantA != OBJECT_INVALID) {
        m_dialogParticipantIdA = m_oidPrevDialogParticipantA;
        m_dialogParticipantIdB = m_oidPrevDialogParticipantB;
    }

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut == 0 &&
        m_pActiveDialogPanel != m_pDialogComputerPanel) {
        CSWCObject* pObjA = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogParticipantIdA)->AsSWCObject();
        CSWCObject* pObjB = g_pAppManager->m_pClientExoApp->GetGameObject(m_dialogParticipantIdB)->AsSWCObject();
        if (pObjA != 0 && pObjB != 0) {
            Vector vDirAB = normalize(pObjA->m_position - pObjB->m_position);
            Vector vOrient = pObjB->m_orientation;
            if (GetCanOrientInDialog(pObjB->m_objectId) &&
                pObjB->AsSWCCreature() != 0 &&
                pObjB->AsSWCCreature()->m_fWalkRate > 0.0f) {
                bool bHeadFollow = false;
                if (GetCanHeadFollowInDialog(pObjB->m_objectId)) {
                    Vector vDir = normalize(pObjA->m_position - pObjB->m_position);
                    Vector vHead = normalize(pObjB->m_orientation);
                    float fDot = vDir.x * vHead.x + vDir.y * vHead.y + vDir.z * vHead.z;
                    float fDotC = 1.0f;
                    if (!(fDot > 1.0f)) {
                        fDotC = -1.0f;
                        if (!(fDot < -1.0f)) fDotC = fDot;
                    }
                    float fAngle = acosf(fDotC) * 57.295776f;
                    float fHeadTurn = pObjB->AsSWCCreature()->GetHeadTurnHorizontal();
                    float fMaxTurn = 0.0f;
#if KOTOR1  // K1: an inline finiteness test
                    if (__builtin_isfinite(fAngle)) fMaxTurn = fAngle;
#else
                    if (_finite(static_cast<double>(fAngle)) != 0) fMaxTurn = fAngle;
#endif
                    if (fMaxTurn > fHeadTurn) {
                        Vector vAxis = { 0.0f, 0.0f, 1.0f };
                        float fTurn = fMaxTurn - (fHeadTurn + 1.0f);
                        float fCross = vDir.x * (vHead.y * vAxis.z - vHead.z * vAxis.y) +
                                       vDir.y * (vHead.z * vAxis.x - vHead.x * vAxis.z) +
                                       vDir.z * (vHead.x * vAxis.y - vHead.y * vAxis.x);
                        if (fCross > 0.0f) fTurn = -fTurn;
                        Quaternion qTurn(vAxis, fTurn * 0.017453292f);
                        if (quaternionidentityoptimization == 0 || (qTurn.w != 1.0f && qTurn.w != -1.0f)) {
                            float w = qTurn.w, x = qTurn.x, y = qTurn.y, z = qTurn.z;
                            float xx = x * x, yy = y * y, zz = z * z;
                            float xy = x * y, xz = x * z, wx = w * x;
                            float yz = y * z, wy = w * y;
                            float wz = w * z;
                            Vector r;
                            r.x = vHead.x * (1.0f - ((yy + zz) + (yy + zz))) + vHead.y * ((xy - wz) + (xy - wz)) + vHead.z * ((xz + wy) + (xz + wy));
                            r.y = vHead.x * ((xy + wz) + (xy + wz)) + vHead.y * (1.0f - ((xx + zz) + (xx + zz))) + vHead.z * ((yz - wx) + (yz - wx));
                            r.z = vHead.x * ((xz - wy) + (xz - wy)) + vHead.y * ((yz + wx) + (yz + wx)) + vHead.z * (1.0f - ((xx + yy) + (xx + yy)));
                            vOrient = r;
                        }
                    }
                    bHeadFollow = true;
                }
                pObjB->AsSWCCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                unsigned long long serverId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pObjB->m_objectId);
                CGameObject* pSrvObj = g_pAppManager->m_pServerExoApp->GetGameObject(serverId);
                if (pSrvObj != 0 && pSrvObj->AsSWSCreature() != 0) {
                    pSrvObj->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                }
                if (bHeadFollow && pObjB->AsSWCCreature()->LookAt(pObjA->m_objectId, 10.0f) != 0) {
                    if (pSrvObj != 0 && pSrvObj->AsSWSCreature() != 0) {
                        pSrvObj->AsSWSCreature()->SetOrientation(vOrient);
                    }
                    CSWCCreature* pCreature = pObjB->AsSWCCreature();
                    pCreature->m_vStationaryOrientation = normalize(vOrient);
                    pObjB->AsSWCCreature()->SetOrientation(vOrient);
                } else {
                    if (pSrvObj != 0 && pSrvObj->AsSWSCreature() != 0) {
                        pSrvObj->AsSWSCreature()->SetOrientation(vDirAB);
                    }
                    CSWCCreature* pCreature = pObjB->AsSWCCreature();
                    pCreature->m_vStationaryOrientation = normalize(vDirAB);
                    pObjB->AsSWCCreature()->SetOrientation(vDirAB);
                }
            }
            vOrient = pObjA->m_orientation;
            if (GetCanOrientInDialog(pObjA->m_objectId) &&
                pObjA->AsSWCCreature() != 0 &&
                pObjA->AsSWCCreature()->m_fWalkRate > 0.0f) {
                bool bHeadFollow = false;
                if (GetCanHeadFollowInDialog(pObjA->m_objectId)) {
                    Vector vDir = normalize(pObjB->m_position - pObjA->m_position);
                    Vector vHead = normalize(pObjA->m_orientation);
                    float fDot = vDir.x * vHead.x + vDir.y * vHead.y + vDir.z * vHead.z;
                    float fDotC = 1.0f;
                    if (!(fDot > 1.0f)) {
                        fDotC = -1.0f;
                        if (!(fDot < -1.0f)) fDotC = fDot;
                    }
                    float fAngle = acosf(fDotC) * 57.295776f;
                    float fHeadTurn = pObjA->AsSWCCreature()->GetHeadTurnHorizontal();
                    float fMaxTurn = 0.0f;
#if KOTOR1  // K1: an inline finiteness test
                    if (__builtin_isfinite(fAngle)) fMaxTurn = fAngle;
#else
                    if (_finite(static_cast<double>(fAngle)) != 0) fMaxTurn = fAngle;
#endif
                    if (fMaxTurn > fHeadTurn) {
                        Vector vAxis = { 0.0f, 0.0f, 1.0f };
                        float fTurn = fMaxTurn - (fHeadTurn + 1.0f);
                        float fCross = vDir.x * (vHead.y * vAxis.z - vHead.z * vAxis.y) +
                                       vDir.y * (vHead.z * vAxis.x - vHead.x * vAxis.z) +
                                       vDir.z * (vHead.x * vAxis.y - vHead.y * vAxis.x);
                        if (fCross > 0.0f) fTurn = -fTurn;
                        Quaternion qTurn(vAxis, fTurn * 0.017453292f);
                        if (quaternionidentityoptimization == 0 || (qTurn.w != 1.0f && qTurn.w != -1.0f)) {
                            float w = qTurn.w, x = qTurn.x, y = qTurn.y, z = qTurn.z;
                            float xx = x * x, yy = y * y, zz = z * z;
                            float xy = x * y, xz = x * z, wx = w * x;
                            float yz = y * z, wy = w * y;
                            float wz = w * z;
                            Vector r;
                            r.x = vHead.x * (1.0f - ((yy + zz) + (yy + zz))) + vHead.y * ((xy - wz) + (xy - wz)) + vHead.z * ((xz + wy) + (xz + wy));
                            r.y = vHead.x * ((xy + wz) + (xy + wz)) + vHead.y * (1.0f - ((xx + zz) + (xx + zz))) + vHead.z * ((yz - wx) + (yz - wx));
                            r.z = vHead.x * ((xz - wy) + (xz - wy)) + vHead.y * ((yz + wx) + (yz + wx)) + vHead.z * (1.0f - ((xx + yy) + (xx + yy)));
                            vOrient = r;
                        }
                    }
                    bHeadFollow = true;
                }
                pObjA->AsSWCCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                unsigned long long serverId = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pObjA->m_objectId);
                CGameObject* pSrvObj = g_pAppManager->m_pServerExoApp->GetGameObject(serverId);
                if (pSrvObj != 0 && pSrvObj->AsSWSCreature() != 0) {
                    pSrvObj->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 0);
                }
                if (bHeadFollow && pObjA->AsSWCCreature()->LookAt(pObjB->m_objectId, 10.0f) != 0) {
                    if (pSrvObj != 0 && pSrvObj->AsSWSCreature() != 0) {
                        pSrvObj->AsSWSCreature()->SetOrientation(vOrient);
                    }
                    CSWCCreature* pCreature = pObjA->AsSWCCreature();
                    pCreature->m_vStationaryOrientation = normalize(vOrient);
                    pObjA->AsSWCCreature()->SetOrientation(vOrient);
                } else {
                    if (pSrvObj != 0 && pSrvObj->AsSWSCreature() != 0) {
                        pSrvObj->AsSWSCreature()->SetOrientation(-vDirAB);
                    }
                    CSWCCreature* pCreature = pObjA->AsSWCCreature();
                    pCreature->m_vStationaryOrientation = normalize(-vDirAB);
                    pObjA->AsSWCCreature()->SetOrientation(-vDirAB);
                }
            }
        }
    }

    if (nShotType != 5) {
        CSWBehaviorCameraDialog* pBehavior = static_cast<CSWBehaviorCameraDialog*>(
            g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_DIALOG));
        if (nShotType == 6) {
            if (pBehavior != 0) {
                int i;
                bool bFound = false;
                for (i = 0; i < m_placeableCameraCount; i++) {
                    if (m_placeableCameras[i].m_charId == nCharId) {
                        bFound = true;
                        break;
                    }
                }
                Vector vPos;
                Quaternion qRot;
                float fShotF;
                float fLastD;
                if (bFound) {
                    CGuiPlaceableCamera* pCam = &m_placeableCameras[i];
                    vPos = pCam->m_position;
                    vPos.z += pCam->m_b;
                    qRot = pCam->m_rotation;
                    qRot = YawPitchRoll(Yaw(qRot), Pitch(qRot) + pCam->m_a, Roll(qRot));
                    fShotF = pCam->m_c;
                    fLastD = pCam->m_d;
                } else {
                    vPos.x = 0.0f;
                    vPos.y = 0.0f;
                    vPos.z = 0.0f;
                    fShotF = 55.0f;
                    fLastD = 0.0f;
                }
#if !KOTOR1  // K1 has no Aspyr camera hacks
                if (!IsGameModded() && AspyrHackCarthDialog) {
                    vPos.x += 2.0f;
                    vPos.y += -6.0f;
                    vPos.z += 0.5f;
                    qRot = YawPitchRoll(Yaw(qRot) + 110.0f, Pitch(qRot), Roll(qRot));
                }
#endif
                pBehavior->SetShot(6, vPos, qRot, fShotF);
                m_lastShotValid = 1;
                m_lastShotPos = vPos;
                m_lastShotD = fLastD;
#if KOTOR1  // K1: a placeable shot only turns the video effect on
                if (static_cast<unsigned int>(nVideoEffect) < static_cast<unsigned int>(VIDEO_EFFECT_NONE_NO_CAPTION)) {
                    g_pAppManager->m_pClientExoApp->EnableVideoEffect(nVideoEffect);
                }
#ifdef K2_FIX_ENGINE_BUGS
                else {
                    g_pAppManager->m_pClientExoApp->DisableVideoEffect();
                }
#endif
#else
                if (static_cast<unsigned int>(nVideoEffect) < static_cast<unsigned int>(VIDEO_EFFECT_NONE_NO_CAPTION)) {
                    g_pAppManager->m_pClientExoApp->EnableVideoEffect(nVideoEffect);
                } else if (m_bScriptVideoEffect == 0) {
                    g_pAppManager->m_pClientExoApp->DisableVideoEffect();
                }
#endif
            }
        } else {
            if (pBehavior != 0) {
                float fRange = 0.0f;
                if (nShotType == 1) {
                    unsigned short nAnim = SERVER_ANIMATION_TALK_NORMAL;
                    for (int i = 0; i < static_cast<int>(m_dialogAnimCount); i++) {
                        if (m_paDialogAnimData[i] == m_dialogParticipantIdA) {
                            nAnim = m_paDialogAnimFlags[i];
                            break;
                        }
                    }
#if KOTOR1  // K1: GetInGameGui is re-fetched for each test
                    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->IsFireForgetDialogAnimation(nAnim) != 0 ||
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->IsLoopingDialogAnimation(nAnim) != 0) {
#else
                    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
                    if (pInGameGui->IsFireForgetDialogAnimation(nAnim) != 0 ||
                        pInGameGui->IsLoopingDialogAnimation(nAnim) != 0) {
#endif
                        C2DA* p2DA = g_pRules->m_p2DArrays->m_pDialogAnimations;
                        int bGot = p2DA->GetFLOATEntry(nAnim - SERVER_ANIMATION_BASE, CExoString("CU_PB_Range"), &fRange);
                        double dRange;
                        if (bGot == 0) {
                            dRange = 0.0;
                            fRange = 0.0f;
                        } else {
                            dRange = fRange;
                        }
                        if (dRange < 0.001 && dRange > -0.001) {
                            fRange = 0.0f;
                        }
#if !KOTOR1  // K1 has no range bonus for the last appearance
                        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_dialogParticipantIdA);
                        if (pCreature != 0 && pCreature->m_pCreatureAppearance->m_nAppearanceId == APPEARANCE_ALIEN_GRAN_01) {
                            fRange += 0.25f;
                        }
#endif
                        if (fRange == 0.0f) {
                            m_fCloseUpPullBackRange = 0.0f;
                        }
                    }
                    if (m_fCloseUpPullBackRange > 0.0f) {
                        fRange = m_fCloseUpPullBackRange;
                    } else {
                        m_fCloseUpPullBackRange = fRange;
                    }
                } else {
                    m_fCloseUpPullBackRange = 0.0f;
                }
#if !KOTOR1
                if (!IsGameModded() && AspyrHackCarthDialog) {
                    unsigned short nAppearanceA = 0;
                    unsigned short nAppearanceB = 0;
                    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_dialogParticipantIdA);
                    if (pCreature != 0) {
                        nAppearanceA = pCreature->m_pCreatureAppearance->m_nAppearanceId;
                    }
                    pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_dialogParticipantIdB);
                    if (pCreature != 0) {
                        nAppearanceB = pCreature->m_pCreatureAppearance->m_nAppearanceId;
                    }
                    AspyrHackCameraDistance = 1.0f;
                    if (nAppearanceA == APPEARANCE_REPUBLIC_OFFICER_MALE_OLD_WHITE) {
                        if (nShotType == 3 && nAppearanceB == APPEARANCE_REPUBLIC_OFFICER_MALE_OLD_BLACK) {
                            AspyrHackCameraFocus = 0.0f;
                            AspyrHackCameraYaw = 70.0f;
                        } else {
                            AspyrHackCameraFocus = 0.0f;
                            AspyrHackCameraDistance = 0.0f;
                            AspyrHackCameraYaw = 0.0f;
                        }
                    } else if (nAppearanceA == APPEARANCE_REPUBLIC_OFFICER_MALE_OLD_BLACK) {
                        if (nShotType == 2 && nAppearanceB == APPEARANCE_REPUBLIC_OFFICER_FEMALE_OLD_BLACK) {
                            AspyrHackCameraFocus = 0.2f;
                            AspyrHackCameraYaw = 70.0f;
                        } else if (nShotType == 2 && nAppearanceB == APPEARANCE_REPUBLIC_OFFICER_MALE_OLD_WHITE) {
                            AspyrHackCameraFocus = 0.8f;
                            AspyrHackCameraDistance = 0.6f;
                            AspyrHackCameraYaw = 120.0f;
                        } else {
                            AspyrHackCameraFocus = 0.0f;
                            AspyrHackCameraDistance = 0.0f;
                            AspyrHackCameraYaw = 0.0f;
                        }
                    } else if (nAppearanceA == APPEARANCE_REPUBLIC_OFFICER_FEMALE_OLD_BLACK) {
                        if (nShotType == 3 && nAppearanceB == APPEARANCE_REPUBLIC_OFFICER_MALE_OLD_BLACK) {
                            AspyrHackCameraFocus = 0.2f;
                            AspyrHackCameraYaw = 70.0f;
                        } else {
                            AspyrHackCameraFocus = 0.0f;
                            AspyrHackCameraDistance = 0.0f;
                            AspyrHackCameraYaw = 0.0f;
                        }
                    } else {
                        AspyrHackCameraFocus = 0.0f;
                        AspyrHackCameraDistance = 0.0f;
                        AspyrHackCameraYaw = 0.0f;
                    }
                }
#endif
                pBehavior->SetShot(m_dialogParticipantIdA, m_dialogParticipantIdB, nShotType, fC, fD, fRange);
#if KOTOR1  // K1: a dialog shot always turns the video effect off
                g_pAppManager->m_pClientExoApp->DisableVideoEffect();
#else
                if (static_cast<unsigned int>(nVideoEffect) < static_cast<unsigned int>(VIDEO_EFFECT_NONE_NO_CAPTION)) {
                    g_pAppManager->m_pClientExoApp->EnableVideoEffect(nVideoEffect);
                } else if (m_bScriptVideoEffect == 0) {
                    g_pAppManager->m_pClientExoApp->DisableVideoEffect();
                }
#endif
            }
        }
    }

    m_nWaitFlags = nArg;
    m_bDialogSoundPolled = 0;
}

// Loads the first valid quicksave slot listed in the save list.
void CGuiInGame::DoQuickLoad()
{
    CExoArrayList<CExoString> fileList;
    CExoString unused;

#if KOTOR1  // K1: the six-argument GetDirectoryList
    g_pExoBase->GetDirectoryList(&fileList, CExoString("SAVES:"), RESTYPE_INVALID, 1, 1, 0);
#else
    g_pExoBase->GetDirectoryList(&fileList, CExoString("SAVES:"), RESTYPE_INVALID, 1, 1);
#endif

    if (g_pExoResMan->CreateDirectory2(CExoString("TEMP:")) == 0)
        g_pExoResMan->CleanDirectory(CExoString("TEMP:"), 0, 0);

    for (int i = 0; i < fileList.m_count; i++) {
        CSWGuiSaveLoadEntry* pEntry = new CSWGuiSaveLoadEntry();
        pEntry->LoadData(fileList.m_pArray[i]);
        if (pEntry->m_nSaveSlot != 0) {
            delete pEntry;
            continue;
        }

#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
        g_pAppManager->m_pClientExoApp->BusyWaitMuteSound();
        CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;

        if (pEntry->m_bLiveContent == 0 || g_bEnableLiveContentIgnore != 0) {
            CSWPartyTable::ResetCurrentSessionStartTime();
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_unpauseableRefCount += 1;
            pClientExoApp->SetLoadScreenByModuleName(pEntry->m_moduleName);
            pClientExoApp->SetLoadMusic(CExoString(""));
#if KOTOR1  // K1: the three-argument ShowLoadScreen
            pClientExoApp->ShowLoadScreen(0, 1, 0);
#else
            pClientExoApp->ShowLoadScreen(0, 1, 0, 1);
#endif
            pClientExoApp->SetGamePlayHint(pEntry->m_gamePlayHint);
#if KOTOR1  // K1: one story hint
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[0]);
#else
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[0], 0);
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[1], 1);
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[2], 2);
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[3], 3);
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[4], 4);
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[5], 5);
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[6], 6);
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[7], 7);
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[8], 8);
            pClientExoApp->SetStoryHint(pEntry->m_storyHints[9], 9);
#endif
#if KOTOR1  // K1: the hint is drawn for an empty module name
            uint64_t hintStrRef =
                pClientExoApp->GetNextLoadScreenHintSTRREF(CExoString(""), 2);
#else
            uint64_t hintStrRef =
                pClientExoApp->GetNextLoadScreenHintSTRREF(pEntry->m_moduleName, 2);
#endif
            pClientExoApp->SetLoadScreenHint(hintStrRef);
            pClientExoApp->SetLoadScreenHintUpdateTimer(0);
            UnpackQuickSaveGame(pEntry->m_nSaveSlot, pEntry->m_saveName);
            while (m_pGuiManager->m_modalPanels.m_count != 0)
                m_pGuiManager->PopModalPanel();
            pClientExoApp->GetInGameGui()->HideSWInGameGui(1);
            g_pAppManager->m_pServerExoApp->SetLoadFromSaveGame(1);
            g_pAppManager->m_pClientExoApp->SendLoadGameRequest(pEntry->m_nSaveSlot,
                                                                pEntry->m_saveName,
                                                                pEntry->m_moduleName);
            delete pEntry;
            return;
        }

        CSWGuiMessageBox* pBox =
            pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        CExoString text;
        text = g_pTlkTable->GetSimpleString(0);
        g_pTlkTable->SetCustomToken(0, pEntry->m_sTokenValue);
        g_pTlkTable->ParseStr(text);
        pBox->SetMessage(text);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    g_pExoResMan->RemoveResourceDirectory(CExoString("TEMP:"));
}

// Unpacks a quicksave archive into the future-game slot.
bool CGuiInGame::CopyQuickSaveGameToFutureGame(unsigned int a, const CExoString& b)
{
    CExoString saveSlot;
    CExoString saveName;
    CExoString saveGameName;
    CExoString unused;

    if (a == 0xffffffff)
        saveName = b;
    else
        saveName.Format(const_cast<char*>("%06d - %s"), a, b.CStr());

    saveSlot = "SAVES:";
    saveSlot = saveSlot + saveName;

    g_pExoResMan->CreateDirectory2(CExoString("FUTUREGAME:"));
    g_pExoResMan->CleanDirectory(CExoString("FUTUREGAME:"), 0, 0);

#if KOTOR1  // K1: a forward-slash separator
    saveGameName = saveSlot + "/" + "SAVEGAME";
#else
    saveGameName = saveSlot + "\\" + "SAVEGAME";
#endif

    CERFFile* pErf = new CERFFile();
    pErf->Read(saveGameName);
    pErf->ReadHeaderVariance(saveGameName);
#if KOTOR1  // K1: the export's user pointer is 0
    pErf->ExportFilesFromERF(saveGameName, CExoString("FUTUREGAME:"), static_cast<void*>(0), LoadScreenUpdate, 0,
                              g_pAppManager->m_pClientExoApp->GetLoadStep(0) & 0xff);
#else
    pErf->ExportFilesFromERF(saveGameName, CExoString("FUTUREGAME:"), reinterpret_cast<void*>(-1), LoadScreenUpdate, 0,
                              g_pAppManager->m_pClientExoApp->GetLoadStep(0) & 0xff);
#endif
    pErf->~CERFFile();
    operator delete(pErf);

    return true;
}

bool CGuiInGame::UnpackQuickSaveGame(unsigned int a, const CExoString& b)
{
    CopyQuickSaveGameToFutureGame(a, b);

    CExoString gameInProgressPath =
        g_pExoBase->m_pAliasList->ResolveFileName("GAMEINPROGRESS:", RESTYPE_INVALID);
    CExoString futureGamePath =
        g_pExoBase->m_pAliasList->ResolveFileName("FUTUREGAME:", RESTYPE_INVALID);

    g_pExoResMan->NukeDirectory("GAMEINPROGRESS:", 1, 1);

#if KOTOR1  // K1: GetLength is out of line; the move goes through the port's _rename
    futureGamePath = futureGamePath.Left(futureGamePath.GetLength() - 1);
    gameInProgressPath = gameInProgressPath.Left(gameInProgressPath.GetLength() - 1);

    _rename(futureGamePath.CStr(), gameInProgressPath.CStr());
#else
    futureGamePath = futureGamePath.Left(
        futureGamePath.m_sString != 0 ? static_cast<int>(strlen(futureGamePath.m_sString)) - 1 : -1);
    gameInProgressPath = gameInProgressPath.Left(
        gameInProgressPath.m_sString != 0 ? static_cast<int>(strlen(gameInProgressPath.m_sString)) - 1 : -1);

    static_cast<void>(rename(futureGamePath.CStr(), gameInProgressPath.CStr()));
#endif

    return true;
}

void CGuiInGame::ResetInterfaceForSize()
{
    if (m_pMainInterface != 0) {
        delete m_pMainInterface;
        m_pMainInterface = new CSWGuiMainInterface(g_pAppManager->m_pClientExoApp->GetSWGuiManager());
    }
    if (m_pDialogCinematicPanel != 0) {
        delete m_pDialogCinematicPanel;
        m_pDialogCinematicPanel = new CSWGuiDialogCinematic(g_pAppManager->m_pClientExoApp->GetSWGuiManager());
    }
    if (m_pItemCreatePanel != 0) {
        m_pItemCreatePanel->CenterPanel();
    }
    if (m_pLoadModuleDebugPanel != 0) {
        m_pLoadModuleDebugPanel->CenterPanel();
    }
    if (m_pPowersFeatsSkillsDebugPanel != 0) {
        m_pPowersFeatsSkillsDebugPanel->CenterPanel();
    }
    if (m_pSubItemCreateMenuPanel != 0) {
        static_cast<CSWGuiPanel*>(m_pSubItemCreateMenuPanel)->CenterPanel();
    }
    if (m_pSoloModeQueryBox != 0) {
        delete m_pSoloModeQueryBox;
        m_pSoloModeQueryBox = new CSWGuiMessageBox(g_pAppManager->m_pClientExoApp->GetSWGuiManager());
    }
    if (m_pContainerGuiPanel != 0) {
        m_pContainerGuiPanel->CenterPanel();
    }
#if KOTOR1  // K1: the skill-info and tutorial boxes are re-centred, not rebuilt
    if (m_pSkillInfoBoxPanel != 0) {
        static_cast<CSWGuiPanel*>(m_pSkillInfoBoxPanel)->CenterPanel();
    }
    if (m_pTutorialBox != 0) {
        static_cast<CSWGuiPanel*>(m_pTutorialBox)->CenterPanel();
    }
#else
    if (m_pSkillInfoBoxPanel != 0) {
        delete m_pSkillInfoBoxPanel;
        m_pSkillInfoBoxPanel = new CSWGuiSkillInfoBox(g_pAppManager->m_pClientExoApp->GetSWGuiManager());
    }
    if (m_pTutorialBox != 0) {
        delete m_pTutorialBox;
        m_pTutorialBox = new CSWGuiTutorialBox(g_pAppManager->m_pClientExoApp->GetSWGuiManager());
    }
    if (m_pChemicalUpgradeScreenPanel != 0) {
        delete m_pChemicalUpgradeScreenPanel;
        m_pChemicalUpgradeScreenPanel = 0;
    }
    if (m_pUpgradeScreenPanel != 0) {
        delete m_pUpgradeScreenPanel;
        m_pUpgradeScreenPanel = 0;
    }
#endif
}

#if KOTOR1
void CGuiInGame::ShowPartySelection(const CExoString& text, int a, int forceNpc1, int forceNpc2)
{
    if (m_bInitialized == 0) {
        return;
    }

    CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
    if (pServerExoApp->GetPartyTable()->m_nControlledNPC != -1) {
        return;
    }
    if (pServerExoApp->GetCreatureByGameObjectID(pServerExoApp->GetPlayerCreatureId()) == 0) {
        return;
    }
    if (pServerExoApp->GetCreatureByGameObjectID(pServerExoApp->GetPlayerCreatureId())->GetArea() == 0) {
        return;
    }
    CSWArea* pArea = pServerExoApp->GetCreatureByGameObjectID(pServerExoApp->GetPlayerCreatureId())->GetArea();
    if (__builtin_expect(a == 0, 1) && static_cast<CSWSArea*>(pArea)->m_nUnescapable != 0) {
        return;
    }
    if (m_pGuiManager->PanelExists(m_pMessagesPanel)) {
        return;
    }

    if (a != 0) {
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
        m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_ADDED);
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
    }

    if (m_pMessagesPanel == 0) {
        m_pMessagesPanel = new CSWGuiPartySelection(m_pGuiManager);
    }
    static_cast<CSWGuiPartySelection*>(m_pMessagesPanel)->m_sExitScript = text;
    static_cast<CSWGuiPartySelection*>(m_pMessagesPanel)->m_bFromScript = a;
    static_cast<CSWGuiPartySelection*>(m_pMessagesPanel)->SetForcedNPC(forceNpc1, forceNpc2);
    static_cast<CSWGuiPartySelection*>(m_pMessagesPanel)->m_nReturnInputClass = (a == 0) << 1;
    m_pGuiManager->AddPanel(m_pMessagesPanel, 3, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}

int CGuiInGame::GetPartyAccessPanelUp(int bEquipOnly)
{
    if (m_pGuiManager == 0) return 0;
    if (m_pGuiManager->PanelExists(m_apScreenPanels[SCREEN_EQUIP])) return 1;
    if (bEquipOnly == 0 && m_pGuiManager->PanelExists(m_apScreenPanels[SCREEN_INVENTORY])) return 1;
    return 0;
}

int CGuiInGame::GetCurrentEquipScreenNPC()
{
    if (m_pGuiManager != 0 && m_pGuiManager->PanelExists(m_apScreenPanels[SCREEN_EQUIP])) {
        return static_cast<CSWGuiInGameEquip*>(m_apScreenPanels[SCREEN_EQUIP])->m_slot.m_nShownNPC;
    }
    return -1;
}
#endif // KOTOR1

#if !KOTOR1  // K1: the 4-argument body above
void CGuiInGame::ShowPartySelection(const CExoString& text, int a, int forceNpc1, int forceNpc2, int d)
{
    if (m_bInitialized == 0) {
        return;
    }

    CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
    if (pServerExoApp->GetPartyTable()->m_nControlledNPC != -1) {
        return;
    }
    if (pServerExoApp->GetCreatureByGameObjectID(pServerExoApp->GetPlayerCreatureId()) == 0) {
        return;
    }
    if (pServerExoApp->GetCreatureByGameObjectID(pServerExoApp->GetPlayerCreatureId())->GetArea() == 0) {
        return;
    }
    CSWArea* pArea = pServerExoApp->GetCreatureByGameObjectID(pServerExoApp->GetPlayerCreatureId())->GetArea();
    if (__builtin_expect(a == 0, 1) && static_cast<CSWSArea*>(pArea)->m_nUnescapable != 0) {
        return;
    }
    if (m_pGuiManager->PanelExists(m_apScreenPanels[SCREEN_PARTY_SELECTION])) {
        return;
    }

    if (a != 0) {
        if (m_bPaused == 0) {
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_TogglePauseRequest();
        }
        m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_ADDED);
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
#endif
    }

    CSWGuiPartySelection* pPanel = static_cast<CSWGuiPartySelection*>(m_apScreenPanels[SCREEN_PARTY_SELECTION]);
    if (pPanel == 0) {
        pPanel = new CSWGuiPartySelection(m_pGuiManager);
        m_apScreenPanels[SCREEN_PARTY_SELECTION] = pPanel;
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel = static_cast<CSWGuiPartySelection*>(m_apScreenPanels[SCREEN_PARTY_SELECTION]);
    }

    pPanel->m_sExitScript = text;
    pPanel->m_bFromScript = a;
    pPanel->SetForcedNPC(forceNpc1, forceNpc2);
    pPanel->m_fChallengeRating = d;
    pPanel->m_nReturnInputClass = 0;
    m_pGuiManager->AddPanel(pPanel, 3, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}
#endif

// Camera cut name for a dialog animation id.
CExoString CGuiInGame::GetCameraAnimationName(unsigned short nState)
{
    CExoString sName("none");
    if (nState == CAMERA_ANIMATION_NONE)
        return sName;
    switch (nState) {
    case 1000: sName = "cut001"; break;
    case 1001: sName = "cut002"; break;
    case 1002: sName = "cut003"; break;
    case 1003: sName = "cut004"; break;
    case 1004: sName = "cut005"; break;
    case 1005: sName = "cut006"; break;
    case 1006: sName = "cut007"; break;
    case 1007: sName = "cut008"; break;
    case 1008: sName = "cut009"; break;
    case 1009: sName = "cut010"; break;
    case 1010: sName = "cut011"; break;
    case 1011: sName = "cut012"; break;
    case 1012: sName = "cut013"; break;
    case 1013: sName = "cut014"; break;
    case 1014: sName = "cut015"; break;
    case 1015: sName = "cut016"; break;
    case 1016: sName = "cut017"; break;
    case 1017: sName = "cut018"; break;
    case 1018: sName = "cut019"; break;
    case 1019: sName = "cut020"; break;
    case 1020: sName = "cut021"; break;
    case 1021: sName = "cut022"; break;
    case 1022: sName = "cut023"; break;
    case 1023: sName = "cut024"; break;
    case 1024: sName = "cut025"; break;
    case 1025: sName = "cut026"; break;
    case 1026: sName = "cut027"; break;
    case 1027: sName = "cut028"; break;
    // The sequence says cut029, but no cut029 exists and every release maps 1028 (and 1228) to cut039;
    // left as is because mods may rely on it.
    case 1028: sName = "cut039"; break;
    case 1029: sName = "cut030"; break;
    case 1030: sName = "cut031"; break;
    case 1031: sName = "cut032"; break;
    case 1032: sName = "cut033"; break;
    case 1033: sName = "cut034"; break;
    case 1034: sName = "cut035"; break;
    case 1035: sName = "cut036"; break;
    case 1036: sName = "cut037"; break;
    case 1037: sName = "cut038"; break;
    case 1038: sName = "cut039"; break;
    case 1039: sName = "cut040"; break;
    case 1040: sName = "cut041"; break;
    case 1041: sName = "cut042"; break;
    case 1042: sName = "cut043"; break;
    case 1043: sName = "cut044"; break;
    case 1044: sName = "cut045"; break;
    case 1045: sName = "cut046"; break;
    case 1046: sName = "cut047"; break;
    case 1047: sName = "cut048"; break;
    case 1048: sName = "cut049"; break;
    case 1049: sName = "cut050"; break;
    case 1050: sName = "cut051"; break;
    case 1051: sName = "cut052"; break;
    case 1052: sName = "cut053"; break;
    case 1053: sName = "cut054"; break;
    case 1054: sName = "cut055"; break;
    case 1055: sName = "cut056"; break;
    case 1056: sName = "cut057"; break;
    case 1057: sName = "cut058"; break;
    case 1058: sName = "cut059"; break;
    case 1059: sName = "cut060"; break;
    case 1060: sName = "cut061"; break;
    case 1061: sName = "cut062"; break;
    case 1062: sName = "cut063"; break;
    case 1063: sName = "cut064"; break;
    case 1064: sName = "cut065"; break;
    case 1065: sName = "cut066"; break;
    case 1066: sName = "cut067"; break;
    case 1067: sName = "cut068"; break;
    case 1068: sName = "cut069"; break;
    case 1069: sName = "cut070"; break;
    case 1070: sName = "cut071"; break;
    case 1071: sName = "cut072"; break;
    case 1072: sName = "cut073"; break;
    case 1073: sName = "cut074"; break;
    case 1074: sName = "cut075"; break;
    case 1075: sName = "cut076"; break;
    case 1076: sName = "cut077"; break;
    case 1077: sName = "cut078"; break;
    case 1078: sName = "cut079"; break;
    case 1079: sName = "cut080"; break;
    case 1080: sName = "cut081"; break;
    case 1081: sName = "cut082"; break;
    case 1082: sName = "cut083"; break;
    case 1083: sName = "cut084"; break;
    case 1084: sName = "cut085"; break;
    case 1085: sName = "cut086"; break;
    case 1086: sName = "cut087"; break;
    case 1087: sName = "cut088"; break;
    case 1088: sName = "cut089"; break;
    case 1089: sName = "cut090"; break;
    case 1090: sName = "cut091"; break;
    case 1091: sName = "cut092"; break;
    case 1092: sName = "cut093"; break;
    case 1093: sName = "cut094"; break;
    case 1094: sName = "cut095"; break;
    case 1095: sName = "cut096"; break;
    case 1096: sName = "cut097"; break;
    case 1097: sName = "cut098"; break;
    case 1098: sName = "cut099"; break;
    case 1099: sName = "cut100"; break;
    case 1100: sName = "cut101"; break;
    case 1101: sName = "cut102"; break;
    case 1102: sName = "cut103"; break;
    case 1103: sName = "cut104"; break;
    case 1104: sName = "cut105"; break;
    case 1105: sName = "cut106"; break;
    case 1106: sName = "cut107"; break;
    case 1107: sName = "cut108"; break;
    case 1108: sName = "cut109"; break;
    case 1109: sName = "cut110"; break;
    case 1110: sName = "cut111"; break;
    case 1111: sName = "cut112"; break;
    case 1112: sName = "cut113"; break;
    case 1113: sName = "cut114"; break;
    case 1114: sName = "cut115"; break;
    case 1115: sName = "cut116"; break;
    case 1116: sName = "cut117"; break;
    case 1117: sName = "cut118"; break;
    case 1118: sName = "cut119"; break;
    case 1119: sName = "cut120"; break;
    case 1120: sName = "cut121"; break;
    case 1121: sName = "cut122"; break;
    case 1122: sName = "cut123"; break;
    case 1123: sName = "cut124"; break;
    case 1124: sName = "cut125"; break;
    case 1125: sName = "cut126"; break;
    case 1126: sName = "cut127"; break;
    case 1127: sName = "cut128"; break;
    case 1200: sName = "cut001w"; break;
    case 1201: sName = "cut002w"; break;
    case 1202: sName = "cut003w"; break;
    case 1203: sName = "cut004w"; break;
    case 1204: sName = "cut005w"; break;
    case 1205: sName = "cut006w"; break;
    case 1206: sName = "cut007w"; break;
    case 1207: sName = "cut008w"; break;
    case 1208: sName = "cut009w"; break;
    case 1209: sName = "cut010w"; break;
    case 1210: sName = "cut011w"; break;
    case 1211: sName = "cut012w"; break;
    case 1212: sName = "cut013w"; break;
    case 1213: sName = "cut014w"; break;
    case 1214: sName = "cut015w"; break;
    case 1215: sName = "cut016w"; break;
    case 1216: sName = "cut017w"; break;
    case 1217: sName = "cut018w"; break;
    case 1218: sName = "cut019w"; break;
    case 1219: sName = "cut020w"; break;
    case 1220: sName = "cut021w"; break;
    case 1221: sName = "cut022w"; break;
    case 1222: sName = "cut023w"; break;
    case 1223: sName = "cut024w"; break;
    case 1224: sName = "cut025w"; break;
    case 1225: sName = "cut026w"; break;
    case 1226: sName = "cut027w"; break;
    case 1227: sName = "cut028w"; break;
    case 1228: sName = "cut039w"; break;   // cut029w in sequence; see 1028
    case 1229: sName = "cut030w"; break;
    case 1230: sName = "cut031w"; break;
    case 1231: sName = "cut032w"; break;
    case 1232: sName = "cut033w"; break;
    case 1233: sName = "cut034w"; break;
    case 1234: sName = "cut035w"; break;
    case 1235: sName = "cut036w"; break;
    case 1236: sName = "cut037w"; break;
    case 1237: sName = "cut038w"; break;
    case 1238: sName = "cut039w"; break;
    case 1239: sName = "cut040w"; break;
    case 1240: sName = "cut041w"; break;
    case 1241: sName = "cut042w"; break;
    case 1242: sName = "cut043w"; break;
    case 1243: sName = "cut044w"; break;
    case 1244: sName = "cut045w"; break;
    case 1245: sName = "cut046w"; break;
    case 1246: sName = "cut047w"; break;
    case 1247: sName = "cut048w"; break;
    case 1248: sName = "cut049w"; break;
    case 1249: sName = "cut050w"; break;
    case 1250: sName = "cut051w"; break;
    case 1251: sName = "cut052w"; break;
    case 1252: sName = "cut053w"; break;
    case 1253: sName = "cut054w"; break;
    case 1254: sName = "cut055w"; break;
    case 1255: sName = "cut056w"; break;
    case 1256: sName = "cut057w"; break;
    case 1257: sName = "cut058w"; break;
    case 1258: sName = "cut059w"; break;
    case 1259: sName = "cut060w"; break;
    case 1260: sName = "cut061w"; break;
    case 1261: sName = "cut062w"; break;
    case 1262: sName = "cut063w"; break;
    case 1263: sName = "cut064w"; break;
    case 1264: sName = "cut065w"; break;
    case 1265: sName = "cut066w"; break;
    case 1266: sName = "cut067w"; break;
    case 1267: sName = "cut068w"; break;
    case 1268: sName = "cut069w"; break;
    case 1269: sName = "cut070w"; break;
    case 1270: sName = "cut071w"; break;
    case 1271: sName = "cut072w"; break;
    case 1272: sName = "cut073w"; break;
    case 1273: sName = "cut074w"; break;
    case 1274: sName = "cut075w"; break;
    case 1275: sName = "cut076w"; break;
    case 1276: sName = "cut077w"; break;
    case 1277: sName = "cut078w"; break;
    case 1278: sName = "cut079w"; break;
    case 1279: sName = "cut080w"; break;
    case 1280: sName = "cut081w"; break;
    case 1281: sName = "cut082w"; break;
    case 1282: sName = "cut083w"; break;
    case 1283: sName = "cut084w"; break;
    case 1284: sName = "cut085w"; break;
    case 1285: sName = "cut086w"; break;
    case 1286: sName = "cut087w"; break;
    case 1287: sName = "cut088w"; break;
    case 1288: sName = "cut089w"; break;
    case 1289: sName = "cut090w"; break;
    case 1290: sName = "cut091w"; break;
    case 1291: sName = "cut092w"; break;
    case 1292: sName = "cut093w"; break;
    case 1293: sName = "cut094w"; break;
    case 1294: sName = "cut095w"; break;
    case 1295: sName = "cut096w"; break;
    case 1296: sName = "cut097w"; break;
    case 1297: sName = "cut098w"; break;
    case 1298: sName = "cut099w"; break;
    case 1299: sName = "cut100w"; break;
    case 1300: sName = "cut101w"; break;
    case 1301: sName = "cut102w"; break;
    case 1302: sName = "cut103w"; break;
    case 1303: sName = "cut104w"; break;
    case 1304: sName = "cut105w"; break;
    case 1305: sName = "cut106w"; break;
    case 1306: sName = "cut107w"; break;
    case 1307: sName = "cut108w"; break;
    case 1308: sName = "cut109w"; break;
    case 1309: sName = "cut110w"; break;
    case 1310: sName = "cut111w"; break;
    case 1311: sName = "cut112w"; break;
    case 1312: sName = "cut113w"; break;
    case 1313: sName = "cut114w"; break;
    case 1314: sName = "cut115w"; break;
    case 1315: sName = "cut116w"; break;
    case 1316: sName = "cut117w"; break;
    case 1317: sName = "cut118w"; break;
    case 1318: sName = "cut119w"; break;
    case 1319: sName = "cut120w"; break;
    case 1320: sName = "cut121w"; break;
    case 1321: sName = "cut122w"; break;
    case 1322: sName = "cut123w"; break;
    case 1323: sName = "cut124w"; break;
    case 1324: sName = "cut125w"; break;
    case 1325: sName = "cut126w"; break;
    case 1326: sName = "cut127w"; break;
    case 1327: sName = "cut128w"; break;
    case 1400: sName = "cut001l"; break;
    case 1401: sName = "cut002l"; break;
    case 1402: sName = "cut003l"; break;
    case 1403: sName = "cut004l"; break;
    case 1404: sName = "cut005l"; break;
    case 1405: sName = "cut006l"; break;
    case 1406: sName = "cut007l"; break;
    case 1407: sName = "cut008l"; break;
    case 1408: sName = "cut009l"; break;
    case 1409: sName = "cut010l"; break;
    case 1410: sName = "cut011l"; break;
    case 1411: sName = "cut012l"; break;
    case 1412: sName = "cut013l"; break;
    case 1413: sName = "cut014l"; break;
    case 1414: sName = "cut015l"; break;
    case 1415: sName = "cut016l"; break;
    case 1416: sName = "cut017l"; break;
    case 1417: sName = "cut018l"; break;
    case 1418: sName = "cut019l"; break;
    case 1419: sName = "cut020l"; break;
    case 1420: sName = "cut021l"; break;
    case 1421: sName = "cut022l"; break;
    case 1422: sName = "cut023l"; break;
    case 1423: sName = "cut024l"; break;
    case 1424: sName = "cut025l"; break;
    case 1425: sName = "cut026l"; break;
    case 1426: sName = "cut027l"; break;
    case 1427: sName = "cut028l"; break;
    case 1428: sName = "cut039l"; break;
    case 1429: sName = "cut030l"; break;
    case 1430: sName = "cut031l"; break;
    case 1431: sName = "cut032l"; break;
    case 1432: sName = "cut033l"; break;
    case 1433: sName = "cut034l"; break;
    case 1434: sName = "cut035l"; break;
    case 1435: sName = "cut036l"; break;
    case 1436: sName = "cut037l"; break;
    case 1437: sName = "cut038l"; break;
    case 1438: sName = "cut039l"; break;
    case 1439: sName = "cut040l"; break;
    case 1440: sName = "cut041l"; break;
    case 1441: sName = "cut042l"; break;
    case 1442: sName = "cut043l"; break;
    case 1443: sName = "cut044l"; break;
    case 1444: sName = "cut045l"; break;
    case 1445: sName = "cut046l"; break;
    case 1446: sName = "cut047l"; break;
    case 1447: sName = "cut048l"; break;
    case 1448: sName = "cut049l"; break;
    case 1449: sName = "cut050l"; break;
    case 1450: sName = "cut051l"; break;
    case 1451: sName = "cut052l"; break;
    case 1452: sName = "cut053l"; break;
    case 1453: sName = "cut054l"; break;
    case 1454: sName = "cut055l"; break;
    case 1455: sName = "cut056l"; break;
    case 1456: sName = "cut057l"; break;
    case 1457: sName = "cut058l"; break;
    case 1458: sName = "cut059l"; break;
    case 1459: sName = "cut060l"; break;
    case 1460: sName = "cut061l"; break;
    case 1461: sName = "cut062l"; break;
    case 1462: sName = "cut063l"; break;
    case 1463: sName = "cut064l"; break;
    case 1464: sName = "cut065l"; break;
    case 1465: sName = "cut066l"; break;
    case 1466: sName = "cut067l"; break;
    case 1467: sName = "cut068l"; break;
    case 1468: sName = "cut069l"; break;
    case 1469: sName = "cut070l"; break;
    case 1470: sName = "cut071l"; break;
    case 1471: sName = "cut072l"; break;
    case 1472: sName = "cut073l"; break;
    case 1473: sName = "cut074l"; break;
    case 1474: sName = "cut075l"; break;
    case 1475: sName = "cut076l"; break;
    case 1476: sName = "cut077l"; break;
    case 1477: sName = "cut078l"; break;
    case 1478: sName = "cut079l"; break;
    case 1479: sName = "cut080l"; break;
    case 1480: sName = "cut081l"; break;
    case 1481: sName = "cut082l"; break;
    case 1482: sName = "cut083l"; break;
    case 1483: sName = "cut084l"; break;
    case 1484: sName = "cut085l"; break;
    case 1485: sName = "cut086l"; break;
    case 1486: sName = "cut087l"; break;
    case 1487: sName = "cut088l"; break;
    case 1488: sName = "cut089l"; break;
    case 1489: sName = "cut090l"; break;
    case 1490: sName = "cut091l"; break;
    case 1491: sName = "cut092l"; break;
    case 1492: sName = "cut093l"; break;
    case 1493: sName = "cut094l"; break;
    case 1494: sName = "cut095l"; break;
    case 1495: sName = "cut096l"; break;
    case 1496: sName = "cut097l"; break;
    case 1497: sName = "cut098l"; break;
    case 1498: sName = "cut099l"; break;
    case 1499: sName = "cut100l"; break;
    case 1500: sName = "cut101l"; break;
    case 1501: sName = "cut102l"; break;
    case 1502: sName = "cut103l"; break;
    case 1503: sName = "cut104l"; break;
    case 1504: sName = "cut105l"; break;
    case 1505: sName = "cut106l"; break;
    case 1506: sName = "cut107l"; break;
    case 1507: sName = "cut108l"; break;
    case 1508: sName = "cut109l"; break;
    case 1509: sName = "cut110l"; break;
    case 1510: sName = "cut111l"; break;
    case 1511: sName = "cut112l"; break;
    case 1512: sName = "cut113l"; break;
    case 1513: sName = "cut114l"; break;
    case 1514: sName = "cut115l"; break;
    case 1515: sName = "cut116l"; break;
    case 1516: sName = "cut117l"; break;
    case 1517: sName = "cut118l"; break;
    case 1518: sName = "cut119l"; break;
    case 1519: sName = "cut120l"; break;
    case 1520: sName = "cut121l"; break;
    case 1521: sName = "cut122l"; break;
    case 1522: sName = "cut123l"; break;
    case 1523: sName = "cut124l"; break;
    case 1524: sName = "cut125l"; break;
    case 1525: sName = "cut126l"; break;
    case 1526: sName = "cut127l"; break;
    case 1527: sName = "cut128l"; break;
    case 1600: sName = "cut001wl"; break;
    case 1601: sName = "cut002wl"; break;
    case 1602: sName = "cut003wl"; break;
    case 1603: sName = "cut004wl"; break;
    case 1604: sName = "cut005wl"; break;
    case 1605: sName = "cut006wl"; break;
    case 1606: sName = "cut007wl"; break;
    case 1607: sName = "cut008wl"; break;
    case 1608: sName = "cut009wl"; break;
    case 1609: sName = "cut010wl"; break;
    case 1610: sName = "cut011wl"; break;
    case 1611: sName = "cut012wl"; break;
    case 1612: sName = "cut013wl"; break;
    case 1613: sName = "cut014wl"; break;
    case 1614: sName = "cut015wl"; break;
    case 1615: sName = "cut016wl"; break;
    case 1616: sName = "cut017wl"; break;
    case 1617: sName = "cut018wl"; break;
    case 1618: sName = "cut019wl"; break;
    case 1619: sName = "cut020wl"; break;
    case 1620: sName = "cut021wl"; break;
    case 1621: sName = "cut022wl"; break;
    case 1622: sName = "cut023wl"; break;
    case 1623: sName = "cut024wl"; break;
    case 1624: sName = "cut025wl"; break;
    case 1625: sName = "cut026wl"; break;
    case 1626: sName = "cut027wl"; break;
    case 1627: sName = "cut028wl"; break;
    case 1628: sName = "cut039wl"; break;
    case 1629: sName = "cut030wl"; break;
    case 1630: sName = "cut031wl"; break;
    case 1631: sName = "cut032wl"; break;
    case 1632: sName = "cut033wl"; break;
    case 1633: sName = "cut034wl"; break;
    case 1634: sName = "cut035wl"; break;
    case 1635: sName = "cut036wl"; break;
    case 1636: sName = "cut037wl"; break;
    case 1637: sName = "cut038wl"; break;
    case 1638: sName = "cut039wl"; break;
    case 1639: sName = "cut040wl"; break;
    case 1640: sName = "cut041wl"; break;
    case 1641: sName = "cut042wl"; break;
    case 1642: sName = "cut043wl"; break;
    case 1643: sName = "cut044wl"; break;
    case 1644: sName = "cut045wl"; break;
    case 1645: sName = "cut046wl"; break;
    case 1646: sName = "cut047wl"; break;
    case 1647: sName = "cut048wl"; break;
    case 1648: sName = "cut049wl"; break;
    case 1649: sName = "cut050wl"; break;
    case 1650: sName = "cut051wl"; break;
    case 1651: sName = "cut052wl"; break;
    case 1652: sName = "cut053wl"; break;
    case 1653: sName = "cut054wl"; break;
    case 1654: sName = "cut055wl"; break;
    case 1655: sName = "cut056wl"; break;
    case 1656: sName = "cut057wl"; break;
    case 1657: sName = "cut058wl"; break;
    case 1658: sName = "cut059wl"; break;
    case 1659: sName = "cut060wl"; break;
    case 1660: sName = "cut061wl"; break;
    case 1661: sName = "cut062wl"; break;
    case 1662: sName = "cut063wl"; break;
    case 1663: sName = "cut064wl"; break;
    case 1664: sName = "cut065wl"; break;
    case 1665: sName = "cut066wl"; break;
    case 1666: sName = "cut067wl"; break;
    case 1667: sName = "cut068wl"; break;
    case 1668: sName = "cut069wl"; break;
    case 1669: sName = "cut070wl"; break;
    case 1670: sName = "cut071wl"; break;
    case 1671: sName = "cut072wl"; break;
    case 1672: sName = "cut073wl"; break;
    case 1673: sName = "cut074wl"; break;
    case 1674: sName = "cut075wl"; break;
    case 1675: sName = "cut076wl"; break;
    case 1676: sName = "cut077wl"; break;
    case 1677: sName = "cut078wl"; break;
    case 1678: sName = "cut079wl"; break;
    case 1679: sName = "cut080wl"; break;
    case 1680: sName = "cut081wl"; break;
    case 1681: sName = "cut082wl"; break;
    case 1682: sName = "cut083wl"; break;
    case 1683: sName = "cut084wl"; break;
    case 1684: sName = "cut085wl"; break;
    case 1685: sName = "cut086wl"; break;
    case 1686: sName = "cut087wl"; break;
    case 1687: sName = "cut088wl"; break;
    case 1688: sName = "cut089wl"; break;
    case 1689: sName = "cut090wl"; break;
    case 1690: sName = "cut091wl"; break;
    case 1691: sName = "cut092wl"; break;
    case 1692: sName = "cut093wl"; break;
    case 1693: sName = "cut094wl"; break;
    case 1694: sName = "cut095wl"; break;
    case 1695: sName = "cut096wl"; break;
    case 1696: sName = "cut097wl"; break;
    case 1697: sName = "cut098wl"; break;
    case 1698: sName = "cut099wl"; break;
    case 1699: sName = "cut100wl"; break;
    case 1700: sName = "cut101wl"; break;
    case 1701: sName = "cut102wl"; break;
    case 1702: sName = "cut103wl"; break;
    case 1703: sName = "cut104wl"; break;
    case 1704: sName = "cut105wl"; break;
    case 1705: sName = "cut106wl"; break;
    case 1706: sName = "cut107wl"; break;
    case 1707: sName = "cut108wl"; break;
    case 1708: sName = "cut109wl"; break;
    case 1709: sName = "cut110wl"; break;
    case 1710: sName = "cut111wl"; break;
    case 1711: sName = "cut112wl"; break;
    case 1712: sName = "cut113wl"; break;
    case 1713: sName = "cut114wl"; break;
    case 1714: sName = "cut115wl"; break;
    case 1715: sName = "cut116wl"; break;
    case 1716: sName = "cut117wl"; break;
    case 1717: sName = "cut118wl"; break;
    case 1718: sName = "cut119wl"; break;
    case 1719: sName = "cut120wl"; break;
    case 1720: sName = "cut121wl"; break;
    case 1721: sName = "cut122wl"; break;
    case 1722: sName = "cut123wl"; break;
    case 1723: sName = "cut124wl"; break;
    case 1724: sName = "cut125wl"; break;
    case 1725: sName = "cut126wl"; break;
    case 1726: sName = "cut127wl"; break;
    case 1727: sName = "cut128wl"; break;    }
    return sName;
}

int CGuiInGame::IsFireForgetDialogAnimation(unsigned short a)
{
    if (static_cast<unsigned short>(a >> 4) >= DIALOG_ANIM_2DA_BASE / 16 &&
        g_pRules->m_p2DArrays->m_pDialogAnimations->m_nRows + DIALOG_ANIM_2DA_BASE > a) {
        unsigned short row = static_cast<unsigned short>(a - DIALOG_ANIM_2DA_BASE);

        int fireForgetEntry;
        if (!g_pRules->m_p2DArrays->m_pDialogAnimations->GetINTEntry(
                row, CTwoDimArrays::COL2DA_DIALOGANIMATIONS_FIREFORGET, &fireForgetEntry)) {
            return 0;
        }

        int dialogEntry;
        if (!g_pRules->m_p2DArrays->m_pDialogAnimations->GetINTEntry(
                row, CTwoDimArrays::COL2DA_DIALOGANIMATIONS_DIALOG, &dialogEntry)) {
            return 0;
        }

        return fireForgetEntry != 0 && dialogEntry != 0;
    }

    int fireForgetEntry;
    int dialogEntry;
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            a, CTwoDimArrays::COL2DA_ANIMATIONS_FIREFORGET, &fireForgetEntry) &&
        g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            a, CTwoDimArrays::COL2DA_ANIMATIONS_DIALOG, &dialogEntry) &&
        fireForgetEntry != 0 && dialogEntry != 0) {
        return 1;
    }

    return static_cast<unsigned short>(a - STUNT_ANIM_FIREFORGET_FIRST) < STUNT_ANIM_COUNT;
}

int CGuiInGame::IsLoopingDialogAnimation(unsigned short a)
{
    if (static_cast<unsigned short>(a >> 4) >= DIALOG_ANIM_2DA_BASE / 16 &&
        g_pRules->m_p2DArrays->m_pDialogAnimations->m_nRows + DIALOG_ANIM_2DA_BASE > a) {
        unsigned short row = static_cast<unsigned short>(a - DIALOG_ANIM_2DA_BASE);

        int loopingEntry;
        if (!g_pRules->m_p2DArrays->m_pDialogAnimations->GetINTEntry(
                row, CTwoDimArrays::COL2DA_DIALOGANIMATIONS_LOOPING, &loopingEntry)) {
            return 0;
        }

        int dialogEntry;
        if (!g_pRules->m_p2DArrays->m_pDialogAnimations->GetINTEntry(
                row, CTwoDimArrays::COL2DA_DIALOGANIMATIONS_DIALOG, &dialogEntry)) {
            return 0;
        }

        return loopingEntry != 0 && dialogEntry != 0;
    }

    int loopingEntry;
    int dialogEntry;
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            a, CTwoDimArrays::COL2DA_ANIMATIONS_LOOPING, &loopingEntry) &&
        g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            a, CTwoDimArrays::COL2DA_ANIMATIONS_DIALOG, &dialogEntry) &&
        loopingEntry != 0 && dialogEntry != 0) {
        return 1;
    }

    return static_cast<unsigned short>(a - STUNT_ANIM_LOOPING_FIRST) < STUNT_ANIM_COUNT;
}
