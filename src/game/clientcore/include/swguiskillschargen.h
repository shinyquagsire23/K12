// CSWGuiSkillsCharGen -- the skill page of the character generator (create mode) and of the
// level-up flow (level-up mode).

#ifndef CSWGUISKILLSCHARGEN_H
#define CSWGUISKILLSCHARGEN_H

#include "swgui.h"
#include "swguicontrol.h"
#include "exostring.h"
#if KOTOR1
#include "IosFloatingButtonIcon.h"
#endif

class CSWCCreature;
class CSWGuiControl;
class CSWGuiManager;

// The same two-mode design and base as CSWGuiAbilitiesCharGen, with eight skill rows instead of six
// ability rows.
class CSWGuiSkillsCharGen : public CSWGuiPanel {
public:

    CSWGuiSkillsCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature, int nMode);

    virtual ~CSWGuiSkillsCharGen();

    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    void OnEnterPointsButton(CSWGuiControl* pControl);
    void OnExitPointsButton(CSWGuiControl* pControl);
    void OnMinusButtonPushed(CSWGuiControl* pControl);
    void OnPlusButtonPushed(CSWGuiControl* pControl);
    void ConfirmAccept(CSWGuiControl* pControl);

    // True when the skill is a class skill for the creature: checked against the class-skill feat
    // first, then against every class the creature has.
    int  IsClassSkill(unsigned short nSkill);

    void HandleCancelButton();
    void OnMinusButton();
    void OnPlusButton();
    void OnAcceptButton();
    void OnRecommendButton();
    // Takes a prebuilt string, not a strref: the page's strrefs were resolved once in the
    // constructor and cached in m_asSkillDescriptions.
    void SetDescription(const CExoString& sDescription);
    void CompletePanel();

    CSWCCreature* m_pCreature;
    CSWGuiPanel*  m_pParentPanel;

    CSWGuiListBox m_listBox;

    CSWGuiLabel m_lblTitle;
    CSWGuiLabel m_lblSubTitle;
    CSWGuiLabel m_lblRemainingBox;
    CSWGuiLabel m_lblSelections;
    CSWGuiLabel m_lblSelRemaining;
#if KOTOR1
    CSWGuiLabel m_lblDesc;
#endif
    CSWGuiLabel m_lblCost;
    CSWGuiLabel m_lblCostPoints;
    CSWGuiLabel m_lblClassSkill;
    CSWGuiLabel m_lblDescRow;
    CSWGuiLabel m_aSkillLabels[8];

    CSWGuiButton m_aPointsButtons[8];
    CSWGuiButton m_btnAccept;
    CSWGuiButton m_btnBack;
    CSWGuiButton m_btnRecommended;
    CSWGuiButton m_aPlusButtons[8];
    CSWGuiButton m_aMinusButtons[8];
#if !KOTOR1  // K2-only: K1 keeps the minus buttons and the description strings adjacent
    CSWGuiLabel  m_lblBar1;
    CSWGuiLabel  m_lblBar2;
#endif

    // One resolved description string per skill, in the same order as m_aSkillLabels. Filled once,
    // in the constructor, from CSWSkill::GetDescriptionText().
    CExoString m_asSkillDescriptions[8];

    int m_nPoints;
    int m_nSelectedRow;
    // Displayed rank per skill, which is not always the creature's stored rank. It is clamped to
    // the class-skill bonus and to the level-up rules, and rebuilt on every repaint.
    unsigned char m_abSkillRank[8];

    // Three 1-bit flags.
    unsigned char m_bLevelUp  : 1;    // set from the constructor's third argument
    unsigned char m_bCanModify : 1;   // always in create mode; in level-up mode only when the
                                      // server creature is in a valid single-PC state
    unsigned char m_bIsDroid  : 1;    // the creature's race is the droid race, which greys out the
                                      // droid-unusable skills
    unsigned char _bits6730_3 : 5;
    int m_nSkillPointsPerLevel;
#if KOTOR1
    IosFloatingButtonIcon m_iconDescScroll;
#endif
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiListBox) == 0x448, "CSWGuiListBox size drift");
K2_LAYOUT_ASSERT(sizeof(CSWGuiLabel) == 0x1b8, "CSWGuiLabel size drift");
K2_LAYOUT_ASSERT(sizeof(CSWGuiButton) == 0x268, "CSWGuiButton size drift");
#endif
K2_LAYOUT_ASSERT(sizeof(CExoString) == 0x10, "CExoString size drift");
#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiSkillsCharGen) == 0x6738, "CSWGuiSkillsCharGen size drift");
#endif

#endif // CSWGUISKILLSCHARGEN_H
