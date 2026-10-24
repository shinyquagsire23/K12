#ifndef CSWGUIINGAMEPAUSE_H
#define CSWGUIINGAMEPAUSE_H

// The pause panel CGuiInGame holds as m_pPauseGuiPanel.

#include "swgui.h"

#include "swguicontrol.h"

class CSWGuiManager;
class CSWGuiControl;

class CSWGuiInGamePause : public CSWGuiPanel {
public:
    CSWGuiInGamePause(CSWGuiManager* pGuiManager);

    void OnUnpauseButtonPressed(CSWGuiControl* pControl);

    // Shows the given pause reason: picks its text and label visibility, then lays out and resizes
    // the panel.
    void SetPauseReason(unsigned char reason);

    virtual void Draw(float f);
#if !KOTOR1  // K1 does not override this slot
    virtual void HandleInputEvent(int type, int val);
#endif
    virtual void OnPanelAdded();
#if !KOTOR1  // K2-only slot
    virtual bool ShouldFreeMouseOnAdd() { return false; }
#endif

    CSWGuiLabel  m_label1;    // "LBL_PAUSEREASON"
    CSWGuiLabel  m_label2;    // "LBL_PRESS", the pad-glyph row
    CSWGuiButton m_button;    // "BTN_UNPAUSE"
    // The panel extent, kept as two ints holding the lanes of the swapped float pair.
    int          m_nExtentY;
    int          m_nExtentX;
};

#if !KOTOR1  // K2 sizes: K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiInGamePause) == 0x688, "CSWGuiInGamePause size drift");
#endif

#endif // CSWGUIINGAMEPAUSE_H
