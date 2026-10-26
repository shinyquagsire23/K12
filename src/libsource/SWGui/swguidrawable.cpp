// CSWGuiText and CSWGuiTextParams share this translation unit: CSWGuiText::WrapString sits
// between CSWGuiTextParams::SetFont and SetFontRename.

#include "k2stdint.h"
#include <stddef.h>
#include "swguidrawable.h"
#include "aurgui.h"
#include "swgui.h"
#include "exolocstring.h"
#include "exolocstringinternal.h"
#include "exotlktable.h"
#include "resgff.h"
#include "Vector.h"
#include "exostring.h"
#include "aurcamera.h"
#include "AURORA/aurmodel.h"
#include "aurscene.h"
#include "exoresman.h"
#include "CExoArrayListImpl.h"

extern CSWGuiManager* g_pGuiMan;

// Design-unit width multiplier the GUI applies to every layout width.
float GetResolutionTextScalar();

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/crtapi.h"
#include "port/k2qolapi.h"
#include "strref.h"
#include "exostringinline.h"

CSWGuiTextParams::CSWGuiTextParams()
{
    m_nAlignment = 9;
    m_defaultColor.x = 1.0f;
    m_defaultColor.y = 1.0f;
    m_defaultColor.z = 1.0f;
    m_nStrRef = STRREF_NONE;
    m_color = m_defaultColor;
    m_fAlpha = 1.0f;
    m_pTextObject = NULL;
    bHilight = false;

    CResRef resRefFont("dialogfont16x16");
    resRefFont.CopyToString(m_szBaseFont);
    SetFontRename();
}

void CSWGuiTextParams::SetBaseFont(const CResRef& resRef)
{
    resRef.CopyToString(m_szBaseFont);
    SetFontRename();
}

CSWGuiTextParams::CSWGuiTextParams(const CExoString& sText, const CResRef& resRefFont,
                                   const Vector& color, int nAlignment, float fAlpha)
{
    m_pTextObject = NULL;
    m_sText = sText;
    m_defaultColor = color;
    SetBaseFont(resRefFont);
    m_color = m_defaultColor;
    m_nStrRef = STRREF_NONE;
    m_fAlpha = fAlpha;
    m_nAlignment = nAlignment;
    bHilight = false;
}

CSWGuiTextParams::CSWGuiTextParams(uint64_t nStrRef, const CResRef& resRefFont,
                                   const Vector& color, int nAlignment, float fAlpha)
{
    m_pTextObject = NULL;
    SetStrRef(nStrRef);
    SetBaseFont(resRefFont);
    m_defaultColor = color;
    m_color = color;
    m_fAlpha = fAlpha;
    m_nAlignment = nAlignment;
    bHilight = false;
}

void CSWGuiTextParams::SetStrRef(uint64_t nStrRef)
{
    m_nStrRef = nStrRef;
    if (nStrRef != STRREF_NONE)
    {
        STR_RES strRes;
        g_pTlkTable->Fetch(static_cast<unsigned int>(m_nStrRef), strRes, CTlkTable::GENDER_MALE);
        m_sText = strRes.text;
        if (m_pTextObject && m_pTextObject->m_pString)
        {
            m_pTextObject->m_pString->SetString(static_cast<char*>(m_sText.CStr()));
            m_pTextObject->WrapString();
        }
    }
}

CSWGuiTextParams::CSWGuiTextParams(const CSWGuiTextParams& other)
{
    m_pTextObject = NULL;
    m_sText = other.m_sText;
    m_nStrRef = other.m_nStrRef;
    __strcpy_chk(m_szBaseFont, other.m_szBaseFont, sizeof(m_szBaseFont));
    m_defaultColor = other.m_color;
    m_color = other.m_color;
    m_nAlignment = other.m_nAlignment;
    m_fAlpha = other.m_fAlpha;
    bHilight = other.bHilight;
}

CSWGuiTextParams& CSWGuiTextParams::operator=(const CSWGuiTextParams& other)
{
    m_sText = other.m_sText;
    m_nStrRef = other.m_nStrRef;
    __strcpy_chk(m_szBaseFont, other.m_szBaseFont, sizeof(m_szBaseFont));
    m_defaultColor = other.m_color;
    m_color = other.m_color;
    m_nAlignment = other.m_nAlignment;
    m_fAlpha = other.m_fAlpha;
    bHilight = other.bHilight;
    SetTextObject(m_pTextObject);
    return *this;
}

void CSWGuiTextParams::SetTextObject(CSWGuiText* pTextObject)
{
    m_pTextObject = pTextObject;
    if (m_pTextObject)
    {
        if (m_pTextObject->m_pString == NULL)
        {
            char szFontTexture[17];
            m_resRefFont.CopyToString(szFontTexture);
            m_pTextObject->m_pString =
                NewCAurGUIString(static_cast<char*>(m_sText.CStr()), szFontTexture, m_color, 1.0f);
        }
        uint64_t nStrRef = m_nStrRef;
        SetFontRename();
        SetText(m_sText);
        SetAlignment(m_nAlignment);
        SetColor(m_color);
        m_nStrRef = nStrRef;
    }
}

void CSWGuiTextParams::SetFont(const CResRef& resRefFont)
{
    m_resRefFont = resRefFont;
    if (m_pTextObject && m_pTextObject->m_pString && m_resRefFont.IsValid())
    {
        char szFontTexture[17];
        m_resRefFont.CopyToString(szFontTexture);
        m_pTextObject->m_pString->SetFontTexture(szFontTexture);
        m_pTextObject->WrapString();
    }
}

CSWGuiText::CSWGuiText()
{
    m_pString = NULL;
    m_params.SetTextObject(this);
    m_nWrapWidth = 400;
}

CSWGuiText::~CSWGuiText()
{
#ifdef K2_QOL_IMPROVEMENTS
    K2QolUnmarkFitText(this);
#endif
    if (m_pString)
    {
        delete m_pString;
#if !KOTOR1
        m_pString = NULL;
#endif
    }
}

void CSWGuiText::WrapString()
{
    // Height is tested before width.
#ifdef K2_QOL_IMPROVEMENTS
    if (m_pString && m_extent.h && m_extent.w && K2QolIsFitText(this))
    {
        K2QolFitText(*this, m_extent.h);   // a button caption: the Text Size, as far as its box holds
        return;
    }
#endif
    if (m_pString && m_extent.h && m_extent.w)
        m_pString->WrapStrings(-1);
}

void CSWGuiTextParams::SetFontRename()
{
    char szFontName[17];
    __strcpy_chk(szFontName, m_szBaseFont, sizeof(szFontName));
    g_pGuiMan->GetUpdatedFontName(szFontName);

    char szCurrentFont[17];
    m_resRefFont.CopyToString(szCurrentFont);

    if (_stricmp(szFontName, szCurrentFont))
    {
        CResRef resRefFont(szFontName);
        SetFont(resRefFont);
    }
}

void CSWGuiTextParams::ReSetFont()
{
    SetFontRename();
}

void CSWGuiTextParams::SetText(const CExoString& sText)
{
    m_sText = sText;
    m_nStrRef = STRREF_NONE;
    if (m_pTextObject && m_pTextObject->m_pString)
    {
        m_pTextObject->m_pString->SetString(static_cast<char*>(m_sText.CStr()));
        m_pTextObject->WrapString();
    }
}

void CSWGuiTextParams::SetLocString(CExoLocString& locString, int nLanguage)
{
    CExoString sText;
    locString.GetString(nLanguage, &sText, 0);
    SetText(sText);
}

void CSWGuiTextParams::SetAlignment(int nAlignment)
{
    m_nAlignment = nAlignment;
    if (m_pTextObject && m_pTextObject->m_pString)
    {
        m_pTextObject->m_pString->SetAlignment(m_nAlignment);
        m_pTextObject->WrapString();
    }
}

void CSWGuiTextParams::SetColor(const Vector& color)
{
    m_color = color;
    if (m_pTextObject && m_pTextObject->m_pString)
        m_pTextObject->m_pString->SetColor(m_color);
}

void CSWGuiTextParams::SetDefaultColor(const Vector& color)
{
    m_defaultColor = color;
    SetColor(color);
}

void CSWGuiTextParams::RestoreDefaultColor()
{
    SetColor(m_defaultColor);
}

CSWGuiText& CSWGuiText::operator=(CSWGuiText& other)
{
    m_params = other.m_params;
    return *this;
}

void CSWGuiText::Initialize(const CSWGuiExtent& extent, const CSWGuiTextParams& params, float fAlpha)
{
    if (m_pString)
        delete m_pString;

    char szFontTexture[17];
    params.m_resRefFont.CopyToString(szFontTexture);
    m_pString = NewCAurGUIString(static_cast<char*>(params.m_sText.CStr()), szFontTexture, params.m_color, fAlpha);

    m_params = params;
    m_extent = extent;

    if (m_pString)
    {
        m_pString->SetPositionBoundingRect(m_extent.x, m_extent.y);
        m_pString->SetSizeBoundingRect(m_extent.w, m_extent.h);
        m_pString->SetAlignment(params.m_nAlignment);
        WrapString();
    }
}

void CSWGuiText::Draw(float fAlpha)
{
    K2_UNUSED(fAlpha);
    if (m_pString)
        m_pString->Draw(m_params.bHilight ? m_fPulsingAlpha : m_params.m_fAlpha);
}

void CSWGuiText::SetExtent(const CSWGuiExtent& extent)
{
    m_extent = extent;
    if (m_pString)
    {
        // Both setters use the parameter, not the member just stored.
        m_pString->SetPositionBoundingRect(extent.x, extent.y);
        m_pString->SetSizeBoundingRect(extent.w, extent.h);
        WrapString();
    }
}

int CSWGuiText::GetIdealHeight()
{
    if (m_pString == NULL)
        return 0;
#if KOTOR1
    // K1: an empty string is 0 high, otherwise always one pixel of leading.
    if (m_pString->GetStringSize() < 1)
        return 0;
    return m_pString->GetIdealPixelHeight(-1) + 1;
#else
    // Small fonts get one extra pixel of leading.
    int nFontPixelHeight = m_pString->GetFontPixelHeight();
    int nHeight = m_pString->GetIdealPixelHeight(-1);
    if (nFontPixelHeight < 16)
        nHeight++;
    return nHeight;
#endif
}

int CSWGuiText::GetFontHeight()
{
    if (m_pString == NULL)
        return 0;
    // GetFontPixelHeight is called twice on purpose; do not cache the first result.
    int nFontPixelHeight = m_pString->GetFontPixelHeight();
    int nHeight = m_pString->GetFontPixelHeight();
    if (nFontPixelHeight < 16)
        nHeight++;
    return nHeight;
}

CSWGuiExtent CSWGuiText::GetIdealWidthAndHeight()
{
#if KOTOR1
    // K1: the wrap width is not resolution-scaled
    int nIdealHeight = m_pString->GetIdealPixelHeight(m_nWrapWidth);
    int nWidth = m_nWrapWidth;
#else
    float fScale = GetResolutionTextScalar();

    int nIdealHeight = m_pString->GetIdealPixelHeight(static_cast<int>(fScale * m_nWrapWidth));
    int nWidth = static_cast<int>(fScale * m_nWrapWidth);
#endif
    // Shrinks the wrap width in 10-unit steps while the laid-out height is unchanged, then adds
    // one step back. The guard is written twice on purpose: the loop re-enters through a latch.
    if (nIdealHeight == m_pString->GetIdealPixelHeight(nWidth))
    {
        do
        {
            nWidth -= 10;
            if (nWidth <= 0)
            {
#if KOTOR1
                nWidth = m_nWrapWidth;
#else
                nWidth = static_cast<int>(fScale * m_nWrapWidth);
#endif
                break;
            }
        } while (nIdealHeight == m_pString->GetIdealPixelHeight(nWidth));
    }

    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 0;
    extent.w = nWidth + 10;
    extent.h = nIdealHeight;
    if (m_pString->GetFontPixelHeight() < 16)
        extent.h++;
    return extent;
}

void CSWGuiText::Load(CResGFF* pGFF, CResStruct& rStruct, const CExoString& sFieldName)
{
    CResStruct subStruct;
    if (pGFF->GetStructFromStruct(&subStruct, &rStruct, static_cast<char*>(sFieldName.CStr())))
    {
        int bSuccess;

        m_params.SetBaseFont(pGFF->ReadFieldCResRef(&subStruct, const_cast<char*>("FONT"), bSuccess,
                                                    CResRef("dialogfont16x16")));

        m_params.SetDefaultColor(pGFF->ReadFieldVector(&subStruct, const_cast<char*>("COLOR"), bSuccess,
                                                       Vector{1.0f, 1.0f, 1.0f}));

        m_params.SetText(pGFF->ReadFieldCExoString(&subStruct, const_cast<char*>("TEXT"), bSuccess,
                                                   CExoString("")));
        m_params.SetStrRef(pGFF->ReadFieldDWORD(&subStruct, const_cast<char*>("STRREF"), bSuccess, STRREF_NONE));
        m_params.SetAlignment(pGFF->ReadFieldINT(&subStruct, const_cast<char*>("ALIGNMENT"), bSuccess, 9));
        m_params.bHilight = pGFF->ReadFieldBYTE(&subStruct, const_cast<char*>("PULSING"), bSuccess, 0);

        if (m_pString)
        {
            delete m_pString;
            m_pString = NULL;
        }

        char szFontTexture[17];
        m_params.m_resRefFont.CopyToString(szFontTexture);
        m_pString = NewCAurGUIString(static_cast<char*>(m_params.m_sText.CStr()), szFontTexture,
                                     m_params.m_color, 1.0f);
#if KOTOR1 && defined(K2_QOL_FULLSCREEN)
        // A layout loaded again into the same control (SetVideoMode after a window change) finds
        // the old layout's extent here, and the new string, still unsized, would be wrapped against
        // it -- WrapStrings then takes it for a narrow box and drops the string's 1.3 text scale.
        // The control's own extent, set after the load, sizes the string as on the first load.
        m_extent = CSWGuiExtent();
#endif
        m_params.SetTextObject(this);
    }
}

CSWGuiEditText::CSWGuiEditText()
{
    m_nMaxLength = -1;
    m_bCaretVisible = 0;
    m_sText = "";
}

void CSWGuiEditText::RemoveLastChar()
{
    CExoString sCaret;
    if (m_sText.GetLength() > 0)
    {
        m_sText = m_sText.SubString(0, m_sText.GetLength() - 1);
        if (!m_bCaretVisible)
            m_params.SetText(m_sText);
        else
        {
            sCaret.Format(const_cast<char*>("%s_"), m_sText.CStr());
            m_params.SetText(sCaret);
        }
    }
}

void CSWGuiEditText::AddNewChar(char* sChar)
{
    CExoString sCaret;
    int nLimit = m_nMaxLength;
    if ((nLimit & 0xffff) != 0xffff &&
        m_sText.GetLength() >= static_cast<short>(nLimit))
        return;
    m_sText = m_sText + CExoString(sChar);
    if (!m_bCaretVisible)
        m_params.SetText(m_sText);
    else
    {
        sCaret.Format(const_cast<char*>("%s_"), m_sText.CStr());
        m_params.SetText(sCaret);
    }
}

void CSWGuiEditText::SetText(const CExoString& sText)
{
    CExoString sCaret;
    m_sText = sText;
    if (!m_bCaretVisible)
        m_params.SetText(m_sText);
    else
    {
        sCaret.Format(const_cast<char*>("%s_"), sText.CStr());
        m_params.SetText(sCaret);
    }
}

void CSWGuiEditText::SetCaretVisible(int bVisible)
{
    m_bCaretVisible = bVisible;
    if (bVisible == 0)
        m_params.SetText(m_sText);
    else
    {
        CExoString sCaret;
        sCaret.Format(const_cast<char*>("%s_"), m_sText.CStr());
        m_params.SetText(sCaret);
    }
}

#if KOTOR1
CSWGuiBorderParams::CSWGuiBorderParams(const CResRef& resRefCorner, const CResRef& resRefEdge,
                                       const CResRef& resRefFill, int nFillStyle, int nBorderDim,
                                       float fAlpha, const Vector& color, int nStack0)
{
    m_cornerImage = resRefCorner;
    m_edgeImage = resRefEdge;
    m_fillImage = resRefFill;

    m_nBorderDim = nBorderDim;
    m_nInnerOffset = nStack0;
    m_fRotate = 0.0f;
    m_fAlpha = fAlpha;

    m_flags.nFillStyle = nFillStyle;
    m_flags.bPulseA = 0;
    m_color.r = color.x;
    m_color.g = color.y;
    m_color.b = color.z;
    m_flags.bPulseB = 0;
    m_flags.bPulseC = 0;

    m_pBorder = 0;
}
#else
CSWGuiBorderParams::CSWGuiBorderParams(const CResRef& resRefCorner, const CResRef& resRefEdge,
                                       const CResRef& resRefFill, int nFillStyle, int nBorderDim,
                                       int nFlipStyle, float fAlpha, const Vector& color,
                                       int nStack0, int nStack1)
{
    m_cornerImage = resRefCorner;
    m_edgeImage = resRefEdge;
    m_fillImage = resRefFill;

    m_nBorderDim = nBorderDim;
    m_nInnerOffset = nStack0;
    m_nInnerOffsetY = nStack1;
    m_fRotate = 0.0f;
    m_fAlpha = fAlpha;

    m_flags.nFillStyle = nFillStyle;
    m_flags.nFlipStyle = nFlipStyle;
    m_flags.bPulseA = 0;
    m_color.r = color.x;
    m_color.g = color.y;
    m_color.b = color.z;
    m_flags.bPulseB = 0;
    m_flags.bPulseC = 0;

    m_pBorder = 0;
}
#endif

CSWGuiBorderParams::CSWGuiBorderParams(const CSWGuiBorderParams& other)
{
    m_cornerImage = other.m_cornerImage;
    m_edgeImage = other.m_edgeImage;
    m_fillImage = other.m_fillImage;

    m_flags.nFillStyle = other.m_flags.nFillStyle;
    m_fRotate = other.m_fRotate;
#if !KOTOR1
    m_flags.nFlipStyle = other.m_flags.nFlipStyle;
#endif
    m_nBorderDim = other.GetBorderDim();
    m_fAlpha = other.m_fAlpha;
    m_flags.bPulseA = other.m_flags.bPulseA;
    m_color = other.m_color;
    m_flags.bPulseB = other.m_flags.bPulseB;
    m_flags.bPulseC = other.m_flags.bPulseC;

    m_nInnerOffset = other.m_nInnerOffset;
#if !KOTOR1
    m_nInnerOffsetY = other.m_nInnerOffsetY ? other.m_nInnerOffsetY : other.m_nInnerOffset;
#endif

    m_pBorder = 0;
}

CSWGuiBorderParams& CSWGuiBorderParams::operator=(const CSWGuiBorderParams& other)
{
    m_cornerImage = other.m_cornerImage;
    m_edgeImage = other.m_edgeImage;
    m_fillImage = other.m_fillImage;

    m_flags.nFillStyle = other.m_flags.nFillStyle;
    m_fRotate = other.m_fRotate;
#if !KOTOR1
    m_flags.nFlipStyle = other.m_flags.nFlipStyle;
#endif
    m_nBorderDim = other.GetBorderDim();
    m_fAlpha = other.m_fAlpha;
    m_flags.bPulseA = other.m_flags.bPulseA;
    m_color = other.m_color;
    m_flags.bPulseB = other.m_flags.bPulseB;
    m_flags.bPulseC = other.m_flags.bPulseC;

    m_nInnerOffset = other.m_nInnerOffset;
#if !KOTOR1
    m_nInnerOffsetY = other.m_nInnerOffsetY ? other.m_nInnerOffsetY : other.m_nInnerOffset;
#endif

#if KOTOR1
    // K1 re-installs the images through SetBorder.
    SetBorder(m_pBorder);
#else
    if (m_pBorder) {
        SetCornerImage(m_cornerImage, 1);
        SetEdgeImage(m_edgeImage, 1);
        SetFillImage(m_fillImage, 1);
    }
#endif
    return *this;
}

int CSWGuiBorderParams::GetBorderDim() const
{
    if (m_nBorderDim != 0)
        return m_nBorderDim;
    if (m_pBorder == 0)
        return 0;
    if (m_pBorder->m_pCornerImage == 0)
        return 0;
    if (m_pBorder->m_pEdgeImage == 0)
        return 0;
    return m_pBorder->m_pCornerImage->GetImageHeight();
}

void CSWGuiBorderParams::SetBorder(CSWGuiBorder* pBorder)
{
    m_pBorder = pBorder;
    if (pBorder) {
        SetCornerImage(m_cornerImage, 1);
        SetEdgeImage(m_edgeImage, 1);
        SetFillImage(m_fillImage, 1);
    }
}

void CSWGuiBorderParams::SetCornerImage(const CResRef& resRef, int flag)
{
    if (resRef != m_cornerImage || flag)
    {
        m_cornerImage = resRef;

        if (m_pBorder != 0)
        {
            if (m_pBorder->m_pCornerImage != 0)
            {
                delete m_pBorder->m_pCornerImage;
                m_pBorder->m_pCornerImage = 0;
            }

            if (m_cornerImage.IsValid())
            {
                char szImage[17];
                m_cornerImage.CopyToString(szImage);
                m_pBorder->m_pCornerImage = NewCAurGUIImage(szImage);
            }
        }
    }
}

void CSWGuiBorderParams::SetEdgeImage(const CResRef& resRef, int flag)
{
    if (resRef != m_edgeImage || flag)
    {
        m_edgeImage = resRef;

        if (m_pBorder != 0)
        {
            if (m_pBorder->m_pEdgeImage != 0)
            {
                delete m_pBorder->m_pEdgeImage;
                m_pBorder->m_pEdgeImage = 0;
            }

            if (m_edgeImage.IsValid())
            {
                char szImage[17];
                m_edgeImage.CopyToString(szImage);
                m_pBorder->m_pEdgeImage = NewCAurGUIImage(szImage);
            }
        }
    }
}

void CSWGuiBorderParams::SetFillImage(const CResRef& resRef, int flag)
{
    if (resRef != m_fillImage || flag)
    {
        m_fillImage = resRef;

        if (m_pBorder != 0)
        {
            if (m_pBorder->m_pFillImage != 0)
            {
                delete m_pBorder->m_pFillImage;
                m_pBorder->m_pFillImage = 0;
            }

            if (m_fillImage.IsValid())
            {
                char szImage[17];
                m_fillImage.CopyToString(szImage);
                m_pBorder->m_pFillImage = NewCAurGUIImage(szImage);
            }
        }
    }
}

void CSWGuiBorderParams::SetPulsingAlpha(int a, int b, int c)
{
    m_flags.bPulseA = a;
    m_flags.bPulseB = b;
    m_flags.bPulseC = c;
}

CSWGuiExtent CSWGuiBorder::GetInnerExtent()
{
    int nInsetW;
    int nInsetH;
    int nTile;
    if (m_params.m_nBorderDim != 0) {
        nInsetW = nInsetH = nTile = m_params.m_nBorderDim;
    } else if (m_params.GetBorderDim() != 0) {
        nInsetW = nInsetH = nTile = m_params.GetBorderDim();
    } else if (m_pCornerImage != 0 && m_pEdgeImage != 0 && m_extent.h != 0 && m_extent.w != 0) {
        nInsetH = m_pCornerImage->GetImageHeight();
        nInsetW = m_pCornerImage->GetImageWidth();
        m_pEdgeImage->GetImageHeight();
        nTile = m_pEdgeImage->GetImageWidth();
    } else {
        nInsetW = nInsetH = nTile = 0;
    }

    CSWGuiExtent inner;
    int nX;
    int nY;
    int nInnerW = m_extent.w - nInsetW * 2;
    int nTilesW = nTile ? nInnerW / nTile : 0;
    if (nInnerW < 1 || (nTilesW == 0 && nTile / 2 > nInnerW)) {
        nX = m_extent.x + m_extent.w / 2;
        inner.w = 0;
    } else {
        nX = m_extent.x + nInsetW;
        inner.w = nInnerW;
    }

    int nInnerH = m_extent.h - nInsetH * 2;
    int nTilesH = nTile ? nInnerH / nTile : 0;
    if (nInnerH < 1 || (nTile / 2 > nInnerH && nTilesH == 0)) {
        nY = m_extent.y + m_extent.h / 2;
    } else {
        nY = m_extent.y + nInsetH;
        inner.h = nInnerH;
    }

    int nPadX = m_params.m_nInnerOffset;
#if KOTOR1
    // K1 uses one inner offset for both axes.
    nY -= m_params.m_nInnerOffset;
#else
    int nPadY = m_params.m_nInnerOffsetY ? m_params.m_nInnerOffsetY : m_params.m_nInnerOffset;

    nY -= nPadY;
#endif

#if KOTOR1
    // K1 asks for the border dimension once per clamp.
    int nDim = m_params.GetBorderDim();
    nX -= nPadX < nDim ? nPadX : nDim;
    nPadX = m_params.m_nInnerOffset;
    nDim = m_params.GetBorderDim();
    nPadX = nPadX < nDim ? nPadX : nDim;
#else
    if (nPadX < m_params.GetBorderDim())
        nX -= nPadX;
    else
        nX -= m_params.GetBorderDim();

    if (nPadX >= m_params.GetBorderDim())
        nPadX = m_params.GetBorderDim();
#endif

    inner.x = nX;
    inner.y = nY;
    inner.w += nPadX * 2;
#if KOTOR1
    inner.h += m_params.m_nInnerOffset * 2;
#else
    inner.h += nPadY * 2;
#endif
    return inner;
}

void CSWGuiBorder::Load(CResGFF* pGFF, CResStruct& rStruct, const CExoString& sFieldName)
{
    CResStruct subStruct;
    if (!pGFF->GetStructFromStruct(&subStruct, &rStruct, static_cast<char*>(sFieldName.CStr())))
        return;

    int bSuccess;
    m_params.SetCornerImage(pGFF->ReadFieldCResRef(&subStruct, const_cast<char*>("CORNER"), bSuccess, CResRef("")), 0);
    m_params.SetEdgeImage(pGFF->ReadFieldCResRef(&subStruct, const_cast<char*>("EDGE"), bSuccess, CResRef("")), 0);
    m_params.SetFillImage(pGFF->ReadFieldCResRef(&subStruct, const_cast<char*>("FILL"), bSuccess, CResRef("")), 0);
    m_params.m_flags.nFillStyle = pGFF->ReadFieldINT(&subStruct, const_cast<char*>("FILLSTYLE"), bSuccess, 0);
    m_params.m_nBorderDim = pGFF->ReadFieldINT(&subStruct, const_cast<char*>("DIMENSION"), bSuccess, 0);
    m_params.m_nInnerOffset = pGFF->ReadFieldINT(&subStruct, const_cast<char*>("INNEROFFSET"), bSuccess, 0);
#if !KOTOR1
    m_params.m_nInnerOffsetY = pGFF->ReadFieldINT(&subStruct, const_cast<char*>("INNEROFFSETY"), bSuccess, 0);
#endif

    Vector vColor = pGFF->ReadFieldVector(&subStruct, const_cast<char*>("COLOR"), bSuccess, Vector(1.0f, 1.0f, 1.0f));
    m_params.m_color.r = vColor.x;
    m_params.m_color.g = vColor.y;
    m_params.m_color.b = vColor.z;

    unsigned char nPulsing = pGFF->ReadFieldBYTE(&subStruct, const_cast<char*>("PULSING"), bSuccess, 0);
    m_params.SetPulsingAlpha(nPulsing != 0, nPulsing & 1, (nPulsing >> 1) & 1);
}

void CSWGuiBorder::FillTile(int nHeight, int nWidth, int nX, int nY, float fAlpha,
                            const Vector& vColor)
{
    int nImageWidth = m_pFillImage->GetImageWidth();
    int nImageHeight = m_pFillImage->GetImageHeight();
    if (nImageWidth < 1 || nImageHeight < 1)
        return;

    float fRotate = m_params.m_fRotate;

    int nRows = nHeight / nImageHeight;
    int nCols = nWidth / nImageWidth;
    int nLeftH = nHeight - nRows * nImageHeight;
    int nLeftW = nWidth - nCols * nImageWidth;

    int nRowExtra K2_UNSET;  // only read where nRows != 0
    if (nRows != 0) {
        nRowExtra = nLeftH / nRows;
        nLeftH -= nRowExtra * nRows;
    }

    if (nCols == 0) {
        if (nRows == 0) {
            m_pFillImage->Draw(nX, nY, nLeftW, nLeftH, 0, fRotate, vColor, fAlpha);
            return;
        }

        int nTileH = nRowExtra + nImageHeight;
        for (int r = 0; r < nRows; r++) {
            int nH = nTileH;
            if (nLeftH) {
                nH++;
                nLeftH--;
            }
            m_pFillImage->Draw(nX, nY, nLeftW, nH, 0, fRotate, vColor, fAlpha);
            nY += nH;
        }
        return;
    }

    int nColExtra = nLeftW / nCols;
    nLeftW -= nColExtra * nCols;
    int nBudgetH = nLeftH;
    for (int c = 0; c < nCols; c++) {
        int nW = nColExtra + nImageWidth;
        if (nLeftW) {
            nW++;
            nLeftW--;
        }

        if (nRows != 0) {
            nBudgetH = nLeftH;
            // The height budget and the running y restart in every column; one column's row
            // run already covers the whole height.
            int nColY = nY;
            int nBaseH = nRowExtra + nImageHeight;
            for (int r = 0; r < nRows; r++) {
                int nH = nBaseH;
                if (nBudgetH) {
                    nH++;
                    nBudgetH--;
                }
                m_pFillImage->Draw(nX, nColY, nW, nH, 0, fRotate, vColor, fAlpha);
                nColY += nH;
            }
        } else {
            m_pFillImage->Draw(nX, nY, nW, nBudgetH, 0, fRotate, vColor, fAlpha);
        }

        nX = nW + nX;
    }
}

void CSWGuiBorder::FillCenter(int nHeight, int nWidth, int nX, int nY, float fAlpha,
                              const Vector& vColor)
{
    int nImageWidth = m_pFillImage->GetImageWidth();
    int nImageHeight = m_pFillImage->GetImageHeight();
    float fRotate = m_params.m_fRotate;

    if (nImageWidth < nWidth && nImageHeight < nHeight) {
        m_pFillImage->Draw(nX + (nWidth - nImageWidth) / 2, nY + (nHeight - nImageHeight) / 2,
                           nImageWidth, nImageHeight, 0, fRotate, vColor, fAlpha);
    } else if (nImageWidth < nWidth) {
        m_pFillImage->Draw(nX + (nWidth - nImageWidth) / 2, nY, nImageWidth, nHeight,
                           0, fRotate, vColor, fAlpha);
    } else if (nImageHeight < nHeight) {
        m_pFillImage->Draw(nX, nY + (nHeight - nImageHeight) / 2, nWidth, nImageHeight,
                           0, fRotate, vColor, fAlpha);
    } else {
        m_pFillImage->Draw(nX, nY, nWidth, nHeight, 0, fRotate, vColor, fAlpha);
    }
}

void CSWGuiBorder::Draw(float fAlpha)
{
    K2_UNUSED(fAlpha);
    if (m_extent.w == 0)
        return;
    if (m_extent.h == 0)
        return;

    // bPulseA gates both overrides: bPulseB drives the image stamps, bPulseC the fill helpers.
    float fAlphaImg = m_params.m_fAlpha;
    float fAlphaFill = m_params.m_fAlpha;
    if (m_params.m_flags.bPulseA) {
        if (m_params.m_flags.bPulseB)
            fAlphaImg = CSWGuiDrawable::m_fPulsingAlpha;
        if (m_params.m_flags.bPulseC)
            fAlphaFill = CSWGuiDrawable::m_fPulsingAlpha;
    }

    int nInsetW = 0;
    int nInsetH = 0;
    int nTile = 0;
    int nTileHeight = 0;
    int nRightW = 0;
    int nBotH = 0;
    int nRightX = 0;
    int nBotY = 0;
    int nX = m_extent.x;
    int nY = m_extent.y;

    if (m_pCornerImage != 0) {
        if (m_params.m_nBorderDim != 0) {
            nInsetW = nInsetH = nTile = nTileHeight = m_params.m_nBorderDim;
        } else if (m_params.GetBorderDim() != 0) {
            nInsetW = nInsetH = nTile = nTileHeight = m_params.GetBorderDim();
        } else {
            nInsetW = m_pCornerImage->GetImageWidth();
            nInsetH = m_pCornerImage->GetImageHeight();
            if (m_pEdgeImage != 0) {
                nTile = m_pEdgeImage->GetImageWidth();
                nTileHeight = m_pEdgeImage->GetImageHeight();
            } else {
                nTile = nInsetW;
                nTileHeight = nInsetH;
            }
        }

        int nInnerW = m_extent.w - nInsetW * 2;
        int nCols = 0;
        int nUsedW = 0;
        if (nInnerW >= 1) {
            nCols = nTile != 0 ? nInnerW / nTile : 0;
            if (nCols != 0)
                nUsedW = nCols * nTile;
            else if (nInnerW >= nTile / 2) {
                nCols = 1;
                nUsedW = nTile;
            }
        }
        int nSlackW = nInnerW;
        int nColExtra = 0;
        int bHasW = 0;
        if (nCols != 0) {
            nSlackW = nInnerW - nUsedW;
            nColExtra = nSlackW / nCols;
            bHasW = 1;
        }

        int nInnerH = m_extent.h - nInsetH * 2;
        int nRows = 0;
        int nUsedH = 0;
        if (nInnerH >= 1) {
            nRows = nTile != 0 ? nInnerH / nTile : 0;
            if (nRows != 0)
                nUsedH = nRows * nTileHeight;
            else if (nInnerH >= nTile / 2) {
                nRows = 1;
                nUsedH = nTileHeight;
            }
        }
        int nSlackH = nInnerH;
        int nRowExtra = 0;
        int bHasH = 0;
        if (nRows != 0) {
            nSlackH = nInnerH - nUsedH;
            nRowExtra = nSlackH / nRows;
            bHasH = 1;
        }

        // A centred axis gives its extent to the two corner stamps, so the second absorbs the
        // odd pixel.
        if (nRows != 0) {
            nSlackH = nInnerH - nUsedH;
            nRowExtra = nSlackH / nRows;
            // The two arms stay separate rather than sharing a trailing if.
            if (bHasW != 0) {
                nX = nInsetW + nX;
                nY = nInsetH + nY;
            } else {
                nY = nInsetH + nY;
                nInsetW = m_extent.w / 2;
            }
            bHasH = 1;
        } else {
            if (bHasW != 0) {
                nX = nInsetW + nX;
                nInsetH = m_extent.h / 2;
            } else {
                nInsetH = m_extent.h / 2;
                nInsetW = m_extent.w / 2;
            }
        }

        // The odd pixel is subtracted separately from the inset width.
        int nExtraW = bHasW != 0 ? 0 : (m_extent.w & 1);
        nRightW = nInsetW + nExtraW;
        nRightX = m_extent.x + m_extent.w - nInsetW - nExtraW;
        int nExtraH = m_extent.h & (bHasH ^ 1);
        nBotH = nInsetH + nExtraH;
        nBotY = m_extent.y + m_extent.h - nInsetH - nExtraH;

        // The fifth argument is the corner turn order, 0, 4, 0, 8 for the four stamps, so only the
        // bottom-right one needs an explicit half turn.
        m_pCornerImage->DrawBuffered(m_extent.x, m_extent.y, nInsetW, nInsetH, 0, 0.0f,
                                     reinterpret_cast<const Vector&>(m_params.m_color), fAlphaImg);
        m_pCornerImage->DrawBuffered(m_extent.x, nBotY, nInsetW, nBotH, 4, 0.0f,
                                     reinterpret_cast<const Vector&>(m_params.m_color), fAlphaImg);
        m_pCornerImage->DrawBuffered(nRightX, nBotY, nRightW, nBotH, 0, 180.0f,
                                     reinterpret_cast<const Vector&>(m_params.m_color), fAlphaImg);
        m_pCornerImage->DrawBuffered(nRightX, m_extent.y, nRightW, nInsetH, 8, 0.0f,
                                     reinterpret_cast<const Vector&>(m_params.m_color), fAlphaImg);

        if (m_pEdgeImage != 0) {
            int nRemainderW = nSlackW - nColExtra * nCols;
            int nTileWidth = nColExtra + nTile;
            for (int i = nCols; i > 0; i--) {
                int nThisW = nTileWidth;
                if (nRemainderW != 0) {
                    nThisW++;
                    nRemainderW--;
                }
                m_pEdgeImage->DrawBuffered(nX, m_extent.y, nThisW, nInsetH, 0, 0.0f,
                                           reinterpret_cast<const Vector&>(m_params.m_color), fAlphaImg);
                m_pEdgeImage->DrawBuffered(nX, nBotY, nThisW, nBotH, 0, 180.0f,
                                           reinterpret_cast<const Vector&>(m_params.m_color), fAlphaImg);
                nX += nThisW;
            }

            int nRemainderH = nSlackH - nRowExtra * nRows;
            int nRowHeight = nRowExtra + nTile;
            for (int i = nRows; i > 0; i--) {
                int nThisH = nRowHeight;
                if (nRemainderH != 0) {
                    nThisH++;
                    nRemainderH--;
                }
                m_pEdgeImage->DrawBuffered(m_extent.x, nY, nInsetW, nThisH, 0, 90.0f,
                                           reinterpret_cast<const Vector&>(m_params.m_color), fAlphaImg);
                m_pEdgeImage->DrawBuffered(nRightX, nY, nRightW, nThisH, 0, 270.0f,
                                           reinterpret_cast<const Vector&>(m_params.m_color), fAlphaImg);
                nY += nThisH;
            }
        }
    }

    CAurGUIImage::FlushBuffer(CAurGUIImage::cm_fAlpha);

    if (m_pFillImage != 0) {
        // Case 2 overwrites these with the inner rect and falls through into case 3's single
        // draw call, so there is one call site and not two.
        int nFillX = m_extent.x;
        int nFillY = m_extent.y;
        int nW = m_extent.w;
        int nH = m_extent.h;
        int nInsetX = nFillX + nInsetW;
        int nInsetY = nFillY + nInsetH;
        int nInsetWidth = nW - nInsetW - nRightW;
        int nInsetHeight = nH - nInsetH - nBotH;

        switch (m_params.m_flags.nFillStyle) {
        case 0:
            FillTile(nInsetHeight, nInsetWidth, nInsetX, nInsetY, fAlphaFill,
                     reinterpret_cast<const Vector&>(m_params.m_color));
            break;
        case 1:
            FillCenter(nInsetHeight, nInsetWidth, nInsetX, nInsetY, fAlphaFill,
                       reinterpret_cast<const Vector&>(m_params.m_color));
            break;
#if KOTOR1
        // K1: only the stretched copy over the inner rect; no raw-extent case.
        case 2:
            m_pFillImage->Draw(nInsetX, nInsetY, nInsetWidth, nInsetHeight, 0,
                               m_params.m_fRotate, reinterpret_cast<const Vector&>(m_params.m_color), fAlphaFill);
            break;
#else
        case 2:
            nFillX = nInsetX;
            nFillY = nInsetY;
            nW = nInsetWidth;
            nH = nInsetHeight;
            // falls through into case 3
        case 3:
            // Over the raw extent, ignoring the insets.
            m_pFillImage->Draw(nFillX, nFillY, nW, nH, m_params.m_flags.nFlipStyle,
                               m_params.m_fRotate, reinterpret_cast<const Vector&>(m_params.m_color), fAlphaFill);
            break;
#endif
        }
    }

    CAurGUIImage::FlushBuffer(CAurGUIImage::cm_fAlpha);
}

CSWGuiBorder::~CSWGuiBorder()
{
    if (m_pCornerImage) {
        delete m_pCornerImage;
        m_pCornerImage = 0;
    }
    if (m_pEdgeImage) {
        delete m_pEdgeImage;
        m_pEdgeImage = 0;
    }
    if (m_pFillImage) {
        delete m_pFillImage;
        m_pFillImage = 0;
    }
}

void CSWGuiBorder::Initialize(const CSWGuiExtent& extent, const CSWGuiBorderParams& params)
{
    m_extent = extent;
    m_params = params;
}

CSWGuiBorder& CSWGuiBorder::operator=(CSWGuiBorder& other)
{
    m_params = other.m_params;
    return *this;
}

CSWGuiBorder::CSWGuiBorder()
    : m_params(CResRef(), CResRef(), CResRef(), 0, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0)
{
#if KOTOR1
    // K1 clears the three image slots and installs the owner through SetBorder.
    m_pCornerImage = 0;
    m_pEdgeImage = 0;
    m_pFillImage = 0;
    m_params.SetBorder(this);
#else
    m_params.m_pBorder = this;
    m_pCornerImage = 0;
    m_pEdgeImage = 0;
    m_pFillImage = 0;

    m_params.SetCornerImage(m_params.m_cornerImage, 1);
    m_params.SetEdgeImage(m_params.m_edgeImage, 1);
    m_params.SetFillImage(m_params.m_fillImage, 1);
#endif
}

// Rotations for the rotation-style index; the guard is two-sided because the index is signed.
static const float s_afRotateStyle[4] = { 0.0f, 90.0f, 180.0f, 270.0f };

CSWGuiImageParams::CSWGuiImageParams(const CResRef& resRefImage, int bDrawStyleIn, int nFlipStyle,
                                     float fRotate, int nAlignment, float fAlpha,
                                     const Vector& color)
{
    // m_color is default-initialized by Vector's constructor and assigned again here.
    m_resRefImage = resRefImage;
    bDrawStyle    = bDrawStyleIn;
    m_nFlipStyle  = nFlipStyle;
    m_fRotate     = fRotate;
    m_nAlignment  = nAlignment;
    m_fAlpha      = fAlpha;
    bHilight      = 0;
    m_color       = color;
    m_pImageObject = NULL;
}

CSWGuiImageParams::CSWGuiImageParams(const CSWGuiImageParams& other)
{
    // Same implicit zeroing of m_color as the 7-argument constructor.
    m_resRefImage = other.m_resRefImage;
    bDrawStyle    = other.bDrawStyle;
    m_nFlipStyle  = other.m_nFlipStyle;
    m_fRotate     = other.m_fRotate;
    m_nAlignment  = other.m_nAlignment;
    m_fAlpha      = other.m_fAlpha;
    bHilight      = other.bHilight;
    m_color       = other.m_color;
    // A copied block is not bound to anything; the widget binds it, not the other way round.
    m_pImageObject = NULL;
}

CSWGuiImageParams& CSWGuiImageParams::operator=(const CSWGuiImageParams& other)
{
    m_resRefImage = other.m_resRefImage;
    bDrawStyle    = other.bDrawStyle;
    m_nFlipStyle  = other.m_nFlipStyle;
    m_fRotate     = other.m_fRotate;
    m_nAlignment  = other.m_nAlignment;
    m_fAlpha      = other.m_fAlpha;
    bHilight      = other.bHilight;
    m_color       = other.m_color;
    // Rebind to self, pushing the copied parameters onto the bound widget.
    SetImageObject(m_pImageObject);
    return *this;
}

void CSWGuiImageParams::SetImageObject(CSWGuiImage* pImageObject)
{
    m_pImageObject = pImageObject;
    if (pImageObject) {
        SetImage(m_resRefImage, 1);
        SetAlignment(m_nAlignment);
        SetDrawStyle(bDrawStyle);
    }
}

void CSWGuiImageParams::SetImage(const CResRef& resRefImage, int bForce)
{
    if (resRefImage != m_resRefImage || bForce) {
        m_resRefImage = resRefImage;
        if (m_pImageObject) {
            if (m_pImageObject->m_pImage) {
                delete m_pImageObject->m_pImage;
                m_pImageObject->m_pImage = NULL;
            }
            if (m_resRefImage.IsValid()) {
                // 16-char ResRef plus the terminating NUL.
                char szImage[17];
                m_resRefImage.CopyToString(szImage);
                m_pImageObject->m_pImage = NewCAurGUIImage(szImage);
                if (m_pImageObject->m_pImage)
                    m_pImageObject->SetExtent(m_pImageObject->m_extent);
            }
        }
    }
}

void CSWGuiImageParams::SetAlignment(int nAlignment)
{
    m_nAlignment = nAlignment;
    if (m_pImageObject)
        m_pImageObject->SetExtent(m_pImageObject->m_extent);
}

void CSWGuiImageParams::SetDrawStyle(int bDrawStyleIn)
{
    bDrawStyle = bDrawStyleIn;
    if (m_pImageObject)
        m_pImageObject->SetExtent(m_pImageObject->m_extent);
}

CSWGuiImage::CSWGuiImage()
    // The defaults are no rotation, full alpha, white tint, centre/middle alignment.
    : m_params(CResRef(), 0, 0, 0.0f, 18, 1.0f, Vector{ 1.0f, 1.0f, 1.0f })
{
    m_pImage = NULL;
    m_params.SetImageObject(this);
}

CSWGuiImage::~CSWGuiImage()
{
    if (m_pImage) {
        delete m_pImage;
        m_pImage = NULL;
    }
}

void CSWGuiImage::Initialize(const CSWGuiExtent& extent, const CSWGuiImageParams& params)
{
    // The params assignment below re-derives the image extent.
    m_extent = extent;
    m_params = params;
}

CSWGuiImage& CSWGuiImage::operator=(CSWGuiImage& other)
{
    m_params = other.m_params;
    return *this;
}

CSWGuiExtent CSWGuiImage::GetImageExtent()
{
    CSWGuiExtent extent;
    if (m_pImage) {
        extent.w = m_pImage->GetImageWidth();
        extent.h = m_pImage->GetImageHeight();
    }
    return extent;
}

void CSWGuiImage::SetExtent(const CSWGuiExtent& extent)
{
    m_extent = extent;
    if (!m_pImage)
        return;

    if (!m_params.bDrawStyle) {
        // To stretch to fill, the texture's rectangle IS the widget's.
        m_imageExtent = m_extent;
        return;
    }

    // Natural size, aligned inside the widget's extent; height is queried first.
    m_imageExtent.h = m_pImage->GetImageHeight();
    m_imageExtent.w = m_pImage->GetImageWidth();

    // Written 1, 2, 4, the chain is re-expanded back to front into 4, 2, 1.
    if (m_extent.w > m_imageExtent.w) {
        int nAlign = m_params.m_nAlignment & CAurGUIStringInternal::kAlignHorizontalMask;
        if (nAlign == CAurGUIStringInternal::kAlignLeft)
            m_imageExtent.x = m_extent.x;
        else if (nAlign == CAurGUIStringInternal::kAlignCenter)
            m_imageExtent.x = m_extent.x + (m_extent.w - m_imageExtent.w) / 2;
        else if (nAlign == CAurGUIStringInternal::kAlignRight)
            m_imageExtent.x = m_extent.x - m_imageExtent.w + m_extent.w;
    } else {
        // The texture is wider than the slot, so clamp to it.
        m_imageExtent.w = m_extent.w;
        m_imageExtent.x = m_extent.x;
    }

    if (m_extent.h > m_imageExtent.h) {
        int nAlign = m_params.m_nAlignment & CAurGUIStringInternal::kAlignVerticalMask;
        if (nAlign == 8)
            m_imageExtent.y = m_extent.y;
        else if (nAlign == CAurGUIStringInternal::kAlignMiddle)
            m_imageExtent.y = m_extent.y + (m_extent.h - m_imageExtent.h) / 2;
        else if (nAlign == CAurGUIStringInternal::kAlignBottom)
            m_imageExtent.y = m_extent.y - m_imageExtent.h + m_extent.h;
        else
            return;
    } else {
        m_imageExtent.h = m_extent.h;
        m_imageExtent.y = m_extent.y;
    }
}

void CSWGuiImage::Load(CResGFF* pGFF, CResStruct& rStruct, const CExoString& sFieldName)
{
    CResStruct subStruct;
    if (!pGFF->GetStructFromStruct(&subStruct, &rStruct, static_cast<char*>(sFieldName.CStr())))
        return;

    int bSuccess;
    m_params.SetImage(pGFF->ReadFieldCResRef(&subStruct, const_cast<char*>("IMAGE"), bSuccess, CResRef("")), 0);
    m_params.SetDrawStyle(pGFF->ReadFieldINT(&subStruct, const_cast<char*>("DRAWSTYLE"), bSuccess, 0));
    // No setter; nothing downstream re-derives from this field.
    m_params.m_nFlipStyle = pGFF->ReadFieldINT(&subStruct, const_cast<char*>("FLIPSTYLE"), bSuccess, 0);
    m_params.SetAlignment(pGFF->ReadFieldINT(&subStruct, const_cast<char*>("ALIGNMENT"), bSuccess, 18));

    float fRotate = pGFF->ReadFieldFLOAT(&subStruct, const_cast<char*>("ROTATE"), bSuccess, 0.0f);
    if (!bSuccess) {
        // Older assets spell the rotation as a quarter-turn index instead.
        int nRotateStyle = pGFF->ReadFieldINT(&subStruct, const_cast<char*>("ROTATESTYLE"), bSuccess, 0);
        if (nRotateStyle >= 0 && nRotateStyle < 4)
            fRotate = s_afRotateStyle[nRotateStyle];
    }
    m_params.m_fRotate = fRotate;
}

void CSWGuiImage::Draw(float fAlpha)
{
    K2_UNUSED(fAlpha);
    // The alpha argument is unused; the drawn alpha comes from the params.
    if (!m_pImage)
        return;
    if (!m_extent.w)
        return;
    if (!m_extent.h)
        return;

    // Kept in a local so it is computed before the extent arguments.
    float fDrawAlpha = m_params.bHilight ? CSWGuiDrawable::m_fPulsingAlpha : m_params.m_fAlpha;
    m_pImage->Draw(m_imageExtent.x, m_imageExtent.y, m_imageExtent.w, m_imageExtent.h,
                   m_params.m_nFlipStyle, m_params.m_fRotate, m_params.m_color, fDrawAlpha);
}

// Aurora GUI viewport bracket, also declared by CSWGuiPanel.h; SetupViewport reports whether
// anything survived the clip.
int  AurGUISetupViewport(int x, int y, int w, int h, const Vector& v, bool b, float f);
void AurGUICloseViewport();

// Aurora object factories used by CSWGuiScene.
CAurScene*  NewCAurScene(char* pszName);
CAurCamera* NewCAurCamera(char* pszName);
#if KOTOR1  // K1: model streams are SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* p3, SDL_RWops* p4);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* p3, FILE* p4);
#endif

CSWGuiIndicatorParams::CSWGuiIndicatorParams(const CResRef& resRefCornerImage,
                                             const CResRef& resRefEdgeImage,
                                             const CResRef& resRefTerminatorImage,
                                             int nStartAnchor, int bStartTerminatorIn,
                                             int nEndAnchor, int bEndTerminatorIn, float fAlpha)
{
    m_resRefCornerImage     = resRefCornerImage;
    m_resRefEdgeImage       = resRefEdgeImage;
    m_resRefTerminatorImage = resRefTerminatorImage;
    m_nStartAnchor   = nStartAnchor;
    bStartTerminator = bStartTerminatorIn;
    m_nEndAnchor     = nEndAnchor;
    bEndTerminator   = bEndTerminatorIn;
    m_fAlpha         = fAlpha;
    m_pIndicator     = NULL;
}

CSWGuiIndicatorParams::CSWGuiIndicatorParams(const CSWGuiIndicatorParams& other)
{
    m_resRefCornerImage     = other.m_resRefCornerImage;
    m_resRefEdgeImage       = other.m_resRefEdgeImage;
    m_resRefTerminatorImage = other.m_resRefTerminatorImage;
    m_nStartAnchor   = other.m_nStartAnchor;
    bStartTerminator = other.bStartTerminator;
    m_nEndAnchor     = other.m_nEndAnchor;
    bEndTerminator   = other.bEndTerminator;
    m_fAlpha         = other.m_fAlpha;
    // A copied block is not bound to anything; the widget binds it, not the other way round.
    m_pIndicator = NULL;
}

CSWGuiIndicatorParams& CSWGuiIndicatorParams::operator=(const CSWGuiIndicatorParams& other)
{
    m_resRefCornerImage     = other.m_resRefCornerImage;
    m_resRefEdgeImage       = other.m_resRefEdgeImage;
    m_resRefTerminatorImage = other.m_resRefTerminatorImage;
    m_nStartAnchor   = other.m_nStartAnchor;
    bStartTerminator = other.bStartTerminator;
    m_nEndAnchor     = other.m_nEndAnchor;
    bEndTerminator   = other.bEndTerminator;
    m_fAlpha         = other.m_fAlpha;
    // Rebind to self, pushing the copied textures onto the bound widget.
    SetIndicator(m_pIndicator);
    return *this;
}

void CSWGuiIndicatorParams::SetIndicator(CSWGuiIndicator* pIndicator)
{
    m_pIndicator = pIndicator;
    if (pIndicator) {
        SetCornerImage(m_resRefCornerImage, 1);
        SetEdgeImage(m_resRefEdgeImage, 1);
        SetTerminatorImage(m_resRefTerminatorImage, 1);
    }
}

void CSWGuiIndicatorParams::SetCornerImage(const CResRef& resRefCornerImage, int bForce)
{
    if (resRefCornerImage != m_resRefCornerImage || bForce) {
        m_resRefCornerImage = resRefCornerImage;
        if (m_pIndicator) {
            if (m_pIndicator->m_pCornerImage) {
                delete m_pIndicator->m_pCornerImage;
                m_pIndicator->m_pCornerImage = NULL;
            }
            if (m_resRefCornerImage.IsValid()) {
                // 16-char ResRef plus the terminating NUL.
                char szImage[17];
                m_resRefCornerImage.CopyToString(szImage);
                m_pIndicator->m_pCornerImage = NewCAurGUIImage(szImage);
            }
        }
    }
}

void CSWGuiIndicatorParams::SetEdgeImage(const CResRef& resRefEdgeImage, int bForce)
{
    if (resRefEdgeImage != m_resRefEdgeImage || bForce) {
        m_resRefEdgeImage = resRefEdgeImage;
        if (m_pIndicator) {
            if (m_pIndicator->m_pEdgeImage) {
                delete m_pIndicator->m_pEdgeImage;
                m_pIndicator->m_pEdgeImage = NULL;
            }
            if (m_resRefEdgeImage.IsValid()) {
                char szImage[17];
                m_resRefEdgeImage.CopyToString(szImage);
                m_pIndicator->m_pEdgeImage = NewCAurGUIImage(szImage);
            }
        }
    }
}

void CSWGuiIndicatorParams::SetTerminatorImage(const CResRef& resRefTerminatorImage, int bForce)
{
    if (resRefTerminatorImage != m_resRefTerminatorImage || bForce) {
        m_resRefTerminatorImage = resRefTerminatorImage;
        if (m_pIndicator) {
            if (m_pIndicator->m_pTerminatorImage) {
                delete m_pIndicator->m_pTerminatorImage;
                m_pIndicator->m_pTerminatorImage = NULL;
            }
            if (m_resRefTerminatorImage.IsValid()) {
                char szImage[17];
                m_resRefTerminatorImage.CopyToString(szImage);
                m_pIndicator->m_pTerminatorImage = NewCAurGUIImage(szImage);
            }
        }
    }
}

CSWGuiIndicator::CSWGuiIndicator()
    // The defaults are both anchors at the top-left, both terminators on, full alpha, no textures.
    : m_params(CResRef(), CResRef(), CResRef(), 0, 1, 0, 1, 1.0f)
{
    m_pEdgeImage       = NULL;
    m_pCornerImage     = NULL;
    m_pTerminatorImage = NULL;
    m_params.SetIndicator(this);
}

CSWGuiIndicator::~CSWGuiIndicator()
{
    // Corner before edge; not declaration order.
    if (m_pCornerImage) {
        delete m_pCornerImage;
        m_pCornerImage = NULL;
    }
    if (m_pEdgeImage) {
        delete m_pEdgeImage;
        m_pEdgeImage = NULL;
    }
    if (m_pTerminatorImage) {
        delete m_pTerminatorImage;
        m_pTerminatorImage = NULL;
    }
}

void CSWGuiIndicator::Initialize(const CSWGuiExtent& extent, const CSWGuiIndicatorParams& params)
{
    m_extent = extent;
    m_params = params;
}

CSWGuiIndicator& CSWGuiIndicator::operator=(CSWGuiIndicator& other)
{
    m_params = other.m_params;
    return *this;
}

void CSWGuiIndicator::SetExtent(const CSWGuiExtent& extent)
{
    m_extent = extent;
}

void CSWGuiIndicator::Draw(float fAlpha)
{
    K2_UNUSED(fAlpha);
    // Corner and edge must both exist and be the same square size.
    if (m_extent.w == 0)
        return;
    if (m_extent.h == 0)
        return;
    if (!m_pCornerImage)
        return;
    if (!m_pEdgeImage)
        return;
    if (m_pEdgeImage->GetImageHeight() != m_pEdgeImage->GetImageWidth())
        return;
    if (m_pCornerImage->GetImageHeight() != m_pCornerImage->GetImageWidth())
        return;
    if (m_pCornerImage->GetImageWidth() != m_pEdgeImage->GetImageWidth())
        return;
    // The terminator is only checked when one is actually drawn.
    if (m_pTerminatorImage
        && (m_pTerminatorImage->GetImageHeight() != m_pTerminatorImage->GetImageWidth()
            || m_pTerminatorImage->GetImageWidth() != m_pCornerImage->GetImageWidth())
        && (m_params.bStartTerminator || m_params.bEndTerminator))
        return;

    // Anchor agreement picks the horizontal run, the vertical run, or the elbow.
    if (m_params.m_nStartAnchor != m_params.m_nEndAnchor) {
        int nAxes = m_params.m_nStartAnchor ^ m_params.m_nEndAnchor;
        if (!(nAxes & 1))
            DrawStraightHorzLine();
        else if (!(nAxes & 2))
            DrawStraightVertLine();
        else
            DrawJointedLine();
    }

    CAurGUIImage::FlushBuffer(CAurGUIImage::cm_fAlpha);
}

void CSWGuiIndicator::DrawStraightHorzLine()
{
    int nLength = m_extent.w;
    int nEdgeWidth  = m_pEdgeImage->GetImageWidth();
    int nEdgeHeight = m_pEdgeImage->GetImageHeight();
    float fAlpha = m_params.m_fAlpha;

    // The flags are named by the anchors but drawn by screen position, so they swap when the
    // start anchor is the right-hand end.
    int bLeftTerminator  = 0;
    int bRightTerminator = 0;
    int nStartAnchor = m_params.m_nStartAnchor;
    if (nStartAnchor & 2) {
        if (m_pTerminatorImage) {
            bLeftTerminator  = m_params.bEndTerminator;
            bRightTerminator = m_params.bStartTerminator;
        }
    } else {
        if (m_pTerminatorImage) {
            bLeftTerminator  = m_params.bStartTerminator;
            bRightTerminator = m_params.bEndTerminator;
        }
    }

    // Anchor bit 0 puts the run along the bottom edge instead of the top.
    int nHeight = (nEdgeHeight > m_extent.h) ? m_extent.h : nEdgeHeight;
    int nY = m_extent.y;
    if (nStartAnchor & 1) {
        int nBottom = m_extent.y - nEdgeHeight + m_extent.h;
        if (nBottom > nY)
            nY = nBottom;
    }

    // Whatever the terminators occupy is not available to the tiled run.
    int nLeftInset  = bLeftTerminator  ? nEdgeWidth : 0;
    int nRightInset = bRightTerminator ? nEdgeWidth : 0;
    int nSpan = nLength - nLeftInset - nRightInset;

    // nEdgeWidth comes from a virtual call, so the quotient can be negative; the loop keeps its
    // own entry guard.
    int nTiles = 0;
    int nUsed  = 0;
    if (nSpan >= 1) {
        nTiles = nSpan / nEdgeWidth;
        if (nTiles) {
            nUsed = nTiles * nEdgeWidth;
        } else if (nSpan >= nEdgeWidth / 2) {
            // Not quite a whole tile, but more than half, so round up to one stretched tile rather
            // than dropping to the degenerate case below.
            nTiles = 1;
            nUsed  = nEdgeWidth;
        }
    }

    if (nTiles) {
        if (bLeftTerminator)
            m_pTerminatorImage->Draw(m_extent.x, nY, nEdgeWidth, nHeight, 0, 270.0f,
                                     CSWGuiDrawable::NO_COLOURING, fAlpha);
        if (bRightTerminator)
            m_pTerminatorImage->Draw(m_extent.x - nEdgeWidth + m_extent.w, nY, nEdgeWidth, nHeight,
                                     0, 90.0f, CSWGuiDrawable::NO_COLOURING, fAlpha);

        // Spread the leftover pixels evenly. Every tile grows by nExtra, and the first nRemainder
        // of them take one more each so the run ends exactly on the inset.
        int nSlack     = nSpan - nUsed;
        int nExtra     = nSlack / nTiles;
        int nRemainder = nSlack % nTiles;
        int nTileWidth = nEdgeWidth + nExtra;
        int nX = m_extent.x + nLeftInset;
        for (int i = 0; i < nTiles; i++) {
            int nThisWidth = nTileWidth;
            if (nRemainder) {
                nThisWidth++;
                nRemainder--;
            }
            m_pEdgeImage->DrawBuffered(nX, nY, nThisWidth, nHeight, 0, 0.0f,
                                       CSWGuiDrawable::NO_COLOURING, fAlpha);
            nX += nThisWidth;
        }
    } else {
        // Too narrow for even half a tile, so there is no edge run at all, just the two terminators
        // splitting the extent down the middle.
        // The right-hand cap is drawn at nHalf without the extent offset and unrotated; two
        // quirks of the engine, kept.
        int nHalf = nLength / 2;
        if (bLeftTerminator)
            m_pTerminatorImage->Draw(m_extent.x, nY, nHalf, nHeight, 0, 270.0f,
                                     CSWGuiDrawable::NO_COLOURING, fAlpha);
        if (bRightTerminator)
            m_pTerminatorImage->Draw(nHalf, nY, nHalf + (m_extent.w & 1), nHeight, 0, 0.0f,
                                     CSWGuiDrawable::NO_COLOURING, fAlpha);
    }
}

void CSWGuiIndicator::DrawStraightVertLine()
{
    int nLength = m_extent.h;
    int nEdgeWidth  = m_pEdgeImage->GetImageWidth();
    int nEdgeHeight = m_pEdgeImage->GetImageHeight();
    float fAlpha = m_params.m_fAlpha;

    // The flags are named by the anchors but drawn by screen position, so they swap when the
    // start anchor is the bottom end.
    int bTopTerminator    = 0;
    int bBottomTerminator = 0;
    int nStartAnchor = m_params.m_nStartAnchor;
    if (nStartAnchor & 1) {
        if (m_pTerminatorImage) {
            bTopTerminator    = m_params.bEndTerminator;
            bBottomTerminator = m_params.bStartTerminator;
        }
    } else {
        if (m_pTerminatorImage) {
            bTopTerminator    = m_params.bStartTerminator;
            bBottomTerminator = m_params.bEndTerminator;
        }
    }

    // Anchor bit 1 puts the run along the right edge instead of the left.
    //
    // nX is computed before nWidth, the opposite of the horizontal renderer; the reverse order
    // merges the two position blocks.
    int nX = m_extent.x;
    if (nStartAnchor & 2) {
        int nRight = m_extent.x - nEdgeWidth + m_extent.w;
        if (nRight > nX)
            nX = nRight;
    }
    int nWidth = (nEdgeWidth > m_extent.w) ? m_extent.w : nEdgeWidth;

    int nTopInset    = bTopTerminator    ? nEdgeHeight : 0;
    int nBottomInset = bBottomTerminator ? nEdgeHeight : 0;
    int nSpan = nLength - nTopInset - nBottomInset;

    int nTiles = 0;
    int nUsed  = 0;
    if (nSpan >= 1) {
        nTiles = nSpan / nEdgeHeight;
        if (nTiles) {
            nUsed = nTiles * nEdgeHeight;
        } else if (nSpan >= nEdgeHeight / 2) {
            nTiles = 1;
            nUsed  = nEdgeHeight;
        }
    }

    if (nTiles) {
        if (bTopTerminator)
            m_pTerminatorImage->Draw(nX, m_extent.y, nWidth, nEdgeHeight, 0, 180.0f,
                                     CSWGuiDrawable::NO_COLOURING, fAlpha);
        if (bBottomTerminator)
            m_pTerminatorImage->Draw(nX, m_extent.y - nEdgeHeight + m_extent.h, nWidth, nEdgeHeight,
                                     0, 0.0f, CSWGuiDrawable::NO_COLOURING, fAlpha);

        int nSlack      = nSpan - nUsed;
        int nExtra      = nSlack / nTiles;
        int nRemainder  = nSlack % nTiles;
        int nTileHeight = nEdgeHeight + nExtra;
        int nY = m_extent.y + nTopInset;
        for (int i = 0; i < nTiles; i++) {
            int nThisHeight = nTileHeight;
            if (nRemainder) {
                nThisHeight++;
                nRemainder--;
            }
            m_pEdgeImage->DrawBuffered(nX, nY, nWidth, nThisHeight, 0, 270.0f,
                                       CSWGuiDrawable::NO_COLOURING, fAlpha);
            nY += nThisHeight;
        }
    } else {
        // The same two quirks as the horizontal renderer, mirrored on the bottom cap.
        int nHalf = m_extent.h / 2;
        if (bTopTerminator)
            m_pTerminatorImage->Draw(nX, m_extent.y, nWidth, nHalf, 0, 180.0f,
                                     CSWGuiDrawable::NO_COLOURING, fAlpha);
        if (bBottomTerminator)
            m_pTerminatorImage->Draw(nX, nHalf, nWidth, nHalf + (m_extent.h & 1), 0, 0.0f,
                                     CSWGuiDrawable::NO_COLOURING, fAlpha);
    }
}

void CSWGuiIndicator::DrawJointedLine()
{
    // The terminator flags are read raw, not anchor-relative; the switch below places each cap.
    int nStartAnchor = m_params.m_nStartAnchor;
    int bStartTerminator = 0;
    int bEndTerminator   = 0;
    if (m_pTerminatorImage) {
        bStartTerminator = m_params.bStartTerminator;
        bEndTerminator   = m_params.bEndTerminator;
    }

    float fAlpha = m_params.m_fAlpha;
    int nEdgeWidth  = m_pEdgeImage->GetImageWidth();
    int nEdgeHeight = m_pEdgeImage->GetImageHeight();

    // A jointed line always stamps the corner, so each run gives up one edge dimension for it
    // unconditionally; the conditional subtraction is that run's own terminator.
    // The terminator select leads the addition; do not reorder the operands.
    int nHorzSpan = (bStartTerminator ? -nEdgeWidth  : 0) + m_extent.w - nEdgeWidth;
    int nVertSpan = (bEndTerminator   ? -nEdgeHeight : 0) + m_extent.h - nEdgeHeight;

    // Same tile fit as the straight renderers, but the leftover is recomputed in the drawing
    // tail.
    int nHorzTiles = 0;
    int nHorzUsed  = 0;
    if (nHorzSpan >= 1) {
        nHorzTiles = nHorzSpan / nEdgeWidth;
        if (nHorzTiles)
            nHorzUsed = nHorzTiles * nEdgeWidth;
        else if (nHorzSpan >= nEdgeWidth / 2) {
            nHorzTiles = 1;
            nHorzUsed  = nEdgeWidth;
        }
    }
    // Defaults to the span, not to zero.
    int nHorzSlack = nHorzSpan;
    int nHorzExtra = 0;
    int bHorzRun   = 0;
    if (nHorzTiles) {
        nHorzSlack = nHorzSpan - nHorzUsed;
        nHorzExtra = nHorzSlack / nHorzTiles;
        bHorzRun   = 1;
    }

    int nVertTiles = 0;
    int nVertUsed  = 0;
    if (nVertSpan >= 1) {
        nVertTiles = nVertSpan / nEdgeHeight;
        if (nVertTiles)
            nVertUsed = nVertTiles * nEdgeHeight;
        else if (nVertSpan >= nEdgeHeight / 2) {
            nVertTiles = 1;
            nVertUsed  = nEdgeHeight;
        }
    }
    int nVertSlack = nVertSpan;
    int nVertExtra = 0;
    int bVertRun   = 0;
    if (nVertTiles) {
        nVertSlack = nVertSpan - nVertUsed;
        nVertExtra = nVertSlack / nVertTiles;
        bVertRun   = 1;
    }

    // The corner shares one axis with each run, which is why four coordinates serve five stamps.
    //
    // Left uninitialized. The switch covers anchors 0-3 and the field is six bits wide, so any
    // other anchor reads them unset (K2_FIX_ENGINE_BUGS draws nothing instead).
    int nHorzX, nHorzY, nHorzH;
    int nVertX, nVertY, nVertW;
    int nStartX, nStartY, nStartW, nStartH;
    int nEndX, nEndY, nEndW, nEndH;
    float fHorzRotate, fCornerRotate, fVertRotate;

    switch (nStartAnchor) {
    case 0:
        // A top-left start runs along the top, elbows at the top-right and goes down the right
        // edge.
        nVertX  = m_extent.x;
        nHorzY  = m_extent.y;
        nVertW  = m_extent.w;
        nHorzH  = m_extent.h;
        nStartW = 0;
        nEndY   = 0;
        nEndH   = 0;
        if (bHorzRun) {
            nVertX  = m_extent.x - nEdgeWidth + m_extent.w;
            nVertW  = nEdgeWidth;
            nStartW = nEdgeWidth;
        } else if (bStartTerminator) {
            // With no room to tile, the cap and the corner split the extent between them, the same
            // shape the straight renderers fall back to.
            nStartW = m_extent.w / 2;
            nVertW  = m_extent.w - nStartW;
            nVertX  = m_extent.x + nStartW;
        }

        if (bVertRun) {
            nEndY  = m_extent.y - nEdgeHeight + m_extent.h;
            nHorzH = nEdgeHeight;
            nEndH  = nEdgeHeight;
        } else if (bEndTerminator) {
            nEndH  = m_extent.h / 2;
            nHorzH = m_extent.h - nEndH;
            nEndY  = m_extent.y + nHorzH;
        }

        nHorzX  = m_extent.x + (bStartTerminator ? nEdgeWidth : 0);
        nVertY  = m_extent.y + nEdgeHeight;
        nStartX = m_extent.x;
        nStartY = m_extent.y;
        nStartH = nHorzH;
        nEndX   = nVertX;
        nEndW   = nVertW;

        fHorzRotate   = 0.0f;
        fCornerRotate = 270.0f;
        fVertRotate   = 270.0f;
        break;

    case 1:
        // A bottom-left start runs along the bottom, elbows at the bottom-right and goes up the
        // right edge.
        nVertX  = 0;
        nHorzY  = m_extent.y;
        nVertW  = m_extent.w;
        nHorzH  = m_extent.h;
        nStartW = 0;
        nEndH   = 0;
        if (bHorzRun) {
            nVertX  = m_extent.x - nEdgeWidth + m_extent.w;
            nVertW  = nEdgeWidth;
            nStartW = nEdgeWidth;
        } else if (bStartTerminator) {
            nStartW = m_extent.w / 2;
            nVertW  = m_extent.w - nStartW;
            nVertX  = m_extent.x + nStartW;
        }

        if (bVertRun) {
            nHorzY = m_extent.y - nEdgeHeight + m_extent.h;
            nHorzH = nEdgeHeight;
            nEndH  = nEdgeHeight;
        } else if (bEndTerminator) {
            nEndH  = m_extent.h / 2;
            nHorzH = m_extent.h - nEndH;
            nHorzY = m_extent.y + nEndH;
        }

        nStartX = m_extent.x;
        nHorzX  = m_extent.x + (bStartTerminator ? nEdgeWidth : 0);
        nStartY = nHorzY;
        nEndY   = m_extent.y;
        nVertY  = m_extent.y + (bEndTerminator ? nEdgeHeight : 0);
        nStartH = nHorzH;
        nEndW   = nVertW;
        nEndX   = nVertX;

        fHorzRotate   = 180.0f;
        fCornerRotate = 180.0f;
        fVertRotate   = 270.0f;
        break;

    case 2:
        // A top-right start runs leftwards along the top, elbows at the top-left and goes down the
        // left edge. The corner owns the left end of the extent, so the run origin is pinned to it
        // rather than inset by the cap.
        nVertX  = m_extent.x;
        nHorzY  = m_extent.y;
        nVertW  = m_extent.w;
        nHorzH  = m_extent.h;
        nStartW = 0;
        nStartX = 0;
        nEndY   = 0;
        nStartH = 0;
        if (bHorzRun) {
            nStartX = m_extent.x - nEdgeWidth + m_extent.w;
            nVertW  = nEdgeWidth;
            nStartW = nEdgeWidth;
        } else if (bStartTerminator) {
            nStartW = m_extent.w / 2;
            nVertW  = m_extent.w - nStartW;
            nStartX = m_extent.x + nVertW;
        }

        if (bVertRun) {
            nEndY   = m_extent.y - nEdgeHeight + m_extent.h;
            nHorzH  = nEdgeHeight;
            nStartH = nEdgeHeight;
        } else if (bEndTerminator) {
            // nStartH keeps its default here.
            nHorzH = m_extent.h - m_extent.h / 2;
            nEndY  = m_extent.y + nHorzH;
        }

        nHorzX  = m_extent.x + nEdgeWidth;
        nVertY  = m_extent.y + nEdgeHeight;
        nStartY = m_extent.y;
        nEndX   = m_extent.x;
        nEndW   = nVertW;
        nEndH   = nHorzH;

        fHorzRotate   = 0.0f;
        fCornerRotate = 0.0f;
        fVertRotate   = 90.0f;
        break;

    case 3:
        // Start cap at the top-left and end cap at the bottom-right, which are anchor 0's
        // endpoints; the rest of the arm is consistent with that reading, including the vertical
        // run being inset by bStartTerminator.
        nVertX  = m_extent.x;
        nHorzY  = m_extent.y;
        nVertW  = m_extent.w;
        nHorzH  = m_extent.h;
        nEndW   = 0;
        nEndX   = 0;
        nStartH = 0;
        if (bHorzRun) {
            nEndX  = m_extent.x - nEdgeWidth + m_extent.w;
            nEndW  = nEdgeWidth;
            nVertW = nEdgeWidth;
        } else if (bEndTerminator) {
            nEndW  = m_extent.w / 2;
            nVertW = m_extent.w - nEndW;
            nEndX  = m_extent.x + nVertW;
        }

        if (bVertRun) {
            nHorzY  = m_extent.y - nEdgeHeight + m_extent.h;
            nHorzH  = nEdgeHeight;
            nStartH = nEdgeHeight;
        } else if (bStartTerminator) {
            nStartH = m_extent.h / 2;
            nHorzH  = m_extent.h - nStartH;
            nHorzY  = m_extent.y + nStartH;
        }

        nHorzX  = m_extent.x + nEdgeWidth;
        nVertY  = m_extent.y + (bStartTerminator ? nEdgeHeight : 0);
        nStartX = m_extent.x;
        nStartY = m_extent.y;
        nStartW = nVertW;
        nEndY   = nHorzY;
        nEndH   = nHorzH;

        fHorzRotate   = 180.0f;
        fCornerRotate = 90.0f;
        fVertRotate   = 90.0f;
        break;
#ifdef K2_FIX_ENGINE_BUGS
    default:
        return;
#endif
    }

    // Spread the leftover pixels the same way the straight renderers do: every tile grows by the
    // extra, and the first `remainder` of them take one more each.
    if (nHorzTiles) {
        int nRemainder = nHorzSlack - nHorzExtra * nHorzTiles;
        int nTileWidth = nHorzExtra + nEdgeWidth;
        for (int i = 0; i < nHorzTiles; i++) {
            int nThisWidth = nTileWidth;
            if (nRemainder) {
                nThisWidth++;
                nRemainder--;
            }
            m_pEdgeImage->DrawBuffered(nHorzX, nHorzY, nThisWidth, nHorzH, 0, fHorzRotate,
                                       CSWGuiDrawable::NO_COLOURING, fAlpha);
            nHorzX += nThisWidth;
        }
    }

    if (nVertTiles) {
        int nRemainder  = nVertSlack - nVertExtra * nVertTiles;
        int nTileHeight = nVertExtra + nEdgeHeight;
        for (int i = 0; i < nVertTiles; i++) {
            int nThisHeight = nTileHeight;
            if (nRemainder) {
                nThisHeight++;
                nRemainder--;
            }
            m_pEdgeImage->DrawBuffered(nVertX, nVertY, nVertW, nThisHeight, 0, fVertRotate,
                                       CSWGuiDrawable::NO_COLOURING, fAlpha);
            nVertY += nThisHeight;
        }
    }

    m_pCornerImage->DrawBuffered(nVertX, nHorzY, nVertW, nHorzH, 0, fCornerRotate,
                                 CSWGuiDrawable::NO_COLOURING, fAlpha);

    // The caps borrow the other run's rotation; the start cap points the way the vertical tiles do,
    // the end cap the way the horizontal ones do.
    if (bStartTerminator)
        m_pTerminatorImage->DrawBuffered(nStartX, nStartY, nStartW, nStartH, 0, fVertRotate,
                                         CSWGuiDrawable::NO_COLOURING, fAlpha);
    if (bEndTerminator)
        m_pTerminatorImage->DrawBuffered(nEndX, nEndY, nEndW, nEndH, 0, fHorzRotate,
                                         CSWGuiDrawable::NO_COLOURING, fAlpha);
}

CSWGuiScene::CSWGuiScene()
    : m_color()
{
    m_pScene  = NewCAurScene(const_cast<char*>("scene"));
    m_pCamera = NewCAurCamera(const_cast<char*>("camera"));

    if (m_pCamera && m_pScene)
    {
        // A 10x10 BSP over a 10-unit cell, with visibility on; the widget's scene is tiny, so the
        // partition exists only to keep the renderer's bookkeeping happy.
        m_pScene->InitBSP(10, 10, 5, true, 10.0f);
        m_pScene->DisableVisibilityGraph();
        m_pScene->SetFog(0);
        m_pCamera->SetClipDist(0.1f, 10000.0f);
        m_pCamera->AttachToScene(m_pScene);
        m_color = CSWGuiDrawable::NO_COLOURING;
    }
}

CSWGuiScene::~CSWGuiScene()
{
    for (int i = 0; i < m_aModels.m_count; i++)
    {
        CAurObject* pObject = m_aModels.m_pArray[i];
        if (pObject)
            delete pObject;
    }
    m_aModels.m_count = 0;

    if (m_pCamera)
    {
        delete m_pCamera;
        m_pCamera = NULL;
    }
    if (m_pScene)
    {
        delete m_pScene;
        m_pScene = NULL;
    }

    if (g_pExoResMan->bMainMenuImageFileLoaded)
    {
        g_pExoResMan->RemoveResourceImageFile(CExoString("RIMS:MAINMENU"));
        g_pExoResMan->bMainMenuImageFileLoaded = 0;
    }
    if (g_pExoResMan->bChargenImageFileLoaded)
    {
        g_pExoResMan->RemoveResourceImageFile(CExoString("RIMS:CHARGEN"));
        g_pExoResMan->bChargenImageFileLoaded = 0;
    }
}

CAurObject* CSWGuiScene::AddModel(const CExoString& sModelName, int nIndex)
{
    CAurObject* pObject = NewCAurObject(static_cast<char*>(sModelName.CStr()), const_cast<char*>(""), 0, 0);
    return AddModel(pObject, nIndex, 1) ? pObject : NULL;
}

int CSWGuiScene::AddModel(CAurObject* pObject, int nIndex, int bAttachToScene)
{
    if (!pObject)
        return 0;

    // An in-range index means "put it exactly here" (draw order is list order); anything else
    // appends.
    if (nIndex >= 0 && nIndex < m_aModels.m_count)
        m_aModels.Insert(pObject, nIndex);
    else
        m_aModels.Add(pObject);

    if (bAttachToScene)
        pObject->AttachToScene(m_pScene);

    return 1;
}

int CSWGuiScene::RemoveModel(int nIndex, int bDelete)
{
    if (nIndex < 0 || nIndex >= m_aModels.m_count)
        return 0;

    CAurObject* pObject = m_aModels.m_pArray[nIndex];
    m_aModels.DelIndex(nIndex);

    if (bDelete && pObject)
        delete pObject;

    return 1;
}

CAurObject* CSWGuiScene::GetModel(int nIndex)
{
    if (nIndex < 0 || nIndex >= m_aModels.m_count)
        return NULL;

    return m_aModels.m_pArray[nIndex];
}

void CSWGuiScene::Draw(float fDelta)
{
    m_pScene->Animate(fDelta);

    if (AurGUISetupViewport(m_extent.x, m_extent.y, m_extent.w, m_extent.h, m_color, true, 1.0f))
    {
        m_pCamera->RenderScene();
        AurGUICloseViewport();
    }
}

float CSWGuiDrawable::m_fAlphaPulseTimer = 0.75f;
float CSWGuiDrawable::m_fPulsingAlpha = 1.0f;
int   CSWGuiDrawable::m_bAlphaPulsingUp;

const Vector CSWGuiDrawable::NO_COLOURING(-1.0f, -1.0f, -1.0f);

void CSWGuiDrawable::SetExtent(const CSWGuiExtent& extent)
{
    m_extent = extent;
}

int CSWGuiDrawable::GetIdealHeight()
{
    return m_extent.h;
}

void CSWGuiDrawable::UpdatePulsing(float fDeltaTime)
{
    m_fAlphaPulseTimer -= fDeltaTime;
    if (m_fAlphaPulseTimer <= 0.0f)
    {
        m_bAlphaPulsingUp = (m_bAlphaPulsingUp == 0);
        m_fAlphaPulseTimer = 0.75f;
    }

    float fAlpha = (m_bAlphaPulsingUp == 0) ? 1.0f : 0.4f;
    float fRange = (m_bAlphaPulsingUp == 0) ? -0.6f : 0.6f;
    m_fPulsingAlpha = fAlpha + fRange * ((0.75f - m_fAlphaPulseTimer) / 0.75f);
}

