#pragma once

// The character generator's opening page, the modal "quick character or custom character?" prompt.
// Two choice buttons plus a back button and the description row the two hover handlers retarget.

class CSWCCreature;
class CSWGuiManager;
class CSWGuiControl;
class CExoString;

#include "swgui.h"
#include "swguicontrol.h"
#include "strref.h"

class CSWGuiQuickOrCustomPanel : public CSWGuiPanel {
public:
    CSWGuiQuickOrCustomPanel(CSWGuiManager* pGuiManager, CSWCCreature* pCreature);

    virtual void HandleInputEvent(int nType, int nArg);

    void SetDescription(unsigned int nStrRef);
    void HandleCancelButton();

    void OnEnterQuickCharacterButton(CSWGuiControl* pControl)
    {
        K2_UNUSED(pControl);
        SetDescription(STRREF_QUICK_CHARACTER_DESCRIPTION);
    }
    void OnEnterCustomCharacterButton(CSWGuiControl* pControl)
    {
        K2_UNUSED(pControl);
        SetDescription(STRREF_CUSTOM_CHARACTER_DESCRIPTION);
    }

    void OnSelectQuickCharacterButton(CSWGuiControl* pControl);
    void OnSelectCustomCharacterButton(CSWGuiControl* pControl);

    CSWCCreature*  m_pCreature;     // stored by the constructor, read nowhere in this unit
    CSWGuiPanel*   m_pParentPanel;  // the owning main char-gen page, written by its constructor
    CSWGuiButton   m_btnQuick;      // "QUICK_CHAR_BTN"
    CSWGuiButton   m_btnCustom;     // "CUST_CHAR_BTN"
    CSWGuiLabel    m_lblDesc;       // the description row, cloned from the list box prototype item
#if KOTOR1
    CSWGuiLabel    m_lblDecoration; // "LBL_DECORATION"
#endif
    CSWGuiLabel    m_lblRanalog;    // "LBL_RANALOG"; its visibility follows the box's overflow state
#if KOTOR1
    CSWGuiLabel    m_lblRbg;        // "LBL_RBG"
#endif
    CSWGuiListBox  m_listBox;       // "LB_DESC"
    CSWGuiButton   m_btnBack;       // "BTN_BACK"
};

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiQuickOrCustomPanel) == 0xfa8, "CSWGuiQuickOrCustomPanel size drift");
#endif
