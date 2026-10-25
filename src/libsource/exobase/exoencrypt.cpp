// MD5 (RFC 1321) behind a class wrapper.

#include "k2stdint.h"
#include "exobase.h"
#include "exostring.h"
#include "exoencrypt.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "exostringinline.h"

// RFC 1321's padding block, non-const on purpose.
static unsigned char s_pPadding[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// File-local hex formatter.
static void Hexify(char* const __attribute__((pass_object_size(0))) pDest, const char* pFmt,
                   ...);

CExoString CExoEncrypt::EncryptString(const CExoString& sInput)
{
    char szHex[40];

    MD5Init();
    MD5Update(reinterpret_cast<unsigned char*>(sInput.CStr()),
              sInput.GetLength());
    MD5Final();

    for (int i = 0; i < 16; i++)
        Hexify(&szHex[i * 2], "%02x", m_pDigest[i]);
    szHex[32] = 0;

    return CExoString(szHex);
}

// RFC 1321's initial digest state.
enum : unsigned int {
    MD5_INIT_A = 0x67452301,
    MD5_INIT_B = 0xefcdab89,
    MD5_INIT_C = 0x98badcfe,
    MD5_INIT_D = 0x10325476,
};

void CExoEncrypt::MD5Init()
{
    m_nCount[0] = 0;
    m_nCount[1] = 0;
    m_nState[0] = MD5_INIT_A;
    m_nState[1] = MD5_INIT_B;
    m_nState[2] = MD5_INIT_C;
    m_nState[3] = MD5_INIT_D;
    __builtin_memset(m_pBuffer, 0, sizeof(m_pBuffer));
    __builtin_memset(m_pDigest, 0, sizeof(m_pDigest));
}

void CExoEncrypt::MD5Update(unsigned char* pInput, unsigned int nInputLen)
{
    unsigned int i;
    int index;

    /* Compute number of bytes mod 64 */
    index = static_cast<int>((m_nCount[0] >> 3) & 63);

    /* Update number of bits */
    unsigned int nBits = static_cast<unsigned int>(nInputLen) << 3;
    if (m_nCount[0] + nBits < m_nCount[0])
        m_nCount[1]++;
    m_nCount[0] += nBits;
    m_nCount[1] += (static_cast<unsigned int>(nInputLen) >> 29);

    while (nInputLen) {
        m_pBuffer[index++] = *pInput++;
        nInputLen--;
        if (index == 64) {
            unsigned int x[16];

            for (i = 0; i < 16; i++)
                x[i] = (static_cast<unsigned int>(m_pBuffer[i * 4])) |
                       ((static_cast<unsigned int>(m_pBuffer[i * 4 + 1])) << 8) |
                       ((static_cast<unsigned int>(m_pBuffer[i * 4 + 2])) << 16) |
                       ((static_cast<unsigned int>(m_pBuffer[i * 4 + 3])) << 24);
            Transform(m_nState, x);
            index = 0;
        }
    }
}

void CExoEncrypt::MD5Final()
{
    unsigned int i, index, padLen;
    unsigned int x[16];

    /* Append the length in bits to the last block. */
    x[14] = m_nCount[0];
    x[15] = m_nCount[1];

    /* Pad out to 56 mod 64. */
    index = static_cast<unsigned int>((m_nCount[0] >> 3) & 63);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    MD5Update(s_pPadding, padLen);

    /* Copy the (now 56-byte) buffer in front of the length. Kept rolled and scalar on purpose. */
#pragma clang loop unroll(disable)
#pragma clang loop vectorize(disable)
    for (i = 0; i < 14; i++)
        x[i] = reinterpret_cast<unsigned int*>(m_pBuffer)[i];  // raw-offset: MD5 byte buffers read as words
    Transform(m_nState, x);

    /* Store the state in the digest. */
    reinterpret_cast<unsigned int*>(m_pDigest)[0] = m_nState[0];  // raw-offset: MD5 byte buffers read as words
    reinterpret_cast<unsigned int*>(m_pDigest)[1] = m_nState[1];  // idiom-exempt: the MD5 digest is a byte buffer  // raw-offset: MD5 byte buffers read as words
    reinterpret_cast<unsigned int*>(m_pDigest)[2] = m_nState[2];  // idiom-exempt: the MD5 digest is a byte buffer  // raw-offset: MD5 byte buffers read as words
    reinterpret_cast<unsigned int*>(m_pDigest)[3] = m_nState[3];  // idiom-exempt: the MD5 digest is a byte buffer  // raw-offset: MD5 byte buffers read as words
}

static void Hexify(char* const __attribute__((pass_object_size(0))) pDest, const char* pFmt,
                   ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, pFmt);
    __vsprintf_chk(pDest, 0, __builtin_object_size(pDest, 0), pFmt, ap);
    __builtin_va_end(ap);
}

CExoString CExoEncrypt::GenerateChallenge(unsigned short nLength)
{
    return g_pExoBase->m_pRand->GetString(nLength);
}

/* F, G, H and I are the basic MD5 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* FF, GG, HH and II are the four round transformations. Rotation is separate from addition to
   prevent recomputation. */
#define FF(a, b, c, d, x, s, ac)                    \
    {                                               \
        (a) += F((b), (c), (d)) + (x) + static_cast<unsigned int>(ac); \
        (a) = ROTATE_LEFT((a), (s));                \
        (a) += (b);                                 \
    }
#define GG(a, b, c, d, x, s, ac)                    \
    {                                               \
        (a) += G((b), (c), (d)) + (x) + static_cast<unsigned int>(ac); \
        (a) = ROTATE_LEFT((a), (s));                \
        (a) += (b);                                 \
    }
#define HH(a, b, c, d, x, s, ac)                    \
    {                                               \
        (a) += H((b), (c), (d)) + (x) + static_cast<unsigned int>(ac); \
        (a) = ROTATE_LEFT((a), (s));                \
        (a) += (b);                                 \
    }
#define II(a, b, c, d, x, s, ac)                    \
    {                                               \
        (a) += I((b), (c), (d)) + (x) + static_cast<unsigned int>(ac); \
        (a) = ROTATE_LEFT((a), (s));                \
        (a) += (b);                                 \
    }

void CExoEncrypt::Transform(unsigned int* paState, unsigned int* paBlock)
{
    unsigned int a = paState[0], b = paState[1], c = paState[2], d = paState[3];
    unsigned int* x = paBlock;

    /* Round 1 */
    FF(a, b, c, d, x[0], 7, 0xd76aa478);  /* 1 */
    FF(d, a, b, c, x[1], 12, 0xe8c7b756); /* 2 */
    FF(c, d, a, b, x[2], 17, 0x242070db); /* 3 */
    FF(b, c, d, a, x[3], 22, 0xc1bdceee); /* 4 */
    FF(a, b, c, d, x[4], 7, 0xf57c0faf);  /* 5 */
    FF(d, a, b, c, x[5], 12, 0x4787c62a); /* 6 */
    FF(c, d, a, b, x[6], 17, 0xa8304613); /* 7 */
    FF(b, c, d, a, x[7], 22, 0xfd469501); /* 8 */
    FF(a, b, c, d, x[8], 7, 0x698098d8);  /* 9 */
    FF(d, a, b, c, x[9], 12, 0x8b44f7af); /* 10 */
    FF(c, d, a, b, x[10], 17, 0xffff5bb1);/* 11 */
    FF(b, c, d, a, x[11], 22, 0x895cd7be);/* 12 */
    FF(a, b, c, d, x[12], 7, 0x6b901122); /* 13 */
    FF(d, a, b, c, x[13], 12, 0xfd987193);/* 14 */
    FF(c, d, a, b, x[14], 17, 0xa679438e);/* 15 */
    FF(b, c, d, a, x[15], 22, 0x49b40821);/* 16 */

    /* Round 2 */
    GG(a, b, c, d, x[1], 5, 0xf61e2562);  /* 17 */
    GG(d, a, b, c, x[6], 9, 0xc040b340);  /* 18 */
    GG(c, d, a, b, x[11], 14, 0x265e5a51);/* 19 */
    GG(b, c, d, a, x[0], 20, 0xe9b6c7aa); /* 20 */
    GG(a, b, c, d, x[5], 5, 0xd62f105d);  /* 21 */
    GG(d, a, b, c, x[10], 9, 0x02441453); /* 22 */
    GG(c, d, a, b, x[15], 14, 0xd8a1e681);/* 23 */
    GG(b, c, d, a, x[4], 20, 0xe7d3fbc8); /* 24 */
    GG(a, b, c, d, x[9], 5, 0x21e1cde6);  /* 25 */
    GG(d, a, b, c, x[14], 9, 0xc33707d6); /* 26 */
    GG(c, d, a, b, x[3], 14, 0xf4d50d87); /* 27 */
    GG(b, c, d, a, x[8], 20, 0x455a14ed); /* 28 */
    GG(a, b, c, d, x[13], 5, 0xa9e3e905); /* 29 */
    GG(d, a, b, c, x[2], 9, 0xfcefa3f8);  /* 30 */
    GG(c, d, a, b, x[7], 14, 0x676f02d9); /* 31 */
    GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);/* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[5], 4, 0xfffa3942);  /* 33 */
    HH(d, a, b, c, x[8], 11, 0x8771f681); /* 34 */
    HH(c, d, a, b, x[11], 16, 0x6d9d6122);/* 35 */
    HH(b, c, d, a, x[14], 23, 0xfde5380c);/* 36 */
    HH(a, b, c, d, x[1], 4, 0xa4beea44);  /* 37 */
    HH(d, a, b, c, x[4], 11, 0x4bdecfa9); /* 38 */
    HH(c, d, a, b, x[7], 16, 0xf6bb4b60); /* 39 */
    HH(b, c, d, a, x[10], 23, 0xbebfbc70);/* 40 */
    HH(a, b, c, d, x[13], 4, 0x289b7ec6); /* 41 */
    HH(d, a, b, c, x[0], 11, 0xeaa127fa); /* 42 */
    HH(c, d, a, b, x[3], 16, 0xd4ef3085); /* 43 */
    HH(b, c, d, a, x[6], 23, 0x04881d05); /* 44 */
    HH(a, b, c, d, x[9], 4, 0xd9d4d039);  /* 45 */
    HH(d, a, b, c, x[12], 11, 0xe6db99e5);/* 46 */
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8);/* 47 */
    HH(b, c, d, a, x[2], 23, 0xc4ac5665); /* 48 */

    /* Round 4 */
    II(a, b, c, d, x[0], 6, 0xf4292244);  /* 49 */
    II(d, a, b, c, x[7], 10, 0x432aff97); /* 50 */
    II(c, d, a, b, x[14], 15, 0xab9423a7);/* 51 */
    II(b, c, d, a, x[5], 21, 0xfc93a039); /* 52 */
    II(a, b, c, d, x[12], 6, 0x655b59c3); /* 53 */
    II(d, a, b, c, x[3], 10, 0x8f0ccc92); /* 54 */
    II(c, d, a, b, x[10], 15, 0xffeff47d);/* 55 */
    II(b, c, d, a, x[1], 21, 0x85845dd1); /* 56 */
    II(a, b, c, d, x[8], 6, 0x6fa87e4f);  /* 57 */
    II(d, a, b, c, x[15], 10, 0xfe2ce6e0);/* 58 */
    II(c, d, a, b, x[6], 15, 0xa3014314); /* 59 */
    II(b, c, d, a, x[13], 21, 0x4e0811a1);/* 60 */
    II(a, b, c, d, x[4], 6, 0xf7537e82);  /* 61 */
    II(d, a, b, c, x[11], 10, 0xbd3af235);/* 62 */
    II(c, d, a, b, x[2], 15, 0x2ad7d2bb); /* 63 */
    II(b, c, d, a, x[9], 21, 0xeb86d391); /* 64 */

    paState[0] += a;
    paState[1] += b;
    paState[2] += c;
    paState[3] += d;
}
