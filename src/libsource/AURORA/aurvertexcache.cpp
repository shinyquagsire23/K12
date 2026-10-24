#include "k2stdint.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library (List.h grows from 8)
#endif
#include "aurvertexcache.h"

VertexCache::VertexCache() {
}

VertexCache::VertexCache(int size) {
    m_nSize = size;
    m_pEntries = new int[size];
    for (int64_t i = 0; i < size; i++)
        m_pEntries[i] = -1;
}

VertexCache::~VertexCache() {
    delete[] m_pEntries;
}

int VertexCache::At(int index) {
    return m_pEntries[index];
}

void VertexCache::Set(int index, int value) {
    m_pEntries[index] = value;
}

void VertexCache::Clear() {
    for (int i = 0; i < m_nSize; i++)
        m_pEntries[i] = -1;
}

// Copies this cache's entries into the argument.
void VertexCache::Copy(VertexCache* pOther) {
    for (int i = 0; i < m_nSize; i++)
        pOther->m_pEntries[i] = m_pEntries[i];
}
