// Client input for CClientExoAppInternal, covering the per-frame input dispatch, the drive and walk
// steps, and the mouse, cursor and hit-check handling.

#include "k2stdint.h"
#ifdef K2_FIX_ENGINE_BUGS
#include <cinttypes>
#endif
#include "port/Mac/MacMain.h"
#include <string>
#include "CExoArrayListImpl.h"
#include "swminigame.h"
#include "serverexoappinternal.h"
#include "exoinputinternal.h"
#include "swguiexamine.h"
#include "swguipazaak.h"
#include "swplayercontrol.h"
#include "AURORA/aurbehavior.h"
#include "nwcitem.h"
#include "nwcvisualeffect.h"
#include "nwccreaturestats.h"
#include "nwcdoor.h"
#include "nwctrigger.h"
#include "nwsplaceable.h"
#include "aurscene.h"
#include "swmgtrackfollower.h"
#include <algorithm>
#include "swguifade.h"
#include "appmanager.h"
#include "guiingame.h"
#include "swguimaininterfacepc2.h"
#include "clientexoapp.h"
#include "swgui.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "ClientExoAppInternal.h"
#include "clientoptions.h"
#include "worldtimer.h"
#include "AURORA/aurmodel.h"
#include "gameobjectarray.h"
#include "exoinput.h"
#include "nwscriptconst.h"
#include "serveranimation.h"
#include "strref.h"

void* g_pGroundRing;
int g_bShiftActionState;
#if !KOTOR1
unsigned char g_gameMenuCancelledActivateInGameGuiButton;
#endif
bool g_bFreeLook;
void* g_pDialogIconModel;

extern char* g_asMouseCursorNames[];

// Ids a frame acts on only once: ProcessInput keeps the first of them in a frame and drops the rest.
enum : unsigned long long {
    kOncePerFrameEvents = (1ull << INPUTEVENT_PAD_FREELOOK) | (1ull << INPUTEVENT_PAD_PAUSE) |
                          (1ull << INPUTEVENT_PAD_PARTY_ACTIVE) | (1ull << INPUTEVENT_PAD_MENU) |
                          (1ull << INPUTEVENT_GUI_A) | (1ull << INPUTEVENT_GUI_B) |
                          (1ull << INPUTEVENT_GUI_X) | (1ull << INPUTEVENT_GUI_Y) |
                          (1ull << INPUTEVENT_GUI_BLACK) | (1ull << INPUTEVENT_GUI_WHITE) |
                          (1ull << INPUTEVENT_GUI_START) | (1ull << INPUTEVENT_GUI_BACK) |
                          (1ull << INPUTEVENT_GUI_LEFT_SHOULDER) |
                          (1ull << INPUTEVENT_GUI_RIGHT_SHOULDER),
#if KOTOR1
    // The vertical moves KOTOR 1 reorders after the horizontal ones.
    kVerticalMoveEvents = (1ull << INPUTEVENT_GUI_DPAD_UP) | (1ull << INPUTEVENT_GUI_DPAD_DOWN) |
                          (1ull << INPUTEVENT_GUI_KEY_UP) | (1ull << INPUTEVENT_GUI_KEY_DOWN),
#endif
};

// The hit check's material bit for trigger geometry.
enum { HITMATERIAL_TRIGGER = 0x40000000 };

// A model's sub-object id for the whole body.
enum { MODEL_PART_BODY = 0xff };

// The input-capture record tags PlayBackInputEvents reads a line for, as bits from 'C'.
enum : unsigned int {
    kPlaybackRecordTags = (1u << ('C' - 'C')) | (1u << ('I' - 'C')) | (1u << ('K' - 'C')) |
                          (1u << ('Z' - 'C')),
};

// DriveControl's move bits. The low two are the move: walk forward, walk back, or both to run.
enum {
    DRIVE_WALK = 0x1,
    DRIVE_BACK = 0x2,
    DRIVE_RUN = DRIVE_WALK | DRIVE_BACK,
    DRIVE_STRAFE_LEFT = 0x4,
    DRIVE_STRAFE_RIGHT = 0x8,
    DRIVE_TURN_LEFT = 0x10,
    DRIVE_TURN_RIGHT = 0x20,
};

// The ids of the target-action menu's entries (CSWGuiInterfaceAction::m_nActionId), whose low 28
// bits pick the mouse cursor.
enum {
    INTERFACE_ACTION_USE_OBJECT = 1001,
    INTERFACE_ACTION_DIALOG = 1002,
    INTERFACE_ACTION_ATTACK = 1003,
    INTERFACE_ACTION_OPEN_DOOR = 1010,
    INTERFACE_ACTION_DISABLE_MINE = 1012,
    INTERFACE_ACTION_BASH = 1013,
    INTERFACE_ACTION_USE_PLACEABLE = 1015,
    INTERFACE_ACTION_RECOVER_MINE = 1026,
    INTERFACE_ACTION_NONE = 1028,
    INTERFACE_ACTION_ID_MASK = 0x0fffffff,
};

// CSWGuiInterfaceAction::m_nFlags: bit 0 enables the entry, bits 1-4 say why it is disabled.
enum {
    ACTIONFLAG_ENABLED = 0x01,
    ACTIONFLAG_REASON_MASK = 0x1e,
    ACTIONFLAG_REASON_PARTY_MEMBER_DEAD = 0x0c,
};

extern int   g_bShowIndicator;

extern int           g_bMouseCameraRotateToggle;

namespace MSG_Mac { void CaptureMouse(bool bCapture); }
#ifdef K2_UI_XBOX
extern "C" int k2shim_touch_click(void);
#endif
#ifdef K2_HOST_BUILD
extern "C" int k2host_move_keys_held(void);
#endif

#include "port/libcbase.h"
#include "port/crtapi.h"
#if !KOTOR1
static void CameraPosMessage(char* buf, int, const char* fmt, ...);
#endif

struct HWND__;
extern int HandleWMKeyDownMessage(HWND__* hWnd, unsigned long long wParam, int64_t lParam);
extern int HandleWMCharMessage(unsigned short wParam);

int g_bLeftMouseDown;
extern float g_inputDisabledTime;
#if !KOTOR1
float g_inputDisabledTime;
#endif
extern int g_bMouseCameraRotateToggleIos;
float g_fMouseSensitivity = 20.0f;
extern float CameraLookUp;
extern float CameraLookDown;
#if KOTOR1
__attribute__((visibility("hidden"))) float s_iosCtrlAxisScale = 115.0f;
__attribute__((visibility("hidden"))) float s_fSwoopIosScale = 20.0f;
#else
extern float s_iosCtrlAxisScale;
#endif
extern float s_iosType1Scaler;
extern float s_iosType2Scaler;
#if KOTOR1
static bool s_bMouseDragged;
__attribute__((visibility("hidden"))) float s_iosType1Scaler;
__attribute__((visibility("hidden"))) float s_iosType2Scaler;
static int s_nLButtonDownX;
__attribute__((visibility("hidden"))) int g_nMouseDragThreshold = 25;
#endif
static void* s_minigameAreaId;
#if KOTOR1
extern int auto_run_active;
extern unsigned char IosActionMenuUp;
int GetMiniGamePlayingType();
extern float ios_gamepad_analog0X;
#ifdef _MSC_VER   // the UCRT's sprintf_s stays the MSVC STL's
#define sprintf_s k1_input_sprintf_s
#endif
static void sprintf_s(char* buf, uint64_t n, const char* fmt, ...);
#endif
bool IsCursorCurrentlyVisible();
float GetIosControl(bool b, float f1, float f2);
float GetIosControl(bool b, float f1, float f2, bool b2);
void IosTemporaryDisableInteraction(int b);
void ClearIosControl(bool b);

extern unsigned char g_gameMenuCancelledActivateInGameGuiButton;
int g_nHSMode;
long long g_lLastLButtonSnapshot;

extern Quaternion YawPitchRoll(float, float, float);

// Rotates a vector by a quaternion, with an early-out for the identity rotations.
static inline Vector RotateByQuaternion(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == 1.0f || w == -1.0f))
        return v;
    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = x * w, wy = y * w, wz = z * w;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

// True when the in-game GUI is up, has a main interface panel and no modal panel is stacked.
bool AllowMouseLookToggle()
{
    if (!g_pGuiMan->PanelExists(g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface))
        return false;
    return g_pGuiMan->m_modalPanels.m_count == 0;
}

// Latches the mouse-look flag, then hands the mouse grab to the platform layer. A negative
// nMode follows the client's invert-mouse-look option; a non-negative one is used literally
// (0 = grab off, 1 = grab on).
void EnableMouseCameraRotateToggle(bool bEnable, int nMode)
{
    g_bMouseCameraRotateToggle = bEnable;

    MSG_Mac::CaptureMouse(nMode < 0
                              ? static_cast<int>(bEnable ^ g_pAppManager->m_pClientExoApp
                                                            ->GetClientOptions()
                                                            ->m_bGameplay1)
                              : static_cast<int>(nMode != 0));
}

void EnableMouseCameraRotateToggle(bool bEnable)
{
    EnableMouseCameraRotateToggle(bEnable, -1);
}

// Resolves a hit-check result into the object that was hit and returns its type. Bare geometry
// (kind 1) has no object and reports the world's tile type; an item reaches its object through
// its CSWCObject base, so it is tested separately from the type threshold below it.
int GetGameobjectHit(CHitInfo& hit, CSWCObject** ppObject)
{
    *ppObject = 0;

    int nResult = 0;
    if (hit.m_nHitType == 1) {
        nResult = GAME_OBJECT_TYPE_TILE;
    } else if (hit.m_nHitType == 2 && hit.m_hitObject.m_pObject != 0) {
        int nType = hit.m_hitObject.m_pObject->GetExternalDataType();
        if (nType == GAME_OBJECT_TYPE_ITEM) {
            CSWCItem* pItem = static_cast<CSWCItem*>(hit.m_hitObject.m_pObject->GetExternalData());
            *ppObject = static_cast<CSWCObject*>(pItem);
            nResult = GAME_OBJECT_TYPE_ITEM;
        } else if (nType > CGameObject::OBJECT_TYPE_AREA) {
            *ppObject = static_cast<CSWCObject*>(hit.m_hitObject.m_pObject->GetExternalData());
            nResult = nType;
        }
    }
    return nResult;
}

// Runs the per-frame input dispatch: input-capture playback, per-event GUI dispatch, the mouse-over
// hit check, and the player and driving camera control.
void CClientExoAppInternal::ProcessInput(float fDelta)
{
#if !KOTOR1
    Vector hitPos = { 0.0f, 0.0f, 0.0f };
#endif
    unsigned long long nMouseTarget = OBJECT_INVALID;

    // Consume the free-look latch before the pause gates.
#if !KOTOR1  // K1 has no free-look latch or input lock-out timer
    if (g_bFreeLook) {
        g_bFreeLook = 0;
        HandleInputEvent(INPUTEVENT_PAD_FREELOOK, INPUTKEY_DOWN);
    }
#endif
    CExoString sModuleProbe;
    int nMouseCamRaw = g_bMouseCameraRotateToggle;
#if !KOTOR1
    if (g_inputDisabledTime > 0.0f) {
        g_inputDisabledTime -= fDelta;
        goto process_input_end;
    }
#endif
    if (m_bInputDisabled)
        goto process_input_end;

    {
    int bMouseCamToggle = g_bMouseCameraRotateToggle;
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if ((pOptions->m_bGameplay1 != 0) && ((m_inputClass & ~1u) != INPUTCLASS_GUI))
        bMouseCamToggle = (nMouseCamRaw == 0);

    // A recorded event stream replaces live input.
    if (g_pAppManager->m_pClientExoApp->GetCaptureEventPlayBack() != 0) {
        PlayBackInputEvents();
        goto process_input_end;
    }

    // Per-frame mouse delta comes from the pause timer, not the world timer.
    g_pExoInput->UpdateMouseDelta(static_cast<float>(static_cast<int>(m_pPauseTimer->GetSnapshotTimeDifference())) / 1000000.0f);

    // The "107PER" long-perk module forces the event walk and skips the hit check.
    bool b107per = false;
#if !KOTOR1  // K1 has no 107PER gate
    if (g_pAppManager->m_pClientExoApp->GetModule() != 0) {
        CSWCModule* pProbe = g_pAppManager->m_pClientExoApp->GetModule();
        CExoString sProbeName(pProbe->m_sModuleDisplayName);
        if (sProbeName.CompareNoCase("107PER") == 0)
            b107per = false;
        else
            b107per = (m_inputClass == INPUTCLASS_DIALOG);
    }
#endif

    if (g_pExoInput->GetActive() != 0 || b107per) {
        int nEvents = 0;
        CExoArrayList<CExoInputEvent> events = g_pExoInput->GetEvents(nEvents, m_inputClass);

        // Rolling ring of the last ten action ids. A frame keeps only its first sticky action,
        // and a stick-release action moves just after the last stick-push that follows it.
        if ((m_pInGameGui->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_SHOWN) || ((m_inputClass & ~1u) == INPUTCLASS_GUI)) {
            bool bWasDown = false;
            for (int i = 0; i < nEvents; i++) {
                if (events.m_pArray[i].m_nValue == 0)
                    continue;
                int nId = events.m_pArray[i].m_pEvent->m_nId;
                if (m_actionKeyCount < 10) {
                    m_actionKeyQueue[m_actionKeyCount++] = nId;
                } else {
                    for (unsigned k = 0; k < 9; k++)
                        m_actionKeyQueue[k] = m_actionKeyQueue[k + 1];
                    m_actionKeyQueue[9] = nId;
                }
                if (static_cast<unsigned>(nId) < INPUTEVENT_GUI_RIGHT_SHOULDER + 1 &&
                    ((1ull << (static_cast<unsigned>(nId) & 63)) & kOncePerFrameEvents) != 0) {
                    if (bWasDown) {
                        events.DelIndex(i);
                        nEvents--;
                        i--;
                        continue;
                    }
                    bWasDown = true;
                }
#if KOTOR1  // K1 also reorders the arrow keys' up and down
                if (static_cast<unsigned>(nId) < INPUTEVENT_GUI_KEY_DOWN + 1 &&
                    ((1ull << (static_cast<unsigned>(nId) & 63)) & kVerticalMoveEvents) != 0 &&
                    i + 1 < nEvents) {
#else
                if (static_cast<unsigned>(nId - INPUTEVENT_GUI_DPAD_UP) < 2 && i + 1 < nEvents) {
#endif
                    int nInsertAt = -1;
                    for (int j = i + 1; j < nEvents; j++) {
                        if (events.m_pArray[j].m_nValue != 0 &&
                            static_cast<unsigned>(events.m_pArray[j].m_pEvent->m_nId - INPUTEVENT_GUI_DPAD_LEFT) < 2)
                            nInsertAt = j + 1;
                    }
                    if (nInsertAt != -1) {
                        if (nInsertAt < nEvents)
                            events.Insert(events.m_pArray[i], nInsertAt);
                        else
                            events.Add(events.m_pArray[i]);
                        events.DelIndex(i);
                    }
                }
            }
        }

        // Per-event playback and GUI dispatch.
        if (nEvents > 0) {
            unsigned int uEventIdx = 0;
            int nHandled = 0;
            for (int i = 0; i < nEvents; i++, uEventIdx++) {
                if (g_pAppManager->m_pClientExoApp->GetCaptureEvents() != 0 &&
                    g_pAppManager->m_pClientExoApp->GetCaptureEventFile() != 0) {
                    CWorldTimer* pCapTimer = g_pAppManager->m_pClientExoApp->GetWorldTimer();
                    unsigned int nCapTime = static_cast<unsigned int>(static_cast<unsigned long long>(pCapTimer->GetSnapshotTime()) / 1000);
                    CExoFile* pCapFile = static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile());
                    g_pExoInput->WriteEventToFile(nCapTime, static_cast<int>(uEventIdx & 0xffffffff), pCapFile);
                }

                CExoInputEvent& ev = events.m_pArray[i];
                int nObj = ev.m_nValue;
                int nId = ev.m_pEvent->m_nId;
#if !KOTOR1
                g_pAppManager->m_pClientExoApp->GetInGameGui();
#endif

                if (static_cast<unsigned>(nId - INPUTEVENT_GUI_A) >= INPUTEVENT_MOUSE_X - INPUTEVENT_GUI_A) {
                    CGuiInGame* pGuiOpt = g_pAppManager->m_pClientExoApp->GetInGameGui();
                    if (pGuiOpt->m_swGuiToggleState != CGuiInGame::SWGUI_STATE_SHOWN) {
                        switch (nId) {
                            case INPUTEVENT_KEY_ESCAPE: case INPUTEVENT_KEY_RETURN:
                            case INPUTEVENT_KEY_UP: case INPUTEVENT_KEY_DOWN:
                            case INPUTEVENT_KEY_LEFT: case INPUTEVENT_KEY_RIGHT:
                            case INPUTEVENT_KEY_SPACE: case INPUTEVENT_KEY_NUMPAD_ENTER:
                            case INPUTEVENT_CHANGE_CHARACTER: case INPUTEVENT_GUI:
#if KOTOR1
                            case INPUTEVENT_KEY_CAPSLOCK:
#endif
                            case INPUTEVENT_PREV_MENU: case INPUTEVENT_NEXT_MENU:
                                break;               // these always go to the GUI
                            default:
                                if (static_cast<unsigned>(nId - INPUTEVENT_DIALOG_1) > INPUTEVENT_DIALOG_9 - INPUTEVENT_DIALOG_1)
                                    goto handle_input_event;
                        }
                        goto gui_rule;
                    }
handle_input_event:
                    nHandled = HandleInputEvent(nId, nObj);
                    goto cursor_reset;
                }
                // Ids outside the console action-bar range fall through to gui_rule, not past it.
gui_rule:
                // The GUI rule handles the action-bar, menu, panel and quick-key ids.
#if KOTOR1  // K1 restores the camera and still offers the event to the GUI
                if (nObj != 0 && m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK &&
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bControllerPauseActive == 0) {
                    g_bShowIndicator = 1;
                    GetModule()->RestoreCamera();
                    SetInputClass(INPUTCLASS_GAME, 1);
                }
                if ((m_pInGameGui->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_SHOWN) || ((m_inputClass & ~1u) == INPUTCLASS_GUI))
                    m_pSWGuiManager->HandleInputEvent(nId, nObj);
#else
                if ((nObj == 0) || (m_pClientOptions->m_nCameraMode != CClientOptions::CAMERA_MODE_FREE_LOOK) ||
                    (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bControllerPauseActive != 0)) {
                    if ((m_pInGameGui->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_SHOWN) || ((m_inputClass & ~1u) == INPUTCLASS_GUI))
                        m_pSWGuiManager->HandleInputEvent(nId, nObj);
                } else {
                    g_bShowIndicator = 1;
                    GetModule()->RestoreCamera();
                    SetInputClass(INPUTCLASS_GAME, 1);
                }
#endif
cursor_reset:
#if !KOTOR1  // K1 moves the GUI cursor even when it is hidden
                if (IsCursorCurrentlyVisible() == 0)
                    goto cursor_hidden;
#endif
                if (bMouseCamToggle == 0) {
                    if (m_inputClass == INPUTCLASS_FREELOOK) goto cursor_hidden;
                } else if (m_inputClass != INPUTCLASS_GUI) goto cursor_hidden;
                m_pSWGuiManager->HandleMouseMove(m_restoreMouseX, m_restoreMouseY);
                nHandled = 0;
                continue;
cursor_hidden:
                if (nHandled != 0 && g_pGuiMan != 0)
                    g_pGuiMan->ResetToolTipTimer();
            }
        }

        // Mouse-over and hit-check resolution. A missing module, the mouse-cam toggle or a missing
        // camera resets the target and jumps straight to the joint below.
        if (GetModule() == 0 || bMouseCamToggle != 0 ||
            GetModule()->m_pCamera == 0) {
            if (m_pInGameGui != 0 && m_pInGameGui->m_pMainInterface != 0) {
                Vector zeroPos1 = { 0.0f, 0.0f, 0.0f };
                HandleMouseOver(OBJECT_INVALID, zeroPos1, 0);
                m_pInGameGui->m_pMainInterface->m_mouseOverTargetId = OBJECT_INVALID;
                goto mouse_over_joint;
            }
        } else {
            // An active target-action menu pre-empts the ray-cast.
            CSWGuiControl* pMenuCtrl = m_pSWGuiManager->m_pMouseOverControl;
            if (pMenuCtrl != 0) {
                int nMenuState = static_cast<int>(m_pInGameGui->m_pMainInterface->m_targetActionMenu
                    .IsTargetActionMenuControl(pMenuCtrl));
                if ((nMenuState & 0xff) != 0) {
                    if (nMenuState == 1) {
                        // The cursor is on the target's action menu, so keep the mouse-over on the
                        // last target.
                        Vector zeroPos2 = { 0.0f, 0.0f, 0.0f };
                        HandleMouseOver(m_lastTarget, zeroPos2, 1);
                    } else {
                        // Any other menu state clears the mouse-over object.
                        Vector zeroPos3 = { 0.0f, 0.0f, 0.0f };
                        HandleMouseOver(OBJECT_INVALID, zeroPos3, 0);
                    }
                    // Both arms keep the last target as the HUD's mouse-over id.
                    m_pInGameGui->m_pMainInterface->m_mouseOverTargetId = m_lastTarget;
                    goto mouse_over_joint;
                }
            }
            // The full hit check covers the ignore list, the ray-cast, the per-type gates and the
            // mouse-over target.
            CHitInfo chit(0, ~HITMATERIAL_TRIGGER, 0);
#if KOTOR1
            Vector hitPos = { 0.0f, 0.0f, 0.0f };
#endif
            int nGuiMX = g_pGuiMan->m_currentMousePos.x;
            int nGuiMY = g_pGuiMan->m_currentMousePos.y;
            short nGuiW = g_pGuiMan->m_nScreenHeight;
            EmptyHitcheckIgnoreList();
            AddObjectToHitcheckIgnoreList(m_playerCreatureId);
            CSWCObject* pLeash = 0;
hitcheck_retry:
            SetHitcheckIgnoreList(&chit.m_ignoreList);
            CAurCamera* pCam = GetModule()->m_pCamera;
            pCam->HitCheck(nGuiMX, static_cast<int>(nGuiW) - nGuiMY, &hitPos, &chit);
            {
                CAurObject* pHitHead = chit.m_hitObject.m_pObject;
                if (pHitHead == 0 || static_cast<Gob*>(pHitHead)->GetExternalData() == 0)
                    goto mouseover_scan;
                // The type comes from the hit object, not from its external data.
                switch (pHitHead->GetExternalDataType()) {
                    case GAME_OBJECT_TYPE_PLACEABLE:
                        pLeash = static_cast<CGameObject*>(static_cast<Gob*>(pHitHead)->GetExternalData())->AsSWCObject();
                        if (pLeash == 0)
                            goto mouseover_scan;
                        if (pLeash->AsSWCPlaceable() != 0) {
                            CSWCPlaceable* pPlaceable = pLeash->AsSWCPlaceable();
                            CSWSPlaceable* pSrv = pPlaceable->GetServerPlaceable();
                            if (pSrv == 0)
                                goto mouseover_scan;
                            if (pPlaceable->m_bIgnoreHitcheck == 0 || pSrv->m_bStatic != 0) {
                                int nIgnore = 0;
                                g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(
                                    pPlaceable->m_nAppearanceType,
                                    CTwoDimArrays::COL2DA_PLACEABLES_IGNORESTATICHITCHECK, &nIgnore);
                                if (nIgnore == 0)
                                    goto mouseover_scan;
                                AddObjectToHitcheckIgnoreList(pPlaceable->m_objectId);
                                goto hitcheck_retry;
                            }
                        }
                        break;
                    case GAME_OBJECT_TYPE_CREATURE:
                        pLeash = static_cast<CGameObject*>(static_cast<Gob*>(pHitHead)->GetExternalData())->AsSWCObject();
                        if (pLeash == 0)
                            goto mouseover_scan;
                        break;
                    case GAME_OBJECT_TYPE_ITEM:                 // the item's CSWCObject base
                        pLeash = static_cast<CSWCItem*>(static_cast<Gob*>(pHitHead)->GetExternalData())->AsSWCObject();
                        if (pLeash == 0)
                            goto mouseover_scan;
                        break;
                    case GAME_OBJECT_TYPE_TRIGGER:
                        pLeash = static_cast<CGameObject*>(static_cast<Gob*>(pHitHead)->GetExternalData())->AsSWCObject();
                        if (pLeash == 0)
                            goto mouseover_scan;
                        if (pLeash->AsSWCTrigger() != 0 && pLeash->AsSWCTrigger()->m_bIsTrap != 0 &&
                            pLeash->AsSWCTrigger()->GetCurrentAnimation() != SERVER_ANIMATION_MINE_VISIBLE)
                            goto mouseover_scan;
                        break;
                    case GAME_OBJECT_TYPE_DOOR:
                        pLeash = static_cast<CGameObject*>(static_cast<Gob*>(pHitHead)->GetExternalData())->AsSWCObject();
                        if (pLeash == 0)
                            goto mouseover_scan;
                        if (pLeash->AsSWCDoor() != 0) {
                            if (pLeash->AsSWCDoor()->m_bStatic != 0)
                                goto mouseover_scan;
                            if (pLeash->AsSWCDoor()->GetIsOpen())
                                goto mouseover_scan;
                        }
                        break;
                    default:
                        goto mouseover_scan;
                }
            }
            // A creature leashes to the object it points at. With no leash it is rejected only
            // as a corpse, which is also where a dead or 0-HP party member lands.
            if (pLeash->AsSWCCreature() != 0) {
                if (pLeash->AsSWCCreature()->m_oidBodyBag == OBJECT_INVALID) {
                    if (pLeash->AsSWCCreature() != 0) {
                        if (pLeash->AsSWCCreature()->GetServerCreature() != 0 &&
                            pLeash->AsSWCCreature()->GetServerCreature()->GetDead() == 0) {
                            CSWSCreature* pSrv = pLeash->AsSWCCreature()->GetServerCreature();
                            if (pSrv->GetDeadTemp() == 0)
                                goto leash_resolve;
                        }
                        if (GetModule()->m_pArea->GetIsObjectCorpse(
                                pLeash->AsSWCCreature()->m_objectId) != 0)
                            goto mouseover_scan;
                    }
                } else {
                    // An id that is not an object keeps the leash.
                    CGameObject* pAtt = g_pAppManager->m_pClientExoApp->GetGameObject(
                        pLeash->AsSWCCreature()->m_oidBodyBag);
                    if (pAtt != 0 && pAtt->AsSWCObject() != 0) {
                        pLeash = pAtt->AsSWCObject();
                        if (pLeash == 0)
                            goto mouseover_scan;
                    }
                }
            }
leash_resolve:
            // An equipped item leashes to its holder.
            if (pLeash->AsSWCItem() != 0) {
                CSWCCreature* pHolder = pLeash->AsSWCItem()->GetEquippedBy();
                if (pHolder != 0) {
                    pLeash = pHolder->AsSWCObject();
                    if (pLeash == 0)
                        goto mouseover_scan;
                }
            }
            if (GetPlayerCreature() != 0) {
                Vector vCam = GetModule()->m_pCamera->GetPosition();
                CSWCCreature* pPC = 0;
                if (GetPlayerCreature() != 0)
                    pPC = GetPlayerCreature();
                else if (m_pParty != 0 && m_pParty->GetCharacter(0) != 0)
                    pPC = m_pParty->GetCharacter(0);
                float fPCx = 0.0f, fPCy = 0.0f;
                if (pPC != 0) {
                    fPCx = pPC->m_position.x;
                    fPCy = pPC->m_position.y;
                }
                Vector vLeash = pLeash->AsSWCObject()->m_position;
#if KOTOR1  // K1 asks and ignores the answer: 30 m for everything
                pLeash->IsHostile();
                float fRange = 900.0f;
#else
                float fRange = pLeash->IsHostile() ? 900.0f : 100.0f;
#endif
                // A NaN facing rejects and a NaN distance accepts.
                float fFacing = (fPCx - vCam.x) * (vLeash.x - vCam.x) +
                                (fPCy - vCam.y) * (vLeash.y - vCam.y) + 0.0f;
                float fDist2 = (vLeash.x - fPCx) * (vLeash.x - fPCx) +
                               (vLeash.y - fPCy) * (vLeash.y - fPCy) + 0.0f;
                // Out of range or under a GUI control, the HUD target is cleared but the leash
                // still gets the mouse-over.
                if (!(fFacing >= 0.0f) || fDist2 > fRange ||
                    m_pSWGuiManager->m_pMouseOverControl != 0)
                    m_pInGameGui->m_pMainInterface->m_mouseOverTargetId = OBJECT_INVALID;
                else
                    m_pInGameGui->m_pMainInterface->m_mouseOverTargetId = pLeash->m_objectId;
                HandleMouseOver(pLeash->m_objectId, hitPos, 0);
                goto mouse_over_cleanup;
            }
mouseover_scan:
            // With no accepted leash, or no player, fall to the visible-object table scan below, or
            // reset if a mini-game is running.
            {
                {
                if (GetModule()->GetMiniGame() == 0 && GetPlayerCreature() != 0) {
                    // Scan the per-frame visible-object table for the best candidate.
                    float fMouseX = static_cast<float>(g_pGuiMan->m_currentMousePos.x);
                    float fMouseY = static_cast<float>(g_pGuiMan->m_currentMousePos.y);
                    CSWCObject* pBest = 0;
                    float fBest = 3.402823466e+38f;
                    CSWCCreature* pPC3 = GetPlayerCreature();
                    for (int k = 0; k < m_visibleTable.m_count; k++) {
                        CGameObject* pObj = GetGameObject(m_visibleTable.m_pArray[k].m_objectId);
                        if (pObj != 0 && pObj->AsSWCObject() != 0) {
                            char cSee = m_visibleTable.m_pArray[k].m_canSee;
                            if (cSee == static_cast<char>(-1)) {
                                cSee = static_cast<char>(pPC3->CanSee(pObj->AsSWCObject()));
                                m_visibleTable.m_pArray[k].m_canSee = cSee;
                            }
                            if (cSee != 0) {
                                Vector vO = { 0,0,0 };
                                pObj->AsSWCObject()->GetReticlePoint(vO);
                                Vector vCam = GetModule()->m_pCamera->GetPosition();
                                float fPCx = pPC3->m_position.x;
                                float fPCy = pPC3->m_position.y;
                                CSWCObject* pO = pObj->AsSWCObject();
                                float fObjX = pO->m_position.x;
                                float fObjY = pO->m_position.y;
#if KOTOR1
                                pObj->AsSWCObject()->IsHostile();
                                float fR2 = 900.0f;
#else
                                float fR2 = pObj->AsSWCObject()->IsHostile() ? 900.0f : 100.0f;
#endif
                                float fRad = pObj->AsSWCObject()->GetStickyMouseThreshhold();
                                float fDx = vO.x - fMouseX;
                                float fDy = vO.y - fMouseY;
                                float fD2 = fDx * fDx + fDy * fDy + vO.z * vO.z;
                                float fFacing = (fPCx - vCam.x) * (fObjX - vCam.x)
                                                + (fPCy - vCam.y) * (fObjY - vCam.y) + 0.0f;
                                float fDist2 = (fObjX - fPCx) * (fObjX - fPCx)
                                               + (fObjY - fPCy) * (fObjY - fPCy) + 0.0f;
                                if (fD2 <= fRad && fD2 < fBest && fFacing >= 0.0f &&
                                    !(fDist2 > fR2)) {
                                    fBest = fD2;
                                    pBest = pObj->AsSWCObject();
                                }
                            }
                        }
                    }
                    // The best candidate always gets the mouse-over. The HUD target takes its id
                    // only off a GUI control, and on a re-hover of the current target only while
                    // its personal-action menu has entries.
                    if (pBest != 0 && m_pSWGuiManager->m_pMouseOverControl == 0 &&
                        (pBest->m_objectId != m_lastTarget ||
                         (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface
                              ->m_personalActions[0].m_count >= 1 &&
                          g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface
                              ->m_personalActions[0].m_pArray != 0)))
                        m_pInGameGui->m_pMainInterface->m_mouseOverTargetId = pBest->m_objectId;
                    else
                        m_pInGameGui->m_pMainInterface->m_mouseOverTargetId = OBJECT_INVALID;
                    HandleMouseOver(pBest != 0 ? pBest->m_objectId : OBJECT_INVALID, hitPos, 0);
                    goto mouse_over_cleanup;
                } else {
                    // A mini-game is live, or there is no player, so reset the mouse-over.
                    HandleMouseOver(OBJECT_INVALID, hitPos, 0);
                    m_pInGameGui->m_pMainInterface->m_mouseOverTargetId = OBJECT_INVALID;
                }
                }
mouse_over_cleanup:
                ;
            }
        }
    }

    // Clear the hover and focus state of the control that was under the cursor, and of its
    // selectable parent.
mouse_over_joint:
    if (bMouseCamToggle != 0 && m_pSWGuiManager->m_pMouseOverControl != 0) {
        CSWGuiControl* pCtrl = m_pSWGuiManager->m_pMouseOverControl;
        CSWGuiControl* pParent = pCtrl->GetSelectableParent();
        pCtrl->HandleMouseOver(0);
        pCtrl->HandleFocusChange(0);
        if (pParent != 0) {
            pParent->HandleMouseOver(0);
            pParent->HandleFocusChange(0);
        }
        m_pSWGuiManager->m_pMouseOverControl = 0;
    }

    // Log the player position to the capture log with a control-frame marker.
    CExoString sLog;
    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
    pApp->GetPlayerCreature();
    if (pApp->GetCaptureEventFile() != 0) {
        CSWCCreature* pPC = pApp->GetPlayerCreature();
        if (pPC != 0) {
            float fPX = pPC->m_position.x, fPY = pPC->m_position.y, fPZ = pPC->m_position.z;
            CWorldTimer* pWT = pApp->GetWorldTimer();
            sLog.Format(const_cast<char*>("%d %c %f %f %f\n"), static_cast<unsigned int>(static_cast<unsigned long long>(pWT->GetSnapshotTime()) / 1000),
                        'X', fPX, fPY, fPZ);
            static_cast<CExoFile*>(pApp->GetCaptureEventFile())->Write(sLog);
        }
    }

    // Player control and camera look, in the non-driving and driving variants.
    {
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        CSWCCreature* pChar = pParty->GetCharacter(0);
        int bCharDead = GetControlledCharacterDead();
#ifdef K2_HOST_BUILD
        if (k2host_move_keys_held()) {
            static char s_szLast[160];
            char szWhy[160];
            snprintf(szWhy, sizeof szWhy, "dead %d, control %s, enabled %d, input class %d, area not ready %d, camera %d",
                     bCharDead, m_pPlayerControl ? "yes" : "none",
                     m_pPlayerControl ? m_pPlayerControl->m_bEnabled : -1, m_inputClass, m_bAreaNotReadyToDisplay,
                     static_cast<int>(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode));
            if (strcmp(szWhy, s_szLast) != 0) {
                fprintf(stderr, "k2: move keys: %s\n", szWhy);
                strcpy(s_szLast, szWhy);
            }
        }
#endif
        if (bCharDead == 0 && m_pPlayerControl != 0) {
            if (m_pPlayerControl->m_bEnabled == 0 ||
                ((m_inputClass != INPUTCLASS_GAME || m_bAreaNotReadyToDisplay != 0) ||
                 g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE)) {
                // When not driving, the navigate camera's pivot writes and the free-look AcclTilt
                // run, with no player move.
                CClientOptions* pOptsA = g_pAppManager->m_pClientExoApp->GetClientOptions();
                if (pOptsA->m_nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE) {
                    float fA = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_MOVE_Y, INPUTCLASS_GAME));
                    float fB = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_MOVE_X, INPUTCLASS_GAME));
                    if (fA == 0.0f && fB == 0.0f) {
                        fB = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ACTION_LEFT_RIGHT, 0));
                        fA = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ACTION_UP_DOWN, 0));
                    }
#if !KOTOR1  // K1's pivot camera ignores the fade
                    int bFade = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetGlobalFadeActive();
                    if (bFade != 0)
                        fA = 0.0f;
#endif
                    float fC = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_CAMERA_TILT, INPUTCLASS_GAME));
                    float fD = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_CAMERA_TURN, INPUTCLASS_GAME));
                    CAurBehavior* pBeh = g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CURRENT);
                    if (pBeh != 0 && pBeh->GetType() == BEHAVIOR_CAMERA_NAVIGATE) {
#if KOTOR1
                        CSWCameraNavigate* pNav = static_cast<CSWCameraNavigate*>(pBeh);
                        pNav->m_fTilt = fC * -30.0f;
                        pNav->m_fSpeed = fA * 10.0f;
                        pNav->m_fTurn = fD * -60.0f;
                        pNav->m_fStrafe = fB * 10.0f;
                        pNav->m_fRoll = fA * fD * -90.0f;
#else
                        fB = fB * 10.0f;
                        CSWCameraNavigate* pNav = static_cast<CSWCameraNavigate*>(pBeh);
                        pNav->m_fTilt = fC * -30.0f;
                        pNav->m_fSpeed = fA * 10.0f;
                        if (bFade != 0)
                            fB = 0.0f;
                        pNav->m_fTurn = fD * -60.0f;
                        pNav->m_fStrafe = fB;
                        pNav->m_fRoll = fA * fD * -90.0f;
#endif
                    }
                }
                CClientOptions* pOptsB = g_pAppManager->m_pClientExoApp->GetClientOptions();
                if (pOptsB->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
                    float fE = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_MOVE_Y, INPUTCLASS_GAME));
                    float fF = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_MOVE_X, INPUTCLASS_GAME));
                    if (fE == 0.0f && fF == 0.0f) {
                        fF = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ACTION_LEFT_RIGHT, 4));
                        fE = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ACTION_UP_DOWN, 4));
                    }
                    if (fE != 0.0f || fF != 0.0f)
                        g_pAppManager->m_pClientExoApp->GetModule()->AcclTiltCamera(fE);
                }
#if KOTOR1
                auto_run_active = 0;
#endif
                m_pPlayerControl->UpDown(0);
                m_pPlayerControl->LeftRight(0);
            } else {
                // Driving uses the tilt and the iOS camera look.
#if KOTOR1
                // K1: stick or keys, the touch stick, and the walk key; no mouse-look tilt.
                float fCamAxisX = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_MOVE_Y, INPUTCLASS_GAME));
                float fCamAxisY = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_MOVE_X, INPUTCLASS_GAME));
                if (fCamAxisX == 0.0f && fCamAxisY == 0.0f) {
                    fCamAxisY = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ACTION_LEFT_RIGHT, 0));
                    fCamAxisX = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ACTION_UP_DOWN, 0));
                    if (fCamAxisY <= -1.0f) fCamAxisY = -1.0f;
                    if (fCamAxisX <= -1.0f) fCamAxisX = -1.0f;
                    if (fCamAxisY > 1.0f) fCamAxisY = 1.0f;
                    if (fCamAxisX > 1.0f) fCamAxisX = 1.0f;
                }
                int nRotate = g_bMouseCameraRotateToggle;
                if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay1 != 0)
                    nRotate = (nRotate == 0);
                // With both buttons held, walk forward unless the stick already pushes forward.
                float fInit = (nRotate != 0 && g_bLeftMouseDown != 0 && fCamAxisX <= 0.0f) ? -1.0f
                                                                                         : fCamAxisX;
                float fSens = static_cast<float>(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMouseSensitivity);
                float fIos = GetIosControl(true, (fSens / 100.0f) * -110.0f + 150.0f, 0.3f - (fSens / 100.0f) * 0.3f);
                IosTemporaryDisableInteraction(0);
                if (m_pModule != 0 && m_pModule->m_pArea != 0 && s_minigameAreaId != static_cast<void*>(m_pModule->m_pArea)) {
                    s_minigameAreaId = static_cast<void*>(m_pModule->m_pArea);
                    ClearIosControl(true);
                }
#ifndef K2_UI_TOUCH
                // pc and xbox UIs: the keyboard's forward/back axis (or both mouse buttons) drives
                // both ways, forward negative; the stick only when they are idle.
                if (fInit == 0.0f)
#else
                if (fInit <= 0.0f)
#endif
                    fInit = fIos;
                float fSide = fCamAxisY + ios_gamepad_analog0X;
                if (ios_gamepad_analog0X == 0.0f)
                    fSide = fCamAxisY;
                fCamAxisY = fSide;
#ifdef K2_FIX_ENGINE_BUGS
                // A global fade holds the player still, as K2 does.
                if (g_pAppManager->m_pClientExoApp->GetInGameGui()->GetGlobalFadeActive() != 0) {
                    fCamAxisY = 0.0f;
                    fInit = 0.0f;
                }
#endif
                float fWalk = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_WALK_MODIFY, INPUTCLASS_GAME));
                if (fWalk == 0.0f)
                    m_pPlayerControl->SetPlayerWalking(0);
                else
                    m_pPlayerControl->SetPlayerWalking(1);

#else
                float fCamAxisX = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_MOVE_Y, INPUTCLASS_GAME));
                float fCamAxisY = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_PAD_MOVE_X, INPUTCLASS_GAME));
                g_pExoInput->PollInput(INPUTEVENT_PAD_CAMERA_TURN, INPUTCLASS_GAME);
                if (fCamAxisX == 0.0f && fCamAxisY == 0.0f) {
                    fCamAxisY = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ACTION_LEFT_RIGHT, 0));
#ifdef K2_UI_TOUCH
                    g_pExoInput->PollInput(INPUTEVENT_AXIS_ACTION_UP_DOWN, 0);
#else
                    // The PC build keeps the keyboard/stick forward/back axis, clamped like the
                    // strafe one; the touch build polls it and discards it for the virtual stick
                    // below.
                    fCamAxisX = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ACTION_UP_DOWN, 0));
                    if (fCamAxisX < -1.0f) fCamAxisX = -1.0f;
                    if (fCamAxisX > 1.0f) fCamAxisX = 1.0f;
#endif
                    fCamAxisY = __builtin_fminf(__builtin_fmaxf(fCamAxisY, -1.0f), 1.0f);
                }
#ifdef K2_UI_TOUCH
                float fSens = static_cast<float>(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMouseSensitivity);
                float fInit = GetIosControl(true, (fSens / 100.0f) * -110.0f + 150.0f, 0.3f - (fSens / 100.0f) * 0.3f);
#else
                // The forward value is the keyboard/stick axis itself on the PC build.
                float fInit = fCamAxisX;
#endif
                IosTemporaryDisableInteraction(0);
                if (m_pModule != 0 && m_pModule->m_pArea != 0 && s_minigameAreaId != static_cast<void*>(m_pModule->m_pArea)) {
                    s_minigameAreaId = static_cast<void*>(m_pModule->m_pArea);
                    ClearIosControl(true);
                }
                int bFade2 = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetGlobalFadeActive();
#ifndef K2_UI_TOUCH
                // pc and xbox UIs: the PC's camera-rotate latch (the right button), not the
                // port's left-press touch-drag one.
                int nIosRotate = g_bMouseCameraRotateToggle;
#else
                int nIosRotate = g_bMouseCameraRotateToggleIos;
#endif
                // The fade zeroes the touch value and the Y axis; those two reach UpDown and
                // LeftRight below.
                if (bFade2 != 0) {
                    fCamAxisY = 0.0f;
                    fInit = 0.0f;
                }
                if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay1 != 0)
                    nIosRotate = (nIosRotate == 0);
#ifdef K2_FIX_ENGINE_BUGS
                // The look limits ease back 3 degrees a frame at 60 fps, at any frame rate.
                const float fLookLimitStep =
                    3.0f * 60.0f * (static_cast<float>(static_cast<int>(m_pPauseTimer->GetSnapshotTimeDifference())) / 1000000.0f);
#else
                const float fLookLimitStep = 3.0f;
#endif
                float fMouseDX = 0.0f, fMouseDY = 0.0f;
                g_pExoInput->GetMouseDelta(&fMouseDX, &fMouseDY);
                if ((IsJoystickConnected() != 0) ||
                    (((nIosRotate != 0 && g_bLeftMouseDown == 0) &&
                      (0.05f < fMouseDY || (fMouseDY < -0.05f || 0.0f < CameraLookUp))) &&
                     (fMouseDX < 0.05f || -0.05f < fMouseDX))) {  // (sic) always true; the intent was probably && here (a mostly-vertical drag)
                    // Camera look takes its pitch and yaw directly from the FL_LOOKUP/FL_LOOKDOWN
                    // 2DA.
                    CSWCameraOnAStick* pStick =
                        static_cast<CSWCameraOnAStick*>(g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
                    if (pStick != 0) {
                        C2DA* pCam2da = g_pRules->m_p2DArrays->m_pCameraStyle;
                        {
                            CExoString sCol("FL_LOOKUP");
                            pCam2da->GetFLOATEntry(0, sCol, &CameraLookUp);
                        }
                        {
                            CExoString sCol("FL_LOOKDOWN");
                            pCam2da->GetFLOATEntry(0, sCol, &CameraLookDown);
                        }
                        float fOld = pStick->m_fTiltSpeed;
                        pStick->m_fTiltSpeed = 300.0f;
                        pStick->m_fSavedTiltSpeed = fOld;
                    }
                } else {
                    // Tilt decays CameraLookUp/CameraLookDown toward the camera 2DA's tilt bounds.
                    C2DA* pCam2da = g_pRules->m_p2DArrays->m_pCameraStyle;
                    float fTiltUp = 0.0f, fTiltSpeed = 0.0f;
                    float fTiltDown = 0.0f;
                    {
                        CExoString sCol("TILTUP");
                        pCam2da->GetFLOATEntry(0, sCol, &fTiltUp);
                    }
                    {
                        CExoString sCol("TILTDOWN");
                        pCam2da->GetFLOATEntry(0, sCol, &fTiltDown);
                    }
                    {
                        CExoString sCol("TILTSPEED");
                        pCam2da->GetFLOATEntry(0, sCol, &fTiltSpeed);
                    }
                    CSWCameraOnAStick* pStick =
                        static_cast<CSWCameraOnAStick*>(g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
                    if (CameraLookUp > fTiltUp) {
                        CameraLookUp -= fLookLimitStep;
                        if (CameraLookUp < fTiltUp)
                            CameraLookUp = fTiltUp;
                    } else {
                        if (pStick != 0)
                            pStick->m_fTiltSpeed = pStick->m_fSavedTiltSpeed;
                    }
                    if (CameraLookDown > fTiltDown) {
                        CameraLookDown -= fLookLimitStep;
                        if (CameraLookDown < fTiltDown)
                            CameraLookDown = fTiltDown;
                    } else {
                        if (pStick != 0)
                            pStick->m_fTiltSpeed = fTiltSpeed;
                    }
                }
#ifndef K2_UI_TOUCH
                // pc and xbox UIs: both buttons held walk forward unless the keys already push
                // back, and the look tilt eases back to level.
                if (nIosRotate != 0 && g_bLeftMouseDown != 0 && fInit <= 0.0f) {
                    CSWCameraOnAStick* pStick =
                        static_cast<CSWCameraOnAStick*>(g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
                    if (CameraLookUp > 0.0f) {
                        CameraLookUp -= fLookLimitStep;
                        if (CameraLookUp < 0.0f)
                            CameraLookUp = 0.0f;
                    } else if (pStick != 0) {
                        pStick->m_fTiltSpeed = pStick->m_fSavedTiltSpeed;
                    }
                    if (CameraLookDown > 0.0f) {
                        CameraLookDown -= fLookLimitStep;
                        if (CameraLookDown < 0.0f)
                            CameraLookDown = 0.0f;
                    } else if (pStick != 0) {
                        pStick->m_fTiltSpeed = pStick->m_fSavedTiltSpeed;
                    }
                    fInit = -1.0f;
                }
#endif
                float fPark = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_WALK_MODIFY, INPUTCLASS_GAME));
                if (fPark == 0.0f)
                    m_pPlayerControl->SetPlayerWalking(0);
                else
                    m_pPlayerControl->SetPlayerWalking(1);

                // Park or release the walk lock through the server dead and hit-point gates.
#endif
                float fMoveX = fInit, fMoveY = fCamAxisY;
                if (pChar != 0 && pChar->IsDriveAnimationBlocked() == 1) {
                    if ((pChar->m_nStateFlags & 1) != 0) {
                        bool bGameplay5 = m_pClientOptions->m_bGameplay5 != 0;
                        fMoveX = 0.0f;
                        if (bGameplay5) fMoveX = fInit;
                        fMoveY = 0.0f;
                        if (bGameplay5) fMoveY = fCamAxisY;
                    }
                    if (fMoveX == 0.0f && fMoveY == 0.0f) {
                        m_fDriveInterruptDelay = -1.0f;
                        fCamAxisY = fMoveX;
                        fCamAxisX = fMoveY;
                    } else {
#if KOTOR1
                        // K1: a zero or stale timer restarts at 0.3 s; when it runs out the move
                        // goes through, and a slow walk re-arms it for 0.1 s.
                        float fTimer = m_fDriveInterruptDelay;
#ifdef K2_FIX_ENGINE_BUGS
                        // The 0.3 s delay starts on a fresh press only; once it has run out the keys
                        // drive every frame, as K2 does.
                        if (fTimer == -1.0f || fTimer == 0.0f)
                            fTimer = 0.3f;
                        m_fDriveInterruptDelay = fTimer - fDelta;
                        if (m_fDriveInterruptDelay < -0.5f)
                            m_fDriveInterruptDelay = -0.5f;
#else
                        if (!(fTimer > 0.0f)) {
                            fTimer = 0.3f;
                            m_fDriveInterruptDelay = 0.3f;
                        }
                        m_fDriveInterruptDelay = fTimer - fDelta;
#endif
                        CSWSCreature* pCS = pChar->GetServerCreature();
                        int nDead = pCS != 0 ? pCS->m_bCommandable : 1;
                        fCamAxisY = 0.0f;
                        fCamAxisX = 0.0f;
                        if (!(m_fDriveInterruptDelay > 0.0f)) {
                            fCamAxisY = fMoveX;
                            fCamAxisX = fMoveY;
                            if (nDead != 0) {
                                if (m_pPlayerControl->GetCurSpeed() <= 0.25f) {
#ifndef K2_FIX_ENGINE_BUGS
                                    m_fDriveInterruptDelay = 0.1f;
#endif
                                } else {
                                    static_cast<CSWCObject*>(pChar)->ClearAllActions();
                                    static_cast<CSWCObject*>(pChar)->AbortFireForgetAnimation(0);
                                    pChar->SetLockOrientationToObject(OBJECT_INVALID, 0);
                                    pChar->SetEngagedState(0);
                                    CSWCameraOnAStick* pCamStick = static_cast<CSWCameraOnAStick*>(
                                        g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
                                    if (pCamStick != 0)
                                        pCamStick->ClearCombatCamera();
                                    pChar->SetWayPoints(0, 0);
                                }
                            }
                        }
#else
                        if (m_fDriveInterruptDelay < 0.0f)
                            m_fDriveInterruptDelay = 0.3f;
                        if (m_fDriveInterruptDelay >= 0.0f) {
                            m_fDriveInterruptDelay = m_fDriveInterruptDelay - fDelta;
                            CSWSCreature* pCS = pChar->GetServerCreature();
                            int nDead = pCS != 0 ? pCS->m_bCommandable : 1;
                            fCamAxisY = 0.0f;
                            fCamAxisX = 0.0f;
                            if (m_fDriveInterruptDelay <= 0.0f) {
                                m_fDriveInterruptDelay = 0.0f;
                                if (nDead == 0) {
                                    m_fDriveInterruptDelay = -1.0f;
                                    fCamAxisY = fMoveX;
                                    fCamAxisX = fMoveY;
                                } else {
                                    float fSpeed = m_pPlayerControl->GetCurSpeed();
                                    fCamAxisY = fMoveX;
                                    fCamAxisX = fMoveY;
                                    if (0.25f < fSpeed) {
                                        m_fDriveInterruptDelay = -1.0f;
                                        static_cast<CSWCObject*>(pChar)->ClearAllActions();
                                        static_cast<CSWCObject*>(pChar)->AbortFireForgetAnimation(0);
                                        pChar->SetLockOrientationToObject(OBJECT_INVALID, 0);
                                        pChar->SetEngagedState(0);
                                        CSWCameraOnAStick* pCamStick = static_cast<CSWCameraOnAStick*>(
                                            g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
                                        if (pCamStick != 0)
                                            pCamStick->ClearCombatCamera();
                                        pChar->SetWayPoints(0, 0);
                                    }
                                }
                            }
                        } else {
                            fCamAxisY = fMoveX;
                            fCamAxisX = fMoveY;
                        }
#endif
                    }
                } else {
                    // When not drive-blocked, UpDown takes the forward value and LeftRight the Y
                    // axis.
                    fCamAxisY = fMoveX;
                    fCamAxisX = fMoveY;
                }
                // Party-member running-shoe combat message, the strref picked by move speed.
                // Guarded by pChar rather than m_pParty, so a cutscene skips it.
                if (pChar != 0) {
                    int nIdx = m_pParty->GetIndex(pChar->m_objectId);
                    if (nIdx > -1 && (pChar->m_nStateFlags & 1) != 0) {
                        CSWPartyMemberData* pMem = &m_pParty->m_members[nIdx];
                        float fSpd = m_pPlayerControl->GetCurSpeed();
                        if (!(fSpd < 0.25f))
                            pMem->SetCombatMessage(STRREF_PLAYER_MOVING_CANCEL_COMBAT);
                        else if (pMem->m_nCombatMessageStrRef == STRREF_PLAYER_MOVING_CANCEL_COMBAT)
                            pMem->SetCombatMessage(STRREF_COMBAT_MODE_ENGAGED);
                    }
                }
                // Enemy-auto-pause streak.
                if (m_combatMode == 0) {
                    if (!(m_fEnemyAutoPauseTimer <= 0.0f) || m_lastTarget == OBJECT_INVALID ||
                        !(m_pPlayerControl->GetCurSpeed() >= 0.25f)) {
                        m_fCombatStreak = 0.0f;
                    } else {
                        m_fCombatStreak += fDelta;
                        if (m_fCombatStreak >= 0.5f) {
                            m_fCombatStreak = 0.0f;
                            m_lastTarget = OBJECT_INVALID;
                        }
                    }
                } else {
                    m_fCombatStreak = 0.0f;
                }
                m_pPlayerControl->UpDown(fCamAxisY);
                m_pPlayerControl->LeftRight(fCamAxisX);
            }
        }
    }

    // Per-frame mini-game tick while not in normal player control.
    {
        if (GetPlayerCreature() == 0) {
            if (m_pModule != 0 && m_pModule->m_pArea != 0 &&
                m_pModule->m_pArea->m_pMiniGame != 0 && !b107per) {
                CSWMiniGame* pMG = m_pModule->m_pArea->m_pMiniGame;
#if KOTOR1
                if (pMG->m_type == 1) {
                    // K1 polls the turn keys and drops them: the swoop steers by the touch stick only.
                    g_pExoInput->PollInput(INPUTEVENT_AXIS_ARROW_LEFT_RIGHT, INPUTCLASS_MINIGAME);
                    g_pExoInput->PollInput(INPUTEVENT_AXIS_MG_LEFT_RIGHT, INPUTCLASS_MINIGAME);
                    Vector vAxis = { GetIosControl(false, s_fSwoopIosScale, s_iosType1Scaler), 0.0f, 0.0f };
                    pMG->UpdateAxis(vAxis, fDelta);
                } else if (pMG->m_type == 2) {
                    float fAmp = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ARROW_LEFT_RIGHT, INPUTCLASS_MINIGAME)) +
                                 static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_MG_LEFT_RIGHT, INPUTCLASS_MINIGAME));
                    float fAmp2 = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ARROW_UP_DOWN, INPUTCLASS_MINIGAME)) +
                                  static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_MG_UP_DOWN, INPUTCLASS_MINIGAME));
                    CClientOptions* pOptsMG = g_pAppManager->m_pClientExoApp->GetClientOptions();
                    fAmp2 = std::min(1.0f, std::max(-1.0f, fAmp2));
                    if (pOptsMG != 0 &&
                        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay3 != 0)
                        fAmp2 = -fAmp2;
                    fAmp = std::min(1.0f, std::max(-1.0f, fAmp));
                    Vector vAxis = { fAmp, 0.0f, fAmp2 };
                    pMG->UpdateAxis(vAxis, fDelta);
                    g_pExoInput->PollInput(INPUTEVENT_MINIGAME_MOUSE_X, INPUTCLASS_MINIGAME);
                    g_pExoInput->PollInput(INPUTEVENT_MINIGAME_MOUSE_Y, INPUTCLASS_MINIGAME);
                    float fMX = GetIosControl(false, s_iosCtrlAxisScale, s_iosType2Scaler);
                    float fMY = GetIosControl(true, s_iosCtrlAxisScale, s_iosType2Scaler);
                    if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0 &&
                        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay3 != 0)
                        fMY = -fMY;
                    pMG->UpdateMouse(fMX, fMY, fDelta);
                    if (GetPausedByCombat() == 0) {
                        unsigned short uW = m_pSWGuiManager->m_nScreenWidth;
                        unsigned short uH = m_pSWGuiManager->m_nScreenHeight;
                        g_pExoInput->SetMousePos((uW * 65536 >> 17), (uH * 65536 >> 17));
                    }
                }
#else
                if (pMG->m_type == 1) {
                    float fJsX = 0.0f, fJsY = 0.0f;
                    g_pExoInput->GetJoystickNStick(0, &fJsX, &fJsY);
                    // The clamps are min(1, max(-1, x)), so a NaN reading is -1.
                    fJsX = std::min(1.0f, std::max(-1.0f, fJsX));
                    if (fJsX == 0.0f) {
                        float fS = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ARROW_LEFT_RIGHT, INPUTCLASS_MINIGAME)) +
                                   static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_MG_LEFT_RIGHT, INPUTCLASS_MINIGAME));
                        fJsX = std::min(1.0f, std::max(-1.0f, fS));
                    }
                    short nAreaW = m_pSWGuiManager->m_nScreenWidth;
                    static float s_minigameScale = static_cast<float>(nAreaW) * 0.0009765625f * 120.0f;
                    Vector vAxis = { GetIosControl(false, s_minigameScale, s_iosType1Scaler, false) + fJsX, 0.0f, 0.0f };
                    pMG->UpdateAxis(vAxis, fDelta);
                    if (GetPausedByCombat() == 0 &&
                        g_pAppManager->m_pClientExoApp->GetInputClass() != INPUTCLASS_GAME) {
                        unsigned short uW = m_pSWGuiManager->m_nScreenWidth;
                        unsigned short uH = m_pSWGuiManager->m_nScreenHeight;
                        g_pExoInput->SetMousePos((uW * 65536 >> 17), (uH * 65536 >> 17), true);
                    }
                } else if (pMG->m_type == 2) {
                    float fJsX = 0.0f, fJsY = 0.0f;
                    g_pExoInput->GetJoystickNStick(0, &fJsX, &fJsY);
                    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay3 != 0)
                        fJsY = -fJsY;
                    pMG->UpdateJoystick(fJsX, fJsY, fDelta);
                    float fAmp = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ARROW_LEFT_RIGHT, INPUTCLASS_MINIGAME)) +
                                 static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_MG_LEFT_RIGHT, INPUTCLASS_MINIGAME));
                    fAmp = std::min(1.0f, std::max(-1.0f, fAmp));
                    float fAmp2 = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_ARROW_UP_DOWN, INPUTCLASS_MINIGAME)) +
                                  static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_AXIS_MG_UP_DOWN, INPUTCLASS_MINIGAME));
                    fAmp2 = std::min(1.0f, std::max(-1.0f, fAmp2));
                    if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0 &&
                        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay3 != 0)
                        fAmp2 = -fAmp2;
                    Vector vAxis = { fAmp, 0.0f, fAmp2 };
                    pMG->UpdateAxis(vAxis, fDelta);
#ifdef K2_FIX_ENGINE_BUGS
                    // The mouse pair is the frame's travel; taken as its rate at 60 fps, the turret
                    // turns as far for the same travel at any frame rate.
                    const float fMouseRate = fDelta > 0.0f ? (1.0f / 60.0f) / fDelta : 1.0f;
                    float fMX = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_MINIGAME_MOUSE_X, INPUTCLASS_MINIGAME)) / g_fMouseSensitivity * fMouseRate;
                    float fMY = -static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_MINIGAME_MOUSE_Y, INPUTCLASS_MINIGAME)) / g_fMouseSensitivity * fMouseRate;
#else
                    float fMX = static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_MINIGAME_MOUSE_X, INPUTCLASS_MINIGAME)) / g_fMouseSensitivity;
                    float fMY = -static_cast<float>(g_pExoInput->PollInput(INPUTEVENT_MINIGAME_MOUSE_Y, INPUTCLASS_MINIGAME)) / g_fMouseSensitivity;
#endif
                    fMX = std::min(1.0f, std::max(-1.0f, fMX));
                    fMY = std::min(1.0f, std::max(-1.0f, fMY));
#ifdef K2_UI_TOUCH
                    // The virtual stick replaces the mouse pair on the touch port.
                    fMX = GetIosControl(false, s_iosCtrlAxisScale, s_iosType2Scaler, false);
                    fMY = GetIosControl(true, s_iosCtrlAxisScale, s_iosType2Scaler, false);
#endif
                    if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0 &&
                        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay3 != 0)
                        fMY = -fMY;
                    pMG->UpdateMouse(fMX, fMY, fDelta);
                    if (GetPausedByCombat() == 0 &&
                        g_pAppManager->m_pClientExoApp->GetInputClass() != INPUTCLASS_GAME) {
                        unsigned short uW = m_pSWGuiManager->m_nScreenWidth;
                        unsigned short uH = m_pSWGuiManager->m_nScreenHeight;
                        g_pExoInput->SetMousePos((uW * 65536 >> 17), (uH * 65536 >> 17), true);
                    }
                }
#endif
                pMG->Control(fDelta);
            }
        } else {
            m_pPlayerControl->Control(fDelta);
            if (m_pModule != 0 && m_pModule->m_pArea != 0 && m_pModule->m_pArea->m_pMiniGame != 0)
                m_pModule->m_pArea->m_pMiniGame->Control(fDelta);
        }
    }
    }

process_input_end:
#if !KOTOR1
    (void)hitPos;
#endif
    (void)nMouseTarget;
    (void)&sModuleProbe;
    return;
}

// Dispatches a bound action id to its handler.
int CClientExoAppInternal::HandleInputEvent(int nAction, int nParam)
{
    int handled = 0;
    int nAbility = 0;

    switch (nAction) {

    // In-game panel keys.
    case INPUTEVENT_PANEL_EQUIP: case INPUTEVENT_PANEL_INVENTORY:
    case INPUTEVENT_PANEL_CHARACTER: case INPUTEVENT_PANEL_ABILITIES:
    case INPUTEVENT_PANEL_MESSAGES: case INPUTEVENT_PANEL_JOURNAL:
    case INPUTEVENT_PANEL_MAP: case INPUTEVENT_PANEL_OPTIONS:
        if (nParam != 0) {
            if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
                g_bShowIndicator = 1;
                GetModule()->RestoreCamera();
                SetInputClass(INPUTCLASS_GAME, 1);
            }
            // A dead character is a hazard; otherwise show, hide or switch the panel.
            if (GetControlledCharacterDead() != 0) {
                if (m_nGameOverReason != 0)
                    HurryDeathSequence();
                return 0;
            }
            // The m_nGlobalDialogState and modal-panel gate decides show, hide or switch.
            if (m_pInGameGui->m_nGlobalDialogState == 0 &&
                m_pSWGuiManager->m_modalPanels.m_count == 0) {
                int nPanel = nAction - INPUTEVENT_PANEL_EQUIP;
                if (m_pInGameGui->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_COVERED) {
                    if (m_pInGameGui->m_lastGuiPanel == nPanel) {
                        if (m_pInGameGui->HideSWInGameGui(0) != 0)
                            goto setInput01;
                        return handled;
                    }
                    m_pInGameGui->SwitchToSWInGameGui(nPanel);
                    return 0;
                }
                if (m_pInGameGui->ShowSWInGameGui(nPanel) == 0)
                    return handled;
                SetInputClass(INPUTCLASS_GUI, 1);
                return 0;
            }
            return 0;
        }
        return 0;

    // Live-party hotkeys.
    case INPUTEVENT_FREELOOK_STICK_X: case INPUTEVENT_FREELOOK_STICK_Y:
    case INPUTEVENT_FREELOOK_EXIT_KEY1: case INPUTEVENT_FREELOOK_EXIT_KEY2:
    case INPUTEVENT_FREELOOK_EXIT_KEY3: case INPUTEVENT_FREELOOK_EXIT_KEY4:
        if (nParam != 0 && m_nGameOverReason == 0)
            goto retrieve;
        return 0;

    // Pause toggle and its escape variants.
    case INPUTEVENT_PAD_PAUSE: case INPUTEVENT_PAUSE: case INPUTEVENT_PAUSE_MINIGAME:
#if KOTOR1
        if (nParam != 0 && !IosAnyMainPanelUp()) {
#else
        if (nParam != 0) {
#endif
            if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
                g_bShowIndicator = 1;
                GetModule()->RestoreCamera();
                SetInputClass(INPUTCLASS_GAME, 1);
            }
            int bPaused = GetPausedByCombat();
            __asm__ __volatile__("" ::: "memory");  // idiom-exempt: the pause chain really is duplicated here
            SetPausedByCombat(bPaused == 0, 4, 0);
            __asm__ __volatile__("" ::: "memory");  // idiom-exempt: the pause chain really is duplicated here
            if (g_pAppManager->m_pClientExoApp->GetAutoPaused() != 0)
                g_pAppManager->m_pClientExoApp->SetAutoPaused(0, 0);
#if KOTOR1
            if (GetMiniGamePlayingType() != 0)
                Ios_minigamePaused = (bPaused == 0);
#endif
            if (bPaused != 0)
                goto playSound;
            if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(
                    6, OBJECT_INVALID, OBJECT_INVALID, 0) != 0)
                return handled;
            goto playSound;
        }
        return 0;

    // Clear one queued action.
    case INPUTEVENT_CLEAR_ONE_ACTION: case INPUTEVENT_ACTION_MENU_REMOVE_QUEUED:
        if (nParam != 0) {
            if (m_pInGameGui != 0) {
                CSWGuiMainInterface* pMain = m_pInGameGui->m_pMainInterface;
                if (pMain != 0) {
                    pMain->ClearOneAction();
                    return 0;
                }
                return handled;
            }
        }
        return 0;

    // Rotate-camera press and release, with the mouse-grab follow-through.
    case INPUTEVENT_LEFT_LOOKABOUT: case INPUTEVENT_RIGHT_LOOKABOUT:
#if KOTOR1
        if (m_pInGameGui == 0 || m_pInGameGui->m_bInGameMenuShown == 0)
            g_bMouseCameraRotateToggle = nParam;
        return 0;
#else
        if (g_pGuiMan->PanelExists(g_pAppManager->m_pClientExoApp->GetInGameGui()
                                       ->m_pMainInterface) == 0)
            return handled;
        if (g_pGuiMan->m_modalPanels.m_count != 0)
            return 0;
        g_bMouseCameraRotateToggle = (nParam != 0);
        MSG_Mac::CaptureMouse((nParam != 0) !=
                              (g_pAppManager->m_pClientExoApp->GetClientOptions()
                                   ->m_bGameplay1 !=
                               0));
        return 0;
#endif

    // Shift-action state latch.
    case INPUTEVENT_ALTERNATE_ACTIONS: case INPUTEVENT_ALTERNATE_ACTIONS2:
        g_bShiftActionState = nParam;
        return handled;

    // High-speed camera-mode toggle.
#if KOTOR1
    case INPUTEVENT_PAD_FREELOOK: case INPUTEVENT_KEY_CAPSLOCK:
#else
    case INPUTEVENT_PAD_FREELOOK: case INPUTEVENT_FREELOOK:
#endif
        if (GetPlayerCreature() != 0) {
            CSWCCreature* pPC = GetPlayerCreature();
            if (nParam != 0 && pPC->m_bInCombat == 0) {
                if (m_nGameOverReason != 0)
                    return 0;
                if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
                    goto retrieve;
                if (GetPlayerCreature()->GetServerCreature() != 0 &&
                    GetPlayerCreature()->GetServerCreature()
                            ->m_actionList.m_pInternal->m_pHead == 0 &&
                    g_pAppManager->m_pClientExoApp->GetPausedByCombat() == 0) {
                    int nHSMode = g_nHSMode;
                    g_bShowIndicator = 0;
                    CSWCModule* pHSModule = GetModule();
                    if (nHSMode == 1) {
                        pHSModule->SetNavigateCamera();
                        g_nHSMode++;
                        return handled;
                    }
                    pHSModule->SetFreeLookCamera(m_pParty->GetPlayerCharacter()->m_objectId);
                    SetInputClass(INPUTCLASS_FREELOOK, 1);
                    return 0;
                }
                goto retrieve;
            }
        }
        return handled;

    // Change character, to the next living party member.
    case INPUTEVENT_PAD_CHANGE_CHARACTER: case INPUTEVENT_CHANGE_CHARACTER:
        if (nParam != 0) {
            if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
                g_bShowIndicator = 1;
                GetModule()->RestoreCamera();
                SetInputClass(INPUTCLASS_GAME, 1);
            } else {
                ChangeCharacterToNextLivingPartyMember(0, 1, 1);
                g_pAppManager->m_pClientExoApp->GetInGameGui()->ChangeCharacter();
            }
            goto playSound;
        }
        return 0;

    // In-game menu toggle.
    case INPUTEVENT_PAD_MENU: case INPUTEVENT_GUI:
        if (nParam != 0) {
#if !KOTOR1
            m_pSWGuiManager->HandleInputEvent(nAction, nParam);
            if (g_gameMenuCancelledActivateInGameGuiButton != 0) {
                g_gameMenuCancelledActivateInGameGuiButton = 0;
                return handled;
            }
#endif
            // Remember the free-look camera for the solo fallback below.
            int bWasFreeLook = 0;
            if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
                bWasFreeLook = 1;
                g_bShowIndicator = 1;
                GetModule()->RestoreCamera();
                SetInputClass(INPUTCLASS_GAME, 1);
            }
            if (GetControlledCharacterDead() != 0) {
                if (m_nGameOverReason != 0) {
                    HurryDeathSequence();
                    return 0;
                }
                CSWGuiMessageBox* pBox = m_pInGameGui->m_pSoloModeQueryBox;
                pBox->SetAllowCancel(0);
                pBox->SetCallback(0, 0, 0);
                pBox->SetMessage(STRREF_START_MENU_UNCONSCIOUS);
                m_pSWGuiManager->AddPanel(pBox, 1, 1);
                return 0;
            }
            if (m_pInGameGui->m_nGlobalDialogState != 0)
                goto soloFallback;
            // Solo panel toggle through the SW party's player character.
            if (g_pAppManager->m_pClientExoApp->GetSWParty() == 0)
                goto soloFallback;
            if (g_pAppManager->m_pClientExoApp->GetSWParty()
                    ->GetPlayerCharacter() == 0)
                goto soloFallback;
            if (m_pInGameGui->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_COVERED) {
                if (m_pInGameGui->HideSWInGameGui(0) != 0)
                    goto setInput01;
                goto soloFallback;
            }
            if (m_inputClass != INPUTCLASS_GAME)
                goto soloFallback;
            // A failed ShowSWInGameGui of the options screen forwards the menu toggle.
            if (m_pInGameGui->ShowSWInGameGui(CGuiInGame::SCREEN_OPTIONS) == 0)
                goto soloFallback;
            SetInputClass(INPUTCLASS_GUI, 1);
            g_pAppManager->m_pClientExoApp->ShowTutorialWindow(
                7, OBJECT_INVALID, OBJECT_INVALID, 0);
            return 0;
        soloFallback:
            // The free-look camera returns early, otherwise the menu toggle is forwarded.
            if (bWasFreeLook)
                return 0;
            m_pSWGuiManager->HandleInputEvent(INPUTEVENT_KEY_ESCAPE, 1);
            return 0;
        }
        return 0;

    // Retrieve mode.
    case INPUTEVENT_FREELOOK_EXIT: case INPUTEVENT_FREELOOK_EXIT_ALT:
        if (nParam != 0)
            goto retrieve;
        return 0;

#if KOTOR1
    case INPUTEVENT_MINIGAME_MOUSE_LEFT: case INPUTEVENT_MG_SHOOT:
        if (nParam != 0 && m_pModule != 0 && m_pModule->m_pArea != 0 &&
            GetPausedByCombat() == 0) {
            CSWMiniGame* pGame = m_pModule->m_pArea->m_pMiniGame;
            if (pGame == 0)
                return 0;
            if (pGame->m_pPlayer == 0)
                return handled;
            static_cast<CSWMiniPlayer*>(pGame->m_pPlayer)->Fire();
            return 0;
        }
        return 0;

#else
    // Driving accelerate.
    case INPUTEVENT_MINIGAME_ACCELERATE: case INPUTEVENT_MINIGAME_MOUSE_LEFT:
        if (nParam != 0 && m_pModule != 0 && m_pModule->m_pArea != 0 &&
            GetPausedByCombat() == 0) {
            CSWMiniGame* pGame = m_pModule->m_pArea->m_pMiniGame;
            if (pGame == 0)
                return 0;
            if (pGame->m_pPlayer == 0)
                return handled;
            if (pGame->m_type == 2)
                static_cast<CSWMiniPlayer*>(pGame->m_pPlayer)->Fire();
            else
                pGame->m_pPlayer->OnAccelerate(0);
            return 0;
        }
        return 0;

    // Driving brake.
    case INPUTEVENT_MINIGAME_MOUSE_RIGHT: case INPUTEVENT_MG_SHOOT:
        if (nParam != 0 && m_pModule != 0 && m_pModule->m_pArea != 0 &&
            GetPausedByCombat() == 0) {
            CSWMiniGame* pGame = m_pModule->m_pArea->m_pMiniGame;
            if (pGame == 0)
                return 0;
            if (pGame->m_pPlayer == 0)
                return handled;
            if (pGame->m_type == 2)
                static_cast<CSWMiniPlayer*>(pGame->m_pPlayer)->Fire();
            else
                pGame->m_pPlayer->OnBrake(0);
            return 0;
        }
        return 0;
#endif

    // Select the nearest object, inverted.
    case INPUTEVENT_PAD_SELECT_NEXT: case INPUTEVENT_SELECT_NEXT:
        if (nParam != 0) {
#if !KOTOR1  // K1's main interface has no party item-use selection
            if (g_pAppManager->m_pClientExoApp->GetInGameGui() != 0) {
                CSWGuiMainInterface* pMain = g_pAppManager->m_pClientExoApp
                                                 ->GetInGameGui()
                                                 ->m_pMainInterface;
                if (pMain != 0)
                    pMain->m_bPartyItemUsePending = 0;
            }
#endif
            if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
                goto camRestore;
            SelectNearestObject(1, (m_combatMode != 0) << 1);
            if (m_combatMode != 0 && m_pClientOptions->m_bAutoPauseNewTarget)
                SetAutoPaused(1, 8);
            return 0;
        }
        return 0;

    // Select the nearest object.
    case INPUTEVENT_PAD_SELECT_PREV: case INPUTEVENT_SELECT_PREV:
        if (nParam != 0) {
#if !KOTOR1  // K1's main interface has no party item-use selection
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface != 0)
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface
                    ->m_bPartyItemUsePending = 0;
#endif
            if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
                goto camRestore;
            SelectNearestObject(0, (m_combatMode != 0) << 1);
            if (m_combatMode != 0 && m_pClientOptions->m_bAutoPauseNewTarget)
                SetAutoPaused(1, 8);
            return 0;
        }
        return 0;

    // Solo-mode query.
    case INPUTEVENT_PAD_PARTY_ACTIVE: case INPUTEVENT_PARTY_ACTIVE:
        if (nParam != 0) {
            if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
                goto camRestore;
            if (m_pInGameGui != 0) {
                if (m_pInGameGui->m_bInGameMenuShown == 0)
                    m_pInGameGui->ShowSoloModeQuery(0);
                return 0;
            }
            return handled;
        }
        return 0;

    // Leave combat, or quit swoop.
    case INPUTEVENT_CANCEL_COMBAT:
        if (nParam != 0) {
#if !KOTOR1
            CSWMiniGame* pGame =
                static_cast<CSWMiniGame*>(g_pAppManager->m_pClientExoApp->GetMiniGame());
            if (pGame != 0 && g_pAppManager->m_pClientExoApp->GetPausedByCombat() != 0) {
                if (pGame->m_type == 1) {
                    CSWGlobalVariableTable* pGlobals =
                        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
                    pGlobals->SetValueBoolean(CExoString("GBL_QUIT_SWOOP"), 1);
                    g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 4, 0);
                    g_pAppManager->m_pClientExoApp->SetAutoPaused(0, 0);
                }
                return 0;
            }
#endif
            // Nothing in combat, so drop combat mode on the party leader.
            if (g_pAppManager->m_pClientExoApp->GetCombatMode() != 0) {
                CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()
                                        ->GetPlayerCharacter();
                if (pPC != 0) {
                    pPC->SetCombatMode(0);
                    static_cast<CSWCObject*>(pPC)->ClearAllActions();
                    return 0;
                }
                return handled;
            }
            return handled;
        }
        return 0;

    // Execute the queued default action.
    case INPUTEVENT_DEFAULT_ACTION:
#if KOTOR1
        if (nParam != 0 && !IosActionMenuUp && m_lastTarget != OBJECT_INVALID) {
#else
        if (nParam != 0 && m_lastTarget != OBJECT_INVALID) {
#endif
            GetDefaultActions();
            if (m_actions.m_count > 0 && m_actions.m_pArray != 0) {
                CSWGuiInterfaceAction& act = m_actions.m_pArray[0];
                CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()
                                        ->GetPlayerCharacter();
                CGameObject* pObj =
                    g_pAppManager->m_pClientExoApp->GetGameObject(act.m_nObjectId);
                if (pObj == 0)
                    return handled;
                CSWCObject* pSWC = pObj->AsSWCObject();
                if (pSWC == 0)
                    return handled;
                CSWCCreature* pTarget = static_cast<CSWCCreature*>(pSWC);
                (pTarget->*act.m_pfnAction)(act.m_nActionId, pPC);
                return 0;
            }
            return 0;
        }
        return 0;

    // Flourish weapons.
    case INPUTEVENT_FLOURISH:
        if (nParam != 0)
            g_pAppManager->m_pClientExoApp->PlayerFlourishWeapons();
        return 0;

    // Restore mouse X, clamped to the screen width.
    case INPUTEVENT_MOUSE_X:
#if KOTOR1
        if (abs(s_nLButtonDownX - m_restoreMouseX) > g_nMouseDragThreshold)
            s_bMouseDragged = true;
#endif
        if (m_restoreMouseX != nParam) {
            int nCandidate = nParam & ((static_cast<int>(nParam) >> 31) ^ 0xffffffff);
#if KOTOR1
            m_restoreMouseX =
                (nCandidate < g_nScreenWidth - 1) ? nCandidate : g_nScreenWidth - 1;
#else
            m_restoreMouseX =
                (nCandidate <= g_nScreenWidth - 1) ? nCandidate : g_nScreenWidth - 1;
#endif
            return 1;
        }
        return 0;

    // Restore mouse Y, clamped to the screen height and inverted.
    case INPUTEVENT_MOUSE_Y:
    {
#if KOTOR1
        if (abs(s_nLButtonDownX - m_restoreMouseX) > g_nMouseDragThreshold)
            s_bMouseDragged = true;
#endif
        int nY = g_nScreenHeight + ~nParam;
        if (m_restoreMouseY != nY) {
            nY = nY & ((static_cast<int>(nY) >> 31) ^ 0xffffffff);
#if KOTOR1
            m_restoreMouseY = (nY < g_nScreenHeight - 1) ? nY : g_nScreenHeight - 1;
#else
            m_restoreMouseY = (nY <= g_nScreenHeight - 1) ? nY : g_nScreenHeight - 1;
#endif
            return 1;
        }
        return 0;
    }

    // L button up/down, with the double-click camera restore.
    case INPUTEVENT_MOUSE_LEFT:
    {
#if KOTOR1
        if (nParam == 0)
            PerformLButtonUpAction();
        else
            PerformLButtonDownAction(nParam == -1);
        return 0;
#else
        long long snapshot = g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer
                                 ->GetSnapshotTime();
        if (nParam == 0) {
            if ((static_cast<unsigned long long>(snapshot - g_lLastLButtonSnapshot) >> 6) <= 3124 &&
                m_nGameOverReason == 0 && m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
                g_bShowIndicator = 1;
                GetModule()->RestoreCamera();
                SetInputClass(INPUTCLASS_GAME, 1);
            }
            PerformLButtonUpAction();
#ifdef K2_UI_TOUCH
            // A lifted finger hovers nothing, so the GUI cursor parks at (0,0). With a mouse the
            // pointer is still over the control just clicked, and parking it sends that control a
            // mouse-out, which reverts a behaviour or stance box on the PC HUD the instant its
            // arrow cycles it. The (-1,-1) restore point goes too: the next frame's cursor reset
            // re-sends it as a HandleMouseMove.
            m_pSWGuiManager->HandleMouseMove(0, 0);
            m_restoreMouseX = -1;
            m_restoreMouseY = -1;
#elif defined(K2_UI_XBOX)
            // A tap arrives as the mouse, but a lifted finger hovers nothing: the cursor leaves the
            // screen and the pad's focus is the only highlight. A mouse or trackpad keeps its
            // pointer where it is.
            if (k2shim_touch_click()) {
                m_pSWGuiManager->HandleMouseMove(-1, -1);
                m_restoreMouseX = -1;
                m_restoreMouseY = -1;
            }
#endif
            return handled;
        }
        g_lLastLButtonSnapshot = snapshot;
        PerformLButtonDownAction(nParam == -1);
        return 0;
#endif
    }

    // R button up/down, the rotate toggle and mouse grab.
    case INPUTEVENT_MOUSE_RIGHT:
#if KOTOR1
        if (nParam == 0) {
            g_bMouseCameraRotateToggle = 0;
            g_pGuiMan->HandleRMouseUp();
        } else {
            g_bMouseCameraRotateToggle = 1;
            g_pGuiMan->HandleRMouseDown();
        }
        return 0;
#else
        if (g_pGuiMan->PanelExists(g_pAppManager->m_pClientExoApp->GetInGameGui()
                                       ->m_pMainInterface) == 0)
            return handled;
        if (g_pGuiMan->m_modalPanels.m_count != 0)
            return 0;
        if (nParam == 0) {
            g_bMouseCameraRotateToggle = 0;
            MSG_Mac::CaptureMouse(g_pAppManager->m_pClientExoApp->GetClientOptions()
                                      ->m_bGameplay1);
            g_pGuiMan->HandleRMouseUp();
        } else {
            g_bMouseCameraRotateToggle = 1;
            MSG_Mac::CaptureMouse(g_pAppManager->m_pClientExoApp->GetClientOptions()
                                      ->m_bGameplay1 == 0);
            g_pGuiMan->HandleRMouseDown();
        }
        return 0;

    // Mouse wheel.
#endif

    case INPUTEVENT_MOUSE_WHEEL:
        g_pGuiMan->HandleZMouse(nParam);
        return 0;

    // Screenshot.
    case INPUTEVENT_SCREENSHOT:
#if KOTOR1
        // K1 names the shot in this body: the first free KotOR%04i.tga, else KotOR%04i[a-z].tga.
        if (nParam != 0) {
            char szPath[1024];
            m_bScreenShotPending = 1;
            sprintf_s(m_sScreenShotFile, sizeof(m_sScreenShotFile), "KotOR%04i.tga", m_nScreenShotIndex);
            sprintf_s(szPath, sizeof(szPath), "{kApplicationSupportFolderType}\\%s", m_sScreenShotFile);
            FILE* pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/game/clientcore/clientinput.cpp",
                                 "HandleInputEvent", 2275, szPath, "rb");
            if (pFile != 0) {
                unsigned char c = 'a';
                do {
                    fclose(pFile);
                    sprintf_s(m_sScreenShotFile, sizeof(m_sScreenShotFile), "KotOR%04i%c.tga", m_nScreenShotIndex, c);
                    sprintf_s(szPath, sizeof(szPath), "{kApplicationSupportFolderType}\\%s", m_sScreenShotFile);
                    pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/game/clientcore/clientinput.cpp",
                                   "HandleInputEvent", 2283, szPath, "rb");
                    c++;
                } while (c <= 'z' && pFile != 0);
                // Every lettered name is taken, so there is no shot, and the last handle is not
                // closed.
                if (c > 'z')
                    m_bScreenShotPending = 0;
            }
            m_nScreenShotIndex++;
        }
        return 0;
#else
        if (nParam != 0)
            DoScreenShot();
        return 0;
#endif

    // Target action while the action menu is open.
    case INPUTEVENT_KEY_RETURN:
        if (nParam != 0 && m_inputClass == INPUTCLASS_GUI)
            m_pSWGuiManager->HandleInputEvent(INPUTEVENT_KEY_RETURN, nParam);
        return 0;

    // In-game primary and secondary ability keys.
    case INPUTEVENT_MOVE_FORWARD:
        if (m_pInGameGui != 0 && m_pInGameGui->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_SHOWN) {
            CSWGuiMainInterface* pMain = m_pInGameGui->m_pMainInterface;
            if (pMain != 0) {
                nAbility = INPUTEVENT_GUI_KEY_UP;
                goto abilityCall;
            }
            return handled;
        }
        return 0;
    case INPUTEVENT_MOVE_BACK:
        if (m_pInGameGui != 0 && m_pInGameGui->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_SHOWN) {
            CSWGuiMainInterface* pMain = m_pInGameGui->m_pMainInterface;
            if (pMain != 0) {
                nAbility = INPUTEVENT_GUI_KEY_DOWN;
                goto abilityCall;
            }
            return handled;
        }
        return 0;

    // Tooltip instant mode.
    case INPUTEVENT_TOOLTIPS:
        if (nParam != 0) {
            SetToolTipInstantMode(1);
            return 0;
        }
        SetToolTipInstantMode(0);
        return 0;

    // Target-action menu left, right and down.
    case INPUTEVENT_TARGET_LEFT_ACTION:
        if (nParam != 0) {
            CSWGuiMainInterface* pMain =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
            if (g_bShiftActionState == 0)
                pMain->DoTargetAction(0, 1);
            else
                pMain->m_targetActionMenu.SelectNextAction(0);
        }
        return 0;
    case INPUTEVENT_TARGET_MIDDLE_ACTION:
        if (nParam != 0) {
            CSWGuiMainInterface* pMain =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
            if (g_bShiftActionState == 0)
                pMain->DoTargetAction(1, 1);
            else
                pMain->m_targetActionMenu.SelectNextAction(1);
        }
        return 0;
    case INPUTEVENT_TARGET_RIGHT_ACTION:
        if (nParam != 0) {
            CSWGuiMainInterface* pMain =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
            if (g_bShiftActionState == 0)
                pMain->DoTargetAction(2, 1);
            else
                pMain->m_targetActionMenu.SelectNextAction(2);
        }
        return 0;

    // Personal-action menu left, right, down and up.
    case INPUTEVENT_PERSONAL_POWER_ACTION:
        if (nParam != 0) {
            CSWGuiMainInterface* pMain =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
            if (g_bShiftActionState == 0)
                pMain->DoPersonalAction(0, 1);
            else
                pMain->SelectNextPersonalAction(0);
        }
        return 0;
    case INPUTEVENT_PERSONAL_MEDICAL_ACTION:
        if (nParam != 0) {
            CSWGuiMainInterface* pMain =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
            if (g_bShiftActionState == 0)
                pMain->DoPersonalAction(1, 1);
            else
                pMain->SelectNextPersonalAction(1);
        }
        return 0;
    case INPUTEVENT_PERSONAL_MINES_ACTION:
        if (nParam != 0) {
            CSWGuiMainInterface* pMain =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
            if (g_bShiftActionState == 0)
                pMain->DoPersonalAction(3, 1);
            else
                pMain->SelectNextPersonalAction(3);
        }
        return 0;
    case INPUTEVENT_PERSONAL_OTHER_ACTION:
        if (nParam != 0) {
            CSWGuiMainInterface* pMain =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
            if (g_bShiftActionState == 0)
                pMain->DoPersonalAction(2, 1);
            else
                pMain->SelectNextPersonalAction(2);
        }
        return 0;

    // Quick load.
    case INPUTEVENT_QUICKLOAD:
        if (nParam != 0 && g_pAppManager->m_pLoadGameErrorInfo->m_flag == 0) {
            g_pExoInput->ClearEvents();
            g_bLeftMouseDown = 0;
            g_bMouseCameraRotateToggle = 0;
#if !KOTOR1
            MSG_Mac::CaptureMouse(g_pAppManager->m_pClientExoApp->GetClientOptions()
                                      ->m_bGameplay1);
#endif
            g_pAppManager->m_pClientExoApp->ResetDriveAcceleration();
            g_pAppManager->m_pClientExoApp->GetInGameGui()->DoQuickLoad();
        }
        return 0;

    // Quick save.
    case INPUTEVENT_QUICKSAVE:
        if (nParam != 0) {
            g_pExoInput->ClearEvents();
            g_pAppManager->m_pClientExoApp->ResetDriveAcceleration();
            DoQuickSave();
            g_pExoInput->CoolDownEvent(INPUTEVENT_QUICKLOAD, 1000);
        }
        return 0;

    // Queue the active action.
    case INPUTEVENT_ACTION_MENU_QUEUE:
        if (nParam != 0 && m_pInGameGui != 0) {
            CSWGuiMainInterface* pMain = m_pInGameGui->m_pMainInterface;
            if (pMain != 0) {
                pMain->QueueActiveAction();
                return 0;
            }
            return handled;
        }
        return 0;

    // Party stealth toggle.
    case INPUTEVENT_STEALTH:
        if (nParam != 0) {
            if (g_pAppManager->m_pClientExoApp->GetPlayerCreature()
                    ->IsStealthCapable() != 0) {
                m_pParty->GetPlayerCharacter()->PartyMenuStealth(0, 0);
                return 0;
            }
            return handled;
        }
        return 0;

    // Swap weapons.
#if !KOTOR1
    case INPUTEVENT_SWITCH_WEAPONS:
        if (nParam != 0) {
            g_pAppManager->m_pClientExoApp->GetInGameGui()
                ->m_pMainInterface->OnSwapWeaponsPressed(0);
        }
        return 0;
#endif

    // Pause toggle, the mirror of the pause case above.
    case INPUTEVENT_PAUSE_ALT:
        if (nParam != 0) {
            __asm__ __volatile__("" ::: "memory");  // idiom-exempt: the pause chain really is duplicated here
            if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
                g_bShowIndicator = 1;
                GetModule()->RestoreCamera();
                SetInputClass(INPUTCLASS_GAME, 1);
            }
            int bPaused = GetPausedByCombat();
            __asm__ __volatile__("" ::: "memory");  // idiom-exempt: the pause chain really is duplicated here
            SetPausedByCombat(bPaused == 0, 4, 0);
            __asm__ __volatile__("" ::: "memory");  // idiom-exempt: the pause chain really is duplicated here
            if (g_pAppManager->m_pClientExoApp->GetAutoPaused() != 0)
                g_pAppManager->m_pClientExoApp->SetAutoPaused(0, 0);
            if (bPaused != 0)
                goto playSound;
            if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(
                    6, OBJECT_INVALID, OBJECT_INVALID, 0) != 0)
                return handled;
            goto playSound;
        }
        return 0;
    }
    return handled;

abilityCall:
    m_pInGameGui->m_pMainInterface->HandleInputEvent(nAbility, nParam);
    return 0;

camRestore:
    // Show the indicator, restore the camera, then set input class 0.
    g_bShowIndicator = 1;
    GetModule()->RestoreCamera();
setInput01:
    SetInputClass(INPUTCLASS_GAME, 1);
    return 0;

retrieve:
    // The free-look camera goes through the camera restore above.
    if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
        goto camRestore;
    return 0;

playSound:
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PlayGuiSound(GUISOUND_ACTION_MENU_USE);
    return 0;
}

// Replays a recorded input capture, one record at a time.
void CClientExoAppInternal::PlayBackInputEvents()
{
#if KOTOR1
    // K1's input playback is an empty stub.
#else
    if (m_playbackEvents.m_pInternal->m_pHead == 0) {
        m_captureEventPlayBack = 0;
        m_pWorldTimer->SetFixedSnapshotRate(0, 30);
    } else {
        do {
            CPlayBackEventNode* pEvt = m_playbackEvents.GetAtPos(m_playbackEvents.m_pInternal->m_pHead);
            long long lTime = static_cast<long long>(pEvt->m_nTime);
            unsigned long long nSnap = static_cast<unsigned long long>(
                g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetSnapshotTime());
            if (nSnap / 1000 <= static_cast<unsigned long long>(lTime))
                break;
            pEvt = m_playbackEvents.RemoveHead();
            if (pEvt->m_type == 'K') {
                HandleWMKeyDownMessage(0, static_cast<unsigned long long>(pEvt->m_wmParam), 0);
            }
            if (pEvt->m_type == 'C') {
                HandleWMCharMessage(pEvt->m_wmParam);
            }
            if (pEvt->m_type == 'L') {
                g_pAppManager->m_pClientExoApp->GetNetLayer()->SetUpPlayBackConnection();
            }
            if (pEvt->m_type == 'M') {
                HandleMessage(pEvt->m_nMsgA, pEvt->m_pMsgData, pEvt->m_nMsgB, pEvt->m_nMsgC);
                if (pEvt->m_pMsgData)
                    delete pEvt->m_pMsgData;
            }
            if (pEvt->m_type == 'I') {
                HandleInputEvent(pEvt->m_nCX, pEvt->m_nCY);
            }
            if (pEvt->m_type == 'Z') {
                m_moveTypeMode = 1;
                HandleInputEvent(pEvt->m_nCX, pEvt->m_nCY);
            }
            if (pEvt->m_type == 'V') {
                CAurCamera* pCam = GetModule()->m_pCamera;
                char aBuf[64];
                CameraPosMessage(aBuf, 0, "dist %f", static_cast<double>(pEvt->m_fA));
                pCam->BehaviorMessage(aBuf);
                CameraPosMessage(aBuf, 0, "pitch %f", static_cast<double>(pEvt->m_fB));
                pCam->BehaviorMessage(aBuf);
                CameraPosMessage(aBuf, 0, "yaw %f", static_cast<double>(pEvt->m_fC));
                pCam->BehaviorMessage(aBuf);
            }
            if (pEvt->m_type == 'Y') {
                Vector vPos;
                vPos.x = pEvt->m_fA;
                vPos.y = pEvt->m_fB;
                vPos.z = pEvt->m_fC;
                unsigned long long nOid = pEvt->m_oid;
                CGameObject* pObj;   // deliberately not zero-initialised: an uninitialised out-parameter is passed in
                if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(nOid, &pObj) ==
                    CGameObjectArray::SUCCESS) {
                    if (pObj->AsSWCCreature()) {
                        pObj->AsSWCCreature()->SetPosition(vPos);
                        pObj->AsSWCCreature()->m_desiredPosition = vPos;
                    }
                }
            }
            delete pEvt;
        } while (m_playbackEvents.m_pInternal->m_pHead != 0);
    }
#endif
}

// Switches the input class, reverting to the previous class when a mini-game class cannot find
// a live mini-game.
void CClientExoAppInternal::SetInputClass(int nClass, int nFlag)
{
    int cur = nClass;
    int curb = m_inputClass;
    int flag = nFlag;
    if (curb == nClass)
        return;
    goto store_class;

retry:
    curb = m_inputClass;
    flag = 1;
    if (curb == cur)
        return;
store_class:
    m_inputClass = cur;
    g_pExoInput->ClearEvents();
    int d = cur;
    cur = curb;

    switch (d) {
    case INPUTCLASS_MINIGAME: {
        CSWMiniGame* pMini = 0;
        if (m_pModule != 0) {
            if (CSWCArea* pArea = m_pModule->m_pArea)
                pMini = pArea->m_pMiniGame;
        }
        if (pMini != 0) {
            IndicateObject(0, 0);
            m_mouseHideMask |= 1;
            if (g_pExoInput) g_pExoInput->HideMouse();
            if (m_pSWGuiManager) m_pSWGuiManager->HideSoftwareMouse();
            if (m_pInGameGui) m_pInGameGui->SetSWGuiStatus(CGuiInGame::SWGUI_TOGGLE_OFF, 0);
            pMini->SetCamera(m_pModule->m_pCamera);
            pMini->Go();
            return;
        }
        // With no live mini-game for class 1, revert and re-run the previous class.
        goto retry;
    }
    case INPUTCLASS_GAME: {
        if (m_sceneCreatureId != OBJECT_INVALID) {
            CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
            if (CSWSCreature* pCreature =
                    pServer->GetCreatureByGameObjectID(pServer->GetPlayerCreatureId())) {
                SetPlayerCreature(g_pAppManager->m_pClientExoApp
                                      ->ServerToClientObjectId(pCreature->m_objectId));
                m_sceneCreatureId = OBJECT_INVALID;
                AttachPlayerGameObjectToArea();
            }
        }
        if (m_pModule != 0 && m_pModule->m_pCamera != 0) {
            Vector vLook(0.0f, 0.0f, 0.0f);
            if (CSWCCreature* pPlayer = GetPlayerCreature()) {
                pPlayer = GetPlayerCreature();
                vLook = pPlayer->m_orientation;
            }
            CSWCameraOnAStick* pStick =
                static_cast<CSWCameraOnAStick*>(m_pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
            if (pStick != 0) {
                pStick->m_nReserved5 = 0;
                pStick->m_vPlayerFacing = vLook;
            }
        }
        if (CSWCCreature* pPlayer = GetPlayerCreature()) {
            pPlayer = GetPlayerCreature();
            pPlayer->SwitchMode(CSWCCreature::MODE_SWDRIVE);
        }
        if (m_pPlayerControl != 0)
            m_pPlayerControl->SetEnabled(1);
        if ((m_mouseHideMask &= ~MOUSEHIDE_MINIGAME) == 0 && m_fMouseShowTimer <= 0.0f) {
            g_pExoInput->ShowMouse();
            if (m_pSWGuiManager != 0 && !m_pClientOptions->m_bHardwareMouseCursor)
                m_pSWGuiManager->ShowSoftwareMouse();
            g_pExoInput->RefreshMouseCursor(g_bLeftMouseDown);
        }
        m_pInGameGui->SetSWGuiStatus(CGuiInGame::SWGUI_UNCOVER, 1);
        m_pInGameGui->m_bInGameMenuShown = 0;
        return;
    }
    case INPUTCLASS_DIALOG:
        IndicateObject(0, 0);
        return;
    case INPUTCLASS_GUI:
        if (m_pInGameGui != 0 && m_pInGameGui->m_swGuiToggleState == CGuiInGame::SWGUI_STATE_SHOWN) {
            if (flag != 0)
                m_pInGameGui->SetSWGuiStatus(CGuiInGame::SWGUI_COVER, 1);
            if ((m_mouseHideMask &= ~MOUSEHIDE_MINIGAME) == 0 && m_fMouseShowTimer <= 0.0f) {
                g_pExoInput->ShowMouse();
                if (m_pSWGuiManager != 0 && !m_pClientOptions->m_bHardwareMouseCursor)
                    m_pSWGuiManager->ShowSoftwareMouse();
                g_pExoInput->RefreshMouseCursor(g_bLeftMouseDown);
            }
        }
        m_pInGameGui->m_bInGameMenuShown = 1;
        return;
    default:
        return;
    }
}

// Picks the cursor to show for the object under the mouse.
void CClientExoAppInternal::HandleMouseOver(unsigned long long nObjectID, const Vector& rPos, int bPick)
{
    m_mouseOverPos = rPos;
    m_objectMouseOver = nObjectID;

    // When the in-game GUI is capturing the mouse (text entry, the target-action menu, the
    // free-look camera), show the plain arrow and get out.
    if (m_pInGameGui != 0 &&
        (m_pInGameGui->m_bInGameMenuShown != 0 || m_pInGameGui->m_nGlobalDialogState == 1 ||
         m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK))
    {
        int nGuiCursor = MOUSECURSOR_DEFAULT;
        if (g_pExoInput != 0)
            nGuiCursor = g_pExoInput->SetMouseCursor(MOUSECURSOR_DEFAULT, 0, 0);
        if (m_pSWGuiManager != 0)
            m_pSWGuiManager->SetSoftwareMouseCursor(nGuiCursor, 0, 0);
        return;
    }

    int nCursor = MOUSECURSOR_DEFAULT;
    int nDown = 0;
    if (bPick != 0) {
    build_actions:
        GetDefaultActions();
        CSWGuiInterfaceAction* pFirst =
            (m_actions.m_count >= 1) ? &m_actions.m_pArray[0] : 0;
        nDown = g_bLeftMouseDown;
        // Follow the first action's icon only while the mouse-over is still the in-game target
        // (or the no-target sentinel); otherwise show the walk cursor.
        if (m_objectMouseOver != OBJECT_INVALID &&
            m_objectMouseOver != m_pInGameGui->m_pMainInterface->m_targetId)
            goto show_walk;
        if (pFirst == 0)
            nCursor = MOUSECURSOR_DEFAULT;
        else {
            nCursor = MOUSECURSOR_ATTACK;
            switch (pFirst->m_nActionId & INTERFACE_ACTION_ID_MASK) {
            case INTERFACE_ACTION_USE_OBJECT:
            case INTERFACE_ACTION_USE_PLACEABLE: nCursor = MOUSECURSOR_USE; break;
            case INTERFACE_ACTION_DIALOG: nCursor = MOUSECURSOR_TALK; break;
            case INTERFACE_ACTION_ATTACK: break;          // keeps the default cursor
            case INTERFACE_ACTION_OPEN_DOOR: nCursor = MOUSECURSOR_DOOR; break;
            case INTERFACE_ACTION_DISABLE_MINE: nCursor = MOUSECURSOR_DISABLE_MINE; break;
            case INTERFACE_ACTION_BASH: nCursor = MOUSECURSOR_BASH; break;
            case INTERFACE_ACTION_RECOVER_MINE: nCursor = MOUSECURSOR_RECOVER_MINE; break;
            default: nCursor = MOUSECURSOR_INVALID; break;
            }
        }
        goto show_cursor;
    }

    // For the normal per-frame hit results, a valid id runs the pick heuristics that decide which
    // object drives the cursor.
    if (nObjectID != OBJECT_INVALID) {
        CSWGuiControl* pMouseOverControl = g_pGuiMan->m_pMouseOverControl;
        if (m_lastTarget != OBJECT_INVALID) {
            unsigned long long nPlayerId =
                g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
            if (pMouseOverControl == 0 && nPlayerId != nObjectID) {
                nObjectID = m_objectMouseOver;
                if (m_lastTarget == nObjectID)
                    goto build_actions;          // the mouse-over is the target
                goto check_sentinel;
            } else {
                nObjectID = m_objectMouseOver;
            }
check_sentinel:
            if (nObjectID == OBJECT_INVALID)
                goto show_default;
        }
        // Backward scan of the visible-object table for the candidate id.
        // inlined CExoArrayList<CNearestStruct>::IndexOf(void*, int (*)(void*, CNearestStruct))
        if (m_visibleTable.m_count >= 1) {
            CNearestStruct* pRow = m_visibleTable.m_pArray;
            int64_t i = 0;
            do {
                if (pRow->m_objectId == nObjectID) {
                    if (pMouseOverControl == 0 && i != 1)
                        goto show_walk;
                    break;
                }
                i--;
                pRow++;
            } while (-i != m_visibleTable.m_count);
        }
    }

show_default:
    nDown = g_pExoInput->GetLastSetMouseDownState();
    nCursor = MOUSECURSOR_DEFAULT;
    goto show_cursor;

show_cursor:
    if (g_pExoInput != 0)
        nCursor = g_pExoInput->SetMouseCursor(nCursor, 0, nDown);
    if (m_pSWGuiManager != 0)
        m_pSWGuiManager->SetSoftwareMouseCursor(nCursor, 0, nDown);
    return;

show_walk:
    {
        int nWalkCursor =
            (g_pExoInput != 0) ? g_pExoInput->SetMouseCursor(MOUSECURSOR_SELECT, 0, 0) : MOUSECURSOR_SELECT;
        if (m_pSWGuiManager != 0)
            m_pSWGuiManager->SetSoftwareMouseCursor(nWalkCursor, 0, 0);
        return;
    }
}

// Handles the per-frame drive (vehicle and swoop) input, turning the held movement keys and the
// current animation into a drive-control packet, a waypoint walk, a turn in place or a strafe.
void CClientExoAppInternal::UpdateDriveMode()
{
    Vector vFacing;
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
#ifdef K2_HOST_BUILD
    if (pPlayer != 0) {
        static int s_nLastWhy = -1;
        bool bKeys = m_keyboardModalFlags[0] || m_keyboardModalFlags[1] || m_keyboardModalFlags[2] ||
                     m_keyboardModalFlags[3] || m_keyboardModalFlags[4] || m_keyboardModalFlags[5] ||
                     m_keyboardModalFlags[6];
        int nWhy = !bKeys ? -1 : (*reinterpret_cast<const unsigned char*>(&pPlayer->m_nAIStateFlags) & 2) == 0 ? 1
                 : pPlayer->m_bCommandable == 0 ? 2 : 0;
        if (nWhy >= 0 && nWhy != s_nLastWhy)
            fprintf(stderr, "k2: drive keys %s (0x404 %#x, 0x4dc %d, ai state %d)\n",
                    nWhy == 1 ? "ignored: 0x404 bit 2 clear" : nWhy == 2 ? "ignored: 0x4dc clear" : "sent",
                    static_cast<unsigned int>(*reinterpret_cast<const unsigned char*>(&pPlayer->m_nAIStateFlags)), static_cast<int>(pPlayer->m_bCommandable),
                    static_cast<int>(pPlayer->m_nAIStateAction));
        if (nWhy >= 0) s_nLastWhy = nWhy;
    }
#endif
    if (pPlayer == 0 || (*reinterpret_cast<const unsigned char*>(&pPlayer->m_nAIStateFlags) & 2) == 0 || pPlayer->m_bCommandable == 0)
        return;

    long long nNow = g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer->GetSnapshotTime();

    bool bDrive = false;
    if (m_bForceDriveUpdate != 0) {
        bDrive = true;
        m_drivingTimerReset = nNow;
    } else if (m_keyboardModalFlags[0] || m_keyboardModalFlags[1] || m_keyboardModalFlags[2] ||
               m_keyboardModalFlags[3] || m_keyboardModalFlags[4] || m_keyboardModalFlags[5] ||
               m_keyboardModalFlags[6]) {
        if (static_cast<unsigned int>(nNow) - static_cast<unsigned int>(m_drivingTimerReset) <= 499999) {
            if (static_cast<unsigned int>(nNow) - static_cast<unsigned int>(m_nLastDriveTime) < 250000)
                return;
        } else {
            bDrive = true;
            m_drivingTimerReset = nNow;
        }
    }

    unsigned char nOrigMode = m_moveTypeMode;
    m_moveTypeMode = 2;

    int nMoveBits;
    unsigned char nTag;
    if (m_keyboardModalFlags[1] != 0) {
        CSWCCreature* pOther = GetPlayerCreature();
        unsigned short nOtherFlags = pOther->m_nActivityFlags;
        if (pPlayer->m_bMovementSpeedLimited != 0)
            nMoveBits = DRIVE_WALK;
#if KOTOR1
        else if ((nOtherFlags & 1) != 0)  // K1: no stealth-run exemption
#else
        else if ((nOtherFlags & 1) != 0 && pPlayer->m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0)
#endif
            nMoveBits = DRIVE_WALK;
        else if ((nOtherFlags & 2) != 0)
            nMoveBits = DRIVE_WALK;
        else
            nMoveBits = DRIVE_RUN;
    } else if (m_keyboardModalFlags[0] != 0) {
        nMoveBits = (m_pClientOptions->m_bAlwaysRun != 0) ? DRIVE_RUN : DRIVE_WALK;
    } else if (m_keyboardModalFlags[2] == 0 && m_keyboardModalFlags[5] == 0 && m_keyboardModalFlags[6] == 0) {
        goto turn_in_place;
    } else {
        nMoveBits = (m_keyboardModalFlags[2] != 0) ? DRIVE_BACK : 0;
    }

    if (m_keyboardModalFlags[5] != m_keyboardModalFlags[6])
        nMoveBits |= (m_keyboardModalFlags[5] != 0) ? DRIVE_STRAFE_LEFT : DRIVE_STRAFE_RIGHT;

    if (m_keyboardModalFlags[3] != m_keyboardModalFlags[4]) {
        if (m_keyboardModalFlags[3] != 0)
            nMoveBits |= DRIVE_TURN_LEFT;
        else if (m_keyboardModalFlags[4] != 0)
            nMoveBits |= DRIVE_TURN_RIGHT;
    } else if (nMoveBits == 0) {
        goto exit_restore;
    }

    nTag = pPlayer->m_commandTag + 1;
    pPlayer->m_commandTag = nTag;

    if (pPlayer->m_nAIStateAction == 2) {
        if (m_keyboardModalFlags[1] != 0 || m_keyboardModalFlags[0] != 0) {
            if (static_cast<unsigned long long>(nNow) - m_nDriveKeyPressTime > 49)
                goto waypoint_else;
            Vector vDir = -pPlayer->m_orientation;
            vDir = normalize(vDir) * 0.5f;
            Vector vTarget = pPlayer->m_position - vDir;
            pPlayer->ComputeZValue(pPlayer->m_pArea, &vTarget, pPlayer->m_position.z, 0);
            m_pSWCMessage->SendPlayerToServerInput_WalkToWayPoint(
                pPlayer->m_pArea->m_objectId, vTarget.x, vTarget.y, vTarget.z,
                nTag, 0, 0, 0, OBJECT_INVALID);
            pPlayer->m_appliedCommandTag = nTag;
            goto exit_restore;
        }
        if (m_keyboardModalFlags[2] != 0) {
            CSWCCreature* pTarget =
                g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(pPlayer->m_oidCombatTarget);
            if (pTarget != 0) {
                float dx = pTarget->m_position.x - pPlayer->m_position.x;
                float dy = pTarget->m_position.y - pPlayer->m_position.y;
                float dz = pTarget->m_position.z - pPlayer->m_position.z;
                if (dx * dx + dy * dy + dz * dz <= 4.0f) {
                    Vector vDir = pPlayer->m_orientation;
                    vDir = normalize(vDir) * 0.5f;
                    Vector vTarget = pPlayer->m_position - vDir;
                    pPlayer->ComputeZValue(pPlayer->m_pArea, &vTarget, pPlayer->m_position.z, 0);
                    m_pSWCMessage->SendPlayerToServerInput_WalkToWayPoint(
                        pPlayer->m_pArea->m_objectId, vTarget.x, vTarget.y, vTarget.z,
                        nTag, 0, 0, 0, OBJECT_INVALID);
                    pPlayer->m_appliedCommandTag = nTag;
                    goto exit_restore;
                }
            }
            goto waypoint_else;
        }
        if (m_keyboardModalFlags[5] != m_keyboardModalFlags[6])
            goto turn_walk;
    }

waypoint_else:
    if (bDrive) {
        vFacing = pPlayer->m_orientation;
        double dFacing = atan2(static_cast<double>(vFacing.y), static_cast<double>(vFacing.x)) * 57.295776f;
        if (dFacing < 0.0)
            dFacing = dFacing + 360.0;
        int nHeading = static_cast<int>(dFacing * 10.0);
        m_pSWCMessage->SendPlayerToServerInput_DriveControl(pPlayer->m_position,
                                                            static_cast<unsigned short>(nHeading),
                                                            nTag, static_cast<unsigned char>(nMoveBits));
        double dAngle = static_cast<double>(static_cast<unsigned short>(nHeading)) / 10.0 * 0.017453292f;
        vFacing.x = static_cast<float>(cos(dAngle));
        vFacing.y = static_cast<float>(sin(dAngle));
        vFacing.z = 0.0f;
        vFacing.Normalize();
        pPlayer->SetOrientation(vFacing);
        pPlayer->SetDesiredWalkOrientation(vFacing);
        pPlayer->SetDesiredStationaryOrientation(vFacing);
    }
    DrivePlayer(static_cast<unsigned char>(nMoveBits));
    pPlayer->m_appliedCommandTag = nTag;
    goto exit_restore;

turn_walk:
    {
        Vector vDir;
        Vector vTarget;
        if (m_keyboardModalFlags[5] != 0)
            vDir = Vector(-pPlayer->m_orientation.y, pPlayer->m_orientation.x, pPlayer->m_orientation.z);
        else if (m_keyboardModalFlags[6] != 0)
            vDir = Vector(pPlayer->m_orientation.y, -pPlayer->m_orientation.x, pPlayer->m_orientation.z);
        vDir = normalize(vDir) * 0.5f;
        vTarget = vDir + pPlayer->m_position;
        pPlayer->ComputeZValue(pPlayer->m_pArea, &vTarget, pPlayer->m_position.z, 0);
        m_pSWCMessage->SendPlayerToServerInput_WalkToWayPoint(
            pPlayer->m_pArea->m_objectId, vTarget.x, vTarget.y, vTarget.z,
            nTag, 0, 0, 0, OBJECT_INVALID);
        pPlayer->m_appliedCommandTag = nTag;
        goto exit_restore;
    }

turn_in_place:
    if ((static_cast<CSWCObject*>(pPlayer)->GetCurrentAnimation() == SERVER_ANIMATION_PAUSE ||
         static_cast<CSWCObject*>(pPlayer)->GetCurrentAnimation() == SERVER_ANIMATION_READY ||
         static_cast<CSWCObject*>(pPlayer)->GetCurrentAnimation() == SERVER_ANIMATION_TURN1 ||
         static_cast<CSWCObject*>(pPlayer)->GetCurrentAnimation() == SERVER_ANIMATION_TURN1_BACKWARD ||
         static_cast<CSWCObject*>(pPlayer)->GetCurrentAnimation() == SERVER_ANIMATION_TURN2 ||
         static_cast<CSWCObject*>(pPlayer)->GetCurrentAnimation() == SERVER_ANIMATION_TURN2_BACKWARD)) {
        if (m_keyboardModalFlags[3] != 0 && m_keyboardModalFlags[4] == 0) {
            Vector vFacing =
                RotateByQuaternion(pPlayer->m_orientation, YawPitchRoll(30.0f, 0.0f, 0.0f));
            vFacing.Normalize();
            pPlayer->SetDesiredWalkOrientation(vFacing);
            pPlayer->SetDesiredStationaryOrientation(vFacing);
            m_pSWCMessage->SendPlayerToServerInput_TurnOnSpot(vFacing);
        } else if (m_keyboardModalFlags[4] != 0 && m_keyboardModalFlags[3] == 0) {
            Vector vFacing =
                RotateByQuaternion(pPlayer->m_orientation, YawPitchRoll(-30.0f, 0.0f, 0.0f));
            vFacing.Normalize();
            pPlayer->SetDesiredWalkOrientation(vFacing);
            pPlayer->SetDesiredStationaryOrientation(vFacing);
            m_pSWCMessage->SendPlayerToServerInput_TurnOnSpot(vFacing);
        }
    }

exit_restore:
    m_moveTypeMode = nOrigMode;
    m_nLastDriveTime = g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer->GetSnapshotTime();
    m_bForceDriveUpdate = 0;
}

// Turns the drive move bits and the player's facing into a walk vector and animation.
bool CClientExoAppInternal::DrivePlayer(unsigned char nMoveBits)
{
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pPlayer == 0 || (*reinterpret_cast<const unsigned char*>(&pPlayer->m_nAIStateFlags) & 2) == 0)
        return false;

    Vector vMove;
    unsigned short nAnimation;
    float fSpeed;
    unsigned char bWalkToPoint;

    if ((nMoveBits & DRIVE_STRAFE_LEFT) != 0) {
        nAnimation = SERVER_ANIMATION_STRAFE_LEFT;
        vMove = Vector(0.0f - pPlayer->m_orientation.y, pPlayer->m_orientation.x + 0.0f, 0.0f);
        fSpeed = 0.8f;
    } else if ((nMoveBits & DRIVE_STRAFE_RIGHT) != 0) {
        nAnimation = SERVER_ANIMATION_STRAFE_RIGHT;
        vMove = Vector(pPlayer->m_orientation.y + 0.0f, 0.0f - pPlayer->m_orientation.x, 0.0f);
        fSpeed = 0.8f;
    } else {
        nAnimation = SERVER_ANIMATION_WALK;
        fSpeed = 0.0f;
    }

    if ((nMoveBits & DRIVE_RUN) == DRIVE_RUN) {
        vMove.x = pPlayer->m_orientation.x + vMove.x;
        vMove.y = pPlayer->m_orientation.y + vMove.y;
        vMove.z = pPlayer->m_orientation.z + 0.0f;
        if ((nMoveBits & DRIVE_STRAFE_LEFT) != 0)
            nAnimation = SERVER_ANIMATION_RUN_STRAFE_LEFT;
        else if ((nMoveBits & DRIVE_STRAFE_RIGHT) != 0)
            nAnimation = SERVER_ANIMATION_RUN_STRAFE_RIGHT;
        else
            nAnimation = SERVER_ANIMATION_RUN;
        fSpeed = (pPlayer->m_fRunRate * 400.0f) / 1000000.0f;
        bWalkToPoint = 1;
    } else if ((nMoveBits & DRIVE_WALK) != 0) {
        vMove.x = pPlayer->m_orientation.x + vMove.x;
        vMove.y = pPlayer->m_orientation.y + vMove.y;
        vMove.z = pPlayer->m_orientation.z + 0.0f;
        if ((nMoveBits & DRIVE_STRAFE_LEFT) != 0)
            nAnimation = SERVER_ANIMATION_WALK_STRAFE_LEFT;
        else if ((nMoveBits & DRIVE_STRAFE_RIGHT) != 0)
            nAnimation = SERVER_ANIMATION_WALK_STRAFE_RIGHT;
        else
            nAnimation = SERVER_ANIMATION_WALK;
        fSpeed = (pPlayer->m_fWalkRate * 400.0f) / 1000000.0f;
        bWalkToPoint = 0;
    } else {
        bWalkToPoint = 0;
        if ((nMoveBits & DRIVE_BACK) != 0) {
            vMove.x = vMove.x - pPlayer->m_orientation.x;
            vMove.y = vMove.y - pPlayer->m_orientation.y;
            vMove.z = 0.0f - pPlayer->m_orientation.z;
            nAnimation = SERVER_ANIMATION_WALK_BACKWARD;
            fSpeed = (pPlayer->m_fWalkRate * 400.0f) / 1000000.0f;
        }
    }

    if ((nMoveBits & DRIVE_TURN_LEFT) != 0) {
        Quaternion q = YawPitchRoll(30.0f, 0.0f, 0.0f);
        vMove = RotateByQuaternion(vMove, q);
        Vector vOriented = RotateByQuaternion(pPlayer->m_orientation, q);
        pPlayer->SetDesiredWalkOrientation(vOriented);
    } else if ((nMoveBits & DRIVE_TURN_RIGHT) != 0) {
        Quaternion q = YawPitchRoll(-30.0f, 0.0f, 0.0f);
        vMove = RotateByQuaternion(vMove, q);
        Vector vOriented = RotateByQuaternion(pPlayer->m_orientation, q);
        pPlayer->SetDesiredWalkOrientation(vOriented);
    }

    vMove.Normalize();
    vMove = vMove * fSpeed;
    Vector vFinal = vMove + pPlayer->m_desiredPosition;
    pPlayer->ClientSideWalkCommand(vFinal, bWalkToPoint, m_moveTypeMode);
    pPlayer->SetAnimation(nAnimation, 0, 0);
    return true;
}

// Reads a recorded input capture back from the capture file.
void CClientExoAppInternal::ReadPlayBackInputEvents()
{
    char          szLine[80];
    int           nStart;
    unsigned char cType;
    int           nTime;
    int           nWm;
    int           nCX, nCY;
    int           nMsgA, nMsgC;
    unsigned int  nSize;
    uint64_t      nOid;
    float         fA, fB, fC;
    CExoFile*     pFile;
    int           bMsgStarted = 0;

    // The header line is read through its terminating newline; EOF aborts before any parse.
    char* pch = szLine;
    do {
        pFile = static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile());
        if (pFile->Read(pch, 1, 1) == 0)
            goto Exit;
    } while (*pch++ != '\n');

    // The first token of the header is the recording start time; every later time is a delta
    // from it.
#if KOTOR1 || defined(K2_FIX_ENGINE_BUGS)
    static_cast<void>(sscanf(&szLine[7], "%d", &nStart));
#else
    // The original passes cType as a second, unused argument.
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 4474)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif
    static_cast<void>(sscanf(&szLine[7], "%d", &nStart, &cType));
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
#endif

    while (true) {
        // Read the per-record prefix through its second space, then parse it.
        int nSpaces = 0;
        pch = szLine;
        do {
            pFile = static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile());
            if (pFile->Read(pch, 1, 1) == 0)
                goto Exit;
            if (*pch == ' ')
                nSpaces++;
            pch++;
        } while (nSpaces < 2);
        static_cast<void>(sscanf(szLine, "%d %c", &nTime, &cType));

        // Generic records are tags C, I, K and Z only.
        if (static_cast<unsigned int>(cType - 'C') <= 'Z' - 'C' &&
            ((1u << ((cType - 'C') & 31)) & kPlaybackRecordTags) != 0) {
            pch = szLine;
            do {
                pFile = static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile());
                if (pFile->Read(pch, 1, 1) == 0)
                    goto Exit;
            } while (*pch++ != '\n');

            CPlayBackEventNode* pEvt = new CPlayBackEventNode;
            pEvt->m_nTime = nTime - nStart;
            pEvt->m_type = cType;
            if ((cType | 8) == 'K') {   // 'C' or 'K': a WM code follows
                static_cast<void>(sscanf(szLine, "%d", &nWm));
                pEvt->m_wmParam = static_cast<unsigned short>(nWm);
            }
            if (cType == 'Z' || cType == 'I') {   // move-to and input coords
                static_cast<void>(sscanf(szLine, "%d %d", &nCX, &nCY));
                pEvt->m_nCX = nCX;
                pEvt->m_nCY = nCY;
            }
            m_playbackEvents.AddTail(pEvt);
        }

        if (cType == 'M') {
            // The first message of a capture inserts an 'L' link record first.
            if (!bMsgStarted) {
                CPlayBackEventNode* pLink = new CPlayBackEventNode;
                pLink->m_nTime = nTime - nStart;
                pLink->m_type = 'L';
                m_playbackEvents.AddTail(pLink);
                bMsgStarted = 1;
            }
            // Read through the third space, counting embedded newlines: a message may span lines.
            unsigned int nIdx = 0, nNewlines = 0, nSpaces3 = 0;
            do {
                pFile = static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile());
                if (pFile->Read(&szLine[nIdx], 1, 1) == 0)
                    goto Exit;
                unsigned char c = szLine[nIdx];
                nIdx++;
                if (c == '\n')
                    nNewlines++;
                if (c == ' ')
                    nSpaces3++;
            } while (nSpaces3 < 3);

#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)
            // K1 scans the 32-bit size with "%ld", which stores 64 bits at its address.
            static_cast<void>(sscanf(szLine, "%d %d %ld ", &nMsgA, &nMsgC, reinterpret_cast<int64_t*>(&nSize)));
#elif defined(K2_FIX_ENGINE_BUGS)
            static_cast<void>(sscanf(szLine, "%d %d %" SCNu32 " ", &nMsgA, &nMsgC, &nSize));
#else
            static_cast<void>(sscanf(szLine, "%d %d %d ", &nMsgA, &nMsgC, &nSize));
#endif
            unsigned char* pData = new unsigned char[nSize];
            pFile = static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile());
            if (pFile->Read(pData, nSize, 1) != nSize)
                goto Exit;
            if (nNewlines == 0) {
                // With no newline in the scan, swallow the rest of the single-line message.
                pch = &szLine[nIdx];
                do {
                    pFile = static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile());
                    if (pFile->Read(pch, 1, 1) == 0)
                        goto Exit;
                } while (*pch++ != '\n');
            }
            CPlayBackEventNode* pEvt = new CPlayBackEventNode;
            pEvt->m_nTime = nTime - nStart;
            pEvt->m_type = cType;
            pEvt->m_nMsgA = nMsgA;
            pEvt->m_nMsgC = nMsgC;
            pEvt->m_nMsgB = nSize;
            pEvt->m_pMsgData = pData;
            m_playbackEvents.AddTail(pEvt);
        }

        if (cType == 'V') {
            pch = szLine;
            do {
                pFile = static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile());
                if (pFile->Read(pch, 1, 1) == 0)
                    goto Exit;
            } while (*pch++ != '\n');
            CPlayBackEventNode* pEvt = new CPlayBackEventNode;
            pEvt->m_nTime = nTime - nStart;
            pEvt->m_type = cType;
            static_cast<void>(sscanf(szLine, "%f %f %f", &fA, &fB, &fC));
            pEvt->m_fA = fA;
            pEvt->m_fB = fB;
            pEvt->m_fC = fC;
            m_playbackEvents.AddTail(pEvt);
        }

        if (cType == 'Y') {
            pch = szLine;
            do {
                pFile = static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile());
                if (pFile->Read(pch, 1, 1) == 0)
                    goto Exit;
            } while (*pch++ != '\n');
            CPlayBackEventNode* pEvt = new CPlayBackEventNode;
            pEvt->m_nTime = nTime - nStart;
            pEvt->m_type = cType;
#ifdef K2_FIX_ENGINE_BUGS
            static_cast<void>(sscanf(szLine, "%" SCNx64 " %f %f %f", &nOid, &fA, &fB, &fC));
#elif KOTOR1
            static_cast<void>(sscanf(szLine, "%lx %f %f %f", &nOid, &fA, &fB, &fC));
#else
            // %x stores 32 bits: the id's high half keeps whatever the slot held.
            static_cast<void>(sscanf(szLine, "%x %f %f %f", reinterpret_cast<unsigned int*>(&nOid), &fA, &fB, &fC));
#endif
            pEvt->m_oid = nOid;
            pEvt->m_fA = fA;
            pEvt->m_fB = fB;
            pEvt->m_fC = fC;
            m_playbackEvents.AddTail(pEvt);
        }
    }

Exit:
    ;
}

// Formats a camera-state message for CAurCamera::BehaviorMessage.
#if !KOTOR1
static void CameraPosMessage(char* buf, int, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    __vsprintf_chk(buf, 0, 64, fmt, ap);
    __builtin_va_end(ap);
}
#endif

int CClientExoAppInternal::PerformXMouseMoveAction(int nX)
{
#if KOTOR1
    if (abs(s_nLButtonDownX - m_restoreMouseX) > g_nMouseDragThreshold)
        s_bMouseDragged = true;
#endif
    if (m_restoreMouseX == nX)
        return 0;
    int nBound = g_nScreenWidth - 1;
    int nClamped = nX < 0 ? 0 : nX;
#if KOTOR1
    m_restoreMouseX = nClamped < nBound ? nClamped : nBound;
#else
    m_restoreMouseX = nBound < nClamped ? nBound : nClamped;
#endif
    return 1;
}

int CClientExoAppInternal::PerformYMouseMoveAction(int nY)
{
#if KOTOR1
    // K1 tests the X distance here too.
    if (abs(s_nLButtonDownX - m_restoreMouseX) > g_nMouseDragThreshold)
        s_bMouseDragged = true;
#endif
    int nHeight = g_nScreenHeight;
    int nT = nHeight - nY - 1;
    if (m_restoreMouseY == nT)
        return 0;
    int nClamped = nT < 0 ? 0 : nT;
    int nBound = nHeight - 1;
#if KOTOR1
    m_restoreMouseY = nClamped < nBound ? nClamped : nBound;
#else
    m_restoreMouseY = nBound < nClamped ? nBound : nClamped;
#endif
    return 1;
}

void CClientExoAppInternal::PerformLButtonDownAction(int a)
{
#if KOTOR1
    s_bMouseDragged = false;
    s_nLButtonDownX = m_restoreMouseX;
#endif
    g_bMouseCameraRotateToggleIos = (m_pSWGuiManager->m_modalPanels.m_count == 0);
    g_bLeftMouseDown = 1;
    if (g_pExoInput)
        g_pExoInput->RefreshMouseCursor(1);
    if (m_pSWGuiManager)
        m_pSWGuiManager->SetSoftwareMouseCursor(m_pSWGuiManager->m_nSoftwareMouseCursor,
                                                (m_pSWGuiManager->m_nMouseStateFlags >> 2) & 1, 1);
    int nToggle = g_bMouseCameraRotateToggle;
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_bGameplay1 != 0 && (m_inputClass & ~1) != INPUTCLASS_GUI)
        nToggle = (nToggle == 0);   // the gamepad path overrides the toggle
    if (nToggle) {
        if ((m_inputClass & ~1) != INPUTCLASS_GUI)
            return;
    } else if (m_inputClass == INPUTCLASS_FREELOOK) {
        return;
    }
    if (a == 0)
        m_lastClickedOnTarget = m_lastTarget;
    m_pSWGuiManager->HandleLMouseDown(a);
}

void CClientExoAppInternal::PerformLButtonUpAction()
{
    g_bMouseCameraRotateToggleIos = 0;
    g_bLeftMouseDown = 0;
    if (g_pExoInput)
        g_pExoInput->RefreshMouseCursor(0);
    if (m_pSWGuiManager)
        m_pSWGuiManager->SetSoftwareMouseCursor(m_pSWGuiManager->m_nSoftwareMouseCursor,
                                                (m_pSWGuiManager->m_nMouseStateFlags >> 2) & 1, 0);
    int nToggle = g_bMouseCameraRotateToggle;
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_bGameplay1 != 0 && (m_inputClass & ~1) != INPUTCLASS_GUI)
        nToggle = (nToggle == 0);
    if (nToggle) {
        if ((m_inputClass & ~1) != INPUTCLASS_GUI)
            return;
    } else if (m_inputClass == INPUTCLASS_FREELOOK) {
        return;
    }
    K2_ASSUME(m_pSWGuiManager != 0);   // non-null for the whole input lifetime
    CSWGuiControl* pControl = m_pSWGuiManager->m_pMouseOverControl;
    if (pControl != 0 &&
        m_pInGameGui->m_pMainInterface->m_targetActionMenu
            .IsTargetActionMenuControl(pControl) == 1)
        goto mouseclick;
    if (g_pGuiMan->HandleLMouseUp() != 0)
        return;
mouseclick:
    if (g_pAppManager->m_pServerExoApp == 0)
        return;
#if KOTOR1
    if (s_bMouseDragged)
        return;
#endif
    HandleMouseClickInWorld();
}

void CClientExoAppInternal::PerformRButtonDownAction()
{
    g_bMouseCameraRotateToggle = 1;
#if !KOTOR1
    MSG_Mac::CaptureMouse(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay1 == 0);
#endif
    g_pGuiMan->HandleRMouseDown();
}

void CClientExoAppInternal::PerformRButtonUpAction()
{
    g_bMouseCameraRotateToggle = 0;
#if !KOTOR1
    MSG_Mac::CaptureMouse(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay1);
#endif
    g_pGuiMan->HandleRMouseUp();
}

#if KOTOR1
static void sprintf_s(char* buf, uint64_t n, const char* fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
}
#endif

void CClientExoAppInternal::PerformMButtonDownAction()
{
}

void CClientExoAppInternal::PerformMButtonUpAction()
{
}

void CClientExoAppInternal::PerformZMouseMoveAction(int zDelta)
{
    g_pGuiMan->HandleZMouse(zDelta);
}

// Builds the radial action menu for the current mouse-over target.
void CClientExoAppInternal::GetDefaultActions()
{
    CSWCCreature* pPlayerChar =
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    CSWSCreature* pPlayerServer = pPlayerChar ? pPlayerChar->GetServerCreature() : 0;

    unsigned long long nTargetId =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->m_targetId;
    CGameObject* pTarget = g_pAppManager->m_pClientExoApp->GetGameObject(nTargetId);
    if (pPlayerChar == 0)
        return;

    if (pPlayerServer != 0 && pPlayerServer->GetDead() == 0)
        pPlayerServer->GetDeadTemp();

    if (pTarget == 0)
        return;
    CSWCObject* pObj = pTarget->AsSWCObject();
    if (pObj == 0)
        return;

    int nType = pPlayerChar->GetInterfaceTargetType(pObj);

    for (int i = 0; i < m_actions.m_count; i++)
        m_actions.m_pArray[i].Clear();
    m_actions.SetSize(0);

    switch (nType) {
    case CSWCCreature::INTERFACE_TARGET_NONE: {
        // With no typeable target, a single inert entry keeps the ring rendering, with the callback
        // pair zeroed.
        m_actions.SetSize(1);
        K2_ASSUME(m_actions.m_pArray != 0);   // SetSize(1) allocated it
        m_actions.m_pArray[0].m_sName =
            g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NO_ACTION);
        m_actions.m_pArray[0].m_pfnAction = 0;
        m_actions.m_pArray[0].m_nObjectId = OBJECT_INVALID;
        m_actions.m_pArray[0].m_nActionId = INTERFACE_ACTION_NONE;
        m_actions.m_pArray[0].m_resRef = const_cast<char*>("i_noaction");
        break;
    }
    case CSWCCreature::INTERFACE_TARGET_TRIGGER: {
        // For a trigger, disable the armed mine when the player can use the recover skill, then
        // always offer to recover it.
        if (pObj->AsSWCTrigger() == 0)
            break;
        int nCount = 0;
        if (pPlayerChar->m_pStats->GetCanUseSkill(SKILL_DEMOLITIONS)) {
            if (pObj->AsSWCTrigger()->IsHostile()) {
                m_actions.SetSize(1);
                K2_ASSUME(m_actions.m_pArray != 0);   // SetSize allocated it
                m_actions.m_pArray[0].m_sName =
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ACTION_DISABLE_MINE);
                m_actions.m_pArray[0].m_pfnAction =
                    reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCTrigger::ActionMenuDisableMine);
                m_actions.m_pArray[0].m_nObjectId = pObj->m_objectId;
                m_actions.m_pArray[0].m_nActionId = INTERFACE_ACTION_DISABLE_MINE;
                m_actions.m_pArray[0].m_resRef = const_cast<char*>("i_disablemine");
                nCount = 1;
            }
        }
        if (pPlayerChar->m_pStats->GetCanUseSkill(SKILL_DEMOLITIONS)) {
            m_actions.SetSize(nCount + 1);
            K2_ASSUME(m_actions.m_pArray != 0);   // SetSize allocated it
            m_actions.m_pArray[nCount].m_sName =
                g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_RECOVER_MINE);
            m_actions.m_pArray[nCount].m_pfnAction =
                reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCTrigger::ActionMenuRecoverMine);
            m_actions.m_pArray[nCount].m_nObjectId = pObj->m_objectId;
            m_actions.m_pArray[nCount].m_nActionId = INTERFACE_ACTION_RECOVER_MINE;
            m_actions.m_pArray[nCount].m_resRef = const_cast<char*>("i_recovermine");
        }
        break;
    }
    case CSWCCreature::INTERFACE_TARGET_HOSTILE: {
        // Attack a hostile creature in a non-menu-disabled area.
        if (pObj->AsSWCCreature() == 0)
            break;
        CSWCCreature* pEnemy = pObj->AsSWCCreature();
        CSWSCreature* pEnemyServer = pEnemy->GetServerCreature();
        if (pEnemyServer == 0)
            break;
        if (static_cast<CSWSArea*>(pEnemyServer->GetArea())->m_nPlayerRestrictMode != 0)
            break;
        m_actions.SetSize(1);
        K2_ASSUME(m_actions.m_pArray != 0);   // SetSize(1) allocated it
        m_actions.m_pArray[0].m_sName =
            g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK);
        m_actions.m_pArray[0].m_pfnAction = &CSWCCreature::ActionMenuAttack;
        m_actions.m_pArray[0].m_nObjectId = pObj->m_objectId;
        m_actions.m_pArray[0].m_nActionId = INTERFACE_ACTION_ATTACK;
        m_actions.m_pArray[0].m_resRef = const_cast<char*>("i_attack");
        break;
    }
    case CSWCCreature::INTERFACE_TARGET_OPENABLE:
    case CSWCCreature::INTERFACE_TARGET_NONHOSTILE: {
        // A non-hostile creature, door or placeable gets the target-action menu.
        if (pObj->AsSWCCreature() != 0) {
#if KOTOR1  // K1 has no server-creature or talk-suppressing faction gate
            {
#else
            CSWCCreature* pTalkTo = pObj->AsSWCCreature();
            CSWSCreature* pTalkServer = pTalkTo->GetServerCreature();
            if (pTalkServer != 0 && pTalkServer->m_bConfused == 0) {
#endif
                m_actions.SetSize(1);
                K2_ASSUME(m_actions.m_pArray != 0);   // SetSize allocated it
                m_actions.m_pArray[0].m_sName =
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DIALOG);
                m_actions.m_pArray[0].m_pfnAction = &CSWCCreature::ActionInitiateDialog;
                m_actions.m_pArray[0].m_nObjectId = pObj->m_objectId;
                m_actions.m_pArray[0].m_nActionId = INTERFACE_ACTION_DIALOG;
                m_actions.m_pArray[0].m_resRef = const_cast<char*>("i_dialog");
            }
        }

        if (pObj->AsSWCPlaceable() != 0) {
            CSWSPlaceable* pServerPlaceable = pObj->AsSWCPlaceable()->GetServerPlaceable();
            int nAreaMenuDisabled = pObj->AsSWCPlaceable()->m_pArea->GetServerArea()->m_nPlayerRestrictMode;
            CExoString sName;
            if (pServerPlaceable != 0 && pServerPlaceable->m_bHasInventory != 0) {
                // An inventory placeable gets the "open/search" action from the placeables.2da
                // UseSearch row, then bash. A party member who is dead greys the entry, so dead
                // members cannot interact.
                int nEntry = 0;
                if (pObj->AsSWCPlaceable()->m_bIgnoreHitcheck != 0) {
                    m_actions.SetSize(1);
                    K2_ASSUME(m_actions.m_pArray != 0);   // SetSize allocated it
#if KOTOR1  // K1 always says "Open"
                    uint64_t nStrRef = STRREF_OPEN;
#else
                    int nUseSearchRow;
                    C2DA* pTable = g_pRules->m_p2DArrays->m_pPlaceables;
                    int nOK = pTable->GetINTEntry(pObj->AsSWCPlaceable()->m_nAppearanceType,
                                                  CExoString("UseSearch"), &nUseSearchRow);
                    int nRow = nOK ? nUseSearchRow : pServerPlaceable->m_bIsBodyBag;
                    uint64_t nStrRef = nRow ? STRREF_SEARCH : STRREF_OPEN;
#endif
                    m_actions.m_pArray[0].m_sName =
                        g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef);
                    m_actions.m_pArray[0].m_pfnAction =
                        reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCPlaceable::UsePlaceable);
                    m_actions.m_pArray[0].m_nObjectId = pObj->m_objectId;
                    m_actions.m_pArray[0].m_nActionId = INTERFACE_ACTION_USE_PLACEABLE;
                    m_actions.m_pArray[0].m_resRef = const_cast<char*>("i_openplace");

                    unsigned long long nPlayerId =
                        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
                    CSWSPlayer* pPlayer =
                        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(nPlayerId);
                    unsigned long long nClientId =
                        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_gameObjectId);
                    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
                    CSWCCreature* pPlayerCreature;
#if !KOTOR1
                    CSWSCreature* pServerCreature;
#endif
                    if (pObject != 0 && pObject->AsSWCCreature() != 0 &&
                        (pPlayerCreature = static_cast<CSWCCreature*>(pObject->AsSWCCreature()),
                         pPlayerChar->m_objectId != nClientId) &&
                        pPlayerCreature->m_bInParty != 0 &&
                        pPlayerCreature->GetServerCreature() != 0 &&
#if KOTOR1  // K1 tests GetDeadTemp()
                        pPlayerCreature->GetServerCreature()->GetDeadTemp() != 0) {
#else
                        (pServerCreature = pPlayerCreature->GetServerCreature(),
                         pServerCreature->GetDeadTemp() != 0)) {
#endif
                        if (pObj->AsSWCPlaceable()->GetServerPlaceable() != 0 &&
                            pObj->AsSWCPlaceable()->GetServerPlaceable()->m_bPartyInteract == 0) {
                            unsigned char& cFlags = m_actions.m_pArray[0].m_nFlags;
                            cFlags &= ~ACTIONFLAG_ENABLED;
                            m_actions.m_pArray[0].m_nFlags =
                                (m_actions.m_pArray[0].m_nFlags & ~ACTIONFLAG_REASON_MASK) |
                                ACTIONFLAG_REASON_PARTY_MEMBER_DEAD;
                        }
                    }
                    nEntry = 1;
                }
                if (pObj->AsSWCPlaceable()->m_bPlot == 0 && nAreaMenuDisabled == 0 &&
                    pObj->AsSWCPlaceable()->m_bLocked != 0) {
                    m_actions.SetSize(nEntry + 1);
                    K2_ASSUME(m_actions.m_pArray != 0);   // SetSize allocated it
                    m_actions.m_pArray[nEntry].m_sName =
                        g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BASH);
                    m_actions.m_pArray[nEntry].m_pfnAction =
                        reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCPlaceable::BashPlaceable);
                    m_actions.m_pArray[nEntry].m_nObjectId = pObj->m_objectId;
                    m_actions.m_pArray[nEntry].m_nActionId = INTERFACE_ACTION_BASH;
                    m_actions.m_pArray[nEntry].m_resRef = const_cast<char*>("i_attack");

                    unsigned long long nPlayerId =
                        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
                    CSWSPlayer* pPlayer =
                        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(nPlayerId);
                    unsigned long long nClientId =
                        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_gameObjectId);
                    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
                    CSWCCreature* pPlayerCreature;
#if !KOTOR1
                    CSWSCreature* pServerCreature;
#endif
                    if (pObject != 0 && pObject->AsSWCCreature() != 0 &&
                        (pPlayerCreature = static_cast<CSWCCreature*>(pObject->AsSWCCreature()),
                         pPlayerChar->m_objectId != nClientId) &&
                        pPlayerCreature->m_bInParty != 0 &&
                        pPlayerCreature->GetServerCreature() != 0 &&
#if KOTOR1  // K1 tests GetDeadTemp()
                        pPlayerCreature->GetServerCreature()->GetDeadTemp() != 0) {
#else
                        (pServerCreature = pPlayerCreature->GetServerCreature(),
                         pServerCreature->GetDeadTemp() != 0)) {
#endif
                        if (pObj->AsSWCPlaceable()->GetServerPlaceable() != 0 &&
                            pObj->AsSWCPlaceable()->GetServerPlaceable()->m_bPartyInteract == 0) {
                            unsigned char& cFlags = m_actions.m_pArray[nEntry].m_nFlags;
                            cFlags &= ~ACTIONFLAG_ENABLED;
                            m_actions.m_pArray[nEntry].m_nFlags =
                                (m_actions.m_pArray[nEntry].m_nFlags & ~ACTIONFLAG_REASON_MASK) |
                                ACTIONFLAG_REASON_PARTY_MEMBER_DEAD;
                        }
                    }
                }
            } else {
                // An inventoryless placeable gets UsePlaceable when the server placeable is not
                // locked or ignored, then a bash entry at the resulting index.
                int nCount = 0;
                CSWCPlaceable* pPlaceable = pObj->AsSWCPlaceable();
                if (pServerPlaceable != 0 && pPlaceable->m_bIgnoreHitcheck != 0 &&
                    pServerPlaceable->m_corpse.m_bIsCorpse == 0) {
                    m_actions.SetSize(1);
                    K2_ASSUME(m_actions.m_pArray != 0);   // SetSize allocated it
                    m_actions.m_pArray[0].m_sName =
                        g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_USE);
                    m_actions.m_pArray[0].m_pfnAction =
                        reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCPlaceable::UsePlaceable);
                    m_actions.m_pArray[0].m_nObjectId = pObj->m_objectId;
                    m_actions.m_pArray[0].m_nActionId = INTERFACE_ACTION_USE_PLACEABLE;
                    m_actions.m_pArray[0].m_resRef = const_cast<char*>("i_useplace");
                    nCount = 1;
                }
                if (pObj->AsSWCPlaceable()->m_bPlot == 0 && nAreaMenuDisabled == 0) {
                    m_actions.SetSize(nCount + 1);
                    K2_ASSUME(m_actions.m_pArray != 0);   // SetSize allocated it
                    m_actions.m_pArray[nCount].m_sName =
                        g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BASH);
                    m_actions.m_pArray[nCount].m_pfnAction =
                        reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCPlaceable::BashPlaceable);
                    m_actions.m_pArray[nCount].m_nObjectId = pObj->m_objectId;
                    m_actions.m_pArray[nCount].m_nActionId = INTERFACE_ACTION_BASH;
                    m_actions.m_pArray[nCount].m_resRef = const_cast<char*>("i_attack");
                }
            }
            break;
        }

        if (pObj->AsSWCDoor() == 0)
            break;
        pObj->AsSWCDoor()->GetServerDoor();
        int nDoorAreaMenuDisabled = pObj->m_pArea->GetServerArea()->m_nPlayerRestrictMode;
        CExoString sName;
        CSWCDoor* pDoor = pObj->AsSWCDoor();
        if (!pDoor->GetIsOpen() && pObj->AsSWCDoor()->m_bStatic == 0) {
            m_actions.SetSize(1);
            K2_ASSUME(m_actions.m_pArray != 0);   // SetSize allocated it
            m_actions.m_pArray[0].m_sName =
                g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_OPEN);
            m_actions.m_pArray[0].m_pfnAction =
                reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCDoor::ToggleDoorState);
            m_actions.m_pArray[0].m_nObjectId = pObj->m_objectId;
            m_actions.m_pArray[0].m_nActionId = INTERFACE_ACTION_OPEN_DOOR;
            m_actions.m_pArray[0].m_resRef = const_cast<char*>("i_opendoor");

            if (pObj->AsSWCDoor()->m_bPlot == 0 && nDoorAreaMenuDisabled == 0 &&
                pObj->AsSWCDoor()->m_bLocked != 0) {
                m_actions.SetSize(2);
                K2_ASSUME(m_actions.m_pArray != 0);   // SetSize allocated it
                m_actions.m_pArray[1].m_sName =
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BASH);
                m_actions.m_pArray[1].m_pfnAction =
                    reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCDoor::MenuActionBash);
                m_actions.m_pArray[1].m_nObjectId = pObj->m_objectId;
                m_actions.m_pArray[1].m_nActionId = INTERFACE_ACTION_BASH;
                m_actions.m_pArray[1].m_resRef = const_cast<char*>("i_attack");
            }
        }
        break;
    }
    }
}

void CClientExoAppInternal::ResetDrivingTimers(float speed)
{
    long long snapshot = g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer->GetSnapshotTime();
    int nScaled = static_cast<int>(static_cast<double>(speed) * 500000.0);
    m_drivingTimerReset = snapshot - static_cast<long long>(500000 - nScaled);
}

// Walks the player to a point after a line-of-sight test.
bool CClientExoAppInternal::WalkPlayerToPoint(float x, float y, float z, int a, unsigned long long id, int b)
{
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (!pPlayer)
        return false;

    Vector vTarget(x, y, z);
    m_bShowWalkToRingVFX = b;

    if (m_bShowWalkToRingVFX)
    {
        if (m_pWalkToRingVFX == NULL)
        {
            CSWCVisualEffect* pVfx = new CSWCVisualEffect(1);
            m_pWalkToRingVFX = pVfx;
            CResRef res("gui_walkto");
            if (pVfx->LoadModel(res, MODEL_PART_BODY, 8) == 0)
                goto lblWalk;
            Gob* pGob = pVfx->GetModel(MODEL_PART_BODY, 1);
            pGob->EnableForceAnimations();
            pVfx = static_cast<CSWCVisualEffect*>(m_pWalkToRingVFX);
            pVfx->m_bImpactOnly = 1;
            pVfx->m_bDeleteWhenFinished = 0;
            pVfx->m_bDetachWhenFinished = 1;
            pVfx->m_bNoFinishDelay = 1;
            pVfx->SetAnimateWhilePaused(1);
        }
        else
        {
            static_cast<CSWCVisualEffect*>(m_pWalkToRingVFX)->Reset();
        }

        static_cast<CSWCVisualEffect*>(m_pWalkToRingVFX)->SetPosition(vTarget);
        static_cast<CSWCVisualEffect*>(m_pWalkToRingVFX)->SetAnimation(1);
        if (m_moveTypeMode == 1)
        {
            static_cast<CSWCVisualEffect*>(m_pWalkToRingVFX)->AttachToScene(NULL);
        }
        else if (m_pModule && m_pModule->m_pArea && m_pModule->m_pArea->m_pScene)
        {
            static_cast<CSWCVisualEffect*>(m_pWalkToRingVFX)->AttachToScene(m_pModule->m_pArea->m_pScene);
        }
    }

lblWalk:
    Vector vDelta = m_vLastMovePoint - vTarget;
    if (static_cast<int>(g_pExoBase->m_pTimers->GetLowResolutionTimer()) - m_nLastMoveTimer <= 1000)
    {
        if (vDelta.x * vDelta.x + vDelta.y * vDelta.y < 0.01f)
            return true;
    }

    if (m_moveTypeMode != 1)
    {
        CSWCCreatureAppearance* pAppearance = pPlayer->m_pCreatureAppearance;
        Vector from(x - pAppearance->m_fPersonalSpace, y - pAppearance->m_fPersonalSpace, z);
        Vector to = Vector(x, y, z) + Vector(0.01f, 0.01f, 0.0f);
        float fHitFraction = pAppearance->m_fPersonalSpace;
        // The test uses the raw int; the bool is only what gets returned.
        int nLineOfSight = pPlayer->m_pArea->TestDirectLine(
            pPlayer->m_objectId, from, to, fHitFraction, pAppearance->GetHeight(), 1, NULL);
        bool bLineOfSight = (nLineOfSight != 0);
        if (nLineOfSight == 0)
            return bLineOfSight;
    }

    CSWCModule* pMod = GetModule();
    unsigned long long uObjId = pMod->m_pArea->m_objectId;
    unsigned char uTag = ++pPlayer->m_commandTag;
    int nRingDist = 1;
    if (pPlayer->m_nAIStateAction == 2)
    {
        CGameObjectArray* pArr = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
        CGameObject* pObj;
        if (pArr->GetGameObject(pPlayer->m_oidCombatTarget, &pObj) == CGameObjectArray::SUCCESS
            && pObj->AsSWCCreature())
        {
            Vector vDiff = vTarget - pPlayer->m_position;
            nRingDist = !(vDiff.x * vDiff.x + vDiff.y * vDiff.y + vDiff.z * vDiff.z < 2.25f);
        }
    }

    int nWalkBusy;
    unsigned short nFlags404 = pPlayer->m_nAIStateFlags;
    if (g_pAppManager->m_pClientExoApp->GetCaptureEventPlayBack() == 1
        || g_pAppManager->m_pClientExoApp->GetCaptureEvents() == 1)
        nWalkBusy = 0;
    else
        nWalkBusy = nRingDist & (nFlags404 >> 1);

#if KOTOR1
    if ((pPlayer->m_nActivityFlags & 1) == 0)  // K1: no stealth-run exemption
#else
    if ((pPlayer->m_nActivityFlags & 1) == 0 || pPlayer->m_pStats->HasFeat(FEAT_STEALTH_RUN))
#endif
        a = 1;

    if (nWalkBusy != 0)
        nWalkBusy = (pPlayer->ClientSideWalkCommand(vTarget, static_cast<unsigned char>(a), static_cast<unsigned char>(m_moveTypeMode)) == 1);

    if ((m_pClientOptions->m_nCameraMode | 2) == 2)
    {
        if (GetModule())
        {
            CSWCModule* pCamModule = GetModule();
            pCamModule->m_nCameraSide = 0;
        }
    }
    if (m_pModule)
    {
        m_pModule->m_bCameraTurned = 1;
        m_pModule->m_bCameraZoomed = 1;
    }
    m_pSWCMessage->SendPlayerToServerInput_WalkToWayPoint(uObjId, vTarget.x, vTarget.y, vTarget.z,
                                                          uTag, a, nWalkBusy, static_cast<unsigned char>(m_moveTypeMode), id);
    m_nLastMoveTimer = static_cast<int>(g_pExoBase->m_pTimers->GetLowResolutionTimer());
    if (nWalkBusy == 0 && m_nLastMoveBusy == 1)
    {
        Vector vDelta2 = m_vLastMovePoint - vTarget;
        if (vDelta2.x * vDelta2.x + vDelta2.y * vDelta2.y < 25.0f)
        {
            nWalkBusy = 1;
        }
        else
        {
            unsigned short nAnim = pPlayer->GetCurrentAnimation();
            nWalkBusy = (pPlayer->AnimationStationary(nAnim) == 0);
        }
    }
    m_nLastMoveBusy = nWalkBusy;
    m_vLastMovePoint = vTarget;
    pPlayer->m_appliedCommandTag = uTag;
    return true;
}

// Shows the mouse cursor once the hide mask reaches zero.
void CClientExoAppInternal::ShowMouse(int bClear)
{
    m_mouseHideMask &= ~bClear;
    if (m_mouseHideMask != 0)
        return;
    if (bClear & (MOUSEHIDE_REINIT_AURORA | MOUSEHIDE_MOVIE | MOUSEHIDE_LOAD_SCREEN))
        m_fMouseShowTimer = 0.5f;
    if (bClear & (MOUSEHIDE_DIALOG_CUTSCENE | MOUSEHIDE_MOUSE_MODE)) {
        m_fMouseShowTimer = -1.0f;
    } else if (!(m_fMouseShowTimer <= 0.0f)) {   // an unsigned "greater than", not a signed one
        return;
    }
    g_pExoInput->ShowMouse();
    if (m_pSWGuiManager != 0 && m_pClientOptions->m_bHardwareMouseCursor == 0)
        m_pSWGuiManager->ShowSoftwareMouse();
    g_pExoInput->RefreshMouseCursor(g_bLeftMouseDown);
}

void CClientExoAppInternal::HideMouse(int bHide)
{
    m_mouseHideMask |= bHide;
    if (m_mouseHideMask == 0)
        return;
    if (g_pExoInput != 0)
        g_pExoInput->HideMouse();
    if (m_pSWGuiManager != 0)
        m_pSWGuiManager->HideSoftwareMouse();
}

// Switches the input-control policy object in m_pPlayerControl; -1 cycles the type.
int CClientExoAppInternal::ChangeControlType(int nType)
{
    if (nType != m_controlType)
    {
        if (nType == -1)
            nType = (m_controlType > 0) ? 0 : m_controlType + 1;
        m_controlType = nType;
        if (m_pPlayerControl) {
            // Destroyed through the base, whose destructor is not virtual (both subclasses add
            // only trivially destructible members).
            m_pPlayerControl->CSWPlayerControl::~CSWPlayerControl();
            operator delete(m_pPlayerControl);
        }
        if (m_controlType == 0) {
            m_pPlayerControl = new CSWPlayerControlDriving(m_playerCreatureId, GetModule()->m_pCamera);
        } else if (m_controlType == 1) {
            m_pPlayerControl = new CSWPlayerControlCamRelative(m_playerCreatureId, GetModule()->m_pCamera);
        }
        m_pPlayerControl->SetEnabled(1);
    }
    return m_controlType;
}

void CClientExoAppInternal::DisableInput()
{
    m_bInputDisabled = 1;
}

void CClientExoAppInternal::EnableInput()
{
    m_bInputDisabled = 0;
}

int CClientExoAppInternal::DismissInGameGUI()
{
    int nShown = static_cast<int>(m_pInGameGui->HideSWInGameGui(0));
    if (nShown == 0)
        return 0;
    SetInputClass(INPUTCLASS_GAME, 1);
    return 1;
}

// Handles a click in the world: the radial action menu, or the queued default action.
void CClientExoAppInternal::HandleMouseClickInWorld()
{
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0)
        return;
    if (m_pInGameGui->m_bInGameMenuShown != 0)
        return;
    if (m_pClientOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
        return;

    // The click lands on an open radial action only when the down coordinates still chain
    // (clicked target == last target == mouse-over object); every other path refreshes the menus
    // instead.
    if (m_lastClickedOnTarget == OBJECT_INVALID || m_lastClickedOnTarget != m_lastTarget ||
        m_lastTarget != m_objectMouseOver) {
        if (m_objectMouseOver != OBJECT_INVALID)
            SetLastTarget(m_objectMouseOver, 0);
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface != 0)
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->PopulateMenus(1);
        return;
    }
    {
        m_lastClickedOnTarget = OBJECT_INVALID;
        if (m_actions.m_count < 1)
            return;
        CSWGuiInterfaceAction* pAction = m_actions.m_pArray;
        if (pAction == 0)
            return;
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
        CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->GetGameObject(pAction->m_nObjectId);
        if (pGameObject == 0)
            return;
        CSWCObject* pClickObject = pGameObject->AsSWCObject();
        if (pClickObject == 0)
            return;
        m_pSWGuiManager->PlayGuiSound(GUISOUND_ACTION_MENU_USE);
        CSWCCreature* pClick = static_cast<CSWCCreature*>(pClickObject);
        (pClick->*pAction->m_pfnAction)(pAction->m_nActionId, pPlayer);
        return;
    }
}

void CClientExoAppInternal::SetMouseCursor(int nCursorId, int yPos, int bButtonDown)
{
    if (g_pExoInput != 0)
        nCursorId = g_pExoInput->SetMouseCursor(nCursorId, yPos, bButtonDown);
    if (m_pSWGuiManager != 0)
        m_pSWGuiManager->SetSoftwareMouseCursor(nCursorId, yPos, bButtonDown);
}

// Comparison predicate for the visible-object scan: matches the wanted object id.
int NearestObjectCompare(void* pObjectId, CNearestStruct nearest)
{
    return nearest.m_objectId == reinterpret_cast<unsigned long long>(pObjectId);
}

int CClientExoAppInternal::ActionIconToMouseCursor(int nAction, int b)
{
    K2_UNUSED(b);
    int r = MOUSECURSOR_ATTACK;
    switch (nAction & INTERFACE_ACTION_ID_MASK) {
    case INTERFACE_ACTION_ATTACK: break;
    case INTERFACE_ACTION_USE_OBJECT: case INTERFACE_ACTION_USE_PLACEABLE: r = MOUSECURSOR_USE; break;
    case INTERFACE_ACTION_DIALOG: r = MOUSECURSOR_TALK; break;
    case INTERFACE_ACTION_OPEN_DOOR: r = MOUSECURSOR_DOOR; break;
    case INTERFACE_ACTION_DISABLE_MINE: r = MOUSECURSOR_DISABLE_MINE; break;
    case INTERFACE_ACTION_BASH: r = MOUSECURSOR_BASH; break;
    case INTERFACE_ACTION_RECOVER_MINE: r = MOUSECURSOR_RECOVER_MINE; break;
    default: r = MOUSECURSOR_INVALID; break;
    }
    return r;
}

CResRef CClientExoAppInternal::ActionIconToCursorResref(int nAction, int b)
{
    K2_UNUSED(b);
    int idx = MOUSECURSOR_ATTACK;
    switch (nAction & INTERFACE_ACTION_ID_MASK) {
    case INTERFACE_ACTION_ATTACK: break;
    case INTERFACE_ACTION_USE_OBJECT: case INTERFACE_ACTION_USE_PLACEABLE: idx = MOUSECURSOR_USE; break;
    case INTERFACE_ACTION_DIALOG: idx = MOUSECURSOR_TALK; break;
    case INTERFACE_ACTION_OPEN_DOOR: idx = MOUSECURSOR_DOOR; break;
    case INTERFACE_ACTION_DISABLE_MINE: idx = MOUSECURSOR_DISABLE_MINE; break;
    case INTERFACE_ACTION_BASH: idx = MOUSECURSOR_BASH; break;
    case INTERFACE_ACTION_RECOVER_MINE: idx = MOUSECURSOR_RECOVER_MINE; break;
    default: idx = MOUSECURSOR_INVALID; break;
    }
    return CResRef(g_asMouseCursorNames[idx]);
}
