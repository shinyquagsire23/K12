#include "k2stdint.h"
#include "port/sdlapi.h"
#include "ClientExoAppInternal.h"
#include "AndroidJNI.h"
#include "serveranimation.h"
#include "restype.h"

int g_bUseJoystick = 1;
int g_bLoggedIn;
#if !KOTOR1
__attribute__((visibility("hidden"))) float s_iosCtrlAxisScale = 115.0f;
__attribute__((visibility("hidden"))) float s_iosType1Scaler;
__attribute__((visibility("hidden"))) float s_iosType2Scaler;
#endif

extern int g_bLeftMouseDown;
extern int g_bShiftActionState;

#include "port/win32api.h"
#include "Win32Find.h"
// K1 never calls it, but under cl.exe the MSVC <string> below calls a sprintf_s the engine CRT
// does not declare, and binds to this one.
#if !KOTOR1 || defined(_MSC_VER)
static int sprintf_s(char* buf, uint64_t n, const char* fmt, ...);
#endif
void ChangeResolutionForMovie();
void RestoreResolutionFromMovie();
#include <string>
#include "port/libcbase.h"
#include "port/libcstdlib.h"

void SetClearColor(char* pszColor);

extern int g_bExitProgram;
const char* GetLocalizationFolderName();
void RevealAndroidWindowIfNeeded();

#include "clientmsgsysadmin.h"

#include "CExoArrayListImpl.h"
#include "nwchandlemessage.h"
#include "nwmessage.h"
#include "partytables.h"
#include "nwsrules.h"
#include "exoresman.h"
#include "nwscript.h"
#include "exosound.h"
#include "exosoundinternal.h"
#include "exoiniinternal.h"
#include "twodimarray.h"
#include "nwcjournal.h"
#include "nwcmodule.h"
#include "swminigame.h"
#include "swpattern.h"
#include "swguilegalscreen.h"

#include "appmanager.h"
#include "serverexoappinternal.h"
#include "ObjectLookupTable.h"
#include "exobase.h"
#include "exotimersinternal.h"
#include "worldtimer.h"
#include "gameobjectarray.h"
#include "nwcobject.h"
#include "AURORA/aurmodel.h"
#include "clientoptions.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "exoinputinternal.h"
#include "exomovieplayer.h"
#include "guiingame.h"
#include "swguiexamine.h"
#include "swguimaininterfacepc2.h"
#include "swgui.h"
#include "swguistartscreen.h"
#include "swguiingamecredits.h"
#include "nwcambientsound.h"

#include "aurorainterface.h"
#include "aurinternal.h"
#include "include/aurora.h"

void AurGetClearColor(float* pClearColor);
void AurSetClearColor(float r, float g, float b, float a);
void UpdateScreen(float deltaTime, int a, int b);

void AspyrSetupSDKs(unsigned char bConsent);
#include "serverexoapp.h"
#include "clientexoapp.h"
#include "swguipazaak.h"
#include "nwscreature.h"
#include "nwsmodule.h"
#include "pathfindinfo.h"
#include "nwsdialog.h"
#include "nwarea.h"
#include "nwcanimbase.h"
#include "swparty.h"
#include "swmgobject.h"
#include "swplayercontrol.h"
#include "aurcamera.h"
#include "swbehaviorcamera.h"
#include "AURORA/aurbehavior.h"
#include "exofile.h"
#include "aurscene.h"
#include "swcroom.h"

#include "List.h"
#include "nwcitem.h"
#include "nwccreature.h"
#include "nwccreatureappearance.h"
#include "nwcvisualeffect.h"
#include "nwccreaturestats.h"
#include "nwcarea.h"
#include "nwcplaceable.h"
#include "nwcdoor.h"
#include "nwctrigger.h"
#include "nwsplaceable.h"
#include "nwsarea.h"
#include "nwsplayer.h"
#include "nwitem.h"
#include "serverinfo.h"
#include "soundset.h"
#include "swguimainmenu.h"
#include "swguiloadscreen.h"
#include "netlayer.h"
#include "clientaimaster.h"
#include "swmgtrackfollower.h"
#include "nwscriptconst.h"
#include "nwsaiaction.h"
#include "soundprioritygroup.h"
#include "feedbackmessage.h"
#include "strref.h"

// An item's five model parts, counting down from the body (CSWCAnimBase::ANIM_MODEL_BODY).
enum {
    ITEM_MODEL_PARTS = 5,
};

// GetEquippedItemID's slot bits.
enum {
    EQUIP_SLOT_HEAD = 1 << INVENTORY_SLOT_HEAD,
    EQUIP_SLOT_RIGHT_WEAPON = 1 << INVENTORY_SLOT_RIGHTWEAPON,
    EQUIP_SLOT_LEFT_WEAPON = 1 << INVENTORY_SLOT_LEFTWEAPON,
};

// A client object id is its server id with the top bit of the low word set.
enum : unsigned long long { CLIENT_OBJECT_ID_FLAG = 0x80000000ull };

// CGuiInGame::AppendToMsgBuffer / AppendToCombatBuffer's message type for a feedback line.
enum { kMessageTypeFeedback = 0x80 };

// The poison bit of a damage-type mask, past nwscript's DAMAGE_TYPE_* bits.
enum { DAMAGE_FLAG_POISON = 0x2000 };

// LockRenderDev's "lock onto the player" request.
enum { kLockRenderDevPlayer = -9999 };

extern int RenderDEV;
extern int RenderQA;
int LockRenderDev;
extern int ShowPath;
#include "nwspell.h"
#include "nwsdoor.h"
#include "SWCollisionMesh.h"

void AurRemoveDrawableSquare(uint64_t nSquareId);
uint64_t AurAddDrawableSquare(float* pSquare, uint64_t nSquareId, const char* pFile, int nLine);
#if KOTOR1  // K1 takes no file and line arguments
uint64_t AurAddDrawableSquare(float* pSquare, uint64_t nSquareId);
#endif

#include <math.h>

extern int quaternionidentityoptimization;

extern CSWGuiManager* g_pGuiMan;
extern CTlkTable*      g_pTlkTable;

extern CExoSound* g_pExoSound;
int g_bShowIndicator = 1;
extern int enablemodelrequest;
extern void* g_pDialogIconModel;
extern void* g_pGroundRing;

extern bool g_bKeepHistory;
int CameraHorzAxisFlip;
extern int           g_bMouseCameraRotateToggle;
int g_bMouseCameraRotateToggleIos;
extern Vector        g_HitCheckBoxExpansion;
extern float         ios_controller_left_right;
extern float         ios_gamepad_analog1X;
extern int           ASLPlat_GetControllerPlugged();
#if KOTOR1
int CameraVertAxisFlip = 1;
extern float         mouseCameraMovement;
#endif


#include "port/crtapi.h"
#include "port/k2qolapi.h"
#include "exostringinline.h"

extern char* g_asMouseCursorNames[];

extern int g_nScreenWidth;
extern int g_nScreenHeight;
extern unsigned char g_fullScreenDesired;

extern int           g_nReInitializing;
extern int           g_bAllowWindowedMode;
void ReInitAurora(unsigned int nWidth, unsigned int nHeight, unsigned char nBpp,
                  int nFullscreen, int bAllow);
void messagepump();
#if KOTOR1  // K1 declares it as a byte
extern bool          g_bRenderWindowActive;
#else
extern int           g_bRenderWindowActive;
#endif

namespace MSG_Mac {
void CaptureMouse(bool b);
}

int CClientExoAppInternal::GetPauseState(unsigned char mask)
{
    return mask & m_pauseFlags;
}

CSWGuiStore* CClientExoAppInternal::GetStorePanel()
{
    return m_pInGameGui->m_pStorePanel;
}

void CClientExoAppInternal::InitializeInGameGUI()
{
    m_pInGameGui->Initialize();
}

CSWCModule* CClientExoAppInternal::GetModule()
{
    return m_pModule;
}

void CClientExoAppInternal::SetDebugMode(int mode)
{
    m_debugMode = mode;
}

int CClientExoAppInternal::GetDebugMode()
{
    return m_debugMode;
}

void CClientExoAppInternal::SetViewTriggers(int b)
{
    m_viewTriggers = b;
}

int CClientExoAppInternal::GetViewTriggers()
{
    return m_viewTriggers;
}

void CClientExoAppInternal::StopCreditSequence()
{
    m_creditSequence = 0;
}

void CClientExoAppInternal::ServerModeChange(unsigned char n)
{
    m_serverModeChange = n;
}

void CClientExoAppInternal::RestoreMousePositionTo(int x, int y)
{
    m_restoreMouseX = x;
    m_restoreMouseY = y;
}

int CClientExoAppInternal::GetActivePauseState()
{
    return (m_pauseFlags & 2) ? 2 : (m_pauseFlags & 1);
}

CAurCamera* CClientExoAppInternal::GetModuleCamera()
{
    if (m_pModule == 0)
        return 0;
    return m_pModule->m_pCamera;
}

void CClientExoAppInternal::UpdateMoviePlayer()
{
}

void CClientExoAppInternal::DisplayOptionsLoadError()
{
}

void CClientExoAppInternal::DestroyMainMenuPanel()
{
}

void CClientExoAppInternal::ShutDownLegalScreen()
{
    CSWGuiLegalScreen* pScreen = m_pLegalScreen;
    if (pScreen == 0)
        return;

    CResRef ref("");
    pScreen->SetImage(ref);
    m_pSWGuiManager->RemovePanel(m_pLegalScreen);

    if (m_pLegalScreen)
        delete m_pLegalScreen;
    m_pLegalScreen = 0;
}

int CClientExoAppInternal::AdmitNetworkAddress(unsigned int addr, const CExoString& s)
{
    K2_UNUSED(addr); K2_UNUSED(s);
    return 1;
}

#if KOTOR1
#if KOTOR1  // K1 reloads it at every use
extern volatile bool g_done;
#else
extern bool g_done;
#endif
#endif
void CClientExoAppInternal::ExitProgram()
{
#if KOTOR1  // K1 raises the port's quit flag
    g_done = true;
#else
    PostQuitMessage(0);
#endif
}

void CClientExoAppInternal::SetGameOver(int reason)
{
    m_nGameOverReason = reason;
    m_bGameOver = 1;
}

void CClientExoAppInternal::EnableVideoEffect(int nVideoEffect)
{
    float fSaturation = 0.0f;
    Vector vSaturation(0.0f, 0.0f, 0.0f);
    int nSaturation = 0, nScanNoise = 0;
#if !KOTOR1
    int nClairvoyance = 0;
    int nForceSight = 0;
    int nClairvoyanceFull = 0, nFury = 0;
#endif

    C2DA* pVideoEffects = g_pRules->m_p2DArrays->m_pVideoEffects;
    DisableVideoEffect();
    if (nVideoEffect < 0 || pVideoEffects == 0)
        return;

    int nOK = pVideoEffects->GetINTEntry(nVideoEffect, CExoString("EnableScanNoise"), &nScanNoise);
    if (nOK && nScanNoise) {
        m_videoEffectType = nVideoEffect;
        AurEnableScanNoise();
    }

    nOK = pVideoEffects->GetINTEntry(nVideoEffect, CExoString("EnableSaturation"), &nSaturation);
    if (nOK && nSaturation) {
        m_videoEffectType = nVideoEffect;
#if KOTOR1  // the columns have no _PC suffix, and there are no other effects
        pVideoEffects->GetFLOATEntry(nVideoEffect, CExoString("ModulationRed"), &vSaturation.x);
        pVideoEffects->GetFLOATEntry(nVideoEffect, CExoString("ModulationGreen"), &vSaturation.y);
        pVideoEffects->GetFLOATEntry(nVideoEffect, CExoString("ModulationBlue"), &vSaturation.z);
        pVideoEffects->GetFLOATEntry(nVideoEffect, CExoString("Saturation"), &fSaturation);
#else
        pVideoEffects->GetFLOATEntry(nVideoEffect, CExoString("ModulationRed_PC"), &vSaturation.x);
        pVideoEffects->GetFLOATEntry(nVideoEffect, CExoString("ModulationGreen_PC"), &vSaturation.y);
        pVideoEffects->GetFLOATEntry(nVideoEffect, CExoString("ModulationBlue_PC"), &vSaturation.z);
        pVideoEffects->GetFLOATEntry(nVideoEffect, CExoString("Saturation_PC"), &fSaturation);
#endif
        AurEnableSaturation();
        AurSetSaturationParams(vSaturation, fSaturation);
    }
#if !KOTOR1

    nOK = pVideoEffects->GetINTEntry(nVideoEffect, CExoString("EnableClairvoyance"), &nClairvoyance);
    if (nOK && nClairvoyance) {
        m_videoEffectType = nVideoEffect;
        AurEnableClairvoyance(true);
    }

    nOK = pVideoEffects->GetINTEntry(nVideoEffect, CExoString("EnableForceSight"), &nForceSight);
    if (nOK && nForceSight) {
        m_videoEffectType = nVideoEffect;
        AurEnableForceSight();
    }

    nOK = pVideoEffects->GetINTEntry(nVideoEffect, CExoString("EnableClairvoyanceFull"), &nClairvoyanceFull);
    if (nOK && nClairvoyanceFull) {
        m_videoEffectType = nVideoEffect;
        AurEnableClairvoyance(false);
    }

    nOK = pVideoEffects->GetINTEntry(nVideoEffect, CExoString("EnableFury"), &nFury);
    if (nOK && nFury) {
        m_videoEffectType = nVideoEffect;
        AurEnableForceFury(static_cast<short>(nFury));
    }
#endif
}

void CClientExoAppInternal::DelayNextEnemyAutoPause()
{
    if (m_fEnemyAutoPauseTimer <= 0.0f)
        m_fEnemyAutoPauseDelay = 5.0f;
}

int CClientExoAppInternal::IsMoviePlaying()
{
    if (m_pMoviePlayer == 0)
        return 0;
    return m_pMoviePlayer->IsMoviePlaying();
}

void CClientExoAppInternal::SetPlayerCreature(unsigned long long id)
{
    m_playerCreatureId = id;
    if (m_pParty)
        m_pParty->AddCharacter(id);
}

CExoString* CClientExoAppInternal::GetPlayerCharacterName()
{
    m_playerCharacterSet = 0;
    return &m_playerCharacterName;
}

int CClientExoAppInternal::GetPausedByCombat()
{
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (pServer == 0)
        return 0;
    return pServer->GetPauseState(2);
}

__attribute__((noinline))
void CClientExoAppInternal::SetLastTarget(unsigned long long id, int b)
{
    unsigned long long old = m_lastTarget;
    m_lastTarget = id;
    m_bLastTargetChanged = (old != id) ? (b & 1) : 0;
}

int TestNearestStruct(void* pObjectId, CNearestStruct nearest);

CSWCObject* CClientExoAppInternal::SelectNearestObject(int nForward, unsigned char nMode)
{
    if (m_pInGameGui->TutorialReasonWillShowWindow(TUTORIAL_CYCLE_TARGETS) &&
        m_pInGameGui->SetTutorialReason(TUTORIAL_CYCLE_TARGETS)) {
        m_tutorialObjA = OBJECT_INVALID;
        m_tutorialObjB = OBJECT_INVALID;
        m_tutorialTimerSpan = 0;
        m_tutorialReason = TUTORIAL_CYCLE_TARGETS;
    }

    if (m_visibleTable.m_count == 0)
        return 0;

    // A hostile-only pick first drops every row that no longer resolves or cannot be seen, from the
    // end, and keeps the filter only when a visible hostile creature is left.
    CSWCObject* pObject = 0;
    unsigned char nFilter = 0;
    if (nMode != 0) {
        for (int i = m_visibleTable.m_count - 1; i >= 0; i--) {
            CGameObject* pGameObject = GetGameObject(m_visibleTable.m_pArray[i].m_objectId);
            pObject = pGameObject != 0 ? pGameObject->AsSWCObject() : 0;
            if (pObject != 0) {
                unsigned char cCanSee = m_visibleTable.m_pArray[i].m_canSee;
                if (cCanSee == CNearestStruct::CANSEE_UNKNOWN) {
                    cCanSee = static_cast<unsigned char>(m_pParty->GetPlayerCharacter()->CanSee(pObject));
                    m_visibleTable.m_pArray[i].m_canSee = cCanSee;
                }
                if (cCanSee != 0) {
                    if (pObject->AsSWCCreature() != 0 && pObject->IsHostile() != 0) {
                        nFilter = nMode;
                        break;
                    }
                    continue;
                }
            }
            m_visibleTable.DelIndex(i);
        }
    }

    // Start one step past the current target, or at the first (last, walking forward) row.
    int nCount = m_visibleTable.m_count;
    int nIndex;
    if (m_lastTarget == OBJECT_INVALID) {
        nIndex = nForward != 0 ? nCount - 1 : 0;
    } else {
        nIndex = nCount - 1;
        int nCurrent = m_visibleTable.IndexOf(reinterpret_cast<void*>(m_lastTarget), TestNearestStruct);
        if (nCurrent >= 0 && nCurrent < nCount) {
            if (nForward == 0)
                nIndex = nCurrent < nIndex ? nCurrent + 1 : 0;
            else if (nCurrent != 0)
                nIndex = nCurrent - 1;
        } else if (nForward == 0) {
            nIndex = 0;
        }
    }

    // Walk at most one lap, dropping stale rows, until a row passes the filter and is visible.
    if (nCount != 0) {
        int nTries = nCount;
        do {
            CGameObject* pGameObject = GetGameObject(m_visibleTable.m_pArray[nIndex].m_objectId);
            pObject = pGameObject != 0 ? pGameObject->AsSWCObject() : 0;
            int bFound = 0;
            int bRemove = 0;
            if (pObject == 0) {
                bRemove = 1;
            } else {
                int bTest = 0;
                switch (nFilter) {
                case 0:
                    bTest = 1;
                    break;
                case 1:
                    bTest = pObject->IsHostile() == 0;
                    break;
                case 2:
                    bTest = pObject->AsSWCCreature() != 0 && pObject->IsHostile() != 0;
                    break;
                }
                if (bTest) {
                    unsigned char cCanSee = m_visibleTable.m_pArray[nIndex].m_canSee;
                    if (cCanSee == CNearestStruct::CANSEE_UNKNOWN) {
                        cCanSee = static_cast<unsigned char>(m_pParty->GetPlayerCharacter()->CanSee(pObject));
                        m_visibleTable.m_pArray[nIndex].m_canSee = cCanSee;
                    }
                    if (cCanSee == 0)
                        bRemove = 1;
                    else
                        bFound = 1;
                }
                if (!bRemove)
                    nIndex += (nForward == 0);
            }
            if (bRemove)
                m_visibleTable.DelIndex(nIndex);
            if (nForward == 0) {
                if (nIndex >= m_visibleTable.m_count)
                    nIndex = 0;
            } else if (nIndex <= 0 || nIndex > m_visibleTable.m_count) {
                nIndex = m_visibleTable.m_count - 1;
            } else {
                nIndex--;
            }
            if (bFound) {
                m_lastTarget = pObject->m_objectId;
                ShowObject(pObject, 1, 1, 1);
                return pObject;
            }
        } while (--nTries != 0 && m_visibleTable.m_count != 0);
    }

    if (m_visibleTable.m_count == 0) {
        // With no selection made in a non-combat pick, clear the target and look away.
        m_lastTarget = OBJECT_INVALID;
        m_pInGameGui->SetMainInterfaceTarget(OBJECT_INVALID, 0);
        CSWCCreature* pLeader = m_pParty->GetCharacter(0);
        if (pLeader != 0)
            pLeader->LookAt(OBJECT_INVALID, 10.0f);
    }
    return pObject;
}

void CClientExoAppInternal::UpdateHostileHilite(float fDelta)
{
    unsigned char cSuccess = CGameObjectArray::SUCCESS;
    unsigned char cCreatureType = CGameObject::OBJECT_TYPE_CREATURE;
    for (int i = m_hiliteList.m_count; i-- > 0; ) {
        CHostileHilite& rRow = m_hiliteList.m_pArray[i];
        rRow.m_fHiliteSize -= fDelta;
        if (!(rRow.m_fHiliteSize <= 0.0f))
            continue;

        CGameObject* pObj = 0;
        unsigned char cOk = m_pGameObjectArray->GetGameObject(rRow.m_dwObjectId, &pObj);
        if (cSuccess == cOk && pObj != 0 && pObj->m_objectType == cCreatureType) {
            CSWCCreature* pCreature = pObj->AsSWCCreature();
            if (pCreature)
                pCreature->UnHilite(0);
        }

        m_hiliteList.DelIndex(i);
    }
}

void CClientExoAppInternal::UpdateCombatMode()
{
    int iChar = 0;
    CSWCObject* pplayer = 0;
    CServerExoApp* pServer = 0;

    if (m_pParty->m_nCharacterCount < 1)
        goto creature_cache;
    pServer = g_pAppManager->m_pServerExoApp;
    for (;;) {
        CSWCCreature* pPlayer = m_pParty->GetCharacter(iChar);
        if (pPlayer == 0)
            goto loop_cont;
        if ((pPlayer->m_nStateFlags & 1) == 0) {
            CSWSCreature* pServerC = pPlayer->GetServerCreature();
            if (pServerC == 0)
                goto loop_cont;
            {
                unsigned long long uAtkA = pServerC->m_attemptedAttackTargetId;
                unsigned long long uAtkB = pServerC->m_oidAttemptedSpellTarget;
                if (uAtkA != OBJECT_INVALID) {
                    CGameObject* pObj = pServer->GetGameObject(uAtkA);
                    if (pObj != 0 && pObj->AsSWSObject() != 0) {
                        if (pObj->AsSWSObject()->GetDead() == 0)
                            goto set_mode1;
                    }
                }
                if (uAtkB != OBJECT_INVALID && uAtkB != uAtkA && uAtkB != pServerC->m_objectId) {
                    CGameObject* pObj = pServer->GetGameObject(uAtkB);
                    if (pObj != 0 && pObj->AsSWSObject() != 0) {
                        if (pObj->AsSWSObject()->GetDead() == 0)
                            goto set_mode1;
                    }
                }
                goto loop_cont;
            }
        set_mode1:
            pPlayer->SetCombatMode(1);
            goto loop_cont;
        }

        {
            CSWCCreature* pLocalC =
                GetCreatureByGameObjectID(m_pParty->m_members[iChar].m_oidLastTarget);
            int bLocal = pLocalC != 0;
            if (!bLocal) {
                CGameObject* pObject = GetGameObject(m_pParty->m_members[iChar].m_oidLastTarget);
                if (pObject != 0)
                    pplayer = pObject->AsSWCObject();
            }
            // On iteration 0, back off while the target is changing, then skip hostiles.
            if (iChar == 0 && m_fTargetChanging > 0.0f)
                goto loop_cont;
            if (bLocal && pLocalC->IsHostile())
                goto loop_cont;
        }
        if (pplayer != 0) {
            CSWCDoor* pDoor = pplayer->AsSWCDoor();
            if (pDoor != 0 && pplayer->AsSWCDoor()->GetServerDoor() != 0 &&
                pplayer->AsSWCDoor()->GetServerDoor()->m_bLocked != 0)
                goto loop_cont;
            CSWCPlaceable* pPlace = pplayer->AsSWCPlaceable();
            if (pPlace != 0 && pplayer->AsSWCPlaceable()->GetServerPlaceable() != 0 &&
                pplayer->AsSWCPlaceable()->GetServerPlaceable()->m_bLocked != 0)
                goto loop_cont;
        }

        {
            CSWSCreature* pServerC = pPlayer->GetServerCreature();
            if (pServerC == 0)
                goto set_mode0;
            CGameObject* pObj = 0;
            unsigned long long uA1 = pServerC->m_attemptedAttackTargetId;
            unsigned long long uA2 = pServerC->m_oidAttemptedSpellTarget;
            unsigned long long uA3 = pServerC->m_oidGoingToBeAttackedBy;
            if (uA1 != OBJECT_INVALID) {
                CGameObject* pTarget1 = pServer->GetGameObject(uA1);
                if (pTarget1 != 0 && pTarget1->AsSWSObject() != 0) {
                    if (pTarget1->AsSWSObject()->GetDead() == 0)
                        goto loop_cont;
                }
            }
            if (uA2 != OBJECT_INVALID && uA2 != uA1) {
                CGameObject* pTarget2 = pServer->GetGameObject(uA2);
                if (pTarget2 != 0 && pTarget2->AsSWSObject() != 0) {
                    if (pTarget2->AsSWSObject()->GetDead() == 0)
                        goto loop_cont;
                }
            }
            if (!(uA3 != uA2 && uA3 != OBJECT_INVALID && uA3 != uA1))
                goto set_mode0;
            pObj = pServer->GetGameObject(uA3);
            if (pObj == 0 || pObj->AsSWSObject() == 0)
                goto set_mode0;
            if (pObj->AsSWSObject()->GetDead() != 0)
                goto set_mode0;
#ifdef K2_FIX_ENGINE_BUGS
            // Only the attacker's death clears m_oidGoingToBeAttackedBy (EndCombatRound), so a
            // hostile that announced an attack and then went idle held the party in combat mode.
            {
                CSWSCreature* pAttacker = pObj->AsSWSCreature();
                if (pAttacker != 0 && pAttacker->m_oidAttackTarget == OBJECT_INVALID
                    && pAttacker->m_attemptedAttackTargetId == OBJECT_INVALID
                    && pAttacker->m_actionList.m_pInternal->m_nCount == 0) {
                    pServerC->m_oidGoingToBeAttackedBy = OBJECT_INVALID;
                    goto set_mode0;
                }
            }
#endif
            {
                CSWSCreature* pC2 = pObj->AsSWSCreature();
                if (pC2 == 0)
                    goto loop_cont;
                CSWCCreature* pCli = pObj->AsSWSCreature()->GetClientCreature();
                if (pCli == 0)
                    goto loop_cont;
                if (pCli->IsHostile() != 0)
                    goto loop_cont;
            }
        }
set_mode0:
        m_fEnemySightedTimer = -1.0f;
        pPlayer->SetCombatMode(0);

loop_cont:
        iChar++;
        if (iChar < m_pParty->m_nCharacterCount)
            continue;
        break;
    }

creature_cache:
    if (m_oidCreatureCache == m_playerCreatureId) {
        // The cache is still valid.
    } else {
        m_pCreatureCache = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            m_playerCreatureId);
        m_oidCreatureCache = m_playerCreatureId;
    }
    if (m_pCreatureCache != 0 && (static_cast<CSWCCreature*>(m_pCreatureCache)->m_nStateFlags & 1) != 0) {
        int iIndex = m_pParty->GetIndex(m_oidCreatureCache);
#if KOTOR1  // K1's "no combat message" strref is 48208
        if (iIndex >= 0 &&
            m_pParty->m_members[iIndex].m_nCombatMessageStrRef != STRREF_COMBAT_MODE_ENGAGED) {
#else
        if (iIndex >= 0 &&
            m_pParty->m_members[iIndex].m_nCombatMessageStrRef != STRREF_COMBAT_MODE_ENGAGED) {
#endif
            m_pInGameGui->SetCombatMessage(m_pParty->m_members[iIndex].m_nCombatMessageStrRef);
        }
    }
}

int CClientExoAppInternal::Initialize()
{
    if (m_pLoadScreenObj == 0)
        m_pLoadScreenObj = new CSWGuiLoadScreen(m_pSWGuiManager);

    CSWRules* pRules = g_pRules;
    if (pRules == 0) {
        pRules = new CSWRules();
        g_pRules = pRules;
    }

    for (unsigned char i = 0; i < g_pRules->m_p2DArrays->m_pSurfaceMaterial->m_nRows; ++i) {
        C2DA* pSurface = g_pRules->m_p2DArrays->m_pSurfaceMaterial;
        int nMaskBit;
        {
            CExoString szColumn("Walk");
            pSurface->GetINTEntry(i, szColumn, &nMaskBit);
        }
        m_hitCheckMaterialMask |= nMaskBit << i;
    }

    m_pInGameGui->InitializeGlobal();
    SetEventDescriptions();
    if (m_pMoviePlayer != 0 && m_pMoviePlayer->IsMoviePlaying() != 0) {
        m_pInGameGui->SetGlobalUnpauseableState(1);
        g_pExoInput->ClearEvents();
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL, 0);
    }
    m_bClientInitialized = 1;
    return 1;
}

int CClientExoAppInternal::HandleMessage(unsigned int nType, unsigned char* pMessage,
                                         unsigned int nSize, int nArg4)
{
    if (g_pAppManager->m_pClientExoApp->GetCaptureEvents() != 0) {
        if (g_pAppManager->m_pClientExoApp->GetCaptureEventFile() != 0) {
            CExoString sText;  // default ctor then Format'ed in place
            sText.Format(const_cast<char*>("%d %c %d %d %d "),
                         static_cast<unsigned long long>(g_pAppManager->m_pClientExoApp->GetWorldTimer()
                             ->GetSnapshotTime()) /
                             1000,
                         'M', nType, nArg4, nSize);
            static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile())->Write(sText);
            static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile())
                ->Write(pMessage, nSize, 1);
            sText.Format(const_cast<char*>("\n"));
            static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile())->Write(sText);
        }
    }

    if (nArg4 == 0) {
        if (pMessage[0] == 'P')
            m_pSWCMessage->HandleServerToPlayerMessage(pMessage, nSize);
        if (pMessage[0] == 'S')
            m_pSysAdminMessenger->HandleServerToSysAdminMessage(pMessage, nSize);
    }
    return 1;
}

void CClientExoAppInternal::UpdateSoundEngine()
{
    if (m_pModule == 0)
        goto lblRender;

    CAurCamera* pCamera;
    pCamera = m_pModule->m_pCamera;
    CSWCCreature* pPlayer;
    if (m_oidCreatureCache == m_playerCreatureId) {
        pPlayer = static_cast<CSWCCreature*>(m_pCreatureCache);
    } else {
        pPlayer = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_playerCreatureId);
        m_pCreatureCache = pPlayer;
        m_oidCreatureCache = m_playerCreatureId;
    }

    if (pCamera == 0)
        goto lblRender;

    CSWMiniGame* pMiniGame;
    pMiniGame = static_cast<CSWMiniGame*>(m_pModule->GetMiniGame());
    if (pMiniGame != 0 && pMiniGame->m_pPlayer != 0) {
        pMiniGame->m_pPlayer->GetPosition();
    } else if (pPlayer == 0) {
        goto lblRender;
    }

    {
        Vector vListener;
        Vector vFwd;
        vListener = pCamera->GetPosition();
        Quaternion q = pCamera->GetOrientation();

        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick != 0) {
            Vector vTarget = pOnAStick->GetTargetHeadPosition();
            vListener.x += (vTarget.x - vListener.x) * 0.75f;
            vListener.y += (vTarget.y - vListener.y) * 0.75f;
            vListener.z += (vTarget.z - vListener.z) * 0.75f;
        } else {
            CSWBehaviorCameraCombat* pCombat = static_cast<CSWBehaviorCameraCombat*>(pCamera->GetBehavior(BEHAVIOR_CAMERA_COMBAT));
            if (pCombat != 0) {
                CSWCCreature* pChar = m_pParty->GetCharacter(0);
#if KOTOR1  // the party leader is used without a null check
                {
#else
                if (pChar != 0) {
#endif
                    Vector vTarget = { pChar->m_position.x, pChar->m_position.y,
                                       pChar->m_position.z + pChar->GetHeadHeightOffset() };
                    vListener.x += (vTarget.x - vListener.x) * 0.75f;
                    vListener.y += (vTarget.y - vListener.y) * 0.75f;
                    vListener.z += (vTarget.z - vListener.z) * 0.75f;
                }
            } else {
                CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
                if (pGui->m_nGlobalDialogState != 0) {
                    pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
                    if (pGui->m_lastShotValid != 0) {
                        pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
                        vListener = vListener - q.zdir() * pGui->m_lastShotD;
                    }
                }
            }
        }

        g_pExoSound->SetListenerPosition(vListener);

        vFwd = -q.zdir();
        Vector vUp = q.ydir();
        g_pExoSound->SetListenerOrientation(vFwd, vUp);
    }

lblRender:
    g_pExoSound->Render();
}

void CClientExoAppInternal::UpdateRumble(float fRumble)
{
    float fLow = 0.0f;
    float fHigh;
    if (fRumble > 0.0f && m_bAreaNotReadyToDisplay == 0 &&
        m_pInGameGui->IsGlobalFadeObscuring() == 0 &&
        m_pInGameGui->IsGlobalFading() == 0) {
        if (m_pLoadScreenObj == 0 || m_pSWGuiManager == 0 ||
            m_pSWGuiManager->PanelExists(m_pLoadScreenObj) == 0) {
            int nCount = m_rumblePatterns.m_count;
            fHigh = 0.0f;
            if (nCount > 0) {
                fLow = 0.0f;
                for (int i = nCount - 1; i >= 0; --i) {
                    SRumblePattern& r = m_rumblePatterns.m_pArray[i];
                    float fPos;
                    if (r.nState != 0) {
                        r.nState = 0;
                        fPos = r.fPhase;
                    } else {
                        r.fPhase += fRumble;
                        fPos = r.fPhase;
                    }
                    float fMagLow, fMagHigh;
                    if (m_rumblePatternList.m_pArray[r.nPattern].GetMagnitudes(fPos, fMagLow, fMagHigh) != 0) {
                        fLow = (fLow > fMagLow) ? fLow : fMagLow;
                        fHigh = (fHigh > fMagHigh) ? fHigh : fMagHigh;
                    } else {
                        m_rumblePatterns.DelIndex(i);
                    }
                }
            }
        } else {
            fHigh = 0.0f;
        }
    } else {
        fHigh = 0.0f;
    }
    g_pExoInput->SetRumble(0, fLow, fHigh, 600000);
}

void CClientExoAppInternal::SetAutoPaused(int nAutoPaused, unsigned char nLevel)
{
    if (m_bAutoPaused == nAutoPaused)
        return;

    if (nAutoPaused != 0) {
        if (g_pAppManager->m_pServerExoApp != 0 &&
            g_pAppManager->m_pServerExoApp->GetPauseState(2) != 0)
            return;

        if (m_fEnemyAutoPauseDelay > 0.0f) {
            m_nPendingAutoPauseReason = nLevel;
            m_fEnemyAutoPauseDelay = 0.0f;
            m_fEnemyAutoPauseTimer = 1.0f;
            return;
        }

#if KOTOR1  // the pause itself is SetPausedByCombat's (no mode-13 / mode-3 cases)
        if (m_pInGameGui->m_nGlobalDialogState != 0)
            return;
        if (nLevel == PAUSE_MODE_PARTY_MEMBER_DOWN) {
            if (m_fPartyDownPauseCooldown > 0.0f)
                return;
            m_fPartyDownPauseCooldown = 2.0f;
        }
        m_bAutoPaused = 1;
        SetPausedByCombat(1, nLevel, 0);
        return;
    }
#else
        if (m_pInGameGui->m_nGlobalDialogState != 0)
            return;
        if (nLevel == PAUSE_MODE_PARTY_MEMBER_DOWN) {
            if (m_fPartyDownPauseCooldown > 0.0f)
                return;
            m_fPartyDownPauseCooldown = 2.0f;
            m_bAutoPaused = 1;
            if (m_pInGameGui->m_nGlobalDialogState != 0)
                return;
        } else {
            m_bAutoPaused = 1;
            if (nLevel != PAUSE_MODE_HELP) {
                if (m_pInGameGui->m_nGlobalDialogState != 0)
                    return;
            }
        }

        // Both paths end here, and only actually grip the pause if the engine isn't already
        // server-paused.
        if (g_pAppManager->m_pServerExoApp != 0 &&
            g_pAppManager->m_pServerExoApp->GetPauseState(2) == 1)
            return;
        if (m_bTogglePauseAtEndOfFrame)
            return;

        m_combatPauseMode = nLevel;
        m_bTogglePauseAtEndOfFrame = 1;
        m_combatPauseRequest = 1;
        if (nLevel == PAUSE_MODE_SILENT)
            return;
        g_pExoInput->PauseRumble();
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE, 0);
        m_bSoundPausedByCombat = 1;
        return;
    }
#endif

    m_bAutoPaused = 0;
    if (g_pAppManager->m_pServerExoApp != 0 &&
        g_pAppManager->m_pServerExoApp->GetPauseState(2) != 0)
    {
        if (m_pInGameGui->m_nGlobalDialogState > 1)
            return;
        if (g_pAppManager->m_pServerExoApp != 0 &&
            g_pAppManager->m_pServerExoApp->GetPauseState(2) != 0)
        {
            if (m_bTogglePauseAtEndOfFrame)
                return;
            m_combatPauseMode = 0;
            m_bTogglePauseAtEndOfFrame = 1;
            m_combatPauseRequest = 0;
            g_pExoInput->UnpauseRumble();
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
            m_bSoundPausedByCombat = 0;
        }
    }
}

void CClientExoAppInternal::PlayerListChange(unsigned int a, int b, int c)
{
    K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c);
}

void CClientExoAppInternal::OnCDChange()
{
}

void CClientExoAppInternal::OnExit()
{
}

void CClientExoAppInternal::OnGainFocus()
{
}

void CClientExoAppInternal::OnLostFocus()
{
}

void CClientExoAppInternal::OnPlayModulePanelExit(int a)
{
    K2_UNUSED(a);
}

void CClientExoAppInternal::OnVideoChange()
{
}

void CClientExoAppInternal::HandleCDKeyPanelExit(int a)
{
    K2_UNUSED(a);
}

void CClientExoAppInternal::EndLoadBar()
{
}

void CClientExoAppInternal::HandleLoadBarExit(int a)
{
    K2_UNUSED(a);
}

CGuiAreaLoadscreen* CClientExoAppInternal::StartLoadBar(int a, char* b, int c)
{
    K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c);
    return 0;
}

int CClientExoAppInternal::ContinueMessageProcessing()
{
    return 1;
}

void CClientExoAppInternal::ShutDownToMainMenu()
{
    CGameObject* pOutObject = 0;
    if (m_bWMCLOSEQuery)
        m_bQuitClicked = 1;

    g_pAppManager->DestroyServer();

    if (m_pModule) {
        CSWCArea* pArea = m_pModule->m_pArea;
        if (pArea) {
            m_pGameObjectArray->Delete(pArea->m_objectId, &pOutObject);
            pArea->UnloadArea();
        }
        UnloadModule();
    }

    if (!m_bWMCLOSEQuery) {
#ifdef K2_FIX_ENGINE_BUGS
        int nMenuArg = 0;
#else
        int nMenuArg;  // vestigial, no caller passes it
#endif
        DisplayMainMenu(nMenuArg);
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
        m_bStartMenuMusicOnMovieEnd = 1;
#if KOTOR1  // the value is a literal 1
        StartMenuMusic(1, 127, 2);
#else
        StartMenuMusic(nMenuArg, 127, 2);
#endif

        if (m_loadBarState) {
            CLoadGameErrorInfo* pInfo = g_pAppManager->m_pLoadGameErrorInfo;
            pInfo->m_bStallFinished = 0;
            pInfo->m_flag = 0;
            pInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_NONE;
            m_loadBarState = 0;
        }
    }
}

int CClientExoAppInternal::IsHostingServer()
{
    return 1;
}

void CClientExoAppInternal::SendSaveGameRequest(unsigned int nSlot, const CExoString& sName,
                                                const CExoString& sDesc)
{
    if (m_pSWCMessage)
        m_pSWCMessage->SendPlayerToServerModule_SaveGame(nSlot, sName, sDesc);
}

void CClientExoAppInternal::SendLoadGameRequest(unsigned int nSlot, const CExoString& sName,
                                                const CExoString& sDesc)
{
    if (m_pSWCMessage)
        m_pSWCMessage->SendPlayerToServerModule_LoadGame(nSlot, sName, sDesc);
}

void CClientExoAppInternal::SendCharacterResponse(int bAccept)
{
    if (m_pSWCMessage)
        m_pSWCMessage->SendPlayerToServerLogin_CharacterResponse(bAccept);
}

void CClientExoAppInternal::ShowServerMem()
{
    if (m_pSWCMessage)
        m_pSWCMessage->SendPlayerToServerCheat_ShowServerMem();
}

void CClientExoAppInternal::HideMovieFadePanel()
{
    if (m_pMoviePlayer)
        m_pMoviePlayer->HideFadePanel();
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CClientExoAppInternal::ResumeStreamMusicForMovie()
{
    g_pExoSound->ResumeAllStreamingMusicForMovie();
}
#endif // !KOTOR1

void CClientExoAppInternal::PauseAllSoundsForMovieLostFocus()
{
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_LOSTFOCUS, 0);
}

void CClientExoAppInternal::StartCreditSequence(const Vector& vPos, const CExoString& sText)
{
    if (m_creditSequence)
        return;

    CSWGuiInGameCredits* pCredits = new CSWGuiInGameCredits(g_pGuiMan, sText);
    if (!g_pGuiMan->PanelExists(pCredits))
        g_pGuiMan->AddPanel(pCredits, m_bEvilEndingHackInProgress ? 1 : 3, 1);
    pCredits->m_vColor = vPos;

    if (m_pModule && m_pModule->m_pArea && m_pModule->m_pArea->m_pAmbientSound)
        m_pModule->m_pArea->m_pAmbientSound->StopSounds(0);

    m_creditSequence = 1;
}

void CClientExoAppInternal::ClearCameraInput()
{
    m_pModule->TurnCamera(0.0f, 0.0f);
    m_pModule->AcclTurnCamera(0.0f, 0.0f);
}

CSWCModule* CClientExoAppInternal::CreateModule()
{
    CSWCModule* pModule = new CSWCModule;
    m_pModule = pModule;
    return pModule;
}

void CClientExoAppInternal::CreateQuestJournal()
{
    if (m_pQuestJournal == 0)
        m_pQuestJournal = new CSWCJournal;
}

void CClientExoAppInternal::CreateWorldJournal()
{
    if (m_pWorldJournal == 0)
        m_pWorldJournal = new CSWCWorldJournal;
}

void CClientExoAppInternal::DestroyQuestJournal()
{
    CSWCJournal* p = m_pQuestJournal;
    if (!p)
        return;

    delete p;
    m_pQuestJournal = 0;
}

void CClientExoAppInternal::DestroyWorldJournal()
{
    CSWCWorldJournal* p = m_pWorldJournal;
    if (p) {
        p->~CSWCWorldJournal();
        operator delete(p);
        m_pWorldJournal = 0;
    }
}

void CClientExoAppInternal::UnloadModule()
{
    SetClearColor(const_cast<char*>("0 0 0"));

    m_playerCreatureId = CGameObjectArray::INVALIDOBJID;
    if (m_pParty)
        m_pParty->AddCharacter(CGameObjectArray::INVALIDOBJID);

    unsigned int nGuiState = m_pInGameGui->m_nGlobalDialogState;
    if (nGuiState <= 1 && g_pAppManager->m_pServerExoApp
        && g_pAppManager->m_pServerExoApp->GetPauseState(2)
        && !m_bTogglePauseAtEndOfFrame) {
        m_bTogglePauseAtEndOfFrame = 1;
        m_combatPauseMode = 0;
        m_combatPauseRequest = 0;
        g_pExoInput->UnpauseRumble();
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
        m_bSoundPausedByCombat = 0;
    }

    SetPauseState(2, 0, 0);
    SetPauseState(1, 0, 0);
    ClearPauseExclusionList();

    if (!m_pModule)
        return;
    delete m_pModule;
    m_pModule = 0;

    g_pAppManager->GetObjectTableManager(0)->RemovePlayer(0);

    if (m_pGameObjectArray) {
        CGameObjectArray* pGameObjectArray = m_pGameObjectArray;
        pGameObjectArray->~CGameObjectArray();
        operator delete(pGameObjectArray);
    }

    m_pWalkToRingVFX = 0;
    if (m_pWorldJournal)
        m_pWorldJournal->DeleteAllEntries();
    if (m_pQuestJournal)
        m_pQuestJournal->ClearAll();
    if (m_pInGameGui)
        m_pInGameGui->ShutDown();
    if (m_pPlayerControl)
        m_pPlayerControl->Initialize();

    m_pGameObjectArray = new CGameObjectArray(1);
    g_pAppManager->m_pClientExoApp->SetAreaNotReadyToDisplay(1);

    m_nPendingAutoPauseReason = PAUSE_MODE_NO_PENDING;
    m_fEnemySightedTimer = 0;
    m_fTrapSightedTimer = 0;
    m_bTrapSighted = 0;
    m_fEnemyAutoPauseDelay = 0;
    m_fEnemyAutoPauseTimer = 0;
    m_bAutoPaused = 0;
    m_combatMode = 0;
    m_bEnemySighted = 0;
}

void CClientExoAppInternal::JournalUpdated(int nType, int nId, const CExoString& sEntry)
{
    if (m_pQuestJournal)
        m_pQuestJournal->JournalUpdated(nType, nId, sEntry);
}

void CClientExoAppInternal::ClearCachedCreatures(CSWCCreature* pCreature)
{
    if (m_pCreatureCache != pCreature)
        return;

    m_oidCreatureCache = OBJECT_INVALID;
    m_pCreatureCache = 0;
}

void CClientExoAppInternal::StallServerCode()
{
    CLoadGameErrorInfo* p = g_pAppManager->m_pLoadGameErrorInfo;
    if (p->m_bStallFirstFrame == 1)
        p->m_bStallFirstFrame = 0;
    else if (p->m_bStallFinished == 1) {
        p->m_bStallFinished = 0;
        p->m_flag = 0;
        p->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_NONE;
        if (m_nLoadGameError)
            ShutDownToMainMenu();
        m_nLoadGameError = 0;
    }
}

void CClientExoAppInternal::SetGameSpeed(float fSpeed)
{
    if (g_pAppManager) {
        if (g_pAppManager->m_pClientExoApp) {
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->SetSnapshotScale(fSpeed);
            g_pAppManager->m_pClientExoApp->GetPauseTimer()->SetSnapshotScale(fSpeed);
            g_pAppManager->m_pClientExoApp->GetAnimationTimer()->SetSnapshotScale(fSpeed);
        }
        if (g_pAppManager->m_pServerExoApp) {
            g_pAppManager->m_pServerExoApp->GetWorldTimer()->SetSnapshotScale(fSpeed);
            static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetPauseTimer())->SetSnapshotScale(fSpeed);
        }
    }
}

void CClientExoAppInternal::UpdateDeathSequence()
{
    int nNow = static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    int nReason = m_nGameOverReason;
    int nOld = m_deathSeqLastMs;
    m_deathSeqLastMs = nNow;
    if (nReason != 0) {
        m_fDeathSeqAccum = static_cast<float>(static_cast<unsigned int>(nNow - nOld)) / 1000.0f + m_fDeathSeqAccum;
#if KOTOR1
        float fFade;
        if (m_bGameOver) {
            m_bGameOver = 0;
            fFade = logf(m_fDeathSeqAccum * 0.25f) / -0.8f;
            m_fDeathSeqFade = fFade;
        } else {
            fFade = m_fDeathSeqFade;
        }
        if (m_fDeathSeqAccum <= 4.0f)
            SetGameSpeed(0.2f - logf(m_fDeathSeqAccum * 0.25f) / fFade);
        else
            SetGameSpeed(0.2f);
        if (m_pInGameGui->IsGlobalFading() && m_bEndGamePending == 0)
            return;
        if (!(m_fEndGameDelay <= 0.0f))
            return;
        SetGameSpeed(1.0f);
        m_nGameOverReason = 0;
        m_bGameOver = 1;
        m_pInGameGui->StopGlobalFade();
        m_fEndGameDelay = 0.0f;
        m_bEndGamePending = 1;
        m_bDisplayEndGameGui = 1;
        CSWGuiMessageBox* pBox = m_pInGameGui->m_pSoloModeQueryBox;
        g_pGuiMan->PopModalPanel();
        g_pGuiMan->RemovePanel(pBox);
#else
        if (m_bGameOver) {
            m_bGameOver = 0;
            m_fDeathSeqFade = logf(m_fDeathSeqAccum * 0.25f) / -0.8f;
        }
#endif
    }
}

void CClientExoAppInternal::FadeAndStopStreamingSoundSource(
    CExoStreamingSoundSource* pSource, unsigned int nFadeMs)
{
    unsigned long long t0ms = g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000;
    pSource->FadeAndStop(nFadeMs);
    if (nFadeMs != 0) {
        do {
            g_pExoSound->Render();
            unsigned long long tNowMs = g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000;
            if (static_cast<unsigned int>(tNowMs - t0ms) >= nFadeMs)
                break;
        } while (1);
    }
}

void CClientExoAppInternal::DoQuickSave()
{
    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
    CExoString sDir("QUICKSAVE");
    CExoString sName("QUICKSAVE");
    pClient->SendSaveGameRequest(0, sDir, sName);
}

CExoString CClientExoAppInternal::GetDisplayCharacterForKey(int key)
{
    if (CExoInput::KEYBOARD_START > key || key >= CExoInput::JOYSTICK_START)
        return g_pTlkTable->GetSimpleString(STRREF_NONE_SET);
    return g_pExoInput->GetDisplayCharacterForKey(key);
}

void CClientExoAppInternal::AddQueuedAnim(const CExoString& sName, unsigned short nFlags, float f, int n)
{
    CQueuedAnim* p = new CQueuedAnim;
    p->m_sName = sName;
    p->m_fParam = f;
    p->m_nParam = n;
    p->m_nAnimation = nFlags;
    m_queuedAnims.AddTail(p);
}

void CClientExoAppInternal::BusyWaitMuteSound()
{
    unsigned int nStart = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_MUTE, 0);
    do {
        g_pExoSound->Render();
    } while (static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000) - nStart < 500);
}

void CClientExoAppInternal::BusyWaitMenuMusic()
{
    unsigned int nStart = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    if (m_pMenuMusic)
        m_pMenuMusic->FadeAndStop(2500);
    do {
        g_pExoSound->Render();
    } while (static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000) - nStart < 750);
    if (m_pMenuMusic)
        m_pMenuMusic->Stop(0);
}

void CClientExoAppInternal::EnableMouseAtEndOfLoadModuleMovie()
{
    if (m_pMoviePlayer)
        if (m_pMoviePlayer->IsMoviePlaying())
            return;
    if (m_bShowMouseAtMovieEnd) {
        m_bShowMouseAtMovieEnd = 0;
        ShowMouse(MOUSEHIDE_MOVIE);
    }
}

void CClientExoAppInternal::SetEvilEndingHackInProgress(int bInProgress)
{
    m_bEvilEndingHackInProgress = bInProgress;
    if (bInProgress == 0 && m_pEvilEndingMusic) {
        m_pEvilEndingMusic->Stop(0);
        delete m_pEvilEndingMusic;
        m_pEvilEndingMusic = 0;
    }
}

void CClientExoAppInternal::CleanUpModuleServices()
{
    if (m_pWorldJournal)
        m_pWorldJournal->DeleteAllEntries();
    if (m_pQuestJournal)
        m_pQuestJournal->ClearAll();
    if (m_pInGameGui)
        m_pInGameGui->ShutDown();
    if (m_pPlayerControl)
        m_pPlayerControl->Initialize();
}

CWorldTimer* CClientExoAppInternal::GetActiveTimer(unsigned long long oid)
{
    if (oid == OBJECT_INVALID)
        return m_pWorldTimer;
    int state = (m_pauseFlags & 2) ? 2 : (m_pauseFlags & 1);
    if (state == 2) {
        if (IsOnExclusionList(oid))
            return m_pPauseTimer;
        return m_pWorldTimer;
    }
    if (state) {
        if (IsOnExclusionList(oid))
            return m_pExclusionTimer;
        return m_pWorldTimer;
    }
    return m_pWorldTimer;
}

int CClientExoAppInternal::IsOnExclusionList(unsigned long long oid)
{
    if (m_pauseExclusionList == 0)
        return 1;
    unsigned long long id = oid;
    CExoLinkedListNode* pNode = m_pauseExclusionList->m_pInternal->m_pHead;
    if (pNode == 0)
        return 0;
    do {
        if (reinterpret_cast<unsigned long long>(m_pauseExclusionList->m_pInternal->GetAtPos(pNode)) == id)
            return 1;
        m_pauseExclusionList->m_pInternal->GetNext(pNode);
        if (pNode == 0)
            return 0;
    } while (1);
}

void CClientExoAppInternal::AddToPauseExclusionList(unsigned long long oid)
{
    CGameObject* pObj;
    m_pGameObjectArray->GetGameObject(oid, &pObj);
    if (pObj) {
        Gob* pModel = pObj->AsSWCObject()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
        pModel->EnableForceAnimations();
        m_pauseExclusionList->m_pInternal->AddTail(reinterpret_cast<void*>(oid));
        unsigned int nA, nB;
        GetActiveTimer(pObj->m_objectId)->GetWorldTime(&nA, &nB);
        pObj->ResetUpdateTimes(nA, nB);
    }
}

void CClientExoAppInternal::ClearPauseExclusionList()
{
    while (m_pauseExclusionList->m_pInternal->m_pHead != 0 &&
           m_pauseExclusionList->m_pInternal->m_pHead->m_pData != 0) {
        unsigned long long oid = reinterpret_cast<unsigned long long>(m_pauseExclusionList->m_pInternal->RemoveHead());
        CGameObject* pObj;
        m_pGameObjectArray->GetGameObject(oid, &pObj);
        if (pObj) {
            unsigned int nA, nB;
            GetActiveTimer(pObj->m_objectId)->GetWorldTime(&nA, &nB);
            pObj->ResetUpdateTimes(nA, nB);
            Gob* pModel = pObj->AsSWCObject()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            pModel->DisableForceAnimations();
        }
    }
}

void CClientExoAppInternal::ReprocessExclusionList()
{
    CExoLinkedList<unsigned long long>* pList = m_pauseExclusionList;
    CExoLinkedListNode* pNode = pList->GetHeadPos();
    while (pNode) {
        unsigned long long oid = reinterpret_cast<unsigned long long>(pList->GetAtPos(pNode));
        CGameObject* pObj;
        unsigned int nA, nB;
        GetActiveTimer(oid)->GetWorldTime(&nA, &nB);
        m_pGameObjectArray->GetGameObject(oid, &pObj);
        pObj->ResetUpdateTimes(nA, nB);
        pList->GetNext(pNode);
    }
}

__attribute__((noinline))
void CClientExoAppInternal::AddObjectToHitcheckIgnoreList(unsigned long long oid)
{
    CExoLinkedListNode* node = m_hitCheckIgnoreList.m_pInternal->m_pHead;
    unsigned long long* pData = 0;
    if (m_hitCheckIgnoreList.m_pInternal->m_pHead != 0)
        pData = static_cast<unsigned long long*>(m_hitCheckIgnoreList.m_pInternal->m_pHead->m_pData);
    while (node != 0) {
        if (pData != 0 && *pData == oid)
            return;
        pData = static_cast<unsigned long long*>(m_hitCheckIgnoreList.m_pInternal->GetNext(node));
    }
    unsigned long long* pNew = new unsigned long long(oid);
    m_hitCheckIgnoreList.m_pInternal->AddTail(pNew);
}

void CClientExoAppInternal::RemoveObjectFromHitcheckIgnoreList(unsigned long long oid)
{
    CExoLinkedListInternal* internal = m_hitCheckIgnoreList.m_pInternal;
    CExoLinkedListNode* node = internal->m_pHead;
    unsigned long long* pData = 0;
    if (internal->m_pHead != 0)
        pData = static_cast<unsigned long long*>(internal->m_pHead->m_pData);
    while (node != 0) {
        if (pData != 0 && *pData == oid) {
            m_hitCheckIgnoreList.m_pInternal->Remove(node);
            operator delete(pData);
            break;
        }
        pData = static_cast<unsigned long long*>(m_hitCheckIgnoreList.m_pInternal->GetNext(node));
    }
}

void CClientExoAppInternal::SetToolTipInstantMode(int bInstant)
{
    if (m_pInGameGui == 0)
        return;
    CSWGuiManager* pGui = g_pGuiMan;
    if (bInstant) {
        pGui->m_fToolTipTimeDelay = 0.0f;
        if (m_pInGameGui->m_pMainInterface)
            m_pInGameGui->m_pMainInterface->DisplayToolTipMessage();
    } else {
        CClientOptions* pOpts = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pGui->m_fToolTipTimeDelay = pOpts->m_fToolTipTimeDelay;
        if (m_pInGameGui->m_pMainInterface)
            m_pInGameGui->m_pMainInterface->RemoveToolTipMessage();
    }
}

void CClientExoAppInternal::DisplayStartScreen()
{
    if (m_pStartScreen == 0)
        m_pStartScreen = new CSWGuiStartScreen(m_pSWGuiManager);
    m_pSWGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pStartScreen), 2, 1);
    SetInputClass(INPUTCLASS_GUI, 1);
#if KOTOR1  // the cult theme on the start screen
    StartMenuMusic(1, 127, 2);
#else
    StartMenuMusic(2, 127, 2);
#endif
}

#ifdef K2_HOST_BUILD
extern "C" void k2host_gl_trace_main_menu(void);   // platforms/shim/shim_window.cpp
#endif

void CClientExoAppInternal::DisplayMainMenu(int n)
{
    if (m_pMainMenuPanel &&
        m_pSWGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pMainMenuPanel)))
        return;

    if (m_pStartScreen)
    {
        if (m_pSWGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pStartScreen)))
        {
            static_cast<CSWGuiPanel*>(m_pStartScreen)->SetToBeDeleted(0);
        }
        else
        {
            delete static_cast<CSWGuiPanel*>(m_pStartScreen);
        }
        m_pStartScreen = 0;
    }

#if !KOTOR1  // no teardown of a stale main-menu panel
    if (m_pMainMenuPanel)
    {
        CSWGuiPanel* pMenu = static_cast<CSWGuiPanel*>(m_pMainMenuPanel);
        {
            unsigned short nFlags = static_cast<unsigned short>(pMenu->m_stateFlags & ~PANEL_REMOVE_DEFERRED);
            if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                nFlags = static_cast<unsigned short>(
                    (pMenu->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE);
            pMenu->m_stateFlags = nFlags;
        }

        CSWGuiPanel* pChain = *reinterpret_cast<CSWGuiPanel* volatile*>(&m_pMainMenuPanel);
        pChain->SetToBeDeleted(0);
        m_pMainMenuPanel = 0;
    }
#endif

    m_pMainMenuPanel = new CSWGuiMainMenu(m_pSWGuiManager);
    SetInputClass(INPUTCLASS_GUI, 1);
    m_pSWGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pMainMenuPanel), 2, 1);
#ifdef K2_HOST_BUILD
    k2host_gl_trace_main_menu();
#endif
#if KOTOR1  // a literal 1, n is not forwarded
    K2_UNUSED(n);
    StartMenuMusic(1, 127, 2);
#else
    StartMenuMusic(n, 127, 2);
#endif
    if (m_bMainMenuReset == 1)
        m_bMainMenuReset = 0;
    m_pInGameGui->SetGlobalUnpauseableState(0);
}

void CClientExoAppInternal::SetPauseState(unsigned char nPauseMask, int bPause,
        CExoLinkedList<unsigned long long>* pPauseList)
{
    if ((m_pauseFlags & nPauseMask) == bPause)
        return;

    if (bPause == 1) {
        m_pauseFlags |= nPauseMask;
        m_pWorldTimer->PauseWorldTimer();
        m_pWorldTimer->SetSnapshotTime();
        m_pWorldTimer->SetSnapshotTime();
        if (m_pauseFlags & 2) {
            m_pExclusionTimer->PauseWorldTimer();
            m_pExclusionTimer->SetSnapshotTime();
            m_pExclusionTimer->SetSnapshotTime();
        }
        {
            if (g_pAppManager->m_pClientExoApp->GetModule()) {
                CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
                if (pModule->m_pArea && pModule->m_pArea->m_pScene)
                    pModule->m_pArea->m_pScene->DisableAnimations();
            }
        }
    } else {
        m_pauseFlags &= ~nPauseMask;
        if (nPauseMask == 2) {
            m_pPauseTimer->PauseWorldTimer();
            m_pAnimationTimer->PauseWorldTimer();
            m_pPauseTimer->ResetTimer(m_pExclusionTimer);
            m_pAnimationTimer->ResetTimer(m_pExclusionTimer);
            m_pPauseTimer->SetSnapshotTime();
            m_pAnimationTimer->SetSnapshotTime();
            m_pExclusionTimer->SetSnapshotTime();
            m_pPauseTimer->ResetLastSnapshotTime();
            m_pAnimationTimer->ResetLastSnapshotTime();
            m_pExclusionTimer->ResetLastSnapshotTime();
            if ((m_pauseFlags & 3) != 1) {
                m_pWorldTimer->SetSnapshotTime();
                m_pWorldTimer->ResetLastSnapshotTime();
                m_pWorldTimer->UnpauseWorldTimer();
                if (m_pModule && m_pModule->m_pArea && m_pModule->m_pArea->m_pScene)
                    m_pModule->m_pArea->m_pScene->EnableAnimations();
            }
            m_pExclusionTimer->UnpauseWorldTimer();
            m_pPauseTimer->UnpauseWorldTimer();
            m_pAnimationTimer->UnpauseWorldTimer();
        } else if (nPauseMask == 1) {
            m_pExclusionTimer->PauseWorldTimer();
            m_pPauseTimer->PauseWorldTimer();
            m_pAnimationTimer->PauseWorldTimer();
            m_pExclusionTimer->ResetTimer(m_pWorldTimer);
            m_pPauseTimer->ResetTimer(m_pWorldTimer);
            m_pAnimationTimer->PauseWorldTimer();
            m_pWorldTimer->SetSnapshotTime();
            m_pExclusionTimer->SetSnapshotTime();
            m_pPauseTimer->SetSnapshotTime();
            m_pAnimationTimer->SetSnapshotTime();
            m_pPauseTimer->ResetLastSnapshotTime();
            m_pAnimationTimer->ResetLastSnapshotTime();
            m_pExclusionTimer->ResetLastSnapshotTime();
            m_pWorldTimer->ResetLastSnapshotTime();
            m_pWorldTimer->UnpauseWorldTimer();
            m_pExclusionTimer->UnpauseWorldTimer();
            m_pPauseTimer->UnpauseWorldTimer();
            m_pAnimationTimer->UnpauseWorldTimer();
            if (m_pModule && m_pModule->m_pArea && m_pModule->m_pArea->m_pScene)
                m_pModule->m_pArea->m_pScene->EnableAnimations();
        }
    }

    ClearPauseExclusionList();

    if (pPauseList) {
        CExoLinkedListNode* pNode = pPauseList->m_pInternal->m_pHead;
        while (pNode != 0) {
            AddToPauseExclusionList(reinterpret_cast<unsigned long long>(pPauseList->m_pInternal->GetAtPos(pNode)));
            pPauseList->m_pInternal->GetNext(pNode);
        }
        delete pPauseList;
    }
}

void CClientExoAppInternal::SetPausedByCombat(int bPause, unsigned char nMode, int bForce)
{
    int nGuiState = m_pInGameGui->m_nGlobalDialogState;
#if KOTOR1  // no mode-13 override, and the sound/rumble switch runs for every mode
    if (bPause != 0 || nGuiState != 1) {
        if (bForce != 1 && nGuiState != 0)
            return;
    }
    int pauseState = 0;
    if (g_pAppManager->m_pServerExoApp)
        pauseState = g_pAppManager->m_pServerExoApp->GetPauseState(2);
    if (pauseState != bPause && !m_bTogglePauseAtEndOfFrame) {
        m_bTogglePauseAtEndOfFrame = 1;
        m_combatPauseMode = nMode;
        m_combatPauseRequest = bPause;
        if (bPause == 1) {
            g_pExoInput->PauseRumble();
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE);
            m_bSoundPausedByCombat = 1;
        } else {
            g_pExoInput->UnpauseRumble();
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
            m_bSoundPausedByCombat = 0;
        }
    }
#else
    if ((nGuiState == 1 && bPause == 0) || nGuiState == 0 || bForce == 1 || nMode == PAUSE_MODE_HELP) {
        int pauseState = 0;
        if (g_pAppManager->m_pServerExoApp)
            pauseState = g_pAppManager->m_pServerExoApp->GetPauseState(2);
        if (pauseState != bPause && !m_bTogglePauseAtEndOfFrame) {
            m_bTogglePauseAtEndOfFrame = 1;
            m_combatPauseMode = nMode;
            m_combatPauseRequest = bPause;
            if (nMode != 3) {
                if (bPause == 1) {
                    g_pExoInput->PauseRumble();
                    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE, 0);
                    m_bSoundPausedByCombat = 1;
                } else {
                    g_pExoInput->UnpauseRumble();
                    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
                    m_bSoundPausedByCombat = 0;
                }
            }
        }
    }
#endif
}

void CClientExoAppInternal::SetHitcheckIgnoreList(List<CAurObject*>* pList)
{
    {
        CGameObject* pGameObject;
        CExoLinkedListNode* pNode = m_hitCheckIgnoreList.m_pInternal->m_pHead;
        void* pData = 0;
        if (m_hitCheckIgnoreList.m_pInternal->m_pHead)
            pData = m_hitCheckIgnoreList.m_pInternal->m_pHead->m_pData;
        while (pNode) {
            if (pData) {
                unsigned long long oid = *static_cast<unsigned long long*>(pData);
#if KOTOR1  // the member lookup (SUCCESS-gated)
                pGameObject = GetGameObject(oid);
#else
                pGameObject = 0;
                m_pGameObjectArray->GetGameObject(oid, &pGameObject);
#endif
                if (pGameObject) {
                    CSWCObject* pObj = pGameObject->AsSWCObject();
                    if (pObj) {
                        CSWCCreature* pCreature = pObj->AsSWCCreature();
                        pObj->AsSWCItem();
                        if (pObj->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1))
                            pList->Add(static_cast<CAurObject*>(pObj->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)));
                        if (pCreature) {
                            CSWCCreatureAppearance* pAppearance = pCreature->m_pCreatureAppearance;
                            if (pAppearance) {
                                CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                                    pAppearance->m_oidHeadItem);
                                if (pItem) {
                                    if (pItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1))
                                        pList->Add(static_cast<CAurObject*>(pItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)));
                                }
                                if (pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1))
                                    pList->Add(static_cast<CAurObject*>(pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1)));
#if !KOTOR1  // the head model is added once
                                if (pAppearance->m_oidHeadItem == CGameObjectArray::INVALIDOBJID) {
                                    if (pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1))
                                        pList->Add(static_cast<CAurObject*>(pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1)));
                                } else {
                                    CSWCItem* pItem2 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                                        pAppearance->m_oidHeadItem);
                                    if (pItem2) {
                                        if (pItem2->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1))
                                            pList->Add(static_cast<CAurObject*>(pItem2->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)));
                                    }
                                }
#endif
                            }
                            CSWCItem* pWeapon = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                                pCreature->GetEquippedItemID(EQUIP_SLOT_RIGHT_WEAPON));
                            if (pWeapon) {
                                int nBaseItem = pWeapon->m_baseItem;
                                g_pRules->GetBaseItem(nBaseItem);
                                if (pWeapon->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1))
                                    pList->Add(static_cast<CAurObject*>(pWeapon->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)));
                            }
                            CSWCItem* pWeapon2 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                                pCreature->GetEquippedItemID(EQUIP_SLOT_LEFT_WEAPON));
                            if (pWeapon2) {
                                if (pWeapon2->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1))
                                    pList->Add(static_cast<CAurObject*>(pWeapon2->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)));
                            }
                        }
                    }
                }
            }
            pData = m_hitCheckIgnoreList.GetNext(pNode);
        }
    }

    CSWCModule* pModule = m_pModule;
    if (pModule) {
        CSWCArea* pArea = pModule->m_pArea;
        if (pArea) {
            CExoLinkedListNode* pNode2 = pArea->m_hitCheckIgnoreList.m_pInternal->m_pHead;
            void* pData2 = 0;
            if (pArea->m_hitCheckIgnoreList.m_pInternal->m_pHead)
                pData2 = pArea->m_hitCheckIgnoreList.m_pInternal->m_pHead->m_pData;
            while (pNode2) {
                if (pData2)
                    pList->Add(static_cast<CAurObject*>(pData2));
                pData2 = pArea->m_hitCheckIgnoreList.GetNext(pNode2);
            }
        }
    }
}

void CClientExoAppInternal::AddCreatureModelDirectlyToIgnoreList(unsigned long long creatureId, List<CAurObject*>* pList)
{
    CGameObject* pGameObject = 0;
    if (m_pGameObjectArray->GetGameObject(creatureId, &pGameObject) != CGameObjectArray::SUCCESS)
        return;
    if (!pGameObject)
        return;
    if (pGameObject->m_objectType != CGameObject::OBJECT_TYPE_CREATURE)
        return;

    CSWCCreature* pCreature = pGameObject->AsSWCCreature();
    if (!pCreature)
        return;

    if (Gob* pModel = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1))
        pList->Add(static_cast<CAurObject*>(pModel));
    if (Gob* pModel = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1))
        pList->Add(static_cast<CAurObject*>(pModel));

    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(pCreature->GetEquippedItemID(EQUIP_SLOT_HEAD));
    if (pItem) {
        if (Gob* pModel = pItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1))
            pList->Add(static_cast<CAurObject*>(pModel));
    }

    CSWCItem* pItem2 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(pCreature->GetEquippedItemID(EQUIP_SLOT_RIGHT_WEAPON));
    if (pItem2) {
        for (unsigned char i = CSWCAnimBase::ANIM_MODEL_BODY; i > CSWCAnimBase::ANIM_MODEL_BODY - ITEM_MODEL_PARTS; i--) {
            if (Gob* pModel = pItem2->GetModel(i, 1))
                pList->Add(static_cast<CAurObject*>(pModel));
        }
    }

    CSWCItem* pItem3 = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(pCreature->GetEquippedItemID(EQUIP_SLOT_LEFT_WEAPON));
    if (pItem3) {
        for (unsigned char i = CSWCAnimBase::ANIM_MODEL_BODY; i > CSWCAnimBase::ANIM_MODEL_BODY - ITEM_MODEL_PARTS; i--) {
            if (Gob* pModel = pItem3->GetModel(i, 1))
                pList->Add(static_cast<CAurObject*>(pModel));
        }
    }
}

void CClientExoAppInternal::AddItemModelDirectlyToIgnoreList(unsigned long long itemId, List<CAurObject*>* pList)
{
    CGameObject* pGameObject = 0;
    if (m_pGameObjectArray->GetGameObject(itemId, &pGameObject) != CGameObjectArray::SUCCESS)
        return;
    if (!pGameObject)
        return;
    if (pGameObject->m_objectType != CGameObject::OBJECT_TYPE_ITEM)
        return;

    CSWCItem* pItem = pGameObject->AsSWCItem();
    if (!pItem)
        return;

    for (unsigned char i = CSWCAnimBase::ANIM_MODEL_BODY; i > CSWCAnimBase::ANIM_MODEL_BODY - ITEM_MODEL_PARTS; i--) {
        if (Gob* pModel = pItem->GetModel(i, 1))
            pList->Add(static_cast<CAurObject*>(pModel));
    }
}

void CClientExoAppInternal::AddFloatyText(unsigned long long objectId, unsigned char bType, int nValue)
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions != 0 && !pOptions->m_bFeedback4)
        return;

    Vector vInfo;
    CExoString sString;
    float fScale;
    switch (bType)
    {
        case CClientExoApp::FLOATY_DAMAGE:
            sString = CExoString(nValue);
            vInfo = CGuiInGame::COLOR_RED;
            fScale = 1.5f;
            break;
        case CClientExoApp::FLOATY_HEALED:
            sString = CExoString(nValue);
            vInfo = CGuiInGame::COLOR_GREEN;
            fScale = 1.5f;
            break;
        case CClientExoApp::FLOATY_MISS:
            sString = GetGUIString(STRREF_MISS);
            vInfo = CGuiInGame::COLOR_WHITE;
            fScale = 1.5f;
            break;
        case CClientExoApp::FLOATY_XP:
            sString.Format(const_cast<char*>("%s %d"), GetGUIString(STRREF_XP).CStr(), nValue);
            vInfo = CGuiInGame::COLOR_PURPLE;
            fScale = 3.0f;
            break;
        case CClientExoApp::FLOATY_LEVEL:
            sString.Format(const_cast<char*>("%s %d"), GetGUIString(STRREF_LEVEL).CStr(), nValue);
            vInfo = CGuiInGame::COLOR_ORANGE;
            fScale = 3.0f;
            break;
        case CClientExoApp::FLOATY_SNEAK_ATTACK:
            sString = GetGUIString(STRREF_SNEAK_ATTACK);
            vInfo = CGuiInGame::COLOR_WHITE;
            fScale = 1.5f;
            break;
        case CClientExoApp::FLOATY_SPOTTED:
            sString = GetGUIString(STRREF_SPOTTED);
            vInfo = CGuiInGame::COLOR_WHITE;
            fScale = 1.5f;
            break;
        default:
            return;  // no text for other types, skip the AddFloatyText call
    }
    m_pInGameGui->AddFloatyText(objectId, sString, vInfo, fScale);
}

// The id -> resident game object lookup (zero when the id is not resident).
#if KOTOR1
CGameObject* CClientExoAppInternal::GetGameObject(unsigned long long id)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(id, &pObject) != CGameObjectArray::SUCCESS)
        return 0;
    return pObject;
}
#endif

int CClientExoAppInternal::GetObjectName(unsigned long long objectId, CExoString& outName)
{
    CExoLinkedListNode* pNode;
    int nStrRef;
    CGameObject* pGameObject;
    CSWCPlayer* pPlayer;
    STR_RES sReply;
    pGameObject = GetGameObject(objectId);

    if (!pGameObject) {
        pNode = m_playerList.m_pInternal->m_pHead;
        while (pNode) {
            pPlayer = static_cast<CSWCPlayer*>(m_playerList.m_pInternal->GetAtPos(pNode));
            if (pPlayer) {
                if (pPlayer->m_characterObjectId == objectId) {
                    outName = pPlayer->m_sCharacterName;
                    return 1;
                }
            }
            m_playerList.m_pInternal->GetNext(pNode);
        }
        outName = "";
        return 0;
    }

    if (pGameObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
#if KOTOR1  // the first + last name built by the client stats
        outName = pGameObject->AsSWCCreature()->m_pStats->GetFullName();
#else
        outName = pGameObject->AsSWCCreature()->m_pStats->m_name;
#endif
        return 1;
    }

    if (pGameObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
        if (pGameObject->AsSWCPlaceable()->GetName() == "") {
            g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(
                pGameObject->AsSWCPlaceable()->m_nAppearanceType, CTwoDimArrays::COL2DA_PLACEABLES_STRREF,
                &nStrRef);
            g_pTlkTable->Fetch(nStrRef, sReply, CTlkTable::GENDER_MALE);
            outName = sReply.text;
            return 1;
        }
        outName = pGameObject->AsSWCPlaceable()->GetName();
        return 1;
    }

    if (pGameObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
        outName = pGameObject->AsSWCDoor()->GetName();
        return 1;
    }

    if (pGameObject->m_objectType == CGameObject::OBJECT_TYPE_ITEM) {
        outName = pGameObject->AsSWCItem()->GetName(1);
        return 1;
    }

    if (pGameObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
        outName = pGameObject->AsSWCTrigger()->GetName();
        if (outName == "") {
            STR_RES sStrRes;
            g_pTlkTable->Fetch(STRREF_MINE, sStrRes, CTlkTable::GENDER_MALE);
            outName = sStrRes.text;
            return 1;
        }
        return 1;
    }

    outName = "";
    return 0;
}

void CClientExoAppInternal::BarkString(unsigned long long objectId, const CExoString& sText,
                                       const CResRef& sVoice, int nDuration)
{
    CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->GetGameObject(objectId);
    if (pGameObject) {
        pGameObject->AsSWCObject()->BarkString(objectId, sText, sVoice, nDuration);
    } else {
        m_pInGameGui->ShowBarkBubble(OBJECT_INVALID, sText, sVoice, nDuration);
    }
}

#if !KOTOR1  // absent in K1
void CClientExoAppInternal::DoScreenShot()
{
    WIN32_FIND_DATAA findData;
    for (int i = 0; i < 99999; i++) {
        sprintf_s(m_sScreenShotFile, 16, "K2_%05i.tga", i);
        void* hFind = FindFirstFileA(m_sScreenShotFile, &findData);
        FindClose(hFind);
        if (hFind == reinterpret_cast<void*>(-1))
            break;
    }
    m_bScreenShotPending = 1;
}
#endif

CExoString CClientExoAppInternal::GetGUIString(uint64_t nStrRef)
{
    int nLow = 0;
    int nHigh = m_stringStructs.m_count - 1;
    int nPos = 0;
    CExoString strReturn;

    while (nLow < nHigh) {
        nPos = nLow + (nHigh - nLow) / 2;
        if (m_stringStructs.m_pArray[nPos].m_strRef == nStrRef) {
            strReturn = m_stringStructs.m_pArray[nPos].m_sText;
            goto Found;
        }
        nLow = m_stringStructs.m_pArray[nPos].m_strRef < nStrRef ? nLow : nPos + 1;
        nHigh = m_stringStructs.m_pArray[nPos].m_strRef < nStrRef ? nPos : nHigh;
    }

    {
        CGuiStringStruct newEntry;
        {
            CExoString strSimple = g_pTlkTable->GetSimpleString(nStrRef);
            strReturn = strSimple;
        }
        newEntry.m_strRef = nStrRef;
        newEntry.m_sText = strReturn;
        if (m_stringStructs.m_count > 0 &&
            m_stringStructs.m_pArray[nPos].m_strRef < nStrRef)
            nPos++;
        m_stringStructs.Insert(newEntry, nPos);
    }

Found:
    CTlkTable* pTlk = g_pTlkTable;
    pTlk->ParseStr(strReturn);
    return strReturn;
}


#ifdef K2_UI_PC

// PC UI screen-edge camera turn. A pointer within ScreenFramePercentage (0.001) of an edge, and at
// least 2 px in, turns (left/right) or tilts (top/bottom) the camera. A window cannot be pushed
// against, so the pointer leaving the focused window left or right counts as that edge too.
// *pX / *pY: -1 left or bottom, 1 right or top, 0 none.
static void PcScreenEdge(CSWGuiManager* pGui, int* pX, int* pY)
{
    *pX = 0;
    *pY = 0;
    if (pGui == 0 || pGui->m_modalPanels.m_count != 0 || SDL_GetRelativeMouseMode())
        return;
    // A hidden window (unattended runs) never has focus; its pointer is the scripted one.
    static const bool s_bHidden = getenv("K2_HIDDEN") != 0;
    SDL_Window* pFocus = SDL_GetKeyboardFocus();
    if (pFocus == 0 && !s_bHidden)
        return;
    const float kScreenFramePercentage = 0.001f;
    int nW = pGui->m_nScreenWidth;
    int nH = pGui->m_nScreenHeight;
    int nMarginX = static_cast<int>(static_cast<float>(nW) * kScreenFramePercentage);
    int nMarginY = static_cast<int>(static_cast<float>(nH) * kScreenFramePercentage);
    if (nMarginX < 2) nMarginX = 2;
    if (nMarginY < 2) nMarginY = 2;
    int nMouseX = pGui->m_currentMousePos.x;
    int nMouseY = pGui->m_currentMousePos.y;   // bottom-up
    if (nMouseX <= nMarginX) *pX = -1;
    else if (nMouseX >= nW - nMarginX) *pX = 1;
    if (nMouseY <= nMarginY) *pY = -1;
    else if (nMouseY >= nH - nMarginY) *pY = 1;
    if (pFocus != 0) {
        int nGX, nGY, nWX, nWY, nWW, nWH;
        SDL_GetGlobalMouseState(&nGX, &nGY);
        SDL_GetWindowPosition(pFocus, &nWX, &nWY);
        SDL_GetWindowSize(pFocus, &nWW, &nWH);
        if (nGX < nWX) *pX = -1;
        else if (nGX >= nWX + nWW) *pX = 1;
        else if (nGY < nWY || nGY >= nWY + nWH) *pX = *pY = 0;   // left through the top / bottom
    }
}

// Applies the edge turn; true when it took the place of the mouse turn.
static bool PcApplyScreenEdge(CSWCModule* pModule, CSWGuiManager* pGui, float fFrameTime)
{
    int nEdgeX, nEdgeY;
    PcScreenEdge(pGui, &nEdgeX, &nEdgeY);
    if (IsJoystickConnected())
        nEdgeY = 0;
    if (nEdgeX == 0 && nEdgeY == 0)
        return false;
    if (nEdgeX != 0)
        pModule->AcclTurnCamera(static_cast<float>((CameraHorzAxisFlip ? 1 : -1) * (nEdgeX < 0 ? 1 : -1)),
                                fFrameTime);
#if KOTOR1
    const int nVertFlip = CameraVertAxisFlip ? 1 : -1;
#else
    const int nVertFlip = -1;   // K2 Android has no CameraVertAxisFlip; the macOS default (0)
#endif
    if (nEdgeY != 0)
        pModule->AcclTiltCamera(static_cast<float>(nVertFlip * (nEdgeY < 0 ? 1 : -1)));
    return true;
}
#endif

#ifdef K2_FIX_ENGINE_BUGS
// The mouse delta is one frame's travel. The camera behaviors multiply a tilt by the frame time, as
// they do a stick's rate, and free-look's turn by the time passed in, so the delta is normalized to
// a 60 fps frame and the same travel tilts and turns as far at any frame rate.
static const float kMouseLookFrame = 1.0f / 60.0f;

static float MouseLookTiltScale(long long nFrameUs)
{
    if (nFrameUs <= 0)
        return 1.0f;
    return kMouseLookFrame / (static_cast<float>(nFrameUs) / 1000000.0f);
}

static float MouseLookTurnTime(float fFrameTime)
{
    return fFrameTime > 0.0f ? kMouseLookFrame : fFrameTime;
}
#endif

void CClientExoAppInternal::UpdateCamera(float fFrameTime)

{
#if KOTOR1
    CSWCModule* pModule = m_pModule;
    if (!pModule)
        return;

    CWorldTimer* pPauseTimer = g_pAppManager->m_pClientExoApp->GetPauseTimer();
    long long frameTimeUs = pPauseTimer->GetSnapshotTimeDifference();
    unsigned char bKeepHistory = g_bKeepHistory;
    g_bKeepHistory = 0;
    CAurCamera* pCamera = m_pModule->m_pCamera;

    float fMouseSen = g_pAppManager->m_pClientExoApp->GetClientOptions()->GetMouseSenSetting();
    float fMouseDX = 0.0f;
    float fMouseDY = 0.0f;
    g_pExoInput->GetMouseDelta(&fMouseDX, &fMouseDY);
#ifndef K2_UI_TOUCH
    // pc and xbox UIs: the mouse looks only while the camera-rotate latch is set (the right button
    // or the Lookabout keys; the Mouse Look option inverts it), as on the PC -- the port's left-press
    // touch-drag latch (g_bMouseCameraRotateToggleIos) does not turn it
    int bPcLook = g_bMouseCameraRotateToggle;
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay1)
        bPcLook = (bPcLook == 0);
    if (bPcLook || g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
#endif
    {
        int nFlip = CameraVertAxisFlip ? 1 : -1;
#ifdef K2_FIX_ENGINE_BUGS
        m_pModule->TiltCamera(fMouseSen * fMouseDY * static_cast<float>(nFlip) * MouseLookTiltScale(frameTimeUs));
#else
        m_pModule->TiltCamera(fMouseSen * fMouseDY * static_cast<float>(nFlip));
#endif
    }

    const float fLimitNeg = -1.0f;
    const float fLimitPos = 1.0f;
    float fPoll = g_pExoInput->PollInput(INPUTEVENT_AXIS_CAMERA_ROTATE, INPUTCLASS_GAME);
    fPoll = fPoll < fLimitNeg ? fLimitNeg : fPoll;
    fPoll = fPoll > fLimitPos ? fLimitPos : fPoll;

    g_pAppManager->m_pClientExoApp->GetSWGuiManager();

    int bMouseCamRotate = g_bMouseCameraRotateToggleIos;
    float fAnalogX;
    if (ASLPlat_GetControllerPlugged() == 2)
        fAnalogX = ios_gamepad_analog1X;
    else
        fAnalogX = SDL_IsChromebook() ? mouseCameraMovement : ios_controller_left_right;
    fAnalogX = fAnalogX < fLimitPos ? fAnalogX : fLimitPos;
    fAnalogX = fAnalogX > fLimitNeg ? fAnalogX : fLimitNeg;
    float fRightStickX = (fAnalogX != 0.0f) ? fAnalogX : fPoll;

    CClientOptions* pCamOpts = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (fRightStickX != 0.0f) {
        int nFlip = CameraHorzAxisFlip ? 1 : -1;
        m_pModule->AcclTurnCamera(-(fRightStickX * static_cast<float>(nFlip)), fFrameTime);
    } else {
#ifndef K2_UI_TOUCH
        int nTurn = bPcLook || pCamOpts->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK;
        static_cast<void>(bMouseCamRotate);
#ifdef K2_UI_PC
        if (!nTurn && PcApplyScreenEdge(m_pModule, m_pSWGuiManager, fFrameTime)) {
        } else
#endif
#else
        int nTurn = (pCamOpts->m_bGameplay1 ? (bMouseCamRotate == 0) : bMouseCamRotate)
                    || g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK;
#endif
        if (nTurn) {
            int nFlip = CameraHorzAxisFlip ? 1 : -1;
#ifdef K2_FIX_ENGINE_BUGS
            m_pModule->TurnCamera(fMouseSen * fMouseDX * static_cast<float>(nFlip), MouseLookTurnTime(fFrameTime));
#else
            m_pModule->TurnCamera(fMouseSen * fMouseDX * static_cast<float>(nFlip), fFrameTime);
#endif
        } else {
            m_pModule->TurnCamera(0.0f, fFrameTime);
        }
    }

#ifdef K2_FIX_ENGINE_BUGS
    if (g_pExoInput->GetActive() == 0)
        m_pModule->TurnCamera(0.0f, 0.0f);
#endif

    if (m_fCameraCollision != 0.0f)
        m_pModule->m_bCameraZoomed = 1;

    if (pCamera) {
        float fDist = pCamera->GetDist();
        float fExpand = (fDist * 0.2f) / 15.0f;
        g_HitCheckBoxExpansion.x = fExpand;
        g_HitCheckBoxExpansion.y = fExpand;
        g_HitCheckBoxExpansion.z = fExpand;
    }
    pCamera->GetPosition();

    Vector vScroll(0.0f, 0.0f, 0.0f);
    if (m_nScrollSpeedX)
        vScroll.x = pCamera->MouseDir(40, 0).x * static_cast<float>(m_nScrollSpeedX);
    if (m_nScrollSpeedY)
        vScroll.y = pCamera->MouseDir(0, 40).y * static_cast<float>(m_nScrollSpeedY);
    float fDeltaSeconds = static_cast<float>(frameTimeUs) / 1000000.0f;
    m_pModule->ScrollCamera(vScroll);

    g_bKeepHistory = bKeepHistory;
    if (m_oidCreatureCache != m_playerCreatureId) {
        m_pCreatureCache = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_playerCreatureId);
        m_oidCreatureCache = m_playerCreatureId;
    }

    m_pModule->UpdateCameraBumpAndShake(fDeltaSeconds);
    if (m_combatMode == 0)
        m_pModule->UpdateCameraFollow(fDeltaSeconds);
    m_fCameraCollision = m_pModule->UpdateCameraCollision(fDeltaSeconds, m_fCameraCollision);
    if (fabsf(m_fCameraCollision) > 0.0001f)
        m_pModule->ZoomCamera(m_fCameraCollision);

    if (g_pAppManager->m_pClientExoApp->GetCaptureEvents() == 1 && m_pModule->m_bCameraMoved == 1) {
        float fA = pCamera->GetDist();
        float fB = pCamera->GetPitch();
        float fC = pCamera->GetYaw();
        CExoString sCapture;
        unsigned long long uTime = static_cast<unsigned long long>(
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetSnapshotTime());
        sCapture.Format(const_cast<char*>("%d %c %f %f %f\n"), uTime / 1000, 'V', fA, fB, fC);
        static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile())->Write(sCapture);
        m_pModule->m_bCameraMoved = 0;
    }

    m_fCameraCollision = 0.0f;
    m_nReserved6 = 0;
    m_nReserved7 = 0;
    m_pModule->m_bCameraMoved = 0;
#else
    CSWCModule* pModule = m_pModule;
    if (!pModule)
        return;

    unsigned char bKeepHistory = g_bKeepHistory;
    g_bKeepHistory = 0;

    CWorldTimer* pPauseTimer = g_pAppManager->m_pClientExoApp->GetPauseTimer();
    long long frameTimeUs = pPauseTimer->GetSnapshotTimeDifference();
    CAurCamera* pCamera = m_pModule->m_pCamera;

    float fMouseSen = g_pAppManager->m_pClientExoApp->GetClientOptions()->GetMouseSenSetting();
    float fMouseDX = 0.0f;
    float fMouseDY = 0.0f;
    g_pExoInput->GetMouseDelta(&fMouseDX, &fMouseDY);
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay4)
        fMouseDY = -fMouseDY;

    const float fLimitNeg = -1.0f;
    const float fLimitPos = 1.0f;
    float fRightStickX;
    float fPoll1 = g_pExoInput->PollInput(INPUTEVENT_AXIS_CAMERA_ROTATE, INPUTCLASS_GAME);
    if (!(fPoll1 < fLimitNeg)) {
        float fPoll2 = g_pExoInput->PollInput(INPUTEVENT_AXIS_CAMERA_ROTATE, INPUTCLASS_GAME);
        fRightStickX = fLimitPos;
        if (fPoll2 > fLimitPos)
            goto done_zoom;
    }
    {
        float fPoll3 = g_pExoInput->PollInput(INPUTEVENT_AXIS_CAMERA_ROTATE, INPUTCLASS_GAME);
        fRightStickX = fLimitNeg;
        if (fPoll3 < fLimitNeg)
            goto done_zoom;
        fRightStickX = g_pExoInput->PollInput(INPUTEVENT_AXIS_CAMERA_ROTATE, INPUTCLASS_GAME);
    }
done_zoom:
    ;

    // Kept for parity; both results are discarded.
    g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    IsJoystickConnected();

    int bMouseCamRotate = g_bMouseCameraRotateToggleIos;
    int nController = ASLPlat_GetControllerPlugged();
    float fAnalogX = (nController == 2) ? ios_controller_left_right : ios_gamepad_analog1X;
    fAnalogX = fAnalogX < fLimitPos ? fAnalogX : fLimitPos;
    fAnalogX = fAnalogX > fLimitNeg ? fAnalogX : fLimitNeg;
    if (fAnalogX != 0.0f)
        fRightStickX = fAnalogX;

    float fJSX = 0.0f;
    float fJSY = 0.0f;
    g_pExoInput->GetJoystickRStick(&fJSX, &fJSY);
#ifdef K2_QOL_IMPROVEMENTS
    // "Disable Right Stick Pitch" (the Controller options page): the stick only turns the camera.
    // Free-look's own tilt (ProcessInput) does not come through here.
    if (k2qol_no_pitch())
        fJSY = 0.0f;
#endif
#ifdef K2_UI_XBOX
    // console UI: in input class 2 (a GUI panel over the game: the examine box a datapad opens in,
    // a container) the right stick belongs to the panel, which scrolls with it.
    if (m_inputClass == INPUTCLASS_GUI) {
        fJSY = 0.0f;
        fRightStickX = 0.0f;
    }
#endif
    float fSign;
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay4)
        fSign = 1.0f;
    else
        fSign = -1.0f;
    m_pModule->AcclTiltCamera(fJSY * fSign);

    CClientOptions* pCamOpts = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (fRightStickX != 0.0f) {
        int nFlip = CameraHorzAxisFlip ? 1 : -1;
        m_pModule->AcclTurnCamera(-(fRightStickX * static_cast<float>(nFlip)), fFrameTime);
    } else {
#ifndef K2_UI_TOUCH
        // pc and xbox UIs: the mouse turns and tilts the camera while the camera-rotate latch is set
        // (the right button or the Lookabout keys; the Mouse Look option inverts it); the horizontal
        // axis flip applies here too. The port's left-press touch-drag latch does not turn it.
        int bPcLook = g_bMouseCameraRotateToggle;
        if (pCamOpts->m_bGameplay1)
            bPcLook = (bPcLook == 0);
        int nTurn = bPcLook || g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK;
        static_cast<void>(bMouseCamRotate);
#ifdef K2_UI_PC
        if (!nTurn && PcApplyScreenEdge(m_pModule, m_pSWGuiManager, fFrameTime)) {
        } else
#endif
        if (nTurn) {
            int nFlip = CameraHorzAxisFlip ? 1 : -1;
#ifdef K2_FIX_ENGINE_BUGS
            m_pModule->TurnCamera(fMouseSen * fMouseDX * static_cast<float>(nFlip), MouseLookTurnTime(fFrameTime));
            m_pModule->TiltCamera(fMouseSen * fMouseDY / 10.0f * static_cast<float>(nFlip) * MouseLookTiltScale(frameTimeUs));
#else
            m_pModule->TurnCamera(fMouseSen * fMouseDX * static_cast<float>(nFlip), fFrameTime);
            m_pModule->TiltCamera(fMouseSen * fMouseDY / 10.0f * static_cast<float>(nFlip));
#endif
        } else {
#else
        int nTurn = (pCamOpts->m_bGameplay1 ? (bMouseCamRotate == 0) : bMouseCamRotate)
                    || g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK;
        if (nTurn) {
            int nFlip = CameraHorzAxisFlip ? 1 : -1;
#ifdef K2_FIX_ENGINE_BUGS
            m_pModule->TurnCamera(fMouseSen * fMouseDX * static_cast<float>(nFlip), MouseLookTurnTime(fFrameTime));
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
                m_pModule->TiltCamera(fMouseSen * fMouseDY / 10.0f * MouseLookTiltScale(frameTimeUs));
#else
            m_pModule->TurnCamera(fMouseSen * fMouseDX * static_cast<float>(nFlip), fFrameTime);
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
                m_pModule->TiltCamera(fMouseSen * fMouseDY / 10.0f);
#endif
        } else {
#endif
            m_pModule->TurnCamera(0.0f, fFrameTime);
        }
    }

    if (g_pExoInput->GetActive() == 0)
        m_pModule->TurnCamera(0.0f, 0.0f);

    if (m_fCameraCollision != 0.0f)
        m_pModule->m_bCameraZoomed = 1;

    if (pCamera) {
        float fDist = pCamera->GetDist();
        float fExpand = (fDist * 0.2f) / 15.0f;
        g_HitCheckBoxExpansion.x = fExpand;
        g_HitCheckBoxExpansion.y = fExpand;
        g_HitCheckBoxExpansion.z = fExpand;
    }
    pCamera->GetPosition();

    Vector vScroll(0.0f, 0.0f, 0.0f);
    if (m_nScrollSpeedX)
        vScroll.x = pCamera->MouseDir(40, 0).x * static_cast<float>(m_nScrollSpeedX);
    if (m_nScrollSpeedY)
        vScroll.y = pCamera->MouseDir(0, 40).y * static_cast<float>(m_nScrollSpeedY);
    float fDeltaSeconds = static_cast<float>(frameTimeUs) / 1000000.0f;
    m_pModule->ScrollCamera(vScroll);

    g_bKeepHistory = bKeepHistory;
    if (m_oidCreatureCache != m_playerCreatureId) {
        m_pCreatureCache = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_playerCreatureId);
        m_oidCreatureCache = m_playerCreatureId;
    }

    m_pModule->UpdateCameraBumpAndShake(fDeltaSeconds);
    if (m_combatMode == 0)
        m_pModule->UpdateCameraFollow(fDeltaSeconds);
    m_fCameraCollision = m_pModule->UpdateCameraCollision(fDeltaSeconds, m_fCameraCollision);
    if (fabsf(m_fCameraCollision) > 0.0001f)
        m_pModule->ZoomCamera(m_fCameraCollision);

    if (g_pAppManager->m_pClientExoApp->GetCaptureEvents() == 1 && m_pModule->m_bCameraMoved == 1) {
        float fA = pCamera->GetDist();
        float fB = pCamera->GetPitch();
        float fC = pCamera->GetYaw();
        CExoString sCapture;
        unsigned long long uTime = static_cast<unsigned long long>(
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetSnapshotTime());
        sCapture.Format(const_cast<char*>("%d %c %f %f %f\n"), uTime / 1000, 'V', fA, fB, fC);
        static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile())->Write(sCapture);
        m_pModule->m_bCameraMoved = 0;
    }

    m_fCameraCollision = 0.0f;
    m_nReserved6 = 0;
    m_nReserved7 = 0;
    m_pModule->m_bCameraMoved = 0;
#endif
}

void CClientExoAppInternal::UpdateRoom(Vector const& vPos)
{
    if (m_pModule) {
        CSWCArea* pArea = m_pModule->m_pArea;
        if (pArea) {
            Scene* pScene = static_cast<Scene*>(pArea->m_pScene);
            if (pScene) {
                if (pScene->GetIgnoreVisibilityGraph()) {
                    // The visibility graph is already active, so there is nothing to do.
                } else {
                    int nRoomIndex = pArea->GetRoomIndex(vPos, 0);
                    CSWCRoom* pRoom = pArea->GetRoom(nRoomIndex);
                    if (nRoomIndex < 0)
                        g_pAppManager->m_pClientExoApp->GetInGameGui();
                    else {
                        pScene->SetCurrentRoom(pRoom->m_nSceneRoomIndex);
                        if (pRoom) {
                            g_pExoSound->SetDesiredEnvironment(pRoom->m_nEnvAudioType);
                            g_pExoSound->SetDesiredAmbientScale(pRoom->m_fEnvAudioValue);
                        }
                    }
                }
            }
        }
    }
}


extern const char* GetOverridePath(int index, bool flag);

#if !KOTOR1 || defined(_MSC_VER)
static int sprintf_s(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int nResult = __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
    return nResult;
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
char* FindMovieInOverrides(const char* pszMovieName)
{
    static char buffer[256];
    CExoString sPath;
    const char* pszOverridePath = 0;
    for (int nOverride = 0; ; nOverride++) {
        CExoFile* pFile;
        pszOverridePath = GetOverridePath(nOverride, true);
        if (pszOverridePath == 0)
            break;
        sPath.Format(const_cast<char*>("%s\\movies\\%s"), const_cast<char*>(pszOverridePath),
                     const_cast<char*>(pszMovieName));
        pFile = new CExoFile(sPath, RESTYPE_BIK, CExoString("rb"));
        if (pFile->FileOpened())
            break;
        delete pFile;
    }
    if (pszOverridePath == 0)
        sPath.Format(const_cast<char*>("MOVIES:%s"), const_cast<char*>(pszMovieName));
    sprintf_s(buffer, sizeof(buffer), "%s", sPath.CStr());
    return buffer;
}
#endif // !KOTOR1

// Console command.
char* testmovie(char* pszMovie)
{
    K2_UNUSED(pszMovie);
    {
        CClientExoAppInternal* p = static_cast<CClientExoAppInternal*>(g_pAppManager->m_pClientExoApp->m_pInternal);
        CExoString sMovie("55");
        p->m_movieFlags.Add(0);
        p->AddMovieToExoArrayList(sMovie, p->m_moviesToPlay);
    }
    {
        CClientExoAppInternal* p = static_cast<CClientExoAppInternal*>(g_pAppManager->m_pClientExoApp->m_pInternal);
        CExoString sCredits("credits");
        p->m_movieFlags.Add(1);
        p->AddMovieToExoArrayList(sCredits, p->m_moviesToPlay);
    }
#if KOTOR1  // plays the queue through PlayMovieQueue(1)
    static_cast<CClientExoAppInternal*>(g_pAppManager->m_pClientExoApp->m_pInternal)->PlayMovieQueue(1);
#else
    CClientExoAppInternal* p = static_cast<CClientExoAppInternal*>(g_pAppManager->m_pClientExoApp->m_pInternal);
    int nUnused;  // passed by reference to PlayMoviesInExoArrayList, which writes it; unread here
    p->PlayMoviesInExoArrayList(p->m_moviesToPlay, nUnused, 0, 1, &p->m_movieFlags, 0);
    p->m_moviesToPlay.m_count = 0;
    p->m_movieFlags.m_count = 0;
#endif
    return const_cast<char*>("Success");
}

#if KOTOR1
int CClientExoAppInternal::PlayMoviesInExoArrayList(CExoArrayList<CExoString>& rMovies,
                                                    int& nPlayed, int nWaitFlag, int bLoop,
                                                    CExoArrayList<int>* pChapters)
{
    if (m_bDisableMovies)
        return 0;

    if (SDL_IsChromebook())
        SDL_SetHint("SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH", "0");
    m_pInGameGui->SetGlobalUnpauseableState(1);
    nPlayed = 0;
    int bInputActive = g_pExoInput->GetActive();
    if (!bInputActive) {
        if (g_pExoInput)
            g_pExoInput->SetActive(1);
        ShowMouse(0);
    }
    g_pExoInput->ClearEvents();
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL);
    m_pMoviePlayer->ShowFadePanel();
    if (m_bEvilEndingHackInProgress) {
        if (m_pEvilEndingMusic) {
            m_pEvilEndingMusic->Stop();
            delete m_pEvilEndingMusic;
        }
        g_pAppManager->m_pClientExoApp->GetInGameGui()->LoadAmbientTrackForDialog(CResRef(""));
        m_pEvilEndingMusic = new CExoStreamingSoundSource();
        unsigned char nVol =
            static_cast<unsigned char>(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMovieVolume *
                            127.0f * 0.01f);
        m_pEvilEndingMusic->SetVolume(nVol);
        m_pEvilEndingMusic->Play(CResRef("evil_ending"), 8, 0, 1, 0);
    }
    m_pMoviePlayer->PlayMoviesAsync(rMovies, bLoop, pChapters);
    Sleep(2);
    WaitForMovie(nWaitFlag);
    nPlayed = 0;
    if (SDL_IsChromebook())
        SDL_SetHint("SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH", "1");
    m_pMoviePlayer->HideFadePanel();
    rMovies.m_count = 0;
    if (pChapters)
        pChapters->m_count = 0;
    if (!bInputActive)
        SetInputActive(0, 0);
    m_pInGameGui->SetGlobalUnpauseableState(0);
    return 1;
}
#else
int CClientExoAppInternal::PlayMoviesInExoArrayList(CExoArrayList<CExoString>& rMovies,
                                                    int& nPlayed, int nWaitFlag, int bLoop,
                                                    CExoArrayList<int>* pChapters, int nSoundArg)
{
    if (m_bDisableMovies)
        return 0;

    m_pInGameGui->SetGlobalUnpauseableState(1);
    nPlayed = 0;
    int bInputActive = g_pExoInput->GetActive();
    if (!bInputActive) {
        if (g_pExoInput)
            g_pExoInput->SetActive(1);
        ShowMouse(0);
    }
    g_pExoInput->ClearEvents();
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL, nSoundArg);
    HideMouse(MOUSEHIDE_MOVIE);
    ChangeResolutionForMovie();
    m_pMoviePlayer->ShowFadePanel();
    m_pMoviePlayer->PlayMoviesAsync(rMovies, bLoop, pChapters);
    Sleep(0);
    WaitForMovie(nWaitFlag);
    nPlayed = 0;
    RestoreResolutionFromMovie();
    ShowMouse(MOUSEHIDE_MOVIE);
    m_pMoviePlayer->HideFadePanel();
    rMovies.m_count = 0;
    if (pChapters)
        pChapters->m_count = 0;
    if (!bInputActive)
        SetInputActive(0, 0);
    m_pInGameGui->SetGlobalUnpauseableState(0);
    return 1;
}
#endif

void CClientExoAppInternal::AddMovieToMovieQueue(const CExoString& sMovie, int nFlags)
{
    m_movieFlags.Add(nFlags);
    AddMovieToExoArrayList(sMovie, m_moviesToPlay);
}

int CClientExoAppInternal::AddMovieToExoArrayList(const CExoString& sMovie,
                                                  CExoArrayList<CExoString>& rList)
{
    if (sMovie != "") {
        int nColon;
        {
            CExoString sColon(":");
            nColon = sMovie.Find(sColon, 0);
        }
        if (nColon != -1) {
            rList.AddUnique(sMovie);
            return 1;
        }

        CExoString sResolved;
        CExoString sAlias;
#if KOTOR1  // no override search, the MOVIES: alias directly
        sResolved.Format(const_cast<char*>("MOVIES:%s"), sMovie.CStr());

        CExoFile* pFile = new CExoFile(sResolved, RESTYPE_BIK, "rb");
#else
        char* pszFound = FindMovieInOverrides(sMovie.CStr());
        sResolved = pszFound;

        CExoFile* pFile = new CExoFile(pszFound, RESTYPE_BIK, "rb");
#endif
        if (!pFile->FileOpened())
            goto retry;
    success:
        delete pFile;
        sResolved = g_pExoBase->m_pAliasList->ResolveFileName(sResolved, RESTYPE_BIK);
        rList.AddUnique(sResolved);
        m_pClientOptions->SetMovieShown(sResolved, 1);
        return 1;
    retry:
        for (unsigned int i = 1; i < CTlkTable::NUM_CONTENT_PKGS; i++) {
            delete pFile;
            pFile = 0;
            sAlias.Format(const_cast<char*>("LIVE%d"), i);
            if (g_pExoBase->m_pAliasList->GetAliasPath(sAlias, 0).GetLength() != 0) {
                sResolved.Format(const_cast<char*>("LIVE%d:movies\\%s"), i, sMovie.CStr());
                pFile = new CExoFile(sResolved, RESTYPE_BIK, "rb");
                if (pFile->FileOpened())
                    goto success;
            }
        }
        if (pFile)
            delete pFile;
        return 0;
    }
    return 0;
}

int CClientExoAppInternal::IsPlayerCreature(CSWCCreature* pCreature)
{
    if (!pCreature)
        return false;

    unsigned long long nId = m_playerCreatureId;
    if (nId == OBJECT_INVALID) {
        unsigned long long nServerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
        nId = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(nServerId);
    }
    return pCreature->m_objectId == nId;
}

int CClientExoAppInternal::StartMovie(const CResRef& ref)
{
    if (!m_pMoviePlayer)
        return 0;

    CExoString sMovie;
    ref.CopyToString(sMovie);
    return m_pMoviePlayer->StartMovie(sMovie);
}

void CClientExoAppInternal::StopMovie()
{
}

int CClientExoAppInternal::WaitForMovie(int a)
{
    int bInputActive = g_pExoInput->GetActive();
    auto bRenderActive = g_bRenderWindowActive;
    if (g_pExoInput)
        g_pExoInput->SetActive(1);
    ShowMouse(0);

#if KOTOR1  // no evil-ending music, and a tight wait with no message pump
    while (m_pMoviePlayer && m_pMoviePlayer->IsMoviePlaying() == 1)
        ;
#else
    if (m_bEvilEndingHackInProgress) {
        if (m_pEvilEndingMusic) {
            m_pEvilEndingMusic->Stop(0);
            delete m_pEvilEndingMusic;
        }
        g_pAppManager->m_pClientExoApp->GetInGameGui()->LoadAmbientTrackForDialog(CResRef(""));
        m_pEvilEndingMusic = new CExoStreamingSoundSource(0);
        unsigned char nVol =
            static_cast<unsigned char>(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMovieVolume *
                            127.0f * 0.01f);
        m_pEvilEndingMusic->SetVolume(nVol);
        m_pEvilEndingMusic->Play(CResRef("evil_ending"), 8, 0, 1, 0);
    }

    if (m_pMoviePlayer) {
        int nCount = 0;
        bool bStop = false;
        while (m_pMoviePlayer && m_pMoviePlayer->IsMoviePlaying() == 1) {
            if (bStop || nCount != 500) {
                // Skip the pump on this frame.
            } else {
                messagepump();
                if (m_bWMCLOSEQuery == 1) {
                    if (!m_pMoviePlayer)
                        break;
                    m_pMoviePlayer->CancelMovie(1, 0);
                    bStop = true;
                } else {
                    bStop = false;
                }
                nCount = 0;
                continue;
            }
            nCount++;
        }
    }
#endif

    if (a) {
        g_pAppManager->m_pClientExoApp->RemoveMoveToModuleMovies();
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
        if (m_bShowMouseAtMovieEnd) {
            m_pInGameGui->SetGlobalUnpauseableState(0);
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->UnpauseWorldTimer();
            g_pAppManager->m_pServerExoApp->GetWorldTimer()->UnpauseWorldTimer();
            m_bShowMouseAtMovieEnd = 0;
        }
    }
    if (g_bRenderWindowActive == bRenderActive)
        SetInputActive(bInputActive, 0);
    ShowMouse(MOUSEHIDE_MOVIE);
    return 0;
}

int CClientExoAppInternal::PlayLoadModuleMovies()
{
    if (m_pMoviePlayer && !m_bDisableMovies) {
        CExoArrayList<CExoString> movies(*g_pAppManager->m_pClientExoApp->GetMoveToModuleMovies());
        if (movies.m_count > 0) {
            if (g_pAppManager->m_pServerExoApp) {
                g_pAppManager->m_pClientExoApp->GetWorldTimer()->PauseWorldTimer();
                g_pAppManager->m_pServerExoApp->GetWorldTimer()->PauseWorldTimer();
                m_pInGameGui->SetGlobalUnpauseableState(1);
                g_pExoInput->ClearEvents();
                g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL, 0);
            }
#if KOTOR1  // no resolution change; the load screen goes up after the movies start
            m_pMoviePlayer->ShowFadePanel();
            m_pMoviePlayer->PlayMoviesAsync(movies, 0, 0);
            ShowLoadScreen(1, 1, 0);
            Sleep(2);
#else
            ChangeResolutionForMovie();
            ShowLoadScreen(1, 1, 0, 1);
            m_pMoviePlayer->ShowFadePanel();
            m_pMoviePlayer->PlayMoviesAsync(movies, 0, 0);
            Sleep(0);
#endif
            m_bShowMouseAtMovieEnd = 1;
            return 1;
        }
    }
    return 0;
}

int CClientExoAppInternal::AddMoveToModuleMovie(const CExoString& sMovie)
{
    return AddMovieToExoArrayList(sMovie, m_moveToModuleMovies);
}

#if KOTOR1
int CClientExoAppInternal::PlayMovie(const CExoString& sMovie, int& nPlayingMovies, int a, int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    if (m_bDisableMovies)
        return 0;
    if (!AddMovieToExoArrayList(sMovie, m_moveToModuleMovies))
        return 0;
    return PlayMoviesInExoArrayList(m_moveToModuleMovies, nPlayingMovies, 1, 0, 0);
}
#else
int CClientExoAppInternal::PlayMovie(const CExoString& sMovie, int& nPlayingMovies, int a, int b,
                                     int c)
{
    K2_UNUSED(a); K2_UNUSED(b);
    if (m_bDisableMovies)
        return 0;
    if (!AddMovieToExoArrayList(sMovie, m_moveToModuleMovies))
        return 0;
    return PlayMoviesInExoArrayList(m_moveToModuleMovies, nPlayingMovies, 1, 0, 0, c);
}
#endif

bool IsPsButtonIcons()
{
    if (g_pExoInput == 0)
        return false;

    return g_pExoInput->IsPsShowButtonsController() != 0;
}

CSWCPlayer* CClientExoAppInternal::GetPlayerByPlayerID(unsigned int playerId)
{
    CExoLinkedListNode* pPrevNode = m_playerList.m_pInternal->m_pHead;
    CExoLinkedListNode* pNode = m_playerList.m_pInternal->m_pHead;
    if (pNode == 0)
        return 0;
    CSWCPlayer* pPlayer = static_cast<CSWCPlayer*>(pNode->m_pData);
    while (pPlayer != 0 && pPlayer->m_nPlayerId != playerId)
        pPlayer = static_cast<CSWCPlayer*>(m_playerList.m_pInternal->GetNext(pPrevNode));
    return pPlayer;
}

CSWCPlayer* CClientExoAppInternal::GetPlayerByGameObjectID(unsigned long long objectId)
{
    CExoLinkedListNode* pPrevNode = m_playerList.m_pInternal->m_pHead;
    CExoLinkedListNode* pNode = m_playerList.m_pInternal->m_pHead;
    if (pNode == 0)
        return 0;
    CSWCPlayer* pPlayer = static_cast<CSWCPlayer*>(pNode->m_pData);
    while (pPlayer != 0 && pPlayer->m_gameObjectId != objectId)
        pPlayer = static_cast<CSWCPlayer*>(m_playerList.m_pInternal->GetNext(pPrevNode));
    return pPlayer;
}

CSWCItem* CClientExoAppInternal::GetItemByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
        return pObject->AsSWCItem();
    return 0;
}

CSWCStore* CClientExoAppInternal::GetStoreByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_STORE)
        return pObject->AsSWCStore();
    return 0;
}

CSWCCreature* CClientExoAppInternal::GetCreatureByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
        return pObject->AsSWCCreature();
    return 0;
}

CSWCModule* CClientExoAppInternal::GetModuleByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_MODULE)
        return pObject->AsSWCModule();
    return 0;
}

CSWCArea* CClientExoAppInternal::GetAreaByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_AREA)
        return pObject->AsSWCArea();
    return 0;
}

CSWCTrigger* CClientExoAppInternal::GetTriggerByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
        return pObject->AsSWCTrigger();
    return 0;
}

CSWCProjectile* CClientExoAppInternal::GetProjectileByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_PROJECTILE)
        return pObject->AsSWCProjectile();
    return 0;
}

CSWCPlaceable* CClientExoAppInternal::GetPlaceableByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        return pObject->AsSWCPlaceable();
    return 0;
}

CSWCDoor* CClientExoAppInternal::GetDoorByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
        return pObject->AsSWCDoor();
    return 0;
}

CSWCAreaOfEffectObject* CClientExoAppInternal::GetAreaOfEffectByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT)
        return pObject->AsSWCAreaOfEffectObject();
    return 0;
}

CSWCSoundObject* CClientExoAppInternal::GetSoundObjectByGameObjectID(unsigned long long objectId)
{
    CGameObject* pObject = 0;
    if (m_pGameObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_SOUND)
        return pObject->AsSWCSoundObject();
    return 0;
}

CSWCPlayer* CClientExoAppInternal::GetPlayerByPlayerName(const CExoString& name)
{
    CExoLinkedListNode* pPrevNode = m_playerList.m_pInternal->m_pHead;
    CExoLinkedListNode* pNode = m_playerList.m_pInternal->m_pHead;
    CSWCPlayer* pPlayer = 0;
    if (pNode != 0) {
        pPlayer = static_cast<CSWCPlayer*>(pNode->m_pData);
        while (pPlayer != 0) {
            if (pPlayer->m_sPlayerName == name)
                break;
            pPlayer = static_cast<CSWCPlayer*>(m_playerList.m_pInternal->GetNext(pPrevNode));
        }
    }
    return pPlayer;
}

CSWCPlayer* CClientExoAppInternal::GetPlayerByCreatureName(const CExoString& name)
{
    CExoLinkedListNode* pNode = m_playerList.m_pInternal->m_pHead;
    CSWCPlayer* pPlayer = 0;
    if (m_playerList.m_pInternal->m_pHead != 0)
        pPlayer = static_cast<CSWCPlayer*>(m_playerList.m_pInternal->m_pHead->m_pData);
    while (pNode != 0) {
        if (pPlayer != 0 && pPlayer->m_sCharacterName == name)
            return pPlayer;
        pPlayer = static_cast<CSWCPlayer*>(m_playerList.m_pInternal->GetNext(pNode));
    }
    return 0;
}

void CClientExoAppInternal::RenderTilePathNodes(int a)
{
    K2_UNUSED(a);
}

void CClientExoAppInternal::PauseMovie()
{
    if (m_pMoviePlayer)
        m_pMoviePlayer->PauseMovie();
}

void CClientExoAppInternal::UnpauseMovie()
{
    if (m_pMoviePlayer)
        m_pMoviePlayer->UnpauseMovie();
}

void CClientExoAppInternal::ClearMovies()
{
    if (m_pMoviePlayer)
        m_pMoviePlayer->ClearMovies();
}

#if KOTOR1
void CClientExoAppInternal::InformMoviePlayerOfWindowsPosChanging(int a)
{
    K2_UNUSED(a);
}
#else
void CClientExoAppInternal::InformMoviePlayerOfWindowsPosChanging(int64_t a)
{
    K2_UNUSED(a);
}
#endif

void CClientExoAppInternal::InformMoviePlayerOfWindowsPosChanged()
{
    if (m_pMoviePlayer)
        m_pMoviePlayer->WindowsPosChanged();
}

#if KOTOR1
int CClientExoAppInternal::DontUseThisSetPlayerCreature(unsigned long long oid)
{
    CSWCCreature* pCreature = GetCreatureByGameObjectID(ServerToClientObjectId(oid));
    if (pCreature == 0)
        return 0;
    m_pCreatureCache = pCreature;
    m_oidCreatureCache = oid;
    m_playerCreatureId = oid;
    return 1;
}
#endif

unsigned long long CClientExoAppInternal::ServerToClientObjectId(unsigned long long serverId)
{
    return (serverId == OBJECT_INVALID) ? serverId : (serverId | CLIENT_OBJECT_ID_FLAG);
}

void CClientExoAppInternal::SaveOptions()
{
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SaveOptions();
}

int CClientExoAppInternal::CrownFutureLeader()
{
    if (m_futureLeader == OBJECT_INVALID)
        return 0;
    int nIndex = m_pParty->GetIndex(m_futureLeader);
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pTarget = pServer->GetCreatureByGameObjectID(
        pServer->ClientToServerObjectId(m_futureLeader));
#if !KOTOR1 || defined(K2_FIX_ENGINE_BUGS)  // K1 has no pTarget null check
    if (pTarget != 0) {
#endif
    unsigned long long oldLeader = m_playerCreatureId;
    if (nIndex > 0) {
        m_pParty->SetLeader(nIndex);
    } else if (nIndex == 0) {
        CExoLinkedListNode* pNode = pServer->GetPlayerList()->m_pInternal->m_pHead;
        CSWSPlayer* pPlayer = pNode != 0 ? static_cast<CSWSPlayer*>(pNode->m_pData) : 0;
        m_playerCreatureId = m_futureLeader;
        if (m_pParty)
            m_pParty->AddCharacter(m_futureLeader);
        pPlayer->SetGameObject(pTarget);
    }
    if (oldLeader != m_futureLeader) {
        CSWSCreature* pOld = pServer->GetCreatureByGameObjectID(
            pServer->ClientToServerObjectId(oldLeader));
        CExoArrayList<CSWVisibilityNode*> temp;
        temp.SetSize(pOld->m_visibilityList.m_count);
        for (int i = 0; i < pOld->m_visibilityList.m_count; i++)
            temp.m_pArray[i] = pOld->m_visibilityList.m_pArray[i];

        pOld->m_visibilityList.m_count = 0;
        pOld->m_visibilityList.SetSize(pTarget->m_visibilityList.m_count);
        for (int i = 0; i < pTarget->m_visibilityList.m_count; i++)
            pOld->m_visibilityList.m_pArray[i] = pTarget->m_visibilityList.m_pArray[i];

        pTarget->m_visibilityList.m_count = 0;
        pTarget->m_visibilityList.SetSize(temp.m_count);
        for (int i = 0; i < temp.m_count; i++)
            pTarget->m_visibilityList.m_pArray[i] = temp.m_pArray[i];
    }
    m_pPlayerControl->SetPlayer(m_futureLeader);
    m_futureLeader = OBJECT_INVALID;
    return 1;
#if !KOTOR1 || defined(K2_FIX_ENGINE_BUGS)
    }
    m_futureLeader = OBJECT_INVALID;
    return 0;
#endif
}

int CClientExoAppInternal::ChangeCharacterToNextLivingPartyMember(int bOverride,
                                                                  int bTeleport,
                                                                  unsigned char nRounds)
{
    if (nRounds == 0)
        return 1;
    if (g_pAppManager->m_pServerExoApp == 0)
        return 0;
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
    if (pPlayer == 0)
        return 0;
#if KOTOR1  // needs a party with a player character
    if (m_pParty == 0 || m_pParty->GetPlayerCharacter() == 0)
        return 0;
#endif

    unsigned long long origOid = m_pParty->GetPlayerCharacter()->m_objectId;
    bool bForce;
    CSWSCreature* pCreature = 0;
    if (bOverride) {
        pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
        if (pCreature == 0 || pCreature->GetDead() != 0 || pCreature->GetDeadTemp() != 0)
            bForce = true;
        else
            bForce = false;
    } else {
        bForce = true;
    }

    unsigned char nFound = 0;
    CSWCCreature* pNew = 0;
    int nTries;
    for (nTries = nRounds * 3;;) {
        this->ChangeCharacter(-2, 0);
        pNew = m_pParty->GetPlayerCharacter();
        if (pNew) {
            CSWSCreature* pSrv = pNew->GetServerCreature();
            bool bDead = pSrv->GetDead() != 0;
            CSWSCreature* pSrv2 = pNew->GetServerCreature();
            bool bHp = pSrv2->GetDeadTemp() != 0;
            if (bDead || bHp) {
                nTries--;
            } else if (bForce || pCreature->m_objectId == pPlayer->m_gameObjectId) {
                nFound++;
            }
        }
        if (nFound == nRounds)
            break;
        if (nTries <= 0)
            break;
    }

    if (nFound != nRounds)
        goto fallback;
    if (nTries < 1)
        goto fallback;
    if (m_pParty->GetPlayerCharacter()->m_objectId == origOid)
        return 0;
    if (bTeleport == 0) {
        return 1;
    } else {
#if KOTOR1
        int nPick = rand() % 3;
#else
        int nPick = rand_wincompatible() % 3;
#endif
        int nSound = nPick == 0 ? 7 : (nPick == 1 ? 8 : 9);
        pNew->PlaySoundSetSound(static_cast<unsigned short>(nSound), 1);
        return 1;
    }

fallback:
    if (nTries == 0)
        return 0;
    int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(pPlayer->m_gameObjectId);
    this->ChangeCharacter(nIndex, 0);
    return 0;
}

#if KOTOR1
int CClientExoAppInternal::LoadOptions()
{
    if (!m_pClientOptions->LoadOptions())
        m_pClientOptions->SetDefaultOptions();
    g_pAurora->ResetGammaRamp(g_pAppManager->m_pClientExoApp->GetClientOptions()->GetGammaSetting(), 0);
    return 1;
}
#else
int CClientExoAppInternal::LoadOptions(int nConfig)
{
    if (!m_pClientOptions->LoadOptions(nConfig))
        m_pClientOptions->SetDefaultOptions();
    g_pAurora->ResetGammaRamp(g_pAppManager->m_pClientExoApp->GetClientOptions()->GetGammaSetting(), 0);
    return 1;
}
#endif

void CClientExoAppInternal::LoadItemDropSoundList()
{
    if (m_pItemDropSounds)
        return;

    C2DA* p2da = new C2DA(CResRef("inventorysnds"), 0);
    if (p2da->Load2DArray()) {
        CExoString str;
        m_pItemDropSounds = new CResRef[p2da->m_nRows];
        for (unsigned char i = 0; i < p2da->m_nRows; i++) {
            int nRes;
            {
                CExoString sColumn("InventorySound");
                nRes = p2da->GetCExoStringEntry(i, sColumn, &str);
            }
            if (nRes)
                m_pItemDropSounds[i] = static_cast<char*>(str.CStr());
        }
        delete p2da;
    }
}

void CClientExoAppInternal::InitializeClientOptions()
{
    CExoIni ini;
    CExoString sValue;

#if KOTOR1
    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Game Options"),
                         CExoString("TooltipDelay Sec"))) {
        if (sValue.AsINT() <= 0) {
            sValue.Format(const_cast<char*>("1"));
            ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Game Options"),
                              CExoString("TooltipDelay Sec"));
        }
    } else {
        ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Game Options"),
                          CExoString("TooltipDelay Sec"));
    }

    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Graphics Options"),
                         CExoString("EnableHardwareMouse"))) {
        m_pClientOptions->SetEnableHardwareMouseCursor(sValue.AsINT() != 0);
    } else {
        m_pClientOptions->SetEnableHardwareMouseCursor(1);
        sValue.Format(const_cast<char*>("%d"), 1);
        ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Graphics Options"),
                          CExoString("EnableHardwareMouse"));
    }
#else
    int nConsent;
    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Game Options"),
                         CExoString("PrivacyConsent"))) {
        nConsent = sValue.AsINT();
        g_PrivacyChecked = 1;
        g_PrivacyConsentGiven = nConsent != 0;
        // The SDKs get the flag re-read from memory.
        AspyrSetupSDKs(*static_cast<volatile bool*>(&g_PrivacyConsentGiven));
    } else {
        g_PrivacyChecked = 0;
    }

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Game Options"),
                         CExoString("TooltipDelay Sec"))) {
        if (sValue.AsINT() <= 0) {
            sValue.Format(const_cast<char*>("1"));
            ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Game Options"),
                              CExoString("TooltipDelay Sec"));
        }
    } else {
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Game Options"),
                          CExoString("TooltipDelay Sec"));
    }

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                         CExoString("EnableHardwareMouse"))) {
        m_pClientOptions->SetEnableHardwareMouseCursor(sValue.AsINT() != 0);
    } else {
        m_pClientOptions->SetEnableHardwareMouseCursor(1);
        sValue.Format(const_cast<char*>("%d"), 1);
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                          CExoString("EnableHardwareMouse"));
    }
#endif
}

void CClientExoAppInternal::ShutDownLoadGameLoadBar()
{
    if (m_loadBarState) {
        g_pAppManager->m_pLoadGameErrorInfo->m_bStallFinished = 0;
        g_pAppManager->m_pLoadGameErrorInfo->m_flag = 0;
        g_pAppManager->m_pLoadGameErrorInfo->m_nStallEvent = CLoadGameErrorInfo::STALL_EVENT_NONE;
        m_loadBarState = 0;
    }
}

void CClientExoAppInternal::SetLoadScreenHint(uint64_t hint)
{
    if (m_pLoadScreenObj)
        m_pLoadScreenObj->SetHint(hint);
}

uint64_t CClientExoAppInternal::GetLoadScreenHint()
{
    if (m_pLoadScreenObj == 0)
        return 0;
    return m_pLoadScreenObj->m_nHintStrRef;
}

int CClientExoAppInternal::GetLoadBarProgress()
{
    if (m_pLoadScreenObj == 0)
        return 0;
    return m_pLoadScreenObj->m_progressBar.m_nCurValue;
}

void CClientExoAppInternal::SetLoadBarProgress(int nProgress, int b)
{
    g_pExoInput->UpdateJoysticks();
    if (m_pLoadScreenObj)
    {
#if KOTOR1
        if (b && nProgress <= 79)
#else
        if (b && nProgress <= 94)
#endif
            UpdateLoadingScreenHint();
        m_pLoadScreenObj->m_progressBar.SetCurValue(nProgress);
        CExoString sLoadBar;
        sLoadBar.Format(const_cast<char*>("Load Bar = %d"), nProgress);
    }
}

void CClientExoAppInternal::EndPazaakGame(int bWin, int nWager, int nRestart,
                                          unsigned long long nObj)
{
    m_pazaakResultB = nWager;
    m_pazaakResultA = bWin;
    SetInputClass(INPUTCLASS_GAME, 1);
    m_bPazaakIsRunning = 0;

    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CSWCCreature* pPlayer = pParty->GetPlayerCharacter();
    unsigned long long nServerId =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pPlayer->m_objectId);
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nServerId);

    if (nWager >= 1 && pCreature != 0) {
        int nGold = pCreature->GetGold();
        int nNewGold;
        if (bWin != 0) {
            nNewGold = nGold + nWager;
        } else {
            int nSub = nGold - nWager;
            nNewGold = nSub & ~(nSub >> 31);
        }
        int nDelta = nNewGold - nGold;
        if (nDelta != 0) {
            pCreature->SetGold(nNewGold);
            CGuiInGame* pInGame = g_pAppManager->m_pClientExoApp->GetInGameGui();
            pInGame->UpdateStatus(CGuiInGame::GUI_STATUS_CREDITS, nDelta);
        }
    }

    if (nRestart != 0) {
        StartPazaakGame(m_nPazaakOpponentDeck, m_sPazaakEndScript, 0, 1, nObj);
        return;
    }

    if (g_pVirtualMachine != 0) {
        if (m_sPazaakEndScript[0] != 0)
            g_pVirtualMachine->RunScript(&m_sPazaakEndScript, OBJECT_INVALID, 0);
    }
    m_pSWGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_REMOVED);
    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
}

void CClientExoAppInternal::StartPazaakGame(int nPlayerIndex, CExoString const& sName, int nWager,
                                            int nArg3, unsigned long long nArg4)
{
    if (m_bPazaakIsRunning)
        return;
    m_bPazaakIsRunning = 1;

    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CSWCCreature* pPlayer = pParty->GetPlayerCharacter();
    unsigned long long nServerId =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pPlayer->m_objectId);
    CSWSCreature* pServerCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nServerId);
    if (nWager > 0 && pServerCreature != 0) {
        int nGold = pServerCreature->GetGold();
        nWager = (nGold > nWager) ? nWager : nGold;
    }

    m_pazaakResultB = nWager;
    m_pazaakResultA = 0;
    m_sPazaakEndScript = sName;
    m_nPazaakOpponentDeck = nPlayerIndex;

    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);

    CSWGuiPazaakStart* pPanel =
        new CSWGuiPazaakStart(m_pSWGuiManager, nWager, nPlayerIndex, nArg3, nArg4);
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
    pPanel->AddExtraBorderPanel(m_pSWGuiManager, CExoString("bg_replacement"));
#endif

    SetInputClass(INPUTCLASS_GUI, 1);
    m_pSWGuiManager->AddPanel(pPanel, 3, 1);
    m_pSWGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_ADDED);
}

#if KOTOR1
void CClientExoAppInternal::SetEventDescriptions()
{
    g_pExoInput->ClearAllInputEventDescriptions();

    // Keyboard action keys.
    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_CAPSLOCK, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_CAPSLOCK, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_CAPSLOCK, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_CAPSLOCK, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_CAPSLOCK, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_CAPSLOCK, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_RETURN, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_RETURN, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RETURN, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RETURN, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RETURN, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_UP, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_UP_ARROW, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_UP, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_UP, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_DOWN, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_DOWN_ARROW, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_DOWN, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_DOWN, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_LEFT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_LEFT_ARROW, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_LEFT, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_LEFT, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_RIGHT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_RIGHT_ARROW, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RIGHT, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RIGHT, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_NUMPAD_ENTER, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_NUMPADENTER, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_NUMPAD_ENTER, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_NUMPAD_ENTER, INPUTCLASS_DIALOG);

    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bScreenShotEnabled) {
        g_pExoInput->CreateNewEvent(INPUTEVENT_SCREENSHOT, INPUTKIND_BUTTON,
                                    CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_PRINTSCREEN, INPUTCONTROL_NONE);
        g_pExoInput->AddEvent(INPUTEVENT_SCREENSHOT, INPUTCLASS_GAME);
        g_pExoInput->AddEvent(INPUTEVENT_SCREENSHOT, INPUTCLASS_GUI);
        g_pExoInput->AddEvent(INPUTEVENT_SCREENSHOT, INPUTCLASS_FREELOOK);
    }

    // Gamepad buttons, delivered as keyboard keys.
    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_A, INPUTKIND_BUTTON, CExoInput::INPUTDEVICE_KEYBOARD,
                                CExoInput::JOYSTICK_BUTTON0, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_A, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_A, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_B, INPUTKIND_BUTTON, CExoInput::INPUTDEVICE_KEYBOARD,
                                CExoInput::JOYSTICK_BUTTON1, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_B, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_B, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_X, INPUTKIND_BUTTON, CExoInput::INPUTDEVICE_KEYBOARD,
                                CExoInput::JOYSTICK_BUTTON2, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_X, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_X, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_Y, INPUTKIND_BUTTON, CExoInput::INPUTDEVICE_KEYBOARD,
                                CExoInput::JOYSTICK_BUTTON3, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_Y, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_Y, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_BLACK, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::JOYSTICK_BUTTON4, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_BLACK, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_BLACK, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_WHITE, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::JOYSTICK_BUTTON5, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_WHITE, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_WHITE, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_LEFT_SHOULDER, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::JOYSTICK_BUTTON9, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::JOYSTICK_BUTTON10, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, INPUTCLASS_DIALOG);

    // Joystick axes and buttons.
    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_MOVE_Y, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_YAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_MOVE_Y, INPUTCLASS_GAME);
    g_pExoInput->ScaleEvent(INPUTEVENT_PAD_MOVE_Y, 1.0f, 1.0f);

    g_pExoInput->CreateNewEvent(INPUTEVENT_FREELOOK_STICK_Y, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_YAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_FREELOOK_STICK_Y, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_MOVE_X, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_XAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_MOVE_X, INPUTCLASS_GAME);
    g_pExoInput->ScaleEvent(INPUTEVENT_PAD_MOVE_X, 1.0f, 1.0f);

    g_pExoInput->CreateNewEvent(INPUTEVENT_FREELOOK_STICK_X, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_XAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_FREELOOK_STICK_X, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_CAMERA_TILT, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_SLIDER0, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_CAMERA_TILT, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_CAMERA_TILT, INPUTCLASS_FREELOOK);
    g_pExoInput->ScaleEvent(INPUTEVENT_PAD_CAMERA_TILT, 1.0f, 1.0f);

    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_CAMERA_TURN, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_SLIDER1, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_CAMERA_TURN, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_CAMERA_TURN, INPUTCLASS_FREELOOK);
    g_pExoInput->ScaleEvent(INPUTEVENT_PAD_CAMERA_TURN, 1.0f, 1.0f);

    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_MENU, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_BUTTON8, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_MENU, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_MENU, INPUTCLASS_FREELOOK);

    // Mouse axes and buttons.
    g_pExoInput->CreateNewEvent(INPUTEVENT_MINIGAME_MOUSE_X, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MINIGAME_MOUSE_X, INPUTCLASS_MINIGAME);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MINIGAME_MOUSE_Y, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MINIGAME_MOUSE_Y, INPUTCLASS_MINIGAME);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MINIGAME_MOUSE_LEFT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_BUTTON0, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MINIGAME_MOUSE_LEFT, INPUTCLASS_MINIGAME);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_X, INPUTKIND_AXIS, CExoInput::INPUTDEVICE_MOUSE,
                                CExoInput::MOUSE_XAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_X, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_X, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_X, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_X, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_Y, INPUTKIND_AXIS, CExoInput::INPUTDEVICE_MOUSE,
                                CExoInput::MOUSE_YAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_Y, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_Y, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_Y, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_Y, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_LEFT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_BUTTON0, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_LEFT, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_LEFT, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_LEFT, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_LEFT, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_RIGHT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_BUTTON1, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_RIGHT, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_RIGHT, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_RIGHT, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_RIGHT, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_MIDDLE, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_BUTTON2, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_MIDDLE, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_MIDDLE, INPUTCLASS_FREELOOK);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_MIDDLE, INPUTCLASS_GAME);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_WHEEL, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_ZAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_WHEEL, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_WHEEL, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_WHEEL, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_WHEEL, INPUTCLASS_FREELOOK);

    CExoIni   ini;
    CExoString row;  // the "Action<i>[A|B]" row key
    CExoString value;  // ReadIniEntry / Format result
    CExoString langCol;  // "Language<clientLanguage>" column name
    int used[256];
    memset(used, 0, sizeof(used));

    C2DA* pKeymap = g_pRules->m_p2DArrays->m_pKeymap;
    if (pKeymap) {
        langCol = CExoString("Language") + CExoString(m_clientLanguage);

        // Scan the keyboard range for the first binding that is not the disabled marker and has a
        // valid-language entry.
        int iScan;
        for (iScan = INPUTEVENT_KEYMAP_FIRST; iScan < INPUTEVENT_KEYMAP_END; iScan++) {
            row = CExoString("Action") + CExoString(iScan);
            int nDisabled;
            int bDisabled;
            {
                CExoString colDisabled("Disabled");
                bDisabled = pKeymap->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1;
            }
            if (iScan == INPUTEVENT_FREELOOK || bDisabled)
                continue;
            int nLang;
            if (pKeymap->GetINTEntry(row, langCol, &nLang) && nLang >= 0)
                break;
        }
        if (iScan == INPUTEVENT_KEYMAP_END)
            langCol = CExoString("Language") + CExoString(0);
    }

    for (int i = INPUTEVENT_KEYMAP_FIRST; i < INPUTEVENT_KEYMAP_END; i++) {
        row = CExoString("Action") + CExoString(i);

        C2DA* pkm = g_pRules->m_p2DArrays->m_pKeymap;
        int bDisabled;
        {
            CExoString colDisabled("Disabled");
            int nDisabled;
            bDisabled = pkm->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1;
        }
        if (i == INPUTEVENT_FREELOOK || bDisabled)
            continue;

        // The .ini override wins first.
        int key = 0;
        if (ini.ReadIniEntry(value, CExoString("swkotor.ini"), CExoString("Keymapping"), row)) {
            key = value.AsINT() & 0xff;
            if (key != 0 && used[key])
                continue;

            int nEventType = 0;
            C2DA* pk2 = g_pRules->m_p2DArrays->m_pKeymap;
            if (pk2) {
                int r2;
                {
                    CExoString colET("EventType");
                    r2 = pk2->GetINTEntry(row, colET, &nEventType);
                }
                if (r2 && nEventType == 1) {
                    g_pExoInput->CreateNewEvent(i, INPUTKIND_STATE, CExoInput::INPUTDEVICE_KEYBOARD,
                                                key, INPUTCONTROL_NONE);
                    continue;
                }
            }
            g_pExoInput->CreateNewEvent(i, INPUTKIND_BUTTON, CExoInput::INPUTDEVICE_KEYBOARD,
                                        key, INPUTCONTROL_NONE);
            continue;
        }

        // No .ini row, so the 2DA default binding stands.
        pkm = g_pRules->m_p2DArrays->m_pKeymap;
        if (pkm == 0)
            continue;
        if (!pkm->GetINTEntry(row, langCol, &key))
            continue;
        if (key < 0)
            continue;
        if (i == INPUTEVENT_LEFT_LOOKABOUT)  // always Left Alt
            key = CExoInput::KEYBOARD_LEFTALT;

        if (key == 0 || !used[key]) {
            int nEventType = 0;
            C2DA* pk3 = g_pRules->m_p2DArrays->m_pKeymap;
            if (pk3) {
                int r2;
                {
                    CExoString colET("EventType");
                    r2 = pk3->GetINTEntry(row, colET, &nEventType);
                }
                if (r2 && nEventType == 1)
                    g_pExoInput->CreateNewEvent(i, INPUTKIND_STATE, CExoInput::INPUTDEVICE_KEYBOARD,
                                                key, INPUTCONTROL_NONE);
                else
                    g_pExoInput->CreateNewEvent(i, INPUTKIND_BUTTON,
                                                CExoInput::INPUTDEVICE_KEYBOARD, key, INPUTCONTROL_NONE);
            } else {
                g_pExoInput->CreateNewEvent(i, INPUTKIND_BUTTON, CExoInput::INPUTDEVICE_KEYBOARD,
                                            key, INPUTCONTROL_NONE);
            }
            if (key == 0)
                continue;
        }

        // A non-empty binding derived from the 2DA default is saved back to the .ini.
        value.Format(const_cast<char*>("%d"), key);
        ini.WriteIniEntry(value, CExoString("swkotor.ini"), CExoString("Keymapping"), row);
    }

    for (int i = INPUTEVENT_KEYMAP_AXIS_FIRST; i < INPUTEVENT_KEYMAP_AXIS_END; i++) {
        row = (CExoString("Action") + CExoString(i)) + CExoString("A");

        C2DA* pkm = g_pRules->m_p2DArrays->m_pKeymap;
        {
            CExoString colDisabled("Disabled");
            int nDisabled;
            if (pkm->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1)
                continue;
        }

        int keyA = -1;
        int keyB = -1;

        if (ini.ReadIniEntry(value, CExoString("swkotor.ini"), CExoString("Keymapping"), row)) {
            keyA = value.AsINT() & 0xff;
        } else {
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm) {
                pkm->GetINTEntry(row, langCol, &keyA);
                if (keyA != 0) {
                    value.Format(const_cast<char*>("%d"), keyA);
                    ini.WriteIniEntry(value, CExoString("swkotor.ini"), CExoString("Keymapping"), row);
                }
            }
        }

        row = (CExoString("Action") + CExoString(i)) + CExoString("B");
        if (ini.ReadIniEntry(value, CExoString("swkotor.ini"), CExoString("Keymapping"), row)) {
            keyB = value.AsINT() & 0xff;
        } else {
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm) {
                pkm->GetINTEntry(row, langCol, &keyB);
                if (keyB != 0) {
                    value.Format(const_cast<char*>("%d"), keyB);
                    ini.WriteIniEntry(value, CExoString("swkotor.ini"), CExoString("Keymapping"), row);
                }
            }
        }

        // Only a non-colliding pair is bound: used[keyA] wins unless it is empty, in which case an
        // empty pair is bound or keyB when it is a different, unused key.
        if (((keyA | keyB) & 0x80000000) != 0)
            continue;
        if (used[keyA]) {
            if ((keyA | keyB) != 0)
                continue;
        } else {
            if ((keyA | keyB) != 0 && (keyB == keyA || used[keyB]))
                continue;
        }
        g_pExoInput->CreateNewEvent(i, INPUTKIND_BUTTON_AXIS, CExoInput::INPUTDEVICE_KEYBOARD,
                                    keyB, keyA);
    }

    for (int i = INPUTEVENT_KEYMAP_FIRST; i < INPUTEVENT_KEYMAP_END; i++) {
        row = CExoString("Action") + CExoString(i);

        C2DA* pkm = g_pRules->m_p2DArrays->m_pKeymap;
        int bDisabled;
        {
            CExoString colDisabled("Disabled");
            int nDisabled;
            bDisabled = pkm->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1;
        }
        if (i == INPUTEVENT_FREELOOK || bDisabled)
            continue;

        if (g_pExoInput->EventExists(i)) {
            int nKey;
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICPC"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_GAME);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICMiniGame"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_MINIGAME);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICPCGUI"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_GUI);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICDialog"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_DIALOG);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICFreeLook"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_FREELOOK);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICMovie"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_MOVIE);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            {
                int nRep, nWait, nRate K2_UNSET;
                if (pkm->GetINTEntry(row, CExoString("Repeatable"), &nRep) && nRep) {
                    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("RepeatWait"), &nWait)
                        && g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("RepeatRate"), &nRate))
                        g_pExoInput->SetEventRepeatable(i, nWait, nRate);
                }
            }

            int nScale;
            float fMag, fExp K2_UNSET;
            if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("Scale"), &nScale) && nScale) {
                if (g_pRules->m_p2DArrays->m_pKeymap->GetFLOATEntry(row, CExoString("ScaleMag"), &fMag)
                    && g_pRules->m_p2DArrays->m_pKeymap->GetFLOATEntry(row, CExoString("ScaleExp"), &fExp))
                    g_pExoInput->ScaleEvent(i, fMag, fExp);
            }
        }
    }

    for (int i = INPUTEVENT_KEYMAP_AXIS_FIRST; i < INPUTEVENT_KEYMAP_AXIS_END; i++) {
        row = (CExoString("Action") + CExoString(i)) + CExoString("A");

        C2DA* pkm = g_pRules->m_p2DArrays->m_pKeymap;
        {
            CExoString colDisabled("Disabled");
            int nDisabled;
            if (pkm->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1)
                continue;
        }

        if (g_pExoInput->EventExists(i)) {
            int nKey;
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICPC"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_GAME);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICMiniGame"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_MINIGAME);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICPCGUI"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_GUI);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICDialog"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_DIALOG);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICFreeLook"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_FREELOOK);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICMovie"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_MOVIE);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            {
                int nRep, nWait, nRate K2_UNSET;
                if (pkm->GetINTEntry(row, CExoString("Repeatable"), &nRep) && nRep) {
                    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("RepeatWait"), &nWait)
                        && g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("RepeatRate"), &nRate))
                        g_pExoInput->SetEventRepeatable(i, nWait, nRate);
                }
            }

            int nScale;
            float fMag, fExp K2_UNSET;
            if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("Scale"), &nScale) && nScale) {
                if (g_pRules->m_p2DArrays->m_pKeymap->GetFLOATEntry(row, CExoString("ScaleMag"), &fMag)
                    && g_pRules->m_p2DArrays->m_pKeymap->GetFLOATEntry(row, CExoString("ScaleExp"), &fExp))
                    g_pExoInput->ScaleEvent(i, fMag, fExp);
            }
        }
    }
}
#else
void CClientExoAppInternal::SetEventDescriptions()
{
    g_pExoInput->ClearAllInputEventDescriptions();

    // Keyboard action keys.
    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_RETURN, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_RETURN, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RETURN, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RETURN, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RETURN, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_UP, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_UP_ARROW, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_UP, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_UP, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_DOWN, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_DOWN_ARROW, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_DOWN, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_DOWN, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_LEFT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_LEFT_ARROW, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_LEFT, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_LEFT, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_RIGHT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_RIGHT_ARROW, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RIGHT, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_RIGHT, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_KEY_NUMPAD_ENTER, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_NUMPADENTER, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_NUMPAD_ENTER, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_KEY_NUMPAD_ENTER, INPUTCLASS_DIALOG);

    g_pExoInput->CreateNewEvent(INPUTEVENT_SCREENSHOT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_KEYBOARD, CExoInput::KEYBOARD_PRINTSCREEN, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_SCREENSHOT, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_SCREENSHOT, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_SCREENSHOT, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_SCREENSHOT, INPUTCLASS_FREELOOK);
    g_pExoInput->AddEvent(INPUTEVENT_SCREENSHOT, INPUTCLASS_MINIGAME);

    // Joystick axes and buttons.
    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_MOVE_Y, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_YAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_MOVE_Y, INPUTCLASS_GAME);
    g_pExoInput->ScaleEvent(INPUTEVENT_PAD_MOVE_Y, 1.0f, 1.0f);

    g_pExoInput->CreateNewEvent(INPUTEVENT_FREELOOK_STICK_Y, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_YAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_FREELOOK_STICK_Y, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_MOVE_X, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_XAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_MOVE_X, INPUTCLASS_GAME);
    g_pExoInput->ScaleEvent(INPUTEVENT_PAD_MOVE_X, 1.0f, 1.0f);

    g_pExoInput->CreateNewEvent(INPUTEVENT_FREELOOK_STICK_X, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_XAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_FREELOOK_STICK_X, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_CAMERA_TILT, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_SLIDER0, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_CAMERA_TILT, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_CAMERA_TILT, INPUTCLASS_FREELOOK);
    g_pExoInput->ScaleEvent(INPUTEVENT_PAD_CAMERA_TILT, 1.0f, 1.0f);

    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_CAMERA_TURN, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_SLIDER1, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_CAMERA_TURN, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_CAMERA_TURN, INPUTCLASS_FREELOOK);
    g_pExoInput->ScaleEvent(INPUTEVENT_PAD_CAMERA_TURN, 1.0f, 1.0f);

    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_FREELOOK, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_RZAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_FREELOOK, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_FREELOOK, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_PAD_MENU, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_BUTTON6, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_MENU, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_PAD_MENU, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_STICK_Y, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_YAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_STICK_Y, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_STICK_Y, INPUTCLASS_DIALOG);
    g_pExoInput->SetEventRepeatable(INPUTEVENT_GUI_STICK_Y, 500, 20);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_STICK_X, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_XAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_STICK_X, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_STICK_X, INPUTCLASS_DIALOG);
    g_pExoInput->SetEventRepeatable(INPUTEVENT_GUI_STICK_X, 500, 20);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_RIGHT_STICK_Y, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_RYAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_RIGHT_STICK_Y, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_RIGHT_STICK_Y, INPUTCLASS_DIALOG);
    g_pExoInput->SetEventRepeatable(INPUTEVENT_GUI_RIGHT_STICK_Y, 250, 20);

    g_pExoInput->CreateNewEvent(INPUTEVENT_GUI_RIGHT_STICK_X, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_JOYSTICKSTART, CExoInput::JOYSTICK_RXAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_RIGHT_STICK_X, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_GUI_RIGHT_STICK_X, INPUTCLASS_DIALOG);
    g_pExoInput->SetEventRepeatable(INPUTEVENT_GUI_RIGHT_STICK_X, 500, 20);

    // Mouse axes and buttons.
    g_pExoInput->CreateNewEvent(INPUTEVENT_MINIGAME_MOUSE_X, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MINIGAME_MOUSE_X, INPUTCLASS_MINIGAME);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MINIGAME_MOUSE_Y, INPUTKIND_AXIS,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MINIGAME_MOUSE_Y, INPUTCLASS_MINIGAME);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MINIGAME_MOUSE_LEFT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_BUTTON0, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MINIGAME_MOUSE_LEFT, INPUTCLASS_MINIGAME);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MINIGAME_MOUSE_RIGHT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_BUTTON1, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MINIGAME_MOUSE_RIGHT, INPUTCLASS_MINIGAME);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_X, INPUTKIND_AXIS, CExoInput::INPUTDEVICE_MOUSE,
                                CExoInput::MOUSE_XAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_X, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_X, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_X, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_X, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_Y, INPUTKIND_AXIS, CExoInput::INPUTDEVICE_MOUSE,
                                CExoInput::MOUSE_YAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_Y, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_Y, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_Y, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_Y, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_LEFT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_BUTTON0, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_LEFT, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_LEFT, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_LEFT, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_LEFT, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_RIGHT, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_BUTTON1, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_RIGHT, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_RIGHT, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_RIGHT, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_RIGHT, INPUTCLASS_FREELOOK);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_MIDDLE, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_BUTTON2, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_MIDDLE, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_MIDDLE, INPUTCLASS_FREELOOK);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_MIDDLE, INPUTCLASS_GAME);

    g_pExoInput->CreateNewEvent(INPUTEVENT_MOUSE_WHEEL, INPUTKIND_BUTTON,
                                CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_ZAXIS, INPUTCONTROL_NONE);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_WHEEL, INPUTCLASS_GUI);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_WHEEL, INPUTCLASS_GAME);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_WHEEL, INPUTCLASS_DIALOG);
    g_pExoInput->AddEvent(INPUTEVENT_MOUSE_WHEEL, INPUTCLASS_FREELOOK);

    CExoIni   ini;
    CExoString row;  // the "Action<i>[A|B]" row key
    CExoString value;  // ReadIniEntry / Format result
    CExoString langCol;  // "Language<clientLanguage>" column name
    int used[256];
    memset(used, 0, sizeof(used));

    C2DA* pKeymap = g_pRules->m_p2DArrays->m_pKeymap;
    if (pKeymap) {
        langCol = CExoString("Language") + CExoString(m_clientLanguage);

        // Scan the keyboard range for the first binding that is not the disabled marker and has a
        // valid-language entry.
        int iScan;
        for (iScan = INPUTEVENT_KEYMAP_FIRST; iScan < INPUTEVENT_KEYMAP_END; iScan++) {
            row = CExoString("Action") + CExoString(iScan);
            int nDisabled;
            {
                CExoString colDisabled("Disabled");
                if (pKeymap->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1)
                    continue;
            }
            int nLang;
            if (pKeymap->GetINTEntry(row, langCol, &nLang) && nLang >= 0)
                break;
        }
        if (iScan == INPUTEVENT_KEYMAP_END)
            langCol = CExoString("Language") + CExoString(0);
    }

    for (int i = INPUTEVENT_KEYMAP_FIRST; i < INPUTEVENT_KEYMAP_END; i++) {
        row = CExoString("Action") + CExoString(i);

        C2DA* pkm = g_pRules->m_p2DArrays->m_pKeymap;
        {
            CExoString colDisabled("Disabled");
            int nDisabled;
            if (pkm->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1)
                continue;
        }

        // The .ini override wins first.
        int key = 0;
        if (ini.ReadIniEntry(value, CExoString(".\\swkotor2.ini"), CExoString("Keymapping"), row)) {
            key = value.AsINT() & 0xff;
            if (key != 0 && used[key])
                continue;

            int nEventType = 0;
            C2DA* pk2 = g_pRules->m_p2DArrays->m_pKeymap;
            if (pk2) {
                int r2;
                {
                    CExoString colET("EventType");
                    r2 = pk2->GetINTEntry(row, colET, &nEventType);
                }
                if (r2 && nEventType == 1) {
                    g_pExoInput->CreateNewEvent(i, INPUTKIND_STATE, CExoInput::INPUTDEVICE_KEYBOARD,
                                                key, INPUTCONTROL_NONE);
                    continue;
                }
            }
            g_pExoInput->CreateNewEvent(i, INPUTKIND_BUTTON, CExoInput::INPUTDEVICE_KEYBOARD,
                                        key, INPUTCONTROL_NONE);
            continue;
        }

        // No .ini row, so the 2DA default binding stands.
        pkm = g_pRules->m_p2DArrays->m_pKeymap;
        if (pkm == 0)
            continue;
        if (!pkm->GetINTEntry(row, langCol, &key))
            continue;
        if (key < 0)
            continue;

        if (key == 0 || !used[key]) {
            int nEventType = 0;
            C2DA* pk3 = g_pRules->m_p2DArrays->m_pKeymap;
            if (pk3) {
                int r2;
                {
                    CExoString colET("EventType");
                    r2 = pk3->GetINTEntry(row, colET, &nEventType);
                }
                if (r2 && nEventType == 1)
                    g_pExoInput->CreateNewEvent(i, INPUTKIND_STATE, CExoInput::INPUTDEVICE_KEYBOARD,
                                                key, INPUTCONTROL_NONE);
                else
                    g_pExoInput->CreateNewEvent(i, INPUTKIND_BUTTON,
                                                CExoInput::INPUTDEVICE_KEYBOARD, key, INPUTCONTROL_NONE);
            } else {
                g_pExoInput->CreateNewEvent(i, INPUTKIND_BUTTON, CExoInput::INPUTDEVICE_KEYBOARD,
                                            key, INPUTCONTROL_NONE);
            }
            if (key == 0)
                continue;
        }

        // A non-empty binding derived from the 2DA default is saved back to the .ini.
        value.Format(const_cast<char*>("%d"), key);
        ini.WriteIniEntry(value, CExoString(".\\swkotor2.ini"), CExoString("Keymapping"), row);
    }

    for (int i = INPUTEVENT_KEYMAP_AXIS_FIRST; i < INPUTEVENT_KEYMAP_AXIS_END; i++) {
        row = (CExoString("Action") + CExoString(i)) + CExoString("A");

        C2DA* pkm = g_pRules->m_p2DArrays->m_pKeymap;
        {
            CExoString colDisabled("Disabled");
            int nDisabled;
            if (pkm->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1)
                continue;
        }

        int keyA = -1;
        int keyB = -1;

        if (ini.ReadIniEntry(value, CExoString(".\\swkotor2.ini"), CExoString("Keymapping"), row)) {
            keyA = value.AsINT() & 0xff;
        } else {
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm) {
                pkm->GetINTEntry(row, langCol, &keyA);
                if (keyA != 0) {
                    value.Format(const_cast<char*>("%d"), keyA);
                    ini.WriteIniEntry(value, CExoString(".\\swkotor2.ini"), CExoString("Keymapping"), row);
                }
            }
        }

        row = (CExoString("Action") + CExoString(i)) + CExoString("B");
        if (ini.ReadIniEntry(value, CExoString(".\\swkotor2.ini"), CExoString("Keymapping"), row)) {
            keyB = value.AsINT() & 0xff;
        } else {
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm) {
                pkm->GetINTEntry(row, langCol, &keyB);
                if (keyB != 0) {
                    value.Format(const_cast<char*>("%d"), keyB);
                    ini.WriteIniEntry(value, CExoString(".\\swkotor2.ini"), CExoString("Keymapping"), row);
                }
            }
        }

        // Only a non-colliding pair is bound: used[keyA] wins unless it is empty, in which case an
        // empty pair is bound or keyB when it is a different, unused key.
        if (((keyA | keyB) & 0x80000000) != 0)
            continue;
        if (used[keyA]) {
            if ((keyA | keyB) != 0)
                continue;
        } else {
            if ((keyA | keyB) != 0 && (keyB == keyA || used[keyB]))
                continue;
        }
        g_pExoInput->CreateNewEvent(i, INPUTKIND_BUTTON_AXIS, CExoInput::INPUTDEVICE_KEYBOARD,
                                    keyB, keyA);
    }

    for (int i = INPUTEVENT_KEYMAP_FIRST; i < INPUTEVENT_KEYMAP_END; i++) {
        row = CExoString("Action") + CExoString(i);

        C2DA* pkm = g_pRules->m_p2DArrays->m_pKeymap;
        {
            CExoString colDisabled("Disabled");
            int nDisabled;
            if (pkm->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1)
                continue;
        }

        if (g_pExoInput->EventExists(i)) {
            int nKey;
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICPC"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_GAME);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICMiniGame"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_MINIGAME);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICPCGUI"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_GUI);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICDialog"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_DIALOG);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICFreeLook"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_FREELOOK);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICMovie"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_MOVIE);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            {
                int nRep, nWait, nRate K2_UNSET;
                if (pkm->GetINTEntry(row, CExoString("Repeatable"), &nRep) && nRep) {
                    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("RepeatWait"), &nWait)
                        && g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("RepeatRate"), &nRate))
                        g_pExoInput->SetEventRepeatable(i, nWait, nRate);
                }
            }

            int nScale;
            float fMag, fExp K2_UNSET;
            if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("Scale"), &nScale) && nScale) {
                if (g_pRules->m_p2DArrays->m_pKeymap->GetFLOATEntry(row, CExoString("ScaleMag"), &fMag)
                    && g_pRules->m_p2DArrays->m_pKeymap->GetFLOATEntry(row, CExoString("ScaleExp"), &fExp))
                    g_pExoInput->ScaleEvent(i, fMag, fExp);
            }
        }
    }

    for (int i = INPUTEVENT_KEYMAP_AXIS_FIRST; i < INPUTEVENT_KEYMAP_AXIS_END; i++) {
        row = (CExoString("Action") + CExoString(i)) + CExoString("A");

        C2DA* pkm = g_pRules->m_p2DArrays->m_pKeymap;
        {
            CExoString colDisabled("Disabled");
            int nDisabled;
            if (pkm->GetINTEntry(row, colDisabled, &nDisabled) && nDisabled == 1)
                continue;
        }

        if (g_pExoInput->EventExists(i)) {
            int nKey;
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICPC"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_GAME);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICMiniGame"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_MINIGAME);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICPCGUI"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_GUI);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICDialog"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_DIALOG);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICFreeLook"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_FREELOOK);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            if (pkm->GetINTEntry(row, CExoString("ICMovie"), &nKey) && nKey)
                g_pExoInput->AddEvent(i, INPUTCLASS_MOVIE);
            pkm = g_pRules->m_p2DArrays->m_pKeymap;
            {
                int nRep, nWait, nRate K2_UNSET;
                if (pkm->GetINTEntry(row, CExoString("Repeatable"), &nRep) && nRep) {
                    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("RepeatWait"), &nWait)
                        && g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("RepeatRate"), &nRate))
                        g_pExoInput->SetEventRepeatable(i, nWait, nRate);
                }
            }

            int nScale;
            float fMag, fExp K2_UNSET;
            if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(row, CExoString("Scale"), &nScale) && nScale) {
                if (g_pRules->m_p2DArrays->m_pKeymap->GetFLOATEntry(row, CExoString("ScaleMag"), &fMag)
                    && g_pRules->m_p2DArrays->m_pKeymap->GetFLOATEntry(row, CExoString("ScaleExp"), &fExp))
                    g_pExoInput->ScaleEvent(i, fMag, fExp);
            }
        }
    }
}
#endif

void CClientExoAppInternal::ShowLoadScreen(int bFade, int bMusic, int bPauseMusic, int bShow)
{
    if (g_pExoSound)
    {
        if (bPauseMusic)
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL, 0);
        else if (m_loadMusic != "")
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC, 0);
    }

    if (m_bLoadScreenNameSet != 0 && m_loadScreenName.GetLength() != 0)
    {
        CSWGuiLoadScreen* pLoadScreen = m_pLoadScreenObj;
        if (pLoadScreen == 0)
        {
            pLoadScreen = new CSWGuiLoadScreen(m_pSWGuiManager);
            m_pLoadScreenObj = pLoadScreen;
        }

        pLoadScreen->SetImage(CResRef(m_loadScreenName));

        m_pLoadScreenObj->SetSaving(bPauseMusic);
        {
            CSWGuiControlTail* pFlags = &m_pLoadScreenObj->m_lblLoading.m_tail;
            pFlags->bVisible = static_cast<unsigned char>(bShow);
        }

        if (m_pSWGuiManager)
        {
            if (!m_pSWGuiManager->PanelExists(m_pLoadScreenObj))
            {
                m_nLoadScreenHintLastTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
                if (bFade != 0 && m_pInGameGui)
                    m_pInGameGui->StartGlobalFade(0, 0.0f, 0.0f, Vector(0.0f, 0.0f, 0.0f));
                m_pSWGuiManager->AddPanel(m_pLoadScreenObj, 1, 1);
            }
        }

        SetInputActive(0, 0);
        HideMouse(MOUSEHIDE_LOAD_SCREEN);
        if (bMusic != 0)
            StartLoadMusic();
    }
}

#if KOTOR1
void IosTemporaryDisableInteraction(int b);
extern bool g_bIsInBackground;

void CClientExoAppInternal::ShowLoadScreen(int bFade, int bMusic, int bPauseMusic)
{
    if (g_pExoSound)
    {
        if (bPauseMusic)
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL);
        else if (m_loadMusic != "")
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC);
    }

    if (m_bLoadScreenNameSet != 0 && m_loadScreenName.GetLength() != 0)
    {
        CSWGuiLoadScreen* pLoadScreen = m_pLoadScreenObj;
        if (pLoadScreen == 0)
        {
            pLoadScreen = new CSWGuiLoadScreen(m_pSWGuiManager);
            m_pLoadScreenObj = pLoadScreen;
        }
        pLoadScreen->SetImage(CResRef(m_loadScreenName));
        m_pLoadScreenObj->SetSaving(bPauseMusic);

        if (m_pSWGuiManager)
        {
            if (!m_pSWGuiManager->PanelExists(m_pLoadScreenObj))
            {
                m_nLoadScreenHintLastTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
                if (bFade != 0 && m_pInGameGui)
                    m_pInGameGui->StartGlobalFade(0, 0.0f, 0.0f, Vector(0.0f, 0.0f, 0.0f));
                m_pSWGuiManager->AddPanel(m_pLoadScreenObj, 1, 1);
            }
        }

        SetInputActive(0, 0);
        HideMouse(MOUSEHIDE_LOAD_SCREEN);
        IosTemporaryDisableInteraction(60);
        if (bMusic != 0 && !g_bIsInBackground)
            StartLoadMusic();
    }
}

void CClientExoAppInternal::HideLoadScreen(int a, int c)
{
    ShowMouse(MOUSEHIDE_LOAD_SCREEN);
    IosTemporaryDisableInteraction(60);

    if (m_pLoadScreenObj && m_pSWGuiManager)
    {
        if (a != 0)
            m_pInGameGui->StartGlobalFade(0, 0.0f, 0.0f, Vector(0.0f, 0.0f, 0.0f));

        m_pSWGuiManager->PopModalPanel();
        if (m_pSWGuiManager->RemovePanel(m_pLoadScreenObj) != 0)
            m_pLoadScreenObj->AddHintToMessageBuffer();
        m_pLoadScreenObj->m_progressBar.SetCurValue(0);

        CSWGuiLoadScreen* pPanel = m_pLoadScreenObj;
        pPanel->SetBackground(CResRef(""));

        if (g_pExoInput)
            g_pExoInput->SetActive(1);

        ShowMouse(0);

        if (m_pLoadMusic)
        {
            m_pLoadMusic->Stop();
            delete m_pLoadMusic;
            m_pLoadMusic = 0;
        }

        if (a == 0 && c != 0)
            UpdateScreen(0.033333335f, 0, 0);
    }
}
#endif

void CClientExoAppInternal::HideLoadScreen(int a, int b, int c)
{
    K2_UNUSED(b);
    ShowMouse(MOUSEHIDE_LOAD_SCREEN);

    if (m_pLoadScreenObj && m_pSWGuiManager)
    {
        float v[4];

        if (a != 0)
            m_pInGameGui->StartGlobalFade(0, 0.0f, 0.0f, Vector(0.0f, 0.0f, 0.0f));

        m_pSWGuiManager->PopModalPanel();
        if (m_pSWGuiManager->RemovePanel(m_pLoadScreenObj) != 0)
            m_pLoadScreenObj->AddHintToMessageBuffer();
        m_pLoadScreenObj->m_progressBar.SetCurValue(0);

        CSWGuiLoadScreen* pPanel = m_pLoadScreenObj;
        pPanel->SetBackground(CResRef(""));

        if (g_pExoInput)
            g_pExoInput->SetActive(1);

        ShowMouse(0);

        if (a == 0 && c != 0)
        {
            AurGetClearColor(v);
            AurSetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            UpdateScreen(0.033333335f, 0, 0);
            AurSetClearColor(v[0], v[1], v[2], v[3]);
        }
    }
}

void CClientExoAppInternal::StartLoadMusic()
{
    if (m_pMenuMusic)
        StopMenuMusic();

    if (g_pExoSound && m_moveToModuleMovies.m_count == 0)
    {
        if (m_pLoadMusic)
        {
            if (m_pLoadMusic->IsPlaying())
                return;
            m_pLoadMusic->Resume();
        }
        else if (m_loadMusic != "")
        {
#if KOTOR1  // the stream is prefetched from HD0 and played with tag 4
            CExoString sPath;
            sPath.Format(const_cast<char*>("HD0:STREAMMUSIC\\%s"), m_loadMusic.CStr());
            g_pExoResMan->CacheResource(sPath, 4);
            m_pLoadMusic = new CExoStreamingSoundSource();
            m_pLoadMusic->SetLooping(1);
            m_pLoadMusic->SetPriorityGroup(PRIORITY_GROUP_STREAMS_MUSIC);
            m_pLoadMusic->Play(CResRef(m_loadMusic), 4, 0, 1, 0);
#else
            m_pLoadMusic = new CExoStreamingSoundSource(1);
            m_pLoadMusic->SetLooping(1);
            m_pLoadMusic->SetPriorityGroup(PRIORITY_GROUP_STREAMS_MUSIC);
            m_pLoadMusic->Play(CResRef(m_loadMusic), 8, 0, 1, 0);
#endif
        }
        else
        {
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_PAUSE_ALL, 0);
        }
    }
}

void CClientExoAppInternal::StartMenuMusic(int a, int nVolume, int nPriority)
{
#if KOTOR1  // the first argument picks the theme
    CExoString musicRes(a ? "mus_theme_cult" : "mus_theme_rep");
#else
    K2_UNUSED(a);
    CExoString musicRes("mus_sion");
#endif
    bool bDiff = false;
    if (m_pMenuMusic)
        bDiff = (m_pMenuMusic->GetFilePlaying() != musicRes);

    if (m_bStartMenuMusicOnMovieEnd || bDiff)
    {
        m_bStartMenuMusicOnMovieEnd = 0;
        if (bDiff)
            StopMenuMusic();

        if (m_pMenuMusic)
        {
            if (m_pMenuMusic->IsPlaying() == 0)
                m_pMenuMusic->Play(CResRef(musicRes), 8, 0, 1, 1);
        }
        else
        {
            m_pMenuMusic = new CExoStreamingSoundSource(0);
            m_pMenuMusic->SetLooping(1);
            m_pMenuMusic->SetVolume(static_cast<unsigned char>(nVolume));
            m_pMenuMusic->SetPriorityGroup(static_cast<unsigned char>(nPriority));
            m_pMenuMusic->Play(CResRef(musicRes), 8, 0, 1, 0);
        }
    }
}


int CClientExoAppInternal::GetLoadScreenVisible()
{
    if (m_pLoadScreenObj != 0 && m_pSWGuiManager != 0)
        return m_pSWGuiManager->PanelExists(m_pLoadScreenObj);
    return 0;
}

void CClientExoAppInternal::IndicateObject(CSWCObject* pObject, int b)
{
    unsigned long long objectId = OBJECT_INVALID;
    if (pObject)
        objectId = pObject->m_objectId;
    m_pInGameGui->SetMainInterfaceTarget(objectId, b);
}

void CClientExoAppInternal::SetCaptureEvents(const CExoString& sFile)
{
    m_captureEvents = 1;
    if (m_captureEventPlayBack == 1 || sFile == "") {
        m_captureEvents = 0;
    } else {
        if (m_pCaptureEventFile != 0) {
            delete m_pCaptureEventFile;
            m_pCaptureEventFile = 0;
        }
        CExoString sPath;
        sPath.Format(const_cast<char*>("HD0:%s"), sFile.CStr());
        m_pCaptureEventFile = new CExoFile(sPath, RESTYPE_TXT, CExoString("wb"));
        if (m_pCaptureEventFile != 0) {
            if (m_pCaptureEventFile->FileOpened()) {
                unsigned long long nStartMs =
                    g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000;
                CExoString sStart;
                sStart.Format(const_cast<char*>("Start: %d\n"), static_cast<int>(nStartMs));
                m_pCaptureEventFile->Write(sStart);
            } else {
                delete m_pCaptureEventFile;
                m_pCaptureEventFile = 0;
            }
        }
    }
}

void CClientExoAppInternal::SetCaptureEventPlayBack(const CExoString& sFile)
{
    m_captureEventPlayBack = 1;
    if (m_captureEvents == 1) {
        m_captureEventPlayBack = 0;
        return;
    }
    if (m_pCaptureEventFile != 0) {
        delete m_pCaptureEventFile;
        m_pCaptureEventFile = 0;
        while (m_playbackEvents.m_pInternal->m_pHead != 0)
            m_playbackEvents.m_pInternal->RemoveTail();
    }

    CExoString sPath;
    sPath.Format(const_cast<char*>("HD0:%s"), sFile.CStr());
    m_pCaptureEventFile = new CExoFile(sPath, RESTYPE_TXT, CExoString("rb"));
    if (m_pCaptureEventFile != 0) {
        if (m_pCaptureEventFile->FileOpened()) {
            ReadPlayBackInputEvents();
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->SetFixedSnapshotRate(
                1, m_captureEventPlayBackRate);
        } else {
            delete m_pCaptureEventFile;
            m_pCaptureEventFile = 0;
        }
    }
}

void CClientExoAppInternal::SetCaptureEventPlayBackRate(int rate)
{
    m_captureEventPlayBackRate = rate;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SetFixedSnapshotRate(1, m_captureEventPlayBackRate);
}

void CClientExoAppInternal::DisableVideoEffect()
{
    m_videoEffectType = VIDEO_EFFECT_NONE;
    AurDisableSaturation();
    AurDisableScanNoise();
#if !KOTOR1  // saturation and scan noise only
    AurDisableClairvoyance();
    AurDisableForceSight();
    AurEnableForceFury(0);
#endif
}

#if !KOTOR1
bool CClientExoAppInternal::CanFlourish()
{
    CSWCCreature* pClient;
    if (m_oidCreatureCache == m_playerCreatureId) {
        pClient = static_cast<CSWCCreature*>(m_pCreatureCache);
    } else {
        pClient = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_playerCreatureId);
        m_pCreatureCache = pClient;
        m_oidCreatureCache = m_playerCreatureId;
    }
    CSWSCreature* pServer;
    if (pClient &&
        (pServer = pClient->GetServerCreature()) != 0 &&
        pServer->GetDead() == 0 &&
        pServer->GetDeadTemp() == 0)
    {
        if (static_cast<CSWSArea*>(pServer->GetArea())->m_nPlayerRestrictMode != 0)
            return 0;
        CSWCAnimBase* pAnimBase = pServer->GetClientCreature()->GetAnimationBase();
        if (pAnimBase->m_nWeaponClass == 1)
            return 0;
        if (pAnimBase->m_nWeaponClass == 8)
            return 0;
    }
    return 1;
}
#endif

inline int CClientExoAppInternal::GetPlayerMovementInput()
{
    if (m_pPlayerControl == 0)
        return 0;
    if (m_pPlayerControl->GetLeftRight() != 0.0f)
        return 1;
    return m_pPlayerControl->GetUpDown() != 0.0f;
}

void CClientExoAppInternal::PlayerFlourishWeapons()
{
#if KOTOR1
    GetPlayerCreature()->ComputeWeaponOverlays(0, 1);

    CSWSCreature* pServer = GetPlayerCreature()->GetServerCreature();
    if (pServer == 0)
        goto lblMusic;
    if (pServer->GetDead() != 0)
        goto lblMusic;
    if (pServer->GetDeadTemp() != 0)
        goto lblMusic;
    {
        CSWCAnimBase* pAnimBase = pServer->GetClientCreature()->GetAnimationBase();
        if (static_cast<unsigned char>(pAnimBase->m_nWeaponClass - 2) > 2)
            return;
    }

    {
        unsigned long long nMainId = GetPlayerCreature()->GetEquippedItemID(EQUIP_SLOT_RIGHT_WEAPON);
        if (nMainId != OBJECT_INVALID) {
            CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(nMainId);
            if (pObj != 0 && pObj->AsSWCItem() != 0)
                pObj->AsSWCItem()->PowerItem(1, 0, 1, 0);
        }
        unsigned long long nOffId = GetPlayerCreature()->GetEquippedItemID(EQUIP_SLOT_LEFT_WEAPON);
        if (nOffId != OBJECT_INVALID) {
            CGameObject* pOffObj = g_pAppManager->m_pClientExoApp->GetGameObject(nOffId);
            if (pOffObj != 0 && pOffObj->AsSWCItem() != 0)
                pOffObj->AsSWCItem()->PowerItem(1, 0, 1, 0);
        }
        pServer->SetCombatState(1, 2);
    }

lblMusic:
    int bFlourishAnim =
        (m_pPlayerControl == 0) ||
        (m_pPlayerControl->GetLeftRight() == 0.0f &&
         m_pPlayerControl->GetUpDown() == 0.0f);

    CSWCCreature* pPlayer2 = GetPlayerCreature();
    if (bFlourishAnim) {
        pPlayer2->InsertFireForgetAnimation(SERVER_ANIMATION_WIELD_DRAW, 1500, 1.0f, 0, 0);
    } else {
        CSWCAnimBase* pAnimBase = pPlayer2->GetAnimationBase();
        CSWCAnimBase* pAnimBase2 = GetPlayerCreature()->GetAnimationBase();
        pAnimBase->SetOverlayAnimation(static_cast<unsigned short>(pAnimBase2->ServerToClientAnimation(SERVER_ANIMATION_WIELD_SWAP)),
                                       1);
    }
#else
    CSWCCreature* pPlayer = GetPlayerCreature();
    if (pPlayer->m_nSwitchWeaponsCooldown > 0)
        return;

    pPlayer->ComputeWeaponOverlays(0, 1);

    CSWSCreature* pServer = GetPlayerCreature()->GetServerCreature();
    if (pServer == 0)
        goto lblMusic;
    if (pServer->GetDead() != 0)
        goto lblMusic;
    if (pServer->GetDeadTemp() != 0)
        goto lblMusic;
    if (!CanFlourish())
        return;

    {
        unsigned long long nMainId = GetPlayerCreature()->GetEquippedItemID(EQUIP_SLOT_RIGHT_WEAPON);
        if (nMainId != OBJECT_INVALID) {
            CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(nMainId);
            if (pObj != 0 && pObj->AsSWCItem() != 0)
                pObj->AsSWCItem()->PowerItem(1, 0, 1, 0);
        }
        unsigned long long nOffId = GetPlayerCreature()->GetEquippedItemID(EQUIP_SLOT_LEFT_WEAPON);
        if (nOffId != OBJECT_INVALID) {
            CGameObject* pOffObj = g_pAppManager->m_pClientExoApp->GetGameObject(nOffId);
            if (pOffObj != 0 && pOffObj->AsSWCItem() != 0)
                pOffObj->AsSWCItem()->PowerItem(1, 0, 1, 0);
        }
        pServer->SetCombatState(1, 2, 0);
    }

lblMusic:
    int bFlourishAnim = !GetPlayerMovementInput();

    CSWCCreature* pPlayer2 = GetPlayerCreature();
    if (bFlourishAnim) {
        pPlayer2->InsertFireForgetAnimation(SERVER_ANIMATION_WIELD_DRAW, 1500, 1.0f, 0, 0);
    } else {
        CSWCAnimBase* pAnimBase = pPlayer2->GetAnimationBase();
        CSWCAnimBase* pAnimBase2 = GetPlayerCreature()->GetAnimationBase();
        pAnimBase->SetOverlayAnimation(static_cast<unsigned short>(pAnimBase2->ServerToClientAnimation(SERVER_ANIMATION_WIELD_SWAP)),
                                       1);
    }
    pPlayer->m_nSwitchWeaponsCooldown = 1000;
#endif
}

void CClientExoAppInternal::SetInputActive(int a, int b)
{
    if (g_pExoInput)
        g_pExoInput->SetActive(a);
    if (a == 0)
        TurnOffKeyboardModal();
    if (b)
        g_pExoInput->ClearEvents();
    if (a)
        ShowMouse(0);
}

#if KOTOR1
void CClientExoAppInternal::ReadAndSetVideoMode(int bPublish, int bFullScreen)
{
    unsigned int nWidth, nHeight, nRefresh;
    unsigned char nBpp;
    CExoString sString;
    CExoIni cIni;
    ReadVideoModeSettings(&nWidth, &nHeight, &nBpp, &nRefresh);
    m_pClientOptions->SetFullScreenEnabled(bFullScreen);
    int nMode = CAuroraInterface::GetExactVideoMode(nWidth, nHeight, nBpp, nRefresh);
    if (nMode == -1) {
        nMode = CAuroraInterface::GetNearestVideoMode(800, 600, 32, 60);
        _devicemodeA devMode;
        memset(&devMode, 0, sizeof(devMode));
        devMode.m_size = sizeof(devMode);
        EnumDisplaySettingsA(0, nMode, &devMode);
    }
    SetVideoMode(nMode, VIDEO_SCREEN_CURRENT);
    if (bPublish) {
        g_nScreenWidth = g_SDL_mode.w;
        g_nScreenHeight = g_SDL_mode.h;
        g_nBitsPerPixel = 32;
    }
}
#else
void CClientExoAppInternal::ReadAndSetVideoMode(int a, int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    unsigned int nWidth = 0, nHeight = 0, nRefresh = 0;
    unsigned char nBpp = 0;
    bool nResult;
    int nMode;
    PortDevMode devMode;
    CExoString sString;
    CExoIni cIni;
    g_fullScreenDesired = 1;
    nResult = ReadVideoModeSettings(&nWidth, &nHeight, &nBpp, &nRefresh);
    m_pClientOptions->SetFullScreenEnabled(1);
    nMode = CAuroraInterface::GetExactVideoMode(nWidth, nHeight, nBpp, nRefresh);
    if (nMode == -1 || nResult == 0) {
        if (nResult != 0) {
            nMode = CAuroraInterface::GetNearestVideoMode(1024, 768, 32, 60);
            memset(&devMode, 0, sizeof(devMode));
            devMode.m_size = sizeof(devMode);
            if (EnumDisplaySettingsA(0, nMode, &devMode) == 0)
                nMode = -1;
        } else {
            nMode = CAuroraInterface::GetNearestVideoMode(1600, 1200, 32, 60);
            memset(&devMode, 0, sizeof(devMode));
            devMode.m_size = sizeof(devMode);
            if (EnumDisplaySettingsA(0, nMode, &devMode) == 0)
                nMode = -1;
        }
    }
    SetVideoMode(nMode, VIDEO_SCREEN_FULLSCREEN);
}
#endif

int CClientExoAppInternal::GetNearestVideoMode(unsigned int a, unsigned int b, unsigned char c, unsigned int d)
{
    if (a == 0)
        a = g_nScreenWidth;
    if (b == 0)
        b = g_nScreenHeight;
    if (c == 0)
        c = g_nBitsPerPixel;
    if (d == 0)
        d = g_nDesiredFullScreenRefreshRate;
    int nMode = CAuroraInterface::GetNearestVideoMode(a, b, c, d);
#if KOTOR1  // the enumerated mode must also have the requested depth
    _devicemodeA devMode;
    memset(&devMode, 0, sizeof(devMode));
    devMode.m_size = sizeof(devMode);
    if (EnumDisplaySettingsA(0, nMode, &devMode) == 0)
        return -1;
    return devMode.m_bitsPerPel == c ? nMode : -1;
#else
    PortDevMode devMode = PortDevMode();
    devMode.m_size = sizeof(devMode);
    if (EnumDisplaySettingsA(0, nMode, &devMode))
        return nMode;
    return -1;
#endif
}

int CClientExoAppInternal::GetVideoModeSpec(unsigned int a, unsigned int* b, unsigned int* c, unsigned char* d, unsigned int* e)
{
#if KOTOR1
    _devicemodeA devMode;
    memset(&devMode, 0, sizeof(devMode));
    devMode.m_size = sizeof(devMode);
#else
    PortDevMode devMode;
    memset(&devMode, 0, sizeof(devMode));
    devMode.m_size = sizeof(devMode);
#endif
    if (EnumDisplaySettingsA(0, a, &devMode))
    {
        if (b)
            *b = devMode.m_pelsWidth;
        if (c)
            *c = devMode.m_pelsHeight;
        if (d)
            *d = static_cast<unsigned char>(devMode.m_bitsPerPel);
        if (e)
            *e = devMode.m_displayFrequency;
        return 1;
    }
    return 0;
}

#include "GraphicsSettings.h"

int CClientExoAppInternal::ReadVideoModeSettings(unsigned int* pWidth, unsigned int* pHeight,
                                                unsigned char* pRefreshRate,
                                                unsigned int* pRefresh)
{
#if KOTOR1  // the SDL display mode, 32 bpp, 60 Hz when SDL reports no rate
    *pWidth = g_SDL_mode.w;
    *pHeight = g_SDL_mode.h;
    *pRefreshRate = 32;
    *pRefresh = g_SDL_mode.refresh_rate ? g_SDL_mode.refresh_rate : 60;
    return 1;
#else
    CExoString sValue;
    CExoIni ini;

    int nResult = ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                                   CExoString("Graphics Options"), CExoString("Width"));
    *pWidth = sValue.AsINT();

    ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                     CExoString("Graphics Options"), CExoString("Height"));
    *pHeight = sValue.AsINT();
    *pRefreshRate = 32;

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                         CExoString("Graphics Options"), CExoString("RefreshRate"))) {
        // inlined CExoString::AsINT()
        if (sValue.m_sString == 0)
            g_nDesiredFullScreenRefreshRate = 0;
        else
            g_nDesiredFullScreenRefreshRate = atoi(sValue.m_sString);
    }
    *pRefresh = g_nDesiredFullScreenRefreshRate;

    if (*pWidth == 0 || *pHeight == 0
        || !CAuroraInterface::IsSupportedResolution(*pWidth, *pHeight)) {
        if (GraphicsSettings::GetQualityLevel() == 0) {
            *pWidth = 1024;
            *pHeight = 768;
        } else {
            unsigned int nBestWidth = 0, nBestHeight = 0, nBestRefresh = 0;
            PortDevMode devMode = PortDevMode();
            devMode.m_size = sizeof(devMode);
            unsigned int i = 0;
            while (EnumDisplaySettingsA(0, i, &devMode)) {
                i += 1;
                if (CAuroraInterface::IsWithinDesktopResolution(devMode.m_pelsWidth,
                                                                devMode.m_pelsHeight)
                    && devMode.m_pelsWidth >= nBestWidth
                    && devMode.m_pelsHeight >= nBestHeight) {
                    nBestRefresh =
                        (devMode.m_pelsHeight == nBestHeight
                         && devMode.m_pelsWidth == nBestWidth)
                            ? (devMode.m_displayFrequency > nBestRefresh
                                   ? devMode.m_displayFrequency
                                   : nBestRefresh)
                            : devMode.m_displayFrequency;
                    nBestWidth = devMode.m_pelsWidth;
                    nBestHeight = devMode.m_pelsHeight;
                }
            }
            *pWidth = nBestWidth;
            *pHeight = nBestHeight;
            *pRefresh = nBestRefresh;
        }
        nResult = 1;
    }

    if (!CAuroraInterface::IsWithinDesktopResolution(*pWidth, *pHeight)
        || !CAuroraInterface::IsValidResolution(*pWidth, *pHeight)) {
        *pWidth = 1024;
        *pHeight = 768;
    }
    if (*pRefreshRate != 32)
        *pRefreshRate = 32;
    if (*pRefresh <= 59 && *pWidth <= 1999)
        *pRefresh = 60;

    return nResult;
#endif
}

#if KOTOR1
int CClientExoAppInternal::SetVideoMode(unsigned int nMode, unsigned char bFullscreen)
{
    K2_UNUSED(nMode); K2_UNUSED(bFullscreen);
    int nWidth = g_SDL_mode.w;
    int nHeight = g_SDL_mode.h;
    g_nScreenWidth = nWidth;
    g_nScreenHeight = nHeight;
    g_nBitsPerPixel = 32;
    ReInitAurora(nWidth, nHeight, 32, 1, 1);
    if (m_pSWGuiManager)
    {
        m_pSWGuiManager->SetSize(g_nScreenWidth, g_nScreenHeight);
        if (m_pMainMenuPanel && m_pSWGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pMainMenuPanel)))
            m_pMainMenuPanel->LoadFromLayout(m_pSWGuiManager);
        if (m_pLoadScreenObj)
        {
            delete m_pLoadScreenObj;
            m_pLoadScreenObj = new CSWGuiLoadScreen(m_pSWGuiManager);
        }
        if (m_pInGameGui)
            m_pInGameGui->ResetInterfaceForSize();
    }
    return 1;
}
#else
int CClientExoAppInternal::SetVideoMode(unsigned int nMode, unsigned char bFullscreen)
{
    if (static_cast<unsigned int>(m_currentVideoMode) == nMode)
        return 1;

    PortDevMode devMode = PortDevMode();
    devMode.m_size = sizeof(devMode);
    if (!EnumDisplaySettingsA(0, nMode, &devMode))
        return 0;

    int nWidth = devMode.m_pelsWidth;
    int nHeight = devMode.m_pelsHeight;
    g_nScreenWidth = nWidth;
    g_nScreenHeight = nHeight;
    g_nBitsPerPixel = static_cast<unsigned char>(devMode.m_bitsPerPel);

    int nFullscreen;
    switch (bFullscreen)
    {
        case VIDEO_SCREEN_WINDOWED: nFullscreen = 0; break;
        case VIDEO_SCREEN_FULLSCREEN: nFullscreen = 1; break;
        default: nFullscreen = g_pAurora->FullScreen();
    }

    nWidth = g_nScreenWidth;
    nHeight = g_nScreenHeight;

    if (g_nReInitializing == 1)
        g_pAurora->RestoreDesktopDisplayMode();

    g_nScreenWidth = nWidth;
    g_nScreenHeight = nHeight;
    g_bAllowWindowedMode = 1;
    ReInitAurora(nWidth, nHeight, g_nBitsPerPixel, nFullscreen, 1);

    m_currentVideoMode = nMode;

    if (m_pSWGuiManager)
    {
        m_pSWGuiManager->SetSize(g_nScreenWidth, g_nScreenHeight);
        if (m_pMainMenuPanel && m_pSWGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pMainMenuPanel)))
            m_pMainMenuPanel->LoadFromLayout(m_pSWGuiManager);

        if (m_pLoadScreenObj)
        {
            delete m_pLoadScreenObj;
            m_pLoadScreenObj = new CSWGuiLoadScreen(m_pSWGuiManager);
        }

        if (m_pInGameGui)
            m_pInGameGui->ResetInterfaceForSize();
    }

    messagepump();
    messagepump();
    messagepump();
    messagepump();
    messagepump();

    return 1;
}
#endif

#if KOTOR1
int CClientExoAppInternal::IsValidResolution(unsigned int nWidth, unsigned int nHeight)
{
#ifdef K2_QOL_FULLSCREEN
    // Desktop: the modes are the host's window sizes, all of them usable.
    K2_UNUSED(nWidth); K2_UNUSED(nHeight);
    return 1;
#else
    if (nWidth == 800 && nHeight == 600)
        return 1;
    if (nWidth == 1024 && nHeight == 768)
        return 1;
    if (nWidth == 1280 && (nHeight == 960 || nHeight == 1024))
        return 1;
    return nWidth == 1600 && nHeight == 1200;
#endif
}
#endif

void CClientExoAppInternal::InitializeSoundOptions()
{
#if KOTOR1
    CExoIni ini;
    CExoString sTemp;
    CExoString sValue;

    int nInit = 0;
    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                         CExoString("Sound Init")))
        nInit = sValue.AsINT();
    sValue.Format(const_cast<char *>("%d"), 1);
    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                      CExoString("Sound Init"));

    unsigned char n2DVoices = 0;
    unsigned char n3DVoices = 0;
    int nEAXLevel = 0;
    int nForceSoftware = 0;
    CClientOptions* pOptions;

    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                         CExoString("Number 2D Voices")))
        n2DVoices = static_cast<unsigned char>(sValue.AsINT());
    else
        n2DVoices = 24;

    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                         CExoString("Number 3D Voices")))
        n3DVoices = static_cast<unsigned char>(sValue.AsINT());
    else
        n3DVoices = 16;

    if (nInit != 0) {
        nEAXLevel = 0;
        nForceSoftware = 1;
    } else {
        if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"),
                             CExoString("Sound Options"), CExoString("EAX")))
            nEAXLevel = sValue.AsINT();
        else
            nEAXLevel = 3;
        if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"),
                             CExoString("Sound Options"), CExoString("Force Software"))) {
            nForceSoftware = sValue.AsINT();
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bForceSoftware = nForceSoftware;
        }
    }

    g_pExoSound = new CExoSound(n2DVoices, n3DVoices, nEAXLevel, !nForceSoftware);

    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                         CExoString("Environment Effects Nonstreaming")))
        g_pExoSound->EnableNonStreamingEnvironmentEffects(sValue.AsINT());

    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                         CExoString("Environment Effects Streaming")))
        g_pExoSound->EnableStreamingEnvironmentEffects(sValue.AsINT());

    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                         CExoString("2D3D Bias")))
        g_pExoSound->Set2D3DBias(sValue.AsFLOAT());

    sValue.Format(const_cast<char *>("%d"), g_pExoSound->GetEAX());
    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                      CExoString("EAX"));
    pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    pOptions->m_nEAXLevel = static_cast<unsigned char>(g_pExoSound->GetEAX());

    sValue.Format(const_cast<char *>("%d"), nForceSoftware);
    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                      CExoString("Force Software"));

    sValue.Format(const_cast<char *>("%.2f"), static_cast<double>(g_pExoSound->Get2D3DBias()));
    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                      CExoString("2D3D Bias"));

    sValue.Format(const_cast<char *>("%d"), static_cast<unsigned char>(g_pExoSound->GetNumber2DVoices()));
    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                      CExoString("Number 2D Voices"));

    sValue.Format(const_cast<char *>("%d"), static_cast<unsigned char>(g_pExoSound->GetNumber3DVoices()));
    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                      CExoString("Number 3D Voices"));

    sValue.Format(const_cast<char *>("%d"), 0);
    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Sound Options"),
                      CExoString("Sound Init"));
#else
    CExoIni ini;
    CExoString sTemp;
    CExoString sValue;

    int nInit = 0;
    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                         CExoString("Sound Init")))
        nInit = sValue.AsINT();
    sValue.Format(const_cast<char *>("%d"), 1);
    ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                      CExoString("Sound Init"));

    unsigned char n2DVoices = 0;
    unsigned char n3DVoices = 0;
    int nEAXLevel = 0;
    int nForceSoftware = 0;
    CClientOptions* pOptions;

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                         CExoString("Number 2D Voices")))
        n2DVoices = static_cast<unsigned char>(sValue.AsINT());
    else
        n2DVoices = 64;

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                         CExoString("Number 3D Voices")))
        n3DVoices = static_cast<unsigned char>(sValue.AsINT());
    else
        n3DVoices = 64;

    if (nInit != 0) {
        nEAXLevel = 0;
        nForceSoftware = 1;
    } else {
        if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                             CExoString("Sound Options"), CExoString("EAX")))
            nEAXLevel = sValue.AsINT();
        else
            nEAXLevel = 3;
        nForceSoftware = 0;
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bForceSoftware = 0;
    }

    g_pExoSound = new CExoSound(n2DVoices, n3DVoices, nEAXLevel, !nForceSoftware);

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                         CExoString("Environment Effects Nonstreaming")))
        g_pExoSound->EnableNonStreamingEnvironmentEffects(
            sValue.AsINT());

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                         CExoString("Environment Effects Streaming")))
        g_pExoSound->EnableStreamingEnvironmentEffects(
            sValue.AsINT());

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                         CExoString("2D3D Bias")))
        g_pExoSound->Set2D3DBias(sValue.AsFLOAT());

    sValue.Format(const_cast<char *>("%d"), g_pExoSound->GetEAX());
    ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                      CExoString("EAX"));
    pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    pOptions->m_nEAXLevel = static_cast<unsigned char>(g_pExoSound->GetEAX());

    sValue.Format(const_cast<char *>("%d"), nForceSoftware);
    ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                      CExoString("Force Software"));

    sValue.Format(const_cast<char *>("%.2f"), static_cast<double>(g_pExoSound->Get2D3DBias()));
    ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                      CExoString("2D3D Bias"));

    sValue.Format(const_cast<char *>("%d"), static_cast<unsigned char>(g_pExoSound->GetNumber2DVoices()));
    ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                      CExoString("Number 2D Voices"));

    sValue.Format(const_cast<char *>("%d"), static_cast<unsigned char>(g_pExoSound->GetNumber3DVoices()));
    ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                      CExoString("Number 3D Voices"));

    sValue.Format(const_cast<char *>("%d"), 0);
    ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Sound Options"),
                      CExoString("Sound Init"));
#endif
}

void CClientExoAppInternal::StopSoundPlayingInDialog()
{
    unsigned long long nSpeakerId = m_pInGameGui->m_oidDialogSoundSpeaker;
#if KOTOR1  // the member lookup (SUCCESS-gated)
    CGameObject* pGameObject = GetGameObject(nSpeakerId);
#else
    CGameObject* pGameObject = 0;
    m_pGameObjectArray->GetGameObject(nSpeakerId, &pGameObject);
#endif
    if (pGameObject)
    {
        CSWCObject* pObject = pGameObject->AsSWCObject();
        if (pObject)
        {
            if (pObject->AsSWCCreature())
                pObject->AsSWCCreature()->StopLipSync();
            if (m_pInGameGui->m_pDialogSound)
                m_pInGameGui->m_pDialogSound->Stop(0);
        }
    }
}

void CClientExoAppInternal::StopMenuMusic()
{
    CExoStreamingSoundSource* pMusic = m_pMenuMusic;
    if (pMusic)
    {
        unsigned int nStart = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
        pMusic->FadeAndStop(500);
        do {
            g_pExoSound->Render();
        } while (static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000) - nStart < 500);
        if (m_pMenuMusic) {
            delete m_pMenuMusic;
            m_pMenuMusic = 0;
        }
    }
}

void CClientExoAppInternal::TogglePartyFollow()
{
    CExoString sScript("k_sup_solo");
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    pPartyTable->SetSoloMode(g_pAppManager->m_pServerExoApp->GetPartyTable()->m_soloMode == 0, 1);
    g_pVirtualMachine->RunScript(&sScript, OBJECT_INVALID, 0);
}

void CClientExoAppInternal::StopLoadMusic()
{
    if (m_pLoadMusic != 0) {
        m_pLoadMusic->Stop(0);
        if (m_pLoadMusic != 0) {
            delete m_pLoadMusic;
        }
        m_pLoadMusic = 0;
    }
}

CExoStreamingSoundSource* CClientExoAppInternal::AquireLoadMusic(const CResRef& r)
{
    CExoStreamingSoundSource* pSound = 0;
    if (m_pLoadMusic)
    {
        CExoString sName;
        r.CopyToString(sName);
        if (sName.CompareNoCase(m_loadMusic) != 0)
        {
            pSound = m_pLoadMusic;
            m_pLoadMusic = 0;
        }
    }
    return pSound;
}

extern CExoSound* g_pExoSound;

// Console command.
void soundrolloff(char* pszValue)
{
    float fRollOff = static_cast<float>(atof(pszValue));
    g_pExoSound->SetRollOffFactor(fRollOff);
}

// Console command.
char* SetFramesPerSecond(char* pszRate)
{
    int nRate = atoi(pszRate);

    if (nRate != 0)
    {
        g_pAppManager->m_pClientExoApp->GetWorldTimer()->SetFixedSnapshotRate(1, nRate);
        g_pAppManager->m_pClientExoApp->GetPauseTimer()->SetFixedSnapshotRate(1, nRate);
        g_pAppManager->m_pClientExoApp->GetAnimationTimer()->SetFixedSnapshotRate(1, nRate);
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->SetFixedSnapshotRate(1, nRate);
        g_pAppManager->m_pServerExoApp->GetPauseTimer()->SetFixedSnapshotRate(1, nRate);
        return const_cast<char*>("Frame rate set.");
    }

    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SetFixedSnapshotRate(0, 0);
    g_pAppManager->m_pClientExoApp->GetPauseTimer()->SetFixedSnapshotRate(0, 0);
    g_pAppManager->m_pClientExoApp->GetAnimationTimer()->SetFixedSnapshotRate(0, 0);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SetFixedSnapshotRate(0, 0);
    g_pAppManager->m_pServerExoApp->GetPauseTimer()->SetFixedSnapshotRate(0, 0);
    return const_cast<char*>("Fixed frame rate off.");
}

int g_nSetMovieFrameRate;

// Console command.
char* SetMovieFrameRate(char* pszRate)
{
    int nRate = atoi(pszRate);
    char* pszMessage = const_cast<char*>(nRate != 0 ? "Frame rate will be set after the next module load."
                                          : "Frame rate returned to normal.");
    g_nSetMovieFrameRate = nRate;
    return pszMessage;
}

// Console command.
char* MarkLog(char* pszArg)
{
    K2_UNUSED(pszArg);
    return const_cast<char*>("Log has been duly noted!");
}

// Console command.
void rendertilepathnodes(char* pszArg)
{
    int bRender = atoi(pszArg) != 0;
    g_pAppManager->m_pClientExoApp->RenderTilePathNodes(bRender);
}

// Console command.
void GameCameraHitCheck(int a, int b, Vector* pvHit, CHitInfo* pInfo)
{
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();

    if (pInfo)
        pInfo->m_bHitBoundingBoxes = 1;

    if (pCamera)
        pCamera->HitCheck(a, b, pvHit, pInfo);
}

// Console command.
long long GetGameHighResTime()
{
    return g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetSnapshotTime();
}

void CSWCPlayer::SetPrimaryPCPortraitId(unsigned short nPortraitId) {
    CExoString sPrefix;
    CExoString sBaseResRef;
    if (nPortraitId != 0xffff) {
        sPrefix = "po_";
        g_pRules->m_p2DArrays->m_pPortrait->GetCExoStringEntry(nPortraitId,
                                                              CExoString("BaseResRef"),
                                                              &sBaseResRef);
        sBaseResRef = sPrefix + sBaseResRef;
        SetPrimaryPCPortraitResRef(CResRef(sBaseResRef.CStr()));
    }
    m_nPortraitId = nPortraitId;
}

CClientExoAppInternal::CClientExoAppInternal()
{
    m_bPazaakIsRunning = 0;
    m_pazaakResultB = 0;
    m_pazaakResultA = 0;
    m_nPazaakOpponentDeck = 0;
    m_aReserved1[0] = 0;
    m_aReserved1[1] = 0;
    m_aReserved1[2] = 0;
    m_aReserved1[3] = 0;
    m_aReserved1[4] = 0;
    m_aReserved1[5] = 0;
    m_bStartMenuMusicOnMovieEnd = 1;
    m_viewTriggers = 1;
    m_mouseOverPos = Vector(0.0f, 0.0f, 0.0f);
#if KOTOR1
    m_bAllowHighMonitorFrequency = 0;
#endif
    m_bStartSoundsAfterLoadBar = 0;
    m_bClientInitialized = 0;
    m_debugMode = 0;
    m_nReserved12 = 0;
    m_pModule = 0;
    m_pSWCMessage = 0;
    m_pLoadGameLoadBar = 0;
    m_loadBarState = 0;
    m_pServerInfo = 0;
    m_inputClass = INPUTCLASS_GAME;
    m_moveTypeMode = 0;
    m_bInputDisabled = 0;
    m_bTrapSighted = 0;
    m_actionKeyCount = 0;
    m_hitCheckMaterialMask = 0;
    m_playerCreatureId = OBJECT_INVALID;
    m_sceneCreatureId = OBJECT_INVALID;
    m_pLoadModulePanel = 0;
    m_combatMode = 0;
    m_bEnemySighted = 0;
    m_pWorldJournal = 0;
    m_pQuestJournal = 0;
    m_nReserved2 = 0;
    m_nReserved1 = 0;
    m_pPlayModulePanel = 0;
    m_nReserved3 = 0;
    m_pCharGenPanel = 0;
    m_nReserved5 = 0;
    m_pLoadScreenObj = 0;
    m_pSWGuiManager = 0;
    m_pMainMenuPanel = 0;
    m_pLegalScreen = 0;
    m_pStartScreen = 0;
    m_keyboardModalFlags[0] = 0;
    m_keyboardModalFlags[1] = 0;
    m_keyboardModalFlags[2] = 0;
    m_keyboardModalFlags[3] = 0;
    m_keyboardModalFlags[4] = 0;
    m_keyboardModalFlags[5] = 0;
    m_keyboardModalFlags[6] = 0;
    m_nScrollSpeedX = 0;
    m_nScrollSpeedY = 0;
    m_bClientImeUI = 0;
    m_pImeUI = 0;
    m_bScreenShotPending = 0;
    m_pCaptureEventFile = 0;
    m_captureEvents = 0;
    m_captureEventPlayBack = 0;
    m_captureEventPlayBackRate = 30;
    m_currentVideoMode = -1;
    m_currentTexturePack = 0xff;
    m_nLastDriveTime = 0;
    m_drivingTimerReset = 0;
    m_nDriveKeyPressTime = 0;
    m_nReserved8 = 0;
    m_nReserved9 = 0;
    m_pInGameGui = 0;
    m_fQueuedAnimDelay = 0.0f;
    m_nReserved10 = 0;
    m_nReserved11 = 0;
    m_characterFileSize = 0;
    m_characterFileSubDir = 0;
    m_pCharacterFileBuffer = 0;
    m_pauseFlags = 0;
    m_containerOpened = CGameObjectArray::INVALIDOBJID;
    m_oidReserved1 = CGameObjectArray::INVALIDOBJID;
    m_pSoundSetList = 0;
    m_pItemDropSounds = 0;
    m_pauseExclusionList = new CExoLinkedList<unsigned long long>;
    m_pMoviePlayer = 0;
    m_bShowMouseAtMovieEnd = 0;
    m_nReserved4 = 0;
    m_bApplicationIdsMatch = 0;
    m_inStartNewModule = 0;
    m_bSendAreaLoadedAfterMovie = 0;
    m_fCameraCollision = 0;
    m_nReserved6 = 0;
    m_nReserved7 = 0;
    m_lastDialogSpeaker = CGameObjectArray::INVALIDOBJID;
    m_pExclusionTimer = 0;
    m_pWorldTimer = 0;
    m_pAnimationTimer = 0;
    m_pPauseTimer = 0;
    m_bEndGamePending = 0;
    m_bDisplayEndGameGui = 1;
    m_pMenuMusic = 0;
    m_pLoadMusic = 0;
    m_bLoadGameLoadBarActive = 0;
    m_bRenderTilePathNodes = 0;
    m_pWalkToRingVFX = 0;
    m_nReserved13 = 0;
    m_bQuitClicked = 0;
    m_bWMCLOSEQuery = 0;
    m_bReachedMainMenu = 0;
    m_bShowWalkToRingVFX = 0;
    m_nExportRequestState = 0;
    m_nLoadGameError = 0;
    m_bOptionsPanelOpen = 0;
    m_pMGOArray = new CSWMiniGameObjectArray;
    m_pParty = new CSWParty;
    m_pPlayerControl = 0;
    m_bAreaNotReadyToDisplay = 1;
    m_fDriveInterruptDelay = -1.0f;
    m_controlType = 1;
    m_nGameOverReason = 0;
    m_bGameOver = 1;
    m_deathSeqLastMs = static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    m_fDeathSeqAccum = 0;
    m_fDeathSeqFade = 0;
    m_lastPartyMemberTempKilled = OBJECT_INVALID;
    m_lastClickedOnTarget = OBJECT_INVALID;
    m_lastTarget = OBJECT_INVALID;
    m_bDialogSent = 0;
    m_bDialogModelsSent = 0;
    m_bCanSendDialog = 0;
    m_nCanSendDialogTime = 0;
    m_loadScreenName = "";
    m_tutorialObjB = OBJECT_INVALID;
    m_tutorialObjA = OBJECT_INVALID;
    m_nPendingAutoPauseReason = PAUSE_MODE_NO_PENDING;
    m_tutorialReason = TUTORIAL_NONE;
    m_futureLeader = OBJECT_INVALID;
    m_oidReserved2 = OBJECT_INVALID;
    m_oidCreatureCache = OBJECT_INVALID;
    m_bTogglePauseAtEndOfFrame = 0;
    m_bAutoPaused = 0;
    m_bRunScriptSet = 0;
    m_bGuiGameSlow = 0;
    m_bLoadingSaveGame = 0;
    m_combatPauseRequest = 0;
    m_fEnemyAutoPauseDelay = 0;
    m_fEnemyAutoPauseTimer = 0;
    m_fPartyDownPauseCooldown = 0.0f;
    m_fTrapSightedTimer = 0;
    m_fEnemySightedTimer = 0;
    m_tutorialTimerSpan = 0;
    m_pCreatureCache = 0;
    m_mouseHideMask = 0;
    m_bLoadScreenNameSet = 0;
    m_bLoadFromIFO = 0;
    m_fTargetHiddenTime = 0.0f;
    m_fCombatStreak = 0.0f;
    m_fStatusSummaryDelay = 0.0f;
    m_fEndGameDelay = 0;
    m_restoreMouseX = 0;
    m_restoreMouseY = 0;
    m_nReserved15 = 0;
    m_nReserved14 = 0;
    m_videoEffectType = VIDEO_EFFECT_NONE;
    m_fMouseShowTimer = -1.0f;
    m_attackMashTutorialTimer = 0;
    memset(m_loadStep, 0, sizeof(unsigned int) + 1);
    memset(m_liveContent, 0, sizeof(m_liveContent));
    m_gamePlayHint = 0;
    m_loadScreenHintUpdateTimer = 0;
    m_nLoadScreenHintLastTime = 0;
#if KOTOR1
    m_storyHint[0] = 0;
#else
    memset(m_storyHint, 0, sizeof(m_storyHint) - 1);
#endif
    m_bMainMenuReset = 0;
    m_fTargetChanging = 0;
    m_bLastTargetChanged = 0;
    m_flags524_bit1 = 0;
    m_pClientOptions = new CClientOptions;
    m_moduleResourceFlags = 0;
    m_nReserved16 = 0;
    m_nReserved17 = 0;
    m_pEvilEndingMusic = 0;
    m_objectMouseOver = OBJECT_INVALID;
    m_bCharacterChangeInProgress = 0;
    m_creditSequence = 0;
    m_bEvilEndingHackInProgress = 0;
#if !KOTOR1
    m_superModelSet = 1;
#endif
    m_bSoundPausedByCombat = 0;
}

CClientExoAppInternal::~CClientExoAppInternal()
{
    delete m_pParty;
    *reinterpret_cast<void* volatile*>(&m_pParty) = 0;
    delete m_pauseExclusionList;
    delete m_pMGOArray;
    *static_cast<CSWMiniGameObjectArray* volatile*>(&m_pMGOArray) = 0;
    delete m_pEvilEndingMusic;
    *reinterpret_cast<void* volatile*>(&m_pEvilEndingMusic) = 0;
}

int CClientExoAppInternal::GetClientMeleeAnimation(unsigned long long nObjId1, unsigned long long nObjId2,
                                                  unsigned short nAttackIndex, int nArg4, int nArg5,
                                                  int nArg6)
{
    CSWCCreature* pCreature1 = GetCreatureByGameObjectID(
        (nObjId1 == OBJECT_INVALID) ? nObjId1 : (nObjId1 | CLIENT_OBJECT_ID_FLAG));
    CSWCCreature* pCreature2 = GetCreatureByGameObjectID(
        (nObjId2 == OBJECT_INVALID) ? nObjId2 : (nObjId2 | CLIENT_OBJECT_ID_FLAG));

    if (pCreature1 == 0)
        return 0;
    if (nArg5 == 0) {
        CSWCAnimBase* pBase = pCreature1->GetAnimationBase();
        unsigned char cBase = pBase->m_nWeaponClass;
        int nCombat = pCreature1->InCreatureCombat(pCreature2);
        pCreature1->ResolveAttackIndex(nAttackIndex, nCombat, -1);
        return pCreature1->UpdateMeleeAttackData(nAttackIndex, nArg4, nCombat, cBase);
    }
    pCreature1->m_nForcedAttackAnimation = nArg6;
    return nArg6;
}

#if !KOTOR1
void CClientExoAppInternal::UnloadSuperModelSet()
{
    if (m_superModelSet == 1)
        g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("SUPERMODELS:smseta"));
    else if (m_superModelSet == 2)
        g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("SUPERMODELS:smsetb"));
    else if (m_superModelSet == 3)
        g_pExoResMan->RemoveEncapsulatedResourceFile(CExoString("SUPERMODELS:smsetc"));
}
#endif

void CClientExoAppInternal::AttachPlayerGameObjectToArea()
{
    CSWCCreature* pCreature;
    if (m_oidCreatureCache == m_playerCreatureId) {
        pCreature = static_cast<CSWCCreature*>(m_pCreatureCache);
        if (pCreature == 0)
            return;
    } else {
        pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_playerCreatureId);
        m_pCreatureCache = pCreature;
        m_oidCreatureCache = m_playerCreatureId;
        if (pCreature == 0)
            return;
    }

    pCreature->AttachToScene(m_pModule->m_pArea->m_pScene);

    if (enablemodelrequest != 0) {
        if (pCreature->m_nAppearanceRequestTimer != 0) {
            pCreature->m_bCameraReattachPending = 1;
            pCreature->m_bCameraFindClearView = 1;
        }
    } else {
        m_pModule->SetCameraFollow(pCreature->m_objectId, 1);
    }

    if (m_pPlayerControl != 0) {
        m_pPlayerControl->SetPlayer(pCreature->m_objectId);
        m_pPlayerControl->SetCamera(m_pModule->m_pCamera);
    } else {
        m_pPlayerControl = new CSWPlayerControlCamRelative(pCreature->m_objectId,
                                                          m_pModule->m_pCamera);
    }
}

void CClientExoAppInternal::SetCharacterFile(unsigned int nLength, unsigned char* pData, int nSubDir)
{
    if (m_pCharacterFileBuffer != 0) {
        delete[] m_pCharacterFileBuffer;
        m_pCharacterFileBuffer = 0;
    }
    if (pData != 0) {
        m_pCharacterFileBuffer = new unsigned char[nLength + 1];
        memcpy(m_pCharacterFileBuffer, pData, nLength);  // nLength is a byte count, not an element count  // idiom-exempt: nLength IS the byte count (raw unsigned char buffer)
        m_pCharacterFileBuffer[nLength] = 0;
    }
    m_characterFileSize = nLength;
    m_characterFileSubDir = nSubDir;
}

void CClientExoAppInternal::AddObjectModelDirectlyToIgnoreList(unsigned long long id,
                                                              List<CAurObject*>* pList)
{
#if KOTOR1  // the member lookup (SUCCESS-gated)
    CGameObject* pObj = GetGameObject(id);
#else
    CGameObject* pObj = 0;
    m_pGameObjectArray->GetGameObject(id, &pObj);
#endif
    if (pObj != 0) {
        if (pObj->m_objectType == 5)
            AddCreatureModelDirectlyToIgnoreList(id, pList);
        else if (pObj->m_objectType == 6)
            AddItemModelDirectlyToIgnoreList(id, pList);
        else
            AddSimpleObjectModelDirectlyToIgnoreList(id, pList);
    }
}

void CClientExoAppInternal::AddSimpleObjectModelDirectlyToIgnoreList(unsigned long long id,
                                                                    List<CAurObject*>* pList)
{
    CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(id);
    if (pObj != 0) {
        CSWCObject* pSwcObject = pObj->AsSWCObject();
        if (pSwcObject != 0) {
            Gob* pModel = pSwcObject->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            if (pModel != 0)
                pList->Add(static_cast<CAurObject*>(pModel));  // stored as CAurObject*
        }
    }
}

int CClientExoAppInternal::GetClientRangedAnimation(unsigned long long nObjId, int nAttackIndex,
                                                    int nArg3, int nArg4)
{
    CSWCCreature* pCreature = GetCreatureByGameObjectID(
        (nObjId == OBJECT_INVALID) ? nObjId : (nObjId | CLIENT_OBJECT_ID_FLAG));
    if (pCreature == 0)
        return 0;
    if (nArg3 != 0) {
        pCreature->m_nForcedAttackAnimation = nArg4;
        return nArg4;
    }
    return pCreature->UpdateRangedAttackData(nAttackIndex,
                                             pCreature->GetAnimationBase()->m_nWeaponClass);
}

void CClientExoAppInternal::SetLoadScreenByModuleName(const CExoString& a)
{
    CExoString sLoading;
    if (a.GetLength())
    {
        if (g_pRules->m_p2DArrays->m_pAreaTransition->GetCExoStringEntry(a, CExoString("BMPResRef"), &sLoading) == 0)
        {
            sLoading.Format(const_cast<char*>("load_%s"), a.CStr());
            if (g_pExoResMan->Exists(CResRef(sLoading), 3, static_cast<unsigned int*>(0)) == 0)
            {
                if (g_pExoResMan->Exists(CResRef(sLoading), RESTYPE_TPC, static_cast<unsigned int*>(0)) == 0)
                {
                    g_pRules->m_p2DArrays->m_pAreaTransition->GetCExoStringEntry(CExoString("DEFAULT"), CExoString("BMPResRef"), &sLoading);
                }
            }
        }
        if (m_loadScreenName.CompareNoCase(sLoading) == 0)
        {
            m_loadScreenName = sLoading;
            m_bLoadScreenNameSet = 1;
        }
    }
    else
    {
        m_loadScreenName = "";
        m_bLoadScreenNameSet = 1;
    }
}

CExoString CClientExoAppInternal::GetLoadMusicByModuleName(const CExoString& a)
{
    CExoString sRet;
    C2DA* pLoadscreens;
    if (a.GetLength())
    {
        pLoadscreens = g_pRules->m_p2DArrays->m_pAreaTransition;
        if (pLoadscreens->GetCExoStringEntry(a, CExoString("MusicResRef"), &sRet) == 0)
        {
            sRet.Format(const_cast<char*>("load_%s"), a.CStr());
            if (g_pExoResMan->Exists(CResRef(sRet), 8, static_cast<unsigned int*>(0)) == 0)
            {
                pLoadscreens = g_pRules->m_p2DArrays->m_pAreaTransition;
                CExoString sRow("DEFAULT");
                CExoString sColumn2("MusicResRef");
                pLoadscreens->GetCExoStringEntry(sRow, sColumn2, &sRet);
            }
        }
    }
    else
    {
        sRet = "";
    }
    return sRet;
}

void CClientExoAppInternal::UpdateLoadingScreenHint()
{
#if KOTOR1  // needs a server; no first-tick guard; always the ("", 2) re-roll
    if (g_pAppManager->m_pServerExoApp == 0 || g_pAppManager->m_pServerExoApp->m_pInternal == 0)
        return;
    if (m_pLoadScreenObj && m_pLoadScreenObj->m_nHintStrRef)
    {
        unsigned int nNow = g_pExoBase->m_pTimers->GetLowResolutionTimer();
        unsigned int nLast = m_nLoadScreenHintLastTime;
        m_nLoadScreenHintLastTime = nNow;
        m_loadScreenHintUpdateTimer += (nNow - nLast);
        if (m_loadScreenHintUpdateTimer >> 4 < 625)
            return;
        m_loadScreenHintUpdateTimer = 0;
        CExoString sDefault("");
        uint64_t nHint = GetNextLoadScreenHintSTRREF(sDefault, 2);
        if (m_pLoadScreenObj)
            m_pLoadScreenObj->SetHint(nHint);
    }
#else
    if (m_pLoadScreenObj && m_pLoadScreenObj->m_nHintStrRef)
    {
        unsigned int nNow = g_pExoBase->m_pTimers->GetLowResolutionTimer();
        unsigned int nLast = m_nLoadScreenHintLastTime;
        m_nLoadScreenHintLastTime = nNow;
        if (nLast == 0)
            nLast = nNow;
        m_loadScreenHintUpdateTimer += (nNow - nLast);
        if (m_loadScreenHintUpdateTimer >> 4 < 625)
            return;
        m_loadScreenHintUpdateTimer = 0;
        if (g_pAppManager->m_pServerExoApp == 0)
            goto lblDefault;
        {
            CExoString sModule = g_pAppManager->m_pServerExoApp->GetModuleName();
            if (sModule.GetLength() < 1)
                goto lblDefault;
        }
        {
            CExoString sStep = g_pAppManager->m_pServerExoApp->GetModuleName();
            uint64_t nHint = GetNextLoadScreenHintSTRREF(sStep, 2);
            if (m_pLoadScreenObj)
                m_pLoadScreenObj->SetHint(nHint);
            goto lblDone;
        }
    lblDefault:
        {
            CExoString sDefault("");
            uint64_t nHint = GetNextLoadScreenHintSTRREF(sDefault, 1);
            if (m_pLoadScreenObj)
                m_pLoadScreenObj->SetHint(nHint);
            goto lblDone;
        }
    lblDone: ;
    }
#endif
}

#if KOTOR1
uint64_t CClientExoAppInternal::GetNextLoadScreenHintSTRREF(const CExoString& sModuleName, unsigned char nModuleType)
{
    int nResult = 0;
    C2DA* pTable = new C2DA(CResRef("loadscreenhints"), 0);
    if (pTable->Load2DArray() == 0)
    {
        delete pTable;
        return 0;
    }
    uint64_t nRet;
    int bStory;
    if (nModuleType == 1)
        bStory = 0;
    else if (nModuleType == 2)
        bStory = 1;
    else
    {
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        if (pServer && pServer->m_pInternal)
            bStory = pServer->GetModulePreviouslyVisited(sModuleName) != 0;
        else
            bStory = 1;
    }
    if (bStory)
    {
        int nIndex = m_storyHint[0];
        int nLookup;
        {
            CExoString sColumn("StoryHint");
            nLookup = pTable->GetINTEntry(nIndex, sColumn, &nResult);
        }
        if (nLookup == 0)
        {
            m_storyHint[0] = 0;
            {
                CExoString sColumn("StoryHint");
                nLookup = pTable->GetINTEntry(0, sColumn, &nResult);
            }
        }
        if (nLookup)
        {
            nRet = nResult;
            ++m_storyHint[0];
        }
        else
            nRet = 0;
    }
    else
    {
        int nIndex = m_gamePlayHint;
        int nLookup;
        {
            CExoString sColumn("GamePlayHint");
            nLookup = pTable->GetINTEntry(nIndex, sColumn, &nResult);
        }
        if (nLookup == 0)
        {
            m_gamePlayHint = 0;
            {
                CExoString sColumn("GamePlayHint");
                nLookup = pTable->GetINTEntry(0, sColumn, &nResult);
            }
        }
        if (nLookup)
        {
            nRet = nResult;
            ++m_gamePlayHint;
        }
        else
            nRet = 0;
    }
    pTable->Unload2DArray();
    delete pTable;
    return nRet;
}
#else
uint64_t CClientExoAppInternal::GetNextLoadScreenHintSTRREF(const CExoString& sModuleName, unsigned char nModuleType)
{
    if (sModuleName.m_sString && sModuleName.m_sString[0])
    {
        unsigned char nModuleIdx = static_cast<unsigned char>(sModuleName[0] - '0');
        if (sModuleName.CompareNoCase("950COR") != 0)
            nModuleIdx = 0xff;
        if (nModuleIdx <= 9)
        {
            CExoIni ini;
            CExoString sIniValue;
            {
                CExoString sIniFile(".\\swkotor2.ini");
                CExoString sIniSection("Game Options");
                CExoString sIniKey("UnlockedPlanetSongs");
                ini.ReadIniEntry(sIniValue, sIniFile, sIniSection, sIniKey);
            }
            {
                CExoString sIniValue2(sIniValue.AsINT() | (1 << nModuleIdx));
                CExoString sIniFile2(".\\swkotor2.ini");
                CExoString sIniSection2("Game Options");
                CExoString sIniKey2("UnlockedPlanetSongs");
                ini.WriteIniEntry(sIniValue2, sIniFile2, sIniSection2, sIniKey2);
            }
        }
    }

    int nResult = 0;
    int nRet;
    C2DA* pTable;
    if (nModuleType == 1)
        goto l_t1;
    if (nModuleType == 2)
    {
        pTable = static_cast<C2DA*>(::operator new(sizeof(C2DA)));
        goto l_story;
    }
    else
    {
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        if (pServer && pServer->m_pInternal)
        {
            int nVisited = pServer->GetModulePreviouslyVisited(sModuleName);
            pTable = static_cast<C2DA*>(::operator new(sizeof(C2DA)));
            if (nVisited)
                goto l_story;
            goto l_screen;
        }
    }
l_t1:
    pTable = static_cast<C2DA*>(::operator new(sizeof(C2DA)));
l_screen:
    new (pTable) C2DA(CResRef("loadscreenhints"), 0);
    if (pTable->Load2DArray() == 0)
        goto l_fail;
    {
        int nIndex = m_gamePlayHint;
        int nIlookup;
        {
            CExoString sColumn("GamePlayHint");
            nIlookup = pTable->GetINTEntry(nIndex, sColumn, &nResult);
        }
        if (nIlookup == 0)
        {
            m_gamePlayHint = 0;
            {
                CExoString sColumn("GamePlayHint");
                nIlookup = pTable->GetINTEntry(0, sColumn, &nResult);
            }
            if (nIlookup == 0)
                goto l_gpFail;
        }
        goto l_gpFound;
    }
l_gpFound:
    nRet = nResult;
    ++m_gamePlayHint;
    goto l_gpDestroy;
l_gpFail:
    nRet = 0;
    goto l_gpDestroy;
l_gpDestroy:
    pTable->Unload2DArray();
    delete pTable;
    return static_cast<uint64_t>(nRet);

l_story:
    new (pTable) C2DA(CResRef("loadstoryhints"), 0);
    if (pTable->Load2DArray() == 0)
        goto l_fail;
    {
        unsigned char nDigit = static_cast<unsigned char>(sModuleName[0] - '0');
        if (nDigit >= 11)
        {
            pTable->Unload2DArray();
            goto l_fail;
        }
        else
        {
            int nAreaCount = pTable->m_nRows;
            int nIter = 0;
            unsigned char nLookup = m_storyHint[nDigit];
            CExoString sAreaColumn("AreaNumber");
            nRet = 0;
            for (;;)
            {
                if (pTable->GetINTEntry(static_cast<unsigned char>(nLookup), sAreaColumn, &nResult) == 0)
                {
                    nLookup = 0;
                    m_storyHint[nDigit] = 0;
                    if (nIter > nAreaCount)
                        break;
                    nIter++;
                    continue;
                }
                unsigned char nCounter = m_storyHint[nDigit];
                if (nResult == static_cast<int>(nDigit))
                {
                    int nStoryRet;
                    {
                        CExoString sStoryColumn("StoryHint");
                        nStoryRet = pTable->GetINTEntry(nCounter, sStoryColumn, &nResult);
                    }
                    nRet = nStoryRet != 0 ? nResult : 0;
                    ++m_storyHint[nDigit];
                    break;
                }
                nLookup = nCounter + 1;
                m_storyHint[nDigit] = nLookup;
                if (nIter > nAreaCount)
                    break;
                nIter++;
            }
            pTable->Unload2DArray();
            delete pTable;
            return static_cast<uint64_t>(nRet);
        }
    }

l_fail:
    delete pTable;
    return 0;
}
#endif

void CClientExoAppInternal::InitializeMoviePlayer()
{
    m_pMoviePlayer = 0;
    m_pMoviePlayer = new CExoMoviePlayer;
    m_pMoviePlayer->Initialize(0);
}

void CClientExoAppInternal::Uninitialize()
{
    CSWCVisualEffect::UnloadCachedVFX();
    m_pWalkToRingVFX = 0;

    while (m_queuedAnims.m_pInternal->m_pHead != 0) {
        CQueuedAnim* pAnim = m_queuedAnims.RemoveHead();
        if (pAnim)
            delete pAnim;  // runs ~CExoString on m_sName, then frees
    }

    while (m_hitCheckIgnoreList.m_pInternal->m_pHead != 0) {
        unsigned long long* pId =
            static_cast<unsigned long long*>(m_hitCheckIgnoreList.m_pInternal->RemoveHead());
        if (pId)
            delete pId;
    }

    if (g_pDialogIconModel != 0) {
        delete static_cast<Gob*>(g_pDialogIconModel);
        g_pDialogIconModel = 0;
    }
    if (g_pGroundRing != 0) {
        delete static_cast<Gob*>(g_pGroundRing);
        g_pGroundRing = 0;
    }
    if (g_pRules != 0) {
        delete g_pRules;
        g_pRules = 0;
    }
    if (m_pLoadScreenObj != 0) {
        delete static_cast<CSWGuiLoadScreen*>(m_pLoadScreenObj);
        m_pLoadScreenObj = 0;
    }
    if (m_pLegalScreen != 0) {
        delete m_pLegalScreen;
        m_pLegalScreen = 0;
    }

    SetTexturePack(0xff, 0);
}

__attribute__((noinline))
void CClientExoAppInternal::EmptyHitcheckIgnoreList()
{
    while (m_hitCheckIgnoreList.m_pInternal->m_pHead != 0) {
        unsigned long long* pId = static_cast<unsigned long long*>(m_hitCheckIgnoreList.m_pInternal->RemoveHead());
        if (pId)
            delete pId;
    }
}

int CClientExoAppInternal::SetTexturePack(unsigned char nPack, int nUnused)
{
    K2_UNUSED(nUnused);
    CExoString sTexture, sUnused1, sUnused2, sGui;
    float fDynMemRatio = 0.0f;
    int nMem = 0;
    C2DA* pC2DA;
    int bLoaded;
    int nResult;
    if (m_currentTexturePack == nPack)
        goto lblResultOne;
#ifdef K2_FIX_ENGINE_BUGS
    // Every request loads row 2 and records 2, so any other row asked for is already loaded.
    if (m_currentTexturePack == 2 && nPack != 0xff)
        goto lblResultOne;
#endif
    m_currentTexturePack = nPack;

    if (!(g_pRules && g_pRules->m_p2DArrays))
        goto lblLoad;
    pC2DA = g_pRules->m_p2DArrays->m_pTexPacks;
    bLoaded = false;
    nResult = 0;
    if (pC2DA == 0)
        goto lblEpilogue;
    if (pC2DA->m_nRows == 0)
        goto lblUnload;

lblRowGate:
    if (pC2DA->m_nRows <= nPack)
        goto lblUnload;

    pC2DA->GetCExoStringEntry(2, CExoString("Texture"), &sTexture);
    pC2DA->GetCExoStringEntry(2, CExoString("Gui"), &sGui);
    pC2DA->GetFLOATEntry(2, CExoString("DynMemRatio"), &fDynMemRatio);
    pC2DA->GetINTEntry(2, CExoString("Mem"), &nMem);

    AurTextureResetAll();
    CAuroraInterface::SetTexturePack(1, CExoString("TEXTUREPACKS:") + sTexture);
    CAuroraInterface::SetTexturePack(2, CExoString("TEXTUREPACKS:") + sGui);
    AurTextureSetDynamicMemoryRatio(fDynMemRatio);
    AurTextureSetMem(nMem);

    if (!bLoaded)
        goto lblStoreTwo;
    pC2DA->Unload2DArray();
    delete pC2DA;
lblStoreTwo:
    m_currentTexturePack = 2;
lblResultOne:
    nResult = 1;
    goto lblEpilogue;

lblUnload:
    pC2DA->Unload2DArray();
lblDelete:
    delete pC2DA;
    nResult = 0;
lblEpilogue:
    return nResult;

lblLoad:
    pC2DA = new C2DA(CResRef("texpacks"), 1);
    if (pC2DA->Load2DArray() == 0)
        goto lblDelete;
    bLoaded = true;
    if (pC2DA->m_nRows != 0)
        goto lblRowGate;
    goto lblUnload;
}

int CClientExoAppInternal::GetVideoMode(unsigned int a, unsigned int b, unsigned char c,
                                           unsigned int d)
{
    return CAuroraInterface::GetExactVideoMode(a, b, c, d);
}

void CClientExoAppInternal::ResetDriveAcceleration()
{
    if (m_pPlayerControl != 0)
        m_pPlayerControl->ResetDriveAcceleration();
}

void CClientExoAppInternal::CancelMovie()
{
    if (m_pMoviePlayer != 0)
        m_pMoviePlayer->CancelMovie(1, 0);
}

void CClientExoAppInternal::SetPlayerCharacterName(CExoString const& newName)
{
    m_playerCharacterName = newName;
    m_playerCharacterSet = 1;
}

void CClientExoAppInternal::PlayMovieQueue(int nArg)
{
#if KOTOR1  // a queued 55.bik starts the evil-ending music and hides the mouse
    int nLocal;
    int nEvil = 0;
    {
        CExoString sEvil(".\\movies\\55.bik");
        for (int i = 0; i < m_moviesToPlay.m_count; i++)
            if (m_moviesToPlay[i] == sEvil)
                nEvil++;
    }
    if (nEvil) {
        m_bEvilEndingHackInProgress = 1;
        HideMouse(MOUSEHIDE_EVIL_ENDING);
    }
#else
    int nLocal;
#endif
    PlayMoviesInExoArrayList(m_moviesToPlay, nLocal, 0, nArg, &m_movieFlags, 0);
    m_moviesToPlay.m_count = 0;
    m_movieFlags.m_count = 0;
}

CSWCCreature* CClientExoAppInternal::GetPlayerCreature()
{
    if (m_oidCreatureCache == m_playerCreatureId)
        return static_cast<CSWCCreature*>(m_pCreatureCache);
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_playerCreatureId);
    m_pCreatureCache = pCreature;
    m_oidCreatureCache = m_playerCreatureId;
    return pCreature;
}

void CClientExoAppInternal::QueryExitProgram()
{
    if (g_pAppManager->m_pClientExoApp && g_pAppManager->m_pClientExoApp->GetInGameGui() &&
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut == 0) {
        g_pAppManager->m_pClientExoApp->StopMovie();
        if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
            CSWCCreature* pCreature;
            if (m_oidCreatureCache == m_playerCreatureId) {
                pCreature = static_cast<CSWCCreature*>(m_pCreatureCache);
            } else {
                pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_playerCreatureId);
                m_pCreatureCache = pCreature;
                m_oidCreatureCache = m_playerCreatureId;
            }
            if (pCreature && !pCreature->m_bInCombat && !m_nGameOverReason && m_pModule) {
                g_bShowIndicator = 1;
                m_pModule->RestoreCamera();
                SetInputClass(INPUTCLASS_GAME, 1);
            }
        }
        if (!m_bAutoPaused) {
            SetPausedByCombat(1, PAUSE_MODE_PLAYER, 0);
            SetAutoPaused(1, PAUSE_MODE_NONE);
        }
        if (m_pModule && m_pModule->GetMiniGame()) {
            ShowMouse(MOUSEHIDE_MINIGAME);
        }
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(1);
        pBox->SetMessage(STRREF_QUIT_CONFIRM);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
#if KOTOR1  // the box is its own handler object; the callback is MacHack's
        pBox->SetCallback(pBox, reinterpret_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&MacHack::HandleExitGameButton), 0);
#else
        pBox->SetCallback(reinterpret_cast<CSWGuiObject*>(this),  // idiom-exempt: cast-this-to-base (K) — the msg-box
                          reinterpret_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CClientExoAppInternal::HandleExitGameButton),
                          0);
#endif
        g_pExoInput->SetActive(1);
        g_pAppManager->m_pClientExoApp->EnableInput();
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    }
}

void CClientExoAppInternal::HandleExitGameButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CGuiInGame* pInGame = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pInGame->m_pSoloModeQueryBox->m_bAccepted) {
        m_bQuitClicked = 1;
        m_bWMCLOSEQuery = 1;
#if KOTOR1  // quitting raises the port's quit flag
        ExitProgram();
#else
        PostQuitMessage(0);
#endif
    }
}

void CClientExoAppInternal::SetCanSendDialog(int bCan)
{
    m_bCanSendDialog = bCan;
    m_nCanSendDialogTime = static_cast<int>(g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer->GetSnapshotTime() / 1000);
}

void CClientExoAppInternal::PlayCharacterSelectSound(CSWCCreature* pCreature)
{
    int nPick = rand_wincompatible() % 3;
    int nSound = nPick == 0 ? 7 : (nPick == 1 ? 8 : 9);
    pCreature->PlaySoundSetSound(static_cast<unsigned short>(nSound), 1);
}

void CClientExoAppInternal::HurryDeathSequence()
{
    m_pInGameGui->StartGlobalFade(0, 0.0f, 0.5f, Vector(0.0f, 0.0f, 0.0f));
}

void CClientExoAppInternal::SetCombatMode(int mode)
{
    if (mode == 0)
        m_attackMashTutorialTimer = 0;
    if (m_combatMode == mode)
        return;
    m_combatMode = mode;
    m_pInGameGui->RePopulateMainInterface();
#ifdef K2_FIX_ENGINE_BUGS
    if (m_pModule != 0 && m_pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_DIALOG) != 0)
        return;
#endif
    if (m_pModule != 0) {
        if (m_combatMode != 0)
            m_pModule->SetCombatCamera();
        else
            m_pModule->RestoreCamera();
    }
}

void CClientExoAppInternal::SetMouseMode(unsigned char mode)
{
    HideMouse(MOUSEHIDE_MOUSE_MODE);
    g_pExoInput->SetMouseMode(mode);
    if (m_pSWGuiManager != 0) {
        if (mode == 1)
            m_pSWGuiManager->ActivateSoftwareMouse();
        else
            m_pSWGuiManager->DeactivateSoftwareMouse();
    }
    ShowMouse(MOUSEHIDE_MOUSE_MODE);
}

void CClientExoAppInternal::ShutDownMoviePlayer()
{
    if (m_pMoviePlayer != 0) {
        while (m_pMoviePlayer->IsMoviePlaying() == 1) {
            m_pMoviePlayer->CancelMovie(1, 1);
            Sleep(5);
        }
        delete m_pMoviePlayer;
        m_pMoviePlayer = 0;
    }
}

void CClientExoAppInternal::SetLastPartyMemberTempKilled(unsigned long long serverId)
{
    if (serverId == OBJECT_INVALID)
        return;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(serverId);
    if (pCreature == 0 || pCreature->m_bInParty == 0)
        return;
    m_lastPartyMemberTempKilled = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(serverId);
}

int CClientExoAppInternal::ShowTutorialWindow(unsigned char reason, unsigned long long a,
                                             unsigned long long b, unsigned int span)
{
    int bShown = m_pInGameGui->TutorialReasonWillShowWindow(reason);
    if (!bShown)
        return bShown;
    bShown = m_pInGameGui->SetTutorialReason(reason);
    if (!bShown)
        return bShown;
    m_tutorialObjA = a;
    m_tutorialObjB = b;
    m_tutorialTimerSpan = span;
    m_tutorialReason = reason;
    return 1;
}

__attribute__((noinline)) std::string& SetLangCode(std::string& sLang, const char* pCode,
                                                         size_t nLen)
{
    return sLang.assign(pCode, nLen);
}

#if KOTOR1
void CClientExoAppInternal::BeginIntro()
{
    if (m_bDisableIntroMovies == 0)
    {
        {
            CExoString sMovie1("leclogo");
            m_movieFlags.Add(1);
            AddMovieToExoArrayList(sMovie1, m_moviesToPlay);
        }
        {
            CExoString sMovie2("biologo");
            m_movieFlags.Add(1);
            AddMovieToExoArrayList(sMovie2, m_moviesToPlay);
        }
        {
            CExoString sMovie3("Aspyr_BlueDust_intro");
            m_movieFlags.Add(1);
            AddMovieToExoArrayList(sMovie3, m_moviesToPlay);
        }
        {
            CExoString sMovie4("aspyr");
            m_movieFlags.Add(1);
            AddMovieToExoArrayList(sMovie4, m_moviesToPlay);
        }
        {
            CExoString sMovie5("legal");
            m_movieFlags.Add(1);
            AddMovieToExoArrayList(sMovie5, m_moviesToPlay);
        }
        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
        jmethodID mid = env->GetStaticMethodID(cls, "showWindow", "()V");
        env->CallStaticVoidMethod(cls, mid);
        env->DeleteLocalRef(cls);
        PlayMovieQueue(1);
    }
}
#else
void CClientExoAppInternal::BeginIntro()
{
    if (m_bDisableIntroMovies == 0)
    {
        {
            CExoString sMovie1("leclogo");
            m_movieFlags.Add(1);
            AddMovieToExoArrayList(sMovie1, m_moviesToPlay);
        }
        {
            CExoString sMovie2("obsidianent");
            m_movieFlags.Add(1);
            AddMovieToExoArrayList(sMovie2, m_moviesToPlay);
        }
        {
            CExoString sMovie3("Aspyr");
            m_movieFlags.Add(1);
            AddMovieToExoArrayList(sMovie3, m_moviesToPlay);
        }
        RevealAndroidWindowIfNeeded();
        {
            int nUnused;
            PlayMoviesInExoArrayList(m_moviesToPlay, nUnused, 0, 1, &m_movieFlags, 0);
        }
        m_moviesToPlay.m_count = 0;
        m_movieFlags.m_count = 0;
        if (g_bExitProgram)
            return;

        if (m_pLegalScreen == NULL)
            m_pLegalScreen = new CSWGuiLegalScreen(m_pSWGuiManager, 2000);
        m_pSWGuiManager->AddPanel(m_pLegalScreen, 1, 1);
        std::string sLang("eng");
        const char* pszFolder = GetLocalizationFolderName();
        if (strstr(pszFolder, "rench") != NULL)
            SetLangCode(sLang, "fre", 3);
        else if (strstr(pszFolder, "talian") != NULL)
            SetLangCode(sLang, "ita", 3);
        else if (strstr(pszFolder, "erman") != NULL)
            SetLangCode(sLang, "deu", 3);
        else if (strstr(pszFolder, "panish") != NULL)
            SetLangCode(sLang, "esp", 3);
        std::string sCopyright = "copyright_" + sLang;
#ifndef K2_UI_TOUCH
        // The port's own copyright art; the PC data has its own legal screen.
        if (!g_pExoResMan->Exists(CResRef(sCopyright.c_str()), 3, static_cast<unsigned int*>(0)) &&
            !g_pExoResMan->Exists(CResRef(sCopyright.c_str()), 3007, static_cast<unsigned int*>(0)))
            sCopyright = "legal";
#endif
        CSWGuiLegalScreen* pLegalScreen = m_pLegalScreen;
        {
            CResRef ref(sCopyright.c_str());
            pLegalScreen->SetImage(ref);
        }
        pLegalScreen = m_pLegalScreen;
        {
            CResRef ref2("");
            pLegalScreen->SetImage(ref2);
        }
        m_pSWGuiManager->RemovePanel(m_pLegalScreen);
        if (m_pLegalScreen)
            delete m_pLegalScreen;
        m_pLegalScreen = NULL;
    }
    else
    {
        RevealAndroidWindowIfNeeded();
        if (g_bExitProgram)
            return;

        if (m_pLegalScreen == NULL)
            m_pLegalScreen = new CSWGuiLegalScreen(m_pSWGuiManager, 2000);
        m_pSWGuiManager->AddPanel(m_pLegalScreen, 1, 1);
        std::string sLang("eng");
        const char* pszFolder = GetLocalizationFolderName();
        if (strstr(pszFolder, "rench") != NULL)
            SetLangCode(sLang, "fre", 3);
        else if (strstr(pszFolder, "talian") != NULL)
            SetLangCode(sLang, "ita", 3);
        else if (strstr(pszFolder, "erman") != NULL)
            SetLangCode(sLang, "deu", 3);
        else if (strstr(pszFolder, "panish") != NULL)
            SetLangCode(sLang, "esp", 3);
        std::string sCopyright = "copyright_" + sLang;
#ifndef K2_UI_TOUCH
        // The port's own copyright art; the PC data has its own legal screen.
        if (!g_pExoResMan->Exists(CResRef(sCopyright.c_str()), 3, static_cast<unsigned int*>(0)) &&
            !g_pExoResMan->Exists(CResRef(sCopyright.c_str()), 3007, static_cast<unsigned int*>(0)))
            sCopyright = "legal";
#endif
        CSWGuiLegalScreen* pLegalScreen = m_pLegalScreen;
        {
            CResRef ref(sCopyright.c_str());
            pLegalScreen->SetImage(ref);
        }
        pLegalScreen = m_pLegalScreen;
        {
            CResRef ref2("");
            pLegalScreen->SetImage(ref2);
        }
        m_pSWGuiManager->RemovePanel(m_pLegalScreen);
        if (m_pLegalScreen)
            delete m_pLegalScreen;
        m_pLegalScreen = NULL;
    }
}
#endif

void CClientExoAppInternal::HandleIntroDelete()
{
}

void CClientExoAppInternal::ReloadDynamicEventDescriptions()
{
    SetEventDescriptions();
}

void CClientExoAppInternal::DoDownloadedResourceDirectories(DoDLResDirParam param, int index)
{
    K2_UNUSED(param); K2_UNUSED(index);
}

void CClientExoAppInternal::AddDownloadedResources()
{
    CExoString sLive;
    CExoString sLIVE;
    const CExoString* pPath;

    for (unsigned int i = 1; i < CTlkTable::NUM_CONTENT_PKGS; i++) {
        sLive.Format(const_cast<char*>("live%d"), i);
        sLIVE.Format(const_cast<char*>("LIVE%d"), i);

        pPath = &g_pExoBase->m_pAliasList->GetAliasPath(sLIVE, 0);
        if (pPath->GetLength() != 0) {
            if (g_pExoResMan->Exists(CResRef(sLive), 2018, static_cast<unsigned int*>(0)) != 0) {
                sLIVE.Format(const_cast<char*>("LIVE%d:live%d"), i, i);
                g_pTlkTable->AddFile(sLIVE, i);
            }

            if (g_pExoResMan->Exists(CResRef(sLive), 9999, static_cast<unsigned int*>(0)) != 0) {
                sLIVE.Format(const_cast<char*>("LIVE%d:live%d"), i, i);
                g_pExoResMan->AddFixedKeyTableFile(sLIVE);
            }

            if (g_pExoResMan->Exists(CResRef(sLive), RESTYPE_RIM, static_cast<unsigned int*>(0)) != 0) {
                sLIVE.Format(const_cast<char*>("LIVE%d:RIMSXBOX\\live%d"), i, i);
                g_pExoResMan->AddResourceImageFile(sLIVE);
            }

            if (g_pExoResMan->Exists(CResRef(sLive), RESTYPE_MOD, static_cast<unsigned int*>(0)) != 0) {
                sLIVE.Format(const_cast<char*>("LIVE%d:live%d"), i, i);
                g_pExoResMan->AddEncapsulatedResourceFile(sLIVE, 2);
            }

            CExoString sLiveDX = sLive + "DX";  // unused

            if (g_pExoResMan->Exists(CResRef(sLive), RESTYPE_RIM, static_cast<unsigned int*>(0)) != 0) {
                sLIVE.Format(const_cast<char*>("LIVE%d:RIMSXBOX\\live%ddx"), i, i);
                g_pExoResMan->AddResourceImageFile(sLIVE);
            }

            sLIVE.Format(const_cast<char*>("LIVE%d:OVERRIDE\\textures"), i);
            g_pExoResMan->AddEncapsulatedResourceFile(sLIVE, 1);
        }
    }
}

int CClientExoAppInternal::AttachPatchFiles(int directory)
{
    K2_UNUSED(directory);
    return 1;
}

void CClientExoAppInternal::NextPartyFormation()
{
    m_pParty->SetFormation(-9999);
}

void CClientExoAppInternal::ChangeCharacter(int nLeader, int bPlaySound)
{
    CSWCCreature* pOldLeader = m_pParty->GetPlayerCharacter();
    m_pParty->SetLeader(nLeader);
    if (pOldLeader != m_pParty->GetPlayerCharacter()) {
        CSWCCreature* pCharacter = m_pParty->GetPlayerCharacter();
        Vector vPos = pCharacter->m_position;
        m_pParty->ResetAllFollowData(vPos, Yaw(pCharacter->m_orientation), 1);
        CSWSCreature* pServer = pCharacter->GetServerCreature();
        if (pServer) {
            pServer->m_pPathfindInfo->m_oidIgnoredBlocker = OBJECT_INVALID;
            pServer->RemoveAction(AIACTION_PARTY_FOLLOW_LEADER);
#if KOTOR1  // K1 uses GetDeadTemp(), and the C library rand
            if (pServer->GetDead() || pServer->GetDeadTemp())
#else
            if (pServer->GetDead() ||
                pServer->GetDeadTemp() != 0)
#endif
                bPlaySound = 0;
        }
        if (pCharacter) {
            m_pModule->SetCameraFollow(pCharacter->m_objectId, 1);
            if (bPlaySound) {
#if KOTOR1
                int nSel = rand() % 3;
#else
                int nSel = rand_wincompatible() % 3;
#endif
                if (nSel == 0)
                    pCharacter->PlaySoundSetSound(7, 1);
                else if (nSel == 1)
                    pCharacter->PlaySoundSetSound(8, 1);
                else
                    pCharacter->PlaySoundSetSound(9, 1);
            }
        }
        if (m_pPlayerControl)
            m_pPlayerControl->ResetDriveAcceleration();
#ifdef K2_FIX_ENGINE_BUGS
        // Only the server's PlayerInfo naming a creature other than m_playerCreatureId clears the
        // flag, so a cycle that lands back on the creature already controlled (every other member
        // incapacitated) would leave it raised and lock the level-up buttons until the next switch.
        m_bCharacterChangeInProgress = (pCharacter == 0 || pCharacter->m_objectId != m_playerCreatureId);
#else
        m_bCharacterChangeInProgress = 1;
#endif
    }
}

void CClientExoAppInternal::RestartMoviesIfNecessary()
{
    if (m_pMoviePlayer != 0 && m_pMoviePlayer->GetNumberOfMoviesToPlay() != 0) {
#if KOTOR1  // no mouse hide and no resolution change around the restart
        m_pMoviePlayer->RestartMoviesAsync();
        WaitForMovie(1);
#else
        HideMouse(MOUSEHIDE_MOVIE);
        ChangeResolutionForMovie();
        m_pMoviePlayer->RestartMoviesAsync();
        WaitForMovie(1);
        RestoreResolutionFromMovie();
        ShowMouse(MOUSEHIDE_MOVIE);
#endif
        m_pMoviePlayer->HideFadePanel();
    }
}

void CClientExoAppInternal::TurnOffKeyboardModal()
{
    if (m_keyboardModalFlags[0] || m_keyboardModalFlags[1] || m_keyboardModalFlags[2] ||
        m_keyboardModalFlags[3] || m_keyboardModalFlags[4] || m_keyboardModalFlags[5] ||
        m_keyboardModalFlags[6]) {
        for (int i = 0; i < 7; ++i)
            m_keyboardModalFlags[i] = 0;
        m_bForceDriveUpdate = 1;
        m_nDriveKeyPressTime = 0;
        m_pSWCMessage->SendPlayerToServerInput_AbortDriveControl();
    }
    if (m_pModule != 0 && m_pModule->m_pArea != 0)
        m_pModule->m_pArea->HiliteAreaObjects(0);
}

int CClientExoAppInternal::IsSoundPlayingInDialog()
{
#if KOTOR1  // no VO-debug block; the speaker lookup must return SUCCESS
    if (m_pInGameGui->m_bDialogSoundPolled == 0) {
        m_pInGameGui->m_bDialogSoundPolled = 1;
        return 1;
    }
    unsigned long long nId = m_pInGameGui->m_oidDialogSoundSpeaker;
    CGameObject* pObj = 0;
    int nPlaying = 0;
    if (m_pGameObjectArray->GetGameObject(nId, &pObj) == CGameObjectArray::SUCCESS && pObj != 0 &&
        pObj->AsSWCObject() != 0 && m_pInGameGui->m_pDialogSound != 0)
        nPlaying = m_pInGameGui->m_pDialogSound->IsPlaying();
    return nPlaying;
#else
    if (m_pInGameGui->m_bDialogSoundPolled != 0) {
        unsigned long long nId = m_pInGameGui->m_oidDialogSoundSpeaker;
        // The pointer and the string never overlap in live time, since the pointer is dead before
        // CopyToString runs.
        union { CGameObject* pObj; char buf[17]; } u;  // CopyToString writes 17 bytes (NUL at [16])
        u.pObj = 0;
        m_pGameObjectArray->GetGameObject(nId, &u.pObj);
        if (u.pObj != 0 && u.pObj->AsSWCObject() != 0 && m_pInGameGui->m_pDialogSound != 0) {
            if (g_bVODebugEnabled) {
                m_pInGameGui->m_pDialogSound->GetFilePlaying().CopyToString(u.buf);
                g_pExoSound->GetSoundMode();
            }
            return m_pInGameGui->m_pDialogSound->IsPlaying();
        }
        return 0;
    }

    if (g_bVODebugEnabled) {
        union { CGameObject* pObj; char buf[17]; } u;
        if (m_pInGameGui->m_pDialogSound != 0) {
            CResRef r = m_pInGameGui->m_pDialogSound->GetFilePlaying();
            r.CopyToString(u.buf);
        }
        g_pExoSound->GetSoundMode();
    }
    m_pInGameGui->m_bDialogSoundPolled = 1;
    return 1;
#endif
}

void CClientExoAppInternal::RunDeathSequence()
{
    m_nGameOverReason = 1;
    m_bGameOver = 1;
    m_pSWGuiManager->HideAllPanels();
#if KOTOR1
    m_pInGameGui->m_pSoloModeQueryBox->DisplayDeathMessage();
#else
    m_pInGameGui->DisplayDeathMessage();
#endif

    CSWCCreature* pCreature = GetCreatureByGameObjectID(m_lastPartyMemberTempKilled);
    if (m_pModule != 0 && pCreature != 0 && m_pModule->m_pCamera != 0) {
        CAurCamera* pCam = m_pModule->m_pCamera;
        CSWCameraDeath* pDeathCam = new CSWCameraDeath(
            pCam->AsCAurObject(), static_cast<CAurObject*>(pCreature->GetCameraFollowPart()));
        pCam->SetBehavior(static_cast<CAurBehavior*>(pDeathCam));  // no-op cast: the pointer passes unchanged
    }

    m_pInGameGui->StartGlobalFade(0, 12.0f, 1.0f, Vector());
}

int CClientExoAppInternal::PlayRumblePattern(int nPattern)
{
    if (nPattern < 0 || nPattern >= m_rumblePatternList.m_count)
        return 0;

    SRumblePattern* pScan = m_rumblePatterns.m_pArray;
    if (m_rumblePatternList.m_pArray[nPattern].m_nLooping != 0 &&
        m_rumblePatterns.m_count >= 1) {
        for (int i = 0; i < m_rumblePatterns.m_count; ++i) {
            if (pScan[i].nPattern == nPattern)
                return 0;
        }
    }

    int nOldCount = m_rumblePatterns.m_count;
    m_rumblePatterns.SetSize(nOldCount + 1);
    SRumblePattern& rNew = m_rumblePatterns.m_pArray[nOldCount];
    rNew.nPattern = nPattern;
    rNew.fPhase = 0.0f;
    rNew.nState = 1;
    return 1;
}

int CClientExoAppInternal::StopRumblePattern(int pattern)
{
    for (int i = 0; i < m_rumblePatterns.m_count; i++) {
        if (m_rumblePatterns.m_pArray[i].nPattern == pattern) {
            m_rumblePatterns.DelIndex(i);
            return 1;
        }
    }
    return 0;
}

void CClientExoAppInternal::LookUpAndPerformRumbleWithCutOff(int nType, unsigned char bTable,
                                                             const Vector& vPos)
{
    CExoString sType;
    C2DA* pTable;
    int nUseString;
    switch (bTable) {
    case CClientExoApp::RUMBLE_TABLE_FOOTSTEP_SOUNDS:
        pTable = g_pRules->m_p2DArrays->m_pFootstepSounds;
        nUseString = 0;
        break;
    case CClientExoApp::RUMBLE_TABLE_VISUAL_EFFECT:
        pTable = g_pRules->m_p2DArrays->m_pVisualEffect;
        sType = CExoString(nType);
        nUseString = 1;
        break;
    default:
        return;
    }
    if (pTable == 0)
        return;

    int nPattern;
    if (nUseString) {
        if (pTable->GetINTEntry(sType, CExoString("RumblePattern"), &nPattern) == 0)
            return;
    } else {
        if (pTable->GetINTEntry(nType, CExoString("RumblePattern"), &nPattern) == 0)
            return;
    }

    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pPlayer == 0)
        return;

    float fCutOff;
    if (nUseString) {
        if (pTable->GetFLOATEntry(sType, CExoString("RumbleCutOff"), &fCutOff) == 0)
            fCutOff = 10.0f;
    } else {
        if (pTable->GetFLOATEntry(nType, CExoString("RumbleCutOff"), &fCutOff) == 0)
            fCutOff = 10.0f;
    }

    float fDx, fDy, fDz;
    Vector vRumblePoint;
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pGui->m_nGlobalDialogState)
        g_pExoSound->GetListenerPosition(&vRumblePoint);
    else
        vRumblePoint = pPlayer->m_position;

    fDx = vRumblePoint.x - vPos.x;
    fDy = vRumblePoint.y - vPos.y;
    fDz = vRumblePoint.z - vPos.z;
    if (fDx * fDx + fDy * fDy + fDz * fDz > fCutOff * fCutOff)
        return;
    PlayRumblePattern(nPattern);
}

int CClientExoAppInternal::GetControlledCharacterDead()
{
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CSWCCreature* pMember = pParty->GetCharacter(0);
    if (pMember == 0)
        return 0;
    CSWSCreature* pServer = pMember->GetServerCreature();
    if (pServer == 0)
        return 0;
#if KOTOR1  // K1 returns GetDeadTemp()
    return pServer->GetDeadTemp();
#else
    if (pServer->m_bInParty == 0)
        return 0;
    return pServer->GetCurrentHitPoints(0) < 1;
#endif
}

class CImeUI {
public:
#if !KOTOR1  // K1's is an empty object
    char                _pad00[0x208];
    unsigned long long  m_nReserved1 = 0;
    int                 m_nReserved2 = 0;
    char                _pad214[0x218-0x214];
    CExoString m_sReserved1;
    CExoString m_sReserved2;
    CExoString m_sReserved3;
#endif
};
#include "exofileinternal.h"
#include "exoinput.h"
extern CAuroraInterface* g_pAuroraInterface;
class CNetLayer; class CClientExoAppInternal;
extern unsigned char g_bExtendedResourceTracking;
#if KOTOR1
extern unsigned char g_is_antialias_on;
#if KOTOR1
unsigned char g_is_antialias_on;
#endif
#endif

#ifdef K2_QOL_CONTROLLER
// rumble.2da, which the mobile build never loads, so PlayRumblePattern found no patterns.
static void K2QolLoadRumblePatterns(CExoArrayList<CSWRumblePattern>& patterns)
{
    if (patterns.m_count != 0)
        return;
    C2DA* p2da = new C2DA(CResRef("rumble"), 0);
    p2da->Load2DArray();
    int nRows = p2da->m_nRows;
    if (nRows > 0) {
        patterns.m_pArray = new CSWRumblePattern[nRows];
        patterns.m_capacity = nRows;
        patterns.m_count = nRows;
        for (int i = 0; i < nRows; ++i)
            patterns.m_pArray[i].LoadPattern(i, p2da);
    }
    delete p2da;
}
#endif

#if KOTOR1
int CClientExoAppInternal::StartServices(int a, const CExoString& b, const CExoString& sLoadMusic,
                                         const CExoString& d, uint64_t e)
{
    K2_UNUSED(b); K2_UNUSED(d); K2_UNUSED(e);
    CExoIni    ini;
    CExoString sValue;
    CExoString sDialogFile;

    m_pSysAdminMessenger = new CClientMsgSysAdmin;
    m_pWorldTimer        = new CWorldTimer;
    m_pExclusionTimer           = new CWorldTimer;
    m_pPauseTimer        = new CWorldTimer;
    m_pAnimationTimer    = new CWorldTimer;
    m_pGameObjectArray   = new CGameObjectArray(1);
    g_pExoResMan         = new CExoResMan;
    g_ExoFileThread      = new CExoFileThread;

    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Graphics Options"),
                         CExoString("AllowWindowedMode"))) {
        g_bAllowWindowedMode = sValue.AsINT() != 0;
    }

    g_pAurora = NewCAurora();

    if (m_pWorldJournal == 0)
        m_pWorldJournal = new CSWCWorldJournal;

    m_pNetLayer = new CNetLayer;

    if (m_pServerInfo == 0)
        m_pServerInfo = new CServerInfo;

    m_pSWCMessage = new CSWCMessage;

    m_pClientAIMaster = new CClientAIMaster;
    m_pClientAIMaster->SetExoAppInternal(this);

    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Game Options"),
                         CExoString("EnableReleaseLogging"))) {
        CExoDebug* pDebug = g_pExoBase->m_pDebug;
        pDebug->m_bEnableReleaseLogging = sValue.AsINT() != 0;
    }

    g_bExtendedResourceTracking = 0;
    g_pExoBase->m_pDebug->OpenLogFiles(CExoString("swc"));

    g_pExoResMan->AddResourceDirectory(CExoString("TEMPCLIENT:"));
    g_pExoResMan->AddResourceDirectory(CExoString("OVERRIDE:"));
    g_pExoResMan->AddResourceDirectory(CExoString("ERRORTEX:"));

    g_pTlkTable = g_pSWTlkTable = new CSWTlkTable;

    g_pExoResMan->CacheResource(CExoString("HD0:DATAXBOX\\items"), RESTYPE_BIF);
    g_pExoResMan->CacheResource(CExoString("HD0:DATAXBOX\\scripts"), RESTYPE_BIF);
    g_pExoResMan->CacheResource(CExoString("HD0:DATAXBOX\\gui"), RESTYPE_BIF);

    g_pExoResMan->AddFixedKeyTableFile(CExoString("HD0:CHITIN"));
    g_pExoResMan->AddResourceDirectory(CExoString("RIMS:"));
    g_pAuroraInterface = new CAuroraInterface;
    g_pExoResMan->AddResourceDirectory(CExoString("SERVERVAULT:"));
    g_pExoResMan->AddResourceDirectory(CExoString("PORTRAITS:"));
    g_pExoResMan->AddEncapsulatedResourceFile(CExoString("OVERRIDE:textures"), 1);
    g_pExoResMan->AddEncapsulatedResourceFile(CExoString("HD0:patch"), 1);
    g_pExoResMan->AddResourceDirectory(CExoString("HD0:MOVIES"));
    g_pExoResMan->AddResourceDirectory(CExoString("HD0:STREAMWAVES"));
    g_pExoResMan->AddResourceDirectory(CExoString("HD0:STREAMMUSIC"));
    if (g_pExoResMan->Exists(CResRef("GLOBAL"), RESTYPE_RIM, static_cast<unsigned int*>(0)))
        g_pExoResMan->AddResourceImageFile(CExoString("RIMS:GLOBAL"));
    InitializeSoundOptions();

    if (a != 0) {
        m_loadMusic = sLoadMusic;
        StartLoadMusic();
    }

    const char* pszDialog;
    switch (ASLPlat_GetCurrentLanguage()) {
        case ASL_LANGUAGE_FRENCH:   pszDialog = "HD0:DIALOGFR"; break;
        case ASL_LANGUAGE_ITALIAN:  pszDialog = "HD0:DIALOGIT"; break;
        case ASL_LANGUAGE_GERMAN:   pszDialog = "HD0:DIALOGDE"; break;
        case ASL_LANGUAGE_SPANISH:  pszDialog = "HD0:DIALOGES"; break;
        default:                    pszDialog = "HD0:DIALOG";   break;
    }
    sDialogFile = g_pExoResMan->CacheResource(CExoString(pszDialog), RESTYPE_TLK);
    g_pTlkTable->OpenFile(sDialogFile);

    AddDownloadedResources();
    m_pNetLayer->Initialize(reinterpret_cast<CBaseExoApp*>(g_pAppManager->m_pClientExoApp));
    srand(GetTickCount());
    m_pModule = new CSWCModule;

    {
        int bRead = ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Graphics Options"),
                                     CExoString("Texture Quality"));
        int nPack = sValue.AsINT();
        if (bRead && nPack <= 3)
            SetTexturePack(static_cast<unsigned char>(nPack), 0);
        else
            SetTexturePack(2, 0);
    }

    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Graphics Options"),
                         CExoString("FullScreen"))) {
        sValue.AsINT();
    } else {
        sValue.Format(const_cast<char*>("%d"), 0);
        ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Graphics Options"),
                          CExoString("FullScreen"));
    }

    int nDisableMovies = 0;
    if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Game Options"),
                         CExoString("Disable Movies")))
        nDisableMovies = sValue.AsINT();
    m_bDisableMovies = nDisableMovies;
    m_bDisableIntroMovies = 0;

    m_pClientOptions->SetDefaultOptions();
    if (!m_pClientOptions->LoadOptions())
        m_pClientOptions->SetDefaultOptions();
    g_pAurora->ResetGammaRamp(g_pAppManager->m_pClientExoApp->GetClientOptions()->GetGammaSetting(), 0);
    ReadAndSetVideoMode(nDisableMovies == 0, 1);

    {
        int bRead = ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Graphics Options"),
                                     CExoString("V-Sync"));
        if (bRead && g_pAppManager->m_pClientExoApp->GetClientOptions())
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetVSync(sValue.AsINT());
    }

    {
        int bRead = ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Graphics Options"),
                                     CExoString("Anisotropy"));
        if (bRead && g_pAppManager->m_pClientExoApp->GetClientOptions())
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetAnisotropy(sValue.AsINT());
    }

    int bReadAA = ini.ReadIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Graphics Options"),
                                   CExoString("Anti Aliasing"));
    if (bReadAA && g_pAppManager->m_pClientExoApp->GetClientOptions()) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetAntiAlias(static_cast<unsigned char>(sValue.AsINT()));
        g_is_antialias_on = sValue.AsINT() > 0;
        if (sValue.AsINT() >= 1)
            ReInitAurora(g_nScreenWidth, g_nScreenHeight, g_nBitsPerPixel,
                         g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFullScreen, 1);
    }

    m_clientName = g_pTlkTable->GetSimpleString(0);
    int nLang = g_pTlkTable->GetLanguageVersion();
    m_clientLanguage = nLang;
    if (nLang == 5) {
#ifndef _WIN32   // category 0 is LC_ALL in the Windows CRT, which knows the name: decimal commas
        setlocale(0, "Polish_Poland.1250");
#endif
        nLang = m_clientLanguage;
    }
    if (999 < nLang) {
        m_bClientImeUI = 1;
        m_pImeUI = new CImeUI;
    }

    if (m_pQuestJournal == 0)
        m_pQuestJournal = new CSWCJournal;

    m_pSoundSetList = new CSoundSetList;
    LoadItemDropSoundList();
    InitializeClientOptions();

    m_pSWGuiManager = new CSWGuiManager;
    m_pSWGuiManager->SetSize(g_nScreenWidth, g_nScreenHeight);
    if (g_pExoInput) {
        g_pExoInput->SetMousePos(g_nScreenWidth / 2, g_nScreenHeight / 2, 0);
        if (g_pExoInput->GetMouseMode() == 1)
            m_pSWGuiManager->ActivateSoftwareMouse();
    }
    SetMouseCursor(MOUSECURSOR_DEFAULT, 0, 0);
    m_pSWGuiManager->LoadGuiSounds();

    m_pInGameGui = new CGuiInGame;
    m_pMoviePlayer = 0;
    m_pMoviePlayer = new CExoMoviePlayer;
    m_pMoviePlayer->Initialize(0);
#ifdef K2_QOL_CONTROLLER
    K2QolLoadRumblePatterns(m_rumblePatternList);
#endif

    return 0;
}
#else
int CClientExoAppInternal::StartServices(int a, const CExoString& b, const CExoString& sLoadMusic,
                                         const CExoString& d, uint64_t e)
{
    K2_UNUSED(b); K2_UNUSED(d); K2_UNUSED(e);
    CExoIni    ini;
    CExoString sValue;
    CExoString sDialogFile;

    m_pSysAdminMessenger = new CClientMsgSysAdmin;
    m_pWorldTimer        = new CWorldTimer;
    m_pExclusionTimer           = new CWorldTimer;
    m_pPauseTimer        = new CWorldTimer;
    m_pAnimationTimer    = new CWorldTimer;
    m_pGameObjectArray   = new CGameObjectArray(1);
    g_pExoResMan         = new CExoResMan;
    g_ExoFileThread      = new CExoFileThread;

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                         CExoString("AllowWindowedMode"))) {
        g_bAllowWindowedMode = sValue.AsINT() != 0;
    }

    g_pAurora = NewCAurora();

    if (m_pWorldJournal == 0)
        m_pWorldJournal = new CSWCWorldJournal;

    m_pNetLayer = new CNetLayer;

    if (m_pServerInfo == 0)
        m_pServerInfo = new CServerInfo;

    m_pSWCMessage = new CSWCMessage;

    m_pClientAIMaster = new CClientAIMaster;
    m_pClientAIMaster->SetExoAppInternal(this);

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Game Options"),
                         CExoString("EnableReleaseLogging"))) {
        CExoDebug* pDebug = g_pExoBase->m_pDebug;
        pDebug->m_bEnableReleaseLogging = sValue.AsINT() != 0;
    }

    g_bExtendedResourceTracking = 0;
    g_pExoBase->m_pDebug->OpenLogFiles(CExoString("swc"));

    g_pExoResMan->AddResourceDirectory(CExoString("TEMPCLIENT:"));
    g_pExoResMan->AddResourceDirectory(CExoString("OVERRIDE:"));

    char pBuf[200];
    for (int i = 0; ; ++i) {
        const char* pPath = GetOverridePath(i, 0);
        if (pPath == 0)
            break;
        sprintf_s(pBuf, sizeof(pBuf), "%s\\override", pPath);
        g_pExoResMan->AddResourceDirectory(CExoString(pBuf));
    }

    g_pTlkTable = g_pSWTlkTable = new CSWTlkTable;

    if (a == 0) {
        g_pExoResMan->CacheResource(CExoString("HD0:DATAXBOX\\templates"), RESTYPE_BIF);
        g_pExoResMan->CacheResource(CExoString("HD0:DATAXBOX\\gui"), RESTYPE_BIF);
    }

    g_pExoResMan->AddFixedKeyTableFile(CExoString("HD0:CHITIN"));
    g_pExoResMan->AddResourceDirectory(CExoString("RIMS:"));
    g_pAuroraInterface = new CAuroraInterface;
    g_pExoResMan->AddEncapsulatedResourceFile(CExoString("HD0:patch"), 1);
    InitializeSoundOptions();
    g_pExoResMan->AddResourceDirectory(CExoString("HD0:MOVIES"));
    if (g_pExoResMan->Exists(CResRef("GLOBAL"), RESTYPE_RIM, static_cast<unsigned int*>(0)))
        g_pExoResMan->AddResourceImageFile(CExoString("RIMS:GLOBAL"));

    if (a != 0) {
        m_loadMusic = sLoadMusic;
        StartLoadMusic();
    }

    char pDlgBuf[200];
    {
        int bOpened = 0;
        for (int i = 0; ; ++i) {
            const char* pDlg = GetOverridePath(i, 1);
            if (pDlg == 0)
                break;
            sprintf_s(pDlgBuf, sizeof(pDlgBuf), "%s\\dialog", pDlg);
            sDialogFile = g_pExoResMan->CacheResource(CExoString(pDlgBuf), RESTYPE_TLK);
            int nRet = g_pTlkTable->OpenFile(sDialogFile);
            bOpened |= (nRet == 1);
            if (nRet == 1)
                break;
        }
        if (!bOpened) {
            sDialogFile = g_pExoResMan->CacheResource(CExoString("HD0:DIALOG"), RESTYPE_TLK);
            g_pTlkTable->OpenFile(sDialogFile);
        }
    }

    AddDownloadedResources();
    m_pNetLayer->Initialize(reinterpret_cast<CBaseExoApp*>(g_pAppManager->m_pClientExoApp));
    srand_wincompatible(GetTickCount());
    m_pModule = new CSWCModule;

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                         CExoString("Texture Quality"))) {
        int nPack = sValue.AsINT();
        if (nPack <= 3) {
            SetTexturePack(static_cast<unsigned char>(nPack), 0);
            goto tex_done;
        }
    }
    SetTexturePack(2, 0);
tex_done:;

    {
        WIN32_FIND_DATAA findData = {};
        void* hFind = FindFirstFileA("d:\\guilocal.erf", &findData);
        if (reinterpret_cast<intptr_t>(hFind) != -1) {
            g_pExoResMan->AddEncapsulatedResourceFile(
                g_pExoResMan->CacheResource(CExoString("HD0:guilocal"), RESTYPE_ERF), 2);
        }
        FindClose(hFind);
    }

    if (!ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                          CExoString("FullScreen"))) {
        sValue.Format(const_cast<char*>("%d"), 1);
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                          CExoString("FullScreen"));
    }

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Game Options"),
                         CExoString("Disable Movies"))) {
        m_bDisableMovies = sValue.AsINT();
    } else {
        m_bDisableMovies = 0;
    }

    if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Display Options"),
                         CExoString("Disable Intro Movies"))) {
        m_bDisableIntroMovies = sValue.AsINT();
        if (m_bDisableIntroMovies != 0)
            goto intro_done;
        sValue.Format(const_cast<char*>("%d"), 1);
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Display Options"),
                          CExoString("Disable Intro Movies"));
    } else {
        m_bDisableIntroMovies = 0;
        sValue.Format(const_cast<char*>("%d"), 1);
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Display Options"),
                          CExoString("Disable Intro Movies"));
    }
intro_done:;

    m_pClientOptions->SetDefaultOptions();
#if KOTOR1
    if (!m_pClientOptions->LoadOptions())
#else
    if (!m_pClientOptions->LoadOptions(0))
#endif
        m_pClientOptions->SetDefaultOptions();
    g_pAurora->ResetGammaRamp(g_pAppManager->m_pClientExoApp->GetClientOptions()->GetGammaSetting(), 0);
    ReadAndSetVideoMode(static_cast<int>(a), 0);

    m_clientName = g_pTlkTable->GetSimpleString(0);
    int nLang = g_pTlkTable->GetLanguageVersion();
    m_clientLanguage = nLang;
    if (nLang == 5) {
#ifndef _WIN32   // category 0 is LC_ALL in the Windows CRT, which knows the name: decimal commas
        setlocale(0, "Polish_Poland.1250");
#endif
        nLang = m_clientLanguage;
    }
    if (999 < nLang) {
        m_bClientImeUI = 1;
        m_pImeUI = new CImeUI;
    }

    if (m_pQuestJournal == 0)
        m_pQuestJournal = new CSWCJournal;

    m_pSoundSetList = new CSoundSetList;
    LoadItemDropSoundList();
    InitializeClientOptions();

    m_pSWGuiManager = new CSWGuiManager;
    m_pSWGuiManager->SetSize(g_nScreenWidth, g_nScreenHeight);
    if (g_pExoInput) {
        g_pExoInput->SetMousePos(g_nScreenWidth / 2, g_nScreenHeight / 2, 0);
        if (g_pExoInput->GetMouseMode() == 1)
            m_pSWGuiManager->ActivateSoftwareMouse();
    }
    SetMouseCursor(MOUSECURSOR_DEFAULT, 0, 0);
    m_pSWGuiManager->LoadGuiSounds();

    m_pInGameGui = new CGuiInGame;
    m_pMoviePlayer = 0;
    m_pMoviePlayer = new CExoMoviePlayer;
    m_pMoviePlayer->Initialize(0);

#ifdef K2_QOL_CONTROLLER
    K2QolLoadRumblePatterns(m_rumblePatternList);
#endif

    return 0;
}
#endif

void CClientExoAppInternal::StopServices()
{
#if !KOTOR1
    UnloadSuperModelSet();
#endif
    ShutDownMoviePlayer();

    if (m_pSWGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pMainMenuPanel)))
        delete m_pMainMenuPanel;
    m_pMainMenuPanel = 0;

    if (m_pStartScreen) {
        delete static_cast<CSWGuiStartScreen*>(m_pStartScreen);
        m_pStartScreen = 0;
    }

    if (m_pSoundSetList) {
        delete m_pSoundSetList;
        m_pSoundSetList = 0;
    }

    if (m_pItemDropSounds) {
        delete[] m_pItemDropSounds;
        m_pItemDropSounds = 0;
    }

    if (m_pModule) {
        delete m_pModule;
        m_pModule = 0;
    }

    if (m_pGameObjectArray) {
        CGameObjectArray* pGameObjectArray = m_pGameObjectArray;
        pGameObjectArray->~CGameObjectArray();
        operator delete(pGameObjectArray);
        m_pGameObjectArray = 0;
    }

    if (m_pWorldJournal) {
        delete m_pWorldJournal;
        m_pWorldJournal = 0;
    }

    if (m_pImeUI) {
        delete m_pImeUI;
        m_pImeUI = 0;
    }

    if (m_pInGameGui) {
        m_pInGameGui->ShutDown();
        delete m_pInGameGui;
        m_pInGameGui = 0;
    }

    if (m_pSWGuiManager) {
        delete m_pSWGuiManager;
        m_pSWGuiManager = 0;
#if KOTOR1  // the global GUI manager pointer is cleared too
        g_pGuiMan = 0;
#endif
    }

    if (m_pClientAIMaster) {
        delete m_pClientAIMaster;
        m_pClientAIMaster = 0;
    }

    while (true) {
        if (m_playerList.m_pInternal->m_pHead == 0)
            break;
        CSWCPlayer* pPlayer = static_cast<CSWCPlayer*>(m_playerList.m_pInternal->RemoveHead());
        if (pPlayer == 0)
            continue;
        pPlayer->CSWCPlayer::~CSWCPlayer();   // a non-virtual destructor; nothing derives from it
        operator delete(pPlayer);
    }

    if (m_pServerInfo) {
        delete m_pServerInfo;
        m_pServerInfo = 0;
    }

    if (m_pNetLayer) {
        m_pNetLayer->ShutDown();
        delete m_pNetLayer;
        m_pNetLayer = 0;
    }

    if (g_pExoInput) {
        delete g_pExoInput;
        g_pExoInput = 0;
    }

    if (m_pMenuMusic) {
        delete m_pMenuMusic;
        m_pMenuMusic = 0;
    }

    if (m_pLoadMusic) {
        delete m_pLoadMusic;
        m_pLoadMusic = 0;
    }

#if !KOTOR1  // the sound system is not deleted here
    if (g_pExoSound) {
        delete g_pExoSound;
        g_pExoSound = 0;
    }
#endif

    if (m_pSWCMessage) {
        delete m_pSWCMessage;
        m_pSWCMessage = 0;
    }

    if (CTlkTable* pTlkTable = g_pTlkTable) {
        // CTlkTable's destructor is not virtual, so the CSWTlkTable is torn down as its base.
        pTlkTable->CTlkTable::~CTlkTable();
        operator delete(pTlkTable);
        g_pTlkTable = 0;
    }

    if (m_pSoundSetList) {
        delete m_pSoundSetList;
        m_pSoundSetList = 0;
    }

    if (g_pAuroraInterface) {
        delete g_pAuroraInterface;
        g_pAuroraInterface = 0;
    }

    if (g_pAurora) {
        DestroyCAurora();
        g_pAurora = 0;
    }

    if (g_pExoResMan) {
        g_pExoResMan->RemoveResourceDirectory(CExoString("PORTRAITS:"));
        g_pExoResMan->RemoveResourceDirectory(CExoString("SERVERVAULT:"));
        g_pExoResMan->RemoveFixedKeyTableFile(CExoString("HD0:CHITIN"));
        g_pExoResMan->RemoveResourceDirectory(CExoString("OVERRIDE:"));
        g_pExoResMan->RemoveResourceDirectory(CExoString("HD0:MOVIES"));
        g_pExoResMan->RemoveResourceDirectory(CExoString("HD0:STREAMMUSIC"));
#if KOTOR1
        g_pExoResMan->RemoveResourceDirectory(CExoString("HD0:STREAMWAVES"));
#else
        g_pExoResMan->RemoveResourceDirectory(CExoString("HD0:STREAMVOICE"));
#endif
        g_pExoResMan->RemoveResourceDirectory(CExoString("TEMPCLIENT:"));
        if (g_pExoResMan->Exists(CResRef("GLOBAL"), RESTYPE_RIM, static_cast<unsigned int*>(0)))
            g_pExoResMan->RemoveResourceImageFile(CExoString("RIMS:GLOBAL"));
        g_pExoResMan->RemoveResourceDirectory(CExoString("RIMS:"));
        delete g_pExoResMan;
        g_pExoResMan = 0;
    }

    if (g_ExoFileThread) {
        delete g_ExoFileThread;
        g_ExoFileThread = 0;
    }

    if (m_pWorldTimer) {
        delete m_pWorldTimer;
        m_pWorldTimer = 0;
    }

    if (m_pExclusionTimer) {
        delete m_pExclusionTimer;
        m_pExclusionTimer = 0;
    }

    if (m_pPauseTimer) {
        delete m_pPauseTimer;
        m_pPauseTimer = 0;
    }

    if (m_pAnimationTimer) {
        delete m_pAnimationTimer;
        m_pAnimationTimer = 0;
    }

    if (m_pClientOptions) {
        delete m_pClientOptions;
        m_pClientOptions = 0;
    }

    if (m_pSysAdminMessenger) {
        delete m_pSysAdminMessenger;
        m_pSysAdminMessenger = 0;
    }

    if (m_pPlayerControl) {
        // Torn down as the base, whose destructor is not virtual.
        m_pPlayerControl->CSWPlayerControl::~CSWPlayerControl();
        operator delete(m_pPlayerControl);
    }

    DestroyQuestJournal();

    ResetCharacterFile();
}


extern float DeltaT;
int SlowGame;
extern unsigned char g_ForceSight;
int g_bEnableScreenCapture = 1;
float g_AIUpdateTime;
float g_GuiUpdateTime;
float g_SndUpdateTime;
void AurPostString(char* pszMessage, int a, int b, float f);
#if KOTOR1
void AurSnapshot(char* pBuffer, bool bGui);  // K1: whether the GUI is captured too
extern CExoString g_sErrorString;  // K1: the pending file read/write error text
#else
void AurSnapshot(char* pBuffer);
#endif
const char* ClearLogCounters(char* a1);

#if !KOTOR1
static int g_nAnimMsgPos;
#endif
static unsigned long long g_starttime;

int CClientExoAppInternal::MainLoop()
{
    if (m_pInGameGui) {
        Vector vZero;
        m_pInGameGui->SetAreaTransition(OBJECT_INVALID, vZero);
    }
    bool bDone = false;
    if (m_bAreaNotReadyToDisplay != 0 && m_pModule) {
        if (GetPlayerCreature() != 0) {
            if (m_pModule->GetMiniGame() == 0
                && m_futureLeader != OBJECT_INVALID) {
                CrownFutureLeader();
                bDone = false;
            } else {
                if (m_pModule->GetMiniGame()) {
                    SetInputClass(INPUTCLASS_MINIGAME, 1);
                } else {
                    SetInputClass(INPUTCLASS_GAME, 1);
                    if (m_pPlayerControl)
                        m_pPlayerControl->SetEnabled(1);
                    if (m_pInGameGui)
                        m_pInGameGui->SetSWGuiStatus(CGuiInGame::SWGUI_TOGGLE_ON, 1);
                    m_pInGameGui->InitializeMap();
                }
                EnableInput();
                {
                    CExoString sName(m_pModule->m_sModuleDisplayName);
                    sName.CStr();
                }
#if KOTOR1  // no wait before the test, no mouse/resolution restore, no minigame gate
                if ((m_pMoviePlayer ? m_pMoviePlayer->IsMoviePlaying() : 0) | static_cast<int>(g_bRenderWindowActive))
                    HideLoadScreen(1, 1);
                if (m_pMoviePlayer && m_pMoviePlayer->IsMoviePlaying() == 1) {
                    WaitForMovie(1);
                } else {
                    m_moveToModuleMovies.m_count = 0;
                }
                m_pMoviePlayer->HideFadePanel();
                if (g_bRenderWindowActive == 0)
                    goto update_phase;
                HideLoadScreen(1, 1);
#else
                WaitForMovie(1);
                if ((m_pMoviePlayer ? m_pMoviePlayer->IsMoviePlaying() : 0) | g_bRenderWindowActive)
                    HideLoadScreen(1, 1, 1);
                if (m_pMoviePlayer && m_pMoviePlayer->IsMoviePlaying() == 1) {
                    WaitForMovie(1);
                } else {
                    m_moveToModuleMovies.m_count = 0;
                }
                if (m_pModule->GetMiniGame() == 0)
                    ShowMouse(MOUSEHIDE_MINIGAME);
                RestoreResolutionFromMovie();
                m_pMoviePlayer->HideFadePanel();
                if (g_bRenderWindowActive == 0 && m_pModule->GetMiniGame() == 0)
                    goto update_phase;
                HideLoadScreen(1, 1, 1);
#endif
                m_pExclusionTimer->PauseWorldTimer();
                m_pPauseTimer->PauseWorldTimer();
                m_pAnimationTimer->PauseWorldTimer();
                m_pExclusionTimer->ResetTimer(m_pWorldTimer);
                m_pPauseTimer->ResetTimer(m_pWorldTimer);
                m_pAnimationTimer->ResetTimer(m_pWorldTimer);
                m_pWorldTimer->SetSnapshotTime();
                m_pExclusionTimer->SetSnapshotTime();
                m_pPauseTimer->SetSnapshotTime();
                m_pAnimationTimer->SetSnapshotTime();
                m_pPauseTimer->ResetLastSnapshotTime();
                m_pAnimationTimer->ResetLastSnapshotTime();
                m_pExclusionTimer->ResetLastSnapshotTime();
                m_pWorldTimer->ResetLastSnapshotTime();
                m_pWorldTimer->UnpauseWorldTimer();
                m_pExclusionTimer->UnpauseWorldTimer();
                m_pPauseTimer->UnpauseWorldTimer();
                m_pAnimationTimer->UnpauseWorldTimer();
                g_pAppManager->m_pServerExoApp->GetWorldTimer()->SetSnapshotTime();
                g_pAppManager->m_pServerExoApp->GetWorldTimer()->UnpauseWorldTimer();
                if (g_pAppManager->m_pServerExoApp->GetAutoSavePending() == 0) {
                    m_pModule->m_pArea->StartSounds();
                    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0
                        || g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog != 0) {
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog = 1;
                    } else {
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(
                            1, 0.5f, 1.0f, Vector(0.0f, 0.0f, 0.0f));
                    }
                    SetCanSendDialog(1);
                    g_pExoInput->EndAttemptedBind();
                    m_pInGameGui->SetGlobalUnpauseableState(0);
                    g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
                }
                if (m_inputClass == INPUTCLASS_MINIGAME) {
                    if (GetPlayerCreature()) {
                        GetPlayerCreature()->AttachToScene(static_cast<CAurScene*>(0));
                        m_sceneCreatureId = GetPlayerCreature()->m_objectId;
                        m_playerCreatureId = OBJECT_INVALID;
                        if (m_pParty)
                            m_pParty->AddCharacter(OBJECT_INVALID);
                    }
                }
                ::ClearLogCounters(static_cast<char*>(0));
                bDone = true;
            }
        }
    }

update_phase:
    if (m_pauseFlags & 3) {
        if (!(m_pauseFlags & 2))
            m_pExclusionTimer->SetSnapshotTime();
        m_pPauseTimer->SetSnapshotTime();
    } else {
        m_pWorldTimer->SetSnapshotTime();
        if (!(m_pauseFlags & 2))
            m_pExclusionTimer->SetSnapshotTime();
        m_pPauseTimer->SetSnapshotTime();
    }
    if (g_pAppManager->m_pLoadGameErrorInfo->m_flag == 1) {
        if (!g_pAppManager->m_pClientExoApp->GetCaptureEventPlayBack()) {
            m_pNetLayer->ProcessReceivedFrames(1);
            m_pNetLayer->UpdateStatusLoop(CNetLayer::APPLICATION_CLIENT);
        }
        StallServerCode();
#if KOTOR1  // a load still in progress keeps the area hidden and ends the frame here
        CLoadGameErrorInfo* pInfo = g_pAppManager->m_pLoadGameErrorInfo;
        if (pInfo->m_flag != 0 && (pInfo->m_nStallEvent | 2) == 3) {
            m_bAreaNotReadyToDisplay = 1;
            UpdateScreen(0.033333335f, 0, 1);
            if (g_pAppManager->m_pLoadGameErrorInfo->m_errorCode == 4) {
                DeltaT = static_cast<float>(static_cast<int>(m_pWorldTimer->GetSnapshotTimeDifference())) / 1000000.0f;
                ProcessInput(DeltaT);
            }
            return m_bQuitClicked && m_bWMCLOSEQuery;
        }
#endif
    }
    DeltaT = static_cast<float>(static_cast<int>(m_pWorldTimer->GetSnapshotTimeDifference())) / 1000000.0f;
    g_pExoResMan->Update(0);
    if (m_pParty && m_pModule && m_pModule->m_pArea)
        m_pParty->Update(DeltaT);
    AurMakeCurrent();
    ProcessInput(DeltaT);
    UpdateDeathSequence();
    if (m_bEndGamePending != 0
        && g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bControllerPauseActive == 0) {
#if KOTOR1
        if (m_fEndGameDelay > 0.0f) {
            if (m_pSWGuiManager->PanelExists(m_pInGameGui->m_pSoloModeQueryBox) == 0
                && m_bDisplayEndGameGui)
                m_pInGameGui->m_pSoloModeQueryBox->DisplayDeathMessage();
            else
                m_fEndGameDelay -= DeltaT;
        } else if (m_nGameOverReason == 0) {
            m_pInGameGui->HideSWInGameGui(0);
            if (g_pAppManager->m_pServerExoApp)
                g_pAppManager->m_pServerExoApp->UnloadModule();
            g_pAppManager->DestroyServer();
#ifdef K2_FIX_ENGINE_BUGS
            int nMenuArg = 0;
#else
            int nMenuArg;
#endif
            DisplayMainMenu(nMenuArg);
            m_pInGameGui->ShutDown();
            m_bEndGamePending = 0;
            m_bDisplayEndGameGui = 1;
            m_fEndGameDelay = 0.0f;
#ifdef K2_FIX_ENGINE_BUGS
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
#endif
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
            m_bStartMenuMusicOnMovieEnd = 1;
            StartMenuMusic(1, 127, 2);
            m_pInGameGui->SetSWGuiStatus(CGuiInGame::SWGUI_TOGGLE_OFF, 1);
        }
#else
        if (m_fEndGameDelay > 0.0f) {
            if (m_pSWGuiManager->PanelExists(static_cast<CSWGuiPanel*>(m_pInGameGui->m_pDeathMessagePanel))
                    == 0
                && m_bDisplayEndGameGui)
                m_pInGameGui->DisplayDeathMessage();
        } else if (m_nGameOverReason == 0) {
            m_pInGameGui->RemoveDeathMessage(1);
            m_pInGameGui->HideSWInGameGui(0);
            if (g_pAppManager->m_pServerExoApp)
                g_pAppManager->m_pServerExoApp->UnloadModule();
            g_pAppManager->DestroyServer();
#ifdef K2_FIX_ENGINE_BUGS
            int nMenuArg = 0;
#else
            int nMenuArg;
#endif
            DisplayMainMenu(nMenuArg);
            m_pInGameGui->ShutDown();
            m_bEndGamePending = 0;
            m_bDisplayEndGameGui = 1;
            m_fEndGameDelay = 0.0f;
            g_pAppManager->m_pClientExoApp->DisableVideoEffect();
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
            m_bStartMenuMusicOnMovieEnd = 1;
            StartMenuMusic(nMenuArg, 127, 2);
            m_pInGameGui->SetSWGuiStatus(CGuiInGame::SWGUI_TOGGLE_OFF, 1);
        }
#endif
    }
    SetGameSpeed(SlowGame || m_bGuiGameSlow ? 0.25f : 1.0f);
    if (g_pAppManager->m_pClientExoApp->GetCaptureEventPlayBack() == 0) {
        m_pNetLayer->ProcessReceivedFrames(1);
        m_pNetLayer->UpdateStatusLoop(CNetLayer::APPLICATION_CLIENT);
    }
    if (m_bRunScriptSet) {
        g_pVirtualMachine->RunScript(&m_runScript, 0, 0);
        m_bRunScriptSet = 0;
    }
    CSWCCreature* pPlayer = GetPlayerCreature();
    if (pPlayer && pPlayer->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)) {
        if (!(m_fQueuedAnimDelay <= DeltaT)) {
            m_fQueuedAnimDelay -= DeltaT;
        } else {
            m_fQueuedAnimDelay = 0.0f;
            if (m_queuedAnims.m_pInternal->m_pHead) {
                CExoString sName;
                CQueuedAnim* pAnim;
                float fB, fA;
                Gob* pModel;
                pAnim = m_queuedAnims.RemoveHead();
                if (pAnim) {
                    if (pAnim->m_sName.GetLength() >= 1) {
                        pPlayer = GetPlayerCreature();
                        pModel = pPlayer->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
                        pModel->GetAnimation(static_cast<char*>(pAnim->m_sName.CStr()), &fB, &fA);
                    } else {
                        // An entry with an empty name is named by its animation index.
                        sName = GetPlayerCreature()->GetAnimationBase()->GetAnimationName(pAnim->m_nAnimation);
                        pPlayer = GetPlayerCreature();
                        pModel = pPlayer->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
                        pModel->GetAnimation(static_cast<char*>(sName.CStr()), &fB, &fA);
                    }
                    if (m_queuedAnims.m_pInternal->m_pHead && fA == -1.0f) {
                        do {
                            pAnim = m_queuedAnims.RemoveHead();
                            if (pAnim) {
                                pPlayer = GetPlayerCreature();
                                pModel = pPlayer->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
                                pModel->GetAnimation(static_cast<char*>(pAnim->m_sName.CStr()), &fB, &fA);
                            }
                        } while (m_queuedAnims.m_pInternal->m_pHead && fA == -1.0f);
                    }
                    if (pAnim) {
#if KOTOR1  // the queue message is not posted (only its name argument is read)
                        if (pAnim->m_sName.GetLength() >= 1)
                            pAnim->m_sName.CStr();
                        else
                            sName.CStr();
                        if (pAnim->m_sName.GetLength() >= 1) {
#else
                        {
                            CExoString sMsg;
                            if (pAnim->m_sName.GetLength() >= 1)
                                sMsg.Format(const_cast<char*>("Playing next anim in queue: %s"),
                                            pAnim->m_sName.CStr());
                            else
                                sMsg.Format(const_cast<char*>("Playing next anim in queue: %s"), sName.CStr());
                            AurPostString(static_cast<char*>(sMsg.CStr()), 0, (g_nAnimMsgPos++ % 5) + 3, 3.0f);
                        }
                        if (pAnim->m_sName.GetLength() > 0) {
#endif
                            if (fA > 0.0f) {
                                if (pAnim->m_nParam) {
                                    pPlayer = GetPlayerCreature();
                                    pPlayer->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->PlayAnimation(
                                        static_cast<char*>(pAnim->m_sName.CStr()), 1.0f, 0, 0.0f);
                                    m_fQueuedAnimDelay = pAnim->m_fParam;
                                } else {
                                    pPlayer = GetPlayerCreature();
                                    pPlayer->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->PlayAnimation(
                                        static_cast<char*>(pAnim->m_sName.CStr()), 1.0f, 1, 0.0f);
                                    m_fQueuedAnimDelay = fA;
                                }
                            }
                        } else {
                            if (GetPlayerCreature()->AnimationLooping(pAnim->m_nAnimation)) {
                                GetPlayerCreature()->SetAnimation(pAnim->m_nAnimation, 0, 0);
                                m_fQueuedAnimDelay = pAnim->m_fParam;
                            } else {
                                GetPlayerCreature()->AddFireForgetAnimation(pAnim->m_nAnimation, 0);
                                m_fQueuedAnimDelay =
                                    GetPlayerCreature()->GetAnimationBase()
                                        ->GetAnimationLength(pAnim->m_nAnimation) / 1000.0f;
                            }
                        }
                    }
                }
            } else {
#if !KOTOR1
                g_nAnimMsgPos = 0;
#endif
            }
        }
    }

    CSWCCreature* pChar = m_pParty->GetCharacter(0);
    CSWCArea* pCurArea = m_pModule ? m_pModule->m_pArea : 0;
    unsigned int nTimeA, nTimeB;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nTimeA, &nTimeB);
    if (pCurArea) {
        if (m_bStartSoundsAfterLoadBar != 0) {
            if (m_pLoadGameLoadBar == 0) {
                pCurArea->StartSounds();
                m_bStartSoundsAfterLoadBar = 0;
            }
        } else if (m_pLoadGameLoadBar != 0) {
            m_bStartSoundsAfterLoadBar = 1;
        }
        if (pChar && pCurArea->m_pScene) {
            if (m_pInGameGui->m_nGlobalDialogState)
                pCurArea->m_pScene->SetSceneFocus(m_pInGameGui->GetDialogCameraPosition(1));
            else
                pCurArea->m_pScene->SetSceneFocus(pChar->m_position);
        }
        int bPanelExists = m_pSWGuiManager->PanelExists(
                               static_cast<CSWGuiPanel*>(m_pInGameGui->m_pTutorialBox));
        if (!g_pAppManager->m_pClientExoApp->GetCaptureEventPlayBack() && !bPanelExists
            && m_inputClass != INPUTCLASS_MINIGAME)
            UpdateCamera(DeltaT);
        else if (bPanelExists) {
            m_pModule->TurnCamera(0.0f, 0.0f);
            m_pModule->AcclTurnCamera(0.0f, 0.0f);
        }
        g_starttime = g_pExoBase->m_pTimers->GetHighResolutionTimer();
#if !KOTOR1  // no Harbinger room update before the AI tick
        if (pCurArea->m_areaName.CStr()
            && pCurArea->m_areaName.CStr()[0] == 'H'
            && pCurArea->m_areaName == "Harbinger - Command Deck") {
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE
                && g_pAppManager->m_pClientExoApp->GetModule()
                && g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera) {
                UpdateRoom(g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetPosition());
            } else if (pChar) {
                if (m_pInGameGui->m_nGlobalDialogState)
                    UpdateRoom(m_pInGameGui->GetDialogCameraPosition(0));
                else
                    UpdateRoom(pChar->m_desiredPosition);
            }
        }
#endif
        m_pClientAIMaster->UpdateState();
        g_AIUpdateTime = static_cast<float>(static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - g_starttime))
                         * 0.001f;
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE
            && g_pAppManager->m_pClientExoApp->GetModule()
            && g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera) {
            UpdateRoom(g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetPosition());
        } else if (pChar) {
            if (m_pInGameGui->m_nGlobalDialogState)
                UpdateRoom(m_pInGameGui->GetDialogCameraPosition(0));
            else
                UpdateRoom(pChar->m_desiredPosition);
        }
    }
    if (m_pInGameGui) {
        unsigned long long nTargetId = m_pInGameGui->m_oidDialogOwner;
        if (nTargetId != OBJECT_INVALID
            && g_pAppManager->m_pServerExoApp) {
            unsigned long long nServerId =
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(nTargetId);
            CGameObject* pObj = g_pAppManager->m_pServerExoApp->GetGameObject(nServerId);
            if (pObj) {
                if (pObj->AsSWSObject()) {
                    pObj->AsSWSObject()->UpdateDialog();
                    if (pObj->AsSWSObject()->m_bDialogFinished != 0)
                        pObj->AsSWSObject()->DeleteDialog();
                }
            }
        }
        if (m_pInGameGui) {
            m_pInGameGui->UpdateDialog();
            m_pInGameGui->UpdateAreaTransitionGUI();
        }
    }
    if ((m_inputClass | 4) == 4 && m_bAreaNotReadyToDisplay == 0
        && m_pInGameGui->IsGlobalFading() == 0
        && m_pInGameGui->IsGlobalFadeObscuring() == 0) {
#if KOTOR1  // passive selection runs every other frame
        static bool s_bPassiveSelectionFrame;
#ifdef K2_FIX_ENGINE_BUGS
        // The skipped frame's time is carried into the next call, so its timers run in real time.
        static float s_fPassiveSelectionDelta;
        s_fPassiveSelectionDelta += DeltaT;
        if (s_bPassiveSelectionFrame)
        {
            DoPassiveSelection(s_fPassiveSelectionDelta);
            s_fPassiveSelectionDelta = 0.0f;
        }
#else
        if (s_bPassiveSelectionFrame)
            DoPassiveSelection(DeltaT);
#endif
        s_bPassiveSelectionFrame = !s_bPassiveSelectionFrame;
#else
        DoPassiveSelection(DeltaT);
#endif
        UpdateCombatMode();
    }
    UpdateHostileHilite(
        static_cast<float>(g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer
            ->GetSnapshotTimeDifference()) / 1000000.0f);
    g_starttime = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    if (m_pInGameGui->GetPendingStatusSummary() != 0 && m_inputClass == INPUTCLASS_GAME
        && m_bAreaNotReadyToDisplay == 0 && m_pInGameGui->m_bHoldFadeInForDialog == 0
        && m_pInGameGui->m_nGlobalDialogState == 0 && m_pInGameGui->IsGlobalFading() == 0
        && m_pInGameGui->IsGlobalFadeObscuring() == 0) {
        if (m_fStatusSummaryDelay == 0.0f)
            m_fStatusSummaryDelay = 0.25f;
        else if (DeltaT < 0.25f)
            m_fStatusSummaryDelay -= DeltaT;
        if (m_fStatusSummaryDelay <= 0.0f) {
            m_fStatusSummaryDelay = 0.0f;
            m_pInGameGui->ShowStatusSummary();
        }
    } else if (m_fStatusSummaryDelay != 0.0f) {
        m_fStatusSummaryDelay = 0.0f;
    }
    {
        m_pSWGuiManager->Update(DeltaT);
#if !KOTOR1
        unsigned char bForce = g_ForceSight;
        g_ForceSight = 0;
#endif
        m_pSWGuiManager->m_fDeltaTStore = DeltaT;
        DeltaT = static_cast<float>(g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer
                     ->GetSnapshotTimeDifference()) / 1000000.0f;
        if (m_fMouseShowTimer > 0.0f) {
            m_fMouseShowTimer -= DeltaT;
            if (m_fMouseShowTimer <= 0.0f)
                ShowMouse(0);
        }
        m_pSWGuiManager->Draw(DeltaT);
        if (m_pSWGuiManager->PanelExists(
                static_cast<CSWGuiPanel*>(m_pInGameGui->m_pMainInterface)) == 0)
            m_pInGameGui->RePopulateMainInterface();
        DeltaT = m_pSWGuiManager->m_fDeltaTStore;
#if !KOTOR1
        g_ForceSight = bForce;
#endif
        g_GuiUpdateTime = static_cast<float>(static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer()
                                       - g_starttime)) * 0.001f;
        g_starttime = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        UpdateRumble(DeltaT);
#if KOTOR1
        m_pInGameGui->UpdateControllerPauseState();
#else
        if (!(g_pAppManager->m_pServerExoApp
              && g_pAppManager->m_pServerExoApp->GetMoveToModulePending()
              && g_pAppManager->m_pServerExoApp))
            m_pInGameGui->UpdateControllerPauseState();
#endif
        if (m_attackMashTutorialTimer != 0) {
            unsigned int nElapsed = static_cast<unsigned int>(DeltaT * 1000.0f);
            m_attackMashTutorialTimer = m_attackMashTutorialTimer >= nElapsed
                                            ? m_attackMashTutorialTimer - nElapsed : 0;
        }
        if (g_pExoSound)
            UpdateSoundEngine();
        g_SndUpdateTime = static_cast<float>(static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer()
                                       - g_starttime)) * 0.001f;
        if (m_bScreenShotPending != 0 && g_bEnableScreenCapture) {
#if KOTOR1  // the GUI is captured when the option or the manager asks for it
            AurSnapshot(m_sScreenShotFile,
                        m_pClientOptions->m_bGUIsInScreenShot || m_pSWGuiManager->m_nCapturedPanelCount);
#else
            AurSnapshot(m_sScreenShotFile);
#endif
            m_bScreenShotPending = 0;
        }
    }
    if (bDone)
        m_bAreaNotReadyToDisplay = 0;
    if (m_fEnemyAutoPauseTimer > 0.0f) {
        m_fEnemyAutoPauseTimer -= DeltaT;
        if (m_fEnemyAutoPauseTimer <= 0.0f && m_combatMode == 0) {
#if !KOTOR1
            SetAutoPaused(1, PAUSE_MODE_ENEMY_SIGHTED);
#endif
            SetAutoPaused(1, m_nPendingAutoPauseReason);
            m_nPendingAutoPauseReason = PAUSE_MODE_NO_PENDING;
        }
#ifdef K2_FIX_ENGINE_BUGS
        // When the timer expires in combat, drop the parked level, or DoPassiveSelection never
        // sights an enemy again (it waits for 0xff) until the module unloads.
        else if (m_fEnemyAutoPauseTimer <= 0.0f)
            m_nPendingAutoPauseReason = PAUSE_MODE_NO_PENDING;
#endif
    } else if (m_fEnemyAutoPauseDelay > 0.0f) {
        m_fEnemyAutoPauseDelay -= DeltaT;
    }
    if (m_fPartyDownPauseCooldown > 0.0f)
        m_fPartyDownPauseCooldown -= DeltaT;
    if (m_bTogglePauseAtEndOfFrame != 0) {
        int bSkipPause = 0;
        if (m_pModule && m_pModule->m_pArea) {
            CSWSArea* pSrvArea = m_pModule->m_pArea->GetServerArea();
            if (pSrvArea && pSrvArea->m_bTransitionPending != 0)
                bSkipPause = 1;
        }
        if (!bSkipPause) {
            if ((g_pAppManager->m_pServerExoApp
                     ? g_pAppManager->m_pServerExoApp->GetPauseState(2)
                     : 0) != m_combatPauseRequest)
                m_pSWCMessage->SendPlayerToServerInput_TogglePauseRequest();
            m_pInGameGui->SetPaused(
                g_pAppManager->m_pServerExoApp
                    ? g_pAppManager->m_pServerExoApp->GetPauseState(2)
                    : 0,
                m_combatPauseMode);
        }
        m_bTogglePauseAtEndOfFrame = 0;
    }
    if (m_tutorialReason != TUTORIAL_NONE) {
        m_pInGameGui->ShowTutorialWindow(m_tutorialReason, m_tutorialObjA, m_tutorialObjB,
                                         m_tutorialTimerSpan);
        m_tutorialReason = TUTORIAL_NONE;
    }
#if KOTOR1
    if (!g_sErrorString.IsEmpty()) {
        printf("ERROR_TO_BE_LOGGED: %s\n", g_sErrorString.CStr());
        g_sErrorString = "";
        CExoFile file(CExoString("HD0:FILEERROR"), RESTYPE_LOG, CExoString("a"));
        if (file.FileOpened()) {
            if (file.GetSize() == 0) {
                file.Write("Star Wars Knights of the Old Republic Patch 2 Error Logging File\n");
                file.Write("This file has been generated to help provide information on read/write "
                           "irregularities\n");
            }
            char szDate[16];
            _strdate(szDate);
            g_sErrorString.Format(const_cast<char*>("\n%s\n%s"), szDate, g_sErrorString.CStr());
            file.Write(g_sErrorString);
            g_sErrorString = "";
        } else {
            g_sErrorString.Format(const_cast<char*>("%sERROR - Failed to open error file\n"));
        }
    }
#else
    if (m_pMainMenuPanel)
        m_pMainMenuPanel->UpdateFacebookButtonStatus();
#endif
    return m_bQuitClicked && m_bWMCLOSEQuery;
}

namespace MSG_Mac { void CaptureMouse(bool bCapture); }

int CClientExoAppInternal::GetCanSendDialog(unsigned long long nSenderId,
                                            unsigned long long nListenerId, CSWSDialog* pDialog)
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nListenerId);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
#if KOTOR1  // no SetKeepStealthInDialog
        pObject->AsSWSCreature()->SetActivity(1, 0, 1);
#else
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bKeepStealthInDialog == 0)
            pObject->AsSWSCreature()->SetActivity(1, 0, 1);
#endif
    }
    pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nSenderId);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
#if KOTOR1  // no SetKeepStealthInDialog
        pObject->AsSWSCreature()->SetActivity(1, 0, 1);
#else
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bKeepStealthInDialog == 0)
            pObject->AsSWSCreature()->SetActivity(1, 0, 1);
#endif
    }

    // Pause and dialog-state gates.
    if (g_pAppManager->m_pServerExoApp != 0
        && g_pAppManager->m_pServerExoApp->GetPauseState(2) != 0)
        return 0;
    if (m_bAutoPaused)
        return 0;
    if (m_bDialogSent)
        return 1;
    if (m_bCanSendDialog == 0)
        return 0;
    if (m_nCanSendDialogTime == 0)
        return 0;

    unsigned int nNow =
        static_cast<unsigned int>(g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer
                           ->GetSnapshotTime() /
                       1000);
    if (static_cast<unsigned int>(nNow - m_nCanSendDialogTime) < 1001)
        return 0;
    m_nCanSendDialogTime = nNow - 2000;

    if (nSenderId != OBJECT_INVALID) {
        unsigned long long nClientId =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(nSenderId);
        CGameObject* pClient = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
        if (pClient == 0 || pClient->AsSWCObject() == 0)
            return 0;
    }
    if (nListenerId != OBJECT_INVALID) {
        unsigned long long nClientId =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(nListenerId);
        CGameObject* pClient = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
        if (pClient == 0 || pClient->AsSWCObject() == 0)
            return 0;
    }

    if (m_bDialogModelsSent) {
        m_bDialogSent = 1;
        return 1;
    }

    // Wait until every dialog model resolves before shipping the entries.
    for (int i = 0; i < static_cast<int>(pDialog->m_nModelCount); ++i) {
        int nFlag;
        unsigned long long nModelId = pDialog->GetDialogModel(i, nFlag);
        if (nFlag != 0)
            continue;
        CGameObject* pModel = g_pAppManager->m_pClientExoApp->GetGameObject(nModelId);
        if (pModel == 0 || pModel->AsSWCObject() == 0)
            return 0;
    }

    // Ship the models to the speaking player's client object.
    unsigned long long nPlayerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(nPlayerId);
    CGameObject* pOwner = g_pAppManager->m_pServerExoApp->GetGameObject(pDialog->m_ownerId);
    if (pOwner == 0 || pOwner->AsSWSObject() == 0)
        return 0;
    pDialog->SendDialogModels(pOwner->AsSWSObject(), pPlayer->m_nPlayerId);
    if (m_bDialogModelsSent) {
        m_bDialogSent = 1;
        return 1;
    }
    return 0;
}


void CClientExoAppInternal::AddFeedbackMessage(unsigned short nMsg, CSWCCMessageData* pData,
                                               const CExoString& sMsg)
{
    CExoString sObjectName;
    CExoString sMessage = "";
    CGuiInGame* pInGameGui = m_pInGameGui;
    CSWCCreature* pPlayer = GetPlayerCreature();
    int bCombat = 0;
    unsigned long long oid;

    switch (nMsg) {
    case FEEDBACK_CANNOT_USE_SKILL:
        sMessage = GetGUIString(STRREF_CANNOT_USE_SKILL);
        break;
    case FEEDBACK_SKILL_NOT_READY: {
        int nInt0 = pData->GetInteger(0);
        g_pTlkTable->SetCustomToken(0, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(STRREF_SKILL_NOT_READY, 0, 0, 0);
        break;
    }
    case 2:
        sMessage = GetGUIString(0);
        break;
    case 3:
        sMessage = GetGUIString(0);
        break;
    case 4: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, pData->GetString(0));
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case 5: {
        int nInt0;
        oid = pData->GetObjectID(0);
        nInt0 = pData->GetInteger(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(1, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(1, sObjectName);
        }
        g_pTlkTable->SetCustomToken(0, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_TARGET_MUST_BE_AWARE:
        sMessage = GetGUIString(STRREF_TARGET_MUST_BE_AWARE);
        break;
    case FEEDBACK_ACTION_NOT_ALLOWED_IN_STATE:
        sMessage = GetGUIString(STRREF_ACTION_NOT_ALLOWED_IN_STATE);
        break;
    case 8:
        sMessage = GetGUIString(0);
        break;
    case 9:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_LEVEL_UP_AVAILABLE:
        sMessage = GetGUIString(STRREF_LEVEL_UP_AVAILABLE);
        break;
    case 12:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_OBJECT_LOCKED:
    {
        // locked door / placeable: play its "Locked" sound at the object, stamp the object's
        // feedback text, then post the generic "locked" line.
        oid = pData->GetObjectID(0);
        CExoString sSound;
        CSWCDoor* pDoor;
        CSWCPlaceable* pPlaceable;
        int nSoundAppType;
        pDoor = g_pAppManager->m_pClientExoApp->GetDoorByGameObjectID(oid);
        pPlaceable = g_pAppManager->m_pClientExoApp->GetPlaceableByGameObjectID(oid);
        if (pDoor != 0) {
            if (pDoor->m_nAppearance != 0)
                g_pRules->m_p2DArrays->m_pDoorTypes->GetINTEntry(
                    pDoor->m_nAppearance, CTwoDimArrays::COL2DA_DOORTYPES_SOUNDAPPTYPE, &nSoundAppType);
            else
                g_pRules->m_p2DArrays->m_pGenericDoors->GetINTEntry(
                    pDoor->m_nGenericType, CTwoDimArrays::COL2DA_GENERICDOORS_SOUNDAPPTYPE, &nSoundAppType);
            g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                nSoundAppType, CTwoDimArrays::COL2DA_PLACEABLESOUNDS_LOCKED, &sSound);
            if (sSound.GetLength() > 0)
                g_pExoSound->Play3DOneShotSound(CResRef(sSound), pDoor->m_position, 1.5f,
                                                PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL, 0, 0, 0.0f, 0.0f);
            pDoor->SetFeedbackInfo(GetGUIString(STRREF_LOCKED));
        } else if (pPlaceable != 0) {
            g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(
                pPlaceable->m_nAppearanceType, CTwoDimArrays::COL2DA_PLACEABLES_SOUNDAPPTYPE, &nSoundAppType);
            g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                nSoundAppType, CTwoDimArrays::COL2DA_PLACEABLESOUNDS_LOCKED, &sSound);
            if (sSound.GetLength() > 0)
                g_pExoSound->Play3DOneShotSound(CResRef(sSound), pPlaceable->m_position, 0.0f,
                                                PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL,
                                                0, 0, 0.0f, 0.0f);
            pPlaceable->SetFeedbackInfo(GetGUIString(STRREF_LOCKED));
        }
        sMessage = GetGUIString(STRREF_OBJECT_LOCKED);
        break;
    }
    case FEEDBACK_OBJECT_NOT_LOCKED:
        sMessage = GetGUIString(STRREF_OBJECT_NOT_LOCKED);
        break;
    case FEEDBACK_CANNOT_OPEN_CONVENTIONALLY:
        sMessage = GetGUIString(STRREF_CANNOT_OPEN_CONVENTIONALLY);
        break;
    case FEEDBACK_USED_KEY:
        sMessage = GetGUIString(STRREF_USED_KEY);
        break;
    case FEEDBACK_REST_EXCITED:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_RESTING:
        sMessage = GetGUIString(0);
        break;
    case 19:
        sMessage = GetGUIString(0);
        break;
    case 20:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_CANNOT_TARGET_UNSEEN:
        sMessage = GetGUIString(STRREF_CANNOT_TARGET_UNSEEN);
        break;
    case 22:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_CANNOT_USE_ITEM:
        sMessage = GetGUIString(STRREF_CANNOT_USE_ITEM);
        break;
    case FEEDBACK_PARTY_NEW_LEADER: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_NOW_PARTY_LEADER, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_MEMBER_KICKED: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_KICKED_YOU:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_PARTY_INVITATION_PENDING: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0)
            g_pTlkTable->SetCustomToken(0, sObjectName);
        else
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);  // no token substitution on this path
#endif
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_INVITEE_INVITING: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_SENT_INVITATION: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_RECEIVED_INVITATION: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_JOINED: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_JOINED_PARTY, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_INVITATION_IGNORED: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_YOU_IGNORED_INVITATION: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_INVITATION_REJECTED: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_YOU_REJECTED_INVITATION: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_INVITATION_EXPIRED: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_MEMBER_LEFT: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_LEFT_PARTY, 0, 0, 0);
        break;
    }
    case FEEDBACK_PARTY_YOU_LEFT:
        sMessage = GetGUIString(0);
        break;
    case 46: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case 47: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, pData->GetString(0));
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case 48: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_ITEM_ACQUIRED:
        g_pTlkTable->SetCustomToken(0, pData->GetString(0));
        sMessage = g_pSWTlkTable->Fetch(STRREF_ACQUIRED_ITEM, 0, 0, 0);
        break;
    case FEEDBACK_ITEM_LOST:
        g_pTlkTable->SetCustomToken(0, pData->GetString(0));
        sMessage = g_pSWTlkTable->Fetch(STRREF_LOST_ITEM, 0, 0, 0);
        break;
    case 52:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_REST_NOT_ALLOWED_IN_AREA:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_HEAL_NO_AILMENT:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_HEAL_TARGET_UNHURT:
        sMessage = GetGUIString(0);
        break;
    case 57:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_OBJECT_TOO_FAR_TO_TALK:
        sMessage = GetGUIString(STRREF_OBJECT_TOO_FAR_TO_TALK);
        break;
    case FEEDBACK_OBJECT_BUSY_CANNOT_TALK:
        sMessage = GetGUIString(STRREF_OBJECT_BUSY_CANNOT_TALK);
        break;
    case FEEDBACK_STEALTH_IN_COMBAT:
        sMessage = GetGUIString(STRREF_NO_STEALTH_IN_COMBAT);
        break;
    case 61:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_DAMAGE_IMMUNITY:
    {
        // damage-immunity feedback: the flag word's lowest set damage-type bit picks the type
        // name (token 1); the creature's name is token 0. Combat log.
        CExoString sDamageType;
        int nFlags;
        oid = pData->GetObjectID(0);
        pData->GetInteger(0);  // result discarded
        nFlags = pData->GetInteger(1);
        if (nFlags & (DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_SLASHING)) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_PHYSICAL, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_UNIVERSAL) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_UNIVERSAL, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_ACID) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ACID, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_COLD) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_COLD, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_LIGHT_SIDE) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_LIGHT_SIDE, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_ELECTRICAL) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ELECTRICAL, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_FIRE) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_FIRE, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_DARK_SIDE) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_DARK_SIDE, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_SONIC) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_SONIC, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_ION) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ION, 0, 0, 0);
        } else if (nFlags & DAMAGE_TYPE_BLASTER) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_ENERGY, 0, 0, 0);
        } else if (nFlags & DAMAGE_FLAG_POISON) {
            g_pTlkTable->SetCustomToken(0, CExoString(""));
            sDamageType = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_POISON, 0, 0, 0);
        }
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, sDamageType);
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_DAMAGE_RESISTED: {
        int nInt0;
        oid = pData->GetObjectID(0);
        nInt0 = pData->GetInteger(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(STRREF_RESISTS_DAMAGE, 0, 0, 0);
        break;
    }
    case FEEDBACK_DAMAGE_REDUCED: {
        int nInt0;
        oid = pData->GetObjectID(0);
        nInt0 = pData->GetInteger(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_REDUCTION_ABSORBS, 0, 0, 0);
        break;
    }
    case FEEDBACK_ENTANGLED_CANNOT_CAST:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_DAMAGE_RESISTED_REMAINING: {
        int nInt0;
        int nInt1;
        oid = pData->GetObjectID(0);
        nInt0 = pData->GetInteger(0);
        nInt1 = pData->GetInteger(1);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, CExoString(nInt0));
        g_pTlkTable->SetCustomToken(2, CExoString(nInt1));
        sMessage = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_RESISTANCE_ABSORBS_REMAINING, 0, 0, 0);
        break;
    }
    case FEEDBACK_DAMAGE_REDUCED_REMAINING: {
        int nInt0;
        int nInt1;
        oid = pData->GetObjectID(0);
        nInt0 = pData->GetInteger(0);
        nInt1 = pData->GetInteger(1);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, CExoString(nInt0));
        g_pTlkTable->SetCustomToken(2, CExoString(nInt1));
        sMessage = g_pSWTlkTable->Fetch(STRREF_DAMAGE_AMOUNT_REDUCTION_ABSORBS_REMAINING, 0, 0, 0);
        break;
    }
    case FEEDBACK_SPELL_IMMUNITY:
    {
        int nSpell;
        oid = pData->GetObjectID(0);
        nSpell = pData->GetInteger(0);
        CSWSpell* pSpell = g_pRules->GetSpell(nSpell);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, pSpell->GetSpellNameText());
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case 69: {
        int nInt0;
        oid = pData->GetObjectID(0);
        nInt0 = pData->GetInteger(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pSWTlkTable->SetCustomToken(1, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case 70: {
        int nInt0;
        int nInt1;
        oid = pData->GetObjectID(0);
        nInt0 = pData->GetInteger(0);
        nInt1 = pData->GetInteger(1);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pSWTlkTable->SetCustomToken(1, CExoString(nInt0));
        g_pSWTlkTable->SetCustomToken(2, CExoString(nInt1));
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case 71: {
        int nInt0;
        int nInt1;
        nInt0 = pData->GetInteger(0);
        nInt1 = pData->GetInteger(1);
        g_pTlkTable->SetCustomToken(0, CExoString(nInt1));
        g_pTlkTable->SetCustomToken(1, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case 72:
        if (pPlayer == 0) {
#if KOTOR1
            sObjectName = sMessage = GetGUIString(0);
#else
            sObjectName = sMessage = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else if (GetObjectName(pPlayer->m_objectId, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    case FEEDBACK_TRANSIT_STARTED:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_TRANSIT_ENDED:
        sMessage = GetGUIString(0);
        break;
    case 76:
        sMessage = GetGUIString(0);
        break;
    case 77:
        sMessage = GetGUIString(0);
        break;
    case 78:
        sMessage = GetGUIString(0);
        break;
    case 79:
        sMessage = GetGUIString(0);
        break;
    case 80:
        sMessage = GetGUIString(0);
        break;
    case 81:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_TRIGGERED_MINE:
        sMessage = GetGUIString(STRREF_TRIGGERED_MINE);
        break;
    case FEEDBACK_FLOATING_TEXT_STRREF: {
        int nInt0 = pData->GetInteger(0);
        sMessage = g_pSWTlkTable->Fetch(nInt0, 0, 0, 0);
        break;
    }
    case FEEDBACK_FLOATING_TEXT_STRING:
        sMessage = pData->GetString(0);
        break;
    case FEEDBACK_ITEM_DROPPED_INVENTORY_FULL:
        sMessage = GetGUIString(STRREF_ITEM_DROPPED_INVENTORY_FULL);
        break;
    case 97:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_EQUIP_LEVEL_TOO_LOW:
        sMessage = GetGUIString(STRREF_LEVEL_TOO_LOW_TO_EQUIP);
        break;
    case FEEDBACK_STORE_PLOT_ITEM:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_STORE_CONTAINER_NOT_EMPTY:
        sMessage = GetGUIString(0);
        break;
    case 101:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_STORE_NOT_ENOUGH_GOLD:
        sMessage = GetGUIString(0);
        break;
    case 103:
        sMessage = GetGUIString(0);
        break;
    case 106:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_EQUIP_FEAT_REQUIRED:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_ALIGNMENT_SHIFT:
    {
        int nLevel = pData->GetInteger(1);
        uint64_t nTierStrRef;
        if (nLevel > 9)
            nTierStrRef = STRREF_ALIGNMENT_A_GREAT_DEAL_CLOSER;
        else if (nLevel >= 5)
            nTierStrRef = STRREF_ALIGNMENT_MUCH_CLOSER;
        else
            nTierStrRef = STRREF_ALIGNMENT_CLOSER;
        CExoString sTier = GetGUIString(nTierStrRef);
        g_pTlkTable->SetCustomToken(0, sTier);
        CExoString sKind;
        if (pData->GetInteger(0) == 2)
            sKind = GetGUIString(STRREF_LIGHT_SIDE_OF_THE_FORCE);
        else
            sKind = GetGUIString(STRREF_DARK_SIDE_OF_THE_FORCE);
        g_pTlkTable->SetCustomToken(1, sKind);
        sMessage = g_pSWTlkTable->Fetch(STRREF_ALIGNMENT_MOVED, 0, 0, 0);
        break;
    }
    case FEEDBACK_CANNOT_DAMAGE_TARGET:
        sMessage = GetGUIString(STRREF_WEAPON_CANNOT_DAMAGE_TARGET);
        break;
    case 118:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_EQUIP_NOT_PROFICIENT:
        sMessage = GetGUIString(STRREF_LACK_PROFICIENCY_TO_EQUIP);
        break;
    case 120:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_WEAPON_EQUIPPED_ONE_HANDED:
        sMessage = GetGUIString(STRREF_EQUIPPED_ONE_HANDED);
        break;
    case FEEDBACK_WEAPON_EQUIPPED_TWO_HANDED:
        sMessage = GetGUIString(STRREF_EQUIPPED_TWO_HANDED);
        break;
    case FEEDBACK_EQUIPPED_ITEM_SWAPPED:
        sMessage = GetGUIString(STRREF_EQUIPPED_ITEM_SWAPPED);
        break;
    case 124:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_IMMUNE_DISEASE: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_DISEASE, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_CRITICAL_HITS: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_CRITICAL_HITS, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_DEATH: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_FEAR: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_FEAR, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_KNOCKDOWN: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_KNOCKDOWN, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_PARALYSIS: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_PARALYSIS, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_NEGATIVE_LEVELS: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_NEGATIVE_LEVELS, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_MIND_EFFECTS: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_MIND_EFFECTS, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_POISON: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_POISON, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_SNEAK_ATTACKS: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_SNEAK_ATTACKS, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_SLEEP: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_SLEEP, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_DAZE: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_CONFUSION: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_CONFUSION, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_STUN: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_STUN, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_BLINDNESS: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_DEAFNESS: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_CURSE: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_ATTRIBUTE_REDUCED:
    {
        oid = pData->GetObjectID(0);
        int nType = pData->GetInteger(0);
        int nAmount = pData->GetInteger(1);
        float fValue = pData->GetFloat(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        uint64_t nTypeStrRef = 0;
        switch (nType) {
            case ABILITY_STRENGTH: nTypeStrRef = STRREF_STRENGTH; break;
            case ABILITY_DEXTERITY: nTypeStrRef = STRREF_DEXTERITY; break;
            case ABILITY_CONSTITUTION: nTypeStrRef = STRREF_CONSTITUTION; break;
            case ABILITY_INTELLIGENCE: nTypeStrRef = STRREF_INTELLIGENCE; break;
            case ABILITY_WISDOM: nTypeStrRef = STRREF_WISDOM; break;
            case ABILITY_CHARISMA: nTypeStrRef = STRREF_CHARISMA; break;
        }
        CExoString sType = g_pSWTlkTable->Fetch(nTypeStrRef, 0, 0, 0);
        g_pTlkTable->SetCustomToken(1, sType);
        g_pTlkTable->SetCustomToken(2, CExoString(nAmount));
        g_pTlkTable->SetCustomToken(3, CExoString(static_cast<int>(fValue)));
        sMessage = g_pSWTlkTable->Fetch(STRREF_ATTRIBUTE_REDUCED, 0, 0, 0);
        break;
    }
    case FEEDBACK_EXPERIENCE_RECEIVED: {
        g_pTlkTable->SetCustomToken(0, CExoString(pData->GetInteger(0)));
        sMessage = g_pSWTlkTable->Fetch(STRREF_XP_RECEIVED, 0, 0, 0);
        break;
    }
    case FEEDBACK_IMMUNE_ENTANGLE: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_ENTANGLE, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_SILENCE: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_IMMUNE_SLOW: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_SLOW, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case 147:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_CREDITS_ACQUIRED: {
        int nInt0 = pData->GetInteger(0);
        g_pTlkTable->SetCustomToken(0, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(STRREF_ACQUIRED_CREDITS, 0, 0, 0);
        break;
    }
    case FEEDBACK_CREDITS_LOST: {
        int nInt0 = pData->GetInteger(0);
        g_pTlkTable->SetCustomToken(0, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(STRREF_LOST_CREDITS, 0, 0, 0);
        break;
    }
    case 150:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_HEALED: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, CExoString(pData->GetInteger(0)));
        sMessage = g_pSWTlkTable->Fetch(STRREF_HEALED_HIT_POINTS, 0, 0, 0);
        break;
    }
    case 152:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_SCRIPT_DEBUG_MESSAGE:
    {
        CExoString sToken1;
        CExoString sTag;
        int nLabel;
        oid = pData->GetObjectID(0);
        sToken1 = pData->GetString(0);
        sTag = pData->GetString(1);
        nLabel = pData->GetInteger(0);
        CExoString sLabel = GetGUIString(nLabel);
        CExoString sDebug;
        sDebug.Format(const_cast<char*>("OID: %08x, Tag: %s, %s"), oid, sTag.CStr(), sLabel.CStr());
        g_pTlkTable->SetCustomToken(0, sDebug);
        g_pTlkTable->SetCustomToken(1, sToken1);
        sMessage = g_pSWTlkTable->Fetch(STRREF_SCRIPT_MESSAGE, 0, 0, 0);
        break;
    }
    case FEEDBACK_ACTION_LIST_OVERFLOW: case FEEDBACK_ACTION_LIST_ERASED:
    {
        CExoString sUnused;  // constructed and destroyed, never read
        CExoString sTag;
        int nFrom;
        int nTo;
        oid = pData->GetObjectID(0);
        sTag = pData->GetString(1);
        nFrom = pData->GetInteger(0);
        nTo = pData->GetInteger(1);
        CExoString sDebug;
        sDebug.Format(const_cast<char*>("(%d > %d) OID: %08x, Tag: %s"), nFrom, nTo, oid, sTag.CStr());
        g_pTlkTable->SetCustomToken(0, sDebug);
        if (nMsg == FEEDBACK_ACTION_LIST_ERASED)
            sMessage = g_pSWTlkTable->Fetch(STRREF_ACTION_LIST_ERASED, 0, 0, 0);
        else
            sMessage = g_pSWTlkTable->Fetch(STRREF_ACTION_LIST_OVERFLOW, 0, 0, 0);
        sMessage.CStr();  // argument to the removed log call
        if (pData != 0)
            delete pData;
        return;
    }
    case FEEDBACK_EFFECT_LIST_OVERFLOW: case FEEDBACK_EFFECT_LIST_ERASED:
    {
        CExoString sUnused;
        CExoString sTag;
        int nFrom;
        int nTo;
        oid = pData->GetObjectID(0);
        sTag = pData->GetString(1);
        nFrom = pData->GetInteger(0);
        nTo = pData->GetInteger(1);
        CExoString sDebug;
        sDebug.Format(const_cast<char*>("(%d > %d) OID: %08x, Tag: %s"), nFrom, nTo, oid, sTag.CStr());
        g_pTlkTable->SetCustomToken(0, sDebug);
        if (nMsg == FEEDBACK_EFFECT_LIST_ERASED)
            sMessage = g_pSWTlkTable->Fetch(STRREF_EFFECT_LIST_ERASED, 0, 0, 0);
        else
            sMessage = g_pSWTlkTable->Fetch(STRREF_EFFECT_LIST_OVERFLOW, 0, 0, 0);
        sMessage.CStr();  // arguments to the removed log calls
        sMessage.CStr();
        sDebug.CStr();
        sMessage.CStr();
        sDebug.CStr();
        if (pData != 0)
            delete pData;
        return;
    }
    case FEEDBACK_AI_UPDATE_TIME_OVERFLOW:
    {
        // script-trace arm; same no-post exit as 161/164.
        CExoString sUnused;
        CExoString sTag;
        CExoString sScripts;
        int nFrom;
        int nTo;
        int nAction;
        int nFlags;
        oid = pData->GetObjectID(0);
        sTag = pData->GetString(1);
        sScripts = pData->GetString(2);
        nFrom = pData->GetInteger(0);
        nTo = pData->GetInteger(1);
        nAction = pData->GetInteger(2);
        nFlags = pData->GetInteger(3);
        CExoString sDebug;
        sDebug.Format(const_cast<char*>("(%d > %d) Scripts: (%s), Action: %d [Flags: %02x], OID: %08x, Tag: %s"),
                      nFrom, nTo, sScripts.CStr(), nAction, nFlags, oid, sTag.CStr());
        g_pTlkTable->SetCustomToken(0, sDebug);
        sMessage = g_pSWTlkTable->Fetch(STRREF_AI_UPDATE_TIME_OVERFLOW, 0, 0, 0);
        sMessage.CStr();  // argument to the removed log call
        if (pData != 0)
            delete pData;
        return;
    }
    case FEEDBACK_XP_GAINED:
    {
        int nValue = pData->GetInteger(0);
        CExoString sLabel = GetGUIString(STRREF_XP_GAINED);
        sMessage.Format(const_cast<char*>("%s %i"), sLabel.CStr(), nValue);
        break;
    }
    case FEEDBACK_XP_LOST:
    {
        int nValue = pData->GetInteger(0);
        CExoString sLabel = GetGUIString(STRREF_XP_LOST);
        sMessage.Format(const_cast<char*>("%s %i"), sLabel.CStr(), nValue);
        break;
    }
    case FEEDBACK_QUEST_ENTRY_ADDED:
    {
        // "quest entry added: <text>" -- the one arm whose strref is a named class constant
        CExoString sText = pData->GetString(0);
        CExoString sLabel = GetGUIString(CSWCJournal::STRREF_CSWCJOURNAL_QUESTENTRYADDED);
        sMessage.Format(const_cast<char*>("%s %s"), sLabel.CStr(), sText.CStr());
        break;
    }
    case FEEDBACK_REST_ENEMIES_NEARBY:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_SKILL_TARGET_HOSTILE:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_COMBAT_MODE_WRONG_WEAPON:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_NATURAL_DEFENSE_NO_STACK:
        sMessage = GetGUIString(STRREF_STACKED_NATURAL_DEFENSE);
        break;
    case FEEDBACK_ARMOR_DEFENSE_NO_STACK:
        sMessage = GetGUIString(STRREF_STACKED_ARMOR_DEFENSE);
        break;
    case FEEDBACK_SHIELD_DEFENSE_NO_STACK:
        sMessage = GetGUIString(STRREF_STACKED_SHIELD_DEFENSE);
        break;
    case FEEDBACK_DEFLECTION_DEFENSE_NO_STACK:
        sMessage = GetGUIString(STRREF_STACKED_DEFLECTION_DEFENSE);
        break;
    case FEEDBACK_ARMOR_EQUIP_IN_COMBAT:
        sMessage = GetGUIString(STRREF_NO_ARMOR_CHANGE_IN_COMBAT);
        break;
    case FEEDBACK_ARMOR_UNEQUIP_IN_COMBAT:
        sMessage = GetGUIString(STRREF_NO_ARMOR_UNEQUIP_IN_COMBAT);
        break;
    case FEEDBACK_PARTY_LEAVE_BLOCKED:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_PARTY_KICK_BLOCKED:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_NO_FEAT_USES_LEFT:
        sMessage = GetGUIString(STRREF_NO_FEAT_USES_LEFT);
        break;
    case 200:
        sMessage = GetGUIString(0);
        break;
    case 201:
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    case FEEDBACK_PARTY_INVITEE_IN_OTHER_PARTY: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case FEEDBACK_SCRIPT_MESSAGE:
        sMessage = pData->GetString(0);
        break;
    case FEEDBACK_PAUSED:
        sMessage = g_pSWTlkTable->Fetch(STRREF_PAUSED, 0, 0, 0);
        break;
    case FEEDBACK_UNPAUSED:
        sMessage = g_pSWTlkTable->Fetch(STRREF_UNPAUSED, 0, 0, 0);
        break;
    case FEEDBACK_EQUIP_ALIGNMENT_RESTRICTED:
        sMessage = GetGUIString(STRREF_WRONG_ALIGNMENT_TO_EQUIP);
        break;
    case FEEDBACK_EQUIP_CLASS_RESTRICTED:
        sMessage = GetGUIString(STRREF_WRONG_CLASS_TO_EQUIP);
        break;
    case FEEDBACK_EQUIP_RACE_RESTRICTED:
        sMessage = GetGUIString(STRREF_WRONG_SPECIES_TO_EQUIP);
        break;
    case 210:
        sMessage = GetGUIString(0);
        break;
    case 211:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_CONTAINER_FULL:
        sMessage = GetGUIString(STRREF_CONTAINER_FULL);
        break;
    case FEEDBACK_REST_NOT_COMMANDABLE:
        sMessage = GetGUIString(0);
        break;
    case 214:
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    case 215:
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    case FEEDBACK_CHARACTER_EXPORTED:
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    case FEEDBACK_CHARACTER_EXPORT_FAILED:
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    case FEEDBACK_TARGET_UNREACHABLE:
        sMessage = GetGUIString(STRREF_UNABLE_TO_REACH_TARGET);
        break;
    case 219:
        sMessage = GetGUIString(0);
        break;
    case 220:
        sMessage = GetGUIString(0);
        break;
    case 221:
        sMessage = GetGUIString(0);
        break;
    case 222:
    {
        CExoString sSpellName;
        CSWSpell* pSpell = g_pRules->GetSpell(pData->GetInteger(0));
        if (pSpell != 0)
            sSpellName = pSpell->GetSpellNameText();
        g_pTlkTable->SetCustomToken(0, sSpellName);
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    }
    case 223:
        sMessage = GetGUIString(0);
        bCombat = 1;
        break;
    case 224:
        sMessage = GetGUIString(0);
        break;
    case FEEDBACK_STALL_EVENT_STARTED:
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    case FEEDBACK_STALL_EVENT_FINISHED:
        sMessage = g_pSWTlkTable->Fetch(0, 0, 0, 0);
        break;
    case FEEDBACK_IMMUNE_FEAR_FEAT: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        sMessage = g_pSWTlkTable->Fetch(STRREF_IMMUNE_TO_FEAR_EFFECT, 0, 0, 0);
        bCombat = 1;
        break;
    }
    case FEEDBACK_FORCE_POINTS_REGAINED: {
        oid = pData->GetObjectID(0);
        if (GetObjectName(oid, sObjectName) != 0) {
            g_pTlkTable->SetCustomToken(0, sObjectName);
        } else {
#if KOTOR1
            sObjectName = GetGUIString(0);
#else
            sObjectName = GetGUIString(STRREF_OTHER);
#endif
            g_pTlkTable->SetCustomToken(0, sObjectName);
        }
        g_pTlkTable->SetCustomToken(1, CExoString(pData->GetInteger(0)));
        sMessage = g_pSWTlkTable->Fetch(STRREF_REGAINS_FORCE_POINTS, 0, 0, 0);
        bCombat = 1;
        break;
    }
#if !KOTOR1  // K1's message ids stop at 228
    case FEEDBACK_EQUIP_GENDER_RESTRICTED:
        sMessage = GetGUIString(STRREF_WRONG_GENDER_TO_EQUIP);
        break;
    case FEEDBACK_EQUIP_ATTRIBUTE_TOO_LOW:
        sMessage = GetGUIString(STRREF_ATTRIBUTE_TOO_LOW_TO_USE);
        break;
    case FEEDBACK_COMPONENTS_ACQUIRED: {
        int nInt0 = pData->GetInteger(0);
        g_pTlkTable->SetCustomToken(0, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(STRREF_ACQUIRED_COMPONENTS, 0, 0, 0);
        break;
    }
    case FEEDBACK_CHEMICALS_ACQUIRED: {
        int nInt0 = pData->GetInteger(0);
        g_pTlkTable->SetCustomToken(0, CExoString(nInt0));
        sMessage = g_pSWTlkTable->Fetch(STRREF_ACQUIRED_CHEMICALS, 0, 0, 0);
        break;
    }
    case FEEDBACK_EQUIP_CHARACTER_RESTRICTED:
        sMessage = GetGUIString(STRREF_CHARACTER_CANNOT_USE_EQUIPMENT);
        break;
#endif
    default:
        break;
    }

#if KOTOR1  // every line goes to the message buffer; K1 has no combat buffer to route bCombat to
    K2_UNUSED(bCombat);
    if (!sMessage.IsEmpty()) {
        sMessage = sMsg + sMessage;
        pInGameGui->AppendToMsgBuffer(sMessage, kMessageTypeFeedback, 0);
    }
#else
    if (sMessage.m_sString != 0 && sMessage.m_sString[0] != 0) {
        sMessage = sMsg + sMessage;
        if (bCombat)
            pInGameGui->AppendToCombatBuffer(sMessage, kMessageTypeFeedback, 0);
        else
            pInGameGui->AppendToMsgBuffer(sMessage, kMessageTypeFeedback, 0);
    }
#endif
    if (pData != 0)
        delete pData;
}

void CClientExoAppInternal::ShowObject(CSWCObject* pObject, int nPartyIndex, int nMode, int nUnused)
{
    K2_UNUSED(nUnused);
    static unsigned int nPathNumbers[200];
    static unsigned char InitArray;

    if (pObject == 0) {
        m_pInGameGui->SetMainInterfaceTarget(OBJECT_INVALID, 0);
        CSWCCreature* pPC = m_pParty->GetCharacter(0);
        if (pPC)
            pPC->LookAt(OBJECT_INVALID, 10.0f);
        return;
    }
    m_pInGameGui->SetMainInterfaceTarget(pObject->m_objectId, 0);
    if (nPartyIndex != 0) {
        CSWCCreature* pPC = m_pParty->GetCharacter(0);
        if (pPC)
            pPC->LookAt(pObject->m_objectId, 10.0f);
        if (nMode != 0 || m_combatMode != 0) {
            if (CAurBehavior* pBeh = m_pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK))
                static_cast<CSWCameraOnAStick*>(pBeh)->SetLookAtTarget(pObject);
            if (m_combatMode != 0) {
                if (pObject->AsSWCCreature() && pObject->AsSWCCreature()->IsHostile()) {
                    pObject->Hilite(CGuiInGame::COLOR_RED, 0, 0.0f, 1);
                    // Records the hostile-target highlight, restamping the row if present, else
                    // appending one.
                    int nCount = m_hiliteList.m_count;
                    if (nCount != 0) {
                        int64_t i = 0;
                        do {
                            if (m_hiliteList.m_pArray[i].m_dwObjectId == pObject->m_objectId) {
                                m_hiliteList.m_pArray[i].m_fHiliteSize = 0.25f;
                                goto hilited;
                            }
                            i++;
                        } while (i < nCount);
                    }
                    m_hiliteList.SetSize(nCount + 1);
                    m_hiliteList.m_pArray[nCount].m_dwObjectId = pObject->m_objectId;
                    m_hiliteList.m_pArray[nCount].m_fHiliteSize = 0.25f;
hilited:;
                }
            }
        }
    }

    // RenderDEV draws the path-walkmesh squares and dumps the spot/listen debug state.
    if (RenderDEV) {
        CSWSObject* pServer = pObject->GetServerObject();
        if (LockRenderDev) {
            if (LockRenderDev == kLockRenderDevPlayer)
                LockRenderDev = static_cast<int>(g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
            CGameObject* pLocked = g_pAppManager->m_pServerExoApp->GetGameObject(LockRenderDev);
            if (pLocked && pLocked->AsSWSObject())
                pServer = pLocked->AsSWSObject();
        }
        if (pServer) {
            pServer->m_sTag.CStr();
            if (CSWSCreature* pCreature = pServer->AsSWSCreature()) {
                CPathfindInformation* pPathInfo = pCreature->m_pPathfindInfo;
                int nFloats = pPathInfo->m_nWayPointFloats;
                int nHalf = nFloats / 2;
                float* pPathArray = pPathInfo->m_paWayPoints;
                if (pPathArray && ShowPath) {
                    // On the first call only, reset the per-slot square ids to -1 and retract every
                    // stale drawable; afterwards the stored ids are reused. InitArray is re-read
                    // per iteration because AurRemoveDrawableSquare could touch it.
                    for (int64_t i = 0; i < 200; i++) {
                        unsigned int nId;
                        if (InitArray == 1)
                            nId = nPathNumbers[i];
                        else {
                            nId = 0xffffffff;
                            nPathNumbers[i] = 0xffffffff;
                        }
                        AurRemoveDrawableSquare(nId);
                        nPathNumbers[i] = 0xffffffff;
                    }
                    InitArray = 1;
                    Vector vPrev(pPathArray[0], pPathArray[1], 0.0f);
                    vPrev.z = g_pAppManager->m_pServerExoApp->ComputeHeight(
                        g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->m_resRef, vPrev, 0);
                    if (nFloats >= 4) {
                        for (int64_t i = 0;; i++) {
                            Vector vNew(pPathArray[2 * (i + 1)], pPathArray[2 * (i + 1) + 1], 0.0f);
                            vNew.z = g_pAppManager->m_pServerExoApp->ComputeHeight(
                                g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->m_resRef,
                                vNew, 0);
                            Vector vDraw[4];
                            vDraw[0] = vPrev;
                            vDraw[1] = vPrev;
                            vDraw[2] = vNew;
                            vDraw[3] = vNew;
#if KOTOR1
                            nPathNumbers[i] = static_cast<unsigned int>(AurAddDrawableSquare(reinterpret_cast<float*>(vDraw), 0xffffffff));
#else
                            nPathNumbers[i] = static_cast<unsigned int>(AurAddDrawableSquare(
                                reinterpret_cast<float*>(vDraw), 0xffffffff,
                                "C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/"
                                "app/src/main/cpp/kotor2/CODE/game/clientcore/"
                                "clientexoappinternal.cpp", 13776));
#endif
                            if (i + 2 >= nHalf)
                                break;
                        }
                    }
                }
                CExoLinkedListNode* pNode = pCreature->m_actionList.m_pInternal->m_pHead;
                if (pNode != 0) {
                    unsigned int i = 0;
                    do {
                        if (pNode == 0)
                            break;
                        pCreature->m_actionList.m_pInternal->GetAtPos(pNode);
                        pCreature->m_actionList.m_pInternal->GetNext(pNode);
                    } while (i++ < 5 || pNode == 0);
                }
                pCreature->GetSpotCheckDistance();
                pCreature->GetListenCheckDistance();
                CSWSCreature* pBase0 = g_pAppManager->m_pClientExoApp->GetSWParty()
                                           ->GetCharacter(0)->GetServerCreature();
                Vector vDist;
                vDist.x = pCreature->m_position.x - pBase0->m_position.x;
                vDist.y = pCreature->m_position.y - pBase0->m_position.y;
                vDist.z = pCreature->m_position.z - pBase0->m_position.z;
                magnitude(vDist);
                pCreature->GetLockOrientationToObject();
                pCreature->GetClientCreature()->GetCurrentAnimation();
                if (pCreature->m_bInParty) {
                    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
                    pParty->GetIndex(g_pAppManager->m_pClientExoApp
                                         ->ServerToClientObjectId(pCreature->m_objectId));
                    g_pAppManager->m_pClientExoApp->GetSWParty();
                }
            }
            // For the per-state walkmesh variants, the two header anchor vectors are transformed to
            // world.
            CSWSPlaceable* pPlaceable = pServer->AsSWSPlaceable();
            if (pPlaceable) {
                CSWPlaceableSurfaceMesh* pWm = &pPlaceable->m_walkMesh;
                pWm->LocalToWorld(pWm->m_headerVec[0]);
                pWm->LocalToWorld(pWm->m_headerVec[0]);
                pWm->LocalToWorld(pWm->m_headerVec[1]);
                pWm->LocalToWorld(pWm->m_headerVec[1]);
            }
            CSWSDoor* pDoor = pServer->AsSWSDoor();
            if (pDoor) {
                CSWCollisionMesh* pMesh;
                pMesh = pDoor->m_meshVariants[0];
                pMesh->LocalToWorld(pMesh->m_headerVec[0]);
                pMesh->LocalToWorld(pMesh->m_headerVec[0]);
                pMesh->LocalToWorld(pMesh->m_headerVec[0]);
                pMesh->LocalToWorld(pMesh->m_headerVec[1]);
                pMesh->LocalToWorld(pMesh->m_headerVec[1]);
                pMesh->LocalToWorld(pMesh->m_headerVec[1]);
                pMesh = pDoor->m_meshVariants[1];
                pMesh->LocalToWorld(pMesh->m_headerVec[0]);
                pMesh->LocalToWorld(pMesh->m_headerVec[0]);
                pMesh->LocalToWorld(pMesh->m_headerVec[0]);
                pMesh->LocalToWorld(pMesh->m_headerVec[1]);
                pMesh->LocalToWorld(pMesh->m_headerVec[1]);
                pMesh->LocalToWorld(pMesh->m_headerVec[1]);
                pMesh = pDoor->m_meshVariants[2];
                pMesh->LocalToWorld(pMesh->m_headerVec[0]);
                pMesh->LocalToWorld(pMesh->m_headerVec[0]);
                pMesh->LocalToWorld(pMesh->m_headerVec[0]);
                pMesh->LocalToWorld(pMesh->m_headerVec[1]);
                pMesh->LocalToWorld(pMesh->m_headerVec[1]);
                pMesh->LocalToWorld(pMesh->m_headerVec[1]);
            }
        }
    }

    // RenderQA dumps the character info: tag, dialog resref, flags, spells, feats and skills.
    if (RenderQA) {
        CSWSObject* pServer = pObject->GetServerObject();
        if (pServer) {
            pServer->m_sTag.CStr();
            CExoString sName;
            pServer->GetDialogResref().CopyToString(sName);
            sName.CStr();
            CSWSCreature* pCreature = pServer->AsSWSCreature();
            if (pCreature) {
                pServer->AsSWSCreature();
                pServer->AsSWSCreature()->m_varTable.GetLocalBoolean(62);
                pServer->AsSWSCreature()->GetUserActionsPending();
                pServer->AsSWSCreature();
                pServer->AsSWSCreature();
                pServer->AsSWSCreature()->GetMaxForcePoints();
                for (int i = 0; i < g_pRules->m_pSpellArray->m_nCount; i++) {
                    if (pServer->AsSWSCreature()->m_pStats->HasSpell(0, i, 1))
                        if (g_pRules->GetSpell(i))
                            g_pRules->GetSpell(i)->m_sName.CStr();
                }
                for (int i = 0; i < g_pRules->m_nFeats; i++) {
                    if (pServer->AsSWSCreature()->m_pStats->HasFeat(static_cast<unsigned short>(i)))
                        if (g_pRules->GetFeat(static_cast<unsigned short>(i)))
                            g_pRules->GetFeat(static_cast<unsigned short>(i))->GetNameText().CStr();
                }
                for (int i = 0; i < 8; i++)
                    pServer->AsSWSCreature()->m_pStats->GetSkillRank(static_cast<unsigned char>(i), 0, 0);
            }
            if (pServer->AsSWSDoor())
                pServer->AsSWSDoor();
        }
    }
}
void CClientExoAppInternal::DoPassiveSelection(float fDelta)
{
    int           bShowCalled = 0;
    CSWCCreature* pPlayer = m_pParty->GetPlayerCharacter();
    if (pPlayer == 0)
        goto final_reset;
    {
        CSWSCreature* pServerCreature = pPlayer->GetServerCreature();
        if (pServerCreature != 0) {

        const unsigned long long nInvalid = OBJECT_INVALID;
        CExoArrayList<unsigned long long> listNear;
        CExoArrayList<unsigned long long> listAround;
        CGameObject* pObj;
        int nCount = 0;
        int bEnemyFound = 0;
        int bNoHostileTrig = 1;
        int bCommitted = 0;
        int bAutoPause = 1;
        int bPicked = 0;
        int bRowCommitted = 0;
        int nFoundOther = 0;
        int nFoundTrigger = 0;

#if KOTOR1  // both radii 30
        pServerCreature->GetNearestObjects(30.0f, 30.0f, listNear, listAround);
#else
        pServerCreature->GetNearestObjects(30.0f, 10.0f, listNear, listAround);
#endif
        nCount = listAround.m_count;

        m_visibleTable.m_count = 0;
        m_visibleTable.SetSize(nCount);
        if (nCount == 0) {
            bShowCalled = 0;
            goto cleanup_return;
        }

        {
            for (int i = 0; i < nCount; i++) {
                int nParity = listNear.Contains(listAround.m_pArray[i]);
                m_visibleTable.m_pArray[i].m_flags = static_cast<unsigned char>
                    ((m_visibleTable.m_pArray[i].m_flags & ~1) | (nParity & 1));
                if (listAround.m_pArray[i] != nInvalid)
                    listAround.m_pArray[i] |= CLIENT_OBJECT_ID_FLAG;
                m_visibleTable.m_pArray[i].m_objectId = listAround.m_pArray[i];
                m_visibleTable.m_pArray[i].m_canSee = static_cast<unsigned char>(CNearestStruct::CANSEE_UNKNOWN);

                if (!bEnemyFound) {
                    pObj = GetGameObject(m_visibleTable.m_pArray[i].m_objectId);
                    if (pObj) {
                        if (pObj->AsSWCCreature() && pObj->AsSWCCreature()->IsHostile()) {
                            bAutoPause = 0;
                            bEnemyFound = 1;
                        }
                        if (!bEnemyFound &&
                            pObj->AsSWCTrigger() && pObj->AsSWCTrigger()->IsHostile()) {
                            bNoHostileTrig = 0;
                        }
                    }
                }
                if (m_visibleTable.m_pArray[i].m_objectId == m_lastTarget)
                    bCommitted = 1;
            }
        }

        if (bEnemyFound == 0)
            goto commit_none;
        bPicked = 1;
        if (bCommitted != 0 || m_combatMode == 0)
            goto common_gate;
        if (m_fTargetChanging <= 0.0f)
            goto reacquire_lo;
        m_fTargetChanging -= fDelta;
        if (!(m_fTargetChanging > 0.0f))
            goto tick_timer;
        {
            CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
            CSWSCreature* pSrvC = pServer->GetCreatureByGameObjectID(
                pServer->ClientToServerObjectId(m_lastTarget));
            if (pSrvC == 0)
                goto clear_timer;
            if (pSrvC->GetDead()) {
                bPicked = 1;
                goto commit_picked;
            }
            if (pSrvC->GetDeadTemp() == 0)
                goto clear_timer;
            bPicked = 1;
            goto commit_picked;
        }

    reacquire_lo:
        {
            CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
            CSWSCreature* pSrvC = pServer->GetCreatureByGameObjectID(
                pServer->ClientToServerObjectId(m_lastTarget));
#if KOTOR1
            if (pSrvC && (pSrvC->GetDead() || pSrvC->GetDeadTemp())) {
#else
            if (pSrvC && (pSrvC->GetDead()
                          || pSrvC->GetDeadTemp() != 0)) {
#endif
                m_fTargetChanging = 1.5f;
                goto commit_picked;
            }
            goto tick_timer;
        }

    tick_timer:
        bCommitted = 0;
        bPicked = 1;
        goto auto_gate;

    clear_timer:
        m_fTargetChanging = 0.0f;
        bCommitted = 0;
        bPicked = 1;
        goto auto_gate;

    commit_picked:
        bCommitted = 1;
        goto auto_gate;

    commit_none:
        bPicked = 0;

    common_gate:
        m_fTargetChanging = 0.0f;

    auto_gate:
        if (bAutoPause != 0 && bNoHostileTrig != 0)
            goto enemy_idle;
        goto loop2;

loop2:
        if (m_visibleTable.m_count < 1)
            goto enemy_idle;
        {
            int nIdx = 0;
            int nSavedPicked = bPicked;

            while (nIdx < m_visibleTable.m_count) {
                pObj = GetGameObject(m_visibleTable.m_pArray[nIdx].m_objectId);
                if (pObj == 0)
                    goto scan_advance;
                if (pObj->AsSWCObject() == 0)
                    goto scan_advance;
                if (pObj->AsSWCObject()->IsHostile() == 0)
                    goto scan_advance;
                if (m_visibleTable.m_pArray[nIdx].m_canSee == static_cast<unsigned char>(CNearestStruct::CANSEE_UNKNOWN)) {
                    m_visibleTable.m_pArray[nIdx].m_canSee =
                        static_cast<unsigned char>(pPlayer->CanSee(pObj->AsSWCObject()));
                }
                if (m_visibleTable.m_pArray[nIdx].m_canSee == 0)
                    goto scan_advance;
                if (pObj->AsSWCTrigger() != 0 && pObj->AsSWCTrigger()->m_bIsTrap != 0)
                    nFoundTrigger = 1;
                else
                    nFoundOther = 1;
                {
                    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
                    if (pServerApp == 0 || pServerApp->GetPauseState(2) == 0) {
                        if (m_nPendingAutoPauseReason == PAUSE_MODE_NO_PENDING) {
                            if (pObj->AsSWCTrigger() != 0 &&
                                pObj->AsSWCTrigger()->m_bIsTrap != 0) {
                                if (m_bTrapSighted == 0)
                                    goto commit_trigger;
                                bNoHostileTrig = 1;
                                if (bAutoPause != 0)
                                    goto gate_pass;
                                goto scan_advance;
                            }
                            if (m_bEnemySighted == 0)
                                goto commit_enemy;
                            bAutoPause = 1;
                            if (bAutoPause != 0)
                                goto gate_pass;
                            goto scan_advance;
                        }
                    }
                }
                if (bAutoPause == 0 || bNoHostileTrig == 0)
                    goto scan_advance;
            gate_pass:
                if (bNoHostileTrig == 0)
                    goto scan_advance;
                goto scan_done;

            scan_advance:
                nIdx++;
            }
            goto conclude;
        scan_done:
            bPicked = nSavedPicked;
            goto conclude;

        commit_trigger:
            if (m_pClientOptions->m_bAutoPauseMineSighted && m_combatMode == 0)
                SetAutoPaused(1, PAUSE_MODE_MINE_SIGHTED);
            goto commit_row;
        commit_enemy:
            g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_HOSTILE_CREATURE_ENCOUNTERED, OBJECT_INVALID, OBJECT_INVALID, 0);
            if (m_pClientOptions->m_bAutoPauseEnemySighted && m_combatMode == 0)
                SetAutoPaused(1, PAUSE_MODE_ENEMY_SIGHTED);

        commit_row:
            bCommitted = 1;
            bRowCommitted = 1;
            m_lastTarget = m_visibleTable.m_pArray[nIdx].m_objectId;
            bPicked = nSavedPicked;
            goto conclude;
        }

    enemy_idle:
        bRowCommitted = 0;
        nFoundOther = 0;
        nFoundTrigger = 0;
        if (nFoundOther == 0)
            goto enemy_timer_head;
    conclude:
        if (nFoundOther == 0)
            goto enemy_timer_head;
        goto enemy_timer_fresh;

    enemy_timer_head:
        if (m_bEnemySighted == 0)
            goto enemy_timer_fresh;
        if (m_fEnemySightedTimer < 0.0f)
            m_bEnemySighted = 0;
        else
            m_fEnemySightedTimer -= fDelta;
        if (nFoundTrigger != 0)
            goto trig_fresh;
        goto trig_decay;

    enemy_timer_fresh:
        m_bEnemySighted = nFoundOther;
        m_fEnemySightedTimer = 10.0f;
        if (nFoundTrigger != 0)
            goto trig_fresh;
        goto trig_decay;

    trig_decay:
        if (m_bTrapSighted == 0)
            goto trig_fresh;
        if (!(m_fTrapSightedTimer < 0.0f)) {
            m_fTrapSightedTimer -= fDelta;
            if (bCommitted == 0)
                goto final_gate;
            goto reacquire;
        }
        m_bTrapSighted = 0;
        if (bCommitted != 0)
            goto reacquire;
        goto final_gate;

    trig_fresh:
        m_bTrapSighted = nFoundTrigger;
        m_fTrapSightedTimer = 10.0f;
        if (bCommitted != 0)
            goto reacquire;
        goto final_gate;

    reacquire:
        {
            pObj = GetGameObject(m_lastTarget);
            if (pObj == 0)
                goto final_gate;
            int nIndex = listAround.IndexOf(m_lastTarget);
            if (nIndex != -1) {
                if (m_visibleTable.m_pArray[nIndex].m_canSee == static_cast<unsigned char>(CNearestStruct::CANSEE_UNKNOWN))
                    m_visibleTable.m_pArray[nIndex].m_canSee =
                        static_cast<unsigned char>(pPlayer->CanSee(pObj->AsSWCObject()));
                if (m_visibleTable.m_pArray[nIndex].m_canSee == 0)
                    goto reacquire_not_visible;
            } else {
                if (pPlayer->CanSee(pObj->AsSWCObject()) == 0)
                    goto reacquire_not_visible;
            }
            m_fTargetHiddenTime = 0.0f;
            ShowObject(pObj->AsSWCObject(),
                       bRowCommitted ? 1 : m_bLastTargetChanged, bRowCommitted, 0);
            goto reacquire_commit;

        reacquire_not_visible:
            m_fTargetHiddenTime += fDelta;
            if (!(m_fTargetHiddenTime < 1.0f)) {
                m_fTargetHiddenTime = 0.0f;
                goto final_gate;
            }
            ShowObject(pObj->AsSWCObject(), 0, 0, 0);
            goto reacquire_commit;

        reacquire_commit:
            bShowCalled = 1;
            goto cleanup_return;
        }

    final_gate:
        if ((bPicked & (m_combatMode != 0)) == 0)
            goto scan_a2;
        {
            int i = 0;
            CGameObject* pBest = 0;
            const unsigned long long nInvB = OBJECT_INVALID;
            if (listNear.m_count < 1)
                pBest = 0;
            while (i < listNear.m_count) {
                unsigned long long nId = listNear.m_pArray[i];
                unsigned long long nMasked = (nId == nInvB) ? nId : (nId | CLIENT_OBJECT_ID_FLAG);
                listNear.m_pArray[i] = nMasked;
                pObj = GetGameObject(nMasked);
                if (pObj && pObj->AsSWCObject() != 0) {
                    if (pObj->AsSWCObject()->IsHostile() != 0) {
                        int k = listAround.IndexOf(nMasked);
                        unsigned char c = m_visibleTable.m_pArray[k].m_canSee;
                        if (c == static_cast<unsigned char>(CNearestStruct::CANSEE_UNKNOWN))
                            m_visibleTable.m_pArray[k].m_canSee =
                                c = static_cast<unsigned char>(pPlayer->CanSee(pObj->AsSWCObject()));
                        if (c != 0) {
                            m_lastTarget = listNear.m_pArray[i];
                            ShowObject(pObj->AsSWCObject(), 1, 0, 0);
                            bShowCalled = 1;
                            goto cleanup_return;
                        }
                    } else if (pBest == 0) {
#ifdef K2_FIX_ENGINE_BUGS
                        // Only a target the player can see, as every other pick here tests.
                        int k = -1;
                        for (int j = 0; j < listAround.m_count; j++) {
                            if (listAround.m_pArray[j] == nMasked) {
                                k = j;
                                break;
                            }
                        }
                        unsigned char c = k >= 0 ? m_visibleTable.m_pArray[k].m_canSee
                                                 : static_cast<unsigned char>(CNearestStruct::CANSEE_UNKNOWN);
                        if (c == static_cast<unsigned char>(CNearestStruct::CANSEE_UNKNOWN)) {
                            c = static_cast<unsigned char>(pPlayer->CanSee(pObj->AsSWCObject()));
                            if (k >= 0)
                                m_visibleTable.m_pArray[k].m_canSee = c;
                        }
                        if (c != 0)
                            pBest = pObj;
#else
                        pBest = pObj;
#endif
                    }
                }
                i++;
            }
            {
                int nIdxB = 0;
                while (nIdxB < m_visibleTable.m_count) {
                    CNearestStruct* pRow = &m_visibleTable.m_pArray[nIdxB];
                    pObj = GetGameObject(pRow->m_objectId);
                    if (pObj) {
                        if (pObj->AsSWCObject() != 0 && pObj->AsSWCObject()->IsHostile() != 0) {
                            unsigned char c = pRow->m_canSee;
                            if (c == static_cast<unsigned char>(CNearestStruct::CANSEE_UNKNOWN))
                                pRow->m_canSee = c = static_cast<unsigned char>(pPlayer->CanSee(pObj->AsSWCObject()));
                            if (c != 0) {
                                m_lastTarget = pRow->m_objectId;
                                ShowObject(pObj->AsSWCObject(), 1, 0, 0);
                                bShowCalled = 1;
                                goto cleanup_return;
                            }
                        }
                    }
                    nIdxB++;
                }
                if (pBest != 0) {
                    m_lastTarget = pBest->m_objectId;
                    ShowObject(pBest->AsSWCObject(), 1, 0, 0);
                    bShowCalled = 1;
                    goto cleanup_return;
                }
                bShowCalled = 0;
                goto cleanup_return;
            }
        }

    scan_a2:
        if (listNear.m_count < 1) {
            bShowCalled = 0;
        } else {
            int i = 0;
            const unsigned long long nInvC = OBJECT_INVALID;
            while (i < listNear.m_count) {
                unsigned long long nId = listNear.m_pArray[i];
                unsigned long long nMasked = (nId == nInvC) ? nId : (nId | CLIENT_OBJECT_ID_FLAG);
                listNear.m_pArray[i] = nMasked;
                pObj = GetGameObject(nMasked);
                if (pObj) {
                    int k = -1;
                    for (int j = 0; j < listAround.m_count; j++) {
                        if (listAround.m_pArray[j] == nMasked) {
                            k = j;
                            break;
                        }
                    }
                    unsigned char c = m_visibleTable.m_pArray[k].m_canSee;
                    if (c == static_cast<unsigned char>(CNearestStruct::CANSEE_UNKNOWN))
                        m_visibleTable.m_pArray[k].m_canSee =
                            c = static_cast<unsigned char>(pPlayer->CanSee(pObj->AsSWCObject()));
                    if (c != 0) {
                        m_lastTarget = listNear.m_pArray[i];
                        ShowObject(pObj->AsSWCObject(), 1, 0, 0);
                        bShowCalled = 1;
                        goto cleanup_return;
                    }
                }
                i++;
            }
            bShowCalled = 0;
        }
        // Falling off the list scope runs the CExoArrayList destructors here, shared by the normal
        // path and every `goto cleanup_return`, which exits the scope.
    }

cleanup_return:
    if (bShowCalled)
        goto epilogue;
    /* 0x470b60 fall -> final_reset (0x470b64) */
}

final_reset:
    m_pInGameGui->SetMainInterfaceTarget(OBJECT_INVALID, 0);
    m_lastTarget = OBJECT_INVALID;
    {
        CSWCCreature* pCh = m_pParty->GetCharacter(0);
        if (pCh)
            pCh->LookAt(OBJECT_INVALID, 10.0f);
    }

epilogue:
    if (m_pParty)
        m_pParty->m_members[0].m_oidLastTarget = m_lastTarget;
    m_bLastTargetChanged = 0;
}

// Console command.
const char* SpawnChar(char* pTemplate)
{
    CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    if (pPlayer == 0)
        return "ERROR: No player creature present.";

    Vector vSpawn = pPlayer->m_orientation * 5.0f + pPlayer->m_position;

    CSWSCreature* pCreature = new CSWSCreature(OBJECT_INVALID, 0);
    CResRef resRef(pTemplate);
    if (pCreature->LoadFromTemplate(resRef, 0) != 1)
        return "ERROR: Failed to load template.";

    g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->ComputeSafeLocation(
        vSpawn, 10.0f, pCreature->m_pPathfindInfo, 1, &vSpawn);
    pCreature->AddToArea(g_pAppManager->m_pServerExoApp->GetModule()->GetArea(),
                         vSpawn.x, vSpawn.y, vSpawn.z, 1, 1);
    return "SUCCESS!";
}

// Console command.
int TestGUIStringStruct(void* pStrRef, CGuiStringStruct guiString)
{
    return guiString.m_strRef == reinterpret_cast<uintptr_t>(pStrRef);
}

// Console command.
int TestNearestStruct(void* pObjectId, CNearestStruct nearest)
{
    return nearest.m_objectId == reinterpret_cast<unsigned long long>(pObjectId);
}
