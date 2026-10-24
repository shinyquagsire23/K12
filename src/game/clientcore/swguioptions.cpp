#include "k2stdint.h"
#include "swguioptions.h"
#include "swguicontrol.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "swgui.h"
#include "twodimarray.h"
#include "exoresman.h"
#include "exoiniinternal.h"
#include "include/aurora.h"
#include "aurinternal.h"
#include "guiingame.h"
#include "swguiingame.h"
#include "exostring.h"
#include "CExoArrayListDtors.h"
#include "CExoArrayListImpl.h"
#include "swguiexamine.h"
#include "swguidrawable.h"
#include "aurorainterface.h"
#include "exosound.h"
#include "swguiingamegameplay.h"
#include "swguiingameautopause.h"
#include "PortDevMode.h"
#if KOTOR1 && !defined(K2_UI_TOUCH)
#include "port/k1gui.h"
#endif
#include "List.h"
#include "exoinput.h"
#include "strref.h"

int SWGUI_NUM_FEEDBACK_OPTIONS = 7;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "port/asllanguage.h"

void ReInitAurora(unsigned int nWidth, unsigned int nHeight, unsigned char nBpp,
                  int nFullscreen, int bAllow);

void AurDisableVertexBufferObjectsWriteOnly();
void AurEnableVertexBufferObjectsWriteOnly();
void ExpandButtonToFitText(CSWGuiButton& button);  // defined below
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif


int  AurQueryAntiAliasingMode(int nMode);
int  AurQueryMaximumAnisotropy();
#if KOTOR1
bool HasTouchScreen();
extern int preferPerfToVisual;
#endif
bool AurCheckFrameBufferModificationsAvailable();
bool AurCheckSoftShadowsAvailable();

extern bool g_allowForceSpeedEffect;
bool g_allowForceSpeedEffect = true;
#ifdef K2_QOL_ADVANCED_ROWS
extern bool g_bFrameRateLocked;
#if !KOTOR1
bool g_bFrameRateLocked;
#endif
#if !KOTOR1
static void K2QolSetForceSpeed(bool bOn);   // below, with CSWGuiOptionsGraphics::OnForceSpeed
#endif
// The ids SetDescription answers with the rows' own text (no TLK strref carries it).
#if !KOTOR1
static const uint64_t kQolDescForceSpeed = static_cast<uint64_t>(-110);
#endif
static const uint64_t kQolDescFramerate  = static_cast<uint64_t>(-111);
#if KOTOR1
// K1 has no override/custom.txt: KOTOR II's English for the same row.
static const char kK1FramerateCaption[] = "Framerate";
static const char kK1FramerateDescription[] =
    "ON: Framerate is locked to 60 Frames Per Second.\n\nOFF: Framerate is unlocked.";
#endif
#endif

CSWGuiOptionsGraphicsAdvanced::CSWGuiOptionsGraphicsAdvanced(CSWGuiManager* pGuiManager,
                                                            CSWGuiOptionsGraphics* pParent)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: layout "optgraphicsadv", K1 help strrefs, no BTN_DEFAULT width fix-up and no
    // button glyphs on the cycler captions.
    StartLoadFromLayout(CResRef("optgraphicsadv"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("optgraphicadv_p"), 1, 1, 1);
#endif
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_btnAntiAlias, CExoString("BTN_ANTIALIAS"), 1, 1, 0, 0, 0);
    InitControl(m_btnAntiAliasLeft, CExoString("BTN_ANTIALIASLEFT"), 1, 1, 0, 0, 0);
    InitControl(m_btnAntiAliasRight, CExoString("BTN_ANTIALIASRIGHT"), 1, 1, 0, 0, 0);
    InitControl(m_btnTexQual, CExoString("BTN_TEXQUAL"), 1, 1, 0, 0, 0);
    InitControl(m_btnTexQualLeft, CExoString("BTN_TEXQUALLEFT"), 1, 1, 0, 0, 0);
    InitControl(m_btnTexQualRight, CExoString("BTN_TEXQUALRIGHT"), 1, 1, 0, 0, 0);
    InitControl(m_cbFrameBuffer, CExoString("CB_FRAMEBUFF"), 1, 1, 0, 0, 0);
    InitControl(m_btnAnisotropy, CExoString("BTN_ANISOTROPY"), 1, 1, 0, 0, 0);
    InitControl(m_btnAnisotropyLeft, CExoString("BTN_ANISOTROPYLEFT"), 1, 1, 0, 0, 0);
    InitControl(m_btnAnisotropyRight, CExoString("BTN_ANISOTROPYRIGHT"), 1, 1, 0, 0, 0);
    InitControl(m_cbSoftShadows, CExoString("CB_SOFTSHADOWS"), 1, 1, 0, 0, 0);
    InitControl(m_cbVSync, CExoString("CB_VSYNC"), 1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aLabelBar[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
#endif

    // The help strrefs the layout resource does not carry; every button of a cycler group
    // shares one.
    m_btnAntiAlias.m_tail.m_nUserData = STRREF_DESC_ANTIALIAS;
    m_btnAntiAliasLeft.m_tail.m_nUserData = STRREF_DESC_ANTIALIAS;
    m_btnAntiAliasRight.m_tail.m_nUserData = STRREF_DESC_ANTIALIAS;
    m_btnTexQual.m_tail.m_nUserData = STRREF_DESC_TEXTURE_QUALITY;
    m_btnTexQualLeft.m_tail.m_nUserData = STRREF_DESC_TEXTURE_QUALITY;
    m_btnTexQualRight.m_tail.m_nUserData = STRREF_DESC_TEXTURE_QUALITY;
    m_cbFrameBuffer.m_tail.m_nUserData = STRREF_DESC_FRAME_BUFFER_EFFECTS;
    m_btnAnisotropy.m_tail.m_nUserData = STRREF_DESC_ANISOTROPY;
    m_btnAnisotropyLeft.m_tail.m_nUserData = STRREF_DESC_ANISOTROPY;
    m_btnAnisotropyRight.m_tail.m_nUserData = STRREF_DESC_ANISOTROPY;
    m_cbSoftShadows.m_tail.m_nUserData = STRREF_DESC_SOFT_SHADOWS;
    m_cbVSync.m_tail.m_nUserData = STRREF_DESC_VSYNC;
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;

    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnDefault, CExoString("BTN_DEFAULT"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
#if defined(K2_QOL_ADVANCED_ROWS) && KOTOR1
    K2QolAddRows();
#endif
    StopLoadFromLayout();
#if defined(K2_QOL_ADVANCED_ROWS) && !KOTOR1
    K2QolAddRows();
#endif

#if KOTOR1
    ScaleExtentForResolution();
#elif !defined(K2_UI_PC)   // pc: the layout's width holds the caption (no pad glyph beside it)
    // BTN_DEFAULT has to be wide enough for its own caption plus three row-heights.
    CSWGuiControl* pDefault = &m_btnDefault;
    CSWGuiExtent extentDefault = m_btnDefault.m_extent;
    int nIdealWidth = extentDefault.h * 3;
    CSWGuiExtent extentIdeal = m_btnDefault.m_text.GetIdealWidthAndHeight();
    nIdealWidth += extentIdeal.w;
    extentDefault.w = (extentDefault.w > nIdealWidth) ? extentDefault.w : nIdealWidth;
    pDefault->SetExtent(extentDefault);
#endif

    // The description listbox's single row is cloned from the listbox's prototype item.
    m_labelDesc.Initialize(m_listBoxDesc.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem), 1.0f);

#if !KOTOR1
    // Three cycler rows, each with the arrows' press pair and the keyboard pair.
    m_btnAntiAlias.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasLeft));
    m_btnAntiAlias.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasRight));
    m_btnTexQual.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualLeft));
    m_btnTexQual.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualRight));
    m_btnAnisotropy.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyLeft));
    m_btnAnisotropy.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyRight));

    m_btnAntiAlias.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasLeft));
    m_btnAntiAlias.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasRight));
    m_btnTexQual.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualLeft));
    m_btnTexQual.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualRight));
    m_btnAnisotropy.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyLeft));
    m_btnAnisotropy.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyRight));
#endif

#if KOTOR1 && !defined(K2_UI_TOUCH)
    // The pad's left / right on a cycler's caption steps it, as KOTOR II binds them; up / down
    // walk the rows top to bottom (the PC layout's move-to links repeat and drop rows).
    m_btnAntiAlias.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasLeft));
    m_btnAntiAlias.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasRight));
    m_btnTexQual.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualLeft));
    m_btnTexQual.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualRight));
    m_btnAnisotropy.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyLeft));
    m_btnAnisotropy.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyRight));

    m_btnAntiAlias.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasLeft));
    m_btnAntiAlias.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasRight));
    m_btnTexQual.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualLeft));
    m_btnTexQual.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualRight));
    m_btnAnisotropy.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyLeft));
    m_btnAnisotropy.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyRight));

#ifdef K2_QOL_ADVANCED_ROWS
    CSWGuiNavigable* apRows[7] = { &m_btnTexQual, &m_btnAntiAlias, &m_btnAnisotropy,
                                   &m_cbFrameBuffer, &m_cbSoftShadows, &m_cbVSync,
                                   &m_cbFramerateLock };
#else
    CSWGuiNavigable* apRows[6] = { &m_btnTexQual, &m_btnAntiAlias, &m_btnAnisotropy,
                                   &m_cbFrameBuffer, &m_cbSoftShadows, &m_cbVSync };
#endif
    const int nRows = sizeof(apRows) / sizeof(apRows[0]);
    for (int i = 0; i < nRows; i++) {
        apRows[i]->SetMoveToControl(0, apRows[(i + nRows - 1) % nRows]);
        apRows[i]->SetMoveToControl(2, apRows[(i + 1) % nRows]);
    }
#endif

    // Then the hover (event 0) help hook on every interactive row.
    m_btnAntiAlias.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_btnAntiAliasLeft.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_btnAntiAliasRight.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_btnTexQual.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_btnTexQualLeft.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_btnTexQualRight.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_cbFrameBuffer.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_btnAnisotropy.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_btnAnisotropyLeft.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_btnAnisotropyRight.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_cbSoftShadows.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_cbVSync.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));

    // Then the activate handler: the arrow acts, the row's caption button does not.
    m_btnAntiAliasLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasLeft));
    m_btnAntiAliasRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAntiAliasRight));
    m_btnTexQualLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualLeft));
    m_btnTexQualRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnTexQualRight));
    m_btnAnisotropyLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyLeft));
    m_btnAnisotropyRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnAnisotropyRight));
    m_cbFrameBuffer.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnFrameBuffer));
    m_cbSoftShadows.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnSoftShadows));
    m_cbVSync.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnVSync));

    SetUpOptions();

    // The three footer buttons. BTN_BACK takes the panel's own virtual B-button entry point.
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnDefault));
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnDiscardChanges));

#if !KOTOR1
    AspyrAddButtonIcon(&m_btnBack, 'a');
    AspyrAddButtonIcon(&m_btnCancel, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');
#endif

    SetActiveControl(&m_btnAntiAlias, 0);

    // When the renderer supports no AA mode at all the whole row is dead, so grey it out.
    if (!AurQueryAntiAliasingMode(2) && !AurQueryAntiAliasingMode(4)
        && !AurQueryAntiAliasingMode(6) && !AurQueryAntiAliasingMode(8)) {
        m_btnAntiAlias.SetEnabled(0);
        m_btnAntiAlias.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnAntiAliasLeft.SetEnabled(0);
        m_btnAntiAliasRight.SetEnabled(0);
    }
#if KOTOR1 && (!defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS))
    // Likewise the anisotropy row when the renderer cannot filter anisotropically.
    if (AurQueryMaximumAnisotropy() < 2) {
        m_btnAnisotropy.SetEnabled(0);
        m_btnAnisotropy.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnAnisotropyLeft.SetEnabled(0);
        m_btnAnisotropyRight.SetEnabled(0);
    }
#endif
#if KOTOR1 && !defined(K2_UI_TOUCH)
    SetActiveControl(&m_btnTexQual, 0);   // the focus starts on the top row
#endif

    m_bDiscardChanges = 0;
    m_bChanged = 0;
    m_pParent = pParent;
}

void CSWGuiOptionsGraphicsAdvanced::OnVSync(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    m_bVSync = (m_bVSync == 0);
}

void CSWGuiOptionsGraphicsAdvanced::OnEnterSlot(CSWGuiControl* pControl)
{
    SetDescription(pControl->m_tail.m_nUserData);
}

void CSWGuiOptionsGraphicsAdvanced::OnEnterDefault(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    SetDescription(STRREF_DESC_DEFAULT_BRIGHTNESS);
}

void CSWGuiOptionsGraphicsAdvanced::OnAntiAliasLeft(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (m_nAntiAlias < 1)
        return;

    bool bFound = false;
    if (m_nAntiAlias >= 8) {
        m_nAntiAlias = 6;
        bFound = AurQueryAntiAliasingMode(6) != 0;
    }
    if (!bFound && m_nAntiAlias >= 6) {
        m_nAntiAlias = 4;
        bFound = AurQueryAntiAliasingMode(4) != 0;
    }
    if (!bFound && m_nAntiAlias >= 4) {
        m_nAntiAlias = 2;
        bFound = AurQueryAntiAliasingMode(2) != 0;
    }
    if (!bFound)
        m_nAntiAlias = 0;

    SetAntiAliasText();
    m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
}

void CSWGuiOptionsGraphicsAdvanced::OnAntiAliasRight(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    int nMode = m_nAntiAlias;
    if (nMode == 8)
        return;

    bool bFound = false;
    if (nMode <= 0) {
        nMode = 2;
        bFound = AurQueryAntiAliasingMode(nMode) != 0;
    }
    if (!bFound && nMode <= 2) {
        nMode = 4;
        bFound = AurQueryAntiAliasingMode(nMode) != 0;
    }
    if (!bFound && nMode <= 4) {
        nMode = 6;
        bFound = AurQueryAntiAliasingMode(nMode) != 0;
    }
    if (!bFound && nMode <= 6) {
        nMode = 8;
        bFound = AurQueryAntiAliasingMode(nMode) != 0;
    }
    if (bFound)
        m_nAntiAlias = nMode;

    SetAntiAliasText();
    m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
}

void CSWGuiOptionsGraphicsAdvanced::OnTexQualLeft(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    int nNew = m_nTexQual - 1;
    if (nNew < 0)
        return;
    m_nTexQual = nNew;
    SetTexQualText();
    m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
}

void CSWGuiOptionsGraphicsAdvanced::OnTexQualRight(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
#if KOTOR1 && defined(K2_UI_TOUCH)
    // K1: the bound test is >, so K1 steps one past m_nTexQualMax. Only the pad reaches that
    // step (the arrow is hidden on the last row), and only the pc / xbox UIs bind the pad here.
    if (m_nTexQual > m_nTexQualMax)
        return;
#else
    if (m_nTexQual >= m_nTexQualMax)
        return;
#endif
    ++m_nTexQual;
    SetTexQualText();
    m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
}

void CSWGuiOptionsGraphicsAdvanced::OnAnisotropyLeft(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (m_nAnisotropy < 1)
        return;
    m_nAnisotropy >>= 1;
#if KOTOR1
    // K1: clamps to 1 and plays the click on every step.
    if (m_nAnisotropy < 1)
        m_nAnisotropy = 1;
    SetAnisotropyText();
    m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
#else
    if (m_nAnisotropy < 1) {
        m_nAnisotropy = 1;
        SetAnisotropyText();
    } else {
        SetAnisotropyText();
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
    }
#endif
}

void CSWGuiOptionsGraphicsAdvanced::OnAnisotropyRight(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (m_nAnisotropy >= AurQueryMaximumAnisotropy())
        return;
    m_nAnisotropy <<= 1;
    SetAnisotropyText();
    m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
}

void CSWGuiOptionsGraphicsAdvanced::OnFrameBuffer(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_bFrameBuffer = (m_bFrameBuffer == 0);
    m_bChanged = 1;
#if !KOTOR1 && !defined(K2_QOL_ADVANCED_ROWS)  // K1 has no force-speed-effect switch
    g_allowForceSpeedEffect = false;
#endif
}

void CSWGuiOptionsGraphicsAdvanced::OnSoftShadows(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        m_bSoftShadows = (m_bSoftShadows == 0);
        m_bChanged = 1;
    }
}

void CSWGuiOptionsGraphicsAdvanced::OnDefault(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultAdvancedGraphicsOptions();
        SetUpOptions();
#ifdef K2_QOL_ADVANCED_ROWS
        // Force Speed on (SetDefaultGraphicsOptions' value); the framerate unlocked, as KOTOR II
        // starts, and locked for K1, whose APK never went past 40 fps.
        m_bForceSpeed = 1;
#if KOTOR1
        m_bFramerateLock = 1;
#else
        m_bFramerateLock = 0;
        m_cbForceSpeed.bToggledOn = 1;
#endif
        m_cbFramerateLock.bToggledOn = m_bFramerateLock;
#endif
    }
}

void CSWGuiOptionsGraphicsAdvanced::OnDiscardChanges(CSWGuiControl* pControl)
{
    m_bDiscardChanges = 1;
    OnBButtonPressed(pControl);
}

void CSWGuiOptionsGraphicsAdvanced::SetUpOptions()
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions != 0) {
        unsigned int nFrameBuffer;
        if (AurCheckFrameBufferModificationsAvailable()) {
#if KOTOR1
            // K1: the checkbox mirrors the K1 global preferPerfToVisual, inverted.
            nFrameBuffer = (preferPerfToVisual == 0);
#else
            nFrameBuffer = pOptions->m_nFrameBuffer;
#endif
            m_cbFrameBuffer.bToggledOn = nFrameBuffer;
        } else {
            m_cbFrameBuffer.bToggledOn = 0;
            m_cbFrameBuffer.SetEnabled(0);
            nFrameBuffer = 0;
#if KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
            // SetNewOptions writes this back through SetFrameBuffer, which in K1 is the High
            // quality graphics switch (preferPerfToVisual): carry its current value.
            nFrameBuffer = (preferPerfToVisual == 0);
#endif
        }
        m_bFrameBuffer = nFrameBuffer;

        m_nAnisotropy = pOptions->m_nAnisotropy;

        unsigned int bSoftShadows;
        if (AurCheckSoftShadowsAvailable()) {
            bSoftShadows = pOptions->m_bSoftShadows;
            m_cbSoftShadows.bToggledOn = bSoftShadows;
        } else {
            m_cbSoftShadows.bToggledOn = 0;
            m_cbSoftShadows.SetEnabled(0);
            bSoftShadows = 0;
        }
        m_bSoftShadows = bSoftShadows;

        m_bVSync = pOptions->m_bVSync;
        m_cbVSync.bToggledOn = pOptions->m_bVSync;

        m_nAntiAlias = pOptions->m_nAntiAlias;
        m_nTexQual = pOptions->m_nTextureQuality;
    }
#ifdef K2_QOL_ADVANCED_ROWS
#if KOTOR1
    m_bForceSpeed = 1;
#else
    m_bForceSpeed = g_allowForceSpeedEffect;
    m_cbForceSpeed.bToggledOn = m_bForceSpeed;
#endif
    m_bFramerateLock = g_bFrameRateLocked;
    m_cbFramerateLock.bToggledOn = m_bFramerateLock;
#endif

    SetAntiAliasText();
    SetAnisotropyText();

    // The texture-quality cycler's right-hand clamp is the row count of the "texpacks" 2DA,
    // not a constant, so it is read once and cached in m_nTexQualMax.
    C2DA* pArray = new C2DA(CResRef("texpacks"), 0);
    if (pArray->Load2DArray()) {
        m_nTexQualMax = pArray->m_nRows - 1;
        pArray->Unload2DArray();
        delete pArray;
        SetTexQualText();
    } else {
        delete pArray;
    }
}

void CSWGuiOptionsGraphicsAdvanced::SetNewOptions()
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions != 0) {
        pOptions->SetAntiAlias(static_cast<unsigned char>(m_nAntiAlias));
        pOptions->SetTexQual(static_cast<unsigned char>(m_nTexQual));
        pOptions->SetFrameBuffer(m_bFrameBuffer);
        pOptions->SetAnisotropy(m_nAnisotropy);
        pOptions->SetVSync(m_bVSync);
        pOptions->SetSoftShadows(m_bSoftShadows);
        if (m_bSoftShadows)
            pOptions->SetShadows(1);
    }
#ifdef K2_QOL_ADVANCED_ROWS
#if !KOTOR1
    if ((m_bForceSpeed != 0) != g_allowForceSpeedEffect)
        K2QolSetForceSpeed(m_bForceSpeed != 0);
#endif
    g_bFrameRateLocked = m_bFramerateLock != 0;
#endif
}

void CSWGuiOptionsGraphicsAdvanced::SetTexQualText()
{
    C2DA* pArray = new C2DA(CResRef("texpacks"), 0);
    if (pArray->Load2DArray()) {
        if (pArray->m_nRows >= 1) {
            int nStrRef;
            if (pArray->GetINTEntry(m_nTexQual, CExoString("StrrefName"), &nStrRef))
                m_btnTexQual.m_text.m_params.SetStrRef(nStrRef);
        }

        m_btnTexQualLeft.m_tail.bVisible = 1;
        m_btnTexQualRight.m_tail.bVisible = 1;
        if (m_nTexQual == 0) {
            m_btnTexQualLeft.m_tail.bVisible = 0;
        } else if (m_nTexQual == 2) {
            m_btnTexQualRight.m_tail.bVisible = 0;
        }
        m_bChanged = 1;
        pArray->Unload2DArray();
    }
    delete pArray;
}

void CSWGuiOptionsGraphicsAdvanced::SetAntiAliasText()
{
#if KOTOR1
    // K1: its own dialog.tlk strrefs for the five captions.
    switch (m_nAntiAlias) {
    case 0:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_OFF);
        break;
    case 2:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_2X);
        break;
    case 4:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_4X);
        break;
    case 6:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_6X);
        break;
    case 8:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_8X);
        break;
    }
#else
    switch (m_nAntiAlias) {
    case 0:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_OFF);
        break;
    case 2:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_2X);
        break;
    case 4:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_4X);
        break;
    case 6:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_6X);
        break;
    case 8:
        m_btnAntiAlias.m_text.m_params.SetStrRef(STRREF_ANTIALIAS_8X);
        break;
    }
#endif

    m_btnAntiAliasLeft.m_tail.bVisible = 1;
    m_btnAntiAliasRight.m_tail.bVisible = 1;
    if (m_nAntiAlias == 0)
        m_btnAntiAliasLeft.m_tail.bVisible = 0;

    // The right arrow's ceiling is the highest mode the renderer reports; the 8 probe is the
    // hard cap, hence the extra == 8 disjunct.
    int nMode2 = AurQueryAntiAliasingMode(2);
    int nMode4 = AurQueryAntiAliasingMode(4);
    int nMode6 = AurQueryAntiAliasingMode(6);
    int nMode8 = AurQueryAntiAliasingMode(8);

    int nMax = nMode2 != 0 ? 2 : 0;
    if (nMode4 != 0)
        nMax = 4;
    if (nMode6 != 0)
        nMax = 6;
    if (nMode8 != 0)
        nMax = 8;

    if (m_nAntiAlias == 8 || m_nAntiAlias >= nMax)
        m_btnAntiAliasRight.m_tail.bVisible = 0;

    m_bChanged = 1;
}

void CSWGuiOptionsGraphicsAdvanced::SetAnisotropyText()
{
    // The five cases all end in the same SetStrRef call.
#if KOTOR1
    // K1: its own dialog.tlk strrefs for the same five captions.
    switch (m_nAnisotropy) {
    case 1:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_OFF);
        break;
    case 2:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_2X);
        break;
    case 4:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_4X);
        break;
    case 8:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_8X);
        break;
    case 16:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_16X);
        break;
    }
#else
    switch (m_nAnisotropy) {
    case 1:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_OFF);
        break;
    case 2:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_2X);
        break;
    case 4:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_4X);
        break;
    case 8:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_8X);
        break;
    case 16:
        m_btnAnisotropy.m_text.m_params.SetStrRef(STRREF_ANISOTROPY_16X);
        break;
    }
#endif

    m_btnAnisotropyLeft.m_tail.bVisible = 1;
    m_btnAnisotropyRight.m_tail.bVisible = 1;
    if (m_nAnisotropy < 2) {
        m_btnAnisotropyLeft.m_tail.bVisible = 0;
    } else if (m_nAnisotropy >= AurQueryMaximumAnisotropy() || m_nAnisotropy >= 16) {
        m_btnAnisotropyRight.m_tail.bVisible = 0;
    }
    m_bChanged = 1;
}

void CSWGuiOptionsGraphicsAdvanced::HandleInputEvent(int type, int val)
{
    if (val != 0) {
#if KOTOR1
        // K1: no restore-defaults arm, so the remaining cases are an if-chain (the value
        // handler written first).
        if (type == INPUTEVENT_GUI_START) {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        } else if (type == INPUTEVENT_GUI_B || type == INPUTEVENT_GUI_BACK) {
#else
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
#endif
            // A discard has nothing to apply; it drops the pending edits.
            if (m_bDiscardChanges) {
                m_bDiscardChanges = 0;
            } else {
                SetNewOptions();
                if (m_bChanged) {
                    CExoIni ini;
                    CExoString sValue;
                    int bFound;

                    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetAntiAlias(static_cast<unsigned char>(m_nAntiAlias));

                    // The mode change has to be re-applied before the card is asked what it
                    // can do, so the refresh-rate probe answers for the new mode.
                    ReInitAurora(g_nScreenWidth, g_nScreenHeight, g_nBitsPerPixel,
                                 g_pAurora->FullScreen(), 1);

#if KOTOR1
                    // K1: "swkotor.ini", read through CExoString::AsINT.
                    bFound = ini.ReadIniEntry(sValue, CExoString("swkotor.ini"),
                                             CExoString("Graphics Options"),
                                             CExoString("Disable Write-Only VBO"));
                    if (bFound) {
                        if (sValue.AsINT() == 1)
                            AurDisableVertexBufferObjectsWriteOnly();
                        else
                            AurEnableVertexBufferObjectsWriteOnly();
                    }
#else
                    bFound = ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                                             CExoString("Graphics Options"),
                                             CExoString("Disable Write-Only VBO"));
                    if (bFound) {
                        if (sValue.m_sString && atoi(sValue.m_sString) == 1)
                            AurDisableVertexBufferObjectsWriteOnly();
                        else
                            AurEnableVertexBufferObjectsWriteOnly();
                    }
#endif
                    m_bChanged = 0;
                }
            }

            m_bChanged = 0;

            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
            // Walk the whole sub-panel chain, not just this panel.
#if KOTOR1
            // K1: no panel chain; one stamp of state 4 (bits 8-10).
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
            CSWGuiPanel* pPanel = this;
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel);
#endif
            m_pParent->SetUpOptions();
#if KOTOR1
        }
#else
            break;
        }

        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
#ifdef K2_QOL_ADVANCED_ROWS
            OnDefault(&m_btnDefault);
#else
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultAdvancedGraphicsOptions();
            SetUpOptions();
#endif
            break;

        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;

        default:
            break;
        }
#endif
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiOptionsGraphicsAdvanced::SetDescription(uint64_t nStrRef)
{
#ifdef K2_QOL_ADVANCED_ROWS
    if (nStrRef == kQolDescFramerate)
#if KOTOR1
        m_labelDesc.m_text.m_params.SetText(CExoString(kK1FramerateDescription));
#else
        m_labelDesc.m_text.m_params.SetText(CExoString(GetLocalizedCustomString(8)));
    else if (nStrRef == kQolDescForceSpeed)
        m_labelDesc.m_text.m_params.SetText(CExoString(GetLocalizedCustomString(6)));
#endif
    else
#endif
    m_labelDesc.m_text.m_params.SetStrRef(nStrRef);

    CExoString sText(m_labelDesc.m_text.m_params.m_sText);
    m_listBoxDesc.ClearItems();

    CSWGuiText* pText = &m_labelDesc.m_text;

    CSWGuiExtent extent = m_labelDesc.m_extent;
#if KOTOR1
    if (sText.GetLength() >= 1)
#else
    if (sText.m_sString && static_cast<int>(strlen(sText.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_labelDesc.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_labelDesc };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_labelDesc, 0);
}

CSWGuiOptionsGraphicsAdvanced::~CSWGuiOptionsGraphicsAdvanced()
{
}

#ifdef K2_QOL_ADVANCED_ROWS
// The rows under V-Sync, one row pitch apart, cloned from it the way the Graphics page clones its
// own extra rows (K1: CB_GRASS -> High quality graphics before the layout is scaled; KOTOR II: the
// framerate lock from CB_FORCESPEED's layout extent, scaled by InitControl).
void CSWGuiOptionsGraphicsAdvanced::K2QolAddRows()
{
#if KOTOR1
    int nPitch = m_cbVSync.m_extent.y - m_cbSoftShadows.m_extent.y;
    m_cbFramerateLock = m_cbVSync;
    CSWGuiExtent extent = m_cbVSync.m_extent;
    extent.y += nPitch;
    m_cbFramerateLock.SetExtent(extent);
    m_cbFramerateLock.m_tail.m_nUserData = kQolDescFramerate;
    AddControl(&m_cbFramerateLock);
    m_cbFramerateLock.m_text.m_params.SetText(CExoString(kK1FramerateCaption));
#else
    CSWGuiOptionsCheckbox* apRows[] = { &m_cbForceSpeed, &m_cbFramerateLock };
    const char* apszCaptions[] = { GetLocalizedCustomString(5), GetLocalizedCustomString(7) };
    const uint64_t anDescriptions[] = { kQolDescForceSpeed, kQolDescFramerate };
    int nPitch = m_cbVSync.m_originalExtent.y - m_cbSoftShadows.m_originalExtent.y;
    for (int i = 0; i < 2; i++) {
        CSWGuiOptionsCheckbox& row = *apRows[i];
        row = m_cbVSync;
        row.m_pContainer = this;
        row.m_tail.nControlId = -1;   // appended, not swapped into CB_VSYNC's slot
        row.m_extent = m_cbVSync.m_originalExtent;
        row.m_extent.y += nPitch * (i + 1);
        InitControl(row, 1, 1, CExoString(static_cast<char*>(0)), 0);
#ifdef K2_QOL_IMPROVEMENTS
        K2QolMarkFitText(&row.m_text);   // built by hand, so CSWGuiButton::Load never marked it
#endif
        row.m_text.m_params.SetText(CExoString(apszCaptions[i]));
        row.m_tail.m_nUserData = anDescriptions[i];
    }
    m_cbForceSpeed.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_cbForceSpeed.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnForceSpeed));
#endif
    m_cbFramerateLock.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnEnterSlot));
    m_cbFramerateLock.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphicsAdvanced::OnFramerateLock));

#if !KOTOR1
    // Spliced into the pad's up / down chain under V-Sync (m_moveToControls: 0 up, 1 left,
    // 2 down, 3 right). K1's rows are added before the layout resolves its links, and the
    // constructor rebuilds its ring over every row.
    const int nRows = sizeof(apRows) / sizeof(apRows[0]);
    CSWGuiControl* pBelow = m_cbVSync.m_moveToControls[2];
    CSWGuiNavigable* pAbove = &m_cbVSync;
    for (int i = 0; i < nRows; i++) {
        pAbove->m_moveToControls[2] = apRows[i];
        apRows[i]->m_moveToControls[0] = pAbove;
        apRows[i]->m_moveToControls[1] = m_cbVSync.m_moveToControls[1];
        apRows[i]->m_moveToControls[3] = m_cbVSync.m_moveToControls[3];
        pAbove = apRows[i];
    }
    apRows[nRows - 1]->m_moveToControls[2] = pBelow;
    if (pBelow && pBelow->AsNavigable()) {
        CSWGuiNavigable* pNav = static_cast<CSWGuiNavigable*>(pBelow->AsNavigable());
        if (pNav->m_moveToControls[0] == &m_cbVSync)
            pNav->m_moveToControls[0] = pAbove;
    }
#endif
}

// The checkbox has already flipped (CSWGuiButtonToggle::HandleInputEvent); the pending value
// follows it and SetNewOptions applies it when the page closes, as for every row here.
void CSWGuiOptionsGraphicsAdvanced::OnForceSpeed(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        m_bForceSpeed = (m_bForceSpeed == 0);
}

void CSWGuiOptionsGraphicsAdvanced::OnFramerateLock(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        m_bFramerateLock = (m_bFramerateLock == 0);
}
#endif



extern int SWGUI_NUM_FEEDBACK_OPTIONS;
extern float GetResolutionTextScalar();

CSWGuiOptionsFeedback::~CSWGuiOptionsFeedback()
{
}

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

void CSWGuiOptionsFeedback::PopulateOptionsListbox()
{
#if KOTOR1
    // K1: rows are not rescaled; nine row kinds (row 6 hide-GUI and row 7 small fonts carry K1
    // strrefs, row 8 is tooltips); every row also gets the restore-defaults arm.
    CExoArrayList<CSWGuiControl*> list;
    CSWGuiButtonToggle* pProto = static_cast<CSWGuiButtonToggle*>(m_listBox.m_pProtoItem);

    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions == 0)
        return;

    for (int i = 0; i < SWGUI_NUM_FEEDBACK_OPTIONS; i++)
    {
        CSWGuiOptionsCheckbox* pCheckbox = new CSWGuiOptionsCheckbox();
#if !defined(K2_UI_TOUCH)
        // pc / xbox: the PC prototype's height is in the 480-line layout's units
        CSWGuiExtent extentRow = pProto->m_extent;
        extentRow.h = static_cast<int>(static_cast<float>(g_SDL_mode.h) / 480.0f * extentRow.h);
        pCheckbox->CSWGuiButtonToggle::Initialize(extentRow, pProto);
#else
        pCheckbox->CSWGuiButtonToggle::Initialize(pProto->m_extent, pProto);
#endif

        pCheckbox->AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnEnterSlot));

        switch (i)
        {
        case 0:
            pCheckbox->m_text.m_params.SetStrRef(STRREF_HIDE_UNEQUIPPABLE);
            pCheckbox->m_tail.m_nUserData = STRREF_DESC_HIDE_UNEQUIPPABLE;
            pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnHide));
            pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnHide));
            break;
        case 4:
            pCheckbox->m_text.m_params.SetStrRef(STRREF_FLOATING_NUMBERS);
            pCheckbox->m_tail.m_nUserData = STRREF_DESC_FLOATING_NUMBERS;
            pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnFloaty));
            pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnFloaty));
            break;
        case 2:
            pCheckbox->m_text.m_params.SetStrRef(STRREF_SUBTITLES);
            pCheckbox->m_tail.m_nUserData = STRREF_DESC_SUBTITLES;
            pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnSubtitles));
            pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnSubtitles));
            break;
        case 3:
            pCheckbox->m_text.m_params.SetStrRef(STRREF_MINI_MAP);
            pCheckbox->m_tail.m_nUserData = STRREF_DESC_MINI_MAP;
            pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnMiniMap));
            pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnMiniMap));
            break;
        case 7:
            pCheckbox->m_text.m_params.SetStrRef(STRREF_SMALL_FONT);
            pCheckbox->m_tail.m_nUserData = STRREF_DESC_SMALL_FONT;
            pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnUseSmallFont));
            pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnUseSmallFont));
            break;
        case 1:
            pCheckbox->m_text.m_params.SetStrRef(STRREF_TUTORIAL_POPUPS);
            pCheckbox->m_tail.m_nUserData = STRREF_DESC_TUTORIAL_POPUPS;
            pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnTutorial));
            pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnTutorial));
            break;
        case 5:
            pCheckbox->m_text.m_params.SetStrRef(STRREF_STATUS_SUMMARY);
            pCheckbox->m_tail.m_nUserData = STRREF_DESC_STATUS_SUMMARY;
            pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnSummary));
            pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnSummary));
            break;
        case 6:
            pCheckbox->m_text.m_params.SetStrRef(STRREF_HIDE_QUICK_MENU);
            pCheckbox->m_tail.m_nUserData = STRREF_DESC_HIDE_QUICK_MENU;
            pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnHideInGameGUI));
            pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnHideInGameGUI));
            break;
        case 8:
            pCheckbox->m_text.m_params.SetStrRef(STRREF_ENABLE_TOOLTIPS);
            pCheckbox->m_tail.m_nUserData = STRREF_DESC_TOOLTIPS;
            pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnEnableToolTips));
            pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnEnableToolTips));
            break;
        }
        pCheckbox->AddEvent(INPUTEVENT_GUI_Y, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnDefault));
        list.Add(pCheckbox);
    }

    m_listBox.ClearItems();
    m_listBox.AddControls(list, 1, 1, 0);
    m_listBox.bLooping = 0;
#else
    CExoArrayList<CSWGuiControl*> list;
    CSWGuiButtonToggle* pProto = static_cast<CSWGuiButtonToggle*>(m_listBox.m_pProtoItem);

    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions == 0)
        return;

    {
        for (int i = 0; i < SWGUI_NUM_FEEDBACK_OPTIONS; i++)
        {
            CSWGuiOptionsCheckbox* pCheckbox = new CSWGuiOptionsCheckbox();
            pCheckbox->CSWGuiButtonToggle::Initialize(pProto->m_extent, pProto);

            CSWGuiExtent extent = pCheckbox->m_extent;
            extent.h = static_cast<int>(extent.h * GetResolutionTextScalar());
            static_cast<CSWGuiControl*>(pCheckbox)->SetExtent(extent);

            pCheckbox->AddEvent(SWGUIEVENT_FOCUS, this,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnEnterSlot));

            switch (i)
            {
            case 0:
                pCheckbox->m_text.m_params.SetStrRef(STRREF_HIDE_UNEQUIPPABLE);
                pCheckbox->m_tail.m_nUserData = STRREF_DESC_HIDE_UNEQUIPPABLE;
                pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnHide));
                pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnHide));
                break;
            case 4:
                pCheckbox->m_text.m_params.SetStrRef(STRREF_FLOATING_NUMBERS);
                pCheckbox->m_tail.m_nUserData = STRREF_DESC_FLOATING_NUMBERS;
                pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnFloaty));
                pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnFloaty));
                break;
            case 2:
                pCheckbox->m_text.m_params.SetStrRef(STRREF_SUBTITLES);
                pCheckbox->m_tail.m_nUserData = STRREF_DESC_SUBTITLES;
                pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnSubtitles));
                pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnSubtitles));
                break;
            case 3:
                pCheckbox->m_text.m_params.SetStrRef(STRREF_MINI_MAP);
                pCheckbox->m_tail.m_nUserData = STRREF_DESC_MINI_MAP;
                pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnMiniMap));
                pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnMiniMap));
                break;
            case 7:
                pCheckbox->m_text.m_params.SetStrRef(STRREF_ENABLE_TOOLTIPS);
                pCheckbox->m_tail.m_nUserData = STRREF_DESC_TOOLTIPS;
                pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnEnableToolTips));
                pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnEnableToolTips));
                break;
            case 1:
                pCheckbox->m_text.m_params.SetStrRef(STRREF_TUTORIAL_POPUPS);
                pCheckbox->m_tail.m_nUserData = STRREF_DESC_TUTORIAL_POPUPS;
                pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnTutorial));
                pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnTutorial));
                break;
            case 5:
                pCheckbox->m_text.m_params.SetStrRef(STRREF_STATUS_SUMMARY);
                pCheckbox->m_tail.m_nUserData = STRREF_DESC_STATUS_SUMMARY;
                pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnSummary));
                pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnSummary));
                break;
            case 6:
                pCheckbox->m_text.m_params.SetStrRef(STRREF_HIDE_QUICK_MENU);
                pCheckbox->m_tail.m_nUserData = STRREF_DESC_HIDE_QUICK_MENU;
                pCheckbox->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnHideInGameGUI));
                pCheckbox->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnHideInGameGUI));
                break;
            }
            list.Add(pCheckbox);
        }
    }

    m_listBox.ClearItems();
    m_listBox.AddControls(list, 1, 1, 0);
    m_listBox.bLooping = 0;
#endif
}

CSWGuiOptionsFeedback::CSWGuiOptionsFeedback(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: layout "optfeedback", no ExpandButtonToFitText, iOS button glyphs last.
    StartLoadFromLayout(CResRef("optfeedback"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("optfeedback_p"), 1, 1, 1);
#endif

    InitControl(m_listBox, CExoString("LB_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnDefault, CExoString("BTN_DEFAULT"), 1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aLabelBar[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
#endif
    StopLoadFromLayout();

#if KOTOR1
    ScaleExtentForResolution();
#else
    // Size the default button's width to fit the longest caption: ideal text width + 3*its
    // height, clamped to never shrink below the laid-out width.
    ExpandButtonToFitText(m_btnDefault);
#endif

    PopulateOptionsListbox();

    // Seed the unregistered label from the description listbox's prototype row.
    CSWGuiControl* pProto = m_listBoxDesc.m_pProtoItem;
    m_labelExtent.Initialize(pProto->m_extent, static_cast<CSWGuiLabel*>(pProto), 1.0f);

    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnDefault));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnDefault));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsFeedback::OnBButtonPressed));

#if !KOTOR1
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');
#endif

    SetActiveControl(&m_listBox, false);

    if (SWGUI_NUM_FEEDBACK_OPTIONS >= 1)
        m_listBox.SetActiveControl(m_listBox.GetControl(0), false);

    SetUpOptions();
#if KOTOR1
    IosAddButtonIcon(&m_btnDefault, 'y');
    IosAddButtonIcon(&m_btnBack, 'b');
#endif
}

void CSWGuiOptionsFeedback::OnEnterSlot(CSWGuiControl* pControl)
{
    SetDescription(pControl->m_tail.m_nUserData);
}

void CSWGuiOptionsFeedback::HandleInputEvent(int type, int val)
{
    if (val != 0) {
#if !KOTOR1
        CSWGuiPanel* pPanel;
#endif
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
#if KOTOR1
            // K1: no panel chain; one stamp of state 4 (bits 8-10).
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
            pPanel = this;
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel);
#endif
            break;
        case INPUTEVENT_GUI_Y:
#if KOTOR1
            // K1: no sound, no BTN_DEFAULT arg; the arm returns without the base call.
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultFeedbackOptions();
            SetUpOptions();
            return;
#else
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultFeedbackOptions();
            SetUpOptions();
            break;
#endif
        case INPUTEVENT_GUI_START: {
            if (m_pActiveControl && m_pActiveControl->AsButtonToggle()) {
                m_pGuiManager->PlayGuiSound(GUISOUND_CHECKBOX_CHECK);
                CSWGuiOptionsCheckbox* pCheckA =
                    static_cast<CSWGuiOptionsCheckbox*>(m_pActiveControl->AsButtonToggle());
                CSWGuiOptionsCheckbox* pCheckB =
                    static_cast<CSWGuiOptionsCheckbox*>(m_pActiveControl->AsButtonToggle());
                pCheckA->bToggledOn = !pCheckB->bToggledOn;
            }
            break;
        }
        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;
        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        default:
            break;
        }
    }
    return CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiOptionsFeedback::SetUpOptions()
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions == 0)
        return;

    CSWGuiOptionsCheckbox* pRow;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(m_listBox.GetControl(0)->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bFeedback0;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(m_listBox.GetControl(1)->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bTutorialWindows;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(m_listBox.GetControl(2)->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bFeedback2;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(m_listBox.GetControl(3)->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bFeedback3;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(m_listBox.GetControl(4)->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bFeedback4;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(m_listBox.GetControl(5)->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bFeedback5;

#if KOTOR1
    // K1: the hide in-game GUI row exists only on touch screens; no tooltip row.
    if (HasTouchScreen()) {
        pRow = static_cast<CSWGuiOptionsCheckbox*>(m_listBox.GetControl(6)->AsButtonToggle());
        pRow->bToggledOn = pOptions->m_bHideInGameGui;
    }
#else
    if (SWGUI_NUM_FEEDBACK_OPTIONS >= 7) {
        pRow = static_cast<CSWGuiOptionsCheckbox*>(m_listBox.GetControl(6)->AsButtonToggle());
        pRow->bToggledOn = pOptions->m_bHideInGameGui;
    }
    if (SWGUI_NUM_FEEDBACK_OPTIONS >= 8) {
        pRow = static_cast<CSWGuiOptionsCheckbox*>(m_listBox.GetControl(7)->AsButtonToggle());
        pRow->bToggledOn = pOptions->m_bEnableToolTips;
    }
#endif
}

void CSWGuiOptionsFeedback::SetDescription(uint64_t nStrRef)
{
    m_labelExtent.m_text.m_params.SetStrRef(nStrRef);
    CExoString str = m_labelExtent.m_text.m_params.m_sText;
    m_listBoxDesc.ClearItems();
    CSWGuiText* pText = &m_labelExtent.m_text;
    CSWGuiExtent extent = m_labelExtent.m_extent;
#if KOTOR1
    if (str.GetLength() > 0)
#else
    if (str.m_sString && static_cast<int>(strlen(str.m_sString)) > 0)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = m_labelExtent.m_text.GetFontHeight();
    m_labelExtent.SetExtent(extent);
    CSWGuiControl* apRows[1] = { &m_labelExtent };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_labelExtent, 0);
}

void CSWGuiOptionsFeedback::OnDefault(CSWGuiControl* pControl)
{
#if KOTOR1
    K2_UNUSED(pControl);
    // K1: BTN_DEFAULT forwards to the panel's own restore-defaults input arm.
    HandleInputEvent(INPUTEVENT_GUI_Y, 1);
#else
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultFeedbackOptions();
        SetUpOptions();
    }
#endif
}

void CSWGuiOptionsFeedback::OnHide(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bFeedback0 = !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFeedback0;
    }
}

void CSWGuiOptionsFeedback::OnTutorial(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bTutorialWindows = !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bTutorialWindows;
    }
}

void CSWGuiOptionsFeedback::OnSubtitles(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bFeedback2 = !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFeedback2;
    }
}

void CSWGuiOptionsFeedback::OnMiniMap(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bFeedback3 = !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFeedback3;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->InitializeMap();
    }
}

void CSWGuiOptionsFeedback::OnFloaty(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bFeedback4 = !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFeedback4;
    }
}

void CSWGuiOptionsFeedback::OnSummary(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bFeedback5 = !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bFeedback5;
    }
}

void CSWGuiOptionsFeedback::OnHideInGameGUI(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        if (pOptions)
            pOptions->m_bHideInGameGui = !pOptions->m_bHideInGameGui;
    }
}

#if KOTOR1
void CSWGuiOptionsFeedback::OnUseSmallFont(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        if (pOptions)
            pOptions->SetUseSmallFonts(pOptions->m_bUseSmallFonts == 0);
    }
}

#endif
void CSWGuiOptionsFeedback::OnEnableToolTips(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        if (pOptions)
            pOptions->m_bEnableToolTips = !pOptions->m_bEnableToolTips;
    }
}

void CSWGuiOptionsFeedback::SetActiveButton(int nIndex)
{
    if (nIndex >= 0 && nIndex < SWGUI_NUM_FEEDBACK_OPTIONS)
        m_listBox.SetActiveControl(m_listBox.GetControl(nIndex), false);
}




void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);


extern bool g_allowForceSpeedEffect;
extern bool g_gameWantsSpeedBlur;
bool g_gameWantsSpeedBlur;
extern bool g_bFrameRateLocked;
#include "aurglcore.h"

void ReInitAurora(unsigned int nWidth, unsigned int nHeight, unsigned char nBpp,
                  int nFullscreen, int bAllow);

void AurDisableVertexBufferObjects();
void AurDisableVertexBufferObjectsWriteOnly();
void AurEnableVertexBufferObjectsWriteOnly();

static const unsigned int SWGUI_STRREF_CUSTOM_FORCESPEED    = static_cast<unsigned int>(-100);
#if !KOTOR1
static const unsigned int SWGUI_STRREF_CUSTOM_FRAMERATELOCK = static_cast<unsigned int>(-101);
#endif
#ifdef K2_QOL_FULLSCREEN
// KOTOR II asks the host for the window's mode; K1 records the setting the next launch opens its
// window with.
extern "C" int k2host_fullscreen(void);
#if KOTOR1
extern "C" void k2host_k1_apply_display(void);   // applies the window for the current settings
void K1QolResizeSceneBuffers();
static int s_bK1ResolutionInGame;                 // the Graphics page that opened the resolution list
static void K1QolApplyDisplay(CSWGuiManager* pGuiManager, int nPanels, int bFocusFullscreen);
// A game is loaded. K1 keeps a module object at the main menu too, but no player creature.
static int K1QolGameLoaded()
{
    return g_pAppManager->m_pClientExoApp->GetPlayerCreatureId() != OBJECT_INVALID;
}
#endif
static const uint64_t kQolDescFullscreen = static_cast<uint64_t>(-112);
static const char* K2QolFullscreenDescription(int bInGame);   // below, with OnFullscreen
#endif
// KOTOR II: Screen Resolution is on the Graphics page wherever the game runs in a window -- the PC
// UI and every desktop build's console UI; on a phone the display mode is the device's.
#if defined(K2_UI_PC) || (defined(K2_QOL_FULLSCREEN) && !defined(K2_UI_TOUCH))
#define K2_RESOLUTION_BUTTON 1
#endif


#if KOTOR1
CSWGuiOptionsGraphics::CSWGuiOptionsGraphics(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("optgraphics"), 1, 1, 1);
    InitControl(m_sliderGamma, CExoString("SLI_GAMMA"), 1, 1, 0, 0, 0);
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_labelGamma, CExoString("LBL_GAMMA"), 1, 1, 0, 0, 0);
    InitControl(m_btnResolution, CExoString("BTN_RESOLUTION"), 1, 1, 0, 0, 0);
    InitControl(m_btnResolutionLeft, CExoString("BTN_RESOLUTIONLEFT"), 1, 1, 0, 0, 0);
    InitControl(m_btnResolutionRight, CExoString("BTN_RESOLUTIONRIGHT"), 1, 1, 0, 0, 0);
    InitControl(m_cbShadows, CExoString("CB_SHADOWS"), 1, 1, 0, 0, 0);
    InitControl(m_cbGrass, CExoString("CB_GRASS"), 1, 1, 0, 0, 0);
    InitControl(m_btnAdvanced, CExoString("BTN_ADVANCED"), 1, 1, 0, 0, 0);

    m_sliderGamma.m_tail.m_nUserData = STRREF_DESC_BRIGHTNESS;
    m_labelGamma.m_tail.m_nUserData = STRREF_DESC_BRIGHTNESS;
    m_btnResolution.m_tail.m_nUserData = STRREF_DESC_RESOLUTION;
    m_btnResolutionLeft.m_tail.m_nUserData = STRREF_DESC_RESOLUTION;
    m_btnResolutionRight.m_tail.m_nUserData = STRREF_DESC_RESOLUTION;
    m_cbShadows.m_tail.m_nUserData = STRREF_DESC_SHADOWS;
    m_cbGrass.m_tail.m_nUserData = STRREF_DESC_GRASS;

    m_cbForceSpeed = m_cbGrass;
    CSWGuiExtent extent = m_cbGrass.m_extent;
    int nStep = extent.y - m_cbShadows.m_extent.y;
    extent.y += nStep;
    m_cbForceSpeed = m_cbGrass;
    m_cbForceSpeed.SetExtent(extent);
    m_cbForceSpeed.m_tail.m_nUserData = STRREF_DESC_RESOLUTION;
    AddControl(&m_cbForceSpeed);

    m_cbFramerateLock = m_cbForceSpeed;
    m_cbFramerateLock.m_tail.m_nUserData = STRREF_DESC_FRAME_BUFFER_EFFECTS;
    extent = m_cbFramerateLock.m_extent;
    extent.y += nStep;
    m_cbFramerateLock.SetExtent(extent);
    m_cbFramerateLock.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbFramerateLock.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnFrameBuffer));
    AddControl(&m_cbFramerateLock);
    m_cbFramerateLock.m_text.m_params.SetStrRef(STRREF_FRAME_BUFFER_EFFECTS);

    switch (ASLPlat_GetCurrentLanguage())
    {
    case ASL_LANGUAGE_FRENCH:
        m_cbForceSpeed.m_text.m_params.SetText(CExoString("Graphismes qualit\xe9 sup\xe9rieure"));
        break;
    case ASL_LANGUAGE_ITALIAN:
        m_cbForceSpeed.m_text.m_params.SetText(CExoString("Grafica di alta qualit\xe0"));
        break;
    case ASL_LANGUAGE_GERMAN:
        m_cbForceSpeed.m_text.m_params.SetText(CExoString("Hohe Graphikqualit\xe4t"));
        break;
    case ASL_LANGUAGE_SPANISH:
        m_cbForceSpeed.m_text.m_params.SetText(CExoString("Gr\xe1" "ficos de alta calidad"));
        break;
    default:
        m_cbForceSpeed.m_text.m_params.SetText(CExoString("High quality graphics"));
        break;
    }
#ifdef K2_QOL_FULLSCREEN
    K2QolAddFullscreenRow();
#endif

    m_btnAdvanced.m_tail.m_nUserData = STRREF_DESC_ADVANCED_GRAPHICS;
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;

    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnDefault, CExoString("BTN_DEFAULT"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_labelDesc.Initialize(m_listBoxDesc.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem), 1.0f);

    m_sliderGamma.SetCurValue(57);
    m_sliderGamma.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlider));
    m_sliderGamma.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnExitSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnDefault));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));

    m_btnResolution.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbShadows.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbGrass.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbForceSpeed.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));

    m_btnResolution.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnResolution));
    m_cbShadows.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnShadows));
    m_cbGrass.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnGrass));
    m_cbForceSpeed.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnHighResolution));

    SetUpOptions();

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnDefault));
    m_btnAdvanced.m_tail.bFocusOnHover = 0;
    m_btnAdvanced.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnAdvanced));

    m_bResolutionChanged = 0;

    IosAddButtonIcon(&m_btnDefault, 'y');
    IosAddButtonIcon(&m_btnBack, 'b');

    m_sliderGamma.SetMoveToControl(2, &m_cbShadows);
    m_sliderGamma.SetMoveToControl(0, &m_cbFramerateLock);
    m_cbShadows.SetMoveToControl(2, &m_cbGrass);
    m_cbShadows.SetMoveToControl(0, &m_sliderGamma);
    m_cbGrass.SetMoveToControl(2, &m_cbForceSpeed);
    m_cbGrass.SetMoveToControl(0, &m_cbShadows);
    m_cbForceSpeed.SetMoveToControl(2, &m_cbFramerateLock);
    m_cbForceSpeed.SetMoveToControl(0, &m_cbGrass);
    m_cbFramerateLock.SetMoveToControl(2, &m_sliderGamma);
    m_cbFramerateLock.SetMoveToControl(0, &m_cbForceSpeed);
#ifndef K2_UI_TOUCH
    // The PC layout shows Screen Resolution under the slider and Advanced Options at the bottom:
    // both join the pad's up / down ring.
    m_btnAdvanced.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbForceSpeed.m_tail.m_nUserData = SWGUI_STRREF_CUSTOM_FORCESPEED;
    m_sliderGamma.SetMoveToControl(2, &m_btnResolution);
    m_btnResolution.SetMoveToControl(0, &m_sliderGamma);
    m_btnResolution.SetMoveToControl(2, &m_cbShadows);
    m_cbShadows.SetMoveToControl(0, &m_btnResolution);
    m_cbFramerateLock.SetMoveToControl(2, &m_btnAdvanced);
    m_btnAdvanced.SetMoveToControl(0, &m_cbFramerateLock);
    m_btnAdvanced.SetMoveToControl(2, &m_sliderGamma);
    m_sliderGamma.SetMoveToControl(0, &m_btnAdvanced);
#endif
#ifdef K2_QOL_FULLSCREEN
    // Fullscreen joins the column under the frame-buffer row.
    {
        CSWGuiControl* pBelow = m_cbFramerateLock.m_moveToControls[2];
        m_cbFramerateLock.SetMoveToControl(2, &m_cbFullscreen);
        m_cbFullscreen.SetMoveToControl(0, &m_cbFramerateLock);
        m_cbFullscreen.SetMoveToControl(2, pBelow);
        if (pBelow && pBelow->AsNavigable()) {
            CSWGuiNavigable* pNav = static_cast<CSWGuiNavigable*>(pBelow->AsNavigable());
            if (pNav->m_moveToControls[0] == &m_cbFramerateLock)
                pNav->SetMoveToControl(0, &m_cbFullscreen);
        }
    }
#endif
}
#else
CSWGuiOptionsGraphics::CSWGuiOptionsGraphics(CSWGuiManager* pGuiManager, int bInGame)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("optgraphics_p"), 1, 1, 1);
    InitControl(m_sliderGamma, CExoString("SLI_GAMMA"), 1, 1, 0, 0, 0);
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_labelGamma, CExoString("LBL_GAMMA"), 1, 1, 0, 0, 0);
    InitControl(m_cbShadows, CExoString("CB_SHADOWS"), 1, 1, 0, 0, 0);
    InitControl(m_cbGrass, CExoString("CB_GRASS"), 1, 1, 0, 0, 0);
#ifndef K2_QOL_ADVANCED_ROWS  // Force Speed Effects and the framerate lock are Advanced rows
    InitControl(m_cbForceSpeed, CExoString("CB_FORCESPEED"), 1, 1, 0, 0, 0);

    m_cbForceSpeed.m_text.m_params.SetText(CExoString(GetLocalizedCustomString(5)));
    m_cbForceSpeed.m_tail.m_nUserData = SWGUI_STRREF_CUSTOM_FORCESPEED;
#endif

    m_sliderGamma.m_tail.m_nUserData = STRREF_DESC_BRIGHTNESS;
    m_labelGamma.m_tail.m_nUserData = STRREF_DESC_BRIGHTNESS;
    m_btnResolution.m_tail.m_nUserData = STRREF_DESC_RESOLUTION;
    m_btnResolutionLeft.m_tail.m_nUserData = STRREF_DESC_RESOLUTION;
    m_btnResolutionRight.m_tail.m_nUserData = STRREF_DESC_RESOLUTION;
    m_cbShadows.m_tail.m_nUserData = STRREF_DESC_SHADOWS;
    m_cbGrass.m_tail.m_nUserData = STRREF_DESC_GRASS;
    m_btnAdvanced.m_tail.m_nUserData = STRREF_DESC_ADVANCED_GRAPHICS;
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;

#ifndef K2_UI_TOUCH
    // BTN_ADVANCED is bound; BTN_RESOLUTION (it opens CSWGuiOptionsResolution, the host's window
    // sizes) only where there is a window -- on a phone the device owns the display mode.
#ifdef K2_RESOLUTION_BUTTON
    InitControl(m_btnResolution, CExoString("BTN_RESOLUTION"), 1, 1, 0, 0, 0);
#endif
    InitControl(m_btnAdvanced, CExoString("BTN_ADVANCED"), 1, 1, 0, 0, 0);
#endif

    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnDefault, CExoString("BTN_DEFAULT"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#ifdef K2_QOL_FULLSCREEN
    K2QolAddFullscreenRow();
#endif

#ifndef K2_QOL_ADVANCED_ROWS
    // The Android-only framerate-lock row, cloned from CB_FORCESPEED.
    int nRowX = m_cbForceSpeed.m_originalExtent.x;
    int nRowY = m_cbForceSpeed.m_originalExtent.y;
    int nRowW = m_cbForceSpeed.m_originalExtent.w;
    int nRowH = m_cbForceSpeed.m_originalExtent.h;

    m_cbFramerateLock = m_cbForceSpeed;
    m_cbFramerateLock.m_pParent = m_cbForceSpeed.m_pParent;
    m_cbFramerateLock.m_pContainer = this;

    // The 25x25 glyph, vertically centred in the row and 2 px below its top edge.
    m_cbFramerateLock.m_border.m_extent.x = nRowX;
    m_cbFramerateLock.m_border.m_extent.y = nRowY + (nRowH - 25) / 2 + 52;
    m_cbFramerateLock.m_border.m_extent.w = 25;
    m_cbFramerateLock.m_border.m_extent.h = 25;
    m_cbFramerateLock.m_borderHilight.m_extent.x = nRowX;
    m_cbFramerateLock.m_borderHilight.m_extent.y = nRowY + (nRowH - 25) / 2 + 52;
    m_cbFramerateLock.m_borderHilight.m_extent.w = 25;
    m_cbFramerateLock.m_borderHilight.m_extent.h = 25;
    m_cbFramerateLock.m_borderToggledHilight.m_extent.x = nRowX;
    m_cbFramerateLock.m_borderToggledHilight.m_extent.y = nRowY + (nRowH - 25) / 2 + 52;
    m_cbFramerateLock.m_borderToggledHilight.m_extent.w = 25;
    m_cbFramerateLock.m_borderToggledHilight.m_extent.h = 25;
    m_cbFramerateLock.m_borderToggled.m_extent.x = nRowX;
    m_cbFramerateLock.m_borderToggled.m_extent.y = nRowY + (nRowH - 25) / 2 + 52;
    m_cbFramerateLock.m_borderToggled.m_extent.w = 25;
    m_cbFramerateLock.m_borderToggled.m_extent.h = 25;

    CSWGuiExtent extentText;
    extentText.x = nRowX + 30;
    extentText.y = nRowY + 50;
    extentText.w = nRowW - 30;
    extentText.h = nRowH;
    m_cbFramerateLock.m_text.SetExtent(extentText);

    m_cbFramerateLock.m_extent.x = nRowX;
    m_cbFramerateLock.m_extent.y = nRowY + 50;
    m_cbFramerateLock.m_extent.w = nRowW;
    m_cbFramerateLock.m_extent.h = nRowH;

    InitControl(m_cbFramerateLock, 1, 1, CExoString(static_cast<char*>(0)), 0);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkFitText(&m_cbFramerateLock.m_text);   // built by hand, so CSWGuiButton::Load never marked it
#endif
    m_cbFramerateLock.m_text.m_params.SetText(CExoString(GetLocalizedCustomString(7)));
    m_cbFramerateLock.m_tail.m_nUserData = SWGUI_STRREF_CUSTOM_FRAMERATELOCK;
#endif

#if !defined(K2_UI_PC)   // pc: the layout's width holds the caption (no pad glyph beside it)
    // BTN_DEFAULT has to be wide enough for its own caption plus three row-heights.
    CSWGuiControl* pDefault = &m_btnDefault;
    CSWGuiExtent extentDefault = m_btnDefault.m_extent;
    int nIdealWidth = extentDefault.h * 3;
    CSWGuiExtent extentIdeal = m_btnDefault.m_text.GetIdealWidthAndHeight();
    nIdealWidth += extentIdeal.w;
    extentDefault.w = (extentDefault.w > nIdealWidth) ? extentDefault.w : nIdealWidth;
    pDefault->SetExtent(extentDefault);
#endif

    m_labelDesc.Initialize(m_listBoxDesc.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem), 1.0f);

    m_sliderGamma.SetCurValue(57);
    m_sliderGamma.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlider));
    m_sliderGamma.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnExitSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnDefault));
    m_sliderGamma.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnMoveGammaSlider));

    m_btnResolution.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbShadows.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbGrass.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbForceSpeed.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbFramerateLock.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));

    m_btnResolution.m_tail.bFocusOnHover = 0;
    m_btnResolution.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnResolution));
    m_cbShadows.m_tail.bFocusOnHover = 0;
    m_cbShadows.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnShadows));
    m_cbGrass.m_tail.bFocusOnHover = 0;
    m_cbGrass.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnGrass));
    m_cbForceSpeed.m_tail.bFocusOnHover = 0;
    m_cbForceSpeed.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnForceSpeed));
    m_cbFramerateLock.m_tail.bFocusOnHover = 0;
    m_cbFramerateLock.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnFramerateLock));

    SetUpOptions();

    // BTN_BACK takes the panel's own virtual B-button entry point.
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnDefault));
    m_btnAdvanced.m_tail.bFocusOnHover = 0;
    m_btnAdvanced.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnAdvanced));

    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');

    SetActiveControl(&m_sliderGamma, 0);

#if defined(K2_QOL_ADVANCED_ROWS) && !defined(K2_UI_TOUCH)
    {
        // The column top to bottom, wrapping. Back and Default stay off it as on every other
        // Options page (they are the B and Y buttons), so whatever led to a row this page no
        // longer has now leads to Advanced Options.
        CSWGuiNavigable* apRows[6];
        int nRows = 0;
        apRows[nRows++] = &m_sliderGamma;
        apRows[nRows++] = &m_cbShadows;
        apRows[nRows++] = &m_cbGrass;
#ifdef K2_QOL_FULLSCREEN
        apRows[nRows++] = &m_cbFullscreen;
#endif
#ifdef K2_RESOLUTION_BUTTON
        apRows[nRows++] = &m_btnResolution;
#endif
        apRows[nRows++] = &m_btnAdvanced;
        for (int i = 0; i < nRows; i++) {
            apRows[i]->m_moveToControls[0] = apRows[(i + nRows - 1) % nRows];
            apRows[i]->m_moveToControls[2] = apRows[(i + 1) % nRows];
        }
        CSWGuiNavigable* apTrim[] = { &m_btnBack, &m_btnDefault };
        for (int i = 0; i < 2; i++) {
            CSWGuiControl* pUp = apTrim[i]->m_moveToControls[0];
            bool bInColumn = false;
            for (int j = 0; j < nRows; j++)
                bInColumn |= pUp == apRows[j];
            if (pUp && !bInColumn && pUp != &m_btnBack && pUp != &m_btnDefault)
                apTrim[i]->m_moveToControls[0] = &m_btnAdvanced;
        }
    }
#elif !defined(K2_UI_TOUCH)
    // Splice the framerate row and Advanced into the pad and keyboard up / down chain. The
    // framerate-lock row is a copy of the force-speed checkbox made after the layout's links were
    // resolved, so it inherited that row's own links and nothing linked to it: the chain runs
    // force-speed, framerate, Advanced and then Advanced's own layout target, and every link that
    // led back to the force-speed row is repointed.
    {
        CSWGuiControl* pBelow = m_btnAdvanced.m_moveToControls[2];
        if (pBelow == 0 || pBelow == &m_cbFramerateLock || pBelow == &m_btnAdvanced ||
            pBelow == &m_btnBack || pBelow == &m_btnDefault)
            pBelow = &m_sliderGamma;
        CSWGuiControl* pLast = &m_btnAdvanced;
        m_cbForceSpeed.m_moveToControls[2] = &m_cbFramerateLock;
        m_cbFramerateLock.m_moveToControls[0] = &m_cbForceSpeed;
#ifdef K2_RESOLUTION_BUTTON
        // ... -> framerate -> BTN_RESOLUTION -> BTN_ADVANCED, the layout's order.
        m_cbFramerateLock.m_moveToControls[2] = &m_btnResolution;
        m_btnResolution.m_moveToControls[0] = &m_cbFramerateLock;
        m_btnResolution.m_moveToControls[2] = &m_btnAdvanced;
        m_btnAdvanced.m_moveToControls[0] = &m_btnResolution;
#else
        m_cbFramerateLock.m_moveToControls[2] = &m_btnAdvanced;
        m_btnAdvanced.m_moveToControls[0] = &m_cbFramerateLock;
#endif
        m_btnAdvanced.m_moveToControls[2] = pBelow;
        CSWGuiNavigable* apNav[] = { &m_sliderGamma, &m_cbShadows, &m_cbGrass, &m_btnBack,
                                     &m_btnDefault };
        for (unsigned i = 0; i < sizeof(apNav) / sizeof(apNav[0]); i++)
            if (apNav[i]->m_moveToControls[0] == &m_cbForceSpeed
#ifndef K2_RESOLUTION_BUTTON
                || apNav[i]->m_moveToControls[0] == &m_btnResolution
#endif
                )
                apNav[i]->m_moveToControls[0] = pLast;
    }
#endif

    m_bResolutionChanged = 0;
    m_bInGame = bInGame;
    if (bInGame) {
        // The mode cannot be changed mid-game, so BTN_RESOLUTION is painted disabled.
        m_btnResolution.m_border.m_params.m_color.r = CGuiInGame::DISABLED_TEXT.x;
        m_btnResolution.m_border.m_params.m_color.g = CGuiInGame::DISABLED_TEXT.y;
        m_btnResolution.m_border.m_params.m_color.b = CGuiInGame::DISABLED_TEXT.z;
        m_btnResolution.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnResolution.m_borderHilight.m_params.m_color.r = CGuiInGame::DISABLED_TEXT.x;
        m_btnResolution.m_borderHilight.m_params.m_color.g = CGuiInGame::DISABLED_TEXT.y;
        m_btnResolution.m_borderHilight.m_params.m_color.b = CGuiInGame::DISABLED_TEXT.z;
    }
    m_bSlotDescriptionArmed = 0;
}
#endif

CSWGuiOptionsGraphics::~CSWGuiOptionsGraphics()
{
}

void CSWGuiOptionsCheckbox::SetExtent(const CSWGuiExtent& extent)
{
#ifdef K2_QOL_FULLSCREEN
    // The box is 25 px whatever the screen: at a desktop's native resolution (2000+ lines) it is a
    // speck beside its caption. It grows with the row from the 60-line row it has at 1080 lines.
    int nBox = extent.h * 25 / 60;
    if (nBox < 25)
        nBox = 25;
    CSWGuiExtent borderExtent(extent.x, extent.y + (extent.h - nBox) / 2 + 2, nBox, nBox);
#else
    CSWGuiExtent borderExtent(extent.x, extent.y + (extent.h - 25) / 2 + 2, 25, 25);
#endif
    m_border.m_extent = borderExtent;
    m_borderHilight.m_extent = borderExtent;
    m_borderToggledHilight.m_extent = borderExtent;
    m_borderToggled.m_extent = borderExtent;

    CSWGuiExtent textExtent = extent;
#ifdef K2_QOL_FULLSCREEN
    textExtent.x += nBox + 5;
    textExtent.w -= nBox + 5;
#else
    textExtent.x += 30;
    textExtent.w -= 30;
#endif
    m_text.SetExtent(textExtent);

#if KOTOR1  // K1: CSWGuiObject::SetExtent is out of line
    CSWGuiObject::SetExtent(extent);
#else
    m_extent = extent;
#endif
}

void CSWGuiOptionsCheckbox::Draw(float f)
{
#if KOTOR1
    // K1: the active caption is COLOR_YELLOW, the enabled one COLOR_BLUE.
    if (m_tail.bActive) {
        m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
        m_text.m_params.bHilight = 1;
    } else if (m_tail.bEnabled) {
        m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_text.m_params.bHilight = 0;
    } else {
#else
    if (m_tail.bActive) {
        m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        m_text.m_params.bHilight = 1;
    } else if (m_tail.bEnabled) {
        m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_text.m_params.bHilight = 0;
    } else {
#endif
        m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
    }
    CSWGuiButtonToggle::Draw(f);
}

void CSWGuiOptionsCheckbox::Load(CResGFF* pGFF, CResStruct& resStruct)
{
    CSWGuiButtonToggle::Load(pGFF, resStruct);
#if KOTOR1
    // K1: COLOR_BLUE frames, COLOR_YELLOW highlights.
    m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_borderToggled.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    m_borderToggledHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
#else
    m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_TEXT);
    m_borderToggled.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_TEXT);
    m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
    m_borderToggledHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
}

void CSWGuiOptionsGraphics::OnEnterSlider(CSWGuiControl* pControl)
{
#if KOTOR1
    if (&m_sliderGamma == pControl)
        SetDescription(STRREF_DESC_BRIGHTNESS);
    static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_imageThumb.m_params.bHilight = 1;
#else
    if (&m_sliderGamma == pControl) {
        m_bSlotDescriptionArmed = 0;
        SetDescription(STRREF_DESC_BRIGHTNESS);
    }
    static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_imageThumb.m_params.bHilight = 1;
#endif
}

void CSWGuiOptionsGraphics::OnExitSlider(CSWGuiControl* pControl)
{
    static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_imageThumb.m_params.bHilight = 0;
}

void CSWGuiOptionsGraphics::OnMoveGammaSlider(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1: no description refresh and no clamp; the whole slider range maps onto the gamma curve.
    int nMax = static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_nMaxValue;
    int nCur = static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_nCurValue;
    g_pAurora->ResetGammaRamp(static_cast<float>(nCur) / static_cast<float>(nMax) * -1.77f + 2.0f, 0);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nGammaIndex = static_cast<unsigned char>(nCur);
#else
    if (&m_sliderGamma == pControl)
        SetDescription(STRREF_DESC_BRIGHTNESS);

    int nMax = static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_nMaxValue;
    int nCur = static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_nCurValue;
    int nClamped = (nCur < 85) ? nCur : 85;
    g_pAurora->ResetGammaRamp(static_cast<float>(nClamped + 15) / static_cast<float>(nMax) * -1.77f + 2.0f, 0);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nGammaIndex = static_cast<unsigned char>(nCur);
#endif
}

void CSWGuiOptionsGraphics::OnDefault(CSWGuiControl* pControl)
{
#if KOTOR1
    K2_UNUSED(pControl);
    // K1: BTN_DEFAULT forwards to the panel's own restore-defaults input arm.
    HandleInputEvent(INPUTEVENT_GUI_Y, 1);
#else
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultGraphicsOptions();
        SetUpOptions();
    }
#endif
}

void CSWGuiOptionsGraphics::OnEnterSlot(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1: no first-hover latch.
    SetDescription(pControl->m_tail.m_nUserData);
#else
    if (m_bSlotDescriptionArmed)
        SetDescription(pControl->m_tail.m_nUserData);
    m_bSlotDescriptionArmed = 1;
#endif
}

void CSWGuiOptionsGraphics::OnResolution(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1: no in-game refusal box; the resolution list always opens.
    if (pControl->m_tail.nCurrentEventArg) {
#ifdef K2_QOL_FULLSCREEN
        s_bK1ResolutionInGame = K1QolGameLoaded();
#endif
        CSWGuiOptionsResolution* pResolution = new CSWGuiOptionsResolution(m_pGuiManager);
        pResolution->CenterPanel();
        m_pGuiManager->AddPanel(pResolution, 1, 1);
    }
#else
    if (pControl->m_tail.nCurrentEventArg) {
        CSWGuiPanel* pPanel;
        if (m_bInGame) {
            CSWGuiMessageBox* pBox = static_cast<CSWGuiMessageBox*>(
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox);
            pBox->SetAllowCancel(0);
            pBox->SetCallback(0, 0, 0);
            pBox->SetMessage(static_cast<uint64_t>(STRREF_RESOLUTION_MAIN_MENU_ONLY));
            pPanel = pBox;
        } else {
            CSWGuiOptionsResolution* pResolution = new CSWGuiOptionsResolution(m_pGuiManager);
            pResolution->CenterPanel();
#if !KOTOR1
            pResolution->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
            pPanel = pResolution;
        }
        m_pGuiManager->AddPanel(pPanel, 1, 1);
    }
#endif
}

void CSWGuiOptionsGraphics::OnShadows(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        if (pOptions != 0) {
            pOptions->SetShadows(pOptions->m_bShadows == 0);
            if (pOptions->m_bShadows == 0)
                pOptions->SetSoftShadows(0);
        }
    }
}

void CSWGuiOptionsGraphics::OnGrass(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->SetGrass(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGrass == 0);
    }
}

void CSWGuiOptionsGraphics::OnForceSpeed(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_allowForceSpeedEffect = !g_allowForceSpeedEffect;
        if (!g_allowForceSpeedEffect)
            AurDisableSpeedBlur();
        else if (g_gameWantsSpeedBlur)
            GLRender::m_bSpeedBlurEnabled = true;
    }
}

#if defined(K2_QOL_ADVANCED_ROWS) && !KOTOR1
// Runs OnForceSpeed's body for a value; Advanced Graphics applies its pending Force Speed row with
// it.
static void K2QolSetForceSpeed(bool bOn)
{
    g_allowForceSpeedEffect = bOn;
    if (!g_allowForceSpeedEffect)
        AurDisableSpeedBlur();
    else if (g_gameWantsSpeedBlur)
        GLRender::m_bSpeedBlurEnabled = true;
}
#endif

void CSWGuiOptionsGraphics::OnFramerateLock(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        g_bFrameRateLocked = !g_bFrameRateLocked;
}

#if KOTOR1
void CSWGuiOptionsGraphics::OnHighResolution(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScreenRes = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nScreenRes;
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetScreenRes(nScreenRes == 2 ? 1 : 2);
    }
}

void CSWGuiOptionsGraphics::OnFrameBuffer(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg)
        return;

    g_pAppManager->m_pClientExoApp->GetClientOptions();
    unsigned int nFrameBuffer;
    if (preferPerfToVisual) {
        m_cbShadows.SetEnabled(1);
        m_sliderGamma.CSWGuiControl::SetEnabled(1);
        m_labelGamma.SetEnabled(1);
        nFrameBuffer = 1;
    } else {
        m_cbShadows.bToggledOn = 0;
        m_cbShadows.SetEnabled(0);
        m_sliderGamma.CSWGuiControl::SetEnabled(0);
        m_labelGamma.SetEnabled(0);
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetShadows(0);
        nFrameBuffer = 0;
    }
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetFrameBuffer(nFrameBuffer);
}

void CSWGuiOptionsGraphics::OnAntiAlias(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->SetAntiAlias(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nAntiAlias == 0);
    }
    m_bResolutionChanged = 1;
}
#endif

void CSWGuiOptionsGraphics::SetUpOptions()
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions == 0)
        return;

    m_sliderGamma.SetCurValue(pOptions->m_nGammaIndex);
    OnMoveGammaSlider(&m_sliderGamma);

    // Soft shadows imply shadows.
    if (pOptions->m_bSoftShadows)
        pOptions->SetShadows(1);

#if KOTOR1
    // K1: the third row is the high-resolution toggle (m_nScreenRes == 2) and the fourth the
    // high-quality-graphics one (preferPerfToVisual, inverted). Performance mode clears and
    // disables shadows and the gamma pair, and parks the focus on CB_GRASS.
    m_cbForceSpeed.bToggledOn = (pOptions->m_nScreenRes == 2);
    if (preferPerfToVisual == 0) {
        m_cbFramerateLock.bToggledOn = 1;
        m_cbShadows.bToggledOn = pOptions->m_bShadows;
        m_cbShadows.SetEnabled(1);
        m_sliderGamma.CSWGuiControl::SetEnabled(1);
        m_labelGamma.SetEnabled(1);
        SetActiveControl(&m_sliderGamma, 0);
    } else {
        m_cbFramerateLock.bToggledOn = 0;
        m_cbShadows.bToggledOn = 0;
        m_cbShadows.SetEnabled(0);
        m_sliderGamma.CSWGuiControl::SetEnabled(0);
        m_labelGamma.SetEnabled(0);
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetShadows(0);
        SetActiveControl(&m_cbGrass, 0);
    }
    m_cbGrass.bToggledOn = pOptions->m_bGrass;
#else
    m_cbShadows.bToggledOn = pOptions->m_bShadows;
    m_cbGrass.bToggledOn = pOptions->m_bGrass;
    m_cbForceSpeed.bToggledOn = g_allowForceSpeedEffect;
    m_cbFramerateLock.bToggledOn = g_bFrameRateLocked;
#endif
#ifdef K2_QOL_FULLSCREEN
    m_cbFullscreen.bToggledOn = k2host_fullscreen() != 0;
#endif
}

void CSWGuiOptionsGraphics::OnAdvanced(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CSWGuiOptionsGraphicsAdvanced* pAdvanced =
            new CSWGuiOptionsGraphicsAdvanced(m_pGuiManager, this);
#if !KOTOR1
        pAdvanced->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pAdvanced, 3, 1);
    }
}

#if KOTOR1
// K1: the back arm is CSWGuiOptionsResolution's; Default also resets the advanced graphics
// options and re-applies the screen resolution, with no click sound.
void CSWGuiOptionsGraphics::HandleInputEvent(int type, int val)
{
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;

            if (m_bResolutionChanged) {
                CExoIni ini;
                CExoString sValue;

                ReInitAurora(g_nScreenWidth, g_nScreenHeight, g_nBitsPerPixel, g_pAurora->FullScreen(), 1);

                if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"),
                                     CExoString("Graphics Options"),
                                     CExoString("Disable Write-Only VBO"))) {
                    if (sValue.AsINT() == 1)
                        AurDisableVertexBufferObjectsWriteOnly();
                    else
                        AurEnableVertexBufferObjectsWriteOnly();
                }
                m_bResolutionChanged = 0;
            }
            break;
        case INPUTEVENT_GUI_Y: {
            CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
            pOptions->SetDefaultGraphicsOptions();
            pOptions->SetDefaultAdvancedGraphicsOptions();
            pOptions->SetScreenRes(pOptions->m_nScreenRes);
            SetUpOptions();
            break;
        }
        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}
#else
void CSWGuiOptionsGraphics::HandleInputEvent(int type, int val)
{
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
#if KOTOR1
            // K1: no panel chain; one stamp of state 4 (bits 8-10).
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
            CSWGuiPanel* pPanel = this;
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel);
#endif

            if (m_bResolutionChanged) {
                CExoIni ini;
                CExoString sValue;

                int bFound = ini.ReadIniEntry(sValue, CExoString(EXO_GAME_INI),
                                              CExoString("Graphics Options"),
                                              CExoString("Disable Vertex Buffer Objects"));
                if (bFound && sValue.m_sString && atoi(sValue.m_sString) == 1)
                    AurDisableVertexBufferObjects();

                ReInitAurora(g_nScreenWidth, g_nScreenHeight, g_nBitsPerPixel,
                             g_pAurora->FullScreen(), 1);

                bFound = ini.ReadIniEntry(sValue, CExoString(EXO_GAME_INI),
                                          CExoString("Graphics Options"),
                                          CExoString("Disable Write-Only VBO"));
                if (bFound) {
                    if (sValue.m_sString && atoi(sValue.m_sString) == 1)
                        AurDisableVertexBufferObjectsWriteOnly();
                    else
                        AurEnableVertexBufferObjectsWriteOnly();
                }
                m_bResolutionChanged = 0;
            }
            break;
        }
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultGraphicsOptions();
            SetUpOptions();
            break;
        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}
#endif

void CSWGuiOptionsGraphics::SetDescription(uint64_t nStrRef)
{
#if KOTOR1
    // K1: the high-quality-graphics and anti-aliasing rows carry hardcoded Latin-1 texts per
    // ASLPlat_GetCurrentLanguage (1 fr, 2 it, 3 de, 4 es, else en), written in the order that
    // makes the compare chain test the anti-aliasing row first.
#ifdef K2_UI_TOUCH
    bool bHighQuality = nStrRef == STRREF_DESC_RESOLUTION;
#else
    // The PC layout's Screen Resolution row owns this strref; the high-quality row carries the
    // custom id instead (the ctor).
    bool bHighQuality = nStrRef == SWGUI_STRREF_CUSTOM_FORCESPEED;
#endif
    if (bHighQuality) {
        switch (ASLPlat_GetCurrentLanguage()) {
        case ASL_LANGUAGE_FRENCH:
            m_labelDesc.m_text.m_params.SetText(CExoString("D\351sactiver les graphismes de qualit\351 sup\351rieure permet au jeu de fonctionner de fa\347on plus r\351guli\350re. S'applique au red\351marrage du jeu."));
            break;
        case ASL_LANGUAGE_ITALIAN:
            m_labelDesc.m_text.m_params.SetText(CExoString("Disattivare l'alta qualit\340 della grafica pu\362 aiutare il gioco a funzionare meglio. Sar\340 efficace la prossima volta che il gioco verr\340 avviato."));
            break;
        case ASL_LANGUAGE_GERMAN:
            m_labelDesc.m_text.m_params.SetText(CExoString("Das Deaktivieren der hohen Graphikqualit\344t kann dabei helfen, dass das Spiel fl\374ssiger l\344uft. Die \334bernahme dieser Einstellung ben\366tig einen Neustart des Spiels."));
            break;
        case ASL_LANGUAGE_SPANISH:
            m_labelDesc.m_text.m_params.SetText(CExoString("Desactivar los gr\341ficos de alta calidad puede contribuir a que el juego se ejecute con m\341s fluided. Se har\341 efectivo al volver a iniciar el juego."));
            break;
        default:
            m_labelDesc.m_text.m_params.SetText(CExoString("Turning High quality graphics off may help the game run smoother. Will be effective on the next launch of the game."));
            break;
        }
    } else if (nStrRef == STRREF_DESC_ANTIALIAS) {
        switch (ASLPlat_GetCurrentLanguage()) {
        case ASL_LANGUAGE_FRENCH:
            m_labelDesc.m_text.m_params.SetText(CExoString("Activer l'anti-cr\351nelage lisse les contours irr\351guliers du jeu."));
            break;
        case ASL_LANGUAGE_ITALIAN:
            m_labelDesc.m_text.m_params.SetText(CExoString("Attivare l'antialiasing leviga i bordi frastagliati o quell'aspetto a gradini del gioco."));
            break;
        case ASL_LANGUAGE_GERMAN:
            m_labelDesc.m_text.m_params.SetText(CExoString("Das Aktivieren der Kantengl\344ttung (Antialiasing) gl\344ttet die unebenden Kanten des Spiels."));
            break;
        case ASL_LANGUAGE_SPANISH:
            m_labelDesc.m_text.m_params.SetText(CExoString("Activar el Antialiasing suaviza los bordes dentados o el aspecto de efecto de escalera del juego."));
            break;
        default:
            m_labelDesc.m_text.m_params.SetText(CExoString("Turning on Anti-Aliasing smooths the jagged edges or stair-step look at the game."));
            break;
        }
    }
#ifdef K2_QOL_FULLSCREEN
    else if (nStrRef == kQolDescFullscreen) {
        m_labelDesc.m_text.m_params.SetText(
            CExoString(K2QolFullscreenDescription(K1QolGameLoaded())));
    }
#endif
    else {
        m_labelDesc.m_text.m_params.SetStrRef(nStrRef);
    }
#else
#ifdef K2_QOL_FULLSCREEN
    if (nStrRef == kQolDescFullscreen) {
        m_labelDesc.m_text.m_params.SetText(CExoString(K2QolFullscreenDescription(m_bInGame)));
    } else
#endif
    if (nStrRef == SWGUI_STRREF_CUSTOM_FORCESPEED) {
        CSWGuiTextParams* pParams = &m_labelDesc.m_text.m_params;
        CExoString sCustom(GetLocalizedCustomString(6));
        pParams->SetText(sCustom);
    } else if (nStrRef == SWGUI_STRREF_CUSTOM_FRAMERATELOCK) {
        CSWGuiTextParams* pParams = &m_labelDesc.m_text.m_params;
        CExoString sCustom(GetLocalizedCustomString(8));
        pParams->SetText(sCustom);
    } else {
        m_labelDesc.m_text.m_params.SetStrRef(nStrRef);
    }
#endif

    CExoString sText(m_labelDesc.m_text.m_params.m_sText);
    m_listBoxDesc.ClearItems();

    CSWGuiText* pText = &m_labelDesc.m_text;

    CSWGuiExtent extent = m_labelDesc.m_extent;
#if KOTOR1
    if (sText.GetLength() >= 1)
#else
    if (sText.m_sString && static_cast<int>(strlen(sText.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_labelDesc.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_labelDesc };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_labelDesc, 0);
}


// BTN_DEFAULT hover.

void CSWGuiOptionsGraphics::OnEnterDefault(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    SetDescription(STRREF_DESC_DEFAULT_BRIGHTNESS);
}



extern CExoSound* g_pExoSound;

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

CSWGuiOptionsSoundAdvanced::CSWGuiOptionsSoundAdvanced(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: layout "optsoundadv", no BTN_DEFAULT width fix-up, no button glyphs.
    StartLoadFromLayout(CResRef("optsoundadv"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("optsoundadv_p"), 1, 1, 1);
#endif
    InitControl(m_checkForceSoftware, CExoString("CB_FORCESOFTWARE"), 1, 1, 0, 0, 0);
    InitControl(m_btnEAX, CExoString("BTN_EAX"), 1, 1, 0, 0, 0);
    InitControl(m_btnEAXRight, CExoString("BTN_EAXRIGHT"), 1, 1, 0, 0, 0);
    InitControl(m_btnEAXLeft, CExoString("BTN_EAXLEFT"), 1, 1, 0, 0, 0);
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnDefault, CExoString("BTN_DEFAULT"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aLabelBar[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
#endif

    // The two hover-description payloads, for the checkbox and for BTN_EAX, then the scroll-sound
    // byte.
#if KOTOR1
    m_checkForceSoftware.m_tail.m_nUserData = STRREF_DESC_FORCE_SOFTWARE_SOUND;  // K1's own strrefs
    m_btnEAX.m_tail.m_nUserData = STRREF_DESC_EAX;
#else
    m_checkForceSoftware.m_tail.m_nUserData = STRREF_DESC_FORCE_SOFTWARE_SOUND;
    m_btnEAX.m_tail.m_nUserData = STRREF_DESC_EAX;
#endif
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;

    StopLoadFromLayout();

#if KOTOR1
    ScaleExtentForResolution();
#elif !defined(K2_UI_PC)   // pc: the layout's width holds the caption (no pad glyph beside it)
    CSWGuiControl* pDefault = &m_btnDefault;
    CSWGuiExtent extentDefault = m_btnDefault.m_extent;
    int nIdealWidth = extentDefault.h * 3;
    CSWGuiExtent extentIdeal = m_btnDefault.m_text.GetIdealWidthAndHeight();
    nIdealWidth += extentIdeal.w;
    extentDefault.w = (extentDefault.w > nIdealWidth) ? extentDefault.w : nIdealWidth;
    pDefault->SetExtent(extentDefault);
#endif

    // The description listbox's single row is cloned from the listbox's prototype item.
    m_labelDesc.Initialize(m_listBoxDesc.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem), 1.0f);

    // Five EAX controls, with the click and toggle events on the three that act and the hover
    // event on all four of the top group.
    m_checkForceSoftware.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnForceSoftware));
    m_checkForceSoftware.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnForceSoftware));
    m_checkForceSoftware.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnEnterOption));

    m_btnEAXLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnEAXLeft));
    m_btnEAXLeft.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnEAXLeft));
    m_btnEAXLeft.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnEnterOption));

    m_btnEAXRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnEAXRight));
    m_btnEAXRight.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnEAXRight));
    m_btnEAXRight.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnEnterOption));

    m_btnEAX.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnEnterOption));

    SetUpOptions();

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnDefault));
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSoundAdvanced::OnDiscardChanges));

#if !KOTOR1
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnCancel, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');
#endif

    SetActiveControl(&m_checkForceSoftware, 0);

    m_nDiscardChanges = 0;
}

void CSWGuiOptionsSoundAdvanced::OnForceSoftware(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
#if KOTOR1
        // K1: no EAX-level reset; the enabled caption colour is COLOR_BLUE.
        int nOld = m_nForceSoftware;
        m_nForceSoftware = (nOld == 0);
        int bEnabled = (nOld != 0);
        m_btnEAX.SetEnabled(bEnabled);
        m_btnEAX.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_BLUE
                                                   : CGuiInGame::DISABLED_TEXT);
        m_btnEAXRight.SetEnabled(bEnabled);
        m_btnEAXLeft.SetEnabled(bEnabled);
        m_nOptionsChanged = 1;
#else
        int nOld = m_nForceSoftware;
        m_nForceSoftware = (nOld == 0);
        int bEnabled;
        if (nOld != 0) {
            m_nEAXLevel = 0;
            SetEAXText();
            bEnabled = (m_nForceSoftware == 0);
        } else {
            bEnabled = 0;
        }
        m_btnEAX.SetEnabled(bEnabled);
        m_btnEAX.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_NEW_TEXT
                                                   : CGuiInGame::DISABLED_TEXT);
        m_btnEAXRight.SetEnabled(bEnabled);
        m_btnEAXLeft.SetEnabled(bEnabled);
        m_nOptionsChanged = 1;
#endif
    }
}

void CSWGuiOptionsSoundAdvanced::OnEnterOption(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1: the same chain with K1's own strrefs.
    if (pControl == &m_checkForceSoftware)
        SetDescription(STRREF_DESC_FORCE_SOFTWARE_SOUND);
    else if (pControl == &m_btnEAXLeft)
        SetDescription(STRREF_DESC_EAX);
    else if (pControl == &m_btnEAX)
        SetDescription(STRREF_DESC_EAX);
    else if (pControl == &m_btnEAXRight)
        SetDescription(STRREF_DESC_EAX);
#else
    if (pControl == &m_checkForceSoftware)
        SetDescription(STRREF_DESC_FORCE_SOFTWARE_SOUND);
    else if (pControl == &m_btnEAXLeft)
        SetDescription(STRREF_DESC_EAX);
    else if (pControl == &m_btnEAX)
        SetDescription(STRREF_DESC_EAX);
    else if (pControl == &m_btnEAXRight)
        SetDescription(STRREF_DESC_EAX);
#endif
}

void CSWGuiOptionsSoundAdvanced::OnEAXLeft(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        int nLevel = m_nEAXLevel - 1;
        if (nLevel >= 0) {
            m_nEAXLevel = nLevel;
            SetEAXText();
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            m_nOptionsChanged = 1;
        }
    }
}

void CSWGuiOptionsSoundAdvanced::OnEAXRight(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        // The getter is called twice: once for the clamp test and once (unclamped) for the
        // comparison.
#if KOTOR1
        // K1: one GetBestEAXAvailable call, clamped to 3.
        int nMax = g_pExoSound->GetBestEAXAvailable();
        int nLimit = (nMax < 3) ? nMax : 3;
#else
        int nMax = g_pExoSound->GetBestEAXAvailable();
        int nLimit = (nMax > 3) ? 3 : g_pExoSound->GetBestEAXAvailable();
#endif
        if (m_nEAXLevel != nLimit) {
            m_nEAXLevel = m_nEAXLevel + 1;
            SetEAXText();
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            m_nOptionsChanged = 1;
        }
    }
}

void CSWGuiOptionsSoundAdvanced::SetUpOptions()
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions != 0) {
        m_nEAXLevel = pOptions->m_nEAXLevel;
        SetEAXText();
        int nForce = pOptions->m_bForceSoftware;
        m_nForceSoftware = nForce;
        m_checkForceSoftware.bToggledOn = (nForce & 1);
        int bEnabled = (pOptions->m_bForceSoftware == 0);
        m_btnEAX.SetEnabled(bEnabled);
#if KOTOR1
        m_btnEAX.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_BLUE  // K1: COLOR_BLUE
                                                   : CGuiInGame::DISABLED_TEXT);
#else
        m_btnEAX.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_NEW_TEXT
                                                   : CGuiInGame::DISABLED_TEXT);
#endif
        m_btnEAXRight.SetEnabled(bEnabled);
        m_btnEAXLeft.SetEnabled(bEnabled);
    }
    m_nOptionsChanged = 0;
}

void CSWGuiOptionsSoundAdvanced::SetNewOptions()
{
    if (g_pAppManager->m_pClientExoApp->GetClientOptions() != 0) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_nEAXLevel = static_cast<unsigned char>(m_nEAXLevel);
        pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bForceSoftware = m_nForceSoftware;
    }
}

void CSWGuiOptionsSoundAdvanced::SetEAXText()
{
    CExoString sTemp;

#if KOTOR1
    // K1: its own strrefs, one clamped GetBestEAXAvailable, and the right arrow is only
    // considered when the left one stays visible (level 0 never hides both).
    switch (m_nEAXLevel) {
    case 0:  m_btnEAX.m_text.m_params.SetStrRef(STRREF_EAX_DISABLED); break;
    case 2:  m_btnEAX.m_text.m_params.SetStrRef(STRREF_EAX_2); break;
    case 3:  m_btnEAX.m_text.m_params.SetStrRef(STRREF_EAX_3); break;
    default: m_btnEAX.m_text.m_params.SetStrRef(STRREF_EAX); break;
    }

    m_btnEAXLeft.m_tail.bVisible = 1;
    m_btnEAXRight.m_tail.bVisible = 1;

    int nBest = g_pExoSound->GetBestEAXAvailable();
    if (m_nEAXLevel == 0)
        m_btnEAXLeft.m_tail.bVisible = 0;
    else if (m_nEAXLevel == ((nBest < 3) ? nBest : 3))
        m_btnEAXRight.m_tail.bVisible = 0;
#else
    switch (m_nEAXLevel) {
    case 0:  m_btnEAX.m_text.m_params.SetStrRef(STRREF_EAX_DISABLED); break;
    case 2:  m_btnEAX.m_text.m_params.SetStrRef(STRREF_EAX_2); break;
    case 3:  m_btnEAX.m_text.m_params.SetStrRef(STRREF_EAX_3); break;
    default: m_btnEAX.m_text.m_params.SetStrRef(STRREF_EAX); break;
    }

    m_btnEAXLeft.m_tail.bVisible = 1;
    m_btnEAXRight.m_tail.bVisible = 1;

    int nBest = g_pExoSound->GetBestEAXAvailable();
    int nLimit = (nBest > 3) ? 3 : g_pExoSound->GetBestEAXAvailable();

    if (m_nEAXLevel == 0)
        m_btnEAXLeft.m_tail.bVisible = 0;

    if (m_nEAXLevel != nLimit)
        return;

    m_btnEAXRight.m_tail.bVisible = 0;
#endif
}

void CSWGuiOptionsSoundAdvanced::EnableEAXControls(int bEnabled)
{
    m_btnEAX.SetEnabled(bEnabled);
#if KOTOR1
    m_btnEAX.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_BLUE  // K1: COLOR_BLUE
                                               : CGuiInGame::DISABLED_TEXT);
#else
    m_btnEAX.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_NEW_TEXT
                                               : CGuiInGame::DISABLED_TEXT);
#endif
    m_btnEAXRight.SetEnabled(bEnabled);
    m_btnEAXLeft.SetEnabled(bEnabled);
}

void CSWGuiOptionsSoundAdvanced::OnDefault(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultAdvancedSoundOptions();
        SetUpOptions();
        m_nOptionsChanged = 1;
    }
}

void CSWGuiOptionsSoundAdvanced::OnDiscardChanges(CSWGuiControl* pControl)
{
    m_nDiscardChanges = 1;
    OnBButtonPressed(pControl);
}

void CSWGuiOptionsSoundAdvanced::OnEnterDefault(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    SetDescription(STRREF_DESC_DEFAULT_BRIGHTNESS);
}

void CSWGuiOptionsSoundAdvanced::HandleInputEvent(int type, int val)
{
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (m_nDiscardChanges == 0) {
                SetNewOptions();
                if (m_nOptionsChanged != 0) {
                    g_pExoSound->ReInitialize(m_nEAXLevel,
                        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bForceSoftware == 0);

                    CClientOptions* pOptions;
                    CExoIni ini;
                    CExoString sValue;

                    sValue.Format(const_cast<char*>("%d"), g_pExoSound->GetEAX());
#if KOTOR1
                    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"),  // K1
                                      CExoString("Sound Options"), CExoString("EAX"));
#else
                    ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                                      CExoString("Sound Options"), CExoString("EAX"));
#endif

                    pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
                    pOptions->m_nEAXLevel = static_cast<unsigned char>(g_pExoSound->GetEAX());

                    sValue.Format(const_cast<char*>("%d"), g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bForceSoftware);
#if KOTOR1
                    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"),
                                      CExoString("Sound Options"), CExoString("Force Software"));
#else
                    ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                                      CExoString("Sound Options"), CExoString("Force Software"));
#endif
                }
            } else {
                m_nDiscardChanges = 0;
            }
            m_nOptionsChanged = 0;
            m_pGuiManager->PopModalPanel();
#if KOTOR1
            // K1: no panel chain; one stamp of state 4 (bits 8-10).
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
            CSWGuiPanel* pPanel = this;
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel);
#endif
            break;
        }
#if !KOTOR1  // K1 has no 0x2a (restore-defaults) input arm here
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultAdvancedSoundOptions();
            SetUpOptions();
            m_nOptionsChanged = 1;
            break;
#endif
        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;
        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;
        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiOptionsSoundAdvanced::SetDescription(uint64_t nStrRef)
{
    m_labelDesc.m_text.m_params.SetStrRef(nStrRef);

    CExoString sText(m_labelDesc.m_text.m_params.m_sText);
    m_listBoxDesc.ClearItems();

    CSWGuiText* pText = &m_labelDesc.m_text;

    CSWGuiExtent extent = m_labelDesc.m_extent;
#if KOTOR1
    if (sText.GetLength() >= 1)
#else
    if (sText.m_sString && static_cast<int>(strlen(sText.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_labelDesc.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_labelDesc };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_labelDesc, 0);
}




void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

CSWGuiOptionsMouse::CSWGuiOptionsMouse(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: layout "optmouse", no ExpandButtonToFitText, no button glyphs.
    StartLoadFromLayout(CResRef("optmouse"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("optmouse_p"), 1, 1, 1);
#endif
    InitControl(m_sliderSensitivity, CExoString("SLI_MOUSESEN"), 1, 1, 0, 0, 0);
    InitControl(m_labelSensitivity, CExoString("LBL_MOUSESEN"), 1, 1, 0, 0, 0);
    InitControl(m_checkRevButtons, CExoString("CB_REVBUTTONS"), 1, 1, 0, 0, 0);
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnDefault, CExoString("BTN_DEFAULT"), 1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aLabelBar[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
#endif

    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    StopLoadFromLayout();

#if KOTOR1
    ScaleExtentForResolution();
#else
    ExpandButtonToFitText(m_btnDefault);
#endif

    CSWGuiButtonToggle* pCheck = &m_checkRevButtons;
    m_labelDesc.Initialize(m_listBoxDesc.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem), 1.0f);

    pCheck->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnRevMouseButtons));
    pCheck->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnRevMouseButtons));
    pCheck->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnEnterRevMouseButtons));

    m_sliderSensitivity.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnMoveMouseSenSlider));
    m_sliderSensitivity.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnEnterSlider));
    m_sliderSensitivity.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnExitSlider));
    m_sliderSensitivity.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnMoveMouseSenSlider));
    m_sliderSensitivity.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnMoveMouseSenSlider));
    m_sliderSensitivity.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnMoveMouseSenSlider));
    m_sliderSensitivity.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnMoveMouseSenSlider));

    // Three hover-description payloads, one for the slider and its caption label, one for the
    // checkbox, which OnEnterRevMouseButtons also uses.
#if KOTOR1
    m_sliderSensitivity.m_tail.m_nUserData = STRREF_DESC_MOUSE_SENSITIVITY;  // K1's own strrefs
    m_labelSensitivity.m_tail.m_nUserData = STRREF_DESC_MOUSE_SENSITIVITY;
    m_checkRevButtons.m_tail.m_nUserData = STRREF_DESC_REVERSE_MOUSE_BUTTONS;
#else
    m_sliderSensitivity.m_tail.m_nUserData = STRREF_DESC_MOUSE_SENSITIVITY;
    m_labelSensitivity.m_tail.m_nUserData = STRREF_DESC_MOUSE_SENSITIVITY;
    m_checkRevButtons.m_tail.m_nUserData = STRREF_DESC_REVERSE_MOUSE_BUTTONS;
#endif

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMouse::OnDefault));

#if !KOTOR1
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');
#endif

    SetActiveControl(&m_sliderSensitivity, 0);

    SetUpOptions();
}

void CSWGuiOptionsMouse::OnRevMouseButtons(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CExoString sValue;
        CExoIni ini;

        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bReverseMouseButtons =
            (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons == 0);

        sValue.Format(const_cast<char*>("%d"), g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bReverseMouseButtons);
#if KOTOR1
        ini.WriteIniEntry(sValue, CExoString("swkotor.ini"),  // K1
                          CExoString("Game Options"), CExoString("Reverse Mouse Buttons"));
#else
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                          CExoString("Game Options"), CExoString("Reverse Mouse Buttons"));
#endif
    }
}

void CSWGuiOptionsMouse::OnEnterRevMouseButtons(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    SetDescription(STRREF_DESC_REVERSE_MOUSE_BUTTONS);
}

void CSWGuiOptionsMouse::OnMoveMouseSenSlider(CSWGuiControl* pControl)
{
    int nMaxValue = static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_nMaxValue;
    int nCurValue = static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_nCurValue;
    float fPosition = static_cast<float>(nCurValue) / static_cast<float>(nMaxValue);
    (void)fPosition;
    g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMouseSensitivity = static_cast<unsigned char>(nCurValue);
}

void CSWGuiOptionsMouse::OnEnterSlider(CSWGuiControl* pControl)
{
    if (&m_sliderSensitivity == pControl)
        SetDescription(STRREF_DESC_MOUSE_SENSITIVITY);
    static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_imageThumb.m_params.bHilight = 1;
}

void CSWGuiOptionsMouse::OnExitSlider(CSWGuiControl* pControl)
{
    static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_imageThumb.m_params.bHilight = 0;
}

void CSWGuiOptionsMouse::OnDefault(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultMouseOptions();
        SetUpOptions();
    }
}

void CSWGuiOptionsMouse::SetUpOptions()
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions == 0)
        return;

    m_sliderSensitivity.SetCurValue(pOptions->m_nMouseSensitivity);
    m_checkRevButtons.bToggledOn = (pOptions->m_bReverseMouseButtons & 1);
}

void CSWGuiOptionsMouse::HandleInputEvent(int type, int val)
{
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
#if KOTOR1
            // K1: no panel chain; one stamp of state 4 (bits 8-10).
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
            CSWGuiPanel* pPanel = this;
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel);
#endif
            break;
        }
#if !KOTOR1  // K1 has no 0x2a (restore-defaults) input arm here
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultMouseOptions();
            SetUpOptions();
            break;
#endif
        case INPUTEVENT_GUI_START:
            if (m_pActiveControl != 0 && m_pActiveControl->AsButtonToggle() != 0) {
                m_pGuiManager->PlayGuiSound(GUISOUND_CHECKBOX_CHECK);
                CSWGuiButtonToggle* pToggle = static_cast<CSWGuiButtonToggle*>(m_pActiveControl->AsButtonToggle());
                pToggle->bToggledOn = !static_cast<CSWGuiButtonToggle*>(m_pActiveControl->AsButtonToggle())->bToggledOn;
            }
            break;
        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;
        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiOptionsMouse::OnPanelAdded()
{
    CSWGuiPanel::OnPanelAdded();
}

void CSWGuiOptionsMouse::SetDescription(uint64_t nDescription)
{
    m_labelDesc.m_text.m_params.SetStrRef(nDescription);

    CExoString sText(m_labelDesc.m_text.m_params.m_sText);
    m_listBoxDesc.ClearItems();

    CSWGuiText* pText = &m_labelDesc.m_text;

    CSWGuiExtent extent = m_labelDesc.m_extent;
#if KOTOR1
    if (sText.GetLength() >= 1)
#else
    if (sText.m_sString && static_cast<int>(strlen(sText.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_labelDesc.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_labelDesc };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_labelDesc, 0);
}

void CSWGuiOptionsMouse::OnEnterSlot(CSWGuiControl* pControl) {
    SetDescription(pControl->m_tail.m_nUserData);
}


// The four options pages the On*Opt handlers allocate.


void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

void ExpandButtonToFitText(CSWGuiButton& button)
{
#ifdef K2_UI_PC
    // pc: the three row-heights are room for a pad glyph the pc captions do not carry
    K2_UNUSED(button);
#else
    CSWGuiExtent extent = button.m_extent;
    int nWidth = extent.h * 3 + button.m_text.GetIdealWidthAndHeight().w;
    extent.w = (extent.w > nWidth) ? extent.w : nWidth;
    button.SetExtent(extent);
#endif
}

CSWGuiOptionsMain::CSWGuiOptionsMain(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: layout "optionsmain", K1 description strrefs, every wall button pushed 20 px down,
    // and one iOS glyph on BTN_BACK.
    StartLoadFromLayout(CResRef("optionsmain"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("optionsmain_p"), 1, 1, 1);
#endif

    InitControl(m_lblTitle,    CExoString("LBL_TITLE"),    1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"),      1, 1, 0, 0, 0);
    InitControl(m_aButtons[0], CExoString("BTN_GAMEPLAY"), 1, 1, 0, 0, 0);
    InitControl(m_aButtons[1], CExoString("BTN_FEEDBACK"), 1, 1, 0, 0, 0);
    InitControl(m_aButtons[2], CExoString("BTN_AUTOPAUSE"),1, 1, 0, 0, 0);
    InitControl(m_aButtons[3], CExoString("BTN_GRAPHICS"), 1, 1, 0, 0, 0);
    InitControl(m_aButtons[4], CExoString("BTN_SOUND"),    1, 1, 0, 0, 0);

    m_aButtons[0].m_tail.m_nUserData = STRREF_DESC_OPTIONS_GAMEPLAY;
    m_aButtons[1].m_tail.m_nUserData = STRREF_DESC_OPTIONS_FEEDBACK;
    m_aButtons[2].m_tail.m_nUserData = STRREF_DESC_OPTIONS_AUTOPAUSE;
    m_aButtons[3].m_tail.m_nUserData = STRREF_DESC_OPTIONS_GRAPHICS;
    m_aButtons[4].m_tail.m_nUserData = STRREF_DESC_OPTIONS_SOUND;

    InitControl(m_btnBack,     CExoString("BTN_BACK"),   1, 1, 0, 0, 0);
#if KOTOR1
    for (int i = 0; i < 6; i++) {
        CSWGuiExtent extent = m_aButtons[i].m_extent;
        extent.y += 20;
        CSWGuiControl* pButton = &m_aButtons[i];
        pButton->SetExtent(extent);
    }
#endif
#if !KOTOR1
    InitControl(m_aBarLabels[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
#endif

    // The description box is a single pre-seated row, so it still scrolls: arrow keys inside it
    // play the scroll sound. 1, not the ctor default of -1 ("silent").
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    StopLoadFromLayout();
#if KOTOR1
    ScaleExtentForResolution();
#endif

    // The back button takes the base panel's virtual B-button handler, and it is the one
    // control whose bFocusOnHover is cleared.
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;

    // Stand the description row up off the list box's prototype widget.
    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem);
    m_labelDesc.Initialize(pProto->m_extent, pProto, 1.0f);

    // The five page openers, once for the pad's X event and once for its menu event.
    m_aButtons[0].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnGameplayOpt));
    m_aButtons[1].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnFeedbackOpt));
    m_aButtons[2].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnAutopauseOpt));
    m_aButtons[3].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnGraphicsOpt));
    m_aButtons[4].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnSoundOpt));
    m_aButtons[0].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnGameplayOpt));
    m_aButtons[1].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnFeedbackOpt));
    m_aButtons[2].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnAutopauseOpt));
    m_aButtons[3].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnGraphicsOpt));
    m_aButtons[4].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnSoundOpt));

    m_aButtons[0].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnEnterSlot));
    m_aButtons[0].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    m_aButtons[1].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnEnterSlot));
    m_aButtons[1].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    m_aButtons[2].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnEnterSlot));
    m_aButtons[2].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    m_aButtons[3].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnEnterSlot));
    m_aButtons[3].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    m_aButtons[4].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnEnterSlot));
    m_aButtons[4].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    m_aButtons[5].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::OnEnterSlot));
    m_aButtons[5].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    SetActiveControl(&m_aButtons[0], 0);
#if KOTOR1
    IosAddButtonIcon(&m_btnBack, 'b');
#else
    AspyrAddButtonIcon(&m_btnBack, 'b');
#endif
#ifdef K2_QOL_CONTROLLER
    K2QolAddControllerButton();
#endif
}

void CSWGuiOptionsMain::SetActiveButton(int nIndex) {
    if (static_cast<unsigned int>(nIndex) <= 5u)
        SetActiveControl(&m_aButtons[nIndex], 0);
}

void CSWGuiOptionsMain::HandleInputEvent(int type, int val) {
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
#if KOTOR1
            // K1: no main-menu idle timer; no panel chain, one stamp of state 4.
            m_pGuiManager->PopModalPanel();
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
            g_pAppManager->m_pClientExoApp->EnableMainMenuIdleTimer();
            m_pGuiManager->PopModalPanel();
            CSWGuiPanel* pPanel = this;
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel);
#endif
            g_pAppManager->m_pClientExoApp->SaveOptions();
            break;
        }
        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;
        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;
        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiOptionsMain::ClosePanel() {
    g_pAppManager->m_pClientExoApp->SaveOptions();
}

void CSWGuiOptionsMain::OnEnterSlot(CSWGuiControl* pControl) {
    SetDescription(pControl->m_tail.m_nUserData);
}

void CSWGuiOptionsMain::SetDescription(uint64_t nDescription)
{
#if KOTOR1
    // K1: the graphics row's description is not read from the TLK but set from a hardcoded
    // Latin-1 list per ASLPlat_GetCurrentLanguage.
    if (nDescription == STRREF_DESC_OPTIONS_GRAPHICS) {
        switch (ASLPlat_GetCurrentLanguage()) {
        case ASL_LANGUAGE_FRENCH:
            m_labelDesc.m_text.m_params.SetText(
                CExoString("Luminosit\xe9\nOmbres\nHerbes\nGraphismes qualit\xe9 sup\xe9rieure\n"));
            break;
        case ASL_LANGUAGE_ITALIAN:
            m_labelDesc.m_text.m_params.SetText(
                CExoString("Luminosit\xe0\nOmbre\nErba\nGrafica di alta qualit\xe0\n"));
            break;
        case ASL_LANGUAGE_GERMAN:
            m_labelDesc.m_text.m_params.SetText(
                CExoString("Helligkeit\nSchatten\nGras\nHohe Graphikqualit\xe4t\n"));
            break;
        case ASL_LANGUAGE_SPANISH:
            m_labelDesc.m_text.m_params.SetText(
                CExoString("Brillo\nSombras\nHierba\nGr\xe1" "ficos de alta calidad\n"));
            break;
        default:
            m_labelDesc.m_text.m_params.SetText(
                CExoString("Brightness\nShadows\nGrass\nHigh quality graphics\n"));
            break;
        }
    } else {
        m_labelDesc.m_text.m_params.SetStrRef(nDescription);
    }
#else
    m_labelDesc.m_text.m_params.SetStrRef(nDescription);
#endif

    CExoString sText(m_labelDesc.m_text.m_params.m_sText);
    m_listBoxDesc.ClearItems();

    CSWGuiText* pText = &m_labelDesc.m_text;

    CSWGuiExtent extent = m_labelDesc.m_extent;
#if KOTOR1
    if (sText.GetLength() >= 1)
#else
    if (sText.m_sString && static_cast<int>(strlen(sText.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_labelDesc.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_labelDesc };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_labelDesc, 0);
}

void CSWGuiOptionsMain::OnGameplayOpt(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiInGameGameplay* pPanel = new CSWGuiInGameGameplay(m_pGuiManager);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiOptionsMain::OnFeedbackOpt(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiOptionsFeedback* pPanel = new CSWGuiOptionsFeedback(m_pGuiManager);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiOptionsMain::OnAutopauseOpt(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiInGameAutoPause* pPanel = new CSWGuiInGameAutoPause(m_pGuiManager);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiOptionsMain::OnGraphicsOpt(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiOptionsGraphics* pPanel = new CSWGuiOptionsGraphics(m_pGuiManager, 0);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiOptionsMain::OnSoundOpt(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiOptionsSound* pPanel = new CSWGuiOptionsSound(m_pGuiManager);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}




extern CExoSound* g_pExoSound;

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

CSWGuiOptionsSound::CSWGuiOptionsSound(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: layout "optsound"; BTN_ADVANCED is bound from the layout; pad navigation wraps
    // between the first and last slider; iOS button glyphs last.
    StartLoadFromLayout(CResRef("optsound"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("optsound_p"), 1, 1, 1);
#endif
    InitControl(m_sliderMusic, CExoString("SLI_MUSIC"), 1, 1, 0, 0, 0);
    InitControl(m_sliderVO, CExoString("SLI_VO"), 1, 1, 0, 0, 0);
    InitControl(m_sliderFX, CExoString("SLI_FX"), 1, 1, 0, 0, 0);
    InitControl(m_sliderMovie, CExoString("SLI_MOVIE"), 1, 1, 0, 0, 0);
    InitControl(m_labelMusic, CExoString("LBL_MUSIC"), 1, 1, 0, 0, 0);
    // LBL_VO targets the member declared third and LBL_FX the one declared second; that
    // asymmetry is deliberate. Do not "fix" it.
    InitControl(m_labelVO, CExoString("LBL_VO"), 1, 1, 0, 0, 0);
    InitControl(m_labelFX, CExoString("LBL_FX"), 1, 1, 0, 0, 0);
    InitControl(m_labelMovie, CExoString("LBL_MOVIE"), 1, 1, 0, 0, 0);
#if KOTOR1
    InitControl(m_btnAdvanced, CExoString("BTN_ADVANCED"), 1, 1, 0, 0, 0);
#endif
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aLabelBar[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
#endif

    m_sliderMusic.m_tail.m_nUserData = STRREF_DESC_MUSIC_VOLUME;
    m_sliderVO.m_tail.m_nUserData = STRREF_DESC_VOICE_OVER_VOLUME;
    m_sliderFX.m_tail.m_nUserData = STRREF_DESC_SOUND_EFFECTS_VOLUME;
    m_sliderMovie.m_tail.m_nUserData = STRREF_DESC_MOVIE_VOLUME;
#if KOTOR1
    m_btnAdvanced.m_tail.m_nUserData = STRREF_DESC_ADVANCED_SOUND;
#endif
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;

    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnDefault, CExoString("BTN_DEFAULT"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

#if KOTOR1
    ScaleExtentForResolution();
#elif !defined(K2_UI_PC)   // pc: the layout's width holds the caption (no pad glyph beside it)
    CSWGuiControl* pDefault = &m_btnDefault;
    CSWGuiExtent extentDefault = m_btnDefault.m_extent;
    int nIdealWidth = extentDefault.h * 3;
    CSWGuiExtent extentIdeal = m_btnDefault.m_text.GetIdealWidthAndHeight();
    nIdealWidth += extentIdeal.w;
    extentDefault.w = (extentDefault.w > nIdealWidth) ? extentDefault.w : nIdealWidth;
    pDefault->SetExtent(extentDefault);
#endif

    // The description listbox's single row is cloned from the listbox's prototype item.
    m_labelDesc.Initialize(m_listBoxDesc.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem), 1.0f);

    m_sliderMusic.SetCurValue(50);
    m_sliderVO.SetCurValue(50);
    m_sliderFX.SetCurValue(50);
    m_sliderMovie.SetCurValue(50);

    // The eight slider events, grouped by event id (four sliders each).
    m_sliderMusic.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnEnterSlider));
    m_sliderVO.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnEnterSlider));
    m_sliderFX.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnEnterSlider));
    m_sliderMovie.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnEnterSlider));

    m_sliderMusic.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnExitSlider));
    m_sliderVO.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnExitSlider));
    m_sliderFX.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnExitSlider));
    m_sliderMovie.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnExitSlider));

    m_sliderMusic.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderVO.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderFX.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderMovie.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));

    m_sliderMusic.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderVO.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderFX.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderMovie.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));

    m_sliderMusic.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderVO.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderFX.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderMovie.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));

    m_sliderMusic.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderVO.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderFX.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderMovie.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));

    m_sliderMusic.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnDefault));
    m_sliderVO.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnDefault));
    m_sliderFX.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnDefault));
    m_sliderMovie.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnDefault));

    m_sliderMusic.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderVO.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderFX.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));
    m_sliderMovie.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnMoveVolumeSlider));

    m_btnAdvanced.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnAdvanced));

    SetUpOptions();

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsSound::OnDefault));

#if KOTOR1
    SetActiveControl(&m_sliderMusic, 0);

    m_sliderMovie.SetMoveToControl(2, &m_sliderMusic);
    m_sliderMusic.SetMoveToControl(0, &m_sliderMovie);

    IosAddButtonIcon(&m_btnDefault, 'y');
    IosAddButtonIcon(&m_btnBack, 'b');
#else
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');

    SetActiveControl(&m_sliderMusic, 0);
#endif
}

void CSWGuiOptionsSound::OnEnterSlider(CSWGuiControl* pControl)
{
    SetDescription(pControl->m_tail.m_nUserData);
    static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_imageThumb.m_params.bHilight = 1;
}

void CSWGuiOptionsSound::OnExitSlider(CSWGuiControl* pControl)
{
    static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_imageThumb.m_params.bHilight = 0;
}

void CSWGuiOptionsSound::OnMoveVolumeSlider(CSWGuiControl* pControl)
{
    int nMax = static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_nMaxValue;
    int nCur = static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_nCurValue;
    float fVolume = static_cast<float>(nCur) / static_cast<float>(nMax) + 0.0f;
    if (&m_sliderMusic == pControl) {
        g_pExoSound->SetMusicVolume(fVolume, 0);
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMusicVolume = static_cast<unsigned char>(nCur);
    } else if (&m_sliderVO == pControl) {
        g_pExoSound->SetDialogVolume(fVolume);
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nVoiceVolume = static_cast<unsigned char>(nCur);
    } else if (&m_sliderFX == pControl) {
        g_pExoSound->SetSoundEffectVolume(fVolume);
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nSoundEffectsVolume = static_cast<unsigned char>(nCur);
    } else if (&m_sliderMovie == pControl) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMovieVolume = static_cast<unsigned char>(nCur);
    }
}

void CSWGuiOptionsSound::OnDefault(CSWGuiControl* pControl)
{
#if KOTOR1
    K2_UNUSED(pControl);
    // K1: BTN_DEFAULT forwards to the panel's own restore-defaults input arm.
    HandleInputEvent(INPUTEVENT_GUI_Y, 1);
#else
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultSoundOptions();
        SetUpOptions();
    }
#endif
}

void CSWGuiOptionsSound::OnAdvanced(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CSWGuiOptionsSoundAdvanced* pAdvanced = new CSWGuiOptionsSoundAdvanced(m_pGuiManager);
#if !KOTOR1
        pAdvanced->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pAdvanced, 3, 1);
    }
}

void CSWGuiOptionsSound::SetUpOptions()
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions == 0)
        return;

    m_sliderMusic.SetCurValue(pOptions->m_nMusicVolume);
    m_sliderVO.SetCurValue(pOptions->m_nVoiceVolume);
    m_sliderFX.SetCurValue(pOptions->m_nSoundEffectsVolume);
    m_sliderMovie.SetCurValue(pOptions->m_nMovieVolume);

    OnMoveVolumeSlider(&m_sliderMusic);
    OnMoveVolumeSlider(&m_sliderVO);
    OnMoveVolumeSlider(&m_sliderFX);
    OnMoveVolumeSlider(&m_sliderMovie);
}

void CSWGuiOptionsSound::HandleInputEvent(int type, int val)
{
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
#if KOTOR1
            // K1: no panel chain; one stamp of state 4 (bits 8-10).
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
            CSWGuiPanel* pPanel = this;
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel);
#endif
            break;
        }
        case INPUTEVENT_GUI_Y:
#if KOTOR1
            // K1: no sound, no BTN_DEFAULT arg; the arm returns without the base call.
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultSoundOptions();
            SetUpOptions();
            return;
#else
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultSoundOptions();
            SetUpOptions();
            break;
#endif
        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;
        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;
        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiOptionsSound::SetDescription(uint64_t nStrRef)
{
    m_labelDesc.m_text.m_params.SetStrRef(nStrRef);

    CExoString sText(m_labelDesc.m_text.m_params.m_sText);
    m_listBoxDesc.ClearItems();

    CSWGuiText* pText = &m_labelDesc.m_text;

    CSWGuiExtent extent = m_labelDesc.m_extent;
#if KOTOR1
    if (sText.GetLength() >= 1)
#else
    if (sText.m_sString && static_cast<int>(strlen(sText.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_labelDesc.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_labelDesc };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_labelDesc, 0);
}

void CSWGuiOptionsSound::OnEnterDefault(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    SetDescription(STRREF_DESC_DEFAULT_BRIGHTNESS);
}




void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

float GetResolutionTextScalar();


void ReInitAurora(unsigned int nWidth, unsigned int nHeight, unsigned char nBpp,
                  int nFullscreen, int bAllow);

void AurDisableVertexBufferObjectsWriteOnly();
void AurEnableVertexBufferObjectsWriteOnly();

extern int g_nDesiredStretch_Mac;

CSWGuiOptionsResolution::CSWGuiOptionsResolution(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: layout "optresolution"; the mode filter is CClientExoApp::IsValidResolution (no
    // desktop-size test); refresh-rate rows need >= 60 Hz, and <= 85 Hz unless high monitor
    // frequencies are allowed; rows are not rescaled, duplicates are not filtered, no glyphs.
    StartLoadFromLayout(CResRef("optresolution"), 1, 1, 1);
    InitControl(m_labelResolution, CExoString("LBL_RESOLUTION"), 1, 1, 0, 0, 0);
    InitControl(m_listBox, CExoString("LB_RESOLUTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_btnOk, CExoString("BTN_OK"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsResolution::OnResolutionChosen));
    m_btnOk.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_listBox.bSelectOnLeftClick = 1;
    SetActiveControl(&m_listBox, 0);

    List<CSWGuiButton*> buttons;
    CExoString sCaption;
    PortDevMode devMode = PortDevMode();
    devMode.m_size = sizeof(PortDevMode);

    if (m_listBox.m_pProtoItem != 0) {
        CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem->AsButton());
        unsigned int nMode = 0;
        CSWGuiButton* pSelected = 0;
        CSWGuiButton* pButton = 0;
#ifndef K2_UI_TOUCH
        // pc / xbox: the PC prototype's height is in the 480-line layout's units (a row of
        // the unscaled height is shorter than the caption at a tall buffer, which draws nothing).
        CSWGuiExtent extentRow = pTemplate->m_extent;
        extentRow.h = static_cast<int>(static_cast<float>(g_SDL_mode.h) / 480.0f * extentRow.h);
#else
        const CSWGuiExtent& extentRow = pTemplate->m_extent;
#endif

        while (EnumDisplaySettingsA(0, nMode, &devMode) != 0) {
            if (devMode.m_displayFrequency < 2) {
                if (devMode.m_bitsPerPel == 32 &&
                    g_pAppManager->m_pClientExoApp->IsValidResolution(devMode.m_pelsWidth, devMode.m_pelsHeight)) {
                    pButton = new CSWGuiButton;
                    pButton->Initialize(extentRow, pTemplate);
                    sCaption.Format(const_cast<char*>("%d x %d"), devMode.m_pelsWidth, devMode.m_pelsHeight);
                    pButton->m_text.m_params.SetText(sCaption);
                    pButton->m_tail.m_nUserData = nMode;
                    pButton->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsResolution::OnResolutionChosen));
                    buttons.Add(pButton);
                    if (devMode.m_pelsWidth == static_cast<unsigned int>(g_nScreenWidth) &&
                        devMode.m_pelsHeight == static_cast<unsigned int>(g_nScreenHeight))
                        pSelected = pButton;
                }
            } else {
                if (devMode.m_displayFrequency >= 60 && devMode.m_bitsPerPel == 32 &&
                    g_pAppManager->m_pClientExoApp->IsValidResolution(devMode.m_pelsWidth, devMode.m_pelsHeight) &&
                    (g_pAppManager->m_pClientExoApp->GetAllowHighMonitorFrequency() ||
                     devMode.m_displayFrequency <= 85)) {
                    pButton = new CSWGuiButton;
                    pButton->Initialize(extentRow, pTemplate);
                    sCaption.Format(const_cast<char*>("%d x %d @ %d Hz"), devMode.m_pelsWidth,
                                    devMode.m_pelsHeight, devMode.m_displayFrequency);
                    pButton->m_text.m_params.SetText(sCaption);
                    pButton->m_tail.m_nUserData = nMode;
                    pButton->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsResolution::OnResolutionChosen));
                    buttons.Add(pButton);
                    if (devMode.m_displayFrequency == static_cast<unsigned int>(g_nDesiredFullScreenRefreshRate) &&
                        devMode.m_pelsWidth == static_cast<unsigned int>(g_nScreenWidth) &&
                        devMode.m_pelsHeight == static_cast<unsigned int>(g_nScreenHeight))
                        pSelected = pButton;
                }
            }
            nMode++;
        }

        m_listBox.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(buttons), 0, 1, 0);
        if (pSelected != 0)
            m_listBox.SetSelectedControl(pSelected, 0);
    }

    m_bResolutionChanged = 0;
#else
    StartLoadFromLayout(CResRef("optresolution_p"), 1, 1, 1);
    InitControl(m_labelResolution, CExoString("LBL_RESOLUTION"), 1, 1, 0, 0, 0);
    InitControl(m_listBox, CExoString("LB_RESOLUTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_btnOk, CExoString("BTN_OK"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsResolution::OnResolutionChosen));
    m_btnOk.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_listBox.bSelectOnLeftClick = 1;
    SetActiveControl(&m_listBox, 0);

    List<CSWGuiButton*> buttons;
    CExoString sCaption;
    int nDesktopWidth = 0;
    int nDesktopHeight = 0;
    PortDevMode devMode = PortDevMode();
    devMode.m_size = sizeof(PortDevMode);
    CAuroraInterface::GetDesktopResolution(nDesktopWidth, nDesktopHeight);

    if (m_listBox.m_pProtoItem != 0) {
        CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem->AsButton());
        unsigned int nMode = 0;
        CSWGuiButton* pSelected = 0;
        CSWGuiButton* pButton = 0;

        while (EnumDisplaySettingsA(0, nMode, &devMode) != 0) {
            if (devMode.m_displayFrequency < 2) {
                if (devMode.m_bitsPerPel == 32 &&
                    CAuroraInterface::IsWithinDesktopResolution(devMode.m_pelsWidth, devMode.m_pelsHeight) &&
                    (CAuroraInterface::IsSupportedResolution(devMode.m_pelsWidth, devMode.m_pelsHeight) ||
                     (devMode.m_pelsWidth == static_cast<unsigned int>(nDesktopWidth) &&
                      devMode.m_pelsHeight == static_cast<unsigned int>(nDesktopHeight)))) {
                    pButton = new CSWGuiButton;
                    CSWGuiExtent extent = pTemplate->m_extent;
                    extent.h = static_cast<int>(GetResolutionTextScalar() * static_cast<float>(extent.h));
                    pButton->Initialize(extent, pTemplate);
                    sCaption.Format(const_cast<char*>("%d x %d"), devMode.m_pelsWidth, devMode.m_pelsHeight);
                    pButton->m_text.m_params.SetText(sCaption);
                    pButton->m_tail.m_nUserData = nMode;
                    pButton->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsResolution::OnResolutionChosen));

                    for (int64_t i = 0; i < buttons.m_count; i++) {
                        if (strcmp(buttons.m_pData[i]->m_text.m_params.m_sText.CStr(), sCaption.CStr()) == 0) {
                            delete pButton;
                            goto nextMode;
                        }
                    }

                    buttons.Add(pButton);
                    if (devMode.m_pelsWidth == static_cast<unsigned int>(g_nScreenWidth) &&
                        devMode.m_pelsHeight == static_cast<unsigned int>(g_nScreenHeight))
                        pSelected = pButton;
                }
            } else {
                if (devMode.m_bitsPerPel == 32 &&
                    CAuroraInterface::IsWithinDesktopResolution(devMode.m_pelsWidth, devMode.m_pelsHeight) &&
                    (CAuroraInterface::IsSupportedResolution(devMode.m_pelsWidth, devMode.m_pelsHeight) ||
                     (devMode.m_pelsWidth == static_cast<unsigned int>(nDesktopWidth) &&
                      devMode.m_pelsHeight == static_cast<unsigned int>(nDesktopHeight)))) {
                    if ((devMode.m_displayFrequency >= 60 && devMode.m_pelsWidth < 2000) ||
                        (devMode.m_displayFrequency >= 30 && devMode.m_pelsWidth >= 2000)) {
                        pButton = new CSWGuiButton;
                        CSWGuiExtent extent = pTemplate->m_extent;
                        extent.h = static_cast<int>(GetResolutionTextScalar() * static_cast<float>(extent.h));
                        pButton->Initialize(extent, pTemplate);
                        sCaption.Format(const_cast<char*>("%d x %d @ %d Hz"), devMode.m_pelsWidth,
                                        devMode.m_pelsHeight, devMode.m_displayFrequency);
                        pButton->m_text.m_params.SetText(sCaption);
                        pButton->m_tail.m_nUserData = nMode;
                        pButton->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsResolution::OnResolutionChosen));

                        for (int64_t i = 0; i < buttons.m_count; i++) {
                            if (strcmp(buttons.m_pData[i]->m_text.m_params.m_sText.CStr(), sCaption.CStr()) == 0) {
                                delete pButton;
                                goto nextMode;
                            }
                        }

                        buttons.Add(pButton);
                        if (devMode.m_displayFrequency == static_cast<unsigned int>(g_nDesiredFullScreenRefreshRate) &&
                            devMode.m_pelsWidth == static_cast<unsigned int>(g_nScreenWidth) &&
                            devMode.m_pelsHeight == static_cast<unsigned int>(g_nScreenHeight))
                            pSelected = pButton;
                    }
                }
            }
        nextMode:
            nMode++;
        }

        m_listBox.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(buttons), 0, 1, 0);
        if (pSelected != 0)
            m_listBox.SetSelectedControl(pSelected, 0);
    }

    m_bResolutionChanged = 0;
    AspyrAddButtonIcon(&m_btnOk, 'a');
    AspyrAddButtonIcon(&m_btnCancel, 'b');
#endif
}

void CSWGuiOptionsResolution::HandleInputEvent(int type, int val)
{
#if KOTOR1
    // K1: an accept arm that commits through OnResolutionChosen(0); the back arm stamps
    // state 4 on this panel only and re-reads the VBO key from "swkotor.ini" via AsInt.
    if (val != 0) {
        if (type == INPUTEVENT_GUI_BACK || type == INPUTEVENT_GUI_B) {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;

            if (m_bResolutionChanged) {
                CExoIni ini;
                CExoString sValue;

                ReInitAurora(g_nScreenWidth, g_nScreenHeight, g_nBitsPerPixel, g_pAurora->FullScreen(), 1);

                if (ini.ReadIniEntry(sValue, CExoString("swkotor.ini"),
                                     CExoString("Graphics Options"),
                                     CExoString("Disable Write-Only VBO"))) {
                    if (sValue.AsINT() == 1)
                        AurDisableVertexBufferObjectsWriteOnly();
                    else
                        AurEnableVertexBufferObjectsWriteOnly();
                }
                m_bResolutionChanged = 0;
            }
        }
        // pc / xbox: A reaches the focused control alone (a row or OK applies, Cancel backs
        // out). This arm applied the mode and then the focused row applied it again, which
        // popped the Graphics page along with the list.
#ifdef K2_UI_TOUCH
        else if (type == INPUTEVENT_GUI_A) {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            OnResolutionChosen(0);
        }
#endif
    }
    CSWGuiPanel::HandleInputEvent(type, val);
#else
    if (val != 0 && (type == INPUTEVENT_GUI_BACK || type == INPUTEVENT_GUI_B)) {
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        m_pGuiManager->PopModalPanel();
        CSWGuiPanel* pPanel = this;
        do {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel);

        if (m_bResolutionChanged) {
            CExoIni ini;
            CExoString sValue;

            ReInitAurora(g_nScreenWidth, g_nScreenHeight, g_nBitsPerPixel, g_pAurora->FullScreen(), 1);

            if (ini.ReadIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                                 CExoString("Graphics Options"),
                                 CExoString("Disable Write-Only VBO"))) {
                if (sValue.m_sString && atoi(sValue.m_sString) == 1)
                    AurDisableVertexBufferObjectsWriteOnly();
                else
                    AurEnableVertexBufferObjectsWriteOnly();
            }
            m_bResolutionChanged = 0;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
#endif
}

void CSWGuiOptionsResolution::OnResolutionChosen(CSWGuiControl* pControl)
{
    if (pControl == 0 || pControl->m_tail.nCurrentEventArg != 0) {
        unsigned int nMode = static_cast<unsigned int>(m_listBox.GetSelectedControl()->m_tail.m_nUserData);

        PortDevMode devMode = PortDevMode();
        devMode.m_size = sizeof(PortDevMode);
        if (EnumDisplaySettingsA(0, nMode, &devMode) != 0)
            g_nDesiredFullScreenRefreshRate = devMode.m_displayFrequency;

#if KOTOR1 && defined(K2_QOL_FULLSCREEN)
        // desktop K1: the mode is a window size, kept in swkotor.ini. From the main menu it applies
        // at once (K1QolApplyDisplay); in a game, at the next launch -- KOTOR II refuses the change
        // there, and K1's in-game interface is built for the window it has.
        if (EnumDisplaySettingsA(0, nMode, &devMode) != 0) {
            CExoString sValue;
            CExoIni ini;
            sValue.Format(const_cast<char*>("%d"), devMode.m_pelsWidth);
            ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Display Options"),
                              CExoString("WindowWidth"));
            sValue.Format(const_cast<char*>("%d"), devMode.m_pelsHeight);
            ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Display Options"),
                              CExoString("WindowHeight"));
            if (!s_bK1ResolutionInGame) {
                K1QolApplyDisplay(m_pGuiManager, 3, 0);   // this list, Graphics, Options
                return;
            }
        }
        m_pGuiManager->PopModalPanel();
        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#elif KOTOR1
        // K1: three keys into "swkotor.ini" (no Stretched), then one PopModalPanel and a
        // single stamp of state 4 on this panel; no rebuild of the options stack.
        if (g_pAppManager->m_pClientExoApp->SetVideoMode(nMode) != 0) {
            CExoString sValue;
            CExoIni ini;

            sValue.Format(const_cast<char*>("%d"), g_nScreenWidth);
            ini.WriteIniEntry(sValue, CExoString("swkotor.ini"),
                              CExoString("Graphics Options"), CExoString("Width"));

            sValue.Format(const_cast<char*>("%d"), g_nScreenHeight);
            ini.WriteIniEntry(sValue, CExoString("swkotor.ini"),
                              CExoString("Graphics Options"), CExoString("Height"));

            sValue.Format(const_cast<char*>("%d"), g_nDesiredFullScreenRefreshRate);
            ini.WriteIniEntry(sValue, CExoString("swkotor.ini"),
                              CExoString("Graphics Options"), CExoString("RefreshRate"));
        }

        m_pGuiManager->PopModalPanel();
        m_bResolutionChanged = 1;
        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        g_pGuiMan->ReloadToolTipPanel();
#else
        if (g_pAppManager->m_pClientExoApp->SetVideoMode(nMode) != 0) {
            CExoString sValue;
            CExoIni ini;

            sValue.Format(const_cast<char*>("%d"), g_nScreenWidth);
            ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                              CExoString("Graphics Options"), CExoString("Width"));

            sValue.Format(const_cast<char*>("%d"), g_nScreenHeight);
            ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                              CExoString("Graphics Options"), CExoString("Height"));

            sValue.Format(const_cast<char*>("%d"), g_nDesiredFullScreenRefreshRate);
            ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                              CExoString("Graphics Options"), CExoString("RefreshRate"));

            sValue.Format(const_cast<char*>("%d"), g_nDesiredStretch_Mac);
            ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"),
                              CExoString("Graphics Options"), CExoString("Stretched"));
        }

        m_pGuiManager->PopModalPanel();
        CSWGuiPanel* pPanel = this;
        do {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel);
        pPanel = m_pGuiManager->PopModalPanel();
        do {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel);
        pPanel = m_pGuiManager->PopModalPanel();
        do {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel);

        CSWGuiOptionsMain* pMain = new CSWGuiOptionsMain(m_pGuiManager);
#ifdef K2_QOL_FULLSCREEN
        // The rebuilt pages get the border the main menu's Options and OnGraphicsOpt give them;
        // without it the screen around the 4:3 frame is left black.
        pMain->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pMain, 3, 1);
        CSWGuiOptionsGraphics* pGraphics = new CSWGuiOptionsGraphics(m_pGuiManager, 0);
#ifdef K2_QOL_FULLSCREEN
        pGraphics->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pGraphics, 3, 1);

        m_bResolutionChanged = 1;
#ifndef K2_RESOLUTION_BUTTON
        // Stretches every panel to the screen's width; the PC panels above were built at the new
        // size, and a stretched one overruns it by a pixel and CSWGuiPanel::Draw culls it.
        g_pGuiMan->ReloadPanels(g_nScreenWidth, g_nScreenHeight);
#endif
        g_pGuiMan->ReloadToolTipPanel();
#endif
    }
}


#ifdef K2_QOL_FULLSCREEN
#if KOTOR1
extern "C" void k2host_set_fullscreen_setting(int bOn);
#else
extern "C" void k2host_fullscreen_mode(int bOn, int* pW, int* pH);
#endif

static const char* K2QolFullscreenDescription(int bInGame)
{
#if KOTOR1
    if (bInGame)
        return "Fullscreen.\n\nOn: the game fills the screen at the display's full resolution. Off: "
               "a window at the Screen Resolution size.\n\nIn a game it takes effect the next time the "
               "game starts; from the main menu's Options, at once.";
    return "Fullscreen.\n\nOn: the game fills the screen at the display's full resolution. Off: a "
           "window at the Screen Resolution size.";
#else
    if (bInGame)
        return "Fullscreen.\n\nOn: the game fills the screen at the display's full resolution. Off: "
               "a window at the last windowed size.\n\nChange it from the main menu's Options.";
    return "Fullscreen.\n\nOn: the game fills the screen at the display's full resolution. Off: a "
           "window at the last windowed size.\n\nScreen Resolution also lists the display's own "
           "sizes, the full resolution and the screen's size in points.";
#endif
}

// Adds the desktop builds' Fullscreen row, one row under Grass (K1: under its frame-buffer row),
// cloned from CB_GRASS as the page clones its other rows.
void CSWGuiOptionsGraphics::K2QolAddFullscreenRow()
{
#if KOTOR1
    int nStep = m_cbGrass.m_extent.y - m_cbShadows.m_extent.y;
    m_cbFullscreen = m_cbGrass;
    CSWGuiExtent extent = m_cbGrass.m_extent;
    extent.y += nStep * 3;
    m_cbFullscreen.SetExtent(extent);
    AddControl(&m_cbFullscreen);
    // Advanced Options sits one row under the frame-buffer row, so the five checkbox rows share
    // the column from Shadows down to it.
    CSWGuiOptionsCheckbox* apRows[5] = { &m_cbShadows, &m_cbGrass, &m_cbForceSpeed,
                                         &m_cbFramerateLock, &m_cbFullscreen };
    int nTop = m_cbShadows.m_extent.y;
    int nPitch = (m_btnAdvanced.m_extent.y - nTop) / 5;
    if (nPitch > 0 && nPitch < nStep) {
        for (int i = 0; i < 5; i++) {
            extent = apRows[i]->m_extent;
            extent.y = nTop + nPitch * i;
            if (extent.h > nPitch)
                extent.h = nPitch;
            apRows[i]->SetExtent(extent);
        }
    }
#else
    int nStep = m_cbGrass.m_originalExtent.y - m_cbShadows.m_originalExtent.y;
    m_cbFullscreen = m_cbGrass;
    m_cbFullscreen.m_pContainer = this;
    m_cbFullscreen.m_tail.nControlId = -1;   // appended, not swapped into CB_GRASS's slot
    m_cbFullscreen.m_extent = m_cbGrass.m_originalExtent;
    m_cbFullscreen.m_extent.y += nStep;
    InitControl(m_cbFullscreen, 1, 1, CExoString(static_cast<char*>(0)), 0);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkFitText(&m_cbFullscreen.m_text);   // built by hand, so CSWGuiButton::Load never marked it
#endif
#endif
    m_cbFullscreen.SetEnabled(1);
    m_cbFullscreen.m_text.m_params.SetText(CExoString("Fullscreen"));
    m_cbFullscreen.m_tail.m_nUserData = kQolDescFullscreen;
    m_cbFullscreen.m_tail.bFocusOnHover = 0;
    m_cbFullscreen.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnEnterSlot));
    m_cbFullscreen.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsGraphics::OnFullscreen));
    m_cbFullscreen.bToggledOn = k2host_fullscreen() != 0;
}

#if KOTOR1
// desktop K1, from the main menu: the window for the settings (k1sdl.cpp: fullscreen or the Screen
// Resolution size; g_SDL_mode becomes its pixels), then what SetVideoMode does on K1 (Aurora
// re-initialized at that size, the GUI manager resized, the main menu reloaded), then Options and
// Graphics rebuilt at the new size, as KOTOR II's OnFullscreen does: K1 scales a panel's layout when
// it is built (CSWGuiObject::ScaleExtentForResolution).
static void K1QolApplyDisplay(CSWGuiManager* pGuiManager, int nPanels, int bFocusFullscreen)
{
    k2host_k1_apply_display();
    g_pAppManager->m_pClientExoApp->SetVideoMode(0);
    K1QolResizeSceneBuffers();
    for (int nPanel = 0; nPanel < nPanels; nPanel++) {   // K1 panels have no border chain
        if (CSWGuiPanel* pPanel = pGuiManager->PopModalPanel())
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
    }
    CSWGuiOptionsMain* pMain = new CSWGuiOptionsMain(pGuiManager);
    pGuiManager->AddPanel(pMain, 3, 1);
    CSWGuiOptionsGraphics* pGraphics = new CSWGuiOptionsGraphics(pGuiManager, 0);
    pGuiManager->AddPanel(pGraphics, 3, 1);
    pGraphics->SetActiveControl(bFocusFullscreen ? static_cast<CSWGuiControl*>(&pGraphics->m_cbFullscreen)
                                                 : static_cast<CSWGuiControl*>(&pGraphics->m_btnResolution), 0);
    g_pGuiMan->ReloadToolTipPanel();
}
#endif

// The check has already flipped (CSWGuiButtonToggle::HandleInputEvent).
void CSWGuiOptionsGraphics::OnFullscreen(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg)
        return;
    int bOn = m_cbFullscreen.bToggledOn != 0;
#if KOTOR1
    // K1 lays its GUI out once for the window it starts with (CSWGuiObject::ScaleExtentForResolution
    // keeps its first scale), so the window changes at the next launch; k1sdl.cpp reads the key.
    CExoIni ini;
    CExoString sValue;
    sValue.Format(const_cast<char*>("%d"), bOn);
    ini.WriteIniEntry(sValue, CExoString("swkotor.ini"), CExoString("Display Options"),
                      CExoString("FullScreen"));
    k2host_set_fullscreen_setting(bOn);
    if (!K1QolGameLoaded())
        K1QolApplyDisplay(m_pGuiManager, 2, 1);   // from the main menu: at once (this page, Options)
#else
    if (m_bInGame) {
        // OnResolution refuses in game, since the display mode is set from the main menu.
        m_cbFullscreen.bToggledOn = k2host_fullscreen() != 0;
        CSWGuiMessageBox* pBox = static_cast<CSWGuiMessageBox*>(
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox);
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(static_cast<uint64_t>(STRREF_RESOLUTION_MAIN_MENU_ONLY));
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    // The host's mode for the choice (shim_display.cpp), then what OnResolutionChosen does with a
    // picked mode: set it, record it in the ini, and rebuild Options and this page at the new size.
    int nWidth = 0, nHeight = 0;
    k2host_fullscreen_mode(bOn, &nWidth, &nHeight);
    PortDevMode devMode = PortDevMode();
    devMode.m_size = sizeof(PortDevMode);
    int nMode = -1;
    for (unsigned int i = 0; EnumDisplaySettingsA(0, i, &devMode) != 0; i++) {
        if (devMode.m_pelsWidth == static_cast<unsigned int>(nWidth) && devMode.m_pelsHeight == static_cast<unsigned int>(nHeight)) {
            nMode = static_cast<int>(i);
            break;
        }
        devMode = PortDevMode();
        devMode.m_size = sizeof(PortDevMode);
    }
    if (nMode < 0) {
        m_cbFullscreen.bToggledOn = k2host_fullscreen() != 0;
        return;
    }
    g_nDesiredFullScreenRefreshRate = devMode.m_displayFrequency;

    if (g_pAppManager->m_pClientExoApp->SetVideoMode(nMode) != 0) {
        CExoString sValue;
        CExoIni ini;
        sValue.Format(const_cast<char*>("%d"), g_nScreenWidth);
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                          CExoString("Width"));
        sValue.Format(const_cast<char*>("%d"), g_nScreenHeight);
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                          CExoString("Height"));
        sValue.Format(const_cast<char*>("%d"), g_nDesiredFullScreenRefreshRate);
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                          CExoString("RefreshRate"));
        sValue.Format(const_cast<char*>("%d"), g_nDesiredStretch_Mac);
        ini.WriteIniEntry(sValue, CExoString(".\\swkotor2.ini"), CExoString("Graphics Options"),
                          CExoString("Stretched"));
    }

    for (int nPanel = 0; nPanel < 2; nPanel++) {   // this page, then Options
        CSWGuiPanel* pPanel = m_pGuiManager->PopModalPanel();
        while (pPanel) {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        }
    }
    CSWGuiOptionsMain* pMain = new CSWGuiOptionsMain(m_pGuiManager);
    pMain->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
    m_pGuiManager->AddPanel(pMain, 3, 1);
    CSWGuiOptionsGraphics* pGraphics = new CSWGuiOptionsGraphics(m_pGuiManager, 0);
    pGraphics->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
    m_pGuiManager->AddPanel(pGraphics, 3, 1);
    pGraphics->SetActiveControl(&pGraphics->m_cbFullscreen, 0);
#ifndef K2_RESOLUTION_BUTTON
    g_pGuiMan->ReloadPanels(g_nScreenWidth, g_nScreenHeight);
#endif
    g_pGuiMan->ReloadToolTipPanel();
#endif
}
#endif
