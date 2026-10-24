#include "exoiniinternal.h"

CExoIni::CExoIni()
{
    m_pInternal = new CExoIniInternal();
}

CExoIni::~CExoIni()
{
    CExoIniInternal* p = m_pInternal;
    if (p) {
        p->~CExoIniInternal();
        operator delete(p);
    }
    m_pInternal = 0;
}

int CExoIni::ReadIniEntry(CExoString& result, const CExoString& fileName, const CExoString& section, const CExoString& key)
{
    return m_pInternal->ReadIniEntry(result, fileName, section, key);
}

int CExoIni::WriteIniEntry(const CExoString& value, const CExoString& fileName, const CExoString& section, const CExoString& key)
{
    return m_pInternal->WriteIniEntry(value, fileName, section, key);
}
