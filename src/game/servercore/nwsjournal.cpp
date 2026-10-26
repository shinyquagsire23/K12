
#include "k2stdint.h"
#include "nwmessagetypes.h"
#include "restype.h"
#include "appmanager.h"
#include "CExoArrayListImpl.h"
#include "resgff.h"
#include "exoresman.h"
#include "serverexoapp.h"
#include "nwsjournal.h"
#include "nwssendplayermessage.h"
#include "nwsplayer.h"
#include "nwsrules.h"

CSWSJournal::CSWSJournal()
{
}

CSWSJournal::~CSWSJournal()
{
}

// Unused stub; the body only returns.
void CSWSJournal::SetPicture(const CExoString& sPlotId, int nPictureId, int bFlag)
{
    K2_UNUSED(sPlotId); K2_UNUSED(nPictureId); K2_UNUSED(bFlag);
}

// Returns the quest's progress state, or 0 when the plot has no journal entry. The same scan as
// GetQuestEntryXP.
int CSWSJournal::GetState(const CExoString& sPlotId)
{
    int nCount = m_entries.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_entries.m_pArray[i].m_sPlotId.CompareNoCase(sPlotId))
            return m_entries.m_pArray[i].m_nState;
    }
    return 0;
}

int CSWSJournal::GetQuestEntryXP(const CExoString& sPlotId)
{
    int nCount = m_entries.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_entries.m_pArray[i].m_sPlotId.CompareNoCase(sPlotId))
            return m_entries.m_pArray[i].m_nQuestEntryXP;
    }
    return 0;
}

// Advances a quest to a new state. The only journal mutator that consults the module's journal
// resource rather than poking a field: it opens global.jrl, finds this quest's category, refreshes
// the live entry from it (title, priority, picture, plot and planet ids), then finds the numbered
// entry matching the new state and refreshes its text, XP and end flag. Each field that changed sets
// its own bit in nFlags, so the pending-update record tells the client what to re-render.
void CSWSJournal::SetState(const CExoString& sPlotId, unsigned int nState, int bFlag)
{
    SJournalEntry entry;
    int nCount = m_entries.m_count;
    int bSuccess = 1;
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSPlayerJournalQuestUpdates update;
    CExoLocString locString;
    CExoString sTag;

    short nFlags = bFlag ? MSG_JOURNAL_QUEST_OBSERVED : 0;

    int bAlreadyPresent = 0;
    int i;
    for (i = 0; i < nCount; i++) {
        if (m_entries.m_pArray[i].m_sPlotId.CompareNoCase(sPlotId)) {
            bAlreadyPresent = 1;
            break;
        }
    }
    if (!bAlreadyPresent) {
        entry.m_sPlotId = sPlotId.LowerCase();
        entry.m_nState = nState;
        m_entries.Add(entry);
        nFlags |= MSG_JOURNAL_QUEST_ALL;
        i = m_entries.m_count - 1;
    }

    CResStruct topStruct;
    CResList catList;
    CResGFF* pGff = new CResGFF(RESTYPE_JRL, const_cast<char*>("JRL "), CResRef("Global"));
    if (pGff->m_nValid) {
        pGff->GetTopLevelStruct(&topStruct);
        pGff->GetList(&catList, &topStruct, const_cast<char*>("Categories"));

        CResStruct catStruct;
        int nCategories;
        int c;
        unsigned int nPriority;
        unsigned short nPicture;
        int nPlotIndex;
        int nPlanetId;
        CServerExoApp* pServerApp;
        CSWSPlayer* pPlayer;
        CResList entryList;
        nCategories = pGff->GetListCount(&catList);
        for (c = 0; c < nCategories; c++) {
            pGff->GetListElement(&catStruct, &catList, c);
            sTag = pGff->ReadFieldCExoString(&catStruct, const_cast<char*>("Tag"), bSuccess, CExoString(""));
            if (bSuccess && sTag != "" && sPlotId.CompareNoCase(sTag)) {
                // For this quest's category, refresh the live entry from the resource.
                if ((locString = pGff->ReadFieldCExoLocString(&catStruct, const_cast<char*>("Name"), bSuccess,
                                                              CExoLocString()))
                    != m_entries.m_pArray[i].m_locName) {
                    m_entries.m_pArray[i].m_locName = locString;
                    nFlags |= MSG_JOURNAL_QUEST_NAME;
                }

                nPriority = pGff->ReadFieldDWORD(&catStruct, const_cast<char*>("Priority"), bSuccess, 0);
                if (nPriority != m_entries.m_pArray[i].m_nPriority) {
                    nFlags |= MSG_JOURNAL_QUEST_PRIORITY;
                    m_entries.m_pArray[i].m_nPriority = nPriority;
                }
                if (m_entries.m_pArray[i].m_nState != static_cast<int>(nState)) {
                    nFlags |= MSG_JOURNAL_QUEST_STATE;
                    m_entries.m_pArray[i].m_nState = nState;
                }
                nPicture = pGff->ReadFieldWORD(&catStruct, const_cast<char*>("Picture"), bSuccess, 0);
                if (m_entries.m_pArray[i].m_nPicture != nPicture) {
                    nFlags |= MSG_JOURNAL_QUEST_PICTURE;
                    m_entries.m_pArray[i].m_nPicture = nPicture;
                }
                nPlotIndex = pGff->ReadFieldINT(&catStruct, const_cast<char*>("PlotIndex"), bSuccess, 0);
                if (nPlotIndex != m_entries.m_pArray[i].m_nPlotIndex) {
                    nFlags |= MSG_JOURNAL_QUEST_PLOT_INDEX;
                    m_entries.m_pArray[i].m_nPlotIndex = nPlotIndex;
                }
                nPlanetId = pGff->ReadFieldINT(&catStruct, const_cast<char*>("PlanetID"), bSuccess, 0);
                if (nPlanetId != m_entries.m_pArray[i].m_nPlanetId) {
                    nFlags |= MSG_JOURNAL_QUEST_PLANET;
                    m_entries.m_pArray[i].m_nPlanetId = nPlanetId;
                }

                // Among the category's numbered entries, find the one this state selects and take
                // its text.
                pGff->GetList(&entryList, &catStruct, const_cast<char*>("EntryList"));
                CResStruct entryStruct;
                int nEntries;
                int k;
                float fXpPercentage;
                int nPlotXP;
                int nAward;
                unsigned short nEnd;
                CSWSPlayerJournalState* pState;
                int bFoundRecord;
                int nUpdCount;
                nEntries = pGff->GetListCount(&entryList);
                for (k = 0; k < nEntries; k++) {
                    pGff->GetListElement(&entryStruct, &entryList, k);
                    if (pGff->ReadFieldDWORD(&entryStruct, const_cast<char*>("ID"), bSuccess, 0) == nState) {
                        if ((locString = pGff->ReadFieldCExoLocString(
                                 &entryStruct, const_cast<char*>("Text"), bSuccess, CExoLocString()))
                            != m_entries.m_pArray[i].m_locText) {
                            m_entries.m_pArray[i].m_locText = locString;
                            nFlags |= MSG_JOURNAL_QUEST_TEXT;
                        }

                        // The XP award is a percentage of the plot's own XP budget, which lives
                        // in the "Plot" table at this quest's plot index row. Rounded half up.
                        fXpPercentage = pGff->ReadFieldFLOAT(
                            &entryStruct, const_cast<char*>("XP_Percentage"), bSuccess, 0.0f);
                        if (bSuccess) {
                            nPlotXP = 0;
                            g_pRules->m_p2DArrays->m_pPlotXP->GetINTEntry(
                                m_entries.m_pArray[i].m_nPlotIndex, CExoString("XP"), &nPlotXP);
                            nAward = static_cast<int>(fXpPercentage * static_cast<float>(nPlotXP) + 0.5f);
                            if (m_entries.m_pArray[i].m_nQuestEntryXP != nAward) {
                                nFlags |= MSG_JOURNAL_QUEST_EXPERIENCE;
                                m_entries.m_pArray[i].m_nQuestEntryXP = nAward;
                            }
                        }

                        nEnd = pGff->ReadFieldWORD(&entryStruct, const_cast<char*>("End"), bSuccess, 0);
                        if (m_entries.m_pArray[i].m_bEnd != nEnd)
                            m_entries.m_pArray[i].m_bEnd = static_cast<unsigned char>(nEnd);
                        nFlags |= MSG_JOURNAL_QUEST_END;
                        break;
                    }
                }

                m_entries.m_pArray[i].m_bNewUnobserved = (bFlag == 0);

                pServerApp = g_pAppManager->m_pServerExoApp;
                pPlayer = pServerApp->GetClientObjectByObjectId(pServerApp->GetPlayerCreatureId());
                if (pPlayer != 0) {
                    pState = pPlayer->m_pJournalState;
                    if (pState->m_bQuestScreenOpen) {
                        pMessage->SendServerToPlayerJournalAddQuest(
                            pPlayer, sPlotId.LowerCase(), nState,
                            m_entries.m_pArray[i].m_nPriority, m_entries.m_pArray[i].m_nPicture,
                            m_entries.m_pArray[i].m_bEnd, m_entries.m_pArray[i].m_nDate,
                            m_entries.m_pArray[i].m_nTime, m_entries.m_pArray[i].m_locName,
                            m_entries.m_pArray[i].m_locText, m_entries.m_pArray[i].m_nPlotIndex,
                            m_entries.m_pArray[i].m_nPlanetId,
                            m_entries.m_pArray[i].m_nQuestEntryXP);
                    } else if (!pState->m_bFullUpdateNeeded) {
                        bFoundRecord = 0;
                        nUpdCount = pState->m_updates.m_count;
                        for (int j = 0; j < nUpdCount; j++) {
                            if (pState->m_updates.m_pArray[j].m_sQuestTag.CompareNoCase(sPlotId)) {
                                CSWSPlayerJournalQuestUpdates& rec = pState->m_updates.m_pArray[j];
                                if (bAlreadyPresent)
                                    rec.m_updateFlags = static_cast<unsigned short>((rec.m_updateFlags & ~MSG_JOURNAL_QUEST_REMOVED) | nFlags);
                                else
                                    rec.m_updateFlags = MSG_JOURNAL_QUEST_ALL;
                                bFoundRecord = 1;
                                break;
                            }
                        }
                        if (!bFoundRecord) {
                            if (bAlreadyPresent)
                                update.m_updateFlags = nFlags;
                            else
                                // sic: `update` is a fresh record whose flags the implicit default
                                // constructor never initializes, so this ORs into uninitialized stack.
                                update.m_updateFlags |= MSG_JOURNAL_QUEST_ALL;
                            update.m_sQuestTag = sPlotId.LowerCase();
                            pState->m_updates.Add(update);
                        }
                    }
                    if (!bFlag)
                        pMessage->SendServerToPlayerJournalUpdated(
                            pPlayer, 1, m_entries.m_pArray[i].m_bEnd,
                            m_entries.m_pArray[i].m_locName);
                }
                goto done;
            }
        }

        // With no category authored for this plot, nothing was refreshed, so there is no delta
        // worth recording. Just hand the player the entry as it stands if they are looking at it.
        delete pGff;
        {
            pServerApp = g_pAppManager->m_pServerExoApp;
            pPlayer = pServerApp->GetClientObjectByObjectId(pServerApp->GetPlayerCreatureId());
            if (pPlayer != 0 && pPlayer->m_pJournalState != 0
                && pPlayer->m_pJournalState->m_bQuestScreenOpen) {
                pMessage->SendServerToPlayerJournalAddQuest(
                    pPlayer, sPlotId.LowerCase(), nState, m_entries.m_pArray[i].m_nPriority,
                    m_entries.m_pArray[i].m_nPicture, m_entries.m_pArray[i].m_bEnd,
                    m_entries.m_pArray[i].m_nDate, m_entries.m_pArray[i].m_nTime,
                    m_entries.m_pArray[i].m_locName, m_entries.m_pArray[i].m_locText,
                    m_entries.m_pArray[i].m_nPlotIndex, m_entries.m_pArray[i].m_nPlanetId,
                    m_entries.m_pArray[i].m_nQuestEntryXP);
            }
        }
        return;
    }
done:
    delete pGff;
}

// Sets a quest's journal date, adding the quest if the player has never seen it, and records
// a pending "date changed" delta so an open quest screen can be updated incrementally.
// SetTime is its twin, with the same body, a different field and a different delta bit.
void CSWSJournal::SetDate(const CExoString& sPlotId, unsigned int nDate, int bFlag)
{
    SJournalEntry entry;
    CSWSPlayerJournalQuestUpdates update;
    CServerExoApp* pServerApp;
    CSWSPlayer* pPlayer;
    CSWSPlayerJournalState* pState;
    int nUpdCount;
    unsigned short flags;

    int bAlreadyPresent = 0;
    int nCount = m_entries.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_entries.m_pArray[i].m_sPlotId.CompareNoCase(sPlotId)) {
            m_entries.m_pArray[i].m_nDate = nDate;
            bAlreadyPresent = 1;
            break;
        }
    }
    if (!bAlreadyPresent) {
        entry.m_sPlotId = sPlotId.LowerCase();
        entry.m_nDate = nDate;
        m_entries.Add(entry);
    }

    pServerApp = g_pAppManager->m_pServerExoApp;
    pPlayer = pServerApp->GetClientObjectByObjectId(pServerApp->GetPlayerCreatureId());
    if (pPlayer == 0)
        return;
    pState = pPlayer->m_pJournalState;
    if (pState->m_bFullUpdateNeeded)
        return;

    nUpdCount = pState->m_updates.m_count;
    for (int j = 0; j < nUpdCount; j++) {
        if (pState->m_updates.m_pArray[j].m_sQuestTag.CompareNoCase(sPlotId)) {
            CSWSPlayerJournalQuestUpdates& rec = pState->m_updates.m_pArray[j];
            if (bAlreadyPresent)
                rec.m_updateFlags = (rec.m_updateFlags & ~MSG_JOURNAL_QUEST_REMOVED) | MSG_JOURNAL_QUEST_DATE;
            else
                rec.m_updateFlags = MSG_JOURNAL_QUEST_ALL;
            if (bFlag)
                rec.m_updateFlags |= MSG_JOURNAL_QUEST_OBSERVED;
            return;
        }
    }
    update.m_sQuestTag = sPlotId.LowerCase();
    flags = bAlreadyPresent ? MSG_JOURNAL_QUEST_DATE : MSG_JOURNAL_QUEST_ALL;
    if (bFlag)
        flags |= MSG_JOURNAL_QUEST_OBSERVED;
    update.m_updateFlags = flags;
    pState->m_updates.Add(update);
}

// Sets a quest's journal time. SetDate's twin in every respect; see its notes.
void CSWSJournal::SetTime(const CExoString& sPlotId, unsigned int nTime, int bFlag)
{
    SJournalEntry entry;
    CSWSPlayerJournalQuestUpdates update;
    CServerExoApp* pServerApp;
    CSWSPlayer* pPlayer;
    CSWSPlayerJournalState* pState;
    int nUpdCount;
    unsigned short flags;

    int bAlreadyPresent = 0;
    int nCount = m_entries.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_entries.m_pArray[i].m_sPlotId.CompareNoCase(sPlotId)) {
            m_entries.m_pArray[i].m_nTime = nTime;
            bAlreadyPresent = 1;
            break;
        }
    }
    if (!bAlreadyPresent) {
        entry.m_sPlotId = sPlotId.LowerCase();
        entry.m_nTime = nTime;
        m_entries.Add(entry);
    }

    pServerApp = g_pAppManager->m_pServerExoApp;
    pPlayer = pServerApp->GetClientObjectByObjectId(pServerApp->GetPlayerCreatureId());
    if (pPlayer == 0)
        return;
    pState = pPlayer->m_pJournalState;
    if (pState->m_bFullUpdateNeeded)
        return;

    nUpdCount = pState->m_updates.m_count;
    for (int j = 0; j < nUpdCount; j++) {
        if (pState->m_updates.m_pArray[j].m_sQuestTag.CompareNoCase(sPlotId)) {
            CSWSPlayerJournalQuestUpdates& rec = pState->m_updates.m_pArray[j];
            if (bAlreadyPresent)
                rec.m_updateFlags = (rec.m_updateFlags & ~MSG_JOURNAL_QUEST_REMOVED) | MSG_JOURNAL_QUEST_TIME;
            else
                rec.m_updateFlags = MSG_JOURNAL_QUEST_ALL;
            if (bFlag)
                rec.m_updateFlags |= MSG_JOURNAL_QUEST_OBSERVED;
            return;
        }
    }
    update.m_sQuestTag = sPlotId.LowerCase();
    flags = bAlreadyPresent ? MSG_JOURNAL_QUEST_TIME : MSG_JOURNAL_QUEST_ALL;
    if (bFlag)
        flags |= MSG_JOURNAL_QUEST_OBSERVED;
    update.m_updateFlags = flags;
    pState->m_updates.Add(update);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
SJournalEntry::SJournalEntry()
    : m_nDate(-1), m_nTime(-1)
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
#endif // !KOTOR1

SJournalEntry& SJournalEntry::operator=(const SJournalEntry& other)
{
    m_locName = other.m_locName;
    m_locText = other.m_locText;
    m_nDate = other.m_nDate;
    m_nTime = other.m_nTime;
    m_sPlotId = other.m_sPlotId;
    m_nState = other.m_nState;
    m_nPriority = other.m_nPriority;
    m_nPicture = other.m_nPicture;
    m_bEnd = other.m_bEnd;
    m_bReserved1 = other.m_bReserved1;
    m_bNewUnobserved = other.m_bNewUnobserved;
    m_nPlotIndex = other.m_nPlotIndex;
    m_nPlanetId = other.m_nPlanetId;
    m_nQuestEntryXP = other.m_nQuestEntryXP;
    return *this;
}

// Merges selectively, overwriting this entry's display fields from `other`, one field per mask bit.
// Bit 15 is a blanket "take everything" flag; the plain fields each have a bit, and the plot id is
// always copied because the entry's identity travels with the merge.
void SJournalEntry::Update(SJournalEntry other, unsigned short nFlags)
{
    m_sPlotId = other.m_sPlotId;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_NAME))
        m_locName = other.m_locName;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_TEXT))
        m_locText = other.m_locText;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_DATE))
        m_nDate = other.m_nDate;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_TIME))
        m_nTime = other.m_nTime;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_STATE))
        m_nState = other.m_nState;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_PRIORITY))
        m_nPriority = other.m_nPriority;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_END))
        m_bEnd = other.m_bEnd;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_PLOT_INDEX))
        m_nPlotIndex = other.m_nPlotIndex;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_PLANET))
        m_nPlanetId = other.m_nPlanetId;
    if (nFlags & (MSG_JOURNAL_QUEST_ALL | MSG_JOURNAL_QUEST_EXPERIENCE))
        m_nQuestEntryXP = other.m_nQuestEntryXP;
    m_bNewUnobserved = 1;
}

// Removes the entry with this plot id and tells the connected player about it. The scan is
// backward and case-insensitive, and a miss returns silently. The loop is spelled
// `while (--i >= 0)` so its head is the decrement-and-test, and the shift copies from the
// source index so the walking base anchors on the destination element.
void CSWSJournal::Destroy(const CExoString& sPlotId)
{
    int64_t i = m_entries.m_count;
    CSWSMessage* pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSPlayerJournalQuestUpdates update;
    CExoLocString locName;
    int bEnd;
    CServerExoApp* pApp;
    CSWSPlayer* pPlayer;
    CSWSPlayerJournalState* pState;
    int nCount;
    int k;

    while (--i >= 0) {
        if (m_entries.m_pArray[i].m_sPlotId.CompareNoCase(sPlotId))
            break;
    }
    if (i < 0)
        return;

    bEnd = m_entries.m_pArray[i].m_bEnd;
    locName = m_entries.m_pArray[i].m_locName;
    // inlined CExoArrayList<SJournalEntry>::DelIndex(i)
    m_entries.m_count--;
    for (int64_t j = i + 1; j <= m_entries.m_count; j++)
        m_entries.m_pArray[j - 1] = m_entries.m_pArray[j];

    pApp = g_pAppManager->m_pServerExoApp;
    pPlayer = pApp->GetClientObjectByObjectId(pApp->GetPlayerCreatureId());
    if (!pPlayer)
        return;

    pState = pPlayer->m_pJournalState;
    if (pState->m_bQuestScreenOpen) {
        pMsg->SendServerToPlayerJournalRemoveQuest(pPlayer, sPlotId.LowerCase());
        return;
    }

    if (!pState->m_bFullUpdateNeeded) {
        nCount = pState->m_updates.m_count;
        for (k = 0; k < nCount; k++) {
            if (pState->m_updates.m_pArray[k].m_sQuestTag.CompareNoCase(sPlotId))
                break;
        }
        if (k >= nCount) {
            update.m_updateFlags = MSG_JOURNAL_QUEST_REMOVED;
            update.m_sQuestTag = sPlotId.LowerCase();
            pState->m_updates.Add(update);
        } else {
            pState->m_updates.m_pArray[k].m_updateFlags = MSG_JOURNAL_QUEST_REMOVED;
        }
    }

    pMsg->SendServerToPlayerJournalUpdated(pPlayer, 1, bEnd, locName);
}

// Merges another journal's entries into this one, skipping duplicates, and reports how many
// landed. Only the entries that were already here are re-scanned, so two copies of one
// plot id in `pOther` both get through.
int CSWSJournal::CopyEntries(CSWSJournal* pOther)
{
    int nBefore = m_entries.m_count;
    for (int64_t j = 0; j < pOther->m_entries.m_count; j++) {
        int i;
        for (i = 0; i < nBefore; i++) {
            if (m_entries.m_pArray[i].m_sPlotId.CompareNoCase(pOther->m_entries.m_pArray[j].m_sPlotId))
                break;
        }
        if (i != nBefore)
            continue;
        SJournalEntry entry = pOther->m_entries.m_pArray[j];
        m_entries.Add(entry);
    }
    return m_entries.m_count - nBefore;
}

// Instantiated here rather than in the CExoArrayList home TU: operator= is defined in this
// TU, and this is the surviving copy that was emitted where that definition is visible.
template void CExoArrayList<SJournalEntry>::Add(SJournalEntry);

// Instantiated here because the surviving weak copy was emitted from this TU, at the tail of
// the CSWSJournal run. Its only instantiated member is this one, so it is the member-only
// form rather than a whole-class instantiation.
template void CExoArrayList<CSWSPlayerJournalQuestUpdates>::Allocate(int);
