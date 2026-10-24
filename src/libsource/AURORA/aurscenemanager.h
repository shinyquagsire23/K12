#pragma once
#include "k2stdint.h"

#ifndef BUCKET_H
#define BUCKET_H

class PartTriMesh;

// Value of the renderer's three per-texture bucket maps (mesh, fade and cap).
class Bucket {
public:
    __attribute__((used)) ~Bucket() { delete[] m_paParts; }

    void allocate(int nNewCapacity) {
        PartTriMesh** paOld = m_paParts;
        m_nCapacity = nNewCapacity;
        m_paParts = new PartTriMesh*[nNewCapacity];
        K2_ASSUME(m_nCount <= nNewCapacity);   // Add grows a full bucket to twice its count
        for (int64_t i = 0; i < m_nCount; i++)
            m_paParts[i] = paOld[i];
        delete[] paOld;
    }

    void Add(PartTriMesh* pPart) {
        if (m_nCount == m_nCapacity)
#if KOTOR1 && defined(K1_AURORA_LIST)  // K1 grows an empty bucket to 8
            allocate(m_nCount ? m_nCount * 2 : 8);
#else
            allocate(m_nCount ? m_nCount * 2 : 16);
#endif
        m_paParts[m_nCount++] = pPart;
    }

    PartTriMesh** m_paParts;
    int m_nCount;
    int m_nCapacity;
};

#endif

#include "List.h"

class CAurTriangleBin;
class Gob;
class Part;
class PartTrigger;
class PartAABB;
class PartEmitter;
class PartLight;
class PartTriMesh;
class TileData;

// Per-leaf payload of the scene BSP: the lists of things currently inside that cell. Every
// BSPNode, and therefore every CAurRoom, owns one.
struct nodedata {
    List<Part*> m_lights;
    List<Gob*> m_gobs;
    List<PartTriMesh*> m_meshes;
    List<PartEmitter*> m_emitters;
    List<Part*> m_shadows;
    List<Part*> m_beams;
    List<Part*> m_otherParts;
    List<PartTrigger*> m_parts;
    List<CAurTriangleBin*> m_bins;
    PartAABB* m_pPartAABB = 0;
    TileData* m_pTileData = 0;
    List<Gob*>        m_movingObjects;
    List<PartLight*>  m_movingLights;
    List<PartEmitter*> m_movingEmitters;

    nodedata();
#if KOTOR1  // K1 exposes the per-frame lists through accessors; K2 touches the Lists directly
    void ClearMovingObjects();
    void ClearMovingLights();
    void ClearMovingEmitters();
    void AddMovingObject(Gob* pGob);
    void AddMovingLight(PartLight* pLight);
    void AddMovingEmitter(PartEmitter* pEmitter);
    List<Gob*>* GetMovingObjects();
    List<PartLight*>* GetMovingLights();
    List<PartEmitter*>* GetMovingEmitters();
#endif
    ~nodedata();
};

K2_LAYOUT_ASSERT(sizeof(nodedata) == 0xd0, "nodedata size drift");
