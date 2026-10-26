// The demo and movie-poster screen, a full-display poster with an optional caption that dismisses
// itself after a deadline.

#include "swguiexamine.h"

#include "exobase.h"
#include "exotimersinternal.h"
#include "exotlktable.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "swguilegalscreen.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "exoresman.h"

#if !KOTOR1
unsigned char g_fullScreenDesired = 1;
#endif

extern CTlkTable* g_pTlkTable;

#if !KOTOR1  // K1 has no CSWDemoScreen
// Fills the display from the manager's cached screen size, loads the layout and its one label, then
// arms the auto-dismiss deadline: a timer stamp plus a tick count, with a one-tick deadline for an
// input event and two ticks for a frame.
CSWDemoScreen::CSWDemoScreen(CSWGuiManager* pGuiManager, unsigned int a, unsigned int b,
                             unsigned int c, unsigned int d)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sText;

    SetExtent(
        CSWGuiExtent(0, 0, pGuiManager->m_nScreenWidth, pGuiManager->m_nScreenHeight));

    StartLoadFromLayout(CResRef("demo_p"), 1, 1, 1);
    InitControl(m_label, CExoString("LBL_DEMOTEXT"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    m_nStartTime = 0;
    m_nDuration = a;
    m_nStartTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
    m_bSuppressGlobalFade = 1;

    if (b != 0)
    {
        sText = g_pTlkTable->GetSimpleString(b);
        g_pTlkTable->ParseStr(sText);

        CSWGuiExtent labelExtent = m_label.m_extent;
        if (c != 0) labelExtent.x = static_cast<int>(c);
        if (d != 0) labelExtent.y = static_cast<int>(d);

        m_label.m_text.m_params.SetText(sText);
        m_label.SetExtent(labelExtent);
        m_label.m_tail.bVisible = 1;
    }
    else
    {
        m_label.m_text.m_params.SetText(CExoString(""));
    }
}

// Past the deadline the panel is deleted and the screen fades out and hides.
void CSWDemoScreen::Update(float fDeltaTime)
{
    K2_UNUSED(fDeltaTime);
    if (g_pExoBase->m_pTimers->GetLowResolutionTimer() < m_nStartTime + 2 * m_nDuration)
        return;

    m_bSuppressGlobalFade = 0;

    SetToBeDeleted(0);

    g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(
        1, 0.0f, 1.0f, Vector(0.0f, 0.0f, 0.0f));
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDemoScreen();
}

// The same dismissal as Update with a one-tick deadline and no chain walk; any input expires the
// screen.
void CSWDemoScreen::HandleInputEvent(int type, int val)
{
    K2_UNUSED(type); K2_UNUSED(val);
    if (g_pExoBase->m_pTimers->GetLowResolutionTimer() < m_nStartTime + m_nDuration)
        return;

    m_bSuppressGlobalFade = 0;

    g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(
        1, 0.0f, 1.0f, Vector(0.0f, 0.0f, 0.0f));
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDemoScreen();
}

// Swaps the poster image in and latches the timer so the screen knows when the swap happened.
void CSWDemoScreen::SetImage(const CResRef& resRef)
{
    m_fAlpha = 1.0f;
    SetBackground(resRef);
    m_nStartTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
}
#endif // !KOTOR1

// The boot legal/copyright screen, built on the first draw. It owns a single fade scalar. SetImage
// cross-fades the background out and back in, with m_fadeBusy marking that a fade is in flight.

void UpdateScreen(float fDeltaTime, int a, int b);
#include "port/win32api.h"

CSWGuiLegalScreen::CSWGuiLegalScreen(CSWGuiManager* pGuiManager, unsigned int nArg)
    : CSWGuiPanel(pGuiManager)
{
    CSWGuiExtent extent(0, 0, pGuiManager->m_nScreenWidth, pGuiManager->m_nScreenHeight);
    SetExtent(extent);
    m_fadeBusy = 0;
    m_nArg = nArg;
}

CSWGuiLegalScreen::~CSWGuiLegalScreen()
{
}

// Cross-fades the background: the old image fades out and the new one fades back in, a frame at a
// time, advancing the alpha by a scaled timer delta.
void CSWGuiLegalScreen::SetImage(const CResRef& resRef)
{
    if (m_fadeBusy != 0)
    {
        float fAlpha = m_fAlpha;

        while (true)
        {
            if (g_pExoBase->m_pTimers->GetLowResolutionTimer() >= static_cast<unsigned int>(m_fadeBusy + m_nArg))
                break;
            Sleep(50);
        }

        unsigned int nTimer = g_pExoBase->m_pTimers->GetLowResolutionTimer();
        unsigned int nPrev = nTimer;
        while (fAlpha > 0.01f)
        {
            fAlpha += static_cast<float>(nTimer - nPrev) / -250.0f;
            nPrev = nTimer;
            nTimer = g_pExoBase->m_pTimers->GetLowResolutionTimer();
            float fValue = fAlpha;
            if (fValue < 0.0f)
                fValue = 0.0f;
            fValue = fValue < 1.0f ? fValue : 1.0f;
            m_fAlpha = fValue > 0.0f ? fValue : 0.0f;
            UpdateScreen(1.0f / 30.0f, 0, 0);
            Sleep(20);
        }
    }

    m_fAlpha = 0.0f;
    SetBackground(resRef);
#if !KOTOR1  // K1 leaves the background image's fill style alone
    m_pBackground->m_params.m_flags.nFillStyle = 2;
#endif

    if (resRef.IsValid())
    {
        UpdateScreen(1.0f / 30.0f, 0, 0);

#if KOTOR1
        float fAlpha = 0.0f;
        unsigned int nTimer = g_pExoBase->m_pTimers->GetLowResolutionTimer();
#else
        unsigned int nTimer = g_pExoBase->m_pTimers->GetLowResolutionTimer();
        float fAlpha = 0.0f;
#endif
        unsigned int nPrev = nTimer;
        while (fAlpha < 0.99)
        {
            fAlpha += static_cast<float>(nTimer - nPrev) / 250.0f;
            nPrev = nTimer;
            nTimer = g_pExoBase->m_pTimers->GetLowResolutionTimer();
            float fValue = fAlpha;
            if (fValue > 1.0f)
                fValue = 1.0f;
            m_fAlpha = fValue > 0.0f ? fValue : 0.0f;
            UpdateScreen(1.0f / 30.0f, 0, 0);
            Sleep(20);
        }
    }

    m_fadeBusy = g_pExoBase->m_pTimers->GetLowResolutionTimer();
}
