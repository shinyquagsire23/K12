#ifndef SAFEPOINTER_H
#define SAFEPOINTER_H

// SafePointer<T>: a pointer to a T that unregisters itself when the pointee dies. The pointee
// owns the SafePointerList<T> that the pointers aimed at it join, so it can clear every pointer
// still watching it.

#include "List.h"

template <class T> class SafePointer;

// The pointers watching one object. Elements are the watchers' own addresses, so this is a
// List<SafePointer<T>*>.
template <class T>
class SafePointerList : public List<SafePointer<T>*> {
public:
    // Reserves capacity 2 before delegating to the base Add.
    void AddPointer(SafePointer<T>* pPointer) {
#if !KOTOR1  // K1 has no grow-to-2 pre-pass
        if (this->m_capacity == 0)
            this->allocate(2);
#endif
        this->Add(pPointer);
    }

    // Clears every pointer still watching this object, back to front, then releases the array.
    ~SafePointerList() {
        for (int i = this->m_count - 1; i >= 0; i--)
            *this->m_pData[i] = 0;
        this->SetSize(0);
    }
};

template <class T>
class SafePointer {
public:
    T* m_pObject;

    SafePointer() : m_pObject(0) {}

    // Lets a SafePointer be used where a raw T* is expected.
    operator T*() const { return m_pObject; }

    // Constructs already bound.
    explicit SafePointer(T* pObject) : m_pObject(pObject) {
        if (m_pObject != 0)
            m_pObject->AddPointer(this);
    }

    // A copy registers itself with the pointee as another watcher.
    SafePointer(const SafePointer<T>& rhs) : m_pObject(rhs.m_pObject) {
        if (m_pObject != 0)
            m_pObject->AddPointer(this);
    }

    // Rebinding unregisters from the old pointee first, so the entry moves to the end of the
    // watcher list. There is no self-assignment guard.
    SafePointer<T>& operator=(T* pNew) {
        if (m_pObject != 0)
            m_pObject->Remove(this);
        m_pObject = pNew;
        if (m_pObject != 0)
            m_pObject->AddPointer(this);
        return *this;
    }

    // Delegates to the pointer overload so copies rebind through the watcher list.
    SafePointer<T>& operator=(const SafePointer<T>& rhs) {
        return *this = rhs.m_pObject;
    }

    // Unregisters from the pointee; does not clear the watched pointer.
    ~SafePointer() {
        if (m_pObject != 0)
            m_pObject->Remove(this);
    }
};

#endif // SAFEPOINTER_H
