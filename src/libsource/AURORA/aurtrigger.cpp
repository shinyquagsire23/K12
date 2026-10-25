// PartTrigger, the Aurora trigger render part, and the free functions that drive it.

#ifndef K2_HOST_BUILD   // a host build may inline every other use of ~Part and emit none: use the inline one
#define PART_DTOR_DECLARED_ONLY  // ~Part is aurmodel.cpp's
#endif
#if KOTOR1
#define K1_AURORA_LIST  // this TU is K1's AURORA library (List.h grows from 8)
#endif
#include "aurtrigger.h"
#include "port/aslglapi.h"

#include "Vector.h"
#include "aurscene.h"
#include "aurglcore.h"
#include "AURORA/aurmodel.h"

// Segment probes owned by the math TU.
unsigned int BoxIntersect(const Vector& a, const Vector& b, const Vector& boxMin,
                          const Vector& boxMax);
unsigned int polyhit(const Vector* verts, int vertCount, const Vector& c, const Vector& d,
                     Vector* outPoint, Vector* outNormal);

// Scene-side add and remove, reached through the Scene subobject of the CAurScene argument.
void SceneBSPRemovePart(Scene* pScene, Part* pPart);
void SceneRemovePartFromRooms(Scene* pScene, Part* pPart);

class Model;
int SceneBSPAddPart(Scene* pScene, Part* pPart, Model* pModel);
void SceneAddPartToRooms(Scene* pScene, Part* pPart);

// Component-wise bounding-box helpers owned by the math TU.
Vector VectorMin(Vector a, Vector b);
Vector VectorMax(Vector a, Vector b);

void AurPartTriggerSetColor(PartTrigger* pPartTrigger, float* pfColor);

// Renderer-wide alpha offset added to every trigger's fill colour.
float parttriggeralpha;

// The GL entry points this file calls.


// In the translation tables, enable indices are GL capabilities, blend indices GL blend factors and
// prim type indices primitive modes.

// The trigger node every trigger part is built on.
MdlNodeTrigger global_nodetrigger(const_cast<char*>("trigger"));

PartTrigger::PartTrigger(MdlNode* pNode)
    : Part(pNode), m_bHidden(0), m_cHighlightGroup(0), m_color{1.0f, 1.0f, 1.0f, 0.5f},
      m_bPulse(0), m_fPulseRate(1.0f), m_bAdditive(0)
{
    __builtin_memset(&m_vBBoxMin, 0, sizeof(Vector) * 2);
}

// Builds the trigger: one PartTrigger and a hit block, the outline ring (each vertex optionally
// probed down against the scene), the bounding box from the ring, the colour, the owner, the fill
// list triangulation, then registration with the BSP tree or the room lists.
PartTrigger* AurPartTriggerCreate(CAurScene* pScene, Vector* paVertices, int nVertices, int bSkipGroundProbe,
                                  float* pafColor, void* pOwner)
{
    PartTrigger* pTrigger = new PartTrigger(&global_nodetrigger);
    CHitInfo* pHit = new CHitInfo(0, ~CHitInfo::kMaterialTrigger, 0);

    for (int i = 0; i < nVertices; i++)
    {
        if (bSkipGroundProbe == 0)
        {
            float x = paVertices[i].x;
            float y = paVertices[i].y;

            pHit->ClearOutput();
            pHit->m_bSkipObjects = 1;
            if (pScene->HitCheck(Vector(x, y, 1000.0f), Vector(x, y, -11.0f), &paVertices[i], pHit) == 0)
                pScene->HitCheck(Vector(x + 0.01f, y + 0.01f, 1000.0f),
                                 Vector(x + 0.01f, y + 0.01f, -11.0f), &paVertices[i], pHit);
            paVertices[i].z = paVertices[i].z + 0.01f;
        }

        pTrigger->m_listOutline.Add(paVertices[i]);
    }

    pTrigger->m_vBBoxMin = Vector(100000.0f, 100000.0f, 100000.0f);
    pTrigger->m_vBBoxMax = Vector(-100000.0f, -100000.0f, -100000.0f);
    for (int i = 0; i < pTrigger->m_listOutline.m_count; i++)
    {
        pTrigger->m_vBBoxMin = VectorMin(pTrigger->m_vBBoxMin, pTrigger->m_listOutline.m_pData[i]);
        pTrigger->m_vBBoxMax = VectorMax(pTrigger->m_vBBoxMax, pTrigger->m_listOutline.m_pData[i]);
    }

    AurPartTriggerSetColor(pTrigger, pafColor);
    pTrigger->m_pOwner = pOwner;

    List<int> listIndices;
    for (int i = 0; i < nVertices; i++)
        listIndices.Add(i);
    pTrigger->Decompose(listIndices);

    Scene* pRenderScene = static_cast<Scene*>(pScene);
    if (pRenderScene->m_pBSPRoot != 0)
        SceneBSPAddPart(pRenderScene, pTrigger, 0);
    else
        SceneAddPartToRooms(pRenderScene, pTrigger);

    pHit->~CHitInfo();
    operator delete(pHit);
    return pTrigger;
}

// Copies the trigger's four colour components as one block.
void AurPartTriggerSetColor(PartTrigger* pPartTrigger, float* pfColor)
{
    __builtin_memcpy(pPartTrigger->m_afColor, pfColor, sizeof(float) * 4);
}

// Removes a trigger's render part from the scene and optionally frees it. A scene with a BSP tree
// keeps its parts there, one without keeps them in the room lists. The delete is null-safe because
// the part handed over may not be owned here.
void AurPartTriggerDelete(CAurScene* pScene, PartTrigger* pPartTrigger, bool bDelete)
{
    Scene* pRenderScene = static_cast<Scene*>(pScene);

    if (pRenderScene->m_pBSPRoot != 0)
        SceneBSPRemovePart(pRenderScene, pPartTrigger);
    else
        SceneRemovePartFromRooms(pRenderScene, pPartTrigger);

    if (pPartTrigger != 0 && bDelete)
        delete pPartTrigger;
}

void AurPartTriggerReleaseMemory(PartTrigger* pPartTrigger)
{
    delete pPartTrigger;
}

void AurPartTriggerSetHighlight(PartTrigger* pPartTrigger, bool bHighlight, float fHeight, bool bAffectDynamic)
{
    pPartTrigger->SetHighlighted(bHighlight, fHeight, bAffectDynamic);
}

void AurPartTriggerGlobalAlpha(float fAlpha)
{
    parttriggeralpha = fAlpha;
}

// The class's own colour setter; the fourth component is the flat alpha.
void PartTrigger::SetHighlightColor(const Vector& vColor, float fAlpha)
{
    m_color[0] = vColor.x;
    m_color[1] = vColor.y;
    m_color[2] = vColor.z;
    m_color[3] = fAlpha;
}

// Geometric hit test of the trigger: the hit block's flag bit and the bounding box gate a triangle
// scan over the fill list, three vertices per probe. The hit point is written back into b and the
// triangle's plane normal into the hit block.
//
// The two out-cells are raw. The flagged one receives the block's own bit and the pointer cell
// receives the part's owner.
int PartTrigger::HitCheckGeom(const Vector& a, Vector& b, Vector* c, CHitInfo* h, float f)
{
    K2_UNUSED(f);
    if ((h->m_nMaterialMask & CHitInfo::kMaterialTrigger) == 0)
        return 0;
    if (!BoxIntersect(a, b, m_vBBoxMin, m_vBBoxMax))
        return 0;

    // i steps by three: three consecutive vertices make one triangle.
    for (int i = 0; i < m_listFillTriangles.m_count; i += 3)
    {
        unsigned int r = polyhit(&m_listFillTriangles.m_pData[i], 3, a, b, &b, &h->m_vHitNormal);
        if (c != 0 && r != 0)
        {
            *c = b;
            h->m_pHitTriggerOwner = m_pOwner;
            h->m_nHitMaterialBit = CHitInfo::kMaterialTrigger;
        }
        if (r != 0)
            return static_cast<int>(r);
    }
    return 0;
}

// Seeds the trigger's highlight state: the colour, whose fourth component is the flat alpha, the
// pulse latch and rate, and the blend selector.
void AurPartTriggerSetHighlightParams(PartTrigger* pPartTrigger, const Vector& vColor, float fAlpha,
                                      bool bPulse, float fPulseRate, bool bAdditive)
{
    pPartTrigger->m_color[0] = vColor.x;
    pPartTrigger->m_color[1] = vColor.y;
    pPartTrigger->m_color[2] = vColor.z;
    pPartTrigger->m_color[3] = fAlpha;
    pPartTrigger->m_bPulse = bPulse;
    pPartTrigger->m_fPulseRate = fPulseRate;
    pPartTrigger->m_bAdditive = bAdditive;
}

PartTrigger::~PartTrigger()
{
    Scene::RemoveFromSceneManager(this);
}

// Free math helpers owned by the math TU.
Vector VectorMin(List<Vector>& list);
Vector VectorMax(List<Vector>& list);

// Rebuilds the highlight prism from the ring in m_listOutline into three of the other lists. The
// rebuild is skipped unless the ring has at least two points and the flag is set, and the height is
// taken as a magnitude.
//
// m_listCageSides receives the ring lifted and lowered in one interleaved pass, top first, closed by
// re-appending its first two points; m_listCageTop receives the lifted ring and m_listCageBottom the
// lowered ring in reverse order. The bounding box comes from the pairs list, not from the ring.
// The three count resets keep the buffers, which are reused across rebuilds.
void PartTrigger::SetHighlighted(bool bFlag, float fHeight, bool bGroup)
{
    float fH = fHeight < 0.0f ? -fHeight : fHeight;

    m_bHidden = bFlag;
    m_cHighlightGroup = bGroup;
    m_fHeight = fH;

    if (m_listOutline.m_count < 2 || !bFlag)
        return;

    m_listCageSides.m_count = 0;
    m_listCageTop.m_count = 0;
    m_listCageBottom.m_count = 0;

    for (int i = 0; i < m_listOutline.m_count; i++) {
        Vector v = m_listOutline.m_pData[i];
        float fTop = v.z + m_fHeight;
        m_listCageSides.Add(Vector(v.x, v.y, fTop));
        m_listCageTop.Add(Vector(v.x, v.y, fTop));
        m_listCageSides.Add(Vector(v.x, v.y, fTop - (m_fHeight + m_fHeight)));
    }

    int nCount = m_listOutline.m_count;
    if (nCount != 0) {
        for (int i = nCount; i != 0; i--) {
            Vector v = m_listOutline.m_pData[i - 1];
            m_listCageBottom.Add(Vector(v.x + 0.0f, v.y + 0.0f, v.z - m_fHeight));
        }
    }

    m_listCageSides.Add(m_listCageSides.m_pData[0]);
    m_listCageSides.Add(m_listCageSides.m_pData[1]);

    m_vBBoxMin = VectorMin(m_listCageSides);
    m_vBBoxMax = VectorMax(m_listCageSides);
}

// Draws the highlight cage: three passes over three of the trigger's vertex lists, each with the
// same begin, vertices, end shape.
void PartTrigger::DrawVolume()
{
    unsigned int nPrimType = primTypeTranslationTable[4];
    ASLgl::glBegin(nPrimType);
    for (int i = 0; i < m_listCageSides.m_count; i++)
        ASLgl::glVertex3fv(reinterpret_cast<const float*>(&m_listCageSides.m_pData[i]));
    ASLgl::glEnd();

    nPrimType = primTypeTranslationTable[8];
    ASLgl::glBegin(nPrimType);
    for (int i = 0; i < m_listCageTop.m_count; i++)
        ASLgl::glVertex3fv(reinterpret_cast<const float*>(&m_listCageTop.m_pData[i]));
    ASLgl::glEnd();

    ASLgl::glBegin(nPrimType);
    for (int i = 0; i < m_listCageBottom.m_count; i++)
        ASLgl::glVertex3fv(reinterpret_cast<const float*>(&m_listCageBottom.m_pData[i]));
    ASLgl::glEnd();
}

// Draws the cage: the outline list between an attribute push and pop, then the fill list after a
// blend-mode selection.
void PartTrigger::Draw(bool)
{
    float afColor[4];
    afColor[0] = m_afColor[0];
    afColor[1] = m_afColor[1];
    afColor[2] = m_afColor[2];
    afColor[3] = m_afColor[3] + parttriggeralpha;
    if (afColor[3] == 0.0f)
        return;

    ASLgl::glPushAttrib(GL_LIGHTING_BIT | GL_TEXTURE_BIT);
    GLRender::Disable(Aurora::kCapBlend);
    GLRender::Disable(Aurora::kCapTexture2D);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glColor4fv(afColor);
    GLRender::BeginImmediatePrimitive(Aurora::kPrimLineLoop);
    for (int i = 0; i < m_listOutline.m_count; i++)
        ASLgl::glVertex3fv(reinterpret_cast<const float*>(&m_listOutline.m_pData[i]));
    ASLgl::glEnd();
    GLRender::Enable(Aurora::kCapBlend);

    if (m_bAdditive)
        GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendOne);
    else
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

    GLRender::BeginImmediatePrimitive(Aurora::kPrimTriangles);
    for (int i = 0; i < m_listFillTriangles.m_count; i++)
        ASLgl::glVertex3fv(reinterpret_cast<const float*>(&m_listFillTriangles.m_pData[i]));
    ASLgl::glEnd();
    ASLgl::glPopAttrib();
}

// Fill-list triangulator. Takes the index list by value, so every recursive call copies it.
//
// Picks the ring vertex with the minimum X and asks whether it and its two ring neighbours form an
// ear: every other vertex is probed with a downward ray against that triangle, whose Z values are
// zeroed first so the probe is 2D. A vertex the ray hits is inside the ear. With no hit the ear is
// accepted, its index leaves the ring and the triangle joins the fill list. With a hit the ring is
// split at the inside vertex and the tail, from the newly inserted entry through it and wrapping,
// is recursed on.
//
// The hit probe's ray end aliases its out-point, so the hit point overwrites the ray end; only the
// success flag is used here.
void PartTrigger::Decompose(List<int> listIndices)
{
    if (listIndices.m_count < 3)
        return;

    while (listIndices.m_count >= 3)
    {
        int nCount = listIndices.m_count;

        // 1. the ring vertex with the minimum X
        int nBest = -1;
        float fMinX = 1000000.0f;
        for (int i = 0; i < nCount; i++)
        {
            float fX = m_listOutline.m_pData[listIndices.m_pData[i]].x;
            if (fX < fMinX)
            {
                fMinX = fX;
                nBest = i;
            }
        }

        // The ring neighbours of the best vertex.
        int iPrev = listIndices.m_pData[(nBest + nCount - 1) % nCount];
        int iCur = listIndices.m_pData[nBest];
        int iNext = listIndices.m_pData[(nBest + nCount + 1) % nCount];

        // The candidate ear, flattened to z = 0 so the probe below is 2D.
        List<Vector> local;
        local.Add(m_listOutline.m_pData[iPrev]);
        local.Add(m_listOutline.m_pData[iCur]);
        local.Add(m_listOutline.m_pData[iNext]);
        for (int i = 0; i < local.m_count; i++)
            local.m_pData[i].z = 0.0f;

        // Any other ring vertex inside the ear?
        int nInside = -1;
        Vector vMin(1000000.0f, 1000000.0f, 1000000.0f);
        for (int k = 0; k < listIndices.m_count; k++)
        {
            int v = listIndices.m_pData[k];
            if (v == iNext || v == iPrev || v == iCur)
                continue;

            Vector vRay = m_listOutline.m_pData[v];
            vRay.z = 1000.0f;
            Vector vEnd = vRay;
            vEnd.z = -1000.0f;
            Vector vNormal;
            if (polyhit(local.m_pData, 3, vRay, vEnd, &vEnd, &vNormal) != 0)
            {
                if (vRay.x < vMin.x)
                {
                    vMin = vRay;
                    nInside = k;
                }
            }
        }

        // Accept the ear by removing its vertex, or split the ring at the inside vertex.
        int iFirst = iPrev;
        int nRecurse = -1;
        if (nInside < 0)
        {
            listIndices.DelIndex(nBest);
        }
        else
        {
            int v = listIndices.m_pData[nInside];
            iFirst = v;
            if (nBest + 1 >= listIndices.m_count)
            {
                listIndices.Add(v);
            }
            else
            {
                listIndices.Insert(v, nBest + 1);
                nRecurse = v;
            }
        }

        // The triangle joins the fill list.
        m_listFillTriangles.Add(m_listOutline.m_pData[iFirst]);
        m_listFillTriangles.Add(m_listOutline.m_pData[iCur]);
        m_listFillTriangles.Add(m_listOutline.m_pData[iNext]);

        // The split path recurses on the drained tail.
        if (nRecurse >= 0)
        {
            int k = 0;
            while (k < listIndices.m_count)
            {
                int v = listIndices.m_pData[k++];
                if (v == nRecurse)
                    break;
            }

            List<int> sub;
            for (;;)
            {
                int v = listIndices.m_pData[k];
                listIndices.DelIndex(k);
                sub.Add(v);
                if (v == nRecurse)
                    break;
            }

            Decompose(sub);
        }
    }
}

// MdlNodeTrigger, the trigger flavour of a parsed .mdl node. The unit holds two empty hooks, two
// that return zero, a short constructor and four file-scope trampolines.
//
// The trampolines re-type their void* receiver and forward. The Control trampoline reaches MdlNode's
// own implementation, since the trigger node adds no control override.
void MdlNodeTriggerParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeTrigger*>(pv)->InternalParseField(pszLine);
}

// A trigger node has no text-parseable fields, so it hides MdlNode's parser with an empty body: the
// keyword that reaches it is simply consumed.
void MdlNodeTrigger::InternalParseField(char* pszLine)
{
    K2_UNUSED(pszLine);
}

void MdlNodeTriggerControl(void* pv, Part* pPart, float fA, float fB, float fC)
{
    static_cast<MdlNode*>(pv)->InternalControl(pPart, fA, fB, fC);
}

// Same forwarding shape as the other node kinds; the body lives in MdlNode.cpp.
MdlNode* MdlNodeTriggerConJoinTwinMysLexia(void* pv)
{
    return static_cast<MdlNodeTrigger*>(pv)->InternalConJoin();
}

Part* MdlNodeTriggerCreateInstance(void* pv, Gob* pGob)
{
    return static_cast<MdlNodeTrigger*>(pv)->InternalCreateInstance(pGob);
}

// Builds the node named by the owning model's string table. A node's names live in its model's
// string list, indexed by m_nNameIndex; for an animation tree the table belongs to the animation's
// model, so the tree's tag word selects the path.
//
// The AsModel and AsAnimation calls are deliberate rather than a tag test, since both are virtual
// and the tree pointer is re-read on each.
static inline char* ListStringAt(List<char*>* pList, int nIndex)
{
    return pList->m_pData[nIndex];
}

__attribute__((noinline)) MdlNode* MdlNodeTrigger::InternalConJoin()
{
    return new MdlNodeTrigger(ListStringAt(
        &(m_pTree->AsModel() ? m_pTree->AsModel() : m_pTree->AsAnimation()->m_pModel)
             ->m_listStrings,
        m_nNameIndex));
}

// This kind builds no render part; the Gob pointer is the tree builder's and every
// InternalCreateInstance shares the one signature.
Part* MdlNodeTrigger::InternalCreateInstance(Gob* pGob)
{
    K2_UNUSED(pGob);
    return 0;
}

MdlNodeTrigger::MdlNodeTrigger(char* pszName) : MdlNode(pszName)
{
    m_nFlags = MDLNODE_TRIGGER_ID;
}
