#include "k2stdint.h"
#include "swpazaak.h"
#include "twodimarray.h"
#include "exostring.h"
#include "exoresman.h"
#include "gameobjectarray.h"

#include "port/crtapi.h"

#if KOTOR1 && defined(K2_HOST_BUILD)
// KOTOR 1's PC release dealt pazaak with the MSVC rand() (a 15-bit LCG); the phone port deals with
// the C library's rand(), a different generator on every platform. The host builds deal with the
// PC's, so a deck tracker that recovers the PC shuffle's seed works on them. The generator's state
// starts from the seeded C library rand(), so a new launch deals a new deck.
static int PazaakRand()
{
    static unsigned int s_nState;
    static bool s_bSeeded;
    if (!s_bSeeded) {
        s_nState = static_cast<unsigned int>(rand());
        s_bSeeded = true;
    }
    s_nState = s_nState * 214013u + 2531011u;
    return static_cast<int>((s_nState >> 16) & 0x7fff);
}
#else
#define PazaakRand rand_wincompatible
#endif

#if KOTOR1  // K1's table stops at the 10 card values (28 entries)
int CPazaakCard::s_pnPazaakCardValues[28] = {
    1, 2, 3, 4, 5, 6,
    -1, -2, -3, -4, -5, -6,
    1, 2, 3, 4, 5, 6,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
};
#else
int CPazaakCard::s_pnPazaakCardValues[33] = {
    1, 2, 3, 4, 5, 6,
    -1, -2, -3, -4, -5, -6,
    1, 2, 3, 4, 5, 6,
    1, 1,
    0, 0,
    1,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
};
#endif

CPazaakPlayer::CPazaakPlayer()
{
    CPazaakCard emptyCard;
    emptyCard.m_nCardType = PAZAAK_CARD_NONE;
    emptyCard.m_bIsNegative = 0;
#if !KOTOR1
    emptyCard.m_nValue = 0;
#endif
    m_bStanding = 0;
    m_nRoundsWon = 0;
    for (int i = 0; i < 4; i++)
        m_hand[i] = emptyCard;
    for (int i = 0; i < 9; i++)
        m_cardsPlayed[i] = emptyCard;
}

int CPazaakPlayer::GetTotal()
{
    int total = 0;
    for (int i = 0; i < 9; i++) {
        if (m_cardsPlayed[i].m_nCardType == PAZAAK_CARD_NONE)
            break;
        total += m_cardsPlayed[i].GetCardValue();
    }
    return total;
}

void CPazaakPlayer::ChooseSidedeck(CPazaakCard* sidedeck)
{
    for (int i = 0; i < 4; i++) {
        int idx = PazaakRand() % (10 - i);
        m_hand[i] = sidedeck[idx];
        sidedeck[idx] = sidedeck[9 - i];
#if KOTOR1
        CPazaakCard emptyCard;
        emptyCard.m_nCardType = PAZAAK_CARD_NONE;
        sidedeck[9 - i] = emptyCard;
#else
        sidedeck[9 - i].m_nCardType = PAZAAK_CARD_NONE;
        sidedeck[9 - i].m_bIsNegative = 0;
        sidedeck[9 - i].m_nValue = 0;
#endif
    }
}

void CPazaakPlayer::AddGameCard(CPazaakCard card)
{
#if KOTOR1  // K1 has no flip or copy-previous cards; the card goes in as dealt
    for (int i = 0; i < 9; i++) {
        if (m_cardsPlayed[i].m_nCardType == PAZAAK_CARD_NONE) {
            m_cardsPlayed[i] = card;
            if (i > 7)
                m_bStanding = 1;
            else if (GetTotal() == 20)
                m_bStanding = 1;
            return;
        }
    }
    m_bStanding = 1;
#else
    if (card.m_nCardType == PAZAAK_CARD_FLIP_2_4 || card.m_nCardType == PAZAAK_CARD_FLIP_3_6)
        card.m_nValue = 1;
    CPazaakCard prevCard;
    prevCard.m_bIsNegative = 0;
    prevCard.m_nValue = 0;
    int i;
    for (i = 0; i < 9; i++) {
        if (m_cardsPlayed[i].m_nCardType == PAZAAK_CARD_NONE)
            break;
        prevCard = m_cardsPlayed[i];
    }
    if (i == 9) {
        m_bStanding = 1;
        return;
    }
    if (card.m_nCardType == PAZAAK_CARD_DOUBLE)
        card.m_nValue = prevCard.GetCardValue();
    m_cardsPlayed[i] = card;
    if (i > 7)
        m_bStanding = 1;
    else if (GetTotal() == 20)
        m_bStanding = 1;
#endif
}

CSWPazaak::CSWPazaak()
{
#if !KOTOR1  // K1: the CPazaakCard ctor zeroes each deck card's sign
    for (int i = 0; i < 39; i++) {
        m_deck[i].m_bIsNegative = 0;
        m_deck[i].m_nValue = 0;
    }
    m_deck[39].m_bIsNegative = 0;
    m_deck[39].m_nValue = 0;
#endif
    m_nWager = 0;
    m_oidOpponent = OBJECT_INVALID;
    m_nDeckIndex = -1;
    m_nAIState = EAISTATE_START_TURN;
}

void CSWPazaak::ClearGameBoard()
{
#if KOTOR1
    CPazaakCard emptyCard;
    emptyCard.m_nCardType = PAZAAK_CARD_NONE;
    for (int i = 0; i < 9; i++) {
        m_players[0].m_cardsPlayed[i] = emptyCard;
        m_players[1].m_cardsPlayed[i] = emptyCard;
    }
#else
    for (int i = 0; i < 9; i++) {
        m_players[0].m_cardsPlayed[i].m_nCardType = PAZAAK_CARD_NONE;
        m_players[0].m_cardsPlayed[i].m_bIsNegative = 0;
        m_players[0].m_cardsPlayed[i].m_nValue = 0;
        m_players[1].m_cardsPlayed[i].m_nCardType = PAZAAK_CARD_NONE;
        m_players[1].m_cardsPlayed[i].m_bIsNegative = 0;
        m_players[1].m_cardsPlayed[i].m_nValue = 0;
    }
#endif
    m_players[0].m_bStanding = 0;
    m_players[1].m_bStanding = 0;
}

// Selection-sampling Fisher-Yates deck shuffle
void CSWPazaak::ShuffleDeck()
{
    CPazaakCard localDeck[40];
#if KOTOR1  // K1's main-deck card types are 18..27
    for (int i = 0; i < 40; i++) {
        CPazaakCard card;
        card.m_nCardType = PAZAAK_CARD_MAIN_1 + i % 10;
        localDeck[i] = card;
    }
#else
    for (int i = 0; i < 40; i++) {
        localDeck[i].m_bIsNegative = 0;
        localDeck[i].m_nValue = 0;
    }
    for (int i = 0; i < 40; i++) {
        localDeck[i].m_nCardType = PAZAAK_CARD_MAIN_1 + i % 10;
        localDeck[i].m_bIsNegative = 0;
        localDeck[i].m_nValue = 0;
    }
#endif
    CPazaakCard* pOut = m_deck;
    CPazaakCard* pTail = &localDeck[39];
    int64_t remaining = 40;
    do {
        int idx = PazaakRand() % static_cast<int>(remaining);
        *pOut = localDeck[idx];
        pOut++;
        localDeck[idx] = *pTail;
#if KOTOR1
        CPazaakCard emptyCard;
        emptyCard.m_nCardType = PAZAAK_CARD_NONE;
        *pTail = emptyCard;
#else
        pTail->m_nCardType = PAZAAK_CARD_NONE;
        pTail->m_bIsNegative = 0;
        pTail->m_nValue = 0;
#endif
        pTail--;
        remaining--;
    } while (remaining != 0);
    m_nDeckIndex = 39;
}

CPazaakCard CSWPazaak::DrawCard()
{
    if (m_nDeckIndex < 0)
        ShuffleDeck();
    K2_ASSUME(m_nDeckIndex >= 0 && m_nDeckIndex < 40);   // a shuffle restarts it at 39; only draws lower it
    return m_deck[m_nDeckIndex--];
}

int CSWPazaak::CanUsePlayerSidedeckCard(int index)
{
    return m_players[0].m_hand[index].m_nCardType >= 0;
}

bool CSWPazaak::UsePlayerSidedeckCard(int index)
{
    if (m_players[0].m_hand[index].m_nCardType < 0)
        return false;
#if !KOTOR1  // K1 has no flip cards
    if (m_players[0].m_hand[index].m_nCardType == PAZAAK_CARD_FLIP_2_4)
        m_players[0].FlipPositiveNegative(2, 4);
    else if (m_players[0].m_hand[index].m_nCardType == PAZAAK_CARD_FLIP_3_6)
        m_players[0].FlipPositiveNegative(3, 6);
#endif
    m_players[0].AddGameCard(m_players[0].m_hand[index]);
#if KOTOR1
    CPazaakCard emptyCard;
    emptyCard.m_nCardType = PAZAAK_CARD_NONE;
    m_players[0].m_hand[index] = emptyCard;
#else
    m_players[0].m_hand[index].m_nCardType = PAZAAK_CARD_NONE;
    m_players[0].m_hand[index].m_bIsNegative = 0;
    m_players[0].m_hand[index].m_nValue = 0;
#endif
    return true;
}

#if !KOTOR1  // K2-only: K1 has no flip cards
void CPazaakPlayer::FlipPositiveNegative(int val1, int val2)
{
    for (int i = 0; i < 9; i++) {
        CPazaakCard* pCard = &m_cardsPlayed[i];
        if (pCard->m_nCardType == PAZAAK_CARD_NONE)
            break;
        if (pCard->GetCardValue() == val1 || pCard->GetCardValue() == val2) {
            if (static_cast<unsigned>(pCard->m_nCardType - PAZAAK_CARD_PLUS_MINUS_1) <= 6)
                pCard->m_bIsNegative = (pCard->m_bIsNegative == 0);
            else
                pCard->m_nValue = -pCard->GetCardValue();
        }
    }
}

#endif

int CSWPazaak::DetermineWinner()
{
    int total1 = m_players[0].GetTotal();
    int total2 = m_players[1].GetTotal();
    if (total1 > 20 || total2 >= 21) {
        m_players[0].m_bStanding = 1;
        m_players[1].m_bStanding = 1;
    }
    if (m_players[0].m_bStanding == 0)
        return WINNER_NONE;
    if (m_players[1].m_bStanding == 0)
        return WINNER_NONE;
    if (total1 > 20)
        total1 = -1000000;
    if (total2 > 20)
        total2 = -1000000;
#if KOTOR1  // K1 has no nine-card win and no tie-breaker card; a tie is a draw
    if (total1 > total2)
        return WINNER_PLAYER;
    if (total1 < total2)
        return WINNER_OPPONENT;
    return WINNER_TIE;
#else
    int num1 = m_players[0].GetNumCardsPlayed();
    int num2 = m_players[1].GetNumCardsPlayed();
    if (num1 == 9 && num2 != 9 && total1 != -1000000)
        return WINNER_PLAYER;
    else if (num1 != 9 && num2 == 9 && total2 != -1000000)
        return WINNER_OPPONENT;
    if (total1 > total2)
        return WINNER_PLAYER;
    if (total1 < total2)
        return WINNER_OPPONENT;
    int tie1 = m_players[0].UsedTieBreaker();
    int tie2 = m_players[1].UsedTieBreaker();
    if (tie1 && !tie2)
        return WINNER_PLAYER;
    if (!tie1 && tie2)
        return WINNER_OPPONENT;
    return WINNER_TIE;
#endif
}

#if !KOTOR1  // K2-only
int CPazaakPlayer::GetNumCardsPlayed()
{
    int i = 0;
    while (i < 9 && m_cardsPlayed[i].m_nCardType != PAZAAK_CARD_NONE)
        i++;
    return i;
}

bool CPazaakPlayer::UsedTieBreaker()
{
    for (int i = 0; i < 9; i++) {
        if (m_cardsPlayed[i].m_nCardType == PAZAAK_CARD_TIEBREAKER)
            return true;
    }
    return false;
}

#endif

void CSWPazaak::LoadInitialSideDeck(int deckIndex, CPazaakCard* pSidedeck)
{
    C2DA twoDA(CResRef("PazaakDecks"), 0);
    if (!twoDA.Load2DArray())
        return;
    CExoString str;
    if (deckIndex < 0)
        deckIndex = PazaakRand() % twoDA.m_nRows;
#if KOTOR1  // K1 accepts only the '-' and '*' prefixes; no F/V/$/T cards
    CPazaakCard* pCard = pSidedeck;
    int type;
    int second;
    for (int i = 0; i < 10; i++, pCard++) {
        twoDA.GetCExoStringEntry(deckIndex, i, &str);
        second = str[1];
        type = second - '1';
        switch (str[0]) {
        case '-':
            type = second - ('1' - PAZAAK_CARD_MINUS_1);
            break;
        case '*':
            type = second - ('1' - PAZAAK_CARD_PLUS_MINUS_1);
            break;
        }
        CPazaakCard card;
        card.m_nCardType = type;
        *pCard = card;
    }
#else
    CPazaakCard* pCard = pSidedeck;
    int type;
    for (int i = 0; i < 10; i++, pCard++) {
        twoDA.GetCExoStringEntry(deckIndex, i, &str);
        int second = str[1];
        type = second - '1';
        switch (str[0]) {
        case 'F':
        case 'f':
            type = str[1] == '1' ? PAZAAK_CARD_FLIP_2_4 : PAZAAK_CARD_FLIP_3_6;
            break;
        case 'V':
        case 'v':
            type = PAZAAK_CARD_PLUS_MINUS_1_2;
            break;
        case '-':
            type = second - ('1' - PAZAAK_CARD_MINUS_1);
            break;
        case '$':
            type = PAZAAK_CARD_DOUBLE;
            break;
        case '*':
            type = second - ('1' - PAZAAK_CARD_PLUS_MINUS_1);
            break;
        case 'T':
        case 't':
            type = PAZAAK_CARD_TIEBREAKER;
            break;
        }
        pCard->m_nCardType = type;
        pCard->m_bIsNegative = 0;
        pCard->m_nValue = 0;
    }
#endif
    twoDA.Unload2DArray();
}

#if !KOTOR1  // K2-only: K1 has no double / flip / value-change side cards
int CSWPazaak::FindPotentialDoubleTotal(CPazaakPlayer& player, int nTarget)
{
    K2_UNUSED(nTarget);
    CPazaakCard lastCard;
    lastCard.m_bIsNegative = 0;
    lastCard.m_nValue = 0;
    for (int i = 0; i < 9; i++) {
        if (player.m_cardsPlayed[i].m_nCardType == PAZAAK_CARD_NONE)
            break;
        lastCard = player.m_cardsPlayed[i];
    }
    int total = 0;
    for (int i = 0; i < 9; i++) {
        if (player.m_cardsPlayed[i].m_nCardType == PAZAAK_CARD_NONE)
            break;
        total += player.m_cardsPlayed[i].GetCardValue();
    }
    return total + lastCard.GetCardValue();
}

int CSWPazaak::FindPotentialFlipTotal(CPazaakPlayer& player, int nTarget)
{
    int val1, val2;
    if (player.m_hand[nTarget].m_nCardType == PAZAAK_CARD_FLIP_2_4) {
        val2 = 4;
        val1 = 2;
    } else {
        val1 = (player.m_hand[nTarget].m_nCardType != PAZAAK_CARD_FLIP_3_6) ? 0 : 3;
        val2 = (player.m_hand[nTarget].m_nCardType != PAZAAK_CARD_FLIP_3_6) ? 0 : 6;
    }
    CPazaakCard temp[9];
    for (int i = 0; i < 9; i++) {
        temp[i].m_bIsNegative = 0;
        temp[i].m_nValue = 0;
    }
    for (int i = 0; i < 9; i++) {
        CPazaakCard card = player.m_cardsPlayed[i];
        if (card.GetCardValue() == val1 || card.GetCardValue() == val2)
            card.m_nValue = -card.GetCardValue();
        temp[i] = card;
    }
    int total = 0;
    for (int i = 0; i < 9; i++)
        total += temp[i].GetCardValue();
    return total;
}

int CSWPazaak::FindPotentialChangeTotal(CPazaakPlayer& player, int nTarget, int& rnBest, int nBest)
{
    CPazaakCard& card = player.m_hand[nTarget];
    int total = player.GetTotal();
    card.m_nValue = 2;
    int newTotal;
    if (nBest != 0) {
        card.m_bIsNegative = 1;
        rnBest = 1;
        newTotal = total + card.GetCardValue();
    } else {
        card.m_bIsNegative = 0;
        newTotal = total + card.GetCardValue();
        if (newTotal >= 21) {
            card.m_nValue = 1;
            newTotal = total + card.GetCardValue();
            if (newTotal >= 21) {
                card.m_bIsNegative = 1;
                rnBest = 1;
                newTotal = total + card.GetCardValue();
                if (newTotal >= 21) {
                    card.m_nValue = 2;
                    newTotal = total + card.GetCardValue();
                }
            }
        }
    }
    return newTotal;
}

#endif

int CSWPazaak::FindPotentialSidecardTotal(CPazaakPlayer& player, int* pOutTotal, int nTarget)
{
    int total = player.GetTotal();
    int bestTotal = (total > 20) ? -1000000 : total;
    int found = 0;
    int bestFlag = 0;
    int bestIdx = -1;
    for (int i = 0; i < 4; i++) {
        CPazaakCard& card = player.m_hand[i];
        for (int j = 2; j != 0; j--) {
            int flip = 0;
            int newTotal;
            int type = card.m_nCardType;
#if KOTOR1  // K1 tries only the +/- cards (12..17) both ways
            if (type >= PAZAAK_CARD_PLUS_MINUS_1 && type < PAZAAK_CARD_MAIN_1) {
                flip = (j == 2);
                card.m_bIsNegative = flip;
            } else {
                j = 1;
            }
            newTotal = total + card.GetCardValue();
#else
            if (static_cast<unsigned>(type - PAZAAK_CARD_FLIP_2_4) < 2) {
                newTotal = FindPotentialFlipTotal(player, i);
                j = 1;
            } else if (type == PAZAAK_CARD_PLUS_MINUS_1_2) {
                newTotal = FindPotentialChangeTotal(player, i, flip, nTarget);
                bestFlag = flip;   // unconditional, even when this card is not taken
                j = 1;
            } else if (type == PAZAAK_CARD_DOUBLE) {
                newTotal = FindPotentialDoubleTotal(player, i);
                j = 1;
            } else if (type >= PAZAAK_CARD_PLUS_MINUS_1 && type < PAZAAK_CARD_DOUBLE) {
                flip = (j == 2);
                card.m_bIsNegative = flip;
                newTotal = total + card.GetCardValue();
            } else {
                newTotal = total + card.GetCardValue();
                j = 1;
            }
#endif
            if (newTotal > 20)
                newTotal = -1000000;
            bool take = (newTotal > bestTotal && nTarget == 0);
            if (!take && nTarget != 0)
#if KOTOR1  // K1: a busted current total is never the bar to go under
                take = (newTotal < bestTotal && newTotal > -1000000);
#else
                take = ((newTotal < bestTotal || bestTotal == -1000000) && newTotal > -1000000);
#endif
            if (!take)
                take = (found && newTotal == bestTotal);
            if (take) {
                found = (static_cast<unsigned>(card.m_nCardType - PAZAAK_CARD_PLUS_MINUS_1) < 6);
                bestFlag = flip;
                bestIdx = i;
                bestTotal = newTotal;
            }
        }
    }
    if (bestIdx >= 0) {
        *pOutTotal = bestIdx;
#if KOTOR1  // K1: the sign is written back whatever the card type
        player.m_hand[bestIdx].m_bIsNegative = bestFlag;
#else
        int type = player.m_hand[bestIdx].m_nCardType;
        if (type == PAZAAK_CARD_TIEBREAKER || type == PAZAAK_CARD_PLUS_MINUS_1_2 ||
            (type >= PAZAAK_CARD_PLUS_MINUS_1 && type < PAZAAK_CARD_TIEBREAKER))
            player.m_hand[bestIdx].m_bIsNegative = bestFlag;
#endif
    }
    return bestTotal;
}

int CSWPazaak::PlayAITurn(CPazaakPlayer& aiPlayer, CPazaakPlayer& humanPlayer, EAIState state)
{
    int humanTotal = humanPlayer.GetTotal();
    if (humanTotal >= 21) {
        humanPlayer.m_bStanding = 1;
        humanTotal = -1000000;
    }
    switch (state) {
    case EAISTATE_START_TURN:
        if (aiPlayer.m_bStanding != 0)
            return EAISTATE_START_TURN;
        aiPlayer.AddGameCard(DrawCard());
        return EAISTATE_DREW_CARD;
    case EAISTATE_DREW_CARD: {
        int aiTotal = aiPlayer.GetTotal();
        int humanRoundsWon = humanPlayer.m_nRoundsWon;
        if (humanPlayer.m_bStanding != 0) {
#if KOTOR1  // K1 has no nine-card stand
            if (aiTotal > humanTotal && aiTotal < 21) {
#else
            if ((aiTotal > humanTotal && aiTotal < 21) || aiPlayer.GetNumCardsPlayed() >= 9) {
#endif
                aiPlayer.m_bStanding = 1;
                return EAISTATE_STOOD;
            }
        }
        int idx1;
        int sideTotal1 = FindPotentialSidecardTotal(aiPlayer, &idx1, 1);
        int idx0 = -1;
        int sideTotal0 = FindPotentialSidecardTotal(aiPlayer, &idx0, 0);
        bool useCard = false;
        if (sideTotal0 >= 18 && sideTotal0 >= humanTotal &&
            (sideTotal0 > 19 || sideTotal1 > 13 || (sideTotal0 == 19 && sideTotal1 > 11)) &&
            idx0 >= 0)
            useCard = true;
        if (aiTotal >= 21) {
            if (humanTotal >= 18 && humanRoundsWon <= 1 && sideTotal0 < humanTotal)
                return EAISTATE_ENDED_TURN;
            if (idx0 >= 0)
                useCard = true;
        }
        if (useCard) {
            CPazaakCard& card = aiPlayer.m_hand[idx0];
#if !KOTOR1
            if (card.m_nCardType == PAZAAK_CARD_FLIP_3_6)
                aiPlayer.FlipPositiveNegative(3, 6);
            else if (card.m_nCardType == PAZAAK_CARD_FLIP_2_4)
                aiPlayer.FlipPositiveNegative(2, 4);
#endif
            aiPlayer.AddGameCard(card);
#if KOTOR1
            CPazaakCard emptyCard;
            emptyCard.m_nCardType = PAZAAK_CARD_NONE;
            card = emptyCard;
#else
            card.m_nCardType = PAZAAK_CARD_NONE;
            card.m_bIsNegative = 0;
            card.m_nValue = 0;
#endif
            return EAISTATE_PLAYED_SIDE_CARD;
        }
    }
    case EAISTATE_PLAYED_SIDE_CARD: {
        int total3 = aiPlayer.GetTotal();
        if (total3 > 20)
            total3 = -1000000;
        if (humanPlayer.m_bStanding != 0 && total3 < humanTotal)
            return EAISTATE_ENDED_TURN;
        if (total3 >= 18) {
            aiPlayer.m_bStanding = 1;
            return EAISTATE_STOOD;
        }
        int idx2 = -1;
        int sideTotal2 = FindPotentialSidecardTotal(aiPlayer, &idx2, 1);
        if (sideTotal2 > 15) {
            aiPlayer.m_bStanding = 1;
            return EAISTATE_STOOD;
        }
        return EAISTATE_ENDED_TURN;
    }
    // EAISTATE_STOOD and EAISTATE_ENDED_TURN fall straight through to EAISTATE_START_TURN; only an
    // out-of-range state returns EAISTATE_ENDED_TURN, which is why the switch keeps its default.
    case EAISTATE_STOOD:
    case EAISTATE_ENDED_TURN:
        break;
    default:
        return EAISTATE_ENDED_TURN;
    }
    return EAISTATE_START_TURN;
}
