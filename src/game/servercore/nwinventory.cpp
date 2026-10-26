#include "nwscreature.h"
#include "nwsrules.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "nwsitem.h"
#include "nwitem.h"

// The two slot bits CSWInventory's SLOT_* list leaves out: NWN's boots and cloak slots, which
// KOTOR never equips.
enum {
    SLOT_BOOTS = 1 << 2,
    SLOT_CLOAK = 1 << 6,
};

CSWInventory::CSWInventory()
{
    __builtin_memset(m_aItemOids, 0, sizeof(m_aItemOids));
}

CSWInventory::~CSWInventory()
{
}

int CSWInventory::FindSlot(unsigned int nBaseItem)
{
    unsigned int nSlots = g_pRules->GetBaseItem(nBaseItem)->m_nEquipableSlots;
    if (nSlots & SLOT_HEAD) return SLOT_HEAD;
    if (nSlots & SLOT_BODY) return SLOT_BODY;
    if (nSlots & SLOT_BOOTS) return SLOT_BOOTS;
    if (nSlots & SLOT_HANDS) return SLOT_HANDS;
    if (nSlots & SLOT_RIGHTWEAPON) return SLOT_RIGHTWEAPON;
    if (nSlots & SLOT_LEFTWEAPON) return SLOT_LEFTWEAPON;
    if (nSlots & SLOT_CLOAK) return SLOT_CLOAK;
    if (nSlots & SLOT_LEFTARM) return SLOT_LEFTARM;
    if (nSlots & SLOT_RIGHTARM) return SLOT_RIGHTARM;
    if (nSlots & SLOT_IMPLANT) return SLOT_IMPLANT;
    if (nSlots & SLOT_BELT) return SLOT_BELT;
    if (nSlots & SLOT_CWEAPON_L) return SLOT_CWEAPON_L;
    if (nSlots & SLOT_CWEAPON_R) return SLOT_CWEAPON_R;
#if KOTOR1  // K1 stops at the carmour slot
    if (nSlots & SLOT_CWEAPON_B) return SLOT_CWEAPON_B;
    return nSlots & SLOT_CARMOUR;
#else
    if (nSlots & SLOT_CWEAPON_B) return SLOT_CWEAPON_B;
    if (nSlots & SLOT_CARMOUR) return SLOT_CARMOUR;
    if (nSlots & SLOT_RIGHTWEAPON2) return SLOT_RIGHTWEAPON2;
    return nSlots & SLOT_LEFTWEAPON2;
#endif
}

unsigned long long* CSWInventory::GetSlotPointer(unsigned int nSlot)
{
    switch (nSlot) {
    case SLOT_HEAD:     return &m_aItemOids[0];
    case SLOT_BODY:     return &m_aItemOids[1];
    case SLOT_BOOTS:     return &m_aItemOids[2];
    case SLOT_HANDS:     return &m_aItemOids[3];
    case SLOT_RIGHTWEAPON:    return &m_aItemOids[4];
    case SLOT_LEFTWEAPON:    return &m_aItemOids[5];
    case SLOT_CLOAK:    return &m_aItemOids[6];
    case SLOT_LEFTARM:    return &m_aItemOids[7];
    case SLOT_RIGHTARM:   return &m_aItemOids[8];
    case SLOT_IMPLANT:   return &m_aItemOids[9];
    case SLOT_BELT:   return &m_aItemOids[10];
    case SLOT_CWEAPON_L:  return &m_aItemOids[11];
    case SLOT_CWEAPON_R:  return &m_aItemOids[12];
    case SLOT_CWEAPON_B: return &m_aItemOids[13];
    case SLOT_CARMOUR: return &m_aItemOids[14];
#if !KOTOR1
    case SLOT_RIGHTWEAPON2: return &m_aItemOids[15];
    case SLOT_LEFTWEAPON2: return &m_aItemOids[16];
#endif
    }
    return 0;
}

CSWSItem* CSWInventory::GetItemInSlot(unsigned int nSlot)
{
    unsigned long long* pOid = GetSlotPointer(nSlot);
    if (pOid)
        return g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(*pOid);
    return 0;
}

int CSWInventory::GetItemInInventory(CSWSItem* pItem)
{
    unsigned long long oid = pItem ? pItem->m_objectId : OBJECT_INVALID;
#if KOTOR1
    for (unsigned int i = 0; i < 18; i++)
#else
    for (unsigned int i = 0; i < 20; i++)
#endif
        if (m_aItemOids[i] == oid)
            return 1;
    return 0;
}

unsigned int CSWInventory::GetSlotFromItem(CSWSItem* pItem)
{
    unsigned long long oid = pItem ? pItem->m_objectId : OBJECT_INVALID;
#if KOTOR1
    for (unsigned int i = 0; i < 18; i++) {
#else
    for (unsigned int i = 0; i < 20; i++) {
#endif
        if (m_aItemOids[i] == oid) {
#if KOTOR1  // K1 carries 18 slot bits, contiguous
            return 1u << i;
#else
            if (i < 11)
                return 1u << i;
            return 8u << i;
#endif
        }
    }
    return 0;
}

int CSWInventory::RemoveItem(CSWSItem* pItem)
{
    unsigned long long oid = pItem ? pItem->m_objectId : OBJECT_INVALID;
#if KOTOR1
    for (unsigned int i = 0; i < 18; i++) {
#else
    for (unsigned int i = 0; i < 20; i++) {
#endif
        if (m_aItemOids[i] == oid) {
            m_aItemOids[i] = OBJECT_INVALID;
            return 1;
        }
    }
    return 0;
}

void CSWInventory::PutItemInSlot(unsigned int nSlot, CSWSItem* pItem)
{
    unsigned long long* pOid = GetSlotPointer(nSlot);
    if (pOid)
        *pOid = pItem->m_objectId;
}
