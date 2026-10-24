#pragma once
#include "k2layout.h"

// A small status-effect summary record, passed by value between server status updates and the GUI.
// The flags share one 16-bit unit, followed by two separate byte members and a run of ints.
//
// SaveToGFF is defined out of line on purpose: its callers reach it through the same shared symbol.

class CResGFF;
struct CResStruct;

class CStatusSummary {
public:
    unsigned short m_bDisplaySpending : 1;   // "DISPLAYSPENDING"
    unsigned short m_bSoundPending    : 1;   // "SOUNDPENDING"
    unsigned short m_bItemReceived    : 1;   // "ITEMRECEIVED"
    unsigned short m_bItemLost        : 1;   // "ITEMLOST"
    unsigned short m_bJournal         : 1;   // "JOURNAL"
    unsigned short m_bLevelUpSound    : 1;   // "LEVELUPSOUND"
    unsigned short m_bNewQuestSound   : 1;   // "NEWQUESTSOUND"
    unsigned short m_bCompleteSound   : 1;   // "COMPLETESOUND"
#if !KOTOR1  // K1 has no alignment sounds, and its credits-net flag is bit 8
    unsigned short m_bDarkAlignSound  : 1;   // "DARKALIGNSOUND"
    unsigned short m_bLightAlignSound : 1;   // "LIGHTALIGNSOUND"
#endif
    unsigned short m_bCreditsNet      : 1;   // "CREDITSNET"
    char m_nLightShift;                      // "LIGHTSHIFT", an alignment nudge
    char m_nDarkShift;                       // "DARKSHIFT"
    int  m_nCredits;                         // "CREDITS"
    int  m_nXP;                              // "XP"
    int  m_nStealthXP;                       // "STEALTHXP"
    int  m_nSuppressed;                      // "SUPPRESSED"
#if !KOTOR1  // K1 has no influence or max-force-point accumulators
    int  m_nInfluenceGained;                 // "INFGAIN", a per-party-member bitmask
    int  m_nInfluenceLost;                   // "INFLOST"
    int  m_nMaxFPGained;                     // "MAXFPGAIN"
    int  m_nMaxFPLost;                       // "MAXFPLOST"
#endif

    // The party table's save round-trip.
    void SaveToGFF(CResGFF* pGff, CResStruct& pStruct);
    void LoadFromGFF(CResGFF* pGff, CResStruct& pStruct);
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CStatusSummary) == 0x14, "CStatusSummary K1 size drift");
#else
K2_LAYOUT_ASSERT(sizeof(CStatusSummary) == 0x24,
              "CStatusSummary is a by-value payload: CSWGuiStatusSummary's +0xa8 union member and "
              "KOTOR_AUTOSAVE_PARAMS' +0x98 field both depend on the 0x24 size");
#endif
