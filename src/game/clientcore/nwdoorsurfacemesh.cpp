// A door's collision mesh. m_doorType and m_doorState are deliberately left uninitialised by the
// constructor; SetDoorType fills them in.

#include "k2stdint.h"
#include "nwdoorsurfacemesh.h"
#include "exostring.h"
#include "restype.h"

const int CSWDoorSurfaceMesh::DOOR_STATE_CLOSED = 0;
const int CSWDoorSurfaceMesh::DOOR_STATE_OPEN1 = 1;
const int CSWDoorSurfaceMesh::DOOR_STATE_OPEN2 = 2;

#include "port/libcbase.h"
#include "port/libcstring.h"

// The file I/O for SaveBinaryMesh. <stdio.h> stays out so the fortified variants of the header's
// wrappers are not pulled in, and the whence values are spelled out for the same reason.
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

// The Windows-spelled case-insensitive compare the asset parsers use.
#include "port/crtapi.h"

CSWDoorSurfaceMesh::CSWDoorSurfaceMesh()
    : CSWCollisionMesh(RESTYPE_MDL, RESTYPE_DWK)
{
}

// The only cleanup is the base's.
CSWDoorSurfaceMesh::~CSWDoorSurfaceMesh()
{
}

// Loads the door's text .wok, the same format its placeable and room siblings use, with a door
// state machine on top. A `node trimesh _DWK_wg_<state>` block holds the walk geometry for one
// door state and only the block matching m_doorState is parsed; a
// `node dummy _DWK_dp_<state>_01/_02` block supplies that state's two action points and picks
// which of m_headerVecA and m_headerVecB the following position lines land in.
//
// Two oddities are deliberate. The loader always reports success, so a malformed or empty file just
// installs the default mesh. And a `_DWK_dp_` node whose suffix is neither _01 nor _02 leaves the
// state at its previous value, so a following position line can index past the pair.
int CSWDoorSurfaceMesh::LoadMeshText(unsigned char* pData, unsigned int size)
{
    unsigned char* lineBuf = new unsigned char[MESH_LINE_SIZE];
    unsigned char* cursor = pData;
    unsigned int remaining = size;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    Vector vert;             // one scratch shared by every verts line
    int doorState = -1;      // _01 -> 0, _02 -> 1; indexes the action-point pair
    int nodeDoorType = -1;   // m_doorType inside this door's own trimesh node, else -1
    int inTrimesh = 0;       // the current node was `trimesh` (not `dummy`)
    int sawNode = 0;         // some node handler ran; if none did, install the default mesh
    int gotNode = 0;

    if (size == 0) {
        delete[] lineBuf;
        LoadDefaultMesh();
        return 1;
    }

    do {
        if (LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) != 1)
            break;

        unsigned char* p = lineBuf;
        while (*p == ' ' || *p == '\t')
            p++;

        if (strncmp(reinterpret_cast<char*>(p), "node", 4) == 0) {
            p += 4;
            if (strncmp(reinterpret_cast<char*>(p), " trimesh ", 9) == 0) {
                p += 11;
                if (_strnicmp(reinterpret_cast<char*>(p), "_DWK_wg_", 8) == 0) {
                    unsigned char* pState = p + 8;
                    nodeDoorType = -1;
                    if (strncmp(reinterpret_cast<char*>(pState), m_doorState, 5) == 0)
                        nodeDoorType = m_doorType;
                    gotNode = 1;
                    inTrimesh = 1;
                    sawNode = 1;
                    continue;
                }
            }
            if (strncmp(reinterpret_cast<char*>(p), " dummy ", 7) == 0) {
                p += 9;
                if (_strnicmp(reinterpret_cast<char*>(p), "_DWK_dp_", 8) == 0) {
                    p += 8;
                    if (strncmp(reinterpret_cast<char*>(p), m_doorState, 5) == 0) {
                        nodeDoorType = m_doorType;
                        p += strlen(m_doorState);
                    }
                    if (strncmp(reinterpret_cast<char*>(p), "_01", 3) == 0)
                        doorState = 0;
                    else if (strncmp(reinterpret_cast<char*>(p), "_02", 3) == 0)
                        doorState = 1;
                    gotNode = 1;
                    inTrimesh = 0;
                    sawNode = 1;
                    continue;
                }
            }
            gotNode = 0;
        }

        if (strncmp(reinterpret_cast<char*>(p), "endnode", 7) == 0) {
            gotNode = 0;
            nodeDoorType = -1;
            continue;
        }

        if (gotNode != 1)
            continue;
        if (nodeDoorType == -1)
            continue;

        if (strncmp(reinterpret_cast<char*>(p), "position", 8) == 0) {
            p += 8;
            static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%f %f %f", &posX, &posY, &posZ));
            if (inTrimesh != 0) {
                m_localOrigin.x = posX;
                m_localOrigin.y = posY;
                m_localOrigin.z = posZ;
            } else {
                SetOpenLocation(static_cast<unsigned char>(doorState), Vector(posX, posY, posZ));
            }
            gotNode = 1;
            continue;
        }

        gotNode = 1;

        if (inTrimesh == 0)
            continue;

        if (strncmp(reinterpret_cast<char*>(p), "orientation", 11) == 0) {
            p += 11;
            Vector axis(0.0f, 0.0f, 0.0f);
            float angle;
            static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%f %f %f %f", &axis.x, &axis.y, &axis.z, &angle));
            if (axis.x != 0.0f || axis.y != 0.0f || axis.z != 0.0f) {
                Quaternion rotation(axis, angle);
                (void)rotation;
            }
        } else {
            // Either spelling of the vertex-count keyword.
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
                        if (LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) == 0) {
                            LoadDefaultMesh();
                            delete[] lineBuf;
                            return 1;
                        }
                        static_cast<void>(sscanf(reinterpret_cast<char*>(lineBuf), "%f %f %f", &vert.x, &vert.y, &vert.z));
                        m_paVertices[i] = vert;
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
                            return 1;
                        }
                        // A face stores three vertex indices plus a trailing record whose last
                        // field goes to m_paTriMaterials.
                        int f0, f1, f2, f3, f4, f5, f6, f7;
                        static_cast<void>(sscanf(reinterpret_cast<char*>(lineBuf), "%d %d %d %d %d %d %d %d", &f0, &f1, &f2, &f3,
                               &f4, &f5, &f6, &f7));
                        m_paTriMaterials[i] = f7;
                        m_paTriVertIndices[i * 3 + 0] = f0;
                        m_paTriVertIndices[i * 3 + 1] = f1;
                        m_paTriVertIndices[i * 3 + 2] = f2;
                    }
                }
            }
        }

        gotNode = 1;
    } while (remaining != 0);

    delete[] lineBuf;
    // No recognised node block means the file carries no geometry of its own.
    if (sawNode == 0)
        LoadDefaultMesh();
    return 1;
}

void CSWDoorSurfaceMesh::SetDoorType(int doorType) {
    m_doorType = doorType;
    switch (doorType) {
    case 0:
        strcpy(m_doorState, "closed");
        break;
    case 1:
        strcpy(m_doorState, "open1");
        break;
    case 2:
        strcpy(m_doorState, "open2");
        break;
    default:
        strcpy(m_doorState, " ");
        break;
    }
}

// Writes the whole .dwk through the base, then reopens it and patches the header's two unused
// vectors with this door's action points in world space. Those fields are the only persistent
// storage for the action points, which LoadMeshText reads out of the .wok's `_DWK_dp_` blocks.
// The header's transform block is zeroed before the read, and the open is not checked.
int CSWDoorSurfaceMesh::SaveBinaryMesh(const CExoString& fileName)
{
    int result = CSWCollisionMesh::SaveBinaryMesh(fileName);
    if (result == 1) {
#if KOTOR1
        FILE* pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/game/clientcore/nwdoorsurfacemesh.cpp",
                             "SaveBinaryMesh", 373, fileName.CStr(), "rb+");
#else
        FILE* pFile = fopen(fileName.CStr(), "rb+");
#endif
        CResBWMHeader header;
        memset(&header.vecA, 0, sizeof(Vector) * 5);
        fread(&header, sizeof(header), 1, pFile);
        fseek(pFile, 0, SEEK_END);
        header.vecC = LocalToWorld(m_headerVec[0]);
        header.vecD = LocalToWorld(m_headerVec[1]);
        fseek(pFile, 0, SEEK_SET);
        fwrite(&header, sizeof(header), 1, pFile);
        fclose(pFile);
    }
    return result;
}

// Reads back the two action-point vectors that SaveBinaryMesh patched into the header's unused
// fields, the .dwk counterpart of LoadMeshText's `_DWK_dp_` arms. Each is taken only when it is
// non-zero, a zero meaning that no action points were saved. The base's result is ignored and this
// override always reports success.
int CSWDoorSurfaceMesh::LoadMeshBinary(CResBWM* pRes)
{
    CSWCollisionMesh::LoadMeshBinary(pRes);
    if (pRes->m_pParsed != 0) {
        Vector vecC = pRes->m_pParsed->vecC;
        if (vecC.x != 0.0f || vecC.y != 0.0f || vecC.z != 0.0f)
            m_headerVec[0] = vecC;
    }
    if (pRes->m_pParsed != 0) {
        Vector vecD = pRes->m_pParsed->vecD;
        if (vecD.x != 0.0f || vecD.y != 0.0f || vecD.z != 0.0f)
            m_headerVec[1] = vecD;
    }
    return 1;
}

// Builds the resource name of this door's binary walkmesh, the resref followed by the door type,
// with no extension. The door and level-stat helpers build the .dwk file name from it.
void CSWDoorSurfaceMesh::GetResourceForBinaryWalkMesh(CResRef& out)
{
    CExoString resName;
    CExoString fileName;
    m_resRef.CopyToString(resName);
    fileName.Format(const_cast<char*>("%s%d"), resName.CStr(), m_doorType);
    out = fileName;
}
