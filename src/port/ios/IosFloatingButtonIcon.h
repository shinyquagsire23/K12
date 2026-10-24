#pragma once

// The iOS port's free-floating gamepad glyph, a button hung off an anchor control's top-left
// corner. It adds no fields and no destructor of its own.

#include "swguicontrol.h"
#include "swgui.h"

class CSWGuiPanel;

class IosFloatingButtonIcon : public CSWGuiButton {
public:
    IosFloatingButtonIcon();

    virtual void Draw(float f);

    void InitAndAdd(CSWGuiPanel* pPanel, char nIcon, CSWGuiExtent extent,
                    int nOffsetX, int nOffsetY);
#if KOTOR1 && !defined(K2_UI_TOUCH)
    void InitAndAddCorner(CSWGuiPanel* pPanel, char nIcon, const CSWGuiExtent& extent);
#endif
};

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(IosFloatingButtonIcon) == 0x268, "IosFloatingButtonIcon size drift");
#endif
