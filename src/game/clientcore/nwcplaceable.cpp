#include "CExoArrayListImpl.h"

#include "nwcplaceable.h"
#include "nwscriptconst.h"
#include "tutorial.h"
#include "nwccreature.h"
#include "nwsplaceable.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwchandlemessage.h"
#include "guiingame.h"
#include "nwcarea.h"
#include "nwsarea.h"
#include "nwccreaturestats.h"
#include "nwscreature.h"
#include "nwsitem.h"
#include "nwcobject.h"
#include "nwspell.h"
#include "nwsplayer.h"
#include "serverexoapp.h"
#include "CExoArrayListDtors.h"
#include "nwsrules.h"
#include "nwskill.h"
#include "nwcmodule.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "AURORA/aurmodel.h"
#include "nwcanimbaseplaceable.h"
#include "clientaimaster.h"
#include "gameobjectarray.h"
#include "worldtimer.h"
#include "swguiexamine.h"
#include "swgui.h"
#include "exosound.h"
#include "exoresman.h"
#include "aurcamera.h"
#include "serveranimation.h"
#include "itemtype.h"
#include "soundprioritygroup.h"
#include "strref.h"

// The placeableobjsnds.2da row of a corpse (KOTOR 1's body bags).
enum {
    PLACEABLE_SOUND_CORPSE = 53,
};

// placeables.2da's appearance row before the template sets one (KOTOR 1 keeps it in a byte).
enum {
    APPEARANCE_NONE = 0xff,
};

extern CExoSound* g_pExoSound;
#if KOTOR1  // K1 streams the model through SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

CSWSPlaceable* CSWCPlaceable::GetServerPlaceable()
{
    if (GetServerObject())
        return GetServerObject()->AsSWSPlaceable();
    return 0;
}

void CSWCPlaceable::UnloadModel(unsigned char a)
{
    if (m_pModel)
        m_pModel->UnloadModel(a);
}

void CSWCPlaceable::MenuActionUseItem(unsigned int a, CSWCCreature* c)
{
    c->PerformUseItem(a & CSWGuiInterfaceAction::ACTION_ID_ITEM_CLEAR, 0, m_objectId, m_position);
}

void CSWCPlaceable::ActionMenuGiveItem(unsigned int a, CSWCCreature* c)
{
    c->PerformGiveItem(a, m_objectId, 1);
}

void CSWCPlaceable::UnloadModel()
{
    if (m_pModel)
    {
        m_pModel->UnloadModel();
        delete m_pModel;
        m_pModel = 0;
    }
}

void CSWCPlaceable::SetMineIcon(unsigned char a)
{
    K2_UNUSED(a);
}

void CSWCPlaceable::ShowFeedbackBubbleText()
{
}

void CSWCPlaceable::ToggleLockState()
{
}

void CSWCPlaceable::HilightPlaceable(int a)
{
    K2_UNUSED(a);
}

void CSWCPlaceable::AddFireAndForgetAnimation(unsigned short a, void* p, unsigned short b)
{
    K2_UNUSED(b);
    float speed = m_animationSpeed;
    float len = GetAnimationLength(a);
    InsertFireForgetAnimation(a, static_cast<unsigned int>(len / speed), speed, p, 0);
}

// Shows the placeable's localized description. Unlike the door's version this leaves the acting
// creature's action queue alone.
void CSWCPlaceable::ActionMenuExamine(unsigned int a, CSWCCreature* c)
{
    K2_UNUSED(a); K2_UNUSED(c);
    if (GetServerObject()) {
        CSWSPlaceable* pServerPlaceable = GetServerObject()->AsSWSPlaceable();
        if (pServerPlaceable) {
            CExoString text;
            pServerPlaceable->m_description.GetString(
                g_pAppManager->m_pClientExoApp->GetClientLanguage(), &text, 0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowExamineBox(text, 0);
        }
    }
}

// Asks the server to unlock the placeable.
void CSWCPlaceable::ActionMenuSecurity(unsigned int a, CSWCCreature* c)
{
    K2_UNUSED(a); K2_UNUSED(c);
    CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMessage)
        pMessage->SendPlayerToServerInput_UnlockObject(m_objectId, OBJECT_INVALID);
}

// Shows the bash tutorial window the first time, otherwise orders the creature to attack.
void CSWCPlaceable::BashPlaceable(unsigned int a, CSWCCreature* c)
{
    K2_UNUSED(a);
    if (c && g_pAppManager->m_pClientExoApp->ShowTutorialWindow(
                 TUTORIAL_BASH, c->m_objectId, m_objectId, 0) == 0)
        c->PerformAttack(m_objectId);
}

// Asks the server to use the placeable, provided the in-game GUI is accepting clicks.
void CSWCPlaceable::UsePlaceable(unsigned int a, CSWCCreature* c)
{
    K2_UNUSED(a); K2_UNUSED(c);
    CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->GetCanClick())
        pMessage->SendPlayerToServerInput_UseObject(m_objectId);
}

// Applies a computer spike's bonus to the server placeable, then asks it to be used.
void CSWCPlaceable::MenuActionUseComputerSpike(unsigned int a, CSWCCreature* c)
{
    K2_UNUSED(c);
    if (GetServerObject()) {
        CSWSPlaceable* pServerPlaceable = GetServerObject()->AsSWSPlaceable();
        if (pServerPlaceable)
            pServerPlaceable->ApplyComputerSpikeBonus(a);
    }
    CSWCMessage* pMessage = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->GetCanClick())
        pMessage->SendPlayerToServerInput_UseObject(m_objectId);
}

// Builds the placeable's radial-menu entries: 0 bash (with a party-interact fixup), 1 security
// followed by the use-item entries, 2 mines.
void CSWCPlaceable::GetTargetActions(CSWCCreature* pCreature, int nType,
                                     CExoArrayList<CSWGuiInterfaceAction>& list)
{
    CSWSPlaceable* pServerPlaceable =
        GetServerObject() ? GetServerObject()->AsSWSPlaceable() : static_cast<CSWSPlaceable*>(0);
    int nAreaMenuDisabled = m_pArea->GetServerArea()->m_nPlayerRestrictMode;
    int nOldCount = list.m_count;
    CExoString sName;

    if (nType != 2) {
        if (nType != 1) {
            if (nType != 0)
                return;
            if (pServerPlaceable == 0 || pServerPlaceable->m_bHasInventory == 0) {
                if (m_bPlot == 0 && nAreaMenuDisabled == 0 && m_bLocked != 0) {
                    list.SetSize(nOldCount + 1);
                    list.m_pArray[nOldCount].m_sName =
                        g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BASH);
                    list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_BASH;
                    list.m_pArray[nOldCount].m_pfnAction =
                        reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCPlaceable::BashPlaceable);
                    list.m_pArray[nOldCount].m_nObjectId = m_objectId;
                    list.m_pArray[nOldCount].m_resRef = const_cast<char*>("i_attack");
                }
                return;
            }
            if (m_bPlot == 0 && nAreaMenuDisabled == 0 && m_bLocked != 0) {
                list.SetSize(nOldCount + 1);
                list.m_pArray[nOldCount].m_sName =
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BASH);
                list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_BASH;
                list.m_pArray[nOldCount].m_pfnAction =
                    reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCPlaceable::BashPlaceable);
                list.m_pArray[nOldCount].m_nObjectId = m_objectId;
                list.m_pArray[nOldCount].m_resRef = const_cast<char*>("i_attack");

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
                     pCreature->m_objectId != nClientId) &&
                    pPlayerCreature->m_bInParty != 0 &&
                    pPlayerCreature->GetServerCreature() != 0 &&
#if KOTOR1  // K1 tests the creature for being dead
                    (pPlayerCreature->GetServerCreature()->GetDeadTemp() != 0 &&
#else
                    (pServerCreature = pPlayerCreature->GetServerCreature(),
                     pServerCreature->GetDeadTemp() != 0 &&
#endif
                         GetServerObject() != 0 &&
                         GetServerObject()->AsSWSPlaceable() != 0 &&
                         (GetServerObject(),
                          GetServerObject()->AsSWSPlaceable()->m_bPartyInteract == 0))) {
                    unsigned char& cFlags = list.m_pArray[nOldCount].m_nFlags;
                    cFlags &= ~CSWGuiInterfaceAction::FLAG_ENABLED;
                    list.m_pArray[nOldCount].m_nFlags =
                        (list.m_pArray[nOldCount].m_nFlags & ~CSWGuiInterfaceAction::FLAG_REASON_MASK) |
                        CSWGuiInterfaceAction::FLAG_REASON_PARTY_MEMBER_DEAD;
                }
            }
            return;
        }
        if (pServerPlaceable != 0 && pServerPlaceable->m_bHasInventory != 0) {
            if (m_bLocked != 0 && pCreature->m_pStats->GetCanUseSkill(SKILL_SECURITY) != 0) {
                list.SetSize(nOldCount + 1);
                list.m_pArray[nOldCount].m_sName =
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SECURITY);
                list.m_pArray[nOldCount].m_pfnAction =
                    reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCPlaceable::ActionMenuSecurity);
                list.m_pArray[nOldCount].m_nObjectId = m_objectId;
                list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_SECURITY;
                list.m_pArray[nOldCount].m_resRef = g_pRules->m_paSkills[6].m_icon;
            }
#if !KOTOR1  // K1 offers no use-item entries on a placeable
            if (pServerPlaceable->m_bHasInventory != 0 && m_bLocked != 0 &&
                pCreature->m_pStats->GetCanUseSkill(SKILL_SECURITY) != 0) {
                GetUseItems(pCreature, list);
            }
#endif
        }
        return;
    }
#if !KOTOR1  // K1 has no placeable mine entries
    if (m_bLocked != 0)
        GetPlaceableInterfaceMines(pCreature, list);
#endif
}

// One menu entry per usable mine in the creature's inventory.
#if !KOTOR1
void CSWCPlaceable::GetPlaceableInterfaceMines(CSWCCreature* pCreature,
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
    sHeader.Format(const_cast<char*>(" (%s)"), pExoApp->GetGUIString(STRREF_LOCK).CStr());

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
            reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCPlaceable::MenuActionSetPlaceableMine);
        list.m_pArray[nOldCount].m_nObjectId = m_objectId;
        list.m_pArray[nOldCount].m_nActionId =
            static_cast<unsigned int>(pItem->m_objectId) | CSWGuiInterfaceAction::ACTION_ID_ITEM;
        list.m_pArray[nOldCount].m_resRef = pItem->GetIcon();
        nOldCount++;
    }
}
#endif

// Adds the creature's computer-spike entries, restamped with this placeable's callback, followed
// by its usable consumables. The consumable scan writes from index 0, so it overwrites the spike
// entries.
void CSWCPlaceable::GetUseItems(CSWCCreature* pCreature,
                                CExoArrayList<CSWGuiInterfaceAction>& list)
{
    CSWSCreature* pServerCreature = pCreature->GetServerCreature();
    if (GetServerObject())
        GetServerObject()->AsSWSPlaceable();
    if (m_bLocked != 0) {
        int nOldCount = list.m_count;
        if (pServerCreature != 0 && pCreature->m_pStats->GetCanUseSkill(SKILL_SECURITY) != 0)
            pServerCreature->GetSpikeDetails(list, 1);
        for (int i = nOldCount; i < list.m_count; i++) {
            list.m_pArray[i].m_pfnAction =
                static_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCObject::MenuActionUseSecuritySpike);
            list.m_pArray[i].m_nObjectId = m_objectId;
        }
    }

    CItemRepository* pRepo = pServerCreature->GetItemRepository(1);
    CExoString sName;
    int nLanguage = g_pAppManager->m_pClientExoApp->GetClientLanguage();
    int nOldCount = 0;
    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
        CSWSItem* pItem;
        CSWItemProperty* pProperty;
        CSWSpell* pSpell;
        pItem = pRepo->ItemListGetItem(i);
        if (pItem != 0 && pItem->m_bPlot != 0) {
            if (pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_ACTIVATE_ITEM, 0) != 0) {
                pSpell = g_pRules->GetSpell(pProperty->m_subType);
                if (pSpell != 0 && pSpell->m_nItemTargeting == 2) {
                    list.SetSize(nOldCount + 1);
                    pItem->m_localizedName.GetString(nLanguage, &sName, 0);
                    list.m_pArray[nOldCount].m_sName = sName;
                    list.m_pArray[nOldCount].m_pfnAction =
                        reinterpret_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCPlaceable::MenuActionUseItem);
                    list.m_pArray[nOldCount].m_nObjectId = m_objectId;
                    list.m_pArray[nOldCount].m_nActionId =
                        static_cast<unsigned int>(pItem->m_objectId) | CSWGuiInterfaceAction::ACTION_ID_ITEM;
                    list.m_pArray[nOldCount].m_resRef = pItem->GetIcon();
                    list.m_pArray[nOldCount].m_nCount =
                        pItem->m_stackCount > pItem->m_nCharges ? pItem->m_stackCount
                                                                : pItem->m_nCharges;
                    nOldCount++;
                }
            }
        }
    }
}

CSWCPlaceable* CSWCPlaceable::AsSWCPlaceable()
{
    return this;
}

CExoString CSWCPlaceable::GetName()
{
    return m_name;
}

int CSWCPlaceable::IsHostile()
{
    if (GetServerPlaceable())
        return GetServerPlaceable()->m_hostileState;
    return 0;
}

// Detaches the light from the scene and deletes it.
void CSWCPlaceable::UnloadLight()
{
    CSWCArea* pArea = 0;
    if (g_pAppManager->m_pClientExoApp->GetModule())
        pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;
    if (pArea)
        pArea->RemovePlaceableObjectLight(m_objectId, 0);
    if (m_pLight)
    {
        m_pLight->AttachToObject(0, 0, 0);
        m_pLight->AttachToScene(0);
        delete m_pLight;
        m_pLight = 0;
    }
}

// A corpse placeable never animates. The two "light off" animations release the attached light
// before the base runs.
int CSWCPlaceable::SetAnimation(unsigned short a, int b, int c)
{
    CSWSPlaceable* pPlaceable = GetServerPlaceable();
    if (pPlaceable && pPlaceable->m_corpse.m_bIsCorpse)
        return 1;
    if (a == CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_DIE || a == SERVER_ANIMATION_DESTROYED)
        UnloadLight();
    return CSWCObject::SetAnimation(a, b, c);
}

// Height at which creatures sight this placeable, from placeables.2da; 1.5m by default.
float CSWCPlaceable::GetCanSeeHeight()
{
    C2DA* p2DA = g_pRules->m_p2DArrays->m_pPlaceables;
    float fHeight = 1.5f;
    CSWSPlaceable* pPlaceable = GetServerPlaceable();
    if (pPlaceable)
    {
        if (p2DA->GetFLOATEntry(pPlaceable->m_appearanceType, "CanSeeHeight", &fHeight) == 0)
            fHeight = 1.5f;
    }
    return fHeight;
}

#if KOTOR1
void CSWCPlaceable::SetAppearance(unsigned char nAppearanceId)
#else
void CSWCPlaceable::SetAppearance(int nAppearanceId)
#endif
{
    m_nAppearanceType = nAppearanceId;
    int nHitCheck = 0;
    g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(nAppearanceId, "HitCheck", &nHitCheck);
    Gob* pGob = GetModel(ANIM_MODEL_BODY, 1);
    if (pGob)
        pGob->SetIgnoreHitCheck(nHitCheck == 0);
    if (m_pModel)
    {
        int nNonCull;
        if (g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(nAppearanceId, "NonCull", &nNonCull) == 0)
            nNonCull = 0;
    }
}

int CSWCPlaceable::ApplyShadowBlob()
{
    CExoString sBlob;
    if (m_pModel == 0)
        return 0;
    int nShadowSize;
    if (g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(m_nAppearanceType, "ShadowSize", &nShadowSize) == 0)
        return 1;
    int nResult;
    switch (nShadowSize)
    {
    case 0:  sBlob = "Fx_Shadow_S";  break;
    case 1:  sBlob = "Fx_Shadow_M";  break;
    case 2:  sBlob = "Fx_Shadow_L";  break;
    default: sBlob = "Fx_Shadow_M";  break;
    }
    if (m_bVisible)
        m_pModel->TurnOffShadows();
    nResult = m_pModel->ApplyShadowBlob(sBlob);
    if (m_bVisible == 0)
        HideShadowBlob();
    return nResult;
}

void CSWCPlaceable::AIUpdate()
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

// Sets a mine at the placeable where the area allows player-laid mines, and refuses with an
// uncancellable message box where it does not.
void CSWCPlaceable::MenuActionSetPlaceableMine(unsigned int itemId, CSWCCreature* pCreature)
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pPlayer = pServerApp->GetPlayerCreature();
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

// Loads the model, hands its Gob a back-pointer to this object and caches the height of the
// "<name>_head_hit" part, which ranged attacks aim at.
int CSWCPlaceable::LoadModel(const CResRef& r, unsigned char a, unsigned char b)
{
    K2_UNUSED(b);
    if (m_pModel == 0)
        m_pModel = new CSWCAnimBasePlaceable();
    if (m_pModel->LoadModel(r, a) == 0)
        return 0;
    Gob* pGob = m_pModel->GetModel(a);
    if (pGob != 0) {
        pGob->SetExternalData(this);
        pGob->SetExternalDataType(CGameObject::OBJECT_TYPE_PLACEABLE);
    }
    r.CopyToString(m_sModelName);
    m_sModelName = m_sModelName.SubString(4, -1);
    Vector vPartPosition(0.0f, 0.0f, 0.0f);
    Quaternion qPartOrientation;
    CExoString sPartName;
    sPartName = m_sModelName + "_head_hit";
    if (pGob->GetPartLocalPosition(static_cast<char*>(sPartName.CStr()), &vPartPosition, &qPartOrientation) != 0)
        m_fHeadHeight = vPartPosition.z;
    else
        m_fHeadHeight = 1.5f;
    return 1;
}

CSWCPlaceable::CSWCPlaceable()
    : CSWCObject(CGameObject::OBJECT_TYPE_PLACEABLE)
{
    m_currentAnimation = SERVER_ANIMATION_PLACEABLE_CLOSE;
    unsigned int nDay, nTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;
    m_bReserved1 = 1;
    m_bTrapDisarmable = 1;
    m_bHilightShown = 0;
    m_bHilighted = 0;
    m_bHasDescription = 0;
    m_bHostileFaction = 0;
    m_bIgnoreHitcheck = 0;
#if KOTOR1
    m_nAppearanceType = APPEARANCE_NONE;
#else
    m_nAppearanceType = -1;
#endif
    m_bPlot = 0;
    m_bLockable = 0;
    m_bLocked = 0;
    m_nReserved1 = -1;
    m_pReserved1 = 0;
    m_name = "";
    m_sConversation = "";
    m_sModelName = "";
    m_pLight = 0;
    m_pReserved2 = 0;
}

CSWCPlaceable::~CSWCPlaceable()
{
    UnloadLight();
    UnloadModel();
}

// Advances the animation state one frame: retires an expired fire-and-forget animation and starts
// the next queued one. An out-of-place animation plays with the model detached from the object's
// transform, and the open, close and destroy animations also play their placeableobjsnds row.
void CSWCPlaceable::AnimationUpdate()
{
    if (AnimationFireAndForget(m_currentAnimation) == 1) {
        m_nFireForgetTimeLeftMs -= m_nAIUpdateDeltaMs;
        if (m_nFireForgetTimeLeftMs < 0)
            SetAnimation(GetLoopingAnimation(), 0, 0);
    }
    if (AnimationLooping(m_currentAnimation) == 1) {
        unsigned short nAnim;
        float fSpeed;
        unsigned int nDuration;
        void* pData;
        if (GetTopFireForgetAnimation(&nAnim, &fSpeed, &nDuration, &pData) == 1) {
            m_currentAnimation = nAnim;
            if (m_bModelSyncDisabled != 0) {
                m_bModelSyncDisabled = 0;
                Vector vPosition = m_position;
                Vector vOrientation = m_orientation;
                m_pModel->SetPosition(vPosition);
                m_pModel->SetOrientation(vOrientation);
                m_pModel->SetAlwaysRender(0);
                m_pModel->SetInCutscene(0);
                AbortFireForgetAnimation(0);
            }
            if (AnimationPlayOutOfPlace(nAnim) != 0) {
                m_bModelSyncDisabled = 1;
                Vector vOrigin(0.0f, 0.0f, 0.0f);
                Quaternion qIdentity;
                m_pModel->SetPosition(vOrigin);
                m_pModel->SetOrientation(qIdentity);
                m_pModel->SetAlwaysRender(1);
                m_pModel->SetInCutscene(1);
                m_bPlayingCutsceneAnimation = 1;
            }
            m_pModel->SetAnimation(nAnim, fSpeed, 1, 0.0f);
            m_nFireForgetTimeLeftMs = nDuration;
            RemoveTopFireForgetAnimation();
            switch (nAnim) {
            case CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_DIE:
            case CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_CLOSE_TO_OPEN:
            case CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_OPEN_TO_CLOSE:
            case CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_ON_TO_OFF:
            case CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_OFF_TO_ON: {
                CExoString sSound;
                int nSoundRow;
                int nAppearance = m_nAppearanceType;
#if KOTOR1
                // K1: a hidden body bag whose parent corpse is still in the area always sounds.
                int bFound = g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(
                    nAppearance, "SoundAppType", &nSoundRow);
                CSWSPlaceable* pServerPlaceable = GetServerPlaceable();
                if (pServerPlaceable != 0 && pServerPlaceable->m_bIsBodyBag != 0 && m_pArea != 0 &&
                    pServerPlaceable->m_bIsBodyBagVisible == 0 &&
                    m_pArea->ParentCorpseExists(m_objectId) != 0) {
                    nSoundRow = PLACEABLE_SOUND_CORPSE;
                    bFound = 1;
                }
                if (bFound != 0) {
#else
                if (g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(
                        nAppearance, "SoundAppType", &nSoundRow) != 0) {
#endif
                    // No sound column applies: fall out of the switch rather than report failure.
                    int nResult;
                    if (nAnim == CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_ON_TO_OFF || nAnim == CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_OFF_TO_ON)
                        nResult = g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                            nSoundRow, "Used", &sSound);
                    else if (nAnim == CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_DIE)
                        nResult = g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                            nSoundRow, "Destroyed", &sSound);
                    else if (nAnim == CSWCAnimBase::CLIENT_ANIMATION_PLACEABLE_OPEN_TO_CLOSE)
                        nResult = g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                            nSoundRow, "Closed", &sSound);
                    else
                        break;
                    if (nResult != 0)
                        g_pExoSound->Play3DOneShotSound(CResRef(sSound), m_position, 0.0f, PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL, 0,
                                                        0, 0.0f, 0.0f);
                }
                break;
            }
            }
        }
    }
}

// Attaches the appearance-driven point light, either the one the area already holds for this
// placeable or a new "fx_placeable01", at the model's "root" part plus the light offset.
int CSWCPlaceable::LoadLight()
{
    Vector vColor(0.0f, 0.0f, 0.0f);
    CExoString sLightNode;
    CExoString sUnused;
    int nLightColorRow;
    if (g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(m_nAppearanceType, "LightColor",
                                                          &nLightColorRow) == 0)
        return 1;
    CSWCArea* pArea = 0;
    Gob* pGob;
    if (g_pAppManager->m_pClientExoApp->GetModule()) {
        pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;
        if (pArea)
            m_pLight = static_cast<CAurObject*>(pArea->GetPlaceableObjectLight(m_objectId));
    }
    if (g_pRules->m_p2DArrays->m_pPlaceables->GetCExoStringEntry(m_nAppearanceType, "LightNode",
                                                                 &sLightNode) == 0)
        sLightNode = "impact";
    pGob = GetModel(ANIM_MODEL_BODY, 1);
    if (pGob == 0)
        return 0;
    if (m_pLight == 0) {
        Vector vNodePosition(0.0f, 0.0f, 0.0f);
        Quaternion qNodeOrientation;
        Vector vOffset(0.0f, 0.0f, 0.0f);
        m_pLight = NewCAurObject(const_cast<char*>("fx_placeable01"), const_cast<char*>("placeablelight"), 0, 0);
        if (m_pLight == 0)
            return 0;
        g_pRules->m_p2DArrays->m_pLightColor->GetFLOATEntry(nLightColorRow, "RED", &vColor.x);
        g_pRules->m_p2DArrays->m_pLightColor->GetFLOATEntry(nLightColorRow, "GREEN", &vColor.y);
        g_pRules->m_p2DArrays->m_pLightColor->GetFLOATEntry(nLightColorRow, "BLUE", &vColor.z);
        if (g_pRules->m_p2DArrays->m_pPlaceables->GetFLOATEntry(m_nAppearanceType, "LightOffsetX",
                                                                &vOffset.x) == 0)
            vOffset.x = 0.0f;
        if (g_pRules->m_p2DArrays->m_pPlaceables->GetFLOATEntry(m_nAppearanceType, "LightOffsetY",
                                                                &vOffset.y) == 0)
            vOffset.y = 0.0f;
        if (g_pRules->m_p2DArrays->m_pPlaceables->GetFLOATEntry(m_nAppearanceType, "LightOffsetZ",
                                                                &vOffset.z) == 0)
            vOffset.z = 3.0f;
        m_pLight->GetPartLocalPosition(const_cast<char*>("root"), &vNodePosition, &qNodeOrientation);
        m_pLight->SetPartLocalPosition(const_cast<char*>("root"), vNodePosition + vOffset, qNodeOrientation);
    }
    m_pLight->AttachToObject(static_cast<CAurObject*>(pGob), const_cast<char*>("root"), 0);
    if (pArea)
        pArea->RemovePlaceableObjectLight(m_objectId, 0);
    return 1;
}

// A corpse placeable's reticle sits on its model's "rootdummy" part rather than on the object
// origin. Returns 1 when the point is in front of the camera plane, 2 when it is behind, and hands
// the point back in normalized screen coordinates scaled to the backbuffer.
char CSWCPlaceable::GetReticlePoint(Vector& v)
{
    CSWSPlaceable* pPlaceable = GetServerPlaceable();
    if (pPlaceable == 0 || pPlaceable->m_corpse.m_bIsCorpse == 0)
        return CSWCObject::GetReticlePoint(v);
    Gob* pGob = m_pModel->GetModel(ANIM_MODEL_BODY);
    if (pGob == 0)
        return CSWCObject::GetReticlePoint(v);
    Part* pPart = pGob->GetPart(const_cast<char*>("rootdummy"));
    if (pPart == 0)
        return CSWCObject::GetReticlePoint(v);
    Quaternion qPart;
    pPart->GetWorld(v, qPart);
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    // The camera's view axis.
    Vector vDir = -pCamera->GetOrientation().zdir();
    float fPointSide = dot(v, vDir);
    Vector vCamera = pCamera->GetPosition();
    float fCameraSide = dot(vDir, vCamera);
    char nSide = (fPointSide > fCameraSide) ? 1 : 2;
    v = pCamera->MapToScreen(v, true);
    v.x *= static_cast<float>(g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth);
    v.y *= static_cast<float>(g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight);
    return nSide;
}
