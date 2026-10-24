// The feats page of the character generator (create mode) and of the level-up flow (level-up mode).

#ifndef CSWGUIFEATSCHARGEN_H
#define CSWGUIFEATSCHARGEN_H

#include "swgui.h"
#include "swguicontrol.h"
#include "exostring.h"
#include "swguiskillflowchart.h"
#include "CExoArrayListDtors.h"

class CSWCCreature;
class CSWGuiControl;
class CSWGuiManager;

// Sibling of CSWGuiSkillsCharGen and CSWGuiAbilitiesCharGen. The page owns the feat lists, the
// selection counters and a skill flow chart that lays the feats out as a grid.
class CSWGuiFeatsCharGen : public CSWGuiPanel {
public:
    // nMode selects create (0) or level-up (1) mode; nBonus is the level-up driver's "this level
    // granted no normal slots" flag. The character generator passes 0, 0.
    CSWGuiFeatsCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature, int nMode, int nBonus);

    virtual ~CSWGuiFeatsCharGen();

    virtual void Draw(float f);
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    // Rebuilds every list and the chart from the creature's current stats.
    void InitiateFeats();

    void OnFeatSelectionChanged(CSWGuiControl* pControl);
    void OnDoubleClick(CSWGuiControl* pControl);
    void ShowGranted(CSWGuiControl* pControl);

    void OnAcceptButton();
    void BuildAvailableList();
    void BuildButtons();
    void HandleCancelButton();
    void OnFeatPicked(unsigned short nFeat);
    void OnRecommendButton();
    void OnEnterFeat(unsigned short nFeat);
    void RemoveChosenFeat(unsigned short nFeat);
    void AddChosenFeat(unsigned short nFeat);
    // Reports which of the four feat lists nFeat is in: 0 available, 1 chosen, 2 new, 3 owned,
    // and 4 when it is in none of them.
    void DetermineFeat(unsigned short nFeat, unsigned char& nStatus);
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
    CSWGuiLabel m_lblName;

    CSWGuiButton m_btnAccept;
    CSWGuiButton m_btnBack;
    CSWGuiButton m_btnRecommended;
    CSWGuiButton m_btnSelect;
#if !KOTOR1  // K2-only: K1 has no bar labels
    CSWGuiLabel  m_aBars[2];
#endif

    CSWGuiListBox m_listBoxFeats;
    CSWGuiListBox m_listBoxDesc;

    CExoArrayList<unsigned short> m_aOwnedFeats;
    CExoArrayList<unsigned short> m_aNewFeats;
    CExoArrayList<unsigned short> m_aAvailableFeats;
    CExoArrayList<unsigned short> m_aChosenFeats;

    unsigned short m_nSelectedFeat;
    unsigned char  m_nNormalSlots;
    unsigned char  m_nBonusSlots;
    unsigned char  m_nNormalLeft;
    unsigned char  m_nBonusLeft;
    unsigned char  m_nSelectionsLeft;

    // Mode and capability flags: bit 0 level-up mode, bit 1 bonus mode, bit 2 "the granted-feats
    // box has been pushed for this build". Bits 3-7 are unused and preserved by every update.
    enum {
        FEATS_FLAG_LEVEL_UP = 0x01,
        FEATS_FLAG_BONUS = 0x02,
        FEATS_FLAG_GRANTED_BOX_PUSHED = 0x04,
    };
    unsigned char m_bFlags;

    CExoString m_sFeatAvailableText;
    CExoString m_sFeatTakenText;

    CSWGuiSkillFlowChart m_featChart;

    // m_nDrawState: which list OnEnterFeat found the focused feat in.
    enum {
        DRAW_STATE_AVAILABLE = 0,
        DRAW_STATE_CHOSEN = 1,
        DRAW_STATE_GRANTED = 2,
        DRAW_STATE_OWNED = 3,
        DRAW_STATE_UNAVAILABLE = 4,
    };
    unsigned char m_nDrawState;
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiFeatsCharGen) == 0x2658, "CSWGuiFeatsCharGen size drift");
#endif

#endif // CSWGUIFEATSCHARGEN_H
