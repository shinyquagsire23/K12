#include "nwsplayerluo.h"
#include "serveranimation.h"
#include "gameobjectarray.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "CExoArrayListImpl.h"
#include "nwscreaturestats.h"

// The per-player inventory snapshot the server diffs against. Each ItemList member is a thin
// adapter over the list nList selects: 0 the container, 1 the repository, 2 the store, and anything
// else the do-nothing arm.
CSWSPlayerLUOInventory::CSWSPlayerLUOInventory()
{
    ClearSlots();
    ClearRepository();
    ClearContainer();
    ClearStore();
    m_openContainerId = CGameObjectArray::INVALIDOBJID;
}

// Clears the whole object on the way out; both special members share this body.
CSWSPlayerLUOInventory::~CSWSPlayerLUOInventory()
{
    ClearSlots();
    ClearRepository();
    ClearContainer();
    ClearStore();
    m_openContainerId = CGameObjectArray::INVALIDOBJID;
}

// Resets the two parallel slot arrays to their sentinels.
void CSWSPlayerLUOInventory::ClearSlots()
{
#if KOTOR1
    for (int i = 0; i < 18; i++)
#else
    for (int i = 0; i < 20; i++)
#endif
    {
        m_slotItemId[i] = CGameObjectArray::INVALIDOBJID;
        m_slotItemStackCount[i] = -1;
    }
}

void CSWSPlayerLUOInventory::ClearRepository()
{
    while (m_repository.m_pInternal->m_pHead != 0)
        delete static_cast<CSWSPlayerLUOInventoryItem*>(m_repository.m_pInternal->RemoveHead());
}

void CSWSPlayerLUOInventory::ClearContainer()
{
    while (m_container.m_pInternal->m_pHead != 0)
        delete static_cast<CSWSPlayerLUOInventoryItem*>(m_container.m_pInternal->RemoveHead());
}

void CSWSPlayerLUOInventory::ClearStore()
{
    while (m_store.m_pInternal->m_pHead != 0)
        delete static_cast<CSWSPlayerLUOInventoryItem*>(m_store.m_pInternal->RemoveHead());
}

unsigned long long CSWSPlayerLUOInventory::ItemListGetItemObjectID(CExoLinkedListNode* pNode,
                                                                   unsigned char nList)
{
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1: pItem is uninitialised on the bad-selector arm
    CSWSPlayerLUOInventoryItem* pItem;
#else
    CSWSPlayerLUOInventoryItem* pItem = 0;
#endif

    if (nList == 0)
        pItem = static_cast<CSWSPlayerLUOInventoryItem*>(m_container.m_pInternal->GetAtPos(pNode));
    else if (nList == 1)
        pItem = static_cast<CSWSPlayerLUOInventoryItem*>(m_repository.m_pInternal->GetAtPos(pNode));
    else if (nList == 2)
        pItem = static_cast<CSWSPlayerLUOInventoryItem*>(m_store.m_pInternal->GetAtPos(pNode));

    if (pItem == 0)
        return CGameObjectArray::INVALIDOBJID;

    return pItem->m_objectId;
}

CSWSPlayerLUOInventoryItem* CSWSPlayerLUOInventory::ItemListGetUpdateItem(CExoLinkedListNode* pNode,
                                                                          unsigned char nList)
{
    if (nList == 0)
        return static_cast<CSWSPlayerLUOInventoryItem*>(m_container.m_pInternal->GetAtPos(pNode));
    if (nList == 1)
        return static_cast<CSWSPlayerLUOInventoryItem*>(m_repository.m_pInternal->GetAtPos(pNode));
    if (nList == 2)
        return static_cast<CSWSPlayerLUOInventoryItem*>(m_store.m_pInternal->GetAtPos(pNode));

    return 0;
}

void CSWSPlayerLUOInventory::ItemListSetStart(CExoLinkedListNode*& pNode, unsigned char nList)
{
    CExoLinkedList<CSWSPlayerLUOInventoryItem>* pList;

    if (nList == 0)
        pList = &m_container;
    else if (nList == 1)
        pList = &m_repository;
    else if (nList == 2)
        pList = &m_store;
    else
        return;

    pNode = pList->m_pInternal->m_pHead;
}

void CSWSPlayerLUOInventory::ItemListSetEnd(CExoLinkedListNode*& pNode, unsigned char nList)
{
    CExoLinkedList<CSWSPlayerLUOInventoryItem>* pList;

    if (nList == 0)
        pList = &m_container;
    else if (nList == 1)
        pList = &m_repository;
    else if (nList == 2)
        pList = &m_store;
    else
        return;

    pNode = pList->m_pInternal->m_pTail;
}

void CSWSPlayerLUOInventory::ItemListGetNext(CExoLinkedListNode*& pNode, unsigned char nList)
{
    if (nList == 0)
        m_container.m_pInternal->GetNext(pNode);
    else if (nList == 1)
        m_repository.m_pInternal->GetNext(pNode);
    else if (nList == 2)
        m_store.m_pInternal->GetNext(pNode);
}

void CSWSPlayerLUOInventory::ItemListGetPrev(CExoLinkedListNode*& pNode, unsigned char nList)
{
    if (nList == 0)
        m_container.m_pInternal->GetPrev(pNode);
    else if (nList == 1)
        m_repository.m_pInternal->GetPrev(pNode);
    else if (nList == 2)
        m_store.m_pInternal->GetPrev(pNode);
}

int CSWSPlayerLUOInventory::ItemListGetNumber(unsigned char nList)
{
    CExoLinkedList<CSWSPlayerLUOInventoryItem>* pList;

    if (nList == 0)
        pList = &m_container;
    else if (nList == 1)
        pList = &m_repository;
    else if (nList == 2)
        pList = &m_store;
    else
        return 0;

    return pList->m_pInternal->m_nCount;
}

void CSWSPlayerLUOInventory::ItemListRemove(CExoLinkedListNode* pNode, unsigned char nList)
{
    if (nList == 0)
        delete static_cast<CSWSPlayerLUOInventoryItem*>(m_container.m_pInternal->Remove(pNode));
    else if (nList == 1)
        delete static_cast<CSWSPlayerLUOInventoryItem*>(m_repository.m_pInternal->Remove(pNode));
    else if (nList == 2)
        delete static_cast<CSWSPlayerLUOInventoryItem*>(m_store.m_pInternal->Remove(pNode));
}

void CSWSPlayerLUOInventory::ItemListAddHead(unsigned long long objectId, unsigned char nList,
                                             unsigned int nStackCount, unsigned int nCost)
{
    CSWSPlayerLUOInventoryItem* pItem = new CSWSPlayerLUOInventoryItem;
    pItem->m_objectId = objectId;
    pItem->m_nStackCount = nStackCount;
    pItem->m_nCost = nCost;

    // One AddHead per arm rather than a list pointer, so the selector ladder stays flat here.
    if (nList == 0)
        m_container.m_pInternal->AddHead(pItem);
    else if (nList == 1)
        m_repository.m_pInternal->AddHead(pItem);
    else if (nList == 2)
        m_store.m_pInternal->AddHead(pItem);
    else
        delete pItem;
}

// Resolves a snapshot entry back to the live item it stands for; every failure returns null.
CSWSItem* CSWSPlayerLUOInventory::ItemListGetItem(CExoLinkedListNode* pNode, unsigned char nList)
{
#if KOTOR1  // K1: a bad selector falls through with pItem uninitialised
    CSWSPlayerLUOInventoryItem* pItem;
#ifdef K2_FIX_ENGINE_BUGS
    pItem = 0;
#endif

    if (nList == 0)
        pItem = static_cast<CSWSPlayerLUOInventoryItem*>(m_container.m_pInternal->GetAtPos(pNode));
    else if (nList == 1)
        pItem = static_cast<CSWSPlayerLUOInventoryItem*>(m_repository.m_pInternal->GetAtPos(pNode));
    else if (nList == 2)
        pItem = static_cast<CSWSPlayerLUOInventoryItem*>(m_store.m_pInternal->GetAtPos(pNode));

    if (pItem == 0)
        return 0;

    CSWSItem* pResult = 0;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(pItem->m_objectId, &pObject)
            == CGameObjectArray::SUCCESS
        && pObject != 0)
        pResult = pObject->AsSWSItem();
    return pResult;
#else
    CExoLinkedList<CSWSPlayerLUOInventoryItem>* pList;

    if (nList == 0)
        pList = &m_container;
    else if (nList == 1)
        pList = &m_repository;
    else if (nList == 2)
        pList = &m_store;
    else
        return 0;

    CSWSPlayerLUOInventoryItem* pItem =
        static_cast<CSWSPlayerLUOInventoryItem*>(pList->m_pInternal->GetAtPos(pNode));
    if (pItem == 0)
        return 0;

    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(pItem->m_objectId, &pObject)
        != CGameObjectArray::SUCCESS)
        return 0;

    if (pObject == 0)
        return 0;

    return pObject->AsSWSItem();
#endif
}

// CSWSPlayerLastUpdateObject leaf accessors, over the KnownSpellUsesLeft snapshot slots.
//
// AddKnownSpell needs CExoArrayList<unsigned int>::Allocate, which this unit defines.
//
// The spell-like-ability list's delete[] needs the element type complete, so the header's forward
// declaration does not suffice in this unit.

// Zeroes the seven CExoArrayList members and the lazily allocated spell-like-ability pointer; the
// character-sheet scalar block is left alone except m_bCanLevelUp, since it is only read after
// StoreValuesInLastPlayerUpdateObject has filled it.
CSWSPlayerLastUpdateObject::CSWSPlayerLastUpdateObject()
{
    // The action-queue sentinels are not written here. CSWActionQueueEntry has a user-provided
    // default constructor, so m_actionQueue stamps itself at its declaration position.
    m_bCanLevelUp = 0;
    m_pSpellLikeAbilityList = 0;
}

// Tears down the three owning members by hand; everything else rides on the member-destructor chain
// in reverse declaration order.
//
// ClearFeats is deliberately absent. The m_featRemainingUses entries this object owns by pointer
// are leaked at teardown, since only the array itself is released.
CSWSPlayerLastUpdateObject::~CSWSPlayerLastUpdateObject()
{
    ClearKnownSpells();
    ClearVisibilityList();

    if (m_pSpellLikeAbilityList != 0)
        delete m_pSpellLikeAbilityList;
    m_pSpellLikeAbilityList = 0;
}

// Resets the whole snapshot to its no-action-here sentinels.
void CSWSPlayerLastUpdateObject::ClearActionQueue()
{
    for (int i = 0; i < NUM_ACTION_QUEUE_ENTRIES; i++)
        m_actionQueue[i].ClearData();
}

// The two knowledge arrays are [N][1], so the class stride and the level stride are the same.
//
// The class and level selectors are unguarded; only the index is checked, as a signed comparison
// against the container's own int count.
int CSWSPlayerLastUpdateObject::GetKnownSpell(unsigned char nClass, unsigned char nLevel,
                                              unsigned char nIndex)
{
    if (m_knownSpells[nClass][nLevel].m_count > nIndex)
        return m_knownSpells[nClass][nLevel].m_pArray[nIndex];
    return -1;
}

void CSWSPlayerLastUpdateObject::AddKnownSpell(unsigned char nClass, unsigned char nLevel,
                                               unsigned int nSpell)
{
    m_knownSpells[nClass][nLevel].Add(nSpell);
}

void CSWSPlayerLastUpdateObject::ClearKnownSpells()
{
    for (int i = 0; i < 2; i++)
        m_knownSpells[i][0].SetSize(0);
}

// Drains the owned visibility nodes from the back.
void CSWSPlayerLastUpdateObject::ClearVisibilityList()
{
    for (int i = m_visibilityList.m_count - 1; i >= 0; i--)
    {
        CSWVisibilityNode* pNode = m_visibilityList.m_pArray[i];
        m_visibilityList.DelIndex(i);
        delete pNode;
    }
}

// Releases the two id lists outright, then walks the remaining-uses entries, which are owned by
// pointer, and clears the array.
void CSWSPlayerLastUpdateObject::ClearFeats()
{
    m_feats.SetSize(0);
    m_bonusFeats.SetSize(0);

    for (int i = 0; i < m_featRemainingUses.m_count; i++)
    {
        if (m_featRemainingUses.m_pArray[i] != 0)
        {
            delete m_featRemainingUses.m_pArray[i];
            m_featRemainingUses.m_pArray[i] = 0;
        }
    }

    m_featRemainingUses.SetSize(0);
}

// Clears the two lists as a pair per class.
void CSWSPlayerLastUpdateObject::ClearSpellAddDeleteLists()
{
    for (int i = 0; i < 2; i++)
    {
        m_spellDeleteList[i][0].SetSize(0);
        m_spellAddList[i][0].SetSize(0);
    }
}

void CSWSPlayerLastUpdateObject::ClearKnownSpellUsesLeft()
{
    m_knownSpellUsesLeft[0] = 0;
    m_knownSpellUsesLeft[1] = 0;
}

unsigned char CSWSPlayerLastUpdateObject::GetKnownSpellUsesLeft(unsigned char nClass,
                                                                unsigned char nLevel)
{
    if (nClass <= 1 && nLevel == 0)
        return m_knownSpellUsesLeft[nClass];
    return 0;
}

void CSWSPlayerLastUpdateObject::SetKnownSpellUsesLeft(unsigned char nClass, unsigned char nLevel,
                                                       unsigned char nUses)
{
    if (nClass <= 1 && nLevel == 0)
        m_knownSpellUsesLeft[nClass] = nUses;
}

// Does nothing beyond the member-destructor chain of two strings, two array lists and two location
// strings, the last one a tail call.
//
// The zeroed position, orientation and last path point are member initialisers, since they must run
// before the first member constructor; the four seeded scalars are body statements, after the last one.
CLastUpdateObject::CLastUpdateObject()
    : m_position(), m_orientation(), m_lastPathPoint(0.0f, 0.0f), m_fLastPathPointZ(0.0f)
{
    m_nSoundSetFile = 0;
    m_nAnimation = SERVER_ANIMATION_READY;
    m_fAnimationSpeed = 1.0f;
    m_nClasses = 0xffff;
    m_nClassLevels = 0xffff;
}

CLastUpdateObject::~CLastUpdateObject()
{
}
