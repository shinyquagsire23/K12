// CSWGuiInGameAbilities: the character sheet's ability panel, with the skill/feat/power rows it
// owns in the same file.

#include "k2stdint.h"
#include "swguiingameabilities.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swparty.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwclass.h"
#include "nwspell.h"
#include "nwsarea.h"
#include "nwsobject.h"
#include "exotlktable.h"
#include "nwskill.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "swguiskillflowchart.h"
#include "nwsrules.h"
#include "exolocstringinternal.h"
#include "exostring.h"
#include "Vector.h"
#include "CExoArrayListImpl.h"
#include "exoresman.h"
#if KOTOR1
#include "serverexoapp.h"
#include "serverinfo.h"
#include "port/k1gui.h"
#include "partytables.h"
#endif

#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "nwscriptconst.h"
#include "exoinput.h"
#include "tutorial.h"
#include "strref.h"
#include "exostringinline.h"

// Controller-glyph helpers, defined elsewhere in the port.
void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
void AspyrAddLabelIcon(CSWGuiLabel* pLabel, char nIcon);

float GetResolutionTextScalar();

#if KOTOR1
// K1 loads the "abilities" layout: portrait, the four party buttons and the mode-button glyphs,
// with no filter bar or separate feat list.
CSWGuiInGameAbilities::CSWGuiInGameAbilities(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sUnused;
    CSWGuiLabelHilight* pProtoRow;

    m_nNPC = -1;
    StartLoadFromLayout(CResRef("abilities"));

    InitControl(m_lblPortrait,     CExoString("LBL_PORTRAIT"),    1);
    InitControl(m_lbDesc,          CExoString("LB_DESC"),         1);
    m_lbDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lbAbility,       CExoString("LB_ABILITY"),      1);
    InitControl(m_labelBonus,      CExoString("LBL_BONUS"),       1);
    InitControl(m_labelBonusVal,   CExoString("LBL_BONUSVAL"),    1);
    InitControl(m_labelSkillRank,  CExoString("LBL_SKILLRANK"),   1);
    InitControl(m_labelRankVal,    CExoString("LBL_RANKVAL"),     1);
    InitControl(m_labelTotal,      CExoString("LBL_TOTAL"),       1);
    InitControl(m_labelTotalVal,   CExoString("LBL_TOTALVAL"),    1);
    InitControl(m_labelName,       CExoString("LBL_NAME"),        1);
    InitControl(m_labelInfoBg,     CExoString("LBL_INFOBG"),      1);
    InitControl(m_btnFeats,        CExoString("BTN_FEATS"),       1);
    InitControl(m_btnPowers,       CExoString("BTN_POWERS"),      1);
    InitControl(m_btnSkills,       CExoString("BTN_SKILLS"),      1);
    InitControl(m_btnExit,         CExoString("BTN_EXIT"),        1);
    InitControl(m_btnChange1,      CExoString("BTN_CHANGE1"),     1);
    InitControl(m_btnChange2,      CExoString("BTN_CHANGE2"),     1);
    InitControl(m_btnCharLeft,     CExoString("BTN_CHARLEFT"),    1);
    InitControl(m_btnCharRight,    CExoString("BTN_CHARRIGHT"),   1);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_btnCharLeft.m_tail.bFocusOnHover = 0;
    m_btnCharLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                           &CSWGuiInGameAbilities::OnSwitchLeft));
    m_btnCharRight.m_tail.bFocusOnHover = 0;
    m_btnCharRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                            &CSWGuiInGameAbilities::OnSwitchRight));
    m_btnCharRight.m_border.m_params.m_fRotate = 180.0f;
    m_btnCharRight.m_borderHilight.m_params.m_fRotate = 180.0f;

    pProtoRow = static_cast<CSWGuiLabelHilight*>(m_lbAbility.m_pProtoItem);
    pProtoRow->m_hilightBorder.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    pProtoRow->m_hilightBorder.m_params.SetPulsingAlpha(1, 1, 0);
    int nWidth = m_lbAbility.m_innerExtent.w - m_lbAbility.m_nPadding * 2;
    for (int i = 0; i < 10; i++) {
        m_aSkillEntry[i].Initialize(pProtoRow->m_text, pProtoRow->m_border,
                                    pProtoRow->m_hilightBorder, nWidth);
        m_aSkillEntry[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                           &CSWGuiInGameAbilities::OnEnterSkill));
    }

    CSWGuiControl* pDescProto = m_lbDesc.m_pProtoItem;
    m_labelDesc.Initialize(pDescProto->m_extent, static_cast<CSWGuiLabel*>(pDescProto), 1.0f);

    m_lbAbility.AddEvent(SWGUIEVENT_SELECTION_CHANGED, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameAbilities::OnAbilitySelectionChanged));
    m_lbAbility.bSelectOnLeftClick = 1;

    m_btnFeats.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameAbilities::OnFeatsButtonPressed));
    m_btnFeats.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                     &CSWGuiInGameAbilities::SetButtonColor));
    m_btnPowers.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameAbilities::OnPowersButtonPressed));
    m_btnPowers.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameAbilities::SetButtonColor));
    m_btnSkills.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameAbilities::OnSkillsButtonPressed));
    m_btnSkills.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameAbilities::SetButtonColor));
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnBButtonPressed));
    m_btnChange1.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameAbilities::OnChangeCharacter));
    m_btnChange2.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameAbilities::OnChangeCharacter));

    m_btnFeats.m_tail.bFocusOnHover = 0;
    m_btnPowers.m_tail.bFocusOnHover = 0;
    m_btnSkills.m_tail.bFocusOnHover = 0;
    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnChange1.m_tail.bFocusOnHover = 0;
    m_btnChange2.m_tail.bFocusOnHover = 0;
    m_btnChange1.m_nToolTipStrRef = STRREF_CHANGE_CHARACTER;
    m_btnChange1.m_nBoundInputEventId = INPUTEVENT_CHANGE_CHARACTER;
    m_btnChange2.m_nToolTipStrRef = STRREF_CHANGE_CHARACTER;

    m_pCreature = 0;

#ifdef K1_UI_XBOX
    // X cycles the three pages: one glyph, on the leftmost of their buttons.
    IosAddButtonIcon(&m_btnSkills, 'x');
#else
    IosAddButtonIcon(&m_btnFeats, 'x');
    IosAddButtonIcon(&m_btnPowers, 'x');
    IosAddButtonIcon(&m_btnSkills, 'x');
#endif
    IosAddButtonIcon(&m_btnExit, 'b');
#ifndef K2_UI_TOUCH
#ifdef K1_UI_XBOX
    m_iconChange.InitAndAddCorner(this, 'r', m_btnChange1.m_extent);   // RB: next party member
#else
    m_iconChange.InitAndAddCorner(this, 'y', m_btnChange1.m_extent);   // pc / xbox: a corner badge
#endif
#else
    m_iconChange.InitAndAdd(this, 'y', m_btnChange1.m_extent, -30, -30);
#endif
    CSWGuiExtent extentScroll = m_lbDesc.m_extent;
    extentScroll.x += extentScroll.w;
    extentScroll.y += extentScroll.h;
#ifndef K2_UI_TOUCH
    // pc / xbox: the description's scroll bar has arrows at its ends; the badge sits beside it.
    if (!m_lbDesc.bScrollBarOnLeft)
        extentScroll.x = m_lbDesc.m_scrollBar.m_extent.x - m_lbDesc.m_scrollBar.m_extent.w;
#endif
    m_iconScroll.InitAndAdd(this, 'a', extentScroll, -30, -30);
}
#else
CSWGuiInGameAbilities::CSWGuiInGameAbilities(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sUnused;

    // Holds the prototype row used below, declared before the layout load.
    CSWGuiLabelHilight* pProtoRow;

    StartLoadFromLayout(CResRef("abilities_p"), 1, 1, 1);

    InitControl(m_lbDesc,          CExoString("LB_DESC"),         1, 1, 0, 0, 0);
    m_lbDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lbAbility,       CExoString("LB_ABILITY"),      1, 1, 0, 0, 0);
    InitControl(m_labelBonus,      CExoString("LBL_BONUS"),       1, 1, 0, 0, 0);
    InitControl(m_labelBonusVal,   CExoString("LBL_BONUSVAL"),    1, 1, 0, 0, 0);
    InitControl(m_labelSkillRank,  CExoString("LBL_SKILLRANK"),   1, 1, 0, 0, 0);
    InitControl(m_labelRankVal,    CExoString("LBL_RANKVAL"),     1, 1, 0, 0, 0);
    InitControl(m_labelTotal,      CExoString("LBL_TOTAL"),       1, 1, 0, 0, 0);
    InitControl(m_labelTotalVal,   CExoString("LBL_TOTALVAL"),    1, 1, 0, 0, 0);
    InitControl(m_labelName,       CExoString("LBL_NAME"),        1, 1, 0, 0, 0);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkFitText(&m_labelName.m_text);   // the selected ability's name, above its description
#endif
    InitControl(m_labelInfoBg,     CExoString("LBL_INFOBG"),      1, 1, 0, 0, 0);
    InitControl(m_btnFeats,        CExoString("BTN_FEATS"),       1, 1, 0, 0, 0);
    InitControl(m_btnPowers,       CExoString("BTN_POWERS"),      1, 1, 0, 0, 0);
    InitControl(m_btnSkills,       CExoString("BTN_SKILLS"),      1, 1, 0, 0, 0);
    InitControl(m_btnExit,         CExoString("BTN_EXIT"),        1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[0],    CExoString("LBL_BAR1"),        1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1],    CExoString("LBL_BAR2"),        1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2],    CExoString("LBL_BAR3"),        1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3],    CExoString("LBL_BAR4"),        1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[4],    CExoString("LBL_BAR5"),        1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[5],    CExoString("LBL_BAR6"),        1, 1, 0, 0, 0);
    InitControl(m_labelFilter,     CExoString("LBL_FILTER"),      1, 1, 0, 0, 0);
    InitControl(m_labelAbilities,  CExoString("LBL_ABILITIES"),   1, 1, 0, 0, 0);
    InitControl(m_lbFeats,         CExoString("LB_DESC_FEATS"),   1, 1, 0, 0, 0);
    StopLoadFromLayout();

    // Clone the ten skill rows from LB_ABILITY's prototype row: it is the pulsing highlight row
    // template, and every clone takes its text, its normal border, its hilight border and the list
    // box's usable width.
    pProtoRow = static_cast<CSWGuiLabelHilight*>(m_lbAbility.m_pProtoItem);
#if KOTOR1  // K1: the prototype's highlight border is tinted COLOR_YELLOW first
    pProtoRow->m_hilightBorder.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
#endif
    pProtoRow->m_hilightBorder.m_params.SetPulsingAlpha(1, 1, 0);
    int nWidth = m_lbAbility.m_innerExtent.w - m_lbAbility.m_nPadding * 2;
    for (int i = 0; i < 10; i++) {
        m_aSkillEntry[i].Initialize(pProtoRow->m_text, pProtoRow->m_border,
                                    pProtoRow->m_hilightBorder, nWidth);
        m_aSkillEntry[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                           &CSWGuiInGameAbilities::OnEnterSkill));
    }

    // Clone the description pane from LB_DESC's prototype row; its height is re-scaled for the
    // current resolution.
    CSWGuiControl* pDescProto = m_lbDesc.m_pProtoItem;
    CSWGuiExtent extentDesc = pDescProto->m_extent;
    float fTextScalar = GetResolutionTextScalar();
    extentDesc.h = static_cast<int>(fTextScalar * static_cast<float>(extentDesc.h));
    m_labelDesc.Initialize(extentDesc, static_cast<CSWGuiLabel*>(pDescProto), 1.0f);

    m_lbAbility.AddEvent(SWGUIEVENT_SELECTION_CHANGED, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameAbilities::OnAbilitySelectionChanged));
    m_lbAbility.bSelectOnLeftClick = 1;

    // The three mode buttons swap which list the panel shows; each also re-colours on hover/drag
    // (event 1, the shared handler).
    m_btnFeats.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameAbilities::OnFeatsButtonPressed));
    m_btnFeats.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                     &CSWGuiInGameAbilities::SetButtonColor));
    m_btnPowers.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameAbilities::OnPowersButtonPressed));
    m_btnPowers.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameAbilities::SetButtonColor));
    m_btnSkills.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameAbilities::OnSkillsButtonPressed));
    m_btnSkills.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameAbilities::SetButtonColor));

    // BTN_EXIT routes to the panel's own virtual B-button handler.
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnBButtonPressed));

    m_btnFeats.m_tail.bFocusOnHover = 0;
    m_btnPowers.m_tail.bFocusOnHover = 0;
    m_btnSkills.m_tail.bFocusOnHover = 0;
    m_btnExit.m_tail.bFocusOnHover = 0;

    m_pCreature = 0;

    AspyrAddButtonIcon(&m_btnExit, 'b');
    AspyrAddLabelIcon(&m_labelFilter, 'x');
}
#endif

CSWGuiInGameAbilities::~CSWGuiInGameAbilities()
{
    m_lbDesc.ClearItems();
#if !KOTOR1
    m_lbFeats.ClearItems();
#endif
    m_lbAbility.ClearItems();
}

void CSWGuiInGameAbilities::UpdateCharacter()
{
    SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0));
}

void CSWGuiInGameAbilities::OnFeatsButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 2;
    UpdateView();
}

void CSWGuiInGameAbilities::OnPowersButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 1;
    UpdateView();
}

void CSWGuiInGameAbilities::OnSkillsButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 0;
    UpdateView();
}

void CSWGuiInGameAbilities::SetButtonColor(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    unsigned char nMode = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode;
    CSWGuiTextParams* pParams;
    if (nMode == 0)
        pParams = &m_btnSkills.m_text.m_params;
    else if (nMode == 1)
        pParams = &m_btnPowers.m_text.m_params;
    else if (nMode == 2)
        pParams = &m_btnFeats.m_text.m_params;
    else
        return;
#if KOTOR1
    pParams->SetColor(CGuiInGame::COLOR_YELLOW);
#else
    pParams->SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
}

// The description pane's text and the flag UpdateDescriptionListBox uses to decide whether the
// list box shows anything at all.

#if KOTOR1  // K1: the callers reach this through a bl
__attribute__((noinline))
#endif
void CSWGuiInGameAbilities::SetDescription(const CExoString& sDescription)
{
    m_labelDesc.m_text.m_params.SetText(sDescription);
    UpdateDescriptionListBox(sDescription.GetLength() > 0);
}

void CSWGuiInGameAbilities::OnPanelAdded()
{
#if KOTOR1  // K1 resets the shown NPC and the portraits; the 'y' glyph needs a party
    m_nNPC = -1;
    m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-2);
    m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-2);
    SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0));
    UpdatePortraits();
    m_btnExit.HandleMouseOver(0);
    CSWGuiPanel::OnPanelAdded();
    if (g_pAppManager && g_pAppManager->m_pClientExoApp
        && g_pAppManager->m_pClientExoApp->GetSWParty())
#ifdef K2_UI_TOUCH
        m_iconChange.m_tail.bVisible =
            g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount > 1;
#else
        // the change button's own rule: a party of two or more, or in a module that browses the
        // party table (the Ebon Hawk, the hideout) any member it can show
        m_iconChange.m_tail.bVisible = m_btnChange1.m_tail.bVisible;
#endif
#else
    SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0));
    m_btnExit.HandleMouseOver(0);
    CSWGuiPanel::OnPanelAdded();
#endif
}

void CSWGuiInGameAbilities::OnPanelRemoved()
{
    m_lbAbility.ClearItems();
    m_powerChart.ClearChart();
    m_featChart.ClearChart();
}

void CSWGuiInGameAbilities::CreateFeatList()
{
    if (m_pCreature != 0) {
        m_featChart.CreateFeatChart(m_pCreature->GetServerCreature()->m_pStats);
        m_featChart.SetSkillStatusAll(CSWGuiSkillFlow::SKILL_STATUS_VIEW);
        m_featChart.SetTopLeft();
    }
}

// Build one row per skill.2da entry, taking both ranks from the server-side stats block and the
// row's icon and name strref from the rules table.
void CSWGuiInGameAbilities::CreateSkillList()
{
    m_aSkillList.m_count = 0;
    unsigned char nNumSkills = g_pRules->m_nNumSkills;
    CSWSCreature* pServer = m_pCreature->GetServerCreature();
    int nRow = 0;
    for (uint64_t i = 0; i < nNumSkills; i++) {
        CSWSkill* pSkill = &g_pRules->m_paSkills[i];
        if (pSkill != 0) {
            char nRank = pServer->m_pStats->GetSkillRank(static_cast<unsigned char>(i), 0, 1);
            char nRankMod = pServer->m_pStats->GetSkillRank(static_cast<unsigned char>(i), 0, 0);

            CSWGuiSkillStruct skill;
            skill.m_pSkill = pSkill;
            skill.m_nRank = nRank;
            skill.m_nRankModifier = nRankMod - nRank;
            m_aSkillList.Add(skill);

            CResRef resRef("inone");
            int64_t nStrRef K2_UNSET;
            if (&g_pRules->m_paSkills[i] != 0) {
                nStrRef = g_pRules->m_paSkills[i].m_nNameStrRef;
                resRef = g_pRules->m_paSkills[i].m_icon;
            }
            m_aSkillEntry[nRow].m_borderIcon.m_params.m_flags.nFillStyle = 2;
            m_aSkillEntry[nRow].m_borderIcon.m_params.SetFillImage(resRef, 0);
            m_aSkillEntry[nRow].m_text.m_params.SetStrRef(nStrRef);
            nRow++;
        }
    }
    m_nSelectedSkill = 0;
}

void CSWGuiInGameAbilities::CreatePowerList()
{
    if (m_pCreature != 0) {
        m_powerChart.CreatePowerChart(m_pCreature->GetServerCreature()->m_pStats, 0);
        m_powerChart.SetSkillStatusAll(CSWGuiSkillFlow::SKILL_STATUS_VIEW);
        m_powerChart.SetTopLeft();
    }
}

// The Android build has no power page, so the jedi-class test's result is deliberately discarded,
// and the K2 UpdatePortraits is an empty override.
#if KOTOR1  // K1: a Jedi with at least one power to list
int CSWGuiInGameAbilities::DisplayPowers()
{
    CSWCCreatureStats* pStats = m_pCreature->m_pStats;
    return CSWClass::IsJedi(pStats->GetClass(pStats->m_nNumClasses - 1) & 0xff)
           && m_powerChart.m_aFlows.m_count > 0;
}
#else
void CSWGuiInGameAbilities::DisplayPowers()
{
    CSWCCreatureStats* pStats = m_pCreature->m_pStats;
    CSWClass::IsJedi(pStats->GetClass(pStats->m_nNumClasses - 1) & 0xff);
}
#endif

#if KOTOR1
// K1 paints the party portraits; the intro module cycles the available party-table NPCs and the PC.
void CSWGuiInGameAbilities::UpdatePortraits()
{
    if (!g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        if (pParty->m_nCharacterCount >= 2) {
            m_btnChange1.m_tail.bVisible = 1;
            m_btnChange1.m_border.m_params.SetFillImage(pParty->GetCharacter(1)->GetPortrait(1), 0);
            m_btnChange1.m_borderHilight.m_params.SetFillImage(pParty->GetCharacter(1)->GetPortrait(1), 0);
        } else {
            m_btnChange1.m_tail.bVisible = 0;
        }
        if (pParty->m_nCharacterCount >= 3) {
            m_btnChange2.m_tail.bVisible = 1;
            m_btnChange2.m_border.m_params.SetFillImage(pParty->GetCharacter(2)->GetPortrait(1), 0);
            m_btnChange2.m_borderHilight.m_params.SetFillImage(pParty->GetCharacter(2)->GetPortrait(1), 0);
        } else {
            m_btnChange2.m_tail.bVisible = 0;
        }
        return;
    }

    char nStart = static_cast<char>(m_btnChange1.m_tail.m_nUserData);
    if (nStart == -2)
        nStart = m_nNPC + 1;

    int nFound = 0;
    for (int i = nStart; i < 9; i++) {
        if (i == m_nNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        CSWCCreature* pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pNPC == 0)
            continue;
        if (nFound) {
            m_btnChange2.m_border.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
            m_btnChange2.m_borderHilight.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
            m_btnChange2.m_tail.m_nUserData = i;
            goto both;
        }
        m_btnChange1.m_border.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
        m_btnChange1.m_borderHilight.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
        m_btnChange1.m_tail.m_nUserData = i;
        nFound = 1;
    }

    if (static_cast<unsigned char>(m_nNPC) != 0xff) {
        if (nFound) {
            m_btnChange2.m_border.m_params.SetFillImage(
                g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
            m_btnChange2.m_borderHilight.m_params.SetFillImage(
                g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
            m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-1);
            goto both;
        }
        m_btnChange1.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-1);
        nFound = 1;
    }

    for (int i = 0; i < nStart; i++) {
        if (i == m_nNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        CSWCCreature* pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pNPC == 0)
            continue;
        if (nFound) {
            m_btnChange2.m_border.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
            m_btnChange2.m_borderHilight.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
            m_btnChange2.m_tail.m_nUserData = i;
            goto both;
        }
        m_btnChange1.m_border.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
        m_btnChange1.m_borderHilight.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
        m_btnChange1.m_tail.m_nUserData = i;
        nFound = 1;
    }
    m_btnChange1.m_tail.bVisible = static_cast<unsigned char>(nFound);
    m_btnChange2.m_tail.bVisible = 0;
    goto cyclers;
both:
    m_btnChange1.m_tail.bVisible = static_cast<unsigned char>(nFound);
    m_btnChange2.m_tail.bVisible = 1;
cyclers:
    m_btnCharLeft.m_tail.bVisible =
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNumNPCAvailable() > 2;
    m_btnCharRight.m_tail.bVisible =
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNumNPCAvailable() > 2;
}
#else
void CSWGuiInGameAbilities::UpdatePortraits()
{
}
#endif

#if KOTOR1  // K1: a portrait tap switches the leader (the intro module shows that NPC), then rebuilds the sheet.
void CSWGuiInGameAbilities::OnChangeCharacter(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    CSWCCreature* pCreature;
    if (!g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
        unsigned char nMember = pControl == &m_btnChange2 ? 2 : 1;
        if (!g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, nMember))
            return;
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
        pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    } else {
        signed char nNPC = static_cast<signed char>(pControl->m_tail.m_nUserData);
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        if (nNPC == -1)
            pCreature = pApp->GetPlayerCreature();
        else
            pCreature = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nNPC, 0, 1)));
        if (pCreature == 0)
            goto portraits;
        if (!m_btnCharRight.m_tail.bVisible)
            m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-2);
        else if (pControl == &m_btnChange1)
            m_btnChange1.m_tail.m_nUserData = m_btnChange2.m_tail.m_nUserData;
        m_nNPC = nNPC;
    }
    SetCharacter(pCreature);
portraits:
    UpdatePortraits();
}
#else
void CSWGuiInGameAbilities::OnChangeCharacter(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
}
#endif

#if KOTOR1
// K1: BTN_CHARLEFT steps the left portrait down to the previous available NPC (or the PC), wrapping.
void CSWGuiInGameAbilities::OnSwitchLeft(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    char nCur = static_cast<char>(m_btnChange1.m_tail.m_nUserData);
    if (nCur == -2)
        return;
    m_btnChange2.m_border.m_params.SetFillImage(m_btnChange1.m_border.m_params.m_fillImage, 0);
    m_btnChange2.m_borderHilight.m_params.SetFillImage(
        m_btnChange1.m_borderHilight.m_params.m_fillImage, 0);
    m_btnChange2.m_tail.m_nUserData = m_btnChange1.m_tail.m_nUserData;

    CSWCCreature* pNPC;
    int i;
    for (i = nCur - 1; i >= 0; i--) {
        if (i == m_nNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pNPC)
            goto found;
    }
    if (static_cast<unsigned char>(m_nNPC) != 0xff && static_cast<char>(m_btnChange2.m_tail.m_nUserData) != -1) {
        m_btnChange1.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-1);
        return;
    }
    for (i = 8; i > nCur; i--) {
        if (i == m_nNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pNPC)
            goto found;
    }
    return;
found:
    m_btnChange1.m_border.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
    m_btnChange1.m_borderHilight.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
    m_btnChange1.m_tail.m_nUserData = i;
}

// K1: BTN_CHARRIGHT steps the right portrait up to the next available NPC (or the PC), wrapping.
void CSWGuiInGameAbilities::OnSwitchRight(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    char nCur = static_cast<char>(m_btnChange2.m_tail.m_nUserData);
    if (nCur == -2)
        return;
    m_btnChange1.m_border.m_params.SetFillImage(m_btnChange2.m_border.m_params.m_fillImage, 0);
    m_btnChange1.m_borderHilight.m_params.SetFillImage(
        m_btnChange2.m_borderHilight.m_params.m_fillImage, 0);
    m_btnChange1.m_tail.m_nUserData = m_btnChange2.m_tail.m_nUserData;

    CSWCCreature* pNPC;
    int i;
    for (i = nCur + 1; i < 9; i++) {
        if (i == m_nNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pNPC)
            goto found;
    }
    if (static_cast<unsigned char>(m_nNPC) != 0xff) {
        m_btnChange2.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange2.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-1);
        return;
    }
    for (i = 0; i < nCur; i++) {
        if (i == m_nNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pNPC)
            goto found;
    }
    return;
found:
    m_btnChange2.m_border.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
    m_btnChange2.m_borderHilight.m_params.SetFillImage(pNPC->GetPortrait(1), 0);
    m_btnChange2.m_tail.m_nUserData = i;
}
#endif

// Show the page for the current abilities mode: re-colour the mode buttons, retitle the readout
// column, flip the readout labels' visibility and hand LB_ABILITY its rows.
void CSWGuiInGameAbilities::UpdateView()
{
#if KOTOR1  // K1: BLUE/YELLOW page tabs, no LB_DESC_FEATS switch, and the POWERS page keeps the
            // previous readout texts.
    CExoArrayList<CSWGuiControl*> aControls;
    int nNumRows;
    SetDescription(CExoString(""));

    unsigned char nMode = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode;
    if (nMode == 2) {
        m_btnSkills.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnFeats.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
        m_btnPowers.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);

        m_labelInfoBg.m_tail.bVisible = 0;
        m_labelSkillRank.m_tail.bVisible = 0;
        m_labelRankVal.m_tail.bVisible = 0;
        m_labelBonus.m_tail.bVisible = 0;
        m_labelBonusVal.m_tail.bVisible = 0;
        m_labelTotal.m_tail.bVisible = 0;
        m_labelTotalVal.m_tail.bVisible = 0;

#ifndef K2_UI_TOUCH
        // pc / xbox: the list hands the rows back at the height they were added with (it
        // stretches its visible rows into the leftover height)
        m_lbAbility.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(m_featChart.m_aFlows), 0, 1, 0);
#else
        m_lbAbility.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(m_featChart.m_aFlows), 0, 0, 0);
#endif
        m_lbAbility.SetSelectedControl(static_cast<unsigned int>(m_featChart.m_nSelectedFlow), 0);
        OnEnterFeat(static_cast<unsigned short>(m_featChart.GetSelected()));
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_FEAT_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
    } else if (nMode == 1) {
        m_btnSkills.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnFeats.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnPowers.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);

        m_labelSkillRank.m_text.m_params.SetStrRef(STRREF_BASE_COST);
        m_labelTotal.m_text.m_params.SetStrRef(STRREF_COST_PER_USE);

        m_labelInfoBg.m_tail.bVisible = 1;
        m_labelSkillRank.m_tail.bVisible = 1;
        m_labelRankVal.m_tail.bVisible = 1;
        m_labelBonus.m_tail.bVisible = 1;
        m_labelBonusVal.m_tail.bVisible = 1;
        m_labelTotal.m_tail.bVisible = 1;
        m_labelTotalVal.m_tail.bVisible = 1;

#ifndef K2_UI_TOUCH
        m_lbAbility.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(m_powerChart.m_aFlows), 0, 1, 0);
#else
        m_lbAbility.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(m_powerChart.m_aFlows), 0, 0, 0);
#endif
        m_lbAbility.SetSelectedControl(static_cast<unsigned int>(m_powerChart.m_nSelectedFlow), 0);
        OnEnterPower(static_cast<unsigned int>(m_powerChart.GetSelected()));
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_FORCE_POWER_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
    } else if (nMode == 0) {
        m_btnSkills.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
        m_btnFeats.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnPowers.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);

        m_labelSkillRank.m_text.m_params.SetStrRef(STRREF_SKILL_RANK);
        m_labelBonus.m_text.m_params.SetStrRef(STRREF_BONUS);
        m_labelTotal.m_text.m_params.SetStrRef(STRREF_TOTAL_RANK);

        m_labelInfoBg.m_tail.bVisible = 1;
        m_labelSkillRank.m_tail.bVisible = 1;
        m_labelRankVal.m_tail.bVisible = 1;
        m_labelBonus.m_tail.bVisible = 1;
        m_labelBonusVal.m_tail.bVisible = 1;
        m_labelTotal.m_tail.bVisible = 1;
        m_labelTotalVal.m_tail.bVisible = 1;

        nNumRows = m_aSkillList.m_count;
        if (nNumRows > 0) {
            if (nNumRows > 10)
                nNumRows = 10;
            for (int i = 0; i < nNumRows; i++) {
                m_aSkillEntry[i].SetActive(0);
                aControls.Add(&m_aSkillEntry[i]);
            }
        }
        m_lbAbility.AddControls(aControls, 0, 1, 0);
        m_lbAbility.SetSelectedControl(static_cast<unsigned int>(m_nSelectedSkill), 0);
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_SKILL_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
    }

    SetActiveControl(0, 0);
    SetActiveControl(&m_lbAbility, 0);
#else
    CExoArrayList<CSWGuiControl*> aControls;
    int nNumRows;
    SetDescription(CExoString(""));

    unsigned char nMode = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode;
    if (nMode == 2) {
        m_btnSkills.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnFeats.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        m_btnPowers.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);

        m_labelInfoBg.m_tail.bVisible = 0;
        m_labelSkillRank.m_tail.bVisible = 0;
        m_labelRankVal.m_tail.bVisible = 0;
        m_labelBonus.m_tail.bVisible = 0;
        m_labelBonusVal.m_tail.bVisible = 0;
        m_labelTotal.m_tail.bVisible = 0;
        m_labelTotalVal.m_tail.bVisible = 0;

        m_lbFeats.m_tail.bVisible = 1;
        m_lbDesc.m_tail.bVisible = 0;

        m_lbAbility.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(m_featChart.m_aFlows), 0, 0, 0);
        m_lbAbility.SetSelectedControl(static_cast<unsigned int>(m_featChart.m_nSelectedFlow), 0);
        OnEnterFeat(static_cast<unsigned short>(m_featChart.GetSelected()));
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_FEAT_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
    } else if (nMode == 1) {
        m_btnSkills.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnFeats.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnPowers.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);

        m_labelName.m_text.m_params.SetText(CExoString(""));
        m_labelRankVal.m_text.m_params.SetText(CExoString(""));
        m_labelBonusVal.m_text.m_params.SetText(CExoString(""));
        m_labelTotalVal.m_text.m_params.SetText(CExoString(""));

        m_labelSkillRank.m_text.m_params.SetStrRef(STRREF_BASE_COST);
        m_labelTotal.m_text.m_params.SetStrRef(STRREF_COST_PER_USE);

        m_labelInfoBg.m_tail.bVisible = 1;
        m_labelSkillRank.m_tail.bVisible = 1;
        m_labelRankVal.m_tail.bVisible = 1;
        m_labelBonus.m_tail.bVisible = 1;
        m_labelBonusVal.m_tail.bVisible = 1;
        m_labelTotal.m_tail.bVisible = 1;
        m_labelTotalVal.m_tail.bVisible = 1;

        m_lbFeats.m_tail.bVisible = 0;
        m_lbDesc.m_tail.bVisible = 1;

        m_lbAbility.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(m_powerChart.m_aFlows), 0, 0, 0);
        m_lbAbility.SetSelectedControl(static_cast<unsigned int>(m_powerChart.m_nSelectedFlow), 0);
        OnEnterPower(static_cast<unsigned int>(m_powerChart.GetSelected()));
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_FORCE_POWER_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
    } else if (nMode == 0) {
        m_btnSkills.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        m_btnFeats.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnPowers.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);

        m_labelSkillRank.m_text.m_params.SetStrRef(STRREF_SKILL_RANK);
        m_labelBonus.m_text.m_params.SetStrRef(STRREF_BONUS);
        m_labelTotal.m_text.m_params.SetStrRef(STRREF_TOTAL_RANK);

        m_labelInfoBg.m_tail.bVisible = 1;
        m_labelSkillRank.m_tail.bVisible = 1;
        m_labelRankVal.m_tail.bVisible = 1;
        m_labelBonus.m_tail.bVisible = 1;
        m_labelBonusVal.m_tail.bVisible = 1;
        m_labelTotal.m_tail.bVisible = 1;
        m_labelTotalVal.m_tail.bVisible = 1;

        m_lbFeats.m_tail.bVisible = 0;
        m_lbDesc.m_tail.bVisible = 1;

        nNumRows = m_aSkillList.m_count;
        if (nNumRows > 0) {
            if (nNumRows > 10)
                nNumRows = 10;
            for (int i = 0; i < nNumRows; i++) {
                m_aSkillEntry[i].SetActive(0);
#ifdef K2_QOL_IMPROVEMENTS
                K2QolSizeSkillInfoRow(m_aSkillEntry[i], m_lbAbility);
#endif
                aControls.Add(&m_aSkillEntry[i]);
            }
        }
        m_lbAbility.AddControls(aControls, 0, 1, 0);
        m_lbAbility.SetSelectedControl(static_cast<unsigned int>(m_nSelectedSkill), 0);
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_SKILL_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
    }

    SetActiveControl(0, 0);
    SetActiveControl(&m_lbAbility, 0);
#endif
}

void CSWGuiInGameAbilities::OnAbilitySelectionChanged(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWGuiControl* pSelected = m_lbAbility.GetSelectedControl();
#if KOTOR1  // K1: an empty selection returns before the hit test
    if (pSelected == 0)
        return;
#endif
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode == 1) {
        unsigned int nSkill = static_cast<CSWGuiSkillFlow*>(pSelected)
                                  ->SkillHitCheckMouse(m_lbAbility.m_nHitLocalX, m_lbAbility.m_nHitLocalY);
        m_powerChart.SetSelectedSkill(nSkill);
        OnEnterPower(static_cast<unsigned short>(nSkill));
    } else if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode == 2) {
        unsigned int nFeat = static_cast<CSWGuiSkillFlow*>(pSelected)
                                 ->SkillHitCheckMouse(m_lbAbility.m_nHitLocalX, m_lbAbility.m_nHitLocalY);
        m_featChart.SetSelectedSkill(nFeat);
        OnEnterFeat(static_cast<unsigned short>(nFeat));
    }
}

void CSWGuiInGameAbilities::SetCharacter(CSWCCreature* pCreature)
{
#if KOTOR1  // K1: the portrait label follows the sheet; POWERS shows only with a power to list
    m_pCreature = pCreature;
    if (pCreature == 0)
        return;

    m_lblPortrait.m_border.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
    m_lbAbility.ClearItems();
    CreateFeatList();
    CreateSkillList();

    CSWCCreatureStats* pStats = m_pCreature->m_pStats;
    if (CSWClass::IsJedi(pStats->GetClass(pStats->m_nNumClasses - 1) & 0xff) == 0) {
        m_btnPowers.m_tail.bVisible = 0;
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode == 1)
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 0;
    } else {
        CreatePowerList();
        if (m_powerChart.m_aFlows.m_count > 0)
            m_btnPowers.m_tail.bVisible = 1;
    }
    UpdateView();
#else
    m_pCreature = pCreature;
    if (pCreature == 0)
        return;

    m_lbAbility.ClearItems();
    CreateFeatList();
    CreateSkillList();

    CSWCCreatureStats* pStats = m_pCreature->m_pStats;
    if (CSWClass::IsJedi(pStats->GetClass(pStats->m_nNumClasses - 1) & 0xff) == 0) {
        m_btnPowers.m_tail.bVisible = 0;
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode == 1)
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 0;
    } else {
        CreatePowerList();
        m_btnPowers.m_tail.bVisible = 1;
    }
    UpdateView();
#endif
}

// Fill the force-point readout column from the spell's adjusted cost, colouring the delta by sign
// and giving the name/description pair its "--" fallback for spells the creature does not have.
void CSWGuiInGameAbilities::OnEnterPower(unsigned int nPower)
{
#if KOTOR1  // K1: the cost comes from the ForceAdjust column rounded by alignment, a non-zero
            // delta is captioned with the light/dark-side word, and there is no null-spell test.
    CSWSpell* pSpell = g_pRules->GetSpell(nPower);
    unsigned char nAlignmentCode = pSpell->m_nAlignmentCode;
    int nAlignment = m_pCreature->GetServerCreature()->m_pStats->m_nAlignment;

    CExoString sCostType;
    CExoString sCostLabel;
    CExoString sCostValue;
    CExoString sAlignment;
    Vector color;
    unsigned char nBand;
    int nDelta;

    if (nAlignmentCode == 'G')
        sCostType = "GoodCost";
    else if (nAlignmentCode == 'E')
        sCostType = "EvilCost";

    nBand = static_cast<unsigned char>(nAlignment / 10);
    nDelta = 0;
    if (sCostType != "") {
        float fScale;
        g_pRules->m_p2DArrays->m_pForceAdjust->GetFLOATEntry(nBand, sCostType, &fScale);
        nDelta = static_cast<int>(fScale * static_cast<float>(pSpell->m_nBaseForcePoints) + 0.5f) - pSpell->m_nBaseForcePoints;
    }

    if (nBand >= 6)
        sAlignment = g_pTlkTable->GetSimpleString(STRREF_ALIGNMENT_LIGHT_SIDE);
    else if (nBand != 5)
        sAlignment = g_pTlkTable->GetSimpleString(STRREF_ALIGNMENT_DARK_SIDE);

    if (nDelta < 0) {
        sCostValue.Format(const_cast<char*>("%d"), nDelta);
        color = CGuiInGame::COLOR_GREEN;
        sCostLabel = sAlignment + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_ADJUSTMENT);
    } else if (nDelta == 0) {
        sCostValue.Format(const_cast<char*>("%d"), 0);
        color = CGuiInGame::COLOR_BLUE;
        sCostLabel = g_pTlkTable->GetSimpleString(STRREF_ADJUSTMENT);
    } else {
        sCostValue.Format(const_cast<char*>("+%d"), nDelta);
        color = CGuiInGame::COLOR_RED;
        sCostLabel = sAlignment + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_ADJUSTMENT);
    }

    m_labelBonusVal.m_text.m_params.SetText(sCostValue);
    m_labelBonusVal.m_text.m_params.SetColor(color);
    m_labelBonus.m_text.m_params.SetText(sCostLabel);
    m_labelRankVal.m_text.m_params.SetText(CExoString(pSpell->m_nBaseForcePoints));
    m_labelTotalVal.m_text.m_params.SetText(CExoString(nDelta + pSpell->m_nBaseForcePoints));

    m_labelName.m_text.m_params.SetStrRef(pSpell->m_nNameStrRef);
    SetDescription(pSpell->m_nDescriptionStrRef);
#else
    CSWSpell* pSpell = g_pRules->GetSpell(nPower);
    if (pSpell == 0)
        return;

    unsigned char nAlignmentCode = pSpell->m_nAlignmentCode;
    int nAlignment = m_pCreature->GetServerCreature()->m_pStats->m_nAlignment;

    CExoString sCostType;
    CExoString sCostLabel;
    CExoString sCostValue;
    Vector color;
    color = CGuiInGame::COLOR_NEW_TEXT;

    if (nAlignmentCode == 'G')
        sCostType = "GoodCost";
    else if (nAlignmentCode == 'E')
        sCostType = "EvilCost";

#if KOTOR1  // K1: no room force rating; alignment and in-party only
    int nCost = pSpell->GetAdjustedForcePoints(static_cast<unsigned char>(nAlignment / 10) * 10, 1);
#else
    CSWSArea* pArea = static_cast<CSWSArea*>(m_pCreature->GetServerCreature()->GetArea());
    int nRoomForce = 0;
    if (pArea != 0)
        nRoomForce = pArea->GetRoomForceRating(m_pCreature->GetServerCreature()->m_position);

    int nCost = pSpell->GetAdjustedForcePoints(
        static_cast<unsigned char>(nAlignment / 10) * 10, 1, nRoomForce,
        m_pCreature->GetServerCreature()->m_stanceId,
        m_pCreature->GetServerCreature()->m_pStats->m_nCHAMod);
#endif

    int nDelta = nCost - pSpell->m_nBaseForcePoints;
    if (nDelta < 0) {
        sCostValue.Format(const_cast<char*>("%d"), nDelta);
        color = *reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS);
        sCostLabel = g_pTlkTable->GetSimpleString(STRREF_ADJUSTMENT);
    } else if (nDelta == 0) {
        sCostValue.Format(const_cast<char*>("%d"), 0);
        color = CGuiInGame::COLOR_NEW_TEXT;
        sCostLabel = g_pTlkTable->GetSimpleString(STRREF_ADJUSTMENT);
    } else {
        sCostValue.Format(const_cast<char*>("+%d"), nDelta);
        color = CGuiInGame::COLOR_RED;
        sCostLabel = g_pTlkTable->GetSimpleString(STRREF_ADJUSTMENT);
    }

    m_labelBonusVal.m_text.m_params.SetText(sCostValue);
    m_labelBonusVal.m_text.m_params.SetColor(color);
    m_labelBonus.m_text.m_params.SetText(sCostLabel);
    m_labelRankVal.m_text.m_params.SetText(CExoString(pSpell->m_nBaseForcePoints));
    m_labelTotalVal.m_text.m_params.SetText(CExoString(nDelta + pSpell->m_nBaseForcePoints));

    if ((((nPower - FORCE_POWER_FORCE_SIGHT < 7) &&
          (nPower - FORCE_POWER_FORCE_SIGHT != FORCE_POWER_MIND_TRICK - FORCE_POWER_FORCE_SIGHT)) ||
         nPower == FORCE_POWER_BREATH_CONTROL)
        && m_pCreature->GetServerCreature()->m_pStats->HasSpell(0, nPower, 0) == 0) {
        m_labelName.m_text.m_params.SetStrRef(STRREF_UNKNOWN_FORCE_POWER);
        SetDescription(STRREF_FORCE_POWER_NOT_LEARNABLE);
        m_labelBonusVal.m_text.m_params.SetText(CExoString("--"));
        m_labelBonusVal.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_labelRankVal.m_text.m_params.SetText(CExoString("--"));
        m_labelTotalVal.m_text.m_params.SetText(CExoString("--"));
        return;
    }

    m_labelName.m_text.m_params.SetStrRef(pSpell->m_nNameStrRef);
    SetDescription(pSpell->m_nDescriptionStrRef);
#endif
}

void CSWGuiInGameAbilities::OnEnterFeat(unsigned short nFeat)
{
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    SetDescription(pFeat->GetDescriptionText());
    m_labelName.m_text.m_params.SetStrRef(pFeat->m_nNameStrRef);
}

void CSWGuiInGameAbilities::SetDescription(unsigned int nStrRef)
{
    STR_RES res;
    g_pTlkTable->Fetch(nStrRef, res, CTlkTable::GENDER_MALE);
    m_labelDesc.m_text.m_params.SetText(res.text);
    UpdateDescriptionListBox(res.text.GetLength() > 0);
}

void CSWGuiInGameAbilities::UpdateDescriptionListBox(int bHasText)
{
    CSWGuiControl* apControls[1];
#if KOTOR1  // K1: one description list for every page
    CSWGuiListBox* pList = &m_lbDesc;
#else
    CSWGuiListBox* pList =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode == 2 ? &m_lbFeats
                                                                             : &m_lbDesc;
#endif
    pList->ClearItems();

    CSWGuiExtent extent = m_labelDesc.m_extent;
    CSWGuiText* pText = &m_labelDesc.m_text;
#ifdef K2_QOL_IMPROVEMENTS
    K2QolFitText(*pText, 0x7fffffff);   // LB_DESC scrolls its one row: no limit
#endif
    if (bHasText != 0)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_labelDesc.SetExtent(extent);

    apControls[0] = &m_labelDesc;
    pList->AddControls(apControls, 1, 0, 0, 0);
    pList->SetSelectedControl(&m_labelDesc, 0);
}

void CSWGuiInGameAbilities::OnEnterSkill(CSWGuiControl* pControl)
{
    unsigned char nSkill = static_cast<unsigned char>(pControl->m_tail.nControlId);
    m_nSelectedSkill = nSkill;
    CSWGuiSkillStruct& entry = m_aSkillList.m_pArray[nSkill];
    CSWSkill* pSkill = entry.m_pSkill;
    int nRank = entry.m_nRank;
    int nModifier = entry.m_nRankModifier;

    CExoString sModifier;
    Vector color;
    if (nModifier >= 1) {
        sModifier = CExoString("+") + CExoString(nModifier);
#if KOTOR1  // K1: bonus COLOR_GREEN, zero COLOR_BLUE
        color = CGuiInGame::COLOR_GREEN;
#else
        color = *reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS);
#endif
    } else if (nModifier >= 0) {
        sModifier = CExoString(nModifier);
#if KOTOR1
        color = CGuiInGame::COLOR_BLUE;
#else
        color = CGuiInGame::COLOR_NEW_TEXT;
#endif
    } else {
        sModifier = CExoString(nModifier);
        color = CGuiInGame::COLOR_RED;
    }

    m_labelRankVal.m_text.m_params.SetText(CExoString(nRank));
    m_labelBonusVal.m_text.m_params.SetText(sModifier);
    m_labelBonusVal.m_text.m_params.SetColor(color);
    m_labelTotalVal.m_text.m_params.SetText(CExoString(nRank + nModifier));
    m_labelName.m_text.m_params.SetStrRef(pSkill->m_nNameStrRef);
    SetDescription(pSkill->GetDescriptionText());
}

// Panel input: the button ids that page the description, cycle the party in the intro module,
// step the flow chart, and switch the abilities mode.
void CSWGuiInGameAbilities::HandleInputEvent(int type, int val)
{
#if KOTOR1  // K1: A pages the description, and the switch key walks the NPCs in the intro module
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_A:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (m_lbDesc.m_scrollBar.m_nCurValue == m_lbDesc.m_scrollBar.m_nMaxValue) {
                m_lbDesc.HandleInputEvent(SWGUIEVENT_SCROLL_PAGE_UP, 1);
            } else {
                for (int i = 0; i < 10; i++)
                    m_lbDesc.HandleInputEvent(SWGUIEVENT_SCROLL_LINE_DOWN, 1);
            }
            break;

#ifdef K1_UI_XBOX  // xbox: RB (0x2b in menus) changes the character, not Y
        case INPUTEVENT_GUI_BLACK:
#else
        case INPUTEVENT_GUI_Y:
#endif
        case INPUTEVENT_CHANGE_CHARACTER:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            if (!g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
                g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
            } else {
                for (int i = m_nNPC + 1; i <= 8; i++) {
                    if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
                        continue;
                    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
                    CSWCCreature* pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
                        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
                    if (pNPC == 0)
                        continue;
                    SetCharacter(pNPC);
                    m_nNPC = static_cast<signed char>(i);
                    goto portraits;
                }
                m_nNPC = -1;
            }
            SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());
        portraits:
            UpdatePortraits();
            break;

        case INPUTEVENT_GUI_DPAD_LEFT: case INPUTEVENT_GUI_DPAD_RIGHT: case INPUTEVENT_GUI_KEY_LEFT: case INPUTEVENT_GUI_KEY_RIGHT:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode != 0)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
        case INPUTEVENT_GUI_DPAD_UP: case INPUTEVENT_GUI_DPAD_DOWN: case INPUTEVENT_GUI_KEY_UP: case INPUTEVENT_GUI_KEY_DOWN:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode == 1)
                OnEnterPower(m_powerChart.HandleInput(type));
            else if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode == 2)
                OnEnterFeat(static_cast<unsigned short>(m_featChart.HandleInput(type)));
            break;

        case INPUTEVENT_GUI_B: case INPUTEVENT_GUI_START: case INPUTEVENT_GUI_BACK:
        case INPUTEVENT_GUI:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0) != 0)
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            break;

        case INPUTEVENT_GUI_X:
            {
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                unsigned char nMode =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode;
                if (nMode == 0) {
                    if (DisplayPowers())
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 1;
                    else
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 2;
                } else if (nMode == 1) {
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 2;
                } else if (nMode == 2) {
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 0;
                }
                UpdateView();
            }
            break;

        case INPUTEVENT_GUI_SCROLL_UP:
            m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;
        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;

        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
#else
    if (val != 0) {
        switch (type) {
#ifndef K2_UI_XBOX  // xbox: RB alone changes the character
        case INPUTEVENT_GUI_Y:
#endif
        case INPUTEVENT_CHANGE_CHARACTER:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
            SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0));
            break;

        case INPUTEVENT_GUI_DPAD_LEFT: case INPUTEVENT_GUI_DPAD_RIGHT: case INPUTEVENT_GUI_KEY_LEFT: case INPUTEVENT_GUI_KEY_RIGHT:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode != 0)
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            // fallthrough: the sound is the only difference between these two ranges
        case INPUTEVENT_GUI_DPAD_UP: case INPUTEVENT_GUI_DPAD_DOWN: case INPUTEVENT_GUI_KEY_UP: case INPUTEVENT_GUI_KEY_DOWN:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode == 1)
                OnEnterPower(m_powerChart.HandleInput(type));
            else if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode == 2)
                OnEnterFeat(static_cast<unsigned short>(m_featChart.HandleInput(type)));
            break;

        case INPUTEVENT_GUI_B: case INPUTEVENT_GUI_START: case INPUTEVENT_GUI_BACK:
        case INPUTEVENT_GUI:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0) != 0)
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            break;

        case INPUTEVENT_GUI_X:
            {
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                unsigned char nMode =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode;
                if (nMode == 0) {
                    if (CSWClass::IsJedi(m_pCreature->m_pStats->GetClass(m_pCreature->m_pStats->m_nNumClasses - 1) & 0xff))
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 1;
                    else
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 2;
                } else if (nMode == 1) {
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 2;
                } else if (nMode == 2) {
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nAbilitiesMode = 0;
                }
                UpdateView();
            }
            break;

        case INPUTEVENT_GUI_SCROLL_UP:
            m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;
        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;

        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
#endif
}




// Point one row at a skill, feat or spell id; nType selects which record the icon and name strref
// come from.
void CSWGuiInGameSkillEntry::SetSkill(unsigned int nId, unsigned char nType)
{
    CResRef resRef("inone");
    int64_t nStrRef = 0;

    switch (nType)
    {
    case ENTRY_SKILL:
        if (&g_pRules->m_paSkills[static_cast<unsigned short>(nId)] != 0)
        {
            nStrRef = g_pRules->m_paSkills[static_cast<unsigned short>(nId)].m_nNameStrRef;
            resRef = g_pRules->m_paSkills[static_cast<unsigned short>(nId)].m_icon;
        }
        break;
    case ENTRY_FEAT:
    {
        CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nId));
        if (pFeat != 0)
        {
            resRef = pFeat->m_icon;
            nStrRef = pFeat->m_nNameStrRef;
        }
        break;
    }
    case ENTRY_POWER:
    {
        CSWSpell* pSpell = g_pRules->GetSpell(nId);
        if (pSpell != 0)
        {
            resRef = pSpell->m_iconResRef;
            nStrRef = pSpell->m_nNameStrRef;
        }
        break;
    }
    }

    m_borderIcon.m_params.m_flags.nFillStyle = 2;
    m_borderIcon.m_params.SetFillImage(resRef);
    m_text.m_params.SetStrRef(nStrRef);
}

// Highlight the row when it becomes the active selection, restoring the default colour otherwise.
#if !KOTOR1  // K1 does not override this slot
void CSWGuiInGameSkillEntry::SetActive(int flag)
{
    m_tail.bActive = static_cast<unsigned char>(flag);

    if (flag)
    {
        m_text.m_params.bHilight = 1;
        m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    }
    else
    {
        m_text.m_params.bHilight = 0;
        m_text.m_params.RestoreDefaultColor();
    }
}
#endif

void CSWGuiInGameSkillEntry::Draw(float f)
{
    if (m_tail.bActive)
    {
        m_hilightBorder.Draw(f);
        m_borderHilight.Draw(f);
    }
    else
    {
        m_border.Draw(f);
        m_borderRow.Draw(f);
    }

    m_borderIcon.Draw(f);
    m_text.Draw(f);
}

// Lay the row out: the icon square plus the caption's two border pairs, all scaled from the text
// size, and the caption sized against the intersection of the outer borders' inner rects.
float GetResolutionTextScalar();

void CSWGuiInGameSkillEntry::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1  // K1: base SetExtent, then a scaled 42-pixel row with a 28-pixel icon inset by 7
    if (extent.w < 43)
        return;

    CSWGuiObject::SetExtent(extent);

#ifndef K2_UI_TOUCH
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;   // pc / xbox: the 480-line layout's units
#else
    static float s_fScale = g_SDL_mode.h / 480.0f;   // pc / xbox: the 480-line layout's units
#endif
#else
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = g_SDL_mode.h / 768.0f;
#else
    static float s_fScale = g_SDL_mode.h / 768.0f;
#endif
#endif
    float fSize = s_fScale * 42.0f;
    int nSize = static_cast<int>(fSize);
    int nIconSize = static_cast<int>(s_fScale * 28.0f);

    m_borderIcon.m_extent = CSWGuiExtent(static_cast<int>(s_fScale * 7.0f + static_cast<float>(m_extent.x)),
                                         static_cast<int>(s_fScale * 7.0f + static_cast<float>(m_extent.y)), nIconSize, nIconSize);
    m_borderRow.m_extent = CSWGuiExtent(m_extent.x, m_extent.y, nSize, nSize);
    m_borderHilight.m_extent = CSWGuiExtent(m_extent.x, m_extent.y, nSize, nSize);

    int nLeft = static_cast<int>(static_cast<float>(m_extent.x) + fSize);
    int nWidth = static_cast<int>(static_cast<float>(m_extent.w) - fSize);
    m_border.m_extent = CSWGuiExtent(nLeft, m_extent.y, nWidth, nSize);
    m_hilightBorder.m_extent = CSWGuiExtent(nLeft, m_extent.y, nWidth, nSize);

    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                m_hilightBorder.GetInnerExtent()));
#else
    if (extent.w < 43)
        return;

    m_extent = extent;

    float fSize = GetResolutionTextScalar() * 42.0f;
    int nSize = static_cast<int>(fSize);
#ifdef K2_FIX_ENGINE_BUGS
    // In a list a few pixels short of a whole number of rows, one row fewer shows and the slack is
    // spread between rows this frame does not grow into: the rows take the height that fills it.
    CSWGuiControl* pList = m_pContainer ? m_pContainer->AsSWGuiListBox() : 0;
    if (pList) {
        const CSWGuiListBox* pBox = static_cast<CSWGuiListBox*>(pList);
        const int nAvail = pBox->m_innerExtent.h - pBox->m_nPadding;
        const int nPitch = nSize + pBox->m_nPadding;
        const int nRows = (nAvail + nPitch / 2) / nPitch;
        const int nFit = nRows > 0 ? nAvail / nRows - pBox->m_nPadding : nSize;
        if (nFit > 0 && nFit < nSize) {
            if (m_extent.h > nFit && m_extent.h <= nSize)
                m_extent.h = nFit;
            nSize = nFit;
            fSize = static_cast<float>(nFit);
        }
    }
#endif
    int nIconSize = static_cast<int>(static_cast<float>(nSize) * 0.8f);
    int nOffset = (nSize - nIconSize) / 2;
    int nLeft = static_cast<int>(static_cast<float>(m_extent.x) + fSize);
    int nWidth = static_cast<int>(static_cast<float>(m_extent.w) - fSize);

    m_border.m_extent = CSWGuiExtent(nLeft, m_extent.y, nWidth, nSize);
    m_hilightBorder.m_extent = CSWGuiExtent(nLeft, m_extent.y, nWidth, nSize);

    m_borderRow.m_extent = CSWGuiExtent(m_extent.x, m_extent.y, nSize, nSize);
    m_borderHilight.m_extent = CSWGuiExtent(m_extent.x, m_extent.y, nSize, nSize);

    m_borderIcon.m_extent = CSWGuiExtent(m_extent.x + nOffset, m_extent.y + nOffset, nIconSize, nIconSize);

    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                m_hilightBorder.GetInnerExtent()));
#ifdef K2_QOL_IMPROVEMENTS
    // At the Text Size a row taller than the game's lays out at its own height: the icon frame is a
    // square of the row's height (at most a third of its width) centred down its left edge, the
    // icon 0.8 of it, the caption's borders the rest of the row; the caption fits its border.
    if (k2qol_dialog_text_scale() <= 100)
        return;
    if (m_extent.h > nSize) {
        int nRowSize = m_extent.h;
        if (nRowSize > m_extent.w / 3)
            nRowSize = m_extent.w / 3;
        const int nRowIcon = static_cast<int>(static_cast<float>(nRowSize) * 0.8f);
        const int nTop = m_extent.y + (m_extent.h - nRowSize) / 2;
        const int nIconOffset = (nRowSize - nRowIcon) / 2;
        const int nTextLeft = m_extent.x + nRowSize;
        const int nTextWidth = m_extent.w - nRowSize;
        m_border.m_extent = CSWGuiExtent(nTextLeft, m_extent.y, nTextWidth, m_extent.h);
        m_hilightBorder.m_extent = CSWGuiExtent(nTextLeft, m_extent.y, nTextWidth, m_extent.h);
        m_borderRow.m_extent = CSWGuiExtent(m_extent.x, nTop, nRowSize, nRowSize);
        m_borderHilight.m_extent = CSWGuiExtent(m_extent.x, nTop, nRowSize, nRowSize);
        m_borderIcon.m_extent = CSWGuiExtent(m_extent.x + nIconOffset, nTop + nIconOffset, nRowIcon, nRowIcon);
        m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                    m_hilightBorder.GetInnerExtent()));
    }
    K2QolFitText(m_text, m_text.m_extent.h);
#endif
#endif
}

// Copy the prototype row's text and borders, build the two highlight borders, and size the row
// from the supplied width and the scaled text height.
void CSWGuiInGameSkillEntry::Initialize(CSWGuiText& text, CSWGuiBorder& border,
                                        CSWGuiBorder& borderHilight, int nWidth)
{
#if KOTOR1  // K1: "lbl_hex_3" rows with BLUE borders and YELLOW highlight borders.
    m_text.m_params = text.m_params;
    CSWGuiBorderParams& borderParams = m_border.m_params;
    borderParams = border.m_params;
    borderParams.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    CSWGuiBorderParams& hilightParams = m_hilightBorder.m_params;
    hilightParams = borderHilight.m_params;
    hilightParams.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    hilightParams.SetPulsingAlpha(1, 1, 0);

    CSWGuiBorderParams& rowParams = m_borderRow.m_params;
    rowParams = CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("lbl_hex_3"),
                                   0, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0);
    rowParams.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);

    CSWGuiBorderParams& rowHilightParams = m_borderHilight.m_params;
    rowHilightParams = CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("lbl_hex_3"),
                                          0, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0);
    rowHilightParams.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    rowHilightParams.SetPulsingAlpha(1, 0, 1);

#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    // the frames stretched rather than tiled once a row is drawn larger than the art
    rowParams.m_flags.nFillStyle = 2;
    rowHilightParams.m_flags.nFillStyle = 2;
#endif
#ifndef K2_UI_TOUCH
    // pc / xbox: the 480-line layout's units
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#else
    static float s_fScale = g_SDL_mode.h / 480.0f;
#endif
#else
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = g_SDL_mode.h / 768.0f;
#else
    static float s_fScale = g_SDL_mode.h / 768.0f;
#endif
#endif
    SetExtent(CSWGuiExtent(0, 0, nWidth, static_cast<int>(s_fScale * 42.0f)));
#else
    m_text.m_params = text.m_params;
    CSWGuiBorderParams& borderParams = m_border.m_params;
    borderParams = border.m_params;
    borderParams.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
    CSWGuiBorderParams& hilightParams = m_hilightBorder.m_params;
    hilightParams = borderHilight.m_params;
    hilightParams.SetPulsingAlpha(1, 1, 0);

    CSWGuiBorderParams& rowParams = m_borderRow.m_params;
    rowParams = CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("UIBIT_EQP_ITM1"),
                                   0, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0);
    rowParams.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);

    CSWGuiBorderParams& rowHilightParams = m_borderHilight.m_params;
    rowHilightParams = CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("UIBIT_EQP_ITM1"),
                                          0, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0);
    rowHilightParams.SetPulsingAlpha(1, 0, 1);
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    // the row frames stretched, not tiled (as the item rows)
    rowParams.m_flags.nFillStyle = 2;
    rowHilightParams.m_flags.nFillStyle = 2;
#endif

    SetExtent(CSWGuiExtent(0, 0, nWidth, static_cast<int>(GetResolutionTextScalar() * 42.0f)));
#endif
}

void CSWGuiInGameSkillEntry::Initialize(CSWGuiExtent const&, CSWGuiTextParams const&,
                                        CSWGuiBorderParams const&, CSWGuiBorderParams const&)
{
}

void CSWGuiInGameSkillEntry::Initialize(CSWGuiExtent const&, CSWGuiLabelHilight*)
{
}