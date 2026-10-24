// The QA panel that grants powers, feats and skills. The menu has two levels; the main level offers
// three entries, each of which rebuilds the listbox in place as a sub-menu, so "back" returns to
// the main level. Granting anything grants its prerequisites first.
#ifndef CSWGUIPOWERSFEATSSKILLSDEBUGMENU_H
#define CSWGUIPOWERSFEATSSKILLSDEBUGMENU_H

#include "swgui.h"
#include "swguicontrol.h"

class CExoString;
class CSWGuiButton;
class CSWGuiControl;
class CSWGuiObject;
class CSWSCreature;

class CSWGuiPowersFeatsSkillsDebugMenu : public CSWGuiPanel {
public:

    CSWGuiPowersFeatsSkillsDebugMenu(CSWGuiManager* pGuiManager);
    virtual void HandleInputEvent(int type, int val);

    CSWGuiListBox m_listBox;  // the scrolling option list this menu fills
    CSWGuiLabel   m_label;    // with its own border and text
    int  m_nSelectedRow;
    int  m_nNumRows;
    int  m_nSubMenu;          // 0: the main three-entry level, otherwise a powers/feats/skills list

    // Builds one menu row: a button cloned from pTemplate's geometry, captioned sText and wired to
    // pMethod for both the press and the release event.
    CSWGuiButton* CreateOption(const CExoString& sText,
                               void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                               CSWGuiButton* pTemplate);

    void Back();
    void HideMenu();

    void CreateMainPowersFeatsSkillsMenu();
    void CreatePowersSubMenu();
    void CreateSkillsSubMenu();
    void CreateFeatsSubMenu();

    // The main-level entries open the matching sub-menu.
    void OnAddPowerPicked(CSWGuiControl* pControl);
    void OnAddSkillPicked(CSWGuiControl* pControl);
    void OnAddFeatPicked(CSWGuiControl* pControl);

    // The sub-menu entries grant the picked thing to the player, then close.
    void OnPowerPicked(CSWGuiControl* pControl);
    void OnSkillPicked(CSWGuiControl* pControl);
    void OnFeatPicked(CSWGuiControl* pControl);

    void AddPrerequisiteSpells(unsigned int nSpell, CSWSCreature* pCreature);
    void AddPrerequisiteFeats(unsigned short nFeat, CSWSCreature* pCreature);
};

#endif // CSWGUIPOWERSFEATSSKILLSDEBUGMENU_H
