#include "k2stdint.h"
#include "resltr.h"
#include "resgff.h"

#include "port/libcstring.h"

// The LTR file's tag and version, read as little-endian words.
enum {
    FOURCC_LTR = 0x2052544c,   // "LTR "
    FOURCC_V1_0 = 0x302e3156,  // "V1.0"
};

CResLTR::CResLTR()
{
    __builtin_memset(m_pTrigram, 0, sizeof(m_pTrigram));
    __builtin_memset(m_pBigram, 0, sizeof(m_pBigram));
    __builtin_memset(m_pUnigram, 0, sizeof(m_pUnigram));
    m_pHeader = 0;
    m_pTableData = 0;
    m_nTableSize = 0;
    m_bLoaded = 0;
}

CResLTR::~CResLTR()
{
}

// Frees the parsed tables innermost-first.
int CResLTR::OnResourceFreed()
{
    m_bLoaded = 0;

    if (m_pUnigram[0]) { delete m_pUnigram[0]; m_pUnigram[0] = 0; }
    if (m_pUnigram[1]) { delete m_pUnigram[1]; m_pUnigram[1] = 0; }
    if (m_pUnigram[2]) { delete m_pUnigram[2]; m_pUnigram[2] = 0; }

    for (uint64_t i = 0; i < m_pHeader->m_nCodeCount; i++) {
        if (m_pBigram[0] && m_pBigram[0][i]) delete m_pBigram[0][i];
        if (m_pBigram[1] && m_pBigram[1][i]) delete m_pBigram[1][i];
        if (m_pBigram[2] && m_pBigram[2][i]) delete m_pBigram[2][i];
    }
    if (m_pBigram[0]) { delete m_pBigram[0]; m_pBigram[0] = 0; }
    if (m_pBigram[1]) { delete m_pBigram[1]; m_pBigram[1] = 0; }
    if (m_pBigram[2]) { delete m_pBigram[2]; m_pBigram[2] = 0; }

    for (uint64_t i = 0; i < m_pHeader->m_nCodeCount; i++) {
        for (uint64_t j = 0; j < m_pHeader->m_nCodeCount; j++) {
            if (m_pTrigram[0][i][j]) delete m_pTrigram[0][i][j];
            if (m_pTrigram[1][i][j]) delete m_pTrigram[1][i][j];
            if (m_pTrigram[2][i][j]) delete m_pTrigram[2][i][j];
        }
        if (m_pTrigram[0][i]) delete m_pTrigram[0][i];
        if (m_pTrigram[1][i]) delete m_pTrigram[1][i];
        if (m_pTrigram[2][i]) delete m_pTrigram[2][i];
    }
    if (m_pTrigram[0]) { delete m_pTrigram[0]; m_pTrigram[0] = 0; }
    if (m_pTrigram[1]) { delete m_pTrigram[1]; m_pTrigram[1] = 0; }
    if (m_pTrigram[2]) { delete m_pTrigram[2]; m_pTrigram[2] = 0; }

    m_pHeader = 0;
    m_pTableData = 0;
    m_nTableSize = 0;
    return 1;
}

// Parses the .ltr body: copy-then-ByteSwap per leaf, in the file's plane-minor order.
int CResLTR::OnResourceServiced()
{
    if (!m_pFileData)
        return 0;
    if (m_bLoaded)
        return 1;

    m_pHeader = static_cast<LTRHeader*>(m_pFileData);
    m_nTableSize = m_nSize - sizeof(LTRHeader);
    ByteSwap(&m_pHeader->m_dwSignature);
    ByteSwap(&m_pHeader->m_dwVersion);
    if (m_pHeader->m_dwSignature != FOURCC_LTR || m_pHeader->m_dwVersion != FOURCC_V1_0) {
        OnResourceFreed();
        return 0;
    }

    m_pUnigram[0] = new float[m_pHeader->m_nCodeCount];
    m_pUnigram[1] = new float[m_pHeader->m_nCodeCount];
    m_pUnigram[2] = new float[m_pHeader->m_nCodeCount];
    m_pBigram[0] = new float*[m_pHeader->m_nCodeCount];
    m_pBigram[1] = new float*[m_pHeader->m_nCodeCount];
    m_pBigram[2] = new float*[m_pHeader->m_nCodeCount];

    for (uint64_t i = 0; i < m_pHeader->m_nCodeCount; i++) {
        m_pBigram[0][i] = new float[m_pHeader->m_nCodeCount];
        m_pBigram[1][i] = new float[m_pHeader->m_nCodeCount];
        m_pBigram[2][i] = new float[m_pHeader->m_nCodeCount];
        if (!m_pBigram[0][i] || !m_pBigram[1][i] || !m_pBigram[2][i])
            return 0;
    }

    m_pTrigram[0] = new float**[m_pHeader->m_nCodeCount];
    m_pTrigram[1] = new float**[m_pHeader->m_nCodeCount];
    m_pTrigram[2] = new float**[m_pHeader->m_nCodeCount];

    for (unsigned char i = 0; i < m_pHeader->m_nCodeCount; i++) {
        m_pTrigram[0][i] = new float*[m_pHeader->m_nCodeCount];
        m_pTrigram[1][i] = new float*[m_pHeader->m_nCodeCount];
        m_pTrigram[2][i] = new float*[m_pHeader->m_nCodeCount];
        if (!m_pTrigram[0][i] || !m_pTrigram[1][i] || !m_pTrigram[2][i])
            return 0;
        for (unsigned char j = 0; j < m_pHeader->m_nCodeCount; j++) {
            m_pTrigram[0][i][j] = new float[m_pHeader->m_nCodeCount];
            m_pTrigram[1][i][j] = new float[m_pHeader->m_nCodeCount];
            m_pTrigram[2][i][j] = new float[m_pHeader->m_nCodeCount];
            if (!m_pTrigram[0][i][j] || !m_pTrigram[1][i][j] || !m_pTrigram[2][i][j])
                return 0;
        }
    }

    unsigned char* pBody = static_cast<unsigned char*>(m_pFileData) + sizeof(LTRHeader);
    m_pTableData = pBody;
    unsigned int nOffset = 0;

    memcpy(m_pUnigram[0], pBody + nOffset, m_pHeader->m_nCodeCount * sizeof(float));
    nOffset += m_pHeader->m_nCodeCount * sizeof(float);
    memcpy(m_pUnigram[1], pBody + nOffset, m_pHeader->m_nCodeCount * sizeof(float));
    nOffset += m_pHeader->m_nCodeCount * sizeof(float);
    memcpy(m_pUnigram[2], pBody + nOffset, m_pHeader->m_nCodeCount * sizeof(float));
    nOffset += m_pHeader->m_nCodeCount * sizeof(float);

    for (uint64_t i = 0; i < m_pHeader->m_nCodeCount; i++) {
        ByteSwap(&m_pUnigram[0][i]);
        ByteSwap(&m_pUnigram[1][i]);
        ByteSwap(&m_pUnigram[2][i]);

        memcpy(m_pBigram[0][i], pBody + nOffset, m_pHeader->m_nCodeCount * sizeof(float));
        nOffset += m_pHeader->m_nCodeCount * sizeof(float);
        memcpy(m_pBigram[1][i], pBody + nOffset, m_pHeader->m_nCodeCount * sizeof(float));
        nOffset += m_pHeader->m_nCodeCount * sizeof(float);
        memcpy(m_pBigram[2][i], pBody + nOffset, m_pHeader->m_nCodeCount * sizeof(float));
        nOffset += m_pHeader->m_nCodeCount * sizeof(float);

        for (uint64_t j = 0; j < m_pHeader->m_nCodeCount; j++) {
            ByteSwap(&m_pBigram[0][i][j]);
            ByteSwap(&m_pBigram[1][i][j]);
            ByteSwap(&m_pBigram[2][i][j]);
        }
    }

    for (uint64_t i = 0; i < m_pHeader->m_nCodeCount; i++) {
        for (unsigned char j = 0; j < m_pHeader->m_nCodeCount; j++) {
            memcpy(m_pTrigram[0][i][j], pBody + nOffset, m_pHeader->m_nCodeCount * sizeof(float));
            nOffset += m_pHeader->m_nCodeCount * sizeof(float);
            memcpy(m_pTrigram[1][i][j], pBody + nOffset, m_pHeader->m_nCodeCount * sizeof(float));
            nOffset += m_pHeader->m_nCodeCount * sizeof(float);
            memcpy(m_pTrigram[2][i][j], pBody + nOffset, m_pHeader->m_nCodeCount * sizeof(float));
            nOffset += m_pHeader->m_nCodeCount * sizeof(float);

            for (unsigned char k = 0; k < m_pHeader->m_nCodeCount; k++) {
                ByteSwap(&m_pTrigram[0][i][j][k]);
                ByteSwap(&m_pTrigram[1][i][j][k]);
                ByteSwap(&m_pTrigram[2][i][j][k]);
            }
        }
    }

    if (nOffset > m_nTableSize)
        return 0;
    m_bLoaded = 1;
    return 1;
}
