#pragma once

#include "swgui.h"
#include "swguicontrol.h"

class CSWCCreature;
class CSWGuiManager;
class CSWGuiControl;
class CExoString;

// The char-gen "quick character" page with three preset rows, the sibling of CSWGuiCustomPanel.
class CSWGuiQuickPanel : public CSWGuiPanel {
public:
    CSWGuiQuickPanel(CSWGuiManager* pGuiManager, CSWCCreature* pCreature);

    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    CSWCCreature* m_pCreature;
    CSWGuiPanel*  m_pParentPanel;
#if KOTOR1
    CSWGuiLabel   m_lblDecoration;  // K1 only, "LBL_DECORATION"
#endif
    CSWGuiLabelHilight m_aHilights[3];
    CSWGuiLabel        m_aLabels[3];
    // Three plus two, not five, because the trailing pair is indexed by a constant, which is what
    // keeps their SetEnabled call direct instead of virtual.
    CSWGuiButton       m_aButtons[3];
    CSWGuiButton       m_btnBack;
    CSWGuiButton       m_btnCancel;
    int                m_nState;

    void PlayGame();
    CExoString GetCurrentName();
    void CancelledByChild();
    void SetState(int nState);
    void PortraitCreationCompleted();
    void NameCreationCompleted();
    void ConfirmCancel(CSWGuiControl* pControl);

    void DoQuickStats();
    void CancelPanel();
    void OnSelectPortraitButton(CSWGuiControl* pControl);
    void OnSelectNameButton(CSWGuiControl* pControl);
    void OnSelectPlayButton(CSWGuiControl* pControl);
    void OnCancelPressed(CSWGuiControl* pControl);
};

#if !KOTOR1  // K1's GUI base classes are smaller (CSWGuiPanel is 0x80)
K2_LAYOUT_ASSERT(sizeof(CSWGuiQuickPanel) == 0x18c8, "CSWGuiQuickPanel size drift");
#else  // K1 size, larger GUI base classes
K2_LAYOUT_ASSERT(sizeof(CSWGuiQuickPanel) == 0x18f8, "CSWGuiQuickPanel K1 size drift");
#endif
