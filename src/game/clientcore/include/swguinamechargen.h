#pragma once

// The character-generation "enter your name" page. It is pushed by CSWGuiQuickPanel::OnSelectNameButton
// and CSWGuiCustomPanel::OnSelectNameButton, which then set themselves as its parent through
// SetParentPanel.
#include "swgui.h"
#include "swguicontrol.h"
#include "exostring.h"
#include "nwnamegen.h"

class CSWCCreature;
class CSWGuiManager;
class CSWGuiControl;
class CResRef;

class CSWGuiNameChargen : public CSWGuiPanel {
public:
    CSWGuiNameChargen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature);

    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    void SetParentPanel(CSWGuiPanel* pPanel, int flag);

    void HandleDoneButton(CSWGuiControl* pControl);
    void HandleRandomName();
    void HandleCancelButton();

    CSWCCreature* m_pCreature;
    CSWGuiPanel*  m_pParentPanel;        // the pushing panel; may be null
    CSWGuiButton  m_btnEnd;
    CSWGuiEditbox m_editBox;
    CSWGuiLabel   m_lblMainTitle;
    CSWGuiLabel   m_lblSubTitle;
    CSWGuiLabel   m_lblName;             // a clone of m_lblSubTitle
    CSWGuiButton  m_btnBack;
    CSWGuiButton  m_btnRandom;
    unsigned char m_nParentKind;         // 1 for CSWGuiQuickPanel, 2 for CSWGuiCustomPanel
    // A bitfield, because both writers do read-modify-write.
    unsigned char m_bNameGenerated : 1;
    unsigned char : 7;
#if !KOTOR1  // K1 constructs no CExoString member, so its name generator sits lower
    CExoString    m_sReserved1;
#endif
    CSWNameGen    m_nameGen;             // the random-name generator, by value
};

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiNameChargen) == 0xf48, "CSWGuiNameChargen size drift");
#endif
