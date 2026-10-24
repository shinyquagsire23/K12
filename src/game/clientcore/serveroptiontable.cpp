#include "serverinfo.h"

#include "port/libcstring.h"

CServerInfo::CServerInfo()
  : m_nReserved2(0)
  , m_nReserved3(1)
  , m_nReserved4(0)
{
    m_nUpdatedFlags = 0;
    m_nServerStatus = 0;
    m_nMinCharacterLevel = 1;
    m_nMaxCharacterLevel = 50;
    m_bPauseAndPlay = 1;
    m_bTutorialModule = 0;
    m_bReserved1 = 0;
    m_bReserved2 = 0;
    m_bWarnPlayersOnShutdown = 1;
    m_difficultyLevel = 2;

    m_Options[0].m_szCategory = "Undefined";
    m_Options[0].m_szName     = "UndefinedOption";
    m_Options[0].m_pValue     = 0;
    m_Options[0].m_nType      = 0;
    m_Options[1].m_szCategory = "JoiningRestrictions";
    m_Options[1].m_szName     = "AllowLocal";
    m_Options[1].m_pValue     = &m_bAllowLocal;
    m_Options[1].m_nType      = 1;
    m_Options[2].m_szCategory = "JoiningRestrictions";
    m_Options[2].m_szName     = "AllowServer";
    m_Options[2].m_pValue     = &m_bAllowServer;
    m_Options[2].m_nType      = 1;
    m_Options[3].m_szCategory = "JoiningRestrictions";
    m_Options[3].m_szName     = "AllowVault";
    m_Options[3].m_pValue     = &m_bAllowVault;
    m_Options[3].m_nType      = 1;
    m_Options[4].m_szCategory = "JoiningRestrictions";
    m_Options[4].m_szName     = "FixedParties";
    m_Options[4].m_pValue     = &m_bFixedParties;
    m_Options[4].m_nType      = 1;
    m_Options[5].m_szCategory = "JoiningRestrictions";
    m_Options[5].m_szName     = "AllowFighter";
    m_Options[5].m_pValue     = &m_bAllowFighter;
    m_Options[5].m_nType      = 1;
    m_Options[6].m_szCategory = "JoiningRestrictions";
    m_Options[6].m_szName     = "AllowBarbarian";
    m_Options[6].m_pValue     = &m_bAllowBarbarian;
    m_Options[6].m_nType      = 1;
    m_Options[7].m_szCategory = "JoiningRestrictions";
    m_Options[7].m_szName     = "AllowRanger";
    m_Options[7].m_pValue     = &m_bAllowRanger;
    m_Options[7].m_nType      = 1;
    m_Options[8].m_szCategory = "JoiningRestrictions";
    m_Options[8].m_szName     = "AllowPaladin";
    m_Options[8].m_pValue     = &m_bAllowPaladin;
    m_Options[8].m_nType      = 1;
    m_Options[9].m_szCategory = "JoiningRestrictions";
    m_Options[9].m_szName     = "AllowWizard";
    m_Options[9].m_pValue     = &m_bAllowWizard;
    m_Options[9].m_nType      = 1;
    m_Options[10].m_szCategory = "JoiningRestrictions";
    m_Options[10].m_szName     = "AllowSorcerer";
    m_Options[10].m_pValue     = &m_bAllowSorcerer;
    m_Options[10].m_nType      = 1;
    m_Options[11].m_szCategory = "JoiningRestrictions";
    m_Options[11].m_szName     = "AllowCleric";
    m_Options[11].m_pValue     = &m_bAllowCleric;
    m_Options[11].m_nType      = 1;
    m_Options[12].m_szCategory = "JoiningRestrictions";
    m_Options[12].m_szName     = "AllowDruid";
    m_Options[12].m_pValue     = &m_bAllowDruid;
    m_Options[12].m_nType      = 1;
    m_Options[13].m_szCategory = "JoiningRestrictions";
    m_Options[13].m_szName     = "AllowRogue";
    m_Options[13].m_pValue     = &m_bAllowRogue;
    m_Options[13].m_nType      = 1;
    m_Options[14].m_szCategory = "JoiningRestrictions";
    m_Options[14].m_szName     = "AllowBard";
    m_Options[14].m_pValue     = &m_bAllowBard;
    m_Options[14].m_nType      = 1;
    m_Options[15].m_szCategory = "JoiningRestrictions";
    m_Options[15].m_szName     = "AllowMonk";
    m_Options[15].m_pValue     = &m_bAllowMonk;
    m_Options[15].m_nType      = 1;
    m_Options[16].m_szCategory = "JoiningRestrictions";
    m_Options[16].m_szName     = "AllowHuman";
    m_Options[16].m_pValue     = &m_bAllowHuman;
    m_Options[16].m_nType      = 1;
    m_Options[17].m_szCategory = "JoiningRestrictions";
    m_Options[17].m_szName     = "AllowElf";
    m_Options[17].m_pValue     = &m_bAllowElf;
    m_Options[17].m_nType      = 1;
    m_Options[18].m_szCategory = "JoiningRestrictions";
    m_Options[18].m_szName     = "AllowDwarf";
    m_Options[18].m_pValue     = &m_bAllowDwarf;
    m_Options[18].m_nType      = 1;
    m_Options[19].m_szCategory = "JoiningRestrictions";
    m_Options[19].m_szName     = "AllowGnome";
    m_Options[19].m_pValue     = &m_bAllowGnome;
    m_Options[19].m_nType      = 1;
    m_Options[20].m_szCategory = "JoiningRestrictions";
    m_Options[20].m_szName     = "AllowHalfling";
    m_Options[20].m_pValue     = &m_bAllowHalfling;
    m_Options[20].m_nType      = 1;
    m_Options[21].m_szCategory = "JoiningRestrictions";
    m_Options[21].m_szName     = "AllowHalfOrc";
    m_Options[21].m_pValue     = &m_bAllowHalfOrc;
    m_Options[21].m_nType      = 1;
    m_Options[22].m_szCategory = "JoiningRestrictions";
    m_Options[22].m_szName     = "AllowHalfElf";
    m_Options[22].m_pValue     = &m_bAllowHalfElf;
    m_Options[22].m_nType      = 1;
    m_Options[23].m_szCategory = "JoiningRestrictions";
    m_Options[23].m_szName     = "MaxPlayers";
    m_Options[23].m_pValue     = &m_nMaxPlayers;
    m_Options[23].m_nType      = 2;
    m_Options[24].m_szCategory = "JoiningRestrictions";
    m_Options[24].m_szName     = "MaxParties";
    m_Options[24].m_pValue     = &m_nMaxParties;
    m_Options[24].m_nType      = 2;
    m_Options[25].m_szCategory = "JoiningRestrictions";
    m_Options[25].m_szName     = "MaxItemPoints";
    m_Options[25].m_pValue     = &m_nMaxItemPoints;
    m_Options[25].m_nType      = 2;
    m_Options[26].m_szCategory = "JoiningRestrictions";
    m_Options[26].m_szName     = "MaxStatTotal";
    m_Options[26].m_pValue     = &m_nMaxStatTotal;
    m_Options[26].m_nType      = 2;
    m_Options[27].m_szCategory = "JoiningRestrictions";
    m_Options[27].m_szName     = "MinLevel";
    m_Options[27].m_pValue     = &m_nMinCharacterLevel;
    m_Options[27].m_nType      = 2;
    m_Options[28].m_szCategory = "JoiningRestrictions";
    m_Options[28].m_szName     = "MaxLevel";
    m_Options[28].m_pValue     = &m_nMaxCharacterLevel;
    m_Options[28].m_nType      = 2;
    m_Options[29].m_szCategory = "PlayOptions";
    m_Options[29].m_szName     = "AllKillable";
    m_Options[29].m_pValue     = &m_bAllKillable;
    m_Options[29].m_nType      = 1;
    m_Options[30].m_szCategory = "PlayOptions";
    m_Options[30].m_szName     = "NonPartyKillable";
    m_Options[30].m_pValue     = &m_bNonPartyKillable;
    m_Options[30].m_nType      = 1;
    m_Options[31].m_szCategory = "PlayOptions";
    m_Options[31].m_szName     = "RequireResurrection";
    m_Options[31].m_pValue     = &m_bRequireResurrection;
    m_Options[31].m_nType      = 1;
    m_Options[32].m_szCategory = "PlayOptions";
    m_Options[32].m_szName     = "LoseStolenItems";
    m_Options[32].m_pValue     = &m_bLoseStolenItems;
    m_Options[32].m_nType      = 1;
    m_Options[33].m_szCategory = "PlayOptions";
    m_Options[33].m_szName     = "LoseItems";
    m_Options[33].m_pValue     = &m_bLoseItems;
    m_Options[33].m_nType      = 1;
    m_Options[34].m_szCategory = "PlayOptions";
    m_Options[34].m_szName     = "LoseExp";
    m_Options[34].m_pValue     = &m_bLoseExp;
    m_Options[34].m_nType      = 1;
    m_Options[35].m_szCategory = "PlayOptions";
    m_Options[35].m_szName     = "LoseGold";
    m_Options[35].m_pValue     = &m_bLoseGold;
    m_Options[35].m_nType      = 1;
    m_Options[36].m_szCategory = "PlayOptions";
    m_Options[36].m_szName     = "LoseGoldNum";
    m_Options[36].m_pValue     = &m_nLoseGoldNum;
    m_Options[36].m_nType      = 2;
    m_Options[37].m_szCategory = "PlayOptions";
    m_Options[37].m_szName     = "LoseExpNum";
    m_Options[37].m_pValue     = &m_nLoseExpNum;
    m_Options[37].m_nType      = 2;
    m_Options[38].m_szCategory = "PlayOptions";
    m_Options[38].m_szName     = "LoseItemsNum";
    m_Options[38].m_pValue     = &m_nLoseItemsNum;
    m_Options[38].m_nType      = 2;
    m_Options[39].m_szCategory = "ResetOption";
    m_Options[39].m_szName     = "OnVacant";
    m_Options[39].m_pValue     = &m_bResetOnVacant;
    m_Options[39].m_nType      = 1;
    m_Options[40].m_szCategory = "ResetOption";
    m_Options[40].m_szName     = "OnVictory";
    m_Options[40].m_pValue     = &m_bResetOnVictory;
    m_Options[40].m_nType      = 1;
    m_Options[41].m_szCategory = "ResetOption";
    m_Options[41].m_szName     = "OnFailure";
    m_Options[41].m_pValue     = &m_bResetOnFailure;
    m_Options[41].m_nType      = 1;
    m_Options[42].m_szCategory = "ResetOption";
    m_Options[42].m_szName     = "OnDeath";
    m_Options[42].m_pValue     = &m_bResetOnDeath;
    m_Options[42].m_nType      = 1;
    m_Options[43].m_szCategory = "ResetOption";
    m_Options[43].m_szName     = "OnPartyDeath";
    m_Options[43].m_pValue     = &m_bResetOnPartyDeath;
    m_Options[43].m_nType      = 1;
    m_Options[44].m_szCategory = "AutoSave";
    m_Options[44].m_szName     = "AtIntervals";
    m_Options[44].m_pValue     = &m_bAutoSaveAtIntervals;
    m_Options[44].m_nType      = 1;
    m_Options[45].m_szCategory = "AutoSave";
    m_Options[45].m_szName     = "AtSavePoints";
    m_Options[45].m_pValue     = &m_bAutoSaveAtSavePoints;
    m_Options[45].m_nType      = 1;
    m_Options[46].m_szCategory = "AutoSave";
    m_Options[46].m_szName     = "OnPlayerChange";
    m_Options[46].m_pValue     = &m_bAutoSaveOnPlayerChange;
    m_Options[46].m_nType      = 1;
    m_Options[47].m_szCategory = "AutoSave";
    m_Options[47].m_szName     = "Interval";
    m_Options[47].m_pValue     = &m_nAutoSaveInterval;
    m_Options[47].m_nType      = 2;
    m_Options[48].m_szCategory = "Undefined";
    m_Options[48].m_szName     = "UndefinedOption";
    m_Options[48].m_pValue     = 0;
    m_Options[48].m_nType      = 0;
    m_Options[49].m_szCategory = "PlayOptions";
    m_Options[49].m_szName     = "PauseAndPlay";
    m_Options[49].m_pValue     = &m_bPauseAndPlay;
    m_Options[49].m_nType      = 1;
}

int CServerInfo::FindOptionIndex(const CExoString& sCategory, const CExoString& sName)
{
    for (int i = 1; i < 50; i++)
    {
        if (strcmp(sCategory.CStr(), m_Options[i].m_szCategory) == 0 &&
            strcmp(sName.CStr(), m_Options[i].m_szName) == 0)
            return i;
    }
    return 0;
}

void CServerInfo::SetDifficultyLevel(int level)
{
    int clamped = level < 4 ? level : 4;
    if (clamped < 0)
        clamped = 0;
    m_difficultyLevel = clamped;
}

void CServerInfo::SetModuleName(const CExoString& name)
{
    m_sModuleName = name;
    bool bTutorial;
    if (m_sModuleName == "ebo_m12aa" || m_sModuleName == "tar_m02af")
        bTutorial = true;
    else
        bTutorial = (m_sModuleName == "ebo_m40ad") != 0;
    m_bTutorialModule = bTutorial;
}
