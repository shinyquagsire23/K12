#include "nwdomain.h"

#include "exolocstringinternal.h"
#include "exotlktable.h"

CSWDomain::CSWDomain()
{
    m_nNameStrRef = -1;
    m_nDescriptionStrRef = -1;
    m_nReserved1 = -1;
    m_nReserved2 = 0xffff;
    m_nReserved3 = 1;
    m_nReserved4 = 0;
}

CSWDomain::~CSWDomain()
{
}

CExoString CSWDomain::GetNameText()
{
    STR_RES strRes;
    g_pTlkTable->Fetch(m_nNameStrRef, strRes, CTlkTable::GENDER_MALE);
    return strRes.text;
}

CExoString CSWDomain::GetDescriptionText()
{
    STR_RES strRes;
    g_pTlkTable->Fetch(m_nDescriptionStrRef, strRes, CTlkTable::GENDER_MALE);
    return strRes.text;
}
