#pragma once

#ifndef VERTEXPRIMITIVE_H
#define VERTEXPRIMITIVE_H

class VertexPrimitive;

struct Vector;
class MdlNode;
class VertexPrimitiveFlat;
class VertexPrimitiveSkinned;
class VertexPrimitiveAnim;
class VertexPrimitiveDangly;
class PartTriMesh;
class Part;

// The base of the vertex-primitive hierarchy holds a part's vertex arrays, shared by the flat,
// skinned, animated and dangly primitives. The whole hierarchy is compiled as one translation unit.
class VertexPrimitive {
public:

    VertexPrimitive(PartTriMesh* part);

    virtual ~VertexPrimitive();
    // The four probes return the derived subtype, or null when the object is not one of them.
    virtual VertexPrimitiveFlat* AsVertexPrimitiveFlat();
    virtual VertexPrimitiveSkinned* AsVertexPrimitiveSkinned();
    virtual VertexPrimitiveAnim* AsVertexPrimitiveAnim();
    virtual VertexPrimitiveDangly* AsVertexPrimitiveDangly();
    virtual void BuildVertexArrays() = 0;
    virtual void SetupVertexArraysStandard();
    virtual void SetupArrays() = 0;
    virtual unsigned char* SetupArraysVAR(unsigned char*) = 0;
    virtual unsigned char* ReconnectArraysVAR(unsigned char*) = 0;
    virtual void Rebuild();

    MdlNode* m_pNode;
    Part* m_pPart;
    int m_nArrayCount;
    int m_bStaticArrays;
    char m_bRebuildPending;
};

// A generic VertexPrimitive is not a flat instance, so the base probe returns null.
inline VertexPrimitiveFlat* VertexPrimitive::AsVertexPrimitiveFlat()
{
    return 0;
}

#endif // VERTEXPRIMITIVE_H

// The flat (unskinned, unanimated) triangle-mesh vertex primitive. It adds one member to the base
// layout: the fog-arrays pool handle.
#ifndef VERTEXPRIMITIVEFLAT_H
#define VERTEXPRIMITIVEFLAT_H

#include "aurglcore.h"

class PartTriMesh;
struct Vector;

class VertexPrimitiveFlat : public VertexPrimitive {
public:

    VertexPrimitiveFlat(PartTriMesh* part);

    virtual ~VertexPrimitiveFlat();

    virtual VertexPrimitiveFlat* AsVertexPrimitiveFlat();
    virtual void BuildVertexArrays();
    virtual void SetupArrays();
    virtual unsigned char* SetupArraysVAR(unsigned char*);
    virtual unsigned char* ReconnectArraysVAR(unsigned char*);
    virtual void UpdateLightVectors(Vector const&);
    virtual void UpdateHalfAngleVectors(Vector const&, Vector const&);
    virtual void UpdateFogValues(Vector const&, float);

    int m_nPoolHandle;
};

#endif // VERTEXPRIMITIVEFLAT_H

// The animated-mesh vertex primitive owns two vertex pools, for positions and for texture
// coordinates, because the anim arrays are rebuilt per frame.
#ifndef VERTEXPRIMITIVEANIM_H
#define VERTEXPRIMITIVEANIM_H


class PartTriMesh;

class VertexPrimitiveAnim : public VertexPrimitiveFlat {
public:

    VertexPrimitiveAnim(PartTriMesh* part);

    virtual ~VertexPrimitiveAnim();
    virtual VertexPrimitiveAnim* AsVertexPrimitiveAnim();
    virtual void BuildVertexArrays();
    virtual unsigned char* SetupArraysVAR(unsigned char*);

    int m_nVertexPool;
    int m_nTexCoordPool;
};

#endif // VERTEXPRIMITIVEANIM_H

// The dangly-mesh vertex primitive adds the per-vertex solver state array.
#ifndef VERTEXPRIMITIVEDANGLY_H
#define VERTEXPRIMITIVEDANGLY_H

#include "Vector.h"

class PartTriMesh;

class VertexPrimitiveDangly : public VertexPrimitiveFlat {
public:

    VertexPrimitiveDangly(PartTriMesh* part);

    virtual ~VertexPrimitiveDangly();
    virtual VertexPrimitiveDangly* AsVertexPrimitiveDangly();
    virtual void BuildVertexArrays();
    virtual unsigned char* SetupArraysVAR(unsigned char*);

    Vector* m_paVertices;
};

#endif // VERTEXPRIMITIVEDANGLY_H

// The skinned-mesh vertex primitive. It has no fields of its own; the skinned geometry is emitted
// by the skinned render family rather than through the flat array-building path.
#ifndef VERTEXPRIMITIVESKINNED_H
#define VERTEXPRIMITIVESKINNED_H


class PartSkin;
class Part;

class VertexPrimitiveSkinned : public VertexPrimitiveFlat {
public:

    VertexPrimitiveSkinned(PartSkin* part);

    virtual ~VertexPrimitiveSkinned();

    // An identity probe; a skinned primitive is the skinned instance.
    virtual VertexPrimitiveSkinned* AsVertexPrimitiveSkinned() { return this; }

    virtual void BuildVertexArrays();

    // Returns null and ignores the argument.
    virtual unsigned char* SetupArraysVAR(unsigned char*) { return 0; }

    virtual unsigned char* ReconnectArraysVAR(unsigned char*);
};

#endif // VERTEXPRIMITIVESKINNED_H

#ifndef NODEVERTEX_H
#define NODEVERTEX_H

// NodeVertex -- the per-vertex record the .mdl mesh kinds assemble. NodeVertexAnim and
// NodeVertexDangly extend the base record for the animated and dangly mesh kinds.
//
// The split is not a virtual hierarchy. The records carry no vtable pointer, and the two accessors
// dispatch on m_nType.
//
// The seven Vector arguments are the per-vertex values the mesh builders select out of their
// scratchpad lists: position, four texture-coordinate sets, an as-yet unnamed vector and the
// colour. An empty colours list passes a null pointer, which is why the seven-argument
// constructor seeds a white colour.

#include "List.h"

extern int vertexsize;

class NodeVertexAnim;
class NodeVertexDangly;

class NodeVertex {
public:
    NodeVertex();
    NodeVertex(Vector* pPos, Vector* pTex0, Vector* pTex1, Vector* pTex2, Vector* pTex3,
               Vector* pNormal, Vector* pColor);
    bool operator==(const NodeVertex& rhs);

    NodeVertexAnim* AsNodeVertexAnim();
    NodeVertexDangly* AsNodeVertexDangly();

    static const unsigned char NODEVERTEX_ANIM_ID = 1;
    static const unsigned char NODEVERTEX_DANGLY_ID = 2;
    static const unsigned char NODEVERTEX_SKINNED_ID = 3;   // no NodeVertexSkinned class exists

    Vector m_vPos;
    Vector m_vTex0;
    Vector m_vTex1;
    Vector m_vTex2;
    Vector m_vTex3;
    Vector m_vNormal;
    Vector m_vColor;
    unsigned char m_nTexCoordFlags;
    unsigned char m_nType;
};

class NodeVertexAnim : public NodeVertex {
public:
    NodeVertexAnim();
    NodeVertexAnim(Vector* pPos, Vector* pTex0, Vector* pTex1, Vector* pTex2, Vector* pTex3,
                   Vector* pNormal, Vector* pColor,
                   List<Vector>& l0, List<Vector>& l1, List<Vector>& l2);
    ~NodeVertexAnim();
    bool operator==(const NodeVertexAnim& rhs);

    List<Vector> m_lVertexFrames;
    List<Vector> m_lTexCoordFrames;
    List<Vector> m_lNormalFrames;
};

class NodeVertexDangly : public NodeVertex {
public:
    NodeVertexDangly();
    NodeVertexDangly(Vector* pPos, Vector* pTex0, Vector* pTex1, Vector* pTex2, Vector* pTex3,
                     Vector* pNormal, Vector* pColor, float fConstraint);
    bool operator==(const NodeVertexDangly& rhs);

    float m_fConstraint;
};

#endif
