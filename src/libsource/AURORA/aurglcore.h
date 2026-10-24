
// The shared GLRender surface, the statics the render passes and the header-defined VertexPrimitive
// hierarchy call into. Declarations only; the bodies and the engine globals live in the .cpp files.

#ifndef GLRENDER_H
#define GLRENDER_H
#include "k2stdint.h"

namespace Aurora { enum AuroraBufferRequestType { }; }
namespace Aurora { enum AuroraPrimitiveTypes { }; }

// The interleaved vertex records the GL pools hold. They stay trivially constructible, with raw
// float triples rather than Vectors: a Vector would add a zero-init loop to every pool new[].
struct RenderVertexVTC {      // 24 B: pos[3], packed AABBGGRR colour, u, v
    float pos[3];
    unsigned int color;
    float u, v;
};

struct RenderVertexVT {       // 20 B
    float pos[3];
    float u, v;
};

struct RenderVertexVC {       // 16 B
    float pos[3];
    unsigned int color;       // packed AABBGGRR; the shadow disc only ever writes the alpha byte
};

struct RenderVertexVTN {      // 32 B: pos[3], normal[3], u, v
    float pos[3];
    union {
        float normal[3];
        int   m_nNormal[3];   // the grass copies a face normal's bits
    };
    float u, v;
};

struct RenderVertexV;
struct RenderVertexC;
class MaxFace;

class MdlNodeTriMesh;
class CAurTexture;

struct HDC__;
class HGLRC__;
class HPBUFFERARB__;
#if KOTOR1
struct PBUFFERARB;  // K1's GLES pbuffer record
#endif

struct Vector;
struct Quaternion;
struct Plane;
template <class T> class List;

class GLRender {
public:
    // Vertex-pool family. RequestPool's fifth argument is an out-pointer written with the pool
    // handle; LockPool returns the locked base pointer.
    static unsigned int RequestPool(Aurora::AuroraBufferRequestType eType, unsigned int a,
                                    unsigned int b, unsigned int c, unsigned int* pnOut);
    static void* LockPool(unsigned int nPool, unsigned int a, unsigned int b, unsigned int c);
    static void UnlockPool(unsigned int nPool);
#ifndef K2_MOBILE_OPT
    // Not in the image. An unlock whose upload waits for the pool's next draw, for the restored
    // dangly solver, which rewrites one node's slice of a model pool every frame.
    static void UnlockPoolDeferred(unsigned int nPool);
#endif
    static void DeletePool(unsigned int nPool);

    static unsigned int GetPoolSize(unsigned int nPool);
    static void ResizePool(unsigned int nPool, unsigned int nSize);

    static void RenderObjectsVTC(Aurora::AuroraPrimitiveTypes eType, unsigned int nPool, int nCount);

    static void DrawElements(Aurora::AuroraPrimitiveTypes eType, unsigned int nCount,
                             unsigned int nOffset, unsigned int nPool);

    static void SetLightmapTexCoordToStage0(unsigned int nStride, int nFlags,
                                            unsigned int nPool, int nGate,
                                            MdlNodeTriMesh* pNode, unsigned int nGeomBase);
    static void SetDecalTexCoordToStage0(unsigned int nStride, int nFlags,
                                         unsigned int nPool, int nGate,
                                         MdlNodeTriMesh* pNode, unsigned int nGeomBase);

    static void SetTexCoordBuffer(unsigned int nStride, unsigned int nOffset, unsigned int nPool, int nCount);
    static void SetNormalBuffer(unsigned int nStride, unsigned int nOffset, unsigned int nPool, int nCount);
    static void SetVertexBuffer(unsigned int nStride, unsigned int nOffset, unsigned int nPool, int nCount);

    static void SetColorBuffer(unsigned int nStride, unsigned int nOffset, unsigned int nPool, int nCount);
    static void SetInterleavedBuffer(unsigned int nStride, int nFlags, unsigned int nPool, int nCount,
                                     MdlNodeTriMesh* pNode, unsigned int nBase);
    static unsigned int GetFogArraysInfoRef();

    static int GetAvailableTextureMemory();

    // Presents the frame and resets the ARB program state; defined in swmain/main.cpp.
    static void SwapBuffers(HDC__* hDC);

    static unsigned int cm_uiCurrentStage;

    // The pixel-shader modes pfuncEnablePixelShaders installs. A mode with its own fragment program
    // binds the program of the same number. EM/LM name an environment-map/lightmap variant, and the
    // Noise families come in subtract/add/multiply triples (a model's noise formula is the offset).
    // The ...Off modes take a family's texture units back down. K1 ends the list after
    // kPSEnvMapLightmapOff, so its last six modes have smaller numbers.
    enum PixelShader {
        kPSNone = 0,
        kPSDecal = 1,
        kPSFog = 2,
        kPSStandardFog = 3,
        kPSPhong = 4,
        kPSBumpmap = 6,
        kPSNormalmap = 7,
        kPSLightmapBumpDiffuse = 8,
        kPSLightmapBumpSpecular1 = 9,
        kPSLightmapBumpSpecular2 = 10,
        kPSLightmapBumpSpecular4 = 11,
        kPSLightmapBumpSpecular8 = 12,
        kPSBumpmapOff = 13,
        kPSDiffuseBumpPass = 15,
        kPSSpecularBumpPass = 18,
        kPSEnvMapCube = 19,
        kPSEnvMapFog = 20,
        kPSEnvMapLightmapNV1x = 21,
        kPSEnvMapLightmap = 22,
        kPSSkinBump = 23,
        kPSBumpyShiny = 24,
        kPSLightmap = 26,
        kPSBumpyShinyOff = 28,
        kPSBumpyShinyTextures = 29,
        kPSMultiTextureOff = 32,
        kPSMotionBlur = 34,
        kPSEnvMapLightmapOff = 37,

        // K2 only from here to kPSPostDistort.
        kPSNoiseSubtract = 38,
        kPSNoiseAdd = 39,
        kPSNoiseMultiply = 40,
        kPSNoiseEMSubtract = 41,
        kPSNoiseEMAdd = 42,
        kPSNoiseEMMultiply = 43,
        kPSNoiseLMSubtract = 44,
        kPSNoiseLMAdd = 45,
        kPSNoiseLMMultiply = 46,
        kPSNoiseEMLMSubtract = 47,
        kPSNoiseEMLMAdd = 48,
        kPSNoiseEMLMMultiply = 49,
        kPSFGlow = 50,
        kPSFGlowLM = 51,
        kPSFGlowEM = 52,
        kPSFGlowEMLM = 53,
        kPSFGlowBumpmap = 54,
        kPSFGlowBumpyShiny = 55,
        kPSDesat = 56,
        kPSDesatLM = 57,
        kPSDesatEM = 58,
        kPSDesatEMLM = 59,
        kPSDesatBumpmap = 60,
        kPSDesatBumpyShiny = 61,
        kPSDesatNoiseSubtract = 62,
        kPSDesatNoiseAdd = 63,
        kPSDesatNoiseMultiply = 64,
        kPSDesatNoiseEMSubtract = 65,
        kPSDesatNoiseEMAdd = 66,
        kPSDesatNoiseEMMultiply = 67,
        kPSDesatNoiseLMSubtract = 68,
        kPSDesatNoiseLMAdd = 69,
        kPSDesatNoiseLMMultiply = 70,
        kPSDesatNoiseEMLMSubtract = 71,
        kPSDesatNoiseEMLMAdd = 72,
        kPSDesatNoiseEMLMMultiply = 73,
        kPSDesatOff = 74,
        kPSHolo = 75,
        kPSHoloDesat = 76,
        kPSHoloOff = 77,
        kPSIllum = 78,
        kPSIllumEM = 79,
        kPSIllumNoiseSubtract = 80,
        kPSIllumNoiseAdd = 81,
        kPSIllumNoiseMultiply = 82,
        kPSIllumNoiseEMSubtract = 83,
        kPSIllumNoiseEMAdd = 84,
        kPSIllumNoiseEMMultiply = 85,
        kPSIllumNoiseLMSubtract = 86,
        kPSIllumNoiseLMAdd = 87,
        kPSIllumNoiseLMMultiply = 88,
        kPSIllumNoiseEMLMSubtract = 89,
        kPSIllumNoiseEMLMAdd = 90,
        kPSIllumNoiseEMLMMultiply = 91,
        kPSIllumDesat = 92,
        kPSIllumDesatLM = 93,
        kPSIllumDesatEM = 94,
        kPSIllumDesatEMLM = 95,
        kPSIllumDesatBumpmap = 96,
        kPSIllumDesatBumpyShiny = 97,
        kPSIllumDesatNoiseSubtract = 98,
        kPSIllumDesatNoiseAdd = 99,
        kPSIllumDesatNoiseMultiply = 100,
        kPSIllumDesatNoiseEMSubtract = 101,
        kPSIllumDesatNoiseEMAdd = 102,
        kPSIllumDesatNoiseEMMultiply = 103,
        kPSIllumDesatNoiseLMSubtract = 104,
        kPSIllumDesatNoiseLMAdd = 105,
        kPSIllumDesatNoiseLMMultiply = 106,
        kPSIllumDesatNoiseEMLMSubtract = 107,
        kPSIllumDesatNoiseEMLMAdd = 108,
        kPSIllumDesatNoiseEMLMMultiply = 109,

        // K2's iOS post-process (K1's build compiles the distortion pass too).
        kPSPostBloomSource = 112,       // the scene into the bloom pbuffer
        kPSPostDistort = 114,

#if KOTOR1
        kPSDefault = 38,
        kPSBaseLightmap = 39,
        kPSPostPresent = 40,            // the iOS post-process: saturation, noise and the present
        kPSPostBlur = 41,               // the iOS bloom blur
        kPSFixedFunction = 42,
        kPSEnvMap = 43,
#else
        kPSDefault = 110,
        kPSPostPresent = 113,           // the iOS post-process: gamma, noise and the present
        kPSPostBlur = 115,              // the bloom blur
        kPSFixedFunction = 116,
        kPSEnvMap = 117,
#endif
    };

    static void (*pfuncEnablePixelShaders)(int mode, void* param, void*, void*, void*);
    static void (*pfuncSetPixelShaderAmbientColor)(Vector& vColor, float fAlpha);
    static void (*pfuncSetBumpLightColor)(Vector& vColor, float fAlpha);
    static int cm_nGLCurrentTID[6];
    static unsigned int cm_nAnisotropy;

    static unsigned int cm_tidAttenuationTexture;

    struct PerInstanceColor { float r, g, b, a; };
    static PerInstanceColor m_PerInstanceIllumination;

    static PerInstanceColor m_ForceSightColor;

    static void CreateTexture(CAurTexture* pTexture, bool bParam, int nIndex);
    static void CreateCubeTexture(CAurTexture* pTexture, bool bParam, int nIndex);
    static void CreateProcessedTexture(CAurTexture* pTexture, bool bParam, int nIndex);
    static void CreateProcessedCubeTexture(CAurTexture* pTexture, bool bParam, int nIndex);

    static void MakeNormalizationCubeMap(int nSize);

    static void SetCurrentStageTID(unsigned int nStage, unsigned int nTID);
    static void InitializeATIFragmentShaders();
    static void SetFragmentShaderConstant0(Vector& v, float f);
    static void SetFragmentShaderConstant1(Vector& v, float f);

    static void EnableFragmentShaders(int nShaderID, CAurTexture* pTex0, CAurTexture* pTex1,
                                      CAurTexture* pTex2, CAurTexture* pTex3);
    static void EnableFragmentShadersARB(int nShaderID, CAurTexture* pTex0, CAurTexture* pTex1,
                                         CAurTexture* pTex2, CAurTexture* pTex3);

    static void MakeLookUpTexture(float fSize);

    static int SetUpImposters(int nCount, float* pOffsets);

    static CAurTexture* m_pHologramFilterTexture;

    static void ConvertHeightFieldAndLoadNormalMapTexture(unsigned char* pData, int nWidth, int nHeight,
                                                          int nSrcWidth, int nSrcHeight, float fScale,
                                                          int nUseSubImage, bool bCopyField);

    static void Transform(const Vector& v, const Quaternion& q, int n);

    static void Shutdown(int nFlag);

    static void RenderObjectsVTQuad(unsigned int nPool, int nCount);

    static void RenderObjectsVTCQuadUP(RenderVertexVTC* paVertices, int nCount);
    static void RenderObjectsVTQuadUP(RenderVertexVT* paVertices, int nCount);

    static void RenderObjectsVTCQuad(unsigned int nPool, int nCount);
    static void RenderObjectsVTQuadStrip(unsigned int nPool, int nCount);
    static void RenderObjectsVTNQuadStrip(unsigned int nPool, int nCount);
    static void RenderObjectsVCUP(Aurora::AuroraPrimitiveTypes eType, RenderVertexVC* paVertices,
                                  int nCount);
    static void RenderObjectsVC(Aurora::AuroraPrimitiveTypes eType, unsigned int nPool,
                                 int nCount);
    static void RenderObjectsVCQuads(unsigned int nPool, int nOffset, int nCount);
    static void RenderObjectsVTN(Aurora::AuroraPrimitiveTypes eType, unsigned int nPool,
                                  int nCount);

    static void RenderObjectsV(Aurora::AuroraPrimitiveTypes eType, unsigned int nCount,
                                unsigned int nOffset, unsigned int nPool);
    static void RenderObjectsVNTCQuad(unsigned int nVertexPool, unsigned int nNormalPool,
                                      unsigned int nTexCoordPool, unsigned int nColorPool,
                                      int nCount);

    static void InitExtensions();
    static void InitRenderSystem();

    static void InitAttenuationTexture();
    static void RemoveSquareToRender(unsigned int nHandle);
    static void RemoveAllDrawableSquares();
    static void EnableDreamDistortion(bool bEnable);
    static void DisableDreamDistortion();
    static void EnableFury(short nLevel);

    static void DrawSphereBound(float fRadius, Vector& vCenter, const Vector& vColor,
                                float fAlpha, bool bMask);

    static void DrawTriangles(List<MaxFace>& faces, RenderVertexV* pVerts,
                              RenderVertexC* pColors);

    static void DrawAxisAlignedFrame(const Vector& vMin, const Vector& vMax,
                                     const Vector& vColor);

    static int SetSpecularMode(int nMode);

    static int cm_bEnablePausePlane;

    static unsigned short m_UsedVertices[4096];
    static void* cm_pUserShadowData;

    static void RenderShadowArray(int nStart, int nCount, int nUnused);

    static int BuildShadowArray(Vector const& vOrigin, float const& fHeight, Plane const& plane,
                                Vector const& vA, Vector const& vB, int nCount,
                                RenderVertexV* pVerts);
    static bool m_bSpeedBlurEnabled;
    static bool m_bSaturationEnabled;
    static bool m_bScanNoiseEnabled;
    static bool m_bFilmNoiseEnabled;
    static float m_fSaturation;
    static Vector m_vSaturationMultiplier;
    static float m_fAccumulationRatio;

    static bool cm_bVSync;
    static bool cm_bSetUpImageSpaceShadows;
    static bool cm_bSetUpFrameBuffer;

    static void* GetPool(unsigned int nPool);
    static void RefreshTemporary(unsigned int a, unsigned int b);
    static void DrawElementsInShadow(int nCount, unsigned int nPool,
                                     int nUnused);

    static void* m_hGLRC;
    static void* m_hDC;
    static void DrawElementStreamQuads(Aurora::AuroraPrimitiveTypes eType, unsigned int nCount,
                                       unsigned int nStart, bool bUnused);

    static void SetDirtyTexture(int nIndex, CAurTexture* pTexture);
    static CAurTexture* m_pDirtyTexture[3];

    static void SetFilmNoiseTexture(CAurTexture* pTexture);
    static CAurTexture* cm_pFilmNoiseTexture;

    static CAurTexture* cm_pForceDistortionTexture;

    static bool m_bDreamDistortEnabled;
    static bool m_bDreamDistortLetterbox;
    static CAurTexture* m_pDreamDistortTexture;

    static short m_FuryLevel;
    static CAurTexture* m_pFuryTexture;

    static List<unsigned int> cm_listSquaresToRender;
    static void AddSquareToRender(unsigned int nHandle);
    static void RenderSquares(unsigned int nHandle);
    static void RenderDrawableSquares();
    static void RenderBox(Vector& vMin, Vector& vMax);

    static void SetViewTransform(Vector& vPosition, Quaternion& qRotation);

    static unsigned int GetVertexArraysInfoRef(void* pInfo);

    static void SetupGrassBuffers(unsigned int nPool, int nOffset);

    static void* CreatePBufferRectangle(int nWidth, int nHeight);
    static void* CreatePBuffer(int nWidth, int nHeight);

    static void FinishUpSkinnedVertexArrays(unsigned int, unsigned int, unsigned int,
                                            unsigned int);
    static void UpdateLightVectorsBumped(unsigned int, const Vector&,
                                         MdlNodeTriMesh*);
    static void UpdateHalfAngleVectorsBumped(unsigned int, const Vector&, const Vector&,
                                             MdlNodeTriMesh*);
    static void UpdateFogValuesBumped(unsigned int, const Vector&, float,
                                      MdlNodeTriMesh*);
    // The modes the FrameBufferModificationPixelShaders family installs, numbered as decimal powers
    // of two spelled in hex. K1 has no dream distortion, and its decal-rectangle mode takes 0x1024.
    enum FrameBufferShader {
        kFBShaderFlare = 1,
        kFBShaderOff = 2,
        kFBShaderDecal = 4,
        kFBShaderDecal4 = 8,
        kFBShaderSaturation = 0x16,
        kFBShaderFilmNoise = 0x32,
        kFBShaderForceDistortion = 0x64,
        kFBShaderAccumulationBlur = 0x128,
        kFBShaderSoftShadow = 0x256,
        kFBShaderOverbrighten = 0x512,
#if KOTOR1
        kFBShaderDecalRect = 0x1024,
#else
        kFBShaderDreamDistort = 0x1024,
        kFBShaderDecalRect = 0x2048,
#endif
    };

    // The passes FrameBufferModifications runs, one bit each in its flags.
    enum FrameBufferPass {
        kFBPassOverbrighten = 0x1,
        kFBPassConvolution = 0x2,
        kFBPassNoise = 0x4,
        kFBPassSaturation = 0x8,
        kFBPassSpeedBlur = 0x10,
        kFBPassForceDistortion = 0x20,
        kFBPassAllButForceDistortion = 0x1f,
        kFBPassAll = 0x3f,
    };

    static void FrameBufferModificationPixelShaders(unsigned int, unsigned int, unsigned int,
                                                    unsigned int, unsigned int);

    static void FrameBufferModificationPixelShadersATI(unsigned int, unsigned int, unsigned int,
                                                       unsigned int, unsigned int);

    static void FrameBufferModificationPixelShadersARB(unsigned int, unsigned int, unsigned int,
                                                       unsigned int, unsigned int);
    static void InitializeImageSpaceSoftShadows();
    static void InitializeImageSpaceSoftShadowsARB();

    static void SetUpSoftShadowsATI();
    static void SetUpSoftShadowsARB();
    static void SetUpSoftShadows();

    static void FinishSoftShadowsATI();
    static void FinishSoftShadowsARB();
    static void FinishSoftShadows();
    static unsigned int cm_nBufferRegionTexture;

    static HDC__* cm_hSoftShadowHDC;
    static HGLRC__* cm_hSoftShadowHGLRC;
    static unsigned int cm_nSoftShadowRenderTexture;
    static HPBUFFERARB__* cm_hSoftShadowPBuffer;
    static HDC__* cm_hSoftShadowTargetHDC;
    static HGLRC__* cm_hSoftShadowTargetHGLRC;
    static unsigned int cm_nSoftShadowTargetRenderTexture;
    static HPBUFFERARB__* cm_hSoftShadowTargetPBuffer;

    static unsigned int s_ShadowKnockDownFBO;
    static unsigned int s_ShadowBlurFBO;

    static unsigned int cm_nScreenWidthPow2;
    static unsigned int cm_nScreenHeightPow2;

    static void EnableCombiners(int nShaderID, CAurTexture* pTex0, CAurTexture* pTex1,
                                CAurTexture* pTex2, CAurTexture* pTex3);

    static void DisableCombiners(CAurTexture* pTexture);

    static void RenderPausePlane();

    static void RenderOverlayOverbrighten(unsigned int nTexture);

    static void RenderGammaCorrection();

    static void* cm_hAccumulationPBuffer;
    static void* cm_hAccumulationHDC;
    static void* cm_hAccumulationHGLRC;
    static int cm_nAccumulationBlur;
    static bool cm_bPBuffersAllocated;
    static bool cm_bFirstFrameBufferFrame;

    static HPBUFFERARB__* cm_hConvolutionPBuffer[6];
    static HDC__* cm_hConvolutionHDC[6];
    static HGLRC__* cm_hConvolutionHGLRC[6];
    static unsigned int cm_nConvolutionRenderTexture[6];

    static unsigned int s_ConvolutionFBOs[6];
    static unsigned int s_ConvolutionRbs[6];
    static unsigned int s_AccumulationFBO;
    static unsigned int s_AccumulationRb;

    static unsigned int cm_nTargetTexture;
    static unsigned int cm_nRedTexture;
    static unsigned int cm_nGreenTexture;
    static unsigned int cm_nBlueTexture;
    static unsigned int cm_nOverbrightenRed;
    static unsigned int cm_nOverbrightenGreen;
    static unsigned int cm_nOverbrightenBlue;

    static unsigned int cm_pHeatDistortionVertexProg;

    static unsigned int cm_nPerPixelFlareShaderARB;

    static unsigned int cm_nPerPixelFlareShader;
    static unsigned int cm_nSaturationShaderARB;
    static unsigned int cm_nDecalShaderARB;
    static unsigned int cm_nDecalShaderRectARB;
    static unsigned int cm_n4DecalShaderARB;

    static unsigned int cm_nFilmNoiseShaderARB;
    static unsigned int cm_nForceDistortionShaderARB;
    static unsigned int cm_nAccumulationBlurShaderARB;
    static unsigned int cm_nSoftShadowShaderARB;
    static unsigned int cm_nOverbrightenShaderARB;
    static unsigned int cm_nDreamDistortShaderARB;

    static void RenderAccumulationBlur(unsigned int nTexture);
    static void RenderFrameToTexture(unsigned int nTexture);
    static void RenderOverlayConvolution(unsigned int nTexture);
    static void RenderForceDistortion(unsigned int nTexture);
    static void RenderOverlayFilmNoise(unsigned int nTexture, bool bScanNoise);
    static void RenderDreamDistortion(unsigned int nTexture);
    static void RenderFury(unsigned int nTexture);
    static void RenderAccumulationBlurATI(unsigned int nTexture);
    static void RenderFrameToTextureATI(unsigned int nTexture);
    static void RenderOverlayOverbrightenATI(unsigned int nTexture);
    static void RenderOverlayConvolutionATI(unsigned int nTexture);
    static void RenderForceDistortionATI(unsigned int nTexture);
    static void RenderOverlayFilmNoiseATI(unsigned int nTexture, bool bScanNoise);
    static void RenderDreamDistortionATI(unsigned int nTexture);
    static void RenderFuryATI(unsigned int nTexture);
    static void RenderSaturationAdjustmentATI(unsigned int nTexture);
    static void RenderSaturationAdjustment(unsigned int nTexture);

    static void FrameBufferModifications(uint64_t nFlags);
    static void FrameBufferModificationsATI(uint64_t nFlags);
    static void FrameBufferModificationsARB(uint64_t nFlags);

    static void FrameBufferModifications2(uint64_t nFlags);
    static void FrameBufferModificationsATI2(uint64_t nFlags);
    static void FrameBufferModificationsARB2(uint64_t nFlags);

    static void RenderAccumulationBlurARB(unsigned int nTexture);
    static void RenderDreamDistortionARB(unsigned int nTexture, bool bFury);
    static void RenderFrameToTextureARB(unsigned int nTexture);
    static void RenderForceDistortionARB(unsigned int nTexture);
    static void RenderSaturationAdjustmentARB(unsigned int nTexture);
    static void RenderOverlayOverbrightenARB(unsigned int nTexture);
    static void RenderOverlayConvolutionARB(unsigned int nTexture);
    static void RenderOverlayFilmNoiseARB(unsigned int nTexture, bool bScanNoise);

#if KOTOR1  // K1's eight pbuffer filters are empty, never-called stubs over its GLES record
    static void MipmapFilter(void* hDC, void* hGLRC, float fWidth, float fHeight,
                             PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize);
    static void BloomFilter(void* hDC, void* hGLRC, float fWidth, float fHeight,
                            PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize,
                            int bSoftShadow);
    static void MipmapFilterATI(void* hDC, void* hGLRC, float fWidth, float fHeight,
                                PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize);
    static void MipmapFilterARB(void* hDC, void* hGLRC, float fWidth, float fHeight,
                                PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize);
    static void BloomFilterATI(void* hDC, void* hGLRC, float fWidth, float fHeight,
                               PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize,
                               int bSoftShadow);
    static void BloomFilterARB(void* hDC, void* hGLRC, float fWidth, float fHeight,
                               PBUFFERARB* pPBuffer, unsigned int nTexture, float fSize,
                               int bSoftShadow);
    static void BloomFilterFromTexture(void* hDC, void* hGLRC, float fWidth, float fHeight,
                                       unsigned int nTexture, float fSizeU,
                                       float fSizeV);
    static void BloomFilterFromTextureATI(void* hDC, void* hGLRC, float fWidth, float fHeight,
                                          unsigned int nTexture, float fSizeU,
                                          float fSizeV);
#else
    static void MipmapFilter(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                             HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize);
    static void BloomFilter(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                            HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize,
                            int bSoftShadow);
    static void MipmapFilterATI(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                                HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize);
    static void MipmapFilterARB(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                                HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize);
    static void BloomFilterATI(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                               HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize,
                               int bSoftShadow);

    static void BloomFilterARB(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                               HPBUFFERARB__* pPBuffer, unsigned int nTexture, float fSize,
                               int bSoftShadow);

    static void BloomFilterFromTexture(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                                       unsigned int nTexture, float fSizeU,
                                       float fSizeV);

    static void BloomFilterFromTextureATI(HDC__* hDC, HGLRC__* hGLRC, float fWidth, float fHeight,
                                          unsigned int nTexture, float fSizeU,
                                          float fSizeV);
#endif

    // In the post-process setup family, the plain variant runs two sequential `if` blocks, the FBO
    // one falling through into the ATI one's own capability guard.
    static void DrawLightmappedGrass(void* pVertices, int nVertexStride, void* pUnk,
                                     int nNormalStride, int nStart, int nVertexCount);
    static void InitializeFrameBufferModificationFragmentShadersATI();

    static void InitializeFrameBufferModificationFragmentShadersARB();

    static void InitializeFrameBufferModifications();
    static void InitializeAntiAliasing();
    static void RenderPeripheralSmoke(unsigned int nTexture);

    static void InitializeFrameBufferModificationsARB();

    static void SetCombinerConstant0(Vector& vConstant, float fValue);
    static void SetCombinerConstant1(Vector& vConstant, float fValue);
};

#endif
