#pragma once

// The itemtype column of baseitems.2da (CSWBaseItem::m_nItemType), for the types the item, creature
// and GUI code test.
enum {
    ITEM_TYPE_DOUBLE_WEAPON          = 0,   // quarter staff, stun baton, double-bladed sword, ...
    ITEM_TYPE_SHORT_BLADE            = 1,   // short sword, vibroblade, Gamorrean battleaxe
    ITEM_TYPE_LONG_BLADE             = 2,   // long sword, vibrosword
    ITEM_TYPE_PISTOL                 = 3,
    ITEM_TYPE_RIFLE                  = 4,
    ITEM_TYPE_HEAVY_WEAPON           = 5,   // repeating and heavy repeating blaster
    ITEM_TYPE_GRENADE                = 6,
    ITEM_TYPE_DROID_UTILITY_DEVICE   = 12,
    ITEM_TYPE_DROID_SHIELD           = 13,
    ITEM_TYPE_DROID_PLATING          = 15,
    ITEM_TYPE_GAUNTLETS              = 19,
    ITEM_TYPE_FOREARM_BANDS          = 20,
    ITEM_TYPE_CREDITS                = 23,
    ITEM_TYPE_DATA_PAD               = 24,
    ITEM_TYPE_STIMULANT              = 25,
    ITEM_TYPE_DROID_REPAIR_EQUIPMENT = 26,
    ITEM_TYPE_SPIKES                 = 27,
    ITEM_TYPE_TRAP_KIT               = 28,
    ITEM_TYPE_LIGHT_SOURCE           = 29,  // glow rod, collar light, torch
    ITEM_TYPE_ARMOR_CLASS_4          = 31,
    ITEM_TYPE_ARMOR_CLASS_5          = 32,
    ITEM_TYPE_ARMOR_CLASS_6          = 33,
    ITEM_TYPE_ARMOR_CLASS_7          = 34,
    ITEM_TYPE_ARMOR_CLASS_8          = 35,
    ITEM_TYPE_ARMOR_CLASS_9          = 36,
    ITEM_TYPE_CLOTHING               = 37,
    ITEM_TYPE_JEDI_ROBE              = 38,
    ITEM_TYPE_DOUBLE_LIGHTSABER      = 39,
    ITEM_TYPE_SHORT_LIGHTSABER       = 40,
    ITEM_TYPE_LIGHTSABER             = 41,
    // The lightsaber types are consecutive from DOUBLE_LIGHTSABER.
    ITEM_TYPE_LIGHTSABER_COUNT       = 3,
    ITEM_TYPE_PAZAAK_CARD            = 42,
    ITEM_TYPE_PAZAAK_SIDEBOARD       = 43,
    ITEM_TYPE_STEALTH_UNIT           = 44,
    ITEM_TYPE_MEDICAL_EQUIPMENT      = 45,
    ITEM_TYPE_LIGHTSABER_CRYSTAL     = 46,
    ITEM_TYPE_SQUAD_RECOVERY_KIT     = 47,
    ITEM_TYPE_WRIST_LAUNCHER         = 48,
    ITEM_TYPE_ROCKET                 = 49,
    ITEM_TYPE_CHEMICALS              = 50,
    ITEM_TYPE_COMPONENTS             = 51,
};
