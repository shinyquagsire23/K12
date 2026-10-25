#include "k2stdint.h"
#include "SWCollisionMesh.h"

#include "exostring.h"
#include "resmdl.h"
#include "nwplaceablesurfacemesh.h"
#include "nwdoorsurfacemesh.h"
#include "lyt.h"
#include "nwsarea.h"
#include "swsroom.h"
#include "swroomsurfacemesh.h"

int bDumpData;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/crtapi.h"
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

extern CExoResMan* g_pExoResMan;
extern int quaternionidentityoptimization;
#if KOTOR1
Quaternion operator-(const Quaternion& q);
#endif

extern Vector zaxis;

unsigned int CSWCollisionMesh::GetAllMaterialsMask()
{
    return CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL;
}

unsigned int CSWCollisionMesh::GetWalkableMaterialMask()
{
    return CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL;
}

unsigned int CSWCollisionMesh::GetWalkCheckMaterialMask()
{
    return CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL;
}

unsigned int CSWCollisionMesh::GetLOSMaterialMask()
{
    return CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL;
}

int CSWCollisionMesh::LoadDefaultMesh()
{
    return 1;
}

int CSWCollisionMesh::LoadMeshText(unsigned char* pData, unsigned int size)
{
    K2_UNUSED(pData); K2_UNUSED(size);
    m_bBinaryBacked = 0;
    return 1;
}

// Publishes the base portions of a binary walkmesh resource: marks the mesh binary-backed and copies
// the header fields out, with every array pointer computed into the mesh's own data block. Without a
// parsed header, everything is zeroed instead.
int CSWCollisionMesh::LoadMeshBinary(CResBWM* pRes)
{
    m_bBinaryBacked = 1;

    Vector vecA;
    if (pRes->m_pParsed != 0) {
        CResBWMHeader* pHeader = static_cast<CResBWMHeader*>(pRes->m_pParsed);
        m_bTransformBaked = pHeader->meshType;
        vecA = pHeader->vecA;
    } else {
        m_bTransformBaked = 0;
        vecA = Vector{0.0f, 0.0f, 0.0f};
    }
    m_headerVec[0] = vecA;

    Vector vecB;
    if (pRes->m_pParsed != 0) {
        vecB = static_cast<CResBWMHeader*>(pRes->m_pParsed)->vecB;
    } else {
        vecB = Vector{0.0f, 0.0f, 0.0f};
    }
    m_headerVec[1] = vecB;

    Vector localOrigin;
    if (pRes->m_pParsed != 0) {
        CResBWMHeader* pHeader = static_cast<CResBWMHeader*>(pRes->m_pParsed);
        m_nVertexCount = pHeader->nVertexCount;
        localOrigin = pHeader->localOrigin;
    } else {
        m_nVertexCount = 0;
        localOrigin = Vector{0.0f, 0.0f, 0.0f};
    }
    m_localOrigin = localOrigin;

#if KOTOR1
    if (pRes->m_pParsed != 0) {
        CResBWMHeader* pHeader = static_cast<CResBWMHeader*>(pRes->m_pParsed);
        char* pData = static_cast<char*>(pRes->m_pBWMData);
        m_paVertices.m_p = reinterpret_cast<Vector*>(pData + pHeader->offVertices);
        m_nTriangleCount = pHeader->nTriangleCount;
        m_nWalkableTriangleCount = pHeader->nAdjacencyCount;
        m_paTriVertIndices.m_p = reinterpret_cast<int*>(pData + pHeader->offTriVertIndices);
        m_paTriMaterials.m_p = reinterpret_cast<int*>(pData + pHeader->offTriMaterials);
        m_paTriNormals.m_p = reinterpret_cast<Vector*>(pData + pHeader->offTriNormals);
    } else {
        m_paVertices.m_p = 0;
        m_paTriMaterials.m_p = 0;
        m_paTriNormals.m_p = 0;
        m_nTriangleCount = 0;
        m_nWalkableTriangleCount = 0;
        m_paTriVertIndices.m_p = 0;
    }
    m_paTriPlaneD.m_p = (pRes->m_pParsed != 0)
        ? reinterpret_cast<float*>(static_cast<char*>(pRes->m_pBWMData) + static_cast<CResBWMHeader*>(pRes->m_pParsed)->offTriPlaneD)
        : 0;
#else
    if (pRes->m_pParsed != 0) {
        CResBWMHeader* pHeader = static_cast<CResBWMHeader*>(pRes->m_pParsed);
        char* pData = static_cast<char*>(pRes->m_pBWMData);
        m_paVertices = reinterpret_cast<Vector*>(pData + pHeader->offVertices);
        m_nTriangleCount = pHeader->nTriangleCount;
        unsigned int offTriVertIndices = pHeader->offTriVertIndices;
        int nAdjacencyCount = pHeader->nAdjacencyCount;
        m_paTriVertIndices = reinterpret_cast<int*>(pData + offTriVertIndices);
        m_nWalkableTriangleCount = nAdjacencyCount;
        m_paTriMaterials = reinterpret_cast<int*>(pData + pHeader->offTriMaterials);
        m_paTriNormals = reinterpret_cast<Vector*>(pData + pHeader->offTriNormals);
    } else {
        m_paVertices = 0;
        m_nTriangleCount = 0;
        m_nWalkableTriangleCount = 0;
        m_paTriMaterials = 0;
        m_paTriNormals = 0;
        m_paTriVertIndices = 0;
    }
    m_paTriPlaneD = (pRes->m_pParsed != 0)
        ? reinterpret_cast<float*>(static_cast<char*>(pRes->m_pBWMData) + static_cast<CResBWMHeader*>(pRes->m_pParsed)->offTriPlaneD)
        : 0;
#endif
    return 1;
}

// Top-level mesh loader. The binary walkmesh resource is tried first (created as a binary walkmesh when
// missing), with the text walkmesh as the fallback (created as a model when missing). An existing
// binary resource is a hard failure when bForceText is set; otherwise it goes through the binary loader
// and the resource is kept. The text path releases its resource afterwards. Returns 1 when no resource
// exists, so the mesh keeps its default, or when a demand fails.
int CSWCollisionMesh::LoadMesh(int bForceText)
{
    CResRef resRef;
    GetResourceForBinaryWalkMesh(resRef);
    if (g_pExoResMan->Exists(resRef, m_resTypeBinary, static_cast<unsigned int*>(0))) {
        CRes* pRes = g_pExoResMan->GetResObject(resRef, m_resTypeBinary);
        if (pRes == 0) {
            pRes = new CResBWM();
            g_pExoResMan->SetResObject(resRef, m_resTypeBinary, pRes);
        }
        if (pRes->Demand()) {
            if (static_cast<CResBWM*>(pRes)->IsBinary()) {
                if (bForceText != 0) {
                    pRes->Release();
                    pRes->Dump(1);
                    return 0;
                }
                m_bBinaryBacked = 1;
                int result = LoadMeshBinary(static_cast<CResBWM*>(pRes));
                m_pResource = pRes;
                return result;
            }
            m_bBinaryBacked = 0;
            unsigned char* pData = static_cast<unsigned char*>(static_cast<CResBWM*>(pRes)->GetBWMDataPtr());
            int size = static_cast<CResBWM*>(pRes)->GetBWMSize();
            int result = LoadMeshText(pData, size);
            pRes->Release();
            pRes->Dump(1);
            return result;
        }
        return 1;
    }

    GetResourceForTextWalkMesh(resRef);
    if (g_pExoResMan->Exists(resRef, m_resTypeText, static_cast<unsigned int*>(0))) {
        CRes* pRes = g_pExoResMan->GetResObject(resRef, m_resTypeText);
        if (pRes == 0) {
            pRes = new CResMDL();
            g_pExoResMan->SetResObject(resRef, m_resTypeText, pRes);
        }
        if (pRes->Demand()) {
            m_bBinaryBacked = 0;
            unsigned char* pData = static_cast<unsigned char*>(static_cast<CResMDL*>(pRes)->GetMDLDataPtr());
            int size = static_cast<CResMDL*>(pRes)->GetMDLSize();
            int result = LoadMeshText(pData, size);
            pRes->Release();
            pRes->Dump(1);
            return result;
        }
    }
    return 1;
}

// Converts one placeable's text walkmesh to binary: builds the target path, ensuring a trailing
// separator on the directory name, loads the text mesh into a fresh placeable surface mesh with the
// text path forced, and re-saves it binary. Returns whether the save succeeded, or -1 when the text
// load fails; the mesh leaks on that path.
int CSWCollisionMesh::ConvertPlaceable(const CResRef& resRef, const CExoString& fileName)
{
    CExoString sOutName;
    CExoString sResRef;
    CExoString sPath;
    resRef.CopyToString(sResRef);
    sPath = fileName;
    if (sPath.Right(1) != "\\") {
        CExoString sSlash("\\");
        CExoString sNewPath = sPath + sSlash;
        sPath = sNewPath;
    }
    CSWPlaceableSurfaceMesh* pMesh = new CSWPlaceableSurfaceMesh();
    pMesh->SetMesh(resRef);
    if (pMesh->LoadMesh(1) < 1)
        return -1;
    sOutName.Format(const_cast<char*>("%s%s.PWK"), sPath.CStr(), sResRef.CStr());
    int result = (pMesh->SaveBinaryMesh(sOutName) != 0);
    delete pMesh;
    return result;
}

// Converts every door type in the configured range to a binary walkmesh: per type, a fresh door
// surface mesh gets the type and resref, loads the text mesh with the text path forced, has its
// normals computed and is saved under the given directory. Returns 0 as soon as a save fails,
// otherwise the last load's status, initially 1.
int CSWCollisionMesh::ConvertDoor(const CResRef& resRef, const CExoString& fileName)
{
    CExoString sOutName;
    CExoString sResRef;
    CExoString sPath;
    resRef.CopyToString(sResRef);
    sPath = fileName;
    if (sPath.Right(1) != "\\") {
        CExoString sSlash("\\");
        CExoString sNewPath = sPath + sSlash;
        sPath = sNewPath;
    }
    int result = 1;
    int doorTypeMax = CSWDoorSurfaceMesh::DOOR_STATE_OPEN2;
    for (int doorType = CSWDoorSurfaceMesh::DOOR_STATE_CLOSED; doorType <= doorTypeMax; doorType++) {
        CSWDoorSurfaceMesh* pMesh = new CSWDoorSurfaceMesh();
        pMesh->SetDoorType(doorType);
        pMesh->SetMesh(resRef);
        int loaded = pMesh->LoadMesh(1);
        if (loaded < 1)
            result = loaded;
        pMesh->ComputeMeshNormals();
        sOutName.Format(const_cast<char*>("%s%s%d.DWK"), sPath.CStr(), sResRef.CStr(), doorType);
        int saved = pMesh->SaveBinaryMesh(sOutName);
        delete pMesh;
        if (saved == 0) {
            result = 0;
            break;
        }
    }
    return result;
}

// Converts every room of an area to a binary walkmesh: parses the area's layout, loads the room array,
// then re-saves each room's walkmesh under the given directory. Returns 0 when the layout will not
// load or any save fails, cleaning up the room array and layout on the way.
int CSWCollisionMesh::ConvertRooms(const CResRef& resRef, const CExoString& fileName)
{
    CExoString s1;
    CExoString sPath;
    char areaName[17];
    resRef.CopyToString(areaName);
    sPath = fileName;
    if (sPath.Right(1) != "\\") {
        sPath = sPath + CExoString("\\");
    }
    CLYT* pLyt = new CLYT();
    if (pLyt->LoadLayout(resRef) == 0) {
        pLyt->UnloadLayout();
        delete pLyt;
        return 0;
    }
    int roomCount = pLyt->GetRoomCount();
    CSWSRoom* pRooms = CSWSArea::LoadRooms(pLyt);
    for (int i = 0; i < roomCount; i++) {
        CExoString sOutName;
        CExoString sRoomName;
        CResRef roomRef = pLyt->GetRoomName(i);
        roomRef.CopyToString(sRoomName);
        sOutName.Format(const_cast<char*>("%s%s.WOK"), sPath.CStr(), sRoomName.CStr());
        if (pRooms[i].m_pSurfaceMesh->SaveBinaryMesh(sOutName) == 0) {
            delete[] pRooms;
            pLyt->UnloadLayout();
            delete pLyt;
            pLyt = 0;
            return 0;
        }
    }
    delete[] pRooms;
    pLyt->UnloadLayout();
    delete pLyt;
    pLyt = 0;
    return 1;
}

// Writes the mesh as a binary walkmesh. Refuses when the mesh is already binary-backed or the file
// already exists, writes the header with the base arrays appended behind it, then seeks home and
// rewrites the header with the array offsets patched in.
int CSWCollisionMesh::SaveBinaryMesh(const CExoString& fileName)
{
    if (m_bBinaryBacked)
        return -1;

#if KOTOR1
    FILE* pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/game/servercore/swcollisionmesh.cpp",
                         "SaveBinaryMesh", 589, fileName.CStr(), "r");
#else
    FILE* pFile = fopen(fileName.CStr(), "r");
#endif
    if (pFile != 0) {
        CExoString s;
        s.Format(const_cast<char*>("ERROR: opening a Binary walkmesh file for writeing that already exists (File: %s)"),
                 fileName.CStr());
        fclose(pFile);
        return -1;
    }

#if KOTOR1
    pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/game/servercore/swcollisionmesh.cpp",
                   "SaveBinaryMesh", 599, fileName.CStr(), "wb+");
#else
    pFile = fopen(fileName.CStr(), "wb+");
#endif
    if (pFile == 0)
        return 0;

    CResBWMHeader header = {};
    __strncpy_chk2(header.magic, "BWM V1.0", 8, sizeof(header.magic), sizeof("BWM V1.0"));
    header.meshType = m_bTransformBaked;
    header.vecA = m_headerVec[0];
    header.vecB = m_headerVec[1];
    header.localOrigin = m_localOrigin;
    header.nVertexCount = m_nVertexCount;
    header.nTriangleCount = m_nTriangleCount;
    header.nAdjacencyCount = m_nWalkableTriangleCount;
    fwrite(&header, sizeof(header), 1, pFile);
    header.offVertices = ftell(pFile);
    fwrite(m_paVertices, sizeof(Vector), m_nVertexCount, pFile);
    header.offTriVertIndices = ftell(pFile);
    fwrite(m_paTriVertIndices, sizeof(int), m_nTriangleCount * 3, pFile);
    header.offTriMaterials = ftell(pFile);
    fwrite(m_paTriMaterials, sizeof(int), m_nTriangleCount, pFile);
    header.offTriNormals = ftell(pFile);
    fwrite(m_paTriNormals, sizeof(Vector), m_nTriangleCount, pFile);
    header.offTriPlaneD = ftell(pFile);
    fwrite(m_paTriPlaneD, sizeof(float), m_nTriangleCount, pFile);
    fseek(pFile, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, pFile);
    fclose(pFile);
    return 1;
}

// Reads one text line into out, stopping at maxLen - 1, a newline, or the end of the data. Returns 0
// when the line does not fit.
int CSWCollisionMesh::LoadMeshString(unsigned char** ppData, unsigned int* pRemaining,
                                     unsigned char* out, unsigned int maxLen)
{
    if (*pRemaining == 0)
        return 0;
    unsigned int i = 0;
    unsigned int j;
#if KOTOR1
    do {
        j = i;
        out[j] = *(*ppData)++;
        --*pRemaining;
        if (j == maxLen - 1) break;
        if (out[j] == '\n' || out[j] == '\r') break;
        i = j + 1;
    } while (*pRemaining != 0);
#ifdef K2_FIX_ENGINE_BUGS
    if (out[j] == '\r' && *pRemaining != 0 && **ppData == '\n') {
        ++*ppData;
        --*pRemaining;
    }
#else
    if (out[j] == '\r' && **ppData == '\n')
        ++*ppData;
#endif
#else
    do {
        j = i;
        out[j] = *(*ppData)++;
        --*pRemaining;
        if (j == maxLen - 1) break;
        if (*pRemaining == 0) break;
        i = j + 1;
    } while (out[j] != '\n');
#endif
    if (j >= maxLen)
        return 0;
    out[j] = 0;
    return 1;
}

// Computes the mesh's four local-space footprint corners, with z pinned to the minimum, then
// transforms each one to world space. Returns 0 when the vertex array is null, in which case the
// outputs are untouched; an empty mesh still writes and transforms the zeroed corners and returns 0.
int CSWCollisionMesh::GetMeshOutline(Vector& a, Vector& b, Vector& c, Vector& d)
{
    if (m_paVertices == 0)
        return 0;
    int bSet = 0;
    float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
    float maxX = 0.0f, maxY = 0.0f;
    for (int i = 0; i < m_nVertexCount; i++) {
#if KOTOR1
        Vector v = m_paVertices.Get(i);
#else
        Vector v = m_paVertices[i];
#endif
        if (bSet != 0) {
            if (v.x > maxX) maxX = v.x;
            if (v.x < minX) minX = v.x;
            if (v.y > maxY) maxY = v.y;
            if (v.y < minY) minY = v.y;
            if (v.z < minZ) minZ = v.z;
        } else {
            maxX = v.x;
            minX = v.x;
            maxY = v.y;
            minY = v.y;
            minZ = v.z;
            bSet = 1;
        }
    }
    a.x = minX; a.y = maxY; a.z = minZ;
    b.x = minX; b.y = minY; b.z = minZ;
    c.x = maxX; c.y = minY; c.z = minZ;
    d.x = maxX; d.y = maxY; d.z = minZ;
    a = LocalToWorld(a);
    b = LocalToWorld(b);
    c = LocalToWorld(c);
    d = LocalToWorld(d);
    return bSet;
}

// Computes the world-space bounding box of the mesh; returns 0 when the mesh has no vertices.
int CSWCollisionMesh::GetMeshBoundingBox(Vector& outMin, Vector& outMax)
{
    int bSet = 0;
    if (m_paVertices != 0 && m_nVertexCount >= 1) {
        for (int i = 0; i < m_nVertexCount; i++) {
#if KOTOR1
            Vector v = LocalToWorld(m_paVertices.Get(i));
#else
            Vector v = LocalToWorld(m_paVertices[i]);
#endif
            if (bSet != 0) {
                if (v.x > outMax.x) outMax.x = v.x;
                if (v.x < outMin.x) outMin.x = v.x;
                if (v.y > outMax.y) outMax.y = v.y;
                if (v.y < outMin.y) outMin.y = v.y;
                if (v.z > outMax.z) outMax.z = v.z;
                if (v.z < outMin.z) outMin.z = v.z;
            } else {
                outMax = v;
                outMin = v;
                bSet = 1;
            }
        }
    }
    return bSet;
}

// Walks the mesh for the polygons the line from a to b crosses, skipping the walkable ones: returns 1
// when the line is clear, or 0 after filling pInfo with the hit.
int CSWCollisionMesh::NoNonWalkPolys(const Vector& a, const Vector& b, float c, float d, float e,
                                     CWalkHitInfo* pInfo)
{
    Vector start = WorldToLocal(a);
    Vector end = WorldToLocal(b);
    if (!m_bTransformBaked) {
        c = (c - m_translation.z) - m_localOrigin.z;
        d = (d - m_translation.z) - m_localOrigin.z;
    }
    if (end.x == start.x && end.y == start.y)
        return 1;

    Vector quad[4] = {};
    Vector seg = {end.x - start.x, end.y - start.y, 0.0f};
    Vector hit = {};
    float segLen = magnitude(seg);
    Vector dir = normalize(seg);

    float ex = end.x + dir.x * e;
    float ey = end.y + dir.y * e;
    float ez = end.z + dir.z * e;
    quad[0].x = start.x - dir.y * e;
    quad[0].y = start.y + dir.x * e;
    quad[0].z = start.z + 0.0f;
    quad[1].x = ex - dir.y * e;
    quad[1].y = dir.x * e + ey;
    quad[1].z = ez + 0.0f;
    quad[2].x = dir.y * e + ex;
    quad[2].y = ey - dir.x * e;
    quad[2].z = ez;
    quad[3].x = start.x + dir.y * e;
    quad[3].y = start.y - dir.x * e;
    quad[3].z = start.z;

    // The direction quadrant says which corners hold the swept region's bounds extremes. Both y
    // tests are written as `end.y < start.y`, so a NaN y picks quadrant 1 on the +x side.
    int idx = (start.x <= end.x) ? (!(end.y < start.y) ? 1 : 0)
                                 : ((end.y < start.y) ? 3 : 2);
    float minX = quad[(idx + 3) & 3].x - 0.01f;
    float minY = quad[(idx + 2) & 3].y - 0.01f;
    float maxX = quad[(idx + 1) & 3].x + 0.01f;
    float maxY = quad[idx].y + 0.01f;

    Vector e01 = {quad[0].x - quad[1].x, quad[0].y - quad[1].y, quad[0].z - quad[1].z};
    Vector n01 = normalize(e01);
    quad[1].x += n01.x * e;
    quad[1].y += n01.y * e;
    quad[1].z += n01.z * e;
    Vector e32 = {quad[3].x - quad[2].x, quad[3].y - quad[2].y, quad[3].z - quad[2].z};
    Vector n32 = normalize(e32);
    quad[2].x += n32.x * e;
    quad[2].y += n32.y * e;
    quad[2].z += n32.z * e;

    Vector triVerts[3] = {};
    Vector v0 = {};
    Vector v1 = {};
    Vector bestHit = {};
    int haveHit = 0;
    int hitIsVertex = 0;
    int bestVert = 0;
    int bestTri = 0;
    float bestDist = 3.402823466e+38f; // FLT_MAX
    unsigned int walkMask = GetWalkCheckMaterialMask();
    for (int i = m_nWalkableTriangleCount; i < m_nTriangleCount; i++) {
        for (int j = 0; j < 3; j++) {
            int vi = m_paTriVertIndices[i * 3 + j];
            if (vi >= 0) {
#if KOTOR1
                triVerts[j] = m_paVertices.Get(vi);
#else
                triVerts[j].z = m_paVertices[vi].z;
                triVerts[j].x = m_paVertices[vi].x;
                triVerts[j].y = m_paVertices[vi].y;
#endif
            }
        }
        if (c <= d &&
            ((walkMask & (1 << m_paTriMaterials[i])) != 0) &&
            (triVerts[0].z <= d || triVerts[1].z <= d || triVerts[2].z <= d) &&
            (c <= triVerts[0].z || c <= triVerts[1].z || c <= triVerts[2].z) &&
            (triVerts[0].x <= maxX || triVerts[1].x <= maxX || triVerts[2].x <= maxX) &&
            (triVerts[0].y <= maxY || triVerts[1].y <= maxY || triVerts[2].y <= maxY) &&
            (minX <= triVerts[0].x || minX <= triVerts[1].x || minX <= triVerts[2].x) &&
            (minY <= triVerts[0].y || minY <= triVerts[1].y || minY <= triVerts[2].y)) {
            int inside[3];
            for (int j = 0; j < 3; j++) {
                v0 = triVerts[j];
                float t = dir.x * (v0.x - quad[0].x) + dir.y * (v0.y - quad[0].y) +
                          dir.z * (v0.z - quad[0].z);
                int inRange;
                if (t <= segLen && t >= 0.0f) {
                    // Perpendicular distance from the segment, as a 2D cross written in 3D form.
                    float perp = dir.y * (v0.x - quad[0].x) - dir.x * (v0.y - quad[0].y) +
                                 (v0.z - quad[0].z) * 0.0f;
                    inRange = (perp <= e + e && perp >= 0.0f);
                } else {
                    // past a segment end: inside only if within the far cap's radius
                    float d2 = (v0.x - end.x) * (v0.x - end.x) +
                               (v0.y - end.y) * (v0.y - end.y) +
                               (v0.z - end.z) * (v0.z - end.z);
                    inRange = (d2 <= e * e);
                }
                if (!inRange) {
                    inside[j] = 1;
                    continue;
                }
                inside[j] = 0;
                if (c <= v0.z && v0.z <= d) {
                    if (pInfo == 0)
                        return 0;
                    float t2 = dir.x * (v0.x - start.x) + dir.y * (v0.y - start.y) +
                               dir.z * (v0.z - start.z);
                    if (t2 < bestDist) {
                        haveHit = 1;
                        hitIsVertex = 1;
                        bestVert = j;
                        bestTri = i;
                        bestHit = v0;
                        bestDist = t2;
                    }
                }
            }
            // an edge with both endpoints outside the swept region can't be hit
            int flag = 1;
            if (inside[0] == 0 && inside[1] == 0) flag = 0;
            int edges[2] = {1, 1};
            if (inside[1] == 0 && inside[2] == 0) edges[0] = 0;
            if (inside[2] == 0 && inside[0] == 0) edges[1] = 0;
            Vector* pv = triVerts;
            // The counter is unsigned char on purpose, because with a wider type the modulus below
            // is folded away entirely. See this function's heading.
            for (unsigned char j = 0;; j++) {
                v0 = *pv;
                v1 = triVerts[(j + 1) % 3];
                if (flag == 1) {
                    for (int k = 0; k < 4; k++) {
                        if (k == 1)
                            continue;
                        int r = IntersectLineSegments2d(v0, v1, quad[k], quad[(k + 1) % 4], hit);
                        if (r == 1) {
                            if (pInfo == 0)
                                return 0;
                            float t = dir.x * (hit.x - start.x) + dir.y * (hit.y - start.y) +
                                      dir.z * (hit.z - start.z);
                            if (t < bestDist) {
                                bestDist = t;
                                bestHit = hit;
                                bestVert = j;
                                bestTri = i;
                                haveHit = 1;
                                hitIsVertex = 0;
                            }
                        }
                    }
                    Vector v0f = {v0.x, v0.y, 0.0f};
                    Vector v1f = {v1.x, v1.y, 0.0f};
                    Vector circleC = {end.x, end.y, 0.0f};
                    Vector c0 = {};
                    Vector c1 = {};
                    int nHits = IntersectCircle2d(circleC, e + 0.001f, v0f, v1f, c0, c1,
                                                  pInfo != 0);
                    if (pInfo == 0) {
                        if (nHits > 0)
                            return 0;
                    } else {
                        if (nHits == 1) {
                            float t = dir.x * (c0.x - start.x) + dir.y * (c0.y - start.y) +
                                      dir.z * (c0.z - start.z);
                            if (t >= 0.0f && t < bestDist) {
                                bestDist = t;
                                bestHit = c0;
                                bestVert = j;
                                bestTri = i;
                                haveHit = 1;
                                hitIsVertex = 0;
                            }
                        } else if (nHits == 2) {
                            float t = dir.x * (c1.x - start.x) + dir.y * (c1.y - start.y) +
                                      dir.z * (c1.z - start.z);
                            if (t >= 0.0f && t < bestDist) {
                                bestDist = t;
                                bestHit = c1;
                                bestVert = j;
                                bestTri = i;
                                haveHit = 1;
                                hitIsVertex = 0;
                            }
                            t = dir.x * (c0.x - start.x) + dir.y * (c0.y - start.y) +
                                dir.z * (c0.z - start.z);
                            if (t >= 0.0f && t < bestDist) {
                                bestDist = t;
                                bestHit = c0;
                                bestVert = j;
                                bestTri = i;
                                haveHit = 1;
                                hitIsVertex = 0;
                            }
                        }
                    }
                }
                if (j == 2) {
                    // Whether the start point is inside the projected triangle, as 3D cross products
                    // against the up axis.
                    if (((zaxis.y * (triVerts[0].z - start.z) -
                          (triVerts[0].y - start.y) * zaxis.z) * (triVerts[1].x - triVerts[0].x) +
                         ((triVerts[0].x - start.x) * zaxis.z -
                          (triVerts[0].z - start.z) * zaxis.x) * (triVerts[1].y - triVerts[0].y) +
                         ((triVerts[0].y - start.y) * zaxis.x -
                          (triVerts[0].x - start.x) * zaxis.y) * (triVerts[1].z - triVerts[0].z) >= 0.0f) &&
                        ((zaxis.y * (triVerts[1].z - start.z) -
                          (triVerts[1].y - start.y) * zaxis.z) * (triVerts[2].x - triVerts[1].x) +
                         ((triVerts[1].x - start.x) * zaxis.z -
                          (triVerts[1].z - start.z) * zaxis.x) * (triVerts[2].y - triVerts[1].y) +
                         ((triVerts[1].y - start.y) * zaxis.x -
                          (triVerts[1].x - start.x) * zaxis.y) * (triVerts[2].z - triVerts[1].z) >= 0.0f) &&
                        ((zaxis.y * (triVerts[2].z - start.z) -
                          (triVerts[2].y - start.y) * zaxis.z) * (triVerts[0].x - triVerts[2].x) +
                         ((triVerts[2].x - start.x) * zaxis.z -
                          (triVerts[2].z - start.z) * zaxis.x) * (triVerts[0].y - triVerts[2].y) +
                         ((triVerts[2].y - start.y) * zaxis.x -
                          (triVerts[2].x - start.x) * zaxis.y) * (triVerts[0].z - triVerts[2].z) >= 0.0f)) {
                        if (pInfo != 0) {
                            pInfo->m_hitType = 2;
                            pInfo->m_hitSubType = 3;
                        }
                        return 0;
                    }
                    break;
                }
                flag = edges[j];
                pv++;
            }
        }
    }
    if (pInfo != 0 && haveHit == 1) {
        for (int j = 0; j < 3; j++) {
            int vi = m_paTriVertIndices[bestTri * 3 + j];
            if (vi >= 0) {
#if KOTOR1
                triVerts[j] = m_paVertices.Get(vi);
#else
                triVerts[j].z = m_paVertices[vi].z;
                triVerts[j].x = m_paVertices[vi].x;
                triVerts[j].y = m_paVertices[vi].y;
#endif
            }
        }
        v0 = triVerts[bestVert];
        v1 = triVerts[(bestVert + 1) % 3];
        Vector v2 = triVerts[(bestVert + 2) % 3];
        int edgeStyleFill;
        if (v0.x != v1.x || v0.y != v1.y) {
            edgeStyleFill = hitIsVertex;
        } else {
            edgeStyleFill = (hitIsVertex != 0 || v0.z != v1.z);
        }
        if (edgeStyleFill) {
            // vertex hit (or degenerate edge): average the two adjacent edge normals
            Vector e1 = {v0.x - v1.x, v0.y - v1.y, v0.z - v1.z};
            Vector n0;
            n0.x = zaxis.y * e1.z - e1.y * zaxis.z;
            n0.y = e1.x * zaxis.z - e1.z * zaxis.x;
            n0.z = e1.y * zaxis.x - e1.x * zaxis.y;
            Vector n0n = normalize(n0);
            Vector e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
            Vector n1;
            n1.x = zaxis.y * e2.z - e2.y * zaxis.z;
            n1.y = e2.x * zaxis.z - e2.z * zaxis.x;
            n1.z = e2.y * zaxis.x - e2.x * zaxis.y;
            Vector n1n = normalize(n1);
            Vector bis = {n0n.x + n1n.x, n0n.y + n1n.y, n0n.z + n1n.z};
            Vector bisn = normalize(bis);
            const Vector& triN = m_paTriNormals[bestTri];
            if (triN.x * zaxis.x + triN.y * zaxis.y + triN.z * zaxis.z <= 0.0f &&
                bisn.x * triN.x + bisn.y * triN.y + bisn.z * triN.z < 0.0f) {
                bisn.x = -bisn.x;
                bisn.y = -bisn.y;
                bisn.z = -bisn.z;
            }
            float t = dir.x * (bestHit.x - start.x) + dir.y * (bestHit.y - start.y) +
                      dir.z * (bestHit.z - start.z);
            t = t + 0.001f - e;
            Vector hp;
            hp.x = dir.x * t + start.x;
            hp.y = dir.y * t + start.y;
            hp.z = dir.z * t + start.z;
            Vector hpw = LocalToWorld(hp);
            Vector nw = NormalLocalToWorld(bisn);
            Vector bhw = LocalToWorld(bestHit);
            pInfo->m_hitType = 1;
            pInfo->m_hitPosition = hpw;
            pInfo->m_hitNormal = nw;
            pInfo->m_hitPointA = bhw;
            pInfo->m_hitPointB = bhw;
            pInfo->m_triNormalLocal = triN;
            pInfo->m_hitSubType = 2;
            return 0;
        }
        // edge hit: the edge's perpendicular, pushed back out of the wall
        Vector edge = {v0.x - v1.x, v0.y - v1.y, 0.0f};
        Vector n = normalize(edge);
        Vector perp = {-n.y, n.x, 0.0f};
        Vector line1p = {start.x, start.y, 0.0f};
        Vector v0f = {v0.x, v0.y, 0.0f};
        Vector segd = {end.x - start.x, end.y - start.y, 0.0f};
        Vector e2 = {v1.x - v0.x, v1.y - v0.y, 0.0f};
        float crz = e2.y * segd.x - e2.x * segd.y;
        float crx = segd.y * e2.z - e2.y * segd.z;
        float cry = e2.x * segd.z - segd.x * e2.z;
        if (crz != 0.0f || crx != 0.0f || cry != 0.0f) {
            float num = end.z * n.z + (end.x * n.x + end.y * n.y) -
                        (n.x * start.x + n.y * start.y + n.z * start.z);
            float den = end.z * 0.0f + (end.y * n.x - end.x * n.y) -
                        ((n.x * start.y - start.x * n.y) + start.z * 0.0f);
            float ratio = (num / den) * e;
            float d0 = __builtin_sqrtf(e * e + ratio * ratio) + 0.03f;
            DistanceBetweenLines(line1p, segd, v0f, e2, &bestHit, 0);
            Vector u = normalize({start.x - end.x, start.y - end.y, start.z - end.z});
            float segLenSq = (end.x - start.x) * (end.x - start.x) +
                             (end.y - start.y) * (end.y - start.y) +
                             (end.z - start.z) * (end.z - start.z);
            Vector newPt;
            newPt.x = bestHit.x + d0 * u.x;
            newPt.y = bestHit.y + d0 * u.y;
            newPt.z = start.z;
            float dist2 = (newPt.x - start.x) * (newPt.x - start.x) +
                          (newPt.y - start.y) * (newPt.y - start.y) +
                          (start.z - start.z) * (start.z - start.z);
            if (dist2 > segLenSq) {
                float num2 = ((end.y * (-n.y) - end.x * n.x) - end.z * n.z) -
                             ((start.y * (-n.y) - start.x * n.x) - start.z * n.z);
                float den2 = (end.z * 0.0f + (end.x * n.y - end.y * n.x)) -
                             ((n.y * start.x - n.x * start.y) + start.z * 0.0f);
                float ratio2 = (num2 / den2) * e + 0.03f;
                d0 = __builtin_sqrtf(e * e + ratio2 * ratio2);
                Vector u2 = normalize({start.x - end.x, start.y - end.y, start.z - end.z});
                newPt.x = bestHit.x + d0 * u2.x;
                newPt.y = bestHit.y + d0 * u2.y;
                newPt.z = d0 * u2.z + bestHit.z;
            }
            newPt.z = start.z + (dist2 * (start.z - end.z)) / segLenSq;
            if (v0.x == v1.x && v0.y == v1.y) {
                const Vector& triN = m_paTriNormals[bestTri];
                Vector hpw = LocalToWorld(newPt);
                Vector nw = NormalLocalToWorld(triN);
                Vector v0w = LocalToWorld(v0);
                Vector v1w = LocalToWorld(v1);
                pInfo->m_hitPosition = hpw;
                pInfo->m_hitNormal = nw;
                pInfo->m_hitPointA = v0w;
                pInfo->m_hitPointB = v1w;
                pInfo->m_hitSubType = 2;
                pInfo->m_hitType = 1;
                pInfo->m_triNormalLocal = triN;
                return 0;
            }
            Vector hpw = LocalToWorld(newPt);
            Vector nw = NormalLocalToWorld(perp);
            Vector v0w = LocalToWorld(v0);
            Vector v1w = LocalToWorld(v1);
            const Vector* pTriN = &m_paTriNormals[bestTri];
            pInfo->m_hitPosition = hpw;
            pInfo->m_hitNormal = nw;
            pInfo->m_hitPointA = v0w;
            pInfo->m_hitPointB = v1w;
            pInfo->m_hitSubType = 1;
            pInfo->m_hitType = 1;
            pInfo->m_triNormalLocal = *pTriN;
            return 0;
        }
        // segment parallel to the edge: report the start point with the triangle normal
        bestHit = start;
        const Vector& triN = m_paTriNormals[bestTri];
        Vector hpw = LocalToWorld(start);
        Vector nw = NormalLocalToWorld(triN);
        pInfo->m_hitType = 1;
        pInfo->m_hitPosition = hpw;
        pInfo->m_hitNormal = nw;
        pInfo->m_hitPointA = hpw;
        pInfo->m_hitPointB = hpw;
        pInfo->m_triNormalLocal = triN;
        pInfo->m_hitSubType = 2;
        return 0;
    }
    return 1;
}

void CSWCollisionMesh::GetResourceForBinaryWalkMesh(CResRef& out)
{
    out = m_resRef;
}

void CSWCollisionMesh::GetResourceForTextWalkMesh(CResRef& out)
{
    out = m_resRef;
}

float CSWCollisionMesh::ZWorldToZLocal(float worldZ)
{
    if (m_bTransformBaked)
        return worldZ;
    return worldZ - m_translation.z - m_localOrigin.z;
}

float CSWCollisionMesh::ZLocalToZWorld(float localZ)
{
    if (m_bTransformBaked)
        return localZ;
    float sum = localZ + m_localOrigin.z;
    return m_translation.z + sum;
}

// Initialises the mesh's identity transform, its resource types and its empty array state.
CSWCollisionMesh::CSWCollisionMesh(unsigned short resTypeText, unsigned short resTypeBinary)
    : m_resRef()
{
    m_rotation.x = 0.0f;
    m_rotation.y = 0.0f;
    m_rotation.z = 0.0f;

    m_resTypeBinary = resTypeBinary;
    m_resTypeText = resTypeText;

    m_bTransformBaked = 0;

    m_pResource = 0;

    m_bBinaryBacked = 0;
    m_nVertexCount = 0;
#if KOTOR1
    m_paVertices = 0;
    m_nTriangleCount = 0;
    m_nWalkableTriangleCount = 0;
    m_paTriVertIndices = 0;
    m_paTriMaterials = 0;
    m_paTriNormals = 0;
    m_paTriPlaneD = 0;
#endif

    m_rotation.w = 1.0f;

#if !KOTOR1
    __builtin_memset(&m_paVertices, 0,
                     reinterpret_cast<char*>(&m_paTriPlaneD) + sizeof(m_paTriPlaneD) - reinterpret_cast<char*>(&m_paVertices));
#endif
}

CSWCollisionMesh::~CSWCollisionMesh()
{
    if (m_bBinaryBacked == 0) {
#if KOTOR1
        m_paVertices.Delete();
        m_paTriVertIndices.Delete();
        m_paTriMaterials.Delete();
        m_paTriNormals.Delete();
        m_paTriPlaneD.Delete();
#else
        delete[] m_paVertices;
        m_paVertices = 0;
        delete[] m_paTriVertIndices;
        m_paTriVertIndices = 0;
        delete[] m_paTriMaterials;
        m_paTriMaterials = 0;
        // An engine bug, reproduced. It is allocated with array new in SetTriangleCount but freed
        // with a plain delete here.
        delete m_paTriNormals;
        m_paTriNormals = 0;
        delete m_paTriPlaneD;
        m_paTriPlaneD = 0;
#endif
    } else {
        m_paVertices = 0;
        m_paTriVertIndices = 0;
        m_paTriMaterials = 0;
#if KOTOR1
        m_paTriPlaneD = 0;
        m_paTriNormals = 0;
#else
        m_paTriNormals = 0;
        m_paTriPlaneD = 0;
#endif
        m_pResource->Release();
        if (!g_pExoResMan->ReleaseResObject(m_pResource) && m_pResource) {
            delete m_pResource;
        }
        m_pResource = 0;
    }
}

// Allocates the vertex array; Vector's own default constructor zeroes the new storage.
void CSWCollisionMesh::SetVertexCount(int n)
{
    m_nVertexCount = n;
    m_paVertices = new Vector[n];
}

// Allocates the per-triangle arrays, one normal per triangle.
void CSWCollisionMesh::SetTriangleCount(int n)
{
    m_nTriangleCount = n;
    if (m_bBinaryBacked == 0) {
        m_paTriVertIndices = new int[n * 3];
        m_paTriMaterials = new int[n];
        m_paTriPlaneD = new float[n];
        m_paTriNormals = new Vector[n];
    }
}

void CSWCollisionMesh::GetTriangleVertices(int triangle, int* a, int* b, int* c)
{
    if (triangle < m_nTriangleCount) {
#if KOTOR1
        unsigned int base = triangle * 3;
        *a = m_paTriVertIndices[static_cast<int>(base)];
        *b = m_paTriVertIndices[static_cast<int>(base + 1)];
        *c = m_paTriVertIndices[static_cast<int>(base + 2)];
#else
        int* p = &m_paTriVertIndices[static_cast<int64_t>(triangle) + static_cast<int64_t>(triangle * 2)];
        *a = p[0];
        *b = p[1];
        *c = p[2];
#endif
    }
}

// Version of GetVertex taking a Vector out-parameter.
void CSWCollisionMesh::GetVertex(int idx, Vector* out)
{
    if (m_paVertices && idx < m_nVertexCount) {
#if KOTOR1
        Vector v = m_paVertices.Get(idx);
        *out = LocalToWorld(v);
#else
        *out = LocalToWorld(m_paVertices[idx]);
#endif
    }
}

void CSWCollisionMesh::GetVertex(int idx, float* x, float* y, float* z)
{
    if (m_paVertices && idx < m_nVertexCount) {
#if KOTOR1
        Vector local = m_paVertices.Get(idx);
        Vector v = LocalToWorld(local);
#else
        Vector v = LocalToWorld(m_paVertices[idx]);
#endif
        *x = v.x;
        *y = v.y;
        *z = v.z;
    }
}

// Rotates (v + m_localOrigin) by the mesh's quaternion, then adds m_translation. When the transform is
// baked, or already applied to the vertices, v passes through unchanged.
Vector CSWCollisionMesh::LocalToWorld(const Vector& v)
{
    if (m_bTransformBaked)
        return v;

    Vector p = { v.x + m_localOrigin.x, v.y + m_localOrigin.y, v.z + m_localOrigin.z };
    float w = m_rotation.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float x = m_rotation.x, y = m_rotation.y, z = m_rotation.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;
        Vector r;
        r.x = p.x * (1.0f - ((yy + zz) + (yy + zz))) + p.y * ((xy - wz) + (xy - wz)) + p.z * ((xz + wy) + (xz + wy));
        r.y = p.x * ((xy + wz) + (xy + wz)) + p.y * (1.0f - ((xx + zz) + (xx + zz))) + p.z * ((yz - wx) + (yz - wx));
        r.z = p.x * ((xz - wy) + (xz - wy)) + p.y * ((yz + wx) + (yz + wx)) + p.z * (1.0f - ((xx + yy) + (xx + yy)));
        p = r;
    }
    p.x += m_translation.x;
    p.y += m_translation.y;
    p.z += m_translation.z;
    return p;
}

// The inverse of LocalToWorld. It subtracts the translation, rotates by the conjugate quaternion
// (the transposed rotation), then subtracts the local origin.
Vector CSWCollisionMesh::WorldToLocal(const Vector& v)
{
    if (m_bTransformBaked)
        return v;

#if KOTOR1
    Quaternion q = -m_rotation;
    Vector p = { v.x - m_translation.x, v.y - m_translation.y, v.z - m_translation.z };
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float x = q.x, y = q.y, z = q.z;
        float nw = w;
#else
    Vector p = { v.x - m_translation.x, v.y - m_translation.y, v.z - m_translation.z };
    float w = m_rotation.w;
    if (quaternionidentityoptimization == 0 || (w != -1.0f && w != 1.0f)) {
        float x = m_rotation.x, y = m_rotation.y, z = m_rotation.z;
        float nw = -w;
#endif
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = nw * x, wy = nw * y, wz = nw * z;
        Vector r;
        r.x = p.x * (1.0f - ((yy + zz) + (yy + zz))) + p.y * ((xy - wz) + (xy - wz)) + p.z * ((xz + wy) + (xz + wy));
        r.y = p.x * ((xy + wz) + (xy + wz)) + p.y * (1.0f - ((xx + zz) + (xx + zz))) + p.z * ((yz - wx) + (yz - wx));
        r.z = p.x * ((xz - wy) + (xz - wy)) + p.y * ((yz + wx) + (yz + wx)) + p.z * (1.0f - ((xx + yy) + (xx + yy)));
        p = r;
    }
    p.x -= m_localOrigin.x;
    p.y -= m_localOrigin.y;
    p.z -= m_localOrigin.z;
    return p;
}

// The same rotation as LocalToWorld without the translation, since normals rotate but do not move.
Vector CSWCollisionMesh::NormalLocalToWorld(const Vector& v)
{
    if (m_bTransformBaked)
        return v;

    float w = m_rotation.w;
    if (quaternionidentityoptimization != 0 && (w == 1.0f || w == -1.0f))
        return v;

    float x = m_rotation.x, y = m_rotation.y, z = m_rotation.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

// The inverse of NormalLocalToWorld, rotating by the conjugate quaternion only.
Vector CSWCollisionMesh::NormalWorldToLocal(const Vector& v)
{
    if (m_bTransformBaked)
        return v;

#if KOTOR1
    Quaternion q = -m_rotation;
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == 1.0f || w == -1.0f))
        return v;

    float x = q.x, y = q.y, z = q.z;
    float nw = w;
#else
    float w = m_rotation.w;
    if (quaternionidentityoptimization != 0 && (w == -1.0f || w == 1.0f))
        return v;

    float x = m_rotation.x, y = m_rotation.y, z = m_rotation.z;
    float nw = -w;
#endif
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = nw * x, wy = nw * y, wz = nw * z;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

// Bakes the transform into the vertices: records the offset, then adds the local origin and the
// translation to every vertex. The loop pragma keeps the body from being vectorised with runtime
// alias checks.
void CSWCollisionMesh::TransformToWorld(const Vector& offset)
{
    if (m_bBinaryBacked == 0) {
        m_translation = offset;
        m_bTransformBaked = 1;
#pragma clang loop vectorize(disable) interleave(disable)
        for (int i = 0; i < m_nVertexCount; i++)
#if KOTOR1
            m_paVertices.Set(i, m_paVertices.Get(i) + m_localOrigin + m_translation);
#else
            m_paVertices[i] = m_paVertices[i] + m_localOrigin + m_translation;
#endif
    }
}

// Computes each triangle's normal and plane distance, plus the plane-D table the edge tests use.
void CSWCollisionMesh::ComputeMeshNormals()
{
    if (m_bBinaryBacked == 0 && m_nTriangleCount > 0) {
        unsigned int idx3 = 0;
        for (int tri = 0; tri < m_nTriangleCount; tri++) {
#if KOTOR1
            Vector a = m_paVertices.Get(m_paTriVertIndices[idx3]);
            Vector b = m_paVertices.Get(m_paTriVertIndices[idx3 + 1]);
            Vector c = m_paVertices.Get(m_paTriVertIndices[idx3 + 2]);
            Vector n = NormalOf(a, b, c);
            m_paTriNormals.Set(tri, n);
            idx3 += 3;
            m_paTriPlaneD.Set(tri, -(n.x * a.x + n.y * a.y + n.z * a.z));
#else
            Vector a = m_paVertices[m_paTriVertIndices[idx3]];
            Vector b = m_paVertices[m_paTriVertIndices[idx3 + 1]];
            Vector c = m_paVertices[m_paTriVertIndices[idx3 + 2]];
            Vector n = NormalOf(a, b, c);
            m_paTriNormals[tri] = n;
            idx3 += 3;
            m_paTriPlaneD[tri] = -(n.x * a.x + n.y * a.y + n.z * a.z);
#endif
        }
    }
}

// The release build's debug-draw entry point draws nothing, and reports the walkable material mask
// instead.
void CSWCollisionMesh::RenderMesh(int flag, unsigned int color1, unsigned int color2)
{
    K2_UNUSED(flag); K2_UNUSED(color1); K2_UNUSED(color2);
    GetWalkableMaterialMask();
}

// The three walkmesh-conversion console commands. Each splits its argument on the first space into a
// resref and a file name, and hands both to the matching converter.
//
// The success literal is spelled "SUSSESS". That is the engine's own typo, and it is a distinct
// string from every other success literal in the engine.
const char* ConvertDoor(char* pszArgs)
{
    CExoString resRefName;
    CExoString fileName(pszArgs);

    int nSpace = fileName.Find(' ', 0);
    resRefName = fileName.Left(nSpace);

#if KOTOR1 // K1: CExoString::GetLength is out of line
    int len = fileName.GetLength();
#else
    int len = 0;
    if (fileName.m_sString != 0)
        len = static_cast<int>(strlen(fileName.m_sString));
#endif
    fileName = fileName.Right(len - nSpace - 1);

    return CSWCollisionMesh::ConvertDoor(CResRef(resRefName), fileName) == 1 ? "SUSSESS" : "FAILURE";
}

const char* ConvertPlaceable(char* pszArgs)
{
    CExoString resRefName;
    CExoString fileName(pszArgs);

    int nSpace = fileName.Find(' ', 0);
    resRefName = fileName.Left(nSpace);

#if KOTOR1 // K1: CExoString::GetLength is out of line
    int len = fileName.GetLength();
#else
    int len = 0;
    if (fileName.m_sString != 0)
        len = static_cast<int>(strlen(fileName.m_sString));
#endif
    fileName = fileName.Right(len - nSpace - 1);

    return CSWCollisionMesh::ConvertPlaceable(CResRef(resRefName), fileName) == 1 ? "SUSSESS"
                                                                                  : "FAILURE";
}

const char* ConvertRooms(char* pszArgs)
{
    CExoString resRefName;
    CExoString fileName(pszArgs);

    int nSpace = fileName.Find(' ', 0);
    resRefName = fileName.Left(nSpace);

#if KOTOR1 // K1: CExoString::GetLength is out of line
    int len = fileName.GetLength();
#else
    int len = 0;
    if (fileName.m_sString != 0)
        len = static_cast<int>(strlen(fileName.m_sString));
#endif
    fileName = fileName.Right(len - nSpace - 1);

    return CSWCollisionMesh::ConvertRooms(CResRef(resRefName), fileName) == 1 ? "SUSSESS" : "FAILURE";
}
