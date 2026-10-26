// The char-gen "quick or custom?" prompt page.

#include "swguiquickorcustompanel.h"
#include "port/sdlapi.h"
#include "swgui.h"
#include "swguimainchargen.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "exoresman.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"

#include <string.h>
#include "exoinput.h"
#include "exostringinline.h"

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif

// The description label is cloned from the list box's prototype row rather than looked up by name;
// the list box keeps its own scroll sound.
CSWGuiQuickOrCustomPanel::CSWGuiQuickOrCustomPanel(CSWGuiManager* pGuiManager, CSWCCreature* pCreature)
    : CSWGuiPanel(pGuiManager)
{
    m_pCreature = pCreature;

#if KOTOR1
    StartLoadFromLayout(CResRef("QORCPNL"));

    InitControl(m_btnQuick, CExoString("QUICK_CHAR_BTN"), 1);
    InitControl(m_btnCustom, CExoString("CUST_CHAR_BTN"), 1);
    InitControl(m_listBox, CExoString("LB_DESC"), 1);
    m_listBox.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblDecoration, CExoString("LBL_DECORATION"), 1);
    InitControl(m_lblRanalog, CExoString("LBL_RANALOG"), 1);
    InitControl(m_lblRbg, CExoString("LBL_RBG"), 1);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1);

    StopLoadFromLayout();
    ScaleExtentForResolution();

    CSWGuiExtent extent = m_extent;
    extent.x += (g_SDL_mode.w - static_cast<int>(g_SDL_mode.h / 768.0f * 1024.0f)) / 2;
    SetExtent(extent);
#else
    StartLoadFromLayout(CResRef("QORCPNL_p"), 0, 1, 1);

    InitControl(m_btnQuick, CExoString("QUICK_CHAR_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_btnCustom, CExoString("CUST_CHAR_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_listBox, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_listBox.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblRanalog, CExoString("LBL_RANALOG"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();
#endif

    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    m_lblDesc.Initialize(pProto->m_extent, pProto, 1.0f);

    m_btnQuick.AddEvent(SWGUIEVENT_FOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickOrCustomPanel::OnEnterQuickCharacterButton));
    m_btnCustom.AddEvent(SWGUIEVENT_FOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickOrCustomPanel::OnEnterCustomCharacterButton));
    m_btnQuick.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickOrCustomPanel::OnSelectQuickCharacterButton));
    m_btnCustom.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickOrCustomPanel::OnSelectCustomCharacterButton));
    m_btnQuick.AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickOrCustomPanel::OnSelectQuickCharacterButton));
    m_btnCustom.AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickOrCustomPanel::OnSelectCustomCharacterButton));

    m_btnQuick.m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    m_btnCustom.m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    SetActiveControl(&m_btnQuick, 0);

#if KOTOR1
    IosAddButtonIcon(&m_btnBack, 'b');
#else
    AspyrAddButtonIcon(&m_btnBack, 'b');
#endif
}

// Each select button only counts as pressed when the manager stamped a non-zero event argument;
// the parent is null-checked because char-gen severs it while tearing the page down.
void CSWGuiQuickOrCustomPanel::OnSelectQuickCharacterButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0 && m_pParentPanel != 0)
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->SetVisiblePanel(CSWGuiMainCharGen::VISIBLE_PANEL_QUICK);
}

void CSWGuiQuickOrCustomPanel::OnSelectCustomCharacterButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0 && m_pParentPanel != 0)
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->SetVisiblePanel(CSWGuiMainCharGen::VISIBLE_PANEL_CUSTOM);
}

// The hide arm stamps the new state into the whole sub-panel chain starting at this panel.
void CSWGuiQuickOrCustomPanel::HandleInputEvent(int nType, int nArg)
{
    if (nArg != 0)
    {
        switch (nType)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            HandleCancelButton();
            break;

        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;

        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBox.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;

        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBox.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(nType, nArg);
}

// Detaches from char-gen, then deletes the page; not a vtable slot.
void CSWGuiQuickOrCustomPanel::HandleCancelButton()
{
    if (m_pParentPanel != 0)
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->RemoveSubPanels();
    SetToBeDeleted(0);
}

// Retargets the page's one description row at a talk-table string and shows the overflow glyph
// label only when the row does not fit.
void CSWGuiQuickOrCustomPanel::SetDescription(unsigned int nStrRef)
{
    STR_RES strRes;
    g_pTlkTable->Fetch(nStrRef, strRes, CTlkTable::GENDER_MALE);

    m_lblDesc.m_text.m_params.SetText(strRes.text);

    m_listBox.ClearItems();

    // The pointer receiver is deliberate, since the virtual height accessors would otherwise be
    // devirtualised.
    CSWGuiText* pText = &m_lblDesc.m_text;
    CSWGuiExtent extent = m_lblDesc.m_extent;
    if (strRes.text.GetLength() > 0)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblDesc.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_lblDesc };
    m_listBox.AddControls(apRows, 1, 0, 0, 0);
    m_listBox.SetSelectedControl(&m_lblDesc, 0);

    m_lblRanalog.m_tail.bVisible = m_listBox.IsOversized();
}
