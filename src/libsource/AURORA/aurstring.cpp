// StringGob: the text object shared by the console, the chat box and floating text.
//
// Almost every method here is a stub: a bare return, or one small constant, is the whole function.
// The text engine behind them was cut and the class survives so its call sites link.

#include "k2stdint.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library; List.h grows from 8
#endif
#include "aurstring.h"
#include "aurStringFont.h"

#if KOTOR1
#include <stdio.h>
#include <string.h>
#include "aurfont.h"
#include "aurglcore.h"
#include "aurtexman.h"
#include "aurbehavior.h"
#include "port/glconst.h"
#include "math3d.h"
#endif
#include "port/libcstring.h"
#include "port/libcmath.h"

class CAurObject;
class CAurPart;
class CHitInfo;
class CAurString;

char g_szNextStringName[256] = "string0";

#if KOTOR1
int g_nStringCount;
int nostrings;
extern int quaternionidentityoptimization;
extern unsigned int enableTranslationTable[];
extern CStringColor g_cSavedCol;
int AurMultiTextureAvailable();
extern "C" {
void android_port_glPushMatrix();
void android_port_glPopMatrix();
void android_port_glTranslatef(float x, float y, float z);
void android_port_glRotatef(float angle, float x, float y, float z);
void android_port_glActiveTexture(unsigned int texture);
void android_port_glGetIntegerv(unsigned int pname, int* params);
void android_port_glEnable(unsigned int cap);
void android_port_glDisable(unsigned int cap);
void android_port_glColor4f(float r, float g, float b, float a);
void android_port_glColor4fv(const float* v);
void android_port_glPushAttrib(unsigned int mask);
void android_port_glPopAttrib();
}

// The same quaternion rotate as Part.cpp.
static inline Vector RotateVector(const Quaternion& q, const Vector& v)
{
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        Vector r;
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        r.x = v.x * m00 + v.y * m01 + v.z * m02;
        r.y = v.x * m10 + v.y * m11 + v.z * m12;
        r.z = v.x * m20 + v.y * m21 + v.z * m22;
        return r;
    }
    return v;
}
#endif


#if KOTOR1
StringGob::StringGob(int a)
    : Gob(g_szNextStringName), m_caret(const_cast<char*>("[")), m_lines(), m_colors()
{
    sprintf(g_szNextStringName, "string%d", ++g_nStringCount);
    m_bTextWrapping = a != 1;
    if (CAurFont::s_LoadedFontList.m_count != 0) {
        m_pFont = CAurFont::s_LoadedFontList[0];
        CAurFont::s_LoadedFontList[0]->m_nRefCount++;
    } else {
        m_pFont = 0;
    }
    m_colors.SetSize(1);
    m_displayInfo.nFirstLine = 0;
    m_displayInfo.nNumLines = 0;
    m_fAlignment = 0.0f;
    m_fAlignmentVertical = 0.0f;
    m_nMaxLines = a;
    m_displayInfo.m_fHitMargin = 0.0f;
    m_displayInfo.m_fMaxWidth = 0.0f;
    m_displayInfo.m_fMaxHeight = -0.00001f;
    m_bCaretVisible = false;
    m_displayInfo.m_fVisibleHeight = 0.0f;
    m_displayInfo.m_fScrollOffset = 0.0f;
    m_displayInfo.m_bScrollMode = false;
    if (a > 0)
        m_lines.allocate(a);
    if (m_nMaxLines < 0)
        m_nMaxLines = 1048576;
}
#else
StringGob::StringGob(int a)
    : Gob(g_szNextStringName), m_caret(const_cast<char*>("[")), m_lines(), m_colors()
{
    K2_UNUSED(a);
}
#endif

#if KOTOR1
StringGob::~StringGob()
{
    DeleteText();
    if (m_pFont) {
        m_pFont->m_nRefCount--;
        m_pFont = 0;
    }
}
#else
StringGob::~StringGob()
{
    // Empty: both arrays are member objects and free themselves (see StringGob.h).
}
#endif


#if KOTOR1
void StringGob::Render(bool a)
{
    if (nostrings)
        return;
    android_port_glPushMatrix();
    android_port_glTranslatef(m_vPosition.x, m_vPosition.y, m_vPosition.z);
    float fAngle = acosf(m_qOrientation.w) + acosf(m_qOrientation.w);
    Vector vAxis;
    if ((fAngle < 0.0f ? -fAngle : fAngle) < 0.0000001) {
        vAxis = Vector(1.0f, 0.0f, 0.0f);
    } else {
        vAxis = Vector(m_qOrientation.x, m_qOrientation.y, m_qOrientation.z);
        vAxis = vAxis * static_cast<float>(1.0 / sin(fAngle * 0.5));
    }
    android_port_glRotatef(fAngle * (180.0f / 3.14159265f), vAxis.x, vAxis.y, vAxis.z);
    GLRender::cm_uiCurrentStage = 0;
    if (AurMultiTextureAvailable())
        android_port_glActiveTexture(GLRender::cm_uiCurrentStage + GL_TEXTURE0);
    m_pFont->m_pTexture.m_pObject->Bind();
    int bLighting;
    android_port_glGetIntegerv(GL_LIGHTING, &bLighting);
    android_port_glDisable(enableTranslationTable[5]);
    *reinterpret_cast<SStringGobColor*>(&g_cSavedCol) = m_colors[0];
    android_port_glColor4fv(&g_cSavedCol.r);
    android_port_glTranslatef(0.0f,
        m_displayInfo.m_fScrollOffset - (m_displayInfo.m_fMaxHeight - m_displayInfo.m_fVisibleHeight) * m_fAlignmentVertical + 0.001f, 0.0f);
    float fY = 0.0f;
    int i = m_displayInfo.nFirstLine;
    if (m_displayInfo.nNumLines >= 1) {
        float fX = 0.0f;
        int nColor = 0;
        for (int n = 0; n < m_displayInfo.nNumLines && i < m_lines.m_count; n++, i++) {
            CStringItem* pItem = m_lines[i];
            // snap the line's horizontal alignment offset to whole pixels
            float fDX = (static_cast<float>(static_cast<int>(((m_displayInfo.m_fMaxWidth - pItem->m_fVisibleWidth) * m_fAlignment - fX)
                                      / 0.01f + 1000000.0f + 0.5f + 0.01f))
                         - 1000000.0f) * 0.01f;
            android_port_glTranslatef(fDX, -pItem->m_fTop, 0.0f);
            if (pItem->m_pAttached) {
                if (bLighting)
                    android_port_glEnable(GL_LIGHTING);
                else
                    android_port_glDisable(GL_LIGHTING);
                pItem->m_pAttached->Render(false);
                android_port_glDisable(enableTranslationTable[5]);
                m_pFont->m_pTexture.m_pObject->Bind();
            }
            if (pItem->m_nColor != nColor) {
                *reinterpret_cast<SStringGobColor*>(&g_cSavedCol) = m_colors[pItem->m_nColor];
                android_port_glColor4fv(&g_cSavedCol.r);
                nColor = pItem->m_nColor;
            }
            m_pFont->TextOut(pItem->m_pszText, pItem->m_nVisibleLength);
            if (i == m_caret.nLine && m_bCaretVisible) {
                android_port_glPushAttrib(1);
                android_port_glColor4f(1.0f, 0.9f, 0.2f, 1.0f);
                android_port_glTranslatef(m_caret.m_fOffset, 0.0f, 0.01f / 10.0f);
                m_pFont->TextOut(m_caret.pszGlyph, 1);
                android_port_glTranslatef(-m_caret.m_fOffset, 0.0f, -(0.01f / 10.0f));
                android_port_glPopAttrib();
            }
            fX += fDX;
            android_port_glTranslatef(0.0f, -(pItem->m_fBottom - pItem->m_fTop), 0.0f);
            fY += pItem->m_fBottom;
        }
    }
    if (i == m_caret.nLine && fY + m_pFont->m_pFontInfo->m_fBottom < m_displayInfo.m_fMaxHeight && m_bCaretVisible) {
        android_port_glColor4f(1.0f, 0.9f, 0.2f, 1.0f);
        android_port_glTranslatef(m_caret.m_fOffset, -m_pFont->m_pFontInfo->m_fTop, 0.01f / 10.0f);
        m_pFont->TextOut(m_caret.pszGlyph, 1);
        android_port_glTranslatef(-m_caret.m_fOffset, 0.0f, -(0.01f / 10.0f));
    }
    if (bLighting)
        android_port_glEnable(GL_LIGHTING);
    else
        android_port_glDisable(GL_LIGHTING);
    android_port_glPopMatrix();
    if (a)
        DoEvent(const_cast<char*>("propagaterender"), 0);
    android_port_glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
#else
void StringGob::Render(bool a) { K2_UNUSED(a); }
#endif

#if KOTOR1
void StringGob::DeleteText()
{
    for (int i = m_lines.m_count - 1; i >= 0; i--) {
        delete m_lines[i];
        m_lines.DelIndex(i);
    }
    m_displayInfo.nFirstLine = 0;
    m_displayInfo.nNumLines = 0;
}
#else
void StringGob::DeleteText() {}
#endif

#if KOTOR1
void StringGob::DeleteLine(int nLine)
{
    delete m_lines[nLine];
    m_lines.DelIndex(nLine);
    if (m_displayInfo.nFirstLine > nLine)
        m_displayInfo.nFirstLine--;
    if (m_displayInfo.nFirstLine + m_displayInfo.nNumLines > m_lines.m_count)
        m_displayInfo.nNumLines = m_lines.m_count - m_displayInfo.nFirstLine;
}
#else
void StringGob::DeleteLine(int nLine) { K2_UNUSED(nLine); }
#endif

#if KOTOR1
void StringGob::GetColorDefault(float& r, float& g, float& b, float& a)
{
    r = m_colors[0].r;
    g = m_colors[0].g;
    b = m_colors[0].b;
    a = m_colors[0].a;
}
#else
void StringGob::GetColorDefault(float& r, float& g, float& b, float& a) { K2_UNUSED(r); K2_UNUSED(g); K2_UNUSED(b); K2_UNUSED(a); }
#endif

#if KOTOR1
void StringGob::SetColorDefault(float r, float g, float b, float a)
{
    m_colors[0].r = r;
    m_colors[0].g = g;
    m_colors[0].b = b;
    m_colors[0].a = a;
}
#else
void StringGob::SetColorDefault(float r, float g, float b, float a) { K2_UNUSED(r); K2_UNUSED(g); K2_UNUSED(b); K2_UNUSED(a); }
#endif

#if KOTOR1
void StringGob::SetIndexedColor(int nIndex, float r, float g, float b, float a)
{
    if (nIndex >= m_colors.m_count)
        m_colors.SetSize(nIndex + 1);
    m_colors[nIndex].r = r;
    m_colors[nIndex].g = g;
    m_colors[nIndex].b = b;
    m_colors[nIndex].a = a;
}
#else
void StringGob::SetIndexedColor(int nIndex, float r, float g, float b, float a) { K2_UNUSED(nIndex); K2_UNUSED(r); K2_UNUSED(g); K2_UNUSED(b); K2_UNUSED(a); }
#endif

#if KOTOR1
void StringGob::SetLinePixelHeight(int nLine, float fHeight)
{
    m_lines[nLine]->m_fBottom = fHeight * 0.01f;
}
#else
void StringGob::SetLinePixelHeight(int nLine, float fHeight) { K2_UNUSED(nLine); K2_UNUSED(fHeight); }
#endif

#if KOTOR1
void StringGob::RecalcDisplayData()
{
    m_displayInfo.m_fTotalHeight = 0.0f;
    for (int i = 0; i < m_lines.m_count; i++) {
        m_pFont->CalculateVisibleStringLengthAndWidth(m_lines[i], m_displayInfo.m_fMaxWidth, false);
        m_displayInfo.m_fTotalHeight += m_lines[i]->m_fBottom;
    }
    ShowAsManyLinesAsYouCan_Temp();
}
#else
void StringGob::RecalcDisplayData() {}
#endif

#if KOTOR1
void StringGob::ShowAsManyLinesAsYouCan_Temp()
{
    float fMaxHeight = m_displayInfo.m_fMaxHeight > -0.000005f ? m_displayInfo.m_fMaxHeight + 0.001f : 10000000.0f;
    m_displayInfo.m_fVisibleHeight = 0.0f;
    m_displayInfo.nNumLines = 0;
    float fHeight = 0.0f;
    int i;
    if (m_displayInfo.m_bScrollMode) {
        for (i = 0; m_displayInfo.nFirstLine + i < m_lines.m_count; i++) {
            fHeight += m_lines[m_displayInfo.nFirstLine + i]->m_fBottom;
            m_displayInfo.m_fVisibleHeight = fHeight;
            m_displayInfo.nNumLines = i + 1;
            if (!(fHeight <= fMaxHeight + m_displayInfo.m_fScrollOffset))
                break;
        }
        m_displayInfo.nNumAbove = m_displayInfo.nNumLines;
        for (i = m_displayInfo.nFirstLine - 1; i >= 0; i--) {
            fHeight += m_lines[i]->m_fBottom;
            m_displayInfo.nNumAbove++;
            if (!(fHeight <= fMaxHeight + m_displayInfo.m_fScrollOffset))
                break;
        }
    } else {
        for (i = 0; m_displayInfo.nFirstLine + i >= 0 && m_displayInfo.nFirstLine + i < m_lines.m_count; i++) {
            fHeight += m_lines[m_displayInfo.nFirstLine + i]->m_fBottom;
            if (!(fHeight <= fMaxHeight))
                break;
            m_displayInfo.m_fVisibleHeight = fHeight;
            m_displayInfo.nNumLines = i + 1;
        }
        m_displayInfo.nNumAbove = m_displayInfo.nNumLines;
        for (i = m_displayInfo.nFirstLine - 1; i >= 0; i--) {
            fHeight += m_lines[i]->m_fBottom;
            if (!(fHeight <= fMaxHeight))
                break;
            m_displayInfo.nNumAbove++;
        }
    }
}
#else
void StringGob::ShowAsManyLinesAsYouCan_Temp() {}
#endif

#if KOTOR1
void StringGob::AttachModel(CAurObject* pObject, int a)
{
    m_lines[a]->m_pAttached = static_cast<Gob*>(pObject);
}
#else
void StringGob::AttachModel(CAurObject* pObject, int a) { K2_UNUSED(pObject); K2_UNUSED(a); }
#endif

#if KOTOR1
void StringGob::AddLine(char* pszText)
{
    CStringItem* pItem = new CStringItem;
    m_lines.Add(pItem);
    int nLine = m_lines.m_count - 1;
    pItem->SetString(pszText, static_cast<int>(strlen(pszText)));
    if (m_pFont) {
        pItem->m_fBottom = 0.0f;
        pItem->m_fTop = 0.0f;
        m_pFont->CalculateVisibleStringLengthAndWidth(pItem, m_displayInfo.m_fMaxWidth, false);
        WrapLines(nLine);
    }
    ShowAsManyLinesAsYouCan_Temp();
}
#else
void StringGob::AddLine(char* pszText) { K2_UNUSED(pszText); }
#endif

#if KOTOR1
void StringGob::InsertLine(char* pszText, int nLine)
{
    CStringItem* pItem = new CStringItem;
    m_lines.Insert(pItem, nLine);
    pItem->SetString(pszText, static_cast<int>(strlen(pszText)));
    if (m_pFont) {
        pItem->m_fBottom = 0.0f;
        pItem->m_fTop = 0.0f;
        m_pFont->CalculateVisibleStringLengthAndWidth(pItem, m_displayInfo.m_fMaxWidth, false);
    }
    ShowAsManyLinesAsYouCan_Temp();
}
#else
void StringGob::InsertLine(char* pszText, int nLine) { K2_UNUSED(pszText); K2_UNUSED(nLine); }
#endif

#if KOTOR1
void StringGob::SetLine(int nLine, char* pszText)
{
    int bAdded = 0;
    while (m_lines.m_count <= nLine) {
        m_lines.Add(new CStringItem);
        bAdded = 1;
    }
    CStringItem* pItem = m_lines[nLine];
    pItem->SetString(pszText, static_cast<int>(strlen(pszText)));
    if (m_pFont) {
        pItem->m_fBottom = 0.0f;
        pItem->m_fTop = 0.0f;
        m_pFont->CalculateVisibleStringLengthAndWidth(pItem, m_displayInfo.m_fMaxWidth, false);
    }
    if (bAdded)
        ShowAsManyLinesAsYouCan_Temp();
}
#else
void StringGob::SetLine(int nLine, char* pszText) { K2_UNUSED(nLine); K2_UNUSED(pszText); }
#endif

void StringGob::GetMinimumBoundingBox(Vector& vMin, Vector& vMax) { K2_UNUSED(vMin); K2_UNUSED(vMax); }

void StringGob::GetMinimumBoundingSphere(Vector& vCenter, float& fRadius) { K2_UNUSED(vCenter); K2_UNUSED(fRadius); }

StringGob* StringGob::AsString() { return this; }

#if KOTOR1
void StringGob::RefreshDisplay() { RecalcDisplayData(); }
#else
void StringGob::RefreshDisplay() {}
#endif


#if KOTOR1
int StringGob::SetFont(char* pszFont)
{
    if (m_pFont) {
        m_pFont->m_nRefCount--;
        m_pFont = 0;
    }
    int i;
    for (i = 0; i < CAurFont::s_LoadedFontList.m_count; i++) {
        if (strncmp(CAurFont::s_LoadedFontList[i]->m_szName, pszFont, 16) == 0) {
            m_pFont = CAurFont::s_LoadedFontList[i];
            break;
        }
    }
    if (m_pFont == 0)
        m_pFont = new CAurFont(pszFont);
    m_pFont->m_nRefCount++;
    RecalcDisplayData();
    m_displayInfo.m_fLineHeight = m_pFont->m_pFontInfo->m_fBottom / 0.01f;
    return i;
}
#else
int StringGob::SetFont(char* pszFont) { K2_UNUSED(pszFont); return 0; }
#endif

#if KOTOR1
int StringGob::SetFontPassword(char* pszFont, char chMask)
{
    int nFont = SetFont(pszFont);
    if (m_pFont) {
        m_pFont->m_nRefCount--;
        m_pFont = 0;
    }
    CAurFont* pFont = CAurFont::s_LoadedFontList[nFont];
    if (pFont->m_nPasswordFontIndex != 0) {
        m_pFont = pFont;
        pFont->m_nRefCount++;
        return CAurFont::s_LoadedFontList[nFont]->m_nPasswordFontIndex;
    }
    m_pFont = new CAurFont(nFont, chMask);
    m_pFont->m_nRefCount++;
    return CAurFont::s_LoadedFontList[nFont]->m_nPasswordFontIndex = CAurFont::s_LoadedFontList.m_count - 1;
}
#else
int StringGob::SetFontPassword(char* pszFont, char chMask) { K2_UNUSED(pszFont); K2_UNUSED(chMask); return 0; }
#endif

#if KOTOR1
int StringGob::SuggestDeleteCurrentFont()
{
    CAurFont* pFont = m_pFont;
    m_pFont = 0;
    if (--pFont->m_nRefCount == 0) {
        delete pFont;
        return 1;
    }
    return 0;
}
#else
int StringGob::SuggestDeleteCurrentFont() { return 0; }
#endif

#if KOTOR1
void* StringGob::AttachToPart(CAurObject* a, CAurPart* b, int c) { return Gob::AttachToPart(a, b, c); }
#else
void* StringGob::AttachToPart(CAurObject* a, CAurPart* b, int c) { K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c); return 0; }
#endif

#if KOTOR1
void* StringGob::AttachToObject(CAurObject* a, char* b, int c) { return Gob::AttachToObject(a, b, c); }
#else
void* StringGob::AttachToObject(CAurObject* a, char* b, int c) { K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c); return 0; }
#endif

#if KOTOR1
int StringGob::AttachCornersToObjects(CAurObject* a, char* b, char* c)
{
    Part* pCorner1 = a->GetPart(b);
    Part* pCorner2 = a->GetPart(c);
    Quaternion qObject = a->GetOrientation();
    Quaternion qFrame = qObject * -pCorner1->m_qLocalOrientation;
    Vector v1;
    Vector v2;
    Quaternion q;
    pCorner1->GetWorld(v1, q);
    pCorner2->GetWorld(v2, q);
    Vector vSize = RotateVector(qFrame, v2 - v1);
    m_displayInfo.m_fMaxWidth = vSize.x > 0.0f ? vSize.x : -vSize.x;
    m_displayInfo.m_fMaxHeight = vSize.y > 0.0f ? vSize.y : -vSize.y;
    if (m_pFont)
        RecalcDisplayData();
    return Gob::AttachToObject(a, b, 0) != 0;
}
#else
int StringGob::AttachCornersToObjects(CAurObject* a, char* b, char* c) { K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c); return 0; }
#endif

#if KOTOR1
int StringGob::SetText(char* pszText)
{
    DeleteText();
    AddText(pszText);
    if (m_caret.nLine >= m_lines.m_count) {
        m_caret.nLine = m_lines.m_count > 0 ? m_lines.m_count - 1 : 0;
        m_caret.nColumn = 1000000;
    }
    if (static_cast<unsigned int>(m_caret.nLine) < static_cast<unsigned int>(m_lines.m_count)) {
        if (static_cast<int>(strlen(m_lines[m_caret.nLine]->m_pszText)) < m_caret.nColumn)
            m_pFont->UpdateCaret(&m_caret, m_lines[m_caret.nLine]->m_pszText, 1000000.0f, false);
    }
    return 1;
}
#else
int StringGob::SetText(char* pszText) { K2_UNUSED(pszText); return 1; }
#endif

#if KOTOR1
int StringGob::AddText(char* pszText)
{
    if (m_lines.m_count <= 0) {
        AddLine(pszText);
    } else {
        int nLine = m_lines.m_count - 1;
        CStringItem* pItem = m_lines[nLine];
        pItem->AppendString(pszText, static_cast<int>(strlen(pszText)));
        m_pFont->CalculateVisibleStringLengthAndWidth(pItem, m_displayInfo.m_fMaxWidth, false);
        if (m_bTextWrapping) {
            WrapLines(nLine);
            ShowAsManyLinesAsYouCan_Temp();
        }
    }
    return 1;
}
#else
int StringGob::AddText(char* pszText) { K2_UNUSED(pszText); return 1; }
#endif

#if KOTOR1
float StringGob::GetAlignment() { return m_fAlignment; }
#else
float StringGob::GetAlignment() { return 0.0f; }
#endif

#if KOTOR1
float StringGob::GetAlignmentVertical() { return m_fAlignmentVertical; }
#else
float StringGob::GetAlignmentVertical() { return 0.0f; }
#endif

#if KOTOR1
char* StringGob::GetFontName() { return m_pFont ? m_pFont->m_szName : const_cast<char*>(""); }
#else
char* StringGob::GetFontName() { return 0; }
#endif

#if KOTOR1
char* StringGob::GetTextLine(int nLine) { return m_lines[nLine]->m_pszText; }
#else
char* StringGob::GetTextLine(int nLine) { K2_UNUSED(nLine); return 0; }
#endif

#if KOTOR1
float StringGob::GetLinePixelWidth(int nLine) { return m_lines[nLine]->m_fVisibleWidth / 0.01f; }
#else
float StringGob::GetLinePixelWidth(int nLine) { K2_UNUSED(nLine); return 0.0f; }
#endif

#if KOTOR1
float StringGob::GetLinePixelHeight(int nLine) { return m_lines[nLine]->m_fBottom / 0.01f; }
#else
float StringGob::GetLinePixelHeight(int nLine) { K2_UNUSED(nLine); return 0.0f; }
#endif

#if KOTOR1
void* StringGob::HitCheck(Vector a, Vector b, Vector* c, CHitInfo* d)
{
    Vector vStart = RotateVector(-m_qOrientation, a - m_vPosition);
    Vector vEnd = RotateVector(-m_qOrientation, b - m_vPosition);
    Vector aCorners[4];
    aCorners[0] = Vector(-m_displayInfo.m_fHitMargin, 0.0f, 0.0f);
    aCorners[1] = Vector(-m_displayInfo.m_fHitMargin, -m_displayInfo.m_fMaxHeight, 0.0f);
    aCorners[2] = Vector(m_displayInfo.m_fHitMargin + m_displayInfo.m_fMaxWidth, -m_displayInfo.m_fMaxHeight, 0.0f);
    aCorners[3] = Vector(m_displayInfo.m_fHitMargin + m_displayInfo.m_fMaxWidth, 0.0f, 0.0f);
    if (!polyhit(aCorners, 4, vStart, vEnd, &vEnd, 0))
        return 0;
    if (c)
        *c = RotateVector(m_qOrientation, vEnd) + m_vPosition;
    float fTop = m_displayInfo.m_fScrollOffset - (m_displayInfo.m_fMaxHeight - m_displayInfo.m_fVisibleHeight) * m_fAlignmentVertical;
    vStart = RotateVector(-m_qOrientation, Vector(vStart.x, vStart.y - fTop, vStart.z));
    vEnd = RotateVector(-m_qOrientation, Vector(vEnd.x, vEnd.y - fTop, vEnd.z));
    void* pResult = this;
    float fX = 0.0f;
    int i = m_displayInfo.nFirstLine;
    for (int n = 0; n < m_displayInfo.nNumLines && i < m_lines.m_count; n++, i++) {
        CStringItem* pItem = m_lines[i];
        float fLineX = (m_displayInfo.m_fMaxWidth - pItem->m_fVisibleWidth) * m_fAlignment;
        float fDX = static_cast<float>(static_cast<int>((fLineX - fX) / 0.01f)) * 0.01f;
        vStart = RotateVector(-m_qOrientation, Vector(vStart.x - fDX, pItem->m_fTop + vStart.y, vStart.z));
        vEnd = RotateVector(-m_qOrientation, Vector(vEnd.x - fDX, pItem->m_fTop + vEnd.y, vEnd.z));
        if (pItem->m_pAttached) {
            void* pHit = pItem->m_pAttached->HitCheck(vStart, vEnd, c, d);
            if (pHit)
                pResult = pHit;
        }
        vStart = RotateVector(-m_qOrientation, Vector(vStart.x, (pItem->m_fBottom - pItem->m_fTop) + vStart.y, vStart.z));
        vEnd = RotateVector(-m_qOrientation, Vector(vEnd.x, (pItem->m_fBottom - pItem->m_fTop) + vEnd.y, vEnd.z));
        fX = fLineX;
    }
    return pResult;
}
#else
void* StringGob::HitCheck(Vector a, Vector b, Vector* c, CHitInfo* d) { K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c); K2_UNUSED(d); return 0; }
#endif

#if KOTOR1
int StringGob::MoveCaretTo(Vector v)
{
    Vector vLocal = RotateVector(-m_qOrientation, v - m_vPosition);
    float fY = -(vLocal.y - m_displayInfo.m_fScrollOffset);
    float fTop = (m_displayInfo.m_fMaxHeight - m_displayInfo.m_fVisibleHeight) * m_fAlignmentVertical;
    int i = m_displayInfo.nFirstLine - 1;
    if (fTop <= fY) {
        for (i = m_displayInfo.nFirstLine; i < m_displayInfo.nFirstLine + m_displayInfo.nNumLines && i < m_lines.m_count; i++) {
            fTop += m_lines[i]->m_fBottom;
            if (!(fTop <= fY))   // a NaN height ends the search too
                break;
        }
    }
    if (i < m_displayInfo.nFirstLine)
        i = m_displayInfo.nFirstLine;
    if (i < m_lines.m_count) {
        m_caret.nLine = i;
        m_caret.nColumn = -1;
        m_pFont->UpdateCaret(&m_caret, m_lines[i]->m_pszText,
                             vLocal.x - (m_displayInfo.m_fMaxWidth - m_lines[i]->m_fVisibleWidth) * m_fAlignment, false);
        return 1;
    }
    if (m_lines.m_count - 1 >= 0) {
        m_caret.nLine = m_lines.m_count - 1;
        m_caret.nColumn = -1;
        m_pFont->UpdateCaret(&m_caret, m_lines[m_lines.m_count - 1]->m_pszText, 1000000.0f, false);
        return 0;
    }
    m_caret.nLine = 0;
    m_caret.nColumn = 0;
    m_caret.m_fOffset = 0.0f;
    return 0;
}
#else
int StringGob::MoveCaretTo(Vector v) { K2_UNUSED(v); return 0; }
#endif

#if KOTOR1
int StringGob::WrapLines(int nWidth)
{
    char* pszText = m_lines[nWidth]->m_pszText;
    int nLength = static_cast<int>(strlen(pszText));
    m_pFont->CalculateVisibleStringLengthAndWidth(m_lines[nWidth], m_displayInfo.m_fMaxWidth, m_bTextWrapping);
    int nSplit = m_lines[nWidth]->m_nVisibleLength;
    if (pszText[nSplit] == ' ')
        nSplit++;
    if (pszText[nSplit] == '\n')
        nSplit++;
    if (nLength != 0 && nSplit == 0)
        return -1;
    if (nSplit == nLength || m_lines.m_count == m_nMaxLines || !m_bTextWrapping)
        return 0;
    if (nWidth + 1 < m_lines.m_count) {
        m_lines[nWidth + 1]->PrependString(pszText + nSplit, nLength - nSplit);
    } else {
        CStringItem* pItem = new CStringItem;
        pItem->m_nVisibleStart = m_lines[nWidth]->m_nVisibleStart;
        pItem->m_nVisibleLength = m_lines[nWidth]->m_nVisibleLength;
        pItem->m_nState = m_lines[nWidth]->m_nState;
        pItem->m_fVisibleWidth = m_lines[nWidth]->m_fVisibleWidth;
        pItem->SetString(pszText + nSplit, nLength - nSplit);
        m_lines.Add(pItem);
    }
    m_lines[nWidth]->SetString(pszText, nSplit);
    return WrapLines(nWidth + 1);
}
#else
int StringGob::WrapLines(int nWidth) { K2_UNUSED(nWidth); return 0; }
#endif

#if KOTOR1
int StringGob::UnWrapLines(int nWidth)
{
    CStringItem* pItem = m_lines[nWidth];
    pItem->m_fBottom = 0.0f;
    pItem->m_fTop = 0.0f;
    m_pFont->CalculateVisibleStringLengthAndWidth(pItem, m_displayInfo.m_fMaxWidth, false);
    if (nWidth + 1 != m_lines.m_count) {
        pItem = m_lines[nWidth];
        if (pItem->m_pszText[pItem->m_nVisibleStart + pItem->m_nVisibleLength] != '\n') {
            char* pszNext = m_lines[nWidth + 1]->m_pszText;
            pItem->AppendString(pszNext, static_cast<int>(strlen(pszNext)));
            m_lines[nWidth + 1]->SetString(const_cast<char*>(""), 0);
            pItem = m_lines[nWidth + 1];
            pItem->m_fBottom = 0.0f;
            pItem->m_fTop = 0.0f;
            m_pFont->CalculateVisibleStringLengthAndWidth(pItem, m_displayInfo.m_fMaxWidth, false);
            WrapLines(nWidth);
            if (m_lines[nWidth + 1]->m_pszText[0] == '\0')
                DeleteLine(nWidth + 1);
        }
    }
    return 0;
}
#else
int StringGob::UnWrapLines(int nWidth) { K2_UNUSED(nWidth); return 0; }
#endif

#if KOTOR1
int StringGob::InsertTextAtCaret(char* pszText)
{
    if (m_caret.nLine < 0)
        return 0;
    int nLength;
    if (m_caret.nLine < m_lines.m_count) {
        nLength = static_cast<int>(strlen(pszText));
        m_lines[m_caret.nLine]->InsertString(pszText, nLength, m_caret.nColumn);
        WrapLines(m_caret.nLine);
        if (!m_bTextWrapping) {
            CStringItem* pItem = m_lines[m_caret.nLine];
            int nText = static_cast<int>(strlen(pItem->m_pszText));
            if (pItem->m_nVisibleLength < nText)
                pItem->DeleteChars(pItem->m_nVisibleLength, nText - pItem->m_nVisibleLength);
        }
    } else {
        if (m_caret.nLine >= m_nMaxLines)
            return 0;
        m_lines.Add(new CStringItem);
        m_caret.nLine = m_lines.m_count - 1;
        nLength = static_cast<int>(strlen(pszText));
        m_lines[m_caret.nLine]->SetString(pszText, nLength);
        WrapLines(m_caret.nLine);
    }
    ShowAsManyLinesAsYouCan_Temp();
    SetCaretColumnPos(GetCaretColumnPos() + nLength);
    return 1;
}
#else
int StringGob::InsertTextAtCaret(char* pszText) { K2_UNUSED(pszText); return 0; }
#endif

#if KOTOR1
int StringGob::DeleteTextAtCaret(int nCount)
{
    if (m_caret.nLine < 0 || m_caret.nLine >= m_lines.m_count)
        return 0;
    int nLength = static_cast<int>(strlen(m_lines[m_caret.nLine]->m_pszText));
    if (m_caret.nColumn + nCount > nLength)
        nCount = nLength - m_caret.nColumn;
    if (nCount == nLength) {
        DeleteLine(m_caret.nLine);
    } else {
        m_lines[m_caret.nLine]->DeleteChars(m_caret.nColumn, nCount);
        UnWrapLines(m_caret.nLine);
    }
    SetCaretColumnPos(m_caret.nColumn);
    return 1;
}
#else
int StringGob::DeleteTextAtCaret(int nCount) { K2_UNUSED(nCount); return 0; }
#endif

#if KOTOR1
bool StringGob::SetCaretLinePos(int nLine)
{
    if (nLine < 0 || nLine > m_lines.m_count)
        return false;
    if (nLine == m_lines.m_count) {
        if (nLine > 0) {
            char* pszText = m_lines[nLine - 1]->m_pszText;
            int nLength = static_cast<int>(strlen(pszText));
            if (nLength < 1 || pszText[nLength - 1] != '\n' || m_nMaxLines <= nLine)
                return false;
        }
        m_caret.nLine = nLine;
        m_caret.nColumn = 0;
        m_caret.m_fOffset = 0.0f;
        return true;
    }
    float fX = 0.0f;
    if (m_caret.nLine >= 0 && m_caret.nLine < m_lines.m_count)
        fX = (m_displayInfo.m_fMaxWidth - m_lines[m_caret.nLine]->m_fVisibleWidth) * m_fAlignment;
    m_caret.nLine = nLine;
    m_caret.nColumn = -1;
    m_pFont->UpdateCaret(&m_caret, m_lines[nLine]->m_pszText,
                         (fX + m_caret.m_fOffset) - (m_displayInfo.m_fMaxWidth - m_lines[nLine]->m_fVisibleWidth) * m_fAlignment,
                         false);
    if (m_displayInfo.nFirstLine > nLine) {
        m_displayInfo.nFirstLine = nLine;
    } else {
        int nEnd = m_displayInfo.nFirstLine + m_displayInfo.nNumLines;
        if (nEnd <= nLine)
            m_displayInfo.nFirstLine += nLine - nEnd + 1;
    }
    return true;
}
#else
int StringGob::SetCaretLinePos(int nLine) { K2_UNUSED(nLine); return 0; }
#endif

#if KOTOR1
int StringGob::SetCaretColumnPos(int nColumn)
{
    int nLine = GetCaretLinePos();
    if (nColumn < 0) {
        if (!SetCaretLinePos(nLine - 1))
            return 0;
        return SetCaretColumnPos(static_cast<int>(strlen(m_lines[m_caret.nLine]->m_pszText)) + nColumn);
    }
    if (nLine >= m_lines.m_count) {
        m_caret.nColumn = 0;
        m_caret.m_fOffset = 0.0f;
        return 1;
    }
    int nText = static_cast<int>(strlen(m_lines[m_caret.nLine]->m_pszText));
    if (nText < nColumn
        || (nColumn > 0 && nText == nColumn && m_lines[m_caret.nLine]->m_pszText[nColumn - 1] == '\n')) {
        int nLength = static_cast<int>(strlen(m_lines[m_caret.nLine]->m_pszText));
        if (SetCaretLinePos(GetCaretLinePos() + 1))
            return SetCaretColumnPos(nColumn - nLength);
        nColumn = nLength;
    }
    m_caret.nColumn = nColumn;
    m_pFont->UpdateCaret(&m_caret, m_lines[m_caret.nLine]->m_pszText, 1000000.0f, true);
    return 1;
}
#else
int StringGob::SetCaretColumnPos(int nColumn) { K2_UNUSED(nColumn); return 1; }
#endif

#if KOTOR1
int StringGob::ScrollToLine(int nLine)
{
    if (nLine > m_lines.m_count)
        nLine = m_lines.m_count;
    else if (nLine < 0)
        nLine = 0;
    int nFirst = m_displayInfo.nFirstLine;
    int nNum = m_displayInfo.nNumLines;
    m_displayInfo.nFirstLine = nLine;
    if (nLine < nFirst && nFirst + nNum == m_lines.m_count)
        ShowAsManyLinesAsYouCan_Temp();
    return nLine;
}
#else
int StringGob::ScrollToLine(int nLine) { K2_UNUSED(nLine); return 0; }
#endif

#if KOTOR1
Vector StringGob::SetPosition(Vector v)
{
    m_vPosition = v;
    return v;
}
#else
Vector StringGob::SetPosition(Vector v)
{
    K2_UNUSED(v);
    Vector vResult;  // zeroed by Vector's default ctor
    return vResult;
}
#endif

#if KOTOR1
int StringGob::SetPositionOffset(Vector v)
{
    CAurBehavior* pBehavior = GetBehavior(-1);
    if (pBehavior == 0)
        return 0;
    if (pBehavior->GetType() == kBehaviorAttachTranslate) {
        static_cast<CAurBehaviorAttachTranslate*>(pBehavior)->m_vTranslation = v;
        return 1;
    }
    return 0;
}
#else
int StringGob::SetPositionOffset(Vector v) { K2_UNUSED(v); return 0; }
#endif

#if KOTOR1
CAurObject* StringGob::GetAttachedModel(int nIndex)
{
    Gob* pAttached = m_lines[nIndex]->m_pAttached;
    if (pAttached)
        return pAttached->AsCAurObject();
    return 0;
}
#else
CAurObject* StringGob::GetAttachedModel(int nIndex) { K2_UNUSED(nIndex); return 0; }
#endif

#if KOTOR1
int StringGob::ScrollToPixel(int nPixel)
{
    m_displayInfo.m_bScrollMode = true;
    float fPixel = nPixel * 0.01f;
    m_displayInfo.m_fScrollOffset = 0.0f;
    if (fPixel >= m_displayInfo.m_fTotalHeight) {
        ScrollToLine(m_lines.m_count - 1);
        return -1;
    }
    float fHeight = 0.0f;
    int i;
    for (i = 0; i < m_lines.m_count; i++) {
        float fNext = fHeight + m_lines[i]->m_fBottom;
        if (fNext > fPixel) {
            m_displayInfo.m_fScrollOffset = fPixel - fHeight;
            ShowAsManyLinesAsYouCan_Temp();
            break;
        }
        fHeight = fNext;
    }
    ScrollToLine(i);
    return -1;
}
#else
int StringGob::ScrollToPixel(int nPixel) { K2_UNUSED(nPixel); return -1; }
#endif

CAurObject* StringGob::AsCAurObject() { return 0; }


void* StringGob::GetExternalData() { return m_pExternalData; }

void StringGob::SetExternalData(void* pData) { m_pExternalData = pData; }

#if KOTOR1
int StringGob::GetCurrentFontIndex() { return m_pFont->m_nIndex; }
#else
int StringGob::GetCurrentFontIndex() { return m_pFont->nIndex; }
#endif

void StringGob::SetAlignment(float f) { m_fAlignment = f; }

void StringGob::SetAlignmentVertical(float f) { m_fAlignmentVertical = f; }

#if KOTOR1
void StringGob::SetMaxWidth(float f) { m_displayInfo.m_fMaxWidth = f; }
#else
void StringGob::SetMaxWidth(float f) { m_fMaxWidth = f; }
#endif

#if KOTOR1
void StringGob::SetMaxHeight(float f) { m_displayInfo.m_fMaxHeight = f; }
#else
void StringGob::SetMaxHeight(float f) { m_fMaxHeight = f; }
#endif

#if KOTOR1
int StringGob::GetTextLineCount() { return m_lines.m_count; }
#else
int StringGob::GetTextLineCount() { return m_lines.nCount; }
#endif

#if KOTOR1
int StringGob::GetLineState(int nLine) { return m_lines[nLine]->m_nState; }
#else
int StringGob::GetLineState(int nLine) { return m_lines.pa[nLine]->nState; }
#endif

#if KOTOR1
void StringGob::SetLineState(int nLine, int nState) { m_lines[nLine]->m_nState = static_cast<short>(nState); }
#else
void StringGob::SetLineState(int nLine, int nState) { m_lines.pa[nLine]->nState = static_cast<short>(nState); }
#endif

#if KOTOR1
void StringGob::SetLineColor(int nLine, int nColor) { m_lines[nLine]->m_nColor = static_cast<short>(nColor); }
#else
void StringGob::SetLineColor(int nLine, int nColor) { m_lines.pa[nLine]->nColor = static_cast<short>(nColor); }
#endif

#if KOTOR1
int StringGob::GetLineColor(int nLine) { return m_lines[nLine]->m_nColor; }
#else
int StringGob::GetLineColor(int nLine) { return m_lines.pa[nLine]->nColor; }
#endif

#if KOTOR1
int StringGob::GetNumColors() { return m_colors.m_count; }
#else
int StringGob::GetNumColors() { return m_colors.nCount; }
#endif

SStringGobDisplayInfo* StringGob::GetDisplayInfo() { return &m_displayInfo; }

void StringGob::SetTextWrapping(bool bWrap) { m_bTextWrapping = bWrap; }

int StringGob::GetCaretLinePos() { return m_caret.nLine; }

int StringGob::GetCaretColumnPos() { return m_caret.nColumn; }

#if KOTOR1
SStringGobLine** StringGob::GetStringsArray() { return reinterpret_cast<SStringGobLine**>(m_lines.m_pData); }
#else
SStringGobLine** StringGob::GetStringsArray() { return m_lines.pa; }
#endif

void StringGob::SetCaretVisible(bool bVisible) { m_bCaretVisible = bVisible; }

void StringGob::SetColorSelected(float r, float g, float b, float a)
{
    SetIndexedColor(1, r, g, b, a);
}


#if KOTOR1
CAurString* NewCAurString(int nId) {
    return new StringGob(nId);
}
#else
CAurString* NewCAurString(int nId) {
    K2_UNUSED(nId);
    return 0;
}
#endif

#if KOTOR1
void AttachString(CAurString* pParent, CAurString* pChild, int nNode) {
    StringGob* pParentGob = static_cast<StringGob*>(pParent);
    pParentGob->m_lines[nNode]->m_pAttached = static_cast<StringGob*>(pChild);
}
#else
void AttachString(CAurString* pParent, CAurString* pChild, int nNode) {
    K2_UNUSED(pParent); K2_UNUSED(pChild); K2_UNUSED(nNode);
}
#endif

#if KOTOR1
CAurString* GetAttachedAsString(CAurString* pParent, int nNode) {
    Gob* pAttached = static_cast<StringGob*>(pParent)->m_lines[nNode]->m_pAttached;
    if (pAttached)
        pAttached = pAttached->AsString();
    return static_cast<StringGob*>(pAttached);
}
#else
CAurString* GetAttachedAsString(CAurString* pParent, int nNode) {
    K2_UNUSED(pParent); K2_UNUSED(nNode);
    return 0;
}
#endif


void CStringItem::SetString(char* pszText, int nLength)
{
    char* pszOld = m_pszText;
    m_pszText = new char[nLength + 1];
    m_pszText[nLength] = 0;
    strncpy(m_pszText, pszText, nLength);
    delete[] pszOld;
}

void CStringItem::AppendString(char* pszText, int nLength)
{
    char* pszOld = m_pszText;
    int nOldLength = static_cast<int>(strlen(pszOld));
    m_pszText = new char[nOldLength + nLength + 1];
    m_pszText[nOldLength + nLength] = 0;
    strncpy(m_pszText, pszOld, nOldLength);
    strncpy(m_pszText + nOldLength, pszText, nLength);
    delete[] pszOld;
}

void CStringItem::PrependString(char* pszText, int nLength)
{
    char* pszOld = m_pszText;
    int nOldLength = static_cast<int>(strlen(pszOld));
    m_pszText = new char[nOldLength + nLength + 1];
    m_pszText[nOldLength + nLength] = 0;
    strncpy(m_pszText, pszText, nLength);
    strncpy(m_pszText + nLength, pszOld, nOldLength);
    delete[] pszOld;
}

void CStringItem::InsertString(char* pszText, int nCount, int nPosition)
{
    char* pszOld = m_pszText;
    int nOldLength = static_cast<int>(strlen(pszOld));
    m_pszText = new char[nOldLength + nCount + 1];
    m_pszText[nOldLength + nCount] = 0;
    strncpy(m_pszText, pszOld, nPosition);
    strncpy(m_pszText + nPosition, pszText, nCount);
    strncpy(m_pszText + nPosition + nCount, pszOld + nPosition, nOldLength - nPosition);
    delete[] pszOld;
}

void CStringItem::DeleteChars(int nStart, int nCount)
{
    char* pszOld = m_pszText;
    int nOldLength = static_cast<int>(strlen(pszOld));
    m_pszText = new char[nOldLength - nCount + 1];
    m_pszText[nOldLength - nCount] = 0;
    strncpy(m_pszText, pszOld, nStart);
    strncpy(m_pszText + nStart, pszOld + nStart + nCount, nOldLength - nStart - nCount);
    delete[] pszOld;
}

void CStringItem::Render(CAurFont* pFont)
{
    pFont->TextOutA(m_pszText, m_nVisibleLength);
}

CStringItem::~CStringItem()
{
    delete[] m_pszText;
}

#if KOTOR1
void CAurString::DieWithoutParent()
{
    StringGob* pGob = static_cast<StringGob*>(this);
    if (pGob->m_pBehavior)
        pGob->m_pBehavior->Message(const_cast<char*>("diewithoutparent 1"));
}
#else
void CAurString::DieWithoutParent()
{
}
#endif
