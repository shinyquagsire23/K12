#pragma once

// idiom-exempt: canonical
// One quest entry, with the localized title and body, the plot it belongs to, its progress state
// and its timestamp. The party table owns the live list.
#include "exostring.h"
#include "exolocstring.h"

struct SJournalEntry {
#if KOTOR1  // K1 inlines the constructor at every use
    SJournalEntry() : m_nDate(-1), m_nTime(-1)
    {
        m_nPicture = 0xffff;
        m_nState = -1;
        m_nPriority = 0xffffffff;
        m_nPlotIndex = 0;
        m_nPlanetId = -1;
        m_bEnd = 0;
        m_bReserved1 = 0;
        m_bNewUnobserved = 0;
        m_nQuestEntryXP = 0;
    }
#else
    SJournalEntry();
#endif
    SJournalEntry(const SJournalEntry& other) = default;
    SJournalEntry& operator=(const SJournalEntry& other);

    // Copies each display field whose bit is set in nFlags from the other entry, then marks this one as
    // not yet looked at.
    void Update(SJournalEntry other, unsigned short nFlags);

    CExoLocString m_locName;
    CExoLocString m_locText;
    int m_nDate;
    int m_nTime;
    CExoString m_sPlotId;
    int m_nState;
    unsigned int m_nPriority;
    unsigned short m_nPicture;
    unsigned char m_bEnd : 1;
    unsigned char m_bReserved1 : 1;
    unsigned char m_bNewUnobserved : 1;
    int m_nPlotIndex;
    int m_nPlanetId;
    int m_nQuestEntryXP;
};

// idiom-exempt: canonical
// The server-side quest journal is a flat entry list, with every method a linear scan keyed by the
// entry's plot id. The client keeps its own list plus the sorting and pending-update machinery.
#include "CExoArrayListDtors.h"
#include "exostring.h"

class CSWSJournal {
public:
    CSWSJournal();
    ~CSWSJournal();

    // Both scan for the entry with this plot id; they differ only in which field they return.
    int GetState(const CExoString& sPlotId);
    int GetQuestEntryXP(const CExoString& sPlotId);

    void SetPicture(const CExoString& sPlotId, int nPictureId, int bFlag);  // does nothing

    void SetState(const CExoString& sPlotId, unsigned int nState, int bFlag);
    void SetDate(const CExoString& sPlotId, unsigned int nDate, int bFlag);
    void SetTime(const CExoString& sPlotId, unsigned int nTime, int bFlag);
    void Destroy(const CExoString& sPlotId);

    // Merges the other journal's entries into this one, skipping plot ids already present, and returns
    // how many were appended.
    int CopyEntries(CSWSJournal* pOther);

    CExoArrayList<SJournalEntry> m_entries;
};

// idiom-exempt: canonical
// A pending "this quest changed" record for one player, queued until the client next asks for the
// journal. The flag word is the set of changes to replay.
#include "exostring.h"

struct CSWSPlayerJournalQuestUpdates {
    unsigned short m_updateFlags;
    CExoString     m_sQuestTag;
};

// idiom-exempt: canonical
// Per-player journal sync state, hanging off the player object. Answers two questions: does this
// player still need the whole journal, and if not, which quests have changed since they last saw it.
#include "CExoArrayListDtors.h"

class CSWSPlayerJournalState {
public:
    CSWSPlayerJournalState()
    {
        m_bQuestScreenOpen = 0;
        m_bFullUpdateNeeded = 1;
    }

    int m_bQuestScreenOpen;

    CExoArrayList<CSWSPlayerJournalQuestUpdates> m_updates;

    int m_bFullUpdateNeeded;
};
