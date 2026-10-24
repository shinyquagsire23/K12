
// Script variable table accessors for the boolean bitfield and the number bytes.

#include "swvartable.h"
#include "resgff.h"

#if KOTOR1
#define SWVAR_LAST_BOOLEAN 95
#define SWVAR_BOOLEAN_WORDS 3
#define SWVAR_LAST_NUMBER 7
#define SWVAR_NUMBERS 8
#else
#define SWVAR_LAST_BOOLEAN 159
#define SWVAR_BOOLEAN_WORDS 5
#define SWVAR_LAST_NUMBER 31
#define SWVAR_NUMBERS 32
#endif

CSWVarTable::CSWVarTable()
{
    __builtin_memset(this, 0, sizeof(*this));
}

int CSWVarTable::GetLocalBoolean(int nIndex)
{
    if (nIndex < 0 || nIndex > SWVAR_LAST_BOOLEAN)
        return 0;
    int nWord = (nIndex >> 5) & 7;
    K2_ASSUME(nWord < static_cast<int>(sizeof(m_localBooleans) / sizeof(m_localBooleans[0])));
    return m_localBooleans[nWord] & static_cast<int>(1u << (nIndex & 31));
}

void CSWVarTable::SetLocalBoolean(int nIndex, int value)
{
    if (nIndex < 0 || nIndex > SWVAR_LAST_BOOLEAN)
        return;
    int nBit = static_cast<int>(1u << (nIndex & 31));
    int nWord = (nIndex >> 5) & 7;
    K2_ASSUME(nWord < static_cast<int>(sizeof(m_localBooleans) / sizeof(m_localBooleans[0])));
    if (value)
        m_localBooleans[nWord] |= nBit;
    else
        m_localBooleans[nWord] &= ~nBit;
}

unsigned char CSWVarTable::GetLocalNumber(int nIndex)
{
    if (nIndex < 0 || nIndex > SWVAR_LAST_NUMBER)
        return 0;
    return m_localNumbers[nIndex];
}

void CSWVarTable::SetLocalNumber(int nIndex, unsigned char value)
{
    if (nIndex < 0 || nIndex > SWVAR_LAST_NUMBER)
        return;
    m_localNumbers[nIndex] = value;
}

void CSWVarTable::SaveVarTable(CResGFF* pGFF, CResStruct* pStruct)
{
    CResStruct sStruct;
    CResList   list;
    CResStruct sElement;

    pGFF->AddStructToStruct(&sStruct, pStruct, const_cast<char*>("SWVarTable"), 0);

    pGFF->AddList(&list, &sStruct, const_cast<char*>("BitArray"));
    for (int i = 0; i < SWVAR_BOOLEAN_WORDS; i++)
    {
        pGFF->AddListElement(&sElement, &list, 0);
        pGFF->WriteFieldDWORD(&sElement, static_cast<unsigned int>(m_localBooleans[i]), const_cast<char*>("Variable"));
    }

    pGFF->AddList(&list, &sStruct, const_cast<char*>("ByteArray"));
    for (int i = 0; i < SWVAR_NUMBERS; i++)
    {
        pGFF->AddListElement(&sElement, &list, 0);
        pGFF->WriteFieldBYTE(&sElement, m_localNumbers[i], const_cast<char*>("Variable"));
    }
}

void CSWVarTable::LoadVarTable(CResGFF* pGFF, CResStruct* pStruct)
{
    CResStruct sStruct;
    CResList   list;
    CResStruct sElement;
    int        bSuccess;

    if (!pGFF->GetStructFromStruct(&sStruct, pStruct, const_cast<char*>("SWVarTable")))
        return;

    if (pGFF->GetList(&list, &sStruct, const_cast<char*>("BitArray")))
    {
#if KOTOR1  // K1 re-reads the count once
        unsigned int nCount = pGFF->GetListCount(&list);
        nCount = pGFF->GetListCount(&list);
        if (nCount > SWVAR_BOOLEAN_WORDS - 1)
            nCount = SWVAR_BOOLEAN_WORDS;

        if (static_cast<int>(nCount) >= 1)
        {
            for (unsigned int i = 0; i < nCount; i++)
            {
                if (pGFF->GetListElement(&sElement, &list, i))
                    m_localBooleans[i] = pGFF->ReadFieldDWORD(&sElement, const_cast<char*>("Variable"), bSuccess, 0);
            }
        }
#else
        int nCount = pGFF->GetListCount(&list);
        if (pGFF->GetListCount(&list) <= SWVAR_BOOLEAN_WORDS - 1)
            nCount = pGFF->GetListCount(&list);
        else
            nCount = SWVAR_BOOLEAN_WORDS;

        if (nCount >= 1)
        {
            for (int i = 0; i < nCount; i++)
            {
                if (pGFF->GetListElement(&sElement, &list, i))
                    m_localBooleans[i] = pGFF->ReadFieldDWORD(&sElement, const_cast<char*>("Variable"), bSuccess, 0);
            }
        }
#endif
    }

    if (pGFF->GetList(&list, &sStruct, const_cast<char*>("ByteArray")))
    {
#if KOTOR1  // K1 re-reads the count once
        unsigned int nCount = pGFF->GetListCount(&list);
        nCount = pGFF->GetListCount(&list);
        if (nCount > SWVAR_NUMBERS - 1)
            nCount = SWVAR_NUMBERS;

        if (static_cast<int>(nCount) >= 1)
        {
            for (unsigned int i = 0; i < nCount; i++)
            {
                if (pGFF->GetListElement(&sElement, &list, i))
                    m_localNumbers[i] = pGFF->ReadFieldBYTE(&sElement, const_cast<char*>("Variable"), bSuccess, 0);
            }
        }
#else
        int nCount = pGFF->GetListCount(&list);
        if (pGFF->GetListCount(&list) <= SWVAR_NUMBERS - 1)
            nCount = pGFF->GetListCount(&list);
        else
            nCount = SWVAR_NUMBERS;

        if (nCount >= 1)
        {
            for (int i = 0; i < nCount; i++)
            {
                if (pGFF->GetListElement(&sElement, &list, i))
                    m_localNumbers[i] = pGFF->ReadFieldBYTE(&sElement, const_cast<char*>("Variable"), bSuccess, 0);
            }
        }
#endif
    }
}
