#include "k2stdint.h"
#include "strref.h"
#include "swguiingametutorialbox.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swguiexamine.h"
#include "swguimaininterfacepc2.h"
#include "swguidrawable.h"
#include "nwccreature.h"
#include "nwcdoor.h"
#include "nwcplaceable.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "exotlktable.h"
#include "exoresman.h"
#include "Vector.h"
#include "tutorial.h"

extern CTlkTable* g_pTlkTable;

CSWGuiTutorialBox::CSWGuiTutorialBox(CSWGuiManager* pGuiManager)
    : CSWGuiMessageBox(pGuiManager)
{
    m_instigatorId = OBJECT_INVALID;
    m_targetId = OBJECT_INVALID;
    m_type = 0;
    m_bCombatWasPaused = 0;
    m_nTutorialReason = TUTORIAL_NONE;
    m_nMessageIndex = 0;

    m_lblMessage.m_text.m_params.SetBaseFont(CResRef("fnt_d16x16"));
    m_nOpenSoundIndex = GUISOUND_PROMPT;
}

void CSWGuiTutorialBox::OnPanelAdded()
{
    CSWGuiMessageBox::OnPanelAdded();
    m_bCombatWasPaused = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 2, 0);
}

void CSWGuiTutorialBox::MessageBoxPause(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_bCombatWasPaused = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 2, 0);
}

void CSWGuiTutorialBox::SetMessage(uint64_t nStrRef)
{
    CExoString sText = g_pTlkTable->GetSimpleString(nStrRef);
    g_pTlkTable->ParseStr(sText);
    CSWGuiMessageBox::SetMessage(sText);
}

void CSWGuiTutorialBox::HandleInputEvent(int nType, int nValue)
{
    if (nValue)
    {
        switch (nType)
        {
            case INPUTEVENT_GUI_A:
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_START:
            case INPUTEVENT_GUI_BACK:
            case SWGUIEVENT_MSGBOX_OK:
                // Swallow the dismissal while there are still pages left; the last page falls
                // through to the base, which runs the Perform* callback and pops the panel.
                if (SetNextMessage())
                    return;
                break;
        }
    }
    CSWGuiMessageBox::HandleInputEvent(nType, nValue);
}

int CSWGuiTutorialBox::SetNextMessage()
{
    CExoString sColumn;
    CExoString sIcon;
    int nStrRef = 0;

#if KOTOR1  // K1 names the columns Message0, Message1, ...
    sColumn.Format(const_cast<char*>("Message%i"), m_nMessageIndex);
#else
    sColumn.Format(const_cast<char*>("Message_PC%i"), m_nMessageIndex);
#endif
    m_nMessageIndex++;
    if (g_pRules->m_p2DArrays->m_pTutorial->GetINTEntry(m_nTutorialReason, sColumn, &nStrRef))
    {
        // Peek one column further to decide whether the OK button says "Next" or "OK".
        int nPeek;
#if KOTOR1
        sColumn.Format(const_cast<char*>("Message%i"), m_nMessageIndex);
#else
        sColumn.Format(const_cast<char*>("Message_PC%i"), m_nMessageIndex);
#endif
        if (g_pRules->m_p2DArrays->m_pTutorial->GetINTEntry(m_nTutorialReason, sColumn, &nPeek))
            SetOkButtonMessage(STRREF_CONTINUE);
        else
            SetOkButtonMessage(STRREF_OK);

        // The Icon column doubles as a gamepad-glyph name; those fourteen spellings are drawn by
        // the button legend, not as a picture, so only a real resref reaches SetIcon.
        if (g_pRules->m_p2DArrays->m_pTutorial->GetCExoStringEntry(m_nTutorialReason,
                                                                  CExoString("Icon"), &sIcon))
        {
#if KOTOR1  // K1: every Icon value is a picture resref (no gamepad-glyph names to skip)
            SetIcon(CResRef(sIcon));
#else
            if (!sIcon.CompareNoCase("WHITEBUTTON")
                && !sIcon.CompareNoCase("STARTBUTTON")
                && !sIcon.CompareNoCase("XBUTTON")
                && !sIcon.CompareNoCase("YBUTTON")
                && !sIcon.CompareNoCase("Abutton")
                && !sIcon.CompareNoCase("Backbutton")
                && !sIcon.CompareNoCase("Bbutton")
                && !sIcon.CompareNoCase("Blbutton")
                && !sIcon.CompareNoCase("Whbutton")
                && !sIcon.CompareNoCase("Xbutton")
                && !sIcon.CompareNoCase("Ybutton")
                && !sIcon.CompareNoCase("uibut_Start")
                && !sIcon.CompareNoCase("uibut_X")
                && !sIcon.CompareNoCase("uibut_Y"))
            {
                SetIcon(CResRef(sIcon));
            }
#endif
        }
        SetAllowCancel(0);
        SetMessage(static_cast<uint64_t>(nStrRef));
    }
    return nStrRef;
}

int CSWGuiTutorialBox::TutorialReasonIsValid(unsigned char reason)
{
    int nStrRef;
#if KOTOR1  // K1's first column is Message0
    return g_pRules->m_p2DArrays->m_pTutorial->GetINTEntry(reason, CExoString("Message0"),
                                                           &nStrRef) != 0;
#else
    return g_pRules->m_p2DArrays->m_pTutorial->GetINTEntry(reason, CExoString("Message_PC0"),
                                                           &nStrRef) != 0;
#endif
}

int CSWGuiTutorialBox::SetTutorialReason(unsigned char reason)
{
    m_nTutorialReason = reason;
    m_nMessageIndex = 0;
    if (!SetNextMessage())
        return 0;

    void (CSWGuiTutorialBox::*pfnDismiss)(CSWGuiControl*);
    switch (reason)
    {
        case TUTORIAL_USE_COMBAT_FEAT: pfnDismiss = &CSWGuiTutorialBox::PerformCombatFeat; break;
        case TUTORIAL_USE_GRENADE: pfnDismiss = &CSWGuiTutorialBox::PerformUseGrenade; break;
        case TUTORIAL_SET_MINE: pfnDismiss = &CSWGuiTutorialBox::PerformSetMine; break;
        case TUTORIAL_USE_FRIENDLY_FORCE_POWER:
        case TUTORIAL_USE_HOSTILE_FORCE_POWER: pfnDismiss = &CSWGuiTutorialBox::PerformForcePower; break;
        case TUTORIAL_OUT_OF_COMBAT_X_BUTTON: pfnDismiss = &CSWGuiTutorialBox::PerformNonCombatXButton; break;
        case TUTORIAL_IN_COMBAT_X_BUTTON: pfnDismiss = &CSWGuiTutorialBox::PerformCombatXButton; break;
        case TUTORIAL_OUT_OF_COMBAT_Y_BUTTON: pfnDismiss = &CSWGuiTutorialBox::PerformPlayerFlourishWeapons; break;
        case TUTORIAL_IN_COMBAT_Y_BUTTON: pfnDismiss = &CSWGuiTutorialBox::PerformCombatYButton; break;
        case TUTORIAL_BASH: pfnDismiss = &CSWGuiTutorialBox::PerformBash; break;
        case TUTORIAL_ATTACK: pfnDismiss = &CSWGuiTutorialBox::PerformAttack; break;
        case TUTORIAL_RETURN_TO_BASE_BUTTON: pfnDismiss = &CSWGuiTutorialBox::PerformReturnToBase; break;
        default: pfnDismiss = &CSWGuiTutorialBox::MessageBoxUnpause; break;
    }
    SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(pfnDismiss), 0);
    return 1;
}

void CSWGuiTutorialBox::PerformCombatFeat(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_instigatorId);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    if (pCreature)
        pCreature->PerformCombatFeat(static_cast<unsigned short>(m_type), pCreature);
}

void CSWGuiTutorialBox::PerformUseGrenade(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_instigatorId);
    CSWCCreature* pTarget =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_targetId);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    if (pCreature)
        pCreature->PerformUseHostileItem(m_type, pTarget);
}

void CSWGuiTutorialBox::PerformSetMine(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_instigatorId);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    if (pCreature)
        pCreature->PerformUseItem(m_targetId, 0, OBJECT_INVALID, pCreature->m_position);
}

void CSWGuiTutorialBox::PerformForcePower(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_instigatorId);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    if (pCreature)
    {
        // sic: the sub-spell selector is the low byte of the target object id, not a field of its
        // own.
        pCreature->PerformCastSpell(m_type, pCreature->m_objectId, Vector(0.0f, 0.0f, 0.0f),
                                    static_cast<unsigned char>(m_targetId), 0, 0, 0);
    }
}

void CSWGuiTutorialBox::PerformPlayerFlourishWeapons(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    g_pAppManager->m_pClientExoApp->PlayerFlourishWeapons();
}

void CSWGuiTutorialBox::PerformBash(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_instigatorId);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    if (pCreature)
    {
        // m_type discriminates the bashable: non-zero is a door, zero a placeable.
        if (m_type)
        {
            CSWCDoor* pDoor = g_pAppManager->m_pClientExoApp->GetDoorByGameObjectID(m_targetId);
            if (pDoor)
                pDoor->MenuActionBash(0, pCreature);
        }
        else
        {
            CSWCPlaceable* pPlaceable =
                g_pAppManager->m_pClientExoApp->GetPlaceableByGameObjectID(m_targetId);
            if (pPlaceable)
                pPlaceable->BashPlaceable(0, pCreature);
        }
    }
}

void CSWGuiTutorialBox::PerformAttack(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_instigatorId);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    if (pCreature)
    {
        CSWCCreature* pTarget =
            g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_targetId);
        if (pTarget)
            pTarget->ActionMenuAttack(0, pCreature);
    }
}

// The in-game GUI this fetches is deliberately unused.
void CSWGuiTutorialBox::PerformNonCombatXButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    g_pAppManager->m_pClientExoApp->GetInGameGui();
}

void CSWGuiTutorialBox::PerformCombatXButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    g_pAppManager->m_pClientExoApp->GetInGameGui();
}

void CSWGuiTutorialBox::PerformCombatYButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->OnCombatYButton();
}

void CSWGuiTutorialBox::PerformReturnToBase(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
    // Screen panel 6 is the in-game map; its X button is the "return to the Ebon Hawk" prompt.
    static_cast<CSWGuiInGameMap*>(g_pAppManager->m_pClientExoApp->GetInGameGui()
        ->GetGuiInGameScreenPanel(6))->OnXButton(0);
}

void CSWGuiTutorialBox::MessageBoxUnpause(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!m_bCombatWasPaused)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
    m_bCombatWasPaused = 0;
}

void CSWGuiTutorialBox::SetTutorialInstigator(unsigned long long instigatorId,
                                              unsigned long long targetId, unsigned int type)
{
    m_instigatorId = instigatorId;
    m_targetId = targetId;
    m_type = type;
}

