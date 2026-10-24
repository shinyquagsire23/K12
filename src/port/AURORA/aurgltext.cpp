#include "k2stdint.h"
#include "port/aslglapi.h"
#include "aurglcore_ios.h"

// FTGL's render-mode word for every face of the glyphs (FTGL::RENDER_ALL).
enum { kFTRenderAll = 0xffff };

// Minimal mirror of the font library's texture font, naming only the slots the text helpers
// dispatch.
struct FTPoint
{
    double x;
    double y;
    double z;
};

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

extern FTTextureFont* textureFont;
#if !KOTOR1
FTTextureFont* textureFont;
#endif

static FTTextureFont* s_textureFonts[256];


#include "port/libcbase.h"
#include "port/libcstring.h"

// The face is sized to (nSize, 72 dpi).
void SetupTruetypeFont(int nSize)
{
    if (s_textureFonts[nSize] == 0)
    {
        FTTextureFont* pFont = new FTTextureFont("override/iosdialog.otf");
        textureFont = pFont;
        s_textureFonts[nSize] = pFont;
        pFont->FaceSize(static_cast<unsigned int>(nSize), 72);
    }
    textureFont = s_textureFonts[nSize];
}

// Re-assert the face size, which makes the font library re-rasterise its glyph cache.
void ResetFace()
{
    if (textureFont == 0)
        return;
    textureFont->FaceSize(textureFont->FaceSize(), 72);
}

// Prints a GL error, ignoring a zero one, so it can be called unconditionally; codes outside the
// six known GL enums print a generic message. Two of the messages spell "overflow"/"underflow"
// with a U+FB02 ligature FL, which is what the string data holds.
void checkGlError(unsigned int nError, const char* pszFile, unsigned int nLine)
{
    const char* pszMessage;
    switch (nError)
    {
    case GL_INVALID_ENUM: pszMessage = "GL_INVALID_ENUM, enum argument out of range."; break;
    case GL_INVALID_VALUE: pszMessage = "GL_INVALID_VALUE, numeric argument out of range"; break;
    case GL_INVALID_OPERATION: pszMessage = "GL_INVALID_OPERATION, operation illegal in current state"; break;
    case GL_STACK_OVERFLOW: pszMessage = "GL_STACK_OVERFLOW, command would cause a stack over\xEF\xAC\x82" "ow"; break;
    case GL_STACK_UNDERFLOW: pszMessage = "GL_STACK_UNDERFLOW, command would cause a stack under\xEF\xAC\x82" "ow"; break;
    case GL_OUT_OF_MEMORY: pszMessage = "GL_OUT_OF_MEMORY, not enough memory left to execute command"; break;
    default:
        if (nError == 0)
            return;
        printf("%s:%d: %d (%s)\n", pszFile, static_cast<int>(nLine), static_cast<int>(nError), "Unknown GL error");
        return;
    }
    printf("%s:%d: %d (%s)\n", pszFile, static_cast<int>(nLine), static_cast<int>(nError), pszMessage);
}

static char s_szUtf8Buffer[1024];

// UTF-8 lead bytes by sequence length, the continuation-byte marker and its 6 payload bits.
enum
{
    kUtf8Continuation = 0x80,
    kUtf8PayloadMask = 0x3f,
    kUtf8Lead2 = 0xc0,
    kUtf8Lead3 = 0xe0,
    kUtf8Lead4 = 0xf0,
    kUtf8Lead5 = 0xf8,
    kUtf8Lead6 = 0xfc,
};

// The CP1252 bytes 0x80-0x9f that differ from Latin-1, as offsets from 0x80 (the switch index).
enum
{
    kCp1252HighHalf = 0x80,
    kCp1252Euro = 0x80 - kCp1252HighHalf,
    kCp1252Ellipsis = 0x85 - kCp1252HighHalf,
    kCp1252SCaron = 0x8a - kCp1252HighHalf,
    kCp1252OELigature = 0x8c - kCp1252HighHalf,
    kCp1252ZCaron = 0x8e - kCp1252HighHalf,
    kCp1252LeftSingleQuote = 0x91 - kCp1252HighHalf,
    kCp1252RightSingleQuote = 0x92 - kCp1252HighHalf,
    kCp1252LeftDoubleQuote = 0x93 - kCp1252HighHalf,
    kCp1252RightDoubleQuote = 0x94 - kCp1252HighHalf,
    kCp1252Bullet = 0x95 - kCp1252HighHalf,
    kCp1252EnDash = 0x96 - kCp1252HighHalf,
    kCp1252EmDash = 0x97 - kCp1252HighHalf,
    kCp1252TradeMark = 0x99 - kCp1252HighHalf,
    kCp1252SCaronSmall = 0x9a - kCp1252HighHalf,
    kCp1252OELigatureSmall = 0x9c - kCp1252HighHalf,
    kCp1252ZCaronSmall = 0x9e - kCp1252HighHalf,
    kCp1252YDiaeresis = 0x9f - kCp1252HighHalf,
};

// The Unicode code points those bytes stand for.
enum
{
    kUnicodeSCaron = 0x0160,
    kUnicodeSCaronSmall = 0x0161,
    kUnicodeOELigature = 0x0152,
    kUnicodeOELigatureSmall = 0x0153,
    kUnicodeYDiaeresis = 0x0178,
    kUnicodeZCaron = 0x017d,
    kUnicodeZCaronSmall = 0x017e,
    kUnicodeLeftSingleQuote = 0x2018,
    kUnicodeRightSingleQuote = 0x2019,
    kUnicodeLeftDoubleQuote = 0x201c,
    kUnicodeRightDoubleQuote = 0x201d,
    kUnicodeBullet = 0x2022,
    kUnicodeEllipsis = 0x2026,
    kUnicodeEuro = 0x20ac,
    kUnicodeTradeMark = 0x2122,
};

// The bytes of a code point's UTF-8 encoding: the lead byte of a 2- or 3-byte sequence, and the
// continuation byte carrying the 6 bits at nShift.
static constexpr char Utf8Lead2(unsigned int nCodePoint)
{
    return static_cast<char>(kUtf8Lead2 | (nCodePoint >> 6));
}
static constexpr char Utf8Lead3(unsigned int nCodePoint)
{
    return static_cast<char>(kUtf8Lead3 | (nCodePoint >> 12));
}
static constexpr char Utf8Trail(unsigned int nCodePoint, int nShift)
{
    return static_cast<char>(kUtf8Continuation | ((nCodePoint >> nShift) & kUtf8PayloadMask));
}

// Transcode one run of nLen bytes from CP1252 to UTF-8. When bIsUtf8 is set the text is already
// UTF-8, so the input pointer is returned untouched and nLen is ignored.
//
// Only the CP1252 characters the bundled font needs are mapped; every other byte in the upper
// half of the CP1252 range is emitted as if the input were Latin-1, and the en/em dashes become a
// plain '-'.
char* ConvertUtf8(bool bIsUtf8, const char* pszText, int nLen)
{
    if (bIsUtf8)
        return const_cast<char*>(pszText);

    char* pDst = s_szUtf8Buffer;
    if (nLen < 1)
        return pDst;

    int nOut = 0;
    int nRest = nLen;
    do
    {
        char c = *pszText;

        if (c >= 0)
        {
            pDst[nOut] = c;
            nOut += 1;
        }
        else
        {
            unsigned char u = static_cast<unsigned char>(c);
            char b0 = static_cast<char>(kUtf8Lead2 | ((u >> 6) & 3));
            char b1 = static_cast<char>(kUtf8Continuation | (u & kUtf8PayloadMask));
            char b2 = 0;
            bool bSingle = false;
            int nIndex = static_cast<int>(u) - kCp1252HighHalf;

            switch (nIndex)
            {
            case kCp1252Euro: b0 = Utf8Lead3(kUnicodeEuro); b1 = Utf8Trail(kUnicodeEuro, 6); b2 = Utf8Trail(kUnicodeEuro, 0); break;
            case kCp1252Ellipsis: b0 = Utf8Lead3(kUnicodeEllipsis); b1 = Utf8Trail(kUnicodeEllipsis, 6); b2 = Utf8Trail(kUnicodeEllipsis, 0); break;
            case kCp1252SCaron: b0 = Utf8Lead2(kUnicodeSCaron); b1 = Utf8Trail(kUnicodeSCaron, 0); break;
            case kCp1252OELigature: b0 = Utf8Lead2(kUnicodeOELigature); b1 = Utf8Trail(kUnicodeOELigature, 0); break;
            case kCp1252ZCaron: b0 = Utf8Lead2(kUnicodeZCaron); b1 = Utf8Trail(kUnicodeZCaron, 0); break;
            case kCp1252LeftSingleQuote: b0 = Utf8Lead3(kUnicodeLeftSingleQuote); b1 = Utf8Trail(kUnicodeLeftSingleQuote, 6); b2 = Utf8Trail(kUnicodeLeftSingleQuote, 0); break;
            case kCp1252RightSingleQuote: b0 = Utf8Lead3(kUnicodeRightSingleQuote); b1 = Utf8Trail(kUnicodeRightSingleQuote, 6); b2 = Utf8Trail(kUnicodeRightSingleQuote, 0); break;
            case kCp1252LeftDoubleQuote: b0 = Utf8Lead3(kUnicodeLeftDoubleQuote); b1 = Utf8Trail(kUnicodeLeftDoubleQuote, 6); b2 = Utf8Trail(kUnicodeLeftDoubleQuote, 0); break;
            case kCp1252RightDoubleQuote: b0 = Utf8Lead3(kUnicodeRightDoubleQuote); b1 = Utf8Trail(kUnicodeRightDoubleQuote, 6); b2 = Utf8Trail(kUnicodeRightDoubleQuote, 0); break;
            case kCp1252Bullet: b0 = Utf8Lead3(kUnicodeBullet); b1 = Utf8Trail(kUnicodeBullet, 6); b2 = Utf8Trail(kUnicodeBullet, 0); break;
            case kCp1252EnDash:
            case kCp1252EmDash: b0 = static_cast<char>('-'); bSingle = true; break;
            case kCp1252TradeMark: b0 = Utf8Lead3(kUnicodeTradeMark); b1 = Utf8Trail(kUnicodeTradeMark, 6); b2 = Utf8Trail(kUnicodeTradeMark, 0); break;
            case kCp1252SCaronSmall: b0 = Utf8Lead2(kUnicodeSCaronSmall); b1 = Utf8Trail(kUnicodeSCaronSmall, 0); break;
            case kCp1252OELigatureSmall: b0 = Utf8Lead2(kUnicodeOELigatureSmall); b1 = Utf8Trail(kUnicodeOELigatureSmall, 0); break;
            case kCp1252ZCaronSmall: b0 = Utf8Lead2(kUnicodeZCaronSmall); b1 = Utf8Trail(kUnicodeZCaronSmall, 0); break;
            case kCp1252YDiaeresis: b0 = Utf8Lead2(kUnicodeYDiaeresis); b1 = Utf8Trail(kUnicodeYDiaeresis, 0); break;
            default: break;
            }

            if (bSingle)
            {
                pDst[nOut] = b0;
                nOut += 1;
            }
            else
            {
                pDst[nOut] = b0;
                pDst[nOut + 1] = b1;
                if (b2 != 0)
                {
                    pDst[nOut + 2] = b2;
                    nOut += 3;
                }
                else
                {
                    nOut += 2;
                }
            }
        }

        pszText++;
    } while (--nRest != 0);
    return pDst;
}

// Draws multi-line text: a greedy word wrap into a 256-entry break table, then either a recursion
// at a smaller font size or the GL draw. The per-line length handed to Advance/Render
// is a character count when the text is already UTF-8 and a byte count otherwise.
void DrawTextWrapped(const char* pszText, bool bUtf8, int nSize, int nW, int nH,
                     int nX, int nY, int nMaxWidth, int nMaxHeight, int nMaxLines, bool bClamp)
{
    if (nSize < 4)
        return;

    FTTextureFont* pFont = s_textureFonts[nSize];
    if (pFont == 0)
    {
        pFont = new FTTextureFont("override/iosdialog.otf");
        textureFont = pFont;
        s_textureFonts[nSize] = pFont;
        pFont->FaceSize(static_cast<unsigned int>(nSize), 72);
        pFont = s_textureFonts[nSize];
    }
    textureFont = pFont;

    int aBreaks[256];
    memset(aBreaks, 0, sizeof(aBreaks));

    float fMaxWidth = static_cast<float>(nMaxWidth);
    int64_t nIndex = 0;
    int nLineStart = 0;
    int nNextStart = 0;
    int nBreaks = 1;

    for (;;)
    {
        char c = pszText[nIndex];
        if (c == '\n' || c == ' ')
            nNextStart = static_cast<int>(nIndex) + 1;
        else if (c == 0)
            break;

        float fAdvance = textureFont->Advance(
            ConvertUtf8(bUtf8, pszText + nLineStart, static_cast<int>(nIndex) - nLineStart),
            static_cast<int>(nIndex) - nLineStart, FTPoint());

        if (fMaxWidth < fAdvance || pszText[nIndex] == '\n')
        {
            aBreaks[nBreaks] = nNextStart;
            nBreaks++;
            if (static_cast<unsigned int>(nBreaks) >= 255)
                break;
            nIndex++;
            nLineStart = nNextStart;
        }
        else
        {
            nIndex++;
        }
    }

    aBreaks[nBreaks] = static_cast<int>(nIndex);

    int nLineHeight = static_cast<int>(textureFont->FaceSize());
    if (nBreaks > nMaxLines || static_cast<float>(nLineHeight * nBreaks) > static_cast<float>(nMaxHeight))
    {
        DrawTextWrapped(pszText, bUtf8, nSize - 2, nW, nH, nX, nY, nMaxWidth, nMaxHeight, nMaxLines,
                        bClamp);
        return;
    }

    ASLgl::glMatrixMode(GL_PROJECTION);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(nW), 0.0, static_cast<double>(nH), -1.0, 1.0);
    ASLgl::glMatrixMode(GL_MODELVIEW);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glColor4f(0.0f, 0.7f, 1.0f, 1.0f);

    if (nBreaks >= 1)
    {
        float fBaseline = static_cast<float>(nLineHeight * nBreaks) * 0.5f
                        + static_cast<float>((nH - nY) - nMaxHeight / 2)
                        - static_cast<float>(nLineHeight) + 4.0f;
        float fLineStep = static_cast<float>(-nLineHeight);
        int nStart = aBreaks[0];

        uint64_t i = 0;
        do
        {
            i++;
            int nEnd = aBreaks[i];
            int nLen = nEnd - nStart;

            int nChars = nLen;
            if (bUtf8)
            {
                const char* pszLine = pszText + nStart;
                nChars = 0;
                int k = 0;
                while (k < nLen)
                {
                    unsigned char c2 = pszLine[k];
                    nChars++;
                    int nExtra;
                    if (c2 >= kUtf8Lead6)
                        nExtra = 5;
                    else if (c2 >= kUtf8Lead5)
                        nExtra = 4;
                    else if (c2 >= kUtf8Lead4)
                        nExtra = 3;
                    else if (c2 >= kUtf8Lead3)
                        nExtra = 2;
                    else
                        nExtra = (c2 >= kUtf8Lead2) ? 1 : 0;
                    k += nExtra + 1;
                }
            }

            float fLineWidth = textureFont->Advance(
                ConvertUtf8(bUtf8, pszText + nStart, nChars), nChars, FTPoint());
            float fX = 0.0f;
            if (bClamp)
                fX = (static_cast<float>(nMaxWidth) - fLineWidth) * 0.5f;

            textureFont->Render(ConvertUtf8(bUtf8, pszText + nStart, nChars), nChars,
                                FTPoint{fX + static_cast<float>(nX), fBaseline, 0.0}, FTPoint(), kFTRenderAll);

            fBaseline += fLineStep;
            nStart = nEnd;
        } while (i != static_cast<uint64_t>(nBreaks));
    }

    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glMatrixMode(GL_PROJECTION);
    ASLgl::glPopMatrix();
    ASLgl::glMatrixMode(GL_MODELVIEW);
    ASLgl::glPopMatrix();
}
