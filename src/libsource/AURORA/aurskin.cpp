// Skinned mesh rendering, the RenderSkinned transform/render family and the PartSkin draw helpers.

#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1: List.h grows from 8
#endif
#include "aurprimitive.h"
#include "AURORA/aurmodel.h"
#include "List.h"
#include "aurmaterial.h"
#include "aurtexman.h"
#include "aurlightmanager.h"


#include "aurvertexprog.h"

float g_DesatureFactor = 0.649999976f;

extern char cBumpyShinyVP[];
extern char cBumpMapOnePassVP[];
extern char cBumpMapSpecularVP[];
extern char cBumpMapDiffuseVP[];
extern char cBumpMapVP[];
extern char cStandardVP[];
#if !KOTOR1
extern char cStandardDirtyVP[];
extern char cStandardDirtyLMVP[];
extern char cStandardDirtyEMVP[];
extern char cStandardDirtyEMLMVP[];
extern char cStandardDirtyLSVP[];
extern char cStandardDirtyLSLMVP[];
extern char cStandardDirtyLSEMVP[];
extern char cStandardDirtyLSEMLMVP[];
#endif
extern char cFogOutputVP[];
extern char cSkinBumpedOutVP[];
extern char cBumpedOutVP[];
VertexProgram* g_pBumpyShinySkinPass2;
VertexProgram* g_pBumpyShinySkinPass2_3bone;
extern unsigned int* cSkinBumpedOutVPDeclaration;

extern char cSkinMotionBlurVP[];
extern unsigned int* cSkinMotionBlurVPDeclaration;

extern unsigned char g_useDistortionOutput;
extern float DeltaT;
#include "aurcamera.h"
#include "aurscene.h"
#include "Plane.h"
#include "aurglcore.h"
extern Camera* CurrentCamera;

extern int usearbvertexprograms;
class Scene;
extern unsigned char g_ForceSight;
unsigned char g_ForceSight;

// Gates and per-light intensity knobs shared by the vertex-program render paths.
extern int enablebumpyshiny;
extern int enablebumpmap;
int usenewbumpyskin = 1;
int use3boneskinning;
extern int enabledecal;
extern float diffuseintensity;
extern float specularintensity;
int usebumpdiffuse = 1;
int usebumpspecular = 1;
VertexProgram* g_pBumpyShinySkinPass1;
int AurVertexProgramsAvailable();

// The shadow pass draws nothing for a skinned mesh yet; kept so its dispatcher stays complete.
void RenderShadowSkinnedVariant(VertexPrimitiveSkinned* vertices)
{
    K2_UNUSED(vertices);
}

#include "Matrix.h"

// Port GL translation tables the render tail indexes.
extern unsigned int clientStateTranslationTable[];
extern unsigned int enableTranslationTable[];
extern unsigned int blendTranslationTable[];
extern const unsigned int cullTranslationTable[];
// Render dispatchers owned by other units; the transform tail routes through them.
void RenderFlat(VertexPrimitiveFlat* vertices);
void RenderEnvironmentMappedVP(VertexPrimitive* vertices, PartTriMesh* part);
void RenderFlatVP(VertexPrimitive* vertices, PartTriMesh* part);

// The bone walks exclusive-or this flag into the index-list counter.
extern bool gHasRevComps_Mac;

// Identity-quaternion fast-out shared with the other engine draw paths.
extern int quaternionidentityoptimization;

#if KOTOR1
// K1 rotates with the quaternion-inverse operator instead.
static inline Vector RotateVector(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
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
#endif

// Recursive bone-frame accumulator; its two struct arguments are passed by value last.
void GatherBones(Part* part, List<Vector>& outVectors, List<Quaternion>& outQuats,
                 Vector pos, Quaternion orient);

// Vendor capability probes.
int  AurMultiTextureAvailable();
bool AurNV1xLevelShadersAvailable();
bool AurATIValidate();
bool AurNV2xLevelShadersAvailable();
int  AurVertexProgramsNVAvailable();
int  AurBumpyShinyAvailable();
bool AurBumpMapCardValidate();
bool AuroraUseARBShaders();
VertexProgram* GetSkinVP(bool);
void RenderEnvironmentMapped(VertexPrimitiveFlat* vertices, bool flag, bool ati);

void SetupForceSightShader(Gob* gob, int a, int b, CAurTexture* c1, CAurTexture* c2,
                           CAurTexture* c3, CAurTexture* c4, bool flag);
extern char cHoloSkinVPARB[];
extern unsigned int* cBumpyShinySkinVPDeclaration;
extern unsigned int* cSkinBumpyVPDeclaration;
extern unsigned int* szNormalmapVPARBDeclaration;
extern char cNewSkinBumpyVP[];
extern char cSkinBumpyVP[];
extern char szSkinBumpyWSVPARB[];
extern char cSkinCubeMapVP[];
extern char szSkinCubeMapVPARB[];
extern unsigned int* cSkinCubeMapVPDeclaration;
extern unsigned int* cSkinSphereMapVPDeclaration;
extern char cSkin3BoneVP[];
extern unsigned int* cSkin3BoneVPDeclaration;
extern char cSkinVP[];
extern char szSkinVPARB[];
extern char szSkinBumpyVPARB[];
extern char szNewSkinBumpyVPARB[];
extern char szSkinFogVPARB[];
extern char cBumpyShinySkinVP[];
char cSkinSphereMapVP[] = "!!VP1.0 # This is the vertex program for skinned animations.\nMUL   R0, v[COL1], c[16].zzzz;ARL   A0.x, R0.x;DP4   R1.x, v[OPOS], c[A0.x + 18];DP4   R1.y, v[OPOS], c[A0.x + 19];DP4   R1.z, v[OPOS], c[A0.x + 20];DP3   R5.x, v[NRML], c[A0.x + 18];DP3   R5.y, v[NRML], c[A0.x + 19];DP3   R5.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.y;DP4   R2.x, v[OPOS], c[A0.x + 18];DP4   R2.y, v[OPOS], c[A0.x + 19];DP4   R2.z, v[OPOS], c[A0.x + 20];DP3   R6.x, v[NRML], c[A0.x + 18];DP3   R6.y, v[NRML], c[A0.x + 19];DP3   R6.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.z;DP4   R3.x, v[OPOS], c[A0.x + 18];DP4   R3.y, v[OPOS], c[A0.x + 19];DP4   R3.z, v[OPOS], c[A0.x + 20];DP3   R7.x, v[NRML], c[A0.x + 18];DP3   R7.y, v[NRML], c[A0.x + 19];DP3   R7.z, v[NRML], c[A0.x + 20];ARL   A0.x, R0.w;DP4   R4.x, v[OPOS], c[A0.x + 18];DP4   R4.y, v[OPOS], c[A0.x + 19];DP4   R4.z, v[OPOS], c[A0.x + 20];DP3   R8.x, v[NRML], c[A0.x + 18];DP3   R8.y, v[NRML], c[A0.x + 19];DP3   R8.z, v[NRML], c[A0.x + 20];MUL   R9, v[WGHT].x, R1;MAD   R9, R2, v[WGHT].y, R9;MAD   R9, R3, v[WGHT].z, R9;MAD   R9, R4, v[WGHT].w, R9;MOV   R9.w, c[16].w;MUL   R10, v[WGHT].x, R5;MAD   R10, R6, v[WGHT].y, R10;MAD   R10, R7, v[WGHT].z, R10;MAD   R10, R8, v[WGHT].w, R10;DP4   o[HPOS].x, c[0], R9;DP4   o[HPOS].y, c[1], R9;DP4   o[HPOS].z, c[2], R9;DP4   o[HPOS].w, c[3], R9;DP3   R11.x, c[8], R10;DP3   R11.y, c[9], R10;DP3   R11.z, c[10], R10;MOV   R11.w, c[16].w;DP3   R11.w, R11, R11;RSQ   R11.w, R11.w;MUL   R11, R11, R11.w;DP4   R10.x, c[8], R9;DP4   R10.y, c[9], R9;DP4   R10.z, c[10], R9;MOV   R10.w, c[16].w;MOV   R9, c[16].xxxw;MOV   o[TEX0], v[TEX0];DP3   R1.w, R10, R10;RSQ   R1.w, R1.w;RCP   R1.w, R1.w;MOV o[FOGC].x, R1.w;DP3   R0.x, -R10, R11;MUL   R1, R0.x, R11;MUL   R2, R1, c[16].y;ADD   R4, R10, R2;ADD   R4.z, R4.z, c[16].w;DP3   R5.x, R4, R4;RSQ   R5.y, R5.x;RCP   R5.z, R5.y;MUL   R5.w, R5.z, c[16].y;RCP   R5.x, R5.w;MOV   R6.xyzw, c[16].xxxw;MAD   R6.x, R3.x, R5.x, c[17].w;MAD   R6.y, R3.y, R5.x, c[17].w;MOV\to[TEX1], R6;";
extern unsigned int* cSkinVPDeclaration;
class CAurTexture;

void ExecuteSkinTransforms(MdlNodeSkin* node, List<Vector>& inPositions, List<Matrix>& inMatrices,
                           VertexPrimitiveSkinned* vp, bool flag);

// Skins every vertex of the node with up to four bone contributions across two bone phases, then
// dispatches the bump, environment-mapped or flat render path.
void ExecuteSkinTransforms(MdlNodeSkin* node, List<Vector>& inPositions, List<Matrix>& inMatrices,
                           VertexPrimitiveSkinned* vp, bool flag)
{
    static unsigned int sVertexCount = static_cast<short>(node->m_nVertexCount);
    static unsigned int sPoolPositions =
        GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0), sVertexCount * 12, 0x42, 0,
                              &sPoolPositions);
    static unsigned int sPoolNormals =
        GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0), sVertexCount * 12, 0x42, 0,
                              &sPoolNormals);

    int nCount = static_cast<short>(node->m_nVertexCount);

    if (sVertexCount < static_cast<unsigned int>(nCount)) {
        sVertexCount = nCount;
        GLRender::ResizePool(sPoolPositions, sVertexCount * 12);
        GLRender::ResizePool(sPoolNormals, sVertexCount * 12);
    }
    Vector* pPositions = static_cast<Vector*>(GLRender::LockPool(sPoolPositions, 0, 0, 1));
    Vector* pNormals = static_cast<Vector*>(GLRender::LockPool(sPoolNormals, 0, 0, 1));
    unsigned char* pVertexBase = static_cast<unsigned char*>(GLRender::LockPool(node->m_pTree->AsModel()->m_nPool, 0, 0, 0x10));

    Vector vPos = {0.0f, 0.0f, 0.0f};
    Vector vNorm = {0.0f, 0.0f, 0.0f};

    for (int nBone = 0; nBone < static_cast<MdlNodeTriMesh*>(vp->m_pNode)->m_lIndexPools.m_count; nBone++) {
        const unsigned int nBonePool = static_cast<MdlNodeTriMesh*>(vp->m_pNode)->m_lIndexPools.m_pData[nBone];
        unsigned char* pBone = static_cast<unsigned char*>(GLRender::LockPool(nBonePool, 0, 0, 8));
        if (static_cast<MdlNodeTriMesh*>(vp->m_pNode)->m_lVertexIndicesCount.m_pData[nBone] >= 1) {
            for (int nVert = 0; nVert < static_cast<MdlNodeTriMesh*>(vp->m_pNode)->m_lVertexIndicesCount.m_pData[nBone]; nVert++) {
#if KOTOR1  // K1 has no reversed-component index flip
                const unsigned short idx = reinterpret_cast<unsigned short*>(pBone)[nVert];
#else
                const unsigned short idx = reinterpret_cast<unsigned short*>(pBone)[nVert ^ static_cast<int>(gHasRevComps_Mac)];
#endif
                unsigned char* pBase = pVertexBase + static_cast<int64_t>(node->m_nVertexStride) * idx;
                vPos = *reinterpret_cast<Vector*>(pBase + node->m_nVertexOfs + node->m_nGeomBase);
                vNorm = *reinterpret_cast<Vector*>(pBase + node->m_nNormalOfs + node->m_nGeomBase);
                Vector vAccumPos = {0.0f, 0.0f, 0.0f};
                Vector vAccumNorm = {0.0f, 0.0f, 0.0f};
                for (int nComp = 0; nComp < 4; nComp++) {
                    const float weight = *reinterpret_cast<float*>(pVertexBase + node->m_nGeomBase + static_cast<int64_t>(node->m_nVertexStride) * idx +
                                                  node->m_nBoneWeightsOfs + nComp * 4);
                    if (weight == 0.0f)
                        continue;
                    const int nBoneIdx = node->m_aBoneNodeIndices[static_cast<unsigned int>(*reinterpret_cast<float*>(pVertexBase +
                                        node->m_nGeomBase + static_cast<int64_t>(node->m_nVertexStride) * idx + node->m_nBoneIndicesOfs + nComp * 4))];
                    vAccumPos = vAccumPos + (inMatrices.m_pData[nBoneIdx] * vPos + inPositions.m_pData[nBoneIdx]) * weight;
                    vAccumNorm = vAccumNorm + (inMatrices.m_pData[nBoneIdx] * vNorm) * weight;
                }
                pPositions[idx] = vAccumPos;
                pNormals[idx] = vAccumNorm;
            }
        }
        GLRender::UnlockPool(nBonePool);
    }

    unsigned char* pBonePhase = static_cast<unsigned char*>(GLRender::LockPool(static_cast<MdlNodeTriMesh*>(vp->m_pNode)->m_nLeftOverPool, 0, 0, 8));
    if (static_cast<MdlNodeTriMesh*>(vp->m_pNode)->m_nLeftOverIndexCount >= 1) {
        for (int nBone2 = 0; nBone2 < static_cast<MdlNodeTriMesh*>(vp->m_pNode)->m_nLeftOverIndexCount; nBone2++) {
#if KOTOR1
            const unsigned short idx = reinterpret_cast<unsigned short*>(pBonePhase)[nBone2];
#else
            const unsigned short idx = reinterpret_cast<unsigned short*>(pBonePhase)[nBone2 ^ static_cast<int>(gHasRevComps_Mac)];
#endif
            unsigned char* pBase = pVertexBase + static_cast<int64_t>(node->m_nVertexStride) * idx;
            vPos = *reinterpret_cast<Vector*>(pBase + node->m_nVertexOfs + node->m_nGeomBase);
            vNorm = *reinterpret_cast<Vector*>(pBase + node->m_nNormalOfs + node->m_nGeomBase);
            Vector vAccumPos = {0.0f, 0.0f, 0.0f};
            Vector vAccumNorm = {0.0f, 0.0f, 0.0f};
            for (int nComp = 0; nComp < 4; nComp++) {
                const float weight = *reinterpret_cast<float*>(pVertexBase + node->m_nGeomBase + static_cast<int64_t>(node->m_nVertexStride) * idx +
                                              node->m_nBoneWeightsOfs + nComp * 4);
                if (weight == 0.0f)
                    continue;
                const int nBoneIdx = node->m_aBoneNodeIndices[*reinterpret_cast<unsigned short*>(pVertexBase +
                                    node->m_nGeomBase + static_cast<int64_t>(node->m_nVertexStride) * idx + node->m_nBoneIndicesOfs + nComp * 2)];
                vAccumPos = vAccumPos + (inMatrices.m_pData[nBoneIdx] * vPos + inPositions.m_pData[nBoneIdx]) * weight;
                vAccumNorm = vAccumNorm + (inMatrices.m_pData[nBoneIdx] * vNorm) * weight;
            }
            if (flag)
                vAccumPos = vAccumPos + vAccumNorm * vp->m_pPart->m_pGob->m_fBumpOutDistance;
            pPositions[idx] = vAccumPos;
            pNormals[idx] = vAccumNorm;
        }
    }
    GLRender::UnlockPool(static_cast<MdlNodeTriMesh*>(vp->m_pNode)->m_nLeftOverPool);
    GLRender::UnlockPool(sPoolPositions);
    GLRender::UnlockPool(sPoolNormals);
    GLRender::UnlockPool(node->m_pTree->AsModel()->m_nPool);

    // Render tail, with the client state, texture-coordinate strategy and skin dispatch.
    if (AurMultiTextureAvailable())
        ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
    GLRender::cm_uiCurrentStage = 0;
    if (static_cast<short>(node->m_nTexCoordSets) > 0)
        ASLgl::glEnableClientState(clientStateTranslationTable[0]);
    else
        ASLgl::glDisableClientState(clientStateTranslationTable[0]);

    switch (static_cast<short>(node->m_nTexCoordSets) - 1) {
        case 3:
            if (AurMultiTextureAvailable())
                ASLgl::glClientActiveTextureARB(GL_TEXTURE3);
            GLRender::cm_uiCurrentStage = 3;
            GLRender::SetTexCoordBuffer(node->m_nVertexStride, node->m_nTex3Ofs + node->m_nGeomBase,
                                        node->m_pTree->AsModel()->m_nPool, static_cast<short>(node->m_nVertexCount));
            // fallthrough
        case 2:
            if (AurMultiTextureAvailable())
                ASLgl::glClientActiveTextureARB(GL_TEXTURE2);
            GLRender::cm_uiCurrentStage = 2;
            GLRender::SetTexCoordBuffer(node->m_nVertexStride, node->m_nTex2Ofs + node->m_nGeomBase,
                                        node->m_pTree->AsModel()->m_nPool, static_cast<short>(node->m_nVertexCount));
            // fallthrough
        case 1:
            if (AurMultiTextureAvailable())
                ASLgl::glClientActiveTextureARB(GL_TEXTURE1);
            GLRender::cm_uiCurrentStage = 1;
            GLRender::SetTexCoordBuffer(node->m_nVertexStride, node->m_nTex1Ofs + node->m_nGeomBase,
                                        node->m_pTree->AsModel()->m_nPool, static_cast<short>(node->m_nVertexCount));
            // fallthrough
        case 0:
            if (AurMultiTextureAvailable())
                ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
            GLRender::cm_uiCurrentStage = 0;
            GLRender::SetTexCoordBuffer(node->m_nVertexStride, node->m_nTex0Ofs + node->m_nGeomBase,
                                        node->m_pTree->AsModel()->m_nPool, static_cast<short>(node->m_nVertexCount));
            break;
    }
    GLRender::SetNormalBuffer(12, 0, sPoolNormals, static_cast<short>(node->m_nVertexCount));
    GLRender::SetVertexBuffer(12, 0, sPoolPositions, static_cast<short>(node->m_nVertexCount));

    PartSkin* skin = vp->m_pPart->AsPartSkin();
    if (flag) {
        // Bumped-out shell path.
        GLRender::cm_uiCurrentStage = 0;
        if (AurMultiTextureAvailable())
            ASLgl::glActiveTextureARB(GLRender::cm_uiCurrentStage + GL_TEXTURE0);
        skin->m_pMaterial->Textures(3).m_pObject->Bind();
        if (skin->m_pMaterial->m_lstSrcBlend.m_count >= 2)
            skin->m_pMaterial->SetBlendingMode(1);
        RenderFlat(vp);
        skin->m_pMaterial->BindTexture0();
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
    } else if ((skin->m_pGob != 0 && skin->m_pGob->m_texWatch.m_pObject != 0) ||
               skin->m_pMaterial->HasEnvMap()) {
#if KOTOR1  // K1 draws every skin pass through the fixed-function renderers
        RenderEnvironmentMapped(vp, false, false);
#else
        RenderEnvironmentMappedVP(vp, skin->AsPartTriMesh());
#endif
    } else if (skin->m_pMaterial->HasBumpMap() && node->HasTangentBases()) {
        if (AurNV1xLevelShadersAvailable() && !AurATIValidate()) {
            // ATI pixel-shader bump pass.
            GLRender::pfuncEnablePixelShaders(GLRender::kPSSkinBump, 0, 0, 0, 0);
#if KOTOR1
            RenderFlat(vp);
#else
            RenderFlatVP(vp, skin->AsPartTriMesh());
#endif
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
        } else {
            // Fixed-function bump pass, with the blend pair swapped around the flat pass.
            ASLgl::glDisable(enableTranslationTable[1]);
            ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[3]);
#if KOTOR1
            RenderFlat(vp);
#else
            RenderFlatVP(vp, skin->AsPartTriMesh());
#endif
            ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
            ASLgl::glEnable(enableTranslationTable[1]);
        }
    } else {
        if (!AurNV1xLevelShadersAvailable() && !AurNV2xLevelShadersAvailable() &&
            AurMultiTextureAvailable() && AurVertexProgramsNVAvailable()) {
            // NV2x vertex-program shell pass.
            GLRender::cm_uiCurrentStage = 1;
            if (AurMultiTextureAvailable())
                ASLgl::glActiveTextureARB(GLRender::cm_uiCurrentStage + GL_TEXTURE0);
            ASLgl::glEnable(enableTranslationTable[4]);
            ASLgl::glBindTexture(enableTranslationTable[4], 0);
#if !KOTOR1  // K1 sets no anisotropy here
            ASLgl::glTexParameteri(enableTranslationTable[4], GL_TEXTURE_MAX_ANISOTROPY_EXT, GLRender::cm_nAnisotropy);
#endif
            GLRender::cm_nGLCurrentTID[GLRender::cm_uiCurrentStage] = 0;
            GLRender::cm_uiCurrentStage = 0;
            if (AurMultiTextureAvailable())
                ASLgl::glActiveTextureARB(GLRender::cm_uiCurrentStage + GL_TEXTURE0);
        }
#if KOTOR1
        RenderFlat(vp);
#else
        RenderFlatVP(vp, skin->AsPartTriMesh());
#endif
    }
}

#if KOTOR1
#include <map>
#include <vector>

// K1 port-layer cache of GPU-skinned buffers, one SkinnedParts per tri-mesh node.
struct SkinnedPart {
    std::vector<float> m_vertices;
    std::vector<unsigned short> m_indices;
    unsigned char m_abReserved1[88];
    unsigned int m_nVertexBuffer;
    unsigned int m_nIndexBuffer;

    ~SkinnedPart()
    {
        if (m_nVertexBuffer)
            android_port_glDeleteBuffers(1, &m_nVertexBuffer);
        if (m_nIndexBuffer)
            android_port_glDeleteBuffers(1, &m_nIndexBuffer);
    }
};

struct SkinnedParts {
    SkinnedPart m_parts[16];
};

static std::map<MdlNodeTriMesh*, SkinnedParts*> s_skinnedParts;

void IOS_MdlNodeTriMeshDestroyed(MdlNodeTriMesh* pNode)
{
    std::map<MdlNodeTriMesh*, SkinnedParts*>::iterator it = s_skinnedParts.find(pNode);
    if (it != s_skinnedParts.end()) {
        delete it->second;
        s_skinnedParts.erase(it);
    }
}
#endif

// Gathers the part's bone frames, transforms each one, and hands the result to the shared skin
// render tail (ExecuteSkinTransforms).
void RenderSkinned(VertexPrimitiveSkinned* vertices, bool flag)
{
#if KOTOR1  // K1 reads the part and node before the statics' guards
    Part* part = vertices->m_pPart;
    MdlNodeSkin* node = static_cast<MdlNodeSkin*>(part->m_pNode);
#endif
    static List<Quaternion> qbones;
    static List<Matrix> rbones;
    static List<Vector> tbones;

    qbones.m_count = 0;
    rbones.m_count = 0;
    tbones.m_count = 0;

#if !KOTOR1
    Part* part = vertices->m_pPart;
    MdlNodeSkin* node = static_cast<MdlNodeSkin*>(part->m_pNode);
#endif

    // Gather the world frame: the part's own frame, then up the parent chain.
    Part* pTop = part;
#if KOTOR1  // the same walk with the quaternion-inverse operator
    Quaternion qInv = -part->m_qLocalOrientation;
    Vector vPos = RotateVector(-part->m_vLocalPosition, qInv);
    Quaternion qAcc = -part->m_qLocalOrientation;
    for (Part* pParent = static_cast<Part*>(part->m_pParent); pParent != 0; pParent = static_cast<Part*>(pParent->m_pParent)) {
        pTop = pParent;
        Quaternion qParentInv = -pParent->m_qLocalOrientation;
        Vector p = RotateVector(-pParent->m_vLocalPosition, qParentInv);
        vPos = vPos + RotateVector(p, qAcc);
        qAcc = qAcc * -pParent->m_qLocalOrientation;
    }
    GatherBones(pTop, tbones, qbones, vPos, qAcc);
#else
    float px, py, pz;               // accumulated translation (world from local, transposed rotation)
    float fX, fY, fZ, fW;           // accumulated quaternion (w component stored negated)
    {
        float qw = part->m_qLocalOrientation.w;
        float qx = part->m_qLocalOrientation.x;
        float qy = part->m_qLocalOrientation.y;
        float qz = part->m_qLocalOrientation.z;
        float nw = -qw;
        fW = nw; fX = qx; fY = qy; fZ = qz;
        px = -part->m_vLocalPosition.x;
        py = -part->m_vLocalPosition.y;
        pz = -part->m_vLocalPosition.z;
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
            t4 = (t4 + t4) * px;
            t6 = (t6 + t6) * px;
            t8 = (t8 + t8) * py;
            px = (1.0f - (t1 + t1)) * px + (t3 + t3) * py + (t5 + t5) * pz;
            py = t4 + (1.0f - (t2 + t2)) * py + (t7 + t7) * pz;
            pz = t6 + t8 + (1.0f - (t9 + t9)) * pz;
        }
    }
    Part* pParent = static_cast<Part*>(part->m_pParent);
    while (pParent != 0) {
        Part* pCur = pParent;
        float qw = pCur->m_qLocalOrientation.w;
        float qx = pCur->m_qLocalOrientation.x;
        float qy = pCur->m_qLocalOrientation.y;
        float qz = pCur->m_qLocalOrientation.z;
        float nw = -qw;
        float mx = -pCur->m_vLocalPosition.x;
        float my = -pCur->m_vLocalPosition.y;
        float mz = -pCur->m_vLocalPosition.z;
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
            t4 = (t4 + t4) * mx;
            t6 = (t6 + t6) * mx;
            t8 = (t8 + t8) * my;
            mx = (1.0f - (t1 + t1)) * mx + (t3 + t3) * my + (t5 + t5) * mz;
            my = t4 + (1.0f - (t2 + t2)) * my + (t7 + t7) * mz;
            mz = t6 + t8 + (1.0f - (t9 + t9)) * mz;
        }
        if (quaternionidentityoptimization == 0 || (fW != -1.0f && fW != 1.0f)) {
            float t1 = fY * fY + fZ * fZ;
            float t2 = fX * fX + fZ * fZ;
            float t3 = fX * fY - fW * fZ;
            float t4 = fX * fY + fW * fZ;
            float t5 = fW * fY + fX * fZ;
            float t6 = fX * fZ - fW * fY;
            float t7 = fY * fZ - fW * fX;
            float t8 = fW * fX + fY * fZ;
            float t9 = fX * fX + fY * fY;
            t4 = (t4 + t4) * mx;
            t6 = (t6 + t6) * mx;
            t8 = (t8 + t8) * my;
            mx = (1.0f - (t1 + t1)) * mx + (t3 + t3) * my + (t5 + t5) * mz;
            my = t4 + (1.0f - (t2 + t2)) * my + (t7 + t7) * mz;
            mz = t6 + t8 + (1.0f - (t9 + t9)) * mz;
        }
        px += mx;
        py += my;
        pz += mz;
        float aw = fW, ax = fX, ay = fY, az = fZ;
        float u1 = aw * qx;
        float u2 = aw * qy;
        float u3 = ax * qz;
        float u4 = aw * qz;
        float u5 = ax * qy;
        float u6 = ay * qx;
        fW = aw * nw - ax * qx - ay * qy - az * qz;
        fX = ax * nw + u1 + ay * qz - az * qy;
        fY = (u2 - u3) + ay * nw + az * qx;
        fZ = az * nw + ((u5 + u4) - u6);
        pTop = pCur;
        pParent = static_cast<Part*>(pCur->m_pParent);
    }
    GatherBones(pTop, tbones, qbones, Vector(px, py, pz), Quaternion(fW, fX, fY, fZ));
#endif

    // For each bone, rotate the bone position by the gathered frame, merge it into the
    // frame list and append the resulting rotation matrix.
    if (0 < tbones.m_count) {
        for (int64_t i = 0; i < tbones.m_count; i++) {
            Quaternion q = qbones.m_pData[i];
            Vector src = node->m_lTBoneRefInv.m_pData[i];
            Vector r;
            if (quaternionidentityoptimization == 0 || (q.w != -1.0f && q.w != 1.0f)) {
                float w = q.w, x = q.x, y = q.y, z = q.z;
                float t1 = y * y + z * z;
                float t2 = x * x + z * z;
                float t3 = x * y - w * z;
                float t4 = x * y + w * z;
                float t5 = w * y + x * z;
                float t6 = x * z - w * y;
                float t7 = y * z - w * x;
                float t8 = w * x + y * z;
                float t9 = x * x + y * y;
                r.x = (1.0f - (t1 + t1)) * src.x + (t3 + t3) * src.y + (t5 + t5) * src.z;
                r.y = (t4 + t4) * src.x + (1.0f - (t2 + t2)) * src.y + (t7 + t7) * src.z;
                r.z = (t6 + t6) * src.x + (t8 + t8) * src.y + (1.0f - (t9 + t9)) * src.z;
            } else {
                r = src;
            }
            tbones.m_pData[i] = tbones.m_pData[i] + r;
            qbones.m_pData[i] = qbones.m_pData[i] * node->m_lQBoneRefInv.m_pData[i];
            Quaternion q2 = qbones.m_pData[i];
            float w2 = q2.w, x2 = q2.x, y2 = q2.y, z2 = q2.z;
            Matrix m;
            m.m_f[0] = 1.0f - ((y2 * y2 + z2 * z2) + (y2 * y2 + z2 * z2));
            m.m_f[1] = (y2 * x2 + z2 * w2) + (y2 * x2 + z2 * w2);
            m.m_f[2] = (z2 * x2 - y2 * w2) + (z2 * x2 - y2 * w2);
            m.m_f[3] = (y2 * x2 - z2 * w2) + (y2 * x2 - z2 * w2);
            m.m_f[4] = 1.0f - ((z2 * z2 + x2 * x2) + (z2 * z2 + x2 * x2));
            m.m_f[5] = (y2 * z2 + x2 * w2) + (y2 * z2 + x2 * w2);
            m.m_f[6] = (z2 * x2 + y2 * w2) + (z2 * x2 + y2 * w2);
            m.m_f[7] = (y2 * z2 - x2 * w2) + (y2 * z2 - x2 * w2);
            m.m_f[8] = 1.0f - ((y2 * y2 + x2 * x2) + (y2 * y2 + x2 * x2));
            rbones.Add(m);
        }
    }

    static Vector coord;
    static Vector normal;
    ExecuteSkinTransforms(static_cast<MdlNodeSkin*>(node), tbones, rbones, vertices, flag);
}

// The file-scope skin vertex-program singletons, built on first use.
static VertexProgram* skinvp;
static VertexProgram* holoskinvp;
static VertexProgram* bumpyshinyskinvp;
static VertexProgram* skinspheremapvp;
static VertexProgram* skincubemapvp;

// Both programs take the plain skin blob as their non-ARB fallback.
VertexProgram* GetSkinVP(bool bHolo)
{
    if (!skinvp)
        skinvp = new VertexProgram(usearbvertexprograms ? szSkinVPARB : cSkinVP,
                                   cSkinVPDeclaration);
    if (!holoskinvp)
        holoskinvp = new VertexProgram(usearbvertexprograms ? cHoloSkinVPARB : cSkinVP,
                                       cSkinVPDeclaration);
    return bHolo ? holoskinvp : skinvp;
}

#if KOTOR1
// K1 has no hologram skin program: one guarded function-local static, built once.
VertexProgram* GetSkinVP()
{
    static VertexProgram* s_pVP = new VertexProgram(usearbvertexprograms ? szSkinVPARB : cSkinVP,
                                                    cSkinVPDeclaration);
    return s_pVP;
}
#endif

// Builds the bumpy-shiny program despite the name.
VertexProgram* GetSkinBumpyVP()
{
#if KOTOR1  // K1: a guarded function-local static, built once and never freed
    static VertexProgram* s_pVP = new VertexProgram(cBumpyShinySkinVP, cBumpyShinySkinVPDeclaration);
    return s_pVP;
#else
    if (!bumpyshinyskinvp)
        bumpyshinyskinvp = new VertexProgram(cBumpyShinySkinVP, cBumpyShinySkinVPDeclaration);
    return bumpyshinyskinvp;
#endif
}

VertexProgram* GetSkinSphereMapVP()
{
#if KOTOR1  // K1: a guarded function-local static
    static VertexProgram* s_pVP = new VertexProgram(cSkinSphereMapVP, cSkinSphereMapVPDeclaration);
    return s_pVP;
#else
    if (!skinspheremapvp)
        skinspheremapvp = new VertexProgram(cSkinSphereMapVP, cSkinSphereMapVPDeclaration);
    return skinspheremapvp;
#endif
}

VertexProgram* GetSkinCubeMapVP()
{
#if KOTOR1  // K1: a guarded function-local static
    static VertexProgram* s_pVP = new VertexProgram(usearbvertexprograms ? szSkinCubeMapVPARB : cSkinCubeMapVP,
                                                    cSkinCubeMapVPDeclaration);
    return s_pVP;
#else
    if (!skincubemapvp)
        skincubemapvp = new VertexProgram(usearbvertexprograms ? szSkinCubeMapVPARB : cSkinCubeMapVP,
                                          cSkinCubeMapVPDeclaration);
    return skincubemapvp;
#endif
}

void AurSkin_ShutDown()
{
    if (skinvp) {
        delete skinvp;
        skinvp = 0;
    }
    if (holoskinvp) {
        delete holoskinvp;
        holoskinvp = 0;
    }
    if (bumpyshinyskinvp) {
        delete bumpyshinyskinvp;
        bumpyshinyskinvp = 0;
    }
    if (skinspheremapvp) {
        delete skinspheremapvp;
        skinspheremapvp = 0;
    }
    if (skincubemapvp) {
        delete skincubemapvp;
        skincubemapvp = 0;
    }
}

// Debug-console handlers for the bump-highlight pair.

const char* usebumpdiffuseonly(char*)
{
    usebumpdiffuse = 1;
    usebumpspecular = 0;
    return "Displaying only diffuse highlights on bumpmapped objects.";
}

const char* usebumpspecularonly(char*)
{
    usebumpdiffuse = 0;
    usebumpspecular = 1;
    return "Displaying only specular highlights on bumpmapped objects.";
}

const char* usebumpspecularanddiffuse(char*)
{
    usebumpdiffuse = 1;
    usebumpspecular = 1;
    return "Displaying both specular and diffuse highlights on bumpmapped objects.";
}
// Renders skinned through the full vendor-dispatch vertex-program path: gathers the bone frames,
// uploads them to the vertex-program constants, then walks the vendor strategy ladder to pick a
// render program.
#if KOTOR1
// K1: the same front half with the quaternion operators and a smaller tail: no hologram,
// do-not-draw, force-sight or ARB fog handling, and a fixed-function arm that picks a cube-map or
// sphere-map program by the env texture.
void RenderSkinnedVP(VertexPrimitiveSkinned* vertices)
{
    Vector vZero(0.0f, 0.0f, 0.0f);
    PartTriMesh* part = vertices->m_pPart->AsPartSkin();
    MdlNodeSkin* node = static_cast<MdlNodeSkin*>(part->m_pNode);

    if (node->m_nTexCoordSets == 0)
        return;
    MdlNodeTriMesh* pNode12 = part->AsPartTriMesh() != 0 ? static_cast<MdlNodeTriMesh*>(part->m_pNode) : 0;

    vertices->SetupVertexArraysStandard();

    static List<Quaternion> qb;
    static List<Vector> tb;

    ASLgl::glEnable(enableTranslationTable[9]);
    if (usearbvertexprograms) {
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 16, 0.0f, 2.0f, 3.0f, 1.0f);
        if (usearbvertexprograms) {
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 17,
                currentscene->m_fFogStart,
                currentscene->m_fFogEnd,
                currentscene->m_bFogEnabled != 0 ? 1.0f : 0.0f, 0.5f);
        }
    }

    if (vertices->m_pPart->m_pGob->m_bBonesDirty) {
        Part* pRoot = part;
        while (pRoot->m_pParent != 0)
            pRoot = static_cast<Part*>(pRoot->m_pParent);
        qb.m_count = 0;
        tb.m_count = 0;
        GatherBones(pRoot, tb, qb, Vector(0.0f, 0.0f, 0.0f),
                    Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        vertices->m_pPart->m_pGob->m_bBonesDirty = 0;
    }

    Quaternion qInv = -part->m_qLocalOrientation;
    Vector vPos = RotateVector(-part->m_vLocalPosition, qInv);
    Quaternion qAcc = -part->m_qLocalOrientation;
    for (Part* pParent = static_cast<Part*>(part->m_pParent); pParent != 0; pParent = static_cast<Part*>(pParent->m_pParent)) {
        Quaternion qParentInv = -pParent->m_qLocalOrientation;
        Vector p = RotateVector(-pParent->m_vLocalPosition, qParentInv);
        vPos = vPos + RotateVector(p, qAcc);
        qAcc = qAcc * -pParent->m_qLocalOrientation;
    }

    for (int64_t i = 0; i < node->m_nBoneCount; i++) {
        int nBone = static_cast<int>(node->m_paBoneMap[i]);
        if (nBone == -1)
            continue;
        Vector vCached = RotateVector(tb.m_pData[i], qAcc);
        Vector vBone = RotateVector(RotateVector(node->m_lTBoneRefInv.m_pData[i], qb.m_pData[i]), qAcc);
        Quaternion q = qAcc * qb.m_pData[i] * node->m_lQBoneRefInv.m_pData[i];
        if (usearbvertexprograms) {
            float w = q.w, x = q.x, y = q.y, z = q.z;
            int nIdx = nBone * 3;
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIdx + 18,
                1.0f - ((y * y + z * z) + (y * y + z * z)),
                (y * x - z * w) + (y * x - z * w),
                (z * x + y * w) + (z * x + y * w),
                vPos.x + vCached.x + vBone.x);
            if (usearbvertexprograms) {
                ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIdx + 19,
                    (y * x + z * w) + (y * x + z * w),
                    1.0f - ((z * z + x * x) + (z * z + x * x)),
                    (y * z - x * w) + (y * z - x * w),
                    vPos.y + vCached.y + vBone.y);
                if (usearbvertexprograms) {
                    ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIdx + 20,
                        (z * x - y * w) + (z * x - y * w),
                        (y * z + x * w) + (y * z + x * w),
                        1.0f - ((y * y + x * x) + (y * y + x * x)),
                        vPos.z + vCached.z + vBone.z);
                }
            }
        }
    }

    if (part->m_pMaterial->HasBumpMap() && part->m_pMaterial->HasBumpyShinyMap() &&
        pNode12->HasTangentBases() && AurBumpyShinyAvailable() && enablebumpyshiny) {
        g_pBumpyShinySkinPass1->Enable(part, true);
        part->m_pMaterial->BindTexture0();
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShinyTextures, part->m_pMaterial->Textures(0).m_pObject, 0, 0,
                                          part->m_pMaterial->Textures(5).m_pObject);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShiny, 0, 0, 0, 0);
        unsigned int nBlendOne = blendTranslationTable[2];
        ASLgl::glBlendFunc(nBlendOne, blendTranslationTable[3]);
        RenderFlat(vertices);
        ASLgl::glBlendFunc(nBlendOne, blendTranslationTable[1]);
    } else if (part->m_pMaterial->HasBumpMap() && AurBumpMapCardValidate() && pNode12->HasTangentBases() &&
               AurNV2xLevelShadersAvailable() && enablebumpmap && usenewbumpyskin) {
        ASLgl::glDisable(enableTranslationTable[1]);
        static VertexProgram* newSkinBumpyVP =
            new VertexProgram(usearbvertexprograms ? szNewSkinBumpyVPARB : cNewSkinBumpyVP,
                              cSkinBumpyVPDeclaration);
        float fAmbiX, fAmbiY, fAmbiZ;
        Vector vSpecColor;
        if (pNode12 != 0) {
            vSpecColor = part->m_pMaterial->Textures(4).m_pObject->GetSpecularColor();
            fAmbiX = pNode12->m_vDiffuse.x;
            fAmbiY = pNode12->m_vDiffuse.y;
            fAmbiZ = pNode12->m_vDiffuse.z;
        } else {
            fAmbiX = 0.0f; fAmbiY = 0.0f; fAmbiZ = 0.0f; vSpecColor = Vector(0.0f, 0.0f, 0.0f);
        }
        List<PartLight*> bumpLights(currentscene->m_pLightManager->GetBumpMapLights());
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmap, part->m_pMaterial->Textures(4).m_pObject,
                                          currentscene->m_pNormalizationCubeMap.m_pObject,
                                          currentscene->m_pNormalizationCubeMap.m_pObject,
                                          part->m_pMaterial->Textures(0).m_pObject);
        GLRender::pfuncSetPixelShaderAmbientColor(currentscene->m_ambient, 1.0f);
        Vector vWS(0.0f, 0.0f, 0.0f);
        Quaternion lq(1.0f, 0.0f, 0.0f, 0.0f);
        part->GetWorld(vWS, lq);
        Vector vView = RotateVector(CurrentCamera->m_vPosition - vWS, -lq);
        ASLgl::glEnable(enableTranslationTable[9]);
        unsigned int nBlendOne = blendTranslationTable[2];
        ASLgl::glBlendFunc(nBlendOne, blendTranslationTable[3]);
        currentscene->m_pLightManager->DisableBumpMapLights();
        for (int64_t k = 0; k < bumpLights.m_count; k++) {
            newSkinBumpyVP->Enable(part, true);
            PartLight* pLight = bumpLights.m_pData[k];
            Vector vLight = RotateVector(pLight->m_vWorldPosition - vWS, -lq);
            {
            Vector vDiff(pLight->m_vLightColour.x * (fAmbiX * diffuseintensity),
                         pLight->m_vLightColour.y * (fAmbiY * diffuseintensity),
                         fAmbiZ * diffuseintensity * pLight->m_vLightColour.z);
            if (usebumpdiffuse == 0)
                GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
            else
                GLRender::pfuncSetBumpLightColor(vDiff, 1.0f);
            }
            float fTheta = specularintensity;
            float fSpec = part->m_pMaterial->Textures(4).m_pObject->GetBumpIntensity();
            Vector vSpec(vSpecColor.x * fTheta * fSpec * pLight->m_vLightColour.x,
                         vSpecColor.y * fTheta * fSpec * pLight->m_vLightColour.y,
                         vSpecColor.z * fTheta * fSpec * pLight->m_vLightColour.z);
            if (usebumpspecular == 0)
                GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
            else
                GLRender::pfuncSetPixelShaderAmbientColor(vSpec, 1.0f);
            if (usearbvertexprograms &&
                (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, vLight.x, vLight.y, vLight.z, 1.0f),
                 usearbvertexprograms) &&
                (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, vView.x, vView.y, vView.z, 1.0f),
                 usearbvertexprograms)) {
                float fR = pLight->m_fLightRadius * pLight->m_fLightRadius;
                ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 14, 1.0f, 0.0f,
                                                  9.0f / (fR + fR), 1.0f);
            }
            RenderFlat(vertices);
        }
        if (bumpLights.m_count == 0) {
            newSkinBumpyVP->Enable(part, false);
            GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
            GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
            RenderFlat(vertices);
        }
        ASLgl::glDisableClientState(clientStateTranslationTable[8]);
        ASLgl::glDisableClientState(clientStateTranslationTable[9]);
        ASLgl::glDisableClientState(clientStateTranslationTable[10]);
        ASLgl::glBlendFunc(nBlendOne, nBlendOne);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 15, 1.0f, 1.0f, 1.0f, 1.0f);
        ASLgl::glEnable(enableTranslationTable[1]);
        currentscene->m_pLightManager->EnableBumpMapLights();
        currentscene->DoFog();
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
    } else if (part->m_pMaterial->HasNormalMap() && AurBumpMapCardValidate() && AurNV2xLevelShadersAvailable() &&
               enablebumpmap && usenewbumpyskin) {
        ASLgl::glDisable(enableTranslationTable[1]);
        static VertexProgram* wsNewSkinBumpyVP =
            new VertexProgram(szSkinBumpyWSVPARB, szNormalmapVPARBDeclaration);
        float fAmbiX, fAmbiY, fAmbiZ;
        Vector vSpecColor;
        if (pNode12 != 0) {
            vSpecColor = part->m_pMaterial->Textures(4).m_pObject->GetSpecularColor();
            fAmbiX = pNode12->m_vDiffuse.x;
            fAmbiY = pNode12->m_vDiffuse.y;
            fAmbiZ = pNode12->m_vDiffuse.z;
        } else {
            fAmbiX = 0.0f; fAmbiY = 0.0f; fAmbiZ = 0.0f; vSpecColor = Vector(0.0f, 0.0f, 0.0f);
        }
        List<PartLight*> bumpLights(currentscene->m_pLightManager->GetBumpMapLights());
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNormalmap, part->m_pMaterial->Textures(0).m_pObject,
                                          part->m_pMaterial->Textures(4).m_pObject,
                                          currentscene->m_pNormalizationCubeMap.m_pObject,
                                          currentscene->m_pNormalizationCubeMap.m_pObject);
        GLRender::pfuncSetPixelShaderAmbientColor(currentscene->m_ambient, 1.0f);
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE1);
        unsigned int nTexCoordState = clientStateTranslationTable[0];
        GLRender::cm_uiCurrentStage = 1;
        ASLgl::glEnableClientState(nTexCoordState);
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
        GLRender::cm_uiCurrentStage = 0;
        Vector vWS(0.0f, 0.0f, 0.0f);
        Quaternion lq(1.0f, 0.0f, 0.0f, 0.0f);
        part->GetWorld(vWS, lq);
        Vector vView = RotateVector(CurrentCamera->m_vPosition - vWS, -lq);
        ASLgl::glEnable(enableTranslationTable[9]);
        unsigned int nBlendOne = blendTranslationTable[2];
        ASLgl::glBlendFunc(nBlendOne, blendTranslationTable[3]);
        currentscene->m_pLightManager->DisableBumpMapLights();
        for (int64_t k = 0; k < bumpLights.m_count; k++) {
            wsNewSkinBumpyVP->Enable(part, false);
            PartLight* pLight = bumpLights.m_pData[k];
            Vector vLight = RotateVector(pLight->m_vWorldPosition - vWS, -lq);
            {
            Vector vDiff(pLight->m_vLightColour.x * (fAmbiX * diffuseintensity),
                         pLight->m_vLightColour.y * (fAmbiY * diffuseintensity),
                         fAmbiZ * diffuseintensity * pLight->m_vLightColour.z);
            if (usebumpdiffuse == 0)
                GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
            else
                GLRender::pfuncSetBumpLightColor(vDiff, 1.0f);
            }
            float fTheta = specularintensity;
            float fSpec = part->m_pMaterial->Textures(4).m_pObject->GetBumpIntensity();
            Vector vSpec(vSpecColor.x * fTheta * fSpec * pLight->m_vLightColour.x,
                         vSpecColor.y * fTheta * fSpec * pLight->m_vLightColour.y,
                         vSpecColor.z * fTheta * fSpec * pLight->m_vLightColour.z);
            if (usebumpspecular == 0)
                GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
            else
                GLRender::pfuncSetPixelShaderAmbientColor(vSpec, 1.0f);
            if (usearbvertexprograms &&
                (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, vLight.x, vLight.y, vLight.z, 1.0f),
                 usearbvertexprograms) &&
                (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, vView.x, vView.y, vView.z, 1.0f),
                 usearbvertexprograms)) {
                float fR = pLight->m_fLightRadius * pLight->m_fLightRadius;
                ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 14, 1.0f, 0.0f,
                                                  9.0f / (fR + fR), 1.0f);
            }
            RenderFlat(vertices);
        }
        if (bumpLights.m_count == 0) {
            wsNewSkinBumpyVP->Enable(part, false);
            GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
            GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
            RenderFlat(vertices);
        }
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE1);
        GLRender::cm_uiCurrentStage = 1;
        ASLgl::glDisableClientState(nTexCoordState);
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
        GLRender::cm_uiCurrentStage = 0;
        currentscene->DoFog();
        ASLgl::glBlendFunc(nBlendOne, nBlendOne);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
        if (usearbvertexprograms)
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 15, 1.0f, 1.0f, 1.0f, 1.0f);
        static VertexProgram* skinCleanVP =
            new VertexProgram(usearbvertexprograms ? szSkinVPARB : cSkinVP,
                              szNormalmapVPARBDeclaration);
        skinCleanVP->Enable(part, true);
        if (enabledecal) {
            if (AurMultiTextureAvailable())
                ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
            GLRender::cm_uiCurrentStage = 0;
            part->m_pMaterial->BindTexture0();
            RenderFlat(vertices);
        }
        ASLgl::glEnable(enableTranslationTable[1]);
        currentscene->m_pLightManager->EnableBumpMapLights();
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
    } else if (part->m_pMaterial->HasBumpMap() && AurBumpMapCardValidate() && pNode12->HasTangentBases() &&
               AurNV2xLevelShadersAvailable() && enablebumpmap) {
        static VertexProgram* skinBumpyVP =
            new VertexProgram(usearbvertexprograms ? szSkinBumpyVPARB : cSkinBumpyVP,
                              cSkinBumpyVPDeclaration);
        float fAmbiX, fAmbiY, fAmbiZ;
        Vector vSpecColor;
        if (pNode12 != 0) {
            vSpecColor = part->m_pMaterial->Textures(4).m_pObject->GetSpecularColor();
            fAmbiX = pNode12->m_vDiffuse.x;
            fAmbiY = pNode12->m_vDiffuse.y;
            fAmbiZ = pNode12->m_vDiffuse.z;
        } else {
            fAmbiX = 0.0f; fAmbiY = 0.0f; fAmbiZ = 0.0f; vSpecColor = Vector(0.0f, 0.0f, 0.0f);
        }
        List<PartLight*> bumpLights(currentscene->m_pLightManager->GetBumpMapLights());
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmap, part->m_pMaterial->Textures(4).m_pObject,
                                          currentscene->m_pNormalizationCubeMap.m_pObject,
                                          currentscene->m_pNormalizationCubeMap.m_pObject,
                                          part->m_pMaterial->Textures(0).m_pObject);
        GLRender::pfuncSetPixelShaderAmbientColor(currentscene->m_ambient, 1.0f);
        Vector vWS(0.0f, 0.0f, 0.0f);
        Quaternion lq(1.0f, 0.0f, 0.0f, 0.0f);
        part->GetWorld(vWS, lq);
        Vector vView = RotateVector(CurrentCamera->m_vPosition - vWS, -lq);
        ASLgl::glEnable(enableTranslationTable[9]);
        // The blend pair is re-used by the closing glBlendFunc.
        unsigned int nBlendSrc = blendTranslationTable[0];
        unsigned int nBlendDst = blendTranslationTable[1];
        ASLgl::glBlendFunc(nBlendSrc, nBlendDst);
        for (int64_t k = 0; k < bumpLights.m_count; k++) {
            currentscene->m_pLightManager->DisableBumpMapLights();
            skinBumpyVP->Enable(part, true);
            PartLight* pLight = bumpLights.m_pData[k];
            Vector vLight = RotateVector(pLight->m_vWorldPosition - vWS, -lq);
            {
            Vector vDiff(pLight->m_vLightColour.x * (fAmbiX * diffuseintensity),
                         pLight->m_vLightColour.y * (fAmbiY * diffuseintensity),
                         fAmbiZ * diffuseintensity * pLight->m_vLightColour.z);
            if (usebumpdiffuse == 0)
                GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
            else
                GLRender::pfuncSetBumpLightColor(vDiff, 1.0f);
            }
            float fTheta = specularintensity;
            float fSpec = part->m_pMaterial->Textures(4).m_pObject->GetBumpIntensity();
            Vector vSpec(vSpecColor.x * fTheta * fSpec * pLight->m_vLightColour.x,
                         vSpecColor.y * fTheta * fSpec * pLight->m_vLightColour.y,
                         vSpecColor.z * fTheta * fSpec * pLight->m_vLightColour.z);
            if (usebumpspecular == 0)
                GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
            else
                GLRender::pfuncSetPixelShaderAmbientColor(vSpec, 1.0f);
            if (usearbvertexprograms &&
                (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, vLight.x, vLight.y, vLight.z, 1.0f),
                 usearbvertexprograms) &&
                (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, vView.x, vView.y, vView.z, 1.0f),
                 usearbvertexprograms)) {
                float fR = pLight->m_fLightRadius * pLight->m_fLightRadius;
                ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 14, 1.0f, 0.0f,
                                                  9.0f / (fR + fR), 1.0f);
            }
            RenderFlat(vertices);
            currentscene->m_pLightManager->EnableBumpMapLights();
        }
        if (bumpLights.m_count == 0) {
            skinBumpyVP->Enable(part, true);
            GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
            GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
            RenderFlat(vertices);
        }
        ASLgl::glDisableClientState(clientStateTranslationTable[8]);
        ASLgl::glDisableClientState(clientStateTranslationTable[9]);
        ASLgl::glDisableClientState(clientStateTranslationTable[10]);
        currentscene->DoFog();
        ASLgl::glBlendFunc(nBlendSrc, nBlendDst);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    } else if ((part->m_pGob != 0 && part->m_pGob->m_texWatch.m_pObject != 0) ||
               part->m_pMaterial->HasEnvMap()) {
        // the env texture is the gob's watched texture, else the material's env slot
        CAurTexture* pEnv = (part->m_pGob != 0 && part->m_pGob->m_texWatch.m_pObject != 0)
                                ? part->m_pGob->m_texWatch.m_pObject
                                : part->m_pMaterial->Textures(5).m_pObject;
        VertexProgram* pEnvVP = pEnv->IsCubeMap() ? GetSkinCubeMapVP() : GetSkinSphereMapVP();
        pEnvVP->Enable(part, true);
        bool bAti = AurATIValidate();
        RenderEnvironmentMapped(vertices, true, bAti);
        if (AurATIValidate()) {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, part->m_pMaterial->Textures(0).m_pObject, 0, 0, 0);
            currentscene->DoFog();
        }
    } else {
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
        GLRender::cm_uiCurrentStage = 0;
        part->m_pMaterial->BindTexture0();
        VertexProgram* pSkinVP;
        if (use3boneskinning == 0) {
            pSkinVP = GetSkinVP();
        } else {
            static VertexProgram* skin3boneVP = new VertexProgram(cSkin3BoneVP, cSkin3BoneVPDeclaration);
            pSkinVP = skin3boneVP;
        }
        pSkinVP->Enable(part, true);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, part->m_pMaterial->Textures(0).m_pObject, 0, 0, 0);
        if ((part->m_pMaterial->HasBumpMap() && pNode12->HasTangentBases() && !AurNV2xLevelShadersAvailable()) ||
            (part->m_pMaterial->HasBumpMap() && pNode12->HasTangentBases() && !AurBumpMapCardValidate())) {
            ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[3]);
            RenderFlat(vertices);
            ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
        } else {
            RenderFlat(vertices);
        }
        if (AurATIValidate()) {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, part->m_pMaterial->Textures(0).m_pObject, 0, 0, 0);
            currentscene->DoFog();
        }
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    }
    ASLgl::glDisable(enableTranslationTable[9]);
}
#else
void RenderSkinnedVP(VertexPrimitiveSkinned* vertices)
{
    // Zero colour handed to the light-colour setters and glFogfv.
    Vector vZero(0.0f, 0.0f, 0.0f);
    PartTriMesh* part = vertices->m_pPart->AsPartSkin();
    MdlNodeSkin* node = static_cast<MdlNodeSkin*>(part->m_pNode);

    if (node->m_nTexCoordSets == 0)
        return;
    PartTriMesh* pTri12 = part->AsPartTriMesh();
    MdlNodeTriMesh* pNode12 = pTri12 != 0 ? node : 0;
    Gob* pGob = pTri12 != 0 ? pTri12->m_pGob : 0;
    if (part->m_bHologramRendering && pNode12->m_bHologramDoNotDraw != 0)
        return;

    vertices->SetupVertexArraysStandard();

    static List<Quaternion> qb;
    static List<Vector> tb;

    if (usearbvertexprograms)
        ASLgl::glEnable(GL_VERTEX_PROGRAM_ARB);
    else
        ASLgl::glEnable(enableTranslationTable[9]);
    if (usearbvertexprograms) {
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 16, 0.0f, 2.0f, 3.0f, 1.0f);
        if (usearbvertexprograms) {
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 17,
                currentscene->m_fFogStart,
                currentscene->m_fFogEnd,
                currentscene->m_bFogEnabled != 0 ? 1.0f : 0.0f, 0.5f);
        }
    }

    // When the dirty latch is set, regather: rebuild the bone-frame cache from the root part.
    if (vertices->m_pPart->m_pGob->m_bBonesDirty) {
        Part* pRoot = part;
        while (pRoot->m_pParent != 0)
            pRoot = static_cast<Part*>(pRoot->m_pParent);
        qb.m_count = 0;
        tb.m_count = 0;
        GatherBones(pRoot, tb, qb, Vector(0.0f, 0.0f, 0.0f),
                    Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
        vertices->m_pPart->m_pGob->m_bBonesDirty = 0;
    }

    // Gather the world frame: the part's own frame plus the parent chain.
    float px, py, pz;               // accumulated translation (world from local, transposed rotation)
    float fX, fY, fZ, fW;           // accumulated quaternion (w component stored negated)
    {
        float qw = part->m_qLocalOrientation.w;
        float qx = part->m_qLocalOrientation.x;
        float qy = part->m_qLocalOrientation.y;
        float qz = part->m_qLocalOrientation.z;
        float nw = -qw;
        fW = nw; fX = qx; fY = qy; fZ = qz;
        px = -part->m_vLocalPosition.x;
        py = -part->m_vLocalPosition.y;
        pz = -part->m_vLocalPosition.z;
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
            t4 = (t4 + t4) * px;
            t6 = (t6 + t6) * px;
            t8 = (t8 + t8) * py;
            px = (1.0f - (t1 + t1)) * px + (t3 + t3) * py + (t5 + t5) * pz;
            py = t4 + (1.0f - (t2 + t2)) * py + (t7 + t7) * pz;
            pz = t6 + t8 + (1.0f - (t9 + t9)) * pz;
        }
    }
    Part* pParent = static_cast<Part*>(part->m_pParent);
    while (pParent != 0) {
        Part* pCur = pParent;
        float qw = pCur->m_qLocalOrientation.w;
        float qx = pCur->m_qLocalOrientation.x;
        float qy = pCur->m_qLocalOrientation.y;
        float qz = pCur->m_qLocalOrientation.z;
        float nw = -qw;
        float mx = -pCur->m_vLocalPosition.x;
        float my = -pCur->m_vLocalPosition.y;
        float mz = -pCur->m_vLocalPosition.z;
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
            t4 = (t4 + t4) * mx;
            t6 = (t6 + t6) * mx;
            t8 = (t8 + t8) * my;
            mx = (1.0f - (t1 + t1)) * mx + (t3 + t3) * my + (t5 + t5) * mz;
            my = t4 + (1.0f - (t2 + t2)) * my + (t7 + t7) * mz;
            mz = t6 + t8 + (1.0f - (t9 + t9)) * mz;
        }
        if (quaternionidentityoptimization == 0 || (fW != -1.0f && fW != 1.0f)) {
            float t1 = fY * fY + fZ * fZ;
            float t2 = fX * fX + fZ * fZ;
            float t3 = fX * fY - fW * fZ;
            float t4 = fX * fY + fW * fZ;
            float t5 = fW * fY + fX * fZ;
            float t6 = fX * fZ - fW * fY;
            float t7 = fY * fZ - fW * fX;
            float t8 = fW * fX + fY * fZ;
            float t9 = fX * fX + fY * fY;
            t4 = (t4 + t4) * mx;
            t6 = (t6 + t6) * mx;
            t8 = (t8 + t8) * my;
            mx = (1.0f - (t1 + t1)) * mx + (t3 + t3) * my + (t5 + t5) * mz;
            my = t4 + (1.0f - (t2 + t2)) * my + (t7 + t7) * mz;
            mz = t6 + t8 + (1.0f - (t9 + t9)) * mz;
        }
        px += mx;
        py += my;
        pz += mz;
        float aw = fW, ax = fX, ay = fY, az = fZ;
        float u1 = aw * qx;
        float u2 = aw * qy;
        float u3 = ax * qz;
        float u4 = aw * qz;
        float u5 = ax * qy;
        float u6 = ay * qx;
        fW = aw * nw - ax * qx - ay * qy - az * qz;
        fX = ax * nw + u1 + ay * qz - az * qy;
        fY = (u2 - u3) + ay * nw + az * qx;
        fZ = az * nw + ((u5 + u4) - u6);
        pParent = static_cast<Part*>(pCur->m_pParent);
    }

    // For each bone, rotate the cached frame and the bone position by the accumulated basis, then
    // upload the resulting rotation-matrix rows as vertex-program constants.
    if (node->m_nBoneCount >= 1) {
        bool bFWPos = (fW == 1.0f);
        bool bFWNeg = (fW == -1.0f);
        float mM00 = 1.0f - ((fY * fY + fZ * fZ) + (fY * fY + fZ * fZ));
        float mM10 = (fX * fY - fW * fZ) + (fX * fY - fW * fZ);
        float mM20 = (fW * fY + fX * fZ) + (fW * fY + fX * fZ);
        float mM01 = (fX * fY + fW * fZ) + (fX * fY + fW * fZ);
        float mM11 = 1.0f - ((fX * fX + fZ * fZ) + (fX * fX + fZ * fZ));
        float mM21 = (fY * fZ - fW * fX) + (fY * fZ - fW * fX);
        float mM02 = (fX * fZ - fW * fY) + (fX * fZ - fW * fY);
        float mM12 = (fW * fX + fY * fZ) + (fW * fX + fY * fZ);
        float mM22 = 1.0f - ((fX * fX + fY * fY) + (fX * fX + fY * fY));
        for (int64_t i = 0; i < node->m_nBoneCount; i++) {
            int nBone = static_cast<int>(node->m_paBoneMap[i]);   // the bone index is stored as a float
            if (nBone == -1)
                continue;
            // Stage 1: rotate the cached frame by the accumulated transpose.
            float olxr, oly, olz;
            if (quaternionidentityoptimization != 0 && (bFWNeg || bFWPos)) {
                olxr = tb.m_pData[i].x;
                oly = tb.m_pData[i].y;
                olz = tb.m_pData[i].z;
            } else {
                olz = mM22 * tb.m_pData[i].z +
                      (mM02 * tb.m_pData[i].x + mM12 * tb.m_pData[i].y);
                oly = mM21 * tb.m_pData[i].z +
                      (mM01 * tb.m_pData[i].x + mM11 * tb.m_pData[i].y);
                olxr = mM20 * tb.m_pData[i].z +
                       (mM00 * tb.m_pData[i].x + mM10 * tb.m_pData[i].y);
            }
            // Stage 2: rotate the bone position by the gathered quaternion, then by the basis (also
            // identity-gated on the accumulated quaternion).
            Vector src = node->m_lTBoneRefInv.m_pData[i];
            Quaternion q = qb.m_pData[i];
            float rx, ry, rz;
            if (quaternionidentityoptimization == 0 || (q.w != -1.0f && q.w != 1.0f)) {
                float w = q.w, x = q.x, y = q.y, z = q.z;
                float t1 = y * y + z * z;
                float t2 = x * x + z * z;
                float t3 = x * y - w * z;
                float t4 = x * y + w * z;
                float t5 = w * y + x * z;
                float t6 = x * z - w * y;
                float t7 = y * z - w * x;
                float t8 = w * x + y * z;
                float t9 = x * x + y * y;
                rx = (1.0f - (t1 + t1)) * src.x + (t3 + t3) * src.y + (t5 + t5) * src.z;
                ry = (t4 + t4) * src.x + (1.0f - (t2 + t2)) * src.y + (t7 + t7) * src.z;
                rz = (t6 + t6) * src.x + (t8 + t8) * src.y + (1.0f - (t9 + t9)) * src.z;
                if (quaternionidentityoptimization == 0 || (!bFWPos && !bFWNeg))
                    goto lbl_second;
            } else {
                rx = src.x; ry = src.y; rz = src.z;
                if (!bFWPos && !bFWNeg)
                    goto lbl_second;
            }
            goto lbl_env;
        lbl_second:
            {
                float nx = mM20 * rz + (mM00 * rx + mM10 * ry);
                float ny = mM21 * rz + (mM01 * rx + mM11 * ry);
                rz = mM22 * rz + (mM02 * rx + mM12 * ry);
                rx = nx;
                ry = ny;
            }
        lbl_env:
            if (usearbvertexprograms) {
                float qx2 = q.x, qy2 = q.y, qz2 = q.z;
                Quaternion bq = node->m_lQBoneRefInv.m_pData[i];
                float pw = fW * q.w - fX * qx2 - fY * qy2 - fZ * qz2;      // acc * bonesQuat (acc w negated)
                float pwx = fX * q.w + fW * qx2 + fY * q.z - fZ * qy2;
                float pwy = fZ * qx2 + (fY * q.w + (fW * qy2 - fX * q.z));
                float pwz = fZ * q.w + ((fX * qy2 + fW * q.z) - fY * qx2);
                float c0 = ((pw * bq.w - pwx * bq.x) - pwy * bq.y) - pwz * bq.z;  // boneQuat * acc * bonesQuat
                float c1 = (bq.w * pwx + pw * bq.x + pwy * bq.z) - bq.y * pwz;
                float c2 = bq.x * pwz + (bq.w * pwy + (pw * bq.y - pwx * bq.z));
                float c3 = bq.w * pwz + ((pwx * bq.y + pw * bq.z) - bq.x * pwy);
                int nIdx = nBone * 3;
                ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIdx + 18,
                    1.0f - ((c2 * c2 + c3 * c3) + (c2 * c2 + c3 * c3)),
                    (c1 * c2 - c0 * c3) + (c1 * c2 - c0 * c3),
                    (c1 * c3 + c0 * c2) + (c1 * c3 + c0 * c2),
                    px + olxr + rx);
                if (usearbvertexprograms) {
                    ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIdx + 19,
                        (c1 * c2 + c0 * c3) + (c1 * c2 + c0 * c3),
                        1.0f - ((c1 * c1 + c3 * c3) + (c1 * c1 + c3 * c3)),
                        (c2 * c3 - c0 * c1) + (c2 * c3 - c0 * c1),
                        py + oly + ry);
                    if (usearbvertexprograms) {
                        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIdx + 20,
                            (c1 * c3 - c0 * c2) + (c1 * c3 - c0 * c2),
                            (c0 * c1 + c2 * c3) + (c0 * c1 + c2 * c3),
                            1.0f - ((c1 * c1 + c2 * c2) + (c1 * c1 + c2 * c2)),
                            pz + olz + rz);
                    }
                }
            }
        }
    }

    // Vendor strategy ladder.

    Material* pMat = part->m_pMaterial;
    LightManager* lm = currentscene->m_pLightManager;
    if (pMat->HasBumpMap() && pMat->HasBumpyShinyMap() && node->HasTangentBases() &&
        AurBumpyShinyAvailable() && enablebumpyshiny) {
        // Bumpy-shiny pass.
        ASLgl::glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
        ASLgl::glEnable(enableTranslationTable[0]);
        ASLgl::glDisable(enableTranslationTable[1]);
        if (part->m_bHologramRendering) {
            static VertexProgram* holoBumpyShinySkinVP =
                new VertexProgram(cHoloSkinVPARB, cBumpyShinySkinVPDeclaration);
            ASLgl::glDisableClientState(clientStateTranslationTable[1]);
            holoBumpyShinySkinVP->Enable(part, false);
            if (g_ForceSight != 0) {
                GLRender::pfuncEnablePixelShaders(GLRender::kPSHoloDesat, 0, 0, 0, 0);
                ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[2]);
            } else {
                GLRender::pfuncEnablePixelShaders(GLRender::kPSHolo, 0, 0, 0, 0);
                ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[0]);
            }
            RenderFlat(vertices);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSHoloOff, 0, 0, 0, 0);
            ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
            if (usearbvertexprograms) {
                ASLgl::glDisableVertexAttribArrayARB(12);
                ASLgl::glDisableVertexAttribArrayARB(13);
                ASLgl::glDisableVertexAttribArrayARB(14);
            }
            ASLgl::glEnableClientState(clientStateTranslationTable[1]);
            ASLgl::glPopAttrib();
        } else {
            g_pBumpyShinySkinPass1->Enable(part, true);
            pMat->BindTexture0();
            GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShinyTextures, pMat->Textures(0).m_pObject, 0, 0,
                                              pMat->Textures(5).m_pObject);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpyShiny, 0, 0, 0, 0);
            ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[3]);
            RenderFlat(vertices);
            ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[1]);
            ASLgl::glEnable(enableTranslationTable[1]);
        }
        goto lbl_epilogue;
    }
    if (pMat->HasBumpMap() && AurBumpMapCardValidate() && node->HasTangentBases() &&
        AurNV2xLevelShadersAvailable() && enablebumpmap && usenewbumpyskin) {
        // New-bumpy-skin pass.
        ASLgl::glDisable(enableTranslationTable[1]);
        static VertexProgram* newSkinBumpyVP =
            new VertexProgram(usearbvertexprograms ? szNewSkinBumpyVPARB : cNewSkinBumpyVP,
                              cSkinBumpyVPDeclaration);
        (void)newSkinBumpyVP;
        if (part->m_bHologramRendering) {
            static VertexProgram* holoNewSkinBumpyVP =
                new VertexProgram(cHoloSkinVPARB, cSkinBumpyVPDeclaration);
            holoNewSkinBumpyVP->Enable(part, false);
            goto lbl_fsflat;
        }
        {
            Vector vWS, vWQ;                     // GetWorld outputs (pos, orient)
            float fAmbiX, fAmbiY, fAmbiZ;        // pNode12->m_vDiffuse
            Vector vSpecColor;                   // Textures(4)->GetSpecularColor()
            if (pNode12 != 0) {
                vSpecColor = pMat->Textures(4).m_pObject->GetSpecularColor();
                fAmbiX = pNode12->m_vDiffuse.x;
                fAmbiY = pNode12->m_vDiffuse.y;
                fAmbiZ = pNode12->m_vDiffuse.z;
            } else {
                fAmbiX = 0.0f; fAmbiY = 0.0f; fAmbiZ = 0.0f; vSpecColor = Vector(0.0f, 0.0f, 0.0f);
            }
            List<PartLight*> bumpLights(lm->GetBumpMapLights());
            if (g_ForceSight == 0) {
                GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmap, pMat->Textures(4).m_pObject,
                                                  currentscene->m_pNormalizationCubeMap.m_pObject,
                                                  currentscene->m_pNormalizationCubeMap.m_pObject,
                                                  pMat->Textures(0).m_pObject);
                GLRender::pfuncSetPixelShaderAmbientColor(currentscene->m_ambient, 1.0f);
                // camera-relative view frame
                Quaternion lq(1.0f, 0.0f, 0.0f, 0.0f);
                part->GetWorld(vWS, lq);
                float vx = CurrentCamera->m_vPosition.x - vWS.x;
                float vy = CurrentCamera->m_vPosition.y - vWS.y;
                float vz = CurrentCamera->m_vPosition.z - vWS.z;
                {
                    float qw = lq.w, qx = lq.x, qy = lq.y, qz = lq.z;
                    float nw = -qw;
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
                        t4 = (t4 + t4) * vx;
                        t6 = (t6 + t6) * vx;
                        t8 = (t8 + t8) * vy;
                        vx = (1.0f - (t1 + t1)) * vx + (t3 + t3) * vy + (t5 + t5) * vz;
                        vy = t4 + (1.0f - (t2 + t2)) * vy + (t7 + t7) * vz;
                        vz = t6 + t8 + (1.0f - (t9 + t9)) * vz;
                    }
                }
                ASLgl::glEnable(enableTranslationTable[9]);
                ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[3]);
                currentscene->m_pLightManager->DisableBumpMapLights();
                ASLgl::glDisable(enableTranslationTable[2]);
                if (bumpLights.m_count > 0) {
                    for (int64_t k = 0; k < bumpLights.m_count; k++) {
                        newSkinBumpyVP->Enable(part, false);
                        PartLight* pLight = bumpLights.m_pData[k];
                        float lx = pLight->m_vWorldPosition.x - vWS.x;
                        float ly = pLight->m_vWorldPosition.y - vWS.y;
                        float lz = pLight->m_vWorldPosition.z - vWS.z;
                        {
                            float qw = lq.w, qx = lq.x, qy = lq.y, qz = lq.z;
                            float nw = -qw;
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
                                t4 = (t4 + t4) * lx;
                                t6 = (t6 + t6) * lx;
                                t8 = (t8 + t8) * ly;
                                lx = (1.0f - (t1 + t1)) * lx + (t3 + t3) * ly + (t5 + t5) * lz;
                                ly = t4 + (1.0f - (t2 + t2)) * ly + (t7 + t7) * lz;
                                lz = t6 + t8 + (1.0f - (t9 + t9)) * lz;
                            }
                        }
                        {
                        Vector vDiff(pLight->m_vLightColour.x * (fAmbiX * diffuseintensity),
                                     pLight->m_vLightColour.y * (fAmbiY * diffuseintensity),
                                     fAmbiZ * diffuseintensity * pLight->m_vLightColour.z);
                        if (usebumpdiffuse == 0)
                            GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
                        else
                            GLRender::pfuncSetBumpLightColor(vDiff, 1.0f);
                        }
                        float fTheta = specularintensity;
                        float fSpec = pMat->Textures(4).m_pObject->GetBumpIntensity();
                        Vector vSpec(vSpecColor.x * fTheta * fSpec * pLight->m_vLightColour.x,
                                     vSpecColor.y * fTheta * fSpec * pLight->m_vLightColour.y,
                                     vSpecColor.z * fTheta * fSpec * pLight->m_vLightColour.z);
                        if (usebumpspecular == 0)
                            GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
                        else
                            GLRender::pfuncSetPixelShaderAmbientColor(vSpec, 1.0f);
                        if (usearbvertexprograms &&
                            (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, lx, ly, lz, 1.0f),
                             usearbvertexprograms) &&
                            (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, vx, vy, vz, 1.0f),
                             usearbvertexprograms)) {
                            float fR = pLight->m_fLightRadius * pLight->m_fLightRadius;
                            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 14, 1.0f, 0.0f,
                                                              9.0f / (fR + fR), 1.0f);
                        }
                        RenderFlat(vertices);
                    }
                }
                if (bumpLights.m_count == 0) {
                    newSkinBumpyVP->Enable(part, false);
                    GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
                    GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
                    RenderFlat(vertices);
                }
            }
            if (!usearbvertexprograms) {
                ASLgl::glDisableClientState(clientStateTranslationTable[8]);
                ASLgl::glDisableClientState(clientStateTranslationTable[9]);
                ASLgl::glDisableClientState(clientStateTranslationTable[10]);
            } else {
                ASLgl::glDisableVertexAttribArrayARB(12);
                ASLgl::glDisableVertexAttribArrayARB(13);
                ASLgl::glDisableVertexAttribArrayARB(14);
            }
            ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[2]);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
            if (usearbvertexprograms)
                ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 15, 1.0f, 1.0f, 1.0f, 1.0f);
            static VertexProgram* skinBumpyCleanVP =
                new VertexProgram(usearbvertexprograms ? szSkinVPARB : cSkinVP,
                                  cSkinBumpyVPDeclaration);
            skinBumpyCleanVP->Enable(part, true);
            if (enabledecal) {
                if (AurMultiTextureAvailable())
                    ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
                GLRender::cm_uiCurrentStage = 0;
                pMat->BindTexture0();
                if (g_ForceSight != 0)
                    SetupForceSightShader(pGob, GLRender::kPSFGlow, GLRender::kPSDesat, 0, 0, 0, 0, false);
                RenderFlat(vertices);
                if (g_ForceSight != 0) {
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
                }
            }
            ASLgl::glEnable(enableTranslationTable[1]);
            currentscene->m_pLightManager->EnableBumpMapLights();
            ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
            currentscene->DoFog();
            if (currentscene->m_bFogEnabled != 0) {
                static VertexProgram* skinFogVP =
                    new VertexProgram(usearbvertexprograms ? szSkinFogVPARB : static_cast<char*>(0),
                                      cSkinBumpyVPDeclaration);
                skinFogVP->Enable(part, false);
                GLRender::pfuncEnablePixelShaders(GLRender::kPSFog, 0, 0, 0, 0);
                RenderFlat(vertices);
                GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
            }
            goto lbl_free;
        }
    }
    if (pMat->HasNormalMap() && AurBumpMapCardValidate() && AurNV2xLevelShadersAvailable() &&
        enablebumpmap && usenewbumpyskin) {
        // Normal-map pass.
        ASLgl::glDisable(enableTranslationTable[1]);
        static VertexProgram* wsNewSkinBumpyVP =
            new VertexProgram(szSkinBumpyWSVPARB, szNormalmapVPARBDeclaration);
        (void)wsNewSkinBumpyVP;
        if (part->m_bHologramRendering) {
            static VertexProgram* holoWSNewSkinBumpyVP =
                new VertexProgram(cHoloSkinVPARB, szNormalmapVPARBDeclaration);
            holoWSNewSkinBumpyVP->Enable(part, false);
            goto lbl_fsflat;
        }
        {
            Vector vWS, vWQ;
            float fAmbiX, fAmbiY, fAmbiZ;
            Vector vSpecColor;
            if (pNode12 != 0) {
                vSpecColor = pMat->Textures(4).m_pObject->GetSpecularColor();
                fAmbiX = pNode12->m_vDiffuse.x;
                fAmbiY = pNode12->m_vDiffuse.y;
                fAmbiZ = pNode12->m_vDiffuse.z;
            } else {
                fAmbiX = 0.0f; fAmbiY = 0.0f; fAmbiZ = 0.0f; vSpecColor = Vector(0.0f, 0.0f, 0.0f);
            }
            List<PartLight*> bumpLights(lm->GetBumpMapLights());
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNormalmap, pMat->Textures(0).m_pObject, pMat->Textures(4).m_pObject,
                                              currentscene->m_pNormalizationCubeMap.m_pObject,
                                              currentscene->m_pNormalizationCubeMap.m_pObject);
            GLRender::pfuncSetPixelShaderAmbientColor(currentscene->m_ambient, 1.0f);
            if (AurMultiTextureAvailable())
                ASLgl::glClientActiveTextureARB(GL_TEXTURE1);
            unsigned int nTexCoordState = clientStateTranslationTable[0];
            GLRender::cm_uiCurrentStage = 1;
            ASLgl::glEnableClientState(nTexCoordState);
            if (AurMultiTextureAvailable())
                ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
            GLRender::cm_uiCurrentStage = 0;
            Quaternion lq(1.0f, 0.0f, 0.0f, 0.0f);
            part->GetWorld(vWS, lq);
            float vx = CurrentCamera->m_vPosition.x - vWS.x;
            float vy = CurrentCamera->m_vPosition.y - vWS.y;
            float vz = CurrentCamera->m_vPosition.z - vWS.z;
            {
                float qw = lq.w, qx = lq.x, qy = lq.y, qz = lq.z;
                float nw = -qw;
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
                    t4 = (t4 + t4) * vx;
                    t6 = (t6 + t6) * vx;
                    t8 = (t8 + t8) * vy;
                    vx = (1.0f - (t1 + t1)) * vx + (t3 + t3) * vy + (t5 + t5) * vz;
                    vy = t4 + (1.0f - (t2 + t2)) * vy + (t7 + t7) * vz;
                    vz = t6 + t8 + (1.0f - (t9 + t9)) * vz;
                }
            }
            ASLgl::glEnable(enableTranslationTable[9]);
            ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[3]);
            currentscene->m_pLightManager->DisableBumpMapLights();
            if (bumpLights.m_count > 0) {
                for (int64_t k = 0; k < bumpLights.m_count; k++) {
                    wsNewSkinBumpyVP->Enable(part, false);
                    PartLight* pLight = bumpLights.m_pData[k];
                    float lx = pLight->m_vWorldPosition.x - vWS.x;
                    float ly = pLight->m_vWorldPosition.y - vWS.y;
                    float lz = pLight->m_vWorldPosition.z - vWS.z;
                    {
                        float qw = lq.w, qx = lq.x, qy = lq.y, qz = lq.z;
                        float nw = -qw;
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
                            t4 = (t4 + t4) * lx;
                            t6 = (t6 + t6) * lx;
                            t8 = (t8 + t8) * ly;
                            lx = (1.0f - (t1 + t1)) * lx + (t3 + t3) * ly + (t5 + t5) * lz;
                            ly = t4 + (1.0f - (t2 + t2)) * ly + (t7 + t7) * lz;
                            lz = t6 + t8 + (1.0f - (t9 + t9)) * lz;
                        }
                    }
                    {
                    Vector vDiff(pLight->m_vLightColour.x * (fAmbiX * diffuseintensity),
                                 pLight->m_vLightColour.y * (fAmbiY * diffuseintensity),
                                 fAmbiZ * diffuseintensity * pLight->m_vLightColour.z);
                    if (usebumpdiffuse == 0)
                        GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
                    else
                        GLRender::pfuncSetBumpLightColor(vDiff, 1.0f);
                    }
                    float fTheta = specularintensity;
                    float fSpec = pMat->Textures(4).m_pObject->GetBumpIntensity();
                    Vector vSpec(vSpecColor.x * fTheta * fSpec * pLight->m_vLightColour.x,
                                 vSpecColor.y * fTheta * fSpec * pLight->m_vLightColour.y,
                                 vSpecColor.z * fTheta * fSpec * pLight->m_vLightColour.z);
                    if (usebumpspecular == 0)
                        GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
                    else
                        GLRender::pfuncSetPixelShaderAmbientColor(vSpec, 1.0f);
                    if (usearbvertexprograms &&
                        (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, lx, ly, lz, 1.0f),
                         usearbvertexprograms) &&
                        (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, vx, vy, vz, 1.0f),
                         usearbvertexprograms)) {
                        float fR = pLight->m_fLightRadius * pLight->m_fLightRadius;
                        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 14, 1.0f, 0.0f,
                                                          9.0f / (fR + fR), 1.0f);
                    }
                    RenderFlat(vertices);
                }
            }
            if (bumpLights.m_count == 0) {
                wsNewSkinBumpyVP->Enable(part, false);
                GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
                GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
                RenderFlat(vertices);
            }
            if (AurMultiTextureAvailable())
                ASLgl::glClientActiveTextureARB(GL_TEXTURE1);
            GLRender::cm_uiCurrentStage = 1;
            ASLgl::glDisableClientState(nTexCoordState);
            if (AurMultiTextureAvailable())
                ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
            GLRender::cm_uiCurrentStage = 0;
            currentscene->DoFog();
            ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[2]);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
            if (usearbvertexprograms)
                ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 15, 1.0f, 1.0f, 1.0f, 1.0f);
            // Clean vertex program plus decal pass.
            static VertexProgram* skinCleanVP =
                new VertexProgram(usearbvertexprograms ? szSkinVPARB : cSkinVP,
                                  szNormalmapVPARBDeclaration);
            skinCleanVP->Enable(part, true);
            if (enabledecal) {
                if (AurMultiTextureAvailable())
                    ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
                GLRender::cm_uiCurrentStage = 0;
                pMat->BindTexture0();
                RenderFlat(vertices);
            }
            ASLgl::glEnable(enableTranslationTable[1]);
            currentscene->m_pLightManager->EnableBumpMapLights();
            ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
            goto lbl_free;
        }
    }
    if (pMat->HasBumpMap() && AurBumpMapCardValidate() && node->HasTangentBases() &&
        AurNV2xLevelShadersAvailable() && enablebumpmap) {
        // Old-bumpy ATI / force-sight arm.
        static VertexProgram* skinBumpyVP =
            new VertexProgram(usearbvertexprograms ? szSkinBumpyVPARB : cSkinBumpyVP,
                              cSkinBumpyVPDeclaration);
        (void)skinBumpyVP;
        if (part->m_bHologramRendering) {
            static VertexProgram* holoSkinBumpyVP =
                new VertexProgram(cHoloSkinVPARB, cSkinBumpyVPDeclaration);
            holoSkinBumpyVP->Enable(part, false);
            goto lbl_fsflat;
        }
        {
            Vector vWS, vWQ;
            float fAmbiX, fAmbiY, fAmbiZ;
            Vector vSpecColor;
            if (pNode12 != 0) {
                vSpecColor = pMat->Textures(4).m_pObject->GetSpecularColor();
                fAmbiX = pNode12->m_vDiffuse.x;
                fAmbiY = pNode12->m_vDiffuse.y;
                fAmbiZ = pNode12->m_vDiffuse.z;
            } else {
                fAmbiX = 0.0f; fAmbiY = 0.0f; fAmbiZ = 0.0f; vSpecColor = Vector(0.0f, 0.0f, 0.0f);
            }
            List<PartLight*> bumpLights(lm->GetBumpMapLights());
            GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmap, pMat->Textures(4).m_pObject,
                                              currentscene->m_pNormalizationCubeMap.m_pObject,
                                              currentscene->m_pNormalizationCubeMap.m_pObject,
                                              pMat->Textures(0).m_pObject);
            GLRender::pfuncSetPixelShaderAmbientColor(currentscene->m_ambient, 1.0f);
            Quaternion lq(1.0f, 0.0f, 0.0f, 0.0f);
            part->GetWorld(vWS, lq);
            float vx = CurrentCamera->m_vPosition.x - vWS.x;
            float vy = CurrentCamera->m_vPosition.y - vWS.y;
            float vz = CurrentCamera->m_vPosition.z - vWS.z;
            {
                float qw = lq.w, qx = lq.x, qy = lq.y, qz = lq.z;
                float nw = -qw;
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
                    t4 = (t4 + t4) * vx;
                    t6 = (t6 + t6) * vx;
                    t8 = (t8 + t8) * vy;
                    vx = (1.0f - (t1 + t1)) * vx + (t3 + t3) * vy + (t5 + t5) * vz;
                    vy = t4 + (1.0f - (t2 + t2)) * vy + (t7 + t7) * vz;
                    vz = t6 + t8 + (1.0f - (t9 + t9)) * vz;
                }
            }
            ASLgl::glEnable(enableTranslationTable[9]);
            // The blend pair is re-used by the closing glBlendFunc.
            unsigned int nBlendSrc = blendTranslationTable[0];
            unsigned int nBlendDst = blendTranslationTable[1];
            ASLgl::glBlendFunc(nBlendSrc, nBlendDst);
            if (bumpLights.m_count > 0) {
                for (int64_t k = 0; k < bumpLights.m_count; k++) {
                    currentscene->m_pLightManager->DisableBumpMapLights();
                    skinBumpyVP->Enable(part, true);
                    PartLight* pLight = bumpLights.m_pData[k];
                    float lx = pLight->m_vWorldPosition.x - vWS.x;
                    float ly = pLight->m_vWorldPosition.y - vWS.y;
                    float lz = pLight->m_vWorldPosition.z - vWS.z;
                    {
                        float qw = lq.w, qx = lq.x, qy = lq.y, qz = lq.z;
                        float nw = -qw;
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
                            t4 = (t4 + t4) * lx;
                            t6 = (t6 + t6) * lx;
                            t8 = (t8 + t8) * ly;
                            lx = (1.0f - (t1 + t1)) * lx + (t3 + t3) * ly + (t5 + t5) * lz;
                            ly = t4 + (1.0f - (t2 + t2)) * ly + (t7 + t7) * lz;
                            lz = t6 + t8 + (1.0f - (t9 + t9)) * lz;
                        }
                    }
                    {
                    Vector vDiff(pLight->m_vLightColour.x * (fAmbiX * diffuseintensity),
                                 pLight->m_vLightColour.y * (fAmbiY * diffuseintensity),
                                 fAmbiZ * diffuseintensity * pLight->m_vLightColour.z);
                    if (usebumpdiffuse == 0)
                        GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
                    else
                        GLRender::pfuncSetBumpLightColor(vDiff, 1.0f);
                    }
                    float fTheta = specularintensity;
                    float fSpec = pMat->Textures(4).m_pObject->GetBumpIntensity();
                    Vector vSpec(vSpecColor.x * fTheta * fSpec * pLight->m_vLightColour.x,
                                 vSpecColor.y * fTheta * fSpec * pLight->m_vLightColour.y,
                                 vSpecColor.z * fTheta * fSpec * pLight->m_vLightColour.z);
                    if (usebumpspecular == 0)
                        GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
                    else
                        GLRender::pfuncSetPixelShaderAmbientColor(vSpec, 1.0f);
                    if (usearbvertexprograms &&
                        (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, lx, ly, lz, 1.0f),
                         usearbvertexprograms) &&
                        (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, vx, vy, vz, 1.0f),
                         usearbvertexprograms)) {
                        float fR = pLight->m_fLightRadius * pLight->m_fLightRadius;
                        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 14, 1.0f, 0.0f,
                                                          9.0f / (fR + fR), 1.0f);
                    }
                    RenderFlat(vertices);
                    currentscene->m_pLightManager->EnableBumpMapLights();
                }
            }
            if (bumpLights.m_count == 0) {
                skinBumpyVP->Enable(part, true);
                GLRender::pfuncSetBumpLightColor(vZero, 1.0f);
                GLRender::pfuncSetPixelShaderAmbientColor(vZero, 1.0f);
                RenderFlat(vertices);
            }
            if (!usearbvertexprograms) {
                ASLgl::glDisableClientState(clientStateTranslationTable[8]);
                ASLgl::glDisableClientState(clientStateTranslationTable[9]);
                ASLgl::glDisableClientState(clientStateTranslationTable[10]);
            } else {
                ASLgl::glDisableVertexAttribArrayARB(12);
                ASLgl::glDisableVertexAttribArrayARB(13);
                ASLgl::glDisableVertexAttribArrayARB(14);
            }
            currentscene->DoFog();
            ASLgl::glBlendFunc(nBlendSrc, nBlendDst);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSBumpmapOff, 0, 0, 0, 0);
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
            goto lbl_free;
        }
    }
    // Env-map / fog / decal arm.
    {
        // The watched env texture is read off the part's gob, and HasEnvMap() implies Textures(5),
        // so the arm itself has no null test.
        CAurTexture* pEnv = 0;
        if (part->m_pGob != 0)
            pEnv = part->m_pGob->m_texWatch.m_pObject;
        if (pEnv != 0 || pMat->HasEnvMap()) {
            if (pEnv == 0)
                pEnv = pMat->Textures(5).m_pObject;
            // A hologram takes the holo cube/sphere program; otherwise the shared singleton.
            if (part->m_bHologramRendering) {
                static VertexProgram* holoCubeVP =
                    new VertexProgram(cHoloSkinVPARB, cSkinCubeMapVPDeclaration);
                static VertexProgram* holosphereVP =
                    new VertexProgram(cHoloSkinVPARB, cSkinSphereMapVPDeclaration);
                (pEnv->IsCubeMap() ? holoCubeVP : holosphereVP)->Enable(part, false);
                goto lbl_fsflat;
            }
            VertexProgram* pEnvVP = GetSkinCubeMapVP();
            if (AurATIValidate()) {
                if (currentscene->m_bFogEnabled == 0) {
                    ASLgl::glFogfv(enableTranslationTable[18], &vZero.x);
                    ASLgl::glFogf(enableTranslationTable[15], 1000.0f);
                    ASLgl::glFogf(enableTranslationTable[16], 2000.0f);
                }
#ifdef K2_QOL_IMPROVEMENTS
                // The PC fragment program fogs in the shader; Android's shader dialect already fogs
                // every draw, so turning fixed-function fog off here would leave the mesh unfogged.
                if (currentscene->m_bFogEnabled == 0)
#endif
                ASLgl::glDisable(enableTranslationTable[2]);
                const Vector& vFogColor = part->m_pGob->m_pScene->m_vFogColor;
                float afFogColor[4] = {vFogColor.x, vFogColor.y, vFogColor.z, 1.0f};
                if (AuroraUseARBShaders())
                    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afFogColor);
            }
            pEnvVP->Enable(part, true);
            bool bAti = AurATIValidate();
            RenderEnvironmentMapped(vertices, true, bAti);
            if (AurATIValidate()) {
                GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, pMat->Textures(0).m_pObject, 0, 0, 0);
                currentscene->DoFog();
            }
            goto lbl_epilogue;
        }
        // the fixed-function / skin path
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
        GLRender::cm_uiCurrentStage = 0;
        pMat->BindTexture0();
        if (AurATIValidate() && currentscene->m_bFogEnabled != 0) {
#ifndef K2_QOL_IMPROVEMENTS   // see the environment-mapped arm above
            ASLgl::glDisable(enableTranslationTable[2]);
#endif
            GLRender::pfuncEnablePixelShaders(GLRender::kPSStandardFog, pMat->Textures(0).m_pObject, 0, 0, 0);
            const Vector& vFogColor = part->m_pGob->m_pScene->m_vFogColor;
            float afFogColor[4] = {vFogColor.x, vFogColor.y, vFogColor.z, 1.0f};
            if (AuroraUseARBShaders())
                ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afFogColor);
        }
        VertexProgram* pSkinVP;
        if (use3boneskinning == 0) {
            pSkinVP = GetSkinVP(part->m_bHologramRendering);
        } else {
            static VertexProgram* skin3boneVP =
                new VertexProgram(cSkin3BoneVP, cSkin3BoneVPDeclaration);
            pSkinVP = skin3boneVP;
        }
        pSkinVP->Enable(part, true);
        if (pMat->HasBumpMap() && pNode12->HasTangentBases() && !AurNV2xLevelShadersAvailable()) {
            ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[3]);
            RenderFlat(vertices);
            ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
        } else if (pMat->HasBumpMap() && pNode12->HasTangentBases() && !AurBumpMapCardValidate()) {
            // ATI bump pass
            GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, pMat->Textures(0).m_pObject, 0, 0, 0);
            ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[3]);
            if (g_ForceSight != 0)
                SetupForceSightShader(pGob, GLRender::kPSFGlow, GLRender::kPSDesat, 0, 0, 0, 0, false);
            RenderFlat(vertices);
            if (g_ForceSight != 0) {
                GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
                ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
                if (!g_useDistortionOutput)
                    ASLgl::glDepthMask(1);
            }
            ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
        } else {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, pMat->Textures(0).m_pObject, 0, 0, 0);
            if (part->m_bHologramRendering) {
                if (g_ForceSight == 0) {
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSHolo, 0, 0, 0, 0);
                    ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[0]);
                } else {
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSHoloDesat, 0, 0, 0, 0);
                    ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[2]);
                }
                RenderFlat(vertices);
                GLRender::pfuncEnablePixelShaders(GLRender::kPSHoloOff, 0, 0, 0, 0);
                ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
            } else {
                if (g_ForceSight != 0)
                    SetupForceSightShader(pGob, GLRender::kPSFGlow, GLRender::kPSDesat, 0, 0, 0, 0, false);
                RenderFlat(vertices);
                if (g_ForceSight != 0) {
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
                    ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
                    if (!g_useDistortionOutput)
                        ASLgl::glDepthMask(1);
                }
            }
        }
        if (AurATIValidate()) {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, pMat->Textures(0).m_pObject, 0, 0, 0);
            currentscene->DoFog();
        }
    }
lbl_free:
    goto lbl_epilogue;

lbl_fsflat:
    // Shared hologram force-sight flat pass; the vertex program is already enabled.
    if (g_ForceSight != 0) {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSHoloDesat, 0, 0, 0, 0);
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[2]);
    } else {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSHolo, 0, 0, 0, 0);
        ASLgl::glBlendFunc(blendTranslationTable[2], blendTranslationTable[0]);
    }
    RenderFlat(vertices);
    GLRender::pfuncEnablePixelShaders(GLRender::kPSHoloOff, 0, 0, 0, 0);
    ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);

lbl_epilogue:
    if (usearbvertexprograms) {
        ASLgl::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
        ASLgl::glDisable(GL_VERTEX_PROGRAM_ARB);
    } else {
        ASLgl::glDisable(enableTranslationTable[9]);
    }
}
#endif

// Renders through the motion-blur vertex-program path: gathers the bone frames, then uploads the
// camera-relative blur vector and the motion-blur side axis to the vertex-program constants.
void RenderSkinnedVPBlur(VertexPrimitiveSkinned* vertices)
{
    PartTriMesh* pTri = vertices->m_pPart->AsPartSkin();
    MdlNodeSkin* node = static_cast<MdlNodeSkin*>(pTri->m_pNode);
    if (node->m_nTexCoordSets != 0) {                            // zero coordinate sets skip the body and the epilogue
    pTri->AsPartTriMesh();   // result discarded

    Gob* pGob = pTri->m_pGob;
    Vector vBlur(pGob->m_vPosition.x - pGob->m_vPreviousPosition.x,
                 pGob->m_vPosition.y - pGob->m_vPreviousPosition.y,
                 pGob->m_vPosition.z - pGob->m_vPreviousPosition.z);
    float fMag = magnitude(vBlur);

    if (fMag > 0.0f) {
        vertices->SetupVertexArraysStandard();

        static List<Quaternion> qbones;
        static List<Vector> tbones;
        qbones.m_count = 0;
        tbones.m_count = 0;

#if !KOTOR1  // K1 always disables depth writes
        if (!g_useDistortionOutput)
#endif
            ASLgl::glDepthMask(0);
        ASLgl::glEnable(enableTranslationTable[9]);

        // Gather the world frame: the part's own frame, then up the parent chain.
        Part* pTop = pTri;
#if KOTOR1  // the same walk with the quaternion-inverse operator
        Quaternion qInv = -pTri->m_qLocalOrientation;
        Vector vPos = RotateVector(-pTri->m_vLocalPosition, qInv);
        Quaternion qAcc = -pTri->m_qLocalOrientation;
        for (Part* pParent = static_cast<Part*>(pTri->m_pParent); pParent != 0; pParent = static_cast<Part*>(pParent->m_pParent)) {
            pTop = pParent;
            Quaternion qParentInv = -pParent->m_qLocalOrientation;
            Vector p = RotateVector(-pParent->m_vLocalPosition, qParentInv);
            vPos = vPos + RotateVector(p, qAcc);
            qAcc = qAcc * -pParent->m_qLocalOrientation;
        }
        GatherBones(pTop, tbones, qbones, vPos, qAcc);
#else
        float px, py, pz;               // accumulated translation (world from local, transposed rotation)
        float fX, fY, fZ, fW;           // accumulated quaternion (w component stored negated)
        {
            float qw = pTri->m_qLocalOrientation.w;
            float qx = pTri->m_qLocalOrientation.x;
            float qy = pTri->m_qLocalOrientation.y;
            float qz = pTri->m_qLocalOrientation.z;
            float nw = -qw;
            fW = nw; fX = qx; fY = qy; fZ = qz;
            px = -pTri->m_vLocalPosition.x;
            py = -pTri->m_vLocalPosition.y;
            pz = -pTri->m_vLocalPosition.z;
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
                t4 = (t4 + t4) * px;
                t6 = (t6 + t6) * px;
                t8 = (t8 + t8) * py;
                px = (1.0f - (t1 + t1)) * px + (t3 + t3) * py + (t5 + t5) * pz;
                py = t4 + (1.0f - (t2 + t2)) * py + (t7 + t7) * pz;
                pz = t6 + t8 + (1.0f - (t9 + t9)) * pz;
            }
        }
        Part* pParent = static_cast<Part*>(pTri->m_pParent);
        while (pParent != 0) {
            Part* pCur = pParent;
            float qw = pCur->m_qLocalOrientation.w;
            float qx = pCur->m_qLocalOrientation.x;
            float qy = pCur->m_qLocalOrientation.y;
            float qz = pCur->m_qLocalOrientation.z;
            float nw = -qw;
            float mx = -pCur->m_vLocalPosition.x;
            float my = -pCur->m_vLocalPosition.y;
            float mz = -pCur->m_vLocalPosition.z;
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
                t4 = (t4 + t4) * mx;
                t6 = (t6 + t6) * mx;
                t8 = (t8 + t8) * my;
                mx = (1.0f - (t1 + t1)) * mx + (t3 + t3) * my + (t5 + t5) * mz;
                my = t4 + (1.0f - (t2 + t2)) * my + (t7 + t7) * mz;
                mz = t6 + t8 + (1.0f - (t9 + t9)) * mz;
            }
            if (quaternionidentityoptimization == 0 || (fW != -1.0f && fW != 1.0f)) {
                float t1 = fY * fY + fZ * fZ;
                float t2 = fX * fX + fZ * fZ;
                float t3 = fX * fY - fW * fZ;
                float t4 = fX * fY + fW * fZ;
                float t5 = fW * fY + fX * fZ;
                float t6 = fX * fZ - fW * fY;
                float t7 = fY * fZ - fW * fX;
                float t8 = fW * fX + fY * fZ;
                float t9 = fX * fX + fY * fY;
                t4 = (t4 + t4) * mx;
                t6 = (t6 + t6) * mx;
                t8 = (t8 + t8) * my;
                mx = (1.0f - (t1 + t1)) * mx + (t3 + t3) * my + (t5 + t5) * mz;
                my = t4 + (1.0f - (t2 + t2)) * my + (t7 + t7) * mz;
                mz = t6 + t8 + (1.0f - (t9 + t9)) * mz;
            }
            px += mx;
            py += my;
            pz += mz;
            float aw = fW, ax = fX, ay = fY, az = fZ;
            float u1 = aw * qx;
            float u2 = aw * qy;
            float u3 = ax * qz;
            float u4 = aw * qz;
            float u5 = ax * qy;
            float u6 = ay * qx;
            fW = aw * nw - ax * qx - ay * qy - az * qz;
            fX = ax * nw + u1 + ay * qz - az * qy;
            fY = (u2 - u3) + ay * nw + az * qx;
            fZ = az * nw + ((u5 + u4) - u6);
            pTop = pCur;
            pParent = static_cast<Part*>(pCur->m_pParent);
        }
        GatherBones(pTop, tbones, qbones, Vector(px, py, pz), Quaternion(fW, fX, fY, fZ));
#endif

        // For each bone, rotate the bone position and upload the rotation rows as constants.
        if (node->m_nBoneCount >= 1) {
            for (int64_t i = 0; i < node->m_nBoneCount; i++) {
                int nBone = static_cast<int>(node->m_paBoneMap[i]);   // the bone index is stored as a float
                if (nBone != -1) {
                    Quaternion q = qbones.m_pData[i];
                    Vector src = node->m_lTBoneRefInv.m_pData[i];
                    Vector r;
                    if (quaternionidentityoptimization == 0 || (q.w != -1.0f && q.w != 1.0f)) {
                        float w = q.w, x = q.x, y = q.y, z = q.z;
                        float t1 = y * y + z * z;
                        float t2 = x * x + z * z;
                        float t3 = x * y - w * z;
                        float t4 = x * y + w * z;
                        float t5 = w * y + x * z;
                        float t6 = x * z - w * y;
                        float t7 = y * z - w * x;
                        float t8 = w * x + y * z;
                        float t9 = x * x + y * y;
                        r.x = (1.0f - (t1 + t1)) * src.x + (t3 + t3) * src.y + (t5 + t5) * src.z;
                        r.y = (t4 + t4) * src.x + (1.0f - (t2 + t2)) * src.y + (t7 + t7) * src.z;
                        r.z = (t6 + t6) * src.x + (t8 + t8) * src.y + (1.0f - (t9 + t9)) * src.z;
                    } else {
                        r = src;
                    }
                    tbones.m_pData[i] = tbones.m_pData[i] + r;
                    qbones.m_pData[i] = qbones.m_pData[i] * node->m_lQBoneRefInv.m_pData[i];
                    if (usearbvertexprograms) {
                        int nIdx = nBone * 3;
                        Quaternion q2 = qbones.m_pData[i];
                        float w2 = q2.w, x2 = q2.x, y2 = q2.y, z2 = q2.z;
                        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIdx + 18,
                            1.0f - ((y2 * y2 + z2 * z2) + (y2 * y2 + z2 * z2)),
                            (y2 * x2 - z2 * w2) + (y2 * x2 - z2 * w2),
                            (z2 * x2 + y2 * w2) + (z2 * x2 + y2 * w2), tbones.m_pData[i].x);
                        if (usearbvertexprograms) {
                            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIdx + 19,
                                (y2 * x2 + z2 * w2) + (y2 * x2 + z2 * w2),
                                1.0f - ((z2 * z2 + x2 * x2) + (z2 * z2 + x2 * x2)),
                                (y2 * z2 - x2 * w2) + (y2 * z2 - x2 * w2), tbones.m_pData[i].y);
                            if (usearbvertexprograms) {
                                ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, nIdx + 20,
                                    (z2 * x2 - y2 * w2) + (z2 * x2 - y2 * w2),
                                    (y2 * z2 + x2 * w2) + (y2 * z2 + x2 * w2),
                                    1.0f - ((y2 * y2 + x2 * x2) + (y2 * y2 + x2 * x2)),
                                    tbones.m_pData[i].z);
                            }
                        }
                    }
                }
            }
        }

        // Motion-blur view transform.
        static VertexProgram* skinmotionblurvp =
            new VertexProgram(cSkinMotionBlurVP, cSkinMotionBlurVPDeclaration);

        Vector vWorldPos;
        Quaternion qWorld;
        pTri->GetWorld(vWorldPos, qWorld);

#if KOTOR1  // the four world-frame rotations via the quaternion-inverse operator
        Quaternion qWorldInv = -qWorld;
        Vector vCam = RotateVector(CurrentCamera->m_vPosition - vWorldPos, qWorldInv);
        float vx = vCam.x, vy = vCam.y, vz = vCam.z;

        float b0 = pGob->m_qOrientation.w, b1 = pGob->m_qOrientation.x,
              b2 = pGob->m_qOrientation.y, b3 = pGob->m_qOrientation.z;
        float g1 = (b1 * b3 + b0 * b2) + (b1 * b3 + b0 * b2);
        float g2 = (b3 * b2 - b1 * b0) + (b3 * b2 - b1 * b0);
        float nu = 1.0f - ((b1 * b1 + b2 * b2) + (b1 * b1 + b2 * b2));
        Vector vSide(vBlur.y * nu - vBlur.z * g2, vBlur.z * g1 - vBlur.x * nu,
                     g2 * vBlur.x - vBlur.y * g1);
        qWorldInv = -qWorld;
        vBlur = RotateVector(vBlur, qWorldInv);
        qWorldInv = -qWorld;
        vSide = RotateVector(vSide, qWorldInv);

        float fDT = DeltaT;
        qWorldInv = -qWorld;
        Vector vCam2 = RotateVector(pGob->m_vPosition - vWorldPos, qWorldInv);
#else
        float fDT = DeltaT;

        // 1) The camera-relative vector, rotated into the world frame.
        float vx = CurrentCamera->m_vPosition.x - vWorldPos.x;
        float vy = CurrentCamera->m_vPosition.y - vWorldPos.y;
        float vz = CurrentCamera->m_vPosition.z - vWorldPos.z;
        {
            float qw = qWorld.w, qx = qWorld.x, qy = qWorld.y, qz = qWorld.z;
            float nw = -qw;
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
                t4 = (t4 + t4) * vx;
                t6 = (t6 + t6) * vx;
                t8 = (t8 + t8) * vy;
                vx = (1.0f - (t1 + t1)) * vx + (t3 + t3) * vy + (t5 + t5) * vz;
                vy = t4 + (1.0f - (t2 + t2)) * vy + (t7 + t7) * vz;
                vz = t6 + t8 + (1.0f - (t9 + t9)) * vz;
            }
        }

        // 2) The blur vector in the world frame, with the motion-blur side axis (its cross product
        //    with the orientation's third basis column) uploaded as a constant.
        float b0 = pGob->m_qOrientation.w, b1 = pGob->m_qOrientation.x,
              b2 = pGob->m_qOrientation.y, b3 = pGob->m_qOrientation.z;
        float g1 = (b1 * b3 + b0 * b2) + (b1 * b3 + b0 * b2);      // 2(b1 b3 + b0 b2)
        float g2 = (b3 * b2 - b1 * b0) + (b3 * b2 - b1 * b0);      // 2(b2 b3 - b0 b1)
        float nu = 1.0f - ((b1 * b1 + b2 * b2) + (b1 * b1 + b2 * b2));  // 1 - 2*(b1*b1 + b2*b2)
        Vector vSide(vBlur.y * nu - vBlur.z * g2, vBlur.z * g1 - vBlur.x * nu,
                     g2 * vBlur.x - vBlur.y * g1);
        {
            float qw = qWorld.w, qx = qWorld.x, qy = qWorld.y, qz = qWorld.z;
            float nw = -qw;
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
                float rbx = (1.0f - (t1 + t1)) * vBlur.x + (t3 + t3) * vBlur.y + (t5 + t5) * vBlur.z;
                float rby = (t4 + t4) * vBlur.x + (1.0f - (t2 + t2)) * vBlur.y + (t7 + t7) * vBlur.z;
                float rbz = (t6 + t6) * vBlur.x + (t8 + t8) * vBlur.y + (1.0f - (t9 + t9)) * vBlur.z;
                vBlur.x = rbx; vBlur.y = rby; vBlur.z = rbz;
                float rsx = (1.0f - (t1 + t1)) * vSide.x + (t3 + t3) * vSide.y + (t5 + t5) * vSide.z;
                float rsy = (t4 + t4) * vSide.x + (1.0f - (t2 + t2)) * vSide.y + (t7 + t7) * vSide.z;
                float rsz = (t6 + t6) * vSide.x + (t8 + t8) * vSide.y + (1.0f - (t9 + t9)) * vSide.z;
                vSide.x = rsx; vSide.y = rsy; vSide.z = rsz;
            }
        }

        // 3) The object's offset from the camera, rotated the same way.
        float wx = pGob->m_vPosition.x - vWorldPos.x;
        float wy = pGob->m_vPosition.y - vWorldPos.y;
        float wz = pGob->m_vPosition.z - vWorldPos.z;
        {
            float qw = qWorld.w, qx = qWorld.x, qy = qWorld.y, qz = qWorld.z;
            float nw = -qw;
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
                t4 = (t4 + t4) * wx;
                t6 = (t6 + t6) * wx;
                t8 = (t8 + t8) * wy;
                wx = (1.0f - (t1 + t1)) * wx + (t3 + t3) * wy + (t5 + t5) * wz;
                wy = t4 + (1.0f - (t2 + t2)) * wy + (t7 + t7) * wz;
                wz = t6 + t8 + (1.0f - (t9 + t9)) * wz;
            }
        }
        Vector vCam2(wx, wy, wz);
#endif

        if (usearbvertexprograms) {
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 16, 0.0f, 2.0f, 3.0f, 1.0f);
            if (usearbvertexprograms) {
                ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 17, vCam2.x, vCam2.y, vCam2.z, 0.15f);
            }
        }

        vBlur = normalize(vBlur);
        Vector vNorm = vBlur;
        if (usearbvertexprograms &&
            (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, vNorm.x, vNorm.y, vNorm.z, 1.0f),
             usearbvertexprograms) &&
            (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 14, fMag / (fDT + fDT), pGob->m_fMaxBlurLength,
                                               0.01f, 0.85f),
             usearbvertexprograms) &&
            (ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 69, vSide.x, vSide.y, vSide.z, 0.5f),
             usearbvertexprograms)) {
            ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, 0.0f, 0.0f, 0.0f, 0.0f);
        }

        // Squared-distance front-face flip test.
        float dx = vCam2.x - vx;         // the gob's offset from the camera, both in the world frame
        float dy = vCam2.y - vy;
        float dz = vCam2.z - vz;
        float fDot = dx * vNorm.x + dy * vNorm.y + dz * vNorm.z;
        if ((dx * dx + dy * dy + dz * dz) * 0.75f * 0.75f <
            (vNorm.z * fDot) * (vNorm.z * fDot) + (vNorm.x * fDot) * (vNorm.x * fDot) +
                (vNorm.y * fDot) * (vNorm.y * fDot)) {
            Vector vSum(dx + vNorm.x, dy + vNorm.y, dz + vNorm.z);
            Vector vDif(dx - vNorm.x, dy - vNorm.y, dz - vNorm.z);
            float fM1 = magnitude(vSum);
            float fM2 = magnitude(vDif);
            if (fM2 < fM1) {
                if (usearbvertexprograms) {
                    ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, 1.0f, 0.0f, 0.0f, 0.0f);
                }
                ASLgl::glFrontFace(cullTranslationTable[0]);               // GL_CW
            }
        }

        GLRender::pfuncEnablePixelShaders(GLRender::kPSMotionBlur, pTri->m_pMaterial->Textures(0).m_pObject, 0, 0, 0);
        skinmotionblurvp->Enable(pTri, true);
        RenderFlat(vertices);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
    }
    ASLgl::glFrontFace(cullTranslationTable[1]);                           // GL_CCW
    ASLgl::glDisable(enableTranslationTable[9]);
    }
}

// PartSkin, the skinned part flavour, and the MdlNodeSkin draw node.

class CAurTexture;
class VertexProgram;

// GL backend shims.
extern unsigned int clientStateTranslationTable[];

// Vendor capability gates.
bool AurNV2xLevelShadersAvailable();
bool AurNonRadeon8500Validate();

// Globals shared with the other engine draw paths.
int renderskin = 1;
extern int  usevertexprograms;
extern int  doskinshadows;
extern int  useshadowmethod;
extern float epsilon;
extern int  usearbvertexprograms;
extern unsigned int enableTranslationTable[];
extern unsigned int blendTranslationTable[];
extern const unsigned int cullTranslationTable[];
extern unsigned int stencilOpTranslationTable[];
class Scene;
extern unsigned char g_ForceSight;
extern unsigned char g_useDistortionOutput;
extern int  quaternionidentityoptimization;
VertexProgram* g_pSkinBumpedOutVP;
// The tangent-bases variant, chosen when the mesh node carries tangent bases.
VertexProgram* g_pSkinBumpedOutWithTangentVP;

// Helpers the draw family drives.
void RenderSkinned(VertexPrimitiveSkinned* p, bool bumped);
void RenderSkinnedVP(VertexPrimitiveSkinned* p);
void RenderSkinnedVPBlur(VertexPrimitiveSkinned* p);
void RenderFlat(VertexPrimitiveFlat* p);
// Clips a ray against a plane, given the plane's a/b/c components plus its d offset, a ray start
// and a cap point.
Vector planelineintersection(const Vector& v, float f, const Vector& a, const Vector& b);
void SetBumpedOutShellIos(float f);
void SetupForceSightShader(Gob* gob, int a, int b, CAurTexture* c1, CAurTexture* c2,
                           CAurTexture* c3, CAurTexture* c4, bool flag);
// Recursive bone-frame accumulator shared with the shell render.
void GatherBones(Part* part, List<Vector>& outVectors, List<Quaternion>& outQuats,
                 Vector pos, Quaternion orient);

// PostProcess's private helpers and the C runtime.
int FindPartIndexDFS(Part* part, char* pszName);
void MakeInlinePartList(Part* part, List<Part*>& out);
#include "port/libcstring.h"

// The skin node's parser-table factory.
Part* MdlNodeSkin::InternalCreateInstance(Gob* pGob)
{
    PartSkin* pPart = new PartSkin(this);
    pGob->m_cHasSkin = 1;
    return pPart;
}

// Recursive bone-frame accumulator shared with the shell render and PostProcess.
void GatherBones(Part* part, List<Vector>& outVectors, List<Quaternion>& outQuats,
                 Vector pos, Quaternion orient)
{
    float w = orient.w, x = orient.x, y = orient.y, z = orient.z;
    Vector rotated;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        rotated.x = part->m_vLocalPosition.x * (1.0f - ((yy + zz) + (yy + zz))) +
                    part->m_vLocalPosition.y * ((xy - wz) + (xy - wz)) +
                    part->m_vLocalPosition.z * ((xz + wy) + (xz + wy));
        rotated.y = part->m_vLocalPosition.x * ((xy + wz) + (xy + wz)) +
                    part->m_vLocalPosition.y * (1.0f - ((xx + zz) + (xx + zz))) +
                    part->m_vLocalPosition.z * ((yz - wx) + (yz - wx));
        rotated.z = part->m_vLocalPosition.x * ((xz - wy) + (xz - wy)) +
                    part->m_vLocalPosition.y * ((yz + wx) + (yz + wx)) +
                    part->m_vLocalPosition.z * (1.0f - ((xx + yy) + (xx + yy)));
    } else {
        rotated = part->m_vLocalPosition;
    }
    Vector posOut(pos.x + rotated.x, pos.y + rotated.y, pos.z + rotated.z);
    Quaternion qat = part->m_qLocalOrientation;
    Quaternion orientOut = orient * qat;
    outVectors.Add(posOut);
    outQuats.Add(orientOut);
    for (int i = 0; i < part->m_nChildren; i++)
        GatherBones(part->m_paChildren[i], outVectors, outQuats, posOut, orientOut);
}

// Member instantiations only.
template void List<Vector>::Add(Vector item);
template void List<Quaternion>::Add(Quaternion item);

#include "port/crtapi.h"

void MakeInlinePartList(Part* pPart, List<Part*>& listParts)
{
    listParts.Add(pPart);
    for (int64_t i = 0; i < pPart->m_nChildren; i++)
        MakeInlinePartList(pPart->m_paChildren[i], listParts);
}

int FindPartIndexDFS(Part* pPart, char* szName)
{
    List<Part*> listParts;
    int nRet = -1;
    if (!_stricmp("root", szName))
        return 0;

    MakeInlinePartList(pPart, listParts);

    for (int64_t i = 0; i < listParts.m_count; i++) {
            if (!_stricmp(listParts.m_pData[i]->Name(), szName)) {
                nRet = static_cast<int>(i);
                break;
            }
        }
    return nRet;
}

void MakeInlineNodeList(MdlNode* pNode, List<MdlNode*>& listNodes)
{
    listNodes.Add(pNode);
    for (int64_t i = 0; i < pNode->m_listChildren.m_count; i++)
        MakeInlineNodeList(pNode->m_listChildren.m_pData[i], listNodes);
}

int FindNodeIndexDFS(MdlNode* pNode, char* szName)
{
    int nRet = -1;
    List<MdlNode*> listNodes;
    if (!_stricmp("root", szName))
        return 0;

    MakeInlineNodeList(pNode, listNodes);

    for (int64_t i = 0; i < listNodes.m_count; i++) {
            MdlNode* pCur = listNodes.m_pData[i];
// The tree is either a model or an animation whose model owns the name table.
            Model* pModel = pCur->m_pTree->AsModel() ? pCur->m_pTree->AsModel()
                                                     : pCur->m_pTree->AsAnimation()->m_pModel;
            if (!_stricmp(pModel->m_listStrings.m_pData[pCur->m_nNameIndex], szName)) {
                nRet = static_cast<int>(i);
                break;
            }
        }
    return nRet;
}

// Renders through the RenderSkinned family, then the bump-outlined shell pass for a bump part.
void PartSkin::Draw(bool b)
{
    if (renderskin == 0)
        return;

    if (usevertexprograms && AurNV2xLevelShadersAvailable() && currentscene &&
        AurNonRadeon8500Validate()) {
        VertexPrimitiveSkinned* p = m_pVertexPrimitive->AsVertexPrimitiveSkinned();
        if (b)
            RenderSkinnedVPBlur(p);
        else
            RenderSkinnedVP(p);
#if !KOTOR1  // K1 leaves attribute array 1 enabled
        ASLgl::glDisableVertexAttribArrayARB(1);
#endif
    } else {
        ASLgl::glDisableClientState(clientStateTranslationTable[2]);
        VertexPrimitiveSkinned* p = m_pVertexPrimitive->AsVertexPrimitiveSkinned();
        RenderSkinned(p, false);
    }

    if (m_pGob != 0 && m_pGob->m_fBumpOutDistance != 0.0f && currentscene != 0) {
        if (AurNV2xLevelShadersAvailable() && AurNonRadeon8500Validate()) {
            DrawBumpedOutShellVP();
            return;
        }
        RenderSkinned(m_pVertexPrimitive->AsVertexPrimitiveSkinned(), true);
        return;
    }
}

void PartSkin::DrawBumpedOutShell()
{
    if (AurNV2xLevelShadersAvailable() != 0 && AurNonRadeon8500Validate() != 0) {
        DrawBumpedOutShellVP();
        return;
    }
    RenderSkinned(m_pVertexPrimitive->AsVertexPrimitiveSkinned(), true);
}

// Bump-outlined shell render through the ARB vertex-program path.
void PartSkin::DrawBumpedOutShellVP()
{
    GLRender::cm_uiCurrentStage = 0;
    if (AurMultiTextureAvailable() != 0)
        ASLgl::glActiveTextureARB(GLRender::cm_uiCurrentStage + GL_TEXTURE0);

    m_pMaterial->Textures(3).m_pObject->Bind();
    if (m_pMaterial->m_lstSrcBlend.m_count >= 2)
        m_pMaterial->SetBlendingMode(1);

    VertexPrimitiveSkinned* pv = m_pVertexPrimitive->AsVertexPrimitiveSkinned();
    Part* part = pv->m_pPart;
    MdlNodeSkin* node = static_cast<MdlNodeSkin*>(part->AsPartSkin()->m_pNode);
    if (node->m_nTexCoordSets != 0) {
    MdlNodeTriMesh* trinode = part->AsPartTriMesh() ? static_cast<MdlNodeTriMesh*>(part->m_pNode) : 0;

    pv->SetupVertexArraysStandard();

    static List<Quaternion> s_quats;
    static List<Vector> s_vectors;
    s_quats.m_count = 0;
    s_vectors.m_count = 0;

    ASLgl::glEnable(enableTranslationTable[9]);

    if (usearbvertexprograms != 0) {
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 16, 0.0f, 2.0f, 3.0f, 1.0f);
        if (usearbvertexprograms != 0) {
            float fShade = 0.0f;
            if (currentscene->m_bFogEnabled != 0)
                fShade = 1.0f;
            ASLgl::glProgramEnvParameter4fARB(
                GL_VERTEX_PROGRAM_ARB, 17,
                currentscene->m_fFogStart,
                currentscene->m_fFogEnd,
                fShade, 0.5f);
        }
    }

    // Gather the world frame: the part's own frame, then up the parent chain.
    Part* pTop = part;
#if KOTOR1  // the same walk with the quaternion-inverse operator
    Quaternion qInv = -part->m_qLocalOrientation;
    Vector vPos = RotateVector(-part->m_vLocalPosition, qInv);
    Quaternion qAcc = -part->m_qLocalOrientation;
    for (Part* pParent = static_cast<Part*>(part->m_pParent); pParent != 0; pParent = static_cast<Part*>(pParent->m_pParent)) {
        pTop = pParent;
        Quaternion qParentInv = -pParent->m_qLocalOrientation;
        Vector p = RotateVector(-pParent->m_vLocalPosition, qParentInv);
        vPos = vPos + RotateVector(p, qAcc);
        qAcc = qAcc * -pParent->m_qLocalOrientation;
    }
    GatherBones(pTop, s_vectors, s_quats, vPos, qAcc);
#else
    float f17, f18, f20;               // accumulated position
    float f19, f21, f22, f23;          // accumulated quaternion (w component stored negated)
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
        float nw = -qw;
        float px = -pCur->m_vLocalPosition.x;
        float py = -pCur->m_vLocalPosition.y;
        float pz = -pCur->m_vLocalPosition.z;
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
            t4 = (t4 + t4) * px;
            t6 = (t6 + t6) * px;
            t8 = (t8 + t8) * py;
            px = (1.0f - (t1 + t1)) * px + (t3 + t3) * py + (t5 + t5) * pz;
            py = t4 + (1.0f - (t2 + t2)) * py + (t7 + t7) * pz;
            pz = t6 + t8 + (1.0f - (t9 + t9)) * pz;
        }
        if (quaternionidentityoptimization == 0 || (f21 != -1.0f && f21 != 1.0f)) {
            float t1 = f22 * f22 + f23 * f23;
            float t2 = f19 * f19 + f23 * f23;
            float t3 = f19 * f22 - f21 * f23;
            float t4 = f19 * f22 + f21 * f23;
            float t5 = f21 * f22 + f19 * f23;
            float t6 = f19 * f23 - f21 * f22;
            float t7 = f22 * f23 - f21 * f19;
            float t8 = f21 * f19 + f22 * f23;
            float t9 = f19 * f19 + f22 * f22;
            t4 = (t4 + t4) * px;
            t6 = (t6 + t6) * px;
            t8 = (t8 + t8) * py;
            px = (1.0f - (t1 + t1)) * px + (t3 + t3) * py + (t5 + t5) * pz;
            py = t4 + (1.0f - (t2 + t2)) * py + (t7 + t7) * pz;
            pz = t6 + t8 + (1.0f - (t9 + t9)) * pz;
        }
        f17 = f17 + px;
        f18 = f18 + py;
        f20 = f20 + pz;
        float aw = f21, ax = f19, ay = f22, az = f23;
        float u1 = aw * qx;
        float u2 = aw * qy;
        float u3 = ax * qz;
        float u4 = aw * qz;
        float u5 = ax * qy;
        float u6 = ay * qx;
        f21 = aw * nw - ax * qx - ay * qy - az * qz;
        f19 = ax * nw + u1 + ay * qz - az * qy;
        f22 = (u2 - u3) + ay * nw + az * qx;
        f23 = az * nw + ((u5 + u4) - u6);
        pTop = pCur;
        pParent = static_cast<Part*>(pCur->m_pParent);
    }
    GatherBones(pTop, s_vectors, s_quats,
                Vector(f17, f18, f20),
                Quaternion(f21, f19, f22, f23));
#endif

    // For each bone, transform the bone frame and feed the vertex-program constants.
    if (0 < node->m_nBoneCount) {
        int64_t i = 0;
        do {
            int bone = static_cast<int>(node->m_paBoneMap[i]);
            if (bone != -1) {
                const Quaternion& qa = s_quats.m_pData[i];
                float aw = qa.w;
                float rx, ry, rz;
                if (quaternionidentityoptimization == 0 || (aw != 1.0f && aw != -1.0f)) {
                    float ax = qa.x, ay = qa.y, az = qa.z;
                    const Vector& vb = node->m_lTBoneRefInv.m_pData[i];
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
                    const Vector& vb = node->m_lTBoneRefInv.m_pData[i];
                    rx = vb.x; ry = vb.y; rz = vb.z;
                }
                Vector& sv = s_vectors.m_pData[i];
                sv.x = rx + sv.x;
                sv.y = ry + sv.y;
                sv.z = rz + sv.z;
                Quaternion* pAccQuat = &s_quats.m_pData[i];
                *pAccQuat = *pAccQuat * node->m_lQBoneRefInv.m_pData[i];
                if (usearbvertexprograms != 0) {
                    float av = pAccQuat->w, bx = pAccQuat->x, by = pAccQuat->y, bz = pAccQuat->z;
                    const Vector& vr = s_vectors.m_pData[i];
                    int ib = bone * 3;
                    float m11 = by * by + bz * bz;
                    float m12 = by * bx - bz * av;
                    float m13 = bz * bx + by * av;
                    ASLgl::glProgramEnvParameter4fARB(
                        GL_VERTEX_PROGRAM_ARB, ib + 18, 1.0f - (m11 + m11), m12 + m12, m13 + m13, vr.x);
                    if (usearbvertexprograms != 0) {
                        float m21 = by * bx + bz * av;
                        float m22 = bz * bz + bx * bx;
                        float m23 = by * bz - bx * av;
                        ASLgl::glProgramEnvParameter4fARB(
                            GL_VERTEX_PROGRAM_ARB, ib + 19, m21 + m21, 1.0f - (m22 + m22), m23 + m23, vr.y);
                        if (usearbvertexprograms != 0) {
                            float m31 = bz * bx - by * av;
                            float m32 = by * bz + bx * av;
                            float m33 = by * by + bx * bx;
                            ASLgl::glProgramEnvParameter4fARB(
                                GL_VERTEX_PROGRAM_ARB, ib + 20, m31 + m31, m32 + m32,
                                1.0f - (m33 + m33), vr.z);
                        }
                    }
                }
            }
            i++;
        } while (i < node->m_nBoneCount);
    }

    if (usearbvertexprograms != 0)
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, m_pGob->m_fBumpOutDistance, 0.0f, 0.0f, 1.0f);
#if !KOTOR1  // K1 binds no pixel shader for the shell
    (*GLRender::pfuncEnablePixelShaders)(GLRender::kPSDefault,
        m_pMaterial->Textures(3).m_pObject,
        0, 0, 0);
#endif
    SetBumpedOutShellIos(m_pGob->m_fBumpOutDistance);
    {
        float local[3] = {1.0f, 1.0f, 1.0f};
        (*GLRender::pfuncSetPixelShaderAmbientColor)(reinterpret_cast<Vector&>(local), m_fOpacity);
    }
    VertexProgram* pVP;
    if (m_pMaterial->HasBumpMap() &&
        trinode->HasTangentBases() &&
        AurNV2xLevelShadersAvailable())
        pVP = g_pSkinBumpedOutWithTangentVP;
    else
        pVP = g_pSkinBumpedOutVP;
    pVP->Enable(part, false);
#if KOTOR1  // K1: no force-sight pass, client-state teardown only, blend/depth left alone
    RenderFlat(pv);
    ASLgl::glDisableClientState(clientStateTranslationTable[4]);
    ASLgl::glDisableClientState(clientStateTranslationTable[6]);
    SetBumpedOutShellIos(0.0f);
    ASLgl::glDisable(enableTranslationTable[9]);
    }
}
#else
    if (g_ForceSight != 0)
        SetupForceSightShader(m_pGob, GLRender::kPSFGlow, GLRender::kPSDesat, 0, 0, 0, 0, false);
    RenderFlat(pv);

    if (g_ForceSight != 0) {
        (*GLRender::pfuncEnablePixelShaders)(GLRender::kPSDesatOff, 0, 0, 0, 0);
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
        if (g_useDistortionOutput != 0)
            goto ShellCommon;
        ASLgl::glDepthMask(1);
        if (usearbvertexprograms != 0)
            goto ShellAttribs;
        goto ShellClientStates;
    }
ShellCommon:
    if (usearbvertexprograms == 0)
        goto ShellClientStates;
ShellAttribs:
    ASLgl::glDisableVertexAttribArrayARB(1);
    ASLgl::glDisableVertexAttribArrayARB(4);
    goto ShellAfter;
ShellClientStates:
    ASLgl::glDisableClientState(clientStateTranslationTable[4]);
    ASLgl::glDisableClientState(clientStateTranslationTable[6]);
ShellAfter:
    ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
    if (g_useDistortionOutput == 0)
        ASLgl::glDepthMask(1);
    SetBumpedOutShellIos(0.0f);
    ASLgl::glDisable(enableTranslationTable[9]);
    }
}
#endif

// Temporary axis-aligned-bounds/skin-segment element resolved by PostProcess.
struct SkinSegment {
    Part*  m_paParts[4];
    float  m_faWeights[4];
    int    m_naBoneIdx[4];
    int    m_naPad[2];
};

// The per-vertex source array a tri-mesh node points at.
struct VertexStrategy {
    char         m_pad80[128];
    void**       m_ppVertices;
    unsigned int m_nVertexCount;
};

// NodeVertex::m_nTexCoordFlags (the strategy vertex's byte at +84): which texture-coordinate sets
// the vertex carries.
enum {
    kNodeVertexTex0 = 0x01,
    kNodeVertexTex1 = 0x02,
    kNodeVertexTex2 = 0x04,
    kNodeVertexTex3 = 0x08,
};

// Float bit patterns PostProcess writes into the vertex pool: the guard vertex past the last one
// sits at 1e6f (the pair covers x and y), a bone slot starts at -1.0f (no bone), and the guard
// row's weights are (1.0f, 0.0f).
enum : unsigned int
{
    kGuardCoordBits = 0x49742400,
    kNoBoneBits = 0xbf800000,
};
enum : unsigned long long
{
    kGuardCoordPairBits = 0x4974240049742400ULL,
    kWeightOneZeroBits = 0x3f800000ULL,
};

// Builds the vertex and skin data for a skinned tri-mesh: bone frames, geometry and skin weights.
void PartSkin::PostProcess(Part* part)
{
    K2_UNUSED(part);
    List<Part*>     listParts;       // MakeInlinePartList output (the root cone)
    MdlNodeSkin* node = static_cast<MdlNodeSkin*>(m_pNode);

    if (node->m_nVertexCount == 0) {
        bool bTangent = false;       // HasBumpMap probe: the tail calls CalculateTangentSpaceBasis
        bool bDone = false;

        if (node->m_lTBoneRefInv.m_count == 0) {
            List<Quaternion> quatFrames; // GatherBones output: world-frame orientations
            List<Vector>     vecFrames;  // GatherBones output: world-frame positions
#if KOTOR1
            // (1) The same world-to-local frame walk.
            Quaternion qInv = -m_qLocalOrientation;
            Vector vPos = RotateVector(-m_vLocalPosition, qInv);
            Quaternion qAcc = -m_qLocalOrientation;
            Part* pTop = this;
            for (Part* pParent = static_cast<Part*>(m_pParent); pParent != 0; pParent = static_cast<Part*>(pParent->m_pParent)) {
                pTop = pParent;
                Quaternion qParentInv = -pParent->m_qLocalOrientation;
                Vector p = RotateVector(-pParent->m_vLocalPosition, qParentInv);
                vPos = vPos + RotateVector(p, qAcc);
                qAcc = qAcc * -pParent->m_qLocalOrientation;
            }
            GatherBones(pTop, vecFrames, quatFrames, vPos, qAcc);

            for (int i = 0; i < vecFrames.m_count; i++) {
                Quaternion qFrameInv = -quatFrames.m_pData[i];
                node->m_lTBoneRefInv.Add(RotateVector(-vecFrames.m_pData[i], qFrameInv));
                node->m_lQBoneRefInv.Add(-quatFrames.m_pData[i]);
            }
#else
            float fPosX, fPosY, fPosZ;    // accumulated negated world position
            float fW, fX, fY, fZ;         // accumulated quaternion (w negated, world-to-local dialect)

            // (1a) This part's own frame.
            {
                float qw = m_qLocalOrientation.w;
                float qx = m_qLocalOrientation.x;
                float qy = m_qLocalOrientation.y;
                float qz = m_qLocalOrientation.z;
                float nw = -qw;
                fW = nw; fX = qx; fY = qy; fZ = qz;
                fPosX = -m_vLocalPosition.x;
                fPosY = -m_vLocalPosition.y;
                fPosZ = -m_vLocalPosition.z;
                if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f)) {
                    float t1 = qy*qy + qz*qz;
                    float t2 = qx*qx + qz*qz;
                    float t3 = qx*qy - qz*nw;
                    float t4 = qx*qy + qz*nw;
                    float t5 = qy*nw + qx*qz;
                    float t6 = qx*qz - qy*nw;
                    float t7 = qy*qz - qx*nw;
                    float t8 = qx*nw + qy*qz;
                    float t9 = qx*qx + qy*qy;
                    float u1 = (t4 + t4) * fPosX;
                    float u2 = (t6 + t6) * fPosX;
                    float u3 = (t8 + t8) * fPosY;
                    fPosX = (1.0f - (t1 + t1)) * fPosX + (t3 + t3) * fPosY + (t5 + t5) * fPosZ;
                    fPosY = u1 + (1.0f - (t2 + t2)) * fPosY + (t7 + t7) * fPosZ;
                    fPosZ = u2 + u3 + (1.0f - (t9 + t9)) * fPosZ;
                }
            }
            // (1b) The parent chain: rotate each parent's negated offset by its own quaternion,
            // then by the accumulated one, then compose that quaternion into the accumulator.
            Part* pTop = this;
            Part* pParent = static_cast<Part*>(m_pParent);
            while (pParent != 0) {
                pTop = pParent;
                float qw = pParent->m_qLocalOrientation.w;
                float qx = pParent->m_qLocalOrientation.x;
                float qy = pParent->m_qLocalOrientation.y;
                float qz = pParent->m_qLocalOrientation.z;
                float nw = -qw;
                float px = -pParent->m_vLocalPosition.x;
                float py = -pParent->m_vLocalPosition.y;
                float pz = -pParent->m_vLocalPosition.z;
                if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f)) {
                    float t1 = qy*qy + qz*qz;
                    float t2 = qx*qx + qz*qz;
                    float t3 = qx*qy - qz*nw;
                    float t4 = qx*qy + qz*nw;
                    float t5 = qy*nw + qx*qz;
                    float t6 = qx*qz - qy*nw;
                    float t7 = qy*qz - qx*nw;
                    float t8 = qx*nw + qy*qz;
                    float t9 = qx*qx + qy*qy;
                    float u1 = (t4 + t4) * px;
                    float u2 = (t6 + t6) * px;
                    float u3 = (t8 + t8) * py;
                    px = (1.0f - (t1 + t1)) * px + (t3 + t3) * py + (t5 + t5) * pz;
                    py = u1 + (1.0f - (t2 + t2)) * py + (t7 + t7) * pz;
                    pz = u2 + u3 + (1.0f - (t9 + t9)) * pz;
                }
                if (quaternionidentityoptimization == 0 || (fW != -1.0f && fW != 1.0f)) {
                    float t1 = fY*fY + fZ*fZ;
                    float t2 = fX*fX + fZ*fZ;
                    float t3 = fX*fY - fZ*fW;
                    float t4 = fX*fY + fZ*fW;
                    float t5 = fY*fW + fX*fZ;
                    float t6 = fX*fZ - fY*fW;
                    float t7 = fY*fZ - fX*fW;
                    float t8 = fX*fW + fY*fZ;
                    float t9 = fX*fX + fY*fY;
                    float u1 = (t4 + t4) * px;
                    float u2 = (t6 + t6) * px;
                    float u3 = (t8 + t8) * py;
                    px = (1.0f - (t1 + t1)) * px + (t3 + t3) * py + (t5 + t5) * pz;
                    py = u1 + (1.0f - (t2 + t2)) * py + (t7 + t7) * pz;
                    pz = u2 + u3 + (1.0f - (t9 + t9)) * pz;
                }
                fPosX += px;
                fPosY += py;
                fPosZ += pz;
                // Hamilton compose of the accumulator with the conjugated parent quaternion.
                float a1 = fW * qx;
                float a2 = fW * qy;
                float a3 = fX * qz;
                float a4 = fW * qz;
                float a5 = fX * qy;
                float a6 = fY * qx;
                float nW = (fW * nw - fX * qx) - fY * qy - fZ * qz;
                float nX = (fX * nw + a1 + fY * qz) - fZ * qy;
                float nY = fZ * qx + (fY * nw + (a2 - a3));
                float nZ = fZ * nw + ((a5 + a4) - a6);
                fW = nW; fX = nX; fY = nY; fZ = nZ;
                pParent = static_cast<Part*>(pParent->m_pParent);
            }
            GatherBones(pTop, vecFrames, quatFrames,
                        Vector(fPosX, fPosY, fPosZ), Quaternion(fW, fX, fY, fZ));

            // (1c) Copy the gathered world frames into the node's bone stores, inverse-rotating
            // each frame by its own orientation.
            if (vecFrames.m_count >= 1) {
                for (int i = 0; i < vecFrames.m_count; i++) {
                    List<Vector>&     rBonePos = *&node->m_lTBoneRefInv;
                    List<Quaternion>& rBoneQuat = *&node->m_lQBoneRefInv;
                    float vx = vecFrames.m_pData[i].x;
                    float vy = vecFrames.m_pData[i].y;
                    float vz = vecFrames.m_pData[i].z;
                    float qw = quatFrames.m_pData[i].w;
                    float qx = quatFrames.m_pData[i].x;
                    float qy = quatFrames.m_pData[i].y;
                    float qz = quatFrames.m_pData[i].z;
                    float nx = -vx, ny = -vy, nz = -vz;
                    if (quaternionidentityoptimization == 0 || (qw != -1.0f && qw != 1.0f)) {
                        float nwn = -qw;
                        float t1 = qy*qy + qz*qz;
                        float t2 = qx*qx + qz*qz;
                        float t9 = qx*qx + qy*qy;
                        float t3 = qx*qy - qz*nwn;
                        float t5 = qy*nwn + qx*qz;
                        float t4 = qx*qy + qz*nwn;
                        float t7 = qy*qz - qx*nwn;
                        float t6 = qx*qz - qy*nwn;
                        float t8 = qx*nwn + qy*qz;
                        float u1 = (t4 + t4) * nx;
                        float u2 = (t6 + t6) * nx;
                        float u3 = (t8 + t8) * ny;
                        nx = (1.0f - (t1 + t1)) * nx + (t3 + t3) * ny + (t5 + t5) * nz;
                        ny = u1 + (1.0f - (t2 + t2)) * ny + (t7 + t7) * nz;
                        nz = u2 + u3 + (1.0f - (t9 + t9)) * nz;
                    }
                    rBonePos.Add(Vector(nx, ny, nz));
                    rBoneQuat.Add(Quaternion(-quatFrames.m_pData[i].w,
                                             quatFrames.m_pData[i].x,
                                             quatFrames.m_pData[i].y,
                                             quatFrames.m_pData[i].z));
                }
            }
#endif

            // (2) The bounds/skin-segment table: resolve unset bone indices and free the part slots.
            if (node->m_lWeights.m_count >= 1) {
                for (int i = 0; i < node->m_lWeights.m_count; i++) {
                    SkinSegment* pS = &reinterpret_cast<SkinSegment*>(node->m_lWeights.m_pData)[i];
                    if (pS->m_naBoneIdx[0] == -1) {
                        pS->m_naBoneIdx[0] = FindPartIndexDFS(pTop, reinterpret_cast<char*>(pS->m_paParts[0]));
                        pS->m_naBoneIdx[1] = FindPartIndexDFS(pTop, reinterpret_cast<char*>(pS->m_paParts[1]));
                        pS->m_naBoneIdx[2] = FindPartIndexDFS(pTop, reinterpret_cast<char*>(pS->m_paParts[2]));
                        pS->m_naBoneIdx[3] = FindPartIndexDFS(pTop, reinterpret_cast<char*>(pS->m_paParts[3]));
                    }
                    if (pS->m_paParts[0]) operator delete[](pS->m_paParts[0]);
                    if (pS->m_paParts[1]) operator delete[](pS->m_paParts[1]);
                    if (pS->m_paParts[2]) operator delete[](pS->m_paParts[2]);
                    if (pS->m_paParts[3]) operator delete[](pS->m_paParts[3]);
                    pS->m_paParts[0] = 0;
                    pS->m_paParts[1] = 0;
                    pS->m_paParts[2] = 0;
                    pS->m_paParts[3] = 0;
                }
            }

            // (3) Node geometry prep.
            node->m_pfnGenVertices(node);
            VertexStrategy* pStrat = static_cast<VertexStrategy*>(node->m_pPostProcess);
            node->m_nVertexCount = static_cast<unsigned short>(pStrat->m_nVertexCount);
            node->BuildVertexIndices();
            node->BuildNeighborLists();

            // (4) Build the piece-vertex element-offset table from the strategy vertex's flag
            // byte; with no vertices, every element offset stays off.
            int  nCount = static_cast<short>(node->m_nVertexCount);
            unsigned char* pVert = (nCount >= 1) ? static_cast<unsigned char*>(pStrat->m_ppVertices[0]) : 0;
            unsigned int uFlags = node->m_uVertexFlags | MDX_VERTICES | MDX_VERTEX_NORMALS;
            int iStride = 24;
            node->m_nNormalOfs = 12;   // normal offset
            node->m_nVertexOfs = 0;     // position offset
            node->m_uVertexFlags = uFlags;
            node->m_nVertexStride = 24;
#ifdef K2_FIX_ENGINE_BUGS
            unsigned char ef = pVert != 0 ? pVert[84] : 0;
#else
            unsigned char ef = pVert[84];
#endif
            if (ef & kNodeVertexTex0) {
                node->m_nTex0Ofs = iStride;
                uFlags |= MDX_TEX0_VERTICES;
                node->m_uVertexFlags = uFlags;
                node->m_nTexCoordSets++;
                iStride = 32;
                node->m_nVertexStride = iStride;
            }
            if (ef & kNodeVertexTex1) {
                node->m_nTex1Ofs = iStride;
                uFlags |= MDX_TEX1_VERTICES;
                node->m_uVertexFlags = uFlags;
                node->m_nTexCoordSets++;
                iStride += 8;
                node->m_nVertexStride = iStride;
            }
            if (ef & kNodeVertexTex2) {
                node->m_nTex2Ofs = iStride;
                uFlags |= MDX_TEX2_VERTICES;
                node->m_uVertexFlags = uFlags;
                node->m_nTexCoordSets++;
                iStride += 8;
                node->m_nVertexStride = iStride;
            }
            if (ef & kNodeVertexTex3) {
                node->m_nTex3Ofs = iStride;
                uFlags |= MDX_TEX3_VERTICES;
                node->m_uVertexFlags = uFlags;
                node->m_nTexCoordSets++;
                iStride += 8;
                node->m_nVertexStride = iStride;
            }
            // (4b) The transient bump-material probe: a bump map adds the bump element to the
            // stride and latches bTangent for the tail.
            Material* pMat = new Material;
            pMat->InitializeTextures(node->m_szTexture, 0, 0, 0);
            bTangent = pMat->HasBumpMap() != 0;
            if (bTangent) {
                node->m_nVertexStride += 36;
                node->m_uVertexFlags |= MDX_TANGENT_SPACE;
                node->m_nTangentOfs = iStride;
                iStride += 36;
            }
            delete pMat;
            node->m_nBoneWeightsOfs = iStride;
            node->m_nVertexStride += 32;
            node->m_nBoneIndicesOfs = iStride + 16;

            // (5) Grow and lock the owning model's pool, then copy every vertex's elements.
            // Do not cache the owning model: re-derive it at each use.
            int nBase = node->m_pTree->AsModel()->m_nPoolSize;
            int nPoolA = node->m_pTree->AsModel()->m_nPool;
            int nAlign = nBase & 0xf;
            GLRender::ResizePool(nPoolA, node->m_pTree->AsModel()->m_nPoolSize + nAlign +
                                             (nCount + 1) * node->m_nVertexStride);
            char* pLock = static_cast<char*>(GLRender::LockPool(node->m_pTree->AsModel()->m_nPool, 0, 0, 1));
            node->m_nGeomBase = node->m_pTree->AsModel()->m_nPoolSize + nAlign;
            node->m_pTree->AsModel()->m_nPoolSize += nAlign + (nCount + 1) * node->m_nVertexStride;
            node->m_paVertices = new float[3 * (nCount + 1)];

            int nStride = node->m_nVertexStride;
            unsigned int nGeom = node->m_nGeomBase;
            if (nCount >= 1) {
                for (int i = 0; i < nCount; i++) {
                    const float* v = static_cast<const float*>(pStrat->m_ppVertices[i]);
                    if (node->m_nTex0Ofs != -1) {
                        *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + node->m_nTex0Ofs]) = v[3];
                        *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + 4 + node->m_nTex0Ofs]) = v[4];
                    }
                    if (node->m_nTex1Ofs != -1) {
                        *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + node->m_nTex1Ofs]) = v[6];
                        *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + 4 + node->m_nTex1Ofs]) = v[7];
                    }
                    if (node->m_nTex2Ofs != -1) {
                        *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + node->m_nTex2Ofs]) = v[9];
                        *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + 4 + node->m_nTex2Ofs]) = v[10];
                    }
                    if (node->m_nTex3Ofs != -1) {
                        *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + node->m_nTex3Ofs]) = v[12];
                        *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + 4 + node->m_nTex3Ofs]) = v[13];
                    }
                    *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + node->m_nVertexOfs]) = v[0];
                    *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + 4 + node->m_nVertexOfs]) = v[1];
                    *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + 8 + node->m_nVertexOfs]) = v[2];
                    node->m_paVertices[3*i + 0] = v[0];
                    node->m_paVertices[3*i + 1] = v[1];
                    node->m_paVertices[3*i + 2] = v[2];
                    *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + node->m_nNormalOfs]) = v[15];
                    *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + 4 + node->m_nNormalOfs]) = v[16];
                    *reinterpret_cast<float*>(&pLock[nGeom + i * nStride + 8 + node->m_nNormalOfs]) = v[17];
                }
            }
            char* pGuard = pLock + nGeom + static_cast<short>(nCount) * nStride + node->m_nVertexOfs;
            memset(pGuard, 0, nStride);  // idiom-exempt: runtime stride (per-mesh vertex stride)
            *reinterpret_cast<unsigned long long*>(pGuard) = kGuardCoordPairBits;
            *(reinterpret_cast<unsigned int*>(pGuard) + 2) = kGuardCoordBits;  // idiom-exempt: GL vertex pool bytes

            // (6) Root walk and short-lived part list, then the bone/skin lists sized to it.
            Part* pRoot = this;
            while (pRoot->m_pParent)
                pRoot = static_cast<Part*>(pRoot->m_pParent);
            MakeInlinePartList(pRoot, listParts);
            int nBones = listParts.m_count;
            List<int>& rSkinSeg = node->m_lBoneConstantIndices;
            rSkinSeg.SetSize(nBones);
            node->m_nBoneCount = nBones;
            node->m_paBoneMap = new float[nBones];
            for (int i = 0; i < nBones; i++)
                node->m_paBoneMap[i] = -1.0f;

            // (7) Skin-segment fill: pair each weight with its remapped bone id in the locked
            // rows, remapping through the bone-index table.
            int nBoneRemap = 0;
            int64_t nRow;
            if (nCount > 0) {
                int i = 0;
                SkinSegment* pSegs = reinterpret_cast<SkinSegment*>(node->m_lWeights.m_pData);
                do {
                    SkinSegment& seg = pSegs[i];
                    char* pW = pLock + nGeom + static_cast<short>(i) * nStride + node->m_nBoneWeightsOfs;
                    char* pB = pLock + nGeom + static_cast<short>(i) * nStride + node->m_nBoneIndicesOfs;
                    for (int s = 0; s < 4; s++) {
                        *reinterpret_cast<unsigned int*>(pB + 4*s) = kNoBoneBits;
                        *reinterpret_cast<unsigned int*>(pW + 4*s) = kNoBoneBits;
                        float w = seg.m_faWeights[s];
                        *reinterpret_cast<float*>(pW + 4*s) = w;
                        if (w != 0.0f) {
                            int bone = seg.m_naBoneIdx[s];
                            float remap = node->m_paBoneMap[bone];
                            if (remap < 0.0f) {
                                remap = static_cast<float>(nBoneRemap++);
                                node->m_paBoneMap[bone] = remap;
                            }
                            *reinterpret_cast<float*>(pB + 4*s) = remap;
                            node->m_aBoneNodeIndices[static_cast<int>(remap)] = static_cast<short>(bone);
                        }
                    }
                    i++;
                } while (i < nCount);
                nRow = i;
            } else {
                nRow = 1;
            }
            // Guard row of zero weights plus a leading 1.0f sentinel weight.
            {
                char* pTail = pLock + nGeom + static_cast<short>(nRow) * nStride;
                *reinterpret_cast<unsigned long long*>(pTail + node->m_nBoneIndicesOfs) = 0;
                *reinterpret_cast<unsigned long long*>(pTail + node->m_nBoneIndicesOfs + 8) = 0;
                *reinterpret_cast<unsigned long long*>(pTail + node->m_nBoneWeightsOfs) = kWeightOneZeroBits;
                *reinterpret_cast<unsigned long long*>(pTail + node->m_nBoneWeightsOfs + 8) = 0;
            }

            // (8) Tear down the transient segment buffer and unlock the pool.
            if (node->m_lWeights.m_pData) {
                operator delete[](node->m_lWeights.m_pData);
                node->m_lWeights.m_pData = 0;
            }
            node->m_lWeights.m_count = 0;
            node->m_lWeights.m_capacity = 0;
            GLRender::UnlockPool(node->m_pTree->AsModel()->m_nPool);
            bDone = true;
        }
        if (bDone) {
            if (bTangent)
                node->CalculateTangentSpaceBasis();
            node->m_pfnRemoveTemporaryArrays(node);
        }
    } else {
        node->m_pfnRemoveTemporaryArrays(node);
    }
}

// Builds the skin node's vertices: gathers each face corner into a vertex, dedups against the
// scratchpad's vertex array and writes the resulting index back into the face.
void MdlNodeSkin::InternalGenVertices()
{
#if !KOTOR1  // K1 has no early-out: the empty list is assigned below
    if (m_faces.m_count < 1) {
        m_lWeights.m_count = 0;
        return;
    }
#endif

    List<SkinVertexWeight> lWeights;

    for (int64_t i = 0; i < static_cast<int64_t>(m_faces.m_count); i++) {
        for (int64_t j = 0; j < 3; j++) {
            int nVert = PostProcess()->m_faces.m_pData[i].m_paVertIndices[j];
            Vector* pTex0 = PostProcess()->m_tverts.m_count
                ? &PostProcess()->m_tverts.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j]] : 0;
            Vector* pTex1 = PostProcess()->m_tverts1.m_count
                ? &PostProcess()->m_tverts1.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 3]] : 0;
            Vector* pTex2 = PostProcess()->m_tverts2.m_count
                ? &PostProcess()->m_tverts2.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 6]] : 0;
            Vector* pTex3 = PostProcess()->m_tverts3.m_count
                ? &PostProcess()->m_tverts3.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 9]] : 0;
            Vector* pNormal =
                &PostProcess()->m_normals.m_pData[PostProcess()->m_faces.m_pData[i].m_paNormalIndices[j]];
            NodeVertex* pVertex = new NodeVertex(&PostProcess()->m_verts.m_pData[nVert],
                                               pTex0, pTex1, pTex2, pTex3, pNormal, 0);
            int64_t index;
            int bFound = 0;
            for (index = 0; index < static_cast<int64_t>(PostProcess()->m_vertices.m_count); index++) {
                if (*reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[index]) == *pVertex) {
                    bFound = 1;
                    break;
                }
            }
            if (bFound) {
                delete pVertex;
                m_faces.m_pData[i].m_anVertIndices[j] = static_cast<unsigned short>(index);
            } else {
                m_faces.m_pData[i].m_anVertIndices[j] =
                    static_cast<unsigned short>(PostProcess()->m_vertices.m_count);
                PostProcess()->m_vertices.Add(reinterpret_cast<unsigned int*>(pVertex));
#if KOTOR1  // K1 re-reads the corner's old vertex index
                lWeights.Add(m_lWeights.m_pData[PostProcess()->m_faces.m_pData[i].m_paVertIndices[j]]);
#else
                lWeights.Add(m_lWeights.m_pData[nVert]);
#endif
            }
        }
    }

#if KOTOR1  // K1 assigns the re-indexed list in one operator=
    m_lWeights = lWeights;
#else
    m_lWeights.m_count = 0;
    for (int i = 0; i < lWeights.m_count; i++)
        m_lWeights.Add(lWeights.m_pData[i]);
#endif
}
