#include "k2stdint.h"
#include "nwcareaofeffectobject.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "gameobjectarray.h"
#include "clientoptions.h"
#include "worldtimer.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "nwcanimbase.h"
#include "nwcanimbasevisualeffect.h"
#include "nwcvisualeffect.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "exosound.h"
#include "AURORA/aurmodel.h"
#include "exostring.h"
#include "exoresman.h"
#include "aurscene.h"
#include "soundprioritygroup.h"

extern CExoSound* g_pExoSound;

#include "port/crtapi.h"
#include "port/libcstring.h"
#include "exostringinline.h"

extern int quaternionidentityoptimization;

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

// A random draw in [0, fMax), quantised to hundredths.
static inline float RandomScaled(float fMax)
{
    return static_cast<float>(rand_wincompatible() % static_cast<unsigned int>(fMax * 100.0f)) / 100.0f;
}

bool QueryPointInFrustrum(Vector& vPoint, float fRadius);

CSWCAreaOfEffectObject::CSWCAreaOfEffectObject()
    : CSWCObject(11)
{
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTimeOfDay);

    m_position    = Vector(0.0f, 0.0f, 0.0f);
    m_orientation = Vector(0.0f, 1.0f, 0.0f);
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTimeOfDay;

    m_panInstanceDelay  = 0;
    m_panInstanceTimer  = 0;
    m_panInstanceLength = 0;
    m_anBandDuration[0] = 0;
    m_anBandDuration[1] = 0;
    m_anBandDuration[2] = 0;
    m_anBandCount[0]    = 0;
    m_anBandCount[1]    = 0;
    m_anBandCount[2]    = 0;
    m_nInstances        = 0;
    m_fRadius           = 0.0f;
    m_fWidth            = 0.0f;
    m_fLength           = 0.0f;
    m_nCessationTimer   = 0;
    m_bAllInstancesDone = 0;
    m_bStopping         = 0;
    m_anBandChance[0]   = 0;
    m_anBandChance[1]   = 0;
    m_anBandChance[2]   = 0;
    m_cShape            = 0;
    m_panInstanceDone   = 0;
    m_pScene            = 0;
    m_panInstanceState  = 0;
    m_pVisualEffect     = 0;
    m_bVisualEffectAttached            = 0;
    m_pSoundSource      = 0;

    m_sHitSound         = const_cast<char*>("");
    m_nHitSoundChance   = 0;
    m_bOrientWithGround            = 0;
    m_nPersistentEffect = -1;
    m_bSoundStarted            = 0;
    m_nCreatorId        = OBJECT_INVALID;
}

CSWCAreaOfEffectObject::~CSWCAreaOfEffectObject() {
    delete m_pSoundSource;
    UnloadModel();
}


CSWCAreaOfEffectObject* CSWCAreaOfEffectObject::AsSWCAreaOfEffectObject() {
    return this;
}

int CSWCAreaOfEffectObject::LoadModel(const CResRef& r, unsigned char a, unsigned char b) {
    K2_UNUSED(r); K2_UNUSED(a); K2_UNUSED(b);
    return 0;
}

int CSWCAreaOfEffectObject::SetAnimation(unsigned short nAnimation) {
    K2_UNUSED(nAnimation);
    return 0;
}

void CSWCAreaOfEffectObject::UnloadModel(unsigned char nSubObject) {
    if (nSubObject == ANIM_MODEL_BODY)
        UnloadModel();
}

void CSWCAreaOfEffectObject::ResetTimer(int nInstance) {
    if (!m_pModel)
        return;

    int nBand;
    if (m_anBandCount[0] > nInstance) {
        nBand = 0;
    } else if (m_anBandCount[0] + m_anBandCount[1] > nInstance) {
        nBand = 1;
    } else if (m_anBandCount[0] + m_anBandCount[1] + m_anBandCount[2] > nInstance) {
        nBand = 2;
    } else {
        return;
    }

    m_panInstanceDelay[nInstance] = static_cast<unsigned int>(m_anBandDuration[nBand]);
}

void CSWCAreaOfEffectObject::AIUpdate() {
    unsigned int nDay;
    unsigned int nTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    unsigned int nDayDelta;
    unsigned int nTodDelta;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nLastAIUpdateDay, m_nLastAIUpdateTod, &nDayDelta, &nTodDelta);
    m_nAIUpdateDeltaMs = nTodDelta;

    if (m_nCreatorId != OBJECT_INVALID) {
        CGameObject* pCreator = g_pAppManager->m_pClientExoApp->GetGameObject(m_nCreatorId);
        if (pCreator)
            SetPosition(static_cast<CSWCObject*>(pCreator->AsSWCObject())->m_position);
    }

    // The row's two sound columns name a hit one-shot and a looping source; a missing column
    // leaves the resref empty.
    if (m_bSoundStarted == 0) {
        CExoString sImpact;
        CExoString sDuration;
        if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                m_nPersistentEffect, CExoString("SoundImpact"), &sImpact))
            sImpact = "";
        if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                m_nPersistentEffect, CExoString("SoundDuration"), &sDuration))
            sDuration = "";

        g_pExoSound->Play3DOneShotSound(CResRef(sImpact), m_position, 1.5f, PRIORITY_GROUP_AREA_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);

        m_pSoundSource = new CExoSoundSource(CResRef(sDuration));
        m_pSoundSource->Set3D(1);
        m_pSoundSource->SetPriorityGroup(PRIORITY_GROUP_AREA_SPELL_EFFECTS);
        m_pSoundSource->SetLooping(1);
        m_pSoundSource->SetPosition(m_position, 1.5f);
        m_pSoundSource->Play();
        m_bSoundStarted = 1;
    }

    for (unsigned char i = 0; i < m_nInstances; i++) {
        if (m_panInstanceLength[i] == 0)
            continue;

        if (m_panInstanceLength[i] > m_nAIUpdateDeltaMs) {
            m_panInstanceLength[i] -= static_cast<unsigned int>(m_nAIUpdateDeltaMs);
            continue;
        }

        m_panInstanceLength[i] = 0;
        // The animation just finished: start the stop animation, or hand the model back.
        if (m_panInstanceState[i] == 1) {
            SetAnimation(2, i);
        } else if (m_panInstanceState[i] == 3) {
            SetMoveTimer(1, static_cast<CAurObject*>(m_pModel[i].GetModel(ANIM_MODEL_BODY)));
        }
    }

    if (m_bAllInstancesDone == 0) {
        UpdateInstancePosition();
    } else if (static_cast<unsigned int>(m_nCessationTimer) > static_cast<unsigned int>(m_nAIUpdateDeltaMs)) {
        m_nCessationTimer -= static_cast<unsigned int>(m_nAIUpdateDeltaMs);
    } else {
        m_nCessationTimer = 0;
        AttachToScene(0);
        g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(m_objectId);
        delete this;
        return;
    }

    // The persisted effect joins the scene on its first frame and is then only ticked; the looping
    // source follows the AoE's own position.
    if (m_pVisualEffect) {
        if (m_bVisualEffectAttached == 0) {
            m_pVisualEffect->AttachToScene(m_pScene);
            m_bVisualEffectAttached = 1;
        }
        m_pVisualEffect->AIUpdate();
    }
    if (m_pSoundSource)
        m_pSoundSource->SetPosition(m_position, 1.5f);

    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;
}

void CSWCAreaOfEffectObject::SetMoveTimer(unsigned int nTimer, CAurObject* pObject) {
    if (!m_pModel)
        return;

    int nFound;
    for (nFound = 0; nFound < m_nInstances; nFound++) {
        if (m_pModel[nFound].GetModel(ANIM_MODEL_BODY) == pObject)
            break;
    }

    if (nFound != m_nInstances)
        m_panInstanceTimer[nFound] = nTimer;
}

void CSWCAreaOfEffectObject::UpdateInstancePosition() {
    int nFinished = 0;

    if (m_pModel) {
        for (int i = 0; i < m_nInstances; i++) {
            if (m_panInstanceDelay[i] != 0) {
                // Only a stopping instance is ticked with the frame delta.
                if (m_panInstanceState[i] == 2) {
                    if (m_panInstanceDelay[i] > static_cast<unsigned int>(m_nAIUpdateDeltaMs)) {
                        m_panInstanceDelay[i] -= static_cast<unsigned int>(m_nAIUpdateDeltaMs);
                    } else {
                        SetAnimation(3, static_cast<unsigned char>(i));
                        m_panInstanceDelay[i] = 0;
                    }
                }
            } else if (m_bStopping) {
                nFinished++;
            } else if (m_panInstanceTimer[i] != 0) {
                if (m_panInstanceTimer[i] > static_cast<unsigned int>(m_nAIUpdateDeltaMs)) {
                    m_panInstanceTimer[i] -= static_cast<unsigned int>(m_nAIUpdateDeltaMs);
                } else if (SetInstanceRandomPosition(static_cast<unsigned char>(i))) {
                    // The first move attaches the instance to the scene; later ones leave it there.
                    if (!m_panInstanceDone[i]) {
                        m_pModel[i].AttachToScene(m_pScene);
                        m_panInstanceDone[i] = 1;
                    }
                    SetAnimation(1, static_cast<unsigned char>(i));
                    ResetTimer(i);
                    m_panInstanceTimer[i] = 0;

                    // A per-hit one-shot, rolled against the 2DA's sound chance.
                    if (m_sHitSound.GetResRef()[0] != 0 &&
                        rand_wincompatible() % 100 < m_nHitSoundChance &&
                        m_pModel[i].GetModel(ANIM_MODEL_BODY))
                        g_pExoSound->Play3DOneShotSound(
                            m_sHitSound,
                            static_cast<CAurObject*>(m_pModel[i].GetModel(ANIM_MODEL_BODY))
                                ->GetPosition(),
                            1.5f, PRIORITY_GROUP_AREA_SPELL_EFFECTS, 0, 0, 0.0f, 0.0f);
                }
            }
        }
    }

    // The AoE is finished once every instance is done and the persisted effect is gone or done.
    bool bEffectDone = !m_pVisualEffect || m_pVisualEffect->m_bFinished != 0;
    if (bEffectDone && m_bStopping && nFinished == m_nInstances)
        m_bAllInstancesDone = 1;
}

// Picks a fresh spot for one instance: rolls the band, places the instance on the shape's boundary
// or inside it, rotates the offset by the object's ground orientation, and hit-checks a vertical
// ray at that spot. Returns 1 when the instance moved.
int CSWCAreaOfEffectObject::SetInstanceRandomPosition(unsigned char nInstance)
{
    if (m_pModel == 0)
        return 0;

    // Cumulative band boundaries.
    int nBand = 0;
    int nBound = 0;
    for (; nBand < 3; nBand++) {
        nBound += m_anBandCount[nBand];
        if (nInstance < nBound)
            break;
    }
    if (nBand > 2)
        return 0;

    Vector vOffset;
    if (rand_wincompatible() % 100 < m_anBandChance[nBand]) {
        if (m_cShape == 'C') {
            int nFlip = rand_wincompatible();
            int nSign = rand_wincompatible();
            int nRand = rand_wincompatible();
            float fRadius = static_cast<float>(nRand % static_cast<unsigned int>(m_fRadius * 100.0f)) / 100.0f;
            if ((nSign & 1) == 0)
                fRadius = -fRadius;
            int nSign2 = rand_wincompatible();
            float fOther = __builtin_sqrtf(m_fRadius * m_fRadius - fRadius * fRadius);
            if ((nSign2 & 1) == 0)
                fOther = -fOther;
            if (nFlip & 1) {
                vOffset.x = fRadius;
                vOffset.y = fOther;
            } else {
                vOffset.x = fOther;
                vOffset.y = fRadius;
            }
        } else if (m_cShape == 'R') {
            int nFlip = rand_wincompatible();
            int nSign = rand_wincompatible();
            int nRand = rand_wincompatible();
            if ((nFlip & 1) == 0) {
// Random along the length axis, pinned to the +/-width/2 edge.
                float fValue = static_cast<float>(nRand % static_cast<unsigned int>(m_fLength * 50.0f)) / 100.0f;
                if (nSign & 1)
                    vOffset.y = fValue;
                else
                    vOffset.y = -fValue;
                if (rand_wincompatible() & 1)
                    vOffset.x = m_fWidth * 0.5f;
                else
                    vOffset.x = m_fWidth * -0.5f;
            } else {
// Random along the width axis, pinned to the +/-length/2 edge.
                float fValue = static_cast<float>(nRand % static_cast<unsigned int>(m_fWidth * 50.0f)) / 100.0f;
                if (nSign & 1)
                    vOffset.x = fValue;
                else
                    vOffset.x = -fValue;
                if (rand_wincompatible() & 1)
                    vOffset.y = m_fLength * 0.5f;
                else
                    vOffset.y = m_fLength * -0.5f;
            }
        }
    } else {
        if (m_cShape == 'C') {
            int nFlip = rand_wincompatible();
            int nSign = rand_wincompatible();
            int nRand = rand_wincompatible();
            float fValue = static_cast<float>(nRand % static_cast<unsigned int>(m_fRadius * 100.0f)) / 100.0f;
            float fX = (nSign & 1) ? (fValue - 0.5f) : (0.5f - fValue);
            float fRemainder = __builtin_sqrtf(m_fRadius * m_fRadius - fX * fX);
            int nSign2 = rand_wincompatible();
            float fValue2 = RandomScaled(fRemainder);
            if (nFlip & 1) {
                vOffset.x = fX;
                if (nSign2 & 1)
                    vOffset.y = fValue2 - 0.5f;
                else
                    vOffset.y = 0.5f - fValue2;
            } else {
                if (nSign2 & 1)
                    vOffset.x = fValue2 - 0.5f;
                else
                    vOffset.x = 0.5f - fValue2;
                vOffset.y = fX;
            }
        } else if (m_cShape == 'R') {
            int nSignX = rand_wincompatible();
            float fValueX = static_cast<float>(rand_wincompatible() % static_cast<unsigned int>(m_fWidth * 50.0f)) / 100.0f;
            vOffset.x = ((nSignX & 1) == 0) ? (0.5f - fValueX) : (fValueX - 0.5f);
            int nSignY = rand_wincompatible();
            float fValueY = static_cast<float>(rand_wincompatible() % static_cast<unsigned int>(m_fLength * 50.0f)) / 100.0f;
            if (nSignY & 1)
                vOffset.y = fValueY - 0.5f;
            else
                vOffset.y = 0.5f - fValueY;
        }
    }

// The offset is in local space; rotate it by the object's ground orientation.
    vOffset = RotateVector(vOffset, RotationArc(Vector(0.0f, 1.0f, 0.0f), m_orientation));

    Vector vPos = m_position;
    Vector vFrom(vPos.x + vOffset.x, vPos.y + vOffset.y, vPos.z + vOffset.z + 10.0f);
    Vector vTo(vFrom.x, vFrom.y, vPos.z + vOffset.z - 10.0f);

    CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
    Vector vHit;
    CAurScene* pScene = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pScene;
    hitInfo.m_bSkipObjects = 1;
    hitInfo.m_nMaterialMask = g_pAppManager->m_pClientExoApp->GetHitCheckMaterialMask();

    if (pScene->HitCheck(vFrom, vTo, &vHit, &hitInfo)) {
        m_pModel[nInstance].SetPosition(vHit);
        if (m_bOrientWithGround)
            m_pModel[nInstance].SetOrientation(
                RotationArc(Vector(0.0f, 0.0f, 1.0f), hitInfo.m_vHitNormal));
        return 1;
    }
    return 0;
}

// Shifts the persisted effect, then every instance's model, by the same delta; m_position is
// written last so the delta is the pre-move one.
void CSWCAreaOfEffectObject::SetPosition(const Vector& v)
{
    CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
    Vector vHit;
    CAurScene* pScene = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pScene;

    if (m_pVisualEffect) {
        m_pVisualEffect->SetPosition(v);
        if (pScene && m_bOrientWithGround) {
            hitInfo.m_bSkipObjects = 1;
            hitInfo.m_nMaterialMask = g_pAppManager->m_pClientExoApp->GetHitCheckMaterialMask();
            if (pScene->HitCheck(Vector(v.x, v.y, v.z + 10.0f),
                                 Vector(v.x, v.y, v.z - 10.0f), &vHit, &hitInfo))
                m_pVisualEffect->GetAnimationBase()->SetOrientation(
                    RotationArc(Vector(0.0f, 0.0f, 1.0f), hitInfo.m_vHitNormal));
        }
    }

    if (m_pModel && m_nInstances > 0) {
        Vector vDelta(v.x - m_position.x, v.y - m_position.y, v.z - m_position.z);
        for (int i = 0; i < m_nInstances; i++) {
            CAurObject* pObject = static_cast<CAurObject*>(m_pModel[i].GetModel(ANIM_MODEL_BODY));
            if (pObject) {
                Vector vPos = pObject->GetPosition();
                Vector vNew(vPos.x + vDelta.x, vPos.y + vDelta.y, vPos.z + vDelta.z);
                pObject->SetPosition(vNew);
                if (pScene && m_bOrientWithGround) {
                    hitInfo.Reset(0, ~CHitInfo::kMaterialTrigger, 0);
                    hitInfo.m_bSkipObjects = 1;
                    hitInfo.m_nMaterialMask = g_pAppManager->m_pClientExoApp->GetHitCheckMaterialMask();
                    if (pScene->HitCheck(Vector(vNew.x, vNew.y, vNew.z + 10.0f),
                                         Vector(vNew.x, vNew.y, vNew.z - 10.0f), &vHit, &hitInfo))
                        m_pModel[i].SetOrientation(
                            RotationArc(Vector(0.0f, 0.0f, 1.0f), hitInfo.m_vHitNormal));
                }
            }
        }
    }

    m_position = v;
}

// Loads a vfx_persistent.2da row: builds the persisted visual effect and the instance bank.
// Returns 0 when the row has no usable duration-effect node, 1 once the bank is built.
int CSWCAreaOfEffectObject::LoadEffect(unsigned int nEffect) {
    CExoString sModel[3];
    CExoString sShape;
    CExoString sDurationVFX;
    float fBandWeight[3];

    int nOrientWithGround;
    if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetINTEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_ORIENTWITHGROUND,
        &nOrientWithGround))
        nOrientWithGround = 0;
    m_bOrientWithGround = nOrientWithGround != 0;

// The duration-effect field's first resolving node becomes the persisted effect's model; a row
// with no node at all is the only failure.
    if (g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
            nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_DURATIONVFX,
            &sDurationVFX)) {
        CExoString sVFXModel;
        if (!g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                sDurationVFX, g_pRules->m_p2DArrays->COL2DA_VISUALEFFECT_IMP_HEADCON_NODE, &sVFXModel) &&
            !g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                sDurationVFX, g_pRules->m_p2DArrays->COL2DA_VISUALEFFECT_IMP_IMPACT_NODE, &sVFXModel) &&
            !g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                sDurationVFX, g_pRules->m_p2DArrays->COL2DA_VISUALEFFECT_IMP_ROOT_M_NODE, &sVFXModel) &&
            !g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                sDurationVFX, g_pRules->m_p2DArrays->COL2DA_VISUALEFFECT_IMP_ROOT_S_NODE, &sVFXModel) &&
            !g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                sDurationVFX, g_pRules->m_p2DArrays->COL2DA_VISUALEFFECT_IMP_ROOT_L_NODE, &sVFXModel) &&
            !g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
                sDurationVFX, g_pRules->m_p2DArrays->COL2DA_VISUALEFFECT_IMP_ROOT_H_NODE, &sVFXModel))
            return 0;

        CSWCVisualEffect* pVFX = new CSWCVisualEffect(0);
        if (pVFX->LoadModel(CResRef(sVFXModel), ANIM_MODEL_BODY, 8)) {
            m_bVisualEffectAttached = 0;
            pVFX->m_bImpactOnly = 0;
            pVFX->m_bDeleteWhenFinished = 0;
            pVFX->SetAnimation(1);
        } else {
            delete pVFX;
            pVFX = 0;
        }
        m_pVisualEffect = pVFX;
    }

// The shape byte picks which geometry pair is live; each extent defaults to 1.0.
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_SHAPE, &sShape);
    m_cShape = sShape.CStr()[0];
    if (m_cShape == 'R') {
        if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetFLOATEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_WIDTH, &m_fWidth))
            m_fWidth = 1.0f;
        if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetFLOATEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_LENGTH, &m_fLength))
            m_fLength = 1.0f;
    } else if (m_cShape == 'C') {
        if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetFLOATEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_RADIUS, &m_fRadius))
            m_fRadius = 1.0f;
    }

// Bit 0 of m_bFlags0Hi selects the full Model01/02/03 columns; otherwise ModelMin01/02/03
// are tried first with Model01/02/03 as the fallback.
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if ((pOptions->m_bFlags0Hi & 1) == 0) {
        if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_MODELMIN01, &sModel[0]))
            g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_MODEL01, &sModel[0]);
        if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_MODELMIN02, &sModel[1]))
            g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_MODEL02, &sModel[1]);
        if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_MODELMIN03, &sModel[2]))
            g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_MODEL03, &sModel[2]);
    } else {
        g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_MODEL01, &sModel[0]);
        g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_MODEL02, &sModel[1]);
        g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
                nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_MODEL03, &sModel[2]);
    }

// NumActxx = each band's instance count (also the cumulative boundaries), Durationxx = the
// per-instance delay ResetTimer stamps.
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetINTEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_NUMACT01, &m_anBandCount[0]);
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetINTEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_NUMACT02, &m_anBandCount[1]);
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetINTEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_NUMACT03, &m_anBandCount[2]);
#ifdef K2_FIX_ENGINE_BUGS
    // A negative NumActxx would shrink the instance total below another band's own count.
    for (int i = 0; i < 3; i++) {
        if (m_anBandCount[i] < 0)
            m_anBandCount[i] = 0;
    }
#endif
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetINTEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_DURATION01,
        &m_anBandDuration[0]);
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetINTEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_DURATION02,
        &m_anBandDuration[1]);
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetINTEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_DURATION03,
        &m_anBandDuration[2]);

// EdgeWghtxx is a 0..1 per-band probability; the byte stored is it x 100, clamped to 100.
// The read results are ignored, so a missing column leaves the weights uninitialised.
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetFLOATEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_EDGEWGHT01, &fBandWeight[0]);
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetFLOATEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_EDGEWGHT02, &fBandWeight[1]);
    g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetFLOATEntry(
        nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_EDGEWGHT03, &fBandWeight[2]);

    m_nAIUpdateDeltaMs = 0;

    unsigned int nWeight = static_cast<int>(fBandWeight[0] * 100.0f);
    if (nWeight > 100)
        nWeight = 100;
    m_anBandChance[0] = static_cast<unsigned char>(nWeight);

    nWeight = static_cast<int>(fBandWeight[1] * 100.0f);
    if (nWeight > 100)
        nWeight = 100;
    m_anBandChance[1] = static_cast<unsigned char>(nWeight);

    nWeight = static_cast<int>(fBandWeight[2] * 100.0f);
    if (nWeight > 100)
        nWeight = 100;
    m_anBandChance[2] = static_cast<unsigned char>(nWeight);

    CExoString sUnusedEffectName;
    (void)sUnusedEffectName;

    CExoString sSoundOneShot;
    if (g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
            nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_SOUNDONESHOT,
            &sSoundOneShot))
        m_sHitSound = sSoundOneShot;

    float fSoundOneShotChance;
    if (!g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetFLOATEntry(
            nEffect, g_pRules->m_p2DArrays->COL2DA_PERSISTENTVISUALEFFECT_SOUNDONESHOTPERCENTAGE,
            &fSoundOneShotChance))
        fSoundOneShotChance = 0.0f;
    m_nHitSoundChance = static_cast<unsigned int>(fSoundOneShotChance * 100.0f);

    m_nInstances = m_anBandCount[0] + m_anBandCount[1] + m_anBandCount[2];
    if (m_nInstances < 1)
        return 1;

    m_pModel = new CSWCAnimBaseVisualEffect[m_nInstances];

    int nInstances = m_nInstances;
    unsigned int*   panDelay;
    unsigned int*   panTimer;
    unsigned int*   panDone;
    unsigned int*   panLength;
    unsigned short* panState;
    m_panInstanceDelay  = panDelay  = new unsigned int[nInstances];
    m_panInstanceTimer  = panTimer  = new unsigned int[nInstances];
    m_panInstanceDone   = panDone   = new unsigned int[nInstances];
    m_panInstanceLength = panLength = new unsigned int[nInstances];
    m_panInstanceState  = panState  = new unsigned short[nInstances];

    for (int i = 0; i < nInstances; i++) {
        panState[i]  = 0;
        panDone[i]   = 0;
        panTimer[i]  = 0;
        panDelay[i]  = 0;
        panLength[i] = 0;
    }

// One pass per band loads the band's model into its instances and staggers their re-randomise
// timers by the band's step.
    int nIndex = 0;
    unsigned int nTimer = 0;
    for (int i = 0; i < m_anBandCount[0]; i++) {
        K2_ASSUME(nIndex < nInstances);   // the three bands sum to the instance count
        if (m_pModel[nIndex].LoadModel(CResRef(sModel[0]), ANIM_MODEL_BODY)) {
            m_panInstanceTimer[nIndex] = nTimer;
            m_pModel[nIndex].Hilite(Vector(1.0f, 1.0f, 1.0f), 3, 0);
        }
        nIndex++;
        nTimer += 300;
    }

    nTimer = 300;
    for (int i = 0; i < m_anBandCount[1]; i++) {
        K2_ASSUME(nIndex < nInstances);   // the three bands sum to the instance count
        if (m_pModel[nIndex].LoadModel(CResRef(sModel[1]), ANIM_MODEL_BODY)) {
            m_panInstanceTimer[nIndex] = nTimer;
            m_pModel[nIndex].Hilite(Vector(1.0f, 1.0f, 1.0f), 3, 0);
        }
        nIndex++;
        nTimer += 400;
    }

    nTimer = 700;
    for (int i = 0; i < m_anBandCount[2]; i++) {
        K2_ASSUME(nIndex < nInstances);   // the three bands sum to the instance count
        if (m_pModel[nIndex].LoadModel(CResRef(sModel[2]), ANIM_MODEL_BODY)) {
            m_panInstanceTimer[nIndex] = nTimer;
            m_pModel[nIndex].Hilite(Vector(1.0f, 1.0f, 1.0f), 3, 0);
        }
        nIndex++;
        nTimer += 350;
    }

// Row 7 is the "default" persistent effect: it re-applies the containing area's env map.
    if (nEffect == 7)
        SetEnvMap(1, 1, "DEFAULT");

    m_nPersistentEffect = nEffect;
    return 1;
}

void CSWCAreaOfEffectObject::UnloadModel() {
    if (m_panInstanceLength) {
        delete[] m_panInstanceLength;
        m_panInstanceLength = 0;
    }
    if (m_panInstanceState) {
        delete[] m_panInstanceState;
        m_panInstanceState = 0;
    }
    if (m_panInstanceDone) {
        delete[] m_panInstanceDone;
        m_panInstanceDone = 0;
    }
    if (m_panInstanceTimer) {
        delete[] m_panInstanceTimer;
        m_panInstanceTimer = 0;
    }
    if (m_panInstanceDelay) {
        delete[] m_panInstanceDelay;
        m_panInstanceDelay = 0;
    }
    if (m_pModel) {
        for (int i = 0; i < m_nInstances; i++)
            m_pModel[i].UnloadModel();
        delete[] m_pModel;
        m_pModel = 0;
    }
    if (m_pVisualEffect) {
        delete m_pVisualEffect;
        m_pVisualEffect = 0;
    }
}

int CSWCAreaOfEffectObject::SetAnimation(unsigned short nAnimation, unsigned char nInstance) {
    if (!m_pModel)
        return 0;

    m_panInstanceState[nInstance] = nAnimation;

    if (nAnimation == 1) {
        unsigned int nLength = static_cast<unsigned int>(m_pModel[nInstance].GetAnimationLength(1));
        if (!nLength)
            nLength = 1;
        m_panInstanceLength[nInstance] = nLength;
        return m_pModel[nInstance].SetAnimation(nAnimation, 1.0f, 1, 0.0f);
    }
    if (nAnimation == 3) {
        unsigned int nLength = static_cast<unsigned int>(m_pModel[nInstance].GetAnimationLength(3));
        if (!nLength)
            nLength = 1;
        m_panInstanceLength[nInstance] = nLength;
        return m_pModel[nInstance].SetAnimation(nAnimation, 1.0f, 1, 0.0f);
    }

    m_panInstanceLength[nInstance] = 0;
    return m_pModel[nInstance].SetAnimation(nAnimation, 1.0f, 0, 0.0f);
}

void CSWCAreaOfEffectObject::HideClippedInstances() {
    if (!m_pModel)
        return;

    for (int i = 0; i < m_nInstances; i++) {
        CAurObject* pInstance = static_cast<CAurObject*>(m_pModel[i].GetModel(ANIM_MODEL_BODY));
        if (!pInstance)
            continue;
        if (!pInstance->GetScene())
            continue;

        Vector vInstance = pInstance->GetPosition();
        if (!QueryPointInFrustrum(vInstance, 0.5f))
            m_pModel[i].AttachToScene(0);
    }
}

void CSWCAreaOfEffectObject::StopEffect() {
    CExoString sCessationSound;
    // A row that carries a cessation sound plays the 2DA default instead.
    if (g_pRules->m_p2DArrays->m_pPersistentVisualEffect->GetCExoStringEntry(
            m_nPersistentEffect, CExoString("SoundCessation"), &sCessationSound))
        sCessationSound = "";

    g_pExoSound->Play3DOneShotSound(CResRef(sCessationSound), m_position, 1.5f, PRIORITY_GROUP_AREA_SPELL_EFFECTS, 0, 0, 0.0f,
                                    0.0f);

    if (m_pSoundSource) {
        m_pSoundSource->Stop();
        delete m_pSoundSource;
        m_pSoundSource = 0;
    }

    m_bStopping = 1;
    if (m_pVisualEffect)
        m_pVisualEffect->SetAnimation(3);

    for (int i = 0; i < m_nInstances; i++) {
        SetAnimation(3, static_cast<unsigned char>(i));
        m_panInstanceDelay[i] = 0;
    }

    m_nCessationTimer = 3000;
}

int CSWCAreaOfEffectObject::SetEnvMap(int a, int b, const CResRef& r) {
    CExoString sEnvMap;
    if (!m_pModel)
        return 0;

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (r == const_cast<char*>("DEFAULT")) {
        if (pModule && pModule->m_pArea) {
            CResRef envMap(pModule->m_pArea->m_defaultEnvMap);
            envMap.CopyToString(sEnvMap);
        }
        if (sEnvMap.GetLength() == 0)
            return 0;
    } else {
        r.CopyToString(sEnvMap);
    }

    for (int i = 0; i < m_nInstances; i++) {
        m_pModel[i].SetEnvMap(a, b, CResRef(sEnvMap));
    }
    return 1;
}

int CSWCAreaOfEffectObject::SetEnvMap(unsigned char nSubObject, int a, int b, const CResRef& r) {
    CExoString sEnvMap;
    if (nSubObject != ANIM_MODEL_BODY)
        return 0;

    if (!m_pModel)
        return 0;

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (r == const_cast<char*>("DEFAULT")) {
        if (pModule && pModule->m_pArea) {
            CResRef envMap(pModule->m_pArea->m_defaultEnvMap);
            envMap.CopyToString(sEnvMap);
        }
        if (sEnvMap.GetLength() == 0)
            return 0;
    } else {
        r.CopyToString(sEnvMap);
    }

    for (int i = 0; i < m_nInstances; i++) {
        m_pModel[i].SetEnvMap(a, b, CResRef(sEnvMap));
    }
    return 1;
}
