#pragma once

// The on-screen text overlay, made of the bitmap-font console printer, its timed fading-message
// queue and the trace watchers.

#ifndef AURSTRING_H
#define AURSTRING_H

#include "List.h"

class StringGob;

// A colour quad.
class CStringColor {
public:
    float r;
    float g;
    float b;
    float a;
};

// A queued screen message that removes itself from the module's post list on destruction.
class PostedString {
public:
    PostedString(char* pszText, int nX, int nY, float fTime);
    ~PostedString();

    void Post();

    char m_szText[1024];
    int m_nLength;
    float m_fTimeLeft;
    float m_fTotalTime;
    CStringColor m_cColorStart;
    CStringColor m_cColorEnd;
    int m_nX;
    int m_nY;
};

// A watched expression registered by the `trace` console command.
class TraceString {
public:
    TraceString(char* pszText);
    ~TraceString();

    char m_szText[256];
};

void SetConsoleFont(char* pszFont);
void AurInitFont();
void AurFreeFont();
void PrintStringScreenPos(char* pszText, float fX, float fY);
void PrintStringScreenPos_UseTextureFont(char* pszText, float fX, float fY);
void AurPrintString(char* pszText, int nX, int nY);
void SetConsoleFontColor(char* pszColor);
void AurStringsFree();
void PostTraceStrings();
void AurPostString(char* pszText, int nX, int nY, float fTime);
char* trace(char* pszExpr);
char* untrace(char* pszExpr);
void AurRenderStrings();
void AurPrintf(const char* pszFormat, ...);

extern List<PostedString*> posts;
extern List<TraceString*> TraceStrings;
extern CStringColor g_cDefCol;
extern CStringColor g_cDefCol2;
extern CStringColor g_cSavedCol;
extern int initialized;
extern int fontOffset;
extern unsigned char rasters[95][13];
extern int g_nCurrentPrintLine;
extern int g_nPrintfIndentChars;
extern int g_bEnableAurPrintf;
extern bool g_bPrintfUseRightSide;

#endif

#ifndef CCARET_H
#define CCARET_H

// A string gob's text-entry cursor, holding the glyph it draws, its line and column, and the
// column's x offset, which UpdateCaret writes.
class CCaret {
public:
    char* pszGlyph;
    int   nLine;
    int   nColumn;
    float m_fOffset;
    CCaret(char* psz) : pszGlyph(psz), nLine(0), nColumn(0), m_fOffset(0.0f) {}
};

#endif
