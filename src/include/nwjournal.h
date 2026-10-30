#pragma once

#include "exostring.h"
#include "k2stdint.h"

// idiom-exempt: canonical
// One journal update deferred while the GUI was busy: a record of the arguments
// CSWCJournal::JournalUpdated would have taken. JournalUpdated queues while the busy gate is set,
// and CSWCJournal::CheckForUpdates replays the queue once the gate clears.
class CSWCJournalPendingUpdate {
public:
    CExoString m_sText;
    int        m_nUpdateType;
};

#include "exostring.h"

// idiom-exempt: canonical
// One entry of the world journal, shared by the client's CSWCWorldJournal and the server module's
// own world journal.
//
// The trailing flag byte is the add-path discriminator. Adding by name clears it and adding by tlk
// strref sets it, and each delete path only matches entries whose flag is clear or set
// respectively. The two sort comparators use the second string as the name key and the date pair as
// the date key.
class CWorldJournalEntry {
public:
    CExoString    m_sName;
    CExoString    m_sText;
    unsigned int  m_nCalendarDay;
    unsigned int  m_nTimeOfDay;
    uint64_t      m_nTextStrRef;
    unsigned int  m_nEntryId;
    unsigned char m_nEntryFlags;
};
