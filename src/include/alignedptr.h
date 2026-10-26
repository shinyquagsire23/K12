#pragma once
#include "k2stdint.h"

#if KOTOR1
// bionic's _FORTIFY_SOURCE=2 memcpy (bits/fortify/string.h), which K1's unaligned stores went
// through: an always-inline wrapper whose destination carries its object size. The engine's own
// declarations (port/libcstring.h) leave memcpy an opaque call, and bionic's header cannot be
// included beside them.
static inline __attribute__((always_inline)) void* K1FortifiedMemcpy(
    void* const pDst __attribute__((pass_object_size(0))), const void* pSrc, size_t nSize)
{
    size_t nDstSize = __builtin_object_size(pDst, 0);
    if (nDstSize == static_cast<size_t>(-1) || nDstSize >= nSize)
        return __builtin_memcpy(pDst, pSrc, nSize);
    return __builtin___memcpy_chk(pDst, pSrc, nSize, nDstSize);
}

// K1 only: a pointer into a possibly unaligned buffer, carried with an alignment flag so elements
// can be read and written through memcpy when the pointer is misaligned.
template <class T>
struct CAlignedPtr {
    T*   m_p;
    bool m_bAligned;

    CAlignedPtr() : m_p(0), m_bAligned(true) {}
    CAlignedPtr(T* p) : m_p(p), m_bAligned((reinterpret_cast<uint64_t>(p) & 3) == 0) {}
    CAlignedPtr(const CAlignedPtr& o) : m_p(o.m_p), m_bAligned(o.m_bAligned) {}
    CAlignedPtr& operator=(T* p)
    {
        m_p = p;
        m_bAligned = (reinterpret_cast<uint64_t>(p) & 3) == 0;
        return *this;
    }
    operator T*() const { return m_p; }
    CAlignedPtr operator+(unsigned int n) const { return CAlignedPtr(m_p + n); }

    T Get() const
    {
        if (m_bAligned) {
            return *m_p;
        }
        T t;
        __builtin_memcpy(&t, reinterpret_cast<const char*>(m_p), sizeof(T));
        return t;
    }
    T Get(int n) const
    {
        T t;
        if (m_bAligned) {
            t = m_p[n];
        } else {
            __builtin_memcpy(&t, reinterpret_cast<const char*>(m_p + n), sizeof(T));
        }
        return t;
    }
    void Set(const T& t)
    {
        if (m_bAligned) {
            *m_p = t;
        } else {
            K1FortifiedMemcpy(reinterpret_cast<char*>(m_p), &t, sizeof(T));
        }
    }
    void Set(int n, const T& t)
    {
        if (m_bAligned) {
            m_p[n] = t;
        } else {
            K1FortifiedMemcpy(reinterpret_cast<char*>(m_p + n), &t, sizeof(T));
        }
    }
    void SetAt(unsigned int n, const T& t)
    {
        if (m_bAligned) {
            m_p[n] = t;
        } else {
            K1FortifiedMemcpy(reinterpret_cast<char*>(m_p) + n * sizeof(T), &t, sizeof(T));  // raw-offset: the aligned-pointer helper steps by sizeof(T)
        }
    }
    // Frees the buffer and nulls the pointer; the alignment flag is left alone.
    void Delete()
    {
        if (m_p != 0) {
            delete m_p;
            m_p = 0;
        }
    }
};
#endif
