#pragma once
#include "k2layout.h"

class CExoString;

// The random-number generator behind CExoRand.
class CExoRandInternal {
public:
    CExoRandInternal();
    ~CExoRandInternal();

    int  Rand();
    CExoString GetString(unsigned short nID);
    void SignalEvent();
    void SignalEvent(unsigned int nEvent);

    // Seed ring buffer size in bytes.
    static const unsigned short BUFFER_SIZE = 0x80;

private:
    void GenSeed();
    void SeedTimers();
    void Add(unsigned char* pData, int nLen);
    void ReSeed(unsigned int a, unsigned int b, unsigned int c, unsigned int d) throw();

    unsigned int m_anState[4];
    unsigned int m_anMask[4];
    unsigned int m_anRShift[4];
    unsigned int m_anBits[4];
    unsigned int m_anFeedbackShift[4];
    unsigned int m_anStepShift[4];
    unsigned int m_bSeeded;
    unsigned char* m_pEntropyRing;
    unsigned short m_nEntropyWritePos;
    unsigned long long m_anLastPerfCounter[4];
    unsigned int m_nLastEventTime;
};

K2_LAYOUT_ASSERT(sizeof(CExoRandInternal) == 0xa0, "CExoRandInternal is 0xa0");
