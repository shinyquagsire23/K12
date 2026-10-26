#include "k2stdint.h"
#include "nwssendplayermessage.h"
#include "nwsplayer.h"
#include "nwsarea.h"
#include "nwsmodule.h"
#include "nwsitem.h"
#include "nwsscriptevent.h"
#include "globalvartables.h"
#include "nwsplaceable.h"

#include "nwarea.h"

#include "nwspell.h"

#include "exobase.h"

#include "CExoArrayListImpl.h"

#include "worldtimer.h"

#include "nwsobject.h"

#include "Vector.h"

#include "nwsrules.h"

#include "exolinkedlistinternal.h"
#include "itemtype.h"

int RenderPlaceableBBox;

void* operator new(size_t size);
#include "serveraimaster.h"
#include "serverexoapp.h"

#include "nwmessage.h"
#include "nwscombatround.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwstrigger.h"
#include "nwcobject.h"
#include "swguiexamine.h"
#include "gameobjectarray.h"
#include "nwsaiaction.h"
#include "serveranimation.h"
#include "restype.h"
#include "factionmanager.h"
#include "nwscriptconst.h"
#include "nwsdialog.h"
#include "strref.h"
#include "exostringinline.h"

// The item local boolean set once a data pad's text has been shown in the datapad box.
enum {
    LOCAL_BOOLEAN_DATAPAD_SHOWN = 94,
};

// The combat-pause mode a message box pauses the game with.
enum {
    PAUSE_MODE_MESSAGE_BOX = 12,
};

// placeables.2da row of the rancor corpse, the one body bag with hardness 1.
enum {
    PLACEABLE_APPEARANCE_RANCOR_CORPSE = 217,
};

// The flag bit AddCastSpellActions ors into the cast action's path-type word for a metamagic
// cast.
enum : unsigned int {
    CAST_FLAG_METAMAGIC = 1u << 31,
};

extern int g_bEnableCombatDebugging;

CSWSPlaceable::CSWSPlaceable(unsigned long long objectId)
    : CSWSObject(CGameObject::OBJECT_TYPE_PLACEABLE, objectId, 0) {
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddObject(this, 0);
    m_bReserved1 = 0;
    m_bInventoryOpen = 0;
    m_lastUpdateDay = 0;
    m_lastUpdateTod = 0;
    m_oidLastOpenedBy = OBJECT_INVALID;
    m_oidLastClosedBy = OBJECT_INVALID;
    m_oidLastUsedBy = OBJECT_INVALID;
    m_oidEnteringObject = OBJECT_INVALID;
    m_oidLastDisarmedBy = OBJECT_INVALID;
    m_oidLastLockedBy = OBJECT_INVALID;
    m_oidLastUnlockedBy = OBJECT_INVALID;
    m_pItemRepository = new CItemRepository(m_objectId, 0);
    m_bTrapDisarmable = 1;
    m_bTrapDetectable = 1;
    m_bTrapOneShot = 1;
    m_bMineFlagged = 0;
#if !KOTOR1
    m_lockDCFormula = 1;
    m_lockDCFlatBase = 0;
#endif
    m_trapType = TRAP_TYPE_NONE;
    m_baseOpenLockDC = 0;
#if !KOTOR1
    m_nOwnerDemolitionsSkill = 0;
    m_bUseTweakColor = 0;
    m_tweakColor = 0;
#endif
    m_bGroundPile = 0;
    m_bDieWhenEmpty = 0;
    m_nInventoryWalkIndex = 0;
    m_nInventoryWalkSubIndex = 0;
    m_oidInventoryWalkItem = CGameObjectArray::INVALIDOBJID;
    m_staticBBoxId = -1;
    m_templateResRef = const_cast<char*>("");  // CResRef::operator=(char*) — the engine's real signature
    m_bearing = 0.0f;
    m_effectSpellId = SPELL_ID_NONE;
    m_lightIsOn = 1;
    m_nReserved5 = 0;
    m_computerSpikeBonus = 0;
    m_bIsBodyBag = 0;
    m_bIsBodyBagVisible = 1;
    m_bPlaceholder = (objectId == OBJECT_INVALID);
    m_nCorpseFlags = 0;
#if !KOTOR1
    m_pOwnedTrigger = 0;
#endif
    m_bLightIsOnDirty = 0;
    m_bodyBagAppearanceRow = 0;
    m_bPreciseUse = 0;
    m_oidCorpse = OBJECT_INVALID;
}

CSWSPlaceable::~CSWSPlaceable() {
    CItemRepository* pRepo = m_pItemRepository;
    if (pRepo != 0) {
        pRepo->~CItemRepository();
        operator delete(pRepo);
        m_pItemRepository = 0;
    }
    if (GetArea() != 0) {
        RemoveFromArea();
    }
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->RemoveObject(this);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0) {
        pModule->RemoveObjectFromLookupTable(m_sTag, m_objectId);
    }
    if (m_bIsBodyBag != 0) {
        unsigned long long clientId =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_oidCorpse);
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(clientId);
        if (pCreature != 0) {
            pCreature->m_oidBodyBag = OBJECT_INVALID;
        }
    }
}

CSWSPlaceable* CSWSPlaceable::AsSWSPlaceable() {
    return this;
}

CExoLocString& CSWSPlaceable::GetFirstName() {
    return m_firstName;
}

CResRef CSWSPlaceable::GetDialogResref() {
    return m_dialogResref;
}

unsigned int CSWSPlaceable::GetEffectSpellId() {
    return m_effectSpellId;
}

void CSWSPlaceable::SetEffectSpellId(unsigned int spellId) {
    m_effectSpellId = spellId;
}

void CSWSPlaceable::DoDamage(int amount) {
    if (m_bPlot) return;
    CSWSObject::DoDamage(amount);
}

int CSWSPlaceable::GetDead() {
    if (m_bPlot) return 0;
    return CSWSObject::GetDead();
}

void* CSWSPlaceable::GetClientPlaceable() {
    if (GetClientObject() == 0) return 0;
    CGameObject* pClientObj = GetClientObject();
    return pClientObj->AsSWCPlaceable();
}

#if !KOTOR1  // K2-only (no K1 counterpart)
unsigned char CSWSPlaceable::GetDisarmDC() {
    unsigned char base = m_baseDisarmDC;
    unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adj = (diff == 0) ? -5 : ((diff == 2) ? 5 : 0);
    unsigned char dc = static_cast<unsigned char>(adj + base);
    return dc ? dc : 1;
}
#endif // !KOTOR1

// Clamps the detect DC as a byte, returning at least 1. CSWSDoor::GetDetectDC does not: it tests the
// full signed int, an engine asymmetry.
#if !KOTOR1  // K2-only (no K1 counterpart)
unsigned char CSWSPlaceable::GetDetectDC() {
    unsigned char base = m_baseDetectDC;
    unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adj = (diff == 0) ? -5 : ((diff == 2) ? 5 : 0);
    unsigned char dc = static_cast<unsigned char>(adj + base);
    return dc ? dc : 1;
}
#endif // !KOTOR1

void CSWSPlaceable::OnEndDialog(CSWSObject* pSpeaker) {
    K2_UNUSED(pSpeaker);
    g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_END_DIALOGUE], m_objectId, 1);
    m_computerSpikeBonus = 0;
}

int CSWSPlaceable::NoNonWalkPolys(const Vector& a, const Vector& b, const float& c,
                                    const float& d, const float& e, CWalkHitInfo* hitInfo) {
    m_walkMesh.m_translation = m_position;
    m_walkMesh.m_rotation = m_orientation;
    return m_walkMesh.NoNonWalkPolys(a, b, c, d, e, hitInfo);
}

int CSWSPlaceable::RemoveItem(CSWSItem* pItem) {
    int found = m_pItemRepository->GetItemInRepository(pItem, 0);
    if (found != 1) return 0;
    m_pItemRepository->RemoveItem(pItem);
    pItem->SetPossessor(OBJECT_INVALID, 1, 1, OBJECT_INVALID);
    return 1;
}

// Takes the item out of the holder `possessorId` names and adds it to this container's repository.
//
// When the item's recorded possessor disagrees with the claimed one, both sides must be party-member
// creatures or the transfer is refused outright.
int CSWSPlaceable::AcquireItem(CSWSItem** ppItem, unsigned long long possessorId, int flag) {
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pObject = 0;
    CItemRepository* pRepository = m_pItemRepository;

#if KOTOR1  // K1: no shared party-inventory fallback
    if ((*ppItem)->m_possessorId != possessorId)
        return 0;
#else
    if ((*ppItem)->m_possessorId != possessorId) {
        CGameObject* pOldOwner =
            g_pAppManager->m_pServerExoApp->GetGameObject((*ppItem)->m_possessorId);
        CGameObject* pNewOwner = g_pAppManager->m_pServerExoApp->GetGameObject(possessorId);
        if (pOldOwner == 0 || pOldOwner->AsSWSCreature() == 0 ||
            pOldOwner->AsSWSCreature()->m_bInParty == 0 || pNewOwner == 0 ||
            pNewOwner->AsSWSCreature() == 0 || pNewOwner->AsSWSCreature()->m_bInParty == 0)
            return 0;
    }
#endif

    if (pObjectArray->GetGameObject(possessorId, &pObject) != CGameObjectArray::SUCCESS ||
        pObject == 0) {
        // With no live holder, the item is lying in the world, so unlink it from the area instead.
        (*ppItem)->RemoveFromArea();
        possessorId = OBJECT_INVALID;
    } else {
        possessorId = pObject->m_objectId;
        if (pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE) {
            pObject->AsSWSCreature()->RemoveItem(*ppItem, 1, flag, ppItem);
        } else if (pObject->m_objectType == GAME_OBJECT_TYPE_ITEM) {  // a container item
            CSWRules* pRules = g_pRules;
            int nBaseItem = pObject->AsSWSItem()->m_baseItem;
            if (pRules->GetBaseItem(nBaseItem)->m_bContainer == 1)
                pObject->AsSWSItem()->m_pItemRepository->RemoveItem(*ppItem);
        } else if (pObject->m_objectType == GAME_OBJECT_TYPE_PLACEABLE) {
            pObject->AsSWSPlaceable()->RemoveItem(*ppItem);
        }
    }

    if (pRepository == 0) return 0;
    // The result is discarded; this always returns 1.
    pRepository->AddItem(ppItem, 1, 0, 1);
    if (*ppItem == 0) return 1;
    (*ppItem)->SetPossessor(m_objectId, 1, 1, possessorId);
    return 1;
}

// Empties another object's carried items into this container. A creature contributes its 16
// equipment slots, its repository and its gold, the last materialized as a fresh
// "g_i_credits001" item; another placeable contributes its repository only. Returns how many
// items were actually taken.
//
// On the placeable path the "equipped only" flag is cleared, so the repository walk takes
// everything.
int CSWSPlaceable::AcquireItemsFromObject(unsigned long long oidOwner, int flag) {
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidOwner);
    CSWSPlaceable* pPlaceable =
        g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidOwner);
    if (pCreature == 0 && pPlaceable == 0)
        return 0;
    if (pCreature != 0) {
        if (pCreature->m_pInventory == 0)
            return 0;
        if (pCreature->GetItemRepository(1) == 0)
            return 0;
    }
    if (pPlaceable != 0 && pPlaceable->m_pItemRepository == 0)
        return 0;

    CSWSItem* pItem;
    CExoArrayList<CSWSItem*> list;
    CItemRepository* pRepository;
    if (pCreature != 0) {
        pRepository = pCreature->GetItemRepository(1);
#if KOTOR1  // K1 scans 14 equip slots
        for (int i = 0; i < 14; i++) {
#else
        for (int i = 0; i < 16; i++) {
#endif
            pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(1 << i));
            if (pItem != 0 && (flag == 0 || (pItem->m_nFlagsLow & 8)))
                list.Add(pItem);
        }
    } else {
        pRepository = pPlaceable->m_pItemRepository;
        flag = 0;
    }
    for (unsigned int i = 0; i < pRepository->ItemListGetCount(); i++) {
        pItem = pRepository->ItemListGetItem(i);
        if (pItem != 0 && (flag == 0 || (pItem->m_nFlagsLow & 8)))
            list.Add(pItem);
    }

    int nAcquired = 0;
    int nCount = list.m_count;
    for (int i = 0; i < nCount; i++) {
        pItem = list.m_pArray[i];
        if (pItem != 0 && AcquireItem(&pItem, oidOwner, 1) != 0)
            nAcquired++;
    }

    if (pCreature != 0 && pCreature->GetGold() != 0) {
        pItem = new CSWSItem(OBJECT_INVALID);
        pItem->LoadFromTemplate(CResRef("g_i_credits001"));
        pItem->m_stackCount = pCreature->GetGold();
        pCreature->SetGold(0);
        AcquireItem(&pItem, OBJECT_INVALID, 1);
    }
    return nAcquired;
}

int CSWSPlaceable::GetItemCount(int flag) {
    if (m_bHasInventory == 0) return 0;
    CItemRepository* repo = m_pItemRepository;
    if (repo == 0) return 0;
    if (repo->ItemListGetCount() == 0) return 0;
    int i = 0;
    int count = 0;
    do {
        CSWSItem* pItem = repo->ItemListGetItem(i);
#if KOTOR1  // K1 reads the 16-bit flags word
        if (pItem != 0 && (flag == 0 || (pItem->m_flags & 8))) count++;
#else
        if (pItem != 0 && (flag == 0 || (pItem->m_nFlagsLow & 8))) count++;
#endif
        i++;
    } while (static_cast<unsigned>(i) < repo->ItemListGetCount());
    return count;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
// The lock DC by formula. Formula 4 shifts the base DC by -5/0/+5 for difficulty 0/1/2 and floors it
// at zero; any other formula starts from m_lockDCFlatBase plus 21 (16 + 5 x difficulty below 3), and
// formula 3 also adds the PC's level at spawn plus 7.
// EFFECTIVE: one commutative add takes its operands in the other order (m_lockDCFlatBase + adj in
// the orig, adj + m_lockDCFlatBase here), which reschedules the non-4 arm. Structure and calls match.
unsigned char CSWSPlaceable::GetOpenLockDC() {
    unsigned char formula = m_lockDCFormula;
    int result;
    if (formula == 4) {
        unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
        int val = (diff < 3) ? diff * 5 - 5 : 0;
        val += m_baseOpenLockDC;
        result = val < 0 ? 0 : val;
    } else {
        int bLevelScaled = formula == 3;
        unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
        int adj = (diff < 3) ? diff * 5 + 16 : 21;
        int extra = m_nPCLevelAtSpawn + 7;
        if (!bLevelScaled) {
            extra = 0;
        }
        result = m_lockDCFlatBase + adj + extra;
    }
    return result;
}
#endif // !KOTOR1

int CSWSPlaceable::GetLightIsOn() {
    int appearance = m_appearanceType;
    C2DA* pTable = g_pRules->m_p2DArrays->m_pPlaceables;
    int outVal;
    int success;
    {
        CExoString col("LightColor");
        success = pTable->GetINTEntry(appearance, col, &outVal);
    }
    return success ? m_lightIsOn : 0;
}

void CSWSPlaceable::SetLightIsOn(int val) {
    if (m_lightIsOn == val) return;
    int appearance = m_appearanceType;
    C2DA* pTable = g_pRules->m_p2DArrays->m_pPlaceables;
    int outVal;
    int success;
    {
        CExoString col("LightColor");
        success = pTable->GetINTEntry(appearance, col, &outVal);
    }
    if (success) {
        m_bLightIsOnDirty = 1;
        m_lightIsOn = val;
    }
}

void CSWSPlaceable::RemoveFromArea() {
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != 0) {
        if (m_staticBBoxId >= 0) {
            GetArea()->RemoveStaticBoundingBox(m_objectId);
            m_staticBBoxId = -1;
        }
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
    }
    SetArea(0);
    Vector zero = {};
    SetPosition(zero, 1, 1, 0);
}

// Attaches this placeable to pArea at vPosition: detaches from any current area, sets the new area
// pointer and position, then re-reads the three per-appearance placeables.2da columns that depend on
// the row ("PreciseUse", "Hostile", "ModelName"), stamps the model resref on the embedded walkmesh
// and loads it. When the mesh loads, its world transform is refreshed from m_position and
// Yaw(m_orientation) and the resulting bounding box is registered with the area as a static
// collision volume, whose handle lands in m_staticBBoxId.
//
// The n parameter is unread, and the area is always told 1.
void CSWSPlaceable::AddToArea(CSWSArea* pArea, const Vector& vPosition, int n) {
    K2_UNUSED(n);
    RemoveFromArea();
    if (pArea == 0) return;
    SetArea(pArea);
    SetPosition(vPosition, 0, 1, 0);
    CExoString sModelName;
    C2DA* pTable = g_pRules->m_p2DArrays->m_pPlaceables;
    int appearance = m_appearanceType;
    int outVal;
    {
        CExoString col("PreciseUse");
        pTable->GetINTEntry(appearance, col, &outVal);
    }
    m_bPreciseUse = (outVal == 1);
    pTable = g_pRules->m_p2DArrays->m_pPlaceables;
    appearance = m_appearanceType;
    int outVal2;
    {
        CExoString col("Hostile");
        pTable->GetINTEntry(appearance, col, &outVal2);
    }
    m_hostileState = (outVal2 == 1);
    pTable = g_pRules->m_p2DArrays->m_pPlaceables;
    appearance = m_appearanceType;
    {
        CExoString col("ModelName");
        pTable->GetCExoStringEntry(appearance, col, &sModelName);
    }
    const CResRef& rModel = CResRef(sModelName);
    m_walkMesh.SetMesh(rModel);
    if (m_walkMesh.LoadMesh(0) != 0) {
        Vector vMin;
        Vector vMax;
        m_walkMesh.m_translation = m_position;
        m_walkMesh.m_rotation =
            Quaternion(Vector{0.0f, 0.0f, 1.0f}, Yaw(CSWSObject::m_orientation) * 0.017453292f);
        m_walkMesh.GetMeshBoundingBox(vMin, vMax);
        if (RenderPlaceableBBox != 0)
            m_walkMesh.RenderMesh(0, DEBUG_COLOR_RED, DEBUG_COLOR_GREEN);
        m_staticBBoxId = static_cast<CSWArea*>(pArea)->AddStaticBoundingBox(m_objectId, vMin, vMax);
    }
    static_cast<CSWSArea*>(GetArea())->AddObjectToArea(m_objectId, 1);
}

// Closes this container's inventory for playerId. On a real close every item still inside is
// drained back into the closing creature's own repository, and the first time a TSF-locker
// item comes back that way the player gets a one-shot message box carrying the item's
// identified description, latched per item so it only ever shows once. Then the OnClosed script
// event is queued, the close animation plays, and a die-when-empty container that ended up
// empty marks itself un-useable and schedules its own destruction.
void CSWSPlaceable::CloseInventory(unsigned long long playerId, int flag) {
    if (m_bInventoryOpen == 0) return;
    if (flag != 0) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerId);
        if (pPlayer != 0) {
            CSWSCreature* pCreature = pPlayer->GetGameObject()->AsSWSCreature();
            while (m_pItemRepository->ItemListGetCount() != 0) {
                CSWSItem* pItem =
                    m_pItemRepository->ItemListGetItem(m_pItemRepository->ItemListGetCount() - 1);
                m_pItemRepository->RemoveItem(pItem);
#if !KOTOR1  // K1: no datapad-item (locker) handling
                if (pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_DATA_PAD) {
                    if (m_sTag.CompareNoCase("lockertsfinv") != 0) {
                        pItem->m_varTable.SetLocalBoolean(LOCAL_BOOLEAN_DATAPAD_SHOWN, 1);
                    } else if (pItem->m_varTable.GetLocalBoolean(LOCAL_BOOLEAN_DATAPAD_SHOWN) == 0) {
                        pItem->m_varTable.SetLocalBoolean(LOCAL_BOOLEAN_DATAPAD_SHOWN, 1);
                        CExoString sText;
                        pItem->m_descriptionIdentified.GetString(0, &sText, 0);
                        CSWGuiMessageBox* pBox =
                            static_cast<CSWGuiMessageBox*>(g_pAppManager->m_pClientExoApp->GetInGameGui()
                                ->m_pDatapadMessageBox);
                        pBox->SetAllowCancel(0);
                        CSWGuiTextParams& params = pBox->m_lblMessage.m_text.m_params;
                        params.SetAlignment(9);
                        params.SetFont(CResRef("fnt_d16x16"));
                        pBox->SetMessage(sText);
                        pBox->SetCallback(0, 0, 0);
                        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
                        g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, PAUSE_MODE_MESSAGE_BOX, 0);
                    }
                }
#endif
                pCreature->GetItemRepository(1)->AddItem(&pItem, 1, 1, 1);
                if (pItem != 0)
                    pItem->SetPossessor(playerId, 1, 1, OBJECT_INVALID);
            }
        }
    }
    CScriptEvent* pEvent = new CScriptEvent();
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLOSE;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, playerId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    SetAnimation(SERVER_ANIMATION_PLACEABLE_CLOSE);
    m_bInventoryOpen = 0;
    if (m_bHasInventory != 0 && m_bDieWhenEmpty != 0 && m_pItemRepository->ItemListGetCount() == 0) {
        m_bUseable = 0;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, m_objectId, m_objectId, CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
    }
}

void CSWSPlaceable::ClosePlaceableForAllPlayers() {
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

void CSWSPlaceable::DropItemsIntoArea() {
    if (m_bHasInventory == 0) return;
    CItemRepository* repo = m_pItemRepository;
    if (repo == 0) return;
    if (repo->ItemListGetCount() == 0) return;
    float x = m_position.x;
    float y = m_position.y;
    float z = m_position.z;
    unsigned int i = 0;
    do {
        CSWSItem* pItem = repo->ItemListGetItem(i);
        int found = m_pItemRepository->GetItemInRepository(pItem, 0);
        if (found == 1) {
            m_pItemRepository->RemoveItem(pItem);
            pItem->SetPossessor(OBJECT_INVALID, 1, 1, OBJECT_INVALID);
        }
        CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
        pItem->AddToArea(pArea, x, y, z, 1);
        repo = m_pItemRepository;
        i++;
    } while (i < repo->ItemListGetCount());
}

void CSWSPlaceable::OpenInventory(unsigned long long playerId, int flag) {

    if (m_bInventoryOpen != 0) return;
    if (m_bHasInventory == 0) return;

    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    CSWSPlayer* pPlayer;
    if (pGui->m_nGlobalDialogState != 0 ||
        (pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(playerId)) == 0) {
        SetAnimation(SERVER_ANIMATION_PLACEABLE_CLOSE);
        return;
    }

    int hasItems = m_pItemRepository->ItemListGetCount() != 0;
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    pMsg->SendServerToPlayerGuiContainerObject_Open(pPlayer, m_objectId, hasItems);

    CScriptEvent* pEvent = new CScriptEvent();
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_OPEN;

    CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    pAI->AddEventDeltaTime(0, 0, playerId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);

    if (flag != 0) {
        SetAnimation(SERVER_ANIMATION_PLACEABLE_OPEN);
    }
    m_bInventoryOpen = 1;
}

int CSWSPlaceable::GetBodyBagAppearance()
{
    int nAppearance;
    int bGot;
    bGot = g_pRules->m_p2DArrays->m_pBodyBag->GetINTEntry(m_bodyBagAppearanceRow,
                                                          CExoString("Appearance"), &nAppearance);
    if (!bGot) {
        int nBodyBag;
        bGot = g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(m_appearanceType,
                                                                 CExoString("BodyBag"), &nBodyBag);
        if (!bGot)
            nBodyBag = 0;
        bGot = g_pRules->m_p2DArrays->m_pBodyBag->GetINTEntry(nBodyBag, CExoString("Appearance"),
                                                              &nAppearance);
        if (!bGot)
            nAppearance = 3;
    }
#if KOTOR1  // K1 stores the full int, not a byte
    return nAppearance;
#else
    return static_cast<unsigned char>(nAppearance);
#endif
}

// Turns a freshly-constructed placeable into the "body bag" corpse container the area's
// SPAWN_BODY_BAG event drops where a creature died. It is a hardcoded blueprint rather than a
// GFF load, so every field is written from a literal here.
//
// nAppearance is a placeables.2da row: it supplies both the visual appearance and, through
// that table's "StrRef" column, the localized name shown on the container. Row 217 gets
// hardness 1 instead of 5.
#if KOTOR1
int CSWSPlaceable::LoadBodyBag(unsigned char nAppearance) {
#else
int CSWSPlaceable::LoadBodyBag(int nAppearance) {
#endif
    CExoLocString locName;
    CExoString    sUnused;

    C2DA* pTable = g_pRules->m_p2DArrays->m_pPlaceables;
    int nStrRef;
    int bFound;
    {
        CExoString col("StrRef");
        bFound = pTable->GetINTEntry(nAppearance, col, &nStrRef);
    }
    if (bFound == 0)
        nStrRef = 0;
    locName.m_nStrRef = nStrRef;

    SetTemplateResRef(CResRef(""));
    SetTag(CExoString("Body Bag"));
    m_firstName = locName;

    m_bAutoRemoveKey = 0;
    m_nFaction = 1;
    m_bTrapDisarmable = 1;
    m_bTrapDetectable = 1;
    m_baseDisarmDC = 15;
    m_baseDetectDC = 0;
    m_bTrapFlag = 0;
    m_bTrapOneShot = 1;
    m_trapType = 0;
    m_bPlot = 0;
    m_baseOpenLockDC = 18;
#if !KOTOR1
    m_lockDCFormula = 1;
    m_lockDCFlatBase = 0;
#endif

    m_keyName = CExoString("");
    m_appearanceType = nAppearance;
    m_hitPoints = 15;
    SetCurrentHitPoints(15);
    m_hardness = (nAppearance == PLACEABLE_APPEARANCE_RANCOR_CORPSE) ? 1 : 5;
    m_fortSave = 16;
    m_willSave = 0;
    m_refSave = 0;
    m_bLockable = 0;
    m_bLocked = 0;
    m_bUseable = 1;
    m_bHasInventory = 1;
    m_bKeyRequired = 0;
    m_closeLockDC = 0;

    SetPortrait(CResRef("PO_PLC_B04_"));
    SetDialogResref(CResRef(""));

    locName.ClearLocString();
    locName.m_nStrRef = STRREF_CREATURE_REMAINS;
    m_description = locName;

    m_scriptsOn[ON_SCRIPT_TRAP_TRIGGERED] = CExoString("");
    m_scriptsOn[ON_SCRIPT_CLOSED]        = CExoString("");
    m_scriptsOn[ON_SCRIPT_DAMAGED]       = CExoString("");
    m_scriptsOn[ON_SCRIPT_DEATH]         = CExoString("");
    m_scriptsOn[ON_SCRIPT_DISARM]        = CExoString("");
    m_scriptsOn[ON_SCRIPT_HEARTBEAT]       = CExoString("");
    m_scriptsOn[ON_SCRIPT_INV_DISTURBED]  = CExoString("");
    m_scriptsOn[ON_SCRIPT_LOCK]          = CExoString("");
    m_scriptsOn[ON_SCRIPT_MELEE_ATTACKED] = CExoString("");
    m_scriptsOn[ON_SCRIPT_OPEN]          = CExoString("");
    m_scriptsOn[ON_SCRIPT_SPELL_CAST_AT]   = CExoString("");
    m_scriptsOn[ON_SCRIPT_UNLOCK]        = CExoString("");
    m_scriptsOn[ON_SCRIPT_USED]          = CExoString("");
    m_scriptsOn[ON_SCRIPT_USER_DEFINED]   = CExoString("");
    m_scriptsOn[ON_SCRIPT_DIALOG]        = CExoString("");
    m_scriptsOn[ON_SCRIPT_END_DIALOGUE]   = CExoString("");

    m_bStatic = 0;
    SetAnimation(SERVER_ANIMATION_PAUSE);
    g_pAppManager->m_pServerExoApp->GetModule()->AddObjectToLookupTable(m_sTag, m_objectId);
    PostProcess();
    return 1;
}

void CSWSPlaceable::ApplyComputerSpikeBonus(unsigned long long objectId) {
    // Every bail path (no item, wrong base type, no such property) leaves `bonus` unassigned, so the
    // shared store writes back whatever the last call left behind: the base-item type, or the
    // passive-property count.
    int bonus;
#ifdef K2_FIX_ENGINE_BUGS
    bonus = 0;
#endif
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(objectId);
    if (pItem != 0 && static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_SPIKES
        && pItem->GetPropertyByTypeExists(ITEM_PROPERTY_COMPUTER_SPIKE, 0)) {
        CSWItemProperty* pProp;
        pItem->GetPropertyByType(&pProp, ITEM_PROPERTY_COMPUTER_SPIKE, 0);
        bonus = 0;
        if (pProp != 0) {
            bonus = pProp->m_costTableValue;
        }
    }
    m_computerSpikeBonus = bonus;
}

void CSWSPlaceable::PostProcess() {
    if (m_hardness == 0) return;
    m_bApplyingResistanceEffect = 1;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) |
                                   CGameEffect::DURATION_TYPE_PERMANENT;
    pEffect->SetInteger(0, CSWSEffectListHandler::DAMAGE_FLAG_ALL);
    pEffect->SetInteger(1, m_hardness);
    pEffect->m_bIsExposed = 0;
    ApplyEffect(pEffect, 0, 0);
    m_bApplyingResistanceEffect = 0;
}

int CSWSPlaceable::LoadFromTemplate(const CResRef& resRef, int flag) {
    CResGFF* pGff = new CResGFF(RESTYPE_UTP, const_cast<char*>("UTP "), resRef);
    CResStruct topStruct;
    int result;
    if (pGff->m_nValid != 0) {
        pGff->GetTopLevelStruct(&topStruct);
        LoadPlaceable(pGff, &topStruct, flag);
        delete pGff;
        result = 1;
    } else {
        delete pGff;
        CExoString msg;
        CExoString resRefStr;
        resRef.CopyToString(resRefStr);
        msg.Format(const_cast<char*>("Item template %s doesn't exist.\n"), resRefStr.CStr());
        result = 0;
    }
    return result;
}

Vector CSWSPlaceable::GetNearestActionPoint(const Vector& target) {
    Vector zeroCheck1 = m_walkMesh.LocalToWorld(m_walkMesh.m_headerVec[0]);
    if (zeroCheck1.x == 0.0f && zeroCheck1.y == 0.0f && zeroCheck1.z == 0.0f) {
        return m_position;
    }

    Vector point0 = m_walkMesh.LocalToWorld(m_walkMesh.m_headerVec[0]);
    Vector vDiff0 = target - point0;
    Vector point1 = m_walkMesh.LocalToWorld(m_walkMesh.m_headerVec[1]);
    Vector vDiff1 = target - point1;
    Vector zeroCheck2 = m_walkMesh.LocalToWorld(m_walkMesh.m_headerVec[1]);

    if (zeroCheck2.x != 0.0f || zeroCheck2.y != 0.0f || zeroCheck2.z != 0.0f) {
        float dist0 = vDiff0.x * vDiff0.x + vDiff0.y * vDiff0.y + vDiff0.z * vDiff0.z;
        float dist1 = vDiff1.x * vDiff1.x + vDiff1.y * vDiff1.y + vDiff1.z * vDiff1.z;
        // A tie, and a NaN, picks B rather than A.
        if (!(dist0 < dist1))
            return m_walkMesh.LocalToWorld(m_walkMesh.m_headerVec[1]);
    }
    return m_walkMesh.LocalToWorld(m_walkMesh.m_headerVec[0]);
}

void CSWSPlaceable::SetOrientation(const Quaternion& q) {
    m_orientation = q;
    Vector dir = q.ydir();
    dir.z = 0.0f;
    dir = normalize(dir);
    CSWSObject::SetOrientation(dir);
}

int CSWSPlaceable::AddCastSpellActions(unsigned int spellId, int casterLevel, const Vector& targetLoc,
                                         unsigned long long targetId, int metaFlag, unsigned char pathType) {
    unsigned int spell = spellId;
    int level = casterLevel;
    unsigned long long id = targetId;
    Vector loc = targetLoc;
    if (targetId != OBJECT_INVALID) {
        CGameObject* pObj = static_cast<CGameObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(targetId));
        if (pObj == 0) return 0;
        CSWSObject* pTarget = pObj->AsSWSObject();
        if (pTarget != 0) {
            if (pTarget->GetArea() != GetArea()) return 0;
            loc = pTarget->m_position;
        }
    }
    unsigned int flags = pathType;
    if (metaFlag != 0) flags |= CAST_FLAG_METAMAGIC;
    AddAction(AIACTION_CAST_SPELL, ACTION_GROUP_NEW, ACTION_PARAM_INT, &spell,
        ACTION_PARAM_INT, &level, ACTION_PARAM_OBJECT, &id, ACTION_PARAM_FLOAT, &loc,
        ACTION_PARAM_FLOAT, reinterpret_cast<void*>(reinterpret_cast<unsigned long long>(&loc) | 4),
        ACTION_PARAM_FLOAT, &loc.z, ACTION_PARAM_INT, &flags, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

int CSWSPlaceable::AIActionCastSpell(CSWSObjectActionNode* pAction) {
    Vector castLoc = {};
    if (GetDead() != 1) {
        unsigned int spellId = pAction->m_args.m_spellId;
        unsigned long long targetId = pAction->m_args.m_targetId;
        float fX = pAction->m_args.m_locX;
        float fY = pAction->m_args.m_locY;
        float fZ = pAction->m_args.m_locZ;
        unsigned int flags = pAction->m_args.m_flags;
        castLoc.x = fX;
        castLoc.y = fY;
        castLoc.z = fZ;
        unsigned char pathCount = (flags & 0xff) != 0 ? 7 : 6;
        if (g_pRules->GetSpell(spellId) != 0) {
            m_castSpellId = spellId;
            m_castTargetId = targetId;
            m_castPosition = castLoc;
            m_bCastBroadcastProjectile = 0;
#if !KOTOR1  // K1: no remembered force-point cost
            m_nLastSpellForcePointCost = 0;
#endif
            CalculateLastSpellProjectileTime(static_cast<unsigned char>(flags));
            if (static_cast<int>(flags) < 0) {
                CGameObject* pObj = static_cast<CGameObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(targetId));
                CSWSObject* pTarget = pObj != 0 ? pObj->AsSWSObject() : 0;
                if (pTarget != 0)
                    BroadcastSafeProjectile(m_objectId, targetId, m_position, *static_cast<Vector*>(&pTarget->m_position),
                        m_castLevel, pathCount, spellId, 0, static_cast<unsigned char>(flags), 0, 0);
                else
                    BroadcastSafeProjectile(m_objectId, targetId, m_position, m_castPosition,
                        m_castLevel, pathCount, spellId, 0, static_cast<unsigned char>(flags), 0, 0);
                m_bCastBroadcastProjectile = 1;
            } else {
                SpellCastAndImpact(spellId, castLoc, targetId, 0xff,
                    CGameObjectArray::INVALIDOBJID, 0, 0, static_cast<unsigned char>(flags), 0);
            }
            return CSWSObject::ACTION_COMPLETE;
        }
    }
    return CSWSObject::ACTION_FAILED;
}

void CSWSPlaceable::AIUpdate() {
    unsigned long long timerSnapshot = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    unsigned int day, tod;
    static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer())->GetWorldTime(&day, &tod);
    unsigned int elapsedDay, elapsedTod;
    static_cast<CWorldTimer*>(g_pAppManager->m_pServerExoApp->GetWorldTimer())->SubtractWorldTimes(
        day, tod, m_lastUpdateDay, m_lastUpdateTod, &elapsedDay, &elapsedTod);
    if ((elapsedTod >> 4) >= HEARTBEAT_INTERVAL_16MS) {
        if (m_lastUpdateTod != 0 && GetDead() == 0) {
#if !KOTOR1  // K1: no PC-level-at-spawn snapshot
            if (m_nPCLevelAtSpawn == 0) {
                char level = 0;
                CSWGlobalVariableTable* pTable = g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
                {
                    CExoString name("G_PC_LEVEL");
                    pTable->GetValueNumber(name, level);
                }
                m_nPCLevelAtSpawn = level;
            }
#endif
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_HEARTBEAT], m_objectId, 1);
        }
        m_lastUpdateDay = day;
        m_lastUpdateTod = tod;
#if KOTOR1  // K1: no immediate first heartbeat
    }
#else
    } else {
        if (m_lastUpdateTod == 0 && GetDead() == 0) {
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_HEARTBEAT], m_objectId, 1);
            m_lastUpdateTod = 1;
        }
    }
#endif
    UpdateEffectList(day, tod);
    RunActions(day, tod, timerSnapshot);
}

// Dispatches the placeable's script events through a flat if/else chain over the AI master's
// event ids, whose SIGNAL_EVENT arm fans out again over the CScriptEvent sub-type into the
// placeable's script slots. It is the CSWSDoor dispatcher's twin, with ACQUIRE_ITEM,
// REMOVE_EFFECT and SPELL_IMPACT arms the door has no use for, and without the door's
// area-transition OnClick arm.
//
// Three engine defects are preserved here: the ON_MELEE_ATTACKED weapon-slot switch dereferences
// the attacker without the null check the surrounding code has; the trap-trigger arm's faction
// compare dereferences the tripper after only a null-checked immunity test; and the SPELL_IMPACT
// payload is freed with a bare operator delete and no destructor, leaking its CExoString on every
// impact. pAttacker is also left uninitialized in the ON_MELEE_ATTACKED arm.
void CSWSPlaceable::EventHandler(unsigned int nEventType, unsigned long long nCallerId, void* pData,
                                 unsigned int a4, unsigned int a5) {
    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_FEEDBACK_MESSAGE)) {
        CSWCCMessageData* pMsg = static_cast<CSWCCMessageData*>(pData);
        if (pMsg == 0) return;
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
        if (pMsg->m_nType == CSWCCMessageData::MESSAGE_TYPE_SAVING_THROW) {
            if (pCreature != 0) {
                pCreature->BroadcastSavingThrowData(pMsg);
            }
        } else if (pMsg->m_nType == CSWCCMessageData::MESSAGE_TYPE_FEEDBACK_TO_CALLER) {
            if (pCreature != 0) {
                // The message is handed to the creature and not destroyed on this path.
                pCreature->SendFeedbackMessage(static_cast<unsigned short>(pMsg->GetInteger(0)), pMsg);
                return;
            }
        } else {
            if (pCreature != 0) {
                pCreature->SendFeedbackMessage(static_cast<unsigned short>(pMsg->GetInteger(0)), pMsg);
                return;
            }
        }
        pMsg->~CSWCCMessageData();
        operator delete(pMsg);
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_ON_MELEE_ATTACKED)) {
        CSWSItem* pWeapon = 0;
        CSWSCombatAttackData* pAttack = static_cast<CSWSCombatAttackData*>(pData);
        // Not initialized, and that is the engine's own behaviour: on the pAttack == 0 path
        // execution falls straight through to the pAttacker test with a stale value, so a
        // bogus pointer reaches BroadcastCombatStateToParty. The door's twin writes `= 0` here.
        CSWSCreature* pAttacker;
#ifdef K2_FIX_ENGINE_BUGS
        pAttacker = 0;
#endif
        if (pAttack != 0) {
            CSWCCMessageData* pMsg = new CSWCCMessageData();
            pMsg->SetObjectID(0, nCallerId);
            pMsg->SetObjectID(1, m_objectId);
            if (g_bEnableCombatDebugging) {
                pMsg->SetString(0, pAttack->m_sAttackDebugText);
            }
            pAttacker = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
            if (pAttacker != 0) {
                pAttacker->BroadcastAttackDataToParty(pMsg, pAttack);
                pAttacker->RemoveCombatInvisibilityEffects();
            }
            pMsg->~CSWCCMessageData();
            operator delete(pMsg);
            m_nLastAttackFeatId = pAttack->m_nFeatId;
            m_nLastAttackMode = pAttack->m_nAttackMode;
            // pAttacker is not re-checked here.
#ifdef K2_FIX_ENGINE_BUGS
            if (pAttacker != 0)
#endif
            switch (pAttack->m_nWeaponAttackType) {
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON1:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_L));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON2:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_R));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON3:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_B));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_HANDS));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_HANDS));
                break;
            default: break;
            }
        }
        m_oidLastAttacker = nCallerId;
        SetLastHostileActor(nCallerId, 0);
        if (pAttacker != 0) {
            pAttacker->BroadcastCombatStateToParty(OBJECT_INVALID);
            pAttacker->RemoveCombatInvisibilityEffects();
            if (pWeapon != 0) {
                pAttacker->m_oidLastWeaponUsed = pWeapon->m_objectId;
            } else {
                pAttacker->m_oidLastWeaponUsed = OBJECT_INVALID;
            }
        }
        g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_MELEE_ATTACKED], m_objectId, 1);
        // A trapped placeable struck from within 4 metres arms itself: schedule the
        // trap-trigger event back at the attacker on the next AI tick.
        if (pAttacker != 0 && m_bTrapFlag == 1) {
            Vector vToAttacker = pAttacker->m_position - m_position;
            if (vToAttacker.x * vToAttacker.x + vToAttacker.y * vToAttacker.y +
                    vToAttacker.z * vToAttacker.z <=
                16.0f) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, nCallerId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_ACQUIRE_ITEM)) {
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(nCallerId);
        if (pItem == 0) return;
        AcquireItem(&pItem, OBJECT_INVALID, 1);
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_SIGNAL_EVENT)) {
        CScriptEvent* pEvent = static_cast<CScriptEvent*>(pData);
        switch (pEvent->m_eventType) {
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_HEARTBEAT:
            m_lastUpdateDay = a4;
            m_lastUpdateTod = a5;
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_HEARTBEAT], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_SPELLCASTAT:
            m_oidLastSpellCaster = pEvent->GetObjectID(0);
            m_nLastSpellId = pEvent->GetInteger(0);
            m_bLastSpellHarmful = pEvent->GetInteger(1);
            if (m_bLastSpellHarmful != 0) {
                SetLastHostileActor(m_oidLastSpellCaster, 0);
                CSWSCreature* pCaster =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidLastSpellCaster);
                if (pCaster != 0) {
                    pCaster->BroadcastCombatStateToParty(OBJECT_INVALID);
                    pCaster->SetExcitedState(2);
                    pCaster->RemoveCombatInvisibilityEffects();
                }
            }
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_SPELL_CAST_AT], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_DAMAGED:
            m_lastDamagerId = nCallerId;
            SetLastHostileActor(nCallerId, 0);
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_DAMAGED], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_DIALOGUE: {
            m_oidLastSpeaker = nCallerId;
            m_nListenPatternNumber = pEvent->GetInteger(1);
            m_sLastConversation = pEvent->GetString(0);
            g_pAppManager->m_pServerExoApp->SetLastConversation(pEvent->GetString(0));
            m_bPrivateConversation = pEvent->GetInteger(2);
            g_pAppManager->m_pServerExoApp->SetActionConversation(1);
            g_pAppManager->m_pServerExoApp->SetIgnoreConversationStartRange(pEvent->GetInteger(3));
            ClearMatchedExpressionStrings();
            for (int i = 1; i < pEvent->GetInteger(0); i++) {
                AddMatchedExpressionString(pEvent->GetString(i));
            }
            // Unlike the door's twin, the placeable also treats an absent or empty script name
            // as unset: three short-circuited disjuncts, in this order.
            if (m_scriptsOn[ON_SCRIPT_DIALOG].IsEmpty() || m_scriptsOn[ON_SCRIPT_DIALOG] == "default") {
                m_scriptsOn[ON_SCRIPT_DIALOG] = "k_hen_dialogue01";
            }
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_DIALOG], m_objectId, 1);
            break;
        }
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_DEATH:
            SetAnimation(SERVER_ANIMATION_DESTROYED);
            m_killerId = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_DEATH], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT:
            m_nUserDefinedEventNumber = pEvent->GetInteger(0);
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_USER_DEFINED], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_OPEN:
            m_oidLastOpenedBy = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_OPEN], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_CLOSE:
            m_oidLastClosedBy = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_CLOSED], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM:
            if (m_bTrapFlag != 0) {
                m_bTrapFlag = 0;
                m_oidLastDisarmedBy = nCallerId;
                g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_DISARM], m_objectId, 1);
            }
            if (m_objectsOnPlaceable.m_count > 0) {
                m_objectsOnPlaceable.SetSize(0);
            }
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_USED:  // Only fires while the placeable is interactable
            if (m_bUseable != 0) {
                m_oidLastUsedBy = nCallerId;
                g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_USED], m_objectId, 1);
            }
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED: {  // the open-lock / trap-trigger attempt
#if KOTOR1  // K1: no lock-pick arm, only the trap trigger
            int nUnused = pEvent->GetInteger(0);
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
            {
#else
            int nUnused = pEvent->GetInteger(0);
            int nInt1 = pEvent->GetInteger(1);
            int nInt2 = pEvent->GetInteger(2);
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
            if (nInt1 == 0 && nInt2 != 0) {
                // Security-skill lock pick.
                if (m_bTrapFlag == 0) break;
                // Divisible by 20, not by 2 as the door's shape invites, because the running total
                // is truncated to a signed char before the event's own bonus is added.
                signed char nBase =
                    static_cast<signed char>(pCreature->m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0) / 20 + 20);
                int nCheck = pEvent->GetInteger(3) + nBase;
                if (nCheck >= GetOpenLockDC()) {
                    m_bLocked = 0;
                    CScriptEvent* pUnlockEvent = new CScriptEvent();
                    pUnlockEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_UNLOCKED;
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, pCreature->m_objectId, m_objectId, nEventType, pUnlockEvent);
                }
                pCreature->SendFeedbackMessage(FEEDBACK_TRIGGERED_MINE, 0);
                m_oidEnteringObject = nCallerId;
                m_bTrapFlag = 0;
                if (GetArea() != 0) {
                    if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(m_objectId)) {
                        static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(m_objectId);
                    }
                }
                if (m_objectsOnPlaceable.m_count > 0) {
                    m_objectsOnPlaceable.SetSize(0);
                }
                // Cached in a local, not re-read, since the trigger is held across
                // every call here, which a bare member spelling cannot do.
#if !KOTOR1  // K1: no owned trap trigger
                CSWSTrigger* pOwnedTrigger = m_pOwnedTrigger;
                if (pOwnedTrigger != 0) {
                    // Only the three "volume" trap categories get sunk 2m before removal, so the
                    // fade-out plays under the floor rather than through the placeable.
                    if (static_cast<unsigned>(pOwnedTrigger->m_trapType - 3) <= 2) {
                        CSWCObject* pClientTrigger = static_cast<CSWCObject*>(pOwnedTrigger->GetClientTrigger());
                        Vector vSunk = pClientTrigger->m_position;
                        vSunk.z = vSunk.z + -2.0f;
                        pClientTrigger->SetPosition(vSunk);
                    }
                    pOwnedTrigger->SetFadeOutTime(1);
                    pOwnedTrigger->RemoveFromArea();
                    delete pOwnedTrigger;
                }
#endif
            } else {
#endif
                // Trap trigger.
                if (pCreature != 0) {
                    if (pCreature->m_pStats->GetEffectImmunity(
                            static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_MINE), 0) != 0) {
                        break;
                    }
                }
                if (nUnused == 0) {
                    // pCreature is dereferenced below without the null check just above.
                    if (GetReputation(nCallerId, 0) > CFactionManager::REPUTATION_FRIENDLY_ABOVE) break;
#ifdef K2_FIX_ENGINE_BUGS
                    if (pCreature != 0 &&
                        m_nFaction == static_cast<unsigned int>(pCreature->m_pStats->m_factionId)) break;
#else
                    if (m_nFaction == static_cast<unsigned int>(pCreature->m_pStats->m_factionId)) break;
#endif
                }
                if (m_bTrapFlag == 0) break;
                if (pCreature != 0) {
                    pCreature->SendFeedbackMessage(FEEDBACK_TRIGGERED_MINE, 0);
                }
                m_oidEnteringObject = nCallerId;
                g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_TRAP_TRIGGERED], m_objectId, 1);
                if (m_bTrapOneShot != 0) {
                    m_bTrapFlag = 0;
#if KOTOR1  // K1: one more (discarded) GetArea() and a == 1 count test
                    if (GetArea() != 0) {
                        GetArea();
                        if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(m_objectId) == 1) {
                            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(m_objectId);
                        }
                    }
#else
                    if (GetArea() != 0) {
                        if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(m_objectId)) {
                            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(m_objectId);
                        }
                    }
#endif
#if !KOTOR1
                    CSWSTrigger* pOwnedTrigger = m_pOwnedTrigger;
                    if (pOwnedTrigger != 0) {
                        pOwnedTrigger->SetFadeOutTime(1);
                        pOwnedTrigger->RemoveFromArea();
                        delete pOwnedTrigger;
                    }
#endif
                }
                if (m_objectsOnPlaceable.m_count > 0) {
                    m_objectsOnPlaceable.SetSize(0);
                }
            }
            break;
        }
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_INVENTORY_DISTURBED:
            m_oidLastDisturbedBy = nCallerId;
            m_nLastDisturbType = pEvent->GetInteger(0);
            m_oidLastDisturbItem = pEvent->GetObjectID(0);
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_INV_DISTURBED], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_LOCKED:
            m_oidLastLockedBy = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_LOCK], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_UNLOCKED:
            m_oidLastUnlockedBy = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_UNLOCK], m_objectId, 1);
            break;
#if !KOTOR1  // K1: no OnFailToOpen
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN:
            m_oidEnteringObject = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptsOn[ON_SCRIPT_FAIL_TO_OPEN], m_objectId, 1);
            break;
#endif
        default:
            break;
        }
        delete pEvent;
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_DESTROY_OBJECT)) {
        ClosePlaceableForAllPlayers();
        SpawnBodyBag();
        RemoveFromArea();
        delete this;
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_REMOVE_FROM_AREA)) {
        RemoveFromArea();
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_APPLY_EFFECT)) {
        if (pData != 0) {
            ApplyEffect(static_cast<CGameEffect*>(pData), 0, 0);
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_REMOVE_EFFECT)) {
        if (pData != 0) {
            RemoveEffectById(*static_cast<unsigned long long*>(pData));
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_PLAY_ANIMATION)) {
        if (pData != 0 && GetDead() == 0) {
            SetAnimation(static_cast<int>(reinterpret_cast<uintptr_t>(pData)));
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_TIMED_EVENT)) {
        if (pData != 0) {
            g_pVirtualMachine->RunScriptSituation(pData, m_objectId, 1);
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_SPELL_IMPACT)) {
        CSWSSpellScriptData* pImpact = static_cast<CSWSSpellScriptData*>(pData);
        if (pImpact == 0) return;
        // Late-arriving impacts whose caster is no longer the event's caller are dropped.
        if (pImpact->m_casterId == nCallerId) {
            m_castSpellId = pImpact->m_nSpellId;
            m_castTargetId = pImpact->m_targetId;
            m_castPosition = pImpact->m_targetPosition;
#if !KOTOR1
            m_nLastSpellForcePointCost = pImpact->m_nFinalForceCost;
#endif
            // If the impact named a live target sharing this placeable's area, retarget the cast
            // position at where that object actually is now.
            if (pImpact->m_targetId != OBJECT_INVALID) {
                CGameObject* pTarget;
                if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                        pImpact->m_targetId, &pTarget) == CGameObjectArray::SUCCESS &&
                    pTarget->m_objectType > CGameObject::OBJECT_TYPE_AREA &&
                    GetArea() == static_cast<CSWSObject*>(pTarget)->GetArea()) {
                    m_castPosition = static_cast<CSWSObject*>(pTarget)->m_position;
                }
            }
            SetEffectSpellId(pImpact->m_nSpellId);
            m_nImpactSpellId = pImpact->m_nSpellId;
            g_pVirtualMachine->RunScript(&pImpact->m_sScript, m_objectId, 1);
            SetEffectSpellId(SPELL_ID_NONE);
        }
        // No destructor, so the payload's string leaks.
        operator delete(pImpact);
    } else if (pData != 0 && nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_OPEN_OBJECT)) {
        OpenInventory(*static_cast<unsigned long long*>(pData), 0);
        operator delete(pData);
    }
}

// Writes the placeable's whole persistent state back out as GFF fields, the paired writer for
// LoadPlaceable. Mostly a flat run of WriteField* calls in source order; the real logic is the
// two difficulty-scaled trap DCs, the "LightState" 2DA gate, the inventory ItemList and the
// bearing, which is recomputed on save.
int CSWSPlaceable::SavePlaceable(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Tag"));
    pGFF->WriteFieldCExoLocString(pStruct, GetFirstName(), const_cast<char*>("LocName"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bAutoRemoveKey), const_cast<char*>("AutoRemoveKey"));
    pGFF->WriteFieldDWORD(pStruct, m_nFaction, const_cast<char*>("Faction"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bPlot), const_cast<char*>("Plot"));
#if !KOTOR1  // K1: no NotBlastable / PCLevelAtSpawn
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bNotBlastable), const_cast<char*>("NotBlastable"));
#endif
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bMin1HP), const_cast<char*>("Min1HP"));
    pGFF->WriteFieldBYTE(pStruct, m_baseOpenLockDC, const_cast<char*>("OpenLockDC"));
#if !KOTOR1
    pGFF->WriteFieldBYTE(pStruct, m_lockDCFormula, const_cast<char*>("OpenLockDiff"));
    pGFF->WriteFieldCHAR(pStruct, m_lockDCFlatBase, const_cast<char*>("OpenLockDiffMod"));
#endif
    pGFF->WriteFieldCExoString(pStruct, m_keyName, const_cast<char*>("KeyName"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapDisarmable), const_cast<char*>("TrapDisarmable"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapDetectable), const_cast<char*>("TrapDetectable"));

    // Both trap DCs are saved difficulty-adjusted (Easy -5 / Difficult +5) and clamped to a
    // minimum of 1, the same adjustment the getters apply on read. Unlike CSWSDoor::SaveDoor,
    // whose detect clamp is a signed test, both clamps here are the unsigned-byte == 0 form.
#if KOTOR1  // K1 stores the raw base DCs
    pGFF->WriteFieldBYTE(pStruct, m_baseDisarmDC, const_cast<char*>("DisarmDC"));
    pGFF->WriteFieldBYTE(pStruct, m_baseDetectDC, const_cast<char*>("TrapDetectDC"));
#else
    unsigned char baseDisarm = m_baseDisarmDC;
    unsigned char diffDisarm = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adjDisarm = (diffDisarm == 0) ? -5 : ((diffDisarm == 2) ? 5 : 0);
    unsigned char disarmDC = static_cast<unsigned char>(adjDisarm + baseDisarm);
    if (disarmDC == 0) disarmDC = 1;
    pGFF->WriteFieldBYTE(pStruct, disarmDC, const_cast<char*>("DisarmDC"));

    unsigned char baseDetect = m_baseDetectDC;
    unsigned char diffDetect = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adjDetect = (diffDetect == 0) ? -5 : ((diffDetect == 2) ? 5 : 0);
    unsigned char detectDC = static_cast<unsigned char>(adjDetect + baseDetect);
    if (detectDC == 0) detectDC = 1;
    pGFF->WriteFieldBYTE(pStruct, detectDC, const_cast<char*>("TrapDetectDC"));
#endif

    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapFlag), const_cast<char*>("TrapFlag"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapOneShot), const_cast<char*>("TrapOneShot"));
    pGFF->WriteFieldBYTE(pStruct, m_trapType, const_cast<char*>("TrapType"));
#if !KOTOR1
    pGFF->WriteFieldINT(pStruct, m_nOwnerDemolitionsSkill, const_cast<char*>("OwnerDemolitionsSkill"));
#endif
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bUseable), const_cast<char*>("Useable"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bStatic), const_cast<char*>("Static"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bGroundPile), const_cast<char*>("GroundPile"));
    pGFF->WriteFieldDWORD(pStruct, m_appearanceType, const_cast<char*>("Appearance"));
#if !KOTOR1
    pGFF->WriteFieldBYTE(pStruct, m_bUseTweakColor, const_cast<char*>("UseTweakColor"));
    pGFF->WriteFieldDWORD(pStruct, m_tweakColor, const_cast<char*>("TweakColor"));
#endif
    pGFF->WriteFieldSHORT(pStruct, static_cast<short>(m_hitPoints), const_cast<char*>("HP"));
    pGFF->WriteFieldSHORT(pStruct, GetCurrentHitPoints(1), const_cast<char*>("CurrentHP"));
    pGFF->WriteFieldBYTE(pStruct, m_hardness, const_cast<char*>("Hardness"));
    pGFF->WriteFieldBYTE(pStruct, m_fortSave, const_cast<char*>("Fort"));
    pGFF->WriteFieldBYTE(pStruct, m_willSave, const_cast<char*>("Will"));
    pGFF->WriteFieldBYTE(pStruct, m_refSave, const_cast<char*>("Ref"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bLockable), const_cast<char*>("Lockable"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bLocked), const_cast<char*>("Locked"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bHasInventory), const_cast<char*>("HasInventory"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bKeyRequired), const_cast<char*>("KeyRequired"));
    pGFF->WriteFieldBYTE(pStruct, m_closeLockDC, const_cast<char*>("CloseLockDC"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bInventoryOpen), const_cast<char*>("Open"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bPartyInteract), const_cast<char*>("PartyInteract"));

    // The portrait is the 16-bit row id if the object has one, else its fallback resref. Both go
    // through the base's virtuals, GetPortraitId and GetPortrait, rather than reading the fields
    // directly.
    unsigned short portraitId = GetPortraitId();
    if (portraitId != PORTRAIT_ID_NONE) {
        pGFF->WriteFieldWORD(pStruct, GetPortraitId(), const_cast<char*>("PortraitId"));
    } else {
        pGFF->WriteFieldCResRef(pStruct, GetPortrait(), const_cast<char*>("Portrait"));
    }

    pGFF->WriteFieldCResRef(pStruct, GetDialogResref(), const_cast<char*>("Conversation"));
    pGFF->WriteFieldBYTE(pStruct, m_bodyBagAppearanceRow, const_cast<char*>("BodyBag"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bDieWhenEmpty), const_cast<char*>("DieWhenEmpty"));

    // "LightState" is only meaningful for an appearance that actually has a light: the 2DA lookup
    // is a pure existence gate (its out-value is discarded), exactly as in GetLightIsOn/SetLightIsOn.
    C2DA* pTable = g_pRules->m_p2DArrays->m_pPlaceables;
    int appearance = m_appearanceType;
    int hasLight;
    {
        int outVal;
        CExoString col("LightColor");
        hasLight = pTable->GetINTEntry(appearance, col, &outVal);
    }
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(hasLight ? m_lightIsOn : 0), const_cast<char*>("LightState"));

    pGFF->WriteFieldCExoLocString(pStruct, m_description, const_cast<char*>("Description"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_CLOSED]), const_cast<char*>("OnClosed"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_DAMAGED]), const_cast<char*>("OnDamaged"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_DEATH]), const_cast<char*>("OnDeath"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_DISARM]), const_cast<char*>("OnDisarm"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_HEARTBEAT]), const_cast<char*>("OnHeartbeat"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_INV_DISTURBED]), const_cast<char*>("OnInvDisturbed"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_LOCK]), const_cast<char*>("OnLock"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_MELEE_ATTACKED]), const_cast<char*>("OnMeleeAttacked"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_OPEN]), const_cast<char*>("OnOpen"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_SPELL_CAST_AT]), const_cast<char*>("OnSpellCastAt"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_UNLOCK]), const_cast<char*>("OnUnlock"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_USED]), const_cast<char*>("OnUsed"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_USER_DEFINED]), const_cast<char*>("OnUserDefined"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_DIALOG]), const_cast<char*>("OnDialog"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_END_DIALOGUE]), const_cast<char*>("OnEndDialogue"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_TRAP_TRIGGERED]), const_cast<char*>("OnTrapTriggered"));
#if !KOTOR1
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptsOn[ON_SCRIPT_FAIL_TO_OPEN]), const_cast<char*>("OnFailToOpen"));
#endif
    pGFF->WriteFieldINT(pStruct, m_nAnimation, const_cast<char*>("Animation"));

    // The inventory is only emitted when the repository actually holds something. An empty
    // placeable gets no "ItemList" field at all, not an empty list.
    if (m_pItemRepository->ItemListGetCount() != 0) {
        CResList list;
        pGFF->AddList(&list, pStruct, const_cast<char*>("ItemList"));
        for (unsigned int i = 0; i < m_pItemRepository->ItemListGetCount(); i++) {
            CResStruct elem;
            pGFF->AddListElement(&elem, &list, 0);
            CSWSItem* pItem = m_pItemRepository->ItemListGetItem(i);
            pGFF->WriteFieldDWORD(&elem, static_cast<unsigned int>(pItem->m_objectId), const_cast<char*>("ObjectId"));
            pItem->SaveItem(pGFF, &elem);
        }
    }

    // Bearing isn't maintained live -- it's recomputed from the live orientation on the way out
    // (Yaw() returns degrees; the stored/saved field is radians) and written back to the field.
    float bearing = Yaw(CSWSObject::m_orientation) * 0.017453292f;
    m_bearing = bearing;
    pGFF->WriteFieldFLOAT(pStruct, bearing, const_cast<char*>("Bearing"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.x, const_cast<char*>("X"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.y, const_cast<char*>("Y"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.z, const_cast<char*>("Z"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bIsBodyBag), const_cast<char*>("IsBodyBag"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bIsBodyBagVisible), const_cast<char*>("IsBodyBagVisible"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_corpse.m_bIsCorpse), const_cast<char*>("IsCorpse"));
#if !KOTOR1
    pGFF->WriteFieldBYTE(pStruct, m_nPCLevelAtSpawn, const_cast<char*>("PCLevelAtSpawn"));
#endif
    return 1;
}

// The LoadPlaceable half of SavePlaceable. As a field-naming oracle the reader is independent
// of the writer: it names the template resref field outright ("TemplateResRef") and carries three fields the
// writer cannot show at all, the "Invulnerable" legacy label, "AnimationState" and
// "InventoryRes".
int CSWSPlaceable::LoadPlaceable(CResGFF* pGFF, CResStruct* pStruct, int bFromTemplate)
{
    int nAnim;
    int bSuccess;

    // One shared write-temp for all seventeen script slots (the same idiom CSWSTrigger::
    // LoadTrigger uses), default-constructed first and destroyed last.
    CExoString sScript;

    SetTag(pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), bSuccess,
                                     CExoString(const_cast<char*>(""))));
    SetTemplateResRef(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("TemplateResRef"), bSuccess,
                                             CResRef("")));
    m_firstName = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LocName"), bSuccess,
                                                CExoLocString());
    m_bAutoRemoveKey = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("AutoRemoveKey"), bSuccess, 0);
    m_nFaction = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Faction"), bSuccess, 0);

    // The plot flag is read under its legacy label first. KOTOR 1 spelled it "Invulnerable", KOTOR
    // 2 spells it "Plot", which is the only label SavePlaceable ever writes. A blueprint that
    // carries neither leaves the live value alone, since both reads default to it. The final store
    // is deferred: the "Static" block below can still force it to 1.
    unsigned char bPlot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Invulnerable"), bSuccess, static_cast<unsigned char>(m_bPlot));
    if (!bSuccess)
        bPlot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Plot"), bSuccess, static_cast<unsigned char>(m_bPlot));

#if !KOTOR1
    m_bNotBlastable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("NotBlastable"), bSuccess,
                                           static_cast<unsigned char>(m_bNotBlastable));
#endif
    m_bMin1HP = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Min1HP"), bSuccess, static_cast<unsigned char>(m_bMin1HP));
    m_bPartyInteract = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("PartyInteract"), bSuccess, 0);
    m_baseOpenLockDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("OpenLockDC"), bSuccess, 0);
#if !KOTOR1
    m_lockDCFormula = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("OpenLockDiff"), bSuccess, 0);
    m_lockDCFlatBase = pGFF->ReadFieldCHAR(pStruct, const_cast<char*>("OpenLockDiffMod"), bSuccess, 0);
#endif
    m_keyName = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("KeyName"), bSuccess,
                                           CExoString(const_cast<char*>("")));
    m_bTrapDisarmable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapDisarmable"), bSuccess, 0);
    m_bTrapDetectable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapDetectable"), bSuccess, 0);
    // Read raw, unlike SavePlaceable's write side, since the writer applies the difficulty
    // adjustment and the clamp-to-1 on the way OUT, so the stored value is always the un-adjusted
    // base.
    m_baseDisarmDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("DisarmDC"), bSuccess, 0);
    m_baseDetectDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapDetectDC"), bSuccess, 0);
#if !KOTOR1
    m_nOwnerDemolitionsSkill = pGFF->ReadFieldINT(pStruct, const_cast<char*>("OwnerDemolitionsSkill"),
                                                   bSuccess, 0);
#endif
    m_bTrapFlag = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapFlag"), bSuccess, 0);
    m_bTrapOneShot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapOneShot"), bSuccess, 0);
    m_trapType = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapType"), bSuccess, m_trapType);
    m_bUseable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Useable"), bSuccess, 0);

    // "Static" defaults to the inverse of useability when the field is absent, is vetoed outright
    // for a placeholder object (m_bPlaceholder), and a static placeable is implicitly plot-protected.
    unsigned char bStaticField = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Static"), bSuccess, 0);
    int bStatic;
    if (bSuccess)
        bStatic = bStaticField;
    else
        bStatic = (m_bUseable != 1);
    if (m_bPlaceholder == 1)
        bStatic = 0;
    m_bStatic = bStatic;
    if (bStatic == 1)
        bPlot = 1;
    m_bPlot = bPlot;

    m_appearanceType = static_cast<decltype(m_appearanceType)>(
        pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Appearance"), bSuccess, 0));
#if !KOTOR1
    m_bUseTweakColor = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("UseTweakColor"), bSuccess, 0);
    m_tweakColor = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("TweakColor"), bSuccess, 0);
#endif
    m_hitPoints = pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("HP"), bSuccess, 0);
    SetCurrentHitPoints(pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("CurrentHP"), bSuccess, 0));
    m_hardness = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Hardness"), bSuccess, 0);
    m_fortSave = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Fort"), bSuccess, 0);
    m_willSave = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Will"), bSuccess, 0);
    m_refSave = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Ref"), bSuccess, 0);
    m_bLockable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Lockable"), bSuccess, 0);
    m_bLocked = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Locked"), bSuccess, 0);
    m_bHasInventory = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("HasInventory"), bSuccess, 0);
    m_bKeyRequired = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("KeyRequired"), bSuccess, 0);
    m_closeLockDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("CloseLockDC"), bSuccess, 0);

    // The portrait load is identical to CSWSTrigger::LoadTrigger's, where both unset sentinel
    // values and send it down the resref fallback.
    unsigned short nPortraitId =
        pGFF->ReadFieldWORD(pStruct, const_cast<char*>("PortraitId"), bSuccess, PORTRAIT_ID_NONE);
    if (nPortraitId >= PORTRAIT_ID_CUSTOM) {
        SetPortrait(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("Portrait"), bSuccess, CResRef("")));
    } else {
        SetPortraitId(nPortraitId);
    }

    SetDialogResref(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("Conversation"), bSuccess, CResRef("")));
    m_bodyBagAppearanceRow = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("BodyBag"), bSuccess,
                                                  m_bodyBagAppearanceRow);
    m_bDieWhenEmpty = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("DieWhenEmpty"), bSuccess, 0);
    // The "GroundPile" read's result is discarded, an engine bug, and the field is then forced to
    // 1, so every placeable that goes through LoadPlaceable becomes a ground pile regardless of
    // what the blueprint said. The read still passes the live value as its default, which is what
    // the correct code would do, so this reads as a dropped assignment.
    pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("GroundPile"), bSuccess, static_cast<unsigned char>(m_bGroundPile));
    m_bGroundPile = 1;

    SetLightIsOn(pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("LightState"), bSuccess, static_cast<unsigned char>(GetLightIsOn())));
    m_description = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("Description"), bSuccess,
                                                  CExoLocString());

    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnClosed"), bSuccess, CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_CLOSED] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnDamaged"), bSuccess, CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_DAMAGED] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnDeath"), bSuccess, CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_DEATH] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnDisarm"), bSuccess, CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_DISARM] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnHeartbeat"), bSuccess,
                            CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_HEARTBEAT] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnInvDisturbed"), bSuccess,
                            CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_INV_DISTURBED] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnLock"), bSuccess, CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_LOCK] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnMeleeAttacked"), bSuccess,
                            CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_MELEE_ATTACKED] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnOpen"), bSuccess, CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_OPEN] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnSpellCastAt"), bSuccess,
                            CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_SPELL_CAST_AT] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnUnlock"), bSuccess, CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_UNLOCK] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnUsed"), bSuccess, CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_USED] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnUserDefined"), bSuccess,
                            CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_USER_DEFINED] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnDialog"), bSuccess, CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_DIALOG] = sScript;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnEndDialogue"), bSuccess,
                            CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_END_DIALOGUE] = sScript;

    // An unset or still-"default" trap script falls back to traps.2da, the same column family
    // as CSWSTrigger::LoadTrigger but keyed on "MineScript" rather than "TrapScript". The two
    // DCs the trigger derives from traps.2da are GFF-stored here instead.
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnTrapTriggered"), bSuccess,
                            CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_TRAP_TRIGGERED] = sScript;
    if (m_scriptsOn[ON_SCRIPT_TRAP_TRIGGERED].IsEmpty() ||
        m_scriptsOn[ON_SCRIPT_TRAP_TRIGGERED] == "default") {
        CExoString sTrapScript;
        g_pRules->m_p2DArrays->m_pTraps->GetCExoStringEntry(m_trapType,
                                                            CExoString("MineScript"),
                                                            &sTrapScript);
        m_scriptsOn[ON_SCRIPT_TRAP_TRIGGERED] = sTrapScript;
    }

#if !KOTOR1
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnFailToOpen"), bSuccess,
                            CResRef("")).CopyToString(sScript);
    m_scriptsOn[ON_SCRIPT_FAIL_TO_OPEN] = sScript;
#endif

    m_bIsBodyBag = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("IsBodyBag"), bSuccess, static_cast<unsigned char>(m_bIsBodyBag));
    m_corpse.m_bIsCorpse = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("IsCorpse"), bSuccess, static_cast<unsigned char>(m_corpse.m_bIsCorpse));
#if !KOTOR1
    m_nPCLevelAtSpawn = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("PCLevelAtSpawn"), bSuccess, m_nPCLevelAtSpawn);
#endif

    // For the animation, an open container is pinned to the "opened" pose outright; otherwise a
    // raw "Animation" id wins, and failing that the legacy "AnimationState" enum is mapped through
    // a small table indexed at (state - 1).
    unsigned char bOpen = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Open"), bSuccess, 0);
    m_bInventoryOpen = bOpen;
    if (bOpen) {
        SetAnimation(SERVER_ANIMATION_PLACEABLE_OPEN);
    } else {
        int nAnimation = pGFF->ReadFieldINT(pStruct, const_cast<char*>("Animation"), bSuccess, 0);
        if (bSuccess) {
            SetAnimation(nAnimation);
        } else {
            unsigned char nAnimationState =
                pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("AnimationState"), bSuccess, 0);
            if (bSuccess) {
                switch (nAnimationState) {
                case 1:  nAnim = SERVER_ANIMATION_PLACEABLE_OPEN; break;
                case 2:  nAnim = SERVER_ANIMATION_PLACEABLE_CLOSE; break;
                case 3:  nAnim = SERVER_ANIMATION_DESTROYED; break;
                case 4:  nAnim = SERVER_ANIMATION_PLACEABLE_ACTIVATE; break;
                case 5:  nAnim = SERVER_ANIMATION_PLACEABLE_DEACTIVATE; break;
                default: nAnim = SERVER_ANIMATION_PAUSE; break;
                }
                SetAnimation(nAnim);
            }
        }
    }

    // Inventory. `bFromTemplate` picks the source of each item: a blueprint resref named by
    // "InventoryRes" (the UTP path, where a missing field is fatal for that element), or the
    // item's own fully-serialized struct (the savegame path).
    CResList itemList;
    if (pGFF->GetList(&itemList, pStruct, const_cast<char*>("ItemList")) == 1) {
        for (unsigned int i = 0; i < pGFF->GetListCount(&itemList); i++) {
            CSWSItem* pItem = 0;
            CResStruct itemStruct;
            pGFF->GetListElement(&itemStruct, &itemList, i);
            unsigned int nObjectId = pGFF->ReadFieldDWORD(&itemStruct, const_cast<char*>("ObjectId"),
                                                           bSuccess, OBJECT_INVALID);
            pItem = new CSWSItem(nObjectId);
            if (bFromTemplate) {
                CResRef resTemplate = pGFF->ReadFieldCResRef(&itemStruct, const_cast<char*>("InventoryRes"),
                                                              bSuccess, CResRef(""));
                if (!bSuccess || !pItem->LoadFromTemplate(resTemplate)) {
                    if (pItem)
                        delete pItem;
                    pItem = 0;
                }
            } else {
                if (!pItem->LoadItem(pGFF, &itemStruct)) {
                    if (pItem)
                        delete pItem;
                    pItem = 0;
                }
            }
            // The teardown and the hand-off are two separate statements sharing one tail, not
            // an if/else. That is why pItem is reloaded right after being nulled and a value
            // just proved to be zero is tested again: &pItem escapes into AcquireItem, so the
            // alloca stays in memory and the null store cannot be forwarded into the guard.
            // Spelling it as an if/else removes the reload and the whole block.
            if (pItem)
                AcquireItem(&pItem, OBJECT_INVALID, 1);
        }
    }

    g_pAppManager->m_pServerExoApp->GetModule()->AddObjectToLookupTable(m_sTag, m_objectId);
    PostProcess();
    return 1;
}

// True for the 16 key-binding strrefs that belong to the PC controls list.
int StringContainsPCControls(int nStrRef)
{
    switch (nStrRef) {
    case STRREF_TUTORIAL_LOCKER_KEYCARD:
    case STRREF_TUTORIAL_NEW_QUESTS:
    case STRREF_TUTORIAL_PARTY_MEMBER:
    case STRREF_TUTORIAL_DIALOG:
    case STRREF_TUTORIAL_TARGETING:
    case STRREF_TUTORIAL_EQUIP_WEAPON:
    case STRREF_TUTORIAL_SOLO_MODE:
    case STRREF_TUTORIAL_DROID_FLAMETHROWER:
    case STRREF_TUTORIAL_LOCKER_KEYCARD_2:
    case STRREF_TUTORIAL_DROID_FLAMETHROWER_2:
    case STRREF_TUTORIAL_REPAIR_KIT:
    case STRREF_TUTORIAL_BASH:
    case STRREF_TUTORIAL_PLOT_COURSE:
    case STRREF_TUTORIAL_MAP:
    case STRREF_TUTORIAL_MOVE_T3M4:
    case STRREF_TUTORIAL_TARGET_OBJECTS:
        return 1;
    }
    return 0;
}
