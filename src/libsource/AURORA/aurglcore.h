
// The shared GLRender surface, the statics the render passes and the header-defined VertexPrimitive
// hierarchy call into. Declarations only; the bodies and the engine globals live in the .cpp files.

#ifndef GLRENDER_H
#define GLRENDER_H
#include "k2stdint.h"
#include "port/aslglapi.h"
#include "Vector.h"
#include "aurblendmodes.h"
#include "List.h"
#include "aurtexman.h"
#include "aurtexcontrol.h"

namespace Aurora { enum AuroraBufferRequestType { }; }

// The engine's render-state enums. Each value indexes the translation table of the same family
// (aurglcore.cpp) that holds the GL constant it stands for.
namespace Aurora {
enum AuroraEnableEnums {        // enableTranslationTable
    kCapBlend = 0,
    kCapAlphaTest = 1,
    kCapFog = 2,
    kCapCullFace = 3,
    kCapTexture2D = 4,
    kCapLighting = 5,
    kCapTexGenS = 6,
    kCapTexGenT = 7,
    kCapTexGenR = 8,
    kCapVertexProgram = 9,
    kCapStencilTest = 10,
    kCapDepthTest = 11,
    kCapTextureCubeMap = 12,
    kCapNormalize = 13,
    kCapFogStart = 15,          // 15-18 are glFog parameter names, not capabilities
    kCapFogEnd = 16,
    kCapFogDensity = 17,
    kCapFogColor = 18,
#if !KOTOR1
    kCapMultisample = 19,
    kCapTexture3D = 20,
#endif
};
enum AuroraClientStates {       // clientStateTranslationTable
    kClientStateTexCoord = 0,
    kClientStateNormal = 1,
    kClientStateColor = 2,
    kClientStateVertex = 3,
    kClientStateAttrib1 = 4,    // the NV vertex-attribute arrays 1, 3, 4, 5, 12, 13, 14, 15
    kClientStateAttrib3 = 5,
    kClientStateAttrib4 = 6,
    kClientStateAttrib5 = 7,
    kClientStateAttrib12 = 8,
    kClientStateAttrib13 = 9,
    kClientStateAttrib14 = 10,
    kClientStateAttrib15 = 11,
};
enum AuroraTransStates {        // transformTranslationTable
    kMatrixProjection = 0,
    kMatrixModelView = 1,
    kMatrixModelView2 = 2,
    kMatrixTexture = 3,
};
enum AuroraFacingModes {        // facingTranslationTable
    kFacingFront = 0,
    kFacingBack = 1,
    kFacingFrontAndBack = 2,
};
enum AuroraFillModes {          // fillTranslationTable
    kFillPoint = 0,
    kFillLine = 1,
    kFillSolid = 2,
};
enum AuroraCullModes {          // cullTranslationTable (glFrontFace)
    kCullCW = 0,
    kCullCCW = 1,
};
enum AuroraStencilOps {         // stencilOpTranslationTable
    kStencilKeep = 0,
    kStencilZero = 1,
    kStencilReplace = 2,
    kStencilIncr = 3,
    kStencilDecr = 4,
    kStencilInvert = 5,
};
enum AuroraCmpFuncs {           // cmpTranslationTable
    kCmpNever = 0,
    kCmpLess = 1,
    kCmpEqual = 2,
    kCmpLEqual = 3,
    kCmpGreater = 4,
    kCmpNotEqual = 5,
    kCmpGEqual = 6,
    kCmpAlways = 7,
};
enum AuroraFogModes {           // fogModeTranslationTable
    kFogLinear = 0,
    kFogExp = 1,
    kFogExp2 = 2,
};
enum AuroraMaterialProperties { // materialTranslationTable
    kMaterialShininess = 0,
    kMaterialAmbient = 1,
    kMaterialDiffuse = 2,
    kMaterialEmission = 3,
    kMaterialSpecular = 4,
};
enum AuroraColorTrackingModes { // colorTrackingTranslationTable
    kColorTrackingEmission = 0,
    kColorTrackingAmbient = 1,
    kColorTrackingDiffuse = 2,
    kColorTrackingSpecular = 3,
    kColorTrackingAmbientAndDiffuse = 4,
};
enum AuroraLightProperties {    // lightTranslationTable
    kLightAmbient = 0,
    kLightDiffuse = 1,
    kLightSpecular = 2,
    kLightPosition = 3,
    kLightQuadraticAttenuation = 4,
};
enum AuroraQueryEnums {         // queryTranslationTable
    kQueryFog = 0,
    kQueryUnmapped = 1,         // both images' table holds 0 here, which no glGet accepts
};
enum AuroraTexParamTargets {    // texParamTargetTranslationTable
    kTexTarget2D = 0,
    kTexTargetCubeMap = 1,
    kTexTargetCubeMapPosX = 2,  // 2-7: the six cube faces, +X -X +Y -Y +Z -Z
    kTexTargetCubeMapNegX = 3,
    kTexTargetCubeMapPosY = 4,
    kTexTargetCubeMapNegY = 5,
    kTexTargetCubeMapPosZ = 6,
    kTexTargetCubeMapNegZ = 7,
};
enum AuroraTexParamModes {      // texParamTranslationTable
    kTexParamWrapS = 0,
    kTexParamWrapT = 1,
    kTexParamMagFilter = 2,
    kTexParamMinFilter = 3,
};
enum AuroraTexCoordMappingModes {   // GLRender::SetTextureCoordGenerationState
    kTexCoordGenReflectionMap = 0,  // S, T and R from the reflection vector (cube maps)
    kTexCoordGenSphereMap = 1,      // S and T sphere-mapped
};
enum AuroraTextureFormats : int {  // textureFormatTranslationTable (9 and 10 have no entry)
    kTexFormatRGBA = 0,
    kTexFormatRGB = 1,
    kTexFormatLuminanceAlpha = 2,
    kTexFormatLuminance = 3,
    kTexFormatBGRA = 4,
    kTexFormatCompressedLuminance = 5,
    kTexFormatCompressedLuminanceAlpha = 6,
    kTexFormatDXT1 = 7,
    kTexFormatDXT5 = 8,
};
enum AuroraPrimitiveTypes {     // primTypeTranslationTable
    kPrimPoints = 0,
    kPrimLines = 1,
    kPrimLineStrip = 2,
    kPrimTriangles = 3,
    kPrimTriangleStrip = 4,
    kPrimTriangleFan = 5,
    kPrimQuads = 6,
    kPrimQuadStrip = 7,
    kPrimPolygon = 8,
    kPrimLineLoop = 9,
};
}

extern unsigned int enableTranslationTable[];
extern const unsigned int clientStateTranslationTable[];
extern const int transformTranslationTable[4];
extern const int facingTranslationTable[3];
extern const int fillTranslationTable[3];
extern const unsigned int cullTranslationTable[];
extern const unsigned int stencilOpTranslationTable[];
extern const unsigned int cmpTranslationTable[];
extern const unsigned int fogModeTranslationTable[];
extern const unsigned int materialTranslationTable[];
extern const unsigned int colorTrackingTranslationTable[];
extern const unsigned int lightTranslationTable[];
extern const int queryTranslationTable[2];
extern const unsigned int primTypeTranslationTable[];
extern const unsigned int texParamTargetTranslationTable[];
extern const unsigned int texParamTranslationTable[];
extern const unsigned int textureFormatTranslationTable[];

int AurMultiTextureAvailable();
extern int usearbvertexprograms;

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

#if !KOTOR1
extern unsigned char g_useDistortionOutput;
#endif

// The depth-mask wrapper. While distortion output is on, the GL backend owns the mask; K1 has no
// distortion output.
class RenderInterface {
public:
    static void SetDepthMask(unsigned char bMask)
    {
#if !KOTOR1
        if (!g_useDistortionOutput)
#endif
            ASLgl::glDepthMask(bMask);
    }
};

class GLRender {
public:
    // The state wrappers: each translates its Aurora enum through the family's table.
    static void Enable(Aurora::AuroraEnableEnums eCap) { ASLgl::glEnable(enableTranslationTable[eCap]); }
    static void Disable(Aurora::AuroraEnableEnums eCap) { ASLgl::glDisable(enableTranslationTable[eCap]); }
    static unsigned char IsEnabled(Aurora::AuroraEnableEnums eCap)
    {
        return ASLgl::glIsEnabled(enableTranslationTable[eCap]);
    }
    static void EnableClientState(Aurora::AuroraClientStates eState)
    {
        ASLgl::glEnableClientState(clientStateTranslationTable[eState]);
    }
    static void DisableClientState(Aurora::AuroraClientStates eState)
    {
        ASLgl::glDisableClientState(clientStateTranslationTable[eState]);
    }
    static void SetBlendFunc(Aurora::AuroraBlendModes eSrc, Aurora::AuroraBlendModes eDst)
    {
        ASLgl::glBlendFunc(blendTranslationTable[eSrc], blendTranslationTable[eDst]);
    }
    static void EnableColorMaterial() { ASLgl::glEnable(GL_COLOR_MATERIAL); }
    static void DisableColorMaterial() { ASLgl::glDisable(GL_COLOR_MATERIAL); }
    static void EnableLight(int nLight) { ASLgl::glEnable(GL_LIGHT0 + nLight); }
    static void DisableLight(int nLight) { ASLgl::glDisable(GL_LIGHT0 + nLight); }
    static void SetLightProperties(unsigned int nLight, Aurora::AuroraLightProperties eProperty, float* pfValue)
    {
        ASLgl::glLightfv(GL_LIGHT0 + nLight, lightTranslationTable[eProperty], pfValue);
    }
    static void GetClearColor(float* pfColor) { ASLgl::glGetFloatv(GL_COLOR_CLEAR_VALUE, pfColor); }
    static void GetStencilBits(int* pnBits) { ASLgl::glGetIntegerv(GL_STENCIL_BITS, pnBits); }
    // The bitmap console font (aurfont.cpp): the 128 display lists and one string drawn in
    // screen-normalised coordinates.
    static void SetUpTextFormat();
    static void FreeTextFont();
    static void PrintStringAtScreenPos(char* pszText, float fX, float fY);
    static void SetMatrixMode(Aurora::AuroraTransStates eMode)
    {
        ASLgl::glMatrixMode(transformTranslationTable[eMode]);
    }
    static void GetViewPort(int* pnViewport) { ASLgl::glGetIntegerv(GL_VIEWPORT, pnViewport); }
    // Reads back GL_UNSIGNED_BYTE texels in the given format.
    static void ReadPixelsFromBuffer(int nX, int nY, int nWidth, int nHeight, Aurora::AuroraTextureFormats eFormat,
                                     void* pData)
    {
        ASLgl::glReadPixels(nX, nY, nWidth, nHeight, textureFormatTranslationTable[eFormat], GL_UNSIGNED_BYTE,
                            pData);
    }
    static void QueryIntegerv(Aurora::AuroraQueryEnums eQuery, int* pnOut)
    {
        ASLgl::glGetIntegerv(queryTranslationTable[eQuery], pnOut);
    }
    static void SetPolygonRenderMode(Aurora::AuroraFacingModes eFacing, Aurora::AuroraFillModes eFill)
    {
        ASLgl::glPolygonMode(facingTranslationTable[eFacing], fillTranslationTable[eFill]);
    }
    static void SetFrontFace(Aurora::AuroraCullModes eCull) { ASLgl::glFrontFace(cullTranslationTable[eCull]); }
    static void SetStencilOp(Aurora::AuroraStencilOps eFail, Aurora::AuroraStencilOps eZFail,
                             Aurora::AuroraStencilOps eZPass)
    {
        ASLgl::glStencilOp(stencilOpTranslationTable[eFail], stencilOpTranslationTable[eZFail],
                           stencilOpTranslationTable[eZPass]);
    }
    static void SetStencilFunc(Aurora::AuroraCmpFuncs eFunc, int nRef, unsigned int nMask)
    {
        ASLgl::glStencilFunc(cmpTranslationTable[eFunc], nRef, nMask);
    }
    static void SetAlphaFunc(Aurora::AuroraCmpFuncs eFunc, float fRef)
    {
        ASLgl::glAlphaFunc(cmpTranslationTable[eFunc], fRef);
    }
    static void SetDepthFunc(Aurora::AuroraCmpFuncs eFunc) { ASLgl::glDepthFunc(cmpTranslationTable[eFunc]); }
    static void SetFogMode(Aurora::AuroraFogModes eMode)
    {
        ASLgl::glFogi(GL_FOG_MODE, fogModeTranslationTable[eMode]);
    }
    static void SetFogModef(Aurora::AuroraEnableEnums eParam, float fValue)
    {
        ASLgl::glFogf(enableTranslationTable[eParam], fValue);
    }
    static void SetFogModev(Aurora::AuroraEnableEnums eParam, Vector& vValue)
    {
        ASLgl::glFogfv(enableTranslationTable[eParam], &vValue.x);
    }
    static void SetLightModel(const float* pfAmbient) { ASLgl::glLightModelfv(GL_LIGHT_MODEL_AMBIENT, pfAmbient); }
    static void SetColorMaterial(Aurora::AuroraFacingModes eFacing, Aurora::AuroraColorTrackingModes eMode)
    {
        ASLgl::glColorMaterial(facingTranslationTable[eFacing], colorTrackingTranslationTable[eMode]);
    }
    static void SetMaterialv(Aurora::AuroraFacingModes eFacing, Aurora::AuroraMaterialProperties eProperty,
                             const float* pfValue)
    {
        ASLgl::glMaterialfv(facingTranslationTable[eFacing], materialTranslationTable[eProperty], pfValue);
    }
    static void BeginImmediatePrimitive(Aurora::AuroraPrimitiveTypes eType)
    {
        ASLgl::glBegin(primTypeTranslationTable[eType]);
    }
    // nColor is packed AABBGGRR; glColor4ubv reads it in memory order.
    static void SetVertexColor(unsigned int nColor)
    {
        ASLgl::glColor4ubv(reinterpret_cast<const unsigned char*>(&nColor));
    }
    static void SetVertexShaderConstant(int nIndex, float fX, float fY, float fZ, float fW)
    {
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIndex, fX, fY, fZ, fW);
    }
    // The client-side (vertex array) unit, then the server-side (texture environment) unit.
    static void ActivateTextureStage(unsigned int nStage)
    {
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE0 + nStage);
        cm_uiCurrentStage = nStage;
    }
    static void SetActiveTexture(unsigned int nStage)
    {
        cm_uiCurrentStage = nStage;
        if (AurMultiTextureAvailable())
            ASLgl::glActiveTextureARB(cm_uiCurrentStage + GL_TEXTURE0);
    }
    static void BindTexture(Aurora::AuroraEnableEnums eTarget, unsigned int nTID)
    {
        ASLgl::glBindTexture(enableTranslationTable[eTarget], nTID);
#if !KOTOR1
        ASLgl::glTexParameteri(enableTranslationTable[eTarget], GL_TEXTURE_MAX_ANISOTROPY_EXT, cm_nAnisotropy);
#endif
        cm_nGLCurrentTID[cm_uiCurrentStage] = nTID;
    }
    // nValue: 1 clamp-to-edge, 2 clamp, 3 repeat, 4 nearest, 5 linear, 6 linear-mipmap-linear,
    // 7 nearest-mipmap-nearest; anything else clamps.
    static void SetTextureParameteri(Aurora::AuroraTexParamTargets eTarget, Aurora::AuroraTexParamModes eMode,
                                     int nValue)
    {
        int nGLValue;
        switch (nValue)
        {
        case 1: nGLValue = GL_CLAMP_TO_EDGE; break;
        case 2: nGLValue = GL_CLAMP; break;
        case 3: nGLValue = GL_REPEAT; break;
        case 4: nGLValue = GL_NEAREST; break;
        case 5: nGLValue = GL_LINEAR; break;
        case 6: nGLValue = GL_LINEAR_MIPMAP_LINEAR; break;
        case 7: nGLValue = GL_NEAREST_MIPMAP_NEAREST; break;
        default: nGLValue = GL_CLAMP; break;
        }
        ASLgl::glTexParameteri(texParamTargetTranslationTable[eTarget], texParamTranslationTable[eMode], nGLValue);
    }
    // Uploads one level as GL_UNSIGNED_BYTE texels and re-applies the anisotropy.
    static void CreateTextureImageRaw(Aurora::AuroraTexParamTargets eTarget, int nLevel, int nInternalFormat,
                                      int nWidth, int nHeight, int nBorder, Aurora::AuroraTextureFormats eFormat,
                                      const void* pData)
    {
        ASLgl::glTexImage2D(texParamTargetTranslationTable[eTarget], nLevel, nInternalFormat, nWidth, nHeight,
                            nBorder, textureFormatTranslationTable[eFormat], GL_UNSIGNED_BYTE, pData);
        ASLgl::glTexParameteri(texParamTargetTranslationTable[eTarget], GL_TEXTURE_MAX_ANISOTROPY_EXT,
                               cm_nAnisotropy);
    }
    // Replaces a whole level from its origin.
    static void CreateTexSubImage(Aurora::AuroraTexParamTargets eTarget, int nLevel, int nWidth, int nHeight,
                                  Aurora::AuroraTextureFormats eFormat, const void* pData)
    {
        ASLgl::glTexSubImage2D(texParamTargetTranslationTable[eTarget], nLevel, 0, 0, nWidth, nHeight,
                               textureFormatTranslationTable[eFormat], GL_UNSIGNED_BYTE, pData);
    }
    static void SetTextureCoordGenerationState(Aurora::AuroraTexCoordMappingModes eMode)
    {
        if (eMode == Aurora::kTexCoordGenReflectionMap)
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
            ASLgl::glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
        }
        else
        {
            ASLgl::glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            ASLgl::glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
        }
    }
    static void DrawFromVerts(Aurora::AuroraPrimitiveTypes eType, List<Vector>& verts)
    {
        ASLgl::glBegin(primTypeTranslationTable[eType]);
        for (int i = 0; i < verts.m_count; i++)
            ASLgl::glVertex3fv(&verts.m_pData[i].x);
        ASLgl::glEnd();
    }
#if !KOTOR1  // K1 has no texture controllers on CAurTexture
    // The dirt texture's noise formula, 0 while it has no controller.
    static int GetNoiseFormula(int nIndex)
    {
        TextureController* pController = m_pDirtyTexture[nIndex]->GetReferencedTexture()->GetController();
        if (pController)
            return static_cast<DirtyTextureController*>(pController)->m_nFormula;
        return 0;
    }
#endif

    // The ARB vertex-program wrappers keep their NV names; each is a no-op without ARB vertex programs.
    static unsigned char IsProgramNV(unsigned int nProgram)
    {
        if (usearbvertexprograms)
            return ASLgl::glIsProgramARB(nProgram);
        return 0;
    }
    static void GenProgramsNV(unsigned int nCount, unsigned int* pnPrograms)
    {
        if (usearbvertexprograms)
            ASLgl::glGenProgramsARB(nCount, pnPrograms);
    }
    static void BindVertexProgram(int nProgram)
    {
        if (usearbvertexprograms)
            ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, nProgram);
    }
    static void DeleteProgramsNV(unsigned int nCount, unsigned int* pnPrograms)
    {
        if (usearbvertexprograms)
            ASLgl::glDeleteProgramsARB(static_cast<int>(nCount), pnPrograms);
    }
    // A non-normalised vertex-attribute array.
    static void SetVertexProgramAttrib(unsigned int nIndex, unsigned int nSize, unsigned int nType,
                                       unsigned int nStride, const void* pData)
    {
        if (usearbvertexprograms)
            ASLgl::glVertexAttribPointerARB(nIndex, static_cast<int>(nSize), nType, 0, static_cast<int>(nStride), pData);
    }
    // The program id and the declaration (a Win32 DWORD array, `unsigned long const*` in the armv7
    // mangling) are unused: the text loads into the bound program.
    static void LoadProgramNV(unsigned int nProgram, int nLength, const unsigned char* pProgram,
                              const unsigned int* pDeclaration)
    {
        if (usearbvertexprograms)
            ASLgl::glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, nLength, pProgram);
    }

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
