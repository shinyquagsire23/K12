#pragma once

#include "exostring.h"

// One row of skill.2da, owned by CSWRules as a heap array.
class CSWSkill {
public:
    // m_flags bits, from skills.2da's columns of the same names.
    enum {
        FLAG_HOSTILE             = 0x01,
        FLAG_UNTRAINED           = 0x02,
        FLAG_ARMOR_CHECK_PENALTY = 0x04,
        FLAG_ALL_CLASSES_CAN_USE = 0x08,
        FLAG_NPC_CAN_USE         = 0x10,
        FLAG_DROID_CAN_USE       = 0x20,
        FLAG_MASK                = 0x3f,
    };

    CSWSkill();
    ~CSWSkill();

    // The skill's display name as a fresh CExoString (talk-table lookup of the name StrRef).
    CExoString GetNameText();

    // The skill's description paragraph as a fresh CExoString (talk-table lookup of the description
    // StrRef).
    CExoString GetDescriptionText();

    int           m_nNameStrRef;
    int           m_nDescriptionStrRef;
    int           m_nCategory;
    int           m_nMaxCR;
    CExoString    m_icon;
    int           m_nKeyAbility;
    unsigned char m_flags;
};
