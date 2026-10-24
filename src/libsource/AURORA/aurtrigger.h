#pragma once

#ifndef PARTTRIGGER_H
#define PARTTRIGGER_H

#include "AURORA/aurmodel.h"
#include "List.h"

class CHitInfo;
class MdlNode;
struct Vector;

class PartTrigger : public Part {
public:

    PartTrigger(MdlNode* pNode);

    virtual ~PartTrigger();
    virtual int HitCheckGeom(Vector const&, Vector&, Vector*, CHitInfo*, float);
    virtual void* AsPartTrigger() { return this; }
    virtual void Draw(bool);
    virtual void DrawVolume();

    float m_afColor[4];
    Vector m_vBBoxMin;
    Vector m_vBBoxMax;
    List<Vector> m_listOutline;
    List<Vector> m_listCageSides;
    List<Vector> m_listCageTop;
    List<Vector> m_listCageBottom;
    void* m_pOwner;
    bool m_bHidden;
    char _padd9[0xdc - 0xd9];
    float m_fHeight;
    char m_cHighlightGroup;
    char _pade1[0xe4 - 0xe1];
    float m_color[4];
    bool m_bPulse;
    char _padf5[0xf8 - 0xf5];
    float m_fPulseRate;
    bool m_bAdditive;
    char _padfd[0x100 - 0xfd];
    List<Vector> m_listFillTriangles;

    void SetHighlighted(bool bFlag, float fHeight, bool bGroup);
    void SetHighlightColor(const Vector& vColor, float fAlpha);
    void Decompose(List<int> listIndices);
};

#endif // PARTTRIGGER_H

#ifndef MDLNODETRIGGER_H
#define MDLNODETRIGGER_H


class Gob;
class Part;

class MdlNodeTrigger : public MdlNode {
public:
    MdlNodeTrigger(char* pszName);

    void InternalParseField(char* pszLine);

    Part* InternalCreateInstance(Gob* pGob);

    MdlNode* InternalConJoin();
};

#endif // MDLNODETRIGGER_H
