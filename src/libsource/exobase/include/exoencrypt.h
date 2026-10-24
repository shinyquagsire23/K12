#pragma once
#include "exostring.h"

// RFC-1321 MD5 (the RSA reference implementation). EncryptString returns the 16-byte digest as 32
// lowercase hex characters.

class CExoEncrypt {
public:
    void MD5Init();
    void MD5Update(unsigned char* pInput, unsigned int nInputLen);
    void MD5Final();
    void Transform(unsigned int* paState, unsigned int* paBlock);  // unused

    CExoString EncryptString(const CExoString& sInput);
    CExoString GenerateChallenge(unsigned short nLength);

private:
    unsigned int  m_nCount[2];
    unsigned int  m_nState[4];
    unsigned char m_pBuffer[128];
    unsigned char m_pDigest[16];
};
