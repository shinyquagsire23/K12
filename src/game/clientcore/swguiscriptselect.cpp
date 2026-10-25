// The "which AI script should this creature run?" modal.

#include "k2stdint.h"
#include "swguiscriptselect.h"

#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE 1
#include "CExoArrayListImpl.h"

#include "swgui.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "exoresman.h"
#include "twodimarray.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "clientexoapp.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"

#include <string.h>
#include "exoinput.h"
#include "tutorial.h"
#include "exostringinline.h"

#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);  // the platform's button glyph
#endif

// Loads the layout, binds the controls, clones a description row, walks aiscripts.2da into the
// script list and binds the two buttons.
CSWGuiScriptSelect::CSWGuiScriptSelect(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
    , _reserved1188(0)
{
#if KOTOR1
    StartLoadFromLayout(CResRef("ScriptSelect"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("ScriptSelect_p"), 1, 1, 1);
#endif

    InitControl(m_listBox, CExoString("LST_AIState"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_Back"), 1, 1, 0, 0, 0);
    InitControl(m_btnAccept, CExoString("BTN_Accept"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#if KOTOR1  // K1: the panel is scaled to the screen
    ScaleExtentForResolution();
#endif
#if KOTOR1 && !defined(K2_UI_TOUCH)
    // The PC ScriptSelect is the one PC layout whose background tiles; the touch build stretches
    // it instead, as it does every other panel's.
    m_pBackground->m_params.m_flags.nFillStyle = 2;
#endif

    // The description row is cloned from the description box's prototype row, which is never
    // bound to a control.
    m_lblColour.Initialize(m_listBoxDesc.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem), 1.0f);

    CExoArrayList<CSWGuiButtonToggle*> buttons;
    CSWGuiButtonToggle* pTemplate = static_cast<CSWGuiButtonToggle*>(m_listBox.m_pProtoItem);

    C2DA* p2DA = new C2DA(CResRef("aiscripts"), 0);
    p2DA->Load2DArray();

    int nSize = p2DA->m_nRows;
    if (nSize != 0)
    {
        buttons.m_capacity = nSize;
        buttons.m_pArray = new CSWGuiButtonToggle*[nSize];
    }
    buttons.m_count = nSize;
    m_aScripts.SetSize(nSize);

    for (int64_t i = 0; i < p2DA->m_nRows; i++)
    {
        int nNameStrRef;
        int nDescStrRef;
        int nAIState;
        p2DA->GetINTEntry(static_cast<int>(i), CExoString("NAME_STRREF"), &nNameStrRef);
        p2DA->GetINTEntry(static_cast<int>(i), CExoString("DESCRIPTION_STRREF"), &nDescStrRef);
        p2DA->GetINTEntry(static_cast<int>(i), CExoString("AISTATE"), &nAIState);

        buttons.m_pArray[i] = CreateOption(nNameStrRef,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiScriptSelect::OnEnter), pTemplate);
        m_aScripts.m_pArray[i].m_nDescriptionStrRef = nDescStrRef;
        m_aScripts.m_pArray[i].m_nAIState = nAIState;
    }

    delete p2DA;

    m_listBox.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(buttons), 1, 0, 0);

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));

    m_listBox.bSelectOnLeftClick = 1;
    SetActiveControl(&m_listBox, 0);
#if KOTOR1
    // K1: the platform's A and B glyphs on the two buttons.
    IosAddButtonIcon(&m_btnAccept, 'a');
    IosAddButtonIcon(&m_btnBack, 'b');
#endif
}

CSWGuiScriptSelect::~CSWGuiScriptSelect()
{
    m_listBoxDesc.ClearItems();
}

CSWGuiButtonToggle* CSWGuiScriptSelect::CreateOption(uint64_t nStrRef,
                                                     void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                                                     CSWGuiButtonToggle* pTemplate)
{
    CSWGuiExtent extent;
    CSWGuiButtonToggle* pButton = new CSWGuiButtonToggle();

    pButton->Initialize(pTemplate->m_extent, pTemplate);
    pButton->m_text.m_params.SetStrRef(nStrRef);

    // Only the height is taken; x, y and the width stay zero.
    extent.h = pTemplate->GetIdealHeight();
    pButton->SetExtent(extent);

    pButton->AddEvent(SWGUIEVENT_FOCUS, this, pMethod);
    pButton->AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiScriptSelect::OnScriptSelected));

    return pButton;
}

// Shows the description of the row named by the control's own event argument.
void CSWGuiScriptSelect::OnEnter(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
        SetDescription(static_cast<unsigned int>(m_aScripts.m_pArray[pControl->m_tail.nControlId].m_nDescriptionStrRef));
}

// Commits the selected row's AI state to the creature, then dismisses the panel. The control
// argument is unused: the row comes from the list box's own selection.
void CSWGuiScriptSelect::OnScriptSelected(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_nCreatureId);
    if (pCreature != 0)
    {
        CSWSCreatureStats* pStats = pCreature->m_pStats;
        CSWGuiControl* pRow = m_listBox.GetSelectedControl();
        int nIndex = pRow->m_tail.nControlId;
        pStats->m_aiState = static_cast<unsigned short>(m_aScripts.m_pArray[nIndex].m_nAIState);
    }

    m_pGuiManager->PopModalPanel();
    {
#if KOTOR1  // K1: the panel state bits sit one higher (bit 3 inserted)
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
        m_stateFlags = nFlags;
    }
}

// Accept and the Y button commit the selection, then fall through into the dismiss pair. The
// remaining codes in that range are not handled by this panel.
void CSWGuiScriptSelect::HandleInputEvent(int nType, int nArg)
{
    if (nArg != 0)
    {
        switch (nType)
        {
        case INPUTEVENT_GUI_A:
        case INPUTEVENT_GUI_START:
            {
                CSWSCreature* pCreature =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_nCreatureId);
                if (pCreature != 0)
                {
                    CSWSCreatureStats* pStats = pCreature->m_pStats;
                    CSWGuiControl* pRow = m_listBox.GetSelectedControl();
                    int nIndex = pRow->m_tail.nControlId;
                    pStats->m_aiState =
                        static_cast<unsigned short>(m_aScripts.m_pArray[nIndex].m_nAIState);
                }
            }
            // Falls through to the dismiss arm deliberately; a break here would repeat the sound.
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
            {
#if KOTOR1  // K1: the panel state bits sit one higher (bit 3 inserted)
                unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
                if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                    nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
                unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
                if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                    nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
                m_stateFlags = nFlags;
            }
            break;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(nType, nArg);
}

void CSWGuiScriptSelect::OnPanelAdded()
{
    CSWGuiPanel::OnPanelAdded();
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_PARTY_SCRIPT_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
}

// Shows the description text, re-sizing the description row to fit it.
void CSWGuiScriptSelect::SetDescription(unsigned int nStrRef)
{
    STR_RES strRes;
    g_pTlkTable->Fetch(nStrRef, strRes, CTlkTable::GENDER_MALE);
    m_lblColour.m_text.m_params.SetText(strRes.text);
    m_listBoxDesc.ClearItems();

    CSWGuiExtent extent = m_lblColour.m_extent;
    CSWGuiText* pText = &m_lblColour.m_text;
    if (strRes.text.GetLength() > 0)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblColour.SetExtent(extent);

    // A one-element array rather than a pointer to the control.
    CSWGuiControl* apRows[1] = { &m_lblColour };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_lblColour, 0);
}

// Points the panel at a creature, or at nothing when the object id is the sentinel, then marks the
// row whose AI state matches the creature's current one.
void CSWGuiScriptSelect::SetCreatureID(unsigned long long nCreatureId)
{
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCreatureId);
    if (pCreature == 0)
    {
        m_nCreatureId = OBJECT_INVALID;
        return;
    }
    m_nCreatureId = nCreatureId;

    if (m_aScripts.m_count < 1)
        return;

    unsigned short nAIState = pCreature->m_pStats->m_aiState;
    for (int64_t i = 0; i < m_aScripts.m_count; i++)
    {
        if (m_aScripts.m_pArray[i].m_nAIState == nAIState)
        {
            m_listBox.SetSelectedControl(static_cast<int>(i), 0);
            static_cast<CSWGuiButtonToggle*>(m_listBox.GetControl(static_cast<int>(i))->AsButtonToggle())->bToggledOn = 1;
        }
        else
            static_cast<CSWGuiButtonToggle*>(m_listBox.GetControl(static_cast<int>(i))->AsButtonToggle())->bToggledOn = 0;
    }
}
