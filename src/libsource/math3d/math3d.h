#ifndef MATH3D_H
#define MATH3D_H
#include "k2stdint.h"

// The engine's free math functions, plus the Matrix and Quaternion helpers that Vector.h and
// Matrix.h do not already declare.

#include "Vector.h"
#include "Plane.h"
#include "Matrix.h"

template <class T> class List;

// Clamp to [0, 1], the alpha and fog blend helper.
float clampf(float v);

// Degrees to radians.
float DegToRad(float degrees);

// 32-entry quarter-wave lookup tables, indexed as (int)(angle / (2*pi)) & 31.
float cosineLUT(float fAngle);
float sineLUT(float fAngle);

// Round v to the nearest multiple of step; ties go toward negative infinity.
float Round(float v, float step);

// Index of the smallest element, 0 for n < 2.
int argmin(float* a, int n);

// Componentwise min and max of two vectors.
Vector VectorMin(Vector a, Vector b);
Vector VectorMax(Vector a, Vector b);

// Answers an arbitrary unit vector perpendicular to v, the cross with the X axis, falling back to
// the Y axis when v is parallel to X, then normalized.
Vector Orth(const Vector& v);

// Unit normal of the plane spanned by a and b, i.e. normalize(a x b).
Vector Orth(const Vector& a, const Vector& b);

// Population count of the low byte.
unsigned char bitcount(unsigned char v);

// Answers how far the box reaches along the plane's normal, the plane equation evaluated at the
// corner furthest along it. Positive means the box is above the plane.
float BoxAbovePlane(const Vector& v1, const Vector& v2, const Plane& plane);

// The sphere twin of BoxAbovePlane, answering the plane equation at the centre minus the radius.
float SphereAbovePlane(const Vector& vSphere, float fRadius, const Plane& plane);

// Componentwise bounds over a vertex list.
Vector VectorMax(List<Vector>& list);
Vector VectorMin(List<Vector>& list);

// Index of the element whose dot product with vDir is largest; 0 for an empty list.
int VectorMaxDir(List<Vector>& list, Vector vDir);

// In-place list transforms.
void Translate(List<Vector>& list, Vector vOffset);
void Scale(List<Vector>& list, float fScale);

// Area of the closed vertex fan around list[0].
float ComputeArea(List<Vector>& list);

// Solves the plane equation for z, answering how far the point sits along z above the plane
// dot(vNormal,x) + fDist == 0, or 0 for a vertical normal.
float PlaneHeightAtPoint(const Vector& vNormal, float fDist, const Vector& vPoint);

// The point projected onto the plane along its normal.
Vector planeproject(const Vector& vNormal, float fDist, const Vector& vPoint);

// True when the vector's length is within 1% of 1.
bool isnormalized(const Vector& v);

// Where the line through the two points crosses the plane; t is 0, so the result is the line start,
// when the line is parallel to the plane.
Vector planelineintersection(const Vector& vNormal, float fDist, const Vector& vLineStart,
                             const Vector& vLineEnd);

// The segment twin computes the same intersection with t clamped to [0, 1], so the result stays on
// the segment.
Vector planesegmentintersection(const Vector& vNormal, float fDist, const Vector& vLineStart,
                                const Vector& vLineEnd);

// The 3x3 determinant.
float Determinant(Matrix m);

// The infinity norm, the largest absolute element.
float norm(const Matrix& m);

// The adjugate-over-determinant 3x3 inverse; the result is a function-local static Matrix.
Matrix Inverse(Matrix m);

// Inverse of the upper-left 2x2 block of an otherwise-identity 3x3.
Matrix inverse2(Matrix m);

// Row-vector product.
Matrix operator*(const Matrix& a, const Matrix& b);

// The 4x4 affine embedding of m, written into a file-scope scratch array; the return is its address.
float* Augment(const Matrix& m);

// A scratch array of the vector's three floats followed by the scalar; the return is its address.
float* Homogenize(const Vector& v, const float& f);

// Closest approach of two infinite lines given as point plus direction, and, when out1/out2 are
// non-null, the closest point on each line.
float DistanceBetweenLines(const Vector& p1, const Vector& d1, const Vector& p2, const Vector& d2,
                           Vector* out1, Vector* out2);

// Computes a pseudo-trackball rotation. Two rays out of a0 are hit against the plane through a1
// normal to a1 - a0, rescaled and folded onto the sphere, and the two folded points are handed to
// RotationArc. Returns a Quaternion, not a Vector.
Quaternion VirtualTrackBall(const Vector& a0, const Vector& a1, const Vector& a2,
                            const Vector& a3);

// A stub that always returns the zero vector.
Vector operator*(const Vector& v, const Quaternion& q);

// n & (n - 1) == 0. Note that n == 0 returns true.
bool IsPowerOf2(int n);

// Intersects a segment with a sphere, answering how many of the quadratic's two roots lie on the
// segment, written into vOut1, the first valid root, and vOut2. The trailing bool is never read.
int IntersectCircle2d(const Vector& vCenter, float fRadius, const Vector& vStart,
                      const Vector& vEnd, Vector& vOut1, Vector& vOut2, bool bUnused);

// True when vPoint lies inside the axis-aligned box.
bool BoxInside(const Vector& vPoint, const Vector& vMin, const Vector& vMax);

// Segment c to d against the given plane, then an edge walk of the polygon testing each edge
// against a small tolerance. Writes the plane hit to pOut and returns whether every edge passed.
bool polyhit(const Vector* pa, int nCount, const Vector& b, const float& fRadius,
              const Vector& c, const Vector& d, Vector* pOut);

// The same segment-versus-polygon test with the plane inferred from the polygon (a Newell fan),
// writing the plane hit to pOut and the polygon normal to pNormal; each may be null. The return is
// an exit code, and the call sites test the whole result. Declared identically in
// CSWRoomSurfaceMesh.h and Part.cpp, so the three must agree.
unsigned int polyhit(const Vector* pa, int nCount, const Vector& c, const Vector& d,
                     Vector* pOut, Vector* pNormal);

// Runs a Cohen-Sutherland outcode test of the segment against the box, with six outcode bits per
// endpoint, a trivial accept, a trivial reject, then up to three plane clips returning the crossed
// face code. The return is an exit code, and a full-width all-ones value is a hit, not an error.
unsigned int BoxIntersect(const Vector& a, const Vector& b, const Vector& vMin, const Vector& vMax);

// The 5-argument variant has the same outcode prologue, but each surviving clip writes the crossed
// point to pOut and returns its face code, or 0 if that clip's guard fails. pOut is written as a
// whole-Vector assignment.
unsigned int BoxIntersect(const Vector& a, const Vector& b, const Vector& vMin, const Vector& vMax,
                          Vector* pOut);

// True when the sphere of the given radius about vCenter reaches the segment.
int SphereIntersect(const Vector& vStart, const Vector& vEnd, const Vector& vCenter,
                     float fRadius);

// Sunday's segment-to-segment distance, with the sN/sD/tN/tD clamping cascade. The return is the
// three-way maximum of the component deltas of the two closest points, not the length of that
// vector.
float ClosestDistanceBetweenSegments(const Vector& S1_P0, const Vector& S1_P1,
                                     const Vector& S2_P0, const Vector& S2_P1);

// 2D segment/segment intersection: the XY-plane hit of the first segment against the second, with
// the hit's Z interpolated along the second. Returns 1 and writes out on a hit.
int IntersectLineSegments2d(const Vector& a, const Vector& b, const Vector& c, const Vector& d,
                            Vector& out);

// The 4x4 row-major affine embedding of q.getmatrix(), with vPos in the translation row and 1.0f at
// [15]: the leading three rows hold the 3x3 and the position, and the fourth column is zero.
void MakeMatrix4x4(float* pOut, const Vector& vPos, const Quaternion& q);

// Not the plain transpose of MakeMatrix4x4's matrix, since rows 1 and 2 are identical to it and
// only row 0 is negated. The translation tail is the same.
void MakeMatrix4x4Transpose(float* pOut, const Vector& vPos, const Quaternion& q);

// The 32-bit bisection masks used by FloorLogBase2.
extern uint64_t topMask[5];

// Five-level bisection floor-log2 over the low 32 bits.
uint64_t FloorLogBase2(uint64_t x);

// Three-level de Casteljau fold. The four parameters are used in the order 0, 2, 3, 1, so the clean
// reading is (start, end, control1, control2).
Vector InterpolateBezier(const Vector& v0, const Vector& v1, const Vector& v2, const Vector& v3,
                         float t);
float InterpolateBezier(float f0, float f1, float f2, float f3, float t);

// A stub that returns b and ignores a and t.
bool Interpolate(const bool& a, const bool& b, float t);

// Linear blend truncated back to int.
int Interpolate(const int& a, const int& b, float t);

// Linear blend, t = 0 at a.
inline Vector Interpolate(const Vector& a, const Vector& b, float t)
{
    Vector r = { (1.0f - t) * a.x + b.x * t, (1.0f - t) * a.y + b.y * t, (1.0f - t) * a.z + b.z * t };
    return r;
}

inline Quaternion Interpolate(const Quaternion& a, const Quaternion& b, float t) { return slerp(a, b, t); }

#endif
