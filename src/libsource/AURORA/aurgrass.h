#pragma once

#include <stddef.h>

#include "Vector.h"

// Per-material grass-layer descriptor, one row of Grass.2da.
class CAurGrassInfo {
public:
    char          m_sName[32];
    float         m_fDensity;
    float         m_fQuadSize;
    Vector        m_vDiffuse;
    Vector        m_vAmbient;
#if !KOTOR1  // K1 has no emissive triple
    Vector        m_vEmissive;
#endif
    float         m_fProbLL;
    float         m_fProbLR;
    float         m_fProbUL;
    float         m_fProbUR;
    unsigned char m_bInUse;

    CAurGrassInfo();
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CAurGrassInfo) == 0x54, "K1 CAurGrassInfo record (AurGrassMaterialInUse stride)");
#endif


#include "List.h"

class MaxFace;
class PartTriMesh;

class CAurTriangleListElem {
public:
    List<float>  m_vertexList;
    MaxFace*     m_pMaxFace;
    PartTriMesh* m_pPartMesh;
    int          m_nGrassCount;

    CAurTriangleListElem();
#if KOTOR1
    __attribute__((used)) CAurTriangleListElem(List<float> data, MaxFace* pMaxFace, PartTriMesh* pPartMesh, int nCount)
        : m_vertexList(data)
    {
#else
    __attribute__((used)) CAurTriangleListElem(List<float> data, MaxFace* pMaxFace, PartTriMesh* pPartMesh, int nCount)
    {
        m_vertexList = data;
#endif
        m_pMaxFace = pMaxFace;
        m_pPartMesh = pPartMesh;
        m_nGrassCount = nCount;
    }

    ~CAurTriangleListElem();
};


#include "Vector.h"
#include "SafePointer.h"

class WindManager;
class MaxFace;
class PartTriMesh;
class MdlNodeTriMesh;
class CAurTexture;
struct RenderVertexV;

class CAurTriangleBin {
public:
    CAurTriangleBin();
    CAurTriangleBin(const Vector& vMin, const Vector& vMax);
    ~CAurTriangleBin();
    void SetMinDistanceSquared(float d);
    void RenderGrassPolys(int nMaterial, const Vector& vCamera, WindManager* pWind);
    void AddTriangle(const MaxFace& face, PartTriMesh* pPart, MdlNodeTriMesh* pNode);
    void BuildGrassPolys();
    void CreateArrays();
    void DestroyGrassPolys();
    void FinishGeometry();
    void UpdateGrassForWind(WindManager* pWind);
    void CreateTriangleGrass(const Vector& v0, const Vector& v1, const Vector& v2, int nCount,
                             int nElem, RenderVertexV* pVerts, float* pAttr);
    void SortTriangles();

    // m_nFlags
    enum {
        kGrassArrays = 0x1,         // the vertex arrays are allocated
        kGrassBoundsSeeded = 0x2,
        kGrassPolysBuilt = 0x4,
        kGrassLightmapped = 0x8,
        kGrassVertexColors = 0x10,  // the mesh's colour pool tints the blades
    };

    unsigned short m_nVertexStride;
    unsigned char m_nFlags;
    char  _pad03[0x4 - 0x3];
    float m_minDistanceSquared;
    float m_fRadius;
    int   m_nTotalTriangles;
    int   m_nMaterial;
    Vector m_vMin;
    Vector m_vMax;
    Vector m_vCenter;
    char* m_pGrassVerts;
    char* m_pGrassCursor;
    int   m_nX48;
    int   m_nVertexBytes;
    List<CAurTriangleListElem> m_elements;
    SafePointer<CAurTexture>   m_pTexture;
    List<int> m_texCounts;
    List<SafePointer<CAurTexture>> m_textureRefs;

    void ReloadColor();

    static float m_fNearRadiusSquared;
    static float m_fFarRadiusSquared;
};



class WindManager;
class PartTriMesh;
class MdlNodeTriMesh;
class MaxFace;

class CAurTriangleList : public List<CAurTriangleListElem> {
public:
    CAurTriangleList();
    CAurTriangleList(int nNumFaces);
    ~CAurTriangleList();
    void AddTriangle(const MaxFace& face, PartTriMesh* pPart, MdlNodeTriMesh* pNode,
                     Vector& vA, Vector& vB);
    void ReloadColor();
    void DestroyGrassPolys();
    void RenderGrassPolys(WindManager* pWind, float fRadius);

    // In the layout behind the list, no constructor writes either member, and only K2's
    // RenderGrassPolys reads m_nNumFaces.
    char _pad10[0x14];
    int  m_nNumFaces;
};
