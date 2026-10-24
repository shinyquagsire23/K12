#ifndef PLANE_H
#define PLANE_H

// idiom-exempt: canonical
// Plane primitive of four floats, `a*x + b*y + c*z + d`, an unnormalized normal followed by the
// signed offset. Default-initialization zeroes it.

struct Vector;
struct Quaternion;

struct Plane {
    float a, b, c, d;

    Plane() : a(0.0f), b(0.0f), c(0.0f), d(0.0f) {}
    Plane(float _a, float _b, float _c, float _d) : a(_a), b(_b), c(_c), d(_d) {}

    // Rotates the normal by `q` and re-derives `d` for the translated frame, in place.
    void Transform(const Vector& v, const Quaternion& q);
};

#endif // PLANE_H
