#ifndef CSWGUILOADMODULEDEBUGMENU_H
#define CSWGUILOADMODULEDEBUGMENU_H

#include "swgui.h"
#include "swguicontrol.h"

class CExoString;
class CSWGuiButton;
class CSWGuiControl;
class CSWGuiObject;

class CSWGuiLoadModuleDebugMenu : public CSWGuiPanel {
public:

    CSWGuiLoadModuleDebugMenu(CSWGuiManager* pGuiManager);
    virtual void HandleInputEvent(int type, int val);

    CSWGuiListBox m_listBox;
    CSWGuiLabel   m_label;
    int  m_nSelectedModule;
    int  m_nNumModules;

    // Clones pTemplate's geometry into a new row button captioned sText.
    CSWGuiButton* CreateOption(const CExoString& sText,
                               void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                               CSWGuiButton* pTemplate);

    void Back();
    void OnModulePicked(CSWGuiControl* pControl);
};

#endif // CSWGUILOADMODULEDEBUGMENU_H
