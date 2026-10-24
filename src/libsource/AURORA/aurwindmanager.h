#ifndef WINDMANAGER_H
#define WINDMANAGER_H

// The engine's global wind field holds a list of registered emitters, a list of point-source cells
// and a pair of noise tiles that updateTimer alternates between. It has no vtable, and its
// SafePointerList base sits at offset 0. Note that m_fNoiseBlend is deliberately left uninitialized
// by both constructors; only updateTimer ever writes it.

#include "SafePointer.h"
#include "List.h"
#include "Vector.h"

class PartEmitter;

// One cell of the point-source wind field, with a position, a radius, a remaining lifetime and a
// strength.
class PointSourceWind {
public:
    PointSourceWind();
    PointSourceWind(Vector vPosition, float fRadius, float fLife, float fStrength);
    ~PointSourceWind();

    Vector m_vPosition;
    float m_fRadius;
    float m_fLife;
    float m_fStrength;
};

class WindManager : public SafePointerList<WindManager> {
public:
    WindManager();
    // Sets the wind and the deviation pair, with fC as the refresh period.
    WindManager(Vector vWind, float fA, float fB, float fC);
    ~WindManager();

    // The two entry points the rest of the engine uses. Scene::GetWindVector tail-calls
    // GetGlobalWind, and Scene::Update calls updateTimer per frame.
    Vector GetGlobalWind(Vector v, float f);
    void updateTimer(float fDeltaT);

    // PartEmitter's destructor calls DeregisterWindyEmitter directly rather than through a vtable,
    // which is why the class needs none.
    void RegisterWindyEmitter(PartEmitter* pEmitter);
    void DeregisterWindyEmitter(PartEmitter* pEmitter);

    // Stores the wind, then asks every registered emitter to recompute its bounding radius.
    void SetGlobalWind(Vector v);
    // The pair of floats the noise field scales by.
    void SetMaximumDeviation(float fA, float fB);
    // A bare tail call onto pointSourceWindVector; the float argument is unused.
    Vector GetGlobalPointWind(const Vector& v, float f);
    Vector pointSourceWindVector(Vector v);

    // The noise generator. The grid is a 16x16 float tile, and the pointer type is what lets
    // updateTimer swap the two tiles.
    void fillNoiseField(float (*pafField)[16]);
    void AddPointSourceWind(Vector v, float fRadius, float fLife, float fStrength);
    // The same noise field as GetGlobalWind, without the point-source half.
    Vector GetGlobalWindNoPoint(const Vector& v, float f);

    List<PointSourceWind> m_lPointSourceWinds;
    Vector m_vWind;
    Vector m_vFrameWind;
    float m_afNoiseTile1[16][16];
    float m_afNoiseTile2[16][16];
    float (*m_pafNoiseCurrent)[16];
    float (*m_pafNoiseNext)[16];
    float m_fMaxDeviationX;
    float m_fMaxDeviationY;
    float m_fNoiseBlend;
    float m_fNoiseTimer;
    float m_fNoisePeriod;
    List<PartEmitter*> m_lWindyEmitters;
};

#endif // WINDMANAGER_H
