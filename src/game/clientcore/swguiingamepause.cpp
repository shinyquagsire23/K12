#include "k2stdint.h"
#include "swguiingamepause.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "swminigame.h"
#include "globalvartables.h"
#include "serverexoapp.h"
#include "exotlktable.h"
#include "exoresman.h"
#include "port/k2qolapi.h"
#include "exoinput.h"
#include "strref.h"

extern CTlkTable* g_pTlkTable;

void AspyrAddLabelIcon(CSWGuiLabel* pLabel, char nIcon);

// The two parallel reason tables hold a visibility flag and a TLK strref per entry.
static const int           aPauseReasonFlags[11]  = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 };
static const uint64_t aPauseReasonStrRefs[11] = {
    STRREF_PAUSE_ENEMY_SIGHTED, STRREF_PAUSED, STRREF_PAUSED, STRREF_PAUSED,
    STRREF_PAUSE_END_OF_COMBAT_ROUND, STRREF_PAUSED, STRREF_PAUSE_MENU_USED,
    STRREF_PAUSE_TARGET_CHANGED, STRREF_PAUSE_PARTY_MEMBER_DOWN, STRREF_PAUSE_ACTION_ADDED,
    STRREF_PAUSE_MINE_SIGHTED,
};

#if KOTOR1
#include "port/k1gui.h"
extern bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
#endif

#ifdef K2_QOL_IMPROVEMENTS

// Sizes the pause banner ("Enemy Sighted!", "Paused" plus the unpause hint) for the Text Size. Its
// box is the HUD's fixed strip, so it grows down by whatever the text lacks at the current setting.
static void K2QolSizePauseBanner(CSWGuiInGamePause* pPause)
{
    // The row may still carry a grown size from SetPauseReason, so re-seat it on the panel's
    // current box before anything else.
    CSWGuiExtent extent = pPause->m_extent;

    // Do not re-seat the row on a strip that has no width: the PC HUD has no TGuiLabel1, which
    // SetupPauseGuiExtent derives the strip from, so the width can come out negative. That state
    // gives the label a zero wrap width and a negative centring offset, which the string wrapper
    // cannot take (its scan reads one byte before the text buffer). Leave the banner as the layout
    // left it.
    if (extent.w <= 0)
        return;

    CSWGuiExtent extentBase(0, 0, extent.w, extent.h);
    pPause->m_label1.SetExtent(extentBase);
    pPause->m_button.SetExtent(extentBase);
    CSWGuiText& text = pPause->m_label1.m_text;
    int nBox = text.m_extent.h;
    int nWant = K2QolFitText(text, 0x7fffffff);
    int nMax = pPause->m_pGuiManager->m_nScreenHeight / 3;
    int nNewH = extent.h + (nWant > nBox ? nWant - nBox : 0);
    if (nNewH > nMax)
        nNewH = nMax > extent.h ? nMax : extent.h;
    if (nNewH != extent.h) {
        extent.h = nNewH;
        pPause->SetExtent(extent);
        CSWGuiExtent extentRow(0, 0, extent.w, extent.h);
        pPause->m_label1.SetExtent(extentRow);
        pPause->m_button.SetExtent(extentRow);
    }
    // Leave a margin each side. The label's text area is the whole strip, so a long hint line
    // wrapped to it ran into the frame.
    CSWGuiExtent extentText = text.m_extent;
    int nInset = extentText.w / 32;
    extentText.x += nInset;
    extentText.w -= nInset * 2;
    text.SetExtent(extentText);
    K2QolFitText(text, text.m_extent.h);
}
#endif

CSWGuiInGamePause::CSWGuiInGamePause(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    StartLoadFromLayout(CResRef("pause"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("pause_p"), 1, 1, 1);
#endif

    InitControl(m_label1, CExoString("LBL_PAUSEREASON"), 1, 1, 0, 0, 0);
    InitControl(m_label2, CExoString("LBL_PRESS"),       1, 1, 0, 0, 0);
    InitControl(m_button, CExoString("BTN_UNPAUSE"),     1, 1, 0, 0, 0);

    m_stateFlags |= 1;
    StopLoadFromLayout();
#if KOTOR1
    ScaleExtentForResolution();
#endif

    m_nExtentY = m_extent.y;
    m_nExtentX = m_extent.x;

    // The layout's own caption for the pad row is expanded through the TLK before being installed.
    CExoString sPressText(m_label2.m_text.m_params.m_sText);
    g_pTlkTable->ParseStr(sPressText);
    m_label2.m_text.m_params.SetText(sPressText);

    m_label1.m_text.m_params.bHilight = 1;
    m_label2.m_text.m_params.bHilight = 1;
    m_button.m_tail.bFocusOnHover = 0;

    m_button.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_button.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGamePause::OnUnpauseButtonPressed));
}

// Clears the pause. SetPausedByCombat gets the negation of the current paused-by-combat state, and
// SetAutoPaused is only reached when something actually auto-paused.
void CSWGuiInGamePause::OnUnpauseButtonPressed(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        int bPaused = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(bPaused == 0, 4, 0);

        if (g_pAppManager->m_pClientExoApp->GetAutoPaused() != 0)
            g_pAppManager->m_pClientExoApp->SetAutoPaused(0, 0);
    }
}

// Sets the panel's whole visible state: the reason tables pick a caption and a strref, a "press"
// reason also shows the pad row below it, and the row's bottom plus a margin becomes both the
// panel's height and the unpause button's.
void CSWGuiInGamePause::SetPauseReason(unsigned char reason)
{
    int nLabelFlags;
    uint64_t nStrRef;
    char nIndex = reason - 1;
    if (static_cast<unsigned char>(nIndex) <= 10)
    {
        nLabelFlags = aPauseReasonFlags[static_cast<int>(nIndex)];
        nStrRef = aPauseReasonStrRefs[static_cast<int>(nIndex)];
    }
    else
    {
        nLabelFlags = 1;
        nStrRef = STRREF_PAUSED;
    }

    CSWGuiText* pText = &m_label1.m_text;

    CExoString sReasonText(g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef));
    m_label1.m_text.m_params.SetText(sReasonText);

#if KOTOR1
    // K1: no press caption, no main-interface extent and no caption merge; the two rows are stacked
    // with the phone text scale, and the panel and button take the stacked height.
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = g_SDL_mode.h / 768.0f;
#else
    static float s_fScale = g_SDL_mode.h / 768.0f;
#endif
    CSWGuiExtent extent;
    CSWGuiExtent extentButton;
    int nScreenWidth;
    int nScreenHeight;

    int nIdealReason = pText->GetIdealHeight();
    if (IosIsIphone(nScreenWidth, nScreenHeight))
        nIdealReason = static_cast<int>(s_fScale * 4.0f + nIdealReason);
    extent.x = m_label1.m_extent.x;
    extent.y = m_label1.m_extent.y;
    extent.w = m_label1.m_extent.w;
    extent.h = nIdealReason;
    m_label1.SetExtent(extent);

    extentButton = extent;

    if (nLabelFlags != 0)
    {
        m_label2.m_tail.bVisible = 1;

        CSWGuiText* pText2 = &m_label2.m_text;
        int nPressY = static_cast<int>(s_fScale + s_fScale + (extent.y + nIdealReason));
        int nIdealPress = pText2->GetIdealHeight();
        if (IosIsIphone(nScreenWidth, nScreenHeight))
            nIdealPress = static_cast<int>(s_fScale * 4.0f + nIdealPress);
        extent = m_label2.m_extent;
        extent.h = nIdealPress;
        extent.y = nPressY;
        m_label2.SetExtent(extent);
    }
    else
    {
        m_label2.m_tail.bVisible = 0;
    }

    CSWGuiExtent extentPanel;
    extentPanel.x = m_extent.x;
    extentPanel.y = m_extent.y;
    extentPanel.w = m_extent.w;
    extentPanel.h = static_cast<int>(s_fScale * 5.0f + (extent.y + extent.h));
    SetExtent(extentPanel);

    extentButton.h = extentPanel.h;
    m_button.SetExtent(extentButton);
#else

    CSWGuiExtent extent;
    int nIdealReason = pText->GetIdealHeight();
    extent.x = m_label1.m_extent.x;
    extent.y = m_label1.m_extent.y;
    extent.w = m_label1.m_extent.w;
    extent.h = nIdealReason;
    m_label1.SetExtent(extent);

    CSWGuiExtent extentButton;
    extentButton = extent;

    if (nLabelFlags != 0)
    {
        m_label2.m_tail.bVisible = 1;

        CSWGuiText* pText2;
        int nPrevY;
        int nPrevH;
        int nIdealPress;
        CExoString sPressText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_PRESS_PAUSE_TO_CONTINUE));
        g_pTlkTable->ParseStr(sPressText);
        m_label2.m_text.m_params.SetText(sPressText);

        pText2 = &m_label2.m_text;
        nPrevY = extent.y;
        nPrevH = extent.h;
        nIdealPress = pText2->GetIdealHeight();
        extent.x = m_label2.m_extent.x;
        extent.w = m_label2.m_extent.w;
        extent.h = nIdealPress;
        extent.y = nPrevY + nPrevH + 2;
        m_label2.SetExtent(extent);
    }
    else
    {
        m_label2.m_tail.bVisible = 0;
    }

    CSWGuiExtent extentPanel;
    extentPanel.x = m_extent.x;
    extentPanel.y = m_extent.y;
    extentPanel.w = m_extent.w;
    extentPanel.h = extent.y + extent.h + 5;
    SetExtent(extentPanel);

    extentButton.h = extentPanel.h;
    m_button.SetExtent(extentButton);

    CSWGuiExtent extentMain;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetupPauseGuiExtent(&extentMain);
    SetExtent(extentMain);

    CSWGuiExtent extentRow(0, 0, extentMain.w, extentMain.h);
    m_label1.SetExtent(extentRow);
    m_label1.m_text.m_params.SetAlignment(SWGUIALIGN_CENTER | SWGUIALIGN_MIDDLE);

    if (m_label2.m_tail.bVisible)
    {
        CExoString sMerged = m_label1.m_text.m_params.m_sText
                           + CExoString("\n")
                           + m_label2.m_text.m_params.m_sText;
        m_label1.m_text.m_params.SetText(sMerged);
        m_label2.m_tail.bVisible = 0;
    }

    m_button.SetExtent(extentRow);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolSizePauseBanner(this);
#endif
#endif
}

// Forwards straight to CSWGuiPanel::Draw.
void CSWGuiInGamePause::Draw(float f)
{
    CSWGuiPanel::Draw(f);
}

// Two jobs, split on whether a swoop minigame exists: with none the main interface owns the
// geometry, while with one the panel centres itself on the cached screen size and captions m_label2
// from one of two strrefs, only the first of which gets the pad glyph.
void CSWGuiInGamePause::OnPanelAdded()
{
#if KOTOR1  // K1: only the geometry -- the extent is computed on a copy and applied through SetExtent
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    if (pMiniGame != 0)
    {
        CSWGuiExtent extent = m_extent;
        extent.x = (m_pGuiManager->m_nScreenWidth - extent.w) / 2;
        extent.y = (m_pGuiManager->m_nScreenHeight - extent.h) / 2;
        CSWGuiObject::SetExtent(extent);
    }
    else
    {
        CSWGuiExtent extent = m_extent;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetupPauseGuiExtent(&extent);
        CSWGuiObject::SetExtent(extent);
    }
#else
    CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
    if (pMiniGame != 0)
    {
        int nMiniGameType = pMiniGame->m_type;

        m_extent.x = (m_pGuiManager->m_nScreenWidth - m_extent.w) / 2;
        m_extent.y = (m_pGuiManager->m_nScreenHeight - m_extent.h) / 2;

        m_label1.m_text.m_params.SetStrRef(STRREF_PAUSED);

        if (nMiniGameType == CSWMiniGame::MINIGAME_TYPE_SWOOP)
        {
            CExoString sPadText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_PRESS_TO_FORFEIT));
            g_pTlkTable->ParseStr(sPadText);
            m_label2.m_text.m_params.SetText(sPadText);
            AspyrAddLabelIcon(&m_label2, 'y');
        }
        else
        {
            CExoString sPadText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_PRESS_TO_UNPAUSE_MINIGAME));
            g_pTlkTable->ParseStr(sPadText);
            m_label2.m_text.m_params.SetText(sPadText);
        }
    }
    else
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetupPauseGuiExtent(&m_extent);
#ifdef K2_QOL_IMPROVEMENTS
        K2QolSizePauseBanner(this);   // the reset above is the base strip again
#endif
    }
#endif

    CSWGuiPanel::OnPanelAdded();
}

// Quits a swoop race on the A button, setting the quit global, unpausing and tearing the pause
// panel down. Every other input falls through to the base.
#if !KOTOR1  // K1 does not override this slot
void CSWGuiInGamePause::HandleInputEvent(int type, int val)
{
    if (type == INPUTEVENT_GUI_A && val != 0)
    {
        CSWMiniGame* pMiniGame = g_pAppManager->m_pClientExoApp->GetMiniGame();
        if (pMiniGame != 0 && pMiniGame->m_type == CSWMiniGame::MINIGAME_TYPE_SWOOP)
        {
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueBoolean(
                CExoString("GBL_QUIT_SWOOP"), 1);

            g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 4, 0);
            g_pAppManager->m_pClientExoApp->SetAutoPaused(0, 0);
            m_pGuiManager->RemovePanel(this);
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}
#endif

// ShouldFreeMouseOnAdd is defined in the header and returns false.
