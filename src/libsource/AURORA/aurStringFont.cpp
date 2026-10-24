#include "k2stdint.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library: List.h grows from 8
#endif
#include "aurStringFont.h"

#include "aurtexman.h"
#include "aurglcore.h"
#include "aurfont.h"
#include "aurstring.h"

#include "port/libcstring.h"

extern CAurTexture* AurTextureGetReference(char* sName, char* sExtension);
void AurTextureRelease(CAurTexture* pTexture);

List<CAurFont*> CAurFont::s_LoadedFontList;

// One corner of a glyph quad in TextOutA's vertex pool: position, packed colour, texture coordinate.
struct CAurFontVertex {
    float x;
    float y;
    float z;
    unsigned int color;
    float u;
    float v;
};

// Vertices the shared font pool holds: six per character, grown by TextOutA and never shrunk.
static int64_t g_nFontPoolVerts = 100;

// Pen X the glyph quads advance, and the parse cursor a nested measurement pass shares with its
// caller.
static float g_fPenX;
static int g_nParseIndex;

void AurDeleteAllFonts()
{
    // Back to front: ~CAurFont removes its own slot from the list being walked.
    for (int64_t i = CAurFont::s_LoadedFontList.m_count; i > 0; i--)
        delete CAurFont::s_LoadedFontList.m_pData[i - 1];
}

CAurFont::CAurFont(char* pszName)
{
    m_nPasswordFontIndex = 0;
    m_nRefCount = 0;
    m_pTexture = AurTextureGetReference(pszName, 0);
    m_pFontInfo = m_pTexture.m_pObject->GetFontInfo();
    strncpy(m_szName, pszName, 16);
    m_szName[15] = '\0';
#if KOTOR1  // K1 does not test the font info for null; a texture without one faults here
    if (m_pFontInfo->m_nCharCount != 0) {
        m_nIndex = s_LoadedFontList.m_count;
        s_LoadedFontList.Add(this);
    } else {
        m_nIndex = -1;
    }
#else
    if (m_pFontInfo != 0) {
        if (m_pFontInfo->m_nCharCount != 0) {
            m_nIndex = s_LoadedFontList.m_count;
            s_LoadedFontList.Add(this);
        } else {
            m_nIndex = -1;
        }
    }
#endif
}

CAurFont::CAurFont(int nFont, char chChar)
{
    m_nRefCount = 0;

    CAurFont* pFont = s_LoadedFontList.m_pData[nFont];
    m_pFontInfo = pFont->m_pFontInfo;
    m_pTexture = pFont->m_pTexture.m_pObject;

    // Writes through the shared CAurFontInfo just aliased, so the source font's glyphs are
    // remapped too. Only u and v are copied, not the whole Vector.
    for (int i = 32; i < 128; i++) {
        m_pFontInfo->m_aMin[i].x = pFont->m_pFontInfo->m_aMin[chChar].x;
        m_pFontInfo->m_aMin[i].y = pFont->m_pFontInfo->m_aMin[chChar].y;
        m_pFontInfo->m_aMax[i].x = pFont->m_pFontInfo->m_aMax[chChar].x;
        m_pFontInfo->m_aMax[i].y = pFont->m_pFontInfo->m_aMax[chChar].y;
    }

    m_nIndex = s_LoadedFontList.m_count;
    s_LoadedFontList.Add(this);
}

CAurFont::~CAurFont()
{
    CAurTexture* pTexture = m_pTexture.m_pObject;
    m_pTexture = 0;
    AurTextureRelease(pTexture);
    m_pTexture = 0;

    if (m_nIndex >= 0) {
        for (int i = m_nIndex + 1; i < s_LoadedFontList.m_count; i++)
            s_LoadedFontList.m_pData[i]->m_nIndex--;

        s_LoadedFontList.DelIndex(m_nIndex);
    }
}

// Recognises one markup tag: `<fN>` selects s_LoadedFontList[N], `<cRGB>` sets a colour,
// `</f>` returns to the enclosing font (via a null *ppFont), `</c>` restores g_cSavedCol. Returns
// the width of the tag in characters, or 0 if this is not a tag.
int CAurFont::ProcessTextMarkup(char* pszText, CAurFont** ppFont, CStringColor& cColor)
{
    cColor = g_cSavedCol;
    *ppFont = this;
    switch (pszText[1]) {
    case '<':
        return 1;
    case 'c':
        cColor.r = static_cast<unsigned char>(pszText[2]) / 255.0f;
        cColor.g = static_cast<unsigned char>(pszText[3]) / 255.0f;
        cColor.b = static_cast<unsigned char>(pszText[4]) / 255.0f;
        cColor.a = 1.0f;
        return 6;
    case 'f':
        *ppFont = s_LoadedFontList.m_pData[static_cast<unsigned char>(pszText[2]) - '0'];
        return 4;
    case '/':
        switch (pszText[2]) {
        case 'f':
            *ppFont = 0;
            return 4;
        case 'c':
            cColor = g_cSavedCol;
            return 4;
        }
        return 0;
    }
    return 0;
}

// Draws `nLength` characters at the current pen, six vertices per glyph into one shared GL pool.
// A `<f>` tag naming a different font flushes the batch and recurses into that font, whose return
// value is how much of the tail it consumed; `</f>` (a null font) ends this level and returns the
// index reached, which is where the outer text resumes. Returns nLength otherwise.
#if KOTOR1
int CAurFont::TextOut(char* pszText, int nLength)
#else
int CAurFont::TextOutA(char* pszText, int nLength)
#endif
{
    static unsigned int nPool = GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0),
                                                      static_cast<unsigned int>(g_nFontPoolVerts * 24), 0x42, 0x1011, &nPool);

    if (g_nFontPoolVerts < nLength * 6) {
        GLRender::ResizePool(nPool, nLength * 6 * 24);
        g_nFontPoolVerts = nLength * 6;
    }

    float fY0 = m_pFontInfo->m_fBottom;
    float fY1 = m_pFontInfo->m_fTop;
    CStringColor cColor = g_cSavedCol;

    CAurFontVertex* pVerts = static_cast<CAurFontVertex*>(GLRender::LockPool(nPool, 0, 0, 1));

    fY0 = fY1 - fY0;

    int64_t nVerts = 0;
    unsigned int nColor = 0;
    bool bColorDirty = true;
    for (int i = 0; i < nLength; i++) {
        char* p = &pszText[i];
        unsigned char c = *p;
        if (c == '<') {
            CAurFont* pFont = this;
            int nSkip = 0;

            cColor = g_cSavedCol;
            switch (p[1]) {
            case 'f':
                pFont = s_LoadedFontList.m_pData[static_cast<unsigned char>(p[2]) - '0'];
                nSkip = 4;
                break;
            case 'c':
                cColor.r = static_cast<unsigned char>(p[2]) / 255.0f;
                cColor.g = static_cast<unsigned char>(p[3]) / 255.0f;
                cColor.b = static_cast<unsigned char>(p[4]) / 255.0f;
                cColor.a = 1.0f;
                nSkip = 6;
                break;
            case '/':
                switch (p[2]) {
                case 'c':
                    nSkip = 4;
                    break;
                case 'f':
                    pFont = 0;
                    nSkip = 4;
                    break;
                }
                break;
            }
            bColorDirty = true;

            if (nSkip != 0) {
                int nNext = i + nSkip;
                if (pFont != this) {
                    GLRender::UnlockPool(nPool);
                    GLRender::RenderObjectsVTC(static_cast<Aurora::AuroraPrimitiveTypes>(3), nPool, static_cast<int>(nVerts));
                    if (pFont == 0)
                        return nNext;
                    pVerts = static_cast<CAurFontVertex*>(GLRender::LockPool(nPool, 0, 0, 1));
                    pFont->m_pTexture.m_pObject->Bind();
                    nNext += pFont->TextOutA(&pszText[nNext], nLength - nNext);
                    pVerts = static_cast<CAurFontVertex*>(GLRender::LockPool(nPool, 0, 0, 1));
                    m_pTexture.m_pObject->Bind();
                    nVerts = 0;
                }
                i = nNext - 1;
                continue;
            }
        }

        CAurFontInfo* pInfo = m_pFontInfo;
        unsigned char g = c;
        if (g >= pInfo->m_aMax.m_count)
            g = static_cast<unsigned char>(pInfo->m_aMax.m_count - 1);
        if (g >= pInfo->m_aMin.m_count)
            g = static_cast<unsigned char>(pInfo->m_aMin.m_count - 1);

        float fX0 = g_fPenX + pInfo->m_fSpacing;
        float fX1 = fX0 + (pInfo->m_aMax[g].x - pInfo->m_aMin[g].x) * pInfo->m_fWidthScale;
        g_fPenX = fX1;

        if (bColorDirty)
            nColor = static_cast<unsigned int>(static_cast<int>(cColor.r * 255.0f))
                   | (static_cast<unsigned int>(static_cast<int>(cColor.g * 255.0f)) << 8)
                   | (static_cast<unsigned int>(static_cast<int>(cColor.b * 255.0f)) << 16)
                   | (static_cast<unsigned int>(static_cast<int>(cColor.a * 255.0f)) << 24);
        bColorDirty = false;

        CAurFontVertex* pQuad = &pVerts[nVerts];
        pQuad[0].color = nColor;
        pQuad[1].color = nColor;
        pQuad[2].color = nColor;
        pQuad[3].color = nColor;
        pQuad[4].color = nColor;
        pQuad[5].color = nColor;

        pQuad[0].x = fX0; pQuad[0].y = fY0; pQuad[0].z = 0.0f;
        pQuad[0].u = pInfo->m_aMin[g].x; pQuad[0].v = pInfo->m_aMax[g].y;
        pQuad[1].x = fX1; pQuad[1].y = fY0; pQuad[1].z = 0.0f;
        pQuad[1].u = pInfo->m_aMax[g].x; pQuad[1].v = pInfo->m_aMax[g].y;
        pQuad[2].x = fX0; pQuad[2].y = fY1; pQuad[2].z = 0.0f;
        pQuad[2].u = pInfo->m_aMin[g].x; pQuad[2].v = pInfo->m_aMin[g].y;
        pQuad[3].x = fX0; pQuad[3].y = fY1; pQuad[3].z = 0.0f;
        pQuad[3].u = pInfo->m_aMin[g].x; pQuad[3].v = pInfo->m_aMin[g].y;
        pQuad[4].x = fX1; pQuad[4].y = fY0; pQuad[4].z = 0.0f;
        pQuad[4].u = pInfo->m_aMax[g].x; pQuad[4].v = pInfo->m_aMax[g].y;
        pQuad[5].x = fX1; pQuad[5].y = fY1; pQuad[5].z = 0.0f;
        pQuad[5].u = pInfo->m_aMax[g].x; pQuad[5].v = pInfo->m_aMin[g].y;
        nVerts += 6;
    }

    GLRender::UnlockPool(nPool);
    GLRender::RenderObjectsVTC(static_cast<Aurora::AuroraPrimitiveTypes>(3), nPool, static_cast<int>(nVerts));
    g_fPenX = 0.0f;
    return nLength;
}

// Measuring twin of TextOutA: how much of pItem's string fits inside fMaxWidth and how wide that
// prefix is. Understands the same markup (recursing into an `<f>` font, through the shared
// g_nParseIndex cursor); with bWrap set it backs the answer up to the last space instead of
// cutting mid-word.
void CAurFont::CalculateVisibleStringLengthAndWidth(CStringItem* pItem, float fMaxWidth, bool bWrap)
{
    CAurFontInfo* pInfo = m_pFontInfo;

    if (pItem->m_fTop < pInfo->m_fTop)
        pItem->m_fTop = pInfo->m_fTop;
    if (pItem->m_fBottom - pItem->m_fTop < pInfo->m_fBottom - pInfo->m_fTop)
        pItem->m_fBottom = pItem->m_fTop + pInfo->m_fBottom - pInfo->m_fTop;

    char* pszText = pItem->m_pszText;
    int nLength = static_cast<int>(strlen(pszText));
    pItem->m_fVisibleWidth = 0.0f;

    // A zero clip width means "no limit"; otherwise allow a hair of slack so a glyph that lands
    // exactly on the boundary still counts as fitting.
    float fLimit = fMaxWidth == 0.0f ? 1000000.0f : fMaxWidth + 1.0000001e-5f;

    int nLastSpace = 0;
    float fWidth = 0.0f;
    for (; g_nParseIndex < nLength; g_nParseIndex++) {
        char* p = &pszText[g_nParseIndex];
        unsigned char c = *p;
        if (c == ' ')
            nLastSpace = g_nParseIndex;
        else if (c == '<') {
            CAurFont* pFont = this;
            int nSkip = 0;
            switch (p[1]) {
            case 'f':
                pFont = s_LoadedFontList.m_pData[static_cast<unsigned char>(p[2]) - '0'];
                nSkip = 4;
                break;
            case 'c':
                nSkip = 6;
                break;
            case '/':
                switch (p[2]) {
                case 'c':
                    nSkip = 4;
                    break;
                case 'f':
                    pFont = 0;
                    nSkip = 4;
                    break;
                }
                break;
            }
            if (nSkip != 0) {
                g_nParseIndex += nSkip;
                if (pFont != this) {
                    if (pFont == 0)
                        return;
                    // The nested font is handed the already-adjusted limit, not the outer fMaxWidth, so
                    // every `<f>` level adds another epsilon of slack.
                    pFont->CalculateVisibleStringLengthAndWidth(pItem, fLimit, bWrap);
                }
                g_nParseIndex--;
                continue;
            }
        }

        float fAdvance = fWidth + (m_pFontInfo->m_aMax[c].x - m_pFontInfo->m_aMin[c].x) * m_pFontInfo->m_fWidthScale;
        if (c == '\n' || fAdvance > fLimit) {
            pItem->m_nVisibleLength = g_nParseIndex;
            if (nLastSpace != 0 && bWrap && fAdvance > fLimit) {
                // Un-count everything since the last space so the line breaks on a word.
                char* pBack = &pszText[nLastSpace];
                for (int64_t j = nLastSpace; j < g_nParseIndex; j++) {
                    unsigned char g = *pBack++;
                    if (g >= m_pFontInfo->m_aMax.m_count)
                        g = static_cast<unsigned char>(m_pFontInfo->m_aMax.m_count - 1);
                    if (g >= m_pFontInfo->m_aMin.m_count)
                        g = static_cast<unsigned char>(m_pFontInfo->m_aMin.m_count - 1);
                    pItem->m_fVisibleWidth -= m_pFontInfo->m_fWidthScale
                                            * (m_pFontInfo->m_aMax[g].x - m_pFontInfo->m_aMin[g].x);
                }
                pItem->m_nVisibleLength = nLastSpace;
            }
            g_nParseIndex = 0;
            return;
        }
        pItem->m_fVisibleWidth = fAdvance;
        fWidth = fAdvance + m_pFontInfo->m_fSpacing;
    }

    pItem->m_nVisibleLength = nLength;
    g_nParseIndex = 0;
}

// Snaps the caret to the character fX falls inside, recording the advance accumulated so far.
void CAurFont::UpdateCaret(CCaret* pCaret, char* pszText, float fX, bool bStopAtNewline)
{
    int nLength = static_cast<int>(strlen(pszText));
    float fWidth = 0.0f;
    for (int i = 0; i < nLength; i++) {
        unsigned char c = pszText[i];
        float fAdvance = (m_pFontInfo->m_aMax[c].x - m_pFontInfo->m_aMin[c].x)
                       * m_pFontInfo->m_fWidthScale;
        // Stop at the caret's own index, past the glyph's midpoint, or at a newline when not
        // stopping at newlines.
        if (fWidth + fAdvance * 0.5f > fX || i == pCaret->nColumn
                || (!bStopAtNewline && c == '\n')) {
            pCaret->nColumn = i;
            pCaret->m_fOffset = fWidth;
            return;
        }
        fWidth += fAdvance + m_pFontInfo->m_fSpacing;
    }
    pCaret->nColumn = nLength;
    pCaret->m_fOffset = fWidth;
}
