#pragma once

#ifndef CSWGUIBGAREATRANSITION_H
#define CSWGUIBGAREATRANSITION_H

#include "swgui.h"
#include "swguicontrol.h"
#include "Vector.h"

class CSWGuiControl;
class CSWGuiManager;

// The "now entering <area>" banner CGuiInGame overlays in game. It never grabs the mouse and its only
// state is the object id and position it is fed.
class CSWGuiInGameAreaTransition : public CSWGuiPanel {
public:
    // Binds the areatrans_p layout, stands the three labels up off it, saves the layout's colours and
    // centres the banner on the manager's screen width.
    CSWGuiInGameAreaTransition(CSWGuiManager* pGuiManager);

    virtual void Draw(float f);

    // An overlay never frees the pointer.
#if !KOTOR1  // K2-only slot (K1 CSWGuiPanel has none)
    virtual bool ShouldFreeMouseOnAdd() { return false; }
#endif

    // Sets the tracked position (the area-transition hook) and, when the object id changes,
    // re-labels the banner from the new object's name (a `-` splits the resref off).
    void SetTransitionObject(unsigned long long objId, const Vector& vPos);

    CSWGuiLabel m_lblTextBG;       // LBL_TEXTBG, the dark plate behind the text
    CSWGuiLabel m_lblIcon;         // LBL_ICON
    CSWGuiLabel m_lblDescription;  // LBL_DESCRIPTION, the area name right of the icon

    Vector m_position;
    unsigned long long m_objId;   // OBJECT_INVALID when unset
    int m_nBannerY;               // Draw's fallback y: the banner's resting row

#if !KOTOR1  // K1 draws with the fixed palette and keeps no copies of the colours
    Vector m_vTextBGColor;
    Vector m_vIconColor;
    Vector m_vDescColor;
#endif
};

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiInGameAreaTransition) == 0x610, "CSWGuiInGameAreaTransition size drift");
#else
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiInGameAreaTransition, m_nBannerY) == 0x578, "K1 layout (ctor 0x3257b8)");
K2_LAYOUT_ASSERT(sizeof(CSWGuiInGameAreaTransition) == 0x580, "K1 CSWGuiInGameAreaTransition size");
#endif

#endif // CSWGUIBGAREATRANSITION_H
