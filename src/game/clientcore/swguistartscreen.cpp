// The boot/title panel.

#include "swguistartscreen.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swgui.h"
#include "exoresman.h"
#include "exostring.h"
#include "exoinput.h"

CSWGuiStartScreen::CSWGuiStartScreen(CSWGuiManager* pManager)
    : CSWGuiPanel(pManager)
{
    if (m_pGuiManager == 0)
        m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();

#if KOTOR1  // K1: the plain "startscreen" layout with the start text only, rescaled
    StartLoadFromLayout(CResRef("startscreen"), 1, 1, 1);
    InitControl(m_labelStart, CExoString("LBL_STARTTEXT"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    ScaleExtentForResolution();
#else
    StartLoadFromLayout(CResRef("startscreen_p"), 1, 1, 1);
    InitControl(m_labelStart, CExoString("LBL_STARTTEXT"), 1, 1, 0, 0, 0);
    InitControl(m_labelLogo, CExoString("LBL_GAMELOGO"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#endif

    m_labelStart.m_text.m_params.bHilight |= 1;
    m_fTimer = 0.0f;
    m_bResetTimer |= 1;

    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalUnpauseableState(0);
}

void CSWGuiStartScreen::HandleInputEvent(int type, int val)
{
    if (val && (type == INPUTEVENT_GUI_START || type == INPUTEVENT_GUI_A)) {
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        g_pAppManager->m_pClientExoApp->DisplayMainMenu(1);
    }
}

// Suppressed entirely while a movie is playing, the base Draw call included.
void CSWGuiStartScreen::Draw(float f)
{
    if (g_pAppManager->m_pClientExoApp->IsMoviePlaying() == 0) {
        unsigned char bWaiting = m_bResetTimer;
        m_fTimer += f;
        if (bWaiting & STARTSCREEN_RESET_TIMER) {
            m_fTimer = 0.0f;
            m_bResetTimer = bWaiting & ~STARTSCREEN_RESET_TIMER;
        }

        CSWGuiPanel::Draw(f);

#if KOTOR1
        // K1: the "sizzle" reel plays after 90 s; a nonzero result goes straight to the main menu.
        if (m_fTimer > 90.0f) {
            int nResult = 0;
            m_fTimer = 0.0f;
            g_pAppManager->m_pClientExoApp->PlayMovie(CExoString("sizzle"), nResult, 1, 0, 0);
            m_bResetTimer |= 1;
            if (nResult)
                g_pAppManager->m_pClientExoApp->DisplayMainMenu(1);
        }
#else
        if (m_fTimer > 60.0f) {
            int nStart = 0;  // the trailer's start position, passed by reference
            m_fTimer = 0.0f;
            g_pAppManager->m_pClientExoApp->PlayMovie(CExoString("Trailer"), nStart, 1, 0, 0);
            m_bResetTimer |= 1;
        }
#endif
    }
}
