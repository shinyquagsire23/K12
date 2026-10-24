// The full-screen loading panel, a progress bar plus four labels, one of which is the screen-size
// background image the screen is named for.
#ifndef CSWGUILOADSCREEN_H
#define CSWGUILOADSCREEN_H
#include "k2stdint.h"

#include "swgui.h"
#include "swguicontrol.h"
#include "exoresman.h"

class CSWGuiManager;

class CSWGuiLoadScreen : public CSWGuiPanel {
public:
    virtual void Draw(float f);
#if !KOTOR1  // K1 does not override this slot
    virtual void OnPanelAdded();
#endif
#if !KOTOR1  // K1 does not override this slot
    virtual void OnPanelRemoved();
#endif

    CSWGuiLoadScreen(CSWGuiManager* pGuiManager);
    void SetImage(const CResRef& resRef);
    void SetHint(uint64_t nStrRef);
    void AddHintToMessageBuffer();

    CSWGuiProgressBar m_progressBar;  // InitControl("PB_PROGRESS")
    CSWGuiLabel m_lblHint;            // InitControl("LBL_HINT"); SetHint and AddHintToMessageBuffer
                                      // set its text
    CSWGuiLabel m_lblLogo;            // InitControl("LBL_LOGO"); OnPanelAdded / OnPanelRemoved
                                      // show and hide its border
    CSWGuiLabel m_lblLoading;         // InitControl("LBL_LOADING")
    uint64_t m_nHintStrRef;      // raw StrRef, 0 when no hint is showing
#if !KOTOR1  // K1: the logo keeps no copy of the background resref
    CResRef m_resRefBackground;       // the screen-size fill image ("800x600load" and the like)
#endif
    CSWGuiLabel m_lblBackground;      // the full-screen image
#if KOTOR1
    // Constructed by the ctor and destroyed by the implicit destructor, read nowhere else.
    CSWGuiImage m_imgReserved1;
#endif
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWGuiLoadScreen) == 0x918, "CSWGuiLoadScreen K1 size drift");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiLoadScreen) == 0x970, "CSWGuiLoadScreen size drift");
#endif

#endif // CSWGUILOADSCREEN_H
