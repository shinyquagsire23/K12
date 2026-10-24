// The QA panel that adds party members.

#include "swguidebugpartymenu.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "serverexoapp.h"
#include "partytables.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "nwscript.h"
#include "exoresman.h"

#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE
#include "CExoArrayListImpl.h"
#include "gameobjectarray.h"
#include "exoinput.h"

extern CExoString g_sBuildNumber;

CSWGuiDebugPartyMenu::CSWGuiDebugPartyMenu(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_nSelectedRow = 0;

    StartLoadFromLayout(CResRef("debug_p"), 1, 1, 1);
    InitControl(m_listBox, CExoString("LB_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_label, CExoString("LBL_BUILD"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    CExoString sBuildString = CExoString("Build: ") + g_sBuildNumber;
    m_label.m_text.m_params.SetText(sBuildString);

    // Rows clone the listbox's load-time prototype widget.
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);

    m_aOptionButtons.Add(CreateOption(CExoString("Add all except Mututally Exclusives"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnAddAll), pTemplate, 0,
        CExoString(static_cast<const char*>(0))));
    m_aOptionButtons.Add(CreateOption(CExoString("Atton"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 0,
        CExoString("p_atton")));
    m_aOptionButtons.Add(CreateOption(CExoString("Bao Dur"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 1,
        CExoString("p_baodur")));
    m_aOptionButtons.Add(CreateOption(CExoString("Bao Dur with Remote"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnAddRemote), pTemplate, 0,
        CExoString(static_cast<const char*>(0))));
    m_aOptionButtons.Add(CreateOption(CExoString("Mandalore"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 2,
        CExoString("p_mand")));
    m_aOptionButtons.Add(CreateOption(CExoString("G0-T0"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 3,
        CExoString("p_g0t0")));
    m_aOptionButtons.Add(CreateOption(CExoString("Handmaiden"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 4,
        CExoString("p_handmaiden")));
    m_aOptionButtons.Add(CreateOption(CExoString("HK-47"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 5,
        CExoString("p_hk47")));
    m_aOptionButtons.Add(CreateOption(CExoString("Kreia"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 6,
        CExoString("p_kreia")));
    m_aOptionButtons.Add(CreateOption(CExoString("Mira"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 7,
        CExoString("p_mira")));
    m_aOptionButtons.Add(CreateOption(CExoString("T3-M4"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 8,
        CExoString("p_t3m4")));
    m_aOptionButtons.Add(CreateOption(CExoString("Visas Marr"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 9,
        CExoString("p_visas")));
    m_aOptionButtons.Add(CreateOption(CExoString("Hanharr"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 10,
        CExoString("p_hanharr")));
    m_aOptionButtons.Add(CreateOption(CExoString("Disciple"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugPartyMenu::OnMemberPicked), pTemplate, 11,
        CExoString("p_disciple")));

    m_listBox.AddControls(m_aOptionButtons, 1, 0, 0);
    m_listBox.bLooping = 1;
    SetActiveControl(&m_listBox, 0);
    CenterPanel();
}

// Unlike the sibling panels this one does not hide itself first; it only reopens the debug menu.
void CSWGuiDebugPartyMenu::Back()
{
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowDebugMenu();
}

// The row's payload hangs off the button; the callback receives the base control.
void CSWGuiDebugPartyMenu::OnMemberPicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CSWGuiControl* pButton = pControl->AsButton();
        SDebugPartyMemberOption* pOption =
            reinterpret_cast<SDebugPartyMemberOption*>(pButton->m_tail.m_nUserData);
        g_pAppManager->m_pServerExoApp->GetPartyTable()->AddNPC(pOption->m_nNpcId,
                                                               pOption->m_sResRef);
    }
}

// Adds every recruitable companion at once. Slots 1, 4 and 7 belong to companions this menu does
// not offer, so the numbers are not contiguous.
void CSWGuiDebugPartyMenu::OnAddAll(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
        if (pPartyTable) {
            pPartyTable->AddNPC(0, "p_atton");
            pPartyTable->AddNPC(2, "p_mand");
            pPartyTable->AddNPC(3, "p_g0t0");
            pPartyTable->AddNPC(5, "p_hk47");
            pPartyTable->AddNPC(6, "p_kreia");
            pPartyTable->AddNPC(8, "p_t3m4");
            pPartyTable->AddNPC(9, "p_visas");
            if (g_pVirtualMachine) {
                CExoString sScript("k_sup_baodur");
                g_pVirtualMachine->RunScript(&sScript, OBJECT_INVALID, 0);
            }
        }
    }
}

// Bao-Dur's remote is spawned by running its support script, not added to the party table.
void CSWGuiDebugPartyMenu::OnAddRemote(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        if (g_pVirtualMachine) {
            CExoString sScript("k_sup_baodur");
            g_pVirtualMachine->RunScript(&sScript, OBJECT_INVALID, 0);
        }
    }
}

void CSWGuiDebugPartyMenu::HandleInputEvent(int type, int val)
{
    if (val) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            Back();
            break;

        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_KEY_UP:
            if (m_nSelectedRow > 0)
                m_nSelectedRow--;
            else if (m_nSelectedRow == 0)
                m_nSelectedRow = m_nNumRows - 1;
            if (m_pGuiManager)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            break;

        case INPUTEVENT_GUI_DPAD_DOWN:
        case INPUTEVENT_GUI_KEY_DOWN:
            if (m_nSelectedRow < m_nNumRows)
                m_nSelectedRow++;
            else if (m_nSelectedRow == m_nNumRows - 1)
                m_nSelectedRow = 0;
            if (m_pGuiManager)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

// The family's CreateOption plus a heap option record on the button's user-data slot, holding the
// party slot and creature template that OnMemberPicked hands to AddNPC. `new CSWGuiButton()` has
// parens and is value-initialised; the record is not, so its id word is set explicitly below.
CSWGuiButton* CSWGuiDebugPartyMenu::CreateOption(const CExoString& sText,
                                                 void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                                                 CSWGuiButton* pTemplate,
                                                 int nNpcId, CExoString sResRef)
{
    CSWGuiExtent extent = pTemplate->m_extent;
    CSWGuiButton* pButton = new CSWGuiButton();
    pButton->Initialize(extent, pTemplate);
    pButton->m_text.m_params.SetText(sText);
    pButton->AddEvent(INPUTEVENT_GUI_START, this, pMethod);
    pButton->AddEvent(INPUTEVENT_GUI_A, this, pMethod);

    SDebugPartyMemberOption* pOption = new SDebugPartyMemberOption;
    pOption->m_nNpcId = nNpcId;
    pOption->m_sResRef = sResRef;
    pButton->m_tail.m_nUserData = reinterpret_cast<unsigned long long>(pOption);
    return pButton;
}

// Each row button owns a heap option record on its user-data slot, so the panel walks its button
// array to free them; the array storage belongs to the implicit ~CExoArrayList.
CSWGuiDebugPartyMenu::~CSWGuiDebugPartyMenu()
{
    for (int i = 0; i < m_aOptionButtons.m_count; i++) {
        CSWGuiControl* pButton = m_aOptionButtons.m_pArray[i];
        if (pButton) {
            SDebugPartyMemberOption* pOption =
                reinterpret_cast<SDebugPartyMemberOption*>(pButton->m_tail.m_nUserData);
            if (pOption)
                delete pOption;
        }
    }
}
