#include "k2stdint.h"
#include "strref.h"
#include "nwcjournal.h"
#include "CExoArrayListImpl.h"
#include "clientexoapp.h"
#include "appmanager.h"
#include "nwchandlemessage.h"
#include "worldtimer.h"
#include "nwmessage.h"
#include "guiingame.h"
#include "exolocstringinternal.h"
#include "nwmessagetypes.h"
#include "feedbackmessage.h"

const unsigned int CSWCJournal::STRREF_CSWCJOURNAL_QUESTENTRYADDED = STRREF_JOURNAL_UPDATED;

// The two pages are cleared as one block; the pending list clears itself.
CSWCJournal::CSWCJournal()
{
    __builtin_memset(static_cast<void*>(&m_done), 0, sizeof(m_done) + sizeof(m_active));
    m_bSortEnabled = 1;
}

int CSWCJournal::GetNumDoneEntries()
{
    return m_done.m_entries.m_count;
}

int CSWCJournal::GetNumActiveEntries()
{
    return m_active.m_entries.m_count;
}

int CSWCJournal::GetDoneSortBy()
{
    return m_done.m_nSortBy;
}

int CSWCJournal::GetActiveSortBy()
{
    return m_active.m_nSortBy;
}

void CSWCJournal::ClearDoneHasChanged()
{
    m_done.m_bHasChanged = 0;
}

void CSWCJournal::ClearActiveHasChanged()
{
    m_active.m_bHasChanged = 0;
}

void CSWCJournal::EnableSort()
{
    m_bSortEnabled = 1;
}

void CSWCJournal::DisableSort()
{
    m_bSortEnabled = 0;
}

// Tells the server the quest screen just opened. The send is skipped when no game session is
// active, i.e. when the message object is null.
void CSWCJournal::SetJournalPageOpen()
{
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerJournalMessage_QuestScreenOpen();
}

void CSWCJournal::SetJournalPageClosed()
{
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerJournalMessage_QuestScreenClosed();
}

// The single entry point for "the journal changed, tell the player". While a load bar is up and the
// caller allows it the update is queued for CheckForUpdates to replay; otherwise the feedback
// message goes out at once, the status line is poked and the sort-order override is raised.
// nUpdateType selects between two adjacent status ids.
void CSWCJournal::JournalUpdated(int bAllowDefer, int nUpdateType, const CExoString& sText)
{
    int bBusy = g_pAppManager->m_pClientExoApp->GetLoadBarState();
    if (bAllowDefer && bBusy) {
        CSWCJournalPendingUpdate update;
        update.m_sText = sText;
        update.m_nUpdateType = nUpdateType;
        m_pendingUpdates.Add(update);
        return;
    }

    CSWCCMessageData* pData = new CSWCCMessageData();
    pData->SetString(0, sText);
    g_pAppManager->m_pClientExoApp->AddFeedbackMessage(FEEDBACK_QUEST_ENTRY_ADDED, pData, CExoString(""));
    g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_JOURNAL, 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(
        nUpdateType ? CGuiInGame::GUI_STATUS_COMPLETE_SOUND : CGuiInGame::GUI_STATUS_NEW_QUEST_SOUND, 0);
    g_bOverrideJournalSortType = 1;
}

// Drains the deferred-update queue. Nothing happens while the GUI is busy; once idle every queued
// update is replayed through JournalUpdated and the list is emptied.
void CSWCJournal::CheckForUpdates()
{
    if (g_pAppManager->m_pClientExoApp->GetLoadBarState())
        return;

    for (int64_t i = 0; i < m_pendingUpdates.m_count; i++) {
        JournalUpdated(1, m_pendingUpdates.m_pArray[i].m_nUpdateType,
                       m_pendingUpdates.m_pArray[i].m_sText);
    }

    m_pendingUpdates.Clear();
}

// Finds the first entry whose plot id matches, searching the done page then the active page, and
// stamps its picture, marks it unread and flags that page as changed.
void CSWCJournal::SetJournalPicture(const CExoString& sPlotId, int nPicture)
{
    int nCount = m_done.m_entries.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_done.m_entries.m_pArray[i].m_sPlotId == sPlotId) {
            SJournalEntry& e = m_done.m_entries.m_pArray[i];
            e.m_nPicture = static_cast<unsigned short>(nPicture);
            e.m_bNewUnobserved = 1;
            m_done.m_bHasChanged = 1;
            return;
        }
    }
    nCount = m_active.m_entries.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_active.m_entries.m_pArray[i].m_sPlotId == sPlotId) {
            SJournalEntry& e = m_active.m_entries.m_pArray[i];
            e.m_nPicture = static_cast<unsigned short>(nPicture);
            e.m_bNewUnobserved = 1;
            m_active.m_bHasChanged = 1;
            return;
        }
    }
}

// Applies one inbound journal entry. The two pages are alternatives picked by the entry's own
// m_bEnd bit: a quest-completing entry belongs on the done page, any other on the active page. The
// body is duplicated per page throughout.
//
// An entry already on its home page is snapshotted, removed, merged through SJournalEntry::Update
// and then filed again; an entry on the other page is updated in place and that page re-sorted;
// anything else is appended and re-sorted. When no entry existed and no full-add flag was set, this
// was a partial update of a missing entry and the diagnostic string is built.
void CSWCJournal::Add(SJournalEntry entry, unsigned short nFlags)
{
    int bEnd = entry.m_bEnd;
    int nDone = m_done.m_entries.m_count;
    int nActive = m_active.m_entries.m_count;
    SJournalEntry merged;
    CExoString sError;

    int bFound = 0;
    if (bEnd) {
        for (int i = 0; i < nActive; i++) {
            if (m_active.m_entries.m_pArray[i].m_sPlotId == entry.m_sPlotId) {
                merged = m_active.m_entries.m_pArray[i];
                Remove(entry.m_sPlotId);
                bFound = 1;
                break;
            }
        }
    } else {
        for (int i = 0; i < nDone; i++) {
            if (m_done.m_entries.m_pArray[i].m_sPlotId == entry.m_sPlotId) {
                merged = m_done.m_entries.m_pArray[i];
                Remove(entry.m_sPlotId);
                bFound = 1;
                break;
            }
        }
    }
    if (bFound) {
        merged.Update(entry, nFlags);
        entry = merged;
    }

    if (bEnd) {
        for (int i = 0; i < nDone; i++) {
            if (m_done.m_entries.m_pArray[i].m_sPlotId == entry.m_sPlotId) {
                m_done.m_entries.m_pArray[i].Update(entry, nFlags);
                if (m_bSortEnabled)
                    m_done.SortList();
                return;
            }
        }
    } else {
        for (int i = 0; i < nActive; i++) {
            if (m_active.m_entries.m_pArray[i].m_sPlotId == entry.m_sPlotId) {
                m_active.m_entries.m_pArray[i].Update(entry, nFlags);
                if (m_bSortEnabled)
                    m_active.SortList();
                return;
            }
        }
    }

    if (!bFound && (nFlags & MSG_JOURNAL_QUEST_ALL) == 0) {
        sError.Format(const_cast<char*>("Journal Crash! Partial Update on non-existant entry; plot:%s flags:%i"),
                      entry.m_sPlotId.CStr(), nFlags);
    }

    if (bEnd) {
        int nIndex = m_done.m_entries.m_count;
        m_done.m_entries.Add(entry);
        m_done.m_sortOrder.Add(nIndex);
        if (m_bSortEnabled)
            m_done.SortList();
    } else {
        int nIndex = m_active.m_entries.m_count;
        m_active.m_entries.Add(entry);
        m_active.m_sortOrder.Add(nIndex);
        if (m_bSortEnabled)
            m_active.SortList();
    }
}

// Removes the first entry with the given plot id, searching the active page then the done page. Per
// page: shift the entries array down over the hole, walk the parallel sort-order list dropping the
// slot that named the removed index and decrementing every slot above it, then flag the page
// changed.
void CSWCJournal::Remove(const CExoString& sPlotId)
{
    int n = m_active.m_entries.m_count;
    for (int i = 0; i < n; i++) {
        if (m_active.m_entries.m_pArray[i].m_sPlotId == sPlotId) {
            m_active.m_entries.DelIndex(i);
            int* pSort = m_active.m_sortOrder.m_pArray;
            for (int64_t k = 0; k < n; k++) {
                if (pSort[k] == i) {
                    m_active.m_sortOrder.DelIndex(static_cast<int>(k));
                    n = m_active.m_sortOrder.m_count;
                }
                if (k != n && pSort[k] > i)
                    pSort[k]--;
            }
            m_active.m_bHasChanged = 1;
            return;
        }
    }
    n = m_done.m_entries.m_count;
    for (int i = 0; i < n; i++) {
        if (m_done.m_entries.m_pArray[i].m_sPlotId == sPlotId) {
            m_done.m_entries.DelIndex(i);
            int* pSort = m_done.m_sortOrder.m_pArray;
            for (int64_t k = 0; k < n; k++) {
                if (pSort[k] == i) {
                    m_done.m_sortOrder.DelIndex(static_cast<int>(k));
                    n = m_done.m_sortOrder.m_count;
                }
                if (k != n && pSort[k] > i)
                    pSort[k]--;
            }
            m_done.m_bHasChanged = 1;
            return;
        }
    }
}

// One journal page, holding the entries plus a sort-order index list.
CSWCSortedJournalArrayList::CSWCSortedJournalArrayList()
{
    __builtin_memset(static_cast<void*>(this), 0, sizeof(*this));
}

SJournalEntry* CSWCSortedJournalArrayList::GetEntryAt(int i)
{
    if (i < 0)
        return 0;
    if (m_entries.m_count <= i)
        return 0;
    return &m_entries.m_pArray[m_sortOrder.m_pArray[i]];
}

// Selection-sorts the sort-order index list with the comparator m_nSortBy picks.
void CSWCSortedJournalArrayList::SortList()
{
    int (CSWCSortedJournalArrayList::*pfnCompare)(int, int);
    if (m_nSortBy == JOURNAL_SORT_BY_DATE)
        pfnCompare = &CSWCSortedJournalArrayList::SortItemsByDate;
    else if (m_nSortBy == JOURNAL_SORT_BY_PRIORITY)
        pfnCompare = &CSWCSortedJournalArrayList::SortItemsByPriority;
    else if (m_nSortBy == JOURNAL_SORT_BY_NAME)
        pfnCompare = &CSWCSortedJournalArrayList::SortItemsByName;
    else
        pfnCompare = &CSWCSortedJournalArrayList::SortItemsByPlanet;

    int count = m_entries.m_count;
    for (int i = 0; i < count; i++) {
        int nMin = i;
        for (int j = i + 1; j < count; j++) {
            if ((this->*pfnCompare)(m_sortOrder.m_pArray[nMin], m_sortOrder.m_pArray[j]) < 0)
                nMin = j;
        }
        int nTmp = m_sortOrder.m_pArray[i];
        m_sortOrder.m_pArray[i] = m_sortOrder.m_pArray[nMin];
        m_sortOrder.m_pArray[nMin] = nTmp;
    }
    m_bHasChanged = 1;
}

// Overwrites entry i in place, copying only the display and state fields. m_sPlotId is left alone, so
// the entry keeps its identity.
void CSWCSortedJournalArrayList::UpdateEntry(int i, const SJournalEntry& e)
{
    m_entries.m_pArray[i].m_locName = e.m_locName;
    m_entries.m_pArray[i].m_locText = e.m_locText;
    m_entries.m_pArray[i].m_nDate = e.m_nDate;
    m_entries.m_pArray[i].m_nTime = e.m_nTime;
    m_entries.m_pArray[i].m_nState = e.m_nState;
    m_entries.m_pArray[i].m_nPriority = e.m_nPriority;
    m_entries.m_pArray[i].m_nPicture = e.m_nPicture;
    m_entries.m_pArray[i].m_bEnd = e.m_bEnd;
    m_entries.m_pArray[i].m_nPlotIndex = e.m_nPlotIndex;
    m_entries.m_pArray[i].m_nPlanetId = e.m_nPlanetId;
    m_entries.m_pArray[i].m_nQuestEntryXP = e.m_nQuestEntryXP;
}

// Orders two entries by their date and time of day, through the world timer.
int CSWCSortedJournalArrayList::SortItemsByDate(int a, int b)
{
    CWorldTimer* pTimer = g_pAppManager->m_pClientExoApp->GetWorldTimer();
    SJournalEntry& ea = m_entries.m_pArray[a];
    SJournalEntry& eb = m_entries.m_pArray[b];
    return pTimer->CompareWorldTimes(ea.m_nDate, ea.m_nTime, eb.m_nDate, eb.m_nTime);
}

// Orders two entries by their localized title, descending.
int CSWCSortedJournalArrayList::SortItemsByName(int a, int b)
{
    CExoString sA;
    CExoString sB;
    int nResult;
    int nLanguage = g_pAppManager->m_pClientExoApp->GetClientLanguage();
    m_entries.m_pArray[a].m_locName.GetString(nLanguage, &sA, 0);
    m_entries.m_pArray[b].m_locName.GetString(nLanguage, &sB, 0);
    if (sA < sB)
        nResult = 1;
    else
        nResult = -(sA > sB != 0);
    return nResult;
}

// Descending by priority, ties broken by timestamp.
int CSWCSortedJournalArrayList::SortItemsByPriority(int a, int b)
{
    unsigned int pa = m_entries.m_pArray[a].m_nPriority;
    unsigned int pb = m_entries.m_pArray[b].m_nPriority;
    if (pa > pb)
        return -1;
    if (pa >= pb) {
        CWorldTimer* pTimer = g_pAppManager->m_pClientExoApp->GetWorldTimer();
        SJournalEntry& ea = m_entries.m_pArray[a];
        SJournalEntry& eb = m_entries.m_pArray[b];
        return pTimer->CompareWorldTimes(ea.m_nDate, ea.m_nTime, eb.m_nDate, eb.m_nTime);
    }
    return 1;
}

// Ascending by planet id, ties broken by timestamp.
int CSWCSortedJournalArrayList::SortItemsByPlanet(int a, int b)
{
    int pa = m_entries.m_pArray[a].m_nPlanetId;
    int pb = m_entries.m_pArray[b].m_nPlanetId;
    if (pa > pb)
        return 1;
    if (pa >= pb) {
        CWorldTimer* pTimer = g_pAppManager->m_pClientExoApp->GetWorldTimer();
        SJournalEntry& ea = m_entries.m_pArray[a];
        SJournalEntry& eb = m_entries.m_pArray[b];
        return pTimer->CompareWorldTimes(ea.m_nDate, ea.m_nTime, eb.m_nDate, eb.m_nTime);
    }
    return -1;
}

void CSWCJournal::SetActiveSortBy(int n)
{
    if (static_cast<unsigned int>(n) > 3)
        return;
    m_active.m_nSortBy = n;
    if (m_bSortEnabled)
        m_active.SortList();
}

void CSWCJournal::SetDoneSortBy(int n)
{
    if (static_cast<unsigned int>(n) > 3)
        return;
    m_done.m_nSortBy = n;
    if (m_bSortEnabled)
        m_done.SortList();
}

// Re-sorts both pages, and through SortList marks both as changed. Unlike the setters above this
// does not consult m_bSortEnabled.
void CSWCJournal::CallFullUpdateComplete()
{
    m_active.SortList();
    m_done.SortList();
}

// Display index to record through the sort-order list, or null when the index is out of range.
SJournalEntry* CSWCJournal::GetDoneEntryAt(int i)
{
    if (i < 0)
        return 0;
    if (m_done.m_entries.m_count <= i)
        return 0;
    return &m_done.m_entries.m_pArray[m_done.m_sortOrder.m_pArray[i]];
}

SJournalEntry* CSWCJournal::GetActiveEntryAt(int i)
{
    if (i < 0)
        return 0;
    if (m_active.m_entries.m_count <= i)
        return 0;
    return &m_active.m_entries.m_pArray[m_active.m_sortOrder.m_pArray[i]];
}

// Empties both pages, freeing the storage of each of the four lists.
void CSWCJournal::ClearAll()
{
    // inlined CExoArrayList<SJournalEntry>::Clear()
    if (m_done.m_entries.m_pArray) {
        delete[] m_done.m_entries.m_pArray;
        m_done.m_entries.m_pArray = 0;
        m_done.m_entries.m_capacity = 0;
    }
    m_done.m_entries.m_count = 0;
    // inlined CExoArrayList<int>::Clear()
    if (m_done.m_sortOrder.m_pArray) {
        delete[] m_done.m_sortOrder.m_pArray;
        m_done.m_sortOrder.m_pArray = 0;
        m_done.m_sortOrder.m_capacity = 0;
    }
    m_done.m_sortOrder.m_count = 0;
    // inlined CExoArrayList<SJournalEntry>::Clear()
    if (m_active.m_entries.m_pArray) {
        delete[] m_active.m_entries.m_pArray;
        m_active.m_entries.m_pArray = 0;
        m_active.m_entries.m_capacity = 0;
    }
    m_active.m_entries.m_count = 0;
    // inlined CExoArrayList<int>::Clear()
    if (m_active.m_sortOrder.m_pArray) {
        delete[] m_active.m_sortOrder.m_pArray;
        m_active.m_sortOrder.m_pArray = 0;
        m_active.m_sortOrder.m_capacity = 0;
    }
    m_active.m_sortOrder.m_count = 0;
}

// True when any active entry is still flagged unread.
int CSWCJournal::HasNewEntries()
{
    for (int64_t i = 0; i < m_active.m_entries.m_count; i++) {
        if (m_active.m_entries.m_pArray[i].m_bNewUnobserved)
            return 1;
    }
    return 0;
}

// Constructs the client's world journal, one record list plus a selection order over it, so the
// records never move when the ordering changes.
CSWCWorldJournal::CSWCWorldJournal()
{
    m_nSortBy = JOURNAL_SORT_BY_DATE;
    __builtin_memset(static_cast<void*>(this), 0, offsetof(CSWCWorldJournal, m_nSortBy));
    m_bModified = 0;
}

int CSWCWorldJournal::GetNumEntries()
{
    return m_entries.m_count;
}

// Ordering index to record. The bounds check is against the selection count, which is the shorter of
// the two lists when it truncates.
CWorldJournalEntry* CSWCWorldJournal::GetEntryAt(int nIndex)
{
    if (nIndex < 0)
        return 0;
    if (m_sortOrder.m_count <= nIndex)
        return 0;
    return &m_entries.m_pArray[m_sortOrder.m_pArray[nIndex]];
}

// Resolves both string refs against the talk table, falling back to the empty string when there is no
// row, then files an entry marked as string-ref sourced.
void CSWCWorldJournal::AddEntryStrRef(uint64_t nNameStrRef, uint64_t nTextStrRef,
                                      unsigned int nDate, unsigned int nTime)
{
    STR_RES strName;
    STR_RES strText;
    if (!g_pTlkTable->Fetch(static_cast<unsigned int>(nNameStrRef), strName, CTlkTable::GENDER_MALE))
        strName.text = "";
    if (!g_pTlkTable->Fetch(static_cast<unsigned int>(nTextStrRef), strText, CTlkTable::GENDER_MALE))
        strText.text = "";

    CWorldJournalEntry entry;
    entry.m_nEntryId = static_cast<unsigned int>(nNameStrRef);
    entry.m_sName = strName.text;
    entry.m_sText = strText.text;
    entry.m_nCalendarDay = nDate;
    entry.m_nTimeOfDay = nTime;
    entry.m_nEntryFlags |= 1;

    int nIndex = m_entries.m_count;
    m_entries.Add(entry);
    m_sortOrder.Add(nIndex);
    SortList();
    m_bModified = 1;
}

// Files an entry built from localized strings; the string-ref marker is cleared here.
void CSWCWorldJournal::AddEntry(unsigned int nId, const CExoString& sName, const CExoString& sText,
                                unsigned int nDate, unsigned int nTime)
{
    CWorldJournalEntry entry;
    entry.m_nEntryId = nId;
    entry.m_sName = sName;
    entry.m_sText = sText;
    entry.m_nCalendarDay = nDate;
    entry.m_nTimeOfDay = nTime;
    entry.m_nEntryFlags &= ~1;

    int nIndex = m_entries.m_count;
    m_entries.Add(entry);
    m_sortOrder.Add(nIndex);
    SortList();
    m_bModified = 1;
}

// Scans the records themselves rather than the selection order, and matches only string-ref
// entries. The selection list still needs repairing. The removed position is dropped and every
// remaining index above the removed record shifts down one.
void CSWCWorldJournal::DeleteEntryStrRef(uint64_t nStrRef)
{
    int n = m_entries.m_count;
    for (int i = 0; i < n; i++) {
        if ((m_entries.m_pArray[i].m_nEntryFlags & 1) == 0)
            continue;
        if (m_entries.m_pArray[i].m_nEntryId != nStrRef)
            continue;

        m_entries.DelIndex(i);
        int* pSort = m_sortOrder.m_pArray;
        for (int k = 0; k < n; k++) {
            if (pSort[k] == i) {
                m_sortOrder.DelIndex(k);
                n = m_sortOrder.m_count;
            }
            if (k != n && pSort[k] > i)
                pSort[k]--;
        }
    }
    m_bModified = 1;
}

// The by-id twin, scanning the selection order so the record index comes from m_sortOrder.
void CSWCWorldJournal::DeleteEntry(unsigned int nEntry)
{
    int n = m_entries.m_count;
    for (int i = 0; i < n; i++) {
        int nIndex = m_sortOrder.m_pArray[i];
        if (m_entries.m_pArray[nIndex].m_nEntryFlags & 1)
            continue;
        if (m_entries.m_pArray[nIndex].m_nEntryId != nEntry)
            continue;

        m_entries.DelIndex(nIndex);
        int nRemoved = m_sortOrder.m_pArray[i];
        m_sortOrder.DelIndex(i);
        n = m_sortOrder.m_count;
        for (int k = 0; k < n; k++) {
            if (m_sortOrder.m_pArray[k] > nRemoved)
                m_sortOrder.m_pArray[k]--;
        }
    }
    m_bModified = 1;
}

// The record array and the selection list tear down differently, one through its storage API, the
// other through Clear().
void CSWCWorldJournal::DeleteAllEntries()
{
    m_entries.SetSize(0);
    m_sortOrder.Clear();
    m_bModified = 1;
}

// Only the two implemented sort keys are accepted; anything else is a silent no-op.
void CSWCWorldJournal::SetSortBy(int nSortBy)
{
    if (static_cast<unsigned int>(nSortBy) > 1u)
        return;
    m_nSortBy = static_cast<unsigned char>(nSortBy);
    SortList();
    m_bModified = 1;
}

// Selection-sorts the selection list, never the records. Unlike the other sort paths this leaves the
// changed flag to its caller.
void CSWCWorldJournal::SortList()
{
    int (CSWCWorldJournal::*pfnCompare)(int, int, int);
    if (m_nSortBy == JOURNAL_SORT_BY_DATE)
        pfnCompare = &CSWCWorldJournal::SortItemsByDate;
    else
        pfnCompare = &CSWCWorldJournal::SortItemsByName;

    int count = m_sortOrder.m_count;
    for (int i = 0; i < count; i++) {
        int nMin = i;
        for (int j = i + 1; j < count; j++) {
            if ((this->*pfnCompare)(m_sortOrder.m_pArray[nMin], m_sortOrder.m_pArray[j], 0) < 0)
                nMin = j;
        }
        int nTmp = m_sortOrder.m_pArray[i];
        m_sortOrder.m_pArray[i] = m_sortOrder.m_pArray[nMin];
        m_sortOrder.m_pArray[nMin] = nTmp;
    }
}

// Timestamps first; an exact tie stays 0 instead of falling back.
int CSWCWorldJournal::SortItemsByDate(int a, int b, int c)
{
    int nResult = g_pAppManager->m_pClientExoApp->GetWorldTimer()->CompareWorldTimes(
        m_entries.m_pArray[a].m_nCalendarDay, m_entries.m_pArray[a].m_nTimeOfDay,
        m_entries.m_pArray[b].m_nCalendarDay, m_entries.m_pArray[b].m_nTimeOfDay);
    if (nResult == 0 && c == 0)
        return SortItemsByName(a, b, 1);
    return nResult;
}

// Alphabetical on the entry text, descending; a tie falls through to the date form.
int CSWCWorldJournal::SortItemsByName(int a, int b, int c)
{
    if (m_entries.m_pArray[a].m_sText < m_entries.m_pArray[b].m_sText)
        return 1;
    if (m_entries.m_pArray[a].m_sText > m_entries.m_pArray[b].m_sText)
        return -1;
    if (c == 0)
        return SortItemsByDate(a, b, 1);
    return 0;
}

CSWCWorldJournal::~CSWCWorldJournal()
{
}
