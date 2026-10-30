// The boot/title panel shows the game logo plus a line of start text, and its Draw ticks a seconds
// counter and plays the "Trailer" movie every 60 s while nothing else is happening.

#ifndef CSWGUISTARTSCREEN_H
#define CSWGUISTARTSCREEN_H

#include "swgui.h"
#include "swguicontrol.h"

class CSWGuiStartScreen : public CSWGuiPanel {
public:
    CSWGuiStartScreen(CSWGuiManager* pManager);

    virtual void Draw(float f);
    virtual void HandleInputEvent(int type, int val);

    CSWGuiLabel m_labelStart;
#if !KOTOR1  // K1 has no logo label
    CSWGuiLabel m_labelLogo;
#endif

    float m_fTimer;
    // m_bResetTimer: Draw restarts m_fTimer
    enum {
        STARTSCREEN_RESET_TIMER = 0x01,
    };
    unsigned char m_bResetTimer;
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWGuiStartScreen) == 0x228, "CSWGuiStartScreen K1 size drift");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiStartScreen) == 0x420, "CSWGuiStartScreen size drift");
#endif

#endif
