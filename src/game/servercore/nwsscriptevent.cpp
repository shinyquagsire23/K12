
#include "k2stdint.h"

#include "nwsscriptevent.h"

#include "CExoArrayListImpl.h"
#include "resgff.h"
#include "exostring.h"
#include "gameobjectarray.h"

// The struct id SaveEvent tags each parameter list element with.
enum {
    GFF_STRUCT_ID_EVENT_PARAMETER = 0x69,
};

CScriptEvent::CScriptEvent()
    : m_params()
{
}

int CScriptEvent::GetInteger(int index)
{
    if (index < m_params.m_aIntegers.m_count)
        return m_params.m_aIntegers.m_pArray[index];
    return 0;
}

float CScriptEvent::GetFloat(int index)
{
    float fValue = 0.0f;
    if (index < m_params.m_aFloats.m_count)
        fValue = m_params.m_aFloats.m_pArray[index];
    return fValue;
}

// An absent object is reported as the "no object" sentinel id.
unsigned long long CScriptEvent::GetObjectID(int index)
{
    if (index < m_params.m_aObjects.m_count)
        return m_params.m_aObjects.m_pArray[index];
    return OBJECT_INVALID;
}

CExoString CScriptEvent::GetString(int index)
{
    if (index < m_params.m_aStrings.m_count)
        return m_params.m_aStrings.m_pArray[index];
    return CExoString("");
}

void CScriptEvent::SetInteger(int index, int value)
{
    if (index >= m_params.m_aIntegers.m_count)
        m_params.m_aIntegers.SetSize(index + 1);
    m_params.m_aIntegers.m_pArray[index] = value;
}

void CScriptEvent::SetFloat(int index, float value)
{
    if (index >= m_params.m_aFloats.m_count)
        m_params.m_aFloats.SetSize(index + 1);
    m_params.m_aFloats.m_pArray[index] = value;
}

void CScriptEvent::SetObjectID(int index, unsigned long long objectId)
{
    if (index >= m_params.m_aObjects.m_count)
        m_params.m_aObjects.SetSize(index + 1);
    m_params.m_aObjects.m_pArray[index] = objectId;
}

void CScriptEvent::SetString(int index, const CExoString& sValue)
{
    if (index >= m_params.m_aStrings.m_count)
        m_params.m_aStrings.SetSize(index + 1);
    m_params.m_aStrings.m_pArray[index] = sValue;
}

CScriptEvent::~CScriptEvent()
{
    m_params.m_aIntegers.SetSize(0);
    m_params.m_aFloats.SetSize(0);
    m_params.m_aObjects.SetSize(0);
    m_params.m_aStrings.SetSize(0);
}

// Compares the integer, float, object and string parameter lists; the event type is ignored.
int CScriptEvent::operator==(const CScriptEvent& other) const
{
    if (m_params.m_aIntegers.m_count != other.m_params.m_aIntegers.m_count)
        return 0;
    for (int i = 0; i < m_params.m_aIntegers.m_count; i++)
        if (m_params.m_aIntegers.m_pArray[i] != other.m_params.m_aIntegers.m_pArray[i])
            return 0;

    if (m_params.m_aFloats.m_count != other.m_params.m_aFloats.m_count)
        return 0;
    for (int i = 0; i < m_params.m_aFloats.m_count; i++)
        if (m_params.m_aFloats.m_pArray[i] != other.m_params.m_aFloats.m_pArray[i])
            return 0;

    if (m_params.m_aObjects.m_count != other.m_params.m_aObjects.m_count)
        return 0;
    for (int i = 0; i < m_params.m_aObjects.m_count; i++)
        if (m_params.m_aObjects.m_pArray[i] != other.m_params.m_aObjects.m_pArray[i])
            return 0;

    if (m_params.m_aStrings.m_count != other.m_params.m_aStrings.m_count)
        return 0;
    for (int i = 0; i < m_params.m_aStrings.m_count; i++)
        if (m_params.m_aStrings.m_pArray[i] != other.m_params.m_aStrings.m_pArray[i])
            return 0;

    return 1;
}

int CScriptEvent::operator!=(const CScriptEvent& other) const
{
    return !(*this == other);
}

int CScriptEvent::SaveEvent(CResGFF* pGff, CResStruct* pStruct)
{
    pGff->WriteFieldWORD(pStruct, m_eventType, const_cast<char*>("EventType"));

    if (m_params.m_aIntegers.m_count >= 1) {
        CResList list;
        pGff->AddList(&list, pStruct, const_cast<char*>("IntList"));
        for (int64_t i = 0; i < m_params.m_aIntegers.m_count; i++) {
            CResStruct elem;
            pGff->AddListElement(&elem, &list, GFF_STRUCT_ID_EVENT_PARAMETER);
            pGff->WriteFieldINT(&elem, m_params.m_aIntegers.m_pArray[i], const_cast<char*>("Parameter"));
        }
    }

    if (m_params.m_aFloats.m_count >= 1) {
        CResList list;
        pGff->AddList(&list, pStruct, const_cast<char*>("FloatList"));
        for (int64_t i = 0; i < m_params.m_aFloats.m_count; i++) {
            CResStruct elem;
            pGff->AddListElement(&elem, &list, GFF_STRUCT_ID_EVENT_PARAMETER);
            pGff->WriteFieldFLOAT(&elem, m_params.m_aFloats.m_pArray[i], const_cast<char*>("Parameter"));
        }
    }

    if (m_params.m_aStrings.m_count >= 1) {
        CResList list;
        pGff->AddList(&list, pStruct, const_cast<char*>("StringList"));
        for (int64_t i = 0; i < m_params.m_aStrings.m_count; i++) {
            CResStruct elem;
            pGff->AddListElement(&elem, &list, GFF_STRUCT_ID_EVENT_PARAMETER);
            pGff->WriteFieldCExoString(&elem, m_params.m_aStrings.m_pArray[i], const_cast<char*>("Parameter"));
        }
    }

    if (m_params.m_aObjects.m_count >= 1) {
        CResList list;
        pGff->AddList(&list, pStruct, const_cast<char*>("ObjectList"));
        for (int64_t i = 0; i < m_params.m_aObjects.m_count; i++) {
            CResStruct elem;
            pGff->AddListElement(&elem, &list, GFF_STRUCT_ID_EVENT_PARAMETER);
            pGff->WriteFieldDWORD(&elem, static_cast<unsigned int>(m_params.m_aObjects.m_pArray[i]), const_cast<char*>("Parameter"));
        }
    }

    return 1;
}

void CScriptEvent::CopyScriptEvent(CScriptEvent* pSource)
{
    if (!pSource)
        return;

    m_eventType = pSource->m_eventType;

    m_params.m_aIntegers.SetSize(pSource->m_params.m_aIntegers.m_count);
    for (int i = 0; i < pSource->m_params.m_aIntegers.m_count; i++)
        m_params.m_aIntegers.m_pArray[i] = pSource->m_params.m_aIntegers.m_pArray[i];

    m_params.m_aFloats.SetSize(pSource->m_params.m_aFloats.m_count);
    for (int i = 0; i < pSource->m_params.m_aFloats.m_count; i++)
        m_params.m_aFloats.m_pArray[i] = pSource->m_params.m_aFloats.m_pArray[i];

    m_params.m_aObjects.SetSize(pSource->m_params.m_aObjects.m_count);
    for (int i = 0; i < pSource->m_params.m_aObjects.m_count; i++)
        m_params.m_aObjects.m_pArray[i] = pSource->m_params.m_aObjects.m_pArray[i];

    m_params.m_aStrings.SetSize(pSource->m_params.m_aStrings.m_count);
    for (int i = 0; i < pSource->m_params.m_aStrings.m_count; i++)
        m_params.m_aStrings.m_pArray[i] = pSource->m_params.m_aStrings.m_pArray[i];
}

int CScriptEvent::LoadEvent(CResGFF* pGff, CResStruct* pStruct)
{
    CResList list;
    CResStruct elem;
    int bSuccess;
    unsigned int i;
    m_eventType = pGff->ReadFieldWORD(pStruct, const_cast<char*>("EventType"), bSuccess, 0);
    if (pGff->GetList(&list, pStruct, const_cast<char*>("IntList"))) {
        for (i = 0; i < pGff->GetListCount(&list); i++) {
            pGff->GetListElement(&elem, &list, i);
            m_params.m_aIntegers.Add(pGff->ReadFieldINT(&elem, const_cast<char*>("Parameter"), bSuccess, 0));
        }
    }

    if (pGff->GetList(&list, pStruct, const_cast<char*>("FloatList"))) {
        for (i = 0; i < pGff->GetListCount(&list); i++) {
            pGff->GetListElement(&elem, &list, i);
            m_params.m_aFloats.Add(pGff->ReadFieldFLOAT(&elem, const_cast<char*>("Parameter"), bSuccess, 0.0f));
        }
    }

    if (pGff->GetList(&list, pStruct, const_cast<char*>("StringList"))) {
        CExoString sValue;
        for (i = 0; i < pGff->GetListCount(&list); i++) {
            pGff->GetListElement(&elem, &list, i);
            sValue = pGff->ReadFieldCExoString(&elem, const_cast<char*>("Parameter"), bSuccess, CExoString(""));
            m_params.m_aStrings.Add(sValue);
        }
    }

    if (pGff->GetList(&list, pStruct, const_cast<char*>("ObjectList"))) {
        for (i = 0; i < pGff->GetListCount(&list); i++) {
            pGff->GetListElement(&elem, &list, i);
            m_params.m_aObjects.Add(pGff->ReadFieldDWORD(&elem, const_cast<char*>("Parameter"), bSuccess, 0));
        }
    }

    return 1;
}
