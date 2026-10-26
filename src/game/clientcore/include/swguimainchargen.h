#ifndef CSWGUIMAINCHARGEN_H
#define CSWGUIMAINCHARGEN_H

#include "swgui.h"
#include "swguicontrol.h"
#include "exostring.h"

class CSWCCreature;
class CSWGuiClassSelection;
class CSWGuiManager;

class CSWGuiMainCharGen : public CSWGuiPanel {
public:

    // Builds the 33 caption labels from the MAINCG_p layout, then the three sub-pages.
    CSWGuiMainCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature,
                      CSWGuiClassSelection* pClassSelection);

    virtual ~CSWGuiMainCharGen();
    virtual void Draw(float f);
    virtual void OnPanelAdded();

    // Reached from CSWGuiCustomPanel's forwarders.
    void BeginPlayGame();
    void ClearFeats();

    void SetNameString(const CExoString& sName);
    void DetermineClass();
    void RemoveSubPanels();
    // SetVisiblePanel's page, kept in m_nVisiblePanel.
    enum {
        VISIBLE_PANEL_QUICK_OR_CUSTOM = 1,
        VISIBLE_PANEL_QUICK = 2,
        VISIBLE_PANEL_CUSTOM = 3,
        VISIBLE_PANEL_NONE = 4,
    };
    void SetVisiblePanel(int nPanel);
    void ClearAbilities();
    void AbilitiesChanged();
    void PortraitChanged();
    void NameChanged();

    CSWCCreature*         m_pCreature;
    CSWGuiClassSelection* m_pClassSelection;
    // The three pages this screen switches between; m_nVisiblePanel starts at 1.
    CSWGuiPanel*          m_pQuickOrCustomPanel;
    CSWGuiPanel*          m_pQuickPanel;
    CSWGuiPanel*          m_pCustomPanel;
    // The 33 captions, deliberately separate members rather than an array.
    CSWGuiLabel           m_lblTitle;
    CSWGuiLabel           m_lblName;
    CSWGuiLabel           m_lblClass;
    CSWGuiLabel           m_lblStr;
    CSWGuiLabel           m_lblDex;
    CSWGuiLabel           m_lblCon;
    CSWGuiLabel           m_lblInt;
    CSWGuiLabel           m_lblWis;
    CSWGuiLabel           m_lblCha;
    CSWGuiLabel           m_lblStrAb;
    CSWGuiLabel           m_lblDexAb;
    CSWGuiLabel           m_lblConAb;
    CSWGuiLabel           m_lblIntAb;
    CSWGuiLabel           m_lblWisAb;
    CSWGuiLabel           m_lblChaAb;
    CSWGuiLabel           m_lblVit;
    CSWGuiLabel           m_lblDef;
    CSWGuiLabel           m_lblNewWill;
    CSWGuiLabel           m_lblNewRefl;
    CSWGuiLabel           m_lblNewFort;
    CSWGuiLabel           m_lblPortrait;
    CSWGuiLabel           m_lblBevelL;
    CSWGuiLabel           m_lblBevelM;
    CSWGuiLabel           m_lblBevelR;
    CSWGuiLabel           m_lblFortitude;
    CSWGuiLabel           m_lblReflex;
    CSWGuiLabel           m_lblWill;
#if !KOTOR1  // K2-only labels; K1 builds 27 and stops at LBL_WILL
    CSWGuiLabel           m_lblStatsBack;
    CSWGuiLabel           m_lblStatsBorder;
    CSWGuiLabel           m_lblPortBorder;
    CSWGuiLabel           m_lblVitName;
    CSWGuiLabel           m_lblDefName;
    CSWGuiLabel           m_lblBar;
#endif
    // The live character preview.
    CSWGui3DSceneView     m_sceneView;
    // Which page is showing: 1, 2 or 3; 4 means none.
    int                   m_nVisiblePanel;
    CExoString            m_sClasses;
    CExoString            m_sName;
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiMainCharGen) == 0x3a98, "CSWGuiMainCharGen size drift");
#endif

#include "nwccreature.h"

// Shows the creature's current name in the name row.
inline void CSWGuiMainCharGen::NameChanged()
{
    SetNameString(m_pCreature->GetName());
}

#endif // CSWGUIMAINCHARGEN_H
