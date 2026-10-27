// The client-side talk table, which does dialog and UI token substitution.

#include "k2stdint.h"
#include "nwtlktable.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "nwsrules.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwrace.h"
#include "nwclass.h"
#include "partytables.h"
#include "globalvartables.h"
#include "nwcmodule.h"
#include "clientoptions.h"
#include "gameobjectarray.h"
#include "exoinput.h"
#include "exoinputinternal.h"
#include "worldtimer.h"
#if KOTOR1
#include "ClientExoAppInternal.h"
#endif
#include "exostring.h"

extern CTlkTable* g_pTlkTable;

#include "port/libcstring.h"
#include "strref.h"
#include "exostringinline.h"

// The good-evil bands of the good/evil token: below the first is the third string, above the
// second the first.
enum {
    ALIGNMENT_NEUTRAL_LOWEST  = 41,
    ALIGNMENT_NEUTRAL_HIGHEST = 59,
};

// Mouse-button strrefs in mouse-button order.
static const uint64_t arACStrRefs[4] = {
    STRREF_MOUSE_BUTTON_RIGHT, STRREF_MOUSE_BUTTON_LEFT,
    STRREF_MOUSE_BUTTON_RIGHT_LOWER, STRREF_MOUSE_BUTTON_LEFT_LOWER,
};

// Resolves one talk-table token. The token's language code says what its strrefs mean; the higher
// codes need the speaker and its stats.
void CSWTlkTable::GetTokenValue(const CTlkTableToken& pToken, CExoString& sValue)
{
    CResRef    cResRef;
    CExoString sString;
    unsigned int nStartDay;
    unsigned int nEndDay;
    unsigned int nCurrDay;

    switch (pToken.m_nLanguage)
    {
    case TOKEN_ACTION_STRREF:
        sValue = g_pAppManager->m_pClientExoApp->GetGUIString(pToken.m_StrRefs[0]);
        break;
    case TOKEN_ACTION_GENDER_STRREF:
        sValue = g_pAppManager->m_pClientExoApp->GetGUIString(pToken.m_StrRefs[m_gender]);
        break;

    case TOKEN_ACTION_FULL_NAME:
    case TOKEN_ACTION_FIRST_NAME:
    case TOKEN_ACTION_LAST_NAME:
    case TOKEN_ACTION_RACE:
    case TOKEN_ACTION_RACE_LOWER:
    case TOKEN_ACTION_SUBRACE:
    case TOKEN_ACTION_CLASS:
    case TOKEN_ACTION_CLASS_LOWER:
    case TOKEN_ACTION_LEVEL:
    case TOKEN_ACTION_ALIGNMENT:
    case TOKEN_ACTION_ALIGNMENT_LOWER:
    case TOKEN_ACTION_GOOD_EVIL:
#if !KOTOR1
    case TOKEN_ACTION_PLAYER_NAME:
#endif
        if (m_pSpeakerObj == 0)
        {
            sValue = g_pAppManager->m_pClientExoApp->GetGUIString(static_cast<unsigned int>(pToken.m_nStringRef));
            break;
        }
        if (m_pSpeakerObj->AsSWCObject() == 0)
        {
            sValue = "NO OBJECT";
            break;
        }
        if (m_pSpeakerObj->AsSWCCreature() == 0)
        {
            sValue = g_pAppManager->m_pClientExoApp->GetGUIString(static_cast<unsigned int>(pToken.m_nStringRef));
            break;
        }
        if (m_pSpeakerObj->AsSWCCreature()->m_pStats == 0)
        {
            sValue = "NO STATS";
            break;
        }

        // The arms below read the speaker's stats, so the gender is captured and restored around
        // them.
        {
            unsigned char nGender = m_gender;

            switch (pToken.m_nLanguage)
            {
#if KOTOR1
            // K1 substitutes the speaker's own names.
            case TOKEN_ACTION_FULL_NAME:
                sValue = m_pSpeakerObj->AsSWCCreature()->m_pStats->GetFullName();
                break;
            case TOKEN_ACTION_FIRST_NAME:
                sValue = m_pSpeakerObj->AsSWCCreature()->m_pStats->m_name;
                break;
            case TOKEN_ACTION_LAST_NAME:
                sValue = m_pSpeakerObj->AsSWCCreature()->m_pStats->m_sLastName;
                break;
#else
            case TOKEN_ACTION_FULL_NAME:
            case TOKEN_ACTION_PLAYER_NAME:
                {
                    CExoString sStr =
                        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_saveTableName;
                    sValue = sStr;
                }
                break;
            case TOKEN_ACTION_FIRST_NAME:
                sValue = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPCFirstName();
                break;
            case TOKEN_ACTION_LAST_NAME:
                sValue = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPCLastName();
                break;
#endif
            case TOKEN_ACTION_RACE:
                sValue = g_pRules->m_paRaces[m_pSpeakerObj->AsSWCCreature()->m_pStats->m_nRace].GetConverNameText();
                break;
            case TOKEN_ACTION_RACE_LOWER:
                sValue = g_pRules->m_paRaces[m_pSpeakerObj->AsSWCCreature()->m_pStats->m_nRace].GetConverNameLowerText();
                break;
            case TOKEN_ACTION_SUBRACE:
                sValue = m_pSpeakerObj->AsSWCCreature()->m_pStats->GetRaceString();
                break;
            case TOKEN_ACTION_CLASS:
                sValue = g_pRules->m_paClasses[m_pSpeakerObj->AsSWCCreature()->m_pStats->GetClass(0)].GetNameText();
                break;
            case TOKEN_ACTION_CLASS_LOWER:
                sValue = g_pRules->m_paClasses[m_pSpeakerObj->AsSWCCreature()->m_pStats->GetClass(0)].GetNameLowerText();
                break;
            case TOKEN_ACTION_LEVEL:
                sValue = CExoString(static_cast<int>(m_pSpeakerObj->AsSWCCreature()->m_pStats->GetClassLevel(0)));
                break;
            case TOKEN_ACTION_ALIGNMENT:
                sValue = m_pSpeakerObj->AsSWCCreature()->m_pStats->GetAlignmentString(0);
                break;
            case TOKEN_ACTION_ALIGNMENT_LOWER:
                sValue = m_pSpeakerObj->AsSWCCreature()->m_pStats->GetAlignmentString(1);
                break;
            case TOKEN_ACTION_GOOD_EVIL:
                // Picks one of three alignment strings, 0 above the band, 1 inside it, 2 below.
                {
                    short nAlign = m_pSpeakerObj->AsSWCCreature()->m_pStats->m_alignmentGoodEvil;
                    unsigned int nIdx = 1;
                    if (nAlign < ALIGNMENT_NEUTRAL_LOWEST)
                        nIdx = 2;
                    else if (nAlign > ALIGNMENT_NEUTRAL_HIGHEST)
                        nIdx = 0;
                    sValue = g_pAppManager->m_pClientExoApp->GetGUIString(pToken.m_StrRefs[nIdx]);
                }
                break;
            default:
                break;
            }

#if KOTOR1
            // K1 restores the captured gender without clamping.
            m_gender = nGender;
#else
            m_gender = nGender;
            if (nGender > CTlkTable::GENDER_FEMALE)
                m_gender = CTlkTable::GENDER_MALE;
#endif
        }
        break;

    case TOKEN_ACTION_GAME_YEAR:
        sValue = CExoString(g_pAppManager->m_pClientExoApp->GetModule()->m_nCalendarYear);
        break;
    case TOKEN_ACTION_GAME_TIME:
        sValue = CExoString(
            static_cast<int>((static_cast<unsigned int>(g_pAppManager->m_pClientExoApp->GetModule()->m_nCalendarHour) +
                   (static_cast<unsigned int>(CWorldTimer::HOURS_IN_DAY) / 2) - 1U) %
                  (static_cast<unsigned int>(CWorldTimer::HOURS_IN_DAY) / 2)) +
            1);
        break;
    case TOKEN_ACTION_GAME_MONTH:
        sValue = CExoString(g_pAppManager->m_pClientExoApp->GetModule()->m_nCalendarMonth);
        break;
    case TOKEN_ACTION_DAY_NIGHT:
        // One of four day bands over the module's day window.
        {
            nStartDay = g_pAppManager->m_pClientExoApp->GetModule()->m_nDawnHour;
            nEndDay = g_pAppManager->m_pClientExoApp->GetModule()->m_nDuskHour;
            nCurrDay = g_pAppManager->m_pClientExoApp->GetModule()->m_nCalendarHour;
            unsigned char nIdx = 0;
            if (__builtin_expect(nCurrDay >= nStartDay && nCurrDay <= nEndDay, 0))
                nIdx = static_cast<unsigned char>(((nCurrDay - nStartDay) * 3) / (nEndDay - nStartDay + 1) + 1);
            sValue = g_pAppManager->m_pClientExoApp->GetGUIString(pToken.m_StrRefs[nIdx]);
        }
        break;

    case TOKEN_ACTION_START_ACTION:
        m_bSuppressText = (m_bSuppressEnabled & SUPPRESS_ENABLED);
        sValue = "";
        break;
    case TOKEN_ACTION_START_CHECK:
        m_bSuppressText = (m_bSuppressEnabled & SUPPRESS_ENABLED);
        sValue = "";
        break;
    case TOKEN_ACTION_START_HIGHLIGHT:
        sValue = "";
        break;
    case TOKEN_ACTION_END_HIGHLIGHT:
        sValue = "";
        m_bSuppressText = 0;
        break;
#if KOTOR1
    case TOKEN_ACTION_PLAYER_NAME:
        // K1: the named player's name, otherwise the token's own strref.
        {
            CSWCPlayer* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerByPlayerID(m_nPlayerId);
            if (pPlayer)
                sValue = pPlayer->m_sPlayerName;
            else
                sValue = g_pAppManager->m_pClientExoApp->GetGUIString(static_cast<unsigned int>(pToken.m_nStringRef));
        }
        break;
#endif

    case TOKEN_ACTION_CHARACTER:
        sValue.Format(const_cast<char*>("%c"), static_cast<int>(pToken.m_nStringRef));
        break;
    case TOKEN_ACTION_KEY_NAME:
        // Key name for the event id in the token, preferring the secondary binding when the input
        // event has one.
        {
            int nNumSubstrings = static_cast<int>(pToken.m_StrRefs[0]);
            sValue = g_pTlkTable->GetSimpleString(0);
            CExoInputEventDesc* pEvent =
                static_cast<CExoInputEventDesc*>(g_pExoInput->GetEvent(static_cast<unsigned int>(pToken.m_nStringRef), -1));
            if (pEvent)
            {
                if (pEvent->Is2Button() && nNumSubstrings >= 2)
                {
                    if (static_cast<CExoInputeventDesc2ButtonAxis*>(pEvent)->m_nKeyCode2)
                        sValue = g_pAppManager->m_pClientExoApp->GetDisplayCharacterForKey(
                            static_cast<CExoInputeventDesc2ButtonAxis*>(pEvent)->m_nKeyCode2);
                }
                else
                {
                    if (pEvent->m_nKeyCode)
                        sValue = g_pAppManager->m_pClientExoApp->GetDisplayCharacterForKey(pEvent->m_nKeyCode);
                }
            }
        }
        break;
    case TOKEN_ACTION_MOUSE_BUTTON:
        // Mouse-button name. With reversed buttons the swapped name is a contiguous offset from the
        // first strref; otherwise the permuted table above applies.
        {
            int64_t nACType = pToken.m_nStringRef;
            uint64_t uStrRef;
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons)
            {
                uint64_t uIdx = static_cast<uint64_t>(static_cast<unsigned int>(nACType));
                if (uIdx < 4UL)
                    uStrRef = uIdx + STRREF_MOUSE_BUTTON_LEFT;
                else
                    uStrRef = 0;
            }
            else
            {
                if (static_cast<unsigned int>(nACType) < 4U)
                    uStrRef = static_cast<uint64_t>(arACStrRefs[nACType]);
                else
                    uStrRef = 0;
            }
            sValue = g_pTlkTable->GetSimpleString(uStrRef);
        }
        break;
#if !KOTOR1
    case TOKEN_ACTION_BOUNTY_VALUE:
        // The Nar Shaddaa bounty payment, scaled by a module global variable.
        {
            CExoString sBounty("300NAR_Bounty");
            char cValue = 0;
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(sBounty, cValue);
            sValue.Format(const_cast<char*>("%d"), static_cast<int>(cValue) * 500);
        }
        break;
#endif
    }

    // Trim the spaces a token substitution can leave at either end.
#if !KOTOR1
    while (sValue.GetLength() >= 1 && sValue[0] == ' ')
        sValue = sValue.Right(sValue.GetLength() - 1);
    while (sValue.GetLength() >= 1 && sValue[sValue.GetLength() - 1] == ' ')
        sValue = sValue.Left(sValue.GetLength() - 1);
#endif

    return;
}

// The gender of the current speaker, clamped to male when out of range.
unsigned char CSWTlkTable::GetObjectGender()
{
    if (m_pSpeakerObj != 0 &&
        m_pSpeakerObj->AsSWCObject() != 0 &&
        m_pSpeakerObj->AsSWCCreature() != 0 &&
        m_pSpeakerObj->AsSWCCreature()->m_pStats != 0)
    {
#if KOTOR1
        // K1 returns the stored gender byte without clamping.
        return m_pSpeakerObj->AsSWCCreature()->m_pStats->m_nGender;
#else
        unsigned char nGender = m_pSpeakerObj->AsSWCCreature()->m_pStats->m_nGender;
        if (nGender > CTlkTable::GENDER_FEMALE)
            return CTlkTable::GENDER_MALE;
        return nGender;
#endif
    }
    return CTlkTable::GENDER_MALE;
}
