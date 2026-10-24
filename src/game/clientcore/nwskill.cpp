
// One row of skill.2da.

#include "nwskill.h"
#include "exostring.h"
#include "exolocstringinternal.h"

CSWSkill::CSWSkill()
{
    m_nNameStrRef = -1;
    m_nDescriptionStrRef = -1;
    m_flags = (m_flags & ~FLAG_MASK) | FLAG_ALL_CLASSES_CAN_USE | FLAG_NPC_CAN_USE | FLAG_DROID_CAN_USE;
}

CSWSkill::~CSWSkill()
{
}

// Looked up as GENDER_MALE, like the other skill and feat accessors.
CExoString CSWSkill::GetDescriptionText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nDescriptionStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}

CExoString CSWSkill::GetNameText()
{
    STR_RES res;
    g_pTlkTable->Fetch(m_nNameStrRef, res, CTlkTable::GENDER_MALE);
    return res.text;
}
