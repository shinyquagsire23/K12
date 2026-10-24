// CSWCObject - client-side wrapper for a server game object (CSWSObject).

#include "k2stdint.h"
#include "nwcobject.h"
#include "exostring.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "clientexoapp.h"
#include "nwchandlemessage.h"
#include "gameobjectarray.h"
#include "nwsobject.h"
#include "guiingame.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "AURORA/aurmodel.h"
#include "nwccreature.h"
#include "clientoptions.h"
#include "worldtimer.h"
#include "nwcplaceable.h"
#include "nwsplaceable.h"
#include "clientaimaster.h"
#include "exosound.h"
#include "nwcdoor.h"
#include "nwctrigger.h"
#include "aurcamera.h"
#include "swgui.h"
#include "CExoArrayListImpl.h"
#include "nwscreature.h"
#include "nwccreatureappearance.h"
#include "twodimarray.h"
#include "nwcanimbase.h"
#include "nwscriptconst.h"
#include <stdio.h>

#include "port/libcstring.h"
#include "port/crtapi.h"
#include "serveranimation.h"

// Componentwise Vector equality.
int equal(const Vector& a, const Vector& b);

// The cutscene animation banks of CUTSCENE_ANIMATION_BANK_SIZE ids each (CSWCAnimBase::
// GetAnimationName names them "cut<NNN>" plus the bank's suffix): fire-and-forget, fire-and-forget
// played out of place ("w"), looping ("l") and looping out of place ("wl").
enum {
    CUTSCENE_ANIMATION_FIRE_FORGET              = 1000,
    CUTSCENE_ANIMATION_FIRE_FORGET_OUT_OF_PLACE = 1200,
    CUTSCENE_ANIMATION_LOOPING                  = 1400,
    CUTSCENE_ANIMATION_LOOPING_OUT_OF_PLACE     = 1600,
    CUTSCENE_ANIMATION_BANK_SIZE                = 128,
};

// How long a SetFeedbackInfo text stays up, in milliseconds.
enum {
    FEEDBACK_INFO_DURATION_MS = 5000,
};

CSWCObject::CSWCObject(unsigned char nObjectType)
    : CGameObject(nObjectType, OBJECT_INVALID)
{
    m_pArea = 0;
    m_pModel = 0;
    m_pQuededAnimationCallback = 0;
    m_currentAnimation = 0;
    m_queuedAnimation = 0;
    m_loopingAnimation = 0;
    m_nFireForgetTimeLeftMs = 0;
    m_nFireForgetCapacity = 8;
    m_paFireForgetAnims = new CSWCFireForgetAnim[8];
    m_nFireForgetCount = 0;
    m_pAINode = 0;
    m_animationSpeed = 1.0f;
    m_fadeStartDay = 0;
    m_fadeStartTime = 0;
    m_fadeState = 1;
    m_desiredFadeState = 1;
    m_finalFadeAlpha = 1.0f;
    m_bTimedFade = 0;
    m_bSnapPositionOnUpdate = 0;
    m_nFadeHoldMs = 0;
    m_nReserved9 = 0;
    m_fHeadHeight = 2.1f;
    m_bShadeTexture = 0;
    m_bNoProjectileAttach = 0;
    m_bStoneTexture = 0;
    m_bBarkTexture = 0;
    m_nLowerGaitAnimation = SERVER_ANIMATION_PAUSE;
    m_vocalizationType = 0;
    m_pStreamingVocalization = 0;
    m_pVocalization = 0;
    m_bSpokeChat = 0;
    m_bAttractsLookAt = 0;
    m_oidLookAtOverride = CGameObjectArray::INVALIDOBJID;
    m_bVisible = 1;
    m_groundNormal.x = 0.0f;
    m_groundNormal.y = 0.0f;
    m_groundNormal.z = 1.0f;
    m_portraitId = CSWPortrait::PORTRAIT_ID_NONE;
    m_nReserved8 = 0;
    m_bPendingRemoval = 0;
    m_feedbackTimer = 0;
    m_bModelSyncDisabled = 0;
    m_bPlayingCutsceneAnimation = 0;
    m_pServerObject = 0;
}

// Drops the object from the rosters that hold bare pointers to it, kills its visual effects and
// releases the members it owns.
CSWCObject::~CSWCObject()
{
    g_pAppManager->m_pClientExoApp->RemoveObjectFromHitcheckIgnoreList(m_objectId);
    KillAllVisualEffects();
    CClientAIMaster* pMaster = g_pAppManager->m_pClientExoApp->GetClientAIMaster();
    pMaster->RemoveObject(this);

    if (m_pVocalization) {
        delete m_pVocalization;
        m_pVocalization = 0;
    }
    if (m_pStreamingVocalization) {
        delete m_pStreamingVocalization;
        m_pStreamingVocalization = 0;
    }
    if (m_paFireForgetAnims) {
        delete[] m_paFireForgetAnims;
        m_paFireForgetAnims = 0;
    }
    if (m_pServerObject) {
        m_pServerObject->DetachFromClientObject();
        m_pServerObject = 0;
    }
}

void CSWCObject::AttachmentFromServerObject(CSWSObject* pServerObject)
{
    m_pServerObject = pServerObject;
}

void CSWCObject::DetachFromServerObject()
{
    m_pServerObject = 0;
}

void CSWCObject::ClearAllActions()
{
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    pServer->ClearAllActions(pServer->ClientToServerObjectId(m_objectId));
}

void CSWCObject::ClearAllQueuedCombatActions()
{
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    pServer->ClearAllQueuedCombatActions(pServer->ClientToServerObjectId(m_objectId));
}

// Clear the acting creature's queue, then ask the server to unlock this object.
void CSWCObject::MenuActionUseSecuritySpike(unsigned int a, CSWCCreature* pCreature)
{
    if (pCreature)
        static_cast<CSWCObject*>(pCreature)->ClearAllActions();  // idiom-exempt: standalone CSWCCreature; base method inlines
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_UnlockObject(
        m_objectId, a & CSWGuiInterfaceAction::ACTION_ID_ITEM_CLEAR);
}

CSWCObject* CSWCObject::AsSWCObject()
{
    return this;
}

void CSWCObject::UpdateWalkMesh()
{
}

void CSWCObject::HidePopUpBubble()
{
}

void CSWCObject::ShowPopUpBubble(const Vector& v)
{
    K2_UNUSED(v);
}

void CSWCObject::KillPopUpBubble()
{
}

void CSWCObject::SetPopUpBubbleText(const CExoString& s, int a, float f, float g)
{
    K2_UNUSED(s); K2_UNUSED(a); K2_UNUSED(f); K2_UNUSED(g);
}

void CSWCObject::SetPopUpBubbleDefaultText(const CExoString& s)
{
    K2_UNUSED(s);
}

void CSWCObject::SetPopUpBubbleDefaultCreatureID(unsigned long long id)
{
    K2_UNUSED(id);
}

void CSWCObject::SetPopUpBubblePortrait(const CResRef& r)
{
    K2_UNUSED(r);
}

void CSWCObject::UpdateFloatyText()
{
}

int CSWCObject::RemoveAnimation(unsigned short a)
{
    K2_UNUSED(a);
    return 0;
}

// Lazily resolves and caches this client object's server-side counterpart. Once the object is
// marked gone, or the server is down, it stays null.
CSWSObject* CSWCObject::GetServerObject()
{
    if (!m_pServerObject) {
        if (m_bPendingRemoval)
            return 0;
        if (g_pAppManager->m_pServerExoApp) {
            unsigned long long sid =
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_objectId);
            CGameObject* pObj = g_pAppManager->m_pServerExoApp->GetGameObject(static_cast<unsigned int>(sid));
            if (pObj) {
                CSWSObject* pServerObject = pObj->AsSWSObject();
                m_pServerObject = pServerObject;
                if (pServerObject)
                    pServerObject->AttachmentFromClientObject(this);
            }
        }
    }
    return m_pServerObject;
}

// Shifts every entry above the head down one slot, resets the slot that falls off the top and
// drops the depth by one. The fields are copied individually so the padding inside the record is
// left alone.
void CSWCObject::RemoveTopFireForgetAnimation()
{
    for (unsigned long long i = 1; i < m_nFireForgetCount; i++)
    {
        m_paFireForgetAnims[i - 1].m_bActive      = m_paFireForgetAnims[i].m_bActive;
        m_paFireForgetAnims[i - 1].m_nAnimation = m_paFireForgetAnims[i].m_nAnimation;
        m_paFireForgetAnims[i - 1].m_nStartDay      = m_paFireForgetAnims[i].m_nStartDay;
        m_paFireForgetAnims[i - 1].m_nStartTime      = m_paFireForgetAnims[i].m_nStartTime;
        m_paFireForgetAnims[i - 1].m_nDuration      = m_paFireForgetAnims[i].m_nDuration;
        m_paFireForgetAnims[i - 1].m_fSpeed      = m_paFireForgetAnims[i].m_fSpeed;
        m_paFireForgetAnims[i - 1].m_pPayload     = m_paFireForgetAnims[i].m_pPayload;
    }

    if (m_nFireForgetCount != 0)
    {
        unsigned char nCount = m_nFireForgetCount;
        m_paFireForgetAnims[nCount - 1] = CSWCFireForgetAnim();
        m_nFireForgetCount = nCount - 1;
    }
}

// Pops as many entries as the stack could hold; the pops past the bottom are no-ops.
void CSWCObject::RemoveAllFireForgetAnimations()
{
    for (unsigned int i = 0; i < m_nFireForgetCapacity; i++)
        RemoveTopFireForgetAnimation();
}

// Schedules a fire-and-forget animation. The stack grows by 8 entries when full, and the new entry
// is inserted in time order: entries scheduled later slide up, so the soonest stays at index 0.
void CSWCObject::InsertFireForgetAnimation(unsigned short a, unsigned int nDuration, float fSpeed,
                                           void* p, unsigned int nDelay)
{
    AnimationFireAndForget(a);

    unsigned int nDay;
    unsigned int nTime;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);

    int nCount = m_nFireForgetCount;
    int nCapacity = m_nFireForgetCapacity;
    K2_ASSUME(nCount >= 0);   // read from an unsigned char
    if (nCount == nCapacity)
    {
        int nNewCapacity = nCount + 8;
        CSWCFireForgetAnim* paNew = new CSWCFireForgetAnim[nNewCapacity];
        K2_ASSUME(nCapacity < nNewCapacity);   // the stack is full: capacity == count
        for (int i = 0; i < nCapacity; i++)
        {
            paNew[i].m_bActive      = m_paFireForgetAnims[i].m_bActive;
            paNew[i].m_nAnimation = m_paFireForgetAnims[i].m_nAnimation;
            paNew[i].m_nStartDay  = m_paFireForgetAnims[i].m_nStartDay;
            paNew[i].m_nStartTime = m_paFireForgetAnims[i].m_nStartTime;
            paNew[i].m_nDuration  = m_paFireForgetAnims[i].m_nDuration;
            paNew[i].m_fSpeed     = m_paFireForgetAnims[i].m_fSpeed;
            paNew[i].m_pPayload     = m_paFireForgetAnims[i].m_pPayload;
        }
#ifdef K2_FIX_ENGINE_BUGS
        delete[] m_paFireForgetAnims;
#else
        // (sic) the block came from new[] but is released with scalar delete here.
        if (m_paFireForgetAnims)
            delete m_paFireForgetAnims;
#endif
        m_paFireForgetAnims = paNew;
        m_nFireForgetCapacity += 8;
    }

    unsigned int nEndDay;
    unsigned int nEndTime;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->AddWorldTimes(nDay, nTime, 0, nDelay,
                                                                  &nEndDay, &nEndTime);

    K2_ASSUME(m_nFireForgetCount == nCount);   // unchanged since the top
    int i = m_nFireForgetCount;
    while (i > 0 &&
           g_pAppManager->m_pClientExoApp->GetWorldTimer()->CompareWorldTimes(
               nEndDay, nEndTime, m_paFireForgetAnims[i - 1].m_nStartDay,
               m_paFireForgetAnims[i - 1].m_nStartTime) == -1)
    {
        i--;
        m_paFireForgetAnims[i + 1].m_bActive      = m_paFireForgetAnims[i].m_bActive;
        m_paFireForgetAnims[i + 1].m_nAnimation = m_paFireForgetAnims[i].m_nAnimation;
        m_paFireForgetAnims[i + 1].m_nStartDay  = m_paFireForgetAnims[i].m_nStartDay;
        m_paFireForgetAnims[i + 1].m_nStartTime = m_paFireForgetAnims[i].m_nStartTime;
        m_paFireForgetAnims[i + 1].m_nDuration  = m_paFireForgetAnims[i].m_nDuration;
        m_paFireForgetAnims[i + 1].m_fSpeed     = m_paFireForgetAnims[i].m_fSpeed;
        m_paFireForgetAnims[i + 1].m_pPayload     = m_paFireForgetAnims[i].m_pPayload;
    }

    m_paFireForgetAnims[i].m_bActive      = 1;
    m_paFireForgetAnims[i].m_nAnimation = a;
    m_paFireForgetAnims[i].m_nStartDay  = nEndDay;
    m_paFireForgetAnims[i].m_nStartTime = nEndTime;
    m_paFireForgetAnims[i].m_nDuration  = nDuration;
    m_paFireForgetAnims[i].m_fSpeed     = fSpeed;
    m_paFireForgetAnims[i].m_pPayload     = p;
    m_nFireForgetCount++;
}

// Routes a fire-and-forget animation to the object kind that owns a richer path for it, and
// otherwise pushes it onto the base stack with the default duration and speed.
void CSWCObject::AddFireForgetAnimation(unsigned short a, void* p)
{
    if (AsSWCCreature() != 0)
    {
        AsSWCCreature()->AddFireForgetAnimation(a, p);
        return;
    }
    if (AsSWCDoor() != 0)
    {
        AsSWCDoor()->AddFireAndForgetAnimation(a, p, 0);
        return;
    }
    if (AsSWCPlaceable() != 0)
    {
        AsSWCPlaceable()->AddFireAndForgetAnimation(a, p, 0);
        return;
    }
    if (AsSWCTrigger() != 0)
    {
        AsSWCTrigger()->AddFireAndForgetAnimation(a, p, 0);
        return;
    }
    InsertFireForgetAnimation(a, 500, 1.0f, p, 0);
}

// Hands the animation code the fire-and-forget entry at the head of the stack, or a "nothing
// playing" default when the slot is dead.
//
// A slot is live when it has been filled and its scheduled time has arrived. A zero elapsed time
// is ambiguous (either the same millisecond or a time still in the future), so that case falls back
// to a full comparison against the current day and time of day.
//
// The reported speed is scaled so the animation still finishes in the time it has left:
// speed * duration / max(duration - elapsed, 50 ms).
int CSWCObject::GetTopFireForgetAnimation(unsigned short* pAnimation, float* pSpeed,
                                          unsigned int* pDuration, void** ppPayload)
{
    unsigned int nStartDay  = m_paFireForgetAnims->m_nStartDay;
    unsigned int nStartTime = m_paFireForgetAnims->m_nStartTime;
    int nElapsed = g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetTimeDifferenceFromWorldTime(
        nStartDay, nStartTime);

    if (m_paFireForgetAnims->m_bActive != 0)
    {
        int bValid = 1;
        if (nElapsed == 0)
        {
            unsigned int nDay;
            unsigned int nTime;
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
            bValid = g_pAppManager->m_pClientExoApp->GetWorldTimer()->CompareWorldTimes(
                    nDay, nTime, nStartDay, nStartTime) != -1;
        }
        if (bValid)
        {
            *pAnimation = m_paFireForgetAnims->m_nAnimation;
            *ppPayload = m_paFireForgetAnims->m_pPayload;
            unsigned int nTotal = m_paFireForgetAnims->m_nDuration;
            int nRemaining = nTotal - nElapsed;
            if (nRemaining < 50)
                nRemaining = 50;
            *pDuration = nRemaining;
            *pSpeed = m_paFireForgetAnims->m_fSpeed * nTotal / nRemaining;
            return 1;
        }
    }

    *pAnimation = SERVER_ANIMATION_PAUSE;
    *ppPayload = 0;
    *pSpeed = 1.0f;
    *pDuration = 500;
    return 0;
}

void CSWCObject::AbortFireForgetAnimation(int a)
{
    m_nFireForgetTimeLeftMs = 0;
    if (a != 0)
        return;
    SetAnimation(m_loopingAnimation, 0, 0);
}

void CSWCObject::SetFeedbackInfo(const CExoString& s)
{
    m_feedbackInfo = s;
    m_feedbackTimer = FEEDBACK_INFO_DURATION_MS;
}

void CSWCObject::GetFeedbackInfo(CExoString& out, unsigned int& outTimer)
{
    out = m_feedbackInfo;
    outTimer = m_feedbackTimer;
}

// True for the two animation ranges that play out of place.
int CSWCObject::AnimationPlayOutOfPlace(unsigned short a)
{
    if (static_cast<unsigned short>(a - CUTSCENE_ANIMATION_FIRE_FORGET_OUT_OF_PLACE) < CUTSCENE_ANIMATION_BANK_SIZE)
        return 1;
    return static_cast<unsigned short>(a - CUTSCENE_ANIMATION_LOOPING_OUT_OF_PLACE) < CUTSCENE_ANIMATION_BANK_SIZE;
}

void CSWCObject::SetId(unsigned long long id)
{
    m_objectId = id;
}

unsigned short CSWCObject::GetPortraitId()
{
    return m_portraitId;
}

unsigned short CSWCObject::GetCurrentAnimation()
{
    return m_currentAnimation;
}

CSWCAnimBase* CSWCObject::GetAnimationBase()
{
    return m_pModel;
}

unsigned short CSWCObject::GetLoopingAnimation()
{
    return m_loopingAnimation;
}

float CSWCObject::GetCanSeeHeight()
{
    return 1.5f;
}

// The "sticky mouse" picking radius, returned squared so callers compare against a squared
// distance: a tenth of the screen in each axis, summed in quadrature.
float CSWCObject::GetStickyMouseThreshhold()
{
    float h = g_nScreenHeight / 10.0f;
    float w = g_nScreenWidth / 10.0f;
    return h * h + w * w;
}

float CSWCObject::GetHeadHeightOffset()
{
    return 1.5f;
}

int CSWCObject::GetFollowingBody()
{
    return 1;
}

void CSWCObject::SetOrientation(const Vector& v)
{
    m_orientation = normalize(v);
    if (m_pModel && !m_bModelSyncDisabled)
        m_pModel->SetOrientation(v);
    UpdateWalkMesh();
}

void CSWCObject::SetGroundNormal(const Vector& v)
{
    m_groundNormal = v;
    if (m_pModel)
        m_pModel->SetShadowBlobOrientation(RotationArc(Vector{0.0f, 0.0f, 1.0f}, v));
}

Vector CSWCObject::GetCutsceneDummyPosition()
{
    if (m_pModel)
        return m_pModel->GetCutsceneDummyPosition();
    Vector v;
    v.x = v.y = v.z = 0.0f;
    return v;
}

unsigned int CSWCObject::CalculateProjectileTimeToTarget(const Vector& target, int a)
{
    Vector diff = { target.x - m_position.x,
                    target.y - m_position.y,
                    target.z - m_position.z };
    float dist = magnitude(diff);
    return static_cast<unsigned int>(dist / (a ? 37.0f : 42.0f) * 1000.0f);
}

void CSWCObject::SetWind(int a, float f)
{
    Gob* pModelPart = GetModel(ANIM_MODEL_BODY, 0);
    if (!pModelPart)
        return;
    if (static_cast<unsigned char>(g_pAppManager->m_pClientExoApp->GetActivePauseState()) == 2)
        pModelPart->SetWind(false, 0.0f, 0.0f, 0.0f);
    else if (a)
        pModelPart->SetWind(true, f, 0.0f, 0.0f);
    else
        pModelPart->SetWind(false, 0.0f, 0.0f, 0.0f);
}

void CSWCObject::SetPortrait(const CResRef& r)
{
    m_portrait.m_baseResRef = r;
    GetPortrait(1);
}

CResRef CSWCObject::GetPortrait(unsigned char a)
{
    return m_portrait.GetPortrait(a);
}

void CSWCObject::SetPortraitId(unsigned short a)
{
    CSWPortrait portrait;
    if (portrait.SetPortraitId(a))
        SetPortrait(portrait.GetPortrait(0));
    m_portraitId = a;
}

void CSWCObject::NewVocalization(const CResRef& r, int a)
{
    delete m_pVocalization;
    m_pVocalization = new CExoSoundSource(r);
    m_vocalizationType = a;
}

CExoString CSWCObject::GetName()
{
    return CExoString("");
}

void CSWCObject::UpdatePopUpBubble()
{
}

void CSWCObject::SetQuededAnimationCallback()
{
}

int CSWCObject::ClearQuededAnimationCallback()
{
    return 0;
}

int CSWCObject::LipSync(const CResRef&)
{
    return 0;
}

int CSWCObject::IsHostile()
{
    return 0;
}

int CSWCObject::ApplyShadowBlob()
{
    return 1;
}

// Forwards to the virtual BarkBubble handler.
int CSWCObject::BarkString(unsigned long long a, const CExoString& s, const CResRef& r, int b)
{
    return BarkBubble(a, s, r, b);
}

int CSWCObject::BarkBubble(unsigned long long a, const CExoString& s, const CResRef& r, int b)
{
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowBarkBubble(a, s, r, b);
    return 1;
}

void CSWCObject::ClientServerSyncHandler_Position(const Vector& v, int, float)
{
    SetPosition(v);
}

void CSWCObject::ClientServerSyncHandler_Orientation(const Vector& v)
{
    SetOrientation(v);
}

void* CSWCObject::GetCameraFollowPart()
{
    return m_pModel->GetModel(ANIM_MODEL_BODY);
}

void CSWCObject::ShowShadowBlob()
{
    if (m_pModel)
        m_pModel->ShowShadowBlob(0);
}

void CSWCObject::HideShadowBlob()
{
    if (m_pModel)
        m_pModel->HideShadowBlob();
}

void CSWCObject::TurnOnShadows()
{
    if (m_pModel)
        m_pModel->TurnOnShadows();
}

void CSWCObject::TurnOffShadows()
{
    if (m_pModel)
        m_pModel->TurnOffShadows();
}

void CSWCObject::RemoveShadowBlob()
{
    if (!m_pModel)
        return;
    m_pModel->RemoveShadowBlob();
    if (m_bVisible)
        m_pModel->TurnOnShadows();
}

void CSWCObject::AttachToScene(CAurScene* pScene)
{
    if (m_pModel)
        m_pModel->AttachToScene(pScene);
}

Gob* CSWCObject::GetModel(unsigned char a, int b)
{
    K2_UNUSED(b);
    if (m_pModel)
        return m_pModel->GetModel(a);
    return 0;
}

void CSWCObject::ReplaceTexture(unsigned char a, const CResRef& r1, const CResRef& r2, int b)
{
    if (m_pModel)
        m_pModel->ReplaceTexture(a, r1, r2, b);
}

void CSWCObject::ReplaceTexture(const CResRef& r1, const CResRef& r2, int b)
{
    if (m_pModel)
        m_pModel->ReplaceTexture(r1, r2, b);
}

void CSWCObject::ReplaceTexture(unsigned char a, const CResRef& r1, const CResRef& r2,
                                unsigned int c, unsigned short* p, int d, unsigned int e)
{
    if (m_pModel)
        m_pModel->ReplaceTexture(a, r1, r2, c, p, d, e);
}

void CSWCObject::ReplaceTexture(const CResRef& r1, const CResRef& r2, unsigned int c,
                                unsigned short* p, int d, unsigned int e)
{
    if (m_pModel)
        m_pModel->ReplaceTexture(r1, r2, c, p, d, e);
}

void CSWCObject::RestoreTexture()
{
    if (m_pModel)
        m_pModel->RestoreTexture();
}

void CSWCObject::RestoreTexture(unsigned char a)
{
    if (m_pModel)
        m_pModel->RestoreTexture(a);
}

// Take over pSource's aurora model: tear down our own, move theirs over, and re-point the moved
// model's external-data back-reference at this object.
void CSWCObject::AcquireAnimationBase(CSWCObject* pSource)
{
    if (!pSource)
        return;
    delete m_pModel;
    if (pSource->AsSWCCreature())
        pSource->AsSWCCreature()->UnRegisterCallbacks();
    m_pModel = pSource->m_pModel;
    pSource->m_pModel->GetModel(ANIM_MODEL_BODY)->SetExternalData(this);
    pSource->m_pModel->GetModel(ANIM_MODEL_BODY)->SetExternalDataType(m_objectType);
    pSource->m_pModel = 0;
}

void CSWCObject::EnableBumpedOutShell(CResRef r, float f)
{
    if (m_pModel)
        m_pModel->EnableBumpedOutShell(r, f);
}

void CSWCObject::DisableBumpedOutShell()
{
    if (m_pModel)
        m_pModel->DisableBumpedOutShell();
}

void CSWCObject::SetMotionBlur(int a)
{
    if (m_pModel)
        m_pModel->SetMotionBlur(a);
}

float CSWCObject::GetAnimationLength(unsigned short a)
{
    if (m_pModel)
        return m_pModel->GetAnimationLength(a);
    return 1.0f;
}

void CSWCObject::SetAlpha(float alpha, unsigned char a)
{
    if (m_pModel)
        m_pModel->SetAlpha(alpha, a);
}

// Projects the model's "lookathook" part into screen space and reports which side of the camera
// plane it fell on: 1 in front, 2 behind or level. Returns 0 when there is no model, and falls
// back to the model's own centre point when it has no lookathook part.
char CSWCObject::GetReticlePoint(Vector& v)
{
    if (m_pModel == 0)
        return 0;
    Gob* pGob = m_pModel->GetModel(ANIM_MODEL_BODY);
    if (pGob != 0) {
        Quaternion qIdentity;
        Part* pPart = pGob->GetPart(const_cast<char*>("lookathook"));
        if (pPart != 0) {
            pPart->GetWorld(v, qIdentity);
            CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
            Quaternion q = pCamera->GetOrientation();
            float fA1 = (q.x * q.z + q.w * q.y) * -2.0f;
            float fA2 = (q.z * q.y - q.x * q.w) * -2.0f;
            float fA3 = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
            float fPoint = v.x * fA1 + v.y * fA2 - v.z * fA3;
            Vector vCamPos = pCamera->GetPosition();
            float fCam = fA1 * vCamPos.x + fA2 * vCamPos.y - vCamPos.z * fA3;
            char nRet = !(fPoint > fCam) ? 2 : 1;
            v = pCamera->MapToScreen(v, true);
            short nWidth = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
            v.x = v.x * nWidth;
            short nHeight = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight;
            v.y = v.y * nHeight;
            return nRet;
        }
    }
    return m_pModel->GetCenterPoint(v);
}

float CSWCObject::GetFinalFadeAlpha()
{
    if (AsSWCCreature())
    {
        CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        if (pPlayer == 0)
        {
            float fAlpha = GetVisualEffectAlpha();
            return fAlpha == -1.0f ? 1.0f : fAlpha;
        }
        for (int i = 0; i < pPlayer->m_visibilityList.m_count; i++)
        {
            if (pPlayer->m_visibilityList.m_pArray[i]->m_objectId == m_objectId)
                return 1.0f;
        }
        if (m_desiredFadeState == 1)
        {
            float fAlpha = GetVisualEffectAlpha();
            return fAlpha == -1.0f ? 1.0f : fAlpha;
        }
        return 0.0f;
    }
    if (m_desiredFadeState == 1)
    {
        float fAlpha = GetVisualEffectAlpha();
        return fAlpha == -1.0f ? 1.0f : fAlpha;
    }
    return 0.0f;
}

// Per-frame fade tick. Once the object has reached its desired state this only reports whether the
// hold time following the fade has elapsed. Otherwise it steps m_finalFadeAlpha toward
// GetFinalFadeAlpha, snaps to the target once reached and pushes the clamped result into the model;
// m_fadeState 2 and 3 mean still stepping in or out, 1 and 0 mean arrived.
int CSWCObject::FadeObject()
{
    if (m_fadeState == m_desiredFadeState)
    {
        if (m_bTimedFade != 0)
        {
            unsigned int nDay, nTime;
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
            unsigned int nDayDelta, nTimeDelta;
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
                nDay, nTime, m_fadeStartDay, m_fadeStartTime, &nDayDelta, &nTimeDelta);
            if (nTimeDelta >= m_nFadeHoldMs + m_nFadeStartDelayMs)
                return 1;
        }
        return 0;
    }

    unsigned long long nSnapshotDelta =
        g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetSnapshotTimeDifference();
    unsigned int nElapsed = static_cast<unsigned int>(nSnapshotDelta / 1000);
    if (m_desiredFadeState == 1)
    {
        if (m_finalFadeAlpha >= GetFinalFadeAlpha())
        {
            m_finalFadeAlpha = GetFinalFadeAlpha();
            m_fadeState = 1;
        }
        else
        {
            m_finalFadeAlpha = static_cast<float>(nElapsed) / 2000.0f + m_finalFadeAlpha;
            m_fadeState = 2;
        }
    }
    else if (m_desiredFadeState == 0)
    {
        if (m_nFadeStartDelayMs != 0)
        {
            unsigned int nDay, nTime;
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
            unsigned int nDayDelta, nTimeDelta;
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
                nDay, nTime, m_fadeStartDay, m_fadeStartTime, &nDayDelta, &nTimeDelta);
            if (nTimeDelta <= m_nFadeStartDelayMs)
                return 0;
        }
        CSWCCreature* pCreature = AsSWCCreature();
        if (pCreature && pCreature->m_bInParty)
            pCreature->SetInParty(0);
        if (m_finalFadeAlpha <= GetFinalFadeAlpha())
        {
            m_finalFadeAlpha = GetFinalFadeAlpha();
            m_fadeState = 0;
            if (m_bTimedFade != 0)
            {
                g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(
                    &m_fadeStartDay, &m_fadeStartTime);
                m_pModel->AttachToScene(0);
            }
            CSWCCreature* pDoneCreature = AsSWCCreature();
            if (pDoneCreature && pDoneCreature->m_bInParty)
                pDoneCreature->SetInParty(0);
        }
        else
        {
            m_finalFadeAlpha = static_cast<float>(nElapsed) / -2000.0f + m_finalFadeAlpha;
            m_fadeState = 3;
        }
    }
    if (m_finalFadeAlpha < 0.0f)
        m_finalFadeAlpha = 0.0f;
    else if (m_finalFadeAlpha > 1.0f)
        m_finalFadeAlpha = 1.0f;
    if (m_pModel)
        m_pModel->SetAlpha(m_finalFadeAlpha, ANIM_MODEL_BODY);
    return 0;
}

// Applies a fade state immediately. State 1 means fading in, anything else fading out. The
// shadow-detail option selects between real shadows and the cheap shadow blob, and at the middle
// setting only the local player keeps real shadows. The final alpha is then pushed into the model.
void CSWCObject::SetFadeState(unsigned char state)
{
    CSWCObject* pSelf = this;
    if (pSelf != 0 && m_bPendingRemoval == 1)
        return;
    m_fadeState = state;
    unsigned char nShadowDetail =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nShadowDetail;
    int bVisible;
    if (state == 1)
    {
        if (m_pModel)
        {
            switch (nShadowDetail)
            {
            case 2:
                TurnOnShadows();
                HideShadowBlob();
                break;
            case 1:
                if (m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
                {
                    TurnOnShadows();
                    HideShadowBlob();
                }
                else
                {
                    TurnOffShadows();
                    ShowShadowBlob();
                }
                break;
            default:
                TurnOffShadows();
                ShowShadowBlob();
                break;
            }
        }
        bVisible = 1;
    }
    else
    {
        if (m_pModel)
        {
            switch (nShadowDetail)
            {
            case 2:
                TurnOffShadows();
                HideShadowBlob();
                break;
            case 1:
                g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
                TurnOffShadows();
                HideShadowBlob();
                break;
            default:
                TurnOffShadows();
                ShowShadowBlob();
                break;
            }
        }
        bVisible = 0;
    }
    m_bVisible = bVisible;
    m_desiredFadeState = static_cast<unsigned char>(bVisible);
    float fAlpha = GetFinalFadeAlpha();
    m_finalFadeAlpha = fAlpha;
    if (m_pModel)
        m_pModel->SetAlpha(fAlpha, ANIM_MODEL_BODY);
}

// Queue a desired fade state (applied later by the fade tick). `a` nonzero starts a timed fade and
// snapshots the current world time; `b`/`c` are the fade parameters. The shadow bookkeeping mirrors
// SetFadeState but there is no immediate alpha push.
void CSWCObject::SetDesiredFadeState(unsigned char state, int a, unsigned int b, unsigned int c)
{
    CSWCObject* pSelf = this;
    if (pSelf != 0 && m_bPendingRemoval == 1)
        return;
    m_desiredFadeState = state;
    m_bTimedFade = a;
    m_nFadeHoldMs = b;
    m_nFadeStartDelayMs = c;
    if (a != 0)
        g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&m_fadeStartDay, &m_fadeStartTime);
    unsigned char nShadowDetail =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nShadowDetail;
    int bVisible;
    if (state == 1)
    {
        if (m_pModel)
        {
            switch (nShadowDetail)
            {
            case 2:
                TurnOnShadows();
                HideShadowBlob();
                break;
            case 1:
                if (m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
                {
                    TurnOnShadows();
                    HideShadowBlob();
                }
                else
                {
                    TurnOffShadows();
                    ShowShadowBlob();
                }
                break;
            default:
                TurnOffShadows();
                ShowShadowBlob();
                break;
            }
        }
        bVisible = 1;
    }
    else
    {
        if (m_pModel)
        {
            switch (nShadowDetail)
            {
            case 2:
                TurnOffShadows();
                HideShadowBlob();
                break;
            case 1:
                g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
                TurnOffShadows();
                HideShadowBlob();
                break;
            default:
                TurnOffShadows();
                ShowShadowBlob();
                break;
            }
        }
        bVisible = 0;
    }
    m_bVisible = bVisible;
}

// Moves the object between client areas: drops its id from the old area's roster, adds it to the
// new one's and re-evaluates its visibility there. The area pointer is stored either way.
void CSWCObject::SetArea(CSWCArea* pArea)
{
    if (pArea != m_pArea)
    {
        if (m_pArea)
            m_pArea->m_objectIdList.Remove(m_objectId);
        if (pArea)
        {
            pArea->m_objectIdList.AddUnique(m_objectId);
            SetObjectVisibilityOnSetArea(pArea);
        }
    }
    m_pArea = pArea;
}

// Decides the object's fade state on entering an area. Only placeables (unless the server marks
// them a body bag that is still invisible), doors and items participate. An object at the origin,
// or one no spawn position can see, starts hidden.
void CSWCObject::SetObjectVisibilityOnSetArea(CSWCArea* pArea)
{
    Vector vPosition = {0.0f, 0.0f, 0.0f};
    if (m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
    {
        CSWSPlaceable* pServer = AsSWCPlaceable()->GetServerPlaceable();
        if (pServer && pServer->m_bIsBodyBag != 0 && pServer->m_bIsBodyBagVisible == 0)
            return;
    }
    else if (m_objectType != CGameObject::OBJECT_TYPE_DOOR &&
             m_objectType != CGameObject::OBJECT_TYPE_ITEM)
    {
        return;
    }
    vPosition = m_position;
    Vector vZero = {0.0f, 0.0f, 0.0f};
    unsigned char bVisible;
    if (equal(m_position, vZero) == 0 && pArea->GetObjectsAtPositionVisible(vPosition) != 0)
    {
        SetFadeState(1);
        bVisible = 1;
    }
    else
    {
        SetFadeState(0);
        bVisible = 0;
    }
    SetDesiredFadeState(bVisible, 0, 0, 0);
}

// Resolves the environment-map resref and forwards it to the model. "DEFAULT" pulls the area's
// own map, and an unset area map leaves the object alone.
int CSWCObject::SetEnvMap(int a, int b, const CResRef& r)
{
    CExoString envName;
    if (!m_pModel)
        return 0;

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (r == const_cast<char*>("DEFAULT")) {
        if (pModule && pModule->m_pArea) {
            CResRef areaEnv = pModule->m_pArea->m_defaultEnvMap;
            areaEnv.CopyToString(envName);
        }
#if KOTOR1  // K1: CExoString::GetLength is out of line
        if (envName.GetLength() == 0)
#else
        if (envName.m_sString == 0 || static_cast<int>(strlen(envName.m_sString)) == 0)
#endif
            return 0;
    } else {
        r.CopyToString(envName);
    }
    return m_pModel->SetEnvMap(a, b, CResRef(envName));
}

int CSWCObject::SetEnvMap(unsigned char a, int b, int c, const CResRef& r)
{
    CExoString envName;
    if (!m_pModel)
        return 0;

    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (r == const_cast<char*>("DEFAULT")) {
        if (pModule && pModule->m_pArea) {
            CResRef areaEnv = pModule->m_pArea->m_defaultEnvMap;
            areaEnv.CopyToString(envName);
        }
#if KOTOR1  // K1: CExoString::GetLength is out of line
        if (envName.GetLength() == 0)
#else
        if (envName.m_sString == 0 || static_cast<int>(strlen(envName.m_sString)) == 0)
#endif
            return 0;
    } else {
        r.CopyToString(envName);
    }
    return m_pModel->SetEnvMap(a, b, c, CResRef(envName));
}

void CSWCObject::RestoreEnvMap()
{
    if (m_pModel)
        m_pModel->RestoreEnvMap();
}

// Server-side animation change: records the new playback speed and, when the animation really
// changed, drives the model. Placeables and doors in one of the open-close ranges always queue;
// everything else follows the animations.2da fire-and-forget column.
void CSWCObject::ClientServerSyncHandler_Animation(int a, float speed)
{
    m_animationSpeed = speed;
    if (m_currentAnimation != a) {
        if (m_objectType == 9 || m_objectType == 5) {
            if (static_cast<unsigned short>(a - CUTSCENE_ANIMATION_FIRE_FORGET) < CUTSCENE_ANIMATION_BANK_SIZE)
                goto queue;
            if (static_cast<unsigned short>(a - CUTSCENE_ANIMATION_FIRE_FORGET_OUT_OF_PLACE) < CUTSCENE_ANIMATION_BANK_SIZE)
                goto queue;
        }
        {
            int nEntry = 0;
            unsigned short nRow;
            if (m_pModel)
                nRow = m_pModel->ServerToClientAnimation(static_cast<unsigned short>(a));
            else
                nRow = static_cast<unsigned short>(a);
            if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
                    nRow, CTwoDimArrays::COL2DA_ANIMATIONS_FIREFORGET, &nEntry) == 0 ||
                nEntry == 0) {
                SetAnimation(static_cast<unsigned short>(a), 0, 0);
                return;
            }
        }
    queue:
        AddFireForgetAnimation(static_cast<unsigned short>(a), 0);
    }
}

float CSWCObject::GetAlpha()
{
    if (m_pModel)
        return m_pModel->m_alpha;
    return 0.0f;
}

void CSWCObject::Hilite(const Vector& v, unsigned char a, float f, int b)
{
    K2_UNUSED(f);
    if (m_pModel)
        m_pModel->Hilite(v, a, b);
}

void CSWCObject::UnHilite(unsigned char a)
{
    if (m_pModel)
        m_pModel->UnHilite(a);
}

void CSWCObject::EnableDistortion()
{
    if (m_pModel)
        m_pModel->EnableDistortion();
}

void CSWCObject::DisableDistortion()
{
    if (m_pModel)
        m_pModel->DisableDistortion();
}

int CSWCObject::AnimationWalking(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_WALKING, &nEntry))
        return nEntry != 0;
    return 0;
}

int CSWCObject::AnimationRunning(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_RUNNING, &nEntry))
        return nEntry != 0;
    return 0;
}

int CSWCObject::AnimationPause(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_PAUSE, &nEntry))
        return nEntry != 0;
    return 0;
}

// A failed table lookup still counts as stationary.
int CSWCObject::AnimationStationary(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_STATIONARY, &nEntry))
        return nEntry != 0;
    return 1;
}

int CSWCObject::AnimationOverlay(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_OVERLAY, &nEntry))
        return nEntry != 0;
    return 0;
}

int CSWCObject::AnimationDamage(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_DAMAGE, &nEntry))
        return nEntry != 0;
    return 0;
}

int CSWCObject::AnimationParry(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_PARRY, &nEntry))
        return nEntry != 0;
    return 0;
}

int CSWCObject::AnimationDodge(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_DODGE, &nEntry))
        return nEntry != 0;
    return 0;
}

int CSWCObject::AnimationAttack(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_ATTACK, &nEntry))
        return nEntry != 0;
    return 0;
}

// True for a looping animation: creatures short-circuit on the two looping id ranges, everything
// else goes through the animations.2da looping column.
int CSWCObject::AnimationLooping(unsigned short a)
{
    if (m_objectType == 5) {
        if (static_cast<unsigned short>(a - CUTSCENE_ANIMATION_LOOPING) < CUTSCENE_ANIMATION_BANK_SIZE)
            return 1;
        if (static_cast<unsigned short>(a - CUTSCENE_ANIMATION_LOOPING_OUT_OF_PLACE) < CUTSCENE_ANIMATION_BANK_SIZE)
            return 1;
    }
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_LOOPING, &nEntry))
        return nEntry != 0;
    return 0;
}

// Client-side animation driver. Ten stages:
//   1. a dead or dying creature accepts only the death animations;
//   2. GUI and model gates, the model must own the animation;
//   3/4. push this object's transform back into the model when model sync was gated off, and park
//        the model at the origin for the two cutscene animation ranges;
//   5. stow or re-equip the creature's weapons for this animation;
//   6. stop whichever looping animation is playing;
//   7. resolve the creature's pause-ready id, then decide looping against looping;
//   8. map the animation pair onto a sound or effect id, and spawn the death VFX;
//   9. fixups, then either a fire-forget push or a combat-attack node;
//  10. the tail: cache the pause id, kill weapon blur, compute the cross-fade factor and hand the
//      animation to the model.
// The second parameter is unused.
int CSWCObject::SetAnimation(unsigned short nAnimation, int b, int nForce)
{
    K2_UNUSED(b);
    // 1. A dead or dying creature accepts only the death/dead-idle animations.
    if (g_pAppManager->m_pServerExoApp != 0 && AsSWCCreature() != 0)
    {
        CSWSCreature* pCreature = AsSWCCreature()->GetServerCreature();
#if KOTOR1  // K1 tests GetDeadTemp(), K2 tests party membership and hit points
        if (pCreature == 0 || pCreature->GetDead() != 0 || pCreature->GetDeadTemp() != 0)
#else
        if (pCreature == 0 || pCreature->GetDead() != 0 ||
            (pCreature->m_bInParty != 0 && pCreature->GetCurrentHitPoints(0) <= 0))
#endif
        {
            if (nAnimation != SERVER_ANIMATION_DEAD && nAnimation != SERVER_ANIMATION_DEAD_PRONE &&
                nAnimation != SERVER_ANIMATION_DEAD_2)
                return 0;
        }
    }

    // 2. GUI gate, then the model must exist and must own the animation.
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut != 0 && m_bPlayingCutsceneAnimation != 0)
        return 0;
    if (m_pModel == 0)
        return 0;
    if (m_pModel->AnimationExists(nAnimation) == 0)
        return 0;

    // 3. Model sync was gated off: push the real transform back in before animating.
    int bResynced = 0;
    if (m_bModelSyncDisabled != 0)
    {
        m_bModelSyncDisabled = 0;
        Vector vPos = m_position;
        Vector vOrientation = m_orientation;
        m_pModel->SetPosition(vPos);
        m_pModel->SetOrientation(vOrientation);
        m_pModel->SetAlwaysRender(0);
        m_pModel->SetInCutscene(0);
        bResynced = 1;
    }

    // 4. The two cutscene ranges animate in model space: park the model at the origin with the
    //    identity orientation and gate sync back off.
    if (static_cast<unsigned short>(nAnimation - CUTSCENE_ANIMATION_FIRE_FORGET_OUT_OF_PLACE) <
            CUTSCENE_ANIMATION_BANK_SIZE ||
        static_cast<unsigned short>(nAnimation - CUTSCENE_ANIMATION_LOOPING_OUT_OF_PLACE) <
            CUTSCENE_ANIMATION_BANK_SIZE)
    {
        m_bModelSyncDisabled = 1;
        Vector vOrigin = {0.0f, 0.0f, 0.0f};
        Quaternion qIdentity(1.0f, 0.0f, 0.0f, 0.0f);
        bResynced = 1;
        m_pModel->SetPosition(vOrigin);
        m_pModel->SetOrientation(qIdentity);
        m_pModel->SetAlwaysRender(1);
        m_pModel->SetInCutscene(1);
        m_bPlayingCutsceneAnimation = 1;
    }

    // 5. Weapons: stow them for the animations that need empty hands, otherwise put them back.
    if (AsSWCCreature() != 0)
    {
        if (nAnimation != SERVER_ANIMATION_PAUSE)
            AsSWCCreature()->StopAmbientPauseAnim();
        if (AnimationHideEquippedItems(nAnimation))
            AsSWCCreature()->HideEquippedItems(CResRef(), CResRef());
        else
            TestAndReEquipweapons(0, nAnimation);
    }

    // 6. Stop whichever looping animation is playing.
    if (m_loopingAnimation == SERVER_ANIMATION_LISTEN) {
        m_pModel->SetOverlayAnimation(SERVER_ANIMATION_LISTEN, 0);
        if (nForce == 1) { m_loopingAnimation = nAnimation; m_currentAnimation = nAnimation; return 1; }
    }
    if (m_loopingAnimation == SERVER_ANIMATION_LISTEN_INJURED) {
        m_pModel->SetOverlayAnimation(SERVER_ANIMATION_LISTEN_INJURED, 0);
        if (nForce == 1) { m_loopingAnimation = nAnimation; m_currentAnimation = nAnimation; return 1; }
    }
    if (m_loopingAnimation == SERVER_ANIMATION_TALK_NORMAL) {
        m_pModel->SetOverlayAnimation(SERVER_ANIMATION_TALK_NORMAL, 0);
        if (nForce == 1) { m_loopingAnimation = nAnimation; m_currentAnimation = nAnimation; return 1; }
    }
    if (m_loopingAnimation == SERVER_ANIMATION_TALK_INJURED) {
        m_pModel->SetOverlayAnimation(SERVER_ANIMATION_TALK_INJURED, 0);
        if (nForce == 1) { m_loopingAnimation = nAnimation; m_currentAnimation = nAnimation; return 1; }
    }
    if (m_loopingAnimation == SERVER_ANIMATION_TALK_PLEADING) {
        m_pModel->SetOverlayAnimation(SERVER_ANIMATION_TALK_PLEADING, 0);
        if (nForce == 1) { m_loopingAnimation = nAnimation; m_currentAnimation = nAnimation; return 1; }
    }
    if (m_loopingAnimation == SERVER_ANIMATION_TALK_FORCEFUL) {
        m_pModel->SetOverlayAnimation(SERVER_ANIMATION_TALK_FORCEFUL, 0);
        if (nForce == 1) { m_loopingAnimation = nAnimation; m_currentAnimation = nAnimation; return 1; }
    }
    if (m_loopingAnimation == SERVER_ANIMATION_TALK_LAUGHING) {
        m_pModel->SetOverlayAnimation(SERVER_ANIMATION_TALK_LAUGHING, 0);
        if (nForce == 1) { m_loopingAnimation = nAnimation; m_currentAnimation = nAnimation; return 1; }
    }
    if (m_loopingAnimation == SERVER_ANIMATION_TALK_SAD) {
        m_pModel->SetOverlayAnimation(SERVER_ANIMATION_TALK_SAD, 0);
        if (nForce == 1) { m_loopingAnimation = nAnimation; m_currentAnimation = nAnimation; return 1; }
    }

    // 7. The creature's "ready" pause animation doubles as this function's no-op sentinel.
    int nPauseReady = SERVER_ANIMATION_PAUSE;
    if (AsSWCCreature() != 0)
        nPauseReady = AsSWCCreature()->GetPauseReadyAnimation();

    if (AnimationLooping(nAnimation))
    {
        if (m_loopingAnimation != nAnimation)
        {
            // The 0/1 pair swaps straight over without any of the machinery below.
            if ((nAnimation == 0 && m_loopingAnimation == 1) ||
                (nAnimation == 1 && m_loopingAnimation == 0))
            {
                if (AnimationLooping(nAnimation) == 1)
                    m_loopingAnimation = nAnimation;
                return 1;
            }

            // 8. Drop everything queued on the fire-forget stack, then map the animation pair onto
            //    the id the fire-forget and attack-data machinery below wants.
            for (unsigned int i = 0; i < m_nFireForgetCapacity; i++)
                RemoveTopFireForgetAnimation();

            int nId = nPauseReady;
            if (nAnimation == SERVER_ANIMATION_THROW_HIGH) nId = nAnimation;
            if (nAnimation == SERVER_ANIMATION_THROW_LOW) nId = nAnimation;
            if (nAnimation == SERVER_ANIMATION_MEDITATE || nAnimation == SERVER_ANIMATION_WORSHIP)
                nId = SERVER_ANIMATION_KNEEL;
            if (m_loopingAnimation == SERVER_ANIMATION_MEDITATE || m_loopingAnimation == SERVER_ANIMATION_WORSHIP)
                nId = SERVER_ANIMATION_STAND_FROM_KNEEL;
            if (nAnimation == SERVER_ANIMATION_CLOSED)
                nId = (m_currentAnimation != SERVER_ANIMATION_DOOR_OPEN1) ? CSWCAnimBase::CLIENT_ANIMATION_DOOR_CLOSING_2
                                                                         : CSWCAnimBase::CLIENT_ANIMATION_DOOR_CLOSING_1;
            if (m_loopingAnimation == SERVER_ANIMATION_CLOSED)
                nId = (nAnimation != SERVER_ANIMATION_DOOR_OPEN1) ? CSWCAnimBase::CLIENT_ANIMATION_DOOR_OPENING_2
                                                                 : CSWCAnimBase::CLIENT_ANIMATION_DOOR_OPENING_1;
            switch (nAnimation) {
            case SERVER_ANIMATION_PLACEABLE_ANIMLOOP01: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP02:
            case SERVER_ANIMATION_PLACEABLE_ANIMLOOP03: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP04:
            case SERVER_ANIMATION_PLACEABLE_ANIMLOOP05: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP06:
            case SERVER_ANIMATION_PLACEABLE_ANIMLOOP07: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP08:
            case SERVER_ANIMATION_PLACEABLE_ANIMLOOP09: case SERVER_ANIMATION_PLACEABLE_ANIMLOOP10:
                break;
            default:
                if (nAnimation == SERVER_ANIMATION_PLACEABLE_OPEN)
                    nId = CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_CLOSE_TO_OPEN;
                if (m_loopingAnimation == SERVER_ANIMATION_PLACEABLE_OPEN)
                    nId = CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_OPEN_TO_CLOSE;
                if (nAnimation == SERVER_ANIMATION_PLACEABLE_DEACTIVATE)
                    nId = CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_ON_TO_OFF;
                if (m_loopingAnimation == SERVER_ANIMATION_PLACEABLE_DEACTIVATE)
                    nId = CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_OFF_TO_ON;
                if (nAnimation == SERVER_ANIMATION_PLACEABLE_ACTIVATE)
                    nId = CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_OFF_TO_ON;
                if (m_loopingAnimation == SERVER_ANIMATION_PLACEABLE_ACTIVATE)
                    nId = CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_ON_TO_OFF;
            }
            if (nAnimation == SERVER_ANIMATION_PRONE) nId = CSWCAnimBase::CLIENT_ANIMATION_KNOCKED_DOWN;
            if (m_loopingAnimation == SERVER_ANIMATION_PRONE) nId = CSWCAnimBase::CLIENT_ANIMATION_STAND_FROM_KNOCKED_DOWN;
            if (nAnimation == SERVER_ANIMATION_SIT_AND_MEDITATE) nId = CSWCAnimBase::CLIENT_ANIMATION_SIT_TO_MEDITATE;
            if (nAnimation == SERVER_ANIMATION_SIT_CHAIR || nAnimation == SERVER_ANIMATION_SIT_CHAIR_DRINK ||
                nAnimation == SERVER_ANIMATION_SIT_CHAIR_PAZAK || nAnimation == SERVER_ANIMATION_SIT_CHAIR_COMP1 ||
                nAnimation == SERVER_ANIMATION_SIT_CHAIR_COMP2)
                nId = CSWCAnimBase::CLIENT_ANIMATION_SIT_DOWN;
            if (nAnimation == SERVER_ANIMATION_COLLAPSE_LOOP) nId = CSWCAnimBase::CLIENT_ANIMATION_COLLAPSE;
            if (nAnimation == SERVER_ANIMATION_DESTROYED) {
                if (m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) nId = CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_DIE;
                else if (m_objectType == CGameObject::OBJECT_TYPE_DOOR) nId = CSWCAnimBase::CLIENT_ANIMATION_DOOR_DIE;
            }
            if (m_loopingAnimation == SERVER_ANIMATION_DEAD) nId = SERVER_ANIMATION_GET_UP_FROM_DEAD;
            if (m_loopingAnimation == SERVER_ANIMATION_DEAD_2) nId = SERVER_ANIMATION_GET_UP_FROM_DEAD_2;
            if (m_loopingAnimation == SERVER_ANIMATION_KNOCKDOWN_FROM_BEHIND)
                nId = (nAnimation == SERVER_ANIMATION_DEAD) ? SERVER_ANIMATION_DIE : SERVER_ANIMATION_GET_UP_FROM_DEAD;
            if (m_loopingAnimation == SERVER_ANIMATION_KNOCKDOWN_FROM_FRONT)
                nId = (nAnimation == SERVER_ANIMATION_DEAD_2) ? SERVER_ANIMATION_DIE_2 : SERVER_ANIMATION_GET_UP_FROM_DEAD_2;
            if (nAnimation == SERVER_ANIMATION_KNOCKDOWN_FROM_BEHIND) nId = SERVER_ANIMATION_FALL_FROM_BEHIND;
            if (nAnimation == SERVER_ANIMATION_KNOCKDOWN_FROM_FRONT) nId = SERVER_ANIMATION_FALL_FROM_FRONT;
            if (nAnimation == SERVER_ANIMATION_DEAD) { m_nFireForgetTimeLeftMs = 0; nId = SERVER_ANIMATION_DIE; }
            if (nAnimation == SERVER_ANIMATION_DEAD_2) { m_nFireForgetTimeLeftMs = 0; nId = SERVER_ANIMATION_DIE_2; }

            // 8b. The two death animations spawn the appearance's death visual effect at a named
            //     model part; this is the body's only try range.
            if (nAnimation == SERVER_ANIMATION_DEAD || nAnimation == SERVER_ANIMATION_DEAD_2)
            {
                if (AsSWCCreature() != 0)
                {
                    int nVisualEffect;
                    if (g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                            AsSWCCreature()->m_pCreatureAppearance->m_nAppearanceId,
                            CExoString("DeathVFX"), &nVisualEffect))
                    {
                        CExoString sNode;
                        Gob* pModel = GetModel(ANIM_MODEL_BODY, 0);
                        if (g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
                                AsSWCCreature()->m_pCreatureAppearance->m_nAppearanceId,
                                CExoString("DeathVFXNode"), &sNode) == 0)
                            sNode = "impact";
                        if (pModel != 0)
                        {
                            Vector vPosition = {0.0f, 0.0f, 0.0f};
                            Quaternion qPart(1.0f, 0.0f, 0.0f, 0.0f);
                            if (pModel->GetPartPosition(static_cast<char*>(sNode.CStr()), &vPosition, &qPart) == 0)
                            {
                                vPosition.x = m_position.x + 0.0f;
                                vPosition.y = m_position.y + 0.0f;
                                vPosition.z = m_position.z + 1.0f;
                            }
                            if (m_pArea != 0)
                                m_pArea->ApplyFireAndForgetVisualEffect(
                                    static_cast<unsigned short>(nVisualEffect), vPosition);
                        }
                    }
                }
            }

            // 9. Two more fixups keyed on the pair, both reading the model's creature type.
            int nSoundId = nId;
            if (m_loopingAnimation == SERVER_ANIMATION_KNOCKED_DOWN_LOOP_2)
                nSoundId = nPauseReady;
            int nAttackId = nPauseReady;
            if (m_loopingAnimation == SERVER_ANIMATION_KNOCKED_DOWN_LOOP ||
                m_loopingAnimation == SERVER_ANIMATION_WHIRLWIND) {
                m_nFireForgetTimeLeftMs = 0;
                nAttackId = (m_pModel->m_creatureType == 1) ? CSWCAnimBase::CLIENT_ANIMATION_STAND_FROM_KNOCKED_DOWN
                                                            : CSWCAnimBase::CLIENT_ANIMATION_MONSTER_STAND_FROM_PRONE;
                nSoundId = (m_pModel->m_creatureType == 1) ? CSWCAnimBase::CLIENT_ANIMATION_KNOCKED_DOWN
                                                           : CSWCAnimBase::CLIENT_ANIMATION_MONSTER_KNOCKED_BACK;
            }
            if (nAnimation == SERVER_ANIMATION_DEAD_PRONE) {
                nAttackId = (m_pModel->m_creatureType == 1) ? CSWCAnimBase::CLIENT_ANIMATION_DIE_3 : nPauseReady;
                nSoundId = (m_pModel->m_creatureType == 1) ? CSWCAnimBase::CLIENT_ANIMATION_KNOCKED_DOWN
                                                           : CSWCAnimBase::CLIENT_ANIMATION_MONSTER_KNOCKED_BACK;
                if (m_loopingAnimation == SERVER_ANIMATION_PRONE) { nSoundId = nAttackId; nAttackId = nPauseReady; }
            }
            if (m_loopingAnimation == SERVER_ANIMATION_DEAD_PRONE) {
                nAttackId = nPauseReady;
                nSoundId = (m_pModel->m_creatureType == 1) ? CSWCAnimBase::CLIENT_ANIMATION_STAND_FROM_KNOCKED_DOWN
                                                           : CSWCAnimBase::CLIENT_ANIMATION_MONSTER_STAND_FROM_PRONE;
            }
            if (nAnimation == SERVER_ANIMATION_KNEEL_TALK_ANGRY || nAnimation == SERVER_ANIMATION_KNEEL_TALK_SAD)
                nSoundId = CSWCAnimBase::CLIENT_ANIMATION_KNEEL_DOWN;
            if (m_loopingAnimation == SERVER_ANIMATION_KNEEL_TALK_ANGRY ||
                m_loopingAnimation == SERVER_ANIMATION_KNEEL_TALK_SAD) {
                if (nAnimation == SERVER_ANIMATION_DEAD)
                    nSoundId = CSWCAnimBase::CLIENT_ANIMATION_DIE_1;
                else if (nAnimation == SERVER_ANIMATION_KNEEL_TALK_ANGRY || nAnimation == SERVER_ANIMATION_KNEEL_TALK_SAD)
                    nSoundId = CSWCAnimBase::CLIENT_ANIMATION_KNEEL_DOWN;
                else
                    nSoundId = SERVER_ANIMATION_STAND_FROM_KNEEL_DOWN;
            }

            if (nSoundId != nPauseReady)
            {
                if (nAttackId == nPauseReady)
                {
                    if (AnimationLooping(nAnimation)) {
                        if (AnimationLooping(nAnimation) == 1)
                            m_loopingAnimation = nAnimation;
                    } else {
                        if (AnimationLooping(SERVER_ANIMATION_PAUSE) == 1)
                            m_loopingAnimation = SERVER_ANIMATION_PAUSE;
                    }
                    AddFireForgetAnimation(static_cast<unsigned short>(nSoundId), 0);
                }
                else
                {
                    CSWCCombatAttackData* pData = new CSWCCombatAttackData();
                    pData->m_nDataType = CSWCCombatAttackData::ATTACK_DATA_REACTION;
                    pData->m_reaxnGroup.m_nReaxnAnimation = static_cast<unsigned short>(nAttackId);
                    float fLength = 1.0f;
                    if (m_pModel != 0)
                        fLength = m_pModel->GetAnimationLength(static_cast<unsigned short>(nSoundId));
                    pData->m_reaxnGroup.m_nReaxnDelay = static_cast<unsigned short>(static_cast<int>(fLength));
                    pData->m_fAnimationSpeed = m_animationSpeed;
                    AddFireForgetAnimation(static_cast<unsigned short>(nSoundId), pData);
                    if (AnimationLooping(nAnimation) == 1)
                        m_loopingAnimation = nAnimation;
                }
            }
            else
            {
                if (AnimationLooping(nAnimation) == 1)
                    m_loopingAnimation = nAnimation;
            }
        }

        // Shared by both arms: a stationary, non-looping current animation with work outstanding
        // holds the new one off entirely.
        if (AnimationLooping(m_currentAnimation) == 0 &&
            AnimationStationary(m_currentAnimation) != 0 && m_nFireForgetTimeLeftMs > 0)
            return 1;
    }

    // 10. Tail.
    m_nLowerGaitAnimation = (nAnimation == SERVER_ANIMATION_RUN) ? SERVER_ANIMATION_WALK : SERVER_ANIMATION_PAUSE;
    if (m_currentAnimation != nAnimation && AsSWCCreature() != 0 &&
        AsSWCCreature()->m_blurWeapon != 0)
        AsSWCCreature()->BlurWeapon(0);

    float fBlend = 0.0f;
    float fCurrentLength = m_pModel->GetAnimationLength(m_currentAnimation);
    if (fCurrentLength != 0.0f && m_currentAnimation != nAnimation)
        fBlend = m_pModel->GetAnimationTime(m_currentAnimation) / fCurrentLength *
                 m_pModel->GetAnimationLength(nAnimation) / 1000.0f;

    m_currentAnimation = nAnimation;
    if (bResynced)
        return m_pModel->SetAnimation(nAnimation, m_animationSpeed, 2, fBlend);
    return m_pModel->SetAnimation(nAnimation, m_animationSpeed, 0, fBlend);
}

// Stores the world position, then re-evaluates visibility for the object kinds that fade
// (placeables, doors and items; a body bag the server still marks invisible opts out), mirrors the
// position into the model unless model sync is gated off, and re-walks the mesh.
int RenderPosition;
void CSWCObject::SetPosition(const Vector& v)
{
    m_position = v;
    if (RenderPosition)
        AsSWCCreature();  // the probe fed a debug draw that is not compiled in
    if (m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
    {
        CSWSPlaceable* pServer = AsSWCPlaceable()->GetServerPlaceable();
        if (pServer && pServer->m_bIsBodyBag != 0 && pServer->m_bIsBodyBagVisible == 0)
            goto skipFade;
    }
    else if (m_objectType != CGameObject::OBJECT_TYPE_DOOR &&
             m_objectType != CGameObject::OBJECT_TYPE_ITEM)
    {
        goto skipFade;
    }
    if (m_pArea != 0 && m_pArea->GetObjectsAtPositionVisible(v) != 0)
    {
        SetFadeState(1);
        SetDesiredFadeState(1, 0, 0, 0);
    }
skipFade:
    if (m_pModel != 0 && m_bModelSyncDisabled == 0)
        m_pModel->SetPosition(v);
    UpdateWalkMesh();
}

// Re-equips a creature's weapons once the animation that hid them is no longer one of the
// hide-equipped-items animations, unless the in-game GUI wants them stowed.
void CSWCObject::TestAndReEquipweapons(int a, unsigned short nAnimation)
{
    K2_UNUSED(a);
    int bHide = AnimationHideEquippedItems(nAnimation);
    if (AsSWCCreature() != 0 && AsSWCCreature()->m_bEquippedItemsHidden != 0 && bHide == 0 &&
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0)
        AsSWCCreature()->ShowEquippedItems();
}

// Render-model event callback. The only event that matters is the "done<anim>" marker for the
// playing animation: when it arrives and another animation is queued behind it, the queued one
// becomes current.
void CSWCObject::QueuedCallback(CAurObject* pObject, char* pszEvent)
{
    K2_UNUSED(pObject);
    if (m_queuedAnimation != 0 && _strnicmp(pszEvent, "DONE", 4) == 0)
    {
        char szName[64];
        sprintf(szName, "done%s", m_pModel->GetAnimationName(m_currentAnimation).CStr());
        if (_stricmp(pszEvent, szName) == 0)
        {
            m_currentAnimation = m_queuedAnimation;
            m_queuedAnimation = 0;
        }
    }
}

// True for a fire-and-forget animation: creatures and placeables short-circuit on the two
// fire-and-forget ranges, everything else goes through the animations.2da column.
int CSWCObject::AnimationFireAndForget(unsigned short a)
{
    if (m_objectType == 9 || m_objectType == 5) {
        if (static_cast<unsigned short>(a - CUTSCENE_ANIMATION_FIRE_FORGET) < CUTSCENE_ANIMATION_BANK_SIZE)
            return 1;
        if (static_cast<unsigned short>(a - CUTSCENE_ANIMATION_FIRE_FORGET_OUT_OF_PLACE) < CUTSCENE_ANIMATION_BANK_SIZE)
            return 1;
    }
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_FIREFORGET, &nEntry))
        return nEntry != 0;
    return 0;
}

// Stores a new looping animation, but only when it is valid for this object: creatures accept the
// two special ranges outright, otherwise the animations.2da looping column must be non-zero.
void CSWCObject::SetLoopingAnimation(unsigned short a)
{
    if (m_objectType == 5) {
        if (static_cast<unsigned short>(a - CUTSCENE_ANIMATION_LOOPING) < CUTSCENE_ANIMATION_BANK_SIZE)
            goto store;
        if (static_cast<unsigned short>(a - CUTSCENE_ANIMATION_LOOPING_OUT_OF_PLACE) < CUTSCENE_ANIMATION_BANK_SIZE)
            goto store;
    }
    {
        int nEntry = 0;
        unsigned short nRow = a;
        if (m_pModel)
            nRow = m_pModel->ServerToClientAnimation(a);
        if (!(g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
                  nRow, CTwoDimArrays::COL2DA_ANIMATIONS_LOOPING, &nEntry) && nEntry != 0))
            return;
    }
store:
    m_loopingAnimation = a;
}

int CSWCObject::AnimationHideEquippedItems(unsigned short a)
{
    int nEntry = 0;
    unsigned short nRow = a;
    if (m_pModel)
        nRow = m_pModel->ServerToClientAnimation(a);
    if (g_pRules->m_p2DArrays->m_pAnimations->GetINTEntry(
            nRow, CTwoDimArrays::COL2DA_ANIMATIONS_HIDEEQUIPPEDITEMS, &nEntry))
        return nEntry != 0;
    return 0;
}

// Delegates to the in-game GUI's two dialog-animation classifiers.
int CSWCObject::AnimationDialog(unsigned short a)
{
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->IsFireForgetDialogAnimation(a))
        return 1;
    return g_pAppManager->m_pClientExoApp->GetInGameGui()->IsLoopingDialogAnimation(a) != 0;
}

void CSWCObject::KillAllVisualEffects()
{
    while (m_visualEffects.m_pInternal->m_pHead != 0)
    {
        CSWCVisualEffectOnObject* p =
            static_cast<CSWCVisualEffectOnObject*>(m_visualEffects.m_pInternal->RemoveHead());
        if (p != 0)
        {
            p->KillVisualEffect();
            delete p;
        }
    }
}

void CSWCObject::StopAllVisualEffects()
{
    CExoLinkedListNode* pos = m_visualEffects.m_pInternal->m_pHead;
    void* pData = m_visualEffects.m_pInternal->m_pHead
                      ? m_visualEffects.m_pInternal->m_pHead->m_pData : 0;
    while (pData != 0)
    {
        static_cast<CSWCVisualEffectOnObject*>(pData)->StopVisualEffect(1);
        pData = m_visualEffects.m_pInternal->GetNext(pos);
    }
}

void CSWCObject::DetachVisualEffects()
{
    void* pData = m_visualEffects.m_pInternal->m_pHead
                      ? m_visualEffects.m_pInternal->m_pHead->m_pData : 0;
    CExoLinkedListNode* pos = m_visualEffects.m_pInternal->m_pHead;
    while (pos != 0)
    {
        CSWCVisualEffectOnObject* p = static_cast<CSWCVisualEffectOnObject*>(pData);
        if (p->m_bFinished == 0 && p->m_bShown != 0)
            p->DetachVisualEffect();
        pData = m_visualEffects.m_pInternal->GetNext(pos);
    }
}

void CSWCObject::ReAttachVisualEffects()
{
    void* pData = m_visualEffects.m_pInternal->m_pHead
                      ? m_visualEffects.m_pInternal->m_pHead->m_pData : 0;
    CExoLinkedListNode* pos = m_visualEffects.m_pInternal->m_pHead;
    while (pos != 0)
    {
        CSWCVisualEffectOnObject* p = static_cast<CSWCVisualEffectOnObject*>(pData);
        if (p->m_bFinished == 0 && p->m_bShown != 0)
            p->ReAttachVisualEffect();
        pData = m_visualEffects.m_pInternal->GetNext(pos);
    }
}

int CSWCObject::AddVisualEffect(unsigned short a, int b, unsigned long long c,
                                unsigned long long d, unsigned char e,
                                unsigned char f, const Vector& v)
{
    if (a != VFX_IMP_SPEED_KNIGHT || !HasVisualEffectApplied(VFX_IMP_SPEED_KNIGHT))
    {
        CSWCVisualEffectOnObject* p = new CSWCVisualEffectOnObject();
        if (!p->LoadVisualEffect(a, b, m_objectId, c, d, e, f, v))
        {
            // Torn down explicitly rather than through `delete`, which would add a null check.
            p->~CSWCVisualEffectOnObject();
            operator delete(p);
            return 0;
        }
        m_visualEffects.m_pInternal->AddTail(p);
        p->StartVisualEffect();
        p->ShowVisualEffect(0);
        p->DetachVisualEffect();
        ActivateHardCodedVisualWithHighestPriority(p->GetHardCodedEffectClass(), p);
    }
    return 1;
}

// Two passes over the effect list. The first finds the highest-priority active effect (the winner)
// of hard-coded class a and attaches or detaches that class's base effects by area darkness. The
// second detaches the losing effects and starts, stops or re-attaches the winner by its class and
// darkness state.
void CSWCObject::ActivateHardCodedVisualWithHighestPriority(unsigned char a,
                                                            CSWCVisualEffectOnObject* pParam)
{
    K2_UNUSED(pParam);
    CSWCVisualEffectOnObject* best = 0;
    unsigned char bestPri = 0xff;

    CSWCVisualEffectOnObject* p = m_visualEffects.GetHead();
    CExoLinkedListNode* pos = m_visualEffects.GetHeadPos();
    {
        while (pos != 0)
        {
            if (p->m_bFinished == 0 && p->GetHardCodedEffectClass() == a)
            {
                if (p->m_nPriority != 0)
                {
                    if (p->m_nPriority <= bestPri)
                    {
                        best = p;
                        bestPri = p->m_nPriority;
                    }
                }
                else if (p->m_bDarkOnly == 0 ||
                         g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->GetIsDark())
                {
                    if (p->m_bAttached == 0)
                        p->ReAttachVisualEffect();
                }
                else
                {
                    if (p->m_bAttached != 0)
                        p->DetachVisualEffect();
                }
            }
            p = m_visualEffects.GetNext(pos);
        }
    }

    p = m_visualEffects.GetHead();
    pos = m_visualEffects.GetHeadPos();
    {
        while (pos != 0)
        {
            K2_ASSUME(p != 0);   // the list holds only allocated effects, so a live position has data
            if (p->m_bFinished == 0 && p->GetHardCodedEffectClass() == a && p->m_nPriority != 0)
            {
                if (p != best)
                {
                    if (p->m_bAttached != 0)
                        p->DetachVisualEffect();
                }
                else
                {
                    unsigned char cls = p->GetHardCodedEffectClass();
                    if (cls == CSWCVisualEffectOnObject::HARDCODED_CLASS_LIGHT_SOURCE && p->m_bDarkOnly != 0 &&
                        !g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->GetIsDark())
                    {
                        if (p->m_bAttached != 0 && p->m_bStopped == 0)
                            p->StopVisualEffect(0);
                    }
                    else
                    {
                        if (cls == CSWCVisualEffectOnObject::HARDCODED_CLASS_LIGHT_SOURCE && p->m_bStopped != 0)
                            p->StartVisualEffect();
                        if (p->m_bAttached == 0)
                            p->ReAttachVisualEffect();
                    }
                }
            }
            p = m_visualEffects.GetNext(pos);
        }
    }
}

// Updates every effect, then removes and deletes the ones that report done; the next list position
// is captured before the removal frees the node.
void CSWCObject::UpdateVisualEffectsList()
{
    CExoLinkedListNode* pos = m_visualEffects.m_pInternal->m_pHead;
    void* pData = m_visualEffects.m_pInternal->m_pHead
                      ? m_visualEffects.m_pInternal->m_pHead->m_pData : 0;
    while (pData != 0)
    {
        CSWCVisualEffectOnObject* p = static_cast<CSWCVisualEffectOnObject*>(pData);
        p->Update(m_nAIUpdateDeltaMs);
        ActivateHardCodedVisualWithHighestPriority(p->GetHardCodedEffectClass(), 0);
        if (!p->IsDone())
        {
            pData = m_visualEffects.m_pInternal->GetNext(pos);
            continue;
        }
        if (p->m_bHasDuration == 0)
        {
            DetachVisualEffectsMatchingHardCodedClass(p);
            ActivateHardCodedVisualWithHighestPriority(p->GetHardCodedEffectClass(), 0);
        }
        CExoLinkedListNode* posNext = pos;
        pData = m_visualEffects.m_pInternal->GetNext(posNext);
        m_visualEffects.m_pInternal->Remove(pos);
        p->UnloadVisualEffect();
        p->~CSWCVisualEffectOnObject();
        operator delete(p);
        pos = posNext;
    }
}

int CSWCObject::DeleteVisualEffect(unsigned short a)
{
    CExoLinkedListNode* pos = m_visualEffects.m_pInternal->m_pHead;
    void* pData = m_visualEffects.m_pInternal->m_pHead
                      ? m_visualEffects.m_pInternal->m_pHead->m_pData : 0;
    while (pos != 0)
    {
        CSWCVisualEffectOnObject* p = static_cast<CSWCVisualEffectOnObject*>(pData);
        if (p->m_effectId == a && p->m_bFinished == 0)
        {
            DetachVisualEffectsMatchingHardCodedClass(p);
            p->StopVisualEffect(1);
            ActivateHardCodedVisualWithHighestPriority(p->GetHardCodedEffectClass(), 0);
            return 1;
        }
        pData = m_visualEffects.m_pInternal->GetNext(pos);
    }
    return 0;
}

// Detaches the first active effect whose hard-coded class matches pParam's, skipping pParam
// itself; a beam effect matches nothing.
void CSWCObject::DetachVisualEffectsMatchingHardCodedClass(CSWCVisualEffectOnObject* pParam)
{
    CExoLinkedListNode* pos = m_visualEffects.m_pInternal->m_pTail;
    if (pos != 0)
    {
        CSWCVisualEffectOnObject* p = static_cast<CSWCVisualEffectOnObject*>(pos->m_pData);
        while (true)
        {
            if (pParam->GetHardCodedEffectClass() != CSWCVisualEffectOnObject::HARDCODED_CLASS_BEAM)
            {
                unsigned char paramClass = pParam->GetHardCodedEffectClass();
                unsigned char nodeClass = p->GetHardCodedEffectClass();
                if (p != pParam && paramClass == nodeClass &&
                    p->m_nPriority != 0 && p->m_bFinished == 0)
                {
                    if (p->m_bAttached != 0)
                        p->DetachVisualEffect();
                    break;
                }
            }
            p = static_cast<CSWCVisualEffectOnObject*>(m_visualEffects.m_pInternal->GetPrev(pos));
            if (pos == 0)
                break;
        }
    }
}

// The parameter is unused; the show call always passes 0.
int CSWCObject::RefreshVisualEffects(int a)
{
    K2_UNUSED(a);
    void* pData = m_visualEffects.m_pInternal->m_pHead
                      ? m_visualEffects.m_pInternal->m_pHead->m_pData : 0;
    CExoLinkedListNode* pos = m_visualEffects.m_pInternal->m_pHead;
    while (pos != 0)
    {
        CSWCVisualEffectOnObject* p = static_cast<CSWCVisualEffectOnObject*>(pData);
        if (p->m_bFinished == 0 && p->m_bShown != 0)
        {
            p->HideVisualEffect();
            p->ShowVisualEffect(0);
        }
        pData = m_visualEffects.m_pInternal->GetNext(pos);
    }
    return 1;
}

// The alpha of the highest-priority active tint/alpha-class effect, or -1.0 when there is none.
float CSWCObject::GetVisualEffectAlpha()
{
    unsigned char best = 0xff;
    float alpha = -1.0f;
    CExoLinkedListNode* pos = m_visualEffects.m_pInternal->m_pTail;
    if (pos != 0)
    {
        CSWCVisualEffectOnObject* p = static_cast<CSWCVisualEffectOnObject*>(pos->m_pData);
        while (true)
        {
            if (p->GetHardCodedEffectClass() == CSWCVisualEffectOnObject::HARDCODED_CLASS_TINT_ALPHA && p->m_bFinished == 0 &&
                p->m_bAttached != 0 && p->m_nPriority < best)
            {
                alpha = p->m_fAlpha;
                best = p->m_nPriority;
            }
            p = static_cast<CSWCVisualEffectOnObject*>(m_visualEffects.m_pInternal->GetPrev(pos));
            if (pos == 0)
                break;
        }
    }
    return alpha;
}

int CSWCObject::HasVisualEffectApplied(unsigned int a)
{
    CExoLinkedListNode* pos = m_visualEffects.m_pInternal->m_pTail;
    if (pos != 0)
    {
        CSWCVisualEffectOnObject* p = static_cast<CSWCVisualEffectOnObject*>(pos->m_pData);
        while (true)
        {
            if (p->m_effectId == a)
                return 1;
            p = static_cast<CSWCVisualEffectOnObject*>(m_visualEffects.m_pInternal->GetPrev(pos));
            if (pos == 0)
                break;
        }
    }
    return 0;
}

// Render-model event-callback trampoline: Gob::RegisterCallBack hands the object back as the
// user-data pointer.
void QueuedCallbackFunction(CAurObject* pObject, char* pszEvent, void* pUserData) {
    static_cast<CSWCObject*>(pUserData)->QueuedCallback(pObject, pszEvent);
}

// CSWGuiInterfaceAction: resets the entry to its empty state.
void CSWGuiInterfaceAction::Clear()
{
    m_sName = "";
    m_nActionId = static_cast<unsigned int>(-1);
    m_pfnAction = 0;
    m_nObjectId = OBJECT_INVALID;
    m_resRef = const_cast<char*>("");
    m_nFlags = (m_nFlags & FLAG_KEEP_MASK) | FLAG_ENABLED;
    m_nCount = 1;
#if !KOTOR1
    m_bChangesState = 0;
#endif
}
