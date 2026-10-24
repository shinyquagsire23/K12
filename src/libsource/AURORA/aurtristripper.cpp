// The Aurora triangle-strip builder, the third-party Nvidia NvTriStrip library (NvStripifier,
// NvStripInfo) plus the engine-side wrapper AurStripify.
//
// Changes to the library: a MyFace vector threaded through Stripify and FindAllStrips, the
// numsamples and numedges tunables, the non-manifold latch, a fixed face-split threshold, and the
// facestodelete list the wrapper cleans up.
#include "k2stdint.h"
#include "List.h"
#include "aurvertexcache.h"
#include <vector>
#include <list>
#include <set>

#include "port/libcbase.h"

class NvStripInfo;

// NvTriStrip's MyFace holds three vertex indices and a face normal. AurStripify fills only the
// indices.
struct MyFace {
    int m_nIndex[3];
    float m_fNormal[3];
};

// NvTriStrip's MyVertex holds a position and a normal.
struct MyVertex {
    float x, y, z;
    float nx, ny, nz;
};

class NvFaceInfo {
public:
    NvFaceInfo(int v0, int v1, int v2)
    {
        m_v0 = v0; m_v1 = v1; m_v2 = v2;
        m_stripId = -1;
        m_testStripId = -1;
        m_experimentId = -1;
    }

    int m_v0, m_v1, m_v2;
    int m_stripId;
    int m_testStripId;
    int m_experimentId;
};

// An edge shared by up to two faces; the two `m_next` links thread it into both vertices' lists.
class NvEdgeInfo {
public:
    NvEdgeInfo(int v0, int v1)
    {
        m_v0 = v0;
        m_v1 = v1;
        m_face0 = NULL;
        m_face1 = NULL;
        m_nextV0 = NULL;
        m_nextV1 = NULL;
        m_refCount = 2;
    }

    void Unref()
    {
        if (--m_refCount == 0)
            delete this;
    }

    unsigned int m_refCount;
    NvFaceInfo* m_face0;
    NvFaceInfo* m_face1;
    int m_v0, m_v1;
    NvEdgeInfo* m_nextV0;
    NvEdgeInfo* m_nextV1;
};

typedef std::vector<NvFaceInfo*> NvFaceInfoVec;
typedef std::vector<NvEdgeInfo*> NvEdgeInfoVec;
typedef std::vector<NvStripInfo*> NvStripInfoVec;
typedef std::vector<unsigned short> WordVec;
typedef std::vector<MyVertex> MyVertexVec;
typedef std::vector<MyFace> MyFaceVec;

class NvStripStartInfo {
public:
    NvStripStartInfo(NvFaceInfo* startFace, NvEdgeInfo* startEdge, bool toV1)
    {
        m_startFace = startFace;
        m_startEdge = startEdge;
        m_toV1 = toV1;
    }

    NvFaceInfo* m_startFace;
    NvEdgeInfo* m_startEdge;
    bool m_toV1;
};

class NvStripInfo {
public:
    NvStripInfo(const NvStripStartInfo& startInfo, int stripId, int experimentId = -1)
        : m_startInfo(startInfo)
    {
        m_stripId = stripId;
        m_experimentId = experimentId;
        visited = false;
    }

    bool IsExperiment() const { return m_experimentId >= 0; }
    bool IsInStrip(const NvFaceInfo* faceInfo) const
    {
        if (faceInfo == NULL)
            return false;
        return (m_experimentId >= 0 ? faceInfo->m_testStripId == m_stripId
                                    : faceInfo->m_stripId == m_stripId);
    }
    bool SharesEdge(const NvFaceInfo* faceInfo, NvEdgeInfoVec& edgeInfos);
    void Combine(const NvFaceInfoVec& forward, const NvFaceInfoVec& backward);
    bool Unique(NvFaceInfoVec& faceVec, NvFaceInfo* face);
    bool IsMarked(NvFaceInfo* faceInfo)
    {
        return (faceInfo->m_stripId >= 0) || (IsExperiment() && faceInfo->m_experimentId == m_experimentId);
    }
    void MarkTriangle(NvFaceInfo* faceInfo)
    {
        if (IsExperiment()) {
            faceInfo->m_experimentId = m_experimentId;
            faceInfo->m_testStripId = m_stripId;
        } else {
            faceInfo->m_experimentId = -1;
            faceInfo->m_stripId = m_stripId;
        }
    }
    void Build(NvEdgeInfoVec& edgeInfos, NvFaceInfoVec& faceInfos);

    NvStripStartInfo m_startInfo;
    NvFaceInfoVec m_faces;
    int m_stripId;
    int m_experimentId;
    bool visited;
};

// The incoming index list and the four tuning scalars Stripify sets.
class NvStripifier {
public:
    NvStripifier();
    ~NvStripifier();

    void Stripify(const WordVec& in_indices, int in_cacheSize, int in_minStripLength,
                  MyFaceVec& faces, NvStripInfoVec& outStrips, NvFaceInfoVec& outFaceList);
    void CreateStrips(NvStripInfoVec& allStrips, WordVec& stripIndices, WordVec& stripLengths);
    void OptimizeVertices(MyVertexVec& vertices, MyVertexVec& optimizedVerts,
                          NvStripInfoVec& strips, NvFaceInfoVec& leftoverFaces);

    static int GetUniqueVertexInB(NvFaceInfo* faceA, NvFaceInfo* faceB);
    static int GetSharedVertex(NvFaceInfo* faceA, NvFaceInfo* faceB);

protected:
    WordVec indices;
    int cacheSize;
    int minStripLength;
    float meshJump;
    bool bFirstTimeResetPoint;

    bool IsCW(NvFaceInfo* faceInfo, int v0, int v1);
    bool NextIsCCW(int numIndices);
    static int GetNextIndex(const WordVec& indices, NvFaceInfo* face);
    static NvEdgeInfo* FindEdgeInfo(NvEdgeInfoVec& edgeInfos, int v0, int v1);
    static NvFaceInfo* FindOtherFace(NvEdgeInfoVec& edgeInfos, int v0, int v1, NvFaceInfo* faceInfo);
    NvFaceInfo* FindGoodResetPoint(NvFaceInfoVec& faceInfos, NvEdgeInfoVec& edgeInfos);
    void FindAllStrips(NvStripInfoVec& allStrips, NvFaceInfoVec& allFaceInfos,
                       NvEdgeInfoVec& allEdgeInfos, int numSamples, MyFaceVec& faces);
    void SplitUpStripsAndOptimize(NvStripInfoVec& allStrips, NvStripInfoVec& outStrips,
                                  NvEdgeInfoVec& edgeInfos, NvFaceInfoVec& outFaceList);
    void RemoveSmallStrips(NvStripInfoVec& allStrips, NvStripInfoVec& allBigStrips,
                           NvFaceInfoVec& faceList);
    bool FindTraversal(NvFaceInfoVec& faceInfos, NvEdgeInfoVec& edgeInfos, NvStripInfo* strip,
                       NvStripStartInfo& startInfo);
    int CountRemainingTris(std::list<NvStripInfo*>::iterator iter,
                           std::list<NvStripInfo*>::iterator end);
    void CommitStrips(NvStripInfoVec& allStrips, const NvStripInfoVec& strips, MyFaceVec& faces);
    float AvgStripSize(const NvStripInfoVec& strips);
    int FindStartPoint(NvFaceInfoVec& faceInfos, NvEdgeInfoVec& edgeInfos);
    void UpdateCacheStrip(VertexCache* vcache, NvStripInfo* strip);
    void UpdateCacheFace(VertexCache* vcache, NvFaceInfo* face);
    float CalcNumHitsStrip(VertexCache* vcache, NvStripInfo* strip);
    int CalcNumHitsFace(VertexCache* vcache, NvFaceInfo* face);
    int NumNeighbors(NvFaceInfo* face, NvEdgeInfoVec& edgeInfoVec);
    void BuildStripifyInfo(NvFaceInfoVec& faceInfos, NvEdgeInfoVec& edgeInfos);
    bool AlreadyExists(NvFaceInfo* faceInfo, NvFaceInfoVec& faceInfos);

    friend class NvStripInfo;
};

bool bNonManifoldFlag;
int numsamples = 10;
int numedges = 6;

List<NvFaceInfo*> facestodelete;

NvStripifier::NvStripifier()
{
}

NvStripifier::~NvStripifier()
{
}

NvEdgeInfo* NvStripifier::FindEdgeInfo(NvEdgeInfoVec& edgeInfos, int v0, int v1)
{
    // we can get to it through either array because the edge infos have a v0 and v1
    // and there is no order except how it was first created.
    NvEdgeInfo* infoIter = edgeInfos[v0];
    while (infoIter != NULL) {
        if (infoIter->m_v0 == v0) {
            if (infoIter->m_v1 == v1)
                return infoIter;
            else
                infoIter = infoIter->m_nextV0;
        } else {
            if (infoIter->m_v0 == v1)
                return infoIter;
            else
                infoIter = infoIter->m_nextV1;
        }
    }
    return NULL;
}

NvFaceInfo* NvStripifier::FindOtherFace(NvEdgeInfoVec& edgeInfos, int v0, int v1, NvFaceInfo* faceInfo)
{
    NvEdgeInfo* edgeInfo = FindEdgeInfo(edgeInfos, v0, v1);
    return (edgeInfo->m_face0 == faceInfo ? edgeInfo->m_face1 : edgeInfo->m_face0);
}

bool NvStripifier::AlreadyExists(NvFaceInfo* faceInfo, NvFaceInfoVec& faceInfos)
{
    for (unsigned int i = 0; i < faceInfos.size(); ++i) {
        if ((faceInfos[i]->m_v0 == faceInfo->m_v0) &&
            (faceInfos[i]->m_v1 == faceInfo->m_v1) &&
            (faceInfos[i]->m_v2 == faceInfo->m_v2))
            return true;
    }
    return false;
}

void NvStripifier::BuildStripifyInfo(NvFaceInfoVec& faceInfos, NvEdgeInfoVec& edgeInfos)
{
    // reserve space for the face infos, but do not resize them.
    int numIndices = static_cast<int>(indices.size());
    faceInfos.reserve(numIndices);

    // we actually resize the edge infos, so we must initialize to NULL
    edgeInfos.resize(numIndices);
    for (int i = 0; i < numIndices; i++)
        edgeInfos[i] = NULL;

    // iterate through the triangles of the triangle list
    int numTriangles = numIndices / 3;
    int index = 0;
    for (int i = 0; i < numTriangles; i++) {
        // grab the indices
        int v0 = indices[index++];
        int v1 = indices[index++];
        int v2 = indices[index++];

        // create the face info and add it to the list of faces, but only if this exact face
        // doesn't already exist in the list
        NvFaceInfo* faceInfo = new NvFaceInfo(v0, v1, v2);
        if (!AlreadyExists(faceInfo, faceInfos)) {
            faceInfos.push_back(faceInfo);

            // grab the edge infos, creating them if they do not already exist
            NvEdgeInfo* edgeInfo01 = FindEdgeInfo(edgeInfos, v0, v1);
            if (edgeInfo01 == NULL) {
                // create the info
                edgeInfo01 = new NvEdgeInfo(v0, v1);

                // update the linked list on both
                edgeInfo01->m_nextV0 = edgeInfos[v0];
                edgeInfo01->m_nextV1 = edgeInfos[v1];
                edgeInfos[v0] = edgeInfo01;
                edgeInfos[v1] = edgeInfo01;

                // set face 0
                edgeInfo01->m_face0 = faceInfo;
            } else {
                if (edgeInfo01->m_face1 != NULL) {
                    printf("BuildStripifyInfo: > 2 triangles on an edge... uncertain consequences\n");
                    bNonManifoldFlag = true;
                } else
                    edgeInfo01->m_face1 = faceInfo;
            }

            // grab the edge infos, creating them if they do not already exist
            NvEdgeInfo* edgeInfo12 = FindEdgeInfo(edgeInfos, v1, v2);
            if (edgeInfo12 == NULL) {
                // create the info
                edgeInfo12 = new NvEdgeInfo(v1, v2);

                // update the linked list on both
                edgeInfo12->m_nextV0 = edgeInfos[v1];
                edgeInfo12->m_nextV1 = edgeInfos[v2];
                edgeInfos[v1] = edgeInfo12;
                edgeInfos[v2] = edgeInfo12;

                // set face 0
                edgeInfo12->m_face0 = faceInfo;
            } else {
                if (edgeInfo12->m_face1 != NULL)
                    printf("BuildStripifyInfo: > 2 triangles on an edge... uncertain consequences\n");
                else
                    edgeInfo12->m_face1 = faceInfo;
            }

            // grab the edge infos, creating them if they do not already exist
            NvEdgeInfo* edgeInfo20 = FindEdgeInfo(edgeInfos, v2, v0);
            if (edgeInfo20 == NULL) {
                // create the info
                edgeInfo20 = new NvEdgeInfo(v2, v0);

                // update the linked list on both
                edgeInfo20->m_nextV0 = edgeInfos[v2];
                edgeInfo20->m_nextV1 = edgeInfos[v0];
                edgeInfos[v2] = edgeInfo20;
                edgeInfos[v0] = edgeInfo20;

                // set face 0
                edgeInfo20->m_face0 = faceInfo;
            } else {
                if (edgeInfo20->m_face1 != NULL)
                    printf("BuildStripifyInfo: > 2 triangles on an edge... uncertain consequences\n");
                else
                    edgeInfo20->m_face1 = faceInfo;
            }
        }
    }
}

//
// Finds a good starting point, namely one which has only one neighbor
int NvStripifier::FindStartPoint(NvFaceInfoVec& faceInfos, NvEdgeInfoVec& edgeInfos)
{
    for (unsigned int i = 0; i < faceInfos.size(); i++) {
        int ctr = 0;

        if (FindOtherFace(edgeInfos, faceInfos[i]->m_v0, faceInfos[i]->m_v1, faceInfos[i]) == NULL)
            ctr++;
        if (FindOtherFace(edgeInfos, faceInfos[i]->m_v1, faceInfos[i]->m_v2, faceInfos[i]) == NULL)
            ctr++;
        if (FindOtherFace(edgeInfos, faceInfos[i]->m_v2, faceInfos[i]->m_v0, faceInfos[i]) == NULL)
            ctr++;
        if (ctr > 1)
            return i;
    }
    return -1;
}

//
// A good reset point is one near other commited areas so that we know that when we've made the
// longest strips its because we're stripifying in the same general orientation.
NvFaceInfo* NvStripifier::FindGoodResetPoint(NvFaceInfoVec& faceInfos, NvEdgeInfoVec& edgeInfos)
{
    // we hop into different areas of the mesh to try to get other large open spans done.  Areas of
    // small strips can just be left to triangle lists added at the end.
    NvFaceInfo* result = NULL;

    int numFaces = static_cast<int>(faceInfos.size());
    int startPoint;
    if (bFirstTimeResetPoint) {
        // first time, find a face with few neighbors (look for an edge of the mesh)
        startPoint = FindStartPoint(faceInfos, edgeInfos);
        bFirstTimeResetPoint = false;
    } else
        startPoint = static_cast<int>((static_cast<float>(numFaces) - 1) * meshJump);

    if (startPoint == -1)
        startPoint = static_cast<int>((static_cast<float>(numFaces) - 1) * meshJump);

    int i = startPoint;
    do {
        // if this guy isn't visited, try him
        if (faceInfos[i]->m_stripId < 0) {
            result = faceInfos[i];
            break;
        }

        // update the index and clamp to 0-(numFaces-1)
        if (++i >= numFaces)
            i = 0;
    } while (i != startPoint);

    // update the meshJump
    meshJump += 0.1f;
    if (meshJump > 1.0f)
        meshJump = .05f;

    // return the best face we found
    return result;
}

//
// Returns the vertex unique to faceB
int NvStripifier::GetUniqueVertexInB(NvFaceInfo* faceA, NvFaceInfo* faceB)
{
    int facev0 = faceB->m_v0;
    if (facev0 != faceA->m_v0 && facev0 != faceA->m_v1 && facev0 != faceA->m_v2)
        return facev0;

    int facev1 = faceB->m_v1;
    if (facev1 != faceA->m_v0 && facev1 != faceA->m_v1 && facev1 != faceA->m_v2)
        return facev1;

    int facev2 = faceB->m_v2;
    if (facev2 != faceA->m_v0 && facev2 != faceA->m_v1 && facev2 != faceA->m_v2)
        return facev2;

    // nothing is different
    return -1;
}

//
// Returns the (at most) one vertex shared between faceA and faceB
int NvStripifier::GetSharedVertex(NvFaceInfo* faceA, NvFaceInfo* faceB)
{
    int facev0 = faceB->m_v0;
    if (facev0 == faceA->m_v0 || facev0 == faceA->m_v1 || facev0 == faceA->m_v2)
        return facev0;

    int facev1 = faceB->m_v1;
    if (facev1 == faceA->m_v0 || facev1 == faceA->m_v1 || facev1 == faceA->m_v2)
        return facev1;

    int facev2 = faceB->m_v2;
    if (facev2 == faceA->m_v0 || facev2 == faceA->m_v1 || facev2 == faceA->m_v2)
        return facev2;

    // nothing is shared
    return -1;
}

void NvStripifier::CommitStrips(NvStripInfoVec& allStrips, const NvStripInfoVec& strips, MyFaceVec& faces)
{
    K2_UNUSED(faces);
    // Iterate through strips
    int numStrips = static_cast<int>(strips.size());
    for (int i = 0; i < numStrips; i++) {
        // Tell the strip that it is now real
        NvStripInfo* strip = strips[i];
        strip->m_experimentId = -1;

        // add to the list of real strips
        allStrips.push_back(strip);

        // Iterate through the faces of the strip
        // Tell the faces of the strip that they belong to a real strip now
        const NvFaceInfoVec& stripFaces = strips[i]->m_faces;
        int numFaces = static_cast<int>(stripFaces.size());
        if (stripFaces[0]->m_v0 == 2302 && stripFaces[0]->m_v1 == 3215 && stripFaces[0]->m_v2 == 2603)
            printf("BLEH");

        for (int j = 0; j < numFaces; j++)
            strip->MarkTriangle(stripFaces[j]);
    }
}

bool NvStripifier::FindTraversal(NvFaceInfoVec& faceInfos, NvEdgeInfoVec& edgeInfos,
                                 NvStripInfo* strip, NvStripStartInfo& startInfo)
{
    K2_UNUSED(faceInfos);
    // if the strip was v0->v1 on the edge, then v1 will be a vertex in the next edge.
    int v = (strip->m_startInfo.m_toV1 ? strip->m_startInfo.m_startEdge->m_v1
                                       : strip->m_startInfo.m_startEdge->m_v0);

    NvFaceInfo* untouchedFace = NULL;
    NvEdgeInfo* edgeIter = edgeInfos[v];
    while (edgeIter != NULL) {
        NvFaceInfo* face0 = edgeIter->m_face0;
        NvFaceInfo* face1 = edgeIter->m_face1;
        if ((face0 != NULL && !strip->IsInStrip(face0)) && face1 != NULL && !strip->IsMarked(face1)) {
            untouchedFace = face1;
            break;
        }
        if ((face1 != NULL && !strip->IsInStrip(face1)) && face0 != NULL && !strip->IsMarked(face0)) {
            untouchedFace = face0;
            break;
        }

        // find the next edgeIter
        edgeIter = (edgeIter->m_v0 == v ? edgeIter->m_nextV0 : edgeIter->m_nextV1);
    }

    startInfo.m_startFace = untouchedFace;
    startInfo.m_startEdge = edgeIter;
    if (edgeIter != NULL) {
        if (strip->SharesEdge(startInfo.m_startFace, edgeInfos))
            startInfo.m_toV1 = (edgeIter->m_v0 == v);  // note! used to be m_v1
        else
            startInfo.m_toV1 = (edgeIter->m_v1 == v);
    }
    return (startInfo.m_startFace != NULL);
}

//
// Returns true if the input face and the current strip share an edge
bool NvStripInfo::SharesEdge(const NvFaceInfo* faceInfo, NvEdgeInfoVec& edgeInfos)
{
    // check v0->v1 edge
    NvEdgeInfo* currEdge = NvStripifier::FindEdgeInfo(edgeInfos, faceInfo->m_v0, faceInfo->m_v1);
    if (IsInStrip(currEdge->m_face0) || IsInStrip(currEdge->m_face1))
        return true;

    // check v1->v2 edge
    currEdge = NvStripifier::FindEdgeInfo(edgeInfos, faceInfo->m_v1, faceInfo->m_v2);
    if (IsInStrip(currEdge->m_face0) || IsInStrip(currEdge->m_face1))
        return true;

    // check v2->v0 edge
    currEdge = NvStripifier::FindEdgeInfo(edgeInfos, faceInfo->m_v2, faceInfo->m_v0);
    if (IsInStrip(currEdge->m_face0) || IsInStrip(currEdge->m_face1))
        return true;

    return false;
}

void NvStripifier::RemoveSmallStrips(NvStripInfoVec& allStrips, NvStripInfoVec& allBigStrips,
                                     NvFaceInfoVec& faceList)
{
    faceList.clear();
    allBigStrips.clear();  // make sure these are empty
    NvFaceInfoVec tempFaceList;

    for (unsigned int i = 0; i < allStrips.size(); i++) {
        if (static_cast<int>(allStrips[i]->m_faces.size()) < minStripLength) {
            // strip is too small, add faces to faceList
            for (unsigned int j = 0; j < allStrips[i]->m_faces.size(); j++)
                tempFaceList.push_back(allStrips[i]->m_faces[j]);

            // and free memory
            delete allStrips[i];
        } else {
            allBigStrips.push_back(allStrips[i]);
        }
    }

    bool* bVisitedList = NULL;
    size_t nFaces = tempFaceList.size();
    if (nFaces != 0) {
        bVisitedList = new bool[nFaces];
        memset(bVisitedList, 0, nFaces * sizeof(bool));
    }

    VertexCache* vcache = new VertexCache(cacheSize);

    int bestNumHits = -1;
    int numHits;
    int bestIndex K2_UNSET;   // read only once a face has set bestNumHits above -1

    while (1) {
        bestNumHits = -1;

        // find best face to add next, given the current cache
        for (unsigned int i = 0; i < tempFaceList.size(); i++) {
            K2_ASSUME(bVisitedList != NULL && i < nFaces);   // the list is not empty, so allocated
            if (bVisitedList[i])
                continue;

            numHits = CalcNumHitsFace(vcache, tempFaceList[i]);
            if (numHits > bestNumHits) {
                bestNumHits = numHits;
                bestIndex = i;
            }
        }

        if (bestNumHits == -1.0f)
            break;
        K2_ASSUME(bestIndex >= 0 && static_cast<size_t>(bestIndex) < nFaces);
        bVisitedList[bestIndex] = true;
        UpdateCacheFace(vcache, tempFaceList[bestIndex]);
        faceList.push_back(tempFaceList[bestIndex]);
    }

    delete vcache;
    delete[] bVisitedList;
}

//
// Returns the number of cache hits per face
int NvStripifier::CalcNumHitsFace(VertexCache* vcache, NvFaceInfo* face)
{
    int numHits = 0;

    if (vcache->InCache(face->m_v0))
        numHits++;
    if (vcache->InCache(face->m_v1))
        numHits++;
    if (vcache->InCache(face->m_v2))
        numHits++;

    return numHits;
}

//
// Updates the input vertex cache with this face's vertices
void NvStripifier::UpdateCacheFace(VertexCache* vcache, NvFaceInfo* face)
{
    if (!vcache->InCache(face->m_v0))
        vcache->AddEntry(face->m_v0);

    if (!vcache->InCache(face->m_v1))
        vcache->AddEntry(face->m_v1);

    if (!vcache->InCache(face->m_v2))
        vcache->AddEntry(face->m_v2);
}

//
// in_indices are the input indices of the mesh to stripify
// in_cacheSize is the target cache size
void NvStripifier::Stripify(const WordVec& in_indices, int in_cacheSize, int in_minStripLength,
                            MyFaceVec& faces, NvStripInfoVec& outStrips, NvFaceInfoVec& outFaceList)
{
    meshJump = 0.0f;
    bFirstTimeResetPoint = true;  // used in FindGoodResetPoint()

    // the cache size
    cacheSize = in_cacheSize;

    // this is the strip size threshold below which we dump the strip into a list
    minStripLength = in_minStripLength;

    indices = in_indices;

    // build the stripification info
    NvFaceInfoVec allFaceInfos;
    NvEdgeInfoVec allEdgeInfos;

    BuildStripifyInfo(allFaceInfos, allEdgeInfos);

    NvStripInfoVec allStrips;

    // stripify
    FindAllStrips(allStrips, allFaceInfos, allEdgeInfos, numsamples, faces);

    // split up the strips into cache friendly pieces, optimize them, then dump these into outStrips
    SplitUpStripsAndOptimize(allStrips, outStrips, allEdgeInfos, outFaceList);

    // clean up
    unsigned int i;
    for (i = 0; i < allStrips.size(); i++)
        delete allStrips[i];

    for (i = 0; i < allEdgeInfos.size(); i++) {
        NvEdgeInfo* info = allEdgeInfos[i];
        while (info != NULL) {
            NvEdgeInfo* next = (static_cast<unsigned int>(info->m_v0) == i ? info->m_nextV0 : info->m_nextV1);
            info->Unref();
            info = next;
        }
    }

    // the face records go back through facestodelete
    facestodelete.m_count = 0;
    for (i = 0; i < allFaceInfos.size(); i++)
        facestodelete.Add(allFaceInfos[i]);
}

//
//
// Does the stripification, puts output strips into vector allStrips
//
// Works by setting runnning a number of experiments in different areas of the mesh, and accepting
// the one which results in the longest strips.  It then accepts this, and moves on to a different
// area of the mesh.  We try to jump around the mesh some, to ensure that large open spans of
// strips get generated.
void NvStripifier::FindAllStrips(NvStripInfoVec& allStrips, NvFaceInfoVec& allFaceInfos,
                                 NvEdgeInfoVec& allEdgeInfos, int numSamples, MyFaceVec& faces)
{
    // the experiments
    int experimentId = 0;
    int i;
    int stripId = 0;
    bool done = false;

    while (!done) {
        //
        // Phase 1: Set up numSamples * numEdges experiments
        //
        int nMaxExperiments = numSamples * numedges;
        NvStripInfoVec* experiments = new NvStripInfoVec[nMaxExperiments];
        int experimentIndex = 0;
        std::set<NvFaceInfo*> resetPoints;
        for (i = 0; i < numSamples; i++) {
            // Try to find another good reset point.
            // If there are none to be found, we are done
            NvFaceInfo* nextFace = FindGoodResetPoint(allFaceInfos, allEdgeInfos);
            if (nextFace == NULL) {
                done = true;
                break;
            }
            // If we have already evaluated starting at this face in this slew of experiments,
            // then skip going any further
            else if (resetPoints.find(nextFace) != resetPoints.end()) {
                continue;
            }

            // trying it now...
            resetPoints.insert(nextFace);
            // at most six experiments per earlier sample, and numedges (never written) is six
            K2_ASSUME(experimentIndex + 6 <= nMaxExperiments);

            // build the strip off of this face's 0-1 edge
            NvEdgeInfo* edge01 = FindEdgeInfo(allEdgeInfos, nextFace->m_v0, nextFace->m_v1);
            NvStripInfo* strip01 = new NvStripInfo(NvStripStartInfo(nextFace, edge01, true), stripId++, experimentId++);
            experiments[experimentIndex++].push_back(strip01);

            // build the strip off of this face's 1-0 edge
            NvEdgeInfo* edge10 = FindEdgeInfo(allEdgeInfos, nextFace->m_v0, nextFace->m_v1);
            NvStripInfo* strip10 = new NvStripInfo(NvStripStartInfo(nextFace, edge10, false), stripId++, experimentId++);
            experiments[experimentIndex++].push_back(strip10);

            // build the strip off of this face's 1-2 edge
            NvEdgeInfo* edge12 = FindEdgeInfo(allEdgeInfos, nextFace->m_v1, nextFace->m_v2);
            NvStripInfo* strip12 = new NvStripInfo(NvStripStartInfo(nextFace, edge12, true), stripId++, experimentId++);
            experiments[experimentIndex++].push_back(strip12);

            // build the strip off of this face's 2-1 edge
            NvEdgeInfo* edge21 = FindEdgeInfo(allEdgeInfos, nextFace->m_v1, nextFace->m_v2);
            NvStripInfo* strip21 = new NvStripInfo(NvStripStartInfo(nextFace, edge21, false), stripId++, experimentId++);
            experiments[experimentIndex++].push_back(strip21);

            // build the strip off of this face's 2-0 edge
            NvEdgeInfo* edge20 = FindEdgeInfo(allEdgeInfos, nextFace->m_v2, nextFace->m_v0);
            NvStripInfo* strip20 = new NvStripInfo(NvStripStartInfo(nextFace, edge20, true), stripId++, experimentId++);
            experiments[experimentIndex++].push_back(strip20);

            // build the strip off of this face's 0-2 edge
            NvEdgeInfo* edge02 = FindEdgeInfo(allEdgeInfos, nextFace->m_v2, nextFace->m_v0);
            NvStripInfo* strip02 = new NvStripInfo(NvStripStartInfo(nextFace, edge02, false), stripId++, experimentId++);
            experiments[experimentIndex++].push_back(strip02);
        }

        //
        // Phase 2: Iterate through that we setup in the last phase and really build each of the
        // strips and strips that follow to see how far we get
        //
        int numExperiments = experimentIndex;
        for (i = 0; i < numExperiments; i++) {
            // get the strip set

            // build the first strip of the list
            experiments[i][0]->Build(allEdgeInfos, allFaceInfos);
            int nStripExperimentId = experiments[i][0]->m_experimentId;

            NvStripInfo* stripIter = experiments[i][0];
            NvStripStartInfo startInfo(NULL, NULL, false);
            while (FindTraversal(allFaceInfos, allEdgeInfos, stripIter, startInfo)) {
                // create the new strip info
                stripIter = new NvStripInfo(startInfo, stripId++, nStripExperimentId);

                // build the next strip
                stripIter->Build(allEdgeInfos, allFaceInfos);

                // add it to the list
                experiments[i].push_back(stripIter);
            }
        }

        //
        // Phase 3: Find the experiment that has the most promise
        //
        int bestIndex = 0;
        double bestValue = 0;
        for (i = 0; i < numExperiments; i++) {
            const float avgStripSizeWeight = 1.0f;
            const float numStripsWeight = 1.0f;
            float avgStripSize = AvgStripSize(experiments[i]);
            float value = avgStripSize * avgStripSizeWeight +
                          (avgStripSize * experiments[i].size() * numStripsWeight);

            if (value > bestValue) {
                bestValue = value;
                bestIndex = i;
            }
        }

        //
        // Phase 4: commit the best experiment of the bunch
        //
        CommitStrips(allStrips, experiments[bestIndex], faces);

        // and destroy all of the others
        for (i = 0; i < numExperiments; i++) {
            if (i != bestIndex) {
                int numStrips = static_cast<int>(experiments[i].size());
                for (int j = 0; j < numStrips; j++)
                    delete experiments[i][j];
            }
        }

        // delete the array that we used for all experiments
        delete[] experiments;
    }
}

//
// Splits the input vector of strips (allBigStrips) into smaller, cache friendly pieces, then
// reorders these pieces to maximize cache hits.  The final strips are output through outStrips
void NvStripifier::SplitUpStripsAndOptimize(NvStripInfoVec& allStrips, NvStripInfoVec& outStrips,
                                            NvEdgeInfoVec& edgeInfos, NvFaceInfoVec& outFaceList)
{
    int threshold = 1000;
    NvStripInfoVec tempStrips;

    // split up strips into threshold-sized pieces
    for (unsigned int i = 0; i < allStrips.size(); i++) {
        NvStripInfo* currentStrip;
        NvStripStartInfo startInfo(NULL, NULL, false);

        if (allStrips[i]->m_faces.size() > static_cast<size_t>(threshold)) {
            unsigned int numTimes = static_cast<unsigned int>(allStrips[i]->m_faces.size() / threshold);
            unsigned int numLeftover = allStrips[i]->m_faces.size() % threshold;

            unsigned int j;
            for (j = 0; j < numTimes; j++) {
                currentStrip = new NvStripInfo(startInfo, 0, -1);

                for (unsigned int faceCtr = j * threshold; faceCtr < threshold + (j * threshold); faceCtr++)
                    currentStrip->m_faces.push_back(allStrips[i]->m_faces[faceCtr]);

                tempStrips.push_back(currentStrip);
            }

            int leftOff = j * threshold;

            if (numLeftover != 0) {
                currentStrip = new NvStripInfo(startInfo, 0, -1);

                for (unsigned int k = 0; k < numLeftover; k++)
                    currentStrip->m_faces.push_back(allStrips[i]->m_faces[leftOff++]);

                tempStrips.push_back(currentStrip);
            }
        } else {
            // we're not just doing a tempStrips.push_back(allBigStrips[i]) because
            // this way we can delete allBigStrips later to free the memory
            currentStrip = new NvStripInfo(startInfo, 0, -1);

            for (unsigned int j = 0; j < allStrips[i]->m_faces.size(); j++)
                currentStrip->m_faces.push_back(allStrips[i]->m_faces[j]);

            tempStrips.push_back(currentStrip);
        }
    }

    // add small strips to face list
    NvStripInfoVec tempStrips2;
    RemoveSmallStrips(tempStrips, tempStrips2, outFaceList);

    outStrips.clear();
    if (tempStrips2.size() != 0) {
        // Optimize for the vertex cache
        VertexCache* vcache = new VertexCache(cacheSize);

        float bestNumHits = -1.0f;
        float numHits;
        int bestIndex K2_UNSET;   // read only once a strip has set bestNumHits above -1

        int firstIndex = 0;
        float minCost = 10000.0f;

        for (unsigned int i = 0; i < tempStrips2.size(); i++) {
            int numNeighbors = 0;

            // find strip with least number of neighbors per face
            for (unsigned int j = 0; j < tempStrips2[i]->m_faces.size(); j++)
                numNeighbors += NumNeighbors(tempStrips2[i]->m_faces[j], edgeInfos);

            float currCost = static_cast<float>(numNeighbors) / static_cast<float>(tempStrips2[i]->m_faces.size());
            if (currCost < minCost) {
                minCost = currCost;
                firstIndex = i;
            }
        }

        UpdateCacheStrip(vcache, tempStrips2[firstIndex]);
        outStrips.push_back(tempStrips2[firstIndex]);

        tempStrips2[firstIndex]->visited = true;

        // this n^2 algo is what slows down stripification so much....
        // needs to be improved
        while (1) {
            bestNumHits = -1.0f;

            // find best strip to add next, given the current cache
            for (unsigned int i = 0; i < tempStrips2.size(); i++) {
                if (tempStrips2[i]->visited)
                    continue;

                numHits = CalcNumHitsStrip(vcache, tempStrips2[i]);
                if (numHits > bestNumHits) {
                    bestNumHits = numHits;
                    bestIndex = i;
                }
            }

            if (bestNumHits == -1.0f)
                break;
            tempStrips2[bestIndex]->visited = true;
            UpdateCacheStrip(vcache, tempStrips2[bestIndex]);
            outStrips.push_back(tempStrips2[bestIndex]);
        }

        delete vcache;
    }
}

//
// Returns the number of neighbors that this face has
int NvStripifier::NumNeighbors(NvFaceInfo* face, NvEdgeInfoVec& edgeInfoVec)
{
    int numNeighbors = 0;

    if (FindOtherFace(edgeInfoVec, face->m_v0, face->m_v1, face) != NULL)
        numNeighbors++;

    if (FindOtherFace(edgeInfoVec, face->m_v1, face->m_v2, face) != NULL)
        numNeighbors++;

    if (FindOtherFace(edgeInfoVec, face->m_v2, face->m_v0, face) != NULL)
        numNeighbors++;

    return numNeighbors;
}

//
// Updates the input vertex cache with this strip's vertices
void NvStripifier::UpdateCacheStrip(VertexCache* vcache, NvStripInfo* strip)
{
    for (unsigned int i = 0; i < strip->m_faces.size(); ++i) {
        if (!vcache->InCache(strip->m_faces[i]->m_v0))
            vcache->AddEntry(strip->m_faces[i]->m_v0);

        if (!vcache->InCache(strip->m_faces[i]->m_v1))
            vcache->AddEntry(strip->m_faces[i]->m_v1);

        if (!vcache->InCache(strip->m_faces[i]->m_v2))
            vcache->AddEntry(strip->m_faces[i]->m_v2);
    }
}

//
// returns the number of cache hits per face in the strip
float NvStripifier::CalcNumHitsStrip(VertexCache* vcache, NvStripInfo* strip)
{
    int numHits = 0;
    int numFaces = 0;

    for (unsigned int i = 0; i < strip->m_faces.size(); i++) {
        if (vcache->InCache(strip->m_faces[i]->m_v0))
            ++numHits;

        if (vcache->InCache(strip->m_faces[i]->m_v1))
            ++numHits;

        if (vcache->InCache(strip->m_faces[i]->m_v2))
            ++numHits;

        numFaces++;
    }

    return (static_cast<float>(numHits) / static_cast<float>(numFaces));
}

//
// Finds the average strip size of the input vector of strips
float NvStripifier::AvgStripSize(const NvStripInfoVec& strips)
{
    int sizeAccum = 0;
    int numStrips = static_cast<int>(strips.size());
    for (int i = 0; i < numStrips; i++) {
        NvStripInfo* strip = strips[i];
        sizeAccum += static_cast<int>(strip->m_faces.size());
    }
    return (static_cast<float>(sizeAccum)) / (static_cast<float>(numStrips));
}

// Returns the next index of a strip, the vertex of `face` that is not one of the strip's last two.
inline int NvStripifier::GetNextIndex(const WordVec& indices, NvFaceInfo* face)
{
    int numIndices = static_cast<int>(indices.size());

    int v0 = indices[numIndices - 2];
    int v1 = indices[numIndices - 1];

    int fv0 = face->m_v0;
    int fv1 = face->m_v1;
    int fv2 = face->m_v2;

    if (fv0 != v0 && fv0 != v1) {
        if ((fv1 != v0 && fv1 != v1) || (fv2 != v0 && fv2 != v1)) {
            printf("GetNextIndex: Triangle doesn't have all of its vertices\n");
            printf("GetNextIndex: Duplicate triangle probably got us derailed\n");
        }
        return fv0;
    }
    if (fv1 != v0 && fv1 != v1) {
        if ((fv0 != v0 && fv0 != v1) || (fv2 != v0 && fv2 != v1)) {
            printf("GetNextIndex: Triangle doesn't have all of its vertices\n");
            printf("GetNextIndex: Duplicate triangle probably got us derailed\n");
        }
        return fv1;
    }
    if (fv2 != v0 && fv2 != v1) {
        if ((fv0 != v0 && fv0 != v1) || (fv1 != v0 && fv1 != v1)) {
            printf("GetNextIndex: Triangle doesn't have all of its vertices\n");
            printf("GetNextIndex: Duplicate triangle probably got us derailed\n");
        }
        return fv2;
    }

    // shouldn't get here
    printf("GetNextIndex: Duplicate triangle sent\n");
    return -1;
}

//
// Builds a strip forward as far as we can go, then builds backwards, and joins the two lists
void NvStripInfo::Build(NvEdgeInfoVec& edgeInfos, NvFaceInfoVec& faceInfos)
{
    K2_UNUSED(faceInfos);
    // used in building the strips forward and backward
    static WordVec scratchIndices;
    scratchIndices.resize(0);

    // build forward... start with the initial face
    NvFaceInfoVec forwardFaces, backwardFaces;
    forwardFaces.push_back(m_startInfo.m_startFace);

    MarkTriangle(m_startInfo.m_startFace);

    int v0 = (m_startInfo.m_toV1 ? m_startInfo.m_startEdge->m_v0 : m_startInfo.m_startEdge->m_v1);
    int v1 = (m_startInfo.m_toV1 ? m_startInfo.m_startEdge->m_v1 : m_startInfo.m_startEdge->m_v0);

    // easiest way to get v2 is to use this function which requires the
    // other indices to already be in the list.
    scratchIndices.push_back(static_cast<unsigned short>(v0));
    scratchIndices.push_back(static_cast<unsigned short>(v1));
    int v2 = NvStripifier::GetNextIndex(scratchIndices, m_startInfo.m_startFace);
    scratchIndices.push_back(static_cast<unsigned short>(v2));

    //
    // build the forward list
    //
    int nv0 = v1;
    int nv1 = v2;

    NvFaceInfo* nextFace = NvStripifier::FindOtherFace(edgeInfos, nv0, nv1, m_startInfo.m_startFace);
    while (nextFace != NULL && !IsMarked(nextFace)) {
        // this tests to see if a face is "unique", meaning that its vertices aren't already in
        // the list so, strips which "wrap-around" are not allowed
        if (!Unique(forwardFaces, nextFace))
            break;

        // add this to the strip
        forwardFaces.push_back(nextFace);

        MarkTriangle(nextFace);

        // add the index
        nv0 = nv1;
        nv1 = NvStripifier::GetNextIndex(scratchIndices, nextFace);
        scratchIndices.push_back(static_cast<unsigned short>(nv1));

        // and get the next face
        nextFace = NvStripifier::FindOtherFace(edgeInfos, nv0, nv1, nextFace);
    }

    // tempAllFaces is going to be forwardFaces + backwardFaces
    // it's used for Unique()
    NvFaceInfoVec tempAllFaces;
    for (unsigned int i = 0; i < forwardFaces.size(); i++)
        tempAllFaces.push_back(forwardFaces[i]);

    //
    // reset the indices for building the strip backwards and do so
    //
    scratchIndices.resize(0);
    scratchIndices.push_back(static_cast<unsigned short>(v2));
    scratchIndices.push_back(static_cast<unsigned short>(v1));
    scratchIndices.push_back(static_cast<unsigned short>(v0));
    nv0 = v1;
    nv1 = v0;
    nextFace = NvStripifier::FindOtherFace(edgeInfos, nv0, nv1, m_startInfo.m_startFace);
    while (nextFace != NULL && !IsMarked(nextFace)) {
        // this tests to see if a face is "unique", meaning that its vertices aren't already in
        // the list so, strips which "wrap-around" are not allowed
        if (!Unique(tempAllFaces, nextFace))
            break;

        // add this to the strip
        backwardFaces.push_back(nextFace);

        // this is just so Unique() will work
        tempAllFaces.push_back(nextFace);

        MarkTriangle(nextFace);

        // add the index
        nv0 = nv1;
        nv1 = NvStripifier::GetNextIndex(scratchIndices, nextFace);
        scratchIndices.push_back(static_cast<unsigned short>(nv1));

        // and get the next face
        nextFace = NvStripifier::FindOtherFace(edgeInfos, nv0, nv1, nextFace);
    }

    // Combine the forward and backwards stripification lists and put into our own face vector
    Combine(forwardFaces, backwardFaces);
}

int NvStripifier::CountRemainingTris(std::list<NvStripInfo*>::iterator iter,
                                     std::list<NvStripInfo*>::iterator end)
{
    int count = 0;
    while (iter != end) {
        count += static_cast<int>((*iter)->m_faces.size());
        iter++;
    }
    return count;
}

//
// Reorders the vertices of the strips (and the leftover faces) into first-use order
void NvStripifier::OptimizeVertices(MyVertexVec& vertices, MyVertexVec& optimizedVerts,
                                    NvStripInfoVec& strips, NvFaceInfoVec& leftoverFaces)
{
    int* vertIndices = new int[vertices.size()];
    memset(vertIndices, -1, sizeof(int) * vertices.size());

    for (unsigned int i = 0; i < strips.size(); i++) {
        for (unsigned int j = 0; j < strips[i]->m_faces.size(); j++) {
            int v0 = strips[i]->m_faces[j]->m_v0;
            int v1 = strips[i]->m_faces[j]->m_v1;
            int v2 = strips[i]->m_faces[j]->m_v2;

            if (vertIndices[v0] == -1) {
                optimizedVerts.push_back(vertices[v0]);
                vertIndices[v0] = strips[i]->m_faces[j]->m_v0 = static_cast<int>(optimizedVerts.size() - 1);
            } else
                strips[i]->m_faces[j]->m_v0 = vertIndices[v0];

            if (vertIndices[v1] == -1) {
                optimizedVerts.push_back(vertices[v1]);
                vertIndices[v1] = strips[i]->m_faces[j]->m_v1 = static_cast<int>(optimizedVerts.size() - 1);
            } else
                strips[i]->m_faces[j]->m_v1 = vertIndices[v1];

            if (vertIndices[v2] == -1) {
                optimizedVerts.push_back(vertices[v2]);
                vertIndices[v2] = strips[i]->m_faces[j]->m_v2 = static_cast<int>(optimizedVerts.size() - 1);
            } else
                strips[i]->m_faces[j]->m_v2 = vertIndices[v2];
        }
    }

    for (unsigned int i = 0; i < leftoverFaces.size(); i++) {
        int v0 = leftoverFaces[i]->m_v0;
        int v1 = leftoverFaces[i]->m_v1;
        int v2 = leftoverFaces[i]->m_v2;

        if (vertIndices[v0] == -1) {
            optimizedVerts.push_back(vertices[v0]);
            vertIndices[v0] = leftoverFaces[i]->m_v0 = static_cast<int>(optimizedVerts.size() - 1);
        } else
            leftoverFaces[i]->m_v0 = vertIndices[v0];

        if (vertIndices[v1] == -1) {
            optimizedVerts.push_back(vertices[v1]);
            vertIndices[v1] = leftoverFaces[i]->m_v1 = static_cast<int>(optimizedVerts.size() - 1);
        } else
            leftoverFaces[i]->m_v1 = vertIndices[v1];

        if (vertIndices[v2] == -1) {
            optimizedVerts.push_back(vertices[v2]);
            vertIndices[v2] = leftoverFaces[i]->m_v2 = static_cast<int>(optimizedVerts.size() - 1);
        } else
            leftoverFaces[i]->m_v2 = vertIndices[v2];
    }

    delete[] vertIndices;
}

bool NvStripifier::NextIsCCW(int numIndices)
{
    return ((numIndices % 2) == 1);
}

//
// Returns true if the face is ordered in CW fashion
bool NvStripifier::IsCW(NvFaceInfo* faceInfo, int v0, int v1)
{
    if (faceInfo->m_v0 == v0)
        return (faceInfo->m_v1 == v1);
    else if (faceInfo->m_v1 == v0)
        return (faceInfo->m_v2 == v1);
    else
        return (faceInfo->m_v0 == v1);
}

//
// Generates actual strips from the list-in-strip-order, one index run per strip, its length in
// stripLengths.
void NvStripifier::CreateStrips(NvStripInfoVec& allStrips, WordVec& stripIndices, WordVec& stripLengths)
{
    int numStrips = static_cast<int>(allStrips.size());
    if (numStrips == 0)
        return;

    stripLengths.resize(numStrips);

    NvFaceInfo tLastFace(0, 0, 0);
    int nPrevSize = 0;

    for (int i = 0; i < numStrips; i++) {
        NvStripInfo* strip = allStrips[i];
        int numFaces = static_cast<int>(strip->m_faces.size());

        // Handle the first face in the strip
        {
            NvFaceInfo tFirstFace(strip->m_faces[0]->m_v0, strip->m_faces[0]->m_v1, strip->m_faces[0]->m_v2);

            // If there is a second face, reorder vertices such that the unique vertex is first
            if (numFaces > 1) {
                int unique = NvStripifier::GetUniqueVertexInB(strip->m_faces[1], &tFirstFace);
                if (unique == tFirstFace.m_v1) {
                    std::swap(tFirstFace.m_v0, tFirstFace.m_v1);
                } else if (unique == tFirstFace.m_v2) {
                    std::swap(tFirstFace.m_v0, tFirstFace.m_v2);
                }

                // If there is a third face, reorder vertices such that the shared vertex is last
                if (numFaces > 2) {
                    int shared = GetSharedVertex(strip->m_faces[2], &tFirstFace);
                    if (shared == tFirstFace.m_v1) {
                        std::swap(tFirstFace.m_v1, tFirstFace.m_v2);
                    }
                }

                // Check CW/CCW ordering
                if (!IsCW(strip->m_faces[0], tFirstFace.m_v0, tFirstFace.m_v1))
                    stripIndices.push_back(static_cast<unsigned short>(tFirstFace.m_v0));
            }

            stripIndices.push_back(static_cast<unsigned short>(tFirstFace.m_v0));
            stripIndices.push_back(static_cast<unsigned short>(tFirstFace.m_v1));
            stripIndices.push_back(static_cast<unsigned short>(tFirstFace.m_v2));

            // Update last face info
            tLastFace = tFirstFace;
        }

        for (int j = 1; j < numFaces; j++) {
            int unique = GetUniqueVertexInB(&tLastFace, strip->m_faces[j]);
            if (unique != -1) {
                stripIndices.push_back(static_cast<unsigned short>(unique));

                // Update last face info
                tLastFace.m_v0 = tLastFace.m_v1;
                tLastFace.m_v1 = tLastFace.m_v2;
                tLastFace.m_v2 = unique;
            }
        }

        stripLengths[i] = static_cast<unsigned short>(stripIndices.size() - nPrevSize);
        nPrevSize = static_cast<int>(stripIndices.size());
    }
}

bool NvStripInfo::Unique(NvFaceInfoVec& faceVec, NvFaceInfo* face)
{
    bool bv0, bv1, bv2;  // bools to indicate whether a vertex is in the faceVec or not
    bv0 = bv1 = bv2 = false;

    for (unsigned int i = 0; i < faceVec.size(); i++) {
        if (!bv0) {
            if ((faceVec[i]->m_v0 == face->m_v0) || (faceVec[i]->m_v1 == face->m_v0) ||
                (faceVec[i]->m_v2 == face->m_v0))
                bv0 = true;
        }

        if (!bv1) {
            if ((faceVec[i]->m_v0 == face->m_v1) || (faceVec[i]->m_v1 == face->m_v1) ||
                (faceVec[i]->m_v2 == face->m_v1))
                bv1 = true;
        }

        if (!bv2) {
            if ((faceVec[i]->m_v0 == face->m_v2) || (faceVec[i]->m_v1 == face->m_v2) ||
                (faceVec[i]->m_v2 == face->m_v2))
                bv2 = true;
        }

        // the face is not unique, all it's vertices exist in the face vector
        if (bv0 && bv1 && bv2)
            return false;
    }

    // if we get out here, it's unique
    return true;
}

//
// Combines the two input face vectors and puts the result into m_faces
void NvStripInfo::Combine(const NvFaceInfoVec& forward, const NvFaceInfoVec& backward)
{
    // add backward faces
    int numFaces = static_cast<int>(backward.size());
    for (int i = numFaces - 1; i >= 0; i--)
        m_faces.push_back(backward[i]);

    // add forward faces
    numFaces = static_cast<int>(forward.size());
    for (int i = 0; i < numFaces; i++)
        m_faces.push_back(forward[i]);
}


void AurStripify(List<int>& faceIndices, List<List<int> >& stripList)
{
    std::vector<MyFace> faces;
    std::vector<NvStripInfo*> stripInfos;
    std::vector<unsigned short> strips;
    std::vector<unsigned short> stripLengths;
    std::vector<NvFaceInfo*> faceInfos;

    NvStripifier* pStripifier = new NvStripifier();

    for (int i = 0; i < faceIndices.m_count; i += 3) {
        MyFace face;
        face.m_nIndex[0] = faceIndices.m_pData[i];
        face.m_nIndex[1] = faceIndices.m_pData[i + 1];
        face.m_nIndex[2] = faceIndices.m_pData[i + 2];
        faces.push_back(face);
    }

    std::vector<unsigned short> indices;
    indices.reserve(faces.size() * 3);
    for (unsigned int i = 0; i < faces.size(); i++) {
        indices.push_back(static_cast<unsigned short>(faces[i].m_nIndex[0]));
        indices.push_back(static_cast<unsigned short>(faces[i].m_nIndex[1]));
        indices.push_back(static_cast<unsigned short>(faces[i].m_nIndex[2]));
    }

    pStripifier->Stripify(indices, 16, 2, faces, stripInfos, faceInfos);
    pStripifier->CreateStrips(stripInfos, strips, stripLengths);

    delete pStripifier;

    int nStrips = static_cast<int>(stripLengths.size()) + 1;
    stripList.SetSize(nStrips);

    int n = 0;
    for (unsigned int i = 0; i < stripLengths.size(); i++)
        for (int j = 0; j < static_cast<int>(stripLengths[i]); j++)
            stripList[i].Add(strips[n++]);

    for (unsigned int i = 0; i < faceInfos.size(); i++) {
        stripList[nStrips - 1].Add(faceInfos[i]->m_v0);
        stripList[nStrips - 1].Add(faceInfos[i]->m_v1);
        stripList[nStrips - 1].Add(faceInfos[i]->m_v2);
    }

    for (int64_t i = static_cast<int64_t>(facestodelete.m_count) - 1; i != -1; i--)
        delete facestodelete.m_pData[i];

    for (unsigned int i = 0; i < stripInfos.size(); i++)
        delete stripInfos[i];
}
