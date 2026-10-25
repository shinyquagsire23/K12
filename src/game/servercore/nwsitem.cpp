// CSWSItem, the server-side item object. CItemRepository's two list-search methods are
// defined here as well.

#include "k2stdint.h"
#include "nwsitem.h"
#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE
#include "CExoArrayListImpl.h"
#include "nwsobject.h"
#include "nwsarea.h"
#include "nwscreature.h"
#include "nwsplaceable.h"
#include "nwsstore.h"
#include "nwmessage.h"
#include "gameobjectarray.h"
#include "nwsplayer.h"
#include "partytables.h"
#include "nwitem.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "serveraimaster.h"
#include "nwscript.h"
#include "guiingame.h"
#include "nwcitem.h"
#include "appmanager.h"
#include "nwtlktable.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "serverexoapp.h"
#include "nwssendplayermessage.h"
#include "resgff.h"
#include "CExoArrayListDtors.h"
#include "worldtimer.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "nwscriptconst.h"
#include "serveranimation.h"
#include "nwseffect.h"
#include "nwsscriptevent.h"
#include "restype.h"
#include "strref.h"

// KOTOR 2's baseitems.2da rows past nwscript.nss's BASE_ITEM_* list.
enum {
    BASE_ITEM_SONIC_GRENADE_TYPE2 = 97,
    BASE_ITEM_ARMORED_FLIGHT_SUIT = 98,
    BASE_ITEM_RESTRICTIVE_JEDI_ROBE = 99,
    BASE_ITEM_ZEISON_SHA_ARMOR = 102,
};

// GetPropertyStrings's arguments for a column the line does not show: no subtype, cost-table
// value or param1 value string is fetched for them.
enum {
    PROPERTY_SUBTYPE_NONE = 0xffff,
    PROPERTY_VALUE_NONE   = 0xffff,
    PROPERTY_PARAM_NONE   = 0xff,
};

// baseitems.2da's max-dexterity bonus of an item that sets none.
enum {
    MAX_DEX_BONUS_NONE = 0xff,
};

void CSWSItem::SetIdentified(int flag)
{
    m_flags = (m_flags & ~FLAG_IDENTIFIED) | (flag & FLAG_IDENTIFIED);
}

// Base item rows 8, 9 and 10 are the lightsabers (normal, double and short).
int CSWSItem::IsLightsaber()
{
    return static_cast<unsigned int>(m_baseItem - BASE_ITEM_LIGHTSABER) < 3;
}

CSWItemProperty* CSWSItem::GetActiveProperty(int nIndex)
{
    if (nIndex < 0 || m_nActiveProperties <= nIndex)
        return 0;
    return &m_pActiveProperties[nIndex];
}

CSWItemProperty* CSWSItem::GetPassiveProperty(int nIndex)
{
    if (nIndex < 0 || m_nPassiveProperties <= nIndex)
        return 0;
    return &m_pPassiveProperties[nIndex];
}

void CSWSItem::RemoveFromArea()
{
    if (GetArea() != 0)
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
    SetArea(0);
    Vector zero = {};
    SetPosition(zero, 1, 1, 0);
}

CSWSItem::CSWSItem(unsigned long long objectId)
    : CSWSObject(CGameObject::OBJECT_TYPE_ITEM, objectId, 0)
{
    m_pItemRepository = 0;
    m_flags = m_flags & ~FLAG_INVENTORY_OPEN;

    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddObject(this, 0);

    m_nAnimation = SERVER_ANIMATION_READY;
    memset(&m_nUsedActivePropertyMask, 0,
           sizeof(m_nUsedActivePropertyMask) + sizeof(m_nUsedActivePropertyUsesLeft));
    m_nBaseArmorClass = 0;
    m_nActiveProperties = 0;
    m_nPassiveProperties = 0;
    m_pActiveProperties = 0;
    m_pPassiveProperties = 0;
    m_nCharges = 50;
    m_nAddCost = 0;
    m_nModelVariation = 1;
    m_nBodyVariation = 0;
    m_nTextureVar = 0;
    m_stackCount = 1;
    m_nUpgrades = 0;
    m_possessorId = CGameObjectArray::INVALIDOBJID;
#if KOTOR1  // K1: no upgrade level or upgrade vars
    m_flags = (m_flags & ~(FLAG_IDENTIFIED | FLAG_INFINITE | FLAG_DROPABLE | FLAG_PICKPOCKETABLE
                        | FLAG_STOLEN | FLAG_NON_EQUIPPABLE | FLAG_NEW_ITEM | FLAG_DELETING))
              | FLAG_DROPABLE | FLAG_PICKPOCKETABLE;
#else
    m_bDoorCutting = 0;
    m_bDoorSabering = 0;
    m_bRapidShot = 0;
    m_bSniper = 0;
    m_flags = (m_flags & ~(FLAG_IDENTIFIED | FLAG_INFINITE | FLAG_DROPABLE | FLAG_PICKPOCKETABLE
                        | FLAG_STOLEN | FLAG_NON_EQUIPPABLE | FLAG_NEW_ITEM | FLAG_DELETING))
              | FLAG_DROPABLE | FLAG_PICKPOCKETABLE;
    memset(m_anUpgVars, 0, sizeof(m_anUpgVars));
    m_nMaxDexBonus = 0;
    m_nUpgradeLevel = 0;
#endif
}
CSWSItem::~CSWSItem()
{
    // Release the item's client-side twin through its own deleting destructor.
    unsigned long long nServerId =
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
    CSWCItem* pObject = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(nServerId);
    if (pObject != 0)
    {
        CGameObject* pRemoved;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(nServerId, &pRemoved)
                == CGameObjectArray::SUCCESS
            && pRemoved != 0)
            delete pObject;
    }

    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0)
        pModule->RemoveObjectFromLookupTable(m_sTag, m_objectId);

    if (m_pItemRepository != 0)
    {
        delete m_pItemRepository;
        m_pItemRepository = 0;
    }
    if (m_pActiveProperties != 0)
    {
        delete[] m_pActiveProperties;
        m_pActiveProperties = 0;
    }
    if (m_pPassiveProperties != 0)
    {
        delete[] m_pPassiveProperties;
        m_pPassiveProperties = 0;
    }

    if (GetArea() != 0)
        RemoveFromArea();

    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->RemoveObject(this);
}

void CSWSItem::SetPossessor(unsigned long long newOwnerId, int a, int b, unsigned long long prevOwnerId)
{
    unsigned long long moduleId = g_pAppManager->m_pServerExoApp->GetModule()->m_objectId;

    // Already held by the requested owner: still signal the acquire, then leave.
    if (m_possessorId == newOwnerId)
    {
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_ACQUIRE_ITEM;
        pEvent->SetObjectID(0, m_objectId);
        pEvent->SetObjectID(1, newOwnerId);
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, m_objectId, moduleId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        return;
    }

    // An invalid previous owner means the previous owner is unknown, so the item's own id is
    // used and nothing counts as changed. Otherwise the passed-in value is authoritative and is
    // stored for the resolution below.
    bool bChanged;
    if (prevOwnerId == CGameObjectArray::INVALIDOBJID)
    {
        bChanged = false;
        prevOwnerId = m_possessorId;
    }
    else
    {
        bChanged = m_possessorId != prevOwnerId;
        m_possessorId = prevOwnerId;
    }

    unsigned long long oidCreatureA = CGameObjectArray::INVALIDOBJID;
    unsigned long long oidPlaceableA = CGameObjectArray::INVALIDOBJID;

    if (prevOwnerId != CGameObjectArray::INVALIDOBJID)
    {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_possessorId,
                                                                           &pObject)
                == CGameObjectArray::SUCCESS
            && pObject != 0)
        {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
            {
                oidCreatureA = prevOwnerId;
            }
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
            {
                CSWSItem* pItem = pObject->AsSWSItem();
                if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                        pItem->m_possessorId, &pObject)
                        == CGameObjectArray::SUCCESS
                    && pObject != 0
                    && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
                    oidCreatureA = pItem->m_possessorId;
            }
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            {
                oidPlaceableA = prevOwnerId;
            }
        }
    }

    unsigned long long oidCreatureB = CGameObjectArray::INVALIDOBJID;
    unsigned long long oidPlaceableB = CGameObjectArray::INVALIDOBJID;

    if (newOwnerId != CGameObjectArray::INVALIDOBJID)
    {
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(newOwnerId, &pObject)
                == CGameObjectArray::SUCCESS
            && pObject != 0)
        {
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
            {
                oidCreatureB = newOwnerId;
            }
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
            {
                CSWSItem* pItem = pObject->AsSWSItem();
                if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                        pItem->m_possessorId, &pObject)
                        == CGameObjectArray::SUCCESS
                    && pObject != 0
                    && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
                    oidCreatureB = pItem->m_possessorId;
            }
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
            {
                oidPlaceableB = newOwnerId;
            }
        }
    }

    // The local player only hears about the handover when the two resolutions disagree.
    if (!bChanged && oidCreatureA != oidCreatureB)
    {
        if (oidCreatureA != CGameObjectArray::INVALIDOBJID)
        {
            if (a == 1)
            {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_LOSE_ITEM;
                pEvent->SetObjectID(0, m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, moduleId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreatureA);
            if (pCreature != 0 && b == 1)
            {
                CSWCCMessageData* pMsg = new CSWCCMessageData();
                pMsg->SetObjectID(0, m_objectId);
                pMsg->SetInteger(0, 1);
                pCreature->SendFeedbackMessage(FEEDBACK_ITEM_LOST, pMsg);
            }
        }
        if (oidCreatureB != CGameObjectArray::INVALIDOBJID)
        {
            // A player character taking the item drops both of its flags at once.
            CGameObject* pOwner;
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreatureB,
                                                                                &pOwner)
                    == CGameObjectArray::SUCCESS
                && pOwner != 0)
            {
                if (pOwner->AsSWSCreature()->m_pStats->m_bIsPC)
                    m_flags |= FLAG_DROPABLE | FLAG_PICKPOCKETABLE;
            }
            if (b == 1)
            {
                CSWSCreature* pCreature =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreatureB);
                if (pCreature != 0)
                {
                    CSWCCMessageData* pMsg = new CSWCCMessageData();
                    pMsg->SetObjectID(0, m_objectId);
                    pMsg->SetInteger(0, 1);
                    pCreature->SendFeedbackMessage(FEEDBACK_ITEM_ACQUIRED, pMsg);
                }
            }
        }
    }

    if (a == 1 && oidCreatureB != CGameObjectArray::INVALIDOBJID)
    {
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_ACQUIRE_ITEM;
        pEvent->SetObjectID(0, m_objectId);
        pEvent->SetObjectID(1, oidCreatureB);
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, m_objectId, moduleId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    }

    // A live container that just lost the item rings both of its doors: the id that left and the
    // id that took over.
    if (oidPlaceableA != CGameObjectArray::INVALIDOBJID)
    {
        CSWSPlaceable* pPlaceable =
            g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidPlaceableA);
        if (pPlaceable != 0 && pPlaceable->m_bDieWhenEmpty == 0)
        {
            unsigned long long oidNotify = (oidCreatureB != CGameObjectArray::INVALIDOBJID)
                                               ? oidCreatureB
                                               : oidPlaceableB;
            if (a == 1 && oidNotify != CGameObjectArray::INVALIDOBJID)
            {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_INVENTORY_DISTURBED;
                pEvent->SetInteger(0, 1);
                pEvent->SetObjectID(0, m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, oidNotify, oidPlaceableA, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    }

    if (oidPlaceableB != CGameObjectArray::INVALIDOBJID)
    {
        CSWSPlaceable* pPlaceable =
            g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidPlaceableB);
        if (pPlaceable != 0 && pPlaceable->m_bDieWhenEmpty == 0)
        {
            unsigned long long oidNotify = (oidCreatureA != CGameObjectArray::INVALIDOBJID)
                                               ? oidCreatureA
                                               : oidPlaceableA;
            if (a == 1 && oidNotify != CGameObjectArray::INVALIDOBJID)
            {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_INVENTORY_DISTURBED;
                pEvent->SetInteger(0, 0);
                pEvent->SetObjectID(0, m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, oidNotify, oidPlaceableB, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    }

    m_possessorId = newOwnerId;
}

static inline int IsDestroyable(CSWSObject* pObject) { return pObject->m_bDestroyable; }

void CSWSItem::EventHandler(unsigned int nEventType, unsigned long long nCallerId, void* pData,
                            unsigned int a, unsigned int b)
{
    K2_UNUSED(nCallerId); K2_UNUSED(a); K2_UNUSED(b);
    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_TIMED_EVENT))
    {
        if (pData != 0)
            g_pVirtualMachine->RunScriptSituation(pData, m_objectId, 1);
        return;
    }

    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_DECREMENT_STACKSIZE))
    {
        // The possessor is only needed by the tail, but it is resolved before the stack test.
        CSWSCreature* pPossessor =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_possessorId);
        if (m_stackCount > 1) {
            m_stackCount = m_stackCount - 1;
        } else {
            // Last one off the stack: the item itself has to go, so it schedules its own destroy.
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, m_objectId, CServerAIMaster::CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
        }
        // Either way the carrier just got one unit lighter.
        if (pPossessor != 0 && pPossessor->m_pInventory->GetItemInInventory(this) != 0) {
            pPossessor->m_nEquippedWeight -=
                g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nTenthLbs;
        }
        return;
    }

    if (nEventType != static_cast<unsigned int>(CServerAIMaster::EVENT_DESTROY_OBJECT)) return;
    if (IsDestroyable(this) == 0) return;

    CloseItemForAllPlayers();
    CGameObject* pOwner = g_pAppManager->m_pServerExoApp->GetGameObject(m_possessorId);
    if (pOwner != 0)
    {
        if (pOwner->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
        {
            // CSWSCreature::RemoveItem clears the possessor itself, so this arm does not fall
            // through to SetPossessor.
            CSWSCreature* pCreature = pOwner->AsSWSCreature();
            pCreature->RemoveItem(this, 0, 1, 0);
            if (pCreature->m_bInParty != 0 && pData == reinterpret_cast<void*>(1))
                g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_ITEM_LOST, 0);
        }
        else if (pOwner->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
        {
            CSWSItem* pContainer = pOwner->AsSWSItem();
            if (pContainer->m_pItemRepository->GetItemInRepository(this, 0) == 1)
                pContainer->m_pItemRepository->RemoveItem(this);
            SetPossessor(OBJECT_INVALID, 1, 1, OBJECT_INVALID);
        }
        else if (pOwner->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
        {
            CSWSPlaceable* pPlaceable = pOwner->AsSWSPlaceable();
            if (pPlaceable->m_bHasInventory == 1 &&
                pPlaceable->m_pItemRepository->GetItemInRepository(this, 0) == 1)
                pPlaceable->m_pItemRepository->RemoveItem(this);
            SetPossessor(OBJECT_INVALID, 1, 1, OBJECT_INVALID);
        }
        else if (pOwner->m_objectType == CGameObject::OBJECT_TYPE_STORE)
        {
            CSWSStore* pStore = pOwner->AsSWSStore();
            if (pStore->GetItemInInventory(m_objectId) == 1)
                pStore->RemoveItem(this);
            SetPossessor(OBJECT_INVALID, 1, 1, OBJECT_INVALID);
        }
    }
    RemoveFromArea();
    delete this;
}

void CSWSItem::CloseItemForAllPlayers()
{
    // Only container items have a UI to close.
    int nBaseItem = m_baseItem;
    CSWBaseItem* pBaseItem = g_pRules->m_pBaseItemArray->GetBaseItem(nBaseItem);
    if (pBaseItem == 0 || pBaseItem->m_bContainer == 0) return;

    CExoLinkedList<CSWSClient>* pList = g_pAppManager->m_pServerExoApp->GetPlayerList();
    CExoLinkedListNode* pNode = pList->m_pInternal->m_pHead;
    if (pNode != 0) {
        do {
            CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(pList->m_pInternal->GetAtPos(pNode));
            if (pPlayer != 0) {
                CGameObject* pGameObj = pPlayer->GetGameObject();
                if (pGameObj != 0 && pPlayer->m_pContainerGUI->m_objectId == m_objectId) {
                    CloseInventory(pGameObj->m_objectId, 1);
                }
            }
            pList->m_pInternal->GetNext(pNode);
        } while (pNode != 0);
    }
}

int CItemRepository::GetItemInRepository(CSWSItem* pItem, int bRecurse)
{
    for (int i = 0; i < m_aItems.m_count; i++)
    {
        CSWSItem* pCur = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(m_aItems.m_pArray[i]);
        if (pCur == pItem)
            return 1;
        if (bRecurse && pCur != 0)
        {
            CSWBaseItem* pBaseItem = g_pRules->m_pBaseItemArray->GetBaseItem(pCur->m_baseItem);
            if (pBaseItem != 0 && pBaseItem->m_bContainer != 0 && pCur->m_pItemRepository != 0 &&
                pCur->m_pItemRepository->GetItemInRepository(pItem, bRecurse) != 0)
                return 1;
        }
    }
    return 0;
}

int CItemRepository::RemoveItem(CSWSItem* pItem)
{
    if (pItem == 0) return 0;
    int i = 0;
    while (i < m_aItems.m_count)
    {
        if (pItem->m_objectId == m_aItems.m_pArray[i])
        {
            m_aItems.DelIndex(i);
            // An item that was still flagged unseen stops counting towards the repository's badge.
            if (pItem->m_bits.m_bNewItem) {
                m_nNewItemCount = m_nNewItemCount - 1;
                pItem->m_bits.m_bNewItem = 0;
            }
            return 1;
        }
        i++;
    }
    return 0;
}

int CItemRepository::AddItem(CSWSItem** ppItem, int bStack, int nPossessor, int bNew)
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(m_objectId);
    if (pObject != 0)
    {
        if (pObject->AsSWSCreature() != 0)
        {
            // The consumable categories never enter the list: the owner's counter eats them and
            // the item is destroyed. Each test re-reads GetBaseItem().
            if ((*ppItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_CREDITS)
            {
                pObject->AsSWSCreature()->AddGold((*ppItem)->m_stackCount, 1);
                goto consumed;
            }
#if !KOTOR1  // K1 has no chemicals or components
            else if ((*ppItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_CHEMICALS)
            {
                pObject->AsSWSCreature()->AddChemical((*ppItem)->m_stackCount, 1);
                goto consumed;
            }
            else if ((*ppItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_COMPONENTS)
            {
                pObject->AsSWSCreature()->AddComponent((*ppItem)->m_stackCount, 1);
                goto consumed;
            }
#endif
            else if ((*ppItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_PAZAAK_CARD)
            {
                pObject->AsSWSCreature()->AddPazaakCard(*ppItem);
                goto consumed;
            }
        }
        else if (pObject->AsSWSItem() != 0)
        {
            // A container item's own repository credits the two consumable kinds to whoever is
            // carrying the container, not to the container.
            CSWSItem* pContainer = pObject->AsSWSItem();
            if (g_pRules->m_pBaseItemArray->GetBaseItem(pContainer->m_baseItem)->m_bContainer == 1 &&
                pContainer->m_possessorId != OBJECT_INVALID)
            {
                CSWSCreature* pCarrier = g_pAppManager->m_pServerExoApp
                                             ->GetCreatureByGameObjectID(pContainer->m_possessorId);
                if (pCarrier != 0)
                {
                    if ((*ppItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_CREDITS)
                    {
                        pCarrier->AddGold((*ppItem)->m_stackCount, 1);
                        goto consumed;
                    }
                    else if ((*ppItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_PAZAAK_CARD)
                    {
                        pCarrier->AddPazaakCard(*ppItem);
                        goto consumed;
                    }
                }
            }
        }
    }

    {
        int nLanguage = g_pAppManager->m_pClientExoApp->GetClientLanguage();
        CExoString sName;
        CExoString sSortName;
        // Only the party-wide inventory sorts by name; the others keep insertion order.
        if (m_bPartyInventory)
            (*ppItem)->m_localizedName.GetString(nLanguage, &sName, 0);

        // nIndex is where the newcomer goes; bFound says whether the scan below actually picked a
        // position (the party branch needs both, the plain branch ignores them and passes 0).
        int nIndex = 0;
        int bFound = 0;
        if (bStack == 1)
        {
            // Stacking requested: one pass looks for both a merge partner and the sort position.
            // The position loop stops updating once it has a name to sort against (bFound), while
            // the merge test keeps running over every remaining entry.
            for (int i = 0; i < m_aItems.m_count; i++)
            {
                CSWSItem* pCur =
                    g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(m_aItems.m_pArray[i]);
                if (pCur == 0)
                    continue;
                if (bFound == 0 && m_bPartyInventory)
                {
                    pCur->m_localizedName.GetString(nLanguage, &sSortName, 0);
                    if (static_cast<unsigned int>((*ppItem)->m_baseItem) < static_cast<unsigned int>(pCur->m_baseItem) ||
                        ((*ppItem)->m_baseItem == pCur->m_baseItem && sName < sSortName))
                    {
                        bFound = 1;
                        nIndex = i;
                    }
                    else
                    {
                        bFound = 0;
                    }
                }
                if (pCur->m_baseItem != (*ppItem)->m_baseItem ||
                    pCur->m_nModelVariation != (*ppItem)->m_nModelVariation)
                    continue;
                if (pCur->MergeItem(*ppItem) == 1)
                {
                    // Absorbed: the incoming item is gone, so the client is told to drop it and
                    // *ppItem is re-pointed at the survivor.
                    if (pObject != 0)
                    {
                        CSWSPlayer* pPlayer = 0;
                        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE)
                            pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                                m_objectId);
                        else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_ITEM)
                            pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                                pObject->AsSWSItem()->m_possessorId);
                        if (pPlayer != 0)
                            g_pAppManager->m_pServerExoApp->GetSWSMessage()
                                ->SendServerPlayerItemUpdate_DestroyItem(pPlayer,
                                                                        (*ppItem)->m_objectId);
                    }
                    delete *ppItem;
                    *ppItem = pCur;
                    pCur->SetPossessor(pCur->m_possessorId, nPossessor, nPossessor, OBJECT_INVALID);
                    if (bNew)
                        m_nNewItemCount++;
                    return 1;
                }
            }
        }
        else
        {
            // No stacking: the party inventory files the newcomer before the first item whose name
            // sorts after it (the loop counter is the insert index).
            nIndex = 0;
            bFound = 0;
            if (m_bPartyInventory)
            {
                for (int i = 0; i < m_aItems.m_count; i++)
                {
                    CSWSItem* pCur =
                        g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(m_aItems.m_pArray[i]);
                    if (pCur != 0 && m_bPartyInventory)
                    {
                        pCur->m_localizedName.GetString(nLanguage, &sSortName, 0);
                        if (sName < sSortName)
                        {
                            bFound = 1;
                            nIndex = i;
                            break;
                        }
                    }
                }
            }
        }

        if (m_bPartyInventory)
        {
            // The party inventory stamps the new item with the driven player's id before filing it.
            CSWSPlayer* pPlayer =
                static_cast<CSWSPlayer*>(g_pAppManager->m_pServerExoApp->GetClientObjectByPlayerId(0, 1));
            if (pPlayer != 0)
                (*ppItem)->SetPossessor(pPlayer->m_gameObjectId, nPossessor, nPossessor,
                                        OBJECT_INVALID);
            if (bFound)
            {
                // The list's own Insert, not InsertItem().
                m_aItems.Insert((*ppItem)->m_objectId, nIndex);
            }
            else
            {
                m_aItems.Add((*ppItem)->m_objectId);
            }
        }
        else
        {
            m_aItems.Insert((*ppItem)->m_objectId, 0);
        }
        // A new arrival is flagged unseen and counted, so the GUI can pulse the items button.
        if (bNew)
            (*ppItem)->m_flags |= CSWSItem::FLAG_NEW_ITEM;
        if ((*ppItem)->m_bits.m_bNewItem)
            m_nNewItemCount++;
    }
    return 1;

consumed:
    delete *ppItem;
    *ppItem = 0;
    return 1;
}

void CItemRepository::InsertItem(CSWSItem* pItem, int index)
{
    if (m_bPartyInventory)
        return;
    if (index < 0 || m_aItems.m_count <= index)
        m_aItems.Add(pItem->m_objectId);
    else
        m_aItems.Insert(pItem->m_objectId, index);
}

CSWSItem* CItemRepository::ItemListGetItem(int index)
{
    if (index < 0 || m_aItems.m_count <= index)
        return 0;
    return g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(m_aItems.m_pArray[index]);
}

unsigned long long CItemRepository::ItemListGetItemObjectID(int index)
{
    if (index < 0 || m_aItems.m_count <= index)
        return OBJECT_INVALID;
    return m_aItems.m_pArray[index];
}

void CItemRepository::ClearItems()
{
    m_aItems.m_count = 0;
    m_nNewItemCount = 0;
}

void CItemRepository::ClearNewItems(CExoArrayList<unsigned long long>& list)
{
    for (int i = list.m_count - 1; i >= 0; i--)
    {
        CSWSItem* pItem =
            g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(list.m_pArray[i]);
        if (pItem == 0)
            continue;
        pItem->m_bits.m_bNewItem = 0;
        m_nNewItemCount = m_nNewItemCount - 1;
        list.DelIndex(i);
    }
}

void CItemRepository::SetParentId(unsigned long long objectId)
{
    int nCount = m_aItems.m_count;
    m_objectId = objectId;
    if (nCount < 1)
        return;
    for (int64_t i = 0; i < m_aItems.m_count; i++)
    {
        CSWSItem* pItem =
            g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(m_aItems.m_pArray[i]);
        if (pItem != 0)
            pItem->SetPossessor(m_objectId, 0, 0, OBJECT_INVALID);
    }
}

unsigned long long CItemRepository::FindItemWithBaseItemType(unsigned char baseItemType, int nSkip)
{
    if (m_aItems.m_count < 1)
        return OBJECT_INVALID;
    int nFound = 0;
    for (int64_t i = 0; i < m_aItems.m_count; i++)
    {
        unsigned long long oid = m_aItems.m_pArray[i];
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oid);
        if (pItem == 0)
            continue;
        if (pItem->m_pItemRepository != 0)
        {
            unsigned long long nSub =
                pItem->m_pItemRepository->FindItemWithBaseItemType(baseItemType, nSkip);
            if (nSub != OBJECT_INVALID)
                return nSub;
        }
        CSWBaseItem* pBase = pItem->GetBaseItem();
        if (nFound == nSkip && pBase->m_nItemType == baseItemType)
            return oid;
        // Bitwise &, not &&: the base-item test is evaluated unconditionally.
        nFound += (nFound != nSkip) & (pItem->GetBaseItem()->m_nItemType == baseItemType);
    }
    return OBJECT_INVALID;
}

unsigned long long CItemRepository::FindItemWithTag(const CExoString& tag)
{
    if (m_aItems.m_count < 1)
        return OBJECT_INVALID;
    for (int64_t i = 0; i < m_aItems.m_count; i++)
    {
        CSWSItem* pItem =
            g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(m_aItems.m_pArray[i]);
        if (pItem == 0)
            continue;
        if (pItem->m_sTag == tag)
            return pItem->m_objectId;
        if (pItem->m_pItemRepository != 0)
        {
            unsigned long long nFound = pItem->m_pItemRepository->FindItemWithTag(tag);
            if (nFound != OBJECT_INVALID)
                return nFound;
        }
    }
    return OBJECT_INVALID;
}

int CItemRepository::GetMatchingItemCount(CSWSItem* pItem)
{
    if (m_aItems.m_count < 1)
        return 0;
    for (int64_t i = 0; i < m_aItems.m_count; i++)
    {
        unsigned int nId = static_cast<unsigned int>(m_aItems.m_pArray[i]);
        CSWSItem* pCur = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(nId);
        if (pCur == 0)
            continue;
        if (pCur->m_baseItem != pItem->m_baseItem)
            continue;
        if (pCur->m_nModelVariation != pItem->m_nModelVariation)
            continue;
        if (!pItem->CompareItem(pCur))
            continue;
        if (pCur->m_nFlagsLow & CSWSItem::FLAG_INFINITE)
            return -1;
        return pCur->m_stackCount;
    }
    return 0;
}

int CItemRepository::CalculateContentsWeight()
{
    if (m_aItems.m_count < 1)
        return 0;
    int nTotal = 0;
    for (int64_t i = 0; i < m_aItems.m_count; i++)
    {
        CSWSItem* pItem =
            g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(m_aItems.m_pArray[i]);
        if (pItem == 0)
            continue;
        int nWeight;
        if (pItem->m_pItemRepository != 0)
            nWeight = pItem->m_nWeight +
                      pItem->m_pItemRepository->CalculateContentsWeight();
        else
            nWeight = (pItem->m_stackCount > 1 ? pItem->m_stackCount : 1) * pItem->m_nWeight;
        nTotal += nWeight;
    }
    return nTotal;
}

CSWSItem* CSWSItem::AsSWSItem()
{
    return this;
}

CExoLocString& CSWSItem::GetFirstName()
{
    return m_localizedName;
}

int CSWSItem::ActiveProperty(unsigned short nType)
{
    switch (nType)
    {
    case ITEM_PROPERTY_ACTIVATE_ITEM:
    case ITEM_PROPERTY_SECURITY_SPIKE:
    case ITEM_PROPERTY_TRAP:
    case ITEM_PROPERTY_COMPUTER_SPIKE:
        return 1;
    }
    return 0;
}

int CSWSItem::ComputeArmorClass()
{
    if (GetBaseItem()->m_nModelType == 1)
        return GetBaseItem()->m_nBaseAC;
    return 0;
}

int CSWSItem::GetExistsHigherValuedProperty(unsigned short type, unsigned short subType,
                                           unsigned short value)
{
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp == 0) continue;
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
            continue;
        if (pProp->m_type != type || pProp->m_subType != subType) continue;
        if (pProp->m_costTableValue > value) return 1;
    }
    return 0;
}

void CSWSItem::RestoreUsedActiveProperties(int bSlottedOnly)
{
    int nCount = m_nActiveProperties;
    CSWItemProperty* paProps = m_pActiveProperties;
    for (int64_t i = 0; i < nCount; i++)
    {
        CSWItemProperty* pProp = &paProps[i];
        if (pProp == 0) continue;
        if (pProp->m_type != ITEM_PROPERTY_ACTIVATE_ITEM) continue;
        if (bSlottedOnly != 0 && pProp->m_nUsesPerDay != CSWItemProperty::USES_PER_DAY_UNSET) continue;
        switch (pProp->m_costTableValue)
        {
        case IPRP_CHARGECOST_1_USE_PER_DAY:  pProp->m_nUsesPerDay = 1; break;
        case IPRP_CHARGECOST_2_USES_PER_DAY: pProp->m_nUsesPerDay = 2; break;
        case IPRP_CHARGECOST_3_USES_PER_DAY: pProp->m_nUsesPerDay = 3; break;
        case IPRP_CHARGECOST_4_USES_PER_DAY: pProp->m_nUsesPerDay = 4; break;
        case IPRP_CHARGECOST_5_USES_PER_DAY: pProp->m_nUsesPerDay = 5; break;
        }
        pProp->m_bUseable = 1;
    }
}

void CSWSItem::UpdateUsedActiveProperties()
{
    unsigned char aUsesLeft[8] = {0};
    int64_t i = 0;
    unsigned char nUsedMask = 0;
    for (; i < m_nActiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pActiveProperties[i];
        if (pProp->m_type != ITEM_PROPERTY_ACTIVATE_ITEM)
            continue;
        switch (pProp->m_costTableValue)
        {
        case IPRP_CHARGECOST_SINGLE_USE:
            if (pProp->m_bUseableByte == 0)
                continue;
            break;
        case IPRP_CHARGECOST_5_CHARGES_PER_USE: case IPRP_CHARGECOST_4_CHARGES_PER_USE:
        case IPRP_CHARGECOST_3_CHARGES_PER_USE: case IPRP_CHARGECOST_2_CHARGES_PER_USE:
        case IPRP_CHARGECOST_1_CHARGE_PER_USE:
            if (m_nCharges < 1)
                continue;
            break;
        case IPRP_CHARGECOST_0_CHARGES_PER_USE: case IPRP_CHARGECOST_UNLIMITED_USE:
        case IPRP_CHARGECOST_1_PER_MINUTE: case IPRP_CHARGECOST_1_PER_2_MINUTES:
        case IPRP_CHARGECOST_1_PER_3_MINUTES: case IPRP_CHARGECOST_1_PER_4_MINUTES:
        case IPRP_CHARGECOST_1_PER_5_MINUTES:
            nUsedMask |= static_cast<unsigned char>(1 << i);
            continue;
        case IPRP_CHARGECOST_1_USE_PER_DAY: case IPRP_CHARGECOST_2_USES_PER_DAY:
        case IPRP_CHARGECOST_3_USES_PER_DAY: case IPRP_CHARGECOST_4_USES_PER_DAY:
        case IPRP_CHARGECOST_5_USES_PER_DAY:
            if (pProp->m_nUsesPerDay == 0)
                continue;
            break;
        default:
            continue;
        }
        if (i <= 7)
            aUsesLeft[i] = static_cast<unsigned char>(GetUsedActivePropertyUsesLeft(static_cast<unsigned char>(i)));
        nUsedMask |= static_cast<unsigned char>(1 << i);
    }

    unsigned char nChanged = 0;
    for (int64_t j = 0; j < 8; j++)
    {
        if (aUsesLeft[j] != m_nUsedActivePropertyUsesLeft[j])
            nChanged |= static_cast<unsigned char>(1 << j);
    }

    if (nUsedMask == m_nUsedActivePropertyMask && nChanged == 0)
        return;

    m_nUsedActivePropertyMask = nUsedMask;
    memcpy(m_nUsedActivePropertyUsesLeft, aUsesLeft, sizeof(m_nUsedActivePropertyUsesLeft));

    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    if (CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_possessorId))
        pMessage->SendServerToPlayerUpdateActiveItemProperties(pPlayer, m_objectId,
            m_nUsedActivePropertyMask, nChanged, m_nUsedActivePropertyUsesLeft);
}

void CSWSItem::SetNumActiveProperties(int nCount)
{
    m_nActiveProperties = nCount;
    if (m_pActiveProperties != 0)
        delete[] m_pActiveProperties;
    if (nCount != 0)
        m_pActiveProperties = new CSWItemProperty[m_nActiveProperties];
    else
        m_pActiveProperties = 0;
}

void CSWSItem::SetNumPassiveProperties(int nCount)
{
    m_nPassiveProperties = nCount;
    if (m_pPassiveProperties != 0)
        delete[] m_pPassiveProperties;
    if (nCount != 0)
        m_pPassiveProperties = new CSWItemProperty[m_nPassiveProperties];
    else
        m_pPassiveProperties = 0;
}

void CSWSItem::AddUpgrade(CSWSItem* pUpgrade, unsigned int nSlot)
{
    if (pUpgrade == 0)
        return;

    int nAdd = pUpgrade->m_nPassiveProperties;
    int nOld = m_nPassiveProperties;
    int nNew = nOld + nAdd;

    CSWItemProperty* pNew = new CSWItemProperty[nNew];
    memcpy(pNew, m_pPassiveProperties, nOld * sizeof(CSWItemProperty));

    SetNumPassiveProperties(nNew);
    memcpy(m_pPassiveProperties, pNew, nOld * sizeof(CSWItemProperty));
    delete[] pNew;

    memcpy(&m_pPassiveProperties[nOld], pUpgrade->m_pPassiveProperties,
           nAdd * sizeof(CSWItemProperty));

    for (int i = nOld; i < nNew; i++)
        m_pPassiveProperties[i].m_nPropUpgrade = static_cast<unsigned short>(nSlot + 1);
}

void CSWSItem::RemoveUpgrade(CSWSItem* pUpgrade, unsigned int nSlot)
{
    int nUpg = pUpgrade->m_nPassiveProperties;
    int nNew = m_nPassiveProperties - nUpg;
#ifdef K2_FIX_ENGINE_BUGS
    CSWItemProperty* pNew = new CSWItemProperty[m_nPassiveProperties];
#else
    CSWItemProperty* pNew = new CSWItemProperty[nNew];
#endif

    int n = 0;
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        int bFromSlot = 0;
        if (nUpg > 0)
        {
            for (int k = 0; k < nUpg; k++)
            {
                if (m_pPassiveProperties[i].m_nPropUpgrade == nSlot + 1)
                    bFromSlot = 1;
            }
        }
        if (bFromSlot == 0)
            pNew[n++] = m_pPassiveProperties[i];
    }
#ifdef K2_FIX_ENGINE_BUGS
    nNew = n;
#endif

    SetNumPassiveProperties(nNew);
    if (m_pPassiveProperties != 0)
        memcpy(m_pPassiveProperties, pNew, nNew * sizeof(CSWItemProperty));
    delete[] pNew;
}

int CSWSItem::GetWeight()
{
    if (m_pItemRepository != 0)
        return m_nWeight + m_pItemRepository->CalculateContentsWeight();
    return (m_stackCount > 1 ? m_stackCount : 1) * m_nWeight;
}

unsigned int CSWSItem::GetCost()
{
    if (m_bPlot == 1)
        return 0;
#if KOTOR1  // K1 reads the base item once
    unsigned int nCost = static_cast<unsigned int>(static_cast<float>(m_nAddCost) * GetBaseItem()->m_costMultiplier);
    return nCost > 1 ? nCost : 1;
#else
    if (static_cast<unsigned int>(static_cast<float>(m_nAddCost) * GetBaseItem()->m_costMultiplier) >= 2)
        return static_cast<unsigned int>(static_cast<float>(m_nAddCost) * GetBaseItem()->m_costMultiplier);
    return 1;
#endif
}

CSWCItem* CSWSItem::GetClientItem()
{
    if (GetClientObject() == 0)
        return 0;
    return GetClientObject()->AsSWCItem();
}

void CSWSItem::PowerItem(int a, int b, int c)
{
    K2_UNUSED(b); K2_UNUSED(c);
    CSWCItem* pClientItem = GetClientItem();
    if (pClientItem != 0)
        pClientItem->PowerItem(a, 0, 1, 0);
}

CItemRepository::CItemRepository(unsigned long long ownerObjectId, int flag)
{
    m_objectId = ownerObjectId;
    m_nNewItemCount = 0;
    m_bPartyInventory = flag;
}

CItemRepository::~CItemRepository()
{
}

void CSWSItem::InitRepository(unsigned long long ownerObjectId)
{
    if (m_pItemRepository != 0)
    {
        delete m_pItemRepository;
        m_pItemRepository = 0;
    }
    if (g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_bContainer != 0)
        m_pItemRepository = new CItemRepository(ownerObjectId, 0);
}

int CSWSItem::MergeItem(CSWSItem* pOther)
{
    if (CompareItem(pOther) == 0)
        return 0;
    int nMaxStack = g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nStacking;
    int nCount = m_stackCount;
    if (nCount + pOther->m_stackCount > nMaxStack)
    {
        m_stackCount = nMaxStack;
        pOther->m_stackCount = pOther->m_stackCount + nCount - nMaxStack;
        return 0;
    }
    m_stackCount = nCount + pOther->m_stackCount;
    return 1;
}

CSWSItem* CSWSItem::SplitItem(int nCount)
{
    if (nCount < 1)
        return 0;
    if (m_stackCount <= nCount)
        return 0;
    CSWSItem* pNewItem = new CSWSItem(OBJECT_INVALID);
    pNewItem->CopyItem(this);
    m_stackCount = m_stackCount - nCount;
    pNewItem->m_stackCount = nCount;
    return pNewItem;
}

int CSWSItem::CopyItem(CSWSItem* pSource)
{
    if (pSource == 0)
        return 0;
    if (pSource->m_pItemRepository != 0 && pSource->m_pItemRepository->ItemListGetCount() != 0)
        return 0;

    m_descriptionIdentified = pSource->m_descriptionIdentified;
    m_description = pSource->m_description;
    m_localizedName = pSource->m_localizedName;
    m_sTag = pSource->m_sTag;
    m_possessorId = pSource->m_possessorId;
    if (pSource->m_pItemRepository != 0)
        m_pItemRepository = new CItemRepository(m_objectId, 0);
    m_nModelVariation = pSource->m_nModelVariation;

    m_nActiveProperties = pSource->m_nActiveProperties;
    if (m_nActiveProperties > 0)
    {
        m_pActiveProperties = new CSWItemProperty[m_nActiveProperties];
        for (int i = 0; i < m_nActiveProperties; i++)
            m_pActiveProperties[i] = pSource->m_pActiveProperties[i];
    }
    m_nPassiveProperties = pSource->m_nPassiveProperties;
    if (m_nPassiveProperties > 0)
    {
        m_pPassiveProperties = new CSWItemProperty[m_nPassiveProperties];
        for (int i = 0; i < m_nPassiveProperties; i++)
            m_pPassiveProperties[i] = pSource->m_pPassiveProperties[i];
    }

    m_nAddCost = pSource->m_nAddCost;
    m_nCharges = pSource->m_nCharges;
    m_nMaxCharges = pSource->m_nMaxCharges;
    m_nBaseArmorClass = pSource->m_nBaseArmorClass;
    m_flags = (m_flags & ~FLAG_INFINITE) | (pSource->m_flags & FLAG_INFINITE);
    m_stackCount = pSource->m_stackCount;
    m_nWeight = pSource->m_nWeight;
    m_flags = (m_flags & ~FLAG_IDENTIFIED) | (pSource->m_flags & FLAG_IDENTIFIED);
    m_nUpgrades = pSource->m_nUpgrades;
#if !KOTOR1  // K1 has no upgrade level or upgrade vars
    m_nUpgradeLevel = pSource->m_nUpgradeLevel;
    __builtin_memcpy(m_anUpgVars, pSource->m_anUpgVars, sizeof(m_anUpgVars));
#endif
    m_nBodyVariation = pSource->m_nBodyVariation;
    m_nTextureVar = pSource->m_nTextureVar;
    m_flags = (m_flags & ~FLAG_NON_EQUIPPABLE) | (pSource->m_flags & FLAG_NON_EQUIPPABLE);

    m_baseItem = pSource->m_baseItem;
    for (int i = 0; i < 6; i++)
        m_aModelParts[i] = pSource->m_aModelParts[i];

    g_pAppManager->m_pServerExoApp->GetModule()->AddObjectToLookupTable(m_sTag, m_objectId);
    return 1;
}

int CSWSItem::AcquireItem(CSWSItem** ppItem, unsigned long long possessorId, int flag)
{
    CSWSItem* pOldPossessor = 0;
    // The result is unused, but the call is required.
    g_pAppManager->m_pServerExoApp->GetObjectArray();

    if ((*ppItem)->m_possessorId != possessorId)
    {
        pOldPossessor =
            g_pAppManager->m_pServerExoApp->GetItemByGameObjectID((*ppItem)->m_possessorId);
        if (pOldPossessor == 0 || pOldPossessor->m_possessorId != possessorId)
            return 0;
        possessorId = (*ppItem)->m_possessorId;
    }

    CItemRepository* pRepository = m_pItemRepository;
    CGameObject* pOwner = g_pAppManager->m_pServerExoApp->GetGameObject(possessorId);
    if (pOwner != 0)
    {
        if (pOwner->m_objectType == GAME_OBJECT_TYPE_CREATURE)
        {
            pOwner->AsSWSCreature()->RemoveItem(*ppItem, 1, flag, 0);
        }
        else if (pOwner->m_objectType == GAME_OBJECT_TYPE_ITEM)
        {
            if (pOwner->AsSWSItem()->m_pItemRepository != 0)
                pOwner->AsSWSItem()->m_pItemRepository->RemoveItem(*ppItem);
        }
        else if (pOwner->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
        {
            pOwner->AsSWSPlaceable()->RemoveItem(*ppItem);
        }
        pRepository->AddItem(ppItem, 1, 0, 1);
    }
    else
    {
        if (pOldPossessor != 0 &&
            g_pRules->m_pBaseItemArray->GetBaseItem((*ppItem)->m_baseItem)->m_bContainer != 0 &&
            g_pRules->m_pBaseItemArray->GetBaseItem(pOldPossessor->m_baseItem)->m_bContainer != 0)
            return 0;
        pRepository->AddItem(ppItem, 1, 0, 1);
        if (*ppItem != 0)
            (*ppItem)->RemoveFromArea();
    }
    if (*ppItem != 0)
        (*ppItem)->SetPossessor(m_objectId, 1, flag, OBJECT_INVALID);
    return 1;
}

int CSWSItem::GetPropertyByTypeExists(unsigned short nType, unsigned short nSubType)
{
    for (int i = 0; i < m_nActiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pActiveProperties[i];
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0) continue;
        if (pProp->m_type != nType) continue;
        if (nSubType != 0 && pProp->m_subType != nSubType) continue;
        return 1;
    }
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0) continue;
        if (pProp->m_type != nType) continue;
        if (nSubType != 0 && pProp->m_subType != nSubType) continue;
        return 1;
    }
    return 0;
}

// Maps the base item's upgrade category to an UPGRADABLE_TYPE; any other category (and every
// non-container item) is UPGRADABLE_TYPE_NONE.
#if KOTOR1  // K1 derives the category from the slotted properties' "UpgradeType" column
// The "UpgradeType" column of upgrade.2da: the kind of upgrade a row is.
enum {
    UPGRADE_TYPE_CRYSTAL = 0,
    UPGRADE_TYPE_VIBRATION_CELL = 1,
    UPGRADE_TYPE_DURASTEEL_ALLOY = 2,
    UPGRADE_TYPE_ENERGY_PROJECTOR = 3,
    UPGRADE_TYPE_SCOPE = 4,
    UPGRADE_TYPE_ENERGY_CELL = 5,
    UPGRADE_TYPE_BEAM_SPLITTER = 6,
    UPGRADE_TYPE_HAIR_TRIGGER = 7,
    UPGRADE_TYPE_ARMOR_REINFORCEMENT = 8,
    UPGRADE_TYPE_MESH_UNDERLAY = 9,
};

unsigned char CSWSItem::GetUpgradableType(C2DA* pTable)
{
    unsigned char nType = UPGRADABLE_TYPE_NONE;
    int nValue;
    for (int i = 0; i < m_nActiveProperties; i++)
    {
        unsigned char nSlot = m_pActiveProperties[i].m_nUpgradeType;
        if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE)
            continue;
        pTable->GetINTEntry(nSlot, CExoString("UpgradeType"), &nValue);
        switch (nValue)
        {
        case UPGRADE_TYPE_CRYSTAL: nType = UPGRADABLE_TYPE_LIGHTSABER; break;
        case UPGRADE_TYPE_VIBRATION_CELL: case UPGRADE_TYPE_DURASTEEL_ALLOY:
        case UPGRADE_TYPE_ENERGY_PROJECTOR:
            nType = UPGRADABLE_TYPE_MELEE;
            break;
        case UPGRADE_TYPE_SCOPE: case UPGRADE_TYPE_ENERGY_CELL: case UPGRADE_TYPE_BEAM_SPLITTER:
        case UPGRADE_TYPE_HAIR_TRIGGER:
            nType = UPGRADABLE_TYPE_RANGED;
            break;
        case UPGRADE_TYPE_ARMOR_REINFORCEMENT: case UPGRADE_TYPE_MESH_UNDERLAY:
            nType = UPGRADABLE_TYPE_ARMOR;
            break;
        }
    }
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        unsigned char nSlot = m_pPassiveProperties[i].m_nUpgradeType;
        if (nSlot == CSWItemProperty::UPGRADE_TYPE_NONE)
            continue;
        pTable->GetINTEntry(nSlot, CExoString("UpgradeType"), &nValue);
        switch (nValue)
        {
        case UPGRADE_TYPE_CRYSTAL: nType = UPGRADABLE_TYPE_LIGHTSABER; break;
        case UPGRADE_TYPE_VIBRATION_CELL: case UPGRADE_TYPE_DURASTEEL_ALLOY:
        case UPGRADE_TYPE_ENERGY_PROJECTOR:
            nType = UPGRADABLE_TYPE_MELEE;
            break;
        case UPGRADE_TYPE_SCOPE: case UPGRADE_TYPE_ENERGY_CELL: case UPGRADE_TYPE_BEAM_SPLITTER:
        case UPGRADE_TYPE_HAIR_TRIGGER:
            nType = UPGRADABLE_TYPE_RANGED;
            break;
        case UPGRADE_TYPE_ARMOR_REINFORCEMENT: case UPGRADE_TYPE_MESH_UNDERLAY:
            nType = UPGRADABLE_TYPE_ARMOR;
            break;
        }
    }
    return nType;
}
#else
unsigned char CSWSItem::GetUpgradableType(C2DA* pTable)
{
    K2_UNUSED(pTable);
    if (m_nUpgradeLevel < 1)
        return UPGRADABLE_TYPE_NONE;
    unsigned char nCategory = GetBaseItem()->m_nItemType;
    if (nCategory == ITEM_TYPE_DOUBLE_WEAPON || nCategory == ITEM_TYPE_SHORT_BLADE ||
        nCategory == ITEM_TYPE_LONG_BLADE)
        return UPGRADABLE_TYPE_MELEE;
    if (nCategory == ITEM_TYPE_PISTOL || nCategory == ITEM_TYPE_RIFLE ||
        nCategory == ITEM_TYPE_HEAVY_WEAPON)
        return UPGRADABLE_TYPE_RANGED;
    if (nCategory == ITEM_TYPE_DOUBLE_LIGHTSABER || nCategory == ITEM_TYPE_SHORT_LIGHTSABER ||
        nCategory == ITEM_TYPE_LIGHTSABER)
        return UPGRADABLE_TYPE_LIGHTSABER;
    if (nCategory >= ITEM_TYPE_ARMOR_CLASS_4 && nCategory <= ITEM_TYPE_JEDI_ROBE)
        return UPGRADABLE_TYPE_ARMOR;
    return UPGRADABLE_TYPE_NONE;
}
#endif

CResRef CSWSItem::GetIcon()
{
    CSWBaseItem* pBaseItem = GetBaseItem();
    if (pBaseItem == 0)
        return CResRef("");
    if (pBaseItem->m_nModelType != 0)
        return pBaseItem->GetIconResRef(m_nTextureVar, 'm');
    return pBaseItem->GetIconResRef(m_nModelVariation, 'm');
}

CExoString CSWSItem::GetPropertyDescription()
{
#if !KOTOR1
    CExoArrayList<CExoString> aPropertyStrings;
#endif
    CExoString sProperties;
#if KOTOR1
    if (GetBaseItem()->m_nItemType != ITEM_TYPE_LIGHTSABER_CRYSTAL && GetBaseItem()->m_nItemType != ITEM_TYPE_GRENADE)
    {
        AddFeatRequirements(sProperties);
        if (GetBaseItem()->m_nWeaponType != 0)
        {
            AddDamageProperties(sProperties);
            AddRangeProperties(sProperties);
            AddCriticalThreatProperties(sProperties);
#else
    if (GetBaseItem()->m_nItemType != ITEM_TYPE_LIGHTSABER_CRYSTAL && GetBaseItem()->m_nItemType != ITEM_TYPE_GRENADE &&
        GetBaseItem()->m_nItemType != ITEM_TYPE_ROCKET)
    {
        AddAttributeRequirements(sProperties);
        AddFeatRequirements(sProperties);
        AddGenderRequirements(sProperties);
        AddSubraceRequirements(sProperties);
        AddPCRequirements(sProperties);
        AddCNPCRequirements(sProperties);
        if (GetBaseItem()->m_nWeaponType != 0 || m_baseItem == BASE_ITEM_GAUNTLETS)
        {
            AddDamageProperties(sProperties);
            AddRangeProperties(sProperties);
            if (m_baseItem != BASE_ITEM_GAUNTLETS)
                AddCriticalThreatProperties(sProperties);
#endif
            AddOnHitProperties(sProperties);
            AddWeaponSizeProperties(sProperties);
        }
        AddAttackModifierProperties(sProperties);
        AddDefenceProperties(sProperties);
        AddMiscellaneousProperties(sProperties);
    }
    CExoString sDescription;
    m_descriptionIdentified.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                      &sDescription, 0);
    if (sDescription == "")
        sDescription = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NO_DESCRIPTION_SET);
    sDescription = sProperties + sDescription;
    return sDescription;
}

#if KOTOR1
void CSWSItem::GetPropertyStrings(unsigned short nType, unsigned short nSubType, unsigned short nValue,
                                  unsigned char nSlot, CExoString& s1, CExoString& s2, CExoString& s3,
                                  CExoString& s4)
#else
void CSWSItem::GetPropertyStrings(unsigned short nType, unsigned short nSubType, unsigned short nValue,
                                  unsigned char nSlot, CExoString& s1, CExoString& s2, CExoString& s3,
                                  CExoString& s4, int nFlag)
#endif
{
    int nIndex = -1;
    int nMapped;
    C2DA* pParamTable;
    C2DA* pSubTypeTable;
    if (g_pRules->m_p2DArrays->m_pItemPropDef->GetINTEntry(nType, CExoString("Name"), &nIndex) == 0)
        return;
#if KOTOR1
    s1 = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
#else
    if (nFlag != 0)
    {
        switch (nIndex)
        {
        case STRREF_ATTRIBUTE_BONUS:
        case STRREF_DAMAGE_RESISTANCE:
        case STRREF_ATTRIBUTE_PENALTY:
        case STRREF_ALIGNMENT_LIMITATION:
            nIndex = -1;
            break;
        case STRREF_DEFENSE_BONUS_VS_DAMAGE_TYPE:
            nIndex = STRREF_DEFENSE_VS_DAMAGE;
            break;
        case STRREF_DAMAGE_BONUS_VS_ALIGNMENT:
            nIndex = STRREF_DAMAGE_VS_ALIGN;
            break;
        case STRREF_DAMAGE_IMMUNITY:
            nIndex = STRREF_IMMUNITY;
            break;
        case STRREF_ATTACK_BONUS:
            nIndex = STRREF_ATTACK_PLUS;
            break;
        case STRREF_DAMAGE_BONUS:
            nIndex = STRREF_DAMAGE_PLUS;
            break;
        }
        if (nIndex == STRREF_PC_RESTRICTION)
            nIndex = -1;
    }
    if (nIndex != -1)
        s1 = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
#endif
    CExoString sSubTypeResRef;
    CExoString sUnused;
    if (g_pRules->m_p2DArrays->m_pItemPropDef->GetCExoStringEntry(
            nType, CExoString("SubTypeResRef"), &sSubTypeResRef) == 1)
    {
        pSubTypeTable = new C2DA(CResRef(sSubTypeResRef), 0);
        if (!pSubTypeTable->Load2DArray())
            return;
        pSubTypeTable->GetINTEntry(nSubType, CExoString("Name"), &nIndex);
#if !KOTOR1
        if (nFlag != 0)
        {
            switch (nIndex)
            {
            case STRREF_DAMAGE_TYPE_LIGHT_SIDE:
            case STRREF_DAMAGE_TYPE_DARK_SIDE:
                nIndex = -1;
                break;
            case STRREF_BLUDGEONING:  // blanked, unlike slashing below
            case STRREF_BLUDGEONING_2:
                nIndex = -1;
                s1 = "";
                return;
            case STRREF_PIERCING:
            case STRREF_PIERCING_2:
                nIndex = -1;
                s1 = "";
                return;
            case STRREF_SLASHING:
            case STRREF_SLASHING_2:
                nIndex = STRREF_DAMAGE_PHYSICAL;
                break;
            case 0:
                nIndex = -1;
                break;
            }
        }
#endif
        s2 = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
        delete pSubTypeTable;
    }
    if (nValue != PROPERTY_VALUE_NONE)
    {
        if (g_pRules->m_p2DArrays->m_pItemPropDef->GetINTEntry(nType, CExoString("CostTableResRef"),
                                                               &nMapped) == 1)
        {
            C2DA* pCostTable =
                g_pRules->m_p2DArrays->GetIPRPCostTable(static_cast<unsigned char>(nMapped));
            pCostTable->GetINTEntry(nValue, CExoString("Name"), &nIndex);
#if KOTOR1
            s3 = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
#else
            if (nIndex > 0)
                s3 = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
#endif
        }
    }
    if (nSlot != PROPERTY_PARAM_NONE)
    {
        if (g_pRules->m_p2DArrays->m_pItemPropDef->GetINTEntry(nType, CExoString("Param1ResRef"),
                                                               &nMapped) == 1)
        {
            pParamTable = g_pRules->m_p2DArrays->GetIPRPParamTable(static_cast<unsigned char>(nMapped));
            pParamTable->GetINTEntry(nSlot, CExoString("Name"), &nIndex);
            s4 = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
        }
        else
        {
            if (g_pRules->m_p2DArrays->m_pItemPropDef->GetCExoStringEntry(
                    nType, CExoString("SubTypeResRef"), &sSubTypeResRef) == 1)
            {
                pSubTypeTable = new C2DA(CResRef(sSubTypeResRef), 0);
                // A failed load still deletes this table, unlike the earlier SubTypeResRef
                // lookup, whose load leaks.
                if (pSubTypeTable->Load2DArray())
                {
                    if (pSubTypeTable->GetINTEntry(nSubType, CExoString("Param1ResRef"), &nMapped)
                        == 1)
                    {
                        pParamTable
                            = g_pRules->m_p2DArrays->GetIPRPParamTable(static_cast<unsigned char>(nMapped));
                        pParamTable->GetINTEntry(nSlot, CExoString("Name"), &nIndex);
                        s4 = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
                    }
                }
                delete pSubTypeTable;
            }
        }
    }
}

CExoString CSWSItem::GetPropertiesOnlyDescription()
{
    CExoString sResult;
    int nCount = 0;
    for (int i = 1; i <= m_nPassiveProperties && nCount < 5; i++)
    {
        CSWItemProperty* pProp = GetPassiveProperty(i - 1);
        if (pProp == 0)
            continue;
        CExoString sName, sDescriptor, sValue, sUnused1, sUnused2;
        GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE, sName,
                           sDescriptor, sValue, sUnused1, 1);
        if (sName.m_sString != 0 && sName.m_sString[0] != 0)
            sResult = sResult + sName;
        if (sValue.m_sString != 0 && sValue.m_sString[0] != 0 &&
            sName.m_sString != 0 && sName.m_sString[0] != 0)
        {
            if (sName[static_cast<int>(strlen(sName.m_sString)) - 1] == '+')
                sResult = sResult + sValue;
            else
                sResult = sResult + ": " + sValue;
        }
        if (sValue.m_sString != 0 && sValue.m_sString[0] != 0 &&
            (sName.m_sString == 0 || sName.m_sString[0] == 0))
            sResult = sResult + sValue;
        if (sName.m_sString != 0 && sName.m_sString[0] != 0 &&
            sValue.m_sString != 0 && sValue.m_sString[0] != 0 &&
            sDescriptor.m_sString != 0 && sDescriptor.m_sString[0] != 0)
            sResult = sResult + ", " + sDescriptor;
        if (sName.m_sString != 0 && sName.m_sString[0] != 0 &&
            (sValue.m_sString == 0 || sValue.m_sString[0] == 0) &&
            sDescriptor.m_sString != 0 && sDescriptor.m_sString[0] != 0)
            sResult = sResult + ": " + sDescriptor;
        if ((sName.m_sString == 0 || sName.m_sString[0] == 0) &&
            sValue.m_sString != 0 && sValue.m_sString[0] != 0 &&
            sDescriptor.m_sString != 0 && sDescriptor.m_sString[0] != 0)
            sResult = sResult + " " + sDescriptor;
        if ((sName.m_sString == 0 || sName.m_sString[0] == 0) &&
            (sValue.m_sString == 0 || sValue.m_sString[0] == 0) &&
            sDescriptor.m_sString != 0 && sDescriptor.m_sString[0] != 0)
            sResult = sResult + sDescriptor;
        if ((sName.m_sString != 0 && sName.m_sString[0] != 0) ||
            (sValue.m_sString != 0 && sValue.m_sString[0] != 0) ||
            (sDescriptor.m_sString != 0 && sDescriptor.m_sString[0] != 0))
        {
            sResult = sResult + "\n";
            nCount++;
        }
    }
    return sResult;
}

int CSWSItem::GetDamageFlags()
{
    int nBaseItem = m_baseItem;
    CSWBaseItem* pBaseItem = g_pRules->m_pBaseItemArray->GetBaseItem(nBaseItem);
    if (pBaseItem != 0 && pBaseItem->m_nDamageFlags != 0)
        return pBaseItem->m_nDamageFlags;
#if KOTOR1  // K1's fallback is 8, not 1
    return 8;
#else
    return 1;
#endif
}

int CSWSItem::GetPropertyByType(CSWItemProperty** ppOut, unsigned short nType,
                                unsigned short nSubType)
{
    for (int i = 0; i < m_nActiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pActiveProperties[i];
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0) continue;
        if (pProp->m_type != nType) continue;
        if (nSubType != 0 && pProp->m_subType != nSubType) continue;
        *ppOut = pProp;
        return 1;
    }
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0) continue;
        if (pProp->m_type != nType) continue;
        if (nSubType != 0 && pProp->m_subType != nSubType) continue;
        *ppOut = pProp;
        return 1;
    }
    return 0;
}

unsigned char CSWSItem::GetMinEquipLevel()
{
    C2DA* pItemValues = g_pRules->m_p2DArrays->m_pItemValues;
    int nLevel = 0;
    int nValue;
    while (nLevel < pItemValues->m_nRows)
    {
        pItemValues->GetINTEntry(nLevel, CExoString("MAXSINGLEITEMVALUE"), &nValue);
        if (m_bPlot == 1) break;
        if (GetCost() <= static_cast<unsigned int>(nValue)) break;
        nLevel++;
    }
    return static_cast<unsigned char>(nLevel + 1);
}

void CSWSItem::AddToArea(CSWSArea* pArea, float x, float y, float z, int flag)
{
    Vector pos = { x, y, z };
    RemoveFromArea();
    if (pArea != 0)
    {
        SetArea(pArea);
        SetPosition(pos, 0, 1, 0);
        static_cast<CSWSArea*>(GetArea())->AddObjectToArea(m_objectId, flag);
    }
}

void CSWSItem::ApplyItemProperties(CSWSCreature* pCreature, unsigned int nSlot, int a)
{
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    if (pAIMaster == 0)
        return;
    for (int64_t i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0) continue;
        pAIMaster->OnItemPropertyApplied(this, pProp, pCreature, nSlot, a);
    }
}

void CSWSItem::RemoveItemProperties(CSWSCreature* pCreature, unsigned int a)
{
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    if (pAIMaster != 0)
    {
        for (int64_t i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0) continue;
            pAIMaster->OnItemPropertyRemoved(this, pProp, pCreature, a);
        }
    }
    pCreature->RemoveEffectByCreator(m_objectId);
}

int CSWSItem::LoadFromTemplate(const CResRef& templateRef)
{
    CResGFF* pGFF = new CResGFF(RESTYPE_UTI, const_cast<char*>("UTI "), templateRef);
    if (pGFF->m_nValid == 0)
    {
        delete pGFF;
        return 0;
    }
    CResStruct sStruct;
    pGFF->GetTopLevelStruct(&sStruct);
    LoadDataFromGff(pGFF, &sStruct);
    delete pGFF;
    return 1;
}

void CSWSItem::OpenInventory(unsigned long long objectId)
{
    if (m_nFlagsLow & FLAG_INVENTORY_OPEN)
        return;
    int nBaseItem = m_baseItem;
    if (g_pRules->m_pBaseItemArray->GetBaseItem(nBaseItem)->m_bContainer == 0)
        return;
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(objectId);
    if (pPlayer != 0)
    {
        int nHasItems = (m_pItemRepository->ItemListGetCount() != 0);
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerGuiContainerObject_Open(
            pPlayer, m_objectId, nHasItems);
    }
    CScriptEvent* pEvent = new CScriptEvent();
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OPEN;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, objectId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    m_flags = m_flags | FLAG_INVENTORY_OPEN;
}

void CSWSItem::AIUpdate()
{
    if (GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0) == 0)
        return;

    unsigned int nDay, nTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);

    for (int64_t i = 0; i < m_nActiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pActiveProperties[i];
        if (pProp == 0)
            continue;
        if (pProp->m_type != ITEM_PROPERTY_ACTIVATE_ITEM)
            continue;
        unsigned short nValue = pProp->m_costTableValue;
        if (static_cast<unsigned short>(nValue - 14) > 4)
            continue;
        if (pProp->m_bUseableByte != 0)
            continue;
        unsigned int nElapsedDay, nElapsedTod;
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
            nDay, nTod, pProp->m_nLastUseDay, pProp->m_nLastUseTod, &nElapsedDay, &nElapsedTod);
        if (((nValue == 14) & (nElapsedTod > 59999)) |
            ((nValue == 15) & (nElapsedTod > 119999)) |
            ((nValue == 16) & (nElapsedTod > 179999)) |
            ((nValue == 17) & (nElapsedTod > 239999)) |
            ((nValue == 18) & (nElapsedTod > 299999)))
            pProp->m_bUseable = 1;
    }
}

void CSWSItem::CloseInventory(unsigned long long objectId, int flag)
{
    if ((m_nFlagsLow & FLAG_INVENTORY_OPEN) == 0)
        return;
    if (flag != 0)
    {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(objectId);
        if (pPlayer != 0)
        {
            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            while (m_pItemRepository->ItemListGetCount() != 0)
            {
                CSWSItem* pItem = m_pItemRepository->ItemListGetItem(
                    m_pItemRepository->ItemListGetCount() - 1);
                if (pItem != 0)
                    m_pItemRepository->RemoveItem(pItem);
                pCreature->GetItemRepository(1)->AddItem(&pItem, 1, 0, 1);
            }
        }
    }
    CScriptEvent* pEvent = new CScriptEvent();
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLOSE;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, objectId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    m_flags = m_flags & ~FLAG_INVENTORY_OPEN;
}

void CSWSItem::AddFeatRequirements(CExoString& sOut)
{
    CExoString sName;
    CExoString sLeft;
    CExoString sRight;
    int nStrRef;
    int nFeat = FEAT_ARMOUR_PROF_LIGHT;
    for (int i = 0; i < GetBaseItem()->m_nRequiredFeats; i++)
    {
        if (GetBaseItem()->GetRequiredFeat(static_cast<unsigned char>(i)) == FEAT_ARMOUR_PROF_MEDIUM &&
            nFeat == FEAT_ARMOUR_PROF_LIGHT)
            nFeat = FEAT_ARMOUR_PROF_MEDIUM;
        if (GetBaseItem()->GetRequiredFeat(static_cast<unsigned char>(i)) == FEAT_ARMOUR_PROF_HEAVY &&
            nFeat >= FEAT_ARMOUR_PROF_LIGHT && nFeat <= FEAT_ARMOUR_PROF_MEDIUM)
            nFeat = FEAT_ARMOUR_PROF_HEAVY;
    }
    int nCount = 0;
    for (int i = 0; i < GetBaseItem()->m_nRequiredFeats; i++)
    {
        unsigned short nFeatId = GetBaseItem()->GetRequiredFeat(static_cast<unsigned char>(i));
        if (nFeatId == 0 ||
            (nFeatId >= FEAT_ARMOUR_PROF_HEAVY && nFeatId <= FEAT_ARMOUR_PROF_MEDIUM && nFeatId != nFeat))
            continue;
        if (nCount == 0)
        {
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FEATS_REQUIRED) + ":\n";
            nCount = 1;
        }
        g_pRules->m_p2DArrays->m_pFeat->GetINTEntry(nFeatId, CExoString("NAME"), &nStrRef);
        sName = g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef);
        int nColon = sName.Find(':', 0);
        if (nColon != -1)
        {
            sLeft = sName.Left(nColon);
#if KOTOR1  // K1: CExoString::GetLength() instead of the raw string
            sRight = sName.Right(sName.GetLength() - nColon - 1);
#else
            sRight = sName.Right((sName.m_sString ? static_cast<int>(strlen(sName.m_sString)) : 0) - nColon - 1);
#endif
            sName = CExoString(sLeft) + " - " + CExoString(sRight);
        }
        sOut = sOut + sName + "\n";
    }
    if (GetPropertyByTypeExists(ITEM_PROPERTY_USE_LIMITATION_FEAT, 0))
    {
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (pProp->m_type != ITEM_PROPERTY_USE_LIMITATION_FEAT)
                continue;
            unsigned short nSubType = pProp->m_subType;
            if (nSubType == 0)
                continue;
            if (nCount == 0)
            {
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FEATS_REQUIRED) + ":\n";
                nCount = 1;
            }
            g_pRules->m_p2DArrays->m_pFeat->GetINTEntry(nSubType, CExoString("NAME"), &nStrRef);
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef);
            int nSubColon = sName.Find(':', 0);
            if (nSubColon != -1)
            {
                sLeft = sName.Left(nSubColon);
#if KOTOR1  // K1: CExoString::GetLength() instead of the raw string
                sRight = sName.Right(sName.GetLength() - nSubColon - 1);
#else
                sRight = sName.Right((sName.m_sString ? static_cast<int>(strlen(sName.m_sString)) : 0) - nSubColon - 1);
#endif
                sName = CExoString(sLeft) + " - " + CExoString(sRight);
            }
            sOut = sOut + sName + "\n";
        }
    }
    if (nCount != 0)
        sOut = sOut + "\n";
}

#if !KOTOR1
void CSWSItem::AddAttributeRequirements(CExoString& sOut)
{
    CExoString sLine;
    int nValue;
    if (!GetPropertyByTypeExists(ITEM_PROPERTY_USE_LIMITATION_ATTRIBUTE, 0))
        return;
    int nCount = 0;
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
            continue;
        if (pProp->m_type != ITEM_PROPERTY_USE_LIMITATION_ATTRIBUTE)
            continue;
        if (nCount == 0)
        {
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTRIBUTES_REQUIRED) + ":\n";
        }
        unsigned short nSubType = pProp->m_subType;
        C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_ATTRIBUTE);
        pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
        switch (nSubType)
        {
        case ABILITY_STRENGTH:
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MINIMUM_STRENGTH) + ": ";
            break;
        case ABILITY_DEXTERITY:
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MINIMUM_DEXTERITY) + ": ";
            break;
        case ABILITY_CONSTITUTION:
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MINIMUM_CONSTITUTION) + ": ";
            break;
        case ABILITY_INTELLIGENCE:
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MINIMUM_INTELLIGENCE) + ": ";
            break;
        case ABILITY_WISDOM:
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MINIMUM_WISDOM) + ": ";
            break;
        case ABILITY_CHARISMA:
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MINIMUM_CHARISMA) + ": ";
            break;
        }
        sLine.Format(const_cast<char*>("%d"), nValue);
        sOut = sOut + sLine + "\n";
        nCount = 1;
    }
    if (nCount != 0)
        sOut = sOut + "\n";
}
#endif

#if !KOTOR1
void CSWSItem::AddGenderRequirements(CExoString& sOut)
{
    CExoString sName;
    CExoString sUnused1;
    CExoString sUnused2;
    int nIndex;
    if (!GetPropertyByTypeExists(ITEM_PROPERTY_LIMIT_USE_BY_GENDER, 0))
        return;
    int nCount = 0;
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
            continue;
        if (pProp->m_type != ITEM_PROPERTY_LIMIT_USE_BY_GENDER)
            continue;
        unsigned short nSubType = pProp->m_subType;
        if (nCount == 0)
        {
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_GENDER_REQUIRED) + ":\n";
            nCount = 1;
        }
        g_pRules->m_p2DArrays->m_pGender->GetINTEntry(nSubType, CExoString("NAME"), &nIndex);
        sName = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
        sOut = sOut + sName + "\n";
    }
    if (nCount != 0)
        sOut = sOut + "\n";
}
#endif

void CSWSItem::AddDamageProperties(CExoString& sOut)
{
    CExoString sLine;
#if KOTOR1  // K1: header first, "0" for a 0x1f item, no clamp
    sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE) + ": ";
    if (GetPropertyByTypeExists(ITEM_PROPERTY_NO_DAMAGE, 0))
    {
        sOut = sOut + "0\n";
    }
    else
    {
        unsigned char nDice = g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nNumDice;
        unsigned char nDieSize =
            g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nDieToRoll;
        CExoString sDamageType;
        GetDamageTypeString(GetDamageFlags(), sDamageType);
        if (sDamageType != "")
            sOut = sOut + sDamageType + ", ";
        int nMin = nDice;
        int nMax = nDieSize * nDice;
        if (GetPropertyByTypeExists(ITEM_PROPERTY_DECREASED_DAMAGE, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_ENHANCEMENT_BONUS, 0) ||
            GetPropertyByTypeExists(ITEM_PROPERTY_DAMAGE_BONUS, 0))
        {
            for (int i = 0; i < m_nPassiveProperties; i++)
            {
                CSWItemProperty* pProp = &m_pPassiveProperties[i];
                if (pProp == 0)
                    continue;
                if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                    continue;
                if (pProp->m_type == ITEM_PROPERTY_DECREASED_DAMAGE)
                {
                    nMin -= pProp->m_costTableValue;
                    nMax -= pProp->m_costTableValue;
                }
                else if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS)
                {
                    if ((1 << pProp->m_subType) & GetDamageFlags())
                    {
                        int nAddMin, nAddMax;
                        GetPropertyDamageRange(pProp->m_costTableValue, nAddMin, nAddMax);
                        nMin += nAddMin;
                        nMax += nAddMax;
                    }
                }
                else if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS)
                {
                    nMin += pProp->m_costTableValue;
                    nMax += pProp->m_costTableValue;
                }
            }
        }
        sLine.Format(const_cast<char*>("%d-%d\n\n"), nMin, nMax);
        sOut = sOut + sLine.CStr();
    }
#else
    if (GetPropertyByTypeExists(ITEM_PROPERTY_NO_DAMAGE, 0))
    {
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE) + ": ";
        sOut = sOut + "1\n";
    }
    else
    {
        unsigned char nDice = g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nNumDice;
        unsigned char nDieSize =
            g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nDieToRoll;
        CExoString sDamageType;
        GetDamageTypeString(GetDamageFlags(), sDamageType);
        int nMin = nDice;
        int nMax = nDieSize * nDice;
        if (GetPropertyByTypeExists(ITEM_PROPERTY_DECREASED_DAMAGE, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_ENHANCEMENT_BONUS, 0) ||
            GetPropertyByTypeExists(ITEM_PROPERTY_DAMAGE_BONUS, 0))
        {
            for (int i = 0; i < m_nPassiveProperties; i++)
            {
                CSWItemProperty* pProp = &m_pPassiveProperties[i];
                if (pProp == 0)
                    continue;
                if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                    continue;
                if (pProp->m_type == ITEM_PROPERTY_DECREASED_DAMAGE)
                {
                    nMin -= pProp->m_costTableValue;
                    nMax -= pProp->m_costTableValue;
                }
                else if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS)
                {
                    if ((1 << pProp->m_subType) & GetDamageFlags())
                    {
                        int nAddMin, nAddMax;
                        GetPropertyDamageRange(pProp->m_costTableValue, nAddMin, nAddMax);
                        nMin += nAddMin;
                        nMax += nAddMax;
                    }
                }
                else if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS)
                {
                    nMin += pProp->m_costTableValue;
                    nMax += pProp->m_costTableValue;
                }
            }
        }
        if (nMin <= 1)
            nMin = 1;
        if (nMax != 0)
        {
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE) + ": ";
            if (m_baseItem == BASE_ITEM_GAUNTLETS)
            {
                if (sDamageType != "")
                    sOut = sOut + sDamageType + " ";
                sOut = sOut + "+" + CExoString(nMax) + "\n\n";
            }
            else
            {
                if (sDamageType != "")
                    sOut = sOut + sDamageType + ", ";
                sLine.Format(const_cast<char*>("%d-%d\n\n"), nMin, nMax);
                sOut = sOut + sLine.CStr();
            }
        }
    }
#endif
    CExoString sValue;
    CExoString s1, s2, s3, s4;
    CExoString sSpare;
    if (GetPropertyByTypeExists(ITEM_PROPERTY_DAMAGE_BONUS_VS_ALIGNMENT_GROUP, 0))
    {
        int nMin = 0;
        int nMax = 0;
        int nIndex = -1;
        int bFound = 0;
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS_VS_ALIGNMENT_GROUP)
            {
                int nAddMin, nAddMax;
                GetPropertyDamageRange(pProp->m_costTableValue, nAddMin, nAddMax);
                nIndex = i;
                nMin += nAddMin;
                nMax += nAddMax;
            }
            bFound = 1;
        }
        if (bFound != 0)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[nIndex];
            GetPropertyStrings(pProp->m_type, pProp->m_subType, PROPERTY_VALUE_NONE, pProp->m_nParam1Value,
                               s1, s2, s3, s4, 0);
            if (nMin == nMax)
                s3.Format(const_cast<char*>("+%d"), nMin);
            else
                s3.Format(const_cast<char*>("+%d-%d"), nMin, nMax);
            sOut = sOut + s4 + ": " + s3 + " " +
#if KOTOR1
                   g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_VS) + " " + s2 + "\n\n";
#else
                   g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_VS) + " " + s2 + "\n";
#endif
        }
    }
    if (GetPropertyByTypeExists(ITEM_PROPERTY_DAMAGE_BONUS_VS_RACIAL_GROUP, 0))
    {
        int nMin = 0;
        int nMax = 0;
        int nIndex = -1;
        int bFound = 0;
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS_VS_RACIAL_GROUP)
            {
                int nAddMin, nAddMax;
                GetPropertyDamageRange(pProp->m_costTableValue, nAddMin, nAddMax);
                nIndex = i;
                nMin += nAddMin;
                nMax += nAddMax;
            }
            bFound = 1;
        }
        if (bFound != 0)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[nIndex];
            GetPropertyStrings(pProp->m_type, pProp->m_subType, PROPERTY_VALUE_NONE, pProp->m_nParam1Value,
                               s1, s2, s3, s4, 0);
            if (nMin == nMax)
                s3.Format(const_cast<char*>("+%d"), nMin);
            else
                s3.Format(const_cast<char*>("+%d-%d"), nMin, nMax);
            sOut = sOut + s4 + ": " + s3 + " " +
                   g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_VS) + " " + s2 + "\n\n";
        }
    }
    if (GetPropertyByTypeExists(ITEM_PROPERTY_DAMAGE_REDUCTION, 0))
    {
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            CExoString d1, d2, d3, d4, d5;
            if (pProp->m_type == ITEM_PROPERTY_DAMAGE_REDUCTION)
            {
                unsigned short nSubType = pProp->m_subType;
                unsigned short nValue = pProp->m_costTableValue;
                if (GetExistsHigherValuedProperty(ITEM_PROPERTY_DAMAGE_REDUCTION, nSubType, nValue) == 0)
                {
                    GetPropertyStrings(ITEM_PROPERTY_DAMAGE_REDUCTION, nSubType, nValue, PROPERTY_PARAM_NONE, d1, d2, d3, d4, 0);
                    char szName[64];
                    int nAmount;
                    static_cast<void>(sscanf(d2.CStr(), "%s +%d", szName, &nAmount));
                    sValue.Format(const_cast<char*>("%d"), nAmount);
                    sOut = sOut + d1 + ": +" + sValue;
                    sValue.Format(const_cast<char*>("%s"), d3.CStr());
                    sOut = sOut + " (" + d3 + ")\n\n";
                }
            }
        }
    }
    if (GetPropertyByTypeExists(ITEM_PROPERTY_DAMAGE_BONUS, 0))
    {
        int nMin = 0;
        int nMax = 0;
        int bFound = 0;
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            CExoString t1, t2, t3, t4, t5;
            if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS)
            {
                if (((1 << pProp->m_subType) & GetDamageFlags()) == 0)
                {
                    GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue,
                                       PROPERTY_PARAM_NONE, t1, t2, t3, t4, 0);
                    if (bFound == 0)
                        sOut = sOut + t1 + ": ";
                    for (int j = 0; j < m_nPassiveProperties; j++)
                    {
                        CSWItemProperty* pOther = &m_pPassiveProperties[j];
                        if (pOther == 0)
                            continue;
                        if (pOther->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE &&
                            (m_nUpgrades & (1 << pOther->m_nUpgradeType)) == 0)
                            continue;
                        if (pOther->m_type != ITEM_PROPERTY_DAMAGE_BONUS)
                            continue;
                        if (pOther->m_subType != pProp->m_subType &&
                            (pProp->m_subType > IPRP_DAMAGETYPE_SLASHING ||
                             pOther->m_subType > IPRP_DAMAGETYPE_SLASHING))
                            continue;
                        if (j < i)
                            goto SkipSection;
                        int nAddMin, nAddMax;
                        GetPropertyDamageRange(pOther->m_costTableValue, nAddMin, nAddMax);
                        nMin += nAddMin;
                        if (nAddMin == nAddMax)
                            nMax += nAddMin;
                        else
                            nMax += nAddMax;
                    }
                    if (nMin == nMax)
                        sValue.Format(const_cast<char*>("+%d"), nMin);
                    else
                        sValue.Format(const_cast<char*>("+%d-%d"), nMin, nMax);
                    if (pProp->m_subType <= IPRP_DAMAGETYPE_SLASHING)
                        t2 = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_PHYSICAL);
                    if (bFound != 0)
                    {
                        sOut = sOut + ", " + sValue + " " + t2;
                        nMin = 0;
                        nMax = 0;
                    }
                    else
                    {
                        sOut = sOut + sValue + " " + t2;
                        nMin = 0;
                        nMax = 0;
                        bFound = 1;
                    }
                }
            }
SkipSection:
            ;
        }
        if (bFound != 0)
            sOut = sOut + "\n\n";
    }
}

void CSWSItem::AddWeaponSizeProperties(CExoString& sOut)
{
    if (GetBaseItem()->m_nWeaponSize == CSWBaseItem::WEAPON_SIZE_SMALL)
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BALANCED) + "\n\n";
}

#if !KOTOR1
void CSWSItem::AddSubraceRequirements(CExoString& sOut)
{
    CExoString sName;
    CExoString sUnused1;
    CExoString sUnused2;
    int nIndex;
    if (!GetPropertyByTypeExists(ITEM_PROPERTY_LIMIT_USE_BY_SUBRACE, 0))
        return;
    int nCount = 0;
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
            continue;
        if (pProp->m_type != ITEM_PROPERTY_LIMIT_USE_BY_SUBRACE)
            continue;
        unsigned short nSubType = pProp->m_subType;
        if (nCount == 0)
        {
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_RACE_REQUIRED) + ":\n";
            nCount = 1;
        }
        g_pRules->m_p2DArrays->m_pSubrace->GetINTEntry(nSubType, CExoString("NAME"), &nIndex);
        sName = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
        sOut = sOut + sName + "\n";
    }
    if (nCount != 0)
        sOut = sOut + "\n";
}
#endif

#if !KOTOR1
void CSWSItem::AddCNPCRequirements(CExoString& sOut)
{
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable == 0)
        return;
    if (pPartyTable->GetInfluence(1) == -1)
        return;
    if (m_baseItem != BASE_ITEM_JEDI_ROBE && m_baseItem != BASE_ITEM_JEDI_KNIGHT_ROBE && m_baseItem != BASE_ITEM_JEDI_MASTER_ROBE && m_baseItem != BASE_ITEM_SONIC_GRENADE_TYPE2 &&
        m_baseItem != BASE_ITEM_ARMORED_FLIGHT_SUIT && m_baseItem != BASE_ITEM_RESTRICTIVE_JEDI_ROBE && m_baseItem != BASE_ITEM_ZEISON_SHA_ARMOR)
        return;
    sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CANNOT_EQUIP) + ":\n";
    sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BAO_DUR) + "\n";
    sOut = sOut + "\n";
}
#endif

#if !KOTOR1
void CSWSItem::AddPCRequirements(CExoString& sOut)
{
    CExoString sName;
    CExoString sUnused1;
    CExoString sUnused2;
    int nIndex;
    if (!GetPropertyByTypeExists(ITEM_PROPERTY_LIMIT_USE_BY_PC, 0))
        return;
    int nCount = 0;
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
            continue;
        if (pProp->m_type != ITEM_PROPERTY_LIMIT_USE_BY_PC)
            continue;
        unsigned short nSubType = pProp->m_subType;
        if (nCount == 0)
        {
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_USEABLE_BY) + ":\n";
            nCount = 1;
        }
        if (nSubType != 0)
        {
            g_pRules->m_p2DArrays->m_pIPRPPC->GetINTEntry(nSubType, CExoString("NAME"), &nIndex);
            sName = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
        }
        else
        {
            CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
            if (pPartyTable != 0)
                sName = CExoString(pPartyTable->m_saveTableName);
        }
        sOut = sOut + sName + "\n";
    }
    if (nCount != 0)
        sOut = sOut + "\n";
}
#endif

void CSWSItem::GetPropertyDamageRange(unsigned short nSubType, int& nMin, int& nMax)
{
    int nDice = 0;
    int nDie = 0;
    if (g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(nSubType, CExoString("NumDice"), &nDice) == 1)
    {
        if (nDice != 0)
        {
            g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(nSubType, CExoString("Die"), &nDie);
            nMin = nDice;
            nMax = nDice * nDie;
        }
        else
        {
            nMin = 1;
            nMax = nSubType;
        }
    }
    else
    {
        nMin = nSubType;
        nMax = nSubType;
    }
}

void CSWSItem::AddCriticalThreatProperties(CExoString& sOut)
{
    CExoString sLine;
    int nMin;
    int nMax;
    sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CRITICAL_THREAT) + ":\n";
    int nCritThreat;
    int nCritMult;
    nCritThreat = GetBaseItem()->m_nCritThreat;
    nCritMult = GetBaseItem()->m_nCritMultiplier;
    // The Keen property doubles the threat range.
    if (GetPropertyByTypeExists(ITEM_PROPERTY_KEEN, 0))
        nCritThreat *= 2;
#if KOTOR1
    int nDamageMax = 0;
    int nDamageMin = 0;
#else
    int nDamageMin = 0;
    int nDamageMax = 0;
#endif
    if (GetPropertyByTypeExists(ITEM_PROPERTY_MASSIVE_CRITICALS, 0))
    {
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (pProp->m_type != ITEM_PROPERTY_MASSIVE_CRITICALS)
                continue;
#if KOTOR1  // K1 counts only the strongest 0x31 property per subtype
            unsigned short nValue = pProp->m_costTableValue;
            unsigned short nSubType = pProp->m_subType;
            bool bBest = true;
            for (int64_t j = 0; j < m_nPassiveProperties; j++)
            {
                CSWItemProperty* pOther = &m_pPassiveProperties[j];
                if (pOther != 0 &&
                    (pOther->m_nUpgradeType == CSWItemProperty::UPGRADE_TYPE_NONE || (m_nUpgrades & (1 << pOther->m_nUpgradeType)) != 0) &&
                    pOther->m_type == ITEM_PROPERTY_MASSIVE_CRITICALS && pOther->m_subType == nSubType &&
                    pOther->m_costTableValue > nValue)
                {
                    bBest = false;
                    break;
                }
            }
            if (!bBest)
                continue;
            GetPropertyDamageRange(nValue, nMin, nMax);
#else
            GetPropertyDamageRange(pProp->m_costTableValue, nMin, nMax);
#endif
            nDamageMin += nMin;
            nDamageMax += nMax;
        }
    }
    if (nDamageMin != 0 || nDamageMax != 0)
        sLine.Format(const_cast<char*>("%d-20,x%d +%d-%d\n"), 21 - nCritThreat, nCritMult, nDamageMin, nDamageMax);
    else
        sLine.Format(const_cast<char*>("%d-20,x%d\n"), 21 - nCritThreat, nCritMult);
    sOut = sOut + sLine + "\n";
}

void CSWSItem::AddAttackModifierProperties(CExoString& sOut)
{
    CExoString sLine;
    int nBonus = 0;
    if (GetPropertyByTypeExists(ITEM_PROPERTY_ATTACK_BONUS, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_ATTACK_PENALTY, 0) ||
        GetPropertyByTypeExists(ITEM_PROPERTY_DECREASED_ATTACK_MODIFIER, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_ENHANCEMENT_BONUS, 0))
    {
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (pProp->m_type == ITEM_PROPERTY_ATTACK_BONUS || pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS)
                nBonus += pProp->m_costTableValue;
            if (pProp->m_type == ITEM_PROPERTY_DECREASED_ATTACK_MODIFIER || pProp->m_type == ITEM_PROPERTY_ATTACK_PENALTY)
                nBonus -= pProp->m_costTableValue;
        }
    }
#if KOTOR1  // K1: no base-item suffix, and a negative total prints with a doubled minus
    if (nBonus < 0)
    {
        sLine.Format(const_cast<char*>("%s: -%d\n"), g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK_MODIFIER).CStr(),
                     nBonus);
        sOut = sOut + sLine;
    }
    else if (nBonus > 0)
    {
        sLine.Format(const_cast<char*>("%s: +%d\n"), g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK_MODIFIER).CStr(),
                     nBonus);
        sOut = sOut + sLine;
    }
#else
    CExoString sSuffix = g_pSWTlkTable->GetSimpleString(STRREF_WHEN_UNARMED);
    if (nBonus < 0)
    {
        if (m_baseItem == BASE_ITEM_GAUNTLETS)
            sLine.Format(const_cast<char*>("%s: %d%s\n"), g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK_MODIFIER).CStr(),
                         nBonus, sSuffix.CStr());
        else
            sLine.Format(const_cast<char*>("%s: %d\n"), g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK_MODIFIER).CStr(),
                         nBonus);
        sOut = sOut + sLine;
    }
    else if (nBonus > 0)
    {
        if (m_baseItem == BASE_ITEM_GAUNTLETS)
            sLine.Format(const_cast<char*>("%s: +%d%s\n"), g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK_MODIFIER).CStr(),
                         nBonus, sSuffix.CStr());
        else
            sLine.Format(const_cast<char*>("%s: +%d\n"), g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK_MODIFIER).CStr(),
                         nBonus);
        sOut = sOut + sLine;
    }
#endif
    if (GetPropertyByTypeExists(ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_RACIAL_GROUP, 0) ||
        GetPropertyByTypeExists(ITEM_PROPERTY_ATTACK_BONUS_VS_ALIGNMENT_GROUP, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_ATTACK_BONUS_VS_RACIAL_GROUP, 0))
    {
        if (nBonus == 0)
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK_MODIFIER) + ": ";
        AddAttackBonusStrings(ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP, sOut);
        AddAttackBonusStrings(ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_RACIAL_GROUP, sOut);
        AddAttackBonusStrings(ITEM_PROPERTY_ATTACK_BONUS_VS_ALIGNMENT_GROUP, sOut);
        AddAttackBonusStrings(ITEM_PROPERTY_ATTACK_BONUS_VS_RACIAL_GROUP, sOut);
    }
    sOut = sOut + "\n";
}

void CSWSItem::AddDefenceProperties(CExoString& sOut)
{
    CExoString sLine;
    CSWItemProperty* pProp;
    int nValue = GetBaseItem()->m_nBaseAC;
    if (GetPropertyByTypeExists(ITEM_PROPERTY_AC_BONUS, 0))
    {
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (pProp->m_type == ITEM_PROPERTY_AC_BONUS)
            {
                if (GetBaseItem()->m_nACEnchant == 0 ||
                    GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                                  pProp->m_costTableValue) == 0)
                    nValue += pProp->m_costTableValue;
            }
        }
    }
    if (GetPropertyByTypeExists(ITEM_PROPERTY_DECREASED_AC, 0))
    {
        GetPropertyByType(&pProp, ITEM_PROPERTY_DECREASED_AC, 0);
        if (pProp != 0 &&
            (pProp->m_nUpgradeType == CSWItemProperty::UPGRADE_TYPE_NONE || (m_nUpgrades & (1 << pProp->m_nUpgradeType)) != 0) &&
            GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                          pProp->m_costTableValue) == 0)
            nValue -= pProp->m_costTableValue;
    }
    if (nValue != 0)
    {
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DEFENSE_BONUS) + ": ";
        sLine.Format(const_cast<char*>("%d\n"), nValue);
        sOut = sOut + sLine + "\n";
    }
    if (GetBaseItem()->m_nItemType != ITEM_TYPE_DROID_PLATING)
    {
        int nMaxDex = GetBaseItem()->m_nMaxDexBonus;
        int nSum = 0;
        if (GetPropertyByTypeExists(ITEM_PROPERTY_MAX_DEX_BONUS, 0))
        {
            for (int i = 0; i < m_nPassiveProperties; i++)
            {
                pProp = &m_pPassiveProperties[i];
                if (pProp != 0 &&
                    (pProp->m_nUpgradeType == CSWItemProperty::UPGRADE_TYPE_NONE || (m_nUpgrades & (1 << pProp->m_nUpgradeType)) != 0))
                {
                    if (pProp->m_type == ITEM_PROPERTY_MAX_DEX_BONUS)
                        nSum += pProp->m_costTableValue;
                }
            }
        }
        unsigned char nTotal = static_cast<unsigned char>((nSum != 0 && nMaxDex == MAX_DEX_BONUS_NONE) ? nSum : nSum + nMaxDex);
        if (nTotal != MAX_DEX_BONUS_NONE)
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MAX_DEXTERITY_BONUS) + ": +" +
                   CExoString(nTotal) + "\n\n";
    }
    CExoString s1, s2, s3, s4, s5;
    if (GetPropertyByTypeExists(ITEM_PROPERTY_AC_BONUS_VS_ALIGNMENT_GROUP, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_AC_BONUS_VS_DAMAGE_TYPE, 0) ||
        GetPropertyByTypeExists(ITEM_PROPERTY_AC_BONUS_VS_RACIAL_GROUP, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_IMMUNITY_DAMAGE_TYPE, 0) ||
        GetPropertyByTypeExists(ITEM_PROPERTY_DAMAGE_RESISTANCE, 0))
    {
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (pProp->m_type >= 2 && pProp->m_type <= 4)
            {
                if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                                  pProp->m_costTableValue) == 0)
                {
                    GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue,
                                       PROPERTY_PARAM_NONE, s1, s2, s3, s4, 0);
                    sOut = sOut + "+" + s3 + " " +
                           g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_VS) + " " + s2 + "\n\n";
                }
            }
            if (pProp->m_type == ITEM_PROPERTY_IMMUNITY_DAMAGE_TYPE || pProp->m_type == ITEM_PROPERTY_DAMAGE_RESISTANCE)
            {
                if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                                  pProp->m_costTableValue) == 0)
                {
                    GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue,
                                       PROPERTY_PARAM_NONE, s1, s2, s3, s4, 0);
                    sOut = sOut + s1 + ": " + s3 + " " +
                           g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_VS) + " " + s2 + "\n\n";
                }
            }
        }
    }
}

void CSWSItem::AddMiscellaneousProperties(CExoString& sOut)
{
    CExoString s1, s2, s3, s4;
    CExoString sUnused;
    CExoString sLine;
    char szName[32];
    int nAmount;
    if (GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0))
    {
        int nCount = 0;
        for (int i = 0; i < m_nActiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pActiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (pProp->m_type != ITEM_PROPERTY_ACTIVATE_ITEM)
                continue;
            switch (pProp->m_costTableValue)
            {
            case IPRP_CHARGECOST_SINGLE_USE:
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SINGLE_USE) + "\n\n";
                continue;
            case IPRP_CHARGECOST_5_CHARGES_PER_USE: nCount = m_nMaxCharges / 5; break;
            case IPRP_CHARGECOST_4_CHARGES_PER_USE: nCount = m_nMaxCharges / 4; break;
            case IPRP_CHARGECOST_3_CHARGES_PER_USE: nCount = m_nMaxCharges / 3; break;
            case IPRP_CHARGECOST_2_CHARGES_PER_USE:
            case IPRP_CHARGECOST_1_CHARGE_PER_USE: nCount = m_nMaxCharges; break;
            case IPRP_CHARGECOST_0_CHARGES_PER_USE: nCount = 0; break;
            case IPRP_CHARGECOST_1_USE_PER_DAY: nCount = 1; break;
            case IPRP_CHARGECOST_2_USES_PER_DAY: nCount = 2; break;
            case IPRP_CHARGECOST_3_USES_PER_DAY: nCount = 3; break;
            case IPRP_CHARGECOST_4_USES_PER_DAY: nCount = 4; break;
            case IPRP_CHARGECOST_5_USES_PER_DAY: nCount = 5; break;
            case IPRP_CHARGECOST_UNLIMITED_USE: nCount = 0; break;
            case IPRP_CHARGECOST_1_PER_MINUTE:
            case IPRP_CHARGECOST_1_PER_2_MINUTES:
            case IPRP_CHARGECOST_1_PER_3_MINUTES:
            case IPRP_CHARGECOST_1_PER_4_MINUTES:
            case IPRP_CHARGECOST_1_PER_5_MINUTES: nCount = 1; break;
            }
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_USES) + ": ";
            if (nCount != 0)
            {
                sLine.Format(const_cast<char*>("%d/%d\n"), GetUsedActivePropertyUsesLeft(static_cast<unsigned char>(i)), nCount);
                sOut = sOut + sLine + "\n";
            }
            else
            {
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_UNLIMITED) + "\n\n";
            }
        }
    }
    if (GetPropertyByTypeExists(ITEM_PROPERTY_IMMUNITY, 0))
    {
        int nLatch = 0;
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (pProp->m_type != ITEM_PROPERTY_IMMUNITY)
                continue;
            GetPropertyStrings(pProp->m_type, pProp->m_subType, PROPERTY_VALUE_NONE, PROPERTY_PARAM_NONE, s1, s2, s3, s4, 0);
            if (nLatch == 0)
                sOut = sOut + s1 + ": ";
            if (nLatch > 0)
                sOut = sOut + ", ";
            sOut = sOut + s2;
            nLatch++;
        }
        sOut = sOut + "\n\n";
    }
    if (GetPropertyByTypeExists(ITEM_PROPERTY_IMPROVED_SAVING_THROW, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_DECREASED_SAVING_THROWS, 0) ||
        GetPropertyByTypeExists(ITEM_PROPERTY_IMPROVED_SAVING_THROW_SPECIFIC, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_DECREASED_SAVING_THROWS_SPECIFIC, 0))
    {
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SAVES) + ": ";
        int nLatch = 0;
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                             pProp->m_costTableValue) != 0)
                continue;
            GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE, s1,
                               s2, s3, s4, 0);
            switch (pProp->m_type)
            {
            case ITEM_PROPERTY_DECREASED_SAVING_THROWS:
            case ITEM_PROPERTY_DECREASED_SAVING_THROWS_SPECIFIC:
                if (nLatch > 0)
                    sOut = sOut + ", ";
                static_cast<void>(sscanf(s3.CStr(), "%s -%d", szName, &nAmount));
                sLine.Format(const_cast<char*>("%d"), nAmount);
                sOut = sOut + s2 + " -" + sLine;
                nLatch++;
                break;
            case ITEM_PROPERTY_IMPROVED_SAVING_THROW:
            case ITEM_PROPERTY_IMPROVED_SAVING_THROW_SPECIFIC:
                if (nLatch > 0)
                    sOut = sOut + ", ";
                sOut = sOut + s2 + " +" + s3;
                nLatch++;
                break;
            }
        }
        sOut = sOut + "\n\n";
    }
    if (GetPropertyByTypeExists(ITEM_PROPERTY_SKILL_BONUS, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_DECREASED_SKILL_MODIFIER, 0))
    {
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SKILLS) + ": ";
        int nLatch = 0;
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp == 0)
                continue;
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                             pProp->m_costTableValue) != 0)
                continue;
            GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE, s1,
                               s2, s3, s4, 0);
            if (pProp->m_type == ITEM_PROPERTY_DECREASED_SKILL_MODIFIER)
            {
                if (GetExistsHigherValuedProperty(ITEM_PROPERTY_DECREASED_SKILL_MODIFIER, pProp->m_subType,
                                                  pProp->m_costTableValue) != 0)
                    continue;
                if (nLatch > 0)
                    sOut = sOut + ", ";
                GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE,
                                   s1, s2, s3, s4, 0);
#if KOTOR1  // K1 re-renders the value as "-N"
                static_cast<void>(sscanf(s3.CStr(), "%s -%d", szName, &nAmount));
                sLine.Format(const_cast<char*>("-%d"), nAmount);
                sOut = sOut + s2 + " " + sLine;
#else
                sOut = sOut + s2 + " " + s3;
#endif
                nLatch++;
            }
            else if (pProp->m_type == ITEM_PROPERTY_SKILL_BONUS)
            {
                if (GetExistsHigherValuedProperty(ITEM_PROPERTY_SKILL_BONUS, pProp->m_subType,
                                                  pProp->m_costTableValue) != 0)
                    continue;
                if (nLatch > 0)
                    sOut = sOut + ", ";
                GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE,
                                   s1, s2, s3, s4, 0);
                sOut = sOut + s2 + " " + s3;
                nLatch++;
            }
        }
        sOut = sOut + "\n\n";
    }
#if !KOTOR1  // K1 renders 0x37 / 0x38 one property at a time in the switch below
    if (GetPropertyByTypeExists(ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_INCREASE, 0) || GetPropertyByTypeExists(ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_DECREASE, 0))
    {
        int nSum = 0;
        int bFirst = 0;
        for (int i = 0; i < m_nPassiveProperties; i++)
        {
            CSWItemProperty* pProp = &m_pPassiveProperties[i];
            if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
                continue;
            if (static_cast<unsigned short>(pProp->m_type - ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_INCREASE) > 1)
                continue;
            GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE, s1,
                               s2, s3, s4, 0);
            if (bFirst == 0)
            {
                CExoString sKey;
                CExoString sKeep;
                int nPos = s1.Find(':', 0);
                if (nPos != -1)
                {
                    sKey = s1.Left(nPos);
                    s1 = CExoString(sKey) + ": ";
                    sOut = sOut + s1 + " ";
                }
                bFirst = 1;
            }
            switch (pProp->m_type)
            {
            case ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_INCREASE: nSum += pProp->m_costTableValue; break;
            case ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_DECREASE: nSum -= pProp->m_costTableValue; break;
            }
        }
        if (nSum > 0)
            sOut = sOut + "+";
        sOut = sOut + CExoString(nSum) + "\n\n";
    }
#endif
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp == 0)
            continue;
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
            continue;
        switch (pProp->m_type)
        {
        case ITEM_PROPERTY_ABILITY_BONUS:
            if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                             pProp->m_costTableValue) == 0)
            {
                GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE,
                                   s1, s2, s3, s4, 0);
                sOut = sOut + s2 + ": " + s3 + "\n\n";
            }
            break;
        case ITEM_PROPERTY_BONUS_FEAT:
            GetPropertyStrings(pProp->m_type, pProp->m_subType, PROPERTY_VALUE_NONE, PROPERTY_PARAM_NONE, s1, s2, s3, s4, 0);
            sOut = sOut + s1 + ": " + s2 + "\n\n";
            break;
        case ITEM_PROPERTY_DAMAGE_VULNERABILITY:
            if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                             pProp->m_costTableValue) == 0)
            {
                int nChance;
                int nDuration;
                int nValue;
                GetPropertyStrings(pProp->m_type, pProp->m_subType, PROPERTY_VALUE_NONE, PROPERTY_PARAM_NONE, s1, s2, s3, s4,
                                   0);
                GetPropertyChanceDurationDC(pProp->m_costTableValue, pProp->m_nParam1Value, nChance,
                                            nDuration, nValue);
                nDuration *= 3;
                sLine.Format(const_cast<char*>("%d%% %s, %d%s "), nChance,
                             g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CHANCE).CStr(), nDuration,
                             g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SECONDS).CStr());
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_VULNERABILITY) + ": " + sLine +
                       g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_VS) + " " + s3 + "\n\n";
            }
            break;
        case ITEM_PROPERTY_DECREASED_ABILITY_SCORE:
            if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                             pProp->m_costTableValue) == 0)
            {
                GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE,
                                   s1, s2, s3, s4, 0);
#if KOTOR1  // K1 re-renders the value as "-N"
                static_cast<void>(sscanf(s3.CStr(), "%s -%d", szName, &nAmount));
                sLine.Format(const_cast<char*>("-%d"), nAmount);
                sOut = sOut + s2 + ": " + sLine + "\n\n";
#else
                sOut = sOut + s2 + ": " + s3 + "\n\n";
#endif
            }
            break;
        case ITEM_PROPERTY_IMPROVED_FORCE_RESISTANCE:
            if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                             pProp->m_costTableValue) == 0)
            {
                GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE,
                                   s1, s2, s3, s4, 0);
                static_cast<void>(sscanf(s3.CStr(), "%s %d", szName, &nAmount));
                sLine.Format(const_cast<char*>("%d"), nAmount);
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FORCE_RESIST) + ": +" + sLine +
                       "\n\n";
            }
            break;
        case ITEM_PROPERTY_LIGHT:
        case ITEM_PROPERTY_TRUE_SEEING:
        case ITEM_PROPERTY_FREEDOM_OF_MOVEMENT:
            GetPropertyStrings(pProp->m_type, PROPERTY_SUBTYPE_NONE, PROPERTY_VALUE_NONE, PROPERTY_PARAM_NONE, s1, s2, s3, s4, 0);
            sOut = sOut + s1 + "\n\n";
            break;
        case ITEM_PROPERTY_REGENERATION:
        case ITEM_PROPERTY_REGENERATION_FORCE_POINTS:
            if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                             pProp->m_costTableValue) == 0)
            {
                GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE,
                                   s1, s2, s3, s4, 0);
                sOut = sOut + s1 + ": " + s2 + "\n\n";
            }
            break;
        case ITEM_PROPERTY_USE_LIMITATION_ALIGNMENT_GROUP:
        case ITEM_PROPERTY_USE_LIMITATION_CLASS:
            GetPropertyStrings(pProp->m_type, pProp->m_subType, PROPERTY_VALUE_NONE, PROPERTY_PARAM_NONE, s1, s2, s3, s4, 0);
            sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_RESTRICTED_TO) + ": " + s2 + "\n\n";
            break;
#if KOTOR1
        case ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_INCREASE:
            if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                             pProp->m_costTableValue) == 0)
            {
                GetPropertyStrings(ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_INCREASE, PROPERTY_SUBTYPE_NONE, pProp->m_costTableValue, PROPERTY_PARAM_NONE, s1, s2, s3, s4, 0);
                sLine.Format(const_cast<char*>("%d"), pProp->m_costTableValue);
                CExoString sKey;
                CExoString sRest;
                int nPos = s1.Find(':', 0);
                if (nPos != -1)
                {
                    sKey = s1.Left(nPos);
                    sRest = s1.Right(s1.GetLength() - nPos - 1);
                    s1 = CExoString(sKey) + ": ";
                }
                sOut = sOut + s1 + "+" + sLine + "\n\n";
            }
            break;
        case ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_DECREASE:
            if (GetExistsHigherValuedProperty(pProp->m_type, pProp->m_subType,
                                             pProp->m_costTableValue) == 0)
            {
                GetPropertyStrings(ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_DECREASE, PROPERTY_SUBTYPE_NONE, pProp->m_costTableValue, PROPERTY_PARAM_NONE, s1, s2, s3, s4, 0);
                CExoString sKey;
                CExoString sRest;
                int nPos = s1.Find(':', 0);
                if (nPos != -1)
                {
                    sKey = s1.Left(nPos);
                    sRest = s1.Right(s1.GetLength() - nPos - 1);
                    s1 = CExoString(sKey) + ": ";
                }
                static_cast<void>(sscanf(s3.CStr(), "%s -%d", szName, &nAmount));
                sLine.Format(const_cast<char*>("%d"), nAmount);
                sOut = sOut + s1 + " -" + sLine + "\n\n";
            }
            break;
#endif
        }
    }
}

void CSWSItem::AddRangeProperties(CExoString& sOut)
{
    CExoString sLine;
    if (GetBaseItem()->m_bRangedWeapon != 0)
    {
        int nRange = static_cast<int>(GetBaseItem()->m_fMaxRange);
        sLine.Format(const_cast<char*>("%s: %dm\n"), g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_RANGE).CStr(),
                     nRange);
        sOut = sOut + sLine + "\n";
    }
}

void CSWSItem::GetPropertyChanceDurationDC(unsigned short nCostTableValue, unsigned char nParam1,
                                           int& nChance, int& nDuration, int& nValue)
{
    int nResult;
    g_pRules->m_p2DArrays->m_pOnHitDuration->GetINTEntry(nParam1, CExoString("EffectChance"),
                                                         &nResult);
    nChance = nResult;
    g_pRules->m_p2DArrays->m_pOnHitDuration->GetINTEntry(nParam1, CExoString("DurationRounds"),
                                                         &nResult);
    nDuration = nResult;
    g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_ON_HIT_DC)->GetINTEntry(nCostTableValue, CExoString("Value"),
                                                               &nResult);
    nValue = nResult;
}

void CSWSItem::GetDamageTypeString(unsigned int nDamageFlags, CExoString& sOut)
{
    int nCount = 0;
    if (nDamageFlags & (DAMAGE_TYPE_BLUDGEONING | DAMAGE_TYPE_PIERCING | DAMAGE_TYPE_SLASHING))
    {
        sOut = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_PHYSICAL);
        nCount = 1;
    }
    if (nDamageFlags & DAMAGE_TYPE_UNIVERSAL)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_UNIVERSAL);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_ACID)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_ACID);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_COLD)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_COLD);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_LIGHT_SIDE)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_LIGHT_SIDE);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_ELECTRICAL)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_ELECTRICAL);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_FIRE)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_FIRE);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_DARK_SIDE)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_DARK_SIDE);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_SONIC)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_SONIC);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_ION)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_ION);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_BLASTER)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_ENERGY);
        nCount++;
    }
    if (nDamageFlags & DAMAGE_TYPE_POISON)
    {
        if (nCount > 0)
            sOut = sOut + ", ";
        sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_POISON);
    }
}

CExoString CSWSItem::GetKeyedPropertyString(unsigned char nType)
{
    CExoArrayList<CExoString> aStrings;
    for (int i = 0; i < m_nActiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pActiveProperties[i];
        if (pProp == 0)
            continue;
        if (pProp->m_nUpgradeType != nType)
            continue;
        if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS)
        {
            CExoString s;
            if (pProp->m_costTableValue != 0)
                s = s + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK) + " +" +
                    CExoString(pProp->m_costTableValue) + ", " +
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE) + " +" +
                    CExoString(pProp->m_costTableValue);
            aStrings.Add(s);
        }
        else
        {
            AddProperty(pProp->m_type, pProp->m_subType, pProp->m_costTableValue,
                        pProp->m_nParam1Value, aStrings);
        }
    }
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp == 0)
            continue;
        if (pProp->m_nUpgradeType != nType)
            continue;
        if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS)
        {
            CExoString s;
            if (pProp->m_costTableValue != 0)
                s = s + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK) + " +" +
                    CExoString(pProp->m_costTableValue) + ", " +
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE) + " +" +
                    CExoString(pProp->m_costTableValue);
            aStrings.Add(s);
        }
        else
        {
            AddProperty(pProp->m_type, pProp->m_subType, pProp->m_costTableValue,
                        pProp->m_nParam1Value, aStrings);
        }
    }
    CExoString sResult;
    GetSortedPropertyStrings(aStrings, sResult);
    return sResult;
}

void CSWSItem::GetSortedPropertyStrings(CExoArrayList<CExoString>& list, CExoString& sOut)
{
    if (list.m_count == 0)
        return;
    sOut = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SPECIAL_PROPERTIES) + ":\n";
    while (list.m_count != 0)
    {
        int nMinIndex = 0;
        for (int i = 1; i < list.m_count; i++)
            if (list.m_pArray[i] < list.m_pArray[nMinIndex])
                nMinIndex = i;
        sOut = sOut + list.m_pArray[nMinIndex] + "\n";
        list.DelIndex(nMinIndex);
    }
}

void CSWSItem::AddProperty(unsigned short nType, unsigned short nSubType, unsigned short nValue,
                           unsigned char nParam, CExoArrayList<CExoString>& list)
{
    CExoString sName;
    CExoString sSubTypeResRef;
    int nCostTableValue = 0;
    int nParam1Table = -1;
    int nIndex = -1;
    if (g_pRules->m_p2DArrays->m_pItemPropDef->GetINTEntry(nType, CExoString("Name"), &nIndex) == 0)
        return;
    int nHasSubType = g_pRules->m_p2DArrays->m_pItemPropDef->GetCExoStringEntry(
        nType, CExoString("SubTypeResRef"), &sSubTypeResRef);
    g_pRules->m_p2DArrays->m_pItemPropDef->GetINTEntry(nType, CExoString("CostTableResRef"),
                                                       &nCostTableValue);
    int nHasParam1 = g_pRules->m_p2DArrays->m_pItemPropDef->GetINTEntry(
        nType, CExoString("Param1ResRef"), &nParam1Table);
    sName = g_pAppManager->m_pClientExoApp->GetGUIString(nIndex);
    if ((nCostTableValue | nHasSubType | nHasParam1) != 0)
        sName = sName + ":";
    if (nHasSubType != 0)
    {
        C2DA* pSubTypeTable = new C2DA(CResRef(sSubTypeResRef), 0);
        if (!pSubTypeTable->Load2DArray())
            return;
        int nSubTypeName = -1;
        pSubTypeTable->GetINTEntry(nSubType, CExoString("Name"), &nSubTypeName);
        // Both inner lookups share one out-parameter, and each read is guarded by its own call's
        // success, so a miss never leaves the previous call's value behind.
        int nSubValue;
        if (pSubTypeTable->GetINTEntry(nSubType, CExoString("CostTableResRef"), &nSubValue) != 0
            && nSubValue != 0)
            nCostTableValue = nSubValue;
        if (pSubTypeTable->GetINTEntry(nSubType, CExoString("Param1ResRef"), &nSubValue) != 0)
        {
            nParam1Table = nSubValue;
            nHasParam1 = 1;
        }
        delete pSubTypeTable;
        if (nType == ITEM_PROPERTY_DAMAGE_BONUS && nSubType <= IPRP_DAMAGETYPE_SLASHING)
            sName = sName + " " + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DAMAGE_PHYSICAL);
        else
            sName = sName + " " + g_pAppManager->m_pClientExoApp->GetGUIString(nSubTypeName);
        if (nCostTableValue != 0 || nHasParam1 != 0)
            sName = sName + ",";
    }
    if (nCostTableValue != 0)
    {
        C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(static_cast<unsigned char>(nCostTableValue));
        int nCostTableName = -1;
        pCostTable->GetINTEntry(nValue, CExoString("Name"), &nCostTableName);
        sName = sName + " " + g_pAppManager->m_pClientExoApp->GetGUIString(nCostTableName);
        if (nHasParam1 != 0)
            sName = sName + ",";
    }
    if (nHasParam1 != 0)
    {
        C2DA* pParamTable = g_pRules->m_p2DArrays->GetIPRPParamTable(static_cast<unsigned char>(nParam1Table));
        int nParamName = -1;
        pParamTable->GetINTEntry(nParam, CExoString("Name"), &nParamName);
        sName = sName + " " + g_pAppManager->m_pClientExoApp->GetGUIString(nParamName);
    }
    list.Add(sName);
}

void CSWSItem::AddOnHitProperties(CExoString& sOut)
{
    if (!GetPropertyByTypeExists(ITEM_PROPERTY_ON_HIT_PROPERTIES, 0))
        return;
    CExoString s1;
    CExoString s2;
    CExoString s3;
    CExoString s4;
    CExoString sUnused;
    CExoString sLine;
    int nValue;
    int nChance;
    int nDuration;
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp == 0)
            continue;
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
            continue;
        if (pProp->m_type != ITEM_PROPERTY_ON_HIT_PROPERTIES)
            continue;
        GetPropertyStrings(ITEM_PROPERTY_ON_HIT_PROPERTIES, pProp->m_subType, PROPERTY_VALUE_NONE, PROPERTY_PARAM_NONE, s1, s2, s3, s4, 0);
        switch (pProp->m_subType)
        {
            case IPRP_ONHIT_SLEEP:
            case IPRP_ONHIT_STUN:
            case IPRP_ONHIT_PARALYZE:
            case IPRP_ONHIT_CONFUSION:
            case IPRP_ONHIT_FEAR:
            case IPRP_ONHIT_SLOW:
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ON_HIT) + ": " + s2 + " ";
                GetPropertyChanceDurationDC(pProp->m_costTableValue, pProp->m_nParam1Value, nChance,
                                            nDuration, nValue);
                nDuration *= 3;
                sLine.Format(const_cast<char*>("%d%% %s, %d %s, %s %d\n"), nChance,
                             g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CHANCE).CStr(), nDuration,
                             g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SECONDS).CStr(),
                             g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DC).CStr(), nValue);
                sOut = sOut + sLine + "\n";
                break;
            case IPRP_ONHIT_ABILITY_DRAIN:
            case IPRP_ONHIT_SLAY_RACIAL_GROUP:
            case IPRP_ONHIT_SLAY_ALIGNMENT_GROUP:
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ON_HIT) + ": " + s2 + " ";
                GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue,
                                   pProp->m_nParam1Value, s1, s2, s3, s4, 0);
                sOut = sOut + s4 + " " + s3 + "\n\n";
                break;
            case IPRP_ONHIT_ITEM_POISON:
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ON_HIT) + ": ";
                GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue,
                                   pProp->m_nParam1Value, s1, s2, s3, s4, 0);
                g_pRules->m_p2DArrays->m_pPoison->GetINTEntry(pProp->m_nParam1Value,
                                                              CExoString("DC_SAVE"), &nValue);
                sOut = sOut + s4 + " " + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DC) + " " +
                       CExoString(nValue) + "\n\n";
                break;
            case IPRP_ONHIT_INSTANT_DEATH:
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ON_HIT) + ": " + s2 + " ";
                GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue,
                                   pProp->m_nParam1Value, s1, s2, s3, s4, 0);
                sOut = sOut + ", " + s3 + "\n\n";
                break;
#if !KOTOR1  // K1 has no knockdown line
            case IPRP_ONHIT_KNOCKDOWN:
                sOut = sOut + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ON_HIT) + ": " + s2 + " ";
                GetPropertyChanceDurationDC(pProp->m_costTableValue, pProp->m_nParam1Value, nChance,
                                            nDuration, nValue);
                nDuration *= 3;
                sLine.Format(const_cast<char*>("%s %d\n"), g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DC).CStr(),
                             nValue);
                sOut = sOut + sLine + "\n";
                break;
#endif
        }
    }
}

void CSWSItem::AddAttackBonusStrings(int nType, CExoString& sOut)
{
    CExoString s1;
    CExoString s2;
    CExoString s3;
    CExoString s4;
    CExoString sUnused;
    int nTotal = 0;
    if (!GetPropertyByTypeExists(static_cast<unsigned short>(nType), 0))
        return;
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        if (pProp == 0)
            continue;
        if (pProp->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pProp->m_nUpgradeType)) == 0)
            continue;
        if (pProp->m_type != nType)
            continue;
        int bEarlier = 0;
        for (int j = 0; j < m_nPassiveProperties; j++)
        {
            CSWItemProperty* pOther = &m_pPassiveProperties[j];
            if (pOther == 0)
                continue;
            if (pOther->m_nUpgradeType != CSWItemProperty::UPGRADE_TYPE_NONE && (m_nUpgrades & (1 << pOther->m_nUpgradeType)) == 0)
                continue;
            if (pOther->m_type != nType)
                continue;
            if (pProp->m_subType != pOther->m_subType)
                continue;
            if (j < i)
            {
                bEarlier = 1;
                break;
            }
            nTotal += pOther->m_costTableValue;
        }
        if (bEarlier == 0)
        {
            GetPropertyStrings(pProp->m_type, pProp->m_subType, pProp->m_costTableValue, PROPERTY_PARAM_NONE, s1,
                               s2, s3, s4, 0);
            sOut = sOut + "+" + CExoString(nTotal) + " " +
                   g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_VS) + " " + s2 + "\n";
        }
    }
}

unsigned short CSWSItem::GetUsedActivePropertyUsesLeft(unsigned char nIndex)
{
    if (m_nActiveProperties <= nIndex)
        return 0;
    CSWItemProperty* pProp = &m_pActiveProperties[nIndex];
    if (pProp == 0)
        return 0;
    if (pProp->m_type != ITEM_PROPERTY_ACTIVATE_ITEM)
        return 0;
    switch (pProp->m_costTableValue)
    {
    case IPRP_CHARGECOST_SINGLE_USE:
        if (pProp->m_bUseableByte == 0)
            return 0;
        return static_cast<unsigned char>(m_stackCount);
    case IPRP_CHARGECOST_5_CHARGES_PER_USE:
        if (pProp->m_bUseableByte == 0)
            return 0;
        if (m_nCharges < 1)
            return 0;
        return static_cast<unsigned short>(m_nCharges / 5);
    case IPRP_CHARGECOST_4_CHARGES_PER_USE:
        if (pProp->m_bUseableByte == 0)
            return 0;
        if (m_nCharges < 1)
            return 0;
        return static_cast<unsigned short>(m_nCharges / 4);
    case IPRP_CHARGECOST_3_CHARGES_PER_USE:
        if (pProp->m_bUseableByte == 0)
            return 0;
        if (m_nCharges < 1)
            return 0;
        return static_cast<unsigned short>(m_nCharges / 3);
    case IPRP_CHARGECOST_2_CHARGES_PER_USE:
        if (pProp->m_bUseableByte == 0)
            return 0;
        if (m_nCharges < 1)
            return 0;
        return static_cast<unsigned short>(m_nCharges / 2);
    case IPRP_CHARGECOST_1_CHARGE_PER_USE:
        if (pProp->m_bUseableByte == 0)
            return 0;
        return static_cast<unsigned short>(m_nCharges > 0 ? m_nCharges : 0);
    case IPRP_CHARGECOST_1_USE_PER_DAY:
    case IPRP_CHARGECOST_2_USES_PER_DAY:
    case IPRP_CHARGECOST_3_USES_PER_DAY:
    case IPRP_CHARGECOST_4_USES_PER_DAY:
    case IPRP_CHARGECOST_5_USES_PER_DAY:
        if (pProp->m_bUseableByte == 0)
            return 0;
        return pProp->m_nUsesPerDay;
    case IPRP_CHARGECOST_1_PER_MINUTE:
    case IPRP_CHARGECOST_1_PER_2_MINUTES:
    case IPRP_CHARGECOST_1_PER_3_MINUTES:
    case IPRP_CHARGECOST_1_PER_4_MINUTES:
    case IPRP_CHARGECOST_1_PER_5_MINUTES:
        return pProp->m_bUseableByte != 0;
    }
    return 0;
}

int CSWSItem::ShowModel()
{
    if (GetClientObject() == 0)
        return 0;
    CSWCItem* pItem = GetClientObject()->AsSWCItem();
    if (pItem == 0)
        return 0;
    if (pItem->LoadModel(pItem->m_modelResRef, CSWCObject::ANIM_MODEL_BODY, 0) != 0)
        return 0;
    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_bHasEnvMap == 0)
        return 1;
    pItem->SetEnvMap(CSWCObject::ANIM_MODEL_BODY, 1, CResRef("default"));
    return 1;
}

int CSWSItem::SaveItem(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldINT(pStruct, m_baseItem, const_cast<char*>("BaseItem"));
    pGFF->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Tag"));
    pGFF->WriteFieldBYTE(pStruct, 1, const_cast<char*>("Identified"));
    pGFF->WriteFieldCExoLocString(pStruct, m_description, const_cast<char*>("Description"));
    pGFF->WriteFieldCExoLocString(pStruct, m_descriptionIdentified, const_cast<char*>("DescIdentified"));
    pGFF->WriteFieldCExoLocString(pStruct, m_localizedName, const_cast<char*>("LocalizedName"));
    pGFF->WriteFieldWORD(pStruct, static_cast<unsigned short>(m_stackCount), const_cast<char*>("StackSize"));
    pGFF->WriteFieldBYTE(pStruct, m_bits.m_bStolen, const_cast<char*>("Stolen"));
    pGFF->WriteFieldDWORD(pStruct, m_nUpgrades, const_cast<char*>("Upgrades"));
#if !KOTOR1  // K1: no UpgradeLevel / UpgradeSlot* fields
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nUpgradeLevel), const_cast<char*>("UpgradeLevel"));
    if (m_anUpgVars[0] != 0)
        pGFF->WriteFieldINT(pStruct, m_anUpgVars[0], const_cast<char*>("UpgradeSlot0"));
    if (m_anUpgVars[1] != 0)
        pGFF->WriteFieldINT(pStruct, m_anUpgVars[1], const_cast<char*>("UpgradeSlot1"));
    if (m_anUpgVars[2] != 0)
        pGFF->WriteFieldINT(pStruct, m_anUpgVars[2], const_cast<char*>("UpgradeSlot2"));
    if (m_anUpgVars[3] != 0)
        pGFF->WriteFieldINT(pStruct, m_anUpgVars[3], const_cast<char*>("UpgradeSlot3"));
    if (m_anUpgVars[4] != 0)
        pGFF->WriteFieldINT(pStruct, m_anUpgVars[4], const_cast<char*>("UpgradeSlot4"));
    if (m_anUpgVars[5] != 0)
        pGFF->WriteFieldINT(pStruct, m_anUpgVars[5], const_cast<char*>("UpgradeSlot5"));
#endif
    pGFF->WriteFieldBYTE(pStruct, m_bits.m_bDropable, const_cast<char*>("Dropable"));
    pGFF->WriteFieldBYTE(pStruct, m_bits.m_bPickpocketable, const_cast<char*>("Pickpocketable"));
    pGFF->WriteFieldBYTE(pStruct, m_nModelVariation, const_cast<char*>("ModelVariation"));
    if (g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nModelType == 1)
    {
        pGFF->WriteFieldBYTE(pStruct, m_nBodyVariation, const_cast<char*>("BodyVariation"));
        pGFF->WriteFieldBYTE(pStruct, m_nTextureVar, const_cast<char*>("TextureVar"));
    }
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nCharges), const_cast<char*>("Charges"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nMaxCharges), const_cast<char*>("MaxCharges"));
    pGFF->WriteFieldDWORD(pStruct, GetCost(), const_cast<char*>("Cost"));
    pGFF->WriteFieldDWORD(pStruct, m_nAddCost, const_cast<char*>("AddCost"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bPlot), const_cast<char*>("Plot"));
    if (g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_bContainer != 0)
        SaveContainerItems(pGFF, pStruct);
    SaveItemProperties(pGFF, pStruct);
    pGFF->WriteFieldFLOAT(pStruct, m_position.x, const_cast<char*>("XPosition"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.y, const_cast<char*>("YPosition"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.z, const_cast<char*>("ZPosition"));
    pGFF->WriteFieldFLOAT(pStruct, m_orientation.x, const_cast<char*>("XOrientation"));
    pGFF->WriteFieldFLOAT(pStruct, m_orientation.y, const_cast<char*>("YOrientation"));
    pGFF->WriteFieldFLOAT(pStruct, m_orientation.z, const_cast<char*>("ZOrientation"));
    pGFF->WriteFieldBYTE(pStruct, m_bits.m_bNonEquippable, const_cast<char*>("NonEquippable"));
    pGFF->WriteFieldBYTE(pStruct, m_bits.m_bNewItem, const_cast<char*>("NewItem"));
    pGFF->WriteFieldBYTE(pStruct, (m_flags >> 8) & 1, const_cast<char*>("DELETING"));
    return 1;
}

int CSWSItem::SaveContainerItems(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    pGFF->AddList(&list, pStruct, const_cast<char*>("ItemList"));
    for (unsigned int i = 0; i < m_pItemRepository->ItemListGetCount(); i++)
    {
        CResStruct element;
        pGFF->AddListElement(&element, &list, i);
        m_pItemRepository->ItemListGetItem(i)->SaveItem(pGFF, &element);
    }
    return 1;
}

int CSWSItem::SaveItemProperties(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    CResStruct element;
    pGFF->AddList(&list, pStruct, const_cast<char*>("PropertiesList"));
    for (int i = 0; i < m_nActiveProperties; i++)
    {
        pGFF->AddListElement(&element, &list, i);
        CSWItemProperty* pProp = GetActiveProperty(i);
        pGFF->WriteFieldWORD(&element, pProp->m_type, const_cast<char*>("PropertyName"));
        pGFF->WriteFieldWORD(&element, pProp->m_subType, const_cast<char*>("Subtype"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nCostTable, const_cast<char*>("CostTable"));
        pGFF->WriteFieldWORD(&element, pProp->m_costTableValue, const_cast<char*>("CostValue"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nParam1, const_cast<char*>("Param1"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nParam1Value, const_cast<char*>("Param1Value"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nChanceAppear, const_cast<char*>("ChanceAppear"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nUsesPerDay, const_cast<char*>("UsesPerDay"));
        pGFF->WriteFieldBYTE(&element, static_cast<unsigned char>(pProp->m_bUseable), const_cast<char*>("Useable"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nUpgradeType, const_cast<char*>("UpgradeType"));
#if !KOTOR1  // K1 has no PropUpgrade field
        pGFF->WriteFieldWORD(&element, pProp->m_nPropUpgrade, const_cast<char*>("PropUpgrade"));
#endif
    }
    for (int i = 0; i < m_nPassiveProperties; i++)
    {
        pGFF->AddListElement(&element, &list, m_nActiveProperties + i);
        CSWItemProperty* pProp = GetPassiveProperty(i);
        pGFF->WriteFieldWORD(&element, pProp->m_type, const_cast<char*>("PropertyName"));
        pGFF->WriteFieldWORD(&element, pProp->m_subType, const_cast<char*>("Subtype"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nCostTable, const_cast<char*>("CostTable"));
        pGFF->WriteFieldWORD(&element, pProp->m_costTableValue, const_cast<char*>("CostValue"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nParam1, const_cast<char*>("Param1"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nParam1Value, const_cast<char*>("Param1Value"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nChanceAppear, const_cast<char*>("ChanceAppear"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nUsesPerDay, const_cast<char*>("UsesPerDay"));
        pGFF->WriteFieldBYTE(&element, static_cast<unsigned char>(pProp->m_bUseable), const_cast<char*>("Useable"));
        pGFF->WriteFieldBYTE(&element, pProp->m_nUpgradeType, const_cast<char*>("UpgradeType"));
#if !KOTOR1  // K1 has no PropUpgrade field
        pGFF->WriteFieldWORD(&element, pProp->m_nPropUpgrade, const_cast<char*>("PropUpgrade"));
#endif
    }
    return 1;
}

void CSWSItem::ReadContainerItemsFromGff(CResGFF* pGFF, CResStruct* pStruct)
{
    if (m_pItemRepository == 0)
        return;
    CResList list;
    if (pGFF->GetList(&list, pStruct, const_cast<char*>("ItemList")) != 1)
        return;
    for (unsigned int i = 0; i < pGFF->GetListCount(&list); i++)
    {
        CResStruct element;
        int nReadOk;
        CSWSItem* pItem = 0;
        pGFF->GetListElement(&element, &list, i);
        uint64_t nObjectId =
            pGFF->ReadFieldDWORD(&element, const_cast<char*>("ObjectId"), nReadOk, OBJECT_INVALID);
        pItem = new CSWSItem(nObjectId);
        if (!pItem->LoadItem(pGFF, &element))
        {
            if (pItem != 0)
                delete pItem;
            continue;
        }
        int nByteOk;
        pGFF->ReadFieldBYTE(&element, const_cast<char*>("Repos_PosX"), nByteOk, 0xff);
        pGFF->ReadFieldBYTE(&element, const_cast<char*>("Repos_Posy"), nByteOk, 0xff);
        m_pItemRepository->AddItem(&pItem, 1, 0, 1);
        if (pItem != 0)
            pItem->SetPossessor(m_objectId, 1, 0, OBJECT_INVALID);
    }
}

int CSWSItem::LoadItem(CResGFF* pGFF, CResStruct* pStruct)
{
    int nResult = 0;
    CResRef sEquipped = pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("EquippedRes"), nResult, CResRef(""));
    if (nResult != 0)
    {
        nResult = LoadFromTemplate(sEquipped);
        int nByteOk;
        unsigned char bDropable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Dropable"), nByteOk, 0);
        if (nByteOk != 0)
            m_bits.m_bDropable = bDropable;
        unsigned char bPickpocketable =
            pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Pickpocketable"), nByteOk, 0);
        if (nByteOk != 0)
            m_bits.m_bPickpocketable = bPickpocketable;
        return nResult;
    }

    nResult = 0;
    CResRef sInventory =
        pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("InventoryRes"), nResult, CResRef(""));
    if (nResult == 0)
    {
        LoadDataFromGff(pGFF, pStruct);
        return 1;
    }
    nResult = LoadFromTemplate(sInventory);
    int nByteOk;
    unsigned char bDropable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Dropable"), nByteOk, 0);
    if (nByteOk != 0)
        m_bits.m_bDropable = bDropable;
    unsigned char bPickpocketable =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Pickpocketable"), nByteOk, 0);
    if (nByteOk != 0)
        m_bits.m_bPickpocketable = bPickpocketable;
    return nResult;
}

void CSWSItem::LoadDataFromGff(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    CResStruct element;
    int nReadOk;
    m_baseItem = pGFF->ReadFieldINT(pStruct, const_cast<char*>("BaseItem"), nReadOk, m_baseItem);
    InitRepository(m_objectId);
    m_sTag = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), nReadOk, m_sTag).LowerCase();
    SetIdentified(pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Identified"), nReadOk, 1));
    m_description = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("Description"), nReadOk,
                                                 m_descriptionIdentified);
    m_descriptionIdentified = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("DescIdentified"),
                                                           nReadOk, m_description);
    m_localizedName = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LocalizedName"), nReadOk,
                                                   m_localizedName);
    m_stackCount = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("StackSize"), nReadOk, static_cast<unsigned short>(m_stackCount));
    m_bits.m_bStolen = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Stolen"), nReadOk, m_bits.m_bStolen);
    m_nUpgrades = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Upgrades"), nReadOk, m_nUpgrades);
#if !KOTOR1  // K1: no UpgradeLevel / UpgradeSlot* fields
    m_nUpgradeLevel = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("UpgradeLevel"), nReadOk, static_cast<unsigned char>(m_nUpgrades));
    m_anUpgVars[0] = pGFF->ReadFieldINT(pStruct, const_cast<char*>("UpgradeSlot0"), nReadOk, -1);
    m_anUpgVars[1] = pGFF->ReadFieldINT(pStruct, const_cast<char*>("UpgradeSlot1"), nReadOk, -1);
    m_anUpgVars[2] = pGFF->ReadFieldINT(pStruct, const_cast<char*>("UpgradeSlot2"), nReadOk, -1);
    m_anUpgVars[3] = pGFF->ReadFieldINT(pStruct, const_cast<char*>("UpgradeSlot3"), nReadOk, -1);
    m_anUpgVars[4] = pGFF->ReadFieldINT(pStruct, const_cast<char*>("UpgradeSlot4"), nReadOk, -1);
    m_anUpgVars[5] = pGFF->ReadFieldINT(pStruct, const_cast<char*>("UpgradeSlot5"), nReadOk, -1);
#endif
    m_bits.m_bDropable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Dropable"), nReadOk, 0);
    m_bits.m_bPickpocketable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Pickpocketable"), nReadOk, 0);
    m_bits.m_bNonEquippable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("NonEquippable"), nReadOk, m_bits.m_bNonEquippable);
    unsigned char nModelVariation =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("ModelVariation"), nReadOk, m_nModelVariation);
    if (nReadOk == 0)
    {
        nModelVariation = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("ModelPart1"), nReadOk, nModelVariation);
        if (nModelVariation == 0)
            nModelVariation = 1;
    }
    m_nModelVariation = nModelVariation;
    if (g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nModelType == 1)
    {
        m_nBodyVariation = g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_nBodyVar;
        m_nTextureVar = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TextureVar"), nReadOk, 1);
    }
    m_nCharges = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Charges"), nReadOk, 50);
    m_nMaxCharges = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("MaxCharges"), nReadOk, static_cast<unsigned char>(m_nCharges));
    m_bits.m_bNewItem = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("NewItem"), nReadOk, m_bits.m_bNewItem);
    m_bits.m_bDeleting = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("DELETING"), nReadOk, m_bits.m_bDeleting);
    m_nAddCost = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("AddCost"), nReadOk, m_nAddCost);
    if (g_pRules->m_pBaseItemArray->GetBaseItem(m_baseItem)->m_bContainer != 0)
        ReadContainerItemsFromGff(pGFF, pStruct);
    m_bPlot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Plot"), nReadOk, static_cast<unsigned char>(m_bPlot));
    if (pGFF->GetList(&list, pStruct, const_cast<char*>("PropertiesList")) == 1)
    {
        if (pGFF->GetListCount(&list) != 0)
        {
            int nCount = pGFF->GetListCount(&list);
            int nActiveCount = 0;
            for (int i = 0; i < nCount; i++)
            {
                pGFF->GetListElement(&element, &list, i);
                if (pGFF->GetFieldCount(&element) == 0)
                    continue;
                unsigned short nType =
                    pGFF->ReadFieldWORD(&element, const_cast<char*>("PropertyName"), nReadOk, 0);
                if (nReadOk != 0 && ActiveProperty(nType) != 0)
                    nActiveCount++;
            }
            m_nActiveProperties = nActiveCount;
            delete[] m_pActiveProperties;
            m_pActiveProperties =
                nActiveCount != 0 ? new CSWItemProperty[m_nActiveProperties] : 0;
            int nActive = 0;
            for (int i = 0; i < nCount; i++)
            {
                pGFF->GetListElement(&element, &list, i);
                if (pGFF->GetFieldCount(&element) == 0)
                    continue;
                unsigned short nType =
                    pGFF->ReadFieldWORD(&element, const_cast<char*>("PropertyName"), nReadOk, 0);
                if (ActiveProperty(nType) == 0)
                    continue;
                if (nReadOk != 0)
                    m_pActiveProperties[nActive].m_type = nType;
                unsigned short nSubType =
                    pGFF->ReadFieldWORD(&element, const_cast<char*>("Subtype"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pActiveProperties[nActive].m_subType = nSubType;
                unsigned char nCostTable =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("CostTable"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pActiveProperties[nActive].m_nCostTable = nCostTable;
                unsigned short nCostValue =
                    pGFF->ReadFieldWORD(&element, const_cast<char*>("CostValue"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pActiveProperties[nActive].m_costTableValue = nCostValue;
                unsigned char nParam1 =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("Param1"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pActiveProperties[nActive].m_nParam1 = nParam1;
                unsigned char nParam1Value =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("Param1Value"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pActiveProperties[nActive].m_nParam1Value = nParam1Value;
                unsigned char nChanceAppear =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("ChanceAppear"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pActiveProperties[nActive].m_nChanceAppear = nChanceAppear;
                unsigned char nUseable =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("Useable"), nReadOk, 1);
                m_pActiveProperties[nActive].m_bUseable = nUseable;
                unsigned char nUsesPerDay =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("UsesPerDay"), nReadOk, CSWItemProperty::USES_PER_DAY_UNSET);
                m_pActiveProperties[nActive].m_nUsesPerDay = nUsesPerDay;
                unsigned char nUpgradeType =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("UpgradeType"), nReadOk, CSWItemProperty::UPGRADE_TYPE_NONE);
                m_pActiveProperties[nActive].m_nUpgradeType = nUpgradeType;
#if !KOTOR1  // K1 has no PropUpgrade field
                unsigned short nPropUpgrade =
                    pGFF->ReadFieldWORD(&element, const_cast<char*>("PropUpgrade"), nReadOk, 0xff);
                m_pActiveProperties[nActive].m_nPropUpgrade = nPropUpgrade;
#endif
                nActive++;
            }
            m_nPassiveProperties = nCount - m_nActiveProperties;
            delete[] m_pPassiveProperties;
            m_pPassiveProperties =
                m_nPassiveProperties != 0 ? new CSWItemProperty[m_nPassiveProperties] : 0;
            int nPassive = 0;
            for (int i = 0; i < nCount; i++)
            {
                pGFF->GetListElement(&element, &list, i);
                if (pGFF->GetFieldCount(&element) == 0)
                    continue;
                unsigned short nType =
                    pGFF->ReadFieldWORD(&element, const_cast<char*>("PropertyName"), nReadOk, 0);
                if (ActiveProperty(nType) != 0)
                    continue;
                if (nReadOk != 0)
                    m_pPassiveProperties[nPassive].m_type = nType;
                unsigned short nSubType =
                    pGFF->ReadFieldWORD(&element, const_cast<char*>("Subtype"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pPassiveProperties[nPassive].m_subType = nSubType;
                unsigned char nCostTable =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("CostTable"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pPassiveProperties[nPassive].m_nCostTable = nCostTable;
                unsigned short nCostValue =
                    pGFF->ReadFieldWORD(&element, const_cast<char*>("CostValue"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pPassiveProperties[nPassive].m_costTableValue = nCostValue;
                unsigned char nParam1 =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("Param1"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pPassiveProperties[nPassive].m_nParam1 = nParam1;
                unsigned char nParam1Value =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("Param1Value"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pPassiveProperties[nPassive].m_nParam1Value = nParam1Value;
                unsigned char nChanceAppear =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("ChanceAppear"), nReadOk, 0);
                if (nReadOk != 0)
                    m_pPassiveProperties[nPassive].m_nChanceAppear = nChanceAppear;
                unsigned char nUseable =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("Useable"), nReadOk, 0);
                m_pPassiveProperties[nPassive].m_bUseable = nUseable;
                unsigned char nUsesPerDay =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("UsesPerDay"), nReadOk, CSWItemProperty::USES_PER_DAY_UNSET);
                m_pPassiveProperties[nPassive].m_nUsesPerDay = nUsesPerDay;
                unsigned char nUpgradeType =
                    pGFF->ReadFieldBYTE(&element, const_cast<char*>("UpgradeType"), nReadOk, CSWItemProperty::UPGRADE_TYPE_NONE);
                m_pPassiveProperties[nPassive].m_nUpgradeType = nUpgradeType;
#if !KOTOR1  // K1 has no PropUpgrade field
                unsigned short nPropUpgrade =
                    pGFF->ReadFieldWORD(&element, const_cast<char*>("PropUpgrade"), nReadOk, 0xff);
                m_pPassiveProperties[nPassive].m_nPropUpgrade = nPropUpgrade;
#endif
                nPassive++;
            }
        }
    }
    for (int i = 0; i < m_nActiveProperties; i++)
    {
        CSWItemProperty* pProp = &m_pActiveProperties[i];
        if (pProp != 0 && pProp->m_type == ITEM_PROPERTY_ACTIVATE_ITEM && pProp->m_nUsesPerDay == CSWItemProperty::USES_PER_DAY_UNSET) {
            if (static_cast<unsigned int>(pProp->m_costTableValue - IPRP_CHARGECOST_1_USE_PER_DAY) <=
                IPRP_CHARGECOST_5_USES_PER_DAY - IPRP_CHARGECOST_1_USE_PER_DAY)
                pProp->m_nUsesPerDay = static_cast<unsigned char>(pProp->m_costTableValue - 7);
            pProp->m_bUseable = 1;
        }
    }
    UpdateUsedActiveProperties();
    if (m_nActiveProperties == 0 && m_nPassiveProperties == 0)
        m_flags |= FLAG_IDENTIFIED;
    if (GetBaseItem()->m_nModelType == 1)
        m_nBaseArmorClass = GetBaseItem()->m_nBaseAC;
    else
        m_nBaseArmorClass = 0;
    g_pAppManager->m_pServerExoApp->GetModule()->AddObjectToLookupTable(m_sTag, m_objectId);
    m_nWeight = GetBaseItem()->m_nTenthLbs;
}

int CSWSItem::CompareItem(CSWSItem* pOther) {
    if (m_nUpgrades != pOther->m_nUpgrades)
        return 0;
    if (m_nActiveProperties != pOther->m_nActiveProperties)
        return 0;
    if (m_nPassiveProperties != pOther->m_nPassiveProperties)
        return 0;
    if (m_bPlot != pOther->m_bPlot)
        return 0;
    if (m_nCharges != pOther->m_nCharges)
        return 0;
    if (m_baseItem != pOther->m_baseItem)
        return 0;
    if (m_bits.m_bStolen != pOther->m_bits.m_bStolen)
        return 0;
    if (m_nModelVariation != pOther->m_nModelVariation)
        return 0;
    if (m_nBodyVariation != pOther->m_nBodyVariation)
        return 0;
    if (m_nTextureVar != pOther->m_nTextureVar)
        return 0;
    if (m_sTag != pOther->m_sTag)
        return 0;
    if (m_localizedName != pOther->m_localizedName)
        return 0;

    for (int i = 0; i < m_nActiveProperties; i++) {
        CSWItemProperty* pProp = &m_pActiveProperties[i];
        CSWItemProperty* pOtherProp;
        if (i < pOther->m_nActiveProperties)
            pOtherProp = &pOther->m_pActiveProperties[i];
        else
            pOtherProp = 0;
#ifdef K2_FIX_ENGINE_BUGS
        if (pOtherProp == 0)
            return 0;
#endif
        if (pProp->m_type != pOtherProp->m_type)
            return 0;
        if (pProp->m_subType != pOtherProp->m_subType)
            return 0;
        if (pProp->m_nCostTable != pOtherProp->m_nCostTable)
            return 0;
        if (pProp->m_costTableValue != pOtherProp->m_costTableValue)
            return 0;
        if (pProp->m_nParam1 != pOtherProp->m_nParam1)
            return 0;
        if (pProp->m_nParam1Value != pOtherProp->m_nParam1Value)
            return 0;
        if (pProp->m_nUsesPerDay != pOtherProp->m_nUsesPerDay)
            return 0;
        if (pProp->m_nUpgradeType != pOtherProp->m_nUpgradeType)
            return 0;
    }
    for (int i = 0; i < m_nPassiveProperties; i++) {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        CSWItemProperty* pOtherProp;
        if (i < pOther->m_nPassiveProperties)
            pOtherProp = &pOther->m_pPassiveProperties[i];
        else
            pOtherProp = 0;
#ifdef K2_FIX_ENGINE_BUGS
        if (pOtherProp == 0)
            return 0;
#endif
        if (pProp->m_type != pOtherProp->m_type)
            return 0;
        if (pProp->m_subType != pOtherProp->m_subType)
            return 0;
        if (pProp->m_nCostTable != pOtherProp->m_nCostTable)
            return 0;
        if (pProp->m_costTableValue != pOtherProp->m_costTableValue)
            return 0;
        if (pProp->m_nParam1 != pOtherProp->m_nParam1)
            return 0;
        if (pProp->m_nParam1Value != pOtherProp->m_nParam1Value)
            return 0;
        if (pProp->m_nUsesPerDay != pOtherProp->m_nUsesPerDay)
            return 0;
        if (pProp->m_nUpgradeType != pOtherProp->m_nUpgradeType)
            return 0;
    }
    for (int i = 0; i < m_nPassiveProperties; i++) {
        CSWItemProperty* pProp = &m_pPassiveProperties[i];
        CSWItemProperty* pOtherProp;
        if (i < pOther->m_nPassiveProperties)
            pOtherProp = &pOther->m_pPassiveProperties[i];
        else
            pOtherProp = 0;
#ifdef K2_FIX_ENGINE_BUGS
        if (pOtherProp == 0)
            return 0;
#endif
        if (pProp->m_type != pOtherProp->m_type)
            return 0;
        if (pProp->m_subType != pOtherProp->m_subType)
            return 0;
        if (pProp->m_nCostTable != pOtherProp->m_nCostTable)
            return 0;
        if (pProp->m_costTableValue != pOtherProp->m_costTableValue)
            return 0;
        if (pProp->m_nParam1 != pOtherProp->m_nParam1)
            return 0;
        if (pProp->m_nParam1Value != pOtherProp->m_nParam1Value)
            return 0;
        if (pProp->m_nUsesPerDay != pOtherProp->m_nUsesPerDay)
            return 0;
        if (pProp->m_nUpgradeType != pOtherProp->m_nUpgradeType)
            return 0;
    }
    return 1;
}

void CSWSItem::DecrementUses(int nIndex, unsigned long long userId) {
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bUnlimitedItemUses != 0)
        return;

    CSWItemProperty* pProp;
    if (nIndex >= 0 && nIndex < m_nActiveProperties)
        pProp = &m_pActiveProperties[nIndex];
    else
        pProp = 0;

    bool bChanged = false;
    bool bFlag = true;
    bool bDestroy = false;

#ifdef K2_FIX_ENGINE_BUGS
    if (pProp == 0)
        return;
#endif
    switch (pProp->m_costTableValue) {
    case IPRP_CHARGECOST_SINGLE_USE:
        if (m_stackCount >= 2) {
            m_stackCount--;
            break;
        }
        // The last use of a single-use item is not a change: the item is destroyed instead.
        pProp->m_bUseable = 0;
        bChanged = false;
        bFlag = true;
        bDestroy = true;
        break;

    case IPRP_CHARGECOST_5_CHARGES_PER_USE:
    case IPRP_CHARGECOST_4_CHARGES_PER_USE:
    case IPRP_CHARGECOST_3_CHARGES_PER_USE:
    case IPRP_CHARGECOST_2_CHARGES_PER_USE:
    case IPRP_CHARGECOST_1_CHARGE_PER_USE: {
        int nDec;
        switch (pProp->m_costTableValue) {
        case IPRP_CHARGECOST_4_CHARGES_PER_USE: nDec = 4; break;
        case IPRP_CHARGECOST_3_CHARGES_PER_USE: nDec = 3; break;
        case IPRP_CHARGECOST_2_CHARGES_PER_USE: nDec = 2; break;
        case IPRP_CHARGECOST_1_CHARGE_PER_USE: nDec = 1; break;
        default: nDec = 5; break;
        }
        if (m_nCharges >= nDec)
            m_nCharges -= nDec;
        // The sweep reuses pProp as its iterator, so the m_bUseableByte test below reads the last
        // active property, not the one that was used.
        for (int i = 0; i < m_nActiveProperties; i++) {
            pProp = &m_pActiveProperties[i];
            if (pProp == 0 || pProp->m_type != ITEM_PROPERTY_ACTIVATE_ITEM)
                continue;
            switch (pProp->m_costTableValue) {
            case IPRP_CHARGECOST_5_CHARGES_PER_USE: if (m_nCharges < 5) pProp->m_bUseable = 0; break;
            case IPRP_CHARGECOST_4_CHARGES_PER_USE: if (m_nCharges < 4) pProp->m_bUseable = 0; break;
            case IPRP_CHARGECOST_3_CHARGES_PER_USE: if (m_nCharges < 3) pProp->m_bUseable = 0; break;
            case IPRP_CHARGECOST_2_CHARGES_PER_USE: if (m_nCharges < 2) pProp->m_bUseable = 0; break;
            case IPRP_CHARGECOST_1_CHARGE_PER_USE: if (m_nCharges < 1) pProp->m_bUseable = 0; break;
            }
        }
        bChanged = false;
        bFlag = false;
        bDestroy = false;
        break;
    }

    case IPRP_CHARGECOST_0_CHARGES_PER_USE:
    case IPRP_CHARGECOST_UNLIMITED_USE:
        break;

    case IPRP_CHARGECOST_1_USE_PER_DAY: case IPRP_CHARGECOST_2_USES_PER_DAY:
    case IPRP_CHARGECOST_3_USES_PER_DAY: case IPRP_CHARGECOST_4_USES_PER_DAY:
    case IPRP_CHARGECOST_5_USES_PER_DAY:
        if (pProp->m_nUsesPerDay != 0) {
            pProp->m_nUsesPerDay--;
            if (pProp->m_nUsesPerDay != 0) {
                bChanged = true;
                bFlag = true;
                break;
            }
        }
        pProp->m_bUseable = 0;
        bChanged = true;
        bFlag = true;
        break;

    case IPRP_CHARGECOST_1_PER_MINUTE: case IPRP_CHARGECOST_1_PER_2_MINUTES:
    case IPRP_CHARGECOST_1_PER_3_MINUTES: case IPRP_CHARGECOST_1_PER_4_MINUTES:
    case IPRP_CHARGECOST_1_PER_5_MINUTES: {
        unsigned int nDay;
        unsigned int nTime;
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
        pProp->m_nLastUseDay = nDay;
        pProp->m_nLastUseTod = nTime;
        pProp->m_bUseable = 0;
        bChanged = true;
        bFlag = true;
        break;
    }
    }

    if (pProp->m_bUseableByte == 0) {
        if (GetBaseItem()->m_nItemType == ITEM_TYPE_STIMULANT || GetBaseItem()->m_nItemType == ITEM_TYPE_MEDICAL_EQUIPMENT) {
            bDestroy = true;
        } else {
            bool bRecoveryKit = (GetBaseItem()->m_nItemType == ITEM_TYPE_SQUAD_RECOVERY_KIT);
            if (bFlag || bRecoveryKit) {
                if (bRecoveryKit)
                    bDestroy = true;
            } else {
                int i;
                for (i = 0; i < m_nActiveProperties; i++) {
                    CSWItemProperty* p = &m_pActiveProperties[i];
                    if (p != 0 && p->m_type == ITEM_PROPERTY_ACTIVATE_ITEM &&
                        static_cast<unsigned int>(p->m_costTableValue - IPRP_CHARGECOST_5_CHARGES_PER_USE) <=
                            IPRP_CHARGECOST_1_CHARGE_PER_USE - IPRP_CHARGECOST_5_CHARGES_PER_USE &&
                        p->m_bUseableByte != 0)
                        break;
                }
                if (i == m_nActiveProperties)
                    bDestroy = true;
            }
        }
    }

    if (!bChanged && bDestroy) {
        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(userId);
        if (pPlayer != 0)
            g_pAppManager->m_pServerExoApp->GetSWSMessage()
                ->SendServerPlayerItemUpdate_DestroyItem(pPlayer, m_objectId);
        m_flags |= FLAG_DELETING;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, userId, m_objectId, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
    } else {
        UpdateUsedActiveProperties();
    }
}

