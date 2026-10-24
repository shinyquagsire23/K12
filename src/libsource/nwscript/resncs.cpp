#include "resncs.h"

CResNCS::CResNCS()
{
    m_bLoaded = 0;
    m_nNCSSize = 0;
    m_pNCSData = 0;
}

CResNCS::~CResNCS()
{
}

void* CResNCS::GetNCSDataPtr()
{
    return m_pNCSData;
}

int CResNCS::GetNCSSize()
{
    return m_nNCSSize;
}

int CResNCS::IsLoaded()
{
    return m_bLoaded;
}

int CResNCS::OnResourceFreed()
{
    m_bLoaded = 0;
    m_nNCSSize = 0;
    m_pNCSData = 0;
    return 1;
}

int CResNCS::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;
    if (m_bLoaded)
        return 1;
    m_pNCSData = m_pFileData;
    m_bLoaded = 1;
    m_nNCSSize = m_nSize;
    return 1;
}
