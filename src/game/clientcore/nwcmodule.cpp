#include "k2stdint.h"
#define K1_AURORA_LIST  // K1: the aurora list growth starts at 8 here
#include "nwcmodule.h"
#include "worldtimer.h"
#include "nwcarea.h"
#include "nwcambientsound.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "nwscriptconst.h"
#include "serverexoappinternal.h"
#include "AURORA/aurbehavior.h"
#include "aurscene.h"
#include "guiingame.h"
#include "exoresman.h"
#include "nwccreature.h"
#include "nwchandlemessage.h"
#include "nwscreature.h"
#include "nwcobject.h"
#include "aurcamera.h"
#include "AURORA/aurmodel.h"
#include "Quaternion.h"
#include <stdio.h>
#include "serveranimation.h"

extern float g_fMouseSensitivity;
extern int g_bEnableFogOfWar;
extern int g_bShowFogOfWar;
extern float DriveWalkThreshold;
extern int g_nHackAlignment;

extern float g_fMaxLean;
extern int enablemodelrequest;

const int CCameraSettings::CAMERA_ON_STICK = 0;
const int CCameraSettings::CAMERA_FREE_LOOK = 1;

// Placement new, defined here so this TU always has a local definition. Kept separate from <new>,
// which pulls in <math.h> and conflicts with the throw()-specified math declarations below.
#if !defined(_MSC_VER)
inline void* operator new(size_t, void* p) throw() { return p; }
#endif


#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/libcmath.h"
#include "port/crtapi.h"

// Local min/max for the clip globals below, which are loaded at run time.
inline float min(float a, float b) { return a < b ? a : b; }
inline float max(float a, float b) { return a > b ? a : b; }
float Yaw(const Vector& orientation);
int lockcamera(char* pszTarget);       // camera-lock query; NULL means the current target
CAurCamera* NewCAurCamera(char* pszName);  // creates the renderer camera

float g_fMinDist  = 5.0f;
float g_fMaxDist  = 20.0f;
float g_fMinPitch = 1.0f;
float g_fMaxPitch = 50.0f;


float fShakePeriod = 30.0f;
float fShakeMag    = 20.0f;

CSWCModule::CSWCModule() : CGameObject(CGameObject::OBJECT_TYPE_MODULE, OBJECT_INVALID) {
    m_nCameraMode = -1;
    m_pCamera = 0;
    m_pReserved1 = 0;
    m_pArea = 0;
    m_vSavedPlayerPos = Vector{0.0f, 0.0f, 0.0f};
    m_vCameraPos = Vector{0.0f, 0.0f, 0.0f};
    m_vCameraOffset = Vector{0.0f, 0.0f, 1.0f};
    m_oidCameraFollow = CGameObjectArray::INVALIDOBJID;
    m_fCameraDist = 0.0f;
    m_fCameraYaw = 0.0f;
    m_fCameraPitch = 0.0f;
    m_bCameraMoved = 0;
    m_bCameraAutoTurning = 0;
    m_bAutoTurnSuppressed = 0;
    m_bFindClearView = 0;
    m_nCameraSide = 0;
    m_fPitchVelCache = 0.0f;
    m_fYawVel = 0.0f;
    m_fDistDecay = 0.0f;
    m_sModuleDisplayName = "";
    m_nMinutesPerHour = 5;
    m_nCalendarYear = CWorldTimer::DEFAULT_YEAR;
    m_nCalendarMonth = 1;
    m_nCalendarDay = 1;
    m_nCalendarHour = 0;
    m_nTimeOfDayState = TIME_OF_DAY_NONE;
    m_fTimeOfDay = 0.0f;
    m_fHourLengthSec = 0.0f;
    memset(m_aReserved2, 0, sizeof(m_aReserved2));
    m_nLastCameraMode = LAST_CAMERA_MODE_NONE;
    m_bCameraModeChanged = 0;
    m_nShakeTimeLeftMs = 0;
    m_nShakeIntervalMs = 0;
    m_nShakeDelayMs = 0;
    m_bShakeReset = 1;
    m_nBumpTimeLeftMs = 0;
    m_nBumpParam = 0;
    m_bBumpReset = 1;
    m_bPlayerStationary = 1;
    m_bCameraTurned = 0;
    m_bCameraZoomed = 0;
    m_nReserved1 = 0;
    m_cSavedCameraSettings.m_bValid = 0;
    m_pAnimatedCameraModel = 0;
}

CSWCModule::~CSWCModule() {
    m_sModuleDisplayName = "";
    if (m_pArea != 0) {
        m_pArea->UnloadArea();
        delete m_pArea;
        m_pArea = 0;
    }
    if (m_pCamera != 0) {
        delete m_pCamera;
        m_pCamera = 0;
    }
    UnloadAnimatedCamera();
}

CSWCModule* CSWCModule::AsSWCModule() {
    return this;
}

void CSWCModule::UnloadModule() {
}

bool CSWCModule::CanUpdate() {
    if (m_pArea == 0) {
        return false;
    }
    return m_pCamera != 0;
}

CSWMiniGame* CSWCModule::GetMiniGame() {
    if (m_pArea == 0) {
        return 0;
    }
    return m_pArea->m_pMiniGame;
}

void CSWCModule::UnloadAnimatedCamera() {
    if (m_pAnimatedCameraModel == 0) {
        return;
    }
    delete m_pAnimatedCameraModel;
    m_pAnimatedCameraModel = 0;
}

void CSWCModule::BumpCamera(unsigned int nParam) {
    if (m_nBumpTimeLeftMs != 0) {
        return;
    }
    m_nBumpParam = nParam;
    m_nBumpTimeLeftMs = 50;
    m_bBumpReset = 1;
}

void CSWCModule::ShakeCamera(unsigned int nDelayMs, unsigned int nDurationMs) {
    if (nDurationMs != 0 && m_nShakeTimeLeftMs <= static_cast<int>(nDurationMs)) {
        m_nShakeTimeLeftMs = nDurationMs;
        m_nShakePeriodMs = nDurationMs;
        m_nShakeDelayMs = nDelayMs;
        m_nShakeIntervalMs = 70;
        m_bShakeReset = 1;
    }
}

int CSWCModule::IsCameraAnimated() {
    if (m_pAnimatedCameraModel == 0) {
        return false;
    }
    return m_pAnimatedCameraModel->GetAnimation(m_sAnimatedCameraAnim.CStr(), 0, 0) != 0;
}

void CSWCModule::SetTimeOfDayState(unsigned char nState, unsigned int nMs) {
    if (m_pArea != 0) {
        switch (nState) {
        case TIME_OF_DAY_DAY:
            m_pArea->SetAsDay();
            break;
        case TIME_OF_DAY_NIGHT:
            m_pArea->SetAsNight();
            break;
        case TIME_OF_DAY_DAWN:
            m_pArea->SetAsNight();
            m_pArea->SetAsDawn();
            break;
        case TIME_OF_DAY_DUSK:
            m_pArea->SetAsDay();
            m_pArea->SetAsDusk();
            break;
        }
    }
    if (m_nTimeOfDayState != nState) {
        m_nTimeOfDayState = nState;
        if (m_pArea != 0) {
            m_pArea->UpdateSoundObjectsTimeOfDay();
            m_pArea->m_pAmbientSound->UpdateTimeOfDay();
        }
    }
    m_fTimeOfDay = static_cast<float>(nMs) / 1000.0f;
    m_fHourLengthSec = static_cast<float>(m_nMinutesPerHour) * 60.0f;
}

float CSWCModule::UpdateCameraCollision(float fParam1, float fParam2) {
    if (m_bFindClearView != 0) {
        return fParam2;
    }
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_nCameraMode == CAMERA_MODE_STIFF_CHASE_CAMERA) {
        return UpdatePushCameraCollision(fParam1, fParam2);
    }
    return UpdateNormalCameraCollision(fParam1, fParam2);
}


__attribute__((no_stack_protector))
void CSWCModule::Update(float fDeltaTime) {
    if (m_pArea == 0 || m_pCamera == 0) {
        return;
    }
    if (m_bFindClearView != 0) {
        float fYaw, fPitch;
        if (FindClearView(fDeltaTime, &fYaw, &fPitch)) {
            CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
            if (pBehavior != 0) {
                CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
                if (pFollow != 0) {
                    pFollow->m_fYaw = fYaw;
                    pFollow->m_fPitch = fPitch;
                }
            }
        }
        m_bFindClearView = 0;
    }
    if (static_cast<unsigned char>(g_pAppManager->m_pClientExoApp->m_pInternal->GetActivePauseState()) == 0) {
        m_fTimeOfDay += fDeltaTime;
    }
    m_pArea->Update(fDeltaTime);
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_bSuppressCameraRender == 0) {
        m_pCamera->RenderScene();
    }
}

void CSWCModule::ZoomCamera(float fDelta) {
    if (m_pCamera != 0) {
        CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
        if (pBehavior != 0) {
            CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
            if (pFollow != 0) {
                float fAccum = pFollow->m_fZoomDeltaAccum + fDelta;
                pFollow->m_fZoomDeltaAccum = fAccum;
                m_fDistDecay = fAccum;
            }
        }
    }
    if (fDelta != 0.0f) {
        m_bCameraZoomed = 1;
        m_bCameraMoved |= 1;
    }
}

// fNearAdjust is unused; fFarAdjust widens both the clip range and the fog range.
void CSWCModule::AdjustCameraClipRange(float fNearAdjust, float fFarAdjust) {
    K2_UNUSED(fNearAdjust);
    if (m_pArea != 0 && m_pArea->m_pScene != 0) {
        float fNear, fFar;
        m_pCamera->GetClipDist(fNear, fFar);
        m_pCamera->SetClipDist(fNear, fFar + fFarAdjust);
        m_pArea->m_pScene->GetFogRange(fNear, fFar);
        m_pArea->m_pScene->SetFogRange(fNear, fFar + fFarAdjust);
        m_bCameraMoved = 1;
    }
}

void CSWCModule::ScrollCamera(Vector vDelta) {
    float fLenSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
    if (!(fLenSq <= 1e-05f)) {
        Vector vCur = m_pCamera->GetPosition();
        Vector vNew = { vDelta.x + vCur.x, vDelta.y + vCur.y, vDelta.z + vCur.z };
        m_pCamera->SetPosition(vNew);
        m_bCameraMoved = 1;
    }
}

void CSWCModule::TiltCamera(float fParam) {
    if (m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        CAurBehavior* pBehavior = m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK);
        if (pBehavior != 0) {
            static_cast<CSWCameraOnAStick*>(pBehavior)->m_fTiltAccel = fParam;
        }
    } else if (m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
        CAurBehavior* pBehavior = m_pCamera->GetBehavior(BEHAVIOR_CAMERA_FREE_LOOK);
        if (pBehavior != 0) {
            static_cast<CSWCameraFreeLook*>(pBehavior)->m_fTiltSpeed = fParam;
        }
    } else if (m_nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE) {
        CAurBehavior* pBehavior = m_pCamera->GetBehavior(BEHAVIOR_CAMERA_NAVIGATE);
        if (pBehavior != 0) {
            static_cast<CSWCameraNavigate*>(pBehavior)->m_fTilt = fParam * -60.0f;
        }
    } else {
        if (m_pCamera != 0) {
            CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
            if (pBehavior != 0) {
                CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
                if (pFollow != 0) {
                    float fAccum = pFollow->m_fPitchVel + fParam;
                    pFollow->m_fPitchVel = fAccum;
                    m_fPitchVelCache = fAccum;
                }
            }
        }
        m_bCameraMoved |= (fParam != 0.0f);
    }
}

void CSWCModule::AcclTiltCamera(float fParam) {
    if (m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
#if KOTOR1
        m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK);  // K1: the on-a-stick camera takes no tilt input here
#else
        CAurBehavior* pBehavior = m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK);
        if (pBehavior != 0) {
            static_cast<CSWCameraOnAStick*>(pBehavior)->m_fTiltAccel = -fParam;
        }
#endif
    } else if (m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
        CAurBehavior* pBehavior = m_pCamera->GetBehavior(BEHAVIOR_CAMERA_FREE_LOOK);
        if (pBehavior != 0) {
            static_cast<CSWCameraFreeLook*>(pBehavior)->m_fTiltAccel = -fParam;
        }
    } else if (m_nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE) {
        m_pCamera->GetBehavior(BEHAVIOR_CAMERA_NAVIGATE);
    }
}

bool CSWCModule::InterpolateDist(float fTargetDist, float fRate, float fDeltaTime) {
    if (m_pCamera == 0) return true;
    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior == 0) return true;
    CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
    if (pFollow == 0) return true;
    float fCur = m_pCamera->GetDist();
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_nCameraMode != CAMERA_MODE_STIFF_CHASE_CAMERA) {
        float fVel;
        if (fCur < fTargetDist) {
            fVel = -fRate;
        } else {
            fVel = 0.0f;
            if (fCur > fTargetDist) {
                fVel = fRate;
            }
        }
        float fCur2 = m_pCamera->GetDist();
        float fNew = exp2f(fVel * fDeltaTime * -0.5f) * fCur2;
        // Both crossing tests are evaluated and combined bitwise, so the pair selects rather than
        // branches.
        bool b1 = (fCur > fTargetDist) & (fNew < fTargetDist);
        bool b2 = (fCur < fTargetDist) & (fNew > fTargetDist);
        if (b1 | b2) {
            pFollow->m_fDist = fTargetDist;
        } else if (fCur != fTargetDist) {
            pFollow->m_fZoomDeltaAccum = fVel;
            return false;
        }
    }
    return true;
}

bool CSWCModule::InterpolateYaw(float fTargetYaw, float fRate, float fDeltaTime) {
    if (m_pCamera == 0) return true;
    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior == 0) return true;
    CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
    if (pFollow == 0) return true;

    while (fTargetYaw < -180.0f) fTargetYaw += 360.0f;
    while (fTargetYaw > 180.0f) fTargetYaw -= 360.0f;

    float fCur = m_pCamera->GetYaw();
    while (fCur < -180.0f) fCur += 360.0f;
    while (fCur > 180.0f) fCur -= 360.0f;

    float fDelta = fTargetYaw - fCur;
    while (fDelta < -180.0f) fDelta += 360.0f;
    while (fDelta > 180.0f) fDelta -= 360.0f;

    float fVel;
    if (!(fDelta < 0.0f)) {
        fVel = 0.0f;
        if (fDelta > 0.0f) {
            fVel = fRate;
        }
    } else {
        fVel = -fRate;
    }

    float fNew = fCur + fVel * fDeltaTime;
    if ((fCur > fTargetYaw && fNew < fTargetYaw) ||
        (fCur < fTargetYaw && fNew > fTargetYaw)) {
        pFollow->m_fYaw = fTargetYaw;
    } else if (fCur != fTargetYaw) {
        pFollow->m_fYawVel = fVel;
        return false;
    }
    return true;
}

void CSWCModule::UpdateCameraModeBG() {
    if (m_bCameraModeChanged == 0) return;
    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior == 0) return;
    CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
    if (pFollow == 0) return;

    pFollow->m_fDistClipMin = 5.0f;
    pFollow->m_fDistClipMax = 20.0f;
    pFollow->m_fPitchClipMin = 1.0f;
    pFollow->m_fPitchClipMax = 50.0f;
    g_fMinDist = 5.0f;
    g_fMaxDist = 20.0f;
    g_fMinPitch = 1.0f;
    g_fMaxPitch = 50.0f;
    m_vCameraOffset = Vector{0.0f, 0.0f, 1.0f};
    pFollow->m_vOffset = m_vCameraOffset;

    float fClipMin, fClipMax;
    m_pCamera->GetClipDist(fClipMin, fClipMax);
    m_pCamera->SetClipDist(fClipMin, 45.0f);
    pFollow->m_fDist = 20.0f;
    pFollow->m_fPitch = 50.0f;
    m_bCameraModeChanged = 0;
}

// The same setup as UpdateCameraModeBG for the other camera mode; the two modes carry duplicated
// bodies rather than a shared helper.
void CSWCModule::UpdateCameraModeNormal() {
    if (m_bCameraModeChanged == 0) return;
    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior == 0) return;
    CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
    if (pFollow == 0) return;

    pFollow->m_fDistClipMin = 5.0f;
    pFollow->m_fDistClipMax = 20.0f;
    pFollow->m_fPitchClipMin = 1.0f;
    pFollow->m_fPitchClipMax = 50.0f;
    g_fMinDist = 5.0f;
    g_fMaxDist = 20.0f;
    g_fMinPitch = 1.0f;
    g_fMaxPitch = 50.0f;
    m_vCameraOffset = Vector{0.0f, 0.0f, 1.0f};
    pFollow->m_vOffset = m_vCameraOffset;

    float fClipMin, fClipMax;
    m_pCamera->GetClipDist(fClipMin, fClipMax);
    m_pCamera->SetClipDist(fClipMin, 45.0f);
    pFollow->m_fDist = 20.0f;
    pFollow->m_fPitch = 50.0f;
    m_bCameraModeChanged = 0;
}

bool CSWCModule::ReAttachHAKFile() {
    if (m_cModuleHak == const_cast<char*>("")) return true;
    CExoString sHak;
    sHak = "HAK:";
    sHak = sHak + CExoString(m_cModuleHak.GetResRef(), 16);
    if (!static_cast<int>(g_pExoResMan->RemoveEncapsulatedResourceFile(sHak))) return false;
    if (!static_cast<int>(g_pExoResMan->AddEncapsulatedResourceFile(sHak, 2))) {
        g_pExoResMan->RemoveEncapsulatedResourceFile(sHak);
        return false;
    }
    return true;
}

void CSWCModule::TurnCamera(float fParam1, float fParam2) {
    if (g_pAppManager->m_pClientExoApp->GetCombatMode() != 0) {
        CSWBehaviorCameraCombat* pCombat = static_cast<CSWBehaviorCameraCombat*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_COMBAT));
        if (pCombat != 0) {
            pCombat->m_fTurn = 0.0f;
            pCombat->SetTurn(-fParam1);
        }
        return;
    }
    if (m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick != 0) {
            pOnAStick->m_fTurn = fParam1;
            pOnAStick->m_fTurnAccel = 0.0f;
        }
    } else if (m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
        CSWCameraFreeLook* pFreeLook = static_cast<CSWCameraFreeLook*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_FREE_LOOK));
        if (pFreeLook != 0) {
            float fTurnSpeed = pFreeLook->m_fTurnSpeed;
            CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            if (pCreature != 0) {
                pCreature->m_fFreeLookTurnVel = 0.0f;
                float fHalf = fParam1 * 0.5f;
                if (fHalf > 30.0f) fHalf = 30.0f;
                if (fHalf <= -30.0f) fHalf = -30.0f;
                float fAngle = fHalf * (fTurnSpeed * fParam2);

                Quaternion q = pointto(pCreature->m_orientation);
                float fYaw = Yaw(q) - fAngle;
                float fPitch = Pitch(q);
                float fRoll = Roll(q);
                q = YawPitchRoll(fYaw, fPitch, fRoll);

                pCreature->SetOrientation(q.zdir());

                if (pCreature->GetServerCreature() != 0) {
                    CSWSCreature* pServer = pCreature->GetServerCreature();
                    pServer->SetOrientation(q.zdir());
                }
            }
        }
    } else if (m_nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE) {
        CSWCameraNavigate* pNav = static_cast<CSWCameraNavigate*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_NAVIGATE));
        if (pNav != 0) {
            pNav->m_fTurn = fParam1 * -60.0f;
        }
    } else {
        if (m_pCamera != 0) {
            CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
            if (pBehavior != 0) {
                CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
                if (pFollow != 0) {
                    float fAccum = pFollow->m_fYawVel + fParam1;
                    pFollow->m_fYawVel = fAccum;
                    m_fYawVel = fAccum;
                }
            }
        }
        if (fParam1 != 0.0f) {
            m_bCameraTurned = 1;
            m_bCameraMoved |= 1;
        }
    }
}

void CSWCModule::AcclTurnCamera(float fParam1, float fParam2) {
    K2_UNUSED(fParam2);
    if (g_pAppManager->m_pClientExoApp->GetCombatMode() != 0) {
        CSWBehaviorCameraCombat* pCombat = static_cast<CSWBehaviorCameraCombat*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_COMBAT));
        if (pCombat != 0) {
            pCombat->SetTurn(0.0f);
            pCombat->m_fTurn = -fParam1;
        }
        return;
    }
    if (m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick != 0) {
            pOnAStick->m_fTurn = 0;
            pOnAStick->m_fTurnAccel = fParam1;
        }
    } else if (m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
        CAurBehavior* pBehavior = m_pCamera->GetBehavior(BEHAVIOR_CAMERA_FREE_LOOK);
        if (pBehavior != 0) {
            CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            if (pCreature != 0) {
                pCreature->m_fFreeLookTurnVel = -fParam1;
            }
        }
    } else if (m_nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE) {
        CSWCameraNavigate* pNav = static_cast<CSWCameraNavigate*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_NAVIGATE));
        if (pNav != 0) {
            pNav->m_fTurn = fParam1 * -60.0f;
        }
    } else {
        if (m_pCamera != 0) {
            CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
            if (pBehavior != 0) {
                CAurBehaviorCameraFollow* pFollow = pBehavior->AsBehaviorCameraFollow();
                if (pFollow != 0) {
                    float fAccum = pFollow->m_fYawVel + fParam1;
                    pFollow->m_fYawVel = fAccum;
                    m_fYawVel = fAccum;
                }
            }
        }
        if (fParam1 != 0.0f) {
            m_bCameraTurned = 1;
            m_bCameraMoved |= 1;
        }
    }
}

// Hands camera control to a new follow target: mode 3 creates or reuses a CSWCameraOnAStick, every
// other mode goes through the generic game-object path and configures the default follow behavior.
void CSWCModule::SetCameraFollow(unsigned long long oid, int bFindClearView) {
    if (g_pAppManager->m_pClientExoApp->GetCombatMode() != 0) {
        return;
    }

    if (m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        CSWCCreature* pCreature = static_cast<CSWCCreature*>(
            g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(oid));
        if (pCreature == 0) return;

        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(
            m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick == 0) {
            pOnAStick = new CSWCameraOnAStick(
                m_pCamera->AsCAurObject(),
                static_cast<CAurObject*>(pCreature->GetCameraFollowPart()));
            m_pCamera->SetBehavior(pOnAStick);
        }

        Vector vPos = {0.0f, 0.0f, 0.0f};
        if (pOnAStick->m_pCameraObject != 0) {
            vPos = pOnAStick->m_pCameraObject->GetPosition();
        }

        pOnAStick->m_nFollowingBody = pCreature->GetFollowingBody();
        pOnAStick->m_fCameraOffset =
            pCreature->GetHeadHeightOffset() + pCreature->GetCameraOffset();
        pOnAStick->ChangeTarget(
            static_cast<CAurObject*>(pCreature->GetCameraFollowPart()), bFindClearView);
        m_oidCameraFollow = oid;

        if (bFindClearView == 0) {
            if (pOnAStick->m_pCameraObject != 0) {
                pOnAStick->m_pCameraObject->SetPosition(vPos);
            }
            pOnAStick->m_vLastCameraPosition.x = vPos.x;
            pOnAStick->m_vLastCameraPosition.y = vPos.y;
            pOnAStick->m_vLastCameraPosition.z = vPos.z;
        }
    } else {
        CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(oid);
        if (pObj == 0) return;
        CSWCObject* pSWCObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
        if (pSWCObj == 0) return;

        Gob* pModel = pSWCObj->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
        if (pModel == 0) return;

        unsigned long long oidOld = m_oidCameraFollow;
        m_oidCameraFollow = oid;
        m_bFindClearView = bFindClearView;

        // Without a camera there is nothing to refresh either.
        if (m_pCamera == 0)
            return;
        m_pCamera->SetFollow(static_cast<CAurObject*>(pModel));
        m_vCameraPos = pModel->GetPosition();

        CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
        if (pBehavior != 0) {
            CAurBehaviorCameraFollow* pFollow =
                pBehavior->AsBehaviorCameraFollow();
            if (pFollow != 0) {
                float fMaxPitch = g_fMaxPitch;
                pFollow->m_fFOV = 750.0f;
                pFollow->m_fPitch = fMaxPitch;

                if (lockcamera(0) != 0) {
                    pFollow->m_fPitchClipGate = 1.0f;
                    pFollow->m_fDistClipGate = 1.0f;
                }

                pFollow->m_fPitchClipMin = g_fMinPitch;
                pFollow->m_fPitchClipMax = g_fMaxPitch;
                pFollow->m_fDistClipMin = g_fMinDist;
                pFollow->m_fDistClipMax = g_fMaxDist;

                if (m_fCameraDist != 0.0f) {
                    pFollow->m_fDist = m_fCameraDist;
                    pFollow->m_fYaw = m_fCameraYaw;
                    pFollow->m_fPitch = m_fCameraPitch;
                }

                pFollow->m_vOffset = m_vCameraOffset;
            }
        }

        CSWCCreature* pOldCreature = static_cast<CSWCCreature*>(
            g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(oidOld));
        if (pOldCreature != 0) {
            pOldCreature->UpdateShadowBlob();
        }

        if (pSWCObj->AsSWCCreature() != 0) {
            pSWCObj->AsSWCCreature()->UpdateShadowBlob();
        }
    }
}

// Reads the module description off the wire, builds the area and camera, and attaches the HAK file.
int CSWCModule::LoadModule() {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();

    m_pArea = new CSWCArea();

    if (m_pCamera == 0) {
        m_pCamera = NewCAurCamera(const_cast<char*>("camera"));
        if (m_pCamera == 0) {
            goto cleanup_area;
        }
    }

    m_pCamera->SetClipDist(0.1f, 10000.0f);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCameraMode(CClientOptions::CAMERA_MODE_ON_A_STICK);

    m_sModuleName = pMsg->ReadCExoString(32);
    m_sModuleDisplayName = static_cast<CSWCMessage*>(pMsg)->ReadCExoLocStringClient();
    m_nMinutesPerHour = pMsg->ReadBYTE(8);
    m_cModuleHak = pMsg->ReadCResRef(16);
    m_cStartMovie = pMsg->ReadCResRef(16);

    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SetMinutesPerHour(m_nMinutesPerHour);

    if (m_cModuleHak != const_cast<char*>("")) {
        CExoString sHak;
        sHak = "HAK:";
        sHak = sHak + CExoString(m_cModuleHak.GetResRef(), 16);
        if (g_pExoResMan->AddEncapsulatedResourceFile(sHak, 2) == 0) {
            if (m_pCamera != 0) {
                delete m_pCamera;
                m_pCamera = 0;
            }
            if (m_pArea != 0) {
                delete m_pArea;
                m_pArea = 0;
            }
            return 0;
        }
    }

    if (pMsg->MessageReadOverflow(1) != 0 || pMsg->MessageReadUnderflow(1) != 0) {
        goto cleanup_all;
    }

    g_pAppManager->m_pClientExoApp->SetLoadScreenByModuleName(m_sModuleName);
#if KOTOR1
    g_pAppManager->m_pClientExoApp->ShowLoadScreen(1, 1, 0);
#else
    g_pAppManager->m_pClientExoApp->ShowLoadScreen(1, 1, 0, 1);
#endif
    {
        int nStep1 = g_pAppManager->m_pClientExoApp->GetLoadStep(1);
        int nStep0 = g_pAppManager->m_pClientExoApp->GetLoadStep(0);
        g_pAppManager->m_pClientExoApp->SetLoadBarProgress(nStep1 + nStep0, 1);
    }
    UpdateScreen(0.033333335f, 0, 0);
    return 1;

cleanup_all:
    if (m_pCamera != 0) {
        delete m_pCamera;
        m_pCamera = 0;
    }
cleanup_area:
    if (m_pArea != 0) {
        delete m_pArea;
        m_pArea = 0;
    }
    return 0;
}

// Computes the camera target's next position, the blended follow position plus the view direction
// scaled by the clamped distance.
int CSWCModule::GetNextCameraPosition(float fDeltaTime, Vector* pvPos) {
    Quaternion qTarget;
    if (m_pCamera == 0) return false;
    CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidCameraFollow);
    if (pObj == 0) return false;
    CSWCObject* pSWCObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
    if (pSWCObj == 0) return false;
    Gob* pModel = pSWCObj->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
    if (pModel == 0) return false;

    float fYaw = m_pCamera->GetYaw() + m_fYawVel * fDeltaTime;
    float fPitch = m_pCamera->GetPitch() + m_fPitchVelCache * fDeltaTime;
    float fDist = m_pCamera->GetDist();
    float fDecay = exp2f(m_fDistDecay * fDeltaTime * -0.5f);

    fPitch = (fPitch < g_fMaxPitch) ? fPitch : g_fMaxPitch;
    float fClampedPitch = (g_fMinPitch > fPitch) ? g_fMinPitch : fPitch;

    float fScaledDist = fDist * fDecay;
    fScaledDist = (fScaledDist < g_fMaxDist) ? fScaledDist : g_fMaxDist;
    float fClampedDist = (g_fMinDist > fScaledDist) ? g_fMinDist : fScaledDist;

    float fBlend = 1.0f - exp2f(fDeltaTime * -750.0f);

    Vector vTargetPos = pModel->GetPosition();
    float fInvBlend = 1.0f - fBlend;
    Vector vBlended = Vector{
        (vTargetPos.x + m_vCameraOffset.x) * fBlend + m_vCameraPos.x * fInvBlend,
        (vTargetPos.y + m_vCameraOffset.y) * fBlend + m_vCameraPos.y * fInvBlend,
        (vTargetPos.z + m_vCameraOffset.z) * fBlend + m_vCameraPos.z * fInvBlend
    };

    qTarget = YawPitchRoll(fYaw, fClampedPitch, 0.0f);
    Quaternion qCam = m_pCamera->GetOrientation();
    Quaternion q = normalize(slerp(qCam, qTarget, fBlend));

    Vector vDir = q.zdir();

    m_vCameraPos = vBlended;
    pvPos->z = vBlended.z + vDir.z * fClampedDist;
    pvPos->x = vBlended.x + vDir.x * fClampedDist;
    pvPos->y = vBlended.y + vDir.y * fClampedDist;
    return true;
}

float CSWCModule::UpdateNormalCameraCollision(float fDeltaTime, float fDistDelta) {
    CAurBehaviorCameraFollow* pFollow = 0;
    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior != 0) pFollow = pBehavior->AsBehaviorCameraFollow();
    // Without a follow behaviour the result is 0, not fDistDelta.
    if (pFollow == 0) return 0.0f;

    CAurScene* pScene = 0;
    if (m_pArea != 0) pScene = m_pArea->m_pScene;
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pPlayer == 0 || pScene == 0 || m_pCamera == 0) return fDistDelta;
    if (!pPlayer->ModelIsLoaded()) return fDistDelta;

    Vector vCamPos = m_pCamera->GetPosition();
    Vector vStart = pPlayer->m_position + m_vCameraOffset;
    Vector vDelta = vStart - vCamPos;
    float fSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
    if (!(fSq > 0.0f)) return fDistDelta;

    CHitInfo cHitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
    Vector vDir = normalize(vDelta);
    float fPushDist = g_fMaxDist;   // only the push casts below use it

    static Vector vLastPos;
    static float fOldDist;
    if (vLastPos.x == 0.0f && vLastPos.y == 0.0f && vLastPos.z == 0.0f)
        vLastPos = pPlayer->m_position;

    float fYaw = m_pCamera->GetYaw() + m_fYawVel * fDeltaTime;
    float fPitch = m_pCamera->GetPitch() + m_fPitchVelCache * fDeltaTime;
    float fCamDist = m_pCamera->GetDist();
    float fDecay = exp2f(fDistDelta * fDeltaTime * -0.5f);
    float fPitchClamped = max(min(fPitch, g_fMaxPitch), g_fMinPitch);
    float fDist = fCamDist * fDecay;
    fDist = min(fDist, g_fMaxDist);
    float fBlend = 1.0f - exp2f(fDeltaTime * -750.0f);
    float fInvBlend = 1.0f - fBlend;

    Quaternion q = YawPitchRoll(fYaw, fPitchClamped, 0.0f);
    Quaternion qCam = m_pCamera->GetOrientation();
    q = normalize(slerp(qCam, q, fBlend));

    Vector vDir2 = q.zdir();

    Vector vPos = pPlayer->m_position;
    Vector vNew = (vPos + Vector(0.0f, 0.0f, 1.0f)) * fBlend + vLastPos * fInvBlend;
    vLastPos = vNew;

    float fEndDist = max(fDist, g_fMinDist);
    Vector vCamEnd = vNew + vDir2 * fEndDist;
    Vector vHit1 = Vector{0.0f, 0.0f, 0.0f};
    Vector vHit2 = Vector{0.0f, 0.0f, 0.0f};
    float fHit = 0.0f;          // the collision-limit distance
    float fRet = fDistDelta;    // the return accumulator
    bool bIsPush = false;       // set when the push arm produced the result
    bool bMoved = false;

    // A negative delta casts from the destination outward first; the normal cast below runs only
    // when nothing blocks it.
    if (fDistDelta < 0.0f) {
        bool bPushHit;
        cHitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
        cHitInfo.m_bSkipObjects = 1;
        if (pScene->HitCheck(vCamEnd, vStart - vDir * fPushDist, &vHit1, &cHitInfo) == 0) {
            cHitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
            cHitInfo.m_bSkipObjects = 1;
            bPushHit = (pScene->HitCheck(vStart - vDir * fPushDist, vCamEnd, &vHit2, &cHitInfo) != 0);
        } else {
            vHit1 = vHit1 + vDir * 0.001f;
            cHitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
            cHitInfo.m_bSkipObjects = 1;
            bPushHit = (pScene->HitCheck(vHit1, vCamEnd, &vHit2, &cHitInfo) != 0);
        }
        if (bPushHit == 0) {
            bIsPush = true;
            goto SHOT;   // unresolved: try the normal cast
        }
        vHit2 = vHit2 - vDir * 0.001f;
        fHit = magnitude(vHit2 - vStart);
        bMoved = false;
        fOldDist = 0.0f;
        bIsPush = true;
        goto JOINOUT;
    }

SHOT:
    // The camera is inside the clip, so push it out along the direction, or fall back to the
    // persisted distance window.
    bool bSecond;
    cHitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
    cHitInfo.m_bSkipObjects = 1;
    if (pScene->HitCheck(vCamEnd, vStart, &vHit1, &cHitInfo) == 0) {
        cHitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
        cHitInfo.m_bSkipObjects = 1;
        bSecond = (pScene->HitCheck(vStart, vCamEnd, &vHit2, &cHitInfo) != 0);
    } else {
        vHit1 = vHit1 - vDir * 0.001f;
        cHitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
        cHitInfo.m_bSkipObjects = 1;
        bSecond = (pScene->HitCheck(vHit1, vCamEnd, &vHit2, &cHitInfo) != 0);
    }

    if (bSecond != 0) {
        // The second shot succeeded, so the hit distance is final.
        if (fOldDist == 0.0f) fOldDist = m_pCamera->GetDist();
        vHit2 = vHit2 + vDir * 0.001f;
        fHit = magnitude(vHit2 - vStart);
        bMoved = false;
        fRet = bIsPush == 0 ? fDistDelta : 0.0f;
    } else {
        // Both casts failed, so clamp to the persisted distance window.
        if (!(fOldDist > 0.0f)) {
            bMoved = false;
            fHit = 0.0f;
        } else {
            bool bR;
            cHitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
            cHitInfo.m_bSkipObjects = 1;
            if (m_pCamera->GetDist() < fOldDist) {
                bR = (pScene->HitCheck(vCamEnd, vStart - vDir * fOldDist, &vHit1, &cHitInfo) != 0);
            } else {
                bR = (pScene->HitCheck(vStart - vDir * fOldDist, vCamEnd, &vHit1, &cHitInfo) != 0);
            }
            if (bR != 0) {
                vHit1 = vHit1 + vDir * 0.001f;
                float fA = m_pCamera->GetDist();
                float fB = fOldDist;
                float fC = m_pCamera->GetDist();
                fHit = fA + (fB - fC) * 3.5f * fDeltaTime;
                bMoved = false;
                Vector vD = vHit1 - vStart;
                float fHitSq = vD.x * vD.x + vD.y * vD.y + vD.z * vD.z;
                if (fHit * fHit >= fHitSq) fHit = __builtin_sqrtf(fHitSq);
            } else {
                float fA = m_pCamera->GetDist();
                float fB = fOldDist;
                float fC = m_pCamera->GetDist();
                fHit = fA + (fB - fC) * 3.5f * fDeltaTime;
                if (fHit - fOldDist >= -0.0001f) {
                    bMoved = true;
                    fHit = fOldDist;
                    fOldDist = 0.0f;
                } else {
                    bMoved = true;
                }
            }
        }
    }

    // Both paths end by gating the result against the clip window.
    if (fDistDelta > 0.0f) fOldDist = 0.0f;
    if ((fHit == 0.0f && m_pCamera->GetDist() < g_fMinDist) ||
        (fHit > 0.0f && g_fMinDist - fHit >= 0.0001f)) {
        fRet = 0.0f;
        bIsPush = false;
        if (fOldDist < g_fMinDist) fOldDist = g_fMinDist;
    } else {
        bIsPush = true;
    }

JOINOUT:
    // Report the clip distance to the follow behaviour while the camera is locked, then gate the
    // return value against the clip window.
    if (lockcamera(0) != 0) {
        pFollow->m_fDistClipGate = bIsPush == 0 ? 0.0f : 1.0f;
        if (fHit > 0.0f) {
            fRet = bMoved == 0 ? 0.0f : fRet;
            pFollow->m_fDist = fHit;
            this->m_bCameraMoved = 1;
        }
        float fNear = m_pCamera->GetDist();
        float fMask = (fNear <= g_fMinDist && fRet > 0.0f) ? 0.0f : fRet;
        float fFar = m_pCamera->GetDist();
        fDistDelta = (fFar >= g_fMaxDist && fMask < 0.0f) ? 0.0f : fMask;
    }
    return fDistDelta;
}

// The camera view console commands are viewmindist, viewmaxdist, viewminpitch, viewmaxpitch,
// viewfollowoffset and setcameramode. The four clip commands share one shape: report the current
// value when the argument is empty, otherwise format "<name> <value>" for the camera's
// BehaviorMessage and store the parsed float into the matching clip tunable.
// Wraps __vsprintf_chk with an explicit size, since the call sites disagree on both the buffer and
// the size.
static int sprintf_s(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int nResult = __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
    return nResult;
}

// Shared result buffer for the view* commands' empty-argument case.
static char g_szViewResult[256];

const char* viewmindist(char* pszArgs) {
    char szBuffer[256];
    if (strlen(pszArgs) != 0) {
        sprintf_s(szBuffer, sizeof(szBuffer), "mindist %s", pszArgs);
        if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
            || g_pAppManager->m_pClientExoApp->GetModule() == 0) {
            return "Failure";
        }
        if (g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera == 0) {
            return "Failure";
        }
        g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->BehaviorMessage(szBuffer);
        g_fMinDist = static_cast<float>(atof(pszArgs));
        return "Success";
    }
    sprintf_s(g_szViewResult, sizeof(g_szViewResult), "%f", static_cast<double>(g_fMinDist));
    return g_szViewResult;
}

const char* viewmaxdist(char* pszArgs) {
    char szBuffer[256];
    if (strlen(pszArgs) != 0) {
        sprintf_s(szBuffer, sizeof(szBuffer), "maxdist %s", pszArgs);
        if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
            || g_pAppManager->m_pClientExoApp->GetModule() == 0) {
            return "Failure";
        }
        if (g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera == 0) {
            return "Failure";
        }
        g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->BehaviorMessage(szBuffer);
        g_fMaxDist = static_cast<float>(atof(pszArgs));
        return "Success";
    }
    sprintf_s(g_szViewResult, sizeof(g_szViewResult), "%f", static_cast<double>(g_fMaxDist));
    return g_szViewResult;
}

const char* viewminpitch(char* pszArgs) {
    char szBuffer[256];
    if (strlen(pszArgs) != 0) {
        sprintf_s(szBuffer, sizeof(szBuffer), "minpitch %s", pszArgs);
        if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
            || g_pAppManager->m_pClientExoApp->GetModule() == 0) {
            return "Failure";
        }
        if (g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera == 0) {
            return "Failure";
        }
        g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->BehaviorMessage(szBuffer);
        g_fMinPitch = static_cast<float>(atof(pszArgs));
        return "Success";
    }
    sprintf_s(g_szViewResult, sizeof(g_szViewResult), "%f", static_cast<double>(g_fMinPitch));
    return g_szViewResult;
}

const char* viewmaxpitch(char* pszArgs) {
    char szBuffer[256];
    if (strlen(pszArgs) != 0) {
        sprintf_s(szBuffer, sizeof(szBuffer), "maxpitch %s", pszArgs);
        if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
            || g_pAppManager->m_pClientExoApp->GetModule() == 0) {
            return "Failure";
        }
        if (g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera == 0) {
            return "Failure";
        }
        g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->BehaviorMessage(szBuffer);
        g_fMaxPitch = static_cast<float>(atof(pszArgs));
        return "Success";
    }
    sprintf_s(g_szViewResult, sizeof(g_szViewResult), "%f", static_cast<double>(g_fMaxPitch));
    return g_szViewResult;
}

const char* viewfollowoffset(char* pszArgs) {
    Vector vOffset = Vector{0.0f, 0.0f, 0.0f};
    char szBuffer[64];
    static_cast<void>(sscanf(pszArgs, "%f %f %f", &vOffset.x, &vOffset.y, &vOffset.z));
    sprintf_s(szBuffer, sizeof(szBuffer), "offset %f %f %f", static_cast<double>(vOffset.x), static_cast<double>(vOffset.y),
              static_cast<double>(vOffset.z));
    if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0
        || g_pAppManager->m_pClientExoApp->GetModule() == 0) {
        return "Failure";
    }
    if (g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera == 0) {
        return "Failure";
    }
    g_pAppManager->m_pClientExoApp->GetModule()->m_vCameraOffset = vOffset;
    g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->BehaviorMessage(szBuffer);
    return "Success";
}

const char* setcameramode(char* pszArgs) {
    int nMode;
    static_cast<void>(sscanf(pszArgs, "%i", &nMode));
    const char* pszResult;
    if (g_pAppManager == 0 || g_pAppManager->m_pClientExoApp == 0 ||
        g_pAppManager->m_pClientExoApp->GetClientOptions() == 0) {
        pszResult = "Failure";
    } else {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCameraMode(static_cast<unsigned char>(nMode));
        pszResult = "Success";
    }
    return pszResult;
}

float CSWCModule::UpdatePushCameraCollision(float fDeltaTime, float fDistDelta) {
    CAurBehaviorCameraFollow* pFollow = 0;
    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior != 0) pFollow = pBehavior->AsBehaviorCameraFollow();
    // Without a follow behaviour the result is 0, not fDistDelta.
    if (pFollow == 0) return 0.0f;

    CAurScene* pScene = 0;
    if (m_pArea != 0) pScene = m_pArea->m_pScene;
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pPlayer == 0 || pScene == 0 || m_pCamera == 0) return fDistDelta;
    if (!pPlayer->ModelIsLoaded()) return fDistDelta;

    Vector vCamPos = m_pCamera->GetPosition();
    Vector vStart = pPlayer->m_position + m_vCameraOffset;
    Vector vDelta = vStart - vCamPos;
    float fSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
    if (!(fSq > 0.0f)) return fDistDelta;

    CHitInfo cHitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
    Vector vHit = Vector{0.0f, 0.0f, 0.0f};
    Vector vDir = normalize(vDelta);
    static Vector vLastPos;         // Vector's default constructor is non-trivial: dynamic init
    static float fOldDist;          // no initialiser, so zero at load time

    if (vLastPos.x == 0.0f && vLastPos.y == 0.0f && vLastPos.z == 0.0f)
        vLastPos = pPlayer->m_position;

    float fYaw = m_pCamera->GetYaw() + m_fYawVel * fDeltaTime;
    float fPitch = m_pCamera->GetPitch() + m_fPitchVelCache * fDeltaTime;
    float fCamDist = m_pCamera->GetDist();
    float fDecay = exp2f(fDistDelta * fDeltaTime * -0.5f);
    float fPitchClamped = max(min(fPitch, g_fMaxPitch), g_fMinPitch);
    float fDist = fCamDist * fDecay;
    fDist = min(fDist, g_fMaxDist);
    float fBlend = 1.0f - exp2f(fDeltaTime * -750.0f);

    Quaternion q = YawPitchRoll(fYaw, fPitchClamped, 0.0f);
    Quaternion qCam = m_pCamera->GetOrientation();
    q = normalize(slerp(qCam, q, fBlend));

    Vector vDir2 = q.zdir();

    Vector vPos = pPlayer->m_position;
    Vector vNew = (vPos + Vector(0.0f, 0.0f, 1.0f)) * fBlend + vLastPos * (1.0f - fBlend);
    vLastPos = vNew;

    float fEndDist = max(fDist, g_fMinDist);
    Vector vCamEnd = vNew + vDir2 * fEndDist;
    float fHit = 0.0f;          // the collision-limit distance
    float fRet = fDistDelta;    // the return accumulator
    bool bIsPush = false;
    bool bMoved = false;

    // Casts from the destination outward; if the cast resolves, the clip distance is final.
    cHitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
    cHitInfo.m_bSkipObjects = 1;
    if (pScene->HitCheck(vStart, vCamEnd, &vHit, &cHitInfo) != 0) {
        // The shot resolved, so the hit distance is final; persist the window distance.
        vHit = vHit + vDir * 0.001f;
        fHit = magnitude(vHit - vStart);
        if (fOldDist != 0.0f) {
            bMoved = false;
        } else {
            bMoved = false;
            fOldDist = m_pCamera->GetDist();
        }
    } else {
        // Unresolved, so clamp to the persisted distance window.
        if (!(fOldDist > 0.0f)) {
            bMoved = false;
            fHit = 0.0f;
        } else {
            bool bR;
            cHitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
            cHitInfo.m_bSkipObjects = 1;
            if (m_pCamera->GetDist() < fOldDist) {
                bR = (pScene->HitCheck(vCamEnd, vStart - vDir * fOldDist, &vHit, &cHitInfo) != 0);
            } else {
                bR = (pScene->HitCheck(vStart - vDir * fOldDist, vCamEnd, &vHit, &cHitInfo) != 0);
            }
            if (bR != 0) {
                vHit = vHit + vDir * 0.001f;
                float fA = m_pCamera->GetDist();
                float fB = fOldDist;
                float fC = m_pCamera->GetDist();
                fHit = fA + (fB - fC) * 3.5f * fDeltaTime;
                bMoved = false;
                Vector vD = vHit - vStart;
                float fHitSq = vD.x * vD.x + vD.y * vD.y + vD.z * vD.z;
                if (fHitSq <= fHit * fHit) fHit = __builtin_sqrtf(fHitSq);
            } else {
                float fA = m_pCamera->GetDist();
                float fB = fOldDist;
                float fC = m_pCamera->GetDist();
                fHit = fA + (fB - fC) * 3.5f * fDeltaTime;
                if (fHit >= fOldDist) {
                    bMoved = true;
                    fOldDist = 0.0f;
                    fHit = fB;
                } else {
                    bMoved = true;
                }
            }
        }
    }

    // shared ending - gate the result against the clip window
    if (fDistDelta > 0.0f) fOldDist = 0.0f;
    if ((fHit == 0.0f && m_pCamera->GetDist() < g_fMinDist) ||
        (fHit > 0.0f && g_fMinDist - fHit >= 0.0001f)) {
        fRet = 0.0f;
        bIsPush = false;
        if (fOldDist < g_fMinDist) fOldDist = g_fMinDist;
    } else {
        bIsPush = true;
    }

    // Report the clip distance to the follow behaviour while the camera is locked, then gate the
    // return value against the clip window.
    if (lockcamera(0) != 0) {
        pFollow->m_fDistClipGate = bIsPush == 0 ? 0.0f : 1.0f;
        if (fHit > 0.0f) {
            fRet = bMoved == 0 ? 0.0f : fRet;
            pFollow->m_fDist = fHit;
            this->m_bCameraMoved = 1;
        }
        float fNear = m_pCamera->GetDist();
        float fMask = (fNear <= g_fMinDist && fRet > 0.0f) ? 0.0f : fRet;
        float fFar = m_pCamera->GetDist();
        fDistDelta = (fFar >= g_fMaxDist && fMask < 0.0f) ? 0.0f : fMask;
    }
    return fDistDelta;
}

void CSWCModule::LoadAnimatedCamera(const CResRef& resRef) {
    if (m_pAnimatedCameraModel == 0 && resRef != const_cast<char*>("")) {
        CExoString sResRef;
        resRef.CopyToString(sResRef);
        m_pAnimatedCameraModel = NewCAurObject(static_cast<char*>(sResRef.CStr()), const_cast<char*>(""), 0, 0);
    }
}

void CSWCModule::SetDialogCamera(int nParam) {
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick != 0) {
            pOnAStick->SaveCameraSettings(m_cSavedCameraSettings);
        }
    }
    CSWBehaviorCameraDialog* pDialog = new CSWBehaviorCameraDialog(m_pCamera->AsCAurObject(), nParam);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCameraMode(CClientOptions::CAMERA_MODE_DIALOG);
    m_pCamera->SetBehavior(pDialog);
}

// Attaches the pre-loaded animated-camera model to the scene, parents the real camera to its
// "camerahook" part and starts the cutscene animation. Returns the animation's length, which is what
// the dialog code schedules the shot against.
float CSWCModule::SetAnimatedCamera(unsigned short nAnimation, float fFOV, unsigned long long oid) {
    if (oid == OBJECT_INVALID || m_pAnimatedCameraModel == 0) {
        return 0.0f;
    }

    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick != 0) {
            pOnAStick->SaveCameraSettings(m_cSavedCameraSettings);
        }
    }
    CAurCamera* pCamera = m_pCamera;
    pCamera->SetClipDist(0.1f, 10000.0f);
    m_pAnimatedCameraModel->AttachToScene(m_pArea->m_pScene);

    CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(oid);
    if (pObj == 0) {
        return 0.0f;
    }
    // AsSWCObject is dispatched twice; the duplicate is kept.
    CSWCObject* pSWCObject = static_cast<CSWCObject*>(pObj->AsSWCObject());
    if (pSWCObject == 0) {
        return 0.0f;
    }
    pSWCObject = static_cast<CSWCObject*>(pObj->AsSWCObject());

    m_pAnimatedCameraModel->SetPosition(Vector{0.0f, 0.0f, 0.0f});
    m_pAnimatedCameraModel->SetOrientation(Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
    pCamera->AttachToObject(m_pAnimatedCameraModel, const_cast<char*>("camerahook"), 0);

    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    m_sAnimatedCameraAnim = pGui->GetCameraAnimationName(nAnimation);
    m_pAnimatedCameraModel->EnableAlwaysRender();
    m_pAnimatedCameraModel->EnableInCutscene();

    float fStart;
    float fLength;
    m_pAnimatedCameraModel->GetAnimation(m_sAnimatedCameraAnim.CStr(), &fStart, &fLength);
    if (fLength <= 0.0f) {
        fLength = 0.0f;
    }
    int bLooping = pGui->IsLoopingDialogStuntAnimation(nAnimation);
    m_pAnimatedCameraModel->PlayAnimation(m_sAnimatedCameraAnim.CStr(), 1.0f, bLooping ? 2 : 3, 0.0f);
    if (fFOV > 0.0f) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetDeferredFOVCallForAnimatedCutscenes(fFOV);
    }
    return fLength;
}

// Drives the per-frame camera bump/shake. Whichever of the four shakeable camera behaviours is
// currently installed gets the yaw/pitch offset pair; the first non-null one wins, since they are
// mutually exclusive in practice. Note the random sign: (rand() + 1) % 2 - 1 yields only 0 or -1,
// never +1, so every shake pushes the same way.
void CSWCModule::UpdateCameraBumpAndShake(float fDeltaTime) {
    CSWCameraOnAStick*       pOnAStick = static_cast<CSWCameraOnAStick*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
    CSWBehaviorCameraDialog* pDialog   = static_cast<CSWBehaviorCameraDialog*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_DIALOG));
    CSWCameraFreeLook*       pFreeLook = static_cast<CSWCameraFreeLook*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_FREE_LOOK));
    CSWBehaviorCameraCombat* pCombat   = static_cast<CSWBehaviorCameraCombat*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_COMBAT));
    if (pOnAStick == 0 && pDialog == 0 && pFreeLook == 0 && pCombat == 0) {
        return;
    }

    if (!g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bCameraShakeEnabled) {
        if (m_nBumpTimeLeftMs != 0) {
            m_nBumpTimeLeftMs = 0;
            m_nBumpParam = 0;
            m_bBumpReset = 1;
        } else if (m_nShakeTimeLeftMs > 0) {
            m_nShakeDelayMs = 0;
            m_nShakeTimeLeftMs = 0;
            m_nShakeIntervalMs = 0;
            m_bShakeReset = 1;
        }
    }

    if (m_nShakeTimeLeftMs <= 0) {
        return;
    }

    int nElapsedMs = static_cast<int>(fDeltaTime * 1000.0f);
    if (m_nShakeDelayMs > 0) {
        if (m_nShakeDelayMs <= nElapsedMs) {
            nElapsedMs -= m_nShakeDelayMs;
            m_nShakeDelayMs = 0;
        } else {
            m_nShakeDelayMs -= nElapsedMs;
        }
    }
    if (m_nShakeDelayMs != 0) {
        return;
    }

    float fShakeYaw;
    float fShakePitch;
    if (m_nShakeTimeLeftMs > nElapsedMs) {
        double dPhase = m_nShakeTimeLeftMs * 6.2831852;
        float fEnvelope = fShakeMag * (static_cast<float>(cos(dPhase / m_nShakePeriodMs)) + 1.0f);
        float fAmplitude = fEnvelope * static_cast<float>(sin(dPhase / fShakePeriod)) * 0.017453292f;
        fShakeYaw   = fAmplitude * ((rand_wincompatible() + 1) % 2 - 1);
        fShakePitch = fAmplitude * ((rand_wincompatible() + 1) % 2 - 1);
    } else {
        m_nShakeTimeLeftMs = 0;
        fShakeYaw = 0.0f;
        fShakePitch = 0.0f;
    }

    if (pOnAStick != 0) {
        pOnAStick->m_fShakeYaw = fShakeYaw;
        pOnAStick->m_fShakePitch = fShakePitch;
    } else if (pCombat != 0) {
        pCombat->m_fShakeYaw = fShakeYaw;
        pCombat->m_fShakePitch = fShakePitch;
    } else if (pDialog != 0) {
        pDialog->m_fShakeYaw = fShakeYaw;
        pDialog->m_fShakePitch = fShakePitch;
    } else if (pFreeLook != 0) {
        pFreeLook->m_fShakeYaw = fShakeYaw;
        pFreeLook->m_fShakePitch = fShakePitch;
    }
    m_nShakeTimeLeftMs -= nElapsedMs;
}

// Hands camera control to CSWCameraFreeLook. When leaving the on-a-stick camera it first turns the
// player to face away from where the camera was standing, so free-look starts aligned with the
// previous view: the flattened horizontal direction from the camera to the creature.
bool CSWCModule::SetFreeLookCamera(unsigned long long oid) {
    CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(oid);
    if (pObj == 0) return false;
    CSWCCreature* pCreature = static_cast<CSWCCreature*>(pObj->AsSWCCreature());
    if (pCreature == 0) return false;

    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick != 0) {
            pOnAStick->SaveCameraSettings(m_cSavedCameraSettings);
            Vector vCameraPos = { 0.0f, 0.0f, 0.0f };
            if (pOnAStick->m_pCameraObject != 0) {
                vCameraPos = pOnAStick->m_pCameraObject->GetPosition();
            }
            Vector vFacing;
            vFacing.x = pCreature->m_position.x - vCameraPos.x;
            vFacing.y = pCreature->m_position.y - vCameraPos.y;
            vFacing.z = 0.0f;
            pCreature->SetOrientation(normalize(vFacing));
        }
    }

    CSWCameraFreeLook* pFreeLook = new CSWCameraFreeLook(
        m_pCamera->AsCAurObject(),
        static_cast<CAurObject*>(pCreature->GetCameraFollowPart()),
        pCreature->GetFreeLookVideoEffect());
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCameraMode(CClientOptions::CAMERA_MODE_FREE_LOOK);
    m_pCamera->SetBehavior(pFreeLook);
    m_nCameraMode = CClientOptions::CAMERA_MODE_FREE_LOOK;
    return true;
}

bool CSWCModule::SetCombatCamera() {
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick != 0) {
            pOnAStick->SaveCameraSettings(m_cSavedCameraSettings);
        }
    }
    CSWBehaviorCameraCombat* pCombat = new CSWBehaviorCameraCombat(m_pCamera->AsCAurObject());
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCameraMode(CClientOptions::CAMERA_MODE_COMBAT);
    m_pCamera->SetBehavior(pCombat);
    return true;
}

bool CSWCModule::SetNavigateCamera() {
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
        if (pOnAStick == 0) {
            return false;
        }
        pOnAStick->SaveCameraSettings(m_cSavedCameraSettings);
    }
    CSWCameraNavigate* pNav = new CSWCameraNavigate(m_pCamera->AsCAurObject());
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCameraMode(CClientOptions::CAMERA_MODE_NAVIGATE);
    m_pCamera->SetBehavior(pNav);
    return true;
}

void CSWCModule::UpdateCameraModeChaseCam() {
    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    CAurBehaviorCameraFollow* pFollow = nullptr;
    if (pBehavior)
        pFollow = pBehavior->AsBehaviorCameraFollow();

    void* pScene = m_pArea ? m_pArea->m_pScene : nullptr;

    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();

    if (!pScene || !pPlayer) return;

    UpdateCameraModeNormal();

    CSWCObject* pObj = static_cast<CSWCObject*>(pPlayer);
    if (pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK_BACKWARD ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_RUN ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_STRAFE_LEFT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_STRAFE_RIGHT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK_STRAFE_LEFT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK_STRAFE_RIGHT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_RUN_STRAFE_LEFT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_RUN_STRAFE_RIGHT) {
        if (m_bPlayerStationary) {
            m_bAutoTurnSuppressed = 0;
            m_bPlayerStationary = 0;
        }
    } else {
        m_bPlayerStationary = 1;
        m_vSavedPlayerPos = pPlayer->m_position;
    }

    if (m_bCameraMoved) {
        m_bCameraAutoTurning = 0;
        m_bAutoTurnSuppressed = 1;
        return;
    }

    if ((m_bAutoTurnSuppressed | m_bCameraMoved)) return;

    Quaternion qRot = m_pCamera->GetOrientation();
    float fCamYaw = Yaw(qRot);
    float fPlayerYaw = Yaw(pPlayer->m_orientation);
    float fYawDiff = fPlayerYaw - fCamYaw;

    // Normalize to [-180, 180]
    while (fYawDiff < -180.0f) fYawDiff += 360.0f;
    while (fYawDiff > 180.0f) fYawDiff -= 360.0f;

    if (!pPlayer->m_bInCombat) {
        // The camera sits on one side of the player, so hold the yaw clear of that half turn.
        if (fYawDiff > 90.0f && m_nCameraSide == 2) {
            fYawDiff = -90.0f;
        } else if (fYawDiff < -90.0f && m_nCameraSide == 1) {
            fYawDiff = 90.0f;
        }
    }

    {
        float fAbsYaw = __builtin_fabsf(fYawDiff);
        if (fAbsYaw > 1.0f) {
            if (fAbsYaw > 30.0f || m_bCameraAutoTurning != 0) {
                float fClampedYaw = fYawDiff;
                if (fYawDiff > 90.0f) fClampedYaw = 90.0f;
                else if (fYawDiff < -90.0f) fClampedYaw = -90.0f;

                if (pFollow) pFollow->m_fYawVel = fClampedYaw;
                m_bCameraAutoTurning = 1;
                m_bCameraMoved = 1;
            }
        } else {
            m_bCameraAutoTurning = 0;
        }
    }
}

// A mode change re-seeds the clip range for this camera mode, then runs the same yaw-tracking walk
// as UpdateCameraModeChaseCam.
void CSWCModule::UpdateCameraModeEQ(float fDeltaTime) {
    void* pScene = m_pArea ? m_pArea->m_pScene : nullptr;

    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();

    if (!pScene || !pPlayer) return;

    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    CAurBehaviorCameraFollow* pFollow = nullptr;
    if (pBehavior)
        pFollow = pBehavior->AsBehaviorCameraFollow();

    // A mode change re-seeds the clip range for this camera mode.
    if (m_bCameraModeChanged && pFollow) {
        pFollow->m_fDistClipMin = 6.0f;
        pFollow->m_fDistClipMax = 6.0f;
        pFollow->m_fPitchClipMin = 1.0f;
        pFollow->m_fPitchClipMax = 62.0f;
        g_fMinDist = 6.0f;
        g_fMaxDist = 6.0f;
        g_fMinPitch = 1.0f;
        g_fMaxPitch = 62.0f;
        m_vCameraOffset.x = 0.0f;
        m_vCameraOffset.y = 0.0f;
        m_vCameraOffset.z = 2.0f;
        pFollow->m_vOffset = m_vCameraOffset;

        float fNearClip, fFarClip;
        m_pCamera->GetClipDist(fNearClip, fFarClip);
        m_pCamera->SetClipDist(fNearClip, 45.0f);

        pFollow->m_fDist = 6.0f;
        pFollow->m_fPitch = 62.0f;
        m_bCameraModeChanged = 0;
    }

    CSWCObject* pObj = static_cast<CSWCObject*>(pPlayer);
    if (pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK_BACKWARD ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_RUN ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_STRAFE_LEFT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_STRAFE_RIGHT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK_STRAFE_LEFT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK_STRAFE_RIGHT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_RUN_STRAFE_LEFT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_RUN_STRAFE_RIGHT) {
        if (m_bPlayerStationary) {
            m_bAutoTurnSuppressed = 0;
            m_bPlayerStationary = 0;
        }
        if (m_bCameraMoved) {
            m_bCameraAutoTurning = 0;
            m_bAutoTurnSuppressed = 1;
            return;
        }
    } else {
        m_bPlayerStationary = 1;
        m_vSavedPlayerPos = pPlayer->m_position;
        if (m_bCameraMoved) {
            m_bCameraAutoTurning = 0;
            m_bAutoTurnSuppressed = 1;
            return;
        }
    }

    if ((m_bAutoTurnSuppressed | m_bCameraMoved)) return;

    // The raw difference stays live for the exact-rate divide below.
    float fCamYaw = m_pCamera->GetYaw();
    float fPlayerYaw = Yaw(pPlayer->m_orientation);
    float fRawYawDiff = fPlayerYaw - fCamYaw;
    float fYawDiff = fRawYawDiff;

    // Normalize to [-180, 180]
    while (fYawDiff < -180.0f) fYawDiff += 360.0f;
    while (fYawDiff > 180.0f) fYawDiff -= 360.0f;

    if (!pPlayer->m_bInCombat) {
        // The camera sits on one side of the player, so hold the yaw clear of that half turn.
        if (fYawDiff > 90.0f && m_nCameraSide == 2) {
            fYawDiff = -90.0f;
        } else if (fYawDiff < -90.0f && m_nCameraSide == 1) {
            fYawDiff = 90.0f;
        }
    }

    if (__builtin_fabsf(fYawDiff) > 1.0f) {
        // An angular velocity in degrees per second, so it is clamped rather than wrapped the way the
        // yaw angles above are.
        float fYawVel = fYawDiff * 5.0f;
        if (fYawVel > 180.0f) fYawVel = 180.0f;
        else if (fYawVel < -180.0f) fYawVel = -180.0f;

        float fNewYaw = fYawVel * fDeltaTime + fCamYaw;
        float fYawDiff2 = fPlayerYaw - fNewYaw;
        while (fYawDiff2 < -180.0f) fYawDiff2 += 360.0f;
        while (fYawDiff2 > 180.0f) fYawDiff2 -= 360.0f;

        // When the remaining error crosses zero the rate overshoots, so use the exact rate that lands
        // on the target this frame, computed from the raw difference before normalization.
        float fThreshold = 0.0001f;
        if ((fYawVel > fThreshold && fYawDiff2 < fThreshold) ||
            (fYawVel < fThreshold && fYawDiff2 > fThreshold)) {
            fYawVel = fRawYawDiff / fDeltaTime;
        }

        if (pFollow) pFollow->m_fYawVel = fYawVel;
        m_bCameraAutoTurning = 1;
        m_bCameraMoved = 1;
    } else {
        m_bCameraAutoTurning = 0;
    }
}

// The pre-move difference between the saved and current player yaw and the camera yaw decides which
// side the camera swings to; the rate is clamped, and an overshooting rate is replaced by the exact
// rate that lands on the target.
void CSWCModule::UpdateCameraModeSW(float fDeltaTime) {
    void* pScene = m_pArea ? m_pArea->m_pScene : nullptr;

    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();

    if (!pScene || !pPlayer) return;

    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    CAurBehaviorCameraFollow* pFollow = nullptr;
    if (pBehavior)
        pFollow = pBehavior->AsBehaviorCameraFollow();

    // A mode change re-seeds the clip range for this camera mode.
    if (m_bCameraModeChanged && pFollow) {
        pFollow->m_fDistClipMin = 6.5f;
        pFollow->m_fDistClipMax = 6.5f;
        pFollow->m_fPitchClipMin = 1.0f;
        pFollow->m_fPitchClipMax = 62.0f;
        g_fMinDist = 6.5f;
        g_fMaxDist = 6.5f;
        g_fMinPitch = 1.0f;
        g_fMaxPitch = 62.0f;
        m_vCameraOffset.x = 0.0f;
        m_vCameraOffset.y = 0.0f;
        m_vCameraOffset.z = 1.0f;
        pFollow->m_vOffset = m_vCameraOffset;

        float fNearClip, fFarClip;
        m_pCamera->GetClipDist(fNearClip, fFarClip);
        m_pCamera->SetClipDist(fNearClip, 45.0f);

        pFollow->m_fDist = 6.5f;
        pFollow->m_fPitch = 62.0f;
        m_bCameraModeChanged = 0;
    }

    CSWCObject* pObj = static_cast<CSWCObject*>(pPlayer);
    if (pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK_BACKWARD ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_RUN ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_STRAFE_LEFT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_STRAFE_RIGHT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK_STRAFE_LEFT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_WALK_STRAFE_RIGHT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_RUN_STRAFE_LEFT ||
        pObj->GetCurrentAnimation() == SERVER_ANIMATION_RUN_STRAFE_RIGHT) {
        m_bAutoTurnSuppressed = (m_bCameraMoved != 0);
        if (m_bCameraMoved == 0) {
            // The camera is asked for its position twice, over two vectors that are zeroed first.
            Vector vDiff1;
            Vector vDiff2;
            __builtin_memset(&vDiff1, 0, sizeof(Vector));
            __builtin_memset(&vDiff2, 0, sizeof(Vector));

            Vector vPos1 = m_pCamera->GetPosition();
            vDiff1.x = m_vSavedPlayerPos.x - vPos1.x;
            vDiff1.y = m_vSavedPlayerPos.y - vPos1.y;
            vDiff1.z = m_vSavedPlayerPos.z - vPos1.z;

            Vector vPos2 = m_pCamera->GetPosition();
            vDiff2.x = pPlayer->m_position.x - vPos2.x;
            vDiff2.y = pPlayer->m_position.y - vPos2.y;
            vDiff2.z = pPlayer->m_position.z - vPos2.z;

            float fLenSq1 = vDiff1.x * vDiff1.x + vDiff1.y * vDiff1.y + vDiff1.z * vDiff1.z;
            if (fLenSq1 > 1e-05f) {
                vDiff1 = normalize(vDiff1);
            } else {
                vDiff1 = {0.0f, 1.0f, 0.0f};
            }

            float fLenSq2 = vDiff2.x * vDiff2.x + vDiff2.y * vDiff2.y + vDiff2.z * vDiff2.z;
            if (fLenSq2 > 1e-05f) {
                vDiff2 = normalize(vDiff2);
            } else {
                vDiff2 = {1.0f, 0.0f, 0.0f};
            }

            float fCamYaw = Yaw(vDiff1);
            float fPlayerYaw = Yaw(vDiff2);

            // The raw difference is kept because the overshoot rate below divides it, not the
            // wrapped copy.
            float fRawYawDiff = fPlayerYaw - fCamYaw;
            float fYawDiff = fRawYawDiff;
            while (fYawDiff < -180.0f) fYawDiff += 360.0f;
            while (fYawDiff > 180.0f) fYawDiff -= 360.0f;

            float fTargetYaw = (fYawDiff < 0.0f) ? -90.0f : 90.0f;

            float fYawDiff2 = fPlayerYaw - (fCamYaw + fTargetYaw * fDeltaTime);
            while (fYawDiff2 < -180.0f) fYawDiff2 += 360.0f;
            while (fYawDiff2 > 180.0f) fYawDiff2 -= 360.0f;

            if (pFollow) {
                float fResult;
                if ((fTargetYaw > 0.0001f && fYawDiff2 < 0.0001f) ||
                    (fYawDiff < 0.0f && fYawDiff2 > 0.0001f)) {
                    fResult = fRawYawDiff / fDeltaTime;
                } else {
                    fResult = fTargetYaw;
                }
                pFollow->m_fYawVel = fResult;
            }

            // An overshooting rate is replaced by the exact one, as in UpdateCameraModeEQ.
            m_bCameraAutoTurning = 1;
            m_bCameraMoved = 1;
        }

        m_vSavedPlayerPos = pPlayer->m_position;
    } else {
        m_vSavedPlayerPos = pPlayer->m_position;
        if (m_bCameraMoved) {
            m_bCameraAutoTurning = 0;
            m_bAutoTurnSuppressed = 1;
        }
    }
}

int CSWCModule::FindClearView(float fDeltaTime, float* pfYaw, float* pfPitch) {
    CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
    CHitInfo* pHitInfo = &hitInfo;

    Vector vCamPos = {0.0f, 0.0f, 0.0f};
    Vector vOutPos = {0.0f, 0.0f, 0.0f};

    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    CAurBehaviorCameraFollow* pFollow = nullptr;
    if (pBehavior)
        pFollow = pBehavior->AsBehaviorCameraFollow();

    m_bCameraAutoTurning = 0;
    m_bAutoTurnSuppressed = 1;

    if (!m_pArea) return 0;

    CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidCameraFollow);
    if (!pObj) return 0;

    CSWCObject* pSWCObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
    if (!pSWCObj) return 0;

    // The yaw comes from the creature's orientation when it has one, otherwise from the object's.
    CSWCCreature* pCreature = static_cast<CSWCCreature*>(pSWCObj->AsSWCCreature());
    float fYaw;
    if (pCreature) {
        pCreature = static_cast<CSWCCreature*>(pSWCObj->AsSWCCreature());
        fYaw = Yaw(pCreature->m_vStationaryOrientation);
    } else {
        fYaw = Yaw(pSWCObj->m_orientation);
    }

    if (pFollow) {
        pFollow->m_fYaw = fYaw;
    }

    if (!GetNextCameraPosition(fDeltaTime, &vCamPos)) return 0;

    // A missing scene is an early exit.
    CAurScene* pScene = m_pArea->m_pScene;
    if (!pScene) return 0;

    Vector vTargetPos = {
        pSWCObj->m_position.x + m_vCameraOffset.x,
        pSWCObj->m_position.y + m_vCameraOffset.y,
        pSWCObj->m_position.z + m_vCameraOffset.z
    };

    pHitInfo->m_bSkipObjects = 1;

    // A clear line of sight from the camera to the target is required; a miss exits with no yaw
    // fallback.
    if (!pScene->HitCheck(vCamPos, vTargetPos, &vOutPos, pHitInfo))
        return 0;

    // Try the reflected position, 180 degrees around the target.
    pHitInfo->Reset(0, ~CHitInfo::kMaterialTrigger, 0);
    pHitInfo->m_bSkipObjects = 1;

    Vector vReflected = {
        vTargetPos.x - (vCamPos.x - vTargetPos.x),
        vTargetPos.y - (vCamPos.y - vTargetPos.y),
        vTargetPos.z + (vCamPos.z - vTargetPos.z)
    };

    if (!pScene->HitCheck(vReflected, vTargetPos, &vOutPos, pHitInfo)) {
        *pfYaw = m_pCamera->GetYaw() + 180.0f;
        *pfPitch = m_pCamera->GetPitch();
        return 1;
    }

    // Try a position rotated 90 degrees from the target.
    pHitInfo->Reset(0, ~CHitInfo::kMaterialTrigger, 0);
    pHitInfo->m_bSkipObjects = 1;

    Vector vRotated = {
        vTargetPos.x - (vCamPos.y - vTargetPos.y),
        vTargetPos.y + (vCamPos.x - vTargetPos.x),
        vTargetPos.z + (vCamPos.z - vTargetPos.z)
    };

    if (!pScene->HitCheck(vRotated, vTargetPos, &vOutPos, pHitInfo)) {
        *pfYaw = m_pCamera->GetYaw() - 90.0f;
        *pfPitch = m_pCamera->GetPitch();
        return 1;
    }

    // Finally the unreflected camera position again.
    pHitInfo->Reset(0, ~CHitInfo::kMaterialTrigger, 0);
    pHitInfo->m_bSkipObjects = 1;

    if (!pScene->HitCheck(vCamPos, vTargetPos, &vOutPos, pHitInfo)) {
        *pfYaw = m_pCamera->GetYaw() + 90.0f;
        *pfPitch = m_pCamera->GetPitch();
        return 1;
    }

    return 0;
}

void CSWCModule::UpdateCameraFollow(float fDeltaTime) {
    unsigned char nCameraMode;
    unsigned char nCurrentMode;
    CGameObject* pObj;
    CSWCObject* pSWCObj;
    Gob* pModel;
    CSWCCreature* pCreature;
    CSWCameraNavigate* pNavCam;
    CSWCameraFreeLook* pFreeCam;
    CSWCameraOnAStick* pStickCam;
    unsigned char nLastMode;
    CAurBehavior* pBehavior;

    nCameraMode = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode;
    if (nCameraMode == CClientOptions::CAMERA_MODE_DIALOG || m_bFindClearView || !m_pCamera)
        return;

    if (m_nCameraMode != nCameraMode)
        goto mode_change;

    // The mode is the same, so refresh the mode-change flag before the mode-specific update.
last_camera_mode_check:
    nLastMode = m_nLastCameraMode;
    nCurrentMode = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode;
    if (nLastMode != nCurrentMode)
        m_bCameraModeChanged = 1;

    // ON_A_STICK and DIALOG return early rather than falling through.
    switch (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode) {
    case CAMERA_MODE_CHASE_CAMERA:
        UpdateCameraModeChaseCam();
        break;
    case CAMERA_MODE_TOP_DOWN:
        UpdateCameraModeNormal();
        break;
    case CAMERA_MODE_STIFF_CHASE_CAMERA:
        UpdateCameraModeEQ(fDeltaTime);
        break;
    case CClientOptions::CAMERA_MODE_ON_A_STICK:
    case CClientOptions::CAMERA_MODE_DIALOG:
        return;
    }

    nCurrentMode = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode;
    if (nCurrentMode == CAMERA_MODE_CHASE_CAMERA) {
        pBehavior = m_pCamera->GetBehavior(BEHAVIOR_CURRENT);
        if (pBehavior)
            pBehavior->AsBehaviorCameraFollow();
    } else {
        nCurrentMode = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode;
        if (nCurrentMode == CAMERA_MODE_TOP_DOWN) {
            pBehavior = m_pCamera->GetBehavior(BEHAVIOR_CURRENT);
            if (pBehavior)
                pBehavior->AsBehaviorCameraFollow();
        }
    }

    m_fCameraDist = m_pCamera->GetDist();
    m_fCameraYaw = m_pCamera->GetYaw();
    m_fCameraPitch = m_pCamera->GetPitch();

    m_nLastCameraMode = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode;
    return;

mode_change:
    // The mode changed, so build the behavior for the new mode.
    pObj = g_pAppManager->m_pClientExoApp->GetGameObject(m_oidCameraFollow);
    if (!pObj) return;

    pSWCObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
    if (!pSWCObj) return;

    pModel = pSWCObj->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
    if (!pModel) return;

    // Leaving ON_A_STICK drops its behavior rather than setting a follow target.
    if (m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK)
        m_pCamera->SetBehavior(NULL);

    if (nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE) {
        pNavCam = new CSWCameraNavigate(m_pCamera->AsCAurObject());
        m_pCamera->SetBehavior(pNavCam);
    } else if (nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
        pCreature = static_cast<CSWCCreature*>(pObj->AsSWCCreature());
        pFreeCam = new CSWCameraFreeLook(m_pCamera->AsCAurObject(),
                                         static_cast<CAurObject*>(pCreature->GetCameraFollowPart()),
                                         pCreature->GetFreeLookVideoEffect());
        m_pCamera->SetBehavior(pFreeCam);
    } else if (nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
        pCreature = static_cast<CSWCCreature*>(pObj->AsSWCCreature());
        pStickCam = new CSWCameraOnAStick(m_pCamera->AsCAurObject(),
                                          static_cast<CAurObject*>(pCreature->GetCameraFollowPart()));
        pStickCam->m_nFollowingBody = pCreature->GetFollowingBody();
        pStickCam->m_fCameraOffset = pCreature->GetHeadHeightOffset() + pCreature->GetCameraOffset();
        m_pCamera->SetBehavior(pStickCam);
    } else {
        m_pCamera->SetFollow(static_cast<CAurObject*>(pModel));
    }

    m_nCameraMode = nCameraMode;
    goto last_camera_mode_check;
}

// Restores the camera to its previously saved state, called when leaving dialog, combat or
// animated-camera modes.
int CSWCModule::RestoreCamera() {
    if (g_pAppManager->m_pClientExoApp->GetCombatMode()) {
        SetCombatCamera();
        CSWBehaviorCameraCombat* pCombat = static_cast<CSWBehaviorCameraCombat*>(m_pCamera->GetBehavior(BEHAVIOR_CAMERA_COMBAT));
        if (pCombat) {
            pCombat->PutAtDesiredLocation();
        }
        return 1;
    }

    if (!m_cSavedCameraSettings.m_bValid) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCameraMode(CClientOptions::CAMERA_MODE_ON_A_STICK);
        return 0;
    }

    // The camera's own position and orientation, taken from the behavior's owner through the
    // CAurObject interface rather than from the camera's scalar yaw and pitch.
    Vector vYaw;
    Quaternion qPitch;
    CAurBehavior* pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior) {
        CAurBehavior* pBehavior2 = m_pCamera->GetBehavior(-1);
        vYaw = pBehavior2->m_pOwner->GetPosition();

        CAurBehavior* pBehavior3 = m_pCamera->GetBehavior(-1);
        qPitch = pBehavior3->m_pOwner->GetOrientation();
    } else {
        vYaw = Vector{0.0f, 0.0f, 0.0f};
        qPitch = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    }

    // Detect the currently installed behavior types.
    int bIsDialogOrFreeLook = 0;
    pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior) {
        CAurBehavior* pBehavior2 = m_pCamera->GetBehavior(-1);
        if (pBehavior2->GetType() == BEHAVIOR_CAMERA_DIALOG) {
            bIsDialogOrFreeLook = 1;
        }
    }

    pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior) {
        CAurBehavior* pBehavior2 = m_pCamera->GetBehavior(-1);
        if (pBehavior2->GetType() == BEHAVIOR_CAMERA_FREE_LOOK) {
            bIsDialogOrFreeLook = 1;
        }
    }

    int bIsCombat = 0;
    pBehavior = m_pCamera->GetBehavior(-1);
    if (pBehavior) {
        CAurBehavior* pBehavior2 = m_pCamera->GetBehavior(-1);
        if (pBehavior2->GetType() == BEHAVIOR_CAMERA_COMBAT) {
            bIsCombat = 1;
        }
    }

    // Only the on-a-stick, free-look and navigate modes restore.
    if (static_cast<unsigned int>(m_nCameraMode) > CClientOptions::CAMERA_MODE_NAVIGATE) return 0;
    if (!((1 << m_nCameraMode) & ((1 << CClientOptions::CAMERA_MODE_ON_A_STICK) |
                                  (1 << CClientOptions::CAMERA_MODE_FREE_LOOK) |
                                  (1 << CClientOptions::CAMERA_MODE_NAVIGATE)))) return 0;

    if (!m_cSavedCameraSettings.m_bValid) return 0;

    if (m_cSavedCameraSettings.m_pTarget.m_pObject) {
        if (m_cSavedCameraSettings.m_nCameraType == 1) {
            CSWCameraFreeLook* pFreeLook = new CSWCameraFreeLook(m_cSavedCameraSettings);
            m_pCamera->SetBehavior(pFreeLook);
            m_nCameraMode = CClientOptions::CAMERA_MODE_FREE_LOOK;
        } else if (m_cSavedCameraSettings.m_nCameraType == 0) {
            // For an on-a-stick camera, refresh the saved target from the party leader.
            CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
            if (pCreature) {
                m_cSavedCameraSettings.m_pTarget = static_cast<CAurObject*>(pCreature->GetCameraFollowPart());
                m_cSavedCameraSettings.m_nFollowingBody = pCreature->GetFollowingBody();
                m_cSavedCameraSettings.m_fTargetHeight =
                    pCreature->GetHeadHeightOffset() + pCreature->GetCameraOffset();
            }

            CSWCameraOnAStick* pOnAStick = new CSWCameraOnAStick(m_cSavedCameraSettings);
            if (bIsCombat) {
                pOnAStick->ResetLastPosition(vYaw, qPitch);
            }
            pOnAStick->m_bIsDialog = bIsDialogOrFreeLook;
            m_pCamera->SetBehavior(pOnAStick);
            m_nCameraMode = CClientOptions::CAMERA_MODE_ON_A_STICK;
        }
    } else {
        // With nothing saved, build a fresh behavior for the party leader.
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pCreature) {
            if (m_cSavedCameraSettings.m_nCameraType == 1) {
                CSWCameraFreeLook* pFreeLook = new CSWCameraFreeLook(
                    m_pCamera->AsCAurObject(), static_cast<CAurObject*>(pCreature->GetCameraFollowPart()),
                    pCreature->GetFreeLookVideoEffect());
                m_pCamera->SetBehavior(pFreeLook);
            } else if (m_cSavedCameraSettings.m_nCameraType == 0) {
                CSWCameraOnAStick* pOnAStick = new CSWCameraOnAStick(
                    m_pCamera->AsCAurObject(), static_cast<CAurObject*>(pCreature->GetCameraFollowPart()));
                pOnAStick->m_nFollowingBody = pCreature->GetFollowingBody();
                pOnAStick->m_fCameraOffset =
                    pCreature->GetHeadHeightOffset() + pCreature->GetCameraOffset();
                if (bIsCombat) {
                    pOnAStick->ResetLastPosition(vYaw, qPitch);
                }
                pOnAStick->m_bIsDialog = bIsDialogOrFreeLook;
                m_pCamera->SetBehavior(pOnAStick);
            }
        }
    }

    if (m_pAnimatedCameraModel) {
        delete m_pAnimatedCameraModel;
        m_pAnimatedCameraModel = 0;
    }

    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetCameraMode(static_cast<unsigned char>(m_nCameraMode));
    return 1;
}

void DialogCameraCallBack(CAurObject* pObj, char* pszExpectedAnim, void* pGuiObj) {
    K2_UNUSED(pObj);
    CGuiInGame* pGui = static_cast<CGuiInGame*>(pGuiObj);
    unsigned short nState = pGui->m_nCameraAnimation;
    CExoString sMsg;
    sMsg.Format(const_cast<char*>("done%s"), pGui->GetCameraAnimationName(nState).CStr());
    if (sMsg.CompareNoCase(pszExpectedAnim) != 0) {
        pGui->m_nCameraAnimation = CGuiInGame::CAMERA_ANIMATION_NONE;
    }
}
