// The engine's free math functions for vectors, planes, matrices, quaternions and Euler angles.

#include "k2stdint.h"
#include "math3d.h"
#include "Vector.h"
#include "List.h"

#include "port/libcstring.h"
#include "port/libcmath.h"

// Epsilon for the parallel-line test; a double literal so the comparison promotes.
#define SMALL_NUM 1E-8

Vector xaxis(1.0f, 0.0f, 0.0f);
Vector yaxis(0.0f, 1.0f, 0.0f);
Vector zaxis(0.0f, 0.0f, 1.0f);

static float s_afAugmented[16];

float clampf(float v)
{
    if (v < 0.0f)
        v = 0.0f;
    if (v > 1.0f)
        v = 1.0f;
    return v;
}

float DegToRad(float degrees)
{
    return degrees * 0.0174532924f;
}

float cosineLUT(float fAngle)
{
    static const float lookUpTable[32] = {
         1.0f,       0.98078525f, 0.9238795f,  0.8314696f,
         0.70710677f, 0.55557024f, 0.38268343f, 0.19509032f,
         0.0f,      -0.19509032f, -0.38268343f, -0.55557024f,
        -0.70710677f, -0.8314696f, -0.9238795f, -0.98078525f,
        -1.0f,      -0.98078525f, -0.9238795f, -0.8314696f,
        -0.70710677f, -0.55557024f, -0.38268343f, -0.19509032f,
         0.0f,       0.19509032f,  0.38268343f,  0.55557024f,
         0.70710677f, 0.8314696f,  0.9238795f,  0.98078525f,
    };
    return lookUpTable[static_cast<int>(fAngle / (6.2831855f / 32.0f)) & 31];
}

float sineLUT(float fAngle)
{
    static const float lookUpTable[32] = {
         0.0f,       0.19509032f, 0.38268343f, 0.55557024f,
         0.70710677f, 0.8314696f,  0.9238795f,  0.98078525f,
         1.0f,       0.98078525f, 0.9238795f,  0.8314696f,
         0.70710677f, 0.55557024f, 0.38268343f, 0.19509032f,
         0.0f,      -0.19509032f, -0.38268343f, -0.55557024f,
        -0.70710677f, -0.8314696f, -0.9238795f, -0.98078525f,
        -1.0f,      -0.98078525f, -0.9238795f, -0.8314696f,
        -0.70710677f, -0.55557024f, -0.38268343f, -0.19509032f,
    };
    return lookUpTable[static_cast<int>(fAngle / (6.2831855f / 32.0f)) & 31];
}

float Round(float v, float step)
{
    return floorf(v / step + 0.5f) * step;
}

int argmin(float* a, int n)
{
    int nBest = 0;
    for (int i = 1; i < n; i++)
        if (a[i] < a[nBest])
            nBest = i;
    return nBest;
}

Vector VectorMin(Vector a, Vector b)
{
    return Vector(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z);
}

Vector VectorMax(Vector a, Vector b)
{
    return Vector(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z);
}

Vector NormalOf(const Vector& a, const Vector& b, const Vector& c)
{
    // The degenerate-length guard is zero here, unlike normalize's epsilon.
    Vector vCross = cross(b - a, c - b);
    float fLen = magnitude(vCross);
    if (fLen == 0.0f)
        return Vector(1.0f, 0.0f, 0.0f);
    return vCross * (1.0f / fLen);
}

float magnitude(const Vector& v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector normalize(const Vector& v)
{
    // The length is compared in double, because the threshold is a double literal.
    float fLen = magnitude(v);
    if (fLen < 1e-09)
        return Vector(1.0f, 0.0f, 0.0f);
    return v * (1.0f / fLen);
}

Vector Orth(const Vector& v)
{
    // Falls back to the Y axis when the first cross product is shorter than 0.01; a length
    // below 1e-09 takes the +X default. The length is measured once per candidate.
    Vector vOrth = cross(v, Vector(1.0f, 0.0f, 0.0f));
    float fLen = magnitude(vOrth);
    if (fLen < 0.01)
    {
        vOrth = cross(v, Vector(0.0f, 1.0f, 0.0f));
        fLen = magnitude(vOrth);
    }
    if (fLen < 1e-09)
        return Vector(1.0f, 0.0f, 0.0f);
    return vOrth * (1.0f / fLen);
}

Vector Orth(const Vector& a, const Vector& b)
{
    return normalize(cross(a, b));
}

unsigned char bitcount(unsigned char v)
{
    return (v & 1) + ((v >> 1) & 1) + ((v >> 2) & 1) + ((v >> 3) & 1) +
           ((v >> 4) & 1) + ((v >> 5) & 1) + ((v >> 6) & 1) + ((v >> 7) & 1);
}

float BoxAbovePlane(const Vector& v1, const Vector& v2, const Plane& plane)
{
    // One corner per sign of the plane's normal.
    Vector vCorner(plane.a > 0.0f ? v1.x : v2.x,
                   plane.b > 0.0f ? v1.y : v2.y,
                   plane.c > 0.0f ? v1.z : v2.z);
    return plane.d + (vCorner.x * plane.a + vCorner.y * plane.b + vCorner.z * plane.c);
}

float SphereAbovePlane(const Vector& vSphere, float fRadius, const Plane& plane)
{
    return plane.d + plane.a * vSphere.x + plane.b * vSphere.y + plane.c * vSphere.z - fRadius;
}

Vector VectorMax(List<Vector>& list)
{
    Vector vMax = list.m_pData[0];
    // An empty list returns the seed element unchanged.
    if (list.m_count > 0)
    for (int64_t i = 1; i < list.m_count; i++)
    {
        vMax.x = vMax.x > list.m_pData[i].x ? vMax.x : list.m_pData[i].x;
        vMax.y = vMax.y > list.m_pData[i].y ? vMax.y : list.m_pData[i].y;
        vMax.z = vMax.z > list.m_pData[i].z ? vMax.z : list.m_pData[i].z;
    }
    return vMax;
}

int VectorMaxDir(List<Vector>& list, Vector vDir)
{
    if (list.m_count < 1)
        return 0;
    int nBest = 0;
    for (int64_t i = 0; i < list.m_count; i++)
    {
        float fDot = vDir.x * list.m_pData[i].x + vDir.y * list.m_pData[i].y
                   + vDir.z * list.m_pData[i].z;
        float fBest = vDir.x * list.m_pData[nBest].x + vDir.y * list.m_pData[nBest].y
                    + vDir.z * list.m_pData[nBest].z;
        if (fDot > fBest)
            nBest = static_cast<int>(i);
    }
    return nBest;
}

Vector VectorMin(List<Vector>& list)
{
    Vector vMin = list.m_pData[0];
    // An empty list returns the seed element unchanged.
    if (list.m_count > 0)
    for (int64_t i = 1; i < list.m_count; i++)
    {
        vMin.x = vMin.x < list.m_pData[i].x ? vMin.x : list.m_pData[i].x;
        vMin.y = vMin.y < list.m_pData[i].y ? vMin.y : list.m_pData[i].y;
        vMin.z = vMin.z < list.m_pData[i].z ? vMin.z : list.m_pData[i].z;
    }
    return vMin;
}

void Translate(List<Vector>& list, Vector vOffset)
{
    Vector* pData = list.m_pData;
    for (int64_t i = 0; i < list.m_count; i++)
    {
        pData[i] = vOffset + pData[i];
    }
}

void Scale(List<Vector>& list, float fScale)
{
    Vector* pData = list.m_pData;
    for (int64_t i = 0; i < list.m_count; i++)
    {
        pData[i] = pData[i] * fScale;
    }
}

float ComputeArea(List<Vector>& list)
{
    if (list.m_count < 1)
        return 0.0f;
    Vector v0 = list.m_pData[0];
    float fArea = 0.0f;
    int i = 1;
    do
    {
        // Wraps around the polygon; the last edge closes back onto the first vertex.
        int nCurr = (i + 1) % list.m_count;
        int nPrev = i % list.m_count;
        Vector vCurr = list.m_pData[nCurr];
        Vector vPrev = list.m_pData[nPrev];
        Vector vA = vPrev - v0;
        Vector vB = vCurr - vPrev;
        fArea += magnitude(cross(vA, vB)) * 0.5f;
    } while (i++ < list.m_count);
    return fArea;
}

void Vector::Normalize()
{
    float fLen = magnitude(*this);
    if (fLen < 1e-09)
    {
        *this = Vector(1.0f, 0.0f, 0.0f);
        return;
    }
    float fInv = 1.0f / fLen;
    x *= fInv;
    y *= fInv;
    z *= fInv;
}

bool isnormalized(const Vector& v)
{
    float fLen2 = dot(v, v);
    return fLen2 >= 0.99 && fLen2 <= 1.01;
}

Vector planelineintersection(const Vector& vNormal, float fDist, const Vector& vLineStart,
                             const Vector& vLineEnd)
{
    // Scratch vector, shared across calls.
    static Vector dif;
    dif = vLineEnd - vLineStart;

    float fDenom = dot(vNormal, dif);
    float fT = 0.0f;
    if (fDenom != 0.0f)
    {
        fT = -(dot(vNormal, vLineStart) + fDist) / fDenom;
    }
    return vLineStart + dif * fT;
}

Vector planesegmentintersection(const Vector& vNormal, float fDist, const Vector& vLineStart,
                                const Vector& vLineEnd)
{
    static Vector dif;
    dif = vLineEnd - vLineStart;

    float fDenom = dot(vNormal, dif);
    float fT = 0.0f;
    if (fDenom != 0.0f)
    {
        fT = -(dot(vNormal, vLineStart) + fDist) / fDenom;
        // Clamp the crossing parameter to the segment.
        if (fT > 1.0f)
            fT = 1.0f;
        else if (fT < 0.0f)
            fT = 0.0f;
    }
    return vLineStart + dif * fT;
}

float PlaneHeightAtPoint(const Vector& vNormal, float fDist, const Vector& vPoint)
{
    // Solves the plane equation for z.
    if (vNormal.z == 0.0f)
        return 0.0f;
    return -(vNormal.x * vPoint.x + fDist + vNormal.y * vPoint.y) / vNormal.z;
}

Vector planeproject(const Vector& vNormal, float fDist, const Vector& vPoint)
{
    float fDot = dot(vPoint, vNormal) + fDist;
    return vPoint - vNormal * fDot;
}

Vector lineproject(const Vector& a, const Vector& b, const Vector& p)
{
    // Scratch vector, shared across calls.
    static Vector w;
    w = b - a;
    float fLen2 = dot(w, w);
    float fT = fLen2;
    if (fLen2 != 0.0f)
    {
        fT = dot(w, p - a) / fLen2;
    }
    return a + w * fT;
}

Vector segmentproject(const Vector& a, const Vector& b, const Vector& p)
{
    if (equal(a, b))
        return a;

    static Vector w;
    w = b - a;
    float fLen2 = dot(w, w);
    float fT = fLen2;
    if (fLen2 != 0.0f)
    {
        fT = dot(w, p - a) / fLen2;
    }
    // Clamp to the segment.
    if (fT > 1.0f)
        fT = 1.0f;
    if (fT < 0.0f)
        fT = 0.0f;
    return a + w * fT;
}

int equal(const Vector& a, const Vector& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

float Determinant(Matrix m)
{
    return m.m_f[0]*m.m_f[4]*m.m_f[8] + m.m_f[3]*m.m_f[7]*m.m_f[2] + m.m_f[6]*m.m_f[1]*m.m_f[5]
         - m.m_f[0]*m.m_f[7]*m.m_f[5] - m.m_f[3]*m.m_f[1]*m.m_f[8] - m.m_f[6]*m.m_f[4]*m.m_f[2];
}

Matrix Inverse(Matrix m)
{
    static Matrix r;
    float d = Determinant(m);
    r.m_f[0] = (m.m_f[4]*m.m_f[8] - m.m_f[5]*m.m_f[7]) / d;
    r.m_f[3] = (m.m_f[5]*m.m_f[6] - m.m_f[3]*m.m_f[8]) / d;
    r.m_f[6] = (m.m_f[3]*m.m_f[7] - m.m_f[4]*m.m_f[6]) / d;
    r.m_f[1] = (m.m_f[2]*m.m_f[7] - m.m_f[1]*m.m_f[8]) / d;
    r.m_f[4] = (m.m_f[0]*m.m_f[8] - m.m_f[2]*m.m_f[6]) / d;
    r.m_f[7] = (m.m_f[1]*m.m_f[6] - m.m_f[0]*m.m_f[7]) / d;
    r.m_f[2] = (m.m_f[1]*m.m_f[5] - m.m_f[2]*m.m_f[4]) / d;
    r.m_f[5] = (m.m_f[2]*m.m_f[3] - m.m_f[0]*m.m_f[5]) / d;
    r.m_f[8] = (m.m_f[0]*m.m_f[4] - m.m_f[1]*m.m_f[3]) / d;
    return r;
}

Matrix inverse2(Matrix m)
{
    m.m_f[2] = 0.0f;
    m.m_f[5] = 0.0f;
    m.m_f[6] = 0.0f;
    m.m_f[7] = 0.0f;
    m.m_f[8] = 1.0f;
    float a0 = m.m_f[0];
    float a1 = m.m_f[1];
    float a3 = m.m_f[3];
    float a4 = m.m_f[4];
    float d = 1.0f / Determinant(m);
    m.m_f[0] =  a4 * d;
    m.m_f[1] = -a1 * d;
    m.m_f[2] =  m.m_f[2] * d;
    m.m_f[3] = -a3 * d;
    m.m_f[4] =  a0 * d;
    m.m_f[5] =  m.m_f[5] * d;
    return m;
}

inline float fAbs(float f)
{
    return f < 0.0f ? -f : f;
}

inline float fMax(float a, float b)
{
    return a > b ? a : b;
}

inline float fMax3(float a, float b, float c)
{
    return fMax(a, fMax(b, c));
}

inline float fMaxAbs3(float a, float b, float c)
{
    return fMax3(fAbs(a), fAbs(b), fAbs(c));
}

// Inclusive range test; the bounds may be given in either order.
inline bool fInRange(float v, float lo, float hi)
{
    return (hi >= lo && v >= lo && v <= hi) || (lo >= hi && v >= hi && v <= lo);
}

// True when v is outside [lo, hi] in either ordering; a NaN bound is never outside.
inline bool fOutOfRange(float v, float lo, float hi)
{
    return (hi >= lo && !(v >= lo && v <= hi)) || (lo >= hi && !(v >= hi && v <= lo));
}

Matrix Transpose(const Matrix& m)
{
    Matrix r;
    Vector v0(m.m_f[0], m.m_f[3], m.m_f[6]);
    Vector v1(m.m_f[1], m.m_f[4], m.m_f[7]);
    Vector v2(m.m_f[2], m.m_f[5], m.m_f[8]);
    r.Rows()[0] = v0;
    r.Rows()[1] = v1;
    r.Rows()[2] = v2;
    return r;
}


float norm(const Matrix& m)
{
    float fRow0 = fMaxAbs3(m.m_f[0], m.m_f[1], m.m_f[2]);
    float fRow1 = fMaxAbs3(m.m_f[3], m.m_f[4], m.m_f[5]);
    float fRow2 = fMaxAbs3(m.m_f[6], m.m_f[7], m.m_f[8]);
    return fMax3(fRow0, fRow1, fRow2);
}

Vector operator*(const Matrix& m, const Vector& v)
{
    Vector r;
    r.x = m.m_f[0]*v.x + m.m_f[3]*v.y + m.m_f[6]*v.z;
    r.y = m.m_f[1]*v.x + m.m_f[4]*v.y + m.m_f[7]*v.z;
    r.z = m.m_f[2]*v.x + m.m_f[5]*v.y + m.m_f[8]*v.z;
    return r;
}

Matrix operator*(const Matrix& m, const Matrix& b)
{
    Matrix r;
    r.m_f[0] = m.m_f[0]*b.m_f[0] + m.m_f[3]*b.m_f[1] + m.m_f[6]*b.m_f[2];
    r.m_f[1] = m.m_f[1]*b.m_f[0] + m.m_f[4]*b.m_f[1] + m.m_f[7]*b.m_f[2];
    r.m_f[2] = m.m_f[2]*b.m_f[0] + m.m_f[5]*b.m_f[1] + m.m_f[8]*b.m_f[2];
    r.m_f[3] = m.m_f[0]*b.m_f[3] + m.m_f[3]*b.m_f[4] + m.m_f[6]*b.m_f[5];
    r.m_f[4] = m.m_f[1]*b.m_f[3] + m.m_f[4]*b.m_f[4] + m.m_f[7]*b.m_f[5];
    r.m_f[5] = m.m_f[2]*b.m_f[3] + m.m_f[5]*b.m_f[4] + m.m_f[8]*b.m_f[5];
    r.m_f[6] = m.m_f[0]*b.m_f[6] + m.m_f[3]*b.m_f[7] + m.m_f[6]*b.m_f[8];
    r.m_f[7] = m.m_f[1]*b.m_f[6] + m.m_f[4]*b.m_f[7] + m.m_f[7]*b.m_f[8];
    r.m_f[8] = m.m_f[2]*b.m_f[6] + m.m_f[5]*b.m_f[7] + m.m_f[8]*b.m_f[8];
    return r;
}

float* Augment(const Matrix& m)
{
    float* p = s_afAugmented;
    memset(p, 0, 15 * sizeof(float));
    p[15] = 1.0f;
    p[0] = m.m_f[0]; p[4] = m.m_f[1]; p[8]  = m.m_f[2];
    p[1] = m.m_f[3]; p[5] = m.m_f[4]; p[9]  = m.m_f[5];
    p[2] = m.m_f[6]; p[6] = m.m_f[7]; p[10] = m.m_f[8];
    return p;
}

Quaternion Matrix::getquaternion() const
{
    float m[3][3];
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            m[r][c] = m_f[r * 3 + c];

    float fTrace = m[0][0] + m[1][1] + m[2][2];
    if (fTrace > 0.0f)
    {
        float s = static_cast<float>(sqrt(1.0 + fTrace));
        float w = s * 0.5f;
        s = 0.5f / s;
        return Quaternion(w, (m[1][2] - m[2][1]) * s,
                             (m[2][0] - m[0][2]) * s,
                             (m[0][1] - m[1][0]) * s);
    }

    static const int aNext[3] = { 1, 2, 0 };
    int i = 0;
    if (m[1][1] > m[0][0])
        i = 1;
    if (m[2][2] > m[i][i])
        i = 2;
    int k = aNext[i];
    int j = aNext[k];

    float af[4];
    float s = static_cast<float>(sqrt(1.0 + (m[i][i] - (m[k][k] + m[j][j]))));
    af[i] = s * 0.5f;
    if (s != 0.0f)
        s = 0.5f / s;
    af[3] = (m[k][j] - m[j][k]) * s;
    af[k] = (m[i][k] + m[k][i]) * s;
    af[j] = (m[i][j] + m[j][i]) * s;
    return Quaternion(af[3], af[0], af[1], af[2]);
}

extern int quaternionidentityoptimization;

// Rotates v by q. This is the inverse-rotation spelling: the scalar is negated first, which the
// AURORA copies of this helper reach the other way round.
static inline Vector RotateVector(const Vector& v, const Quaternion& q)
{
    float w = q.w, x = q.x, y = q.y, z = q.z;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        w = -w;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float xw = x * w, yw = y * w, zw = z * w;
        Vector r;
        r.x = (1.0f - 2.0f*(yy + zz)) * v.x + 2.0f*(xy - zw) * v.y + 2.0f*(xz + yw) * v.z;
        r.y = 2.0f*(xy + zw) * v.x + (1.0f - 2.0f*(zz + xx)) * v.y + 2.0f*(yz - xw) * v.z;
        r.z = 2.0f*(xz - yw) * v.x + 2.0f*(yz + xw) * v.y + (1.0f - 2.0f*(xx + yy)) * v.z;
        return r;
    }
    return v;
}

void Plane::Transform(const Vector& vPoint, const Quaternion& q)
{
    static Vector newnormal;
    static Vector origin;

    newnormal = RotateVector(Vector(a, b, c), q);
    origin = RotateVector(-(Vector(a, b, c) * d) - vPoint, q);
    a = newnormal.x;
    b = newnormal.y;
    c = newnormal.z;
    d = -dot(newnormal, origin);
}

#if KOTOR1  // K1: the quaternion operators are defined here, not in the header
Quaternion operator-(const Quaternion& q)
{
    return Quaternion(-q.w, q.x, q.y, q.z);
}

Quaternion operator*(const Quaternion& a, const Quaternion& b)
{
    Quaternion r;
    r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return r;
}

Quaternion operator*(const Quaternion& q, float f)
{
    return Quaternion(q.w * f, q.x * f, q.y * f, q.z * f);
}

Quaternion& Quaternion::operator*=(float f)
{
    w *= f;
    x *= f;
    y *= f;
    z *= f;
    return *this;
}

Quaternion operator+(const Quaternion& a, const Quaternion& b)
{
    return Quaternion(a.w + b.w, a.x + b.x, a.y + b.y, a.z + b.z);
}

float dot(const Quaternion& a, const Quaternion& b)
{
    return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
}

float norm(const Vector& v)
{
    return fAbs(v.x) > (fAbs(v.y) > fAbs(v.z) ? fAbs(v.y) : fAbs(v.z))
               ? fAbs(v.x)
               : (fAbs(v.y) > fAbs(v.z) ? fAbs(v.y) : fAbs(v.z));
}
#endif

// A stub that ignores both arguments and returns the zero vector.
Vector operator*(const Vector& v, const Quaternion& q)
{
    K2_UNUSED(v); K2_UNUSED(q);
    return Vector();
}

Quaternion normalize(Quaternion q)
{
    float fLen = sqrtf(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
    if (fLen == 0.0f)
    {
        q.w = 1.0f;
        q.x = q.y = q.z = 0.0f;
        return q;
    }
    float fInv = 1.0f / fLen;
    q.w *= fInv;
    q.x *= fInv;
    q.y *= fInv;
    q.z *= fInv;
    return q;
}

Quaternion slerp(Quaternion qFrom, const Quaternion& qTo, float t)
{
    float fDot = qFrom.w*qTo.w + qFrom.x*qTo.x + qFrom.y*qTo.y + qFrom.z*qTo.z;
    if (fDot < 0.0f)
    {
        qFrom.w = -qFrom.w;
        qFrom.x = -qFrom.x;
        qFrom.y = -qFrom.y;
        qFrom.z = -qFrom.z;
    }
    float fAbs = fDot;
    if (fAbs < 0.0f)
        fAbs = -fAbs;
    if (fAbs >= 1.0f)
        return qFrom;

    float fTheta = acosf(fAbs);
    if (fTheta == 0.0f)
        return qFrom;

    float fSinTheta = sinf(fTheta);
    float fT = fTheta * t;
    float f0 = sinf(fTheta - fT) / fSinTheta;
    qFrom.w *= f0;
    qFrom.x *= f0;
    qFrom.y *= f0;
    qFrom.z *= f0;
    float f1 = sinf(fT) / fSinTheta;
    qFrom.w += qTo.w * f1;
    qFrom.x += qTo.x * f1;
    qFrom.y += qTo.y * f1;
    qFrom.z += qTo.z * f1;
    return qFrom;
}

Quaternion YawPitchRoll(float yaw, float pitch, float roll)
{
    float fYawRad = DegToRad(yaw);
    float fPitchRad = DegToRad(pitch);
    float fRollRad = DegToRad(roll);

    Quaternion qYaw;
    if (fYawRad != 0.0f)
    {
        Vector vAxis = normalize(zaxis);
        float fSin = sinf(fYawRad * 0.5f);
        float fCos = cosf(fYawRad * 0.5f);
        qYaw.w = fCos;
        qYaw.x = fSin * vAxis.x;
        qYaw.y = fSin * vAxis.y;
        qYaw.z = fSin * vAxis.z;
    }

    Quaternion qPitch;
    if (fPitchRad != 0.0f)
    {
        Vector vAxis = normalize(xaxis);
        float fSin = sinf(fPitchRad * 0.5f);
        float fCos = cosf(fPitchRad * 0.5f);
        qPitch.w = fCos;
        qPitch.x = fSin * vAxis.x;
        qPitch.y = fSin * vAxis.y;
        qPitch.z = fSin * vAxis.z;
    }

    Quaternion q = qYaw * qPitch;

    Quaternion qRoll;
    if (fRollRad != 0.0f)
    {
        Vector vAxis = normalize(yaxis);
        float fSin = sinf(fRollRad * 0.5f);
        float fCos = cosf(fRollRad * 0.5f);
        qRoll.w = fCos;
        qRoll.x = fSin * vAxis.x;
        qRoll.y = fSin * vAxis.y;
        qRoll.z = fSin * vAxis.z;
    }

    q = q * qRoll;
    return q;
}

float Yaw(const Quaternion& q)
{
    static Vector v;
    v = q.ydir();
    if (v.x == 0.0f && v.y == 0.0f)
        return 0.0f;
    return atan2f(-v.x, v.y) * 57.295776f;
}

float Pitch(const Quaternion& q)
{
    static Vector v;
    v = q.ydir();
    return atan2f(v.z, sqrtf(v.x * v.x + v.y * v.y)) * 57.295776f;
}

float Roll(Quaternion q)
{
    Quaternion qYaw(zaxis, DegToRad(-Yaw(q)));
    q = qYaw * q;

#if KOTOR1
    Quaternion qPitch(xaxis, -DegToRad(Pitch(q)));
#else
    Quaternion qPitch(xaxis, DegToRad(-Pitch(q)));
#endif
    q = qPitch * q;

    return atan2f(-q.xdir().z, q.xdir().x) * 57.295776f;
}

float Yaw(const Vector& v)
{
    if (v.y == 0.0f && v.x == 0.0f)
        return 0.0f;
    return atan2f(-v.x, v.y) * 57.295776f;
}

float Pitch(const Vector& v)
{
    return atan2f(v.z, sqrtf(v.x * v.x + v.y * v.y)) * 57.295776f;
}

Quaternion pointto(const Vector& n)
{
    static Vector v;
    static Vector a;
    v = normalize(n);

    Vector vUp(0.0f, 0.0f, 1.0f);
    a = cross(vUp, v);

    float fCross = magnitude(a);
    if (fCross > 1.0f)
        fCross = 1.0f;

    if (fCross == 0.0f)
    {
        if (v.z > 0.0f)
            return Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
        return Quaternion(xaxis, 3.1415927f);
    }

    float fAngle = asinf(fCross);
    float fDot = vUp.x * v.x + vUp.y * v.y + vUp.z * v.z;
    if (fDot < 0.0f)
        fAngle = 3.1415927f - fAngle;
    return Quaternion(normalize(a), fAngle);
}

Quaternion RotationArc(Vector from, Vector to)
{
    static Vector v;
    v = normalize(from);
    Vector vTo = normalize(to);

    float fDot = v.x * vTo.x + v.y * vTo.y + v.z * vTo.z;
    if (fDot <= -1.0f)
        return Quaternion(0.0f, 1.0f, 0.0f, 0.0f);

    Vector vAxis = cross(v, vTo);
    float fScale = sqrtf(2.0f * (1.0f + fDot));

    Quaternion q;
    q.x = vAxis.x / fScale;
    q.y = vAxis.y / fScale;
    q.z = vAxis.z / fScale;
    q.w = fScale * 0.5f;
    return q;
}

bool polyhit(const Vector* pa, int nCount, const Vector& b, const float& fRadius,
              const Vector& c, const Vector& d, Vector* pOut)
{
    float fDistC = dot(b, c) + fRadius;
    float fDistD = dot(b, d) + fRadius;

    if (fDistC == fDistD)
        return 0;
    if (fDistC < 0.0f)
        return 0;
    if (fDistD > 0.0f)
        return 0;

    static Vector the_point;
    static Vector side;

    float fT = fDistC / (fDistC - fDistD);
    the_point = c * (1.0f - fT) + d * fT;

    bool bInside = true;
    float fDot = 0.0f;
    for (int i = 1; i <= nCount; i++)
    {
        side = cross(pa[i % nCount] - pa[i - 1], the_point - pa[i - 1]);
        fDot = dot(side, b);
        bInside = fDot >= -1e-05f;
        if (!bInside)
            break;
    }
    if (pOut != 0 && bInside)
        *pOut = the_point;
    return fDot >= -1e-05f;
}

unsigned int polyhit(const Vector* pa, int nCount, const Vector& c, const Vector& d,
                     Vector* pOut, Vector* pNormal)
{
    Vector b(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < nCount - 2; i++)
    {
        int j = (i + 1) % nCount;
        int k = (i + 2) % nCount;
        b = b + cross(pa[j] - pa[i], pa[k] - pa[j]);
    }

    float fLen = magnitude(b);
    if (fLen < 1e-08f)
        return 0;
    b = b * (1.0f / fLen);

    float fDot0 = b.x * pa[0].x + b.y * pa[0].y + b.z * pa[0].z;
    float fDistC = b.x * c.x + b.y * c.y + b.z * c.z - fDot0;
    if (fDistC < 0.0f)
        return 0;

    float fDistD = b.x * d.x + b.y * d.y + b.z * d.z - fDot0;
    if (fDistD > 0.0f)
        return 0;
    if (fDistC == fDistD)
        return 0;

    static Vector the_point;

    float fT = fDistC / (fDistC - fDistD);
    the_point = c * (1.0f - fT) + d * fT;

    float fDot = 0.0f;
    for (int64_t i = 1; i <= nCount; i++)
    {
        Vector side = cross(pa[i % nCount] - pa[i - 1], the_point - pa[i - 1]);
        fDot = side.x * b.x + side.y * b.y + side.z * b.z;
        // A NaN edge dot counts as a miss.
        if (!(fDot >= 0.0f))
            break;
    }
    if (!(fDot >= 0.0f))
        return 0;

    if (pNormal != 0)
        *pNormal = b;
    if (pOut != 0)
        *pOut = the_point;
    return 1;
}

// The box-intersection outcodes: which side of each slab a segment end lies past.
enum {
    OUTCODE_MAX_Z = 0x01,
    OUTCODE_MIN_Z = 0x02,
    OUTCODE_MAX_Y = 0x04,
    OUTCODE_MIN_Y = 0x08,
    OUTCODE_MAX_X = 0x10,
    OUTCODE_MIN_X = 0x20,
};
// The box intersections' result when the segment starts inside the box.
enum : unsigned int {
    BOX_START_INSIDE = 0xffffffff,
};

// The point's outcode against the box: one bit per slab face it lies beyond.
inline void SetOpCode(const Vector& vPoint, const Vector& vMin, const Vector& vMax, unsigned char& nCode)
{
    nCode = 0;
    if (vPoint.x < vMin.x)
        nCode |= OUTCODE_MIN_X;
    else if (vPoint.x > vMax.x)
        nCode |= OUTCODE_MAX_X;
    if (vPoint.y < vMin.y)
        nCode |= OUTCODE_MIN_Y;
    else if (vPoint.y > vMax.y)
        nCode |= OUTCODE_MAX_Y;
    if (vPoint.z < vMin.z)
        nCode |= OUTCODE_MIN_Z;
    else if (vPoint.z > vMax.z)
        nCode |= OUTCODE_MAX_Z;
}

bool BoxInside(const Vector& vPoint, const Vector& vMin, const Vector& vMax)
{
    // Ordered >=/<= chain so a NaN coordinate fails every test.
    return vPoint.x >= vMin.x && vPoint.x <= vMax.x
        && vPoint.y >= vMin.y && vPoint.y <= vMax.y
        && vPoint.z >= vMin.z && vPoint.z <= vMax.z;
}

unsigned int BoxIntersect(const Vector& a, const Vector& b, const Vector& vMin,
                          const Vector& vMax, Vector* pOut)
{
    unsigned char nOutA;
    SetOpCode(a, vMin, vMax, nOutA);
    if (nOutA == 0)
        return BOX_START_INSIDE;

    unsigned char nOutB;
    SetOpCode(b, vMin, vMax, nOutB);

    if ((nOutA & nOutB) != 0)
        return 0;

    if ((nOutA & OUTCODE_MIN_X) != 0)
    {
        float t = (vMin.x - a.x) / (b.x - a.x);
        float fY = t * b.y + (1.0f - t) * a.y;
        float fZ = t * b.z + (1.0f - t) * a.z;
        if (fY >= vMin.y && fZ >= vMin.z && fY <= vMax.y && fZ <= vMax.z)
        {
            *pOut = Vector(vMin.x, fY, fZ);
            return 1;
        }
    }
    else if ((nOutA & OUTCODE_MAX_X) != 0)
    {
        float t = (vMax.x - a.x) / (b.x - a.x);
        float fY = t * b.y + (1.0f - t) * a.y;
        float fZ = t * b.z + (1.0f - t) * a.z;
        if (fY >= vMin.y && fZ >= vMin.z && fY <= vMax.y && fZ <= vMax.z)
        {
            *pOut = Vector(vMax.x, fY, fZ);
            return 2;
        }
    }

    if ((nOutA & OUTCODE_MIN_Y) != 0)
    {
        float t = (vMin.y - a.y) / (b.y - a.y);
        float fX = t * b.x + (1.0f - t) * a.x;
        float fZ = t * b.z + (1.0f - t) * a.z;
        if (fX >= vMin.x && fZ >= vMin.z && fX <= vMax.x && fZ <= vMax.z)
        {
            *pOut = Vector(fX, vMin.y, fZ);
            return 3;
        }
    }
    else if ((nOutA & OUTCODE_MAX_Y) != 0)
    {
        float t = (vMax.y - a.y) / (b.y - a.y);
        float fX = t * b.x + (1.0f - t) * a.x;
        float fZ = t * b.z + (1.0f - t) * a.z;
        if (fX >= vMin.x && fZ >= vMin.z && fX <= vMax.x && fZ <= vMax.z)
        {
            *pOut = Vector(fX, vMax.y, fZ);
            return 4;
        }
    }

    if ((nOutA & OUTCODE_MIN_Z) != 0)
    {
        float t = (vMin.z - a.z) / (b.z - a.z);
        float fY = t * b.y + (1.0f - t) * a.y;
        float fX = t * b.x + (1.0f - t) * a.x;
        if (fY >= vMin.y && fY <= vMax.y && fX >= vMin.x && fX <= vMax.x)
        {
            *pOut = Vector(fX, fY, vMin.z);
            return 5;
        }
    }
    else if ((nOutA & OUTCODE_MAX_Z) != 0)
    {
        float t = (vMax.z - a.z) / (b.z - a.z);
        float fY = t * b.y + (1.0f - t) * a.y;
        float fX = t * b.x + (1.0f - t) * a.x;
        if (fY >= vMin.y && fY <= vMax.y && fX >= vMin.x && fX <= vMax.x)
        {
            *pOut = Vector(fX, fY, vMax.z);
            return 6;
        }
    }
    return 0;
}

unsigned int BoxIntersect(const Vector& a, const Vector& b, const Vector& vMin, const Vector& vMax)
{
    unsigned char nOutA;
    SetOpCode(a, vMin, vMax, nOutA);
    if (nOutA == 0)
        return BOX_START_INSIDE;

    unsigned char nOutB;
    SetOpCode(b, vMin, vMax, nOutB);

    if ((nOutA & nOutB) != 0)
        return 0;

    if ((nOutA & OUTCODE_MIN_X) != 0)
    {
        float t = (vMin.x - a.x) / (b.x - a.x);
        float fY = t * b.y + (1.0f - t) * a.y;
        float fZ = t * b.z + (1.0f - t) * a.z;
        if (fY >= vMin.y && fZ >= vMin.z && fY <= vMax.y && fZ <= vMax.z)
            return 1;
    }
    else if ((nOutA & OUTCODE_MAX_X) != 0)
    {
        float t = (vMax.x - a.x) / (b.x - a.x);
        float fY = t * b.y + (1.0f - t) * a.y;
        float fZ = t * b.z + (1.0f - t) * a.z;
        if (fY >= vMin.y && fZ >= vMin.z && fY <= vMax.y && fZ <= vMax.z)
            return 2;
    }

    if ((nOutA & OUTCODE_MIN_Y) != 0)
    {
        float t = (vMin.y - a.y) / (b.y - a.y);
        float fX = t * b.x + (1.0f - t) * a.x;
        float fZ = t * b.z + (1.0f - t) * a.z;
        if (fX >= vMin.x && fZ >= vMin.z && fX <= vMax.x && fZ <= vMax.z)
            return 3;
    }
    else if ((nOutA & OUTCODE_MAX_Y) != 0)
    {
        float t = (vMax.y - a.y) / (b.y - a.y);
        float fX = t * b.x + (1.0f - t) * a.x;
        float fZ = t * b.z + (1.0f - t) * a.z;
        if (fX >= vMin.x && fZ >= vMin.z && fX <= vMax.x && fZ <= vMax.z)
            return 4;
    }

    if ((nOutA & OUTCODE_MIN_Z) != 0)
    {
        float t = (vMin.z - a.z) / (b.z - a.z);
        float fY = t * b.y + (1.0f - t) * a.y;
        float fX = t * b.x + (1.0f - t) * a.x;
        if (fY >= vMin.y && fX >= vMin.x && fY <= vMax.y && fX <= vMax.x)
            return 5;
    }
    else if ((nOutA & OUTCODE_MAX_Z) != 0)
    {
        float t = (vMax.z - a.z) / (b.z - a.z);
        float fY = t * b.y + (1.0f - t) * a.y;
        float fX = t * b.x + (1.0f - t) * a.x;
        if (fY >= vMin.y && fX >= vMin.x && fY <= vMax.y && fX <= vMax.x)
            return 6;
    }
    return 0;
}

int SphereIntersect(const Vector& vStart, const Vector& vEnd, const Vector& vCenter, float fRadius)
{
    Vector vSeg = vEnd - vStart;
    float fLenSq = dot(vSeg, vSeg);
    if (fLenSq < 1e-09)
        return 0;
    float t = dot(vSeg, vCenter - vStart) / fLenSq;
    // Clamped by comparison rather than fmaxf/fminf, which would take the NaN-propagating path.
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;
    Vector vClosest = vStart + vSeg * t;
    float fDist = magnitude(vClosest - vCenter);
    return fDist < fRadius;
}

float DistanceBetweenLines(const Vector& p1, const Vector& d1, const Vector& p2, const Vector& d2,
                           Vector* out1, Vector* out2)
{
    // Scratch vector, shared across calls.
    static Vector cp;
    cp = cross(d1, d2);
    cp.Normalize();

    float fDot1 = dot(cp, p1);
    float fDot2 = dot(cp, p2);
    float fDist = fDot2 - fDot1;
    if (fDist < 0.0f)
        fDist = -fDist;

    if (out1)
    {
        // Plane through p2 whose normal is perpendicular to d2 and the common normal;
        // its crossing with line 1 is the closest point on line 1.
        Vector v1 = normalize(cross(d2, cp));
        *out1 = planelineintersection(v1, -(v1.x * p2.x + v1.y * p2.y + v1.z * p2.z),
                                      p1, p1 + d1);
    }
    if (out2)
    {
        // The mirror image, where cross(d1, cp) is perpendicular to d1.
        Vector v2 = normalize(cross(d1, cp));
        *out2 = planelineintersection(v2, -(v2.x * p1.x + v2.y * p1.y + v2.z * p1.z),
                                      p2, p2 + d2);
    }
    return fDist;
}

float ClosestDistanceBetweenSegments(const Vector& S1_P0, const Vector& S1_P1,
                                     const Vector& S2_P0, const Vector& S2_P1)
{
    Vector u = S1_P1 - S1_P0;
    Vector v = S2_P1 - S2_P0;
    Vector w = S1_P0 - S2_P0;
    float a = u.x * u.x + u.y * u.y + u.z * u.z;
    float b = u.x * v.x + u.y * v.y + u.z * v.z;
    float c = v.x * v.x + v.y * v.y + v.z * v.z;
    float d = u.x * w.x + u.y * w.y + u.z * w.z;
    float e = v.x * w.x + v.y * w.y + v.z * w.z;
    float D = a * c - b * b;
    float sc, sN, sD = D;
    float tc, tN, tD = D;

    if (D < SMALL_NUM)
    {
        sN = 0.0f;
        sD = 1.0f;
        tN = e;
        tD = c;
    }
    else
    {
        sN = b * e - c * d;
        tN = a * e - b * d;
        if (sN < 0.0f)
        {
            sN = 0.0f;
            tN = e;
            tD = c;
        }
        else if (sN > sD)
        {
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0.0f)
    {
        tN = 0.0f;
        if (-d < 0.0f)
            sN = 0.0f;
        else if (-d > a)
            sN = sD;
        else
        {
            sN = -d;
            sD = a;
        }
    }
    else if (tN > tD)
    {
        tN = tD;
        float fT = -d + b;
        sN = fT < 0.0f ? 0.0f : (fT > a ? sD : fT);
        sD = (fT < 0.0f || fT > a) ? sD : a;
    }

    sc = fabsf(sN) < static_cast<float>(SMALL_NUM) ? 0.0f : sN / sD;
    tc = fabsf(tN) < static_cast<float>(SMALL_NUM) ? 0.0f : tN / tD;

    Vector dP = w + u * sc - v * tc;
    return fAbs(dP.x) > (fAbs(dP.y) > fAbs(dP.z) ? fAbs(dP.y) : fAbs(dP.z))
               ? fAbs(dP.x)
               : (fAbs(dP.y) > fAbs(dP.z) ? fAbs(dP.y) : fAbs(dP.z));
}

// n == 0 counts as a power of two.
bool IsPowerOf2(int n)
{
    return (n & (n - 1)) == 0;
}

Quaternion VirtualTrackBall(const Vector& a0, const Vector& a1, const Vector& a2,
                            const Vector& a3)
{
    Vector vAxis = a1 - a0;
    float fLen = magnitude(vAxis);
    float fScale = 1.0f / (fLen * 0.25f);
    vAxis = normalize(vAxis);

    float fPlaneDist = -(vAxis.x * a1.x + vAxis.y * a1.y + vAxis.z * a1.z);

    Vector v1 = planelineintersection(vAxis, fPlaneDist, a0, a0 + a2);
    v1 = (v1 - a1) * fScale;
    float fLen1 = magnitude(v1);
    if (fLen1 > 1.0f)
    {
        if (fLen1 != 0.0f)
            v1 = v1 * (1.0f / fLen1);
    }
    else
    {
        float f = sqrtf(1.0f - fLen1 * fLen1);
        v1 = v1 - vAxis * f;
    }

    Vector v2 = planelineintersection(vAxis, fPlaneDist, a0, a0 + a3);
    v2 = (v2 - a1) * fScale;
    float fLen2 = magnitude(v2);
    if (fLen2 > 1.0f)
    {
        if (fLen2 != 0.0f)
            v2 = v2 * (1.0f / fLen2);
    }
    else
    {
        float f = sqrtf(1.0f - fLen2 * fLen2);
        v2 = v2 - vAxis * f;
    }

    return RotationArc(v1, v2);
}

void MakeMatrix4x4(float* pOut, const Vector& vPos, const Quaternion& q)
{
    float fXX = q.x * q.x, fYY = q.y * q.y, fZZ = q.z * q.z;
    float fXY = q.x * q.y, fXZ = q.x * q.z, fXW = q.x * q.w;
    float fYZ = q.y * q.z, fYW = q.y * q.w, fZW = q.z * q.w;
    pOut[0] = 1.0f - 2.0f * (fYY + fZZ);
    pOut[1] = 2.0f * (fXY + fZW);
    pOut[2] = 2.0f * (fXZ - fYW);
    pOut[3] = 0.0f;
    pOut[4] = 2.0f * (fXY - fZW);
    pOut[5] = 1.0f - 2.0f * (fZZ + fXX);
    pOut[6] = 2.0f * (fYZ + fXW);
    pOut[7] = 0.0f;
    pOut[8] = 2.0f * (fXZ + fYW);
    pOut[9] = 2.0f * (fYZ - fXW);
    pOut[10] = 1.0f - 2.0f * (fXX + fYY);
    pOut[11] = 0.0f;
    pOut[12] = vPos.x;   pOut[13] = vPos.y;   pOut[14] = vPos.z;   pOut[15] = 1.0f;
}

void MakeMatrix4x4Transpose(float* pOut, const Vector& vPos, const Quaternion& q)
{
    float fXX = q.x * q.x, fYY = q.y * q.y, fZZ = q.z * q.z;
    float fXY = q.x * q.y, fXZ = q.x * q.z, fXW = q.x * q.w;
    float fYZ = q.y * q.z, fYW = q.y * q.w, fZW = q.z * q.w;
    pOut[0] = -(1.0f - 2.0f * (fYY + fZZ));
    pOut[1] = -2.0f * (fXY + fZW);
    pOut[2] = -2.0f * (fXZ - fYW);
    pOut[3] = 0.0f;
    pOut[4] = 2.0f * (fXY - fZW);
    pOut[5] = 1.0f - 2.0f * (fZZ + fXX);
    pOut[6] = 2.0f * (fYZ + fXW);
    pOut[7] = 0.0f;
    pOut[8] = 2.0f * (fXZ + fYW);
    pOut[9] = 2.0f * (fYZ - fXW);
    pOut[10] = 1.0f - 2.0f * (fXX + fYY);
    pOut[11] = 0.0f;
    pOut[12] = vPos.x;   pOut[13] = vPos.y;   pOut[14] = vPos.z;   pOut[15] = 1.0f;
}

float* Homogenize(const Vector& v, const float& f)
{
    static float a[4];
    *reinterpret_cast<Vector*>(a) = v;
    a[3] = f;
    return a;
}

// A stub that returns b regardless of a and t.
bool Interpolate(const bool& a, const bool& b, float t)
{
    K2_UNUSED(a); K2_UNUSED(t);
    return b;
}

int Interpolate(const int& a, const int& b, float t)
{
    return static_cast<int>((1.0f - t) * static_cast<float>(a) + static_cast<float>(b) * t);
}

Vector InterpolateBezier(const Vector& v0, const Vector& v1, const Vector& v2, const Vector& v3,
                         float t)
{
    float fA = 1.0f - t;
    Vector vA = v0 * fA + v2 * t;
    Vector vB = v2 * fA + v3 * t;
    Vector vC = v3 * fA + v1 * t;
    Vector vD = vA * fA + vB * t;
    Vector vE = vB * fA + vC * t;
    return vD * fA + vE * t;
}

float InterpolateBezier(float f0, float f1, float f2, float f3, float t)
{
    float fA = 1.0f - t;
    float fA0 = f2 * t + f0 * fA;
    float fB0 = f3 * t + f2 * fA;
    float fC0 = f1 * t + f3 * fA;
    float fD0 = fA * fA0 + t * fB0;
    float fE0 = fB0 * fA + fC0 * t;
    return fA * fD0 + fE0 * t;
}

int quaternionidentityoptimization = 1;
uint64_t topMask[5] = {0xffff0000, 0xff00ff00, 0xf0f0f0f0, 0xcccccccc, 0xaaaaaaaa};

uint64_t FloorLogBase2(uint64_t x)
{
    uint64_t r = 0;

    bool b0 = (x & topMask[0]) != 0;
    if (b0) x &= topMask[0];
    r |= static_cast<uint64_t>(b0) << 4;

    bool b1 = (x & topMask[1]) != 0;
    if (b1) x &= topMask[1];
    r |= static_cast<uint64_t>(b1) << 3;

    bool b2 = (x & topMask[2]) != 0;
    if (b2) x &= topMask[2];
    r |= static_cast<uint64_t>(b2) << 2;

    bool b3 = (x & topMask[3]) != 0;
    if (b3) x &= topMask[3];
    r |= static_cast<uint64_t>(b3) << 1;

    bool b4 = (x & topMask[4]) != 0;
    if (b4) x &= topMask[4];
    r |= static_cast<uint64_t>(b4);

    return r;
}

int IntersectLineSegments2d(const Vector& a, const Vector& b, const Vector& c, const Vector& d,
                            Vector& out)
{
    float fUx = b.x - a.x;
    float fUy = b.y - a.y;
    float fVx = d.x - c.x;
    float fVy = d.y - c.y;
    float fAy = a.y;
    float fCy = c.y;
    float fDot = fUx * fVx + fUy * fVy;
    float fLen1 = sqrtf(fUx * fUx + fUy * fUy);
    float fLen2 = sqrtf(fVx * fVx + fVy * fVy);
    if (fDot / fLen1 / fLen2 == 1.0f)
        return 0;

    if (a.x == b.x)
    {
        out.x = a.x;
        float fCx = c.x;
        float fM2 = fVy / fVx;
        out.y = fCy - fM2 * fCx + fM2 * a.x;
        if (!fInRange(out.y, c.y, d.y))
            return 0;
        if (fOutOfRange(out.y, a.y, b.y))
            return 0;
        if (fOutOfRange(out.x, fCx, d.x))
            return 0;
    }
    else
    {
        float fM1 = fUy / fUx;
        if (c.x == d.x)
        {
            out.x = c.x;
            float fAx = a.x;
            out.y = fAy - fM1 * fAx + fM1 * c.x;
            if (!fInRange(out.y, a.y, b.y))
                return 0;
            if (fOutOfRange(out.y, c.y, d.y))
                return 0;
            if (fOutOfRange(out.x, fAx, b.x))
                return 0;
        }
        else
        {
            float fM2 = fVy / fVx;
            if (fM2 - fM1 == 0.0f)
                return 0;

            Vector vNormal(fUy, a.x - b.x, 0.0f);
            float fULen = sqrtf(fUy * fUy + fUx * fUx);
            if (fULen != 0.0f)
            {
                float fInv = 1.0f / fULen;
                vNormal.x *= fInv;
                vNormal.y *= fInv;
                vNormal.z *= fInv;
            }
            // The plane hit is computed in 2D, with a and c/d lifted with z = 0.
            Vector vA(a.x, a.y, 0.0f);
            Vector vC(c.x, c.y, 0.0f);
            Vector vD(d.x, d.y, 0.0f);
            out = planelineintersection(vNormal, -(vNormal.x * vA.x + vNormal.y * vA.y + vNormal.z * vA.z),
                                        vC, vD);

            if (fAbs(fM2) > 1.0f)
            {
                if (!fInRange(out.y, c.y, d.y))
                    return 0;
            }
            else
            {
                if (!fInRange(out.x, c.x, d.x))
                    return 0;
            }
            if (fAbs(fM1) > 1.0f)
            {
                if (fOutOfRange(out.y, a.y, b.y))
                    return 0;
            }
            else
            {
                if (fOutOfRange(out.x, a.x, b.x))
                    return 0;
            }
        }
    }

    // Interpolate z along c->d on the longer axis.
    float fT = (fAbs(c.x - d.x) >= fAbs(c.y - d.y) ? out.x - c.x : out.y - c.y)
             / (fAbs(c.x - d.x) >= fAbs(c.y - d.y) ? d.x - c.x : d.y - c.y);
    out.z = c.z + (d.z - c.z) * fT;
    return 1;
}

// EFFECTIVE (K1+K2): s4/s5 and s7/s16 swap between the discriminant/root and two dot-product temporaries;
// same instructions and operand order, armv7 exact in both games.
int IntersectCircle2d(const Vector& vCenter, float fRadius, const Vector& vStart,
                      const Vector& vEnd, Vector& vOut1, Vector& vOut2, bool bUnused)
{
    K2_UNUSED(bUnused);
    Vector vDir = vEnd - vStart;
    Vector vToStart = vStart - vCenter;
    float fA = vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z;
    float fDot = vDir.x * vToStart.x + vDir.y * vToStart.y + vDir.z * vToStart.z;
    float fC = vToStart.x * vToStart.x + vToStart.y * vToStart.y + vToStart.z * vToStart.z
               - fRadius * fRadius;
    float fB = 2.0f * fDot;
    float fDisc = fB * fB + fA * fC * -4.0f;
    if (fDisc == 0.0f)
    {
        float t = fB / (-2.0f * fA);
        if (t >= 0.0f && t <= 1.0f)
        {
            vOut1 = vStart + vDir * t;
            return 1;
        }
        return 0;
    }
    if (fDisc > 0.0f)
    {
        float fRoot = sqrtf(fDisc);
        float fNegB = -fB;
        float fTwoA = 2.0f * fA;
        int nHits = 1;
        float t1 = (fRoot - fB) / fTwoA;
        if (t1 >= 0.0f && t1 <= 1.0f)
        {
            vOut1 = vStart + vDir * t1;
            nHits = 2;
        }
        float t2 = (fNegB - fRoot) / fTwoA;
        if (t2 >= 0.0f && t2 <= 1.0f)
        {
            Vector vHit = vStart + vDir * t2;
            if (nHits != 2)
            {
                vOut1 = vHit;
                return 1;
            }
            vOut2 = vHit;
            return 2;
        }
        return nHits - 1;
    }
    return 0;
}

Quaternion::Quaternion(Vector axis, float angle)
{
    if (angle == 0.0f)
    {
        w = 1.0f;
        x = y = z = 0.0f;
        return;
    }
    Vector vAxis = normalize(axis);
    float fSin = sinf(angle * 0.5f);
    float fCos = cosf(angle * 0.5f);
    Vector vScaled = vAxis * fSin;
    w = fCos;
    x = vScaled.x;
    y = vScaled.y;
    z = vScaled.z;
}
