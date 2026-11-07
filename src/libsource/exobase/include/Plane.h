#ifndef PLANE_H
#define PLANE_H

// idiom-exempt: canonical
// Plane primitive of four floats, `dot(normal, p) + d`: an unnormalized normal followed by the
// signed offset. Default-initialization zeroes it.

#include "Vector.h"

struct Quaternion;

struct Plane {
    Vector normal;
    float d;

    Plane() : normal(0.0f, 0.0f, 0.0f), d(0.0f) {}
    Plane(float _a, float _b, float _c, float _d) : normal(_a, _b, _c), d(_d) {}

    // Rotates the normal by `q` and re-derives `d` for the translated frame, in place.
    void Transform(const Vector& v, const Quaternion& q);
};

#endif // PLANE_H
