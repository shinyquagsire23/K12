#include "k2stdint.h"
#include "nwplaceablesurfacemesh.h"
#include "restype.h"

#include "port/libcbase.h"
#include "port/libcstring.h"

#include "port/crtapi.h"

CSWPlaceableSurfaceMesh::CSWPlaceableSurfaceMesh()
    : CSWCollisionMesh(RESTYPE_MDL, RESTYPE_PWK)
{
}

CSWPlaceableSurfaceMesh::~CSWPlaceableSurfaceMesh()
{
}

// Loads the placeable's text .wok, the same walkmesh format the room loader reads without the
// bounding-volume tree: a node and position block, then optional endnode, orientation, verts and
// faces blocks.
//
// A header tag of "pwk_use01"/"pwk_use02"/"pwk_dp_use_01"/"pwk_dp_use_02" selects a door state,
// and while one is selected a "position" line records that state's action point into
// m_headerVec rather than moving the mesh.
//
// Reports success for an empty or short file and failure for a malformed state guard or a truncated
// verts or faces block. The verts failure path leaks its line buffer and the faces one frees it.
int CSWPlaceableSurfaceMesh::LoadMeshText(unsigned char* pData, unsigned int size)
{
    unsigned char* lineBuf = new unsigned char[MESH_LINE_SIZE];
    unsigned char* cursor = pData;
    unsigned int remaining = size;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float vertX = 0.0f, vertY = 0.0f, vertZ = 0.0f;
    unsigned char state = 0;
    int gotNode = 0;
    int sawTrimesh = 0;

    if (size == 0) {
        delete[] lineBuf;
        m_resRef.GetResRef();
        return 1;
    }

    {
        do {
            if (LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) != 1)
                break;

            unsigned char* p = lineBuf;
            while (*p == ' ' || *p == '\t')
                p++;

            if (strncmp(reinterpret_cast<char*>(p), "node", 4) == 0) {
                p += 4;
                if (strncmp(reinterpret_cast<char*>(p), " trimesh ", 9) == 0) {
                    gotNode = 1;
                    sawTrimesh = 1;
                } else if (strncmp(reinterpret_cast<char*>(p), " dummy ", 7) == 0) {
                    gotNode = 0;
                    // The mesh-tag suffix selects which door state this .wok's action points
                    // belong to; an unrecognised suffix leaves `state` alone.
                    unsigned char* pTag = p + 11;
                    unsigned char* pSuffix = 0;
                    if (_strnicmp(reinterpret_cast<char*>(pTag), "pwk_use", 7) == 0)
                        pSuffix = pTag + 7;
                    else if (_strnicmp(reinterpret_cast<char*>(pTag), "pwk_dp_use_", 11) == 0)
                        pSuffix = pTag + 11;
                    if (pSuffix != 0) {
                        if (strncmp(reinterpret_cast<char*>(pSuffix), "01", 2) == 0)
                            state = 1;
                        else if (strncmp(reinterpret_cast<char*>(pSuffix), "02", 2) == 0)
                            state = 2;
                    }
                } else {
                    gotNode = 0;
                }
                continue;
            }

            if (strncmp(reinterpret_cast<char*>(p), "position", 8) == 0) {
                p += 8;
                static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%f %f %f", &posX, &posY, &posZ));
                if (state == 0) {
                    if (gotNode != 1) {
                        delete[] lineBuf;
                        return 0;
                    }
                    m_localOrigin.x = posX;
                    m_localOrigin.y = posY;
                    m_localOrigin.z = posZ;
                    state = 0;
                } else {
                    if (state != 1 && state != 2) {
                        delete[] lineBuf;
                        return 0;
                    }
                    // The two action-point handles are adjacent members, selected by state - 1.
                    m_headerVec[state - 1] = WorldToLocal(Vector(posX, posY, posZ));
                    state = 0;
                }
                continue;
            }

            if (gotNode != 1)
                continue;

            if (strncmp(reinterpret_cast<char*>(p), "endnode", 7) == 0) {
                gotNode = 0;
                state = 0;
                continue;
            }

            if (strncmp(reinterpret_cast<char*>(p), "orientation", 11) == 0) {
                p += 11;
                float axisX = 0.0f, axisY = 0.0f, axisZ = 0.0f;
                float angle;
                static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%f %f %f %f", &axisX, &axisY, &axisZ, &angle));
                if (axisX != 0.0f || axisY != 0.0f || axisZ != 0.0f) {
                    Quaternion rotation(Vector(axisX, axisY, axisZ), angle);
                    (void)rotation;
                }
            } else {

                int nVertsKw = strncmp(reinterpret_cast<char*>(p), "verts", 5);
                if (nVertsKw == 0 || strncmp(reinterpret_cast<char*>(p), "vertices", 8) == 0) {
                    if (nVertsKw == 0)
                        p += 5;
                    if (strncmp(reinterpret_cast<char*>(p), "vertices", 8) == 0)
                        p += 8;
                    int nVerts;
                    char ch;
                    static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%d %c", &nVerts, &ch));
                    SetVertexCount(nVerts);
                    if (nVerts > 0) {
                        for (int i = 0; i < nVerts; i++) {
                            if (LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) == 0)
                                return 0;
                            static_cast<void>(sscanf(reinterpret_cast<char*>(lineBuf), "%f %f %f", &vertX, &vertY, &vertZ));
                            m_paVertices[i].x = vertX;
                            m_paVertices[i].y = vertY;
                            m_paVertices[i].z = vertZ;
                        }
                    }
                } else if (strncmp(reinterpret_cast<char*>(p), "faces", 5) == 0) {
                    p += 5;
                    int nTris;
                    char ch;
                    static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%d %c", &nTris, &ch));
                    SetTriangleCount(nTris);
                    if (nTris > 0) {
                        for (int i = 0; i < nTris; i++) {
                            if (LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) == 0) {
                                delete[] lineBuf;
                                return 0;
                            }
                            // A face is three vertex indices plus a trailing attribute record.
                            int f0, f1, f2, f3, f4, f5, f6;
                            static_cast<void>(sscanf(reinterpret_cast<char*>(lineBuf), "%d %d %d %d %d %d %d %d", &f0, &f1, &f2, &f3,
                                   &f4, &f5, &f6, &m_paTriMaterials[i]));
                            m_paTriVertIndices[i * 3 + 0] = f0;
                            m_paTriVertIndices[i * 3 + 1] = f1;
                            m_paTriVertIndices[i * 3 + 2] = f2;
                        }
                    }
                }
            }

            gotNode = 1;
        } while (remaining != 0);
    }

    delete[] lineBuf;
    // A file with no trimesh node keeps the resource key alive by reading it back.
    if (sawTrimesh == 0)
        m_resRef.GetResRef();
    return 1;
}
