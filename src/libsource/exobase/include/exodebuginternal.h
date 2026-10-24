#pragma once

class CExoString;
class CExoFile;

// Log files behind CExoDebug, one per channel.
class CExoDebugInternal {
public:
    int    m_bLogFilesOpen;
    CExoFile* m_pLogFile;
    CExoFile* m_pScriptLogFile;
    CExoFile* m_pErrorFile;
    CExoFile* m_pResourcesLogFile;
    CExoFile* m_pSoundLogFile;
    CExoFile* m_pResourceTrackingFile;
#if !KOTOR1  // K1 has no VOTest channel
    CExoFile* m_pVOTestFile;
#endif

    CExoDebugInternal();
    ~CExoDebugInternal();

    void Assert(int nLine, char* sFile, const char* sMsg);
    void Warning(int nLine, char* sFile, const char* sMsg);
    void CloseLogFiles();
    void InitializeAssertFile();
    void InitializeWarningFile();
    void FlushErrorFile();
    void FlushLogFile();
    void FlushResourcesLogFile();
    void FlushSoundLogFile();
    void FlushResourceTrackingFile();
    void FlushScriptLogFile();
    void FlushVOTestFile();
    CExoFile* OpenLogFile(const CExoString& sName, CExoString sType);
    void OpenLogFiles(const CExoString& sName);
    void WriteToErrorFile(const CExoString& sText);
    void WriteToLogFile(const CExoString& sText);
    void WriteToResourcesLogFile(const CExoString& sText);
    void WriteToResourceTrackingFile(const CExoString& sText);
    void WriteToScriptLogFile(const CExoString& sText);
    void WriteToSoundLogFile(const CExoString& sText);
    void WriteToVOTestFile(const CExoString& sText);
    // Only sText is live; the screen position and duration are unused.
    void WriteResourceToScreen(const CExoString& sText, int x, int y, float fDuration);
    void GetCurrentTimestamp(CExoString& sOut);
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CExoDebugInternal) == 0x38, "CExoDebugInternal K1 size drift");
#endif
