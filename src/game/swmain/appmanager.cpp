#include "swgui.h"
#include "appmanager.h"
#include "ObjectLookupTable.h"
#include "exostring.h"
#include <stddef.h>

#include "serverexoapp.h"

class CSWGuiManager;
class CClientExoAppInternal;

#include "clientexoapp.h"


// The classes CAppManager owns pointers to are declared here rather than in CAppManager.h, so that
// header stays includable from units with their own view of them.

CLoadGameErrorInfo::CLoadGameErrorInfo() {
    __builtin_memset(&m_flag, 0, offsetof(CLoadGameErrorInfo, m_saveDirectory));
}

CAppManager::CAppManager() {
    m_pServerExoApp = 0;
    m_pClientExoApp = 0;
    m_appId = GetTickCount();

    m_pClientExoApp = new CClientExoApp();

    m_pLoadGameErrorInfo = new CLoadGameErrorInfo();

    m_pObjectTableManager1 = new CObjectTableManager();
    m_pObjectTableManager2 = new CObjectTableManager();

    m_pLargeTempBuffer = 0;
    m_pLargeTempBuffer = new unsigned char[CAppManager::LARGE_TEMP_BUFFER_SIZE];
}

CAppManager::~CAppManager() {
    CClientExoApp* pClient = m_pClientExoApp;
    if (pClient) {
        pClient->CClientExoApp::~CClientExoApp();
        operator delete(pClient);
        m_pClientExoApp = 0;
    }

    if (m_pServerExoApp) {
        m_pServerExoApp->Uninitialize();
        m_pServerExoApp->StopServices();
        CServerExoApp* pServer = m_pServerExoApp;
        if (pServer) {
            pServer->CServerExoApp::~CServerExoApp();
            operator delete(pServer);
        }
        m_pServerExoApp = 0;
    }
    SetWindowTitle();

    CObjectTableManager* pOTM1 = m_pObjectTableManager1;
    if (pOTM1) {
        pOTM1->~CObjectTableManager();
        operator delete(pOTM1);
        m_pObjectTableManager1 = 0;
    }

    CObjectTableManager* pOTM2 = m_pObjectTableManager2;
    if (pOTM2) {
        pOTM2->~CObjectTableManager();
        operator delete(pOTM2);
        m_pObjectTableManager2 = 0;
    }

    CLoadGameErrorInfo* pErr = m_pLoadGameErrorInfo;
    if (pErr) {
        pErr->~CLoadGameErrorInfo();
        operator delete(pErr);
        m_pLoadGameErrorInfo = 0;
    }

    if (m_pLargeTempBuffer) {
        // A new[] buffer released with scalar delete.
#ifdef K2_FIX_ENGINE_BUGS
        delete[] m_pLargeTempBuffer;
#else
        delete m_pLargeTempBuffer;
#endif
        m_pLargeTempBuffer = 0;
    }
}

void CAppManager::AllocLargeTempBuffer() {
    if (!m_pLargeTempBuffer) {
        m_pLargeTempBuffer = new unsigned char[CAppManager::LARGE_TEMP_BUFFER_SIZE];
    }
}

void CAppManager::FreeLargeTempBuffer() {
    unsigned char* p = m_pLargeTempBuffer;
    if (p) {
        delete p;
        m_pLargeTempBuffer = 0;
    }
}

CObjectTableManager* CAppManager::GetObjectTableManager(unsigned int type) {
    switch (type) {
    case 0: return m_pObjectTableManager1;
    case 1: return m_pObjectTableManager2;
    default: return 0;
    }
}

void CAppManager::CreateServer(int flag) {
    if (m_pServerExoApp) {
        m_pServerExoApp->Uninitialize();
        m_pServerExoApp->StopServices();
        CServerExoApp* pOld = m_pServerExoApp;
        if (pOld) {
            pOld->CServerExoApp::~CServerExoApp();
            operator delete(pOld);
        }
        m_pServerExoApp = 0;
        SetWindowTitle();
    }

    CServerExoApp* pNew = new CServerExoApp();
    m_pServerExoApp = pNew;
    pNew->StartServices(flag);
    m_pServerExoApp->Initialize();
}

void CAppManager::DestroyServer() {
    if (m_pServerExoApp) {
        m_pServerExoApp->Uninitialize();
        m_pServerExoApp->StopServices();
        CServerExoApp* pOld = m_pServerExoApp;
        if (pOld) {
            pOld->CServerExoApp::~CServerExoApp();
            operator delete(pOld);
        }
        m_pServerExoApp = 0;
    }
    SetWindowTitle();
}

void CAppManager::TakeScreenShot(const CExoString& fileName) {
    const char* path = fileName.CStr();
    CSWGuiManager* gui = m_pClientExoApp->GetSWGuiManager();
#if KOTOR1
    AurSaveGameSnapshot(const_cast<char*>(path), gui->m_nScreenWidth, false, true);
#else
    AurSaveGameSnapshot(const_cast<char*>(path), gui->m_nScreenWidth, false);
#endif
}

void CAppManager::DoSaveGameScreenShot(const CExoString& name) {
#if KOTOR1
    AurSaveGameSnapshot(static_cast<char*>(name.CStr()), 256, true, true);
#else
    AurSaveGameSnapshot(static_cast<char*>(name.CStr()), 256, true);
#endif
}

void CAppManager::ShowServerMem() {
    if (m_pClientExoApp) m_pClientExoApp->ShowServerMem();
}
