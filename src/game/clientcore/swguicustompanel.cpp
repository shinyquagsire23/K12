#include "k2stdint.h"
#include "port/sdlapi.h"
#include "swguicustompanel.h"
#include "swguimainchargen.h"
#include "swguiingame.h"
#include "swguiabilitieschargen.h"
#include "swguiskillschargen.h"
#include "swguifeatschargen.h"
#include "swguinamechargen.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "swguiportraitchargen.h"
#include "guiingame.h"
#include "clientexoapp.h"
#include "appmanager.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwcanimbase.h"
#include "AURORA/aurmodel.h"
#include "exoresman.h"
#include "exoinput.h"
#include "strref.h"

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
#include "port/k1gui.h"
#endif

// Builds the six-row widget wall: one loop makes the three parallel arrays (highlight plate,
// caption label, step button) off the shared char-gen layout, and the two trailing named buttons
// are bound separately.
//
// The loop stops on m_aLabels[6], which aliases m_aButtons[0], so the test is `i != 6`.
CSWGuiCustomPanel::CSWGuiCustomPanel(CSWGuiManager* pGuiManager, CSWCCreature* pCreature)
    : CSWGuiPanel(pGuiManager)
{
    CExoString s;

    m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    m_pCreature = pCreature;

#if KOTOR1
    // K1 loads the plain layout plus its background label, and the hilights keep their fill style.
    StartLoadFromLayout(CResRef("CUSTPNL"));
    InitControl(m_lblBg, CExoString("LBL_BG"), 1);
#else
    StartLoadFromLayout(CResRef("CUSTPNL_p"), 0, 1, 1);
#endif

    for (int i = 0; i < 6; i++)
    {
        s.Format(const_cast<char*>("LBL_%d"), i + 1);
        InitControl(m_aHilights[i], s, 1, 1, 0, 0, 0);
        m_aHilights[i].m_hilightBorder.m_params.SetPulsingAlpha(1, 0, 1);
#if !KOTOR1
        m_aHilights[i].m_border.m_params.m_flags.nFillStyle = 2;
        m_aHilights[i].m_hilightBorder.m_params.m_flags.nFillStyle = 2;
#endif

        s.Format(const_cast<char*>("BTN_STEPNAME%d"), i + 1);
        InitControl(m_aButtons[i], s, 1, 1, 0, 0, 0);

        s.Format(const_cast<char*>("LBL_NUM%d"), i + 1);
        InitControl(m_aLabels[i], s, 1, 1, 0, 0, 0);
    }

    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();
#if KOTOR1
    ScaleExtentForResolution();
    // K1: centre the 4:3 page horizontally on a wider screen.
    CSWGuiExtent extent = m_extent;
    extent.x += (g_SDL_mode.w - static_cast<int>(g_SDL_mode.h / 768.0f * 1024.0f)) / 2;
    SetExtent(extent);
#endif

    m_aButtons[0].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectPortraitButton));
    m_aButtons[1].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectAbilitiesButton));
    m_aButtons[2].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectSkillsButton));
    m_aButtons[3].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectFeatsButton));
    m_aButtons[4].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectNameButton));
    m_aButtons[5].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectPlayButton));

    m_aButtons[0].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectPortraitButton));
    m_aButtons[1].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectAbilitiesButton));
    m_aButtons[2].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectSkillsButton));
    m_aButtons[3].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectFeatsButton));
    m_aButtons[4].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectNameButton));
    m_aButtons[5].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnSelectPlayButton));

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::OnCancelPressed));

    SetState(0);

    AspyrAddButtonIcon(&m_btnBack, 'b');
#if KOTOR1  // K1: the cancel button shows the B glyph too
    AspyrAddButtonIcon(&m_btnCancel, 'b');
#else
    AspyrAddButtonIcon(&m_btnCancel, 'x');
#endif
}

// Selects a row and paints the whole family. nState is both the new row index, which also picks
// that row's button, and the mode: the commit button is enabled and green in any non-zero state
// and greyed in state 0; only the current row's highlight plate is activated, and its label,
// button and button border take the highlight colour. Rows after the current one are painted
// live-text/green and rows before it disabled-text/green, with every non-current row disabled.
void CSWGuiCustomPanel::SetState(int nState)
{
#if KOTOR1
    // K1 paints the commit button and the current step with the global blue/yellow pair; earlier
    // and later steps keep their colours and are only switched off.
    m_nCurrentRow = nState;

    if (nState != 0)
    {
        m_btnBack.SetEnabled(1);
        m_btnBack.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
    }
    else
    {
        m_btnBack.SetEnabled(0);
        m_btnBack.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
    }

    for (int i = 0; i < 6; i++)
    {
        if (i == m_nCurrentRow)
        {
            m_aHilights[i].SetActive(1);
            m_aLabels[i].SetEnabled(1);
            m_aButtons[i].SetEnabled(1);
            m_aLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
            m_aButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
            m_aLabels[i].m_text.m_params.bHilight = 1;
            m_aButtons[i].m_text.m_params.bHilight = 1;
        }
        else
        {
            m_aHilights[i].SetActive(0);
            m_aLabels[i].m_text.m_params.bHilight = 0;
            m_aButtons[i].m_text.m_params.bHilight = 0;
            m_aLabels[i].SetEnabled(0);
            m_aButtons[i].SetEnabled(0);
        }
    }

    SetActiveControl(&m_aButtons[m_nCurrentRow], 0);
#else
    m_nCurrentRow = nState;

    if (nState != 0)
    {
        m_btnBack.SetEnabled(1);
        m_btnBack.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnBack.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
    }
    else
    {
        m_btnBack.SetEnabled(0);
        m_btnBack.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnBack.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_TEXT);
    }

    for (int i = 0; i < 6; i++)
    {
        if (i == m_nCurrentRow)
        {
            m_aHilights[i].SetActive(1);
            m_aLabels[i].SetEnabled(1);
            m_aButtons[i].SetEnabled(1);
            m_aLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
            m_aButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
            m_aButtons[i].m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
            m_aLabels[i].m_text.m_params.bHilight = 1;
            m_aButtons[i].m_text.m_params.bHilight = 1;
        }
        else
        {
            m_aHilights[i].SetActive(0);
            m_aLabels[i].m_text.m_params.bHilight = 0;
            m_aButtons[i].m_text.m_params.bHilight = 0;
            if (i < m_nCurrentRow)
            {
                m_aLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
                m_aButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
                m_aButtons[i].m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
            }
            else
            {
                m_aLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
                m_aButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
                m_aButtons[i].m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_TEXT);
            }
            m_aLabels[i].SetEnabled(0);
            m_aButtons[i].SetEnabled(0);
        }
    }

    SetActiveControl(&m_aButtons[m_nCurrentRow], 0);
#endif
}

// Opens the portrait page, the first row of the family. Its tail is unique to this opener: it
// puts the creature's two sub-models into the idle pose before the panel is pushed.
void CSWGuiCustomPanel::OnSelectPortraitButton(CSWGuiControl* pControl)
{
    if (m_nCurrentRow >= 0 && pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiPortraitCharGen* pPanel = new CSWGuiPortraitCharGen(m_pGuiManager, m_pCreature);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel->SetParentPanel(this, 2);

        if (m_pCreature->GetAnimationBase() != 0)
        {
            m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY)
                ->PlayAnimation(const_cast<char*>("pause2"), 1.0f, 0, 0.0f);
            m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD)
                ->PlayAnimation(const_cast<char*>("pause2"), 1.0f, 0, 0.0f);
        }

        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

// Opens the abilities page. Every opener in the family is the same five statements: guard, `new`
// the page with a zero mode, give it the shared border name "bg_replacement", link the new page's
// parent back to this panel, and push it as a state-3 modal panel.
//
// The guard's row number is the button's own row (abilities 1, skills 2, feats 3, name 4, play 5)
// and the second test is the event-arg latch, as in OnSelectPlayButton.
void CSWGuiCustomPanel::OnSelectAbilitiesButton(CSWGuiControl* pControl)
{
    if (m_nCurrentRow >= 1 && pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiAbilitiesCharGen* pPanel = new CSWGuiAbilitiesCharGen(m_pGuiManager, m_pCreature, 0);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel->m_pParentPanel = this;
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiCustomPanel::OnSelectSkillsButton(CSWGuiControl* pControl)
{
    if (m_nCurrentRow >= 2 && pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiSkillsCharGen* pPanel = new CSWGuiSkillsCharGen(m_pGuiManager, m_pCreature, 0);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel->m_pParentPanel = this;
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

// Opens the feats page: the same shape as the two pages above except for the guard's row (3) and
// a fourth constructor argument, both trailing arguments being 0 here.
void CSWGuiCustomPanel::OnSelectFeatsButton(CSWGuiControl* pControl)
{
    if (m_nCurrentRow >= 3 && pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiFeatsCharGen* pPanel = new CSWGuiFeatsCharGen(m_pGuiManager, m_pCreature, 0, 0);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel->m_pParentPanel = this;
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

// Opens the name page, the odd one out of the family: no extra border panel, a two-argument
// constructor, and a modal flag of 1 instead of 3. The parent link goes through
// CSWGuiNameChargen::SetParentPanel, whose second argument is stored as a flag.
void CSWGuiCustomPanel::OnSelectNameButton(CSWGuiControl* pControl)
{
    if (m_nCurrentRow >= 4 && pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiNameChargen* pPanel = new CSWGuiNameChargen(m_pGuiManager, m_pCreature);
        pPanel->SetParentPanel(this, 2);
#if KOTOR1
        // K1 opens the Chromebook soft keyboard before pushing the page with modal flag 3.
        if (SDL_IsChromebook())
            SDL_StartTextInput();
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pPanel), 3, 1);  // idiom-exempt: same flat-model cast
#else
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pPanel), 1, 1);  // idiom-exempt: same flat-model cast
#endif
    }
}

// Starts play. Both guards are needed: the row index must be at the last row (5) and the button
// must carry a pending selection. The parent is not null-checked here, unlike the clear and cancel
// paths.
void CSWGuiCustomPanel::OnSelectPlayButton(CSWGuiControl* pControl)
{
    if (m_nCurrentRow >= 5 && pControl->m_tail.nCurrentEventArg != 0)
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->BeginPlayGame();
}

// Backs out. On any row but the first, raises the shared "are you sure?" box, with Cancel allowed
// and ConfirmCancel as its callback, and pushes it as a modal panel; on the first row there is
// nothing left to back out of, so CancelPanel runs instead.
void CSWGuiCustomPanel::OnCancelPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (m_nCurrentRow != 0)
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(1);
        pBox->SetMessage(static_cast<uint64_t>(STRREF_CONFIRM_CANCEL_CHARACTER));
        // `this` is an implicit derived-to-base conversion; the member pointer goes the other way
        // and needs the explicit conversion.
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCustomPanel::ConfirmCancel), 0);
        m_pGuiManager->AddPanel(pBox, 1, 1);
    }
    else
    {
        CancelPanel();
    }
}

// A non-zero value gates the whole switch, which is on the key code: the back keys step the row
// index back one and re-apply the state, at row 0 backing out entirely; the confirm key raises the
// confirm-cancel box; the click key plays a sound only; the rest of the range is unhandled. Every
// exit, handled or not, forwards to the base panel. OnCancelPressed takes no argument on any path,
// so its local control pointer is left uninitialised.
void CSWGuiCustomPanel::HandleInputEvent(int type, int val)
{
#if KOTOR1
    // K1 handles back and the click sound only: no feats step and no cancel-all binding.
    if (val != 0)
    {
        if (type == INPUTEVENT_GUI_START)
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        }
        else if (type == INPUTEVENT_GUI_BACK || type == INPUTEVENT_GUI_B)
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (m_nCurrentRow == 0)
            {
                CancelPanel();
            }
            else
            {
                m_nCurrentRow = m_nCurrentRow - 1;
                SetState(m_nCurrentRow);
                if (m_nCurrentRow == 1)
                    ClearAbilities();
            }
        }
    }
#else
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (m_nCurrentRow == 0)
            {
                CancelPanel();
            }
            else
            {
                m_nCurrentRow = m_nCurrentRow - 1;
                SetState(m_nCurrentRow);
                if (m_nCurrentRow == 1)
                    ClearAbilities();
                else if (m_nCurrentRow == 2)
                    ClearFeats();
            }
            break;

        case INPUTEVENT_GUI_X:
        {
#ifdef K2_FIX_ENGINE_BUGS
            CSWGuiControl* pControl = nullptr;
#else
            CSWGuiControl* pControl;  // left uninitialised: the cancel path carries no control
#endif
            OnCancelPressed(pControl);
            break;
        }

        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;

        default:
            break;
        }
    }

#endif

    // Qualified so that it stays a direct call rather than a dispatch on the override below.
    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiCustomPanel::SetParentPanel(CSWGuiPanel* pParentPanel)
{
    m_pParentPanel = pParentPanel;
}

// Forwards to the owning char-gen screen. The field is declared CSWGuiPanel*, but the callee is a
// CSWGuiMainCharGen member, so the body names the real class.
void CSWGuiCustomPanel::PlayGame()
{
    static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->BeginPlayGame();
}

// Returns the owning char-gen screen's name string, through the same downcast as PlayGame.
CExoString CSWGuiCustomPanel::GetCurrentName()
{
    return static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->m_sName;
}

// The same forward as above, null-guarded.
void CSWGuiCustomPanel::ClearFeats()
{
    if (m_pParentPanel != 0)
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->ClearFeats();
}

// The skill page is done.
void CSWGuiCustomPanel::SkillCreationCompleted()
{
    SetState(3);
}

// The feat page is done, which is the next page.
void CSWGuiCustomPanel::FeatCreationCompleted()
{
    SetState(4);
}

// Backs out of the custom page: shows the screen the flow came from, blanks the ability page's
// effect on the character, and re-picks the live page. The two forwards are guarded separately,
// and the second guard is needed: SetVisiblePanel can change the parent, so it is re-read and
// re-tested afterwards.
void CSWGuiCustomPanel::CancelPanel()
{
    if (m_pParentPanel != 0)
    {
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->SetVisiblePanel(CSWGuiMainCharGen::VISIBLE_PANEL_QUICK_OR_CUSTOM);

        if (m_pParentPanel != 0)
            static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->ClearAbilities();

        m_pCreature->m_pStats->m_nAbilityPointsRemaining = 30;
    }

    SetState(0);
}

// Blanks the ability page and then stops the stats object from re-seeding itself; the cancel body
// without the panel swap or the state change.
void CSWGuiCustomPanel::ClearAbilities()
{
    if (m_pParentPanel != 0)
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->ClearAbilities();

    m_pCreature->m_pStats->m_nAbilityPointsRemaining = 30;
}

// The callback the shared "are you sure?" box dismisses with. The guard is the box's accepted
// result bit, not this panel's state, so a confirmed cancel commits and a dismissal that was not
// accepted returns with no state change at all.
void CSWGuiCustomPanel::ConfirmCancel(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
    {
        if (m_pParentPanel != 0)
        {
            static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->SetVisiblePanel(CSWGuiMainCharGen::VISIBLE_PANEL_QUICK_OR_CUSTOM);

            if (m_pParentPanel != 0)
                static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->ClearAbilities();

            m_pCreature->m_pStats->m_nAbilityPointsRemaining = 30;
        }

        SetState(0);
    }
}

// A child page was backed out of: repaints the row it was on and sets its "new" flag. The row
// index is re-read after the colour write, which can change this panel's state.
void CSWGuiCustomPanel::CancelledByChild()
{
#if KOTOR1
    m_aButtons[m_nCurrentRow].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
#else
    m_aButtons[m_nCurrentRow].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
    m_aButtons[m_nCurrentRow].m_text.m_params.bHilight = 1;
}

// Commits the name the player typed: copies it into the owning screen and advances to the last
// row.
void CSWGuiCustomPanel::NameCreationCompleted()
{
    static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->NameChanged();
    SetState(5);
}

// Once the portrait page is done, refreshes the owning screen's portrait and returns to row 1.
void CSWGuiCustomPanel::PortraitCreationCompleted()
{
    static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->PortraitChanged();
    SetState(1);
}

// Tells the owning char-gen screen the ability grid changed, then advances to the skill page.
void CSWGuiCustomPanel::AbilityCreationCompleted()
{
    static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->AbilitiesChanged();
    SetState(2);
}

// The destructor is left implicit: there is nothing to define, and declaring one here would move
// the vtable into this translation unit.
