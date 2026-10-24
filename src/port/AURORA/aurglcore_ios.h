#pragma once
#include "k2stdint.h"

#include "aurglcore.h"
#include "List.h"

namespace {
class RequestPoolManager {
public:
    virtual ~RequestPoolManager();
    virtual unsigned int RequestPoolGeneric(Aurora::AuroraBufferRequestType eType, unsigned int a,
                                            unsigned int b) = 0;
    virtual void* LockPool(unsigned int nPool, unsigned int a, unsigned int b, unsigned int c) = 0;
    virtual void UnlockPool(unsigned int nPool) = 0;
    virtual void* GetPool(unsigned int nPool) = 0;
    virtual void* GetIndexPool(unsigned int nPool) = 0;
    virtual void DeletePool(unsigned int nPool) = 0;
    virtual void ResizePool(unsigned int nPool, unsigned int nSize) = 0;
    virtual unsigned int GetPoolSize(unsigned int nPool) = 0;
    virtual void RefreshPool(unsigned int a, unsigned int b) = 0;
    virtual void BackUpPoolManager() = 0;
    virtual void RestorePoolManager() = 0;

    List<uint64_t> m_aPoolIndex;
    List<int>           m_anPoolInUse;
    unsigned int        m_nNumPools;
};

class RequestPoolManagerVBO : public RequestPoolManager {
public:
    RequestPoolManagerVBO();
    ~RequestPoolManagerVBO();
    virtual unsigned int RequestPoolGeneric(Aurora::AuroraBufferRequestType eType, unsigned int a,
                                            unsigned int b);
    virtual void* LockPool(unsigned int nPool, unsigned int a, unsigned int b, unsigned int c);
    virtual void UnlockPool(unsigned int nPool);
    virtual void* GetPool(unsigned int nPool);
    virtual void* GetIndexPool(unsigned int nPool);
    virtual void DeletePool(unsigned int nPool);
    virtual void ResizePool(unsigned int nPool, unsigned int nSize);
    virtual unsigned int GetPoolSize(unsigned int nPool);
    virtual void RefreshPool(unsigned int a, unsigned int b);
    virtual void BackUpPoolManager();
    virtual void RestorePoolManager();

    List<unsigned int> m_anLockFlags;
    List<unsigned int> m_anBufferID;
    List<char*>        m_apBackupData;
    List<unsigned int> m_anBackupSize;
    List<unsigned int> m_anBufferTarget;
    List<unsigned int> m_anBufferSize;
    List<void*>        m_apBufferData;
#if !KOTOR1  // K1 has no lock window and no dirty flag; UnlockPool uploads the whole buffer
    List<unsigned int> m_anPoolOffset;
    List<unsigned int> m_anPoolLength;
    List<bool>         m_abBufferDirty;
#endif
};
#if KOTOR1
// K1's backups keep heap copies of the mapped buffers; RestorePoolManager re-uploads and frees them.
K2_LAYOUT_ASSERT(sizeof(RequestPoolManagerVBO) == 0xa0, "K1 RequestPoolManagerVBO size");
#endif

class RequestPoolManagerSys : public RequestPoolManager {
public:
    RequestPoolManagerSys();
    ~RequestPoolManagerSys();
    virtual unsigned int RequestPoolGeneric(Aurora::AuroraBufferRequestType eType, unsigned int a,
                                            unsigned int b);
    virtual void* LockPool(unsigned int nPool, unsigned int a, unsigned int b, unsigned int c);
    virtual void UnlockPool(unsigned int nPool);
    virtual void* GetPool(unsigned int nPool);
    virtual void* GetIndexPool(unsigned int nPool);
    virtual void DeletePool(unsigned int nPool);
    virtual void ResizePool(unsigned int nPool, unsigned int nSize);
    virtual unsigned int GetPoolSize(unsigned int nPool);
    virtual void RefreshPool(unsigned int a, unsigned int b);
    virtual void BackUpPoolManager();
    virtual void RestorePoolManager();

    char**        m_apPoolData;
    unsigned int* m_anPoolSize;
};

}   // namespace

namespace {
class ImposterManager
{
public:
    // Static; the body indexes the shared table.
    static void AllocateImposter(unsigned int nSlot, int nCount, float* pOffsets);
};
}   // namespace

struct Imposter
{
    Imposter() : m_bUsed(0) {}

    unsigned char m_bUsed;
    int           m_nTexture;
    unsigned char _pad08[0x18];
    float*        m_pOffsets;
    int           m_nOffsetCount;
};

struct ImposterTable
{
    Imposter* m_pImposters;
    int       m_nCount;
};
__attribute__((visibility("hidden"))) extern ImposterTable manRefImposter;

void InitializeSystemPoolManager();
void InitializeVertexBufferObjectPoolManager();

void SpitLetters(char* pszText);
int ones32popcount(unsigned int nBits);
int log2integer(unsigned int nValue);
void FindSmallestPowerOf2WidthHeight(unsigned int nWidth, unsigned int nHeight,
                                     unsigned int& nPow2Width, unsigned int& nPow2Height);
int GetCompressedContactSheetOffset(int nWidth, int nHeight, int nFace);

#if KOTOR1  // K1's PBUFFERARB is smaller: one renderbuffer, no stencil handle
struct PBUFFERARB
{
    int m_nFramebuffer;
    int m_nDepthRenderbuffer;
    int m_nTexture;
    int m_nWidth;
    int m_nHeight;
};
typedef PBUFFERARB PBUFFERARB_IOS;
PBUFFERARB* CreatePBufferIos(int nWidth, int nHeight);
#else
struct PBUFFERARB_IOS
{
    int m_nFramebuffer;
    int m_nDepthRenderbuffer;
    int m_nStencilRenderbuffer;
    int m_nTexture;
    int m_nWidth;
    int m_nHeight;
};

PBUFFERARB_IOS* CreatePBufferIos(int nWidth, int nHeight, bool bNoDepth);
#endif
int  _aglRestoreContext();
int  _aglSetCurrentPBuffer(PBUFFERARB_IOS* pPBuffer);
int  _aglDestroyPBuffer(PBUFFERARB_IOS* pPBuffer);
int  GetPbufferWidth(PBUFFERARB_IOS* pPBuffer);
int  GetPbufferHeight(PBUFFERARB_IOS* pPBuffer);
int  GetPbufferTex(PBUFFERARB_IOS* pPBuffer);
void IosSetDistortBuffer(bool bEnable);
void IosAntiAliasScaleSize(int& nWidth, int& nHeight);
bool IosAntiAliasOn();
bool SaturationThisFrameIos();
bool IosIsRecentDeviceVersion();
bool IosBloomEnabled();
void FrameBufferModificationsStartIos(bool bEnable);
void RenderModificationQuad();

class RenderModificationQuadSetup
{
public:
    RenderModificationQuadSetup();
    ~RenderModificationQuadSetup();
};

void RenderDistortIos(CAurTexture* pSrc, CAurTexture* pDst);
void FrameBufferModificationsEndIos(bool bEnable);
