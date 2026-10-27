// CSWGuiPowersFeatsSkillsDebugMenu, the QA grant-powers/feats/skills panel.

#include "k2stdint.h"
#include "swguiPowersFeatsSkillsDebugMenu.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "serverexoapp.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "nwsrules.h"
#include "nwspell.h"
#include "nwscreature.h"
#include "nwskill.h"
#include "exostring.h"
#include "CExoArrayListImpl.h"
#include "exoresman.h"
#include "exoinput.h"

extern CExoString g_sBuildNumber;

// Resets to the main level on the way out, so re-opening the panel never lands mid-sub-menu.
void CSWGuiPowersFeatsSkillsDebugMenu::HideMenu()
{
    CreateMainPowersFeatsSkillsMenu();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HidePowersFeatsSkillsDebugMenu();
}

// Steps back one level: out of a sub-menu to the main level, or out of the panel entirely.
void CSWGuiPowersFeatsSkillsDebugMenu::Back()
{
    if (m_nSubMenu) {
        CreateMainPowersFeatsSkillsMenu();
    } else {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HidePowersFeatsSkillsDebugMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowDebugMenu();
    }
}

// Builds the panel from the "debug_p" layout, binds the two hard-wired controls by name, stamps the
// build number onto LBL_BUILD, and builds the main menu as the starting level.
CSWGuiPowersFeatsSkillsDebugMenu::CSWGuiPowersFeatsSkillsDebugMenu(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1  // K1: the layout is "debug"
    StartLoadFromLayout(CResRef("debug"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("debug_p"), 1, 1, 1);
#endif
    InitControl(m_listBox, CExoString("LB_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_label, CExoString("LBL_BUILD"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#if KOTOR1  // K1: the panel is scaled to the screen
    ScaleExtentForResolution();
#endif
    CExoString sBuildString = CExoString("Build: ") + g_sBuildNumber;
    m_label.m_text.m_params.SetText(sBuildString);
    CreateMainPowersFeatsSkillsMenu();
    CenterPanel();
}

// Rebuilds the main three-entry level (Add Power / Add Skill / Add Feat).
void CSWGuiPowersFeatsSkillsDebugMenu::CreateMainPowersFeatsSkillsMenu()
{
    m_nSelectedRow = 0;
    m_listBox.ClearItems();
    CExoArrayList<CSWGuiControl*> list;
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    list.SetSize(3);
    list.m_pArray[0] = CreateOption(CExoString("Add Power"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPowersFeatsSkillsDebugMenu::OnAddPowerPicked),
        pTemplate);
    list.m_pArray[1] = CreateOption(CExoString("Add Skill"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPowersFeatsSkillsDebugMenu::OnAddSkillPicked),
        pTemplate);
    list.m_pArray[2] = CreateOption(CExoString("Add Feat"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPowersFeatsSkillsDebugMenu::OnAddFeatPicked),
        pTemplate);
    m_listBox.AddControls(list, 1, 0, 0);
    m_listBox.bLooping = 1;
    SetActiveControl(&m_listBox, 0);
    m_nSubMenu = 0;
    m_listBox.SetActiveControl(m_listBox.GetControl(m_nSelectedRow), 0);
}

// Rebuilds the powers sub-menu: one row per useable force power. A spell gets a row only when it
// has a name and is flagged useable, so this list is grown with CExoArrayList::Add. Each row keeps
// its spells.2da row id in the button's user-data word, which OnPowerPicked reads back.
void CSWGuiPowersFeatsSkillsDebugMenu::CreatePowersSubMenu()
{
    m_nSelectedRow = 0;
    m_listBox.ClearItems();
    CExoArrayList<CSWGuiControl*> list;
    int nCount = g_pRules->m_pSpellArray->m_nCount;
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    for (int i = 0; i < nCount; i++) {
        CSWSpell* pSpell = g_pRules->GetSpell(static_cast<int>(i));
        if (pSpell->m_nUserType == 1 && pSpell->m_nNameStrRef != 0) {
            CExoString s("Add ");
            s = s + g_pRules->GetSpell(static_cast<int>(i))->GetSpellNameText();
            CSWGuiButton* pButton = CreateOption(s,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPowersFeatsSkillsDebugMenu::OnPowerPicked),
                pTemplate);
            pButton->m_tail.m_nUserData = i;
            list.Add(pButton);
        }
    }
    m_listBox.AddControls(list, 1, 0, 0);
    m_listBox.bLooping = 1;
    SetActiveControl(&m_listBox, 0);
    m_nSubMenu = 1;
    m_listBox.SetActiveControl(m_listBox.GetControl(m_nSelectedRow), 0);
}

// Rebuilds the skills sub-menu: one row per skill.2da row.
void CSWGuiPowersFeatsSkillsDebugMenu::CreateSkillsSubMenu()
{
    m_nSelectedRow = 0;
    m_listBox.ClearItems();
    CExoArrayList<CSWGuiControl*> list;
    int64_t nCount = g_pRules->m_nNumSkills;
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    list.SetSize(static_cast<int>(nCount));
    int64_t i;
    for (i = 0; i < nCount; i++) {
        CExoString s("Add ");
        s = s + g_pRules->m_paSkills[i].GetNameText();
        CSWGuiButton* pButton = CreateOption(s,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPowersFeatsSkillsDebugMenu::OnSkillPicked),
            pTemplate);
        list.m_pArray[i] = pButton;
    }
    m_listBox.AddControls(list, 1, 0, 0);
    m_listBox.bLooping = 1;
    SetActiveControl(&m_listBox, 0);
    m_nSubMenu = 2;
    m_listBox.SetActiveControl(m_listBox.GetControl(m_nSelectedRow), 0);
}

// Rebuilds the feats sub-menu: one row per entry in the compacted feat list (m_paFeatIndices, count
// in m_nFeatIndices). Each row is captioned with the feat's TLK name, so the row index OnFeatPicked
// recovers by walking the listbox is an index into that list, not a feat id.
void CSWGuiPowersFeatsSkillsDebugMenu::CreateFeatsSubMenu()
{
    m_nSelectedRow = 0;
    m_listBox.ClearItems();
    CExoArrayList<CSWGuiControl*> list;
    int64_t nCount = g_pRules->m_nFeatIndices;
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    list.SetSize(static_cast<int>(nCount));
    int64_t i;
    for (i = 0; i < nCount; i++) {
        CExoString s("Add ");
        s = s + g_pRules->GetFeat(g_pRules->m_paFeatIndices[i])->GetNameText();
        CSWGuiButton* pButton = CreateOption(s,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPowersFeatsSkillsDebugMenu::OnFeatPicked),
            pTemplate);
        list.m_pArray[i] = pButton;
    }
    m_listBox.AddControls(list, 1, 0, 0);
    m_listBox.bLooping = 1;
    SetActiveControl(&m_listBox, 0);
    m_nSubMenu = 3;
    m_listBox.SetActiveControl(m_listBox.GetControl(m_nSelectedRow), 0);
}

// ---------------------------------------------------------------------------------------------
// Main-level entries.

void CSWGuiPowersFeatsSkillsDebugMenu::OnAddPowerPicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        CreatePowersSubMenu();
}

void CSWGuiPowersFeatsSkillsDebugMenu::OnAddSkillPicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        CreateSkillsSubMenu();
}

void CSWGuiPowersFeatsSkillsDebugMenu::OnAddFeatPicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        CreateFeatsSubMenu();
}

// ---------------------------------------------------------------------------------------------
// Sub-menu entries.

// A power row carries its own spell id in the control's user-data word, so the handler needs no
// listbox search, unlike the skill and feat handlers.
void CSWGuiPowersFeatsSkillsDebugMenu::OnPowerPicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        int nSpell = static_cast<int>(pControl->m_tail.m_nUserData);
        CSWSpell* pSpell = g_pRules->GetSpell(nSpell);
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
        if (pSpell && pCreature)
            AddPrerequisiteSpells(nSpell, pCreature);
        CreateMainPowersFeatsSkillsMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HidePowersFeatsSkillsDebugMenu();
    }
}

// Grants the picked entry's prerequisites recursively: walks the prerequisite list, grants anything
// missing (which recurses into its own prerequisites), and only then grants the thing asked for.
void CSWGuiPowersFeatsSkillsDebugMenu::AddPrerequisiteSpells(unsigned int nSpell,
                                                             CSWSCreature* pCreature)
{
    CSWSpell* pSpell = g_pRules->GetSpell(nSpell);
    if (pCreature && pSpell) {
        unsigned char nClass = pCreature->m_pStats->m_numClasses - 1;
        for (int* pnPrereq = pSpell->m_anPrereq1; *pnPrereq != -1; pnPrereq++) {
            int nPrereq = *pnPrereq;
            if (!pCreature->m_pStats->HasSpell(0, nPrereq, 1))
                AddPrerequisiteSpells(nPrereq, pCreature);
        }
        pCreature->m_pStats->AddKnownSpell(nClass, nSpell);
    }
}

// Grants a feat's prerequisites the same way. Feats have exactly two prerequisite slots rather
// than a terminated list, so the walk is unrolled by hand.
void CSWGuiPowersFeatsSkillsDebugMenu::AddPrerequisiteFeats(unsigned short nFeat,
                                                            CSWSCreature* pCreature)
{
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if (pCreature && pFeat) {
        unsigned short nPrereq1 = pFeat->m_nPrereqFeat1;
        unsigned short nPrereq2 = pFeat->m_nPrereqFeat2;
        if (nPrereq1 != 0xffff && !pCreature->m_pStats->HasFeat(nPrereq1))
            AddPrerequisiteFeats(nPrereq1, pCreature);
        if (nPrereq2 != 0xffff && !pCreature->m_pStats->HasFeat(nPrereq2))
            AddPrerequisiteFeats(nPrereq2, pCreature);
        pCreature->m_pStats->AddFeat(nFeat);
    }
}

// ---------------------------------------------------------------------------------------------

void CSWGuiPowersFeatsSkillsDebugMenu::HandleInputEvent(int type, int val)
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

// ---------------------------------------------------------------------------------------------

// `new CSWGuiButton()` needs its parentheses: CSWGuiButton has no user-provided default constructor,
// so value-initialization zero-fills the whole object before the constructor runs. Without the
// parentheses that zero fill would be dropped.
CSWGuiButton* CSWGuiPowersFeatsSkillsDebugMenu::CreateOption(const CExoString& sText,
                                  void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                                  CSWGuiButton* pTemplate)
{
#if KOTOR1
    // K1 sizes the option to its own text: a zero extent, then the height is the text's ideal
    // height plus twice the larger border.
    CSWGuiExtent extent = {0, 0, 0, 0};
    CSWGuiButton* pButton = new CSWGuiButton();
    pButton->Initialize(extent, pTemplate);
    pButton->m_text.m_params.SetText(sText);
    int nText = static_cast<CSWGuiDrawable*>(&pButton->m_text)->GetIdealHeight();
    int nHilight = pButton->m_borderHilight.m_params.GetBorderDim();
    int nBorder = pButton->m_border.m_params.GetBorderDim();
    extent.h = nText + (nHilight > nBorder ? nHilight : nBorder) * 2;
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

// A skill row carries no payload of its own, so the handler recovers its index by walking the
// listbox until GetControl returns the control that was clicked; that index is the skill id. The
// granted rank is the character's level plus 3, the class maximum for a class skill.
void CSWGuiPowersFeatsSkillsDebugMenu::OnSkillPicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        int nSkill = 0;
        while (m_listBox.GetControl(nSkill) != pControl)
            nSkill++;
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
        if (pCreature) {
            CSWSCreatureStats* pStats = pCreature->m_pStats;
            pStats->SetSkillRank(static_cast<unsigned char>(nSkill), static_cast<char>(pStats->GetLevel(0) + 3));
        }
        CreateMainPowersFeatsSkillsMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HidePowersFeatsSkillsDebugMenu();
    }
}

// Same listbox walk as OnSkillPicked, but the row index is an index into the compacted feat list
// (g_pRules->m_paFeatIndices), not a feat id: the submenu only lists feats that exist in feat.2da,
// so the two numbering spaces differ.
void CSWGuiPowersFeatsSkillsDebugMenu::OnFeatPicked(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        int nRow = 0;
        while (m_listBox.GetControl(nRow) != pControl)
            nRow++;
        unsigned short nFeat = g_pRules->m_paFeatIndices[static_cast<unsigned short>(nRow)];
        CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
        CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
        if (pFeat && pCreature)
            AddPrerequisiteFeats(nFeat, pCreature);
        CreateMainPowersFeatsSkillsMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HidePowersFeatsSkillsDebugMenu();
    }
}
