#include "IosFloatingButtonIcon.h"

#include "swgui.h"
#include "swguidrawable.h"
#include "exoresman.h"
#include "exostring.h"
#include "port/k1gui.h"

bool ASLPlat_IsControllerPlugged();

// Emitted out of line: the body is empty but still stores the vptr, because the implicit
// CSWGuiButton constructor makes it non-trivial.
IosFloatingButtonIcon::IosFloatingButtonIcon() {}

// The icon's only own virtual. The glyph is hidden outright when no gamepad is connected.
void IosFloatingButtonIcon::Draw(float f)
{
    if (ASLPlat_IsControllerPlugged())
        CSWGuiButton::Draw(f);
}

// The glyph resrefs are the only difference from AspyrFloatingButtonIcon::InitAndAdd: this port has
// one switch (no icon-set branch) and its four names are ios_button_*.
void IosFloatingButtonIcon::InitAndAdd(CSWGuiPanel* pPanel, char nIcon, CSWGuiExtent extent,
                                       int nOffsetX, int nOffsetY)
{
#if KOTOR1
    static float s_fScale = g_SDL_mode.h / 768.0f;
#endif
    CResRef resRef;
    switch (nIcon)
    {
    case 'a': resRef = CResRef("ios_button_a"); break;
    case 'x': resRef = CResRef("ios_button_x"); break;
    case 'b': resRef = CResRef("ios_button_b"); break;
    case 'y': resRef = CResRef("ios_button_y"); break;
#ifdef K2_QOL_CONTROLLER
    case 'l': resRef = CResRef("ios_button_l"); break;   // the shoulder buttons (padglyphs.py)
    case 'r': resRef = CResRef("ios_button_r"); break;
#endif
    }

    // A fixed 48x48 glyph, hung off the anchor control's top-left corner by the given offset.
    CSWGuiExtent iconExtent;
#if KOTOR1
    // K1: the offset and the glyph size are scaled to the screen height
    iconExtent.x = static_cast<int>(s_fScale * nOffsetX + extent.x);
    iconExtent.y = static_cast<int>(s_fScale * nOffsetY + extent.y);
    iconExtent.w = static_cast<int>(s_fScale * 48.0f);
    iconExtent.h = static_cast<int>(s_fScale * 48.0f);
#else
    iconExtent.x = extent.x + nOffsetX;
    iconExtent.y = extent.y + nOffsetY;
    iconExtent.w = 48;
    iconExtent.h = 48;
#endif
    SetExtent(iconExtent);
    m_text.m_params.SetText(CExoString(""));
    m_border.m_params.SetFillImage(resRef, 0);
    m_borderHilight.m_params.SetFillImage(resRef, 0);
    m_tail.bFocusOnHover = 0;
    SetEnabled(0);
    pPanel->AddControl(this);
    pPanel->BringControlToFront(this);
}

#if KOTOR1 && !defined(K2_UI_TOUCH)
void IosFloatingButtonIcon::InitAndAddCorner(CSWGuiPanel* pPanel, char nIcon,
                                             const CSWGuiExtent& extent)
{
    InitAndAdd(pPanel, nIcon, extent, 0, 0);
    CSWGuiExtent corner;
    corner.w = corner.h = extent.h * 11 / 20;
    corner.x = extent.x + extent.w - corner.w * 3 / 4;
    corner.y = extent.y + extent.h - corner.h * 3 / 4;
    SetExtent(corner);
}
#endif
