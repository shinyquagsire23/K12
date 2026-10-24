#pragma once
#include "k2stdint.h"

#include "Vector.h"
#include "exoresman.h"
#include "exostring.h"

// The shared root of the network-message hierarchy, a bit-and-byte packing serializer over two
// owned buffers, one for writing and one for reading.
class CSWMessage {
public:
    static const unsigned int DEFAULT_WRITE_BUFFER_SIZE = 128;

    CSWMessage();

    virtual ~CSWMessage();

    unsigned char* m_pWriteMessage;
    unsigned int   m_nWriteMessageBufferSize;
    unsigned int   m_nWriteMessageSize;
    unsigned char* m_pWriteFragments;
    unsigned int   m_nReserved1;
    unsigned int   m_nWriteFragmentsPos;
    unsigned int   m_nBytesWritten;
    unsigned char  m_nWriteBitIndex;
    unsigned char  _pad2d[0x30 - 0x2d];
    int            m_bWriteMessageOpen;
    char           _pad34[0x38 - 0x34];

    unsigned char* m_pReadMessage;
    unsigned int   m_nReadMessageSize;
    unsigned int   m_nReadMessagePos;
    unsigned char* m_pReadFragments;
    unsigned int   m_nReadFragmentsSize;
    unsigned int   m_nReadFragmentsPos;
    unsigned int   m_nReserved2;
    unsigned char  m_nReadBitIndex;
    unsigned char  m_nReadStartBitIndex;
    unsigned char  _pad5e[0x60 - 0x5e];

    int            m_bGuaranteed;
    unsigned int   m_nPlayerId;

    void SetReadMessage(unsigned char* pBuffer, unsigned int nSize,
                        unsigned int a3, int a4);
    void ClearReadMessage();
    unsigned char ReadBit(unsigned char nBit);
    unsigned long long ReadBits(int nBits);
    void WriteBit(unsigned char nValue);
    void WriteBits(unsigned long long nValue, int nBits);
    void WriteUnsigned(unsigned long long nValue, int nBits);
    // Empty in this class; a derived class must allocate the fragments buffer.
    void ExtendWriteFragmentsBuffer(unsigned int nExtra);
    void ExtendWriteBuffer(unsigned int nExtra);

    // Each fixed-width writer byte-swaps, grows the buffer when it needs to, stores the value and
    // advances both counters; each reader mirrors it, advancing its cursor on both arms.
    void  WriteDWORD64(unsigned long long nValue, int nBits);
    void  WriteDWORD(unsigned int nValue, int nBits);
    void  WriteWORD(unsigned short nValue, int nBits);
    void  WriteBYTE(unsigned char nValue, int nBits);
#if KOTOR1  // K1: `long` in place of `long long`
    void  WriteSigned(int64_t nValue, int nBits);
#else
    void  WriteSigned(long long nValue, int nBits);
#endif
    unsigned long long ReadUnsigned(int nBits);
    unsigned long long ReadDWORD64(int nBits);
    unsigned int   ReadDWORD(int nBits);
    unsigned short ReadWORD(int nBits);
    unsigned char  ReadBYTE(int a);
    long long      ReadSigned(int nBits);
    int            ReadBOOL();
    char           ReadCHAR(int a);
    short          ReadSHORT(int nBits);
    int            ReadINT(int nBits);
    long long      ReadINT64(int nBits);
    Vector         ReadVector();
    float          ReadFLOAT(float scale, int nBits);
    float          ReadFLOAT(float lo, float hi, int nBits);
    double         ReadDOUBLE(double scale, int nBits);
    double         ReadDOUBLE(double lo, double hi, int nBits);
    CResRef        ReadCResRef(int nSize);
    CExoString     ReadCExoString(int nBits);
    void* ReadVOIDPtr(int nSize);
    int   MessageReadOverflow(int a);
    int   MessageReadUnderflow(int a);
    int   MessageMoreDataToRead();
    void  CreateWriteMessage(unsigned int nSize, unsigned int a2, int a3);
    void  WriteBOOL(int nValue);
    void  WriteCHAR(char nValue, int nBits);
    void  WriteSHORT(short nValue, int nBits);
    void  WriteINT(int nValue, int nBits);
#if KOTOR1  // K1: `long` in place of `long long`
    void  WriteINT64(int64_t nValue, int nBits);
#else
    void  WriteINT64(long long nValue, int nBits);
#endif
    void  WriteVector(const Vector& v);
    void  WriteFLOAT(float fValue, float scale, int nBits);
    void  WriteFLOAT(float fValue, float lo, float hi, int nBits);
    void  WriteDOUBLE(double fValue, double scale, int nBits);
    void  WriteDOUBLE(double fValue, double lo, double hi, int nBits);
    void  WriteCResRef(CResRef resRef, int nSize);
    void  WriteCExoString(const CExoString& s, int nBits);
    void  WriteVOIDPtr(void* pData, int nSize);
    unsigned int PeekAtWriteMessageSize();
    int   GetWriteMessage(unsigned char** ppBuffer, unsigned int* pnSize);
};

#include "CExoArrayListDtors.h"

class CResGFF;
struct CResStruct;

// Client-side combat/feedback message payload, a tagged bag of four parallel value lists (ints,
// floats, object ids, strings) plus a one-byte "Type" discriminator.
class CSWCCMessageData {  // idiom-exempt: home TU (src/CSWCCMessageData)
public:
    // m_nType, as the EVENT_FEEDBACK_MESSAGE handlers route the payload: saving-throw data for
    // BroadcastSavingThrowData, a feedback message for the event's caller only, or for its
    // receiver only. Any other value sends the feedback to both.
    enum {
        MESSAGE_TYPE_SAVING_THROW       = 1,
        MESSAGE_TYPE_FEEDBACK_TO_CALLER = 3,
        MESSAGE_TYPE_FEEDBACK_TO_SELF   = 4,
    };

    unsigned char                     m_nType;
    CExoArrayList<int>                m_integers;
    CExoArrayList<float>              m_floats;
    CExoArrayList<CExoString>         m_strings;
    CExoArrayList<unsigned long long> m_objectIds;

    CSWCCMessageData();
    ~CSWCCMessageData();

    int                GetInteger(int index);
    void               SetInteger(int index, int value);
    float              GetFloat(int index);
    void               SetFloat(int index, float value);
    unsigned long long GetObjectID(int index);
    void               SetObjectID(int index, unsigned long long value);
    CExoString         GetString(int index);
    void               SetString(int index, const CExoString& value);

    int  SaveData(CResGFF* pRes, CResStruct* pStruct);
    int  LoadData(CResGFF* pRes, CResStruct* pStruct);
    void ClearData();
    void CopyTo(CSWCCMessageData* pDest);
};
