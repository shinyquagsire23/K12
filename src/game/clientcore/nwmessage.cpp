#include "k2stdint.h"
#include "nwmessage.h"
#include "CExoArrayListImpl.h"
#include "resgff.h"
#include "gameobjectarray.h"

#include <string.h>
#include "port/libcbase.h"
#include "exostringinline.h"

void ByteSwap(unsigned short* p);
void ByteSwap(short* p);
void ByteSwap(unsigned int* p);
void ByteSwap(int* p);
void ByteSwap(float* p);
void ByteSwap(uint64_ll_t* p);
#if KOTOR1
void ByteSwap(int64_t* p);
#else
void ByteSwap(long long* p);
#endif
void ByteSwap(double* p);

const unsigned int CSWMessage::DEFAULT_WRITE_BUFFER_SIZE;

CSWMessage::CSWMessage()
    : m_pWriteMessage(new unsigned char[DEFAULT_WRITE_BUFFER_SIZE]),
      m_nWriteMessageBufferSize(DEFAULT_WRITE_BUFFER_SIZE),
      m_nWriteMessageSize(0),
      m_bWriteMessageOpen(0),
      m_pReadMessage(0),
      m_nReadMessageSize(0),
      m_nReadMessagePos(0)
{
    m_pWriteFragments = 0;
}

CSWMessage::~CSWMessage()
{
    if (m_pWriteMessage != 0) {
        delete[] m_pWriteMessage;
        m_pWriteMessage = 0;
    }
    if (m_pWriteFragments != 0) {
        delete[] m_pWriteFragments;
        m_pWriteFragments = 0;
    }
}

void CSWMessage::SetReadMessage(unsigned char* pBuffer, unsigned int nSize,
                                unsigned int a3, int a4)
{
    m_pReadMessage = pBuffer;
    m_nReadMessageSize = nSize;
    m_nReadMessagePos = 0;
    m_nReadFragmentsSize = 0;
    m_nReadFragmentsPos = 0;
    m_nPlayerId = a3;
    m_bGuaranteed = a4;

    if (nSize == 0)
        m_nReadBitIndex = m_nReadStartBitIndex;
}

void CSWMessage::ClearReadMessage()
{
    m_nReadMessagePos = m_nReadMessageSize;
}

unsigned char CSWMessage::ReadBit(unsigned char nBit)
{
    unsigned char nValue = m_pReadFragments[m_nReadFragmentsPos] & (1 << (7 - nBit));
    return nValue >> (7 - nBit);
}

unsigned long long CSWMessage::ReadBits(int nBits)
{
    unsigned long long nValue = 0;

    if (nBits > 0) {
        do {
            nValue = (nValue << 1) | (ReadBit(m_nReadBitIndex) & 1);
            m_nReadBitIndex++;
            if (m_nReadBitIndex == 8) {
                m_nReadFragmentsPos++;
                m_nReadBitIndex = 0;
            }
        } while (--nBits != 0);
    }

    return nValue;
}

void CSWMessage::WriteBit(unsigned char nValue)
{
    m_pWriteFragments[m_nWriteFragmentsPos] &= ~(1 << (7 - m_nWriteBitIndex));
    m_pWriteFragments[m_nWriteFragmentsPos] |= (nValue << (7 - m_nWriteBitIndex));
}

void CSWMessage::WriteBits(uint64_ll_t nValue, int nBits)
{
    while (nBits > 0) {
        nBits--;
        WriteBit(static_cast<unsigned char>((nValue >> nBits) & 1));
        m_nWriteBitIndex++;
        if (m_nWriteBitIndex == 8) {
            m_nWriteFragmentsPos++;
            m_nWriteBitIndex = 0;
        }
    }
}

void CSWMessage::WriteUnsigned(uint64_ll_t nValue, int nBits)
{
    unsigned int nExtra = static_cast<unsigned short>(nBits >> 3);
    if (m_nWriteMessageSize + nExtra > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(nExtra);

    ByteSwap(&nValue);

    if (nBits == 64) {
        WriteDWORD64(nValue, nBits);
        return;
    }

    while (nBits >= 32) {
        nBits -= 32;
        WriteDWORD(static_cast<unsigned int>(nValue >> nBits), nBits);
    }
    while (nBits >= 16) {
        nBits -= 16;
        WriteWORD(static_cast<unsigned short>(nValue >> nBits), nBits);
    }
    while (nBits >= 8) {
        nBits -= 8;
        WriteBYTE(static_cast<unsigned char>(nValue >> nBits), nBits);
    }

    if (nBits > 0)
        WriteBits(nValue & (0xff >> (8 - nBits)), nBits);
}

void CSWMessage::ExtendWriteFragmentsBuffer(unsigned int nExtra)
{
    K2_UNUSED(nExtra);
}

void CSWMessage::ExtendWriteBuffer(unsigned int nExtra)
{
    int nNewSize = nExtra + m_nWriteMessageBufferSize + DEFAULT_WRITE_BUFFER_SIZE;
    unsigned char* pNew = new unsigned char[nNewSize];

    unsigned char* pOld = m_pWriteMessage;
    memcpy(pNew, pOld, m_nWriteMessageSize);  // idiom-exempt: raw byte-count copy of a network buffer, there is no element type to take sizeof of
    if (pOld != 0)
        delete[] pOld;

    m_pWriteMessage = pNew;
    m_nWriteMessageBufferSize = nNewSize;
}

void CSWMessage::WriteDWORD64(uint64_ll_t nValue, int nBits)
{
    K2_UNUSED(nBits);
    ByteSwap(&nValue);

    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

void CSWMessage::WriteDWORD(unsigned int nValue, int nBits)
{
    K2_UNUSED(nBits);
    ByteSwap(&nValue);

    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

void CSWMessage::WriteWORD(unsigned short nValue, int nBits)
{
    K2_UNUSED(nBits);
    ByteSwap(&nValue);

    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

void CSWMessage::WriteBYTE(unsigned char nValue, int nBits)
{
    K2_UNUSED(nBits);
    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

// Sign bit in the fragments stream, magnitude through WriteUnsigned with one fewer bit.
#if KOTOR1
void CSWMessage::WriteSigned(int64_t nValue, int nBits)
#else
void CSWMessage::WriteSigned(long long nValue, int nBits)
#endif
{
    if (nValue >= 0) {
        WriteBit(0);
        m_nWriteBitIndex++;
        if (m_nWriteBitIndex == 8) {
            m_nWriteFragmentsPos++;
            m_nWriteBitIndex = 0;
        }
    } else {
        WriteBit(1);
        m_nWriteBitIndex++;
        if (m_nWriteBitIndex == 8) {
            m_nWriteFragmentsPos++;
            m_nWriteBitIndex = 0;
        }
        nValue = -nValue;
    }

    WriteUnsigned(nValue, nBits - 1);
}

unsigned long long CSWMessage::ReadUnsigned(int nBits)
{
    if (nBits == 64)
        return ReadDWORD64(nBits);

    unsigned long long nValue = 0;

    while (nBits >= 32) {
        if (m_nReadMessagePos + sizeof(unsigned int) > m_nReadMessageSize) {
            m_nReadMessagePos += sizeof(unsigned int);
            return 0;
        }
        nValue = (nValue << 32) | ReadDWORD(nBits);
        nBits -= 32;
    }
    while (nBits >= 16) {
        if (m_nReadMessagePos + sizeof(unsigned short) > m_nReadMessageSize) {
            m_nReadMessagePos += sizeof(unsigned short);
            return 0;
        }
        nValue = (nValue << 16) | ReadWORD(nBits);
        nBits -= 16;
    }
    while (nBits >= 8) {
        if (m_nReadMessagePos >= m_nReadMessageSize) {
            m_nReadMessagePos += 1;
            return 0;
        }
        nValue = (nValue << 8) | ReadBYTE(nBits);
        nBits -= 8;
    }

    if (nBits > 0) {
        unsigned char nEnd = static_cast<unsigned char>((nBits + m_nReadBitIndex - 1) >> 3);
        if (m_nReadFragmentsPos + nEnd > m_nReadFragmentsSize) {
            m_nReadFragmentsPos += nEnd;
            return 0;
        }
        nValue = (nValue << nBits) | ReadBits(nBits);
    }

    return nValue;
}

uint64_ll_t CSWMessage::ReadDWORD64(int nBits)
{
    K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(uint64_ll_t) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(uint64_ll_t);
        return 0;
    }

    uint64_ll_t nValue = *reinterpret_cast<uint64_ll_t*>(&m_pReadMessage[m_nReadMessagePos]);
    m_nReadMessagePos += sizeof(uint64_ll_t);
    ByteSwap(&nValue);
    return nValue;
}

unsigned int CSWMessage::ReadDWORD(int nBits)
{
    K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(unsigned int) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(unsigned int);
        return 0;
    }

    unsigned int nValue = *reinterpret_cast<unsigned int*>(&m_pReadMessage[m_nReadMessagePos]);
    m_nReadMessagePos += sizeof(unsigned int);
    ByteSwap(&nValue);
    return nValue;
}

unsigned short CSWMessage::ReadWORD(int nBits)
{
    K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(unsigned short) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(unsigned short);
        return 0;
    }

    unsigned short nValue = *reinterpret_cast<unsigned short*>(&m_pReadMessage[m_nReadMessagePos]);
    m_nReadMessagePos += sizeof(unsigned short);
    ByteSwap(&nValue);
    return nValue;
}

unsigned char CSWMessage::ReadBYTE(int a)
{
    K2_UNUSED(a);
    unsigned char nValue;

    if (m_nReadMessagePos < m_nReadMessageSize)
        nValue = m_pReadMessage[m_nReadMessagePos];
    else
        nValue = 0;

    m_nReadMessagePos += 1;
    return nValue;
}

long long CSWMessage::ReadSigned(int nBits)
{
    if (m_nReadFragmentsPos > m_nReadFragmentsSize)
        return 0;

    unsigned char bNegative = ReadBit(m_nReadBitIndex);
    m_nReadBitIndex++;
    if (m_nReadBitIndex == 8) {
        m_nReadFragmentsPos++;
        m_nReadBitIndex = 0;
    }

    long long nValue = ReadUnsigned(nBits - 1);
    if (bNegative) {
        if (nValue == 0)
            return -0x7fffffffffffffffLL - 1;
        return -nValue;
    }

    return nValue;
}

int CSWMessage::ReadBOOL()
{
    int nValue;

    if (m_nReadMessagePos + sizeof(nValue) <= m_nReadMessageSize)
        nValue = *reinterpret_cast<int*>(&m_pReadMessage[m_nReadMessagePos]);
    else
        nValue = 0;

    m_nReadMessagePos += sizeof(nValue);
    return nValue;
}

char CSWMessage::ReadCHAR(int a)
{
    K2_UNUSED(a);
    char nValue;

    if (m_nReadMessagePos < m_nReadMessageSize)
        nValue = m_pReadMessage[m_nReadMessagePos];
    else
        nValue = 0;

    m_nReadMessagePos += 1;
    return nValue;
}

short CSWMessage::ReadSHORT(int nBits)
{
    K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(short) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(short);
        return 0;
    }

    short nValue = *reinterpret_cast<short*>(&m_pReadMessage[m_nReadMessagePos]);
    m_nReadMessagePos += sizeof(short);
    ByteSwap(&nValue);
    return nValue;
}

int CSWMessage::ReadINT(int nBits)
{
    K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(int) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(int);
        return 0;
    }

    int nValue = *reinterpret_cast<int*>(&m_pReadMessage[m_nReadMessagePos]);
    m_nReadMessagePos += sizeof(int);
    ByteSwap(&nValue);
    return nValue;
}

long long CSWMessage::ReadINT64(int nBits)
{
    K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(long long) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(long long);
        return 0;
    }

#if KOTOR1  // K1's 64-bit ints are long
    int64_t nValue = *reinterpret_cast<int64_t*>(&m_pReadMessage[m_nReadMessagePos]);
#else
    long long nValue = *reinterpret_cast<long long*>(&m_pReadMessage[m_nReadMessagePos]);
#endif
    m_nReadMessagePos += sizeof(long long);
    ByteSwap(&nValue);
    return nValue;
}

Vector CSWMessage::ReadVector()
{
    Vector v;

    v.x = ReadFLOAT(0.0f, 32);
    v.y = ReadFLOAT(0.0f, 32);
    v.z = ReadFLOAT(0.0f, 32);

    return v;
}

float CSWMessage::ReadFLOAT(float scale, int nBits)
{
    K2_UNUSED(scale); K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(float) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(float);
        return 0.0f;
    }

    float fValue = *reinterpret_cast<float*>(&m_pReadMessage[m_nReadMessagePos]);
    m_nReadMessagePos += sizeof(float);
    ByteSwap(&fValue);
    return fValue;
}

float CSWMessage::ReadFLOAT(float lo, float hi, int nBits)
{
    K2_UNUSED(lo); K2_UNUSED(hi); K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(float) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(float);
        return 0.0f;
    }

    float fValue = *reinterpret_cast<float*>(&m_pReadMessage[m_nReadMessagePos]);
    m_nReadMessagePos += sizeof(float);
    ByteSwap(&fValue);
    return fValue;
}

double CSWMessage::ReadDOUBLE(double scale, int nBits)
{
    K2_UNUSED(scale); K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(double) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(double);
        return 0.0;
    }

    double fValue = *reinterpret_cast<double*>(&m_pReadMessage[m_nReadMessagePos]);
    m_nReadMessagePos += sizeof(double);
    ByteSwap(&fValue);
    return fValue;
}

double CSWMessage::ReadDOUBLE(double lo, double hi, int nBits)
{
    K2_UNUSED(lo); K2_UNUSED(hi); K2_UNUSED(nBits);
    if (m_nReadMessagePos + sizeof(double) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(double);
        return 0.0;
    }

    double fValue = *reinterpret_cast<double*>(&m_pReadMessage[m_nReadMessagePos]);
    m_nReadMessagePos += sizeof(double);
    ByteSwap(&fValue);
    return fValue;
}

CResRef CSWMessage::ReadCResRef(int nSize)
{
    if (nSize >= 16)
        nSize = 16;

    if (m_nReadMessagePos + nSize > m_nReadMessageSize) {
        m_nReadMessagePos += nSize;
        return CResRef("");
    }

    unsigned char sResRef[16];
    __memcpy_chk(sResRef, &m_pReadMessage[m_nReadMessagePos], nSize, sizeof(sResRef));
    m_nReadMessagePos += nSize;
    return CResRef(sResRef);
}

CExoString CSWMessage::ReadCExoString(int nBits)
{
    int nLength = ReadINT(nBits);

    if (m_nReadMessagePos + nLength > m_nReadMessageSize) {
        m_nReadMessagePos += nLength;
        return CExoString("");
    }

    CExoString sValue(reinterpret_cast<const char*>(&m_pReadMessage[m_nReadMessagePos]), nLength);
    m_nReadMessagePos += nLength;
    return sValue;
}

void* CSWMessage::ReadVOIDPtr(int nSize)
{
    void* pValue;

    if (m_nReadMessagePos + nSize <= m_nReadMessageSize)
        pValue = m_pReadMessage + m_nReadMessagePos;
    else
        pValue = 0;

    m_nReadMessagePos += nSize;
    return pValue;
}

int CSWMessage::MessageReadOverflow(int a)
{
    K2_UNUSED(a);
    return m_nReadMessagePos > m_nReadMessageSize;
}

int CSWMessage::MessageReadUnderflow(int a)
{
    K2_UNUSED(a);
    return m_nReadMessagePos < m_nReadMessageSize;
}

int CSWMessage::MessageMoreDataToRead()
{
    return m_nReadMessagePos < m_nReadMessageSize;
}

void CSWMessage::CreateWriteMessage(unsigned int nSize, unsigned int a2, int a3)
{
    m_bWriteMessageOpen = 1;
    m_nPlayerId = a2;
    m_bGuaranteed = a3;
    m_nWriteMessageSize = 3;

    if (m_nWriteMessageSize + nSize > m_nWriteMessageBufferSize) {
        int nNewSize = m_nWriteMessageSize + nSize + DEFAULT_WRITE_BUFFER_SIZE;
        unsigned char* pNew = new unsigned char[nNewSize];

        unsigned char* pOld = m_pWriteMessage;
        K2_ASSUME(static_cast<unsigned int>(nNewSize) >= m_nWriteMessageSize);   // nSize is a message length, never near 2^31
        memcpy(pNew, pOld, m_nWriteMessageSize);  // idiom-exempt: raw byte-count copy of a network buffer, there is no element type to take sizeof of
        operator delete[](pOld);  // idiom-exempt: an explicit deallocation call, a `delete[]` expression would emit an extra null check

        m_pWriteMessage = pNew;
        m_nWriteMessageBufferSize = nNewSize;
    }
}

void CSWMessage::WriteBOOL(int nValue)
{
    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

void CSWMessage::WriteCHAR(char nValue, int nBits)
{
    K2_UNUSED(nBits);
    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

void CSWMessage::WriteSHORT(short nValue, int nBits)
{
    K2_UNUSED(nBits);
    ByteSwap(&nValue);

    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

void CSWMessage::WriteINT(int nValue, int nBits)
{
    K2_UNUSED(nBits);
    ByteSwap(&nValue);

    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

#if KOTOR1
void CSWMessage::WriteINT64(int64_t nValue, int nBits)
#else
void CSWMessage::WriteINT64(long long nValue, int nBits)
#endif
{
    K2_UNUSED(nBits);
    ByteSwap(&nValue);

    if (m_nWriteMessageSize + sizeof(nValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(nValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &nValue, sizeof(nValue));
    m_nWriteMessageSize += sizeof(nValue);
    m_nBytesWritten += sizeof(nValue);
}

void CSWMessage::WriteVector(const Vector& v)
{
    WriteFLOAT(v.x, 0.0f, 32);
    WriteFLOAT(v.y, 0.0f, 32);
    WriteFLOAT(v.z, 0.0f, 32);
}

__attribute__((noinline))
void CSWMessage::WriteFLOAT(float fValue, float scale, int nBits)
{
    K2_UNUSED(scale); K2_UNUSED(nBits);
    ByteSwap(&fValue);

    if (m_nWriteMessageSize + sizeof(fValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(fValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &fValue, sizeof(fValue));
    m_nWriteMessageSize += sizeof(fValue);
    m_nBytesWritten += sizeof(fValue);
}

void CSWMessage::WriteFLOAT(float fValue, float lo, float hi, int nBits)
{
    K2_UNUSED(lo); K2_UNUSED(hi); K2_UNUSED(nBits);
    ByteSwap(&fValue);

    if (m_nWriteMessageSize + sizeof(fValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(fValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &fValue, sizeof(fValue));
    m_nWriteMessageSize += sizeof(fValue);
    m_nBytesWritten += sizeof(fValue);
}

void CSWMessage::WriteDOUBLE(double fValue, double scale, int nBits)
{
    K2_UNUSED(scale); K2_UNUSED(nBits);
    ByteSwap(&fValue);

    if (m_nWriteMessageSize + sizeof(fValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(fValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &fValue, sizeof(fValue));
    m_nWriteMessageSize += sizeof(fValue);
    m_nBytesWritten += sizeof(fValue);
}

void CSWMessage::WriteDOUBLE(double fValue, double lo, double hi, int nBits)
{
    K2_UNUSED(lo); K2_UNUSED(hi); K2_UNUSED(nBits);
    ByteSwap(&fValue);

    if (m_nWriteMessageSize + sizeof(fValue) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(fValue));

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &fValue, sizeof(fValue));
    m_nWriteMessageSize += sizeof(fValue);
    m_nBytesWritten += sizeof(fValue);
}

void CSWMessage::WriteCResRef(CResRef resRef, int nSize)
{
    if (m_nWriteMessageSize + nSize > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(nSize);

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], resRef.GetResRef(), nSize);  // idiom-exempt: raw byte-count copy of a network buffer, there is no element type to take sizeof of
    m_nWriteMessageSize += nSize;
    m_nBytesWritten += nSize;
}

void CSWMessage::WriteCExoString(const CExoString& sValue, int nBits)
{
    int nLength = sValue.GetLength();
    WriteINT(nLength, nBits);

    if (m_nWriteMessageSize + nLength > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(nLength);

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], sValue.CStr(), nLength);  // idiom-exempt: raw byte-count copy of a network buffer, there is no element type to take sizeof of
    m_nWriteMessageSize += nLength;
    m_nBytesWritten += nLength;
}

void CSWMessage::WriteVOIDPtr(void* pData, int nSize)
{
    if (m_nWriteMessageSize + nSize > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(nSize);

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], pData, nSize);  // idiom-exempt: raw byte-count copy of a network buffer, there is no element type to take sizeof of
    m_nWriteMessageSize += nSize;
    m_nBytesWritten += nSize;
}

unsigned int CSWMessage::PeekAtWriteMessageSize()
{
    return m_nWriteMessageSize;
}

int CSWMessage::GetWriteMessage(unsigned char** ppBuffer, unsigned int* pnSize)
{
    m_bWriteMessageOpen = 0;
    *pnSize = m_nWriteMessageSize;

    int bResult = (m_nWriteMessageSize != 0);
    if (bResult)
        *ppBuffer = m_pWriteMessage;
    else
        *ppBuffer = 0;

    return bResult;
}

CSWCCMessageData::CSWCCMessageData() : m_nType(0)
{
    __builtin_memset(static_cast<void*>(&m_integers), 0,
                     sizeof(m_integers) + sizeof(m_floats) + sizeof(m_strings) + sizeof(m_objectIds));
}

int CSWCCMessageData::GetInteger(int index)
{
    if (m_integers.m_count > index)
        return m_integers.m_pArray[index];
    return 0;
}

float CSWCCMessageData::GetFloat(int index)
{
    if (m_floats.m_count > index)
        return m_floats.m_pArray[index];
    return 0.0f;
}

unsigned long long CSWCCMessageData::GetObjectID(int index)
{
    if (m_objectIds.m_count > index)
        return m_objectIds.m_pArray[index];
    return OBJECT_INVALID;
}

CExoString CSWCCMessageData::GetString(int index)
{
    if (m_strings.m_count > index)
        return m_strings.m_pArray[index];
    return "";
}

void CSWCCMessageData::SetInteger(int index, int value)
{
    if (m_integers.m_count <= index)
        m_integers.SetSize(index + 1);
    m_integers.m_pArray[index] = value;
}

void CSWCCMessageData::SetFloat(int index, float value)
{
    if (m_floats.m_count <= index)
        m_floats.SetSize(index + 1);
    m_floats.m_pArray[index] = value;
}

void CSWCCMessageData::SetObjectID(int index, unsigned long long value)
{
    if (m_objectIds.m_count <= index)
        m_objectIds.SetSize(index + 1);
    m_objectIds.m_pArray[index] = value;
}

void CSWCCMessageData::SetString(int index, const CExoString& value)
{
    if (m_strings.m_count <= index)
        m_strings.SetSize(index + 1);
    m_strings.m_pArray[index] = value;
}

// The struct id SaveData stamps on every list element.
enum { kListElementStructId = 0xbaad };

int CSWCCMessageData::SaveData(CResGFF* pRes, CResStruct* pStruct)
{
    pRes->WriteFieldBYTE(pStruct, m_nType, const_cast<char*>("Type"));

    CResList list;
    pRes->AddList(&list, pStruct, const_cast<char*>("IntList"));
    for (int64_t i = 0; i < m_integers.m_count; i++)
    {
        CResStruct elem;
        pRes->AddListElement(&elem, &list, kListElementStructId);
        pRes->WriteFieldINT(&elem, m_integers.m_pArray[i], const_cast<char*>("IntegerValue"));
    }

    pRes->AddList(&list, pStruct, const_cast<char*>("FloatList"));
    for (int64_t i = 0; i < m_floats.m_count; i++)
    {
        CResStruct elem;
        pRes->AddListElement(&elem, &list, kListElementStructId);
        pRes->WriteFieldFLOAT(&elem, m_floats.m_pArray[i], const_cast<char*>("FloatValue"));
    }

    pRes->AddList(&list, pStruct, const_cast<char*>("ObjectIDList"));
    for (int64_t i = 0; i < m_objectIds.m_count; i++)
    {
        CResStruct elem;
        pRes->AddListElement(&elem, &list, kListElementStructId);
        pRes->WriteFieldDWORD(&elem, static_cast<unsigned int>(m_objectIds.m_pArray[i]), const_cast<char*>("ObjectValue"));
    }

    pRes->AddList(&list, pStruct, const_cast<char*>("StringList"));
    for (int64_t i = 0; i < m_strings.m_count; i++)
    {
        CResStruct elem;
        pRes->AddListElement(&elem, &list, kListElementStructId);
        pRes->WriteFieldCExoString(&elem, m_strings.m_pArray[i], const_cast<char*>("StringValue"));
    }

    return 1;
}

void CSWCCMessageData::ClearData()
{
    m_integers.m_count = 0;
    m_floats.m_count = 0;
    m_objectIds.m_count = 0;
    m_strings.m_count = 0;
    m_nType = 0;
}

int CSWCCMessageData::LoadData(CResGFF* pRes, CResStruct* pStruct)
{
    CResList list;
    CResStruct elem;
    int bSuccess;

    m_nType = pRes->ReadFieldBYTE(pStruct, const_cast<char*>("Type"), bSuccess, 0);

    if (pRes->GetList(&list, pStruct, const_cast<char*>("IntList")) == 1)
    {
        for (unsigned int i = 0; i < pRes->GetListCount(&list); i++)
        {
            pRes->GetListElement(&elem, &list, i);
            SetInteger(i, pRes->ReadFieldINT(&elem, const_cast<char*>("IntegerValue"), bSuccess, 0));
        }
    }

    if (pRes->GetList(&list, pStruct, const_cast<char*>("FloatList")) == 1)
    {
        for (unsigned int i = 0; i < pRes->GetListCount(&list); i++)
        {
            pRes->GetListElement(&elem, &list, i);
            SetFloat(i, pRes->ReadFieldFLOAT(&elem, const_cast<char*>("FloatValue"), bSuccess, 0.0f));
        }
    }

    if (pRes->GetList(&list, pStruct, const_cast<char*>("ObjectIDList")) == 1)
    {
        for (unsigned int i = 0; i < pRes->GetListCount(&list); i++)
        {
            pRes->GetListElement(&elem, &list, i);
            SetObjectID(i, pRes->ReadFieldDWORD(&elem, const_cast<char*>("ObjectValue"), bSuccess, 0));
        }
    }

    if (pRes->GetList(&list, pStruct, const_cast<char*>("StringList")) == 1)
    {
        for (unsigned int i = 0; i < pRes->GetListCount(&list); i++)
        {
            pRes->GetListElement(&elem, &list, i);
            SetString(i, pRes->ReadFieldCExoString(&elem, const_cast<char*>("StringValue"), bSuccess,
                                                   CExoString("")));
        }
    }

    return 1;
}

void CSWCCMessageData::CopyTo(CSWCCMessageData* pDest)
{
    if (!pDest)
        return;

    pDest->ClearData();

    for (int i = 0; i < m_integers.m_count; i++)
        pDest->SetInteger(i, m_integers.m_pArray[i]);

    for (int i = 0; i < m_floats.m_count; i++)
        pDest->SetFloat(i, m_floats.m_pArray[i]);

    for (int i = 0; i < m_objectIds.m_count; i++)
        pDest->SetObjectID(i, m_objectIds.m_pArray[i]);

    for (int i = 0; i < m_strings.m_count; i++)
        pDest->SetString(i, m_strings.m_pArray[i]);

    pDest->m_nType = m_nType;
}

CSWCCMessageData::~CSWCCMessageData() {}
