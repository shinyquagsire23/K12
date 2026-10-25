#include "k2stdint.h"
#include "resBWM.h"

#include "port/libcstring.h"
#if KOTOR1
#include "alignedptr.h"
#endif

extern void ByteSwap(int* p);
extern void ByteSwap(float* p);
extern void ByteSwap(unsigned int* p);

CResBWM::CResBWM()
{
    m_bLoaded = 0;
}

CResBWM::~CResBWM()
{
}

int CResBWM::OnResourceFreed()
{
#if KOTOR1
    m_pParsed = 0;
#endif
    m_bLoaded = 0;
    m_nBWMSize = 0;
    m_pBWMData = 0;
#if !KOTOR1
    m_pParsed = 0;
#endif
    return 1;
}

// Decodes a binary walkmesh in place; every multi-word field is big-endian on disk.
int CResBWM::OnResourceServiced()
{
#if KOTOR1
    void* pFile = m_pFileData;
    if (pFile == 0)
        return 0;
    if (m_bLoaded)
        return 1;

    m_pBWMData = pFile;
    m_nBWMSize = m_nSize;
    m_header = CAlignedPtr<CResBWMHeader>(static_cast<CResBWMHeader*>(pFile)).Get();
    m_pParsed = &m_header;
    m_pParsed = strncmp(reinterpret_cast<char*>(&m_header), "BWM V1.0", 8) == 0 ? &m_header : 0;
    m_bLoaded = 1;
    return 1;
#else
    int* pData = static_cast<int*>(m_pFileData);
    if (pData == 0)
        return 0;
    if (m_bLoaded)
        return 1;

    m_pParsed = reinterpret_cast<CResBWMHeader*>(pData);
    m_nBWMSize = m_nSize;
    m_pBWMData = pData;
    if (strncmp(reinterpret_cast<char*>(pData), "BWM V1.0", 8) != 0) {
        pData = 0;
        m_pParsed = 0;
    }

    ByteSwap(pData + 2);
    ByteSwap(&m_pParsed->vecA.x);
    ByteSwap(&m_pParsed->vecA.y);
    ByteSwap(&m_pParsed->vecA.z);
    ByteSwap(&m_pParsed->vecB.x);
    ByteSwap(&m_pParsed->vecB.y);
    ByteSwap(&m_pParsed->vecB.z);
    ByteSwap(&m_pParsed->vecC.x);
    ByteSwap(&m_pParsed->vecC.y);
    ByteSwap(&m_pParsed->vecC.z);
    ByteSwap(&m_pParsed->vecD.x);
    ByteSwap(&m_pParsed->vecD.y);
    ByteSwap(&m_pParsed->vecD.z);
    ByteSwap(&m_pParsed->localOrigin.x);
    ByteSwap(&m_pParsed->localOrigin.y);
    ByteSwap(&m_pParsed->localOrigin.z);

    ByteSwap(&m_pParsed->nVertexCount);
    ByteSwap(&m_pParsed->offVertices);
    if (m_pParsed != 0)
        pData = reinterpret_cast<int*>(static_cast<char*>(m_pBWMData) + m_pParsed->offVertices);
    else
        pData = 0;
    for (uint64_t i = 0; i < m_pParsed->nVertexCount * 3; i++)
        ByteSwap(&pData[i]);

    ByteSwap(&m_pParsed->nTriangleCount);
    ByteSwap(&m_pParsed->offTriVertIndices);
    ByteSwap(&m_pParsed->offTriMaterials);
    ByteSwap(&m_pParsed->offTriNormals);
    ByteSwap(&m_pParsed->offTriPlaneD);

    if (m_pParsed != 0)
        pData = reinterpret_cast<int*>(static_cast<char*>(m_pBWMData) + m_pParsed->offTriVertIndices);
    else
        pData = 0;
    for (uint64_t i = 0; i < m_pParsed->nTriangleCount * 3; i++)
        ByteSwap(&pData[i]);

    if (m_pParsed != 0)
        pData = reinterpret_cast<int*>(static_cast<char*>(m_pBWMData) + m_pParsed->offTriMaterials);
    else
        pData = 0;
    for (uint64_t i = 0; i < m_pParsed->nTriangleCount; i++)
        ByteSwap(&pData[i]);

    if (m_pParsed != 0)
        pData = reinterpret_cast<int*>(static_cast<char*>(m_pBWMData) + m_pParsed->offTriNormals);
    else
        pData = 0;
    for (uint64_t i = 0; i < m_pParsed->nTriangleCount * 3; i++)
        ByteSwap(&pData[i]);

    if (m_pParsed != 0)
        pData = reinterpret_cast<int*>(static_cast<char*>(m_pBWMData) + m_pParsed->offTriPlaneD);
    else
        pData = 0;
    for (uint64_t i = 0; i < m_pParsed->nTriangleCount; i++)
        ByteSwap(&pData[i]);

    ByteSwap(&m_pParsed->nAABBNodeCount);
    ByteSwap(&m_pParsed->offAABBNodes);
    ByteSwap(&m_pParsed->nRootAABBIndex);

    CResBWMAABBNode* pNode;
    if (m_pParsed != 0)
        pNode = reinterpret_cast<CResBWMAABBNode*>(static_cast<char*>(m_pBWMData) + m_pParsed->offAABBNodes);
    else
        pNode = 0;
    for (uint64_t i = 0; i < m_pParsed->nAABBNodeCount; i++) {
        ByteSwap(&pNode[i].corner0.x);
        ByteSwap(&pNode[i].corner0.y);
        ByteSwap(&pNode[i].corner0.z);
        ByteSwap(&pNode[i].corner1.x);
        ByteSwap(&pNode[i].corner1.y);
        ByteSwap(&pNode[i].corner1.z);
        ByteSwap(&pNode[i].leafTriangleIndex);
        ByteSwap(&pNode[i].m_nReserved1);
        ByteSwap(&pNode[i].sideMask);
        ByteSwap(&pNode[i].child1);
        ByteSwap(&pNode[i].child2);
    }

    ByteSwap(&m_pParsed->nAdjacencyCount);
    ByteSwap(&m_pParsed->offTriAdjacency);
    if (m_pParsed != 0)
        pData = reinterpret_cast<int*>(static_cast<char*>(m_pBWMData) + m_pParsed->offTriAdjacency);
    else
        pData = 0;
    for (uint64_t i = 0; i < m_pParsed->nAdjacencyCount * 3; i++)
        ByteSwap(&pData[i]);

    ByteSwap(&m_pParsed->nEdgeCount);
    ByteSwap(&m_pParsed->offEdgeAdjacency);
    if (m_pParsed != 0)
        pData = reinterpret_cast<int*>(static_cast<char*>(m_pBWMData) + m_pParsed->offEdgeAdjacency);
    else
        pData = 0;
    for (uint64_t i = 0; i < m_pParsed->nEdgeCount * 2; i++)
        ByteSwap(&pData[i]);

    ByteSwap(&m_pParsed->nPerimeterCount);
    ByteSwap(&m_pParsed->offPerimeters);
    if (m_pParsed != 0)
        pData = reinterpret_cast<int*>(static_cast<char*>(m_pBWMData) + m_pParsed->offPerimeters);
    else
        pData = 0;
    for (uint64_t i = 0; i < m_pParsed->nPerimeterCount; i++)
        ByteSwap(&pData[i]);

    m_bLoaded = 1;
    return 1;
#endif
}

void* CResBWM::GetBWMDataPtr()
{
    return m_pBWMData;
}

int CResBWM::GetBWMSize()
{
    return m_nBWMSize;
}

int CResBWM::IsBinary()
{
    return strncmp(static_cast<const char*>(m_pBWMData), "BWM V1.0", 8) == 0;
}
