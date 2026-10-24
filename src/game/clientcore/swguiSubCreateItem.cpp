// The per-base-item sub-menu of the QA "create item" cheat panel.

#include "k2stdint.h"
#include "swguiSubCreateItem.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "resgff.h"
#include "exoresman.h"
#include "exostring.h"
#include "exolocstring.h"

#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE
#include "CExoArrayListImpl.h"

// The build number the sub-menu shows in its banner.
extern CExoString g_sBuildNumber;

#include "port/libcbase.h"
#include "restype.h"
#include "exoinput.h"

CSWGuiCreateItemSubMenu::CSWGuiCreateItemSubMenu(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_nSelectedIndex = 0;

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

    CExoString sBuildString = CExoString("Build: ") + g_sBuildNumber;
    m_label.m_text.m_params.SetText(sBuildString);

    CenterPanel();
}

// The two wrap arms are deliberately asymmetric: a down-press maps the -99 sentinel before testing
// the updated value, so a fresh panel moves to the first row in one keystroke, while an up-press
// stops at -1.
void CSWGuiCreateItemSubMenu::HandleInputEvent(int type, int val)
{
    if (val) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSubItemCreateMenu();
            break;

        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_KEY_UP:
            if (m_nSelectedIndex == -99)
                m_nSelectedIndex = -1;
            else if (m_nSelectedIndex > 0)
                m_nSelectedIndex--;
            else if (m_nSelectedIndex == 0)
                m_nSelectedIndex = m_paButtons.m_count - 1;
            if (m_pGuiManager)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            break;

        case INPUTEVENT_GUI_DPAD_DOWN:
        case INPUTEVENT_GUI_KEY_DOWN:
            if (m_nSelectedIndex == -99)
                m_nSelectedIndex = -1;
            if (m_nSelectedIndex < m_paButtons.m_count)
                m_nSelectedIndex++;
            else if (m_nSelectedIndex == m_paButtons.m_count)
                m_nSelectedIndex = 0;
            if (m_pGuiManager)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

// Builds one row widget for the sub-menu's list box.
CSWGuiButton* CSWGuiCreateItemSubMenu::CreateOption(const CExoString& sText,
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

// Loads the blueprints whose BaseItem column matches, then rebuilds the list with a Back row
// followed by one row per blueprint.
void CSWGuiCreateItemSubMenu::LoadItems(int nBaseItem)
{
    m_listItems.m_bSorted = 1;
    m_listRefs.m_bSorted = 1;

    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);

    CExoStringList* pList = g_pExoResMan->GetResOfType(RESTYPE_UTI, 0);
    if (pList != 0) {
        int nListCount = pList->GetCount();
        if (nListCount >= 1) {
            int i = 0;
            int bNewGff = 0;
            for (; i < nListCount; i++) {
                CExoString* pName = pList->GetString(i);

                CResGFF* pGFF = static_cast<CResGFF*>(g_pExoResMan->GetResObject(CResRef(pName->CStr()), RESTYPE_UTI));
                if (pGFF == 0) {
                    pGFF = new CResGFF();
                    __strncpy_chk2(pGFF->m_sFileType, "UTI ", 4, 4, 5);
                    g_pExoResMan->SetResObject(CResRef(pName->CStr()), RESTYPE_UTI, pGFF);
                    bNewGff = 1;
                }

                if (pGFF->Demand() != 0) {
                    CResRef templateRef;
                    CResStruct sTop;
                    pGFF->GetTopLevelStruct(&sTop);

                    int bOk;
                    templateRef = pGFF->ReadFieldCResRef(&sTop, const_cast<char*>("TemplateResRef"), bOk,
                                                         CResRef("BAD_BLUEPRINT"));
                    int nBase = pGFF->ReadFieldINT(&sTop, const_cast<char*>("BaseItem"), bOk, 0);

                    if (nBase == nBaseItem && bOk != 0) {
                        CExoString sName;
                        templateRef.CopyToString(sName);
#if !KOTOR1  // K1 lists the bare template resref (no localized name)

                        CExoString sLocalized;
                        CExoLocString sLocString = pGFF->ReadFieldCExoLocString(
                            &sTop, const_cast<char*>("LocalizedName"), bOk, CExoLocString());
                        sLocString.GetString(
                            g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sLocalized, 0);

                        sName = sLocalized + CExoString(" (") + sName + CExoString(")");
#endif

                        m_listItems.Add(new CExoString(sName));
                        m_listRefs.Add(new CExoString(*pName));
                    }

                    pGFF->Release();
                    g_pExoResMan->ReleaseResObject(pGFF);
                    if (bNewGff)
                        delete pGFF;
                    bNewGff = false;
                }
            }
        }
        delete pList;
    }

    m_paButtons.SetSize(m_listItems.GetCount() + 1);
    m_paButtons[0] = CreateOption(CExoString("Back"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItemSubMenu::Back), pTemplate);

    for (int i = 0; i < m_listItems.GetCount(); i++) {
        m_paButtons[i + 1] = CreateOption(CExoString(*m_listItems.GetString(i)),
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItemSubMenu::CreateItem), pTemplate);
    }

    m_listBox.AddControls(m_paButtons, 1, 0, 0);
    m_listBox.bLooping = 1;
    m_listBox.SetSelectedControl(0, 0);
    m_nSelectedIndex = 0;
    SetActiveControl(&m_listBox, 0);
}

// Closes the sub-menu and reopens the category menu it was opened from.
void CSWGuiCreateItemSubMenu::Back(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSubItemCreateMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowItemCreateMenu();
    }
}

// Grants the row's item to the player and closes the sub-menu.
void CSWGuiCreateItemSubMenu::CreateItem(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CResRef resRef;
        resRef = m_listRefs[pControl->m_tail.nControlId - 1]->CStr();

        g_pAppManager->m_pClientExoApp->CreatureAcquireItem(
            g_pAppManager->m_pClientExoApp->GetPlayerCreatureId(), resRef);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSubItemCreateMenu();
    }
}

// Clears the rows; -99 is the "nothing loaded" sentinel.
void CSWGuiCreateItemSubMenu::ClearItems()
{
    m_listBox.ClearItems();
    m_paButtons.Clear();
    m_listRefs.Clear();
    m_listItems.Clear();
    m_nSelectedIndex = -99;
}
