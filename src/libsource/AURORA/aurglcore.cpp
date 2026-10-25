// OpenGL renderer core.

#include "k2stdint.h"
#include "port/aslglapi.h"
#include "port/sdlapi.h"
#include "port/win32api.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurglcore_ios.h"
#include "List.h"
#include "Vector.h"
#include "aurtexman.h"
#include "AURORA/aurmodel.h"
#include "math3d.h"

// The vertex pools grow in whole 4 KB pages: a size rounds up by kPoolPageSlack, then kPoolPageMask.
enum : unsigned int {
    kPoolPageSlack = 0xFFF,
    kPoolPageMask = 0xFFFFF000,
};

// The WGL attribute names and values the pbuffer setup passes, and the PVRTC formats K1 uploads,
// as wglext.h and the GLES IMG extension spell them.
#define WGL_SUPPORT_OPENGL_ARB                   0x2010
#define WGL_DOUBLE_BUFFER_ARB                    0x2011
#define WGL_RED_BITS_ARB                         0x2015
#define WGL_GREEN_BITS_ARB                       0x2017
#define WGL_BLUE_BITS_ARB                        0x2019
#define WGL_ALPHA_BITS_ARB                       0x201B
#define WGL_DRAW_TO_PBUFFER_ARB                  0x202D
#define WGL_BIND_TO_TEXTURE_RGBA_ARB             0x2071
#define WGL_TEXTURE_FORMAT_ARB                   0x2072
#define WGL_TEXTURE_TARGET_ARB                   0x2073
#define WGL_TEXTURE_RGBA_ARB                     0x2076
#define WGL_TEXTURE_2D_ARB                       0x207A
#define WGL_FRONT_LEFT_ARB                       0x2083
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV    0x20A1
#define WGL_TEXTURE_RECTANGLE_NV                 0x20A2
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG       0x8C00
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG      0x8C02

// The fragment programs the port supplies past the shipped ones (platforms/k1/k1kotorgl.cpp).
enum {
    kPortProgramDistortion = 0x100,
    kPortProgramSpeedBlur = 0x101,
    kPortProgramCopy = 0x102,
    kPortProgramSoftShadowBlur = 0x103,
};


extern const unsigned int fogModeTranslationTable[] = { 9729, 2048, 2049 };
int queryTranslationTable[2] = { 2912 };
unsigned int blendTranslationTable[] = { 770, 771, 1, 0, 768, 774, 773, 772 };
extern const int transformTranslationTable[4] = { 5889, 5888, 5888, 5890 };
extern const unsigned int cmpTranslationTable[] = { 512, 513, 514, 515, 516, 517, 518, 519 };
extern const unsigned int cullTranslationTable[] = { 2304, 2305 };
extern const unsigned int stencilOpTranslationTable[] = { 7680, 0, 7681, 7682, 7683, 5386 };
extern const int fillTranslationTable[3] = { 6912, 6913, 6914 };
extern const int facingTranslationTable[3] = { 1028, 1029, 1032 };
extern const unsigned int materialTranslationTable[] = { 5633, 4608, 4609, 5632, 4610 };
extern const unsigned int colorTrackingTranslationTable[] = { 5632, 4608, 4609, 4610, 5634 };
extern const unsigned int lightTranslationTable[] = { 4608, 4609, 4610, 4611, 4617 };
extern const unsigned int texParamTranslationTable[] = { 10242, 10243, 10240, 10241 };
#if KOTOR1
extern const unsigned int clientStateTranslationTable[] = {
    32888, 32885, 32886, 32884, 34385, 34387, 34388, 34389, 34396, 34397, 34398, 34399, 34099
};
#else
extern const unsigned int clientStateTranslationTable[] = {
    32888, 32885, 32886, 32884, 34385, 34387, 34388, 34389, 34396, 34397, 34398, 34399, 34077
};
#endif
bool gHasRevComps_Mac;

int g_nOldAliasingMode = -1;
bool GLRender::m_bSaturationEnabled;

#if KOTOR1
int g_IosAntiAlias;
#endif
#if KOTOR1
float largest_supported_anisotropy;
#else
float largest_supported_anisotropy = 16.0f;
#endif
#if KOTOR1
int visualizepass2;
#else
int visualizepass2 = 1;
#endif
#if KOTOR1
int visualizepass3;
#else
int visualizepass3 = 1;
#endif
#if KOTOR1
int visualizepass4;
#else
int visualizepass4 = 1;
#endif
#if KOTOR1
int visualizepass5;
#else
int visualizepass5 = 1;
#endif
#if KOTOR1
int visualizepass6;
#else
int visualizepass6 = 1;
#endif
#if KOTOR1
int visualizepass7;
#else
int visualizepass7 = 1;
#endif
void* GLRender::cm_pUserShadowData;
unsigned short GLRender::m_UsedVertices[4096];
unsigned int GLRender::cm_nScreenWidthPow2;
unsigned int GLRender::cm_nScreenHeightPow2;
int GLRender::cm_nGLCurrentTID[6];
unsigned int GLRender::cm_uiCurrentStage;
int totalvertexpool4k;
int totalindexpool;
int totalindexpool4;

GLRender::PerInstanceColor GLRender::m_PerInstanceIllumination = { 0.5f, 0.5f, 0.5f, 1.0f };
GLRender::PerInstanceColor GLRender::m_ForceSightColor = { 0.0f, 0.0f, 0.0f, 1.0f };
unsigned int GLRender::cm_nAnisotropy = 1;
bool GLRender::cm_bVSync = true;
float GLRender::m_fAccumulationRatio = 0.980000019f;
bool GLRender::cm_bFirstFrameBufferFrame = true;
bool GLRender::cm_bSetUpFrameBuffer = true;
bool GLRender::cm_bSetUpImageSpaceShadows = true;
bool GLRender::cm_bPBuffersAllocated;
unsigned int GLRender::cm_nOverbrightenRed;
unsigned int GLRender::cm_nOverbrightenGreen;
unsigned int GLRender::cm_nOverbrightenBlue;
unsigned int GLRender::cm_nRedTexture;
unsigned int GLRender::cm_nGreenTexture;
unsigned int GLRender::cm_nBlueTexture;
unsigned int GLRender::cm_nTargetTexture;
bool GLRender::m_bSpeedBlurEnabled;
bool GLRender::m_bScanNoiseEnabled;
bool GLRender::m_bFilmNoiseEnabled;
CAurTexture* GLRender::m_pDreamDistortTexture;
bool GLRender::m_bDreamDistortEnabled;
bool GLRender::m_bDreamDistortLetterbox;
CAurTexture* GLRender::m_pHologramFilterTexture;
short GLRender::m_FuryLevel;
CAurTexture* GLRender::m_pFuryTexture;
CAurTexture* GLRender::m_pDirtyTexture[3];
float GLRender::m_fSaturation;
int GLRender::cm_bEnablePausePlane;
unsigned int GLRender::cm_tidAttenuationTexture;
void* GLRender::m_hGLRC;
void* GLRender::m_hDC;
void (*GLRender::pfuncEnablePixelShaders)(int, void *, void *, void *, void *);
void (*GLRender::pfuncSetPixelShaderAmbientColor)(Vector&, float);
void (*GLRender::pfuncSetBumpLightColor)(Vector&, float);
HDC__* GLRender::cm_hSoftShadowHDC;
HGLRC__* GLRender::cm_hSoftShadowHGLRC;
unsigned int GLRender::cm_nSoftShadowRenderTexture;
HPBUFFERARB__* GLRender::cm_hSoftShadowPBuffer;
HDC__* GLRender::cm_hSoftShadowTargetHDC;
HGLRC__* GLRender::cm_hSoftShadowTargetHGLRC;
unsigned int GLRender::cm_nSoftShadowTargetRenderTexture;
HPBUFFERARB__* GLRender::cm_hSoftShadowTargetPBuffer;
unsigned int GLRender::cm_nBufferRegionTexture;
#if !KOTOR1
unsigned int GLRender::s_ShadowKnockDownFBO;
unsigned int GLRender::s_ShadowBlurFBO;
#endif
unsigned int GLRender::cm_nPerPixelFlareShader;
unsigned int GLRender::cm_nConvolutionRenderTexture[6];
HDC__* GLRender::cm_hConvolutionHDC[6];
HGLRC__* GLRender::cm_hConvolutionHGLRC[6];
HPBUFFERARB__* GLRender::cm_hConvolutionPBuffer[6];
int GLRender::cm_nAccumulationBlur;
void* GLRender::cm_hAccumulationHDC;
void* GLRender::cm_hAccumulationHGLRC;
void* GLRender::cm_hAccumulationPBuffer;
#if !KOTOR1
unsigned int GLRender::s_ConvolutionFBOs[6];
unsigned int GLRender::s_AccumulationFBO;
unsigned int GLRender::s_ConvolutionRbs[6];
unsigned int GLRender::s_AccumulationRb;
#endif
unsigned int GLRender::cm_pHeatDistortionVertexProg;
unsigned int GLRender::cm_nPerPixelFlareShaderARB;
unsigned int GLRender::cm_nDecalShaderARB;
unsigned int GLRender::cm_nDecalShaderRectARB;
unsigned int GLRender::cm_n4DecalShaderARB;
unsigned int GLRender::cm_nSaturationShaderARB;
unsigned int GLRender::cm_nAccumulationBlurShaderARB;
unsigned int GLRender::cm_nFilmNoiseShaderARB;
unsigned int GLRender::cm_nForceDistortionShaderARB;
unsigned int GLRender::cm_nSoftShadowShaderARB;
unsigned int GLRender::cm_nOverbrightenShaderARB;
unsigned int GLRender::cm_nDreamDistortShaderARB;
CAurTexture* GLRender::cm_pFilmNoiseTexture;
CAurTexture* GLRender::cm_pForceDistortionTexture;

extern "C" void glBindFramebuffer(unsigned int eTarget, unsigned int nFramebuffer);
extern "C" void glFramebufferTexture2D(unsigned int eTarget, unsigned int eAttachment,
                                       unsigned int eTextureTarget, unsigned int nTexture,
                                       int nLevel);
extern "C" void glDeleteFramebuffers(int nCount, const unsigned int* paFramebuffers);
#if !KOTOR1
extern "C" void glGenFramebuffers(int nCount, unsigned int* paFramebuffers);
#endif
#if KOTOR1 // K1: the C++ symbol
int gluBuild2DMipmaps(unsigned int eTarget, int nComponents, int nWidth, int nHeight,
                      unsigned int eFormat, unsigned int eType, const void* pData);
#else
extern "C" int gluBuild2DMipmaps(unsigned int eTarget, int nComponents, int nWidth, int nHeight,
                                 unsigned int eFormat, unsigned int eType, const void* pData);
#endif

extern HWND__* g_hRenderWnd;
extern int usearbvertexprograms;
char CheckExtension(char* pszName, const unsigned char* pszExtensions);
extern int existingextensions;
extern int GL_STENCIL_TWO_SIDED_BIT;
extern int EXT_BGRA_BIT;
extern int ARB_MULTITEXTURE_BIT;
extern int EXT_TEXTURE_ENV_COMBINE_BIT;
extern int NV_REGISTER_COMBINERS_BIT;
extern int NV_REGISTER_COMBINERS2_BIT;
extern int EXT_TEXTURE_CUBE_MAP_BIT;
extern int EXT_FOG_COORD_BIT;
extern int EXT_SECONDARY_COLOR_BIT;
extern int NV_VERTEX_PROGRAMS_BIT;
extern int EXT_TEXTURE_FILTER_ANISOTROPIC_BIT;
extern int NV_PIXEL_SHADERS_BIT;
extern int NV_VERTEX_ARRAY_RANGE_BIT;
extern int NV_VERTEX_ARRAY_RANGE2_BIT;
extern int NV_FENCE_BIT;
extern int ARB_TEXTURE_COMPRESSION_BIT;
extern int EXT_TEXTURE_COMPRESSION_S3TC_BIT;
extern int ATIX_PN_TRIANGLES_BIT;
extern int EXT_COMPILED_VERTEX_ARRAY_BIT;
extern int WGL_ARB_RENDER_TO_TEXTURE_BIT;
extern int EXT_SGIS_MIPMAP_BIT;
extern int ATI_FRAGMENT_SHADER_BIT;
extern int GL_ARB_VERTEX_PROGRAM_BIT;
extern int GL_ARB_FRAGMENT_PROGRAM_BIT;
extern int GL_ARB_DEPTH_TEXTURE_BIT;
extern int GL_ARB_VERTEX_BUFFER_OBJECT_BIT;
extern int WGL_NV_RENDER_TO_TEXTURE_RECTANGLE_BIT;
extern int WGL_ARB_MULTISAMPLE_BIT;
extern int WGL_ARB_BUFFER_REGION_BIT;
extern int EXT_SWAP_CONTROL_BIT;

extern const unsigned int primTypeTranslationTable[] = { 0, 1, 3, 4, 5, 6, 7, 8, 9, 2 };
extern int initialized;
extern int fontOffset;
int AurVertexBufferObjectARBAvailable();
int AurTwoSidedStencilAvailable();
int usetwosidedshadows;
extern int countfaces;
#if KOTOR1
unsigned int enableTranslationTable[] = { 3042, 3008, 2912, 2884, 3553, 2896, 3168, 3169, 3170, 34336, 2960, 2929, 34067, 2977, 0, 2915, 2916, 2914, 2918 };
#else
unsigned int enableTranslationTable[] = { 3042, 3008, 2912, 2884, 3553, 2896, 3168, 3169, 3170, 34336, 2960, 2929, 34067, 2977, 0, 2915, 2916, 2914, 2918, 32925, 32879 };
#endif
int noclipshadows;
int dontprotectagainstclip = 1;
void ClipProjection(Vector& v, const Vector& vOrigin, const Plane& plane);
int AurAnisotropicFilterAvailable();
int AurMultiTextureAvailable();
int AurVertexProgramsAvailable();
bool AurNV2xLevelShadersAvailable();
int AurATISoftShadowsAvailable();
int AurATIFrameBufferModificationsAvailable();
int AurSoftShadowsAvailable();
bool AuroraUseARBShaders();
const float* Homogenize(const Vector& v, const float& f);

extern int envmapon;
extern int tex1on;
extern int g_rgbaPauseOverlayColor;
extern "C" void gluOrtho2D(double fLeft, double fRight, double fBottom, double fTop);
void AurInitFont();
void AurTextureRelease(CAurTexture* pTexture);
CAurTexture* AurTextureGetReference(char* pszName, char* pszExtension);

#include "port/libcbase.h"
#include <string.h>
#include "port/libcmath.h"
namespace ASL { bool GetPrefAsBool(const char* pszKey, bool bDefault); }

int multmatrix = 1;

extern int counttrans;

bool glExtensionSupported(const char* pszExtension);
extern int  enablesoftshadows;
extern int  g_nScreenWidth;
extern int  g_nScreenHeight;
extern char s_frameBufferModificationsStarted;
#if KOTOR1
bool AurQueryGrassEnabled();
bool AurQueryShadowsEnabled();
bool IosGrassEnabled();
bool IosShadowsEnabled();
#ifdef _MSC_VER
extern int skippauseplaneforscreenshot;
#else
extern bool skippauseplaneforscreenshot;
#endif
extern int  g_IosAntiAlias;
#endif
extern bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
extern int  ASLGetIPhoneVersion();
extern int  ASLGetIPadVersion();
extern float g_fGammaExponent;
extern bool  gMac_GammaShaderNeedsInit;
extern unsigned char g_ForceSight;
extern unsigned char g_useDistortionOutput;
extern bool  g_bLetterBoxMode;
extern int   g_Letterbox_top;
extern int   g_Letterbox_bottom;
float multiplier = 0.349999994f;
float pixeloffset = 2.0f;
int junk;
static unsigned int s_anProbeTextures[1000];
extern unsigned int lookup;
extern const unsigned int texParamTargetTranslationTable[8] = {
    3553, 34067, 34069, 34070, 34071, 34072, 34073, 34074
};

int displaylayer = 5;
int baselayer;
float basesize = 64.0f;
int renderregular = 1;

#if !KOTOR1
float g_fIosDistortScale   __attribute__((visibility("hidden"))) = 1.9f;
float g_fIosBloomScaleU    __attribute__((visibility("hidden"))) = 0.75f;
float g_fIosBloomScaleV    __attribute__((visibility("hidden"))) = 0.8f;
float g_fIosDistortOffsetU __attribute__((visibility("hidden"))) = 0.5f;
float g_fIosDistortOffsetV __attribute__((visibility("hidden"))) = 2.4f;
#endif
float g_fIosBloomAmount    __attribute__((visibility("hidden")));
__attribute__((visibility("hidden"))) ImposterTable manRefImposter;
#if KOTOR1
float g_afIosPostParams[4] __attribute__((visibility("hidden"))) = { 0.6f, 1.9f, 1.0f, 3.0f };
#endif

static int  s_nSavedFramebuffer;
#if !KOTOR1
static int  s_nSavedDistortFramebuffer;
#endif
static int  s_nRecentDeviceVersion = -1;
static PBUFFERARB_IOS* s_pPBuffer;
#if !KOTOR1
static PBUFFERARB_IOS* s_pDistortPBuffer;
#endif
static PBUFFERARB_IOS* s_pHalfPBuffer1;
static PBUFFERARB_IOS* s_pHalfPBuffer2;
static PBUFFERARB_IOS* s_pHalfPBuffer3;


int visualizepass1;
extern int visualizepass2;
extern int visualizepass3;
extern int visualizepass4;
extern int visualizepass5;
extern int visualizepass6;
extern int visualizepass7;
#if !KOTOR1
static unsigned char s_bSpeedBlurApplied;
#endif
static unsigned char s_bSpeedBlurAppliedDefault;
#if !KOTOR1
static unsigned char s_bSpeedBlurAppliedATI;
#endif

void InitializeSystemPoolManager();
void InitializeVertexBufferObjectPoolManager();
extern char  szBumpyShinySetupVPARB[];
extern char  szBumpyShinySkinSetupVPARB[];
extern char  szBumpyShinyVPARB_ATI[];
extern char  szBumpyShinyVPARB_NV[];
extern char* szBumpyShinyVPARB;
extern char* szBumpyShinySkinVPARB;
extern bool  g_bVertexBufferObjectsWriteOnly;
extern float largest_supported_anisotropy;

static RequestPoolManager* manRef;

#include <stddef.h>
#include <string.h>

RequestPoolManager::~RequestPoolManager()
{
}

RequestPoolManagerSys::RequestPoolManagerSys()
{
    m_nNumPools = 100;
    m_apPoolData = new char*[m_nNumPools];
    m_anPoolSize = new unsigned int[m_nNumPools];
    memset(m_anPoolSize, 0, sizeof(unsigned int) * m_nNumPools);
    memset(m_apPoolData, 0, sizeof(char*) * m_nNumPools);
    for (uint64_t i = 0; i < 100; i++)
    {
        m_aPoolIndex.Add(i);
        m_anPoolInUse.Add(0);
    }
}

void InitializeSystemPoolManager()
{
    if (manRef)
        return;
    manRef = new RequestPoolManagerSys;
}

RequestPoolManagerSys::~RequestPoolManagerSys()
{
    for (unsigned int i = 0; i < m_nNumPools; i++)
        if (m_apPoolData[i])
            delete[] m_apPoolData[i];
    delete[] m_apPoolData;
    delete[] m_anPoolSize;
}


int totalvertexpool;
extern int totalvertexpool4k;
static unsigned int s_nSysPoolBytesRequested;
static unsigned int s_nVboPoolBytesRequested;

void* RequestPoolManagerSys::LockPool(unsigned int nPool, unsigned int a, unsigned int b, unsigned int c)
{
    K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c);
    if (nPool == static_cast<unsigned int>(-1) || m_anPoolInUse[nPool])
        return 0;
    m_anPoolInUse[nPool] = 1;
    return m_apPoolData[nPool];
}

void RequestPoolManagerSys::UnlockPool(unsigned int nPool)
{
    if (nPool != static_cast<unsigned int>(-1) && m_anPoolInUse[nPool])
        m_anPoolInUse[nPool] = 0;
}

void* RequestPoolManagerSys::GetPool(unsigned int nPool)
{
    if (nPool == static_cast<unsigned int>(-1))
        return 0;
    return m_apPoolData[nPool];
}

void* RequestPoolManagerSys::GetIndexPool(unsigned int nPool)
{
    if (nPool == static_cast<unsigned int>(-1))
        return 0;
    return m_apPoolData[nPool];
}

void RequestPoolManagerSys::DeletePool(unsigned int nPool)
{
    if (nPool == static_cast<unsigned int>(-1))
        return;
    m_aPoolIndex.Add(nPool);
    totalvertexpool -= m_anPoolSize[nPool];
    totalvertexpool4k -= (m_anPoolSize[nPool] + kPoolPageSlack) & kPoolPageMask;
    m_anPoolInUse[nPool] = 0;
    if (m_apPoolData[nPool])
        delete[] m_apPoolData[nPool];
    m_apPoolData[nPool] = 0;
    m_anPoolSize[nPool] = 0;
}

void RequestPoolManagerSys::ResizePool(unsigned int nPool, unsigned int nSize)
{
    totalvertexpool += nSize - m_anPoolSize[nPool];
    totalvertexpool4k +=
        ((nSize + kPoolPageSlack) & kPoolPageMask) - ((m_anPoolSize[nPool] + kPoolPageSlack) & kPoolPageMask);
    char* pOld = m_apPoolData[nPool];
    char* pNew = new char[nSize];
    memcpy(pNew, pOld, m_anPoolSize[nPool]);  // idiom-exempt: a runtime byte count (the pool's current length), no sizeof exists
    if (pOld)
        delete[] pOld;
    m_anPoolSize[nPool] = nSize;
    m_apPoolData[nPool] = pNew;
}

unsigned int RequestPoolManagerSys::GetPoolSize(unsigned int nPool)
{
    return m_anPoolSize[nPool];
}

void RequestPoolManagerSys::RefreshPool(unsigned int a, unsigned int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
}

void RequestPoolManagerSys::BackUpPoolManager()
{
}

void RequestPoolManagerSys::RestorePoolManager()
{
}

unsigned int RequestPoolManagerSys::RequestPoolGeneric(Aurora::AuroraBufferRequestType eType,
                                                      unsigned int a, unsigned int b)
{
    K2_UNUSED(eType); K2_UNUSED(b);
    unsigned int nSize = a;
    totalvertexpool += nSize;
    totalvertexpool4k += (nSize + kPoolPageSlack) & kPoolPageMask;
    if (nSize == 0)
        return static_cast<unsigned int>(-1);
    s_nSysPoolBytesRequested += nSize;

    if (m_aPoolIndex.m_count)
    {
        char* pNew = new char[nSize];
        unsigned int nPool = static_cast<unsigned int>(m_aPoolIndex[m_aPoolIndex.m_count - 1]);
        m_aPoolIndex.DelIndex(m_aPoolIndex.m_count - 1);
        m_apPoolData[nPool] = pNew;
        m_anPoolSize[nPool] = nSize;
        return nPool;
    }

    m_anPoolInUse.Add(0);
    for (unsigned int i = m_nNumPools + 1; i < m_nNumPools * 2; i++)
    {
        m_aPoolIndex.Add(i);
        m_anPoolInUse.Add(0);
    }

    unsigned int nNewNumPools = m_nNumPools * 2;
    char** paPoolData = new char*[nNewNumPools];
    unsigned int* panPoolSize = new unsigned int[nNewNumPools];
    memset(paPoolData, 0, sizeof(char*) * nNewNumPools);
    memcpy(paPoolData, m_apPoolData, sizeof(char*) * m_nNumPools);
    if (m_apPoolData)
        delete[] m_apPoolData;
    memset(panPoolSize, 0, sizeof(unsigned int) * nNewNumPools);
    memcpy(panPoolSize, m_anPoolSize, sizeof(unsigned int) * m_nNumPools);
    if (m_anPoolSize)
        delete[] m_anPoolSize;
    m_apPoolData = paPoolData;
    m_anPoolSize = panPoolSize;
    m_apPoolData[m_nNumPools] = new char[nSize];
    m_anPoolSize[m_nNumPools] = nSize;
    unsigned int nPool = m_nNumPools;
    m_nNumPools = nPool * 2;
    return nPool;
}

unsigned int RequestPoolManagerVBO::RequestPoolGeneric(Aurora::AuroraBufferRequestType eType,
                                                       unsigned int a, unsigned int b)
{
    if (a == 0)
        return static_cast<unsigned int>(-1);
    s_nVboPoolBytesRequested += a;

    if (m_aPoolIndex.m_count)
    {
        unsigned int nPool = static_cast<unsigned int>(m_aPoolIndex[m_aPoolIndex.m_count - 1]);
        m_aPoolIndex.DelIndex(m_aPoolIndex.m_count - 1);
        m_anPoolInUse[nPool] = 0;
        m_anLockFlags[nPool] = b;
        m_anBufferID[nPool] = 0;
        ASLgl::glGenBuffersARB(1, &m_anBufferID[nPool]);
        m_anBufferSize[nPool] = a;

        unsigned int nTarget;
        if (eType)
        {
            nTarget = GL_ELEMENT_ARRAY_BUFFER;
            m_anBufferTarget[nPool] = GL_ELEMENT_ARRAY_BUFFER;
            ASLgl::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, m_anBufferID[nPool]);
            ASLgl::glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, a, 0, GL_DYNAMIC_DRAW);
        }
        else
        {
            nTarget = GL_ARRAY_BUFFER;
            m_anBufferTarget[nPool] = GL_ARRAY_BUFFER;
            ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, m_anBufferID[nPool]);
            ASLgl::glBufferDataARB(GL_ARRAY_BUFFER, a, 0, GL_DYNAMIC_DRAW);
        }
        (void)nTarget;
        return nPool;
    }

    m_anPoolInUse.Add(0);
    m_anLockFlags.Add(0);
    m_anBufferID.Add(0);
    m_anBufferTarget.Add(GL_ARRAY_BUFFER);
    m_anBufferSize.Add(0);
    m_apBufferData.Add(0);
#if !KOTOR1
    m_anPoolOffset.Add(0);
    m_anPoolLength.Add(0);
    m_abBufferDirty.Add(true);
#endif
    for (unsigned int i = m_nNumPools + 1; i < m_nNumPools * 2; i++)
    {
        m_aPoolIndex.Add(i);
        m_anPoolInUse.Add(0);
        m_anLockFlags.Add(0);
        m_anBufferID.Add(0);
        m_anBufferSize.Add(0);
        m_apBufferData.Add(0);
#if !KOTOR1
        m_anPoolOffset.Add(0);
        m_anPoolLength.Add(0);
        m_abBufferDirty.Add(true);
#endif
        m_anBufferTarget.Add(GL_ARRAY_BUFFER);
    }

    ASLgl::glGenBuffersARB(1, &m_anBufferID[m_nNumPools]);
    unsigned int nTarget = eType ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    m_anBufferTarget[m_nNumPools] = nTarget;
    ASLgl::glBindBufferARB(nTarget, m_anBufferID[m_nNumPools]);
    ASLgl::glBufferDataARB(nTarget, a, 0, GL_DYNAMIC_DRAW);
    m_anBufferSize[m_nNumPools] = a;
    unsigned int nPool = m_nNumPools;
    m_nNumPools = nPool * 2;
    return nPool;
}


#ifndef K2_MOBILE_OPT
static RequestPoolManagerVBO* s_pVboManager;
#if KOTOR1
// Not in the image: K1 pools GLRender::UnlockPoolDeferred left for GetPool to upload.
static List<unsigned char> s_abK1PoolDirty;
#endif
#endif

// A dirty slot is re-uploaded whole, with the flag cleared, before the id is handed back.
void* RequestPoolManagerVBO::GetPool(unsigned int nPool)
{
    if (nPool == static_cast<unsigned int>(-1))
        return 0;
#if KOTOR1 && !defined(K2_MOBILE_OPT)
    if (nPool < static_cast<unsigned int>(s_abK1PoolDirty.m_count) && s_abK1PoolDirty[nPool])
    {
        s_abK1PoolDirty[nPool] = 0;
        if (m_apBufferData[nPool])
        {
            ASLgl::glBindBufferARB(m_anBufferTarget[nPool], m_anBufferID[nPool]);
            ASLgl::glBufferDataARB(m_anBufferTarget[nPool], m_anBufferSize[nPool],
                                   m_apBufferData[nPool], GL_DYNAMIC_DRAW);
        }
    }
#endif
#if !KOTOR1 // K1: only the id (UnlockPool already uploaded)
    if (m_abBufferDirty[nPool])
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, m_anBufferID[nPool]);
        ASLgl::glBufferDataARB(m_anBufferTarget[nPool],
                               m_anBufferSize[nPool],
                               m_apBufferData[nPool], GL_DYNAMIC_DRAW);
        m_abBufferDirty[nPool] = false;
    }
#endif
    return reinterpret_cast<void*>(static_cast<uintptr_t>(m_anBufferID[nPool]));
}

void* RequestPoolManagerVBO::GetIndexPool(unsigned int nPool)
{
    if (nPool == static_cast<unsigned int>(-1))
        return 0;
    ASLgl::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, m_anBufferID[nPool]);
#if !KOTOR1 // K1: the bind only
    if (m_abBufferDirty[nPool])
    {
        ASLgl::glBufferDataARB(m_anBufferTarget[nPool],
                               m_anBufferSize[nPool],
                               m_apBufferData[nPool], GL_DYNAMIC_DRAW);
        m_abBufferDirty[nPool] = false;
    }
#endif
    return 0;
}

void* RequestPoolManagerVBO::LockPool(unsigned int nPool, unsigned int a, unsigned int b,
                                      unsigned int c)
{
    if (nPool == static_cast<unsigned int>(-1) || m_anPoolInUse[nPool])
        return 0;
    ASLgl::glBindBufferARB(m_anBufferTarget[nPool],
                           m_anBufferID[nPool]);
    m_anLockFlags[nPool] = c;
    m_anPoolInUse[nPool] = 1;
    char* pData = static_cast<char*>(m_apBufferData[nPool]);
    if (!pData)
    {
        pData = new char[m_anBufferSize[nPool]];
        m_apBufferData[nPool] = pData;
    }
#if KOTOR1 // K1: no lock window is recorded
    K2_UNUSED(b);
    return pData + a;
#else
    char* pResult = pData + a;
    m_anPoolOffset[nPool] = a;
    if (!a && !b)
        b = m_anBufferSize[nPool];
    m_anPoolLength[nPool] = b;
    return pResult;
#endif
}

void RequestPoolManagerVBO::UnlockPool(unsigned int nPool)
{
    if (nPool == static_cast<unsigned int>(-1) || !m_anPoolInUse[nPool])
        return;
    m_anPoolInUse[nPool] = 0;
    ASLgl::glBindBufferARB(m_anBufferTarget[nPool],
                           m_anBufferID[nPool]);
    if (m_anLockFlags[nPool] == 8)
        return;
#if KOTOR1 // K1: the whole buffer is re-uploaded on every unlock
    ASLgl::glBufferDataARB(m_anBufferTarget[nPool], m_anBufferSize[nPool],
                           m_apBufferData[nPool], GL_DYNAMIC_DRAW);
#else
    unsigned int nOffset = m_anPoolOffset[nPool];
    if (nOffset == 0 &&
        m_anPoolLength[nPool] == m_anBufferSize[nPool])
    {
        m_abBufferDirty[nPool] = true;
        return;
    }
    if (!m_abBufferDirty[nPool])
        ASLgl::glBufferSubDataARB(m_anBufferTarget[nPool], nOffset,
                                  m_anPoolLength[nPool],
                                  static_cast<char*>(m_apBufferData[nPool]) + nOffset);
#endif
}

void RequestPoolManagerVBO::DeletePool(unsigned int nPool)
{
    if (nPool == static_cast<unsigned int>(-1))
        return;
    m_aPoolIndex.Add(nPool);
    m_anPoolInUse[nPool] = 0;
    unsigned int nBuffer = m_anBufferID[nPool];
    ASLgl::glDeleteBuffersARB(1, &nBuffer);
    m_anBufferID[nPool] = 0;
    m_anBufferSize[nPool] = 0;
    if (m_apBufferData[nPool])
        delete[] static_cast<char*>(m_apBufferData[nPool]);
    m_apBufferData[nPool] = 0;
}

void RequestPoolManagerVBO::ResizePool(unsigned int nPool, unsigned int nSize)
{
    if (nPool == static_cast<unsigned int>(-1))
        return;
    ASLgl::glBindBufferARB(m_anBufferTarget[nPool],
                           m_anBufferID[nPool]);
    int nOldSize;
    ASLgl::glGetBufferParameterivARB(m_anBufferTarget[nPool], GL_BUFFER_SIZE, &nOldSize);
    char* pTemp = new char[nOldSize];
    memset(pTemp, 0, nOldSize);  // idiom-exempt: the cached buffer's own byte length, not a struct extent
    unsigned int nBuffer = m_anBufferID[nPool];
    ASLgl::glDeleteBuffersARB(1, &nBuffer);
    m_anBufferID[nPool] = 0;
    ASLgl::glGenBuffersARB(1, &m_anBufferID[nPool]);
    ASLgl::glBindBufferARB(m_anBufferTarget[nPool],
                           m_anBufferID[nPool]);
    ASLgl::glBufferDataARB(m_anBufferTarget[nPool], nSize, 0, GL_DYNAMIC_DRAW);
    ASLgl::glBufferSubDataARB(m_anBufferTarget[nPool], 0, nOldSize, pTemp);
    if (m_apBufferData[nPool])
        delete[] static_cast<char*>(m_apBufferData[nPool]);
    m_apBufferData[nPool] = 0;
    m_anBufferSize[nPool] = nSize;
    delete[] pTemp;
}

// Backs up every live buffer's byte size, then destroys the buffer.
#if KOTOR1
extern void* (*glMapBufferOES)(unsigned int eTarget, unsigned int eAccess);
extern unsigned char (*glUnmapBufferOES)(unsigned int eTarget);
#endif
void RequestPoolManagerVBO::BackUpPoolManager()
{
    for (int i = 0; i < m_anBufferID.m_count; i++)
    {
        if (m_anBufferID[i])
        {
            ASLgl::glBindBufferARB(m_anBufferTarget[i], m_anBufferID[i]);
            int nSize;
            ASLgl::glGetBufferParameterivARB(m_anBufferTarget[i], GL_BUFFER_SIZE, &nSize);
            m_anBackupSize.Add(static_cast<unsigned int>(nSize));
#if KOTOR1 // K1: a heap copy of the mapped buffer, not a marker
            m_apBackupData.Add(new char[nSize]);
// GL_BUFFER_MAPPED_OES, not an access mode.
            void* pMapped = glMapBufferOES(m_anBufferTarget[i], GL_BUFFER_MAPPED);
            memcpy(m_apBackupData[i], pMapped, m_anBackupSize[i]);  // idiom-exempt: the buffer's own GL byte size
            glUnmapBufferOES(m_anBufferTarget[i]);
#else
            m_apBackupData.Add(reinterpret_cast<char*>(1));
#endif
            ASLgl::glDeleteBuffersARB(1, &m_anBufferID[i]);
            m_anBufferID[i] = 0;
        }
        else
        {
            m_apBackupData.Add(0);
            m_anBackupSize.Add(0);
        }
    }
}

void RequestPoolManagerVBO::RestorePoolManager()
{
    for (int i = 0; i < m_apBackupData.m_count; i++)
    {
        if (!m_apBackupData[i])
            continue;
        unsigned int nBuffer;
        ASLgl::glGenBuffersARB(1, &nBuffer);
        m_anBufferID[i] = nBuffer;
        ASLgl::glBindBufferARB(m_anBufferTarget[i], nBuffer);
#if KOTOR1 // K1: re-uploads the backed-up copy and frees it
        ASLgl::glBufferDataARB(m_anBufferTarget[i], m_anBackupSize[i],
                               m_apBackupData[i], GL_DYNAMIC_DRAW);
        if (m_apBackupData[i])
            delete[] m_apBackupData[i];
#else
        ASLgl::glBufferDataARB(m_anBufferTarget[i], m_anBufferSize[i],
                               m_apBufferData[i], GL_DYNAMIC_DRAW);
#endif
    }
    m_apBackupData.m_count = 0;
    m_anBackupSize.m_count = 0;
    m_apBackupData.SetSize(0);
    m_anBackupSize.SetSize(0);
}

unsigned int RequestPoolManagerVBO::GetPoolSize(unsigned int nPool)
{
    if (nPool == static_cast<unsigned int>(-1))
        return 0;
    return m_anBufferSize[nPool];
}

void RequestPoolManagerVBO::RefreshPool(unsigned int a, unsigned int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
}

RequestPoolManagerVBO::RequestPoolManagerVBO()
{
    m_nNumPools = 100;
    for (uint64_t i = 0; i < m_nNumPools; i++)
    {
        m_aPoolIndex.Add(i);
        m_anLockFlags.Add(0);
        m_anPoolInUse.Add(0);
        m_anBufferID.Add(0);
        m_anBufferSize.Add(0);
        m_apBufferData.Add(0);
#if !KOTOR1
        m_anPoolOffset.Add(0);
        m_anPoolLength.Add(0);
        m_abBufferDirty.Add(true);
#endif
        m_anBufferTarget.Add(GL_ARRAY_BUFFER);
    }
}

void InitializeVertexBufferObjectPoolManager()
{
    if (manRef)
        return;
    manRef = new RequestPoolManagerVBO;
#ifndef K2_MOBILE_OPT
    s_pVboManager = static_cast<RequestPoolManagerVBO*>(manRef);
#endif
}

RequestPoolManagerVBO::~RequestPoolManagerVBO()
{
    for (unsigned int i = 0; i < m_nNumPools; i++)
    {
        if (m_anBufferID[i])
        {
            ASLgl::glDeleteBuffersARB(1, &m_anBufferID[i]);
            m_anBufferID[i] = 0;
        }
    }
#if KOTOR1 // K1: the backed-up buffer copies are freed too
    for (int i = 0; i < m_apBackupData.m_count; i++)
        if (m_apBackupData[i])
            delete[] m_apBackupData[i];
#endif
    m_apBackupData.SetSize(0);
    m_anBackupSize.SetSize(0);
    m_anBufferTarget.SetSize(0);
}


namespace {
// The fog-arrays pool record; only its first 12 bytes are written.
struct FogArraysInfo {
    void* m_pData;
    int m_nCount;
};

struct VertexArraysInfo {
    void* m_pData;
    int m_nCount;
};
}

unsigned int GLRender::RequestPool(Aurora::AuroraBufferRequestType eType, unsigned int a,
                                   unsigned int b, unsigned int c, unsigned int* pnOut)
{
    K2_UNUSED(c);
    unsigned int nPool = manRef->RequestPoolGeneric(eType, a, b);
    *pnOut = nPool;
    return nPool;
}

void* GLRender::LockPool(unsigned int nPool, unsigned int a, unsigned int b, unsigned int c)
{
    return manRef->LockPool(nPool, a, b, c);
}

void GLRender::UnlockPool(unsigned int nPool)
{
    manRef->UnlockPool(nPool);
}

#ifndef K2_MOBILE_OPT
// The restored dangly solver rewrites one node's slice of a model pool every frame, once per
// node. An immediate upload either copies the whole pool per node (K1's UnlockPool) or stalls on
// the GPU still reading the buffer (glBufferSubData), so the upload is deferred instead: the pool is
// marked dirty and GetPool sends it once, whole, before its next draw. K2's manager already has
// that dirty flag; K1's is kept beside it. The system-memory manager has nothing to upload.
void GLRender::UnlockPoolDeferred(unsigned int nPool)
{
    RequestPoolManagerVBO* pMan = s_pVboManager;
    if (pMan == 0 || nPool == static_cast<unsigned int>(-1) || !pMan->m_anPoolInUse[nPool] ||
        pMan->m_anLockFlags[nPool] == 8)
    {
        manRef->UnlockPool(nPool);
        return;
    }
    pMan->m_anPoolInUse[nPool] = 0;
#if KOTOR1
    while (s_abK1PoolDirty.m_count <= static_cast<int>(nPool))
        s_abK1PoolDirty.Add(0);
    s_abK1PoolDirty[nPool] = 1;
#else
    pMan->m_abBufferDirty[nPool] = true;
#endif
}
#endif

void GLRender::DeletePool(unsigned int nPool)
{
    if (nPool == static_cast<unsigned int>(-1))
        return;
    manRef->DeletePool(nPool);
}

void GLRender::ResizePool(unsigned int nPool, unsigned int nSize)
{
    manRef->ResizePool(nPool, nSize);
}

void* GLRender::GetPool(unsigned int nPool)
{
    return manRef->GetPool(nPool);
}

unsigned int GLRender::GetPoolSize(unsigned int nPool)
{
    return manRef->GetPoolSize(nPool);
}

int GLRender::GetAvailableTextureMemory()
{
    int nCount = 0;
    cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
    if (!junk)
    {
        unsigned char aTextureData[256 * 256 * 4];
        for (uint64_t i = 0; i < 1000; i++)
        {
            unsigned int nTexture;
            aTextureData[0] = static_cast<unsigned char>(i);
            ASLgl::glGenTextures(1, &nTexture);
            ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
            s_anProbeTextures[i] = nTexture;
            ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                aTextureData);
        }
    }
    junk = 1;
    for (uint64_t i = 0; i < 1000; i++)
    {
        int nResident = -1;
        ASLgl::glBindTexture(GL_TEXTURE_2D, s_anProbeTextures[i]);
        ASLgl::glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_RESIDENT, &nResident);
        if (nResident)
            nCount++;
    }
    return nCount << 18;
}

void GLRender::SetCurrentStageTID(unsigned int nStage, unsigned int nTID)
{
    cm_nGLCurrentTID[nStage] = nTID;
}

void GLRender::SetDirtyTexture(int nIndex, CAurTexture* pTexture)
{
    m_pDirtyTexture[nIndex] = pTexture;
}

void GLRender::SetFilmNoiseTexture(CAurTexture* pTexture)
{
    cm_pFilmNoiseTexture = pTexture;
}

int GLRender::SetSpecularMode(int nMode)
{
    K2_UNUSED(nMode);
    return 1;
}

// Displays a bitmap-font string through one glCallLists.
void SpitLetters(char* pszText)
{
    if (!initialized)
        AurInitFont();
    ASLgl::glPushAttrib(GL_LIST_BIT);
    ASLgl::glListBase(fontOffset);
    ASLgl::glCallLists(static_cast<int>(strlen(pszText)), GL_UNSIGNED_BYTE, pszText);
    ASLgl::glPopAttrib();
}

// Four fixed-pipeline vertex-buffer attach points follow. Each binds the pool as a VBO, or folds
// the offset into the base.

void GLRender::SetVertexBuffer(unsigned int nStride, unsigned int nOffset, unsigned int nPool,
                               int nCount)
{
    K2_UNUSED(nCount);
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pBuffer = static_cast<char*>(manRef->GetPool(nPool));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pBuffer)));
        pBuffer = reinterpret_cast<char*>(static_cast<uint64_t>(nOffset));
    }
    else
        pBuffer += nOffset;
    ASLgl::glVertexPointer(3, GL_FLOAT, nStride, pBuffer);
}

void GLRender::SetNormalBuffer(unsigned int nStride, unsigned int nOffset, unsigned int nPool,
                               int nCount)
{
    K2_UNUSED(nCount);
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pBuffer = static_cast<char*>(manRef->GetPool(nPool));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pBuffer)));
        pBuffer = reinterpret_cast<char*>(static_cast<uint64_t>(nOffset));
    }
    else
        pBuffer += nOffset;
    ASLgl::glNormalPointer(GL_FLOAT, nStride, pBuffer);
}

void GLRender::SetTexCoordBuffer(unsigned int nStride, unsigned int nOffset, unsigned int nPool,
                                 int nCount)
{
    K2_UNUSED(nCount);
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pBuffer = static_cast<char*>(manRef->GetPool(nPool));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pBuffer)));
        pBuffer = reinterpret_cast<char*>(static_cast<uint64_t>(nOffset));
    }
    else
        pBuffer += nOffset;
    ASLgl::glTexCoordPointer(2, GL_FLOAT, nStride, pBuffer);
}

void GLRender::SetColorBuffer(unsigned int nStride, unsigned int nOffset, unsigned int nPool,
                              int nCount)
{
    K2_UNUSED(nCount);
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pBuffer = static_cast<char*>(manRef->GetPool(nPool));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pBuffer)));
        pBuffer = reinterpret_cast<char*>(static_cast<uint64_t>(nOffset));
    }
    else
        pBuffer += nOffset;
    ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, nStride, pBuffer);
}

// The lightmap and decal texcoord binders are the same body with the node's two texcoord sets swapped.

void GLRender::SetLightmapTexCoordToStage0(unsigned int nStride, int nFlags,
                                           unsigned int nPool, int nGate,
                                           MdlNodeTriMesh* pNode, unsigned int nGeomBase)
{
    K2_UNUSED(nStride); K2_UNUSED(nFlags); K2_UNUSED(nGate);
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pBuffer = static_cast<char*>(manRef->GetPool(nPool));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pBuffer)));
        GLRender::ActivateTextureStage(0);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
            reinterpret_cast<char*>(static_cast<uint64_t>(static_cast<unsigned int>(pNode->m_nTex1Ofs))) + nGeomBase);
        GLRender::ActivateTextureStage(1);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
            reinterpret_cast<char*>(static_cast<uint64_t>(static_cast<unsigned int>(pNode->m_nTex0Ofs))) + nGeomBase);
    }
    else
    {
        pBuffer += nGeomBase;
        GLRender::ActivateTextureStage(0);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
            pBuffer + pNode->m_nTex1Ofs);
        GLRender::ActivateTextureStage(1);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
            pBuffer + pNode->m_nTex0Ofs);
    }
    GLRender::ActivateTextureStage(0);
}

// Binds the decal texcoords with the same body, the node's two texcoord sets the other way round.
void GLRender::SetDecalTexCoordToStage0(unsigned int nStride, int nFlags,
                                        unsigned int nPool, int nGate,
                                        MdlNodeTriMesh* pNode, unsigned int nGeomBase)
{
    K2_UNUSED(nStride); K2_UNUSED(nFlags); K2_UNUSED(nGate);
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pBuffer = static_cast<char*>(manRef->GetPool(nPool));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pBuffer)));
        GLRender::ActivateTextureStage(0);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
            reinterpret_cast<char*>(static_cast<uint64_t>(static_cast<unsigned int>(pNode->m_nTex0Ofs))) + nGeomBase);
        GLRender::ActivateTextureStage(1);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
            reinterpret_cast<char*>(static_cast<uint64_t>(static_cast<unsigned int>(pNode->m_nTex1Ofs))) + nGeomBase);
    }
    else
    {
        pBuffer += nGeomBase;
        GLRender::ActivateTextureStage(0);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
            pBuffer + pNode->m_nTex0Ofs);
        GLRender::ActivateTextureStage(1);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
            pBuffer + pNode->m_nTex1Ofs);
    }
    GLRender::ActivateTextureStage(0);
}

void GLRender::DrawElementStreamQuads(Aurora::AuroraPrimitiveTypes eType, unsigned int nCount,
                                      unsigned int nStart, bool bUnused)
{
    K2_UNUSED(bUnused);
    ASLgl::glDrawArrays(primTypeTranslationTable[eType], nStart, nCount);
}

void GLRender::DrawElements(Aurora::AuroraPrimitiveTypes eType, unsigned int nCount,
                            unsigned int nOffset, unsigned int nPool)
{
    void* paIndices = manRef->GetIndexPool(nPool);
    ASLgl::glDrawElements(primTypeTranslationTable[eType], nCount, GL_UNSIGNED_SHORT,
                          static_cast<unsigned short*>(paIndices) + nOffset);
}

unsigned int GLRender::GetFogArraysInfoRef()
{
    unsigned int nPool = manRef->RequestPoolGeneric(
        static_cast<Aurora::AuroraBufferRequestType>(0), 16, 2);
    FogArraysInfo* pInfo = static_cast<FogArraysInfo*>(manRef->LockPool(nPool, 0, 0, 1));
    pInfo->m_pData = 0;
    pInfo->m_nCount = 0;
    manRef->UnlockPool(nPool);
    return nPool;
}

unsigned int GLRender::GetVertexArraysInfoRef(void* pInfo)
{
    unsigned int nPool = manRef->RequestPoolGeneric(
        static_cast<Aurora::AuroraBufferRequestType>(0), 16, 2);
    VertexArraysInfo* pRecord = static_cast<VertexArraysInfo*>(manRef->LockPool(nPool, 0, 0, 1));
    pRecord->m_pData = pInfo;
    pRecord->m_nCount = 0;
    manRef->UnlockPool(nPool);
    return nPool;
}


void GLRender::FinishUpSkinnedVertexArrays(unsigned int a, unsigned int b, unsigned int c,
                                           unsigned int d) { K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c); K2_UNUSED(d); }

void GLRender::UpdateLightVectorsBumped(unsigned int a, const Vector& v, MdlNodeTriMesh* pNode) { K2_UNUSED(a); K2_UNUSED(v); K2_UNUSED(pNode); }

void GLRender::UpdateHalfAngleVectorsBumped(unsigned int a, const Vector& v1, const Vector& v2,
                                            MdlNodeTriMesh* pNode) { K2_UNUSED(a); K2_UNUSED(v1); K2_UNUSED(v2); K2_UNUSED(pNode); }

void GLRender::UpdateFogValuesBumped(unsigned int a, const Vector& v, float f,
                                     MdlNodeTriMesh* pNode) { K2_UNUSED(a); K2_UNUSED(v); K2_UNUSED(f); K2_UNUSED(pNode); }

// The post-process env-parameter basis vectors.
static float s_afDreamDistortParam[4] = { 0.0f, 0.0f, 0.0f, 0.5f };
static float s_afSaturationBasisR[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
static float s_afSaturationBasisG[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
static float s_afSaturationBasisB[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
static float s_afOverbrightenBasisR[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
static float s_afOverbrightenBasisG[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
static float s_afOverbrightenBasisB[4] = { 0.0f, 0.0f, 1.0f, 0.0f };

void GLRender::FrameBufferModificationPixelShadersATI(unsigned int nMode, unsigned int a,
                                                      unsigned int b, unsigned int c,
                                                      unsigned int d)
{
#if KOTOR1
    K2_UNUSED(nMode); K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c); K2_UNUSED(d);
#endif
#if !KOTOR1 // K1: an empty body
    if (AuroraUseARBShaders())
    {
        FrameBufferModificationPixelShadersARB(nMode, a, b, c, d);
        return;
    }

    switch (nMode)
    {
    case kFBShaderFlare:
    case kFBShaderOverbrighten:
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, a);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, b);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, c);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, d);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    case kFBShaderOff:
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glDisable(GL_TEXTURE_1D);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glBindTexture(GL_TEXTURE_1D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_1D);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glBindTexture(GL_TEXTURE_1D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_1D);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_1D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        break;
    case kFBShaderDreamDistort:
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glEnable(GL_TEXTURE_2D);
        return;
    default:
        return;
    }

    ASLgl::glActiveTextureARB(GL_TEXTURE0);
#endif
}

void GLRender::FrameBufferModificationPixelShadersARB(unsigned int nMode, unsigned int a,
                                                      unsigned int b, unsigned int c,
                                                      unsigned int d)
{
#if KOTOR1 // K1: GL_TEXTURE_2D rectangle target, static overbrighten vectors, no dream arm
    switch (nMode)
    {
    case kFBShaderFlare:
        android_port_glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, a);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        android_port_glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, b);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        android_port_glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, c);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        android_port_glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, d);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        android_port_glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nPerPixelFlareShaderARB);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, s_afOverbrightenBasisR);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, s_afOverbrightenBasisG);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 2, s_afOverbrightenBasisB);
        break;
    case kFBShaderOff:
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        android_port_glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        android_port_glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glDisable(GL_TEXTURE_1D);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glBindTexture(GL_TEXTURE_1D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        android_port_glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_1D);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glBindTexture(GL_TEXTURE_1D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        android_port_glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_1D);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_1D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        android_port_glActiveTextureARB(GL_TEXTURE0);
        break;
    case kFBShaderDecal:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderARB);
        break;
    case kFBShaderDecal4:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_n4DecalShaderARB);
        break;
    case kFBShaderSaturation:
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 3, s_afSaturationBasisR);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 4, s_afSaturationBasisG);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 5, s_afSaturationBasisB);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSaturationShaderARB);
        break;
    case kFBShaderFilmNoise:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nFilmNoiseShaderARB);
        break;
    case kFBShaderForceDistortion:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nForceDistortionShaderARB);
        break;
    case kFBShaderAccumulationBlur:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nAccumulationBlurShaderARB);
        break;
    case kFBShaderSoftShadow:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSoftShadowShaderARB);
        break;
    case kFBShaderOverbrighten:
    {
        android_port_glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, a);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        android_port_glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, b);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        android_port_glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, c);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        android_port_glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, d);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        android_port_glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nOverbrightenShaderARB);
        static float afRed[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
        static float afGreen[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
        static float afBlue[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afRed);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afGreen);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 2, afBlue);
        break;
    }
    case kFBShaderDecalRect:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderRectARB);
        break;
    }
#else
    switch (nMode)
    {
    case kFBShaderFlare:
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, a);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, b);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, c);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, d);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nPerPixelFlareShaderARB);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, s_afOverbrightenBasisR);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, s_afOverbrightenBasisG);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 2, s_afOverbrightenBasisB);
        break;
    case kFBShaderOff:
        ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glDisable(GL_TEXTURE_1D);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glBindTexture(GL_TEXTURE_1D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_1D);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glBindTexture(GL_TEXTURE_1D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_1D);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_1D, 0);
        ASLgl::glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        break;
    case kFBShaderDecal:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderARB);
        break;
    case kFBShaderDecal4:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_n4DecalShaderARB);
        break;
    case kFBShaderSaturation:
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 3, s_afSaturationBasisR);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 4, s_afSaturationBasisG);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 5, s_afSaturationBasisB);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSaturationShaderARB);
        break;
    case kFBShaderFilmNoise:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nFilmNoiseShaderARB);
        break;
    case kFBShaderForceDistortion:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nForceDistortionShaderARB);
        break;
    case kFBShaderAccumulationBlur:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nAccumulationBlurShaderARB);
        break;
    case kFBShaderSoftShadow:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSoftShadowShaderARB);
        break;
    case kFBShaderOverbrighten:
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, a);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, b);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, c);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glEnable(GL_TEXTURE_1D);
        ASLgl::glBindTexture(GL_TEXTURE_1D, d);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nOverbrightenShaderARB);
        float afRed[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
        float afGreen[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
        float afBlue[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afRed);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afGreen);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 2, afBlue);
        break;
    }
    case kFBShaderDreamDistort:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, s_afDreamDistortParam);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDreamDistortShaderARB);
        break;
    case kFBShaderDecalRect:
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderRectARB);
        break;
    }
#endif
}

void GLRender::FrameBufferModificationPixelShaders(unsigned int a, unsigned int b, unsigned int c,
                                                   unsigned int d, unsigned int e) { K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c); K2_UNUSED(d); K2_UNUSED(e); }

void GLRender::InitializeImageSpaceSoftShadows()
{
#ifdef K2_QOL_SOFT_SHADOWS
    // the port's pass makes its own textures (QolSoftShadowsSetUp)
#elif !KOTOR1 // K1: an empty body
    if (AurSoftShadowsAvailable() && cm_bSetUpImageSpaceShadows && enablesoftshadows)
    {
        if (cm_nTargetTexture == 0)
        {
            ASLgl::glGenTextures(1, &cm_nTargetTexture);
            ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
            ASLgl::glCopyTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, 0, 0, g_nScreenWidth, g_nScreenHeight, 0);
            ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        }
        ASLgl::glGenTextures(1, &cm_nBufferRegionTexture);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nBufferRegionTexture);
        ASLgl::glCopyTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, 0, 0, g_nScreenWidth, g_nScreenHeight, 0);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        cm_hSoftShadowPBuffer = static_cast<HPBUFFERARB__*>(CreatePBuffer(512, 512));
        cm_hSoftShadowHDC = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hSoftShadowPBuffer));
        cm_hSoftShadowHGLRC = static_cast<HGLRC__*>(wglCreateContext(cm_hSoftShadowHDC));
        wglShareLists(m_hGLRC, cm_hSoftShadowHGLRC);
        wglMakeCurrent(cm_hSoftShadowHDC, cm_hSoftShadowHGLRC);
        ASLgl::glGenTextures(1, &cm_nSoftShadowRenderTexture);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nSoftShadowRenderTexture);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        cm_hSoftShadowTargetPBuffer = static_cast<HPBUFFERARB__*>(CreatePBuffer(512, 512));
        cm_hSoftShadowTargetHDC = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hSoftShadowTargetPBuffer));
        cm_hSoftShadowTargetHGLRC = static_cast<HGLRC__*>(wglCreateContext(cm_hSoftShadowTargetHDC));
        wglShareLists(m_hGLRC, cm_hSoftShadowTargetHGLRC);
        wglMakeCurrent(cm_hSoftShadowTargetHDC, cm_hSoftShadowTargetHGLRC);
        ASLgl::glGenTextures(1, &cm_nSoftShadowTargetRenderTexture);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nSoftShadowTargetRenderTexture);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
    }
    else if (AurATISoftShadowsAvailable() && cm_bSetUpImageSpaceShadows && enablesoftshadows)
    {
        if (AuroraUseARBShaders())
        {
            InitializeImageSpaceSoftShadowsARB();
        }
        else
        {
            cm_nScreenWidthPow2 = 2 << log2integer(g_nScreenWidth);
            cm_nScreenHeightPow2 = 2 << log2integer(g_nScreenHeight);
            if (cm_nTargetTexture == 0)
            {
                ASLgl::glGenTextures(1, &cm_nTargetTexture);
                ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
                ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                ASLgl::glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, cm_nScreenWidthPow2,
                                        cm_nScreenHeightPow2, 0);
                ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
            }
            ASLgl::glGenTextures(1, &cm_nBufferRegionTexture);
            ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nBufferRegionTexture);
            ASLgl::glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, cm_nScreenWidthPow2,
                                    cm_nScreenHeightPow2, 0);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            cm_hSoftShadowPBuffer = static_cast<HPBUFFERARB__*>(CreatePBuffer(512, 512));
            cm_hSoftShadowHDC = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hSoftShadowPBuffer));
            cm_hSoftShadowHGLRC = static_cast<HGLRC__*>(m_hGLRC);
            wglMakeContextCurrentARB(cm_hSoftShadowHDC, cm_hSoftShadowHDC, cm_hSoftShadowHGLRC);
            ASLgl::glGenTextures(1, &cm_nSoftShadowRenderTexture);
            ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nSoftShadowRenderTexture);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            wglMakeContextCurrentARB(m_hDC, m_hDC, m_hGLRC);

            cm_hSoftShadowTargetPBuffer = static_cast<HPBUFFERARB__*>(CreatePBuffer(512, 512));
            cm_hSoftShadowTargetHDC = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hSoftShadowTargetPBuffer));
            cm_hSoftShadowTargetHGLRC = static_cast<HGLRC__*>(m_hGLRC);
            wglMakeContextCurrentARB(cm_hSoftShadowTargetHDC, cm_hSoftShadowTargetHDC,
                                     cm_hSoftShadowTargetHGLRC);
            ASLgl::glGenTextures(1, &cm_nSoftShadowTargetRenderTexture);
            ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nSoftShadowTargetRenderTexture);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            wglMakeContextCurrentARB(m_hDC, m_hDC, m_hGLRC);
        }
    }
#endif
}

#if KOTOR1
// K1: a 4-tap blend program and two 512x512 OES pbuffers with their render textures.
void GLRender::InitializeImageSpaceSoftShadowsARB()
{
    static const char szSoftShadowFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "TEMP r0, r1, r2, r3, r4;\n"
    "PARAM c[4] = { program.env[0..3] };\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 2D;\n"
    "TEX r3, T3, texture[3], 2D;\n"
    "MUL r4.xyz, r0, c[0];\n"
    "MUL r4.w, r0, c[0];\n"
    "MAD r4.xyz, r1, c[1], r4;\n"
    "MAD r4.w, r1, c[1], r4;\n"
    "MAD r1.xyz, r2, c[2], r4;\n"
    "MAD r1.w, r2, c[2], r4;\n"
    "MAD r0.xyz, r3, c[3], r1;\n"
    "MAD r0.w, r3, c[3], r1;\n"
    "MOV result.color, r0;\n"
    "END\n";
    if (!cm_nTargetTexture)
    {
        ASLgl::glGenTextures(1, &cm_nTargetTexture);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, g_nScreenWidth, g_nScreenHeight, 0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    }
    ASLgl::glGenTextures(1, &cm_nBufferRegionTexture);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nBufferRegionTexture);
    ASLgl::glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, g_nScreenWidth, g_nScreenHeight, 0);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);

    cm_hSoftShadowPBuffer = static_cast<HPBUFFERARB__*>(CreatePBuffer(512, 512));
    _aglSetCurrentPBuffer(reinterpret_cast<PBUFFERARB*>(cm_hSoftShadowPBuffer));
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClearDepth(1.0);
    ASLgl::glClearStencil(0);
    ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    ASLgl::glGenTextures(1, &cm_nSoftShadowRenderTexture);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nSoftShadowRenderTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _aglRestoreContext();

    cm_hSoftShadowTargetPBuffer = static_cast<HPBUFFERARB__*>(CreatePBuffer(512, 512));
    _aglSetCurrentPBuffer(reinterpret_cast<PBUFFERARB*>(cm_hSoftShadowTargetPBuffer));
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClearDepth(1.0);
    ASLgl::glClearStencil(0);
    ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    ASLgl::glGenTextures(1, &cm_nSoftShadowTargetRenderTexture);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nSoftShadowTargetRenderTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    _aglRestoreContext();

    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glGenProgramsARB(1, &cm_nSoftShadowShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSoftShadowShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szSoftShadowFrag, sizeof(szSoftShadowFrag))),
                              szSoftShadowFrag);
    int nErrorPos;
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
}
#else
void GLRender::InitializeImageSpaceSoftShadowsARB() {}
#endif

void GLRender::InitializeFrameBufferModificationFragmentShadersATI() {}

void GLRender::InitializeAntiAliasing() {}

void GLRender::RenderPeripheralSmoke(unsigned int nTexture) { K2_UNUSED(nTexture); }


void GLRender::EnableDreamDistortion(bool bLetterbox)
{
    m_bDreamDistortEnabled = true;
    m_bDreamDistortLetterbox = bLetterbox;
}

void GLRender::DisableDreamDistortion()
{
    m_bDreamDistortEnabled = false;
}

void GLRender::EnableFury(short nLevel)
{
    unsigned short nCur = nLevel;
    if (nLevel <= 0)
        nCur = 0;
    else if (nLevel >= 3)
        nCur = 3;

    if (nLevel < 1)
    {
        if (m_pFuryTexture)
        {
            GLRender::SetActiveTexture(3);
            GLRender::Disable(Aurora::kCapTexture2D);
            GLRender::SetActiveTexture(0);
            AurTextureRelease(m_pFuryTexture);
            m_pFuryTexture = 0;
        }
        m_FuryLevel = 0;
        return;
    }

    if (m_FuryLevel == nCur)
        return;

    if (m_pFuryTexture)
    {
        cm_nGLCurrentTID[3] = 0;
        AurTextureRelease(m_pFuryTexture);
    }
    char* pszName;
    if (nCur == 3)
        pszName = const_cast<char*>("fx_Fury03");
    else if (nCur == 2)
        pszName = const_cast<char*>("fx_Fury02");
    else
        pszName = const_cast<char*>("fx_Fury01");
    m_pFuryTexture = AurTextureGetReference(pszName, 0);
    m_FuryLevel = nCur;
}


// The high half is shifted and masked first.
int ones32popcount(unsigned int nBits)
{
    nBits = nBits - ((nBits >> 1) & 0x55555555u);
    nBits = ((nBits >> 2) & 0x33333333u) + (nBits & 0x33333333u);
    nBits = (nBits + (nBits >> 4)) & 0x0f0f0f0fu;
    nBits = nBits + (nBits >> 8);
    nBits = nBits + (nBits >> 16);
    return nBits & 0x3f;
}

// Smears the bits down over every lower position; the population of the shifted word is the index
// of the highest set bit plus one.
int log2integer(unsigned int nValue)
{
    nValue |= nValue >> 1;
    nValue |= nValue >> 2;
    nValue |= nValue >> 4;
    nValue |= nValue >> 8;
    nValue |= nValue >> 16;
    return ones32popcount(nValue >> 1);
}

void FindSmallestPowerOf2WidthHeight(unsigned int nWidth, unsigned int nHeight,
                                     unsigned int& nPow2Width, unsigned int& nPow2Height)
{
    nPow2Width = 2 << log2integer(nWidth);
    nPow2Height = 2 << log2integer(nHeight);
}

int  AurFrameBufferModificationsAvailable();
int  AurATIFrameBufferModificationsAvailable();
void AurGetVertexProgramARBError(char* pszText);
int doframebuffer = 1;
float aquadraticterm = 0.5f;

// Sets up the non-ARB framebuffer path: six downsampling pbuffers, three ramps and the distortion program.
void GLRender::InitializeFrameBufferModifications()
{
#if KOTOR1 // K1: only the ARB path
    if (AuroraUseARBShaders())
        InitializeFrameBufferModificationsARB();
#else
    static char szForceDistortProg[] =
        "!!ARBvp1.0\n"
        "TEMP vReg0;\n"
        "TEMP vReg1;\n"
        "TEMP vReg2;\n"
        "TEMP vReg4, vReg3;\n"
        "DP4   result.position.x, state.matrix.mvp.row[0], vertex.position;\n"
        "DP4   result.position.y, state.matrix.mvp.row[1], vertex.position;\n"
        "DP4   result.position.z, state.matrix.mvp.row[2], vertex.position;\n"
        "DP4   result.position.w, state.matrix.mvp.row[3], vertex.position;\n"
        "MOV result.texcoord[0], vertex.texcoord[0];\n"
        "MOV vReg1, vertex.texcoord[1];\n"
        "MOV vReg2, vertex.texcoord[2];\n"
        "MOV vReg3.x, vReg1.x;\n"
        "MOV vReg3.y, program.env[15].x;\n"
        "MOV vReg3.z, vReg2.x;\n"
        "MOV vReg4.x, program.env[15].x;\n"
        "MOV vReg4.y, vReg1.y;\n"
        "MOV vReg4.z, vReg2.y;\n"
        "MOV result.texcoord[1].xyz, vReg3;\n"
        "MOV result.texcoord[2].xyz, vReg4;\n"
        "MOV result.color.primary, program.env[15].y;\n"
        "END";

#ifndef K2_QOL_FRAME_BUFFER_EFFECTS   // the scene pbuffer pass always runs; the switch trims it
    if (!doframebuffer)
        return;
#endif
    if (!cm_bSetUpFrameBuffer)
        return;
    if (AuroraUseARBShaders())
    {
        InitializeFrameBufferModificationsARB();
        return;
    }

    if (AurFrameBufferModificationsAvailable())
    {
        if (cm_nTargetTexture == 0)
        {
            ASLgl::glGenTextures(1, &cm_nTargetTexture);
            ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
            ASLgl::glCopyTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, 0, 0, g_nScreenWidth,
                                    g_nScreenHeight, 0);
            ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        }
        unsigned int anRed[256];
        unsigned int anGreen[256];
        unsigned int anBlue[256];
        unsigned int anRedOverbrighten[256];
        unsigned int anGreenOverbrighten[256];
        unsigned int anBlueOverbrighten[256];

        for (int i = 0; i < 256; i++)
            anRed[i] = (i < 234) ? 0 : static_cast<unsigned int>((static_cast<float>(i - 234) / 21.0f) * 255.0f * 0.7f);
        for (int i = 0; i < 256; i++)
            anGreen[i] = (i < 216) ? 0 : (static_cast<unsigned int>((static_cast<float>(i - 216) / 39.0f) * 255.0f * 0.7f) << 8);
        for (int i = 0; i < 256; i++)
            anBlue[i] = (i < 216) ? 0 : (static_cast<unsigned int>((static_cast<float>(i - 216) / 39.0f) * 255.0f * 0.7f) << 16);

        ASLgl::glGenTextures(1, &cm_nRedTexture);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nRedTexture);
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anRed);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glGenTextures(1, &cm_nGreenTexture);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nGreenTexture);
// Only the red ramp is uploaded as GL_RGBA8; green and blue use GL_RGBA.
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, anGreen);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glGenTextures(1, &cm_nBlueTexture);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nBlueTexture);
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anBlue);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        cm_bPBuffersAllocated = true;
        for (int i = 0; i < 6; i++)
        {
            cm_hConvolutionPBuffer[i] = 0;
            cm_hConvolutionHDC[i] = 0;
            cm_hConvolutionHGLRC[i] = 0;
            cm_nConvolutionRenderTexture[i] = 0;
        }

        cm_hConvolutionPBuffer[0] = static_cast<HPBUFFERARB__*>(CreatePBuffer(64, 64));
        cm_hConvolutionHDC[0] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[0]));
        cm_hConvolutionHGLRC[0] = static_cast<HGLRC__*>(wglCreateContext(cm_hConvolutionHDC[0]));
        wglShareLists(m_hGLRC, cm_hConvolutionHGLRC[0]);
        wglMakeCurrent(cm_hConvolutionHDC[0], cm_hConvolutionHGLRC[0]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[0]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[0]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[1] = static_cast<HPBUFFERARB__*>(CreatePBuffer(128, 128));
        cm_hConvolutionHDC[1] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[1]));
        cm_hConvolutionHGLRC[1] = static_cast<HGLRC__*>(wglCreateContext(cm_hConvolutionHDC[1]));
        wglShareLists(m_hGLRC, cm_hConvolutionHGLRC[1]);
        wglMakeCurrent(cm_hConvolutionHDC[1], cm_hConvolutionHGLRC[1]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[1]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[1]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[2] = static_cast<HPBUFFERARB__*>(CreatePBuffer(256, 256));
        cm_hConvolutionHDC[2] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[2]));
        cm_hConvolutionHGLRC[2] = static_cast<HGLRC__*>(wglCreateContext(cm_hConvolutionHDC[2]));
        wglShareLists(m_hGLRC, cm_hConvolutionHGLRC[2]);
        wglMakeCurrent(cm_hConvolutionHDC[2], cm_hConvolutionHGLRC[2]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[2]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[2]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[3] = static_cast<HPBUFFERARB__*>(CreatePBuffer(512, 512));
        cm_hConvolutionHDC[3] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[3]));
        cm_hConvolutionHGLRC[3] = static_cast<HGLRC__*>(wglCreateContext(cm_hConvolutionHDC[3]));
        wglShareLists(m_hGLRC, cm_hConvolutionHGLRC[3]);
        wglMakeCurrent(cm_hConvolutionHDC[3], cm_hConvolutionHGLRC[3]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[3]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[3]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[4] = static_cast<HPBUFFERARB__*>(CreatePBuffer(256, 256));
        cm_hConvolutionHDC[4] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[4]));
        cm_hConvolutionHGLRC[4] = static_cast<HGLRC__*>(wglCreateContext(cm_hConvolutionHDC[4]));
        wglShareLists(m_hGLRC, cm_hConvolutionHGLRC[4]);
        wglMakeCurrent(cm_hConvolutionHDC[4], cm_hConvolutionHGLRC[4]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[4]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[4]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[5] = static_cast<HPBUFFERARB__*>(CreatePBuffer(256, 256));
        cm_hConvolutionHDC[5] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[5]));
        cm_hConvolutionHGLRC[5] = static_cast<HGLRC__*>(wglCreateContext(cm_hConvolutionHDC[5]));
        wglShareLists(m_hGLRC, cm_hConvolutionHGLRC[5]);
        wglMakeCurrent(cm_hConvolutionHDC[5], cm_hConvolutionHGLRC[5]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[5]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[5]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);

        cm_hAccumulationHDC = 0;
        cm_hAccumulationHGLRC = 0;
        cm_hAccumulationPBuffer = 0;
        cm_nAccumulationBlur = 0;
        cm_bFirstFrameBufferFrame = true;
        cm_hAccumulationPBuffer = CreatePBuffer(128, 128);
        cm_hAccumulationHDC = wglGetPBufferDCARB(cm_hAccumulationPBuffer);
        cm_hAccumulationHGLRC = wglCreateContext(cm_hAccumulationHDC);
        wglShareLists(m_hGLRC, cm_hAccumulationHGLRC);
        wglMakeCurrent(cm_hAccumulationHDC, cm_hAccumulationHGLRC);
        ASLgl::glGenTextures(1, reinterpret_cast<unsigned int*>(&cm_nAccumulationBlur));
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);

        ASLgl::glEnable(GL_VERTEX_PROGRAM_ARB);
        ASLgl::glGenProgramsARB(1, &cm_pHeatDistortionVertexProg);
        ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, cm_pHeatDistortionVertexProg);
        ASLgl::glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szForceDistortProg, sizeof(szForceDistortProg))), szForceDistortProg);
        ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
        ASLgl::glDisable(GL_VERTEX_PROGRAM_ARB);
        AurGetVertexProgramARBError(szForceDistortProg);

        float fQuadraticTerm = aquadraticterm * 4.0f;
        for (int i = 0; i < 256; i++)
        {
            float f = static_cast<float>(i) / 255.0f;
            anRedOverbrighten[i] = (static_cast<unsigned int>((f < 0.5f ? f * f * fQuadraticTerm : sqrtf((f - 0.5f) * 0.5f) + 0.5f) * 255.0f));
        }
        for (int i = 0; i < 256; i++)
        {
            float f = static_cast<float>(i) / 255.0f;
            anGreenOverbrighten[i] = (static_cast<unsigned int>((f < 0.5f ? f * f * fQuadraticTerm : sqrtf((f - 0.5f) * 0.5f) + 0.5f) * 255.0f) << 8);
        }
        for (int i = 0; i < 256; i++)
        {
            float f = static_cast<float>(i) / 255.0f;
            anBlueOverbrighten[i] = (static_cast<unsigned int>((f < 0.5f ? f * f * fQuadraticTerm : sqrtf((f - 0.5f) * 0.5f) + 0.5f) * 255.0f) << 16);
        }

        ASLgl::glGenTextures(1, &cm_nOverbrightenRed);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nOverbrightenRed);
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anRedOverbrighten);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glGenTextures(1, &cm_nOverbrightenGreen);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nOverbrightenGreen);
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, anGreenOverbrighten);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glGenTextures(1, &cm_nOverbrightenBlue);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nOverbrightenBlue);
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anBlueOverbrighten);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    if (AurATIFrameBufferModificationsAvailable())
    {
        unsigned int nPow2Width = 2 << log2integer(g_nScreenWidth);
        unsigned int nPow2Height = 2 << log2integer(g_nScreenHeight);
        cm_nScreenWidthPow2 = nPow2Width;
        cm_nScreenHeightPow2 = nPow2Height;
        if (cm_nTargetTexture == 0)
        {
            ASLgl::glGenTextures(1, &cm_nTargetTexture);
            ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
            ASLgl::glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, cm_nScreenWidthPow2,
                                    cm_nScreenHeightPow2, 0);
            ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        }
        unsigned int anRed[256];
        unsigned int anGreen[256];
        unsigned int anBlue[256];
        unsigned int anRedOverbrighten[256];
        unsigned int anGreenOverbrighten[256];
        unsigned int anBlueOverbrighten[256];

        for (int i = 0; i < 256; i++)
            anRed[i] = (i < 234) ? 0 : static_cast<unsigned int>((static_cast<float>(i - 234) / 21.0f) * 255.0f * 0.7f);
        for (int i = 0; i < 256; i++)
            anGreen[i] = (i < 216) ? 0 : (static_cast<unsigned int>((static_cast<float>(i - 216) / 39.0f) * 255.0f * 0.7f) << 8);
        for (int i = 0; i < 256; i++)
            anBlue[i] = (i < 216) ? 0 : (static_cast<unsigned int>((static_cast<float>(i - 216) / 39.0f) * 255.0f * 0.7f) << 16);

        ASLgl::glGenTextures(1, &cm_nRedTexture);
        ASLgl::glBindTexture(GL_TEXTURE_1D, cm_nRedTexture);
        ASLgl::glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anRed);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glGenTextures(1, &cm_nGreenTexture);
        ASLgl::glBindTexture(GL_TEXTURE_1D, cm_nGreenTexture);
// Only the red ramp is RGBA8 here too.
        ASLgl::glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anGreen);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glGenTextures(1, &cm_nBlueTexture);
        ASLgl::glBindTexture(GL_TEXTURE_1D, cm_nBlueTexture);
        ASLgl::glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anBlue);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        cm_bPBuffersAllocated = true;
        for (int i = 0; i < 6; i++)
        {
            cm_hConvolutionPBuffer[i] = 0;
            cm_hConvolutionHDC[i] = 0;
            cm_hConvolutionHGLRC[i] = 0;
            cm_nConvolutionRenderTexture[i] = 0;
        }

        cm_hConvolutionPBuffer[0] = static_cast<HPBUFFERARB__*>(CreatePBuffer(64, 64));
        cm_hConvolutionHDC[0] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[0]));
        cm_hConvolutionHGLRC[0] = static_cast<HGLRC__*>(m_hGLRC);
        wglMakeCurrent(cm_hConvolutionHDC[0], cm_hConvolutionHGLRC[0]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[0]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[0]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[1] = static_cast<HPBUFFERARB__*>(CreatePBuffer(128, 128));
        cm_hConvolutionHDC[1] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[1]));
        cm_hConvolutionHGLRC[1] = static_cast<HGLRC__*>(m_hGLRC);
        wglMakeCurrent(cm_hConvolutionHDC[1], cm_hConvolutionHGLRC[1]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[1]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[1]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[2] = static_cast<HPBUFFERARB__*>(CreatePBuffer(256, 256));
        cm_hConvolutionHDC[2] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[2]));
        cm_hConvolutionHGLRC[2] = static_cast<HGLRC__*>(m_hGLRC);
        wglMakeCurrent(cm_hConvolutionHDC[2], cm_hConvolutionHGLRC[2]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[2]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[2]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[3] = static_cast<HPBUFFERARB__*>(CreatePBuffer(512, 512));
        cm_hConvolutionHDC[3] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[3]));
        cm_hConvolutionHGLRC[3] = static_cast<HGLRC__*>(m_hGLRC);
        wglMakeCurrent(cm_hConvolutionHDC[3], cm_hConvolutionHGLRC[3]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[3]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[3]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[4] = static_cast<HPBUFFERARB__*>(CreatePBuffer(256, 256));
        cm_hConvolutionHDC[4] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[4]));
        cm_hConvolutionHGLRC[4] = static_cast<HGLRC__*>(m_hGLRC);
        wglMakeCurrent(cm_hConvolutionHDC[4], cm_hConvolutionHGLRC[4]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[4]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[4]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);
        cm_hConvolutionPBuffer[5] = static_cast<HPBUFFERARB__*>(CreatePBuffer(256, 256));
        cm_hConvolutionHDC[5] = static_cast<HDC__*>(wglGetPBufferDCARB(cm_hConvolutionPBuffer[5]));
        cm_hConvolutionHGLRC[5] = static_cast<HGLRC__*>(m_hGLRC);
        wglMakeCurrent(cm_hConvolutionHDC[5], cm_hConvolutionHGLRC[5]);
        ASLgl::glGenTextures(1, &cm_nConvolutionRenderTexture[5]);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[5]);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);

        cm_hAccumulationHDC = 0;
        cm_hAccumulationHGLRC = 0;
        cm_hAccumulationPBuffer = 0;
        cm_nAccumulationBlur = 0;
        cm_bFirstFrameBufferFrame = true;
        cm_hAccumulationPBuffer = CreatePBuffer(128, 128);
        cm_hAccumulationHDC = wglGetPBufferDCARB(cm_hAccumulationPBuffer);
        cm_hAccumulationHGLRC = m_hGLRC;
        wglMakeCurrent(cm_hAccumulationHDC, cm_hAccumulationHGLRC);
        ASLgl::glGenTextures(1, reinterpret_cast<unsigned int*>(&cm_nAccumulationBlur));
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        wglMakeCurrent(m_hDC, m_hGLRC);

        ASLgl::glEnable(GL_VERTEX_PROGRAM_ARB);
        ASLgl::glGenProgramsARB(1, &cm_pHeatDistortionVertexProg);
        ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, cm_pHeatDistortionVertexProg);
        ASLgl::glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szForceDistortProg, sizeof(szForceDistortProg))), szForceDistortProg);
        ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
        ASLgl::glDisable(GL_VERTEX_PROGRAM_ARB);
        AurGetVertexProgramARBError(szForceDistortProg);

        float fQuadraticTerm = aquadraticterm * 4.0f;
        for (int i = 0; i < 256; i++)
        {
            float f = static_cast<float>(i) / 255.0f;
            anRedOverbrighten[i] = (static_cast<unsigned int>((f < 0.5f ? f * f * fQuadraticTerm : sqrtf((f - 0.5f) * 0.5f) + 0.5f) * 255.0f));
        }
        for (int i = 0; i < 256; i++)
        {
            float f = static_cast<float>(i) / 255.0f;
            anGreenOverbrighten[i] = (static_cast<unsigned int>((f < 0.5f ? f * f * fQuadraticTerm : sqrtf((f - 0.5f) * 0.5f) + 0.5f) * 255.0f) << 8);
        }
        for (int i = 0; i < 256; i++)
        {
            float f = static_cast<float>(i) / 255.0f;
            anBlueOverbrighten[i] = (static_cast<unsigned int>((f < 0.5f ? f * f * fQuadraticTerm : sqrtf((f - 0.5f) * 0.5f) + 0.5f) * 255.0f) << 16);
        }

        ASLgl::glGenTextures(1, &cm_nOverbrightenRed);
        ASLgl::glBindTexture(GL_TEXTURE_1D, cm_nOverbrightenRed);
        ASLgl::glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anRedOverbrighten);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glGenTextures(1, &cm_nOverbrightenGreen);
        ASLgl::glBindTexture(GL_TEXTURE_1D, cm_nOverbrightenGreen);
        ASLgl::glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anGreenOverbrighten);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glGenTextures(1, &cm_nOverbrightenBlue);
        ASLgl::glBindTexture(GL_TEXTURE_1D, cm_nOverbrightenBlue);
        ASLgl::glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, anBlueOverbrighten);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
#endif
}


Vector GLRender::m_vSaturationMultiplier(1.0f, 1.0f, 1.0f);

List<unsigned int> GLRender::cm_listSquaresToRender;

void GLRender::AddSquareToRender(unsigned int nHandle)
{
    if (nHandle == static_cast<unsigned int>(-1))
        return;
    cm_listSquaresToRender.Add(nHandle);
}

void GLRender::RemoveSquareToRender(unsigned int nHandle)
{
    if (nHandle == static_cast<unsigned int>(-1))
        return;
    if (cm_listSquaresToRender.Contains(nHandle) == 0)
        return;
    cm_listSquaresToRender.Remove(nHandle);
}

// Walks the handles from the top; each live one loses its last occurrence.
void GLRender::RemoveAllDrawableSquares()
{
    for (uint64_t i = cm_listSquaresToRender.m_count; i > 0; i--)
    {
        unsigned int nHandle = cm_listSquaresToRender.m_pData[i - 1];
        if (nHandle != static_cast<unsigned int>(-1) && cm_listSquaresToRender.Contains(nHandle))
            cm_listSquaresToRender.Remove(nHandle);
    }
}

// Draws one square as four consecutive V/C vertices.
void GLRender::RenderSquares(unsigned int nHandle)
{
    int bVBO = AurVertexBufferObjectARBAvailable();
    RenderVertexVC* pVertex = reinterpret_cast<RenderVertexVC*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nHandle)));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pVertex)));
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVC), static_cast<void*>(0));
        ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RenderVertexVC), reinterpret_cast<void*>(offsetof(RenderVertexVC, color)));
        ASLgl::glDrawArrays(2, 0, 4);
    }
    else
    {
        ASLgl::glBegin(2);
        ASLgl::glColor4ubv(reinterpret_cast<unsigned char*>(&pVertex->color));
        ASLgl::glVertex3fv(pVertex[0].pos);
        ASLgl::glVertex3fv(pVertex[1].pos);
        ASLgl::glVertex3fv(pVertex[2].pos);
        ASLgl::glVertex3fv(pVertex[3].pos);
        ASLgl::glEnd();
    }
}

// Draws every queued square with blend, texture and depth off.
void GLRender::RenderDrawableSquares()
{
#if KOTOR1 // K1: skipped while a screenshot is taken or with nothing queued; texturing is re-enabled
    if (skippauseplaneforscreenshot)
        return;
    if (!cm_listSquaresToRender.m_count)
        return;
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glDisable(GL_DEPTH_TEST);
    for (int64_t i = 0; i < cm_listSquaresToRender.m_count; i++)
        RenderSquares(cm_listSquaresToRender.m_pData[i]);
    ASLgl::glEnable(GL_DEPTH_TEST);
    ASLgl::glEnable(GL_LIGHTING);
    ASLgl::glEnable(GL_TEXTURE_2D);
#else
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glDisable(GL_DEPTH_TEST);
    for (int64_t i = 0; i < cm_listSquaresToRender.m_count; i++)
        RenderSquares(cm_listSquaresToRender.m_pData[i]);
    ASLgl::glEnable(GL_DEPTH_TEST);
    ASLgl::glEnable(GL_LIGHTING);
#endif
}

// Draws the box's six faces in black.
void GLRender::RenderBox(Vector& vMin, Vector& vMax)
{
    ASLgl::glColor3f(0.0f, 0.0f, 0.0f);
    ASLgl::glBegin(7);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMin.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMin.z);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMin.z);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMax.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMin.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMin.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMin.z);
    ASLgl::glEnd();
}

void GLRender::SetupGrassBuffers(unsigned int nPool, int nOffset)
{
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pBuffer = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
    float* pVerts;
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pBuffer)));
        pBuffer = 0;
        pVerts = 0;
    }
    else
        pVerts = reinterpret_cast<float*>(pBuffer);
    ASLgl::glVertexPointer(3, GL_FLOAT, 12, pBuffer);
    float* pVertex = pVerts + nOffset;
    ASLgl::glNormalPointer(GL_FLOAT, 28, pVertex);
    ASLgl::glTexCoordPointer(2, GL_FLOAT, 28, pVertex + 12);
    GLRender::ActivateTextureStage(1);
    ASLgl::glTexCoordPointer(2, GL_FLOAT, 28, pVertex + 20);
    GLRender::ActivateTextureStage(0);
}

void* GLRender::CreatePBufferRectangle(int nWidth, int nHeight)
{
#if KOTOR1 // K1: no pbuffer
    K2_UNUSED(nWidth); K2_UNUSED(nHeight);
    return 0;
#else
    int aPixelFormatAttribs[17] = {
        WGL_SUPPORT_OPENGL_ARB, 1, WGL_DRAW_TO_PBUFFER_ARB, 1, WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV, 1,
        WGL_RED_BITS_ARB, 8, WGL_GREEN_BITS_ARB, 8,
        WGL_BLUE_BITS_ARB, 8, WGL_ALPHA_BITS_ARB, 8, WGL_DOUBLE_BUFFER_ARB, 0, 0
    };
    int nPixelFormat;
    float fAttribF = 0.0f;
    unsigned int nNumFormats;
    void* hDC = m_hDC;
    wglChoosePixelFormatARB(hDC, aPixelFormatAttribs, &fAttribF, 1, &nPixelFormat, &nNumFormats);
    int aPbufferAttribs[6] = {
        WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB, WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_RECTANGLE_NV, 0, 0
    };
    return wglCreatePBufferARB(hDC, nPixelFormat, nWidth, nHeight, aPbufferAttribs);
#endif
}

void* GLRender::CreatePBuffer(int nWidth, int nHeight)
{
#if KOTOR1 // K1: an OES framebuffer with an RGBA texture and a 24-bit depth renderbuffer
    PBUFFERARB* pPBuffer = new PBUFFERARB;
    memset(pPBuffer, 0, 16);  // idiom-exempt: K1 zeroes the leading 16 bytes of the 0x14-byte record
    pPBuffer->m_nWidth = nWidth;
    pPBuffer->m_nHeight = nHeight;
    ASLgl::glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &s_nSavedFramebuffer);
    glGenFramebuffersOES(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nFramebuffer));
    ASLgl::glGenTextures(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nTexture));
    glGenRenderbuffersOES(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nDepthRenderbuffer));
    glBindFramebufferOES(GL_FRAMEBUFFER, pPBuffer->m_nFramebuffer);
    ASLgl::glBindTexture(GL_TEXTURE_2D, pPBuffer->m_nTexture);
    ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pPBuffer->m_nTexture, 0);
    glBindRenderbufferOES(GL_RENDERBUFFER, pPBuffer->m_nDepthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, nWidth, nHeight);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pPBuffer->m_nDepthRenderbuffer);
    unsigned int nStatus = glCheckFramebufferStatusOES(GL_FRAMEBUFFER);
    if (nStatus == GL_FRAMEBUFFER_COMPLETE)
        puts("fbo complete");
    else if (nStatus == GL_FRAMEBUFFER_UNSUPPORTED)
        puts("fbo unsupported");
    else
        puts("Framebuffer Error");
    glBindFramebufferOES(GL_FRAMEBUFFER, s_nSavedFramebuffer);
    return pPBuffer;
#else
    int aPixelFormatAttribs[17] = {
        WGL_SUPPORT_OPENGL_ARB, 1, WGL_DRAW_TO_PBUFFER_ARB, 1, WGL_BIND_TO_TEXTURE_RGBA_ARB, 1,
        WGL_RED_BITS_ARB, 8, WGL_GREEN_BITS_ARB, 8,
        WGL_BLUE_BITS_ARB, 8, WGL_ALPHA_BITS_ARB, 8, WGL_DOUBLE_BUFFER_ARB, 0, 0
    };
    int nPixelFormat;
    float fAttribF = 0.0f;
    unsigned int nNumFormats;
    void* hDC = m_hDC;
    wglChoosePixelFormatARB(hDC, aPixelFormatAttribs, &fAttribF, 1, &nPixelFormat, &nNumFormats);
    int aPbufferAttribs[6] = {
        WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB, WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_2D_ARB, 0, 0
    };
    return wglCreatePBufferARB(hDC, nPixelFormat, nWidth, nHeight, aPbufferAttribs);
#endif
}

// Builds the modelview: the legacy-matrix path or the axis-angle form, both ending at glTranslatef(-vPosition).
void GLRender::SetViewTransform(Vector& vPosition, Quaternion& qRotation)
{
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    if (multmatrix)
    {
        float aMatrix[16];
        Vector vZero(0.0f, 0.0f, 0.0f);
#if KOTOR1 // K1: the out-of-line unary minus
        Quaternion qNeg = -qRotation;
#else
        Quaternion qNeg(-qRotation.w, qRotation.x, qRotation.y, qRotation.z);
#endif
        MakeMatrix4x4(aMatrix, vZero, qNeg);
        ASLgl::glMultMatrixf(aMatrix);
    }
    else
    {
        float fAngle = qRotation.angle();
        Vector vAxis = qRotation.axis();
        ASLgl::glRotatef(fAngle * -57.295776f, vAxis.x, vAxis.y, vAxis.z);
    }
    ASLgl::glTranslatef(-vPosition.x, -vPosition.y, -vPosition.z);
}

void GLRender::RenderObjectsVTC(Aurora::AuroraPrimitiveTypes eType, unsigned int nPool, int nCount)
{
    if (AurVertexBufferObjectARBAvailable())
    {
        char* pPool = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pPool)));
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glEnableClientState(GL_COLOR_ARRAY);
#if !KOTOR1 || defined(K2_FIX_ENGINE_BUGS) // K1: the normal array is left as it is
        ASLgl::glDisableClientState(GL_NORMAL_ARRAY);
#endif
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVTC), static_cast<void*>(0));
        ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RenderVertexVTC), reinterpret_cast<void*>(offsetof(RenderVertexVTC, color)));
        ASLgl::glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertexVTC), reinterpret_cast<void*>(offsetof(RenderVertexVTC, u)));
        ASLgl::glDrawArrays(primTypeTranslationTable[eType], 0, nCount);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
        ASLgl::glDisableClientState(GL_COLOR_ARRAY);
    }
    else
    {
        ASLgl::glBegin(primTypeTranslationTable[eType]);
        RenderVertexVTC* pVertex = reinterpret_cast<RenderVertexVTC*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glColor4ubv(reinterpret_cast<unsigned char*>(&pVertex->color));
            ASLgl::glTexCoord2f(pVertex->u, pVertex->v);
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVTCQuadUP(RenderVertexVTC* paVertices, int nCount)
{
    if (AurVertexBufferObjectARBAvailable())
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_COLOR_ARRAY);
        ASLgl::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glDisableClientState(GL_NORMAL_ARRAY);
        ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RenderVertexVTC), &paVertices->color);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertexVTC), &paVertices->u);
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVTC), paVertices);
        ASLgl::glDrawArrays(7, 0, nCount);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
#if !KOTOR1 // K1: the colour and texcoord arrays stay enabled
        ASLgl::glDisableClientState(GL_COLOR_ARRAY);
        ASLgl::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
    }
    else
    {
        ASLgl::glBegin(7);
        RenderVertexVTC* pVertex = paVertices;
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glColor4ubv(reinterpret_cast<unsigned char*>(&pVertex->color));
            ASLgl::glTexCoord2f(pVertex->u, pVertex->v);
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVTCQuad(unsigned int nPool, int nCount)
{
    if (AurVertexBufferObjectARBAvailable())
    {
        char* pPool = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_COLOR_ARRAY);
        ASLgl::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glDisableClientState(GL_NORMAL_ARRAY);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pPool)));
        ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RenderVertexVTC), reinterpret_cast<void*>(offsetof(RenderVertexVTC, color)));
        ASLgl::glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertexVTC), reinterpret_cast<void*>(offsetof(RenderVertexVTC, u)));
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVTC), static_cast<void*>(0));
        ASLgl::glDrawArrays(7, 0, nCount);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        ASLgl::glBegin(7);
        RenderVertexVTC* pVertex = reinterpret_cast<RenderVertexVTC*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glColor4ubv(reinterpret_cast<unsigned char*>(&pVertex->color));
            ASLgl::glTexCoord2f(pVertex->u, pVertex->v);
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVTQuadUP(RenderVertexVT* paVertices, int nCount)
{
    if (AurVertexBufferObjectARBAvailable())
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glDisableClientState(GL_NORMAL_ARRAY);
        ASLgl::glDisableClientState(GL_COLOR_ARRAY);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertexVT), &paVertices->u);
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVT), paVertices);
        ASLgl::glDrawArrays(7, 0, nCount);
    }
    else
    {
        ASLgl::glBegin(7);
        RenderVertexVT* pVertex = paVertices;
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glTexCoord2f(pVertex->u, pVertex->v);
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVTQuad(unsigned int nPool, int nCount)
{
    if (AurVertexBufferObjectARBAvailable())
    {
        char* pPool = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glDisableClientState(GL_NORMAL_ARRAY);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pPool)));
        ASLgl::glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertexVT), reinterpret_cast<void*>(offsetof(RenderVertexVT, u)));
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVT), static_cast<void*>(0));
        ASLgl::glDrawArrays(7, 0, nCount);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        ASLgl::glBegin(7);
        RenderVertexVT* pVertex = reinterpret_cast<RenderVertexVT*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glTexCoord2f(pVertex->u, pVertex->v);
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVTQuadStrip(unsigned int nPool, int nCount)
{
    if (AurVertexBufferObjectARBAvailable())
    {
        char* pPool = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glDisableClientState(GL_NORMAL_ARRAY);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pPool)));
        ASLgl::glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertexVT), reinterpret_cast<void*>(offsetof(RenderVertexVT, u)));
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVT), static_cast<void*>(0));
#if KOTOR1  // K1: a triangle strip (GLES has no GL_QUAD_STRIP)
        ASLgl::glDrawArrays(5, 0, nCount);
#else
        ASLgl::glDrawArrays(8, 0, nCount);
#endif
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        ASLgl::glBegin(8);
        RenderVertexVT* pVertex = reinterpret_cast<RenderVertexVT*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glTexCoord2f(pVertex->u, pVertex->v);
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVTNQuadStrip(unsigned int nPool, int nCount)
{
    if (AurVertexBufferObjectARBAvailable())
    {
        char* pPool = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glEnableClientState(GL_NORMAL_ARRAY);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pPool)));
        ASLgl::glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertexVTN), reinterpret_cast<void*>(offsetof(RenderVertexVTN, u)));
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVTN), static_cast<void*>(0));
        ASLgl::glNormalPointer(GL_FLOAT, sizeof(RenderVertexVTN), reinterpret_cast<void*>(offsetof(RenderVertexVTN, normal)));
#if KOTOR1  // K1: a triangle strip (GLES has no GL_QUAD_STRIP)
        ASLgl::glDrawArrays(5, 0, nCount);
#else
        ASLgl::glDrawArrays(8, 0, nCount);
#endif
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        ASLgl::glBegin(8);
        RenderVertexVTN* pVertex = reinterpret_cast<RenderVertexVTN*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glTexCoord2f(pVertex->u, pVertex->v);
            ASLgl::glNormal3f(pVertex->normal[0], pVertex->normal[1], pVertex->normal[2]);
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVCUP(Aurora::AuroraPrimitiveTypes eType, RenderVertexVC* paVertices,
                                 int nCount)
{
    if (AurVertexBufferObjectARBAvailable())
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_COLOR_ARRAY);
        ASLgl::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glDisableClientState(GL_NORMAL_ARRAY);
        ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RenderVertexVC), &paVertices->color);
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVC), paVertices);
        ASLgl::glDrawArrays(primTypeTranslationTable[eType], 0, nCount);
    }
    else
    {
        ASLgl::glBegin(primTypeTranslationTable[eType]);
        RenderVertexVC* pVertex = paVertices;
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glColor4ubv(reinterpret_cast<unsigned char*>(&pVertex->color));
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVC(Aurora::AuroraPrimitiveTypes eType, unsigned int nPool, int nCount)
{
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pPool = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
    if (bVBO)
    {
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_COLOR_ARRAY);
        ASLgl::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glDisableClientState(GL_NORMAL_ARRAY);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pPool)));
        ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RenderVertexVC), reinterpret_cast<void*>(offsetof(RenderVertexVC, color)));
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVC), static_cast<void*>(0));
        ASLgl::glDrawArrays(primTypeTranslationTable[eType], 0, nCount);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        ASLgl::glBegin(primTypeTranslationTable[eType]);
        RenderVertexVC* pVertex = reinterpret_cast<RenderVertexVC*>(pPool);
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glColor4ubv(reinterpret_cast<unsigned char*>(&pVertex->color));
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVCQuads(unsigned int nPool, int nOffset, int nCount)
{
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pPool = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
    if (bVBO)
    {
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_COLOR_ARRAY);
        ASLgl::glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glDisableClientState(GL_NORMAL_ARRAY);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pPool)));
        ASLgl::glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertexVC), reinterpret_cast<void*>(offsetof(RenderVertexVC, color)));
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVC), static_cast<void*>(0));
        ASLgl::glDrawArrays(7, 0, nCount);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
        ASLgl::glDisableClientState(GL_COLOR_ARRAY);
    }
    else
    {
        ASLgl::glBegin(7);
        RenderVertexVC* pVertex = reinterpret_cast<RenderVertexVC*>(pPool) + nOffset;
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glColor4ubv(reinterpret_cast<unsigned char*>(&pVertex->color));
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

void GLRender::RenderObjectsVTN(Aurora::AuroraPrimitiveTypes eType, unsigned int nPool, int nCount)
{
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pPool = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nPool)));
    if (bVBO)
    {
        ASLgl::glEnableClientState(GL_VERTEX_ARRAY);
        ASLgl::glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        ASLgl::glEnableClientState(GL_NORMAL_ARRAY);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pPool)));
        ASLgl::glTexCoordPointer(2, GL_FLOAT, sizeof(RenderVertexVTN), reinterpret_cast<void*>(offsetof(RenderVertexVTN, u)));
        ASLgl::glNormalPointer(GL_FLOAT, sizeof(RenderVertexVTN), reinterpret_cast<void*>(offsetof(RenderVertexVTN, normal)));
        ASLgl::glVertexPointer(3, GL_FLOAT, sizeof(RenderVertexVTN), static_cast<void*>(0));
        ASLgl::glDrawArrays(7, 0, nCount);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        ASLgl::glBegin(primTypeTranslationTable[eType]);
        RenderVertexVTN* pVertex = reinterpret_cast<RenderVertexVTN*>(pPool);
        if (nCount > 0)
        for (uint64_t i = static_cast<unsigned int>(nCount); i > 0; i--)
        {
            ASLgl::glTexCoord2f(pVertex->u, pVertex->v);
            ASLgl::glNormal3f(pVertex->normal[0], pVertex->normal[1], pVertex->normal[2]);
            ASLgl::glVertex3fv(pVertex->pos);
            pVertex++;
        }
        ASLgl::glEnd();
    }
}

// An indexed draw out of the pool's index buffer.
void GLRender::RenderObjectsV(Aurora::AuroraPrimitiveTypes eType, unsigned int nCount,
                               unsigned int nOffset, unsigned int nPool)
{
    void* paIndices = manRef->GetIndexPool(nPool);
    ASLgl::glDrawElements(primTypeTranslationTable[eType], nCount, GL_UNSIGNED_SHORT,
                          static_cast<unsigned short*>(paIndices) + nOffset);
}

// Installs the NV/ARB combiners: a switch on the shader id picks the texture unit, the enable pair
// and the bind.
void GLRender::EnableCombiners(int nShaderID, CAurTexture* pTex0, CAurTexture* pTex1,
                               CAurTexture* pTex2, CAurTexture* pTex3)
{
#if KOTOR1
    K2_UNUSED(nShaderID); K2_UNUSED(pTex0); K2_UNUSED(pTex1); K2_UNUSED(pTex2); K2_UNUSED(pTex3);
#endif
#if !KOTOR1 // K1: an empty body
    switch (nShaderID)
    {
    case kPSNoiseEMSubtract:
    case kPSNoiseEMAdd:
    case kPSNoiseEMMultiply:
    case kPSDesatNoiseEMSubtract:
    case kPSDesatNoiseEMAdd:
    case kPSDesatNoiseEMMultiply:
    case kPSIllumNoiseEMAdd:
    case kPSIllumNoiseEMMultiply:
    case kPSIllumDesatNoiseEMSubtract:
    case kPSIllumDesatNoiseEMAdd:
    case kPSIllumDesatNoiseEMMultiply:
    {
        if (!pTex2)
            return;
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        pTex1->Bind();
        unsigned int nEnable0;
        unsigned int nEnable1;
        if (!pTex1->IsCubeMap())
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            nEnable0 = GL_TEXTURE_GEN_S;
            nEnable1 = GL_TEXTURE_GEN_T;
        }
        else
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            nEnable0 = GL_TEXTURE_GEN_T;
            nEnable1 = GL_TEXTURE_GEN_R;
        }
        ASLgl::glEnable(nEnable0);
        ASLgl::glEnable(nEnable1);
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }

    case kPSNoiseAdd:
    case kPSNoiseMultiply:
    case kPSDesatNoiseAdd:
    case kPSDesatNoiseMultiply:
    case kPSIllumNoiseAdd:
    case kPSIllumNoiseMultiply:
    case kPSIllumDesatNoiseAdd:
    case kPSIllumDesatNoiseMultiply:
        if (!pTex1)
            return;
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex1->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    case kPSNoiseEMLMSubtract:
    case kPSNoiseEMLMAdd:
    case kPSNoiseEMLMMultiply:
    case kPSDesatNoiseEMLMSubtract:
    case kPSDesatNoiseEMLMAdd:
    case kPSDesatNoiseEMLMMultiply:
    {
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(2);
        pTex2->Bind();
        unsigned int nEnable0;
        unsigned int nEnable1;
        if (!pTex2->IsCubeMap())
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            nEnable0 = GL_TEXTURE_GEN_S;
            nEnable1 = GL_TEXTURE_GEN_T;
        }
        else
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            nEnable0 = GL_TEXTURE_GEN_T;
            nEnable1 = GL_TEXTURE_GEN_R;
        }
        ASLgl::glEnable(nEnable0);
        ASLgl::glEnable(nEnable1);
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        GLRender::SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex3->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }

    case kPSNoiseLMSubtract:
    case kPSNoiseLMAdd:
    case kPSNoiseLMMultiply:
    case kPSDesatNoiseLMSubtract:
    case kPSDesatNoiseLMAdd:
    case kPSDesatNoiseLMMultiply:
        if (!pTex2)
            return;
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        cm_uiCurrentStage = 2;
        tex1on = 1;
        if (AurMultiTextureAvailable())
            ASLgl::glActiveTextureARB(cm_uiCurrentStage + GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    case kPSDesatEM:
    case kPSIllumEM:
    case kPSIllumDesatEM:
    case kPSIllumDesatEMLM:
    {
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        pTex1->Bind();
        unsigned int nEnable;
        if (!pTex1->IsCubeMap())
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            nEnable = GL_TEXTURE_GEN_T;
        }
        else
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            nEnable = GL_TEXTURE_GEN_R;
        }
        ASLgl::glEnable(nEnable);
        break;
    }

    case kPSNoiseSubtract:
    case kPSDesatNoiseSubtract:
    case kPSIllumNoiseSubtract:
    case kPSIllumDesatNoiseSubtract:
        if (!pTex1)
            return;
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex1->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;

    case kPSFGlowBumpmap:
    case kPSDesatBumpmap:
    case kPSIllumDesatBumpmap:
        tex1on = 1;
        ASLgl::glDisable(GL_LIGHTING);
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex1->Bind();
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex2->Bind();
        GLRender::SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex3->Bind();
        break;

    case kPSFGlowBumpyShiny:
    case kPSDesatBumpyShiny:
    case kPSIllumDesatBumpyShiny:
        MakeLookUpTexture(5.0f);
        break;

    case kPSIllumNoiseEMLMSubtract:
    case kPSIllumNoiseEMLMAdd:
    case kPSIllumNoiseEMLMMultiply:
    {
        if (!pTex2)
            return;
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        pTex2->Bind();
        unsigned int nEnable0;
        unsigned int nEnable1;
        if (!pTex1->IsCubeMap())
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            nEnable0 = GL_TEXTURE_GEN_S;
            nEnable1 = GL_TEXTURE_GEN_T;
        }
        else
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            nEnable0 = GL_TEXTURE_GEN_T;
            nEnable1 = GL_TEXTURE_GEN_R;
        }
        ASLgl::glEnable(nEnable0);
        ASLgl::glEnable(nEnable1);
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex3->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }

    case kPSIllumNoiseLMAdd:
    case kPSIllumNoiseLMMultiply:
    case kPSIllumDesatNoiseLMAdd:
        if (!pTex2)
            return;
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    case kPSIllumDesatNoiseEMLMSubtract:
    case kPSIllumDesatNoiseEMLMAdd:
    case kPSIllumDesatNoiseEMLMMultiply:
    {
        if (!pTex2)
            return;
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        pTex2->Bind();
        unsigned int nEnable0;
        unsigned int nEnable1;
        if (!pTex2->IsCubeMap())
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            nEnable0 = GL_TEXTURE_GEN_S;
            nEnable1 = GL_TEXTURE_GEN_T;
        }
        else
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            nEnable0 = GL_TEXTURE_GEN_T;
            nEnable1 = GL_TEXTURE_GEN_R;
        }
        ASLgl::glEnable(nEnable0);
        ASLgl::glEnable(nEnable1);
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex3->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }
    case kPSFGlowLM:
    case kPSDesatLM:
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        tex1on = 1;
        break;

    case kPSFGlowEMLM:
    case kPSDesatEMLM:
    {
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(2);
        pTex2->Bind();
        unsigned int nEnable0;
        unsigned int nEnable1;
        if (!pTex2->IsCubeMap())
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            nEnable0 = GL_TEXTURE_GEN_S;
            nEnable1 = GL_TEXTURE_GEN_T;
        }
        else
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            nEnable0 = GL_TEXTURE_GEN_T;
            nEnable1 = GL_TEXTURE_GEN_R;
        }
        ASLgl::glEnable(nEnable0);
        ASLgl::glEnable(nEnable1);
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex1->Bind();
        break;
    }

    case kPSHolo:
    case kPSHoloDesat:
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_2D);
        m_pHologramFilterTexture->Bind();
        break;

    case kPSIllumNoiseLMSubtract:
    case kPSIllumDesatNoiseLMSubtract:
        if (!pTex2)
            return;
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;

    case kPSNone:
        DisableCombiners(pTex0);
        ASLgl::glEnable(GL_LIGHTING);
        GLRender::SetActiveTexture(0);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        GLRender::SetActiveTexture(1);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        GLRender::SetActiveTexture(2);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        GLRender::SetActiveTexture(3);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        GLRender::SetActiveTexture(0);
        cm_nGLCurrentTID[0] = 0;
        cm_nGLCurrentTID[1] = 0;
        cm_nGLCurrentTID[2] = 0;
        cm_nGLCurrentTID[3] = 0;
        break;

    case kPSLightmapBumpSpecular2:
        tex1on = 1;
        ASLgl::glDisable(GL_LIGHTING);
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_CUBE_MAP);
        pTex1->Bind();
        GLRender::SetActiveTexture(2);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex2->Bind();
        GLRender::SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_2D);
        pTex3->Bind();
        break;

    case kPSBumpmapOff:
        GLRender::SetActiveTexture(1);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        GLRender::SetActiveTexture(2);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        GLRender::SetActiveTexture(3);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        DisableCombiners(pTex0);
        ASLgl::glEnable(GL_LIGHTING);
        GLRender::SetActiveTexture(0);
        break;

    case kPSMultiTextureOff:
        if (AurNV2xLevelShadersAvailable())
        {
            GLRender::SetActiveTexture(3);
            ASLgl::glDisable(GL_TEXTURE_3D);
            ASLgl::glBindTexture(GL_TEXTURE_3D, 0);
            cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
            ASLgl::glDisable(GL_TEXTURE_2D);
            GLRender::SetActiveTexture(2);
            ASLgl::glDisable(GL_TEXTURE_3D);
            ASLgl::glBindTexture(GL_TEXTURE_3D, 0);
            cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
            ASLgl::glDisable(GL_TEXTURE_2D);
            if (envmapon)
                ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        }
        GLRender::SetActiveTexture(1);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glDisable(GL_TEXTURE_3D);
        ASLgl::glBindTexture(GL_TEXTURE_3D, 0);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        if (envmapon)
        {
            ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
            ASLgl::glDisable(GL_TEXTURE_GEN_S);
            ASLgl::glDisable(GL_TEXTURE_GEN_T);
            ASLgl::glDisable(GL_TEXTURE_GEN_R);
            envmapon = 0;
        }
        GLRender::SetActiveTexture(0);
        DisableCombiners(pTex0);
        break;

    case kPSEnvMapLightmapOff:
        GLRender::SetActiveTexture(1);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        GLRender::SetActiveTexture(2);
        ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
        ASLgl::glDisable(GL_TEXTURE_2D);
        cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
        ASLgl::glDisable(GL_TEXTURE_GEN_S);
        ASLgl::glDisable(GL_TEXTURE_GEN_T);
        ASLgl::glDisable(GL_TEXTURE_GEN_R);
        DisableCombiners(pTex0);
        GLRender::SetActiveTexture(0);
        break;

    case kPSFGlow:
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        break;

    case kPSFGlowEM:
    {
        envmapon = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        pTex1->Bind();
        unsigned int nEnable;
        if (!pTex1->IsCubeMap())
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            nEnable = GL_TEXTURE_GEN_T;
        }
        else
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            ASLgl::glEnable(GL_TEXTURE_GEN_T);
            nEnable = GL_TEXTURE_GEN_R;
        }
        ASLgl::glEnable(nEnable);
        break;
    }

    case kPSDesatOff:
        ASLgl::glEnable(GL_LIGHTING);
        break;

    case kPSHoloOff:
        DisableCombiners(pTex0);
        GLRender::SetActiveTexture(1);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        GLRender::SetActiveTexture(0);
        cm_nGLCurrentTID[0] = 0;
        cm_nGLCurrentTID[1] = 0;
        break;

    case kPSIllumNoiseEMSubtract:
    {
        if (!pTex2)
            return;
        envmapon = 1;
        tex1on = 1;
        GLRender::SetActiveTexture(0);
        pTex0->Bind();
        GLRender::SetActiveTexture(1);
        pTex1->Bind();
        unsigned int nEnable0;
        unsigned int nEnable1;
        if (!pTex1->IsCubeMap())
        {
            ASLgl::glEnable(GL_TEXTURE_2D);
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            nEnable0 = GL_TEXTURE_GEN_S;
            nEnable1 = GL_TEXTURE_GEN_T;
        }
        else
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glEnable(GL_TEXTURE_GEN_S);
            nEnable0 = GL_TEXTURE_GEN_T;
            nEnable1 = GL_TEXTURE_GEN_R;
        }
        ASLgl::glEnable(nEnable0);
        ASLgl::glEnable(nEnable1);
        GLRender::SetActiveTexture(3);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
    }

    case kPSIllumDesatNoiseLMMultiply:
        if (!pTex1)
            return;
        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        GLRender::SetActiveTexture(1);
        ASLgl::glEnable(GL_TEXTURE_3D);
        pTex2->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;

    }
#endif
}

// The texture argument is unused; the body is pure GL state teardown.
inline void GLRender::DisableCombiners(CAurTexture* pTexture)
{
    K2_UNUSED(pTexture);
    GLRender::SetActiveTexture(1);
    ASLgl::glDisable(GL_TEXTURE_CUBE_MAP);
    ASLgl::glDisable(GL_TEXTURE_GEN_S);
    ASLgl::glDisable(GL_TEXTURE_GEN_T);
    ASLgl::glDisable(GL_TEXTURE_GEN_R);
    ASLgl::glDisable(GL_TEXTURE_2D);

    envmapon = 0;
    cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
    tex1on = 0;
    GLRender::SetActiveTexture(0);
    ASLgl::glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
}

// Four separate pools, one per interleaved array.
void GLRender::RenderObjectsVNTCQuad(unsigned int nVertexPool, unsigned int nNormalPool,
                                     unsigned int nTexCoordPool, unsigned int nColorPool, int nCount)
{
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pVertex = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nVertexPool)));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pVertex)));
        pVertex = static_cast<char*>(0);
    }
    ASLgl::glVertexPointer(3, GL_FLOAT, 12, pVertex);

    bVBO = AurVertexBufferObjectARBAvailable();
    char* pNormal = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nNormalPool)));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pNormal)));
        pNormal = static_cast<char*>(0);
    }
    ASLgl::glNormalPointer(GL_FLOAT, 12, pNormal);

    bVBO = AurVertexBufferObjectARBAvailable();
    char* pTexCoord = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nTexCoordPool)));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pTexCoord)));
        pTexCoord = static_cast<char*>(0);
    }
    ASLgl::glTexCoordPointer(2, GL_FLOAT, 8, pTexCoord);

    bVBO = AurVertexBufferObjectARBAvailable();
    char* pColor = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(manRef->GetPool(nColorPool)));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pColor)));
        pColor = static_cast<char*>(0);
    }
    ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, 4, pColor);

    ASLgl::glDrawArrays(7, 0, nCount);
}

// Probes the capabilities once, walking glGetString(GL_EXTENSIONS) for every known extension.
void GLRender::InitExtensions()
{
#if KOTOR1 // K1: a fixed GLES capability set; no extension-string probe
#if defined(K2_UI_TOUCH) && !defined(K2_FIX_ENGINE_BUGS)
    ASLgl::glGetString(GL_EXTENSIONS /* GL_EXTENSIONS */);
#else
    // The Advanced Graphics page offers anisotropic filtering (the touch page binds the row too).
    const unsigned char* pszExtensions = ASLgl::glGetString(GL_EXTENSIONS /* GL_EXTENSIONS */);
    if (pszExtensions != 0
        && CheckExtension(const_cast<char*>("GL_EXT_texture_filter_anisotropic"), pszExtensions))
        existingextensions |= EXT_TEXTURE_FILTER_ANISOTROPIC_BIT;
#endif

    existingextensions |= ARB_MULTITEXTURE_BIT;
    existingextensions |= EXT_BGRA_BIT;
    existingextensions |= EXT_TEXTURE_CUBE_MAP_BIT;
    existingextensions |= GL_ARB_VERTEX_PROGRAM_BIT;
    existingextensions |= GL_ARB_FRAGMENT_PROGRAM_BIT;
    existingextensions |= GL_ARB_DEPTH_TEXTURE_BIT;
    existingextensions |= ATI_FRAGMENT_SHADER_BIT;
    existingextensions |= NV_REGISTER_COMBINERS_BIT;
    existingextensions |= NV_PIXEL_SHADERS_BIT;
    existingextensions |= WGL_ARB_RENDER_TO_TEXTURE_BIT;
    existingextensions |= GL_ARB_VERTEX_BUFFER_OBJECT_BIT;

    // The GeForce3 driver writes VBOs in a way this renderer cannot consume: turn the port's
    // "write-only" shortcut off for it.
    if (existingextensions & NV_VERTEX_PROGRAMS_BIT)
    {
        if (strstr(reinterpret_cast<const char*>(ASLgl::glGetString(GL_RENDERER /* GL_RENDERER */)), "GeForce3") != 0)
            g_bVertexBufferObjectsWriteOnly = false;
    }

    if (AurAnisotropicFilterAvailable())
        ASLgl::glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT /* GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT */,
                           &largest_supported_anisotropy);

    InitAttenuationTexture();

// ATI fragment-shader drivers get the fragment-program back end, the rest the combiner one.
    if ((existingextensions & ATI_FRAGMENT_SHADER_BIT) == ATI_FRAGMENT_SHADER_BIT)
    {
        pfuncEnablePixelShaders = reinterpret_cast<void (*)(int, void*, void*, void*, void*)>(EnableFragmentShaders);
        pfuncSetPixelShaderAmbientColor = SetFragmentShaderConstant0;
        pfuncSetBumpLightColor = SetFragmentShaderConstant1;

        szBumpyShinyVPARB = new char[strlen(szBumpyShinySetupVPARB)
                                     + strlen(szBumpyShinyVPARB_ATI) + 1];
        szBumpyShinySkinVPARB = new char[strlen(szBumpyShinySkinSetupVPARB)
                                         + strlen(szBumpyShinyVPARB_ATI) + 1];
        strcpy(szBumpyShinyVPARB, szBumpyShinySetupVPARB);
        strcat(szBumpyShinyVPARB, szBumpyShinyVPARB_ATI);
        strcpy(szBumpyShinySkinVPARB, szBumpyShinySkinSetupVPARB);
        strcat(szBumpyShinySkinVPARB, szBumpyShinyVPARB_ATI);

        InitializeATIFragmentShaders();
    }
    else
    {
        pfuncEnablePixelShaders = reinterpret_cast<void (*)(int, void*, void*, void*, void*)>(EnableCombiners);
        pfuncSetPixelShaderAmbientColor = SetCombinerConstant0;
        pfuncSetBumpLightColor = SetCombinerConstant1;

        szBumpyShinyVPARB = new char[strlen(szBumpyShinySetupVPARB)
                                     + strlen(szBumpyShinyVPARB_NV) + 1];
        szBumpyShinySkinVPARB = new char[strlen(szBumpyShinySkinSetupVPARB)
                                         + strlen(szBumpyShinyVPARB_NV) + 1];
        strcpy(szBumpyShinyVPARB, szBumpyShinySetupVPARB);
        strcat(szBumpyShinyVPARB, szBumpyShinyVPARB_NV);
        strcpy(szBumpyShinySkinVPARB, szBumpyShinySkinSetupVPARB);
        strcat(szBumpyShinySkinVPARB, szBumpyShinyVPARB_NV);
    }

    if (AurVertexBufferObjectARBAvailable())
        InitializeVertexBufferObjectPoolManager();
    else
        InitializeSystemPoolManager();

    manRef->RestorePoolManager();
#else
    const unsigned char* pszExtensions = ASLgl::glGetString(GL_EXTENSIONS /* GL_EXTENSIONS */);

    if (CheckExtension(const_cast<char*>("GL_EXT_stencil_two_side"), pszExtensions))
        existingextensions |= GL_STENCIL_TWO_SIDED_BIT;
    if (CheckExtension(const_cast<char*>("GL_EXT_bgra"), pszExtensions))
        existingextensions |= EXT_BGRA_BIT;
    if (CheckExtension(const_cast<char*>("GL_ARB_multitexture"), pszExtensions))
        existingextensions |= ARB_MULTITEXTURE_BIT;
    if (CheckExtension(const_cast<char*>("GL_ARB_texture_env_combine"), pszExtensions))
        existingextensions |= EXT_TEXTURE_ENV_COMBINE_BIT;
    if (CheckExtension(const_cast<char*>("GL_NV_register_combiners"), pszExtensions))
        existingextensions |= NV_REGISTER_COMBINERS_BIT;
    if (CheckExtension(const_cast<char*>("GL_NV_register_combiners2"), pszExtensions))
        existingextensions |= NV_REGISTER_COMBINERS2_BIT;
    if (CheckExtension(const_cast<char*>("GL_ARB_texture_cube_map"), pszExtensions))
        existingextensions |= EXT_TEXTURE_CUBE_MAP_BIT;
    if (CheckExtension(const_cast<char*>("GL_EXT_fog_coord"), pszExtensions))
        existingextensions |= EXT_FOG_COORD_BIT;
    if (CheckExtension(const_cast<char*>("GL_EXT_secondary_color"), pszExtensions))
        existingextensions |= EXT_SECONDARY_COLOR_BIT;
    if (CheckExtension(const_cast<char*>("GL_NV_vertex_program"), pszExtensions))
        existingextensions |= NV_VERTEX_PROGRAMS_BIT;
    if (CheckExtension(const_cast<char*>("GL_EXT_texture_filter_anisotropic"), pszExtensions))
        existingextensions |= EXT_TEXTURE_FILTER_ANISOTROPIC_BIT;
    if (CheckExtension(const_cast<char*>("GL_NV_texture_shader"), pszExtensions))
        existingextensions |= NV_PIXEL_SHADERS_BIT;

    if (ASL::GetPrefAsBool("VAREnable", false))
    {
        if (CheckExtension(const_cast<char*>("GL_APPLE_vertex_array_range"), pszExtensions))
            existingextensions |= NV_VERTEX_ARRAY_RANGE_BIT;
        if (CheckExtension(const_cast<char*>("GL_APPLE_vertex_array_object"), pszExtensions))
            existingextensions |= NV_VERTEX_ARRAY_RANGE2_BIT;
        if (CheckExtension(const_cast<char*>("GL_APPLE_fence"), pszExtensions))
            existingextensions |= NV_FENCE_BIT;
    }

    if (CheckExtension(const_cast<char*>("GL_ARB_texture_compression"), pszExtensions))
        existingextensions |= ARB_TEXTURE_COMPRESSION_BIT;
    if (CheckExtension(const_cast<char*>("GL_EXT_texture_compression_s3tc"), pszExtensions))
        existingextensions |= EXT_TEXTURE_COMPRESSION_S3TC_BIT;
    if (CheckExtension(const_cast<char*>("GL_ATI_pn_triangles"), pszExtensions))
        existingextensions |= ATIX_PN_TRIANGLES_BIT;
    if (CheckExtension(const_cast<char*>("GL_EXT_compiled_vertex_array"), pszExtensions))
        existingextensions |= EXT_COMPILED_VERTEX_ARRAY_BIT;
    if (CheckExtension(const_cast<char*>("GL_ARB_framebuffer_object"), pszExtensions))
        existingextensions |= WGL_ARB_RENDER_TO_TEXTURE_BIT;
    if (CheckExtension(const_cast<char*>("GL_SGIS_generate_mipmap"), pszExtensions))
        existingextensions |= EXT_SGIS_MIPMAP_BIT;
    if (CheckExtension(const_cast<char*>("GL_ATI_text_fragment_shader"), pszExtensions))
        existingextensions |= ATI_FRAGMENT_SHADER_BIT;
    if (CheckExtension(const_cast<char*>("GL_ARB_vertex_program"), pszExtensions))
        existingextensions |= GL_ARB_VERTEX_PROGRAM_BIT;
    if (CheckExtension(const_cast<char*>("GL_ARB_fragment_program"), pszExtensions))
    {
        existingextensions |= GL_ARB_FRAGMENT_PROGRAM_BIT;
        existingextensions |= ATI_FRAGMENT_SHADER_BIT;
    }
    if (CheckExtension(const_cast<char*>("GL_ARB_depth_texture"), pszExtensions))
        existingextensions |= GL_ARB_DEPTH_TEXTURE_BIT;
    if (CheckExtension(const_cast<char*>("GL_ARB_vertex_buffer_object"), pszExtensions)
        && !ASL::GetPrefAsBool("VBODisable", false))
        existingextensions |= GL_ARB_VERTEX_BUFFER_OBJECT_BIT;

    if (CheckExtension(const_cast<char*>("GL_EXT_texture_rectangle"), pszExtensions))
        existingextensions |= WGL_NV_RENDER_TO_TEXTURE_RECTANGLE_BIT;
    if (CheckExtension(const_cast<char*>("GL_ARB_multisample"), pszExtensions))
        existingextensions |= WGL_ARB_MULTISAMPLE_BIT;

    if (CheckExtension(const_cast<char*>("WGL_NV_render_texture_rectangle"), reinterpret_cast<const unsigned char*>("WGL_EXT_swap_control")))
        existingextensions |= WGL_NV_RENDER_TO_TEXTURE_RECTANGLE_BIT;
    if (CheckExtension(const_cast<char*>("WGL_ARB_buffer_region"), reinterpret_cast<const unsigned char*>("WGL_EXT_swap_control")))
        existingextensions |= WGL_ARB_BUFFER_REGION_BIT;
    if (CheckExtension(const_cast<char*>("WGL_EXT_swap_control"), reinterpret_cast<const unsigned char*>("WGL_EXT_swap_control")))
        existingextensions |= EXT_SWAP_CONTROL_BIT;

    // The GeForce3 driver writes VBOs in a way this renderer cannot consume: turn the port's
    // "write-only" shortcut off for it.
    if (existingextensions & NV_VERTEX_PROGRAMS_BIT)
    {
        if (strstr(reinterpret_cast<const char*>(ASLgl::glGetString(GL_RENDERER /* GL_RENDERER */)), "GeForce3") != 0)
            g_bVertexBufferObjectsWriteOnly = false;
    }

    if (AurAnisotropicFilterAvailable())
        ASLgl::glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT /* GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT */,
                           &largest_supported_anisotropy);

    InitAttenuationTexture();

// Of the pixel-shader dispatch pair, ATI gets the fragment-program back end, the rest the combiner
// one.
    if ((existingextensions & ATI_FRAGMENT_SHADER_BIT) == ATI_FRAGMENT_SHADER_BIT)
    {
        pfuncEnablePixelShaders = reinterpret_cast<void (*)(int, void*, void*, void*, void*)>(EnableFragmentShaders);
        pfuncSetPixelShaderAmbientColor = SetFragmentShaderConstant0;
        pfuncSetBumpLightColor = SetFragmentShaderConstant1;

        szBumpyShinyVPARB = new char[strlen(szBumpyShinySetupVPARB)
                                     + strlen(szBumpyShinyVPARB_ATI) + 1];
        szBumpyShinySkinVPARB = new char[strlen(szBumpyShinySkinSetupVPARB)
                                         + strlen(szBumpyShinyVPARB_ATI) + 1];
        strcpy(szBumpyShinyVPARB, szBumpyShinySetupVPARB);
        strcat(szBumpyShinyVPARB, szBumpyShinyVPARB_ATI);
        strcpy(szBumpyShinySkinVPARB, szBumpyShinySkinSetupVPARB);
        strcat(szBumpyShinySkinVPARB, szBumpyShinyVPARB_ATI);

        InitializeATIFragmentShaders();
    }
    else
    {
        pfuncEnablePixelShaders = reinterpret_cast<void (*)(int, void*, void*, void*, void*)>(EnableCombiners);
        pfuncSetPixelShaderAmbientColor = SetCombinerConstant0;
        pfuncSetBumpLightColor = SetCombinerConstant1;

        szBumpyShinyVPARB = new char[strlen(szBumpyShinySetupVPARB)
                                     + strlen(szBumpyShinyVPARB_NV) + 1];
        szBumpyShinySkinVPARB = new char[strlen(szBumpyShinySkinSetupVPARB)
                                         + strlen(szBumpyShinyVPARB_NV) + 1];
        strcpy(szBumpyShinyVPARB, szBumpyShinySetupVPARB);
        strcat(szBumpyShinyVPARB, szBumpyShinyVPARB_NV);
        strcpy(szBumpyShinySkinVPARB, szBumpyShinySkinSetupVPARB);
        strcat(szBumpyShinySkinVPARB, szBumpyShinyVPARB_NV);
    }

    if (usearbvertexprograms)
        enableTranslationTable[Aurora::kCapVertexProgram] = GL_VERTEX_PROGRAM_ARB;

    if (AurVertexBufferObjectARBAvailable())
        InitializeVertexBufferObjectPoolManager();
    else
        InitializeSystemPoolManager();

    manRef->RestorePoolManager();
#endif
}

// Builds the 64x64 attenuation ramp, a radial falloff with the border rows and columns forced to 0.
void GLRender::InitAttenuationTexture()
{
    unsigned char anAttenuation[64][64][4];
    for (int nY = 0; nY < 64; nY++)
    {
        for (int nX = 0; nX < 64; nX++)
        {
            float fX = static_cast<float>(nX - 32) * 0.125f;
            float fY = static_cast<float>(nY - 32) * 0.125f;
#if KOTOR1
            unsigned char nValue = static_cast<unsigned char>((1.0f / ((fX * fX + fY * fY) * 4.5f + 1.0f)) * 255.0f);
            if (nX == 0 || nX == 63 || nY == 0 || nY == 63)
                nValue = 0;
#else
            unsigned char nValue =
                (nX == 0 || nX == 63 || nY == 0 || nY == 63)
                    ? 0
                    : static_cast<unsigned char>((1.0f / ((fX * fX + fY * fY) * 4.5f + 1.0f)) * 255.0f);
#endif
            anAttenuation[nY][nX][0] = nValue;
            anAttenuation[nY][nX][1] = nValue;
            anAttenuation[nY][nX][2] = nValue;
            anAttenuation[nY][nX][3] = 255;
        }
    }

#if KOTOR1 // K1: the GL error state is cleared around the id; no anisotropy before the bind is recorded
    ASLgl::glGetError();
    ASLgl::glGenTextures(1, &cm_tidAttenuationTexture);
    ASLgl::glGetError();
    BindTexture(Aurora::kCapTexture2D, cm_tidAttenuationTexture);
#else
    ASLgl::glGenTextures(1, &cm_tidAttenuationTexture);
    BindTexture(Aurora::kCapTexture2D, cm_tidAttenuationTexture);
#endif
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, anAttenuation);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cm_nAnisotropy);
}

// The one-time GL context latch.
void GLRender::InitRenderSystem()
{
#if KOTOR1 // K1: no WGL context latch
    cm_nGLCurrentTID[0] = 0;
    cm_nGLCurrentTID[1] = 0;
    cm_nGLCurrentTID[2] = 0;
    cm_nGLCurrentTID[3] = 0;
    cm_nGLCurrentTID[4] = 0;
    cm_nGLCurrentTID[5] = 0;
    cm_uiCurrentStage = 0;
#else
    m_hGLRC = wglGetCurrentContext();
    m_hDC = wglGetCurrentDC();
    cm_nGLCurrentTID[2] = 0;
    cm_nGLCurrentTID[3] = 0;
    cm_nGLCurrentTID[4] = 0;
    cm_nGLCurrentTID[5] = 0;
    cm_nGLCurrentTID[0] = 0;
    cm_nGLCurrentTID[1] = 0;
    cm_uiCurrentStage = 0;
#endif
}

// Tears everything down: the pool backup, the soft-shadow contexts, then every texture, program and
// pbuffer.
void GLRender::Shutdown(int nFlag)
{
#if KOTOR1 // K1: no pool backup, no GL object deletes beyond the textures, no FBOs or ARB programs
    if (cm_hSoftShadowHDC != 0)
    {
        AurATISoftShadowsAvailable();
        cm_hSoftShadowHDC = 0;
        cm_hSoftShadowHGLRC = 0;
        cm_hSoftShadowPBuffer = 0;
        cm_nSoftShadowRenderTexture = 0;
        cm_hSoftShadowTargetHDC = 0;
        cm_hSoftShadowTargetHGLRC = 0;
        cm_hSoftShadowTargetPBuffer = 0;
        cm_nSoftShadowTargetRenderTexture = 0;
        ASLgl::glDeleteTextures(1, &cm_nBufferRegionTexture);
        cm_nBufferRegionTexture = 0;
    }

    if (cm_nTargetTexture != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nTargetTexture);
        cm_nTargetTexture = 0;
    }
    if (cm_nRedTexture != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nRedTexture);
        cm_nRedTexture = 0;
    }
    if (cm_nGreenTexture != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nGreenTexture);
        cm_nGreenTexture = 0;
    }
    if (cm_nBlueTexture != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nBlueTexture);
        cm_nBlueTexture = 0;
    }
    if (cm_nOverbrightenRed != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nOverbrightenRed);
        cm_nOverbrightenRed = 0;
    }
    if (cm_nOverbrightenGreen != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nOverbrightenGreen);
        cm_nOverbrightenGreen = 0;
    }
    if (cm_nOverbrightenBlue != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nOverbrightenBlue);
        cm_nOverbrightenBlue = 0;
    }

    if (cm_bPBuffersAllocated)
    {
        int bFrameBufferModsATI = AurATIFrameBufferModificationsAvailable();
        for (int i = 0; i < 6; i++)
        {
            cm_hConvolutionHDC[i] = 0;
            cm_hConvolutionHGLRC[i] = 0;
            cm_nConvolutionRenderTexture[i] = 0;
            cm_hConvolutionPBuffer[i] = 0;
        }
        if (bFrameBufferModsATI != 0 || cm_hAccumulationPBuffer != 0)
        {
            cm_hAccumulationHDC = 0;
            cm_hAccumulationHGLRC = 0;
            cm_hAccumulationPBuffer = 0;
            cm_nAccumulationBlur = 0;
        }
        cm_bPBuffersAllocated = false;
    }

    if (cm_pHeatDistortionVertexProg != 0)
        cm_pHeatDistortionVertexProg = 0;

    ReleaseDC(g_hRenderWnd, m_hDC);

    if (nFlag != 0)
    {
        delete manRef;
        manRef = 0;
    }
#else
    if (manRef)
        manRef->BackUpPoolManager();

    if (cm_hSoftShadowHDC != 0)
    {
        if (AurATISoftShadowsAvailable())
        {
            wglMakeContextCurrentARB(cm_hSoftShadowHDC, cm_hSoftShadowHDC, cm_hSoftShadowHGLRC);
            ASLgl::glDeleteTextures(1, &cm_nSoftShadowRenderTexture);
            wglMakeContextCurrentARB(m_hDC, m_hDC, m_hGLRC);
            wglReleasePBufferDCARB(cm_hSoftShadowPBuffer, cm_hSoftShadowHDC);
            wglDestroyPBufferARB(cm_hSoftShadowPBuffer);
            cm_hSoftShadowHDC = 0;
            cm_hSoftShadowHGLRC = 0;
            cm_hSoftShadowPBuffer = 0;
            cm_nSoftShadowRenderTexture = 0;
            wglMakeContextCurrentARB(cm_hSoftShadowTargetHDC, cm_hSoftShadowTargetHDC,
                                     cm_hSoftShadowTargetHGLRC);
            ASLgl::glDeleteTextures(1, &cm_nSoftShadowTargetRenderTexture);
            wglMakeContextCurrentARB(m_hDC, m_hDC, m_hGLRC);
        }
        else
        {
            wglMakeCurrent(cm_hSoftShadowHDC, cm_hSoftShadowHGLRC);
            ASLgl::glDeleteTextures(1, &cm_nSoftShadowRenderTexture);
            wglMakeCurrent(m_hDC, m_hGLRC);
            wglDeleteContext(cm_hSoftShadowHGLRC);
            wglReleasePBufferDCARB(cm_hSoftShadowPBuffer, cm_hSoftShadowHDC);
            wglDestroyPBufferARB(cm_hSoftShadowPBuffer);
            cm_hSoftShadowHDC = 0;
            cm_hSoftShadowHGLRC = 0;
            cm_hSoftShadowPBuffer = 0;
            cm_nSoftShadowRenderTexture = 0;
            wglMakeCurrent(cm_hSoftShadowTargetHDC, cm_hSoftShadowTargetHGLRC);
            ASLgl::glDeleteTextures(1, &cm_nSoftShadowTargetRenderTexture);
            wglMakeCurrent(m_hDC, m_hGLRC);
            wglDeleteContext(cm_hSoftShadowTargetHGLRC);
        }

        wglReleasePBufferDCARB(cm_hSoftShadowTargetPBuffer, cm_hSoftShadowTargetHDC);
        wglDestroyPBufferARB(cm_hSoftShadowTargetPBuffer);
        cm_hSoftShadowTargetHDC = 0;
        cm_hSoftShadowTargetHGLRC = 0;
        cm_hSoftShadowTargetPBuffer = 0;
        cm_nSoftShadowTargetRenderTexture = 0;
        ASLgl::glDeleteTextures(1, &cm_nBufferRegionTexture);
        cm_nBufferRegionTexture = 0;
    }

    if (s_ShadowKnockDownFBO != 0)
    {
        glDeleteFramebuffers(1, &s_ShadowKnockDownFBO);
        s_ShadowKnockDownFBO = 0;
    }
    if (s_ShadowBlurFBO != 0)
    {
        glDeleteFramebuffers(1, &s_ShadowBlurFBO);
        s_ShadowBlurFBO = 0;
    }
    if (cm_nBufferRegionTexture != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nBufferRegionTexture);
        cm_nBufferRegionTexture = 0;
    }
    if (cm_nTargetTexture != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nTargetTexture);
        cm_nTargetTexture = 0;
    }
    if (cm_nRedTexture != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nRedTexture);
        cm_nRedTexture = 0;
    }
    if (cm_nGreenTexture != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nGreenTexture);
        cm_nGreenTexture = 0;
    }
    if (cm_nBlueTexture != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nBlueTexture);
        cm_nBlueTexture = 0;
    }
    if (cm_nOverbrightenRed != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nOverbrightenRed);
        cm_nOverbrightenRed = 0;
    }
    if (cm_nOverbrightenGreen != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nOverbrightenGreen);
        cm_nOverbrightenGreen = 0;
    }
    if (cm_nOverbrightenBlue != 0)
    {
        ASLgl::glDeleteTextures(1, &cm_nOverbrightenBlue);
        cm_nOverbrightenBlue = 0;
    }
    if (cm_nPerPixelFlareShader != 0)
    {
        ASLgl::glDeleteProgramsARB(1, &cm_nPerPixelFlareShader);
        cm_nPerPixelFlareShader = 0;
    }

    if (cm_bPBuffersAllocated)
    {
        int bFrameBufferModsATI = AurATIFrameBufferModificationsAvailable();
        for (int i = 0; i < 6; i++)
        {
            ASLgl::glDeleteTextures(1, &cm_nConvolutionRenderTexture[i]);
            cm_hConvolutionHDC[i] = 0;
            cm_hConvolutionHGLRC[i] = 0;
            cm_nConvolutionRenderTexture[i] = 0;
            cm_hConvolutionPBuffer[i] = 0;
        }
        if (bFrameBufferModsATI != 0 || cm_hAccumulationPBuffer != 0)
        {
            ASLgl::glDeleteTextures(1, reinterpret_cast<unsigned int*>(&cm_nAccumulationBlur));
            cm_hAccumulationHDC = 0;
            cm_hAccumulationHGLRC = 0;
            cm_hAccumulationPBuffer = 0;
            cm_nAccumulationBlur = 0;
        }
        cm_bPBuffersAllocated = false;
    }

    if (s_ConvolutionFBOs[0] != 0)
    {
        glDeleteFramebuffers(6, s_ConvolutionFBOs);
        s_ConvolutionFBOs[0] = 0;
        s_ConvolutionFBOs[1] = 0;
        s_ConvolutionFBOs[2] = 0;
        s_ConvolutionFBOs[3] = 0;
        s_ConvolutionFBOs[4] = 0;
        s_ConvolutionFBOs[5] = 0;
    }
    if (s_AccumulationFBO != 0)
    {
        glDeleteFramebuffers(1, &s_AccumulationFBO);
        s_AccumulationFBO = 0;
    }
    if (s_ConvolutionRbs[0] != 0)
    {
        glDeleteFramebuffers(6, s_ConvolutionRbs);
        s_ConvolutionRbs[0] = 0;
    }
    if (s_AccumulationRb != 0)
    {
        ASLgl::glDeleteRenderbuffersEXT(1, &s_AccumulationRb);
        s_AccumulationRb = 0;
    }
    if (cm_pHeatDistortionVertexProg != 0)
    {
        ASLgl::glDeleteProgramsARB(1, &cm_pHeatDistortionVertexProg);
        cm_pHeatDistortionVertexProg = 0;
    }

    if (cm_nPerPixelFlareShaderARB != 0)
    {
        ASLgl::glDeleteProgramsARB(1, &cm_nPerPixelFlareShaderARB);
        cm_nPerPixelFlareShaderARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_nDecalShaderARB);
        cm_nDecalShaderARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_nDecalShaderRectARB);
        cm_nDecalShaderRectARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_n4DecalShaderARB);
        cm_n4DecalShaderARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_nSaturationShaderARB);
        cm_nSaturationShaderARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_nAccumulationBlurShaderARB);
        cm_nAccumulationBlurShaderARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_nFilmNoiseShaderARB);
        cm_nFilmNoiseShaderARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_nForceDistortionShaderARB);
        cm_nForceDistortionShaderARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_nSoftShadowShaderARB);
        cm_nSoftShadowShaderARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_nOverbrightenShaderARB);
        cm_nOverbrightenShaderARB = 0;
        ASLgl::glDeleteProgramsARB(1, &cm_nDreamDistortShaderARB);
        cm_nDreamDistortShaderARB = 0;
    }

    ReleaseDC(g_hRenderWnd, m_hDC);

    if (nFlag != 0)
    {
        delete manRef;
        manRef = 0;
    }
#endif
}

// The full-screen pause tint, drawn under an orthographic projection.
void GLRender::RenderPausePlane()
{
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glEnable(GL_BLEND);
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();

    ASLgl::glBegin(7);
    ASLgl::glColor4ubv(reinterpret_cast<unsigned char*>(&g_rgbaPauseOverlayColor));
    ASLgl::glVertex2f(-1.0f, 1.0f);
    ASLgl::glVertex2f(-1.0f, -1.0f);
    ASLgl::glVertex2f(1.0f, -1.0f);
    ASLgl::glVertex2f(1.0f, 1.0f);
    ASLgl::glEnd();

    ASLgl::glEnable(GL_DEPTH_TEST);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glEnable(GL_LIGHTING);
}

// Draws the overbrighten composite as a full-screen quad built from screen-size globals only.
void GLRender::RenderOverlayOverbrighten(unsigned int nTexture)
{
    K2_UNUSED(nTexture);
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex4f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f, 1.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);

    ASLgl::glEnd();
#if !KOTOR1 // K1: the texture stays bound
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
#endif
}


void GLRender::FrameBufferModificationsATI2(uint64_t nFlags)
{
#if KOTOR1 // K1: the depth mask is always written
    if (AuroraUseARBShaders())
    {
        FrameBufferModificationsARB2(nFlags);
        return;
    }

    if (!(nFlags & kFBPassSaturation) || !visualizepass4 || !m_bSaturationEnabled)
        return;

    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glDepthMask(0);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    if (visualizepass4 && m_bSaturationEnabled)
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        RenderSaturationAdjustmentATI(cm_nTargetTexture);
    }

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapLighting);
    ASLgl::glDepthMask(1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#else
    if (AuroraUseARBShaders())
    {
        FrameBufferModificationsARB2(nFlags);
        return;
    }

    if (!(nFlags & kFBPassSaturation) || !visualizepass4 || !m_bSaturationEnabled)
        return;

    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    RenderInterface::SetDepthMask(0);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    if (visualizepass4 && m_bSaturationEnabled)
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        RenderSaturationAdjustmentATI(cm_nTargetTexture);
    }

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapLighting);
    RenderInterface::SetDepthMask(1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#endif
}

void GLRender::FrameBufferModificationsARB2(uint64_t nFlags)
{
#if KOTOR1
    K2_UNUSED(nFlags);
#endif
#if !KOTOR1 // K1: an empty body
    if (!(nFlags & kFBPassSaturation) || !visualizepass4 || !m_bSaturationEnabled)
        return;

    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    RenderInterface::SetDepthMask(0);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    if (visualizepass4 && m_bSaturationEnabled)
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        RenderSaturationAdjustmentARB(cm_nTargetTexture);
    }

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapLighting);
    RenderInterface::SetDepthMask(1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#endif
}

// The generic fixed-function driver of the saturation cluster, and the cluster's entry point.
void GLRender::FrameBufferModifications2(uint64_t nFlags)
{
#if KOTOR1 // K1: GL_TEXTURE_2D target; the depth mask is always written
    if (!doframebuffer || !cm_bPBuffersAllocated || !cm_bSetUpFrameBuffer)
        return;

    if (!AurFrameBufferModificationsAvailable())
    {
        if (AurATIFrameBufferModificationsAvailable())
            FrameBufferModificationsATI2(nFlags);
        return;
    }

    if (!(nFlags & kFBPassSaturation) || !visualizepass4 || !m_bSaturationEnabled)
        return;

    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glDepthMask(0);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    if (visualizepass4 && m_bSaturationEnabled)
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        RenderSaturationAdjustment(cm_nTargetTexture);
    }

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapLighting);
    ASLgl::glDepthMask(1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#else
    if (!doframebuffer || !cm_bPBuffersAllocated || !cm_bSetUpFrameBuffer)
        return;

    if (!AurFrameBufferModificationsAvailable())
    {
        if (AurATIFrameBufferModificationsAvailable())
            FrameBufferModificationsATI2(nFlags);
        return;
    }

    if (!(nFlags & kFBPassSaturation) || !visualizepass4 || !m_bSaturationEnabled)
        return;

    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    RenderInterface::SetDepthMask(0);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    if (visualizepass4 && m_bSaturationEnabled)
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        RenderSaturationAdjustment(cm_nTargetTexture);
    }

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapLighting);
    RenderInterface::SetDepthMask(1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#endif
}

// Runs the generic frame driver's nine conditional post-process passes under an orthographic screen
// projection.
void GLRender::FrameBufferModifications(uint64_t nFlags)
{
#if KOTOR1 // K1: GL_TEXTURE_2D target, no force-sight gate, no dream/fury passes
    GLRender::Disable(Aurora::kCapFog);

    if (!doframebuffer || !cm_bPBuffersAllocated || !cm_bSetUpFrameBuffer)
        return;

    if (!AurFrameBufferModificationsAvailable())
    {
        if (AurATIFrameBufferModificationsAvailable())
            FrameBufferModificationsATI(nFlags);
        return;
    }

    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glDepthMask(0);
    GLRender::Disable(Aurora::kCapBlend);

    if (!cm_bFirstFrameBufferFrame && (nFlags & kFBPassSpeedBlur) && visualizepass6
        && m_bSpeedBlurEnabled && s_bSpeedBlurAppliedDefault == 1)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glFlush();
        RenderAccumulationBlur(nTexture);
    }

    bool bBlurred = false;
    if ((nFlags & kFBPassSpeedBlur) && visualizepass6 && m_bSpeedBlurEnabled)
    {
        android_port_glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glFlush();
        GLRender::SetActiveTexture(0);
        RenderFrameToTexture(cm_nTargetTexture);
        bBlurred = true;
    }
    s_bSpeedBlurAppliedDefault = bBlurred;

    if ((nFlags & kFBPassOverbrighten) && visualizepass1)
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glFlush();
        RenderOverlayOverbrighten(cm_nTargetTexture);
    }

    if ((nFlags & kFBPassConvolution) && visualizepass2)
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glFlush();
        RenderOverlayConvolution(cm_nTargetTexture);
    }

    if ((nFlags & kFBPassForceDistortion) && visualizepass5)
    {
        android_port_glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        RenderForceDistortion(cm_nTargetTexture);
    }

    if (visualizepass3 && m_bScanNoiseEnabled && (nFlags & kFBPassNoise) && cm_pFilmNoiseTexture != 0
        && !cm_pFilmNoiseTexture->IsNull())
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        RenderOverlayFilmNoise(cm_nTargetTexture, true);
    }

    if (visualizepass7 && m_bFilmNoiseEnabled && (nFlags & kFBPassNoise) && cm_pFilmNoiseTexture != 0
        && !cm_pFilmNoiseTexture->IsNull())
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        RenderOverlayFilmNoise(cm_nTargetTexture, false);
    }

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapLighting);
    ASLgl::glDepthMask(1);
    cm_bFirstFrameBufferFrame = 0;
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#else
    GLRender::Disable(Aurora::kCapFog);

    if (!doframebuffer || !cm_bPBuffersAllocated || !cm_bSetUpFrameBuffer)
        return;

    if (!AurFrameBufferModificationsAvailable())
    {
        if (AurATIFrameBufferModificationsAvailable())
            FrameBufferModificationsATI(nFlags);
        return;
    }

    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glDepthMask(0);
    GLRender::Disable(Aurora::kCapBlend);

    if (!cm_bFirstFrameBufferFrame && !g_ForceSight && (nFlags & kFBPassSpeedBlur) && visualizepass6
        && m_bSpeedBlurEnabled && s_bSpeedBlurAppliedDefault == 1)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glFlush();
        RenderAccumulationBlur(nTexture);
    }

    bool bBlurred = false;
    if (!g_ForceSight && (nFlags & kFBPassSpeedBlur) && visualizepass6 && m_bSpeedBlurEnabled)
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glFlush();
        GLRender::SetActiveTexture(0);
        RenderFrameToTexture(cm_nTargetTexture);
        bBlurred = true;
    }
    s_bSpeedBlurAppliedDefault = bBlurred;

    if ((nFlags & kFBPassOverbrighten) && visualizepass1)
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glFlush();
        RenderOverlayOverbrighten(cm_nTargetTexture);
    }

    if ((nFlags & kFBPassConvolution) && visualizepass2)
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glFlush();
        RenderOverlayConvolution(cm_nTargetTexture);
    }

    if ((nFlags & kFBPassForceDistortion) && visualizepass5)
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        RenderForceDistortion(cm_nTargetTexture);
    }

    if (visualizepass3 && m_bScanNoiseEnabled && (nFlags & kFBPassNoise) && cm_pFilmNoiseTexture != 0
        && !cm_pFilmNoiseTexture->IsNull())
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        RenderOverlayFilmNoise(cm_nTargetTexture, true);
    }

    if (visualizepass7 && m_bFilmNoiseEnabled && (nFlags & kFBPassNoise) && cm_pFilmNoiseTexture != 0
        && !cm_pFilmNoiseTexture->IsNull())
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        RenderOverlayFilmNoise(cm_nTargetTexture, false);
    }

    if (m_bDreamDistortEnabled)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glFlush();
        GLRender::SetActiveTexture(0);
        RenderFrameToTexture(cm_nTargetTexture);
        RenderDreamDistortion(nTexture);
    }

    if (m_FuryLevel != 0)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glFlush();
        GLRender::SetActiveTexture(0);
        RenderFrameToTexture(cm_nTargetTexture);
        RenderFury(nTexture);
    }

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapLighting);
    ASLgl::glDepthMask(1);
    cm_bFirstFrameBufferFrame = 0;
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#endif
}

// Runs the ARB post-process chain, nine conditional passes over the framebuffer copy in
// cm_nTargetTexture.
void GLRender::FrameBufferModificationsARB(uint64_t nFlags)
{
#if KOTOR1
    K2_UNUSED(nFlags);
#endif
#if !KOTOR1 // K1: an empty body
    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glDepthMask(0);
    GLRender::Disable(Aurora::kCapBlend);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    if (!cm_bFirstFrameBufferFrame && (nFlags & kFBPassSpeedBlur) && visualizepass6 && m_bSpeedBlurEnabled
        && s_bSpeedBlurApplied == 1)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        RenderAccumulationBlurARB(nTexture);
    }

    bool bBlurred = false;
    if ((nFlags & kFBPassSpeedBlur) && visualizepass6 && m_bSpeedBlurEnabled)
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        GLRender::SetActiveTexture(0);
        RenderFrameToTextureARB(cm_nTargetTexture);
        bBlurred = true;
    }
    s_bSpeedBlurApplied = bBlurred;

    if ((nFlags & kFBPassOverbrighten) && visualizepass1)
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        RenderOverlayOverbrightenARB(cm_nTargetTexture);
    }

    if ((nFlags & kFBPassConvolution) && visualizepass2)
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        RenderOverlayConvolutionARB(cm_nTargetTexture);
    }

    if ((nFlags & kFBPassForceDistortion) && visualizepass5)
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        RenderForceDistortionARB(cm_nTargetTexture);
    }

    if (visualizepass3 && m_bScanNoiseEnabled && (nFlags & kFBPassNoise) && cm_pFilmNoiseTexture != 0
        && !cm_pFilmNoiseTexture->IsNull())
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        RenderOverlayFilmNoiseARB(cm_nTargetTexture, true);
    }

    if (visualizepass7 && m_bFilmNoiseEnabled && (nFlags & kFBPassNoise) && cm_pFilmNoiseTexture != 0
        && !cm_pFilmNoiseTexture->IsNull())
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        RenderOverlayFilmNoiseARB(cm_nTargetTexture, false);
    }

    if (m_bDreamDistortEnabled)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        GLRender::SetActiveTexture(0);
        RenderFrameToTextureARB(cm_nTargetTexture);
        RenderDreamDistortionARB(nTexture, false);
    }

    if (m_FuryLevel != 0)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
        GLRender::SetActiveTexture(0);
        RenderFrameToTextureARB(cm_nTargetTexture);
        RenderDreamDistortionARB(nTexture, true);
    }

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapLighting);
    ASLgl::glDepthMask(1);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    cm_bFirstFrameBufferFrame = 0;
#endif
}

// Runs the ATI twin of the ARB chain, the GL_TEXTURE_2D path with the power-of-two copy extent.
void GLRender::FrameBufferModificationsATI(uint64_t nFlags)
{
#if KOTOR1 // K1: the ATI chain is gone; only the shader probe is left
    K2_UNUSED(nFlags);
    AuroraUseARBShaders();
#else
    if (AuroraUseARBShaders())
    {
        FrameBufferModificationsARB(nFlags);
        return;
    }

    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glDepthMask(0);
    GLRender::Disable(Aurora::kCapBlend);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    if (!cm_bFirstFrameBufferFrame && !g_ForceSight && (nFlags & kFBPassSpeedBlur) && visualizepass6
        && m_bSpeedBlurEnabled && s_bSpeedBlurAppliedATI == 1)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glFlush();
        RenderAccumulationBlurATI(nTexture);
    }

    bool bBlurred = false;
    if (!g_ForceSight && (nFlags & kFBPassSpeedBlur) && visualizepass6 && m_bSpeedBlurEnabled)
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glFlush();
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        GLRender::SetActiveTexture(0);
        RenderFrameToTextureATI(cm_nTargetTexture);
        bBlurred = true;
    }
    s_bSpeedBlurAppliedATI = bBlurred;

    if ((nFlags & kFBPassOverbrighten) && visualizepass1)
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glFlush();
        RenderOverlayOverbrightenATI(cm_nTargetTexture);
    }

    if ((nFlags & kFBPassConvolution) && visualizepass2)
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glFlush();
        RenderOverlayConvolutionATI(cm_nTargetTexture);
    }

    if ((nFlags & kFBPassForceDistortion) && visualizepass5)
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glFlush();
        RenderForceDistortionATI(cm_nTargetTexture);
    }

    if (visualizepass3 && m_bScanNoiseEnabled && (nFlags & kFBPassNoise) && cm_pFilmNoiseTexture != 0
        && !cm_pFilmNoiseTexture->IsNull())
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glFlush();
        RenderOverlayFilmNoiseATI(cm_nTargetTexture, true);
    }

    if (visualizepass7 && m_bFilmNoiseEnabled && (nFlags & kFBPassNoise) && cm_pFilmNoiseTexture != 0
        && !cm_pFilmNoiseTexture->IsNull())
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glFlush();
        RenderOverlayFilmNoiseATI(cm_nTargetTexture, false);
    }

    if (m_bDreamDistortEnabled)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glFlush();
        GLRender::SetActiveTexture(0);
        RenderFrameToTextureATI(cm_nTargetTexture);
        RenderDreamDistortionATI(nTexture);
    }

    if (m_FuryLevel != 0)
    {
#ifdef K2_FIX_ENGINE_BUGS
        unsigned int nTexture = 0;
#else
        unsigned int nTexture;
#endif
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glFlush();
        GLRender::SetActiveTexture(0);
        RenderFrameToTextureATI(cm_nTargetTexture);
        RenderFuryATI(nTexture);
    }

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapLighting);
    ASLgl::glDepthMask(1);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    cm_bFirstFrameBufferFrame = 0;
#endif
}

// Draws the ping-pong blur composite. Unit 0 is the accumulation pbuffer, unit 1 the scene target.
void GLRender::RenderAccumulationBlur(unsigned int nTexture)
{
    K2_UNUSED(nTexture);
#if !KOTOR1 // K1: an empty body
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
    wglBindTexImageARB(cm_hAccumulationPBuffer, WGL_FRONT_LEFT_ARB);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glBegin(7);

    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, (i < 2) ? 0.0f : 1.0f,
                                    (i == 0 || i == 3) ? 1.0f : 0.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, (i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                                    (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f);
        ASLgl::glVertex3f((i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                          (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f, 0.0f);
    }

    ASLgl::glEnd();
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
#endif
}

// The ARB-shader build of the blur composite, with four explicit corners.
void GLRender::RenderAccumulationBlurATI(unsigned int nTexture)
{
    K2_UNUSED(nTexture);
#if !KOTOR1 // K1: an empty body
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
    wglBindTexImageARB(cm_hAccumulationPBuffer, WGL_FRONT_LEFT_ARB);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (AuroraUseARBShaders())
    {
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nAccumulationBlurShaderARB);
    }
    ASLgl::glDisable(GL_LIGHTING);

    float fU = static_cast<float>(g_nScreenWidth) / static_cast<float>(cm_nScreenWidthPow2);
    float fV = static_cast<float>(g_nScreenHeight) / static_cast<float>(cm_nScreenHeightPow2);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, fV);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 0.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 1.0f, 0.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, fU, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 1.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, fU, fV);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glEnd();
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    GLRender::SetActiveTexture(0);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glFlush();
#endif
}

// Draws the fixed-function saturation composite, overwriting the screen with the source texture.
void GLRender::RenderSaturationAdjustment(unsigned int nTexture)
{
#if KOTOR1
    K2_UNUSED(nTexture);
#endif
#if !KOTOR1 // K1: an empty body
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glDisable(GL_BLEND);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glEnd();
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
#endif
}

// Renders the accumulation pbuffer into nTexture under a 0..128 orthographic projection.
void GLRender::RenderFrameToTexture(unsigned int nTexture)
{
#if KOTOR1
    K2_UNUSED(nTexture);
#endif
#if !KOTOR1 // K1: an empty body
    wglMakeCurrent(cm_hAccumulationHDC, cm_hAccumulationHGLRC);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, 128.0, 0.0, 128.0, -1.0, 1.0);
    ASLgl::glViewport(0, 0, 128, 128);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, nTexture);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, 128.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(128.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(128.0f, 128.0f, 0.0f);

    ASLgl::glEnd();
    ASLgl::glFlush();
    wglMakeCurrent(m_hDC, m_hGLRC);
#endif
}

// Draws the film-noise overlay. The second argument only offsets the noise texcoords.
void GLRender::RenderOverlayFilmNoise(unsigned int nTexture, bool bScanNoise)
{
#if KOTOR1
    K2_UNUSED(nTexture); K2_UNUSED(bScanNoise);
#endif
#if !KOTOR1 // K1: an empty body
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pFilmNoiseTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    ASLgl::glBegin(7);

    float fOffset = bScanNoise ? 0.0f : 7.5f;
    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, (i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                                    (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, (i < 2) ? fOffset : 0.0f,
                                    (i == 0 || i == 3) ? 0.0f : 7.5f);
        ASLgl::glVertex3f((i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                          (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f, 0.0f);
    }

    ASLgl::glEnd();
    GLRender::SetActiveTexture(0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glFlush();
#endif
}

// The ARB-shader saturation composite, with the basis vectors pushed as env parameters.
void GLRender::RenderSaturationAdjustmentATI(unsigned int nTexture)
{
#if KOTOR1
    K2_UNUSED(nTexture);
#endif
#if !KOTOR1 // K1: an empty body
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (AuroraUseARBShaders())
    {
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 3, s_afSaturationBasisR);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 4, s_afSaturationBasisG);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 5, s_afSaturationBasisB);
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSaturationShaderARB);
    }
    ASLgl::glBegin(7);

    float fU = static_cast<float>(g_nScreenWidth) / static_cast<float>(cm_nScreenWidthPow2);
    float fV = static_cast<float>(g_nScreenHeight) / static_cast<float>(cm_nScreenHeightPow2);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, fV);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, fV);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glEnd();
    FrameBufferModificationPixelShadersATI(kFBShaderOff,  0, 0, 0, 0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#endif
}

// The ARB-shader build of RenderFrameToTexture, using the dual-DC context switch.
void GLRender::RenderFrameToTextureATI(unsigned int nTexture)
{
#if KOTOR1
    K2_UNUSED(nTexture);
#endif
#if !KOTOR1 // K1: an empty body
    void* hDC = m_hDC;
    void* hGLRC = m_hGLRC;
    wglMakeContextCurrentARB(cm_hAccumulationHDC, cm_hAccumulationHDC, cm_hAccumulationHGLRC);
    ASLgl::glDrawBuffer(GL_FRONT);
    ASLgl::glViewport(0, 0, 128, 128);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, 128.0, 0.0, 128.0, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    if (AuroraUseARBShaders())
    {
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderARB);
    }
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    ASLgl::glBlendFunc(1, 0);

    float fU = static_cast<float>(g_nScreenWidth) / static_cast<float>(cm_nScreenWidthPow2);
    float fV = static_cast<float>(g_nScreenHeight) / static_cast<float>(cm_nScreenHeightPow2);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, fV);
    ASLgl::glVertex3f(0.0f, 128.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, 0.0f);
    ASLgl::glVertex3f(128.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, fV);
    ASLgl::glVertex3f(128.0f, 128.0f, 0.0f);

    ASLgl::glEnd();
    FrameBufferModificationPixelShadersATI(kFBShaderOff,  0, 0, 0, 0);
    ASLgl::glFlush();
    wglMakeContextCurrentARB(hDC, hDC, hGLRC);
    ASLgl::glDrawBuffer(GL_BACK);
#endif
}

// Draws the ARB-shader overbrighten composite. The three ramp ids go straight to the program.
void GLRender::RenderOverlayOverbrightenATI(unsigned int nTexture)
{
    ASLgl::glBlendFunc(1, 0);
    FrameBufferModificationPixelShadersATI(kFBShaderOverbrighten,  nTexture, cm_nOverbrightenRed,
                                           cm_nOverbrightenGreen, cm_nOverbrightenBlue);

    float fU = static_cast<float>(g_nScreenWidth) / static_cast<float>(cm_nScreenWidthPow2);
    float fV = static_cast<float>(g_nScreenHeight) / static_cast<float>(cm_nScreenHeightPow2);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, fV);
    ASLgl::glVertex4f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, 0.0f);
    ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f, 1.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, fV);
    ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);

    ASLgl::glEnd();
    FrameBufferModificationPixelShadersATI(kFBShaderOff,  0, 0, 0, 0);
#if !KOTOR1 // K1: the texture stays bound
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
#endif
}

// The ARB-shader film-noise overlay, with four explicit vertices.
void GLRender::RenderOverlayFilmNoiseATI(unsigned int nTexture, bool bScanNoise)
{
#if KOTOR1
    K2_UNUSED(nTexture); K2_UNUSED(bScanNoise);
#endif
#if !KOTOR1 // K1: an empty body
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pFilmNoiseTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (AuroraUseARBShaders())
    {
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nFilmNoiseShaderARB);
    }

    float fU = static_cast<float>(g_nScreenWidth) / static_cast<float>(cm_nScreenWidthPow2);
    float fV = static_cast<float>(g_nScreenHeight) / static_cast<float>(cm_nScreenHeightPow2);
    ASLgl::glBegin(7);
    float fOffset = bScanNoise ? 0.0f : 7.5f;

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, fV);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, fOffset, 0.0f);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 0.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, fOffset, 7.5f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, fU, 0.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, 7.5f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, fU, fV);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glEnd();
    FrameBufferModificationPixelShadersATI(kFBShaderOff,  0, 0, 0, 0);
    GLRender::SetActiveTexture(0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glFlush();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#endif
}

// Draws the bloom/convolution composite with a context switch, three downsample passes and a final
// blit.
void GLRender::RenderOverlayConvolution(unsigned int nTexture)
{
    K2_UNUSED(nTexture);
#if !KOTOR1 // K1: an empty body
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::Enable(Aurora::kCapBlend);
    wglMakeCurrent(cm_hConvolutionHDC[3], cm_hConvolutionHGLRC[3]);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, 512.0, 0.0, 512.0, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, 512, 512);
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    ASLgl::glDisable(GL_DEPTH_TEST);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, 512.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(512.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(512.0f, 512.0f, 0.0f);

    ASLgl::glEnd();
    ASLgl::glFlush();

#ifdef K2_FIX_ENGINE_BUGS
    float fSize = 0.0f;
#else
    float fSize;
#endif
    MipmapFilter(reinterpret_cast<HDC__*>(cm_hConvolutionPBuffer[2]), cm_hConvolutionHGLRC[2], 256.0f, 256.0f,
                 cm_hConvolutionPBuffer[3], cm_nConvolutionRenderTexture[3], fSize);
    MipmapFilter(reinterpret_cast<HDC__*>(cm_hConvolutionPBuffer[1]), cm_hConvolutionHGLRC[1], 128.0f, 128.0f,
                 cm_hConvolutionPBuffer[2], cm_nConvolutionRenderTexture[2], fSize);
    MipmapFilter(reinterpret_cast<HDC__*>(cm_hConvolutionPBuffer[0]), cm_hConvolutionHGLRC[0], 64.0f, 64.0f,
                 cm_hConvolutionPBuffer[1], cm_nConvolutionRenderTexture[1], fSize);
    BloomFilter(reinterpret_cast<HDC__*>(cm_hConvolutionPBuffer[5]), cm_hConvolutionHGLRC[5], 256.0f, 256.0f,
                cm_hConvolutionPBuffer[baselayer], cm_nConvolutionRenderTexture[baselayer],
                basesize, 0);

    wglMakeCurrent(m_hDC, m_hGLRC);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[displaylayer]);
    wglBindTexImageARB(cm_hConvolutionPBuffer[displaylayer], WGL_FRONT_LEFT_ARB);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 1.0f);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 1.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glEnd();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

// The ARB-shader build of the convolution composite.
void GLRender::RenderOverlayConvolutionATI(unsigned int nTexture)
{
#if KOTOR1
    K2_UNUSED(nTexture);
#endif
#if !KOTOR1 // K1: an empty body
    if (!renderregular)
    {
        ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    }
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::Enable(Aurora::kCapBlend);
    void* hDC = m_hDC;
    void* hGLRC = m_hGLRC;
    wglMakeContextCurrentARB(cm_hConvolutionHDC[3], cm_hConvolutionHDC[3], cm_hConvolutionHGLRC[3]);
    ASLgl::glDrawBuffer(GL_FRONT);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, 512.0, 0.0, 512.0, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, 512, 512);
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GLRender::Disable(Aurora::kCapLighting);
    FrameBufferModificationPixelShadersATI(kFBShaderFlare,  nTexture, cm_nRedTexture, cm_nGreenTexture,
                                           cm_nBlueTexture);

    float fU = static_cast<float>(g_nScreenWidth) / static_cast<float>(cm_nScreenWidthPow2);
    float fV = static_cast<float>(g_nScreenHeight) / static_cast<float>(cm_nScreenHeightPow2);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, fV);
    ASLgl::glVertex3f(0.0f, 512.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, fU, 0.0f);
    ASLgl::glVertex3f(512.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, fU, fV);
    ASLgl::glVertex3f(512.0f, 512.0f, 0.0f);

    ASLgl::glEnd();
    FrameBufferModificationPixelShadersATI(kFBShaderOff,  0, 0, 0, 0);
    ASLgl::glFlush();
    wglMakeContextCurrentARB(hDC, hDC, hGLRC);

#ifdef K2_FIX_ENGINE_BUGS
    float fSize = 0.0f;
#else
    float fSize;
#endif
    MipmapFilterATI(reinterpret_cast<HDC__*>(cm_hConvolutionPBuffer[2]), cm_hConvolutionHGLRC[2], 256.0f, 256.0f,
                    cm_hConvolutionPBuffer[3], cm_nConvolutionRenderTexture[3], fSize);
    MipmapFilterATI(reinterpret_cast<HDC__*>(cm_hConvolutionPBuffer[1]), cm_hConvolutionHGLRC[1], 128.0f, 128.0f,
                    cm_hConvolutionPBuffer[2], cm_nConvolutionRenderTexture[2], fSize);
    MipmapFilterATI(reinterpret_cast<HDC__*>(cm_hConvolutionPBuffer[0]), cm_hConvolutionHGLRC[0], 64.0f, 64.0f,
                    cm_hConvolutionPBuffer[1], cm_nConvolutionRenderTexture[1], fSize);
    BloomFilterATI(reinterpret_cast<HDC__*>(cm_hConvolutionPBuffer[5]), cm_hConvolutionHGLRC[5], 256.0f, 256.0f,
                   cm_hConvolutionPBuffer[baselayer], cm_nConvolutionRenderTexture[baselayer],
                   basesize, 0);

    wglMakeContextCurrentARB(hDC, hDC, hGLRC);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[displaylayer]);
    wglBindTexImageARB(cm_hConvolutionPBuffer[displaylayer], WGL_FRONT_LEFT_ARB);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (AuroraUseARBShaders())
    {
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderARB);
    }
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 1.0f);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 1.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glEnd();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    FrameBufferModificationPixelShadersATI(kFBShaderOff,  0, 0, 0, 0);
#endif
}

#if KOTOR1
// K1: an empty stub that nothing calls (the pbuffer path does not exist there)
void GLRender::MipmapFilter(void* hDC, void* hGLRC, float fWidth, float fHeight, PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(fWidth); K2_UNUSED(fHeight); K2_UNUSED(pPBuffer); K2_UNUSED(nTexture); K2_UNUSED(fSize);
}
#else
// Runs one convolution downsample: ortho over the level size, clear, bind the source, blit a
// unit-square quad.
void GLRender::MipmapFilter(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                           HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize)
{
    K2_UNUSED(fSize);
    wglMakeCurrent(hDC, hGLRC);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(fWidth), 0.0, static_cast<double>(fHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, static_cast<int>(fWidth), static_cast<int>(fHeight));
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    wglBindTexImageARB(pPBuffer, WGL_FRONT_LEFT_ARB);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 1.0f);
    ASLgl::glVertex3f(0.0f, fHeight, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 0.0f);
    ASLgl::glVertex3f(fWidth, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 1.0f);
    ASLgl::glVertex3f(fWidth, fHeight, 0.0f);

    ASLgl::glEnd();
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
}
#endif

#if KOTOR1
// K1: an empty stub that nothing calls (the pbuffer path does not exist there)
void GLRender::MipmapFilterATI(void* hDC, void* hGLRC, float fWidth, float fHeight, PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(fWidth); K2_UNUSED(fHeight); K2_UNUSED(pPBuffer); K2_UNUSED(nTexture); K2_UNUSED(fSize);
}
#else
// ARB-shader build of MipmapFilter, with the dual-DC context switch and a program-gated decal.
void GLRender::MipmapFilterATI(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                               HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize)
{
    K2_UNUSED(fSize);
    wglMakeContextCurrentARB(hDC, hDC, hGLRC);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(fWidth), 0.0, static_cast<double>(fHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, static_cast<int>(fWidth), static_cast<int>(fHeight));
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    wglBindTexImageARB(pPBuffer, WGL_FRONT_LEFT_ARB);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (AuroraUseARBShaders())
    {
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderARB);
    }
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 1.0f);
    ASLgl::glVertex3f(0.0f, fHeight, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 1.0f, 0.0f);
    ASLgl::glVertex3f(fWidth, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 1.0f, 1.0f);
    ASLgl::glVertex3f(fWidth, fHeight, 0.0f);

    ASLgl::glEnd();
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
}
#endif

#if KOTOR1
// K1: an empty stub that nothing calls (the pbuffer path does not exist there)
void GLRender::BloomFilterATI(void* hDC, void* hGLRC, float fWidth, float fHeight, PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize, int bSoftShadow)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(fWidth); K2_UNUSED(fHeight); K2_UNUSED(pPBuffer); K2_UNUSED(nTexture); K2_UNUSED(fSize); K2_UNUSED(bSoftShadow);
}
#else
// The ATI bloom accumulation pass, with the same tap walk as the plain build.
void GLRender::BloomFilterATI(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                              HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize,
                              int bSoftShadow)
{
    FrameBufferModificationPixelShadersATI(kFBShaderOff,  0, 0, 0, 0);
    wglMakeContextCurrentARB(hDC, hDC, hGLRC);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(fWidth), 0.0, static_cast<double>(fHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, static_cast<int>(fWidth), static_cast<int>(fHeight));
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);

    for (int nUnit = 0; nUnit < 4; nUnit++)
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0 + nUnit);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
        wglBindTexImageARB(pPBuffer, WGL_FRONT_LEFT_ARB);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    if (bSoftShadow)
    {
        if (AuroraUseARBShaders())
        {
            ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
            ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSoftShadowShaderARB);
        }
    }
    else
    {
        if (AuroraUseARBShaders())
        {
            ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
            ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_n4DecalShaderARB);
        }
    }
    ASLgl::glEnable(GL_BLEND);

    int nRow = 0;
    int nCol = 0;
    for (int nIndex = 0; nIndex < 25; nIndex += 4)
    {
        int nRow1 = (nRow + 1) % 5;
        int nCol1 = nCol + ((nRow + 1) > 4);
        int nRow2 = (nRow + 2) % 5;
        int nCol2 = nCol + ((nRow + 2) > 4);
        int nRow3 = (nRow + 3) % 5;
        int nCol3 = nCol + ((nRow + 3) > 4);
        float fU0 = static_cast<float>(nRow),  fV0 = static_cast<float>(nCol);
        float fU1 = static_cast<float>(nCol1), fV1 = static_cast<float>(nRow1);
        float fU2 = static_cast<float>(nCol2), fV2 = static_cast<float>(nRow2);
        float fU3 = static_cast<float>(nCol3), fV3 = static_cast<float>(nRow3);

        ASLgl::glBegin(7);
        for (int nCorner = 0; nCorner < 4; nCorner++)
        {
            float fTexOffset = 0.0f;

            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, multiplier);

            ASLgl::glActiveTextureARB(GL_TEXTURE0);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0,
                (nCorner > 1) ? (fU0 + fSize - pixeloffset) / fSize
                              : (fU0 - pixeloffset) / fSize + fTexOffset,
                (nCorner != 1 && nCorner != 2) ? (fV0 + fSize - pixeloffset) / fSize
                                               : (fV0 - pixeloffset) / fSize + fTexOffset);

            ASLgl::glActiveTextureARB(GL_TEXTURE1);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1,
                (nCorner > 1) ? (fU1 + fSize - pixeloffset) / fSize
                              : (fU1 - pixeloffset) / fSize + fTexOffset,
                (nCorner != 1 && nCorner != 2) ? (fV1 + fSize - pixeloffset) / fSize
                                               : (fV1 - pixeloffset) / fSize);

            ASLgl::glActiveTextureARB(GL_TEXTURE2);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE2,
                (nCorner > 1) ? (fU2 + fSize - pixeloffset) / fSize
                              : (fU2 - pixeloffset) / fSize,
                (nCorner != 1 && nCorner != 2) ? (fV2 + fSize - pixeloffset) / fSize
                                               : (fV2 - pixeloffset) / fSize);

            ASLgl::glActiveTextureARB(GL_TEXTURE3);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3,
                (nCorner > 1) ? (fU3 + fSize - pixeloffset) / fSize
                              : (fU3 - pixeloffset) / fSize,
                (nCorner != 1 && nCorner != 2) ? (fV3 + fSize - pixeloffset) / fSize
                                               : (fV3 - pixeloffset) / fSize);
            ASLgl::glActiveTextureARB(GL_TEXTURE0);

            ASLgl::glVertex3f(nCorner < 2 ? 0.0f : fWidth,
                              (nCorner == 0 || nCorner == 3) ? fHeight : 0.0f, 0.0f);
        }
        ASLgl::glEnd();
        nCol += ((nRow + 4) > 4);
        nRow = (nRow + 4) % 5;
    }

    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glFlush();
}
#endif

#if KOTOR1
// K1: an empty stub that nothing calls (the pbuffer path does not exist there)
void GLRender::BloomFilter(void* hDC, void* hGLRC, float fWidth, float fHeight, PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize, int bSoftShadow)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(fWidth); K2_UNUSED(fHeight); K2_UNUSED(pPBuffer); K2_UNUSED(nTexture); K2_UNUSED(fSize); K2_UNUSED(bSoftShadow);
}
#else
// Runs the bloom accumulation pass with four units on the same pbuffer, seven quads in a rolling
// 5x5 tap grid.
void GLRender::BloomFilter(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                           HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize,
                           int bSoftShadow)
{
    __asm__ __volatile__("" : : "r"(bSoftShadow));  // idiom-exempt: RenderOverlayConvolution + FinishSoftShadows MATCH
#if defined(_MSC_VER) && !defined(__clang__)
    K2_UNUSED(bSoftShadow);   // cl.exe has no GNU asm: the barrier above is compiled away
#endif
    wglMakeCurrent(hDC, hGLRC);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(fWidth), 0.0, static_cast<double>(fHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, static_cast<int>(fWidth), static_cast<int>(fHeight));
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);

    for (int nUnit = 0; nUnit < 4; nUnit++)
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0 + nUnit);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
        wglBindTexImageARB(pPBuffer, WGL_FRONT_LEFT_ARB);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_BLEND);

    int nRow = 0;
    int nCol = 0;
    for (int nIndex = 0; nIndex < 25; nIndex += 4)
    {
        int nRow1 = (nRow + 1) % 5;
        int nCol1 = nCol + ((nRow + 1) > 4);
        int nRow2 = (nRow + 2) % 5;
        int nCol2 = nCol + ((nRow + 2) > 4);
        int nRow3 = (nRow + 3) % 5;
        int nCol3 = nCol + ((nRow + 3) > 4);
        float fU0 = static_cast<float>(nRow),  fV0 = static_cast<float>(nCol);
        float fU1 = static_cast<float>(nCol1), fV1 = static_cast<float>(nRow1);
        float fU2 = static_cast<float>(nCol2), fV2 = static_cast<float>(nRow2);
        float fU3 = static_cast<float>(nCol3), fV3 = static_cast<float>(nRow3);

        ASLgl::glBegin(7);
        for (int nCorner = 0; nCorner < 4; nCorner++)
        {
            float fTexOffset = 0.0f;

            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, multiplier);

            ASLgl::glActiveTextureARB(GL_TEXTURE0);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0,
                (nCorner > 1) ? (fU0 + fSize - pixeloffset) / fSize
                              : (fU0 - pixeloffset) / fSize + fTexOffset,
                (nCorner != 1 && nCorner != 2) ? (fV0 + fSize - pixeloffset) / fSize
                                               : (fV0 - pixeloffset) / fSize + fTexOffset);

            ASLgl::glActiveTextureARB(GL_TEXTURE1);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1,
                (nCorner > 1) ? (fU1 + fSize - pixeloffset) / fSize
                              : (fU1 - pixeloffset) / fSize + fTexOffset,
                (nCorner != 1 && nCorner != 2) ? (fV1 + fSize - pixeloffset) / fSize
                                               : (fV1 - pixeloffset) / fSize);

            ASLgl::glActiveTextureARB(GL_TEXTURE2);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE2,
                (nCorner > 1) ? (fU2 + fSize - pixeloffset) / fSize
                              : (fU2 - pixeloffset) / fSize,
                (nCorner != 1 && nCorner != 2) ? (fV2 + fSize - pixeloffset) / fSize
                                               : (fV2 - pixeloffset) / fSize);

            ASLgl::glActiveTextureARB(GL_TEXTURE3);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3,
                (nCorner > 1) ? (fU3 + fSize - pixeloffset) / fSize
                              : (fU3 - pixeloffset) / fSize,
                (nCorner != 1 && nCorner != 2) ? (fV3 + fSize - pixeloffset) / fSize
                                               : (fV3 - pixeloffset) / fSize);

            ASLgl::glVertex3f(nCorner < 2 ? 0.0f : fWidth,
                              (nCorner == 0 || nCorner == 3) ? fHeight : 0.0f, 0.0f);
        }
        ASLgl::glEnd();
        nCol += ((nRow + 4) > 4);
        nRow = (nRow + 4) % 5;
    }

    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glFlush();
}
#endif

#if KOTOR1
// K1: an empty stub that nothing calls (the pbuffer path does not exist there)
void GLRender::BloomFilterFromTexture(void* hDC, void* hGLRC, float fWidth, float fHeight, unsigned int nTexture, float fSizeU, float fSizeV)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(fWidth); K2_UNUSED(fHeight); K2_UNUSED(nTexture); K2_UNUSED(fSizeU); K2_UNUSED(fSizeV);
}
#else
// The BloomFilter twin that samples an existing texture instead of a pbuffer.
void GLRender::BloomFilterFromTexture(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                                      unsigned int nTexture, float fSizeU, float fSizeV)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);

    for (int nUnit = 0; nUnit < 4; nUnit++)
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0 + nUnit);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_BLEND);

    int nRow = 0;
    int nCol = 0;
    for (int nIndex = 0; nIndex < 25; nIndex += 4)
    {
        int nRow1 = (nRow + 1) % 5;
        int nCol1 = nCol + ((nRow + 1) > 4);
        int nRow2 = (nRow + 2) % 5;
        int nCol2 = nCol + ((nRow + 2) > 4);
        int nRow3 = (nRow + 3) % 5;
        int nCol3 = nCol + ((nRow + 3) > 4);
        float fU0 = static_cast<float>(nRow),  fV0 = static_cast<float>(nCol);
        float fU1 = static_cast<float>(nCol1), fV1 = static_cast<float>(nRow1);
        float fU2 = static_cast<float>(nCol2), fV2 = static_cast<float>(nRow2);
        float fU3 = static_cast<float>(nCol3), fV3 = static_cast<float>(nRow3);

        ASLgl::glBegin(7);
        for (int nCorner = 0; nCorner < 4; nCorner++)
        {
            float fTexOffset = 0.0f;

            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, multiplier);

            ASLgl::glActiveTextureARB(GL_TEXTURE0);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0,
                (nCorner < 2) ? (fU0 - 1.5f) / fSizeU + fTexOffset
                              : (fU0 + fSizeU - 1.5f) / fSizeU,
                (nCorner == 0 || nCorner == 3) ? (fV0 + fSizeV - 1.5f) / fSizeV
                                               : (fV0 - 1.5f) / fSizeV + fTexOffset);

            ASLgl::glActiveTextureARB(GL_TEXTURE1);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1,
                (nCorner < 2) ? (fU1 - 1.5f) / fSizeU + fTexOffset
                              : (fU1 + fSizeU - 1.5f) / fSizeU,
                (nCorner == 0 || nCorner == 3) ? (fV1 + fSizeV - 1.5f) / fSizeV
                                               : (fV1 - 1.5f) / fSizeV);

            ASLgl::glActiveTextureARB(GL_TEXTURE2);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE2,
                (nCorner < 2) ? (fU2 - 1.5f) / fSizeU
                              : (fU2 + fSizeU - 1.5f) / fSizeU,
                (nCorner == 0 || nCorner == 3) ? (fV2 + fSizeV - 1.5f) / fSizeV
                                               : (fV2 - 1.5f) / fSizeV);

            ASLgl::glActiveTextureARB(GL_TEXTURE3);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3,
                (nCorner < 2) ? (fU3 - 1.5f) / fSizeU
                              : (fU3 + fSizeU - 1.5f) / fSizeU,
                (nCorner == 0 || nCorner == 3) ? (fV3 + fSizeV - 1.5f) / fSizeV
                                               : (fV3 - 1.5f) / fSizeV);

            ASLgl::glVertex3f(nCorner < 2 ? 0.0f : fWidth,
                              (nCorner == 0 || nCorner == 3) ? fHeight : 0.0f, 0.0f);
        }
        ASLgl::glEnd();
        nCol += ((nRow + 4) > 4);
        nRow = (nRow + 4) % 5;
    }

    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glFlush();
}
#endif

#if KOTOR1
// K1: an empty stub that nothing calls (the pbuffer path does not exist there)
void GLRender::BloomFilterFromTextureATI(void* hDC, void* hGLRC, float fWidth, float fHeight, unsigned int nTexture, float fSizeU, float fSizeV)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(fWidth); K2_UNUSED(fHeight); K2_UNUSED(nTexture); K2_UNUSED(fSizeU); K2_UNUSED(fSizeV);
}
#else
// The ATI half of the FromTexture pair, sampling the power-of-two render targets.
void GLRender::BloomFilterFromTextureATI(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                                         unsigned int nTexture, float fSizeU, float fSizeV)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(fSizeU); K2_UNUSED(fSizeV);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);

    for (int nUnit = 0; nUnit < 4; nUnit++)
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0 + nUnit);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    if (AuroraUseARBShaders())
    {
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSoftShadowShaderARB);
    }
    ASLgl::glEnable(GL_BLEND);

    int nRow = 0;
    int nCol = 0;
    for (int nIndex = 0; nIndex < 25; nIndex += 4)
    {
        int nRow1 = (nRow + 1) % 5;
        int nCol1 = nCol + ((nRow + 1) > 4);
        int nRow2 = (nRow + 2) % 5;
        int nCol2 = nCol + ((nRow + 2) > 4);
        int nRow3 = (nRow + 3) % 5;
        int nCol3 = nCol + ((nRow + 3) > 4);
        float fU0 = static_cast<float>(nRow),  fV0 = static_cast<float>(nCol);
        float fU1 = static_cast<float>(nCol1), fV1 = static_cast<float>(nRow1);
        float fU2 = static_cast<float>(nCol2), fV2 = static_cast<float>(nRow2);
        float fU3 = static_cast<float>(nCol3), fV3 = static_cast<float>(nRow3);

        float fSizeW = static_cast<float>(cm_nScreenWidthPow2);
        float fSizeH = static_cast<float>(cm_nScreenHeightPow2);
        ASLgl::glBegin(7);
        for (int nCorner = 0; nCorner < 4; nCorner++)
        {
            float fTexOffset = 0.0f;

            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, multiplier);

            ASLgl::glActiveTextureARB(GL_TEXTURE0);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0,
                (nCorner > 1) ? (fU0 + fSizeW - pixeloffset) / fSizeW
                              : (fU0 - pixeloffset) / fSizeW + fTexOffset,
                (nCorner != 1 && nCorner != 2) ? (fV0 + fSizeH - pixeloffset) / fSizeH
                                               : (fV0 - pixeloffset) / fSizeH + fTexOffset);

            ASLgl::glActiveTextureARB(GL_TEXTURE1);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1,
                (nCorner > 1) ? (fU1 + fSizeW - pixeloffset) / fSizeW
                              : (fU1 - pixeloffset) / fSizeW + fTexOffset,
                (nCorner != 1 && nCorner != 2) ? (fV1 + fSizeH - pixeloffset) / fSizeH
                                               : (fV1 - pixeloffset) / fSizeH);

            ASLgl::glActiveTextureARB(GL_TEXTURE2);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE2,
                (nCorner > 1) ? (fU2 + fSizeW - pixeloffset) / fSizeW
                              : (fU2 - pixeloffset) / fSizeW,
                (nCorner != 1 && nCorner != 2) ? (fV2 + fSizeH - pixeloffset) / fSizeH
                                               : (fV2 - pixeloffset) / fSizeH);

            ASLgl::glActiveTextureARB(GL_TEXTURE3);
            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3,
                (nCorner > 1) ? (fU3 + fSizeW - pixeloffset) / fSizeW
                              : (fU3 - pixeloffset) / fSizeW,
                (nCorner != 1 && nCorner != 2) ? (fV3 + fSizeH - pixeloffset) / fSizeH
                                               : (fV3 - pixeloffset) / fSizeH);

            ASLgl::glVertex3f(nCorner < 2 ? 0.0f : fWidth,
                              (nCorner == 0 || nCorner == 3) ? fHeight : 0.0f, 0.0f);
        }
        ASLgl::glEnd();
        nCol += ((nRow + 4) > 4);
        nRow = (nRow + 4) % 5;
    }

    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glFlush();
}
#endif

#if !KOTOR1
// The 5x5 bloom accumulation kernel, pushed into the fragment program's env parameters.
static const float s_afBloomKernel[25] = {
    0.12f, 0.25f, 0.29f, 0.25f, 0.12f,
    0.25f, 0.49f, 0.58f, 0.49f, 0.25f,
    0.29f, 0.58f, 0.67f, 0.58f, 0.29f,
    0.25f, 0.49f, 0.58f, 0.49f, 0.25f,
    0.12f, 0.25f, 0.29f, 0.25f, 0.12f,
};
#endif
float softshadowdarkeningfactor = 0.400000006f;

#if KOTOR1
// K1: an empty stub that nothing calls (the pbuffer path does not exist there)
void GLRender::BloomFilterARB(void* hDC, void* hGLRC, float fWidth, float fHeight, PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize, int bSoftShadow)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(fWidth); K2_UNUSED(fHeight); K2_UNUSED(pPBuffer); K2_UNUSED(nTexture); K2_UNUSED(fSize); K2_UNUSED(bSoftShadow);
}
#else
// Runs the ARB bloom accumulation pass, the same tap walk with the kernel pushed as env parameters.
void GLRender::BloomFilterARB(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                              HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize,
                              int bSoftShadow)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(pPBuffer);
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(fWidth), 0.0, static_cast<double>(fHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, static_cast<int>(fWidth), static_cast<int>(fHeight));
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);

    for (int nUnit = 0; nUnit < 4; nUnit++)
    {
        ASLgl::glActiveTextureARB(GL_TEXTURE0 + nUnit);
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, bSoftShadow ? cm_nSoftShadowShaderARB : cm_n4DecalShaderARB);
    ASLgl::glEnable(GL_BLEND);

    int nRow = 0;
    int nCol = 0;
    for (int nIndex = 0; nIndex < 25; nIndex += 4)
    {
        int nRow1 = (nRow + 1) % 5;
        int nCol1 = nCol + ((nRow + 1) > 4);
        int nRow2 = (nRow + 2) % 5;
        int nCol2 = nCol + ((nRow + 2) > 4);
        int nRow3 = (nRow + 3) % 5;
        int nCol3 = nCol + ((nRow + 3) > 4);
        float fU0 = static_cast<float>(nRow),  fV0 = static_cast<float>(nCol);
        float fU1 = static_cast<float>(nCol1), fV1 = static_cast<float>(nRow1);
        float fU2 = static_cast<float>(nCol2), fV2 = static_cast<float>(nRow2);
        float fU3 = static_cast<float>(nCol3), fV3 = static_cast<float>(nRow3);

        if (bSoftShadow)
        {
            for (int k = 0; k < 4; k++)
            {
                float afParam[4];
                afParam[0] = afParam[1] = afParam[2] = afParam[3] =
                    softshadowdarkeningfactor * s_afBloomKernel[nIndex + k];
                ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, k, afParam);
            }
        }
        else
        {
            for (int k = 0; k < 4; k++)
            {
                float afParam[4];
                afParam[0] = afParam[1] = afParam[2] = afParam[3] = s_afBloomKernel[nIndex + k];
                ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, k, afParam);
            }
        }

        ASLgl::glBegin(7);
        for (int nCorner = 0; nCorner < 4; nCorner++)
        {
            float fTexOffset = 0.0f;

            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, multiplier);

            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0,
                (nCorner > 1) ? (fU0 + fSize - pixeloffset) / fSize
                              : (fU0 - pixeloffset) / fSize + fTexOffset,
                (nCorner != 1 && nCorner != 2) ? (fV0 + fSize - pixeloffset) / fSize
                                               : (fV0 - pixeloffset) / fSize + fTexOffset);

            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1,
                (nCorner > 1) ? (fU1 + fSize - pixeloffset) / fSize
                              : (fU1 - pixeloffset) / fSize + fTexOffset,
                (nCorner != 1 && nCorner != 2) ? (fV1 + fSize - pixeloffset) / fSize
                                               : (fV1 - pixeloffset) / fSize);

            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE2,
                (nCorner > 1) ? (fU2 + fSize - pixeloffset) / fSize
                              : (fU2 - pixeloffset) / fSize,
                (nCorner != 1 && nCorner != 2) ? (fV2 + fSize - pixeloffset) / fSize
                                               : (fV2 - pixeloffset) / fSize);

            ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3,
                (nCorner > 1) ? (fU3 + fSize - pixeloffset) / fSize
                              : (fU3 - pixeloffset) / fSize,
                (nCorner != 1 && nCorner != 2) ? (fV3 + fSize - pixeloffset) / fSize
                                               : (fV3 - pixeloffset) / fSize);

            ASLgl::glVertex3f(nCorner < 2 ? 0.0f : fWidth,
                              (nCorner == 0 || nCorner == 3) ? fHeight : 0.0f, 0.0f);
        }
        ASLgl::glEnd();
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        nCol += ((nRow + 4) > 4);
        nRow = (nRow + 4) % 5;
    }

    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
}
#endif

// Runs the heat-haze distortion pass, the only member of the family that binds a vertex program,
// across all three units.
void GLRender::RenderForceDistortion(unsigned int nTexture)
{
#if KOTOR1
    K2_UNUSED(nTexture);
#endif
#if !KOTOR1 // K1: an empty body
    ASLgl::glEnable(GL_VERTEX_PROGRAM_ARB);
    ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 15, 0.0f, 1.0f, 0.0f, 0.0f);
    ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, cm_pHeatDistortionVertexProg);
    cm_pForceDistortionTexture->GetWidth();
    ASLgl::glBlendFunc(1, 0);
    GLRender::SetActiveTexture(0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pForceDistortionTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    GLRender::SetActiveTexture(2);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::Enable(Aurora::kCapStencilTest); // GL_STENCIL_TEST
    ASLgl::glStencilFunc(GL_LESS, 0, 1);
    ASLgl::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glBegin(7);

    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, (i < 2) ? 0.0f : 1.0f,
                                    (i == 0 || i == 3) ? 0.0f : 1.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, static_cast<float>(g_nScreenWidth) / 10.0f,
                                    static_cast<float>(g_nScreenHeight) / 10.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE2, (i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                                    (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f);
        ASLgl::glVertex3f((i >= 2) ? static_cast<float>(g_nScreenWidth) + 25.0f : 0.0f,
                          (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) + 25.0f : -25.0f, 0.0f);
    }

    ASLgl::glEnd();
    GLRender::Disable(Aurora::kCapStencilTest);
    ASLgl::glDisable(GL_VERTEX_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
    GLRender::SetActiveTexture(0);
    ASLgl::glFlush();
#endif
}

// The ARB-shader build of the distortion pass.
void GLRender::RenderForceDistortionATI(unsigned int nTexture)
{
#if KOTOR1
    K2_UNUSED(nTexture);
#endif
#if !KOTOR1 // K1: an empty body
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glEnable(GL_VERTEX_PROGRAM_ARB);
    ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 15, 0.0f, 1.0f, 0.0f, 0.0f);
    ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, cm_pHeatDistortionVertexProg);
    cm_pForceDistortionTexture->GetWidth();
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(1, 0);
    GLRender::SetActiveTexture(0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    cm_pForceDistortionTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (AuroraUseARBShaders())
    {
        ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nForceDistortionShaderARB);
    }
    GLRender::Enable(Aurora::kCapStencilTest);
    ASLgl::glStencilFunc(GL_LESS, 0, 1);
    ASLgl::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);

    float fU = static_cast<float>(g_nScreenWidth) / static_cast<float>(cm_nScreenWidthPow2);
    float fV = static_cast<float>(g_nScreenHeight) / static_cast<float>(cm_nScreenHeightPow2);
    ASLgl::glBegin(7);

    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, (i < 2) ? 0.0f : 1.0f,
                                    (i == 0 || i == 3) ? 0.0f : 1.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, fU / 10.0f, fV / 10.0f);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE2, (i < 2) ? 0.0f : fU,
                                    (i == 0 || i == 3) ? fV : 0.0f);
        ASLgl::glVertex3f((i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f,
                          (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) + 25.0f : -25.0f, 0.0f);
    }

    ASLgl::glEnd();
    GLRender::Disable(Aurora::kCapStencilTest);
    FrameBufferModificationPixelShadersATI(kFBShaderOff,  0, 0, 0, 0);
    ASLgl::glDisable(GL_VERTEX_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
    GLRender::SetActiveTexture(0);
    ASLgl::glFlush();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#endif
}

// Draws the dream-distortion quad: the four-unit texcoord walk and the letterbox vertex diamond.
void GLRender::RenderDreamDistortion(unsigned int nTexture)
{
    K2_UNUSED(nTexture);
    ASLgl::glDisable(GL_DEPTH_TEST);
    GLRender::Enable(Aurora::kCapAlphaTest); // GL_ALPHA_TEST
    GLRender::Enable(Aurora::kCapBlend); // GL_BLEND
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pForceDistortionTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glEnable(GL_TEXTURE_2D);
    m_pDreamDistortTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glBegin(7);

    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        float fU = (i < 2) ? 0.0f : 1.0f;
        float fV = (i == 0 || i == 3) ? 1.0f : 0.0f;
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, fU, fV);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE1, 0.15625f, 0.0f, fU);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE2, 0.0f, 0.15625f, fV);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3, fU, fV);

        if (g_bLetterBoxMode)
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_Letterbox_bottom) + 25.0f
                                          : static_cast<float>(g_Letterbox_top) - 25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
        else
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) + 25.0f : -25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
    }

    ASLgl::glEnd();
    ASLgl::glFlush();
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
}

// The same draw as RenderDreamDistortion, except that unit 3 binds m_pFuryTexture.
void GLRender::RenderFury(unsigned int nTexture)
{
    K2_UNUSED(nTexture);
    ASLgl::glDisable(GL_DEPTH_TEST);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapBlend);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pForceDistortionTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glEnable(GL_TEXTURE_2D);
    m_pFuryTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glBegin(7);

    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        float fU = (i < 2) ? 0.0f : 1.0f;
        float fV = (i == 0 || i == 3) ? 1.0f : 0.0f;
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, fU, fV);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE1, 0.15625f, 0.0f, fU);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE2, 0.0f, 0.15625f, fV);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3, fU, fV);

        if (g_bLetterBoxMode)
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_Letterbox_bottom) + 25.0f
                                          : static_cast<float>(g_Letterbox_top) - 25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
        else
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) + 25.0f : -25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
    }

    ASLgl::glEnd();
    ASLgl::glFlush();
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
}

// The ARB-shader build of RenderDreamDistortion.
void GLRender::RenderDreamDistortionATI(unsigned int nTexture)
{
    K2_UNUSED(nTexture);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glDisable(GL_DEPTH_TEST);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapBlend);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pForceDistortionTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glEnable(GL_TEXTURE_2D);
    m_pDreamDistortTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glBegin(7);

    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        float fU = (i < 2) ? 0.0f : 1.0f;
        float fV = (i == 0 || i == 3) ? 1.0f : 0.0f;
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, fU, fV);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE1, 0.15625f, 0.0f, fU);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE2, 0.0f, 0.15625f, fV);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3, fU, fV);

        if (g_bLetterBoxMode)
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_Letterbox_bottom) + 25.0f
                                          : static_cast<float>(g_Letterbox_top) - 25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
        else
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) + 25.0f : -25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
    }

    ASLgl::glEnd();
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glFlush();
}

// The same draw as RenderDreamDistortionATI, except that unit 3 binds m_pFuryTexture.
void GLRender::RenderFuryATI(unsigned int nTexture)
{
    K2_UNUSED(nTexture);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glDisable(GL_DEPTH_TEST);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapBlend);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pForceDistortionTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glEnable(GL_TEXTURE_2D);
    m_pFuryTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glBegin(7);

    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
        ASLgl::glActiveTextureARB(GL_TEXTURE0);
        float fU = (i < 2) ? 0.0f : 1.0f;
        float fV = (i == 0 || i == 3) ? 1.0f : 0.0f;
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, fU, fV);
        ASLgl::glActiveTextureARB(GL_TEXTURE1);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE1, 0.15625f, 0.0f, fU);
        ASLgl::glActiveTextureARB(GL_TEXTURE2);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE2, 0.0f, 0.15625f, fV);
        ASLgl::glActiveTextureARB(GL_TEXTURE3);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3, fU, fV);

        if (g_bLetterBoxMode)
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_Letterbox_bottom) + 25.0f
                                          : static_cast<float>(g_Letterbox_top) - 25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
        else
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) + 25.0f : -25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
    }

    ASLgl::glEnd();
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glFlush();
}

// The grass array holds a block of positions, then a block of normals and texcoord pairs.
struct GrassEntry
{
    float m_afNormal[3];
    float m_afTexCoord0[2];
    float m_afTexCoord1[2];
};

// The body is duplicated on purpose: the VBO arm ends by rebinding buffer 0, the immediate arm draws.
void GLRender::DrawLightmappedGrass(void* pVertices, int nVertexStride, void* pUnk,
                                    int nNormalStride, int nStart, int nVertexCount)
{
    K2_UNUSED(pUnk);
    char* pEntries = static_cast<char*>(pVertices);

#if KOTOR1 // K1: nothing is drawn with grass switched off
    if (!IosGrassEnabled())
        return;
#endif
    if (AurVertexBufferObjectARBAvailable())
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
        ASLgl::glVertexPointer(3, GL_FLOAT, nVertexStride, pVertices);
        nVertexCount *= 4;
        pEntries += nVertexCount * 12;
        GrassEntry* pEntry = reinterpret_cast<GrassEntry*>(pEntries);
        ASLgl::glNormalPointer(GL_FLOAT, nNormalStride, pEntry->m_afNormal);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, nNormalStride, pEntry->m_afTexCoord0);
        GLRender::ActivateTextureStage(1);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, nNormalStride, pEntry->m_afTexCoord1);
        GLRender::ActivateTextureStage(0);
        ASLgl::glDrawArrays(7, nStart, nVertexCount);
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);
    }
    else
    {
        ASLgl::glVertexPointer(3, GL_FLOAT, nVertexStride, pVertices);
        nVertexCount *= 4;
        pEntries += nVertexCount * 12;
        GrassEntry* pEntry = reinterpret_cast<GrassEntry*>(pEntries);
        ASLgl::glNormalPointer(GL_FLOAT, nNormalStride, pEntry->m_afNormal);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, nNormalStride, pEntry->m_afTexCoord0);
        GLRender::ActivateTextureStage(1);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, nNormalStride, pEntry->m_afTexCoord1);
        GLRender::ActivateTextureStage(0);
        ASLgl::glDrawArrays(7, nStart, nVertexCount);
    }
}

#if KOTOR1
bool IosGrassEnabled()
{
    return AurQueryGrassEnabled();
}
#endif

void GLRender::RefreshTemporary(unsigned int a, unsigned int b)
{
    manRef->RefreshPool(a, b);
}

// The third argument is unused; the count is scaled by four on the way to glDrawElements.
void GLRender::DrawElementsInShadow(int nCount, unsigned int nPool, int nUnused)
{
    K2_UNUSED(nUnused);
    void* paIndices = manRef->GetIndexPool(nPool);
    GLRender::SetVertexShaderConstant(15, 1.0f, 0.0f, 0.0f, 100000.0f);
    ASLgl::glDrawElements(4, nCount * 4, GL_UNSIGNED_SHORT, paIndices);
}

// Returns the byte offset of one face inside a compressed contact sheet, with 16-byte blocks for
// 4-level, 8 otherwise.
int GetCompressedContactSheetOffset(int nNumBlocks, int nSize, int nLevels)
{
    int nBytesPerBlock = (nLevels == 4) ? 16 : 8;
    int nBlocks = (nSize + 3) / 4;
    int nTotal = nBlocks * nBlocks * nBytesPerBlock;

    unsigned int nCur = nSize;
    if (nSize >= 2)
    {
        do
        {
            nCur >>= 1;
            unsigned int n = (nCur + 3) >> 2;
            nTotal += n * n * nBytesPerBlock;
        } while (nCur > 1);
    }
    return nTotal * nNumBlocks;
}

extern int maxshadowvar;

// Returns the scratch buffer, not a status.
void* AllocateShadowMemory(int nSize)
{
    static char* s_pShadowMemory = new char[nSize];
    static int s_nShadowMemorySize = nSize;

    if (maxshadowvar < nSize)
        maxshadowvar = nSize;
    if (s_nShadowMemorySize < nSize)
    {
        delete[] s_pShadowMemory;
        s_pShadowMemory = new char[nSize];
        s_nShadowMemorySize = nSize;
    }
    return s_pShadowMemory;
}


// Empty in this build: the ARB combiner path belongs to the vertex-program variant.
inline void GLRender::SetCombinerConstant0(Vector& vConstant, float fValue)
{
    K2_UNUSED(vConstant); K2_UNUSED(fValue);
}

inline void GLRender::SetCombinerConstant1(Vector& vConstant, float fValue)
{
    K2_UNUSED(vConstant); K2_UNUSED(fValue);
}

int g_nAliasingMode;

#ifdef K2_QOL_IMPROVEMENTS
// The scene buffer's size. Anti-aliasing is supersampling: the screen scaled by the square root of
// the sample count per axis, which the present pass's bilinear tap averages back down, so 4 samples
// is the ceiling. Without it, the screen's size.
static void K2QolSceneBufferSize(int& nWidth, int& nHeight)
{
    nWidth = g_nScreenWidth;
    nHeight = g_nScreenHeight;
    if (g_nAliasingMode < 2)
    {
#ifdef K2_MOBILE_OPT
        if (nWidth >= 2048)
        {
            nWidth = nWidth * 7 / 10;
            nHeight = nHeight * 7 / 10;
        }
#endif
        return;
    }
    float fScale = g_nAliasingMode >= 4 ? 2.0f : 1.41421356f;
    int nMaxTexture = 0;
    int nMaxRenderbuffer = 0;
    ASLgl::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &nMaxTexture);        // GL_MAX_TEXTURE_SIZE
    ASLgl::glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &nMaxRenderbuffer);   // GL_MAX_RENDERBUFFER_SIZE
    int nMax = nMaxTexture;
    if (nMaxRenderbuffer > 0 && (nMax <= 0 || nMaxRenderbuffer < nMax))
        nMax = nMaxRenderbuffer;
    int nLongest = nWidth > nHeight ? nWidth : nHeight;
    if (nMax > 0 && nLongest * fScale > static_cast<float>(nMax))
        fScale = static_cast<float>(nMax) / static_cast<float>(nLongest);
    if (fScale < 1.0f)
        fScale = 1.0f;
    nWidth = static_cast<int>(nWidth * fScale);
    nHeight = static_cast<int>(nHeight * fScale);
}

// A re-initialisation (a mode or anti-aliasing change) builds a new set; the context, and with it
// the old set, lives on.
static void K2QolDeletePBufferIos(PBUFFERARB_IOS*& pPBuffer)
{
    if (!pPBuffer)
        return;
    glDeleteFramebuffers(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nFramebuffer));
    ASLgl::glDeleteTextures(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nTexture));
    ASLgl::glDeleteRenderbuffersEXT(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nDepthRenderbuffer));
    ASLgl::glDeleteRenderbuffersEXT(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nStencilRenderbuffer));
    delete pPBuffer;
    pPBuffer = 0;
}
#endif

#if KOTOR1 && defined(K2_QOL_K1_ANTIALIAS)
extern "C" void glDeleteRenderbuffers(int nCount, const unsigned int* paRenderbuffers);

// Scales the scene buffer's size under anti-aliasing, which supersamples: mode 2 is the APK's 1.4
// per axis, 4 and up is 2 per axis (the present pass's bilinear tap is then a 2x2 box), clamped to
// the largest buffer the driver allocates.
static void K1SceneBufferScale(int& nWidth, int& nHeight)
{
    if (g_IosAntiAlias < 4)
    {
        nWidth = nWidth * 14 / 10;
        nHeight = nHeight * 14 / 10;
        return;
    }
    float fScale = 2.0f;
    int nMaxTexture = 0;
    int nMaxRenderbuffer = 0;
    ASLgl::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &nMaxTexture);        // GL_MAX_TEXTURE_SIZE
    ASLgl::glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &nMaxRenderbuffer);   // GL_MAX_RENDERBUFFER_SIZE
    int nMax = nMaxTexture;
    if (nMaxRenderbuffer > 0 && (nMax <= 0 || nMaxRenderbuffer < nMax))
        nMax = nMaxRenderbuffer;
    int nLongest = nWidth > nHeight ? nWidth : nHeight;
    if (nMax > 0 && nLongest * fScale > static_cast<float>(nMax))
        fScale = static_cast<float>(nMax) / static_cast<float>(nLongest);
    if (fScale < 1.0f)
        fScale = 1.0f;
    nWidth = static_cast<int>(nWidth * fScale);
    nHeight = static_cast<int>(nHeight * fScale);
}

// Anti-aliasing runs the pbuffer pass with the frame-buffer switch off too (Scene::RenderSinglePass);
// bloom stays with the switch.
extern int preferPerfToVisual;
static bool K1BloomThisFrame()
{
    return IosBloomEnabled() && (GLRender::m_bSaturationEnabled || !preferPerfToVisual);
}

// A re-initialisation (an anti-aliasing or effects change) builds a new set; the context, and with
// it the old set, lives on.
static void K1DeletePBuffer(PBUFFERARB_IOS*& pPBuffer)
{
    if (!pPBuffer)
        return;
    glDeleteFramebuffers(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nFramebuffer));
    ASLgl::glDeleteTextures(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nTexture));
    glDeleteRenderbuffers(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nDepthRenderbuffer));
    delete pPBuffer;
    pPBuffer = 0;
}
#endif

// Creates the five pbuffers and clears the distort target.
#if KOTOR1 && defined(K2_QOL_FULLSCREEN)
// desktop K1, a live window change (swguioptions.cpp K1QolApplyDisplay): K1's re-init keeps the
// scene pbuffers, sized for the old window -- the scene and its viewport stayed at that size.
void K1QolResizeSceneBuffers()
{
    GLRender::InitializeFrameBufferModifications();
}
#endif

void GLRender::InitializeFrameBufferModificationsARB()
{
#if KOTOR1 // K1: shaders first, the display size scaled UP for AA, no distort target, two halves only with bloom
    InitializeFrameBufferModificationFragmentShadersARB();
#ifdef K2_QOL_K1_ANTIALIAS
    K1DeletePBuffer(s_pPBuffer);
    K1DeletePBuffer(s_pHalfPBuffer1);
    K1DeletePBuffer(s_pHalfPBuffer2);
#endif
    int nWidth = g_SDL_mode.w;
    int nHeight = g_SDL_mode.h;
    if (IosAntiAliasOn())
        IosAntiAliasScaleSize(nWidth, nHeight);
    s_pPBuffer = CreatePBufferIos(nWidth, nHeight);
    if (IosBloomEnabled())
    {
        nWidth /= 2;
        nHeight /= 2;
        s_pHalfPBuffer1 = CreatePBufferIos(nWidth, nHeight);
        s_pHalfPBuffer2 = CreatePBufferIos(nWidth, nHeight);
    }
#else
    int nWidth = g_nScreenWidth;
    int nHeight = g_nScreenHeight;
#ifdef K2_QOL_IMPROVEMENTS
    K2QolDeletePBufferIos(s_pPBuffer);
    K2QolDeletePBufferIos(s_pDistortPBuffer);
    K2QolDeletePBufferIos(s_pHalfPBuffer1);
    K2QolDeletePBufferIos(s_pHalfPBuffer2);
    K2QolDeletePBufferIos(s_pHalfPBuffer3);
    K2QolSceneBufferSize(nWidth, nHeight);
#elif defined(K2_MOBILE_OPT)   // the mobile port's: the scene at 7/10 of a screen 2048 or wider
    if (nWidth >= 2048)
    {
        nWidth = nWidth * 7 / 10;
        nHeight = nHeight * 7 / 10;
    }
#endif
    s_pPBuffer = CreatePBufferIos(nWidth, nHeight, false);
    s_pDistortPBuffer = CreatePBufferIos(s_pPBuffer->m_nWidth, s_pPBuffer->m_nHeight, true);
    IosSetDistortBuffer(true);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, s_nSavedDistortFramebuffer);
    nWidth /= 2;
    nHeight /= 2;
    s_pHalfPBuffer1 = CreatePBufferIos(nWidth, nHeight, false);
    s_pHalfPBuffer2 = CreatePBufferIos(nWidth, nHeight, false);
    s_pHalfPBuffer3 = CreatePBufferIos(nWidth, nHeight, false);
#endif
}

#ifdef K2_QOL_SOFT_SHADOWS
// Soft shadows, the PC's image-space pass (SetUpSoftShadows / FinishSoftShadowsARB /
// BloomFilterARB): the scene so far is saved and its colour cleared, the stencil fans write the
// shadow mask into it (alpha = darkness, ONE / ZERO, Scene::RenderStencilShadowsForThisLight), and
// the mask, knocked down to 512x512 and blurred, darkens the restored scene. It needs the scene in
// the pbuffer, whose alpha holds the mask: KOTOR II always draws it there, K1 only with the
// frame-buffer switch on, which its shadows need anyway.
bool g_bQolSoftShadowPass;
static unsigned int s_anQolSoftShadowTextures[4];       // scene copy, mask copy, 512 knock-down, 512 blur
static unsigned int s_anQolSoftShadowFramebuffers[2];   // knock-down, blur
static int s_nQolSoftShadowWidth;
static int s_nQolSoftShadowHeight;

// The framebuffer calls, each game's spelling.
static void QolSoftShadowBind(unsigned int nFramebuffer)
{
#if KOTOR1
    glBindFramebufferOES(GL_FRAMEBUFFER, nFramebuffer);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, nFramebuffer);
#endif
}

static void QolSoftShadowViewport(int nX, int nY, int nWidth, int nHeight)
{
#if KOTOR1
    android_port_glViewportRaw(nX, nY, nWidth, nHeight);
#else
    ASLgl::glViewport(nX, nY, nWidth, nHeight);
#endif
}

static void QolSoftShadowTexture(unsigned int nTexture, int nWidth, int nHeight)
{
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// Unit 0's nTexture over the whole viewport through the port program nProgram.
static void QolSoftShadowQuad(unsigned int nProgram, unsigned int nTexture)
{
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, nProgram);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glBegin(7);
    ASLgl::glTexCoord2f(0.0f, 1.0f);
    ASLgl::glVertex4f(0.0f, 1.0f, 0.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 0.0f);
    ASLgl::glVertex4f(1.0f, 0.0f, 0.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 1.0f);
    ASLgl::glVertex4f(1.0f, 1.0f, 0.0f, 1.0f);
    ASLgl::glEnd();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
}

// Saves the scene and clears its colour for the mask; false (the hard shadows draw instead) when the
// scene is not in the pbuffer.
static bool QolSoftShadowsSetUp()
{
    int nFramebuffer = 0;
    ASLgl::glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &nFramebuffer);
    if (!s_pPBuffer || nFramebuffer != s_pPBuffer->m_nFramebuffer)
        return false;
    int nWidth = s_pPBuffer->m_nWidth;
    int nHeight = s_pPBuffer->m_nHeight;

    GLRender::SetActiveTexture(0);
    if (s_anQolSoftShadowTextures[0] == 0)
    {
        ASLgl::glGenTextures(4, s_anQolSoftShadowTextures);
#if KOTOR1
        ASLgl::glGenFramebuffers(2, s_anQolSoftShadowFramebuffers);
#else
        ::glGenFramebuffers(2, s_anQolSoftShadowFramebuffers);
#endif
        for (int i = 0; i < 2; i++)
        {
            QolSoftShadowTexture(s_anQolSoftShadowTextures[2 + i], 512, 512);
            QolSoftShadowBind(s_anQolSoftShadowFramebuffers[i]);
#if KOTOR1
            glFramebufferTexture2DOES(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_anQolSoftShadowTextures[2 + i], 0);
#else
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_anQolSoftShadowTextures[2 + i], 0);
#endif
        }
        QolSoftShadowBind(nFramebuffer);
    }
    if (s_nQolSoftShadowWidth != nWidth || s_nQolSoftShadowHeight != nHeight)
    {
        QolSoftShadowTexture(s_anQolSoftShadowTextures[0], nWidth, nHeight);
        QolSoftShadowTexture(s_anQolSoftShadowTextures[1], nWidth, nHeight);
        s_nQolSoftShadowWidth = nWidth;
        s_nQolSoftShadowHeight = nHeight;
    }

    ASLgl::glBindTexture(GL_TEXTURE_2D, s_anQolSoftShadowTextures[0]);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, nWidth, nHeight);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    float afClearColor[4];
    ASLgl::glGetFloatv(GL_COLOR_CLEAR_VALUE, afClearColor);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    ASLgl::glClearColor(afClearColor[0], afClearColor[1], afClearColor[2], afClearColor[3]);
    return true;
}

// Knocks the mask down, blurs it, restores the scene and darkens it by the blurred mask.
static void QolSoftShadowsFinish()
{
    int nWidth = s_nQolSoftShadowWidth;
    int nHeight = s_nQolSoftShadowHeight;
    int nFramebuffer = 0;
    ASLgl::glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &nFramebuffer);
    int anViewport[4];
    GLRender::GetViewPort(anViewport);

    GLRender::Disable(Aurora::kCapDepthTest);
    ASLgl::glDisable(GL_STENCIL_TEST);
    ASLgl::glDisable(GL_BLEND);
    ASLgl::glDepthMask(0);
    GLRender::SetActiveTexture(0);
    ASLgl::glEnable(GL_TEXTURE_2D);

    ASLgl::glBindTexture(GL_TEXTURE_2D, s_anQolSoftShadowTextures[1]);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, nWidth, nHeight);

    QolSoftShadowViewport(0, 0, 512, 512);
    QolSoftShadowBind(s_anQolSoftShadowFramebuffers[0]);
    QolSoftShadowQuad(kPortProgramCopy, s_anQolSoftShadowTextures[1]);
    QolSoftShadowBind(s_anQolSoftShadowFramebuffers[1]);
    QolSoftShadowQuad(kPortProgramSoftShadowBlur, s_anQolSoftShadowTextures[2]);

    QolSoftShadowBind(nFramebuffer);
    QolSoftShadowViewport(0, 0, nWidth, nHeight);
    QolSoftShadowQuad(kPortProgramCopy, s_anQolSoftShadowTextures[0]);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    QolSoftShadowQuad(kPortProgramCopy, s_anQolSoftShadowTextures[3]);

    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    QolSoftShadowViewport(anViewport[0], anViewport[1], anViewport[2], anViewport[3]);
    GLRender::Enable(Aurora::kCapDepthTest);
    ASLgl::glDepthMask(1);
    ASLgl::glStencilMask(0xff);
}
#endif

// Sets up ATI soft shadows. The ARB-shader build grabs the real screen size.
void GLRender::SetUpSoftShadowsATI()
{
#if !KOTOR1 // K1: an empty body
    if (AuroraUseARBShaders())
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nBufferRegionTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    }
    else
    {
        ASLgl::glEnable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nBufferRegionTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2,
                                   cm_nScreenHeightPow2);
        ASLgl::glDisable(GL_TEXTURE_2D);
        ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
        ASLgl::glFlush();
    }

    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
#endif
}

// Sets up soft shadows on the ARB fallback: grabs the framebuffer region and clears it to black.
void GLRender::SetUpSoftShadowsARB()
{
#if KOTOR1 // K1: GL_TEXTURE_2D
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nBufferRegionTexture);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
#else
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nBufferRegionTexture);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
#endif
}

// Dispatches to the ATI path when available, else the ARB-shaped fallback.
void GLRender::SetUpSoftShadows()
{
#ifdef K2_QOL_SOFT_SHADOWS
    g_bQolSoftShadowPass = QolSoftShadowsSetUp();
#elif !KOTOR1 // K1: an empty body
    if (AurATISoftShadowsAvailable())
    {
        SetUpSoftShadowsATI();
        return;
    }

    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nBufferRegionTexture);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glFlush();
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
#endif
}

// Runs the ATI soft-shadow blend pass, four phases over three contexts and a 512x512 bloom.
void GLRender::FinishSoftShadowsATI()
{
#if !KOTOR1 // K1: an empty body
    if (AuroraUseARBShaders())
    {
        FinishSoftShadowsARB();
        return;
    }

    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapBlend);
    ASLgl::glDisable(GL_STENCIL_TEST);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, cm_nScreenWidthPow2, cm_nScreenHeightPow2);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glFlush();
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    wglMakeContextCurrentARB(cm_hSoftShadowHDC, cm_hSoftShadowHDC, cm_hSoftShadowHGLRC);
    ASLgl::glDrawBuffer(GL_FRONT);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nTargetTexture);
    ASLgl::glDisable(GL_LIGHTING);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glDepthMask(0);
    ASLgl::glViewport(0, 0, 512, 512);
    float fU = static_cast<float>(g_nScreenWidth) / static_cast<float>(cm_nScreenWidthPow2);
    float fV = static_cast<float>(g_nScreenHeight) / static_cast<float>(cm_nScreenHeightPow2);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, fV);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, fV);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glEnd();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    wglMakeContextCurrentARB(m_hDC, m_hDC, m_hGLRC);
    ASLgl::glDrawBuffer(GL_BACK);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nBufferRegionTexture);
    ASLgl::glDisable(GL_LIGHTING);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glDepthMask(0);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, fV);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(fU, fV);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glEnd();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    ASLgl::glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    wglMakeContextCurrentARB(cm_hSoftShadowTargetHDC, cm_hSoftShadowTargetHDC,
                             cm_hSoftShadowTargetHGLRC);
    ASLgl::glDrawBuffer(GL_FRONT);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, 512, 512);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(1, 0);
    BloomFilterATI(reinterpret_cast<HDC__*>(cm_hSoftShadowTargetPBuffer), cm_hSoftShadowTargetHGLRC, 512.0f,
                   512.0f, cm_hSoftShadowPBuffer, cm_nSoftShadowRenderTexture, 512.0f, 1);
    FrameBufferModificationPixelShadersATI(kFBShaderOff,  0, 0, 0, 0);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    ASLgl::glFlush();
    wglMakeContextCurrentARB(m_hDC, m_hDC, m_hGLRC);
    ASLgl::glDrawBuffer(GL_BACK);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nSoftShadowTargetRenderTexture);
    wglBindTexImageARB(cm_hSoftShadowTargetPBuffer, WGL_FRONT_LEFT_ARB);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 1.0f);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 1.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glEnd();
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glPopAttrib();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    GLRender::Enable(Aurora::kCapDepthTest);
    ASLgl::glDisable(GL_STENCIL_TEST);
    ASLgl::glDepthMask(1);
    ASLgl::glStencilMask(0xff);
#endif
}

// Runs the ARB blend pass, with three framebuffer bindings in place of the ATI context pair.
void GLRender::FinishSoftShadowsARB()
{
#if !KOTOR1 // K1: an empty body
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapBlend);
    ASLgl::glDisable(GL_STENCIL_TEST);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    glBindFramebuffer(GL_FRAMEBUFFER, s_ShadowKnockDownFBO);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
    ASLgl::glDisable(GL_LIGHTING);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glDepthMask(0);
    ASLgl::glViewport(0, 0, 512, 512);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glEnd();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nBufferRegionTexture);
    ASLgl::glDisable(GL_LIGHTING);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glDepthMask(0);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glEnd();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, s_ShadowBlurFBO);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, 512, 512);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(1, 0);
#ifdef K2_FIX_ENGINE_BUGS
    HDC__* hDC = nullptr;
    HGLRC__* hGLRC = nullptr;
    HPBUFFERARB__* pPBuffer = nullptr;
#else
    HDC__* hDC;
    HGLRC__* hGLRC;
    HPBUFFERARB__* pPBuffer;
#endif
    BloomFilterARB(hDC, hGLRC, 512.0f, 512.0f, pPBuffer, cm_nSoftShadowRenderTexture, 512.0f, 1);
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nSoftShadowTargetRenderTexture);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 1.0f);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 1.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glEnd();
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glPopAttrib();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    GLRender::Enable(Aurora::kCapDepthTest);
    ASLgl::glDisable(GL_STENCIL_TEST);
    ASLgl::glDepthMask(1);
    ASLgl::glStencilMask(0xff);
    ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
#endif
}

// Runs the top-level blend pass, the ATI body when available, else the wglMakeCurrent fallback.
void GLRender::FinishSoftShadows()
{
#ifdef K2_QOL_SOFT_SHADOWS
    if (g_bQolSoftShadowPass)
        QolSoftShadowsFinish();
    g_bQolSoftShadowPass = false;
#elif !KOTOR1 // K1: an empty body
    if (AurATISoftShadowsAvailable())
    {
        FinishSoftShadowsATI();
        return;
    }

    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glFlush();
    wglMakeCurrent(cm_hSoftShadowTargetHDC, cm_hSoftShadowTargetHGLRC);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glDisable(GL_STENCIL_TEST);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, 512, 512);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glEnd();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    wglMakeCurrent(m_hDC, m_hGLRC);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nBufferRegionTexture);
    ASLgl::glDisable(GL_LIGHTING);
    GLRender::Enable(Aurora::kCapBlend);
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glDisable(GL_STENCIL_TEST);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glDepthMask(0);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glEnd();
    ASLgl::glEnable(GL_DEPTH_TEST);
    ASLgl::glDepthMask(1);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glDepthMask(0);
    ASLgl::glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    wglMakeCurrent(cm_hSoftShadowHDC, cm_hSoftShadowHGLRC);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    ASLgl::glViewport(0, 0, 512, 512);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    BloomFilter(reinterpret_cast<HDC__*>(cm_hSoftShadowPBuffer), cm_hSoftShadowHGLRC, 512.0f, 512.0f,
                cm_hSoftShadowTargetPBuffer, cm_nSoftShadowTargetRenderTexture, 512.0f, 1);
    wglMakeCurrent(m_hDC, m_hGLRC);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nSoftShadowRenderTexture);
    wglBindTexImageARB(cm_hSoftShadowPBuffer, WGL_FRONT_LEFT_ARB);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 1.0f);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 1.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glEnd();
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glPopAttrib();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    GLRender::Enable(Aurora::kCapDepthTest);
    ASLgl::glDisable(GL_STENCIL_TEST);
    ASLgl::glDepthMask(1);
    ASLgl::glStencilMask(0xff);
#endif
}

#if KOTOR1
// K1: a texture with linear/clamp parameters, one depth renderbuffer (packed depth-stencil when
// available, else a stencil renderbuffer the record does not keep); the renderbuffer binding is not saved.
PBUFFERARB* CreatePBufferIos(int nWidth, int nHeight)
{
    PBUFFERARB* pPBuffer = new PBUFFERARB;
    memset(pPBuffer, 0, 16);  // idiom-exempt: K1 zeroes the leading 16 bytes of the 0x14-byte record
    pPBuffer->m_nWidth = nWidth;
    pPBuffer->m_nHeight = nHeight;
    ASLgl::glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &s_nSavedFramebuffer);
    ASLgl::glGenTextures(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nTexture));
    ASLgl::glBindTexture(GL_TEXTURE_2D, pPBuffer->m_nTexture);
    ASLgl::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<float>(GL_LINEAR));
    ASLgl::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<float>(GL_LINEAR));
    ASLgl::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<float>(GL_CLAMP_TO_EDGE));
    ASLgl::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<float>(GL_CLAMP_TO_EDGE));
    ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glGenRenderbuffers(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nDepthRenderbuffer));
    ASLgl::glBindRenderbuffer(GL_RENDERBUFFER, pPBuffer->m_nDepthRenderbuffer);
    bool bPackedDepthStencil = glExtensionSupported("GL_OES_packed_depth_stencil");
    if (bPackedDepthStencil)
        ASLgl::glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, nWidth, nHeight);
    else
        ASLgl::glRenderbufferStorage(GL_RENDERBUFFER, glExtensionSupported("GL_OES_depth24") ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16,
                                     nWidth, nHeight);
    ASLgl::glBindRenderbuffer(GL_RENDERBUFFER, 0);
    ASLgl::glGenFramebuffers(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nFramebuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, pPBuffer->m_nFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pPBuffer->m_nTexture, 0);
    ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pPBuffer->m_nDepthRenderbuffer);
    if (bPackedDepthStencil)
    {
        ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pPBuffer->m_nDepthRenderbuffer);
    }
    else
    {
        unsigned int nStencilRenderbuffer;
        ASLgl::glGenRenderbuffers(1, &nStencilRenderbuffer);
        ASLgl::glBindRenderbuffer(GL_RENDERBUFFER, nStencilRenderbuffer);
        ASLgl::glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, nWidth, nHeight);
        ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, nStencilRenderbuffer);
    }
    ASLgl::glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, s_nSavedFramebuffer);
    return pPBuffer;
}
#else
// The iOS pbuffer factory; the third argument selects the texture format and the depth block.
PBUFFERARB_IOS* CreatePBufferIos(int nWidth, int nHeight, bool bNoDepth)
{
    PBUFFERARB_IOS* pPBuffer = new PBUFFERARB_IOS;
    pPBuffer->m_nFramebuffer = 0;
    pPBuffer->m_nDepthRenderbuffer = 0;
    pPBuffer->m_nStencilRenderbuffer = 0;
    pPBuffer->m_nTexture = 0;
    pPBuffer->m_nWidth = nWidth;
    pPBuffer->m_nHeight = nHeight;
    ASLgl::glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &s_nSavedFramebuffer);
    int nSavedRenderbuffer;
    ASLgl::glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &nSavedRenderbuffer);
    ::glGenFramebuffers(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nFramebuffer));
    ASLgl::glGenTextures(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nTexture));
    ASLgl::glGenRenderbuffers(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nDepthRenderbuffer));
    ASLgl::glGenRenderbuffers(1, reinterpret_cast<unsigned int*>(&pPBuffer->m_nStencilRenderbuffer));
    glBindFramebuffer(GL_FRAMEBUFFER, pPBuffer->m_nFramebuffer);
    ASLgl::glBindTexture(GL_TEXTURE_2D, pPBuffer->m_nTexture);
    if (bNoDepth)
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nWidth, nHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0);
    else
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pPBuffer->m_nTexture, 0);
    if (!bNoDepth)
    {
        ASLgl::glBindRenderbuffer(GL_RENDERBUFFER, pPBuffer->m_nDepthRenderbuffer);
        if (glExtensionSupported("GL_OES_packed_depth_stencil"))
        {
            // A packed depth/stencil renderbuffer is attached to both attachments.
            ASLgl::glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, nWidth, nHeight);
            ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                             pPBuffer->m_nDepthRenderbuffer);
            ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                             pPBuffer->m_nDepthRenderbuffer);
        }
        else
        {
            ASLgl::glRenderbufferStorage(GL_RENDERBUFFER, glExtensionSupported("GL_OES_depth24")
                                                    ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16, nWidth, nHeight);
            ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                             pPBuffer->m_nDepthRenderbuffer);
            ASLgl::glBindRenderbuffer(GL_RENDERBUFFER, pPBuffer->m_nStencilRenderbuffer);
            ASLgl::glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, nWidth, nHeight);
            ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                             pPBuffer->m_nStencilRenderbuffer);
        }
        int nStatus = static_cast<int>(ASLgl::glCheckFramebufferStatus(GL_FRAMEBUFFER));
        if (nStatus == GL_FRAMEBUFFER_COMPLETE)
            puts("fbo complete");
        else if (nStatus == GL_FRAMEBUFFER_UNSUPPORTED)
        {
            // GL_FRAMEBUFFER_UNSUPPORTED: drop the stencil attachment and keep the depth one.
            puts("fbo unsupported");
            ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
            pPBuffer->m_nStencilRenderbuffer = 0;
            ASLgl::glCheckFramebufferStatus(GL_FRAMEBUFFER);
            puts("running with no stencil");
        }
        else
            puts("Framebuffer Error");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, s_nSavedFramebuffer);
    ASLgl::glBindRenderbuffer(GL_RENDERBUFFER, nSavedRenderbuffer);
    return pPBuffer;
}
#endif

int _aglRestoreContext()
{
#if KOTOR1
    glBindFramebufferOES(GL_FRAMEBUFFER, s_nSavedFramebuffer);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, s_nSavedFramebuffer);
#endif
    return 1;
}

int _aglSetCurrentPBuffer(PBUFFERARB_IOS* pPBuffer)
{
    ASLgl::glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &s_nSavedFramebuffer);
#if KOTOR1
    glBindFramebufferOES(GL_FRAMEBUFFER, pPBuffer->m_nFramebuffer);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, pPBuffer->m_nFramebuffer);
#endif
    return 1;
}

int _aglDestroyPBuffer(PBUFFERARB_IOS* pPBuffer)
{
    K2_UNUSED(pPBuffer);
    return 1;
}

int GetPbufferWidth(PBUFFERARB_IOS* pPBuffer)
{
    return pPBuffer->m_nWidth;
}

int GetPbufferHeight(PBUFFERARB_IOS* pPBuffer)
{
    return pPBuffer->m_nHeight;
}

int GetPbufferTex(PBUFFERARB_IOS* pPBuffer)
{
    return pPBuffer->m_nTexture;
}

#if !KOTOR1  // K1 has no distort pbuffer switch
// Binds the distort pbuffer and attaches the scene pbuffer's depth renderbuffer to it.
void IosSetDistortBuffer(bool bEnable)
{
    if (!bEnable)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, s_nSavedDistortFramebuffer);
        return;
    }
    ASLgl::glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &s_nSavedDistortFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, s_pDistortPBuffer->m_nFramebuffer);
    ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                     s_pPBuffer->m_nDepthRenderbuffer);
    if (glExtensionSupported("GL_OES_packed_depth_stencil"))
        ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                         s_pPBuffer->m_nDepthRenderbuffer);
    else if (s_pPBuffer->m_nStencilRenderbuffer)
        ASLgl::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                         s_pPBuffer->m_nStencilRenderbuffer);
}

#endif

// Seeds the two out-params from the screen size, then shrinks them by 7/10 on wide screens.
void IosAntiAliasScaleSize(int& nWidth, int& nHeight)
{
#if KOTOR1 // K1: the caller's size is scaled up by 1.4
#ifdef K2_QOL_K1_ANTIALIAS
    // The scene viewport is the scene buffer, whatever K1SceneBufferScale made it.
    if (s_pPBuffer)
    {
        nWidth = s_pPBuffer->m_nWidth;
        nHeight = s_pPBuffer->m_nHeight;
        return;
    }
    K1SceneBufferScale(nWidth, nHeight);
#else
    nWidth = nWidth * 14 / 10;
    nHeight = nHeight * 14 / 10;
#endif
#else
#ifdef K2_QOL_IMPROVEMENTS
    // The scene viewport is the scene buffer, whatever K2QolSceneBufferSize made it.
    if (s_pPBuffer)
    {
        nWidth = s_pPBuffer->m_nWidth;
        nHeight = s_pPBuffer->m_nHeight;
        return;
    }
#endif
    nWidth = g_nScreenWidth;
    nHeight = g_nScreenHeight;
#ifdef K2_MOBILE_OPT   // the mobile port's: the scene at 7/10 of a screen 2048 or wider
    if (g_nScreenWidth >= 2048)
    {
        nWidth = nWidth * 7 / 10;
        nHeight = nHeight * 7 / 10;
    }
#endif
#endif
}

#if KOTOR1 && defined(K2_QOL_K1_ANTIALIAS)
// The camera's anti-aliased viewport (the pbuffer's size) back to the screen's, for a scene drawn
// straight to the screen.
void K1SceneViewportToScreen()
{
    int anViewport[4];
    GLRender::GetViewPort(anViewport);
    if (s_pPBuffer && anViewport[0] == 0 && anViewport[1] == 0 &&
        anViewport[2] == s_pPBuffer->m_nWidth && anViewport[3] == s_pPBuffer->m_nHeight)
        ASLgl::glViewport(0, 0, g_SDL_mode.w, g_SDL_mode.h);
}
#endif

bool IosAntiAliasOn()
{
#if KOTOR1 // K1: the anti-alias setting
    return g_IosAntiAlias > 0;
#else
    return true;
#endif
}

bool SaturationThisFrameIos()
{
    return GLRender::m_bSaturationEnabled;
}

// Latches the device-version verdict; version 0 counts as recent.
bool IosIsRecentDeviceVersion()
{
    if (s_nRecentDeviceVersion == -1)
    {
        int nWidth;
        int nHeight;
        if (IosIsIphone(nWidth, nHeight))
        {
            int nVersion = ASLGetIPhoneVersion();
            s_nRecentDeviceVersion = (nVersion > 54) || (nVersion == 0);
        }
        else
        {
            int nVersion = ASLGetIPadVersion();
            s_nRecentDeviceVersion = (nVersion > 40) || (nVersion == 0);
        }
    }
    return s_nRecentDeviceVersion == 1;
}

bool IosBloomEnabled()
{
#if KOTOR1 // K1: the frame-buffer switch
    return doframebuffer != 0;
#else
    return true;
#endif
}

#if KOTOR1
void SetupScreenViewport(int nDivisor)
{
    android_port_glViewportRaw(0, 0, g_SDL_mode.w / nDivisor, g_SDL_mode.h / nDivisor);
}

bool IosShadowsEnabled()
{
    return AurQueryShadowsEnabled();
}
#endif

void FrameBufferModificationsStartIos(bool bEnable)
{
#if KOTOR1 // K1: gated on the argument and the four effect switches; the clear colour is kept
    if (bEnable && (SaturationThisFrameIos() || IosBloomEnabled() || GLRender::m_bFilmNoiseEnabled ||
                    IosAntiAliasOn()))
    {
        PBUFFERARB_IOS* pPBuffer = s_pPBuffer;
        ASLgl::glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &s_nSavedFramebuffer);
        glBindFramebufferOES(GL_FRAMEBUFFER, pPBuffer->m_nFramebuffer);
        float afClearColor[4];
        ASLgl::glGetFloatv(GL_COLOR_CLEAR_VALUE, afClearColor);
        ASLgl::glClearDepth(1.0);
        ASLgl::glClearStencil(0);
        ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        ASLgl::glClearColor(afClearColor[0], afClearColor[1], afClearColor[2], afClearColor[3]);
    }
#else
    K2_UNUSED(bEnable);
    if (!s_frameBufferModificationsStarted)
        return;
    PBUFFERARB_IOS* pPBuffer = s_pPBuffer;
    ASLgl::glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &s_nSavedFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, pPBuffer->m_nFramebuffer);
    ASLgl::glClearDepth(1.0);
    ASLgl::glClearStencil(0);
    ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
#endif
}

// Draws the full-screen modification quad with film noise, the distort pbuffer and the distortion
// texture.
void RenderModificationQuad()
{
#if KOTOR1 // K1: the film-noise unit only (no distort pbuffer, no force-distortion unit); no setup object
    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glDepthMask(0);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    GLRender::cm_pFilmNoiseTexture->Bind();

    GLRender::SetActiveTexture(0);

    ASLgl::glBegin(7);
    {
        float fOpaque = 1.0f;
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(0.0f, 1.0f);
        ASLgl::glVertex4f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(0.0f, 0.0f);
        ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(1.0f, 0.0f);
        ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f, 1.0f);
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(1.0f, 1.0f);
        ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
    }
    ASLgl::glEnd();

    GLRender::SetActiveTexture(1);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);

    GLRender::SetActiveTexture(0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);

    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapLighting);
    ASLgl::glDepthMask(1);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
#else
    RenderModificationQuadSetup setup;

    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    GLRender::cm_pFilmNoiseTexture->Bind();

    GLRender::SetActiveTexture(0);

    GLRender::SetActiveTexture(2);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, s_pDistortPBuffer->m_nTexture);

    GLRender::SetActiveTexture(0);

    GLRender::SetActiveTexture(3);
    ASLgl::glEnable(GL_TEXTURE_2D);
    GLRender::cm_pForceDistortionTexture->Bind();

    GLRender::SetActiveTexture(0);

    ASLgl::glBegin(7);
    {
        float fOpaque = 1.0f;
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(0.0f, 1.0f);
        ASLgl::glVertex4f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(0.0f, 0.0f);
        ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(1.0f, 0.0f);
        ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f, 1.0f);
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(1.0f, 1.0f);
        ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
    }
    ASLgl::glEnd();

    GLRender::SetActiveTexture(1);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);

    GLRender::SetActiveTexture(2);
    ASLgl::glDisable(GL_TEXTURE_2D);

    GLRender::SetActiveTexture(3);
    ASLgl::glDisable(GL_TEXTURE_2D);

    GLRender::SetActiveTexture(0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
#endif
}

#if KOTOR1 && defined(K2_QOL_K1_DISTORTION)
// Scene::DoDistortionBuckets: the force-push bucket stamped stencil bit 0 into the scene pbuffer.
bool g_bK1ForceDistortion;
static unsigned int s_nK1DistortCopyTexture;
static int s_nK1DistortCopyWidth;
static int s_nK1DistortCopyHeight;

// The PC's force-distortion pass (FrameBufferModifications bit 0x20, RenderForceDistortion) on the
// scene pbuffer: a copy of the scene, re-drawn where stencil bit 0 is set, read through
// distortiontex (the port's program 0x100, k1kotorgl.cpp).
static void K1RenderForceDistortion()
{
    int nWidth = s_pPBuffer->m_nWidth;
    int nHeight = s_pPBuffer->m_nHeight;

    GLRender::SetActiveTexture(0);
    if (s_nK1DistortCopyTexture == 0 || s_nK1DistortCopyWidth != nWidth || s_nK1DistortCopyHeight != nHeight)
    {
        if (s_nK1DistortCopyTexture == 0)
            ASLgl::glGenTextures(1, &s_nK1DistortCopyTexture);
        ASLgl::glBindTexture(GL_TEXTURE_2D, s_nK1DistortCopyTexture);
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        s_nK1DistortCopyWidth = nWidth;
        s_nK1DistortCopyHeight = nHeight;
    }
    ASLgl::glBindTexture(GL_TEXTURE_2D, s_nK1DistortCopyTexture);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, nWidth, nHeight);
    android_port_glViewportRaw(0, 0, nWidth, nHeight);

    GLRender::SetActiveTexture(2);
    ASLgl::glEnable(GL_TEXTURE_2D);
    GLRender::cm_pForceDistortionTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(0);

    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPortProgramDistortion);
    GLRender::Enable(Aurora::kCapStencilTest); // GL_STENCIL_TEST
    ASLgl::glStencilFunc(GL_LESS, 0, 1);
    ASLgl::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    RenderModificationQuad();
    GLRender::Disable(Aurora::kCapStencilTest);

    GLRender::SetActiveTexture(2);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    GLRender::SetActiveTexture(0);
}
#endif

#if KOTOR1 && defined(K2_QOL_K1_SPEED_BLUR)
extern int preferPerfToVisual;
static unsigned int s_nK1SpeedBlurTexture;
static int s_nK1SpeedBlurWidth;
static int s_nK1SpeedBlurHeight;
static bool s_bK1SpeedBlurHistory;   // the texture holds the last blurred frame

// Applies the PC's Force Speed blur (FrameBufferModifications bit 0x10: RenderAccumulationBlur,
// then RenderFrameToTexture) on the scene pbuffer: the last blurred frame is laid over this one at
// m_fAccumulationRatio (the port's program 0x101), and the result is kept for the next frame.
static void K1RenderSpeedBlur()
{
    if (!GLRender::m_bSpeedBlurEnabled || preferPerfToVisual)
    {
        s_bK1SpeedBlurHistory = false;
        return;
    }
    int nWidth = s_pPBuffer->m_nWidth;
    int nHeight = s_pPBuffer->m_nHeight;

    GLRender::SetActiveTexture(0);
    if (s_nK1SpeedBlurTexture == 0 || s_nK1SpeedBlurWidth != nWidth || s_nK1SpeedBlurHeight != nHeight)
    {
        if (s_nK1SpeedBlurTexture == 0)
            ASLgl::glGenTextures(1, &s_nK1SpeedBlurTexture);
        ASLgl::glBindTexture(GL_TEXTURE_2D, s_nK1SpeedBlurTexture);
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        s_nK1SpeedBlurWidth = nWidth;
        s_nK1SpeedBlurHeight = nHeight;
        s_bK1SpeedBlurHistory = false;
    }
    android_port_glViewportRaw(0, 0, nWidth, nHeight);

    if (s_bK1SpeedBlurHistory)
    {
        float afRatio[4] = { GLRender::m_fAccumulationRatio, 0.0f, 0.0f, 0.0f };
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, kPortProgramSpeedBlur);
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afRatio);
        ASLgl::glBindTexture(GL_TEXTURE_2D, s_nK1SpeedBlurTexture);
        GLRender::Enable(Aurora::kCapBlend);
        ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        RenderModificationQuad();
        GLRender::Disable(Aurora::kCapBlend);
    }

    ASLgl::glBindTexture(GL_TEXTURE_2D, s_nK1SpeedBlurTexture);
    ASLgl::glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, nWidth, nHeight);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    s_bK1SpeedBlurHistory = true;
}
#endif

// Tears down the iOS post-process with one pass per enabled effect, then restores the app
// framebuffer.
void FrameBufferModificationsEndIos(bool bEnable)
{
#if KOTOR1 // K1: the StartIos gate, bloom passes only with the frame-buffer switch, no dream/fury/speed-blur; the viewport is restored
    int   anViewport[4];
    float afParam0[4];
    float afParam1[4];
    float afParam3[4];
    float afParam2[4];
#ifdef K2_QOL_K1_DISTORTION
    bool bForceDistortion = g_bK1ForceDistortion;
    g_bK1ForceDistortion = false;
#endif
    if (bEnable && (SaturationThisFrameIos() || IosBloomEnabled() || GLRender::m_bFilmNoiseEnabled ||
                    IosAntiAliasOn()))
    {
        GLRender::GetViewPort(anViewport);
        GLRender::Disable(Aurora::kCapBlend);
#ifdef K2_QOL_K1_DISTORTION
        if (bForceDistortion && GLRender::cm_pForceDistortionTexture)
            K1RenderForceDistortion();
#endif
#ifdef K2_QOL_K1_SPEED_BLUR
        K1RenderSpeedBlur();
#endif
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostPresent);

        afParam0[0] = GLRender::m_vSaturationMultiplier.x;
        afParam0[1] = GLRender::m_vSaturationMultiplier.y;
        afParam0[2] = GLRender::m_vSaturationMultiplier.z;
        afParam0[3] = GLRender::m_bSaturationEnabled ? 1.0f - GLRender::m_fSaturation : 0.0f;
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afParam0);
        afParam1[0] = 0.0f;
        afParam1[1] = 0.0f;
        afParam1[2] = 1.0f;
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);

#ifdef K2_QOL_K1_ANTIALIAS
        if (K1BloomThisFrame())
#else
        if (IosBloomEnabled())
#endif
        {
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
            int nHalfWidth = g_SDL_mode.w / 2;
#else
            static int nHalfWidth = g_SDL_mode.w / 2;
            static int nHalfHeight = g_SDL_mode.h / 2;
#endif

            _aglRestoreContext();
            _aglSetCurrentPBuffer(s_pHalfPBuffer1);
            android_port_glViewportRaw(0, 0, s_pHalfPBuffer1->m_nWidth, s_pHalfPBuffer1->m_nHeight);
            ASLgl::glBindTexture(GL_TEXTURE_2D, s_pPBuffer->m_nTexture);
            afParam1[1] = g_afIosPostParams[0];
            ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);
            RenderModificationQuad();
            afParam1[1] = 0.0f;
            ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);

            float fScale = g_afIosPostParams[1] * (1.0f / static_cast<float>(nHalfWidth));
            _aglRestoreContext();
            _aglSetCurrentPBuffer(s_pHalfPBuffer2);
            ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostBlur);
            afParam2[0] = 0.0f;
            afParam2[1] = 0.0f;
            afParam2[2] = 0.0f;
            afParam2[3] = 1.0f;
            ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afParam2);
            afParam3[0] = fScale;
            afParam3[1] = 0.0f;
            afParam3[2] = 1.0f;
            afParam3[3] = 1.0f;
            ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam3);
            ASLgl::glBindTexture(GL_TEXTURE_2D, s_pHalfPBuffer1->m_nTexture);
            RenderModificationQuad();
            ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostPresent);

            fScale = g_afIosPostParams[1] * (1.0f / static_cast<float>(nHalfWidth));
            _aglRestoreContext();
            _aglSetCurrentPBuffer(s_pHalfPBuffer1);
            ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostBlur);
            afParam2[0] = g_fIosBloomAmount;
            afParam2[1] = g_fIosBloomAmount;
            afParam2[2] = g_fIosBloomAmount;
            afParam2[3] = g_afIosPostParams[3];
            ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afParam2);
            afParam3[0] = 0.0f;
            afParam3[1] = fScale;
            afParam3[2] = g_afIosPostParams[2];
            afParam3[3] = g_afIosPostParams[3];
            ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam3);
            ASLgl::glBindTexture(GL_TEXTURE_2D, s_pHalfPBuffer2->m_nTexture);
            RenderModificationQuad();
            ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostPresent);
        }

        afParam0[0] = GLRender::m_vSaturationMultiplier.x;
        afParam0[1] = GLRender::m_vSaturationMultiplier.y;
        afParam0[2] = GLRender::m_vSaturationMultiplier.z;
        afParam0[3] = GLRender::m_bSaturationEnabled ? 1.0f - GLRender::m_fSaturation : 0.0f;
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afParam0);
        afParam1[0] = (GLRender::m_bScanNoiseEnabled || GLRender::m_bFilmNoiseEnabled) ? 1.0f : 0.0f;
        float fGamma = g_fGammaExponent;
        if (static_cast<double>(g_fGammaExponent) >= 0.98 && static_cast<double>(g_fGammaExponent) <= 1.02)
            g_fGammaExponent = 1.0f;
        afParam1[2] = (fGamma + 1.0f) * 0.5f;
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);

        _aglRestoreContext();
        if (IosAntiAliasOn())
            ASLgl::glViewport(0, 0, g_SDL_mode.w, g_SDL_mode.h);
        android_port_glViewportRaw(0, 0, g_SDL_mode.w, g_SDL_mode.h);
        ASLgl::glBindTexture(GL_TEXTURE_2D, s_pPBuffer->m_nTexture);
        RenderModificationQuad();
#ifdef K2_QOL_K1_ANTIALIAS
        if (K1BloomThisFrame())
#else
        if (IosBloomEnabled())
#endif
        {
            GLRender::Enable(Aurora::kCapBlend);
            ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
            ASLgl::glBindTexture(GL_TEXTURE_2D, s_pHalfPBuffer1->m_nTexture);
            RenderModificationQuad();
            GLRender::Disable(Aurora::kCapBlend);
        }
        if (!IosAntiAliasOn())
            ASLgl::glViewport(anViewport[0], anViewport[1], anViewport[2], anViewport[3]);
    }
#ifdef K2_QOL_K1_SPEED_BLUR
    else
        s_bK1SpeedBlurHistory = false;
#endif
#else
    K2_UNUSED(bEnable);
    int   anViewport[4];

    if (!s_frameBufferModificationsStarted)
        return;
    s_frameBufferModificationsStarted = false;
    GLRender::GetViewPort(anViewport);

    if (GLRender::m_bDreamDistortEnabled)
        RenderDistortIos(GLRender::cm_pForceDistortionTexture, GLRender::m_pDreamDistortTexture);

    if (GLRender::m_FuryLevel)
    {
        RenderModificationQuadSetup setup;

        GLRender::SetActiveTexture(0);
        ASLgl::glEnable(GL_TEXTURE_2D);
        GLRender::m_pFuryTexture->Bind();
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);   // GL_TEXTURE_WRAP_S = GL_CLAMP_TO_EDGE
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);   // GL_TEXTURE_WRAP_T = GL_CLAMP_TO_EDGE
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // GL_TEXTURE_MAG_FILTER = GL_LINEAR
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   // GL_TEXTURE_MIN_FILTER = GL_LINEAR

        ASLgl::glBegin(7);
        {
            float fOpaque = 1.0f;
            ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
            ASLgl::glTexCoord2f(0.0f, 1.0f);
            ASLgl::glVertex4f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
            ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
            ASLgl::glTexCoord2f(0.0f, 0.0f);
            ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
            ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
            ASLgl::glTexCoord2f(1.0f, 0.0f);
            ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f, 1.0f);
            ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
            ASLgl::glTexCoord2f(1.0f, 1.0f);
            ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
        }
        ASLgl::glEnd();
        RenderDistortIos(GLRender::cm_pForceDistortionTexture, GLRender::m_pFuryTexture);
    }

    GLRender::Disable(Aurora::kCapBlend);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostBloomSource);

#ifdef K2_QOL_FRAME_BUFFER_EFFECTS
    // Frame Buffer Effects off: no bloom and no Force Speed blur, as on the PC (its
    // FrameBufferModifications are the switch); the present, the noise and the saturation stay.
    const bool bEffects = doframebuffer != 0;
#endif
    float afParam0[4];
    float afParam1[4];
#ifdef K2_QOL_FRAME_BUFFER_EFFECTS
    afParam1[1] = 0.0f;   // what the present reads of the bloom passes' parameters
    afParam1[3] = 1.0f;
    // The bloom passes start by leaving the scene pbuffer (_aglRestoreContext); without them the
    // next _aglSetCurrentPBuffer would save the scene pbuffer as the one to come back to, and the
    // present would draw into it.
    if (!bEffects)
        _aglRestoreContext();
    else
#endif
    {
    float afParam2[4];
    int   nPhoneWidth;
    int   nPhoneHeight;
    afParam0[0] = GLRender::m_vSaturationMultiplier.x;
    afParam0[1] = GLRender::m_vSaturationMultiplier.y;
    afParam0[2] = GLRender::m_vSaturationMultiplier.z;
    afParam0[3] = GLRender::m_bSaturationEnabled ? 1.0f - GLRender::m_fSaturation : 0.0f;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afParam0);

    float fNoise = (GLRender::m_bScanNoiseEnabled || GLRender::m_bFilmNoiseEnabled) ? 1.0f : 0.3f;
    afParam1[0] = fNoise;
    afParam1[1] = 0.0f;
    afParam1[2] = 1.0f;
    afParam1[3] = 1.0f;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);

    bool bPhone = IosIsIphone(nPhoneWidth, nPhoneHeight);
    float fHalfScreen = bPhone ? static_cast<float>(nPhoneWidth / 2) : 512.0f;
    float fInvScale = 1.0f / fHalfScreen;
    float fDistortScale = fInvScale * g_fIosDistortScale;

    _aglRestoreContext();
    _aglSetCurrentPBuffer(s_pHalfPBuffer1);
    ASLgl::glViewport(0, 0, s_pHalfPBuffer1->m_nWidth, s_pHalfPBuffer1->m_nHeight);
    ASLgl::glBindTexture(GL_TEXTURE_2D, s_pPBuffer->m_nTexture);

    afParam1[1] = g_fIosBloomScaleU;
    afParam1[2] = g_fIosBloomScaleV;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);
    RenderModificationQuad();
    afParam1[1] = 0.0f;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);

    _aglRestoreContext();
    _aglSetCurrentPBuffer(s_pHalfPBuffer2);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostBlur);

    afParam2[0] = 0.0f;
    afParam2[1] = 0.0f;
    afParam2[2] = 0.0f;
    afParam2[3] = 1.0f;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afParam2);
    afParam1[0] = fDistortScale;
    afParam1[1] = 0.0f;
    afParam1[2] = 1.0f;
    afParam1[3] = 1.0f;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);

    ASLgl::glBindTexture(GL_TEXTURE_2D, s_pHalfPBuffer1->m_nTexture);
    RenderModificationQuad();

    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostBloomSource);
    fInvScale = fInvScale * g_fIosDistortScale;
    _aglRestoreContext();
    _aglSetCurrentPBuffer(s_pHalfPBuffer1);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostBlur);

    afParam2[0] = g_fIosBloomAmount;
    afParam2[1] = g_fIosBloomAmount;
    afParam2[2] = g_fIosBloomAmount;
    afParam2[3] = g_fIosDistortOffsetV;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afParam2);
    afParam1[0] = 0.0f;
    afParam1[1] = fInvScale;
    afParam1[2] = g_fIosDistortOffsetU;
    afParam1[3] = 1.0f;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);

    ASLgl::glBindTexture(GL_TEXTURE_2D, s_pHalfPBuffer2->m_nTexture);
    RenderModificationQuad();
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostBloomSource);
    _aglRestoreContext();
    }

#ifdef K2_QOL_FRAME_BUFFER_EFFECTS
    if (GLRender::m_bSpeedBlurEnabled && !g_ForceSight && bEffects)
#else
    if (GLRender::m_bSpeedBlurEnabled && !g_ForceSight)
#endif
    {
#ifdef K2_QOL_IMPROVEMENTS
        // The blend this pass turns on would otherwise stay on for the copy and present quads
        // below. Wherever nothing was drawn, the scene's alpha is the clear colour's 0, so those
        // pixels would never reach the screen and Force Speed would show black there.
        bool bBlendWasOn = GLRender::IsEnabled(Aurora::kCapBlend) != 0;
#endif
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSDefault);
        _aglSetCurrentPBuffer(s_pPBuffer);
        ASLgl::glBindTexture(GL_TEXTURE_2D, s_pHalfPBuffer3->m_nTexture);
        ASLgl::glViewport(0, 0, s_pPBuffer->m_nWidth, s_pPBuffer->m_nHeight);
        {
            RenderModificationQuadSetup setup;
            GLRender::Enable(Aurora::kCapBlend);
            ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            ASLgl::glBegin(7);
            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, GLRender::m_fAccumulationRatio);
            ASLgl::glTexCoord2f(-0.0f, 1.0f);
            ASLgl::glVertex4f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, GLRender::m_fAccumulationRatio);
            ASLgl::glTexCoord2f(-0.0f, 0.0f);
            ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, GLRender::m_fAccumulationRatio);
            ASLgl::glTexCoord2f(1.0f, 0.0f);
            ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f, 1.0f);
            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, GLRender::m_fAccumulationRatio);
            ASLgl::glTexCoord2f(1.0f, 1.0f);
            ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
            ASLgl::glEnd();
        }
#ifdef K2_QOL_IMPROVEMENTS
        if (!bBlendWasOn)
            GLRender::Disable(Aurora::kCapBlend);
#endif

        _aglRestoreContext();
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    }

    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSDefault);
    ASLgl::glViewport(0, 0, s_pHalfPBuffer3->m_nWidth, s_pHalfPBuffer3->m_nHeight);
    _aglSetCurrentPBuffer(s_pHalfPBuffer3);
    ASLgl::glBindTexture(GL_TEXTURE_2D, s_pPBuffer->m_nTexture);
    RenderModificationQuad();
    _aglRestoreContext();
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostPresent);

    afParam0[0] = GLRender::m_vSaturationMultiplier.x;
    afParam0[1] = GLRender::m_vSaturationMultiplier.y;
    afParam0[2] = GLRender::m_vSaturationMultiplier.z;
    afParam0[3] = GLRender::m_bSaturationEnabled ? 1.0f - GLRender::m_fSaturation : 0.0f;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, afParam0);

    float fGammaNoise = (GLRender::m_bScanNoiseEnabled || GLRender::m_bFilmNoiseEnabled)
                            ? 1.0f : 0.3f;
    float fGamma = g_fGammaExponent;
    if (static_cast<double>(g_fGammaExponent) >= 0.98 && static_cast<double>(g_fGammaExponent) <= 1.02)
        g_fGammaExponent = 1.0f;
    afParam1[0] = fGammaNoise;
    afParam1[2] = fGamma * 0.67f + 0.33f;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam1);

    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glBindTexture(GL_TEXTURE_2D, s_pPBuffer->m_nTexture);
    RenderModificationQuad();
#ifdef K2_QOL_FRAME_BUFFER_EFFECTS
    if (!bEffects)
        return;
#endif
    GLRender::Enable(Aurora::kCapBlend);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    ASLgl::glBindTexture(GL_TEXTURE_2D, s_pHalfPBuffer1->m_nTexture);
    RenderModificationQuad();
    GLRender::Disable(Aurora::kCapBlend);
#endif
}

// Runs the distort pass, two units staged as in the modification quad, with the same full-screen
// quad.
void RenderDistortIos(CAurTexture* pSrc, CAurTexture* pDst)
{
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostDistort);

    RenderModificationQuadSetup setup;

    GLRender::SetActiveTexture(3);
    pSrc->Bind();

    GLRender::SetActiveTexture(0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, s_pHalfPBuffer3->m_nTexture);

    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    pDst->Bind();

    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);   // GL_TEXTURE_WRAP_S = GL_CLAMP_TO_EDGE
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);   // GL_TEXTURE_WRAP_T = GL_CLAMP_TO_EDGE
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // GL_TEXTURE_MAG_FILTER = GL_LINEAR
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   // GL_TEXTURE_MIN_FILTER = GL_LINEAR

    ASLgl::glBegin(7);
    {
        float fOpaque = 1.0f;
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(0.0f, 1.0f);
        ASLgl::glVertex4f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(0.0f, 0.0f);
        ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(1.0f, 0.0f);
        ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f, 1.0f);
        ASLgl::glColor4f(fOpaque, fOpaque, fOpaque, fOpaque);
        ASLgl::glTexCoord2f(1.0f, 1.0f);
        ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
    }
    ASLgl::glEnd();

    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, GLRender::kPSPostBloomSource);
}


// Draws the ARB accumulation-blur composite. The source is not the argument, and the quad colour is
// magenta.
void GLRender::RenderAccumulationBlurARB(unsigned int nTexture)
{
    K2_UNUSED(nTexture);
#if !KOTOR1 // K1: an empty body
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glBlendFunc(1, 0);

    Vector vRatio(m_fAccumulationRatio, m_fAccumulationRatio, m_fAccumulationRatio);

    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nTargetTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);

    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, Homogenize(vRatio, 1.0f));
    Vector vInvRatio(1.0f - vRatio.x, 1.0f - vRatio.y, 1.0f - vRatio.z);
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, Homogenize(vInvRatio, 0.0f));

    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nAccumulationBlurShaderARB);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 0.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, 0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 1.0f, 0.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 0.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glEnd();
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    GLRender::SetActiveTexture(0);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
#endif
}

// The ARB build of both the dream-distortion and fury composites; the second argument picks the texture.
void GLRender::RenderDreamDistortionARB(unsigned int nTexture, bool bFury)
{
    K2_UNUSED(nTexture);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glDisable(GL_DEPTH_TEST);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapBlend);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pForceDistortionTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(1);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nAccumulationBlur);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE3);
    ASLgl::glEnable(GL_TEXTURE_2D);
    if (bFury)
        m_pFuryTexture->Bind();
    else
        m_pDreamDistortTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 7, s_afDreamDistortParam);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDreamDistortShaderARB);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glBegin(7);

    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
        float fU = (i < 2) ? 0.0f : 1.0f;
        float fV = (i == 0 || i == 3) ? 1.0f : 0.0f;
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, fU, fV);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE1, 0.15625f, 0.0f, fU);
        ASLgl::glMultiTexCoord3fARB(GL_TEXTURE2, 0.0f, 0.15625f, fV);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE3, fU, fV);

        if (g_bLetterBoxMode)
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_Letterbox_bottom) + 25.0f
                                          : static_cast<float>(g_Letterbox_top) - 25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
        else
        {
            float fX = (i < 2) ? -25.0f : static_cast<float>(g_nScreenWidth) + 25.0f;
            float fY = (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) + 25.0f : -25.0f;
            ASLgl::glVertex3f(fX, fY, 0.0f);
        }
    }

    ASLgl::glEnd();
    ASLgl::glFlush();
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
}

static const char szOverbrightenFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "TEMP r0, r1, r2, r3;\n"
    "PARAM c[3] = { program.env[0..2] };\n"
    "TEX r0, T0, texture[0], RECT;\n"
    "DP3 r1.xyz, r0, c[0];\n"
    "DP3 r2.xyz, r0, c[1];\n"
    "DP3 r3.xyz, r0, c[2];\n"
    "TEX r1, r1, texture[1], 1D;\n"
    "TEX r2, r2, texture[2], 1D;\n"
    "TEX r3, r3, texture[3], 1D;\n"
    "ADD r0.xyz, r1, r2;\n"
    "ADD r0.xyz, r0, r3;\n"
    "MOV r0.w, v0;\n"
    "MOV result.color, r0;\n"
    "END\n"
    ""
    ;

static const char szForceDistortFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "TEMP r0, r1, r2, r4;\n"
    "TEMP rTemp1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "MOV r1, T1;\n"
    "MOV r2, T2;\n"
    "MAD  rTemp1,  r0, 2.0, -1.0;\n"
    "DP3 r4.x, rTemp1, r1;\n"
    "MAD  rTemp1,  r0, 2.0, -1.0;\n"
    "DP3 r4.y, rTemp1, r2;\n"
    "TEX r2, r4, texture[2], RECT;\n"
    "MOV r0.xyz, r2;\n"
    "MOV result.color, r0;\n"
    "END\n"
    ""
    ;

static const char szFilmNoiseFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "TEMP r0, r1;\n"
    "TEX r0, T0, texture[0], RECT;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "ADD r0.xyz, r0, r1;\n"
    "MOV r0.a, 1.0;\n"
    "MOV result.color, r0;\n"
    "END"
    ;

static const char szAccumulationBlurFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "TEMP r0, r1, r2;\n"
    "PARAM c[2] = { program.env[0..1] };\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], RECT;\n"
    "MUL r2.xyz, r0, c[0];\n"
    "MAD r0.xyz, r1, c[1], r2;\n"
    "MOV r0.w, c[0];\n"
    "MOV result.color, r0;\n"
    "END"
    ;

static const char szSaturationFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "TEMP r0, r1, r2;\n"
    "PARAM c[6] = { program.env[0..5] };\n"
    "TEX r0, T0, texture[0], RECT;\n"
    "DP3 r1.xyz, r0, c[0];\n"
    "DP3 r2.xyz, r0, c[1];\n"
    "MUL r1.xyz, r1, c[3];\n"
    "MAD r1.xyz, r2, c[4], r1;\n"
    "DP3 r2.xyz, r0, c[2];\n"
    "MAD r0.xyz, r2, c[5], r1;\n"
    "MOV result.color, r0;\n"
    "END"
    ;

static const char szPerPixelFlareFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "TEMP r0, r1, r2, r3;\n"
    "PARAM c[3] = { program.env[0..2] };\n"
    "TEX r0, T0, texture[0], RECT;\n"
    "DP3 r1.xyz, r0, c[0];\n"
    "DP3 r2.xyz, r0, c[1];\n"
    "DP3 r3.xyz, r0, c[2];\n"
    "TEX r1, r1, texture[1], 1D;\n"
    "TEX r2, r2, texture[2], 1D;\n"
    "TEX r3, r3, texture[3], 1D;\n"
    "ADD r0.xyz, r1, r2;\n"
    "ADD r0.xyz, r0, r3;\n"
    "MOV r0.a, 1.0;\n"
    "MOV result.color, r0;\n"
    "END"
    ;

static const char szDecalFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "TEMP r0;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "MOV r0.xyz, r0;\n"
    "MOV r0.a, 1.0;\n"
    "MOV result.color, r0;\n"
    "END"
    ;

static const char szDecalRectFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "TEMP r0;\n"
    "TEX r0, T0, texture[0], RECT;\n"
    "MOV r0.xyz, r0;\n"
    "MOV r0.a, 1.0;\n"
    "MOV result.color, r0;\n"
    "END"
    ;

static const char sz4DecalFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "TEMP r0, r1, r2, r3;\n"
    "PARAM c[4] = { program.env[0..3] };\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "TEX r1, T1, texture[1], 2D;\n"
    "TEX r2, T2, texture[2], 2D;\n"
    "TEX r3, T3, texture[3], 2D;\n"
    "MUL r0.xyz, r0, c[0];\n"
    "MAD r1.xyz, r1, c[1], r0;\n"
    "MAD r2.xyz, r2, c[2], r1;\n"
    "MAD r0.xyz, r3, c[3], r2;\n"
    "MOV r0.w, v0.w;\n"
    "MOV result.color, r0;\n"
    "END"
    ;

#if !KOTOR1
static const char szDreamDistortFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "ATTRIB T1 = fragment.texcoord[1];\n"
    "ATTRIB T2 = fragment.texcoord[2];\n"
    "ATTRIB T3 = fragment.texcoord[3];\n"
    "PARAM c7 = program.env[7];\n"
    "PARAM oneHalf = {0.500001, 0.500001, 0.500001, 0.500001};\n"
    "TEMP r0, r1, r2, r3, r4, rTemp1;\n"
    "TEX r0, T0, texture[0], 2D;\n"
    "MOV r1, T1;\n"
    "MOV r2, T2;\n"
    "MAD  rTemp1,  r0, 2.0, -1.0;\n"
    "DP3 r4.x, rTemp1, r1;\n"
    "MAD  rTemp1,  r0, 2.0, -1.0;\n"
    "DP3 r4.y, rTemp1, r2;\n"
    "TEX r2, r4, texture[2], 2D;\n"
    "TEX r3, T3, texture[3], 2D;\n"
    "ADD r0.a, r3.a, c7.a;\n"
    "SGE r1.a, r0, oneHalf;\n"
    "MAD r0.rgb, r2, r1.a, r3;\n"
    "MOV r0.a, r3.a;\n"
    "MOV result.color, r0;\n"
    "END\n"
    ""
    ;
#endif

// Initialises the ten ARB post-process fragment programs, in their string-address order.
void GLRender::InitializeFrameBufferModificationFragmentShadersARB()
{
    int nErrorPos;
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glGenProgramsARB(1, &cm_nOverbrightenShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nOverbrightenShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szOverbrightenFrag, sizeof(szOverbrightenFrag))), szOverbrightenFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &cm_nForceDistortionShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nForceDistortionShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szForceDistortFrag, sizeof(szForceDistortFrag))), szForceDistortFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &cm_nFilmNoiseShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nFilmNoiseShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szFilmNoiseFrag, sizeof(szFilmNoiseFrag))), szFilmNoiseFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &cm_nAccumulationBlurShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nAccumulationBlurShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szAccumulationBlurFrag, sizeof(szAccumulationBlurFrag))), szAccumulationBlurFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &cm_nSaturationShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSaturationShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szSaturationFrag, sizeof(szSaturationFrag))), szSaturationFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &cm_nPerPixelFlareShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nPerPixelFlareShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szPerPixelFlareFrag, sizeof(szPerPixelFlareFrag))), szPerPixelFlareFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &cm_nDecalShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szDecalFrag, sizeof(szDecalFrag))), szDecalFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &cm_nDecalShaderRectARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderRectARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szDecalRectFrag, sizeof(szDecalRectFrag))), szDecalRectFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
    ASLgl::glGenProgramsARB(1, &cm_n4DecalShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_n4DecalShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(sz4DecalFrag, sizeof(sz4DecalFrag))), sz4DecalFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
#if !KOTOR1 // K1: no dream-distortion program
    ASLgl::glGenProgramsARB(1, &cm_nDreamDistortShaderARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDreamDistortShaderARB);
    ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, static_cast<int>(__strlen_chk(szDreamDistortFrag, sizeof(szDreamDistortFrag))), szDreamDistortFrag);
    ASLgl::glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &nErrorPos);
#endif
    ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

// Renders the given texture into the 128x128 accumulation target through the rect decal program.
void GLRender::RenderFrameToTextureARB(unsigned int nTexture)
{
#if KOTOR1 // K1: renders into the accumulation pbuffer, GL_TEXTURE_2D source
    _aglSetCurrentPBuffer(static_cast<PBUFFERARB*>(cm_hAccumulationPBuffer));
#else
    glBindFramebuffer(GL_FRAMEBUFFER, s_AccumulationFBO);
#endif
    ASLgl::glViewport(0, 0, 128, 128);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, 128.0, 0.0, 128.0, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderRectARB);
    ASLgl::glDisable(GL_DEPTH_TEST);
#if KOTOR1
    android_port_glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
#else
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, nTexture);
#endif
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, 128.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(128.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(128.0f, 128.0f, 0.0f);
    ASLgl::glEnd();
#if KOTOR1
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
    _aglRestoreContext();
#else
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

// Draws the heat-haze ARB composite, the plain distortion pass's three-unit walk, driving both
// programs.
void GLRender::RenderForceDistortionARB(unsigned int nTexture)
{
#if KOTOR1
    K2_UNUSED(nTexture);
#endif
#if !KOTOR1 // K1: an empty body
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glEnable(GL_VERTEX_PROGRAM_ARB);
    ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 15, 0.0f, 1.0f, 0.0f, 0.0f);
    ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, cm_pHeatDistortionVertexProg);
    cm_pForceDistortionTexture->GetWidth();
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(1, 0);
    GLRender::SetActiveTexture(0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pForceDistortionTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glActiveTextureARB(GL_TEXTURE1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE2);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nForceDistortionShaderARB);
    GLRender::Enable(Aurora::kCapStencilTest); // GL_STENCIL_TEST
    ASLgl::glStencilFunc(GL_LESS, 0, 1);
    ASLgl::glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBegin(7);

    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, (i < 2) ? 0.0f : 1.0f,
                                    (i == 0 || i == 3) ? 0.0f : 1.0f);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, static_cast<float>(g_nScreenWidth) / 10.0f,
                                    static_cast<float>(g_nScreenHeight) / 10.0f);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE2, (i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                                    (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f);
        ASLgl::glVertex3f((i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                          (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f, 0.0f);
    }

    ASLgl::glEnd();
    GLRender::Disable(Aurora::kCapStencilTest);
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
    ASLgl::glDisable(GL_VERTEX_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
    GLRender::SetActiveTexture(0);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#endif
}

// Draws the ARB saturation composite from the rect-texture source, with the saturation matrix as
// env parameters.
void GLRender::RenderSaturationAdjustmentARB(unsigned int nTexture)
{
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(1, 0);
#if KOTOR1 // K1: GL_TEXTURE_2D source
    android_port_glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 3, s_afSaturationBasisR);
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 4, s_afSaturationBasisG);
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 5, s_afSaturationBasisB);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nSaturationShaderARB);

    Vector vColumn0, vColumn1, vColumn2;
    float fInvSaturation = 1.0f - m_fSaturation;

    vColumn0.x = (m_fSaturation + fInvSaturation * 0.3086f) * m_vSaturationMultiplier.x;
    vColumn0.y = fInvSaturation * 0.6094f * m_vSaturationMultiplier.x;
    vColumn0.z = fInvSaturation * 0.0820f * m_vSaturationMultiplier.x;
    vColumn1.x = fInvSaturation * 0.3086f * m_vSaturationMultiplier.y;
    vColumn1.y = (m_fSaturation + fInvSaturation * 0.6094f) * m_vSaturationMultiplier.y;
    vColumn1.z = fInvSaturation * 0.0820f * m_vSaturationMultiplier.y;
    vColumn2.x = fInvSaturation * 0.3086f * m_vSaturationMultiplier.z;
    vColumn2.y = fInvSaturation * 0.6094f * m_vSaturationMultiplier.z;
    vColumn2.z = (m_fSaturation + fInvSaturation * 0.0820f) * m_vSaturationMultiplier.z;

    vColumn0.x = vColumn0.x > 1.0f ? 1.0f : vColumn0.x;
    vColumn0.y = vColumn0.y > 1.0f ? 1.0f : vColumn0.y;
    vColumn0.z = vColumn0.z > 1.0f ? 1.0f : vColumn0.z;
    vColumn1.x = vColumn1.x > 1.0f ? 1.0f : vColumn1.x;
    vColumn1.y = vColumn1.y > 1.0f ? 1.0f : vColumn1.y;
    vColumn1.z = vColumn1.z > 1.0f ? 1.0f : vColumn1.z;
    vColumn2.x = vColumn2.x > 1.0f ? 1.0f : vColumn2.x;
    vColumn2.y = vColumn2.y > 1.0f ? 1.0f : vColumn2.y;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, Homogenize(vColumn0, 0.0f));
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, Homogenize(vColumn1, 0.0f));
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 2, Homogenize(vColumn2, 0.0f));
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glEnd();
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
#if KOTOR1
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
#else
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
#endif
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
}

// Runs the overbrighten pass with the given texture plus the three ramp vectors.
void GLRender::RenderOverlayOverbrightenARB(unsigned int nTexture)
{
#if KOTOR1 // K1: a quarter-size quad with normalised texcoords on GL_TEXTURE_2D
    ASLgl::glBlendFunc(1, 0);
    FrameBufferModificationPixelShadersARB(kFBShaderOverbrighten,  nTexture, cm_nOverbrightenRed,
                                           cm_nOverbrightenGreen, cm_nOverbrightenBlue);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 1.0f);
    ASLgl::glVertex4f(0.0f, static_cast<float>(g_nScreenHeight) * 0.25f, 0.0f, 1.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 0.0f);
    ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth) * 0.25f, 0.0f, 0.0f, 1.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 1.0f);
    ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth) * 0.25f, static_cast<float>(g_nScreenHeight) * 0.25f, 0.0f, 1.0f);
    ASLgl::glEnd();
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    ASLgl::glDisable(GL_TEXTURE_2D);
#else
    ASLgl::glBlendFunc(1, 0);
    FrameBufferModificationPixelShadersARB(kFBShaderOverbrighten,  nTexture, cm_nOverbrightenRed,
                                           cm_nOverbrightenGreen, cm_nOverbrightenBlue);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex4f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex4f(0.0f, 0.0f, 0.0f, 1.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenHeight), 0.0f);
    ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f, 1.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(static_cast<float>(g_nScreenHeight), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex4f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f, 1.0f);
    ASLgl::glEnd();
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
#endif
}

// Draws the ARB convolution composite: the source pass, the downsample chain and the final blit.
void GLRender::RenderOverlayConvolutionARB(unsigned int nTexture)
{
#if KOTOR1
    K2_UNUSED(nTexture);
#endif
#if !KOTOR1 // K1: an empty body
    if (!renderregular)
    {
        ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    }
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::Enable(Aurora::kCapBlend);
    glBindFramebuffer(GL_FRAMEBUFFER, s_ConvolutionFBOs[3]);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, 512.0, 0.0, 512.0, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, 512, 512);
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GLRender::Disable(Aurora::kCapLighting);
    FrameBufferModificationPixelShadersARB(kFBShaderFlare,  nTexture, cm_nRedTexture, cm_nGreenTexture,
                                          cm_nBlueTexture);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(0.0f, 512.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, static_cast<float>(g_nScreenWidth), 0.0f);
    ASLgl::glVertex3f(512.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight));
    ASLgl::glVertex3f(512.0f, 512.0f, 0.0f);

    ASLgl::glEnd();
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);

#ifdef K2_FIX_ENGINE_BUGS
    float fSize = 0.0f;
#else
    float fSize;
#endif
#ifdef K2_FIX_ENGINE_BUGS
    HDC__* hDC = nullptr;
    HGLRC__* hGLRC = nullptr;
    HPBUFFERARB__* pPBuffer = nullptr;
#else
    HDC__* hDC;
    HGLRC__* hGLRC;
    HPBUFFERARB__* pPBuffer;
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, s_ConvolutionFBOs[2]);
    MipmapFilterARB(hDC, hGLRC, 256.0f, 256.0f, pPBuffer, cm_nConvolutionRenderTexture[3], fSize);
    glBindFramebuffer(GL_FRAMEBUFFER, s_ConvolutionFBOs[1]);
    MipmapFilterARB(hDC, hGLRC, 128.0f, 128.0f, pPBuffer, cm_nConvolutionRenderTexture[2], fSize);
    glBindFramebuffer(GL_FRAMEBUFFER, s_ConvolutionFBOs[0]);
    MipmapFilterARB(hDC, hGLRC, 64.0f, 64.0f, pPBuffer, cm_nConvolutionRenderTexture[1], fSize);
    glBindFramebuffer(GL_FRAMEBUFFER, s_ConvolutionFBOs[5]);
    BloomFilterARB(hDC, hGLRC, 256.0f, 256.0f, pPBuffer, cm_nConvolutionRenderTexture[baselayer],
                   basesize, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, g_nScreenWidth, g_nScreenHeight);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, cm_nConvolutionRenderTexture[displaylayer]);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderARB);
    ASLgl::glBegin(7);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 1.0f);
    ASLgl::glVertex3f(0.0f, static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 0.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), 0.0f, 0.0f);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glTexCoord2f(1.0f, 1.0f);
    ASLgl::glVertex3f(static_cast<float>(g_nScreenWidth), static_cast<float>(g_nScreenHeight), 0.0f);

    ASLgl::glEnd();
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
#endif
}

#if KOTOR1
// K1: an empty stub that nothing calls (the pbuffer path does not exist there)
void GLRender::MipmapFilterARB(void* hDC, void* hGLRC, float fWidth, float fHeight, PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(fWidth); K2_UNUSED(fHeight); K2_UNUSED(pPBuffer); K2_UNUSED(nTexture); K2_UNUSED(fSize);
}
#else
// The downsampling pass that the convolution composites end their quad with.
void GLRender::MipmapFilterARB(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                               HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize)
{
    K2_UNUSED(hDC); K2_UNUSED(hGLRC); K2_UNUSED(pPBuffer); K2_UNUSED(fSize);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, 1);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(fWidth), 0.0, static_cast<double>(fHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    ASLgl::glViewport(0, 0, static_cast<int>(fWidth), static_cast<int>(fHeight));
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nDecalShaderARB);
    ASLgl::glEnable(GL_BLEND);
    ASLgl::glBegin(7);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 1.0f);
    ASLgl::glVertex3f(0.0f, fHeight, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 0.0f, 0.0f);
    ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 1.0f, 0.0f);
    ASLgl::glVertex3f(fWidth, 0.0f, 0.0f);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, 1.0f, 1.0f);
    ASLgl::glVertex3f(fWidth, fHeight, 0.0f);
    ASLgl::glEnd();
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glBindTexture(GL_TEXTURE_2D, 0);
}
#endif

// The ARB-shader build of the film-noise pass.
void GLRender::RenderOverlayFilmNoiseARB(unsigned int nTexture, bool bScanNoise)
{
#if KOTOR1
    K2_UNUSED(nTexture); K2_UNUSED(bScanNoise);
#endif
#if !KOTOR1 // K1: an empty body
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glBlendFunc(1, 0);
    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, nTexture);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLRender::SetActiveTexture(1);
    ASLgl::glEnable(GL_TEXTURE_2D);
    cm_pFilmNoiseTexture->Bind();
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, cm_nFilmNoiseShaderARB);
    ASLgl::glBegin(7);

    float fOffset = bScanNoise ? 0.0f : 7.5f;
    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE0, (i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                                    (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f);
        ASLgl::glMultiTexCoord2fARB(GL_TEXTURE1, (i < 2) ? fOffset : 0.0f,
                                    (i == 0 || i == 3) ? 0.0f : 7.5f);
        ASLgl::glVertex3f((i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                          (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f, 0.0f);
    }

    ASLgl::glEnd();
    FrameBufferModificationPixelShadersARB(kFBShaderOff,  0, 0, 0, 0);
    GLRender::SetActiveTexture(0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
#endif
}


struct RenderVertexV { // the walkmesh vertex-pool element, position only
    float pos[3];
};

struct RenderVertexC {        // the packed AABBGGRR colour token, indexed by material
    unsigned int color;
};

// Draws the face list directly, re-loading its buffer pointer after every GL call.
void GLRender::DrawTriangles(List<MaxFace>& faces, RenderVertexV* pVerts, RenderVertexC* pColors)
{
    ASLgl::glBegin(4);
    Vector v0, v1, v2;
    for (int64_t i = 0; i < faces.m_count; i++)
    {
        ASLgl::glColor4ubv(reinterpret_cast<const unsigned char*>(&pColors[faces.m_pData[i].m_nMaterial]));
        v0 = reinterpret_cast<const Vector*>(pVerts)[static_cast<unsigned int>(faces.m_pData[i].m_nVIndex[0])];
        v1 = reinterpret_cast<const Vector*>(pVerts)[static_cast<unsigned int>(faces.m_pData[i].m_nVIndex[1])];
        v2 = reinterpret_cast<const Vector*>(pVerts)[static_cast<unsigned int>(faces.m_pData[i].m_nVIndex[2])];
        Vector vNormal = NormalOf(v0, v1, v2);
        ASLgl::glNormal3fv(&vNormal.x);
        ASLgl::glVertex3fv(&v0.x);
        ASLgl::glVertex3fv(&v1.x);
        ASLgl::glVertex3fv(&v2.x);
    }
    ASLgl::glEnd();
}

// Draws the debug bound as a flat 8-gon in the XY plane (every vertex shares z), radius times
// alpha.
void GLRender::DrawSphereBound(float fRadius, Vector& vCenter, const Vector& vColor, float fAlpha,
                               bool bMask)
{
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glDisable(GL_LIGHTING);
    if (bMask)
    {
        float afMatrix[16];
        ASLgl::glGetFloatv(GL_MODELVIEW_MATRIX, afMatrix);

        afMatrix[12] = vCenter.x;
        afMatrix[13] = vCenter.y;
        afMatrix[14] = vCenter.z;
        float fTemp = afMatrix[1];
        afMatrix[1] = afMatrix[4];
        afMatrix[4] = fTemp;
        fTemp = afMatrix[2];
        afMatrix[2] = afMatrix[8];
        afMatrix[8] = fTemp;
        fTemp = afMatrix[6];
        afMatrix[6] = afMatrix[9];
        afMatrix[9] = fTemp;

        ASLgl::glPushMatrix();
        GLRender::SetMatrixMode(Aurora::kMatrixModelView);
        ASLgl::glMultMatrixf(afMatrix);
    }

    ASLgl::glColor3f(vColor.x, vColor.y, vColor.z);

    float fSize = fRadius * fAlpha;
    ASLgl::glBegin(2);
    ASLgl::glVertex3f(0.0f, fSize, 0.0f);
    ASLgl::glVertex3f(fSize * 0.717f, fSize * 0.717f, 0.0f);
    ASLgl::glVertex3f(fSize, 0.0f, 0.0f);
    ASLgl::glVertex3f(fSize * 0.717f, fSize * -0.717f, 0.0f);
    ASLgl::glVertex3f(0.0f, -(fRadius * fAlpha), 0.0f);
    ASLgl::glVertex3f(fSize * -0.717f, fSize * -0.717f, 0.0f);
    ASLgl::glVertex3f(-(fRadius * fAlpha), 0.0f, 0.0f);
    ASLgl::glVertex3f(fSize * -0.717f, fSize * 0.717f, 0.0f);
    ASLgl::glEnd();

    if (bMask)
        ASLgl::glPopMatrix();
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glEnable(GL_LIGHTING);
}

// Uploads a height field as a normal map, mip-downsampling one level per iteration.
void GLRender::ConvertHeightFieldAndLoadNormalMapTexture(unsigned char* pData, int nWidth,
                                                         int nHeight, int nSrcWidth, int nSrcHeight,
                                                         float fScale, int nUseSubImage, bool bCopyField)
{
    Normal* pBuffer;
    if (bCopyField)
    {
        int nCount = nHeight * nWidth;
        pBuffer = new Normal[nCount];
        memcpy(pBuffer, pData, nCount * 4);  // idiom-exempt: sizeof(Normal) is 4, and the ORIG's size argument is a 32-BIT product sign-extended (`lsl w8,#2` + `sxtw x2`); `sizeof(Normal) * nCount` would be an i64 multiply and change the row
    }
    else
    {
        pBuffer = convertHeightFieldToNormalMap(pData, nWidth, nHeight, nSrcWidth, nSrcHeight, fScale);
    }

    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, pBuffer);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cm_nAnisotropy);

    if (nHeight > 1 || nWidth > 1)
    {
        int nLevel = 1;
        do
        {
            int nNewWidth = nWidth >> 1;
            int nNewHeight = nHeight >> 1;
            if (nNewWidth == 0)
                nNewWidth = 1;
            if (nNewHeight == 0)
                nNewHeight = 1;

            pBuffer = downSampleNormalMap(pBuffer, nWidth, nHeight, nNewWidth, nNewHeight);
            if (nUseSubImage == 0)
            {
                ASLgl::glTexImage2D(GL_TEXTURE_2D, nLevel, GL_RGBA8, nNewWidth, nNewHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE,
                                    pBuffer);
                ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cm_nAnisotropy);
            }
            else
            {
                ASLgl::glTexSubImage2D(GL_TEXTURE_2D, nLevel, 0, 0, nNewWidth, nNewHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                                       pBuffer);
            }
            nLevel++;
            nWidth = nNewWidth;
            nHeight = nNewHeight;
        } while (nHeight > 1 || nWidth > 1);
    }

    if (pBuffer != 0)
        delete[] pBuffer;
}


// Finds a free slot in the imposter table, doubling the table when there is none.
int GLRender::SetUpImposters(int nCount, float* pOffsets)
{
    int nSlots = manRefImposter.m_nCount;
    Imposter* pOld = manRefImposter.m_pImposters;
    int nSlot;

    for (nSlot = 0; nSlot < nSlots; nSlot++)
    {
        if (!pOld[nSlot].m_bUsed)
        {
            ImposterManager::AllocateImposter(nSlot, nCount, pOffsets);
            return nSlot;
        }
    }

    Imposter* pNew = new Imposter[nSlots * 2];

    manRefImposter.m_pImposters = pNew;
    memset(pNew, 0, nSlots * 2);  // idiom-exempt: the ORIG's size argument is a BYTE count of `nSlots * 2` (`mov x2,x23`), not a sizeof-shaped one -- see the PARKED note above
    memcpy(pNew, pOld, nSlots * sizeof(Imposter));
    if (pOld)
        delete[] pOld;

    ImposterManager::AllocateImposter(nSlot, nCount, pOffsets);
    nSlot = manRefImposter.m_nCount;
    manRefImposter.m_nCount = nSlot * 2;
    return nSlot;
}

// Fills one imposter slot: three floats per imposter, uploaded as a unit-0 texture.
__attribute__((noinline))
void ImposterManager::AllocateImposter(unsigned int nSlot, int nCount, float* pOffsets)
{
    Imposter* pImposter = &manRefImposter.m_pImposters[nSlot];

    pImposter->m_bUsed = 1;
    pImposter->m_nOffsetCount = nCount;
    pImposter->m_pOffsets = new float[nCount * 3];
    memcpy(pImposter->m_pOffsets, pOffsets, nCount * 3 * sizeof(float));

    unsigned int nTexture;
    ASLgl::glGenTextures(1, &nTexture);
    pImposter->m_nTexture = nTexture;
    ASLgl::glBindTexture(GL_TEXTURE_2D, nTexture);
    GLRender::cm_nGLCurrentTID[GLRender::cm_uiCurrentStage] = 0;
}

#if !KOTOR1
static const char szGammaFrag[] =
    "!!ARBfp1.0\n"
    "ATTRIB T0 = fragment.texcoord[0];\n"
    "PARAM exponent = program.env[1];\n"
    "TEMP r0;\n"
    "TEX r0, T0, texture[0], RECT;\n"
    "POW r0.r, r0.r, exponent.r;\n"
    "POW r0.g, r0.g, exponent.r;\n"
    "POW r0.b, r0.b, exponent.r;\n"
    "MOV result.color.rgb, r0;\n"
    "MOV result.color.a, 1.0;\n"
    "END\n"
    ""
    ;
#endif

// Draws the gamma-correction composite: one shader init, then the framebuffer region through the
// POW program.
void GLRender::RenderGammaCorrection()
{
#if KOTOR1 // K1: a blended full-screen quad (DST_COLOR add above 1, SRC_COLOR scale below), no fragment program
    if (g_fGammaExponent == 1.0f)
        return;

    float fGamma = 1.0f / g_fGammaExponent;
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glLoadIdentity();
    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glDisable(GL_LIGHTING);
    ASLgl::glDisable(GL_TEXTURE_2D);
    ASLgl::glEnable(GL_BLEND);
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glLoadIdentity();
    if (fGamma > 1.0f)
    {
        ASLgl::glBlendFunc(GL_DST_COLOR, 1);
        ASLgl::glColor3f(fGamma - 1.0f, fGamma - 1.0f, fGamma - 1.0f);
    }
    else
    {
        ASLgl::glBlendFunc(0, GL_SRC_COLOR);
        ASLgl::glColor3f(fGamma, fGamma, fGamma);
    }
    ASLgl::glBegin(7);
    ASLgl::glVertex2f(-1.0f, 1.0f);
    ASLgl::glVertex2f(-1.0f, -1.0f);
    ASLgl::glVertex2f(1.0f, -1.0f);
    ASLgl::glVertex2f(1.0f, 1.0f);
    ASLgl::glEnd();
    ASLgl::glEnable(GL_DEPTH_TEST);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glEnable(GL_LIGHTING);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else
    static unsigned int gammaPS;

    if (gMac_GammaShaderNeedsInit)
    {
        gammaPS = 0;
        ASLgl::glGenProgramsARB(1, &gammaPS);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, gammaPS);
        ASLgl::glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                                  static_cast<int>(__strlen_chk(szGammaFrag, sizeof(szGammaFrag))), szGammaFrag);
        ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
        gMac_GammaShaderNeedsInit = 0;
    }

    if (g_fGammaExponent == 1.0f)
        return;

    if (cm_nBufferRegionTexture == 0)
    {
        ASLgl::glGenTextures(1, &cm_nBufferRegionTexture);
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nBufferRegionTexture);
        ASLgl::glCopyTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, 0, 0, g_nScreenWidth, g_nScreenHeight, 0);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ASLgl::glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else
    {
        ASLgl::glEnable(GL_TEXTURE_RECTANGLE);
        ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, cm_nBufferRegionTexture);
        ASLgl::glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, 0, 0, g_nScreenWidth, g_nScreenHeight);
    }

    ASLgl::glActiveTextureARB(GL_TEXTURE0);
    ASLgl::glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, gammaPS);

    float afParam[4];
    afParam[0] = g_fGammaExponent;
    afParam[1] = 0.0f;
    afParam[2] = 0.0f;
    afParam[3] = 0.0f;
    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afParam);

    ASLgl::glDisable(GL_DEPTH_TEST);
    ASLgl::glDisable(GL_LIGHTING);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glOrtho(0.0, static_cast<double>(g_nScreenWidth), 0.0, static_cast<double>(g_nScreenHeight), -1.0, 1.0);
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    ASLgl::glBegin(7);
    for (int i = 0; i < 4; i++)
    {
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        ASLgl::glTexCoord2f((i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                            (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f);
        ASLgl::glVertex3f((i < 2) ? 0.0f : static_cast<float>(g_nScreenWidth),
                          (i == 0 || i == 3) ? static_cast<float>(g_nScreenHeight) : 0.0f, 0.0f);
    }
    ASLgl::glEnd();

    ASLgl::glDisable(GL_FRAGMENT_PROGRAM_ARB);
    ASLgl::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);
    GLRender::SetMatrixMode(Aurora::kMatrixProjection);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
    ASLgl::glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    ASLgl::glDisable(GL_TEXTURE_RECTANGLE);
    ASLgl::glEnable(GL_DEPTH_TEST);
    ASLgl::glEnable(GL_TEXTURE_2D);
    ASLgl::glEnable(GL_LIGHTING);
    ASLgl::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}

// Attaches one interleaved pool, with every element the flag mask enables pointed at that node's
// offset.
void GLRender::SetInterleavedBuffer(unsigned int nStride, int nFlags, unsigned int nPool, int nCount,
                                    MdlNodeTriMesh* pNode, unsigned int nBase)
{
    K2_UNUSED(nStride); K2_UNUSED(nCount);
    int bVBO = AurVertexBufferObjectARBAvailable();
    char* pData = static_cast<char*>(manRef->GetPool(nPool));
    if (bVBO)
    {
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pData)));
        pData = reinterpret_cast<char*>(static_cast<uint64_t>(nBase));
    }
    else
    {
        pData += nBase;
    }

    if (nFlags & MDX_VERTICES)
        ASLgl::glVertexPointer(3, GL_FLOAT, pNode->m_nVertexStride,
                               pData + pNode->m_nVertexOfs);
    if (nFlags & MDX_VERTEX_NORMALS)
        ASLgl::glNormalPointer(GL_FLOAT, pNode->m_nVertexStride,
                               pData + pNode->m_nNormalOfs);
    if (nFlags & MDX_VERTEX_COLORS)
        ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, pNode->m_nVertexStride,
                              pData + pNode->m_nColorOfs);

    if (nFlags & MDX_TEX0_VERTICES)
    {
        GLRender::ActivateTextureStage(0);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
                                 pData + pNode->m_nTex0Ofs);
    }
    if (nFlags & MDX_TEX1_VERTICES)
    {
        GLRender::ActivateTextureStage(1);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
                                 pData + pNode->m_nTex1Ofs);
    }
    if (nFlags & MDX_TEX2_VERTICES)
    {
        GLRender::ActivateTextureStage(2);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
                                 pData + pNode->m_nTex2Ofs);
    }
    if (nFlags & MDX_TEX3_VERTICES)
    {
        GLRender::ActivateTextureStage(3);
        ASLgl::glTexCoordPointer(2, GL_FLOAT, pNode->m_nVertexStride,
                                 pData + pNode->m_nTex3Ofs);
    }
    GLRender::ActivateTextureStage(0);

    if (AurVertexProgramsAvailable())
    {
        if (nFlags & MDX_TANGENT_SPACE)
        {
            if (usearbvertexprograms)
                ASLgl::glVertexAttribPointerARB(12, 3, GL_FLOAT, 0, pNode->m_nVertexStride,
                                                pData + pNode->m_nTangentOfs);
            if (usearbvertexprograms)
                ASLgl::glVertexAttribPointerARB(13, 3, GL_FLOAT, 0, pNode->m_nVertexStride,
                                                pData + pNode->m_nTangentOfs + 12);
            if (usearbvertexprograms)
                ASLgl::glVertexAttribPointerARB(14, 3, GL_FLOAT, 0, pNode->m_nVertexStride,
                                                pData + pNode->m_nTangentOfs + 24);
            ASLgl::glEnableVertexAttribArrayARB(12);
            ASLgl::glEnableVertexAttribArrayARB(13);
            ASLgl::glEnableVertexAttribArrayARB(14);
        }
        else
        {
            ASLgl::glDisableVertexAttribArrayARB(12);
            ASLgl::glDisableVertexAttribArrayARB(13);
            ASLgl::glDisableVertexAttribArrayARB(14);
        }

        if (pNode->AsMdlNodeSkin())
        {
            int nSkinStride = pNode->m_nVertexStride;
            MdlNodeSkin* pSkin = pNode->AsMdlNodeSkin();
            if (usearbvertexprograms)
                ASLgl::glVertexAttribPointerARB(1, 4, GL_FLOAT, 0, nSkinStride,
                    pData + static_cast<uint64_t>(static_cast<unsigned int>(pSkin->m_nBoneWeightsOfs)));
            nSkinStride = pNode->m_nVertexStride;
            pSkin = pNode->AsMdlNodeSkin();
            if (usearbvertexprograms)
                ASLgl::glVertexAttribPointerARB(4, 4, GL_FLOAT, 0, nSkinStride,
                    pData + static_cast<uint64_t>(static_cast<unsigned int>(pSkin->m_nBoneIndicesOfs)));
            ASLgl::glEnableVertexAttribArrayARB(1);
            ASLgl::glEnableVertexAttribArrayARB(4);
        }
        else
        {
            ASLgl::glDisableVertexAttribArrayARB(1);
            ASLgl::glDisableVertexAttribArrayARB(4);
        }
    }
}

// The cube-map face order and the per-face quarter-turn count.
static const int anCubeFaceOffset[6]   = { 1, 0, 2, 3, 4, 5 };
static const int anCubeFaceRotation[6] = { 3, 1, 0, 2, 2, 0 };

unsigned char* ImageScale(unsigned char* pData, int nSrcWidth, int nSrcHeight, int nBpp,
                          int nDestWidth, int nDestHeight);
unsigned char* ImageGetSubImage(unsigned char* pData, int nWidth, int nHeight, int nBPP,
                                int nX, int nY, int nSubWidth, int nSubHeight);
unsigned char* ImageQuarterRotated(unsigned char* pImage, int nWidth, int nHeight, int nBPP,
                                   int nQuarter);

extern const unsigned int textureFormatTranslationTable[] = {
    6408, 6407, 6410, 6409, 32993, 34026, 34027, 33776, 33779
};
int AurBumpMapAvailable();
int AurSGIMipmapAvailable();
int BuildTextureNormalMap(unsigned char* pData, float fScale, int& nWidth, int& nHeight,
                          int nBPP, int nUseSubImage, bool bCopyField);

// Uploads six cube faces, each carved out of the image and tiled into sub-tiles.
void GLRender::CreateCubeTexture(CAurTexture* pTexture, bool bParam, int nIndex)
{
    for (int i = 0; i < 6; i++)
    {
        int nWidth = pTexture->GetWidth();
        int nHeight = pTexture->GetHeight();
        int nBPP = pTexture->GetBPP();
        int nFaceHeight = nHeight / 6;
        int nFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X + anCubeFaceOffset[i];
        unsigned char* pFace = static_cast<unsigned char*>(pTexture->GetImage())
                             + nWidth * i * nFaceHeight * nBPP;
        int nSubWidth = nWidth / pTexture->GetNumX();
        int nSubHeight = nFaceHeight / pTexture->GetNumY();

        unsigned char* pUnrotated = pFace;
        int nTileX = (nIndex % pTexture->GetNumX()) * nSubWidth;
        int nTileY = (nIndex / pTexture->GetNumX()) * nSubHeight;
        if (pTexture->GetNumX() != 1 || pTexture->GetNumY() != 1)
        {
            pFace = ImageGetSubImage(pFace, nWidth, nFaceHeight, nBPP, nTileX, nTileY,
                                     nSubWidth, nSubHeight);
        }

        unsigned char* pRotated = ImageQuarterRotated(pFace, nSubWidth, nSubHeight,
                                                      pTexture->GetBPP(),
                                                      anCubeFaceRotation[i]);

        int nFormatBPP = pTexture->GetBPP();
        unsigned int eFormat;
        if (nFormatBPP == 3)
            eFormat = GL_RGB;
        else if (nFormatBPP == 4)
            eFormat = GL_RGBA;
        else
            eFormat = (nFormatBPP == 2) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;

        if (pTexture->GetMipMap())
            gluBuild2DMipmaps(nFace, nFormatBPP, nSubWidth, nSubHeight, eFormat, GL_UNSIGNED_BYTE,
                              pRotated);
        else if (bParam)
            ASLgl::glTexSubImage2D(nFace, 0, 0, 0, nSubWidth, nSubHeight, eFormat, GL_UNSIGNED_BYTE,
                                   pRotated);
        else
            ASLgl::glTexImage2D(nFace, 0, nFormatBPP, nSubWidth, nSubHeight, 0, eFormat, GL_UNSIGNED_BYTE,
                                pRotated);

        if (pRotated != pFace)
            delete pRotated;
        if (pFace != pUnrotated)
            delete pFace;
    }

    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, pTexture->GetFilter() ? GL_LINEAR : GL_NEAREST);

    bool bMipMap = pTexture->GetMipMap();
    bool bFilter = pTexture->GetFilter();
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                           bMipMap ? (bFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST)
                                   : (bFilter ? GL_LINEAR : GL_NEAREST));
}

// Uploads a pre-processed cube map. The source already holds every mip level back to back.
void GLRender::CreateProcessedCubeTexture(CAurTexture* pTexture, bool bParam, int nIndex)
{
    K2_UNUSED(nIndex);
    int nWidth = pTexture->GetWidth();
    int nHeight = pTexture->GetHeight();
    int nFaceHeight = nHeight / 6;
    int nBPP = pTexture->GetBPP();
    bool bMipMap = pTexture->GetMipMap();
    unsigned char* pImage = static_cast<unsigned char*>(pTexture->GetImage());

    unsigned int eFormat = (nBPP == 4) ? GL_RGBA : GL_RGB;
    int nFaceSize = nFaceHeight * nWidth * nBPP;
    int nBlockSize = (nBPP == 4) ? 16 : 8;

    for (int i = 0; i < 6; i++)
    {
        unsigned int nFace = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;

        if (pTexture->GetCompressedSize() >= 1)
        {
#if KOTOR1 // K1: the cube faces upload through the ARB entry point
            android_port_glCompressedTexImage2DARB(nFace, 0,
                textureFormatTranslationTable[static_cast<unsigned int>(pTexture->GetCompressionFormat())],
                nWidth, nFaceHeight, 0, pTexture->GetCompressedSize(), pImage);
#else
            ASLgl::glCompressedTexImage2DARB(nFace, 0,
                textureFormatTranslationTable[static_cast<unsigned int>(pTexture->GetCompressionFormat())],
                nWidth, nFaceHeight, 0, pTexture->GetCompressedSize(), pImage);
#endif

            if (bMipMap)
            {
                int nSize = pTexture->GetCompressedSize();
                int nW = nWidth >> 1;
                int nH = nFaceHeight >> 1;
                int nLevel = 1;
                while ((nW | nH) != 0)
                {
                    if (nW == 0)
                        nW = 1;
                    if (nH == 0)
                        nH = 1;
                    pImage += nSize;
                    nSize = ((nW + 3) / 4) * nBlockSize * ((nH + 3) / 4);
#if KOTOR1
                    android_port_glCompressedTexImage2DARB(nFace, nLevel,
                        textureFormatTranslationTable[static_cast<unsigned int>(pTexture->GetCompressionFormat())],
                        nW, nH, 0, nSize, pImage);
#else
                    ASLgl::glCompressedTexImage2DARB(nFace, nLevel,
                        textureFormatTranslationTable[static_cast<unsigned int>(pTexture->GetCompressionFormat())],
                        nW, nH, 0, nSize, pImage);
#endif
                    nW >>= 1;
                    nH >>= 1;
                    nLevel++;
                }
                pImage += nSize;
            }
        }
        else if (bMipMap)
        {
            if (bParam)
                ASLgl::glTexSubImage2D(nFace, 0, 0, 0, nWidth, nFaceHeight, eFormat, GL_UNSIGNED_BYTE,
                                       pImage);
            else
                ASLgl::glTexImage2D(nFace, 0, nBPP, nWidth, nFaceHeight, 0, eFormat, GL_UNSIGNED_BYTE,
                                    pImage);

            int nNumMipMaps = pTexture->GetNumMipMaps();
            int nW = nWidth;
            int nH = nFaceHeight;
            for (int nLevel = 1; nLevel < nNumMipMaps; nLevel++)
            {
                nW >>= 1;
                nH >>= 1;
                if (nW == 0)
                    nW = 1;
                if (nH == 0)
                    nH = 1;
                int nSize = nW * nH;
                pImage += nFaceSize;
                if (bParam)
                    ASLgl::glTexSubImage2D(nFace, nLevel, 0, 0, nW, nH, eFormat, GL_UNSIGNED_BYTE, pImage);
                else
                    ASLgl::glTexImage2D(nFace, nLevel, nBPP, nW, nH, 0, eFormat, GL_UNSIGNED_BYTE, pImage);
                nFaceSize = nSize * nBPP;
            }
            pImage += nFaceSize;
        }
        else
        {
            if (bParam)
                ASLgl::glTexSubImage2D(nFace, 0, 0, 0, nWidth, nFaceHeight, eFormat, GL_UNSIGNED_BYTE,
                                       pImage);
            else
                ASLgl::glTexImage2D(nFace, 0, nBPP, nWidth, nFaceHeight, 0, eFormat, GL_UNSIGNED_BYTE,
                                    pImage);
        }
    }

    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, pTexture->GetFilter() ? GL_LINEAR : GL_NEAREST);

    bool bMipMapped = pTexture->GetMipMap();
    bool bFiltered = pTexture->GetFilter();
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                           bMipMapped ? (bFiltered ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST)
                                      : (bFiltered ? GL_LINEAR : GL_NEAREST));
}

// Uploads a pre-processed 2D texture, the sibling of the processed cube-map upload. The image
// already holds every mip level back to back.
void GLRender::CreateProcessedTexture(CAurTexture* pTexture, bool bParam, int nIndex)
{
    int nHeight = pTexture->GetHeight();
    int nBPP = pTexture->GetBPP();
    unsigned char* pImage = static_cast<unsigned char*>(pTexture->GetImage());
    int nNumX = pTexture->GetNumX();
    int nNumY = pTexture->GetNumY();
    int nWidth = pTexture->GetWidth();
    int nSubWidth = nWidth / nNumX;
    int nSubHeight = nHeight / nNumY;

    unsigned char* pSub = pImage;
    if (nNumX != 1 || nNumY != 1)
        pSub = pImage + GetCompressedContactSheetOffset(nIndex, nSubWidth, nBPP);

    bool bMipMap = pTexture->GetMipMap();
    bool bBumpMap = pTexture->IsBumpMap();

    if (nBPP == 1 && bBumpMap)
    {
        if (AurBumpMapAvailable())
        {
            float fScaling = pTexture->GetBumpMapScaling();
            bool bNormalMap = pTexture->IsNormalMap();
            BuildTextureNormalMap(pSub, fScaling, nSubWidth, nSubHeight, 1, 0, bNormalMap);
        }
        if (pSub != pImage)
            delete pSub;
        return;
    }
    else if (pTexture->GetCompressedSize() >= 1)
    {
        int nBlockSize = (nBPP == 4) ? 16 : 8;
        int nSize = ((nSubWidth + 3) / 4) * nBlockSize * ((nSubHeight + 3) / 4);

#if KOTOR1 // K1: a PVRTC image (format 10) uploads its whole mip chain and sets its own sampler state
        if (pTexture->GetCompressionFormat() == 10)
        {
            unsigned int eFormat = (nBPP == 5) ? GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG : GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            int nLevelSize = nWidth * nHeight / 2;
            int nRemaining = pTexture->GetCompressedSize();
            int nLevel = 0;
            while (nRemaining >= nLevelSize)
            {
                ASLgl::glCompressedTexImage2DARB(GL_TEXTURE_2D, nLevel, eFormat, nWidth, nHeight, 0, nLevelSize, pSub);
                pSub += nLevelSize;
                nRemaining -= nLevelSize;
                nLevelSize = (nLevelSize / 4 > 32) ? nLevelSize / 4 : 32;
                nWidth /= 2;
                nHeight /= 2;
                nLevel++;
            }
            int nClamp = pTexture->GetClamp();
            pTexture->GetFilter();
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (nClamp & 1) ? GL_CLAMP : GL_REPEAT);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (nClamp & 2) ? GL_CLAMP : GL_REPEAT);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bMipMap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            float fMaxAnisotropy;
            ASLgl::glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fMaxAnisotropy);
            ASLgl::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fMaxAnisotropy);
            return;
        }
#endif
        ASLgl::glCompressedTexImage2DARB(GL_TEXTURE_2D, 0,
            textureFormatTranslationTable[static_cast<unsigned int>(pTexture->GetCompressionFormat())],
            nSubWidth, nSubHeight, 0, nSize, pSub);

        if (pTexture->GetMipMap())
        {
            int nH = nSubHeight >> 1;
            int nW = nSubWidth >> 1;
            int nLevel = 1;
            while ((nW | nH) != 0)
            {
                if (nW == 0)
                    nW = 1;
                if (nH == 0)
                    nH = 1;
                pSub += nSize;
                int nBlockW = (nW + 3) / 4;
                int nBlockH = (nH + 3) / 4;
                nSize = nBlockW * nBlockSize * nBlockH;
                ASLgl::glCompressedTexImage2DARB(GL_TEXTURE_2D, nLevel,
                    textureFormatTranslationTable[static_cast<unsigned int>(pTexture->GetCompressionFormat())],
                    nW, nH, 0, nSize, pSub);
                nH >>= 1;
                nW >>= 1;
                nLevel++;
            }
        }
    }
    else
    {
        unsigned int eFormat;
        if (nBPP == 4)
            eFormat = GL_RGBA;
        else if (nBPP == 3)
            eFormat = GL_RGB;
        else
        {
            eFormat = GL_RGBA;
            if (nBPP != 2)
                eFormat = GL_LUMINANCE;
        }

        int nInternalFormat = (nBPP == 2) ? 4 : nBPP;
        unsigned int eType = (nBPP == 2) ? GL_UNSIGNED_SHORT_4_4_4_4 : GL_UNSIGNED_BYTE;

        if (bMipMap)
        {
            unsigned char* pData = static_cast<unsigned char*>(pTexture->GetImage());
            if (bParam)
                ASLgl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nSubWidth, nSubHeight, eFormat, GL_UNSIGNED_BYTE,
                                       pData);
            else
                ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, nInternalFormat, nSubWidth, nSubHeight, 0, eFormat,
                                    eType, pData);

            if (pTexture->GetMipMap())
            {
                int nNumMipMaps = pTexture->GetNumMipMaps();
                int nW = nSubWidth;
                int nH = nSubHeight;
                int nSize = nSubHeight * nSubWidth;
                for (int nLevel = 0; nLevel < nNumMipMaps - 1; nLevel++)
                {
                    nW >>= 1;
                    if (nW == 0)
                        nW = 1;
                    nH >>= 1;
                    if (nH == 0)
                        nH = 1;
                    pData += nSize * nBPP;
                    nSize = nH * nW;
                    if (bParam)
                        ASLgl::glTexSubImage2D(GL_TEXTURE_2D, nLevel + 1, 0, 0, nW, nH, eFormat, eType, pData);
                    else
                        ASLgl::glTexImage2D(GL_TEXTURE_2D, nLevel + 1, nInternalFormat, nW, nH, 0, eFormat,
                                            eType, pData);
                }
            }
        }
        else
        {
            if (bParam)
                ASLgl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nSubWidth, nSubHeight, eFormat, GL_UNSIGNED_BYTE,
                                       pSub);
            else
                ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, nInternalFormat, nSubWidth, nSubHeight, 0, eFormat,
                                    eType, pSub);
        }
    }

    if (!pTexture->IsBumpMap())
    {
        int nClamp = pTexture->GetClamp();
        bool bFilter = pTexture->GetFilter();
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (nClamp & 1) ? GL_CLAMP : GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (nClamp & 2) ? GL_CLAMP : GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bFilter ? GL_LINEAR : GL_NEAREST);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                               bMipMap ? (bFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST)
                                       : (bFilter ? GL_LINEAR : GL_NEAREST));
#if KOTOR1  // K1: maximum anisotropy on every processed texture
        float fMaxAnisotropy;
        ASLgl::glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fMaxAnisotropy);
        ASLgl::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fMaxAnisotropy);
#endif
    }
}

// Builds mipmaps on the CPU: squares the image, uploads level 0, then box-filters each halved
// level.
void AurBuild2DMipMaps(int nWidth, int nHeight, int nBPP, unsigned char* pData, bool bSubImage)
{
    unsigned char* pImage;
    int nSrcSize;   // the byte size of the level pSrc holds below
    if (nWidth == nHeight)
    {
        nSrcSize = nWidth * nWidth * nBPP;
        pImage = new unsigned char[nSrcSize];
        memcpy(pImage, pData, nSrcSize);  // idiom-exempt: a runtime byte count (nWidth*nWidth*nBPP), no sizeof exists
    }
    else
    {
        int nDimension = (nWidth > nHeight) ? nWidth : nHeight;
        pImage = ImageScale(pData, nWidth, nHeight, nBPP, nDimension, nDimension);
        nWidth = nDimension;
        nSrcSize = nDimension * nDimension * nBPP;
    }

    int nType = (nBPP == 2) ? GL_UNSIGNED_SHORT_4_4_4_4 : GL_UNSIGNED_BYTE;
    unsigned int eFormat;
    if (nBPP == 3)
        eFormat = GL_RGB;
    else if (nBPP == 4)
        eFormat = GL_RGBA;
    else
        eFormat = (nBPP == 2) ? GL_RGBA : GL_LUMINANCE;

    if (bSubImage)
        ASLgl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nWidth, nWidth, eFormat, nType, pImage);
    else
        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, nBPP, nWidth, nWidth, 0, eFormat, nType, pImage);

    int nLevel = 0;
    nHeight = nWidth;
    unsigned char* pSrc = pImage;
    unsigned char* pDst;
    while (nWidth * nHeight > 1)
    {
        int nHalfWidth = nWidth / 2;
        int nHalfHeight = nHeight / 2;
        int nDstSize = nHalfHeight * nBPP * nHalfWidth;
        pDst = new unsigned char[nDstSize];

        if (nHeight >= 2)
        {
            for (int y = 0; y < nHalfHeight; y++)
            {
                int nRow0 = 2 * y * nWidth;
                int nRow1 = nRow0 + nWidth;
                int nDstRow = y * nHalfWidth;
                for (int x = 0; x < nHalfWidth; x++)
                {
                    int nSrcIdx0 = (nRow0 + 2 * x) * nBPP;
                    int nSrcIdx1 = (nRow0 + 2 * x + 1) * nBPP;
                    int nSrcIdx2 = (nRow1 + 2 * x) * nBPP;
                    int nSrcIdx3 = (nRow1 + 2 * x + 1) * nBPP;
                    int nDstIdx = (nDstRow + x) * nBPP;
                    // 2y + 1 < nHeight and 2x + 1 < nWidth: the four taps and the texel stay in their levels.
                    K2_ASSUME(nSrcIdx0 >= 0 && nSrcIdx3 + nBPP <= nSrcSize && nDstIdx >= 0 && nDstIdx + nBPP <= nDstSize);
                    if (nBPP == 2)
                    {
                        int nLo = (pSrc[nSrcIdx0] & 0xF) + (pSrc[nSrcIdx1] & 0xF)
                                + (pSrc[nSrcIdx2] & 0xF) + (pSrc[nSrcIdx3] & 0xF);
                        int nHi = (pSrc[nSrcIdx0] >> 4) + (pSrc[nSrcIdx1] >> 4)
                                + (pSrc[nSrcIdx2] >> 4) + (pSrc[nSrcIdx3] >> 4);
                        pDst[nDstIdx] = static_cast<unsigned char>(((nHi << 2) & 0xF0) | (nLo >> 2));
                        nLo = (pSrc[nSrcIdx0 + 1] & 0xF) + (pSrc[nSrcIdx1 + 1] & 0xF)
                            + (pSrc[nSrcIdx2 + 1] & 0xF) + (pSrc[nSrcIdx3 + 1] & 0xF);
                        nHi = (pSrc[nSrcIdx0 + 1] >> 4) + (pSrc[nSrcIdx1 + 1] >> 4)
                            + (pSrc[nSrcIdx2 + 1] >> 4) + (pSrc[nSrcIdx3 + 1] >> 4);
                        pDst[nDstIdx + 1] = static_cast<unsigned char>(((nHi << 2) & 0xF0) | (nLo >> 2));
                    }
                    else
                    {
                        for (int k = 0; k < nBPP; k++)
                        {
                            K2_ASSUME(nDstIdx + k < nDstSize);
                            pDst[nDstIdx + k] = (pSrc[nSrcIdx0 + k] + pSrc[nSrcIdx1 + k]
                                               + pSrc[nSrcIdx2 + k] + pSrc[nSrcIdx3 + k]) >> 2;
                        }
                    }
                }
            }
        }

#ifdef K2_FIX_ENGINE_BUGS
        delete[] pSrc;
#else
        delete pSrc;
#endif
        nLevel++;
        nWidth = nHalfWidth;
        nHeight = nHalfHeight;
        if (bSubImage)
            ASLgl::glTexSubImage2D(GL_TEXTURE_2D, nLevel, 0, 0, nWidth, nHeight, eFormat, nType, pDst);
        else
            ASLgl::glTexImage2D(GL_TEXTURE_2D, nLevel, nBPP, nWidth, nHeight, 0, eFormat, nType, pDst);
        pSrc = pDst;
        nSrcSize = nDstSize;
    }

#ifdef K2_FIX_ENGINE_BUGS
    delete[] pSrc;
#else
    delete pSrc;
#endif
}

// Uploads a plain texture, covering the bump-map arm, the volume-texture split and both mip paths.
void GLRender::CreateTexture(CAurTexture* pTexture, bool bParam, int nIndex)
{
    int nHeight = pTexture->GetHeight();
    int nBPP = pTexture->GetBPP();
    unsigned char* pImage = static_cast<unsigned char*>(pTexture->GetImage());
    int nNumX = pTexture->GetNumX();
    int nNumY = pTexture->GetNumY();
    int nWidth = pTexture->GetWidth();
    int nSubWidth = nWidth / nNumX;
    int nSubHeight = nHeight / nNumY;

    unsigned char* pSub = pImage;
    if (nNumX != 1 || nNumY != 1)
    {
        int nTileY = (nIndex / nNumX) * nSubHeight;
        int nTileX = (nIndex - (nIndex / nNumX) * nNumX) * nSubWidth;
        pSub = ImageGetSubImage(pImage, nWidth, nHeight, nBPP, nTileX, nTileY, nSubWidth,
                                nSubHeight);
    }

    if (pTexture->IsBumpMap())
    {
        if (AurBumpMapAvailable())
        {
            float fScaling = pTexture->GetBumpMapScaling();
            bool bNormalMap = pTexture->IsNormalMap();
            BuildTextureNormalMap(pSub, fScaling, nSubWidth, nSubHeight, nBPP, 0, bNormalMap);
        }
        if (pSub != pImage)
            delete pSub;
    }
    else
    {
        bool bMipMap = pTexture->GetMipMap();
        if (pTexture->GetCompressedSize() >= 1)
        {
            int nBlockSize = (nBPP == 4) ? 16 : 8;
            unsigned char* pData = static_cast<unsigned char*>(pTexture->GetImage());
            unsigned int eFormat =
                textureFormatTranslationTable[static_cast<unsigned int>(pTexture->GetCompressionFormat())];
            int nSize = pTexture->GetCompressedSize();
            ASLgl::glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, eFormat, nWidth, nHeight, 0, nSize,
                                             static_cast<unsigned char*>(pTexture->GetImage()));

            if (pTexture->GetMipMap())
            {
                nSize = pTexture->GetCompressedSize();
                int nH = nHeight >> 1;
                int nW = nWidth >> 1;
                int nLevel = 1;
                while ((nW | nH) != 0)
                {
                    if (nW == 0)
                        nW = 1;
                    if (nH == 0)
                        nH = 1;
                    pData += nSize;
                    int nBlockW = (nW + 3) / 4;
                    int nBlockH = (nH + 3) / 4;
                    nSize = nBlockW * nBlockSize * nBlockH;
                    ASLgl::glCompressedTexImage2DARB(GL_TEXTURE_2D, nLevel,
                        textureFormatTranslationTable[static_cast<unsigned int>(pTexture->GetCompressionFormat())],
                        nW, nH, 0, nSize, pData);
                    nH >>= 1;
                    nW >>= 1;
                    nLevel++;
                }
            }
        }
        else
        {
            unsigned int eFormat;
            if (nBPP == 4)
                eFormat = GL_RGBA;
            else if (nBPP == 3)
                eFormat = GL_RGB;
            else
            {
                eFormat = GL_RGBA;
                if (nBPP != 2)
                    eFormat = GL_LUMINANCE;
            }

            int nInternalFormat = (nBPP == 2) ? 4 : nBPP;
            unsigned int eType = (nBPP == 2) ? GL_UNSIGNED_SHORT_4_4_4_4 : GL_UNSIGNED_BYTE;

            if (bMipMap)
            {
#if KOTOR1 // K1: no GL_GENERATE_MIPMAP path
                if (nBPP == 2 || bParam)
#else
                if (AurSGIMipmapAvailable())
                {
                    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
                    ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, nInternalFormat, nSubWidth, nSubHeight, 0,
                                        eFormat, eType, pSub);
                }
                else if (nBPP == 2 || bParam)
#endif
                {
                    AurBuild2DMipMaps(nSubWidth, nSubHeight, nBPP, pSub, bParam);
                }
                else
                {
                    gluBuild2DMipmaps(GL_TEXTURE_2D, nInternalFormat, nSubWidth, nSubHeight, eFormat,
                                      eType, pSub);
                }
            }
            else
            {
#if KOTOR1 // K1: no 3D textures
                if (bParam)
                    ASLgl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nSubWidth, nSubHeight, eFormat,
                                           GL_UNSIGNED_BYTE, pSub);
                else
                    ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, nInternalFormat, nSubWidth, nSubHeight, 0,
                                        eFormat, eType, pSub);
#else
                int nDepth = pTexture->GetDepth();
                if (bParam)
                {
                    if (nDepth == 0)
                        ASLgl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nSubWidth, nSubHeight, eFormat,
                                               GL_UNSIGNED_BYTE, pSub);
                    else
                        ASLgl::glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nSubWidth, nSubHeight,
                                               pTexture->GetDepth(), eFormat, GL_UNSIGNED_BYTE, pSub);
                }
                else
                {
                    if (nDepth != 0)
                        ASLgl::glTexImage3D(GL_TEXTURE_3D, 0, nInternalFormat, nSubWidth, nSubHeight,
                                            pTexture->GetDepth(), 0, eFormat, eType, pSub);
                    else
                        ASLgl::glTexImage2D(GL_TEXTURE_2D, 0, nInternalFormat, nSubWidth, nSubHeight, 0,
                                            eFormat, eType, pSub);
                }
#endif
            }
        }

        int nClamp = pTexture->GetClamp();
        bool bFilter = pTexture->GetFilter();
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (nClamp & 1) ? GL_CLAMP : GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (nClamp & 2) ? GL_CLAMP : GL_REPEAT);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bFilter ? GL_LINEAR : GL_NEAREST);
        ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                               bMipMap ? (bFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST)
                                       : (bFilter ? GL_LINEAR : GL_NEAREST));

        if (pSub != pImage)
            delete pSub;
    }
}

// Draws the per-frame shadows: expands index pairs into a strip-shaped list and draws it against
// both stencil faces.
void GLRender::RenderShadowArray(int nStart, int nCount, int nUnused)
{
    K2_UNUSED(nUnused);
    static unsigned short* pShadowData = new unsigned short[6144];

    if (nCount == 0)
        return;

    if (nCount > 0)
    {
        int nBase = nStart * 2;
        unsigned short* pOut = pShadowData;
        for (uint64_t i = 0; i < static_cast<unsigned int>(nCount); i++)
        {
            int nVertex0 = m_UsedVertices[PartTriMesh::m_VertexIndices[i * 2]] * 2;
            int nVertex1 = m_UsedVertices[PartTriMesh::m_VertexIndices[i * 2 + 1]] * 2;
            if ((i & 1) == 0)
            {
                pOut[1] = static_cast<unsigned short>(nVertex0 - 2);
                pOut[0] = static_cast<unsigned short>(nBase);
                pOut[2] = static_cast<unsigned short>(nVertex1 - 2);
                pOut[3] = static_cast<unsigned short>(nVertex0 - 1);
                pOut[4] = static_cast<unsigned short>(nVertex1 - 1);
                pOut[5] = static_cast<unsigned short>(nBase | 1);
            }
            else
            {
                pOut[2] = static_cast<unsigned short>(nVertex0 - 1);
                pOut[0] = static_cast<unsigned short>(nBase | 1);
                pOut[1] = static_cast<unsigned short>(nVertex1 - 1);
                pOut[3] = static_cast<unsigned short>(nVertex1 - 2);
                pOut[4] = static_cast<unsigned short>(nVertex0 - 2);
                pOut[5] = static_cast<unsigned short>(nBase);
            }
            pOut += 6;
        }
    }

    if (AurVertexBufferObjectARBAvailable())
        ASLgl::glBindBufferARB(GL_ARRAY_BUFFER, 0);

    ASLgl::glVertexPointer(3, GL_FLOAT, 12, cm_pUserShadowData);

#if KOTOR1 // K1: always the two-pass front/back draw, culling left as it is
    ASLgl::glFrontFace(GL_CW);
    ASLgl::glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
    if (AurVertexBufferObjectARBAvailable())
        ASLgl::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
    ASLgl::glDrawElements(5, nCount * 6, GL_UNSIGNED_SHORT, pShadowData);
    ASLgl::glFrontFace(GL_CCW);
    ASLgl::glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
    if (AurVertexBufferObjectARBAvailable())
        ASLgl::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
    ASLgl::glDrawElements(5, nCount * 6, GL_UNSIGNED_SHORT, pShadowData);
#else
    if (usetwosidedshadows != 0 && AurTwoSidedStencilAvailable())
    {
        ASLgl::glDisable(GL_CULL_FACE);
        ASLgl::glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
        ASLgl::glActiveStencilFaceEXT(GL_BACK);
        ASLgl::glStencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP);
        ASLgl::glStencilMask(0xffffffff);
        ASLgl::glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
        ASLgl::glActiveStencilFaceEXT(GL_FRONT);
        ASLgl::glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP);
        ASLgl::glStencilMask(0xffffffff);
        if (AurVertexBufferObjectARBAvailable())
            ASLgl::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
        ASLgl::glDrawElements(5, nCount * 6, GL_UNSIGNED_SHORT, pShadowData);
        ASLgl::glEnable(GL_CULL_FACE);
        ASLgl::glFrontFace(GL_CCW);
        ASLgl::glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
    }
    else
    {
        unsigned char bWasEnabled = GLRender::IsEnabled(Aurora::kCapCullFace);
        GLRender::Enable(Aurora::kCapCullFace);
        ASLgl::glFrontFace(GL_CW);
        ASLgl::glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
        if (AurVertexBufferObjectARBAvailable())
            ASLgl::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
        ASLgl::glDrawElements(5, nCount * 6, GL_UNSIGNED_SHORT, pShadowData);
        ASLgl::glFrontFace(GL_CCW);
        ASLgl::glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
        if (AurVertexBufferObjectARBAvailable())
            ASLgl::glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
        ASLgl::glDrawElements(5, nCount * 6, GL_UNSIGNED_SHORT, pShadowData);
        if (!bWasEnabled)
            GLRender::Disable(Aurora::kCapCullFace);
    }
#endif

#if !KOTOR1  // K1 keeps no draw counters
    countfaces += nCount * 8;
#endif
}

// Builds the shadow volume: renumbers the used vertices, then writes one 24-byte record per vertex.
int GLRender::BuildShadowArray(Vector const& vOrigin, float const& fHeight, Plane const& plane,
                               Vector const& vA, Vector const& vB, int nCount,
                               RenderVertexV* pVerts)
{
    static Vector light2temp;
    static Vector scratch[2];

    int nUsed = 0;
    if (nCount >= 1)
    {
        unsigned short* pUsed = m_UsedVertices;
        for (uint64_t i = static_cast<unsigned int>(nCount); i != 0; i--, pUsed++)
        {
            if (*pUsed == 1)
            {
                nUsed++;
                *pUsed = static_cast<unsigned short>(nUsed);
            }
        }
    }

    float* pShadow = static_cast<float*>(AllocateShadowMemory(nUsed * 24 + 24));
    cm_pUserShadowData = pShadow;

    if (nCount >= 1)
    {
        unsigned short* pUsed = m_UsedVertices;
        for (uint64_t i = static_cast<unsigned int>(nCount); i != 0; pVerts++, i--, pUsed++)
        {
            uint64_t nIndex = *pUsed;
            if (nIndex == 0)
                continue;

            uint64_t nOffset = nIndex * 6 - 6;

            scratch[0] = reinterpret_cast<const Vector*>(pVerts)[0];
            light2temp = normalize(scratch[0] - vOrigin);
            scratch[1] = light2temp * fHeight + vOrigin;

            if (noclipshadows == 0)
            {
                ClipProjection(scratch[0], vOrigin, plane);
                ClipProjection(scratch[1], vOrigin, plane);
            }

            if (dontprotectagainstclip != 0)
            {
                if ((scratch[0].x - scratch[1].x) * light2temp.x +
                    (scratch[0].y - scratch[1].y) * light2temp.y +
                    (scratch[0].z - scratch[1].z) * light2temp.z > 0.0f)
                    scratch[1] = scratch[0];
            }

            memcpy(&pShadow[nOffset], scratch, sizeof(scratch));
        }
    }

    reinterpret_cast<Vector*>(&pShadow[nUsed * 6])[0] = vA;
    reinterpret_cast<Vector*>(&pShadow[nUsed * 6])[1] = vB;

// The number of used vertices is the return value.
    return nUsed;
}

// Builds the 256x256 luminance-alpha gamma ramp, a linear luminance byte and a gamma alpha byte.
void GLRender::MakeLookUpTexture(float fGamma)
{
    if (ASLgl::glIsTexture(lookup))
        return;

    unsigned char* pData = new unsigned char[131072];

    double dValue = 0.0;
    unsigned char* pRow = pData;
    for (int i = 0; i < 256; i++)
    {
        int nValue = static_cast<int>(pow(dValue / 255.0, fGamma) * 255.99);
        for (int j = 0; j < 256; j++, pRow += 2)
        {
            pRow[0] = static_cast<unsigned char>(static_cast<unsigned int>(static_cast<float>(j) / 255.0f * 255.99f));
            pRow[1] = static_cast<unsigned char>(nValue);
        }
        dValue += 1.0;
    }

    ASLgl::glGenTextures(1, &lookup);
    ASLgl::glBindTexture(GL_TEXTURE_2D, lookup);
    cm_nGLCurrentTID[cm_uiCurrentStage] = 0;
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_LUMINANCE_ALPHA, 256, 256, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, pData);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    delete[] pData;
}

// The 3-byte-per-texel normalization cube map.
void GLRender::MakeNormalizationCubeMap(int nSize)
{
    unsigned char* pData = new unsigned char[nSize * nSize * 3];
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ASLgl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    static const unsigned int mapping[6] = { 2, 3, 4, 5, 6, 7 };

    for (int i = 0; i < 6; i++)
    {
        for (int y = 0; y < nSize; y++)
        {
            float fV = ((y + 0.5f) / nSize) * 2.0f - 1.0f;
            for (int x = 0; x < nSize; x++)
            {
                float fU = ((x + 0.5f) / nSize) * 2.0f - 1.0f;
                float fBlue K2_UNSET;
                float fGreen K2_UNSET;
                float fRed K2_UNSET;
                switch (i)
                {
                case 0:
                    fBlue = -fU; fGreen = -fV; fRed = 1.0f;
                    break;
                case 1:
                    fBlue = fU;  fGreen = -fV; fRed = -1.0f;
                    break;
                case 2:
                    fBlue = fV;  fGreen = 1.0f; fRed = fU;
                    break;
                case 3:
                    fBlue = -fV; fGreen = -1.0f; fRed = fU;
                    break;
                case 4:
                    fBlue = 1.0f; fGreen = -fV;  fRed = fU;
                    break;
                case 5:
                    fBlue = -1.0f; fGreen = -fV;  fRed = -fU;
                    break;
                }
                float fInv = 1.0f / sqrtf(fRed * fRed + fGreen * fGreen + fBlue * fBlue);
                fRed *= fInv;
                fGreen *= fInv;
                fBlue *= fInv;
                pData[(y * nSize + x) * 3] = static_cast<unsigned char>(fRed * 127.0f + 128.0f);
                pData[(y * nSize + x) * 3 + 1] = static_cast<unsigned char>(fGreen * 127.0f + 128.0f);
                pData[(y * nSize + x) * 3 + 2] = static_cast<unsigned char>(fBlue * 127.0f + 128.0f);
            }
        }
        gluBuild2DMipmaps(texParamTargetTranslationTable[mapping[i]], 3, nSize, nSize,
                          GL_RGB, GL_UNSIGNED_BYTE, pData);
    }
    delete[] pData;
}

// The wireframe box spanning two diagonal corners; the third argument is its colour.
void GLRender::DrawAxisAlignedFrame(const Vector& vMin, const Vector& vMax, const Vector& vColor)
{
    ASLgl::glDisable(GL_TEXTURE_2D);    // GL_TEXTURE_2D
    ASLgl::glDisable(GL_LIGHTING);    // GL_FOG
    ASLgl::glColor3f(vColor.x, vColor.y, vColor.z);

// The -z cap.
    ASLgl::glBegin(2);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMin.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMin.z);
    ASLgl::glEnd();

// The +z cap.
    ASLgl::glBegin(2);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMax.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMax.z);
    ASLgl::glEnd();

// The four connectors, each as its own segment pair.
    ASLgl::glBegin(1);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMin.z);
    ASLgl::glVertex3f(vMin.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMin.y, vMax.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMin.z);
    ASLgl::glVertex3f(vMax.x, vMax.y, vMax.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMin.z);
    ASLgl::glVertex3f(vMin.x, vMax.y, vMax.z);
    ASLgl::glEnd();

    ASLgl::glEnable(GL_LIGHTING);
    ASLgl::glEnable(GL_TEXTURE_2D);
}

// Applies a position and orientation to the current matrix stack, as a ready 4x4 or as the axis-angle form.
#ifndef _MSC_VER
__attribute__((used)) inline
#endif
void GLRender::Transform(const Vector& v, const Quaternion& qRot, int n)
{
    static float m[16];

    if ((multmatrix | n) == 0)
        return;

#if !KOTOR1  // K1 keeps no draw counters
    counttrans++;
#endif

    if (multmatrix >= 1)
    {
        MakeMatrix4x4(m, v, qRot);
        ASLgl::glMultMatrixf(m);
        return;
    }

    static Vector p;
    static Quaternion q;

    p = v;
    ASLgl::glTranslatef(p.x, p.y, p.z);
    q = qRot;

    float fAngle = q.angle();
    Vector vAxis = q.axis();
    ASLgl::glRotatef(fAngle * 57.295776f, vAxis.x, vAxis.y, vAxis.z);
}
