#ifndef CSWGUIINGAMECREDITS_H
#define CSWGUIINGAMECREDITS_H
#include "k2stdint.h"

#include "swgui.h"

#include "swguicontrol.h"

class CExoString;
class CExoStreamingSoundSource;
class CSWGuiManager;
class CSWGuiControl;

// The end-credits overlay, a scrolling column of TLK paragraphs with a fade-in per entry.
class CSWGuiInGameCredits : public CSWGuiPanel {
public:
    CSWGuiInGameCredits(CSWGuiManager* pGuiManager, const CExoString& sText);

    void Initialize();

    // nStrRef is a TLK id; bFlag re-lays the label out from its freshly measured height.
    void SetText(uint64_t nStrRef, int bFlag);

    void SetNextText();

    void UpdateFading(float f);
    void UpdateScrolling(float f);

    ~CSWGuiInGameCredits();

    void Draw(float f) override;
    void HandleInputEvent(int nType, int nArg) override;
    void OnPanelAdded() override;

    CSWGuiListBox m_listBox;
    CSWGuiLabel   m_labelText;
    CSWGuiLabel   m_labelFull;
    CExoString    m_sMusicName;

    CExoStreamingSoundSource* m_pMusic;
    uint64_t*            m_paNameStrRefs;
    float         m_fScrollAccum;
    float         m_fFadeTimer;
    int           m_nOldInputClass;
    unsigned char m_nCurrentIndex;
    int           m_nUpdateMode;
    int           m_nFadePhase;
    unsigned char m_nTextCount;
    float         m_fLineStep;
    char _reserved8a0[0x8a8 - 0x8a0];
};

#if !KOTOR1  // K1's GUI base classes are smaller (CSWGuiPanel is 0x80)
K2_LAYOUT_ASSERT(sizeof(CSWGuiInGameCredits) == 0x8a8, "CSWGuiInGameCredits size drift");
#endif

#endif // CSWGUIINGAMECREDITS_H
