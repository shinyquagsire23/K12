#ifndef AURVERTEXCACHE_H
#define AURVERTEXCACHE_H

// NvTriStrip's vertex cache: a fixed-size array of entry slots with the recent-entry helpers.
class VertexCache {
public:
    VertexCache();
    VertexCache(int size);
    ~VertexCache();
    int  At(int index);
    void Set(int index, int value);
    void Clear();
    void Copy(VertexCache* pOther);

    // NvTriStrip's two header-defined members, expanded into every cache helper.
    bool InCache(int entry)
    {
        bool returnVal = false;
        for (int i = 0; i < m_nSize; i++) {
            if (m_pEntries[i] == entry) {
                returnVal = true;
                break;
            }
        }
        return returnVal;
    }

    int AddEntry(int entry)
    {
        int removed = m_pEntries[m_nSize - 1];
        for (int i = m_nSize - 2; i >= 0; i--)
            m_pEntries[i + 1] = m_pEntries[i];
        m_pEntries[0] = entry;
        return removed;
    }

private:
    int* m_pEntries;
    int  m_nSize;
};

#endif
