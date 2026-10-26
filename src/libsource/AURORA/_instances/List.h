#ifndef LIST_H
#define LIST_H
#include "k2stdint.h"

// Placement new/delete are declared here rather than pulled in with <new>, whose CRT declarations
// clash with this tree's hand-written prototypes. Declarations only: a definition here would collide
// with both libc++'s <new> and the definitions a few translation units carry themselves. The size is
// the compiler's own size_t.
#if defined(_MSC_VER)
#include <new>
#else
void* operator new(__SIZE_TYPE__, void* p) throw();
void  operator delete(void*, void*) throw();
#endif

// List<T> is the renderer's own dynamic array: a non-polymorphic container with a heap element
// array, a live count and an allocated capacity. ~List() is a plain delete[] for every
// trivially-destructible element type.
//
// Add appends one element, growing the buffer to twice the count (16 for the first element) and
// copying the old elements over. operator= clears the list and then Adds every element of the
// source; the copy constructor zeroes the fields and Adds every element.
template <class T>
class List {
public:
    // The count and capacity are cleared together.
    List() { m_pData = 0; __builtin_memset(&m_count, 0, sizeof(m_count) + sizeof(m_capacity)); }
    ~List();
    List(List<T>& rhs);
    // Grows the buffer and moves the existing elements over. Shared by Add and SetSize, and
    // called directly to reserve a small capacity.
    void allocate(int nNewCapacity);
    // Resizes to an exact count: a non-zero size goes through allocate, zero frees the buffer and
    // clears the capacity.
    void SetSize(int nNewSize);
    void Add(T item);
    T& operator[](int nIndex) { return m_pData[nIndex]; }
    // Counts the matching elements; the whole list is scanned rather than stopping at the first.
    template <class U = T> int Contains(U item) const
    {
        int nFound = 0;
        for (int64_t i = 0; i < m_count; i++)
            if (m_pData[i] == item)
                nFound++;
        return nFound;
    }

    // Index of the first match, or -1.
    template <class U = T> int IndexOf(U item) const
    {
        for (int i = 0; i < m_count; i++)
            if (m_pData[i] == item)
                return i;
        return -1;
    }

    // Appends the item only when it is not already present.
    void AddUnique(T item)
    {
        if (!Contains(item))
            Add(item);
    }

    // Appends the item at the end and shifts the tail up, so the item is written twice.
    void Insert(T item, int nIndex);
    // Erases the element at nIndex and shifts the tail down; no bounds check.
    void DelIndex(int nIndex);
    template <class U = T> void Remove(U item);
    List<T>& operator=(List<T>& rhs);

    T* m_pData;
    int m_count;
    int m_capacity;
};

template <class T>
List<T>::~List() {
    delete[] m_pData;
}

// Clear, then Add every element of the source, reusing the grow path rather than sizing exactly.
template <class T>
List<T>& List<T>::operator=(List<T>& rhs) {
    m_count = 0;
    for (int i = 0; i < rhs.m_count; i++)
        Add(rhs.m_pData[i]);
    return *this;
}

template <class T>
List<T>::List(List<T>& rhs) : m_pData(0), m_count(0), m_capacity(0) {
    for (int i = 0; i < rhs.m_count; i++)
        Add(rhs.m_pData[i]);
}

// Erase the last matching element: a backwards scan, then the shift is delegated to DelIndex,
// which is also what hands it -1 when nothing matched.
template <class T>
template <class U>
void List<T>::Remove(U item) {
    int i = m_count - 1;
    while (i >= 0 && m_pData[i] != item)
        i--;
#ifdef K2_FIX_ENGINE_BUGS
    if (i < 0)
        return;
#endif
    DelIndex(i);
}

template <class T>
void List<T>::allocate(int nNewCapacity) {
    T* pOld = m_pData;
    m_capacity = nNewCapacity;
    m_pData = new T[nNewCapacity];
#ifdef K2_FIX_ENGINE_BUGS
    int nKeep = m_count < nNewCapacity ? m_count : nNewCapacity;
    for (int i = 0; i < nKeep; i++)
#else
    for (int i = 0; i < m_count; i++)
#endif
        m_pData[i] = pOld[i];
    delete[] pOld;
}

template <class T>
#if KOTOR1
inline  // K1 inlines SetSize at every call site
#endif
void List<T>::SetSize(int nNewSize) {
    if (nNewSize) {
        allocate(nNewSize);
    } else {
        if (m_pData) {
            delete[] m_pData;
            m_pData = 0;
        }
        m_capacity = 0;
    }
    m_count = nNewSize;
}

#if KOTOR1 && defined(K1_AURORA_LIST)
// K1's renderer library copy grows an empty list to 8; a game translation unit's copy specializes
// this to 16.
template <class T> struct K1ListFirstGrow { enum { value = 8 }; };
#endif

template <class T>
void List<T>::Add(T item) {
    if (m_count == m_capacity)
#if KOTOR1 && defined(K1_AURORA_LIST)
        allocate(m_count ? m_count * 2 : static_cast<int>(K1ListFirstGrow<T>::value));
#else
        allocate(m_count ? m_count * 2 : 16);
#endif
    m_pData[m_count++] = item;
}

template <class T>
void List<T>::DelIndex(int nIndex) {
    m_count--;
    for (int i = nIndex; i < m_count; i++)
        m_pData[i] = m_pData[i + 1];
}

template <class T>
void List<T>::Insert(T item, int nIndex) {
    int i = m_count;
    Add(item);
    while (i > nIndex) {
        m_pData[i] = m_pData[i - 1];
        i--;
    }
    m_pData[nIndex] = item;
}

#endif
