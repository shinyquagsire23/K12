#include "SWAccelerationControlers.h"

// A critically damped spring acceleration model and its RK4 integrator.
CSWRK4Acceleration::CSWRK4Acceleration()
    : m_fMaxSpeed(100.0f), m_fAcceleration(100.0f)
{
}

CSWRK4Acceleration::CSWRK4Acceleration(float a, float b)
    : m_fMaxSpeed(a), m_fAcceleration(b)
{
}

// Damped spring acceleration pulling the value back toward zero.
Vector CSWRK4Acceleration::GetAccelerationVector(CSWPhysicsState& state, Vector const& vVelocity)
{
    return vVelocity * m_fAcceleration - state.m_vValue * (m_fAcceleration / m_fMaxSpeed);
}

void CSWRK4Acceleration::SetVelocity(float velocity)
{
    m_prev = m_cur;
    m_cur.m_vDisplacement.x = 0;
    m_cur.m_vDisplacement.y = 0;
    m_cur.m_vDisplacement.z = 0;
    m_cur.m_vValue.x = 0;
    m_cur.m_vValue.y = velocity;
    m_cur.m_vValue.z = 0;
}

// One RK4 step of the spring integration.
void CSWRK4Acceleration::Accelerate(Vector const& vVelocity, float fDeltaTime)
{
    // The value pairs integrate each stage's k term times dt.
    CSWPhysicsState sStage = {};
    Vector vMidVelocity = (vVelocity + m_velocity) * 0.5f;
    float fDt = __builtin_fminf(fDeltaTime, 0.2f);

    Vector k1v = GetAccelerationVector(m_prev, m_velocity) * fDt;
    Vector k1u = m_prev.m_vValue * fDt;
    sStage.m_vValue = (m_prev.m_vValue + k1v) * 0.5f;
    sStage.m_vDisplacement = (m_prev.m_vDisplacement + k1u) * 0.5f;

    Vector k2v = GetAccelerationVector(sStage, vMidVelocity) * fDt;
    Vector k2u = sStage.m_vValue * fDt;
    sStage.m_vValue = (m_prev.m_vValue + k2v) * 0.5f;
    sStage.m_vDisplacement = (m_prev.m_vDisplacement + k2u) * 0.5f;

    Vector k3v = GetAccelerationVector(sStage, vMidVelocity) * fDt;
    Vector k3u = sStage.m_vValue * fDt;
    sStage.m_vValue = m_prev.m_vValue + k3v;
    sStage.m_vDisplacement = m_prev.m_vDisplacement + k3u;

    Vector k4v = GetAccelerationVector(sStage, vVelocity) * fDt;
    Vector k4u = sStage.m_vValue * fDt;

    const float fSixth = 1.0f / 6.0f;
    const float fThird = 1.0f / 3.0f;
    m_cur.m_vValue = m_prev.m_vValue + (k1v * fSixth + k2v * fThird + k3v * fThird + k4v * fSixth);
    m_cur.m_vDisplacement = m_prev.m_vDisplacement + (k1u * fSixth + k2u * fThird + k3u * fThird + k4u * fSixth);
    m_velocity = vVelocity;
}

// Advances the state: four per-slot reset flags, the m_cur to m_prev copy, then the RK4 step. Each
// flag zeroes exactly one vector, so they stay separate assignments.
CSWPhysicsState CSWRK4Acceleration::Update(Vector vVelocity, float fDeltaTime,
                                           int a, int b, int c, int d)
{
    if (c)
        m_cur.m_vDisplacement = Vector(0.0f, 0.0f, 0.0f);
    if (d)
        m_cur.m_vValue = Vector(0.0f, 0.0f, 0.0f);

    m_prev = m_cur;

    if (a)
        m_prev.m_vDisplacement = Vector(0.0f, 0.0f, 0.0f);
    if (b)
        m_prev.m_vValue = Vector(0.0f, 0.0f, 0.0f);

    Accelerate(vVelocity, fDeltaTime);
    return m_cur;
}


CSWRK4SplitAcceleration::CSWRK4SplitAcceleration()
    : CSWRK4Acceleration(100.0f, 100.0f), m_fDeceleration(100.0f)
{
}

CSWRK4SplitAcceleration::CSWRK4SplitAcceleration(float a, float b, float c)
    : CSWRK4Acceleration(a, b), m_fDeceleration(c)
{
}

// Applies the base formula with the damping term switched: a velocity of exactly (0, 0, 0) uses the
// subclass's m_fDeceleration, anything else the base's m_fAcceleration.
Vector CSWRK4SplitAcceleration::GetAccelerationVector(CSWPhysicsState& state, Vector const& vVelocity)
{
    float fDamping = m_fAcceleration;
    if (vVelocity.x == 0.0f && vVelocity.y == 0.0f && vVelocity.z == 0.0f) {
        fDamping = m_fDeceleration;
    }
    return vVelocity * fDamping - state.m_vValue * (fDamping / m_fMaxSpeed);
}
