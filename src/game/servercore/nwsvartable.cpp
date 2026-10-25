#include "k2stdint.h"
#include "nwsvartable.h"
#include "exostring.h"
#include "gameobjectarray.h"
#include "nwsjournal.h"
#include "nwscreature.h"
#include "nwsobject.h"
#include "resgff.h"

#include "port/libcstring.h"
#include "exostringinline.h"

// Entries named "NW_JOURNAL*" mirror the owning creature's journal state: "NW_JOURNAL_ENTRY<tag>",
// "NW_JOURNAL_DATE<tag>", "NW_JOURNAL_TIME<tag>" and "NW_JOURNAL_PICTURE<tag>". The letter after
// "NW_JOURNAL_" picks the field, and the quest tag follows the field's name.
enum {
    JOURNAL_VAR_PREFIX_LENGTH   = 10,   // "NW_JOURNAL"
    JOURNAL_VAR_KIND_INDEX      = 11,   // the E, D, T or P after "NW_JOURNAL_"
    JOURNAL_VAR_ENTRY_TAG       = 16,   // the tag's offset after "NW_JOURNAL_ENTRY"
    JOURNAL_VAR_DATE_TAG        = 15,   // after "NW_JOURNAL_DATE" and "NW_JOURNAL_TIME"
    JOURNAL_VAR_PICTURE_TAG     = 18,   // after "NW_JOURNAL_PICTURE"
    JOURNAL_VAR_TAG_MAX_LENGTH  = 128,
};

#define NW_JOURNAL_OWNER_AND_LENGTH(sName) \
    (m_pOwner != 0 && (sName).GetLength() >= JOURNAL_VAR_KIND_INDEX)

const int CSWSScriptVarTable::VARTYPE_INTEGER  = 1;
const int CSWSScriptVarTable::VARTYPE_FLOAT    = 2;
const int CSWSScriptVarTable::VARTYPE_STRING   = 3;
const int CSWSScriptVarTable::VARTYPE_OBJECT   = 4;
const int CSWSScriptVarTable::VARTYPE_LOCATION = 5;

// Finds the entry for a name and type, creating an empty one when bCreate is set.
CSWSScriptVar* CSWSScriptVarTable::MatchIndex(const CExoString& sName, unsigned int nType, int bCreate)
{
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType == static_cast<int>(nType))
        {
            if (m_vars.m_pArray[i].m_sName == sName)
                return &m_vars.m_pArray[i];
        }
    }
    if (bCreate == 0)
        return 0;

    CSWSScriptVar var;
    var.m_sName = sName;
    var.m_nType = static_cast<int>(nType);
    var.m_oObject = 0;
    if (static_cast<int>(nType) == VARTYPE_STRING)
    {
        CExoString* pString = new CExoString();
        var.m_pString = pString;
    }
    else if (static_cast<int>(nType) == VARTYPE_LOCATION)
    {
        CScriptLocation* pLocation = new CScriptLocation();
        var.m_pLocation = pLocation;
    }

    m_vars.Add(var);
    int nIndex = m_vars.m_count - 1;
    return &m_vars.m_pArray[nIndex];
}

int CSWSScriptVarTable::GetInt(const CExoString& sName)
{
    CSWSScriptVar* pVar = 0;
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType != VARTYPE_INTEGER)
            continue;
        if (m_vars.m_pArray[i].m_sName == sName)
        {
            pVar = &m_vars.m_pArray[i];
            break;
        }
    }
    if (pVar != 0)
        return pVar->m_nInt;
    return 0;
}

float CSWSScriptVarTable::GetFloat(const CExoString& sName)
{
    float fResult = 0.0f;
    CSWSScriptVar* pVar = 0;
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType != VARTYPE_FLOAT)
            continue;
        if (m_vars.m_pArray[i].m_sName == sName)
        {
            pVar = &m_vars.m_pArray[i];
            break;
        }
    }
    if (pVar != 0)
        fResult = pVar->m_fFloat;
    return fResult;
}

#if KOTOR1  // K1 inlines this into SaveVarTable, its only caller
__attribute__((always_inline))
#endif
CExoString CSWSScriptVarTable::GetString(const CExoString& sName)
{
    CExoString sResult;
    CSWSScriptVar* pVar = 0;
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType != VARTYPE_STRING)
            continue;
        if (m_vars.m_pArray[i].m_sName == sName)
        {
            pVar = &m_vars.m_pArray[i];
            break;
        }
    }
    if (pVar != 0)
        sResult = *pVar->m_pString;
    return sResult;
}

// K1 declares this without the trailing A.
#if KOTOR1
unsigned long long CSWSScriptVarTable::GetObject(const CExoString& sName)
#else
unsigned long long CSWSScriptVarTable::GetObjectA(const CExoString& sName)
#endif
{
    unsigned long long oResult = CGameObjectArray::INVALIDOBJID;
    CSWSScriptVar* pVar = 0;
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType != VARTYPE_OBJECT)
            continue;
        if (m_vars.m_pArray[i].m_sName == sName)
        {
            pVar = &m_vars.m_pArray[i];
            break;
        }
    }
    if (pVar != 0)
        oResult = pVar->m_oObject;
    return oResult;
}

CScriptLocation CSWSScriptVarTable::GetLocation(const CExoString& sName)
{
    CScriptLocation result;
    CSWSScriptVar* pVar = 0;
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType != VARTYPE_LOCATION)
            continue;
        if (m_vars.m_pArray[i].m_sName == sName)
        {
            pVar = &m_vars.m_pArray[i];
            break;
        }
    }
    if (pVar != 0)
        result = *pVar->m_pLocation;
    return result;
}

// Writing an NW_JOURNAL entry also updates the owning creature's journal.
void CSWSScriptVarTable::SetInt(const CExoString& sName, int nValue, int nFlag)
{
    CSWSScriptVar* pVar = MatchIndex(sName, VARTYPE_INTEGER, 1);
    pVar->m_oObject = static_cast<unsigned int>(nValue);

    if (NW_JOURNAL_OWNER_AND_LENGTH(sName) && m_pOwner->GetJournal() != 0
        && sName.Left(JOURNAL_VAR_PREFIX_LENGTH) == "NW_JOURNAL")
    {
        if (sName[JOURNAL_VAR_KIND_INDEX] == 'E')
            m_pOwner->GetJournal()->SetState(sName.SubString(JOURNAL_VAR_ENTRY_TAG, JOURNAL_VAR_TAG_MAX_LENGTH), nValue, nFlag);
        else if (sName[JOURNAL_VAR_KIND_INDEX] == 'D')
            m_pOwner->GetJournal()->SetDate(sName.SubString(JOURNAL_VAR_DATE_TAG, JOURNAL_VAR_TAG_MAX_LENGTH), nValue, nFlag);
        else if (sName[JOURNAL_VAR_KIND_INDEX] == 'T')
            m_pOwner->GetJournal()->SetTime(sName.SubString(JOURNAL_VAR_DATE_TAG, JOURNAL_VAR_TAG_MAX_LENGTH), nValue, nFlag);
        else if (sName[JOURNAL_VAR_KIND_INDEX] == 'P')
            m_pOwner->GetJournal()->SetPicture(sName.SubString(JOURNAL_VAR_PICTURE_TAG, JOURNAL_VAR_TAG_MAX_LENGTH), nValue, nFlag);
    }
}

// Stores through the union's 8-byte arm; only the low 4 bytes hold the float.
void CSWSScriptVarTable::SetFloat(const CExoString& sName, float fValue)
{
    CSWSScriptVar* pVar = MatchIndex(sName, VARTYPE_FLOAT, 1);
    pVar->m_oObject = *reinterpret_cast<unsigned long long*>(&fValue);
}

// Assigns through the entry's own heap CExoString.
void CSWSScriptVarTable::SetString(const CExoString& sName, CExoString& sValue)
{
    CSWSScriptVar* pVar = MatchIndex(sName, VARTYPE_STRING, 1);
    *pVar->m_pString = sValue;
}

void CSWSScriptVarTable::SetObject(const CExoString& sName, unsigned long long oValue)
{
    CSWSScriptVar* pVar = MatchIndex(sName, VARTYPE_OBJECT, 1);
    pVar->m_oObject = oValue;
}

void CSWSScriptVarTable::SetLocation(const CExoString& sName, CScriptLocation loc)
{
    CSWSScriptVar* pVar = MatchIndex(sName, VARTYPE_LOCATION, 1);
    *pVar->m_pLocation = loc;
}

// Erases the first entry matching the type and name.
void CSWSScriptVarTable::DeleteIndex(const CExoString& sName, unsigned int nType)
{
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType == static_cast<int>(nType) && m_vars.m_pArray[i].m_sName == sName)
        {
            m_vars.DelIndex(i);
            break;
        }
    }
}

// Destroying an NW_JOURNAL int also drops the journal entry it backed.
void CSWSScriptVarTable::DestroyInt(const CExoString& sName)
{
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType == VARTYPE_INTEGER && m_vars.m_pArray[i].m_sName == sName)
        {
            m_vars.DelIndex(i);
            break;
        }
    }

    // The journal pointer is not tested here, unlike in SetInt.
    if (NW_JOURNAL_OWNER_AND_LENGTH(sName) && sName.Left(JOURNAL_VAR_PREFIX_LENGTH) == "NW_JOURNAL")
        m_pOwner->GetJournal()->Destroy(sName.SubString(JOURNAL_VAR_ENTRY_TAG, JOURNAL_VAR_TAG_MAX_LENGTH));
}

void CSWSScriptVarTable::DestroyFloat(const CExoString& sName)
{
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType == VARTYPE_FLOAT && m_vars.m_pArray[i].m_sName == sName)
        {
            m_vars.DelIndex(i);
            break;
        }
    }
}

// The entry's heap CExoString is freed by the destructor, not here.
void CSWSScriptVarTable::DestroyString(const CExoString& sName)
{
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType == VARTYPE_STRING && m_vars.m_pArray[i].m_sName == sName)
        {
            m_vars.DelIndex(i);
            break;
        }
    }
}

void CSWSScriptVarTable::DestroyObject(const CExoString& sName)
{
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType == VARTYPE_OBJECT && m_vars.m_pArray[i].m_sName == sName)
        {
            m_vars.DelIndex(i);
            break;
        }
    }
}

void CSWSScriptVarTable::DestroyLocation(const CExoString& sName)
{
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType == VARTYPE_LOCATION && m_vars.m_pArray[i].m_sName == sName)
        {
            m_vars.DelIndex(i);
            break;
        }
    }
}

// Frees the heap-owned string and location entries; the in-class CExoArrayList frees the elements.
CSWSScriptVarTable::~CSWSScriptVarTable()
{
    for (int i = 0; i < m_vars.m_count; i++)
    {
        if (m_vars.m_pArray[i].m_nType == VARTYPE_STRING && m_vars.m_pArray[i].m_pString != 0)
            delete m_vars.m_pArray[i].m_pString;
        if (m_vars.m_pArray[i].m_nType == VARTYPE_LOCATION && m_vars.m_pArray[i].m_pLocation != 0)
            delete m_vars.m_pArray[i].m_pLocation;
    }
}

// Deserializes the "VarTable" list, one row of {Name, Type, Value} per entry.
void CSWSScriptVarTable::LoadVarTable(CResGFF* pGff, CResStruct* pStruct)
{
    CScriptLocation location;
    CExoString sVarName;
    CExoString sValue;
    CResList list;
    CResStruct row;
    CResStruct sub;
    int bSuccess;

    if (!pGff->GetList(&list, pStruct, const_cast<char*>("VarTable")))
        return;

    for (unsigned int i = 0; i < pGff->GetListCount(&list); i++)
    {
        pGff->GetListElement(&row, &list, i);
        sVarName = pGff->ReadFieldCExoString(&row, const_cast<char*>("Name"), bSuccess, "");
        unsigned int nType = pGff->ReadFieldDWORD(&row, const_cast<char*>("Type"), bSuccess, 0);

        switch (nType)
        {
        case VARTYPE_INTEGER:
            SetInt(sVarName, pGff->ReadFieldINT(&row, const_cast<char*>("Value"), bSuccess, 0), 1);
            break;
        case VARTYPE_FLOAT:
            SetFloat(sVarName, pGff->ReadFieldFLOAT(&row, const_cast<char*>("Value"), bSuccess, 0.0f));
            break;
        case VARTYPE_STRING:
            sValue = pGff->ReadFieldCExoString(&row, const_cast<char*>("Value"), bSuccess, "");
            SetString(sVarName, sValue);
            break;
        case VARTYPE_OBJECT:
            SetObject(sVarName, pGff->ReadFieldDWORD(&row, const_cast<char*>("Value"), bSuccess, 0));
            break;
        case VARTYPE_LOCATION:
            pGff->GetStructFromStruct(&sub, &row, const_cast<char*>("Value"));
            location.LoadLocation(pGff, &sub);
            SetLocation(sVarName, location);
            break;
        }
    }
}

// Serializes the table into the "VarTable" list, one row of {Name, Type, Value} per entry. Each
// value is read back through the matching getter rather than from the loop entry.
void CSWSScriptVarTable::SaveVarTable(CResGFF* pGff, CResStruct* pStruct)
{
    CScriptLocation location;
    CResList list;
    CResStruct row;
    CResStruct sub;

    pGff->AddList(&list, pStruct, const_cast<char*>("VarTable"));

    for (int i = 0; i < m_vars.m_count; i++)
    {
        pGff->AddListElement(&row, &list, 0);
        pGff->WriteFieldCExoString(&row, m_vars.m_pArray[i].m_sName, const_cast<char*>("Name"));
        pGff->WriteFieldDWORD(&row, m_vars.m_pArray[i].m_nType, const_cast<char*>("Type"));

        switch (m_vars.m_pArray[i].m_nType)
        {
        case VARTYPE_INTEGER:
            pGff->WriteFieldINT(&row, GetInt(m_vars.m_pArray[i].m_sName), const_cast<char*>("Value"));
            break;
        case VARTYPE_FLOAT:
            pGff->WriteFieldFLOAT(&row, GetFloat(m_vars.m_pArray[i].m_sName), const_cast<char*>("Value"));
            break;
        case VARTYPE_STRING:
        {
            CExoString sValue = GetString(m_vars.m_pArray[i].m_sName);
            pGff->WriteFieldCExoString(&row, sValue, const_cast<char*>("Value"));
            break;
        }
        case VARTYPE_OBJECT:
            pGff->WriteFieldDWORD(&row, static_cast<unsigned int>(GetObjectA(m_vars.m_pArray[i].m_sName)), const_cast<char*>("Value"));
            break;
        case VARTYPE_LOCATION:
            location = GetLocation(m_vars.m_pArray[i].m_sName);
            pGff->AddStructToStruct(&sub, &row, const_cast<char*>("Value"), 1);
            location.SaveLocation(pGff, &sub);
            break;
        }
    }
}
