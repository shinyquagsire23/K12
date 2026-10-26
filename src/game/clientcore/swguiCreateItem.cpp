// The QA create-item panel's callbacks and key handling.

#include "swguiCreateItem.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "nwsrules.h"
#include "nwitem.h"
#include "exoresman.h"
#include "twodimarray.h"
#include "exostring.h"
#include "exoinput.h"
#include "CExoArrayListImpl.h"

extern CExoString g_sBuildNumber;

CSWGuiCreateItemMenu::CSWGuiCreateItemMenu(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_nSelectedItemType = 0;

#if KOTOR1
    StartLoadFromLayout(CResRef("debug"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("debug_p"), 1, 1, 1);
#endif
    InitControl(m_listBox, CExoString("LB_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_label, CExoString("LBL_BUILD"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#if KOTOR1
    ScaleExtentForResolution();
#endif

    // The build string must outlive CenterPanel, so it cannot be a temporary.
    CExoString sBuildString = CExoString("Build: ") + g_sBuildNumber;
    m_label.m_text.m_params.SetText(sBuildString);

    CExoArrayList<CSWGuiControl*> list;
    int nCount = g_pRules->m_pBaseItemArray->m_count;
    int nSize = nCount + 1;
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    list.SetSize(nSize);

    list.m_pArray[0] = CreateOption(CExoString("Back"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItemMenu::Back), pTemplate);

    for (int i = 1; i < nSize; i++) {
        CExoString sName;
        if (g_pRules->GetBaseItem(static_cast<int>(i - 1))->m_nNameStrRef == 0) {
            // Rows with no TLK strref fall back to the 2DA's own Label column, so an unmapped row is
            // still identifiable. The table is re-opened per row.
            C2DA* p2DA = new C2DA(CResRef("BASEITEMS"), 0);
            p2DA->Load2DArray();
            CExoString sLabel;
            p2DA->GetCExoStringEntry(static_cast<int>(i - 1), CExoString("Label"), &sLabel);
            sName.Format(const_cast<char*>("No StrRef: %s"), sLabel.CStr());
            delete p2DA;
        }
        else {
            // Re-derived rather than cached from the test above.
            sName = g_pRules->GetBaseItem(static_cast<int>(i - 1))->GetNameText();
        }
        list.m_pArray[i] = CreateOption(sName,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItemMenu::ItemSubTypeMenu), pTemplate);
    }

    m_listBox.AddControls(list, 1, 0, 0);
    m_listBox.bLooping = 1;
    SetActiveControl(&m_listBox, 0);
    CenterPanel();
}

// Closes the panel and returns to the main debug menu.
void CSWGuiCreateItemMenu::Back(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideItemCreateMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowDebugMenu();
    }
}

// The control id is the baseitems.2da row biased by one.
void CSWGuiCreateItemMenu::ItemSubTypeMenu(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideItemCreateMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowSubItemCreateMenu(
            pControl->m_tail.nControlId - 1);
    }
}

// The two wrap tests are deliberately asymmetric: down-wraps compare against m_count, so the index
// legitimately reaches m_count (one past the last row), while up-wraps restore m_count. Every
// sibling menu wraps at m_count - 1 instead.
void CSWGuiCreateItemMenu::HandleInputEvent(int type, int val)
{
    if (val) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            g_pAppManager->m_pClientExoApp->GetInGameGui()->HideItemCreateMenu();
            break;

        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_KEY_UP:
            if (m_nSelectedItemType > 0)
                m_nSelectedItemType--;
            else if (m_nSelectedItemType == 0)
                m_nSelectedItemType = g_pRules->m_pBaseItemArray->m_count;
            if (m_pGuiManager)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            break;

        case INPUTEVENT_GUI_DPAD_DOWN:
        case INPUTEVENT_GUI_KEY_DOWN:
            if (m_nSelectedItemType < g_pRules->m_pBaseItemArray->m_count)
                m_nSelectedItemType++;
            else if (m_nSelectedItemType == g_pRules->m_pBaseItemArray->m_count)
                m_nSelectedItemType = 0;
            if (m_pGuiManager)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

// Value-initialises the button, zero-filling the whole object before construction.
CSWGuiButton* CSWGuiCreateItemMenu::CreateOption(const CExoString& sText,
                                  void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                                  CSWGuiButton* pTemplate)
{
#if KOTOR1  // K1: a zero extent, sized afterwards to the caption's ideal height plus the wider border
    CSWGuiExtent extent;
    CSWGuiButton* pButton = new CSWGuiButton();
    pButton->Initialize(extent, pTemplate);
    pButton->m_text.m_params.SetText(sText);
    CSWGuiText* pText = &pButton->m_text;
    int nIdealHeight = pText->GetIdealHeight();
    int nHilightDim = pButton->m_borderHilight.m_params.GetBorderDim();
    int nBorderDim = pButton->m_border.m_params.GetBorderDim();
    extent.h = nIdealHeight + ((nHilightDim > nBorderDim) ? nHilightDim : nBorderDim) * 2;
    pButton->SetExtent(extent);
#else
    CSWGuiExtent extent = pTemplate->m_extent;
    CSWGuiButton* pButton = new CSWGuiButton();
    pButton->Initialize(extent, pTemplate);
    pButton->m_text.m_params.SetText(sText);
#endif
    pButton->AddEvent(INPUTEVENT_GUI_A, this, pMethod);
    pButton->AddEvent(INPUTEVENT_GUI_START, this, pMethod);
    return pButton;
}
