#pragma once

#ifndef CUSTOMANIMATION_H
#define CUSTOMANIMATION_H

class MaxTree;
struct AnimRun;

// CustomAnimation: the abstract base of the look-at machinery: the deactivation latch, the built
// animation tree and the live play-state record.
class CustomAnimation {
public:
    CustomAnimation();

    // The tree is destroyed as MaxTree, so ~MaxTree is kept out of line in this translation unit
    // (MAXTREE_KEEP_DTOR_OUT_OF_LINE).
    virtual ~CustomAnimation();  // slots 0-1 (D2+D0)
    virtual bool IsActive() = 0;
    virtual void Deactivate() = 0;
    virtual void Control() = 0;

    char m_bDeactivated;
    Animation* m_pAnimation;         // torn down as MaxTree
    AnimRun* m_pRun;                 // the live play-state record
};

#endif // CUSTOMANIMATION_H

#ifndef LOOKATANIMATION_H
#define LOOKATANIMATION_H

#include "SafePointer.h"

class CAurObject;
class Gob;
class Part;
class Animation;
struct AnimRun;

// LookAtAnimation: the head and eye-tracking CustomAnimation subclass. Its own fields start at the
// watched object; the two key pointers at the end point into this animation's own key buffer.
class LookAtAnimation : public CustomAnimation {
public:
    // Declared only, so that the inliner cannot fold this into its caller.
    LookAtAnimation(CAurObject* pObject, Part* pPart, Gob* pGob, Part* pPart2,
                    float fArcH, float fArcV, float f3, float f4);

    virtual ~LookAtAnimation();  // slots 0-1 (D2+D0)
    virtual bool IsActive();
    virtual void Deactivate();
    virtual void Control();
    virtual void BuildAnimation();

    SafePointer<CAurObject> m_pObject;  // the watched object
    Gob*  m_pGob;
    Part* m_pTargetPart;             // the looked-at part
    Part* m_pSourcePart;
    float m_fArcH;                   // horizontal arc limit
    float m_fArcV;                   // vertical arc limit
    float m_fTransTime;
    float m_fMaxDistance;
    Quaternion* m_pGoalKey;         // the identity, or the composed look-at angle
    Quaternion* m_pSourceKey;       // the source part's local orientation
    char m_bReleaseStarted;
};

#endif // LOOKATANIMATION_H

// CallBack: one entry of Gob's animation-event callback list. Non-polymorphic and trivially
// destructible: Gob::DeregisterCallback deletes one with a bare operator delete, so no destructor is
// declared here.

class CAurObject;

class CallBack {
public:
    CallBack();
    CallBack(void (*pfn)(CAurObject*, char*, void*), char* pszName, void* pUserData,
             float fTime, bool b);

    char  m_szName[32];
    void (*m_pfnCallback)(CAurObject*, char*, void*);
    void* m_pUserData;
    float m_fTime;                                         // seconds; defaults to 10.0f
    bool  m_bFlag;
};

// NewController: one animation controller attached to a .mdl node, and the element type of
// MdlNode::m_listControllers. Only the two entry points are declared here; the node's control loop
// needs nothing beyond those and the element stride.

struct Quaternion;
struct Vector;

// Gob::DoEvent("ReportKey", ...)'s stack payload: the sampled key time and the two candidate
// values, plus which of them the controller actually carries.
struct NewControllerKeyReport {
    float       m_fAlpha;
    Vector*     m_pVector;
    Quaternion* m_pQuaternion;
    bool        m_bQuaternion;
};
class MdlNode;
class Part;

class NewController {
public:
    // One control tick: evaluate the controller's keys against the node's animation time and
    // write the result onto the part.
    void Control(Part* pPart, MdlNode* pNode, float fA, float fB, float fC);
    // The AnimRun::m_pTraceRun tracing path: the same receiver and arguments as Control, but only
    // two floats, because the controller reports its keys rather than applying them.
    void ReportKeys(Part* pPart, MdlNode* pNode, float fA, float fB);

    // Locate the key the given time falls in: nIndex counts the leading key times below fTime and
    // fAlpha is the fractional position inside that span. The key times are the node's floats.
    void GetKeyIndexAndAlpha(float fTime, MdlNode* pNode, int& nIndex, float& fAlpha);
    // Component readers: each returns true when it wrote pValue, false when the controller's kind
    // does not supply that component. GetQuaternionValue defers to GetQuaternionFromIndexLocation.
    bool GetFloatValue(float fTime, MdlNode* pNode, float& fValue);
    int  GetVectorValue(float fTime, MdlNode* pNode, Vector& vValue);
    int  GetQuaternionValue(float fTime, MdlNode* pNode, Quaternion& qValue);
    void GetQuaternionFromIndexLocation(int nKey, MdlNode* pNode, Quaternion& qValue);
    void ReportKey(Part* pPart, MdlNode* pNode, int nKey, float fAlpha);

    // m_keys.m_nKind: the key layout in the low nibble, and the bezier flag in the high one.
    enum {
        kKeyFloat = 1,
        kKeyPackedQuaternion = 2,
        kKeyVector = 3,
        kKeyQuaternion = 4,
        kKeyLayoutMask = 0xf,
        kKeyFlagsMask = 0xf0,
        kKeyBezier = 0x10,
    };

    // m_target.m_nOffset carries kTargetUnconverted while it is still the PC part record's offset
    // (the controller type); GetOffset64 converts it to this build's Part layout.
    enum {
        kTargetUnconverted = 0xf000000,
    };
    enum : unsigned int {
        kTargetOffsetMask = 0xf0ffffff,
    };

    // Packed controller state, built by AddGenericControllerSingle (aurparse.cpp). The readers load
    // the fields below rather than the raw words; the unions exist because the packer writes whole
    // words.
    union {
        unsigned long long m_nTargetPacked;
        struct {
            int            m_nOffset;      // the target field's offset in the part (signed)
            short          m_nDefaultOffset;        // -1 means "no second offset"
            unsigned short m_nKeyCount;    // keys carried by this controller
        } m_target;
    };
    union {
        unsigned long long m_nKeysPacked;
        struct {
            unsigned short m_nTimeIndex;   // first key time in the node's float list
            unsigned short m_nValueIndex;  // first key value (the times are m_nKeyCount floats)
            unsigned char  m_nKind;        // low nibble is the kind; the high bit is the bezier arm
            unsigned char  m_nReserved1;
            unsigned short m_nReserved2;
        } m_keys;
    };
};
