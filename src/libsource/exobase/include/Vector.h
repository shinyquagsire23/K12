#ifndef VECTOR_H
#define VECTOR_H

#include "Quaternion.h"

// A float's bit pattern, for the engine's exact-colour tests.
inline int FloatBits(const float& f)
{
    int n;
    __builtin_memcpy(&n, &f, sizeof(n));
    return n;
}

// idiom-exempt: canonical
// 3-float point/direction primitive, embedded by value across the engine. Its default constructor
// zeroes all three components.
struct Vector {
    float x, y, z;

    Vector() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    void Normalize();
    int IsZero() const { return x == 0.0f && y == 0.0f && z == 0.0f; }
};

// Returns the input vector normalized to unit length.
Vector normalize(const Vector& v);

// Unit normal of the triangle (a, b, c).
Vector NormalOf(const Vector& a, const Vector& b, const Vector& c);

// Projects `p` onto the segment `a`->`b`.
Vector lineproject(const Vector& a, const Vector& b, const Vector& p);

// Projects `p` onto the infinite line through `a` and `b`.
Vector segmentproject(const Vector& a, const Vector& b, const Vector& p);

// Shortest-arc rotation taking `a` to `b`.
Quaternion RotationArc(Vector a, Vector b);

// Componentwise equality.
int equal(const Vector& a, const Vector& b);

// Euclidean length of the vector.
float magnitude(const Vector& v);

// Heading angle of the vector.
float Yaw(const Vector& v);

// Elevation angle of the vector.
float Pitch(const Vector& v);

inline Vector operator*(const Vector& v, float f) {
    Vector r = { v.x * f, v.y * f, v.z * f };
    return r;
}

inline Vector operator*(float f, const Vector& v) {
    Vector r = { f * v.x, f * v.y, f * v.z };
    return r;
}

// Divides by f through its reciprocal; a zero divisor returns the vector unchanged.
inline Vector operator/(const Vector& v, float f) {
    if (f == 0.0f)
        return v;
    float fInv = 1.0f / f;
    Vector r = { fInv * v.x, fInv * v.y, fInv * v.z };
    return r;
}

inline int operator!=(const Vector& a, const Vector& b) { return !equal(a, b); }

inline Vector operator-(const Vector& a, const Vector& b) {
    Vector r = { a.x - b.x, a.y - b.y, a.z - b.z };
    return r;
}

inline Vector operator+(const Vector& a, const Vector& b) {
    Vector r = { a.x + b.x, a.y + b.y, a.z + b.z };
    return r;
}

inline float dot(const Vector& a, const Vector& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector cross(const Vector& a, const Vector& b) {
    Vector r = { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    return r;
}

inline Vector operator-(const Vector& v) {
    Vector r = { -v.x, -v.y, -v.z };
    return r;
}

inline Vector Quaternion::xdir() const {
    Vector r = { 1.0f - 2.0f * (y * y + z * z), 2.0f * (y * x + z * w), 2.0f * (z * x - y * w) };
    return r;
}

inline Vector Quaternion::ydir() const {
    Vector r = { 2.0f * (x * y - w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z + x * w) };
    return r;
}

inline float Quaternion::angle() const { return __builtin_acosf(w) * 2.0f; }

inline Vector Quaternion::axis() const {
    Vector v = { x, y, z };
    float fAngle = angle();
    if ((fAngle < 0.0f ? -fAngle : fAngle) < 1.0e-7) {
        Vector r = { 1.0f, 0.0f, 0.0f };
        return r;
    }
    float fScale = static_cast<float>(1.0 / __builtin_sin(angle() * 0.5));
    Vector r = { v.x * fScale, v.y * fScale, v.z * fScale };
    return r;
}

inline Vector Quaternion::zdir() const {
    Vector r = { 2.0f * (x * z + w * y), 2.0f * (z * y - x * w), 1.0f - 2.0f * (x * x + y * y) };
    return r;
}

#endif
