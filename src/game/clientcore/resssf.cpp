#include "resssf.h"

CResSSF::~CResSSF()
{
}

CResSSF::CResSSF()
{
    m_bLoaded = 0;
}

int CResSSF::OnResourceServiced()
{
    if (m_pFileData == 0)
        return 0;

    if (!m_bLoaded)
        m_bLoaded = 1;

    return 1;
}

int CResSSF::IsLoaded()
{
    return m_bLoaded;
}

int CResSSF::OnResourceFreed()
{
    m_bLoaded = 0;
    return 1;
}
