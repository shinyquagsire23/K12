#ifndef QUATERNION_H
#define QUATERNION_H

// idiom-exempt: canonical
// The engine's 4-float orientation primitive, embedded by value across servercore and the GUI.
// Components are (w, x, y, z): the scalar comes first.
struct Vector;
class Matrix;

struct Quaternion {
    float w, x, y, z;

    Quaternion() { z = 0.0f; y = 0.0f; x = 0.0f; w = 1.0f; }
    Quaternion(Vector axis, float angle);
    Quaternion(float w_, float x_, float y_, float z_) { z = z_; y = y_; x = x_; w = w_; }

    // Rotation matrix, defined in Matrix.h where the complete Matrix type lives.
    Matrix getmatrix() const;
    // The matrix's rows: the rotated +X, +Y and +Z axes. Defined in Vector.h.
    Vector xdir() const;
    Vector ydir() const;
    Vector zdir() const;
    // The rotation angle in radians and its unit axis (+X for a near-zero angle). Defined in Vector.h.
    float angle() const;
    Vector axis() const;
#if KOTOR1  // out of line in K1's math3d.cpp
    Quaternion& operator*=(float f);
#endif
};

// Returns the input normalized to unit length.
Quaternion normalize(Quaternion q);

#if KOTOR1  // K1 keeps the product and the negated-w inverse out of line
Quaternion operator*(const Quaternion& a, const Quaternion& b);
Quaternion operator-(const Quaternion& q);
Quaternion operator*(const Quaternion& q, float f);
Quaternion operator+(const Quaternion& a, const Quaternion& b);
float dot(const Quaternion& a, const Quaternion& b);
#else
inline Quaternion operator*(const Quaternion& a, const Quaternion& b) {
    Quaternion r;
    r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return r;
}
#endif

// Euler-angle extraction and composition.
float Yaw(const Quaternion& q);
float Pitch(const Quaternion& q);
float Roll(Quaternion q);
Quaternion YawPitchRoll(float yaw, float pitch, float roll);

// Builds the quaternion that rotates +Z to point along `v`.
Quaternion pointto(const Vector& v);

// Spherical interpolation toward `qTo`.
Quaternion slerp(Quaternion qFrom, const Quaternion& qTo, float t);
Quaternion normalize(Quaternion q);

#endif
