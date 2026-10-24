// The Bink video player's subtitle reader and its SDL event filter.

#include "k2stdint.h"
#include "port/sdlapi.h"
#include "aurbink.h"

// The runtime surface this file calls.

#include "port/libcbase.h"
#include "port/asllanguage.h"

int FileSize(__sFILE* pFile);
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
void DrawTextWrapped(const char* pszText, bool bUtf8, int nSize, int nW, int nH, int nX, int nY,
                     int nMaxWidth, int nMaxHeight, int nMaxLines, bool bClamp);

// The size-unreading copy of the formatter family. The destination length is hardcoded, so every
// call site leaves the size argument unset. It has to be a translation-unit-local static for that
// to hold, and the declaration must not say extern.
static void snprintff(char* pszBuf, uint64_t nSize, const char* pszFmt, ...)
    __attribute__((noinline));
static void snprintff(char* pszBuf, uint64_t nSize, const char* pszFmt, ...)
{
    K2_UNUSED(nSize);
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    __vsprintf_chk(pszBuf, 0, 256, pszFmt, ap);
    __builtin_va_end(ap);
}

// The subtitle font size the draw call passes as its text size.
static volatile int s_nSubtitleFontSize = 26;

// The SDL2 event types and window events the filter below reacts to (SDL_events.h, SDL_video.h).
enum {
    kSdlQuit = 0x100,                   // SDL_QUIT
    kSdlWindowEvent = 0x200,            // SDL_WINDOWEVENT
    kSdlKeyUp = 0x301,                  // SDL_KEYUP
    kSdlMouseButtonUp = 0x402,          // SDL_MOUSEBUTTONUP
    kSdlJoyButtonUp = 0x604,            // SDL_JOYBUTTONUP
    kSdlControllerButtonUp = 0x652,     // SDL_CONTROLLERBUTTONUP
    kSdlFingerUp = 0x701,               // SDL_FINGERUP
    kSdlWindowFocusLost = 0x0c,         // SDL_WINDOWEVENT_FOCUS_LOST
    kSdlWindowFocusGained = 0x0d,       // SDL_WINDOWEVENT_FOCUS_GAINED
};

// State shared between the movie player and the filter below; the player allocates it.
struct BinkEventFilterState
{
    unsigned char m_bSkip;                 // set by every skip-ish event
    unsigned char m_bQuit;                 // set only by SDL_QUIT
    unsigned char _pad02[30];
    unsigned long long m_nLastCounter;      // the last event's performance counter
    unsigned long long m_nDeadline;         // events before this are ignored
};

// Written by the movie player, so volatile here; the filter only ever reads the pointer once.
static BinkEventFilterState* volatile s_pBinkEventFilter;
static volatile unsigned char s_bFocusGained;

// Parses one line's "MM:SS - MM:SS <rest>" header into two second counts. Returns false for a
// malformed header; a line past the end reports true, which reads as "always show".
bool BinkSubtitles::GetTimeOnLine(int nLine, int& nStart, int& nEnd)
{
    if (static_cast<uint64_t>(nLine) >= m_lines.size())
        return true;

    const char* pszLine = m_lines[nLine];
    int nMin1 = 0;
    int nSec1 = 0;
    int nMin2 = 0;
    int nSec2 = 0;
    int nParsed = sscanf(pszLine, "%d:%d - %d:%d %*s", &nMin1, &nSec1, &nMin2, &nSec2);
    nStart = nMin1 * 60 + nSec1;
    nEnd = nMin2 * 60 + nSec2;
    return nParsed == 4;
}

// Reads <name>.<lang>sub.txt whole and splits it in place into lines. A missing file leaves the
// object empty, so every later row is a no-op.
BinkSubtitles::BinkSubtitles(const char* pszName)
{
    m_pszData = 0;

    const char* pszSuffix;
    switch (ASLPlat_GetCurrentLanguage())
    {
    case ASL_LANGUAGE_FRENCH:   pszSuffix = "fr"; break;
    case ASL_LANGUAGE_ITALIAN:  pszSuffix = "it"; break;
    case ASL_LANGUAGE_GERMAN:   pszSuffix = "de"; break;
    case ASL_LANGUAGE_SPANISH:  pszSuffix = "es"; break;
    default:                    pszSuffix = "";   break;
    }

    char szPath[256];
    snprintff(szPath, sizeof(szPath), "%s.%ssub.txt", pszName, pszSuffix);

    FILE* pFile = fopen(szPath, "rb");
    if (pFile != 0)
    {
        int nSize = FileSize(pFile);
        if (nSize > 0)
        {
            m_pszData = new char[nSize + 1];
            m_pszData[nSize] = 0;
            fread(m_pszData, 1, static_cast<uint64_t>(nSize), pFile);

            char* p = m_pszData;
            bool bNewLine = true;
            while (*p != 0)
            {
                if (bNewLine)
                    m_lines.push_back(p);
                bNewLine = false;
                if (*p == '\n')
                {
                    *p = '\0';
                    bNewLine = true;
                }
                if (*p == '\r')
                    *p = ' ';
                p++;
            }

            for (uint64_t i = 0; i < m_lines.size(); i++)
                printf("'%s'\n", m_lines[i]);
        }
        fclose(pFile);
    }
}

BinkSubtitles::~BinkSubtitles()
{
    delete[] m_pszData;
}

// Per-frame subtitle draw. What is drawn is the text after the line's time header; a line whose
// first non-time character is '<' is markup and is skipped.
void BinkSubtitles::Draw(float fTime)
{
    int nScreenWidth;
    int nScreenHeight;
    IosIsIphone(nScreenWidth, nScreenHeight);

    int nWidth = static_cast<int>(static_cast<float>(nScreenWidth) / static_cast<float>(nScreenHeight) * 768.0f);
    int nFontSize = s_nSubtitleFontSize;

    float fStep = static_cast<float>(nFontSize) * 1.1538461f;
    int nMaxWidth = nWidth - 100;
    int nY = 50;

    for (int i = 0; static_cast<size_t>(i) < m_lines.size(); i++)
    {
        int nStart;
        int nEnd;
        GetTimeOnLine(i, nStart, nEnd);

        if (static_cast<float>(nStart) < fTime && fTime <= static_cast<float>(nEnd) && m_lines[i] != 0)
        {
            char* p = m_lines[i];
            for (;;)
            {
                char c = *p;
                if (c == ' ')
                {
                    p++;
                    continue;
                }
                if (c == 0)
                    break;
                if ((c < '0' || c > '9') && c != '-' && c != ':')
                    break;
                p++;
            }

            if (*p != '<')
            {
                char szBuffer[256];
                snprintff(szBuffer, sizeof(szBuffer), "%s", p);
                DrawTextWrapped(szBuffer, true, nFontSize, nWidth, 768, 50, nY, nMaxWidth, 100, 2,
                                true);
                nY = static_cast<int>(fStep + static_cast<float>(nY));
            }
        }
    }
}

// A throttled SDL event filter. Every event stamps m_nLastCounter, and anything arriving before
// m_nDeadline is then ignored. A release ends the movie (the player polls m_bSkip); SDL_QUIT sets
// both bytes; a window focus change only records the new state.
void BinkEventFilter(SDL_Event* pEvent)
{
    BinkEventFilterState* pState = s_pBinkEventFilter;
    if (pState == 0)
        return;

    unsigned long long nNow = SDL_GetPerformanceCounter();
    pState->m_nLastCounter = nNow;
    if (nNow < pState->m_nDeadline)
        return;

    switch (pEvent->type)
    {
    case kSdlQuit:
        pState->m_bSkip = 1;
        pState->m_bQuit = 1;
        return;

    case kSdlWindowEvent:
        if (pEvent->window.event == kSdlWindowFocusGained)
        {
            s_bFocusGained = 1;
            return;
        }
        if (pEvent->window.event == kSdlWindowFocusLost)
        {
            s_bFocusGained = 0;
            return;
        }
        return;

    case kSdlKeyUp:
    case kSdlMouseButtonUp:
    case kSdlJoyButtonUp:
    case kSdlControllerButtonUp:
    case kSdlFingerUp:
        pState->m_bSkip = 1;
        return;

    default:
        return;
    }
}
