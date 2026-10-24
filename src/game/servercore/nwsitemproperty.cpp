#include "nwsitemproperty.h"
#include "nwitem.h"
#include "nwscriptconst.h"

// The duration type (CGameEffect::m_durationTypeBits, bits 0-2) of an item property's effects,
// which last while the item is equipped.
enum {
    EFFECT_DURATION_TYPE_EQUIPPED = 3,
};

CSWSItemPropertyHandler::~CSWSItemPropertyHandler()
{
    if (m_paOnApply) {
        delete[] m_paOnApply;
        m_paOnApply = 0;
    }
    if (m_paOnRemove) {
        delete[] m_paOnRemove;
        m_paOnRemove = 0;
    }
}

// Builds the pointer-to-member dispatch tables that the Apply and Remove slots index, one entry per
// item property type; types this build does not handle stay null. The per-property handlers each guard
// a null item, write one CSWSItem bonus or flag from the property, and return 0.
// Type 8 is assigned twice, the later attack penalty winning.
void CSWSItemPropertyHandler::InitializeItemProperties()
{
    m_paOnApply = new ApplyFn[NUM_ITEM_PROPERTY_TYPES];
    m_paOnRemove = new RemoveFn[NUM_ITEM_PROPERTY_TYPES];
    for (int i = 0; i < NUM_ITEM_PROPERTY_TYPES; i++)
    {
        m_paOnApply[i] = 0;
        m_paOnRemove[i] = 0;
    }

    m_paOnApply[ITEM_PROPERTY_ABILITY_BONUS] = &CSWSItemPropertyHandler::ApplyAbilityBonus;
    m_paOnRemove[ITEM_PROPERTY_ABILITY_BONUS] = &CSWSItemPropertyHandler::RemoveAbilityBonus;
    m_paOnApply[ITEM_PROPERTY_AC_BONUS] = &CSWSItemPropertyHandler::ApplyACBonus;
    m_paOnRemove[ITEM_PROPERTY_AC_BONUS] = &CSWSItemPropertyHandler::RemoveACBonus;
    m_paOnApply[ITEM_PROPERTY_AC_BONUS_VS_ALIGNMENT_GROUP] = &CSWSItemPropertyHandler::ApplyACBonus;
    m_paOnRemove[ITEM_PROPERTY_AC_BONUS_VS_ALIGNMENT_GROUP] = &CSWSItemPropertyHandler::RemoveACBonus;
    m_paOnApply[ITEM_PROPERTY_AC_BONUS_VS_DAMAGE_TYPE] = &CSWSItemPropertyHandler::ApplyACBonus;
    m_paOnRemove[ITEM_PROPERTY_AC_BONUS_VS_DAMAGE_TYPE] = &CSWSItemPropertyHandler::RemoveACBonus;
    m_paOnApply[ITEM_PROPERTY_AC_BONUS_VS_RACIAL_GROUP] = &CSWSItemPropertyHandler::ApplyACBonus;
    m_paOnRemove[ITEM_PROPERTY_AC_BONUS_VS_RACIAL_GROUP] = &CSWSItemPropertyHandler::RemoveACBonus;
    m_paOnApply[ITEM_PROPERTY_ENHANCEMENT_BONUS] = &CSWSItemPropertyHandler::ApplyEnhancementBonus;
    m_paOnRemove[ITEM_PROPERTY_ENHANCEMENT_BONUS] = &CSWSItemPropertyHandler::RemoveEnhancementBonus;
    m_paOnApply[ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP] = &CSWSItemPropertyHandler::ApplyEnhancementBonus;
    m_paOnRemove[ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP] = &CSWSItemPropertyHandler::RemoveEnhancementBonus;
    m_paOnApply[ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_RACIAL_GROUP] = &CSWSItemPropertyHandler::ApplyEnhancementBonus;
    m_paOnRemove[ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_RACIAL_GROUP] = &CSWSItemPropertyHandler::RemoveEnhancementBonus;
    m_paOnApply[ITEM_PROPERTY_ATTACK_PENALTY] = &CSWSItemPropertyHandler::ApplyEnhancementPenalty;
    m_paOnRemove[ITEM_PROPERTY_ATTACK_PENALTY] = &CSWSItemPropertyHandler::RemoveEnhancementPenalty;
    m_paOnApply[ITEM_PROPERTY_BONUS_FEAT] = &CSWSItemPropertyHandler::ApplyBonusFeat;
    m_paOnRemove[ITEM_PROPERTY_BONUS_FEAT] = &CSWSItemPropertyHandler::RemoveBonusFeat;
    m_paOnApply[ITEM_PROPERTY_DAMAGE_BONUS] = &CSWSItemPropertyHandler::ApplyDamageBonus;
    m_paOnRemove[ITEM_PROPERTY_DAMAGE_BONUS] = &CSWSItemPropertyHandler::RemoveDamageBonus;
    m_paOnApply[ITEM_PROPERTY_DAMAGE_BONUS_VS_ALIGNMENT_GROUP] = &CSWSItemPropertyHandler::ApplyDamageBonus;
    m_paOnRemove[ITEM_PROPERTY_DAMAGE_BONUS_VS_ALIGNMENT_GROUP] = &CSWSItemPropertyHandler::RemoveDamageBonus;
    m_paOnApply[ITEM_PROPERTY_DAMAGE_BONUS_VS_RACIAL_GROUP] = &CSWSItemPropertyHandler::ApplyDamageBonus;
    m_paOnRemove[ITEM_PROPERTY_DAMAGE_BONUS_VS_RACIAL_GROUP] = &CSWSItemPropertyHandler::RemoveDamageBonus;
    m_paOnApply[ITEM_PROPERTY_IMMUNITY_DAMAGE_TYPE] = &CSWSItemPropertyHandler::ApplyDamageImmunity;
    m_paOnRemove[ITEM_PROPERTY_IMMUNITY_DAMAGE_TYPE] = &CSWSItemPropertyHandler::RemoveDamageImmunity;
    m_paOnApply[ITEM_PROPERTY_DECREASED_DAMAGE] = &CSWSItemPropertyHandler::ApplyDamagePenalty;
    m_paOnRemove[ITEM_PROPERTY_DECREASED_DAMAGE] = &CSWSItemPropertyHandler::RemoveDamagePenalty;
    m_paOnApply[ITEM_PROPERTY_DAMAGE_REDUCTION] = &CSWSItemPropertyHandler::ApplyDamageReduction;
    m_paOnRemove[ITEM_PROPERTY_DAMAGE_REDUCTION] = &CSWSItemPropertyHandler::RemoveDamageReduction;
    m_paOnApply[ITEM_PROPERTY_DAMAGE_RESISTANCE] = &CSWSItemPropertyHandler::ApplyDamageResistance;
    m_paOnRemove[ITEM_PROPERTY_DAMAGE_RESISTANCE] = &CSWSItemPropertyHandler::RemoveDamageResistance;
    m_paOnApply[ITEM_PROPERTY_DAMAGE_VULNERABILITY] = &CSWSItemPropertyHandler::ApplyDamageVulnerability;
    m_paOnRemove[ITEM_PROPERTY_DAMAGE_VULNERABILITY] = &CSWSItemPropertyHandler::RemoveDamageVulnerability;
    m_paOnApply[ITEM_PROPERTY_DECREASED_ABILITY_SCORE] = &CSWSItemPropertyHandler::ApplyDecreaseAbilityScore;
    m_paOnRemove[ITEM_PROPERTY_DECREASED_ABILITY_SCORE] = &CSWSItemPropertyHandler::RemoveDecreaseAbilityScore;
    m_paOnApply[ITEM_PROPERTY_DECREASED_AC] = &CSWSItemPropertyHandler::ApplyDecreaseAC;
    m_paOnRemove[ITEM_PROPERTY_DECREASED_AC] = &CSWSItemPropertyHandler::RemoveDecreaseAC;
    m_paOnApply[ITEM_PROPERTY_DECREASED_SKILL_MODIFIER] = &CSWSItemPropertyHandler::ApplyDecreaseSkillModifier;
    m_paOnRemove[ITEM_PROPERTY_DECREASED_SKILL_MODIFIER] = &CSWSItemPropertyHandler::RemoveDecreaseSkillModifier;
    m_paOnApply[ITEM_PROPERTY_IMMUNITY] = &CSWSItemPropertyHandler::ApplyImmunity;
    m_paOnRemove[ITEM_PROPERTY_IMMUNITY] = &CSWSItemPropertyHandler::RemoveImmunity;
    m_paOnApply[ITEM_PROPERTY_IMPROVED_FORCE_RESISTANCE] = &CSWSItemPropertyHandler::ApplyImprovedForceResistance;
    m_paOnRemove[ITEM_PROPERTY_IMPROVED_FORCE_RESISTANCE] = &CSWSItemPropertyHandler::RemoveImprovedForceResistance;
    m_paOnApply[ITEM_PROPERTY_IMPROVED_SAVING_THROW] = &CSWSItemPropertyHandler::ApplyImprovedSavingThrow;
    m_paOnRemove[ITEM_PROPERTY_IMPROVED_SAVING_THROW] = &CSWSItemPropertyHandler::RemoveImprovedSavingThrow;
    m_paOnApply[ITEM_PROPERTY_IMPROVED_SAVING_THROW_SPECIFIC] = &CSWSItemPropertyHandler::ApplyImprovedSavingThrow;
    m_paOnRemove[ITEM_PROPERTY_IMPROVED_SAVING_THROW_SPECIFIC] = &CSWSItemPropertyHandler::RemoveImprovedSavingThrow;
    m_paOnApply[ITEM_PROPERTY_LIGHT] = &CSWSItemPropertyHandler::ApplyLight;
    m_paOnRemove[ITEM_PROPERTY_LIGHT] = &CSWSItemPropertyHandler::RemoveLight;
    m_paOnApply[ITEM_PROPERTY_DECREASED_SAVING_THROWS] = &CSWSItemPropertyHandler::ApplyReducedSavingThrow;
    m_paOnRemove[ITEM_PROPERTY_DECREASED_SAVING_THROWS] = &CSWSItemPropertyHandler::RemoveReducedSavingThrow;
    m_paOnApply[ITEM_PROPERTY_DECREASED_SAVING_THROWS_SPECIFIC] = &CSWSItemPropertyHandler::ApplyReducedSavingThrow;
    m_paOnRemove[ITEM_PROPERTY_DECREASED_SAVING_THROWS_SPECIFIC] = &CSWSItemPropertyHandler::RemoveReducedSavingThrow;
    m_paOnApply[ITEM_PROPERTY_REGENERATION] = &CSWSItemPropertyHandler::ApplyRegeneration;
    m_paOnRemove[ITEM_PROPERTY_REGENERATION] = &CSWSItemPropertyHandler::RemoveRegeneration;
    m_paOnApply[ITEM_PROPERTY_REGENERATION_FORCE_POINTS] = &CSWSItemPropertyHandler::ApplyRegeneration;
    m_paOnRemove[ITEM_PROPERTY_REGENERATION_FORCE_POINTS] = &CSWSItemPropertyHandler::RemoveRegeneration;
    m_paOnApply[ITEM_PROPERTY_SKILL_BONUS] = &CSWSItemPropertyHandler::ApplySkillBonus;
    m_paOnRemove[ITEM_PROPERTY_SKILL_BONUS] = &CSWSItemPropertyHandler::RemoveSkillBonus;
    m_paOnApply[ITEM_PROPERTY_ATTACK_BONUS] = &CSWSItemPropertyHandler::ApplyAttackBonus;
    m_paOnRemove[ITEM_PROPERTY_ATTACK_BONUS] = &CSWSItemPropertyHandler::RemoveAttackBonus;
    m_paOnApply[ITEM_PROPERTY_ATTACK_BONUS_VS_ALIGNMENT_GROUP] = &CSWSItemPropertyHandler::ApplyAttackBonus;
    m_paOnRemove[ITEM_PROPERTY_ATTACK_BONUS_VS_ALIGNMENT_GROUP] = &CSWSItemPropertyHandler::RemoveAttackBonus;
    m_paOnApply[ITEM_PROPERTY_ATTACK_BONUS_VS_RACIAL_GROUP] = &CSWSItemPropertyHandler::ApplyAttackBonus;
    m_paOnRemove[ITEM_PROPERTY_ATTACK_BONUS_VS_RACIAL_GROUP] = &CSWSItemPropertyHandler::RemoveAttackBonus;
    m_paOnApply[ITEM_PROPERTY_ATTACK_PENALTY] = &CSWSItemPropertyHandler::ApplyAttackPenalty;
    m_paOnRemove[ITEM_PROPERTY_ATTACK_PENALTY] = &CSWSItemPropertyHandler::RemoveAttackPenalty;
    m_paOnApply[ITEM_PROPERTY_TRUE_SEEING] = &CSWSItemPropertyHandler::ApplyTrueSeeing;
    m_paOnRemove[ITEM_PROPERTY_TRUE_SEEING] = &CSWSItemPropertyHandler::RemoveTrueSeeing;
    m_paOnApply[ITEM_PROPERTY_FREEDOM_OF_MOVEMENT] = &CSWSItemPropertyHandler::ApplyFreedomOfMovement;
    m_paOnRemove[ITEM_PROPERTY_FREEDOM_OF_MOVEMENT] = &CSWSItemPropertyHandler::RemoveFreedomOfMovement;
    m_paOnApply[ITEM_PROPERTY_SPECIAL_WALK] = &CSWSItemPropertyHandler::ApplySpecialWalk;
    m_paOnRemove[ITEM_PROPERTY_SPECIAL_WALK] = &CSWSItemPropertyHandler::RemoveSpecialWalk;
    m_paOnApply[ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_INCREASE] = &CSWSItemPropertyHandler::ApplyBlasterBoltDeflectionIncrease;
    m_paOnRemove[ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_INCREASE] = &CSWSItemPropertyHandler::RemoveBlasterBoltDeflectionIncrease;
    m_paOnApply[ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_DECREASE] = &CSWSItemPropertyHandler::ApplyBlasterBoltDeflectionDecrease;
    m_paOnRemove[ITEM_PROPERTY_BLASTER_BOLT_DEFLECT_DECREASE] = &CSWSItemPropertyHandler::RemoveBlasterBoltDeflectionDecrease;
    m_paOnApply[ITEM_PROPERTY_DISGUISE] = &CSWSItemPropertyHandler::ApplyDisguise;
    m_paOnRemove[ITEM_PROPERTY_DISGUISE] = &CSWSItemPropertyHandler::RemoveDisguise;
#if !KOTOR1
    m_paOnApply[ITEM_PROPERTY_DAMPEN_SOUND] = &CSWSItemPropertyHandler::ApplyDampenSound;
    m_paOnRemove[ITEM_PROPERTY_DAMPEN_SOUND] = &CSWSItemPropertyHandler::RemoveDampenSound;
    m_paOnApply[ITEM_PROPERTY_DOORCUTTING] = &CSWSItemPropertyHandler::ApplyDoorCutting;
    m_paOnApply[ITEM_PROPERTY_DOORSABERING] = &CSWSItemPropertyHandler::ApplyDoorSabering;
    m_paOnApply[ITEM_PROPERTY_SNIPER_BONUS] = &CSWSItemPropertyHandler::ApplySniperBonus;
    m_paOnApply[ITEM_PROPERTY_RAPID_SHOT_BONUS] = &CSWSItemPropertyHandler::ApplyRapidShotBonus;
    m_paOnApply[ITEM_PROPERTY_MAX_DEX_BONUS] = &CSWSItemPropertyHandler::ApplyMaxDexBonus;
    m_paOnRemove[ITEM_PROPERTY_MAX_DEX_BONUS] = &CSWSItemPropertyHandler::RemoveMaxDexBonus;
#endif
}

#include "nwsitem.h"

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSItemPropertyHandler::ApplyDoorCutting(CSWSItem* pItem, CSWItemProperty* pProp,
                                              CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(pProp); K2_UNUSED(pCreature); K2_UNUSED(a); K2_UNUSED(b);
    if (pItem)
        pItem->m_bDoorCutting = 1;
    return 0;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSItemPropertyHandler::ApplyDoorSabering(CSWSItem* pItem, CSWItemProperty* pProp,
                                               CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(pProp); K2_UNUSED(pCreature); K2_UNUSED(a); K2_UNUSED(b);
    if (pItem)
        pItem->m_bDoorSabering = 1;
    return 0;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSItemPropertyHandler::ApplySniperBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                              CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(pProp); K2_UNUSED(pCreature); K2_UNUSED(a); K2_UNUSED(b);
    if (pItem)
        pItem->m_bSniper = 1;
    return 0;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSItemPropertyHandler::ApplyRapidShotBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                                 CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(pProp); K2_UNUSED(pCreature); K2_UNUSED(a); K2_UNUSED(b);
    if (pItem)
        pItem->m_bRapidShot = 1;
    return 0;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSItemPropertyHandler::ApplyMaxDexBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                              CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(pCreature); K2_UNUSED(a); K2_UNUSED(b);
    if (pItem)
        pItem->m_nMaxDexBonus = pItem->m_nMaxDexBonus + pProp->m_costTableValue;
    return 0;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSItemPropertyHandler::RemoveMaxDexBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                               CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(pCreature); K2_UNUSED(a);
    if (pItem)
        pItem->m_nMaxDexBonus = pItem->m_nMaxDexBonus - pProp->m_costTableValue;
    return 0;
}
#endif // !KOTOR1


#include "nwscreature.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"

// Dispatches by indexing the table by the property's type, bounds-checking, then calling the
// handler so its verdict comes straight back.
int CSWSItemPropertyHandler::OnItemPropertyApplied(CSWSItem* pItem, CSWItemProperty* pProp,
                                                   CSWSCreature* pCreature, unsigned int a, int b)
{
    unsigned short nType = pProp->m_type;
    if (nType >= NUM_ITEM_PROPERTY_TYPES)
        return 0;

    ApplyFn pfn = m_paOnApply[nType];
    if (pfn != 0)
        return (this->*pfn)(pItem, pProp, pCreature, a, b);
    return 0;
}

// Slot 4, over m_paOnRemove, and without the trailing int argument.
int CSWSItemPropertyHandler::OnItemPropertyRemoved(CSWSItem* pItem, CSWItemProperty* pProp,
                                                   CSWSCreature* pCreature, unsigned int a)
{
    unsigned short nType = pProp->m_type;
    if (nType >= NUM_ITEM_PROPERTY_TYPES)
        return 0;

    RemoveFn pfn = m_paOnRemove[nType];
    if (pfn != 0)
        return (this->*pfn)(pItem, pProp, pCreature, a);
    return 0;
}

int CSWSItemPropertyHandler::ApplyFreedomOfMovement(CSWSItem* pItem, CSWItemProperty* pProp,
                                                    CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(pProp); K2_UNUSED(a);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_IMMUNITY);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, CSWSEffectListHandler::IMMUNITY_TYPE_PARALYSIS);
    pEffect->SetInteger(1, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pEffect, b, b);

    pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_IMMUNITY);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, CSWSEffectListHandler::IMMUNITY_TYPE_SLOW);
    pEffect->SetInteger(1, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pEffect, b, b);

    pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_IMMUNITY);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, CSWSEffectListHandler::IMMUNITY_TYPE_ENTANGLE);
    pEffect->SetInteger(1, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pEffect, b, b);

    pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_IMMUNITY);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, CSWSEffectListHandler::IMMUNITY_TYPE_MOVEMENT_SPEED_DECREASE);
    pEffect->SetInteger(1, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pEffect, b, b);

    return 0;
}

// Applies the attack-penalty property, reading the negated penalty out of the iprp cost table's
// "Value" column, then tagging the effect with the inventory slot it is applied through. A
// double-sided weapon (baseitems.2da WeaponWield 3) gets an off-hand copy as well.
int CSWSItemPropertyHandler::ApplyAttackPenalty(CSWSItem* pItem, CSWItemProperty* pProp,
                                                CSWSCreature* pCreature, unsigned int a, int b)
{
    CGameEffect* pEffect = new CGameEffect(1);

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;
#if KOTOR1  // K1: a zero penalty leaks the effect (kept)
    if (nValue == 0)
        return 0;
#else
    if (nValue == 0) {
        delete pEffect;
        return 0;
    }
#endif

    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ATTACK_DECREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);

    switch (a) {
    case CSWInventory::SLOT_HANDS: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_UNARMED);  break;
    case CSWInventory::SLOT_RIGHTWEAPON: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_ONHAND);   break;
    case CSWInventory::SLOT_LEFTWEAPON: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_OFFHAND);  break;
    case CSWInventory::SLOT_CWEAPON_L: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1); break;
    case CSWInventory::SLOT_CWEAPON_R: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON2); break;
    case CSWInventory::SLOT_CWEAPON_B: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON3); break;
    }

    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType ==
        CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
        CGameEffect* pOffHand = new CGameEffect(0);
        pOffHand->CopyEffect(pEffect, 0);
        pOffHand->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_OFFHAND);
        pCreature->ApplyEffect(pOffHand, b, 0);
    }
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// Two separate early-out guards, with the cost-table load between them.
int CSWSItemPropertyHandler::ApplyDamageBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                              CSWSCreature* pCreature, unsigned int a, int b)
{
    CGameEffect* pEffect = new CGameEffect(1);

    // The second weapon set's two slots never take a damage bonus this way.
    if (a == CSWInventory::SLOT_RIGHTWEAPON2 || a == CSWInventory::SLOT_LEFTWEAPON2) {
        delete pEffect;
        return 0;
    }
    int nAmount = pProp->m_costTableValue;
    if (nAmount == 0) {
        delete pEffect;
        return 0;
    }

    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, nAmount);

// The damage type arrives in the subtype on a vs-type row and elsewhere in the property itself.
    int nDamageType;
    if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS) {
        nDamageType = pProp->m_subType;
    } else {
        nDamageType = pProp->m_nParam1Value;
    }
    pEffect->SetInteger(1, 1 << nDamageType);

    switch (a) {
    case CSWInventory::SLOT_HANDS: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_UNARMED);  break;
    case CSWInventory::SLOT_RIGHTWEAPON: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_ONHAND);   break;
    case CSWInventory::SLOT_LEFTWEAPON: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND);  break;
    case CSWInventory::SLOT_CWEAPON_L: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1); break;
    case CSWInventory::SLOT_CWEAPON_R: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON2); break;
    case CSWInventory::SLOT_CWEAPON_B: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON3); break;
    }

    pEffect->SetInteger(2, g_pRules->m_nNumRaces);

// Re-read rather than cached, since the SetInteger call between the two uses may alias.
    if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS_VS_RACIAL_GROUP) {
        pEffect->SetInteger(2, pProp->m_subType);
    } else if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS_VS_ALIGNMENT_GROUP) {
        if (pProp->m_subType == ALIGNMENT_NEUTRAL) {
            pEffect->SetInteger(3, ALIGNMENT_NEUTRAL);
        } else if (pProp->m_subType == ALIGNMENT_LIGHT_SIDE) {
            pEffect->SetInteger(4, ALIGNMENT_LIGHT_SIDE);
        } else if (pProp->m_subType == ALIGNMENT_DARK_SIDE) {
            pEffect->SetInteger(4, ALIGNMENT_DARK_SIDE);
        }
    }

    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType ==
        CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
        CGameEffect* pOffHand = new CGameEffect(0);
        pOffHand->CopyEffect(pEffect, 0);
        pOffHand->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND);
        pCreature->ApplyEffect(pOffHand, b, 0);
    }
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

int CSWSItemPropertyHandler::ApplyDamagePenalty(CSWSItem* pItem, CSWItemProperty* pProp,
                                                CSWSCreature* pCreature, unsigned int a, int b)
{
    // The result is dropped; the base item is fetched again below for the twin-blade test.
    g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem);

    CGameEffect* pEffect = new CGameEffect(1);

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;
    if (nValue == 0) {
        delete pEffect;
        return 0;
    }

    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);

    switch (a) {
    case CSWInventory::SLOT_HANDS: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_UNARMED);  break;
    case CSWInventory::SLOT_RIGHTWEAPON: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_ONHAND);   break;
    case CSWInventory::SLOT_LEFTWEAPON: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND);  break;
    case CSWInventory::SLOT_CWEAPON_L: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1); break;
    case CSWInventory::SLOT_CWEAPON_R: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON2); break;
    case CSWInventory::SLOT_CWEAPON_B: pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON3); break;
    }

    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(1, pItem->GetDamageFlags());
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);

    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType ==
        CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
        CGameEffect* pOffHand = new CGameEffect(0);
        pOffHand->CopyEffect(pEffect, 0);
        pOffHand->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND);
        pCreature->ApplyEffect(pOffHand, b, 0);
    }
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

int CSWSItemPropertyHandler::RemoveAbilityBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                                CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_BONUS);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);

    for (int i = pCreature->m_pStats->m_iFirstAbilityIncrease; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_ABILITY_INCREASE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_ABILITY_INCREASE &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(1) == nValue)
        {
            switch (pProp->m_subType)
            {
            case ABILITY_STRENGTH:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_STRENGTH)
                    continue;
                break;
            case ABILITY_DEXTERITY:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_DEXTERITY)
                    continue;
                break;
            case ABILITY_CONSTITUTION:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_CONSTITUTION)
                    continue;
                break;
            case ABILITY_INTELLIGENCE:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_INTELLIGENCE)
                    continue;
                break;
            case ABILITY_WISDOM:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_WISDOM)
                    continue;
                break;
            case ABILITY_CHARISMA:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_CHARISMA)
                    continue;
                break;
            }
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

int CSWSItemPropertyHandler::RemoveDecreaseAbilityScore(CSWSItem* pItem, CSWItemProperty* pProp,
                                                        CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG10);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;

    for (int i = pCreature->m_pStats->m_iFirstAbilityIncrease; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_ABILITY_DECREASE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_ABILITY_DECREASE &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(1) == nValue)
        {
            switch (pProp->m_subType)
            {
            case ABILITY_STRENGTH:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_STRENGTH)
                    continue;
                break;
            case ABILITY_DEXTERITY:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_DEXTERITY)
                    continue;
                break;
            case ABILITY_CONSTITUTION:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_CONSTITUTION)
                    continue;
                break;
            case ABILITY_INTELLIGENCE:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_INTELLIGENCE)
                    continue;
                break;
            case ABILITY_WISDOM:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_WISDOM)
                    continue;
                break;
            case ABILITY_CHARISMA:
                if (pEffect->GetInteger(0) != CSWSEffectListHandler::EFFECT_ABILITY_CHARISMA)
                    continue;
                break;
            }
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

int CSWSItemPropertyHandler::ApplyTrueSeeing(CSWSItem* pItem, CSWItemProperty* pProp,
                                             CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(pProp); K2_UNUSED(a);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_TRUESEEING);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

int CSWSItemPropertyHandler::RemoveTrueSeeing(CSWSItem* pItem, CSWItemProperty* pProp,
                                              CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(pProp); K2_UNUSED(a);
    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_TRUESEEING &&
            pEffect->m_creatorId == pItem->m_objectId &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSItemPropertyHandler::ApplyDampenSound(CSWSItem* pItem, CSWItemProperty* pProp,
                                              CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(pItem); K2_UNUSED(pProp); K2_UNUSED(a); K2_UNUSED(b);
    pCreature->SetSilentMove(1);
    return 0;
}

int CSWSItemPropertyHandler::RemoveDampenSound(CSWSItem* pItem, CSWItemProperty* pProp,
                                               CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(pItem); K2_UNUSED(pProp); K2_UNUSED(a);
    pCreature->SetSilentMove(0);
    return 0;
}
#endif // !KOTOR1

int CSWSItemPropertyHandler::ApplyBlasterBoltDeflectionIncrease(CSWSItem* pItem, CSWItemProperty* pProp,
                                                                CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    unsigned short nAmount = pProp->m_costTableValue;
    if (nAmount == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONINCREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, pProp->m_subType);
    pEffect->SetInteger(1, nAmount);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

int CSWSItemPropertyHandler::RemoveBlasterBoltDeflectionIncrease(CSWSItem* pItem, CSWItemProperty* pProp,
                                                                 CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    unsigned short nValue = pProp->m_costTableValue;
    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONINCREASE &&
            pEffect->GetInteger(0) == nValue &&
            pEffect->m_creatorId == pItem->m_objectId &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

int CSWSItemPropertyHandler::ApplyBlasterBoltDeflectionDecrease(CSWSItem* pItem, CSWItemProperty* pProp,
                                                                CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    unsigned short nAmount = pProp->m_costTableValue;
    if (nAmount == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONDECREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, pProp->m_subType);
    pEffect->SetInteger(1, nAmount);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

int CSWSItemPropertyHandler::RemoveBlasterBoltDeflectionDecrease(CSWSItem* pItem, CSWItemProperty* pProp,
                                                                 CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    unsigned short nValue = pProp->m_costTableValue;
    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONDECREASE &&
            pEffect->GetInteger(0) == nValue &&
            pEffect->m_creatorId == pItem->m_objectId &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Applies the cosmetic-disguise property, with integer 0 the race to look like, taken from the
// subtype.
int CSWSItemPropertyHandler::ApplyDisguise(CSWSItem* pItem, CSWItemProperty* pProp,
                                           CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    unsigned short nRace = pProp->m_subType;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DISGUISE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, nRace);
    pEffect->SetInteger(1, 0);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

int CSWSItemPropertyHandler::RemoveDisguise(CSWSItem* pItem, CSWItemProperty* pProp,
                                            CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    unsigned short nRace = pProp->m_subType;
    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DISGUISE &&
            pEffect->GetInteger(0) == nRace &&
            pEffect->m_creatorId == pItem->m_objectId &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Installs two effects per application, the walk-animation override (its integer 0 is the animation
// id from the property's subtype) and the matching movement-speed limit, both behind a duration
// rewrite.
int CSWSItemPropertyHandler::ApplySpecialWalk(CSWSItem* pItem, CSWItemProperty* pProp,
                                             CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETWALKANIMATION);
    pEffect->SetInteger(0, pProp->m_subType);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pCreature->ApplyEffect(pEffect, b, b);

    pEffect = new CGameEffect(1);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_LIMIT_MOVEMENT_SPEED);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pCreature->ApplyEffect(pEffect, b, b);
    return 0;
}

int CSWSItemPropertyHandler::RemoveSpecialWalk(CSWSItem* pItem, CSWItemProperty* pProp,
                                               CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    for (int i = pCreature->m_pStats->m_iFirstSetWalkAnimation;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_LIMIT_MOVEMENT_SPEED)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_SETWALKANIMATION &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(0) == pProp->m_subType &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
        }
        else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_LIMIT_MOVEMENT_SPEED &&
                 pEffect->m_creatorId == pItem->m_objectId &&
                 (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Drops the four immunity effects ApplyFreedomOfMovement installs.
int CSWSItemPropertyHandler::RemoveFreedomOfMovement(CSWSItem* pItem, CSWItemProperty* pProp,
                                                     CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(pProp); K2_UNUSED(a);
    for (int i = pCreature->m_pStats->m_iFirstImmunity; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_IMMUNITY)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_IMMUNITY &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId &&
            (pEffect->GetInteger(0) == CSWSEffectListHandler::IMMUNITY_TYPE_PARALYSIS ||
             pEffect->GetInteger(0) == CSWSEffectListHandler::IMMUNITY_TYPE_SLOW ||
             pEffect->GetInteger(0) == CSWSEffectListHandler::IMMUNITY_TYPE_ENTANGLE ||
             pEffect->GetInteger(0) == CSWSEffectListHandler::IMMUNITY_TYPE_MOVEMENT_SPEED_DECREASE))
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
        }
    }
    return 0;
}

// Applies the damage-immunity property, with integer 0 the immunity flag bit selected by the
// subtype and integer 1 the value out of the iprp cost table's "Value" column.
int CSWSItemPropertyHandler::ApplyDamageImmunity(CSWSItem* pItem, CSWItemProperty* pProp,
                                                 CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(pProp->m_nCostTable);
    if (pCostTable == 0)
        return 0;

    unsigned short nSubType = pProp->m_subType;
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_INCREASE);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | EFFECT_DURATION_TYPE_EQUIPPED;
    pEffect->SetInteger(0, 1 << nSubType);
    pEffect->SetInteger(1, nValue);
    pEffect->SetCreator(pItem->m_objectId);
    pCreature->ApplyEffect(pEffect, b, b);
    return 0;
}

// The damage-type flag bit for one itempropdef damage subtype, in subtype order.
static const int* const s_apDamageFlags[] = {
    &CSWSEffectListHandler::DAMAGE_FLAG_BLUDGEONING,
    &CSWSEffectListHandler::DAMAGE_FLAG_PIERCING,
    &CSWSEffectListHandler::DAMAGE_FLAG_SLASHING,
    &CSWSEffectListHandler::DAMAGE_FLAG_UNIVERSAL,
    &CSWSEffectListHandler::DAMAGE_FLAG_ACID,
    &CSWSEffectListHandler::DAMAGE_FLAG_COLD,
    &CSWSEffectListHandler::DAMAGE_FLAG_LIGHT_SIDE,
    &CSWSEffectListHandler::DAMAGE_FLAG_ELECTRICAL,
    &CSWSEffectListHandler::DAMAGE_FLAG_FIRE,
    &CSWSEffectListHandler::DAMAGE_FLAG_DARK_SIDE,
    &CSWSEffectListHandler::DAMAGE_FLAG_SONIC,
    &CSWSEffectListHandler::DAMAGE_FLAG_ION,
    &CSWSEffectListHandler::DAMAGE_FLAG_BLASTER,
    &CSWSEffectListHandler::DAMAGE_FLAG_POISON,
};

int CSWSItemPropertyHandler::RemoveDamageImmunity(CSWSItem* pItem, CSWItemProperty* pProp,
                                                  CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    short nSubType = pProp->m_subType;

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(pProp->m_nCostTable);
    if (pCostTable == 0)
        return 0;

    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);

    if (static_cast<unsigned short>(nSubType) >= 14)
        return 0;

    int nFlag = *s_apDamageFlags[nSubType];
    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_INCREASE &&
            pEffect->GetInteger(0) == nFlag &&
            pEffect->GetInteger(1) == nValue &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Applies the generic immunity property, with integer 0 one of the IMMUNITY_TYPE ids and integer 1
// the race limiter. Subtype 1 (immunity to ability decrease) also installs an ABILITY_DECREASE
// effect next to its primary NEGATIVE_LEVEL one.
int CSWSItemPropertyHandler::ApplyImmunity(CSWSItem* pItem, CSWItemProperty* pProp,
                                           CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    const int* pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_BACKSTAB;
    int nExtraImmunityType K2_UNSET;
    int bExtra = 0;
    switch (pProp->m_subType)
    {
        case IPRP_IMMUNITY_BACKSTAB:
            break;
        case IPRP_IMMUNITY_LEVEL_ABILITY_DRAIN:
            nExtraImmunityType = CSWSEffectListHandler::IMMUNITY_TYPE_ABILITY_DECREASE;
            bExtra = 1;
            pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_NEGATIVE_LEVEL;
            break;
        case IPRP_IMMUNITY_MIND_SPELLS:   pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_MIND_AFFECTING; break;
        case IPRP_IMMUNITY_POISON:        pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_POISON;         break;
        case IPRP_IMMUNITY_DISEASE:       pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_DISEASE;        break;
        case IPRP_IMMUNITY_FEAR:          pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_FEAR;           break;
        case IPRP_IMMUNITY_KNOCKDOWN:     pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_KNOCKDOWN;      break;
        case IPRP_IMMUNITY_PARALYSIS:     pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_PARALYSIS;      break;
        case IPRP_IMMUNITY_CRITICAL_HITS: pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_CRITICAL_HITS;  break;
        case IPRP_IMMUNITY_DEATH_MAGIC:   pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_DEATH_MAGIC;    break;
        default:
            return 0;
    }
    int nImmunityType = *pnImmunityType;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_IMMUNITY);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | EFFECT_DURATION_TYPE_EQUIPPED;
    pEffect->SetInteger(0, nImmunityType);
    pEffect->SetInteger(1, g_pRules->m_nNumRaces);
    pEffect->SetCreator(pItem->m_objectId);
    pCreature->ApplyEffect(pEffect, b, b);

    if (!bExtra)
        return 0;

    pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_IMMUNITY);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | EFFECT_DURATION_TYPE_EQUIPPED;
    pEffect->SetInteger(0, nExtraImmunityType);
    pEffect->SetInteger(1, g_pRules->m_nNumRaces);
    pEffect->SetCreator(pItem->m_objectId);
    pCreature->ApplyEffect(pEffect, b, b);
    return 0;
}

// Drops the effects ApplyImmunity installed: the matching primary, and for subtype 1 also the
// ABILITY_DECREASE companion.
int CSWSItemPropertyHandler::RemoveImmunity(CSWSItem* pItem, CSWItemProperty* pProp,
                                            CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    const int* pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_BACKSTAB;
    int nExtraImmunityType K2_UNSET;
    int bExtra = 0;
    switch (pProp->m_subType)
    {
        case IPRP_IMMUNITY_BACKSTAB:
            break;
        case IPRP_IMMUNITY_LEVEL_ABILITY_DRAIN:
            nExtraImmunityType = CSWSEffectListHandler::IMMUNITY_TYPE_ABILITY_DECREASE;
            bExtra = 1;
            pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_NEGATIVE_LEVEL;
            break;
        case IPRP_IMMUNITY_MIND_SPELLS:   pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_MIND_AFFECTING; break;
        case IPRP_IMMUNITY_POISON:        pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_POISON;         break;
        case IPRP_IMMUNITY_DISEASE:       pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_DISEASE;        break;
        case IPRP_IMMUNITY_FEAR:          pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_FEAR;           break;
        case IPRP_IMMUNITY_KNOCKDOWN:     pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_KNOCKDOWN;      break;
        case IPRP_IMMUNITY_PARALYSIS:     pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_PARALYSIS;      break;
        case IPRP_IMMUNITY_CRITICAL_HITS: pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_CRITICAL_HITS;  break;
        case IPRP_IMMUNITY_DEATH_MAGIC:   pnImmunityType = &CSWSEffectListHandler::IMMUNITY_TYPE_DEATH_MAGIC;    break;
        default:
            return 0;
    }
    int nImmunityType = *pnImmunityType;

    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_IMMUNITY &&
            pEffect->GetInteger(0) == nImmunityType &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
        if (bExtra &&
            pEffect->m_subType == CSWSEffectListHandler::EFFECT_IMMUNITY &&
            pEffect->GetInteger(0) == nExtraImmunityType &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Applies the force-resistance property, putting the cost table's "Value" into the
// EFFECT_FORCE_RESISTANCE_INCREASE effect's integer 0. Unlike the damage-immunity family there is
// no null-cost-table guard, and a zero value installs nothing.
int CSWSItemPropertyHandler::ApplyImprovedForceResistance(CSWSItem* pItem, CSWItemProperty* pProp,
                                                          CSWSCreature* pCreature, unsigned int a,
                                                          int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_SPELL_RESIST);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_INCREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, nValue);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// Scans from m_pStats->m_iFirstForceResistanceIncrease, the effect list being sorted by subtype;
// the EFFECT_FORCE_RESISTANCE_INCREASE test bounds the scan.
int CSWSItemPropertyHandler::RemoveImprovedForceResistance(CSWSItem* pItem, CSWItemProperty* pProp,
                                                           CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_SPELL_RESIST);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);

    for (int i = pCreature->m_pStats->m_iFirstForceResistanceIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_INCREASE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_INCREASE &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(0) == nValue)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Grants the feat the property's subtype names. The m_pStats null guard comes before the
// allocation.
int CSWSItemPropertyHandler::ApplyBonusFeat(CSWSItem* pItem, CSWItemProperty* pProp,
                                            CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    if (pCreature->m_pStats == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_BONUSFEAT);
    pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | EFFECT_DURATION_TYPE_EQUIPPED;
    pEffect->SetInteger(0, pProp->m_subType);
    pEffect->SetCreator(pItem->m_objectId);
    pCreature->ApplyEffect(pEffect, b, b);
    return 0;
}

// The condition order (subType, creator, integer 0) is this handler's own, and the duration-type
// test comes last.
int CSWSItemPropertyHandler::RemoveBonusFeat(CSWSItem* pItem, CSWItemProperty* pProp,
                                             CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    if (pCreature->m_pStats == 0)
        return 0;

    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_BONUSFEAT &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(0) == pProp->m_subType &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Installs the light effect only when the item already carries a light property (the use limiter),
// so this handler never creates light on its own.
int CSWSItemPropertyHandler::ApplyLight(CSWSItem* pItem, CSWItemProperty* pProp,
                                        CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(pProp); K2_UNUSED(a);
    if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_LIGHT, 0) == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_LIGHT);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, 0x1388);   // the feedback string, cloned through by OnApplyLight
    pCreature->ApplyEffect(pEffect, b, b);
    return 0;
}

// The light effect is not owned per item, so there is nothing to drop.
int CSWSItemPropertyHandler::RemoveLight(CSWSItem* pItem, CSWItemProperty* pProp,
                                         CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(pItem); K2_UNUSED(pProp); K2_UNUSED(pCreature); K2_UNUSED(a);
    return 0;
}

// Applies the skill-bonus property, with integer 0 the skill id (the property's subtype), integer 1
// the bonus and integer 2 the any-race sentinel. The zero test wraps the whole body, the allocation
// included.
int CSWSItemPropertyHandler::ApplySkillBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                             CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    unsigned short nValue = pProp->m_costTableValue;
    if (nValue == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SKILL_INCREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, pProp->m_subType);
    pEffect->SetInteger(1, nValue);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// The sorted-list hint is m_iFirstAttackIncrease, where the shared attack and skill increase block
// starts.
int CSWSItemPropertyHandler::RemoveSkillBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                              CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    for (int i = pCreature->m_pStats->m_iFirstAttackIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_SKILL_INCREASE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_SKILL_INCREASE &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(0) == pProp->m_subType &&
            pEffect->GetInteger(1) == pProp->m_costTableValue)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Applies the regeneration property, with integer 0 hit points per tick (the property's
// m_costTableValue), integer 1 the 6000 ms tick interval and integer 4 the property type; slots 2
// and 3 are the world-time stamp. The effect is allocated before the zero-value test, so the
// early-out has to delete it.
int CSWSItemPropertyHandler::ApplyRegeneration(CSWSItem* pItem, CSWItemProperty* pProp,
                                               CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    CGameEffect* pEffect = new CGameEffect(1);

    unsigned short nValue = pProp->m_costTableValue;
    if (nValue == 0)
    {
        delete pEffect;
        return 0;
    }

    pEffect->SetCreator(pItem->m_objectId);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_REGENERATE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(1, 6000);
    pEffect->SetInteger(4, pProp->m_type);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// No sorted first index here, so the scan starts at 0.
int CSWSItemPropertyHandler::RemoveRegeneration(CSWSItem* pItem, CSWItemProperty* pProp,
                                                CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    unsigned short nValue = pProp->m_costTableValue;
    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_REGENERATE &&
            pEffect->GetInteger(0) == nValue &&
            pEffect->m_creatorId == pItem->m_objectId &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// The "Amount" column of iprp_damagereduction holds the reduction points; integer 1 is the subtype
// biased by one, since zero is not a valid row.
int CSWSItemPropertyHandler::ApplyDamageReduction(CSWSItem* pItem, CSWItemProperty* pProp,
                                                  CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_DAMAGE_SOAK);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Amount"), &nValue);

    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(1, pProp->m_subType + 1);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

int CSWSItemPropertyHandler::RemoveDamageReduction(CSWSItem* pItem, CSWItemProperty* pProp,
                                                   CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_DAMAGE_SOAK);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Amount"), &nValue);

    for (int i = pCreature->m_pStats->m_iFirstDamageReduction;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(0) == nValue &&
            pEffect->GetInteger(1) == pProp->m_subType + 1)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Integer 0 is the damage-type flag bit (the subtype is the damage type directly here, unlike the
// immunity handler's table), integer 1 the amount out of the cost table.
int CSWSItemPropertyHandler::ApplyDamageResistance(CSWSItem* pItem, CSWItemProperty* pProp,
                                                   CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_DAMAGE_RESIST);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Amount"), &nValue);

    pEffect->SetInteger(0, 1 << pProp->m_subType);
    pEffect->SetInteger(1, nValue);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

int CSWSItemPropertyHandler::RemoveDamageResistance(CSWSItem* pItem, CSWItemProperty* pProp,
                                                    CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    unsigned short nType = pProp->m_subType;

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_DAMAGE_RESIST);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Amount"), &nValue);

    int nFlag = 1 << nType;
    for (int i = pCreature->m_pStats->m_iFirstDamageResistance;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(0) == nFlag &&
            pEffect->GetInteger(1) == nValue)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// Installs EFFECT_DAMAGE_IMMUNITY_DECREASE for the vulnerability property. The subtype is a
// damage-type bit index, and a vulnerability is the negative immunity.
int CSWSItemPropertyHandler::ApplyDamageVulnerability(CSWSItem* pItem, CSWItemProperty* pProp,
                                                      CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    CGameEffect* pEffect = new CGameEffect(1);
    unsigned short nType = pProp->m_subType;
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_DECREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_DAMAGE_VULNERABILITY);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);

    pEffect->SetInteger(0, 1 << nType);
    pEffect->SetInteger(1, nValue);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

int CSWSItemPropertyHandler::RemoveDamageVulnerability(CSWSItem* pItem, CSWItemProperty* pProp,
                                                       CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    unsigned short nType = pProp->m_subType;

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_DAMAGE_VULNERABILITY);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);

    int nFlag = 1 << nType;
    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_DECREASE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_DECREASE &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(0) == nFlag &&
            pEffect->GetInteger(1) == nValue)
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// The verification twin of ApplyDamageBonus. It re-derives the same damage-type bit, the same wield
// slot and the same type-specific trailing payload, and removes the effect only when all match.
int CSWSItemPropertyHandler::RemoveDamageBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                               CSWSCreature* pCreature, unsigned int a)
{
    int nAmount = pProp->m_costTableValue;

    int nDamageType;
    if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS)
        nDamageType = pProp->m_subType;
    else
        nDamageType = pProp->m_nParam1Value;
    int nFlag = 1 << nDamageType;

    for (int i = pCreature->m_pStats->m_iFirstDamageIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE)
            break;
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE)
            continue;
        if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
            continue;
        if (pEffect->m_creatorId != pItem->m_objectId)
            continue;
        if (pEffect->GetInteger(0) != nAmount)
            continue;

        switch (a)
        {
        case CSWInventory::SLOT_RIGHTWEAPON:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_ONHAND)
                continue;
            break;
        case CSWInventory::SLOT_LEFTWEAPON:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_L:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_R:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON2)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_B:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON3)
                continue;
            break;
        case CSWInventory::SLOT_HANDS:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_UNARMED)
                continue;
            break;
        default:
// An unknown slot never matches: the test falls back to the loop.
            continue;
        }

        if (pEffect->GetInteger(1) != nFlag)
            continue;

        if (pProp->m_type >= ITEM_PROPERTY_DAMAGE_BONUS && pProp->m_type <= ITEM_PROPERTY_DAMAGE_BONUS_VS_ALIGNMENT_GROUP) {
            if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
                continue;
            if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS_VS_ALIGNMENT_GROUP) {
                if (pProp->m_subType == ALIGNMENT_NEUTRAL) {
                    if (pEffect->GetInteger(3) != ALIGNMENT_NEUTRAL)
                        continue;
                } else if (pProp->m_subType == ALIGNMENT_DARK_SIDE) {
                    if (pEffect->GetInteger(4) != ALIGNMENT_DARK_SIDE)
                        continue;
                } else if (pProp->m_subType == ALIGNMENT_LIGHT_SIDE) {
                    if (pEffect->GetInteger(4) != ALIGNMENT_LIGHT_SIDE)
                        continue;
                }
            }
        } else if (pProp->m_type == ITEM_PROPERTY_DAMAGE_BONUS_VS_RACIAL_GROUP) {
            if (pEffect->GetInteger(2) != pProp->m_subType)
                continue;
        }

        pCreature->RemoveEffectById(pEffect->m_effectId);
        break;
    }
    return 0;
}

// The EFFECT_DAMAGE_DECREASE twin of RemoveDamageBonus, with the same negated value row and the
// same six wield slots, but the trailing test is the damage-type bit against
// CSWSItem::GetDamageFlags() instead of the type payload. The subType early-out bounds the scan.
int CSWSItemPropertyHandler::RemoveDamagePenalty(CSWSItem* pItem, CSWItemProperty* pProp,
                                                 CSWSCreature* pCreature, unsigned int a)
{
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;

    for (int i = pCreature->m_pStats->m_iFirstAttackIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE)
            break;
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE)
            continue;
        if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
            continue;
        if (pEffect->m_creatorId != pItem->m_objectId)
            continue;
        if (pEffect->GetInteger(0) != nValue)
            continue;
        if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
            continue;

        switch (a)
        {
        case CSWInventory::SLOT_RIGHTWEAPON:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_ONHAND)
                continue;
            break;
        case CSWInventory::SLOT_LEFTWEAPON:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_L:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_R:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON2)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_B:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON3)
                continue;
            break;
        case CSWInventory::SLOT_HANDS:
            if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_UNARMED)
                continue;
            break;
        default:
// An unknown slot never matches: the range tests fall back to the loop.
            continue;
        }

        if (pEffect->GetInteger(1) & pItem->GetDamageFlags()) {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// The EFFECT_ATTACK_DECREASE twin of RemoveDamagePenalty, with the same negated value row and the
// same six wield slots, but the slot constant lives in integer 1 and there is no damage-flags test
// after the switch. One scan covers both decrease subtypes.
int CSWSItemPropertyHandler::RemoveAttackPenalty(CSWSItem* pItem, CSWItemProperty* pProp,
                                                 CSWSCreature* pCreature, unsigned int a)
{
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;

    for (int i = pCreature->m_pStats->m_iFirstAttackIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE)
            break;
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_ATTACK_DECREASE)
            continue;
        if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
            continue;
        if (pEffect->m_creatorId != pItem->m_objectId)
            continue;
        if (pEffect->GetInteger(0) != nValue)
            continue;
        if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
            continue;

        switch (a)
        {
        case CSWInventory::SLOT_RIGHTWEAPON:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_ONHAND)
                continue;
            break;
        case CSWInventory::SLOT_LEFTWEAPON:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_OFFHAND)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_L:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_R:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON2)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_B:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON3)
                continue;
            break;
        case CSWInventory::SLOT_HANDS:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_UNARMED)
                continue;
            break;
        default:
// An unknown slot never matches: the range tests fall back to the loop.
            continue;
        }

        pCreature->RemoveEffectById(pEffect->m_effectId);
        break;
    }
    return 0;
}

// The attack-bonus twin of ApplyDamageBonus differs in three ways: the cost table is the direct
// m_pIPRPMelee member, a zero value aborts without freeing the already-allocated effect, and the
// type-specific payload keys off the attack-bonus property types.
int CSWSItemPropertyHandler::ApplyAttackBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                              CSWSCreature* pCreature, unsigned int a, int b)
{
    CGameEffect* pEffect = new CGameEffect(1);

    int nValue;
    g_pRules->m_p2DArrays->m_pIPRPMelee->GetINTEntry(pProp->m_costTableValue,
                                                     CExoString("Value"), &nValue);
    if (nValue == 0)
        return 0;

    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ATTACK_INCREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, nValue);

    switch (a)
    {
    case CSWInventory::SLOT_HANDS: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_UNARMED);  break;
    case CSWInventory::SLOT_RIGHTWEAPON: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_ONHAND);   break;
    case CSWInventory::SLOT_LEFTWEAPON: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_OFFHAND);  break;
    case CSWInventory::SLOT_CWEAPON_L: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1); break;
    case CSWInventory::SLOT_CWEAPON_R: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON2); break;
    case CSWInventory::SLOT_CWEAPON_B: pEffect->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON3); break;
    }

    pEffect->SetInteger(2, g_pRules->m_nNumRaces);

    if (pProp->m_type == ITEM_PROPERTY_ATTACK_BONUS_VS_ALIGNMENT_GROUP) {
        if (pProp->m_subType == ALIGNMENT_NEUTRAL) {
            pEffect->SetInteger(3, ALIGNMENT_NEUTRAL);
        } else if (pProp->m_subType == ALIGNMENT_LIGHT_SIDE) {
            pEffect->SetInteger(4, ALIGNMENT_LIGHT_SIDE);
        } else if (pProp->m_subType == ALIGNMENT_DARK_SIDE) {
            pEffect->SetInteger(4, ALIGNMENT_DARK_SIDE);
        }
    } else if (pProp->m_type == ITEM_PROPERTY_ATTACK_BONUS_VS_RACIAL_GROUP) {
        pEffect->SetInteger(2, pProp->m_subType);
    }

    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType ==
        CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
        CGameEffect* pOffHand = new CGameEffect(0);
        pOffHand->CopyEffect(pEffect, 0);
        pOffHand->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_OFFHAND);
        pCreature->ApplyEffect(pOffHand, b, 0);
    }
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// The verification twin of ApplyAttackBonus, and structurally RemoveDamageBonus's twin, sharing the
// direct m_pIPRPMelee value row, the six wield slots, the races check and a per-type trailing
// payload.
int CSWSItemPropertyHandler::RemoveAttackBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                               CSWSCreature* pCreature, unsigned int a)
{
    int nValue;
    g_pRules->m_p2DArrays->m_pIPRPMelee->GetINTEntry(pProp->m_costTableValue,
                                                     CExoString("Value"), &nValue);

    for (int i = pCreature->m_pStats->m_iFirstAttackIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_ATTACK_INCREASE)
            break;
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_ATTACK_INCREASE)
            continue;
        if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
            continue;
        if (pEffect->m_creatorId != pItem->m_objectId)
            continue;
        if (pEffect->GetInteger(0) != nValue)
            continue;

        switch (a)
        {
        case CSWInventory::SLOT_RIGHTWEAPON:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_ONHAND)
                continue;
            break;
        case CSWInventory::SLOT_LEFTWEAPON:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_OFFHAND)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_L:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_R:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON2)
                continue;
            break;
        case CSWInventory::SLOT_CWEAPON_B:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON3)
                continue;
            break;
        case CSWInventory::SLOT_HANDS:
            if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_UNARMED)
                continue;
            break;
        default:
// An unknown slot never matches: the range tests fall back to the loop.
            continue;
        }

        if ((pProp->m_type & 0xfffe) == ITEM_PROPERTY_ATTACK_BONUS) {
            if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
                continue;
            if (pProp->m_type == ITEM_PROPERTY_ATTACK_BONUS_VS_ALIGNMENT_GROUP) {
                if (pProp->m_subType == ALIGNMENT_NEUTRAL) {
                    if (pEffect->GetInteger(3) != ALIGNMENT_NEUTRAL)
                        continue;
                } else if (pProp->m_subType == ALIGNMENT_DARK_SIDE) {
                    if (pEffect->GetInteger(4) != ALIGNMENT_DARK_SIDE)
                        continue;
                } else if (pProp->m_subType == ALIGNMENT_LIGHT_SIDE) {
                    if (pEffect->GetInteger(4) != ALIGNMENT_LIGHT_SIDE)
                        continue;
                }
            }
        } else if (pProp->m_type == ITEM_PROPERTY_ATTACK_BONUS_VS_RACIAL_GROUP) {
            if (pEffect->GetInteger(2) != pProp->m_subType)
                continue;
        }

        pCreature->RemoveEffectById(pEffect->m_effectId);
        break;
    }
    return 0;
}

// The enhancement-bonus property installs two effects in one pass: the melee attack bonus
// (EFFECT_ATTACK_INCREASE, the wield slot in integer 1) and the matching damage bonus
// (EFFECT_DAMAGE_INCREASE, the wield slot in integer 5, the damage-type mask in integer 1).
// Both halves carry the same per-type trailing payload that the plain
// ApplyAttackBonus/ApplyDamageBonus pair does not.
int CSWSItemPropertyHandler::ApplyEnhancementBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                                   CSWSCreature* pCreature, unsigned int a, int b)
{
// The second weapon set's two slots never take an enhancement bonus this way.
    if (a == CSWInventory::SLOT_RIGHTWEAPON2 || a == CSWInventory::SLOT_LEFTWEAPON2)
        return 0;

    g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem);

    CGameEffect* pAttack = new CGameEffect(1);
    CGameEffect* pDamage = new CGameEffect(1);

    int nValue;
    g_pRules->m_p2DArrays->m_pIPRPMelee->GetINTEntry(pProp->m_costTableValue,
                                                     CExoString("Value"), &nValue);
    if (nValue == 0)
        return 0;

    pAttack->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ATTACK_INCREASE);
    pAttack->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pAttack->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pAttack->SetCreator(pItem->m_objectId);
    pAttack->SetInteger(0, nValue);

    switch (a) {
    case CSWInventory::SLOT_HANDS: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_UNARMED);  break;
    case CSWInventory::SLOT_RIGHTWEAPON: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_ONHAND);   break;
    case CSWInventory::SLOT_LEFTWEAPON: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_OFFHAND);  break;
    case CSWInventory::SLOT_CWEAPON_L: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1); break;
    case CSWInventory::SLOT_CWEAPON_R: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON2); break;
    case CSWInventory::SLOT_CWEAPON_B: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON3); break;
    }

    pAttack->SetInteger(2, g_pRules->m_nNumRaces);

    if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP) {
        if (pProp->m_subType == ALIGNMENT_NEUTRAL) {
            pAttack->SetInteger(3, ALIGNMENT_NEUTRAL);
        } else if (pProp->m_subType == ALIGNMENT_LIGHT_SIDE) {
            pAttack->SetInteger(4, ALIGNMENT_LIGHT_SIDE);
        } else if (pProp->m_subType == ALIGNMENT_DARK_SIDE) {
            pAttack->SetInteger(4, ALIGNMENT_DARK_SIDE);
        }
    } else if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_RACIAL_GROUP) {
        pAttack->SetInteger(2, pProp->m_subType);
    }

    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType ==
        CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
        CGameEffect* pOffHand = new CGameEffect(0);
        pOffHand->CopyEffect(pAttack, 0);
        pOffHand->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_OFFHAND);
        pCreature->ApplyEffect(pOffHand, b, 0);
    }
    pCreature->ApplyEffect(pAttack, b, 0);

    pDamage->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE);
    pDamage->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pDamage->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pDamage->SetCreator(pItem->m_objectId);

    switch (a) {
    case CSWInventory::SLOT_HANDS: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_UNARMED);  break;
    case CSWInventory::SLOT_RIGHTWEAPON: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_ONHAND);   break;
    case CSWInventory::SLOT_LEFTWEAPON: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND);  break;
    case CSWInventory::SLOT_CWEAPON_L: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1); break;
    case CSWInventory::SLOT_CWEAPON_R: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON2); break;
    case CSWInventory::SLOT_CWEAPON_B: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON3); break;
    }

    pDamage->SetInteger(0, nValue);
    pDamage->SetInteger(1, pItem->GetDamageFlags());
    pDamage->SetInteger(2, g_pRules->m_nNumRaces);

    if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP) {
        if (pProp->m_subType == ALIGNMENT_NEUTRAL) {
            pDamage->SetInteger(3, ALIGNMENT_NEUTRAL);
        } else if (pProp->m_subType == ALIGNMENT_LIGHT_SIDE) {
            pDamage->SetInteger(4, ALIGNMENT_LIGHT_SIDE);
        } else if (pProp->m_subType == ALIGNMENT_DARK_SIDE) {
            pDamage->SetInteger(4, ALIGNMENT_DARK_SIDE);
        }
    } else if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_RACIAL_GROUP) {
        pDamage->SetInteger(2, pProp->m_subType);
    }

    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType ==
        CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
        CGameEffect* pOffHand = new CGameEffect(0);
        pOffHand->CopyEffect(pDamage, 0);
        pOffHand->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND);
        pCreature->ApplyEffect(pOffHand, b, 0);
    }
    pCreature->ApplyEffect(pDamage, b, 0);
    return 0;
}

// The verification twin of ApplyEnhancementBonus. One scan matches either the attack-increase or
// the
// damage-increase effect, re-deriving the same wield slot (integer 1 against integer 5), the same
// per-type payload and, for the damage half, the damage-type mask in integer 1. The attack half
// restarts the scan after a removal because the array shifts; the damage half breaks out.
int CSWSItemPropertyHandler::RemoveEnhancementBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                                    CSWSCreature* pCreature, unsigned int a)
{
    int nValue;
    g_pRules->m_p2DArrays->m_pIPRPMelee->GetINTEntry(pProp->m_costTableValue,
                                                     CExoString("Value"), &nValue);

    for (int i = pCreature->m_pStats->m_iFirstAttackIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_ATTACK_INCREASE) {
            if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
                continue;
            if (pEffect->m_creatorId != pItem->m_objectId)
                continue;
            if (pEffect->GetInteger(0) != nValue)
                continue;

            switch (a) {
            case CSWInventory::SLOT_HANDS:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_UNARMED)
                    continue;
                break;
            case CSWInventory::SLOT_RIGHTWEAPON:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_ONHAND)
                    continue;
                break;
            case CSWInventory::SLOT_LEFTWEAPON:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_OFFHAND)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_L:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_R:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON2)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_B:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON3)
                    continue;
                break;
            default:
// An unknown slot never matches.
                continue;
            }

            if (pProp->m_type >= ITEM_PROPERTY_ENHANCEMENT_BONUS && pProp->m_type <= ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP) {
                if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
                    continue;
                if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP) {
                    if (pProp->m_subType == ALIGNMENT_NEUTRAL) {
                        if (pEffect->GetInteger(3) != ALIGNMENT_NEUTRAL)
                            continue;
                    } else if (pProp->m_subType == ALIGNMENT_LIGHT_SIDE) {
                        if (pEffect->GetInteger(4) != ALIGNMENT_LIGHT_SIDE)
                            continue;
                    } else if (pProp->m_subType == ALIGNMENT_DARK_SIDE) {
                        if (pEffect->GetInteger(4) != ALIGNMENT_DARK_SIDE)
                            continue;
                    }
                }
            } else if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_RACIAL_GROUP) {
                if (pEffect->GetInteger(2) != pProp->m_subType)
                    continue;
            }

            pCreature->RemoveEffectById(pEffect->m_effectId);
            i = pCreature->m_pStats->m_iFirstAttackIncrease - 1;
        } else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE) {
            if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
                continue;
            if (pEffect->m_creatorId != pItem->m_objectId)
                continue;
            if (pEffect->GetInteger(0) != nValue)
                continue;

            switch (a) {
            case CSWInventory::SLOT_HANDS:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_UNARMED)
                    continue;
                break;
            case CSWInventory::SLOT_RIGHTWEAPON:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_ONHAND)
                    continue;
                break;
            case CSWInventory::SLOT_LEFTWEAPON:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_L:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_R:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON2)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_B:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON3)
                    continue;
                break;
            default:
// An unknown slot never matches.
                continue;
            }

            if (!(pEffect->GetInteger(1) & pItem->GetDamageFlags()))
                continue;

            if (pProp->m_type >= ITEM_PROPERTY_ENHANCEMENT_BONUS && pProp->m_type <= ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP) {
                if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
                    continue;
                if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_ALIGNMENT_GROUP) {
                    if (pProp->m_subType == ALIGNMENT_NEUTRAL) {
                        if (pEffect->GetInteger(3) != ALIGNMENT_NEUTRAL)
                            continue;
                    } else if (pProp->m_subType == ALIGNMENT_LIGHT_SIDE) {
                        if (pEffect->GetInteger(4) != ALIGNMENT_LIGHT_SIDE)
                            continue;
                    } else if (pProp->m_subType == ALIGNMENT_DARK_SIDE) {
                        if (pEffect->GetInteger(4) != ALIGNMENT_DARK_SIDE)
                            continue;
                    }
                }
            } else if (pProp->m_type == ITEM_PROPERTY_ENHANCEMENT_BONUS_VS_RACIAL_GROUP) {
                if (pEffect->GetInteger(2) != pProp->m_subType)
                    continue;
            }

            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// The negative twin of ApplyEnhancementBonus, doing the same two-effect install (attack decrease
// plus damage decrease) reading the negated value out of the cost table, but with no per-type
// payload and no second-weapon-set early-out.
int CSWSItemPropertyHandler::ApplyEnhancementPenalty(CSWSItem* pItem, CSWItemProperty* pProp,
                                                     CSWSCreature* pCreature, unsigned int a, int b)
{
    g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem);

    CGameEffect* pAttack = new CGameEffect(1);
    CGameEffect* pDamage = new CGameEffect(1);

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;
    if (nValue == 0)
        return 0;

    pAttack->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ATTACK_DECREASE);
    pAttack->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pAttack->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pAttack->SetCreator(pItem->m_objectId);
    pAttack->SetInteger(0, nValue);
    pAttack->SetInteger(2, g_pRules->m_nNumRaces);

    switch (a) {
    case CSWInventory::SLOT_HANDS: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_UNARMED);  break;
    case CSWInventory::SLOT_RIGHTWEAPON: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_ONHAND);   break;
    case CSWInventory::SLOT_LEFTWEAPON: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_OFFHAND);  break;
    case CSWInventory::SLOT_CWEAPON_L: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1); break;
    case CSWInventory::SLOT_CWEAPON_R: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON2); break;
    case CSWInventory::SLOT_CWEAPON_B: pAttack->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_CWEAPON3); break;
    }

    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType ==
        CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
        CGameEffect* pOffHand = new CGameEffect(0);
        pOffHand->CopyEffect(pAttack, 0);
        pOffHand->SetInteger(1, CSWSEffectListHandler::ATTACK_BONUS_OFFHAND);
        pCreature->ApplyEffect(pOffHand, b, 0);
    }
    pCreature->ApplyEffect(pAttack, b, 0);

    pDamage->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE);
    pDamage->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pDamage->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pDamage->SetCreator(pItem->m_objectId);

    switch (a) {
    case CSWInventory::SLOT_HANDS: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_UNARMED);  break;
    case CSWInventory::SLOT_RIGHTWEAPON: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_ONHAND);   break;
    case CSWInventory::SLOT_LEFTWEAPON: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND);  break;
    case CSWInventory::SLOT_CWEAPON_L: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1); break;
    case CSWInventory::SLOT_CWEAPON_R: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON2); break;
    case CSWInventory::SLOT_CWEAPON_B: pDamage->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON3); break;
    }

    pDamage->SetInteger(0, nValue);
    pDamage->SetInteger(1, pItem->GetDamageFlags());
    pDamage->SetInteger(2, g_pRules->m_nNumRaces);

    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType ==
        CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
        CGameEffect* pOffHand = new CGameEffect(0);
        pOffHand->CopyEffect(pDamage, 0);
        pOffHand->SetInteger(5, CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND);
        pCreature->ApplyEffect(pOffHand, b, 0);
    }
    pCreature->ApplyEffect(pDamage, b, 0);
    return 0;
}

// RemoveEnhancementBonus's negative twin, with the same one-scan, two-branch shape, but the attack
// half matches EFFECT_ATTACK_DECREASE (no trailing payload, no races check) and the damage half
// matches EFFECT_DAMAGE_DECREASE with the races check before the wield switch. The attack half
// restarts the scan after a removal.
int CSWSItemPropertyHandler::RemoveEnhancementPenalty(CSWSItem* pItem, CSWItemProperty* pProp,
                                                      CSWSCreature* pCreature, unsigned int a)
{
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;

    for (int i = pCreature->m_pStats->m_iFirstAttackIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_ATTACK_DECREASE) {
            if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
                continue;
            if (pEffect->m_creatorId != pItem->m_objectId)
                continue;
            if (pEffect->GetInteger(0) != nValue)
                continue;
            if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
                continue;

            switch (a) {
            case CSWInventory::SLOT_HANDS:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_UNARMED)
                    continue;
                break;
            case CSWInventory::SLOT_RIGHTWEAPON:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_ONHAND)
                    continue;
                break;
            case CSWInventory::SLOT_LEFTWEAPON:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_OFFHAND)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_L:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_R:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON2)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_B:
                if (pEffect->GetInteger(1) != CSWSEffectListHandler::ATTACK_BONUS_CWEAPON3)
                    continue;
                break;
            default:
// An unknown slot never matches.
                continue;
            }

            pCreature->RemoveEffectById(pEffect->m_effectId);
            i = pCreature->m_pStats->m_iFirstAttackIncrease - 1;
        } else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE) {
            if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
                continue;
            if (pEffect->m_creatorId != pItem->m_objectId)
                continue;
            if (pEffect->GetInteger(0) != nValue)
                continue;
            if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
                continue;

            switch (a) {
            case CSWInventory::SLOT_HANDS:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_UNARMED)
                    continue;
                break;
            case CSWInventory::SLOT_RIGHTWEAPON:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_ONHAND)
                    continue;
                break;
            case CSWInventory::SLOT_LEFTWEAPON:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_L:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_R:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON2)
                    continue;
                break;
            case CSWInventory::SLOT_CWEAPON_B:
                if (pEffect->GetInteger(5) != CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON3)
                    continue;
                break;
            default:
// An unknown slot never matches.
                continue;
            }

            if (pEffect->GetInteger(1) & pItem->GetDamageFlags()) {
                pCreature->RemoveEffectById(pEffect->m_effectId);
                break;
            }
        }
    }
    return 0;
}

// Applies the ability-bonus property, the cost table's "Value", to the ability the subtype names.
// The subtype is dispatched through a jump table.
int CSWSItemPropertyHandler::ApplyAbilityBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                               CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_BONUS);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_INCREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);

    switch (pProp->m_subType) {
    case ABILITY_STRENGTH:     pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_STRENGTH);     break;
    case ABILITY_DEXTERITY:    pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_DEXTERITY);    break;
    case ABILITY_CONSTITUTION: pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_CONSTITUTION); break;
    case ABILITY_INTELLIGENCE: pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_INTELLIGENCE); break;
    case ABILITY_WISDOM:       pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_WISDOM);       break;
    case ABILITY_CHARISMA:     pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_CHARISMA);     break;
    }

    pEffect->SetInteger(1, nValue);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// The AC-bonus property installs one AC-increase effect: integer 0 is the base item's own AC
// byte (the identity RemoveACBonus compares against), integer 1 the cost table's "Value", integer
// 5 the damage-type slot (the physical type unless the property names one) and integer 2 the race
// limiter. Property types 2, 3 and 4 each rewrite one of those slots.
int CSWSItemPropertyHandler::ApplyACBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                          CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    CGameEffect* pEffect = new CGameEffect(1);

    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_BONUS);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue == 0) {
        delete pEffect;
        return 0;
    }

    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_INCREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_nACEnchant);
    pEffect->SetInteger(1, nValue);
    pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);

    if (pProp->m_type == ITEM_PROPERTY_AC_BONUS_VS_ALIGNMENT_GROUP) {
// An unsigned range test, so subtype 0 falls through.
        if (pProp->m_subType >= ALIGNMENT_NEUTRAL && pProp->m_subType <= ALIGNMENT_DARK_SIDE)
            pEffect->SetInteger(4, pProp->m_subType);
    } else if (pProp->m_type == ITEM_PROPERTY_AC_BONUS_VS_DAMAGE_TYPE) {
        pEffect->SetInteger(5, pProp->m_subType);
    } else if (pProp->m_type == ITEM_PROPERTY_AC_BONUS_VS_RACIAL_GROUP) {
        pEffect->SetInteger(2, pProp->m_subType);
    }

    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// ApplyACBonus's verification twin. One scan over the AC-increase run re-derives integer 0 (the
// base
// item byte), integer 1 (the value) and the same per-type slot rewrite.
int CSWSItemPropertyHandler::RemoveACBonus(CSWSItem* pItem, CSWItemProperty* pProp,
                                           CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_BONUS);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);

    CSWBaseItem* pBaseItem = g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem);

    for (int i = pCreature->m_pStats->m_iFirstACIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_AC_INCREASE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_AC_INCREASE &&
            (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_EQUIPPED &&
            pEffect->m_creatorId == pItem->m_objectId &&
            pEffect->GetInteger(0) == pBaseItem->m_nACEnchant &&
            pEffect->GetInteger(1) == nValue)
        {
            if (pProp->m_type == ITEM_PROPERTY_AC_BONUS_VS_ALIGNMENT_GROUP) {
                if (pProp->m_subType >= ALIGNMENT_NEUTRAL && pProp->m_subType <= ALIGNMENT_DARK_SIDE) {
                    if (pEffect->GetInteger(4) != pProp->m_subType)
                        continue;
                }
            } else if (pProp->m_type == ITEM_PROPERTY_AC_BONUS_VS_DAMAGE_TYPE) {
                if (pEffect->GetInteger(5) != pProp->m_subType)
                    continue;
            } else if (pProp->m_type == ITEM_PROPERTY_AC_BONUS_VS_RACIAL_GROUP) {
                if (pEffect->GetInteger(2) != pProp->m_subType)
                    continue;
            }

            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// ApplyAbilityBonus's negative twin, with the same ability-id dispatch and jump table, but the
// value comes out of a lower cost-table row, negated, and is only checked for zero after the
// negation.
int CSWSItemPropertyHandler::ApplyDecreaseAbilityScore(CSWSItem* pItem, CSWItemProperty* pProp,
                                                       CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG10);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;
    if (nValue == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);

    switch (pProp->m_subType) {
    case ABILITY_STRENGTH:     pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_STRENGTH);     break;
    case ABILITY_DEXTERITY:    pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_DEXTERITY);    break;
    case ABILITY_CONSTITUTION: pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_CONSTITUTION); break;
    case ABILITY_INTELLIGENCE: pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_INTELLIGENCE); break;
    case ABILITY_WISDOM:       pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_WISDOM);       break;
    case ABILITY_CHARISMA:     pEffect->SetInteger(0, CSWSEffectListHandler::EFFECT_ABILITY_CHARISMA);     break;
    }

    pEffect->SetInteger(1, nValue);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// Applies the AC penalty, with integer 0 the property subtype (not the base item's AC byte as in
// ApplyACBonus) and integer 5 the damage-type slot, always the physical type here.
int CSWSItemPropertyHandler::ApplyDecreaseAC(CSWSItem* pItem, CSWItemProperty* pProp,
                                             CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;
    if (nValue == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_DECREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, pProp->m_subType);
    pEffect->SetInteger(1, nValue);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    pEffect->SetInteger(5, CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// ApplyDecreaseAC's verification twin, matching the same negated value, the property subtype in
// integer 0 and the races limiter in integer 2.
int CSWSItemPropertyHandler::RemoveDecreaseAC(CSWSItem* pItem, CSWItemProperty* pProp,
                                              CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;

    for (int i = pCreature->m_pStats->m_iFirstACIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_AC_DECREASE)
            break;
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_AC_DECREASE)
            continue;
        if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
            continue;
        if (pEffect->m_creatorId != pItem->m_objectId)
            continue;
        if (pEffect->GetInteger(0) != pProp->m_subType)
            continue;
        if (pEffect->GetInteger(1) != nValue)
            continue;
        if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
            continue;

        pCreature->RemoveEffectById(pEffect->m_effectId);
        break;
    }
    return 0;
}

// Applies the skill penalty. The subtype is the skill id (integer 0), the negated value the amount
// in integer 1, and integer 2 the race limiter.
int CSWSItemPropertyHandler::ApplyDecreaseSkillModifier(CSWSItem* pItem, CSWItemProperty* pProp,
                                                        CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG10);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;
    if (nValue == 0)
        return 0;

    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SKILL_DECREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);
    pEffect->SetInteger(0, pProp->m_subType);
    pEffect->SetInteger(1, nValue);
    pEffect->SetInteger(2, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// ApplyDecreaseSkillModifier's verification twin. It has no zero-value early-out: it negates and scans
// unconditionally.
int CSWSItemPropertyHandler::RemoveDecreaseSkillModifier(CSWSItem* pItem, CSWItemProperty* pProp,
                                                         CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG10);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    nValue = -nValue;

    for (int i = pCreature->m_pStats->m_iFirstSkillIncrease;
         i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_SKILL_DECREASE)
            break;
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_SKILL_DECREASE)
            continue;
        if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
            continue;
        if (pEffect->m_creatorId != pItem->m_objectId)
            continue;
        if (pEffect->GetInteger(0) != pProp->m_subType)
            continue;
        if (pEffect->GetInteger(1) != nValue)
            continue;
        if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
            continue;

        pCreature->RemoveEffectById(pEffect->m_effectId);
        break;
    }
    return 0;
}

// In the saving-throw family, eight wrappers read one cost-table value and hand off to the four
// ChangedSavingThrow cores, which pick the effect subtype from the sign of that value.

// The sign of nValue picks the effect subtype (increase or decrease, the latter negated so the
// effect always carries a magnitude); integer 1 is the save type, integer 2 is left zero and
// integer 3 the race limiter.
int CSWSItemPropertyHandler::ApplyChangedSavingThrow(CSWSItem* pItem, int nValue,
                                                     CSWItemProperty* pProp, CSWSCreature* pCreature, int b)
{
    CGameEffect* pEffect = new CGameEffect(1);
    if (nValue >= 0) {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE);
    } else {
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE);
        nValue = -nValue;
    }
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(pItem->m_objectId);

    unsigned short nSaveType = pProp->m_subType - 1;
    if (nSaveType < 3)
        pEffect->SetInteger(1, nSaveType + 1);

    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(2, 0);
    pEffect->SetInteger(3, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// The "vs. X" variant, where the discriminator lands in integer 2 instead of integer 1 and there is
// no sign-driven subtype branch.
int CSWSItemPropertyHandler::ApplyChangedSavingThrowVsX(CSWSItem* pItem, int nValue,
                                                        CSWItemProperty* pProp, CSWSCreature* pCreature, int b)
{
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>((nValue >= 0)
                                                     ? CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE
                                                     : CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_EQUIPPED |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    if (nValue < 0)
        nValue = -nValue;
    pEffect->SetCreator(pItem->m_objectId);

    pEffect->SetInteger(0, nValue);
    pEffect->SetInteger(1, 0);
    pEffect->SetInteger(3, g_pRules->m_nNumRaces);
    pEffect->SetInteger(2, pProp->m_subType);
    pCreature->ApplyEffect(pEffect, b, 0);
    return 0;
}

// ApplyChangedSavingThrow's verification twin. The save type is matched against integer 1, and the
// per-type arm is three explicit pair tests rather than a switch.
int CSWSItemPropertyHandler::RemoveChangedSavingThrow(int nValue, CSWSItem* pItem,
                                                      CSWItemProperty* pProp, CSWSCreature* pCreature)
{
    unsigned short nSaveType = pProp->m_subType;

    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE &&
            pEffect->m_subType != CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE)
            continue;
        if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
            continue;
        if (pEffect->m_creatorId != pItem->m_objectId)
            continue;
        if (pEffect->GetInteger(0) != nValue)
            continue;
        if (pEffect->GetInteger(1) != 0)
            continue;
        if (pEffect->GetInteger(3) != g_pRules->m_nNumRaces)
            continue;

        int nType = pEffect->GetInteger(1);
        if ((nSaveType == 1 && nType == 1) ||
            (nSaveType == 3 && nType == 3) ||
            (nSaveType == 2 && nType == 2))
        {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

// The "vs. X" removal does the same scan and the same integer-1 and integer-3 tests, but the
// discriminators are compared in integer 2 and a zero/zero pair counts as a match too.
int CSWSItemPropertyHandler::RemoveChangedSavingThrowVsX(int nValue, CSWSItem* pItem,
                                                         CSWItemProperty* pProp, CSWSCreature* pCreature)
{
    for (int i = 0; i < pCreature->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE &&
            pEffect->m_subType != CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE)
            continue;
        if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_EQUIPPED)
            continue;
        if (pEffect->m_creatorId != pItem->m_objectId)
            continue;
        if (pEffect->GetInteger(0) != nValue)
            continue;
        if (pEffect->GetInteger(1) != 0)
            continue;
        if (pEffect->GetInteger(3) != g_pRules->m_nNumRaces)
            continue;

        int nVs = pEffect->GetInteger(2);
        if (pProp->m_subType == nVs || (pProp->m_subType | nVs) == 0) {
            pCreature->RemoveEffectById(pEffect->m_effectId);
            break;
        }
    }
    return 0;
}

int CSWSItemPropertyHandler::ApplyImprovedSavingThrow(CSWSItem* pItem, CSWItemProperty* pProp,
                                                      CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_BONUS);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue != 0)
        ApplyChangedSavingThrow(pItem, nValue, pProp, pCreature, b);
    return 0;
}

int CSWSItemPropertyHandler::RemoveImprovedSavingThrow(CSWSItem* pItem, CSWItemProperty* pProp,
                                                       CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_BONUS);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue != 0)
        RemoveChangedSavingThrow(nValue, pItem, pProp, pCreature);
    return 0;
}

int CSWSItemPropertyHandler::ApplyReducedSavingThrow(CSWSItem* pItem, CSWItemProperty* pProp,
                                                     CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue != 0)
        ApplyChangedSavingThrow(pItem, nValue, pProp, pCreature, b);
    return 0;
}

int CSWSItemPropertyHandler::RemoveReducedSavingThrow(CSWSItem* pItem, CSWItemProperty* pProp,
                                                      CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue != 0)
        RemoveChangedSavingThrow(nValue, pItem, pProp, pCreature);
    return 0;
}

int CSWSItemPropertyHandler::ApplyImprovedSavingThrowVsX(CSWSItem* pItem, CSWItemProperty* pProp,
                                                         CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_BONUS);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue != 0)
        ApplyChangedSavingThrowVsX(pItem, nValue, pProp, pCreature, b);
    return 0;
}

int CSWSItemPropertyHandler::RemoveImprovedSavingThrowVsX(CSWSItem* pItem, CSWItemProperty* pProp,
                                                          CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_BONUS);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue != 0)
        RemoveChangedSavingThrowVsX(nValue, pItem, pProp, pCreature);
    return 0;
}

int CSWSItemPropertyHandler::ApplyReducedSavingThrowVsX(CSWSItem* pItem, CSWItemProperty* pProp,
                                                        CSWSCreature* pCreature, unsigned int a, int b)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue != 0)
        ApplyChangedSavingThrowVsX(pItem, nValue, pProp, pCreature, b);
    return 0;
}

int CSWSItemPropertyHandler::RemoveReducedSavingThrowVsX(CSWSItem* pItem, CSWItemProperty* pProp,
                                                         CSWSCreature* pCreature, unsigned int a)
{
    K2_UNUSED(a);
    C2DA* pCostTable = g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_NEG5);
    int nValue;
    pCostTable->GetINTEntry(pProp->m_costTableValue, CExoString("Value"), &nValue);
    if (nValue != 0)
        RemoveChangedSavingThrowVsX(nValue, pItem, pProp, pCreature);
    return 0;
}

// An unconditional no-op. Improved evasion is a creature feat, so an item property has nothing to
// apply.
int CSWSItemPropertyHandler::ApplyImprovedEvasion(CSWSItem* pItem, CSWItemProperty* pProp, CSWSCreature* pCreature,
                                                  unsigned int a, int b)
{
    K2_UNUSED(pItem); K2_UNUSED(pProp); K2_UNUSED(pCreature); K2_UNUSED(a); K2_UNUSED(b);
    return 0;
}

// The matching no-op teardown.
int CSWSItemPropertyHandler::RemoveImprovedEvasion(CSWSItem* pItem, CSWItemProperty* pProp, CSWSCreature* pCreature,
                                                   unsigned int a)
{
    K2_UNUSED(pItem); K2_UNUSED(pProp); K2_UNUSED(pCreature); K2_UNUSED(a);
    return 0;
}
