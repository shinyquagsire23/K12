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

// One RK4 step of the spring integration. Each k term holds the stage's acceleration and value
// times dt.
void CSWRK4Acceleration::Accelerate(Vector const& vVelocity, float fDeltaTime)
{
    CSWPhysicsState sStage, k1, k2, k3, k4;
    Vector vMidVelocity = (vVelocity + m_velocity) * 0.5f;
    CSWPhysicsState sSum, d1, d2, d3, d4;
    float fDt = fDeltaTime < 0.2f ? fDeltaTime : 0.2f;

    k1 = CSWPhysicsState(fDt * GetAccelerationVector(m_prev, m_velocity), fDt * m_prev.m_vValue);
    sSum = m_prev + k1;
    sStage = sSum * 0.5f;

    k2 = CSWPhysicsState(fDt * GetAccelerationVector(sStage, vMidVelocity), fDt * sStage.m_vValue);
    sSum = m_prev + k2;
    sStage = sSum * 0.5f;

    k3 = CSWPhysicsState(fDt * GetAccelerationVector(sStage, vMidVelocity), fDt * sStage.m_vValue);
    sStage = m_prev + k3;

    k4 = CSWPhysicsState(fDt * GetAccelerationVector(sStage, vVelocity), fDt * sStage.m_vValue);

    d1 = k1 / 6.0f;
    sSum = k2 / 3.0f;
    d3 = k3 / 3.0f;
    d4 = k4 / 6.0f;
    sSum = d1 + sSum + d3 + d4;
    m_cur = m_prev + sSum;
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
// subclass's m_fDeceleration, anything else is the base's formula with m_fAcceleration.
Vector CSWRK4SplitAcceleration::GetAccelerationVector(CSWPhysicsState& state, Vector const& vVelocity)
{
    if (vVelocity.IsZero())
        return vVelocity * m_fDeceleration - state.m_vValue * (m_fDeceleration / m_fMaxSpeed);
    return CSWRK4Acceleration::GetAccelerationVector(state, vVelocity);
}
