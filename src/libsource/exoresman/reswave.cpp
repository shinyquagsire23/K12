#include "reswave.h"

CResWave::CResWave()
{
    m_bCompressed = 0;
    m_bModifiedWav = 0;
    m_bLoaded = 0;
    m_nWaveSize = 0;
    m_nReserved1 = 0;
}

CResWave::CResWave(unsigned int)
{
}

CResWave::~CResWave()
{
}

int CResWave::GetWaveSize()
{
    return m_nWaveSize;
}

int CResWave::IsLoaded()
{
    return m_bLoaded;
}

int CResWave::IsCompressed()
{
    return m_bCompressed;
}

int CResWave::IsModifiedWav()
{
    return m_bModifiedWav;
}

int CResWave::OnResourceFreed()
{
#if KOTOR1  // K1 reports the free as not handled
    return 0;
#else
    return 1;
#endif
}

int CResWave::OnResourceServiced()
{
#if !KOTOR1  // K1 does not flag the wave loaded here
    m_bLoaded = 1;
#endif
    return 1;
}

// An uncompressed, engine-"modified" wav carries a custom header whose payload member is the data;
// otherwise the buffer start is the data.
void* CResWave::GetData()
{
    if (!m_bCompressed && m_bModifiedWav == 1)
        return static_cast<ModifiedWavData*>(m_pFileData)->m_data;
    return m_pFileData;
}
