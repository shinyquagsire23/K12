#ifndef CEXODEBUG_H
#define CEXODEBUG_H

#include "exostring.h"

// The base library's debug and logging service, reached engine-wide through g_pExoBase->m_pDebug.
// Each method forwards to CExoDebugInternal behind a null guard; file logging and the memory counters
// are compiled out, so those bodies are empty.
//
// idiom-exempt: canonical -- this header is the CANONICAL home for CExoDebug; CExoBase.h (which
// owns the g_pExoBase->m_pDebug pointer) includes it rather than redeclaring a shortcut copy.
class CExoDebugInternal;

class CExoDebug {
public:
    CExoDebug();
    ~CExoDebug();

    void CloseLogFiles();  // stubbed

    // The engine's assert channel, called with the caller's __LINE__/__FILE__. `file` is char*, not
    // const char*, so call sites need the (char*)"..." cast.
    void Assert(int nLine, char* sFile, const char* sMsg);
    void Warning(int nLine, char* sFile, const char* sMsg);

    void FlushErrorFile();  // stubbed
    void FlushLogFile();  // stubbed
    void FlushScriptLogFile();
    void FlushResourcesLogFile();
    void FlushVOTestFile();

    int  GetCurrentAllocatedMemory();  // stubbed
    int  GetMaxAllocatedMemory();  // stubbed
    int  GetTotalMemoryAllocations();  // stubbed
    int  GetCurrentMemoryAllocations();  // stubbed

    void OpenLogFiles(const CExoString& sName);

    void WriteToErrorFile(const CExoString& sText);  // stubbed
    void WriteToLogFile(const CExoString& sText);  // stubbed
    void WriteToResourcesLogFile(const CExoString& sText);
    void WriteToSoundLogFile(const CExoString& sText);  // stubbed
    void WriteToResourceTrackingFile(const CExoString& sText);
    void WriteToVOTestFile(const CExoString& sText);
    void WriteToScriptLogFile(const CExoString& sText);

    void WriteResourceToScreen(const CExoString& sText, int x, int y, float fDuration);

    void GetCurrentTimestamp(CExoString& sOut);

    // Formatting scratch buffer, shared by NWScript's PrintString and WriteToScriptLogFile.
    CExoString         m_sScratch;
    CExoDebugInternal* m_pInternal;
    int                m_bEnableReleaseLogging;     // zeroed by the ctor; role unknown
};

#endif
