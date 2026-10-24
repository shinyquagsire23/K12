#pragma once

// The game's global script-variable table, holding booleans, numbers, strings and locations keyed
// by name, loaded from globalcat.2da.

class CExoString;
class CResGFF;
class CResRef;
struct CResStruct;
#include "exostring.h"
#include "nwsobject.h"

// One identifier-table slot. The name is stored in place rather than by pointer, and the type tag
// shares its 16-bit word with the value index.
struct CSWGlobalVariableIdentifier {
    char           m_szName[22];
    unsigned short m_nIndex : 14;
    unsigned short m_nType  : 2;
};
K2_LAYOUT_ASSERT(sizeof(CSWGlobalVariableIdentifier) == 0x18, "entries are 0x18 apart in the table");

struct CSWGlobalVariableTable {
    enum {
        GVT_BOOLEAN  = 0,
        GVT_NUMBER   = 1,
        GVT_LOCATION = 2,
        GVT_STRING   = 3,
    };

    // The identifier table's slot count (the modulus Hash reduces to), and the sizes of the value
    // arrays: the packed boolean bytes, the numbers and the locations.
    enum {
#if KOTOR1
        NUM_IDENTIFIERS   = 1775,
        NUM_NUMBERS       = 500,
#else
        NUM_IDENTIFIERS   = 2365,
        NUM_NUMBERS       = 1000,
#endif
        NUM_BOOLEAN_BYTES = 113,
        NUM_LOCATIONS     = 100,
    };

    // The table has four value arrays plus the hash table, each with its own count.
    CSWGlobalVariableTable();
    ~CSWGlobalVariableTable();

    // The open-addressed identifier table has 2365 slots, the exact modulus Hash reduces to. An
    // empty slot is one whose name is empty.
    CSWGlobalVariableIdentifier m_aIdentifiers[NUM_IDENTIFIERS];
    // The packed boolean bits, most significant bit first inside each byte. The tail of the last byte is
    // padding.
    unsigned char m_aBooleanBits[NUM_BOOLEAN_BYTES];
    char          m_aNumbers[NUM_NUMBERS];
    // The locations array, read and written whole-struct by the value accessors.
    CScriptLocation m_aLocations[NUM_LOCATIONS];
    CExoString      m_aStrings[5];
    // The four live-entry counts, each bumped by its AddIdentifier wrapper and summed by
    // GetPerformanceInfo, which ignores the string count.
    int m_nBooleanCount;
    int m_nNumberCount;
    int m_nLocationCount;
    int m_nStringCount;
    // The CRC-32 table the hash walks, built by the constructor with the standard eight-round
    // shift-xor loop. ClearValues leaves it alone.
    unsigned int m_aHashTable[256];
    // Hash statistics, maintained by AddIdentifier and read back by GetPerformanceInfo: collisions,
    // total probe steps, and the longest probe seen.
    int m_nCollisions;
    int m_nProbes;
    int m_nMaxProbes;

    // Names are stored as given but hashed and compared upper-cased.
    unsigned int Hash(const CExoString& sName);
    int  FindIdentifierIndex(const CExoString& sName);
    // Adds an identifier and returns its slot, or -1 when the type's array is full.
    int  AddIdentifier(const CExoString& sName, unsigned int nType, unsigned int nIndex);
    // Each of the four typed wrappers passes its own count as the slot and bumps it on success.
    int  AddIdentifierBoolean(const CExoString& sName);
    int  AddIdentifierNumber(const CExoString& sName);
    int  AddIdentifierLocation(const CExoString& sName);
    int  AddIdentifierString(const CExoString& sName);
    void ClearIdentifiers();
    void ClearValues();

    // Both take the bit array from the argument list and ignore `this`; they are non-static so the array
    // lands in the second argument slot.
    int  GetBit(unsigned char* pBits, int nBit);
    void SetBit(unsigned char* pBits, int nBit, int nValue);

    void GetValueBoolean(const CExoString& sName, int& nValue);
    // The out-parameter is a plain char reference.
    void GetValueNumber(const CExoString& sName, char& nValue);
    void GetValueLocation(const CExoString& sName, CScriptLocation& out);
    void GetValueString(const CExoString& sName, CExoString& out);
    void SetValueBoolean(const CExoString& sName, int nValue);
    void SetValueNumber(const CExoString& sName, char nValue);
    void SetValueLocation(const CExoString& sName, const CScriptLocation& value);
    void SetValueString(const CExoString& sName, const CExoString& value);
    void GetPerformanceInfo(int& nIdentifiers, int& nCollisions, int& nAvgProbes,
                            int& nMaxProbes);

    // Takes the resref by const reference.
    void ReadCatalogue(const CResRef& resRef);
    int  ReadTable(const CExoString& sFileName);
    void ReadTableWithCatalogue(CResGFF* pGFF, CResStruct& sTop);
    void ReadTableWithoutCatalogue(CResGFF* pGFF, CResStruct& sTop);
    void WriteTable(const CExoString& sFileName, int bBinary);
    void RegressionTest();
    int  Load();
    void Save(const CExoString& sPath);
};

// CServerExoAppInternal embeds this class by value, so its size is fixed.
#if KOTOR1
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGlobalVariableTable, m_nBooleanCount) == 0xb280, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGlobalVariableTable, m_nCollisions) == 0xb690, "K1 layout");
K2_LAYOUT_ASSERT(sizeof(CSWGlobalVariableTable) == 0xb6a0, "K1 CSWGlobalVariableTable size");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGlobalVariableTable) == 0xefe8,
              "CServerExoAppInternal embeds this class by value at +0x10178 and its following "
              "members start at +0x1f160; sizeof must stay 0xefe8");
#endif
