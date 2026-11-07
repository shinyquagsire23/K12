#pragma once

// One vehicle's per-frame physics pair, the state the minigame and camera-relative drive
// integrators step: a velocity and a position, with the sum, scale and divide the Runge-Kutta
// steps combine them by.

#include "Vector.h"

struct CSWMGPhysicsState {
    Vector m_vVelocity;
    Vector m_vPosition;

    CSWMGPhysicsState() : m_vVelocity(), m_vPosition() {}
    CSWMGPhysicsState(const Vector& vVelocity, const Vector& vPosition)
        : m_vVelocity(vVelocity), m_vPosition(vPosition) {}
};

inline CSWMGPhysicsState operator+(const CSWMGPhysicsState& a, const CSWMGPhysicsState& b)
{
    return CSWMGPhysicsState(a.m_vVelocity + b.m_vVelocity, a.m_vPosition + b.m_vPosition);
}

inline CSWMGPhysicsState operator*(const CSWMGPhysicsState& s, float f)
{
    return CSWMGPhysicsState(s.m_vVelocity * f, s.m_vPosition * f);
}

// A zero divisor returns the zero state.
inline CSWMGPhysicsState operator/(const CSWMGPhysicsState& s, float f)
{
    if (f != 0.0f)
        return CSWMGPhysicsState(s.m_vVelocity / f, s.m_vPosition / f);
    return CSWMGPhysicsState();
}
