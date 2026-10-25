#pragma once
#include "k2stdint.h"

#ifndef CSWGUIINGAMEJOURNAL_H
#define CSWGUIINGAMEJOURNAL_H

#include "swgui.h"
#include "swguicontrol.h"
#include "CExoArrayListDtors.h"
#include "swguiingame.h"
#if KOTOR1
#include "swguiquestitem.h"
#endif

class CSWGuiControl;
class CSWGuiManager;
struct SJournalEntry;


// The in-game quest screen has a title, a filter row, a list box of quest rows, a description box
// for the selected row, and a chained messages sub-panel the buttons swap to.
class CSWGuiInGameJournal : public CSWGuiPanel {
public:
    virtual ~CSWGuiInGameJournal();
    virtual void Draw(float f);
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();
    virtual void OnPanelRemoved();
    // Maps a list-box row index to the journal record behind it, picking the done or the active
    // page off CGuiInGame's flag (the shown page lives there, not on this class).
    virtual SJournalEntry* GetQuestEntryAt(int nIndex);

    CSWGuiInGameJournal(CSWGuiManager* pGuiManager);

    // Caption StrRefs, indexed by sort type (0 date / 1 name / 2 priority / 3 planet).
    static uint64_t s_pnSortLabels[4];

    void PopulateItemListBox();
    // Keep out of line: the four call sites are plain calls.
#if !KOTOR1  // K1: no filter row
    __attribute__((noinline)) void UpdateFilterLabelHighlights();
#endif
    void MarkNewQuestsObserved();
    // Non-virtual mouse-over hooks the rows are AddEvent()ed with.
    void OnControlEntered(CSWGuiControl* pControl);
#if !KOTOR1  // K1: rows have no exit handler and there is no filter row
    void OnControlExited(CSWGuiControl* pControl);
    void OnFilterPressed(CSWGuiControl* pControl);
#endif
    // The filter row cycles 0 -> 1 -> 3 -> 0, because sort key 2 has no button on this screen.
    int  GetNextSortType();
    int  GetPreviousSortType();

    CSWGuiLabel   m_lblTitle;
    CSWGuiListBox m_lbDescription;
    CSWGuiLabel   m_lblDescription;
    CSWGuiListBox m_lbItems;
#if KOTOR1  // K1: quest items / swap / sort / exit, no filter row and no bars
    CSWGuiButton  m_btnQuestItems;
    CSWGuiButton  m_btnSwapText;
    CSWGuiButton  m_btnSort;
    CSWGuiButton  m_btnExit;
    CSWGuiQuestItem* m_pQuestItem;
#else
    CSWGuiButton  m_btnSwapText;
    CSWGuiButton  m_btnExit;
    CSWGuiButton  m_btnFilterTime;
    CSWGuiButton  m_btnFilterName;
    CSWGuiButton  m_btnFilterPlanet;
    CSWGuiButton  m_btnMessages;
    CSWGuiLabel   m_aBars[5];
    CSWGuiInGameMessages* m_pMessages;
#endif
    CSWGuiControl* m_pHoveredEntry;
    CExoArrayList<int> m_aNewQuestRows;
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWGuiInGameJournal) == 0x1530, "CSWGuiInGameJournal K1 size drift");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiInGameJournal) == 0x23d0, "CSWGuiInGameJournal size drift");
#endif

#endif // CSWGUIINGAMEJOURNAL_H

#ifndef CSWGUIJOURNALITEMENTRY_H
#define CSWGUIJOURNALITEMENTRY_H

#include "swguicontrol.h"

// One row of the quest list box. It is a CSWGuiButton with two extra fields and no overrides.
class CSWGuiJournalItemEntry : public CSWGuiButton {
public:
    int           m_nEntryIndex;
    unsigned char m_bNewHilight : 1;
    unsigned char _rest26c      : 7;
    char          _pad26d[0x270 - 0x26d];

#if !KOTOR1  // K1 rows only ever take the purple highlight (PopulateItemListBox writes it out)
    // Turns the new-quest highlight on (a half-alpha bonus-coloured frame) or back to the plain
    // row colours; a no-op when the row is already in that state.
    void SetEncouraged(int bEncouraged);
#endif
};

#if !KOTOR1
#include "guiingame.h"

inline void CSWGuiJournalItemEntry::SetEncouraged(int bEncouraged)
{
    if (m_bNewHilight == bEncouraged)
        return;
    m_bNewHilight = bEncouraged;
    if (m_bNewHilight)
    {
        m_border.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
        m_border.m_params.m_fAlpha = 0.5f;
        m_borderHilight.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
    }
    else
    {
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        m_borderHilight.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
    }
}
#endif

#endif // CSWGUIJOURNALITEMENTRY_H
