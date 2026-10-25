#include "swguiingamesolomodequery.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "nwccreature.h"
#include "nwscreature.h"
#include "nwsobject.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "nwsarea.h"
#include "partytables.h"
#include "serverexoapp.h"
#include "exoinput.h"
#include "strref.h"

CSWGuiInGameSoloModeQuery::CSWGuiInGameSoloModeQuery(CSWGuiManager* pGuiManager)
    : CSWGuiMessageBox(pGuiManager)
{
    m_nQueryType = 0;
}

// Hides the box when the dialog has no business being on screen, then draws the base.
// Both GetModule() calls are deliberate.
void CSWGuiInGameSoloModeQuery::Draw(float f)
{
    CSWCCreature* pPlayerCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    CSWSCreature* pServerCreature =
        pPlayerCreature != 0 ? pPlayerCreature->GetServerCreature() : 0;

    CSWSArea* pServerArea = 0;
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule != 0)
    {
        pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule->m_pArea != 0)
        {
            pServerArea = pModule->m_pArea->GetServerArea();
        }
    }

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0
        || (pServerArea != 0 && pServerArea->m_bTransitionPending != 0)
        || g_pAppManager->m_pClientExoApp->GetAreaNotReadyToDisplay() != 0
        || (pServerCreature != 0
            && (pServerCreature->GetDead() != 0
                || pServerCreature->GetDeadTemp() != 0)))
    {
        // The second assignment overwrites the first in the guarded case; the pair stays unfolded.
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
        m_stateFlags = nFlags;
    }

#if KOTOR1 && defined(K1_UI_XBOX)
    CSWGuiMessageBox::Draw(f);   // also places the A / B glyphs
#else
    CSWGuiPanel::Draw(f);
#endif
}

void CSWGuiInGameSoloModeQuery::SetQueryText(int text)
{
    if (text != 0)
    {
        SetMessage(STRREF_SOLO_MODE_FOR_STEALTH_QUERY);
    }
    else if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_soloMode != 0)
    {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
        if ((pCreature->m_nActivityFlags & 1) != 0)
            SetMessage(STRREF_SOLO_MODE_OFF_ENDS_STEALTH_QUERY);
        else
            SetMessage(STRREF_SOLO_MODE_OFF_QUERY);
    }
    else
    {
        SetMessage(STRREF_SOLO_MODE_ON_QUERY);
    }
    m_nQueryType = text;
}

// Accepting toggles party-follow and, when a query was armed, rolls the solo skill on the player.
// That arm and the pure-dismissal ids fall through to the pop; everything else goes to the base.
void CSWGuiInGameSoloModeQuery::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
            case INPUTEVENT_GUI_A:
            case INPUTEVENT_GUI_START:
            case SWGUIEVENT_MSGBOX_OK:
            {
                CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
                g_pAppManager->m_pClientExoApp->TogglePartyFollow();
                if (pCreature != 0 && m_nQueryType != 0)
                    pCreature->PerformSkill(2, 0, pCreature->m_objectId);
            }
            // Falls through: accepting always dismisses the box too.
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_BACK:
            case SWGUIEVENT_MSGBOX_CANCEL:
                g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSoloMode();
                return;
            default:
                break;
        }
    }
    CSWGuiMessageBox::HandleInputEvent(type, val);
}
