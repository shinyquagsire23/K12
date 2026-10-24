#ifndef CEXOBASE_H
#define CEXOBASE_H
#include "k2stdint.h"

#include "exostring.h"
#include "exotimersinternal.h"
#include "exodebug.h"
#include "exobaseinternal.h"

// Base-lib subsystem owner (g_pExoBase). Its constructor heap-allocates six subsystem singletons
// and stores their pointers as fields, and its destructor tears all six back down. The
// public accessor surface is a pure pimpl forwarder to CExoBaseInternal.
//
// idiom-exempt: canonical -- this header is the CANONICAL home for CExoBase and the 5 subsystem
// singletons it owns (CExoIni/CExoDebug/CExoAliasList/CExoRand/CExoBaseInternal); CExoTimers'
// canonical home is the sibling CExoTimers.h, included above. Consumers #include this header
// rather than redeclaring a local copy.

template<class T> class CExoArrayList;
class CFileInfo;

class CExoIni;
class CExoAliasListInternal;
class CExoRandInternal;

// Alias-path resolver, giving the real filesystem path for a resource-manager alias name. The
// public surface forwards to CExoAliasListInternal.
class CExoAliasList {
public:
    CExoAliasList();
    ~CExoAliasList();

    void Add(const CExoString& alias, const CExoString& path);
    void ChangePath(const CExoString& alias, const CExoString& path);
    void Clear();
    void Delete(const CExoString& alias);

    // Maps a bare alias name to its real filesystem directory, returning a reference to an
    // internally cached string.
    const CExoString& GetAliasPath(const CExoString& alias, int flag) const;

    CExoString ResolveFileName(const CExoString& path, unsigned short resType) const;
private:
    CExoAliasListInternal* m_pInternal;
};

// Base-lib random-number source; the public surface forwards to CExoRandInternal.
class CExoRand {
public:
    CExoRand();
    ~CExoRand();

    int  Rand();
    CExoString GetString(unsigned short nID);
    void SignalEvent();
    void SignalEvent(unsigned int nEvent);
private:
    CExoRandInternal* m_pInternal;
};

// Implementation behind CExoBase's accessor surface. Every CExoBase method is a pure tail-call
// forwarder to the same-named method here.
class CExoBase {
public:
    CExoBase();
    ~CExoBase();

    unsigned int GetDirectorySize(const CExoString& path);
#if KOTOR1  // K1 has only the six-argument form; the K2 spelling forwards with c = 0
    int GetDirectoryList(CExoArrayList<CExoString>* pOut, const CExoString& dir, unsigned short types, int a, int b, int c);
    int GetDirectoryList(CExoArrayList<CExoString>* pOut, const CExoString& dir, unsigned short types, int a, int b)
    {
        return GetDirectoryList(pOut, dir, types, a, b, 0);
    }
#else
    int GetDirectoryList(CExoArrayList<CExoString>* pOut, const CExoString& dir, unsigned short types, int a, int b);
#endif
    int GetAugmentedDirectoryList(CExoArrayList<CFileInfo>* pOut, const CExoString& dir, unsigned short types, int flag);
    const CExoString& GetResourceExtension(unsigned short resType);
    unsigned short GetResTypeFromExtension(const CExoString& ext);
    void LoadAliases(const CExoString& path);
    void ShutDown();
    void SpawnExternalApplication(const CExoString& path, const CExoString& args, int flag);
    void WcsToMbs(char* dst, const K2WChar* src, uint64_t n);
    int CheckForCD();

    CExoIni*         m_pIni;
    CExoTimers*       m_pTimers;
    CExoDebug*        m_pDebug;
    CExoAliasList*    m_pAliasList;
    CExoRand*         m_pRand;
    CExoBaseInternal*  m_pInternal;
};

// A global with default visibility, so every access goes through the global offset table. Do not
// mark it hidden.
extern CExoBase* g_pExoBase;

#endif
