// CSWGuiDebugPartyMenu -- the QA "add party member" panel: one row per recruitable NPC plus two
// bulk entries, "add all" and "add the remote".

#ifndef CSWGUIDEBUGPARTYMENU_H
#define CSWGUIDEBUGPARTYMENU_H

#include "swgui.h"
#include "swguicontrol.h"
#include "exostring.h"
#include "CExoArrayListDtors.h"

class CSWGuiButton;
class CSWGuiControl;
class CSWGuiObject;

// One menu row's payload, hung off the row button's CSWGuiControl::m_tail.m_nUserData slot.
struct SDebugPartyMemberOption {
    int        m_nNpcId;
    CExoString m_sResRef;
};

// Sibling of CSWGuiCreateItemMenu and the other debug menus. Unlike them, each row button carries a
// heap option record, so the destructor has to walk the row list to free them.
class CSWGuiDebugPartyMenu : public CSWGuiPanel {
public:

    CSWGuiDebugPartyMenu(CSWGuiManager* pGuiManager);
    virtual ~CSWGuiDebugPartyMenu();
    virtual void HandleInputEvent(int type, int val);

    CSWGuiListBox m_listBox;
    CSWGuiLabel   m_label;
    int  m_nSelectedRow;
    int  m_nNumRows;
    // The row buttons, whose user-data slots are the only way to reach the option records.
    CExoArrayList<CSWGuiControl*> m_aOptionButtons;

    // Builds one menu row: a button cloned from pTemplate, captioned sText, wired to pMethod for
    // both the press and the release event, and carrying a fresh option record.
    CSWGuiButton* CreateOption(const CExoString& sText,
                               void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                               CSWGuiButton* pTemplate,
                               int nNpcId, CExoString sResRef);

    void Back();
    void OnMemberPicked(CSWGuiControl* pControl);
    void OnAddAll(CSWGuiControl* pControl);
    void OnAddRemote(CSWGuiControl* pControl);
};

#endif // CSWGUIDEBUGPARTYMENU_H
