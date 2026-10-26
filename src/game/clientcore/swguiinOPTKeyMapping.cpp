#include "k2stdint.h"
#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE

#include "SwGuiInOptKeyMapping.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "exoiniinternal.h"
#include "CExoArrayListDtors.h"
#include "CExoArrayListImpl.h"
#include "exoinput.h"
#include "exoinputinternal.h"
#include "exostring.h"
#include "guiingame.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "exotlktable.h"
#include "twodimarray.h"
#include "swguidrawable.h"
#include "swguicontrol.h"
#include "port/sdlapi.h"
#include "swgui.h"
#include "strref.h"

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiInGameOptKeyMappings) == 0x1a60, "CSWGuiInGameOptKeyMappings size drift");
K2_LAYOUT_ASSERT(sizeof(CSWGuiKeyMapButton) == 0x508, "CSWGuiKeyMapButton size drift");
#endif

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

extern CTlkTable* g_pTlkTable;

CSWGuiInGameOptKeyMappings::CSWGuiInGameOptKeyMappings(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    SetCaptureKeyboardMode(0);
    m_nFilterMode = FILTER_MOVE;
    m_pCaptureEvent = 0;

#if KOTOR1
    // K1 has no bar labels and no controller glyphs.
    StartLoadFromLayout(CResRef("OPTKeyMapping"));
    InitControl(m_lblTitle,      CExoString("LBL_Title"),       1);
    InitControl(m_btnDefault,    CExoString("BTN_Default"),     1);
    InitControl(m_btnAccept,     CExoString("BTN_Accept"),      1);
    InitControl(m_btnCancel,     CExoString("BTN_Cancel"),      1);
    InitControl(m_btnFilterMove, CExoString("BTN_Filter_Move"), 1);
    InitControl(m_btnFilterGame, CExoString("BTN_Filter_Game"), 1);
    InitControl(m_btnFilterMini, CExoString("BTN_Filter_Mini"), 1);
    InitControl(m_listBox,       CExoString("LST_EventList"),   1);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_btnFilterMove.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);

    FillListWithEvents();
    FillListBox(m_nFilterMode);

    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnDefaultClick));
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnAcceptClick));
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    m_btnFilterMove.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnFilterMove));
    m_btnFilterMove.AddEvent(SWGUIEVENT_UNFOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::FixFilterColors));
    m_btnFilterMove.m_tail.bFocusOnHover = 0;

    m_btnFilterGame.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnFilterGame));
    m_btnFilterGame.AddEvent(SWGUIEVENT_UNFOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::FixFilterColors));
    m_btnFilterGame.m_tail.bFocusOnHover = 0;

    m_btnFilterMini.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnFilterMini));
    m_btnFilterMini.AddEvent(SWGUIEVENT_UNFOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::FixFilterColors));
    m_btnFilterMini.m_tail.bFocusOnHover = 0;

    SetActiveControl(&m_listBox, 0);
#else
    StartLoadFromLayout(CResRef("OPTKeyMapping_p"), 1, 1, 1);

    InitControl(m_lblTitle,      CExoString("LBL_Title"),       1, 1, 0, 0, 0);
    InitControl(m_btnDefault,    CExoString("BTN_Default"),     1, 1, 0, 0, 0);
    InitControl(m_btnAccept,     CExoString("BTN_Accept"),      1, 1, 0, 0, 0);
    InitControl(m_btnCancel,     CExoString("BTN_Cancel"),      1, 1, 0, 0, 0);
    InitControl(m_btnFilterMove, CExoString("BTN_Filter_Move"), 1, 1, 0, 0, 0);
    InitControl(m_btnFilterGame, CExoString("BTN_Filter_Game"), 1, 1, 0, 0, 0);
    InitControl(m_btnFilterMini, CExoString("BTN_Filter_Mini"), 1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aBars[0],      CExoString("LBL_BAR1"),        1, 1, 0, 0, 0);
    InitControl(m_aBars[1],      CExoString("LBL_BAR2"),        1, 1, 0, 0, 0);
    InitControl(m_aBars[2],      CExoString("LBL_BAR3"),        1, 1, 0, 0, 0);
#endif
    InitControl(m_listBox,       CExoString("LST_EventList"),   1, 1, 0, 0, 0);

    StopLoadFromLayout();

    // The Move filter is the default page, so its caption starts highlighted.
    m_btnFilterMove.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);

    FillListWithEvents();
    FillListBox(m_nFilterMode);

    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnDefaultClick));
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnAcceptClick));
    // Cancel uses the panel's own handler.
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    m_btnFilterMove.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnFilterMove));
    m_btnFilterMove.AddEvent(SWGUIEVENT_UNFOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::FixFilterColors));
    m_btnFilterMove.m_tail.bFocusOnHover = 0;

    m_btnFilterGame.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnFilterGame));
    m_btnFilterGame.AddEvent(SWGUIEVENT_UNFOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::FixFilterColors));
    m_btnFilterGame.m_tail.bFocusOnHover = 0;

    m_btnFilterMini.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::OnFilterMini));
    m_btnFilterMini.AddEvent(SWGUIEVENT_UNFOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptKeyMappings::FixFilterColors));

    AspyrAddButtonIcon(&m_btnAccept, 'a');
    AspyrAddButtonIcon(&m_btnCancel, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');
    m_btnFilterMini.m_tail.bFocusOnHover = 0;

    SetActiveControl(&m_listBox, 0);
#endif
}

void CSWGuiInGameOptKeyMappings::FillListWithEvents()
{
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    m_aOptions.m_count = 0;
    // Two rows per id: the class-2 form and the class-1 form.
    for (int i = 0; i != INPUTEVENT_KEYMAP_AXIS_END - INPUTEVENT_KEYMAP_AXIS_FIRST; i++) {
        CSWGuiKeyMapButton* pButton = CreateOption(i + INPUTEVENT_KEYMAP_AXIS_FIRST, pTemplate, 2);
        if (pButton != 0)
            m_aOptions.Add(pButton);
        pButton = CreateOption(i + INPUTEVENT_KEYMAP_AXIS_FIRST, pTemplate, 1);
        if (pButton != 0)
            m_aOptions.Add(pButton);
    }
    for (int i = 0; i != INPUTEVENT_KEYMAP_END - INPUTEVENT_KEYMAP_FIRST; i++) {
        CSWGuiKeyMapButton* pButton = CreateOption(i + INPUTEVENT_KEYMAP_FIRST, pTemplate, 0);
        if (pButton != 0)
            m_aOptions.Add(pButton);
    }
}

void CSWGuiInGameOptKeyMappings::FillListBox(int nPage)
{
    CExoArrayList<CSWGuiControl*> aPage;
    for (int64_t i = 0; i < static_cast<int64_t>(m_aOptions.m_count); i++) {
        CSWGuiKeyMapButton* pButton = static_cast<CSWGuiKeyMapButton*>(m_aOptions.m_pArray[i]);
        if (pButton->m_nPage != nPage)
            continue;
        // Insertion sort by SortPos: before the first entry that sorts after the new one.
        if (aPage.m_count < 1) {
            aPage.Add(pButton);
        } else {
            int nIndex;
            for (nIndex = 0; nIndex < aPage.m_count; nIndex++) {
                if (static_cast<CSWGuiKeyMapButton*>(aPage.m_pArray[nIndex])->m_nSortPos > pButton->m_nSortPos)
                    break;
            }
            aPage.Insert(pButton, nIndex);
        }
    }

    m_listBox.ClearItems();
    m_listBox.AddControls(aPage, 0, 1, 0);
    m_listBox.bLooping = 0;
}

void CSWGuiInGameOptKeyMappings::OnDefaultClick(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    for (int i = 0; i < m_aOptions.m_count; i++) {
        if (static_cast<CSWGuiKeyMapButton*>(m_aOptions.m_pArray[i])->m_bDisabled == 0)
            static_cast<CSWGuiKeyMapButton*>(m_aOptions.m_pArray[i])->ResetToDefault();
    }
}

void CSWGuiInGameOptKeyMappings::OnAcceptClick(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    for (int i = 0; i < m_aOptions.m_count; i++) {
        CSWGuiKeyMapButton* pButton = static_cast<CSWGuiKeyMapButton*>(m_aOptions.m_pArray[i]);
        if (pButton->m_bDisabled == 0 && pButton->m_bChanged != 0)
            pButton->PostChangeToEvent();
    }
    SetCaptureKeyboardMode(0);
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    m_pGuiManager->PopModalPanel();
    SetToBeDeleted(0);
}

void CSWGuiInGameOptKeyMappings::OnFilterMove(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_nFilterMode = FILTER_MOVE;
    FixFilterColors(0);
    FillListBox(m_nFilterMode);
}

void CSWGuiInGameOptKeyMappings::FixFilterColors(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1 paints the active filter COLOR_YELLOW and the other two STANDARD_BLUE.
    switch (m_nFilterMode) {
    case FILTER_MOVE:
        m_btnFilterMove.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
        if (pControl != 0)
            return;
        m_btnFilterGame.m_text.m_params.SetColor(CGuiInGame::STANDARD_BLUE);
        m_btnFilterMini.m_text.m_params.SetColor(CGuiInGame::STANDARD_BLUE);
        break;
    case FILTER_GAME:
        m_btnFilterGame.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
        if (pControl != 0)
            return;
        m_btnFilterMove.m_text.m_params.SetColor(CGuiInGame::STANDARD_BLUE);
        m_btnFilterMini.m_text.m_params.SetColor(CGuiInGame::STANDARD_BLUE);
        break;
    case FILTER_MINI:
        m_btnFilterMini.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
        if (pControl != 0)
            return;
        m_btnFilterMove.m_text.m_params.SetColor(CGuiInGame::STANDARD_BLUE);
        m_btnFilterGame.m_text.m_params.SetColor(CGuiInGame::STANDARD_BLUE);
        break;
    }
#else
    switch (m_nFilterMode) {
    case FILTER_MOVE:
        m_btnFilterMove.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        if (pControl != 0)
            return;
        m_btnFilterGame.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnFilterMini.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        break;
    case FILTER_GAME:
        m_btnFilterGame.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        if (pControl != 0)
            return;
        m_btnFilterMove.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnFilterMini.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        break;
    case FILTER_MINI:
        m_btnFilterMini.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        if (pControl != 0)
            return;
        m_btnFilterMove.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnFilterGame.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        break;
    }
#endif
}

void CSWGuiInGameOptKeyMappings::OnFilterGame(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_nFilterMode = FILTER_GAME;
    FixFilterColors(0);
    FillListBox(m_nFilterMode);
}

void CSWGuiInGameOptKeyMappings::OnFilterMini(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_nFilterMode = FILTER_MINI;
    FixFilterColors(0);
    FillListBox(m_nFilterMode);
}

CSWGuiInGameOptKeyMappings::~CSWGuiInGameOptKeyMappings()
{
    m_listBox.ClearItems();
    while (m_aOptions.m_count != 0) {
        int nIndex = m_aOptions.m_count - 1;
        if (m_aOptions.m_pArray[nIndex] != 0)
            delete m_aOptions.m_pArray[nIndex];
        m_aOptions.m_pArray[nIndex] = 0;
        m_aOptions.DelIndex(nIndex);
    }
}


void CSWGuiInGameOptKeyMappings::HandleInputEvent(int type, int val)
{
#if KOTOR1
    // K1 handles only the close keys, without the defaults key or the filter cycling.
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            SetCaptureKeyboardMode(0);
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
            SetToBeDeleted(0);
            return;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
#else
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            SetCaptureKeyboardMode(0);
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
            SetToBeDeleted(0);
            return;
        }
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
            OnDefaultClick(0);
            break;
        case INPUTEVENT_GUI_DPAD_LEFT:
        case INPUTEVENT_GUI_DPAD_RIGHT:
            m_nFilterMode = (m_nFilterMode + ((type == INPUTEVENT_GUI_DPAD_LEFT) ? -1 : 1) + 3) % 3;
            SetActiveControl(0, 0);
            FixFilterColors(0);
            FillListBox(m_nFilterMode);
            SetActiveControl(&m_listBox, 0);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
#endif
}

CSWGuiKeyMapButton* CSWGuiInGameOptKeyMappings::CreateOption(int nActionId, CSWGuiButton* pTemplate,
                                                           int nEventClass)
{
    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 0;
    extent.w = 0;
    extent.h = 0;

    CSWGuiKeyMapButton* pButton = new CSWGuiKeyMapButton(this);
    pButton->m_nActionId = nActionId;
    pButton->m_nEventClass = nEventClass;
    pButton->Initialize(extent, pTemplate);
    pButton->m_btnKey.Initialize(extent, pTemplate);

    pButton->m_btnKey.m_text.m_params.SetText(CExoString(nActionId));

    CExoString sActionName = pButton->GetActionId(nActionId, nEventClass);

    // nReMappable and nForceDisplay are filled in by the 2DA lookups below.
    int nReMappable;
    int nForceDisplay;
    int nDisabled = 0;
    int nSortPos = -1;
    g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("ReMappable"),
                                                   &nReMappable);
    g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("ForceDisplay"),
                                                   &nForceDisplay);
    g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("Disabled"), &nDisabled);
    g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("SortPos"), &nSortPos);

    pButton->m_nSortPos = nSortPos;

#if KOTOR1
    // K1 excludes one more action id and takes a single border dimension for the row height.
    if ((nForceDisplay == 0 && nReMappable == 0) || nDisabled == 1 || nActionId == INPUTEVENT_FREELOOK) {
        delete pButton;
        return 0;
    }

    pButton->SetDescription(sActionName);

    int nIdealHeight = pButton->m_text.GetIdealHeight();
    int nHilightDim = pButton->m_borderHilight.m_params.GetBorderDim();
    int nBorderDim = pButton->m_border.m_params.GetBorderDim();
    extent.h = (((nHilightDim > nBorderDim) ? nHilightDim : nBorderDim) + nIdealHeight) * 2;
    pButton->SetExtent(extent);
#else
    if ((nForceDisplay == 0 && nReMappable == 0) || nDisabled == 1) {
        delete pButton;
        return 0;
    }

    pButton->SetDescription(sActionName);

    int nIdealHeight = pButton->m_text.GetIdealHeight();
    int nBorderDim = pButton->m_border.m_params.GetBorderDim();
    int nHilightDim = pButton->m_borderHilight.m_params.GetBorderDim();
    if (nHilightDim < nBorderDim)
        nBorderDim = pButton->m_border.m_params.GetBorderDim();
    else
        nBorderDim = pButton->m_borderHilight.m_params.GetBorderDim();
    extent.h = (nBorderDim + nIdealHeight) * 2;
    pButton->SetExtent(extent);
#endif

    pButton->SetEvents();

    if (nForceDisplay != 0)
        pButton->SetUnchangable();
    return pButton;
}

CExoString CSWGuiKeyMapButton::GetActionId(int nActionId, int nEventClass)
{
    CExoString sResult;
    if (nEventClass == 0)
        sResult = CExoString("Action") + CExoString(nActionId);
    else if (nEventClass == 1)
        sResult.Format(const_cast<char*>("%s%dB"), "Action", nActionId);
    else if (nEventClass == 2)
        sResult.Format(const_cast<char*>("%s%dA"), "Action", nActionId);
    return sResult;
}

void CSWGuiKeyMapButton::SetDescription(const CExoString& sActionName)
{
    uint64_t nActionStrRef;
    g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("ActionStrRef"),
                                                   reinterpret_cast<int*>(&nActionStrRef));
    g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("DescStrRef"),
                                                   reinterpret_cast<int*>(&m_nDescStrRef));
    m_nToolTipStrRef = m_nDescStrRef;
    m_btnKey.m_nToolTipStrRef = m_nDescStrRef;

    g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("Page"), &m_nPage);

    int nValue = 0;
    m_nInputClassMask = 0;
    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("ICPC"), &nValue) &&
        nValue == 1)
        m_nInputClassMask |= IC_PC;
    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("ICMiniGame"),
                                                      &nValue) && nValue == 1)
        m_nInputClassMask |= IC_MINIGAME;
    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("ICPCGUI"), &nValue) &&
        nValue == 1)
        m_nInputClassMask |= IC_GUI;
    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("ICDialog"),
                                                      &nValue) && nValue == 1)
        m_nInputClassMask |= IC_DIALOG;
    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("ICFreeLook"),
                                                      &nValue) && nValue == 1)
        m_nInputClassMask |= IC_FREELOOK;
    if (g_pRules->m_p2DArrays->m_pKeymap->GetINTEntry(sActionName, CExoString("ICMovie"),
                                                      &nValue) && nValue == 1)
        m_nInputClassMask |= IC_MOVIE;

    CExoString sKey = g_pTlkTable->GetSimpleString(nActionStrRef);
    m_text.m_params.SetText(sKey);
    sKey = g_pTlkTable->GetSimpleString(STRREF_NONE_SET);

    int nKey;
    CExoInputEventDesc* pEvent =
        static_cast<CExoInputEventDesc*>(g_pExoInput->GetEvent(m_nActionId, -1));
    if (pEvent != 0) {
        if (pEvent->Is2Button() != 0 && m_nEventClass >= 2) {
            if (m_bChanged == 0) {
                nKey = static_cast<CExoInputeventDesc2ButtonAxis*>(pEvent)->m_nKeyCode2;
                m_nDefaultKey = nKey;
            } else {
                nKey = m_nKeyCode;
            }
            if (nKey != 0)
                sKey = g_pAppManager->m_pClientExoApp->GetDisplayCharacterForKey(nKey);
        } else {
            if (m_bChanged == 0) {
                nKey = pEvent->m_nKeyCode;
                m_nDefaultKey = nKey;
            } else {
                nKey = m_nKeyCode;
            }
            if (nKey != 0)
                sKey = g_pAppManager->m_pClientExoApp->GetDisplayCharacterForKey(nKey);
        }
    }
    m_btnKey.m_text.m_params.SetText(sKey);
}

void CSWGuiKeyMapButton::SetEvents()
{
    AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiKeyMapButton::onEventClicked));
    AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiKeyMapButton::onControlEntered));
    AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiKeyMapButton::onControlExit));
}

int CSWGuiInGameOptKeyMappings::UpdateEvent(int nActionId, int nEventClass, unsigned int nKeyCode,
                                            unsigned int nInputClassMask)
{
    CExoString sKey;
    sKey.Format(const_cast<char*>("KEY%d"), nKeyCode - CExoInput::KEYBOARD_START);

    int nBindable = 0;
    C2DA* pBindable = g_pRules->m_p2DArrays->m_pBindableKey;
#if KOTOR1
    // K1 also refuses Caps Lock and Left Ctrl.
    int bFound = pBindable->GetINTEntry(sKey, CExoString("Bindable"), &nBindable);
    int nResult = CSWGuiKeyMapButton::KEY_INVALID;
    if (bFound != 0 && nBindable != 0 &&
        CExoInput::KEYBOARD_CAPSLOCK != nKeyCode && CExoInput::KEYBOARD_LEFTCTRL != nKeyCode) {
        IsKeyUsedAlready(nKeyCode, nActionId, nEventClass, nInputClassMask);
        nResult = CSWGuiKeyMapButton::KEY_GOOD;
    }
    return nResult;
#else
    if (pBindable->GetINTEntry(sKey, CExoString("Bindable"), &nBindable) != 0 && nBindable != 0) {
        IsKeyUsedAlready(nKeyCode, nActionId, nEventClass, nInputClassMask);
        return CSWGuiKeyMapButton::KEY_GOOD;
    }
    return CSWGuiKeyMapButton::KEY_INVALID;
#endif
}

int CSWGuiInGameOptKeyMappings::IsKeyUsedAlready(unsigned int nKeyCode, int nActionId,
                                                 int nEventClass, unsigned int nInputClassMask)
{
    for (int i = 0; i < m_aOptions.m_count; i++) {
        CSWGuiKeyMapButton* pButton = static_cast<CSWGuiKeyMapButton*>(m_aOptions.m_pArray[i]);
        if ((pButton->m_nActionId != nActionId || pButton->m_nEventClass != nEventClass) &&
            (pButton->m_nInputClassMask & nInputClassMask) != 0) {
            int nKey;
            if (pButton->m_bChanged == 0)
                nKey = pButton->m_nDefaultKey;
            else
                nKey = pButton->m_nKeyCode;
            if (static_cast<unsigned int>(nKey) == nKeyCode) {
                pButton->m_bChanged = 1;
                pButton->m_nKeyCode = 0;
                pButton->SetDescription(pButton->GetActionId(pButton->m_nActionId,
                                                              pButton->m_nEventClass));
#if KOTOR1
                // K1: unchanged rows STANDARD_BLUE, rebound rows COLOR_PURPLE.
                if (pButton->m_bChanged == 0)
                    pButton->m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::STANDARD_BLUE);
                else
                    pButton->m_border.m_params.m_color =
                        *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
                if (pButton->m_bChanged == 0)
                    pButton->m_btnKey.m_border.m_params.m_color =
                        *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::STANDARD_BLUE);
                else
                    pButton->m_btnKey.m_border.m_params.m_color =
                        *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
#else
                if (pButton->m_bChanged == 0)
                    pButton->m_border.m_params.m_color =
                        *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
                else
                    pButton->m_border.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
                if (pButton->m_bChanged == 0)
                    pButton->m_btnKey.m_border.m_params.m_color =
                        *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
                else
                    pButton->m_btnKey.m_border.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
#endif
            }
        }
    }
    return 0;
}

void CSWGuiKeyMapButton::UpdateColor()
{
#if KOTOR1
    // K1: unchanged rows STANDARD_BLUE, rebound rows COLOR_PURPLE.
    if (m_bChanged == 0) {
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::STANDARD_BLUE);
        m_btnKey.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::STANDARD_BLUE);
    } else {
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
        m_btnKey.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
    }
#else
    if (m_bChanged == 0) {
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        m_btnKey.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
    } else {
        m_border.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
        m_btnKey.m_border.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
    }
#endif
}

void CSWGuiKeyMapButton::ResetToDefault()
{
    C2DA* pKeymap = g_pRules->m_p2DArrays->m_pKeymap;

    int nDefaultKey;
    if (pKeymap->GetINTEntry(GetActionId(m_nActionId, m_nEventClass), CExoString("Language0"),
                             &nDefaultKey) != 0) {
#if KOTOR1
        // K1 forces the default back to Left Alt.
        if (m_nActionId == INPUTEVENT_LEFT_LOOKABOUT)
            nDefaultKey = CExoInput::KEYBOARD_LEFTALT;
#endif
        m_bChanged = (m_nDefaultKey != nDefaultKey);
        m_nKeyCode = nDefaultKey;
        SetDescription(GetActionId(m_nActionId, m_nEventClass));
        UpdateColor();
    }
}

void CSWGuiKeyMapButton::PostChangeToEvent()
{
    CExoInputEventDesc* pEvent =
        static_cast<CExoInputEventDesc*>(g_pExoInput->GetEvent(m_nActionId, -1));
    if (pEvent != 0) {
        if (pEvent->Is2Button() != 0 && m_nEventClass >= 2)
            static_cast<CExoInputeventDesc2ButtonAxis*>(pEvent)->m_nKeyCode2 = m_nKeyCode;
        else
            pEvent->m_nKeyCode = m_nKeyCode;
    }

    CExoIni ini;
    CExoString sKey;
    sKey.Format(const_cast<char*>("%d"), m_nKeyCode);
#if KOTOR1
    ini.WriteIniEntry(sKey, CExoString("swkotor.ini"), CExoString("Keymapping"),
                      GetActionId(m_nActionId, m_nEventClass));
#else
    ini.WriteIniEntry(sKey, CExoString(".\\swkotor2.ini"), CExoString("Keymapping"),
                      GetActionId(m_nActionId, m_nEventClass));
#endif
}

void CSWGuiInGameOptKeyMappings::SetCaptureEvent(CSWGuiKeyMapButton* pButton)
{
    if (m_pCaptureEvent != pButton) {
        if (m_pCaptureEvent != 0)
            m_pCaptureEvent->SetEditing(0);
        m_listBox.bSelectOnLeftClick = 0;
        SetCaptureKeyboardMode(0);
    }
    if (pButton != 0) {
        m_pCaptureEvent = pButton;
        pButton->SetEditing(1);
        m_listBox.bSelectOnLeftClick = 1;
        SetCaptureKeyboardMode(1);
    }
}

void CSWGuiKeyMapButton::SetEditing(int bEditing)
{
    if (bEditing != 0) {
#if KOTOR1
        // K1's editing highlight is COLOR_YELLOW.
        m_btnKey.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
#else
        m_btnKey.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
        m_btnKey.m_border.m_params.SetPulsingAlpha(1, 1, 1);
    } else {
        UpdateColor();
        m_btnKey.m_border.m_params.SetPulsingAlpha(0, 0, 0);
    }
}

void CSWGuiInGameOptKeyMappings::Update(float f)
{
    K2_UNUSED(f);
    if (m_bCapturing != 1)
        return;

    unsigned int nKey = static_cast<unsigned int>(g_pExoInput->GetLastCapturedKeyboardKey());
    if (nKey == 0 || m_pCaptureEvent == 0)
        return;

    if (nKey == CExoInput::KEYBOARD_ESC) {
        SetCaptureEvent(0);
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        return;
    }

    int nResult = m_pCaptureEvent->UpdateEvent(nKey);
    if (nResult == CSWGuiKeyMapButton::KEY_INVALID)
        SetCaptureKeyboardMode(1);
    else if (nResult == CSWGuiKeyMapButton::KEY_ALREADYUSED)
        SetCaptureKeyboardMode(1);
    else if (nResult == CSWGuiKeyMapButton::KEY_GOOD) {
        SetCaptureKeyboardMode(0);
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        SetCaptureEvent(0);
    }
}

int CSWGuiKeyMapButton::UpdateEvent(unsigned int nKey)
{
    int nResult = m_pOwner->UpdateEvent(m_nActionId, m_nEventClass, nKey, m_nInputClassMask);
    if (nResult == 0) {
        unsigned int nOldKey = static_cast<unsigned int>(m_nDefaultKey);
        m_nKeyCode = static_cast<int>(nKey);
        m_bChanged = (nOldKey != nKey);
        SetDescription(GetActionId(m_nActionId, m_nEventClass));
        UpdateColor();
    }
    return nResult;
}

CSWGuiKeyMapButton::CSWGuiKeyMapButton(CSWGuiInGameOptKeyMappings* pOwner)
{
    m_pOwner = pOwner;
    m_nActionId = 0;
    m_bChanged = 0;
    m_nKeyCode = 0;
    m_nDefaultKey = -1;
    m_bDisabled = 0;
    m_nInputClassMask = 0;
    m_nPage = 0;
    m_nSortPos = -1;
    m_btnKey.m_pContainer = pOwner;
    m_btnKey.m_pParent = this;
    AddChildControl(&m_btnKey);
}

void CSWGuiKeyMapButton::onControlEntered(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_pOwner->SetCaptureEvent(0);
}

void CSWGuiKeyMapButton::onControlExit(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    return;
}

void CSWGuiKeyMapButton::onEventClicked(CSWGuiControl* pControl)
{
    if (m_bDisabled == 0 && pControl->m_tail.nCurrentEventArg != 0)
        m_pOwner->SetCaptureEvent(this);
}

#if KOTOR1  // K1 routes the narrowed extent through CSWGuiObject::SetExtent
void CSWGuiKeyMapButton::SetExtent(const CSWGuiExtent& extent)
{
#ifndef K2_UI_TOUCH
    // pc / xbox: the key's column is 195 of the 768-line layout's pixels, as the row it shares
    // was before the panel scaled it ("Escape" broke over two lines in 195 screen pixels).
    int nKeyW = static_cast<int>(195.0f * static_cast<float>(g_SDL_mode.h) / 768.0f);
#else
    const int nKeyW = 195;
#endif
    CSWGuiExtent labelExtent = extent;
    labelExtent.w -= nKeyW + 2;
    CSWGuiObject::SetExtent(labelExtent);
    m_border.m_extent = labelExtent;
    m_borderHilight.m_extent = labelExtent;

    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                m_borderHilight.GetInnerExtent()));

    CSWGuiExtent buttonExtent = extent;
    buttonExtent.x = labelExtent.x + labelExtent.w + 2;
    buttonExtent.w = nKeyW;
    m_btnKey.SetExtent(buttonExtent);
}
#else
void CSWGuiKeyMapButton::SetExtent(const CSWGuiExtent& extent)
{
    int nX = extent.x;
    int nY = extent.y;
    int nWidth = extent.w - 197;
    int nHeight = extent.h;
    m_extent.x = nX;
    m_extent.y = nY;
    m_extent.w = nWidth;
    m_extent.h = nHeight;
    m_border.m_extent.x = nX;
    m_border.m_extent.y = nY;
    m_border.m_extent.w = nWidth;
    m_border.m_extent.h = nHeight;
    m_borderHilight.m_extent.x = nX;
    m_borderHilight.m_extent.y = nY;
    m_borderHilight.m_extent.w = nWidth;
    m_borderHilight.m_extent.h = nHeight;

    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                m_borderHilight.GetInnerExtent()));

    CSWGuiExtent buttonExtent = extent;
    buttonExtent.x = nX + nWidth + 2;
    buttonExtent.w = 195;
    m_btnKey.SetExtent(buttonExtent);
}
#endif

void CSWGuiKeyMapButton::Draw(float f)
{
    CSWGuiButton::Draw(f);
    m_btnKey.Draw(f);
}

CSWGuiControl* CSWGuiKeyMapButton::HitCheckMouse(int x, int y)
{
    CSWGuiControl* pHit = CSWGuiControl::HitCheckMouse(x, y);
    if (pHit != 0)
        return pHit;
    return m_btnKey.HitCheckMouse(x, y);
}


const int CSWGuiInGameOptKeyMappings::FILTER_MOVE;
const int CSWGuiInGameOptKeyMappings::FILTER_GAME;
const int CSWGuiInGameOptKeyMappings::FILTER_MINI;

const int CSWGuiKeyMapButton::KEY_INVALID;
const int CSWGuiKeyMapButton::KEY_ALREADYUSED;
const int CSWGuiKeyMapButton::KEY_GOOD;
const int CSWGuiKeyMapButton::IC_PC;
const int CSWGuiKeyMapButton::IC_MINIGAME;
const int CSWGuiKeyMapButton::IC_GUI;
const int CSWGuiKeyMapButton::IC_DIALOG;
const int CSWGuiKeyMapButton::IC_FREELOOK;
const int CSWGuiKeyMapButton::IC_MOVIE;
