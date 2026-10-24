#include "statussummary.h"
#include "resgff.h"

// The field order below is the save format's, not the declaration order.
void CStatusSummary::SaveToGFF(CResGFF* pGff, CResStruct& pStruct)
{
    pGff->WriteFieldBYTE(&pStruct, m_bDisplaySpending, const_cast<char*>("DISPLAYSPENDING"));
    pGff->WriteFieldBYTE(&pStruct, m_bItemReceived, const_cast<char*>("ITEMRECEIVED"));
    pGff->WriteFieldBYTE(&pStruct, m_bItemLost, const_cast<char*>("ITEMLOST"));
    pGff->WriteFieldBYTE(&pStruct, m_bJournal, const_cast<char*>("JOURNAL"));
    pGff->WriteFieldBYTE(&pStruct, m_nLightShift, const_cast<char*>("LIGHTSHIFT"));
    pGff->WriteFieldBYTE(&pStruct, m_nDarkShift, const_cast<char*>("DARKSHIFT"));
    pGff->WriteFieldINT(&pStruct, m_nCredits, const_cast<char*>("CREDITS"));
    pGff->WriteFieldBYTE(&pStruct, m_bCreditsNet, const_cast<char*>("CREDITSNET"));
    pGff->WriteFieldINT(&pStruct, m_nXP, const_cast<char*>("XP"));
    pGff->WriteFieldINT(&pStruct, m_nStealthXP, const_cast<char*>("STEALTHXP"));
    pGff->WriteFieldBYTE(&pStruct, m_bSoundPending, const_cast<char*>("SOUNDPENDING"));
    pGff->WriteFieldBYTE(&pStruct, m_bLevelUpSound, const_cast<char*>("LEVELUPSOUND"));
    pGff->WriteFieldBYTE(&pStruct, m_bNewQuestSound, const_cast<char*>("NEWQUESTSOUND"));
    pGff->WriteFieldBYTE(&pStruct, m_bCompleteSound, const_cast<char*>("COMPLETESOUND"));
#if !KOTOR1  // K1 has no alignment-sound, influence or max-FP fields
    pGff->WriteFieldBYTE(&pStruct, m_bDarkAlignSound, const_cast<char*>("DARKALIGNSOUND"));
    pGff->WriteFieldBYTE(&pStruct, m_bLightAlignSound, const_cast<char*>("LIGHTALIGNSOUND"));
#endif
    pGff->WriteFieldINT(&pStruct, m_nSuppressed, const_cast<char*>("SUPPRESSED"));
#if !KOTOR1
    pGff->WriteFieldINT(&pStruct, m_nInfluenceGained, const_cast<char*>("INFGAIN"));
    pGff->WriteFieldINT(&pStruct, m_nInfluenceLost, const_cast<char*>("INFLOST"));
    pGff->WriteFieldINT(&pStruct, m_nMaxFPGained, const_cast<char*>("MAXFPGAIN"));
    pGff->WriteFieldINT(&pStruct, m_nMaxFPLost, const_cast<char*>("MAXFPLOST"));
#endif
}

// Reads the same 21 fields in the same order. The four accumulators default to 0 rather than their
// current value, so they are cleared on every load; the suppressed count keeps its value.
void CStatusSummary::LoadFromGFF(CResGFF* pGff, CResStruct& pStruct)
{
    int bSuccess;

    m_bDisplaySpending = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("DISPLAYSPENDING"), bSuccess, m_bDisplaySpending);
    m_bItemReceived    = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("ITEMRECEIVED"), bSuccess, m_bItemReceived);
    m_bItemLost        = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("ITEMLOST"), bSuccess, m_bItemLost);
    m_bJournal         = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("JOURNAL"), bSuccess, m_bJournal);
    m_nLightShift      = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("LIGHTSHIFT"), bSuccess, m_nLightShift);
    m_nDarkShift       = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("DARKSHIFT"), bSuccess, m_nDarkShift);
    m_nCredits         = pGff->ReadFieldINT(&pStruct, const_cast<char*>("CREDITS"), bSuccess, m_nCredits);
    m_bCreditsNet      = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("CREDITSNET"), bSuccess, m_bCreditsNet);
    m_nXP              = pGff->ReadFieldINT(&pStruct, const_cast<char*>("XP"), bSuccess, m_nXP);
    m_nStealthXP       = pGff->ReadFieldINT(&pStruct, const_cast<char*>("STEALTHXP"), bSuccess, m_nStealthXP);
    m_bSoundPending    = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("SOUNDPENDING"), bSuccess, m_bSoundPending);
    m_bLevelUpSound    = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("LEVELUPSOUND"), bSuccess, m_bLevelUpSound);
    m_bNewQuestSound   = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("NEWQUESTSOUND"), bSuccess, m_bNewQuestSound);
    m_bCompleteSound   = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("COMPLETESOUND"), bSuccess, m_bCompleteSound);
#if !KOTOR1  // K1 has no alignment-sound, influence or max-FP fields
    m_bDarkAlignSound  = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("DARKALIGNSOUND"), bSuccess, m_bDarkAlignSound);
    m_bLightAlignSound = pGff->ReadFieldBYTE(&pStruct, const_cast<char*>("LIGHTALIGNSOUND"), bSuccess, m_bLightAlignSound);
#endif
    m_nSuppressed      = pGff->ReadFieldINT(&pStruct, const_cast<char*>("SUPPRESSED"), bSuccess, m_nSuppressed);
#if !KOTOR1
    m_nInfluenceGained = pGff->ReadFieldINT(&pStruct, const_cast<char*>("INFGAIN"), bSuccess, 0);
    m_nInfluenceLost   = pGff->ReadFieldINT(&pStruct, const_cast<char*>("INFLOST"), bSuccess, 0);
    m_nMaxFPGained     = pGff->ReadFieldINT(&pStruct, const_cast<char*>("MAXFPGAIN"), bSuccess, 0);
    m_nMaxFPLost       = pGff->ReadFieldINT(&pStruct, const_cast<char*>("MAXFPLOST"), bSuccess, 0);
#endif
}
