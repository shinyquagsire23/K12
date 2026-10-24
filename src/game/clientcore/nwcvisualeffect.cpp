#include "k2stdint.h"
#include "nwcvisualeffect.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "worldtimer.h"
#include "gameobjectarray.h"
#include "clientaimaster.h"
#include "nwcanimbase.h"
#include "AURORA/aurmodel.h"
#include "exosound.h"
#include "nwsrules.h"
#include "exoresman.h"
#include "clientoptions.h"
#include "nwcmodule.h"
#include "exolinkedlistinternal.h"
#include "aurscene.h"
#include "nwcarea.h"
#include "nwccreature.h"
#include "nwanimbase.h"
#include "nwcanimbasevisualeffect.h"
#include "Quaternion.h"
#include "soundprioritygroup.h"

// The two effect ids that turn to face the player as they move; the second also plays at 0.8
// scale. Neither game's visualeffects.2da has rows below 1001.
enum {
    EFFECT_ID_FACE_PLAYER   = 39,
    EFFECT_ID_FACE_PLAYER_2 = 40,
};

// How long a finished effect lingers before it is removed, in milliseconds.
enum {
    FINISH_DELAY_MS = 3000,
};

#if KOTOR1  // K1 streams the model through SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

extern CExoSound* g_pExoSound;

CExoLinkedList<CSWCVisualEffect> CSWCVisualEffect::m_lstCachedVFX;

CSWCVisualEffect::CSWCVisualEffect(int a) : CSWCObject(GAME_OBJECT_TYPE_AREA_OF_EFFECT) {
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);

    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTimeOfDay;
    m_bFinishing = 0;
    m_bFinished = 0;
    m_bImpactOnly = 0;
    m_bDeleteWhenFinished = 1;
    m_nFinishDelayMs = 0;
    m_nAnimationTimeLeftMs = 0;
    m_nAnimation = 0;

    if (a != 0) {
        g_pAppManager->m_pClientExoApp->GetGameObjectArray()->AddInternalObject(m_objectId, this, 0);
        g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(this);
    }

    m_pSoundSource = 0;
    m_bOrientWithGround = 0;
    m_bDetachWhenFinished = 0;
    m_bNoFinishDelay = 0;
    m_bAnimateWhilePaused = 0;
    m_nEffectId = -1;
}

CSWCVisualEffect::~CSWCVisualEffect() {
    g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(m_objectId);
    delete m_pSoundSource;
    UnloadModel();
}

void CSWCVisualEffect::AttachToScene(CAurScene* pScene) {
    if (m_pModel)
        m_pModel->AttachToScene(pScene);
}

void CSWCVisualEffect::UnloadModel(unsigned char a) {
    if (m_pModel)
        m_pModel->UnloadModel(a);
}

// Moves the effect, then orients it: two effect ids turn to face the player, and one flagged
// orient-with-ground is aligned to the surface below it.
void CSWCVisualEffect::SetPosition(const Vector& v) {
    if (m_pSoundSource != 0)
        m_pSoundSource->SetPosition(v, 1.5f);
    CSWCObject::SetPosition(v);

    Quaternion q;
    if (m_nEffectId == EFFECT_ID_FACE_PLAYER || m_nEffectId == EFFECT_ID_FACE_PLAYER_2) {
        if (g_pAppManager->m_pClientExoApp->GetPlayerCreature() != 0) {
            Vector vDir = {0.0f, 0.0f, 0.0f};
            Vector vPlayerPos = g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_position;
            vDir = vPlayerPos - v;
            if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z > 1e-05f) {
                vDir = normalize(vDir);
                q = RotationArc(Vector(0.0f, 1.0f, 0.0f), vDir);
                q = YawPitchRoll(Yaw(q), 0.0f, 0.0f);
            }
        }
        UnHilite(3);
    }

    if (m_bOrientWithGround != 0) {
        CAurScene* pScene = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pScene;
        if (pScene != 0) {
            CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
            Vector vHit = {0.0f, 0.0f, 0.0f};
            hitInfo.m_bSkipObjects = 1;
            hitInfo.m_nMaterialMask = g_pAppManager->m_pClientExoApp->GetHitCheckMaterialMask();
            if (pScene->HitCheck(Vector(v.x, v.y, v.z + 10.0f),
                                 Vector(v.x, v.y, v.z - 10.0f), &vHit, &hitInfo))
                q = RotationArc(Vector(0.0f, 0.0f, 1.0f), hitInfo.m_vHitNormal) * q;
        }
    }

    if (m_pModel != 0)
        m_pModel->SetOrientation(q);
}

#include "port/libcbase.h"

static int FormatName(char* pszBuf, uint64_t nSize, const char* pszFmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    int nResult = __vsprintf_chk(pszBuf, 0, nSize, pszFmt, ap);
    __builtin_va_end(ap);
    return nResult;
}

int CSWCVisualEffect::LoadModel(const CResRef& r, unsigned char a, unsigned char b) {
    K2_UNUSED(a); K2_UNUSED(b);
    if (m_pModel == 0)
        m_pModel = new CSWCAnimBaseVisualEffect();

    r.GetResRefStr();

    if (m_pModel->LoadModel(r, ANIM_MODEL_BODY) == 0) {
        char szMsg[256];
        FormatName(szMsg, sizeof(szMsg),
                   "CSWCVisualEffect::LoadModel: Failed to load visual effect model '%s'.",
                   r.GetResRefStr());
        return 0;
    }

    Gob* pGob = m_pModel->GetModel(ANIM_MODEL_BODY);
    if (pGob == 0)
        return 0;

    pGob->SetExternalDataType(CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT);
    pGob->SetExternalData(this);
    pGob->SetLightPriority(3);
    Hilite(Vector(1.0f, 1.0f, 1.0f), 3, 0.0f, 0);

    if (m_bAnimateWhilePaused != 0)
        pGob->EnableForceAnimations();

    return 1;
}

void CSWCVisualEffect::SetAnimateWhilePaused(int a)
{
    m_bAnimateWhilePaused = a;
    if (a == 0)
        return;

    if (m_pModel != 0 && m_pModel->GetModel(ANIM_MODEL_BODY) != 0)
        m_pModel->GetModel(ANIM_MODEL_BODY)->EnableForceAnimations();
}

void CSWCVisualEffect::Reset()
{
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);

    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTimeOfDay;
    m_bFinishing = 0;
    m_bFinished = 0;
}

void CSWCVisualEffect::UnloadCachedVFX()
{
    while (m_lstCachedVFX.m_pInternal->m_pHead != 0)
        delete static_cast<CSWCVisualEffect*>(m_lstCachedVFX.m_pInternal->RemoveHead());
}

void CSWCVisualEffect::DoVFXRumble()
{
    g_pAppManager->m_pClientExoApp->LookUpAndPerformRumbleWithCutOff(
        m_nEffectId, CClientExoApp::RUMBLE_TABLE_VISUAL_EFFECT, m_position);
}

void CSWCVisualEffect::SetAnimation(unsigned short nAnim) {
    m_nAnimationTimeLeftMs = 0;
    m_nAnimation = nAnim;

    CExoString szRow(m_nEffectId);
    float fScale = (m_nEffectId == EFFECT_ID_FACE_PLAYER_2) ? 0.8f : 1.0f;

    switch (nAnim) {
    case 1: {
        CExoString szSound;
        if (!g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                szRow, CExoString("SoundImpact"), &szSound))
            szSound = "";
        g_pExoSound->Play3DOneShotSound(CResRef(szSound), m_position, 1.5f, PRIORITY_GROUP_NORMAL_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);
        if (m_pModel) {
            m_nAnimationTimeLeftMs = static_cast<unsigned int>(m_pModel->GetAnimationLength(1));
            if (m_bAnimateWhilePaused == 0)
                m_pModel->SetAnimation(1, fScale, 1, 0.0f);
            else
                m_pModel->SetAnimation(1, fScale, 0, 0.0f);
        }
        break;
    }
    case 3: {
        CExoString szSound;
        if (!g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                szRow, CExoString("SoundCessation"), &szSound))
            szSound = "";
        g_pExoSound->Play3DOneShotSound(CResRef(szSound), m_position, 1.5f, PRIORITY_GROUP_NORMAL_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);
        if (m_pSoundSource) {
            m_pSoundSource->Stop();
            delete m_pSoundSource;
            m_pSoundSource = 0;
        }
        if (m_pModel) {
            m_nAnimationTimeLeftMs = static_cast<unsigned int>(m_pModel->GetAnimationLength(3));
            if (m_bAnimateWhilePaused == 0)
                m_pModel->SetAnimation(3, fScale, 1, 0.0f);
            else
                m_pModel->SetAnimation(3, fScale, 0, 0.0f);
        }
        break;
    }
    default: {
        CExoString szSound;
        if (!g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                szRow, CExoString("SoundDuration"), &szSound))
            szSound = "";
        m_pSoundSource = new CExoSoundSource(CResRef(szSound));
        m_pSoundSource->Set3D(1);
        m_pSoundSource->SetPriorityGroup(PRIORITY_GROUP_NORMAL_SPELL_EFFECTS);
        m_pSoundSource->SetLooping(1);
        m_pSoundSource->Play();
        if (m_pModel)
            m_pModel->SetAnimation(nAnim, fScale, 0, 0.0f);
        break;
    }
    }
}

void CSWCVisualEffect::UnloadModel() {
    if (m_pModel) {
        m_pModel->UnloadModel();
        delete m_pModel;
        m_pModel = 0;
    }
}

int CSWCVisualEffect::LoadEffect(unsigned int nEffect) {
    CExoString szResRef;
    CExoString szRow(nEffect);

    float fShakeDuration;
    float fShakeDelay;
    int nOrient;
    int nShakeType;

    if (!g_pRules->m_p2DArrays->m_pVisualEffect->GetINTEntry(
            szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_ORIENTWITHGROUND, &nOrient))
        nOrient = 0;
    m_bOrientWithGround = (nOrient != 0);

    // Unless the client options ask for full quality, the low-quality resource is tried first; the
    // impact node columns follow.
    bool bFound =
        ((g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFlags0Hi & 1) == 0
             && g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                    szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_LOWQUALITY, &szResRef))
        || g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
               szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_HEADCON_NODE, &szResRef)
        || g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
               szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_IMPACT_NODE, &szResRef)
        || g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
               szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_ROOT_M_NODE, &szResRef)
        || g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
               szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_ROOT_S_NODE, &szResRef)
        || g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
               szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_ROOT_L_NODE, &szResRef)
        || g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
               szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_IMP_ROOT_H_NODE, &szResRef);

    if (!g_pRules->m_p2DArrays->m_pVisualEffect->GetINTEntry(
            szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_SHAKETYPE, &nShakeType))
        nShakeType = 0;

    if (!g_pRules->m_p2DArrays->m_pVisualEffect->GetFLOATEntry(
            szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_SHAKEDURATION, &fShakeDuration))
        fShakeDuration = 0.0f;

    if (!g_pRules->m_p2DArrays->m_pVisualEffect->GetFLOATEntry(
            szRow, CTwoDimArrays::COL2DA_VISUALEFFECT_SHAKEDELAY, &fShakeDelay))
        fShakeDelay = 0.0f;

    if (nShakeType == 1) {
        g_pAppManager->m_pClientExoApp->GetModule()->BumpCamera(static_cast<unsigned int>(fShakeDelay * 1000.0f));
    } else if (nShakeType == 2) {
        g_pAppManager->m_pClientExoApp->GetModule()->ShakeCamera(
            static_cast<unsigned int>(fShakeDelay * 1000.0f), static_cast<unsigned int>(fShakeDuration * 1000.0f));
    } else if (nShakeType == 1) {
        g_pAppManager->m_pClientExoApp->GetModule()->BumpCamera(static_cast<unsigned int>(fShakeDelay * 1000.0f));
    }

    if (bFound && !LoadModel(CResRef(szResRef), ANIM_MODEL_BODY, 8))
        return 0;

    // EFFECT_ID_FACE_PLAYER_2 is the one SetAnimation halves the playback rate for.
    if (nEffect == EFFECT_ID_FACE_PLAYER || nEffect == EFFECT_ID_FACE_PLAYER_2)
        UnHilite(3);
    m_nEffectId = nEffect;
    return 1;
}

void CSWCVisualEffect::LoadCachedVFX() {
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Grn_M_Bone"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("GRN_M_Giblet"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("GRN_m_Rib"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("GRN_M_Thigh"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("GRN_s_Giblet"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_L_Bone"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_L_Giblet"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_L_Rib"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_L_Thigh"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_M_Bone"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_M_Giblet"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_M_Rib"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_M_Thigh"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_M_Torso"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_S_Bone"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_S_Giblet"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_S_Rib"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Red_s_Thigh"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Wht_M_Bone"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("WHT_M_head"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Wht_M_pelves"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("Wht_s_Bone"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("vwp_m_chunk_grn"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("vwp_m_chunk_red"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("vwp_M_chunk_wht"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("vwp_m_chunk_yel"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("vwp_s_chunk_red"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("vwp_s_chunk_grn"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("vwp_s_chunk_yel"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
    if (CAurObject* p = NewCAurObject(const_cast<char*>("vwp_L_chunk_red"), const_cast<char*>("cv"), 0, 0))
        m_lstCachedVFX.m_pInternal->AddTail(p);
}

// Counts the current animation down and, when it expires, either re-arms the effect (3 s) or
// deletes it.
void CSWCVisualEffect::AIUpdate() {
    if (m_bFinished)
        return;

    unsigned int nDay;
    unsigned int nTimeOfDay;
    unsigned int nElapsedDay;
    unsigned int nElapsedTime;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nLastAIUpdateDay, m_nLastAIUpdateTod, &nElapsedDay, &nElapsedTime);
    m_nAIUpdateDeltaMs = nElapsedTime;
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTimeOfDay;

    if (m_pSoundSource)
        m_pSoundSource->SetPosition(m_position, 1.5f);

    if (m_nAnimationTimeLeftMs) {
        if (static_cast<unsigned int>(m_nAnimationTimeLeftMs) > m_nAIUpdateDeltaMs) {
            m_nAnimationTimeLeftMs -= m_nAIUpdateDeltaMs;
        } else {
            unsigned short nAnim = m_nAnimation;
            m_nAnimationTimeLeftMs = 0;
            if (nAnim == 1) {
                if (m_bImpactOnly) {
                    m_nFinishDelayMs = m_bNoFinishDelay ? 0 : FINISH_DELAY_MS;
                    m_bFinishing = 1;
                } else {
                    SetAnimation(2);
                }
            } else if (nAnim == 3) {
                m_nFinishDelayMs = FINISH_DELAY_MS;
                m_bFinishing = 1;
            }
        }
    }

    if (m_bFinishing) {
        if (static_cast<unsigned int>(m_nFinishDelayMs) > m_nAIUpdateDeltaMs) {
            m_nFinishDelayMs -= m_nAIUpdateDeltaMs;
            return;
        }
        if (m_bDeleteWhenFinished) {
            delete this;
            return;
        }
        if (m_bDetachWhenFinished)
            AttachToScene(0);
        m_bFinishing = 0;
        m_bFinished = 1;
    }
}
