// A loaded texture font: the texture, the glyph table it hands back, and a slot in the process-wide
// s_LoadedFontList that markup tags and the console index by number.
#ifndef CAURFONT_H
#define CAURFONT_H

#include "List.h"
#include "SafePointer.h"

class CAurFontInfo;
class CAurTexture;
class CCaret;
class CStringColor;
class CStringItem;

class CAurFont {
public:
    CAurFont(char* pszName);
    // Clone `s_LoadedFontList[nFont]` with every printable glyph remapped onto the single
    // character `chChar`: the solid-block font the GUI draws password fields and bars with.
    CAurFont(int nFont, char chChar);
    ~CAurFont();

#if KOTOR1  // K1 was built without the Windows A-suffix rename
    int TextOut(char* pszText, int nLength);
    int TextOutA(char* pszText, int nLength) { return TextOut(pszText, nLength); }
#else
    int TextOutA(char* pszText, int nLength);
#endif
    int ProcessTextMarkup(char* pszText, CAurFont** ppFont, CStringColor& cColor);
    void CalculateVisibleStringLengthAndWidth(CStringItem* pItem, float fMaxWidth, bool bWrap);
    void UpdateCaret(CCaret* pCaret, char* pszText, float fX, bool bStopAtNewline);

    // Every font ever loaded, in creation order; m_nIndex is the owner's slot in it.
    static List<CAurFont*> s_LoadedFontList;

    int m_nIndex;
    int m_nPasswordFontIndex;
    char m_szName[16];
    CAurFontInfo* m_pFontInfo;
    SafePointer<CAurTexture> m_pTexture;
    int m_nRefCount;
};

#endif // CAURFONT_H
