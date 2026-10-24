#pragma once
#include "k2stdint.h"

#include "exoresman.h"

class CSWGuiBorder;
struct Vector;

// Three-float colour triple. Its own type rather than a Vector because widgets copy a
// function-local static instance of it into their borders.
struct CSWGuiColor {  // idiom-exempt: canonical
    float r, g, b;
    CSWGuiColor() : r(0.0f), g(0.0f), b(0.0f) {}
    CSWGuiColor(float fR, float fG, float fB) : r(fR), g(fG), b(fB) {}
};

// Border-drawing parameters, with three tiled image keys, the frame colour and a back-pointer to
// the border they are installed on. SetPulsingAlpha packs three independent "pulse this edge's
// alpha" enable bits into one flag halfword.
class CSWGuiBorderParams {
public:
    // Explicit frame thickness in pixels; GetBorderDim falls back to the owning border's corner
    // image when it is zero.
    int  GetBorderDim() const;
    void SetPulsingAlpha(int a, int b, int c);
    void SetFillImage(const CResRef& resRef, int flag);
    void SetCornerImage(const CResRef& resRef, int flag);
    void SetEdgeImage(const CResRef& resRef, int flag);
    void SetBorder(CSWGuiBorder* pBorder);

    // Declared out of line so that every assignment of a params block is a single call.
    CSWGuiBorderParams& operator=(const CSWGuiBorderParams& other);

    // Declared out of line for the same reason: every by-value copy of a params block is one call.
    CSWGuiBorderParams(const CSWGuiBorderParams& other);

    // Builds every image key, the colour and the frame alpha in one go. The parameters are named
    // by position; the two unk ints land in the flag word.
#if KOTOR1
    // K1 has no nFlipStyle and no trailing stack int. The K2 spelling forwards, so the shared
    // call sites keep it and still emit K1's own call.
    CSWGuiBorderParams(const CResRef& resRefCorner, const CResRef& resRefEdge, const CResRef& resRefFill,
                       int nFillStyle, int nBorderDim, float fAlpha, const Vector& color, int nStack0);
    CSWGuiBorderParams(const CResRef& resRefCorner, const CResRef& resRefEdge, const CResRef& resRefFill,
                       int nFillStyle, int nBorderDim, int, float fAlpha,
                       const Vector& color, int nStack0, int)
        : CSWGuiBorderParams(resRefCorner, resRefEdge, resRefFill, nFillStyle, nBorderDim, fAlpha, color, nStack0)
    {
    }
#else
    CSWGuiBorderParams(const CResRef& resRefCorner, const CResRef& resRefEdge, const CResRef& resRefFill,
                       int nFillStyle, int nBorderDim, int nFlipStyle, float fAlpha,
                       const Vector& color, int nStack0, int nStack1);
#endif

    int  m_nBorderDim;
    int  m_nInnerOffset;
#if !KOTOR1  // K2 only: K1's border load reads no INNEROFFSETY
    int  m_nInnerOffsetY;
#endif
    float m_fRotate;
    float m_fAlpha;
    CSWGuiColor m_color;
#if KOTOR1
    // K1: a byte of flag bits, with the three image keys following it unaligned.
    struct {
        unsigned char nFillStyle : 2;
        unsigned char bPulseA : 1;
        unsigned char bPulseB : 1;
        unsigned char bPulseC : 1;
        unsigned char nReserved1 : 3;
    } m_flags;
#else
    struct {
        unsigned short nFillStyle : 2;
        unsigned short bPulseA : 1;
        unsigned short bPulseB : 1;
        unsigned short bPulseC : 1;
        unsigned short nFlipStyle : 4;
        unsigned short nReserved1 : 7;
    } m_flags;
#endif
    CResRef m_cornerImage;
    CResRef m_edgeImage;
    CResRef m_fillImage;
#if KOTOR1
    char _pad4d[0x50-0x4d];
#else
    char _pad52[0x58-0x52];
#endif
    CSWGuiBorder* m_pBorder;
};


#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWGuiBorderParams) == 0x58, "CSWGuiBorderParams K1 size (CSWGuiBorder 0x88)");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiBorderParams) == 0x60, "CSWGuiBorderParams size drift");
#endif

// Polymorphic base of every self-drawing GUI element that is not a CSWGuiControl: CSWGuiBorder,
// CSWGuiImage, CSWGuiText, CSWGuiIndicator and CSWGuiScene. Only the screen-rect extent is modelled;
// the ideal height accessor returns that extent's height.
//
// All five derived classes share the same six primary slots: destructor, SetExtent, GetIdealHeight,
// Draw and Load, with Draw and Load pure here since the base is never instantiated.
//
// SetExtent and GetIdealHeight are defined in CSWGuiDrawable.cpp.
#include "swgui.h"

class CExoString;
class CResGFF;
struct CResStruct;
struct Vector;

class CSWGuiDrawable {
public:
    // Declared virtual even though nothing here defines it: the declaration is what makes the
    // derived classes' destructors virtual.
    virtual ~CSWGuiDrawable() {}
    virtual void SetExtent(const CSWGuiExtent& extent);
    virtual int  GetIdealHeight();
    virtual void Draw(float fAlpha) = 0;
    virtual void Load(CResGFF* pGFF, CResStruct& rStruct,
                      const CExoString& sFieldName) = 0;

    // Ticks the attract alpha one frame. A static member function because it only reads the two
    // statics below.
    static void UpdatePulsing(float fDelta);

    static float m_fPulsingAlpha;

    static float m_fAlphaPulseTimer;
    static int   m_bAlphaPulsingUp;

    static const Vector NO_COLOURING;

    CSWGuiExtent m_extent;
};

// The tiled image frame drawn around a GUI element. Owns a CSWGuiBorderParams by value and three
// lazily created image objects (corner, edge, fill) that the params' setters install and destroy
// on its behalf.
class CAurGUIImageInternal;
class CExoString;
class CResGFF;
struct CResStruct;
struct Vector;

class CSWGuiBorder : public CSWGuiDrawable {
public:
    CSWGuiBorder();

    // Defining this destructor out of line is what emits the class vtable, which several
    // translation units reference.
    virtual ~CSWGuiBorder();
    virtual void Draw(float fAlpha);
    virtual void Load(CResGFF* pGFF, CResStruct& rStruct, const CExoString& sFieldName);

    void Initialize(const CSWGuiExtent& extent, const CSWGuiBorderParams& params);
    CSWGuiBorder& operator=(CSWGuiBorder& other);

    // The rectangle left inside the edging, which a label hands to its text.
    CSWGuiExtent GetInnerExtent();

    // The int parameters are (height, width, x, y), not (x, y, w, h). FillTile divides the first
    // two by the tile's size, and Draw calls it with the same four values.
    void FillTile(int nHeight, int nWidth, int nX, int nY, float fAlpha, const Vector& vColor);
    void FillCenter(int nHeight, int nWidth, int nX, int nY, float fAlpha, const Vector& vColor);

    CSWGuiBorderParams m_params;

    CAurGUIImageInternal* m_pCornerImage;
    CAurGUIImageInternal* m_pEdgeImage;
    CAurGUIImageInternal* m_pFillImage;
};


#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWGuiBorder) == 0x88, "CSWGuiBorder K1 size (new in the CSWGuiPanel ctor 0x6245fc)");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiBorder) == 0x90, "CSWGuiBorder size drift");
#endif

// The text-rendering parameter block a GUI widget carries by value, holding the string or StrRef,
// the font, the colours, the alignment, and a back-pointer to the CSWGuiText object that draws it.
#include "exostring.h"
#include "Vector.h"
#include "exoresman.h"

class CSWGuiText;
class CExoLocString;

class CSWGuiTextParams {
public:
    CSWGuiTextParams();
    CSWGuiTextParams(const CSWGuiTextParams& other);
    // The two "build it in one go" constructors. They are the same body up to which of the text
    // setters runs, one taking a literal string and the other a StrRef.
    CSWGuiTextParams(const CExoString& sText, const CResRef& resRefFont, const Vector& color,
                     int nAlignment, float fAlpha);
    CSWGuiTextParams(uint64_t nStrRef, const CResRef& resRefFont, const Vector& color,
                     int nAlignment, float fAlpha);
    CSWGuiTextParams& operator=(const CSWGuiTextParams& other);

    void SetText(const CExoString& sText);
    void SetStrRef(uint64_t nStrRef);
    // Resolves one language/gender variant out of a locstring and installs it as the literal text.
    // Takes the locstring by non-const reference to reach the non-const GetString overload.
    void SetLocString(CExoLocString& locString, int nLanguage);
    void SetAlignment(int nAlignment);

    // Binds this parameter block to the CSWGuiText that renders it, creating that widget's
    // CAurGUIStringInternal on first bind and then replaying the whole parameter set onto it.
    void SetTextObject(CSWGuiText* pTextObject);

    void SetColor(const Vector& color);
    // Sets the fallback colour and makes it current in one call.
    void SetDefaultColor(const Vector& color);
    // The two halves of "enabled/disabled text" restore the fallback colour or force the disabled
    // tint.
    void RestoreDefaultColor();
    void ReSetFont();
    // Re-resolves the font handle from the base-font name, keeping the current one when the name
    // resolves to the same variant.
    void SetFontRename();
    // Installs a font handle directly. Does nothing unless the widget is already bound and the
    // ResRef is non-empty.
    void SetFont(const CResRef& resRefFont);
    // Rewrites the base-font stem.
    void SetBaseFont(const CResRef& resRef);

    CExoString         m_sText;
    unsigned long long m_nStrRef;     // -1 == none
    CResRef            m_resRefFont;
    Vector             m_color;
    float              m_fAlpha;
    Vector             m_defaultColor;
    unsigned char      m_nAlignment : 6;
    unsigned char      bHilight     : 1;
    unsigned char      _bits44_7    : 1;
    char               m_szBaseFont[0x56 - 0x45];
    char               _pad56[0x58 - 0x56];
    CSWGuiText*        m_pTextObject;
};

#ifndef CSWGUITEXT_H
#define CSWGUITEXT_H

// The GUI text element, a CSWGuiTextParams parameter block plus the Aurora-side laid-out string
// object that actually renders it. Every method forwards the params or extent state onto that
// object.
class CAurGUIStringInternal;
class CExoString;
class CResGFF;
struct CResStruct;

class CSWGuiText : public CSWGuiDrawable {
public:
    // Defining this destructor out of line is what emits the class vtable.
    virtual ~CSWGuiText();
    virtual void SetExtent(const CSWGuiExtent& extent);
    virtual int GetIdealHeight();
    virtual void Draw(float fAlpha);
    virtual void Load(CResGFF* pGFF, CResStruct& rStruct, const CExoString& sFieldName);

    CSWGuiText();
    CSWGuiText& operator=(CSWGuiText& other);
    void Initialize(const CSWGuiExtent& extent, const CSWGuiTextParams& params, float f);

    // Re-flows the string into the current extent.
    void WrapString();

    // The rendered line height of the current base font.
    int GetFontHeight();

    // The ideal size of the laid-out string, with x and y always 0.
    CSWGuiExtent GetIdealWidthAndHeight();

    CAurGUIStringInternal* m_pString;
    CSWGuiTextParams m_params;
    int m_nWrapWidth;
    short m_nMaxLength;
};

#endif // CSWGUITEXT_H

// The editable text element, CSWGuiText plus the text actually being typed. Defined in
// CSWGuiText.cpp rather than a file of its own, so that the base constructor and its
// exception-handling destructor stay visible to the derived constructor.
#ifndef CSWGUIEDITTEXT_H
#define CSWGUIEDITTEXT_H


class CSWGuiEditText : public CSWGuiText {
public:
    CSWGuiEditText();

    void SetText(const CExoString& sText);
    // Removes the character before the caret.
    void RemoveLastChar();
    // Appends one NUL-terminated printable character.
    void AddNewChar(char* sChar);
    void SetCaretVisible(int bVisible);

    unsigned char m_bCaretVisible : 1;
    unsigned char : 7;
    CExoString m_sText;
};

K2_LAYOUT_ASSERT(sizeof(CSWGuiEditText) == 0x98, "CSWGuiEditbox's m_editText member extent");

#endif // CSWGUIEDITTEXT_H

// The image-rendering parameter block a GUI widget carries by value, holding the texture ResRef,
// rotation, alpha, tint colour, alignment and flip/draw style, plus a back-pointer to the
// CSWGuiImage that draws it. The structural twin of CSWGuiTextParams, down to the "every setter
// replays itself onto the bound widget" idiom.
#include "exoresman.h"

class CSWGuiImage;

class CSWGuiImageParams {
public:
    // The "build it in one go" constructor. Its argument order is not the member layout order,
    // since the two floats straddle the alignment argument.
    CSWGuiImageParams(const CResRef& resRefImage, int bDrawStyle, int nFlipStyle, float fRotate,
                      int nAlignment, float fAlpha, const Vector& color);
    CSWGuiImageParams(const CSWGuiImageParams& other);
    CSWGuiImageParams& operator=(const CSWGuiImageParams& other);

    // Binds this parameter block to the CSWGuiImage that renders it, then replays the whole
    // parameter set onto it.
    void SetImageObject(CSWGuiImage* pImageObject);

    // Swaps the texture: destroys the widget's current CAurGUIImageInternal and builds a new one.
    // bForce re-does the work even when the ResRef is unchanged, which is how SetImageObject
    // pushes an already-set ResRef onto a freshly bound widget.
    void SetImage(const CResRef& resRefImage, int bForce);

    void SetAlignment(int nAlignment);
    void SetDrawStyle(int bDrawStyle);

    CResRef m_resRefImage;
    float m_fRotate;
    float m_fAlpha;
    Vector m_color;

    unsigned short bHilight     : 1;
    unsigned short bDrawStyle   : 1;
    unsigned short m_nFlipStyle : 4;
    unsigned short m_nAlignment : 6;

    CSWGuiImage* m_pImageObject;
};

// The GUI image element, a CSWGuiDrawable that owns one CAurGUIImageInternal and the
// CSWGuiImageParams block describing how to draw it. The structural twin of CSWGuiText, which owns
// a string object and a CSWGuiTextParams, and of CSWGuiBorder, which owns three images.
//
// GetIdealHeight is deliberately not declared here: the class vtable names the base's, so an
// override would be a phantom.
class CAurGUIImageInternal;
class CExoString;
class CResGFF;
struct CResStruct;

class CSWGuiImage : public CSWGuiDrawable {
public:
    // Defining this destructor out of line is what emits the class vtable.
    virtual ~CSWGuiImage();
    virtual void SetExtent(const CSWGuiExtent& extent);
    virtual void Draw(float fAlpha);
    virtual void Load(CResGFF* pGFF, CResStruct& rStruct, const CExoString& sFieldName);

    CSWGuiImage();
    CSWGuiImage& operator=(CSWGuiImage& other);
    void Initialize(const CSWGuiExtent& extent, const CSWGuiImageParams& params);

    // Answers the texture's own size as a rectangle at the origin. Its x and y are always 0, only w
    // and h come back filled.
    CSWGuiExtent GetImageExtent();

    CSWGuiExtent m_imageExtent;
    CAurGUIImageInternal* m_pImage;
    CSWGuiImageParams m_params;
};

// The parameter block a CSWGuiIndicator carries by value, holding three texture ResRefs (corner,
// edge, terminator), the two endpoint anchors, whether each endpoint gets a terminator drawn on it,
// a shared alpha, and a back-pointer to the widget that renders them. Structurally the third of the
// family after CSWGuiTextParams and CSWGuiImageParams, and it follows the same two idioms.
#include "exoresman.h"

class CSWGuiIndicator;

class CSWGuiIndicatorParams {
public:
    CSWGuiIndicatorParams(const CResRef& resRefCornerImage, const CResRef& resRefEdgeImage,
                          const CResRef& resRefTerminatorImage, int nStartAnchor,
                          int bStartTerminator, int nEndAnchor, int bEndTerminator,
                          float fAlpha);
    CSWGuiIndicatorParams(const CSWGuiIndicatorParams& other);
    CSWGuiIndicatorParams& operator=(const CSWGuiIndicatorParams& other);

    // Binds this block to the widget that draws it, then replays all three textures onto it.
    void SetIndicator(CSWGuiIndicator* pIndicator);

    // One body written three times over, differing only in which ResRef is compared against and
    // which of the widget's three CAurGUIImageInternal pointers is rebuilt. bForce re-does the
    // work even when the ResRef is unchanged, which is how SetIndicator pushes already-set ResRefs
    // onto a freshly bound widget.
    void SetCornerImage(const CResRef& resRefCornerImage, int bForce);
    void SetEdgeImage(const CResRef& resRefEdgeImage, int bForce);
    void SetTerminatorImage(const CResRef& resRefTerminatorImage, int bForce);

    unsigned short bStartTerminator : 1;
    unsigned short bEndTerminator   : 1;
    unsigned short bReserved1       : 1;
    unsigned short m_nStartAnchor   : 6;
    unsigned short m_nEndAnchor     : 6;

    float m_fAlpha;

    CResRef m_resRefCornerImage;
    CResRef m_resRefEdgeImage;
    CResRef m_resRefTerminatorImage;

    CSWGuiIndicator* m_pIndicator;
};

// The GUI indicator element, a CSWGuiDrawable that draws a bracketed line between two corners of
// its extent out of three tiled textures. It owns one CAurGUIImageInternal per texture and carries
// the CSWGuiIndicatorParams block that describes them, exactly as CSWGuiImage owns one image plus
// its params and CSWGuiBorder owns three.
//
// GetIdealHeight is deliberately not declared here: the class vtable names the base's, so an
// override would be a phantom. SetExtent genuinely is overridden.
class CAurGUIImageInternal;
class CExoString;
class CResGFF;
struct CResStruct;

class CSWGuiIndicator : public CSWGuiDrawable {
public:
    // Defining this destructor out of line is what emits the class vtable.
    virtual ~CSWGuiIndicator();
    virtual void SetExtent(const CSWGuiExtent& extent);
    virtual void Draw(float fAlpha);
    // An empty body, since there is nothing to load from a GFF.
    virtual void Load(CResGFF* pGFF, CResStruct& rStruct, const CExoString& sFieldName) { K2_UNUSED(pGFF); K2_UNUSED(rStruct); K2_UNUSED(sFieldName); }

    CSWGuiIndicator();
    CSWGuiIndicator& operator=(CSWGuiIndicator& other);
    void Initialize(const CSWGuiExtent& extent, const CSWGuiIndicatorParams& params);

    // The three renderers Draw dispatches to on the XOR of the two anchors. Each tiles the edge
    // texture along its run, stamps the corner texture at the bends, and caps whichever endpoints
    // the two terminator bits ask for.
    void DrawStraightHorzLine();
    void DrawStraightVertLine();
    void DrawJointedLine();

    CSWGuiIndicatorParams m_params;

    CAurGUIImageInternal* m_pEdgeImage;
    CAurGUIImageInternal* m_pCornerImage;
    CAurGUIImageInternal* m_pTerminatorImage;
};

// The GUI 3D viewport element, a CSWGuiDrawable that owns a private Aurora scene plus its own
// camera, holds a list of the models placed in it, and each frame animates the scene and renders it
// through the camera inside a GUI viewport clipped to the widget's extent. This is what puts a
// spinning character or item model inside a 2D panel.
//
// SetExtent and GetIdealHeight are deliberately not declared here: the class vtable names the
// base's in both slots, so overrides would be phantoms.
#include "CExoArrayListDtors.h"

class CAurCamera;
class CAurObject;
class CAurScene;
class CExoString;
class CResGFF;
struct CResStruct;

class CSWGuiScene : public CSWGuiDrawable {
public:
    // Defining this destructor out of line is what emits the class vtable.
    virtual ~CSWGuiScene();
    virtual void Draw(float fAlpha);
    // An empty body, since there is nothing to load from a GFF.
    virtual void Load(CResGFF* pGFF, CResStruct& rStruct, const CExoString& sFieldName) { K2_UNUSED(pGFF); K2_UNUSED(rStruct); K2_UNUSED(sFieldName); }

    CSWGuiScene();

    // Places an already-built object in the scene. nIndex < 0 or past the end appends; otherwise
    // the object is inserted at that position, so the insertion index decides draw order.
    // bAttachToScene drives the object's own AttachToScene.
    int AddModel(CAurObject* pObject, int nIndex, int bAttachToScene);
    // The by-name overload builds the model and hands it to the one above, returning it only if it
    // took.
    CAurObject* AddModel(const CExoString& sModelName, int nIndex);
    int RemoveModel(int nIndex, int bDelete);
    CAurObject* GetModel(int nIndex);

    CAurScene*  m_pScene;
    CAurCamera* m_pCamera;
    CExoArrayList<CAurObject*> m_aModels;
    Vector m_color;
};
