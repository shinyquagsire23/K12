// Aurora grass layer: the grass material registry, the per-bin triangle geometry builder, and the
// legacy CAurTriangleList stub.

#include "k2stdint.h"
#include "port/aslglapi.h"
#include "aurres.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurgrass.h"
#include "List.h"
#include "Vector.h"
#include "Quaternion.h"

float CAurTriangleBin::m_fNearRadiusSquared = 225.0f;
float CAurTriangleBin::m_fFarRadiusSquared = 900.0f;

#if KOTOR1
List<CAurGrassInfo> g_GrassInfo;  // K1: strong definition
#else
__attribute__((weak)) List<CAurGrassInfo> g_GrassInfo;
#endif

#include "port/libcbase.h"


class CAurTexture;
void AurTextureRelease(CAurTexture* pTexture);

bool AurGrassMaterialInUse(int nMaterial)
{
    if (g_GrassInfo.m_count <= nMaterial)
        return false;
    return g_GrassInfo.m_pData[nMaterial].m_bInUse != 0;
}

void AurGrassClearInfo()
{
    g_GrassInfo.m_count = 0;
}

void AurGrassAddInfo(int nMaterial, const CAurGrassInfo& info)
{
    while (g_GrassInfo.m_count <= nMaterial)
        g_GrassInfo.Add(CAurGrassInfo());

    CAurGrassInfo* pInfo = &g_GrassInfo.m_pData[nMaterial];

#if KOTOR1
    // K1: the two colour triples are Vector copies, ambient first.
    pInfo->m_fDensity = info.m_fDensity;
    pInfo->m_fQuadSize = info.m_fQuadSize;
    pInfo->m_vAmbient = info.m_vAmbient;
    pInfo->m_vDiffuse = info.m_vDiffuse;
#else
    pInfo->m_fDensity = info.m_fDensity;
    pInfo->m_fQuadSize = info.m_fQuadSize;
    pInfo->m_vDiffuse.x = info.m_vDiffuse.x;
    pInfo->m_vDiffuse.y = info.m_vDiffuse.y;
    pInfo->m_vDiffuse.z = info.m_vDiffuse.z;
    pInfo->m_vAmbient.x = info.m_vAmbient.x;
    pInfo->m_vAmbient.y = info.m_vAmbient.y;
    pInfo->m_vAmbient.z = info.m_vAmbient.z;
    pInfo->m_vEmissive.x = info.m_vEmissive.x;
    pInfo->m_vEmissive.y = info.m_vEmissive.y;
    pInfo->m_vEmissive.z = info.m_vEmissive.z;
#endif

    pInfo->m_fProbLL = info.m_fProbLL;
    pInfo->m_fProbLR = info.m_fProbLR;
    pInfo->m_fProbUL = info.m_fProbUL;
    pInfo->m_fProbUR = info.m_fProbUR;

    float fSum = 0.0f;
    fSum += pInfo->m_fProbLL;
    fSum += pInfo->m_fProbLR;
    fSum += pInfo->m_fProbUL;
    fSum += pInfo->m_fProbUR;
    if (fSum != 1.0f) {
        pInfo->m_fProbLL /= fSum;
        pInfo->m_fProbLR /= fSum;
        pInfo->m_fProbUL /= fSum;
        pInfo->m_fProbUR /= fSum;
    }

#if KOTOR1
    __strcpy_chk(g_GrassInfo.m_pData[nMaterial].m_sName, info.m_sName, sizeof(pInfo->m_sName));
    g_GrassInfo.m_pData[nMaterial].m_bInUse = 1;
#else
    __strcpy_chk(pInfo->m_sName, info.m_sName, sizeof(pInfo->m_sName));
    pInfo->m_bInUse = 1;
#endif
}

CAurGrassInfo::CAurGrassInfo()
{
    m_sName[0] = 0;
    m_bInUse = 0;
#if KOTOR1
    __builtin_memset(&m_fDensity, 0, 8 * sizeof(float));
#else
    __builtin_memset(&m_fDensity, 0, 11 * sizeof(float));
#endif
    m_fProbLL = m_fProbLR = m_fProbUL = m_fProbUR = 0.25f;
}

void AurGrassReadData(char* pszName)
{
    int nSize = 0;
    g_GrassInfo.SetSize(0);

    OpenResource* pRes = AurResGet(pszName, const_cast<char*>(".txi"), &nSize, true);
    if (!pRes)
        return;

    if ((nSize < 0 ? ~nSize : nSize) > 0) {
        int nMaterial = 0;
        char sName[32];
        float fDensity = 0.0f;
        float fQuadSize = 0.0f;
#if KOTOR1
        Vector vDiffuse;
        Vector vAmbient;
#else
        Vector vAmbient;
        Vector vDiffuse;
#endif

        char* pszLine = AurResGetNextLine();
        while (pszLine) {
            while (*pszLine == ' ' || *pszLine == '\t')
                pszLine++;

            static_cast<void>(sscanf(pszLine, "%d %s %f %f %f%f%f %f%f%f", &nMaterial, sName,
                   &fDensity, &fQuadSize, &vDiffuse.x, &vDiffuse.y, &vDiffuse.z,
                   &vAmbient.x, &vAmbient.y, &vAmbient.z));

            while (nMaterial >= g_GrassInfo.m_count)
                g_GrassInfo.Add(CAurGrassInfo());

#if KOTOR1
            g_GrassInfo.m_pData[nMaterial].m_fDensity = fDensity;
            g_GrassInfo.m_pData[nMaterial].m_fQuadSize = fQuadSize;
            g_GrassInfo.m_pData[nMaterial].m_vAmbient = vAmbient;
            g_GrassInfo.m_pData[nMaterial].m_vDiffuse = vDiffuse;
            g_GrassInfo.m_pData[nMaterial].m_bInUse = 1;
            __strcpy_chk(g_GrassInfo.m_pData[nMaterial].m_sName, sName,
                         sizeof(g_GrassInfo.m_pData[nMaterial].m_sName));
#else
            CAurGrassInfo* pInfo = &g_GrassInfo.m_pData[nMaterial];
            pInfo->m_fDensity = fDensity;
            pInfo->m_fQuadSize = fQuadSize;
            __builtin_memcpy(&pInfo->m_vDiffuse, &vDiffuse, sizeof(vDiffuse));
            __builtin_memcpy(&pInfo->m_vAmbient, &vAmbient, sizeof(vAmbient));
            pInfo->m_bInUse = 1;
            __strcpy_chk(pInfo->m_sName, sName, sizeof(pInfo->m_sName));
#endif

            pszLine = AurResGetNextLine();
        }
    }

    AurResFree(pRes, nSize);
}

#include "aurtexman.h"
#include "aurmaterial.h"
#include "aurglcore.h"
#include "AURORA/aurmodel.h"
#include "aurvertexprog.h"
#include "aurwindmanager.h"
#include "aurscene.h"
#include <math.h>

CAurTriangleBin::CAurTriangleBin()
{
    m_nFlags = 0;
    __builtin_memset(&m_vMin, 0, sizeof(m_vMin) * 3 + 12 + sizeof(int));
    __builtin_memset(&m_nX48, 0, sizeof(m_nX48) + sizeof(m_nVertexBytes));
    __builtin_memset(static_cast<void*>(&m_elements), 0, sizeof(*this) - offsetof(CAurTriangleBin, m_elements));
    m_nMaterial = -1;
    m_nTotalTriangles = 0;
}

CAurTriangleBin::CAurTriangleBin(const Vector& vMin, const Vector& vMax)
{
    m_nFlags = 0;
    m_nTotalTriangles = 0;
    __builtin_memset(&m_vMin, 0, sizeof(Vector) * 3 - sizeof(float));
    __builtin_memset(&m_vCenter.z, 0, sizeof(float));
    __builtin_memset(&m_pGrassVerts, 0, sizeof(void*) * 2);
    m_nX48 = 0;
    m_nVertexBytes = 0;
    __builtin_memset(static_cast<void*>(&m_elements), 0, sizeof(*this) - offsetof(CAurTriangleBin, m_elements));
    m_pTexture = 0;
    m_vMin = vMin;
    m_vMax = vMax;
    m_nMaterial = -1;
}

void CAurTriangleBin::FinishGeometry()
{
    m_vCenter = (m_vMin + m_vMax) * 0.5f;
    m_fRadius = magnitude(m_vMin - m_vMax) * 0.5f;
}

void CAurTriangleBin::SetMinDistanceSquared(float d)
{
    m_minDistanceSquared = d;
}

void CAurTriangleBin::BuildGrassPolys()
{
    if (m_nFlags & kGrassPolysBuilt)
        return;
    if (m_nTotalTriangles == 0)
        return;

    if (m_nFlags & kGrassLightmapped) {
        m_nVertexStride = 40;
        CreateArrays();
        return;
    }

    m_nVertexStride = (m_nFlags & kGrassVertexColors) ? 36 : 32;
    CreateArrays();
}

void CAurTriangleBin::DestroyGrassPolys()
{
    if (!(m_nFlags & kGrassPolysBuilt))
        return;

    if (m_nFlags & kGrassArrays) {
        delete[] m_pGrassCursor;
        m_pGrassCursor = 0;
#if KOTOR1
        m_nX48 = 0;           // K1: also clears the attribute pair
        m_nVertexBytes = 0;
#endif
        delete[] m_pGrassVerts;
        m_pGrassVerts = 0;
        m_nFlags ^= kGrassArrays;
    }

    if (m_nFlags & kGrassLightmapped) {
        m_texCounts.SetSize(0);
        m_textureRefs.SetSize(0);
    }

    m_nFlags ^= kGrassPolysBuilt;
}



int GrassSortTris(const void* pA, const void* pB);

void CAurTriangleBin::SortTriangles()
{
    if (m_elements.m_count == 1) {
        m_texCounts.Add(m_elements.m_pData[0].m_nGrassCount);
#if KOTOR1
        m_textureRefs.Add(SafePointer<CAurTexture>(m_elements.m_pData[0].m_pPartMesh->m_pMaterial->m_spr1));
#else
        m_textureRefs.Add(SafePointer<CAurTexture>(m_elements.m_pData[0].m_pPartMesh->m_pMaterial->m_spr1.m_pObject));
#endif
        return;
    }

    List<CAurTriangleListElem> sortedElems(m_elements);
    qsort(sortedElems.m_pData, sortedElems.m_count, sizeof(CAurTriangleListElem), GrassSortTris);

    int64_t i = 0;
    int nCount = 0;
    while (i + 1 < sortedElems.m_count) {
        if (sortedElems.m_pData[i].m_pPartMesh != sortedElems.m_pData[i + 1].m_pPartMesh) {
#if KOTOR1
            m_textureRefs.Add(SafePointer<CAurTexture>(m_elements.m_pData[i].m_pPartMesh->m_pMaterial->m_spr1));
#else
            m_textureRefs.Add(SafePointer<CAurTexture>(m_elements.m_pData[i].m_pPartMesh->m_pMaterial->m_spr1.m_pObject));
#endif
            m_texCounts.Add(nCount + sortedElems.m_pData[i].m_nGrassCount);
            nCount = 0;
        } else {
            nCount += sortedElems.m_pData[i].m_nGrassCount;
        }
        i++;
    }

    if (sortedElems.m_pData[i].m_pPartMesh != sortedElems.m_pData[i - 1].m_pPartMesh)
        nCount = 0;
#if KOTOR1
    m_textureRefs.Add(SafePointer<CAurTexture>(m_elements.m_pData[i].m_pPartMesh->m_pMaterial->m_spr1));
#else
    m_textureRefs.Add(SafePointer<CAurTexture>(m_elements.m_pData[i].m_pPartMesh->m_pMaterial->m_spr1.m_pObject));
#endif
    m_texCounts.Add(nCount + sortedElems.m_pData[i].m_nGrassCount);
}

int GrassSortTris(const void* pA, const void* pB)
{
    CAurTriangleListElem* elemA = static_cast<CAurTriangleListElem*>(const_cast<void*>(pA));
    CAurTriangleListElem* elemB = static_cast<CAurTriangleListElem*>(const_cast<void*>(pB));

    List<float> vA(elemA->m_vertexList);
    PartTriMesh* pMeshA = elemA->m_pPartMesh;
    List<float> vB(elemB->m_vertexList);
    PartTriMesh* pMeshB = elemB->m_pPartMesh;

    if (pMeshA < pMeshB) return -1;
    if (pMeshA > pMeshB) return 1;
    return 0;
}



bool AurNV1xLevelShadersAvailable();
int  AurMultiTextureAvailable();
bool AurNonRadeon8500Validate();


int maxgrass;
int newgrass = 1;
int enablegrasswind = 1;
float grassstiffness = 1.0f;
extern float DeltaT;
int rendertexturedgrass = 1;
extern int usearbvertexprograms;
extern unsigned char g_ForceSight;
extern int countfaces;

extern char cStandardVP[];
extern char szStandardVPARB[];
extern unsigned int* cStandardVPDeclaration;

#if KOTOR1
// K1: the grass row's own colour triples feed ambient/diffuse (no emission), and one
// DrawLightmappedGrass covers the whole bin.
float* Homogenize(const Vector& v, const float& w);
extern bool s_useAlphaTestShader;
void CAurTriangleBin::RenderGrassPolys(int nMaterial, const Vector& vCamera, WindManager* pWind)
{
    K2_UNUSED(vCamera);
    if (!AurNV1xLevelShadersAvailable() && !AurMultiTextureAvailable())
        return;
    if (!(m_nFlags & kGrassPolysBuilt))
        return;
    if (m_nTotalTriangles == 0)
        return;
    if (m_nMaterial != nMaterial)
        return;

    float fAlpha = 0.0f;
    float fFar = (maxgrass == 0) ? m_fFarRadiusSquared : 10000000.0f;
    float fNear = (maxgrass == 0) ? m_fNearRadiusSquared : 1000000.0f;

    if (m_minDistanceSquared < fNear) {
        fAlpha = 1.0f;
    } else if (m_minDistanceSquared < fFar) {
        fAlpha = 1.0f - (m_minDistanceSquared - fNear) / (fFar - fNear);
        if (fAlpha == 0.0f)
            return;
    } else {
        return;
    }

    const float* paAmbient = Homogenize(g_GrassInfo.m_pData[nMaterial].m_vAmbient, 1.0f);
    ASLgl::glMaterialfv(GL_FRONT_AND_BACK, materialTranslationTable[1], paAmbient);
    const float* paDiffuse = Homogenize(g_GrassInfo.m_pData[nMaterial].m_vDiffuse, fAlpha);
    ASLgl::glMaterialfv(GL_FRONT_AND_BACK, materialTranslationTable[2], paDiffuse);
#ifdef K2_FIX_ENGINE_BUGS
    if (AurNonRadeon8500Validate() && usearbvertexprograms != 0) {
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 94,
            g_GrassInfo.m_pData[nMaterial].m_vDiffuse.x,
            g_GrassInfo.m_pData[nMaterial].m_vDiffuse.y,
            g_GrassInfo.m_pData[nMaterial].m_vDiffuse.z, fAlpha);
        ASLgl::glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 95,
            g_GrassInfo.m_pData[nMaterial].m_vAmbient.x,
            g_GrassInfo.m_pData[nMaterial].m_vAmbient.y,
            g_GrassInfo.m_pData[nMaterial].m_vAmbient.z, 1.0f);
    }
#endif

    if (newgrass != 0 && m_minDistanceSquared < m_fNearRadiusSquared) {
        m_pGrassCursor = m_pGrassVerts;
        UpdateGrassForWind(pWind);
        m_pGrassCursor = 0;
    }

    if (rendertexturedgrass != 0)
        m_pTexture.m_pObject->Bind();

    GLRender::EnableClientState(Aurora::kClientStateVertex);
    GLRender::EnableClientState(Aurora::kClientStateNormal);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);

    // The normal array starts one Vector into each vertex record.
    int nNormalStride = m_nVertexStride - 12;

    if ((m_nFlags & kGrassLightmapped) != 0 && rendertexturedgrass != 0)
    {
        if (!AurNonRadeon8500Validate()) {
            GLRender::Enable(Aurora::kCapVertexProgram);

            static VertexProgram* pLightmappedVP =
                new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                                  cStandardVPDeclaration);
            pLightmappedVP->Enable(0, true);

            GLRender::SetVertexShaderConstant(94, g_GrassInfo.m_pData[nMaterial].m_vDiffuse.x,
                                              g_GrassInfo.m_pData[nMaterial].m_vDiffuse.y,
                                              g_GrassInfo.m_pData[nMaterial].m_vDiffuse.z, fAlpha);
            GLRender::SetVertexShaderConstant(95, g_GrassInfo.m_pData[nMaterial].m_vAmbient.x,
                                              g_GrassInfo.m_pData[nMaterial].m_vAmbient.y,
                                              g_GrassInfo.m_pData[nMaterial].m_vAmbient.z, 1.0f);
        }

        if (AurNV1xLevelShadersAvailable())
        {
            GLRender::ActivateTextureStage(1);
            GLRender::EnableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);

            s_useAlphaTestShader = true;
            int nStart = 0;
            for (int64_t i = 0; i < m_texCounts.m_count; i++) {
                GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmap, m_pTexture.m_pObject,
                                                  m_textureRefs.m_pData[i].m_pObject, 0, 0);
                GLRender::DrawLightmappedGrass(m_pGrassVerts, 12, &m_nVertexBytes, nNormalStride,
                                               nStart, m_texCounts.m_pData[i]);
                nStart += m_texCounts.m_pData[i] * 4;
            }
            s_useAlphaTestShader = false;
            GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);

            GLRender::ActivateTextureStage(1);
            GLRender::DisableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);
        }
        else if (AurMultiTextureAvailable())
        {
            GLRender::ActivateTextureStage(1);
            GLRender::EnableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);
            GLRender::Disable(Aurora::kCapLighting);
            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            GLRender::SetActiveTexture(0);
            GLRender::Enable(Aurora::kCapTexture2D);
            m_pTexture.m_pObject->Bind();
            ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

            int64_t i = 0;
            int nStart = 0;
            while (i < m_texCounts.m_count)
            {
                GLRender::SetActiveTexture(1);
                GLRender::Enable(Aurora::kCapTexture2D);
                m_textureRefs.m_pData[i].m_pObject->Bind();
                ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                GLRender::SetActiveTexture(0);
                GLRender::DrawLightmappedGrass(&m_nX48, 12, &m_nVertexBytes, nNormalStride,
                                               nStart, m_texCounts.m_pData[i]);
                nStart += m_texCounts.m_pData[i] * 4;
                i++;
            }

            GLRender::SetActiveTexture(0);
            ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            GLRender::SetActiveTexture(1);
            GLRender::Disable(Aurora::kCapTexture2D);
            GLRender::SetActiveTexture(0);
            ASLgl::glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
            GLRender::Enable(Aurora::kCapLighting);
            GLRender::ActivateTextureStage(1);
            GLRender::DisableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);
        }

        if (!AurNonRadeon8500Validate())
            GLRender::Disable(Aurora::kCapVertexProgram);
    }
    else
    {
#ifdef K2_FIX_ENGINE_BUGS
        s_useAlphaTestShader = true;
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, m_pTexture.m_pObject, 0, 0, 0);
        GLRender::DrawLightmappedGrass(m_pGrassVerts, 12, &m_nVertexBytes, nNormalStride, 0,
                                       m_nTotalTriangles);
        s_useAlphaTestShader = false;
        GLRender::pfuncEnablePixelShaders(GLRender::kPSNone, 0, 0, 0, 0);
#else
        GLRender::DrawLightmappedGrass(&m_nX48, 12, &m_nVertexBytes, nNormalStride, 0,
                                       m_nTotalTriangles);
#endif
    }
}
#else
void CAurTriangleBin::RenderGrassPolys(int nMaterial, const Vector& vCamera, WindManager* pWind)
{
    K2_UNUSED(vCamera);
    if (!AurNV1xLevelShadersAvailable() && !AurMultiTextureAvailable())
        return;
    if (!(m_nFlags & kGrassPolysBuilt))
        return;
    if (m_nTotalTriangles == 0)
        return;
    if (m_nMaterial != nMaterial)
        return;

    float fFar = (maxgrass == 0) ? m_fFarRadiusSquared : 10000000.0f;
    float fNear = (maxgrass == 0) ? m_fNearRadiusSquared : 1000000.0f;

    float fAlpha;
    if (m_minDistanceSquared < fNear) {
        fAlpha = 1.0f;
    } else if (m_minDistanceSquared < fFar) {
        fAlpha = 1.0f - (m_minDistanceSquared - fNear) / (fFar - fNear);
        if (fAlpha == 0.0f)
            return;
    } else {
        return;
    }

    float afAmbient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float afDiffuse[4] = { 0.0f, 0.0f, 0.0f, fAlpha };

    GLRender::SetMaterialv(Aurora::kFacingFront, Aurora::kMaterialAmbient, afAmbient);
    GLRender::SetMaterialv(Aurora::kFacingFront, Aurora::kMaterialDiffuse, afDiffuse);

    float afEmission[4] = { g_GrassInfo.m_pData[nMaterial].m_vEmissive.x,
                            g_GrassInfo.m_pData[nMaterial].m_vEmissive.y,
                            g_GrassInfo.m_pData[nMaterial].m_vEmissive.z, fAlpha };
    GLRender::SetMaterialv(Aurora::kFacingFront, Aurora::kMaterialEmission, afEmission);

    if (newgrass != 0 && m_minDistanceSquared < m_fNearRadiusSquared) {
        m_pGrassCursor = m_pGrassVerts;
        UpdateGrassForWind(pWind);
        m_pGrassCursor = 0;
    }

    if (rendertexturedgrass != 0)
        m_pTexture.m_pObject->Bind();

    GLRender::EnableClientState(Aurora::kClientStateVertex);
    GLRender::EnableClientState(Aurora::kClientStateNormal);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);

    // The normal array starts one Vector into each vertex record.
    int nNormalStride = m_nVertexStride - 12;

    if ((m_nFlags & kGrassLightmapped) != 0 && rendertexturedgrass != 0)
    {
        GLRender::Enable(Aurora::kCapVertexProgram);

        static VertexProgram* pLightmappedVP =
            new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                              cStandardVPDeclaration);
        pLightmappedVP->Enable(0, true);

        GLRender::SetVertexShaderConstant(94, g_GrassInfo.m_pData[nMaterial].m_vDiffuse.x,
                                          g_GrassInfo.m_pData[nMaterial].m_vDiffuse.y,
                                          g_GrassInfo.m_pData[nMaterial].m_vDiffuse.z, fAlpha);
        GLRender::SetVertexShaderConstant(95, g_GrassInfo.m_pData[nMaterial].m_vAmbient.x,
                                          g_GrassInfo.m_pData[nMaterial].m_vAmbient.y,
                                          g_GrassInfo.m_pData[nMaterial].m_vAmbient.z, 1.0f);

        if (AurNV1xLevelShadersAvailable())
        {
            GLRender::ActivateTextureStage(1);
            GLRender::EnableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);

            bool bNoForceSight = (g_ForceSight == 0);
            int64_t i = 0;
            int nStart = 0;
            while (i < m_texCounts.m_count)
            {
                CAurTexture* pRef = m_textureRefs.m_pData[i].m_pObject;
                if (!bNoForceSight) {
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatLM, m_pTexture.m_pObject, pRef, 0, 0);
                    float afSightTint[4] = { 0.0f, 0.0f, 0.0f, 0.75f };
                    ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afSightTint);
                } else {
                    GLRender::pfuncEnablePixelShaders(GLRender::kPSLightmap, m_pTexture.m_pObject, pRef, 0, 0);
                }
                GLRender::DrawLightmappedGrass(m_pGrassVerts, 12, &m_nVertexBytes, nNormalStride,
                                               nStart, m_texCounts.m_pData[i]);
                nStart += m_texCounts.m_pData[i] * 4;
                i++;
                bNoForceSight = (g_ForceSight == 0);
            }

            GLRender::pfuncEnablePixelShaders(bNoForceSight ? GLRender::kPSNone : GLRender::kPSDesatOff, 0, 0, 0, 0);

            GLRender::ActivateTextureStage(1);
            GLRender::DisableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);
        }
        else if (AurMultiTextureAvailable())
        {
            GLRender::ActivateTextureStage(1);
            GLRender::EnableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);
            GLRender::Disable(Aurora::kCapLighting);
            ASLgl::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            GLRender::SetActiveTexture(0);
            GLRender::Enable(Aurora::kCapTexture2D);
            m_pTexture.m_pObject->Bind();
            ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

            int64_t i = 0;
            int nStart = 0;
            while (i < m_texCounts.m_count)
            {
                GLRender::SetActiveTexture(1);
                GLRender::Enable(Aurora::kCapTexture2D);
                m_textureRefs.m_pData[i].m_pObject->Bind();
                ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                GLRender::SetActiveTexture(0);
                GLRender::DrawLightmappedGrass(&m_nX48, 12, &m_nVertexBytes, nNormalStride,
                                               nStart, m_texCounts.m_pData[i]);
                nStart += m_texCounts.m_pData[i] * 4;
                i++;
            }

            GLRender::SetActiveTexture(0);
            ASLgl::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            GLRender::SetActiveTexture(1);
            GLRender::Disable(Aurora::kCapTexture2D);
            GLRender::SetActiveTexture(0);
            ASLgl::glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
            GLRender::Enable(Aurora::kCapLighting);
            GLRender::ActivateTextureStage(1);
            GLRender::DisableClientState(Aurora::kClientStateTexCoord);
            GLRender::ActivateTextureStage(0);
        }

        if (!AurNonRadeon8500Validate())
            GLRender::Disable(Aurora::kCapVertexProgram);
    }
    else
    {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, m_pTexture.m_pObject, 0, 0, 0);
        GLRender::Enable(Aurora::kCapVertexProgram);

        static VertexProgram* pGrassVP =
            new VertexProgram(usearbvertexprograms != 0 ? szStandardVPARB : cStandardVP,
                              cStandardVPDeclaration);
        pGrassVP->Enable(0, true);

        GLRender::SetVertexShaderConstant(94, g_GrassInfo.m_pData[nMaterial].m_vDiffuse.x,
                                          g_GrassInfo.m_pData[nMaterial].m_vDiffuse.y,
                                          g_GrassInfo.m_pData[nMaterial].m_vDiffuse.z, fAlpha);
        GLRender::SetVertexShaderConstant(95, g_GrassInfo.m_pData[nMaterial].m_vAmbient.x,
                                          g_GrassInfo.m_pData[nMaterial].m_vAmbient.y,
                                          g_GrassInfo.m_pData[nMaterial].m_vAmbient.z, 1.0f);
        GLRender::SetVertexShaderConstant(93, g_GrassInfo.m_pData[nMaterial].m_vEmissive.x,
                                          g_GrassInfo.m_pData[nMaterial].m_vEmissive.y,
                                          g_GrassInfo.m_pData[nMaterial].m_vEmissive.z, 1.0f);

        if (g_ForceSight) {
            GLRender::pfuncEnablePixelShaders(GLRender::kPSDesat, m_pTexture.m_pObject, 0, 0, 0);
            float afSight[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afSight);
        }

        GLRender::DrawLightmappedGrass(m_pGrassVerts, 12, &m_nVertexBytes, nNormalStride, 0,
                                       m_nTotalTriangles);

        if (g_ForceSight)
            GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
    }

#if !KOTOR1  // K1 keeps no draw counters
    countfaces += m_nTotalTriangles;
#endif
}
#endif  // KOTOR1

void CAurTriangleBin::UpdateGrassForWind(WindManager* pWind)
{
    if (!(m_nFlags & kGrassPolysBuilt))
        return;
    if (enablegrasswind == 0)
        return;
    if (!currentscene->IsAnimating())
        return;

    float fDelta = DeltaT;
    if (fDelta > 0.1f)
        fDelta = 0.1f;

    static Vector vSpringDist;
    static Vector vForceDirection;
    static Vector vWind;
    static Vector vPoint;
    static Vector vMiddle;

    float fWindScale = magnitude(pWind->m_vWind);
    fWindScale = (fWindScale > 5.0f) ? 5.0f / fWindScale : 1.0f;

    if (m_nTotalTriangles > 0)
    {
        float fSpring = fDelta * g_GrassInfo.m_pData[m_nMaterial].m_fQuadSize * grassstiffness;

        int nPos = 0;
        int64_t n = 0;
        while (n < m_nTotalTriangles)
        {
            Vector vA = reinterpret_cast<Vector*>(m_pGrassCursor)[nPos];
            Vector vB = reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 1];

            vMiddle.x = (vA.x + vB.x) * 0.5f;
            vMiddle.y = (vA.y + vB.y) * 0.5f;
            vMiddle.z = vA.z;

            vWind = pWind->GetGlobalWindNoPoint(vMiddle, 1.0f) * fWindScale;
            vPoint = pWind->GetGlobalPointWind(vMiddle, fDelta);

            Vector vC = reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 3];

            vSpringDist.x = vA.x - vC.x;
            vSpringDist.y = vA.y - vC.y;
            vSpringDist.z = 0.0f;

            vForceDirection.x = vWind.x + vSpringDist.x + vPoint.x;
            vForceDirection.y = vWind.y + vSpringDist.y + vPoint.y;
            vForceDirection.z = vWind.z + vSpringDist.z + vPoint.z;

            if (!vForceDirection.IsZero())
            {
                float fNewX = vC.x + fSpring * vForceDirection.x;
                float fNewY = vC.y + fSpring * vForceDirection.y;

                reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 3].x = fNewX;
                reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 3].y = fNewY;
                reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 2].x = vB.x + (fNewX - vA.x);
                reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 2].y = vB.y + (fNewY - vA.y);
            }

            nPos += 4;
            n++;
        }
    }
}

// Explicit instantiation, so SafePointerList<CAurTexture>'s members are emitted in this unit.

template class SafePointerList<CAurTexture>;


extern int quaternionidentityoptimization;
extern int enablelightmap;
extern void AurTextureSetModeStatic();
extern void AurTextureSetModeDynamic();
extern CAurTexture* AurTextureGetReference(char* sName, char* sExtension);
extern Vector VectorMin(Vector a, Vector b);
extern Vector VectorMax(Vector a, Vector b);

// Rotate v into world space by q, with the identity-quaternion fast path.
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

// Append the grass blades for one walkmesh face and fold the triangle into the bin bounds.
void CAurTriangleBin::AddTriangle(const MaxFace& face, PartTriMesh* pMesh, MdlNodeTriMesh* pNode)
{
    static Vector v0, v1, v2;

    float* pv = pNode->m_paVertices;

    if (m_nMaterial == -1) {
        m_nMaterial = face.m_nMaterial;
        AurTextureSetModeStatic();
        m_pTexture = AurTextureGetReference(g_GrassInfo.m_pData[m_nMaterial].m_sName, 0);
        AurTextureSetModeDynamic();
    }

    // Per-scalar staging: the three components are copied independently.
    float* pf0 = &pv[face.m_nVIndex[0] * 3];
    v0.x = pf0[0]; v0.y = pf0[1]; v0.z = pf0[2];
    float* pf1 = &pv[face.m_nVIndex[1] * 3];
    v1.x = pf1[0]; v1.y = pf1[1]; v1.z = pf1[2];
    float* pf2 = &pv[face.m_nVIndex[2] * 3];
    v2.x = pf2[0]; v2.y = pf2[1]; v2.z = pf2[2];

    Vector vTranslation;
    Quaternion qLocal(1.0f, 0.0f, 0.0f, 0.0f);
    pMesh->GetWorld(vTranslation, qLocal);

    Vector r0 = RotateVector(v0, qLocal);
    r0.x += vTranslation.x;
    r0.y += vTranslation.y;
    r0.z += vTranslation.z;
    v0 = r0;

    Vector r1 = RotateVector(v1, qLocal);
    r1.x += vTranslation.x;
    r1.y += vTranslation.y;
    r1.z += vTranslation.z;
    v1 = r1;

    Vector r2 = RotateVector(v2, qLocal);
    r2.x += vTranslation.x;
    r2.y += vTranslation.y;
    r2.z += vTranslation.z;
    v2 = r2;

    Vector e1 = v1 - v0;
    Vector e2 = v2 - v1;
    float fMag = magnitude(Vector(e1.y * e2.z - e1.z * e2.y,
                                  e1.z * e2.x - e1.x * e2.z,
                                  e1.x * e2.y - e1.y * e2.x));
    int nCount = static_cast<int>(fMag * 0.5f * g_GrassInfo.m_pData[face.m_nMaterial].m_fDensity);
    if (nCount == 0)
        return;

    if (pNode->m_bHasLightmap && enablelightmap)
        m_nFlags |= kGrassLightmapped;
    if (pMesh->m_bHasColorPool && !(m_nFlags & kGrassLightmapped))
        m_nFlags |= kGrassVertexColors;

    List<float> data;
    data.Add(v0.x);
    data.Add(v0.y);
    data.Add(v0.z);
    data.Add(v1.x);
    data.Add(v1.y);
    data.Add(v1.z);
    data.Add(v2.x);
    data.Add(v2.y);
    data.Add(v2.z);

    CAurTriangleListElem elem(data, const_cast<MaxFace*>(&face), pMesh, nCount);
    m_elements.Add(elem);

    m_nTotalTriangles += nCount;

    if (!(m_nFlags & kGrassBoundsSeeded)) {
        m_vMin = v0;
        m_vMax = v0;
        m_nFlags |= kGrassBoundsSeeded;
    } else {
        m_vMin = VectorMin(m_vMin, v0);
        m_vMax = VectorMax(m_vMax, v0);
    }
    m_vMin = VectorMin(m_vMin, v1);
    m_vMax = VectorMax(m_vMax, v1);
    m_vMin = VectorMin(m_vMin, v2);
    m_vMax = VectorMax(m_vMax, v2);
    m_vCenter = (m_vMin + m_vMax) * 0.5f;
}

#include "port/crtapi.h"

// Grass vertex-array builder. The buffer is two blocks: one displaced quad (four Vectors) per
// grass sample in blade order, then one attribute record per vertex holding the normal, the uv pair
// and a tail selected by m_nVertexStride (nothing, one colour float, or a lightmap uv pair).
void CAurTriangleBin::CreateArrays()
{
    int nVerts = m_nTotalTriangles * 4;
    int nBytes = nVerts * m_nVertexStride;
    m_pGrassVerts = new char[nBytes];
    m_nVertexBytes = nVerts * sizeof(Vector);
    m_pGrassCursor = m_pGrassVerts;

    if (m_nFlags & kGrassLightmapped)
        SortTriangles();
    m_nFlags |= kGrassArrays;

    if (m_elements.m_count > 0) {
        float* pF = reinterpret_cast<float*>(m_pGrassVerts);
        int iAttr = nVerts * 3 - 1;   // the last float written by the previous attribute record
        int nPos = 0;                  // in Vector units; 4 per blade

        for (int i = 0; i < m_elements.m_count; i++) {
            static Vector v0, v1, v2;
            CAurTriangleListElem& elem = m_elements.m_pData[i];
            Vector* pTri = reinterpret_cast<Vector*>(elem.m_vertexList.m_pData);
            v0 = pTri[0];
            v1 = pTri[1];
            v2 = pTri[2];

            int nCount = elem.m_nGrassCount;
            if (nCount == 0)
                continue;

            srand_wincompatible(static_cast<int>(v0.x + v1.y + v2.z));
            float fWidth = g_GrassInfo.m_pData[m_nMaterial].m_fQuadSize;
            MaxFace* pFace = elem.m_pMaxFace;
            MdlNodeTriMesh* pTriMesh = elem.m_pPartMesh->m_pNode->AsMdlNodeTriMesh();
            if (nCount <= 0)
                continue;

            float fHalf = fWidth * 0.5f;
            float fNegHalf = fWidth * -0.5f;

            for (int j = 0; j < nCount; j++) {
                // m_nTotalTriangles is the sum of the elements' grass counts; the stride is the
                // record written below
                K2_ASSUME((nPos + 4) * static_cast<int>(sizeof(Vector)) <= nBytes);
                K2_ASSUME((iAttr + 1) * 4 + 4 * (m_nVertexStride - 12) <= nBytes);
                float fR0 = static_cast<float>(rand_wincompatible() % 10000) / 10000.0f;
                float fR1 = static_cast<float>(rand_wincompatible() % 10000) / 10000.0f;
                float fR2 = static_cast<float>(rand_wincompatible() % 10000) / 10000.0f;
                float fSum = fR0 + fR1 + fR2;
                if (fSum != 0.0f) {
                    fSum = 1.0f / fSum;
                    fR0 *= fSum;
                    fR1 *= fSum;
                    fR2 *= fSum;
                }

                float fLmU = 0.0f;
                float fLmV = 0.0f;
                int nC0 = 0;
                int nC1 = 0;
                int nC2 = 0;
                int nC3 = 0;

                if (m_nFlags & kGrassLightmapped) {
                    // Per-vertex lightmap uv pair, blended with the barycentric weights.
                    Model* pModel = pTriMesh->m_pTree->AsModel();
                    char* pPool = static_cast<char*>(GLRender::LockPool(pModel->m_nPool, 0, 0, 8))
                                  + pTriMesh->m_nTex1Ofs + pTriMesh->m_nGeomBase;
                    int nStride = pTriMesh->m_nVertexStride;
                    float* pU0 = reinterpret_cast<float*>(pPool + nStride * pFace->m_nVIndex[0]);
                    float* pU1 = reinterpret_cast<float*>(pPool + nStride * pFace->m_nVIndex[1]);
                    float* pU2 = reinterpret_cast<float*>(pPool + nStride * pFace->m_nVIndex[2]);
                    fLmU = pU0[0] * fR0 + pU1[0] * fR1 + pU2[0] * fR2;
                    fLmV = pU0[1] * fR0 + pU1[1] * fR1 + pU2[1] * fR2;
                    pModel = pTriMesh->m_pTree->AsModel();
                    GLRender::UnlockPool(pModel->m_nPool);
                } else if (m_nFlags & kGrassVertexColors) {
                    // One packed colour dword per vertex, blended and clamped per channel.
                    int nColorPool = elem.m_pPartMesh->m_nColorPool;
                    unsigned int* pPool = static_cast<unsigned int*>(GLRender::LockPool(nColorPool, 0, 0, 8));
                    unsigned int c0 = pPool[pFace->m_nVIndex[0]];
                    unsigned int c1 = pPool[pFace->m_nVIndex[1]];
                    unsigned int c2 = pPool[pFace->m_nVIndex[2]];
                    float fC0 = fmaxf(fminf(static_cast<float>(c0 & 0xff) * fR0 +
                                            static_cast<float>(c1 & 0xff) * fR1 +
                                            static_cast<float>(c2 & 0xff) * fR2, 255.0f), 0.0f);
                    float fC1 = fmaxf(fminf(static_cast<float>((c0 >> 8) & 0xff) * fR0 +
                                            static_cast<float>((c1 >> 8) & 0xff) * fR1 +
                                            static_cast<float>((c2 >> 8) & 0xff) * fR2, 255.0f), 0.0f);
                    float fC2 = fmaxf(fminf(static_cast<float>((c0 >> 16) & 0xff) * fR0 +
                                            static_cast<float>((c1 >> 16) & 0xff) * fR1 +
                                            static_cast<float>((c2 >> 16) & 0xff) * fR2, 255.0f), 0.0f);
                    float fC3 = fmaxf(fminf(static_cast<float>(c0 >> 24) * fR0 +
                                            static_cast<float>(c1 >> 24) * fR1 +
                                            static_cast<float>(c2 >> 24) * fR2, 255.0f), 0.0f);
                    nC0 = static_cast<int>(fC0);
                    nC1 = static_cast<int>(fC1);
                    nC2 = static_cast<int>(fC2);
                    nC3 = static_cast<int>(fC3);
                    GLRender::UnlockPool(nColorPool);
                }

                Vector vBase = v0 * fR0 + v1 * fR1 + v2 * fR2;
                Quaternion q(Vector(0.0f, 0.0f, 1.0f),
                             static_cast<float>(rand_wincompatible() % 10000) / 10000.0f * 2.0f * 3.1415927f);

                // Four cumulative style thresholds pick one 0.5-step uv octant.
                float fStyle = static_cast<float>(rand_wincompatible() % 10000) / 10000.0f;
                const CAurGrassInfo& gi = g_GrassInfo.m_pData[m_nMaterial];
                int nStyle;
                if (fStyle < gi.m_fProbLL) {
                    nStyle = 0;
                } else {
                    fStyle -= gi.m_fProbLL;
                    if (fStyle < gi.m_fProbLR) {
                        nStyle = 1;
                    } else {
                        fStyle -= gi.m_fProbLR;
                        if (fStyle < gi.m_fProbUL) {
                            nStyle = 2;
                        } else {
                            fStyle -= gi.m_fProbUL;
                            if (fStyle < gi.m_fProbUR)
                                nStyle = 3;
                            else
                                nStyle = 0;
                        }
                    }
                }
                fR0 = (nStyle & 1) ? 0.5f : 0.0f;
                fR1 = (nStyle & 2) ? 0.5f : 0.0f;
                float fScale = (static_cast<float>(rand_wincompatible() % 10000) / 10000.0f - 0.5f) * 0.4f + 1.0f;

                // Corner 0: bottom-left.
                reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 0] =
                    vBase + RotateVector(Vector(fNegHalf, 0.0f, 0.0f), q);
                *reinterpret_cast<int*>(&pF[iAttr + 1]) = pFace->m_nNormal[0];
                *reinterpret_cast<int*>(&pF[iAttr + 2]) = pFace->m_nNormal[1];
                *reinterpret_cast<int*>(&pF[iAttr + 3]) = pFace->m_nNormal[2];
                pF[iAttr + 4] = fR0;
                pF[iAttr + 5] = fR1;
                iAttr += 5;
                if (m_nFlags & kGrassLightmapped) {
                    pF[iAttr + 1] = fLmU;
                    pF[iAttr + 2] = fLmV;
                    iAttr += 2;
                } else if (m_nFlags & kGrassVertexColors) {
                    pF[iAttr + 1] = static_cast<float>(static_cast<unsigned int>((nC0 & 0xff) | ((nC1 & 0xff) << 8) |
                                                          ((nC2 & 0xff) << 16) | (nC3 << 24)));
                    iAttr += 1;
                }

                // Corner 1: bottom-right.
                reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 1] =
                    vBase + RotateVector(Vector(fHalf, 0.0f, 0.0f), q);
                *reinterpret_cast<int*>(&pF[iAttr + 1]) = pFace->m_nNormal[0];
                *reinterpret_cast<int*>(&pF[iAttr + 2]) = pFace->m_nNormal[1];
                *reinterpret_cast<int*>(&pF[iAttr + 3]) = pFace->m_nNormal[2];
                pF[iAttr + 4] = fR0 + 0.5f;
                pF[iAttr + 5] = fR1;
                iAttr += 5;
                if (m_nFlags & kGrassLightmapped) {
                    pF[iAttr + 1] = fLmU;
                    pF[iAttr + 2] = fLmV;
                    iAttr += 2;
                } else if (m_nFlags & kGrassVertexColors) {
                    pF[iAttr + 1] = static_cast<float>(static_cast<unsigned int>((nC0 & 0xff) | ((nC1 & 0xff) << 8) |
                                                          ((nC2 & 0xff) << 16) | (nC3 << 24)));
                    iAttr += 1;
                }

                // Corner 2: top-right, scaled by the random blade height.
                reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 2] =
                    vBase + RotateVector(Vector(fHalf, 0.0f, fWidth), q) * fScale;
                *reinterpret_cast<int*>(&pF[iAttr + 1]) = pFace->m_nNormal[0];
                *reinterpret_cast<int*>(&pF[iAttr + 2]) = pFace->m_nNormal[1];
                *reinterpret_cast<int*>(&pF[iAttr + 3]) = pFace->m_nNormal[2];
                pF[iAttr + 4] = fR0 + 0.5f;
                pF[iAttr + 5] = fR1 + 0.5f;
                iAttr += 5;
                if (m_nFlags & kGrassLightmapped) {
                    pF[iAttr + 1] = fLmU;
                    pF[iAttr + 2] = fLmV;
                    iAttr += 2;
                } else if (m_nFlags & kGrassVertexColors) {
                    pF[iAttr + 1] = static_cast<float>(static_cast<unsigned int>((nC0 & 0xff) | ((nC1 & 0xff) << 8) |
                                                          ((nC2 & 0xff) << 16) | (nC3 << 24)));
                    iAttr += 1;
                }

                // Corner 3: top-left, scaled by the random blade height.
                reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 3] =
                    vBase + RotateVector(Vector(fNegHalf, 0.0f, fWidth), q) * fScale;
                *reinterpret_cast<int*>(&pF[iAttr + 1]) = pFace->m_nNormal[0];
                *reinterpret_cast<int*>(&pF[iAttr + 2]) = pFace->m_nNormal[1];
                *reinterpret_cast<int*>(&pF[iAttr + 3]) = pFace->m_nNormal[2];
                pF[iAttr + 4] = fR0;
                pF[iAttr + 5] = fR1 + 0.5f;
                iAttr += 5;
                if (m_nFlags & kGrassLightmapped) {
                    pF[iAttr + 1] = fLmU;
                    pF[iAttr + 2] = fLmV;
                    iAttr += 2;
                } else if (m_nFlags & kGrassVertexColors) {
                    pF[iAttr + 1] = static_cast<float>(static_cast<unsigned int>((nC0 & 0xff) | ((nC1 & 0xff) << 8) |
                                                          ((nC2 & 0xff) << 16) | (nC3 << 24)));
                    iAttr += 1;
                }

                nPos += 4;
            }
        }
    }

    m_pGrassCursor = 0;
    m_nFlags |= kGrassPolysBuilt;
}

void CAurTriangleBin::ReloadColor() {}

CAurTriangleBin::~CAurTriangleBin()
{
    if (m_nFlags & kGrassArrays) {
        delete[] m_pGrassCursor;
        m_pGrassCursor = 0;
        delete[] m_pGrassVerts;
        m_pGrassVerts = 0;
#if KOTOR1
        m_nX48 = 0;           // K1: also clears the attribute pair
        m_nVertexBytes = 0;
#endif
        m_nFlags ^= kGrassArrays;
    }

    CAurTexture* pOldTexture = m_pTexture.m_pObject;
    m_pTexture = 0;
    AurTextureRelease(pOldTexture);
}

void CAurTriangleBin::CreateTriangleGrass(const Vector& v0, const Vector& v1, const Vector& v2,
                                          int nCount, int nElem, RenderVertexV* pVerts,
                                          float* pAttr)
{
    srand_wincompatible(static_cast<int>(v0.x + v1.y + v2.z));
    float fWidth = g_GrassInfo.m_pData[m_nMaterial].m_fQuadSize;
    m_pGrassCursor = reinterpret_cast<char*>(pVerts);

    CAurTriangleListElem& elem = m_elements.m_pData[nElem];
    MaxFace* pFace = elem.m_pMaxFace;
    MdlNodeTriMesh* pTriMesh = elem.m_pPartMesh->m_pNode->AsMdlNodeTriMesh();
    int nN0 = pFace->m_nNormal[0];
    int nN1 = pFace->m_nNormal[1];
    int nN2 = pFace->m_nNormal[2];

    float fLmU = 0.0f, fLmV = 0.0f;
    int nC0 = 0, nC1 = 0, nC2 = 0, nC3 = 127;
    if (m_nFlags & kGrassLightmapped) {
        Model* pModel = pTriMesh->m_pTree->AsModel();
        char* pPool = static_cast<char*>(GLRender::LockPool(pModel->m_nPool, 0, 0, 8))
                      + pTriMesh->m_nTex1Ofs + pTriMesh->m_nGeomBase;
        int nStride = pTriMesh->m_nVertexStride;
        float* pU0 = reinterpret_cast<float*>(pPool + nStride * pFace->m_nVIndex[0]);
        float* pU1 = reinterpret_cast<float*>(pPool + nStride * pFace->m_nVIndex[1]);
        float* pU2 = reinterpret_cast<float*>(pPool + nStride * pFace->m_nVIndex[2]);
        fLmU = pU0[0] * 0.0f + pU1[0] * 0.0f + pU2[0] * 0.0f;
        fLmV = pU0[1] * 0.0f + pU1[1] * 0.0f + pU2[1] * 0.0f;
        pModel = pTriMesh->m_pTree->AsModel();
        GLRender::UnlockPool(pModel->m_nPool);
    } else if (m_nFlags & kGrassVertexColors) {
        int nColorPool = elem.m_pPartMesh->m_nColorPool;
        unsigned int* pPool = static_cast<unsigned int*>(GLRender::LockPool(nColorPool, 0, 0, 8));
        unsigned int c0 = pPool[pFace->m_nVIndex[0]];
        unsigned int c1 = pPool[pFace->m_nVIndex[1]];
        unsigned int c2 = pPool[pFace->m_nVIndex[2]];
        float fC0 = fmaxf(fminf(static_cast<float>(c0 & 0xff) * 0.0f +
                                static_cast<float>(c1 & 0xff) * 0.0f +
                                static_cast<float>(c2 & 0xff) * 0.0f, 255.0f), 0.0f);
        float fC1 = fmaxf(fminf(static_cast<float>((c0 >> 8) & 0xff) * 0.0f +
                                static_cast<float>((c1 >> 8) & 0xff) * 0.0f +
                                static_cast<float>((c2 >> 8) & 0xff) * 0.0f, 255.0f), 0.0f);
        float fC2 = fmaxf(fminf(static_cast<float>((c0 >> 16) & 0xff) * 0.0f +
                                static_cast<float>((c1 >> 16) & 0xff) * 0.0f +
                                static_cast<float>((c2 >> 16) & 0xff) * 0.0f, 255.0f), 0.0f);
        float fC3 = fmaxf(fminf(static_cast<float>(c0 >> 24) * 0.0f +
                                static_cast<float>(c1 >> 24) * 0.0f +
                                static_cast<float>(c2 >> 24) * 0.0f, 255.0f), 0.0f);
        nC0 = static_cast<int>(fC0);
        nC1 = static_cast<int>(fC1);
        nC2 = static_cast<int>(fC2);
        nC3 = static_cast<int>(fC3);
        GLRender::UnlockPool(nColorPool);
    }
    float fColor = static_cast<float>(static_cast<unsigned int>(nC0 | (nC1 << 8) | (nC2 << 16) | (nC3 << 24)));

    if (nCount <= 0)
        return;

    float fHalf = fWidth * 0.5f;
    float fNegHalf = fWidth * -0.5f;
    int iAttr = 0;   // float index of the current vertex record
    int nPos = 0;    // Vector index, 4 per blade

    for (int j = 0; j < nCount; j++) {
        float fR0 = static_cast<float>(rand_wincompatible() % 10000) / 10000.0f;
        float fR1 = static_cast<float>(rand_wincompatible() % 10000) / 10000.0f;
        float fR2 = static_cast<float>(rand_wincompatible() % 10000) / 10000.0f;
        float fSum = fR0 + fR1 + fR2;
        if (fSum != 0.0f) {
            fSum = 1.0f / fSum;
            fR0 *= fSum;
            fR1 *= fSum;
            fR2 *= fSum;
        }

        Vector vBase = v0 * fR0 + v1 * fR1 + v2 * fR2;
        Quaternion q(Vector(0.0f, 0.0f, 1.0f),
                     static_cast<float>(rand_wincompatible() % 10000) / 10000.0f * 2.0f * 3.1415927f);

        float fStyle = static_cast<float>(rand_wincompatible() % 10000) / 10000.0f;
        const CAurGrassInfo& gi = g_GrassInfo.m_pData[m_nMaterial];
        int nStyle;
        if (fStyle < gi.m_fProbLL) {
            nStyle = 0;
        } else {
            fStyle -= gi.m_fProbLL;
            if (fStyle < gi.m_fProbLR) {
                nStyle = 1;
            } else {
                fStyle -= gi.m_fProbLR;
                if (fStyle < gi.m_fProbUL) {
                    nStyle = 2;
                } else {
                    fStyle -= gi.m_fProbUL;
                    if (fStyle < gi.m_fProbUR)
                        nStyle = 3;
                    else
                        nStyle = 0;
                }
            }
        }
        fR0 = (nStyle & 1) ? 0.5f : 0.0f;
        fR1 = (nStyle & 2) ? 0.5f : 0.0f;
        float fScale = (static_cast<float>(rand_wincompatible() % 10000) / 10000.0f - 0.5f) * 0.4f + 1.0f;

        // Corner 0: bottom-left.
        Vector vC = vBase + RotateVector(Vector(fNegHalf, 0.0f, 0.0f), q);
        reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 0] = vC;
        RenderVertexVTN* pV = reinterpret_cast<RenderVertexVTN*>(&pAttr[iAttr]);
        pV->pos[0] = vC.x;
        pV->pos[1] = vC.y;
        pV->pos[2] = vC.z;
        pV->m_nNormal[0] = nN0;
        pV->m_nNormal[1] = nN1;
        pV->m_nNormal[2] = nN2;
        pV->u = fR0 + 0.0f;
        pV->v = fR1 + 0.0f;
        iAttr += 8;
        if (m_nFlags & kGrassLightmapped) {
            pAttr[iAttr] = fLmU;
            pAttr[iAttr + 1] = fLmV;
            iAttr += 2;
        } else if (m_nFlags & kGrassVertexColors) {
            pAttr[iAttr] = fColor;
            iAttr += 1;
        }

        // Corner 1: bottom-right.
        vC = vBase + RotateVector(Vector(fHalf, 0.0f, 0.0f), q);
        reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 1] = vC;
        pV = reinterpret_cast<RenderVertexVTN*>(&pAttr[iAttr]);
        pV->pos[0] = vC.x;
        pV->pos[1] = vC.y;
        pV->pos[2] = vC.z;
        pV->m_nNormal[0] = nN0;
        pV->m_nNormal[1] = nN1;
        pV->m_nNormal[2] = nN2;
        pV->u = fR0 + 0.5f;
        pV->v = fR1 + 0.0f;
        iAttr += 8;
        if (m_nFlags & kGrassLightmapped) {
            pAttr[iAttr] = fLmU;
            pAttr[iAttr + 1] = fLmV;
            iAttr += 2;
        } else if (m_nFlags & kGrassVertexColors) {
            pAttr[iAttr] = fColor;
            iAttr += 1;
        }

        // Corner 2: top-right, scaled by the random blade height.
        vC = vBase + RotateVector(Vector(fHalf, 0.0f, fWidth), q) * fScale;
        reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 2] = vC;
        pV = reinterpret_cast<RenderVertexVTN*>(&pAttr[iAttr]);
        pV->pos[0] = vC.x;
        pV->pos[1] = vC.y;
        pV->pos[2] = vC.z;
        pV->m_nNormal[0] = nN0;
        pV->m_nNormal[1] = nN1;
        pV->m_nNormal[2] = nN2;
        pV->u = fR0 + 0.5f;
        pV->v = fR1 + 0.5f;
        iAttr += 8;
        if (m_nFlags & kGrassLightmapped) {
            pAttr[iAttr] = fLmU;
            pAttr[iAttr + 1] = fLmV;
            iAttr += 2;
        } else if (m_nFlags & kGrassVertexColors) {
            pAttr[iAttr] = fColor;
            iAttr += 1;
        }

        // Corner 3: top-left, scaled by the random blade height.
        vC = vBase + RotateVector(Vector(fNegHalf, 0.0f, fWidth), q) * fScale;
        reinterpret_cast<Vector*>(m_pGrassCursor)[nPos + 3] = vC;
        pV = reinterpret_cast<RenderVertexVTN*>(&pAttr[iAttr]);
        pV->pos[0] = vC.x;
        pV->pos[1] = vC.y;
        pV->pos[2] = vC.z;
        pV->m_nNormal[0] = nN0;
        pV->m_nNormal[1] = nN1;
        pV->m_nNormal[2] = nN2;
        pV->u = fR0 + 0.0f;
        pV->v = fR1 + 0.5f;
        iAttr += 8;
        if (m_nFlags & kGrassLightmapped) {
            pAttr[iAttr] = fLmU;
            pAttr[iAttr + 1] = fLmV;
            iAttr += 2;
        } else if (m_nFlags & kGrassVertexColors) {
            pAttr[iAttr] = fColor;
            iAttr += 1;
        }

        nPos += 4;
    }
}

#include <stddef.h>

CAurTriangleListElem::CAurTriangleListElem()
{
    __builtin_memset(static_cast<void*>(this), 0, offsetof(CAurTriangleListElem, m_nGrassCount) + sizeof(m_nGrassCount));
}


CAurTriangleListElem::~CAurTriangleListElem()
{
    m_vertexList.SetSize(0);
    m_nGrassCount = 0;
    m_pMaxFace = 0;
    m_pPartMesh = 0;
}

// Explicit instantiations for the container members emitted in this unit.
template List<CAurTriangleListElem>::~List();

template void List<CAurTriangleListElem>::allocate(int);

template List<CAurTriangleListElem>::List(List<CAurTriangleListElem>&);

template void List<SafePointer<CAurTexture>>::Add(SafePointer<CAurTexture>);

// Legacy stub class, not referenced by the game.

CAurTriangleList::CAurTriangleList() {}

CAurTriangleList::CAurTriangleList(int nNumFaces) { K2_UNUSED(nNumFaces); }

CAurTriangleList::~CAurTriangleList() {}

void CAurTriangleList::AddTriangle(const MaxFace& face, PartTriMesh* pPart, MdlNodeTriMesh* pNode,
                                   Vector& vA, Vector& vB) { K2_UNUSED(face); K2_UNUSED(pPart); K2_UNUSED(pNode); K2_UNUSED(vA); K2_UNUSED(vB); }

void CAurTriangleList::ReloadColor() {}

void CAurTriangleList::DestroyGrassPolys() {}

void CAurTriangleList::RenderGrassPolys(WindManager* pWind, float fRadius)
{
    K2_UNUSED(pWind); K2_UNUSED(fRadius);
#if !KOTOR1  // K1 keeps no draw counters
    countfaces += m_nNumFaces;
#endif
}
