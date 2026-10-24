#include "k2stdint.h"
#include "exobase.h"
#include "exoiniinternal.h"

CExoBase::CExoBase()
{
    m_pDebug = new CExoDebug();
    m_pTimers = new CExoTimers();
    m_pAliasList = new CExoAliasList();
    m_pIni = new CExoIni();
    m_pRand = new CExoRand();
    m_pInternal = new CExoBaseInternal();
}

CExoBase::~CExoBase()
{
    CExoBaseInternal* pInternal = m_pInternal;
    if (pInternal != 0)
    {
        pInternal->~CExoBaseInternal();
        operator delete(pInternal);
        m_pInternal = 0;
    }
    CExoIni* pIni = m_pIni;
    if (pIni != 0)
    {
        pIni->~CExoIni();
        operator delete(pIni);
        m_pIni = 0;
    }
    CExoAliasList* pAliasList = m_pAliasList;
    if (pAliasList != 0)
    {
        pAliasList->~CExoAliasList();
        operator delete(pAliasList);
        m_pAliasList = 0;
    }
    CExoTimers* pTimers = m_pTimers;
    if (pTimers != 0)
    {
        pTimers->~CExoTimers();
        operator delete(pTimers);
        m_pTimers = 0;
    }
    CExoDebug* pDebug = m_pDebug;
    if (pDebug != 0)
    {
        pDebug->~CExoDebug();
        operator delete(pDebug);
        m_pDebug = 0;
    }
    CExoRand* pRand = m_pRand;
    if (pRand != 0)
    {
        pRand->~CExoRand();
        operator delete(pRand);
        m_pRand = 0;
    }
}

#if KOTOR1
int CExoBase::GetDirectoryList(CExoArrayList<CExoString>* pOut, const CExoString& dir, unsigned short types, int a, int b, int c)
{
    return m_pInternal->GetDirectoryList(pOut, dir, types, a, b, c);
}
#else
int CExoBase::GetDirectoryList(CExoArrayList<CExoString>* pOut, const CExoString& dir, unsigned short types, int a, int b)
{
    return m_pInternal->GetDirectoryList(pOut, dir, types, a, b);
}
#endif

unsigned int CExoBase::GetDirectorySize(const CExoString& path)
{
    return m_pInternal->GetDirectorySize(path);
}

int CExoBase::GetAugmentedDirectoryList(CExoArrayList<CFileInfo>* pOut, const CExoString& dir, unsigned short types, int flag)
{
    return m_pInternal->GetAugmentedDirectoryList(pOut, dir, types, flag);
}

const CExoString& CExoBase::GetResourceExtension(unsigned short resType)
{
    return m_pInternal->GetResourceExtension(resType);
}

unsigned short CExoBase::GetResTypeFromExtension(const CExoString& ext)
{
    return m_pInternal->GetResTypeFromExtension(ext);
}

void CExoBase::LoadAliases(const CExoString& path)
{
    m_pInternal->LoadAliases(path);
}

void CExoBase::ShutDown()
{
    m_pInternal->ShutDown();
}

void CExoBase::SpawnExternalApplication(const CExoString& path, const CExoString& args, int flag)
{
    m_pInternal->SpawnExternalApplication(path, args, flag);
}

void CExoBase::WcsToMbs(char* dst, const K2WChar* src, uint64_t n)
{
    m_pInternal->WcsToMbs(dst, src, n);
}

int CExoBase::CheckForCD()
{
    return m_pInternal->CheckForCD();
}
