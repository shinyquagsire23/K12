#include "k2stdint.h"
#include <stddef.h>
#include "exodebuginternal.h"
#include "exofile.h"
#include "port/win32const.h"
#include <stdio.h>
#include <time.h>
#if KOTOR1
#include "sdlrwops.h"
#include "port/crtapi.h"
#endif

CExoDebugInternal::CExoDebugInternal()
{
    m_bLogFilesOpen = 0;
    // Only the first four file pointers are cleared here. The tail slots stay uninitialised until
    // OpenLogFile assigns them, and CloseLogFiles only frees them once m_bLogFilesOpen is set.
    __builtin_memset(&m_pLogFile, 0,
                     offsetof(CExoDebugInternal, m_pResourcesLogFile) + sizeof(m_pResourcesLogFile)
                     - offsetof(CExoDebugInternal, m_pLogFile));
}

CExoDebugInternal::~CExoDebugInternal()
{
    CloseLogFiles();
}

// Every Flush* and WriteTo* below tests the master flag, then calls the channel's CExoFile.

void CExoDebugInternal::FlushErrorFile()
{
    if (m_bLogFilesOpen)
        m_pErrorFile->Flush();
}

void CExoDebugInternal::FlushLogFile()
{
    if (m_bLogFilesOpen)
        m_pLogFile->Flush();
}

void CExoDebugInternal::FlushResourcesLogFile()
{
    if (m_bLogFilesOpen)
        m_pResourcesLogFile->Flush();
}

void CExoDebugInternal::FlushSoundLogFile()
{
    if (m_bLogFilesOpen)
        m_pSoundLogFile->Flush();
}

void CExoDebugInternal::FlushResourceTrackingFile()
{
    if (m_bLogFilesOpen)
        m_pResourceTrackingFile->Flush();
}

#if !KOTOR1  // K1 has no VO-test channel
void CExoDebugInternal::FlushVOTestFile()
{
    if (m_bLogFilesOpen)
        m_pVOTestFile->Flush();
}
#endif

void CExoDebugInternal::WriteToErrorFile(const CExoString& sText)
{
    if (m_bLogFilesOpen)
        m_pErrorFile->Write(sText);
}

void CExoDebugInternal::WriteToLogFile(const CExoString& sText)
{
    if (m_bLogFilesOpen)
        m_pLogFile->Write(sText);
}

void CExoDebugInternal::WriteToResourcesLogFile(const CExoString& sText)
{
    if (m_bLogFilesOpen)
        m_pResourcesLogFile->Write(sText);
}

void CExoDebugInternal::WriteToScriptLogFile(const CExoString& sText)
{
    if (m_bLogFilesOpen)
        m_pScriptLogFile->Write(sText);
}

void CExoDebugInternal::FlushScriptLogFile()
{
    if (m_bLogFilesOpen)
        m_pScriptLogFile->Flush();
}

void CExoDebugInternal::OpenLogFiles(const CExoString& sName)
{
    K2_UNUSED(sName);  // empty in the shipped build
}

void CExoDebugInternal::CloseLogFiles()
{
    if (m_bLogFilesOpen)
    {
        if (m_pLogFile)              { delete m_pLogFile;              m_pLogFile = 0; }
        if (m_pErrorFile)            { delete m_pErrorFile;            m_pErrorFile = 0; }
        if (m_pResourcesLogFile)     { delete m_pResourcesLogFile;     m_pResourcesLogFile = 0; }
        if (m_pSoundLogFile)         { delete m_pSoundLogFile;         m_pSoundLogFile = 0; }
        if (m_pResourceTrackingFile) { delete m_pResourceTrackingFile; m_pResourceTrackingFile = 0; }
        if (m_pScriptLogFile)        { delete m_pScriptLogFile;        m_pScriptLogFile = 0; }
#if !KOTOR1
        if (m_pVOTestFile)           { delete m_pVOTestFile;           m_pVOTestFile = 0; }
#endif
        m_bLogFilesOpen = 0;
    }
}

// The three channels that flush on every write.
void CExoDebugInternal::WriteToSoundLogFile(const CExoString& sText)
{
    if (m_bLogFilesOpen)
    {
        m_pSoundLogFile->Write(sText);
        FlushSoundLogFile();
    }
}

void CExoDebugInternal::WriteToResourceTrackingFile(const CExoString& sText)
{
    if (m_bLogFilesOpen)
    {
        m_pResourceTrackingFile->Write(sText);
        FlushResourceTrackingFile();
    }
}

#if !KOTOR1  // K1 has no VO-test channel
void CExoDebugInternal::WriteToVOTestFile(const CExoString& sText)
{
    if (m_bLogFilesOpen)
    {
        m_pVOTestFile->Write(sText);
        FlushVOTestFile();
    }
}
#endif

// Whether the serviced-resource overlay is on screen; set by the renderer.
unsigned char g_bExtendedResourceTracking;
bool g_bDisplayServicedResources;
#include "port/win32api.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"

void CExoDebugInternal::Assert(int nLine, char* sFile, const char* sMsg)
{
    CExoString sText;
    if (sMsg == 0)
        sText.Format(const_cast<char*>("An Assertion failed in %s\n\nLine number %d"), sFile, nLine);
    else
        sText.Format(const_cast<char*>("An Assertion failed in %s\n\nLine number %d \n\nProgrammer says: %s"),
                     sFile, nLine, sMsg);

    // Only the file write is guarded; the message box runs either way.
#if KOTOR1  // K1 opens the port's tracing stream instead
    FILE* pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exobase/exodebuginternal.cpp",
                         "Assert", 203, "Logs\\swcAsserts.txt", "at");
#else
    FILE* pFile = fopen("Logs\\swcAsserts.txt", "at");
#endif
    if (pFile)
        fputs(sText.CStr(), pFile);

    MessageBoxA(0, sText.CStr(), "Exo Assert", MB_ICONHAND | MB_SYSTEMMODAL);

    if (pFile)
        fclose(pFile);

    abort();
}

CExoFile* CExoDebugInternal::OpenLogFile(const CExoString& sName, CExoString sType)
{
    CExoString sLogFileName;
    CExoString sUnused;  // constructed and destroyed without ever being read
    sLogFileName.Format(const_cast<char*>("LOGS:%s%s"), sName.CStr(), sType.CStr());

    if (sType == CExoString("Tracking"))
    {
        if (g_bExtendedResourceTracking)
            return new CExoFile(sLogFileName, 10, CExoString("a"));
    }
    else
    {
        return new CExoFile(sLogFileName, 10, CExoString("w"));
    }
    return new CExoFile(sLogFileName, 10, CExoString("w"));
}

void CExoDebugInternal::WriteResourceToScreen(const CExoString& sText, int x, int y, float fDuration)
{
    K2_UNUSED(x);
    K2_UNUSED(y);
    K2_UNUSED(fDuration);
    if (g_bDisplayServicedResources)
        sText.CStr();
}

void CExoDebugInternal::GetCurrentTimestamp(CExoString& sOut)
{
    char szBuffer[20];
    time_t tNow;
    time(&tNow);
    sprintf(szBuffer, "%.19s", ctime(&tNow));
    sOut.Format(szBuffer);
}

void CExoDebugInternal::InitializeWarningFile()
{
    time_t tNow;
    char szBuffer[256];
    time(&tNow);
    sprintf(szBuffer, "\n--------------------------------------\n"
                      "Messages for: %s"
                      "--------------------------------------\n", ctime(&tNow));
    CExoString sHeader(szBuffer);

#if KOTOR1  // K1: an SDL stream at a lower-case relative path
    if (SDL_RWops* pFile = SDL_RWFromFile("logs/swcWarnings.txt", "at"))
    {
        SDL_RWwrite(pFile, sHeader.CStr(), 1, sHeader.GetLength());
        SDL_RWclose(pFile);
    }
#else
    if (FILE* pFile = fopen("{kApplicationSupportFolderType}\\Logs\\swcWarnings.txt", "at"))
    {
        fputs(sHeader.CStr(), pFile);
        fclose(pFile);
    }
#endif
}

void CExoDebugInternal::InitializeAssertFile()
{
    time_t tNow;
    char szBuffer[256];
    time(&tNow);
    sprintf(szBuffer, "\n--------------------------------------\n"
                      "Messages for: %s"
                      "--------------------------------------\n", ctime(&tNow));
    CExoString sHeader(szBuffer);

#if KOTOR1  // K1: an SDL stream at a lower-case relative path
    if (SDL_RWops* pFile = SDL_RWFromFile("logs/swcAsserts.txt", "at"))
    {
        SDL_RWwrite(pFile, sHeader.CStr(), 1, sHeader.GetLength());
        SDL_RWclose(pFile);
    }
#else
    if (FILE* pFile = fopen("Logs\\swcAsserts.txt", "at"))
    {
        fputs(sHeader.CStr(), pFile);
        fclose(pFile);
    }
#endif
}


// strlen with an explicit declaration so the loop below keeps its shape: pure merges the per-iteration
// length reads into one call, and noduplicate keeps the loop from being rotated.
#if !defined(K2_HOST_BUILD)  // a host keeps its C library's declaration
#if KOTOR1
extern "C" size_t strlen(const char* s) throw() __attribute__((pure));  // idiom-exempt: the loop's attributes
#else
extern "C" size_t strlen(const char* s) throw() __attribute__((pure, noduplicate));  // idiom-exempt: the loop's attributes
#endif
#endif

// Narrows a char string into a wide buffer, stopping at the bound or at the source's terminator.
K2WChar* ConvertCharToWChar(K2WChar* pDest, const char* pszSrc, unsigned int nMax)
{
#if KOTOR1  // K1 keeps a 32-bit index and truncates it at the exit
    unsigned int i = 0;
#else
    size_t i = 0;
#endif
    while (strlen(pszSrc) > i)
    {
        if (i == nMax - 1)
        {
            i = nMax - 1;
            break;
        }
        pDest[i] = pszSrc[i];
        i++;
    }
    pDest[i] = 0;
    return pDest;
}

void CExoDebugInternal::Warning(int nLine, char* sFile, const char* sMsg)
{
    CExoString sText;
    if (sMsg != 0)
        sText.Format(const_cast<char*>("A warning has occured:\n\n File: %s\n\nLine #: %d \n\nProgrammer says: %s"
                                      "\n\nPress Abort to quit, Retry to debug, or Ignore to continue"),
                     sFile, nLine, sMsg);
    else
        sText.Format(const_cast<char*>("A warning has occured:\n\n File: %s\n\nLine #: %d"
                                      "\n\nPress Abort to quit, Retry to debug, or Ignore to continue"),
                     sFile, nLine);

#if KOTOR1  // K1: an SDL stream at a lower-case relative path
    SDL_RWops* pFile = SDL_RWFromFile("logs/swcWarnings.txt", "at");
    if (pFile != 0)
        SDL_RWwrite(pFile, sText.CStr(), 1, sText.GetLength());
#else
    FILE* pFile = fopen("{kApplicationSupportFolderType}\\Logs\\swcWarnings.txt", "at");
    if (pFile != 0)
        fputs(sText.CStr(), pFile);
#endif

    CExoString sLine;
    CExoString sRest;
    char* pSlash = strrchr(sFile, '\\');
    sLine.Format(const_cast<char*>("WARNING: %s(%d) Comment:"), pSlash != 0 ? pSlash + 1 : sFile, nLine);

    sLine.CStr();
    if (sMsg == 0)
        goto done;

    sRest = sMsg;
#if KOTOR1  // K1: CExoString::GetLength is out of line, read once per pass
    for (;;)
    {
        int nLen = sRest.GetLength();
        if (nLen == 0)
            break;
#else
    while (sRest.m_sString ? static_cast<int>(strlen(sRest.m_sString)) : 0)
    {
        int nLen = sRest.m_sString ? static_cast<int>(strlen(sRest.m_sString)) : 0;
#endif
        if (nLen <= 50)
        {
            sLine = sRest;
            sRest = "";
        }
        else
        {
            sLine = sRest.Left(50);

            int nCut = 50;
            char* pSpace = strrchr(sLine.CStr(), ' ');
#if KOTOR1  // K1 cuts at a separator even in column 0
            if (pSpace != 0)
#else
            if (pSpace != 0 && pSpace != sLine.CStr())
#endif
                nCut = static_cast<int>(pSpace - sLine.CStr());

            char* pTab = strrchr(sLine.CStr(), '\t');
#if KOTOR1  // K1 cuts at a separator even in column 0
            if (pTab != 0)
#else
            if (pTab != 0 && pTab != sLine.CStr())
#endif
                nCut = static_cast<int>(pTab - sLine.CStr());

            char* pCR = strrchr(sLine.CStr(), '\r');
#if KOTOR1  // K1 cuts at a separator even in column 0
            if (pCR != 0)
#else
            if (pCR != 0 && pCR != sLine.CStr())
#endif
                nCut = static_cast<int>(pCR - sLine.CStr());

            sLine = sRest.Left(nCut);
            sRest = sRest.Right(nLen - nCut);
        }
        sLine.CStr();
    }

done:
    if (pFile != 0)
#if KOTOR1
        SDL_RWclose(pFile);
#else
        fclose(pFile);
#endif
}
