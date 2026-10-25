#include "k2stdint.h"
#include "CExoArrayListImpl.h"

#include "nwctrigger.h"
#include "nwccreature.h"
#include "appmanager.h"
#include "worldtimer.h"
#include "gameobjectarray.h"
#include "aurcamera.h"
#include "swgui.h"
#include "AURORA/aurmodel.h"
#include "clientexoapp.h"
#include "nwchandlemessage.h"
#include "nwccreaturestats.h"
#include "nwstrigger.h"
#include "nwcobject.h"
#include "guiingame.h"
#include "aurscene.h"
#include "CExoArrayListDtors.h"
#include "nwsrules.h"
#include "nwspell.h"
#include "nwscreature.h"
#include "nwsarea.h"
#include "nwseffectlist.h"
#include "nwseffect.h"
#include "nwcarea.h"
#include "nwcmodule.h"
#include "nwarea.h"
#include "nwcanimbasetrigger.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "nwscriptconst.h"
#include "strref.h"

// Aurora trigger free functions, defined in aurtrigger.cpp.
PartTrigger* AurPartTriggerCreate(CAurScene* pScene, Vector* paVertices, int nVertices, int bSkipGroundProbe,
                                  float* pafColor, void* pOwner);
void AurPartTriggerDelete(CAurScene* pScene, PartTrigger* pPartTrigger, bool bDelete);
void AurPartTriggerSetHighlightParams(PartTrigger* pPartTrigger, const Vector& vColor, float fAlpha,
                                      bool bPulse, float fPulseRate, bool bAdditive);
void AurPartTriggerSetHighlight(PartTrigger* pPartTrigger, bool bHighlight, float fHeight, bool bAffectDynamic);
// Frees a PartTrigger through its own allocator; the destructor releases m_pPartTrigger with it.
void AurPartTriggerReleaseMemory(PartTrigger* pPartTrigger);
CSWCTrigger::~CSWCTrigger()
{
    if (m_pModel != 0)
    {
        m_pModel->UnloadModel();
        delete m_pModel;
        m_pModel = 0;
    }
    if (m_pVertices != 0)
    {
        delete m_pVertices;
        m_pVertices = 0;
    }
    if (m_pPartTrigger != 0)
    {
        AurPartTriggerReleaseMemory(m_pPartTrigger);
        m_pPartTrigger = 0;
    }
}

// Ages the feedback timer off the world clock, then either counts down the fade-out latch and
// removes the trigger from the object array once it expires, or updates position, pop-up, transition
// and animation.
void CSWCTrigger::AIUpdate()
{
    unsigned int nDay, nTod;
    unsigned int nElapsedDay, nElapsedTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nLastAIUpdateDay, m_nLastAIUpdateTod, &nElapsedDay, &nElapsedTod);

    if (m_feedbackTimer != 0) {
        if (static_cast<unsigned int>(m_feedbackTimer) < m_nAIUpdateDeltaMs)
            m_feedbackTimer = 0;
        else
            m_feedbackTimer -= m_nAIUpdateDeltaMs;
    }

    m_nAIUpdateDeltaMs = nElapsedTod;

    if (m_bPendingRemoval != 0) {
        if (m_nFadeOutTimer > m_nAIUpdateDeltaMs) {
            m_nFadeOutTimer -= m_nAIUpdateDeltaMs;
        } else {
            CGameObject* pObject;
            if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(m_objectId, &pObject)
                    == CGameObjectArray::SUCCESS && pObject != 0)
                delete this;
        }
    } else {
        UpdatePosition();
        UpdatePopUpBubble();
        UpdateAreaTransitionDisplay();
        AnimationUpdate();

        g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
        m_nLastAIUpdateDay = nDay;
        m_nLastAIUpdateTod = nTod;
    }
}

// The trigger's area-transition billboard. When the trigger is transition-enabled and the player is
// nearby, it casts the player's flattened facing ray at the trigger's ground polygon and checks the
// hit point is visible (a scene hitcheck plus the server area's walkmesh test), then asks the GUI to
// advertise the transition, unless a nearer point already is.
void CSWCTrigger::UpdateAreaTransitionDisplay()
{
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pGui == 0 || pPlayer == 0)
        return;
    if (m_bAreaTransition == 0)
        return;

    Vector vHit;
    Vector vFacing;
    if (pGui->m_bAreaTransitionUseCameraFacing != 0) {
        // The camera's rotated Y axis, flattened.
        vFacing = g_pAppManager->m_pClientExoApp->GetModuleCamera()->GetOrientation().ydir();
        vFacing.z = 0.0f;
        vFacing = normalize(vFacing);
    } else {
        vFacing = pPlayer->m_orientation;
    }

    Vector vFrom = pPlayer->m_position;
    vFrom.z = 0.0f;
    if (m_pArea == 0)
        return;
    Vector vAhead;
    vAhead.x = pPlayer->m_position.x + vFacing.x * 8.0f;
    vAhead.y = pPlayer->m_position.y + vFacing.y * 8.0f;
    vAhead.z = 0.0f;
    if (LineSegmentIntersectTrigger(vFrom, vAhead, &vHit) == 0)
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
        float fDistToHit = (vHit.x - vFrom.x) * (vHit.x - vFrom.x) +
                           (vHit.y - vFrom.y) * (vHit.y - vFrom.y) +
                           (vHit.z - vFrom.z) * (vHit.z - vFrom.z);
        // A hit point nearer than the advertised one wins.
        if (fDistToHit < fDistToCurrent)
            pGui->SetAreaTransition(m_objectId, vHit);
    } else {
        pGui->SetAreaTransition(m_objectId, vHit);
    }
}

// Advances the trigger's animation state one frame. A fire-and-forget animation that ran out latches
// the fade-out and the 500 ms hold; otherwise a looping animation whose queued fire-and-forget entry
// is ready is promoted into m_currentAnimation.
void CSWCTrigger::AnimationUpdate()
{
    if (AnimationFireAndForget(m_currentAnimation) == 1) {
        m_nFireForgetTimeLeftMs -= m_nAIUpdateDeltaMs;
        if (m_nFireForgetTimeLeftMs < 0) {
            m_bPendingRemoval = 1;
            m_nFadeOutTimer = 500;
            m_nFireForgetTimeLeftMs = 0;
            return;
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
        }
        if (pData != 0) {
            operator delete(pData);
            pData = 0;
        }
    }
}

CSWCTrigger::CSWCTrigger()
    : CSWCObject(CGameObject::OBJECT_TYPE_TRIGGER)
{
    unsigned int nDay, nTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;
    m_nVertices = 0;
    m_pVertices = 0;
    m_bIsTrap = 0;
    m_bAreaTransition = 0;
    m_displayState = 0;
    m_fHighlightHeight = 0.0f;
    m_nReserved1 = 0;
    m_nTrapType = TRAP_TYPE_NONE;
    m_pPartTrigger = 0;
}

CSWSTrigger* CSWCTrigger::GetServerTrigger()
{
    if (GetServerObject())
        return GetServerObject()->AsSWSTrigger();
    return 0;
}

int CSWCTrigger::IsHostile()
{
    return m_bHostile;
}

void CSWCTrigger::UnloadModel(unsigned char a)
{
    if (m_pModel)
        m_pModel->UnloadModel(a);
}

void CSWCTrigger::DisplayTrigger(int a)
{
    if (m_displayState != a)
    {
        m_displayState = a;
        UpdateTriggerColor(a);
    }
}

int CSWCTrigger::GetMineStrengthLevel(int a)
{
    static const int levels[27] = {
        0, 1, 3, 0, 1, 3, 0, 1, 3, 0, 1, 3, -1, -1,
        0, 1, 3, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4,
    };
    if (static_cast<unsigned int>(a) <= 26)
        return levels[a];
    return -1;
}

void CSWCTrigger::ActionMenuRecoverMine(unsigned int a, CSWCCreature* c)
{
    K2_UNUSED(a);
    c->PerformSkill(SKILL_DEMOLITIONS, SUBSKILL_RECOVERTRAP, m_objectId);
}

void CSWCTrigger::UnloadModel()
{
    if (m_pModel)
    {
        m_pModel->UnloadModel();
        delete m_pModel;
        m_pModel = 0;
    }
}

void CSWCTrigger::SetMineIcon(unsigned char a)
{
    K2_UNUSED(a);
}

void CSWCTrigger::AddFireAndForgetAnimation(unsigned short a, void* p, unsigned short b)
{
    K2_UNUSED(b);
    InsertFireForgetAnimation(a, static_cast<unsigned int>(GetAnimationLength(a)), 1.0f, p, 0);
}

// Menu "disable mine" action: sends UseSkill (skill 1 / subtype 0) at this trigger with a zeroed
// target position.
void CSWCTrigger::ActionMenuDisableMine(unsigned int a, CSWCCreature* c)
{
    K2_UNUSED(a); K2_UNUSED(c);
    CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMessage)
        pMessage->SendPlayerToServerInput_UseSkill(1, 0, m_objectId, Vector{0.0f, 0.0f, 0.0f});
}

// Builds this trigger's radial-menu entries for pCreature, selected by nType:
//   0: "disable mine" entry, gated on the creature's Demolitions skill and this trigger being
//      hostile (the virtual IsHostile).
//   1: "recover mine" entry, gated on Demolitions only.
//   2: mine-flavored force powers, selected from the server trigger's trap type.
void CSWCTrigger::GetTargetActions(CSWCCreature* pCreature, int nType,
                                   CExoArrayList<CSWGuiInterfaceAction>& list)
{
    int nOldCount = list.m_count;
#if !KOTOR1  // K1 has no mine force powers (no nType 2 arm)
    if (nType != 2) {
#endif
        if (nType != 1) {
            if (nType != 0)
                return;
            if (pCreature->m_pStats->GetCanUseSkill(SKILL_DEMOLITIONS) == 0)
                return;
            if (IsHostile() == 0)
                return;
            list.SetSize(nOldCount + 1);
            list.m_pArray[nOldCount].m_sName =
                g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ACTION_DISABLE_MINE);
            list.m_pArray[nOldCount].m_pfnAction =
                reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCTrigger::ActionMenuDisableMine);
            list.m_pArray[nOldCount].m_nObjectId = m_objectId;
            list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_DISABLE_MINE;
            list.m_pArray[nOldCount].m_resRef = const_cast<char*>("i_disablemine");
            return;
        }
        if (pCreature->m_pStats->GetCanUseSkill(SKILL_DEMOLITIONS) == 0)
            return;
        list.SetSize(nOldCount + 1);
        list.m_pArray[nOldCount].m_sName =
            g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_RECOVER_MINE);
        list.m_pArray[nOldCount].m_pfnAction =
            reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCTrigger::ActionMenuRecoverMine);
        list.m_pArray[nOldCount].m_nObjectId = m_objectId;
        list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_RECOVER_MINE;
        list.m_pArray[nOldCount].m_resRef = const_cast<char*>("i_recovermine");
        return;
#if !KOTOR1
    }
    {
        GetServerObject();
        CSWSTrigger* pServerTrigger = GetServerObject()->AsSWSTrigger();
        int nLevel = -1;
        switch (pServerTrigger->m_trapType) {
        case TRAP_BASE_TYPE_FLASH_STUN_MINOR: case TRAP_BASE_TYPE_FRAGMENTATION_MINE_MINOR:
        case TRAP_BASE_TYPE_LASER_SLICING_MINOR: case TRAP_BASE_TYPE_POISON_GAS_MINOR:
        case TRAP_BASE_TYPE_SONIC_CHARGE_MINOR:
            nLevel = 0;
            break;
        case TRAP_BASE_TYPE_FLASH_STUN_AVERAGE: case TRAP_BASE_TYPE_FRAGMENTATION_MINE_AVERAGE:
        case TRAP_BASE_TYPE_LASER_SLICING_AVERAGE: case TRAP_BASE_TYPE_POISON_GAS_AVERAGE:
        case TRAP_BASE_TYPE_SONIC_CHARGE_AVERAGE:
            nLevel = 1;
            break;
        case TRAP_BASE_TYPE_FLASH_STUN_STRONG: case TRAP_BASE_TYPE_FRAGMENTATION_MINE_STRONG:
        case TRAP_BASE_TYPE_LASER_SLICING_STRONG: case TRAP_BASE_TYPE_POISON_GAS_STRONG:
        case TRAP_BASE_TYPE_SONIC_CHARGE_STRONG:
            nLevel = 2;
            break;
        case TRAP_BASE_TYPE_FLASH_STUN_DEADLY: case TRAP_BASE_TYPE_FRAGMENTATION_MINE_DEADLY:
        case TRAP_BASE_TYPE_LASER_SLICING_DEADLY: case TRAP_BASE_TYPE_POISON_GAS_DEADLY:
        case TRAP_BASE_TYPE_SONIC_CHARGE_DEADLY:
            nLevel = 3;
            break;
        case TRAP_BASE_TYPE_FLASH_STUN_DEVASTATING: case TRAP_BASE_TYPE_FRAGMENTATION_MINE_DEVASTATING:
        case TRAP_BASE_TYPE_LASER_SLICING_DEVASTATING: case TRAP_BASE_TYPE_POISON_GAS_DEVASTATING:
        case TRAP_BASE_TYPE_SONIC_CHARGE_DEVASTATING:
            nLevel = 4;
            break;
        }
        int bMatched = 0;
        if (nLevel >= 0 && nLevel <= 2) {
            if (pCreature->m_pStats->HasSpell(0, FORCE_POWER_DROID_DESTROY)) {
                GetMineForcePowers(pCreature, FORCE_POWER_DROID_DESTROY, list);
                bMatched = 1;
            } else if (nLevel > 1) {
                // nLevel == 2: the tier chain has no Droid Disable fallback.
            } else if (pCreature->m_pStats->HasSpell(0, FORCE_POWER_DROID_DISABLE)) {
                GetMineForcePowers(pCreature, FORCE_POWER_DROID_DISABLE, list);
                bMatched = 1;
            }
        }
        if (bMatched | nLevel)
            return;
        if (pCreature->m_pStats->HasSpell(0, FORCE_POWER_DROID_STUN))
            GetMineForcePowers(pCreature, FORCE_POWER_DROID_STUN, list);
    }
#endif
}

// Builds one radial-menu entry for a mine force power (spell id nSpellId) castable at this trigger,
// selected from the trigger's trap type. The entry carries the hostile force power stamp: GUI name
// from the spell row, ForceMenuHostileAction callback, action id with the spell bit, this trigger's
// object id. Whether the entry greys out depends on the spell's adjusted FP cost against the caster's
// FP pool and current HP, with a Force Body effect letting HP cover a discounted share.
#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWCTrigger::GetMineForcePowers(CSWCCreature* pCreature, int nSpellId,
                                     CExoArrayList<CSWGuiInterfaceAction>& list)
{
    CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
    CSWSCreature* pServer = pCreature->GetServerCreature();
    if (pSpell == 0 || pServer == 0)
        return;

    int nRoomForceRating = 0;
    CSWSArea* pArea = static_cast<CSWSArea*>(pServer->GetArea());
    if (pArea != 0)
        nRoomForceRating = pArea->GetRoomForceRating(pServer->m_position);
    int nAlignment = pCreature->m_pStats->m_alignmentGoodEvil;
    int nOldCount = list.m_count;

    int nDiscountTier = 0;
    for (int64_t i = 0; i < pServer->m_effects.m_count; i++) {
        if (pServer->m_effects.m_pArray[i]->m_subType == CSWSEffectListHandler::EFFECT_FORCE_BODY) {
            nDiscountTier = pServer->m_effects.m_pArray[i]->GetInteger(0);
            break;
        }
    }

    int nStanceId = pServer->m_stanceId;
    char cCHAMod = pServer->m_pStats->m_nCHAMod;
    short nCurrentHP = pServer->GetCurrentHitPoints(0);
    int nForcePoints = static_cast<short>(pServer->m_pStats->m_nCurrentForcePoints + pServer->m_pStats->m_nTemporaryForcePoints);

    list.SetSize(nOldCount + 1);
    list.m_pArray[nOldCount].m_sName =
        g_pAppManager->m_pClientExoApp->GetGUIString(pSpell->m_nNameStrRef);
    list.m_pArray[nOldCount].m_resRef = pSpell->m_iconResRef;
    list.m_pArray[nOldCount].m_nFlags &= ~CSWGuiInterfaceAction::FLAG_REASON_MASK;
    list.m_pArray[nOldCount].m_nActionId = nSpellId | CSWGuiInterfaceAction::ACTION_ID_SPELL;
    list.m_pArray[nOldCount].m_pfnAction = &CSWCCreature::ForceMenuHostileAction;
    list.m_pArray[nOldCount].m_nObjectId = m_objectId;

    int nAdjustedFP = pSpell->GetAdjustedForcePoints(nAlignment, 1,
                                                     nRoomForceRating, nStanceId, cCHAMod);

    unsigned char bEnabled = 0;
    if (nDiscountTier == -1) {
        if (static_cast<unsigned int>(nForcePoints) < static_cast<unsigned int>(nAdjustedFP))
            list.m_pArray[nOldCount].m_nFlags = (list.m_pArray[nOldCount].m_nFlags & ~CSWGuiInterfaceAction::FLAG_REASON_MASK) |
                CSWGuiInterfaceAction::FLAG_REASON_NOT_ENOUGH_FORCE;
        else
            bEnabled = 1;
    } else {
        int nPercent;
        switch (nDiscountTier) {
        case 1:
            nPercent = 40;
            break;
        case 2:
            nPercent = 30;
            break;
        case 0:
            nPercent = 50;
            break;
        default:
            nPercent = 0;
            break;
        }
        int nDiscount = nPercent * nAdjustedFP / 100;
        if (static_cast<unsigned int>(nForcePoints) < static_cast<unsigned int>(nAdjustedFP - nDiscount))
            list.m_pArray[nOldCount].m_nFlags = (list.m_pArray[nOldCount].m_nFlags & ~CSWGuiInterfaceAction::FLAG_REASON_MASK) |
                CSWGuiInterfaceAction::FLAG_REASON_NOT_ENOUGH_FORCE;
        else if (nCurrentHP <= nDiscount)
            list.m_pArray[nOldCount].m_nFlags = (list.m_pArray[nOldCount].m_nFlags & ~CSWGuiInterfaceAction::FLAG_REASON_MASK) |
                CSWGuiInterfaceAction::FLAG_REASON_NOT_ENOUGH_HEALTH;
        else
            bEnabled = 1;
    }
    list.m_pArray[nOldCount].m_nFlags = (list.m_pArray[nOldCount].m_nFlags & ~CSWGuiInterfaceAction::FLAG_ENABLED) | bEnabled;
}
#endif // !KOTOR1

CSWCTrigger* CSWCTrigger::AsSWCTrigger()
{
    return this;
}

CExoString CSWCTrigger::GetName()
{
    return m_name;
}

// Registers this trigger with the area's render list and builds the Aurora PartTrigger from the
// ground polygon.
void CSWCTrigger::AddToRenderList(int a)
{
    g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_triggerIdList.Add(m_objectId);

    float afColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    m_pPartTrigger = AurPartTriggerCreate(
        g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pScene,
        m_pVertices, m_nVertices, 1, afColor, this);
    UpdateTriggerColor(a);
}

void CSWCTrigger::RemoveFromRenderList()
{
    CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;
    // Deleting the part only makes sense for a trigger that entered the render list: an undetected
    // mine has none, and SceneRemovePartFromRooms dereferenced a null part.
    if (pArea->m_triggerIdList.Contains(m_objectId))
    {
        pArea->m_triggerIdList.Remove(m_objectId);
        AurPartTriggerDelete(g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pScene,
                             m_pPartTrigger, false);
    }
}

// Recolours the trigger's highlight: white-blue for a detected mine, amber otherwise, with the
// alpha lowered while the pop-up is displayed.
void CSWCTrigger::UpdateTriggerColor(int a)
{
    K2_UNUSED(a);
    if (m_bIsTrap)
        return;

    if (m_bAreaTransition)
    {
        Vector vColor(0.0f, 0.0f, 1.0f);
        AurPartTriggerSetHighlightParams(m_pPartTrigger, vColor,
                                         m_displayState ? 0.25f : 0.0f, false, 0.0f, false);
    }
    else
    {
        PartTrigger* pPart = m_pPartTrigger;
        if (m_displayState)
        {
            Vector vColor(0.3f, 1.0f, 1.0f);
            AurPartTriggerSetHighlightParams(pPart, vColor, 0.25f, false, 0.0f, false);
        }
        else
        {
            Vector vColor(0.3f, 1.0f, 1.0f);
            AurPartTriggerSetHighlightParams(pPart, vColor, 0.0f, false, 0.0f, false);
        }
    }
    AurPartTriggerSetHighlight(m_pPartTrigger, true, m_fHighlightHeight, true);
}

// Colours the trigger for the QA/debug pass with the same highlight as UpdateTriggerColor in
// different colours, materializing the render part first if needed. Mode 0 on a hidden trigger
// drops the alpha to 0.
void CSWCTrigger::SpecialDisplayTrigger(int a)
{
    Vector vColor;
    if (m_bIsTrap)
        return;

    float fAlpha;
    if (m_bAreaTransition) {
        fAlpha = 0.25f;
        if (m_pPartTrigger == 0)
            AddToRenderList(a);
        vColor = Vector(0.0f, 0.0f, 1.0f);
    } else {
        fAlpha = 0.35f;
        if (m_pPartTrigger == 0)
            AddToRenderList(a);
        vColor = Vector(1.0f, 1.0f, 0.0f);
    }

    if (a == 0 && m_displayState == 0)
        fAlpha = 0.0f;

    AurPartTriggerSetHighlightParams(m_pPartTrigger, vColor, fAlpha, false, 0.0f, false);
    AurPartTriggerSetHighlight(m_pPartTrigger, true, m_fHighlightHeight, true);
}

// Reverses the ground polygon in place: winding order is a property of the vertex order, so flipping
// the trigger's facing means flipping the array.
void CSWCTrigger::ReverseOrder()
{
    Vector* pReversed = new Vector[m_nVertices];
    for (int i = 0; i < m_nVertices; i++)
        pReversed[i] = m_pVertices[m_nVertices - 1 - i];
    for (int i = 0; i < m_nVertices; i++)
        m_pVertices[i] = pReversed[i];
    delete[] pReversed;
}

// The cursors.2da "CursorID" column for the trigger's cursor row, into the 16-bit cached id.
void CSWCTrigger::SetCursor(unsigned char a)
{
    int nCursorId;
    if (g_pRules->m_p2DArrays->m_pCursor->GetINTEntry(a, CExoString("CursorID"), &nCursorId))
        m_nCursorId = static_cast<unsigned short>(nCursorId);
}

// Crossing-number point-in-polygon test against the trigger's ground polygon, casting the ray
// straight up from v. Degenerate vertical edges are handled by a separate test after the main body.
int CSWCTrigger::InTrigger(const Vector& v)
{
    int nCrossings = 0;
    for (int i = 0; i < m_nVertices; i++)
    {
        int j = (i == m_nVertices - 1) ? 0 : i + 1;
        const Vector& a = m_pVertices[i];
        const Vector& b = m_pVertices[j];

        if (b.x != a.x)
        {
            float fY = a.y - (b.y - a.y) / (b.x - a.x) * (a.x - v.x);
            if (fY > v.y)
            {
                if (b.y > a.y) {
                    if (fY >= a.y && fY < b.y)
                        nCrossings++;
                } else if (a.y > b.y) {
                    if (fY > b.y && fY <= a.y)
                        nCrossings++;
                } else if (b.x > a.x) {
                    if (v.x >= a.x && v.x < b.x)
                        nCrossings++;
                } else if (a.x > b.x) {
                    if (v.x > b.x && v.x <= a.x)
                        nCrossings++;
                }
            }
        }

        if (b.x == v.x)
        {
            if (b.y > v.y || a.y >= v.y)
                nCrossings++;
        }
    }
    return nCrossings % 2 == 1;
}

// 2D segment-vs-polygon-edge test: walks the ground polygon's edges in order and takes the first one
// the area reports as a hit, writing the hit point back through pOut.
int CSWCTrigger::LineSegmentIntersectTrigger(const Vector& vStart, const Vector& vEnd, Vector* pOut)
{
    CSWArea* pArea = static_cast<CSWArea*>(m_pArea);
    if (pArea == 0)
        return 0;
    float fT1, fT2;

    for (int i = 0; i < m_nVertices; i++)
    {
        int j = (i == m_nVertices - 1) ? 0 : i + 1;
        if (pArea->IntersectLineSegments(
                vStart.x, vStart.y, vEnd.x, vEnd.y,
                m_pVertices[i].x, m_pVertices[i].y,
                m_pVertices[j].x, m_pVertices[j].y, &fT1, &fT2) == 1)
        {
            pOut->x = fT1;
            pOut->y = fT2;
            pOut->z = 0.0f;
            return 1;
        }
    }
    return 0;
}

// Decides the ground polygon's winding by probing just inside each of the first two edges: step
// 0.01 units along the edge's 2D left normal from its midpoint and ask InTrigger whether that point is
// inside. Both probes must land inside.
bool CSWCTrigger::CheckWindingOrder()
{
    if (!InTrigger((m_pVertices[0] + m_pVertices[1]) * 0.5f +
                   normalize(Vector(-(m_pVertices[1].y - m_pVertices[0].y),
                                    m_pVertices[1].x - m_pVertices[0].x, 0.0f)) * 0.01f))
        return false;

    return InTrigger((m_pVertices[1] + m_pVertices[2]) * 0.5f +
                     normalize(Vector(-(m_pVertices[2].y - m_pVertices[1].y),
                                      m_pVertices[2].x - m_pVertices[1].x, 0.0f)) * 0.01f);
}

// Centres the trigger on the bounding box of its ground polygon.
void CSWCTrigger::UpdatePosition()
{
    Vector vMax = m_pVertices[0];
    Vector vMin = m_pVertices[0];
    for (int i = 1; i < m_nVertices; i++)
    {
        const Vector& v = m_pVertices[i];
        if (v.x < vMin.x) vMin.x = v.x;
        if (v.y < vMin.y) vMin.y = v.y;
        if (v.z < vMin.z) vMin.z = v.z;
        if (v.x > vMax.x) vMax.x = v.x;
        if (v.y > vMax.y) vMax.y = v.y;
        if (v.z > vMax.z) vMax.z = v.z;
    }
    Vector vSize = vMax - vMin;
    m_position.x = vMin.x + vSize.x * 0.5f;
    m_position.y = vMin.y + vSize.y * 0.5f;
    m_position.z = vMin.z + vSize.z * 0.5f;
}

// Creates this trigger's anim base lazily, loads the model into it, then hands the loaded Gob a
// back-pointer to this trigger and stamps the trigger object type on it.
int CSWCTrigger::LoadModel(const CResRef& r, unsigned char a, unsigned char b)
{
    K2_UNUSED(b);
    if (m_pModel == 0)
        m_pModel = new CSWCAnimBaseTrigger();
    if (m_pModel->LoadModel(r, a) == 0)
        return 0;
    Gob* pGob = m_pModel->GetModel(a);
    if (pGob != 0)
    {
        pGob->SetExternalData(this);
        pGob->SetExternalDataType(CGameObject::OBJECT_TYPE_TRIGGER);
    }
    return 1;
}

// Projects the polygon's bounding-box centre into camera space and onto the screen. Returns 2 when
// the centre clears the camera's depth plane, 1 when it is behind or level; the out-param only
// receives x and y.
char CSWCTrigger::GetCenter(Vector& v)
{
    char nRet = 0;
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    if (pCamera != 0)
    {
        Vector vCamPos = pCamera->GetPosition();
        Vector vDir = -pCamera->GetOrientation().zdir();

        Vector vMin(3.402823466e+38f, 3.402823466e+38f, 3.402823466e+38f);
        Vector vMax(-3.402823466e+38f, -3.402823466e+38f, -3.402823466e+38f);
        for (int i = 0; i < m_nVertices; i++) {
            vMin.x = (vMin.x < m_pVertices[i].x) ? vMin.x : m_pVertices[i].x;
            vMin.y = (vMin.y < m_pVertices[i].y) ? vMin.y : m_pVertices[i].y;
            vMin.z = (vMin.z < m_pVertices[i].z) ? vMin.z : m_pVertices[i].z;
            vMax.x = (vMax.x > m_pVertices[i].x) ? vMax.x : m_pVertices[i].x;
            vMax.y = (vMax.y > m_pVertices[i].y) ? vMax.y : m_pVertices[i].y;
            vMax.z = (vMax.z > m_pVertices[i].z) ? vMax.z : m_pVertices[i].z;
        }
        Vector vCenter = (vMin + vMax) * 0.5f;
        float fPoint = dot(vDir, vCenter);
        float fCam = dot(vDir, vCamPos);
        nRet = !(fPoint > fCam) ? 2 : 1;

        Vector vScreen = pCamera->MapToScreen(vCenter, true);
        CSWGuiManager* pGui = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        short nWidth = pGui->m_nScreenWidth;
        v.x = vScreen.x * nWidth;
        short nHeight = pGui->m_nScreenHeight;
        v.y = vScreen.y * nHeight;
    }
    return nRet;
}

// Projects the trigger's "lookathook" part when the render model carries one, and falls back to the
// polygon centre otherwise. Unlike GetCenter, the whole screen vector is written back.
char CSWCTrigger::GetReticlePoint(Vector& v)
{
    Gob* pGob = m_pModel->GetModel(ANIM_MODEL_BODY);
    if (pGob != 0) {
        Quaternion qIdentity;
        Part* pPart = pGob->GetPart(const_cast<char*>("lookathook"));
        if (pPart != 0) {
            pPart->GetWorld(v, qIdentity);

            CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
            Vector vDir = -pCamera->GetOrientation().zdir();
            float fPoint = dot(v, vDir);
            Vector vCamPos = pCamera->GetPosition();
            float fCam = dot(vDir, vCamPos);
            char nRet = !(fPoint > fCam) ? 2 : 1;

            v = g_pAppManager->m_pClientExoApp->GetModuleCamera()->MapToScreen(v, true);
            short nWidth = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
            v.x = nWidth * v.x;
            short nHeight = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight;
            v.y = nHeight * v.y;
            return nRet;
        }
    }
    return GetCenter(v);
}
