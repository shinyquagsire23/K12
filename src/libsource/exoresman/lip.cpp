#include "k2stdint.h"
#include "lip.h"

#include "port/libcstring.h"
#if KOTOR1
void ByteSwap(unsigned int* p);
void ByteSwap(float* p);
#endif

CLIP::CLIP()
{
    m_nNumVisemes = 0;
    m_fAnimLength = 0.0f;
    m_pData = 0;
}

CLIP::~CLIP()
{
    UnloadLip();
    m_pData = 0;
}

void CLIP::UnloadLip()
{
    if (m_bDemanded) {
        m_pRes->Release();
        m_bDemanded = 0;
    }
}

int CLIP::LoadLip(CExoString sResRef)
{
    SetResRef(CResRef(sResRef), 0);

    if (!m_pRes->Demand())
        return 0;

    m_pData = static_cast<char*>(m_pRes->GetLIPDataPtr());
    if (strncmp("LIP V1.0", m_pData, 8) != 0)
        return 0;

    m_pData += 8;
    memcpy(&m_fAnimLength, m_pData, sizeof(m_fAnimLength));
    m_pData += 4;
#if KOTOR1
    // K1 byte-swaps the header values in place.
    ByteSwap(&m_fAnimLength);
#endif
    memcpy(&m_nNumVisemes, m_pData, sizeof(m_nNumVisemes));
    m_pData += 4;
#if KOTOR1
    ByteSwap(&m_nNumVisemes);
#endif

    m_bDemanded = 1;
    return 1;
}

unsigned int CLIP::GetNumVisemes()
{
    return m_nNumVisemes;
}

float CLIP::GetAnimLength()
{
    return m_fAnimLength;
}

unsigned int CLIP::GetVisemeIndex(unsigned int nKeyFrame)
{
#if KOTOR1
    // K1 copies the byte into a zeroed word before swapping.
    if (nKeyFrame < m_nNumVisemes) {
        unsigned int nViseme = 0;
        __builtin_memcpy(&nViseme, &reinterpret_cast<const LIPKeyFrame*>(m_pData)[nKeyFrame].nViseme,
                         sizeof(LIPKeyFrame::nViseme));
        ByteSwap(&nViseme);
        return nViseme;
    }
    return 0;
#else
    if (nKeyFrame >= m_nNumVisemes)
        return 0;
    return reinterpret_cast<const LIPKeyFrame*>(m_pData)[nKeyFrame].nViseme;
#endif
}

float CLIP::GetTimeIndex(unsigned int nKeyFrame)
{
#if KOTOR1
    if (nKeyFrame < m_nNumVisemes) {
        float fTime = 0.0f;
        __builtin_memcpy(&fTime, &reinterpret_cast<const LIPKeyFrame*>(m_pData)[nKeyFrame].fTime, sizeof(fTime));
        ByteSwap(&fTime);
        return fTime;
    }
    return 0.0f;
#else
    if (nKeyFrame >= m_nNumVisemes)
        return 0.0f;
    return reinterpret_cast<const LIPKeyFrame*>(m_pData)[nKeyFrame].fTime;
#endif
}
