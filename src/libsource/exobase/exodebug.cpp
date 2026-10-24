
#include "k2stdint.h"
#include "exodebug.h"
#include "exodebuginternal.h"
#include "exobase.h"

#include "port/libcstdlib.h"

// Empty in the shipped build.
void ExoFlashScreen(float, float, float)
{
}

#if KOTOR1  // K1's allocator pair takes no debug file and line
void* ExoMalloc(uint64_t nSize)
{
    return malloc(nSize);
}

void ExoFree(void* p)
{
    free(p);
}
#else
// The file and line arguments are for a caller-side debug build; the body ignores them.
void* EXOMALLOC(uint64_t nSize, const char*, int)
{
    return malloc(nSize);
}

void EXOFREE(void* p, const char*, int)
{
    free(p);
}
#endif

CExoDebug::CExoDebug()
{
    m_pInternal = new CExoDebugInternal();
    m_bEnableReleaseLogging = 0;
}

CExoDebug::~CExoDebug()
{
    if (m_pInternal) {
        delete m_pInternal;
        m_pInternal = 0;
    }
}

void CExoDebug::CloseLogFiles()
{
}

// Shuts the whole base library down afterwards.
void CExoDebug::Assert(int nLine, char* sFile, const char* sMsg)
{
    if (m_pInternal)
        m_pInternal->Assert(nLine, sFile, sMsg);
    g_pExoBase->ShutDown();
}

void CExoDebug::FlushErrorFile()
{
}

void CExoDebug::FlushLogFile()
{
}

void CExoDebug::FlushScriptLogFile()
{
    if (m_pInternal)
        m_pInternal->FlushScriptLogFile();
}

void CExoDebug::FlushResourcesLogFile()
{
    if (m_pInternal)
        m_pInternal->FlushResourcesLogFile();
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CExoDebug::FlushVOTestFile()
{
    if (m_pInternal)
        m_pInternal->FlushVOTestFile();
}
#endif // !KOTOR1

int CExoDebug::GetCurrentAllocatedMemory()
{
    return 0;
}

int CExoDebug::GetMaxAllocatedMemory()
{
    return 0;
}

int CExoDebug::GetTotalMemoryAllocations()
{
    return 0;
}

int CExoDebug::GetCurrentMemoryAllocations()
{
    return 0;
}

void CExoDebug::OpenLogFiles(const CExoString& sName)
{
    if (m_pInternal)
        m_pInternal->OpenLogFiles(sName);
}

void CExoDebug::Warning(int nLine, char* sFile, const char* sMsg)
{
    if (m_pInternal)
        m_pInternal->Warning(nLine, sFile, sMsg);
}

void CExoDebug::WriteToErrorFile(const CExoString& sText)
{
    K2_UNUSED(sText);
}

void CExoDebug::WriteToLogFile(const CExoString& sText)
{
    K2_UNUSED(sText);
}

void CExoDebug::WriteToResourcesLogFile(const CExoString& sText)
{
    if (m_pInternal)
        m_pInternal->WriteToResourcesLogFile(sText);
}

void CExoDebug::WriteToSoundLogFile(const CExoString& sText)
{
    K2_UNUSED(sText);
}

void CExoDebug::WriteToResourceTrackingFile(const CExoString& sText)
{
    if (m_pInternal)
        m_pInternal->WriteToResourceTrackingFile(sText);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CExoDebug::WriteToVOTestFile(const CExoString& sText)
{
    if (m_pInternal)
        m_pInternal->WriteToVOTestFile(sText);
}
#endif // !KOTOR1

void CExoDebug::WriteResourceToScreen(const CExoString& sText, int x, int y, float fDuration)
{
    if (m_pInternal)
        m_pInternal->WriteResourceToScreen(sText, x, y, fDuration);
}

void CExoDebug::GetCurrentTimestamp(CExoString& sOut)
{
    if (m_pInternal)
        m_pInternal->GetCurrentTimestamp(sOut);
}

void CExoDebug::WriteToScriptLogFile(const CExoString& sText)
{
    if (m_pInternal)
        m_pInternal->WriteToScriptLogFile(sText);
}
