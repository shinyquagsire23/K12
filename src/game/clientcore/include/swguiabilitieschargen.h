#ifndef CSWGUIABILITIESCHARGEN_H
#define CSWGUIABILITIESCHARGEN_H
#include "k2stdint.h"

#include "swgui.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "Vector.h"

class CSWCCreature;
class CSWGuiControl;
class CSWGuiManager;

// The ability-score page of the character generator and of the level-up flow. In create mode the
// player allocates 30 points from scratch; in level-up mode the points come from level increases.
// The two modes differ in where the six scores come from, what they may be raised to, and which
// parent panel is told when the page is done.
//
// The six scores are kept in on-screen order, STR DEX CON WIS INT CHA, not the engine's
// STR DEX CON INT WIS CHA. Everything downstream, from the description strref index to the cost
// tables and the base-score setters, indexes that display order.
class CSWGuiAbilitiesCharGen : public CSWGuiPanel {
public:
    CSWGuiAbilitiesCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature, int nMode);

    virtual ~CSWGuiAbilitiesCharGen();

    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    void OnEnterPointsButton(CSWGuiControl* pControl);
    void OnExitPointsButton(CSWGuiControl* pControl);
    void OnMinusButtonPushed(CSWGuiControl* pControl);
    void OnPlusButtonPushed(CSWGuiControl* pControl);

    void HandleCancelButton();
    void OnMinusButton();
    void OnPlusButton();
    void OnAcceptButton();
    void OnRecommendButton();
    int  DetermineCostToDecreaseScore(int nScore);
    int  DetermineCostToIncreaseScore(int nScore);
    void SetDescription(unsigned int nStrRef);
    void CompletePanel();

    CSWCCreature* m_pCreature;
    CSWGuiPanel*  m_pParentPanel;

    CSWGuiListBox m_listBox;

    CSWGuiLabel m_lblTitle;
    CSWGuiLabel m_lblSubTitle;
    CSWGuiLabel m_lblRemainingBox;
    CSWGuiLabel m_lblPoints;
    CSWGuiLabel m_lblSelections;
#if KOTOR1
    CSWGuiLabel m_lblDesc;
#endif
    CSWGuiLabel m_lblCost;
    CSWGuiLabel m_lblCostPoints;
    // The one control never bound by name. The list box's prototype item is copied onto it, and it
    // becomes LB_DESC's only row.
    CSWGuiLabel m_lblDescRow;
#if KOTOR1
    CSWGuiLabel m_lblAbilityMod;
    CSWGuiLabel m_lblModifier;
#endif
    CSWGuiLabel m_aStatLabels[6];

    CSWGuiButton m_aPointsButtons[6];
    CSWGuiButton m_btnAccept;
    CSWGuiButton m_btnBack;
    CSWGuiButton m_btnRecommended;
    CSWGuiButton m_aPlusButtons[6];
    CSWGuiButton m_aMinusButtons[6];
#if !KOTOR1  // K1 has no bar or bonus columns
    CSWGuiLabel  m_aBarLabels[4];
    CSWGuiLabel  m_aBonusLabels[6];
#endif

    // The tier-name strrefs, in row order.
    uint64_t m_anStatStrRefs[6];
    // Points left to spend, or in level-up mode "(level & 3) == 0", which is the gate for the
    // one-point-per-level increase. Accept refuses to complete while this is positive.
    int m_nPoints;
    int m_anStatValues[6];
    int m_anBaseValues[6];
    // The row the mouse is over, 0..5.
    int m_nHoveredRow;
    int m_nMode;
#if !KOTOR1  // K1's object ends at m_nMode
    CSWGuiLabel  m_lblPortrait;
    // Six colour triples, where rows 0/1 are the stat caption's normal and highlight colour, 2/3
    // the stat row's border pair and 4/5 the plus button's border pair.
    Vector m_avRowColors[6];
#endif
};

#if !KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWGuiListBox) == 0x448, "CSWGuiListBox size drift");
K2_LAYOUT_ASSERT(sizeof(CSWGuiAbilitiesCharGen) == 0x6338, "CSWGuiAbilitiesCharGen size drift");
#endif

#endif
