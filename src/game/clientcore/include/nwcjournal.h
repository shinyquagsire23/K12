#pragma once
#include "k2stdint.h"

#ifndef CSWCJOURNAL_H
#define CSWCJOURNAL_H

#include "CExoArrayListDtors.h"
#include "nwsjournal.h"
#include "nwjournal.h"

// The key a journal page is sorted by (CSWCSortedJournalArrayList::m_nSortBy, the GUI's
// g_nJournalSortType and the save's JNL_SortOrder); the world journal knows only the first two.
enum JournalSortBy {
    JOURNAL_SORT_BY_DATE     = 0,
    JOURNAL_SORT_BY_NAME     = 1,
    JOURNAL_SORT_BY_PRIORITY = 2,
    JOURNAL_SORT_BY_PLANET   = 3,
};

// One journal page, holding an entry array, a parallel sort-order index list, the current sort key
// and a contents-changed flag. CSWCJournal embeds two of these, one per page.
class CSWCSortedJournalArrayList {
public:
    CSWCSortedJournalArrayList();
    // Maps a display index through the sort order.
    SJournalEntry* GetEntryAt(int i);
    void SortList();
    void UpdateEntry(int i, const SJournalEntry& e);
    // Comparators; SortList dispatches on m_nSortBy.
    int SortItemsByDate(int a, int b);
    int SortItemsByName(int a, int b);
    int SortItemsByPriority(int a, int b);
    int SortItemsByPlanet(int a, int b);

    CExoArrayList<SJournalEntry> m_entries;
    CExoArrayList<int>           m_sortOrder;
    int m_nSortBy;
    int m_bHasChanged;
};

extern int g_bOverrideJournalSortType;
extern int g_nJournalSortType;

// The client-side journal is the GUI-facing display model, with two pages (completed and active
// quests), each a sortable view over its own entry array, plus a queue of updates deferred while
// the GUI was busy.
class CSWCJournal {
public:
    CSWCJournal();

    static const unsigned int STRREF_CSWCJOURNAL_QUESTENTRYADDED;
    static const unsigned int STRREF_CSWCJOURNAL_WORLDENTRYADDED;

    int  GetNumDoneEntries();
    int  GetNumActiveEntries();
    int  GetDoneSortBy();
    int  GetActiveSortBy();
    void ClearDoneHasChanged();
    void ClearActiveHasChanged();
    void EnableSort();
    void DisableSort();
    // Tells the server the quest screen opened, and that it closed.
    void SetJournalPageOpen();
    void SetJournalPageClosed();
    void JournalUpdated(int bAllowDefer, int nUpdateType, const CExoString& sText);
    // Drains the deferred-update queue once the GUI is idle.
    void CheckForUpdates();
    void SetJournalPicture(const CExoString& sPlotId, int nPicture);
    // The unsigned short is a flag word; a server-driven add sets its high bit.
    void Add(SJournalEntry entry, unsigned short nFlags);
    void Remove(const CExoString& sPlotId);
    // Sets the sort key and resorts the page.
    void SetActiveSortBy(int n);
    void SetDoneSortBy(int n);
    SJournalEntry* GetDoneEntryAt(int i);
    SJournalEntry* GetActiveEntryAt(int i);
    int  HasNewEntries();
    void ClearAll();
    // End-of-bulk-rebuild hook.
    void CallFullUpdateComplete();

    CSWCSortedJournalArrayList m_done;
    CSWCSortedJournalArrayList m_active;
    unsigned char m_bSortEnabled : 1;
    CExoArrayList<CSWCJournalPendingUpdate> m_pendingUpdates;
};

#endif

#include <stddef.h>

#include "exostring.h"

// The client-side world journal holds a list of CWorldJournalEntry records plus a parallel
// selection order that maps a display index to a record, the current sort key and a
// contents-changed flag.
class CSWCWorldJournal {
public:
    int GetNumEntries();
    CWorldJournalEntry* GetEntryAt(int nIndex);
    CSWCWorldJournal();
    ~CSWCWorldJournal();

    void AddEntryStrRef(uint64_t nNameStrRef, uint64_t nTextStrRef,
                        unsigned int nDate, unsigned int nTime);
    void AddEntry(unsigned int nId, const CExoString& sName, const CExoString& sText,
                  unsigned int nDate, unsigned int nTime);

    void DeleteEntry(unsigned int nEntry);
    void DeleteEntryStrRef(uint64_t nStrRef);
    void DeleteAllEntries();

    void SetSortBy(int nSortBy);
    void SortList();
    // Comparators, dispatched as member-function pointers by SortList. The third parameter is the
    // "don't fall back to the other comparator" flag: SortList passes 0, and each comparator
    // tail-calls the other with 1 once its own key ties.
    int SortItemsByDate(int a, int b, int c);
    int SortItemsByName(int a, int b, int c);

    CExoArrayList<CWorldJournalEntry> m_entries;
    CExoArrayList<int>                m_sortOrder;
    unsigned char m_nSortBy;
    unsigned char m_bModified : 1;
    unsigned char m_nReserved1 : 7;
    char _pad22[0x28 - 0x22];
};
