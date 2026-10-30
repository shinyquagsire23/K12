
#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurglcore.h"
#include "nullname.h"
#include "Vector.h"
#include "Quaternion.h"
#include "Plane.h"
#include "Matrix.h"
#include "math3d.h"
#include "AURORA/aurmodel.h"
#include "aurmaterial.h"
#include "aurscene.h"

int maxshadowvar;
int cullwithLOD;

int counttrans;
int countgobs;
int countfaces;
int countparts;
int countshadowfaces;
int countcullpart;
#if !KOTOR1
unsigned int g_SceneRender_CurrentTime_ms;
#endif




















void GetAllNamedTextures(Part* pPart, List<CAurTexture*>& out, char* pszName);

void PartSetFog(Part* pPart, bool bFog);
int  PartSetCanDownSample(Part* pPart, char c);

Animation* FindAnimation(Model* pModel, char* pszName);
#if KOTOR1  // K1: the model streams are SDL_RWops
struct SDL_RWops;
Model* LoadModel(SDL_RWops* pFile, SDL_RWops* pAux);
#else
Model* LoadModel(FILE* pFile, FILE* pAux);
#endif

Part* CreateInstanceTreeR(MdlNode* pNode, Gob* pGob);
void  PartInitMaterial(Gob* pGob, Part* pPart, char* pszName);
void  CreateReferenceObjects(Gob* pGob, Part* pPart);

#if KOTOR1  // K1: the model streams are SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

// Applies pNode's pose to pPart's subtree.
void AnimateHierarchy(Part* pPart, MdlNode* pNode, float a, float b, float c, bool d);

extern int removedelref;

#define AABB_DTOR_DECLARED_ONLY noexcept(false)
#include "AABB.h"
#include "PointerMap.h"


#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"

#include "port/crtapi.h"


void exportparts(Gob* pGob, Part* pPart);





int enabletransform = 1;

extern int systemmemoryshadows;

extern int quaternionidentityoptimization;

extern int counttrans;

void SetBeaming();

int rendermaxdepth = 999999;
extern Vector currentmat __attribute__((visibility("hidden")));




extern List<Gob*> gobs;















// Rotates a world-space point into the part's local frame (conjugate orientation).
static inline Vector WorldToLocal(const Vector& v, const Quaternion& q, const Vector& vOrigin)
{
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
    Quaternion nq = -q;
    float w = nq.w, x = nq.x, y = nq.y, z = nq.z;
    Vector p = { v.x - vOrigin.x, v.y - vOrigin.y, v.z - vOrigin.z };
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        Vector r;
        r.x = p.x * (1.0f - ((yy + zz) + (yy + zz))) + p.y * ((xy - wz) + (xy - wz)) + p.z * ((xz + wy) + (xz + wy));
        r.y = p.x * ((xy + wz) + (xy + wz)) + p.y * (1.0f - ((xx + zz) + (xx + zz))) + p.z * ((yz - wx) + (yz - wx));
        r.z = p.x * ((xz - wy) + (xz - wy)) + p.y * ((yz + wx) + (yz + wx)) + p.z * (1.0f - ((xx + yy) + (xx + yy)));
        p = r;
    }
    return p;
#else
    float w = q.w, x = q.x, y = q.y, z = q.z;
    float nw = -w;
    Vector p = { v.x - vOrigin.x, v.y - vOrigin.y, v.z - vOrigin.z };
    if (quaternionidentityoptimization == 0 || (w != -1.0f && w != 1.0f)) {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * nw;
        float yz = y * z, wy = y * nw;
        float wz = z * nw;
        Vector r;
        r.x = p.x * (1.0f - ((yy + zz) + (yy + zz))) + p.y * ((xy - wz) + (xy - wz)) + p.z * ((xz + wy) + (xz + wy));
        r.y = p.x * ((xy + wz) + (xy + wz)) + p.y * (1.0f - ((xx + zz) + (xx + zz))) + p.z * ((yz - wx) + (yz - wx));
        r.z = p.x * ((xz - wy) + (xz - wy)) + p.y * ((yz + wx) + (yz + wx)) + p.z * (1.0f - ((xx + yy) + (xx + yy)));
        p = r;
    }
    return p;
#endif
}















extern List<Gob*> gobs;

#include "aurconsole.h"









extern int quaternionidentityoptimization;


unsigned int BoxIntersect(const Vector& a, const Vector& b,
                          const Vector& boxMin, const Vector& boxMax);


Vector VectorMin(Vector a, Vector b);
Vector VectorMax(Vector a, Vector b);




















Gob* FindGob(char* pszName);
static void snprintff(char* buf, uint64_t n, const char* fmt, ...);






class CAurTexture;





extern int usevertexprograms;
int doskinshadows;
int useshadowmethod;
float epsilon;
int usearbvertexprograms = 1;
extern int  quaternionidentityoptimization;

Vector planelineintersection(const Vector& v, float f, const Vector& a, const Vector& b);
void SetupForceSightShader(Gob* gob, int a, int b, CAurTexture* c1, CAurTexture* c2,
                           CAurTexture* c3, CAurTexture* c4, bool flag);
void GatherBones(Part* part, List<Vector>& outVectors, List<Quaternion>& outQuats,
                 Vector pos, Quaternion orient);

int FindPartIndexDFS(Part* part, char* pszName);
void MakeInlinePartList(Part* part, List<Part*>& out);







extern float lightsabersegmentlife;


int lightsaberrender = 1;
int renderpieces = 1;
int lightsaberwireframe;
int lightsaberlighting;
int enablelightsaberbluradd;
float lightsabersegmentinterval = 0.00100000005f;
float lightsaberfudgefactor = 0.5f;
int lightsabertotalsegments;
int lightsaberrenderfronthalf = 1;


extern int quaternionidentityoptimization;

// The piece-cap vertex block holds 8 front-cap verts, 20*4 front ribbon, 8 back-cap, 20*4 back
// ribbon.
static const int kBackPieceBase = 88;


int renderdangly = 1;





bool AurNV2xLevelShadersAvailable();
int AurTextureGetTID(CAurTexture* pTexture);

int enablebumpmap = 1;
int enablelightmap = 1;

int rendertrimesh = 1;
int cullnonrenderable = 1;
int culltransparent;
class VertexProgram;
VertexProgram* g_pBumpyShinyPass2;
VertexProgram* g_pBumpyShinyPass3;
VertexProgram* g_pBumpyShinyPass4;
VertexProgram* g_pBumpyShinyPass5;
VertexProgram* g_pBumpedOutVP;

#if KOTOR1
void SetBumpedOutShellIos(float f);
void MakeMatrix4x4(float* pMatrix, const Vector& v, const Quaternion& q);
#endif

Vector g_vOne(1.0f, 1.0f, 1.0f);
Vector g_vZero(0.0f, 0.0f, 0.0f);
Vector projectioncolor(1.0f, 1.0f, 1.0f);
float projectionalpha = 0.200000003f;
int stenciloff = 1;
extern int enabletransform;
float* Homogenize(const Vector& v, const float& w);

#if defined(K2_QOL_BUMPMAPS) && KOTOR1
bool AurBumpMapCardValidate();
extern int enablebumpyshiny;
bool AurNonRadeon8500Validate();
#elif defined(K2_QOL_BUMPMAPS)
bool AurBumpMapCardValidate();
extern int enablebumpyshiny;

bool AuroraUseARBShaders();

// The noise path's cards. The PC asks for its frame-buffer modifications (render-to-texture and NV
// combiners2 or ATI fragment shaders), which every port answered no; here the ARB shaders, as the
// frame-buffer effects ask. A phone's GLES 2 has no 3D textures.
static bool QolNoiseMapAvailable()
{
#ifdef K2P_MOBILE_HOST
    return false;
#else
    return AuroraUseARBShaders();
#endif
}
#endif

// Picks the render pass id from the part's material maps, lightmap flag and env-map watcher.
int PartTriMesh::GetRenderPath()
{
    MdlNodeTriMesh* mesh = m_pNode->AsMdlNodeTriMesh();

    bool bBump = m_pMaterial->HasBumpMap();
    bool bNormal = m_pMaterial->HasNormalMap();
#ifdef K2_QOL_BUMPMAPS
    bool bBumpyShiny = m_pMaterial->HasBumpyShinyMap();
#else
    m_pMaterial->HasBumpyShinyMap();
#endif
    bool bTangent = mesh->HasTangentBases();
    bool bEnv = m_pMaterial->HasEnvMap();

    int nLightmap = mesh->m_bHasLightmap;
    float fAlpha = m_pMaterial->m_fWaterAlpha;
    if (m_pMaterial->m_spr1.m_pObject != 0 &&
        AurTextureGetTID(m_pMaterial->m_spr1.m_pObject) == 0)
        nLightmap = 0;
    if (!bEnv)
        bEnv = m_pGob != 0 && m_pGob->m_texWatch.m_pObject != 0;

#if defined(K2_QOL_BUMPMAPS) && KOTOR1
    // The PC's bumpy-shiny path (swkotor.exe GetRenderPath), taken first and whatever the water
    // alpha: the reflection is blended at that alpha (RenderEMBM).
    if (bBumpyShiny && bBump && bTangent && enablebumpyshiny &&
        AurNV2xLevelShadersAvailable() && AurBumpMapCardValidate() && AurNonRadeon8500Validate())
        return RENDER_PATH_EMBM;
#endif

#if defined(K2_QOL_BUMPMAPS) && !KOTOR1
    // The PC's order (swkotor2.exe GetRenderPath): the bumpy-shiny path first, whatever the water
    // alpha; the noise ("dirt") path; then the one-pass bump path, for a lightmapped mesh too (the
    // PC's lightmapped bump paths need NV vertex programs, which it never chose). No flat path for
    // a water alpha below 1: only the bumpy-shiny path reads it.
    if (bBumpyShiny && bBump && bTangent && enablebumpyshiny && AurNV2xLevelShadersAvailable())
        return RENDER_PATH_EMBM;
    if (mesh->m_bDirtEnabled && QolNoiseMapAvailable())
        return RENDER_PATH_NOISE_MAPPED;
    if (bBump && bTangent && enablebumpmap && AurNV2xLevelShadersAvailable())
        return AurBumpMapCardValidate() ? RENDER_PATH_BUMPMAPPED : RENDER_PATH_BUMPMAP_FALLBACK;
    K2_UNUSED(fAlpha);
#else
    if (fAlpha < 1.0f)
        return RENDER_PATH_FLAT_VP;
#endif

#if defined(K2_QOL_BUMPMAPS) && KOTOR1
    // The Mac and PC's one-pass bump path for a mesh without a lightmap, which the APK took out.
    if (bBump && bTangent && !(nLightmap && enablelightmap) && enablebumpmap &&
        AurNV2xLevelShadersAvailable() && AurBumpMapCardValidate() && AurNonRadeon8500Validate())
        return RENDER_PATH_BUMPMAPPED;
    // The PC's lightmapped bump paths (swkotor.exe GetRenderPath), the material's renderbmlmtype
    // picking the specular one or the diffuse one. A bumpy-shiny mesh takes the PC's EMBM path
    // instead, which is not here.
    if (bBump && bTangent && nLightmap && enablelightmap && enablebumpmap &&
        !bBumpyShiny &&
        AurNV2xLevelShadersAvailable() && AurBumpMapCardValidate() && AurNonRadeon8500Validate())
        return m_pMaterial->m_nRenderBMLMType != 0 ? RENDER_PATH_LIGHTMAPPED_BUMP_SPECULAR
                                                   : RENDER_PATH_LIGHTMAPPED_BUMP_DIFFUSE;
#endif

    if (bNormal && !bTangent && enablebumpmap && AurNV2xLevelShadersAvailable())
        return RENDER_PATH_NORMAL_MAPPED;

    if (nLightmap != 0) {
        int nPath = (bEnv && enablelightmap) ? RENDER_PATH_EMLM_VP : RENDER_PATH_ENVIRONMENT_MAPPED_VP;
        if (bEnv)
            return nPath;
    } else if (bEnv) {
        return RENDER_PATH_ENVIRONMENT_MAPPED_VP;
    }
    if (nLightmap && enablelightmap)
        return RENDER_PATH_LIGHTMAPPED_VP;
    if (!bBump)
        return RENDER_PATH_FLAT_VP;
    return (bTangent && enablebumpmap) ? RENDER_PATH_BUMPMAP_FALLBACK : RENDER_PATH_FLAT_VP;
}

extern int countbinds;
extern int countgobs;
extern int countfaces;
extern int countparts;
extern int countshadowfaces;
extern int counttrans;
extern int countcullpart;
extern int countdisplaycreates;
extern int countbackshadows;
class Camera;
extern Camera* CurrentCamera;

#include "aurprimitive.h"
#include "aurvertexprog.h"
#include "aurtexman.h"
#include "aurmodelrender.h"
#include "aurcamera.h"
#include "aurlightmanager.h"
#include "aurtexcontrol.h"



struct RenderVertexC;


extern int renderaabb;
extern int countfaces;
extern int enabletransform;


static const Vector walkMeshColors[32] = {
    {0.8f, 0.8f, 0.8f},
    {0.580392f, 0.486275f, 0.313726f},
    {0.8f, 0.8f, 0.0f},
    {0.243137f, 0.588235f, 0.12549f},
    {0.4f, 0.4f, 0.4f},
    {0.596078f, 0.34902f, 0.184314f},
    {0.0f, 0.745098f, 0.694118f},
    {0.8f, 0.0f, 0.0f},
    {0.996078f, 0.8f, 0.745098f},
    {0.509804f, 0.0f, 0.501961f},
    {0.623529f, 0.72549f, 0.72549f},
    {0.501961f, 0.996078f, 0.8f},
    {0.537255f, 0.615686f, 0.0f},
    {0.286275f, 0.113725f, 0.0f},
    {0.278431f, 0.356863f, 0.137255f},
    {0.8f, 0.388235f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0941177f, 0.8f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
};

// A walkmesh colour is packed 0xAABBGGRR with an opaque alpha byte.
enum : unsigned int
{
    kWalkMeshAlphaOpaque = 0xff000000,
};

// The walkmesh's front-face colours, built on first use.
static unsigned int* GetWalkMeshColors()
{
    static unsigned int* colorarray = new unsigned int[32];
    for (int i = 0; i < 32; i++) {
        colorarray[i] = kWalkMeshAlphaOpaque
            | (static_cast<int>(walkMeshColors[i].z * 255.0f) << 16)
            | (static_cast<int>(walkMeshColors[i].y * 255.0f) << 8)
            | (static_cast<int>(walkMeshColors[i].x * 255.0f));
    }
    return colorarray;
}

// The back-face tint, with each component inverted before scaling.
static unsigned int* GetInverseWalkMeshColors()
{
    static unsigned int* colorarray = new unsigned int[32];
    for (int i = 0; i < 32; i++) {
        colorarray[i] = kWalkMeshAlphaOpaque
            | (static_cast<int>((1.0f - walkMeshColors[i].z) * 255.0f) << 16)
            | (static_cast<int>((1.0f - walkMeshColors[i].y) * 255.0f) << 8)
            | (static_cast<int>((1.0f - walkMeshColors[i].x) * 255.0f));
    }
    return colorarray;
}

void RenderFlat(VertexPrimitiveFlat* vertices);
void RenderLightMapped(VertexPrimitiveFlat* vertices);
void RenderEnvironmentMapped(VertexPrimitiveFlat* vertices, bool flag, bool ati);
void RenderEMLM(VertexPrimitiveFlat* vertices);
void RenderEMBM(VertexPrimitiveFlat* vertices);
void RenderLightMappedBumpSpecular(VertexPrimitiveFlat* vertices);
void RenderLightMappedBumpDiffuse(VertexPrimitiveFlat* vertices);
void RenderBumpMapped(VertexPrimitiveFlat* vertices);
void RenderNV1xEMLM(VertexPrimitiveFlat* vertices);
void RenderLightMappedNoShaders(VertexPrimitiveFlat* vertices);
void RenderNormalMapped(VertexPrimitiveFlat* vertices);
void RenderNoiseMapped(VertexPrimitiveFlat* vertices, int nDirtTexture, int nDirtWorldspace);
void RenderHologramEffect(int nRenderPath, VertexPrimitiveFlat* vertices);
int AurBumpyShinyAvailable();
bool AurNV1xLevelShadersAvailable();

void SetupForceSightShader(Gob* gob, int a, int b, CAurTexture* c1, CAurTexture* c2,
                           CAurTexture* c3, CAurTexture* c4, bool flag);

extern unsigned int g_SceneRender_CurrentTime_ms;

#if !KOTOR1  // K2 only
void SetupForceSightShader(Gob* gob, int a, int b, CAurTexture* c1, CAurTexture* c2,
                           CAurTexture* c3, CAurTexture* c4, bool flag)
{
// With no owning gob, reset the tint to opaque white.
    if (gob == 0)
    {
        GLRender::m_ForceSightColor = GLRender::PerInstanceColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        GLRender::pfuncEnablePixelShaders(b, c1, c2, c3, c4);
        return;
    }

    bool bForceGlow = gob->IsForceGlowEnabled();
    bool bNonForceBlend = gob->IsNonForceBlending();
    gob->GetForcePoints();
    bool bIllumination = gob->IsIlluminationEnable();

// Force glow is a pulsing tint scaled into the glow colour's alpha.
    if (bForceGlow)
    {
        GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendInvSrcAlpha);
        float fPulse = sinf(static_cast<float>(static_cast<int>(static_cast<unsigned int>(static_cast<float>(g_SceneRender_CurrentTime_ms) * 0.125f)
                                        & 0xff)) * 0.024639944f);
        fPulse = fPulse * 0.25f + 0.75f;
        float fColorA, fColorB, fColorC, fColorD;
        gob->GetForceGlowColor(fColorA, fColorB, fColorC, fColorD);
        GLRender::m_ForceSightColor = GLRender::PerInstanceColor{
            fColorA, fColorB, fColorC, fPulse * fColorD };
        GLRender::pfuncEnablePixelShaders(a, c1, c2, c3, c4);
        return;
    }

// Restore the normal alpha pair, then dim the tint to 3/4 or leave it opaque.
    if (bNonForceBlend && !flag)
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

    static const GLRender::PerInstanceColor kDimColor  = { 0.0f, 0.0f, 0.0f, 0.75f };
    static const GLRender::PerInstanceColor kFullColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (bNonForceBlend)
        GLRender::m_ForceSightColor = kDimColor;
    else
        GLRender::m_ForceSightColor = kFullColor;

    if (!bIllumination)
    {
        GLRender::pfuncEnablePixelShaders(b, c1, c2, c3, c4);
        return;
    }

    int nMode = b + (GLRender::kPSIllumDesat - GLRender::kPSDesat);
    if (nMode > GLRender::kPSEnvMap)
        nMode = GLRender::kPSIllumDesat;
    GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
    gob->GetIlluminationColor(color.r, color.g, color.b);
    GLRender::m_PerInstanceIllumination = color;
    GLRender::pfuncEnablePixelShaders(nMode, c1, c2, c3, c4);
}
#endif

extern char cStandardVP[];
extern char szStandardVPARB[];
extern char szStandardCubeVPARB[];
extern char szStandardLightmapCubeVPARB[];
extern unsigned int* cStandardVPDeclaration;
extern int usearbvertexprograms;
extern unsigned char g_ForceSight;
extern unsigned char g_useDistortionOutput;

void RenderFlatVP(VertexPrimitive* vertices, PartTriMesh* part)
{
#if KOTOR1  // K1: no texture-2D cap, decal offset, force sight or illumination
    static VertexProgram* standardvp =
        new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                          cStandardVPDeclaration);

    standardvp->Enable(part, true);
    CAurTexture* pTexture0 = part->m_pMaterial->Textures(0).m_pObject;
    GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, pTexture0, 0, 0, 0);
    RenderFlat(vertices->AsVertexPrimitiveFlat());
#else
    GLRender::Enable(Aurora::kCapVertexProgram);

    static VertexProgram* standardvp =
        new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                          cStandardVPDeclaration);

    standardvp->Enable(part, true);

    bool bOffset = false;
    if (part != 0 && part->m_nNodeID == 219)
    {
        CAurTexture* pTexture = part->m_pMaterial->Textures(0).m_pObject;
        if (pTexture == 0)
            __builtin_trap();
        if (pTexture->GetName() != 0 && strstr(pTexture->GetName(), "MG_Ondr3") != 0)
        {
            ASLgl::glPolygonOffset(14.0f, 14.0f);
            ASLgl::glEnable(GL_POLYGON_OFFSET_FILL);
            bOffset = true;
        }
    }

    K2_ASSUME(part != 0);   // the calling trimesh itself (PartTriMesh::Render, PartSkin::Render)
    CAurTexture* pTexture0 = part->m_pMaterial->Textures(0).m_pObject;
    GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, pTexture0, 0, 0, 0);

    Gob* pGob = vertices->m_pPart->m_pGob;
    if (g_ForceSight)
    {
        SetupForceSightShader(pGob, GLRender::kPSFGlow, GLRender::kPSDesat, 0, 0, 0, 0, false);
        RenderFlat(vertices->AsVertexPrimitiveFlat());
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        RenderInterface::SetDepthMask(1);
    }
    else if (pGob == 0)
    {
        RenderFlat(vertices->AsVertexPrimitiveFlat());
    }
    else
    {
        if (pGob->IsIlluminationEnable() && AurNV1xLevelShadersAvailable())
        {
            GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
            pGob->GetIlluminationColor(color.r, color.g, color.b);
            GLRender::m_PerInstanceIllumination = color;
            GLRender::pfuncEnablePixelShaders(GLRender::kPSIllum, 0, 0, 0, 0);
        }

        RenderFlat(vertices->AsVertexPrimitiveFlat());

        if (pGob->IsIlluminationEnable())
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    }

    GLRender::Disable(Aurora::kCapVertexProgram);

    if (bOffset)
    {
        ASLgl::glDisable(GL_POLYGON_OFFSET_FILL);
        ASLgl::glPolygonOffset(0.0f, 0.0f);
    }
#endif
}

#if KOTOR1
extern int enablerenderlightmapped;
int AurMultiTextureAvailable();
#endif
void RenderLightmappedVP(VertexPrimitive* vertices, PartTriMesh* part)
{
#if KOTOR1  // K1: gated on enablerenderlightmapped; the two-stage draw is inline
    if (!enablerenderlightmapped)
        return;

    GLRender::Enable(Aurora::kCapVertexProgram);

    static VertexProgram* standardvp =
        new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                          cStandardVPDeclaration);

    standardvp->Enable(part, true);

    VertexPrimitiveFlat* pFlat = vertices->AsVertexPrimitiveFlat();
    PartTriMesh* pMesh = pFlat->m_pPart->AsPartTriMesh();
    GLRender::ActivateTextureStage(1);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);

    CAurTexture* pTex0 = pMesh->m_pMaterial->Textures(0).m_pObject;
    CAurTexture* pTex1 = pMesh->m_pMaterial->Textures(1).m_pObject;
    GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmap, pTex0, pTex1, 0, 0);
    RenderFlat(pFlat);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSMultiTextureOff, 0, 0, 0, 0);

    GLRender::ActivateTextureStage(1);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);
    GLRender::Disable(Aurora::kCapVertexProgram);
#else
    GLRender::Enable(Aurora::kCapVertexProgram);

    static VertexProgram* standardvp =
        new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                          cStandardVPDeclaration);

    standardvp->Enable(part, true);
    RenderLightMapped(vertices->AsVertexPrimitiveFlat());
    GLRender::Disable(Aurora::kCapVertexProgram);
#endif
}

void RenderEnvironmentMappedVP(VertexPrimitive* vertices, PartTriMesh* part)
{
    GLRender::Enable(Aurora::kCapVertexProgram);

    static VertexProgram* standardcubevp =
        new VertexProgram(usearbvertexprograms != 0 ? szStandardCubeVPARB : cStandardVP,
                          cStandardVPDeclaration);

    standardcubevp->Enable(part, true);

    GLRender::SetVertexShaderConstant(15, 0.0f, 0.0f, 1.0f, 0.0f);
    GLRender::SetVertexShaderConstant(16, 0.0f, 2.0f, 3.0f, 1.0f);

    RenderEnvironmentMapped(vertices->AsVertexPrimitiveFlat(), false, false);
    GLRender::Disable(Aurora::kCapVertexProgram);
}

void RenderEMLMVP(VertexPrimitive* vertices, PartTriMesh* part)
{
    GLRender::Enable(Aurora::kCapVertexProgram);

    static VertexProgram* standardlightmapcubevp =
        new VertexProgram(usearbvertexprograms != 0 ? szStandardLightmapCubeVPARB : cStandardVP,
                          cStandardVPDeclaration);

    standardlightmapcubevp->Enable(part, true);

    GLRender::SetVertexShaderConstant(15, 0.0f, 0.0f, 1.0f, 0.0f);
    GLRender::SetVertexShaderConstant(16, 0.0f, 2.0f, 3.0f, 1.0f);

    RenderEMLM(vertices->AsVertexPrimitiveFlat());
    GLRender::Disable(Aurora::kCapVertexProgram);
}

void RenderBumpmapFallback(VertexPrimitive* vertices)
{
#if !KOTOR1  // K1: no force-sight arm
    if (g_ForceSight) {
        SetupForceSightShader(vertices->m_pPart->AsPartTriMesh()->m_pGob, GLRender::kPSFGlow, GLRender::kPSDesat,
                              0, 0, 0, 0, false);

        RenderFlat(vertices->AsVertexPrimitiveFlat());
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

        RenderInterface::SetDepthMask(1);
    } else
#endif
    {
        GLRender::Disable(Aurora::kCapAlphaTest);
        GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);

        RenderFlat(vertices->AsVertexPrimitiveFlat());

        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        GLRender::Enable(Aurora::kCapAlphaTest);
    }
}

// The opacity, the two cull switches and an empty mesh.
inline bool IsPartRenderable(PartTriMesh* pPart)
{
    if (pPart->m_fOpacity <= 0.001f)
        return false;
    MdlNodeTriMesh* pNode = static_cast<MdlNodeTriMesh*>(pPart->m_pNode);
    if (cullnonrenderable != 0 && pNode->m_bRender == 0)
        return false;
    if (culltransparent != 0 && pNode->m_nTransparencyHint != 0)
        return false;
    return pNode->m_nVertexCount != 0;
}

void PartTriMesh::Draw(bool)
{
#if KOTOR1  // K1: path cached, no hologram, force sight, illumination or noise
    if (rendertrimesh == 0 && AsPartDanglyMesh() == 0 && m_pGob != 0)
        return;

    if (!IsPartRenderable(this))
        return;

    MdlNodeTriMesh* pMesh = m_pNode->AsMdlNodeTriMesh();

    m_pMaterial->HasBumpMap();
    m_pMaterial->HasBumpyShinyMap();
    pMesh->HasTangentBases();

    TransformModelviewMatrix();
    TransformTextureMatrix();
    m_pVertexPrimitive->SetupVertexArraysStandard();

    if (m_nRenderPath == -1)
        m_nRenderPath = GetRenderPath();

    switch (m_nRenderPath)
    {
    case RENDER_PATH_FLAT:
        RenderFlat(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    case RENDER_PATH_FLAT_VP:
        RenderFlatVP(m_pVertexPrimitive, this);
        break;
    case RENDER_PATH_EMBM:
        RenderEMBM(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    case RENDER_PATH_LIGHTMAPPED_BUMP_SPECULAR:
        RenderLightMappedBumpSpecular(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    case RENDER_PATH_LIGHTMAPPED_BUMP_DIFFUSE:
        RenderLightMappedBumpDiffuse(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    case RENDER_PATH_BUMPMAPPED:
        RenderBumpMapped(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    case RENDER_PATH_BUMPMAP_FALLBACK:
        GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);
        RenderFlatVP(m_pVertexPrimitive, this);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        break;
    case RENDER_PATH_EMLM:
        RenderEMLM(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    case RENDER_PATH_EMLM_VP:
        RenderEMLMVP(m_pVertexPrimitive, this);
        break;
    case RENDER_PATH_NV1X_EMLM:
        RenderNV1xEMLM(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    case RENDER_PATH_ENVIRONMENT_MAPPED:
        RenderEnvironmentMapped(m_pVertexPrimitive->AsVertexPrimitiveFlat(), false, false);
        break;
    case RENDER_PATH_ENVIRONMENT_MAPPED_VP:
        RenderEnvironmentMappedVP(m_pVertexPrimitive, this);
        break;
    case RENDER_PATH_LIGHTMAPPED:
        RenderLightMapped(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    case RENDER_PATH_LIGHTMAPPED_VP:
        RenderLightmappedVP(m_pVertexPrimitive, this);
        break;
    case RENDER_PATH_LIGHTMAPPED_NO_SHADERS:
        RenderLightMappedNoShaders(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    case RENDER_PATH_NORMAL_MAPPED:
        RenderNormalMapped(m_pVertexPrimitive->AsVertexPrimitiveFlat());
        break;
    }

    if (AurBumpyShinyAvailable() != 0 && m_pGob != 0 && m_pGob->m_fBumpOutDistance != 0.0f
        && currentscene != 0)
    {
        VertexPrimitiveFlat* pFlat = m_pVertexPrimitive->AsVertexPrimitiveFlat();
        MdlNodeTriMesh* pDrawMesh = static_cast<MdlNodeTriMesh*>(pFlat->m_pNode);

        GLRender::Enable(Aurora::kCapVertexProgram);
        GLRender::SetVertexShaderConstant(12, m_pGob->m_fBumpOutDistance, 0.0f, 0.0f, 1.0f);
        SetBumpedOutShellIos(m_pGob->m_fBumpOutDistance);
        g_pBumpedOutVP->Enable(this, false);
        m_pMaterial->m_spr3.m_pObject->Bind();
        if (m_pMaterial->m_lstSrcBlend.m_count >= 2)
            m_pMaterial->SetBlendingMode(1);

        if (pDrawMesh->m_lVertexIndicesCount.m_count >= 1) {
            for (int64_t i = 0; i < pDrawMesh->m_lVertexIndicesCount.m_count; i++)
                GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(pDrawMesh->m_nPrimitiveType),
                                       pDrawMesh->m_lVertexIndicesCount.m_pData[i], 0,
                                       pDrawMesh->m_lIndexPools.m_pData[i]);
        }
        if (pDrawMesh->m_nPrimitiveType == 4 && pDrawMesh->m_nLeftOverIndexCount != 0)
            GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(3),
                                   pDrawMesh->m_nLeftOverIndexCount,
                                   pDrawMesh->m_nGeomBase,
                                   pDrawMesh->m_nLeftOverPool);
        SetBumpedOutShellIos(0.0f);
        GLRender::Disable(Aurora::kCapVertexProgram);
    }

    RestoreTextureMatrix();
    RestoreModelviewMatrix();
#else
    if (rendertrimesh == 0 && AsPartDanglyMesh() == 0 && m_pGob != 0)
        return;

    if (!IsPartRenderable(this))
        return;

    MdlNodeTriMesh* pMesh = m_pNode->AsMdlNodeTriMesh();

    m_pMaterial->HasBumpMap();
    m_pMaterial->HasBumpyShinyMap();
    pMesh->HasTangentBases();

    TransformModelviewMatrix();
    TransformTextureMatrix();
    m_pVertexPrimitive->SetupVertexArraysStandard();

    int nPath = GetRenderPath();

    if (m_bHologramRendering && AurNV2xLevelShadersAvailable())
    {
        if (!pMesh->m_bHologramDoNotDraw)
            RenderHologramEffect(nPath, m_pVertexPrimitive->AsVertexPrimitiveFlat());
    }
    else
    {
        switch (nPath)
        {
        case RENDER_PATH_FLAT:
            if (g_ForceSight) {
                SetupForceSightShader(m_pGob, GLRender::kPSFGlow, GLRender::kPSDesat, 0, 0, 0, 0, false);
                RenderFlat(m_pVertexPrimitive->AsVertexPrimitiveFlat());
                GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
                GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
                RenderInterface::SetDepthMask(1);
            } else {
                if (m_pGob != 0 && m_pGob->IsIlluminationEnable()
                    && AurNV1xLevelShadersAvailable()) {
                    GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
                    m_pGob->GetIlluminationColor(color.r, color.g, color.b);
                    GLRender::m_PerInstanceIllumination = color;
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSIllum, 0, 0, 0, 0);
                }
                RenderFlat(m_pVertexPrimitive->AsVertexPrimitiveFlat());
                if (m_pGob != 0 && m_pGob->IsIlluminationEnable())
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
            }
            break;
        case RENDER_PATH_FLAT_VP:
            RenderFlatVP(m_pVertexPrimitive, this);
            break;
        case RENDER_PATH_EMBM:
            RenderEMBM(m_pVertexPrimitive->AsVertexPrimitiveFlat());
            break;
        case RENDER_PATH_LIGHTMAPPED_BUMP_SPECULAR:
            RenderLightMappedBumpSpecular(m_pVertexPrimitive->AsVertexPrimitiveFlat());
            break;
        case RENDER_PATH_LIGHTMAPPED_BUMP_DIFFUSE:
            RenderLightMappedBumpDiffuse(m_pVertexPrimitive->AsVertexPrimitiveFlat());
            break;
        case RENDER_PATH_BUMPMAPPED:
            RenderBumpMapped(m_pVertexPrimitive->AsVertexPrimitiveFlat());
            break;
        case RENDER_PATH_BUMPMAP_FALLBACK:
            RenderBumpmapFallback(m_pVertexPrimitive);
            break;
        case RENDER_PATH_EMLM:
            RenderEMLM(m_pVertexPrimitive->AsVertexPrimitiveFlat());
            break;
        case RENDER_PATH_EMLM_VP:
            RenderEMLMVP(m_pVertexPrimitive, this);
            break;
        case RENDER_PATH_NV1X_EMLM:
            RenderNV1xEMLM(m_pVertexPrimitive->AsVertexPrimitiveFlat());
            break;
        case RENDER_PATH_ENVIRONMENT_MAPPED:
            RenderEnvironmentMapped(m_pVertexPrimitive->AsVertexPrimitiveFlat(), false, false);
            break;
        case RENDER_PATH_ENVIRONMENT_MAPPED_VP:
            RenderEnvironmentMappedVP(m_pVertexPrimitive, this);
            break;
        case RENDER_PATH_LIGHTMAPPED:
            RenderLightMapped(m_pVertexPrimitive->AsVertexPrimitiveFlat());
            break;
        case RENDER_PATH_LIGHTMAPPED_VP:
            RenderLightmappedVP(m_pVertexPrimitive, this);
            break;
        case RENDER_PATH_LIGHTMAPPED_NO_SHADERS:
            RenderLightMappedNoShaders(m_pVertexPrimitive->AsVertexPrimitiveFlat());
            break;
        case RENDER_PATH_NORMAL_MAPPED:
            RenderNormalMapped(m_pVertexPrimitive->AsVertexPrimitiveFlat());
            break;
        case RENDER_PATH_NOISE_MAPPED:
            RenderNoiseMapped(m_pVertexPrimitive->AsVertexPrimitiveFlat(),
                              static_cast<short>(pMesh->m_nDirtTexture) - 1,
                              static_cast<short>(pMesh->m_nDirtWorldspace));
            break;
        }
    }

    if (AurBumpyShinyAvailable() != 0 && m_pGob != 0 && m_pGob->m_fBumpOutDistance != 0.0f
        && currentscene != 0)
    {
        countparts++;

        VertexPrimitiveFlat* pFlat = m_pVertexPrimitive->AsVertexPrimitiveFlat();
        MdlNodeTriMesh* pDrawMesh = static_cast<MdlNodeTriMesh*>(pFlat->m_pNode);

        GLRender::Enable(Aurora::kCapVertexProgram);
        GLRender::SetVertexShaderConstant(12, m_pGob->m_fBumpOutDistance, 0.0f, 0.0f, 1.0f);
        g_pBumpedOutVP->Enable(this, false);
        m_pMaterial->m_spr3.m_pObject->Bind();
        if (m_pMaterial->m_lstSrcBlend.m_count >= 2)
            m_pMaterial->SetBlendingMode(1);
        if (g_ForceSight)
            SetupForceSightShader(m_pGob, GLRender::kPSFGlow, GLRender::kPSDesat, 0, 0, 0, 0, false);

        if (pDrawMesh->m_lVertexIndicesCount.m_count >= 1) {
            for (int64_t i = 0; i < pDrawMesh->m_lVertexIndicesCount.m_count; i++) {
                int nCount = pDrawMesh->m_lVertexIndicesCount.m_pData[i];
                GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(pDrawMesh->m_nPrimitiveType),
                                       nCount, 0,
                                       pDrawMesh->m_lIndexPools.m_pData[i]);
                countfaces += nCount / 3;
            }
        }
        if (pDrawMesh->m_nPrimitiveType == 4 && pDrawMesh->m_nLeftOverIndexCount != 0)
            GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(3),
                                   pDrawMesh->m_nLeftOverIndexCount,
                                   pDrawMesh->m_nGeomBase,
                                   pDrawMesh->m_nLeftOverPool);

        if (g_ForceSight) {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
            GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
            RenderInterface::SetDepthMask(1);
        }
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        RenderInterface::SetDepthMask(1);
        GLRender::Disable(Aurora::kCapVertexProgram);
    }

    RestoreTextureMatrix();
    RestoreModelviewMatrix();
    ASLgl::glTexCoordPointer(2, GL_FLOAT, 0, 0);
#endif
}


float shadowdepth = 0.100000001f;
float beamdepth = 20.0f;
int shadowvertcount;
int shadowstripcount;
extern int quaternionidentityoptimization;
Vector VectorMin(Vector a, Vector b);
Vector VectorMax(Vector a, Vector b);

struct CAurShadowData {
    int     m_nCount = 0;
    bool    m_bFlag = true;
    Vector* m_pVertices = 0;
    int     m_nVertexCount = 0;
    int     m_nPool = -1;

    ~CAurShadowData() { delete[] m_pVertices; }
};

static inline Vector RotateVector(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, wx = x * w;
    float yz = y * z, wy = y * w;
    float wz = z * w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        Vector r;
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        r.x = v.x * m00 + v.y * m01 + v.z * m02;
        r.y = v.x * m10 + v.y * m11 + v.z * m12;
        r.z = v.x * m20 + v.y * m21 + v.z * m22;
        return r;
    }
    return v;
}

#if !KOTOR1
static inline void BuildShadowFrame(Vector& vLight, Vector& vFoot, Vector& vNormal, float fDepth,
                                    const Vector& vPos, const Quaternion& q)
{
    float w = q.w;
    float nw = -w;
    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = nw * x, wy = nw * y, wz = nw * z;

#define SHADOWFRAME_ROTATE(v)                                                          \
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {            \
        Vector r;                                                                      \
        float m00 = 1.0f - ((yy + zz) + (yy + zz));                                    \
        float m01 = (xy - wz) + (xy - wz);                                             \
        float m02 = (xz + wy) + (xz + wy);                                             \
        float m10 = (xy + wz) + (xy + wz);                                             \
        float m11 = 1.0f - ((xx + zz) + (xx + zz));                                    \
        float m12 = (yz - wx) + (yz - wx);                                             \
        float m20 = (xz - wy) + (xz - wy);                                             \
        float m21 = (yz + wx) + (yz + wx);                                             \
        float m22 = 1.0f - ((xx + yy) + (xx + yy));                                    \
        r.x = (v).x * m00 + (v).y * m01 + (v).z * m02;                                 \
        r.y = (v).x * m10 + (v).y * m11 + (v).z * m12;                                 \
        r.z = (v).x * m20 + (v).y * m21 + (v).z * m22;                                 \
        (v) = r;                                                                       \
    }

    SHADOWFRAME_ROTATE(vLight);
    vFoot = Vector(0.0f, 0.0f, -fDepth) - vPos;
    SHADOWFRAME_ROTATE(vFoot);
    SHADOWFRAME_ROTATE(vNormal);
#undef SHADOWFRAME_ROTATE
}
#endif

void PartProjection::Init()
{
    m_vBBoxMin = Vector(1000000.0f, 1000000.0f, 1000000.0f);
    m_vBBoxMax = Vector(-1000000.0f, -1000000.0f, -1000000.0f);

    float fOffset = 0.0f;
    TileData* pTile = m_pTriMesh->m_pTileData;
    if (pTile != 0)
        fOffset = pTile->m_vPosition.z;

// A PartBeam's shadow is the full beam depth; everything else takes the plane depth.
    float fDepth = AsPartBeam() == 0 ? shadowdepth - fOffset : beamdepth;

// The light's world position and the plane's foot, in the caster's local frame.
#if KOTOR1  // K1: three forward rotates through the out-of-line inverse
    Quaternion qInv = -m_pTriMesh->m_qLocalOrientation;
    Vector vLight = RotateVector(m_pLight->m_vWorldPosition - m_pTriMesh->m_vLocalPosition, qInv);
    qInv = -m_pTriMesh->m_qLocalOrientation;
    Vector vFoot = RotateVector(Vector(0.0f, 0.0f, -fDepth) - m_pTriMesh->m_vLocalPosition, qInv);
    qInv = -m_pTriMesh->m_qLocalOrientation;
    Vector vNormal = RotateVector(Vector(0.0f, 0.0f, -1.0f), qInv);
#else
    Vector vLight = m_pLight->m_vWorldPosition - m_pTriMesh->m_vLocalPosition;
    Vector vFoot;
    Vector vNormal(0.0f, 0.0f, -1.0f);
    BuildShadowFrame(vLight, vFoot, vNormal, fDepth, m_pTriMesh->m_vLocalPosition,
                     m_pTriMesh->m_qLocalOrientation);
#endif
    Plane plane(vNormal.x, vNormal.y, vNormal.z,
                -(vFoot.x * vNormal.x + vFoot.y * vNormal.y + vFoot.z * vNormal.z));

    ASLgl::glPushMatrix();
    if (enabletransform)
        GLRender::Transform(m_pTriMesh->m_vLocalPosition, m_pTriMesh->m_qLocalOrientation, 0);

    CAurShadowData data;
    unsigned int nPoolSize;
    m_pTriMesh->Project(vLight, m_pLight->m_fLightRadius, plane, &data);

    ASLgl::glPopMatrix();

    shadowvertcount += data.m_nVertexCount * 2;
    shadowstripcount += data.m_nVertexCount / 3;

    if (data.m_nPool == -1) {
        m_nIndexPool = -1;
    } else {
        {
            unsigned int nSize = data.m_nCount * 12;
            GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(1), nSize, 2, 0,
                                  reinterpret_cast<unsigned int*>(&m_nIndexPool));
            Vector* pDst = static_cast<Vector*>(GLRender::LockPool(m_nIndexPool, 0, 0, 1));
            void* pSrc = GLRender::LockPool(data.m_nPool, 0, 0, 8);
            memcpy(pDst, pSrc, nSize);  // idiom-exempt: nSize is the pool size requested above (12 per Vector)
            GLRender::UnlockPool(m_nIndexPool);
            GLRender::UnlockPool(data.m_nPool);
        }

        if (data.m_nVertexCount > 0) {
            GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0), data.m_nVertexCount * 12,
                                  0x42, 0, reinterpret_cast<unsigned int*>(&m_nVertexPool));
            nPoolSize = GLRender::GetPoolSize(m_nIndexPool);
            m_nVertexCount = data.m_nVertexCount;
            m_nQuadCount = nPoolSize / 12;
            if (nPoolSize <= 11)
                m_nQuadCount = -1;

            Vector* pPool = static_cast<Vector*>(GLRender::LockPool(m_nVertexPool, 0, 0, 1));
            int i;
            for (i = 0; i < data.m_nVertexCount; i++) {
                Vector v = RotateVector(data.m_pVertices[i], m_pTriMesh->m_qLocalOrientation) +
                           m_pTriMesh->m_vLocalPosition;
                m_vBBoxMin = VectorMin(m_vBBoxMin, v);
                m_vBBoxMax = VectorMax(m_vBBoxMax, v);
                pPool[i] = v;
            }
            GLRender::UnlockPool(m_nVertexPool);
        }
    }
}

inline void PartProjection::SetupVertexArrays()
{
    if (m_nVertexCount != 0)
        GLRender::SetVertexBuffer(12, m_nVertexPool, 0, m_nVertexCount);
}

static float ShadowPlaneCornerDist(float x, float y, float z, const Plane& plane)
{
    return plane.d + (plane.a * x + plane.b * y + plane.c * z);
}

// Draws the shadow volume in two stencil passes, one per face winding.
void PartShadow::Draw(bool)
{
    if (m_nQuadCount == -1)
        return;

    if (m_nQuadCount == 0)
    {
        Init();
        if (m_nQuadCount == 0)
            return;
    }

    bool bVisible = false;
    if (CurrentCamera != 0)
    {
        const Plane& plane = CurrentCamera->m_viewState.m_aViewVolumes[1];
        if (ShadowPlaneCornerDist(m_vBBoxMax.x, m_vBBoxMax.y, m_vBBoxMax.z, plane) > 0.0f ||
            ShadowPlaneCornerDist(m_vBBoxMin.x, m_vBBoxMax.y, m_vBBoxMax.z, plane) > 0.0f ||
            ShadowPlaneCornerDist(m_vBBoxMax.x, m_vBBoxMin.y, m_vBBoxMax.z, plane) > 0.0f ||
            ShadowPlaneCornerDist(m_vBBoxMin.x, m_vBBoxMin.y, m_vBBoxMax.z, plane) > 0.0f ||
            ShadowPlaneCornerDist(m_vBBoxMax.x, m_vBBoxMax.y, m_vBBoxMin.z, plane) > 0.0f ||
            ShadowPlaneCornerDist(m_vBBoxMin.x, m_vBBoxMax.y, m_vBBoxMin.z, plane) > 0.0f ||
            ShadowPlaneCornerDist(m_vBBoxMax.x, m_vBBoxMin.y, m_vBBoxMin.z, plane) > 0.0f ||
            ShadowPlaneCornerDist(m_vBBoxMin.x, m_vBBoxMin.y, m_vBBoxMin.z, plane) > 0.0f)
            bVisible = true;
    }

    SetupVertexArrays();

    if (m_nIndexPool == -1)
        return;

    if (bVisible)
    {
        GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilKeep, Aurora::kStencilIncr);
        GLRender::SetFrontFace(Aurora::kCullCCW);
        GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(4), m_nQuadCount * 6, 0, m_nIndexPool);

        GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilKeep, Aurora::kStencilDecr);
        GLRender::SetFrontFace(Aurora::kCullCW);
        GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(4), m_nQuadCount * 6, 0, m_nIndexPool);

        GLRender::SetFrontFace(Aurora::kCullCCW);
        countbackshadows++;
    }
    else
    {
        GLRender::SetFrontFace(Aurora::kCullCW);
        GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilIncr, Aurora::kStencilKeep);
        GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(4), m_nQuadCount * 6, 0, m_nIndexPool);

        GLRender::SetFrontFace(Aurora::kCullCCW);
        GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilDecr, Aurora::kStencilKeep);
        GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(4), m_nQuadCount * 6, 0, m_nIndexPool);
    }

#if !KOTOR1  // K1: no draw counters
    countfaces += m_nQuadCount * 8;
#endif
}

void PartBeam::Draw(bool)
{
    if (m_nQuadCount == 0)
        Init();

    SetupVertexArrays();

    if (m_nQuadCount != 0)
        GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(4), m_nQuadCount * 6, 0, m_nIndexPool);

#if !KOTOR1
    countfaces += m_nQuadCount * 4;
#endif
}

void SetBeaming()
{
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendOne);
    ASLgl::glColorMask(1, 1, 1, 1);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glColor4fv(Homogenize(projectioncolor, projectionalpha));
    if (stenciloff)
        ASLgl::glStencilMask(0);
}

// Applies the part's world transform unless the backend owns the modelview itself.
void Transform(const Vector& v, const Quaternion& q, int n)
{
    if (enabletransform)
        GLRender::Transform(v, q, n);
}

int enableplaceholders;
void Part::Draw(bool)
{
    if (enableplaceholders)
    {
        Vector vColor = { 1.0f, 0.0f, 0.0f };
        GLRender::DrawSphereBound(0.1f, m_vLocalPosition, vColor, 1.0f, true);
    }
}

void PartTriMesh::TransformTextureMatrix()
{
    float fTextureAngle = m_fTextureAngle;
    if (fTextureAngle == 0.0f)
        return;
    ASLgl::glMatrixMode(GL_TEXTURE);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glRotatef(fTextureAngle, 0.0f, 0.0f, 1.0f);
    ASLgl::glMatrixMode(GL_MODELVIEW);
}

void PartTriMesh::RestoreTextureMatrix()
{
    if (m_fTextureAngle == 0.0f)
        return;
    ASLgl::glMatrixMode(GL_TEXTURE);
    ASLgl::glPopMatrix();
    ASLgl::glMatrixMode(GL_MODELVIEW);
}

void PartTriMesh::TransformModelviewMatrix()
{
    if (m_pGob != 0)
        return;
    ASLgl::glPushMatrix();
    const Vector& v = m_vLocalPosition;
    const Quaternion& q = m_qLocalOrientation;
    if (enabletransform)
        GLRender::Transform(v, q, 0);
}

int PartTriMesh::WithinRange(Vector v, float f)
{
    return magnitude(m_vLocalPosition - v) < f + 2.0f;
}

void PartTriMesh::RestoreModelviewMatrix()
{
    if (m_pGob == 0)
        ASLgl::glPopMatrix();
}

void PartTriMesh::RotateTexture(float f)
{
    m_fTextureAngle = f;
}

void countreset()
{
    countbinds = 0;
    countgobs = 0;
    countfaces = 0;
    countparts = 0;
    countshadowfaces = 0;
    counttrans = 0;
    countcullpart = 0;
    countdisplaycreates = 0;
}

void DrawFrame(const Vector& vOrigin, const Vector& vX, const Vector& vZ)
{
    GLRender::DrawAxisAlignedFrame(vOrigin, vX, vZ);
}

void PartAnimMesh::Draw(bool)
{
    if (m_pGob == 0) {
        ASLgl::glPushMatrix();
        const Vector& v = m_vLocalPosition;
        const Quaternion& q = m_qLocalOrientation;
        if (enabletransform != 0)
            GLRender::Transform(v, q, 0);
    }
    m_pVertexPrimitive->SetupVertexArraysStandard();
    RenderFlat(m_pVertexPrimitive->AsVertexPrimitiveFlat());
    if (m_pGob == 0)
        ASLgl::glPopMatrix();
}

// Draws the walkmesh triangles with the face colours, filled and then wireframe.
void PartAABB::Draw(bool)
{
    if (!renderaabb)
        return;

    static RenderVertexC* walkMeshColorToken = reinterpret_cast<RenderVertexC*>(GetWalkMeshColors());
    static RenderVertexC* inverseWalkMeshColorToken = reinterpret_cast<RenderVertexC*>(GetInverseWalkMeshColors());

    MdlNodeAABB* node = reinterpret_cast<MdlNodeAABB*>(m_pNode);
    GLRender::Disable(Aurora::kCapTexture2D);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    if (!node)
        return;

    if (m_pGob == 0) {
        ASLgl::glPushMatrix();
        if (enabletransform)
            GLRender::Transform(m_vLocalPosition, m_qLocalOrientation, 0);
    }

#if !KOTOR1
    countfaces += node->Faces().m_count;
#endif

    GLRender::DrawTriangles(node->Faces(), node->Vertices(), walkMeshColorToken);
    GLRender::SetFrontFace(Aurora::kCullCW);
    GLRender::DrawTriangles(node->Faces(), node->Vertices(), inverseWalkMeshColorToken);
    GLRender::SetFrontFace(Aurora::kCullCCW);
    GLRender::SetPolygonRenderMode(Aurora::kFacingFront, Aurora::kFillLine);
    GLRender::DrawTriangles(node->Faces(), node->Vertices(), inverseWalkMeshColorToken);
    GLRender::SetFrontFace(Aurora::kCullCW);
    GLRender::DrawTriangles(node->Faces(), node->Vertices(), walkMeshColorToken);
    GLRender::SetFrontFace(Aurora::kCullCCW);
    GLRender::SetPolygonRenderMode(Aurora::kFacingFront, Aurora::kFillSolid);

    if (m_pGob == 0)
        ASLgl::glPopMatrix();

    GLRender::Enable(Aurora::kCapTexture2D);
    GLRender::Enable(Aurora::kCapLighting);
}

void PartLightsaber::Draw(bool)
{
    if (lightsaberrender == 0)
        return;

    MdlNodeLightsaber* node = m_pNode->AsMdlNodeLightsaber();

    Vector vWorldPos;
    Quaternion qWorld;
    GetWorld(vWorldPos, qWorld);

    GLRender::Disable(Aurora::kCapCullFace);
    if (enablelightsaberbluradd != 0) {
        GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendOne);
        RenderInterface::SetDepthMask(0);
        ASLgl::glStencilMask(0);
    }
    if (lightsaberlighting == 0)
        GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    GLRender::SetPolygonRenderMode(Aurora::kFacingFrontAndBack,
                                   lightsaberwireframe != 0 ? Aurora::kFillLine : Aurora::kFillSolid);

    Vector3* paPositions = node->m_paPositions;
    Vector2* aTexCoords = node->m_aTexCoords;
    Vector3* aNormals = node->m_aNormals;
    int nHalf = MdlNodeLightsaber::NUM_PIECE_VERTS / 2;

    if (renderpieces == 0 || m_nSegmentCount > 1) {
        lightsabertotalsegments = 0;

        Vector avPiece[4];
        avPiece[0].x = paPositions[nHalf].x - paPositions[0].x;
        avPiece[0].y = paPositions[nHalf].y - paPositions[0].y;
        avPiece[0].z = paPositions[nHalf].z - paPositions[0].z;
        avPiece[1] = avPiece[0];
        avPiece[2] = avPiece[0];
        avPiece[3] = avPiece[0];
        Vector vPiece = avPiece[0];
        float fFudge = magnitude(avPiece[0]);

        short nPrev = m_nOrientIndex - 1;
        if (nPrev < 0)
            nPrev += MdlNodeLightsaber::NUM_BACK_SEGMENTS;
        short nCur = nPrev - 1;
        if (nCur < 0)
            nCur += MdlNodeLightsaber::NUM_BACK_SEGMENTS;
        Vector vSeg = m_avSegmentPos[nPrev];
        Quaternion qSeg = m_aqSegmentOrient[nPrev];

        GLRender::BeginImmediatePrimitive(Aurora::kPrimQuadStrip);

        if (m_nSegmentCount > 1) {
            float fAge = 0.0f;

            for (int n = 1; n < m_nSegmentCount; n++) {
                float fRatio = lightsabersegmentinterval
                               / (m_afSegmentLife[nCur] - m_afSegmentLife[nPrev]);

                if (fRatio < 1.0f) {
                    fAge += lightsabersegmentinterval;
                    if (!(fAge > lightsabersegmentlife)) {
                        do {
                            Vector vNew = Interpolate(m_avSegmentPos[nPrev], m_avSegmentPos[nCur], fRatio);
                            Quaternion qNew = slerp(m_aqSegmentOrient[nPrev], m_aqSegmentOrient[nCur],
                                                    fRatio);
                            lightsabertotalsegments = lightsabertotalsegments + 1;

                            for (int j = 0; j < MdlNodeLightsaber::NUM_SEGMENT_VERTS; j++) {
                                ASLgl::glTexCoord2f(aTexCoords[kBackPieceBase + j].u,
                                                    aTexCoords[kBackPieceBase + j].v);
                                Vector v = RotateVector(Vector(paPositions[kBackPieceBase + j].x,
                                                               paPositions[kBackPieceBase + j].y,
                                                               paPositions[kBackPieceBase + j].z), qNew);
                                v.x = vNew.x + v.x - vWorldPos.x;
                                v.y = vNew.y + v.y - vWorldPos.y;
                                v.z = vNew.z + v.z - vWorldPos.z;
#if KOTOR1
                                v = RotateVector(v, -qWorld);
#else
                                v = RotateVector(v, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
                                ASLgl::glVertex4f(v.x, v.y, v.z, 1.0f);
                                if (j == 0)
                                    ASLgl::glVertex4f(v.x, v.y, v.z, 1.0f);
                                avPiece[j] = v;

                                ASLgl::glTexCoord2f(aTexCoords[kBackPieceBase + j].u,
                                                    aTexCoords[kBackPieceBase + j].v);
                                Vector u = RotateVector(Vector(paPositions[kBackPieceBase + j].x,
                                                               paPositions[kBackPieceBase + j].y,
                                                               paPositions[kBackPieceBase + j].z), qSeg);
                                u.x = vSeg.x + u.x - vWorldPos.x;
                                u.y = vSeg.y + u.y - vWorldPos.y;
                                u.z = vSeg.z + u.z - vWorldPos.z;
#if KOTOR1
                                u = RotateVector(u, -qWorld);
#else
                                u = RotateVector(u, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
                                ASLgl::glVertex4f(u.x, u.y, u.z, 1.0f);
                                if (j == MdlNodeLightsaber::NUM_SEGMENT_VERTS - 1)
                                    ASLgl::glVertex4f(u.x, u.y, u.z, 1.0f);
                                avPiece[j] = avPiece[j] - u;
                            }
                            fRatio = fRatio + lightsabersegmentinterval
                                              / (m_afSegmentLife[nCur] - m_afSegmentLife[nPrev]);
                            vSeg = vNew;
                            qSeg = qNew;
                        } while (fRatio < 1.0f
                                 && !((fAge = fAge + lightsabersegmentinterval) > lightsabersegmentlife));
                    }
                }

                nPrev = nCur;
                nCur = nCur - 1;
                if (nCur < 0)
                    nCur += MdlNodeLightsaber::NUM_BACK_SEGMENTS;
            }
        }

        for (int i = 0; i < MdlNodeLightsaber::NUM_SEGMENT_VERTS; i++) {
            float fMag = magnitude(avPiece[i]);
            if (fMag != 0.0f) {
                if (!(fMag < fFudge * lightsaberfudgefactor)) {
                    avPiece[i] = fFudge * (avPiece[i] / fMag);
                } else {
                    avPiece[i] = vPiece;
                }
            }
        }

        for (int j = 0; j < nHalf; j++) {
            ASLgl::glTexCoord2f(aTexCoords[kBackPieceBase + nHalf + j].u,
                                aTexCoords[kBackPieceBase + nHalf + j].v);
            Vector v = RotateVector(Vector(paPositions[kBackPieceBase + j].x,
                                           paPositions[kBackPieceBase + j].y,
                                           paPositions[kBackPieceBase + j].z), qSeg);
            v.x = vSeg.x + v.x - vWorldPos.x;
            v.y = vSeg.y + v.y - vWorldPos.y;
            v.z = vSeg.z + v.z - vWorldPos.z;
#if KOTOR1
            v = RotateVector(v, -qWorld);
#else
            v = RotateVector(v, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
            ASLgl::glVertex4f(v.x + avPiece[j].x, v.y + avPiece[j].y, v.z + avPiece[j].z, 1.0f);
            if (j == 0)
                ASLgl::glVertex4f(v.x + avPiece[j].x, v.y + avPiece[j].y, v.z + avPiece[j].z, 1.0f);

            ASLgl::glTexCoord2f(aTexCoords[kBackPieceBase + j].u, aTexCoords[kBackPieceBase + j].v);
            Vector u = RotateVector(Vector(paPositions[kBackPieceBase + j].x,
                                           paPositions[kBackPieceBase + j].y,
                                           paPositions[kBackPieceBase + j].z), qSeg);
            u.x = vSeg.x + u.x - vWorldPos.x;
            u.y = vSeg.y + u.y - vWorldPos.y;
            u.z = vSeg.z + u.z - vWorldPos.z;
#if KOTOR1
            u = RotateVector(u, -qWorld);
#else
            u = RotateVector(u, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
            ASLgl::glVertex4f(u.x, u.y, u.z, 1.0f);
            if (j == nHalf - 1)
                ASLgl::glVertex4f(u.x, u.y, u.z, 1.0f);
        }

        if (lightsaberrenderfronthalf != 0) {
// For the front half, extrapolate the ribbon ahead of the newest segment.
// Each step advances the state linearly and turns the orientation by the arc that took the
// previous blade axis onto the newest one.
            Quaternion qNext;
            nPrev = m_nOrientIndex - 1;
            if (nPrev < 0)
                nPrev += MdlNodeLightsaber::NUM_BACK_SEGMENTS;
            vSeg = m_avSegmentPos[nPrev];
            qSeg = m_aqSegmentOrient[nPrev];

            if (m_nSegmentCount > 1) {
                nCur = m_nOrientIndex - 2;
                if (nCur < 0)
                    nCur += MdlNodeLightsaber::NUM_BACK_SEGMENTS;
                float fAge = 0.0f;
                Vector vB = m_avSegmentPos[nCur];
                Quaternion qB = m_aqSegmentOrient[nCur];
                Vector vA = vSeg;
                Quaternion qA = qSeg;

                for (int n = 1; n < m_nSegmentCount; n++) {
// B's blade axis, in the world-conjugate frame
                    Vector vB0 = RotateVector(Vector(paPositions[0].x, paPositions[0].y,
                                                     paPositions[0].z), qB);
                    vB0.x = vB.x + vB0.x - vWorldPos.x;
                    vB0.y = vB.y + vB0.y - vWorldPos.y;
                    vB0.z = vB.z + vB0.z - vWorldPos.z;
#if KOTOR1
                    vB0 = RotateVector(vB0, -qWorld);
#else
                    vB0 = RotateVector(vB0, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
                    Vector vB3 = RotateVector(Vector(paPositions[3].x, paPositions[3].y,
                                                     paPositions[3].z), qB);
                    vB3.x = vB.x + vB3.x - vWorldPos.x;
                    vB3.y = vB.y + vB3.y - vWorldPos.y;
                    vB3.z = vB.z + vB3.z - vWorldPos.z;
#if KOTOR1
                    vB3 = RotateVector(vB3, -qWorld);
#else
                    vB3 = RotateVector(vB3, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
                    Vector vAxisB(vB3.x - vB0.x, vB3.y - vB0.y, vB3.z - vB0.z);

// The same axis of A
                    Vector vA0 = RotateVector(Vector(paPositions[0].x, paPositions[0].y,
                                                     paPositions[0].z), qA);
                    vA0.x = vA.x + vA0.x - vWorldPos.x;
                    vA0.y = vA.y + vA0.y - vWorldPos.y;
                    vA0.z = vA.z + vA0.z - vWorldPos.z;
#if KOTOR1
                    vA0 = RotateVector(vA0, -qWorld);
#else
                    vA0 = RotateVector(vA0, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
                    Vector vA3 = RotateVector(Vector(paPositions[3].x, paPositions[3].y,
                                                     paPositions[3].z), qA);
                    vA3.x = vA.x + vA3.x - vWorldPos.x;
                    vA3.y = vA.y + vA3.y - vWorldPos.y;
                    vA3.z = vA.z + vA3.z - vWorldPos.z;
#if KOTOR1
                    vA3 = RotateVector(vA3, -qWorld);
#else
                    vA3 = RotateVector(vA3, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
                    Vector vAxisA(vA3.x - vA0.x, vA3.y - vA0.y, vA3.z - vA0.z);

                    Vector vNext((vA.x + vA.x) - vB.x, (vA.y + vA.y) - vB.y, (vA.z + vA.z) - vB.z);
                    qNext = qA * RotationArc(vAxisB, vAxisA);

                    float fRatio = lightsabersegmentinterval
                                   / (m_afSegmentLife[nCur] - m_afSegmentLife[nPrev]);
                    if (fRatio < 1.0f) {
                        fAge += lightsabersegmentinterval;
                        if (!(fAge > lightsabersegmentlife)) {
                            do {
                                float fInv = 1.0f - fRatio;
                                Vector vNew(vNext.x * fRatio + vA.x * fInv,
                                            vNext.y * fRatio + vA.y * fInv,
                                            vNext.z * fRatio + vA.z * fInv);
                                Quaternion qNew = slerp(qA, qNext, fRatio);
                                lightsabertotalsegments = lightsabertotalsegments + 1;

                                for (int j = 0; j < MdlNodeLightsaber::NUM_SEGMENT_VERTS; j++) {
                                    ASLgl::glTexCoord2f(aTexCoords[j].u, aTexCoords[j].v);
                                    Vector v = RotateVector(Vector(paPositions[j].x, paPositions[j].y,
                                                                   paPositions[j].z), qSeg);
                                    v.x = vSeg.x + v.x - vWorldPos.x;
                                    v.y = vSeg.y + v.y - vWorldPos.y;
                                    v.z = vSeg.z + v.z - vWorldPos.z;
#if KOTOR1
                                    v = RotateVector(v, -qWorld);
#else
                                    v = RotateVector(v, Quaternion(-qWorld.w, qWorld.x, qWorld.y,
                                                                   qWorld.z));
#endif
                                    ASLgl::glVertex4f(v.x, v.y, v.z, 1.0f);
                                    if (j == 0)
                                        ASLgl::glVertex4f(v.x, v.y, v.z, 1.0f);
                                    avPiece[j] = v;

                                    ASLgl::glTexCoord2f(aTexCoords[j].u, aTexCoords[j].v);
                                    Vector u = RotateVector(Vector(paPositions[j].x, paPositions[j].y,
                                                                   paPositions[j].z), qNew);
                                    u.x = vNew.x + u.x - vWorldPos.x;
                                    u.y = vNew.y + u.y - vWorldPos.y;
                                    u.z = vNew.z + u.z - vWorldPos.z;
#if KOTOR1
                                    u = RotateVector(u, -qWorld);
#else
                                    u = RotateVector(u, Quaternion(-qWorld.w, qWorld.x, qWorld.y,
                                                                   qWorld.z));
#endif
                                    ASLgl::glVertex4f(u.x, u.y, u.z, 1.0f);
                                    if (j == MdlNodeLightsaber::NUM_SEGMENT_VERTS - 1)
                                        ASLgl::glVertex4f(u.x, u.y, u.z, 1.0f);
                                    avPiece[j] = avPiece[j] - u;
                                }
                                fRatio = fRatio + lightsabersegmentinterval
                                                  / (m_afSegmentLife[nCur] - m_afSegmentLife[nPrev]);
                                vSeg = vNew;
                                qSeg = qNew;
                            } while (fRatio < 1.0f
                                     && !((fAge = fAge + lightsabersegmentinterval)
                                              > lightsabersegmentlife));
                        }
                    }

                    vB = vA;
                    qB = qA;
                    vA = vNext;
                    qA = qNext;
                    nPrev = nCur;
                    nCur = nCur - 1;
                    if (nCur < 0)
                        nCur += MdlNodeLightsaber::NUM_BACK_SEGMENTS;
                }
            }

// For each front-cap corner, rescale its last step to -fFudge and emit base / base + step
            for (int j = 0; j < nHalf; j++) {
                float fMag = magnitude(avPiece[j]);
                if (fMag != 0.0f) {
                    if (!(fMag < fFudge * lightsaberfudgefactor)) {
                        avPiece[j] = (avPiece[j] / fMag) * -fFudge;
                    } else {
                        avPiece[j].x = -vPiece.x;
                        avPiece[j].y = -vPiece.y;
                        avPiece[j].z = -vPiece.z;
                    }
                }

                ASLgl::glTexCoord2f(aTexCoords[j].u, aTexCoords[j].v);
                Vector v = RotateVector(Vector(paPositions[j].x, paPositions[j].y, paPositions[j].z),
                                        qSeg);
                v.x = vSeg.x + v.x - vWorldPos.x;
                v.y = vSeg.y + v.y - vWorldPos.y;
                v.z = vSeg.z + v.z - vWorldPos.z;
#if KOTOR1
                v = RotateVector(v, -qWorld);
#else
                v = RotateVector(v, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
                ASLgl::glVertex4f(v.x, v.y, v.z, 1.0f);
                if (j == 0)
                    ASLgl::glVertex4f(v.x, v.y, v.z, 1.0f);

                ASLgl::glTexCoord2f(aTexCoords[nHalf + j].u, aTexCoords[nHalf + j].v);
                Vector u = RotateVector(Vector(paPositions[j].x, paPositions[j].y, paPositions[j].z),
                                        qSeg);
                u.x = vSeg.x + u.x - vWorldPos.x;
                u.y = vSeg.y + u.y - vWorldPos.y;
                u.z = vSeg.z + u.z - vWorldPos.z;
#if KOTOR1
                u = RotateVector(u, -qWorld);
#else
                u = RotateVector(u, Quaternion(-qWorld.w, qWorld.x, qWorld.y, qWorld.z));
#endif
                ASLgl::glVertex4f(u.x + avPiece[j].x, u.y + avPiece[j].y, u.z + avPiece[j].z, 1.0f);
                if (j == nHalf - 1)
                    ASLgl::glVertex4f(u.x + avPiece[j].x, u.y + avPiece[j].y, u.z + avPiece[j].z,
                                      1.0f);
            }
        }

        ASLgl::glEnd();
        GLRender::SetPolygonRenderMode(Aurora::kFacingFrontAndBack, Aurora::kFillSolid);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        GLRender::Enable(Aurora::kCapLighting);
        GLRender::Enable(Aurora::kCapCullFace);
    } else {
// The static path draws two piece-cap quad strips, front cap then back cap
        GLRender::BeginImmediatePrimitive(Aurora::kPrimQuadStrip);

        for (int i = 0; i < nHalf; i++) {
            ASLgl::glTexCoord2f(aTexCoords[i].u, aTexCoords[i].v);
            ASLgl::glNormal3f(aNormals[i].x, aNormals[i].y, aNormals[i].z);
            ASLgl::glVertex4f(paPositions[i].x, paPositions[i].y, paPositions[i].z, 1.0f);
            ASLgl::glTexCoord2f(aTexCoords[nHalf + i].u, aTexCoords[nHalf + i].v);
            ASLgl::glNormal3f(aNormals[nHalf + i].x, aNormals[nHalf + i].y, aNormals[nHalf + i].z);
            ASLgl::glVertex4f(paPositions[nHalf + i].x, paPositions[nHalf + i].y,
                              paPositions[nHalf + i].z, 1.0f);
            if (i == nHalf - 1)
                ASLgl::glVertex4f(paPositions[nHalf + i].x, paPositions[nHalf + i].y,
                                  paPositions[nHalf + i].z, 1.0f);
        }

        int nBack = MdlNodeLightsaber::NUM_PIECE_VERTS
                    + MdlNodeLightsaber::NUM_FRONT_SEGMENTS * MdlNodeLightsaber::NUM_SEGMENT_VERTS;
        for (int i = nBack; i < nBack + nHalf; i++) {
            ASLgl::glTexCoord2f(aTexCoords[nHalf + i].u, aTexCoords[nHalf + i].v);
            ASLgl::glNormal3f(aNormals[nHalf + i].x, aNormals[nHalf + i].y, aNormals[nHalf + i].z);
            ASLgl::glVertex4f(paPositions[nHalf + i].x, paPositions[nHalf + i].y,
                              paPositions[nHalf + i].z, 1.0f);
            if (i == 0)
                ASLgl::glVertex4f(paPositions[nHalf + i].x, paPositions[nHalf + i].y,
                                  paPositions[nHalf + i].z, 1.0f);
            ASLgl::glTexCoord2f(aTexCoords[i].u, aTexCoords[i].v);
            ASLgl::glNormal3f(aNormals[i].x, aNormals[i].y, aNormals[i].z);
            ASLgl::glVertex4f(paPositions[i].x, paPositions[i].y, paPositions[i].z, 1.0f);
        }

        ASLgl::glEnd();
        GLRender::SetPolygonRenderMode(Aurora::kFacingFrontAndBack, Aurora::kFillSolid);
        GLRender::Enable(Aurora::kCapCullFace);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        GLRender::Enable(Aurora::kCapLighting);
    }

    RenderInterface::SetDepthMask(1);
    ASLgl::glStencilMask(0xff);
}

// The cloth mesh is drawn through the plain tri-mesh path.
void PartDanglyMesh::Draw(bool)
{
    if (renderdangly == 0)
        return;
    m_pNode->AsMdlNodeDanglyMesh();
    PartTriMesh::Draw(false);
}

const char* setenvironmentmap(char* pszParams)
{
    char szGobName[64];
    char szTexture[64];
    szTexture[0] = 0;
    szGobName[0] = 0;

    static_cast<void>(sscanf(pszParams, "%s %s", szGobName, szTexture));

    static char szBuf[256];

    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
    {
        snprintff(szBuf, 256, "couldn't find object %s", szGobName);
        return szBuf;
    }

    if (_stricmp(NULL_NAME, szTexture) == 0 || _stricmp("0", szTexture) == 0)
    {
        pGob->m_texWatch = static_cast<CAurTexture*>(0);
        snprintff(szBuf, 256, "Setting object %s environmentmap to 0", szGobName);
        return szBuf;
    }

    if (pGob->SetEnvironmentMap(szTexture) == 0)
        return "Bad texture!  Environment map not changed.";

    CAurTexture* pTexture = pGob->m_texWatch;
    snprintff(szBuf, 256, "bitmap %s  %s  texid=%d", pszParams,
              pTexture == 0 ? "not found" : "found", static_cast<int>(reinterpret_cast<intptr_t>(pTexture)));
    return szBuf;
}

static void snprintff(char* buf, uint64_t n, const char* fmt, ...) __attribute__((noinline));
static void snprintff(char* buf, uint64_t n, const char* fmt, ...)
{
    K2_UNUSED(n);
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    __vsprintf_chk(buf, 0, 256, fmt, ap);
    __builtin_va_end(ap);
}

const char* turnoffshadows(char* pszParams)
{
    char szName[64];
    szName[0] = 0;

    if (sscanf(pszParams, "%s", szName) != 1)
        return "Usage: turnoffshadowing <gobname>";

    Gob* pGob = FindGob(szName);

    static char szBuf[256];

    if (pGob == 0)
    {
        snprintff(szBuf, 256, "couldn't find object %s", szName);
        return szBuf;
    }

    pGob->TurnOffShadows();
    return "Success, shadowing turned off for specified gob.";
}

const char* turnonshadows(char* pszParams)
{
    char szName[64];
    szName[0] = 0;

    if (sscanf(pszParams, "%s", szName) != 1)
        return "Usage: turnonshadowing <gobname>";

    Gob* pGob = FindGob(szName);

    static char szBuf[256];

    if (pGob == 0)
    {
        snprintff(szBuf, 256, "couldn't find object %s", szName);
        return szBuf;
    }

    pGob->TurnOnShadows();
    return "Success, shadowing turned on for specified gob.";
}

void Part::Mat() {}

void PartLight::Draw(bool)
{
}

void PartLight::Mat()
{
    ASLgl::glColor4f(m_vLightColour.x, m_vLightColour.y, m_vLightColour.z, 1.0f);
    currentmat = m_vLightColour;
}

int ignoremat;
int alwaysmat;
int enablebumpyshiny = 1;
extern int usevertexprograms;
int AurVertexProgramsAvailable();

struct CAurMaterialContainer
{
    unsigned char m_abData[41];

    CAurMaterialContainer();
};

CAurMaterialContainer::CAurMaterialContainer() : m_abData() {}

#if KOTOR1
bool IosAntiAliasOn();
#endif
// A static part needs the material pass unless it is a plain white, opaque, unlit, undecaled
// mesh; a tiled part always does, and reports it through bTiled.
static bool StaticPartNeedsMat(PartTriMesh* part, MdlNodeTriMesh* pNode, bool& bTileMode, bool& bTiled)
{
    if (!part->m_bHasColorPool)
    {
        if (part->m_pTileData != 0)
            bTileMode = (part->m_pTileData->m_nMode != 0);
        if (bTileMode)
        {
            bTiled = true;
            return true;
        }
    }
    if (!equal(pNode->m_vDiffuse, g_vOne))
        return true;
    if (part->m_fOpacity != 1.0f)
        return true;
    if (!equal(part->m_vSelfIllumColor, g_vZero))
        return true;
    return part->m_pMaterial->m_bDecal;
}

// The gob's colour shift, fade and illumination applied to a dynamic part's material.
static void DynamicPartCalculateMaterial(Gob* pGob, Vector& vDiffuse, Vector& vEnv, float& fAlpha)
{
    if (pGob->m_nColorShifting != 0)
    {
        fAlpha = fAlpha * pGob->m_fColorShiftAmount;
        vDiffuse = Vector(vDiffuse.x * pGob->m_vColorShift.x,
                          vDiffuse.y * pGob->m_vColorShift.y,
                          vDiffuse.z * pGob->m_vColorShift.z);
    }
    if ((pGob->m_pModel->m_nFlags & 8) != 0)
        fAlpha = fAlpha * pGob->m_fFadeAlpha;
    if (pGob->m_nIllumination != 0)
        vEnv = vEnv + pGob->m_vIllumination;
}

void PartTriMesh::Mat()
{
    MdlNodeTriMesh* pNode = static_cast<MdlNodeTriMesh*>(m_pNode);
#if !KOTOR1  // K1: compares against the current colour cache, no previous-colour copies
    static Vector s_vPrevDiffuse(-1.0f, -1.0f, -1.0f);
    static Vector s_vPrevAmbient(-1.0f, -1.0f, -1.0f);
    static Vector s_vPrevEnv(-1.0f, -1.0f, -1.0f);
#endif
    static float  s_fPrevAlpha = -1.0f;
    static bool   s_bPrevTextured = true;

    bool bTileMode = false;
    bool bTiled = false;
    unsigned int nAlpha = *reinterpret_cast<const unsigned int*>(&m_fOpacity);
    float& fAlpha = *reinterpret_cast<float*>(&nAlpha);

    if (currentscene != 0 && currentscene->m_bFogEnabled != 0)
    {
        if (m_bFog)
            GLRender::Enable(Aurora::kCapFog);
#if KOTOR1 && defined(K2_MOBILE_OPT)  // K1's mobile port: drops fog only under anti-aliasing or the one fog colour
        // That colour's channels (112, 143, 142) / 255, as float bits (Scene::DoFog's table).
        enum
        {
            kFogByte112 = 0x3ee0e0e1,
            kFogByte142 = 0x3f0e8e8f,
            kFogByte143 = 0x3f0f8f90,
        };
        const Vector& vFog = currentscene->m_vFogColor;
        if ((IosAntiAliasOn() && !m_bFog) ||
            (FloatBits(vFog.x) == kFogByte112 && FloatBits(vFog.y) == kFogByte143 && FloatBits(vFog.z) == kFogByte142 &&
             !m_bFog))
#else
        if (!m_bFog)
#endif
            GLRender::Disable(Aurora::kCapFog);
    }

    bool bTextured;
    if (m_pGob != 0)
    {
        if (m_pGob->m_bHasSingleTexture == 0)
        {
            m_pMaterial->BindTexture0();
            m_pMaterial->SetBlendingMode(0);
        }
        bTextured = true;
    }
    else
    {
        bTextured = StaticPartNeedsMat(this, pNode, bTileMode, bTiled);
    }

    if (ignoremat != 0)
        return;

    static Vector s_vCurDiffuse(0.0f, 0.0f, 0.0f);
    static Vector s_vCurAmbient(0.0f, 0.0f, 0.0f);
    static Vector s_vCurEnv(0.0f, 0.0f, 0.0f);
#if KOTOR1
    static bool s_bValid;
#endif

    bool bUnchanged;

    if (bTextured)
    {
#if KOTOR1  // K1: each colour is compared against the cache as it is formed
// The colour shift scales the cached diffuse before the node's colour is compared in.
        static Vector s_vNewDiffuse(0.0f, 0.0f, 0.0f);
        static Vector s_vNewEnv(0.0f, 0.0f, 0.0f);
        s_vNewDiffuse = pNode->m_vDiffuse;
        if (!equal(s_vCurAmbient, pNode->m_vAmbient))
        {
            s_bValid = false;
            s_vCurAmbient = pNode->m_vAmbient;
        }
        s_vNewEnv = (m_pMaterial->m_bDecal == 0) ? m_vSelfIllumColor : g_vOne;

        if (m_pGob != 0)
            DynamicPartCalculateMaterial(m_pGob, s_vCurDiffuse, s_vNewEnv, fAlpha);

        if (!equal(s_vCurDiffuse, s_vNewDiffuse))
        {
            s_bValid = false;
            s_vCurDiffuse = s_vNewDiffuse;
        }
        if (!equal(s_vCurEnv, s_vNewEnv))
        {
            s_bValid = false;
            s_vCurEnv = s_vNewEnv;
        }
#else
        s_vCurDiffuse = pNode->m_vDiffuse;
        s_vCurAmbient = pNode->m_vAmbient;
        s_vCurEnv = (m_pMaterial->m_bDecal == 0) ? m_vSelfIllumColor : g_vOne;

        if (m_pGob != 0)
            DynamicPartCalculateMaterial(m_pGob, s_vCurDiffuse, s_vCurEnv, fAlpha);
#endif

        bUnchanged = true;
    }
    else
    {
        if (s_bPrevTextured == 0 && alwaysmat == 0)
            return;

#if KOTOR1
        if (!equal(s_vCurDiffuse, g_vOne))
        {
            s_vCurDiffuse = g_vOne;
            s_bValid = false;
        }
        if (!equal(s_vCurAmbient, g_vOne))
        {
            s_vCurAmbient = g_vOne;
            s_bValid = false;
        }
        if (!equal(s_vCurEnv, g_vZero))
        {
            s_vCurEnv = g_vZero;
            s_bValid = false;
        }
#else
        s_vCurDiffuse = Vector(1.0f, 1.0f, 1.0f);
        s_vCurAmbient = Vector(1.0f, 1.0f, 1.0f);
        s_vCurEnv = Vector(0.0f, 0.0f, 0.0f);
#endif

        bUnchanged = (s_bPrevTextured != 0);
    }

#if KOTOR1
    if (bUnchanged && s_bValid == true && s_fPrevAlpha == fAlpha && m_bMaterialHasChanged == 0)
#else
    if (bUnchanged && equal(s_vPrevDiffuse, s_vCurDiffuse) && equal(s_vPrevAmbient, s_vCurAmbient) &&
        equal(s_vPrevEnv, s_vCurEnv) && s_fPrevAlpha == fAlpha && m_bMaterialHasChanged == 0)
#endif
    {
        if (alwaysmat == 0)
            return;
    }

    if ((enablebumpyshiny != 0 || usevertexprograms != 0) && AurVertexProgramsAvailable() != 0)
    {
        Vector vEnv = s_vCurEnv;
        if (currentscene != 0 && m_pGob != 0)
        {
            vEnv.x = vEnv.x + currentscene->m_ambient.x * s_vCurAmbient.x;
            vEnv.y = vEnv.y + currentscene->m_ambient.y * s_vCurAmbient.y;
            vEnv.z = vEnv.z + currentscene->m_ambient.z * s_vCurAmbient.z;
        }
        float fEnvX = vEnv.x;
        float fEnvY = vEnv.y;
        float fEnvZ = vEnv.z;

        GLRender::SetVertexShaderConstant(93, fEnvX, fEnvY, fEnvZ, 1.0f);
        GLRender::SetVertexShaderConstant(94, s_vCurDiffuse.x, s_vCurDiffuse.y, s_vCurDiffuse.z, fAlpha);
        GLRender::SetVertexShaderConstant(95, s_vCurAmbient.x, s_vCurAmbient.y, s_vCurAmbient.z, 1.0f);
    }

#if KOTOR1
    m_bMaterialHasChanged = false;
    s_bValid = true;
    s_fPrevAlpha = fAlpha;
    s_bPrevTextured = bTextured;
#else
    s_bPrevTextured = bTextured;
    s_fPrevAlpha = fAlpha;
    s_vPrevDiffuse = s_vCurDiffuse;
    s_vPrevAmbient = s_vCurAmbient;
    s_vPrevEnv = s_vCurEnv;
    m_bMaterialHasChanged = false;
#endif
}

void Gob::PartDraw(Part* pPart, bool b)
{
    if (!pPart->m_bDrawEnabled)
        return;

    Vector vSavedMat = currentmat;
    ASLgl::glPushMatrix();
    const Vector& v = pPart->m_vLocalPosition;
    const Quaternion& q = pPart->m_qLocalOrientation;
    if (enabletransform)
        GLRender::Transform(v, q, 0);
    ASLgl::glScalef(pPart->m_scale.m_fScale, pPart->m_scale.m_fScale, pPart->m_scale.m_fScale);

    if (pPart->AsPartTriMesh() == 0 || pPart->m_pNode == 0 ||
        static_cast<MdlNodeTriMesh*>(pPart->m_pNode)->m_bRender)
    {
        pPart->Mat();
        pPart->Draw(b);
    }

    if (rendermaxdepth != 0)
    {
        rendermaxdepth--;
        for (int i = 0; i < pPart->m_nChildren; i++)
            PartDraw(pPart->m_paChildren[i], b);
        rendermaxdepth++;
    }

    ASLgl::glPopMatrix();
    currentmat = vSavedMat;
}

void Part::Project(Vector const&, float, Plane const&, CAurShadowData*) {}

extern Vector planelineintersection(const Vector& vNormal, float fDist, const Vector& vLineStart,
                                    const Vector& vLineEnd);

// Pushes a shadow-volume point back onto the projection plane.
void ClipProjection(Vector& v, const Vector& vOrigin, const Plane& plane)
{
    const Vector& vNormal = *reinterpret_cast<const Vector*>(&plane);
    if (dot(v, vNormal) + plane.d > 0.0f)
        v = planelineintersection(vNormal, plane.d, vOrigin, v);
}

struct ShadowInfo
{
    float m_fDist;
    short m_anNeighbors[3];
    unsigned short m_anVertIndices[3];
};
static ShadowInfo g_aShadowFaces[2048];

// Pulls the shadow volume's near cap and far sink back onto the receiver plane's front side.
// False when the sink is behind a plane that faces away from the origin: nothing to cast.
static inline bool ClipProjectionCaps(Vector& vCap, Vector& vSink, const Plane& plane)
{
    const Vector& vNormal = *reinterpret_cast<const Vector*>(&plane);
    if (dot(vSink, vNormal) + plane.d > 0.0f)
    {
        if (plane.d > 0.0f)
            return false;
        vSink = planelineintersection(vNormal, plane.d, vCap, vSink);
    }
    if (dot(vCap, vNormal) + plane.d > 0.0f)
        vCap = planelineintersection(vNormal, plane.d, vCap, vSink);
    return true;
}

// Each face's signed distance to the light, with its neighbour and vertex indices.
static void ProcessFaces(const Vector& light, ShadowInfo* pInfo, MdlNodeTriMesh* node)
{
    for (int i = 0; i < node->m_faces.m_count; i++)
    {
        MaxFace& face = node->m_faces.m_pData[i];
        pInfo[i].m_fDist = dot(face.Normal(), light) + face.m_fPlaneDist;
        pInfo[i].m_anNeighbors[0] = face.m_anNeighbors[0];
        pInfo[i].m_anNeighbors[1] = face.m_anNeighbors[1];
        pInfo[i].m_anNeighbors[2] = face.m_anNeighbors[2];
        pInfo[i].m_anVertIndices[0] = face.m_anVertIndices[0];
        pInfo[i].m_anVertIndices[1] = face.m_anVertIndices[1];
        pInfo[i].m_anVertIndices[2] = face.m_anVertIndices[2];
    }
}

extern int systemmemoryshadows;
extern int useshadowmethod;
extern int usevertexprograms;
extern float epsilon;
void PartTriMesh::Project(const Vector& light, float fDist, const Plane& plane, CAurShadowData* data)
{
    if (m_pNode->AsMdlNodeLightsaber() != 0)
        return;

    MdlNodeTriMesh* node = static_cast<MdlNodeTriMesh*>(m_pVertexPrimitive->m_pNode);

    static Vector vp2v0;
    static Vector vp2v1;
    static Vector invVp;
    static Vector cap;
    static Vector sink;

    if (node->m_bShadow == 0 && node->m_bBeaming == 0)
        return;

    if (useshadowmethod != 3 || usevertexprograms == 0 || systemmemoryshadows == 0)
    {
        bool beaming = node->m_bBeaming;
        if (data != 0)
            data->m_nCount = 0;

        Vector vDir = normalize(Vector(-light.x, -light.y, -light.z));
        cap = vDir * epsilon;
        sink = light + vDir * fDist;

        if (beaming == 0 && !ClipProjectionCaps(cap, sink, plane))
            return;

        if (systemmemoryshadows == 0)
        {
            __builtin___memset_chk(GLRender::m_UsedVertices, 0,
                                   static_cast<short>(node->m_nVertexCount) * sizeof(unsigned short),
                                   __builtin_object_size(GLRender::m_UsedVertices, 0));

            ProcessFaces(light, g_aShadowFaces, node);

            unsigned short* pOut = PartTriMesh::m_VertexIndices;
            for (int i = 0; i < node->m_faces.m_count; i++)
            {
                ShadowInfo& rec = g_aShadowFaces[i];
                float fFaceDist = rec.m_fDist;
                if (fFaceDist < 0.0f)
                    continue;
                if (rec.m_anNeighbors[0] != -1 &&
                    fFaceDist * g_aShadowFaces[rec.m_anNeighbors[0]].m_fDist > 0.0f)
                {
                }
                else
                {
                    short v0 = rec.m_anVertIndices[0];
                    short v1 = rec.m_anVertIndices[1];
                    pOut[0] = v0;
                    pOut[1] = v1;
                    pOut += 2;
                    GLRender::m_UsedVertices[v0] = 1;
                    GLRender::m_UsedVertices[v1] = 1;
                }
                if (rec.m_anNeighbors[1] != -1 &&
                    fFaceDist * g_aShadowFaces[rec.m_anNeighbors[1]].m_fDist > 0.0f)
                {
                }
                else
                {
                    short v1 = rec.m_anVertIndices[1];
                    short v2 = rec.m_anVertIndices[2];
                    pOut[0] = v1;
                    pOut[1] = v2;
                    pOut += 2;
                    GLRender::m_UsedVertices[v1] = 1;
                    GLRender::m_UsedVertices[v2] = 1;
                }
                if (rec.m_anNeighbors[2] != -1 &&
                    fFaceDist * g_aShadowFaces[rec.m_anNeighbors[2]].m_fDist > 0.0f)
                {
                }
                else
                {
                    short v2 = rec.m_anVertIndices[2];
                    short v0 = rec.m_anVertIndices[0];
                    GLRender::m_UsedVertices[v2] = 1;
                    pOut[0] = v2;
                    pOut[1] = v0;
                    pOut += 2;
                    GLRender::m_UsedVertices[v0] = 1;
                }
            }

            int nEdges = static_cast<int>((pOut - PartTriMesh::m_VertexIndices) / 2);
            if (nEdges >= 1)
            {
                RenderVertexV* pVerts;
                if (AsPartDanglyMesh() != 0)
                    pVerts = reinterpret_cast<RenderVertexV*>(AsPartDanglyMesh()->m_paVertices);
                else
                    pVerts = reinterpret_cast<RenderVertexV*>(node->m_paVertices);
                int nStart = GLRender::BuildShadowArray(light, fDist, plane, cap, sink,
                                                        static_cast<short>(node->m_nVertexCount), pVerts);
                GLRender::RenderShadowArray(nStart, nEdges, beaming);
            }
            if (data != 0)
                data->m_nCount = nEdges;
            return;
        }
    }

    MdlNodeTriMesh* pNode = static_cast<MdlNodeTriMesh*>(m_pNode->AsMdlNodeTriMesh());
    GLRender::SetInterleavedBuffer(pNode->m_nVertexStride, pNode->m_uVertexFlags,
                                   pNode->m_pTree->AsModel()->m_nPool,
                                   static_cast<short>(pNode->m_nVertexCount) + 1, pNode, pNode->m_nGeomBase);
    GLRender::SetFrontFace(Aurora::kCullCW);
    GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilIncr, Aurora::kStencilKeep);
    m_pVertexPrimitive->AsVertexPrimitiveFlat();
    GLRender::SetFrontFace(Aurora::kCullCCW);
    GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilDecr, Aurora::kStencilKeep);
    m_pVertexPrimitive->AsVertexPrimitiveFlat();
}

void PartSkin::Project(const Vector& light, float fDist, const Plane& plane, CAurShadowData* data)
{
    if (doskinshadows == 0)
        return;

    MdlNodeTriMesh* node = static_cast<MdlNodeTriMesh*>(m_pVertexPrimitive->m_pNode);
    bool beaming = node->m_bBeaming;
    if (data != 0)
        data->m_nCount = 0;

    static Vector vp2v0;
    static Vector vp2v1;
    static Vector invVp;
    static Vector cap;
    static Vector sink;

    invVp = normalize(-light);
    cap = invVp * epsilon;
    sink = light + invVp * fDist;

    if (beaming == 0 && !ClipProjectionCaps(cap, sink, plane))
        return;

    if (useshadowmethod == 0 || usevertexprograms == 0 || AurVertexProgramsAvailable() == 0)
        return;

    GLRender::EnableClientState(Aurora::kClientStateAttrib1);
    GLRender::EnableClientState(Aurora::kClientStateAttrib4);

    Part* part = m_pVertexPrimitive->m_pPart->AsPartSkin();
    MdlNodeSkin* pNode = static_cast<MdlNodeSkin*>(part->m_pNode);
    part->AsPartTriMesh();
    m_pVertexPrimitive->SetupVertexArraysStandard();

    static List<Quaternion> qbones;
    static List<Vector>     tbones;
    qbones.m_count = 0;
    tbones.m_count = 0;

    GLRender::SetVertexShaderConstant(16, 0.0f, 2.0f, 3.0f, 1.0f);

// Walk the parent chain, composing the part's world frame.
#if KOTOR1  // K1: the same gather through the out-of-line inverse and product operators
    Part* pTop = part;
    Quaternion qInv = -part->m_qLocalOrientation;
    Vector vAcc = RotateVector(-part->m_vLocalPosition, qInv);
    Quaternion qAcc = -part->m_qLocalOrientation;
    Part* pParent = static_cast<Part*>(part->m_pParent);
    while (pParent != 0) {
        Part* pCur = pParent;
        qInv = -pCur->m_qLocalOrientation;
        Vector v = RotateVector(-pCur->m_vLocalPosition, qInv);
        v = RotateVector(v, qAcc);
        vAcc = vAcc + v;
        qAcc = qAcc * -pCur->m_qLocalOrientation;
        pTop = pCur;
        pParent = static_cast<Part*>(pCur->m_pParent);
    }
    GatherBones(pTop, tbones, qbones, vAcc, qAcc);
#else
    Part* pTop = part;
    float f17, f18, f20;
    float f19, f21, f22, f23;
    {
        float qw = part->m_qLocalOrientation.w;
        float qx = part->m_qLocalOrientation.x;
        float qy = part->m_qLocalOrientation.y;
        float qz = part->m_qLocalOrientation.z;
        float nw = -qw;
        f21 = nw; f19 = qx; f22 = qy; f23 = qz;
        f17 = -part->m_vLocalPosition.x;
        f18 = -part->m_vLocalPosition.y;
        f20 = -part->m_vLocalPosition.z;
        if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f)) {
            float t1 = qy * qy + qz * qz;
            float t2 = qx * qx + qz * qz;
            float t3 = qx * qy - qz * nw;
            float t4 = qx * qy + qz * nw;
            float t5 = qy * nw + qx * qz;
            float t6 = qx * qz - qy * nw;
            float t7 = qy * qz - qx * nw;
            float t8 = qx * nw + qy * qz;
            float t9 = qx * qx + qy * qy;
            t4 = (t4 + t4) * f17;
            t6 = (t6 + t6) * f17;
            t8 = (t8 + t8) * f18;
            f17 = (1.0f - (t1 + t1)) * f17 + (t3 + t3) * f18 + (t5 + t5) * f20;
            f18 = t4 + (1.0f - (t2 + t2)) * f18 + (t7 + t7) * f20;
            f20 = t6 + t8 + (1.0f - (t9 + t9)) * f20;
        }
    }
    Part* pParent = static_cast<Part*>(part->m_pParent);
    while (pParent != 0) {
        Part* pCur = pParent;
        float qw = pCur->m_qLocalOrientation.w;
        float qx = pCur->m_qLocalOrientation.x;
        float qy = pCur->m_qLocalOrientation.y;
        float qz = pCur->m_qLocalOrientation.z;
        float vx = -pCur->m_vLocalPosition.x;
        float vy = -pCur->m_vLocalPosition.y;
        float vz = -pCur->m_vLocalPosition.z;
// (1) Rotate the parent offset by the parent's own quaternion.
        if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f)) {
            float nw = -qw;
            float t1 = qy * qy + qz * qz;
            float t2 = qx * qx + qz * qz;
            float t3 = qx * qy - qz * nw;
            float t4 = qx * qy + qz * nw;
            float t5 = qy * nw + qx * qz;
            float t6 = qx * qz - qy * nw;
            float t7 = qy * qz - qx * nw;
            float t8 = qx * nw + qy * qz;
            float t9 = qx * qx + qy * qy;
            float rx = (1.0f - (t1 + t1)) * vx + (t3 + t3) * vy + (t5 + t5) * vz;
            float ry = (t4 + t4) * vx + (1.0f - (t2 + t2)) * vy + (t7 + t7) * vz;
            float rz = (t6 + t6) * vx + (t8 + t8) * vy + (1.0f - (t9 + t9)) * vz;
            vx = rx;
            vy = ry;
            vz = rz;
        }
// (2) Rotate the result again by the accumulated quaternion.
        if (quaternionidentityoptimization == 0 || (f21 != 1.0f && f21 != -1.0f)) {
            float aw = f21, ax = f19, ay = f22, az = f23;
            float t1 = ay * ay + az * az;
            float t2 = ax * ax + az * az;
            float t3 = ax * ay - aw * az;
            float t4 = ax * ay + aw * az;
            float t5 = aw * ay + ax * az;
            float t6 = ax * az - aw * ay;
            float t7 = ay * az - aw * ax;
            float t8 = aw * ax + ay * az;
            float t9 = ax * ax + ay * ay;
            float rx = (1.0f - (t1 + t1)) * vx + (t3 + t3) * vy + (t5 + t5) * vz;
            float ry = (t4 + t4) * vx + (1.0f - (t2 + t2)) * vy + (t7 + t7) * vz;
            float rz = (t6 + t6) * vx + (t8 + t8) * vy + (1.0f - (t9 + t9)) * vz;
            vx = rx;
            vy = ry;
            vz = rz;
        }
        f17 = f17 + vx;
        f18 = f18 + vy;
        f20 = f20 + vz;
        float aw = f21, ax = f19, ay = f22, az = f23;
        float nw = -qw;
        float p1 = aw * qx;
        float p2 = aw * qy;
        float p3 = ax * qz;
        float p4 = aw * qz;
        float p5 = ax * qy;
        float p6 = ay * qx;
        f21 = (aw * nw - ax * qx - ay * qy) - az * qz;
        f19 = (ax * nw + p1 + ay * qz) - az * qy;
        f22 = (p2 - p3) + ay * nw + az * qx;
        f23 = az * nw + ((p5 + p4) - p6);
        pTop = pCur;
        pParent = static_cast<Part*>(pCur->m_pParent);
    }
    GatherBones(pTop, tbones, qbones,
                Vector(f17, f18, f20),
                Quaternion(f21, f19, f22, f23));
#endif

    if (0 < pNode->m_nBoneCount) {
        int i = 0;
        do {
            int bone = static_cast<int>(pNode->m_paBoneMap[i]);
            if (bone != -1) {
                const Quaternion& qa = qbones.m_pData[i];
                float aw = qa.w;
                float rx, ry, rz;
                if (quaternionidentityoptimization == 0 || (aw != 1.0f && aw != -1.0f)) {
                    float ax = qa.x, ay = qa.y, az = qa.z;
                    const Vector& vb = pNode->m_lTBoneRefInv.m_pData[i];
                    float bx = vb.x, by = vb.y, bz = vb.z;
                    float t1 = ay * ay + az * az;
                    float t2 = ax * ax + az * az;
                    float t3 = ax * ay - aw * az;
                    float t4 = ax * ay + aw * az;
                    float t5 = aw * ay + ax * az;
                    float t6 = ax * az - aw * ay;
                    float t7 = ay * az - aw * ax;
                    float t8 = aw * ax + ay * az;
                    float t9 = ax * ax + ay * ay;
                    rx = (1.0f - (t1 + t1)) * bx + (t3 + t3) * by + (t5 + t5) * bz;
                    ry = (t4 + t4) * bx + (1.0f - (t2 + t2)) * by + (t7 + t7) * bz;
                    rz = (t6 + t6) * bx + (t8 + t8) * by + (1.0f - (t9 + t9)) * bz;
                } else {
                    const Vector& vb = pNode->m_lTBoneRefInv.m_pData[i];
                    rx = vb.x; ry = vb.y; rz = vb.z;
                }
                Vector& tv = tbones.m_pData[i];
                tv.x = rx + tv.x;
                tv.y = ry + tv.y;
                tv.z = rz + tv.z;
                Quaternion* pAccQuat = &qbones.m_pData[i];
                *pAccQuat = *pAccQuat * pNode->m_lQBoneRefInv.m_pData[i];
                // inlined Quaternion::getmatrix() const
                if (usearbvertexprograms != 0) {
                    float av = pAccQuat->w, bx = pAccQuat->x, by = pAccQuat->y, bz = pAccQuat->z;
                    const Vector& vr = tbones.m_pData[i];
                    int ib = bone * 3;
// Row 0 of the composed rotation matrix.
                    float m11 = by * by + bz * bz;
                    float m12 = bx * by - bz * av;
                    float m13 = bx * bz + by * av;
                    ASLgl::glProgramEnvParameter4fARB(
                        GL_VERTEX_PROGRAM_ARB, ib + 18, 1.0f - (m11 + m11), m12 + m12, m13 + m13, vr.x);
                    if (usearbvertexprograms != 0) {
                        float m21 = bx * by + bz * av;
                        float m22 = bx * bx + bz * bz;
                        float m23 = by * bz - bx * av;
                        ASLgl::glProgramEnvParameter4fARB(
                            GL_VERTEX_PROGRAM_ARB, ib + 19, m21 + m21, 1.0f - (m22 + m22), m23 + m23, vr.y);
                        if (usearbvertexprograms != 0) {
                            float m31 = bx * bz - by * av;
                            float m32 = by * by + bx * bx;
                            float m33 = by * bz + bx * av;
                            ASLgl::glProgramEnvParameter4fARB(
                                GL_VERTEX_PROGRAM_ARB, ib + 20, m31 + m31, m33 + m33,
                                1.0f - (m32 + m32), vr.z);
                        }
                    }
                }
            }
            i++;
        } while (i < pNode->m_nBoneCount);
    }

    GLRender::SetFrontFace(Aurora::kCullCW);
    GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilIncr, Aurora::kStencilKeep);
    m_pVertexPrimitive->AsVertexPrimitiveSkinned();
    GLRender::SetFrontFace(Aurora::kCullCCW);
    GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilDecr, Aurora::kStencilKeep);
    m_pVertexPrimitive->AsVertexPrimitiveSkinned();
}

// Recursively projects the part subtree's shadow volume into the light's frame.
void Gob::PartVolumeProject(Part* pPart, Vector v, float f, Plane p)
{
    if (!pPart->m_bDrawEnabled)
        return;

    ASLgl::glPushMatrix();

    if (m_pPart == pPart)
    {
        const Vector& vGob = m_vPosition;
        const Quaternion& qGob = m_qOrientation;
        if (enabletransform)
            GLRender::Transform(vGob, qGob, 1);
        if (!systemmemoryshadows)
        {
            v = WorldToLocal(v, qGob, vGob);
            p.Transform(vGob, qGob);
        }
        if (m_fObjectScale != 1.0f)
            ASLgl::glScalef(m_fObjectScale, m_fObjectScale, m_fObjectScale);
    }

    Plane planeLocal = {};
    if (!systemmemoryshadows)
    {
        v = WorldToLocal(v, pPart->m_qLocalOrientation, pPart->m_vLocalPosition);
        planeLocal = p;
        planeLocal.Transform(pPart->m_vLocalPosition, pPart->m_qLocalOrientation);
    }

    if (enabletransform)
        GLRender::Transform(pPart->m_vLocalPosition, pPart->m_qLocalOrientation, 0);
#if !KOTOR1
    counttrans++;
#endif
    if (pPart->m_scale.m_fScale != 1.0f)
        ASLgl::glScalef(pPart->m_scale.m_fScale, pPart->m_scale.m_fScale, pPart->m_scale.m_fScale);

    if (systemmemoryshadows || v.x != 0.0f || v.y != 0.0f || v.z != 0.0f)
    {
        if (pPart->AsPartTriMesh() != 0 && static_cast<MdlNodeTriMesh*>(pPart->m_pNode)->m_bBeaming)
            SetBeaming();
        pPart->Project(v, f, planeLocal, 0);
    }

    for (int i = 0; i < pPart->m_nChildren; i++)
        PartVolumeProject(pPart->m_paChildren[i], v, f, planeLocal);

    ASLgl::glPopMatrix();
}

void Gob::RenderBlur()
{
    ASLgl::glPushMatrix();
    const Vector& v = m_vPosition;
    const Quaternion& q = m_qOrientation;
    if (enabletransform)
        GLRender::Transform(v, q, 1);
    if (m_fObjectScale != 1.0f)
        ASLgl::glScalef(m_fObjectScale, m_fObjectScale, m_fObjectScale);
    ProxyPartDraw(m_pPart, true);
    m_vPreviousPosition = v;
    m_qPreviousOrientation = q;
    ASLgl::glPopMatrix();
}

inline void Gob::ProxyPartDraw(Part* pPart, bool b)
{
    K2_UNUSED(pPart);
    if (m_pTileData != 0 && m_pTileData->m_fFade < 1.0f)
    {
        GLRender::Enable(Aurora::kCapFog);

        float fFade = m_pTileData->m_fFade * m_pTileData->m_fFade;
        Vector vSceneFog = m_pScene->m_vFogColor;
        Vector vFogColor = { vSceneFog.x * fFade, vSceneFog.y * fFade, fFade * vSceneFog.z };
        GLRender::SetFogModev(Aurora::kCapFogColor, vFogColor);
        GLRender::SetFogModef(Aurora::kCapFogDensity, m_pScene->m_fFogDensity * 2.0f);
        GLRender::SetFogModef(Aurora::kCapFogStart,
                      m_pTileData->m_fFade * m_pScene->m_fFogStart);
        GLRender::SetFogModef(Aurora::kCapFogEnd,
                      m_pTileData->m_fFade * m_pScene->m_fFogEnd + 0.1f);

        PartDraw(m_pPart, b);

        if (!m_pScene->m_bFogEnabled)
            GLRender::Disable(Aurora::kCapFog);
        float fDensity = m_pScene->m_fFogDensity;
        GLRender::SetFogModef(Aurora::kCapFogDensity, fDensity);
        GLRender::SetFogModef(Aurora::kCapFogStart, m_pScene->m_fFogStart);
        GLRender::SetFogModef(Aurora::kCapFogEnd, m_pScene->m_fFogEnd);
        GLRender::SetFogModev(Aurora::kCapFogColor, m_pScene->m_vFogColor);
    }
    else
    {
        PartDraw(m_pPart, b);
    }
}

Gob* FindGob(char* pszName);

char* setpersonalradius(char* pszArgs)
{
    char szGobName[64];
    float fRadius = 0.0f;
    static_cast<void>(sscanf(pszArgs, "%s %f", szGobName, &fRadius));
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("no object by that name");
    pGob->m_fPersonalRadius = fRadius;
    return const_cast<char*>("done");
}

char* setcreatureradius(char* pszArgs)
{
    char szGobName[64];
    float fRadius = 0.0f;
    static_cast<void>(sscanf(pszArgs, "%s %f", szGobName, &fRadius));
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("no object by that name");
    pGob->m_fCreatureRadius = fRadius;
    return const_cast<char*>("done");
}

static float aDefaultPathPoints[12] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                                       1.0f, 2.0f, 0.0f, 2.0f, 0.0f, 0.0f};

char* setpath(char* pszArgs)
{
    char szGobName[64];
    static_cast<void>(sscanf(pszArgs, "%s", szGobName));
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("no object by that name");
    pGob->m_pathInfo.m_nPathFloats = 12;
    pGob->m_pathInfo.m_pPathFloats = aDefaultPathPoints;
    return const_cast<char*>("done");
}

static void SetAlphaRecursive(Part* pPart, float fAlpha)
{
    if (pPart->AsPartTriMesh() != 0)
        pPart->AsPartTriMesh()->m_fOpacity = fAlpha;
    if (pPart->m_nChildren >= 1)
        for (int i = 0; i < pPart->m_nChildren; i++)
            SetAlphaRecursive(pPart->m_paChildren[i], fAlpha);
}

char* setalpha(char* pszArgs)
{
    char szGobName[64];
    float fAlpha = 1.0f;
    static_cast<void>(sscanf(pszArgs, "%s %f", szGobName, &fAlpha));
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("no object by that name");
    SetAlphaRecursive(pGob->m_pPart, fAlpha);
    return const_cast<char*>("done");
}

void setalpha(CAurObject* pObject, float fAlpha)
{
    SetAlphaRecursive(static_cast<Gob*>(pObject)->m_pPart, fAlpha);
}

char* setseparatespecular(char* pszArgs)
{
    int nMode = atoi(pszArgs);
    int nResult = GLRender::SetSpecularMode(nMode);
    return nResult != 0 ? const_cast<char*>("Success")
                        : const_cast<char*>("Failed, card does not support separate specular color");
}

int enablegobrender = 1;
int enablerendercharacter = 1;
extern int    enableplaceholders;
int g_bFadeDoors;
int rendergobspheres;
int rendergobbbox;
extern int    countgobs;
extern float  DeltaT;
extern float  fadeToZeroTime;
float mindooralpha = 0.25f;
extern List<Gob*> blurbucket;
#if KOTOR1
extern int g_RenderSkip;
#endif

void Gob::Render(bool b)
{
    if (enablegobrender == 0)
        return;

    if (m_pPart == 0 ||
        (enablerendercharacter == 0 && m_pModel != 0 && (m_pModel->m_nFlags & 4) != 0) ||
        (m_bHasGeometry == 0 && enableplaceholders == 0))
    {
        m_vPreviousPosition = m_vPosition;
        m_qPreviousOrientation = m_qOrientation;
        return;
    }

    if (g_bFadeDoors != 0 && (m_pModel->m_nFlags & 8) != 0 && currentscene != 0)
    {
        Vector vCamera = CurrentCamera->m_vPosition;
        Vector vFocus = currentscene->m_vSceneFocus;
        CHitInfo hitInfo(0, CHitInfo::kMaterialAllButTrigger, 0);
        hitInfo.m_bSkipFaded = 0;
        float fFade = HitCheck(vCamera, vFocus, &vFocus, &hitInfo) != 0
                          ? -(DeltaT / fadeToZeroTime)
                          : DeltaT / fadeToZeroTime;
        m_fFadeAlpha = m_fFadeAlpha + fFade;
        float fOne = 1.0f;
        const float* pfAlpha = m_fFadeAlpha < fOne ? &m_fFadeAlpha : &fOne;
        float fAlpha = mindooralpha < *pfAlpha ? *pfAlpha : mindooralpha;
        m_fFadeAlpha = fAlpha;
    }

    DoEvent(const_cast<char*>("GetPersonalRadius"), &m_fPersonalRadius);
    DoEvent(const_cast<char*>("GetCreatureRadius"), &m_fCreatureRadius);
    DoEvent(const_cast<char*>("GetPath"), &m_pathInfo);
#if !KOTOR1  // K1: no gob counter; a render-skip frame draws nothing
    countgobs++;
#endif
    m_bBonesDirty = 1;

#if KOTOR1
    if (!g_RenderSkip)
    {
#endif
    ASLgl::glPushMatrix();
    const Vector& v = m_vPosition;
    const Quaternion& q = m_qOrientation;
    if (enabletransform)
        GLRender::Transform(v, q, 1);
    if (m_fObjectScale != 1.0f)
        ASLgl::glScalef(m_fObjectScale, m_fObjectScale, m_fObjectScale);
    if (m_bHasSingleTexture)
    {
        m_pSingleMaterial->BindTexture0();
        m_pSingleMaterial->SetBlendingMode(0);
    }
    ProxyPartDraw(m_pPart, false);
    if (m_bMotionBlurred)
        blurbucket.Add(this);
    else
    {
        m_vPreviousPosition = v;
        m_qPreviousOrientation = q;
    }
    if (rendergobspheres == 1)
    {
        GLRender::DrawSphereBound(m_fMinSphereRadius, m_vMinSphereCenter,
                                  Vector(1.0f, 1.0f, 0.0f), 1.0f, true);
    }
    if (rendergobbbox != 0 || m_nRenderGobBBox != 0)
        GLRender::DrawAxisAlignedFrame(m_vBoundingBoxMin, m_vBoundingBoxMax, m_vGobBBox);
    ASLgl::glPopMatrix();
#if KOTOR1
    }
#endif
    if (b)
        DoEvent(const_cast<char*>("propagaterender"), 0);
}



class Scene;


extern char cStandardVP[];
extern char szStandardVPARB[];
extern unsigned int* cStandardVPDeclaration;
extern char cBumpyShinyVP[];
extern unsigned int* cBumpyShinyRegularVPDeclaration;
extern int usearbvertexprograms;
int enableold;
int enabledecal = 1;
float shinyalpha = 1.0f;
int enableonepassenvmap = 1;
int environmentmapdest;
int bumpspecular = 1;
float diffuseintensity = 1.0f;
extern int quaternionidentityoptimization;
extern char cBumpMapVP[];
extern unsigned int* cBumpMapVPDeclaration;
extern char cBumpMapDiffuseVP[];
extern unsigned int* cBumpMapDiffuseVPDeclaration;
extern char cBumpMapSpecularVP[];
extern unsigned int* cBumpMapSpecularVPDeclaration;
int bmspecularexponent = 1;

extern char cBumpMapOnePassVP[];
extern char szBumpmapVPARB[];
extern unsigned int* cBumpMapOnePassVPDeclaration;
extern char cFogOutputVP[];
extern unsigned int* cFogOutputVPDeclaration;
int enablerenderbumpmap = 1;
int bumpdiffuse = 1;
int bumpother = 1;
int bumptexture = 1;
extern int usebumpdiffuse;
extern int usebumpspecular;
int useonepasssolution = 1;

extern char szNormalmapVPARB[];
extern unsigned int* szNormalmapVPARBDeclaration;
int showrancoraxes;

char cMotionBlurredVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4   R9.x,\tc[0],\tv[OPOS];DP4   R9.y,\tc[1],\tv[OPOS];DP4   R9.z,\tc[2],\tv[OPOS];DP4   R9.w,\tc[3],\tv[OPOS];DP4   R10.x, c[8],\tv[OPOS];DP4   R10.y, c[9],\tv[OPOS];DP4   R10.z, c[10],\tv[OPOS];DP4   R10.w, c[11],\tv[OPOS];DP3   R1.w,\tR10,\tR10;RSQ   R1.w,\tR1.w;RCP   R1.w,\tR1.w;MOV   o[FOGC].x, -R1.w;DP3   R11.x,\tc[8],  v[NRML];DP3   R11.y, c[9],  v[NRML];DP3   R11.z, c[10], v[NRML];MOV   R4, c[50];DP3   R6.x,\t R4, R4;RSQ   R6.y,\t R6.x;MUL   R4.xyz, R4, R6.y;MOV   R7.x, c[51].x;MIN   R2.y, R7.x, c[51].y;MUL   R5.xyz, R4, R2.y;MOV   R5.w, c[17].x;DP3   R3.w, R4, v[NRML];SLT   R2.w, R3.w, c[17].x;MOV   R1, c[53].xyzw;MOV   R1.z, c[17].x;MAD   R6, R2.w, -R5, v[OPOS].xyzw;MUL   R7.y, R2.y, c[13].x;RCP   R8.y, c[51].y;MUL   R8.w, R2.y, R8.y;MUL   R7.y, R7.y, R8.w;MUL   R7.y, R7.y, c[52].w;MOV   R3.w, c[17].x;MOV   R3.xyz, c[52];DP3   R3.w, R3, R3;RSQ   R5.w, R3.w;MUL   R3,   R3, R5.w;ADD   R5.xyz, R6, -R1;DP3   R5.w, R5, R3;SLT   R5.x, R5.w, c[17].x;MOV   R7.x, c[17].w;MAD   R5.y, R5.x, -c[17].y, R7.x;MUL   R5.z, R5.y, R7.y;MUL   R5.z, R5.z, R2.w;MAD   R6, R5.z, R3, R6;ADD   R2.x, -R2.w, c[17].w;MUL   R1, v[NRML], -c[51].z;MOV   R1.w, c[17].x;MAD   R5, R2.x, R1, R6;DP4   o[HPOS].x, R5, c[0];DP4   o[HPOS].y, R5, c[1];DP4   o[HPOS].z, R5, c[2];DP4   o[HPOS].w, R5, c[3];ADD   R2.w, c[17].w, -R2.w;MOV R9.xyz, c[17].xxxx;MUL R9.w, c[17].z, R2.w;MOV o[COL0], R9;MOV   o[TEX0], v[TEX0];";
unsigned int* cMotionBlurredVPDeclaration;
extern float DeltaT;
extern unsigned char g_useDistortionOutput;

float specularintensity = 1.0f;
extern Camera* CurrentCamera;

#if !KOTOR1  // K1: inlines the copy at every call site
extern template List<PartLight*>::List(List<PartLight*>&);
#endif
int AurCubeMapAvailable();
int AurOnePassEnvMapAvailable();
bool AurATIValidate();

class CAurTexture;
bool AurNV1xLevelShadersAvailable();
bool AurNV2xLevelShadersAvailable();
bool AurTextureEnvMapDisabled();
int AurMultiTextureAvailable();
int AurPixelShadersAvailable();
void SetupForceSightShader(Gob* gob, int a, int b, CAurTexture* c1, CAurTexture* c2,
                           CAurTexture* c3, CAurTexture* c4, bool flag);

int enablerender = 1;
int enablerenderlightmapped = 1;
int renderemlm = 1;
int enablerenderenv = 1;
int enableenvmap = 1;
int enablenoshaderlightmaps = 1;
int enablelighting = 1;
extern int countparts;
extern int countfaces;

void RenderFlat(VertexPrimitiveFlat* vertices);
void RenderFlatNonTextured(VertexPrimitiveFlat* vertices);
void RenderFlatDecal(VertexPrimitiveFlat* vertices);
void RenderDiffuseBumpMappedPass(VertexPrimitiveFlat* vertices);
void RenderSpecularBumpMappedPass(VertexPrimitiveFlat* vertices);

void RenderBumpedOutTriMesh(VertexPrimitiveFlat* vertices)
{
    K2_UNUSED(vertices);
}

// Object-space motion blur; a part that did not move is not blurred.
void RenderMotionBlurred(VertexPrimitiveFlat* vertices)
{
    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    Quaternion qLocalOrientation;
    Vector vLocalPosition;

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();

    Vector vMotion = part->m_pGob->m_vPosition - part->m_pGob->m_vPreviousPosition;
    float fMotion = magnitude(vMotion);

    if (fMotion > 0.0f)
    {
        unsigned char bAlpha = GLRender::IsEnabled(Aurora::kCapAlphaTest);
        GLRender::Disable(Aurora::kCapAlphaTest);

        static VertexProgram* blurvp =
            new VertexProgram(cMotionBlurredVP, cMotionBlurredVPDeclaration);

        GLRender::Enable(Aurora::kCapVertexProgram);
        RenderInterface::SetDepthMask(0);

        part->GetWorld(vLocalPosition, qLocalOrientation);

        Vector vCam = CurrentCamera->m_vPosition - vLocalPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
        vCam = RotateVector(vCam, -qLocalOrientation);
#else
        {
            float qw = qLocalOrientation.w;
            float qx = qLocalOrientation.x;
            float qy = qLocalOrientation.y;
            float qz = qLocalOrientation.z;
            float nw = -qw;
            if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
            {
                float t1 = qy * qy + qz * qz;
                float t2 = qx * qx + qz * qz;
                float t3 = qx * qy - qz * nw;
                float t4 = qx * qy + qz * nw;
                float t5 = qy * nw + qx * qz;
                float t6 = qx * qz - qy * nw;
                float t7 = qy * qz - qx * nw;
                float t8 = qx * nw + qy * qz;
                float t9 = qx * qx + qy * qy;
                float ncX = (1.0f - (t1 + t1)) * vCam.x + (t3 + t3) * vCam.y + (t5 + t5) * vCam.z;
                float ncY = (t4 + t4) * vCam.x + (1.0f - (t2 + t2)) * vCam.y + (t7 + t7) * vCam.z;
                float ncZ = (t6 + t6) * vCam.x + (t8 + t8) * vCam.y + (1.0f - (t9 + t9)) * vCam.z;
                vCam.x = ncX;
                vCam.y = ncY;
                vCam.z = ncZ;
            }
        }
#endif

        GLRender::SetVertexShaderConstant(16, currentscene->m_fFogStart, currentscene->m_fFogEnd,
                                          currentscene->m_bFogEnabled != 0 ? 1.0f : 0.0f, 0.5f);
        GLRender::SetVertexShaderConstant(17, 0.0f, 2.0f, 0.5f, 1.0f);

// The blur axis is the cross of the object's motion with the gob's local +Z.
        Vector vBlur = cross(vMotion, part->m_pGob->m_qOrientation.zdir());

#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
        vMotion = RotateVector(vMotion, -qLocalOrientation);
#else
        {
            float qw = qLocalOrientation.w;
            float qx = qLocalOrientation.x;
            float qy = qLocalOrientation.y;
            float qz = qLocalOrientation.z;
            float nw = -qw;
            if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
            {
                float t1 = qy * qy + qz * qz;
                float t2 = qx * qx + qz * qz;
                float t3 = qx * qy - qz * nw;
                float t4 = qx * qy + qz * nw;
                float t5 = qy * nw + qx * qz;
                float t6 = qx * qz - qy * nw;
                float t7 = qy * qz - qx * nw;
                float t8 = qx * nw + qy * qz;
                float t9 = qx * qx + qy * qy;
                float ncX = (1.0f - (t1 + t1)) * vMotion.x + (t3 + t3) * vMotion.y + (t5 + t5) * vMotion.z;
                float ncY = (t4 + t4) * vMotion.x + (1.0f - (t2 + t2)) * vMotion.y + (t7 + t7) * vMotion.z;
                float ncZ = (t6 + t6) * vMotion.x + (t8 + t8) * vMotion.y + (1.0f - (t9 + t9)) * vMotion.z;
                vMotion.x = ncX;
                vMotion.y = ncY;
                vMotion.z = ncZ;
            }
        }
#endif
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
        vBlur = RotateVector(vBlur, -qLocalOrientation);
#else
        {
            float qw = qLocalOrientation.w;
            float qx = qLocalOrientation.x;
            float qy = qLocalOrientation.y;
            float qz = qLocalOrientation.z;
            float nw = -qw;
            if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
            {
                float t1 = qy * qy + qz * qz;
                float t2 = qx * qx + qz * qz;
                float t3 = qx * qy - qz * nw;
                float t4 = qx * qy + qz * nw;
                float t5 = qy * nw + qx * qz;
                float t6 = qx * qz - qy * nw;
                float t7 = qy * qz - qx * nw;
                float t8 = qx * nw + qy * qz;
                float t9 = qx * qx + qy * qy;
                float ncX = (1.0f - (t1 + t1)) * vBlur.x + (t3 + t3) * vBlur.y + (t5 + t5) * vBlur.z;
                float ncY = (t4 + t4) * vBlur.x + (1.0f - (t2 + t2)) * vBlur.y + (t7 + t7) * vBlur.z;
                float ncZ = (t6 + t6) * vBlur.x + (t8 + t8) * vBlur.y + (1.0f - (t9 + t9)) * vBlur.z;
                vBlur.x = ncX;
                vBlur.y = ncY;
                vBlur.z = ncZ;
            }
        }
#endif

        float fDeltaT = DeltaT;
        Vector vDelta = part->m_pGob->m_vPosition - vLocalPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
        vDelta = RotateVector(vDelta, -qLocalOrientation);
#else
        {
            float qw = qLocalOrientation.w;
            float qx = qLocalOrientation.x;
            float qy = qLocalOrientation.y;
            float qz = qLocalOrientation.z;
            float nw = -qw;
            if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
            {
                float t1 = qy * qy + qz * qz;
                float t2 = qx * qx + qz * qz;
                float t3 = qx * qy - qz * nw;
                float t4 = qx * qy + qz * nw;
                float t5 = qy * nw + qx * qz;
                float t6 = qx * qz - qy * nw;
                float t7 = qy * qz - qx * nw;
                float t8 = qx * nw + qy * qz;
                float t9 = qx * qx + qy * qy;
                float ncX = (1.0f - (t1 + t1)) * vDelta.x + (t3 + t3) * vDelta.y + (t5 + t5) * vDelta.z;
                float ncY = (t4 + t4) * vDelta.x + (1.0f - (t2 + t2)) * vDelta.y + (t7 + t7) * vDelta.z;
                float ncZ = (t6 + t6) * vDelta.x + (t8 + t8) * vDelta.y + (1.0f - (t9 + t9)) * vDelta.z;
                vDelta.x = ncX;
                vDelta.y = ncY;
                vDelta.z = ncZ;
            }
        }
#endif

        vMotion = normalize(vMotion);

        GLRender::SetVertexShaderConstant(50, vMotion.x, vMotion.y, vMotion.z, 1.0f);
        GLRender::SetVertexShaderConstant(51, fMotion / (fDeltaT + fDeltaT), part->m_pGob->m_fMaxBlurLength,
                                          0.01f, 0.85f);
        GLRender::SetVertexShaderConstant(52, vBlur.x, vBlur.y, vBlur.z, 0.15f);
        GLRender::SetVertexShaderConstant(53, vDelta.x, vDelta.y, vDelta.z, 0.0f);
        GLRender::SetVertexShaderConstant(13, 0.0f, 0.0f, 0.0f, 0.0f);

        Vector vDiff = vDelta - vCam;
        float fDiffLen2 = vDiff.x * vDiff.x + vDiff.y * vDiff.y + vDiff.z * vDiff.z;
        float fDot = vDiff.x * vMotion.x + vDiff.y * vMotion.y + vDiff.z * vMotion.z;
        Vector vProj = vMotion * fDot;
        if (vProj.x * vProj.x + vProj.y * vProj.y + vProj.z * vProj.z >
            fDiffLen2 * 0.75f * 0.75f)
        {
            if (magnitude(vDiff + vMotion) > magnitude(vDiff - vMotion))
            {
                GLRender::SetVertexShaderConstant(13, 1.0f, 0.0f, 0.0f, 0.0f);
                GLRender::SetFrontFace(Aurora::kCullCW);
            }
        }

        GLRender::pfuncEnablePixelShaders(GLRender::kPSMotionBlur, part->m_pMaterial->Textures(0).m_pObject, 0, 0, 0);
        blurvp->Enable(part, true);
        RenderFlat(vertices);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
        if (bAlpha != 0)
            GLRender::Enable(Aurora::kCapAlphaTest);
        GLRender::SetFrontFace(Aurora::kCullCCW);
    }

    GLRender::Disable(Aurora::kCapVertexProgram);
}

void RenderShadowTriMesh(VertexPrimitiveFlat* vertices)
{
    K2_UNUSED(vertices);
}

void RenderFlatNonTextured(VertexPrimitiveFlat* vertices)
{
    if (!enablerender)
        return;

    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::Disable(Aurora::kCapTexture2D);
    RenderFlat(vertices);
    GLRender::Enable(Aurora::kCapTexture2D);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
}

#if KOTOR1
void DoEnvMapTransform(CAurTexture* pTexture, unsigned int nStage);
// K1: DoEnvMapTransform's pop half.
static inline void UndoEnvMapTransform(CAurTexture* pTexture, unsigned int nStage)
{
    if (AurCubeMapAvailable() && pTexture->IsCubeMap())
    {
        GLRender::SetActiveTexture(nStage);
        GLRender::SetMatrixMode(Aurora::kMatrixTexture);
        ASLgl::glPopMatrix();
        GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
        GLRender::SetActiveTexture(0);
    }
}
#endif

#if KOTOR1
unsigned short GetTextureBasicHash(CAurTexture* pTexture);
// AurTextureCalculateHashID("CM_M26mg") (the only K1 resource name with this hash): the one
// environment map that is drawn flat, without the environment pass.
enum
{
    kTextureHashCM_M26mg = 0xdcd3,
};
#endif
// Environment map plus lightmap (NV2x-class cards).
void RenderEMLM(VertexPrimitiveFlat* vertices)
{
#if KOTOR1  // K1: env map through the stage-2 texture matrix and different shader arguments
    if (!renderemlm)
        return;
    if (!enablerenderenv)
        return;
    if (!enablerender)
        return;
    if (!AurNV2xLevelShadersAvailable())
        return;

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    Gob* pGob = part->m_pGob;

    if (!enableenvmap || AurTextureEnvMapDisabled())
    {
        int nFormat = part->m_pMaterial->Textures(0).m_pObject->GetCompressionFormat();
        if (nFormat != 7 && nFormat != 8)
        {
            RenderFlat(vertices);
            return;
        }
    }

    float fAlpha = part->m_fOpacity;
    CAurTexture* pEnvMap = 0;
    if (pGob != 0)
    {
        pEnvMap = pGob->m_texWatch.m_pObject;
        if (pGob->m_nColorShifting != 0)
            fAlpha *= pGob->m_fColorShiftAmount;
    }
    if (pEnvMap == 0)
        pEnvMap = part->m_pMaterial->Textures(5).m_pObject;

    if (pEnvMap != 0 && GetTextureBasicHash(pEnvMap) == kTextureHashCM_M26mg)
    {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, part->m_pMaterial->Textures(0).m_pObject, 0, 0, 0);
        RenderFlat(vertices);
        return;
    }

    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::ActivateTextureStage(1);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);

    GLRender::pfuncEnablePixelShaders(GLRender::kPSEnvMapLightmap, part->m_pMaterial->Textures(0).m_pObject, pEnvMap,
                                      part->m_pMaterial->Textures(1).m_pObject, 0);
    DoEnvMapTransform(pEnvMap, 2);

    Vector vWhite(1.0f, 1.0f, 1.0f);
    GLRender::pfuncSetPixelShaderAmbientColor(vWhite, fAlpha);

    RenderFlat(vertices);
    UndoEnvMapTransform(pEnvMap, 2);

    GLRender::pfuncEnablePixelShaders(GLRender::kPSEnvMapLightmapOff, 0, 0, 0, 0);

    GLRender::ActivateTextureStage(1);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);
    GLRender::Enable(Aurora::kCapAlphaTest);

    GLRender::SetActiveTexture(1);
    GLRender::Disable(Aurora::kCapTextureCubeMap);

    GLRender::SetActiveTexture(0);
#else
    if (!renderemlm)
        return;
    if (!enablerenderenv)
        return;
    if (!enablerender)
        return;
    if (!AurNV2xLevelShadersAvailable())
        return;

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    Gob* pGob = part->m_pGob;

// Only a texture with compression format 7 or 8 can carry this pass.
    if (!enableenvmap || AurTextureEnvMapDisabled())
    {
        int nFormat = part->m_pMaterial->Textures(0).m_pObject->GetCompressionFormat();
        if (nFormat != 7 && nFormat != 8)
        {
            RenderFlat(vertices);
            return;
        }
    }

    float fAlpha = part->m_fOpacity;
    CAurTexture* pEnvMap = 0;
    if (pGob != 0)
    {
        pEnvMap = pGob->m_texWatch.m_pObject;
        if (pGob->m_nColorShifting != 0)
            fAlpha *= pGob->m_fColorShiftAmount;
    }
    if (pEnvMap == 0)
        pEnvMap = part->m_pMaterial->Textures(5).m_pObject;

    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::ActivateTextureStage(1);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);

    CAurTexture* pTexture0 = part->m_pMaterial->Textures(0).m_pObject;
    CAurTexture* pTexture1 = part->m_pMaterial->Textures(1).m_pObject;
    if (g_ForceSight)
    {
        SetupForceSightShader(pGob, GLRender::kPSFGlowEMLM, GLRender::kPSDesatEMLM, pTexture0, pTexture1, pEnvMap, 0, false);
    }
    else
    {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSEnvMapLightmap, pTexture0, pTexture1, pEnvMap, 0);
    }

    Vector vWhite(1.0f, 1.0f, 1.0f);
    if (g_ForceSight == 0)
        GLRender::pfuncSetPixelShaderAmbientColor(vWhite, fAlpha);

    RenderFlat(vertices);

    GLRender::pfuncEnablePixelShaders(g_ForceSight == 0 ? GLRender::kPSEnvMapLightmapOff : GLRender::kPSDesatOff, 0, 0, 0, 0);

    GLRender::ActivateTextureStage(1);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);
    GLRender::Enable(Aurora::kCapAlphaTest);

    GLRender::SetActiveTexture(1);
    GLRender::Disable(Aurora::kCapTextureCubeMap);

    GLRender::SetActiveTexture(0);
#endif
}

void RenderNV1xEMLM(VertexPrimitiveFlat* vertices)
{
    if (!AurNV1xLevelShadersAvailable())
        return;

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    if (part->m_pGob != 0)
        return;

    if (currentscene == 0 ||
        part->m_pMaterial->Textures(0).m_pObject->IsNull() ||
        part->m_pMaterial->Textures(1).m_pObject->IsNull() ||
        part->m_pMaterial->Textures(5).m_pObject->IsNull())
    {
        RenderFlat(vertices);
        return;
    }

    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapVertexProgram);

    GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);
    GLRender::ActivateTextureStage(1);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);

    CAurTexture* pTex0 = part->m_pMaterial->Textures(0).m_pObject;
    CAurTexture* pTex1 = part->m_pMaterial->Textures(1).m_pObject;
    GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmap, pTex0, pTex1, 0, 0);
    RenderFlat(vertices);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSMultiTextureOff, 0, 0, 0, 0);

    GLRender::ActivateTextureStage(1);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);
    GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendOne);

    CAurTexture* pTexB = part->m_pMaterial->Textures(0).m_pObject;
    CAurTexture* pEnvTex = part->m_pMaterial->Textures(5).m_pObject;
    GLRender::pfuncEnablePixelShaders(GLRender::kPSEnvMapLightmapNV1x, pTexB, pEnvTex, 0, 0);
#if KOTOR1  // K1: the env map goes through the stage-1 texture matrix
    DoEnvMapTransform(part->m_pMaterial->Textures(5).m_pObject, 1);
#endif

    Vector vWhite(1.0f, 1.0f, 1.0f);
    GLRender::pfuncSetPixelShaderAmbientColor(vWhite, 1.0f);

    RenderFlat(vertices);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSMultiTextureOff, 0, 0, 0, 0);

    GLRender::Disable(Aurora::kCapTexGenS);
    GLRender::Disable(Aurora::kCapTexGenT);
    GLRender::Disable(Aurora::kCapTexGenR);
#if KOTOR1
    UndoEnvMapTransform(part->m_pMaterial->Textures(5).m_pObject, 1);
#endif
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
}

void RenderLightMappedNoShaders(VertexPrimitiveFlat* vertices)
{
    if (!enablenoshaderlightmaps)
        return;

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    MdlNodeTriMesh* node = part->m_pNode->AsMdlNodeTriMesh();

    if (part->m_pMaterial->Textures(0).m_pObject->IsNull() ||
        part->m_pMaterial->Textures(1).m_pObject->IsNull() ||
        !currentscene)
    {
        RenderFlat(vertices);
        return;
    }

    if (part->m_pMaterial->HasEnvMap())
        GLRender::Disable(Aurora::kCapBlend);

    ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    GLRender::Disable(Aurora::kCapLighting);

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

    GLRender::SetActiveTexture(0);
    GLRender::Enable(Aurora::kCapTexture2D);
    part->m_pMaterial->BindTexture1();
    ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    GLRender::SetActiveTexture(1);
    GLRender::Enable(Aurora::kCapTexture2D);
    part->m_pMaterial->BindTexture0();
    ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    GLRender::SetActiveTexture(0);

    GLRender::ActivateTextureStage(1);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);

    GLRender::SetLightmapTexCoordToStage0(node->m_nVertexStride, node->m_uVertexFlags,
                                          node->m_pTree->AsModel()->m_nPool,
                                          static_cast<short>(node->m_nVertexCount) + 1,
                                          node, node->m_nGeomBase);
    RenderFlat(vertices);

    GLRender::SetDecalTexCoordToStage0(node->m_nVertexStride, node->m_uVertexFlags,
                                       node->m_pTree->AsModel()->m_nPool,
                                       static_cast<short>(node->m_nVertexCount) + 1,
                                       node, node->m_nGeomBase);

    GLRender::ActivateTextureStage(1);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);

    GLRender::SetActiveTexture(1);
    ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    GLRender::Disable(Aurora::kCapTexture2D);

    GLRender::SetActiveTexture(0);
    ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    part->m_pMaterial->BindTexture0();
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    ASLgl::glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    GLRender::Enable(Aurora::kCapLighting);

    if (part->m_pMaterial->HasEnvMap())
        GLRender::Enable(Aurora::kCapBlend);
}

void RenderLightMapped(VertexPrimitiveFlat* vertices)
{
    if (!enablerenderlightmapped)
        return;

    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    if (part->m_pMaterial->Textures(0).m_pObject->IsNull())
    {
        RenderFlat(vertices);
        return;
    }

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::ActivateTextureStage(1);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);

#if !KOTOR1  // K1: no force-sight arm
    if (g_ForceSight)
    {
        SetupForceSightShader(part->m_pGob, GLRender::kPSFGlowLM, GLRender::kPSDesatLM, part->m_pMaterial->Textures(0).m_pObject,
                              part->m_pMaterial->Textures(1).m_pObject, 0, 0, false);
        RenderFlat(vertices);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
    }
    else
#endif
    {
        CAurTexture* pTex0 = part->m_pMaterial->Textures(0).m_pObject;
        CAurTexture* pTex1 = part->m_pMaterial->Textures(1).m_pObject;
        GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmap, pTex0, pTex1, 0, 0);
        RenderFlat(vertices);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSMultiTextureOff, 0, 0, 0, 0);
    }

    GLRender::ActivateTextureStage(1);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);
}

void RenderFlatDecal(VertexPrimitiveFlat* vertices)
{
    if (!enablerender)
        return;

    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    vertices->m_pPart->m_pMaterial->BindTexture0();
    CAurTexture* pTexture = vertices->m_pPart->m_pMaterial->Textures(0).m_pObject;
    GLRender::pfuncEnablePixelShaders(GLRender::kPSDecal, pTexture, 0, 0, 0);

    GLRender::ActivateTextureStage(0);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    GLRender::EnableClientState(Aurora::kClientStateVertex);
    GLRender::DisableClientState(Aurora::kClientStateNormal);
    GLRender::DisableClientState(Aurora::kClientStateColor);
    RenderFlat(vertices);
    GLRender::EnableClientState(Aurora::kClientStateNormal);
    GLRender::EnableClientState(Aurora::kClientStateColor);
}

void RenderFlat(VertexPrimitiveFlat* vertices)
{
    if (!enablerender)
        return;

    if (!enablelighting)
    {
        ASLgl::glDisable(GL_LIGHTING);
        ASLgl::glDisableClientState(GL_COLOR_ARRAY);
        ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

#if !KOTOR1
    countparts++;
#endif

    MdlNodeTriMesh* node = static_cast<MdlNodeTriMesh*>(vertices->m_pNode);
    int nCount = node->m_lVertexIndicesCount.m_count;
    if (node->m_nTex0Ofs < 0)
        GLRender::DisableClientState(Aurora::kClientStateTexCoord);

    for (int i = 0; i < nCount; i++)
    {
        int nVertices = node->m_lVertexIndicesCount.m_pData[i];
        int nType = node->m_nPrimitiveType;
        if (nType == 3)
            nVertices = nVertices / 3 * 3;
        GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(nType), nVertices, 0,
                               node->m_lIndexPools.m_pData[i]);
#if !KOTOR1
        countfaces += nVertices / 3;
#endif
    }

    if (node->m_nPrimitiveType == 4 && node->m_nLeftOverIndexCount != 0)
        GLRender::DrawElements(static_cast<Aurora::AuroraPrimitiveTypes>(3), node->m_nLeftOverIndexCount,
                               node->m_nGeomBase, node->m_nLeftOverPool);

    if (node->m_nTex0Ofs < 0)
        GLRender::EnableClientState(Aurora::kClientStateTexCoord);

    if (!enablelighting)
    {
        ASLgl::glEnable(GL_LIGHTING);
        ASLgl::glEnableClientState(GL_COLOR_ARRAY);
    }
}

void RenderBumpyShiny(VertexPrimitiveFlat* vertices)
{
    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();

// Built before the attrib pair, because the tail below re-enables it.
    static VertexProgram* standardvp = new VertexProgram(
        usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP, cStandardVPDeclaration);

    ASLgl::glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    GLRender::Enable(Aurora::kCapBlend);

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);

    GLRender::EnableClientState(Aurora::kClientStateAttrib12);
    GLRender::EnableClientState(Aurora::kClientStateAttrib13);
    GLRender::EnableClientState(Aurora::kClientStateAttrib14);
    GLRender::DisableClientState(Aurora::kClientStateNormal);

    static VertexProgram* bumpyshinyvp =
        new VertexProgram(cBumpyShinyVP, cBumpyShinyRegularVPDeclaration);

    GLRender::SetVertexShaderConstant(15, 1.0f, 1.0f, 1.0f, 1.0f);

    bumpyshinyvp->Enable(part, false);
    GLRender::Enable(Aurora::kCapVertexProgram);

    CAurTexture* pTex4 = part->m_pMaterial->Textures(4).m_pObject;
    CAurTexture* pTex5 = part->m_pMaterial->Textures(5).m_pObject;
    GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShinyTextures, pTex4, 0, 0, pTex5);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShiny, 0, 0, 0, 0);
    RenderFlat(vertices);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShinyOff, 0, 0, 0, 0);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
    GLRender::Disable(Aurora::kCapVertexProgram);

    GLRender::DisableClientState(Aurora::kClientStateAttrib12);
    GLRender::DisableClientState(Aurora::kClientStateAttrib13);
    GLRender::DisableClientState(Aurora::kClientStateAttrib14);
    GLRender::EnableClientState(Aurora::kClientStateNormal);
    ASLgl::glPopAttrib();

    if (enabledecal != 0)
    {
        part->m_pMaterial->BindTexture0();
        GLRender::Enable(Aurora::kCapBlend);
        if (enableold == 0)
            GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendInvSrcAlpha);
        else
            GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        standardvp->Enable(part, true);
        GLRender::Enable(Aurora::kCapVertexProgram);
        GLRender::SetVertexShaderConstant(15, 0.0f, 1.0f, 0.0f, shinyalpha);
        RenderFlat(vertices);
        GLRender::BindVertexProgram(0);
        GLRender::Disable(Aurora::kCapVertexProgram);
    }

    GLRender::SetBlendFunc(Aurora::kBlendZero, Aurora::kBlendSrcColor);
    standardvp->Enable(part, true);
    GLRender::Enable(Aurora::kCapVertexProgram);

    if (enablerender != 0)
    {
        GLRender::DisableClientState(Aurora::kClientStateTexCoord);
        GLRender::Disable(Aurora::kCapTexture2D);
        RenderFlat(vertices);
        GLRender::Enable(Aurora::kCapTexture2D);
        GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    }

    GLRender::Disable(Aurora::kCapVertexProgram);
    GLRender::Enable(Aurora::kCapAlphaTest);
}

VertexProgram* g_pBumpyShinyPass1;
extern VertexProgram* g_pBumpyShinyPass2;
extern VertexProgram* g_pBumpyShinyPass3;
extern VertexProgram* g_pBumpyShinyPass4;
extern VertexProgram* g_pBumpyShinyPass5;

int enablerenderembm = 1;
extern int enablelightmap;
int enablebsenvmap = 1;
int overridestuff;
int AurTextureGetTID(CAurTexture* pTexture);
namespace ASL { bool GetPrefAsBool(const char* pszKey, bool bDefault); }

void RenderEMBM(VertexPrimitiveFlat* vertices)
{
#if KOTOR1  // K1: no EMBM pref, fog pass, attrib push, ARB attribute arrays, force sight or illumination
    if (!enablerenderembm)
        return;

    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();

    GLRender::Enable(Aurora::kCapBlend);
    GLRender::Disable(Aurora::kCapAlphaTest);

    int nLightmapped = 0;
    if (static_cast<MdlNodeTriMesh*>(vertices->m_pNode)->m_bHasLightmap && enablelightmap != 0)
    {
        CAurTexture* pLightmap = part->m_pMaterial->Textures(1).m_pObject;
        nLightmapped = (pLightmap != 0) ? (AurTextureGetTID(pLightmap) != 0) : 1;
    }

    GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);

    if (part->m_pMaterial->m_fWaterAlpha == 1.0f)
    {
        if (!part->m_pMaterial->Textures(0).m_pObject->IsNull() && nLightmapped == 1)
        {
            if (enabledecal != 0)
            {
                GLRender::ActivateTextureStage(1);
                GLRender::EnableClientState(Aurora::kClientStateTexCoord);
                GLRender::ActivateTextureStage(0);

                GLRender::pfuncEnablePixelShaders(GLRender::kPSBaseLightmap, part->m_pMaterial->Textures(0).m_pObject,
                                                  part->m_pMaterial->Textures(5).m_pObject,
                                                  part->m_pMaterial->Textures(1).m_pObject, 0);
                GLRender::Enable(Aurora::kCapVertexProgram);
                g_pBumpyShinyPass1->Enable(part, true);
                GLRender::SetVertexShaderConstant(15, 1.0f, 1.0f, 1.0f, 0.0f);
                RenderFlat(vertices);

                GLRender::ActivateTextureStage(1);
                GLRender::DisableClientState(Aurora::kClientStateTexCoord);
                GLRender::ActivateTextureStage(0);
            }
        }
        else if (enabledecal != 0)
        {
            g_pBumpyShinyPass3->Enable(part, true);
            GLRender::Enable(Aurora::kCapVertexProgram);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, part->m_pMaterial->Textures(0).m_pObject, 0, 0, 0);
            RenderFlat(vertices);
        }
#ifndef K2_QOL_BUMPMAPS
        return;
#endif
    }

    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapVertexProgram);

    if (enablebsenvmap != 0)
    {
        GLRender::EnableClientState(Aurora::kClientStateAttrib12);
        GLRender::EnableClientState(Aurora::kClientStateAttrib13);
        GLRender::EnableClientState(Aurora::kClientStateAttrib14);
        GLRender::DisableClientState(Aurora::kClientStateNormal);

        VertexProgram* pVP = (nLightmapped != 0) ? g_pBumpyShinyPass5 : g_pBumpyShinyPass4;
        pVP->Enable(part, false);
        GLRender::SetVertexShaderConstant(15, 1.0f, 0.0f, 1.0f, 0.0f);

        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShinyTextures, part->m_pMaterial->Textures(4).m_pObject, 0, 0,
                                          part->m_pMaterial->Textures(5).m_pObject);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShiny, 0, 0, 0, 0);

        if (part->m_pMaterial->m_fWaterAlpha < 1.0f)
        {
            GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
            Vector vAmbient(1.0f, 1.0f, 1.0f);
            GLRender::pfuncSetPixelShaderAmbientColor(vAmbient, part->m_pMaterial->m_fWaterAlpha);
        }
        else
        {
            GLRender::SetBlendFunc(Aurora::kBlendInvDstAlpha, Aurora::kBlendOne);
        }
        if (overridestuff != 0)
            GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);

        RenderFlat(vertices);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShinyOff, 0, 0, 0, 0);

        GLRender::DisableClientState(Aurora::kClientStateAttrib12);
        GLRender::DisableClientState(Aurora::kClientStateAttrib13);
        GLRender::DisableClientState(Aurora::kClientStateAttrib14);
        GLRender::EnableClientState(Aurora::kClientStateNormal);
        GLRender::SetVertexShaderConstant(15, 1.0f, 1.0f, 1.0f, 0.0f);
    }

    GLRender::SetVertexShaderConstant(15, 1.0f, 1.0f, 1.0f, 0.0f);
    GLRender::Disable(Aurora::kCapVertexProgram);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
#else
    static bool bEnableEMBM = ASL::GetPrefAsBool("EnableEMBM", true);
    if (!bEnableEMBM)
        return;

    if (!enablerenderembm)
        return;

    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    K2_ASSUME(part != 0);   // the primitive's owning trimesh: every primitive is built with it
    MdlNodeTriMesh* node = static_cast<MdlNodeTriMesh*>(part->m_pNode);
    Gob* pGob = (part != 0) ? part->m_pGob : 0;

    GLRender::Enable(Aurora::kCapBlend);
    GLRender::Disable(Aurora::kCapAlphaTest);

    int nLightmapped = 0;
    if (static_cast<MdlNodeTriMesh*>(vertices->m_pNode)->m_bHasLightmap && enablelightmap != 0)
    {
        CAurTexture* pLightmap = part->m_pMaterial->Textures(1).m_pObject;
// A null name cell counts as lit.
        nLightmapped = (pLightmap != 0) ? (AurTextureGetTID(pLightmap) != 0) : 1;
    }

    int bPass2 = 0;
    if (currentscene->m_bFogEnabled != 0 &&
        node->m_pTree->AsModel()->m_cAffectedByFog != 0)
    {
        GLRender::Disable(Aurora::kCapFog);
        bPass2 = 1;
    }

    GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);

    if (part->m_pMaterial->m_fWaterAlpha == 1.0f)
    {
        if (!part->m_pMaterial->Textures(0).m_pObject->IsNull() && nLightmapped == 1)
        {
            if (enabledecal != 0)
            {
                GLRender::ActivateTextureStage(1);
                GLRender::EnableClientState(Aurora::kClientStateTexCoord);
                GLRender::ActivateTextureStage(0);

                CAurTexture* pTex0 = part->m_pMaterial->Textures(0).m_pObject;
                CAurTexture* pTex1 = part->m_pMaterial->Textures(1).m_pObject;
                if (g_ForceSight != 0)
                {
                    SetupForceSightShader(pGob, GLRender::kPSFGlowLM, GLRender::kPSDesatLM, pTex0, pTex1, 0, 0, false);
                }
                else
                {
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmap, pTex0, pTex1, 0, 0);
                }

                GLRender::Enable(Aurora::kCapVertexProgram);
                g_pBumpyShinyPass1->Enable(part, true);
                GLRender::SetVertexShaderConstant(15, 1.0f, 1.0f, 1.0f, 0.0f);
                RenderFlat(vertices);
                GLRender::pfuncEnablePixelShaders(GLRender::kPSMultiTextureOff, 0, 0, 0, 0);

                GLRender::ActivateTextureStage(1);
                GLRender::DisableClientState(Aurora::kClientStateTexCoord);
                GLRender::ActivateTextureStage(0);
            }
        }
        else if (enabledecal != 0)
        {
            g_pBumpyShinyPass3->Enable(part, true);
            GLRender::Enable(Aurora::kCapVertexProgram);

            if (g_ForceSight != 0)
            {
                SetupForceSightShader(pGob, GLRender::kPSFGlow, GLRender::kPSDesat, 0, 0, 0, 0, false);
            }
            else
            {
                if (pGob != 0 && pGob->IsIlluminationEnable() && AurNV1xLevelShadersAvailable())
                {
                    GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
                    pGob->GetIlluminationColor(color.r, color.g, color.b);
                    GLRender::m_PerInstanceIllumination = color;
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSIllum, 0, 0, 0, 0);
                }
                else
                {
#ifdef K2_QOL_BUMPMAPS
                    // The PC's fixed-function texture-times-light, which only a program draws here.
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, part->m_pMaterial->Textures(0).m_pObject,
                                                      0, 0, 0);
#else
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
#endif
                }
// Only the non-force-sight path rebinds texture 0.
                part->m_pMaterial->BindTexture0();
            }

            RenderFlat(vertices);

            if (g_ForceSight != 0)
            {
                GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
                GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
                RenderInterface::SetDepthMask(1);
            }
            else if (pGob != 0 && pGob->IsIlluminationEnable())
            {
                GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
            }
        }
    }

    GLRender::Disable(Aurora::kCapAlphaTest);
    ASLgl::glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    GLRender::Enable(Aurora::kCapVertexProgram);

    if (enablebsenvmap != 0)
    {
        if (usearbvertexprograms != 0)
        {
            ASLgl::glEnableVertexAttribArrayARB(12);
            ASLgl::glEnableVertexAttribArrayARB(13);
            ASLgl::glEnableVertexAttribArrayARB(14);
        }
        else
        {
            GLRender::EnableClientState(Aurora::kClientStateAttrib12);
            GLRender::EnableClientState(Aurora::kClientStateAttrib13);
            GLRender::EnableClientState(Aurora::kClientStateAttrib14);
        }
        GLRender::DisableClientState(Aurora::kClientStateNormal);

        VertexProgram* pVP = (nLightmapped != 0) ? g_pBumpyShinyPass5 : g_pBumpyShinyPass4;
        pVP->Enable(part, false);
        GLRender::SetVertexShaderConstant(15, 1.0f, 0.0f, 1.0f, 0.0f);

        CAurTexture* pTex4 = part->m_pMaterial->Textures(4).m_pObject;
        CAurTexture* pTex5 = part->m_pMaterial->Textures(5).m_pObject;
        if (g_ForceSight != 0)
        {
            SetupForceSightShader(pGob, GLRender::kPSFGlowBumpyShiny, GLRender::kPSDesatBumpyShiny, pTex4, 0, 0, pTex5, true);
        }
        else
        {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShinyTextures, pTex4, 0, 0, pTex5);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShiny, 0, 0, 0, 0);
        }

        if (part->m_pMaterial->m_fWaterAlpha < 1.0f)
        {
            GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
            if (g_ForceSight == 0)
            {
                Vector vAmbient(1.0f, 1.0f, 1.0f);
                GLRender::pfuncSetPixelShaderAmbientColor(vAmbient,
                                                          part->m_pMaterial->m_fWaterAlpha);
            }
        }
        else
        {
            GLRender::SetBlendFunc(Aurora::kBlendInvDstAlpha, Aurora::kBlendOne);
        }
        if (overridestuff != 0)
            GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);

        RenderFlat(vertices);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShinyOff, 0, 0, 0, 0);

        if (usearbvertexprograms != 0)
        {
            ASLgl::glDisableVertexAttribArrayARB(12);
            ASLgl::glDisableVertexAttribArrayARB(13);
            ASLgl::glDisableVertexAttribArrayARB(14);
        }
        else
        {
            GLRender::DisableClientState(Aurora::kClientStateAttrib12);
            GLRender::DisableClientState(Aurora::kClientStateAttrib13);
            GLRender::DisableClientState(Aurora::kClientStateAttrib14);
        }
        GLRender::EnableClientState(Aurora::kClientStateNormal);
        GLRender::SetVertexShaderConstant(15, 1.0f, 1.0f, 1.0f, 0.0f);
    }

    if (bPass2 != 0)
    {
        GLRender::Enable(Aurora::kCapFog);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSFog, part->m_pMaterial->Textures(0).m_pObject, 0, 0, 0);
        GLRender::Enable(Aurora::kCapVertexProgram);
        g_pBumpyShinyPass2->Enable(part, false);
        RenderFlat(vertices);
    }

    ASLgl::glPopAttrib();
    GLRender::SetVertexShaderConstant(15, 1.0f, 1.0f, 1.0f, 0.0f);
    GLRender::Disable(Aurora::kCapVertexProgram);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
#endif
}

extern int enablelightmap;
extern char cStandardDirtyVP[];
extern char cStandardDirtyLMVP[];
extern char cStandardDirtyEMVP[];
extern char cStandardDirtyEMLMVP[];
extern char cStandardDirtyLSVP[];
extern char cStandardDirtyLSLMVP[];
extern char cStandardDirtyLSEMVP[];
extern char cStandardDirtyLSEMLMVP[];
extern char szcStandardDirtyVPARB[];
extern char szcStandardDirtyLMVPARB[];
extern char szcStandardDirtyEMVPARB[];
extern char szcStandardDirtyEMLMVPARB[];
extern char szcStandardDirtyLSVPARB[];
extern char szcStandardDirtyLSLMVPARB[];
extern char szcStandardDirtyLSEMVPARB[];
extern char szszcStandardDirtyLSEMLMVPARB[];
extern unsigned int* cStandardVP2Declaration;

// The node's dirt_texture is a 1-based index into GLRender::m_pDirtyTexture.

#if !KOTOR1  // K2 only
// Renders the dirt pass with eight pixel-shader variants, one per dirt texture and space.
#ifdef K2_QOL_BUMPMAPS
extern bool g_bQolNoiseLocal;
#endif

void RenderNoiseMapped(VertexPrimitiveFlat* vertices, int nDirtTexture, int nDirtWorldspace)
{
    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    Gob* pGob = part->m_pGob;
#ifdef K2_QOL_BUMPMAPS
    // Which vertex program below the shim's noise program stands for: world or model space.
    g_bQolNoiseLocal = nDirtWorldspace != 1;
#endif

    int nFormula = GLRender::GetNoiseFormula(nDirtTexture);

    if ((pGob != 0 && pGob->m_texWatch.m_pObject != 0) || part->m_pMaterial->HasEnvMap())
    {
        MdlNodeTriMesh* node = part->m_pNode->AsMdlNodeTriMesh();

        if (node->m_bHasLightmap && enablelightmap != 0)
        {
// Arm A: env map + light map.
            static VertexProgram* emlmvp =
                new VertexProgram(usearbvertexprograms != 0 ? szcStandardDirtyEMLMVPARB
                                                             : cStandardDirtyEMLMVP,
                                  cStandardVP2Declaration);
            static VertexProgram* lsemlmvp =
                new VertexProgram(usearbvertexprograms != 0 ? szszcStandardDirtyLSEMLMVPARB
                                                             : cStandardDirtyLSEMLMVP,
                                  cStandardVP2Declaration);
            VertexProgram* pVP = (nDirtWorldspace == 1) ? emlmvp : lsemlmvp;

            GLRender::Enable(Aurora::kCapVertexProgram);
            pVP->Enable(part, true);
            GLRender::ActivateTextureStage(1);
            GLRender::EnableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);

            GLRender::SetVertexShaderConstant(15, 1.0f, 0.0f, 1.0f, 0.0f);
            GLRender::SetVertexShaderConstant(16, 0.0f, 2.0f, 3.0f, 1.0f);

            if (g_ForceSight != 0)
            {
                SetupForceSightShader(pGob, nFormula + GLRender::kPSDesatNoiseEMLMSubtract, nFormula + GLRender::kPSDesatNoiseEMLMSubtract,
                                      part->m_pMaterial->Textures(0).m_pObject,
                                      part->m_pMaterial->Textures(1).m_pObject,
                                      part->m_pMaterial->Textures(5).m_pObject,
                                      GLRender::m_pDirtyTexture[nDirtTexture], false);
            }
            else if (pGob != 0 && pGob->IsIlluminationEnable())
            {
                GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
                pGob->GetIlluminationColor(color.r, color.g, color.b);
                GLRender::m_PerInstanceIllumination = color;
                GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSIllumNoiseEMLMSubtract,
                                                  part->m_pMaterial->Textures(0).m_pObject,
                                                  part->m_pMaterial->Textures(1).m_pObject,
                                                  part->m_pMaterial->Textures(5).m_pObject,
                                                  GLRender::m_pDirtyTexture[nDirtTexture]);
            }
            else
            {
                GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSNoiseEMLMSubtract,
                                                  part->m_pMaterial->Textures(0).m_pObject,
                                                  part->m_pMaterial->Textures(1).m_pObject,
                                                  part->m_pMaterial->Textures(5).m_pObject,
                                                  GLRender::m_pDirtyTexture[nDirtTexture]);
            }
        }
        else
        {
// Arm B: env map only.
            static VertexProgram* emvp =
                new VertexProgram(usearbvertexprograms != 0 ? szcStandardDirtyEMVPARB
                                                             : cStandardDirtyEMVP,
                                  cStandardVP2Declaration);
            static VertexProgram* lsemvp =
                new VertexProgram(usearbvertexprograms != 0 ? szcStandardDirtyLSEMVPARB
                                                             : cStandardDirtyLSEMVP,
                                  cStandardVP2Declaration);
            VertexProgram* pVP = (nDirtWorldspace == 1) ? emvp : lsemvp;

            GLRender::Enable(Aurora::kCapVertexProgram);
            pVP->Enable(part, true);

            GLRender::SetVertexShaderConstant(15, 1.0f, 0.0f, 1.0f, 0.0f);
            GLRender::SetVertexShaderConstant(16, 0.0f, 2.0f, 3.0f, 1.0f);

            if (pGob != 0 && pGob->m_texWatch.m_pObject != 0)
            {
                if (g_ForceSight != 0)
                {
                    SetupForceSightShader(pGob, nFormula + GLRender::kPSDesatNoiseEMSubtract, nFormula + GLRender::kPSDesatNoiseEMSubtract,
                                          part->m_pMaterial->Textures(0).m_pObject,
                                          pGob->m_texWatch.m_pObject,
                                          GLRender::m_pDirtyTexture[nDirtTexture], 0, false);
                }
                else if (pGob->IsIlluminationEnable())
                {
                    GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
                    pGob->GetIlluminationColor(color.r, color.g, color.b);
                    GLRender::m_PerInstanceIllumination = color;
                    GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSIllumNoiseEMSubtract,
                                                      part->m_pMaterial->Textures(0).m_pObject,
                                                      pGob->m_texWatch.m_pObject,
                                                      GLRender::m_pDirtyTexture[nDirtTexture], 0);
                }
                else
                {
                    GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSNoiseEMSubtract,
                                                      part->m_pMaterial->Textures(0).m_pObject,
                                                      pGob->m_texWatch.m_pObject,
                                                      GLRender::m_pDirtyTexture[nDirtTexture], 0);
                }
            }
            else
            {
                if (g_ForceSight != 0)
                {
                    SetupForceSightShader(pGob, nFormula + GLRender::kPSDesatNoiseEMSubtract, nFormula + GLRender::kPSDesatNoiseEMSubtract,
                                          part->m_pMaterial->Textures(0).m_pObject,
                                          part->m_pMaterial->Textures(5).m_pObject,
                                          GLRender::m_pDirtyTexture[nDirtTexture], 0, false);
                }
                else if (pGob != 0 && pGob->IsIlluminationEnable())
                {
                    GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
                    pGob->GetIlluminationColor(color.r, color.g, color.b);
                    GLRender::m_PerInstanceIllumination = color;
                    GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSIllumNoiseEMSubtract,
                                                      part->m_pMaterial->Textures(0).m_pObject,
                                                      part->m_pMaterial->Textures(5).m_pObject,
                                                      GLRender::m_pDirtyTexture[nDirtTexture], 0);
                }
                else
                {
                    GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSNoiseEMSubtract,
                                                      part->m_pMaterial->Textures(0).m_pObject,
                                                      part->m_pMaterial->Textures(5).m_pObject,
                                                      GLRender::m_pDirtyTexture[nDirtTexture], 0);
                }
            }
        }
    }
    else
    {
        MdlNodeTriMesh* node = part->m_pNode->AsMdlNodeTriMesh();

        if (node->m_bHasLightmap && enablelightmap != 0)
        {
// Arm C: light map only.
            static VertexProgram* lmvp =
                new VertexProgram(usearbvertexprograms != 0 ? szcStandardDirtyLMVPARB
                                                             : cStandardDirtyLMVP,
                                  cStandardVP2Declaration);
            static VertexProgram* lslmvp =
                new VertexProgram(usearbvertexprograms != 0 ? szcStandardDirtyLSLMVPARB
                                                             : cStandardDirtyLSLMVP,
                                  cStandardVP2Declaration);
            VertexProgram* pVP = (nDirtWorldspace == 1) ? lmvp : lslmvp;

            GLRender::Enable(Aurora::kCapVertexProgram);
            pVP->Enable(part, true);
            GLRender::ActivateTextureStage(1);
            GLRender::EnableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);

            GLRender::SetVertexShaderConstant(15, 1.0f, 0.0f, 1.0f, 0.0f);
            GLRender::SetVertexShaderConstant(16, 0.0f, 2.0f, 3.0f, 1.0f);

            if (g_ForceSight != 0)
            {
                SetupForceSightShader(pGob, nFormula + GLRender::kPSDesatNoiseLMSubtract, nFormula + GLRender::kPSDesatNoiseLMSubtract,
                                      part->m_pMaterial->Textures(0).m_pObject,
                                      part->m_pMaterial->Textures(1).m_pObject,
                                      GLRender::m_pDirtyTexture[nDirtTexture], 0, false);
            }
            else if (pGob != 0 && pGob->IsIlluminationEnable())
            {
                GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
                pGob->GetIlluminationColor(color.r, color.g, color.b);
                GLRender::m_PerInstanceIllumination = color;
                GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSIllumNoiseLMSubtract,
                                                  part->m_pMaterial->Textures(0).m_pObject,
                                                  part->m_pMaterial->Textures(1).m_pObject,
                                                  GLRender::m_pDirtyTexture[nDirtTexture], 0);
            }
            else
            {
                GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSNoiseLMSubtract,
                                                  part->m_pMaterial->Textures(0).m_pObject,
                                                  part->m_pMaterial->Textures(1).m_pObject,
                                                  GLRender::m_pDirtyTexture[nDirtTexture], 0);
            }
        }
        else
        {
// Arm D: neither.
            static VertexProgram* plainvp =
                new VertexProgram(usearbvertexprograms != 0 ? szcStandardDirtyVPARB
                                                             : cStandardDirtyVP,
                                  cStandardVPDeclaration);
            static VertexProgram* plainlsvp =
                new VertexProgram(usearbvertexprograms != 0 ? szcStandardDirtyLSVPARB
                                                             : cStandardDirtyLSVP,
                                  cStandardVPDeclaration);
            VertexProgram* pVP = (nDirtWorldspace == 1) ? plainvp : plainlsvp;

            GLRender::Enable(Aurora::kCapVertexProgram);
            pVP->Enable(part, true);

            GLRender::SetVertexShaderConstant(15, 1.0f, 0.0f, 1.0f, 0.0f);
            GLRender::SetVertexShaderConstant(16, 0.0f, 2.0f, 3.0f, 1.0f);

            if (g_ForceSight != 0)
            {
                SetupForceSightShader(pGob, nFormula + GLRender::kPSDesatNoiseSubtract, nFormula + GLRender::kPSDesatNoiseSubtract,
                                      part->m_pMaterial->Textures(0).m_pObject,
                                      GLRender::m_pDirtyTexture[nDirtTexture], 0, 0, false);
            }
            else if (pGob != 0 && pGob->IsIlluminationEnable())
            {
                GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
                pGob->GetIlluminationColor(color.r, color.g, color.b);
                GLRender::m_PerInstanceIllumination = color;
                GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSIllumNoiseSubtract,
                                                  part->m_pMaterial->Textures(0).m_pObject,
                                                  GLRender::m_pDirtyTexture[nDirtTexture], 0, 0);
            }
            else
            {
                GLRender::pfuncEnablePixelShaders(nFormula + GLRender::kPSNoiseSubtract,
                                                  part->m_pMaterial->Textures(0).m_pObject,
                                                  GLRender::m_pDirtyTexture[nDirtTexture], 0, 0);
            }
        }
    }

// The shared epilogue puts every unit back to how it was.
    GLRender::SetVertexShaderConstant(28, 0.5f, 0.25f, 0.125f, 0.0675f);
    RenderFlat(vertices);
    GLRender::Disable(Aurora::kCapVertexProgram);
    GLRender::ActivateTextureStage(1);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);
    GLRender::pfuncEnablePixelShaders(g_ForceSight == 0 ? GLRender::kPSMultiTextureOff : GLRender::kPSDesatOff, 0, 0, 0, 0);
}
#endif


// Renders environment maps through the one-pass pixel-shader path, else the two-pass texture-gen
// path.
void RenderEnvironmentMapped(VertexPrimitiveFlat* vertices, bool bParam2, bool bParam3)
{
    K2_UNUSED(bParam2);
#if KOTOR1  // K1: cube maps through the camera texture matrix; no force sight or illumination
    if (!enablerenderenv || !enablerender)
        return;

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    Gob* pGob = part->m_pGob;

    if (!enableenvmap || AurTextureEnvMapDisabled())
    {
        int nFormat = part->m_pMaterial->Textures(0).m_pObject->GetCompressionFormat();
        if (nFormat != 7 && nFormat != 8)
        {
            RenderFlat(vertices);
            return;
        }
    }

    float fAlpha = part->m_fOpacity;
    CAurTexture* pEnvMap = 0;
    if (pGob != 0)
    {
        pEnvMap = pGob->m_texWatch.m_pObject;
        if (pGob->m_nColorShifting != 0)
            fAlpha *= pGob->m_fColorShiftAmount;
    }
    if (pEnvMap == 0)
        pEnvMap = part->m_pMaterial->Textures(5).m_pObject;

    if (enableonepassenvmap && AurOnePassEnvMapAvailable())
    {
        GLRender::Disable(Aurora::kCapAlphaTest);

        if (bParam3 && AurATIValidate())
            GLRender::pfuncEnablePixelShaders(GLRender::kPSEnvMapFog, part->m_pMaterial->Textures(0).m_pObject,
                                              pEnvMap, 0, 0);
        else
            GLRender::pfuncEnablePixelShaders(GLRender::kPSEnvMapCube, part->m_pMaterial->Textures(0).m_pObject,
                                              pEnvMap, 0, 0);
        DoEnvMapTransform(pEnvMap, 1);

        Vector vWhite(1.0f, 1.0f, 1.0f);
        GLRender::pfuncSetPixelShaderAmbientColor(vWhite, fAlpha);

        RenderFlat(vertices);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSMultiTextureOff, 0, 0, 0, 0);
        UndoEnvMapTransform(pEnvMap, 1);

        GLRender::SetActiveTexture(1);
        if (AurCubeMapAvailable())
            GLRender::Disable(Aurora::kCapTextureCubeMap);

        GLRender::SetActiveTexture(0);

        GLRender::Enable(Aurora::kCapAlphaTest);
        return;
    }

    GLRender::Disable(Aurora::kCapAlphaTest);

    if (environmentmapdest != 0)
    {
        GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);
        RenderFlat(vertices);
        GLRender::SetBlendFunc(Aurora::kBlendInvDstAlpha, Aurora::kBlendOne);
        DoEnvMapTransform(pEnvMap, 0);
        pEnvMap->Bind();

        if (pEnvMap->IsCubeMap())
        {
            GLRender::Enable(Aurora::kCapTexGenS);
            GLRender::Enable(Aurora::kCapTexGenT);
            GLRender::Enable(Aurora::kCapTexGenR);
        }
        else
        {
            GLRender::SetTextureCoordGenerationState(Aurora::kTexCoordGenSphereMap);
            GLRender::Enable(Aurora::kCapTexGenS);
            GLRender::Enable(Aurora::kCapTexGenT);
        }
        UndoEnvMapTransform(pEnvMap, 0);
        RenderFlat(vertices);

        if (part->m_pGob == 0)
            part->m_pMaterial->BindTexture0();
    }
    else
    {
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        DoEnvMapTransform(pEnvMap, 0);
        pEnvMap->Bind();

        if (pEnvMap->IsCubeMap())
        {
            GLRender::Enable(Aurora::kCapTexGenS);
            GLRender::Enable(Aurora::kCapTexGenT);
            GLRender::Enable(Aurora::kCapTexGenR);
        }
        else
        {
            GLRender::SetTextureCoordGenerationState(Aurora::kTexCoordGenSphereMap);
            GLRender::Enable(Aurora::kCapTexGenS);
            GLRender::Enable(Aurora::kCapTexGenT);
        }
        RenderFlat(vertices);
        UndoEnvMapTransform(pEnvMap, 0);

        GLRender::Disable(Aurora::kCapTexGenS);
        GLRender::Disable(Aurora::kCapTexGenT);
        GLRender::Disable(Aurora::kCapTexGenR);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        part->m_pMaterial->BindTexture0();
        RenderFlat(vertices);
    }

    if (AurCubeMapAvailable())
        GLRender::BindTexture(Aurora::kCapTextureCubeMap, 0);

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::Enable(Aurora::kCapAlphaTest);

    GLRender::SetActiveTexture(1);
    GLRender::Disable(Aurora::kCapTexGenS);
    GLRender::Disable(Aurora::kCapTexGenT);
    GLRender::Disable(Aurora::kCapTexGenR);
    if (AurCubeMapAvailable())
        GLRender::Disable(Aurora::kCapTextureCubeMap);

    GLRender::SetActiveTexture(0);
#else
    if (!enablerenderenv || !enablerender)
        return;

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    Gob* pGob = part->m_pGob;

    if (!enableenvmap || AurTextureEnvMapDisabled())
    {
        int nFormat = part->m_pMaterial->Textures(0).m_pObject->GetCompressionFormat();
        if (nFormat != 7 && nFormat != 8)
        {
            RenderFlat(vertices);
            return;
        }
    }

    float fAlpha = part->m_fOpacity;
    CAurTexture* pEnvMap = 0;
    if (pGob != 0)
    {
        pEnvMap = pGob->m_texWatch.m_pObject;
        if (pGob->m_nColorShifting != 0)
            fAlpha *= pGob->m_fColorShiftAmount;
    }
    if (pEnvMap == 0)
        pEnvMap = part->m_pMaterial->Textures(5).m_pObject;

    if (enableonepassenvmap && AurOnePassEnvMapAvailable())
    {
        GLRender::Disable(Aurora::kCapAlphaTest);

        if (bParam3 && AurATIValidate())
        {
            if (g_ForceSight != 0)
            {
                SetupForceSightShader(pGob, GLRender::kPSFGlowEM, GLRender::kPSDesatEM, part->m_pMaterial->Textures(0).m_pObject,
                                      pEnvMap, 0, 0, false);
            }
            else if (pGob != 0 && pGob->IsIlluminationEnable())
            {
                GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
                pGob->GetIlluminationColor(color.r, color.g, color.b);
                GLRender::m_PerInstanceIllumination = color;
                GLRender::pfuncEnablePixelShaders(GLRender::kPSIllumEM, part->m_pMaterial->Textures(0).m_pObject,
                                                  pEnvMap, 0, 0);
            }
            else
            {
                GLRender::pfuncEnablePixelShaders(GLRender::kPSEnvMapFog, part->m_pMaterial->Textures(0).m_pObject,
                                                  pEnvMap, 0, 0);
            }
        }
        else if (g_ForceSight != 0)
        {
            SetupForceSightShader(pGob, GLRender::kPSFGlowEM, GLRender::kPSDesatEM, part->m_pMaterial->Textures(0).m_pObject,
                                  pEnvMap, 0, 0, false);
        }
        else if (pGob != 0 && pGob->IsIlluminationEnable())
        {
            GLRender::PerInstanceColor color = { 0.0f, 0.0f, 0.0f, 1.0f };
            pGob->GetIlluminationColor(color.r, color.g, color.b);
            GLRender::m_PerInstanceIllumination = color;
            GLRender::pfuncEnablePixelShaders(GLRender::kPSIllumEM, part->m_pMaterial->Textures(0).m_pObject,
                                              pEnvMap, 0, 0);
        }
        else
        {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSEnvMapCube, part->m_pMaterial->Textures(0).m_pObject,
                                              pEnvMap, 0, 0);
        }

        Vector vWhite(1.0f, 1.0f, 1.0f);
        if (g_ForceSight == 0)
            GLRender::pfuncSetPixelShaderAmbientColor(vWhite, fAlpha);

        RenderFlat(vertices);
        GLRender::pfuncEnablePixelShaders(g_ForceSight == 0 ? GLRender::kPSMultiTextureOff : GLRender::kPSDesatOff, 0, 0, 0, 0);

        GLRender::SetActiveTexture(1);
        if (AurCubeMapAvailable())
            GLRender::Disable(Aurora::kCapTextureCubeMap);

        GLRender::SetActiveTexture(0);

        GLRender::Enable(Aurora::kCapAlphaTest);
        return;
    }

    GLRender::Disable(Aurora::kCapAlphaTest);

    if (environmentmapdest != 0)
    {
        GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);
        RenderFlat(vertices);
        GLRender::SetBlendFunc(Aurora::kBlendInvDstAlpha, Aurora::kBlendOne);
        pEnvMap->Bind();

        if (pEnvMap->IsCubeMap())
        {
            GLRender::SetTextureCoordGenerationState(Aurora::kTexCoordGenReflectionMap);
            GLRender::Enable(Aurora::kCapTexGenS);
            GLRender::Enable(Aurora::kCapTexGenT);
            GLRender::Enable(Aurora::kCapTexGenR);
        }
        else
        {
            GLRender::SetTextureCoordGenerationState(Aurora::kTexCoordGenSphereMap);
            GLRender::Enable(Aurora::kCapTexGenS);
            GLRender::Enable(Aurora::kCapTexGenT);
        }
        RenderFlat(vertices);

        if (part->m_pGob == 0)
            part->m_pMaterial->BindTexture0();
    }
    else
    {
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        pEnvMap->Bind();

        if (pEnvMap->IsCubeMap())
        {
            GLRender::SetTextureCoordGenerationState(Aurora::kTexCoordGenReflectionMap);
            GLRender::Enable(Aurora::kCapTexGenS);
            GLRender::Enable(Aurora::kCapTexGenT);
            GLRender::Enable(Aurora::kCapTexGenR);
        }
        else
        {
            GLRender::SetTextureCoordGenerationState(Aurora::kTexCoordGenSphereMap);
            GLRender::Enable(Aurora::kCapTexGenS);
            GLRender::Enable(Aurora::kCapTexGenT);
        }
        RenderFlat(vertices);

        GLRender::Disable(Aurora::kCapTexGenS);
        GLRender::Disable(Aurora::kCapTexGenT);
        GLRender::Disable(Aurora::kCapTexGenR);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        part->m_pMaterial->BindTexture0();
        RenderFlat(vertices);
    }

    if (AurCubeMapAvailable())
        GLRender::BindTexture(Aurora::kCapTextureCubeMap, 0);

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::Enable(Aurora::kCapAlphaTest);

    GLRender::SetActiveTexture(1);
    GLRender::Disable(Aurora::kCapTexGenS);
    GLRender::Disable(Aurora::kCapTexGenT);
    GLRender::Disable(Aurora::kCapTexGenR);
    if (AurCubeMapAvailable())
        GLRender::Disable(Aurora::kCapTextureCubeMap);

    GLRender::SetActiveTexture(0);
#endif
}

#if KOTOR1
void DoEnvMapTransform(CAurTexture* pTexture, unsigned int nStage)
{
    if (AurCubeMapAvailable() && pTexture->IsCubeMap())
    {
        GLRender::SetActiveTexture(nStage);
        GLRender::SetMatrixMode(Aurora::kMatrixTexture);
        ASLgl::glPushMatrix();
        float aMatrix[16];
        if (CurrentCamera != 0)
            MakeMatrix4x4(aMatrix, g_vZero, CurrentCamera->m_qOrientation);
        else
            MakeMatrix4x4(aMatrix, g_vZero, Quaternion());
        ASLgl::glMultMatrixf(aMatrix);
        GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
        GLRender::SetActiveTexture(0);
    }
}
#endif

void RenderLightMappedBumpDiffuse(VertexPrimitiveFlat* vertices)
{
    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

    List<PartLight*> bumpLights(
        (currentscene->m_pLightManager)
            ->GetBumpMapLights());

    if (bumpLights.m_count == 0)
    {
        RenderLightMapped(vertices);
        return;
    }

    if (AurPixelShadersAvailable() == 0)
        return;

    GLRender::Disable(Aurora::kCapFog);

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    K2_ASSUME(part != 0);   // the primitive's owning trimesh: every primitive is built with it
    MdlNodeTriMesh* node = (part != 0) ? static_cast<MdlNodeTriMesh*>(part->m_pNode) : 0;

    static VertexProgram* bumpmapvp =
        new VertexProgram(cBumpMapDiffuseVP, cBumpMapDiffuseVPDeclaration);

    GLRender::Enable(Aurora::kCapVertexProgram);

    Vector vDiffuse;
    if (node != 0)
        vDiffuse = node->m_vDiffuse;


    unsigned char bWasAlphaTest = GLRender::IsEnabled(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapAlphaTest);

    GLRender::ActivateTextureStage(1);

    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    GLRender::EnableClientState(Aurora::kClientStateAttrib12);
    GLRender::EnableClientState(Aurora::kClientStateAttrib13);
    GLRender::EnableClientState(Aurora::kClientStateAttrib14);

    GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmapBumpDiffuse, part->m_pMaterial->Textures(4).m_pObject,
                                      currentscene->m_pNormalizationCubeMap.m_pObject,
                                      part->m_pMaterial->Textures(1).m_pObject,
                                      part->m_pMaterial->Textures(0).m_pObject);

    GLRender::SetVertexShaderConstant(16, currentscene->m_fFogStart, currentscene->m_fFogEnd,
                                      currentscene->m_bFogEnabled != 0 ? 1.0f : 0.0f, 0.5f);
    GLRender::SetVertexShaderConstant(17, 1.0f, -1.0f, 0.0f, 0.0f);

    Vector vLocalPosition;
    Quaternion qLocalOrientation;
    part->GetWorld(vLocalPosition, qLocalOrientation);

    Vector vCam = CurrentCamera->m_vPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
    float cx, cy, cz;
    {
        Vector r = WorldToLocal(vCam, qLocalOrientation, vLocalPosition);
        cx = r.x;
        cy = r.y;
        cz = r.z;
    }
#else
    float cx = vCam.x - vLocalPosition.x;
    float cy = vCam.y - vLocalPosition.y;
    float cz = vCam.z - vLocalPosition.z;
    {
// Rotate by the inverse orientation, with the identity fast-out.
        float qw = qLocalOrientation.w;
        float qx = qLocalOrientation.x;
        float qy = qLocalOrientation.y;
        float qz = qLocalOrientation.z;
        float nw = -qw;
        if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
        {
            float t1 = qy * qy + qz * qz;
            float t2 = qx * qx + qz * qz;
            float t3 = qx * qy - qz * nw;
            float t4 = qx * qy + qz * nw;
            float t5 = qy * nw + qx * qz;
            float t6 = qx * qz - qy * nw;
            float t7 = qy * qz - qx * nw;
            float t8 = qx * nw + qy * qz;
            float t9 = qx * qx + qy * qy;
            float ncX = (1.0f - (t1 + t1)) * cx + (t3 + t3) * cy + (t5 + t5) * cz;
            float ncY = (t4 + t4) * cx + (1.0f - (t2 + t2)) * cy + (t7 + t7) * cz;
            float ncZ = (t6 + t6) * cx + (t8 + t8) * cy + (1.0f - (t9 + t9)) * cz;
            cx = ncX;
            cy = ncY;
            cz = ncZ;
        }
    }
#endif
    vCam.x = cx;
    vCam.y = cy;
    vCam.z = cz;

    float fR;
    for (int k = 0; k < bumpLights.m_count; k++)
    {
        (currentscene->m_pLightManager)
            ->DisableBumpMapLights();
        bumpmapvp->Enable(part, true);

        Vector vLight = bumpLights.m_pData[k]->m_vWorldPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
        float lx, ly, lz;
        {
            Vector r = WorldToLocal(vLight, qLocalOrientation, vLocalPosition);
            lx = r.x;
            ly = r.y;
            lz = r.z;
        }
#else
        float lx = vLight.x - vLocalPosition.x;
        float ly = vLight.y - vLocalPosition.y;
        float lz = vLight.z - vLocalPosition.z;
        {
            float qw = qLocalOrientation.w;
            float qx = qLocalOrientation.x;
            float qy = qLocalOrientation.y;
            float qz = qLocalOrientation.z;
            float nw = -qw;
            if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
            {
                float t1 = qy * qy + qz * qz;
                float t2 = qx * qx + qz * qz;
                float t3 = qx * qy - qz * nw;
                float t4 = qx * qy + qz * nw;
                float t5 = qy * nw + qx * qz;
                float t6 = qx * qz - qy * nw;
                float t7 = qy * qz - qx * nw;
                float t8 = qx * nw + qy * qz;
                float t9 = qx * qx + qy * qy;
                float nlX = (1.0f - (t1 + t1)) * lx + (t3 + t3) * ly + (t5 + t5) * lz;
                float nlY = (t4 + t4) * lx + (1.0f - (t2 + t2)) * ly + (t7 + t7) * lz;
                float nlZ = (t6 + t6) * lx + (t8 + t8) * ly + (1.0f - (t9 + t9)) * lz;
                lx = nlX;
                ly = nlY;
                lz = nlZ;
            }
        }
#endif

        Vector vColor(vDiffuse.x * diffuseintensity * bumpLights.m_pData[k]->m_vLightColour.x,
                      vDiffuse.y * diffuseintensity * bumpLights.m_pData[k]->m_vLightColour.y,
                      vDiffuse.z * diffuseintensity * bumpLights.m_pData[k]->m_vLightColour.z);
        GLRender::pfuncSetBumpLightColor(vColor, 1.0f);

        GLRender::SetVertexShaderConstant(12, lx, ly, lz, 1.0f);
        GLRender::SetVertexShaderConstant(13, cx, cy, cz, 1.0f);
        fR = bumpLights.m_pData[k]->m_fLightRadius * bumpLights.m_pData[k]->m_fLightRadius;
        GLRender::SetVertexShaderConstant(14, 1.0f, 0.0f, 9.0f / (fR + fR), 1.0f);

        RenderFlat(vertices);

        if (bWasAlphaTest != 0)
            GLRender::Enable(Aurora::kCapAlphaTest);

        (currentscene->m_pLightManager)
            ->EnableBumpMapLights();
    }

    GLRender::ActivateTextureStage(1);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::ActivateTextureStage(0);
    GLRender::DisableClientState(Aurora::kClientStateAttrib12);
    GLRender::DisableClientState(Aurora::kClientStateAttrib13);
    GLRender::DisableClientState(Aurora::kClientStateAttrib14);

    currentscene->DoFog();
    GLRender::pfuncEnablePixelShaders(GLRender::kPSMultiTextureOff, 0, 0, 0, 0);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    GLRender::Disable(Aurora::kCapVertexProgram);
    part->m_pMaterial->Textures(0).m_pObject->Bind();
}

char* setspecularexponent(char* pszArgs)
{
    int nExponent = atoi(pszArgs);
    switch (nExponent)
    {
    case 1:
    case 2:
    case 4:
    case 8:
        bmspecularexponent = nExponent;
        return const_cast<char*>("Okey dokey.");
    default:
        return const_cast<char*>("Exponent must be one of 1,2,4,8");
    }
}

void RenderLightMappedBumpSpecular(VertexPrimitiveFlat* vertices)
{
    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

    List<PartLight*> bumpLights(
        (currentscene->m_pLightManager)
            ->GetBumpMapLights());

    if (bumpLights.m_count == 0)
    {
        RenderLightMapped(vertices);
        return;
    }

    unsigned char bWasAlphaTest = GLRender::IsEnabled(Aurora::kCapAlphaTest);
#if KOTOR1  // K1: this first save is never restored
    K2_UNUSED(bWasAlphaTest);
#endif
    GLRender::Disable(Aurora::kCapAlphaTest);

    if (AurPixelShadersAvailable() != 0)
    {
        PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
        K2_ASSUME(part != 0);   // the primitive's owning trimesh: every primitive is built with it
#if KOTOR1  // K1: disables it here and never touches it again
        GLRender::Disable(Aurora::kCapFog);
#else
        GLRender::Enable(Aurora::kCapFog);
#endif
        MdlNodeTriMesh* node = (part != 0) ? static_cast<MdlNodeTriMesh*>(part->m_pNode) : 0;

        Vector vSpec;
        Vector vDiffuse;

        static VertexProgram* bumpmapvp =
            new VertexProgram(cBumpMapSpecularVP, cBumpMapSpecularVPDeclaration);

        GLRender::Enable(Aurora::kCapVertexProgram);

        if (node != 0)
        {
            vSpec = part->m_pMaterial->Textures(4).m_pObject->GetSpecularColor();
            vDiffuse = node->m_vDiffuse;
            float fDiffuseMag = magnitude(vDiffuse);
            float fSpecMag = 0.0f;
            if (bumpspecular != 0 && part->m_pMaterial->BumpMapIsSpecular())
                fSpecMag = magnitude(vSpec);
            Vector vScale = vDiffuse * fDiffuseMag;
            if (fDiffuseMag + fSpecMag != 0.0f)
                vScale = vScale * (1.0f / (fDiffuseMag + fSpecMag));
            vDiffuse = vScale;
        }


        unsigned char bWasAlphaTest2 = GLRender::IsEnabled(Aurora::kCapAlphaTest);
        GLRender::Disable(Aurora::kCapAlphaTest);

        GLRender::ActivateTextureStage(1);

        GLRender::EnableClientState(Aurora::kClientStateTexCoord);
        GLRender::EnableClientState(Aurora::kClientStateAttrib12);
        GLRender::EnableClientState(Aurora::kClientStateAttrib13);
        GLRender::EnableClientState(Aurora::kClientStateAttrib14);

        switch (bmspecularexponent)
        {
        case 1:
            GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmapBumpSpecular1, part->m_pMaterial->Textures(4).m_pObject,
                currentscene->m_pNormalizationCubeMap.m_pObject,
                part->m_pMaterial->Textures(1).m_pObject, part->m_pMaterial->Textures(0).m_pObject);
            break;
        case 2:
            GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmapBumpSpecular2, part->m_pMaterial->Textures(4).m_pObject,
                currentscene->m_pNormalizationCubeMap.m_pObject,
                part->m_pMaterial->Textures(1).m_pObject, part->m_pMaterial->Textures(0).m_pObject);
            break;
        case 4:
            GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmapBumpSpecular4, part->m_pMaterial->Textures(4).m_pObject,
                currentscene->m_pNormalizationCubeMap.m_pObject,
                part->m_pMaterial->Textures(1).m_pObject, part->m_pMaterial->Textures(0).m_pObject);
            break;
        case 8:
            GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmapBumpSpecular8, part->m_pMaterial->Textures(4).m_pObject,
                currentscene->m_pNormalizationCubeMap.m_pObject,
                part->m_pMaterial->Textures(1).m_pObject, part->m_pMaterial->Textures(0).m_pObject);
            break;
        }

        GLRender::SetVertexShaderConstant(16, currentscene->m_fFogStart, currentscene->m_fFogEnd,
                                          currentscene->m_bFogEnabled != 0 ? 1.0f : 0.0f, 0.5f);
        GLRender::SetVertexShaderConstant(17, 1.0f, -1.0f, 0.0f, 0.0f);

// The light and the camera are carried into the part's accumulated world frame.
        Quaternion qLocalOrientation;
        Vector vLocalPosition;
        part->GetWorld(vLocalPosition, qLocalOrientation);

        Vector vCam = CurrentCamera->m_vPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
        float cx, cy, cz;
        {
            Vector r = WorldToLocal(vCam, qLocalOrientation, vLocalPosition);
            cx = r.x;
            cy = r.y;
            cz = r.z;
        }
#else
        float cx = vCam.x - vLocalPosition.x;
        float cy = vCam.y - vLocalPosition.y;
        float cz = vCam.z - vLocalPosition.z;
        {
// Rotate by the inverse orientation, with the identity fast-out.
            float qw = qLocalOrientation.w;
            float qx = qLocalOrientation.x;
            float qy = qLocalOrientation.y;
            float qz = qLocalOrientation.z;
            float nw = -qw;
            if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
            {
                float t1 = qy * qy + qz * qz;
                float t2 = qx * qx + qz * qz;
                float t3 = qx * qy - qz * nw;
                float t4 = qx * qy + qz * nw;
                float t5 = qy * nw + qx * qz;
                float t6 = qx * qz - qy * nw;
                float t7 = qy * qz - qx * nw;
                float t8 = qx * nw + qy * qz;
                float t9 = qx * qx + qy * qy;
                float ncX = (1.0f - (t1 + t1)) * cx + (t3 + t3) * cy + (t5 + t5) * cz;
                float ncY = (t4 + t4) * cx + (1.0f - (t2 + t2)) * cy + (t7 + t7) * cz;
                float ncZ = (t6 + t6) * cx + (t8 + t8) * cy + (1.0f - (t9 + t9)) * cz;
                cx = ncX;
                cy = ncY;
                cz = ncZ;
            }
        }
#endif
        vCam.x = cx;
        vCam.y = cy;
        vCam.z = cz;

        float fR;
        for (int k = 0; k < bumpLights.m_count; k++)
        {
            bumpmapvp->Enable(part, true);

            PartLight* pLight = bumpLights.m_pData[k];
            Vector vLight = pLight->m_vWorldPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
            float lx, ly, lz;
            {
                Vector r = WorldToLocal(vLight, qLocalOrientation, vLocalPosition);
                lx = r.x;
                ly = r.y;
                lz = r.z;
            }
#else
            float lx = vLight.x - vLocalPosition.x;
            float ly = vLight.y - vLocalPosition.y;
            float lz = vLight.z - vLocalPosition.z;
            {
                float qw = qLocalOrientation.w;
                float qx = qLocalOrientation.x;
                float qy = qLocalOrientation.y;
                float qz = qLocalOrientation.z;
                float nw = -qw;
                if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
                {
                    float t1 = qy * qy + qz * qz;
                    float t2 = qx * qx + qz * qz;
                    float t3 = qx * qy - qz * nw;
                    float t4 = qx * qy + qz * nw;
                    float t5 = qy * nw + qx * qz;
                    float t6 = qx * qz - qy * nw;
                    float t7 = qy * qz - qx * nw;
                    float t8 = qx * nw + qy * qz;
                    float t9 = qx * qx + qy * qy;
                    float nlX = (1.0f - (t1 + t1)) * lx + (t3 + t3) * ly + (t5 + t5) * lz;
                    float nlY = (t4 + t4) * lx + (1.0f - (t2 + t2)) * ly + (t7 + t7) * lz;
                    float nlZ = (t6 + t6) * lx + (t8 + t8) * ly + (1.0f - (t9 + t9)) * lz;
                    lx = nlX;
                    ly = nlY;
                    lz = nlZ;
                }
            }
#endif

            Vector vSpecular = vSpec;
            float fSpecularIntensity = specularintensity;
            float fBumpIntensity =
                part->m_pMaterial->Textures(4).m_pObject->GetSpecularBumpIntensity();
            Vector vColor(
                vSpecular.x * fSpecularIntensity * fBumpIntensity * pLight->m_vLightColour.x,
                vSpecular.y * fSpecularIntensity * fBumpIntensity * pLight->m_vLightColour.y,
                vSpecular.z * fSpecularIntensity * fBumpIntensity * pLight->m_vLightColour.z);
            GLRender::pfuncSetPixelShaderAmbientColor(vColor, 1.0f);

            GLRender::SetVertexShaderConstant(12, lx, ly, lz, 1.0f);
            GLRender::SetVertexShaderConstant(13, cx, cy, cz, 1.0f);
            fR = bumpLights.m_pData[k]->m_fLightRadius * bumpLights.m_pData[k]->m_fLightRadius;
            GLRender::SetVertexShaderConstant(14, 1.0f, 0.0f, 9.0f / (fR + fR), 1.0f);

            RenderFlat(vertices);

            if (bWasAlphaTest2 != 0)
                GLRender::Enable(Aurora::kCapAlphaTest);
        }

#if !KOTOR1
        GLRender::Disable(Aurora::kCapFog);
#endif
        GLRender::DisableClientState(Aurora::kClientStateAttrib5);
        GLRender::DisableClientState(Aurora::kClientStateAttrib12);
        GLRender::DisableClientState(Aurora::kClientStateAttrib13);
        GLRender::DisableClientState(Aurora::kClientStateAttrib14);
        GLRender::ActivateTextureStage(1);
        GLRender::DisableClientState(Aurora::kClientStateTexCoord);
        GLRender::ActivateTextureStage(0);

        currentscene->DoFog();
        GLRender::pfuncEnablePixelShaders(GLRender::kPSMultiTextureOff, 0, 0, 0, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
        GLRender::Disable(Aurora::kCapVertexProgram);
    }

#if !KOTOR1
    if (bWasAlphaTest != 0)
        GLRender::Enable(Aurora::kCapAlphaTest);
#endif
}

void RenderDiffuseBumpMappedPass(VertexPrimitiveFlat* vertices)
{
    if (!enablerender)
        return;

    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    Part* part = vertices->m_pPart;
    MdlNodeTriMesh* node = (part->AsPartTriMesh() != 0) ? static_cast<MdlNodeTriMesh*>(part->m_pNode) : 0;

    Vector vDiffuse;
    Vector vSpec;
    if (node != 0)
    {
        vSpec = part->m_pMaterial->Textures(4).m_pObject->GetSpecularColor();
        vDiffuse = node->m_vDiffuse;
        float fDiffuseMag = magnitude(vDiffuse);
        float fSpecMag = 0.0f;
        if (bumpspecular != 0 && part->m_pMaterial->BumpMapIsSpecular())
            fSpecMag = magnitude(vSpec);
        Vector vScale = vDiffuse * fDiffuseMag;
        if (fDiffuseMag + fSpecMag != 0.0f)
            vScale = vScale * (1.0f / (fDiffuseMag + fSpecMag));
        vDiffuse = vScale;
    }

    static VertexProgram* bumpmapvp = new VertexProgram(cBumpMapVP, cBumpMapVPDeclaration);
    bumpmapvp->Enable(part, false);

    GLRender::Enable(Aurora::kCapVertexProgram);
    GLRender::SetVertexShaderConstant(17, 1.0f, -1.0f, 0.0f, 0.0f);
    GLRender::Enable(Aurora::kCapFog);
    GLRender::SetFogMode(Aurora::kFogLinear);
    GLRender::SetFogModef(Aurora::kCapFogStart, 0.0f);
    GLRender::SetFogModef(Aurora::kCapFogEnd, 1.0f);
    Vector vFog;
    GLRender::SetFogModev(Aurora::kCapFogColor, vFog);

    List<PartLight*> bumpLights(
        (currentscene->m_pLightManager)
            ->GetBumpMapLights());

    Vector& vAmbient = currentscene->m_ambient;
    GLRender::pfuncSetPixelShaderAmbientColor(vAmbient, 1.0f);

    ASLgl::glPushClientAttrib(2);
    GLRender::EnableClientState(Aurora::kClientStateAttrib5);
    GLRender::EnableClientState(Aurora::kClientStateAttrib12);
    GLRender::EnableClientState(Aurora::kClientStateAttrib13);
    GLRender::EnableClientState(Aurora::kClientStateAttrib14);
    GLRender::EnableClientState(Aurora::kClientStateAttrib15);
    GLRender::DisableClientState(Aurora::kClientStateNormal);
    GLRender::DisableClientState(Aurora::kClientStateColor);

    GLRender::pfuncEnablePixelShaders(GLRender::kPSDiffuseBumpPass, part->m_pMaterial->Textures(4).m_pObject,
                                      currentscene->m_pNormalizationCubeMap.m_pObject, 0, 0);

    for (int k = 0; k < bumpLights.m_count; k++)
    {
        Vector vLight = bumpLights.m_pData[k]->m_vWorldPosition;
        PartLight* pLight = bumpLights.m_pData[k];
        float fRadius = pLight->m_fLightRadius;

        vLight = WorldToLocal(vLight, part->m_qLocalOrientation, part->m_vLocalPosition);

        vertices->UpdateLightVectors(vLight);
        vertices->UpdateFogValues(vLight, fRadius);

        Vector vScaled = vDiffuse * diffuseintensity;
        Vector vColor(vScaled.x * pLight->m_vLightColour.x, vScaled.y * pLight->m_vLightColour.y,
                      vScaled.z * pLight->m_vLightColour.z);
        GLRender::pfuncSetBumpLightColor(vColor, 1.0f);

        GLRender::SetBlendFunc(Aurora::kBlendOne, k == 0 ? Aurora::kBlendZero : Aurora::kBlendOne);
        RenderFlat(vertices);
    }

    GLRender::DisableClientState(Aurora::kClientStateAttrib5);
    GLRender::DisableClientState(Aurora::kClientStateAttrib12);
    GLRender::DisableClientState(Aurora::kClientStateAttrib13);
    GLRender::DisableClientState(Aurora::kClientStateAttrib14);
    GLRender::DisableClientState(Aurora::kClientStateAttrib15);
    GLRender::EnableClientState(Aurora::kClientStateNormal);
    GLRender::EnableClientState(Aurora::kClientStateColor);
    ASLgl::glPopClientAttrib();

    currentscene->DoFog();
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    GLRender::Disable(Aurora::kCapVertexProgram);
}

void RenderSpecularBumpMappedPass(VertexPrimitiveFlat* vertices)
{
    if (!enablerender)
        return;

    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
    K2_ASSUME(part != 0);   // the primitive's owning trimesh: every primitive is built with it
    MdlNodeTriMesh* node = (part != 0) ? static_cast<MdlNodeTriMesh*>(part->m_pNode) : 0;

    Vector vSpec;
    Vector vDiffuse;
    vSpec = part->m_pMaterial->Textures(4).m_pObject->GetSpecularColor();
    if (node != 0)
    {
        vDiffuse = node->m_vDiffuse;
        float fDiffuseMag = magnitude(vDiffuse);
        float fSpecMag = magnitude(vSpec);
        Vector vScale = vDiffuse * fSpecMag;
        if (fDiffuseMag + fSpecMag != 0.0f)
            vScale = vScale * (1.0f / (fDiffuseMag + fSpecMag));
        vSpec = vScale;
    }

    static VertexProgram* bumpmapvp = new VertexProgram(cBumpMapVP, cBumpMapVPDeclaration);
    bumpmapvp->Enable(part, false);

    GLRender::Enable(Aurora::kCapVertexProgram);
    GLRender::SetVertexShaderConstant(17, 1.0f, -1.0f, 0.0f, 0.0f);

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendOne);

    GLRender::Enable(Aurora::kCapFog);
    GLRender::SetFogMode(Aurora::kFogLinear);
    GLRender::SetFogModef(Aurora::kCapFogStart, 0.0f);
    GLRender::SetFogModef(Aurora::kCapFogEnd, 1.0f);
    Vector vFog;
    GLRender::SetFogModev(Aurora::kCapFogColor, vFog);

    List<PartLight*> bumpLights(
        (currentscene->m_pLightManager)
            ->GetBumpMapLights());

    ASLgl::glPushClientAttrib(2);
    GLRender::EnableClientState(Aurora::kClientStateAttrib5);
    GLRender::EnableClientState(Aurora::kClientStateAttrib12);
    GLRender::EnableClientState(Aurora::kClientStateAttrib13);
    GLRender::EnableClientState(Aurora::kClientStateAttrib14);
    GLRender::EnableClientState(Aurora::kClientStateAttrib15);
    GLRender::DisableClientState(Aurora::kClientStateNormal);
    GLRender::DisableClientState(Aurora::kClientStateColor);

    GLRender::pfuncEnablePixelShaders(GLRender::kPSSpecularBumpPass, part->m_pMaterial->Textures(0).m_pObject,
                                      part->m_pMaterial->Textures(4).m_pObject, 0,
                                      currentscene->m_pNormalizationCubeMap.m_pObject);

    for (int k = 0; k < bumpLights.m_count; k++)
    {
        Vector vLight = bumpLights.m_pData[k]->m_vWorldPosition;
        Vector vCam = CurrentCamera->m_vPosition;
        float fRadius = bumpLights.m_pData[k]->m_fLightRadius;

        vLight = WorldToLocal(vLight, part->m_qLocalOrientation, part->m_vLocalPosition);
        vCam = WorldToLocal(vCam, part->m_qLocalOrientation, part->m_vLocalPosition);

        vertices->UpdateHalfAngleVectors(vLight, vCam);
        vertices->UpdateFogValues(vLight, fRadius);

        PartLight* pLight = bumpLights.m_pData[k];
        Vector vScaled = vSpec * specularintensity
                         * part->m_pMaterial->Textures(4).m_pObject->GetBumpIntensity();
        Vector vColor(vScaled.x * pLight->m_vLightColour.x, vScaled.y * pLight->m_vLightColour.y,
                      vScaled.z * pLight->m_vLightColour.z);
        GLRender::pfuncSetBumpLightColor(vColor, 1.0f);

        RenderFlat(vertices);
    }

    GLRender::DisableClientState(Aurora::kClientStateAttrib5);
    GLRender::DisableClientState(Aurora::kClientStateAttrib12);
    GLRender::DisableClientState(Aurora::kClientStateAttrib13);
    GLRender::DisableClientState(Aurora::kClientStateAttrib14);
    GLRender::DisableClientState(Aurora::kClientStateAttrib15);
    GLRender::EnableClientState(Aurora::kClientStateNormal);
    GLRender::EnableClientState(Aurora::kClientStateColor);
    ASLgl::glPopClientAttrib();

    currentscene->DoFog();
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    GLRender::Disable(Aurora::kCapVertexProgram);
}

// Renders bump maps through the one-pass pixel-shader path, else the two-pass fixed-function
// fallback.
void RenderBumpMapped(VertexPrimitiveFlat* vertices)
{
    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

    if (enablerenderbumpmap == 0)
        return;
    if (enablerender == 0)
        return;

    List<PartLight*> bumpLights(
        (currentscene->m_pLightManager)
            ->GetBumpMapLights());

    if (bumpLights.m_count == 0)
    {
#ifdef K2_QOL_BUMPMAPS
        // The PC drew this with fixed-function lighting, which only a program does here.
        RenderFlatVP(vertices, vertices->m_pPart->AsPartTriMesh());
#else
        RenderFlat(vertices);
#endif
        return;
    }

    unsigned char bWasAlphaTest = GLRender::IsEnabled(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapAlphaTest);

    if (AurPixelShadersAvailable() != 0 && useonepasssolution != 0)
    {
// The one-pass pixel-shader arm.
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
        GLRender::Disable(Aurora::kCapVertexProgram);

        PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
        K2_ASSUME(part != 0);   // the primitive's owning trimesh: every primitive is built with it
        MdlNodeTriMesh* node = (part != 0) ? static_cast<MdlNodeTriMesh*>(part->m_pNode) : 0;

        static VertexProgram* bumpmapvp =
            new VertexProgram(usearbvertexprograms != 0 ? szBumpmapVPARB : cBumpMapOnePassVP,
                              cBumpMapOnePassVPDeclaration);

        Vector vSpec;
        Vector vDiffuse;

        GLRender::Enable(Aurora::kCapVertexProgram);

        if (node != 0)
        {
            vSpec = part->m_pMaterial->Textures(4).m_pObject->GetSpecularColor();
            vDiffuse = node->m_vDiffuse;
        }

        GLRender::EnableClientState(Aurora::kClientStateAttrib12);
        GLRender::EnableClientState(Aurora::kClientStateAttrib13);
        GLRender::EnableClientState(Aurora::kClientStateAttrib14);

#if !KOTOR1  // K1: no force-sight arm
        if (g_ForceSight != 0)
        {
// The force-sight passthrough puts the bump map in parameter 1, the cube map in 2 and 3, and
// material slot 0 in 4.
            bumpmapvp->Enable(part, true);
            SetupForceSightShader(vertices->m_pPart->m_pGob, GLRender::kPSFGlowBumpmap, GLRender::kPSDesatBumpmap,
                                  part->m_pMaterial->Textures(4).m_pObject,
                                  currentscene->m_pNormalizationCubeMap.m_pObject,
                                  currentscene->m_pNormalizationCubeMap.m_pObject,
                                  part->m_pMaterial->Textures(0).m_pObject, false);
            RenderFlat(vertices);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
            GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        }
        else
#endif
        {
// Mode 6 takes the bump map, the cube map (as texture and previous unit), then material slot 0.
            GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmap, part->m_pMaterial->Textures(4).m_pObject,
                                              currentscene->m_pNormalizationCubeMap.m_pObject,
                                              currentscene->m_pNormalizationCubeMap.m_pObject,
                                              part->m_pMaterial->Textures(0).m_pObject);

            GLRender::SetVertexShaderConstant(16, currentscene->m_fFogStart, currentscene->m_fFogEnd,
                                              currentscene->m_bFogEnabled != 0 ? 1.0f : 0.0f, 0.5f);
            GLRender::SetVertexShaderConstant(17, 1.0f, -1.0f, 0.0f, 0.0f);

            Quaternion qLocalOrientation;
            Vector vLocalPosition;
            part->GetWorld(vLocalPosition, qLocalOrientation);

            Vector vCam = CurrentCamera->m_vPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
            float cx, cy, cz;
            {
                Vector r = WorldToLocal(vCam, qLocalOrientation, vLocalPosition);
                cx = r.x;
                cy = r.y;
                cz = r.z;
            }
#else
            float cx = vCam.x - vLocalPosition.x;
            float cy = vCam.y - vLocalPosition.y;
            float cz = vCam.z - vLocalPosition.z;
            {
// Rotate by the inverse orientation, with the identity fast-out.
                float qw = qLocalOrientation.w;
                float qx = qLocalOrientation.x;
                float qy = qLocalOrientation.y;
                float qz = qLocalOrientation.z;
                float nw = -qw;
                if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
                {
                    float t1 = qy * qy + qz * qz;
                    float t2 = qx * qx + qz * qz;
                    float t3 = qx * qy - qz * nw;
                    float t4 = qx * qy + qz * nw;
                    float t5 = qy * nw + qx * qz;
                    float t6 = qx * qz - qy * nw;
                    float t7 = qy * qz - qx * nw;
                    float t8 = qx * nw + qy * qz;
                    float t9 = qx * qx + qy * qy;
                    float ncX = (1.0f - (t1 + t1)) * cx + (t3 + t3) * cy + (t5 + t5) * cz;
                    float ncY = (t4 + t4) * cx + (1.0f - (t2 + t2)) * cy + (t7 + t7) * cz;
                    float ncZ = (t6 + t6) * cx + (t8 + t8) * cy + (1.0f - (t9 + t9)) * cz;
                    cx = ncX;
                    cy = ncY;
                    cz = ncZ;
                }
            }
#endif
            vCam.x = cx;
            vCam.y = cy;
            vCam.z = cz;

            float fR;
            for (int k = 0; k < bumpLights.m_count; k++)
            {
                (currentscene->m_pLightManager)
                    ->DisableBumpMapLights();
                bumpmapvp->Enable(part, true);

                PartLight* pLight = bumpLights.m_pData[k];
                Vector vLight = pLight->m_vWorldPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
                float lx, ly, lz;
                {
                    Vector r = WorldToLocal(vLight, qLocalOrientation, vLocalPosition);
                    lx = r.x;
                    ly = r.y;
                    lz = r.z;
                }
#else
                float lx = vLight.x - vLocalPosition.x;
                float ly = vLight.y - vLocalPosition.y;
                float lz = vLight.z - vLocalPosition.z;
                {
                    float qw = qLocalOrientation.w;
                    float qx = qLocalOrientation.x;
                    float qy = qLocalOrientation.y;
                    float qz = qLocalOrientation.z;
                    float nw = -qw;
                    if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
                    {
                        float t1 = qy * qy + qz * qz;
                        float t2 = qx * qx + qz * qz;
                        float t3 = qx * qy - qz * nw;
                        float t4 = qx * qy + qz * nw;
                        float t5 = qy * nw + qx * qz;
                        float t6 = qx * qz - qy * nw;
                        float t7 = qy * qz - qx * nw;
                        float t8 = qx * nw + qy * qz;
                        float t9 = qx * qx + qy * qy;
                        float nlX = (1.0f - (t1 + t1)) * lx + (t3 + t3) * ly + (t5 + t5) * lz;
                        float nlY = (t4 + t4) * lx + (1.0f - (t2 + t2)) * ly + (t7 + t7) * lz;
                        float nlZ = (t6 + t6) * lx + (t8 + t8) * ly + (1.0f - (t9 + t9)) * lz;
                        lx = nlX;
                        ly = nlY;
                        lz = nlZ;
                    }
                }
#endif

                Vector vFallback;
                {
                    float fDiffuseIntensity = diffuseintensity;
                    float fBumpIntensity =
                        part->m_pMaterial->Textures(4).m_pObject->GetDiffuseBumpIntensity();
                    Vector vColor(
                        vDiffuse.x * fDiffuseIntensity * fBumpIntensity * pLight->m_vLightColour.x,
                        vDiffuse.y * fDiffuseIntensity * fBumpIntensity * pLight->m_vLightColour.y,
                        vDiffuse.z * fDiffuseIntensity * fBumpIntensity * pLight->m_vLightColour.z);
                    if (usebumpdiffuse != 0)
                    {
                        GLRender::pfuncSetBumpLightColor(vColor, 1.0f);
                    }
                    else
                    {
                        vFallback = Vector(1.0f, 0.0f, 0.0f);
                        GLRender::pfuncSetBumpLightColor(vFallback, 1.0f);
                    }
                }

                float fSpecularIntensity = specularintensity;
                float fSpecBumpIntensity =
                    part->m_pMaterial->Textures(4).m_pObject->GetSpecularBumpIntensity();
                Vector vSpecColor(
                    vSpec.x * fSpecularIntensity * fSpecBumpIntensity * pLight->m_vLightColour.x,
                    vSpec.y * fSpecularIntensity * fSpecBumpIntensity * pLight->m_vLightColour.y,
                    vSpec.z * fSpecularIntensity * fSpecBumpIntensity * pLight->m_vLightColour.z);
                if (usebumpspecular != 0)
                {
                    GLRender::pfuncSetPixelShaderAmbientColor(vSpecColor, 1.0f);
                }
                else
                {
                    vFallback = Vector(0.0f, 1.0f, 0.0f);
                    GLRender::pfuncSetPixelShaderAmbientColor(vFallback, 1.0f);
                }

                GLRender::SetVertexShaderConstant(12, lx, ly, lz, 1.0f);
                GLRender::SetVertexShaderConstant(13, cx, cy, cz, 1.0f);
                fR = pLight->m_fLightRadius * pLight->m_fLightRadius;
                GLRender::SetVertexShaderConstant(14, 1.0f, 0.0f, 9.0f / (fR + fR), 1.0f);

                RenderFlat(vertices);

// The one-pass loop restores the alpha test per light; the multi-pass arm does it once at the end.
                if (bWasAlphaTest != 0)
                    GLRender::Enable(Aurora::kCapAlphaTest);

                (currentscene->m_pLightManager)
                    ->EnableBumpMapLights();
            }
        }

        GLRender::DisableClientState(Aurora::kClientStateAttrib12);
        GLRender::DisableClientState(Aurora::kClientStateAttrib13);
        GLRender::DisableClientState(Aurora::kClientStateAttrib14);

        currentscene->DoFog();
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
#if KOTOR1  // K1: no pixel-shader reset and no ARB-specific disable
        GLRender::Disable(Aurora::kCapVertexProgram);
#else
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
        if (usearbvertexprograms != 0)
            ASLgl::glDisable(GL_VERTEX_PROGRAM_ARB);
        else
            GLRender::Disable(Aurora::kCapVertexProgram);
#endif
    }
    else
    {
// The multi-pass fixed-function arm.
        GLRender::Disable(Aurora::kCapFog);
        if (bumpdiffuse != 0)
            RenderDiffuseBumpMappedPass(vertices);

        static VertexProgram* standardvp =
            new VertexProgram(cStandardVP, cStandardVPDeclaration);

        if (bumpother != 0)
        {
            (currentscene->m_pLightManager)
                ->DisableBumpMapLights();
            standardvp->Enable(vertices->m_pPart, true);
            GLRender::Enable(Aurora::kCapVertexProgram);
            GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendOne);
            RenderFlatNonTextured(vertices);
            GLRender::Disable(Aurora::kCapVertexProgram);
            (currentscene->m_pLightManager)
                ->EnableBumpMapLights();
        }

        if (bumptexture != 0)
        {
            standardvp->Enable(vertices->m_pPart, true);
            GLRender::Enable(Aurora::kCapVertexProgram);
            GLRender::SetBlendFunc(Aurora::kBlendDstColor, Aurora::kBlendZero);
            RenderFlatDecal(vertices);
            GLRender::Disable(Aurora::kCapVertexProgram);
        }

        if (bumpspecular != 0)
            RenderSpecularBumpMappedPass(vertices);

        currentscene->DoFog();

        int nFogEnabled;
        GLRender::QueryIntegerv(Aurora::kQueryFog, &nFogEnabled);
        if (nFogEnabled != 0)
        {
            static VertexProgram* fogvp =
                new VertexProgram(cFogOutputVP, cFogOutputVPDeclaration);
            fogvp->Enable(vertices->m_pPart, false);
            GLRender::Enable(Aurora::kCapVertexProgram);

            GLRender::SetVertexShaderConstant(20, currentscene->m_fFogStart, currentscene->m_fFogEnd,
                                              static_cast<float>(currentscene->m_nFogMode), 1.0f);
            GLRender::SetVertexShaderConstant(21, currentscene->m_vFogColor.x, currentscene->m_vFogColor.y,
                                              currentscene->m_vFogColor.z, 1.0f);
            GLRender::SetVertexShaderConstant(22, 1.0f, 0.0f, 0.0f, 0.0f);

            GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
            RenderFlatNonTextured(vertices);
            GLRender::Disable(Aurora::kCapVertexProgram);
        }

        if (bWasAlphaTest != 0)
            GLRender::Enable(Aurora::kCapAlphaTest);

        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, vertices->m_pPart->m_pMaterial->Textures(0).m_pObject,
                                          0, 0, 0);
    }
}

// Normal maps; pixel-shader cards only, with no multi-pass fallback.
void RenderNormalMapped(VertexPrimitiveFlat* vertices)
{
    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

    if (enablerenderbumpmap == 0)
        return;
    if (enablerender == 0)
        return;

    List<PartLight*> bumpLights(
        (currentscene->m_pLightManager)
            ->GetBumpMapLights());

    if (bumpLights.m_count == 0)
    {
        RenderFlat(vertices);
        return;
    }

    unsigned char bWasAlphaTest = GLRender::IsEnabled(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapAlphaTest);

    if (AurPixelShadersAvailable() != 0 && useonepasssolution != 0)
    {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
        GLRender::Disable(Aurora::kCapVertexProgram);

// The arm works through the tri-mesh identity; m_pNode is read off that.
        PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();
        K2_ASSUME(part != 0);   // the primitive's owning trimesh: every primitive is built with it
        MdlNodeTriMesh* node = (part != 0) ? static_cast<MdlNodeTriMesh*>(part->m_pNode) : 0;

        static VertexProgram* normalmapvp =
            new VertexProgram(szNormalmapVPARB, szNormalmapVPARBDeclaration);

        Vector vSpec;
        Vector vDiffuse;
        GLRender::Enable(Aurora::kCapVertexProgram);

        if (node != 0)
        {
            vSpec = part->m_pMaterial->Textures(4).m_pObject->GetSpecularColor();
            vDiffuse = node->m_vDiffuse;
        }

        GLRender::pfuncEnablePixelShaders(GLRender::kPSNormalmap, part->m_pMaterial->Textures(0).m_pObject,
                                          part->m_pMaterial->Textures(4).m_pObject,
                                          currentscene->m_pNormalizationCubeMap.m_pObject,
                                          currentscene->m_pNormalizationCubeMap.m_pObject);

        GLRender::SetVertexShaderConstant(16, currentscene->m_fFogStart, currentscene->m_fFogEnd,
                                          currentscene->m_bFogEnabled != 0 ? 1.0f : 0.0f, 0.5f);
        GLRender::SetVertexShaderConstant(17, 1.0f, -1.0f, 0.0f, 0.0f);

        Quaternion qLocalOrientation;
        Vector vLocalPosition;
        part->GetWorld(vLocalPosition, qLocalOrientation);

        Vector vCam = CurrentCamera->m_vPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
        float cx, cy, cz;
        {
            Vector r = WorldToLocal(vCam, qLocalOrientation, vLocalPosition);
            cx = r.x;
            cy = r.y;
            cz = r.z;
        }
#else
        float cx = vCam.x - vLocalPosition.x;
        float cy = vCam.y - vLocalPosition.y;
        float cz = vCam.z - vLocalPosition.z;
        {
// Rotate by the inverse orientation, with the identity fast-out.
            float qw = qLocalOrientation.w;
            float qx = qLocalOrientation.x;
            float qy = qLocalOrientation.y;
            float qz = qLocalOrientation.z;
            float nw = -qw;
            if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
            {
                float t1 = qy * qy + qz * qz;
                float t2 = qx * qx + qz * qz;
                float t3 = qx * qy - qz * nw;
                float t4 = qx * qy + qz * nw;
                float t5 = qy * nw + qx * qz;
                float t6 = qx * qz - qy * nw;
                float t7 = qy * qz - qx * nw;
                float t8 = qx * nw + qy * qz;
                float t9 = qx * qx + qy * qy;
                float ncX = (1.0f - (t1 + t1)) * cx + (t3 + t3) * cy + (t5 + t5) * cz;
                float ncY = (t4 + t4) * cx + (1.0f - (t2 + t2)) * cy + (t7 + t7) * cz;
                float ncZ = (t6 + t6) * cx + (t8 + t8) * cy + (1.0f - (t9 + t9)) * cz;
                cx = ncX;
                cy = ncY;
                cz = ncZ;
            }
        }
#endif
        vCam.x = cx;
        vCam.y = cy;
        vCam.z = cz;

// Two constant fallbacks, hoisted out of the loop.
        Vector vFallbackDiffuse(1.0f, 0.0f, 0.0f);
        Vector vFallbackSpecular(0.0f, 1.0f, 0.0f);

        for (int k = 0; k < bumpLights.m_count; k++)
        {
            (currentscene->m_pLightManager)
                ->DisableBumpMapLights();
            normalmapvp->Enable(part, true);

            PartLight* pLight = bumpLights.m_pData[k];
            Vector vLight = pLight->m_vWorldPosition;
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
            float lx, ly, lz;
            {
                Vector r = WorldToLocal(vLight, qLocalOrientation, vLocalPosition);
                lx = r.x;
                ly = r.y;
                lz = r.z;
            }
#else
            float lx = vLight.x - vLocalPosition.x;
            float ly = vLight.y - vLocalPosition.y;
            float lz = vLight.z - vLocalPosition.z;
            {
                float qw = qLocalOrientation.w;
                float qx = qLocalOrientation.x;
                float qy = qLocalOrientation.y;
                float qz = qLocalOrientation.z;
                float nw = -qw;
                if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
                {
                    float t1 = qy * qy + qz * qz;
                    float t2 = qx * qx + qz * qz;
                    float t3 = qx * qy - qz * nw;
                    float t4 = qx * qy + qz * nw;
                    float t5 = qy * nw + qx * qz;
                    float t6 = qx * qz - qy * nw;
                    float t7 = qy * qz - qx * nw;
                    float t8 = qx * nw + qy * qz;
                    float t9 = qx * qx + qy * qy;
                    float nlX = (1.0f - (t1 + t1)) * lx + (t3 + t3) * ly + (t5 + t5) * lz;
                    float nlY = (t4 + t4) * lx + (1.0f - (t2 + t2)) * ly + (t7 + t7) * lz;
                    float nlZ = (t6 + t6) * lx + (t8 + t8) * ly + (1.0f - (t9 + t9)) * lz;
                    lx = nlX;
                    ly = nlY;
                    lz = nlZ;
                }
            }
#endif

            {
                float fDiffuseIntensity = diffuseintensity;
                float fBumpIntensity =
                    part->m_pMaterial->Textures(4).m_pObject->GetDiffuseBumpIntensity();
                Vector vColor(
                    vDiffuse.x * fDiffuseIntensity * fBumpIntensity * pLight->m_vLightColour.x,
                    vDiffuse.y * fDiffuseIntensity * fBumpIntensity * pLight->m_vLightColour.y,
                    vDiffuse.z * fDiffuseIntensity * fBumpIntensity * pLight->m_vLightColour.z);
                if (usebumpdiffuse != 0)
                    GLRender::pfuncSetBumpLightColor(vColor, 1.0f);
                else
                    GLRender::pfuncSetBumpLightColor(vFallbackDiffuse, 1.0f);
            }

            float fSpecularIntensity = specularintensity;
            float fSpecBumpIntensity =
                part->m_pMaterial->Textures(4).m_pObject->GetSpecularBumpIntensity();
            Vector vSpecColor(
                vSpec.x * fSpecularIntensity * fSpecBumpIntensity * pLight->m_vLightColour.x,
                vSpec.y * fSpecularIntensity * fSpecBumpIntensity * pLight->m_vLightColour.y,
                vSpec.z * fSpecularIntensity * fSpecBumpIntensity * pLight->m_vLightColour.z);
            if (usebumpspecular != 0)
                GLRender::pfuncSetPixelShaderAmbientColor(vSpecColor, 1.0f);
            else
                GLRender::pfuncSetPixelShaderAmbientColor(vFallbackSpecular, 1.0f);

            GLRender::SetVertexShaderConstant(12, lx, ly, lz, 1.0f);
            GLRender::SetVertexShaderConstant(13, cx, cy, cz, 1.0f);
            float fR = pLight->m_fLightRadius * pLight->m_fLightRadius;
            GLRender::SetVertexShaderConstant(14, 1.0f, 0.0f, 9.0f / (fR + fR), 1.0f);

// The texture-coord client array is bound on unit 1, per light.
            GLRender::ActivateTextureStage(1);
            GLRender::EnableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);

            RenderFlat(vertices);

            GLRender::ActivateTextureStage(1);
            GLRender::DisableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);

            if (bWasAlphaTest != 0)
                GLRender::Enable(Aurora::kCapAlphaTest);

            (currentscene->m_pLightManager)
                ->EnableBumpMapLights();
        }

        currentscene->DoFog();
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
        GLRender::Disable(Aurora::kCapVertexProgram);

        if (showrancoraxes != 0)
        {
            ASLgl::glDisable(GL_LIGHTING);
            ASLgl::glDisable(GL_TEXTURE_2D);
            ASLgl::glDisable(GL_DEPTH_TEST);
            ASLgl::glLineWidth(5.0f);
            ASLgl::glBegin(1);
            ASLgl::glColor3f(1.0f, 0.0f, 0.0f);
            ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
            ASLgl::glVertex3f(5.0f, 0.0f, 0.0f);
            ASLgl::glColor3f(0.0f, 1.0f, 0.0f);
            ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
            ASLgl::glVertex3f(0.0f, 5.0f, 0.0f);
            ASLgl::glColor3f(0.0f, 0.0f, 1.0f);
            ASLgl::glVertex3f(0.0f, 0.0f, 0.0f);
            ASLgl::glVertex3f(0.0f, 0.0f, 5.0f);
            ASLgl::glEnd();
            ASLgl::glEnable(GL_DEPTH_TEST);
            ASLgl::glEnable(GL_LIGHTING);
            ASLgl::glEnable(GL_TEXTURE_2D);
        }
    }
}
enum AuroraRenderPaths : int {
    AURORA_RENDERPATH_EMBM = 0,
    AURORA_RENDERPATH_EMBM_LIGHT,
    AURORA_RENDERPATH_BUMP_SPEC,
    AURORA_RENDERPATH_BUMP_DIFFUSE,
    AURORA_RENDERPATH_BUMP_ONEPASS,
    AURORA_RENDERPATH_ENV,
    AURORA_RENDERPATH_LIGHT,
    AURORA_RENDERPATH_FLAT
};
int dophong;
int phongrender = 1;
char cPhongVP[] = "!!VP1.0 # This is the standard vertex program.\nDP4   o[HPOS].x, c[0], v[OPOS];\nDP4   o[HPOS].y, c[1], v[OPOS];\nDP4   o[HPOS].z, c[2], v[OPOS];\nDP4   o[HPOS].w, c[3], v[OPOS];\nDP4   R10.x, c[8], v[OPOS];\nDP4   R10.y, c[9], v[OPOS];\nDP4   R10.z, c[10], v[OPOS];\nDP4   R10.w, c[11], v[OPOS];\nDP3   R1.w, R10, R10;\nRSQ   R1.w, R1.w;\nRCP   R1.w, R1.w;\nMOV  o[FOGC].x, -R1.w;\nDP3   R11.x, c[8], v[NRML];\nDP3   R11.y, c[9], v[NRML];\nDP3   R11.z, c[10], v[NRML];\nMOV  o[TEX3], v[NRML];\nADD  R0, c[12], -v[OPOS];\nMOV o[TEX2], R0;\nRCP R2, c[14].y;\nMUL R3, R0, R2.y;\nMUL R3, c[15].x, R3;\nADD R3.xy, c[15].y, R3;\nDP3 R4.x, R3, R3;\nMUL R5, R3, c[15].wwyy;\nDP3 R5.x, R5, R5;\nRCP R5.x, R5.x;\nMUL R4.x, R4.x, R5.x;\nMUL R3, R3, R4.x;\nMOV o[TEX1], R3;\nMOV o[TEX0], v[TEX0];\n";
unsigned int* cPhongEMBMVPDeclaration;
unsigned int* cPhongEMBMLightVPDeclaration;
unsigned int* cPhongBumpSpecVPDeclaration;
unsigned int* cPhongBumpDiffuseVPDeclaration;
unsigned int* cPhongBumpOnePassVPDeclaration;
unsigned int* cPhongEnvVPDeclaration;
unsigned int* cPhongLightVPDeclaration;
unsigned int* cPhongFlatVPDeclaration;

// Renders the Phong pass, picking one of eight vertex programs from the render path.
void RenderPhong(VertexPrimitiveFlat* vertices, AuroraRenderPaths nRenderPath)
{
    if (dophong == 0)
        return;

    if (currentscene == 0)
    {
        RenderFlat(vertices);
        return;
    }

    static VertexProgram* phongEMBMVP = new VertexProgram(cPhongVP, cPhongEMBMVPDeclaration);
    static VertexProgram* phongEMBMLightVP = new VertexProgram(cPhongVP, cPhongEMBMLightVPDeclaration);
    static VertexProgram* phongBumpSpecVP = new VertexProgram(cPhongVP, cPhongBumpSpecVPDeclaration);
    static VertexProgram* phongBumpDiffuseVP = new VertexProgram(cPhongVP, cPhongBumpDiffuseVPDeclaration);
    static VertexProgram* phongBumpOnePassVP = new VertexProgram(cPhongVP, cPhongBumpOnePassVPDeclaration);
    static VertexProgram* phongEnvVP = new VertexProgram(cPhongVP, cPhongEnvVPDeclaration);
    static VertexProgram* phongLightVP = new VertexProgram(cPhongVP, cPhongLightVPDeclaration);
    static VertexProgram* phongFlatVP = new VertexProgram(cPhongVP, cPhongFlatVPDeclaration);
    static VertexProgram** const apPhongVP[8] = {
        &phongEMBMVP, &phongEMBMLightVP, &phongBumpSpecVP, &phongBumpDiffuseVP,
        &phongBumpOnePassVP, &phongEnvVP, &phongLightVP, &phongFlatVP
    };

    if (vertices->m_pPart->m_pGob != 0)
    {
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        return;
    }

    if (static_cast<unsigned int>(nRenderPath) > 7)
        return;

    (*apPhongVP[nRenderPath])->Enable(vertices->m_pPart, false);

    List<PartLight*> phongLights(
        (currentscene->m_pLightManager)
            ->GetPhongLights());

    GLRender::Enable(Aurora::kCapBlend);
    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendOne);

    Quaternion qOrient;
    Vector vWorld;
    vertices->m_pPart->GetWorld(vWorld, qOrient);

    GLRender::Enable(Aurora::kCapVertexProgram);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSPhong, 0,
        currentscene->m_pNormalizationCubeMap.m_pObject,
        currentscene->m_pNormalizationCubeMap.m_pObject,
        vertices->m_pPart->m_pMaterial->Textures(0).m_pObject);

    for (int k = 0; k < phongLights.m_count; k++)
    {
#if KOTOR1  // K1: the forward rotate through the out-of-line inverse
        Vector vLightPos = phongLights.m_pData[k]->m_vWorldPosition;
        Vector vLocal = WorldToLocal(vLightPos, qOrient, vWorld);
        float lx = vLocal.x, ly = vLocal.y, lz = vLocal.z;
#else
        float lx = phongLights.m_pData[k]->m_vWorldPosition.x - vWorld.x;
        float ly = phongLights.m_pData[k]->m_vWorldPosition.y - vWorld.y;
        float lz = phongLights.m_pData[k]->m_vWorldPosition.z - vWorld.z;
        {
// Rotate by the inverse orientation, with the identity fast-out.
            float qw = qOrient.w;
            float qx = qOrient.x;
            float qy = qOrient.y;
            float qz = qOrient.z;
            float nw = -qw;
            if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f))
            {
                float xx = qx * qx;
                float yy = qy * qy;
                float zz = qz * qz;
                float xy = qx * qy;
                float xz = qx * qz;
                float yz = qy * qz;
                float xnw = qx * nw;
                float ynw = qy * nw;
                float znw = qz * nw;
                float t1 = yy + zz;
                float t2 = xx + zz;
                float t3 = xy - znw;
                float t4 = xy + znw;
                float t5 = ynw + xz;
                float t6 = xz - ynw;
                float t7 = yz - xnw;
                float t8 = xnw + yz;
                float t9 = xx + yy;
                float nlX = (1.0f - (t1 + t1)) * lx + (t3 + t3) * ly + (t5 + t5) * lz;
                float nlY = (t4 + t4) * lx + (1.0f - (t2 + t2)) * ly + (t7 + t7) * lz;
                float nlZ = (t6 + t6) * lx + (t8 + t8) * ly + (1.0f - (t9 + t9)) * lz;
                lx = nlX;
                ly = nlY;
                lz = nlZ;
            }
        }
#endif

        vertices->m_pPart->m_pNode->AsMdlNodeTriMesh();

        GLRender::SetVertexShaderConstant(12, lx, ly, lz, 1.0f);
        GLRender::SetVertexShaderConstant(13, 0.0f, 1.0f, 2.0f, -1.0f);
        GLRender::SetVertexShaderConstant(14, 1.0f, phongLights.m_pData[k]->m_fLightRadius, 4.5f, 0.0f);
        GLRender::SetVertexShaderConstant(15, 0.125f, 0.5f, 0.0f, 1.0f);

        if (phongrender != 0)
            RenderFlat(vertices);
    }

    GLRender::Disable(Aurora::kCapVertexProgram);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
}

extern char cHoloVPARB[];

// Renders the hologram pass, picking a vertex program from the render path, then drawing flat.
void RenderHologramEffect(int nRenderPath, VertexPrimitiveFlat* vertices)
{
    PartTriMesh* part = vertices->m_pPart->AsPartTriMesh();

    GLRender::Enable(Aurora::kCapVertexProgram);

    VertexProgram* pVP = 0;
    switch (nRenderPath)
    {
    case PartTriMesh::RENDER_PATH_FLAT: case PartTriMesh::RENDER_PATH_FLAT_VP: case PartTriMesh::RENDER_PATH_ENVIRONMENT_MAPPED: case PartTriMesh::RENDER_PATH_ENVIRONMENT_MAPPED_VP:
        {
            static VertexProgram* vp =
                new VertexProgram(cHoloVPARB, cStandardVPDeclaration);
            pVP = vp;
        }
        break;
    case PartTriMesh::RENDER_PATH_BUMPMAPPED: case PartTriMesh::RENDER_PATH_BUMPMAP_FALLBACK:
        {
            static VertexProgram* vp =
                new VertexProgram(cHoloVPARB, cBumpMapOnePassVPDeclaration);
            pVP = vp;
        }
        break;
    case PartTriMesh::RENDER_PATH_LIGHTMAPPED_BUMP_DIFFUSE:
        {
            static VertexProgram* vp =
                new VertexProgram(cHoloVPARB, cBumpMapDiffuseVPDeclaration);
            pVP = vp;
        }
        break;
    case PartTriMesh::RENDER_PATH_EMBM:
        {
            static VertexProgram* vp =
                new VertexProgram(cHoloVPARB, cBumpyShinyRegularVPDeclaration);
            pVP = vp;
        }
        break;
    case PartTriMesh::RENDER_PATH_NORMAL_MAPPED:
        {
            static VertexProgram* vp =
                new VertexProgram(cHoloVPARB, szNormalmapVPARBDeclaration);
            pVP = vp;
        }
        break;
    case PartTriMesh::RENDER_PATH_LIGHTMAPPED_BUMP_SPECULAR:
        {
            static VertexProgram* vp =
                new VertexProgram(cHoloVPARB, cBumpMapSpecularVPDeclaration);
            pVP = vp;
        }
        break;
    default:
        break;
    }

    if (pVP != 0)
    {
        pVP->Enable(part, false);

        if (g_ForceSight != 0)
        {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSHoloDesat, 0, 0, 0, 0);
            GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendOne);
        }
        else
        {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSHolo, 0, 0, 0, 0);
            GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendSrcAlpha);
        }
        RenderFlat(vertices);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSHoloOff, 0, 0, 0, 0);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    }

    GLRender::Disable(Aurora::kCapVertexProgram);
}

