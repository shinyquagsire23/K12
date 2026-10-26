#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurgui.h"

#include "aurtexman.h"
#include "exostring.h"
#include "aurglcore.h"
#include "aurmaterial.h"
#include "aurtexcontrol.h"
#include "Vector.h"
#if KOTOR1
#include "port/k1gui.h"
#endif

float CAurGUIImage::cm_fAlpha = 1.0f;
float CAurGUIImage::cm_fGUIBuffer[2048];
int CAurGUIImage::cm_nGUIBufferSizeUsed;
int CAurGUIImage::cm_nGUITID;
Material* CAurGUIImage::cm_pMaterial;
bool CAurGUIImage::cm_bIsVTC;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/k2qolapi.h"

// The cursor index a string carries when it shows no cursor.
enum
{
    kCursorNone = 0xffff,
};

// Packed 0xAABBGGRR colours: the RGB of white (an alpha byte is or-ed in), and the translucent
// black a (-2, -2, -2) colour asks for.
enum : unsigned int
{
    kColourWhiteRgb = 0x00ffffff,
    kColourTranslucentBlack = 0x96000000,
};

// A texture-coordinate pair (u, v) stored as one 64-bit word: kUvPairUV.
enum : uint64_t
{
    kUvPair00 = 0x0000000000000000ul,
    kUvPair10 = 0x000000003f800000ul,
    kUvPair01 = 0x3f80000000000000ul,
    kUvPair11 = 0x3f8000003f800000ul,
};

void AurTextureRelease(CAurTexture* pTexture);

extern int g_nScreenWidth;
extern int g_nScreenHeight;

// The GUI's resolution text scalar, 1.0 at the 976-px design width, plus 1 per 1024 px above it.
static float s_currentResolutionTextScalar = 1.0f;

float GetResolutionTextScalar()
{
#ifdef K2_QOL_IMPROVEMENTS
    // Scale text with the native 4:3 height, as K1 does, instead of with the screen width.
#ifdef K2_UI_PC
    const float fUnitHeight = 768.0f;
#else
    const float fUnitHeight = 540.0f;
#endif
    int nHeight = g_nScreenHeight;
    if (g_nScreenWidth > 0 && static_cast<int>(static_cast<float>(g_nScreenWidth) / 1.33f) < nHeight)
        nHeight = static_cast<int>(static_cast<float>(g_nScreenWidth) / 1.33f);
    float fScale = nHeight > 0 ? static_cast<float>(nHeight) / fUnitHeight : 1.0f;
#else
    int nWidth = g_nScreenWidth > 976 ? g_nScreenWidth : 976;
    float fScale = static_cast<float>((nWidth - 976) / 1024) + 1.0f;
#endif
    s_currentResolutionTextScalar = fScale;
    return fScale;
}

void CAurGUIStringInternal::SetAlignment(unsigned char alignment) {
    m_alignment = alignment;
}

unsigned char CAurGUIStringInternal::GetAlignment() {
    return m_alignment;
}

void CAurGUIStringInternal::SetAlignmentVertical(unsigned char vertical) {
    m_alignment = (m_alignment & kAlignHorizontalMask) | vertical;
}

unsigned char CAurGUIStringInternal::GetAlignmentVertical() {
    return m_alignment & kAlignVerticalMask;
}

void CAurGUIStringInternal::SetAlignmentHorizontal(unsigned char horizontal) {
    m_alignment = (m_alignment & kAlignVerticalMask) | horizontal;
}

unsigned char CAurGUIStringInternal::GetAlignmentHorizontal() {
    return m_alignment & kAlignHorizontalMask;
}

void CAurGUIStringInternal::SetColor(const Vector& color) {
    m_color = color;
}


CAurTexture* AurTextureGetReference(char* sName, char* sExtension);

// True when the pad glyphs use the PlayStation button order rather than the swapped one.
bool IsPsButtonIcons();

// Sets the string. The held buffer only grows, so a shorter string is copied into it in place.
void CAurGUIStringInternal::SetString(char* szText) {
    if (strlen(szText) > strlen(m_pText)) {
        delete[] m_pText;
        m_pText = new char[strlen(szText) + 1];
    }
    strcpy(m_pText, szText);
}

#if KOTOR1
short CAurGUIStringInternal::GetStringSize() {
    return static_cast<short>(strlen(m_pText));
}
#else
int CAurGUIStringInternal::GetStringSize() {
    return static_cast<int>(strlen(m_pText));
}
#endif

// SetString with the length supplied instead of measured: grow and copy when the request is
// longer than the held buffer, otherwise truncate in place.
void CAurGUIStringInternal::SetStringSize(short nSize) {
    if (nSize > static_cast<short>(strlen(m_pText))) {
        char* pText = new char[nSize + 1];
        strcpy(pText, m_pText);
        delete[] m_pText;
        m_pText = pText;
    } else {
        m_pText[nSize] = 0;
    }
}

Vector CAurGUIStringInternal::GetColor() {
    return m_color;
}

#if KOTOR1
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);

#ifndef K2_UI_TOUCH
#include "exoresman.h"

// pc / xbox: whether a texture exists in the data at all (a TGA or a TPC).
// AurTextureGetReference answers every name with a texture object, loaded on first use, so a
// missing one would draw blank.
static bool K1TextureExists(const char* pszName)
{
    CResRef resRef(pszName);
    return g_pExoResMan->Exists(resRef, 3, static_cast<unsigned int*>(0)) ||
           g_pExoResMan->Exists(resRef, 3007, static_cast<unsigned int*>(0));
}
#endif

// K1 binds one alternate face: the iPhone's d2x font. There is no d3x variant and no "_ps" set,
// and the replaced texture is dropped without an AurTextureRelease.
__attribute__((always_inline)) static inline void K1LoadIphoneFont(
    SafePointer<CAurTexture>& spFontD2, char* pszFont) {
    spFontD2 = 0;
    int nScreenWidth, nScreenHeight;
    if (IosIsIphone(nScreenWidth, nScreenHeight) && strstr(pszFont, "dialogfont16x16b"))
        spFontD2 = AurTextureGetReference(const_cast<char*>("d2xfont16x16b"), 0);
#ifndef K2_UI_TOUCH
    // pc / xbox: the PC HUD's labels ask for dialogfont10x10, whose glyph sheet is the same
    // 16-line atlas as dialogfont16x16b, so it takes the same 2x sprite.
    if (IosIsIphone(nScreenWidth, nScreenHeight) && strstr(pszFont, "dialogfont10x10b"))
        spFontD2 = AurTextureGetReference(const_cast<char*>("d2xfont16x16b"), 0);
#endif
    if (IosIsIphone(nScreenWidth, nScreenHeight) && strstr(pszFont, "fnt_d16x16b"))
        spFontD2 = AurTextureGetReference(const_cast<char*>("d2xfnt_d16x16b"), 0);
#ifndef K2_UI_TOUCH
    // The 2x atlases are the port's own; on a PC data set the text keeps the font's own atlas,
    // dialogfont16x16b / fnt_d16x16b.
    if (spFontD2.m_pObject && !K1TextureExists(spFontD2.m_pObject->GetName()))
        spFontD2 = 0;
#endif
}
#endif

#if KOTOR1 && defined(K2_QOL_CONTROLLER)

// The Controller page's button prompts are drawn from the chosen copy of the K1 glyph atlases,
// dialogfont16x16b and d2xfont16x16b; the metrics stay the font's own. Each texture is referenced
// once and kept, and matched by name, since every string holds its own CAurTexture for the font.
static CAurTexture* K1QolGlyphAtlas(CAurTexture* pAtlas, int nSize)
{
    static CAurTexture* s_apChosen[2][5];   // by size, then style; 0 is K1's own art
    int i = nSize == 32;
    const char* pszName = pAtlas ? pAtlas->GetName() : 0;
    const char* pszChosen;
    if (!pszName || strcasecmp(pszName, i ? "d2xfont16x16b" : "dialogfont16x16b") != 0 ||
        (pszChosen = k2qol_glyph_atlas(nSize)) == 0)
        return pAtlas;
    // The atlas style suffix is "_an", "_ps", "_nx" or "_gc", or "_io" for K1's own art.
    int nStyle = pszChosen[9] == 's' ? 2 : pszChosen[9] == 'x' ? 3 : pszChosen[9] == 'c' ? 4 :
                 pszChosen[9] == 'o' ? 0 : 1;
    if (!s_apChosen[i][nStyle])
        s_apChosen[i][nStyle] = AurTextureGetReference(const_cast<char*>(pszChosen), 0);
    return s_apChosen[i][nStyle] ? s_apChosen[i][nStyle] : pAtlas;
}
#endif

#if !KOTOR1 && defined(K2_QOL_IMPROVEMENTS)
#include "exoresman.h"

// An atlas whose texture is in no data set binds the null texture, which fixed-function GL draws
// untextured: every glyph a solid box in the text colour. A PC install has no diafnt16x16b_an, and
// a data set may lack the 2x / 3x atlases. Such a slot is released, so Draw falls back to the next
// atlas.
static void K2QolDropMissingAtlas(SafePointer<CAurTexture>& spAtlas)
{
    CAurTexture* pAtlas = spAtlas.m_pObject;
    if (!pAtlas)
        return;
    CResRef resRef(pAtlas->GetName());
    if (g_pExoResMan->Exists(resRef, 3, static_cast<unsigned int*>(0)) ||
        g_pExoResMan->Exists(resRef, 3007, static_cast<unsigned int*>(0)))
        return;
    spAtlas = 0;
    AurTextureRelease(pAtlas);
}
#endif

#if !KOTOR1
// Binds the fallback faces used when a dialog font is on screen: the _ps / _an button-icon set for
// the dialog fonts, and the _d set for names containing "fnt_d16x16b". Both write the same two
// slots, so the later test wins when both match.
void CAurGUIStringInternal::LoadAlternateFonts(const char* pszFont) {
    if (strstr(pszFont, "dialogfont16x16b") || strstr(pszFont, "dialogfont10x10b")) {
        m_spFontTextureD2 = AurTextureGetReference(const_cast<char*>("d2xfont16x16b"), 0);
        m_spFontTextureD3 = AurTextureGetReference(const_cast<char*>("d3xfont16x16b"), 0);
        bool bPsButtonIcons = IsPsButtonIcons();
        char* pszPS0 = const_cast<char*>(bPsButtonIcons ? "diafnt16x16b_ps" : "diafnt16x16b_an");
        char* pszPS1 = const_cast<char*>(bPsButtonIcons ? "d2xfont16x16b_ps" : "diafnt16x16b_an");
        char* pszPS2 = const_cast<char*>(bPsButtonIcons ? "d3xfont16x16b_ps" : "diafnt16x16b_an");
        m_spTexturesPS[0] = AurTextureGetReference(pszPS0, 0);
        m_spTexturesPS[1] = AurTextureGetReference(pszPS1, 0);
        m_spTexturesPS[2] = AurTextureGetReference(pszPS2, 0);
        if (m_spTexturesPS[0].m_pObject) {
            m_spTexturesPS[0].m_pObject->SetFilter(false);
        }
    }
    if (strstr(pszFont, "fnt_d16x16b")) {
        m_spFontTextureD2 = AurTextureGetReference(const_cast<char*>("d2xfnt_d16x16b"), 0);
        m_spFontTextureD3 = AurTextureGetReference(const_cast<char*>("d3xfnt_d16x16b"), 0);
    }
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDropMissingAtlas(m_spFontTextureD2);
    K2QolDropMissingAtlas(m_spFontTextureD3);
    for (int i = 0; i < 3; i++)
        K2QolDropMissingAtlas(m_spTexturesPS[i]);
#endif
}
#endif

#if KOTOR1
bool IsIphoneNoParams()
{
    int nScreenWidth, nScreenHeight;
    return IosIsIphone(nScreenWidth, nScreenHeight);
}

#endif
// Called only through NewCAurGUIString: binds the font set, copies the text into a new buffer and
// seeds the line list with its length.
CAurGUIStringInternal::CAurGUIStringInternal(char* szText, char* szFontTexture, Vector color,
                                            float fAlpha)
  : m_color(), m_lineOffsets(), m_lines()
{
    if (szFontTexture != 0) {
        m_spFontTexture = AurTextureGetReference(szFontTexture, 0);
#if KOTOR1
        K1LoadIphoneFont(m_spFontTextureD2, szFontTexture);
#else
        LoadAlternateFonts(szFontTexture);
#endif
    } else {
        m_spFontTexture = 0;
    }

    m_pText = new char[strlen(szText) + 1];
    strcpy(m_pText, szText);

    m_lines.Add(static_cast<int>(strlen(szText)));

    m_color = color;
    m_cursorIndex = kCursorNone;
    m_alignment = 9;
    m_positionX = 0;
    m_positionY = 0;
    m_sizeX = 0;
    m_sizeY = 0;

#if KOTOR1  // K1: the scale is 1.3x the screen height over 768 lines; the fAlpha seed is overwritten
    m_fFontScale = fAlpha;
    m_fFontScale = static_cast<float>(g_SDL_mode.h) * 1.3f / 768.0f;
#else
    float fScale = GetResolutionTextScalar();
    m_fResolutionScale = fScale;
    m_fFontScale = fScale * fAlpha;
#endif
}

// Rebinds the whole font set: every watched texture is unregistered and released, then the new
// primary font is looked up and its alternates loaded. A null name leaves no primary font.
void CAurGUIStringInternal::SetFontTexture(char* pszFont) {
    CAurTexture* pOldTexture;

    pOldTexture = m_spFontTexture.m_pObject;
    if (pOldTexture != 0) {
        m_spFontTexture = 0;
        AurTextureRelease(pOldTexture);
    }
#if KOTOR1  // K1: only the primary font is released; the d2x font is rebound first
    if (pszFont != 0) {
        K1LoadIphoneFont(m_spFontTextureD2, pszFont);
        m_spFontTexture = AurTextureGetReference(pszFont, 0);
    } else {
        m_spFontTexture = 0;
    }
#else

    CAurTexture* pOldD2 = m_spFontTextureD2.m_pObject;
    CAurTexture* pOldD3 = m_spFontTextureD3.m_pObject;
    m_spFontTextureD2 = 0;
    m_spFontTextureD3 = 0;
    AurTextureRelease(pOldD2);
    AurTextureRelease(pOldD3);

    pOldTexture = m_spTexturesPS[0].m_pObject;
    m_spTexturesPS[0] = 0;
    AurTextureRelease(pOldTexture);

    pOldTexture = m_spTexturesPS[1].m_pObject;
    m_spTexturesPS[1] = 0;
    AurTextureRelease(pOldTexture);

    pOldTexture = m_spTexturesPS[2].m_pObject;
    m_spTexturesPS[2] = 0;
    AurTextureRelease(pOldTexture);

    if (pszFont != 0) {
        m_spFontTexture = AurTextureGetReference(pszFont, 0);
        LoadAlternateFonts(pszFont);
    } else {
        m_spFontTexture = 0;
    }
#endif
    m_lineOffsets.m_count = 0;
}

// Returns the scaled point size of the bound font's face, its far-edge metric in hundredths of a
// point, times the resolution text scalar, rounded.
int CAurGUIStringInternal::GetFontPixelHeight() {
#if KOTOR1  // K1: scaled by the screen height over 768 lines, rounded up
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    CAurFontInfo* pFontInfo = m_spFontTexture.m_pObject->GetFontInfo();
    float fSize = pFontInfo->m_fBottom * 100.0f;
    return static_cast<int>(__builtin_ceilf(fSize * s_fScale + s_fScale * 0.5f));
#else
    CAurFontInfo* pFontInfo = m_spFontTexture.m_pObject->GetFontInfo();
    float fSize = pFontInfo->m_fBottom * 100.0f;
    float fScale = GetResolutionTextScalar();
    return static_cast<int>(fSize * fScale + 0.5f);
#endif
}

#if KOTOR1
void CAurGUIStringInternal::SetFontScale(float fScale) {
    m_fFontScale = fScale;
}
#endif

#if !KOTOR1
// Re-derives the cached layout scale after a resolution change, and only when the scalar actually
// moved: every widget's font size is expressed relative to it.
void CAurGUIStringInternal::UpdateFontScale() {
    float fOld = m_fResolutionScale;
    float fScale = s_currentResolutionTextScalar;
    if (fOld == fScale) {
        return;
    }
    m_fResolutionScale = fScale;
    m_fFontScale = m_fFontScale * (fScale / fOld);
}
#endif

// The ASL GL wrapper for this file.

// Indices into the GL translation tables for the capabilities, primitive types and matrix modes

// The characters 0x10..0x16 are the controller-button glyphs.
enum { kPadGlyphFirst = 0x10, kPadGlyphCount = 7 };

// An image's flip bits (the a argument of the Draw family).
enum { kImageFlipMask = 0xc, kImageFlipV = 0x4, kImageFlipH = 0x8 };

int noguirender;

short CAurGUIDraw::cm_nCurrentViewport;
CAurGUIDraw::Viewport CAurGUIDraw::cm_nViewportStack[10];

// Returns the running alpha of the current viewport. The stack holds nested scissor clips whose
// alphas multiply, and -1 means nothing has been pushed. The accumulation includes the current
// entry.
float GetViewportAlpha() {
    int64_t nViewport = CAurGUIDraw::cm_nCurrentViewport;
    if (nViewport < 0) {
        return 1.0f;
    }
    float fAlpha = 1.0f;
    for (int64_t i = 0; i <= nViewport; i++) {
        fAlpha *= CAurGUIDraw::cm_fViewportAlpha[i];
    }
    return fAlpha;
}

// Pushes the GUI layer. The current GL viewport becomes stack entry 0 (the whole screen), then a
// 0..1 NDC projection is installed so every widget can position in normalized units.
void AurGUIStartLayer() {
    int anViewport[4];
    GLRender::GetViewPort(anViewport);

    CAurGUIDraw::cm_nViewportStack[0].nX = static_cast<short>(anViewport[0]);
    CAurGUIDraw::cm_nViewportStack[0].nY = static_cast<short>(anViewport[1]);
    CAurGUIDraw::cm_nViewportStack[0].nWidth = static_cast<short>(anViewport[2]);
    CAurGUIDraw::cm_nViewportStack[0].nHeight = static_cast<short>(anViewport[3]);
    CAurGUIDraw::cm_nViewportStack[0].bBlend = 0;
    CAurGUIDraw::cm_fViewportAlpha[0] = 1.0f;
    CAurGUIDraw::cm_nCurrentViewport = 0;

    GLRender::Disable(Aurora::kCapLighting);
    GLRender::Disable(Aurora::kCapDepthTest);

    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
    ASLgl::glLoadIdentity();

    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
}

// Pops the layer; -1 in the 16-bit index marks it as not current any more.
void AurGUIStopLayer() {
    CAurGUIDraw::cm_nCurrentViewport = -1;
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
    GLRender::Enable(Aurora::kCapLighting);
    GLRender::Enable(Aurora::kCapDepthTest);
}

// Pushes one scissor sub-viewport: clips the rectangle against the screen, accumulates it onto the
// previous stack entry, installs it as the GL viewport and paints it. Returns 0 when the rectangle
// is rejected (entirely off screen, degenerate, or GUI rendering disabled).
//
// vColor is both a colour and a mode: (-1,-1,-1) pushes the viewport but paints nothing, (-2,-2,-2)
// paints a fixed grey, and any other value is the RGB. The alpha comes from the float parameter,
// not from the vector. bClear also clears depth and stencil and turns blending on.
int AurGUISetupViewport(int x, int y, int w, int h, const Vector& vColor, bool bClear, float fAlpha)
{
    if (x + w < 0 || noguirender || g_nScreenWidth <= x || h < 1 || w < 1 || y + h < 0 ||
        g_nScreenHeight <= y)
        return 0;

    short nParent = CAurGUIDraw::cm_nCurrentViewport;
#ifdef K2_HOST_BUILD
    // The GUI draws on unit 0 only. The post-process passes can leave units 1-3 enabled under
    // gl4es's fixed-function emulation, which would multiply every GUI quad by those textures.
    if (nParent == 0) {
        for (unsigned int nUnit = 1; nUnit < 4; nUnit++) {
            ASLgl::glActiveTextureARB(GL_TEXTURE0 + nUnit);
            ASLgl::glDisable(GL_TEXTURE_2D);
            ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        }
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
    }
#endif
    const CAurGUIDraw::Viewport& parent = CAurGUIDraw::cm_nViewportStack[nParent];
    short nViewport = ++CAurGUIDraw::cm_nCurrentViewport;
    CAurGUIDraw::Viewport& viewport = CAurGUIDraw::cm_nViewportStack[nViewport];

    viewport.nX = parent.nX + static_cast<short>(x);
    viewport.nY = parent.nY + static_cast<short>(y);
    viewport.nWidth = static_cast<short>(w);
    viewport.nHeight = static_cast<short>(h);
    viewport.bBlend = bClear;

    CAurGUIDraw::cm_fViewportAlpha[nViewport] = fAlpha;

    // The stack is in port screen coordinates (origin top left), so the GL row is the flipped one.
    ASLgl::glViewport(viewport.nX,
                      CAurGUIDraw::cm_nViewportStack[0].nHeight - (viewport.nY + h), w, h);

    float fRed = vColor.x;
    float fGreen = vColor.y;
    float fBlue = vColor.z;
    int nAlpha = static_cast<int>(fAlpha * 255.0f);
    int bNotSentinel = vColor != Vector(-1.0f, -1.0f, -1.0f);
    if (nAlpha >= 1 && bNotSentinel) {
        // glColor4ubv reads the bytes in memory order: R = x, G = y, B = z, A = alpha.
        unsigned int nPacked = static_cast<unsigned int>(static_cast<int>(fRed * 255.0f)) |
                               (static_cast<unsigned int>(nAlpha) << 24) |
                               (static_cast<unsigned int>(static_cast<int>(fGreen * 255.0f)) << 8) |
                               (static_cast<unsigned int>(static_cast<int>(fBlue * 255.0f)) << 16);
        unsigned int nColor = equal(vColor, Vector(-2.0f, -2.0f, -2.0f)) ? kColourTranslucentBlack : nPacked;
        GLRender::Disable(Aurora::kCapTexture2D);
        GLRender::Disable(Aurora::kCapLighting);
        GLRender::BeginImmediatePrimitive(Aurora::kPrimQuads);
        GLRender::SetVertexColor(nColor);
        ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
        ASLgl::glVertex4f(1.0f, 0.0f, 0.0f, 1.0f);
        ASLgl::glVertex4f(1.0f, 1.0f, 0.0f, 1.0f);
        ASLgl::glVertex4f(0.0f, 1.0f, 0.0f, 1.0f);
        ASLgl::glEnd();
        GLRender::Enable(Aurora::kCapTexture2D);
        GLRender::Enable(Aurora::kCapLighting);
    }
    if (bClear) {
        ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        GLRender::Enable(Aurora::kCapDepthTest);
    }
    return 1;
}
// Entry 0 of the viewport stack is the whole screen: the one viewport whose size the GUI reads
// directly.
void AurGUIGetScreenSize(int& nWidth, int& nHeight) {
    nWidth = CAurGUIDraw::cm_nViewportStack[0].nWidth;
    nHeight = CAurGUIDraw::cm_nViewportStack[0].nHeight;
}

// Pop one scissor sub-viewport and re-install the one below it as the GL viewport. The stack is
// in port screen coordinates whose origin is the top left, so the GL row is the flipped one.
void AurGUICloseViewport() {
    if (noguirender)
        return;

    short nViewport = --CAurGUIDraw::cm_nCurrentViewport;
    const CAurGUIDraw::Viewport& viewport = CAurGUIDraw::cm_nViewportStack[nViewport];
    int nBottom = CAurGUIDraw::cm_nViewportStack[0].nHeight - viewport.nY - viewport.nHeight;

    ASLgl::glViewport(viewport.nX, nBottom, viewport.nWidth, viewport.nHeight);

    if (CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].bBlend)
        GLRender::Enable(Aurora::kCapDepthTest);
    else
        GLRender::Disable(Aurora::kCapDepthTest);
}

// Draws the debug outline of the object's bounding rect as a GL line loop in NDC. The pixel corners
// are divided by the current viewport and the Y axis flipped. The Vector and float parameters are
// unused.
void CAurGUIStringInternal::DrawBoundingRect(Vector, float) {
    if (noguirender) {
        return;
    }

    GLRender::Disable(Aurora::kCapLighting);
    GLRender::Disable(Aurora::kCapTexture2D);
    GLRender::Disable(Aurora::kCapBlend);

    const CAurGUIDraw::Viewport& viewport =
        CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport];
    float fWidth = static_cast<float>(viewport.nWidth);
    float fHeight = static_cast<float>(viewport.nHeight);
    float fLeft = static_cast<float>(m_positionX) / fWidth;
    float fTop = 1.0f - static_cast<float>(m_positionY) / fHeight;
    float fBottom = 1.0f - static_cast<float>(m_positionY + m_sizeY) / fHeight;
    float fRight = static_cast<float>(m_positionX + m_sizeX) / fWidth;

    GLRender::BeginImmediatePrimitive(Aurora::kPrimLineLoop);
    GLRender::SetVertexColor(0xffe5a566);
    ASLgl::glVertex4f(fLeft, fTop, 0.0f, 1.0f);
    ASLgl::glVertex4f(fLeft, fBottom, 0.0f, 1.0f);
    ASLgl::glVertex4f(fRight, fBottom, 0.0f, 1.0f);
    ASLgl::glVertex4f(fRight, fTop, 0.0f, 1.0f);
    ASLgl::glVertex4f(fLeft, fBottom, 0.0f, 1.0f);
    ASLgl::glEnd();

    GLRender::Enable(Aurora::kCapLighting);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::Enable(Aurora::kCapTexture2D);
}

// The four float-overload bounding-rect accessors convert between the object's raw pixel
// coordinates and NDC through the current viewport: widgets position in normalized units while the
// layout members stay in pixels.

void CAurGUIStringInternal::GetPositionBoundingRect(float& x, float& y) {
    const CAurGUIDraw::Viewport& viewport =
        CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport];
    x = static_cast<float>(m_positionX) / static_cast<float>(viewport.nWidth);
    y = static_cast<float>(m_positionY) / static_cast<float>(viewport.nHeight);
}

void CAurGUIStringInternal::SetPositionBoundingRect(float x, float y) {
    const CAurGUIDraw::Viewport& viewport =
        CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport];
    m_positionX = static_cast<int>(x * static_cast<float>(viewport.nWidth));
    m_positionY = static_cast<int>(y * static_cast<float>(viewport.nHeight));
}

void CAurGUIStringInternal::GetSizeBoundingRect(float& x, float& y) {
    const CAurGUIDraw::Viewport& viewport =
        CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport];
    x = static_cast<float>(m_sizeX) / static_cast<float>(viewport.nWidth);
    y = static_cast<float>(m_sizeY) / static_cast<float>(viewport.nHeight);
}

void CAurGUIStringInternal::SetSizeBoundingRect(float x, float y) {
    const CAurGUIDraw::Viewport& viewport =
        CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport];
#if KOTOR1  // K1: the pixel size is rounded up
    m_sizeX = static_cast<int>(__builtin_ceilf(x * static_cast<float>(viewport.nWidth)));
    m_sizeY = static_cast<int>(__builtin_ceilf(y * static_cast<float>(viewport.nHeight)));
#else
    m_sizeX = static_cast<int>(x * static_cast<float>(viewport.nWidth));
    m_sizeY = static_cast<int>(y * static_cast<float>(viewport.nHeight));
#endif
}

// Returns the height the string wants at the given wrap width; nWidth == -1 means "as laid out
// already" and is only answered directly when there is a layout. Measuring is destructive
// (WrapStrings rebuilds both layout lists), so the current layout is copied out first and restored
// afterwards. The two measured arms' formulas differ. The nWidth != -1 arm rounds the per-line cell
// height to int first and scales that, the nWidth == -1 arm scales the un-rounded float height and
// rounds once.
int CAurGUIStringInternal::GetIdealPixelHeight(int nWidth)
{
#if KOTOR1
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
#else
    UpdateFontScale();
#endif
#ifdef K2_QOL_IMPROVEMENTS
    // Rounded up, as K1 does, because the text scalar is fractional and Draw drops a line whose
    // cell is any fraction taller than the box it was sized for.
#endif

    if (nWidth != -1) {
        List<int> savedOffsets(m_lineOffsets);
        List<int> savedLines(m_lines);

        WrapStrings(nWidth);

        int nLines = m_lines.m_count;
        m_lines = savedLines;
        m_lineOffsets = savedOffsets;

        CAurFontInfo* pFontInfo = m_spFontTexture.m_pObject->GetFontInfo();
#if KOTOR1  // K1: the size scale goes into the cell height; both roundings are ceilings
        int nCellHeight = static_cast<int>(__builtin_ceilf(
            (pFontInfo->m_fBottom + pFontInfo->m_fSpacingB) * m_fFontScale * 100.0f + s_fScale * 0.5f));
        return static_cast<int>(__builtin_ceilf(s_fScale * 0.5f + static_cast<float>(nCellHeight))) * static_cast<short>(nLines);
#else
        int nCellHeight = static_cast<int>((pFontInfo->m_fBottom + pFontInfo->m_fSpacingB) * 100.0f + 0.5f);
#ifdef K2_QOL_IMPROVEMENTS
        (void)nCellHeight;
        return static_cast<int>(__builtin_ceilf((pFontInfo->m_fBottom + pFontInfo->m_fSpacingB) * 100.0f *
                                    m_fFontScale)) * static_cast<short>(nLines);
#else
        return static_cast<int>(static_cast<float>(nCellHeight) * m_fFontScale + 0.5f) * static_cast<short>(nLines);
#endif
#endif
    }

    if (m_lines.m_count != 0) {
        CAurFontInfo* pFontInfo = m_spFontTexture.m_pObject->GetFontInfo();
#if KOTOR1  // K1: rounded up by half the screen scale
        return static_cast<int>(__builtin_ceilf((pFontInfo->m_fBottom + pFontInfo->m_fSpacingB) * 100.0f *
                                        m_fFontScale * static_cast<float>(m_lines.m_count) +
                                    s_fScale * 0.5f));
#else
#ifdef K2_QOL_IMPROVEMENTS
        return static_cast<int>(__builtin_ceilf((pFontInfo->m_fBottom + pFontInfo->m_fSpacingB) * 100.0f *
                                    m_fFontScale * static_cast<float>(m_lines.m_count)));
#else
        return static_cast<int>((pFontInfo->m_fBottom + pFontInfo->m_fSpacingB) * 100.0f * m_fFontScale *
                         static_cast<float>(m_lines.m_count) + 0.5f);
#endif
#endif
    }

    List<int> savedOffsets(m_lineOffsets);
    List<int> savedLines(m_lines);

    WrapStrings(-1);

    int nLines = m_lines.m_count;
    m_lines = savedLines;
    m_lineOffsets = savedOffsets;

    CAurFontInfo* pFontInfo = m_spFontTexture.m_pObject->GetFontInfo();
#if KOTOR1  // K1: rounded up by half the screen scale
    return static_cast<int>(__builtin_ceilf((pFontInfo->m_fBottom + pFontInfo->m_fSpacingB) * 100.0f *
                                    m_fFontScale * static_cast<float>(static_cast<short>(nLines)) +
                                s_fScale * 0.5f));
#else
#ifdef K2_QOL_IMPROVEMENTS
    return static_cast<int>(__builtin_ceilf((pFontInfo->m_fBottom + pFontInfo->m_fSpacingB) * 100.0f *
                                m_fFontScale * static_cast<float>(static_cast<short>(nLines))));
#else
    return static_cast<int>((pFontInfo->m_fBottom + pFontInfo->m_fSpacingB) * 100.0f * m_fFontScale *
                     static_cast<float>(static_cast<short>(nLines)) + 0.5f);
#endif
#endif
}

// Lays the string out into lines of at most nWidth pixels (nWidth == -1 means the object's own
// m_sizeX). Rebuilds both layout lists from scratch: m_lines holds each line's length in characters
// and m_lineOffsets its accumulated pixel width. The scan is a greedy word wrap: it measures
// characters until the running width would exceed the limit, remembers the last break opportunity
// (a space, or a hyphen flanked by two letters) and breaks there; with no break opportunity it
// breaks mid-word.
//
// K2_FIX_WRAP_ROUNDING makes the per-glyph measure round up, so a line no longer draws past the
// width it was wrapped to.
#if !KOTOR1
#ifdef K2_FIX_WRAP_ROUNDING
static const float kGlyphRoundBias = 0.96875f;
#else
static const float kGlyphRoundBias = 0.25f;
#endif
#endif
#if KOTOR1
// K1: a glyph's advance in pixels, left unscaled here and scaled and rounded at the call sites.
static inline float K1GlyphAdvance(CAurFontInfo* pFontInfo, float fScale, unsigned char c) {
    return fScale * pFontInfo->m_fSpacing * 100.0f +
           fScale * ((pFontInfo->m_aMax[c].x - pFontInfo->m_aMin[c].x) * pFontInfo->m_fWidthScale) *
               100.0f;
}
#endif
void CAurGUIStringInternal::WrapStrings(int nWidth)
{
#if KOTOR1  // K1: an object under 30 px wide falls back to the plain screen scale
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    if (m_sizeX < 30)
        m_fFontScale = s_fScale;
#endif
    if (m_positionX == 0 && m_sizeX == 0)
        return;

#if !KOTOR1
    UpdateFontScale();
#endif

    CAurFontInfo* pFontInfo = m_spFontTexture.m_pObject->GetFontInfo();

    m_lineOffsets.m_count = 0;
    m_lines.m_count = 0;

    char* pszText = m_pText;
#if KOTOR1  // K1: every glyph width below is rounded up
    int nCharWidth = static_cast<int>(__builtin_ceilf((pFontInfo->m_aMax[static_cast<unsigned char>(*pszText)].x -
                                           pFontInfo->m_aMin[static_cast<unsigned char>(*pszText)].x) *
                                          pFontInfo->m_fWidthScale * m_fFontScale * 100.0f));
#else
    int nCharWidth = static_cast<int>((pFontInfo->m_aMax[static_cast<unsigned char>(*pszText)].x -
                            pFontInfo->m_aMin[static_cast<unsigned char>(*pszText)].x) *
                           pFontInfo->m_fWidthScale * m_fFontScale * 100.0f);
#endif

    if (nWidth == -1)
        nWidth = m_sizeX;

    short nMaxWidth = static_cast<short>(nWidth);

#if KOTOR1
    int nOSWidth = static_cast<int>(__builtin_ceilf(K1GlyphAdvance(pFontInfo, m_fFontScale, 111)));
#else
    int nOSWidth = static_cast<int>((pFontInfo->m_fSpacing + pFontInfo->m_aMax[111].x -
                          pFontInfo->m_aMin[111].x) *
                         pFontInfo->m_fWidthScale * m_fFontScale * 100.0f);
#endif

    if (strlen(m_pText) == 1 && static_cast<short>(nWidth) < nOSWidth)
        return;
    if (strlen(m_pText) <= 2 && static_cast<short>(nWidth) < 2 * nOSWidth)
        return;
#if !KOTOR1
    if (m_pText[0] == 0)
        return;
#endif

    char* pszCur = pszText;
    int nCurWidth = nCharWidth;
    int nAcc = nCharWidth;

#if KOTOR1  // K1: no pass limit; it stops past 1000 lines instead
    while (*pszCur != 0)
#else
    for (int nCount = 0; nCount < 100; nCount++)
#endif
    {
        char* pszNext = pszCur + 1;
        while (*pszCur == '\n')
        {
            m_lineOffsets.Add(0);
            m_lines.Add(0);
            pszCur++;
            pszNext++;
        }

        char* pszBreak = 0;
        char* pszScan = pszCur;

        if (*pszCur == 0)
            nCurWidth = nMaxWidth;

        if (nCurWidth < nMaxWidth)
        {
            while (true)
            {
                pszScan = pszNext;
#if KOTOR1  // K1: only a hyphen between two letters is a break opportunity here
                if (pszScan[-1] == '-' && isalpha(static_cast<unsigned char>(pszScan[-2])) &&
                    isalpha(static_cast<unsigned char>(*pszScan)))
#else
                if (pszScan[-1] == ' ' ||
                    (pszScan[-1] == '-' && isalpha(static_cast<unsigned char>(pszScan[-2])) &&
                                           isalpha(static_cast<unsigned char>(*pszScan))))
#endif
                {
                    pszBreak = pszScan - 1;
                    nAcc = nCurWidth;
                }
                if (*pszScan == 0 || *pszScan == '\n')
                    break;
#if KOTOR1  // K1: rounded up past a quarter of the screen scale
                nCurWidth += static_cast<int>(__builtin_ceilf(
                    s_fScale * 0.25f + K1GlyphAdvance(pFontInfo, m_fFontScale, static_cast<unsigned char>(*pszScan))));
#else
                nCurWidth += static_cast<int>(m_fFontScale * (pFontInfo->m_fSpacing +
                                     (pFontInfo->m_aMax[static_cast<unsigned char>(*pszScan)].x -
                                      pFontInfo->m_aMin[static_cast<unsigned char>(*pszScan)].x) *
                                      pFontInfo->m_fWidthScale) * 100.0f + kGlyphRoundBias);
#endif
                if (*pszScan == ' ')
                {
                    pszBreak = pszScan;
                    nAcc = nCurWidth;
                }
                pszNext = pszScan + 1;
                if (nCurWidth >= nMaxWidth)
                    break;
            }
        }

        if (*pszScan != 0 && *pszScan != '\n')
        {
            if (pszBreak != 0)
            {
                int nLen = static_cast<int>(pszBreak - pszCur);
                if (*pszBreak == ' ')
                {
                    m_lines.Add(nLen);
#if KOTOR1
                    nAcc -= static_cast<int>(__builtin_ceilf(K1GlyphAdvance(pFontInfo, m_fFontScale, 32)));
#else
                    nAcc -= static_cast<int>(m_fFontScale * (pFontInfo->m_fSpacing +
                                 (pFontInfo->m_aMax[32].x - pFontInfo->m_aMin[32].x) *
                                  pFontInfo->m_fWidthScale) * 100.0f);
#endif
                }
                else
                {
                    m_lines.Add(nLen + 1);
                }
                m_lineOffsets.Add(static_cast<short>(nAcc));
                pszCur = pszBreak + 1;
            }
            else
            {
                char* pszLast = pszScan - 1;
#if KOTOR1
                if (pszLast == m_pText)
#else
                if (pszLast == pszText)
#endif
                {
                    m_lineOffsets.m_count = 0;
                    m_lines.m_count = 0;
                    return;
                }
#if KOTOR1
                nAcc = static_cast<unsigned short>(nCurWidth -
                           static_cast<int>(__builtin_ceilf(
                               K1GlyphAdvance(pFontInfo, m_fFontScale, static_cast<unsigned char>(*pszScan)))) -
                           static_cast<int>(__builtin_ceilf(
                               K1GlyphAdvance(pFontInfo, m_fFontScale, static_cast<unsigned char>(*pszLast))))) +
                       static_cast<int>(__builtin_ceilf(K1GlyphAdvance(pFontInfo, m_fFontScale, 45)));
#else
                nAcc = static_cast<unsigned short>(nCurWidth -
                           static_cast<int>(m_fFontScale * (pFontInfo->m_fSpacing +
                                  (pFontInfo->m_aMax[static_cast<unsigned char>(*pszScan)].x -
                                   pFontInfo->m_aMin[static_cast<unsigned char>(*pszScan)].x) *
                                  pFontInfo->m_fWidthScale) * 100.0f) -
                           static_cast<int>(m_fFontScale * (pFontInfo->m_fSpacing +
                                  (pFontInfo->m_aMax[static_cast<unsigned char>(*pszLast)].x -
                                   pFontInfo->m_aMin[static_cast<unsigned char>(*pszLast)].x) *
                                  pFontInfo->m_fWidthScale) * 100.0f)) +
                       static_cast<int>(m_fFontScale * (pFontInfo->m_fSpacing +
                              (pFontInfo->m_aMax[45].x - pFontInfo->m_aMin[45].x) *
                              pFontInfo->m_fWidthScale) * 100.0f);
#endif
                m_lines.Add(static_cast<int>(pszCur - pszLast));
                m_lineOffsets.Add(static_cast<short>(nAcc));
                pszCur = pszLast;
            }
        }
        else
        {
            m_lineOffsets.Add(nCurWidth);
            m_lines.Add(static_cast<int>(pszScan - pszCur));
            pszCur = (*pszScan == '\n') ? pszScan + 1 : pszScan;
        }

#if KOTOR1
        nCurWidth = static_cast<int>(__builtin_ceilf((pFontInfo->m_aMax[static_cast<unsigned char>(*pszCur)].x -
                                          pFontInfo->m_aMin[static_cast<unsigned char>(*pszCur)].x) *
                                         pFontInfo->m_fWidthScale * m_fFontScale * 100.0f));
        if (m_lines.m_count > 1000)
            return;
    }
#else
        nCurWidth = static_cast<int>((pFontInfo->m_aMax[static_cast<unsigned char>(*pszCur)].x -
                           pFontInfo->m_aMin[static_cast<unsigned char>(*pszCur)].x) *
                          pFontInfo->m_fWidthScale * m_fFontScale * 100.0f);
        if (*pszCur == 0)
            return;
    }
#endif

#if !KOTOR1
    CExoString sDebug(m_pText);
#endif
}

bool IsJoystickConnected();
#if KOTOR1
bool ASLPlat_IsControllerPlugged();
#endif

#if KOTOR1 && defined(K2_QOL_CONTROLLER)
// A pad glyph the renderer leaves out (no pad plugged in, or past a line's first eight characters)
// takes no room, and neither does the space after it, so a prefixed caption stays centred.
static bool UndrawnPadGlyph(const char* pszText, int i)
{
    unsigned char c = static_cast<unsigned char>(pszText[i]);
    return static_cast<unsigned char>(c - kPadGlyphFirst) < kPadGlyphCount && !(ASLPlat_IsControllerPlugged() && i < 8);
}

static bool NoRoomForPadGlyph(const char* pszText, int i)
{
    return UndrawnPadGlyph(pszText, i) || (i > 0 && pszText[i] == ' ' && UndrawnPadGlyph(pszText, i - 1));
}
#endif

// Draws the string with one quad per glyph, in the object's own colour, laid out from the
// m_lines/m_lineOffsets pair WrapStrings built. Vertical alignment (m_alignment bits 3-5) picks
// which leading lines to skip, horizontal alignment (bits 0-2) offsets the pen; each character then
// advances the pen by one glyph's width and each line drops the pen by one cell height.
void CAurGUIStringInternal::Draw(float fAlpha)
{
#if KOTOR1  // K1: screen-height scale, the d2x font bound over the primary, unscaled line/pen spacing
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif

    if (static_cast<int>(strlen(m_pText)) == 0 || noguirender)
        return;

    GLRender::Disable(Aurora::kCapLighting);
    GLRender::cm_bEnablePausePlane = 0;

    if (m_positionX == 0 && m_sizeX == 0)
        return;

    CAurFontInfo* pFontInfo = m_spFontTexture.m_pObject->GetFontInfo();

    char* pszText = m_pText;
    int nLine = 0;
    float fPenY = static_cast<float>(m_positionY) /
                  static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nHeight);

    if ((m_alignment & kAlignVerticalMask) == kAlignMiddle) {
        float fCellHeight = m_fFontScale * pFontInfo->m_fBottom * 100.0f;
        float fHeight =
            static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nHeight);
        float fOffset =
            (static_cast<float>(m_sizeY) - fCellHeight * static_cast<float>(m_lines.m_count)) / (fHeight + fHeight);
        if (fOffset < 0.0f) {
            float fStep = fCellHeight / fHeight;
            do {
                int nChars = m_lines.m_pData[nLine];
                fOffset += fStep;
                pszText += nChars < 0 ? -nChars : nChars;
                if (*pszText == ' ' || *pszText == '\n')
                    pszText++;
                nLine++;
            } while (fOffset < 0.0f);
        }
        fPenY += fOffset;
    } else if ((m_alignment & kAlignVerticalMask) == kAlignBottom) {
        float fCellHeight = m_fFontScale * pFontInfo->m_fBottom * 100.0f;
        float fBlockHeight = fCellHeight * static_cast<float>(m_lines.m_count);
        float fSizeY = static_cast<float>(m_sizeY);
        if (fBlockHeight > fSizeY) {
            do {
                fBlockHeight -= fCellHeight;
                int nChars = m_lines.m_pData[nLine];
                pszText += nChars < 0 ? -nChars : nChars;
                if (*pszText == ' ' || *pszText == '\n')
                    pszText++;
                nLine++;
            } while (fBlockHeight > fSizeY);
        }
        fPenY += (fSizeY - fBlockHeight) /
                 static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nHeight);
    }

#ifdef K2_QOL_CONTROLLER
    K1QolGlyphAtlas(m_spFontTexture.m_pObject, 16)->Bind();
    if (m_spFontTextureD2.m_pObject != 0)
        K1QolGlyphAtlas(m_spFontTextureD2.m_pObject, 32)->Bind();
#else
    m_spFontTexture.m_pObject->Bind();
    if (m_spFontTextureD2.m_pObject != 0)
        m_spFontTextureD2.m_pObject->Bind();
#endif

    GLRender::BeginImmediatePrimitive(Aurora::kPrimQuads);

#ifdef K2_HOST_BUILD
    float fHalfTexelV = 0.0f;
    {
        CAurTexture* pAtlas = m_spFontTextureD2.m_pObject ? m_spFontTextureD2.m_pObject
                                                           : m_spFontTexture.m_pObject;
        int nH = pAtlas ? pAtlas->GetHeight() : 0;
        if (nH > 0)
            fHalfTexelV = 0.5f / static_cast<float>(nH);
    }
#endif
    float fGlyphScale = pFontInfo->m_fWidthScale * m_fFontScale * 100.0f;
    float fUnitX =
        fGlyphScale / static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nWidth);
    float fUnitY = fGlyphScale /
                   static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nHeight);

    for (; nLine < m_lines.m_count; nLine++) {
        while (nLine < m_lines.m_count && m_lines.m_pData[nLine] == 0) {
            unsigned char c = static_cast<unsigned char>(*pszText++);
            fPenY += (pFontInfo->m_fSpacingB * m_fFontScale * 100.0f) /
                         static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                             .nHeight) +
                     fUnitY * (pFontInfo->m_aMin[c].y - pFontInfo->m_aMax[c].y);
            nLine++;
        }
        if (nLine >= m_lines.m_count)
            break;

        unsigned char cFirst = static_cast<unsigned char>(*pszText);
        int nChars = m_lines.m_pData[nLine];
        int nAbsChars = nChars < 0 ? -nChars : nChars;

        if (fPenY + ((pFontInfo->m_fSpacingB * m_fFontScale * 100.0f) /
                         static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                             .nHeight) +
                     fUnitY * (pFontInfo->m_aMin[cFirst].y - pFontInfo->m_aMax[cFirst].y)) <
            0.0f) {
            pszText += nChars;
            unsigned char c = static_cast<unsigned char>(*pszText);
            if (c == ' ' || c == '\n')
                c = static_cast<unsigned char>(*++pszText);
            fPenY += (pFontInfo->m_fSpacingB * m_fFontScale * 100.0f) /
                         static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                             .nHeight) +
                     fUnitY * (pFontInfo->m_aMin[c].y - pFontInfo->m_aMax[c].y);
            continue;
        }

        if (m_lineOffsets.m_count == 0) {
            short nWidth = 0;
            unsigned char cWidth = static_cast<unsigned char>(*pszText);
            for (char* p = m_pText; *p != 0; p++) {
                nWidth = static_cast<short>(nWidth +
                                 static_cast<int>(__builtin_ceilf(K1GlyphAdvance(pFontInfo, m_fFontScale, cWidth))) -
                                 static_cast<int>(__builtin_ceilf(m_fFontScale * pFontInfo->m_fSpacing * 100.0f)));
            }
            m_lineOffsets.Add(nWidth);
        }

        float fPenX =
            static_cast<float>(m_positionX) /
            static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nWidth);
#if !defined(K2_UI_TOUCH) || defined(K2_QOL_CONTROLLER)
        // pc / xbox, and the port's touch: centre or right-align the line on the width the pen
        // below really advances. m_lineOffsets holds WrapStrings' measure, which rounds every glyph
        // up past a quarter of the screen scale, so a centred caption would sit slightly left of
        // centre.
        float fLineWidth = 0.0f;
        for (int i = 0; i < nAbsChars; i++) {
            unsigned char c = static_cast<unsigned char>(pszText[i]);
#ifdef K2_QOL_CONTROLLER
            if (NoRoomForPadGlyph(pszText, i))
                continue;
#endif
            if (!(ASLPlat_IsControllerPlugged() && i < 8) && static_cast<unsigned char>(c - kPadGlyphFirst) < kPadGlyphCount)
                fLineWidth += s_fScale * 0.01125f *
                              static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                                  .nWidth);
            else {
                fLineWidth += K1GlyphAdvance(pFontInfo, m_fFontScale, c);
                if (static_cast<unsigned char>(c - kPadGlyphFirst) < kPadGlyphCount)   // a drawn pad glyph's gap (below)
                    fLineWidth += fGlyphScale * (pFontInfo->m_aMax[c].x - pFontInfo->m_aMin[c].x) * 0.6f;
            }
        }
        if (nChars < 0)
            fLineWidth += fGlyphScale * (pFontInfo->m_aMax[45].x - pFontInfo->m_aMin[45].x);
        if ((m_alignment & kAlignHorizontalMask) == kAlignCenter) {
            fPenX += (static_cast<float>(m_sizeX) - fLineWidth) /
                     static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                                 .nWidth *
                             2.0f);
        } else if ((m_alignment & kAlignHorizontalMask) == kAlignRight) {
            fPenX += (static_cast<float>(m_sizeX) - fLineWidth) /
                     static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nWidth);
        }
#else
        if ((m_alignment & kAlignHorizontalMask) == kAlignCenter) {
            fPenX += static_cast<float>(m_sizeX - m_lineOffsets.m_pData[nLine]) /
                     static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                                 .nWidth *
                             2.0f);
        } else if ((m_alignment & kAlignHorizontalMask) == kAlignRight) {
            fPenX += static_cast<float>(m_sizeX - m_lineOffsets.m_pData[nLine]) /
                     static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nWidth);
        }

#endif
        if (fPenY > static_cast<float>(m_positionY + m_sizeY) /
                        static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                            .nHeight)) {
            nLine = m_lines.m_count;
        } else {
            float fQuadTop = 1.0f - fPenY;

            int nAlpha = static_cast<int>(GetViewportAlpha() * fAlpha * 255.0f);
            unsigned int nColor = static_cast<unsigned int>(static_cast<int>(m_color.x * 255.0f)) |
                                  static_cast<unsigned int>(static_cast<int>(m_color.y * 255.0f)) << 8 |
                                  static_cast<unsigned int>(static_cast<int>(m_color.z * 255.0f)) << 16 |
                                  static_cast<unsigned int>(nAlpha) << 24;
            unsigned int nWhiteColor = kColourWhiteRgb | static_cast<unsigned int>(nAlpha) << 24;

            if (nAbsChars > 0) {
                int nCharIndex = 0;
                for (;;) {
                    unsigned char c = static_cast<unsigned char>(pszText[nCharIndex]);
                    float fAdvance;
#ifdef K2_QOL_CONTROLLER
                    if (NoRoomForPadGlyph(pszText, nCharIndex))
                        fAdvance = 0.0f;
                    else
#endif
                    if (!(ASLPlat_IsControllerPlugged() && nCharIndex < 8) &&
                        static_cast<unsigned char>(c - kPadGlyphFirst) < kPadGlyphCount) {
                        static float s_skipXAmount = s_fScale * 0.01125f;   // the gap a pad glyph leaves
                        fAdvance = s_skipXAmount;
                    } else {
                        float fRight =
                            fPenX + fUnitX * (pFontInfo->m_aMax[c].x - pFontInfo->m_aMin[c].x);
                        float fQuadBottom =
                            1.0f - (fPenY + (m_fFontScale * pFontInfo->m_fBottom * 100.0f) /
                                                static_cast<float>(CAurGUIDraw::cm_nViewportStack
                                                    [CAurGUIDraw::cm_nCurrentViewport]
                                                        .nHeight));

                        float fLeft = fPenX;
                        float fTop = fQuadTop;
                        float fBottom = fQuadBottom;
#if !defined(K2_UI_TOUCH) || defined(K2_QOL_CONTROLLER)
                        // pc / xbox, and the port's touch: a pad glyph a size up from its cell,
                        // centred on the capitals rather than top-aligned in the cell, and a gap
                        // before the text.
                        float fGap = 0.0f;
                        if (static_cast<unsigned char>(c - kPadGlyphFirst) < kPadGlyphCount) {
                            float fW = fRight - fPenX;
                            float fH = fQuadTop - fQuadBottom;
                            float fMid = (fQuadTop + fQuadBottom) * 0.5f - fH * 0.1f;
                            fTop = fMid + fH * 0.65f;
                            fBottom = fMid - fH * 0.65f;
                            fLeft = fPenX - fW * 0.15f;
                            fRight = fLeft + fW * 1.3f;
                            fGap = fW * 0.6f;
                        }
#endif
                        GLRender::SetVertexColor(c > 31 ? nColor : nWhiteColor);
#ifdef K2_HOST_BUILD
                        // Linear filtering at a cell's top or bottom edge blends in the row of
                        // glyphs above or below (a sliver over A, a, 4 at high resolutions): the
                        // corners sit half a texel inside the cell vertically. Horizontally the
                        // cell is the glyph's own width, whose edge texels are its antialiasing.
                        float fU0 = pFontInfo->m_aMin[c].x - 0.0001f;
                        float fU1 = pFontInfo->m_aMax[c].x - 0.0001f;
                        float fV0 = pFontInfo->m_aMin[c].y;
                        float fV1 = pFontInfo->m_aMax[c].y;
                        if (fV0 < fV1) { fV0 += fHalfTexelV; fV1 -= fHalfTexelV; }
                        else { fV0 -= fHalfTexelV; fV1 += fHalfTexelV; }
                        ASLgl::glTexCoord2f(fU0, fV0);
                        ASLgl::glVertex4f(fLeft, fTop, 0.0f, 1.0f);
                        ASLgl::glTexCoord2f(fU0, fV1);
                        ASLgl::glVertex4f(fLeft, fBottom, 0.0f, 1.0f);
                        ASLgl::glTexCoord2f(fU1, fV1);
                        ASLgl::glVertex4f(fRight, fBottom, 0.0f, 1.0f);
                        ASLgl::glTexCoord2f(fU1, fV0);
                        ASLgl::glVertex4f(fRight, fTop, 0.0f, 1.0f);
#else
                        ASLgl::glTexCoord2f(pFontInfo->m_aMin[c].x - 0.0001f,
                                            pFontInfo->m_aMin[c].y - 0.0001f);
                        ASLgl::glVertex4f(fLeft, fTop, 0.0f, 1.0f);
                        ASLgl::glTexCoord2f(pFontInfo->m_aMin[c].x - 0.0001f,
                                            pFontInfo->m_aMax[c].y - 0.0001f);
                        ASLgl::glVertex4f(fLeft, fBottom, 0.0f, 1.0f);
                        ASLgl::glTexCoord2f(pFontInfo->m_aMax[c].x - 0.0001f,
                                            pFontInfo->m_aMax[c].y - 0.0001f);
                        ASLgl::glVertex4f(fRight, fBottom, 0.0f, 1.0f);
                        ASLgl::glTexCoord2f(pFontInfo->m_aMax[c].x - 0.0001f,
                                            pFontInfo->m_aMin[c].y - 0.0001f);
                        ASLgl::glVertex4f(fRight, fTop, 0.0f, 1.0f);
#endif

                        fAdvance = (pFontInfo->m_fSpacing * m_fFontScale * 100.0f) /
                                       static_cast<float>(CAurGUIDraw::cm_nViewportStack
                                           [CAurGUIDraw::cm_nCurrentViewport]
                                               .nWidth) +
                                   fUnitX * (pFontInfo->m_aMax[c].x - pFontInfo->m_aMin[c].x);
#if !defined(K2_UI_TOUCH) || defined(K2_QOL_CONTROLLER)
                        fAdvance += fGap;
#endif
                    }
                    nCharIndex++;
                    fPenX += fAdvance;
                    if (nCharIndex == nAbsChars)
                        break;
                }
                pszText += nCharIndex;
            }

            if (*pszText == '\n' || *pszText == ' ')
                pszText++;

            if (nChars < 0) {
                float fRight =
                    fPenX + fUnitX * (pFontInfo->m_aMax[45].x - pFontInfo->m_aMin[45].x);
                float fQuadBottom =
                    1.0f - (fPenY + (m_fFontScale * pFontInfo->m_fBottom * 100.0f) /
                                        static_cast<float>(CAurGUIDraw::cm_nViewportStack
                                            [CAurGUIDraw::cm_nCurrentViewport]
                                                .nHeight));
                GLRender::SetVertexColor(nColor);
                ASLgl::glTexCoord2f(pFontInfo->m_aMin[45].x - 0.0001f,
                                    pFontInfo->m_aMin[45].y - 0.0001f);
                ASLgl::glVertex4f(fPenX, fQuadTop, 0.0f, 1.0f);
                ASLgl::glTexCoord2f(pFontInfo->m_aMin[45].x - 0.0001f,
                                    pFontInfo->m_aMax[45].y - 0.0001f);
                ASLgl::glVertex4f(fPenX, fQuadBottom, 0.0f, 1.0f);
                ASLgl::glTexCoord2f(pFontInfo->m_aMax[45].x - 0.0001f,
                                    pFontInfo->m_aMax[45].y - 0.0001f);
                ASLgl::glVertex4f(fRight, fQuadBottom, 0.0f, 1.0f);
                ASLgl::glTexCoord2f(pFontInfo->m_aMax[45].x - 0.0001f,
                                    pFontInfo->m_aMin[45].y - 0.0001f);
                ASLgl::glVertex4f(fRight, fQuadTop, 0.0f, 1.0f);
            }

            unsigned char cLast = static_cast<unsigned char>(*pszText);
            fPenY += (pFontInfo->m_fSpacingB * m_fFontScale * 100.0f) /
                         static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                             .nHeight) +
                     fUnitY * (pFontInfo->m_aMin[cLast].y - pFontInfo->m_aMax[cLast].y);
            if (fPenY > 1.0f)
                break;
        }
    }

    ASLgl::glEnd();
    GLRender::Enable(Aurora::kCapLighting);
#else
    static volatile float s_skipXAmount = 0.01125f;   // the gap a gamepad glyph leaves behind

    UpdateFontScale();

    if (CAurGUIStringInternal::GetStringSize() == 0 || noguirender)
        return;

    GLRender::Disable(Aurora::kCapLighting);
    GLRender::cm_bEnablePausePlane = 0;

    if (m_positionX == 0 && m_sizeX == 0)
        return;

    CAurFontInfo* pFontInfo = m_spFontTexture.m_pObject->GetFontInfo();

    char* pszText = m_pText;
    int nLine = 0;
    float fPenY = static_cast<float>(m_positionY) /
                  static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nHeight);

    if ((m_alignment & kAlignVerticalMask) == kAlignMiddle) {
        float fCellHeight = m_fFontScale * pFontInfo->m_fBottom * 100.0f;
        float fHeight =
            static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nHeight);
        float fOffset =
            (static_cast<float>(m_sizeY) - fCellHeight * static_cast<float>(m_lines.m_count)) / (fHeight + fHeight);
        if (fOffset < 0.0f) {
            float fStep = fCellHeight / fHeight;
            do {
                int nChars = m_lines.m_pData[nLine];
                fOffset += fStep;
                pszText += nChars < 0 ? -nChars : nChars;
                if (*pszText == ' ' || *pszText == '\n')
                    pszText++;
                nLine++;
            } while (fOffset < 0.0f);
        }
        fPenY += fOffset;
    } else if ((m_alignment & kAlignVerticalMask) == kAlignBottom) {
        float fCellHeight = m_fFontScale * pFontInfo->m_fBottom * 100.0f;
        float fBlockHeight = fCellHeight * static_cast<float>(m_lines.m_count);
        float fSizeY = static_cast<float>(m_sizeY);
        if (fBlockHeight > fSizeY) {
            do {
                fBlockHeight -= fCellHeight;
                int nChars = m_lines.m_pData[nLine];
                pszText += nChars < 0 ? -nChars : nChars;
                if (*pszText == ' ' || *pszText == '\n')
                    pszText++;
                nLine++;
            } while (fBlockHeight > fSizeY);
        }
        fPenY += (fSizeY - fBlockHeight) /
                 static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nHeight);
    }

    CAurTexture* pBase0 = m_spFontTexture.m_pObject;
    CAurTexture* pBase1 = m_spFontTextureD2.m_pObject;
    CAurTexture* pBase2 = m_spFontTextureD3.m_pObject;
    CAurTexture* apTextures[3];
    apTextures[0] =
        m_spTexturesPS[0].m_pObject ? m_spTexturesPS[0].m_pObject : pBase0;
    apTextures[1] =
        m_spTexturesPS[1].m_pObject ? m_spTexturesPS[1].m_pObject : pBase1;
    apTextures[2] =
        m_spTexturesPS[2].m_pObject ? m_spTexturesPS[2].m_pObject : pBase2;

    float fScale = GetResolutionTextScalar();
#ifdef K2_QOL_IMPROVEMENTS
    // The atlas by the size the glyphs draw at (the scale times the Text Size), and the texel
    // insets of the atlas bound: they were tuned for the 1x / 2x / 3x atlases, not for a scale.
    float fDrawScale = m_fFontScale;
    if (m_spFontTextureD3.m_pObject != 0 && fDrawScale > 2.0f) {
        apTextures[2]->Bind();
        fScale = 3.0f;
    } else if (m_spFontTextureD2.m_pObject != 0 && fDrawScale > 1.0f) {
        apTextures[1]->Bind();
        fScale = 2.0f;
    } else {
        apTextures[0]->Bind();
        fScale = 1.0f;
    }
    // Sampled linearly whatever the atlas's TXI says, since no atlas is drawn texel for pixel (a
    // 32-texel cell is ~19 pixels at 1x).
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   // GL_TEXTURE_MAG_FILTER, GL_LINEAR
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // GL_TEXTURE_MIN_FILTER, GL_LINEAR
#else
    if (m_spFontTextureD3.m_pObject != 0 && fScale > 2.0f)
        apTextures[2]->Bind();
    else if (m_spFontTextureD2.m_pObject != 0 && fScale > 1.0f)
        apTextures[1]->Bind();
    else
        apTextures[0]->Bind();
#endif

    GLRender::BeginImmediatePrimitive(Aurora::kPrimQuads);

    float fGlyphScale = pFontInfo->m_fWidthScale * m_fFontScale * 100.0f;
    float fUnitX =
        fGlyphScale / static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nWidth);
    float fUnitY = fGlyphScale /
                   static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nHeight);
    float fTexelU = fScale * 0.0001f;
    float fTexelV = (fScale - 1.0f) * 0.0009f + 0.0001f;

    for (; nLine < m_lines.m_count; nLine++) {
        while (nLine < m_lines.m_count && m_lines.m_pData[nLine] == 0) {
            unsigned char c = static_cast<unsigned char>(*pszText++);
            fPenY += fUnitY *
                     ((pFontInfo->m_fSpacingB * m_fFontScale * 100.0f) /
                          static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                              .nHeight) +
                      pFontInfo->m_aMin[c].y - pFontInfo->m_aMax[c].y);
            nLine++;
        }
        if (nLine >= m_lines.m_count)
            break;

        unsigned char cFirst = static_cast<unsigned char>(*pszText);
        int nChars = m_lines.m_pData[nLine];
        int nAbsChars = nChars < 0 ? -nChars : nChars;

        if (fPenY + fUnitY *
                        ((pFontInfo->m_fSpacingB * m_fFontScale * 100.0f) /
                             static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                                 .nHeight) +
                         pFontInfo->m_aMin[cFirst].y - pFontInfo->m_aMax[cFirst].y) <
            0.0f) {
            pszText += nChars;
            unsigned char c = static_cast<unsigned char>(*pszText);
            if (c == ' ' || c == '\n')
                c = static_cast<unsigned char>(*++pszText);
            fPenY += fUnitY *
                     ((pFontInfo->m_fSpacingB * m_fFontScale * 100.0f) /
                          static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                              .nHeight) +
                      pFontInfo->m_aMin[c].y - pFontInfo->m_aMax[c].y);
            continue;
        }

        if (m_lineOffsets.m_count == 0) {
            short nWidth = 0;
            unsigned char cWidth = static_cast<unsigned char>(*pszText);
            for (char* p = m_pText; *p != 0; p++) {
                nWidth =
                    static_cast<short>(nWidth +
                            static_cast<int>(((pFontInfo->m_aMax[cWidth].x - pFontInfo->m_aMin[cWidth].x) *
                                       pFontInfo->m_fWidthScale +
                                   pFontInfo->m_fSpacing) *
                                  m_fFontScale * 100.0f));
            }
            nWidth = static_cast<short>(nWidth - static_cast<int>(m_fFontScale * pFontInfo->m_fSpacing * 100.0f));
            m_lineOffsets.Add(nWidth);
        }

        float fPenX =
            static_cast<float>(m_positionX) /
            static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nWidth);
        if ((m_alignment & kAlignHorizontalMask) == kAlignCenter) {
            fPenX += static_cast<float>(m_sizeX - m_lineOffsets.m_pData[nLine]) /
                     static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                                 .nWidth *
                             2.0f);
        } else if ((m_alignment & kAlignHorizontalMask) == kAlignRight) {
            fPenX += static_cast<float>(m_sizeX - m_lineOffsets.m_pData[nLine]) /
                     static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport].nWidth);
        }

        if (fPenY > static_cast<float>(m_positionY + m_sizeY) /
                        static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                            .nHeight)) {
            nLine = m_lines.m_count;
        } else {
            float fQuadTop = 1.0f - fPenY;
            float fQuadTopInset = fQuadTop - 0.002f;

            int nAlpha = static_cast<int>(GetViewportAlpha() * fAlpha * 255.0f);
            unsigned int nColor = static_cast<unsigned int>(static_cast<int>(m_color.x * 255.0f)) |
                                  static_cast<unsigned int>(static_cast<int>(m_color.y * 255.0f)) << 8 |
                                  static_cast<unsigned int>(static_cast<int>(m_color.z * 255.0f)) << 16 |
                                  static_cast<unsigned int>(nAlpha) << 24;
            unsigned int nWhiteColor = kColourWhiteRgb | static_cast<unsigned int>(nAlpha) << 24;

            if (nAbsChars > 0) {
                int nCharIndex = 0;
                for (;;) {
                    unsigned char c = static_cast<unsigned char>(*pszText);
                    float fAdvance;
                    if (!(IsJoystickConnected() && nCharIndex < 8) &&
                        static_cast<unsigned char>(c - kPadGlyphFirst) < kPadGlyphCount) {
                        fAdvance = s_skipXAmount;
                        if (*++pszText == ' ') {
                            pszText++;
                            nCharIndex++;
                        }
                    } else {
                        float fRight =
                            fPenX + fUnitX * (pFontInfo->m_aMax[c].x - pFontInfo->m_aMin[c].x);
                        float fQuadBottom =
                            1.0f - (fPenY + (m_fFontScale * pFontInfo->m_fBottom * 100.0f) /
                                                static_cast<float>(CAurGUIDraw::cm_nViewportStack
                                                    [CAurGUIDraw::cm_nCurrentViewport]
                                                        .nHeight));
                        float fTop = static_cast<unsigned char>(c - kPadGlyphFirst) < kPadGlyphCount ? fQuadTopInset : fQuadTop;
                        float fBottom =
                            static_cast<unsigned char>(c - kPadGlyphFirst) < kPadGlyphCount ? fQuadBottom - 0.002f : fQuadBottom;

                        GLRender::SetVertexColor(c > 31 ? nColor : nWhiteColor);
                        ASLgl::glTexCoord2f(pFontInfo->m_aMin[c].x - fTexelU,
                                            pFontInfo->m_aMin[c].y - fTexelV);
                        ASLgl::glVertex4f(fPenX, fTop, 0.0f, 1.0f);
                        ASLgl::glTexCoord2f(pFontInfo->m_aMin[c].x - fTexelU,
                                            pFontInfo->m_aMax[c].y - fTexelU);
                        ASLgl::glVertex4f(fPenX, fBottom, 0.0f, 1.0f);
                        ASLgl::glTexCoord2f(pFontInfo->m_aMax[c].x - fTexelU,
                                            pFontInfo->m_aMax[c].y - fTexelU);
                        ASLgl::glVertex4f(fRight, fBottom, 0.0f, 1.0f);
                        ASLgl::glTexCoord2f(pFontInfo->m_aMax[c].x - fTexelU,
                                            pFontInfo->m_aMin[c].y - fTexelV);
                        ASLgl::glVertex4f(fRight, fTop, 0.0f, 1.0f);

                        fAdvance =
                            fUnitX *
                            ((pFontInfo->m_fSpacing * m_fFontScale * 100.0f) /
                                 static_cast<float>(CAurGUIDraw::cm_nViewportStack
                                     [CAurGUIDraw::cm_nCurrentViewport]
                                         .nWidth) +
                             pFontInfo->m_aMax[c].x - pFontInfo->m_aMin[c].x);
                        pszText++;
                    }
                    nCharIndex++;
                    fPenX += fAdvance;
                    if (nCharIndex >= nAbsChars)
                        break;
                }
            }

            if (*pszText == '\n' || *pszText == ' ')
                pszText++;

            if (nChars < 0) {
                float fRight =
                    fPenX + fUnitX * (pFontInfo->m_aMax[45].x - pFontInfo->m_aMin[45].x);
                float fQuadBottom =
                    1.0f - (fPenY + (m_fFontScale * pFontInfo->m_fBottom * 100.0f) /
                                        static_cast<float>(CAurGUIDraw::cm_nViewportStack
                                            [CAurGUIDraw::cm_nCurrentViewport]
                                                .nHeight));
                GLRender::SetVertexColor(nColor);
                ASLgl::glTexCoord2f(pFontInfo->m_aMin[45].x - fTexelU,
                                    pFontInfo->m_aMin[45].y - fTexelV);
                ASLgl::glVertex4f(fPenX, fQuadTop, 0.0f, 1.0f);
                ASLgl::glTexCoord2f(pFontInfo->m_aMin[45].x - fTexelU,
                                    pFontInfo->m_aMax[45].y - fTexelU);
                ASLgl::glVertex4f(fPenX, fQuadBottom, 0.0f, 1.0f);
                ASLgl::glTexCoord2f(pFontInfo->m_aMax[45].x - fTexelU,
                                    pFontInfo->m_aMax[45].y - fTexelU);
                ASLgl::glVertex4f(fRight, fQuadBottom, 0.0f, 1.0f);
                ASLgl::glTexCoord2f(pFontInfo->m_aMax[45].x - fTexelU,
                                    pFontInfo->m_aMin[45].y - fTexelV);
                ASLgl::glVertex4f(fRight, fQuadTop, 0.0f, 1.0f);
            }

            unsigned char cLast = static_cast<unsigned char>(*pszText);
            fPenY += fUnitY *
                     ((pFontInfo->m_fSpacingB * m_fFontScale * 100.0f) /
                          static_cast<float>(CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport]
                              .nHeight) +
                      pFontInfo->m_aMin[cLast].y - pFontInfo->m_aMax[cLast].y);
            if (fPenY > 1.0f)
                break;
        }
    }

    ASLgl::glEnd();
    GLRender::Enable(Aurora::kCapLighting);
#endif
}

void CAurGUIStringInternal::SetCursorIndex(short index) {
    m_cursorIndex = index;
}

unsigned short CAurGUIStringInternal::GetCursorIndex() {
    return m_cursorIndex;
}

void CAurGUIStringInternal::DisableCursor() {
    m_cursorIndex = kCursorNone;
}

char* CAurGUIStringInternal::GetFontTexture() {
    return m_spFontTexture.m_pObject ? m_spFontTexture.m_pObject->GetName() : 0;
}

int CAurGUIStringInternal::GetNumLines() {
    return m_lines.m_count;
}

void CAurGUIStringInternal::GetPositionBoundingRect(int& x, int& y) {
    x = m_positionX;
    y = m_positionY;
}

void CAurGUIStringInternal::GetSizeBoundingRect(int& x, int& y) {
    x = m_sizeX;
    y = m_sizeY;
}

void CAurGUIStringInternal::Draw() { Draw(1.0f); }

void CAurGUIStringInternal::SetPositionBoundingRect(int x, int y)
{
    m_positionX = x;
    m_positionY = y;
}

void CAurGUIStringInternal::SetSizeBoundingRect(int x, int y)
{
    m_sizeX = x;
    m_sizeY = y;
}

// The class's key function. Every texture the object holds a reference on is unregistered and
// released, then both layout lists are emptied.
CAurGUIStringInternal::~CAurGUIStringInternal()
{
    CAurTexture* pOld;

    pOld = m_spFontTexture.m_pObject;
    m_spFontTexture = 0;
    AurTextureRelease(pOld);

#if !KOTOR1  // K1: the d2x font is not released
    CAurTexture* pOldD2 = m_spFontTextureD2.m_pObject;
    CAurTexture* pOldD3 = m_spFontTextureD3.m_pObject;
    m_spFontTextureD2 = 0;
    m_spFontTextureD3 = 0;
    AurTextureRelease(pOldD2);
    AurTextureRelease(pOldD3);

    for (int i = 0; i < 3; i++) {
        pOld = m_spTexturesPS[i].m_pObject;
        m_spTexturesPS[i] = 0;
        AurTextureRelease(pOld);
    }
#endif

    m_spFontTexture = 0;

    delete[] m_pText;
    m_pText = 0;

    m_lines.SetSize(0);
    m_lineOffsets.SetSize(0);

    m_positionX = 0;
    m_positionY = 0;
    m_sizeX = 0;
    m_sizeY = 0;
}


// Creates a GUI string with a plain new plus a full initialize; every GUI widget goes through this
// factory.
CAurGUIStringInternal* NewCAurGUIString(char* szText, char* szFontTexture, const Vector& color,
                                        float fAlpha) {
#ifdef K2_QOL_IMPROVEMENTS
    // For the dialog text size, the 4th argument is the string's size factor, used for the glyph
    // size, the wrapping and the ideal height, scaled while a dialog panel builds its strings.
    fAlpha *= k2qol_text_size_factor();
#endif
    return new CAurGUIStringInternal(szText, szFontTexture, color, fAlpha);
}

template class SafePointer<CAurTexture>;

extern bool g_bTextureRefsGotRefreshed;

// Both CAurGUIDraw bodies are defined here so the derived constructors and destructors below see
// them.
CAurGUIDraw::CAurGUIDraw() {
}

CAurGUIDraw::~CAurGUIDraw() {
}

// All three constructors share the same shape: the vptr store, then a new Material() into
// m_pMaterial. The two overloads differ only in what they then tell the material to do, so the
// default one is the other two's prefix.
//
// SafePointer's assignment operator stays out of line, forced by the explicit
// `template class SafePointer<CAurTexture>;` above.

CAurGUIImageInternal::CAurGUIImageInternal() {
    m_pMaterial = new Material();
}

CAurGUIImageInternal::CAurGUIImageInternal(char* szTexture) {
    m_pMaterial = new Material();
    m_pMaterial->InitializeTextures(szTexture, 0, 0, 0);
}

// Binds an existing texture: sprite 0 of the material takes the SafePointer assignment, which
// releases the old reference and acquires the new one, then the material is named after it.
CAurGUIImageInternal::CAurGUIImageInternal(CAurTexture* pTexture) {
    m_pMaterial = new Material();
    m_pMaterial->Textures(0) = pTexture;
    m_pMaterial->Init(pTexture->GetName());
}

void CAurGUIImageInternal::SetTexture(char* szTexture) {
    m_pMaterial->InitializeTextures(szTexture, 0, 0, 0);
}

int CAurGUIImageInternal::GetImageHeight() {
    return m_pMaterial->Textures(0).m_pObject->GetHeight();
}

int CAurGUIImageInternal::GetImageWidth() {
    return m_pMaterial->Textures(0).m_pObject->GetWidth();
}

// Both shorthands draw the whole sprite with a fixed (0,0,w,h,0,0.0f) frame, a (-1,-1,-1) "no
// tinting" colour and alpha 1.0f or a value supplied at the call site.
void CAurGUIImageInternal::Draw() {
    Draw(0, 0, CAurGUIImageInternal::GetImageWidth(), CAurGUIImageInternal::GetImageHeight(),
         0, 0.0f, Vector(-1.0f, -1.0f, -1.0f), 1.0f);
}

void CAurGUIImageInternal::Draw(float fAlpha) {
    Draw(0, 0, CAurGUIImageInternal::GetImageWidth(), CAurGUIImageInternal::GetImageHeight(),
         0, 0.0f, Vector(-1.0f, -1.0f, -1.0f), fAlpha);
}

// When this material is the currently bound GUI texture the pending batch is flushed first.
CAurGUIImageInternal::~CAurGUIImageInternal()
{
    if (CAurGUIImage::cm_nGUITID == m_pMaterial->GetTextureTID()) {
        CAurGUIImage::FlushBuffer(CAurGUIImage::cm_fAlpha);
        CAurGUIImage::cm_nGUITID = 0;
        CAurGUIImage::cm_pMaterial = 0;
    }
    delete m_pMaterial;
}

// Emits whatever the buffered Draw calls queued. The batch is one of two user-pointer quad draws
// picked by cm_bIsVTC; the unbuffered variant additionally tints with the accumulated viewport
// alpha and makes the GUI texture current first.
void CAurGUIImage::FlushBuffer(float fAlpha) {
    if (cm_nGUIBufferSizeUsed >= 1) {
        GLRender::Disable(Aurora::kCapLighting);
        GLRender::cm_bEnablePausePlane = 0;
        AurTextureMakeCurrent();
        if (cm_pMaterial != 0) {
            cm_pMaterial->BindTexture0();
            cm_pMaterial->SetGUIBlendingMode();
        }
        if (cm_bIsVTC) {
            RenderVertexVTC* paVerts = reinterpret_cast<RenderVertexVTC*>(cm_fGUIBuffer);
            GLRender::RenderObjectsVTCQuadUP(paVerts, cm_nGUIBufferSizeUsed / 6);
        } else {
            float fViewportAlpha = GetViewportAlpha() * fAlpha;
            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, fViewportAlpha);
            RenderVertexVT* paVerts = reinterpret_cast<RenderVertexVT*>(cm_fGUIBuffer);
            GLRender::RenderObjectsVTQuadUP(paVerts, cm_nGUIBufferSizeUsed / 5);
        }
        GLRender::Enable(Aurora::kCapLighting);
    }
    cm_nGUIBufferSizeUsed = 0;
}

// The int-coordinate Draw overloads are thin converters, mapping pixel coordinates to NDC through
// the current viewport's scale and then calling the float overload.
extern int noguirender;

// This one also clears GLRender::cm_bEnablePausePlane; DrawBuffered below does not.
void CAurGUIImageInternal::Draw(int nX, int nY, int nWidth, int nHeight, unsigned char a, float f1,
                               const Vector& v, float f2)
{
    if (noguirender)
        return;
    GLRender::cm_bEnablePausePlane = 0;
    short nViewport = CAurGUIDraw::cm_nCurrentViewport;
    const CAurGUIDraw::Viewport& viewport = CAurGUIDraw::cm_nViewportStack[nViewport];
    Draw(static_cast<float>(nX) / static_cast<float>(viewport.nWidth), static_cast<float>(nY) / static_cast<float>(viewport.nHeight),
         static_cast<float>(nWidth) / static_cast<float>(viewport.nWidth), static_cast<float>(nHeight) / static_cast<float>(viewport.nHeight),
         a, f1, v, f2);
}

void CAurGUIImageInternal::DrawBuffered(int nX, int nY, int nWidth, int nHeight, unsigned char a,
                                        float f1, const Vector& v, float f2)
{
    if (noguirender)
        return;
    short nViewport = CAurGUIDraw::cm_nCurrentViewport;
    const CAurGUIDraw::Viewport& viewport = CAurGUIDraw::cm_nViewportStack[nViewport];
    DrawBuffered(static_cast<float>(nX) / static_cast<float>(viewport.nWidth), static_cast<float>(nY) / static_cast<float>(viewport.nHeight),
                 static_cast<float>(nWidth) / static_cast<float>(viewport.nWidth), static_cast<float>(nHeight) / static_cast<float>(viewport.nHeight),
                 a, f1, v, f2);
}

// The float overloads are the real bodies, a sprite-sheet blit. The material's texture is tiled
// GetNumX() x GetNumY() and the drawn rect covers one tile centred in the surface, so the texcoords
// span [0.5/nFrameW, 1 - 0.5/nFrameW] on both axes. `a`'s bits 2-3 pick the corner order (and two
// of those settings add a fixed quarter-turn); any other angle rotates the matrix about the rect's
// centre. The (-1,-1,-1) sentinel Vector, tested with the exported equal(), means "no tinting".
void CAurGUIImageInternal::Draw(float x, float y, float w, float h, unsigned char a, float fRot,
                               const Vector& v, float fAlpha)
{
#if KOTOR1  // K1: nothing is drawn at a non-positive width or a fully transparent alpha
    if (w <= 0.0f)
        return;
    if (GetViewportAlpha() * fAlpha <= 0.0f)
        return;
#endif
    if (noguirender)
        return;

#if KOTOR1  // K1 still queries the tile size but draws the whole texture
    static_cast<void>(m_pMaterial->Textures(0).m_pObject->GetWidth());
    static_cast<void>(m_pMaterial->Textures(0).m_pObject->GetNumX());
    static_cast<void>(m_pMaterial->Textures(0).m_pObject->GetHeight());
    static_cast<void>(m_pMaterial->Textures(0).m_pObject->GetNumY());
#else
    int nFrameWidth = m_pMaterial->Textures(0).m_pObject->GetWidth()
                    / m_pMaterial->Textures(0).m_pObject->GetNumX();
    int nFrameHeight = m_pMaterial->Textures(0).m_pObject->GetHeight()
                     / m_pMaterial->Textures(0).m_pObject->GetNumY();
#endif

    AurTextureMakeCurrent();
    m_pMaterial->BindTexture0();

    uint64_t nCorner0, nCorner1, nCorner2, nCorner3;
    if ((a & kImageFlipMask) == kImageFlipV) {
        nCorner0 = 1;
        nCorner1 = 0;
        nCorner2 = 3;
        nCorner3 = 2;
    } else if ((a & kImageFlipMask) == kImageFlipH) {
        nCorner0 = 3;
        nCorner1 = 2;
        nCorner2 = 1;
        nCorner3 = 0;
    } else {
        nCorner0 = 0;
        nCorner1 = 1;
        nCorner2 = 2;
        nCorner3 = 3;
    }

    float fAngle = (fRot < 0.0f) ? (fRot + 360.0f) : fRot;

    ASLgl::glPushMatrix();

    unsigned char anUV[4];
    if (fAngle == 0.0f) {
        anUV[0] = static_cast<unsigned char>(nCorner0);
        anUV[1] = static_cast<unsigned char>(nCorner1);
        anUV[2] = static_cast<unsigned char>(nCorner2);
        anUV[3] = static_cast<unsigned char>(nCorner3);
    } else if (fAngle == 90.0f) {
        anUV[nCorner0] = 1;
        anUV[nCorner1] = 2;
        anUV[nCorner2] = 3;
        anUV[nCorner3] = 0;
    } else if (fAngle == 180.0f) {
        anUV[nCorner0] = 2;
        anUV[nCorner1] = 3;
        anUV[nCorner2] = 0;
        anUV[nCorner3] = 1;
    } else if (fAngle == 270.0f) {
        anUV[nCorner0] = 3;
        anUV[nCorner1] = 0;
        anUV[nCorner2] = 1;
        anUV[nCorner3] = 2;
    } else {
        float fCenterX = -x - w * 0.5f;
        float fCenterY = w * 0.5f - (1.0f - y);
        anUV[0] = static_cast<unsigned char>(nCorner0);
        anUV[1] = static_cast<unsigned char>(nCorner1);
        anUV[2] = static_cast<unsigned char>(nCorner2);
        anUV[3] = static_cast<unsigned char>(nCorner3);
        ASLgl::glLoadIdentity();
        ASLgl::glTranslatef(-fCenterX, -fCenterY, 0.0f);
        ASLgl::glRotatef(fAngle, 0.0f, 0.0f, 1.0f);
#if KOTOR1  // K1: an off-axis rotation is stretched toward the viewport's aspect ratio
        float fTurn = static_cast<float>(static_cast<int>(fAngle) % 180) / 90.0f;
        fTurn += fTurn > 1.0f ? 1.0f - fTurn : 0.0f;
        const CAurGUIDraw::Viewport& viewport =
            CAurGUIDraw::cm_nViewportStack[CAurGUIDraw::cm_nCurrentViewport];
        int nWidth = viewport.nWidth;
        int nHeight = viewport.nHeight;
        float fAspectX, fAspectY;
        if (nWidth > nHeight) {
            fAspectY = static_cast<float>(nHeight) / static_cast<float>(nWidth);
            fAspectX = 1.0f / fAspectY;
        } else if (nHeight > nWidth) {
            fAspectX = static_cast<float>(nWidth) / static_cast<float>(nHeight);
            fAspectY = 1.0f / fAspectX;
        } else {
            fAspectY = 1.0f;
            fAspectX = 1.0f;
        }
        ASLgl::glScalef((1.0f - fTurn) + fTurn * fAspectX, (1.0f - fTurn) + fTurn * fAspectY, 1.0f);
#else
        ASLgl::glScalef(1.0f, 1.0f, 1.0f);
#endif
        ASLgl::glTranslatef(fCenterX, fCenterY, 0.0f);
    }

#if !KOTOR1
    float fU = 0.5f / static_cast<float>(nFrameWidth);
    float fV = 0.5f / static_cast<float>(nFrameHeight);
#endif

    GLRender::Disable(Aurora::kCapLighting);
    m_pMaterial->SetGUIBlendingMode();

    float fUV[4][2];
#if KOTOR1  // K1: the whole texture, no half-texel inset
    fUV[anUV[0]][0] = 0.0f;
    fUV[anUV[0]][1] = 1.0f;
    fUV[anUV[1]][0] = 0.0f;
    fUV[anUV[1]][1] = -0.0f;
    fUV[anUV[2]][0] = 1.0f;
    fUV[anUV[2]][1] = -0.0f;
    fUV[anUV[3]][0] = 1.0f;
    fUV[anUV[3]][1] = 1.0f;
#else
    fUV[anUV[0]][0] = fU;
    fUV[anUV[0]][1] = 1.0f - fV;
    fUV[anUV[1]][0] = fU;
    fUV[anUV[1]][1] = fV;
    fUV[anUV[2]][0] = 1.0f - fU;
    fUV[anUV[2]][1] = fV;
    fUV[anUV[3]][0] = 1.0f - fU;
    fUV[anUV[3]][1] = 1.0f - fV;
#endif

    GLRender::BeginImmediatePrimitive(Aurora::kPrimQuads);

    if (equal(v, Vector(-1.0f, -1.0f, -1.0f))) {
        float fViewportAlpha = GetViewportAlpha() * fAlpha;
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, fViewportAlpha);
        // inlined GLRender::SetVertexColor(unsigned long)
        unsigned char anColor[4];
        *reinterpret_cast<unsigned int*>(anColor) =
            kColourWhiteRgb | (static_cast<unsigned int>(GetViewportAlpha() * fAlpha * 255.0f) << 24);
        ASLgl::glColor4ubv(anColor);
    } else {
        int nR = static_cast<int>(v.x * 255.0f);
        int nG = static_cast<int>(v.y * 255.0f);
        int nB = static_cast<int>(v.z * 255.0f);
        int nA = static_cast<int>(GetViewportAlpha() * fAlpha * 255.0f);
        unsigned char anColor[4];
        *reinterpret_cast<unsigned int*>(anColor) = static_cast<unsigned int>(nR | (nG << 8) | (nB << 16) | (nA << 24));
        ASLgl::glColor4ubv(anColor);
    }
    ASLgl::glTexCoord2f(fUV[0][0], fUV[0][1]);
    ASLgl::glVertex4f(x, 1.0f - y, 0.0f, 1.0f);
    ASLgl::glTexCoord2f(fUV[1][0], fUV[1][1]);
    ASLgl::glVertex4f(x, 1.0f - (y + h), 0.0f, 1.0f);
    ASLgl::glTexCoord2f(fUV[2][0], fUV[2][1]);
    ASLgl::glVertex4f(x + w, 1.0f - (y + h), 0.0f, 1.0f);
    ASLgl::glTexCoord2f(fUV[3][0], fUV[3][1]);
    ASLgl::glVertex4f(x + w, 1.0f - y, 0.0f, 1.0f);

    ASLgl::glEnd();
    ASLgl::glPopMatrix();
    GLRender::Enable(Aurora::kCapLighting);
}

// Appends four vertices to the shared deferred GUI bank (CAurGUIImage::cm_fGUIBuffer) instead of
// issuing GL calls, as the buffered twin of Draw, flushing the batch first whenever any of the
// state the pending vertices were built with has changed. The bank holds 6 floats per vertex when
// the draw is tinted and 5 when it is not.
//
// The rotation is quantized rather than turned into a matrix. The quarter-turn index is
// (int)((fAngle + 45) / 90) and the corner permutation is a four-case switch. The switch has no
// default, so a quarter index of 4 (any fAngle at or past 315 degrees) leaves the UVs unwritten.
// K2_FIX_ENGINE_BUGS wraps the index into 0..3.
void CAurGUIImageInternal::DrawBuffered(float x, float y, float w, float h, unsigned char a,
                                        float fRot, const Vector& v, float fAlpha)
{
    if (noguirender)
        return;

    // The same (-1,-1,-1) sentinel Draw uses, here also stored as the batch's tint flag.
    int bTint = (v.x != -1.0f) || (v.y != -1.0f) || (v.z != -1.0f);

    int nTextureTID = m_pMaterial->GetTextureTID();
    if (nTextureTID != CAurGUIImage::cm_nGUITID
        || bTint != static_cast<int>(CAurGUIImage::cm_bIsVTC)
        || CAurGUIImage::cm_fAlpha != fAlpha
        || (bTint && CAurGUIImage::cm_nGUIBufferSizeUsed > 2023)
        || (!bTint && CAurGUIImage::cm_nGUIBufferSizeUsed > 2027)
#if !KOTOR1  // K1: a texture-reference refresh does not force a flush
        || g_bTextureRefsGotRefreshed
#endif
        ) {
        CAurGUIImage::FlushBuffer(CAurGUIImage::cm_fAlpha);
        CAurGUIImage::cm_nGUITID = nTextureTID;
        CAurGUIImage::cm_bIsVTC = bTint;
        CAurGUIImage::cm_fAlpha = fAlpha;
#if !KOTOR1
        g_bTextureRefsGotRefreshed = false;
#endif
        CAurGUIImage::cm_pMaterial = m_pMaterial;
    }

    uint64_t nIdxA, nIdxB, nIdxC, nIdxD;
    if ((a & kImageFlipMask) == kImageFlipV) {
        nIdxA = 2;
        nIdxB = 3;
        nIdxC = 0;
        nIdxD = 1;
    } else if ((a & kImageFlipMask) == kImageFlipH) {
        nIdxA = 0;
        nIdxB = 1;
        nIdxC = 2;
        nIdxD = 3;
    } else {
        nIdxA = 3;
        nIdxB = 2;
        nIdxC = 1;
        nIdxD = 0;
    }

    float fAngle = (fRot < 0.0f) ? (fRot + 360.0f) : fRot;

    unsigned char anUV[4] K2_UNSET;
    int nQuarter = static_cast<int>((fAngle + 45.0f) / 90.0f);
#ifdef K2_FIX_ENGINE_BUGS
    nQuarter &= 3;
#endif
    switch (nQuarter) {
    case 0:
        anUV[0] = static_cast<unsigned char>(nIdxD);
        anUV[1] = static_cast<unsigned char>(nIdxC);
        anUV[2] = static_cast<unsigned char>(nIdxB);
        anUV[3] = static_cast<unsigned char>(nIdxA);
        break;
    case 1:
        anUV[nIdxD] = 1;
        anUV[nIdxC] = 2;
        anUV[nIdxB] = 3;
        anUV[nIdxA] = 0;
        break;
    case 2:
        anUV[nIdxD] = 2;
        anUV[nIdxC] = 3;
        anUV[nIdxB] = 0;
        anUV[nIdxA] = 1;
        break;
    case 3:
        anUV[nIdxD] = 3;
        anUV[nIdxC] = 0;
        anUV[nIdxB] = 1;
        anUV[nIdxA] = 2;
        break;
    }

    float fBottom = 1.0f - y;
    float fTop = 1.0f - (y + h);
    float fRight = x + w;


    if (bTint) {
        int nUsed = CAurGUIImage::cm_nGUIBufferSizeUsed;
        unsigned int nColor = static_cast<unsigned int>(static_cast<int>(v.x * 255.0f) | (static_cast<int>(v.y * 255.0f) << 8)
                                             | (static_cast<int>(v.z * 255.0f) << 16)
                                             | (static_cast<int>(GetViewportAlpha() * fAlpha * 255.0f) << 24));
        CAurGUIImage::cm_fGUIBuffer[nUsed + 0] = x;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 1] = fBottom;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 2] = 0.0f;
        *reinterpret_cast<unsigned int*>(&CAurGUIImage::cm_fGUIBuffer[nUsed + 3]) = nColor;
        int64_t nUV0 = anUV[0] * 6 + nUsed;
        *reinterpret_cast<uint64_t*>(&CAurGUIImage::cm_fGUIBuffer[nUV0 + 4]) = kUvPair01;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 6] = x;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 7] = fTop;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 8] = 0.0f;
        *reinterpret_cast<unsigned int*>(&CAurGUIImage::cm_fGUIBuffer[nUsed + 9]) = nColor;
        int64_t nUV1 = anUV[1] * 6 + nUsed;
        *reinterpret_cast<uint64_t*>(&CAurGUIImage::cm_fGUIBuffer[nUV1 + 4]) = kUvPair00;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 12] = fRight;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 13] = fTop;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 14] = 0.0f;
        *reinterpret_cast<unsigned int*>(&CAurGUIImage::cm_fGUIBuffer[nUsed + 15]) = nColor;
        int64_t nUV2 = anUV[2] * 6 + nUsed;
        *reinterpret_cast<uint64_t*>(&CAurGUIImage::cm_fGUIBuffer[nUV2 + 4]) = kUvPair10;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 18] = fRight;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 19] = fBottom;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 20] = 0.0f;
        *reinterpret_cast<unsigned int*>(&CAurGUIImage::cm_fGUIBuffer[nUsed + 21]) = nColor;
        int64_t nUV3 = anUV[3] * 6 + nUsed;
        *reinterpret_cast<uint64_t*>(&CAurGUIImage::cm_fGUIBuffer[nUV3 + 4]) = kUvPair11;
        CAurGUIImage::cm_nGUIBufferSizeUsed = nUsed + 24;
    } else {
        int nUsed = CAurGUIImage::cm_nGUIBufferSizeUsed;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 0] = x;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 1] = fBottom;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 2] = 0.0f;
        int64_t nUV0 = anUV[0] * 5 + nUsed;
        *reinterpret_cast<uint64_t*>(&CAurGUIImage::cm_fGUIBuffer[nUV0 + 3]) = kUvPair01;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 5] = x;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 6] = fTop;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 7] = 0.0f;
        int64_t nUV1 = anUV[1] * 5 + nUsed;
        *reinterpret_cast<uint64_t*>(&CAurGUIImage::cm_fGUIBuffer[nUV1 + 3]) = kUvPair00;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 10] = fRight;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 11] = fTop;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 12] = 0.0f;
        int64_t nUV2 = anUV[2] * 5 + nUsed;
        *reinterpret_cast<uint64_t*>(&CAurGUIImage::cm_fGUIBuffer[nUV2 + 3]) = kUvPair10;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 15] = fRight;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 16] = fBottom;
        CAurGUIImage::cm_fGUIBuffer[nUsed + 17] = 0.0f;
        int64_t nUV3 = anUV[3] * 5 + nUsed;
        *reinterpret_cast<uint64_t*>(&CAurGUIImage::cm_fGUIBuffer[nUV3 + 3]) = kUvPair11;
        CAurGUIImage::cm_nGUIBufferSizeUsed = nUsed + 20;
    }
}

// Creates an image widget through the texture overload when there is a name to bind, the default
// constructor otherwise; the image widgets all go through this factory.
CAurGUIImageInternal* NewCAurGUIImage(char* szTexture) {
    if (szTexture != 0)
        return new CAurGUIImageInternal(szTexture);
    return new CAurGUIImageInternal();
}
