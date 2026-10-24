#pragma once

#include "exoresman.h"

class CExoString;
struct STR_RES;

// One row of the cleric-domain table: a name and description StrRef pair, the row's own CResRef key
// and four bookkeeping words.
class CSWDomain {
public:
    int m_nNameStrRef;
    int m_nDescriptionStrRef;
    CResRef m_resRef;
    int m_nReserved1;
    unsigned short m_nReserved2;
    int m_nReserved3;
    int m_nReserved4;

    CSWDomain();
    ~CSWDomain();
    CExoString GetDescriptionText();
    CExoString GetNameText();
};
