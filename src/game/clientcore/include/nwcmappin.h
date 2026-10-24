// Client-side map-pin object. No extra fields yet.
#ifndef CSWCMAPPIN_H
#define CSWCMAPPIN_H

#include "nwcobject.h"
#include "exostring.h"

class CResRef;

class CSWCMapPin : public CSWCObject {
public:

    virtual ~CSWCMapPin();

    // Copies the mutable state of another pin. Takes a pointer, so it is a separate overload
    // from the implicit copy assignment.
    CSWCMapPin& operator=(CSWCMapPin* pOther);
    CSWCMapPin();
    virtual void AIUpdate();
    virtual int LoadModel(const CResRef& r, unsigned char a, unsigned char b);
    virtual void UnloadModel();
    virtual void UnloadModel(unsigned char a);

    int                m_bActive;
    char               _pad14c[0x150-0x14c];
    CExoString         m_sName;
    unsigned long long m_nPinId;
    int                m_nPinType;
};

#endif // CSWCMAPPIN_H
