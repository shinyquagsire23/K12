#include "k2stdint.h"
#include "port/aslglapi.h"
#include "port/sdlapi.h"
#include "IosWidgets.h"

#include "swguicontrol.h"
#include "exoinput.h"

void DrawTextWrapped(char const* pszText, bool bUtf8, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8, bool bCenter);

bool ASLPlat_IsControllerPlugged();

#if KOTOR1
#include "port/k1gui.h"
#endif

#include "port/libcstring.h"
#include "port/libcmath.h"
#include "IosFloatingButtonIcon.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "exoresman.h"
#include "exostring.h"

// The touch cursor's drawn size, and the buffer a reply's text is filtered into.
enum {
    TOUCH_CURSOR_SIZE = 160,
    FILTERED_TEXT_SIZE = 2048,
};

// Copies sText into pszBuffer as a NUL-terminated string. With bFilterNewline set every newline
// becomes two spaces (the label wraps instead of breaking); without it a run of three or more
// newlines collapses to two and the trailing ones are trimmed.
void FilterString(bool bFilterNewline, CExoString& sText, char* pszBuffer, int nBufferSize)
{
    const char* pSrc = sText.CStr();
    int nOut = 0;
#if KOTOR1
    // K1: without the flag the text is copied verbatim, with no run collapse.
    while (*pSrc != 0)
    {
        if (bFilterNewline && *pSrc == '\n')
        {
            pszBuffer[nOut] = ' ';
            pszBuffer[nOut + 1] = ' ';
            nOut += 2;
        }
        else
        {
            pszBuffer[nOut++] = *pSrc;
        }
        if (nOut > nBufferSize - 2)
            break;
        pSrc++;
    }
#else
    int nRun = 0;
    for (;;)
    {
        char c = *pSrc;
        if (c == 0)
            break;
        if (bFilterNewline)
        {
            if (c == '\n')
            {
                pszBuffer[nOut++] = ' ';
                c = ' ';
            }
        }
        else
        {
            if (c == '\n')
                nRun++;
            else
                nRun = 0;
            if (nRun >= 3)
            {
                pSrc++;
                continue;
            }
        }
        pszBuffer[nOut++] = c;
        if (nOut > nBufferSize - 2)
            break;
        pSrc++;
    }
#endif
    while (nOut > 0 && pszBuffer[nOut - 1] == '\n')
        nOut--;
    pszBuffer[nOut] = 0;
}

// Forwards to the ASL layer with the current viewport's pixel size and a literal false.
void IosControl::DrawTextWrapped(char const* pszText, int n1, int n2, int n3, int n4, int n5, int n6, bool bCenter)
{
    const CAurGUIDraw::Viewport& viewport =
        CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport];
    int nHeight = viewport.nHeight;
    int nWidth = viewport.nWidth;
    ::DrawTextWrapped(pszText, false, n1, nWidth, nHeight, n2, n3, n4, n5, n6, bCenter);
}

// Draws the background image over the given rectangle, then the wrapped text inset by (n2, n3).
#if KOTOR1
// K1: the inset arrives in 768-line units and is scaled to the screen first.
void IosControl::DrawTextWithBackground(CAurGUIImage* pImage, char const* pszText,
                                        int n1, int n2, int n3, int n4, int n5,
                                        int n6, int n7, int n8, bool bCenter)
{
    static float s_fScale = g_SDL_mode.h / 768.0f;
    int nPadX = static_cast<int>(s_fScale * n2);
    int nPadY = static_cast<int>(s_fScale * n3);
    if (pImage != 0)
    {
        const Vector vTint(-1.0f, -1.0f, -1.0f);
        static_cast<CAurGUIImageInternal*>(pImage)->Draw(n4, n5, n6, n7, 0, 0.0f, vTint, 1.0f);
    }

    const CAurGUIDraw::Viewport& viewport =
        CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport];
    int nHeight = viewport.nHeight;
    int nWidth = viewport.nWidth;
    ::DrawTextWrapped(pszText, false, n1, nWidth, nHeight,
                      nPadX + n4, nPadY + n5, n6 - 2 * nPadX, n7 - 2 * nPadY, n8, bCenter);
}
#else
void IosControl::DrawTextWithBackground(CAurGUIImage* pImage, char const* pszText,
                                        int n1, int n2, int n3, int n4, int n5,
                                        int n6, int n7, int n8, bool bCenter)
{
    if (pImage != 0)
    {
        // Draw lives on the derived CAurGUIImageInternal; the downcast is a no-op.
        const Vector vTint(-1.0f, -1.0f, -1.0f);
        static_cast<CAurGUIImageInternal*>(pImage)->Draw(n4, n5, n6, n7, 0, 0.0f, vTint, 1.0f);
    }

    const CAurGUIDraw::Viewport& viewport =
        CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport];
    int nHeight = viewport.nHeight;
    int nWidth = viewport.nWidth;
    ::DrawTextWrapped(pszText, false, n1, nWidth, nHeight,
                      n2 + n4, n3 + n5, n6 - 2 * n2, n7 - 2 * n3, n8, bCenter);
}
#endif

#if KOTOR1
// Defined ahead of the constructor that uses it.
void IosLabel::SetFontSize(int nSize) {
    static float s_fScale = g_SDL_mode.h / 768.0f;
    m_nFontSize = static_cast<int>(s_fScale * nSize);
}
#endif

IosLabel::IosLabel()
#if KOTOR1
{
    m_pImage = 0;
    SetFontSize(24);
    m_nMaxLines = 4;
    SetImage(const_cast<char*>("ipho_dialog"));
    m_bFilterNewline = 1;
    m_bCenterText = 1;
}
#else
{
    m_pImage = 0;
    m_nMaxLines = 12;
    m_nFontSize = 24;
    m_pImage = NewCAurGUIImage(const_cast<char*>("ipho_dialog"));
    m_bFilterNewline = 1;
    m_bCenterText = 1;
    m_fScaleX = 1.0f;
    m_fScaleY = 1.0f;
}
#endif

IosLabel::~IosLabel()
{
    delete m_pImage;
    m_pImage = 0;
}

// Paints the label's background image and its text, with the pads derived from the font size.
void IosLabel::Draw(float fAlpha)
#if KOTOR1
// K1: fixed 20/10 insets instead of the font-derived pads.
{
    K2_UNUSED(fAlpha);
    DrawTextWithBackground(m_pImage, m_sText.CStr(), m_nFontSize, 20, 10,
                           m_extent.x, m_extent.y, m_extent.w, m_extent.h,
                           m_nMaxLines, m_bCenterText != 0);
}
#else
{
    K2_UNUSED(fAlpha);
    DrawTextWithBackground(m_pImage, m_sText.CStr(), m_nFontSize,
                           static_cast<int>(m_fScaleX * 15.0f), static_cast<int>(m_fScaleY * 8.0f),
                           m_extent.x, m_extent.y, m_extent.w, m_extent.h,
                           m_nMaxLines, m_bCenterText != 0);
}
#endif

#if KOTOR1
void IosLabel::SetExtent(const CSWGuiExtent& extent)
{
    CSWGuiObject::SetExtent(extent);
}
#endif

// Walks the container chain to the owning panel's GUI manager and plays the widget's click sound.
void IosLabel::PlayClickSound()
{
    if (m_pContainer == 0)
        return;

    CSWGuiManager* pManager = 0;
    if (m_pContainer->AsSWGuiPanel() != 0)
        pManager = m_pContainer->AsSWGuiPanel()->m_pGuiManager;
    else if (m_pContainer->AsSWGuiListBox() != 0)
        pManager = m_pContainer->AsSWGuiListBox()->m_pContainer->AsSWGuiPanel()->m_pGuiManager;

    if (pManager != 0)
        pManager->PlayGuiSound(m_tail.m_nClickSound);
}

// Horizontal reference width of the reply band.
float IosDialogReplies::GetWidthScale() const
{
    return static_cast<float>(m_extent.w) / 860.0f;
}

// Returns the vertical reference height; the iPad layout uses a different band height from the
// phone one.
float IosDialogReplies::GetHeightScale() const
{
    return static_cast<float>(m_extent.h) / (m_bIpadLayout ? 300.0f : 478.0f);
}

#if !KOTOR1
// Reply-band frame insets, in panel-local pixels. They are read through volatile so the values
// stay loads rather than folded constants; the NoBg set has the larger top and side insets.
static volatile int s_nReplyTopBg      = 42;
static volatile int s_nReplyBottomBg   = 50;
static volatile int s_nReplySideBg     = 30;
static volatile int s_nReplyTopNoBg    = 61;
static volatile int s_nReplyBottomNoBg = 48;
static volatile int s_nReplySideNoBg   = 44;

// The scrollbar track insets, top and bottom off the content frame's height, width and right edge
// off its width. Unlike the reply insets these have no no-background variant.
static volatile int s_nBarTop    = 54;
static volatile int s_nBarBottom = 67;
static volatile int s_nBarWidth  = 8;
static volatile int s_nBarRight  = 14;
#endif

// Returns the reply area inside the panel, in panel-local pixels, as the base m_extent inset by the
// metrics above, scaled to the screen. bAbsolute additionally adds the panel's own origin,
// converting the frame to screen coordinates.
#if KOTOR1
// K1: one metric set, scaled from 768 lines.
CSWGuiExtent IosDialogReplies::GetContentFrame(int bAbsolute)
{
    static float s_fScale = g_SDL_mode.h / 768.0f;
    static int s_nInsetY = static_cast<int>(s_fScale * 36.0f);
    static int s_nTop = static_cast<int>(s_fScale * 42.0f);
    static int s_nSide = static_cast<int>(s_fScale * 30.0f);
    CSWGuiExtent frame;
    int nY = 0;
    if (bAbsolute) {
        frame.x = s_nSide + m_extent.x;
        nY = m_extent.y;
    } else {
        frame.x = s_nSide;
    }
    frame.y = s_nTop + nY;
    frame.w = m_extent.w - 2 * s_nSide;
    frame.h = static_cast<int>(static_cast<float>(m_extent.h - 2 * s_nInsetY) + s_fScale * -10.0f);
    return frame;
}

// K1: one column; the row height is a scaled 72 lines and is also the returned height.
CSWGuiExtent IosDialogReplies::GetReplyExtent(int bAbsolute, int nIndex)
{
    static float s_fScale = g_SDL_mode.h / 768.0f;
    CSWGuiExtent extent = GetContentFrame(bAbsolute);
    int nScroll = 0;
    if (bAbsolute)
        nScroll = m_nVertScroll;
    const int nRowH = static_cast<int>(s_fScale * 72.0f);
    extent.y = (m_replyLayout.h + nRowH) * nIndex + (nScroll + extent.y);
    extent.h = nRowH;
    return extent;
}

float IosDialogReplies::GetScrollableHeight()
{
    return static_cast<float>(GetReplyExtent(0, m_aReplies.m_count).y);
}

bool IosDialogReplies::EnoughScrollableHeightToScroll()
{
    const CSWGuiExtent frame = GetContentFrame(1);
    return !(GetScrollableHeight() - static_cast<float>(frame.h) < 20.0f);
}
#else
CSWGuiExtent IosDialogReplies::GetContentFrame(int bAbsolute)
{
    const unsigned char bNoBg = m_bNoBackground;
    int nTop = s_nReplyTopBg;
    int nBottom = s_nReplyBottomBg;
    int nSide = s_nReplySideBg;
    if (bNoBg)
    {
        nTop = s_nReplyTopNoBg;
        nBottom = s_nReplyBottomNoBg;
        nSide = s_nReplySideNoBg;
    }
    float fOffX = 0.0f;
    if (bAbsolute)
        fOffX = static_cast<float>(m_extent.x);
    const float fScale = GetHeightScale();
    CSWGuiExtent frame;
    frame.x = static_cast<int>(fOffX + fScale * nSide);
    const float fTopScaled = fScale * nTop;
    float fOffY = 0.0f;
    if (bAbsolute)
        fOffY = static_cast<float>(m_extent.y);
    frame.y = static_cast<int>(fTopScaled + fOffY);
    frame.w = static_cast<int>(static_cast<float>(m_extent.w) - 2.0f * fScale * nSide);
    frame.h = static_cast<int>(static_cast<float>(m_extent.h) - (fTopScaled + fScale * nBottom));
    return frame;

}

// Returns GetContentFrame's x/y plus the column grid. The content width is split into m_nColumns
// columns, so a reply is one cell wide, its x is (nIndex % columns) cells in, and its y is nIndex /
// columns rows of the m_replyLayout.h + m_nReplyHeight pitch down. The returned height is
// m_nReplyHeight.
CSWGuiExtent IosDialogReplies::GetReplyExtent(int bAbsolute, int nIndex)
{
    const unsigned char bNoBg = m_bNoBackground;
    int nTop = s_nReplyTopBg;
    static_cast<void>(s_nReplyBottomBg);
    int nSide = s_nReplySideBg;
    if (bNoBg)
    {
        nTop = s_nReplyTopNoBg;
        static_cast<void>(s_nReplyBottomNoBg);
        nSide = s_nReplySideNoBg;
    }

    float fOffX = 0.0f;
    if (bAbsolute)
        fOffX = static_cast<float>(m_extent.x);
    const float fScale = GetHeightScale();
    const int nX = static_cast<int>(fOffX + fScale * nSide);
    float fOffY = 0.0f;
    if (bAbsolute)
        fOffY = static_cast<float>(m_extent.y);
    const float fTopScaled = fScale * nTop;
    int nY = static_cast<int>(fTopScaled + fOffY);
    const int nContentW = static_cast<int>(static_cast<float>(m_extent.w) - 2.0f * fScale * nSide);

    // Only y carries the vertical scroll offset.
    int nScroll = 0;
    if (bAbsolute)
        nScroll = m_nVertScroll;
    nY += nScroll;

    const int nColumns = m_nColumns;
    CSWGuiExtent extent;
    extent.w = nContentW / nColumns;
    extent.y = (m_replyLayout.h + m_nReplyHeight) * (nIndex / nColumns) + nY;
    extent.x = (nIndex % nColumns) * extent.w + nX;
    extent.h = m_nReplyHeight;
    return extent;
}

// Returns the reply list's total scrollable height, the row pitch times the row count, minus half a
// row (the list is centred), plus the top inset scaled to the screen.
float IosDialogReplies::GetScrollableHeight()
{
    const int nColumns = m_nColumns;
    const int nReplyTotal = m_aReplies.m_count + nColumns - 1;

    int nTop = s_nReplyTopBg;
    static_cast<void>(s_nReplyBottomBg);
    static_cast<void>(s_nReplySideBg);
    if (m_bNoBackground)
    {
        nTop = s_nReplyTopNoBg;
        static_cast<void>(s_nReplyBottomNoBg);
        static_cast<void>(s_nReplySideNoBg);
    }

    const int nRows = nReplyTotal / nColumns;
    const int nPitch = m_replyLayout.h + m_nReplyHeight;
    const int nHeight = nPitch * nRows - m_nReplyHeight / 2;
    return static_cast<float>(nHeight + static_cast<int>(GetHeightScale() * nTop + 0.0f));
}

// True when the reply list's total scrollable height is at least 20 pixels taller than the
// content frame.
bool IosDialogReplies::EnoughScrollableHeightToScroll()
{
    const CSWGuiExtent frame = GetContentFrame(0);
    return !(GetScrollableHeight() - static_cast<float>(frame.h) < 20.0f);
}
#endif

#if !KOTOR1
// Per-key navigation deltas, indexed by nType minus the first mapped key code. Only four entries
// of each table are nonzero, at adjacent pairs of key codes: the row table moves one reply row,
// the line table one reply. An unmapped key zeroes both deltas.
static const int s_nReplyRowDelta[18]  = { 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0 };
static const int s_nReplyLineDelta[18] = { -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 1 };
#endif

extern int s_iosJoystickReplySelection;

// Navigates the reply list. On a key-down it moves the highlighted reply or scrolls the list,
// publishes the selection to the port-wide gamepad global, then keeps the selection inside the
// content frame.
// With the two-column layout a line key wraps at both ends and a row key keeps the column; with
// one column a row key clamps. See the archive for the exact wrap and clamp rules.
#if KOTOR1
// K1: one column, so only the up/down keys move the highlight, clamped at both ends, and nothing
// is published to a gamepad global.
void IosDialogReplies::HandleInputEvent(int nType, int nValue)
{
    if (nValue == 0)
        return;

    int nDelta = 1;
    switch (nType)
    {
    case INPUTEVENT_GUI_DPAD_UP:
    case INPUTEVENT_GUI_KEY_UP:
        nDelta = -1;
        break;
    case INPUTEVENT_GUI_DPAD_DOWN:
    case INPUTEVENT_GUI_KEY_DOWN:
        break;
    default:
        return;
    }
    if (g_pGuiMan->m_nCaptureFlags == 1)
        return;

    const int nSel = m_nSelectedReply + nDelta;
    m_nSelectedReply = nSel;
    if (nSel < 0)
        m_nSelectedReply = 0;
    else if (nSel >= m_aReplies.m_count)
        m_nSelectedReply = m_aReplies.m_count - 1;

    const CSWGuiExtent frame = GetContentFrame(0);
    const CSWGuiExtent reply = GetReplyExtent(0, m_nSelectedReply);
    const int nReplyTop = reply.y - frame.y;
    const int nReplyBottom = nReplyTop + reply.h;
    const int nWindowTop = -m_nVertScroll;
    const int nWindowBottom = frame.h - m_nVertScroll;

    if (static_cast<float>(nReplyBottom) - static_cast<float>(nWindowBottom) > 0.0f)
        m_nVertScroll = static_cast<int>(static_cast<float>(m_nVertScroll) - (static_cast<float>(nReplyBottom) - static_cast<float>(nWindowBottom)));
    if (static_cast<float>(nWindowTop) - static_cast<float>(nReplyTop) > 0.0f)
        m_nVertScroll = static_cast<int>((static_cast<float>(nWindowTop) - static_cast<float>(nReplyTop)) + static_cast<float>(m_nVertScroll));

    ClampVertScroll();
}
#else
void IosDialogReplies::HandleInputEvent(int nType, int nValue)
{
    if (nValue == 0)
        return;

    int nLineDelta = 0;
    int nRowDelta = 0;
    if (nType >= INPUTEVENT_GUI_DPAD_LEFT && nType <= INPUTEVENT_GUI_KEY_RIGHT)
    {
        nRowDelta = s_nReplyRowDelta[nType - INPUTEVENT_GUI_DPAD_LEFT];
        nLineDelta = s_nReplyLineDelta[nType - INPUTEVENT_GUI_DPAD_LEFT];
    }
    if (nRowDelta == 0 && nLineDelta == 0)
        return;

    const int nCount = m_aReplies.m_count;
    const int nLastIndex = nCount - 2;
    if (nLastIndex < 0)
        return;

    if (m_nColumns == 1)
    {
        if (nRowDelta != 0)
        {
            const int nSel = m_nSelectedReply + nRowDelta;
            m_nSelectedReply = nSel;
            if (nSel < 0)
                m_nSelectedReply = 0;
            else if (nSel >= nCount)
                m_nSelectedReply = nCount - 1;
        }
    }    else if (m_nColumns == 2)
    {
        if (nLineDelta != 0)
        {
            const int nSel = m_nSelectedReply + nLineDelta;
            m_nSelectedReply = nSel;
            if (nSel < 0)
                m_nSelectedReply = nCount - 1;
            else if (nSel >= nCount)
                m_nSelectedReply = 0;
        }
        else if (nRowDelta != 0)
        {
            const int nRows = (nCount + m_nColumns - 1) / m_nColumns;
            const int nOld = m_nSelectedReply;
            const int nRow = nOld / m_nColumns + nRowDelta;
            m_nSelectedReply = nOld + m_nColumns * nRowDelta;
            if ((nOld & 1) != 0)
            {
                if (nRow < 0)
                {
                    if ((nCount & 1) != 0)
                        m_nSelectedReply = nCount - 1;
                    else
                        m_nSelectedReply = nLastIndex;
                }
                else if (((nCount & 1) != 0 && nRow >= nRows - 1) ||
                         ((nCount & 1) == 0 && nRow >= nRows))
                    m_nSelectedReply = 0;
            }
            else
            {
                if (nRow >= nRows)
                    m_nSelectedReply = 1;
                else if (nRow < 0)
                {
                    if ((nCount & 1) != 0)
                        m_nSelectedReply = nLastIndex;
                    else
                        m_nSelectedReply = nCount - 1;
                }
            }
        }
    }


    s_iosJoystickReplySelection = m_nSelectedReply;

    const CSWGuiExtent frame = GetContentFrame(0);
    const CSWGuiExtent reply = GetReplyExtent(0, m_nSelectedReply);
    const int nReplyTop = reply.y - frame.y;
    const int nReplyBottom = nReplyTop + reply.h;
    const int nWindowTop = -m_nVertScroll;
    const int nWindowBottom = frame.h - m_nVertScroll;

    if (static_cast<float>(nReplyBottom) - static_cast<float>(nWindowBottom) > 0.0f)
        m_nVertScroll = static_cast<int>(static_cast<float>(m_nVertScroll) - (static_cast<float>(nReplyBottom) - static_cast<float>(nWindowBottom)));
    if (static_cast<float>(nWindowTop) - static_cast<float>(nReplyTop) > 0.0f)
        m_nVertScroll = static_cast<int>((static_cast<float>(nWindowTop) - static_cast<float>(nReplyTop)) + static_cast<float>(m_nVertScroll));

    ClampVertScroll();
}
#endif

// Paints the reply band: a fade ramp, the background image, the scrollbar thumb, the scissored
// reply cells and the iOS touch cursor.
#if KOTOR1
// K1: every metric is a literal in 768-line units scaled by the screen height, one reply column
// drawn by DrawTextWithBackground.
void IosDialogReplies::Draw(float fAlpha)
{
    const float fRamp = fAlpha * 4.0f + m_fCursorFadePhase;
    const float fScale = g_SDL_mode.h / 768.0f;
    const bool bFadeIn = !(static_cast<double>(fRamp) >= 12.564);
    m_fCursorFadePhase = bFadeIn ? fRamp : 12.564f;

    const CSWGuiExtent frame = GetContentFrame(1);

    if (m_pBackgroundImage != 0)
    {
        const Vector vTint(-1.0f, -1.0f, -1.0f);
        m_pBackgroundImage->Draw(m_extent.x, m_extent.y, m_extent.w, m_extent.h,
                            0, 0.0f, vTint, 1.0f);
    }

    const float fTrackH = static_cast<float>(static_cast<int>(static_cast<float>(m_extent.h) + fScale * -90.0f));
    const int nThumbY = m_replyLayout.y + static_cast<int>(fScale * 46.0f + static_cast<float>(m_extent.y));
    const int nThumbX = m_replyLayout.x + static_cast<int>(static_cast<float>(m_extent.x + m_extent.w) + fScale * -15.0f);
    const int nThumbW = m_replyLayout.w + static_cast<int>(fScale * 8.0f);

    float fThumb = static_cast<float>(frame.h) / GetScrollableHeight();
    if (fThumb > 1.0f)
        fThumb = 1.0f;
    const bool bScrollable = EnoughScrollableHeightToScroll();
    if (!bScrollable)
        fThumb = 1.0f;

    {
        const Vector vTint(-1.0f, -1.0f, -1.0f);
        m_pScrollbarImage->Draw(
            nThumbX,
            static_cast<int>(static_cast<float>(nThumbY) - (1.0f - fThumb) * fTrackH *
                      (static_cast<float>(m_nVertScroll) / (GetScrollableHeight() - static_cast<float>(frame.h)))),
            nThumbW, static_cast<int>(fThumb * fTrackH), 0, 0.0f, vTint, 1.0f);
    }

    ASLgl::glEnable(GL_SCISSOR_TEST);
    ASLgl::glScissor(0, CAurGUIDraw::cm_nViewportStack[0].nHeight - (frame.y + frame.h),
                     CAurGUIDraw::cm_nViewportStack[0].nWidth, frame.h);

    for (int64_t i = 0; i < m_aReplies.m_count; i++)
    {
        const CSWGuiExtent reply = GetReplyExtent(1, static_cast<int>(i));
        CAurGUIImageInternal* pArrow = (i == m_nSelectedReply) ? m_pReplyDownImage : m_pReplyUpImage;
        DrawTextWithBackground(static_cast<CAurGUIImage*>(pArrow), m_aReplies[static_cast<int>(i)].CStr(), m_nFontSize, 20, 10,
                               reply.x, static_cast<int>(fScale * 10.0f + static_cast<float>(reply.y)), reply.w, reply.h,
                               2, true);
    }

#ifdef K2_QOL_CONTROLLER
    // The port resets the box to the whole screen, which can be wider than 2000 pixels (an iPhone's
    // 2868); whatever later enables the scissor test without setting a box clipped the rest.
    ASLgl::glScissor(0, 0, CAurGUIDraw::cm_nViewportStack[0].nWidth,
                     CAurGUIDraw::cm_nViewportStack[0].nHeight);
#else
    ASLgl::glScissor(0, 0, 2000, 2000);
#endif
    ASLgl::glDisable(GL_SCISSOR_TEST);

    static CAurGUIImageInternal* pCursor = NewCAurGUIImage(const_cast<char*>("ios_touch_cursor"));
    if (bFadeIn && bScrollable)
    {
        const Vector vTint(-1.0f, -1.0f, -1.0f);
        const int nHalfW = m_extent.w / 2;
        const int nHalfH = m_extent.h / 2;
        pCursor->Draw(m_extent.x + nHalfW - TOUCH_CURSOR_SIZE / 2, m_extent.y + nHalfH - TOUCH_CURSOR_SIZE / 2,
                      TOUCH_CURSOR_SIZE, TOUCH_CURSOR_SIZE, 0, 0.0f, vTint,
                      (1.0f - cosf(m_fCursorFadePhase)) * 0.5f * 0.75f);
        if (m_bCursorShown == 0)
            m_bCursorShown = 1;
    }
}
#else
void IosDialogReplies::Draw(float fAlpha)
{
    const float fRamp = fAlpha * 4.0f + m_fCursorFadePhase;
    const bool bFadeIn = !(static_cast<double>(fRamp) >= 12.564);
    m_fCursorFadePhase = bFadeIn ? fRamp : 12.564f;

    const CSWGuiExtent frame = GetContentFrame(1);

    if (m_pBackgroundImage != 0)
    {
        const Vector vTint(-1.0f, -1.0f, -1.0f);
        m_pBackgroundImage->Draw(m_extent.x, m_extent.y, m_extent.w, m_extent.h,
                            0, 0.0f, vTint, 1.0f);
    }

    const float fWidthScale = GetWidthScale();
    const float fHeightScale = GetHeightScale();

    const float fBarTop    = fHeightScale * static_cast<float>(s_nBarTop);
    const float fBarBottom = fHeightScale * static_cast<float>(s_nBarBottom);
    const int nBarTop   = static_cast<int>(fBarTop + static_cast<float>(m_extent.y));
    const int nBarH     = static_cast<int>(static_cast<float>(m_extent.h) - (fBarTop + fBarBottom));
    const int nBarWidth = static_cast<int>(fWidthScale * static_cast<float>(s_nBarWidth));
    const int nBarRight = static_cast<int>(static_cast<float>(m_extent.x + m_extent.w) - fWidthScale * static_cast<float>(s_nBarRight));

    const int nThumbX = static_cast<int>(fWidthScale * static_cast<float>(m_replyLayout.x) + static_cast<float>(nBarRight));
    const int nThumbY = static_cast<int>(fHeightScale * static_cast<float>(m_replyLayout.y) + static_cast<float>(nBarTop));
    const int nThumbW = static_cast<int>(fWidthScale * static_cast<float>(m_replyLayout.w) + static_cast<float>(nBarWidth));

    const float fScrollable = GetScrollableHeight();
    const float fRatio = static_cast<float>(frame.h) / fScrollable;
    const bool bScrollable = EnoughScrollableHeightToScroll();
    const bool bShowCursor = bScrollable && bFadeIn;
    const float fThumb =
        bScrollable ? (fRatio > 1.0f ? 1.0f : fRatio) : 1.0f;
    const float fTrackH = static_cast<float>(nBarH);

    {
        const Vector vTint(-1.0f, -1.0f, -1.0f);
        m_pScrollbarImage->Draw(
            nThumbX,
            static_cast<int>(static_cast<float>(nThumbY) - (1.0f - fThumb) * fTrackH *
                      (static_cast<float>(m_nVertScroll) /
                       (static_cast<float>(GetScrollableHeight()) - static_cast<float>(frame.h)))),
            nThumbW, static_cast<int>(fThumb * fTrackH), 0, 0.0f, vTint, 1.0f);
    }

    ASLgl::glEnable(GL_SCISSOR_TEST);
    // Clips against the whole-screen viewport, not the current one.
    ASLgl::glScissor(0, CAurGUIDraw::cm_nViewportStack[0].nHeight - (frame.y + frame.h),
                     CAurGUIDraw::cm_nViewportStack[0].nWidth, frame.h);

    if (m_aReplies.m_count >= 1)
    {
        const int nCellW = frame.w / m_nColumns;

        for (int64_t i = 0; i < m_aReplies.m_count; i++)
        {
            const int nIdx = static_cast<int>(i);
            const int nX = nIdx % m_nColumns * nCellW + frame.x;
            const int nY = (m_replyLayout.h + m_nReplyHeight) * (nIdx / m_nColumns)
                           + (m_nVertScroll + frame.y);
            CAurGUIImageInternal* pArrow =
                (i == m_nSelectedReply) ? m_pReplyDownImage : m_pReplyUpImage;
            const char* pszText = m_aReplies[static_cast<int>(i)].CStr();

            const int nPadX = static_cast<int>(GetWidthScale() * 18.0f);
            const int nPadY = static_cast<int>(GetHeightScale() * 15.0f);
            int nViewportH =
                CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nHeight;
            int nViewportW =
                CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nWidth;

            if (pArrow != 0)
            {
                const Vector vTint(-1.0f, -1.0f, -1.0f);
                pArrow->Draw(nX, nY, nCellW, m_nReplyHeight, 0, 0.0f, vTint, 1.0f);
            }

            ::DrawTextWrapped(pszText, false, m_nFontSize, nViewportW, nViewportH,
                              nX + nPadX, nY + nPadY, nCellW - 2 * nPadX,
                              m_nReplyHeight - 2 * nPadY, 3, true);
        }
    }

#ifdef K2_QOL_CONTROLLER
    // The port resets the box to the whole screen, which can be wider than 2000 pixels (an iPhone's
    // 2868); whatever later enables the scissor test without setting a box clipped the rest.
    ASLgl::glScissor(0, 0, CAurGUIDraw::cm_nViewportStack[0].nWidth,
                     CAurGUIDraw::cm_nViewportStack[0].nHeight);
#else
    ASLgl::glScissor(0, 0, 2000, 2000);
#endif
    ASLgl::glDisable(GL_SCISSOR_TEST);

    static CAurGUIImageInternal* pCursor = NewCAurGUIImage(const_cast<char*>("ios_touch_cursor"));
    if (bShowCursor)
    {
        const Vector vTint(-1.0f, -1.0f, -1.0f);
        const int nHalfW = m_extent.w / 2;
        const int nHalfH = m_extent.h / 2;
        pCursor->Draw(m_extent.x + nHalfW - TOUCH_CURSOR_SIZE / 2, m_extent.y + nHalfH - TOUCH_CURSOR_SIZE / 2,
                      TOUCH_CURSOR_SIZE, TOUCH_CURSOR_SIZE, 0, 0.0f, vTint,
                      (1.0f - cosf(m_fCursorFadePhase)) * 0.5f * 0.75f);
        if (m_bCursorShown == 0)
            m_bCursorShown = 1;
    }
}
#endif

// Clamps the scroll offset into [-(scrollable - contentH), 0].
#if KOTOR1
void IosDialogReplies::ClampVertScroll()
{
    if (m_nVertScroll >= 1)
        m_nVertScroll = 0;

    const CSWGuiExtent frame = GetContentFrame(0);
    const int nRange = static_cast<int>(GetScrollableHeight()) - frame.h;
    const int nMin = -(nRange < 0 ? 0 : nRange);
    if (m_nVertScroll < nMin)
        m_nVertScroll = nMin;
}
#else
void IosDialogReplies::ClampVertScroll()
{
    int nScroll = m_nVertScroll;
    if (nScroll >= 1)
    {
        nScroll = 0;
        m_nVertScroll = 0;
    }

    const CSWGuiExtent frame = GetContentFrame(0);
    const int nRange = static_cast<int>(GetScrollableHeight()) - frame.h;
    const int nMin = -(nRange < 0 ? 0 : nRange);
    if (nScroll < nMin)
        m_nVertScroll = nMin;
}
#endif

// Turns a captured drag into a scroll; a touch that moved more than 10 pixels since mouse-down
// drops the highlighted reply.
int IosDialogReplies::HandleMouseCapturedMovement(int nDx, int nDy)
{
    K2_UNUSED(nDx); K2_UNUSED(nDy);
    int nUnusedX;
    int nMouseY = 0;
    static_cast<CSWGuiPanel*>(m_pContainer)->GetLocalMouseCoords(&nUnusedX, &nMouseY);

    const int nPrevY = m_nLastTouchY;
#if KOTOR1
    m_nLastTouchY = nMouseY;

    int nDelta = nMouseY - m_nTouchStartY;
    if (nDelta < 0)
        nDelta = -nDelta;
    if (nDelta >= 11)
        m_nSelectedReply = -1;

    const int nPrevScroll = m_nVertScroll;
    if (EnoughScrollableHeightToScroll())
        m_nVertScroll += nMouseY - nPrevY;
#else
    const int nPrevScroll = m_nVertScroll;
    m_nLastTouchY = nMouseY;

    int nDelta = nMouseY - m_nTouchStartY;
    if (nDelta < 0)
        nDelta = -nDelta;
    if (nDelta >= 11)
        m_nSelectedReply = -1;

    if (EnoughScrollableHeightToScroll())
        m_nVertScroll = nPrevScroll + (nMouseY - nPrevY);
#endif

    ClampVertScroll();

    if (m_nVertScroll != nPrevScroll)
        m_fCursorFadePhase = 1000.0f;

    return 1;
}

// Newline-filtered copy of sText used as the label's caption.
void IosLabel::SetString(CExoString sText) {
    char szFiltered[FILTERED_TEXT_SIZE];
    FilterString(m_bFilterNewline, sText, szFiltered, FILTERED_TEXT_SIZE);
    m_sText = szFiltered;
}

void IosLabel::SetImage(char* pImage) {
    delete m_pImage;
    m_pImage = 0;
    if (pImage != 0)
        m_pImage = NewCAurGUIImage(pImage);
}

#if !KOTOR1
void IosLabel::SetFontSize(int nSize, float fA, float fB) {
    m_nFontSize = nSize;
    m_fScaleX = fA;
    m_fScaleY = fB;
}
#endif

void IosLabel::SetFilterNewline(int bFilter) {
    m_bFilterNewline = bFilter;
}


// Rebuilds the background, scrollbar and arrow images from the given resrefs.
void IosDialogReplies::SetImages(char* pBackground, char* pScrollbar, char* pUp, char* pDown) {
#if !KOTOR1  // K1 derives no iPad / no-background flags (its metrics have one set)
    if (pBackground != 0 && strstr(pBackground, "ipho_respbgipad") != 0)
        m_bIpadLayout = 1;
    else
        m_bIpadLayout = 0;
    m_bNoBackground = (pBackground == 0);
#endif

    delete m_pBackgroundImage;
    m_pBackgroundImage = 0;
    if (pBackground != 0)
        m_pBackgroundImage = NewCAurGUIImage(pBackground);

    delete m_pScrollbarImage;
    m_pScrollbarImage = 0;
    if (pScrollbar != 0)
        m_pScrollbarImage = NewCAurGUIImage(pScrollbar);

    delete m_pReplyUpImage;
    m_pReplyUpImage = 0;
    if (pUp != 0)
        m_pReplyUpImage = NewCAurGUIImage(pUp);

    delete m_pReplyDownImage;
    m_pReplyDownImage = 0;
    if (pDown != 0)
        m_pReplyDownImage = NewCAurGUIImage(pDown);
}

int IosDialogReplies::GetConfirmedReplyAndReset() {
    int nConfirmedReply = m_nConfirmedReply;
    if (nConfirmedReply < 0)
        return -1;
    m_nConfirmedReply = -1;
    m_nSelectedReply = -1;
    return nConfirmedReply;
}

// Releases every image by calling the setter with nulls; the null-argument folding is what also
// flattens the two background flags.
IosDialogReplies::~IosDialogReplies()
{
    SetImages(0, 0, 0, 0);
}

// Rebuilds m_aReplies from aReplies, filtering each caption, and resets the mode word, the
// highlighted reply and the scroll offset.
#if KOTOR1

// K1: one column always, and a Chromebook also gets the keyboard highlight.
void IosDialogReplies::SetReplies(CExoArrayList<CExoString> aReplies)
{
    m_fCursorFadePhase = 0.0f;
    m_nConfirmedReply = -1;
    m_nSelectedReply = -1;
    if (ASLPlat_IsControllerPlugged() || SDL_IsChromebook())
        m_nSelectedReply = 0;

    m_aReplies.m_count = 0;

    for (int i = 0; i < aReplies.m_count; i++)
    {
        char szFiltered[FILTERED_TEXT_SIZE];
        FilterString(m_bFilterNewline, aReplies.m_pArray[i], szFiltered, FILTERED_TEXT_SIZE);
        m_aReplies.Add(CExoString(szFiltered));
        GetReplyExtent(1, i);  // result unused
    }

    m_nVertScroll = 0;
}
#else
void IosDialogReplies::SetReplies(CExoArrayList<CExoString> aReplies, float fScale, int bReset)
{
    if (bReset)
        m_nColumns = bReset;
    else if (fScale < 1.4)
        m_nColumns = 2;
    else
        m_nColumns = 1;

    m_fCursorFadePhase = 0.0f;
    m_nConfirmedReply = -1;
    m_nSelectedReply = -1;
    if (ASLPlat_IsControllerPlugged())
        m_nSelectedReply = 0;

    m_aReplies.m_count = 0;

    for (int i = 0; i < aReplies.m_count; i++)
    {
        char szFiltered[FILTERED_TEXT_SIZE];
        FilterString(m_bFilterNewline, aReplies.m_pArray[i], szFiltered, FILTERED_TEXT_SIZE);
        m_aReplies.Add(CExoString(szFiltered));
    }

    m_nVertScroll = 0;
}
#endif

// Seeds the row pitch and column count, clears the reply state and loads the four stock images.
IosDialogReplies::IosDialogReplies()
{
    m_replyLayout.h = 11;
#if !KOTOR1
    m_nReplyHeight = 72;
    m_nColumns = 2;
#endif
    m_nConfirmedReply = -1;
    m_nSelectedReply = -1;
    m_nTouchStartX = 0;
    m_nTouchStartY = 0;
    m_nVertScroll = 0;
    m_fCursorFadePhase = 0;
    m_bCursorShown = 0;
    m_pBackgroundImage = 0;
    m_pScrollbarImage = 0;
    m_pReplyUpImage = 0;
    m_pReplyDownImage = 0;
    SetImages(const_cast<char*>("ipho_responsebg"), const_cast<char*>("ipho_scrollbar"), const_cast<char*>("ipho_resup"), const_cast<char*>("ipho_resdown"));
}

// Hit-tests the touch against every reply cell, records the last hit as the highlighted reply,
// then captures the mouse so the drag that follows routes to this widget.
void IosDialogReplies::HandleLMouseDown()
{
    int nMouseX, nMouseY;
    static_cast<CSWGuiPanel*>(m_pContainer)->GetLocalMouseCoords(&nMouseX, &nMouseY);

    m_nTouchStartX = nMouseX;
    m_nTouchStartY = nMouseY;
    m_nLastTouchY = nMouseY;

    for (int i = 0; i < m_aReplies.m_count; i++)
    {
        const CSWGuiExtent reply = GetReplyExtent(1, i);
#if KOTOR1
        // K1 tests x first; K2 tests y first.
        if (nMouseX >= reply.x && nMouseY >= reply.y &&
            nMouseX <= reply.x + reply.w && nMouseY <= reply.y + reply.h)
            m_nSelectedReply = i;
#else
        if (nMouseY >= reply.y && nMouseX >= reply.x &&
            nMouseY <= reply.y + reply.h && nMouseX <= reply.x + reply.w)
            m_nSelectedReply = i;
#endif
    }

    g_pGuiMan->CaptureMouse(this, 1);
}

// Promotes the highlighted reply into the confirmed one and records its index in the control's
// user-data payload before notifying the base control.
void IosDialogReplies::HandleLMouseUp()
#if KOTOR1
{
    m_nConfirmedReply = m_nSelectedReply;
    if (m_nConfirmedReply >= 0)
        PlayClickSound();
    g_pGuiMan->ReleaseMouse(this);
}
#else
{
    m_nConfirmedReply = m_nSelectedReply;
    if (m_nConfirmedReply >= 0)
    {
        m_tail.m_nUserData = m_nSelectedReply;
        CSWGuiControl::HandleInputEvent(INPUTEVENT_GUI_A, 1);
        if (m_nSelectedReply >= 0)
            PlayClickSound();
    }
    g_pGuiMan->ReleaseMouse(this);
}
#endif

// Prefixes the caption with the glyph for the given button letter.
void IosAddStringIcon(CExoString& sText, char cIcon)
{
#ifdef K2_UI_PC
    // pc: the fonts have no pad glyphs, so the prefix is a blank that pushes the caption off centre
    K2_UNUSED(sText);
    K2_UNUSED(cIcon);
#else
    CExoString sIcon;

    switch (cIcon)
    {
    case 'a':
        sIcon = "\x10";
        break;
    case 'b':
        sIcon = "\x11";
        break;
    case 'x':
        sIcon = "\x12";
        break;
    case 'y':
        sIcon = "\x13";
        break;
#ifdef K2_QOL_CONTROLLER
    case 'l':   // the shoulder buttons (platforms/tools/padglyphs.py)
        sIcon = "\x15";
        break;
    case 'r':
        sIcon = "\x16";
        break;
#endif
    }

    sText = sIcon + sText;
#endif
}

// Prefixes the label's caption with the glyph for the given button letter.
void IosAddLabelIcon(CSWGuiLabel* pLabel, char nIcon)
{
    CExoString sText(pLabel->m_text.m_params.m_sText);
    IosAddStringIcon(sText, nIcon);
    pLabel->m_text.m_params.SetText(sText);
}

// The button-side twin of IosAddLabelIcon.
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon)
{
    CExoString sText(pButton->m_text.m_params.m_sText);
    IosAddStringIcon(sText, nIcon);
    pButton->m_text.m_params.SetText(sText);
}

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
