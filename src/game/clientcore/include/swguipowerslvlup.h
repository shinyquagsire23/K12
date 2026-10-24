// CSWGuiPowersLevelUp -- the force powers page the level-up flow opens from
// CSWGuiLevelUpPanel::OnSelectPowersButton.

#ifndef CSWGUIPOWERSLEVELUP_H
#define CSWGUIPOWERSLEVELUP_H

#include "swgui.h"
#include "swguicontrol.h"
#include "exostring.h"
#include "swguiskillflowchart.h"
#include "CExoArrayListDtors.h"

class CSWCCreature;
class CSWGuiControl;
class CSWGuiManager;

// Level-up-only sibling of CSWGuiFeatsCharGen, with the same panel shape and four-list model, but
// the payload ids are force powers (unsigned int) and the page has no create-mode arm. Its "am I a
// level-up page" bit comes from the server creature rather than a constructor argument.
class CSWGuiPowersLevelUp : public CSWGuiPanel {
public:

    // The second argument is the creature's game-object id; the constructor resolves the creature
    // itself through GetCreatureByGameObjectID.
    CSWGuiPowersLevelUp(CSWGuiManager* pGuiManager, unsigned long long objectId);

    virtual ~CSWGuiPowersLevelUp();

    virtual void Draw(float f);
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    // Rebuilds every list and the chart from the creature's current stats.
    void InitiatePowers();

    void OnPowerSelectionChanged(CSWGuiControl* pControl);
    void OnDoubleClick(CSWGuiControl* pControl);

#if !KOTOR1
    void ShowGranted(CSWGuiControl* pControl);
#endif

    void OnAcceptButton();
    void BuildAvailableList();
    void BuildButtons();
    void HandleCancelButton();
    void OnPowerPicked(unsigned int nPower);
    void OnRecommendButton();
    void OnEnterPower(unsigned int nPower);
    void RemoveChosenPower(unsigned int nPower);
    void AddChosenPower(unsigned int nPower);
    // Reports which of the four power lists holds nPower, as the draw state Draw switches on.
    void DeterminePower(unsigned int nPower, unsigned char& nStatus);
    void SetDescription(unsigned int nStrRef);

    CSWCCreature* m_pCreature;
    CSWGuiPanel*  m_pParentPanel;

    CSWGuiLabel m_lblTitle;
    CSWGuiLabel m_lblSubTitle;
    CSWGuiLabel m_lblRemainingBox;
    CSWGuiLabel m_lblSelections;
    CSWGuiLabel m_lblSelRemaining;
#if KOTOR1
    CSWGuiLabel m_lblDesc;
#endif
    CSWGuiLabel m_lblReserved1;
    CSWGuiLabel m_lblReserved2;
    CSWGuiLabel m_lblDescRow;
    CSWGuiLabel m_lblPower;

    CSWGuiListBox m_listBoxPowers;
    CSWGuiListBox m_listBoxDesc;

    // The button order differs from the feats page's: recommended, select, accept, back.
    CSWGuiButton m_btnRecommended;
    CSWGuiButton m_btnSelect;
    CSWGuiButton m_btnAccept;
    CSWGuiButton m_btnBack;
#if !KOTOR1  // K2-only: K1 has no bar labels
    CSWGuiLabel  m_aBars[2];
#endif

    unsigned char m_nSelectionsLeft;
    // One more than m_nSelectionsLeft: the ceiling CanChooseSpell validates the chosen list against.
    unsigned char m_nSelectionsMax;

    // Bit 0 is the level-up flag, seeded from the server stats' m_bIsPC; no other bit is used.
    enum {
        POWERS_FLAG_IS_PC = 0x01,
    };
    unsigned char m_bFlags;
    unsigned char _pad25d3[0x25d4 - 0x25d3];

    unsigned int m_nSelectedPower;

#if !KOTOR1  // K2-only: K1 has no granted-powers box
    // Bit 0 records that the granted-powers box has been pushed for this build.
    enum {
        POWERS_GRANTED_BOX_PUSHED = 0x01,
    };
    unsigned char m_bGrantedPushed;
    unsigned char _pad25d9[0x25e0 - 0x25d9];
#endif

    CExoString m_sPowerAvailableText;
    CExoString m_sPowerTakenText;

#if KOTOR1
    CExoArrayList<unsigned int> m_aPowerLists[3];
#else
    CExoArrayList<unsigned int> m_aPowerLists[4];
#endif
#define m_aOwnedPowers m_aPowerLists[0]
#define m_aAvailablePowers m_aPowerLists[1]
#define m_aChosenPowers m_aPowerLists[2]
#if !KOTOR1
#define m_aNewPowers m_aPowerLists[3]
#endif

    CSWGuiSkillFlowChart m_powerChart;

    // m_nDrawState: which list DeterminePower found the focused power in.
    enum {
        DRAW_STATE_AVAILABLE = 0,
        DRAW_STATE_CHOSEN = 1,
        DRAW_STATE_OWNED = 2,
        DRAW_STATE_UNAVAILABLE = 3,
        DRAW_STATE_GRANTED = 4,
    };
    unsigned char m_nDrawState;
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiPowersLevelUp) == 0x2660, "CSWGuiPowersLevelUp size drift");
#endif

#endif // CSWGUIPOWERSLEVELUP_H
