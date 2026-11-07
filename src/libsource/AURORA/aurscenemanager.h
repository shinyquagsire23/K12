#pragma once
#include "k2stdint.h"

#ifndef BUCKET_H
#define BUCKET_H

class PartTriMesh;

#include "List.h"

// Value of the renderer's three per-texture bucket maps (mesh, fade and cap).
class Bucket {
public:
    __attribute__((used)) ~Bucket() {}

    List<PartTriMesh*> m_parts;
};

#endif

#include "List.h"

class CAurTriangleBin;
class Gob;
class Part;
class PartTrigger;
class PartAABB;
class PartBeam;
class PartEmitter;
class PartLight;
class PartShadow;
class PartTriMesh;
class TileData;

// Per-leaf payload of the scene BSP: the lists of things currently inside that cell. Every
// BSPNode, and therefore every CAurRoom, owns one.
struct nodedata {
    List<PartLight*> m_lights;
    List<Gob*> m_gobs;
    List<PartTriMesh*> m_meshes;
    List<PartEmitter*> m_emitters;
    List<PartShadow*> m_shadows;
    List<PartBeam*> m_beams;
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
