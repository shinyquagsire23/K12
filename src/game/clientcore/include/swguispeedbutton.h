// A button that repeats its action while held.
#ifndef CSWGUISPEEDBUTTON_H
#define CSWGUISPEEDBUTTON_H

#include "swguicontrol.h"

class CSWGuiSpeedButton : public CSWGuiButton {
public:
    virtual void HandleLMouseDown();
    virtual void Draw(float f);
    virtual void HandleRMouseDown();

    float         m_fStep = 0.2f;
    float         m_fInitialDelay = 0.5f;
    float         m_fTimeLeft;
    enum {
        SPEEDBUTTON_REPEAT_LEFT = 0x01,
    };
    unsigned char m_bRepeatLeft;           // SPEEDBUTTON_REPEAT_LEFT: the repeat came from the left button
};

#if !KOTOR1  // K1's GUI base classes are smaller (CSWGuiPanel is 0x80)
K2_LAYOUT_ASSERT(sizeof(CSWGuiSpeedButton) == 0x278, "wall pitch: speed button stride");
#endif

#endif // CSWGUISPEEDBUTTON_H
