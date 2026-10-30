#pragma once

// The character generator's "custom character" page walks the player through portrait, abilities,
// skills, feats, name and play, one row at a time.

class CSWCCreature;
class CSWGuiManager;

#include "swgui.h"
#include "swguicontrol.h"

class CSWGuiControl;
class CExoString;  // GetFullScreenBG's return type

class CSWGuiCustomPanel : public CSWGuiPanel {
public:
    CSWGuiCustomPanel(CSWGuiManager* pGuiManager, CSWCCreature* pCreature);

    virtual void HandleInputEvent(int type, int val);

    CSWCCreature* m_pCreature;    // the creature the page is editing
    CSWGuiPanel* m_pParentPanel;
    CSWGuiLabelHilight m_aHilights[6];
    CSWGuiLabel        m_aLabels[6];
    CSWGuiButton m_aButtons[6];
#if KOTOR1
    CSWGuiLabel  m_lblBg;         // K1: LBL_BG
#endif
    CSWGuiButton m_btnBack;  // the page's commit button: SetState enables and colours it
    CSWGuiButton m_btnCancel;
    int          m_nCurrentRow;    // the row the page is currently on

    void SetParentPanel(CSWGuiPanel* pParentPanel);

    void PlayGame();
    void CancelledByChild();
    void CancelPanel();
    void ClearAbilities();
    void NameCreationCompleted();
    void PortraitCreationCompleted();
    // The control hooks. All are registered as event handlers, so each takes the control that fired;
    // OnCancelPressed ignores it.
    void OnSelectPortraitButton(CSWGuiControl* pControl);
    void OnSelectPlayButton(CSWGuiControl* pControl);
    void OnSelectAbilitiesButton(CSWGuiControl* pControl);
    void OnSelectSkillsButton(CSWGuiControl* pControl);
    void OnSelectFeatsButton(CSWGuiControl* pControl);
    void OnSelectNameButton(CSWGuiControl* pControl);
    void OnCancelPressed(CSWGuiControl* pControl);
    void ConfirmCancel(CSWGuiControl* pControl);
    void AbilityCreationCompleted();
    void ClearFeats();
    void SkillCreationCompleted();
    void FeatCreationCompleted();
    void SetState(int nState);  // switches the page to the given row

    CExoString GetCurrentName();
};

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiCustomPanel) == 0x2c00, "CSWGuiCustomPanel size drift");
#endif
