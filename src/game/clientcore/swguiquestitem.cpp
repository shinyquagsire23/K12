#include "k2stdint.h"
#include "swguiquestitem.h"

#include "swgui.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "exoresman.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "swparty.h"
#include "nwccreature.h"
#include "nwscreature.h"
#include "nwsitem.h"
#include "serverexoapp.h"
#include "swguiingameequip.h"

#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE
#include "CExoArrayListImpl.h"

#include "port/libcstring.h"
#include "exoinput.h"

float GetResolutionTextScalar();
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif

CSWGuiQuestItem::CSWGuiQuestItem(CSWGuiManager* pGuiManager) : CSWGuiPanel(pGuiManager)
{
    m_pSelectedEntry = 0;
#if KOTOR1
    StartLoadFromLayout(CResRef("QuestItem"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("QuestItem_p"), 1, 1, 1);
#endif

    InitControl(m_listBox, CExoString("LB_ITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_ITEM_DESCRIPTION"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);

    CSWGuiControl* pProto = m_listBoxDesc.m_pProtoItem;
    m_lblDescription.Initialize(pProto->m_extent, static_cast<CSWGuiLabel*>(pProto), 1.0f);

    StopLoadFromLayout();
#if KOTOR1
    ScaleExtentForResolution();
#endif

    m_listBox.bShowScrollBar = 1;
    m_lblDescription.m_text.m_params.SetText(CExoString(""));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    SetActiveControl(&m_listBox, 0);
#if KOTOR1
    IosAddButtonIcon(&m_btnBack, 'b');
#endif
}

// Empties both boxes, then frees the owned entry list.
CSWGuiQuestItem::~CSWGuiQuestItem()
{
    m_listBox.ClearItems();
    m_listBoxDesc.ClearItems();

    while (m_aItemEntries.m_pInternal->m_pHead != 0)
    {
        CSWGuiInGameItemEntry* pEntry =
            static_cast<CSWGuiInGameItemEntry*>(m_aItemEntries.m_pInternal->RemoveHead());
        if (pEntry != 0)
            delete pEntry;
    }
}

void CSWGuiQuestItem::Draw(float f)
{
    CSWGuiPanel::Draw(f);
}

void CSWGuiQuestItem::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();

#if KOTOR1  // K1: the panel state bits sit one higher (bit 3 inserted)
            unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
            if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
            unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
            if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
            m_stateFlags = nFlags;
            break;
        }
        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;
        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}

// Populates before the base call, so the list is ready when the panel is shown.
void CSWGuiQuestItem::OnPanelAdded()
{
    PopulateItemListBox();
    CSWGuiPanel::OnPanelAdded();
}

// One row per plot item in the player creature's item repository.
void CSWGuiQuestItem::PopulateItemListBox()
{
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CSWCCreature* pCreature = pParty->GetPlayerCharacter();
    CSWSCreature* pServerCreature = pCreature->GetServerCreature();
    CItemRepository* pRepository = pServerCreature->GetItemRepository(1);

    CExoArrayList<CSWGuiControl*> aControls;
    if (pRepository->ItemListGetCount() != 0)
    {
        int nWidth = m_listBox.m_innerExtent.w - static_cast<int>(m_listBox.m_nPadding) * 2;
        CSWGuiControl* pProto = m_listBox.m_pProtoItem;

        for (unsigned int i = 0; i < pRepository->ItemListGetCount(); i++)
        {
            CSWSItem* pItem = pRepository->ItemListGetItem(i);
            if (pItem != 0 && pItem->m_bPlot != 0)
            {
                CSWGuiInGameItemEntry* pEntry = new CSWGuiInGameItemEntry();
                pEntry->Initialize(static_cast<CSWGuiButton*>(pProto)->m_text,
                                   static_cast<CSWGuiButton*>(pProto)->m_border,
                                   static_cast<CSWGuiButton*>(pProto)->m_borderHilight,
                                   nWidth, 0);
                pEntry->AddEvent(SWGUIEVENT_FOCUS, this,
                    static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuestItem::OnControlEntered));
                pEntry->SetItem(pItem->m_objectId, 0, 0);
                m_aItemEntries.AddTail(pEntry);
                aControls.Add(pEntry);
            }
        }
    }

    m_listBox.AddControls(aControls, 0, 1, 0);

    if (m_pSelectedEntry != 0)
        m_pSelectedEntry->SetActive(0);

    if (aControls.m_count < 1)
    {
        m_listBox.SetSelectedControl(-1, 0);
        m_lblDescription.m_text.m_params.SetText(CExoString(""));
        OnControlEntered(0);
    }
    else
    {
        m_listBox.SetSelectedControl(0, 0);
    }
}

// Refreshes the description box with the hovered row's item.
void CSWGuiQuestItem::OnControlEntered(CSWGuiControl* pControl)
{
    if (pControl != 0 && pControl->m_tail.nCurrentEventArg != 0)
    {
        m_pSelectedEntry = pControl;

        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                              static_cast<CSWGuiInGameItemEntry*>(pControl)->m_oidItem);
        if (pItem != 0)
            m_lblDescription.m_text.m_params.SetText(pItem->GetPropertyDescription());
        else
            m_lblDescription.m_text.m_params.SetText(CExoString("Error: Invalid item"));
    }

    CSWGuiText* pText = &m_lblDescription.m_text;

    CSWGuiExtent extent = m_lblDescription.m_extent;
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (m_lblDescription.m_text.m_params.m_sText.GetLength() >= 1)
#else
    if (m_lblDescription.m_text.m_params.m_sText.m_sString != 0 &&
        static_cast<int>(strlen(m_lblDescription.m_text.m_params.m_sText.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
#if !KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // resolution text scaling is K2-only
    extent.h = static_cast<int>(GetResolutionTextScalar() * static_cast<float>(extent.h));
#endif
    m_lblDescription.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_lblDescription };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_lblDescription, 0);
}
