#pragma once

#include "exoresman.h"

class CAurObject;

class CSWPortrait {
public:
    // The last id SetPortraitId accepts (0xfffe and 0xffff are reserved), and the id of an object
    // with no portrait of its own.
    enum {
        PORTRAIT_ID_LAST = 0xfffd,
        PORTRAIT_ID_NONE = 0xffff,
    };

    CResRef m_baseResRef;

    CSWPortrait();
    ~CSWPortrait();
    CResRef GetPortrait(unsigned char nState);
    int     SetPortraitId(unsigned short a);

    void    SetBaseResRef(CResRef resRef) { m_baseResRef = resRef; }

    static int ReplacePortraitTexture(CAurObject* pObject, const CResRef& resRef);
};
