#pragma once

// One player's 16-slot ring of game-object ids, with a read and a write cursor.
class CObjectLookupTable {
public:
    // The ring's slot count.
    enum {
        RING_SIZE = 16,
    };

    CObjectLookupTable();
    ~CObjectLookupTable();

    // Appends an id at the write cursor and returns the slot it landed in.
    int AddID(unsigned long long nID);
    unsigned long long GetIDFromIndex(unsigned int nIndex);
    // -1 when the id is not in the ring.
    int  GetIndexFromID(unsigned long long nID);
    void Touch(unsigned int nIndex);
    void* GetUpdate(void* pUnused, unsigned int* pnSize);
    void Update(void* pBuffer, unsigned int nSize);

    unsigned long long* m_paIDs;
    unsigned int m_nCursorA;
    unsigned int m_nCursorB;
    int m_nReserved1;
};

// The block layout the save/load pair trades is the read cursor, then the ids from the next byte
// on, which is not 8-byte aligned and so cannot be spelled as an array here.
struct CObjectLookupBlockHeader {
    unsigned int m_nReadCursor;
};

K2_LAYOUT_ASSERT(sizeof(CObjectLookupTable) == 0x18, "CObjectLookupTable size drift");

// One aggregate so the constructor can value-initialise the whole array as a unit.
struct CObjectTableSlots {
    // The player slots, one CObjectLookupTable each.
    enum {
        MAX_PLAYERS = 96,
    };

    CObjectLookupTable* m_pSlots[MAX_PLAYERS];
};

// The server's game-object-id table, with one CObjectLookupTable per player, indexed by object id.
class CObjectTableManager {
public:
    // AddID's *pIndex when the id was not added.
    enum : unsigned int {
        INDEX_NONE = 0xffffffffu,
    };

    CObjectTableManager();
    ~CObjectTableManager();

    void ClearAll();
    bool RemovePlayer(unsigned int playerId);
    int  CreateNewPlayer(unsigned int playerId);

    bool Touch(unsigned int nIndex, unsigned int playerId);
    bool AddID(unsigned long long nID, unsigned int* pIndex, unsigned int playerId);
    unsigned long long GetIDFromIndex(unsigned int nIndex, unsigned int playerId);
    int  GetIndexFromID(unsigned long long nID, unsigned int playerId);

private:
    CObjectTableSlots m_slots;
    unsigned int      m_nCount;
};
