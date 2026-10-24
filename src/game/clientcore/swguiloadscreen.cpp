#include "k2stdint.h"
#include "swguiloadscreen.h"
#include "exoresman.h"
#include "swgui.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoappinternal.h"
#include "exotlktable.h"
#include "guiingame.h"
#include "port/k2qolapi.h"
#include "restype.h"

#if KOTOR1
#include "port/k1gui.h"
#endif

#ifdef K2_QOL_IMPROVEMENTS

// The loading label's box as the layout made it; sizing starts from here rather than from the
// label's last growth.
static CSWGuiExtent s_extentK2QolLoading;

// Grows the "Loading" caption upward from the framed strip above the progress bar by whatever the
// text lacks at the current Text Size, capped at a sixth of the screen height.
static void K2QolSizeLoadingLabel(CSWGuiLoadScreen* pScreen)
{
    CSWGuiExtent extent = s_extentK2QolLoading;
    if (extent.h <= 0)
        return;
    CSWGuiLabel& label = pScreen->m_lblLoading;
    label.SetExtent(extent);
    CSWGuiText& text = label.m_text;
    int nFrame = extent.h - text.m_extent.h;
    int nNewH = K2QolFitText(text, 0x7fffffff) + nFrame;
    int nMax = pScreen->m_pGuiManager->m_nScreenHeight / 6;
    if (nNewH > nMax)
        nNewH = nMax;
    if (nNewH > extent.h) {
        extent.y -= nNewH - extent.h;
        extent.h = nNewH;
        label.SetExtent(extent);
    }
    K2QolFitText(text, text.m_extent.h);
}
#endif

CSWGuiLoadScreen::CSWGuiLoadScreen(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: the "loadscreen" layout, a black fill (the resolution string is computed then
    // overwritten), and the panel rescaled and centred as a 4:3 box of the SDL mode's height.
    StartLoadFromLayout(CResRef("loadscreen"), 1, 1, 0);
    InitControl(m_progressBar, CExoString("PB_PROGRESS"), 1, 1, 0, 0, 0);
    InitControl(m_lblHint, CExoString("LBL_HINT"), 1, 1, 0, 0, 0);
    InitControl(m_lblLogo, CExoString("LBL_LOGO"), 1, 1, 0, 0, 0);
    InitControl(m_lblLoading, CExoString("LBL_LOADING"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    m_progressBar.SetCurValue(0);
    m_lblHint.m_text.m_params.SetText(CExoString(""));
    m_lblBackground.SetExtent(
        CSWGuiExtent(0, 0, pGuiManager->m_nScreenWidth, pGuiManager->m_nScreenHeight));

    CExoString name = CSWGuiManager::GetScreenResolutionString(g_nScreenWidth, g_nScreenHeight);
    name = "black";
    m_lblBackground.m_border.m_params.SetFillImage(CResRef(name), 0);
    m_lblBackground.m_border.m_params.m_flags.nFillStyle = 2;

    ScaleExtentForResolution();
    int nWidth = static_cast<int>(g_SDL_mode.h / 768.0f * 1024.0f);
    SetExtent(CSWGuiExtent((g_SDL_mode.w - nWidth) / 2, 0, nWidth, g_SDL_mode.h));
#else
    StartLoadFromLayout(CResRef("loadscreen_p"), 1, 1, 0);
    m_stateFlags |= 1;

    InitControl(m_progressBar, CExoString("PB_PROGRESS"), 1, 1, 0, 0, 0);
    InitControl(m_lblHint, CExoString("LBL_HINT"), 1, 1, 0, 0, 0);
    InitControl(m_lblLogo, CExoString("LBL_LOGO"), 1, 1, 0, 0, 0);
    InitControl(m_lblLoading, CExoString("LBL_LOADING"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    m_progressBar.SetCurValue(0);
    m_lblHint.m_text.m_params.SetText(CExoString(""));

    // The background label is the whole screen.
    m_lblBackground.SetExtent(
        CSWGuiExtent(0, 0, pGuiManager->m_nScreenWidth, pGuiManager->m_nScreenHeight));

    CExoString name("800x600");
    if (g_nScreenHeight >= 1200)
        name = "1600x1200";
    else if (g_nScreenHeight >= 1024)
        name = "1280x1024";
    else if (g_nScreenHeight >= 960)
        name = "1280x960";
    else if (g_nScreenHeight >= 768)
        name = "1024x768";

    name = name + "load";
    m_lblBackground.m_border.m_params.SetFillImage(CResRef(name), 0);
    m_lblBackground.m_border.m_params.m_flags.nFillStyle = 2;
    m_stateFlags &= ~(PANEL_STATE_CENTER_X | PANEL_STATE_CENTER_Y);

    // The logo keeps the background resref so OnPanelAdded can re-install it, and starts with no
    // fill image of its own.
    m_resRefBackground = m_lblLogo.m_border.m_params.m_fillImage;
    m_lblLogo.m_border.m_params.SetFillImage(CResRef(""), 0);
#ifdef K2_QOL_IMPROVEMENTS
    s_extentK2QolLoading = m_lblLoading.m_extent;
#endif
#endif
}

// Shows the given image as the background, but only when the module actually ships it.
void CSWGuiLoadScreen::SetImage(const CResRef& resRef)
{
    if (!g_pExoResMan->Exists(resRef, 3, static_cast<unsigned int*>(0)) &&
        !g_pExoResMan->Exists(resRef, RESTYPE_TPC, static_cast<unsigned int*>(0)))
        return;

    CSWGuiPanel::SetBackground(resRef);
#if KOTOR1
    // K1: fit the panel background and the full-screen label to the image's aspect (115:48 of the
    // SDL height), centred on the 4:3 box and on the screen respectively.
    float fHeight = static_cast<float>(g_SDL_mode.h);
    int nBoxWidth = static_cast<int>(fHeight / 768.0f * 1024.0f);
    CSWGuiExtent extent;
    extent.w = static_cast<int>(__builtin_ceilf(fHeight * 115.0f / 48.0f));
    extent.h = g_SDL_mode.h;
    extent.x = (nBoxWidth - extent.w) / 2;
    extent.y = 0;
    m_pBackground->SetExtent(extent);
    extent.x = (g_SDL_mode.w - extent.w) / 2;
    m_lblBackground.m_border.m_params.SetFillImage(resRef, 0);
    m_lblBackground.SetExtent(extent);
#ifndef K2_UI_TOUCH
    // The progress bar's x and width are positions on the load art, so they follow the art's
    // horizontal stretch rather than the 4:3 box's.
    {
        // The layout's bar is taken once per screen size, since a rebuild at a new size would
        // otherwise reuse the first size's pixels.
        static CSWGuiExtent s_bar;
        static int s_nBarScreenHeight;
        if (s_bar.w == 0 || s_nBarScreenHeight != g_SDL_mode.h) {
            s_bar = m_progressBar.m_extent;
            s_nBarScreenHeight = g_SDL_mode.h;
        }
        float fUnit = fHeight / 768.0f;
        float fArt = static_cast<float>(m_pBackground->m_extent.w) / 1024.0f;
        CSWGuiExtent bar = s_bar;
        bar.x = m_pBackground->m_extent.x + static_cast<int>(static_cast<float>(s_bar.x) / fUnit * fArt);
        bar.w = static_cast<int>(static_cast<float>(s_bar.w) / fUnit * fArt);
        m_progressBar.SetExtent(bar);
    }
#endif
#endif
}

#if !KOTOR1  // K1 does not override this slot
void CSWGuiLoadScreen::OnPanelAdded()
{
    m_lblLogo.m_border.m_params.SetFillImage(m_resRefBackground, 0);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolSizeLoadingLabel(this);
#endif
    CSWGuiPanel::OnPanelAdded();
}
#endif

#if !KOTOR1  // K1 does not override this slot
void CSWGuiLoadScreen::OnPanelRemoved()
{
    m_lblLogo.m_border.m_params.SetFillImage(CResRef(""), 0);
    CSWGuiPanel::OnPanelRemoved();
}
#endif

// Stores the hint's strref so AddHintToMessageBuffer can resolve it again.
void CSWGuiLoadScreen::SetHint(uint64_t nStrRef)
{
    CExoString sText;
    if (nStrRef)
    {
        m_nHintStrRef = nStrRef;
        sText = g_pTlkTable->GetSimpleString(nStrRef);
        g_pTlkTable->ParseStr(sText);
    }
    else
    {
        m_nHintStrRef = 0;
    }

    m_lblHint.m_text.m_params.SetText(sText);
#ifdef K2_QOL_IMPROVEMENTS
    // The tip at the Dialog Text size, as far as LBL_HINT (fixed, below the progress bar) holds it.
    K2QolFitText(m_lblHint.m_text, m_lblHint.m_text.m_extent.h);
#endif
}

// Appends the hint to the dialog buffer.
void CSWGuiLoadScreen::AddHintToMessageBuffer()
{
    g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToDialogBuffer(
        m_lblHint.m_text.m_params.m_sText, CExoString(""));
}

// Draws the whole-screen background label; the panel's control walk draws the rest.
void CSWGuiLoadScreen::Draw(float f)
{
    AurGUIStartLayer();
    if (AurGUISetupViewport(0, 0, m_pGuiManager->m_nScreenWidth, m_pGuiManager->m_nScreenHeight,
                            CGuiInGame::COLOR_BLACK, 0, 1.0f))
    {
        m_lblBackground.Draw(f);
        AurGUICloseViewport();
    }
    AurGUIStopLayer();

    CSWGuiPanel::Draw(f);
}
