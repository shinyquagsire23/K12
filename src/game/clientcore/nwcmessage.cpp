// The message layer's primitive reader and writer for a single object id or location string. The
// rest of the CSWCMessage surface lives with the outbound send family, the inbound handle family and
// the message base.

#include "k2stdint.h"
#include "nwchandlemessage.h"

#include "exoresman.h"
#include "exostring.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"

// The byte-order helper, declared here rather than pulling the whole GFF format header into the
// network layer.
void ByteSwap(unsigned long long* p);
#include "port/libcstring.h"

// Reads what WriteOBJECTIDClient writes. It bounds-tests the cursor, loads the value, byte-swaps it
// and bumps the cursor on both paths.
unsigned long long CSWCMessage::ReadOBJECTIDClient()
{
    if (m_nReadMessagePos + sizeof(unsigned long long) > m_nReadMessageSize) {
        m_nReadMessagePos += sizeof(unsigned long long);
        return 0;
    }

    unsigned long long objId = *reinterpret_cast<unsigned long long*>(&m_pReadMessage[m_nReadMessagePos]);
    ByteSwap(&objId);
    m_nReadMessagePos += sizeof(objId);
    return objId;
}

// Reads a location string, which is either a plain string or a strref the talk table resolves into
// text, a sound resref and a sound length. Both out-parameters are set on entry, so a failed lookup
// still leaves them defined.
CExoString CSWCMessage::ReadCExoLocStringClient(CResRef& rSound, float* pfDuration)
{
    CExoString sText;
    unsigned int nStrRef;
    rSound = const_cast<char*>("");
    if (pfDuration != 0)
        *pfDuration = 0.0f;

    if (ReadBOOL() == 0)
        return ReadCExoString(32);

    STR_RES res;
    unsigned char nStrRefType = static_cast<unsigned char>(ReadBYTE(8));
    nStrRef = ReadDWORD(32);
    if (g_pTlkTable->Fetch(nStrRef, res, nStrRefType) == 0)
        return CExoString(const_cast<char*>(""));

    rSound = res.soundResRef;
    if (pfDuration != 0)
        *pfDuration = res.fSoundLength;
    return res.text;
}

// Reads a location string whose sound details the caller does not want.
CExoString CSWCMessage::ReadCExoLocStringClient()
{
    CResRef ref;
    return ReadCExoLocStringClient(ref, 0);
}

// Writes an object id, extending the buffer when it is full.
void CSWCMessage::WriteOBJECTIDClient(unsigned long long objId)
{
    if (m_nWriteMessageSize + sizeof(objId) > m_nWriteMessageBufferSize)
        ExtendWriteBuffer(sizeof(objId));

    ByteSwap(&objId);

    memcpy(&m_pWriteMessage[m_nWriteMessageSize], &objId, sizeof(objId));
    m_nWriteMessageSize += sizeof(objId);
    m_nBytesWritten += sizeof(objId);
}
