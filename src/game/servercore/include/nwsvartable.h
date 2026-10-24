#pragma once

#include "exostring.h"

struct CScriptLocation;

// idiom-exempt: canonical
// One named script variable. m_nType selects the active union arm.
class CSWSScriptVar {
public:
    CExoString m_sName;
    int m_nType;
    union {
        int m_nInt;
        float m_fFloat;
        CExoString* m_pString;
        unsigned long long m_oObject;
        CScriptLocation* m_pLocation;
    };
};

#include "CExoArrayListDtors.h"

class CResGFF;
struct CResStruct;
class CExoString;
struct CScriptLocation;
class CSWSObject;
class CSWSCreature;

// idiom-exempt: canonical
// The script-visible variable table; one lives by value inside each CSWSObject. m_pOwner is the
// creature whose journal the table backs. The VARTYPE_* tags are 1..5 in declaration order.
class CSWSScriptVarTable {
public:
    static const int VARTYPE_INTEGER;
    static const int VARTYPE_FLOAT;
    static const int VARTYPE_STRING;
    static const int VARTYPE_OBJECT;
    static const int VARTYPE_LOCATION;

    CExoArrayList<CSWSScriptVar> m_vars;
    CSWSCreature* m_pOwner;

    CSWSScriptVarTable() : m_pOwner(0) {}

    ~CSWSScriptVarTable();

    CSWSScriptVar* MatchIndex(const CExoString& sName, unsigned int nType, int bCreate);
    void DeleteIndex(const CExoString& sName, unsigned int nType);

    int                GetInt(const CExoString& sName);
    float              GetFloat(const CExoString& sName);
    CExoString         GetString(const CExoString& sName);
#if KOTOR1  // K1: GetObject keeps its name
    unsigned long long GetObject(const CExoString& sName);
    unsigned long long GetObjectA(const CExoString& sName) { return GetObject(sName); }
#else
    unsigned long long GetObjectA(const CExoString& sName);
#endif
    CScriptLocation    GetLocation(const CExoString& sName);

    void SetInt(const CExoString& sName, int nValue, int a);
    void SetFloat(const CExoString& sName, float fValue);
    void SetString(const CExoString& sName, CExoString& sValue);
    void SetObject(const CExoString& sName, unsigned long long oValue);
    void SetLocation(const CExoString& sName, CScriptLocation loc);

    void DestroyInt(const CExoString& sName);
    void DestroyFloat(const CExoString& sName);
    void DestroyString(const CExoString& sName);
    void DestroyObject(const CExoString& sName);
    void DestroyLocation(const CExoString& sName);

    void LoadVarTable(CResGFF* pGff, CResStruct* pStruct);
    void SaveVarTable(CResGFF* pGff, CResStruct* pStruct);
};
