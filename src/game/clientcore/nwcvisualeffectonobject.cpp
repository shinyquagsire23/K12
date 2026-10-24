#include "k2stdint.h"
#include "nwcvisualeffectonobject.h"

#include "appmanager.h"
#include "gameobjectarray.h"
#include "nwcobject.h"
#include "clientexoapp.h"
#include "AURORA/aurmodel.h"
#include "nwcanimbase.h"
#include "clientoptions.h"
#include "aurcamera.h"
#include "aurscene.h"
#include "nwccreature.h"
#include "nwccreatureappearance.h"
#include "nwscreature.h"
#include "nwcitem.h"
#include "nwcprojectile.h"
#include "nwcanimbaseprojectile.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "nwcplaceable.h"
#include "nwcdoor.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "aurorainterface.h"
#include "exosound.h"
#include "nwscriptconst.h"
#include "soundprioritygroup.h"
#include "include/aurbehavior.h"

unsigned short CSWCVisualEffectOnObject::m_nMotionBlurVFXCount;

#include "port/libcstring.h"
#include "port/libcmath.h"
#include "serverexoappinternal.h"

extern CAppManager* g_pAppManager;


// The three self-illumination vectors are cleared in the member-init list and again in the body.
CSWCVisualEffectOnObject::CSWCVisualEffectOnObject()
    : m_vSelfIllum()
    , m_vSelfIllumTarget()
    , m_vSelfIllumBlend()
{
    m_vTargetPosition = Vector();
    m_sSourceNode = "";
    m_gameObjectId = OBJECT_INVALID;
    m_sourceObjectId = OBJECT_INVALID;
    m_ownerObjectId = OBJECT_INVALID;
    for (int i = 0; i < 3; i++)
    {
        m_anSlotTimer[i] = 0;
        m_aSlotId[i] = STAGE_NONE;
        m_aReserved1[i] = 0;
        m_aReserved2[i] = 0;
        m_apSlotEffect[i] = 0;
        m_apReserved1[i] = 0;
    }
    m_vSelfIllum = Vector();
    m_vSelfIllumTarget = Vector();
    m_vSelfIllumBlend = Vector();
    m_fAlpha = 1.0f;
    m_pLight = 0;
    m_nEffectDuration0 = 0;
    m_nEffectDuration1 = 0;
    m_nEffectDuration2 = 0;
    m_nEffectTimer = 0;
    m_nEffectId0 = 0;
    m_nEffectId1 = 0;
    m_nEffectId2 = 0;
    m_bShown = 0;
    m_bEffect0Expired = 0;
    m_bEffect1Expired = 0;
    m_bEffect2Expired = 0;
    m_bExpiring = 0;
    m_bStopped = 0;
    m_bFinished = 0;
    m_effectId = 0;
    m_nState = STAGE_NONE;
    m_nPhase = 0;
    m_fFixedAlpha = 0.0f;
    m_bCastShadows = 0;
    m_nPriority = 0;
    m_hardCodedEffectClass = 0;
    m_bDarkOnly = 0;
    m_bAttached = 0;
    m_fSavedViewAngle = 0.0f;
    m_fSavedCameraDistance = 0.0f;
    m_pSoundSource = 0;
    m_bRumbleDone = 0;
    m_bHardCodedApplied = 0;
#if !KOTOR1
    m_bOnHeadModel = 0;
#endif
}

unsigned char CSWCVisualEffectOnObject::GetHardCodedEffectClass()
{
    return m_hardCodedEffectClass;
}

int CSWCVisualEffectOnObject::LoadAlpha(unsigned short animId)
{
    // Anim ids 400, 402 and 403 force half alpha.
    unsigned int rel = static_cast<unsigned int>(animId) - PROGFX_ALPHA;
    if (rel <= 3 && rel != 1)
        m_fFixedAlpha = 0.5f;
    return 1;
}

void CSWCVisualEffectOnObject::EnableHardCodedEffectShadow(int b)
{
    if (m_pLight != 0)
        m_bCastShadows = (b != 0);
}

int CSWCVisualEffectOnObject::GetHardCodedCessationLength(unsigned short nType)
{
    return (nType == PROGFX_NONE) ? 0 : 1000;
}

void CSWCVisualEffectOnObject::SetEffectDone(CAurObject* pEffect)
{
    if (pEffect == m_apSlotEffect[0])
        m_nEffectDuration0 = 3000;
    else if (pEffect == m_apSlotEffect[1])
        m_nEffectDuration1 = 3000;
    else if (pEffect == m_apSlotEffect[2])
        m_nEffectDuration2 = 3000;
}

void CSWCVisualEffectOnObject::RemoveSelfIllumination(unsigned short nId)
{
    K2_UNUSED(nId);
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (pClient == 0)
        return;
    pClient->UnHilite(3);
}

// Same body as RemoveSelfIllumination.
void CSWCVisualEffectOnObject::RemoveSelfIlluminationPulse(unsigned short nId)
{
    K2_UNUSED(nId);
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (pClient == 0)
        return;
    pClient->UnHilite(3);
}

void CSWCVisualEffectOnObject::RemoveBeam(unsigned short nId)
{
    K2_UNUSED(nId);
    if (m_pLight == 0)
        return;
    m_pLight->AttachToObject(0, 0, 0);
    m_pLight->AttachToScene(0);
}

void CSWCVisualEffectOnObject::RemoveAlpha(unsigned short nId)
{
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (pClient == 0)
        return;
    switch (nId)
    {
    case PROGFX_ALPHA_HALF:
    case PROGFX_ALPHA_FADE:
    case PROGFX_ALPHA_PULSE:
        break;
    case PROGFX_ALPHA_HALF_BLUE_TINT:
    case PROGFX_ALPHA_HALF_PURPLE_TINT:
        pClient->UnHilite(3);
        break;
    default:
        return;
    }
    pClient->SetAlpha(1.0f, CSWCObject::ANIM_MODEL_BODY);
}

void CSWCVisualEffectOnObject::RemoveMotionBlur()
{
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (pClient == 0)
        return;
    short nCount = m_nMotionBlurVFXCount - 1;
    if (nCount < 0)
        nCount = 0;
    m_nMotionBlurVFXCount = nCount;
    if (nCount > 0)
        return;
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    if (pMiniGame == 0)
        AurDisableSpeedBlur();
}

CSWCObject* CSWCVisualEffectOnObject::GetTargetCSWCObject()
{
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return 0;
    return pObject->AsSWCObject();
}

CSWCObject* CSWCVisualEffectOnObject::GetSourceCSWCObject()
{
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_sourceObjectId);
    if (pObject == 0)
        return 0;
    return pObject->AsSWCObject();
}

CSWCObject* CSWCVisualEffectOnObject::GetOwnerCSWCObject()
{
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_ownerObjectId);
    if (pObject == 0)
        return 0;
    return pObject->AsSWCObject();
}

// Anchors the beam between the source's and the target's root sub-model.
void CSWCVisualEffectOnObject::AttachBeam()
{
    CSWCObject* pTarget = GetTargetCSWCObject();
    CSWCObject* pSource = GetSourceCSWCObject();
    if (pTarget == 0)
        return;
    if (pSource == 0)
        return;
    if (m_pLight == 0)
        return;
    m_pLight->AttachToObject(static_cast<CAurObject*>(pSource->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                             m_sSourceNode.CStr(), 0);
    m_pLight->ReattachReference(static_cast<CAurObject*>(pTarget->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                                m_sTargetNode.CStr());
}

// Detaches the light, refreshing the area's shadowing lights first when this is the player's own
// effect and the m_bAttached gate is set.
void CSWCVisualEffectOnObject::RemoveLightSource(unsigned short nId)
{
    if (m_pLight == 0)
        return;
    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pTarget != 0 && m_bAttached != 0)
    {
        CSWCCreature* pCreature = pTarget->AsSWCCreature();
        if (pCreature != 0 && pCreature == g_pAppManager->m_pClientExoApp->GetPlayerCreature())
        {
            CSWCArea* pArea = pCreature->m_pArea;
            if (pArea != 0)
                pArea->UpdateShadowingLights();
        }
    }
    if (nId == PROGFX_LIGHT_PLAYER_BLIND_VISION
        && g_pAppManager->m_pClientExoApp->GetPlayerCreature() != 0
        && m_gameObjectId == g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_objectId)
    {
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule != 0 && pModule->m_pArea != 0)
            pModule->m_pArea->m_pScene->SetBlindLights(0);
    }
    m_pLight->AttachToObject(0, 0, 0);
    m_pLight->AttachToScene(0);
}

// Ends the view-angle animation and restores the camera's saved view angle.
void CSWCVisualEffectOnObject::RemoveFOV(unsigned short nId, int b)
{
    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pPlayerCreature == 0)
        return;
    if (pTarget == 0)
        return;
    if (b == 0 && pPlayerCreature->m_objectId != pTarget->m_objectId)
        return;
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    if (static_cast<unsigned short>(nId) != PROGFX_FOV_WIDE_ANGLE)
        return;
    if (pCamera == 0)
        return;
    pCamera->EndViewAngleAnimation(0);
    pCamera->SetViewAngle(m_fSavedViewAngle);
    CAurCamera* pCamera2 = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(pCamera2->GetBehavior(-1));
    if (pOnAStick == 0)
        return;
    if (pOnAStick->GetType() == BEHAVIOR_CAMERA_ON_A_STICK)
    {
        pOnAStick->m_fDist = m_fSavedCameraDistance;
        pOnAStick->m_bPreserveFraming = 0;
    }
    pOnAStick->GetType();
}

// Fades the camera's view angle in while the effect still has a usable target.
void CSWCVisualEffectOnObject::UpdateFOV(unsigned short nId, unsigned int nDelta)
{
    K2_UNUSED(nDelta);
    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pPlayerCreature == 0 || pTarget == 0
        || pTarget->m_objectId != pPlayerCreature->m_objectId
        || g_pAppManager->m_pClientExoApp->GetPausedByCombat() != 0)
    {
        RemoveFOV(nId, 1);
        return;
    }
    if (m_nPhase != 0)
        return;
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    if (pCamera == 0)
        return;
    if (m_nPhase != 0)
        return;
    if (m_nEffectTimer > 4000)
        return;
    if (nId == PROGFX_FOV_WIDE_ANGLE)
    {
        pCamera->EndViewAngleAnimation(0);
        pCamera->BeginViewAngleAnimation(m_fSavedViewAngle, static_cast<float>(m_nEffectTimer) / 1000.0f, 0);
    }
    m_nPhase = 1;
}

// Motion blur is kept only on the player's own creature, and only while that creature is not
// debilitated (stunned or paralysed); anything else drops the effect instead.
void CSWCVisualEffectOnObject::UpdateMotionBlur(unsigned int nDelta)
{
    K2_UNUSED(nDelta);
    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pPlayerCreature == 0)
        return;
    unsigned short nEffectId;
    switch (m_nState)
    {
    case STAGE_IMPACT:
        nEffectId = m_nEffectId0;
        break;
    case STAGE_DURATION:
        nEffectId = m_nEffectId1;
        break;
    case STAGE_CESSATION:
        nEffectId = m_nEffectId2;
        break;
#ifdef K2_FIX_ENGINE_BUGS
    default:
        return;
#endif
    }
    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pTarget != 0)
    {
        // Captured before the AsSWCCreature call and re-evaluated in the creature chain.
        bool bSame = (pTarget->m_objectId == pPlayerCreature->m_objectId);
        CSWCCreature* pCreature = pTarget->AsSWCCreature();
        if (bSame && pCreature != 0)
        {
            CSWSCreature* pServerCreature = pCreature->GetServerCreature();
            if (pServerCreature != 0 && pServerCreature->IsDebilitated(0)
                && pTarget->m_objectId == pPlayerCreature->m_objectId)
            {
                RemoveHardCodedVisualEffect(nEffectId);
                return;
            }
        }
        if (!bSame)
        {
            RemoveHardCodedVisualEffect(nEffectId);
            return;
        }
    }
    ApplyHardCodedVisualEffect(nEffectId);
    float fRatio = pPlayerCreature->m_swAnimationSpeed
                 / pPlayerCreature->m_pCreatureAppearance->m_fDriveMaxSpeed;
    fRatio = fRatio >= 1.0f ? 1.0f : fRatio;
    AurSetSpeedBlurRatio(fRatio * 0.75f);
}

int CSWCVisualEffectOnObject::LoadBeam(unsigned short nId)
{
    CExoString sModel;
    switch (nId)
    {
    case PROGFX_BEAM_LIGHTNING_DARK_S: sModel = "v_lightns_dur"; break;
    case PROGFX_BEAM_LIGHTNING_DARK_L: sModel = "v_lightnx_dur"; break;
    case PROGFX_BEAM_DROID_DISABLE: sModel = "v_drddisab_dur"; break;
    case PROGFX_BEAM_DROID_DESTROY: sModel = "v_drdkill_dur"; break;
    case PROGFX_BEAM_DEATH_FIELD: sModel = "v_deathfld_dur"; break;
    case PROGFX_BEAM_DRAIN_LIFE: sModel = "v_drain_dur"; break;
    case PROGFX_BEAM_FLAME_SPRAY: sModel = "v_flame_dur"; break;
    case PROGFX_BEAM_STUN_RAY: sModel = "v_stunray_dur"; break;
    case PROGFX_BEAM_COLD_RAY: sModel = "v_coldray_dur"; break;
    case PROGFX_BEAM_ION_RAY_01: sModel = "v_ionray01_dur"; break;
    case PROGFX_BEAM_ION_RAY_02: sModel = "v_ionray02_dur"; break;
    case PROGFX_BEAM_FORCE_STORM_ARC: sModel = "v_fstorm_dur"; break;
    case PROGFX_BEAM_DROID_STUN: sModel = "v_drdstun_dur"; break;
    case PROGFX_BEAM_SHOCK: sModel = "v_fshock_dur"; break;
#if !KOTOR1  // K1's beam band has two fewer ids
    case PROGFX_BEAM_POISON_SPRAY: sModel = "v_flamep_dur"; break;
    case PROGFX_BEAM_PLASMA_SPRAY: sModel = "v_flamepl_dur"; break;
#endif
    }
    m_pLight = NewCAurObject(sModel.CStr(), const_cast<char*>("fxbeam"), 0, 0);
    if (m_pLight == 0)
        return 0;
    m_pLight->SetLightPriority(3);
    m_pLight->DisableFog();
    return 1;
}

// Ticks the two fading alpha effects.
void CSWCVisualEffectOnObject::UpdateAlpha(unsigned int nDelta)
{
    CSWCObject* pClient = GetTargetCSWCObject();
    if (pClient == 0)
        return;
    if (m_nEffectId1 != PROGFX_ALPHA_PULSE && m_nEffectId1 != PROGFX_ALPHA_FADE)
        return;
    if (m_nEffectId1 == PROGFX_ALPHA_FADE)
    {
        if (m_nEffectTimer >= 6000)
        {
            unsigned int nPeriods = m_nEffectTimer / 6000;
            m_nPhase = (m_nPhase + nPeriods) & 3;
            m_nEffectTimer %= 6000;
        }
    }
    else
    {
        if (m_nEffectTimer >= 1000)
        {
            unsigned int nPeriods = m_nEffectTimer / 1000;
            unsigned char nState = (m_nPhase + nPeriods) & 3;
            m_nPhase = nState;
            m_nEffectTimer %= 1000;
            if (nState == 1)
                m_nPhase = 2;
            else if (nState == 3)
                m_nPhase = 0;
        }
    }
    switch (m_nPhase)
    {
    case 0:
        if (m_nEffectId1 == PROGFX_ALPHA_FADE)
        {
            m_fAlpha = 1.0f - static_cast<float>(m_nEffectTimer) / 6000.0f;
            if (m_fAlpha < 0.0f)
                m_fAlpha = 0.0f;
        }
        else
        {
            m_fAlpha = 1.0f - (static_cast<float>(m_nEffectTimer) / 1000.0f * 0.5f + 0.2f);
            if (m_fAlpha < 0.3f)
                m_fAlpha = 0.3f;
        }
        break;
    case 1:
        if (m_nEffectId1 == PROGFX_ALPHA_FADE)
            m_fAlpha = 0.0f;
        else
            m_fAlpha = 0.3f;
        break;
    case 2:
        if (m_nEffectId1 == PROGFX_ALPHA_FADE)
        {
            m_fAlpha = static_cast<float>(m_nEffectTimer) / 6000.0f;
            if (m_fAlpha > 1.0f)
                m_fAlpha = 1.0f;
        }
        else
        {
            m_fAlpha = static_cast<float>(m_nEffectTimer) / 1000.0f * 0.5f + 0.3f;
            if (m_fAlpha > 0.8f)
                m_fAlpha = 0.8f;
        }
        break;
    case 3:
        if (m_nEffectId1 == PROGFX_ALPHA_FADE)
            m_fAlpha = 1.0f;
        else
            m_fAlpha = 0.8f;
        break;
    }
    pClient->SetAlpha(m_fAlpha, CSWCObject::ANIM_MODEL_BODY);
    m_nEffectTimer += nDelta;
}

// Blends the self-illumination tint towards the pulse target, flipping direction once per period.
void CSWCVisualEffectOnObject::UpdateSelfIlluminationPulse(unsigned int nDelta)
{
    CSWCObject* pClient = GetTargetCSWCObject();
    if (pClient == 0)
        return;
    if (m_nEffectTimer > 1000)
    {
        m_nEffectTimer -= 1000;
        m_nPhase = !m_nPhase;
    }
    float fFade = static_cast<float>(m_nEffectTimer) / 1000.0f;
    if (m_nPhase)
        m_vSelfIllumBlend = m_vSelfIllum * fFade + m_vSelfIllumTarget * (1.0f - fFade);
    else
        m_vSelfIllumBlend = m_vSelfIllum * (1.0f - fFade) + m_vSelfIllumTarget * fFade;
    pClient->Hilite(m_vSelfIllumBlend, 3, 0.0f, 1);
    m_nEffectTimer += nDelta;
}

// Dispatches the per-frame tick to the handler that owns this effect id's band.
void CSWCVisualEffectOnObject::UpdateHardCodedVisualEffect(unsigned short nId, unsigned int nDelta)
{
    if (static_cast<unsigned short>(nId - PROGFX_ALPHA) < PROGFX_BAND_SIZE)
        UpdateAlpha(nDelta);
    else if (static_cast<unsigned short>(nId - PROGFX_SELF_ILLUMINATION_PULSE) < PROGFX_BAND_SIZE)
        UpdateSelfIlluminationPulse(nDelta);
    else if (static_cast<unsigned short>(nId - PROGFX_FOV) < PROGFX_BAND_SIZE)
        UpdateFOV(nId, nDelta);
    else if (static_cast<unsigned short>(nId - PROGFX_MOTION_BLUR) < PROGFX_BAND_SIZE)
        UpdateMotionBlur(static_cast<unsigned short>(nId - PROGFX_MOTION_BLUR));
}

void CSWCVisualEffectOnObject::RemoveEnvironmentMap(unsigned short nId)
{
    K2_UNUSED(nId);
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (pClient == 0)
        return;
    pClient->RestoreEnvMap();
    pClient->RestoreTexture();
}

void CSWCVisualEffectOnObject::RemoveDistortion(unsigned short nId)
{
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (nId == PROGFX_DISTORTION && pClient != 0)
        pClient->DisableDistortion();
}

// Brings the effect back into the scene: re-attaches every slot to a joint of the target's current
// model, re-attaches the light slot and re-applies the state machine's hard-coded effect. The joint
// suffix depends on the target: a placeable appends "_ground"/"_impact"/"_head_hit", a door
// "grnd"/"impact"/"hhit", and anything else uses the plain "root"/"impact"/"talkdummy".
int CSWCVisualEffectOnObject::ShowVisualEffect(int a)
{
    if (a == 0 && m_bShown != 0)
        return 1;

    CGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    if (pArray == 0)
        return 0;

    CGameObject* pObject;
    if (pArray->GetGameObject(m_gameObjectId, &pObject) != CGameObjectArray::SUCCESS)
        return 0;

    CSWCObject* pTarget = pObject->AsSWCObject();
    Gob* pModel = pTarget->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);

    CExoString sPart;
    CSWCPlaceable* pPlaceable;
    CSWCDoor* pDoor;
    CSWCPlaceable* pPlaceable2;

    pPlaceable = pTarget->AsSWCPlaceable();
    pDoor = pTarget->AsSWCDoor();

    if (m_apSlotEffect[0] != 0)
    {
        if (pPlaceable != 0)
        {
            sPart = pPlaceable->m_sModelName + CExoString("_ground");
        }
        else if (pDoor != 0)
        {
            sPart = pDoor->m_sModelName + CExoString("grnd");
        }
        else
            sPart = "root";
        AttachVFXModelToTargetModel(m_apSlotEffect[0], static_cast<CAurObject*>(pModel), sPart);
    }

    if (m_apSlotEffect[1] != 0)
    {
        if (pPlaceable != 0)
        {
            sPart = pPlaceable->m_sModelName + CExoString("_impact");
        }
        else if (pDoor != 0)
        {
            sPart = pDoor->m_sModelName + CExoString("impc");   // "impc" is the game data's own spelling here, not "impact"
        }
        else
            sPart = "impact";
        AttachVFXModelToTargetModel(m_apSlotEffect[1], static_cast<CAurObject*>(pModel), sPart);
    }

    if (m_apSlotEffect[2] != 0)
    {
        Gob* pSlotModel = pModel;
        if (pPlaceable != 0)
        {
            sPart = pPlaceable->m_sModelName + CExoString("_head_hit");
        }
        else if (pDoor != 0)
        {
            sPart = pDoor->m_sModelName + CExoString("hhit");
        }
        else
            sPart = "talkdummy";

        // The special sub-model stands in for the target's root model only while the animation base
        // sits in mode 3.
        if (pTarget->GetAnimationBase() != 0
            && pTarget->GetAnimationBase()->m_nAnimBaseType == 3)
            pSlotModel = pTarget->GetModel(CSWCObject::ANIM_MODEL_HEAD, 1);
        AttachVFXModelToTargetModel(m_apSlotEffect[2], static_cast<CAurObject*>(pSlotModel), sPart);
    }

    if (m_pLight != 0)
    {
        pPlaceable2 = pTarget->AsSWCPlaceable();
        if (pPlaceable2 != 0)
        {
            sPart = pPlaceable2->m_sModelName + CExoString("_ground");
        }
        else if (pDoor != 0)
        {
            sPart = pDoor->m_sModelName + CExoString("grnd");
        }
        else
            sPart = "root";
        m_pLight->AttachToObject(static_cast<CAurObject*>(pModel), sPart.CStr(), 0);
    }

    m_bShown = 1;
    m_bAttached = 1;
    switch (m_nState)
    {
    case STAGE_IMPACT:
        if (m_nEffectId0 != PROGFX_NONE)
            ApplyHardCodedVisualEffect(m_nEffectId0);
        break;
    case STAGE_DURATION:
        if (m_nEffectId1 != PROGFX_NONE)
            ApplyHardCodedVisualEffect(m_nEffectId1);
        break;
    case STAGE_CESSATION:
        if (m_nEffectId2 != PROGFX_NONE)
            ApplyHardCodedVisualEffect(m_nEffectId2);
        break;
    }
    return 1;
}

// Re-hangs the effect's models on the target's current model and re-applies the state machine's
// hard-coded effect, run after the target's model has been rebuilt. The joint vocabulary matches
// ShowVisualEffect's; the light slot's joint name comes from the cached m_sTargetNode, and its no-handler
// case falls through into the band chain below rather than returning.
void CSWCVisualEffectOnObject::ReAttachVisualEffect()
{
    CExoString sPart;
    CSWCObject* pTarget;
    Gob* pModel;
    CSWCPlaceable* pPlaceable;
    CSWCDoor* pDoor;
    Gob* pSlotModel;
    CSWCObject* pDistortTarget;

    pTarget = GetTargetCSWCObject();
    if (pTarget == 0)
        return;

#if KOTOR1  // K1 has no mask hook: always the root sub-model
    pModel = pTarget->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
#else
    if (m_bOnHeadModel != 0)
        pModel = pTarget->GetModel(CSWCObject::ANIM_MODEL_HEAD, 1);
    else
        pModel = pTarget->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
#endif
    if (pModel == 0)
        return;

    m_bAttached = 1;

    pPlaceable = pTarget->AsSWCPlaceable();
    pDoor = pTarget->AsSWCDoor();

    if (m_apSlotEffect[0] != 0)
    {
        if (pPlaceable != 0)
            sPart = pPlaceable->m_sModelName + CExoString("_ground");
        else if (pDoor != 0)
            sPart = pDoor->m_sModelName + CExoString("grnd");
        else
            sPart = "root";
        AttachVFXModelToTargetModel(m_apSlotEffect[0], static_cast<CAurObject*>(pModel), sPart);
    }

    if (m_apSlotEffect[1] != 0)
    {
        if (pPlaceable != 0)
            sPart = pPlaceable->m_sModelName + CExoString("_impact");
        else if (pDoor != 0)
            sPart = pDoor->m_sModelName + CExoString("impc");   // "impc" is the game data's own spelling here, not "impact"
        else
            sPart = "impact";
        AttachVFXModelToTargetModel(m_apSlotEffect[1], static_cast<CAurObject*>(pModel), sPart);
    }

    if (m_apSlotEffect[2] != 0)
    {
        pSlotModel = pModel;
        if (pPlaceable != 0)
            sPart = pPlaceable->m_sModelName + CExoString("_head_hit");
        else if (pDoor != 0)
            sPart = pDoor->m_sModelName + CExoString("hhit");
        else
            sPart = "talkdummy";

        // The special sub-model stands in for the base model only while that base sits in mode 3.
        if (pTarget->GetAnimationBase() != 0
            && pTarget->GetAnimationBase()->m_nAnimBaseType == 3)
            pSlotModel = pTarget->GetModel(CSWCObject::ANIM_MODEL_HEAD, 1);
        AttachVFXModelToTargetModel(m_apSlotEffect[2], static_cast<CAurObject*>(pSlotModel), sPart);
    }

    unsigned short nId;
    switch (m_nState)
    {
    case STAGE_IMPACT:
        nId = m_nEffectId0;
        break;
    case STAGE_DURATION:
        nId = m_nEffectId1;
        break;
    case STAGE_CESSATION:
        nId = m_nEffectId2;
        break;
    default:
        nId = PROGFX_NONE;
        break;
    }

    if (m_pLight != 0)
    {
        if (nId >= PROGFX_BEAM && nId < PROGFX_BEAM + PROGFX_BAND_SIZE)
        {
            AttachBeam();
            return;
        }
        if (nId >= PROGFX_LIGHT_SOURCE && nId < PROGFX_LIGHT_SOURCE + PROGFX_BAND_SIZE)
        {
            AttachLight();
            return;
        }
        if (nId >= PROGFX_SPELL_VISUAL && nId < PROGFX_SPELL_VISUAL + PROGFX_BAND_SIZE)
        {
            ApplySpellVisual(nId);
            return;
        }
        // No return here: the no-handler case falls through into the band chain below.
        m_pLight->AttachToObject(static_cast<CAurObject*>(pModel), m_sTargetNode.CStr(), 0);
    }

    if (nId >= PROGFX_ENVIRONMENT_MAP && nId < PROGFX_ENVIRONMENT_MAP + PROGFX_BAND_SIZE)
        ApplyEnvironmentMap(nId);
    else if (nId >= PROGFX_SELF_ILLUMINATION && nId < PROGFX_SELF_ILLUMINATION + PROGFX_BAND_SIZE)
        ApplySelfIllumination(nId);
    else if (nId >= PROGFX_ALPHA && nId < PROGFX_ALPHA + PROGFX_BAND_SIZE)
    {
        if (nId == PROGFX_ALPHA_PULSE || nId == PROGFX_ALPHA_FADE)
            pTarget->SetAlpha(m_fAlpha, CSWCObject::ANIM_MODEL_BODY);
        else
            ApplyAlpha(nId);
    }
    else if (nId >= PROGFX_SELF_ILLUMINATION_PULSE && nId < PROGFX_SELF_ILLUMINATION_PULSE + PROGFX_BAND_SIZE)
        pTarget->Hilite(m_vSelfIllumBlend, 3, 0.0f, 1);
    else if (nId >= PROGFX_TEXTURE_REPLACE && nId < PROGFX_TEXTURE_REPLACE + PROGFX_BAND_SIZE)
        ApplyTextureReplace(nId);
    else if (nId >= PROGFX_BUMPED_OUT_SHELL && nId < PROGFX_BUMPED_OUT_SHELL + PROGFX_BAND_SIZE)
        ApplyBumpedOutShell(nId);
    else if (nId >= PROGFX_DISTORTION && nId < PROGFX_DISTORTION + PROGFX_BAND_SIZE)
    {
        pDistortTarget = GetTargetCSWCObject();
        if (nId == PROGFX_DISTORTION && pDistortTarget != 0)
            pDistortTarget->EnableDistortion();
    }
}

// Asks each live slot effect to play the "cessation" animation and turns its remaining length into
// an expiry countdown (the timer is the animation's length in milliseconds, and the duration field
// gets that plus 3000). The tail re-derives the state machine for the third slot, re-applies its
// hard-coded effect, plays the visualeffects 2DA's SoundCessation one-shot and drops the sound
// source. All three slots stamp the same m_nEffectDuration0 and m_bEffect0Expired.
void CSWCVisualEffectOnObject::StopVisualEffect(int a)
{
    if (m_bStopped != 0)
    {
        m_bFinished = a;
        return;
    }
    m_bStopped = 1;
    m_bFinished = a;

    int bPlaying = 0;
    m_anSlotTimer[0] = 0;
    if (m_apSlotEffect[0] != 0)
    {
        float fUnused;
        float fLength;
        m_apSlotEffect[0]->GetAnimation(const_cast<char*>("cessation"), &fUnused, &fLength);
        if (m_apSlotEffect[0]->PlayAnimation(const_cast<char*>("cessation"), 1.0f, 1, 0.0f))
        {
            m_aSlotId[0] = STAGE_CESSATION;
            m_anSlotTimer[0] = static_cast<unsigned int>(fLength * 1000.0f);
            m_nEffectDuration0 = m_anSlotTimer[0] + 3000;
            bPlaying = 1;
        }
        else
        {
            m_bEffect0Expired = 1;
            bPlaying = 0;
        }
    }

    m_anSlotTimer[1] = 0;
    if (m_apSlotEffect[1] != 0)
    {
        float fUnused;
        float fLength;
        m_apSlotEffect[1]->GetAnimation(const_cast<char*>("cessation"), &fUnused, &fLength);
        if (m_apSlotEffect[1]->PlayAnimation(const_cast<char*>("cessation"), 1.0f, 1, 0.0f))
        {
            m_aSlotId[1] = STAGE_CESSATION;
            m_anSlotTimer[1] = static_cast<unsigned int>(fLength * 1000.0f);
            m_nEffectDuration0 = m_anSlotTimer[1] + 3000;
            bPlaying = 1;
        }
        else
            m_bEffect0Expired = 1;
    }

    m_anSlotTimer[2] = 0;
    if (m_apSlotEffect[2] != 0)
    {
        float fUnused;
        float fLength;
        m_apSlotEffect[2]->GetAnimation(const_cast<char*>("cessation"), &fUnused, &fLength);
        if (m_apSlotEffect[2]->PlayAnimation(const_cast<char*>("cessation"), 1.0f, 1, 0.0f))
        {
            m_aSlotId[2] = STAGE_CESSATION;
            m_anSlotTimer[2] = static_cast<unsigned int>(fLength * 1000.0f);
            m_nEffectDuration0 = m_anSlotTimer[2] + 3000;
            bPlaying = 1;
        }
        else
            m_bEffect0Expired = 1;
    }

    if (a != 0 && !bPlaying)
        m_bExpiring = 1;

    if (m_nState == STAGE_IMPACT)
    {
        RemoveHardCodedVisualEffect(m_nEffectId0);
        UnloadHardCodedVisualEffect();
    }
    else if (m_nState == STAGE_DURATION)
    {
        if (!(static_cast<unsigned short>(m_nEffectId1 - PROGFX_LIGHT_SOURCE) < PROGFX_BAND_SIZE
              && static_cast<unsigned int>(m_nEffectId2 - PROGFX_LIGHT_SOURCE) < PROGFX_BAND_SIZE))
        {
            RemoveHardCodedVisualEffect(m_nEffectId1);
            UnloadHardCodedVisualEffect();
        }

        // The IMPACT arm jumps past this re-derivation, so only DURATION falls through to it.
        if (m_nEffectId2 == PROGFX_NONE)
            m_nState = STAGE_NONE;
        else
        {
            m_nState = STAGE_CESSATION;
            m_hardCodedEffectClass = GetHardCodedEffectClass(m_nEffectId2);
            m_nEffectTimer = 1000;
        }
    }

    if (m_nEffectId2 != PROGFX_NONE)
    {
        if (!(static_cast<unsigned short>(m_nEffectId2 - PROGFX_LIGHT_SOURCE) < PROGFX_BAND_SIZE
              && static_cast<unsigned short>(m_nEffectId1 - PROGFX_LIGHT_SOURCE) < PROGFX_BAND_SIZE))
            LoadHardCodedVisualEffect(m_nEffectId2);
        ApplyHardCodedVisualEffect(m_nEffectId2);
    }

    CExoString sSound;
    CSWCObject* pTarget = GetTargetCSWCObject();
    CExoString sId(m_effectId);
    if (g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
            sId, CExoString("SoundCessation"), &sSound) == 0)
        sSound = "";

#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (pTarget != 0 && sSound.GetLength() > 0)
#else
    if (pTarget != 0 && sSound.m_sString != 0 && static_cast<int>(strlen(sSound.m_sString)) > 0)
#endif
        g_pExoSound->Play3DOneShotSound(CResRef(sSound), pTarget->m_position, 1.5f,
                                        PRIORITY_GROUP_NORMAL_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);

    if (m_pSoundSource != 0)
    {
        m_pSoundSource->Stop();
        delete m_pSoundSource;
        m_pSoundSource = 0;
    }
    if (m_bAttached == 0)
        DetachVisualEffect();
}

// Tears down what ApplyHardCodedVisualEffect creates, over the same 100-wide bands, but only the
// bands that own persistent state have a Remove arm -- the one-shot effects (delete-model, chunk,
// mirv, cheat, spell visual, lightsaber throw) just clear the applied latch. The latch
// (m_bHardCodedApplied) is what makes the pair idempotent.
void CSWCVisualEffectOnObject::RemoveHardCodedVisualEffect(unsigned short nId)
{
    if (m_bShown == 0)
        return;
    if (m_bHardCodedApplied == 0)
        return;

    if (static_cast<unsigned short>(nId - PROGFX_ENVIRONMENT_MAP) < PROGFX_BAND_SIZE)
        RemoveEnvironmentMap(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_SELF_ILLUMINATION) < PROGFX_BAND_SIZE)
        RemoveSelfIllumination(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_LIGHT_SOURCE) < PROGFX_BAND_SIZE)
        RemoveLightSource(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_ALPHA) < PROGFX_BAND_SIZE)
        RemoveAlpha(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_SELF_ILLUMINATION_PULSE) < PROGFX_BAND_SIZE)
        RemoveSelfIlluminationPulse(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_BEAM) < PROGFX_BAND_SIZE)
        RemoveBeam(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_TEXTURE_REPLACE) < PROGFX_BAND_SIZE)
        RemoveTextureReplace(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_BUMPED_OUT_SHELL) < PROGFX_BAND_SIZE)
        RemoveBumpedOutShell();
    else if (static_cast<unsigned short>(nId - PROGFX_FOV) < PROGFX_BAND_SIZE)
        RemoveFOV(nId, 0);
    else if (static_cast<unsigned short>(nId - PROGFX_MOTION_BLUR) < PROGFX_BAND_SIZE)
        RemoveMotionBlur();
#if !KOTOR1  // K1 has no model-attachment removal
    else if (static_cast<unsigned short>(nId - PROGFX_MODEL_ATTACHMENT) < PROGFX_BAND_SIZE)
        RemoveModelAttachment(nId);
#endif
    else if (static_cast<unsigned short>(nId - PROGFX_DISTORTION) < PROGFX_BAND_SIZE)
        RemoveDistortion(nId);

    m_bHardCodedApplied = 0;
}

// Creates a hard-coded effect through one handler per 100-wide id band, tested as an ascending
// ladder of `nId < band-end` comparisons rather than the per-band subtract-and-mask form the
// Remove* and Update* twins use. The applied latch (m_bHardCodedApplied) gates it symmetrically
// with RemoveHardCodedVisualEffect.
void CSWCVisualEffectOnObject::ApplyHardCodedVisualEffect(unsigned short nId)
{
    if (m_bShown == 0)
        return;
    if (m_bHardCodedApplied != 0)
        return;

    if (nId < PROGFX_SELF_ILLUMINATION)
        ApplyEnvironmentMap(nId);
    else if (nId < PROGFX_LIGHT_SOURCE)
        ApplySelfIllumination(nId);
    else if (nId < PROGFX_ALPHA)
        ApplyLightSource(nId);
    else if (nId < PROGFX_SELF_ILLUMINATION_PULSE)
        ApplyAlpha(nId);
    else if (nId < PROGFX_BEAM)
    {
        if (nId == PROGFX_SELF_ILLUMINATION_PULSE_RED)
        {
            m_vSelfIllum = Vector(1.0f, 0.0f, 0.0f);
            m_vSelfIllumTarget = Vector(1.0f, 1.0f, 1.0f);
        }
        else if (nId == PROGFX_SELF_ILLUMINATION_PULSE_BLUE)
        {
            m_vSelfIllum = Vector(0.0f, 0.0f, 1.0f);
            m_vSelfIllumTarget = Vector(1.0f, 1.0f, 1.0f);
        }
    }
    else if (nId < PROGFX_DELETE_MODEL)
    {
        AttachBeam();
        m_pLight->PlayAnimation(const_cast<char*>("cast01"), 1.0f, 0, 0.0f);
    }
    else if (nId < PROGFX_CHUNK)
        ApplyDeleteModel(nId);
    else if (nId < PROGFX_MIRV)
        ApplyChunk(nId);
    else if (nId < PROGFX_TEXTURE_REPLACE)
        ApplyMirv(nId);
    else if (nId < PROGFX_CHEAT)
        ApplyTextureReplace(nId);
    else if (nId < PROGFX_SPELL_VISUAL)
        ApplyCheat(nId);
    else if (nId < PROGFX_LIGHTSABER_THROW)
        ApplySpellVisual(nId);
    else if (nId < PROGFX_BUMPED_OUT_SHELL)
        ApplyLightsaberThrow(nId);
    else if (nId < PROGFX_FOV)
        ApplyBumpedOutShell(nId);
    else if (nId < PROGFX_MOTION_BLUR)
        ApplyFOV(nId);
    else if (nId < PROGFX_MODEL_ATTACHMENT)
        ApplyMotionBlur(nId);
    else if (nId < PROGFX_DISTORTION)
        ApplyModelAttachment(nId);
    else if (nId < PROGFX_DISTORTION + PROGFX_BAND_SIZE)
        ApplyDistortion(nId);

    m_bHardCodedApplied = 1;
}

// Detaches every visual object the effect owns without destroying any of them: the three slot
// effects explicitly (not in a loop), then the light slot.
void CSWCVisualEffectOnObject::DetachVisualEffect()
{
    m_bAttached = 0;

    if (m_apSlotEffect[0] != 0)
    {
        m_apSlotEffect[0]->AttachToObject(0, 0, 0);
        m_apSlotEffect[0]->AttachToScene(0);
    }
    if (m_apSlotEffect[1] != 0)
    {
        m_apSlotEffect[1]->AttachToObject(0, 0, 0);
        m_apSlotEffect[1]->AttachToScene(0);
    }
    if (m_apSlotEffect[2] != 0)
    {
        m_apSlotEffect[2]->AttachToObject(0, 0, 0);
        m_apSlotEffect[2]->AttachToScene(0);
    }

    if (m_pLight != 0)
    {
        if (m_hardCodedEffectClass == HARDCODED_CLASS_LIGHT_SOURCE)
        {
            CSWCObject* pTarget = GetTargetCSWCObject();
            if (pTarget != 0)
            {
                CSWCCreature* pCreature = pTarget->AsSWCCreature();
                if (pCreature != 0
                    && pCreature == g_pAppManager->m_pClientExoApp->GetPlayerCreature())
                {
                    CSWCArea* pArea = pCreature->m_pArea;
                    if (pArea != 0)
                        pArea->UpdateShadowingLights();
                }
            }
        }
        m_pLight->AttachToObject(0, 0, 0);
        m_pLight->AttachToScene(0);
    }
}

void CSWCVisualEffectOnObject::ApplyBeam(unsigned short nId)
{
    K2_UNUSED(nId);
    AttachBeam();
    m_pLight->PlayAnimation(const_cast<char*>("cast01"), 1.0f, 0, 0.0f);
}

void CSWCVisualEffectOnObject::ApplySelfIlluminationPulse(unsigned short nId)
{
    if (nId == PROGFX_SELF_ILLUMINATION_PULSE_RED)
    {
        m_vSelfIllum = Vector(1.0f, 0.0f, 0.0f);
        m_vSelfIllumTarget = Vector(1.0f, 1.0f, 1.0f);
    }
    else if (nId == PROGFX_SELF_ILLUMINATION_PULSE_BLUE)
    {
        m_vSelfIllum = Vector(0.0f, 0.0f, 1.0f);
        m_vSelfIllumTarget = Vector(1.0f, 1.0f, 1.0f);
    }
}

void CSWCVisualEffectOnObject::ApplyDeleteModel(unsigned short nId)
{
    K2_UNUSED(nId);
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nViolenceLevel < 2)
        return;
    CSWCObject* pClient = GetTargetCSWCObject();
    if (pClient == 0)
        return;
    CSWCCreature* pCreature = pClient->AsSWCCreature();
    if (pCreature != 0)
        pCreature->KillPopUpBubble();
    pClient->GetAnimationBase()->UnloadModel();
}

// Retires the effect the state machine currently names and drops the sound source.
void CSWCVisualEffectOnObject::KillVisualEffect()
{
    switch (m_nState)
    {
    case STAGE_IMPACT:
        if (m_nEffectId0 != PROGFX_NONE)
            RemoveHardCodedVisualEffect(m_nEffectId0);
        break;
    case STAGE_DURATION:
        if (m_nEffectId1 != PROGFX_NONE)
            RemoveHardCodedVisualEffect(m_nEffectId1);
        break;
    case STAGE_CESSATION:
        if (m_nEffectId2 != PROGFX_NONE)
            RemoveHardCodedVisualEffect(m_nEffectId2);
        break;
    }

    UnloadVisualEffect();
    m_nState = STAGE_NONE;
    m_hardCodedEffectClass = 0;
    if (m_pSoundSource != 0)
    {
        m_pSoundSource->Stop();
        delete m_pSoundSource;
        m_pSoundSource = 0;
    }
}

void CSWCVisualEffectOnObject::ApplyMotionBlur(unsigned short nId)
{
    CSWCObject* pClient = GetTargetCSWCObject();
    if (pClient == 0)
        return;
    unsigned short nType = nId - PROGFX_MOTION_BLUR;
    switch (nType)
    {
    case 1:
    case 2:
        break;
    default:
        return;
    }
    m_nMotionBlurVFXCount++;
    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pPlayerCreature != 0 && pPlayerCreature->m_objectId != pClient->m_objectId)
        return;
    if (m_nMotionBlurVFXCount != 1)
        return;
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    if (pMiniGame == 0)
    {
        AurSetSpeedBlurRatio(0.75f);
        AurEnableSpeedBlur();
    }
}

void CSWCVisualEffectOnObject::ApplySelfIllumination(unsigned short nId)
{
    Vector vColor;
    CSWCObject* pClient = GetTargetCSWCObject();
    if (pClient == 0)
        return;
    m_nPriority = 20;
    unsigned short nRel = nId - PROGFX_SELF_ILLUMINATION;
    switch (nRel)
    {
    case 0: vColor = Vector(1.0f, 0.0f, 0.0f); break;
    case 1: vColor = Vector(0.0f, 1.0f, 0.0f); break;
    case 2: vColor = Vector(0.0f, 0.0f, 1.0f); break;
    case 3: vColor = Vector(1.0f, 0.0f, 1.0f); break;
    case 4: vColor = Vector(1.0f, 1.0f, 0.0f); break;
    case 5: vColor = Vector(1.0f, 1.0f, 1.0f); break;
    default: return;
    }
    pClient->Hilite(vColor, 3, 0.0f, 1);
}

// Builds the "vdu_envmap<NNN>" replacement texture name from the effect id.
void CSWCVisualEffectOnObject::ApplyEnvironmentMap(unsigned short nId)
{
    CExoString sTexName;
    CSWCObject* pClient = GetTargetCSWCObject();
    if (pClient == 0)
        return;
    sTexName.Format(const_cast<char*>("vdu_envmap%03u"), static_cast<unsigned short>(nId - PROGFX_ENVIRONMENT_MAP));
    pClient->ReplaceTexture(CResRef(""), CResRef(""), 0);
    pClient->SetEnvMap(1, 0, CResRef(sTexName));
}

// Starts the view-angle animation, saving the camera's live view angle so RemoveFOV can restore it.
void CSWCVisualEffectOnObject::ApplyFOV(unsigned short nId)
{
    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pPlayerCreature == 0)
        return;
    if (pTarget == 0)
        return;
    if (pPlayerCreature->m_objectId != pTarget->m_objectId)
        return;
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    if (static_cast<unsigned short>(nId) != PROGFX_FOV_WIDE_ANGLE)
        return;
    if (pCamera == 0)
        return;
    m_fSavedViewAngle = pCamera->GetViewAngle();
    CAurCamera* pCamera2 = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(pCamera2->GetBehavior(-1));
    if (pOnAStick != 0)
    {
        if (pOnAStick->GetType() == BEHAVIOR_CAMERA_ON_A_STICK)
        {
            m_fSavedCameraDistance = pOnAStick->m_fDist;
            pOnAStick->m_bPreserveFraming = 1;
        }
        pOnAStick->GetType();
    }
    pCamera->BeginViewAngleAnimation(135.0f, 0.75f, 0);
    m_nPhase = 0;
    m_nEffectTimer = 4750;
}

// The case arms run {1,4} first, then 0, 3 and 2.
void CSWCVisualEffectOnObject::ApplyAlpha(unsigned short nId)
{
    CSWCObject* pClient = GetTargetCSWCObject();
    if (pClient == 0)
        return;
    m_nPriority = 1;
    switch (static_cast<unsigned short>(nId - PROGFX_ALPHA))
    {
    case 1:
    case 4:
        m_nPhase = 0;
        break;
    case 0:
        pClient->SetAlpha(m_fFixedAlpha, CSWCObject::ANIM_MODEL_BODY);
        m_fAlpha = m_fFixedAlpha;
        break;
    case 3:
        pClient->SetAlpha(m_fFixedAlpha, CSWCObject::ANIM_MODEL_BODY);
        m_fAlpha = m_fFixedAlpha;
        pClient->Hilite(Vector(0.5f, 0.0f, 1.0f), 3, 0.0f, 1);
        break;
    case 2:
        pClient->SetAlpha(m_fFixedAlpha, CSWCObject::ANIM_MODEL_BODY);
        m_fAlpha = m_fFixedAlpha;
        pClient->Hilite(Vector(0.25f, 0.25f, 1.0f), 3, 0.0f, 1);
        break;
    }
}

void CSWCVisualEffectOnObject::HideVisualEffect()
{
    if (m_bShown == 0)
        return;

    if (m_apSlotEffect[0] != 0)
    {
        m_apSlotEffect[0]->AttachToObject(0, 0, 0);
        m_apSlotEffect[0]->AttachToScene(0);
    }
    if (m_apSlotEffect[1] != 0)
    {
        m_apSlotEffect[1]->AttachToObject(0, 0, 0);
        m_apSlotEffect[1]->AttachToScene(0);
    }
    if (m_apSlotEffect[2] != 0)
    {
        m_apSlotEffect[2]->AttachToObject(0, 0, 0);
        m_apSlotEffect[2]->AttachToScene(0);
    }
    if (m_pLight != 0)
    {
        m_pLight->AttachToObject(0, 0, 0);
        m_pLight->AttachToScene(0);
    }

    switch (m_nState)
    {
    case STAGE_IMPACT:
        if (m_nEffectId0 != PROGFX_NONE)
            RemoveHardCodedVisualEffect(m_nEffectId0);
        break;
    case STAGE_DURATION:
        if (m_nEffectId1 != PROGFX_NONE)
            RemoveHardCodedVisualEffect(m_nEffectId1);
        break;
    case STAGE_CESSATION:
        if (m_nEffectId2 != PROGFX_NONE)
            RemoveHardCodedVisualEffect(m_nEffectId2);
        break;
    }

    m_bShown = 0;
    m_bAttached = 0;
}

// Hangs the attachment on the hook the id names. The mask id hangs off the model's own special
// sub-model, falls back to the root sub-model when there is none, and is the only id that plays the
// "impact" animation.
void CSWCVisualEffectOnObject::ApplyModelAttachment(unsigned short nId)
{
#if KOTOR1  // K1 has three hooks (no "maskhook"), the root sub-model only, no "impact" animation
    unsigned short nType = nId - PROGFX_MODEL_ATTACHMENT;
    switch (nType)
    {
    case 0: m_sTargetNode = "medalhook"; break;
    case 1: m_sTargetNode = "revmask1hook"; break;
    case 2: m_sTargetNode = "revmask2hook"; break;
    }
    if (m_pLight == 0)
        return;
    m_pLight->AttachToObject(static_cast<CAurObject*>(GetTargetCSWCObject()->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                             m_sTargetNode.CStr(), 0);
#else
    unsigned short nType = nId - PROGFX_MODEL_ATTACHMENT;
    switch (nType)
    {
    case 0: m_sTargetNode = "medalhook"; break;
    case 1: m_sTargetNode = "revmask1hook"; break;
    case 2: m_sTargetNode = "revmask2hook"; break;
    case 3: m_sTargetNode = "maskhook"; break;
    }
    if (m_pLight == 0)
        return;

    CAurObject* pModel;
    if (nType == 3)
    {
        pModel = static_cast<CAurObject*>(GetTargetCSWCObject()->GetModel(CSWCObject::ANIM_MODEL_HEAD, 1));
        if (pModel == 0)
            pModel = static_cast<CAurObject*>(GetTargetCSWCObject()->GetModel(CSWCObject::ANIM_MODEL_BODY, 1));
        else
            m_bOnHeadModel = 1;
    }
    else
    {
        m_pLight->AttachToObject(static_cast<CAurObject*>(GetTargetCSWCObject()->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                                 m_sTargetNode.CStr(), 0);
        return;
    }
    m_pLight->AttachToObject(pModel, m_sTargetNode.CStr(), 0);
    m_pLight->PlayAnimation(const_cast<char*>("impact"), 1.0f, 4, 0.0f);
#endif
}

// Destroys the light slot and every slot effect. The m_apReserved1 trio is never set by the engine, so
// those three deletes are always null tests.
void CSWCVisualEffectOnObject::UnloadVisualEffect()
{
    UnloadHardCodedVisualEffect();
    for (int i = 0; i < 3; i++)
    {
        if (m_apSlotEffect[i] != 0)
        {
            delete m_apSlotEffect[i];
            m_apSlotEffect[i] = 0;
        }
        if (m_apReserved1[i] != 0)
        {
            delete m_apReserved1[i];
            m_apReserved1[i] = 0;
        }
    }
}

CSWCVisualEffectOnObject::~CSWCVisualEffectOnObject()
{
    delete m_pSoundSource;
    UnloadVisualEffect();
}

void CSWCVisualEffectOnObject::ApplyDistortion(unsigned short nId)
{
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (nId == PROGFX_DISTORTION && pClient != 0)
        pClient->EnableDistortion();
}

// The bands that own a light slot select their Load* handler; the others just report success.
int CSWCVisualEffectOnObject::LoadHardCodedVisualEffect(unsigned short nId)
{
    if (static_cast<unsigned short>(nId - PROGFX_LIGHT_SOURCE) < PROGFX_BAND_SIZE)
        return LoadLightSource(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_ALPHA) < PROGFX_BAND_SIZE)
        return LoadAlpha(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_BEAM) < PROGFX_BAND_SIZE)
        return LoadBeam(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_SPELL_VISUAL) < PROGFX_BAND_SIZE)
        return LoadSpellVisual(nId);
    else if (static_cast<unsigned short>(nId - PROGFX_MODEL_ATTACHMENT) < PROGFX_BAND_SIZE)
        return LoadModelAttachment(nId);
    return 1;
}

int CSWCVisualEffectOnObject::LoadSpellVisual(unsigned short nId)
{
    if (nId == PROGFX_SPELL_FIZZLE_HEAD || nId == PROGFX_SPELL_FIZZLE_HAND)
        m_pLight = NewCAurObject(const_cast<char*>("v_fizzle_imp"), const_cast<char*>("fxfail"), 0, 0);
    else if (nId == PROGFX_SPELL_RESISTED)
        m_pLight = NewCAurObject(const_cast<char*>("v_fresist_imp"), const_cast<char*>("fxresist"), 0, 0);
    else
        return 0;
    return m_pLight != 0;
}

int CSWCVisualEffectOnObject::LoadLightSource(unsigned short nId)
{
    K2_UNUSED(nId);
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return 0;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (pClient == 0)
        return 0;
    m_pLight = NewCAurObject(const_cast<char*>("fx_light_clr"), const_cast<char*>("fx_light"), 0, 0);
    if (m_pLight == 0)
        return 0;
    m_pLight->SetLightPriority(3);
    return 1;
}

void CSWCVisualEffectOnObject::RestoreObjectPosition()
{
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (pClient == 0)
        return;
    pClient->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)->SetPartLocalPosition(const_cast<char*>("root"), Vector(0.0f, 0.0f, 0.0f),
                                                    Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
}

int CSWCVisualEffectOnObject::LoadModelAttachment(unsigned short nId)
{
    switch (nId)
    {
    case PROGFX_ATTACH_MEDAL:
        m_pLight = NewCAurObject(const_cast<char*>("v_medal_dur"), const_cast<char*>("fxmedal"), 0, 0);
        break;
    case PROGFX_ATTACH_REVAN_MASK_1:
        m_pLight = NewCAurObject(const_cast<char*>("v_revmask1_dur"), const_cast<char*>("fxrevanmask1"), 0, 0);
        break;
    case PROGFX_ATTACH_REVAN_MASK_2:
        m_pLight = NewCAurObject(const_cast<char*>("v_revmask2_dur"), const_cast<char*>("fxrevanmask2"), 0, 0);
        break;
#if !KOTOR1  // K1 has three attachment ids (no mask)
    case PROGFX_ATTACH_BREATH:
        m_pLight = NewCAurObject(const_cast<char*>("v_MthMst"), const_cast<char*>("v_MthMst"), 0, 0);
        break;
#endif
    }
    return m_pLight != 0;
}

void CSWCVisualEffectOnObject::RemoveBumpedOutShell()
{
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (pClient == 0)
        return;
    pClient->DisableBumpedOutShell();
    CSWCCreature* pCreature = pClient->AsSWCCreature();
    if (pCreature == 0)
        return;
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
        pCreature->GetEquippedItemID(CSWInventory::SLOT_HEAD));
    if (pItem == 0)
        return;
    pItem->DisableBumpedOutShell();
}

void CSWCVisualEffectOnObject::RemoveTextureReplace(unsigned short nId)
{
    K2_UNUSED(nId);
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_gameObjectId);
    if (pObject == 0)
        return;
    CSWCObject* pClient = pObject->AsSWCObject();
    if (pClient == 0)
        return;
    if (pClient->m_bStoneTexture == 1)
        pClient->m_bStoneTexture = 0;
    if (pClient->m_bBarkTexture == 1)
        pClient->m_bBarkTexture = 0;
    if (pClient->m_bShadeTexture == 1)
        pClient->m_bShadeTexture = 0;
    pClient->RestoreTexture();
    CSWCCreature* pCreature = pClient->AsSWCCreature();
    if (pCreature == 0)
        return;
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
        pCreature->GetEquippedItemID(CSWInventory::SLOT_HEAD));
    if (pItem == 0)
        return;
    pItem->RestoreTexture();
}

#if !KOTOR1
// For the mask id the object goes to the area's deletion list instead of being unregistered from
// the scene here.
void CSWCVisualEffectOnObject::RemoveModelAttachment(unsigned short nId)
{
    if (m_pLight == 0)
        return;
    if (nId == PROGFX_ATTACH_BREATH)
    {
        m_pLight->PlayAnimation(const_cast<char*>("stop"), 0.0f, 1, 0.0f);
        m_pLight->AttachToObject(0, 0, 0);
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule != 0 && pModule->m_pArea != 0)
        {
            pModule->m_pArea->AddToBeDeletedEmitter(m_pLight);
            m_pLight = 0;
            return;
        }
    }
    else
    {
        m_pLight->AttachToObject(0, 0, 0);
    }
    m_pLight->AttachToScene(0);
}
#endif

void CSWCVisualEffectOnObject::UnloadHardCodedVisualEffect()
{
    if (m_pLight == 0)
        return;
    m_pLight->AttachToObject(0, 0, 0);
    m_pLight->AttachToScene(0);
    delete m_pLight;
    m_pLight = 0;
}

// The id bands are monotone in the id but not in the class, so they cannot be folded
// arithmetically.
unsigned char CSWCVisualEffectOnObject::GetHardCodedEffectClass(unsigned short nId)
{
    if (nId < PROGFX_SELF_ILLUMINATION)
        return HARDCODED_CLASS_ENVIRONMENT_MAP;
    if (nId < PROGFX_LIGHT_SOURCE)
        return HARDCODED_CLASS_TINT_ALPHA;
    if (nId < PROGFX_ALPHA)
        return HARDCODED_CLASS_LIGHT_SOURCE;
    if (nId < PROGFX_BEAM)
        return HARDCODED_CLASS_TINT_ALPHA;
    if (nId < PROGFX_DELETE_MODEL)
        return HARDCODED_CLASS_BEAM;
    if (nId < PROGFX_CHUNK)
        return HARDCODED_CLASS_DELETE_MODEL;
    if (nId < PROGFX_MIRV)
        return HARDCODED_CLASS_CHUNK;
    if (nId < PROGFX_TEXTURE_REPLACE)
        return HARDCODED_CLASS_MIRV;
    if (nId < PROGFX_CHEAT)
        return HARDCODED_CLASS_TEXTURE_REPLACE;
    if (nId < PROGFX_SPELL_VISUAL)
        return HARDCODED_CLASS_CHEAT;
    if (nId < PROGFX_LIGHTSABER_THROW)
        return HARDCODED_CLASS_SPELL_VISUAL;
    if (nId < PROGFX_MOTION_BLUR)
        return HARDCODED_CLASS_THROW_SHELL_FOV;
    return HARDCODED_CLASS_NONE;
}

extern int quaternionidentityoptimization;

// Rotates v by q, skipping the work when the rotation is the identity.
static inline Vector RotateVector(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        Vector r;
        r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
        r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
        r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
        return r;
    }
    return v;
}

// Throws the owner's equipped lightsaber: builds a CSWCProjectile from the saber item's art resref,
// parks it on the source's "rhand" part (or the target's "impact" part when source and owner are the
// same object), spins it 70 degrees about the horizontal axis perpendicular to the source's facing,
// aims it at the target and hands it to the area's scene.
void CSWCVisualEffectOnObject::ApplyLightsaberThrow(unsigned short nId)
{
    K2_UNUSED(nId);
    CSWCObject* pTarget = GetTargetCSWCObject();
    CSWCObject* pSource = GetSourceCSWCObject();
    CSWCObject* pOwner = GetOwnerCSWCObject();
    if (pOwner == 0)
        return;

    // No throw rate on the creature yet: fall back to a flat 37 units/second baseline.
    float fThrowRate = 37.0f;
    if (pOwner->AsSWCCreature() != 0)
        fThrowRate = pOwner->AsSWCCreature()->GetLightSaberThrowRate();

    if (pTarget == 0)
        return;
    if (pSource == 0)
        return;
    if (pOwner->AsSWCCreature() == 0)
        return;

    CSWCCreature* pCreature = pOwner->AsSWCCreature();
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
        pCreature->GetEquippedItemID(CSWInventory::SLOT_RIGHTWEAPON));
    if (pItem == 0)
        return;

    CSWCProjectile* pProjectile = new CSWCProjectile;
    pProjectile->SetProjectileSpellId(FORCE_POWER_LIGHT_SABER_THROW);
    pProjectile->m_sourceObjectId = pOwner->m_objectId;
    pProjectile->LoadModel(CResRef(pItem->m_modelResRef.GetResRef()), CSWCObject::ANIM_MODEL_BODY, 0);
    pProjectile->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)->DisableForceAnimations();

    Vector vPart;
    Quaternion qPart;
    if (pSource->m_objectId == pOwner->m_objectId)
        pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)->GetPartPosition(const_cast<char*>("rhand"), &vPart, &qPart);
    else
        pSource->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)->GetPartPosition(const_cast<char*>("impact"), &vPart, &qPart);
    pProjectile->SetPosition(vPart);
    pCreature->SetThrownLightsaber(pProjectile->m_objectId);

    // The cross product is spelled against an explicit world-up rather than simplified to
    // (v.y, -v.x, 0).
    Vector vOrient = pSource->m_orientation;
    Vector vUp = { 0.0f, 0.0f, 1.0f };
    Vector vSide;
    vSide.x = vOrient.y * vUp.z - vOrient.z * vUp.y;
    vSide.y = vOrient.z * vUp.x - vOrient.x * vUp.z;
    vSide.z = vOrient.x * vUp.y - vOrient.y * vUp.x;
    Quaternion qSpin(vSide, static_cast<float>(70.0f * 3.141592653589793 / 180.0));
    pProjectile->SetOrientation(RotateVector(vOrient, qSpin));

    pProjectile->m_desiredPosition = pTarget->m_position;
    if (pOwner == pTarget)
        pProjectile->SetDesiredTargetObject(pTarget->m_objectId, CExoString("rhand"));
    else
        pProjectile->SetDesiredTargetObject(pTarget->m_objectId, CExoString(""));

    // Travel time is the target distance at the owner's throw rate, in 2000ths of a second.
    Vector vDelta = pTarget->m_position - pSource->m_position;
    pProjectile->SetDesiredPositionTimer(static_cast<int>(magnitude(vDelta) / fThrowRate * 2000.0f));
    pProjectile->SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_THROW_OUT);
    pProjectile->SetProjectileType(CSWCProjectile::PROJECTILE_TYPE_HOMING);
    pProjectile->m_nImpactMode = 0;

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule == 0)
        return;
    if (pModule->m_pArea == 0)
        return;
    pProjectile->AttachToScene(pModule->m_pArea->m_pScene);
}

// Hangs the effect's model off a part of the target's model. The "OrientationOff" column of the
// effect's visualeffects 2DA row decides the attachment mode: nonzero snaps the part to its "root"
// position with an identity rotation, zero (or a failed lookup) attaches it unoriented.
void CSWCVisualEffectOnObject::AttachVFXModelToTargetModel(CAurObject* pTarget, CAurObject* pModel,
                                                          const CExoString& sPart)
{
    int nOrientationOff;
    if (g_pRules->m_p2DArrays->m_pVisualEffect->GetINTEntry(CExoString(m_effectId),
                                                            CExoString("OrientationOff"),
                                                            &nOrientationOff) == 0)
        nOrientationOff = 0;

    if (nOrientationOff != 0)
    {
        Vector vPos;
        Quaternion qRot;
        pTarget->AttachToObject(pModel, sPart.CStr(), 3);
        pTarget->GetPartPosition(const_cast<char*>("root"), &vPos, &qRot);
        pTarget->SetPartPosition(const_cast<char*>("root"), vPos, Quaternion());
    }
    else
    {
        pTarget->AttachToObject(pModel, sPart.CStr(), 0);
    }
}

// Applies the arcing-projectile ("mirv") effects magmisl, arofire and aroacid. A new CSWCProjectile
// launches from the source model's "handconjure" part, aimed at the target's position or along the
// cached m_vTargetPosition aim vector when that is still zero, and is handed to the area's scene.
// Its travel timer is the logarithmic toss curve for magmisl/arofire and twice the ballistic
// estimate for aroacid.
void CSWCVisualEffectOnObject::ApplyMirv(unsigned short nId)
{
    Quaternion qRot;
    Vector vPos;
    CExoString sModel;
    CSWCObject* pTarget;
    CSWCObject* pSource;
    CSWCProjectile* pProjectile;

    pTarget = GetTargetCSWCObject();
    pSource = GetSourceCSWCObject();
    if (pTarget == 0)
        return;
    if (pSource == 0)
        return;

    pProjectile = new CSWCProjectile();
    pProjectile->SetProjectileSpellId(CSWCProjectile::SPELL_ID_MIRV);

    switch (nId)
    {
    case PROGFX_MIRV_MAGIC_MISSILE:
        sModel = "vpr_magmisl";
        break;
    case PROGFX_MIRV_FIRE_ARROW:
        sModel = "vpr_arofire";
        pProjectile->m_nProjOrientation = 2;
        break;
    case PROGFX_MIRV_ACID_ARROW:
        sModel = "vpr_aroacid";
        pProjectile->m_nProjOrientation = 2;
        break;
    }

    if (pProjectile->LoadModel(CResRef(sModel), CSWCObject::ANIM_MODEL_BODY, 0) == 0)
    {
        delete pProjectile;
        return;
    }

    if (pSource->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)->GetPartPosition(const_cast<char*>("handconjure"), &vPos, &qRot) == 0)
        vPos = pSource->m_position;
    pProjectile->SetPosition(vPos);
    pProjectile->SetOrientation(pSource->m_orientation);

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule != 0)
    {
        CSWCArea* pArea = pModule->m_pArea;
        if (pArea != 0)
            pProjectile->AttachToScene(pArea->m_pScene);
    }

    pProjectile->SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_TRAVEL);

    if (equal(m_vTargetPosition, Vector(0.0f, 0.0f, 0.0f)))
    {
        pProjectile->m_desiredPosition = pTarget->m_position;
        pProjectile->SetDesiredTargetObject(pTarget->m_objectId, CExoString(""));
    }
    else
        pProjectile->m_desiredPosition = m_vTargetPosition;

    switch (nId)
    {
    case PROGFX_MIRV_MAGIC_MISSILE:
    {
        Vector vDelta = pTarget->m_position - pSource->m_position;
        float fDistance = magnitude(vDelta);
        unsigned int nTimer = static_cast<unsigned int>(fDistance / (logf(fDistance) * 3.0f + 2.0f) * 1000.0f);
        pProjectile->SetDesiredPositionTimer(nTimer);
        pProjectile->SetProjectileType(CSWCProjectile::PROJECTILE_TYPE_BURST_UP);
        break;
    }
    case PROGFX_MIRV_FIRE_ARROW:
    {
        Vector vDelta = pTarget->m_position - pSource->m_position;
        float fDistance = magnitude(vDelta);
        unsigned int nTimer = static_cast<unsigned int>(fDistance / (logf(fDistance) * 3.0f + 2.0f) * 1000.0f);
        pProjectile->SetDesiredPositionTimer(nTimer);
        pProjectile->SetProjectileType(CSWCProjectile::PROJECTILE_TYPE_BURST_UP);
        break;
    }
    case PROGFX_MIRV_ACID_ARROW:
    {
        // The receiver is the projectile, not the target. CSWCProjectile does not derive from
        // CSWCObject yet, hence the explicit base cast.
        unsigned int nTimer =
            static_cast<CSWCObject*>(pProjectile)->CalculateProjectileTimeToTarget(pTarget->m_position, 0) * 2;
        pProjectile->SetDesiredPositionTimer(nTimer);
        pProjectile->SetProjectileType(CSWCProjectile::PROJECTILE_TYPE_HOMING);
        break;
    }
    }
}

// Bands 1010..1040 (step 10) each stamp one "vdu_tex_*" material onto the target and latch a flag on
// the client object (stone/granite-stone set m_bStoneTexture, bark m_bBarkTexture, shade m_bShadeTexture), then the same
// replacement is repeated on the target's equipped item when the target is a creature.
void CSWCVisualEffectOnObject::ApplyTextureReplace(unsigned short nId)
{
    CExoString sTexture;
    CSWCObject* pTarget;
    unsigned short nType;
    CSWCCreature* pCreature;

    pTarget = GetTargetCSWCObject();
    if (pTarget == 0)
        return;
    nType = nId - PROGFX_TEXTURE_REPLACE;
    switch (nType)
    {
    case 10:
        sTexture = "vdu_tex_stone";
        pTarget->m_bStoneTexture = 1;
        break;
    case 20:
        sTexture = "vdu_tex_grstone";
        pTarget->m_bStoneTexture = 1;
        break;
    case 30:
        sTexture = "vdu_tex_bark";
        pTarget->m_bBarkTexture = 1;
        break;
    case 40:
        sTexture = "vdu_tex_shade";
        pTarget->m_bShadeTexture = 1;
        break;
    default:
        return;
    }
    pTarget->ReplaceTexture(CResRef(""), CResRef(sTexture), 0);

    pCreature = pTarget->AsSWCCreature();
    if (pCreature == 0)
        return;
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
        pCreature->GetEquippedItemID(CSWInventory::SLOT_HEAD));
    if (pItem == 0)
        return;
    pItem->ReplaceTexture(CResRef(""), CResRef(sTexture), 0);
}

// The light's attachment joint comes from a 3-entry table (headconjure / handconjure / impact); the
// third entry is the projectile-trail variant, which aims the light's "root" part from the target at
// the source. Both paths finish by playing "impact".
void CSWCVisualEffectOnObject::ApplySpellVisual(unsigned short nId)
{
    static const char* apSpellVisualNames[3] = { "headconjure", "handconjure", "impact" };
    unsigned short nType = nId - PROGFX_SPELL_VISUAL;
    if (nType <= 2)
        m_sTargetNode = apSpellVisualNames[static_cast<short>(nType)];
    if (m_pLight == 0)
        return;
    if (nId == PROGFX_SPELL_RESISTED)
    {
        Quaternion qRot;
        Vector vPos(0.0f, 0.0f, 0.0f);
        CSWCObject* pSource = GetSourceCSWCObject();
        CSWCObject* pTarget = GetTargetCSWCObject();
        m_pLight->GetPartPosition(const_cast<char*>("root"), &vPos, &qRot);
        if (pSource != 0 && pTarget != 0)
        {
            Vector vDir = pSource->m_position - pTarget->m_position;
            if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z <= 0.0001f)
                vDir = Vector(0.0f, 0.0f, 0.0f);
            vDir = normalize(vDir);
            qRot = YawPitchRoll(Yaw(vDir), 0.0f, 0.0f);
        }
        m_pLight->SetPartPosition(const_cast<char*>("root"), vPos, qRot);
        m_pLight->AttachToObject(static_cast<CAurObject*>(GetTargetCSWCObject()->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                                 m_sTargetNode.CStr(), 3);
    }
    else
    {
        m_pLight->AttachToObject(static_cast<CAurObject*>(GetTargetCSWCObject()->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)),
                                 m_sTargetNode.CStr(), 0);
    }
    m_pLight->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
}

// Bands 1401..1426 name the 26 "fx_tex_*" bumped-shell materials (the sequence skips fx_tex_13 and
// ends on fx_tex_stealth), applied at 0.02 to the target and, for a creature, to its equipped item.
void CSWCVisualEffectOnObject::ApplyBumpedOutShell(unsigned short nId)
{
    CExoString sTexture;
    CSWCObject* pTarget;
    unsigned short nType;
    CSWCCreature* pCreature;

    m_nPriority = 20;
    pTarget = GetTargetCSWCObject();
    if (pTarget == 0)
        return;
    nType = nId - (PROGFX_BUMPED_OUT_SHELL + 1);
    switch (nType)
    {
    case 0:
        sTexture = "fx_tex_01";
        break;
    case 1:
        sTexture = "fx_tex_02";
        break;
    case 2:
        sTexture = "fx_tex_03";
        break;
    case 3:
        sTexture = "fx_tex_04";
        break;
    case 4:
        sTexture = "fx_tex_05";
        break;
    case 5:
        sTexture = "fx_tex_06";
        break;
    case 6:
        sTexture = "fx_tex_07";
        break;
    case 7:
        sTexture = "fx_tex_08";
        break;
    case 8:
        sTexture = "fx_tex_09";
        break;
    case 9:
        sTexture = "fx_tex_10";
        break;
    case 10:
        sTexture = "fx_tex_11";
        break;
    case 11:
        sTexture = "fx_tex_12";
        break;
    case 12:
        sTexture = "fx_tex_14";
        break;
    case 13:
        sTexture = "fx_tex_15";
        break;
    case 14:
        sTexture = "fx_tex_16";
        break;
    case 15:
        sTexture = "fx_tex_17";
        break;
    case 16:
        sTexture = "fx_tex_18";
        break;
    case 17:
        sTexture = "fx_tex_19";
        break;
    case 18:
        sTexture = "fx_tex_20";
        break;
    case 19:
        sTexture = "fx_tex_21";
        break;
    case 20:
        sTexture = "fx_tex_22";
        break;
    case 21:
        sTexture = "fx_tex_23";
        break;
    case 22:
        sTexture = "fx_tex_24";
        break;
    case 23:
        sTexture = "fx_tex_25";
        break;
    case 24:
        sTexture = "fx_tex_26";
        break;
    case 25:
        sTexture = "fx_tex_stealth";
        break;
    default:
        return;
    }
    pTarget->EnableBumpedOutShell(CResRef(sTexture), 0.02f);

    pCreature = pTarget->AsSWCCreature();
    if (pCreature == 0)
        return;
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
        pCreature->GetEquippedItemID(CSWInventory::SLOT_HEAD));
    if (pItem == 0)
        return;
    pItem->EnableBumpedOutShell(CResRef(sTexture), 0.02f);
}

// Spawns a CSWCProjectile to carry the cheat effect: its model resref is "vpr_kngpow" for one
// variant (empty otherwise), it starts 4 "orientation units" in front of the source object and one
// unit up, and it steers at the target's position (or along the cached m_vTargetPosition aim vector when that
// is still zero). That variant stretches its travel timer to `dist / (3*log(dist) + 2)` seconds and
// plays "c_cow_atk1" at the source. A projectile whose model fails to load is deleted.
void CSWCVisualEffectOnObject::ApplyCheat(unsigned short nId)
{
    CExoString sTexture;
    CSWCObject* pTarget = GetTargetCSWCObject();
    CSWCObject* pSource = GetSourceCSWCObject();
    if (pTarget == 0 || pSource == 0)
        return;
    CSWCProjectile* pProjectile = new CSWCProjectile();
    if (nId == PROGFX_CHEAT_KNGPOW)
        sTexture = "vpr_kngpow";
    if (pProjectile->LoadModel(CResRef(sTexture), CSWCObject::ANIM_MODEL_BODY, 0))
    {
        Vector vPos = pSource->m_orientation * 4.0f + pSource->m_position;
        vPos.z += 1.0f;
        pProjectile->SetPosition(vPos);
        pProjectile->SetOrientation(pSource->m_orientation);

        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule != 0 && pModule->m_pArea != 0)
            pProjectile->AttachToScene(pModule->m_pArea->m_pScene);

        pProjectile->SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_TRAVEL);
        pProjectile->SetProjectileSpellId(CSWCProjectile::SPELL_ID_COW);

        if (equal(m_vTargetPosition, Vector(0.0f, 0.0f, 0.0f)))
        {
            pProjectile->m_desiredPosition = pTarget->m_position;
            pProjectile->SetDesiredTargetObject(pTarget->m_objectId, CExoString(""));
        }
        else
            pProjectile->m_desiredPosition = m_vTargetPosition;

        if (nId == PROGFX_CHEAT_KNGPOW)
        {
            float fDistance = magnitude(pTarget->m_position - pSource->m_position);
            pProjectile->SetDesiredPositionTimer(
                static_cast<unsigned int>(fDistance / (logf(fDistance) * 3.0f + 2.0f) * 1000.0f));
            pProjectile->m_nProjOrientation = 2;
            pProjectile->SetProjectileType(CSWCProjectile::PROJECTILE_TYPE_BURST_UP);
            g_pExoSound->Play3DOneShotSound(CResRef("c_cow_atk1"), pSource->m_position, 1.5f,
                                            PRIORITY_GROUP_NORMAL_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);
        }
    }
    else
        delete pProjectile;
}

// The chunk/blood-splat arm, gated on the graphics settings and on the target resolving to a
// creature. Both 2DA lookups at the tail are dead: the "bloodcolr" result lands in a local that is
// only destroyed and the size-category int goes into a local that is never read.
void CSWCVisualEffectOnObject::ApplyChunk(unsigned short nId)
{
    K2_UNUSED(nId);
    int nSizeCategory;
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nViolenceLevel < 2)
        return;

    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pTarget == 0)
        return;
    if (pTarget->m_pArea == 0)
        return;

    CSWCCreature* pCreature = pTarget->AsSWCCreature();
    if (pCreature == 0)
        return;

    if (pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1) != 0)
    {
        Vector vPos;
        Quaternion qRot;
        pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)->GetPartLocalPosition(const_cast<char*>("impact"), &vPos, &qRot);
    }

    CExoString sBloodColor;
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        pCreature->m_pCreatureAppearance->m_nAppearanceId, CExoString("bloodcolr"), &sBloodColor);
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        pCreature->m_pCreatureAppearance->m_nAppearanceId,
        CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSizeCategory);
}

// The per-frame tick. The IMPACT and CESSATION stages count m_nEffectTimer down and tear the
// hard-coded effect down when it runs out; then the three effect slots are ticked, the sound source
// is re-anchored to the target and the three expiry flags are walked. `sUnused` is constructed and
// destroyed but never read.
void CSWCVisualEffectOnObject::Update(unsigned int nDelta)
{
    CExoString sUnused;
    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pTarget == 0)
        return;

    switch (m_nState)
    {
    case STAGE_IMPACT:
    case STAGE_CESSATION:
        if (m_nEffectTimer <= nDelta)
        {
            m_nEffectTimer = 0;
            if (m_nState == STAGE_IMPACT)
            {
                RemoveHardCodedVisualEffect(m_nEffectId0);
                UnloadHardCodedVisualEffect();
                if (m_bHasDuration != 0)
                {
                    if (m_bFinished == 0)
                    {
                        m_nState = STAGE_DURATION;
                        m_hardCodedEffectClass = GetHardCodedEffectClass(m_nEffectId1);
                        LoadHardCodedVisualEffect(m_nEffectId1);
                        ApplyHardCodedVisualEffect(m_nEffectId1);
                        if (m_bAttached == 0)
                            DetachVisualEffect();
                    }
                }
                else
                {
                    m_nState = STAGE_NONE;
                    m_bFinished = 1;
                }
            }
            else if (m_nState == STAGE_CESSATION)
            {
                if (m_bAttached != 0)
                    RemoveHardCodedVisualEffect(m_nEffectId2);
                UnloadHardCodedVisualEffect();
                m_nState = STAGE_NONE;
            }
        }
        else
        {
            m_nEffectTimer -= nDelta;
            if (m_bShown != 0 && m_bAttached != 0)
            {
                if (m_bFinished == 0 && m_nState == STAGE_IMPACT)
                    UpdateHardCodedVisualEffect(m_nEffectId0, nDelta);
                else if (m_nState == STAGE_CESSATION)
                    UpdateHardCodedVisualEffect(m_nEffectId2, nDelta);
            }
        }
        break;
    case STAGE_DURATION:
        if (m_bShown != 0 && m_bAttached != 0 && m_bFinished == 0)
            UpdateHardCodedVisualEffect(m_nEffectId1, nDelta);
        break;
    }

    for (int i = 0; i < 3; i++)
    {
        if (m_anSlotTimer[i] == 0)
            continue;
        if (static_cast<unsigned int>(m_anSlotTimer[i]) > nDelta)
        {
            m_anSlotTimer[i] -= nDelta;
            continue;
        }
        m_anSlotTimer[i] = 0;
        // A CESSATION slot is retired through SetEffectDone; an IMPACT one either
        // restarts its animation or, when m_bHasDuration asks for it, is retired the same way.
        switch (m_aSlotId[i])
        {
        case STAGE_IMPACT:
            if (m_bHasDuration != 0)
            {
                m_aSlotId[i] = STAGE_DURATION;
                m_apSlotEffect[i]->PlayAnimation(const_cast<char*>("duration"), 1.0f, 0, 0.0f);
                break;
            }
            SetEffectDone(m_apSlotEffect[i]);
            m_bExpiring = 1;
            m_bFinished = 1;
            break;
        case STAGE_CESSATION:
            SetEffectDone(m_apSlotEffect[i]);
            m_bExpiring = 1;
            m_bFinished = 1;
            break;
        }
    }

    if (m_pSoundSource != 0)
        m_pSoundSource->SetPosition(pTarget->m_position, 0.0f);

    if (m_bExpiring != 0)
    {
        if (m_bEffect0Expired == 0)
        {
            if (static_cast<unsigned int>(m_nEffectDuration0) > nDelta)
                m_nEffectDuration0 -= nDelta;
            else
                m_bEffect0Expired = 1;
        }
        if (m_bEffect1Expired == 0)
        {
            if (static_cast<unsigned int>(m_nEffectDuration1) > nDelta)
                m_nEffectDuration1 -= nDelta;
            else
                m_bEffect1Expired = 1;
        }
        if (m_bEffect2Expired == 0)
        {
            if (static_cast<unsigned int>(m_nEffectDuration2) > nDelta)
                m_nEffectDuration2 -= nDelta;
            else
                m_bEffect2Expired = 1;
        }
    }
}

// Creates the effect. It resolves the visualeffects.2da row for this effect id into four part-name
// strings (the head-conjure hook, the impact hook and a size-category-dependent root hook, each
// with a LowQuality/LowViolence override), picks the two model-hook names from the source and
// target objects, seeds the slot state and instantiates up to three CAurObjects.
int CSWCVisualEffectOnObject::LoadVisualEffect(unsigned short a, int b, unsigned long long objId,
                                               unsigned long long c, unsigned long long d,
                                               unsigned char e, unsigned char f, Vector v)
{
    CExoString sTypeFD;
    CExoString sHeadCon;
    CExoString sImpactNode;
    CExoString sRootNode;

    int nProgFXImpact K2_UNSET;
    int nProgFXDuration K2_UNSET;
    int nProgFXCessation K2_UNSET;
    int nSizeCategory;
    int bHasImpact;
    int bHasDuration;
    int bHasCessation;
    CSWCObject* pSource;
    CSWCPlaceable* pPlaceable;
    CSWCDoor* pDoor;
    CSWCPlaceable* pTargetPlaceable;
    CSWCDoor* pTargetDoor;

    m_effectId = a;
    m_gameObjectId = objId;
    m_sourceObjectId = c;
    m_ownerObjectId = d;

    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pTarget == 0)
        return 0;
    pSource = GetSourceCSWCObject();

    CExoString sId(a);
    g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
        sId, CExoString("Type_FD"), &sTypeFD);
    g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
        sId, CExoString("Imp_HeadCon_Node"), &sHeadCon);

    if (sHeadCon != "")
    {
        CExoString sTmp;
        if ((g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFlags0Hi & 1) == 0)
        {
            if (g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                    sId, CExoString("LowQuality"), &sTmp))
                sHeadCon = sTmp;
        }
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nViolenceLevel <= 1)
        {
            if (g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                    sId, CExoString("LowViolence"), &sTmp))
                sHeadCon = sTmp;
        }
    }

    g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
        sId, CExoString("Imp_Impact_Node"), &sImpactNode);
    if (sImpactNode != "")
    {
        CExoString sTmp;
        if ((g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFlags0Hi & 1) == 0)
        {
            if (g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                    sId, CExoString("LowQuality"), &sTmp))
                sImpactNode = sTmp;
        }
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nViolenceLevel <= 1)
        {
            CExoString sLowViolence;
            if (g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                    sId, CExoString("LowViolence"), &sLowViolence))
                g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                    sLowViolence, CExoString("Imp_Impact_Node"), &sImpactNode);
        }
    }

    CSWCCreature* pCreature = pTarget->AsSWCCreature();
    if (pCreature == 0
        || g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
               pCreature->m_pCreatureAppearance->m_nAppearanceId,
               CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSizeCategory) == 0)
    {
        nSizeCategory = CREATURE_SIZE_MEDIUM;
    }
    else
    {
        if (nSizeCategory > CREATURE_SIZE_LARGE)
            g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                sId, CExoString("Imp_Root_H_Node"), &sRootNode);
#if KOTOR1  // K1: CExoString::GetLength is out of line
        if (nSizeCategory >= CREATURE_SIZE_LARGE
            && sRootNode.GetLength() == 0)
#else
        if (nSizeCategory >= CREATURE_SIZE_LARGE
            && (sRootNode.m_sString == 0 || static_cast<int>(strlen(sRootNode.m_sString)) == 0))
#endif
            g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                sId, CExoString("Imp_Root_L_Node"), &sRootNode);
    }
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (nSizeCategory > CREATURE_SIZE_INVALID
        && sRootNode.GetLength() == 0)
#else
    if (nSizeCategory > CREATURE_SIZE_INVALID
        && (sRootNode.m_sString == 0 || static_cast<int>(strlen(sRootNode.m_sString)) == 0))
#endif
        g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
            sId, CExoString("Imp_Root_M_Node"), &sRootNode);

    if (sRootNode != "")
    {
        CExoString sTmp;
        if ((g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFlags0Hi & 1) == 0)
        {
            if (g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                    sId, CExoString("LowQuality"), &sTmp))
                sRootNode = sTmp;
        }
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nViolenceLevel <= 1)
        {
            CExoString sLowViolence;
            if (g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                    sId, CExoString("LowViolence"), &sLowViolence))
            {
                if (nSizeCategory > CREATURE_SIZE_LARGE)
                    g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                        sLowViolence, CExoString("Imp_Root_H_Node"), &sRootNode);
#if KOTOR1  // K1: CExoString::GetLength is out of line
                if (nSizeCategory >= CREATURE_SIZE_LARGE
                    && sRootNode.GetLength() == 0)
#else
                if (nSizeCategory >= CREATURE_SIZE_LARGE
                    && (sRootNode.m_sString == 0 || static_cast<int>(strlen(sRootNode.m_sString)) == 0))
#endif
                    g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                        sLowViolence, CExoString("Imp_Root_L_Node"), &sRootNode);
#if KOTOR1  // K1: CExoString::GetLength is out of line
                if (nSizeCategory > CREATURE_SIZE_INVALID
                    && sRootNode.GetLength() == 0)
#else
                if (nSizeCategory > CREATURE_SIZE_INVALID
                    && (sRootNode.m_sString == 0 || static_cast<int>(strlen(sRootNode.m_sString)) == 0))
#endif
                    g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                        sLowViolence, CExoString("Imp_Root_M_Node"), &sRootNode);
            }
        }
    }

#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (sRootNode.GetLength() == 0)
#else
    if (sRootNode.m_sString == 0 || static_cast<int>(strlen(sRootNode.m_sString)) == 0)
#endif
    {
        bHasImpact = g_pRules->m_p2DArrays->m_pVisualEffect->GetINTEntry(
            sId, CExoString("ProgFX_Impact"), &nProgFXImpact);
        bHasDuration = g_pRules->m_p2DArrays->m_pVisualEffect->GetINTEntry(
            sId, CExoString("ProgFX_Duration"), &nProgFXDuration);
        bHasCessation = g_pRules->m_p2DArrays->m_pVisualEffect->GetINTEntry(
            sId, CExoString("ProgFX_Cessation"), &nProgFXCessation);
    }
    else
    {
        bHasImpact = 0;
        bHasDuration = 0;
        bHasCessation = 0;
    }

    pPlaceable = 0;
    pDoor = 0;
    if (pSource != 0)
    {
        pPlaceable = pSource->AsSWCPlaceable();
        pDoor = pSource->AsSWCDoor();
    }

    switch (e)
    {
    case 0:
        if (pPlaceable != 0)
            m_sSourceNode = pPlaceable->m_sModelName + CExoString("_hand");
        else if (pDoor != 0)
            m_sSourceNode = pDoor->m_sModelName + CExoString("impc");
        else
            m_sSourceNode = "handconjure";
        break;
    case 1:
        if (pPlaceable != 0)
            m_sSourceNode = pPlaceable->m_sModelName + CExoString("_impact");
        else if (pDoor != 0)
            m_sSourceNode = pDoor->m_sModelName + CExoString("impc");
        else
            m_sSourceNode = "impact";
        break;
    case 2:
        if (pPlaceable != 0)
            m_sSourceNode = pPlaceable->m_sModelName + CExoString("_head");
        else if (pDoor != 0)
            m_sSourceNode = pDoor->m_sModelName + CExoString("impc");
        else
            m_sSourceNode = "headconjure";
        break;
    case 3:
        if (pPlaceable != 0)
            m_sSourceNode = pPlaceable->m_sModelName + CExoString("_ground");
        else if (pDoor != 0)
            m_sSourceNode = pDoor->m_sModelName + CExoString("grnd");
        else
            m_sSourceNode = "lhand";
        break;
    default:
        if (e == 4)
        {
            if (pPlaceable != 0)
                m_sSourceNode = pPlaceable->m_sModelName + CExoString("_ground");
            else if (pDoor != 0)
                m_sSourceNode = pDoor->m_sModelName + CExoString("grnd");
            else
                m_sSourceNode = "rhand";
        }
        else
        {
            if (pPlaceable != 0)
                m_sSourceNode = pPlaceable->m_sModelName + CExoString("_ground");
            else if (pDoor != 0)
                m_sSourceNode = pDoor->m_sModelName + CExoString("grnd");
            else
                m_sSourceNode = "root";
        }
        break;
    }

    pTargetPlaceable = 0;
    pTargetDoor = 0;
    if (pTarget != 0)
    {
        pTargetPlaceable = pTarget->AsSWCPlaceable();
        pTargetDoor = pTarget->AsSWCDoor();
    }

    if (f <= 1)
    {
        if (pTargetPlaceable != 0)
            m_sTargetNode = pTargetPlaceable->m_sModelName + CExoString("_impact");
        else if (pTargetDoor != 0)
            m_sTargetNode = pTargetDoor->m_sModelName + CExoString("impc");
        else
            m_sTargetNode = "impact";
    }
    else
    {
        if (pTargetPlaceable != 0)
            m_sTargetNode = pTargetPlaceable->m_sModelName + CExoString("_ground");
        else if (pTargetDoor != 0)
            m_sTargetNode = pTargetDoor->m_sModelName + CExoString("grnd");
        else
            m_sTargetNode = "root";
    }

    if (pSource != 0)
    {
        Gob* pModel = pSource->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
        if (pModel != 0 && pModel->GetPart(m_sSourceNode.CStr()) == 0)
            m_sSourceNode = "root";
    }
    if (pTarget != 0)
    {
        Gob* pModel = pTarget->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
        if (pModel != 0 && pModel->GetPart(m_sTargetNode.CStr()) == 0)
            m_sTargetNode = "root";
    }

    m_vTargetPosition = v;
    m_nEffectId0 = PROGFX_NONE;
    m_nEffectId1 = PROGFX_NONE;
    m_nEffectId2 = PROGFX_NONE;
    m_bEffect0Expired = 1;
    m_bEffect1Expired = 1;
    m_bEffect2Expired = 1;
    m_bHasDuration = (b == 0);

    if (bHasImpact)
        m_nEffectId0 = static_cast<unsigned short>(nProgFXImpact);
    if (b == 0)
    {
        if (bHasDuration)
            m_nEffectId1 = static_cast<unsigned short>(nProgFXDuration);
        if (bHasCessation)
            m_nEffectId2 = static_cast<unsigned short>(nProgFXCessation);
    }

#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (sRootNode.GetLength() > 0)
#else
    if (sRootNode.m_sString != 0 && static_cast<int>(strlen(sRootNode.m_sString)) > 0)
#endif
    {
        m_apSlotEffect[0] = NewCAurObject(sRootNode.CStr(), const_cast<char*>("fxground"), 0, 0);
        if (m_apSlotEffect[0] != 0)
        {
            m_bEffect0Expired = 0;
            m_apSlotEffect[0]->SetLightPriority(3);
            m_apSlotEffect[0]->DisableFog();
        }
    }
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (sImpactNode.GetLength() > 0)
#else
    if (sImpactNode.m_sString != 0 && static_cast<int>(strlen(sImpactNode.m_sString)) > 0)
#endif
    {
        m_apSlotEffect[1] = NewCAurObject(sImpactNode.CStr(), const_cast<char*>("fximpact"), 0, 0);
        if (m_apSlotEffect[1] != 0)
        {
            m_bEffect1Expired = 0;
            m_apSlotEffect[1]->SetLightPriority(3);
            m_apSlotEffect[1]->DisableFog();
        }
    }
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (sHeadCon.GetLength() > 0)
#else
    if (sHeadCon.m_sString != 0 && static_cast<int>(strlen(sHeadCon.m_sString)) > 0)
#endif
    {
        m_apSlotEffect[2] = NewCAurObject(sHeadCon.CStr(), const_cast<char*>("fxhead"), 0, 0);
        if (m_apSlotEffect[2] != 0)
        {
            m_bEffect2Expired = 0;
            m_apSlotEffect[2]->SetLightPriority(3);
            m_apSlotEffect[2]->DisableFog();
        }
    }

    m_nEffectTimer = 0;
    if (m_nEffectId0 != PROGFX_NONE)
        m_nEffectTimer = GetHardCodedImpactLength(m_nEffectId0);
    else if (m_nEffectId1 == PROGFX_NONE && m_nEffectId2 != PROGFX_NONE)
        m_nEffectTimer = 1000;

    m_bStopped = 1;
    m_bRumbleDone = 0;
    return 1;
}

// The activation twin of StopVisualEffect. It re-arms the three slot objects, starts the first of
// the three effect-id slots that names a hard-coded effect, plays the table's Shake*/Sound* columns
// for this effect's id and fires the rumble once.
void CSWCVisualEffectOnObject::StartVisualEffect()
{
    if (m_bStopped == 0)
        return;
    if (m_bFinished != 0)
        return;

    int bNoEffect = 1;
    float fParam1;
    float fParam2;
#pragma clang loop unroll(disable)
    for (int i = 0; i < 3; i++)
    {
        m_anSlotTimer[i] = 0;
        if (m_apSlotEffect[i] == 0)
            continue;

        if (m_apSlotEffect[i]->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f))
        {
            m_aSlotId[i] = STAGE_IMPACT;
            m_apSlotEffect[i]->GetAnimation(const_cast<char*>("impact"), &fParam1, &fParam2);
            m_anSlotTimer[i] = static_cast<unsigned int>(fParam2 * 1000.0f);
            bNoEffect = 0;
        }
        else if (m_bHasDuration == 1)
        {
            m_aSlotId[i] = STAGE_DURATION;
            m_apSlotEffect[i]->PlayAnimation(const_cast<char*>("duration"), 1.0f, 0, 0.0f);
            bNoEffect = 0;
        }
        else
        {
            m_aSlotId[i] = STAGE_IMPACT;
            m_anSlotTimer[i] = 1000;
            bNoEffect = 0;
        }
    }

    m_nState = STAGE_NONE;
    if (m_nEffectId0 != PROGFX_NONE)
    {
        UnloadHardCodedVisualEffect();
        LoadHardCodedVisualEffect(m_nEffectId0);
        ApplyHardCodedVisualEffect(m_nEffectId0);
        m_nState = STAGE_IMPACT;
        m_hardCodedEffectClass = GetHardCodedEffectClass(m_nEffectId0);
        m_nEffectTimer = GetHardCodedImpactLength(m_nEffectId0);
        bNoEffect = 0;
    }
    else if (m_nEffectId1 != PROGFX_NONE)
    {
        UnloadHardCodedVisualEffect();
        LoadHardCodedVisualEffect(m_nEffectId1);
        ApplyHardCodedVisualEffect(m_nEffectId1);
        m_nState = STAGE_DURATION;
        m_hardCodedEffectClass = GetHardCodedEffectClass(m_nEffectId1);
        bNoEffect = 0;
    }
    else if (m_nEffectId2 != PROGFX_NONE)
    {
        UnloadHardCodedVisualEffect();
        LoadHardCodedVisualEffect(m_nEffectId2);
        ApplyHardCodedVisualEffect(m_nEffectId2);
        m_nState = STAGE_CESSATION;
        m_hardCodedEffectClass = GetHardCodedEffectClass(m_nEffectId2);
        m_nEffectTimer = GetHardCodedCessationLength(m_nEffectId2);
        bNoEffect = 0;
    }

    if (m_nPriority == 0)
    {
        CExoString sId(m_effectId);
        float fShakeDuration;
        float fShakeDelay;
        int nShakeType;
        if (g_pRules->m_p2DArrays->m_pVisualEffect->GetINTEntry(
                sId, CTwoDimArrays::COL2DA_VISUALEFFECT_SHAKETYPE, &nShakeType) == 0)
            nShakeType = 0;
        if (g_pRules->m_p2DArrays->m_pVisualEffect->GetFLOATEntry(
                sId, CTwoDimArrays::COL2DA_VISUALEFFECT_SHAKEDURATION, &fShakeDuration) == 0)
            fShakeDuration = 0.0f;
        if (g_pRules->m_p2DArrays->m_pVisualEffect->GetFLOATEntry(
                sId, CTwoDimArrays::COL2DA_VISUALEFFECT_SHAKEDELAY, &fShakeDelay) == 0)
            fShakeDelay = 0.0f;

        if (nShakeType == 1)
            g_pAppManager->m_pClientExoApp->GetModule()->BumpCamera(
                static_cast<unsigned int>(fShakeDelay * 1000.0f));
        else if (nShakeType == 2)
            g_pAppManager->m_pClientExoApp->GetModule()->ShakeCamera(
                static_cast<unsigned int>(fShakeDelay * 1000.0f),
                static_cast<unsigned int>(fShakeDuration * 1000.0f));
    }

    CExoString sSoundImpact;
    CExoString sSoundDuration;
    CSWCObject* pTarget = GetTargetCSWCObject();
    CExoString sSoundId(m_effectId);
    if (g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
            sSoundId, CTwoDimArrays::COL2DA_VISUALEFFECT_SOUNDIMPACT, &sSoundImpact) == 0)
        sSoundImpact = "";
    if (g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
            sSoundId, CTwoDimArrays::COL2DA_VISUALEFFECT_SOUNDDURATION, &sSoundDuration) == 0)
        sSoundDuration = "";

#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (pTarget != 0 && sSoundImpact.GetLength() > 0)
#else
    if (pTarget != 0 && sSoundImpact.m_sString != 0
        && static_cast<int>(strlen(sSoundImpact.m_sString)) > 0)
#endif
        g_pExoSound->Play3DOneShotSound(CResRef(sSoundImpact), pTarget->m_position, 1.5f,
                                        PRIORITY_GROUP_NORMAL_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);

#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (sSoundDuration.GetLength() > 0)
#else
    if (sSoundDuration.m_sString != 0 && static_cast<int>(strlen(sSoundDuration.m_sString)) > 0)
#endif
    {
        m_pSoundSource = new CExoSoundSource(CResRef(sSoundDuration));
        m_pSoundSource->Set3D(1);
        m_pSoundSource->SetPriorityGroup(PRIORITY_GROUP_NORMAL_SPELL_EFFECTS);
        m_pSoundSource->SetLooping(1);
        m_pSoundSource->Play();
    }

    m_bStopped = 0;
    if (bNoEffect)
        m_bFinished = 1;

    if (m_bRumbleDone == 0)
    {
        DoVFXRumble();
        m_bRumbleDone = 1;
    }
}

// An effect is done once all three slot expiry flags and the m_bFinished latch are set.
int CSWCVisualEffectOnObject::IsDone()
{
    if (m_bEffect0Expired == 0 || m_bEffect1Expired == 0 || m_bEffect2Expired == 0 || m_bFinished == 0)
        return 0;
    return m_nState == STAGE_NONE;
}

// Kicks the controller rumble for this effect's id at the target's world position.
void CSWCVisualEffectOnObject::DoVFXRumble()
{
    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pTarget == 0)
        return;
    g_pAppManager->m_pClientExoApp->LookUpAndPerformRumbleWithCutOff(
        m_effectId, CClientExoApp::RUMBLE_TABLE_VISUAL_EFFECT, pTarget->m_position);
}

// The all-ones sentinel means no effect; every other id uses the fixed 1000-unit length, except the
// one id that asks the light/attachment object for the length of its "impact" animation.
int CSWCVisualEffectOnObject::GetHardCodedImpactLength(unsigned short nType)
{
    if (nType == PROGFX_NONE)
        return 0;
    if (m_pLight == 0)
        return 1000;
    if (nType == PROGFX_SPELL_FIZZLE_HAND)
    {
        float fParam1;
        float fParam2;
        m_pLight->GetAnimation(const_cast<char*>("impact"), &fParam1, &fParam2);
        return static_cast<unsigned int>(fParam2 * 1000.0f);
    }
    return 1000;
}

// Hangs the effect's light slot on a part of the target's model and brings it into the scene. A
// creature uses the model's "headconjure" part with no prefix, a placeable/door appends
// "_head_hit"/"hhit" to its own model name, and anything else lights the model's root. The
// attachment name follows the same split one suffix over ("_ground"/"grnd", otherwise "root").
void CSWCVisualEffectOnObject::AttachLight()
{
    Quaternion qRot;
    Vector vPos;
    CExoString sPart;
    CSWCObject* pTarget;
    Gob* pModel;
    CSWCPlaceable* pPlaceable;
    CSWCDoor* pDoor;
    CSWCCreature* pCreature;

    pTarget = GetTargetCSWCObject();
    if (pTarget == 0)
        return;
    if (m_pLight == 0)
        return;

    pModel = pTarget->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
    if (pModel == 0)
        return;

    pPlaceable = pTarget->AsSWCPlaceable();
    pDoor = pTarget->AsSWCDoor();

    if (pTarget->AsSWCCreature() != 0)
    {
        pModel->GetPartLocalPosition(const_cast<char*>("headconjure"), &vPos, &qRot);
        vPos.y += 0.75f;
        vPos.z += 0.5f;
    }
    else if (pPlaceable != 0)
    {
        CExoString sTemp;
        sTemp = pPlaceable->m_sModelName + CExoString("_head_hit");
        pModel->GetPartLocalPosition(sTemp.CStr(), &vPos, &qRot);
        vPos.y += 0.375f;
        vPos.z += 0.25f;
    }
    else if (pDoor != 0)
    {
        CExoString sTemp;
        sTemp = pDoor->m_sModelName + CExoString("hhit");
        pModel->GetPartLocalPosition(sTemp.CStr(), &vPos, &qRot);
        vPos.y += 0.375f;
        vPos.z += 0.25f;
    }
    else
    {
        vPos = Vector();
        qRot = Quaternion();
    }

    m_pLight->SetPartLocalPosition(const_cast<char*>("root"), vPos, qRot);

    if (pPlaceable != 0)
        sPart = pPlaceable->m_sModelName + CExoString("_ground");
    else if (pDoor != 0)
        sPart = pDoor->m_sModelName + CExoString("grnd");
    else
        sPart = const_cast<char*>("root");

    m_pLight->AttachToObject(static_cast<CAurObject*>(pModel), sPart.CStr(), 0);

    pCreature = pTarget->AsSWCCreature();
    if (pCreature != 0 && pCreature == g_pAppManager->m_pClientExoApp->GetPlayerCreature())
    {
        CSWCArea* pArea = pCreature->m_pArea;
        if (pArea != 0)
            pArea->UpdateShadowingLights();
    }
}

// Installs a light-source effect: picks the light's part name from the effect id, hangs it on the
// target and brings it into the scene. The id band covers the colored lights and the vision modes,
// and a second block repeats the first 33 of them; one id additionally re-points the scene's blind
// lights, but only for the player's own effect.
void CSWCVisualEffectOnObject::ApplyLightSource(unsigned short nId)
{
    int bCastShadows;
    CExoString sPart;

    CSWCObject* pTarget = GetTargetCSWCObject();
    if (pTarget == 0)
        return;
    if (m_pLight == 0)
        return;

    m_nPriority = 20;

    switch (nId)
    {
    case PROGFX_LIGHT_WHITE_5M:
    case PROGFX_LIGHT_WHITE_5M + PROGFX_LIGHT_BACKWARDS:
        sPart = "White_5m"; bCastShadows = 0; break;
    case PROGFX_LIGHT_WHITE_10M:
    case PROGFX_LIGHT_WHITE_10M + PROGFX_LIGHT_BACKWARDS:
        sPart = "White_10m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_WHITE_15M:
    case PROGFX_LIGHT_WHITE_15M + PROGFX_LIGHT_BACKWARDS:
        sPart = "White_15m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_WHITE_20M:
    case PROGFX_LIGHT_WHITE_20M + PROGFX_LIGHT_BACKWARDS:
        sPart = "White_20m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_BLUE_5M:
    case PROGFX_LIGHT_BLUE_5M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Blue_5m"; bCastShadows = 0; break;
    case PROGFX_LIGHT_BLUE_10M:
    case PROGFX_LIGHT_BLUE_10M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Blue_10m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_BLUE_15M:
    case PROGFX_LIGHT_BLUE_15M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Blue_15m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_BLUE_20M:
    case PROGFX_LIGHT_BLUE_20M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Blue_20m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_YELLOW_5M:
    case PROGFX_LIGHT_YELLOW_5M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Yello_5m"; bCastShadows = 0; break;
    case PROGFX_LIGHT_YELLOW_10M:
    case PROGFX_LIGHT_YELLOW_10M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Yello_10m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_YELLOW_15M:
    case PROGFX_LIGHT_YELLOW_15M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Yello_15m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_YELLOW_20M:
    case PROGFX_LIGHT_YELLOW_20M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Yello_20m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_RED_5M:
    case PROGFX_LIGHT_RED_5M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Red_5m"; bCastShadows = 0; break;
    case PROGFX_LIGHT_RED_10M:
    case PROGFX_LIGHT_RED_10M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Red_10m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_RED_15M:
    case PROGFX_LIGHT_RED_15M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Red_15m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_RED_20M:
    case PROGFX_LIGHT_RED_20M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Red_20m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_ORANGE_5M:
    case PROGFX_LIGHT_ORANGE_5M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Orng_5m"; bCastShadows = 0; break;
    case PROGFX_LIGHT_ORANGE_10M:
    case PROGFX_LIGHT_ORANGE_10M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Orng_10m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_ORANGE_15M:
    case PROGFX_LIGHT_ORANGE_15M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Orng_15m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_ORANGE_20M:
    case PROGFX_LIGHT_ORANGE_20M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Orng_20m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_PURPLE_5M:
    case PROGFX_LIGHT_PURPLE_5M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Purp_5m"; bCastShadows = 0; break;
    case PROGFX_LIGHT_PURPLE_10M:
    case PROGFX_LIGHT_PURPLE_10M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Purp_10m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_PURPLE_15M:
    case PROGFX_LIGHT_PURPLE_15M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Purp_15m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_PURPLE_20M:
    case PROGFX_LIGHT_PURPLE_20M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Purp_20m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_GREEN_5M:
    case PROGFX_LIGHT_GREEN_5M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Gren_5m"; bCastShadows = 0; break;
    case PROGFX_LIGHT_GREEN_10M:
    case PROGFX_LIGHT_GREEN_10M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Gren_10m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_GREEN_15M:
    case PROGFX_LIGHT_GREEN_15M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Gren_15m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_GREEN_20M:
    case PROGFX_LIGHT_GREEN_20M + PROGFX_LIGHT_BACKWARDS:
        sPart = "Gren_20m"; bCastShadows = 1; break;
    case PROGFX_LIGHT_DARK_VISION:
    case PROGFX_LIGHT_DARK_VISION + PROGFX_LIGHT_BACKWARDS:
        sPart = "Dark_Vision"; m_nPriority = 51; m_bDarkOnly = 1; bCastShadows = 0; break;
    case PROGFX_LIGHT_BLIND_VISION:
    case PROGFX_LIGHT_BLIND_VISION + PROGFX_LIGHT_BACKWARDS:
        sPart = "Blind_Vision"; m_nPriority = 53; m_bDarkOnly = 1; bCastShadows = 0; break;
    case PROGFX_LIGHT_LOW_LIGHT_VISION:
    case PROGFX_LIGHT_LOW_LIGHT_VISION + PROGFX_LIGHT_BACKWARDS:
        sPart = "Low_Light_Vision"; m_nPriority = 52; m_bDarkOnly = 1; bCastShadows = 0; break;
    case PROGFX_LIGHT_ULTRA_VISION:
    case PROGFX_LIGHT_ULTRA_VISION + PROGFX_LIGHT_BACKWARDS:
        sPart = "Ultra_Vision"; m_nPriority = 49; m_bDarkOnly = 0; bCastShadows = 0; break;
    case PROGFX_LIGHT_DARKNESS:
    case PROGFX_LIGHT_DARKNESS + PROGFX_LIGHT_BACKWARDS:
        sPart = "darkness"; m_nPriority = 50; m_bDarkOnly = 1; bCastShadows = 0; break;
    case PROGFX_LIGHT_PLAYER_BLIND_VISION:
        // SetBlindLights runs only for the player's own effect, even when the creature lookup
        // fails: a Blind_Vision on an NPC must not re-point the scene's blind lights.
        if (g_pAppManager->m_pClientExoApp != 0
            && g_pAppManager->m_pClientExoApp->GetPlayerCreature() != 0
            && m_gameObjectId == g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_objectId)
        {
            if (g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_gameObjectId) != 0)
            {
                sPart = "Blind_Vision";
                m_nPriority = 13;
                m_bDarkOnly = 0;
                bCastShadows = 0;
            }
            else
                bCastShadows = 1;
            CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
            if (pModule != 0 && pModule->m_pArea != 0)
                pModule->m_pArea->m_pScene->SetBlindLights(m_pLight);
        }
        else
            bCastShadows = 1;
        break;
    default:
        return;
    }

    AttachLight();

    m_pLight->PlayAnimation(sPart.CStr(),
                            static_cast<unsigned short>(nId - PROGFX_LIGHT_SOURCE) > PROGFX_LIGHT_BACKWARDS - 1
                                ? -0.31f : 1.0f,
                            3, 0.0f);
    if (m_pLight != 0)
        m_bCastShadows = bCastShadows;

    CSWCCreature* pCreature = pTarget->AsSWCCreature();
    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pCreature != 0 && pCreature == pPlayerCreature)
        m_pLight->SetLightPriority(2);
    else
        m_pLight->SetLightPriority(4);
}
