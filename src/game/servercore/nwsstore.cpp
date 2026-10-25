#include "nwsstore.h"
#include "restype.h"
#include "nwsscriptevent.h"
#include "CExoArrayListImpl.h"

#if !defined(_MSC_VER)  // cl.exe takes the inline one from <new>, through List.h
inline void* operator new(size_t, void* p) throw() { return p; }
#endif

CSWSStore::CSWSStore(unsigned long long objectId)
    : CSWSObject(CGameObject::OBJECT_TYPE_STORE, objectId, 0) {
    m_nInventoryWalkIndex = 0;
    m_pItemRepository = new CItemRepository(m_objectId, 0);
    m_nBonusMarkDown = 0;
    m_nBonusMarkUp = 0;
    m_buySellFlag = STORE_BUY_AND_SELL;
    CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    pAI->AddObject(this, 0);
}

void CSWSStore::AIUpdate() {
}

// Every other CSWSObject derivative detaches from its area here; stores do not.
void CSWSStore::RemoveFromArea() {
}

CSWSStore* CSWSStore::AsSWSStore() {
    return this;
}

void CSWSStore::RemoveItem(CSWSItem* pItem) {
    m_pItemRepository->RemoveItem(pItem);
}

void CSWSStore::RemoveItemFromInventory(CSWSItem* pItem) {
    m_pItemRepository->RemoveItem(pItem);
}

void CSWSStore::RemoveCustomer(CSWSPlayer* pPlayer) {
    K2_UNUSED(pPlayer);
    m_customerId = OBJECT_INVALID;
}

void CSWSStore::AddCustomer(CSWSPlayer* pPlayer) {
    CGameObject* pObj = pPlayer->GetGameObject();
    m_customerId = pObj->m_objectId;
}

int CSWSStore::CalculateItemSellPrice(CSWSItem* pItem, unsigned long long qty) {
    K2_UNUSED(qty);
    if (pItem->m_bPlot) return 0;
    int value = pItem->GetCost();
    return value > 1 ? value : 1;
}

int CSWSStore::CalculateItemBuyPrice(CSWSItem* pItem, unsigned long long qty) {
    K2_UNUSED(qty);
    if (pItem->m_bPlot) return 0;
    int value = pItem->GetCost();
    int clamped = value > 1 ? value : 1;
    return value == 0 ? 0 : clamped;
}

int CSWSStore::GetItemInInventory(const CExoString& tag) {
    unsigned long long itemId = m_pItemRepository->FindItemWithTag(tag);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemId);
    return pItem != 0;
}

int CSWSStore::GetItemInInventory(unsigned long long objectId) {
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(objectId);
    if (pItem == 0) return 0;
    return m_pItemRepository->GetItemInRepository(pItem, 0) != 0;
}

#if KOTOR1
// K1 always files by price (K2's flag == 0 path); a placed copy of the same object refuses the add.
int CSWSStore::AddItemToInventory(CSWSItem** ppItem) {
    unsigned int newCost = (*ppItem)->GetCost();
    CSWSItem* pCandidate = m_pItemRepository->ItemListGetItem(0);
    if (pCandidate != 0) {
        unsigned int cost = pCandidate->GetCost();
        if (newCost < cost) {
            int index = m_pItemRepository->ItemListGetCount();
            do {
                index--;
                pCandidate = m_pItemRepository->ItemListGetItem(index);
                if (pCandidate != 0) {
                    if (pCandidate->m_objectId == (*ppItem)->m_objectId) {
                        return 0;
                    }
                    cost = pCandidate->GetCost();
                    if (newCost < cost) {
                        m_pItemRepository->InsertItem(*ppItem, index);
                        return 1;
                    }
                }
            } while (true);
        }
    }
    m_pItemRepository->AddItem(ppItem, 1, 0, 1);
    return 1;
}
#endif

int CSWSStore::AddItemToInventory(CSWSItem** ppItem, int flag) {
    if (flag == 0) {
        unsigned int newCost = (*ppItem)->GetCost();
        CSWSItem* pCandidate = m_pItemRepository->ItemListGetItem(0);
        if (pCandidate != 0) {
            unsigned int cost = pCandidate->GetCost();
            if (newCost < cost) {
                int index = m_pItemRepository->ItemListGetCount();
                do {
                    index--;
                    pCandidate = m_pItemRepository->ItemListGetItem(index);
                    if (pCandidate != 0) {
                        if (pCandidate->m_objectId == (*ppItem)->m_objectId) {
                            return 0;
                        }
                        cost = pCandidate->GetCost();
                        if (newCost < cost) {
                            m_pItemRepository->InsertItem(*ppItem, index);
                            return 1;
                        }
                    }
                } while (true);
            }
        }
    }
    m_pItemRepository->AddItem(ppItem, 1, 0, 1);
    return 1;
}

void CSWSStore::AddToArea(CSWSArea* pArea, float x, float y, float z, int a5) {
    if (pArea != 0) {
        SetArea(pArea);
        Vector pos = {x, y, z};
        SetPosition(pos, 0, 1, 0);
        static_cast<CSWSArea*>(GetArea())->AddObjectToArea(m_objectId, a5);
    }
}

void CSWSStore::SellItem(CSWSItem* pItem, CSWSCreature* pCreature) {
    CSWSItem* local40 = pItem;
    CSWSItem* local48;
    if ((pItem->m_flags & 4) == 0) {
        if (pItem->m_stackCount > 1) {
            local48 = pItem->SplitItem(1);
            goto shared;
        }
        if (!m_pItemRepository->RemoveItem(pItem)) return;
        pCreature->AcquireItem(&local40, m_objectId, OBJECT_INVALID, 1);
        return;
    } else {
        local48 = new CSWSItem(OBJECT_INVALID);
        local48->CopyItem(local40);
        local48->m_stackCount = 1;
        local48->m_flags &= ~CSWSItem::FLAG_INFINITE;
    }
shared:
    pCreature->AcquireItem(&local48, m_objectId, OBJECT_INVALID, 1);
}

int CSWSStore::LoadFromTemplate(const CResRef& resRef, int flag) {
    CResGFF* pGff = new CResGFF(RESTYPE_UTM, const_cast<char*>("UTM "), resRef);
    CResStruct topStruct;
    int result;
    if (pGff->m_nValid != 0) {
        pGff->GetTopLevelStruct(&topStruct);
        LoadStore(pGff, &topStruct, flag);
        delete pGff;
        result = 1;
    } else {
        delete pGff;
        CExoString msg;
        CExoString resRefStr;
        resRef.CopyToString(resRefStr);
        msg.Format(const_cast<char*>("Store template %s doesn't exist.\n"), resRefStr.CStr());
        result = 0;
    }
    return result;
}

// Reads the GFF fields SaveStore writes, then rebuilds the item list: each entry resolves an
// existing game object by id, or builds a fresh CSWSItem, and is staged into an array kept in
// descending cost order. Staged items are added back to front. SetPossessor is gated on whether
// the last element of the GFF list was staged, not the current one.
int CSWSStore::LoadStore(CResGFF* pGFF, CResStruct* pStruct, int flag) {
    int valid;
    SetTag(pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), valid, CExoString("")));
    m_locName = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LocName"), valid, CExoLocString());
    m_nMarkDown = pGFF->ReadFieldINT(pStruct, const_cast<char*>("MarkDown"), valid, 0);
    m_nMarkUp = pGFF->ReadFieldINT(pStruct, const_cast<char*>("MarkUp"), valid, 0);
    CExoString onOpenStoreStr;
    pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnOpenStore"), valid, CResRef("")).CopyToString(onOpenStoreStr);
    m_sOnOpenStore = onOpenStoreStr;
    m_buySellFlag = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("BuySellFlag"), valid, m_buySellFlag);

    CResList itemList;
    if (pGFF->GetList(&itemList, pStruct, const_cast<char*>("ItemList"))) {
        CExoArrayList<CSWSItem*> items;
        CSWSItem* pLastItem K2_UNSET;
        for (unsigned int i = 0; i < pGFF->GetListCount(&itemList); i++) {
            CResStruct itemStruct;
            pGFF->GetListElement(&itemStruct, &itemList, i);
            unsigned int objId = pGFF->ReadFieldDWORD(&itemStruct, const_cast<char*>("ObjectId"), valid, OBJECT_INVALID);
            void* pExisting = g_pAppManager->m_pServerExoApp->GetGameObject(objId);
            if (pExisting != 0) {
                pLastItem = 0;
                continue;
            }
            CSWSItem* pItem = new CSWSItem(objId);
            if (flag != 0) {
                CResRef templateRef = pGFF->ReadFieldCResRef(&itemStruct, const_cast<char*>("InventoryRes"), valid, CResRef(""));
                if (valid == 0) goto skipItem;
                if (pItem->LoadFromTemplate(templateRef) == 0) goto skipItem;
                goto loaded;
            }
            if (pItem->LoadItem(pGFF, &itemStruct) != 0) goto loaded;
            goto skipItem;
        loaded:;
            {
                unsigned char infinite = pGFF->ReadFieldBYTE(&itemStruct, const_cast<char*>("Infinite"), valid, 0);
                pItem->m_flags = static_cast<unsigned short>((pItem->m_flags & ~CSWSItem::FLAG_INFINITE) | ((infinite & 1) << 2));
            }
            {
                unsigned int newCost = pItem->GetCost();
                int insertIndex = -1;
                for (int j = 0; j < items.m_count && insertIndex == -1; j++) {
                    if (newCost > items.m_pArray[j]->GetCost())
                        insertIndex = j;
                }
                if (insertIndex != -1) {
                    items.Insert(pItem, insertIndex);
                } else {
                    items.Add(pItem);
                }
            }
            pLastItem = pItem;
            continue;
        skipItem:
            delete pItem;
            pLastItem = 0;
            continue;
        }
        if (items.m_count > 0) {
            for (int idx = items.m_count - 1; idx >= 0; idx--) {
#if KOTOR1
                int added = AddItemToInventory(&items.m_pArray[idx]);
#else
                int added = AddItemToInventory(&items.m_pArray[idx], 0);
#endif
                if (pLastItem != 0 && added != 0) {
                    items.m_pArray[idx]->SetPossessor(m_objectId, 1, 1, OBJECT_INVALID);
                }
            }
        }
    }
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    pModule->AddObjectToLookupTable(m_sTag, m_objectId);
    return 1;
}

// Returns 0 for a rejected item, the add result when the item is added, and 1 when it merely
// stacked onto one already in the store.
int CSWSStore::AcquireItem(CSWSItem* pItem, int flag) {
    CSWSItem* local60 = pItem;
    if (pItem == 0 || pItem->m_bPlot != 0) {
        return 0;
    }
    pItem->SetIdentified(1);
    CGameObject* pObj = g_pAppManager->m_pServerExoApp->GetGameObject(pItem->m_possessorId);
    if (pObj != 0) {
        CSWSCreature* r = pObj->AsSWSCreature();
        CSWSCreature* pCreature;
        if (r == 0) {
            CSWSItem* r2 = pObj->AsSWSItem();
            if (r2 != 0) {
                CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
                CSWSItem* r3 = pObj->AsSWSItem();
                pCreature = pServerApp->GetCreatureByGameObjectID(r3->m_possessorId);
                if (pCreature != 0) goto acquireRemove;
            }
        } else {
            pCreature = pObj->AsSWSCreature();
        acquireRemove:
            pCreature->RemoveItem(pItem, 1, 1, 0);
        }
    }
    unsigned long long tag = m_pItemRepository->FindItemWithTag(pItem->m_sTag);
    CSWSItem* pExisting = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(tag);
    if (pExisting == 0 || !pExisting->CompareItem(pItem)) {
        pItem->SetPossessor(m_objectId, 1, 1, OBJECT_INVALID);
        return AddItemToInventory(&local60, flag);
    }
    if ((pExisting->m_flags & 4) == 0) {
        pExisting->m_stackCount = pExisting->m_stackCount + 1;
    }
    delete pItem;
    return 1;
}

#if KOTOR1
int CSWSStore::AcquireItem(CSWSItem* pItem) {
    CSWSItem* local60 = pItem;
    if (pItem == 0 || pItem->m_bPlot != 0) {
        return 0;
    }
    pItem->SetIdentified(1);
    CGameObject* pObj = g_pAppManager->m_pServerExoApp->GetGameObject(pItem->m_possessorId);
    if (pObj != 0) {
        CSWSCreature* r = pObj->AsSWSCreature();
        CSWSCreature* pCreature;
        if (r == 0) {
            CSWSItem* r2 = pObj->AsSWSItem();
            if (r2 != 0) {
                CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
                CSWSItem* r3 = pObj->AsSWSItem();
                pCreature = pServerApp->GetCreatureByGameObjectID(r3->m_possessorId);
                if (pCreature != 0) goto acquireRemove;
            }
        } else {
            pCreature = pObj->AsSWSCreature();
        acquireRemove:
            pCreature->RemoveItem(pItem, 1, 1, 0);
        }
    }
    unsigned long long tag = m_pItemRepository->FindItemWithTag(pItem->m_sTag);
    CSWSItem* pExisting = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(tag);
    if (pExisting == 0 || !pExisting->CompareItem(pItem)) {
        pItem->SetPossessor(m_objectId, 1, 1, OBJECT_INVALID);
        return AddItemToInventory(&local60);
    }
    if ((pExisting->m_flags & 4) == 0) {
        pExisting->m_stackCount = pExisting->m_stackCount + 1;
    }
    delete pItem;
    return 1;
}
#endif

int CSWSStore::SaveStore(CResGFF* pGFF, CResStruct* pStruct) {
    pGFF->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Tag"));
    pGFF->WriteFieldCExoLocString(pStruct, m_locName, const_cast<char*>("LocName"));
    pGFF->WriteFieldINT(pStruct, m_nMarkDown, const_cast<char*>("MarkDown"));
    pGFF->WriteFieldINT(pStruct, m_nMarkUp, const_cast<char*>("MarkUp"));
    {
        // Stored as a CExoString, written as a ResRef.
        CResRef onOpenStore(m_sOnOpenStore);
        pGFF->WriteFieldCResRef(pStruct, onOpenStore, const_cast<char*>("OnOpenStore"));
    }
    pGFF->WriteFieldBYTE(pStruct, m_buySellFlag, const_cast<char*>("BuySellFlag"));
    CResList itemList;
    pGFF->AddList(&itemList, pStruct, const_cast<char*>("ItemList"));
    CItemRepository* pRepo = m_pItemRepository;
    if (pRepo->ItemListGetCount() != 0) {
        unsigned int i = 0;
        do {
            CSWSItem* pItem = pRepo->ItemListGetItem(i);
            CResStruct itemStruct;
            pGFF->AddListElement(&itemStruct, &itemList, i);
            pGFF->WriteFieldDWORD(&itemStruct, static_cast<unsigned int>(pItem->m_objectId), const_cast<char*>("ObjectId"));
            pGFF->WriteFieldBYTE(&itemStruct, static_cast<unsigned char>(pItem->m_flags >> 2) & 1, const_cast<char*>("Infinite"));
            pItem->SaveItem(pGFF, &itemStruct);
            pRepo = m_pItemRepository;
            i++;
        } while (i < pRepo->ItemListGetCount());
    }
    pGFF->WriteFieldFLOAT(pStruct, m_position.x, const_cast<char*>("XPosition"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.y, const_cast<char*>("YPosition"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.z, const_cast<char*>("ZPosition"));
    pGFF->WriteFieldFLOAT(pStruct, m_orientation.x, const_cast<char*>("XOrientation"));
    pGFF->WriteFieldFLOAT(pStruct, m_orientation.y, const_cast<char*>("YOrientation"));
    pGFF->WriteFieldFLOAT(pStruct, m_orientation.z, const_cast<char*>("ZOrientation"));
    return 1;
}

void CSWSStore::EventHandler(unsigned int eventType, unsigned long long a2, void* a3,
                               unsigned int a4, unsigned int a5) {
    K2_UNUSED(a4); K2_UNUSED(a5);
    CScriptEvent* pEvent = static_cast<CScriptEvent*>(a3);
    if (eventType == static_cast<unsigned int>(CServerAIMaster::EVENT_SIGNAL_EVENT)) {
        if (pEvent->m_eventType == CSWSSCRIPTEVENT_EVENTTYPE_ON_OPEN) {
            m_oidLastOpenedBy = a2;
            g_pVirtualMachine->RunScript(&m_sOnOpenStore, m_objectId, 1);
        }
        pEvent->~CScriptEvent();
        operator delete(pEvent);
    } else if (eventType == static_cast<unsigned int>(CServerAIMaster::EVENT_TIMED_EVENT)) {
        if (pEvent != 0) {
            g_pVirtualMachine->RunScriptSituation(pEvent, m_objectId, 1);
        }
    } else if (reinterpret_cast<uintptr_t>(this) != 0) {
        if (eventType == static_cast<unsigned int>(CServerAIMaster::EVENT_DESTROY_OBJECT)) {
            delete this;
        }
    }
}

CSWSStore::~CSWSStore() {
    delete m_pItemRepository;
    m_customerId = OBJECT_INVALID;
    GetArea();
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->RemoveObject(this);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0) {
        pModule->RemoveObjectFromLookupTable(m_sTag, m_objectId);
    }
}

