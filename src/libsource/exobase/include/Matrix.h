#ifndef MATRIX_H
#define MATRIX_H

// idiom-exempt: canonical
#include "Quaternion.h"
#include "Vector.h"

// The engine's 3x3 rotation matrix, nine floats, row-major.
class Matrix {
public:
    float m_f[9];

    Matrix()
    {
        m_f[0] = 1.0f; m_f[1] = 0.0f; m_f[2] = 0.0f;
        m_f[3] = 0.0f; m_f[4] = 1.0f; m_f[5] = 0.0f;
        m_f[6] = 0.0f; m_f[7] = 0.0f; m_f[8] = 1.0f;
    }

    Matrix(Vector a, Vector b, Vector c)
    {
        m_f[0] = a.x; m_f[1] = a.y; m_f[2] = a.z;
        m_f[3] = b.x; m_f[4] = b.y; m_f[5] = b.z;
        m_f[6] = c.x; m_f[7] = c.y; m_f[8] = c.z;
    }

    Vector* Rows() { return reinterpret_cast<Vector*>(m_f); }  // raw-offset: the nine floats as three Vector rows

    Quaternion getquaternion() const;
};

Vector operator*(Matrix const& m, Vector const& v);

inline Matrix operator+(const Matrix& a, const Matrix& b) {
    Matrix r;
    for (int i = 0; i < 9; i++)
        r.m_f[i] = a.m_f[i] + b.m_f[i];
    return r;
}

Matrix Transpose(const Matrix& m);

// Quaternion to rotation matrix, row-vector convention.
inline Matrix Quaternion::getmatrix() const { return Matrix(xdir(), ydir(), zdir()); }

#endif
