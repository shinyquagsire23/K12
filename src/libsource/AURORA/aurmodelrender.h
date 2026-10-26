#pragma once

#ifndef PARTPROJECTION_H
#define PARTPROJECTION_H

#include "AURORA/aurmodel.h"

// The planar-projection shadow receiver part. It is abstract, since the Draw slot is re-pured here,
// so the body comes from a concrete subclass.
class PartProjection : public Part {
public:

    virtual void Draw(bool b) = 0;

    virtual PartProjection* AsPartProjection() { return this; }
    virtual void* AsPartDummy() { return 0; }

    virtual void Init();
    void SetupVertexArrays();

    ~PartProjection();

    PartProjection(PartTriMesh* pTriMesh, PartLight* pLight);

    static int m_nNextProjectionID;

    PartTriMesh* m_pTriMesh;
    PartLight*   m_pLight;
    Vector m_vBBoxMin;
    Vector m_vBBoxMax;
    int  m_nVertexPool = -1;
    int  m_nVertexCount = 0;
    int  m_nIndexPool = -1;
    int  m_nQuadCount = 0;
    int   m_nProjectionId;
};

K2_LAYOUT_ASSERT(sizeof(PartProjection) == 0xa8, "PartProjection size drift");

#endif

#ifndef PARTBEAM_H
#define PARTBEAM_H

// The beam variant of the projection part.
class PartBeam : public PartProjection {
public:
    PartBeam(PartTriMesh* pTriMesh, PartLight* pLight) : PartProjection(pTriMesh, pLight) {}

    virtual void* AsPartBeam() { return this; }
    virtual void* AsPartDummy() { return 0; }
    virtual void Draw(bool);
};

#endif

#ifndef PARTSHADOW_H
#define PARTSHADOW_H

// The planar shadow variant of the projection part.
class PartShadow : public PartProjection {
public:
    PartShadow(PartTriMesh* pTriMesh, PartLight* pLight) : PartProjection(pTriMesh, pLight) {}

    virtual void* AsPartShadow() { return this; }
    virtual void* AsPartDummy() { return 0; }
    virtual void Draw(bool);
};

#endif
