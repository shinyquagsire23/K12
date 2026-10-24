// FTGL's FTTextureFont for the host, on FreeType.
//
// The Android build links FTGL 2.x and FreeType statically. The engine uses one class of it, for
// the mobile dialogue and subtitle text (AURORA::DrawTextWrapped), which creates a FTTextureFont
// per size and calls FaceSize, Advance and Render. This file is that class, built against the
// engine's own model of it -- the same declaration, the same object size and vtable order -- with
// the FTGL internals replaced by a small FreeType glyph cache.
//
// Drawing follows Aspyr's FTGL-ES build, whose entry points are tail calls into ASLgl:
//   PreRender  -- save GL_BLEND and the blend func, enable GL_BLEND and GL_TEXTURE_2D, blend
//                 (SRC_ALPHA, ONE_MINUS_SRC_ALPHA), re-issue the current colour, glBegin(GL_QUADS)
//   glyphs     -- one quad each from a GL_LUMINANCE_ALPHA atlas (luminance 255, alpha = coverage),
//                 CLAMP_TO_EDGE and GL_LINEAR, a glEnd/glBegin pair whenever the atlas page changes
//   PostRender -- glEnd, restore GL_BLEND and the blend func.
// FTGL positions are y-up: a glyph spans (x + left, y + top - h) .. (x + left + w, y + top).
// Strings are UTF-8 and `len` counts characters (-1 = to the NUL), as in FTGL 2.1.

#include <stdint.h>
#include "k2stdint.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

struct FTPoint
{
    double x;
    double y;
    double z;
};

// Must stay identical to the declaration in src/port/AURORA/aurgltext.cpp: one class, two TUs.
class FTTextureFont
{
public:
    FTTextureFont(const char* pszFontFile);

    virtual ~FTTextureFont();
    virtual bool Attach(const char* pszFontFile);
    virtual bool Attach(const char* pszFont, uint64_t nLen);
    virtual int  GlyphLoadFlags(int nFlags);
    virtual bool CharMap(int nEncoding);
    virtual unsigned int CharMapCount() const;
    virtual const int* CharMapList();
    virtual void FaceSize(unsigned int nSize, unsigned int nRes);
    virtual unsigned int FaceSize() const;
    virtual void Depth(float fDepth);
    virtual void Outset(float fOutset);
    virtual void Outset(float fOutsetX, float fOutsetY);
    virtual void UseDisplayList(bool bUseList);
    virtual float Ascender() const;
    virtual float Descender() const;
    virtual float LineHeight() const;
    virtual void BBox(const char* pszText, int nLen, FTPoint vLL, FTPoint vUR);
    virtual void BBox(const K2WChar* pszText, int nLen, FTPoint vLL, FTPoint vUR);
    virtual float Advance(const char* pszText, int nLen, FTPoint vPos);
    virtual float Advance(const K2WChar* pszText, int nLen, FTPoint vPos);
    virtual void Render(const char* pszText, int nLen, FTPoint vPos, FTPoint vSpacing,
                        int nRenderMode);

private:
    void* m_pImpl;
};

// Pulls in the game's GL layer, which is KOTOR II's ASLgl (the entry points Aspyr's FTGL-ES
// tail-calls) or KOTOR 1's own k1gl forwarders.
#include "port/aslglapi.h"

namespace {

const int kPage = 512;   // atlas page edge, in texels

struct Glyph { unsigned int page; int left, top, w, h; float advance; float u0, v0, u1, v1; };

struct Impl
{
    FT_Face face = 0;
    std::vector<unsigned char> file;   // FT_New_Memory_Face keeps pointing into it
    unsigned int size = 0, res = 72;
    std::map<unsigned int, Glyph> glyphs;
    std::vector<unsigned int> pages;
    int penX = 0, penY = 0, rowH = 0;  // packing cursor on the last page
    bool blendWasOn = false;
    int blendSrc = GL_SRC_ALPHA, blendDst = GL_ONE_MINUS_SRC_ALPHA;
};

FT_Library Library()
{
    static FT_Library s_lib = 0;
    if (!s_lib && FT_Init_FreeType(&s_lib) != 0) s_lib = 0;
    return s_lib;
}

// Decodes one UTF-8 code point; a stray byte decodes as itself (Latin-1).
unsigned int NextChar(const unsigned char*& p)
{
    unsigned int c = *p++;
    int extra = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : c >= 0xc0 ? 1 : 0;
    unsigned int cp = extra ? c & (0x3f >> extra) : c;
    for (int i = 0; i < extra; i++) {
        if ((*p & 0xc0) != 0x80) return c;
        cp = cp << 6 | (*p++ & 0x3f);
    }
    return cp;
}

unsigned int NewPage(Impl* m)
{
    std::vector<unsigned char> blank(kPage * kPage * 2, 0);
    for (size_t i = 0; i < blank.size(); i += 2) blank[i] = 0xff;
    unsigned int tex = 0;
    int prev = 0;
    ASLgl::glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
    ASLgl::glGenTextures(1, &tex);
    ASLgl::glBindTexture(GL_TEXTURE_2D, tex);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glPixelStorei(0x0cf5, 1);   // GL_UNPACK_ALIGNMENT
    ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, kPage, kPage, 0, GL_LUMINANCE_ALPHA,
                        GL_UNSIGNED_BYTE, blank.data());
    ASLgl::glBindTexture(GL_TEXTURE_2D, static_cast<unsigned int>(prev));
    m->pages.push_back(tex);
    m->penX = m->penY = m->rowH = 0;
    return tex;
}

const Glyph* GetGlyph(Impl* m, unsigned int cp, bool bUpload)
{
    std::map<unsigned int, Glyph>::iterator it = m->glyphs.find(cp);
    if (it != m->glyphs.end() && (!bUpload || it->second.page != 0 || it->second.w == 0)) return &it->second;
    // A control character -- the CR of a CRLF line end in the touch UI's computer screens -- has no
    // glyph in the font: FreeType would draw glyph 0, the missing-glyph box. Nothing, and no advance.
    if (cp < 0x20) return &(m->glyphs[cp] = Glyph());
    if (!m->face || FT_Load_Char(m->face, cp, FT_LOAD_RENDER) != 0) return 0;
    FT_GlyphSlot s = m->face->glyph;
    Glyph g = Glyph();
    g.left = s->bitmap_left; g.top = s->bitmap_top;
    g.w = static_cast<int>(s->bitmap.width); g.h = static_cast<int>(s->bitmap.rows);
    g.advance = s->advance.x / 64.0f;
    if (bUpload && g.w > 0 && g.h > 0 && g.w + 2 <= kPage && g.h + 2 <= kPage) {
        if (m->pages.empty()) NewPage(m);
        if (m->penX + g.w + 1 > kPage) { m->penX = 0; m->penY += m->rowH + 1; m->rowH = 0; }
        if (m->penY + g.h + 1 > kPage) NewPage(m);
        std::vector<unsigned char> la(static_cast<size_t>(g.w) * g.h * 2);
        for (int y = 0; y < g.h; y++)
            for (int x = 0; x < g.w; x++) {
                la[(y * g.w + x) * 2] = 0xff;
                la[(y * g.w + x) * 2 + 1] = s->bitmap.buffer[y * s->bitmap.pitch + x];
            }
        int prev = 0;
        ASLgl::glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
        g.page = m->pages.back();
        ASLgl::glBindTexture(GL_TEXTURE_2D, g.page);
        ASLgl::glPixelStorei(0x0cf5, 1);
        ASLgl::glTexSubImage2D(GL_TEXTURE_2D, 0, m->penX, m->penY, g.w, g.h, GL_LUMINANCE_ALPHA,
                               GL_UNSIGNED_BYTE, la.data());
        ASLgl::glBindTexture(GL_TEXTURE_2D, static_cast<unsigned int>(prev));
        g.u0 = static_cast<float>(m->penX) / kPage; g.v0 = static_cast<float>(m->penY) / kPage;
        g.u1 = static_cast<float>(m->penX + g.w) / kPage; g.v1 = static_cast<float>(m->penY + g.h) / kPage;
        m->penX += g.w + 1;
        if (g.h > m->rowH) m->rowH = g.h;
    }
    return &(m->glyphs[cp] = g);
}

float Kerning(Impl* m, unsigned int a, unsigned int b)
{
    if (!m->face || !a || !FT_HAS_KERNING(m->face)) return 0;
    FT_Vector k;
    if (FT_Get_Kerning(m->face, FT_Get_Char_Index(m->face, a), FT_Get_Char_Index(m->face, b),
                       FT_KERNING_DEFAULT, &k) != 0) return 0;
    return k.x / 64.0f;
}

void DropGlyphs(Impl* m)
{
    if (!m->pages.empty()) ASLgl::glDeleteTextures(static_cast<int>(m->pages.size()), m->pages.data());
    m->pages.clear();
    m->glyphs.clear();
}

} // namespace

FTTextureFont::FTTextureFont(const char* pszFontFile) : m_pImpl(new Impl())
{
    Attach(pszFontFile);
}

FTTextureFont::~FTTextureFont()
{
    Impl* m = static_cast<Impl*>(m_pImpl);
    DropGlyphs(m);
    if (m->face) FT_Done_Face(m->face);
    delete m;
}

// Reads through the shim's fopen, so the case-blind game path resolves like every other file.
bool FTTextureFont::Attach(const char* pszFontFile)
{
    FILE* f = pszFontFile ? fopen(pszFontFile, "rb") : 0;
    if (!f) { fprintf(stderr, "FTTextureFont: %s: not found\n", pszFontFile ? pszFontFile : "(null)"); return false; }
    std::vector<unsigned char> data;
    unsigned char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) data.insert(data.end(), buf, buf + n);
    fclose(f);
    return Attach(reinterpret_cast<const char*>(data.data()), data.size());
}

bool FTTextureFont::Attach(const char* pszFont, uint64_t nLen)
{
    Impl* m = static_cast<Impl*>(m_pImpl);
    if (!Library()) return false;
    DropGlyphs(m);
    if (m->face) { FT_Done_Face(m->face); m->face = 0; }
    // FT_New_Memory_Face reads the buffer for the face's whole life: keep a private copy.
    std::vector<unsigned char> copy(pszFont, pszFont + nLen);
    if (FT_New_Memory_Face(Library(), copy.data(), static_cast<FT_Long>(copy.size()), 0, &m->face) != 0) { m->face = 0; return false; }
    m->file.swap(copy);
    FT_Select_Charmap(m->face, FT_ENCODING_UNICODE);
    return true;
}

int FTTextureFont::GlyphLoadFlags(int nFlags) { return nFlags; }
bool FTTextureFont::CharMap(int) { return true; }
unsigned int FTTextureFont::CharMapCount() const { return 1; }
const int* FTTextureFont::CharMapList() { return 0; }

void FTTextureFont::FaceSize(unsigned int nSize, unsigned int nRes)
{
    Impl* m = static_cast<Impl*>(m_pImpl);
    if (!m->face) return;
    DropGlyphs(m);
    m->size = nSize; m->res = nRes;
    FT_Set_Char_Size(m->face, 0, static_cast<FT_F26Dot6>(nSize) * 64, nRes, nRes);
}

unsigned int FTTextureFont::FaceSize() const { return static_cast<Impl*>(m_pImpl)->size; }
void FTTextureFont::Depth(float) {}
void FTTextureFont::Outset(float) {}
void FTTextureFont::Outset(float, float) {}
void FTTextureFont::UseDisplayList(bool) {}

float FTTextureFont::Ascender() const
{
    Impl* m = static_cast<Impl*>(m_pImpl);
    return m->face && m->face->size ? m->face->size->metrics.ascender / 64.0f : 0;
}
float FTTextureFont::Descender() const
{
    Impl* m = static_cast<Impl*>(m_pImpl);
    return m->face && m->face->size ? m->face->size->metrics.descender / 64.0f : 0;
}
float FTTextureFont::LineHeight() const
{
    Impl* m = static_cast<Impl*>(m_pImpl);
    return m->face && m->face->size ? m->face->size->metrics.height / 64.0f : 0;
}

// The engine's model declares BBox as returning void (FTGL returns an FTBBox); nothing calls it.
void FTTextureFont::BBox(const char*, int, FTPoint, FTPoint) {}
void FTTextureFont::BBox(const K2WChar*, int, FTPoint, FTPoint) {}

float FTTextureFont::Advance(const char* pszText, int nLen, FTPoint vSpacing)
{
    Impl* m = static_cast<Impl*>(m_pImpl);
    float x = 0;
    unsigned int prev = 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(pszText);
    for (int i = 0; p && *p && (nLen < 0 || i < nLen); i++) {
        unsigned int cp = NextChar(p);
        const Glyph* g = GetGlyph(m, cp, false);
        x += Kerning(m, prev, cp) + (g ? g->advance : 0);
        if (*p && (nLen < 0 || i + 1 < nLen)) x += static_cast<float>(vSpacing.x);
        prev = cp;
    }
    return x;
}

float FTTextureFont::Advance(const K2WChar* pszText, int nLen, FTPoint vSpacing)
{
    Impl* m = static_cast<Impl*>(m_pImpl);
    float x = 0;
    unsigned int prev = 0;
    for (int i = 0; pszText && pszText[i] && (nLen < 0 || i < nLen); i++) {
        const Glyph* g = GetGlyph(m, static_cast<unsigned int>(pszText[i]), false);
        x += Kerning(m, prev, static_cast<unsigned int>(pszText[i])) + (g ? g->advance : 0) + static_cast<float>(vSpacing.x);
        prev = static_cast<unsigned int>(pszText[i]);
    }
    return x;
}

void FTTextureFont::Render(const char* pszText, int nLen, FTPoint vPos, FTPoint vSpacing, int)
{
    Impl* m = static_cast<Impl*>(m_pImpl);
    if (!m->face || !pszText || getenv("K2_FTGL_OFF")) return;

    // Glyphs are uploaded before glBegin: texture uploads cannot sit inside a begin/end pair.
    std::vector<unsigned int> cps;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(pszText);
    for (int i = 0; *p && (nLen < 0 || i < nLen); i++) {
        cps.push_back(NextChar(p));
        GetGlyph(m, cps.back(), true);
    }

    // PreRender.
    m->blendWasOn = ASLgl::glIsEnabled(GL_BLEND) != 0;
    ASLgl::glGetIntegerv(GL_BLEND_SRC, &m->blendSrc);
    ASLgl::glGetIntegerv(GL_BLEND_DST, &m->blendDst);
    ASLgl::glEnable(GL_BLEND);
    bool texWasOn = ASLgl::glIsEnabled(GL_TEXTURE_2D) != 0;
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Aspyr's FTGL-ES draws through a GLES2 program, where fixed-function lighting cannot apply. The
    // host's quads go down the real fixed-function path, and the 3D pass leaves GL_LIGHTING on: the
    // lit colour replaced glColor and the text came out dim grey.
    bool litWasOn = ASLgl::glIsEnabled(GL_LIGHTING) != 0;
    ASLgl::glDisable(GL_LIGHTING);
    float c[4];
    ASLgl::glGetFloatv(GL_CURRENT_COLOR, c);
    int prevTex = 0;
    ASLgl::glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);

    unsigned int bound = 0;
    bool open = false;
    float x = static_cast<float>(vPos.x), y = static_cast<float>(vPos.y);
    unsigned int prev = 0;
    for (size_t i = 0; i < cps.size(); i++) {
        const Glyph* g = GetGlyph(m, cps[i], true);
        x += Kerning(m, prev, cps[i]);
        if (g && g->w > 0 && g->page) {
            if (g->page != bound) {
                if (open) ASLgl::glEnd();
                ASLgl::glBindTexture(GL_TEXTURE_2D, g->page);
                bound = g->page;
                ASLgl::glBegin(GL_QUADS);
                open = true;
            }
            float x0 = x + g->left, y1 = y + g->top, x1 = x0 + g->w, y0 = y1 - g->h;
            ASLgl::glColor4f(c[0], c[1], c[2], c[3]); ASLgl::glTexCoord2f(g->u0, g->v0); ASLgl::glVertex2f(x0, y1);
            ASLgl::glColor4f(c[0], c[1], c[2], c[3]); ASLgl::glTexCoord2f(g->u0, g->v1); ASLgl::glVertex2f(x0, y0);
            ASLgl::glColor4f(c[0], c[1], c[2], c[3]); ASLgl::glTexCoord2f(g->u1, g->v1); ASLgl::glVertex2f(x1, y0);
            ASLgl::glColor4f(c[0], c[1], c[2], c[3]); ASLgl::glTexCoord2f(g->u1, g->v0); ASLgl::glVertex2f(x1, y1);
        }
        x += (g ? g->advance : 0) + static_cast<float>(vSpacing.x);
        prev = cps[i];
    }

    // PostRender.
    if (open) ASLgl::glEnd();
    ASLgl::glBindTexture(GL_TEXTURE_2D, static_cast<unsigned int>(prevTex));
    if (!m->blendWasOn) ASLgl::glDisable(GL_BLEND);
    else ASLgl::glBlendFunc(static_cast<unsigned int>(m->blendSrc), static_cast<unsigned int>(m->blendDst));
    if (!texWasOn) ASLgl::glDisable(GL_TEXTURE_2D);
    if (litWasOn) ASLgl::glEnable(GL_LIGHTING);
}
