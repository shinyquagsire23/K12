#pragma once

#include "exostring.h"

// One row of racialtypes.2da, owned by CSWRules as a new'd array. Only the fields the engine reads
// are named; the rest is padding.
class CSWRace {
public:
    static const unsigned short CRACE_HUMAN;
    static const unsigned short CRACE_DROID;
    static const unsigned short CRACE_HUMAN_MASK;
    static const unsigned short CRACE_DROID_MASK;
    static const unsigned short CRACE_PLACEABLE_MASK;
    // The inclusive upper bound of the valid racialtypes.2da id range.
    static const unsigned short CRACE_ALL_RACES;

    int            m_nNameStrRef;
    int            m_nConverNameStrRef;
    int            m_nConverNameLowerStrRef;
    int            m_nNamePluralStrRef;
    int            m_nDescriptionStrRef;
    int            m_nDefaultBiographyStrRef;
    // Per-ability racial score adjustment, in the order STR, DEX, INT, CHA, WIS, CON.
    signed char    m_anAbilityAdjust[6];
    // racialtypes.2da "Endurance".
    unsigned char  m_nEndurance;
    // racialtypes.2da "Favored", the race's favored class id; the sentinel means "highest class".
    unsigned char  m_nFavoredClass;
    // racialtypes.2da "PlayerRace".
    int            m_bPlayerRace;
    // racialtypes.2da's own first-level bonus feat list, distinct from a class's granted feats.
    unsigned short  m_nNumFirstLevelGrantedFeats;
    unsigned char   _pad26[0x28 - 0x26];
    unsigned short* m_paFirstLevelGrantedFeats;
    // racialtypes.2da "Age".
    int             m_nAge;
    // racialtypes.2da "Appearance", a row index into appearance.2da.
    unsigned short  m_nAppearance;
    unsigned char  _pad36[0x38 - 0x36];

    CExoString GetNameText();
    CExoString GetNamePluralText();
    CExoString GetDescriptionText();
    CExoString GetDefaultBiographyText();
    CSWRace();
    ~CSWRace();
    // Gendered conversation display name, with a lowercase variant.
    CExoString GetConverNameText();
    CExoString GetConverNameLowerText();
    int IsFirstLevelGrantedFeat(unsigned short nFeat);
    // Rebuilds m_paFirstLevelGrantedFeats from a racialtypes.2da "FeatsTable" column value.
    void LoadFeatsTable(const CExoString& sTable);
};
