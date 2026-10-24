#pragma once
#include "k2stdint.h"

// Screen-fade panel: a full-screen quad whose opacity animates from 0 to a target over a duration.

#include "swgui.h"
#include "swguicontrol.h"

struct Vector;

class CSWGuiControl;
struct CSWGuiExtent;
class CSWGuiManager;

class CSWGuiFade : public CSWGuiPanel {
public:
    CSWGuiFade(CSWGuiManager* pGuiManager);

    virtual void Draw(float f);
    virtual CSWGuiControl* HitCheckMouse(int x, int y);
#if !KOTOR1  // K2-only slot (K1 CSWGuiPanel has none)
    virtual bool ShouldFreeMouseOnAdd();
#endif
    void SetMsg(uint64_t nMsg);
    void SetFade(int nType, float fLength, float fDelay, const Vector& vColor);
    void ResetFade();
    int  IsFadeDone();

    float m_fadeElapsed;
    float m_fadeDuration;
    float m_fadeAmount;
    int   m_nFadeType;
    CSWGuiLabel m_label;
    int   m_nLastDrawTime;
};

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiFade) == 0x278, "CSWGuiFade size drift");
#endif
