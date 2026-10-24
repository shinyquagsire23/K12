#include "k2stdint.h"
#include "nwarea.h"
#include "gameobjectarray.h"

const float CSWArea::EPSILON = 9.99999975e-05f;
const float CSWArea::SPLINE_COSINE_LIMIT = 0.899999976f;

#include "port/libcstring.h"
#include "port/libcmath.h"

CSWArea::CSWArea()
{
    m_nHeight = 0;
    m_nAreaFlags = 0;
    m_nWidth = 0;

    m_fogColor.x = 0.0f;
    m_fogColor.y = 0.0f;
    m_fogColor.z = 0.0f;

    m_sunShadows = 0;

    m_moonAmbientColor = 0;
    m_moonDiffuseColor = 0;
    m_moonFogColor = 0;
    m_moonFogNear = 0.0f;
    m_moonFogFar = 0.0f;
    m_moonShadows = 0;
    m_sunAmbientColor = 0;
    m_sunDiffuseColor = 0;
    m_nFogColor = 0;
    m_sunFogNear = 0.0f;
    m_sunFogFar = 0.0f;

    m_dayNightCycle = 1;

    m_isNight = 0;
    m_chanceRain = 0;
    m_chanceSnow = 0;
    m_chanceLightning = 0;
    m_windPower = 0;
    m_dynAmbientColor = 0;
    m_noRest = 0;

    m_shadowOpacity = 50;

    m_pStaticObjectIds = 0;
    m_pStaticObjectFirstTriangles = 0;
    m_pStaticObjectTriangleCounts = 0;
    m_pStaticObjectFirstVertices = 0;
    m_pStaticObjectVertexCounts = 0;
    m_pStaticTriangleIndices = 0;
    m_pStaticVertices = 0;
    m_pStaticBBoxes = 0;
    m_pStaticBBoxIds = 0;

    m_cameraStyle = 0;
}

// Bounding-box guards for the segment intersection. Both reject a NaN coordinate.
static inline bool InRange(float v, float lo, float hi)
{
    return hi >= lo && v >= lo && v <= hi;
}

static inline bool OutOf(float v, float lo, float hi)
{
    return hi >= lo && !(v >= lo && v <= hi);
}

// Intersects two 2D segments, handling the axis-aligned degeneracies separately, and reports
// whether the intersection lies inside both segments. The outputs receive its x and y.
int CSWArea::IntersectLineSegments(float ax, float ay, float bx, float by,
                                   float cx, float cy, float dx, float dy,
                                   float* pOutT1, float* pOutT2)
{
    float d1x = bx - ax;
    float d1y = by - ay;
    float d2x = dx - cx;
    float d2y = dy - cy;

    if ((d1x * d2x + d1y * d2y) / sqrtf(d1x * d1x + d1y * d1y) /
            sqrtf(d2x * d2x + d2y * d2y) == 1.0f)
        return 0;

    if (ax == bx) {
        float m2 = d2y / d2x;
        *pOutT1 = ax;
        *pOutT2 = m2 * ax + (cy - m2 * cx);

        if (!InRange(*pOutT2, cy, dy) && !InRange(*pOutT2, dy, cy))
            return 0;
        if (OutOf(*pOutT1, cx, dx) || OutOf(*pOutT1, dx, cx))
            return 0;
        return !OutOf(*pOutT2, ay, by) && !OutOf(*pOutT2, by, ay);
    }

    float m1 = d1y / d1x;
    if (cx == dx) {
        *pOutT1 = cx;
        *pOutT2 = m1 * cx + (ay - m1 * ax);

        if (!InRange(*pOutT2, ay, by) && !InRange(*pOutT2, by, ay))
            return 0;
        if (OutOf(*pOutT1, ax, bx) || OutOf(*pOutT1, bx, ax))
            return 0;
        return !OutOf(*pOutT2, cy, dy) && !OutOf(*pOutT2, dy, cy);
    }

    float m2 = d2y / d2x;
    if (m2 - m1 == 0.0f)
        return 0;

    float b1 = ay - m1 * ax;
    float b2 = cy - m2 * cx;
    *pOutT1 = (1.0f / (m1 - m2)) * (b2 - b1);
    *pOutT2 = b2 + m2 * *pOutT1;

    if (!InRange(*pOutT1, cx, dx) && !InRange(*pOutT1, dx, cx))
        return 0;
    return !OutOf(*pOutT1, ax, bx) && !OutOf(*pOutT1, bx, ax);
}

int CSWArea::GetIsNight()
{
    return m_isNight;
}

int CSWArea::SetIsNight(int bIsNight)
{
    m_isNight = bIsNight;
    return 1;
}

int CSWArea::SetWind(unsigned char wind)
{
    m_windPower = wind;
    return 1;
}

int CSWArea::SetFog(float fogNear, float fogFar, Vector color)
{
    m_fogColor = color;
    m_fogNear = fogNear;
    m_fogFar = fogFar;
    return 1;
}

int CSWArea::GetIsIDInExcludeList(unsigned long long, CExoArrayList<CGameObject*>*)
{
    return 0;
}

// Registers an object id with its slice of the vertex and triangle pools and returns the new
// entry's index, which is the handle removal takes.
int CSWArea::AddStaticObject(unsigned long long objectId, const Vector& vMin, const Vector& vMax,
                             int nVertexCount, Vector* pVertices, int nTriangleCount,
                             int* pTriangles)
{
    if (m_nStaticObjectCapacity == m_nStaticObjectCount)
        GrowStaticObjectArray();

    int nFirstVertex = 0;
    int nFirstTriangle = 0;
    if (m_nStaticObjectCount != 0) {
        nFirstVertex = m_nStaticVertexCount;
        nFirstTriangle = m_nStaticTriangleCount;
    }

    if (nFirstVertex + nVertexCount >= m_nStaticVertexCapacity)
        GrowStaticObjectVertices(nVertexCount);

    if (3 * (nFirstTriangle + nTriangleCount) >= m_nStaticTriangleIndexCapacity)
        GrowStaticObjectTriangles(nTriangleCount);

    m_pStaticObjectFirstVertices[m_nStaticObjectCount] = nFirstVertex;
    m_pStaticObjectVertexCounts[m_nStaticObjectCount] = nVertexCount;
    m_pStaticObjectFirstTriangles[m_nStaticObjectCount] = nFirstTriangle;
    m_pStaticObjectTriangleCounts[m_nStaticObjectCount] = nTriangleCount;
    m_pStaticObjectIds[m_nStaticObjectCount] = objectId;
    m_nStaticObjectCount = m_nStaticObjectCount + 1;

    // Each vertex is transformed straight into its pool slot.
    for (int i = 0; i < nVertexCount; i++) {
        Vector& v = m_pStaticVertices[nFirstVertex + i];
        v.x = pVertices[i].x * vMin.y + pVertices[i].y * vMin.x;
        v.y = vMin.y * pVertices[i].y - pVertices[i].x * vMin.x;
        v.z = pVertices[i].z;
        v.x = v.x + vMax.x;
        v.y = v.y + vMax.y;
        v.z = v.z + vMax.z;
    }

    for (int i = 0; i < 3 * nTriangleCount; i++)
        m_pStaticTriangleIndices[3 * nFirstTriangle + i] = pTriangles[i] + nFirstVertex;

    m_nStaticTriangleCount = m_nStaticTriangleCount + nTriangleCount;
    m_nStaticVertexCount = m_nStaticVertexCount + nVertexCount;

    return m_nStaticObjectCount - 1;
}

// Adds an axis-aligned bounding box and returns the registry's entry count.
int CSWArea::AddStaticBoundingBox(unsigned long long objectId, const Vector& vMin,
                                  const Vector& vMax)
{
    if (m_nStaticBBoxCount == m_nStaticBBoxCapacity) {
        int nNewCapacity = m_nStaticBBoxCount * 2;
        if (nNewCapacity < 8)
            nNewCapacity = 8;

        Vector* pNewBoxes = new Vector[nNewCapacity * 2]();
        unsigned long long* pNewIds = new unsigned long long[nNewCapacity];

        if (m_pStaticBBoxes) {
            memcpy(pNewBoxes, m_pStaticBBoxes, m_nStaticBBoxCount * 2 * sizeof(Vector));
            delete[] m_pStaticBBoxes;
        }
        if (m_pStaticBBoxIds) {
            memcpy(pNewIds, m_pStaticBBoxIds, m_nStaticBBoxCount * sizeof(unsigned long long));
            delete[] m_pStaticBBoxIds;
        }

        m_nStaticBBoxCapacity = nNewCapacity;
        m_pStaticBBoxes = pNewBoxes;
        m_pStaticBBoxIds = pNewIds;
    }

    m_pStaticBBoxes[m_nStaticBBoxCount * 2] = vMin;
    m_pStaticBBoxes[m_nStaticBBoxCount * 2 + 1] = vMax;
    m_pStaticBBoxIds[m_nStaticBBoxCount] = objectId;
    m_nStaticBBoxCount = m_nStaticBBoxCount + 1;

    return m_nStaticBBoxCount;
}

// Removes the box with this id; returns 1 when it was found, 0 otherwise.
int CSWArea::RemoveStaticBoundingBox(unsigned long long objectId)
{
    if (m_nStaticBBoxCount < 1)
        return 0;

    int i = 0;
    while (i < m_nStaticBBoxCount && m_pStaticBBoxIds[i] != objectId)
        i++;

    if (i >= m_nStaticBBoxCount)
        return 0;

    m_nStaticBBoxCount = m_nStaticBBoxCount - 1;

    for (int j = i; j < m_nStaticBBoxCount; j++) {
        m_pStaticBBoxIds[j] = m_pStaticBBoxIds[j + 1];
        m_pStaticBBoxes[j * 2] = m_pStaticBBoxes[(j + 1) * 2];
        m_pStaticBBoxes[j * 2 + 1] = m_pStaticBBoxes[(j + 1) * 2 + 1];
    }

    return 1;
}

// Removes a static object, the inverse of AddStaticObject. Invalidates the entry, then compacts
// both pools and rebases the per-entry offsets and triangle indices, so indexes already held stay
// valid.
void CSWArea::RemoveStaticObject(int nIndex)
{
    if (nIndex == -1)
        return;
    if (m_nStaticObjectCount <= nIndex)
        return;

    int nFirstVertex = m_pStaticObjectFirstVertices[nIndex];
    int nVertexCount = m_pStaticObjectVertexCounts[nIndex];
    int nFirstTriangle = m_pStaticObjectFirstTriangles[nIndex];
    int nTriangleCount = m_pStaticObjectTriangleCounts[nIndex];

    m_pStaticObjectFirstVertices[nIndex] = 0;
    m_pStaticObjectVertexCounts[nIndex] = 0;
    m_pStaticObjectFirstTriangles[nIndex] = 0;
    m_pStaticObjectTriangleCounts[nIndex] = 0;
    m_pStaticObjectIds[nIndex] = OBJECT_INVALID;

    for (int i = nFirstVertex + nVertexCount; i < m_nStaticVertexCount; i++)
        m_pStaticVertices[i - nVertexCount] = m_pStaticVertices[i];

    for (int i = 3 * (nFirstTriangle + nTriangleCount); i < 3 * m_nStaticTriangleCount; i++)
        m_pStaticTriangleIndices[i - 3 * nTriangleCount] = m_pStaticTriangleIndices[i] - nVertexCount;

    m_nStaticTriangleCount = m_nStaticTriangleCount - nTriangleCount;
    m_nStaticVertexCount = m_nStaticVertexCount - nVertexCount;

    for (int i = nIndex + 1; i < m_nStaticObjectCount; i++) {
        m_pStaticObjectFirstVertices[i] = m_pStaticObjectFirstVertices[i] - nVertexCount;
        m_pStaticObjectFirstTriangles[i] = m_pStaticObjectFirstTriangles[i] - nTriangleCount;
    }
}

CSWArea::~CSWArea()
{
    delete[] m_pStaticObjectFirstTriangles;
    delete[] m_pStaticObjectTriangleCounts;
    delete[] m_pStaticObjectFirstVertices;
    delete[] m_pStaticObjectVertexCounts;
    delete[] m_pStaticObjectIds;
    delete[] m_pStaticTriangleIndices;
    delete[] m_pStaticVertices;
    delete[] m_pStaticBBoxes;
    delete[] m_pStaticBBoxIds;
}

// Initialises the whole static-object registry; the area constructor calls this at the end.
void CSWArea::InitializeStaticObjects()
{
    m_nStaticObjectCapacity = 0;
    m_nStaticObjectCount = 0;
    m_pStaticObjectIds = 0;
    m_pStaticObjectFirstTriangles = 0;
    m_pStaticObjectTriangleCounts = 0;
    m_pStaticObjectFirstVertices = 0;
    m_pStaticObjectVertexCounts = 0;
    m_nStaticTriangleIndexCapacity = 0;
    m_nStaticTriangleCount = 0;
    m_pStaticTriangleIndices = 0;
    m_nStaticVertexCapacity = 0;
    m_nStaticVertexCount = 0;
    m_pStaticVertices = 0;
    m_nStaticBBoxCapacity = 0;
    m_nStaticBBoxCount = 0;
    m_pStaticBBoxes = 0;
    m_pStaticBBoxIds = 0;
}

// Grows the five parallel arrays indexed by static-object slot.
void CSWArea::GrowStaticObjectArray()
{
    int nNewCapacity = (m_nStaticObjectCapacity == 0) ? 16 : m_nStaticObjectCapacity * 2;
    m_nStaticObjectCapacity = nNewCapacity;

    int* pNewFirstTriangles = new int[nNewCapacity];
    int* pNewTriangleCounts = new int[nNewCapacity];
    int* pNewFirstVertices = new int[nNewCapacity];
    int* pNewVertexCounts = new int[nNewCapacity];
    unsigned long long* pNewIds = new unsigned long long[nNewCapacity];

    K2_ASSUME(m_nStaticObjectCount < nNewCapacity);   // called only when full: count == old capacity
    for (int i = 0; i < m_nStaticObjectCount; i++) {
        pNewFirstTriangles[i] = m_pStaticObjectFirstTriangles[i];
        pNewTriangleCounts[i] = m_pStaticObjectTriangleCounts[i];
        pNewFirstVertices[i] = m_pStaticObjectFirstVertices[i];
        pNewVertexCounts[i] = m_pStaticObjectVertexCounts[i];
        pNewIds[i] = m_pStaticObjectIds[i];
    }

    delete[] m_pStaticObjectFirstTriangles;
    delete[] m_pStaticObjectTriangleCounts;
    delete[] m_pStaticObjectFirstVertices;
    delete[] m_pStaticObjectVertexCounts;
    delete[] m_pStaticObjectIds;

    m_pStaticObjectFirstTriangles = pNewFirstTriangles;
    m_pStaticObjectTriangleCounts = pNewTriangleCounts;
    m_pStaticObjectFirstVertices = pNewFirstVertices;
    m_pStaticObjectVertexCounts = pNewVertexCounts;
    m_pStaticObjectIds = pNewIds;
}

// Grows the vertex pool.
void CSWArea::GrowStaticObjectVertices(int nAdditional)
{
    int nNewCapacity = (m_nStaticVertexCapacity == 0) ? 16 : m_nStaticVertexCapacity;
    while (nNewCapacity <= m_nStaticVertexCapacity + nAdditional)
        nNewCapacity *= 2;

    Vector* pNewVertices = new Vector[nNewCapacity]();
    K2_ASSUME(m_nStaticVertexCount < nNewCapacity);   // the count never passes the old capacity
    for (int i = 0; i < m_nStaticVertexCount; i++)
        pNewVertices[i] = m_pStaticVertices[i];
    delete[] m_pStaticVertices;

    m_nStaticVertexCapacity = nNewCapacity;
    m_pStaticVertices = pNewVertices;
}

// Grows the flat triangle pool, which holds three indices per triangle.
void CSWArea::GrowStaticObjectTriangles(int nAdditional)
{
    int nOldCount = m_nStaticTriangleCount;
    int nNewCapacity = (m_nStaticTriangleIndexCapacity == 0) ? 48 : m_nStaticTriangleIndexCapacity;
    while (nNewCapacity <= 3 * (nOldCount + nAdditional))
        nNewCapacity *= 2;

    int* pNewIndices = new int[nNewCapacity];
    int nOldIndices = 3 * nOldCount;
    K2_ASSUME(nOldIndices < nNewCapacity);   // the loop above grew it past 3 * (old + new)
    for (int i = 0; i < nOldIndices; i++)
        pNewIndices[i] = m_pStaticTriangleIndices[i];
    delete[] m_pStaticTriangleIndices;

    m_nStaticTriangleIndexCapacity = nNewCapacity;
    m_pStaticTriangleIndices = pNewIndices;
}
