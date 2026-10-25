// CSWCAnimBase's leaf accessors and its m_pModel forwarding layer.

#include "k2stdint.h"
#include "nwcanimbase.h"
#include "AURORA/aurmodel.h"
#include "aurcamera.h"
#include "swgui.h"
#include "exostring.h"
#include "appmanager.h"
#include "nwcmodule.h"
#include "clientexoapp.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include <stdio.h>

#include "port/libcbase.h"

#include "port/crtapi.h"
#include "port/libcstring.h"
#include "exostringinline.h"

extern Vector zaxis;

#if KOTOR1  // K1 model streams are SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

Gob* CSWCAnimBase::GetModel(unsigned char which)
{
    if (which == ANIM_MODEL_BODY)
        return static_cast<Gob*>(m_pModel);
    return 0;
}

bool CSWCAnimBase::SetMaxMotionBlurLength(float length)
{
    K2_UNUSED(length);
    return m_pModel != 0;
}

void CSWCAnimBase::HideWieldedItems(CResRef const&, CResRef const&) {}

void CSWCAnimBase::ShowWieldedItems() {}

void CSWCAnimBase::HideHeadItem() {}

void CSWCAnimBase::ShowHeadItem() {}

void CSWCAnimBase::SetWieldLimited(int) {}

void CSWCAnimBase::UpdateSound(int) {}

void CSWCAnimBase::SetHeadItem(unsigned long long) {}

int CSWCAnimBase::GetBoundingRect(CSWGuiExtent& rExtent)
{
    return GetBoundingRect(static_cast<CAurObject*>(m_pModel), rExtent);
}

float g_fMaxLean = 5.0f;

void CSWCAnimBase::AttachToScene(CAurScene* pScene)
{
    if (m_pModel)
        m_pModel->AttachToScene(pScene);
}

int CSWCAnimBase::SetOverlayAnimation(unsigned short nAnimation, int bOverlay)
{
    if (!m_pModel)
        return 0;
    return SetOverlayAnimation(m_pModel, nAnimation, bOverlay);
}

int CSWCAnimBase::SetOverlayAnimation(CAurObject* pObject, unsigned short nAnimation, int bOverlay)
{
    CExoString sAnim;
    if (!pObject)
        return 0;
    int nFlags;
    if (bOverlay) {
        nFlags = AnimRun::kAnimOverlay;
    } else if (nAnimation == SERVER_ANIMATION_GREETING || nAnimation == SERVER_ANIMATION_LISTEN ||
               nAnimation == SERVER_ANIMATION_SALUTE || nAnimation == SERVER_ANIMATION_TALK_NORMAL ||
               nAnimation == SERVER_ANIMATION_TALK_PLEADING || nAnimation == SERVER_ANIMATION_TALK_FORCEFUL ||
               nAnimation == SERVER_ANIMATION_TALK_LAUGHING || nAnimation == SERVER_ANIMATION_TALK_SAD ||
               nAnimation == SERVER_ANIMATION_INJECT || nAnimation == 10071 ||
               nAnimation == SERVER_ANIMATION_ACTIVATE_ITEM || nAnimation == SERVER_ANIMATION_FORCE_BLOCK ||
               nAnimation == SERVER_ANIMATION_TALK_INJURED || nAnimation == SERVER_ANIMATION_LISTEN_INJURED) {
        nFlags = AnimRun::kAnimStop;
    } else {
        nFlags = AnimRun::kAnimRemove;
    }
    sAnim = GetAnimationName(nAnimation);
    float fSpeed = 1.0f;
    switch (nAnimation) {
    case SERVER_ANIMATION_LISTEN:
    case SERVER_ANIMATION_TALK_NORMAL:
    case SERVER_ANIMATION_TALK_PLEADING:
    case SERVER_ANIMATION_TALK_FORCEFUL:
    case SERVER_ANIMATION_TALK_LAUGHING:
    case SERVER_ANIMATION_TALK_SAD:
    case SERVER_ANIMATION_TALK_INJURED:
    case SERVER_ANIMATION_LISTEN_INJURED:
        break;
    case CLIENT_ANIMATION_NATURAL_WIELD_DRAW:
        nFlags |= AnimRun::kAnimOnce;
        fSpeed = -1.0f;
        break;
    default:
        nFlags |= AnimRun::kAnimOnce;
        break;
    }
    if (sAnim == "pause1")
        return 0;
    return pObject->PlayAnimation(sAnim.CStr(), fSpeed, nFlags, 0.0f);
}

int CSWCAnimBase::LoadModel(CResRef const& cResRef, unsigned char which)
{
    Quaternion qOrientation;
    Vector vPosition;
    if (which != ANIM_MODEL_BODY)
        return 0;
    char szName[20];
    cResRef.CopyToString(szName);
    m_pModel = NewCAurObject(szName, const_cast<char*>("body"), 0, 0);
    if (!m_pModel)
        return 0;
    if (m_pModel->GetPartLocalPosition(const_cast<char*>("handconjure"), &vPosition, &qOrientation) &&
        (qOrientation.x != 0.0f || qOrientation.y != 0.0f || qOrientation.z != 0.0f ||
         qOrientation.w != 1.0f)) {
        CExoString sMessage;
        sMessage.Format(const_cast<char*>("CSWCAnimBase::LoadModel(): The handconjure dummy has an orientation....It shouldn't!!  The %s model needs to be fixed or else the spell visuals will not be correct."), szName);
    }
    if (m_pModel->GetPartLocalPosition(const_cast<char*>("headconjure"), &vPosition, &qOrientation) &&
        (qOrientation.x != 0.0f || qOrientation.y != 0.0f || qOrientation.z != 0.0f ||
         qOrientation.w != 1.0f)) {
        CExoString sMessage;
        sMessage.Format(const_cast<char*>("CSWCAnimBase::LoadModel(): The headconjure dummy has an orientation....It shouldn't!!  The %s model needs to be fixed or else the spell visuals will not be correct."), szName);
    }
    m_resModelTexture = cResRef;
    m_bModelTextureDataCached = 0;
    ShowShadowBlob(0);
    return 1;
}

int CSWCAnimBase::AddAnimationFlagInternal(CAurObject* pObject, int nFlag)
{
    if (!pObject)
        return 0;
    pObject->AddAnimationFlag(nFlag);
    return 1;
}

int CSWCAnimBase::RemoveAnimationFlagInternal(CAurObject* pObject, int nFlag)
{
    if (!pObject)
        return 0;
    pObject->RemoveAnimationFlag(nFlag);
    return 1;
}

int CSWCAnimBase::SetAnimation(unsigned short nAnimation, float fSpeed, int nFlags,
                               float fStart)
{
    return SetAnimationInternal(m_pModel, nAnimation, fSpeed, nFlags, fStart);
}

int CSWCAnimBase::SetAnimationInternal(CAurObject* pObject, unsigned short nAnimation, float fSpeed,
                                       int nFlags, float fStart)
{
    CExoString sAnim = GetAnimationName(nAnimation);
    if (!pObject)
        return 0;
    if (fSpeed == 0.0f)
        fSpeed = 1.0f;
    switch (nAnimation) {
    case SERVER_ANIMATION_WALK_BACKWARD:
    case SERVER_ANIMATION_TURN1_BACKWARD:
    case SERVER_ANIMATION_TURN2_BACKWARD:
    case 10212:
    case SERVER_ANIMATION_STAND_FROM_KNEEL:
    case 10256:
    case 10258:
    case SERVER_ANIMATION_STAND_FROM_KNEEL_DOWN:
        if (fSpeed >= 0.0f)
            fSpeed = -fSpeed;
        pObject->PlayAnimation(sAnim.CStr(), fSpeed, nFlags, fStart);
        return 1;
    case SERVER_ANIMATION_GREETING:
    case SERVER_ANIMATION_LISTEN:
    case SERVER_ANIMATION_SALUTE:
    case SERVER_ANIMATION_TALK_NORMAL:
    case SERVER_ANIMATION_TALK_PLEADING:
    case SERVER_ANIMATION_TALK_FORCEFUL:
    case SERVER_ANIMATION_TALK_LAUGHING:
    case SERVER_ANIMATION_TALK_SAD:
    case SERVER_ANIMATION_INJECT:
    case 10071:
    case SERVER_ANIMATION_THROW_HIGH:
    case SERVER_ANIMATION_THROW_LOW:
    case SERVER_ANIMATION_ACTIVATE_ITEM:
    case SERVER_ANIMATION_TALK_INJURED:
    case SERVER_ANIMATION_LISTEN_INJURED:
    case SERVER_ANIMATION_DAMAGE_GENERIC:
#if !KOTOR1
    case SERVER_ANIMATION_WRIST_LAUNCH:
#endif
        SetOverlayAnimation(pObject, nAnimation, 1);
        return 1;
    default:
        pObject->PlayAnimation(sAnim.CStr(), fSpeed, nFlags, fStart);
        return 1;
    }
}

int CSWCAnimBase::SetAnimationOnSubObject(unsigned char nSubObject, unsigned short nAnimation,
                                          float fSpeed, int nFlags, float fStart)
{
    if (nSubObject != ANIM_MODEL_BODY)
        return 0;
    return SetAnimationInternal(m_pModel, nAnimation, fSpeed, nFlags, fStart);
}

int CSWCAnimBase::SetAnimationOnSubObjectInternal(CAurObject* pObject, unsigned char nSubObject,
                                                  unsigned short nAnimation, float fSpeed,
                                                  int nFlags, float fStart)
{
    if (nSubObject != ANIM_MODEL_BODY)
        return 0;
    return SetAnimationInternal(pObject, nAnimation, fSpeed, nFlags, fStart);
}

int CSWCAnimBase::SetOrientation(Quaternion const& qOrientation)
{
    if (!m_pModel)
        return 0;
    m_pModel->SetOrientation(qOrientation);
    return 1;
}

int CSWCAnimBase::SetPosition(Vector const& vPosition)
{
    if (!m_pModel)
        return 0;
    m_pModel->SetPosition(vPosition);
    return 1;
}

// Tilts the model to match the ground: the tilt axis is the unit-Z basis crossed with the
// incoming direction, the angle is the direction's complement elevation (90 - pitch), blended
// 50 % toward the current orientation and composed with a pure yaw from the facing direction.
void CSWCAnimBase::SetGroundTilt(Vector const& vTilt, Vector const& vFacing)
{
    if (!m_pModel)
        return;
    float fComplement = 90.0f - Pitch(vTilt);
    Quaternion qTilt(cross(zaxis, vTilt), fComplement * 0.017453292f);
    Quaternion qBlended = slerp(qTilt, m_quatFc, 0.5f);
    m_quatFc = qTilt;
    Quaternion qFacing = YawPitchRoll(Yaw(vFacing), 0.0f, 0.0f);
#if KOTOR1  // K1: the multiplication precedes the m_pModel read
    Quaternion qOrientation = qBlended * qFacing;
    m_pModel->SetOrientation(qOrientation);
#else
    m_pModel->SetOrientation(qBlended * qFacing);
#endif
}

void CSWCAnimBase::SetAlwaysRender(int bAlwaysRender)
{
    if (!m_pModel)
        return;
    if (bAlwaysRender)
        m_pModel->EnableAlwaysRender();
    else
        m_pModel->DisableAlwaysRender();
}

void CSWCAnimBase::SetInCutscene(int bInCutscene)
{
    if (!m_pModel)
        return;
    if (bInCutscene)
        m_pModel->EnableInCutscene();
    else
        m_pModel->DisableInCutscene();
}

// Sets one of the five hilite slots; `which` picks a colour slot and its matching flag/int pair.
int CSWCAnimBase::Hilite(Vector const& v, unsigned char which, int b)
{
    int nResult = 0;
    if (!m_pModel || which > 4)
        return nResult;
    m_hiliteColors.Color(which) = v;
    nResult = 1;
    m_anHiliteSlot[which] = nResult;
    m_anHiliteSlot[which + 5] = b;
    RefreshHilite();
    return nResult;
}

int CSWCAnimBase::UnHilite(unsigned char which)
{
    int nResult = 0;
    if (!m_pModel || which > 4)
        return nResult;
    m_anHiliteSlot[which] = 0;
    RefreshHilite();
    nResult = 1;
    return nResult;
}

int CSWCAnimBase::SetOrientation(Vector const& vOrientation)
{
    if (!m_pModel)
        return 0;
    m_pModel->SetOrientation(YawPitchRoll(Yaw(vOrientation), 0.0f, 0.0f));
    return 1;
}

Quaternion CSWCAnimBase::GetOrientation()
{
    if (!m_pModel)
        return Quaternion();
    return m_pModel->GetOrientation();
}

Vector CSWCAnimBase::GetCutsceneDummyPosition()
{
    if (m_pModel)
        return m_pModel->GetCutsceneDummyPosition();
    Vector vRet;
    return vRet;
}

// Both detach the scene and then delete the model; the selector overload only acts on its wildcard
// argument.
void CSWCAnimBase::UnloadModel()
{
    if (!m_pModel)
        return;
    AttachToScene(0);
    delete m_pModel;
    m_pModel = 0;
}

void CSWCAnimBase::UnloadModel(unsigned char which)
{
    if (which != ANIM_MODEL_BODY)
        return;
    if (!m_pModel)
        return;
    AttachToScene(0);
    delete m_pModel;
    m_pModel = 0;
}

// In the texture family, each arm copies both CResRefs into 20-byte scratch, lower-cases them and
// forwards to the model. The cached variants revisit the same pair to remember the replacement.

int CSWCAnimBase::ReplaceTexture(CResRef const& sOld, CResRef const& sNew, int bCache)
{
    if (!m_pModel)
        return 0;
    char szOld[20];
    char szNew[20];
    sOld.CopyToString(szOld);
    sNew.CopyToString(szNew);
    _strlwr(szNew);
    _strlwr(szOld);
    m_pModel->ReplaceTexture(szOld, szNew);
    if (bCache)
    {
        m_resModelTexture = sNew;
        m_bModelTextureDataCached = 0;
    }
    return 1;
}

int CSWCAnimBase::ReplaceTexture(unsigned char which, CResRef const& sOld, CResRef const& sNew,
                                 int bCache)
{
    if (!m_pModel || which != ANIM_MODEL_BODY)
        return 0;
    char szOld[20];
    char szNew[20];
    sOld.CopyToString(szOld);
    sNew.CopyToString(szNew);
    _strlwr(szNew);
    _strlwr(szOld);
    m_pModel->ReplaceTexture(szOld, szNew);
    if (bCache)
    {
        m_resModelTexture = sNew;
        m_bModelTextureDataCached = 0;
    }
    return 1;
}

// The cached overloads additionally mirror the replacement on the object: the cached resref, the
// count/offset bytes and the accompanying data table.
int CSWCAnimBase::ReplaceTexture(unsigned char which, CResRef const& sOld, CResRef const& sNew,
                                 unsigned int nCount, unsigned short* paData, int bCache,
                                 unsigned int nOffset)
{
    int nResult = 0;
    if (!m_pModel || which != ANIM_MODEL_BODY)
        return nResult;
    char szOld[20];
    char szNew[20];
    sOld.CopyToString(szOld);
    sNew.CopyToString(szNew);
    _strlwr(szOld);
    _strlwr(szNew);
    m_pModel->ReplaceTexture(szOld, szNew, nCount, paData, nOffset);
    nResult = 1;
    if (bCache)
    {
        m_resModelTexture = sNew;
        m_bModelTextureDataCached = nResult;
        m_nModelTextureCount = static_cast<unsigned char>(nCount);
        m_nModelTextureOffset = static_cast<unsigned char>(nOffset);
        memcpy(&m_nModelTextureData1, paData + nOffset, nCount * sizeof(unsigned short));
    }
    return nResult;
}

int CSWCAnimBase::ReplaceTexture(CResRef const& sOld, CResRef const& sNew, unsigned int nCount,
                                 unsigned short* paData, int bCache, unsigned int nOffset)
{
    int nResult = 0;
    if (m_pModel)
    {
        char szOld[20];
        char szNew[20];
        sOld.CopyToString(szOld);
        sNew.CopyToString(szNew);
        _strlwr(szOld);
        _strlwr(szNew);
        m_pModel->ReplaceTexture(szOld, szNew, nCount, paData, nOffset);
        nResult = 1;
        if (bCache)
        {
            m_resModelTexture = sNew;
            m_bModelTextureDataCached = nResult;
            m_nModelTextureCount = static_cast<unsigned char>(nCount);
            m_nModelTextureOffset = static_cast<unsigned char>(nOffset);
            memcpy(&m_nModelTextureData1, paData + nOffset, nCount * sizeof(unsigned short));
        }
    }
    return nResult;
}

// RestoreTexture undoes a cached ReplaceTexture: with nothing cached the model is asked to restore
// its own texture, otherwise the cached name and data are put back.
int CSWCAnimBase::RestoreTexture()
{
    if (!m_pModel)
        return 0;
    if (m_bModelTextureDataCached)
    {
        CResRef cEmpty = "";
        ReplaceTexture(ANIM_MODEL_BODY, cEmpty, m_resModelTexture, m_nModelTextureCount, reinterpret_cast<unsigned short*>(&m_nModelTextureData1), 0, m_nModelTextureOffset);
    }
    return 1;
}

int CSWCAnimBase::RestoreTexture(unsigned char which)
{
    if (!m_pModel || which != ANIM_MODEL_BODY)
        return 0;
    if (m_bModelTextureDataCached)
    {
        CResRef cEmpty = "";
        ReplaceTexture(which, cEmpty, m_resModelTexture, m_nModelTextureCount, reinterpret_cast<unsigned short*>(&m_nModelTextureData1), 0,
                       m_nModelTextureOffset);
    }
    else
        m_pModel->RestoreTexture();
    return 1;
}

// Sets the environment map to the argument's name, the cached one, or the empty string.
int CSWCAnimBase::SetEnvMap(int nIndex, int bCache, CResRef const& cResRef)
{
    if (!m_pModel)
        return 0;
    if (nIndex)
    {
        char szName[17];
        cResRef.CopyToString(szName);
        m_pModel->SetEnvironmentMap(szName);
    }
    else
        m_pModel->SetEnvironmentMap(const_cast<char*>(""));
    if (bCache)
    {
        m_resModelEnvMap = cResRef;
        m_bModelEnvMapEnabled = nIndex;
    }
    return 1;
}

int CSWCAnimBase::SetEnvMap(unsigned char which, int nIndex, int bCache, CResRef const& cResRef)
{
    if (!m_pModel || which != ANIM_MODEL_BODY)
        return 0;
    if (nIndex)
    {
        char szName[17];
        cResRef.CopyToString(szName);
        m_pModel->SetEnvironmentMap(szName);
    }
    else
        m_pModel->SetEnvironmentMap(const_cast<char*>(""));
    if (bCache)
    {
        m_resModelEnvMap = cResRef;
        m_bModelEnvMapEnabled = nIndex;
    }
    return 1;
}

int CSWCAnimBase::RestoreEnvMap()
{
    if (!m_pModel)
        return 0;
    if (m_bModelEnvMapEnabled)
    {
        char szName[17];
        m_resModelEnvMap.CopyToString(szName);
        m_pModel->SetEnvironmentMap(szName);
    }
    else
        m_pModel->SetEnvironmentMap(const_cast<char*>(""));
    return 1;
}

// Maps an animation id to the name the "Animations" 2DA holds, consulting the 10000 fallback row
// when the direct lookup is empty. Ids in the four 128-wide synthetic banks (1000, 1200, 1400,
// 1600) get a "cut<NNN>" name with the bank's own suffix appended.
CExoString CSWCAnimBase::GetAnimationName(unsigned short nAnimation)
{
    unsigned short nAnimId = ServerToClientAnimation(nAnimation);
    CExoString sResult;
    if (!g_pRules->m_p2DArrays->m_pAnimations->GetCExoStringEntry(
            nAnimId, CTwoDimArrays::COL2DA_ANIMATIONS_NAME, &sResult))
        sResult = "";

    if (sResult.GetLength() > 0)
        return sResult;

    CExoString sSuffix;
    int nBase = 0;
    if (static_cast<unsigned short>(nAnimation - 1000) < 128)
        nBase = -1000;
    else if (static_cast<unsigned short>(nAnimation - 1200) <= 127)
    {
        nBase = -1200;
        sSuffix = "w";
    }
    else if (static_cast<unsigned short>(nAnimation - 1400) <= 127)
    {
        nBase = -1400;
        sSuffix = "l";
    }
    else if (static_cast<unsigned short>(nAnimation - 1600) <= 127)
    {
        nBase = -1600;
        sSuffix = "wl";
    }

    if (nBase != 0 && static_cast<unsigned>(nBase + nAnimation + 1) != 0)
    {
        char szNumber[4];  // three digits plus the terminator
        sprintf(szNumber, "%03d", nBase + nAnimation + 1);
        CExoString sNumber(szNumber);
        sResult = CExoString("cut") + sNumber + sSuffix;
    }

    // NONE keeps its empty name instead of falling back to PAUSE's.
    bool bHasName = !sResult.IsEmpty();
    if (nAnimation != SERVER_ANIMATION_NONE && !bHasName)
    {
        unsigned short nDefaultId = ServerToClientAnimation(SERVER_ANIMATION_PAUSE);
        if (!g_pRules->m_p2DArrays->m_pAnimations->GetCExoStringEntry(
                nDefaultId, CTwoDimArrays::COL2DA_ANIMATIONS_NAME, &sResult))
            sResult = "";
#if KOTOR1  // K1 reads and discards the fallback's length
        sResult.GetLength();
#endif
    }
    return sResult;
}

// All four of the animation-name quartet fetch the name and read the model's playhead pair (start,
// end), differing only in which half they report and how they guard it.

int CSWCAnimBase::AnimationExists(unsigned short nAnimation)
{
    float fStart, fEnd;
    CExoString sName;
    sName = GetAnimationName(nAnimation);
    if (m_pModel)
    {
        m_pModel->GetAnimation(sName.CStr(), &fStart, &fEnd);
        return fEnd >= 0.0f;
    }
    return 0;
}

int CSWCAnimBase::GetOverlayAnimationPlaying(unsigned short nAnimation)
{
    if (!m_pModel)
        return 0;
    CExoString sName = GetAnimationName(nAnimation);
    float fStart, fEnd;
    m_pModel->GetAnimation(sName.CStr(), &fStart, &fEnd);
    return fStart > 0.0f;
}

int CSWCAnimBase::GetOverlayAnimation(unsigned short nAnimation)
{
    if (!m_pModel)
        return 0;
    CExoString sName = GetAnimationName(nAnimation);
    float fStart, fEnd;
    return m_pModel->GetAnimation(sName.CStr(), &fStart, &fEnd) != 0;
}

// The millisecond accessors read the same pair. The second out-param is the animation's current
// time and the third its total length, so the two differ only in which half is scaled.

float CSWCAnimBase::GetAnimationTime(unsigned short nAnimation)
{
    float fStart, fEnd;
    CExoString sName = GetAnimationName(nAnimation);
    if (m_pModel)
    {
        m_pModel->GetAnimation(sName.CStr(), &fStart, &fEnd);
        if (fStart > 0.0f)
            return fStart * 1000.0f;
    }
    return 0.0f;
}

float CSWCAnimBase::GetAnimationLength(unsigned short nAnimation)
{
    float fStart, fEnd;
    CExoString sName = GetAnimationName(nAnimation);
    if (m_pModel)
    {
        m_pModel->GetAnimation(sName.CStr(), &fStart, &fEnd);
        if (fEnd > 0.0f)
            return fEnd * 1000.0f;
    }
    return 0.0f;
}

// Re-applies the latched hilite colour (or white when nothing is latched) and pokes the module so
// the selection display updates.
void CSWCAnimBase::RefreshHilite()
{
    if (!m_pModel)
        return;
    int64_t nSlot = 0;
    while (nSlot < 5 && m_anHiliteSlot[nSlot] == 0)
        nSlot++;
    if (nSlot < 5)
    {
        m_pModel->SetIllumination(m_hiliteColors.Color(static_cast<int>(nSlot)), 1);
        m_pModel->SetColorShifting(m_hiliteColors.Color(static_cast<int>(nSlot)), m_alpha, 1);
        if (m_anHiliteSlot[nSlot + 5])
        {
            if (!g_pAppManager->m_pClientExoApp->GetModule())
                return;
            if (!g_pAppManager->m_pClientExoApp->GetModule()->m_pArea)
                return;
            g_pAppManager->m_pClientExoApp->GetModule();
            m_pModel->GetScene();
        }
    }
    else
    {
        m_pModel->SetIllumination(Vector(0.0f, 0.0f, 0.0f), 1);
        m_pModel->SetColorShifting(Vector(1.0f, 1.0f, 1.0f), m_alpha, 1);
        if (!g_pAppManager->m_pClientExoApp->GetModule())
            return;
        if (!g_pAppManager->m_pClientExoApp->GetModule()->m_pArea)
            return;
        g_pAppManager->m_pClientExoApp->GetModule();
    }
}

// Rebuilds the model's orientation from its current yaw and pitch plus the stored lean.
void CSWCAnimBase::Lean()
{
    if (!m_pModel)
        return;
    Quaternion qOrientation = m_pModel->GetOrientation();
    float fYaw = Yaw(qOrientation);
    qOrientation = m_pModel->GetOrientation();
    float fPitch = Pitch(qOrientation);
    m_pModel->SetOrientation(YawPitchRoll(fYaw, fPitch, m_fLean));
}

// Sets the colour-shift alpha; the first latched hilite slot picks the colour, otherwise white.
int CSWCAnimBase::SetAlpha(float fAlpha, unsigned char which)
{
    int nResult = 0;
    if (!m_pModel || which != ANIM_MODEL_BODY)
        return nResult;
    m_alpha = fAlpha;
    int64_t nSlot = 0;
    while (nSlot < 5 && m_anHiliteSlot[nSlot] == 0)
        nSlot++;
    if (nSlot < 5)
        m_pModel->SetColorShifting(m_hiliteColors.Color(static_cast<int>(nSlot)), fAlpha, 1);
    else
        m_pModel->SetColorShifting(Vector(1.0f, 1.0f, 1.0f), fAlpha, 1);
    return 1;
}

// Applies the projected shadow blob, a second model attached to the model's "root" part, shown and
// hidden through m_bShadowBlobVisible.
int CSWCAnimBase::ApplyShadowBlob(CExoString const& sName)
{
    int nResult = 0;
    m_pShadowBlob = NewCAurObject(static_cast<char*>(sName.CStr()), const_cast<char*>("shadowblob"), 0, 0);
    if (m_pShadowBlob)
    {
        m_pShadowBlob->TurnOffShadows();
        nResult = 1;
        if (m_pModel)
        {
            m_pShadowBlob->AttachToObject(m_pModel, const_cast<char*>("root"), 3);
            m_bShadowBlobVisible = nResult;
        }
    }
    return nResult;
}

void CSWCAnimBase::ShowShadowBlob(int bShow)
{
    if (!m_pShadowBlob)
        return;
    if (!m_pModel)
        return;
    if (!bShow && m_bShadowBlobVisible)
        return;
    m_pShadowBlob->AttachToObject(m_pModel, const_cast<char*>("root"), 3);
    m_bShadowBlobVisible = 1;
}

void CSWCAnimBase::HideShadowBlob()
{
    if (!m_pShadowBlob)
        return;
    if (!m_bShadowBlobVisible)
        return;
    m_pShadowBlob->AttachToObject(0, 0, 0);
    m_pShadowBlob->AttachToScene(0);
    m_bShadowBlobVisible = 0;
}

void CSWCAnimBase::SetShadowBlobOrientation(Quaternion const& qOrientation)
{
    Quaternion qRot;
    Vector vPos;
    if (!m_pShadowBlob)
        return;
    m_pShadowBlob->GetPartPosition(const_cast<char*>("root"), &vPos, &qRot);
    m_pShadowBlob->SetPartPosition(const_cast<char*>("root"), vPos, qOrientation);
}

// Visemes play through the model's out-of-order animation pair under the name "talk".
int CSWCAnimBase::SetAnimationVisemes(float* pfParam1, float* pfParam2, int nParam, float fParam)
{
    if (!m_pModel)
        return 0;
    if (pfParam1 != 0 && pfParam2 != 0)
        m_pModel->PlayOutOfOrderAnimation(const_cast<char*>("talk"), pfParam1, pfParam2, nParam, fParam);
    else
        m_pModel->StopOutOfOrderAnimation(const_cast<char*>("talk"));
    return 1;
}

void CSWCAnimBase::TurnOnShadows()
{
    if (m_pModel)
        m_pModel->TurnOnShadows();
}

void CSWCAnimBase::TurnOffShadows()
{
    if (m_pModel)
        m_pModel->TurnOffShadows();
}

void CSWCAnimBase::SetLean(float fLean)
{
    if (fLean < -1.0f)
        fLean = -1.0f;
    if (fLean > 1.0f)
        fLean = 1.0f;
    m_fLean = fLean * g_fMaxLean;
}

int CSWCAnimBase::EnableBumpedOutShell(const CResRef& resRef, float fScale)
{
    if (!m_pModel)
        return 0;
    char szName[17];
    resRef.CopyToString(szName);
    _strlwr(szName);
    m_pModel->EnableRenderBumpedOut(szName, fScale);
    return 1;
}

int CSWCAnimBase::DisableBumpedOutShell()
{
    if (!m_pModel)
        return 0;
    m_pModel->DisableRenderBumpedOut();
    return 1;
}

int CSWCAnimBase::SetMotionBlur(int bMotionBlur)
{
    if (!m_pModel)
        return 0;
    m_pModel->SetMotionBlurred(bMotionBlur != 0);
    return 1;
}

void CSWCAnimBase::EnableDistortion()
{
    if (m_pModel)
        m_pModel->EnableDistortion();
}

void CSWCAnimBase::DisableDistortion()
{
    if (m_pModel)
        m_pModel->DisableDistortion();
}

int CSWCAnimBase::Wield(unsigned long long, CExoString const&, unsigned char, float)
{
    return 0;
}

int CSWCAnimBase::AddAnimationFlag(int nFlag)
{
    return AddAnimationFlagInternal(m_pModel, nFlag);
}

int CSWCAnimBase::RemoveAnimationFlag(int nFlag)
{
    return RemoveAnimationFlagInternal(m_pModel, nFlag);
}

CSWCAnimBase::CSWCAnimBase()
{
    m_pModel = 0;
#if !KOTOR1
    m_bLightsaber = 0;
    m_bHasUnarmedFeat = 0;
    m_stanceId = 0;
#endif
    m_nAnimBaseType = ANIM_BASE_TYPE_PLAIN;
    m_nWeaponClass = 0;
    m_creatureType = 0;
    m_nWeaponType = 0;
    m_resModelTexture = const_cast<char*>("");
    m_bModelTextureDataCached = 0;
    m_nModelTextureCount = 0;
    m_nModelTextureOffset = 0;
    m_nModelTextureData1 = 0;
    m_nModelTextureData2 = 0;
    m_nModelTextureData3 = 0;
    for (int i = 0; i < 10; i++)
        m_anHiliteSlot[i] = 0;
    m_resModelEnvMap = const_cast<char*>("");
    m_bModelEnvMapEnabled = 0;
    m_alpha = 1.0f;
    m_specialWalkAnimation = 0;
    m_runLooksAwful = 0;
    m_nDestroyedAnimVariant = 1;
    m_pShadowBlob = 0;
    m_bShadowBlobVisible = 0;
    m_fLean = 0.0f;
    m_nPoweredHandMask = 0;
    m_quatFc = Quaternion(zaxis, 0.0f);
}

void CSWCAnimBase::RemoveShadowBlob()
{
    if (m_pShadowBlob)
    {
        m_pShadowBlob->AttachToObject(0, 0, 0);
        m_pShadowBlob->AttachToScene(0);
        delete m_pShadowBlob;
        m_pShadowBlob = 0;
        m_bShadowBlobVisible = 0;
    }
}

CSWCAnimBase::~CSWCAnimBase()
{
    RemoveShadowBlob();
    UnloadModel();
}

// Local quaternion-to-vector rotation helper; the identity case is skipped when
// quaternionidentityoptimization is on.
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

// Projects the model's rotated bounding-box centre into camera space and maps it to screen. Returns
// 2 when the centre is in front of the camera plane, 1 when it is behind or level, and 0 when there
// is no camera or no model.
char CSWCAnimBase::GetCenterPoint(Vector& v)
{
    char nRet = 0;
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    if (pCamera != 0 && m_pModel != 0)
    {
        Vector vMin;
        Vector vMax;
        Vector vPos = m_pModel->GetPosition();
        Quaternion qOrientation = m_pModel->GetOrientation();
        Vector vCamPos = pCamera->GetPosition();
        Vector vDir = -pCamera->GetOrientation().zdir();
        m_pModel->GetBoundingBox(vMin, vMax);

        Vector vCorners[8];
        vCorners[0] = vMin;
        vCorners[1] = vMax;
        vCorners[2] = Vector(vMin.x, vMin.y, vMax.z);
        vCorners[3] = Vector(vMin.x, vMax.y, vMin.z);
        vCorners[4] = Vector(vMax.x, vMin.y, vMin.z);
        vCorners[5] = Vector(vMin.x, vMax.y, vMax.z);
        vCorners[6] = Vector(vMax.x, vMin.y, vMax.z);
        vCorners[7] = Vector(vMax.x, vMax.y, vMin.z);

        float fMinX = 3.402823466e+38f;
        float fMinY = 3.402823466e+38f;
        float fMinZ = 3.402823466e+38f;
        float fMaxX = -3.402823466e+38f;
        float fMaxY = -3.402823466e+38f;
        float fMaxZ = -3.402823466e+38f;
        for (int i = 0; i < 8; i++) {
            Vector vCorner = RotateVector(vCorners[i], qOrientation);
            vCorner.x = vCorner.x + vPos.x;
            vCorner.y = vCorner.y + vPos.y;
            vCorner.z = vCorner.z + vPos.z;
            vCorners[i] = vCorner;
            fMinX = (fMinX < vCorner.x) ? fMinX : vCorner.x;
            fMinY = (fMinY < vCorner.y) ? fMinY : vCorner.y;
            fMinZ = (fMinZ < vCorner.z) ? fMinZ : vCorner.z;
            fMaxX = (fMaxX > vCorner.x) ? fMaxX : vCorner.x;
            fMaxY = (fMaxY > vCorner.y) ? fMaxY : vCorner.y;
            fMaxZ = (fMaxZ > vCorner.z) ? fMaxZ : vCorner.z;
        }

        Vector vCenter = Vector((fMaxX - fMinX) * 0.5f + fMinX, (fMaxY - fMinY) * 0.5f + fMinY,
                                (fMaxZ - fMinZ) * 0.5f + fMinZ);
        float fPoint = dot(vDir, vCenter);
        float fCam = dot(vDir, vCamPos);
        nRet = !(fPoint > fCam) ? 2 : 1;

        Vector vScreen = pCamera->MapToScreen(vCenter, true);
        CSWGuiManager* pGui = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        v.x = vScreen.x * pGui->m_nScreenWidth;
        v.y = vScreen.y * pGui->m_nScreenHeight;
    }
    return nRet;
}

// The screen-space bounding rectangle of the model's rotated bounding box. Only X and Y are
// tracked, in screen space; a corner behind the camera's near plane aborts the call (the box is
// off-screen). Returns 1 when every corner cleared the near plane.
int CSWCAnimBase::GetBoundingRect(CAurObject* pObject, CSWGuiExtent& rExtent)
{
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    if (pObject == 0 || pCamera == 0)
        return 0;

    Vector vMin;
    Vector vMax;
    Vector vPos = pObject->GetPosition();
    Quaternion qOrientation = pObject->GetOrientation();
    Vector vCamPos = pCamera->GetPosition();
    Vector vDir = -pCamera->GetOrientation().zdir();
    float fClipNear, fClipFar;
    pCamera->GetClipDist(fClipNear, fClipFar);
    pObject->GetBoundingBox(vMin, vMax);

    Vector vCorners[8];
    vCorners[0] = vMin;
    vCorners[1] = vMax;
    vCorners[2] = Vector(vMin.x, vMin.y, vMax.z);
    vCorners[3] = Vector(vMin.x, vMax.y, vMin.z);
    vCorners[4] = Vector(vMax.x, vMin.y, vMin.z);
    vCorners[5] = Vector(vMin.x, vMax.y, vMax.z);
    vCorners[6] = Vector(vMax.x, vMin.y, vMax.z);
    vCorners[7] = Vector(vMax.x, vMax.y, vMin.z);

    float fMinX = 3.402823466e+38f;
    float fMinY = 3.402823466e+38f;
    float fMaxX = -3.402823466e+38f;
    float fMaxY = -3.402823466e+38f;
    for (int i = 0; i < 8; i++) {
        vCorners[i] = RotateVector(vCorners[i], qOrientation);
        vCorners[i].x = vCorners[i].x + vPos.x;
        vCorners[i].y = vCorners[i].y + vPos.y;
        vCorners[i].z = vCorners[i].z + vPos.z;
        float fDepth = dot(vDir, vCorners[i] - vCamPos);
        if (fDepth <= fClipNear)
            return 0;
        vCorners[i] = pCamera->MapToScreen(vCorners[i], true);
        fMinX = (fMinX < vCorners[i].x) ? fMinX : vCorners[i].x;
        fMinY = (fMinY < vCorners[i].y) ? fMinY : vCorners[i].y;
        fMaxX = (fMaxX > vCorners[i].x) ? fMaxX : vCorners[i].x;
        fMaxY = (fMaxY > vCorners[i].y) ? fMaxY : vCorners[i].y;
    }

    CSWGuiManager* pGui = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    rExtent.x = static_cast<int>(fMinX * pGui->m_nScreenWidth);
    rExtent.y = static_cast<int>(fMinY * pGui->m_nScreenHeight);
    rExtent.w = static_cast<int>((fMaxX - fMinX) * pGui->m_nScreenWidth);
    rExtent.h = static_cast<int>((fMaxY - fMinY) * pGui->m_nScreenHeight);
    return 1;
}

// Maps a server animation id to the client animation id this model should play. The model's variant
// bytes choose the mapping table and select between the ambiguous ids in a case.
unsigned short CSWCAnimBase::ServerToClientAnimation(unsigned short nAnimation)
{
    unsigned short nResult = nAnimation;
    if (m_creatureType == CREATURE_TYPE_FULL) {
        switch (nAnimation) {
        case SERVER_ANIMATION_PAUSE:
        case SERVER_ANIMATION_PAUSE_ALT:
            nResult = CLIENT_ANIMATION_PAUSE_1;
            break;
        case SERVER_ANIMATION_READY:
        case SERVER_ANIMATION_READY_ALT:
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_STUN_BATON:
                nResult = CLIENT_ANIMATION_STUN_BATON_READY;
                break;
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = CLIENT_ANIMATION_SINGLE_SABER_READY;
                break;
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_READY;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_READY;
                break;
            case WEAPON_CLASS_SINGLE_BLASTER:
                nResult = CLIENT_ANIMATION_SINGLE_BLASTER_READY;
                break;
            case WEAPON_CLASS_DUAL_BLASTER:
                nResult = CLIENT_ANIMATION_DUAL_BLASTER_READY;
                break;
            case WEAPON_CLASS_RIFLE:
            case WEAPON_CLASS_HEAVY_CARBINE:
                nResult = CLIENT_ANIMATION_RIFLE_READY;
                break;
            case WEAPON_CLASS_NATURAL:
#if KOTOR1
                nResult = CLIENT_ANIMATION_NATURAL_READY;
#else
                nResult = (m_bHasUnarmedFeat == 0) ? CLIENT_ANIMATION_NATURAL_READY : CLIENT_ANIMATION_UNARMED_READY;
#endif
                break;
            }
            break;
        case SERVER_ANIMATION_WALK:
        case SERVER_ANIMATION_WALK_BACKWARD:
            nResult = CLIENT_ANIMATION_WALK_RIFLE;
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = CLIENT_ANIMATION_WALK_SINGLE_SABER;
                if (static_cast<unsigned char>(m_nPoweredHandMask) == 0)
                    nResult = CLIENT_ANIMATION_WALK;
                break;
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_WALK_STUN_BATON;
                if (static_cast<unsigned char>(m_nPoweredHandMask) == 0)
                    nResult = CLIENT_ANIMATION_WALK;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_WALK_DUAL_SABER;
                if (static_cast<unsigned char>(m_nPoweredHandMask) == 0)
                    nResult = CLIENT_ANIMATION_WALK;
                break;
            case WEAPON_CLASS_SINGLE_BLASTER:
            case WEAPON_CLASS_DUAL_BLASTER:
            case WEAPON_CLASS_NATURAL:
                nResult = CLIENT_ANIMATION_WALK;
                break;
            case WEAPON_CLASS_RIFLE:
            case WEAPON_CLASS_HEAVY_CARBINE:
                break;
            default:
                nResult = CLIENT_ANIMATION_WALK;
                break;
            }
            break;
        case SERVER_ANIMATION_RUN:
            if (m_runLooksAwful == 0) {
                nResult = CLIENT_ANIMATION_RUN_RIFLE;
                switch (m_nWeaponClass) {
                case WEAPON_CLASS_SINGLE_SABER:
                    nResult = CLIENT_ANIMATION_RUN_SINGLE_SABER;
                    if (static_cast<unsigned char>(m_nPoweredHandMask) == 0)
                        nResult = CLIENT_ANIMATION_RUN;
                    break;
                case WEAPON_CLASS_TWO_HANDED_SABER:
                    nResult = CLIENT_ANIMATION_RUN_STUN_BATON;
                    if (static_cast<unsigned char>(m_nPoweredHandMask) == 0)
                        nResult = CLIENT_ANIMATION_RUN;
                    break;
                case WEAPON_CLASS_DUAL_SABER:
                    nResult = CLIENT_ANIMATION_RUN_DUAL_SABER;
                    if (static_cast<unsigned char>(m_nPoweredHandMask) == 0)
                        nResult = CLIENT_ANIMATION_RUN;
                    break;
                case WEAPON_CLASS_SINGLE_BLASTER:
                case WEAPON_CLASS_DUAL_BLASTER:
                case WEAPON_CLASS_NATURAL:
                    nResult = CLIENT_ANIMATION_RUN;
                    break;
                case WEAPON_CLASS_RIFLE:
                case WEAPON_CLASS_HEAVY_CARBINE:
                    break;
                default:
                    nResult = CLIENT_ANIMATION_RUN;
                    break;
                }
            } else
                nResult = CLIENT_ANIMATION_WALK;
            break;
#if !KOTOR1
        case SERVER_ANIMATION_KNOCKDOWN_FROM_BEHIND:
#endif
#if !KOTOR1
        case SERVER_ANIMATION_KNOCKDOWN_FROM_FRONT:
#endif
        case SERVER_ANIMATION_FALL_FROM_BEHIND:
        case SERVER_ANIMATION_FALL_FROM_FRONT:
            nResult = CLIENT_ANIMATION_KNOCKED_DOWN;
            break;
        case SERVER_ANIMATION_DEAD:
            nResult = CLIENT_ANIMATION_DEAD_1;
            break;
        case SERVER_ANIMATION_DEAD_2:
            nResult = CLIENT_ANIMATION_DEAD_2;
            break;
        case SERVER_ANIMATION_ATTACK:
        case SERVER_ANIMATION_ATTACK_2:
            nResult = CLIENT_ANIMATION_GENERIC_ATTACK;
            break;
        case SERVER_ANIMATION_DODGE:
            nResult = CLIENT_ANIMATION_GENERIC_DODGE;
            break;
        case SERVER_ANIMATION_PARRY:
            nResult = CLIENT_ANIMATION_GENERIC_PARRY;
            break;
        case SERVER_ANIMATION_DAMAGE:
        case SERVER_ANIMATION_DAMAGE_2:
            nResult = CLIENT_ANIMATION_GENERIC_DAMAGE;
            break;
        case SERVER_ANIMATION_CONJURE_1:
            nResult = CLIENT_ANIMATION_CAST_OUT_1;
            break;
        case SERVER_ANIMATION_CONJURE_2:
            nResult = CLIENT_ANIMATION_CAST_OUT_2;
            break;
        case SERVER_ANIMATION_CAST_1:
            nResult = CLIENT_ANIMATION_CAST_OUT_LOOP_1;
            break;
        case SERVER_ANIMATION_CAST_2:
            nResult = CLIENT_ANIMATION_CAST_OUT_LOOP_2;
            break;
        case SERVER_ANIMATION_CAST_3:
            nResult = CLIENT_ANIMATION_CAST_OUT_LOOP_3;
            break;
#if !KOTOR1
        case SERVER_ANIMATION_OPEN:
            nResult = CLIENT_ANIMATION_PLACEABLE_OPEN;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_CLOSED:
            nResult = CLIENT_ANIMATION_DOOR_CLOSED;
            break;
#endif
        case SERVER_ANIMATION_SPASM:
            nResult = CLIENT_ANIMATION_SPASM;
            break;
        case SERVER_ANIMATION_TAUNT:
            nResult = CLIENT_ANIMATION_TAUNT;
            break;
        case SERVER_ANIMATION_GREETING:
            nResult = CLIENT_ANIMATION_GREETING;
            break;
        case SERVER_ANIMATION_LISTEN:
            nResult = CLIENT_ANIMATION_LISTEN;
            break;
        case SERVER_ANIMATION_MEDITATE:
        case SERVER_ANIMATION_WORSHIP:
            nResult = CLIENT_ANIMATION_MEDITATE;
            break;
        case SERVER_ANIMATION_SALUTE:
            nResult = CLIENT_ANIMATION_SALUTE;
            break;
        case SERVER_ANIMATION_BOW:
            nResult = CLIENT_ANIMATION_BOW;
            break;
        case SERVER_ANIMATION_TALK_NORMAL:
            nResult = CLIENT_ANIMATION_TALK_NORMAL;
            break;
        case SERVER_ANIMATION_TALK_PLEADING:
            nResult = CLIENT_ANIMATION_TALK_PLEADING;
            break;
        case SERVER_ANIMATION_TALK_FORCEFUL:
            nResult = CLIENT_ANIMATION_TALK_FORCEFUL;
            break;
        case SERVER_ANIMATION_TALK_LAUGHING:
            nResult = CLIENT_ANIMATION_TALK_LAUGHING;
            break;
        case SERVER_ANIMATION_TALK_SAD:
            nResult = CLIENT_ANIMATION_TALK_SAD;
            break;
        case SERVER_ANIMATION_VICTORY:
            nResult = CLIENT_ANIMATION_VICTORY;
            break;
        case SERVER_ANIMATION_PAUSE2:
            nResult = CLIENT_ANIMATION_PAUSE_2;
            break;
        case SERVER_ANIMATION_HEAD_TURN_LEFT:
            nResult = CLIENT_ANIMATION_HEAD_TURN_LEFT;
            break;
        case SERVER_ANIMATION_HEAD_TURN_RIGHT:
            nResult = CLIENT_ANIMATION_HEAD_TURN_RIGHT;
            break;
        case SERVER_ANIMATION_PAUSE_SCRATCH_HEAD:
            nResult = CLIENT_ANIMATION_PAUSE_SCRATCH_HEAD;
            break;
        case SERVER_ANIMATION_PAUSE_BORED:
            nResult = CLIENT_ANIMATION_PAUSE_BORED;
            break;
        case SERVER_ANIMATION_PAUSE_TIRED:
            nResult = CLIENT_ANIMATION_PAUSE_TIRED;
            break;
        case SERVER_ANIMATION_PAUSE_DRUNK:
            nResult = CLIENT_ANIMATION_PAUSE_POISONED;
            break;
        case SERVER_ANIMATION_GET_LOW:
            nResult = CLIENT_ANIMATION_GET_FROM_GROUND;
            break;
        case SERVER_ANIMATION_GET_MID:
            nResult = CLIENT_ANIMATION_GET_FROM_TABLE;
            break;
        case SERVER_ANIMATION_THROW_SABER_LOOP:
            nResult = CLIENT_ANIMATION_THROW_SABER_LOOP;
            break;
        case SERVER_ANIMATION_INJECT:
            nResult = CLIENT_ANIMATION_INJECT;
            break;
        case SERVER_ANIMATION_PAUSE_INJURED:
            nResult = CLIENT_ANIMATION_PAUSE_INJURED;
            break;
        case SERVER_ANIMATION_WALK_INJURED:
            nResult = CLIENT_ANIMATION_WALK_INJURED;
            break;
        case SERVER_ANIMATION_RUN_INJURED:
            nResult = (m_runLooksAwful == 0) ? CLIENT_ANIMATION_RUN_INJURED : CLIENT_ANIMATION_WALK_INJURED;
            break;
        case SERVER_ANIMATION_WHIRLWIND:
            nResult = CLIENT_ANIMATION_WHIRLWIND;
            break;
        case SERVER_ANIMATION_DEACTIVATE:
            nResult = CLIENT_ANIMATION_DROID_DISABLED;
            break;
        case SERVER_ANIMATION_FLIRT:
            nResult = CLIENT_ANIMATION_FLIRT;
            break;
        case SERVER_ANIMATION_USE_COMPUTER_LOOP:
            nResult = CLIENT_ANIMATION_USE_COMPUTER_LOOP;
            break;
        case SERVER_ANIMATION_DANCE:
            nResult = CLIENT_ANIMATION_DANCE_1;
            break;
        case SERVER_ANIMATION_DANCE1:
            nResult = CLIENT_ANIMATION_DANCE_2;
            break;
        case SERVER_ANIMATION_HORROR:
            nResult = CLIENT_ANIMATION_HORROR;
            break;
        case SERVER_ANIMATION_USE_COMPUTER:
            nResult = CLIENT_ANIMATION_USE_COMPUTER;
            break;
        case SERVER_ANIMATION_PERSUADE:
            nResult = CLIENT_ANIMATION_PERSUADE;
            break;
        case SERVER_ANIMATION_ACTIVATE:
        case SERVER_ANIMATION_ACTIVATE_ITEM:
            nResult = CLIENT_ANIMATION_ACTIVATE;
            break;
        case SERVER_ANIMATION_UNLOCK_DOOR:
        case SERVER_ANIMATION_DISARM_DOOR_MINE:
            nResult = CLIENT_ANIMATION_UNLOCK_DOOR;
            break;
        case SERVER_ANIMATION_THROW_HIGH:
            nResult = CLIENT_ANIMATION_THROW_GRENADE_1;
            break;
        case SERVER_ANIMATION_THROW_LOW:
            nResult = CLIENT_ANIMATION_THROW_GRENADE_2;
            break;
        case SERVER_ANIMATION_UNLOCK_CONTAINER:
        case SERVER_ANIMATION_DISARM_CONTAINER_MINE:
            nResult = CLIENT_ANIMATION_UNLOCK_CONTAINER;
            break;
        case SERVER_ANIMATION_DISARM_MINE:
        case SERVER_ANIMATION_RECOVER_MINE:
#if !KOTOR1
        case SERVER_ANIMATION_CHECK_BODY:
#endif
            nResult = CLIENT_ANIMATION_DISABLE_MINE;
            break;
        case SERVER_ANIMATION_STEALTH:
            nResult = CLIENT_ANIMATION_WALK_STEALTH;
            break;
        case SERVER_ANIMATION_SLEEP:
            nResult = CLIENT_ANIMATION_SLEEP;
            break;
        case SERVER_ANIMATION_PARALYZED:
            nResult = CLIENT_ANIMATION_PARALYZED;
            break;
        case SERVER_ANIMATION_PRONE:
            nResult = CLIENT_ANIMATION_PRONE;
            break;
        case SERVER_ANIMATION_SET_MINE:
            nResult = CLIENT_ANIMATION_SET_MINE;
            break;
        case SERVER_ANIMATION_CUSTOM01:
            nResult = CLIENT_ANIMATION_CUSTOM01;
            break;
        case SERVER_ANIMATION_FORCE_BLOCK:
            nResult = CLIENT_ANIMATION_FORCE_BLOCK;
            break;
        case SERVER_ANIMATION_PAUSE_FIDGET:
            nResult = CLIENT_ANIMATION_PAUSE_FIDGET;
            break;
        case SERVER_ANIMATION_CHOKE:
            nResult = CLIENT_ANIMATION_CHOKE;
            break;
        case SERVER_ANIMATION_PAUSE3:
            nResult = CLIENT_ANIMATION_PAUSE_3;
            break;
        case SERVER_ANIMATION_WELD:
            nResult = CLIENT_ANIMATION_WELD;
            break;
        case SERVER_ANIMATION_TALK_INJURED:
            nResult = CLIENT_ANIMATION_TALK_INJURED;
            break;
        case SERVER_ANIMATION_LISTEN_INJURED:
            nResult = CLIENT_ANIMATION_LISTEN_INJURED;
            break;
        case SERVER_ANIMATION_DEAD_PRONE:
            nResult = CLIENT_ANIMATION_DEAD_PRONE;
            break;
        case SERVER_ANIMATION_WIELD_SWAP:
            nResult = SERVER_ANIMATION_WIELD_SWAP;
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = CLIENT_ANIMATION_SINGLE_SABER_WIELD_SWAP;
                break;
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_WIELD_SWAP;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_WIELD_SWAP;
                break;
#if !KOTOR1  // K1: no ranged arms
            case WEAPON_CLASS_SINGLE_BLASTER:
                nResult = CLIENT_ANIMATION_SINGLE_BLASTER_WIELD_SWAP;
                break;
            case WEAPON_CLASS_DUAL_BLASTER:
                nResult = CLIENT_ANIMATION_DUAL_BLASTER_WIELD_SWAP;
                break;
            case WEAPON_CLASS_RIFLE:
                nResult = CLIENT_ANIMATION_RIFLE_WIELD_SWAP;
                break;
            case WEAPON_CLASS_HEAVY_CARBINE:
                nResult = CLIENT_ANIMATION_HEAVY_CARBINE_WIELD_SWAP;
                break;
#endif
            }
            break;
        case SERVER_ANIMATION_WIELD_DRAW:
            nResult = SERVER_ANIMATION_WIELD_DRAW;
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = CLIENT_ANIMATION_SINGLE_SABER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_WIELD_DRAW;
                break;
#if !KOTOR1
            case WEAPON_CLASS_SINGLE_BLASTER:
                nResult = CLIENT_ANIMATION_SINGLE_BLASTER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_DUAL_BLASTER:
                nResult = CLIENT_ANIMATION_DUAL_BLASTER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_RIFLE:
            case WEAPON_CLASS_HEAVY_CARBINE:
                nResult = CLIENT_ANIMATION_RIFLE_WIELD_DRAW;
                break;
#endif
            }
            break;
        case SERVER_ANIMATION_TREAT_INJURED:
            nResult = CLIENT_ANIMATION_TREAT_INJURED;
            break;
        case SERVER_ANIMATION_TREAT_INJURED_LOOP:
            nResult = CLIENT_ANIMATION_TREAT_INJURED_LOOP;
            break;
        case SERVER_ANIMATION_CATCH_SABER:
            nResult = CLIENT_ANIMATION_CATCH_SABER;
            break;
        case SERVER_ANIMATION_THROW_SABER:
            nResult = CLIENT_ANIMATION_THROW_SABER;
            break;
        case SERVER_ANIMATION_KNEEL_TALK_ANGRY:
            nResult = CLIENT_ANIMATION_KNEEL_TALK_ANGRY;
            break;
        case SERVER_ANIMATION_KNEEL_TALK_SAD:
            nResult = CLIENT_ANIMATION_KNEEL_TALK_SAD;
            break;
        case SERVER_ANIMATION_DIE:
            nResult = CLIENT_ANIMATION_DIE_1;
            break;
        case SERVER_ANIMATION_DIE_2:
            nResult = CLIENT_ANIMATION_DIE_2;
            break;
        case SERVER_ANIMATION_GET_UP_FROM_DEAD:
            nResult = CLIENT_ANIMATION_GET_UP_FROM_DEAD_1;
            break;
        case SERVER_ANIMATION_GET_UP_FROM_DEAD_2:
            nResult = CLIENT_ANIMATION_GET_UP_FROM_DEAD_2;
            break;
        case SERVER_ANIMATION_KNEEL:
        case SERVER_ANIMATION_STAND_FROM_KNEEL:
            nResult = CLIENT_ANIMATION_KNEEL;
            break;
        case SERVER_ANIMATION_DRAW_WEAPON:
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_STUN_BATON:
                nResult = CLIENT_ANIMATION_STUN_BATON_WIELD_DRAW;
                break;
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = CLIENT_ANIMATION_SINGLE_SABER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_SINGLE_BLASTER:
                nResult = CLIENT_ANIMATION_SINGLE_BLASTER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_DUAL_BLASTER:
                nResult = CLIENT_ANIMATION_DUAL_BLASTER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_RIFLE:
            case WEAPON_CLASS_HEAVY_CARBINE:
                nResult = CLIENT_ANIMATION_RIFLE_WIELD_DRAW;
                break;
            case WEAPON_CLASS_NATURAL:
#if KOTOR1
                nResult = CLIENT_ANIMATION_NATURAL_WIELD_DRAW;
#else
                nResult = (m_bHasUnarmedFeat == 0) ? CLIENT_ANIMATION_NATURAL_WIELD_DRAW : CLIENT_ANIMATION_UNARMED_WIELD_DRAW;
#endif
                break;
            default:
                nResult = SERVER_ANIMATION_DRAW_WEAPON;
                break;
            }
            break;
        case SERVER_ANIMATION_STAND_FROM_KNEEL_DOWN:
            nResult = CLIENT_ANIMATION_KNEEL_DOWN;
            break;
#if KOTOR1  // K1: lightsaber arms are unconditional and there is no feat arm
        case SERVER_ANIMATION_DEFLECT_1:
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = CLIENT_ANIMATION_SINGLE_SABER_DEFLECT_1;
                break;
#ifdef K2_FIX_ENGINE_BUGS
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_DEFLECT_1;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_DEFLECT_1;
                break;
#else
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_DEFLECT_2;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_DEFLECT_2;
                break;
#endif
            }
            break;
        case SERVER_ANIMATION_DEFLECT_2:
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = CLIENT_ANIMATION_SINGLE_SABER_DEFLECT_2;
                break;
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_DEFLECT_2;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_DEFLECT_2;
                break;
            }
            break;
#else
        case SERVER_ANIMATION_DEFLECT_1:
            nResult = CLIENT_ANIMATION_HAND_DEFLECT;
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = (m_bLightsaber == 0) ? CLIENT_ANIMATION_HAND_DEFLECT : CLIENT_ANIMATION_SINGLE_SABER_DEFLECT_1;
                break;
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = (m_bLightsaber == 0) ? CLIENT_ANIMATION_HAND_DEFLECT : CLIENT_ANIMATION_TWO_HANDED_SABER_DEFLECT_1;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = (m_bLightsaber == 0) ? CLIENT_ANIMATION_HAND_DEFLECT : CLIENT_ANIMATION_DUAL_SABER_DEFLECT_1;
                break;
            case WEAPON_CLASS_NATURAL:
                nResult = (m_bHasUnarmedFeat == 0) ? CLIENT_ANIMATION_HAND_DEFLECT : CLIENT_ANIMATION_UNARMED_DEFLECT_1;
                break;
            }
            break;
        case SERVER_ANIMATION_DEFLECT_2:
            nResult = CLIENT_ANIMATION_HAND_DEFLECT;
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = (m_bLightsaber == 0) ? CLIENT_ANIMATION_HAND_DEFLECT : CLIENT_ANIMATION_SINGLE_SABER_DEFLECT_2;
                break;
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = (m_bLightsaber == 0) ? CLIENT_ANIMATION_HAND_DEFLECT : CLIENT_ANIMATION_TWO_HANDED_SABER_DEFLECT_2;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = (m_bLightsaber == 0) ? CLIENT_ANIMATION_HAND_DEFLECT : CLIENT_ANIMATION_DUAL_SABER_DEFLECT_2;
                break;
            case WEAPON_CLASS_NATURAL:
                nResult = (m_bHasUnarmedFeat == 0) ? CLIENT_ANIMATION_HAND_DEFLECT : CLIENT_ANIMATION_UNARMED_DEFLECT_2;
                break;
            }
            break;
#endif
        case SERVER_ANIMATION_DAMAGE_GENERIC:
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = CLIENT_ANIMATION_SINGLE_SABER_DAMAGE;
                break;
#ifdef K2_FIX_ENGINE_BUGS
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_DAMAGE;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_DAMAGE;
                break;
#else
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_DAMAGE;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_DAMAGE;
                break;
#endif
            case WEAPON_CLASS_SINGLE_BLASTER:
                nResult = CLIENT_ANIMATION_SINGLE_BLASTER_DAMAGE;
                break;
            case WEAPON_CLASS_DUAL_BLASTER:
                nResult = CLIENT_ANIMATION_DUAL_BLASTER_DAMAGE;
                break;
            case WEAPON_CLASS_RIFLE:
            case WEAPON_CLASS_HEAVY_CARBINE:
                nResult = CLIENT_ANIMATION_RIFLE_DAMAGE;
                break;
            case WEAPON_CLASS_NATURAL:
#if KOTOR1
                nResult = CLIENT_ANIMATION_MONSTER_DAMAGE;
#else
                nResult = (m_bHasUnarmedFeat == 0) ? CLIENT_ANIMATION_MONSTER_DAMAGE : CLIENT_ANIMATION_UNARMED_DAMAGE;
#endif
                break;
            default:
                nResult = SERVER_ANIMATION_DAMAGE_GENERIC;
                break;
            }
            break;
        case SERVER_ANIMATION_KNOCKED_DOWN_LOOP:
        case SERVER_ANIMATION_KNOCKED_DOWN_LOOP_2:
            nResult = CLIENT_ANIMATION_KNOCKED_DOWN_LOOP;
            break;
        case SERVER_ANIMATION_POWER_ATTACK:
            nResult = CLIENT_ANIMATION_SINGLE_SABER_POWER_ATTACK_1;
            break;
#if !KOTOR1
        case SERVER_ANIMATION_TOUCH_HEART:
            nResult = CLIENT_ANIMATION_TOUCH_HEART;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_ROLL_EYES:
            nResult = CLIENT_ANIMATION_ROLL_EYES;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_USE_ITEM_ON_OTHER:
            nResult = CLIENT_ANIMATION_USE_ITEM_ON_OTHER;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_STAND_AT_ATTENTION:
            nResult = CLIENT_ANIMATION_STAND_AT_ATTENTION;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_NOD_YES:
            nResult = CLIENT_ANIMATION_NOD_YES;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_NOD_NO:
            nResult = CLIENT_ANIMATION_NOD_NO;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_POINT:
            nResult = CLIENT_ANIMATION_POINT;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_POINT_LOOP:
            nResult = CLIENT_ANIMATION_POINT_LOOP;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_POINT_DOWN:
            nResult = CLIENT_ANIMATION_POINT_DOWN;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_SCANNING:
            nResult = CLIENT_ANIMATION_SCANNING;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_SHRUG:
            nResult = CLIENT_ANIMATION_SHRUG;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_FORCE_RAGE:
            nResult = CLIENT_ANIMATION_FORCE_RAGE;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_FORCE_CRUSH:
            nResult = CLIENT_ANIMATION_FORCE_CRUSH;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_FORCE_CRUSH_DAMAGE:
            nResult = CLIENT_ANIMATION_FORCE_CRUSH_DAMAGE;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_WRIST_LAUNCH:
            nResult = CLIENT_ANIMATION_WRIST_LAUNCH;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_SIT_AND_MEDITATE:
            nResult = CLIENT_ANIMATION_MEDITATE_SITTING;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_MEDITATE_STAND:
            nResult = CLIENT_ANIMATION_MEDITATE_STANDING;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_SIT_CHAIR:
            nResult = CLIENT_ANIMATION_PLACEABLE_ANIMLOOP01;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_SIT_CHAIR_DRINK:
            nResult = CLIENT_ANIMATION_PLACEABLE_ANIMLOOP02;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_SIT_CHAIR_PAZAK:
            nResult = CLIENT_ANIMATION_PLACEABLE_ANIMLOOP03;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_SIT_CHAIR_COMP1:
            nResult = CLIENT_ANIMATION_PLACEABLE_ANIMLOOP04;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_SIT_CHAIR_COMP2:
            nResult = CLIENT_ANIMATION_PLACEABLE_ANIMLOOP05;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_DOOR_SABER:
            if (static_cast<unsigned>(m_nWeaponClass - WEAPON_CLASS_SINGLE_SABER) < 3)  // the three sabers
                nResult = static_cast<unsigned char>(m_nWeaponClass - WEAPON_CLASS_SINGLE_SABER) +
                          CLIENT_ANIMATION_SINGLE_SABER_BURN_DOOR;
            else
                nResult = SERVER_ANIMATION_DOOR_SABER;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_CUT_HAND:
            nResult = CLIENT_ANIMATION_CUT_HAND;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_REACT_HAND_CUT_OFF:
            nResult = CLIENT_ANIMATION_REACT_HAND_CUT_OFF;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_COLLAPSE_LOOP:
            nResult = CLIENT_ANIMATION_COLLAPSE_LOOP;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_COLLAPSE_STAND:
            nResult = CLIENT_ANIMATION_COLLAPSE_STAND;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_POWER_PUNCH:
            nResult = CLIENT_ANIMATION_POWER_PUNCH;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_RAISE_ARM_TO_POINT:
            nResult = CLIENT_ANIMATION_RAISE_ARM_TO_POINT;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_LOWER_ARM_FROM_POINT:
            nResult = CLIENT_ANIMATION_LOWER_ARM_FROM_POINT;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_LOWER_HOOD:
            nResult = CLIENT_ANIMATION_LOWER_HOOD;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_RAISE_HOOD:
            nResult = CLIENT_ANIMATION_RAISE_HOOD;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_DIVE_ROLL:
            nResult = CLIENT_ANIMATION_DIVE_ROLL;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_TALK_PRONE:
            nResult = CLIENT_ANIMATION_TALK_PRONE;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_TALK_DIE_PRONE:
            nResult = CLIENT_ANIMATION_TALK_DIE_PRONE;
            break;
#endif
        case SERVER_ANIMATION_CONJURE_3:
            nResult = CLIENT_ANIMATION_CAST_OUT_3;
            break;
        case SERVER_ANIMATION_DROID_UTILITY_CONJURE:
#if KOTOR1
            nResult = CLIENT_ANIMATION_DROID_UTILITY_CONJURE;
#else
            nResult = CLIENT_ANIMATION_DROID_UTILITY_CONJURE;
#endif
            break;
        case SERVER_ANIMATION_DROID_UTILITY_CAST:
#if KOTOR1
            nResult = CLIENT_ANIMATION_DROID_UTILITY_CAST;
#else
            nResult = CLIENT_ANIMATION_DROID_UTILITY_CAST;
#endif
            break;
        }
    } else {
        switch (nAnimation) {
        case SERVER_ANIMATION_PAUSE:
        case SERVER_ANIMATION_PAUSE_SCRATCH_HEAD:
        case SERVER_ANIMATION_PAUSE_BORED:
        case SERVER_ANIMATION_PAUSE_TIRED:
        case SERVER_ANIMATION_PAUSE_DRUNK:
        case SERVER_ANIMATION_PAUSE_INJURED:
        case SERVER_ANIMATION_PAUSE_ALT:
            nResult = CLIENT_ANIMATION_MONSTER_PAUSE_1;
            break;
        case SERVER_ANIMATION_READY:
        case SERVER_ANIMATION_READY_ALT:
            nResult = CLIENT_ANIMATION_MONSTER_READY;
            break;
        case SERVER_ANIMATION_WALK:
        case SERVER_ANIMATION_WALK_BACKWARD:
        case SERVER_ANIMATION_WALK_STRAFE_LEFT:
        case SERVER_ANIMATION_STEALTH:
            nResult = CLIENT_ANIMATION_MONSTER_WALK;
            break;
        case SERVER_ANIMATION_RUN:
        case SERVER_ANIMATION_RUN_INJURED:
            nResult = (m_runLooksAwful == 0) ? CLIENT_ANIMATION_MONSTER_RUN : CLIENT_ANIMATION_MONSTER_WALK;
            break;
        case SERVER_ANIMATION_DEAD:
        case SERVER_ANIMATION_DEAD_2:
        case SERVER_ANIMATION_DEAD_PRONE:
            nResult = CLIENT_ANIMATION_MONSTER_DEAD;
            break;
        case SERVER_ANIMATION_ATTACK:
        case SERVER_ANIMATION_ATTACK_2:
            nResult = CLIENT_ANIMATION_GENERIC_ATTACK;
            break;
        case SERVER_ANIMATION_DODGE:
            nResult = CLIENT_ANIMATION_GENERIC_DODGE;
            break;
        case SERVER_ANIMATION_PARRY:
            nResult = CLIENT_ANIMATION_GENERIC_PARRY;
            break;
        case SERVER_ANIMATION_DAMAGE:
        case SERVER_ANIMATION_DAMAGE_2:
        case SERVER_ANIMATION_DAMAGE_GENERIC:
            nResult = CLIENT_ANIMATION_MONSTER_DAMAGE;
            break;
        case SERVER_ANIMATION_CONJURE_1:
        case SERVER_ANIMATION_CONJURE_2:
            nResult = CLIENT_ANIMATION_MONSTER_CONJURE;
            break;
#if !KOTOR1
        case SERVER_ANIMATION_OPEN:
            nResult = CLIENT_ANIMATION_PLACEABLE_OPEN;
            break;
#endif
#if !KOTOR1
        case SERVER_ANIMATION_CLOSED:
            nResult = CLIENT_ANIMATION_DOOR_CLOSED;
            break;
#endif
        case SERVER_ANIMATION_SPASM:
            nResult = CLIENT_ANIMATION_MONSTER_SPASM;
            break;
        case SERVER_ANIMATION_TAUNT:
            nResult = CLIENT_ANIMATION_MONSTER_TAUNT;
            break;
        case SERVER_ANIMATION_LISTEN:
        case SERVER_ANIMATION_LISTEN_INJURED:
            nResult = CLIENT_ANIMATION_LISTEN;
            break;
        case SERVER_ANIMATION_TALK_NORMAL:
        case SERVER_ANIMATION_TALK_PLEADING:
        case SERVER_ANIMATION_TALK_FORCEFUL:
        case SERVER_ANIMATION_TALK_LAUGHING:
        case SERVER_ANIMATION_TALK_SAD:
            nResult = CLIENT_ANIMATION_MONSTER_TALK;
            break;
        case SERVER_ANIMATION_VICTORY:
            nResult = CLIENT_ANIMATION_MONSTER_VICTORY;
            break;
        case SERVER_ANIMATION_PAUSE2:
            nResult = CLIENT_ANIMATION_MONSTER_PAUSE_2;
            break;
        case SERVER_ANIMATION_HEAD_TURN_LEFT:
            nResult = CLIENT_ANIMATION_MONSTER_HEAD_TURN_LEFT;
            break;
        case SERVER_ANIMATION_HEAD_TURN_RIGHT:
            nResult = CLIENT_ANIMATION_MONSTER_HEAD_TURN_RIGHT;
            break;
        case SERVER_ANIMATION_PLACEABLE_DEACTIVATE:
            nResult = CLIENT_ANIMATION_DEACTIVATED;
            break;
        case SERVER_ANIMATION_WALK_INJURED:
            nResult = CLIENT_ANIMATION_MONSTER_WALK_INJURED;
            break;
        case SERVER_ANIMATION_WHIRLWIND:
            nResult = CLIENT_ANIMATION_MONSTER_WHIRLWIND;
            break;
        case SERVER_ANIMATION_DEACTIVATE:
            nResult = CLIENT_ANIMATION_DROID_DISABLED;
            break;
        case SERVER_ANIMATION_HORROR:
            nResult = CLIENT_ANIMATION_HORROR;
            break;
        case SERVER_ANIMATION_USE_COMPUTER:
            nResult = CLIENT_ANIMATION_USE_COMPUTER;
            break;
        case SERVER_ANIMATION_SLEEP:
            nResult = CLIENT_ANIMATION_MONSTER_SLEEP;
            break;
        case SERVER_ANIMATION_PARALYZED:
            nResult = CLIENT_ANIMATION_MONSTER_PARALYZED;
            break;
        case SERVER_ANIMATION_CUSTOM01:
            nResult = CLIENT_ANIMATION_CUSTOM01;
            break;
        case SERVER_ANIMATION_PAUSE_FIDGET:
            nResult = CLIENT_ANIMATION_MONSTER_PAUSE_FIDGET;
            break;
        case SERVER_ANIMATION_CHOKE:
            nResult = CLIENT_ANIMATION_MONSTER_CHOKE;
            break;
        case SERVER_ANIMATION_WELD:
            nResult = CLIENT_ANIMATION_MONSTER_WELD;
            break;
        case SERVER_ANIMATION_TALK_INJURED:
            nResult = CLIENT_ANIMATION_MONSTER_TALK_INJURED;
            break;
        case SERVER_ANIMATION_FALL_FROM_BEHIND:
        case SERVER_ANIMATION_FALL_FROM_FRONT:
            nResult = CLIENT_ANIMATION_MONSTER_KNOCKED_BACK;
            break;
        case SERVER_ANIMATION_DIE:
        case SERVER_ANIMATION_DIE_2:
            nResult = CLIENT_ANIMATION_MONSTER_DIE;
            break;
        case SERVER_ANIMATION_GET_UP_FROM_DEAD:
        case SERVER_ANIMATION_GET_UP_FROM_DEAD_2:
            nResult = CLIENT_ANIMATION_MONSTER_STAND_FROM_PRONE;
            break;
        case SERVER_ANIMATION_DRAW_WEAPON:
            nResult = SERVER_ANIMATION_DRAW_WEAPON;
            switch (m_nWeaponClass) {
            case WEAPON_CLASS_STUN_BATON:
                nResult = CLIENT_ANIMATION_STUN_BATON_WIELD_DRAW;
                break;
            case WEAPON_CLASS_SINGLE_SABER:
                nResult = CLIENT_ANIMATION_SINGLE_SABER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_TWO_HANDED_SABER:
                nResult = CLIENT_ANIMATION_TWO_HANDED_SABER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_DUAL_SABER:
                nResult = CLIENT_ANIMATION_DUAL_SABER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_SINGLE_BLASTER:
                nResult = CLIENT_ANIMATION_SINGLE_BLASTER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_DUAL_BLASTER:
                nResult = CLIENT_ANIMATION_DUAL_BLASTER_WIELD_DRAW;
                break;
            case WEAPON_CLASS_RIFLE:
            case WEAPON_CLASS_HEAVY_CARBINE:
                nResult = CLIENT_ANIMATION_RIFLE_WIELD_DRAW;
                break;
            }
            break;
        case SERVER_ANIMATION_KNOCKED_DOWN_LOOP:
        case SERVER_ANIMATION_KNOCKED_DOWN_LOOP_2:
            nResult = CLIENT_ANIMATION_MONSTER_KNOCKED_BACK_LOOP;
            break;
        case SERVER_ANIMATION_POWER_ATTACK:
            nResult = CLIENT_ANIMATION_SINGLE_SABER_POWER_ATTACK_1;
            break;
#if !KOTOR1
        case SERVER_ANIMATION_FORCE_CRUSH_DAMAGE:
            nResult = CLIENT_ANIMATION_FORCE_CRUSH_DAMAGE;
            break;
#endif
        case SERVER_ANIMATION_CONJURE_3:
            nResult = CLIENT_ANIMATION_MONSTER_CONJURE_3;
            break;
        case SERVER_ANIMATION_DROID_UTILITY_CONJURE:
#if KOTOR1
            nResult = CLIENT_ANIMATION_DROID_UTILITY_CONJURE;
#else
            nResult = CLIENT_ANIMATION_DROID_UTILITY_CONJURE;
#endif
            break;
        case SERVER_ANIMATION_DROID_UTILITY_CAST:
#if KOTOR1
            nResult = CLIENT_ANIMATION_DROID_UTILITY_CAST;
#else
            nResult = CLIENT_ANIMATION_DROID_UTILITY_CAST;
#endif
            break;
        }
    }
    return nResult;
}
