// The in-game conversation screens, made of the dialog panel base and the terminal, cinematic,
// letterbox and camera overlays built on it.

#include "swguidialog.h"
#include "port/sdlapi.h"

#include "Achievements.h"
#include "clientoptions.h"
#include "guiingame.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "exostring.h"
#include "CExoArrayListImpl.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "exosound.h"
#include "CExoArrayListDtors.h"
#include "appmanager.h"
#include "swparty.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwscreature.h"
#include "nwsitem.h"
#include "nwsrules.h"
#include "nwskill.h"
#include "exoresman.h"
#include <stdio.h>
#include <algorithm>
#include "port/k2qolapi.h"
#include "exoinput.h"
#include "nwscriptconst.h"

extern int s_iosJoystickReplySelection;
#if KOTOR1
#include <math.h>
// K1 sizes its dialog UI against a 768-line screen.
#endif

bool ASLPlat_IsControllerPlugged();

// This port renders dialogs through CSWGuiDialog, so the iOS dialog path is compiled out and the
// three helpers below are unreachable.
int IosNewDialog()
{
    return 1;
}

#if KOTOR1
extern bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
// K1 only: on a sub-1024-wide iPhone screen the safe width grows by half the shortfall.
void IosGetSafeWidth(int& nSafeWidth, int& nSafeHeight)
{
    K2_UNUSED(nSafeHeight);
    int nWidth;
    int nHeight;
    if (IosIsIphone(nWidth, nHeight) && nWidth <= 1023)
        nSafeWidth += (1024 - nWidth) / 2;
}
#else
void IosGetSafeWidth(int& nSafeWidth, int& nSafeHeight)
{
    K2_UNUSED(nSafeWidth); K2_UNUSED(nSafeHeight);
}
#endif

// Whitespace is space, tab and newline only; the scan runs to the terminator, so a line that
// starts with a non-space still costs a full walk.
bool IsAllWhitespace(const char* pszText)
{
    bool bAllWhitespace = true;
    while (*pszText != 0)
    {
#if KOTOR1  // K1 tests the two control characters first
        if (*pszText != '\t' && *pszText != '\n' && *pszText != ' ')
#else
        if (*pszText != ' ' && *pszText != '\t' && *pszText != '\n')
#endif
            bAllWhitespace = false;
        ++pszText;
    }
    return bAllWhitespace;
}

CSWGuiDialog::CSWGuiDialog(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    unsigned char nFlags = m_nMessageFlags;
    m_ownerId = OBJECT_INVALID;                                        // "no object" sentinel
    m_nMessageFlags = static_cast<unsigned char>(
        (nFlags & ~(DIALOGMSG_SHOWING_LINE | DIALOGMSG_LINE_PENDING | DIALOGMSG_DIALOG_ENDED))
        | (DIALOGMSG_SHOWING_LINE | DIALOGMSG_DIALOG_ENDED));
    Reset();
}

// Report the dialog close to the achievement tracker.
void CSWGuiDialog::EndDialog()
{
    m_nMessageFlags |= DIALOGMSG_DIALOG_ENDED;
#if !KOTOR1  // K1 has no achievement tracker
    Achievements::OnEndDialog();
#endif
}

void CSWGuiDialog::SetOwner(unsigned long long ownerId)
{
    m_ownerId = ownerId;
}

#ifdef K2_UI_TOUCH
void CSWGuiDialog::HandleSelectReply(CSWGuiControl*) {}
#else
void CSWGuiDialog::HandleSelectReply(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    SelectReply(m_listBox.m_nSelectedIndex);
}
#endif

// Bounds-checks the reply index against the live reply count and returns its id, or -1 when the
// index is negative or past the end.
unsigned int CSWGuiDialog::GetReplyIndex(int index)
{
    return (m_nReplyCount > index && index >= 0) ? m_aReplyIds[index] : static_cast<unsigned int>(-1);
}

// Paints the hovered reply label in the highlight colour.
void CSWGuiDialog::SetReplyActive(CSWGuiControl* pControl)
{
#ifndef K2_UI_TOUCH
#ifdef K2_UI_XBOX
    if (ASLPlat_IsControllerPlugged()) {
        pControl->AsLabel()->m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
        return;
    }
#endif
#if KOTOR1  // K1 caches no highlight colour and uses the selection yellow
    pControl->AsLabel()->m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
#else
    pControl->AsLabel()->m_text.m_params.SetColor(m_vColorNewHighlight);
#endif
#else
    if (ASLPlat_IsControllerPlugged())
        pControl->AsLabel()->m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
#endif
}

// Restores the reply's resting text colour.
void CSWGuiDialog::SetReplyInactive(CSWGuiControl* pControl)
{
    pControl->AsLabel()->m_text.m_params.SetColor(m_vColorNewText);
}

#if !KOTOR1  // K2 only
// Applies the reply the touch widget confirmed this frame, then clears the request.
void CSWGuiDialog::Update(float f)
{
    K2_UNUSED(f);
    if (s_iosJoystickReplySelection >= 0)
    {
        m_listBox.SetSelectedControl(s_iosJoystickReplySelection, 1);
        s_iosJoystickReplySelection = -1;
    }
}
#endif // !KOTOR1

// Hands a picked reply to the achievement tracker and the in-game GUI. An index past the live
// reply count is ignored, but the call still reports success.
int CSWGuiDialog::SelectReply(int n)
{
    if (m_nReplyCount <= n)
        return 0;

#if !KOTOR1  // K1 has no achievement tracker
    Achievements::OnDialogOptionChosen(n);
#endif
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetDialogSelection(n, m_nReplyCount, 1);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateDialog();
    return 1;
}

// Resets to a clean dialog: unhides the widgets SetDialogMessage hid, drops the reply count and the
// selection cursor, blanks the spoken line, and resets every label's colour and text.
void CSWGuiDialog::Reset()
{
    m_listBox.m_tail.bVisible = 0;
    m_msgLabel.m_tail.bVisible = 0;
    m_nSelectedReply = -1;
    m_nReplyCount = 0;
    m_sDialogMessage = "";

#if KOTOR1
    for (int i = 0; i < 20; i++)
#else
    for (int i = 0; i < 30; i++)
#endif
    {
        m_replyLabels[i].m_text.m_params.SetColor(m_vColorNewText);
        m_replyLabels[i].m_text.m_params.SetText(CExoString(""));
    }
}

// Shows the current line of dialogue, refreshing its text unless the feedback options are holding
// it back, then unhides the label that carries it.
void CSWGuiDialog::ShowDialogMessage()
{
    m_nMessageFlags &= ~DIALOGMSG_LINE_PENDING;

    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_bFeedback2 || g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bForceDialogText)
    {
        m_msgLabel.m_text.m_params.SetText(m_sDialogMessage);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bForceDialogText = 0;
    }

    m_msgLabel.m_tail.bVisible = 1;
}

// Handles reply-selection input: the reply range selects a live reply, the confirm pair advances
// the conversation once the line is up, the two cursor pairs move the selection, and the touch
// confirm skips ahead.
void CSWGuiDialog::HandleInputEvent(int type, int val)
{
    if (val == 0)
        return;

    int nReply = type - INPUTEVENT_DIALOG_1;
    if (static_cast<unsigned>(nReply) < 9)
    {
        if (nReply < m_nReplyCount)
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            SelectReply(nReply);
        }
    }
    else if (type == INPUTEVENT_GUI_A || type == INPUTEVENT_GUI_START)
    {
        // With the line already up, confirm fast-forwards the dialog instead of picking a reply.
        if ((m_nMessageFlags & DIALOGMSG_SHOWING_LINE) != 0)
        {
            if ((m_nMessageFlags & DIALOGMSG_LINE_PENDING) == 0)
                g_pAppManager->m_pServerExoApp->SkipPastDialog(
                    g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_ownerId));
        }
        else
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            SelectReply(m_listBox.m_nSelectedIndex);
        }
    }
    else if (type == INPUTEVENT_GUI_DPAD_UP || type == INPUTEVENT_GUI_KEY_UP)
    {
        if (m_nSelectedReply >= 1)
            m_nSelectedReply--;
    }
    else if (type == INPUTEVENT_GUI_DPAD_DOWN || type == INPUTEVENT_GUI_KEY_DOWN)
    {
        if (m_nSelectedReply < m_nReplyCount - 1)
            m_nSelectedReply++;
    }
    else if (type == SWGUIEVENT_DOUBLE_CLICK)
    {
        if ((m_nMessageFlags & (DIALOGMSG_SHOWING_LINE | DIALOGMSG_LINE_PENDING)) == DIALOGMSG_SHOWING_LINE)
            g_pAppManager->m_pServerExoApp->SkipPastDialog(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_ownerId));
    }

    if (type == INPUTEVENT_GUI_A || type == INPUTEVENT_GUI_START)
        return;
    if (static_cast<unsigned int>(type) >= 2 && (m_nMessageFlags & DIALOGMSG_SHOWING_LINE) != 0)
        return;

    CSWGuiPanel::HandleInputEvent(type, val);
}

// Lays out a fresh reply set from n strings and their parallel id array. d re-lays out the list and
// sends the selection cursor home; c is part of the virtual's signature only.
void CSWGuiDialog::SetReplies(int n, CExoString* pa, unsigned int* pb, int c, int d)
{
    K2_UNUSED(c);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
    Reset();

    CSWGuiExtent extent;
    CExoArrayList<CSWGuiControl*> aControls;
    CSWGuiLabel* pLabel;
    CSWGuiText* pText;
    CExoString sText;

#if KOTOR1
    m_nReplyCount = n < 20 ? n : 20;
#else
    m_nReplyCount = n < 30 ? n : 30;
#endif

    for (int i = 0; i < m_nReplyCount; i++)
    {
#if KOTOR1  // K1 uses the out-of-line IsEmpty test
        if (!pa[i].IsEmpty())
#else
        if (pa[i].m_sString != 0 && pa[i].m_sString[0] != 0)
#endif
        {
#if KOTOR1  // K1 formats the number into the shared string
            sText.Format(const_cast<char*>("%d. "), i + 1);

            pLabel = &m_replyLabels[i];
            pText = &pLabel->m_text;
            pText->m_params.SetText(sText + pa[i]);
#else
            CExoString sFormat;
            sFormat.Format(const_cast<char*>("%d. "), i + 1);

            pLabel = &m_replyLabels[i];
            pText = &pLabel->m_text;
            pText->m_params.SetText(sFormat + pa[i]);
#endif
            pText->m_params.ReSetFont();

#ifdef K2_QOL_IMPROVEMENTS
            // No taller than LB_REPLIES can draw, less one line: the conversation trims the box
            // to whole lines afterwards (CSWGuiDialogCinematic::SetReplies).
            K2QolFitText(*pText, K2QolListRowLimit(m_listBox, pText->GetFontHeight()));
#endif
            extent = pLabel->m_extent;
            extent.h = pText->GetIdealHeight();
            pLabel->SetExtent(extent);

            pLabel->AddEvent(INPUTEVENT_GUI_A, this,
                             static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialog::HandleSelectReply));
            aControls.Add(pLabel);
        }
        m_aReplyIds[i] = pb[i];
    }

    if (d != 0)
    {
        m_listBox.AddControls(aControls, 0, 0, 1);
#ifdef K2_UI_TOUCH
        m_listBox.m_tail.bVisible = 0;
#else
        // PC and console let this list box draw the numbered replies; the touch build draws
        // them in its own overlay instead.
        m_listBox.m_tail.bVisible = 1;
#endif
        SetActiveControl(&m_listBox, 0);
        m_nSelectedReply = 0;
        m_listBox.SetSelectedControl(0, 0);
        // The last reply sits below the visible window.
        m_listBox.bShowScrollBar = (m_listBox.m_aControls.m_count - 1 > m_listBox.m_nBottomRow);
    }

    m_nMessageFlags &= ~DIALOGMSG_SHOWING_LINE;
    m_pGuiManager->UpdateMouseMove();
}

// Recomputes the panel's height every frame so it stays centred with equal margins.
void CSWGuiDialogTop::Draw(float f) {
#if KOTOR1  // K1 stretches the panel to the screen height every frame
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    int s_nScreenHeight = g_SDL_mode.h;
#else
    static int s_nScreenHeight = g_SDL_mode.h;
#endif
    CSWGuiObject::SetExtent(CSWGuiExtent(m_extent.x, m_extent.y, m_extent.w, s_nScreenHeight));
#elif defined(K2_UI_TOUCH)
    m_extent.h = m_pGuiManager->m_nScreenHeight - m_extent.y * 2;
#endif
    // PC and console use the panel's own draw; the per-frame stretch suits the full-height touch
    // plate and would defeat SetReply's sizing.
    CSWGuiPanel::Draw(f);
}

// Shows the current line of dialogue: sizes and places the IosLabel that displays it, hands the
// letterbox the full-width strip it has to cover, and hides the label when the string is blank.
// The blank test is a whitespace scan, so a message of pure whitespace counts as blank.
void CSWGuiDialogCinematic::ShowDialogMessage()
{
    m_nMessageFlags &= ~DIALOGMSG_LINE_PENDING;

    // The autopause gate passes when the option is on or a pending message was queued.
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_bFeedback2 || g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bForceDialogText)
    {
        m_msgLabel.m_text.m_params.SetText(m_sDialogMessage);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bForceDialogText = 0;
    }

    m_msgLabel.m_tail.bVisible = 1;

    // The spoken-line label grows to whatever the wrapped text needs, and the panel's extent
    // grows with it: the letterbox bars have to move out of the way.
    // The text is reached through a pointer so the height query stays virtual.
    CSWGuiExtent extent(m_msgLabel.m_extent);
    CSWGuiText* pMsgText = &m_msgLabel.m_text;
#ifdef K2_QOL_IMPROVEMENTS
    {
        // The line grows the panel upward; it may grow until the panel's top meets the safe margin.
        int nMarginX, nMarginY;
        m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);
        K2QolFitText(*pMsgText, extent.h + m_extent.y - nMarginY);
    }
#endif
#if KOTOR1
    // K1: the height is measured but the label keeps its extent and the panel does not grow.
    pMsgText->GetIdealHeight();
    m_msgLabel.SetExtent(extent);

    extent = m_extent;
    extent.x = 0;
    extent.w = m_pGuiManager->m_nScreenWidth;
    extent.h = m_pGuiManager->m_nScreenHeight - extent.y;
#ifndef K2_UI_TOUCH
    // The line draws in the message box here, so the bottom bar must cover the band.
    g_pAppManager->m_pClientExoApp->GetInGameGui()
        ->m_pBottomLetterbox->SetExtentSpecial(extent);
#endif
#else
    int nIdealHeight = pMsgText->GetIdealHeight();
    int nGrow = nIdealHeight - extent.h;
    if (nGrow >= 1)
    {
        m_extent.y -= nGrow;
        m_extent.h += nGrow;
        extent.h = nIdealHeight;
    }
    m_msgLabel.SetExtent(extent);

    // The strip the letterbox covers, reusing the same extent local.
    extent = m_extent;
    extent.x = 0;
    extent.w = m_pGuiManager->m_nScreenWidth;
    extent.h = m_pGuiManager->m_nScreenHeight - extent.y;
    g_pAppManager->m_pClientExoApp->GetInGameGui()
        ->m_pBottomLetterbox->SetExtentSpecial(extent);
#endif

#ifndef K2_UI_TOUCH
    // The PC and console builds stop here. Everything below is the touch overlay: it hides the
    // spoken-line label the layout just placed and draws the line in a plate across the top.
#else

    m_msgLabel.m_tail.bVisible = 0;

    if (IsAllWhitespace(m_sDialogMessage.CStr()))
    {
        m_iosLabel.m_tail.bVisible = 0;
        if (m_pGuiManager->PanelExists(&m_panel2))
            m_pGuiManager->RemovePanel(&m_panel2);
        if (m_pGuiManager->PanelExists(&m_panel1))
            m_pGuiManager->RemovePanel(&m_panel1);
        return;
    }

#if KOTOR1
    // K1: the plate is 0.71 of a 4:3-normalised screen width, 0.14 of its height, at y 580 in
    // 768-line units, centred on the SDL screen.
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    m_iosLabel.m_tail.bVisible = 1;
    m_iosLabel.SetString(m_sDialogMessage);

    float fAspect = static_cast<float>(g_SDL_mode.w) / static_cast<float>(g_SDL_mode.h);
    double dWidth = g_SDL_mode.h * 0.71 * fAspect;
    m_iosLabel.SetExtent(CSWGuiExtent(static_cast<int>(g_SDL_mode.w * 0.5 - dWidth * 0.5),
                                      static_cast<int>(s_fScale * 580.0f), static_cast<int>(dWidth),
                                      static_cast<int>(g_SDL_mode.h * 0.14)));
#else
    m_iosLabel.m_tail.bVisible = 1;
    m_iosLabel.SetString(m_sDialogMessage);

    // The letterbox Y is captured before the font size is set, and the width comes from extent.w.
    int nLetterboxY = g_pAppManager->m_pClientExoApp->GetInGameGui()
                          ->m_pBottomLetterbox->m_extent.y;
    float fScaleX = static_cast<float>(m_pGuiManager->m_nScreenWidth) / 1024.0f;
    float fScaleY = static_cast<float>(m_pGuiManager->m_nScreenHeight) / 768.0f;
    float fLabelWidth = fScaleX * 934.0f;
    float fLabelHeight = fScaleY * 92.0f;
    m_iosLabel.SetFontSize(static_cast<int>(fScaleY * 24.0f), fScaleX, fScaleY);

    m_iosLabel.m_extent.x = static_cast<int>((static_cast<float>(extent.w) - fLabelWidth) * 0.5f);
    m_iosLabel.m_extent.y = static_cast<int>(static_cast<float>(nLetterboxY) - fLabelHeight * 0.5f);
    m_iosLabel.m_extent.w = static_cast<int>(fLabelWidth);
    m_iosLabel.m_extent.h = static_cast<int>(fLabelHeight);
#endif

    m_panel1.AddControl(&m_iosLabel);

    if (!m_pGuiManager->PanelExists(&m_panel2))
        m_pGuiManager->AddPanel(&m_panel2, 0, 1);
    if (!m_pGuiManager->PanelExists(&m_panel1))
        m_pGuiManager->AddPanel(&m_panel1, 0, 1);
#endif
}

// Returns the screen to its resting state: re-extents the panel band, pushes the letterbox down,
// clears the spoken line and hides the label.
#if KOTOR1
// K1's band is 160 of 768 lines high and sits on the letterbox line, with no text-scalar clamp.
void CSWGuiDialogCinematic::Reset()
{
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    CSWGuiExtent extent;
    int nMarginX;
    int nMarginY;
    m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);
    IosGetSafeWidth(nMarginX, nMarginY);

    extent.x = nMarginX;
    extent.w = m_pGuiManager->m_nScreenWidth - nMarginX * 2;
    int nBandHeight = static_cast<int>(s_fScale * 160.0f);
    extent.y = m_pGuiManager->m_nScreenHeight
             - (m_pGuiManager->m_nScreenHeight
                - static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth) / 2.333333f)) / 2;
    extent.h = nBandHeight;
#ifndef K2_UI_TOUCH
    // Keep the whole band on screen; the bottom letterbox grows up to its top.
    if (extent.y > m_pGuiManager->m_nScreenHeight - nBandHeight)
        extent.y = m_pGuiManager->m_nScreenHeight - nBandHeight;
#endif

    int nExtentHeight = m_extent.h;
    SetExtent(extent);

    extent.x = 0;
    extent.w = m_pGuiManager->m_nScreenWidth;
    extent.h = m_pGuiManager->m_nScreenHeight - extent.y;
    g_pAppManager->m_pClientExoApp->GetInGameGui()
        ->m_pBottomLetterbox->SetExtent(extent);

    extent = m_msgLabel.m_extent;
    extent.h = extent.h + (nBandHeight - nExtentHeight);
    m_msgLabel.SetExtent(extent);

    m_msgLabel.m_text.m_params.SetText(CExoString(""));

    CSWGuiDialog::Reset();

    m_iosLabel.m_tail.bVisible = 0;
    if (m_pGuiManager->PanelExists(&m_panel1))
        m_pGuiManager->RemovePanel(&m_panel1);
}
#endif

// The reply list's rectangle in the 1024x768 iPad design grid.
__attribute__((visibility("hidden"))) int g_nReplyListX = 91;
__attribute__((visibility("hidden"))) int g_nReplyListY = 291;
__attribute__((visibility("hidden"))) int g_nReplyListW = 800;
__attribute__((visibility("hidden"))) int g_nReplyListH = 301;

extern int s_iosJoystickReplySelection;

// Handles a reply button click: the button's payload is its index, and the click goes to the
// inherited CSWGuiDialog::SelectReply after the list is reset.
void CSWGuiDialogComputer::HandleButtonChoice(CSWGuiControl* pControl)
{
    SetReplies(0, 0);
    SelectReply(static_cast<int>(pControl->m_tail.m_nUserData));
}

// The terminal conversation screen, with four readout label pairs, ten bar labels, a scrolling
// message box and a reply list, plus the sixteen touch reply buttons.
#if KOTOR1 && !defined(K2_UI_TOUCH)
// The phone panel art is Android data; the PC installs only have the layout's own backdrop.
static bool ComputerPhoneArtExists()
{
    static const bool s_bExists = g_pExoResMan->Exists(CResRef("ios_phpanel01"), 3, static_cast<unsigned int*>(0)) ||
                                  g_pExoResMan->Exists(CResRef("ios_phpanel01"), 3007, static_cast<unsigned int*>(0));
    return s_bExists;
}

// The side panels beside the 4:3 terminal (and_pan_left / and_pan_right) are Android art too; a
// missing texture draws white.
static bool ComputerSideArtExists()
{
    static const bool s_bExists = g_pExoResMan->Exists(CResRef("and_pan_left"), 3, static_cast<unsigned int*>(0)) ||
                                  g_pExoResMan->Exists(CResRef("and_pan_left"), 3007, static_cast<unsigned int*>(0));
    return s_bExists;
}
#endif

#if KOTOR1
// K1 uses the plain "computer" layout: three labels per readout, static plates instead of the ten
// bars, twenty reply labels, and the two side arrows sized off the screen height.
CSWGuiDialogComputer::CSWGuiDialogComputer(CSWGuiManager* pGuiManager)
    : CSWGuiDialog(pGuiManager)
{
    CExoString sName;

    m_vColorNewText = Vector(0.67f, 0.56f, 0.87f);

    m_pSoundSource = new CExoSoundSource(CResRef("computer_lp_04"));
    m_pSoundSource->SetLooping(1);
    m_pSoundSource->SetVolume(30);

    StartLoadFromLayout(CResRef("computer"), 1, 1, 0);
    // The derived class shadows the base's m_listBox: this one is the scrolling message box, the
    // base's is the reply list.
    InitControl(CSWGuiDialogComputer::m_listBox, CExoString("LB_MESSAGE"), 1, 1, 0, 0, 0);
    InitControl(CSWGuiDialog::m_listBox, CExoString("LB_REPLIES"), 1, 1, 0, 0, 0);
    InitControl(m_lblObscure, CExoString("LBL_OBSCURE"), 1, 1, 0, 0, 0);
    for (int i = 0; i < 4; i++)
    {
        sName.Format(const_cast<char*>("LBL_STATIC%d"), i + 1);
        InitControl(m_aStaticLabels[i], sName, 1, 1, 0, 0, 0);
        m_aStaticLabels[i].m_border.m_params.m_fAlpha = 0.4f;
    }
    InitControl(m_aInfo[0].m_labelCaption, CExoString("LBL_COMP_SKILL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[0].m_labelIcon, CExoString("LBL_COMP_SKILL_ICON"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[0].m_labelValue, CExoString("LBL_COMP_SKILL_VAL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[1].m_labelCaption, CExoString("LBL_REP_SKILL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[1].m_labelIcon, CExoString("LBL_REP_SKILL_ICON"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[1].m_labelValue, CExoString("LBL_REP_SKILL_VAL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[2].m_labelCaption, CExoString("LBL_COMP_SPIKES"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[2].m_labelIcon, CExoString("LBL_COMP_SPIKES_ICON"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[2].m_labelValue, CExoString("LBL_COMP_SPIKES_VAL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[3].m_labelCaption, CExoString("LBL_REP_UNITS"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[3].m_labelIcon, CExoString("LBL_REP_UNITS_ICON"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[3].m_labelValue, CExoString("LBL_REP_UNITS_VAL"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#ifndef K2_UI_TOUCH
    // The panel wears the phone art, whose screen is far wider than the PC layout's two lists:
    // the message takes the touch plate's span and the replies the reply frame's (1024 units).
    if (ComputerPhoneArtExists())
    {
        CSWGuiExtent listExtent(CSWGuiDialogComputer::m_listBox.m_extent);
        listExtent.x = 82;
        listExtent.w = 860;
        CSWGuiDialogComputer::m_listBox.SetExtent(listExtent);
        listExtent = CSWGuiDialog::m_listBox.m_extent;
        listExtent.x = 120;
        listExtent.w = 740;
        CSWGuiDialog::m_listBox.SetExtent(listExtent);
    }
#endif

    m_aInfo[0].m_labelCaption.m_text.m_params.SetStrRef(
        g_pRules->m_paSkills[0].m_nNameStrRef);
    m_aInfo[0].m_labelIcon.m_border.m_params.SetFillImage(
        CResRef(g_pRules->m_paSkills[0].m_icon), 0);
    m_aInfo[1].m_labelCaption.m_text.m_params.SetStrRef(
        g_pRules->m_paSkills[5].m_nNameStrRef);
    m_aInfo[1].m_labelIcon.m_border.m_params.SetFillImage(
        CResRef(g_pRules->m_paSkills[5].m_icon), 0);

    CSWGuiLabel* pProtoLabel = static_cast<CSWGuiLabel*>(CSWGuiDialog::m_listBox.m_pProtoItem);
    for (int i = 0; i < 20; i++)
    {
        CSWGuiExtent replyExtent(0, 0,
                                 CSWGuiDialog::m_listBox.m_innerExtent.w
                                     - CSWGuiDialog::m_listBox.m_nPadding * 2,
                                 20);
        m_replyLabels[i].Initialize(replyExtent, pProtoLabel, 1.0f);
        m_replyLabels[i].AddEvent(SWGUIEVENT_FOCUS, this,
                                  static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialog::SetReplyActive));
        m_replyLabels[i].AddEvent(SWGUIEVENT_UNFOCUS, this,
                                  static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialog::SetReplyInactive));
    }

    pProtoLabel = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    m_msgLabel.Initialize(pProtoLabel->m_extent, pProtoLabel, 1.0f);
    m_msgLabel.m_text.m_params.SetColor(m_vColorNewText);

    CSWGuiExtent extent(m_extent);
    for (int i = 0; i < 16; i++)
    {
        char szName[256];
        sprintf(szName, "ios_reply_%d", i + 1);

        extent = CSWGuiExtent(0x3ac, 0xa0 + i * 0x46,
                              g_nReplyButtonWidth, g_nReplyButtonHeight);
        m_aButtons[i].SetExtent(extent);
        m_aButtons[i].m_text.m_params.SetText(CExoString(""));
        m_aButtons[i].m_tail.m_nUserData = i;
        m_aButtons[i].m_border.m_params.SetFillImage(CResRef(szName), 0);
        m_aButtons[i].m_borderHilight.m_params.SetFillImage(CResRef(szName), 0);
        m_aButtons[i].m_tail.bVisible = 0;
        m_aButtons[i].m_border.m_params.SetPulsingAlpha(1, 1, 1);
        m_aButtons[i].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 1);
        AddControl(&m_aButtons[i]);
        m_aButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
                               static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialogComputer::HandleButtonChoice));
    }

    SetReplies(0, 0);

    m_replies.SetImages(0, const_cast<char*>("ipho_cscrollbar"), const_cast<char*>("ipho_cresup"),
                        const_cast<char*>("ipho_cresdown"));
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    m_replies.m_replyLayout = CSWGuiExtent(static_cast<int>(7.0f * s_fScale), static_cast<int>(5.0f * s_fScale),
                                       static_cast<int>(s_fScale * 10.0f), 1);
    ScaleExtentForResolution();
#ifndef K2_UI_TOUCH
    // The replies wrap, and take their row height, at the scaled reply list's width.
    for (int i = 0; i < 20; i++)
    {
        CSWGuiExtent replyExtent(m_replyLabels[i].m_extent);
        replyExtent.w = CSWGuiDialog::m_listBox.m_innerExtent.w - CSWGuiDialog::m_listBox.m_nPadding * 2;
        m_replyLabels[i].SetExtent(replyExtent);
    }
#endif

    int nArrowW = static_cast<int>(ceilf(static_cast<float>(g_SDL_mode.h) * 204.0f / 512.0f));
    extent = CSWGuiExtent(static_cast<int>((static_cast<float>(g_SDL_mode.w) - ceilf(static_cast<float>(g_SDL_mode.h) * 4.0f / 3.0f)) * 0.5f
                                - static_cast<float>(nArrowW)),
                          0, nArrowW, g_SDL_mode.h);
    m_imgPanLeft.Initialize(extent, CSWGuiImageParams(CResRef("and_pan_left"), 0, 0, 0.0f, SWGUIALIGN_MIDDLE | SWGUIALIGN_CENTER, 1.0f,
                                                      Vector(1.0f, 1.0f, 1.0f)));
    extent.x = g_SDL_mode.w - extent.x - extent.w;
    m_imgPanRight.Initialize(extent, CSWGuiImageParams(CResRef("and_pan_right"), 0, 0, 0.0f, SWGUIALIGN_MIDDLE | SWGUIALIGN_CENTER, 1.0f,
                                                       Vector(1.0f, 1.0f, 1.0f)));
}
#else
CSWGuiDialogComputer::CSWGuiDialogComputer(CSWGuiManager* pGuiManager)
    : CSWGuiDialog(pGuiManager)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
    CExoString sUnused;    // unused

    m_pSoundSource = new CExoSoundSource(CResRef("computer_lp_04"));
    m_pSoundSource->SetLooping(1);
    m_pSoundSource->SetVolume(30);

    StartLoadFromLayout(CResRef("computer_p"), 1, 1, 0);
    InitControl(CSWGuiDialogComputer::m_listBox, CExoString("LB_MESSAGE"), 1, 1, 0, 0, 0);
    InitControl(CSWGuiDialog::m_listBox, CExoString("LB_REPLIES"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[0].m_labelCaption, CExoString("LBL_COMP_SKILL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[0].m_labelValue, CExoString("LBL_COMP_SKILL_VAL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[1].m_labelCaption, CExoString("LBL_REP_SKILL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[1].m_labelValue, CExoString("LBL_REP_SKILL_VAL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[2].m_labelCaption, CExoString("LBL_COMP_SPIKES"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[2].m_labelValue, CExoString("LBL_COMP_SPIKES_VAL"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[3].m_labelCaption, CExoString("LBL_REP_UNITS"), 1, 1, 0, 0, 0);
    InitControl(m_aInfo[3].m_labelValue, CExoString("LBL_REP_UNITS_VAL"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[4], CExoString("LBL_BAR5"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[5], CExoString("LBL_BAR6"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[6], CExoString("LBL_BAR7"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[7], CExoString("LBL_BAR8"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[8], CExoString("LBL_BAR9"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[9], CExoString("LBL_BAR10"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    int i;
    m_aInfo[0].m_labelCaption.m_text.m_params.SetStrRef(
        g_pRules->m_paSkills[0].m_nNameStrRef);
    m_aInfo[1].m_labelCaption.m_text.m_params.SetStrRef(
        g_pRules->m_paSkills[5].m_nNameStrRef);

    // The reply labels are as wide as the reply list's inner rectangle and twenty pixels tall,
    // cloned off that list box's prototype row. Event 0 fires when a reply becomes active, 1 when
    // it goes inactive.
    CSWGuiLabel* pProtoLabel = static_cast<CSWGuiLabel*>(CSWGuiDialog::m_listBox.m_pProtoItem);
    for (i = 0; i < 30; i++)
    {
        m_replyLabels[i].Initialize(CSWGuiExtent(0, 0,
                                                 CSWGuiDialog::m_listBox.m_innerExtent.w
                                                     - CSWGuiDialog::m_listBox.m_nPadding * 2,
                                                 20),
                                    pProtoLabel, 1.0f);
        m_replyLabels[i].AddEvent(SWGUIEVENT_FOCUS, this,
                                  static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialog::SetReplyActive));
        m_replyLabels[i].AddEvent(SWGUIEVENT_UNFOCUS, this,
                                  static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialog::SetReplyInactive));
    }

    // The spoken-line label is cloned off the message box's prototype row and adopts that row's
    // live text colour.
    pProtoLabel = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    m_msgLabel.Initialize(pProtoLabel->m_extent, pProtoLabel, 1.0f);
    m_vColorNewText = pProtoLabel->m_text.m_params.m_color;
    m_msgLabel.m_text.m_params.SetColor(m_vColorNewText);
    m_vColorNewHighlight = CGuiInGame::COLOR_WHITE;

#ifdef K2_UI_TOUCH
    // The sixteen touch reply buttons, one 60x60 tile per reply.
    CSWGuiExtent extent(m_extent);
    for (i = 0; i < 16; i++)
    {
        char szName[256];
        sprintf(szName, "ios_reply_%d", i + 1);

        extent = CSWGuiExtent(0x3ac, 0xa0 + i * 0x46,
                              g_nReplyButtonWidth, g_nReplyButtonHeight);
        m_aButtons[i].SetExtent(extent);
        m_aButtons[i].m_text.m_params.SetText(CExoString(""));
        m_aButtons[i].m_tail.m_nUserData = i;
        m_aButtons[i].m_border.m_params.SetFillImage(CResRef(szName));
        m_aButtons[i].m_borderHilight.m_params.SetFillImage(CResRef(szName));
        m_aButtons[i].m_tail.bVisible = 0;
        m_aButtons[i].m_border.m_params.SetPulsingAlpha(1, 1, 1);
        m_aButtons[i].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 1);
        AddControl(&m_aButtons[i]);
        m_aButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
                               static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialogComputer::HandleButtonChoice));
    }

    SetReplies(0, 0);

    m_replies.SetImages(0, const_cast<char*>("ipho_cscrollbar"), const_cast<char*>("ipho_cresup"),
                        const_cast<char*>("ipho_cresdown"));
    m_replies.m_replyLayout = CSWGuiExtent(7, 9, 10, 1);
    m_replies.AddEvent(INPUTEVENT_GUI_A, this,
                       static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialogComputer::HandleButtonChoice));
#else
    // The PC and console composition stops here: the touch reply buttons, the widget skin and the
    // widget's own event belong to the touch overlay. Both are still by-value members, left
    // default-constructed and never shown.
#endif
}
#endif

// Rebuilds the reply wall from n strings and hands it to the touch reply widget: the strings are
// deep-copied into a local array, then every size is re-derived from the aspect multipliers.
#if KOTOR1
// K1 places the reply list on a 4:3-normalised SDL screen and hands the widget the list alone.
void CSWGuiDialogComputer::SetReplies(int n, CExoString* pa)
{
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif

    m_nNumReplies = n;
    CExoArrayList<CExoString> aReplies;
    for (int i = 0; i < n; i++)
        aReplies.Add(pa[i]);

    m_replies.m_tail.bVisible = 1;
    m_replies.SetReplies(aReplies);

    m_replies.SetExtent(CSWGuiExtent(static_cast<int>(s_fScale * 90.0f),
                                     static_cast<int>(g_SDL_mode.h * 0.5f + s_fScale * -320.0f * 0.5f),
                                     static_cast<int>(794.0f * s_fScale), static_cast<int>(290.0f * s_fScale)));
    AddControl(&m_replies);
}
#else
void CSWGuiDialogComputer::SetReplies(int n, CExoString* pa)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
    m_nNumReplies = n;
#ifndef K2_UI_TOUCH
    // The PC and console build has no two-argument overload: its replies live in the base's reply
    // list box, and everything below drives the touch reply widget.
    K2_UNUSED(pa);
#else

    CExoArrayList<CExoString> aReplies;
    for (int i = 0; i < n; i++)
        aReplies.Add(pa[i]);

    m_replies.m_tail.bVisible = 1;

    if (m_bCenterOnNativeRect)
        m_replies.SetReplies(aReplies,
                             static_cast<float>(g_nScaledNativeWidth / g_nScaledNativeHeight), 2);
    else
        m_replies.SetReplies(aReplies,
                             static_cast<float>(m_pGuiManager->m_nScreenWidth)
                                 / static_cast<float>(m_pGuiManager->m_nScreenHeight), 2);

    // A null array means no dialog is in flight; the in-game GUI is still poked here for its
    // side effect.
    if (pa != 0)
    {
        CGuiInGame* pInGame = g_pAppManager->m_pClientExoApp->GetInGameGui();
        K2_UNUSED(pInGame);
    }

    float fYMult = 1.0f;
    float fXMult = 1.0f;
    int nXOffset = 0;
    int nYOffset = 0;
    m_pGuiManager->GenerateAspectMultipliers(m_bCenterOnNativeRect, fXMult, fYMult,
                                             nXOffset, nYOffset, 1024.0f, 768.0f);

    m_replies.SetFontSize(static_cast<int>(fYMult * 24.0f), fXMult, fYMult);
    m_replies.m_nReplyHeight = static_cast<int>(fYMult * 72.0f);
    m_replies.m_extent = CSWGuiExtent(static_cast<int>(fXMult * g_nReplyListX),
                                      static_cast<int>(fYMult * g_nReplyListY),
                                      static_cast<int>(fXMult * g_nReplyListW),
                                      static_cast<int>(fYMult * g_nReplyListH));
    AddControl(&m_replies);
#endif
}
#endif

void CSWGuiDialogComputer::Draw(float f)
{
#if KOTOR1  // K1 draws the two side arrows first, in their own tinted layer
    AurGUIStartLayer();
#ifndef K2_UI_TOUCH
    // Without the side art the layer is a black fill alone.
    if (!ComputerSideArtExists())
    {
        if (AurGUISetupViewport(0, 0, g_SDL_mode.w, g_SDL_mode.h, Vector(0.0f, 0.0f, 0.0f), 0, 1.0f))
            AurGUICloseViewport();
    }
    else
#endif
    if (AurGUISetupViewport(0, 0, g_SDL_mode.w, g_SDL_mode.h, Vector(0.376f, 0.396f, 0.459f), 0, 1.0f))
    {
        m_imgPanLeft.Draw(f);
        m_imgPanRight.Draw(f);
        AurGUICloseViewport();
    }
    AurGUIStopLayer();
#endif
    CSWGuiPanel::Draw(f);
}

// Harvests the reply the touch reply widget confirmed this frame, hands it to the reply list and
// clears the request. The CSWGuiDialog:: qualifier is required: this class declares its own
// m_listBox, and the reply list is the base's.
void CSWGuiDialogComputer::Update(float f)
{
    K2_UNUSED(f);
#if KOTOR1  // K1 clears the reply wall and selects the confirmed reply
    int nReply = m_replies.GetConfirmedReplyAndReset();
    if (nReply != -1)
    {
        SetReplies(0, 0);
        SelectReply(nReply);
    }
#else
    m_replies.GetConfirmedReplyAndReset();
    if (s_iosJoystickReplySelection >= 0)
    {
        CSWGuiDialog::m_listBox.SetSelectedControl(s_iosJoystickReplySelection, 1);
        s_iosJoystickReplySelection = -1;
    }
#endif
}

// The reply list scrolls on two input codes; every event also reaches the touch reply widget.
void CSWGuiDialogComputer::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        if (type == INPUTEVENT_GUI_SCROLL_DOWN)
            m_listBox.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
        else if (type == INPUTEVENT_GUI_SCROLL_UP)
            m_listBox.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
    }
    m_replies.HandleInputEvent(type, val);
    CSWGuiDialog::HandleInputEvent(type, val);
}

// Handles the five-argument virtual: the two-argument overload lays the replies out, the base
// records their ids, and a non-zero d marks a live reply set, which shows the message label. The
// skill readouts are recomputed last.
void CSWGuiDialogComputer::SetReplies(int n, CExoString* pa, unsigned int* pb, int c, int d)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
#ifdef K2_UI_TOUCH
    SetReplies(n, pa);
#endif
#ifdef K2_FIX_ENGINE_BUGS
    int nUnused = c;
#else
    // Left uninitialised, as the base call does not read its fourth argument.
    K2_UNUSED(c);
    int nUnused;
#endif
    CSWGuiDialog::SetReplies(n, pa, pb, nUnused, d);
    if (d != 0)
        m_msgLabel.m_tail.bVisible |= 1;
    UpdateSkills();
}

// Releases the sound source and clears the reply list's rows.
CSWGuiDialogComputer::~CSWGuiDialogComputer()
{
    if (m_pSoundSource != 0)
    {
        delete m_pSoundSource;
        m_pSoundSource = 0;
    }
    m_listBox.ClearItems();
}

// Updates the terminal's four readouts with the party leader's Computer Use and Repair ranks, then
// the totals of the two consumable tags those skills work on, summed over the leader's own item
// repository.
//
// The repository's count is unsigned and both skill ranks narrow to char before formatting.
void CSWGuiDialogComputer::UpdateSkills()
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    CItemRepository* pRepo = pCreature->GetServerCreature()->GetItemRepository(1);

    int nRepUnits = 0;
    unsigned int i;
    int nCompSpikes = 0;
    for (i = 0; i < pRepo->ItemListGetCount(); i++)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem->m_sTag == "k_computer_spike")
            nCompSpikes += pItem->m_stackCount;
        else if (pItem->m_sTag == "k_repair_part")
            nRepUnits += pItem->m_stackCount;
    }

    m_aInfo[0].m_labelValue.m_text.m_params.SetText(
        CExoString(static_cast<char>(pCreature->m_pStats->GetSkillRank(SKILL_COMPUTER_USE))));
    m_aInfo[1].m_labelValue.m_text.m_params.SetText(
        CExoString(static_cast<char>(pCreature->m_pStats->GetSkillRank(SKILL_REPAIR))));
    m_aInfo[2].m_labelValue.m_text.m_params.SetText(CExoString(nCompSpikes));
    m_aInfo[3].m_labelValue.m_text.m_params.SetText(CExoString(nRepUnits));
}

// Takes a new line of terminal dialogue. The reply list is invalidated, the base records the
// message, and the spoken line is either sized and shown or hidden.
//
// The spoken line is centred inside the panel's own width.
void CSWGuiDialogComputer::SetDialogMessage(const CExoString& msg)
{
#ifdef K2_UI_TOUCH
    SetReplies(0, 0);
#endif
    UpdateSkills();

    CSWGuiDialog::SetDialogMessage(msg);

#ifdef K2_UI_TOUCH
    m_listBox.m_tail.bVisible = 0;

#if KOTOR1  // K1 uses the out-of-line IsEmpty test
    if (!msg.IsEmpty())
#else
    if (msg.m_sString != 0 && msg.m_sString[0] != 0)
#endif
    {
        m_iosLabel.SetFilterNewline(0);
        m_iosLabel.m_tail.bVisible = 1;
        m_iosLabel.m_bCenterText = 0;
        m_iosLabel.SetString(msg);
        m_iosLabel.SetImage(0);

#if KOTOR1
        // K1 uses an 860x92 plate at y 80, centred in the panel's width and scaled by the screen
        // height.
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
        float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
        static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
        m_iosLabel.SetExtent(CSWGuiExtent(static_cast<int>((static_cast<float>(m_extent.w) - 860.0f * s_fScale) * 0.5f),
                                          static_cast<int>(s_fScale * 80.0f), static_cast<int>(860.0f * s_fScale),
                                          static_cast<int>(92.0f * s_fScale)));
#else
        // Called for its side effect only.
        g_pAppManager->m_pClientExoApp->GetInGameGui();

        float fYMult = 1.0f;
        float fXMult = 1.0f;
        int nXOffset = 0;
        int nYOffset = 0;
        m_pGuiManager->GenerateAspectMultipliers(m_bCenterOnNativeRect, fXMult, fYMult,
                                                 nXOffset, nYOffset, 1024.0f, 768.0f);

        // Centred inside the panel's own width, as CSWGuiDialogCinematic::ShowDialogMessage does.
        m_iosLabel.m_extent.x = static_cast<int>((static_cast<float>(m_extent.w) - fXMult * 860.0f) * 0.5f
                                      + static_cast<float>(nXOffset));
        m_iosLabel.m_extent.y = static_cast<int>(fYMult * 80.0f + static_cast<float>(nYOffset));
        m_iosLabel.m_extent.w = static_cast<int>(fXMult * 860.0f);
        m_iosLabel.m_extent.h = static_cast<int>(fYMult * 220.0f);
        m_iosLabel.SetFontSize(static_cast<int>(fYMult * 24.0f), fXMult, fYMult);
        m_iosLabel.m_nMaxLines = 12;
#endif
        AddControl(&m_iosLabel);
    }
    else
    {
        m_iosLabel.m_tail.bVisible = 0;
    }
#else
    // The spoken line is one row of the scrolling message box, sized to its own wrapped height.
    m_listBox.m_tail.bVisible = m_msgLabel.m_tail.bVisible;
    if (m_msgLabel.m_tail.bVisible)
    {
        m_listBox.ClearItems();

        CSWGuiExtent extent(m_msgLabel.m_extent);
#ifdef K2_FIX_ENGINE_BUGS
        extent.w = m_listBox.m_innerExtent.w - 2 * m_listBox.m_nPadding;
        m_msgLabel.SetExtent(extent);
#endif
        CSWGuiText* pMsgText = &m_msgLabel.m_text;
#ifdef K2_QOL_IMPROVEMENTS
        K2QolFitText(*pMsgText, K2QolListRowLimit(m_listBox, 0));   // LB_MESSAGE's one row
#endif
        // A null or empty string is measured by font height; a non-empty one by its wrapped height.
        if (msg.m_sString != 0 && msg.m_sString[0] != 0)
            extent.h = pMsgText->GetIdealHeight();
        else
            extent.h = pMsgText->GetFontHeight();
        m_msgLabel.SetExtent(extent);

        CSWGuiControl* pRow = &m_msgLabel;
        m_listBox.AddControls(&pRow, 1, 0, 0, 0);
        m_listBox.SetSelectedControl(&m_msgLabel, 0);
    }
#endif
}

// Builds the terminal's full-screen backdrop name: the resolution, then the panel's own suffix,
// then the background index as a decimal string. Only three resolutions have a name of their own;
// anything else keeps the 800x600 seed.
#if KOTOR1
// K1 resolves the name through the manager's own table.
CExoString CSWGuiDialogComputer::GetFullScreenBG()
{
    CExoString sName = CSWGuiManager::GetScreenResolutionString(g_nScreenWidth, g_nScreenHeight);
    sName = sName + CSWGuiManager::FULLSCREEN_BGS[4] + CExoString(static_cast<int>(m_nFullScreenBGIndex));
    return sName;
}
#else
CExoString CSWGuiDialogComputer::GetFullScreenBG()
{
    CExoString sName("800x600");

    if (g_nScreenWidth == 1024 && g_nScreenHeight == 768)
        sName = "1024x768";
    else if (g_nScreenWidth == 1280 && (g_nScreenHeight == 1024 || g_nScreenHeight == 960))
        sName = "1280x960";
    else if (g_nScreenWidth == 1600 && g_nScreenHeight == 1200)
        sName = "1600x1200";

    sName = sName + CSWGuiManager::FULLSCREEN_BGS[4] + CExoString(static_cast<int>(m_nFullScreenBGIndex));

    return sName;
}
#endif

void CSWGuiDialogComputer::OnPanelAdded()
{
#if KOTOR1  // K1 re-skins the backdrop with the phone panel art first
#ifndef K2_UI_TOUCH
    if (ComputerPhoneArtExists())
#endif
    m_pBackground->m_params.SetFillImage(CResRef("ios_phpanel01"), 0);
#endif
    m_pSoundSource->Play();
    CSWGuiPanel::OnPanelAdded();
}

void CSWGuiDialogComputer::OnPanelRemoved()
{
    m_pSoundSource->Stop();
    CSWGuiPanel::OnPanelRemoved();
}

#if KOTOR1
// K1 picks the terminal backdrop from comptypes.2da and records the type as the background index.
void CSWGuiDialogComputer::SetType(unsigned char type)
{
    C2DA* p2DA = new C2DA(CResRef("comptypes"), 0);
    if (!p2DA->Load2DArray())
        return;
    if (type >= p2DA->m_nRows)
    {
        if (p2DA->m_nRows == 0)
            return;
        type = 0;
    }
    m_nFullScreenBGIndex = type;
    CExoString sBackground;
    p2DA->GetCExoStringEntry(type, CExoString("ComputerBackground"), &sBackground);
    SetBackground(CResRef(sBackground));
    delete p2DA;
}
#else
void CSWGuiDialogComputer::SetType(unsigned char type)
{
    K2_UNUSED(type);
}
#endif

// Defined by the GUI text layer.
float GetResolutionTextScalar();

// The letterbox bars that frame the conversation screen. m_state says which edge the bar is pinned
// to: 1 top, 2 bottom, 3 the full-screen middle.
CSWGuiDialogLetterbox::CSWGuiDialogLetterbox(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager),
      m_targetExtent(0, 0, 0, 0),
      m_baseExtent(0, 0, 0, 0)
{
#if KOTOR1  // K1 clears the state before the fade reset reads it
    m_state = 0;
    m_bScrollDone = 0;
    m_vColor = CGuiInGame::COLOR_BLACK;
    ResetFade();
#else
    m_vColor = CGuiInGame::COLOR_BLACK;
    ResetFade();
    m_state = 0;
    m_bScrollDone = 0;
#endif
}

// Re-anchors the bar at the bottom of the screen, keeping its present height.
void CSWGuiDialogLetterbox::Reset()
{
    CSWGuiExtent extent(m_extent);
    extent.y = m_pGuiManager->m_nScreenHeight - extent.h;
    SetExtent(extent);
}

void CSWGuiDialogLetterbox::ResetFade() {
    m_fAlpha = (m_state == 3) ? 0.0f : 1.0f;
    m_fadeElapsed = 0.0f;
    m_fadeDuration = 0.0f;
    m_fadeAmount = 0.0f;
    m_nFadeType = 0;
}

// Moves the bars to the rectangle the dialog screens computed. Each state pins one edge and
// publishes it to its own global, and only a bar that is mid-animation keeps the new rectangle as
// its target.
#if KOTOR1
// K1 has no minimum bar height and publishes no edge globals.
void CSWGuiDialogLetterbox::SetExtentSpecial(CSWGuiExtent extent)
{
    if (m_state == 1)
        extent.y = 0;
    else if (m_state == 2)
        extent.h = m_pGuiManager->m_nScreenHeight - extent.y;

    if (m_bScrollDone == 0)
    {
        if (m_state == 1)
        {
            if (extent.h > m_targetExtent.h)
                m_targetExtent.h = extent.h;
        }
        else if (m_state == 2)
        {
            if (m_targetExtent.y > extent.y)
            {
                int nDelta = m_targetExtent.y - extent.y;
                m_targetExtent.y = extent.y;
                m_targetExtent.h = m_targetExtent.h + nDelta;
            }
        }
    }
    else
    {
        m_targetExtent = extent;
        CSWGuiObject::SetExtent(extent);
    }
}
#else
void CSWGuiDialogLetterbox::SetExtentSpecial(CSWGuiExtent extent)
{
    if (m_state == 1)
    {
        float fMinimum = GetResolutionTextScalar() * 100.0f;
        if (fMinimum > static_cast<float>(extent.h))
            extent.h = static_cast<int>(fMinimum);
        extent.y = 0;
        g_Letterbox_top = extent.h;
    }
    else if (m_state == 2)
    {
        extent.h = m_pGuiManager->m_nScreenHeight - extent.y;
        g_Letterbox_bottom = extent.y;
    }

    // A bar that is mid-animation keeps its target and is only nudged by the block above; a
    // resting bar has the new rectangle installed outright.
    if (m_bScrollDone == 0)
    {
        if (m_state == 1)
        {
            if (m_targetExtent.h < extent.h)
                m_targetExtent.h = extent.h;
        }
        else if (m_state == 2)
        {
            if (m_targetExtent.y > extent.y)
            {
                int nDelta = m_targetExtent.y - extent.y;
                m_targetExtent.y = extent.y;
                m_targetExtent.h = m_targetExtent.h + nDelta;
            }
        }
    }
    else
    {
        m_targetExtent = extent;
        m_extent = extent;
    }
}
#endif

// Sets the extent with the state's own edge rule: the bottom bar re-derives its height from the
// screen bottom, the top bar pins its top edge to 0.
void CSWGuiDialogLetterbox::SetExtent(const CSWGuiExtent& extent)
{
    CSWGuiExtent copy(extent);
    if (m_state == 1)
        copy.y = 0;
    else if (m_state == 2)
        copy.h = m_pGuiManager->m_nScreenHeight - copy.y;
    CSWGuiPanel::SetExtent(copy);
}

// Drives the top bar: the rectangle collapses to zero height at the top of the screen while the
// animation target becomes the bar's full height.
#if KOTOR1
// K1 has no minimum bar height and no g_Letterbox_top.
void CSWGuiDialogLetterbox::SetTop()
{
    if (m_pGuiManager == 0)
        m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();

    int nScreenWidth = m_pGuiManager->m_nScreenWidth;
    int nBarHeight = static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenHeight
                                   - static_cast<int>(static_cast<float>(nScreenWidth) / 2.333333f)) * 0.5f);

    m_baseExtent.h = 0;
    m_targetExtent.x = 0;
    m_targetExtent.y = 0;
    m_targetExtent.w = nScreenWidth;
    m_targetExtent.h = nBarHeight;
    CSWGuiObject::SetExtent(CSWGuiExtent(0, 0, nScreenWidth, 0));
    m_state = 1;
}

// K1 leaves the bar at the screen bottom with no minimum height.
void CSWGuiDialogLetterbox::SetBottom()
{
    if (m_pGuiManager == 0)
        m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();

    int nScreenWidth = m_pGuiManager->m_nScreenWidth;
    int nScreenHeight = m_pGuiManager->m_nScreenHeight;
    int nBarHeight = static_cast<int>(static_cast<float>(nScreenHeight - static_cast<int>(static_cast<float>(nScreenWidth) / 2.333333f)) * 0.5f);

    m_baseExtent.y = nScreenHeight;
    m_targetExtent = CSWGuiExtent(0, nScreenHeight - nBarHeight, nScreenWidth, nBarHeight);
    CSWGuiObject::SetExtent(CSWGuiExtent(0, nScreenHeight, nScreenWidth, nBarHeight));
    m_state = 2;
}

// K1 installs the full-screen rectangle through CSWGuiObject::SetExtent.
void CSWGuiDialogLetterbox::SetMiddle()
{
    if (m_pGuiManager == 0)
        m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();

    m_fAlpha = 0.0f;
    m_fadeElapsed = 0.0f;
    m_fadeDuration = 0.0f;
    m_fadeAmount = 0.0f;
    m_nFadeType = 0;
    m_state = 3;
    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 0;
    extent.w = m_pGuiManager->m_nScreenWidth;
    extent.h = m_pGuiManager->m_nScreenHeight;
    CSWGuiObject::SetExtent(extent);
}
#else
void CSWGuiDialogLetterbox::SetTop()
{
    if (m_pGuiManager == 0)
        m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();

    int nScreenWidth = m_pGuiManager->m_nScreenWidth;
    int nBarHeight = static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenHeight
                                   - static_cast<int>(static_cast<float>(nScreenWidth) / 2.333333f)) * 0.5f);

    m_baseExtent.h = 0;
    m_targetExtent.x = 0;
    m_targetExtent.y = 0;
    m_targetExtent.w = nScreenWidth;
    m_targetExtent.h = nBarHeight;

    float fMinimum = GetResolutionTextScalar() * 100.0f;
    if (fMinimum > static_cast<float>(m_targetExtent.h))
        m_targetExtent.h = static_cast<int>(fMinimum);

    m_extent.x = 0;
    m_extent.y = 0;
    m_extent.w = m_pGuiManager->m_nScreenWidth;
    m_extent.h = 0;
    m_state = 1;
    g_Letterbox_top = 0;
}

// Mirrors SetTop for the bottom bar: its target y is the bar's top edge, pulled up so the bar never
// grows past the minimum height.
void CSWGuiDialogLetterbox::SetBottom()
{
    if (m_pGuiManager == 0)
        m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();

    int nBarHeight = static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenHeight
                                   - static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth) / 2.333333f))
                           * 0.5f);

    m_baseExtent.y = m_pGuiManager->m_nScreenHeight;
    m_targetExtent.y = m_baseExtent.y - nBarHeight;

    float fScaled = GetResolutionTextScalar() * 100.0f;
    float fLimit = static_cast<float>(m_pGuiManager->m_nScreenHeight) - fScaled;

    m_targetExtent.x = 0;
    m_targetExtent.y = static_cast<int>((fLimit > static_cast<float>(m_targetExtent.y)) ? static_cast<float>(m_targetExtent.y) : fLimit);

    m_extent.x = 0;
    m_extent.y = m_pGuiManager->m_nScreenHeight;
    m_targetExtent.w = m_pGuiManager->m_nScreenWidth;
    m_targetExtent.h = nBarHeight;
    m_extent.w = m_pGuiManager->m_nScreenWidth;
    m_extent.h = nBarHeight;
    g_Letterbox_bottom = m_pGuiManager->m_nScreenHeight;
    m_state = 2;
}

// Snaps the bar to the full-screen middle rest position and drops the fade.
void CSWGuiDialogLetterbox::SetMiddle()
{
    if (m_pGuiManager == 0)
        m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();

    m_fAlpha = 0.0f;
    m_fadeElapsed = 0.0f;
    m_fadeDuration = 0.0f;
    m_fadeAmount = 0.0f;
    m_nFadeType = 0;
    m_state = 3;

    m_extent.w = m_pGuiManager->m_nScreenWidth;
    m_extent.h = m_pGuiManager->m_nScreenHeight;
    m_extent.x = 0;
    m_extent.y = 0;
}
#endif

// Starts a fade: latches the colour, the direction, the two durations and the per-tick amount.
void CSWGuiDialogLetterbox::SetFade(int nFadeType, float fFadeLength, float fFadeDelay,
                                   const Vector& vFadeColor)
{
    m_vColor = vFadeColor;
    m_fAlpha = (nFadeType == 0) ? 0.0f : 1.0f;
    m_fadeElapsed = fFadeLength;
    m_fadeDuration = fFadeDelay;
    m_fadeAmount = 0.1f;
    m_nFadeType = nFadeType;
}

int CSWGuiDialogLetterbox::GetLetterboxChangeHeight(const float& f) {
    return static_cast<int>(f * (2.0f * static_cast<float>(m_targetExtent.h - m_baseExtent.h)));
}

int CSWGuiDialogLetterbox::GetLetterboxChangeTop(const float& f) {
    return static_cast<int>(f * (2.0f * static_cast<float>(m_targetExtent.y - m_baseExtent.y)));
}

int CSWGuiDialogLetterbox::IsFadeDone() {
    if (m_fadeAmount == 0.0f)
        return 1;
    return m_fadeAmount > m_fadeElapsed + m_fadeDuration;
}

// Advances the fade and recomputes the panel's alpha, walks a still-animating bar one step toward
// its target, then draws the panel base.
void CSWGuiDialogLetterbox::Draw(float f)
{
    if (m_state == 3 && m_fadeAmount > 0.0f)
    {
        // A frame longer than five seconds does not advance the fade at all.
        if (f < 5.0f)
            m_fadeAmount += f;

        if (m_fadeAmount > m_fadeElapsed)
        {
            float fAlpha = (m_fadeAmount - m_fadeElapsed) / m_fadeDuration;
            // fade-in (mode 0) ramps the alpha up toward 1, fade-out ramps 1-t down toward 0.
            if (m_nFadeType == 0)
                fAlpha = std::min(fAlpha, 1.0f);
            else
                fAlpha = std::max(1.0f - fAlpha, 0.0f);
            fAlpha = fAlpha < 1.0f ? fAlpha : 1.0f;
            fAlpha = fAlpha > 0.0f ? fAlpha : 0.0f;
            m_fAlpha = fAlpha;
        }
    }

#if KOTOR1
    // K1 steps the bar first and tests the stepped rectangle; an overshoot snaps to the target
    // and no edge globals are published.
    if (m_bScrollDone == 0)
    {
        if (f > 0.5f)
            f = 0.5f;

        if (m_state == 1)
        {
            CSWGuiExtent extent(m_targetExtent);
            extent.h = m_extent.h
                     + static_cast<int>(f * (2.0f * static_cast<float>(m_targetExtent.h - m_baseExtent.h)));
            if (m_extent.h > m_targetExtent.h)
            {
                m_bScrollDone = 1;
                extent = m_targetExtent;
            }
            SetExtent(extent);
        }
        else if (m_state == 2)
        {
            CSWGuiExtent extent(m_targetExtent);
            int nDelta = static_cast<int>(f * (2.0f * static_cast<float>(m_targetExtent.y - m_baseExtent.y)));
            extent.y = m_extent.y + nDelta;
            extent.h = m_extent.h - nDelta;
            if (extent.y < m_targetExtent.y)
            {
                m_bScrollDone = 1;
                g_pAppManager->m_pClientExoApp->GetInGameGui()->DialogGuiDoneScroll();
                extent = m_targetExtent;
            }
            SetExtent(extent);
        }
    }
#else
    if (m_bScrollDone == 0)
    {
        if (f > 0.5f)
            f = 0.5f;

        // state 1, the top bar: it grows to the target height and pins its top edge at 0.
        if (m_state == 1)
        {
            CSWGuiExtent extent(m_targetExtent);
            if (m_extent.h < m_targetExtent.h)
                extent.h = m_extent.h
                         + static_cast<int>(f * (2.0f * static_cast<float>(m_targetExtent.h - m_baseExtent.h)));
            else
                m_bScrollDone = 1;  // arrived: the next frame takes the plain path again
            SetExtent(extent);
            g_Letterbox_top = m_extent.h;
        }
        // state 2, the bottom bar: it grows upward off the screen bottom, so the top edge moves
        // by the same delta the height gains.
        else if (m_state == 2)
        {
            CSWGuiExtent extent(m_targetExtent);
            if (m_extent.y > m_targetExtent.y)
            {
                int nDelta = static_cast<int>(f * (2.0f * static_cast<float>(m_targetExtent.y - m_baseExtent.y)));
                extent.y = m_extent.y + nDelta;
                extent.h = m_extent.h - nDelta;
            }
            else
            {
                m_bScrollDone = 1;
                // The callback re-enters this object, so the target extent has to be re-read
                // after it rather than carried over from before the call.
                g_pAppManager->m_pClientExoApp->GetInGameGui()->DialogGuiDoneScroll();
                extent = m_targetExtent;
            }
            SetExtent(extent);
            g_Letterbox_bottom = m_extent.y;
        }
    }
#endif

    CSWGuiPanel::Draw(f);
}

extern bool  IosIsIphone(int& nScreenWidth, int& nScreenHeight);
float        GetResolutionTextScalar();

int s_iosJoystickReplySelection = -1;    // -1 means no reply is selected

// The cinematic conversation screen, with two full-screen panels, the two 3-pixel rules the
// letterbox bars push around, the reply list and spoken-line labels, and the reply panel this class
// owns.
#if KOTOR1 && !defined(K2_UI_TOUCH)
// The band the reply list and the spoken line draw in, kept on screen.
static CSWGuiExtent K1ReplyBandExtent(CSWGuiManager* pGuiManager, int nMarginX)
{
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    int nBand = static_cast<int>(s_fScale * 160.0f);
    int nTop = pGuiManager->m_nScreenHeight
             - (pGuiManager->m_nScreenHeight - static_cast<int>(static_cast<float>(pGuiManager->m_nScreenWidth) / 2.333333f)) / 2;
    if (nTop > pGuiManager->m_nScreenHeight - nBand)
        nTop = pGuiManager->m_nScreenHeight - nBand;
    return CSWGuiExtent(nMarginX, nTop, pGuiManager->m_nScreenWidth - nMarginX * 2, nBand);
}
#endif
#if KOTOR1
// K1 loads the "ydialog" layout with blue reply text, twenty reply labels and a fixed-height band.
CSWGuiDialogCinematic::CSWGuiDialogCinematic(CSWGuiManager* pGuiManager)
    : CSWGuiDialog(pGuiManager),
      m_panel1(pGuiManager),
      m_panel2(pGuiManager)
{
    m_vColorNewText = CGuiInGame::COLOR_BLUE;

    CSWGuiExtent screenExtent(0, 0, g_SDL_mode.w, g_SDL_mode.h);
    m_panel1.SetExtent(screenExtent);
    m_panel1.m_stateFlags |= 1;
    m_panel1.m_vColor = CGuiInGame::COLOR_TRANSPARENT;
    m_panel2.SetExtent(screenExtent);
    m_panel2.m_stateFlags |= 1;
    m_panel2.m_vColor = CGuiInGame::COLOR_TRANSPARENT;

    m_labelScrollRelated.Initialize(
        CSWGuiExtent(0, 600, screenExtent.w, 3),
        CSWGuiTextParams(),
        CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("ipho_letterline"), 2, 0, 0, 1.0f,
                           Vector(1.0f, 1.0f, 1.0f), 0, 0),
        1.0f);
    m_panel2.AddControl(&m_labelScrollRelated);

    m_labelTopBarRelated.Initialize(
        CSWGuiExtent(0, 160, screenExtent.w, 3),
        CSWGuiTextParams(),
        CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("ipho_letterline"), 2, 0, 0, 1.0f,
                           Vector(1.0f, 1.0f, 1.0f), 0, 0),
        1.0f);
    m_panel2.AddControl(&m_labelTopBarRelated);

#ifdef K2_UI_TOUCH
    StartLoadFromLayout(CResRef("ydialog"), 1, 1, 0);
#else
    // The PC lineage's conversation layout is "dialog"; "ydialog" ships only as the phone variant.
    StartLoadFromLayout(CResRef("dialog"), 1, 1, 0);
#endif
    InitControl(m_listBox, CExoString("LB_REPLIES"), 1, 0, 0, 0, 0);
    InitControl(m_msgLabel, CExoString("LBL_MESSAGE"), 1, 0, 0, 0, 0);
    StopLoadFromLayout();

    int nMarginX;
    int nMarginY;
    m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);
    IosGetSafeWidth(nMarginX, nMarginY);

    int nIphoneWidth;
    int nIphoneHeight;
    if (IosIsIphone(nIphoneWidth, nIphoneHeight))
    {
        CSWGuiExtent listExtent(m_listBox.m_extent);
        if (nIphoneWidth <= 1023)
            listExtent.h = 100;
        m_listBox.SetExtent(listExtent);
    }

    CSWGuiExtent extent(nMarginX,
                        (static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth) / 2.333333f)
                         + m_pGuiManager->m_nScreenHeight) / 2,
                        m_pGuiManager->m_nScreenWidth - nMarginX * 2,
                        160);
    SetExtent(extent);

    for (int i = 0; i < 20; i++)
    {
        CSWGuiExtent replyExtent(0, 0,
                                 m_listBox.m_innerExtent.w - m_listBox.m_nPadding * 2,
                                 20);
        m_replyLabels[i].Initialize(replyExtent, static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem), 1.0f);
        m_replyLabels[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialog::SetReplyActive));
        m_replyLabels[i].AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialog::SetReplyInactive));
    }

    m_msgLabel.m_text.m_params.SetColor(m_vColorNewText);

    CResRef resRef;
    resRef = m_msgLabel.m_text.m_params.m_szBaseFont;
    m_pDialogTop = new CSWGuiDialogTop(m_pGuiManager, resRef, m_vColorNewText, this);

    m_stateFlags |= 1;
    ScaleExtentForResolution();
#ifndef K2_UI_TOUCH
    // Re-places the band where Reset will put it, and gives the scroll bar its pixel width.
    SetExtent(K1ReplyBandExtent(m_pGuiManager, nMarginX));
#ifdef K1_UI_XBOX
    // The console conversation uses KOTOR II's slim bar, with no arrows, at its 16 pixels (the
    // layout's width, which KOTOR II's xbox conversation does not scale).
    m_listBox.m_scrollBar.bSlimBar = 1;
    m_listBox.m_scrollBar.m_extent.w = 16;
#else
    m_listBox.m_scrollBar.m_extent.w = static_cast<int>(16.0f * static_cast<float>(g_SDL_mode.h) / 768.0f);
#endif
    // The replies sit inside an 8-unit border inset, as in KOTOR II's dialog_p, and are as wide as
    // the list's final inner rectangle.
    m_listBox.m_border.m_params.m_nBorderDim = static_cast<int>(8.0f * static_cast<float>(g_SDL_mode.h) / 768.0f);
    m_listBox.SetExtent(m_listBox.m_extent);
    for (int i = 0; i < 20; i++)
    {
        CSWGuiExtent replyExtent(m_replyLabels[i].m_extent);
        replyExtent.w = m_listBox.m_innerExtent.w - m_listBox.m_nPadding * 2;
        m_replyLabels[i].SetExtent(replyExtent);
    }
#endif
}
#else
CSWGuiDialogCinematic::CSWGuiDialogCinematic(CSWGuiManager* pGuiManager)
    : CSWGuiDialog(pGuiManager),
      m_panel1(pGuiManager),
      m_panel2(pGuiManager)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
    int nMarginX;
    int nMarginY;

    m_vColorNewText = CGuiInGame::COLOR_NEW_TEXT;
    m_vColorNewHighlight = CGuiInGame::COLOR_NEW_HIGHLIGHT;

    // Both panels are the full screen; the inner one is the letterbox-safe box the labels live in.
    CSWGuiExtent screenExtent(0, 0, m_pGuiManager->m_nScreenWidth, m_pGuiManager->m_nScreenHeight);
    m_panel1.SetExtent(screenExtent);
    m_panel1.m_stateFlags |= 1;
    m_panel1.m_vColor = CGuiInGame::COLOR_TRANSPARENT;
    m_panel2.SetExtent(screenExtent);
    m_panel2.m_stateFlags |= 1;
    m_panel2.m_vColor = CGuiInGame::COLOR_TRANSPARENT;

    // The two 3-pixel rules, a full-width line at y=600 and another at y=160, both filled with the
    // shared "ipho_letterline" art.
    m_labelScrollRelated.Initialize(
        CSWGuiExtent(0, 600, screenExtent.w, 3),
        CSWGuiTextParams(),
        CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("ipho_letterline"), 2, 0, 0, 1.0f,
                           Vector(1.0f, 1.0f, 1.0f), 0, 0),
        1.0f);
    m_panel2.AddControl(&m_labelScrollRelated);

    m_labelTopBarRelated.Initialize(
        CSWGuiExtent(0, 160, screenExtent.w, 3),
        CSWGuiTextParams(),
        CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("ipho_letterline"), 2, 0, 0, 1.0f,
                           Vector(1.0f, 1.0f, 1.0f), 0, 0),
        1.0f);
    m_panel2.AddControl(&m_labelTopBarRelated);

    // InitControl binds the two named controls while the layout load is in flight.
#ifdef K2_UI_TOUCH
    StartLoadFromLayout(CResRef("dialog"), 1, 1, 0);
    InitControl(m_listBox, CExoString("LB_REPLIES"), 1, 0, 0, 0, 0);
    InitControl(m_msgLabel, CExoString("LBL_MESSAGE"), 1, 0, 0, 0, 0);
    StopLoadFromLayout();
#else
    // The PC conversation layout is "dialog_p", a 640x480 composition. The reference-size globals
    // are pinned at 800x600 for the mobile layouts, so the load and its two binds run inside a
    // 640x480 window.
    const int nSavedGUIWidth = g_nGUIWidth;
    const int nSavedGUIHeight = g_nGUIHeight;
    g_nGUIWidth = 640;
    g_nGUIHeight = 480;
    StartLoadFromLayout(CResRef("dialog_p"), 1, 1, 0);
    InitControl(m_listBox, CExoString("LB_REPLIES"), 1, 0, 0, 0, 0);
    InitControl(m_msgLabel, CExoString("LBL_MESSAGE"), 1, 0, 0, 0, 0);
    StopLoadFromLayout();
    g_nGUIWidth = nSavedGUIWidth;
    g_nGUIHeight = nSavedGUIHeight;
#endif

    m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);

    // On a sub-1024-wide screen the reply list keeps the port's 100-pixel height.
    int nIphoneWidth;
    int nIphoneHeight;
    if (IosIsIphone(nIphoneWidth, nIphoneHeight))
    {
        CSWGuiExtent listExtent(m_listBox.m_extent);
        if (nIphoneWidth <= 1023)
            listExtent.h = 100;
        m_listBox.SetExtent(listExtent);
    }

    // The reply list occupies a centred band, full width minus the two safe margins, with its top
    // at the midpoint of the letterboxed area (2.333333f is the port's 21:9 letterbox divisor,
    // shared with CSWGuiDialogTop).
    CSWGuiExtent extent;
    extent.x = nMarginX;
    extent.y = (static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth) / 2.333333f)
                + m_pGuiManager->m_nScreenHeight) / 2;
    extent.w = m_pGuiManager->m_nScreenWidth - nMarginX * 2;
    extent.h = 100;

    float fScale = GetResolutionTextScalar();
    float fMaxY = static_cast<float>(m_pGuiManager->m_nScreenHeight) - fScale * 100.0f;
    float fH = fScale * static_cast<float>(extent.h);
    extent.y = static_cast<int>(fMaxY > static_cast<float>(extent.y) ? static_cast<float>(extent.y) : fMaxY);
    extent.h = static_cast<int>(fH);

    // The list itself is scaled off its own current height rather than the band's.
    CSWGuiExtent listExtent(m_listBox.m_extent);
    listExtent.h = static_cast<int>(fScale * static_cast<float>(listExtent.h));
#if defined(K2_UI_XBOX) && !defined(K2_UI_TOUCH)
    // The console conversation keeps the phone's slim bar, with no arrows, at the layout's width.
    m_listBox.m_scrollBar.bSlimBar = 1;
#elif defined(K2_UI_PC)
    // pc: the PC's bar, arrows at both ends, as wide as the reply rows are scaled.
    m_listBox.m_scrollBar.m_extent.w = static_cast<int>(fScale * static_cast<float>(m_listBox.m_scrollBar.m_extent.w));
#endif
    m_listBox.SetExtent(listExtent);

    SetExtent(extent);

    // The reply labels are as wide as the list's inner rectangle and twenty scaled pixels tall,
    // cloned off the list's prototype row. Event 0 fires when a reply becomes active, 1 when it
    // goes inactive.
    CSWGuiLabel* pProtoLabel = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    for (int i = 0; i < 30; i++)
    {
        int nHeight = static_cast<int>(GetResolutionTextScalar() * 20.0f);
        m_replyLabels[i].Initialize(CSWGuiExtent(0, 0, m_listBox.m_innerExtent.w - m_listBox.m_nPadding * 2, nHeight),
                                    pProtoLabel, 1.0f);
        m_replyLabels[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialog::SetReplyActive));
        m_replyLabels[i].AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialog::SetReplyInactive));
    }

    m_msgLabel.m_text.m_params.SetColor(m_vColorNewText);

    // The reply panel takes this as its fourth argument, which is how a button click gets back to
    // SelectReply.
    CResRef resRef;
    resRef = m_msgLabel.m_text.m_params.m_szBaseFont;
    m_pDialogTop = new CSWGuiDialogTop(m_pGuiManager, resRef, m_vColorNewText, this);

    m_stateFlags |= 1;
}
#endif

// Frees the owned reply panel.
CSWGuiDialogCinematic::~CSWGuiDialogCinematic()
{
    if (m_pDialogTop)
    {
        delete m_pDialogTop;
        m_pDialogTop = 0;
    }
}

// Sets the panel's extent, then gives both children its new width and keeps their height.
void CSWGuiDialogCinematic::SetExtent(const CSWGuiExtent& extent)
{
    CSWGuiPanel::SetExtent(extent);

    // One extent local, reused for both children.
    CSWGuiExtent ext(m_listBox.m_extent);
    ext.w = m_extent.w;
    m_listBox.SetExtent(ext);

    ext = m_msgLabel.m_extent;
    ext.w = m_extent.w;
    m_msgLabel.SetExtent(ext);
}

// Rebuilds the reply list: hands the base its copy, mirrors it into the reply panel, disables the
// list while there is nothing to pick, snaps the list's height to whole text lines, and shows the
// panel if the base has not already.
void CSWGuiDialogCinematic::SetReplies(int n, CExoString* pa, unsigned int* pb, int c, int d)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
    if (d)
        m_pDialogTop->SetReply(m_sDialogMessage);

    CSWGuiDialog::SetReplies(n, pa, pb, c, d);
#ifdef K2_QOL_IMPROVEMENTS
    // The reply band holds at least five reply lines at the Text Size, growing down into the bar
    // below it and upward only by what the screen's bottom lacks; the list takes as many whole
    // lines as the band holds. This runs after the base SetReplies, whose Reset puts the band back
    // to its fixed height.
    const int nLine = static_cast<int>(static_cast<float>(m_replyLabels[0].m_text.GetFontHeight())
                            * static_cast<float>(k2qol_dialog_text_scale()) / 100.0f + 0.5f);
#if KOTOR1
    const int nListSlack = m_listBox.m_nPadding * 2 + m_listBox.m_border.m_params.m_nBorderDim * 2 + 1;
#else
    const int nListSlack = m_listBox.m_nPadding * 2 + 1;
#endif
    {
        const int nBand = 5 * nLine + nListSlack;
        const int nGrow = nBand - m_extent.h;
        if (nGrow > 0)
        {
            m_extent.h = nBand;
            // The spoken-line label grows with the band, as ShowDialogMessage's growth does: Reset
            // takes the band's change back out of it.
            CSWGuiExtent msgExtent(m_msgLabel.m_extent);
            msgExtent.h += nGrow;
            m_msgLabel.SetExtent(msgExtent);
            const int nTop = m_pGuiManager->m_nScreenHeight - nBand;
            if (m_extent.y > nTop)
            {
                m_extent.y = nTop;
                CSWGuiExtent strip(0, m_extent.y, m_pGuiManager->m_nScreenWidth,
                                   m_pGuiManager->m_nScreenHeight - m_extent.y);
                g_pAppManager->m_pClientExoApp->GetInGameGui()
                    ->m_pBottomLetterbox->SetExtentSpecial(strip);
            }
        }
        CSWGuiExtent listExtent(m_listBox.m_extent);
        listExtent.h = nLine > 0 ? (m_extent.h - nListSlack) / nLine * nLine + nListSlack : m_extent.h;
        m_listBox.SetExtent(listExtent);
        // The scroll bar, as the base set it for the list's old height.
        if (d != 0)
            m_listBox.bShowScrollBar = (m_listBox.m_aControls.m_count - 1 > m_listBox.m_nBottomRow);
    }
#endif
#ifdef K2_UI_TOUCH
    m_pDialogTop->SetReplies(n, pa);
    m_listBox.SetEnabled(0);
#endif
    // The touch build disables the list because its own widget draws the replies; on PC and
    // console the list stays enabled, since a disabled control cannot be selected.

#ifndef K2_QOL_IMPROVEMENTS
    if (m_listBox.m_tail.bVisible)
#else
    if (m_listBox.m_tail.bVisible && nLine <= 0)
#endif
    {
        CSWGuiExtent listExtent(m_listBox.m_extent);
        int nFontHeight = m_replyLabels[0].m_text.GetFontHeight();
        listExtent.h = listExtent.h / nFontHeight * nFontHeight;
        m_listBox.SetExtent(listExtent);
    }

    if (d && !m_pGuiManager->PanelExists(m_pDialogTop))
        m_pGuiManager->AddPanel(m_pDialogTop, 0, 1);
}

// Hands a picked reply to the achievement tracker and the in-game GUI, clears the spoken line and
// drops the reply panel. An index past the live reply count is ignored, but still succeeds.
int CSWGuiDialogCinematic::SelectReply(int n)
{
    if (m_nReplyCount > n)
    {
#if !KOTOR1  // K1 has no achievement tracker
        Achievements::OnDialogOptionChosen(n);
#endif
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetDialogSelection(n, m_nReplyCount, 1);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateDialog();
        m_pDialogTop->SetReply(CExoString(""));
        if (m_pGuiManager->PanelExists(m_pDialogTop))
            m_pGuiManager->RemovePanel(m_pDialogTop);
    }
    return 1;
}

#if !KOTOR1
void CSWGuiDialogCinematic::Reset()
{
    CSWGuiExtent extent;
    int nMarginX;
    int nMarginY;
    m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);

    extent.x = nMarginX;
    extent.w = m_pGuiManager->m_nScreenWidth - nMarginX * 2;

    int nLetterboxY = m_pGuiManager->m_nScreenHeight
                    - (m_pGuiManager->m_nScreenHeight
                       - static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth) / 2.333333f)) / 2;
    float fScale = GetResolutionTextScalar();
    float fMaxY = static_cast<float>(m_pGuiManager->m_nScreenHeight) - fScale * 100.0f;
    extent.h = static_cast<int>(fScale * 100.0f);
    extent.y = static_cast<int>(fMaxY > static_cast<float>(nLetterboxY) ? static_cast<float>(nLetterboxY) : fMaxY);

    int nExtentHeight = m_extent.h;
    SetExtent(extent);

    // The letterbox gets the full-width strip below the panel's top edge, in the same extent local.
    extent.x = 0;
    extent.w = m_pGuiManager->m_nScreenWidth;
    extent.h = m_pGuiManager->m_nScreenHeight - extent.y;
    g_pAppManager->m_pClientExoApp->GetInGameGui()
        ->m_pBottomLetterbox->SetExtent(extent);

    extent = m_msgLabel.m_extent;
    extent.h = extent.h + (static_cast<int>(fScale * 100.0f) - nExtentHeight);
    m_msgLabel.SetExtent(extent);

    m_msgLabel.m_text.m_params.SetText(CExoString(""));

    CSWGuiDialog::Reset();

    m_iosLabel.m_tail.bVisible = 0;
    if (m_pGuiManager->PanelExists(&m_panel1))
        m_pGuiManager->RemovePanel(&m_panel1);
}
#endif

// Removes the two sub-panels from the manager and clears everything they were showing.
void CSWGuiDialogCinematic::OnPanelRemoved()
{
    if (m_pGuiManager->PanelExists(&m_panel1))
        m_pGuiManager->RemovePanel(&m_panel1);
    if (m_pGuiManager->PanelExists(&m_panel2))
        m_pGuiManager->RemovePanel(&m_panel2);

    m_pDialogTop->SetReplies(0, 0);
    m_pDialogTop->SetReply(CExoString(""));

    CSWGuiPanel::OnPanelRemoved();
}

// Keeps the two 3-pixel rules glued to their letterbox bars, and drains the one-shot joystick reply
// selection an iOS build hands over through a global.
void CSWGuiDialogCinematic::Update(float f)
{
    K2_UNUSED(f);
    CSWGuiExtent extent(m_labelTopBarRelated.m_extent);
    extent.y = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pTopLetterbox->m_extent.h;
    m_labelTopBarRelated.SetExtent(extent);

    extent = m_labelScrollRelated.m_extent;
    extent.y = g_pAppManager->m_pClientExoApp->GetInGameGui()
                   ->m_pBottomLetterbox->m_extent.y;
    m_labelScrollRelated.SetExtent(extent);

#if !KOTOR1  // K1 has no joystick reply hand-over
    if (s_iosJoystickReplySelection >= 0)
    {
        m_listBox.SetSelectedControl(s_iosJoystickReplySelection, 1);
        s_iosJoystickReplySelection = -1;
    }
#endif
}

// Defined by the GUI text layer.
float GetResolutionTextScalar();

// The live-feed camera overlay, a full-screen camera view carrying the live-feed message label.
#if KOTOR1
// K1: the label sits a flat 75 pixels above the screen bottom and the panel is rescaled at the end.
CSWGuiDialogComputerCamera::CSWGuiDialogComputerCamera(CSWGuiManager* pGuiManager)
    : CSWGuiDialog(pGuiManager), m_pReserved1(0)
{
    StartLoadFromLayout(CResRef("computercamera"), 1, 1, 1);

#ifdef K2_FIX_ENGINE_BUGS
    // The panel and the label are placed in screen pixels, so after the layout is scaled, not
    // before it: scaled again, the caption ended below the screen on any screen over 768 lines.
    InitControl(m_liveFeedLabel, CExoString("LBL_RETURN"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    CSWGuiExtent extent(m_extent);
    extent.h = g_nScreenHeight;
    extent.w = g_nScreenWidth;
    SetExtent(extent);

    extent = m_liveFeedLabel.m_extent;
    extent.x = (g_nScreenWidth - extent.w) / 2;
    extent.y = g_nScreenHeight - static_cast<int>(75.0f * static_cast<float>(g_SDL_mode.h) / 768.0f);
    m_liveFeedLabel.SetExtent(extent);
#else
    // The panel is the screen; the height is assigned before the width.
    CSWGuiExtent extent(m_extent);
    extent.h = g_nScreenHeight;
    extent.w = g_nScreenWidth;
    SetExtent(extent);

    InitControl(m_liveFeedLabel, CExoString("LBL_RETURN"), 1, 1, 0, 0, 0);

    extent = m_liveFeedLabel.m_extent;
    extent.x = (g_nScreenWidth - extent.w) / 2;
    extent.y = g_nScreenHeight - 75;
    m_liveFeedLabel.SetExtent(extent);

    StopLoadFromLayout();
    ScaleExtentForResolution();
#endif
}
#else
CSWGuiDialogComputerCamera::CSWGuiDialogComputerCamera(CSWGuiManager* pGuiManager)
    : CSWGuiDialog(pGuiManager), m_pReserved1(0)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
    StartLoadFromLayout(CResRef("computercam_p"), 1, 1, 1);

    // The panel is the screen; the height is assigned before the width.
    CSWGuiExtent extent(m_extent);
    extent.h = g_nScreenHeight;
    extent.w = g_nScreenWidth;
    SetExtent(extent);

    InitControl(m_liveFeedLabel, CExoString("LBL_RETURN"), 1, 1, 0, 0, 0);

    extent = m_liveFeedLabel.m_extent;
    extent.x = (g_nScreenWidth - extent.w) / 2;
    float fScale = GetResolutionTextScalar();
    extent.y = static_cast<int>(static_cast<float>(g_nScreenHeight) - fScale * 75.0f);
    m_liveFeedLabel.SetExtent(extent);

    StopLoadFromLayout();
}
#endif

// Frees and clears the owned camera object. The null test is load-bearing: the re-zeroing store
// lives inside it.
CSWGuiDialogComputerCamera::~CSWGuiDialogComputerCamera()
{
    if (m_pReserved1 != 0)
    {
        delete m_pReserved1;
        m_pReserved1 = 0;
    }
}

// The CGuiInGame action toggles the live feed by setting this label's visible bit.
void CSWGuiDialogComputerCamera::SetLiveFeedMessageVisible(int visible)
{
    m_liveFeedLabel.m_tail.bVisible = visible;
}

// The reply button's size, read by the ctor's sizing loop and by SetReplies' re-layout loop.
__attribute__((visibility("hidden"))) int g_nReplyButtonWidth = 60;
__attribute__((visibility("hidden"))) int g_nReplyButtonHeight = 60;

int CSWGuiDialogTop::ms_iCurrentClickedChoice = -1;

// Builds the conversation screen, with the NPC's spoken line in a label, then the sixteen-button
// touch reply wall between a caption label and the scrolling reply list.
CSWGuiDialogTop::CSWGuiDialogTop(CSWGuiManager* pGuiManager, const CResRef& resRef,
                                 const Vector& vColor, CSWGuiDialogCinematic* pDialog)
    : CSWGuiPanel(pGuiManager)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
    m_pDialog = pDialog;

#if KOTOR1  // K1's panel is the whole screen
    CSWGuiObject::SetExtent(CSWGuiExtent(0, 0, g_SDL_mode.w, g_SDL_mode.h));
#else
    // GetSafeMargin's two out-parameters are the x/y letterbox the panel keeps on both sides.
    int nMarginX;
    int nMarginY;
    m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);
    m_extent.x = nMarginX;
    m_extent.y = nMarginY;
    m_extent.w = m_pGuiManager->m_nScreenWidth - nMarginX * 2;
    m_extent.h = (m_pGuiManager->m_nScreenHeight
                  - static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth) / 2.333333f)) / 2 - nMarginY - 5;
#endif

    // The label keeps the panel's width but its own (shorter) height.
    CSWGuiExtent labelExtent = m_label.m_extent;
    labelExtent.w = m_extent.w;
    m_label.SetExtent(labelExtent);

    m_vColor = CGuiInGame::COLOR_TRANSPARENT;

    m_label.Initialize(CSWGuiExtent(0, 0, m_extent.w, m_extent.h),
                       CSWGuiTextParams(CExoString(""), resRef, vColor, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f),
                       CSWGuiBorderParams(CResRef(), CResRef(), CResRef(), 0, 0, 0, 1.0f,
                                          Vector(1.0f, 1.0f, 1.0f), 0, 0),
                       1.0f);
    AddControl(&m_label);

#ifdef K2_UI_TOUCH
    // The sixteen reply buttons; each names its own art and reports its index through
    // m_tail.m_nUserData.
    for (int i = 0; i < 16; i++)
    {
        char sImage[256];
        CSWGuiExtent extent(940, 160 + i * 63, g_nReplyButtonWidth, g_nReplyButtonHeight);
        sprintf(sImage, "ios_reply_%d", i + 1);

        m_replyButtons[i].SetExtent(extent);
        m_replyButtons[i].m_text.m_params.SetText(CExoString(""));
        m_replyButtons[i].m_tail.m_nUserData = i;
        m_replyButtons[i].m_border.m_params.SetFillImage(CResRef(sImage));
        m_replyButtons[i].m_borderHilight.m_params.SetFillImage(CResRef(sImage));
        m_replyButtons[i].m_border.m_params.SetPulsingAlpha(1, 1, 1);
        m_replyButtons[i].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 1);
        AddControl(&m_replyButtons[i]);
        m_replyButtons[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialogTop::HandleButtonChoice));
    }

    SetReplies(0, 0);
#else
    // The PC and console composition stops here: the touch reply buttons and the reply widget
    // belong to the touch overlay, and both stay default-constructed by-value members.
#endif
    m_stateFlags |= 1;
}

// Handles a reply button click; the control's payload is the reply number.
void CSWGuiDialogTop::HandleButtonChoice(CSWGuiControl* pControl)
{
    m_pDialog->SelectReply(static_cast<int>(pControl->m_tail.m_nUserData));
}

// Rebuilds the reply list after a dialog page changes: the strings are deep-copied into a local
// array and handed to the touch reply widget, which re-lays-out its own rows, and every reply button
// is then re-positioned and re-hidden. The layout comes from two scale factors, the screen
// resolution normalised against the 1024x768 design grid.
void CSWGuiDialogTop::SetReplies(int nReplies, CExoString* pReplies)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
#ifndef K2_UI_TOUCH
    // PC and console never reach this body; CSWGuiDialogCinematic::OnPanelRemoved still calls it.
    K2_UNUSED(nReplies); K2_UNUSED(pReplies);
#else
    CSWGuiExtent extent = m_extent;

#if KOTOR1
    // K1 places the list under the spoken-line plate, with no images, font size or event.
    {
        CExoArrayList<CExoString> aReplies;
        for (int i = 0; i < nReplies; i++)
            aReplies.Add(pReplies[i]);

        m_replies.m_tail.bVisible = 1;
        m_replies.SetReplies(aReplies);

        m_replies.SetExtent(CSWGuiExtent(
            static_cast<int>(g_SDL_mode.w * 0.5
                             - g_SDL_mode.h * 0.71
                                   * (static_cast<float>(g_SDL_mode.w) / static_cast<float>(g_SDL_mode.h)) * 0.5),
            static_cast<int>(g_SDL_mode.h * 0.06 + g_SDL_mode.h * 0.14 + g_SDL_mode.h * 0.03),
            static_cast<int>(g_SDL_mode.h * 0.71
                             * (static_cast<float>(g_SDL_mode.w) / static_cast<float>(g_SDL_mode.h))),
            static_cast<int>(g_SDL_mode.h * 0.71)));
        AddControl(&m_replies);
    }

    m_nReserved1 = 0;

    float fOffset = static_cast<float>(m_pDialog->m_listBox.m_nTopRow) * 70.0f;
    for (int i = 0; i < 16; i++)
    {
        extent = CSWGuiExtent(0x3ac, static_cast<int>(static_cast<float>(0xa0 + i * 0x3f) - fOffset),
                              g_nReplyButtonWidth, g_nReplyButtonHeight);
        m_replyButtons[i].SetExtent(extent);
        m_replyButtons[i].ScaleExtentForResolution();
        m_replyButtons[i].m_tail.bVisible = 0;
    }
#else
    {
        CExoArrayList<CExoString> aReplies;
        for (int i = 0; i < nReplies; i++)
            aReplies.Add(pReplies[i]);

        // No reply array at all means "no dialog in flight", so the vertical origin collapses to 0
        // instead of the letterbox's own top edge.
        float fTop = 0.0f;
        if (pReplies != 0)
            fTop = static_cast<float>(g_pAppManager->m_pClientExoApp->GetInGameGui()
                             ->m_pBottomLetterbox->m_extent.y);

        float fW = static_cast<float>(m_pGuiManager->m_nScreenWidth) / 1024.0f;
        float fH = static_cast<float>(m_pGuiManager->m_nScreenHeight) / 768.0f;
        m_replies.m_tail.bVisible = 1;
        m_replies.SetReplies(aReplies,
                             static_cast<float>(m_pGuiManager->m_nScreenWidth)
                                 / static_cast<float>(m_pGuiManager->m_nScreenHeight),
                             0);

        m_replies.SetFontSize(static_cast<int>(fH * 24.0f), fW, fH);

        {
            float fListW = fW * 860.0f;
            m_replies.m_nReplyHeight = static_cast<int>(fH * 72.0f);
            float fListH = static_cast<float>(m_pGuiManager->m_nScreenHeight * 300) / 768.0f;
            m_replies.m_extent = CSWGuiExtent(static_cast<int>((extent.w - fListW) * 0.5f),
                                              static_cast<int>(fTop - fListH * 0.5f),
                                              static_cast<int>(fListW), static_cast<int>(fListH));
            m_replies.SetImages(const_cast<char*>("ipho_respbgipad"), const_cast<char*>("ipho_scrollbar"),
                                const_cast<char*>("ipho_resup"), const_cast<char*>("ipho_resdown"));
        }
        AddControl(&m_replies);
        m_replies.AddEvent(INPUTEVENT_GUI_A, this,
                           static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDialogTop::HandleButtonChoice));
    }

    m_nReserved1 = 0;

    // Re-stack the buttons clear of the list (the reply count can shrink the whole block) and hide
    // every one of them; SetReplies above re-shows the ones that have a reply on them.
    float fOffset = static_cast<float>(m_pDialog->m_listBox.m_nTopRow) * 70.0f;
    for (int i = 0; i < 16; i++)
    {
        extent = CSWGuiExtent(0x3ac, static_cast<int>(static_cast<float>(0xa0 + i * 0x3f) - fOffset),
                              g_nReplyButtonWidth, g_nReplyButtonHeight);
        m_replyButtons[i].SetExtent(extent);
        m_replyButtons[i].m_tail.bVisible = 0;
    }
#endif
#endif
}

// Polls the reply widget for a confirmed choice, which the widget then clears.
void CSWGuiDialogTop::Update(float f)
{
    K2_UNUSED(f);
#if KOTOR1  // K1 clears the reply wall and selects the confirmed reply
    int nReply = m_replies.GetConfirmedReplyAndReset();
    if (nReply != -1)
    {
        SetReplies(0, 0);
        m_pDialog->SelectReply(nReply);
    }
#else
    m_replies.GetConfirmedReplyAndReset();
#endif
}

// Gives the reply list first refusal on every input event, then the panel chain.
void CSWGuiDialogTop::HandleInputEvent(int type, int val)
{
    m_replies.HandleInputEvent(type, val);
    CSWGuiPanel::HandleInputEvent(type, val);
}

// Installs one line of spoken dialogue: sets the label's text, re-fits the whole panel around the
// wrapped height of that text and stretches the letterbox to match. The panel only ever grows, so
// a short reply does not shrink it back down.
void CSWGuiDialogTop::SetReply(const CExoString& sText)
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDialogText qolText;
#endif
    // No letterbox offset in flight means the dialog screen is not up; nothing to lay out.
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pTopLetterbox->m_bScrollDone == 0)
        return;

#if KOTOR1
    int nMarginY, nMarginX;
#else
    int nMarginX, nMarginY;
#endif
    m_pGuiManager->GetSafeMargin(nMarginX, nMarginY);

    // Addressed through a pointer so the text calls stay virtual.
    CSWGuiText* pText = &m_label.m_text;
    pText->m_params.SetText(sText);

#ifdef K2_QOL_IMPROVEMENTS
    // The top letterbox grows downward with it; never past the middle of the screen.
    K2QolFitText(*pText, m_pGuiManager->m_nScreenHeight / 2 - nMarginY);
#endif
    int nHeight = pText->GetIdealHeight();
    CSWGuiExtent extent = m_label.m_extent;
#if KOTOR1  // K1 sets the panel height through CSWGuiObject::SetExtent
    if (nHeight > m_extent.h)
    {
        CSWGuiObject::SetExtent(CSWGuiExtent(m_extent.x, m_extent.y, m_extent.w, nHeight));
    }
    else
    {
        CSWGuiExtent panelExtent(m_extent.x, m_extent.y, m_extent.w, 0);
        nHeight = (m_pGuiManager->m_nScreenHeight
                   - static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth) / 2.333333f)) / 2
                  - nMarginY - 5;
        panelExtent.h = nHeight;
        CSWGuiObject::SetExtent(panelExtent);
    }
#else
    if (nHeight <= m_extent.h)
        nHeight = (m_pGuiManager->m_nScreenHeight
                   - static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth) / 2.333333f)) / 2
                  - nMarginY - 5;
    m_extent.h = nHeight;
#endif
    extent.h = nHeight;
    m_label.SetExtent(extent);

    // The letterbox covers the full panel width plus the safe margin. The height is adjusted
    // before x and y are zeroed.
    extent = m_extent;
    extent.h += nMarginY;
    extent.x = 0;
    extent.y = 0;
    extent.w = m_pGuiManager->m_nScreenWidth;
#if !KOTOR1  // K1: the letterbox strip is computed but never handed to the top letterbox
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pTopLetterbox->SetExtentSpecial(extent);
#endif

#ifndef K2_UI_TOUCH
    // The PC and console bodies stop here; the touch overlay below draws the line in its own plate
    // instead of the label this panel exists to show.
#else

    m_label.m_tail.bVisible = 0;
#if KOTOR1
    // K1: the line plate is 0.71 of a 4:3-normalised screen wide and 0.14 of it high, at y 0.06.
    if (!sText.IsEmpty())
    {
        m_iosLabel.m_tail.bVisible = 1;
        m_iosLabel.SetString(sText);

        float fAspect = static_cast<float>(g_SDL_mode.w) / static_cast<float>(g_SDL_mode.h);
        double dWidth = g_SDL_mode.h * 0.71 * fAspect;
        m_iosLabel.SetExtent(CSWGuiExtent(static_cast<int>(g_SDL_mode.w * 0.5 - dWidth * 0.5),
                                          static_cast<int>(g_SDL_mode.h * 0.06), static_cast<int>(dWidth),
                                          static_cast<int>(g_SDL_mode.h * 0.14)));
        AddControl(&m_iosLabel);
    }
    else
    {
        m_iosLabel.m_tail.bVisible = 0;
    }
#else
    if (sText.m_sString != 0 && sText.m_sString[0] != 0)
    {
        m_iosLabel.m_tail.bVisible = 1;
        m_iosLabel.SetString(sText);

        float fW = static_cast<float>(m_pGuiManager->m_nScreenWidth) / 1024.0f;
        float fH = static_cast<float>(m_pGuiManager->m_nScreenHeight) / 768.0f;
        m_iosLabel.SetFontSize(static_cast<int>(fH * 24.0f), fW, fH);

        CSWGuiDialogLetterbox* pLetterbox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pTopLetterbox;

        m_iosLabel.m_extent.x = static_cast<int>((m_extent.w
            - static_cast<float>(m_pGuiManager->m_nScreenWidth * 934) / 1024.0f) * 0.5f);
        m_iosLabel.m_extent.y = static_cast<int>(static_cast<float>(pLetterbox->m_extent.y + pLetterbox->m_extent.h)
            - static_cast<float>(m_pGuiManager->m_nScreenHeight * 92) / 768.0f * 0.5f);
        m_iosLabel.m_extent.w = static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth * 934) / 1024.0f);
        m_iosLabel.m_extent.h = static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenHeight * 92) / 768.0f);
        AddControl(&m_iosLabel);
    }
    else
    {
        m_iosLabel.m_tail.bVisible = 0;
    }
#endif
#endif
}
