#pragma once

class CExoString;
class CClientExoApp;
class CServerExoApp;
class CObjectTableManager;
struct CLoadGameErrorInfo;

class CAppManager {
public:
    // The shared scratch buffer m_pLargeTempBuffer points at.
    enum {
        LARGE_TEMP_BUFFER_SIZE = 256 * 1024,
    };

    CAppManager();
    ~CAppManager();

    void AllocLargeTempBuffer();
    void FreeLargeTempBuffer();
    CObjectTableManager* GetObjectTableManager(unsigned int type);
    void CreateServer(int flag);
    void DestroyServer();
    void TakeScreenShot(const CExoString& fileName);
    void DoSaveGameScreenShot(const CExoString& name);
    void ShowServerMem();

    unsigned char*       m_pLargeTempBuffer;
    CClientExoApp*       m_pClientExoApp;
    CServerExoApp*       m_pServerExoApp;
    CObjectTableManager* m_pObjectTableManager1;
    CObjectTableManager* m_pObjectTableManager2;
    CLoadGameErrorInfo*  m_pLoadGameErrorInfo;
    int                  m_appId;
    // The pause state cached across a focus loss, so the window can be unpaused back to it.
    int                  m_nPauseStateOnDeactivate;
};

extern CAppManager* g_pAppManager;

#include "port/win32api.h"
extern void SetWindowTitle();
#if KOTOR1
extern void AurSaveGameSnapshot(char* path, int value, bool flag, bool bSquare);
#else
extern void AurSaveGameSnapshot(char* path, int value, bool flag);
#endif

// The save/load progress and error record the app manager and the server share.

#include "exostring.h"

struct CLoadGameErrorInfo {
    // m_nStallEvent: the long operation the load bar is stalled on, as the StartStallEvent message
    // carries it to the client.
    enum StallEvent {
        STALL_EVENT_NONE        = 0,
        STALL_EVENT_LOAD_MODULE = 1,
        STALL_EVENT_SAVE_GAME   = 2,
        STALL_EVENT_LOAD_SAVE   = 3,
    };

    CLoadGameErrorInfo();

    int          m_flag;
    int          m_nStallEvent;
    union {
        unsigned long long m_nLoadBarPair;
        struct { int m_total; int m_progress; } m_loadBar;
    };
    int          m_bStallFirstFrame;
    int          m_bStallFinished;
    CExoString   m_saveDirectory;
    CExoString   m_saveGameName;
    CExoString   m_sCopyDstPath;
    int          m_nResType;     // the resource type of the save being loaded (0xbc1, 0x809, ...)
    char         _pad4c[0x50-0x4c];
    unsigned int m_errorCode;
    char         _pad54[0x58-0x54];
};
