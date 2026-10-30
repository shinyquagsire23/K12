#pragma once

#ifndef CSTRINGITEM_H
#define CSTRINGITEM_H

class CAurFont;
#if KOTOR1
class Gob;
#endif

// One laid-out run of GUI text, the string itself plus the layout results that
// CAurFont::CalculateVisibleStringLengthAndWidth writes back into it.
class CStringItem {
public:
#if KOTOR1
    // K1's constructor is defined here; K2 has none.
    CStringItem()
        : m_pszText(0), m_nVisibleStart(0), m_nVisibleLength(0), m_nState(0), m_nColor(0),
          m_fVisibleWidth(0.0f), m_fBottom(0.0f), m_fTop(0.0f), m_pAttached(0) {}
#endif

    virtual ~CStringItem();

    virtual void Render(CAurFont*);

    // Each of these reallocates m_pszText and frees the old buffer.
    void SetString(char* pszText, int nLength);
    void AppendString(char* pszText, int nLength);
    void PrependString(char* pszText, int nLength);
    void InsertString(char* pszText, int nCount, int nPosition);
    void DeleteChars(int nStart, int nCount);

    char* m_pszText;
    int m_nVisibleStart;
    int m_nVisibleLength;
    short m_nState;
    short m_nColor;
    float m_fVisibleWidth;
    float m_fBottom;
    float m_fTop;
#if KOTOR1
    Gob* m_pAttached;
#endif
};

#endif // CSTRINGITEM_H

#ifndef CAURSTRING_H
#define CAURSTRING_H

#include "Vector.h"
#include "aurfont.h"

class CAurObject;
class CAurPart;
class CHitInfo;
// Only ever handled by pointer here; defined by the concrete implementation, StringGob.
struct SStringGobDisplayInfo;

// The renderable-text interface, a pure interface with no data beyond the vptr and no
// implementation of its own. The declaration order below is the vtable order and must not change;
// StringGob overrides every slot.
class CAurString {
public:
    // Must stay empty and defined here: StringGob's destructor calls only Gob's.
    virtual ~CAurString() {}
    virtual void* AttachToObject(CAurObject* a, char* b, int c) = 0;
    virtual void* AttachToPart(CAurObject* a, CAurPart* b, int c) = 0;
    virtual int  AttachCornersToObjects(CAurObject* a, char* b, char* c) = 0;
    virtual int  SetPositionOffset(Vector v) = 0;
    virtual Vector SetPosition(Vector v) = 0;
    virtual void AttachModel(CAurObject* pObject, int a) = 0;
    virtual CAurObject* GetAttachedModel(int nIndex) = 0;
    virtual void* GetExternalData() = 0;
    virtual void SetExternalData(void* pData) = 0;
    virtual int  SetFont(char* pszFont) = 0;
    virtual int  SetFontPassword(char* pszFont, char chMask) = 0;
    virtual int  GetCurrentFontIndex() = 0;
    virtual void SetAlignment(float f) = 0;
    virtual void SetAlignmentVertical(float f) = 0;
    virtual void SetColorDefault(float r, float g, float b, float a) = 0;
    virtual void SetIndexedColor(int nIndex, float r, float g, float b, float a) = 0;
    virtual int  ScrollToLine(int nLine) = 0;
    virtual int  ScrollToPixel(int nPixel) = 0;
    virtual float GetLinePixelWidth(int nLine) = 0;
    virtual float GetLinePixelHeight(int nLine) = 0;
    virtual void SetLinePixelHeight(int nLine, float fHeight) = 0;
    virtual void RefreshDisplay() = 0;
    virtual SStringGobDisplayInfo* GetDisplayInfo() = 0;
    virtual void SetMaxWidth(float f) = 0;
    virtual void SetMaxHeight(float f) = 0;
    virtual int  SetText(char* pszText) = 0;
    virtual int  AddText(char* pszText) = 0;
    virtual char* GetTextLine(int nLine) = 0;
    virtual int  GetTextLineCount() = 0;
    virtual int  GetLineState(int nLine) = 0;
    virtual void SetLineState(int nLine, int nState) = 0;
    virtual void SetTextWrapping(bool bWrap) = 0;
    virtual void SetLineColor(int nLine, int nColor) = 0;
    virtual int  GetLineColor(int nLine) = 0;
    virtual int  GetNumColors() = 0;
    virtual void AddLine(char* pszText) = 0;
    virtual void InsertLine(char* pszText, int nLine) = 0;
    virtual void DeleteLine(int nLine) = 0;
    virtual void SetLine(int nLine, char* pszText) = 0;
    virtual int  InsertTextAtCaret(char* pszText) = 0;
    virtual int  DeleteTextAtCaret(int nCount) = 0;
    virtual int  GetCaretLinePos() = 0;
#if KOTOR1
    virtual bool SetCaretLinePos(int nLine) = 0;                                    // K1: returns bool
#else
    virtual int  SetCaretLinePos(int nLine) = 0;
#endif
    virtual int  GetCaretColumnPos() = 0;
    virtual int  SetCaretColumnPos(int nColumn) = 0;
    virtual int  MoveCaretTo(Vector v) = 0;
    virtual void SetCaretVisible(bool bVisible) = 0;
    virtual int  SuggestDeleteCurrentFont() = 0;
    virtual CStringItem** GetStringsArray() = 0;
    virtual void SetColorSelected(float r, float g, float b, float a) = 0;

    // A plain member, not one of the interface's slots.
    void DieWithoutParent();
};

#endif // CAURSTRING_H

#ifndef STRINGGOB_H
#define STRINGGOB_H

#include "AURORA/aurmodel.h"

class CAurObject;
class CAurPart;
class CHitInfo;

// A scene-graph Gob whose renderable content is a list of laid-out text lines. Most of the class is
// stubbed out here: all but a handful of its methods are empty or return a constant.
//
// Modelled with real multiple inheritance: a Gob base, then a CAurString base. All of CAurString's
// slots are overridden here, which is why that side of the vtable is a block of thunks.

struct SStringGobFont {
    int nIndex;
};

// The caret is a member object rather than three loose fields; its constructor takes the glyph.
#if KOTOR1
// K1: the scroll window.
struct SStringGobDisplayInfo {
    int   nFirstLine;
    int   nNumLines;
    int   nNumAbove;
    float m_fMaxWidth;
    float m_fMaxHeight;
    float m_fTotalHeight;
    float m_fVisibleHeight;
    float m_fScrollOffset;
    bool  m_bScrollMode;
    float m_fHitMargin;
    float m_fLineHeight;
};

// K1: one entry of the colour table; a new entry is opaque white.
struct SStringGobColor {
    float r, g, b, a;
    SStringGobColor() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
};
#else
struct SStringGobDisplayInfo {
    char _pad00[0x2fc - 0x2f0];
};
#endif

// An owning pointer list. The destructor deletes the array, and nCount is what the counts accessors
// return. nCapacity is never read.
template <class T>
struct SStringGobList {
    T*  pa;
    int nCount;
    int nCapacity;
    ~SStringGobList() { delete[] pa; }
};

class StringGob : public Gob, public CAurString {
public:
    // The int argument is unused, because the constructor overwrites it with the global name buffer
    // before the base call.
    StringGob(int a);
    virtual ~StringGob();

    // Gob overrides, in slot order.
    virtual void Render(bool a);
    virtual void* AttachToObject(CAurObject* a, char* b, int c);
    virtual void* AttachToPart(CAurObject* a, CAurPart* b, int c);
    virtual Vector SetPosition(Vector v);
    virtual void* GetExternalData();
    virtual void SetExternalData(void* pData);
    virtual void* HitCheck(Vector a, Vector b, Vector* c, CHitInfo* d);
    virtual void GetMinimumBoundingBox(Vector& vMin, Vector& vMax);
    virtual void GetMinimumBoundingSphere(Vector& vCenter, float& fRadius);
    virtual CAurObject* AsCAurObject();
    virtual StringGob* AsString();

    // The class's own virtuals, in vtable order.
    virtual int  AttachCornersToObjects(CAurObject* a, char* b, char* c);
    virtual int  SetPositionOffset(Vector v);
    virtual void AttachModel(CAurObject* pObject, int a);
    virtual CAurObject* GetAttachedModel(int nIndex);
    virtual int  SetFont(char* pszFont);
    virtual int  SetFontPassword(char* pszFont, char chMask);
    virtual int  SuggestDeleteCurrentFont();
    virtual int  GetCurrentFontIndex();
    virtual void SetAlignment(float f);
    virtual void SetAlignmentVertical(float f);
    virtual float GetAlignment();
    virtual float GetAlignmentVertical();
    virtual char* GetFontName();
    virtual void GetColorDefault(float& r, float& g, float& b, float& a);
    virtual void RefreshDisplay();
    virtual void SetMaxWidth(float f);
    virtual void SetMaxHeight(float f);
    virtual int  SetText(char* pszText);
    virtual int  AddText(char* pszText);
    virtual char* GetTextLine(int nLine);
    virtual int  GetTextLineCount();
    virtual void SetColorDefault(float r, float g, float b, float a);
    virtual void SetIndexedColor(int nIndex, float r, float g, float b, float a);
    virtual int  GetLineState(int nLine);
    virtual void SetLineState(int nLine, int nState);
    virtual void SetLineColor(int nLine, int nColor);
    virtual int  GetLineColor(int nLine);
    virtual int  GetNumColors();
    virtual float GetLinePixelWidth(int nLine);
    virtual float GetLinePixelHeight(int nLine);
    virtual void SetLinePixelHeight(int nLine, float fHeight);
    virtual SStringGobDisplayInfo* GetDisplayInfo();
    virtual void SetTextWrapping(bool bWrap);
    virtual void AddLine(char* pszText);
    virtual void InsertLine(char* pszText, int nLine);
    virtual void DeleteLine(int nLine);
    virtual void SetLine(int nLine, char* pszText);
    virtual int  InsertTextAtCaret(char* pszText);
    virtual int  DeleteTextAtCaret(int nCount);
    virtual int  GetCaretLinePos();
#if KOTOR1
    virtual bool SetCaretLinePos(int nLine);
#else
    virtual int  SetCaretLinePos(int nLine);
#endif
    virtual int  GetCaretColumnPos();
    virtual int  SetCaretColumnPos(int nColumn);
    virtual int  ScrollToLine(int nLine);
    virtual int  ScrollToPixel(int nPixel);
    virtual CStringItem** GetStringsArray();
    virtual int  MoveCaretTo(Vector v);
    virtual void SetCaretVisible(bool bVisible);
    virtual void SetColorSelected(float r, float g, float b, float a);

    // Non-virtual helpers.
    void DeleteText();
    void RecalcDisplayData();
    int  WrapLines(int nWidth);
    int  UnWrapLines(int nWidth);
    void ShowAsManyLinesAsYouCan_Temp();

    // The Gob base comes first, then the CAurString base, so the class's own fields follow both.
#if KOTOR1
    // K1: the Gob base is smaller, so every field sits lower.
    bool  m_bTextWrapping;
    CAurFont* m_pFont;
    CCaret m_caret;
    float m_fAlignment;
    float m_fAlignmentVertical;
    List<CStringItem*> m_lines;
    int   m_nMaxLines;
    bool  m_bCaretVisible;
    List<SStringGobColor> m_colors;
    SStringGobDisplayInfo m_displayInfo;
#else
    bool  m_bTextWrapping;
    SStringGobFont* m_pFont;
    CCaret m_caret;
    float m_fAlignment;
    float m_fAlignmentVertical;
    SStringGobList<CStringItem*> m_lines;
    char  _pad2d8[0x2dc - 0x2d8];
    bool  m_bCaretVisible;
    SStringGobList<void*> m_colors;
    SStringGobDisplayInfo m_displayInfo;
    float m_fMaxWidth;
    float m_fMaxHeight;
    char  _pad304[0x31c - 0x304];
#endif
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(StringGob) == 0x2e8, "K1 StringGob size (NewCAurString @0x55f27c)");
#endif

#endif // STRINGGOB_H
