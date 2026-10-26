#include "k2stdint.h"
#include "strref.h"
#include "swguiingamejournal.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "nwcjournal.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "nwtlktable.h"
#include "swparty.h"
#include "nwccreature.h"
#include "CExoArrayListImpl.h"

#include "twodimarray.h"
#include "nwsrules.h"
#include "twodimarrays.h"

int g_nJournalSortType;
int g_bOverrideJournalSortType;


float GetResolutionTextScalar();

#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "exoinput.h"
extern CTlkTable* g_pTlkTable;
void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
#include "port/k1gui.h"
#include "aurgui.h"
extern bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
#endif

uint64_t CSWGuiInGameJournal::s_pnSortLabels[4] = {
    STRREF_BY_ORDER_RECEIVED, STRREF_BY_NAME, STRREF_BY_PRIORITY, STRREF_BY_PLANET,
};

int CSWGuiInGameJournal::GetNextSortType()
{
    int nSortType = g_nJournalSortType + 1;
    if (nSortType == JOURNAL_SORT_BY_PRIORITY)
        nSortType = JOURNAL_SORT_BY_PLANET;
    if (nSortType > JOURNAL_SORT_BY_PLANET)
        nSortType = JOURNAL_SORT_BY_DATE;
    return nSortType;
}

int CSWGuiInGameJournal::GetPreviousSortType()
{
    int nSortType = g_nJournalSortType - 1;
    if (nSortType == JOURNAL_SORT_BY_PRIORITY)
        nSortType = JOURNAL_SORT_BY_NAME;
    if (nSortType < 0)
        nSortType = JOURNAL_SORT_BY_PLANET;
    return nSortType;
}

#if !KOTOR1  // K1 has one BTN_SORT caption instead of the three filter buttons
void CSWGuiInGameJournal::UpdateFilterLabelHighlights()
{
    m_btnFilterTime.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnFilterName.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnFilterPlanet.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    CSWGuiTextParams* pParams;
    switch (g_nJournalSortType)
    {
        case JOURNAL_SORT_BY_DATE:    pParams = &m_btnFilterTime.m_text.m_params;   break;
        case JOURNAL_SORT_BY_NAME:    pParams = &m_btnFilterName.m_text.m_params;   break;
        case JOURNAL_SORT_BY_PLANET:  pParams = &m_btnFilterPlanet.m_text.m_params; break;
        default:                      return;
    }
    pParams->SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
}

// One handler for all three filter buttons; each carries its sort key in the control's user data.
void CSWGuiInGameJournal::OnFilterPressed(CSWGuiControl* pControl)
{
    g_nJournalSortType = static_cast<int>(pControl->m_tail.m_nUserData);

    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
    void (CSWCJournal::*pfnSetSortBy)(int) =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
            ? &CSWCJournal::SetDoneSortBy
            : &CSWCJournal::SetActiveSortBy;
    (pJournal->*pfnSetSortBy)(g_nJournalSortType);

    UpdateFilterLabelHighlights();
}
#endif

CSWGuiInGameJournal::~CSWGuiInGameJournal()
{
    m_lbDescription.ClearItems();
    m_lbItems.ClearItems();
#if KOTOR1  // K1 does not clear the pointer
    if (m_pQuestItem)
        delete m_pQuestItem;
#else
    if (m_pMessages)
    {
        delete m_pMessages;
        m_pMessages = 0;
    }
#endif
}

void CSWGuiInGameJournal::Draw(float fDelta)
{
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
    if ((!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
             && pJournal->m_active.m_bHasChanged)
        || (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
             && pJournal->m_done.m_bHasChanged))
    {
        PopulateItemListBox();
    }
    CSWGuiPanel::Draw(fDelta);
}

void CSWGuiInGameJournal::MarkNewQuestsObserved()
{
    for (int64_t i = 0; i < m_aNewQuestRows.m_count; i++)
    {
        SJournalEntry* pEntry = GetQuestEntryAt(m_aNewQuestRows.m_pArray[i]);
        if (pEntry)
            pEntry->m_bNewUnobserved = 0;
    }
    m_aNewQuestRows.SetSize(0);
}

#if KOTOR1  // K1 also relabels the sort button and the page title
void CSWGuiInGameJournal::OnPanelAdded()
{
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();

    int nSortType;
    if (g_bOverrideJournalSortType)
    {
        nSortType = JOURNAL_SORT_BY_DATE;
        g_bOverrideJournalSortType = 0;
    }
    else
    {
        nSortType = g_nJournalSortType;
    }

    void (CSWCJournal::*pfnSetSortBy)(int) =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
            ? &CSWCJournal::SetDoneSortBy
            : &CSWCJournal::SetActiveSortBy;
    (pJournal->*pfnSetSortBy)(nSortType);

    int nNextSort = nSortType + 1;
    if (nNextSort > JOURNAL_SORT_BY_PLANET)
        nNextSort = JOURNAL_SORT_BY_DATE;
    CExoString sTitle = g_pTlkTable->GetSimpleString(STRREF_SORT) + CExoString(" ")
                      + g_pTlkTable->GetSimpleString(s_pnSortLabels[nNextSort]);
    m_btnSort.m_text.m_params.SetText(sTitle);
    IosAddButtonIcon(&m_btnSort, 'a');

    sTitle = g_pTlkTable->GetSimpleString(
                 g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
                     ? STRREF_COMPLETED_QUESTS : STRREF_ACTIVE_QUESTS)
           + CExoString(" - ") + g_pTlkTable->GetSimpleString(s_pnSortLabels[nSortType]);
    m_lblTitle.m_text.m_params.SetText(sTitle);

    PopulateItemListBox();
    m_btnExit.HandleMouseOver(0);
    CSWGuiPanel::OnPanelAdded();
}
#else
void CSWGuiInGameJournal::OnPanelAdded()
{
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();

    // A journal update that landed while the screen was closed forces the default sort, once.
    int nSortType;
    if (g_bOverrideJournalSortType)
    {
        nSortType = JOURNAL_SORT_BY_DATE;
        g_bOverrideJournalSortType = 0;
    }
    else
    {
        nSortType = g_nJournalSortType;
    }

    void (CSWCJournal::*pfnSetSortBy)(int) =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
            ? &CSWCJournal::SetDoneSortBy
            : &CSWCJournal::SetActiveSortBy;
    (pJournal->*pfnSetSortBy)(nSortType);

    PopulateItemListBox();
    m_btnExit.HandleMouseOver(0);
    CSWGuiPanel::OnPanelAdded();
}
#endif

// Deliberately does not chain to the base's OnPanelRemoved.
void CSWGuiInGameJournal::OnPanelRemoved()
{
    // Puts the chained messages panel back into a non-drawn state.
#if !KOTOR1  // K1 has no messages sub-panel to put away
    if (m_pGuiManager->PanelExists(m_pMessages))
    {
        unsigned short nFlags = m_pMessages->m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_pMessages->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
        m_pMessages->m_stateFlags = nFlags;
    }
#endif
    MarkNewQuestsObserved();
}

#if !KOTOR1  // K1 rows have no exit handler
void CSWGuiInGameJournal::OnControlExited(CSWGuiControl* pControl)
{
    static_cast<CSWGuiJournalItemEntry*>(pControl)->SetEncouraged(0);
}
#endif

SJournalEntry* CSWGuiInGameJournal::GetQuestEntryAt(int nIndex)
{
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests)
    {
        if (pJournal->GetNumDoneEntries() > nIndex)
            return pJournal->GetDoneEntryAt(nIndex);
    }
    else
    {
        if (pJournal->GetNumActiveEntries() > nIndex)
            return pJournal->GetActiveEntryAt(nIndex);
    }
    return 0;
}

// Rebuilds the quest list: one row per entry on the selected page, sized off the list box's
// prototype row.
void CSWGuiInGameJournal::PopulateItemListBox()
{
#if KOTOR1 && defined(K2_UI_TOUCH)  // K1 applies the phone text scale to every row on an iPhone-class screen
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
#endif
    CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
    CExoString sText;
    int nLanguage = g_pAppManager->m_pClientExoApp->GetClientLanguage();
    CExoArrayList<CSWGuiControl*> aRows;

    CSWGuiButton* pProtoRow = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    CSWGuiExtent extent = pProtoRow->m_extent;
    extent.w = m_lbItems.m_innerExtent.w - m_lbItems.m_nPadding * 2;
#if !KOTOR1  // K1: the row height is not text-scaled
    extent.h = static_cast<int>(GetResolutionTextScalar() * static_cast<float>(extent.h));
#elif !defined(K2_UI_TOUCH)
    // pc / xbox: the PC prototype's height is in the 480-line layout's units
    extent.h = static_cast<int>(static_cast<float>(g_SDL_mode.h) / 480.0f * static_cast<float>(extent.h));
#endif

    for (int i = 0; ; i++)
    {
        SJournalEntry* pEntry = GetQuestEntryAt(i);
        if (pEntry == 0)
            break;

        CSWGuiJournalItemEntry* pRow = new CSWGuiJournalItemEntry;
        pRow->m_nEntryIndex = i;
        pRow->m_bNewHilight = 0;
        pRow->Initialize(extent, pProtoRow);
        pRow->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                    &CSWGuiInGameJournal::OnControlEntered));
#if !KOTOR1  // K1 rows get the enter event only
        pRow->AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                    &CSWGuiInGameJournal::OnControlExited));
#endif

        pEntry->m_locName.GetString(nLanguage, &sText, 0);
#if KOTOR1  // K1: no <CUSTOM>/{gender} token expansion; phone-sized row text instead
        pRow->m_text.m_params.SetText(sText);
#ifdef K2_UI_TOUCH  // the phone caption size; pc / xbox keep the 1.3x string default
        int nScreenWidth;
        int nScreenHeight;
        if (IosIsIphone(nScreenWidth, nScreenHeight) && pRow->m_text.m_pString)
            pRow->m_text.m_pString->SetFontScale(s_fScale);
#endif
#else
        if (sText.Find('<', 0) != -1 || sText.Find('{', 0) != -1)
        {
            CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            g_pSWTlkTable->ParseStr(sText, pPC, 1, 1);
        }
        pRow->m_text.m_params.SetText(sText);
#endif

#if KOTOR1
        if (pEntry->m_bNewUnobserved && !pRow->m_bNewHilight)
        {
            pRow->m_bNewHilight = 1;
            pRow->m_border.m_params.m_fAlpha = 0.5f;
            pRow->m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
            pRow->m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
        }
#else
        if (pEntry->m_bNewUnobserved)
            pRow->SetEncouraged(1);
#endif
        aRows.Add(pRow);
    }

    m_lbItems.AddControls(aRows, 1, 1, 0);
    // The hovered row is one of the widgets AddControls just replaced.
    if (m_pHoveredEntry)
        m_pHoveredEntry->SetActive(0);

    if (aRows.m_count >= 1)
    {
        m_lbItems.SetSelectedControl(0, 0);
    }
    else
    {
        m_lbItems.SetSelectedControl(-1, 0);
        m_lblDescription.m_text.m_params.SetText("");
        OnControlEntered(0);
    }

    void (CSWCJournal::*pfnClearHasChanged)() =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
            ? &CSWCJournal::ClearDoneHasChanged
            : &CSWCJournal::ClearActiveHasChanged;
    (pJournal->*pfnClearHasChanged)();
}

// Shows the quest's description, prefixed by its planet name when it has one, and remembers the row
// as read. Called with a null control when the list comes up empty.
void CSWGuiInGameJournal::OnControlEntered(CSWGuiControl* pControl)
{
    if (pControl)
    {
        int nIndex = static_cast<CSWGuiJournalItemEntry*>(pControl)->m_nEntryIndex;
        m_lbDescription.ClearItems();

        SJournalEntry* pEntry = GetQuestEntryAt(nIndex);
        if (pEntry)
        {
            m_aNewQuestRows.AddUnique(nIndex);

            CExoString sText;
            pEntry->m_locText.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                        &sText, 0);
            int nPlanetId = pEntry->m_nPlanetId;
            if (nPlanetId != -1)
            {
                C2DA* p2DA = g_pRules->m_p2DArrays->m_pPlanetary;
                int nStrRef;
                p2DA->GetINTEntry(nPlanetId, "Name", &nStrRef);
                sText = g_pTlkTable->GetSimpleString(nStrRef) + CExoString(":\n") + sText;
            }
#if !KOTOR1  // K1 has no token expansion
            if (sText.Find('<', 0) != -1 || sText.Find('{', 0) != -1)
            {
                // inlined CSWTlkTable::ParseStr(CExoString&, CGameObject*, int, int)
                g_pSWTlkTable->m_pSpeakerObj =
                    g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
                g_pSWTlkTable->m_gender = g_pSWTlkTable->GetObjectGender();
                g_pSWTlkTable->m_bSuppressEnabled |= 1;
                g_pSWTlkTable->ParseStr(sText);
            }
#endif
            m_lblDescription.m_text.m_params.SetText(sText);
        }
    }

    // Re-fits the description row to its new text.
    CSWGuiExtent extent = m_lblDescription.m_extent;
    int nHeight;
    CSWGuiText* pText = &m_lblDescription.m_text;
#ifdef K2_QOL_IMPROVEMENTS
    K2QolFitText(*pText, 0x7fffffff);   // LBL_ITEM_DESCRIPTION scrolls its one row: no limit
#endif
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (pText->m_params.m_sText.GetLength() >= 1)
#else
    if ((pText->m_params.m_sText.m_sString
             ? static_cast<int>(strlen(pText->m_params.m_sText.m_sString)) : 0) >= 1)
#endif
        nHeight = pText->GetIdealHeight();
    else
        nHeight = pText->GetFontHeight();
    extent.h = nHeight;
#if !KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 does not text-scale
    extent.h = static_cast<int>(GetResolutionTextScalar() * extent.h);
#endif
    m_lblDescription.SetExtent(extent);

    CSWGuiControl* pRow = &m_lblDescription;
    m_lbDescription.AddControls(&pRow, 1, 0, 0, 0);
    m_lbDescription.SetSelectedControl(&m_lblDescription, 0);
}

// The quest screen's key and pad handler; it does not chain to the base while the messages panel is
// up.
#if KOTOR1  // K1 steps all four sort keys and relabels BTN_SORT and the title, opens the
             // quest-items screen, swaps the page and scrolls the description ten lines
void CSWGuiInGameJournal::HandleInputEvent(int nType, int nValue)
{
    if (nValue)
    {
        CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
        switch (nType)
        {
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_START:
            case INPUTEVENT_GUI_BACK:
            case INPUTEVENT_GUI:
                if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
                    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
                break;

            case INPUTEVENT_GUI_A:
            case INPUTEVENT_GUI_BLACK:
            {
                int (CSWCJournal::*pfnGetSortBy)() =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
                        ? &CSWCJournal::GetDoneSortBy
                        : &CSWCJournal::GetActiveSortBy;
                int nSortType = (pJournal->*pfnGetSortBy)() + 1;
                if (nSortType > JOURNAL_SORT_BY_PLANET)
                    nSortType = JOURNAL_SORT_BY_DATE;
                g_nJournalSortType = nSortType;

                void (CSWCJournal::*pfnSetSortBy)(int) =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
                        ? &CSWCJournal::SetDoneSortBy
                        : &CSWCJournal::SetActiveSortBy;
                (pJournal->*pfnSetSortBy)(g_nJournalSortType);

                int nNextSort = g_nJournalSortType + 1;
                if (nNextSort > JOURNAL_SORT_BY_PLANET)
                    nNextSort = JOURNAL_SORT_BY_DATE;
                CExoString sTitle = g_pTlkTable->GetSimpleString(STRREF_SORT) + CExoString(" ")
                                  + g_pTlkTable->GetSimpleString(s_pnSortLabels[nNextSort]);
                m_btnSort.m_text.m_params.SetText(sTitle);
                IosAddButtonIcon(&m_btnSort, 'a');

                sTitle = g_pTlkTable->GetSimpleString(
                             g_pAppManager->m_pClientExoApp->GetInGameGui()
                                     ->m_journalFlags.bShowDoneQuests ? STRREF_COMPLETED_QUESTS : STRREF_ACTIVE_QUESTS)
                       + CExoString(" - ")
                       + g_pTlkTable->GetSimpleString(s_pnSortLabels[g_nJournalSortType]);
                m_lblTitle.m_text.m_params.SetText(sTitle);
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                break;
            }

            case INPUTEVENT_GUI_X:
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                m_pGuiManager->AddPanel(m_pQuestItem, 3, 1);
                break;

            case INPUTEVENT_GUI_Y:
            {
                MarkNewQuestsObserved();
                int (CSWCJournal::*pfnGetSortBy)() =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
                        ? &CSWCJournal::GetDoneSortBy
                        : &CSWCJournal::GetActiveSortBy;
                g_nJournalSortType = (pJournal->*pfnGetSortBy)();
                CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
                pGui->m_journalFlags.bShowDoneQuests =
                    !g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests;
                PopulateItemListBox();

                CExoString sTitle;
                if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests)
                {
                    m_btnSwapText.m_text.m_params.SetStrRef(STRREF_ACTIVE_QUESTS);
                    sTitle = g_pTlkTable->GetSimpleString(STRREF_COMPLETED_QUESTS) + CExoString(" - ")
                           + g_pTlkTable->GetSimpleString(s_pnSortLabels[g_nJournalSortType]);
                    m_lblTitle.m_text.m_params.SetText(sTitle);
                }
                else
                {
                    m_btnSwapText.m_text.m_params.SetStrRef(STRREF_COMPLETED_QUESTS);
                    sTitle = g_pTlkTable->GetSimpleString(STRREF_ACTIVE_QUESTS) + CExoString(" - ")
                           + g_pTlkTable->GetSimpleString(s_pnSortLabels[g_nJournalSortType]);
                    m_lblTitle.m_text.m_params.SetText(sTitle);
                }
                IosAddButtonIcon(&m_btnSwapText, 'y');
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                break;
            }

            case INPUTEVENT_GUI_DPAD_LEFT:
                for (int i = 0; i < 10; i++)
                    m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
            case INPUTEVENT_GUI_DPAD_RIGHT:
                for (int i = 0; i < 10; i++)
                    m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;

            case INPUTEVENT_GUI_SCROLL_UP:
                m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
            case INPUTEVENT_GUI_SCROLL_DOWN:
                m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;
        }
    }
    CSWGuiPanel::HandleInputEvent(nType, nValue);
}
#else
void CSWGuiInGameJournal::HandleInputEvent(int nType, int nValue)
{
    if (nValue)
    {
        if (m_pGuiManager->PanelExists(m_pMessages))
            return;

        CSWCJournal* pJournal = g_pAppManager->m_pClientExoApp->GetQuestJournal();
        switch (nType)
        {
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_START:
            case INPUTEVENT_GUI_BACK:
            case INPUTEVENT_GUI:
                if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
                    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
                break;

            // Swaps between the active and completed pages, each of which keeps its own sort key.
            case INPUTEVENT_GUI_A:
            {
                MarkNewQuestsObserved();
                int bDone = g_pAppManager->m_pClientExoApp->GetInGameGui()
                                ->m_journalFlags.bShowDoneQuests;
                int nSortBy = bDone ? pJournal->m_done.m_nSortBy : pJournal->m_active.m_nSortBy;
                CExoString sUnused;  // sic: constructed and destroyed, never read
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests =
                    !bDone;
                if (bDone)
                {
                    pJournal->SetActiveSortBy(nSortBy);
                    m_btnSwapText.m_text.m_params.SetStrRef(STRREF_ACTIVE_QUESTS_BY);
                    AspyrAddButtonIcon(&m_btnSwapText, 'a');
                }
                else
                {
                    pJournal->SetDoneSortBy(nSortBy);
                    m_btnSwapText.m_text.m_params.SetStrRef(STRREF_COMPLETED_QUESTS_BY);
                    AspyrAddButtonIcon(&m_btnSwapText, 'a');
                }
                PopulateItemListBox();
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                break;
            }

            case INPUTEVENT_GUI_X:
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                m_pGuiManager->AddPanel(m_pMessages, 2, 1);
                m_pGuiManager->SendPanelToBack(m_pMessages);
                m_pGuiManager->SendPanelToBack(this);
                break;

            case INPUTEVENT_GUI_Y:
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
                break;

            case INPUTEVENT_GUI_DPAD_LEFT:
            {
                int (CSWCJournal::*pfnGetSortBy)() =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
                        ? &CSWCJournal::GetDoneSortBy
                        : &CSWCJournal::GetActiveSortBy;
                int nSortType = (pJournal->*pfnGetSortBy)() - 1;
                if (nSortType == JOURNAL_SORT_BY_PRIORITY)
                    nSortType = JOURNAL_SORT_BY_NAME;
                if (nSortType < 0)
                    nSortType = JOURNAL_SORT_BY_PLANET;
                g_nJournalSortType = nSortType;

                void (CSWCJournal::*pfnSetSortBy)(int) =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
                        ? &CSWCJournal::SetDoneSortBy
                        : &CSWCJournal::SetActiveSortBy;
                (pJournal->*pfnSetSortBy)(g_nJournalSortType);
#if !KOTOR1  // K1 relabels the sort button here
                UpdateFilterLabelHighlights();
#endif
                PopulateItemListBox();
                break;
            }

            case INPUTEVENT_GUI_DPAD_RIGHT:
            {
                int (CSWCJournal::*pfnGetSortBy)() =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
                        ? &CSWCJournal::GetDoneSortBy
                        : &CSWCJournal::GetActiveSortBy;
                int nSortType = (pJournal->*pfnGetSortBy)() + 1;
                if (nSortType == JOURNAL_SORT_BY_PRIORITY)
                    nSortType = JOURNAL_SORT_BY_PLANET;
                if (nSortType > JOURNAL_SORT_BY_PLANET)
                    nSortType = JOURNAL_SORT_BY_DATE;
                g_nJournalSortType = nSortType;

                void (CSWCJournal::*pfnSetSortBy)(int) =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests
                        ? &CSWCJournal::SetDoneSortBy
                        : &CSWCJournal::SetActiveSortBy;
                (pJournal->*pfnSetSortBy)(g_nJournalSortType);
#if !KOTOR1  // K1 relabels the sort button here
                UpdateFilterLabelHighlights();
#endif
                PopulateItemListBox();
                break;
            }

            // Scrolls the description box by one line.
            case INPUTEVENT_GUI_SCROLL_UP:
                m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
            case INPUTEVENT_GUI_SCROLL_DOWN:
                m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;
        }
    }
    CSWGuiPanel::HandleInputEvent(nType, nValue);
}
#endif

// Builds the quest screen: the chained messages panel, every widget bound to its layout entry, then
// the run-time state (filter captions, the prototype row's colours and the button bindings).
#if KOTOR1  // K1 binds the quest-items screen as the chained sub-panel instead, has no filter
             // buttons and does not text-scale the description row
CSWGuiInGameJournal::CSWGuiInGameJournal(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager), m_pHoveredEntry(0)
{
    m_pQuestItem = new CSWGuiQuestItem(m_pGuiManager);

    StartLoadFromLayout(CResRef("journal"));
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1);
    InitControl(m_lbItems, CExoString("LB_ITEMS"), 1);
    InitControl(m_lbDescription, CExoString("LBL_ITEM_DESCRIPTION"), 1);
    InitControl(m_btnQuestItems, CExoString("BTN_QUESTITEMS"), 1);
    InitControl(m_btnSwapText, CExoString("BTN_SWAPTEXT"), 1);
    InitControl(m_btnSort, CExoString("BTN_SORT"), 1);
    InitControl(m_btnExit, CExoString("BTN_EXIT"), 1);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    CSWGuiLabel* pProtoDesc = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
    m_lblDescription.Initialize(pProtoDesc->m_extent, pProtoDesc, 1.0f);

    CSWGuiButton* pProtoRow = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    pProtoRow->m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    pProtoRow->m_borderHilight.m_params.m_color =
        *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    pProtoRow->m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    m_lbItems.bShowScrollBar = 1;
    m_btnQuestItems.m_tail.bFocusOnHover = 0;
    m_btnQuestItems.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                             &CSWGuiPanel::OnXButtonPressed));
    m_btnSwapText.m_tail.bFocusOnHover = 0;
    m_btnSwapText.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                           &CSWGuiPanel::OnYButtonPressed));
    m_btnSort.m_tail.bFocusOnHover = 0;
    m_btnSort.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnBlackButtonPressed));
    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnBButtonPressed));
    m_lbItems.bSelectOnLeftClick = 1;

    SetActiveControl(&m_lbItems, 0);

    // K1 cycles all four sort keys (priority included).
    int nNextSort = g_nJournalSortType + 1;
    if (nNextSort > JOURNAL_SORT_BY_PLANET)
        nNextSort = JOURNAL_SORT_BY_DATE;
    CExoString sTitle = g_pTlkTable->GetSimpleString(STRREF_SORT) + CExoString(" ")
                      + g_pTlkTable->GetSimpleString(s_pnSortLabels[nNextSort]);
    m_btnSort.m_text.m_params.SetText(sTitle);
    IosAddButtonIcon(&m_btnSort, 'a');

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests)
    {
        m_btnSwapText.m_text.m_params.SetStrRef(STRREF_ACTIVE_QUESTS);
        sTitle = g_pTlkTable->GetSimpleString(STRREF_COMPLETED_QUESTS) + CExoString(" - ")
               + g_pTlkTable->GetSimpleString(s_pnSortLabels[g_nJournalSortType]);
        m_lblTitle.m_text.m_params.SetText(sTitle);
    }
    else
    {
        m_btnSwapText.m_text.m_params.SetStrRef(STRREF_COMPLETED_QUESTS);
        sTitle = g_pTlkTable->GetSimpleString(STRREF_ACTIVE_QUESTS) + CExoString(" - ")
               + g_pTlkTable->GetSimpleString(s_pnSortLabels[g_nJournalSortType]);
        m_lblTitle.m_text.m_params.SetText(sTitle);
    }
    m_lblDescription.m_text.m_params.SetText(CExoString(""));

    IosAddButtonIcon(&m_btnQuestItems, 'x');
    IosAddButtonIcon(&m_btnSwapText, 'y');
    IosAddButtonIcon(&m_btnSort, 'a');
    IosAddButtonIcon(&m_btnExit, 'b');
}
#else
CSWGuiInGameJournal::CSWGuiInGameJournal(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager), m_pHoveredEntry(0)
{
    m_pMessages = new CSWGuiInGameMessages(m_pGuiManager);
#if !KOTOR1  // K1 has no Aspyr "bg_replacement" border panel
    m_pMessages->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif

    StartLoadFromLayout(CResRef("journal_p"), 1, 1, 1);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_lbItems, CExoString("LB_ITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lbDescription, CExoString("LBL_ITEM_DESCRIPTION"), 1, 1, 0, 0, 0);
    InitControl(m_btnSwapText, CExoString("BTN_SWAPTEXT"), 1, 1, 0, 0, 0);
    InitControl(m_btnExit, CExoString("BTN_EXIT"), 1, 1, 0, 0, 0);
    AspyrAddButtonIcon(&m_btnSwapText, 'a');
    AspyrAddButtonIcon(&m_btnExit, 'b');
    InitControl(m_btnFilterTime, CExoString("BTN_FILTER_TIME"), 1, 1, 0, 0, 0);
    InitControl(m_btnFilterName, CExoString("BTN_FILTER_NAME"), 1, 1, 0, 0, 0);
    InitControl(m_btnFilterPlanet, CExoString("BTN_FILTER_PLANET"), 1, 1, 0, 0, 0);
    InitControl(m_btnMessages, CExoString("BTN_MESSAGES"), 1, 1, 0, 0, 0);
    AspyrAddButtonIcon(&m_btnMessages, 'x');
    InitControl(m_aBars[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[4], CExoString("LBL_BAR5"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    {
        CExoString sCaption;
        sCaption = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_TIME);
        m_btnFilterTime.m_text.m_params.SetText(sCaption);
        sCaption = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_PLANET);
        m_btnFilterPlanet.m_text.m_params.SetText(sCaption);
    }

    // The description row is cloned from the description box's prototype item.
    CSWGuiLabel* pProtoDesc = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
    float fTextScalar = GetResolutionTextScalar();
    CSWGuiExtent extent = pProtoDesc->m_extent;
    extent.h = static_cast<int>(fTextScalar * static_cast<float>(extent.h));
    m_lblDescription.Initialize(extent, pProtoDesc, 1.0f);

    // Every quest row inherits the prototype's colours, so the "new quest" pulse is configured
    // once.
    CSWGuiButton* pProtoRow = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
#if KOTOR1
    pProtoRow->m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    pProtoRow->m_borderHilight.m_params.m_color =
        *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
#else
    pProtoRow->m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
    pProtoRow->m_borderHilight.m_params.m_color =
        *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
    pProtoRow->m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    m_lbItems.bShowScrollBar = 1;
    m_btnSwapText.m_tail.bFocusOnHover = 0;
    m_btnSwapText.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                           &CSWGuiPanel::OnAButtonPressed));
    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnBButtonPressed));

    m_lbItems.bSelectOnLeftClick = 1;
    // The filter buttons carry their sort key in the control's user data, which is why one handler
    // serves all three.
#if !KOTOR1  // K1 has no filter buttons
    void (CSWGuiObject::*pfnFilter)(CSWGuiControl*) =
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameJournal::OnFilterPressed);
    m_btnFilterTime.m_tail.bFocusOnHover = 0;
    m_btnFilterTime.m_tail.m_nUserData = JOURNAL_SORT_BY_DATE;
    m_btnFilterTime.AddEvent(INPUTEVENT_GUI_A, this, pfnFilter);
    m_btnFilterName.m_tail.bFocusOnHover = 0;
    m_btnFilterName.m_tail.m_nUserData = JOURNAL_SORT_BY_NAME;
    m_btnFilterName.AddEvent(INPUTEVENT_GUI_A, this, pfnFilter);
    m_btnFilterPlanet.m_tail.bFocusOnHover = 0;
    m_btnFilterPlanet.m_tail.m_nUserData = JOURNAL_SORT_BY_PLANET;
    m_btnFilterPlanet.AddEvent(INPUTEVENT_GUI_A, this, pfnFilter);
#endif
    m_btnMessages.m_tail.bFocusOnHover = 0;
    m_btnMessages.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                           &CSWGuiPanel::OnXButtonPressed));

    SetActiveControl(&m_lbItems, 0);

    int nNextSort = GetNextSortType();
#if !KOTOR1
    UpdateFilterLabelHighlights();
#endif
    // sic: built and discarded; nothing reads the result.
    CExoString sSortTitle = g_pTlkTable->GetSimpleString(STRREF_SORT) + CExoString(" ")
                          + g_pTlkTable->GetSimpleString(s_pnSortLabels[nNextSort]);

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_journalFlags.bShowDoneQuests)
    {
        m_btnSwapText.m_text.m_params.SetStrRef(STRREF_COMPLETED_QUESTS_BY);
        AspyrAddButtonIcon(&m_btnSwapText, 'a');
    }
    else
    {
        m_btnSwapText.m_text.m_params.SetStrRef(STRREF_ACTIVE_QUESTS_BY);
        AspyrAddButtonIcon(&m_btnSwapText, 'a');
    }
    m_lblDescription.m_text.m_params.SetText(CExoString(""));
}
#endif
