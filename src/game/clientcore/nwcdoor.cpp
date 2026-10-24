// The client-side door object, the local view of a server door.
#include "k2stdint.h"
#include "CExoArrayListImpl.h"

#include "nwcdoor.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwchandlemessage.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwcarea.h"
#include "nwsarea.h"
#include "nwscreature.h"
#include "nwsitem.h"
#include "nwsobject.h"
#include "nwsdoor.h"
#include "nwsplaceable.h"
#include "nwcobject.h"
#include "CExoArrayListDtors.h"
#include "nwsrules.h"
#include "nwskill.h"
#include "guiingame.h"
#include "AURORA/aurmodel.h"
#include "worldtimer.h"
#include "nwcanimbasedoor.h"
#include "serverexoapp.h"
#include "swguiexamine.h"
#include "swgui.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "exosound.h"
#include "clientaimaster.h"
#include "gameobjectarray.h"
#include "aurscene.h"
#include "aurcamera.h"
#include "swroomsurfacemesh.h"
#include "serveranimation.h"
#include "itemtype.h"
#include "soundprioritygroup.h"
#include "nwscriptconst.h"
#include "tutorial.h"

extern CExoSound* g_pExoSound;
extern int quaternionidentityoptimization;

// Rotates a vector by a quaternion, skipping the work for the identity.
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
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        r.x = v.x * m00 + v.y * m01 + v.z * m02;
        r.y = v.x * m10 + v.y * m11 + v.z * m12;
        r.z = v.x * m20 + v.y * m21 + v.z * m22;
        return r;
    }
    return v;
}

#include "port/crtapi.h"
#include "strref.h"

// Advances the server door's open state on the "door" event, or on a "done" prefix followed by
// "opening"/"closing". Every door model is registered with this animation callback.
void MoveNextDoorState(CAurObject* pObject, char* pszEvent, void* pData)
{
    K2_UNUSED(pObject);
    if (_stricmp(pszEvent, "door") != 0) {
        if (_strnicmp(pszEvent, "done", 4) != 0)
            return;
        pszEvent += 4;
        if (_strnicmp(pszEvent, "opening", 7) != 0 && _strnicmp(pszEvent, "closing", 7) != 0)
            return;
    }
    CSWCObject* pDoor = static_cast<CSWCObject*>(pData);
    if (pDoor->GetServerObject() == 0)
        return;
    CSWSDoor* pServerDoor = pDoor->GetServerObject()->AsSWSDoor();
    if (pServerDoor != 0)
        pServerDoor->MoveToNextOpenState();
}


// Starts closed and idle, stamps the current world time as its last update and seeds every field.
CSWCDoor::CSWCDoor()
    : CSWCObject(CGameObject::OBJECT_TYPE_DOOR)
{
    m_currentAnimation = SERVER_ANIMATION_CLOSED;
    unsigned int nDay, nTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;
    m_bHilightShown = 0;
    m_isAreaTransition = 0;
    m_bPlot = 0;
#if !KOTOR1
    m_bNotBlastable = 0;
#endif
    m_bLocked = 0;
    m_bLockable = 0;
    m_bHilighted = 0;
    m_bHostileFaction = 0;
    m_nDestroyedAnimVariant = 1;
    m_nAppearance = APPEARANCE_NONE;
    m_nGenericType = APPEARANCE_NONE;
    m_sModelName = "";
    m_nReserved1 = 0;
    m_nReserved2 = 0;
    m_bStatic = 0;
}

CSWCDoor::~CSWCDoor()
{
    UnloadModel();
}

CSWSDoor* CSWCDoor::GetServerDoor()
{
    if (GetServerObject())
        return GetServerObject()->AsSWSDoor();
    return 0;
}

void CSWCDoor::AIUpdate()
{
    unsigned int nDay, nTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    unsigned int nElapsedDay, nElapsedTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nLastAIUpdateDay, m_nLastAIUpdateTod, &nElapsedDay, &nElapsedTod);
    m_nAIUpdateDeltaMs = nElapsedTod;
    if (m_feedbackTimer != 0) {
        if (static_cast<unsigned int>(m_feedbackTimer) < nElapsedTod)
            m_feedbackTimer = 0;
        else
            m_feedbackTimer -= nElapsedTod;
    }
    UpdatePopUpBubble();
    AnimationUpdate();
    UpdateVisualEffectsList();
    UpdateFloatyText();
    UpdateAreaTransitionDisplay();
    if (FadeObject() != 0) {
        g_pAppManager->m_pClientExoApp->GetClientAIMaster()->m_list.RemoveObject(this);
        CGameObject* pObject;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(m_objectId, &pObject) ==
                CGameObjectArray::SUCCESS &&
            pObject != 0) {
            delete this;
            return;
        }
    }
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;
}

// Decides once per frame whether this door is the GUI's advertised area transition: it must be an
// area transition, open or opening, in front of the player and closer than the one already
// advertised. With a model loaded, the ends of the door's bounding box are rotated into world space
// and intersected in 2D with the player's 8 m look-ahead segment, and the hit must be reachable.
// Without a model a 30-degree cone plus radius test stands in.
void CSWCDoor::UpdateAreaTransitionDisplay()
{
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pGui == 0 || pPlayer == 0)
        return;
    if (m_isAreaTransition == 0)
        return;
    if (m_state != DOOR_STATE_OPEN1 && m_state != DOOR_STATE_OPEN2)
        return;

    Vector vFacing;
    if (pGui->m_bAreaTransitionUseCameraFacing != 0) {
        Quaternion q = g_pAppManager->m_pClientExoApp->GetModuleCamera()->GetOrientation();
        // The camera's rotated Y axis, flattened.
        vFacing.x = (q.x * q.y - q.w * q.z) + (q.x * q.y - q.w * q.z);
        vFacing.y = 1.0f - ((q.x * q.x + q.z * q.z) + (q.x * q.x + q.z * q.z));
        vFacing.z = 0.0f;
        vFacing = normalize(vFacing);
    } else {
        vFacing = pPlayer->m_orientation;
    }

    float fDot = (m_position.x - pPlayer->m_position.x) * vFacing.x +
                 (m_position.y - pPlayer->m_position.y) * vFacing.y +
                 (m_position.z - pPlayer->m_position.z) * vFacing.z;
    if (fDot <= 0.0f)
        return;

    if (GetModel(ANIM_MODEL_BODY, 1) == 0) {
        if (fDot >= 0.866) {
            float fDistToDoor = (m_position.x - pPlayer->m_position.x) *
                                    (m_position.x - pPlayer->m_position.x) +
                                (m_position.y - pPlayer->m_position.y) *
                                    (m_position.y - pPlayer->m_position.y) +
                                (m_position.z - pPlayer->m_position.z) *
                                    (m_position.z - pPlayer->m_position.z);
            if (fDistToDoor <= 64.0f) {
                Vector vCurrent = pGui->GetAreaTranstionPosition();
                if (pGui->m_areaTransitionObjId != OBJECT_INVALID) {
                    float fDistToCurrent = (vCurrent.x - pPlayer->m_position.x) *
                                               (vCurrent.x - pPlayer->m_position.x) +
                                           (vCurrent.y - pPlayer->m_position.y) *
                                               (vCurrent.y - pPlayer->m_position.y) +
                                           (vCurrent.z - pPlayer->m_position.z) *
                                               (vCurrent.z - pPlayer->m_position.z);
                    if (fDistToDoor < fDistToCurrent)
                        pGui->SetAreaTransition(m_objectId, m_position);
                } else {
                    pGui->SetAreaTransition(m_objectId, m_position);
                }
            }
        }
        return;
    }

    Vector vLeft;
    Vector vRight;
    GetModel(ANIM_MODEL_BODY, 1)->GetBoundingBox(vLeft, vRight);
    vLeft.y = 0.0f;
    vLeft.z = 0.0f;
    vRight.y = 0.0f;
    vRight.z = 0.0f;
    Quaternion qDoor = RotationArc(Vector(0.0f, 1.0f, 0.0f), m_orientation);
    vLeft = RotateVector(vLeft, qDoor);
    vRight = RotateVector(vRight, qDoor);
    vLeft = vLeft + m_position;
    vRight = vRight + m_position;

    Vector vHit;
    Vector vFrom = pPlayer->m_position;
    vFrom.z = 0.0f;
    Vector vAhead;
    vAhead.x = vFrom.x + vFacing.x * 8.0f;
    vAhead.y = vFrom.y + vFacing.y * 8.0f;
    vAhead.z = 0.0f;
    if (IntersectLineSegments2d(vFrom, vAhead, vLeft, vRight, vHit) == 0)
        return;

    Gob* pPlayerGob = pPlayer->GetModel(ANIM_MODEL_HEAD, 1);
    float fEyeHeight;
    if (pPlayerGob != 0)
        fEyeHeight = pPlayerGob->GetPosition().z;
    else
        fEyeHeight = pPlayer->m_position.z + 1.0f;
    vFrom.z = fEyeHeight;
    vHit.z = fEyeHeight;

    if (m_pArea != 0) {
        int bVisible;
        {
            CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
            CAurScene* pScene = m_pArea->m_pScene;
            CSWSArea* pServerArea = m_pArea->GetServerArea();
            hitInfo.m_bSkipObjects = 1;
            if (pScene->HitCheck(vFrom, vHit, 0, &hitInfo) == 0 &&
                (pServerArea == 0 ||
                 pServerArea->NoNonWalkPolys(vFrom, vHit, vFrom.z, vFrom.z, 0.0f, OBJECT_INVALID, 0,
                                             1) != 0))
                bVisible = 1;
            else
                bVisible = 0;
        }
        if (bVisible == 0)
            return;
    }

    Vector vCurrent = pGui->GetAreaTranstionPosition();
    if (pGui->m_areaTransitionObjId != OBJECT_INVALID) {
        float fDistToCurrent = (vCurrent.x - pPlayer->m_position.x) *
                                   (vCurrent.x - pPlayer->m_position.x) +
                               (vCurrent.y - pPlayer->m_position.y) *
                                   (vCurrent.y - pPlayer->m_position.y) +
                               (vCurrent.z - pPlayer->m_position.z) *
                                   (vCurrent.z - pPlayer->m_position.z);
        float fDistToHit = (vFrom.x - vHit.x) * (vFrom.x - vHit.x) +
                           (vFrom.y - vHit.y) * (vFrom.y - vHit.y) +
                           (vFrom.z - vHit.z) * (vFrom.z - vHit.z);
        if (fDistToCurrent < fDistToHit)
            pGui->SetAreaTransition(m_objectId, m_position);
    } else {
        pGui->SetAreaTransition(m_objectId, m_position);
    }
}

unsigned char CSWCDoor::GetState()
{
    return m_state;
}

// Stores the new door state and makes the model walk-through while the door is not closed.
void CSWCDoor::SetState(unsigned char state)
{
    Gob* pGob = GetModel(ANIM_MODEL_BODY, 1);
    m_state = state;
    if (!pGob)
        return;
    switch (state) {
    case DOOR_STATE_OPEN1:
    case DOOR_STATE_OPEN2:
    case DOOR_STATE_DESTROYED:
        pGob->SetIgnoreHitCheck(true);
        break;
    case DOOR_STATE_CLOSED:
        pGob->SetIgnoreHitCheck(false);
        break;
    }
}

// Drives the door's animation each frame. A fire-and-forget animation counts down and falls back to
// the looping one when it expires, and while the looping animation is current the next queued
// fire-and-forget is popped and committed to the model. The swing and destroy animations also play
// the matching placeableobjsnds row, found through the door's doortypes.2da sound appearance
// (genericdoors.2da when that row is 0). The animation then maps onto the door state, which drives
// the model's hit check.
void CSWCDoor::AnimationUpdate()
{
    if (AnimationFireAndForget(m_currentAnimation) == 1) {
        m_nFireForgetTimeLeftMs -= m_nAIUpdateDeltaMs;
        if (m_nFireForgetTimeLeftMs < 0) {
            SetAnimation(GetLoopingAnimation(), 0, 0);
            m_nFireForgetTimeLeftMs = 0;
        }
    }
    if (AnimationLooping(m_currentAnimation) == 1) {
        unsigned short nAnim;
        float fSpeed;
        unsigned int nDuration;
        void* pData;
        if (GetTopFireForgetAnimation(&nAnim, &fSpeed, &nDuration, &pData) == 1) {
            m_currentAnimation = nAnim;
            m_pModel->SetAnimation(nAnim, fSpeed, 1, 0.0f);
            m_nFireForgetTimeLeftMs = nDuration;
            RemoveTopFireForgetAnimation();
            if ((nAnim >= CSWCAnimBase::CLIENT_ANIMATION_DOOR_OPENING_1 && nAnim <= CSWCAnimBase::CLIENT_ANIMATION_DOOR_CLOSING_2) ||
                nAnim == CSWCAnimBase::CLIENT_ANIMATION_DOOR_DIE) {
                CExoString sSound;
                int nSoundRow;
                int nAppearance;
                if (m_nAppearance != 0) {
                    g_pRules->m_p2DArrays->m_pDoorTypes->GetINTEntry(
                        m_nAppearance, CTwoDimArrays::COL2DA_DOORTYPES_SOUNDAPPTYPE, &nSoundRow);
                } else {
                    nAppearance = m_nGenericType;
                    g_pRules->m_p2DArrays->m_pGenericDoors->GetINTEntry(
                        nAppearance, CTwoDimArrays::COL2DA_GENERICDOORS_SOUNDAPPTYPE, &nSoundRow);
                }
                if (nAnim == CSWCAnimBase::CLIENT_ANIMATION_DOOR_OPENING_1 || nAnim == CSWCAnimBase::CLIENT_ANIMATION_DOOR_OPENING_2)
                    g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                        nSoundRow, CTwoDimArrays::COL2DA_PLACEABLESOUNDS_OPENED, &sSound);
                else if (nAnim == CSWCAnimBase::CLIENT_ANIMATION_DOOR_CLOSING_1 || nAnim == CSWCAnimBase::CLIENT_ANIMATION_DOOR_CLOSING_2)
                    g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                        nSoundRow, CTwoDimArrays::COL2DA_PLACEABLESOUNDS_CLOSED, &sSound);
                else if (nAnim == CSWCAnimBase::CLIENT_ANIMATION_DOOR_DIE)
                    g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                        nSoundRow, CTwoDimArrays::COL2DA_PLACEABLESOUNDS_DESTROYED, &sSound);
                g_pExoSound->Play3DOneShotSound(CResRef(sSound), m_position, 1.5f, PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL, 0, 0,
                                                0.0f, 0.0f);
            }
        }
        switch (m_currentAnimation) {
        case SERVER_ANIMATION_CLOSED:
            SetState(DOOR_STATE_CLOSED);
            break;
        case SERVER_ANIMATION_DOOR_OPEN1:
            SetState(DOOR_STATE_OPEN1);
            break;
        case SERVER_ANIMATION_DOOR_OPEN2:
            SetState(DOOR_STATE_OPEN2);
            break;
        case SERVER_ANIMATION_DESTROYED:
            SetState(DOOR_STATE_DESTROYED);
            break;
        }
        // The popped record has no destructor; only the creature's twin ever reads the payload.
        if (pData != 0) {
            operator delete(pData);
            pData = 0;
        }
    }
}

int CSWCDoor::LoadModel(const CResRef& r, unsigned char a, unsigned char b)
{
    K2_UNUSED(b);
    if (m_pModel == 0) {
        CSWCAnimBaseDoor* pAnimBase = new CSWCAnimBaseDoor();
        m_pModel = pAnimBase;
        pAnimBase->m_nDestroyedAnimVariant = m_nDestroyedAnimVariant;
    }
    if (m_pModel->LoadModel(r, a) == 0)
        return 0;
    Gob* pGob = m_pModel->GetModel(a);
    if (pGob != 0) {
        pGob->SetExternalData(this);
        pGob->SetExternalDataType(CGameObject::OBJECT_TYPE_DOOR);
        pGob->RegisterCallBack(MoveNextDoorState, const_cast<char*>(""), this, 9999.0f, false);
#if !KOTOR1  // K1 has no non-force-blending flag
        pGob->SetNonForceBlending(true);
#endif
    }
    r.CopyToString(m_sModelName);
    return 1;
}

// Turns the door's area-transition geometry off by amputating the model's "trans" node.
void CSWCDoor::SetIsAreaTransition(int b)
{
    if (b == 0) {
        Gob* pGob = GetModel(ANIM_MODEL_BODY, 1);
        if (pGob)
            pGob->Amputate(const_cast<char*>("trans"));
    }
    m_isAreaTransition = b;
}

// Sets the current animation without firing its trigger.
void CSWCDoor::SetAnimationWithoutTrigger(unsigned short anim)
{
    m_currentAnimation = anim;
    if (AnimationLooping(anim))
        SetLoopingAnimation(anim);
    m_pModel->SetAnimation(anim, 0.0f, 0, 0.0f);
}

void CSWCDoor::UnloadModel(unsigned char a)
{
    if (m_pModel)
        m_pModel->UnloadModel(a);
}

void CSWCDoor::UnloadModel()
{
    if (m_pModel)
    {
        m_pModel->UnloadModel();
        delete m_pModel;
        m_pModel = 0;
    }
}

void CSWCDoor::ToggleLockState()
{
}

void CSWCDoor::HilightDoor(int a)
{
    K2_UNUSED(a);
}

void CSWCDoor::SetMineIcon(unsigned char a)
{
    K2_UNUSED(a);
}

void CSWCDoor::AddFireAndForgetAnimation(unsigned short a, void* p, unsigned short b)
{
    K2_UNUSED(b);
    float speed = m_animationSpeed;
    float len = GetAnimationLength(a);
    if (speed != 0.0f)
        len = len / __builtin_fabsf(m_animationSpeed);
    InsertFireForgetAnimation(a, static_cast<unsigned int>(len), m_animationSpeed, p, 0);
}

void CSWCDoor::ActionMenuExamine(unsigned int a, CSWCCreature* pCreature)
{
    K2_UNUSED(a);
    if (pCreature)
        pCreature->ClearAllActions();
    if (GetServerObject()) {
        CSWSDoor* pServerDoor = GetServerObject()->AsSWSDoor();
        if (pServerDoor) {
            CExoString text;
            pServerDoor->m_description.GetString(
                g_pAppManager->m_pClientExoApp->GetClientLanguage(), &text, 0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowExamineBox(text, 0);
        }
    }
}

// Asks the server to unlock this door, with no facing value.
void CSWCDoor::MenuActionSecurity(unsigned int a, CSWCCreature* pCreature)
{
    K2_UNUSED(a);
    if (pCreature)
        pCreature->ClearAllActions();
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_UnlockObject(
        m_objectId, OBJECT_INVALID);
    g_pAppManager->m_pClientExoApp->DelayNextEnemyAutoPause();
}

void CSWCDoor::DoorSaber(unsigned int a, CSWCCreature* pCreature)
{
    K2_UNUSED(a);
    if (pCreature)
        pCreature->ClearAllActions();
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_DoorSaber(
        m_objectId);
}

// Requests the closing state from an opening or opened animation, the opening state from closed.
void CSWCDoor::ToggleDoorState(unsigned int a, CSWCCreature* pCreature)
{
    K2_UNUSED(a);
    if (pCreature)
        pCreature->ClearAllActions();
    CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (m_currentAnimation == SERVER_ANIMATION_DOOR_OPEN1 || m_currentAnimation == SERVER_ANIMATION_DOOR_OPEN2) {
        pMessage->SendPlayerToServerInput_ChangeDoorState(m_objectId, SERVER_ANIMATION_CLOSED);
        return;
    }
    if (m_currentAnimation == SERVER_ANIMATION_CLOSED) {
        pMessage->SendPlayerToServerInput_ChangeDoorState(m_objectId, SERVER_ANIMATION_OPEN);
        g_pAppManager->m_pClientExoApp->DelayNextEnemyAutoPause();
    }
}

// Shows the bash tutorial window when it has not been seen, otherwise attacks the door.
void CSWCDoor::MenuActionBash(unsigned int a, CSWCCreature* pCreature)
{
    K2_UNUSED(a);
    if (pCreature) {
        if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(
                TUTORIAL_BASH, pCreature->m_objectId, m_objectId, 1) == 0) {
            pCreature->ClearAllActions();
            pCreature->PerformAttack(m_objectId);
            g_pAppManager->m_pClientExoApp->DelayNextEnemyAutoPause();
        }
    }
}

// Sets a mine on this door. When the player's area forbids mines the shared message box goes up
// instead, otherwise the item is handed to the acting creature's use-item path at the door. The
// item bit is masked off the incoming id; GetDoorInterfaceMines sets it.
void CSWCDoor::MenuActionSetDoorMine(unsigned int itemId, CSWCCreature* pCreature)
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pPlayer =
        pServerApp->GetCreatureByGameObjectID(pServerApp->GetPlayerCreatureId());
    if (pPlayer == 0 || pPlayer->GetArea() == 0 ||
        static_cast<CSWSArea*>(pPlayer->GetArea())->GetPlayerCanSetMines() != 0) {
        itemId &= CSWGuiInterfaceAction::ACTION_ID_ITEM_CLEAR;
        if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_SET_MINE, m_objectId, itemId,
                                                               0) == 0)
            pCreature->PerformUseItem(itemId, 0, m_objectId, m_position);
    } else {
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetMessage(STRREF_MINE_LIMIT);
        pBox->SetCallback(0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
    }
}

// Adds the acting creature's security spikes as "use an item" entries, restamped with this door's
// action and id.
void CSWCDoor::GetUseItems(CSWCCreature* pCreature,
                           CExoArrayList<CSWGuiInterfaceAction>& list)
{
    CSWSCreature* pServerCreature = pCreature->GetServerCreature();
    if (m_bLocked != 0) {
        int nOldCount = list.m_count;
        if (pCreature->m_pStats->GetCanUseSkill(SKILL_SECURITY) != 0) {
            if (pServerCreature != 0)
                pServerCreature->GetSpikeDetails(list, 1);
            for (int i = nOldCount; i < list.m_count; i++) {
                list.m_pArray[i].m_pfnAction =
                    static_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCObject::MenuActionUseSecuritySpike);
                list.m_pArray[i].m_nObjectId = m_objectId;
            }
        }
    }
}

// Builds the door's radial-menu entries for a creature: 2 mines, 1 security (an unlock entry plus
// the creature's security spikes), 0 physical (cut the door open with a lightsaber, otherwise
// bash it).
void CSWCDoor::GetTargetActions(CSWCCreature* pCreature, int nType,
                                CExoArrayList<CSWGuiInterfaceAction>& list)
{
    CSWSDoor* pServerDoor =
        GetServerObject() ? GetServerObject()->AsSWSDoor() : static_cast<CSWSDoor*>(0);
    int nAreaMenuDisabled = m_pArea->GetServerArea()->m_nPlayerRestrictMode;
    int nOldCount = list.m_count;
    CExoString sName;

#if KOTOR1  // K1 has only the security and bash entries
    if (nType != 1) {
        if (nType == 0 && GetCurrentAnimation() != SERVER_ANIMATION_DOOR_OPEN1 &&
            GetCurrentAnimation() != SERVER_ANIMATION_DOOR_OPEN2 &&
            m_bStatic == 0 && m_bPlot == 0 && nAreaMenuDisabled == 0 && m_bLocked != 0) {
            list.SetSize(nOldCount + 1);
            list.m_pArray[nOldCount].m_sName =
                g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BASH);
            list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_BASH;
            list.m_pArray[nOldCount].m_pfnAction =
                reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCDoor::MenuActionBash);
            list.m_pArray[nOldCount].m_nObjectId = m_objectId;
            list.m_pArray[nOldCount].m_resRef = const_cast<char*>("i_attack");
        }
    } else if (pServerDoor != 0 && m_bLocked != 0 && pServerDoor->m_bKeyRequired == 0 &&
               pCreature->m_pStats->GetCanUseSkill(SKILL_SECURITY) != 0) {
        list.SetSize(nOldCount + 1);
        list.m_pArray[nOldCount].m_sName =
            g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SECURITY);
        list.m_pArray[nOldCount].m_pfnAction =
            reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCDoor::MenuActionSecurity);
        list.m_pArray[nOldCount].m_nObjectId = m_objectId;
        list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_SECURITY;
        list.m_pArray[nOldCount].m_resRef = g_pRules->m_paSkills[6].m_icon;
    }
#else
    if (nType != 2) {
        if (nType != 1) {
            if (nType != 0)
                return;
            if (GetCurrentAnimation() == SERVER_ANIMATION_DOOR_OPEN1 || GetCurrentAnimation() == SERVER_ANIMATION_DOOR_OPEN2)
                return;
            if (m_bStatic != 0)
                return;

            int bAdded = 0;
            if (m_bLocked != 0 && m_bPlot == 0) {
                CSWSCreature* pServerCreature = pCreature->GetServerCreature();
                CSWItem* pItem = pServerCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON);
                if (pItem != 0) {
                    CSWBaseItem* pBaseItem = pItem->GetBaseItem();
#if KOTOR1  // K1 has no door-sabering item property
                    if (static_cast<unsigned char>(pBaseItem->m_nItemType - ITEM_TYPE_DOUBLE_LIGHTSABER) < ITEM_TYPE_LIGHTSABER_COUNT) {
#else
                    if (static_cast<CSWSItem*>(pItem)->m_bDoorSabering != 0 ||
                        static_cast<unsigned char>(pBaseItem->m_nItemType - ITEM_TYPE_DOUBLE_LIGHTSABER) < ITEM_TYPE_LIGHTSABER_COUNT) {
#endif
                        list.SetSize(nOldCount + 1);
                        list.m_pArray[nOldCount].m_sName =
                            g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_USE_LIGHTSABER);
                        list.m_pArray[nOldCount].m_pfnAction =
                            reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCDoor::DoorSaber);
                        list.m_pArray[nOldCount].m_nObjectId = m_objectId;
                        list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_DOOR_SABER;
                        list.m_pArray[nOldCount].m_resRef = const_cast<char*>("i_doorsaber");
                        bAdded = 1;
                    }
                }
            }
            if (m_bPlot == 0 && nAreaMenuDisabled == 0 && bAdded == 0 && m_bLocked != 0) {
                list.SetSize(nOldCount + 1);
                list.m_pArray[nOldCount].m_sName =
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BASH);
                list.m_pArray[nOldCount].m_pfnAction =
                    reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCDoor::MenuActionBash);
                list.m_pArray[nOldCount].m_nObjectId = m_objectId;
                list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_BASH;
                list.m_pArray[nOldCount].m_resRef = const_cast<char*>("i_attack");
            }
            return;
        }
        if (pServerDoor != 0 && m_bLocked != 0 && pServerDoor->m_bKeyRequired == 0 &&
            pCreature->m_pStats->GetCanUseSkill(SKILL_SECURITY) != 0) {
            list.SetSize(nOldCount + 1);
            list.m_pArray[nOldCount].m_sName =
                g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SECURITY);
            list.m_pArray[nOldCount].m_pfnAction =
                reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCDoor::MenuActionSecurity);
            list.m_pArray[nOldCount].m_nObjectId = m_objectId;
            list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_SECURITY;
            list.m_pArray[nOldCount].m_resRef = g_pRules->m_paSkills[6].m_icon;
        }
        if (pServerDoor != 0 && m_bLocked != 0 && pServerDoor->m_bKeyRequired == 0 &&
            pCreature->m_pStats->GetCanUseSkill(SKILL_SECURITY) != 0) {
            CSWSCreature* pServerCreature = pCreature->GetServerCreature();
            if (m_bLocked != 0) {
                int nSpikeOldCount = list.m_count;
                if (pCreature->m_pStats->GetCanUseSkill(SKILL_SECURITY) != 0) {
                    if (pServerCreature != 0)
                        pServerCreature->GetSpikeDetails(list, 1);
                    for (int i = nSpikeOldCount; i < list.m_count; i++) {
                        list.m_pArray[i].m_pfnAction =
                            static_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCObject::MenuActionUseSecuritySpike);
                        list.m_pArray[i].m_nObjectId = m_objectId;
                    }
                }
            }
        }
        return;
    }
    if (m_bLocked != 0)
        GetDoorInterfaceMines(pCreature, list);
#endif
}

// Builds the door's mine entries, one per mine in the acting creature's inventory, gated on a
// mine-capable door and the Demolitions skill. An entry's text is the item name, the localized
// "(mine)" wrapper and the stack count.
#if !KOTOR1
void CSWCDoor::GetDoorInterfaceMines(CSWCCreature* pCreature,
                                     CExoArrayList<CSWGuiInterfaceAction>& list)
{
    if (m_bNotBlastable != 0)
        return;
    CSWSCreature* pServerCreature = pCreature->GetServerCreature();
    if (pServerCreature == 0)
        return;
    if (static_cast<CSWSArea*>(pServerCreature->GetArea())->m_nPlayerRestrictMode != 0)
        return;
    if (pCreature->m_pStats->GetCanUseSkill(SKILL_DEMOLITIONS) == 0)
        return;

    CClientExoApp* pExoApp = g_pAppManager->m_pClientExoApp;
    int nOldCount = list.m_count;
    int nLanguage = pExoApp->GetClientLanguage();
    CItemRepository* pRepo = pServerCreature->GetItemRepository(1);
    CExoString sName;
    CExoString sCount;
    CExoString sHeader;
    sHeader.Format(const_cast<char*>(" (%s)"), pExoApp->GetGUIString(STRREF_DOOR).CStr());

    CSWSItem* pItem;
    CSWBaseItem* pBaseItem;
    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
        pItem = pRepo->ItemListGetItem(i);
        if (pItem->m_bPlot != 0)
            continue;
        pBaseItem = static_cast<CSWItem*>(pItem)->GetBaseItem();
        if (pBaseItem->m_nItemType != ITEM_TYPE_TRAP_KIT)
            continue;
        switch (pItem->GetActiveProperty(0)->m_subType) {
        case TRAP_BASE_TYPE_FRAGMENTATION_MINE_MINOR: case TRAP_BASE_TYPE_FRAGMENTATION_MINE_AVERAGE:
        case TRAP_BASE_TYPE_FRAGMENTATION_MINE_DEADLY: case TRAP_BASE_TYPE_LASER_SLICING_MINOR:
        case TRAP_BASE_TYPE_LASER_SLICING_AVERAGE: case TRAP_BASE_TYPE_LASER_SLICING_DEADLY:
        case TRAP_BASE_TYPE_SONIC_CHARGE_MINOR: case TRAP_BASE_TYPE_SONIC_CHARGE_AVERAGE:
        case TRAP_BASE_TYPE_SONIC_CHARGE_DEADLY: case TRAP_BASE_TYPE_FRAGMENTATION_MINE_STRONG:
        case TRAP_BASE_TYPE_FRAGMENTATION_MINE_DEVASTATING: case TRAP_BASE_TYPE_LASER_SLICING_STRONG:
        case TRAP_BASE_TYPE_LASER_SLICING_DEVASTATING: case TRAP_BASE_TYPE_SONIC_CHARGE_STRONG:
        case TRAP_BASE_TYPE_SONIC_CHARGE_DEVASTATING:
            break;
        default:
            continue;
        }
        pItem->m_localizedName.GetString(nLanguage, &sName, 0);
        if (pItem->m_stackCount >= 2)
            sCount.Format(const_cast<char*>(" (%d)"), pItem->m_stackCount);
        else
            sCount = const_cast<char*>("");
        sName = sName + sHeader + sCount;
        list.SetSize(nOldCount + 1);
        list.m_pArray[nOldCount].m_sName = sName;
        list.m_pArray[nOldCount].m_pfnAction =
            reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCDoor::MenuActionSetDoorMine);
        list.m_pArray[nOldCount].m_nObjectId = m_objectId;
        list.m_pArray[nOldCount].m_nActionId =
            static_cast<unsigned int>(pItem->m_objectId) | CSWGuiInterfaceAction::ACTION_ID_ITEM;
        list.m_pArray[nOldCount].m_resRef = pItem->GetIcon();
        nOldCount++;
    }
}
#endif

CSWCDoor* CSWCDoor::AsSWCDoor()
{
    return this;
}

CExoString CSWCDoor::GetName()
{
    return m_name;
}
