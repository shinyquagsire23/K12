// The QA "create item" cheat panel's per-base-item sub-menu, a scrolling list of the blueprints
// whose BaseItem column matches the chosen category.
#ifndef CSWGUICREATEITEMSUBMENU_H
#define CSWGUICREATEITEMSUBMENU_H

#include "swgui.h"
#include "swguicontrol.h"
#include "CExoArrayListDtors.h"
#include "exostringlist.h"

class CExoString;
class CSWGuiButton;
class CSWGuiControl;
class CSWGuiObject;

class CSWGuiCreateItemSubMenu : public CSWGuiPanel {
public:
    CSWGuiCreateItemSubMenu(CSWGuiManager* pGuiManager);
    virtual void HandleInputEvent(int type, int val);

    // Fills the list from every item blueprint whose BaseItem matches, then rebuilds the button
    // array: [0] is "Back" and [i+1] one button per collected entry.
    void LoadItems(int nBaseItem);
    void ClearItems();

    CSWGuiButton* CreateOption(const CExoString& sText,
                               void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                               CSWGuiButton* pTemplate);

    void Back(CSWGuiControl* pControl);
    void CreateItem(CSWGuiControl* pControl);

    CSWGuiListBox m_listBox;
    CSWGuiLabel   m_label;

    // Highlighted row; -99 means nothing is loaded yet.
    int m_nSelectedIndex;

    CExoArrayList<CSWGuiControl*> m_paButtons;

    CExoStringList m_listItems;
    CExoStringList m_listRefs;
};

#endif // CSWGUICREATEITEMSUBMENU_H
