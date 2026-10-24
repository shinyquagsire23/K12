#pragma once

// Aspyr's touch-oriented replacement for the PC conversation UI: the reply list the player taps is
// an IosDialogReplies, the speaker line above it a plain IosLabel, and both derive from
// IosControl.

#include "swguicontrol.h"
#include "CExoArrayListDtors.h"
#include "exostring.h"
#include "aurgui.h"

class CSWGuiLabel;
class CSWGuiButton;

class IosControl : public CSWGuiControl {
public:
    // Wraps the text inside the given rect.
    void DrawTextWrapped(char const* pszText, int n1, int n2, int n3, int n4, int n5, int n6, bool bCenter);
    // Draws pImage over the given (n4, n5, n6, n7) rect and the text inset by (n2, n3) on each
    // side of it.
    void DrawTextWithBackground(CAurGUIImage* pImage, char const* pszText,
                                int n1, int n2, int n3, int n4, int n5,
                                int n6, int n7, int n8, bool bCenter);

private:
    // No user-declared constructor or destructor: IosLabel constructs CSWGuiControl directly and
    // tears it down itself.
};

class IosLabel : public IosControl {
public:
    IosLabel();
    ~IosLabel();
#if KOTOR1
    void SetFontSize(int nSize);                   // K1: the size only, scaled to the screen
#else
    void SetFontSize(int nSize, float fA, float fB);
#endif
    void SetString(CExoString sText);
    void SetFilterNewline(int bFilter);
    void SetImage(char* pImage);
    void PlayClickSound();
    // Paints the background image over the whole extent, then the wrapped text inset by a pad
    // derived from the two SetFontSize scales.
    virtual void Draw(float fAlpha);
#if KOTOR1
    virtual void SetExtent(const CSWGuiExtent& extent);  // K1: forwards to CSWGuiObject
#endif

    int  m_bCenterText;
    int  m_nMaxLines;
    int  m_nFontSize;
    int  m_bFilterNewline;
#if !KOTOR1  // K1 offsets differ: image at +0xa0, text after it
    float m_fScaleX;
    float m_fScaleY;
#endif
    CAurGUIImageInternal* m_pImage;
    CExoString m_sText;
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(IosLabel) == 0xb8, "K1 IosLabel size drift");
#else  // K2: the GUI base classes are larger than K1's
K2_LAYOUT_ASSERT(sizeof(IosLabel) == 0xd0, "IosLabel size drift");
#endif

class IosDialogReplies : public IosLabel {
public:
    IosDialogReplies();
    ~IosDialogReplies();
#if KOTOR1
    void SetReplies(CExoArrayList<CExoString> aReplies);  // K1: no scale or column mode
#else
    void SetReplies(CExoArrayList<CExoString> aReplies, float fScale, int bReset);
#endif
    void SetImages(char* pBackground, char* pScrollbar, char* pUp, char* pDown);
    // Returns the confirmed reply index, or -1 if none, and clears the stored index.
    int  GetConfirmedReplyAndReset();
    float GetWidthScale() const;
    float GetHeightScale() const;
    virtual void HandleInputEvent(int type, int val);
    virtual void HandleLMouseUp();
    virtual int  HandleMouseCapturedMovement(int nX, int nY);
    virtual void HandleLMouseDown();
    virtual void Draw(float fAlpha);
    CSWGuiExtent GetContentFrame(int bAbsolute);
    CSWGuiExtent GetReplyExtent(int bAbsolute, int nIndex);
    float GetScrollableHeight();
    // True when the reply list is taller than the scroll threshold.
    bool EnoughScrollableHeightToScroll();
    // Keeps the vertical scroll offset inside the scrollable range.
    void ClampVertScroll();

    CSWGuiExtent m_replyLayout;
#if !KOTOR1
    int  m_nReplyHeight;
#endif
#if !KOTOR1
    int  m_nColumns;
#endif
    CExoArrayList<CExoString> m_aReplies;
    int  m_nConfirmedReply;
    int  m_nSelectedReply;
    int  m_nTouchStartX;
    int  m_nTouchStartY;
    int  m_nLastTouchY;
    int  m_nVertScroll;
    float m_fCursorFadePhase;
    unsigned char m_bCursorShown;
    unsigned char m_bIpadLayout;
    unsigned char m_bNoBackground;
    char _pad117[0x118 - 0x117];
    CAurGUIImageInternal* m_pBackgroundImage;
    CAurGUIImageInternal* m_pScrollbarImage;
    CAurGUIImageInternal* m_pReplyUpImage;
    CAurGUIImageInternal* m_pReplyDownImage;
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(IosDialogReplies) == 0x118, "K1 IosDialogReplies size drift");
#else  // K2: the GUI base classes are larger than K1's
K2_LAYOUT_ASSERT(sizeof(IosDialogReplies) == 0x138, "IosDialogReplies size drift");
#endif

// Writes the newline-filtered, NUL-terminated text into pszBuffer.
void FilterString(bool bFilterNewline, CExoString& sText, char* pszBuffer, int nBufferSize);
void IosAddStringIcon(CExoString& sText, char cIcon);

// Copy the caption out, prefix it with cIcon and put the copy back.
void IosAddLabelIcon(CSWGuiLabel* pLabel, char nIcon);
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
