#pragma once

// A script-visible variable table attached to game objects: local booleans packed as a bitfield
// plus local numbers, with GFF save and load. Every accessor bound-checks the index.
class CResGFF;
struct CResStruct;

class CSWVarTable {
public:
#if KOTOR1  // K1: 96 booleans and 8 numbers
    int           m_localBooleans[3];
    unsigned char m_localNumbers[8];
#else
    int           m_localBooleans[5];    // 160 bits, one per index 0..159
    unsigned char m_localNumbers[32];  // one per index 0..31
#endif

    CSWVarTable();
    int           GetLocalBoolean(int nIndex);
    void          SetLocalBoolean(int nIndex, int value);
    unsigned char GetLocalNumber(int nIndex);
    void          SetLocalNumber(int nIndex, unsigned char value);

    void SaveVarTable(CResGFF* pGFF, CResStruct* pStruct);
    void LoadVarTable(CResGFF* pGFF, CResStruct* pStruct);
};
