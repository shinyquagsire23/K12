#ifndef CEXOCRITICALSECTION_H
#define CEXOCRITICALSECTION_H

// idiom-exempt: canonical
// Win32 CRITICAL_SECTION behind CExoCriticalSection; consumers embed the facade by value.
class CExoCriticalSectionInternal;

class CExoCriticalSection {
public:
    CExoCriticalSection();
    ~CExoCriticalSection();
    void EnterCriticalSection();
    void LeaveCriticalSection();
private:
    CExoCriticalSectionInternal* m_pInternal;
};

#endif
