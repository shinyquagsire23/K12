#include "k2stdint.h"
#include "port/sdlapi.h"
#include "exorandinternal.h"
#include "exostring.h"

#include "port/win32api.h"
#include "port/libcstring.h"

// GetSystemInfo's out-parameter: only its size matters, and the bytes go to Add() opaque.
struct _SYSTEM_INFO {
    unsigned long long w0, w1, w2, w3, w4, w5;
};

#ifdef K2_FIX_ENGINE_BUGS
extern "C" void k2host_entropy(unsigned char* pOut, int nLen);
#endif

#if !KOTOR1
void CExoRandInternal::SeedTimers()
{
    long long llCounter;
    if (QueryPerformanceCounter(&llCounter))
        Add(reinterpret_cast<unsigned char*>(&llCounter), sizeof(llCounter));

    _SYSTEM_INFO si;
    GetSystemInfo(&si);
    Add(reinterpret_cast<unsigned char*>(&si), sizeof(si));
}
#endif

// The four generators' state before the first seeding.
enum : unsigned int {
    RAND_INITIAL_STATE_0 = 0x1745d12d,
    RAND_INITIAL_STATE_1 = 0x249248ed,
    RAND_INITIAL_STATE_2 = 0x333332f9,
    RAND_INITIAL_STATE_3 = 0x383ef9fa,
};

CExoRandInternal::CExoRandInternal()
{
    CExoString sSeed;

    m_bSeeded = 0;
    m_anState[0] = RAND_INITIAL_STATE_0;
    m_anState[1] = RAND_INITIAL_STATE_1;
    m_anState[2] = RAND_INITIAL_STATE_2;
    m_anState[3] = RAND_INITIAL_STATE_3;
    m_anBits[0] = 31;
    m_anBits[1] = 29;
    m_anBits[2] = 28;
    m_anBits[3] = 25;
    m_anFeedbackShift[0] = 6;
    m_anFeedbackShift[1] = 2;
    m_anFeedbackShift[2] = 13;
    m_anFeedbackShift[3] = 3;
    m_anStepShift[0] = 20;
    m_anStepShift[1] = 12;
    m_anStepShift[2] = 7;
    m_anStepShift[3] = 15;
    m_nEntropyWritePos = 0;
    m_pEntropyRing = new unsigned char[BUFFER_SIZE];
#ifdef K2_FIX_ENGINE_BUGS
    k2host_entropy(m_pEntropyRing, BUFFER_SIZE);
#endif

#if !KOTOR1  // K1 seeds from the string pass alone
    SeedTimers();
#endif

    sSeed = GetString(16);
    unsigned int nSeed0 = *reinterpret_cast<unsigned int*>(sSeed.SubString(0, 4).CStr());
    unsigned int nSeed1 = *reinterpret_cast<unsigned int*>(sSeed.SubString(4, 4).CStr());
    unsigned int nSeed2 = *reinterpret_cast<unsigned int*>(sSeed.SubString(8, 4).CStr());
    unsigned int nSeed3 = *reinterpret_cast<unsigned int*>(sSeed.SubString(12, 4).CStr());
    ReSeed(nSeed0, nSeed1, nSeed2, nSeed3);

#if KOTOR1
    long long llCounter = SDL_GetPerformanceCounter();
#else
    long long llCounter;
    QueryPerformanceCounter(&llCounter);
#endif
    m_anLastPerfCounter[0] = llCounter;
    m_anLastPerfCounter[1] = llCounter;
    m_anLastPerfCounter[2] = llCounter;
    m_anLastPerfCounter[3] = llCounter;
    m_nLastEventTime = GetTickCount();
}

void CExoRandInternal::GenSeed()
{
#ifdef K2_FIX_ENGINE_BUGS
    k2host_entropy(m_pEntropyRing, BUFFER_SIZE);
#else
    unsigned char aStackEntropy[BUFFER_SIZE];
    __builtin_memcpy(m_pEntropyRing, aStackEntropy, BUFFER_SIZE);  // idiom-exempt: fixed-size ring clear, inlined in the orig
#endif
#if !KOTOR1
    SeedTimers();
#endif
}

void CExoRandInternal::Add(unsigned char* pData, int nLen)
{
    if (m_nEntropyWritePos + nLen <= BUFFER_SIZE - 1)
    {
        memcpy(m_pEntropyRing + m_nEntropyWritePos, pData, nLen);  // idiom-exempt: byte count comes from the caller
        m_nEntropyWritePos += nLen;
    }
    else
    {
        memcpy(m_pEntropyRing + m_nEntropyWritePos, pData, BUFFER_SIZE - m_nEntropyWritePos);  // idiom-exempt: byte count is the ring's, not an object
        memcpy(m_pEntropyRing, pData + BUFFER_SIZE - m_nEntropyWritePos, nLen - (BUFFER_SIZE - m_nEntropyWritePos));  // idiom-exempt: same
        m_nEntropyWritePos = static_cast<unsigned short>(m_nEntropyWritePos - BUFFER_SIZE + nLen);
    }
}

// One round of the four-lane state step, then the xor fold of the new state.
int CExoRandInternal::Rand()
{
    for (int i = 0; i < 4; i++)
    {
        unsigned int nState = m_anState[i];
        unsigned int nShifted = nState ^ (nState << m_anFeedbackShift[i]);
        m_anState[i] = (nShifted >> m_anRShift[i]) ^ ((nState & m_anMask[i]) << m_anStepShift[i]);
    }
    return m_anState[0] ^ m_anState[1] ^ m_anState[2] ^ m_anState[3];
}

// Rebuilds the per-lane derived vectors, clamps each seed up to its lane's minimum, then runs one
// step.
void CExoRandInternal::ReSeed(unsigned int a, unsigned int b, unsigned int c, unsigned int d) throw()
{
    unsigned int anSeed[4];
    anSeed[0] = a;
    anSeed[1] = b;
    anSeed[2] = c;
    anSeed[3] = d;

    m_bSeeded = 1;

    for (int i = 0; i < 4; i++)
    {
        m_anRShift[i] = m_anBits[i] - m_anStepShift[i];
        m_anMask[i] = 0xffffffffu << (32 - m_anBits[i]);
    }

    for (int i = 0; i < 4; i++)
    {
        if (anSeed[i] > (1u << m_anRShift[i]))
            m_anState[i] = anSeed[i];
    }

    Rand();
}

void CExoRandInternal::SignalEvent(unsigned int nEvent)
{
    unsigned short nDelta = static_cast<unsigned short>(m_nLastEventTime - nEvent);
    if (nDelta)
    {
        Add(reinterpret_cast<unsigned char*>(&nDelta), 2);
        m_nLastEventTime = nEvent;
    }
}

// Stretches the entropy ring into a string of `nID` bytes: XOR-folds the ring into the first block,
// replaces every NUL with a random 1..250 byte, then feeds the result back into the ring. `nID` is a
// divisor, so callers must not pass zero.
CExoString CExoRandInternal::GetString(unsigned short nID)
{
    unsigned short nBlocks = BUFFER_SIZE / nID;
    unsigned short nRem = static_cast<unsigned short>(BUFFER_SIZE % nID);

    char* pBuf = new char[nID + 1];
    CExoString sRet;
#ifdef K2_FIX_ENGINE_BUGS
    // A string longer than the ring starts from the whole ring and zeroes the rest.
    unsigned short nCopy = (nID < BUFFER_SIZE) ? nID : BUFFER_SIZE;
    memcpy(pBuf, m_pEntropyRing, nCopy);  // idiom-exempt: a runtime byte count, not an object size
    memset(pBuf + nCopy, 0, nID - nCopy);  // idiom-exempt: a runtime byte count, not an object size
#else
    memcpy(pBuf, m_pEntropyRing, nID);  // idiom-exempt: nID is the caller's byte count, not an object size
#endif

    for (int i = 1; i < nBlocks; i++)
    {
        for (int k = 0; k < nID; k++)
            pBuf[k] ^= m_pEntropyRing[k + i * nID];
    }

    K2_ASSUME(nRem < nID);   // a remainder of division by nID
    for (int k = 0; k < nRem; k++)
        pBuf[k] ^= m_pEntropyRing[nBlocks * nID + k];

    for (int i = 0; i < nID; i++)
    {
        if (pBuf[i] == 0)
            pBuf[i] = static_cast<char>(static_cast<unsigned int>(Rand()) % 250 + 1);
    }

    pBuf[nID] = 0;
    Add(reinterpret_cast<unsigned char*>(pBuf), nID);
    sRet = pBuf;
    delete[] pBuf;
    return sRet;
}

// Picks one of the four tick counters by the low two bits of the random stream, feeds the wrap-around
// delta against a fresh performance counter into the ring, then stores the counter back.
void CExoRandInternal::SignalEvent()
{
#if KOTOR1
    long long llCounter = SDL_GetPerformanceCounter();
#else
    long long llCounter;
    QueryPerformanceCounter(&llCounter);
#endif

    uint64_ll_t* pSlot = &m_anLastPerfCounter[0];
    unsigned int nDelta;
#if KOTOR1
    unsigned short sDelta;
    unsigned char cDelta;
    switch (Rand() & 3)
    {
#else
    switch (Rand() & 3)
    {
        unsigned char cDelta;
#endif
    case 0:
        {
            pSlot = &m_anLastPerfCounter[0];
            nDelta = static_cast<unsigned int>(*pSlot) - static_cast<unsigned int>(llCounter);
            Add(reinterpret_cast<unsigned char*>(&nDelta), sizeof(nDelta));
            break;
        }
    case 1:
        {
            pSlot = &m_anLastPerfCounter[1];
            nDelta = static_cast<unsigned int>(*pSlot) - static_cast<unsigned int>(llCounter);
            Add(reinterpret_cast<unsigned char*>(&nDelta), sizeof(nDelta));
            break;
        }
    case 2:
        {
            pSlot = &m_anLastPerfCounter[2];
#if KOTOR1
            sDelta = static_cast<unsigned short>(*pSlot - llCounter);
#else
            unsigned short sDelta = static_cast<unsigned short>(*pSlot - llCounter);
#endif
            Add(reinterpret_cast<unsigned char*>(&sDelta), sizeof(sDelta));
            break;
        }
    case 3:
        {
            pSlot = &m_anLastPerfCounter[3];
            cDelta = static_cast<unsigned char>(*pSlot - llCounter);
            Add(static_cast<unsigned char*>(&cDelta), sizeof(cDelta));
            break;
        }
    }
    *pSlot = llCounter;
}

CExoRandInternal::~CExoRandInternal()
{
    if (m_pEntropyRing)
    {
#ifdef K2_FIX_ENGINE_BUGS
        delete[] m_pEntropyRing;
#else
        delete m_pEntropyRing;
#endif
        m_pEntropyRing = 0;
    }
}
