#pragma once

// The pazaak minigame's cards, players and game state. All three types live in one compile unit, so
// they are declared together here and defined in CSWPazaak.cpp.

// CPazaakCard::m_nCardType. The side cards are PazaakDecks.2da's codes: "+N", "-N" and "*N" (plus
// or minus), and KOTOR II's "T", "$", "F1", "F2" and "V"; the main deck's 1..10 follow them.
enum PazaakCardType {
    PAZAAK_CARD_FACE_DOWN = -2,     // the opponent's hidden hand cards
    PAZAAK_CARD_NONE = -1,
    PAZAAK_CARD_PLUS_1 = 0, PAZAAK_CARD_PLUS_2, PAZAAK_CARD_PLUS_3,
    PAZAAK_CARD_PLUS_4, PAZAAK_CARD_PLUS_5, PAZAAK_CARD_PLUS_6,
    PAZAAK_CARD_MINUS_1, PAZAAK_CARD_MINUS_2, PAZAAK_CARD_MINUS_3,
    PAZAAK_CARD_MINUS_4, PAZAAK_CARD_MINUS_5, PAZAAK_CARD_MINUS_6,
    PAZAAK_CARD_PLUS_MINUS_1, PAZAAK_CARD_PLUS_MINUS_2, PAZAAK_CARD_PLUS_MINUS_3,
    PAZAAK_CARD_PLUS_MINUS_4, PAZAAK_CARD_PLUS_MINUS_5, PAZAAK_CARD_PLUS_MINUS_6,
#if !KOTOR1
    PAZAAK_CARD_TIEBREAKER,         // "T": +/-1, and wins a tied set
    PAZAAK_CARD_DOUBLE,             // "$": the value of the last card dealt
    PAZAAK_CARD_FLIP_2_4,           // "F1"
    PAZAAK_CARD_FLIP_3_6,           // "F2"
    PAZAAK_CARD_PLUS_MINUS_1_2,     // "V": 1, 2, -1 or -2
#endif
    PAZAAK_CARD_MAIN_1, PAZAAK_CARD_MAIN_2, PAZAAK_CARD_MAIN_3, PAZAAK_CARD_MAIN_4,
    PAZAAK_CARD_MAIN_5, PAZAAK_CARD_MAIN_6, PAZAAK_CARD_MAIN_7, PAZAAK_CARD_MAIN_8,
    PAZAAK_CARD_MAIN_9, PAZAAK_CARD_MAIN_10,
};

// One pazaak card, with the card type, its sign flag and, on K2, an override value.
class CPazaakCard {
public:
    // A PazaakCardType: PAZAAK_CARD_NONE for an empty slot, else an index into s_pnPazaakCardValues.
    int m_nCardType;
    // Sign flag for the flippable cards.
    int m_bIsNegative;
#if KOTOR1  // K1 cards are 8 bytes, without value-override cards
    // K1 default-constructs every card with a clear sign.
    CPazaakCard() : m_bIsNegative(0) {}
#else
    int m_nValue;
#endif

    int GetCardValue();

#if KOTOR1  // K1 has 28 types: +1..+6, -1..-6, +/-1..6 and the main deck 1..10
    static int s_pnPazaakCardValues[28];
#else
    static int s_pnPazaakCardValues[33];
#endif
};

inline int CPazaakCard::GetCardValue()
{
    if (m_nCardType < 0)
        return 0;
#if KOTOR1  // K1: only the flippable cards carry a sign
    if (static_cast<unsigned>(m_nCardType - PAZAAK_CARD_PLUS_MINUS_1) <= 5 && m_bIsNegative != 0)
        return -s_pnPazaakCardValues[m_nCardType];
    return s_pnPazaakCardValues[m_nCardType];
#else
    if (static_cast<unsigned>(m_nCardType - PAZAAK_CARD_PLUS_MINUS_1) <= 6 && m_bIsNegative != 0)
        return -s_pnPazaakCardValues[m_nCardType];
    if (m_nCardType == PAZAAK_CARD_FLIP_2_4 || m_nCardType == PAZAAK_CARD_FLIP_3_6)
        return s_pnPazaakCardValues[m_nCardType];
    if (m_nCardType == PAZAAK_CARD_PLUS_MINUS_1_2) {
        if (m_nValue == 0)
            m_nValue = 1;
        return m_bIsNegative != 0 ? -m_nValue : m_nValue;
    }
    if (m_nCardType != PAZAAK_CARD_DOUBLE && m_nValue == 0)
        return s_pnPazaakCardValues[m_nCardType];
    return m_nValue;
#endif
}

// One pazaak player, with the 4-card side-deck hand plus the cards played out.
class CPazaakPlayer {
public:
    CPazaakCard m_hand[4];
    CPazaakCard m_cardsPlayed[9];
    // Set on an auto-stand (total 20, or 9 cards played).
    int m_bStanding;
    int m_nRoundsWon;

    CPazaakPlayer();

    int GetTotal();
    void ChooseSidedeck(CPazaakCard* sidedeck);
    void AddGameCard(CPazaakCard card);
#if !KOTOR1  // K1 has no flip cards, no tie-breaker card and no nine-card rule
    void FlipPositiveNegative(int val1, int val2);
    int GetNumCardsPlayed();
#endif
#if !KOTOR1
    bool UsedTieBreaker();
#endif
};

// The pazaak game state.
class CSWPazaak {
public:
    // The opponent's turn: the step PlayAITurn runs next, and what it returns (the step it did).
    enum EAIState {
        EAISTATE_START_TURN = 0,    // draw, or end the turn at once when standing
        EAISTATE_DREW_CARD,
        EAISTATE_PLAYED_SIDE_CARD,
        EAISTATE_STOOD,
        EAISTATE_ENDED_TURN,
    };

    // DetermineWinner's result.
    enum Winner {
        WINNER_NONE = 0,            // the set is still being played
        WINNER_PLAYER,
        WINNER_OPPONENT,
        WINNER_TIE,
    };

    int m_nWager;
    unsigned long long m_oidOpponent;
    CPazaakPlayer m_players[2];
    CPazaakCard m_deck[40];
    int m_nDeckIndex;
    int m_nAIState;

    CSWPazaak();

    void ClearGameBoard();
    __attribute__((noinline)) void ShuffleDeck();
    CPazaakCard DrawCard();
    int  CanUsePlayerSidedeckCard(int index);
    bool UsePlayerSidedeckCard(int index);
    int DetermineWinner();
    int PlayAITurn(CPazaakPlayer& aiPlayer, CPazaakPlayer& humanPlayer, EAIState state);
#if KOTOR1  // K1 inlines this into PlayAITurn
    __attribute__((always_inline)) int FindPotentialSidecardTotal(CPazaakPlayer& player, int* pOutTotal, int nTarget);
#else
    int FindPotentialSidecardTotal(CPazaakPlayer& player, int* pOutTotal, int nTarget);
#endif
#if !KOTOR1  // K1 has no double, flip or value-change side cards
    int FindPotentialDoubleTotal(CPazaakPlayer& player, int nTarget);
    int FindPotentialFlipTotal(CPazaakPlayer& player, int nTarget);
    int FindPotentialChangeTotal(CPazaakPlayer& player, int nTarget, int& rnBest, int nBest);
#endif
    void LoadInitialSideDeck(int nDeckIndex, CPazaakCard* pSidedeck);

    // Deals the player the next card off the main deck.
    void DrawPlayerCard() { m_players[0].AddGameCard(DrawCard()); }
    // Runs the opponent's next turn step and records it as the step to run after; returns that step.
    int AIPlayNPCTurn()
    {
        m_nAIState = PlayAITurn(m_players[1], m_players[0], static_cast<EAIState>(m_nAIState));
        return m_nAIState;
    }
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CPazaakCard) == 8, "K1 CPazaakCard size");
K2_LAYOUT_ASSERT(sizeof(CPazaakPlayer) == 0x70, "K1 CPazaakPlayer size");
K2_LAYOUT_ASSERT(sizeof(CSWPazaak) == 0x238, "K1 CSWPazaak size");
#endif
