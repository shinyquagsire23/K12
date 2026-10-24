#pragma once
#include "k2layout.h"

#include "exostring.h"

// One row of CServerInfo's server-option table: a category and option name to match on, the member
// the row reads and writes, and how to parse its value.
struct ServerOption {
    const char* m_szCategory;
    const char* m_szName;
    int*        m_pValue;   // 0 marks the sentinel row
    int         m_nType;    // 0 = sentinel, 1 = boolean, 2 = integer
};

class CServerInfo {
public:
    CServerInfo();
    void SetDifficultyLevel(int level);
    void SetModuleName(const CExoString& name);
    int  FindOptionIndex(const CExoString& sCategory, const CExoString& sName);

    int   m_nUpdatedFlags;
    short m_nServerStatus;
    char  _pad06[2];
    CExoString m_sModuleList;
    CExoString m_sPlayerList;
    CExoString m_sSavegameList;
    CExoString m_sCharacterVaultList;

    int m_bAllowLocal;
    int m_bAllowServer;
    int m_bAllowVault;
    int m_bFixedParties;
    int m_bAllowFighter;
    int m_bAllowBarbarian;
    int m_bAllowRanger;
    int m_bAllowPaladin;
    int m_bAllowWizard;
    int m_bAllowSorcerer;
    int m_bAllowCleric;
    int m_bAllowDruid;
    int m_bAllowRogue;
    int m_bAllowBard;
    int m_bAllowMonk;
    int m_bAllowHuman;
    int m_bAllowElf;
    int m_bAllowDwarf;
    int m_bAllowGnome;
    int m_bAllowHalfling;
    int m_bAllowHalfOrc;
    int m_bAllowHalfElf;
    int m_nMaxPlayers;
    int m_nMaxParties;
    int m_nMaxItemPoints;
    int m_nMaxStatTotal;
    int m_nMinCharacterLevel;
    int m_nMaxCharacterLevel;

    unsigned int m_bWarnPlayersOnShutdown;
    int m_bAllKillable;
    int m_bNonPartyKillable;
    int m_bRequireResurrection;
    int m_bLoseStolenItems;
    int m_bLoseItems;
    int m_bLoseExp;
    int m_bLoseGold;
    int m_nLoseGoldNum;
    int m_nLoseExpNum;
    int m_nLoseItemsNum;
    int m_bPauseAndPlay;
#if !KOTOR1  // K1 has no such words here
    int m_bBlockPartyChanges;
    int m_nReserved1;
#endif
    unsigned int m_bItemLevelRestrictions;        // CanUseItem gate
    int m_bResetOnVacant;
    int m_bResetOnVictory;
    int m_bResetOnFailure;
    int m_bResetOnDeath;
    int m_bResetOnPartyDeath;
    int m_bAutoSaveAtIntervals;
    int m_bAutoSaveAtSavePoints;
    int m_bAutoSaveOnPlayerChange;
    int m_nAutoSaveInterval;
    int    m_nReserved2;
    int    m_nReserved3;
    int    m_nReserved4;
    char   _pad124[4];
    CExoString m_sModuleName;          // GetModuleName returns a copy of this
    // SetModuleName sets the first of these three flags for the three tutorial modules.
    unsigned m_bTutorialModule : 1;
    unsigned m_bReserved1 : 1;
    unsigned m_bReserved2 : 1;
    int m_difficultyLevel;

    ServerOption m_Options[50];
};
#if KOTOR1  // K1 layout
K2_LAYOUT_ASSERT(__builtin_offsetof(CServerInfo, m_bItemLevelRestrictions) == 0xe8, "K1 CServerInfo layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CServerInfo, m_difficultyLevel) == 0x134, "K1 CServerInfo layout");
K2_LAYOUT_ASSERT(sizeof(CServerInfo) == 0x778, "K1 CServerInfo size");
#endif
