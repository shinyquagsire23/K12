#pragma once

// The character-generation portrait-picker GUI screen.

#include "swgui.h"
#include "swguicontrol.h"
#include "CExoArrayListDtors.h"

class CSWGuiControl;
struct CSWGuiExtent;
class CSWCCreature;
class CSWGuiLabelHilight;
class CExoString;

class CSWGuiPortraitCharGen : public CSWGuiPanel {
public:
    // Declared only: CSWGuiCustomPanel::OnSelectPortraitButton is the one call site.
    CSWGuiPortraitCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature);

    virtual void Draw(float f);
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    void GetPortraitsList(int nSex);
    void SetParentPanel(CSWGuiPanel* pPanel, int flag);
    void FlashArrow(CSWGuiLabelHilight& hilight);
    void UpdatePortraitButton();
    void OnSelectDisplayButton();
    void HandleCancelButton();
    void HandleLeftButton();
    void HandleRightButton();

    CSWCCreature*  m_pCreature;
    CSWGuiPanel*   m_pParentPanel;
    CSWGuiLabel    m_lblMainTitle;
    CSWGuiLabel    m_lblSubTitle;
    CSWGuiLabel    m_lblPortrait;
    CSWGuiLabel    m_lblBevel[5];
    CSWGui3DSceneView m_view3DChar;
    CSWGuiButton   m_btnAccept;
    CSWGuiButton   m_btnBack;
    CSWGuiButton   m_btnArrR;
    CSWGuiButton   m_btnArrL;
#if !KOTOR1  // K1's ctor builds no bar labels
    CSWGuiLabel    m_aBarLabels[8];
#endif
    CExoArrayList<int> m_paPortraitIds;
    CExoArrayList<int> m_paAppearanceIds;
    struct ArrowFlash {
        CSWGuiLabelHilight* m_pHilight;
        float               m_fTimer;
    };
    CExoArrayList<ArrowFlash> m_aFlashes;
    float          m_fIdleAnimTimer;      // a countdown re-rolled on expiry
    int            m_nCurrentPortrait;
    int            m_nCurrentPortraitId;
    unsigned char  m_nParentKind;

    void OnLeftArrowPressed(CSWGuiControl* pControl);
    void OnRightArrowPressed(CSWGuiControl* pControl);
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiPortraitCharGen) == 0x2700, "CSWGuiPortraitCharGen size drift");
#endif
