#include "k2stdint.h"
#include "port/sdlapi.h"
#include "swguimaininterfacepc2.h"
#include "swguicontrol.h"
#include "exoresman.h"
#include "swgui.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwscreature.h"
#include "nwscombatround.h"
#include "nwsaiaction.h"
#include "nwsobject.h"
#include "nwsrules.h"
#include "nwspell.h"
#include "nwsitem.h"
#include "swparty.h"
#include "aurgui.h"
#include "aurcamera.h"
#include "swplayercontrol.h"
#include "nwcobject.h"
#include "nwcdoor.h"
#include "nwctrigger.h"
#include "nwcplaceable.h"
#include "nwsplaceable.h"
#include "twodimarray.h"
#include "nwtlktable.h"
#include "swguidrawable.h"
#include "nwseffectlist.h"
#include "nwseffect.h"
#include "nwccreaturestats.h"
#include "nwupdatestats.h"
#include "nwccreature.h"
#include "guiingame.h"
#include "serverexoappinternal.h"
#include "serverexoapp.h"
#include "partytables.h"
#include "clientoptions.h"
#include "nwsmodule.h"
#include "swsareamap.h"
#include "nwcjournal.h"
#include "AURORA/aurmodel.h"
#include "CExoArrayListImpl.h"
#include "swguiingame.h"
#include "exotlktable.h"
#include "port/k2qolapi.h"
#include "port/asllanguage.h"
#include "exoinput.h"
#include "nwscriptconst.h"
#include "itemtype.h"
#include "tutorial.h"
#include "strref.h"

#if KOTOR1  // K1 only: platform queries, declared by hand
int ASLPlat_GetControllerPlugged();
bool ASLPlat_IsControllerPlugged();
// Returns the HUD scale unit: the screen height over 768 on a 4:3 screen, or over 640 on a wider
// one.
inline float AndroidHudScaleFactor() {
#ifdef K2_UI_TOUCH
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    bool is_4_3 = static_cast<float>(g_SDL_mode.w) / static_cast<float>(g_SDL_mode.h) <= 1.4333333f;
    float SCALE_FACTOR = static_cast<float>(g_SDL_mode.h) / (is_4_3 ? 768.0f : 640.0f);
#else
    static bool is_4_3 = static_cast<float>(g_SDL_mode.w) / static_cast<float>(g_SDL_mode.h) <= 1.4333333f;
    static float SCALE_FACTOR = static_cast<float>(g_SDL_mode.h) / (is_4_3 ? 768.0f : 640.0f);
#endif
#else
    // pc / xbox: the PC HUD is a 1024x768 composition laid out to the screen height; the desktop
    // window can resize, so this is recomputed per call.
    float SCALE_FACTOR = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    return SCALE_FACTOR;
}

// Keeps the minimap art in step with its window, so a larger screen frames the same patch (the PC's
// 120 texels) instead of showing more of the map.
#ifdef K2_FIX_ENGINE_BUGS
inline float K1MiniMapZoom() {
    return AndroidHudScaleFactor();
}
#endif
#endif
#ifdef K2_UI_XBOX
#include "ClientExoAppInternal.h"
extern "C" void k2console_tick(void);
#ifdef K2_HOST_BUILD
void k2console_mouse_step(CSWGuiMainInterface* pMain, int nIndex, bool bNext);   // platforms/console
#endif

#ifdef K2_HOST_BUILD
// One entry per GetActionByIndex index (0..5 personal, 6..8 the target columns).
namespace {
struct K2ConsoleScroll {
    float        fTime;
    bool         bFromTop;
    CSWGuiLabel* pOld;
};
K2ConsoleScroll s_k2Scroll[9];
// `maininterface_x` ships every bar control twice, LBH_BORDER%d / LBL_ICON%d / LBH_ARROW%d and a
// smaller `B` twin: the selected box draws the large set, the others the small one. One entry per
// GetActionByIndex index; each holds the box, its icon and its chevron.
struct K2ConsoleBoxSizes {
    bool         bValid;
    CSWGuiExtent aBig[3];
    CSWGuiExtent aSmall[3];
};
K2ConsoleBoxSizes s_k2Sizes[9];
}  // namespace

static void K2ConsoleSizeBox(CSWGuiMainInterfaceAction& widget, bool bSelected) {
    uint64_t nIndex = widget.m_button.m_tail.m_nUserData;
    if (nIndex >= 9 || !s_k2Sizes[nIndex].bValid)
        return;
    if (!bSelected)
        s_k2Scroll[nIndex].fTime = 0.0f;
    const CSWGuiExtent* pRects = bSelected ? s_k2Sizes[nIndex].aBig : s_k2Sizes[nIndex].aSmall;
    CSWGuiControl* apControl[3] = { &widget.m_button, &widget.m_icon, &widget.m_btnUp };
    for (int k = 0; k < 3; k++) {
        const CSWGuiExtent& e = apControl[k]->m_extent;
        if (e.x != pRects[k].x || e.y != pRects[k].y || e.w != pRects[k].w || e.h != pRects[k].h)
            apControl[k]->SetExtent(pRects[k]);
    }
}

void K2ConsoleStartScroll(CSWGuiMainInterface* pMain, int nIndex, bool bNext, const CResRef& sOld,
                          float fOldAlpha) {
    if (static_cast<unsigned int>(nIndex) >= 9)
        return;
    K2ConsoleScroll& s = s_k2Scroll[nIndex];
    s.fTime = 0.0f;
    CSWGuiMainInterfaceAction* pWidget = pMain->GetActionByIndex(nIndex);
    if (!pWidget || !s.pOld || !(pWidget->m_icon.m_border.m_params.m_fillImage != sOld))
        return;
    CSWGuiBorderParams& params = s.pOld->m_border.m_params;
    if (params.m_fillImage != sOld)
        params.SetFillImage(sOld, 0);
    params.m_fAlpha = fOldAlpha;
    s.fTime = 0.1f;
    s.bFromTop = bNext;
}

static void K2ConsoleTickScroll(CSWGuiMainInterface* pMain, float fDelta, int* pVisible) {
    for (int i = 0; i < 9; i++) {
        CSWGuiMainInterfaceAction* pWidget = pMain->GetActionByIndex(i);
        K2ConsoleScroll& s = s_k2Scroll[i];
        pVisible[i] = pWidget->m_icon.m_tail.bVisible;
        if (s.fTime > 0.0f) {
            s.fTime -= fDelta;
            if (s.fTime <= 0.0f)
                s.fTime = 0.0f;
            else
                pWidget->m_icon.m_tail.bVisible = 0;
        }
    }
}

static void K2ConsoleDrawScroll(CSWGuiMainInterface* pMain, float fDelta, const int* pVisible) {
    for (int i = 0; i < 9; i++) {
        CSWGuiButton& icon = pMain->GetActionByIndex(i)->m_icon;
        icon.m_tail.bVisible = pVisible[i];
        K2ConsoleScroll& s = s_k2Scroll[i];
        if (s.fTime <= 0.0f || !pVisible[i])
            continue;
        CSWGuiBorder& cur = icon.m_tail.bActive ? icon.m_borderHilight : icon.m_border;
        CSWGuiExtent sHome = cur.m_extent;
        int nOff = static_cast<int>(s.fTime / 0.1f * static_cast<float>(sHome.h));
        int nNewY = s.bFromTop ? -nOff : nOff;
        int nOldY = s.bFromTop ? sHome.h - nOff : nOff - sHome.h;
        if (AurGUISetupViewport(sHome.x, sHome.y, sHome.w, sHome.h, CSWGuiDrawable::NO_COLOURING,
                                false, 1.0f)) {
            CSWGuiBorder& old = s.pOld->m_border;
            CSWGuiExtent sOldSaved = old.m_extent;
            old.m_extent = CSWGuiExtent(0, nOldY, sHome.w, sHome.h);
            old.Draw(fDelta);
            old.m_extent = sOldSaved;
            cur.m_extent = CSWGuiExtent(0, nNewY, sHome.w, sHome.h);
            cur.Draw(fDelta);
            cur.m_extent = sHome;
            AurGUICloseViewport();
        }
    }
}
#endif  // K2_HOST_BUILD
#endif

extern bool g_bFreeLook;

#if !KOTOR1
unsigned char g_initiatedByController;
#endif

#ifdef K1_UI_XBOX
static void K1ConsoleRetarget(CSWGuiMainInterface* pMain, int bFlag);
#endif

void CSWGuiMainInterface::SetTarget(unsigned long long objId, int flag) {
    K2_UNUSED(flag);
    if (m_targetId != objId) {
        m_targetId = objId;
        m_fReticlePulseTimer = -1.0f;
#ifdef K1_UI_XBOX
        K1ConsoleRetarget(this, flag);
#endif
#ifdef K2_UI_XBOX
        m_targetActionMenu.m_actionIds[0][0] = -1;
        m_targetActionMenu.m_actionIds[1][0] = CSWGuiInterfaceAction::ACTION_ID_RECOVER_MINE;
        m_targetActionMenu.m_actionIds[2][0] = CSWGuiInterfaceAction::ACTION_ID_TALK;
#endif
    }
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::ProcessExtent(CSWGuiExtent& extent) {
    extent.x = static_cast<int>(extent.x * m_scaleX);
    extent.y = static_cast<int>(extent.y * m_scaleY);
    extent.w = static_cast<int>(extent.w * m_scaleX);
    extent.h = static_cast<int>(extent.h * m_scaleY);
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::ResetPartyMemberItemUse() {
    m_bPartyItemUsePending = 0;
    m_nPartyItemUseTarget = -1;
}
#endif // !KOTOR1

#ifdef K2_UI_XBOX
// Grows the caption's backdrop from a fixed bottom edge up to nTop, reading that edge back from the
// control so repeated calls converge.
static void SizeActionDescPlate(CSWGuiLabel& plate, int nTop) {
    CSWGuiExtent extent = plate.m_extent;
    int nBottom = extent.y + extent.h;
    if (nTop >= nBottom)
        return;
    extent.y = nTop;
    extent.h = nBottom - nTop;
    plate.SetExtent(extent);
}
#endif

#if !KOTOR1 && (defined(K2_UI_XBOX) || defined(K2_UI_PC))
// Places the feedback box above the action bar's plate, as wide as the plate, sized to the text at
// the current text size setting.
static void PlaceFeedbackAbovePlate(CSWGuiButton& button, const CSWGuiExtent& plate, int nScreenH) {
    if (plate.w <= 0 || plate.h <= 0)
        return;
    CSWGuiExtent extent = button.m_extent;
    const int nInsetH = extent.h - button.m_text.m_extent.h;
    extent.x = plate.x;
    extent.w = plate.w;
    button.SetExtent(extent);
#ifdef K2_QOL_IMPROVEMENTS
    int nTextH = K2QolFitText(button.m_text, nScreenH / 3);
#else
    int nTextH = button.m_text.GetIdealHeight();
#endif
    extent.h = nTextH + (nInsetH > 0 ? nInsetH : 0);
    extent.y = plate.y - extent.h - 4;
    if (extent.y < 0)
        extent.y = 0;
    button.SetExtent(extent);
}
#endif

#ifdef K1_UI_XBOX
static void K1ConsoleFitCaption(CSWGuiLabel& caption, CSWGuiLabel& plate);
#endif
void CSWGuiMainInterface::SetActionDescription(const CExoString& sText) {
    CSWGuiText* pText = &m_actionDescLabel.m_text;
    pText->m_params.SetText(sText);
#ifdef K1_UI_XBOX
    K1ConsoleFitCaption(m_actionDescLabel, m_actionDescLabel2);
    return;
#endif
#ifdef K2_QOL_IMPROVEMENTS
    // The caption grows up from its fixed bottom; a quarter of the screen at most.
    K2QolFitText(*pText, m_pGuiManager->m_nScreenHeight / 4);
#endif
    CSWGuiExtent extent = m_actionDescLabel.m_extent;
    extent.h = pText->GetIdealHeight();
    extent.y = m_nActionDescBottom - extent.h;
    m_actionDescLabel.SetExtent(extent);
    // On the console the caption is the top of the action bar's plate: the plate's bottom edge is
    // fixed by the layout and only its top follows the caption. m_nActionDescBottom is left untouched.
#ifdef K2_UI_XBOX
    SizeActionDescPlate(m_actionDescLabel2, extent.y);
#else
    m_actionDescLabel2.SetExtent(extent);
#endif
}

// Refreshes the caption once per frame: either the hint DoPersonalAction armed, fading in over the
// first half of its countdown, or the highlighted personal action's own name with a "(n)" count
// suffix when it stacks.
void CSWGuiMainInterface::UpdateActionDescription() {
#ifdef K2_UI_XBOX
    // The manager suspends a panel's draw while a modal is up, so this path can go quiet with the
    // tick in CSWGuiMainInterfaceStatus::Update; k2console_tick rate-limits itself.
    k2console_tick();
#endif
    CExoString sDesc;
    float fAlpha;
    if (m_fChallengeRating > 0.0f) {
        float f = m_fChallengeRating / 5.0f;
        fAlpha = f < 0.5f ? f + f : 1.0f;
        sDesc = g_pAppManager->m_pClientExoApp->GetGUIString(m_nHintStrRef);
    } else {
        fAlpha = 1.0f;
        int nIndex = m_nFocusedAction;
        CSWGuiMainInterfaceAction& widget = m_personalActionWidgets[nIndex];
#if defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
        // console: the caption is the selected box's whatever the pointer is on (leaving a box
        // deactivates its controls, which emptied the caption into box 1's fallback below).
        static_cast<void>(widget);
        if (nIndex >= 0) {
#else
        if ((widget.m_icon.m_tail.bActive || widget.m_btnUp.m_tail.bActive ||
             widget.m_btnDown.m_tail.bActive || widget.m_button.m_tail.bActive) &&
            nIndex >= 0) {
#endif
            int nCount = m_personalActions[nIndex].m_count;
            if (nCount >= 1) {
                int nActionId = m_personalActionIds[nIndex];
                CSWGuiInterfaceAction* pAction = 0;
                if (nActionId != -1) {
                    for (int i = 0; i < nCount; i++)
                        if (m_personalActions[nIndex].m_pArray[i].m_nActionId ==
                            static_cast<unsigned int>(nActionId))
                            pAction = &m_personalActions[nIndex].m_pArray[i];
                }
                if (!pAction)
                    pAction = m_personalActions[nIndex].m_pArray;
                sDesc = pAction->m_sName;
                if (pAction->m_nCount >= 2)
                    sDesc.Format(const_cast<char*>("%s (%d)"), sDesc.CStr(), pAction->m_nCount);
            }
        }
    }
#ifdef K2_UI_XBOX
    // For box 1's caption, the console action bar's first box has no personal action list, so when
    // the personal side produced nothing, fall back to the target menu's first column, picking the
    // entry by the sticky action id as PopulateMenus does.
    if ((sDesc.m_sString == 0 || sDesc.m_sString[0] == 0) && m_fChallengeRating <= 0.0f
#ifdef K2_HOST_BUILD
        && m_nFocusedAction == CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT   // box 1 only
#endif
        ) {
        CExoArrayList<CSWGuiInterfaceAction>& rTarget = m_targetActionMenu.m_actions[0];
        if (rTarget.m_count >= 1) {
            CSWGuiInterfaceAction* pAction = &rTarget.m_pArray[0];
            int nWant = m_targetActionMenu.m_actionRow >= 0
                            ? m_targetActionMenu.m_actionIds[static_cast<int>(m_targetActionMenu.m_actionRow)][0]
                            : -1;
            for (int i = 0; i < rTarget.m_count; i++)
                if (rTarget.m_pArray[i].m_nActionId == static_cast<unsigned int>(nWant)) {
                    pAction = &rTarget.m_pArray[i];
                    break;
                }
            sDesc = pAction->m_sName;
        }
    }
#endif
    SetActionDescription(sDesc);
    m_actionDescLabel.m_text.m_params.m_fAlpha = fAlpha;
}

#if KOTOR1  // K1: a colour per message, and an auto-paused message only re-fits its line
void CSWGuiMainInterface::SetCombatMessage(uint64_t nStrRef) {
    if (m_fCombatMessageTimer != -1.0f)
        return;
    CSWGuiText* pText = &m_lblCombatModeMsg.m_text;
    switch (nStrRef) {
    case STRREF_PLAYER_MOVING:
        if (g_pAppManager->m_pClientExoApp->GetAutoPaused())
            goto fit;
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_GREEN);
        break;
    case STRREF_CLOSING_TO_ATTACK_RANGE:
        if (g_pAppManager->m_pClientExoApp->GetAutoPaused())
            goto fit;
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        break;
    case STRREF_COMBAT_MODE_ALREADY_ENGAGED:
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_GREEN);
        m_fCombatMessageDuration = 2.5f;
        m_fCombatMessageTimer = 2.5f;
        break;
    case STRREF_PATH_TO_TARGET_BLOCKED:
        if (g_pAppManager->m_pClientExoApp->GetAutoPaused())
            goto fit;
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_PURPLE);
        m_fCombatMessageDuration = 2.5f;
        m_fCombatMessageTimer = 2.5f;
        break;
    case STRREF_PLAYER_DEBILITATED:
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_ORANGE);
        break;
    case STRREF_COMBAT_MODE_ENGAGED:
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_RED);
        break;
    default:
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_fCombatMessageDuration = 2.5f;
        m_fCombatMessageTimer = 2.5f;
        break;
    }
    m_combatMessage = nStrRef;
    {
        CExoString sText = g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef);
        m_lblCombatModeMsg.m_text.m_params.SetText(sText);
        m_lblCombatModeMsg.m_text.m_params.m_fAlpha = 1.0f;
    }
fit:
    CSWGuiExtent sExtent = m_lblCombatModeMsg.m_extent;
    sExtent.h = pText->GetIdealHeight();
    m_lblCombatModeMsg.SetExtent(sExtent);
}
#else
void CSWGuiMainInterface::SetCombatMessage(uint64_t nStrRef) {
    if (m_fCombatMessageTimer != -1.0f)
        return;
    switch (nStrRef) {
    case STRREF_PLAYER_MOVING:
        if (g_pAppManager->m_pClientExoApp->GetAutoPaused())
            return;
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
    case STRREF_CLOSING_TO_ATTACK_RANGE:
        if (g_pAppManager->m_pClientExoApp->GetAutoPaused())
            return;
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
    case STRREF_COMBAT_MODE_ALREADY_ENGAGED:
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        m_fCombatMessageDuration = 2.5f;
        m_fCombatMessageTimer = 2.5f;
        break;
    case STRREF_PATH_TO_TARGET_BLOCKED:
        if (g_pAppManager->m_pClientExoApp->GetAutoPaused())
            return;
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        m_fCombatMessageDuration = 2.5f;
        m_fCombatMessageTimer = 2.5f;
        break;
    case STRREF_PLAYER_DEBILITATED:
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
    case STRREF_COMBAT_MODE_ENGAGED:
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
    default:
        m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        m_fCombatMessageDuration = 2.5f;
        m_fCombatMessageTimer = 2.5f;
        break;
    }
    m_combatMessage = nStrRef;
    CExoString sText = g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef);
    m_lblCombatModeMsg.m_text.m_params.SetText(sText);
    m_lblCombatModeMsg.m_text.m_params.m_fAlpha = 1.0f;
}
#endif

void CSWGuiMainInterface::QueueActiveAction() {
}

void CSWGuiMainInterface::OnCharacterRightClicked(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
}

void CSWGuiMainInterface::OnClearOneButtonPressed(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg)
        ClearOneAction();
}

void CSWGuiMainInterface::StartRotation() {
#if KOTOR1 && defined(K1_UI_XBOX)
    // xbox: with two or more members, rotates the portrait slots one place over a quarter second --
    // leader off the bottom, each companion one slot down, the off-screen slot in at the top.
    // Draw runs the countdown; UpdatePortraits remaps the members while it is armed.
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    int nCount = pParty->m_nCharacterCount;
    if (nCount < 2)
        return;
    m_fRotationTimer = 0.25f;
    for (int nI = 0; nI < 4; nI++)
        m_aCharacters[nI].SetupGoto(nI, nCount, m_pGuiManager);
#else
    g_pAppManager->m_pClientExoApp->GetSWParty();
#endif
}

void CSWGuiMainInterface::OnFirstPersonButtonPressed(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    g_bFreeLook = true;
}

void CSWGuiMainInterface::ShowActionQueue(int nParam) {
    if (((m_nInterfaceFlags >> 1) & 1) == nParam) return;
    int nBit = (nParam & 1) << 1;
    m_nInterfaceFlags = static_cast<unsigned char>((m_nInterfaceFlags & ~2) | nBit);
    int nGo;
    if (nParam == 0) nGo = m_lblActionQueue.m_tail.bVisible; else nGo = 1;
#if KOTOR1
    m_lblCombatBg1.m_tail.bVisible = nParam & 1;
    m_lblCombatBg2.m_tail.bVisible = nParam & 1;
    m_lblCombatBg3.m_tail.bVisible = nParam & 1;
    m_lblActionQueue.m_tail.bVisible = nParam & 1;
    m_btnClearOne.m_tail.bVisible = nParam & 1;
    m_btnClearOne2.m_tail.bVisible = nParam & 1;
    static bool s_bHasTouchScreen = HasTouchScreen();
    m_btnClearAll.m_tail.bVisible = nParam != 0 && s_bHasTouchScreen;
#else
    m_lblCombatBg2.m_tail.bVisible = nParam & 1;
    m_lblCombatBg3.m_tail.bVisible = nParam & 1;
    m_actionQueueHilight.m_tail.bVisible = nParam & 1;
    m_lblActionQueue.m_tail.bVisible = nParam & 1;
    m_btnClearOne.m_tail.bVisible = nParam & 1;
    m_btnClearOne2.m_tail.bVisible = nParam & 1;
    m_btnClearAll.m_tail.bVisible = nParam & 1;
#endif
    if (m_bToolTipMessageShown == 0) {
        m_lblCombatModeMsg.m_tail.bVisible = nParam & 1;
        m_lblCombatMsgBg.m_tail.bVisible = nParam & 1;
    }
    m_actionQueueIcons[0].m_tail.bVisible = nParam & 1;
    m_actionQueueIcons[1].m_tail.bVisible = nParam & 1;
    m_actionQueueIcons[2].m_tail.bVisible = nParam & 1;
    m_actionQueueIcons[3].m_tail.bVisible = nParam & 1;
    int nShift = m_interfaceOffset;
    if (nParam == 0) {
        nShift = -nShift;
        m_fCombatMessageTimer = -1.0f;
    }
    if (nGo == 0) return;
    CSWGuiExtent sTmp;
    sTmp = m_miniMap.m_extent;
    sTmp.y += nShift;
    CSWGuiLabel* pMiniMap = &m_miniMap;
    pMiniMap->SetExtent(sTmp);
    sTmp = m_miniMapImage.m_extent;
    sTmp.y += nShift;
    CSWGuiLabel* pImage = &m_miniMapImage;
    pImage->SetExtent(sTmp);
    sTmp = m_altMenuButton.m_extent;
    sTmp.y += nShift;
    CSWGuiButton* pAlt = &m_altMenuButton;
    pAlt->SetExtent(sTmp);
    m_mapViewExtent.y += nShift;
#if KOTOR1
    for (int i = 0; i < 9; i++) {
#else
    for (int i = 0; i < 13; i++) {
#endif
        sTmp = m_statusIcons[i].m_label.m_extent;
        sTmp.y += nShift;
        m_statusIcons[i].m_label.CSWGuiLabel::SetExtent(sTmp);
    }
}

// Maps the pressed HUD menu-bar button onto its in-game GUI page and raises it; when the page
// actually opens, input is handed over to it.
void CSWGuiMainInterface::OnMenuButtonPressed(CSWGuiControl* pControl) {
    if (!pControl->m_tail.nCurrentEventArg)
        return;
    CSWGuiControl* pButton = pControl->AsButton();
#if KOTOR1
    pButton->SetActive(0);
    static_cast<CSWGuiButton*>(pButton)->m_borderHilight.m_params.SetPulsingAlpha(0, 0, 0);
#endif
    int nMenu;
    if (pButton == &m_btnEquip)
        nMenu = 0;
    else if (pButton == &m_btnInventory)
        nMenu = 1;
    else if (pButton == &m_btnCharacter)
        nMenu = 2;
    else if (pButton == &m_btnAbilities)
        nMenu = 3;
    else if (pButton == &m_btnMessages)
        nMenu = 4;
    else if (pButton == &m_btnJournal)
        nMenu = 5;
    else if (pButton == &m_btnMap || pButton == &m_altMenuButton)
        nMenu = 6;
    else
        nMenu = 7;
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowSWInGameGui(nMenu))
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
}

#define HIDDEN __attribute__((visibility("hidden")))

extern unsigned char g_gameMenuCancelledActivateInGameGuiButton;
#if !KOTOR1
extern unsigned char g_gameMenuCancelledActivateInGameGuiButton;
#endif

HIDDEN int  g_nQueuedHelpPage = -1;
HIDDEN int  g_nSelectedAction = -1;
HIDDEN int  g_bQueuedHelpGamepad;
HIDDEN int  g_nQueuedHelpDelay;
#if KOTOR1
static void ShowHelpInternal(int nPage);
extern unsigned char IosActionMenuUp;
#else
static void ShowHelpInternal(int nPage, bool bGamepad);
#endif
#if KOTOR1
static int           g_nHelpPage;
static int           g_nHelpSavedInputClass;
static int           g_nHelpSavedPausedByCombat;
static int           g_nOpenMapCountdown;
HIDDEN int           g_nIphoneLabelGrow = 1;
#else
HIDDEN int           g_nOpenMapCountdown;
#endif
float               g_fGamepadHelpTimer = -1.0f;
#if !KOTOR1
float IOS_PADDING_START_RES = 1024.0f;
float IOS_PADDING_DOUBLE_RES = 1024.0f;
float IOS_ACTION_MENU_SHIFT_X = 30.0f;
float IOS_ACTION_EXPANSION_PADDING = 5.0f;
#else
float IOS_ACTION_MENU_SHIFT_X = -7.0f;
#endif
// The action menu layout's row-span base and its two top-pad terms, which RelayoutActionMenu scales
// the two half-backdrops and the 32 slots by.
HIDDEN float        g_fActionMenuRowSpan = 70.0f;
HIDDEN float        g_fActionMenuTopPad = 30.0f;
HIDDEN float        g_fActionMenuBottomPad = 30.0f;
extern float GetResolutionTextScalar();
extern int g_scaleMipc;
extern int g_bShowIndicator;
extern bool ASLIsDeviceWidescreen();
extern bool g_bAddEdgeOffset;
#if !KOTOR1
bool g_bAddEdgeOffset = true;
#endif
#include "port/libcbase.h"
#include "port/libcstring.h"
const char* GetLocalizationFolderName();
void AurMakeCurrent();
#if KOTOR1
// K1: the pop-up action menu's slot rows, the iPhone pair and the live pair the constructor copies
// them into on an iPhone.
HIDDEN float s_fIosActionMenuIphoneTop = 90.0f;
HIDDEN float s_fIosActionMenuIphonePitch = 50.0f;
HIDDEN float s_fIosActionMenuTop = 30.0f;
HIDDEN float s_fIosActionMenuPitch = 60.0f;
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);

// K1's HUD rectangles are authored for 1024x768 and re-anchored per side: a negative nX pins the
// rectangle to the right edge, a negative nY to the bottom, a positive nY lifts it 64 per step;
// then everything is scaled by the HUD unit.
#if defined(K2P_IOS) && defined(K2_QOL_CONTROLLER)
// Returns the touch HUD's edge margin on an iPhone. KOTOR II's g_nUIEdgeOffset (80 at its 800-wide
// design size), scaled by whole steps, keeps the edge buttons clear of the rounded corners and the
// camera. None on a tablet-shaped screen, matching IsTablet().
static inline int K1PhoneEdgeInset() {
    if (g_SDL_mode.w * 10 < g_SDL_mode.h * 19)
        return 0;
    return 80 * (g_SDL_mode.w / 800);
}
#endif

inline void AndroidExtentShift(CSWGuiExtent& extent, int nX, int nY) {
#ifdef K1_UI_XBOX
    // xbox: one scale for everything (the screen height), each rectangle pinned to the screen half
    // its centre lies in. The anchor arguments are the touch HUD's and do not apply, except (0, -1),
    // the combat-queue group, which keeps its place relative to the screen's centre: beside the
    // action bar at 4:3, centred under the view on a wider screen.
    {
        float fScale = AndroidHudScaleFactor();
        int nLeft = extent.x;
        bool bRight = nLeft + extent.w / 2 > 512;
        if (nX == 0 && nY == -1)
            extent.x = g_SDL_mode.w / 2 + static_cast<int>(fScale * (nLeft - 512));
        else
            extent.x = bRight ? g_SDL_mode.w - static_cast<int>(fScale * (1024 - nLeft)) : static_cast<int>(fScale * nLeft);
        extent.y = static_cast<int>(fScale * extent.y);
        extent.w = static_cast<int>(fScale * extent.w);
        extent.h = static_cast<int>(fScale * extent.h);
    }
#elif !defined(K2_UI_TOUCH)
    // pc / xbox: the touch arithmetic below is authored against a 1136-wide phone layout with a
    // 64-unit top band; the PC layout is a plain 1024x768 composition. The anchor arguments keep
    // their meaning: negative x pins to the right edge, negative y to the bottom, positive to the
    // left or top, and nX == 0 centres across the screen unless the rectangle is unanchored (0, 0:
    // the target banner and its menu, which sit over the target as it moves).
    float fScale = AndroidHudScaleFactor();
    int nX0 = extent.x;
    int nY0 = extent.y;
    extent.w = static_cast<int>(fScale * extent.w);
    extent.h = static_cast<int>(fScale * extent.h);
    if (nX < 0)
        extent.x = g_SDL_mode.w - static_cast<int>(fScale * (1024 - nX0));
    else if (nX == 0 && nY != 0)
        extent.x = g_SDL_mode.w / 2 + static_cast<int>(fScale * (nX0 - 512));
    else
        extent.x = static_cast<int>(fScale * nX0);
    if (nY < 0)
        extent.y = g_SDL_mode.h - static_cast<int>(fScale * (768 - nY0));
    else
        extent.y = static_cast<int>(fScale * nY0);
#else
    if (nX < 0)
        extent.x -= (g_SDL_mode.w - 1136) * nX;
    if (nY > 0)
        extent.y -= nY * 64;
    else if (nY < 0)
        extent.y -= (g_SDL_mode.h - 704) * nY;
    extent.x = static_cast<int>(AndroidHudScaleFactor() * extent.x);
    extent.y = static_cast<int>(AndroidHudScaleFactor() * extent.y);
    extent.w = static_cast<int>(AndroidHudScaleFactor() * extent.w);
    extent.h = static_cast<int>(AndroidHudScaleFactor() * extent.h);
    if (nX < 0)
        extent.x = static_cast<int>((1.0f - AndroidHudScaleFactor()) * g_SDL_mode.w + extent.x);
    if (nY < 0)
        extent.y = static_cast<int>((1.0f - AndroidHudScaleFactor()) * g_SDL_mode.h + extent.y);
#if defined(K2P_IOS) && defined(K2_QOL_CONTROLLER)
    if (nX != 0)
        extent.x += nX < 0 ? -K1PhoneEdgeInset() : K1PhoneEdgeInset();
#endif
#endif
}

static inline void ShiftControlExtent(CSWGuiControl& control, int nX, int nY) {
    CSWGuiExtent extent = control.m_extent;
    AndroidExtentShift(extent, nX, nY);
    control.SetExtent(extent);
}
#endif

#ifdef K1_UI_XBOX
// xbox: the console action bar's state. Six boxes, one per personal list, each with a large rectangle
// set for the selected box and a small one for the rest. The selected box and the entry shown in each
// box are remembered per target type, as the Switch's PopulateMenus (0x398e80) and SetTarget
// (0x39be58) keep them: a new target carries the selected action over to whichever box offers it
// (box 1 if none does), unless the player is walking; a door or container's box 2 comes up on
// Security and a hostile's box 1 on Attack; box 5 shows the same power for every kind.
namespace {
struct K1ConsoleBox {
    CSWGuiExtent aBig[3];
    CSWGuiExtent aSmall[3];
};
K1ConsoleBox s_k1Boxes[6];
int s_k1Ctx = 0;
int s_k1Sel[5] = { 0, 0, 0, 0, 0 };
int s_k1Sticky[5][6];
bool s_k1StickyInit = false;
int s_k1Pending = -1;   // the action a new target re-selects (the Switch's +0x178)
CSWGuiExtent s_k1Caption;
CSWGuiExtent s_k1CaptionPlate;
struct K1ConsoleScroll {
    float        fTime;
    bool         bFromTop;
    CSWGuiLabel* pOld;
};
K1ConsoleScroll s_k1Scroll[6];
}  // namespace

static void K1ConsoleInitSticky()
{
    if (s_k1StickyInit)
        return;
    for (int c = 0; c < 5; c++)
        for (int i = 0; i < 6; i++)
            s_k1Sticky[c][i] = -1;
    s_k1StickyInit = true;
}

// The Switch's SetTarget arm for a new target: park the selected box's action for PopulateMenus to
// re-find (not when the target changes under a walking player, nor for a flagged call), then re-arm
// the entries it resets -- Security in a door or container's box 2, Attack in a hostile's box 1 --
// and give every kind the current kind's box 5 entry.
static void K1ConsoleRetarget(CSWGuiMainInterface* pMain, int bFlag)
{
    K1ConsoleInitSticky();
    const int nSel = s_k1Sel[s_k1Ctx];
    if (!bFlag && !g_pAppManager->m_pClientExoApp->GetPlayerMovementInput() && nSel >= 0 && nSel < 6)
        s_k1Pending = pMain->m_personalActionIds[nSel];
    int& rDoorBox2 = s_k1Ctx == CSWCCreature::INTERFACE_TARGET_OPENABLE ? pMain->m_personalActionIds[1] : s_k1Sticky[CSWCCreature::INTERFACE_TARGET_OPENABLE][1];
    rDoorBox2 = CSWGuiInterfaceAction::ACTION_ID_SECURITY;
    int& rHostileBox1 = s_k1Ctx == CSWCCreature::INTERFACE_TARGET_HOSTILE ? pMain->m_personalActionIds[0] : s_k1Sticky[CSWCCreature::INTERFACE_TARGET_HOSTILE][0];
    rHostileBox1 = CSWGuiInterfaceAction::ACTION_ID_ATTACK;
    for (int c = 0; c < 5; c++)
        if (c != s_k1Ctx)
            s_k1Sticky[c][4] = pMain->m_personalActionIds[4];
}

static void K1ConsoleSaveCaption(const CSWGuiExtent& caption, const CSWGuiExtent& plate) {
    s_k1Caption = caption;
    s_k1CaptionPlate = plate;
}

static void K1ConsoleRestoreCaption(CSWGuiControl& caption, CSWGuiControl& plate) {
    caption.SetExtent(s_k1Caption);
    plate.SetExtent(s_k1CaptionPlate);
}

// Grows the caption plate up from its fixed bottom to fit the caption text; the plate keeps its own
// art size and Draw clips it at the bottom line, where the bottom cap takes over.
static void K1ConsoleFitCaption(CSWGuiLabel& caption, CSWGuiLabel& plate) {
    if (s_k1CaptionPlate.h <= 0)
        return;
    int nIdeal = caption.m_text.GetIdealHeight();
    int nBottom = s_k1CaptionPlate.y + s_k1CaptionPlate.h;
    int nTop = nBottom - (s_k1CaptionPlate.h - s_k1Caption.h) - nIdeal;
    CSWGuiExtent sPlate(s_k1CaptionPlate.x, nTop, s_k1CaptionPlate.w,
                        nBottom - nTop > s_k1CaptionPlate.h ? nBottom - nTop : s_k1CaptionPlate.h);
    CSWGuiExtent sCaption(s_k1Caption.x, nTop + (s_k1Caption.y - s_k1CaptionPlate.y),
                          s_k1Caption.w, nIdeal);
    const CSWGuiExtent& e = plate.m_extent;
    if (e.x != sPlate.x || e.y != sPlate.y || e.w != sPlate.w || e.h != sPlate.h)
        plate.SetExtent(sPlate);
    const CSWGuiExtent& c = caption.m_extent;
    if (c.x != sCaption.x || c.y != sCaption.y || c.w != sCaption.w || c.h != sCaption.h)
        caption.SetExtent(sCaption);
}

// Draws the plate's border on its own, cut off where the bottom cap begins.
static void K1ConsoleDrawPlate(CSWGuiLabel& plate, const CSWGuiControl& cap, float fDelta) {
    int nBottom = s_k1CaptionPlate.y + s_k1CaptionPlate.h;
    if (cap.m_extent.h > 0 && cap.m_extent.y >= nBottom)
        nBottom = cap.m_extent.y + 1;
    CSWGuiExtent sSaved = plate.m_border.m_extent;
    if (nBottom <= sSaved.y)
        return;
    if (AurGUISetupViewport(sSaved.x, sSaved.y, sSaved.w, nBottom - sSaved.y,
                            CSWGuiDrawable::NO_COLOURING, false, 1.0f)) {
        plate.m_border.m_extent = CSWGuiExtent(0, 0, sSaved.w, sSaved.h);
        plate.m_border.Draw(fDelta);
        plate.m_border.m_extent = sSaved;
        AurGUICloseViewport();
    }
}

// Sizes the selected box large and the rest small; an empty box takes the dark frame. The arrow
// pair is set up by SetupActionWidget.
static void K1ConsoleSetBox(CSWGuiMainInterfaceAction& widget, int nBox, bool bSelected,
                            bool bEmpty) {
    if (!bSelected)
        s_k1Scroll[nBox].fTime = 0.0f;
    const CSWGuiExtent* pRects = bSelected ? s_k1Boxes[nBox].aBig : s_k1Boxes[nBox].aSmall;
    CSWGuiControl* apControl[3] = { &widget.m_button, &widget.m_icon, &widget.m_btnUp };
    for (int k = 0; k < 3; k++) {
        const CSWGuiExtent& e = apControl[k]->m_extent;
        if (e.x != pRects[k].x || e.y != pRects[k].y || e.w != pRects[k].w || e.h != pRects[k].h)
            apControl[k]->SetExtent(pRects[k]);
    }
    CResRef sFrame(bEmpty ? "lbl_mibox03" : "lbl_mibox01");
    if (widget.m_button.m_border.m_params.m_fillImage != sFrame)
        widget.m_button.m_border.m_params.SetFillImage(sFrame, 0);
    widget.m_button.SetActive(bSelected && !bEmpty);
}

// Arms the entry scroll once the box shows its new entry. No scroll when the icon is unchanged.
static void K1ConsoleStartScroll(CSWGuiMainInterfaceAction& widget, int nBox, bool bNext,
                                 const CResRef& sOld, float fOldAlpha) {
    K1ConsoleScroll& s = s_k1Scroll[nBox];
    s.fTime = 0.0f;
    if (!s.pOld || !(widget.m_icon.m_border.m_params.m_fillImage != sOld))
        return;
    CSWGuiBorderParams& params = s.pOld->m_border.m_params;
    if (params.m_fillImage != sOld)
        params.SetFillImage(sOld, 0);
    params.m_flags.nFillStyle = 2;
    params.m_fAlpha = fOldAlpha;
    s.fTime = 0.1f;
    s.bFromTop = bNext;
}

// Ticks the scroll before the panel draws, and keeps a scrolling icon out of the panel's pass.
static void K1ConsoleTickScroll(CSWGuiMainInterfaceAction* pWidgets, float fDelta, int* pVisible) {
    for (int i = 0; i < 6; i++) {
        K1ConsoleScroll& s = s_k1Scroll[i];
        pVisible[i] = pWidgets[i].m_icon.m_tail.bVisible;
        if (s.fTime > 0.0f) {
            s.fTime -= fDelta;
            if (s.fTime <= 0.0f)
                s.fTime = 0.0f;
            else
                pWidgets[i].m_icon.m_tail.bVisible = 0;
        }
    }
}

// Draws both icons inside a viewport on the icon's rect, the new one sliding in from above or below
// and the old one a full icon-height behind it.
static void K1ConsoleDrawScroll(CSWGuiMainInterfaceAction* pWidgets, float fDelta,
                                const int* pVisible) {
    for (int i = 0; i < 6; i++) {
        CSWGuiButton& icon = pWidgets[i].m_icon;
        icon.m_tail.bVisible = pVisible[i];
        K1ConsoleScroll& s = s_k1Scroll[i];
        if (s.fTime <= 0.0f || !pVisible[i])
            continue;
        CSWGuiExtent sHome = icon.m_border.m_extent;
        int nOff = static_cast<int>(s.fTime / 0.1f * static_cast<float>(sHome.h));
        int nNewY = s.bFromTop ? -nOff : nOff;
        int nOldY = s.bFromTop ? sHome.h - nOff : nOff - sHome.h;
        if (AurGUISetupViewport(sHome.x, sHome.y, sHome.w, sHome.h, CSWGuiDrawable::NO_COLOURING,
                                false, 1.0f)) {
            CSWGuiBorder& old = s.pOld->m_border;
            CSWGuiExtent sOldSaved = old.m_extent;
            old.m_extent = CSWGuiExtent(0, nOldY, sHome.w, sHome.h);
            old.Draw(fDelta);
            old.m_extent = sOldSaved;
            icon.m_border.m_extent = CSWGuiExtent(0, nNewY, sHome.w, sHome.h);
            icon.m_border.Draw(fDelta);
            icon.m_border.m_extent = sHome;
            AurGUICloseViewport();
        }
    }
}
#endif

// Chains to the base panel's OnPanelAdded, then releases the mouse so the HUD can be clicked.
#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::OnPanelAdded() {
    CSWGuiPanel::OnPanelAdded();
    EnableMouseCameraRotateToggle(false);
}
#endif // !KOTOR1

#if KOTOR1  // K1: the gate closes the pop-up menu instead of firing it, and the toggles show/hide without the _ios skins
void CSWGuiMainInterface::Update(float fDelta) {
    if (m_pGuiManager->m_modalPanels.m_count == 0) {
        if (g_nQueuedHelpPage >= 0) {
            ShowHelpInternal(g_nQueuedHelpPage);
            g_nQueuedHelpPage = -1;
        }
    }
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState) {
        if (g_pGuiMan->PanelExists(&m_actionMenuIos)) {
            g_pGuiMan->RemovePanel(&m_actionMenuIos);
            IosActionMenuUp = 0;
            if (!m_actionMenuIos.m_bWasPausedByCombat)
                g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
        }
    }
    if (g_nOpenMapCountdown != 0 && --g_nOpenMapCountdown == 0) {
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowSWInGameGui(6))
            g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    }
    for (int i = m_floatyTexts.m_count - 1; i >= 0; i--) {
        CSWGuiFloatyText* pFloaty = m_floatyTexts.m_pArray[i];
        if (pFloaty)
            pFloaty->Update(fDelta);
    }
    if (m_fCombatMessageTimer != -1.0f) {
        m_fCombatMessageTimer -= fDelta;
        if (m_fCombatMessageTimer <= 0.0f) {
            m_fCombatMessageTimer = -1.0f;
            CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
            int nPlayerIdx = pApp->GetSWParty()->GetIndex(pApp->GetPlayerCreatureId());
            if (nPlayerIdx >= 0)
                pApp->GetSWParty()->m_members[nPlayerIdx].SetCombatMessage(STRREF_COMBAT_MODE_ENGAGED);
        } else {
            float f = m_fCombatMessageTimer / m_fCombatMessageDuration;
            m_lblCombatModeMsg.m_text.m_params.m_fAlpha = f < 0.5f ? f + f : 1.0f;
        }
    }
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    m_tbSolo.m_tail.bVisible = g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nNumPartyMembers > 0;
    m_tbSolo.bToggledOn = g_pAppManager->m_pServerExoApp->GetPartyTable()->m_soloMode & 1;
    int bStealthOn;
    if (pPlayer) {
        m_tbStealth.m_tail.bVisible = pPlayer->IsStealthCapable() & 1;
        bStealthOn = pPlayer->m_nStealthState & 1;
    } else {
        m_tbStealth.m_tail.bVisible = 0;
        bStealthOn = 0;
    }
    m_tbStealth.bToggledOn = bStealthOn;
    m_tbPause.bToggledOn = g_pAppManager->m_pClientExoApp->GetPausedByCombat() & 1;
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    const bool bShowMenu = pOptions == 0 || !pOptions->m_bHideInGameGui;
    m_btnEquip.m_tail.bVisible = bShowMenu;
    m_btnInventory.m_tail.bVisible = bShowMenu;
    m_btnCharacter.m_tail.bVisible = bShowMenu;
    m_btnAbilities.m_tail.bVisible = bShowMenu;
    m_btnMessages.m_tail.bVisible = bShowMenu;
    m_btnJournal.m_tail.bVisible = bShowMenu;
    m_btnMap.m_tail.bVisible = bShowMenu;
    m_btnOptions.m_tail.bVisible = bShowMenu;
    m_lblMenuBg.m_tail.bVisible = bShowMenu;
    m_btnQuickSave.m_tail.bVisible = bShowMenu;
    m_btnCornerHelp.m_tail.bVisible = bShowMenu;
    m_btnCornerPlate.m_tail.bVisible = bShowMenu;
}
#else
void CSWGuiMainInterface::Update(float fDelta) {
    if (m_pGuiManager->m_modalPanels.m_count == 0) {
        if (g_nQueuedHelpPage >= 0) {
            if (g_nQueuedHelpDelay <= 0) {
                ShowHelpInternal(g_nQueuedHelpPage, g_bQueuedHelpGamepad != 0);
                g_nQueuedHelpPage = -1;
            }
            g_nQueuedHelpDelay = g_nQueuedHelpDelay - 1;
        }
    }
    if (g_fGamepadHelpTimer > 0.0f) {
        g_fGamepadHelpTimer -= fDelta;
        if (g_fGamepadHelpTimer <= 0.0f)
            HideHelp(0);
    }
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    int nGlobalDialogState = pInGameGui->m_nGlobalDialogState;
    g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (g_pGuiMan->PanelExists(&m_actionMenuIos)) {
        if (m_nActionMenuGraceFrames > 0) {
            m_nActionMenuGraceFrames = m_nActionMenuGraceFrames - 1;
            int nSlot = GetActiveActionIdx(g_nSelectedAction);
            m_pGuiManager->HandleMouseMove(m_actionSlots[nSlot].m_extent.x + 1,
                                           m_actionSlots[nSlot].m_extent.y + 1);
        }
        CSWGuiControl* pCaptured =
            g_pGuiMan->m_pMouseCaptureObject
                ? g_pGuiMan->m_pMouseCaptureObject->AsSWGuiControl()
                : 0;
        bool bFound = pCaptured == 0 || (
                pCaptured->m_pContainer != &m_actionMenuIos
                && pCaptured != &m_personalActionWidgets[0].m_btnDown
                && pCaptured != &m_personalActionWidgets[0].m_btnUp
                && pCaptured != &m_personalActionWidgets[0].m_button
                && pCaptured != &m_personalActionWidgets[0].m_icon
                && pCaptured != &m_personalActionWidgets[1].m_btnDown
                && pCaptured != &m_personalActionWidgets[1].m_btnUp
                && pCaptured != &m_personalActionWidgets[1].m_button
                && pCaptured != &m_personalActionWidgets[1].m_icon
                && pCaptured != &m_personalActionWidgets[2].m_btnDown
                && pCaptured != &m_personalActionWidgets[2].m_btnUp
                && pCaptured != &m_personalActionWidgets[2].m_button
                && pCaptured != &m_personalActionWidgets[2].m_icon
                && pCaptured != &m_personalActionWidgets[3].m_btnDown
                && pCaptured != &m_personalActionWidgets[3].m_btnUp
                && pCaptured != &m_personalActionWidgets[3].m_button
                && pCaptured != &m_personalActionWidgets[3].m_icon
                && pCaptured != &m_personalActionWidgets[4].m_btnDown
                && pCaptured != &m_personalActionWidgets[4].m_btnUp
                && pCaptured != &m_personalActionWidgets[4].m_button
                && pCaptured != &m_personalActionWidgets[4].m_icon
                && pCaptured != &m_personalActionWidgets[5].m_btnDown
                && pCaptured != &m_personalActionWidgets[5].m_btnUp
                && pCaptured != &m_personalActionWidgets[5].m_button
                && pCaptured != &m_personalActionWidgets[5].m_icon
                && pCaptured != &m_personalActionWidgets[6].m_btnDown
                && pCaptured != &m_personalActionWidgets[6].m_btnUp
                && pCaptured != &m_personalActionWidgets[6].m_button
                && pCaptured != &m_personalActionWidgets[6].m_icon
                && pCaptured != &m_targetActionMenu.m_widgets[0].m_btnDown
                && pCaptured != &m_targetActionMenu.m_widgets[0].m_btnUp
                && pCaptured != &m_targetActionMenu.m_widgets[0].m_button
                && pCaptured != &m_targetActionMenu.m_widgets[0].m_icon
                && pCaptured != &m_targetActionMenu.m_widgets[1].m_btnDown
                && pCaptured != &m_targetActionMenu.m_widgets[1].m_btnUp
                && pCaptured != &m_targetActionMenu.m_widgets[1].m_button
                && pCaptured != &m_targetActionMenu.m_widgets[1].m_icon
                && pCaptured != &m_targetActionMenu.m_widgets[2].m_btnDown
                && pCaptured != &m_targetActionMenu.m_widgets[2].m_btnUp
                && pCaptured != &m_targetActionMenu.m_widgets[2].m_button
                && pCaptured != &m_targetActionMenu.m_widgets[2].m_icon
                );

        if ((bFound && g_bLeftMouseDown) || nGlobalDialogState) {
            m_bClearSelectionOnClose = 1;
            HandleButtonActionX(0);
        }
    }
    if (g_nOpenMapCountdown != 0 && --g_nOpenMapCountdown == 0) {
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowSWInGameGui(6))
            g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    }
    for (int i = m_floatyTexts.m_count - 1; i >= 0; i--) {
        CSWGuiFloatyText* pFloaty = m_floatyTexts.m_pArray[i];
        if (pFloaty)
            pFloaty->Update(fDelta);
    }
    if (m_fCombatMessageTimer != -1.0f) {
        m_fCombatMessageTimer -= fDelta;
        if (m_fCombatMessageTimer <= 0.0f) {
            m_fCombatMessageTimer = -1.0f;
            CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
            int nPlayerIdx = pApp->GetSWParty()->GetIndex(pApp->GetPlayerCreatureId());
            if (nPlayerIdx >= 0)
                pApp->GetSWParty()->m_members[nPlayerIdx].SetCombatMessage(STRREF_COMBAT_MODE_ENGAGED);
        } else {
            float f = m_fCombatMessageTimer / m_fCombatMessageDuration;
            m_lblCombatModeMsg.m_text.m_params.m_fAlpha = f < 0.5f ? f + f : 1.0f;
        }
    }
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
#ifdef K2_UI_PC
    // PC keeps the layout's own toggle art and swaps no fill images, and it has no flourish button.
    // The menu bar -- eight buttons and LBL_MENUBG -- shows unless the "hide in-game GUI" option is
    // set, on a touch screen or not.
    m_tbSolo.m_tail.bVisible = g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nNumPartyMembers > 0;
    m_tbSolo.bToggledOn = g_pAppManager->m_pServerExoApp->GetPartyTable()->m_soloMode & 1;
    if (pPlayer) {
        m_tbStealth.m_tail.bVisible = pPlayer->IsStealthCapable() & 1;
        m_tbStealth.bToggledOn = pPlayer->m_nStealthState & 1;
        m_btnSwapWeapons.m_tail.bVisible = pPlayer->CanSwapWeapons() & 1;
    } else {
        m_tbStealth.m_tail.bVisible = 0;
        m_tbStealth.bToggledOn = 0;
        m_btnSwapWeapons.m_tail.bVisible = 0;
    }
    m_tbPause.bToggledOn = g_pAppManager->m_pClientExoApp->GetPausedByCombat() & 1;
    CClientOptions* pPcOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    const bool bShowMenu = pPcOptions == 0 || !pPcOptions->m_bHideInGameGui;
    m_btnEquip.m_tail.bVisible = bShowMenu;
    m_btnInventory.m_tail.bVisible = bShowMenu;
    m_btnCharacter.m_tail.bVisible = bShowMenu;
    m_btnAbilities.m_tail.bVisible = bShowMenu;
    m_btnMessages.m_tail.bVisible = bShowMenu;
    m_btnJournal.m_tail.bVisible = bShowMenu;
    m_btnMap.m_tail.bVisible = bShowMenu;
    m_btnOptions.m_tail.bVisible = bShowMenu;
    m_lblMenuBg.m_tail.bVisible = bShowMenu;
#else
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nNumPartyMembers >= 1) {
        m_tbSolo.m_border.m_params.SetFillImage(CResRef("i_solo_ios"), 0);
        m_tbSolo.m_borderHilight.m_params.SetFillImage(CResRef("i_solo1_ios"), 0);
        m_tbSolo.bToggledOn =
                       (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_soloMode & 1);
    } else {
        m_tbSolo.m_border.m_params.SetFillImage(CResRef("i_solo_off"), 0);
        m_tbSolo.m_borderHilight.m_params.SetFillImage(CResRef("i_solo_off"), 0);
        m_tbSolo.bToggledOn = 0;
    }
    if (pPlayer && pPlayer->IsStealthCapable()) {
        m_tbStealth.m_border.m_params.SetFillImage(CResRef("i_mistealth_ios"), 0);
        m_tbStealth.m_borderHilight.m_params.SetFillImage(CResRef("i_mistealth1_ios"), 0);
        m_tbStealth.bToggledOn = pPlayer->m_nStealthState & 1;
    } else {
        m_tbStealth.m_border.m_params.SetFillImage(CResRef("i_stealth_off"), 0);
        m_tbStealth.m_borderHilight.m_params.SetFillImage(CResRef("i_stealth_off"), 0);
        m_tbStealth.bToggledOn = 0;
    }
    if (pPlayer && pPlayer->CanSwapWeapons()) {
        m_btnSwapWeapons.m_border.m_params.SetFillImage(CResRef("i_swapwpn_ios"), 0);
        m_btnSwapWeapons.m_borderHilight.m_params.SetFillImage(CResRef("i_swapwpn_ios2"), 0);
    } else {
        m_btnSwapWeapons.m_border.m_params.SetFillImage(CResRef("i_swapwpn_off"), 0);
        m_btnSwapWeapons.m_borderHilight.m_params.SetFillImage(CResRef("i_swapwpn_off"), 0);
    }
    if (g_pAppManager->m_pClientExoApp->CanFlourish() & 1) {
        m_btnFlourish.m_border.m_params.SetFillImage(CResRef("i_flrish_ios"), 0);
        m_btnFlourish.m_borderHilight.m_params.SetFillImage(CResRef("i_flrish1_ios"), 0);
    } else {
        m_btnFlourish.m_border.m_params.SetFillImage(CResRef("i_flrish_off"), 0);
        m_btnFlourish.m_borderHilight.m_params.SetFillImage(CResRef("i_flrish_off"), 0);
    }
    m_tbPause.bToggledOn = g_pAppManager->m_pClientExoApp->GetPausedByCombat() & 1;
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    int nVis;
    if (HasTouchScreen() & 1)
        nVis = pOptions->m_bHideInGameGui ? 0 : 2;
    else
        nVis = 0;
    m_btnEquip.m_tail.bVisible = nVis != 0;
    m_btnInventory.m_tail.bVisible = nVis != 0;
    m_btnCharacter.m_tail.bVisible = nVis != 0;
    m_btnAbilities.m_tail.bVisible = nVis != 0;
    m_btnMessages.m_tail.bVisible = nVis != 0;
    m_btnJournal.m_tail.bVisible = nVis != 0;
    m_btnMap.m_tail.bVisible = nVis != 0;
    m_btnOptions.m_tail.bVisible = nVis != 0;
    m_lblMenuBg.m_tail.bVisible = nVis != 0;
    m_btnCamera.m_tail.bVisible = nVis != 0;
    m_btnHelp.m_tail.bVisible = nVis != 0;
    m_btnSave.m_tail.bVisible = nVis != 0;
    m_lblMenuKeyBg.m_tail.bVisible = nVis != 0;
#endif
}
#endif

#if !KOTOR1 && defined(K2_QOL_IMPROVEMENTS)
// Converts a reticle size in the HUD's layout units (g_nGUIHeight lines, the height every panel is
// scaled from) to pixels.
static int K2QolReticlePixels(CSWGuiManager* pManager, int nSize)
{
    return static_cast<int>(static_cast<float>(nSize) * static_cast<float>(pManager->m_nScreenHeight) / static_cast<float>(g_nGUIHeight));
}
#endif
extern int g_bMouseCameraRotateToggle;
void CSWGuiMainInterface::UpdateStickyTarget() {
#if KOTOR1
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
#endif
    int nMode = g_bMouseCameraRotateToggle;
    CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_mouseOverTargetId);
    CSWCObject* pObject = 0;
    if (pGameObject)
        pObject = static_cast<CSWCObject*>(pGameObject->AsSWCObject());
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_bGameplay1)
        nMode = !nMode;
    if (pObject != 0 && nMode != 0) {
        m_mouseOverTargetId = 0;
        return;
    }
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    if (pObject == 0 || pPlayer == 0) {
        m_reticleLabel.m_tail.bVisible = false;
        return;
    }
    CSWGuiExtent sExtent = m_reticleLabel.m_extent;
#if KOTOR1  // K1: the point starts at the origin and the result is a byte
    Vector vPos;
    vPos.x = 0.0f;
    vPos.y = 0.0f;
    vPos.z = 0.0f;
    unsigned char nRet = pObject->GetReticlePoint(vPos);
#else
    Vector vPos;
    int nRet = pObject->GetReticlePoint(vPos);
#endif
    if (nRet == 0)
        return;
    float fDX = pPlayer->m_position.x - pObject->m_position.x;
    float fDY = pPlayer->m_position.y - pObject->m_position.y;
    float fDZ = pPlayer->m_position.z - pObject->m_position.z;
    float fDist2 = fDX * fDX + fDY * fDY + fDZ * fDZ;
    int nSize;
    int nFriendly;
    if (pObject->AsSWCCreature() != 0) {
        if (fDist2 >= 900.0f)
            nSize = 16;
        else {
            nSize = 64;
            if (fDist2 > 25.0f)
                nSize = 64 - static_cast<int>((__builtin_sqrtf(fDist2) - 5.0f) / 25.0f * 48.0f);
        }
        // Creatures skip the door/trigger/placeable ladder entirely.
        nFriendly = pObject->IsHostile() == 0;
    } else {
#if KOTOR1  // K1: the non-creature reticle shrinks over the creature range (30 m, not 10 m)
        if (fDist2 >= 900.0f)
            nSize = 32;
        else {
            nSize = 64;
            if (fDist2 > 25.0f)
                nSize = 64 - static_cast<int>((__builtin_sqrtf(fDist2) - 5.0f) / 25.0f * 32.0f);
        }
#else
        if (fDist2 >= 100.0f)
            nSize = 32;
        else {
            nSize = 64;
            if (fDist2 > 25.0f)
                nSize = 64 - static_cast<int>((__builtin_sqrtf(fDist2) - 5.0f) / 5.0f * 32.0f);
        }
#endif
        if (pObject->AsSWCDoor() != 0 && pObject->AsSWCDoor()->m_bHilighted != 0 &&
            pObject->AsSWCDoor()->m_bHostileFaction != 0)
            nFriendly = 0;
        else if (pObject->AsSWCTrigger() != 0)
            nFriendly = pObject->IsHostile() == 0;
        else if (pObject->AsSWCPlaceable() != 0 && pObject->AsSWCPlaceable()->m_bHilighted != 0 &&
                 pObject->AsSWCPlaceable()->m_bHostileFaction != 0)
            nFriendly = 0;
        else if (pObject->AsSWCPlaceable() != 0 && pObject->AsSWCPlaceable()->IsHostile() != 0) {
            CSWSObject* pTargetSrv = pObject->AsSWCPlaceable()->GetServerObject();
            CSWSObject* pPlayerSrv = pPlayer->GetServerObject();
            nFriendly = pTargetSrv->GetReputation(pPlayerSrv->m_objectId, 0) > 10;
        }
        else
            nFriendly = 1;
    }
#if !KOTOR1 && defined(K2_QOL_IMPROVEMENTS)
    nSize = K2QolReticlePixels(m_pGuiManager, nSize);
#endif
    if (nRet == 2 ||
        vPos.x < m_lblArrowMargin.m_extent.x + 32 ||
        vPos.x > m_lblArrowMargin.m_extent.x + m_lblArrowMargin.m_extent.w - 32 ||
        vPos.y < m_lblArrowMargin.m_extent.y + 32 ||
        vPos.y > m_lblArrowMargin.m_extent.y + m_lblArrowMargin.m_extent.h - 32) {
#if KOTOR1
        sExtent.w = sExtent.h = static_cast<int>(s_fScale * nSize);
#else
        sExtent.w = sExtent.h = nSize;
#endif
        m_reticleLabel.m_tail.bVisible = false;
        return;
    }
#if KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
    int nHalf = static_cast<int>(s_fScale * nSize) / 2;
#else
    int nHalf = nSize / 2;
#endif
    sExtent.y = static_cast<int>(vPos.y - nHalf);
    sExtent.x = static_cast<int>(vPos.x - nHalf);
#if KOTOR1
    sExtent.w = sExtent.h = static_cast<int>(s_fScale * nSize);
#else
    sExtent.w = sExtent.h = nSize;
#endif
    m_reticleLabel.m_tail.bVisible = true;
    m_reticleLabel.SetExtent(sExtent);
    CResRef sFriendly("friendlyreticle");
    CResRef sHostile("hostilereticle");
    m_reticleLabel.m_border.m_params.SetFillImage(nFriendly != 0 ? sFriendly : sHostile, 0);
    m_reticleLabel.m_border.m_params.m_fAlpha = 0.5f;
#if KOTOR1
    m_reticleLabel.m_border.m_params.m_flags.nFillStyle = 2;
#endif
}

#if KOTOR1
void CSWGuiMainInterface::UpdateIndicator(float fDelta) {
    CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_targetId);
    Vector vPos;
    CResRef sImage;
    CResRef sArrow;
    CSWGuiExtent sExtent;
    bool bVisible = false;
    bool bLive = false;
    if (pGameObject != 0) {
        short nScreenW = m_pGuiManager->m_nScreenWidth;
        short nScreenH = m_pGuiManager->m_nScreenHeight;
        if (pGameObject->AsSWCObject() != 0) {
            CSWCObject* pObject = pGameObject->AsSWCObject();
            CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            unsigned char nRet = pObject->GetReticlePoint(vPos);
            if (nRet == 0) {
                bLive = true;
                m_targetActionMenu.SetFriend(1, 0);
            } else {
                unsigned char nType = pGameObject->m_objectType;
                int nFriendly;
                int nSize;
                int bPoisoned;
                const char* pszArrow;
                if (nType - GAME_OBJECT_TYPE_PLACEABLE < 2u || nType == GAME_OBJECT_TYPE_TRIGGER) {
                    CSWCDoor* pDoor = pObject->AsSWCDoor();
                    CSWCPlaceable* pPlaceable = pObject->AsSWCPlaceable();
                    CSWCTrigger* pTrigger = pObject->AsSWCTrigger();
                    if (pDoor != 0 && pDoor->m_bHilighted != 0 && pDoor->m_bHostileFaction != 0)
                        nFriendly = 0;
                    else if (pTrigger != 0)
                        nFriendly = pTrigger->IsHostile() == 0;
                    else if (pPlaceable != 0 && pPlaceable->m_bHilighted != 0 && pPlaceable->m_bHostileFaction != 0)
                        nFriendly = 0;
                    else if (pPlaceable != 0 && pPlaceable->IsHostile() != 0) {
                        CSWSObject* pSrv = pPlaceable->GetServerObject();
                        CSWSObject* pPlayerSrv = pPlayer->GetServerObject();
                        nFriendly = pSrv->GetReputation(pPlayerSrv->m_objectId, 0) > 10;
                    } else
                        nFriendly = 1;
                    float fDX = pPlayer->m_position.x - pObject->m_position.x;
                    float fDY = pPlayer->m_position.y - pObject->m_position.y;
                    float fDZ = pPlayer->m_position.z - pObject->m_position.z;
                    float fDist2 = fDX * fDX + fDY * fDY + fDZ * fDZ;
                    if (fDist2 >= 900.0f)
                        nSize = 32;
                    else {
                        nSize = 64;
                        if (fDist2 > 25.0f)
                            nSize = 64 - static_cast<int>((__builtin_sqrtf(fDist2) - 5.0f) / 25.0f * 32.0f);
                    }
                    pszArrow = nFriendly != 0 ? "friendlyarrow" : "hostilearrow";
                    sImage = const_cast<char*>(nFriendly != 0 ? "friendlyreticle2" : "hostilereticle2");
                    bPoisoned = 0;
                } else if (nType == GAME_OBJECT_TYPE_CREATURE) {
                    CSWSCreature* pSrvCreature = pObject->AsSWCCreature()->GetServerCreature();
                    bool bFriendly = pObject->IsHostile() == 0 &&
                                     g_pAppManager->m_pClientExoApp->GetTargetChanging() == 0;
                    bPoisoned = 0;
                    if (pSrvCreature != 0)
                        bPoisoned = pSrvCreature->m_bPoisoned;
                    float fDX = pPlayer->m_position.x - pObject->m_position.x;
                    float fDY = pPlayer->m_position.y - pObject->m_position.y;
                    float fDZ = pPlayer->m_position.z - pObject->m_position.z;
                    float fDist2 = fDX * fDX + fDY * fDY + fDZ * fDZ;
                    if (fDist2 >= 900.0f)
                        nSize = 16;
                    else {
                        nSize = 64;
                        if (fDist2 > 25.0f)
                            nSize = 64 - static_cast<int>((__builtin_sqrtf(fDist2) - 5.0f) / 25.0f * 48.0f);
                    }
                    if (bFriendly) {
                        sImage = const_cast<char*>("friendlyreticle2");
                        nFriendly = 1;
                        pszArrow = "friendlyarrow";
                    } else {
                        if (g_pAppManager->m_pClientExoApp->GetCombatMode() == 0) {
                            sImage = const_cast<char*>("hostilereticle2");
                        } else {
                            sImage = const_cast<char*>("combatreticle");
                            float fT = m_fReticlePulseTimer;
                            if (!(fT > 0.0f)) {
                                if (fT == -1.0f) {
                                    nSize += 64;
                                    m_fReticlePulseTimer = 0.5f;
                                }
                            } else {
                                fT = fT - fDelta;
                                m_fReticlePulseTimer = fT;
                                if (fT <= 0.0f) {
                                    m_fReticlePulseTimer = 0.0f;
                                } else {
                                    nSize += static_cast<int>((1.0f - (0.5f - fT) * 2.0f) * 64.0f);
                                }
                            }
                        }
                        nFriendly = 0;
                        pszArrow = "hostilearrow";
                    }
                } else {
                    return;
                }
                sArrow = const_cast<char*>(pszArrow);
                m_targetActionMenu.SetFriend(nFriendly, bPoisoned);
#ifdef K2_FIX_ENGINE_BUGS
                // The arrow is drawn at 32 lines of a 768-line screen, so it is placed by that size,
                // not by 32 pixels.
                int nArrow = static_cast<int>(static_cast<float>(g_SDL_mode.h) / 768.0f * 32.0f);
#else
                int nArrow = 32;
#endif
                float fArrowHalf = static_cast<float>(nArrow) * 0.5f;
                int nEx = m_lblArrowMargin.m_extent.x;
                int nEy = m_lblArrowMargin.m_extent.y;
                int nEw = m_lblArrowMargin.m_extent.w;
                int nEh = m_lblArrowMargin.m_extent.h;
                int nW;
                float fRot;
                if (nRet == 2) {
                    sImage = sArrow;
                    int nHalfW = nScreenW / 2;
                    bool bNear = vPos.x < nHalfW;
                    fRot = bNear ? 180.0f : 0.0f;
                    sExtent.x = bNear ? nEx + nEw - nArrow : nEx;
                    if (vPos.y < nEy + nArrow) {
                        sExtent.y = nEy + nEh - nArrow;
                    } else if (vPos.y > nEy + nEh - nArrow) {
                        sExtent.y = nEy;
                    } else {
                        int nHalfH = nScreenH / 2;
                        if (vPos.y < nHalfH)
                            sExtent.y = nScreenH - static_cast<int>(vPos.y + fArrowHalf);
                        else
                            sExtent.y = static_cast<int>(vPos.y - fArrowHalf);
                    }
                    nW = 32;
                } else if (vPos.x < nEx + nArrow) {
                    sImage = sArrow;
                    sExtent.x = nEx;
                    if (vPos.y < nEy + nArrow) {
                        sExtent.y = nEy;
                    } else {
                        int nHiY = nEy + nEh - nArrow;
                        if (vPos.y > nHiY)
                            sExtent.y = nHiY;
                        else
                            sExtent.y = static_cast<int>(vPos.y - fArrowHalf);
                    }
                    fRot = 0.0f;
                    nW = 32;
                } else {
                    int nHiX = nEx + nEw - nArrow;
                    if (vPos.x > nHiX) {
                        sImage = sArrow;
                        sExtent.x = nHiX;
                        if (vPos.y < nEy + nArrow) {
                            sExtent.y = nEy;
                        } else {
                            int nHiY = nEy + nEh - nArrow;
                            if (vPos.y > nHiY)
                                sExtent.y = nHiY;
                            else
                                sExtent.y = static_cast<int>(vPos.y - fArrowHalf);
                        }
                        fRot = 180.0f;
                        nW = 32;
                    } else if (vPos.y < nEy + nArrow) {
                        sImage = sArrow;
                        fRot = 270.0f;
                        sExtent.x = static_cast<int>(vPos.x - fArrowHalf);
                        sExtent.y = nEy;
                        nW = 32;
                    } else {
                        int nHiY = nEy + nEh - nArrow;
                        if (vPos.y > nHiY) {
                            sImage = sArrow;
                            fRot = 90.0f;
                            sExtent.x = static_cast<int>(vPos.x - fArrowHalf);
                            sExtent.y = nHiY;
                            nW = 32;
                        } else {
#ifdef K2_FIX_ENGINE_BUGS
                            int nHalf = static_cast<int>(static_cast<float>(g_SDL_mode.h) / 768.0f * nSize) / 2;
#else
                            int nHalf = nSize / 2;
#endif
                            sExtent.x = static_cast<int>(vPos.x - nHalf);
                            sExtent.y = static_cast<int>(vPos.y - nHalf);
                            nW = nSize;
                            fRot = 0.0f;
                        }
                    }
                }
#ifdef K2_QOL_FULLSCREEN
                float s_fReticleScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
                static float s_fReticleScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
                sExtent.w = sExtent.h = static_cast<int>(s_fReticleScale * nW);
                m_lblTargetIndicator.SetExtent(sExtent);
                m_lblTargetIndicator.m_border.m_params.m_fRotate = fRot;
                m_lblTargetIndicator.m_border.m_params.SetFillImage(sImage, 0);
                m_lblTargetIndicator.m_border.m_params.m_flags.nFillStyle = 2;
                bLive = true;
                bVisible = true;
            }
        }
    }
    m_lblTargetIndicator.m_tail.bVisible = bVisible;
    m_targetActionMenu.m_nMenuFlags =
        static_cast<unsigned char>((m_targetActionMenu.m_nMenuFlags & ~1) | (bLive ? 1 : 0));
    UpdateStickyTarget();
}
#else
void CSWGuiMainInterface::UpdateIndicator(float fDelta) {
    CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_targetId);
    Vector vPos;
    CResRef sImage;
    CResRef sArrow;
    CSWGuiExtent sExtent;
    if (pGameObject == 0 || pGameObject->AsSWCObject() == 0) {
        m_lblTargetIndicator.m_tail.bVisible = false;
        m_targetActionMenu.m_nMenuFlags = static_cast<unsigned char>(m_targetActionMenu.m_nMenuFlags & ~1);
        UpdateStickyTarget();
        return;
    }
    CSWCObject* pObject = pGameObject->AsSWCObject();
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    char nRet = pObject->GetReticlePoint(vPos);
    int nFriendly;
    int nSize;
    int bPoisoned = 0;
    bool bVisible;
    bool bLive;
    if (nRet == 0) {
        bPoisoned = 0;
        nSize = 64;
        nFriendly = 1;
        bVisible = false;
        bLive = true;
    } else {
        unsigned char nType = pObject->m_objectType;
        if (nType - GAME_OBJECT_TYPE_PLACEABLE < 2u || nType == GAME_OBJECT_TYPE_TRIGGER) {
            CSWCDoor* pDoor = pObject->AsSWCDoor();
            CSWCPlaceable* pPlaceable = pObject->AsSWCPlaceable();
            CSWCTrigger* pTrigger = pObject->AsSWCTrigger();
            if (pDoor != 0 && pDoor->m_bHilighted != 0 && pDoor->m_bHostileFaction != 0)
                nFriendly = 0;
            else if (pTrigger != 0)
                nFriendly = pTrigger->IsHostile() == 0;
            else if (pPlaceable != 0 && pPlaceable->m_bHilighted != 0 && pPlaceable->m_bHostileFaction != 0)
                nFriendly = 0;
            else if (pPlaceable != 0 && pPlaceable->IsHostile() == 0)
                nFriendly = 1;
            else if (pPlaceable != 0) {
                CSWSObject* pSrv = pPlaceable->GetServerObject();
                CSWSObject* pPlayerSrv = pPlayer->GetServerObject();
                nFriendly = pSrv->GetReputation(pPlayerSrv->m_objectId, 0) > 10;
            }
            else
                nFriendly = 1;
            float fDX = pPlayer->m_position.x - pObject->m_position.x;
            float fDY = pPlayer->m_position.y - pObject->m_position.y;
            float fDZ = pPlayer->m_position.z - pObject->m_position.z;
            float fDist2 = fDX * fDX + fDY * fDY + fDZ * fDZ;
            if (fDist2 >= 100.0f)
                nSize = 32;
            else {
                nSize = 64;
                if (fDist2 > 25.0f)
                    nSize = 64 - static_cast<int>((__builtin_sqrtf(fDist2) - 5.0f) / 5.0f * 32.0f);
            }
            sImage = const_cast<char*>(nFriendly != 0 ? "friendlyreticle2" : "hostilereticle2");
            sArrow = const_cast<char*>(nFriendly != 0 ? "friendlyarrow" : "hostilearrow");
        } else if (nType == GAME_OBJECT_TYPE_CREATURE) {
            CSWSCreature* pSrvCreature = pObject->AsSWCCreature()->GetServerCreature();
            nFriendly = pObject->IsHostile() == 0 &&
                        g_pAppManager->m_pClientExoApp->GetTargetChanging() == 0;
            if (pSrvCreature != 0)
                bPoisoned = pSrvCreature->m_bPoisoned;
            float fDX = pPlayer->m_position.x - pObject->m_position.x;
            float fDY = pPlayer->m_position.y - pObject->m_position.y;
            float fDZ = pPlayer->m_position.z - pObject->m_position.z;
            float fDist2 = fDX * fDX + fDY * fDY + fDZ * fDZ;
            if (fDist2 >= 900.0f)
                nSize = 16;
            else {
                nSize = 64;
                if (fDist2 > 25.0f)
                    nSize = 64 - static_cast<int>((__builtin_sqrtf(fDist2) - 5.0f) / 25.0f * 48.0f);
            }
            if (nFriendly) {
                sImage = const_cast<char*>("friendlyreticle2");
                sArrow = const_cast<char*>("friendlyarrow");
                nFriendly = 1;
            } else {
                if (g_pAppManager->m_pClientExoApp->GetCombatMode() == 0) {
                    sImage = const_cast<char*>("hostilereticle2");
                } else {
                    sImage = const_cast<char*>("combatreticle");
                    float fT = m_fReticlePulseTimer;
                    if (!(fT > 0.0f)) {
                        if (fT == -1.0f) {
                            nFriendly = 0;
                            nSize += 64;
                            m_fReticlePulseTimer = 0.5f;
                        }
                    } else {
                        nFriendly = 0;
                        fT = fT - fDelta;
                        m_fReticlePulseTimer = fT;
                        if (fT <= 0.0f) {
                            m_fReticlePulseTimer = 0.0f;
                        } else {
                            nSize += static_cast<int>((1.0f - (0.5f - fT) * 2.0f) * 64.0f);
                        }
                    }
                }
                sArrow = const_cast<char*>("hostilearrow");
            }
        } else {
            return;
        }
    }
    m_targetActionMenu.SetFriend(nFriendly, bPoisoned);
    CResRef sBg(nFriendly == 0 ? "ios_action_bg2" : "ios_action_bg");
    m_actionMenuHalves[1].m_border.m_params.SetFillImage(sBg, 0);
    m_actionMenuHalves[1].m_borderHilight.m_params.SetFillImage(sBg, 0);
    if (nRet == 0) {
        bVisible = false;
        bLive = true;
    } else {
        int nEx = m_lblArrowMargin.m_extent.x;
        int nEy = m_lblArrowMargin.m_extent.y;
        int nEw = m_lblArrowMargin.m_extent.w;
        int nEh = m_lblArrowMargin.m_extent.h;
#ifdef K2_QOL_IMPROVEMENTS
        // The arrow is drawn at 32 of the HUD's layout lines (LsetImage below), so it is placed by
        // that size, not by 32 pixels.
        int nArrow = K2QolReticlePixels(m_pGuiManager, 32);
#else
        int nArrow = 32;
#endif
        float fArrowHalf = static_cast<float>(nArrow) * 0.5f;
        int nW;
        float fRot;
        if (nRet == 2) {
            sImage = sArrow;
            int nHalfW = m_pGuiManager->m_nScreenWidth / 2;
            bool bNear = vPos.x < nHalfW;
            fRot = bNear ? 180.0f : 0.0f;
            int nX = bNear ? nEx + nEw - nArrow : nEx;
            int nY;
            if (!(vPos.y < nEy + nArrow)) {
                if (vPos.y > nEy + nEh - nArrow) {
                    nY = nEy;
                } else {
                    int nHalfH = m_pGuiManager->m_nScreenHeight / 2;
                    if (!(vPos.y < nHalfH))
                        nY = static_cast<int>(vPos.y - fArrowHalf);
                    else
                        nY = m_pGuiManager->m_nScreenHeight - static_cast<int>(vPos.y + fArrowHalf);
                }
            } else {
                nY = nEy + nEh - nArrow;
            }
            sExtent.x = nX;
            sExtent.y = nY;
            nW = 32;
        } else if (!(vPos.x < nEx + nArrow)) {
            int nHiX = nEx + nEw - nArrow;
            if (!(vPos.x > nHiX)) {
                if (!(vPos.y < nEy + nArrow)) {
                    int nHiY = nEy + nEh - nArrow;
                    if (!(vPos.y > nHiY)) {
#ifdef K2_QOL_IMPROVEMENTS
                        int nHalf = K2QolReticlePixels(m_pGuiManager, nSize) / 2;
#else
                        int nHalf = nSize / 2;
#endif
                        sExtent.x = static_cast<int>(vPos.x - nHalf);
                        sExtent.y = static_cast<int>(vPos.y - nHalf);
                        nW = nSize;
                        fRot = 0.0f;
                        goto LsetImage;
                    }
                    sImage = sArrow;
                    fRot = 90.0f;
                    sExtent.y = nHiY;
                } else {
                    sImage = sArrow;
                    fRot = 270.0f;
                    sExtent.y = nEy;
                }
                sExtent.x = static_cast<int>(vPos.x - fArrowHalf);
            } else {
                sImage = sArrow;
                fRot = 180.0f;
                if (nEy + nArrow > vPos.y) {
                    sExtent.y = nEy;
                } else if (vPos.y > nEy + nEh - nArrow) {
                    sExtent.y = nEy + nEh - nArrow;
                } else {
                    sExtent.y = static_cast<int>(vPos.y - fArrowHalf);
                }
                sExtent.x = nHiX;
            }
            nW = 32;
        } else {
            sImage = sArrow;
            fRot = 0.0f;
            if (!(vPos.y < nEy + nArrow)) {
                int nHiY = nEy + nEh - nArrow;
                if (!(vPos.y > nHiY)) {
                    sExtent.y = static_cast<int>(vPos.y - fArrowHalf);
                } else {
                    sExtent.y = nHiY;
                }
            } else {
                sExtent.y = nEy;
            }
            sExtent.x = nEx;
            nW = 32;
        }
    LsetImage:
#ifdef K2_QOL_IMPROVEMENTS
        nW = K2QolReticlePixels(m_pGuiManager, nW);
#endif
        sExtent.w = sExtent.h = nW;
        m_lblTargetIndicator.SetExtent(sExtent);
        m_lblTargetIndicator.m_border.m_params.m_fRotate = fRot;
        m_lblTargetIndicator.m_border.m_params.SetFillImage(sImage, 0);
        bVisible = true;
        bLive = true;
    }
    m_lblTargetIndicator.m_tail.bVisible = bVisible;
    m_targetActionMenu.m_nMenuFlags =
        static_cast<unsigned char>((m_targetActionMenu.m_nMenuFlags & ~1) | (bLive ? 1 : 0));
    UpdateStickyTarget();
}
#endif

void CSWGuiMainInterface::OnTargetActionFocus(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg)
#if KOTOR1  // K1: the target boxes are 4..6
        m_targetActionMenu.m_nFocusedAction = static_cast<unsigned char>(pControl->m_tail.m_nUserData - 4);
#else
        m_targetActionMenu.m_nFocusedAction = static_cast<unsigned char>(pControl->m_tail.m_nUserData - 6);
#endif
}

void CSWGuiMainInterface::OnFlourishButtonPressed(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg)
        g_pAppManager->m_pClientExoApp->PlayerFlourishWeapons();
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::OnCancelPartyItemSelect(CSWGuiControl* pControl) {
    if (pControl == 0)
        return;
    if (pControl->m_tail.nCurrentEventArg)
        ResetPartyMemberItemUse();
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::OnCharacterExited(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    if (m_bPartyItemUsePending)
        m_nPartyItemUseTarget = -1;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::OnCharacterEntered(CSWGuiControl* pControl) {
    if (pControl == 0)
        return;
    if (m_bPartyItemUsePending)
        m_nPartyItemUseTarget = static_cast<int>(pControl->m_tail.m_nUserData) - 1;
}
#endif // !KOTOR1

void CSWGuiMainInterface::OnCharacterClicked(CSWGuiControl* pControl) {
    if (!pControl->m_tail.nCurrentEventArg)
        return;
#if KOTOR1
    int nIndex = static_cast<int>(pControl->m_tail.m_nUserData) - 1;
#else
    int bPending = m_bPartyItemUsePending;
    int nIndex = static_cast<int>(pControl->m_tail.m_nUserData) - 1;
    if (bPending) {
        if (nIndex == 2 &&
            g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount != 3)
            nIndex = 1;
        m_nPartyItemUseTarget = nIndex;
        CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pChar == 0) {
            ResetPartyMemberItemUse();
            return;
        }
        pChar->PartyMenuUseItemSelect(static_cast<unsigned int>(m_oidPartyItemUse) | CSWGuiInterfaceAction::ACTION_ID_ITEM, 0);
        return;
    }
#endif
    if (nIndex == 0) {
        CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pChar == 0)
            return;
        CSWSCreature* pServer = pChar->GetServerCreature();
        if (pServer == 0)
            return;
        if (pServer->m_pStats->CanLevelUp()) {
            m_btnCharacter.m_tail.nCurrentEventArg = 1;
            OnMenuButtonPressed(&m_btnCharacter);
        } else {
            m_btnEquip.m_tail.nCurrentEventArg = 1;
            OnMenuButtonPressed(&m_btnEquip);
        }
        return;
    }
    if (nIndex > 2)
        return;
    if (g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount < 3)
        nIndex = 1;
    CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nIndex);
    if (pChar == 0)
        return;
    if (pChar->GetServerCreature() == 0)
        return;
#if KOTOR1  // K1: a downed companion is GetDeadTemp
    if (pChar->GetServerCreature()->GetDeadTemp())
        return;
#else
    CSWSCreature* pServer = pChar->GetServerCreature();
    if (pServer->m_bInParty != 0 && pServer->GetCurrentHitPoints(0) < 1)
        return;
#endif
    g_pAppManager->m_pClientExoApp->ChangeCharacter(nIndex, 1);
}

bool CSWGuiMainInterface::HasForceResistance(CSWSCreature* pCreature) {
    return pCreature->m_pStats->GetForceResistance() > 0;
}

void CSWGuiMainInterface::HandleButtonAction(CSWGuiControl* pControl) {
#if !KOTOR1
    m_bClearSelectionOnClose = 1;
#endif
    HandleButtonAction(m_actionMenuIos.m_nActionIndex, static_cast<int>(pControl->m_tail.m_nUserData));
}

#ifdef K2_UI_PC
#if KOTOR1
#define K1_OR_K2_TARGET_BOX0 4
#else
#define K1_OR_K2_TARGET_BOX0 6
#endif
static void PcArrowHead(CSWGuiMainInterface* pInterface) {
#if !KOTOR1  // K1 has no party item-use selection to cancel
    pInterface->ResetPartyMemberItemUse();
#else
    K2_UNUSED(pInterface);
#endif
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bAutoPauseActionMenu)
        g_pAppManager->m_pClientExoApp->SetAutoPaused(1, 7);
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_USE_VERTICAL_MENU, OBJECT_INVALID, OBJECT_INVALID, 0);
}
#endif

void CSWGuiMainInterface::OnTargetUpArrowPressed(CSWGuiControl* pControl) {
#ifdef K2_UI_PC
    if (pControl->m_tail.nCurrentEventArg) {
        PcArrowHead(this);
        m_targetActionMenu.SelectPrevAction(static_cast<int>(pControl->m_tail.m_nUserData) - K1_OR_K2_TARGET_BOX0);
    }
#elif defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
    // console: an arrow click or a wheel notch steps the box's entries, as the d-pad does.
    if (pControl->m_tail.nCurrentEventArg)
        k2console_mouse_step(this, static_cast<int>(pControl->m_tail.m_nUserData), false);
#else
    if (pControl->m_tail.nCurrentEventArg) {
#if !KOTOR1  // K1 has no party item-use selection to cancel
        ResetPartyMemberItemUse();
#endif
        int nIndex = static_cast<int>(pControl->m_tail.m_nUserData);
        if (nIndex >= 0)
            PopupActionMenu(nIndex);
    }
#endif
}

void CSWGuiMainInterface::OnTargetDownArrowPressed(CSWGuiControl* pControl) {
#ifdef K2_UI_PC
    if (pControl->m_tail.nCurrentEventArg) {
        PcArrowHead(this);
        m_targetActionMenu.SelectNextAction(static_cast<int>(pControl->m_tail.m_nUserData) - K1_OR_K2_TARGET_BOX0);
    }
#elif defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
    // console: an arrow click or a wheel notch steps the box's entries, as the d-pad does.
    if (pControl->m_tail.nCurrentEventArg)
        k2console_mouse_step(this, static_cast<int>(pControl->m_tail.m_nUserData), true);
#else
    if (pControl->m_tail.nCurrentEventArg) {
#if !KOTOR1  // K1 has no party item-use selection to cancel
        ResetPartyMemberItemUse();
#endif
        int nIndex = static_cast<int>(pControl->m_tail.m_nUserData);
        if (nIndex >= 0)
            PopupActionMenu(nIndex);
    }
#endif
}

void CSWGuiMainInterface::OnActionUpArrowPressed(CSWGuiControl* pControl) {
#ifdef K2_UI_PC
    if (pControl->m_tail.nCurrentEventArg) {
        PcArrowHead(this);
#if !KOTOR1  // K1 has no arrow-key latch
        m_bActionMenuLatch = 1;
#endif
        SelectPrevPersonalAction(static_cast<int>(pControl->m_tail.m_nUserData));
    }
#elif defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
    // console: an arrow click or a wheel notch steps the box's entries, as the d-pad does.
    if (pControl->m_tail.nCurrentEventArg)
        k2console_mouse_step(this, static_cast<int>(pControl->m_tail.m_nUserData), false);
#else
    if (pControl->m_tail.nCurrentEventArg) {
#if !KOTOR1
        ResetPartyMemberItemUse();
        m_bActionMenuLatch = 1;
#endif
        int nIndex = static_cast<int>(pControl->m_tail.m_nUserData);
        if (nIndex >= 0)
            PopupActionMenu(nIndex);
    }
#endif
}

void CSWGuiMainInterface::OnSoloButtonPressed(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowSoloModeQuery(0);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::OnSwapWeaponsPressed(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pPlayer)
        pPlayer->ActionMenuSwitchWeapons(0, 0);
}
#endif // !KOTOR1

void CSWGuiMainInterface::SetActionMenuSelected(int nSlot) {
    m_pGuiManager->HandleMouseMove(m_actionSlots[nSlot].m_extent.x + 1,
                                   m_actionSlots[nSlot].m_extent.y + 1);
}

void CSWGuiMainInterface::UpdateActionMenuSelected(int nStep) {
    int nSelected = GetActionMenuSelected();
    for (int i = 0; i < 32; i++)
        m_actionSlots[i].SetActive(0);
    int nDelta = (nSelected == -1) ? 1 : nStep;
    int nCount = IosActionMenuCount;
    int nNew = (nSelected + nDelta + nCount) % nCount;
    m_pGuiManager->HandleMouseMove(m_actionSlots[nNew].m_extent.x + 1,
                                   m_actionSlots[nNew].m_extent.y + 1);
}

int CSWGuiMainInterface::GetActionMenuSelected() {
    int nSelected = -1;
    for (int i = 0; i < 32; i++) {
        if (m_actionSlots[i].m_tail.bActive)
            nSelected = i;
    }
    return nSelected;
}

unsigned char CSWGuiTargetActionMenu::IsTargetActionMenuControl(CSWGuiControl* pControl) {
    if (pControl == &m_lblName || pControl == &m_progressBar ||
        pControl == &m_lblNameBg || pControl == &m_lblHealthBg)
        return 1;
    for (int i = 0; i < 3; i++) {
        if (pControl == &m_widgets[i].m_btnUp || pControl == &m_widgets[i].m_btnDown ||
            pControl == &m_widgets[i].m_icon || pControl == &m_widgets[i].m_button)
            return 2;
    }
    return 0;
}

void CSWGuiTargetActionMenu::ResetFont() {
    m_lblName.ReSetFont();
    m_lblNameBg.ReSetFont();
    m_lblHealthBg.ReSetFont();
}

#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
static void DrawRevealBar(CSWGuiProgressBar& bar, float fDelta) {
    CSWGuiExtent reveal = bar.m_extent;
    CSWGuiExtent extentProgress = bar.m_extent;
    extentProgress.x = 0;
    extentProgress.y = 0;
    if (bar.m_nMaxValue > 0) {
        float fRatio = static_cast<float>(bar.m_nCurValue) / static_cast<float>(bar.m_nMaxValue);
        if (bar.m_extent.h > bar.m_extent.w) {
            int nLength = static_cast<int>(static_cast<float>(reveal.h) * fRatio);
            if (!bar.m_bStartFromLeft) {
                reveal.y += reveal.h - nLength;
                extentProgress.y = nLength - reveal.h;
            }
            reveal.h = nLength;
        } else {
            int nLength = static_cast<int>(static_cast<float>(reveal.w) * fRatio);
            if (!bar.m_bStartFromLeft) {
                reveal.x += reveal.w - nLength;
                extentProgress.x = nLength - reveal.w;
            }
            reveal.w = nLength;
        }
    }
    bar.m_borderProgress.m_extent = extentProgress;
    bar.m_border.Draw(fDelta);
    if (AurGUISetupViewport(reveal.x, reveal.y, reveal.w, reveal.h, CSWGuiDrawable::NO_COLOURING,
                            false, 1.0f)) {
        bar.m_borderProgress.Draw(fDelta);
        AurGUICloseViewport();
    }
}

// The console bar's arrow strip (LBH_ARROW) is one control: the up arrow above the box's icon and
// the down arrow below it, the height of the box in K1 and overhanging it in K2. Hit-tested whole
// it swallowed the middle of the box and lit both arrows, so it is split at the icon: the top half
// is m_btnUp, the bottom half m_btnDown (no art of its own), and between them the icon.
static CSWGuiControl* ConsoleArrowHit(CSWGuiMainInterfaceAction& widget, int x, int y) {
    if (!widget.m_btnUp.m_tail.bVisible)
        return 0;
    const CSWGuiExtent& strip = widget.m_btnUp.m_extent;
    if (x < strip.x || x > strip.x + strip.w || y < strip.y || y > strip.y + strip.h)
        return 0;
    const CSWGuiExtent& icon = widget.m_icon.m_extent;
    if (y < icon.y)
        return &widget.m_btnUp;
    if (y > icon.y + icon.h)
        return &widget.m_btnDown;
    return &widget.m_icon;
}

// The strip draws unlit (hovering either half activates the whole control, which would light both
// arrows); ConsoleDrawArrowHover then lights the hovered half. Returns the activity to restore.
static int ConsoleStripUnlit(CSWGuiMainInterfaceAction& widget) {
    const int bActive = widget.m_btnUp.m_tail.bActive;
    widget.m_btnUp.m_tail.bActive = 0;
    return bActive;
}

// Lights the hovered arrow only: the strip's highlight art is clipped to its half.
static void ConsoleDrawArrowHover(CSWGuiMainInterfaceAction& widget, float fDelta) {
    CSWGuiControl* pOver = g_pGuiMan->m_pMouseOverControl;
    const bool bUp = pOver == &widget.m_btnUp;
    if (!widget.m_btnUp.m_tail.bVisible || (!bUp && pOver != &widget.m_btnDown))
        return;
    CSWGuiBorder& hilight = widget.m_btnUp.m_borderHilight;
    const CSWGuiExtent strip = hilight.m_extent;
    const CSWGuiExtent& icon = widget.m_icon.m_extent;
    const int nTop = bUp ? strip.y : icon.y + icon.h;
    const int nBottom = bUp ? icon.y : strip.y + strip.h;
    if (nBottom <= nTop)
        return;
    if (AurGUISetupViewport(strip.x, nTop, strip.w, nBottom - nTop, CSWGuiDrawable::NO_COLOURING,
                            false, 1.0f)) {
        hilight.m_extent = CSWGuiExtent(0, strip.y - nTop, strip.w, strip.h);
        hilight.Draw(fDelta);
        hilight.m_extent = strip;
        AurGUICloseViewport();
    }
}

#ifdef K1_UI_XBOX
void K1ConsoleRevealBar::Draw(float fDelta) {
    DrawRevealBar(*this, fDelta);
}
#endif

#endif

void CSWGuiTargetActionMenu::Draw(float fDelta) {
    if (!(m_nMenuFlags & 1))
        return;

    AurGUIStartLayer();
    if (AurGUISetupViewport(m_aViewport[0], m_aViewport[1], m_aViewport[2], m_aViewport[3],
                            CSWGuiDrawable::NO_COLOURING, false, 1.0f)) {
        m_lblHealthBg.Draw(fDelta);
        m_lblNameBg.Draw(fDelta);
        m_lblName.Draw(fDelta);
#ifdef K2_UI_XBOX
        DrawRevealBar(m_progressBar, fDelta);   // console HUD: drawn by the shared bar helper
#else
        m_progressBar.Draw(fDelta);
#endif

        for (int nI = 0; nI < 3; nI++) {
            if (m_widgets[nI].m_button.m_tail.bVisible)
                m_widgets[nI].m_button.Draw(fDelta);
            if (m_widgets[nI].m_btnUp.m_tail.bVisible)
                m_widgets[nI].m_btnUp.Draw(fDelta);
            if (m_widgets[nI].m_btnDown.m_tail.bVisible)
                m_widgets[nI].m_btnDown.Draw(fDelta);
            if (m_widgets[nI].m_icon.m_tail.bVisible)
                m_widgets[nI].m_icon.Draw(fDelta);
        }

        AurGUICloseViewport();
    }
    AurGUIStopLayer();
}

void CSWGuiTargetActionMenu::UpdateTimers(float fDelta) {
    if (m_fHintTimer != -1.0f) {
        m_fHintTimer -= fDelta;
        if (m_fHintTimer <= 0.0f)
            m_fHintTimer = -1.0f;
    }
    if (m_fHoverReleaseTimer != -1.0f) {
        m_fHoverReleaseTimer -= fDelta;
        if (m_fHoverReleaseTimer <= 0.0f)
            m_fHoverReleaseTimer = -1.0f;
    }
}

void CSWGuiTargetActionMenu::SelectNextAction(int nIndex)
{
    if (static_cast<unsigned int>(nIndex) > 2)
        return;
    if (m_actions[nIndex].m_count == 0)
        return;

    g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();

    int* pCurrentId = &m_actionIds[static_cast<signed char>(m_actionRow)][nIndex];
    int nId = *pCurrentId;

    CSWGuiInterfaceAction* pNext = 0;
    if (nId != -1) {
        for (int i = 0; i < m_actions[nIndex].m_count; i++) {
            if (static_cast<int>(m_actions[nIndex].m_pArray[i].m_nActionId) == nId) {
                pNext = m_actions[nIndex].m_pArray;
                if (i + 1 < m_actions[nIndex].m_count)
                    pNext = &m_actions[nIndex].m_pArray[i + 1];
                break;
            }
        }
    }

    if (m_actions[nIndex].m_count < 2)
        return;
    if (pNext == 0)
        pNext = &m_actions[nIndex].m_pArray[1];

    *pCurrentId = static_cast<int>(pNext->m_nActionId);

    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PlayGuiSound(GUISOUND_ACTION_MENU_SCROLL);
}

void CSWGuiTargetActionMenu::SelectPrevAction(int nIndex)
{
    if (static_cast<unsigned int>(nIndex) > 2)
        return;
    if (m_actions[nIndex].m_count == 0)
        return;

    g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();

    int* pCurrentId = &m_actionIds[static_cast<signed char>(m_actionRow)][nIndex];
    int nId = *pCurrentId;

    int64_t nCount = m_actions[nIndex].m_count;
    CSWGuiInterfaceAction* pPrev = 0;
    if (nId != -1) {
        for (int i = 0; i < nCount; i++) {
            if (static_cast<int>(m_actions[nIndex].m_pArray[i].m_nActionId) == nId) {
                if (i == 0)
                    pPrev = &m_actions[nIndex].m_pArray[nCount - 1];
                else
                    pPrev = &m_actions[nIndex].m_pArray[i - 1];
                break;
            }
        }
    }

    if (m_actions[nIndex].m_count < 2)
        return;
    if (pPrev == 0)
        pPrev = &m_actions[nIndex].m_pArray[m_actions[nIndex].m_count - 1];

    *pCurrentId = static_cast<int>(pPrev->m_nActionId);

    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PlayGuiSound(GUISOUND_ACTION_MENU_SCROLL);
}

CSWGuiControl* CSWGuiTargetActionMenu::HitCheckMouse(int x, int y)
{
    CSWGuiControl* pControl = 0;
    if (m_nMenuFlags & 1) {
        int nX = x - m_aViewport[0];
        int nY = y - m_aViewport[1];

        if (m_lblName.HitCheckMouse(nX, nY))
            pControl = &m_lblName;
        else if (m_progressBar.HitCheckMouse(nX, nY))
            pControl = &m_progressBar;
        else {
            int nI = 0;
            int nCount;
            do {
                nCount = m_actions[nI].m_count;
                nI++;
            } while (nCount == 0 && nI < 3);
            if (nCount != 0) {
                for (int i = 0; i < 3 && !pControl; i++) {
                    if (m_widgets[i].m_btnUp.HitCheckMouse(nX, nY))
                        pControl = &m_widgets[i].m_btnUp;
                    else if (m_widgets[i].m_btnDown.HitCheckMouse(nX, nY))
                        pControl = &m_widgets[i].m_btnDown;
                    else if (m_widgets[i].m_icon.HitCheckMouse(nX, nY))
                        pControl = &m_widgets[i].m_icon;
                    else if (m_widgets[i].m_button.HitCheckMouse(nX, nY))
                        pControl = &m_widgets[i].m_button;
                }
            }
        }
    }

    if (!pControl && (m_nMenuFlags & 2))
        m_fHoverReleaseTimer = 0.5f;

    m_nMenuFlags = (m_nMenuFlags & ~2) | (pControl ? 2 : 0);
    return pControl;
}

void CSWGuiMainInterface::ResetFont() {
    CSWGuiPanel::ResetFont();
    m_targetActionMenu.ResetFont();
}

CSWGuiControl* CSWGuiMainInterface::HitCheckMouse(int x, int y) {
#if defined(K1_UI_XBOX)
    for (int i = 0; i < 6; i++)
        if (CSWGuiControl* pArrow = ConsoleArrowHit(m_personalActionWidgets[i], x - m_extent.x, y - m_extent.y))
            return pArrow;
#elif defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
    for (int i = 0; i <= CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT; i++)
        if (CSWGuiControl* pArrow = ConsoleArrowHit(*GetActionByIndex(i), x - m_extent.x, y - m_extent.y))
            return pArrow;
#endif
    CSWGuiControl* pControl = m_targetActionMenu.HitCheckMouse(x, y);
    if (pControl)
        return pControl;
    return CSWGuiPanel::HitCheckMouse(x, y);
}

void CSWGuiMainInterface::DisplayToolTipMessage() {
    m_lblCombatModeMsg.m_tail.bVisible = 0;
    m_lblCombatModeMsg2.m_tail.bVisible = 1;
    m_lblCombatMsgBg.m_tail.bVisible = 1;
    m_bToolTipMessageShown = 1;
}

// Hides the tooltip message, restores the combat message if ShowActionQueue still wants it, then
// mirrors that visibility onto the message backdrop.
void CSWGuiMainInterface::RemoveToolTipMessage() {
    m_lblCombatModeMsg2.m_tail.bVisible = 0;
    if (m_nInterfaceFlags & 2)
        m_lblCombatModeMsg.m_tail.bVisible = 1;
    m_bToolTipMessageShown = 0;
    m_lblCombatMsgBg.m_tail.bVisible = m_lblCombatModeMsg.m_tail.bVisible;
}

void CSWGuiMainInterface::OnActionDownArrowPressed(CSWGuiControl* pControl) {
#ifdef K2_UI_PC
    if (pControl->m_tail.nCurrentEventArg) {
        PcArrowHead(this);
#if !KOTOR1  // K1 has no arrow-key latch
        m_bActionMenuLatch = 1;
#endif
        SelectNextPersonalAction(static_cast<int>(pControl->m_tail.m_nUserData));
    }
#elif defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
    // console: an arrow click or a wheel notch steps the box's entries, as the d-pad does.
    if (pControl->m_tail.nCurrentEventArg)
        k2console_mouse_step(this, static_cast<int>(pControl->m_tail.m_nUserData), true);
#else
    if (pControl->m_tail.nCurrentEventArg) {
#if !KOTOR1  // K1 has no party item-use selection to cancel
        ResetPartyMemberItemUse();
#endif
        int nIndex = static_cast<int>(pControl->m_tail.m_nUserData);
        if (nIndex >= 0)
            PopupActionMenu(nIndex);
    }
#endif
}


void CSWGuiMainInterface::OnCombatYButton() {
    CSWSCreature* pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()
                                  ->GetPlayerCharacter()->GetServerCreature();
    if (pCreature) {
        CSWSCombatRound* pRound = pCreature->m_pCombatRound;
        if (pRound && !pRound->RemoveLastAction())
            pCreature->ClearAllActions(1);
    }
}

void CSWGuiMainInterface::ClearOneAction() {
    if (!m_btnClearOne.m_tail.bVisible)
        return;
    if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_IN_COMBAT_Y_BUTTON, OBJECT_INVALID, OBJECT_INVALID, 0))
        return;
    CSWSCreature* pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()
                                  ->GetPlayerCharacter()->GetServerCreature();
    if (pCreature) {
        CSWSCombatRound* pRound = pCreature->m_pCombatRound;
        if (pRound && !pRound->RemoveLastAction())
            pCreature->ClearAllActions(1);
    }
}

void CSWGuiMainInterface::ClearAllActions() {
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    if (pPlayer) {
        pPlayer->SetCombatMode(0);
        pPlayer->ClearAllActions();
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        m_fReticlePulseTimer = -1.0f;
        m_fChallengeRating = -1.0f;
    }
}

void CSWGuiMainInterface::OnClearAllButtonPressed(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg)
        ClearAllActions();
}

void CSWGuiMainInterface::OnStealthButtonPressed(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg) {
        if (g_pAppManager->m_pClientExoApp->GetPlayerCreature()->IsStealthCapable())
            g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter()
                ->PartyMenuStealth(0, 0);
    }
}

void CSWGuiMainInterface::OnDefaultActionFocus(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg) {
        int nAction = static_cast<int>(pControl->m_tail.m_nUserData);
#if defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
        // console: pointing at an empty box leaves the caption on the selected one.
        if (CSWGuiMainInterfaceAction* pWidget = GetActionByIndex(nAction))
            if (pWidget->m_bEmpty)
                return;
#endif
#if KOTOR1
        if (m_nFocusedAction != nAction)
            m_nFocusedAction = nAction;
#else
        if (!g_pGuiMan->PanelExists(&m_actionMenuIos)) {
            if (m_nFocusedAction != nAction)
                m_nFocusedAction = nAction;
        }
#endif
    }
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::OnPanelRemoved() {
    g_pGuiMan->RemovePanel(&m_gamepadMenu);
    g_pGuiMan->RemovePanel(&m_actionMenuIos);
    g_pGuiMan->RemovePanel(&m_helpPanel);
    CSWGuiPanel::OnPanelRemoved();
}
#endif // !KOTOR1

#if KOTOR1
void CSWGuiMainInterface::SetupPauseGuiExtent(CSWGuiExtent* pExtent) {
    pExtent->y = static_cast<int>(AndroidHudScaleFactor() * 3.0f);
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (!pOptions || !pOptions->m_bHideInGameGui)
        pExtent->y = static_cast<int>(static_cast<float>(m_btnEquip.m_extent.h + m_btnEquip.m_extent.y) -
                           AndroidHudScaleFactor() + pExtent->y);
    pExtent->x = static_cast<int>(static_cast<float>(m_pGuiManager->m_nScreenWidth - pExtent->w) +
                       AndroidHudScaleFactor() * -3.0f);
#ifdef K1_UI_XBOX
    // xbox: no menu bar to hang under, and the minimap owns the top right, so centre the pause menu
    // with its top just under the target banner's plate (LBL_INDICATE) to keep the two apart.
    pExtent->x = (m_pGuiManager->m_nScreenWidth - pExtent->w) / 2;
    const CSWGuiExtent& banner = m_targetActionMenu.m_lblNameBg.m_extent;
    if (banner.h > 0)
        pExtent->y = banner.y + banner.h + static_cast<int>(AndroidHudScaleFactor() * 4.0f);
#endif
}
#else
void CSWGuiMainInterface::SetupPauseGuiExtent(CSWGuiExtent* pExtent) {
#if defined(K2_UI_XBOX) || defined(K2_UI_PC)
    if (m_lblCombatModeMsg.m_extent.w <= 0 || m_lblMenuKeyBg.m_extent.w <= 0) {
        const int nScreenW = m_extent.w > 0 ? m_extent.w : 800;
        const int nScreenH = m_extent.h > 0 ? m_extent.h : 600;
        pExtent->w = 251 * nScreenW / 800;
        pExtent->h = 70 * nScreenH / 600;
        pExtent->x = (nScreenW - pExtent->w) / 2;
        pExtent->y = 8 * nScreenH / 600;
        return;
    }
#endif
    pExtent->y = 2;
    pExtent->x = m_lblCombatModeMsg.m_extent.x + m_lblCombatModeMsg.m_extent.w + 4;
    pExtent->w = m_lblMenuKeyBg.m_extent.x - pExtent->x - 2;
    pExtent->h = m_lblCombatModeMsg.m_extent.h;
}
#endif

void CSWGuiMainInterface::ShiftControl(CSWGuiControl& control, int nDelta) {
    CSWGuiExtent extent = control.m_extent;
    extent.y += nDelta;
    control.SetExtent(extent);
}

void CSWGuiMainInterface::OnPauseButtonPressed(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg) {
        int bPaused = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(bPaused == 0, 4, 0);
        if (g_pAppManager->m_pClientExoApp->GetAutoPaused())
            g_pAppManager->m_pClientExoApp->SetAutoPaused(0, 0);
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_PAUSE_GAME, OBJECT_INVALID, OBJECT_INVALID, 0);
    }
}

void CSWGuiMainInterface::Quicksave(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->SendSaveGameRequest(0, CExoString("QUICKSAVE"),
                                                        CExoString("QUICKSAVE"));
}

void CSWGuiMainInterface::ShowActionIcon(int& nCount, const CResRef& resRef,
                                         const CResRef& resFallback, int nQueued, int bForce) {
    if (!bForce && !(resRef != const_cast<char*>("")))
        return;
#if KOTOR1
    int bValid = resRef.IsValid();
    if (bValid)
        m_actionQueueIcons[nCount].m_border.m_params.SetFillImage(resRef, 0);
    else
        m_actionQueueIcons[nCount].m_border.m_params.SetFillImage(resFallback, 0);
    m_actionQueueIcons[nCount].m_border.m_params.m_flags.nFillStyle = 2;
    if ((nCount | nQueued) == 0)
        m_actionQueueIcons[0].m_border.m_params.m_fAlpha = 0.75f;
    else
        m_actionQueueIcons[nCount].m_border.m_params.m_fAlpha = 1.0f;
#else
    int bValid = resRef.IsValid();
    if (bValid)
        m_actionQueueIcons[nCount].m_border.m_params.SetFillImage(resRef, 0);
    else
        m_actionQueueIcons[nCount].m_border.m_params.SetFillImage(resFallback, 0);
    if ((nCount | nQueued) == 0)
        m_actionQueueIcons[0].m_border.m_params.m_fAlpha = 0.25f;
    else
        m_actionQueueIcons[nCount].m_border.m_params.m_fAlpha = 1.0f;
#endif
    int nSlot = nCount;
    nCount = nSlot + 1;
    m_actionQueueIcons[nSlot].m_tail.bVisible = 1;
}

void CSWGuiMainInterface::GetActionIcon(CSWSCombatRound* pCombatRound, CSWSObjectActionNode* pActionNode,
                                        CSWSCombatRoundAction* pRoundAction, int& nCount,
                                        CResRef& resIcon, CResRef& resFallback) {
    K2_UNUSED(pCombatRound); K2_UNUSED(nCount);
    resIcon = const_cast<char*>("");
    resFallback = const_cast<char*>("");
    int nId = pActionNode ? pActionNode->m_nActionId : AIACTION_PHYSICAL_ATTACKS;
    if (nId == AIACTION_PHYSICAL_ATTACKS) {
#if !KOTOR1 || defined(K2_FIX_ENGINE_BUGS)
        if (!pRoundAction)
            return;
#endif
        switch (pRoundAction->m_nActionType) {
        case CSWSCombatRoundAction::TYPE_ATTACK:
            resIcon = const_cast<char*>("i_attack");
            resFallback = const_cast<char*>("i_attackm");
            break;
        case CSWSCombatRoundAction::TYPE_EQUIP:
            resIcon = const_cast<char*>("i_equip");
            resFallback = const_cast<char*>("i_equipm");
            break;
        case CSWSCombatRoundAction::TYPE_UNEQUIP:
            resIcon = const_cast<char*>("i_unequip");
            resFallback = const_cast<char*>("i_equipm");
            break;
        case CSWSCombatRoundAction::TYPE_CAST_SPELL: {
            CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(pRoundAction->m_nSpellId);
            if (pSpell) {
                resIcon = pSpell->m_iconResRef;
                resFallback = const_cast<char*>("i_powerm");
            }
            break;
        }
        case CSWSCombatRoundAction::TYPE_ITEM_SPELL: {
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pRoundAction->m_oidItem);
            if (pItem) {
                resIcon = pItem->GetIcon();
                resFallback = const_cast<char*>("i_useitemm");
            }
            break;
        }
        case CSWSCombatRoundAction::TYPE_FEAT_ATTACK: {
            CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(pRoundAction->m_nFeatId));
            if (pFeat) {
                resIcon = pFeat->m_icon;
                resFallback = const_cast<char*>("i_featm");
            }
            break;
        }
        }
    } else if (nId == AIACTION_COMBAT) {
        if (!pRoundAction) {
#if KOTOR1
            resIcon = const_cast<char*>("i_attack");
            resFallback = const_cast<char*>("i_attackm");
#else
            CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            CSWSCreature* pPCServer = pPC ? pPC->GetServerCreature() : 0;
            if (pPCServer && pPCServer->IsInTotalDefense()) {
                resIcon = const_cast<char*>("i_stancedef");
                resFallback = const_cast<char*>("i_stancedefm");
            } else {
                resIcon = const_cast<char*>("i_attack");
                resFallback = const_cast<char*>("i_attackm");
            }
#endif
            return;
        }
        switch (pRoundAction->m_nActionType) {
        case CSWSCombatRoundAction::TYPE_ATTACK:
            resIcon = const_cast<char*>("i_attack");
            resFallback = const_cast<char*>("i_attackm");
            break;
        case CSWSCombatRoundAction::TYPE_EQUIP:
            resIcon = const_cast<char*>("i_equip");
            resFallback = const_cast<char*>("i_equipm");
            break;
        case CSWSCombatRoundAction::TYPE_UNEQUIP:
            resIcon = const_cast<char*>("i_unequip");
            resFallback = const_cast<char*>("i_equipm");
            break;
        case CSWSCombatRoundAction::TYPE_CAST_SPELL: {
            CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(pRoundAction->m_nSpellId);
            if (pSpell) {
                resIcon = pSpell->m_iconResRef;
                resFallback = const_cast<char*>("i_powerm");
            }
            break;
        }
        case CSWSCombatRoundAction::TYPE_ITEM_SPELL: {
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pRoundAction->m_oidItem);
            if (pItem) {
                resIcon = pItem->GetIcon();
                resFallback = const_cast<char*>("i_useitemm");
            }
            break;
        }
        case CSWSCombatRoundAction::TYPE_FEAT_ATTACK: {
            CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(pRoundAction->m_nFeatId));
            if (pFeat) {
                resIcon = pFeat->m_icon;
                resFallback = const_cast<char*>("i_featm");
            }
            break;
        }
#if !KOTOR1  // K1: no stance / weapon-swap round actions
        case CSWSCombatRoundAction::TYPE_STANCE:
            resIcon = const_cast<char*>("i_stancedef");
            resFallback = const_cast<char*>("i_stancedefm");
            break;
        case CSWSCombatRoundAction::TYPE_SWITCH_WEAPONS:
            resIcon = const_cast<char*>("i_swtchwpn");
            resFallback = const_cast<char*>("i_swtchwpn");
            break;
#endif
        }
    } else if (nId == AIACTION_CAST_SPELL) {
        resIcon = g_pRules->m_pSpellArray->GetSpell(pActionNode->m_args.m_spellId)->m_iconResRef;
        resFallback = const_cast<char*>("i_powerm");
    } else if (nId == AIACTION_ITEM_CAST_SPELL) {
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(static_cast<unsigned int>(pActionNode->m_args.m_useItemPropItemId));
        if (pItem) {
            resIcon = pItem->GetIcon();
            resFallback = const_cast<char*>("i_useitemm");
        }
    }
}

void CSWGuiMainInterface::UpdateActionQueue() {
    CSWSCreature* pServer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter()->GetServerCreature();
    CSWSCombatRound* pCombatRound = pServer->m_pCombatRound;
    bool bExecuting = pCombatRound->m_bRoundStarted && pCombatRound->GetCurrentAction();
    CExoString sUnused;
    CExoString sDash("-");
    CResRef resIcon;
    CResRef resFallback;
    int nCount = 0;
    bool bIconDone = false;
    CSWSObjectActionNode* pActionNode;
    CSWSCombatRoundAction* pRoundAction;
    for (CExoLinkedListNode* pNode = pServer->m_actionList.m_pInternal->m_pHead;
         pNode && nCount < 4;
         pServer->m_actionList.GetNext(pNode)) {
        pActionNode = static_cast<CSWSObjectActionNode*>(pServer->m_actionList.GetAtPos(pNode));
        if (!pActionNode)
            continue;
        int nType = pActionNode->m_nActionId;
        bool bMatch = (bExecuting & (nType == AIACTION_COMBAT)) | (nType == AIACTION_PHYSICAL_ATTACKS) | (nType == AIACTION_CAST_SPELL) | (nType == AIACTION_ITEM_CAST_SPELL);
        if (!bIconDone && bMatch) {
            CSWSCombatRoundAction* pCur = pCombatRound->m_pCurrentAction;
            if (!pCur) {
                if (nType == AIACTION_COMBAT) {
                    bIconDone = false;
                } else {
                    GetActionIcon(pCombatRound, pActionNode, 0, nCount, resIcon, resFallback);
                    ShowActionIcon(nCount, resIcon, resFallback, bExecuting, 0);
                    bIconDone = true;
                }
            } else {
                GetActionIcon(pCombatRound, pActionNode, pCur, nCount, resIcon, resFallback);
                ShowActionIcon(nCount, resIcon, resFallback, bExecuting, 0);
                bIconDone = true;
            }
        }
#if KOTOR1  // K1 re-reads the action id after the icon calls
        if (pActionNode->m_nActionId == AIACTION_COMBAT && pCombatRound->m_pRoundActionList) {
#else
        if (nType == AIACTION_COMBAT && pCombatRound->m_pRoundActionList) {
#endif
            CExoLinkedListNode* pRoundNode =
                pCombatRound->m_pRoundActionList->m_pInternal->m_pHead;
            if (!pRoundNode) {
                if (!bIconDone) {
                    GetActionIcon(pCombatRound, pActionNode, pCombatRound->m_pCurrentAction,
                                  nCount, resIcon, resFallback);
                    ShowActionIcon(nCount, resIcon, resFallback, bExecuting, 0);
                    bIconDone = false;
                }
            } else {
                for (; pRoundNode && nCount <= 3;
                     pCombatRound->m_pRoundActionList->GetNext(pRoundNode)) {
                    pRoundAction = static_cast<CSWSCombatRoundAction*>(pCombatRound->m_pRoundActionList->GetAtPos(pRoundNode));
                    GetActionIcon(pCombatRound, pActionNode, pRoundAction, nCount, resIcon, resFallback);
                    ShowActionIcon(nCount, resIcon, resFallback, bExecuting, 0);
                }
            }
        }
    }
    for (int i = nCount; i < 4; i++)
        m_actionQueueIcons[i].m_tail.bVisible = false;
}

void CSWGuiMainInterface::ShowFlashingStatus(unsigned char nStatus) {
    switch (nStatus) {
    case CSWGuiMainInterfaceStatus::STATUS_XP: m_statusIcons[CSWGuiMainInterfaceStatus::STATUS_STEALTH_XP].m_label.m_tail.bVisible = 0; break;
    case CSWGuiMainInterfaceStatus::STATUS_STEALTH_XP: m_statusIcons[CSWGuiMainInterfaceStatus::STATUS_XP].m_label.m_tail.bVisible = 0; break;
    case CSWGuiMainInterfaceStatus::STATUS_DARK_SHIFT: m_statusIcons[CSWGuiMainInterfaceStatus::STATUS_LIGHT_SHIFT].m_label.m_tail.bVisible = 0; break;
    case CSWGuiMainInterfaceStatus::STATUS_LIGHT_SHIFT: m_statusIcons[CSWGuiMainInterfaceStatus::STATUS_DARK_SHIFT].m_label.m_tail.bVisible = 0; break;
    }
    if (!m_statusIcons[nStatus].m_label.m_tail.bVisible)
        m_statusIcons[nStatus].m_label.m_tail.bVisible = 1;
    m_statusIcons[nStatus].m_flashTimer = 4.0f;
    m_statusIcons[nStatus].m_nShowFlags |= 1;
}

void CSWGuiMainInterfaceStatus::Show(int n) {
    if (n) {
        if (!m_label.m_tail.bVisible)
            m_label.m_tail.bVisible = 1;
        m_flashTimer = 4.0f;
        m_nShowFlags |= 1;
    } else {
        m_label.m_tail.bVisible = 0;
    }
}

void CSWGuiMainInterfaceStatus::ShiftControl(int n) {
    CSWGuiExtent sExtent = m_label.m_extent;
    sExtent.y += n;
    m_label.CSWGuiLabel::SetExtent(sExtent);
}

void CSWGuiMainInterfaceStatus::Update(float fDelta) {
#ifdef K2_UI_XBOX
    k2console_tick();
#endif
    if (m_label.m_tail.bVisible) {
        if (fDelta > 0.0f && (m_nShowFlags & 1)) {
            if (m_nStatusIndex) {
                g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(
                    CGuiInGame::GUI_STATUS_NEW_QUEST_SOUND, 0);
            }
            m_nShowFlags &= ~1;
        }
        m_flashTimer -= fDelta;
        if (m_flashTimer <= 0.0f)
            m_label.m_tail.bVisible = 0;
    }
}

#ifdef K2_UI_XBOX
static void InitStatusControl(CSWGuiMainInterface* pInterface, CSWGuiLabel& label,
                              const CExoString& sName, int bOffsetToEdge)
{
    const int nSavedMipc = g_scaleMipc;
    g_scaleMipc = 0;
    pInterface->InitControl(label, sName, 1, 1, 0, 0, 0);
    g_scaleMipc = nSavedMipc;

    CSWGuiManager* pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    if (!pGuiManager)
        return;

    const float fScreenW = static_cast<float>(pGuiManager->m_nScreenWidth);
    const float fScreenH = static_cast<float>(pGuiManager->m_nScreenHeight);
    const float fScaleX = 1.3333334f / (fScreenW / fScreenH);

    CSWGuiExtent extent = label.m_extent;
    const float fL = static_cast<float>(extent.x);
    const float fR = static_cast<float>(extent.x) + static_cast<float>(extent.w);
    const bool bNearEdge = ((fL + fR) * 0.5f < fScreenW * 0.5f);
    const float fNewL = bNearEdge ? fL * fScaleX : fScreenW - (fScreenW - fL) * fScaleX;
    const float fNewR = bNearEdge ? fR * fScaleX : fScreenW - (fScreenW - fR) * fScaleX;
    extent.x = static_cast<int>(fNewL);
    extent.w = static_cast<int>(fNewR - fNewL);
    if (bOffsetToEdge)
        extent.x += extent.x >= g_nScreenWidth / 2 ? -pGuiManager->m_nUIEdgeInset
                                                   :  pGuiManager->m_nUIEdgeInset;
    label.CSWGuiLabel::SetExtent(extent);
}
#endif

#if KOTOR1
// K1: the same nine labels, re-anchored to the bottom-left HUD unit after binding.
void CSWGuiMainInterfaceStatus::Initialize(CSWGuiMainInterface* pInterface, unsigned char nIndex) {
    CExoString sName;
    m_nStatusIndex = nIndex;
    switch (nIndex) {
    case STATUS_JOURNAL: sName = "LBL_JOURNAL"; break;
    case STATUS_DARK_SHIFT: sName = "LBL_DARKSHIFT"; break;
    case STATUS_XP: sName = "LBL_PLOTXP"; break;
    case STATUS_STEALTH_XP: sName = "LBL_STEALTHXP"; break;
    case STATUS_ITEM_RECEIVED: sName = "LBL_ITEMRCVD"; break;
    case STATUS_CREDITS: sName = "LBL_CASH"; break;
    case STATUS_LIGHT_SHIFT: sName = "LBL_LIGHTSHIFT"; break;
    case STATUS_NET_SHIFT: m_label.m_tail.bVisible = 0; return;
    case STATUS_ITEM_LOST: sName = "LBL_ITEMLOST"; break;
    }
    pInterface->InitControl(m_label, sName, 1);
    CSWGuiExtent extent = m_label.m_extent;
    AndroidExtentShift(extent, 1, 1);
    m_label.SetExtent(extent);
    m_label.m_tail.bVisible = 0;
    m_label.m_border.m_params.SetPulsingAlpha(1, 0, 1);
}
#endif

void CSWGuiMainInterfaceStatus::Initialize(CSWGuiMainInterface* pInterface, unsigned char nIndex, int c) {
    CExoString sName;
    m_nStatusIndex = nIndex;
    switch (nIndex) {
    case STATUS_JOURNAL: sName = "LBL_JOURNAL"; break;
    case STATUS_DARK_SHIFT: sName = "LBL_DARKSHIFT"; break;
    case STATUS_XP: sName = "LBL_PLOTXP"; break;
    case STATUS_STEALTH_XP: sName = "LBL_STEALTHXP"; break;
    case STATUS_ITEM_RECEIVED: sName = "LBL_ITEMRCVD"; break;
    case STATUS_CREDITS: sName = "LBL_CASH"; break;
    case STATUS_LIGHT_SHIFT: sName = "LBL_LIGHTSHIFT"; break;
    case STATUS_NET_SHIFT: m_label.m_tail.bVisible = 0; return;
    case STATUS_ITEM_LOST: sName = "LBL_ITEMLOST"; break;
    }
#ifdef K2_UI_XBOX
    InitStatusControl(pInterface, m_label, sName, c);
#else
    pInterface->InitControl(m_label, sName, 1, 1, c, 0, 0);
#endif
    m_label.m_tail.bVisible = 0;
    m_label.m_border.m_params.SetPulsingAlpha(1, 0, 1);
}

static int FindFloatyText(const CExoArrayList<CSWGuiFloatyText*>& list, CSWGuiFloatyText* pText) {
    for (int i = 0; i < list.m_count; i++)
        if (list.m_pArray[i] == pText)
            return i;
    return -1;
}

void CSWGuiMainInterface::RemoveFloatyText(CSWGuiFloatyText* pText) {
    if (pText) {
        int nIndex = FindFloatyText(m_floatyTexts, pText);
        if (nIndex != -1)
            m_floatyTexts.DelIndex(nIndex);
        delete pText;
    }
}

void CSWGuiMainInterface::AddFloatyText(unsigned long long objId, const CExoString& sText,
                                        const Vector& pos, float f) {
    CSWGuiFloatyText* pText = new CSWGuiFloatyText();
    pText->CreateEntry(objId, sText, pos, f);
    for (int i = m_floatyTexts.m_count - 1; i >= 0; i--) {
        CSWGuiFloatyText* pOld = m_floatyTexts.m_pArray[i];
        if (pOld->m_objId == objId && !pOld->IncrementPosition()) {
            delete pOld;
            m_floatyTexts.DelIndex(i);
        }
    }
    m_floatyTexts.Add(pText);
}

void CSWGuiFloatyText::CreateEntry(unsigned long long objId, const CExoString& sText,
                                   const Vector& vColor, float f) {
#if KOTOR1
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
#endif
    m_fTimeRemaining = f;
    m_fTotalTime = f;
    m_nPositionStep = 1;
    m_objId = objId;
    m_text.m_params.SetAlignment(SWGUIALIGN_MIDDLE | SWGUIALIGN_CENTER);
    m_text.m_params.SetText(sText);
    m_text.m_params.SetColor(vColor);
    m_text.m_params.m_fAlpha = 1.0f;
    m_text.m_params.SetBaseFont(CResRef("fnt_d16x16"));
    Vector vPoint;  // zeroed by Vector's default ctor
    CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_objId);
    if (pGameObject) {
        CSWCObject* pObject = static_cast<CSWCObject*>(pGameObject->AsSWCObject());
        if (pObject) {
            pObject->GetReticlePoint(vPoint);
#if KOTOR1  // K1: the base extent is set first, then the label re-lays itself out
            CSWGuiExtent extent;
#ifdef K2_FIX_ENGINE_BUGS
            // Grows the box and its gap over the reticle point with the font: a fixed 200 pixels held
            // six glyphs at 2160 lines.
            extent.w = static_cast<int>(200.0f * s_fScale);
            extent.h = m_text.GetFontHeight();
            extent.y = static_cast<int>(vPoint.y - 32.0 * s_fScale - extent.h);
            extent.x = static_cast<int>(vPoint.x - 100.0f * s_fScale);
#else
            extent.w = 200;
            extent.h = m_text.GetFontHeight();
            extent.y = static_cast<int>(vPoint.y - 32.0 - extent.h);
            extent.x = static_cast<int>(vPoint.x - 100.0f);
#endif
#ifdef K2_QOL_IMPROVEMENTS
            K2QolGrowFloatyText(m_text, extent);
#endif
            CSWGuiObject::SetExtent(extent);
            SetExtent(m_extent);
            if (m_text.m_pString)
                m_text.m_pString->SetFontScale(s_fScale);
#else
#ifdef K2_FIX_ENGINE_BUGS
            // Grows the box and its gap over the reticle point with the text scalar.
            float fBoxScale = GetResolutionTextScalar();
            if (fBoxScale < 1.0f)
                fBoxScale = 1.0f;
            m_extent.w = static_cast<int>(200.0f * fBoxScale);
            m_extent.h = m_text.GetFontHeight();
            m_extent.y = static_cast<int>(vPoint.y - 32.0 * fBoxScale - m_extent.h);
            m_extent.x = static_cast<int>(vPoint.x - 100.0f * fBoxScale);
#else
            m_extent.w = 200;
            m_extent.h = m_text.GetFontHeight();
            m_extent.y = static_cast<int>(vPoint.y - 32.0 - m_extent.h);
            m_extent.x = static_cast<int>(vPoint.x - 100.0f);
#endif
#ifdef K2_QOL_IMPROVEMENTS
            // Damage, healing and experience numbers follow the Text Size.
            K2QolGrowFloatyText(m_text, m_extent);
#endif
            SetExtent(m_extent);
#endif
            if (pObject->AsSWCCreature()) {
                Gob* pModel = pObject->GetModel(0xff, 1);
                CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
                Vector vScreen = pCamera->MapToScreen(pModel->GetPosition(), true);
                m_nScreenX = static_cast<short>(vScreen.x *
                    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth);
                m_nScreenY = static_cast<short>(vScreen.y *
                    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight);
            }
        }
    }
}

CSWGuiMainInterface* CSWGuiFloatyText::m_pMainInterface;


void CSWGuiFloatyText::Update(float fDelta) {
    m_fTimeRemaining -= fDelta;
    if (m_fTimeRemaining <= 0.0f) {
        m_pMainInterface->RemoveFloatyText(this);
        return;
    }
    CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_objId);
    if (pGameObject) {
        CSWCObject* pObject = static_cast<CSWCObject*>(pGameObject->AsSWCObject());
        if (pObject && pObject->AsSWCCreature()) {
            Gob* pModel = pObject->GetModel(0xff, 1);
            CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
            Quaternion q = pCamera->GetOrientation();
            float fA1 = (q.x * q.z + q.w * q.y) * -2.0f;
            float fA2 = (q.z * q.y - q.x * q.w) * -2.0f;
            float fA3 = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
            Vector vPos = pModel->GetPosition();
            float fPoint = vPos.x * fA1 + vPos.y * fA2 - vPos.z * fA3;
            Vector vCamPos = pCamera->GetPosition();
            float fCam = fA1 * vCamPos.x + fA2 * vCamPos.y - fA3 * vCamPos.z;
            if (fPoint > fCam) {
                Vector vScreen = pCamera->MapToScreen(pModel->GetPosition(), true);
                int nX = static_cast<int>(vScreen.x *
                    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth);
                int nY = static_cast<int>(vScreen.y *
                    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight);
#if KOTOR1
                CSWGuiExtent extent = m_extent;
                extent.y += nY - m_nScreenY;
                extent.x += nX - m_nScreenX;
                CSWGuiObject::SetExtent(extent);
                m_nScreenY = static_cast<short>(nY);
                m_nScreenX = static_cast<short>(nX);
                SetExtent(m_extent);
#else
                m_extent.y += nY - m_nScreenY;
                m_extent.x += nX - m_nScreenX;
                m_nScreenY = static_cast<short>(nY);
                m_nScreenX = static_cast<short>(nX);
                SetExtent(m_extent);
#endif
                m_tail.bVisible = 1;
            } else {
                m_tail.bVisible = 0;
            }
        }
    }
    m_text.m_params.m_fAlpha = m_fTimeRemaining / m_fTotalTime;
}


bool CSWGuiFloatyText::IncrementPosition() {
    if (++m_nPositionStep > 5)
        return false;
#if KOTOR1
    CSWGuiExtent extent = m_extent;
    extent.y -= extent.h;
    CSWGuiObject::SetExtent(extent);
    SetExtent(m_extent);
#else
    m_extent.y -= m_extent.h;
    SetExtent(m_extent);
#endif
    return true;
}


bool CSWGuiMainInterface::CombatEffectivenessIncreased(CSWSCreature* pCreature) {
    if (pCreature->m_pStats->GetTotalCHABonus() > 0) return true;
    if (pCreature->m_pStats->GetTotalCONBonus() > 0) return true;
    if (pCreature->m_pStats->GetTotalDEXBonus() > 0) return true;
    if (pCreature->m_pStats->GetTotalINTBonus() > 0) return true;
    if (pCreature->m_pStats->GetTotalSTRBonus() > 0) return true;
    if (pCreature->m_pStats->GetTotalWISBonus() > 0) return true;
    if (pCreature->m_pStats->m_iFirstSavingThrowIncrease) return true;
    int nIndex = pCreature->m_pStats->m_iFirstAttackIncrease;
    if (pCreature->m_effects.m_count > nIndex) {
        int nType = pCreature->m_effects.m_pArray[nIndex]->m_subType;
        if (nType <= CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE) {
            if (nType == CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE) return true;
            if (nType == CSWSEffectListHandler::EFFECT_ATTACK_INCREASE) return true;
        }
    }
    return false;
}

bool CSWGuiMainInterface::CombatEffectivenessReduced(CSWSCreature* pCreature) {
    if (pCreature->m_pStats->GetTotalCHABonus() < 0) return true;
    if (pCreature->m_pStats->GetTotalCONBonus() < 0) return true;
    if (pCreature->m_pStats->GetTotalDEXBonus() < 0) return true;
    if (pCreature->m_pStats->GetTotalINTBonus() < 0) return true;
    if (pCreature->m_pStats->GetTotalSTRBonus() < 0) return true;
    if (pCreature->m_pStats->GetTotalWISBonus() < 0) return true;
    int nIndex = pCreature->m_pStats->m_iFirstAttackIncrease;
    if (pCreature->m_effects.m_count > nIndex) {
        int nType = pCreature->m_effects.m_pArray[nIndex]->m_subType;
        if (nType <= CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE) {
            if (nType == CSWSEffectListHandler::EFFECT_ATTACK_DECREASE) return true;
            if (nType == CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE) return true;
        }
    }
    return false;
}

void CSWGuiMainInterface::CSWGamepadMenuIos::Update(float f) {
    K2_UNUSED(f);
    UpdateStealth();
}


HIDDEN unsigned char g_nHelpMode;
#if !KOTOR1
HIDDEN int           g_nHelpPage;
HIDDEN int           g_nHelpSavedInputClass;
HIDDEN int           g_nHelpSavedPausedByCombat;
#endif
HIDDEN int           g_nGamepadMenuSuppressed;
HIDDEN unsigned char g_bHelpSecondPagePending;


float IOS_ACTION_MENU_GROW_X;
unsigned char IosActionMenuUp;
extern unsigned char g_initiatedByController;
int IosActionMenuCount;


#if KOTOR1
static void ShowHelpInternal(int nPage) {
    if (nPage >= 4 && !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bTutorialWindows)
        return;
#ifndef K2_UI_TOUCH
    // Pages 4-7 are the ios_tut_* cards: pictures of the touch HUD, placed against its widgets.
    if (nPage >= 4)
        return;
#endif
    g_nHelpPage = nPage;
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (g_pGuiMan->PanelExists(&pInGameGui->m_pMainInterface->m_helpPanel))
        return;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->AddHelpPanel(nPage);
    g_nHelpSavedPausedByCombat = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
    if (!g_nHelpSavedPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 0, 0);
    g_nHelpSavedInputClass = g_pAppManager->m_pClientExoApp->GetInputClass();
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 0);
}
#else
static void ShowHelpInternal(int nPage, bool bGamepad) {
    bool bJoystick = IsJoystickConnected();
    if (nPage >= 3 && bJoystick)
        return;
    g_pAppManager->m_pClientExoApp->GetClientOptions();
    bool bTouch = HasTouchScreen();
    if (nPage > 3 && !bTouch)
        return;
    g_nHelpPage = nPage;
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (bGamepad) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->AddHelpPanel(nPage, true);
        return;
    }
    if (g_pGuiMan->PanelExists(&pInGameGui->m_pMainInterface->m_helpPanel))
        return;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->AddHelpPanel(nPage, false);
#if defined(K2_HOST_BUILD) && !defined(K2_UI_TOUCH)
    if (!g_pGuiMan->PanelExists(&pInGameGui->m_pMainInterface->m_helpPanel))
        return;
#endif
    g_nHelpSavedPausedByCombat = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
    if (!g_nHelpSavedPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 13, 0);
    g_nHelpSavedInputClass = g_pAppManager->m_pClientExoApp->GetInputClass();
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 0);
}
#endif

void CSWGuiMainInterface::ShowHelp(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
#if KOTOR1
    ShowHelpInternal(0);
#else
    g_nHelpMode = 1;
    ShowHelpInternal(0, false);
#endif
}

#if KOTOR1
void CSWGuiMainInterface::HideHelp(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    g_pGuiMan->RemovePanel(&m_helpPanel);
    CResRef sNoBackground;
    m_helpPanel.SetBackground(sNoBackground);
    if (g_nHelpPage == 1) {
        m_btnHelpTopLeft.m_border.m_params.SetFillImage(CResRef(""), 0);
        m_btnHelpTopLeft.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
        m_btnHelpTopRight.m_border.m_params.SetFillImage(CResRef(""), 0);
        m_btnHelpTopRight.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
        m_btnHelpBottomLeft.m_border.m_params.SetFillImage(CResRef(""), 0);
        m_btnHelpBottomLeft.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
        m_btnHelpBottomRight.m_border.m_params.SetFillImage(CResRef(""), 0);
        m_btnHelpBottomRight.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    }
    int nPages = ASLPlat_GetControllerPlugged() ? 4 : 3;
    if (++g_nHelpPage < nPages)
        AddHelpPanel(g_nHelpPage);
    if (g_nHelpPage >= nPages) {
        g_nHelpPage = 0;
        g_pAppManager->m_pClientExoApp->SetInputClass(g_nHelpSavedInputClass, 0);
        if (!g_nHelpSavedPausedByCombat)
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
    }
}
#else
void CSWGuiMainInterface::HideHelp(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    g_pGuiMan->RemovePanel(&m_helpPanel);
    m_helpPanel.SetBackground(CResRef());
    m_btnGamepadHelp.m_tail.bVisible = 0;
    if (g_nHelpMode == 1) {
        if (++g_nHelpPage <= 2)
            AddHelpPanel(g_nHelpPage, false);
    }
    if (g_nHelpPage > 2 || !(g_nHelpMode & 1)) {
        g_nHelpPage = 0;
        g_nHelpMode = 0;
        g_pAppManager->m_pClientExoApp->SetInputClass(g_nHelpSavedInputClass, 0);
        if (!g_nHelpSavedPausedByCombat)
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
    }
}
#endif


void CSWGuiMainInterface::CSWGuiHelpPanel::HandleInputEvent(int nType, int nValue) {
    K2_UNUSED(nValue);
#if KOTOR1
    if (nType == INPUTEVENT_GUI_A)
        m_pInterface->HideHelp(0);
#else
    if (nType != INPUTEVENT_GUI_A && nType != INPUTEVENT_GUI_B) {
        if (nType != INPUTEVENT_GUI_BACK)
            return;
        g_nHelpMode = 0;
    }
    m_pInterface->HideHelp(0);
#endif
}


void CSWGuiMainInterface::CSWGuiActionMenuIos::HandleInputEvent(int nType, int nValue) {
    K2_UNUSED(nValue);
    int nStep = -1;
    switch (nType) {
    case INPUTEVENT_GUI_A: {
        CSWGuiMainInterface* pInterface = m_pInterface;
        int nSelected = pInterface->GetActionMenuSelected();
        if (nSelected < 0)
            break;
#if KOTOR1
        pInterface->HandleButtonAction(pInterface->m_actionMenuIos.m_nActionIndex, nSelected);
#else
        g_initiatedByController = 1;
        pInterface->HandleButtonAction(pInterface->m_actionMenuIos.m_nActionIndex, nSelected);
        g_initiatedByController = 0;
#endif
        break;
    }
    case INPUTEVENT_GUI_B:
        m_pInterface->HandleButtonActionX(0);
        break;
    case INPUTEVENT_GUI_DPAD_DOWN:
        nStep = 1;
        // fall through
    case INPUTEVENT_GUI_DPAD_UP:
        if (IosActionMenuCount)
            m_pInterface->UpdateActionMenuSelected(nStep);
        break;
    }
}

#if KOTOR1
void CSWGuiMainInterface::PopupGamepadMenu() {
    if (g_nOpenMapCountdown || g_pGuiMan->PanelExists(&m_gamepadMenu))
        return;
    g_pGuiMan->AddPanel(&m_gamepadMenu, 1, 1);
    m_gamepadMenu.InitGamepadMenu();
}
#else
void CSWGuiMainInterface::PopupGamepadMenu() {
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    if (pInGameGui->m_nGlobalDialogState || g_nGamepadMenuSuppressed)
        return;
    if (g_pGuiMan->PanelExists(&m_gamepadMenu))
        return;
    g_pGuiMan->AddPanel(&m_gamepadMenu, 1, 1);
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 0);
    m_gamepadMenu.InitGamepadMenu();
}
#endif

CSWGuiInterfaceAction* CSWGuiMainInterface::GetActionsFromActionIndex(int nIndex, int& nCount,
                                                                     unsigned int* pId) {
    nCount = 0;
    if (nIndex < 0)
        return 0;
    if (nIndex <= CSWGuiMainInterfaceAction::ACTION_SLOT_FORM) {
        if (pId)
            *pId = m_personalActionIds[nIndex];
        nCount = m_personalActions[nIndex].m_count;
        if (nCount < 1)
            return 0;
        return m_personalActions[nIndex].m_pArray;
    }
    if (pId) {
        int* pRow = m_targetActionMenu.m_actionIds[static_cast<signed char>(m_targetActionMenu.m_actionRow)];
        *pId = pRow[nIndex - CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT];
    }
    nCount = m_targetActionMenu.m_actions[nIndex - CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT].m_count;
    if (nCount < 1)
        return 0;
    return m_targetActionMenu.m_actions[nIndex - CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT].m_pArray;
}

#if KOTOR1
// K1: no id out-parameter; four personal lists, then the target menu's three.
CSWGuiInterfaceAction* CSWGuiMainInterface::GetActionsFromActionIndex(int nIndex, int& nCount) {
    nCount = 0;
    if (nIndex < 0)
        return 0;
    if (nIndex <= 3) {
        nCount = m_personalActions[nIndex].m_count;
        if (nCount < 1)
            return 0;
        return m_personalActions[nIndex].m_pArray;
    }
    nCount = m_targetActionMenu.m_actions[nIndex - 4].m_count;
    if (nCount < 1)
        return 0;
    return m_targetActionMenu.m_actions[nIndex - 4].m_pArray;
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::ShowHelpForString(int nStrRef) {
    int nPage;
    switch (nStrRef) {
    case STRREF_HELP_PARTY_MEMBER_JOINED:
        nPage = 4;
        break;
    case STRREF_ENEMY_SIGHTED:
        if (g_bHelpSecondPagePending != 1)
            return;
        g_bHelpSecondPagePending = 0;
        nPage = 2;
        break;
    case STRREF_HELP_EQUIP_WEAPON:
        g_bHelpSecondPagePending = 1;
        nPage = 5;
        break;
    case STRREF_HELP_T3_MOVEMENT:
        ShowHelpInternal(0, false);
        if (!HasJoystick())
            return;
        g_nQueuedHelpPage = 3;
        g_bQueuedHelpGamepad = 1;
        g_nQueuedHelpDelay = 1;
        return;
    default:
        return;
    }
    ShowHelpInternal(nPage, false);
}
#endif // !KOTOR1

CSWGuiMainInterfaceAction* CSWGuiMainInterface::GetActionByIndex(int nIndex) {
    if (nIndex < 0)
        return 0;
#if KOTOR1
    if (nIndex <= 3)
        return &m_personalActionWidgets[nIndex];
    if (nIndex <= 6)
        return &m_targetActionMenu.m_widgets[nIndex - 4];
#else
    if (nIndex <= CSWGuiMainInterfaceAction::ACTION_SLOT_FORM)
        return &m_personalActionWidgets[nIndex];
    if (nIndex <= CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_RIGHT)
        return &m_targetActionMenu.m_widgets[nIndex - CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT];
#endif
    return 0;
}

int CSWGuiMainInterface::GetActiveActionIdx(int nIndex) {
    int nActionId = m_personalActionIds[nIndex];
    if (nActionId == -1)
        return 0;
    int nCount = m_personalActions[nIndex].m_count;
    if (nCount < 1)
        return 0;
    CSWGuiInterfaceAction* pActions = m_personalActions[nIndex].m_pArray;
    for (int64_t i = 0; i < nCount; i++) {
        if (static_cast<int>(pActions[i].m_nActionId) == nActionId)
            return static_cast<int>(i);
    }
    return 0;
}

void CSWGuiMainInterface::InitializeMap() {
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    CExoString sModuleRef;
    CExoString sMapImage;
    pModule->m_resRef.CopyToString(sModuleRef);
    sMapImage.Format(const_cast<char*>("lbl_map%s"), sModuleRef.CStr());
    CSWSAreaMap* pAreaMap = pModule->m_pAreaMap;
    int bHasMap;
    if (pAreaMap->m_nSize) {
        float fScale = pAreaMap->m_fMapScale;
        m_miniMapImage.m_border.m_params.SetFillImage(CResRef(sMapImage), 0);
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)
        m_miniMapImage.m_border.m_params.m_flags.nFillStyle = 0;
#else
        m_miniMapImage.m_border.m_params.m_flags.nFillStyle = 2;
#endif
        CSWGuiExtent extent;
#if KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
        // The 512x256 map art stretched by the HUD scale, as the window around it is.
        extent.w = static_cast<int>(512.0f * K1MiniMapZoom() * fScale);
        extent.h = static_cast<int>(256.0f * K1MiniMapZoom() * fScale);
#else
        extent.w = static_cast<int>(m_nMapWidth * fScale);
        extent.h = static_cast<int>(m_nMapHeight * fScale);
#endif
        extent.x = 0;
        extent.y = 0;
        m_miniMapImage.SetExtent(extent);
        bHasMap = 1;
    } else {
        bHasMap = 0;
    }
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions) {
        int bVisible;
        if (bHasMap) {
            m_miniMap.m_tail.bVisible = pOptions->m_bFeedback3;
            bVisible = pOptions->m_bFeedback3;
        } else {
            m_miniMap.m_tail.bVisible = 0;
            bVisible = 0;
        }
        m_lblArrow.m_tail.bVisible = bVisible;
    }
}

#if KOTOR1
void CSWGuiMainInterface::CSWGamepadMenuIos::InitGamepadMenu() {
    m_bWasPausedByCombat = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
    if (!m_bWasPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 0, 0);
    m_nSelectedEntry = 0;
    m_nSelectedPartySlot = 0;
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_soloMode) {
        m_menuButtons[GPAD_ENTRY_SOLO].m_border.m_params.SetFillImage(CResRef("ios_gpad_solox"), 0);
        m_menuButtons[GPAD_ENTRY_SOLO].m_border.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_SOLO].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_solox2"), 0);
    } else {
        m_menuButtons[GPAD_ENTRY_SOLO].m_border.m_params.SetFillImage(CResRef("ios_gpad_solo"), 0);
        m_menuButtons[GPAD_ENTRY_SOLO].m_border.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_SOLO].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_solo2"), 0);
    }
    m_menuButtons[GPAD_ENTRY_SOLO].m_borderHilight.m_params.m_flags.nFillStyle = 2;
    UpdateStealth();
    m_nLastPartySlot = 0;
    for (int i = 0; i < 3; i++) {
        CSWGuiButton& rPortrait = m_portraitButtons[i];
        CSWGuiButton& rFrame = m_frameButtons[i];
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        CSWCCreature* pMember = pParty ? pParty->GetCharacter(i) : 0;
        if (pMember)
            pMember->GetServerCreature();
        if (pMember && i < pParty->m_nCharacterCount) {
            m_nLastPartySlot = i;
            rPortrait.m_tail.bVisible = 1;
            rFrame.m_tail.bVisible = 1;
            rPortrait.m_border.m_params.SetFillImage(pMember->GetPortrait(1), 0);
            rPortrait.m_border.m_params.m_flags.nFillStyle = 2;
            rPortrait.m_borderHilight.m_params.SetFillImage(pMember->GetPortrait(1), 0);
            rPortrait.m_borderHilight.m_params.m_flags.nFillStyle = 2;
        } else {
            rPortrait.m_tail.bVisible = 0;
            rFrame.m_tail.bVisible = 0;
        }
        if (SDL_IsChromebook()) {
            rPortrait.m_tail.nControlId = i;
            rPortrait.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                &CSWGuiMainInterface::CSWGamepadMenuIos::OnCharacterSelect));
        }
    }
    LayoutGamepadMenu(false);
}

// K1: the six-entry menu's own handlers. Each un-pauses the game unless the menu found it paused,
// acts, and closes the menu.
void CSWGuiMainInterface::CSWGamepadMenuIos::OnCharacterSelect(CSWGuiControl* pControl) {
    if (!m_bWasPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
    g_pAppManager->m_pClientExoApp->ChangeCharacter(pControl->m_tail.nControlId == 2 ? 2 : 1, 1);
    UpdateStealth();
    g_pGuiMan->RemovePanel(this);
}

void CSWGuiMainInterface::CSWGamepadMenuIos::OnMenusButtonClicked(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    if (!m_bWasPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowSWInGameGui(7))
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 1);
    g_pGuiMan->RemovePanel(this);
}

void CSWGuiMainInterface::CSWGamepadMenuIos::OnSoloButtonClicked(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowSoloModeQuery(0);
    g_pGuiMan->RemovePanel(this);
    if (!m_bWasPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
}

void CSWGuiMainInterface::CSWGamepadMenuIos::OnStealthButtonClicked(CSWGuiControl* pControl) {
    if (!m_bWasPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
    if (pControl->m_tail.nCurrentEventArg &&
        g_pAppManager->m_pClientExoApp->GetPlayerCreature()->IsStealthCapable())
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter()->PartyMenuStealth(0, nullptr);
    g_pGuiMan->RemovePanel(this);
}

void CSWGuiMainInterface::CSWGamepadMenuIos::QuickSave(CSWGuiControl* pControl) {
    reinterpret_cast<CSWGuiMainInterface*>(this)->Quicksave(pControl);  // idiom-exempt: the original passes the menu as the interface; Quicksave reads no member
    g_pGuiMan->RemovePanel(this);
}

void CSWGuiMainInterface::CSWGamepadMenuIos::ShowHelp(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    if (!m_bWasPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
    ShowHelpInternal(0);
    g_pGuiMan->RemovePanel(this);
}
#else
void CSWGuiMainInterface::CSWGamepadMenuIos::InitGamepadMenu() {
    m_bWasPausedByCombat = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
    if (!m_bWasPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 0, 0);
    m_nSelectedEntry = 0;
    m_nSelectedPartySlot = 0;
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_soloMode) {
        m_menuButtons[GPAD_ENTRY_SOLO].m_border.m_params.SetFillImage(CResRef("cus_gpad_solox"), 0);
        m_menuButtons[GPAD_ENTRY_SOLO].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_solox2"), 0);
    } else {
        m_menuButtons[GPAD_ENTRY_SOLO].m_border.m_params.SetFillImage(CResRef("cus_gpad_solo"), 0);
        m_menuButtons[GPAD_ENTRY_SOLO].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_solo2"), 0);
    }
    UpdateStealth();
    m_nLastPartySlot = 0;
    for (int i = 0; i < 3; i++) {
        CSWGuiButton& rPortrait = m_portraitButtons[i];
        CSWGuiButton& rFrame = m_frameButtons[i];
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        CSWCCreature* pMember = pParty ? pParty->GetCharacter(i) : 0;
        if (pMember)
            pMember->GetServerCreature();
        if (pMember && i < pParty->m_nCharacterCount) {
            m_nLastPartySlot = i;
            rPortrait.m_tail.bVisible = 1;
            rFrame.m_tail.bVisible = 1;
            rPortrait.m_border.m_params.SetFillImage(pMember->GetPortrait(1), 0);
            rPortrait.m_borderHilight.m_params.SetFillImage(pMember->GetPortrait(1), 0);
        } else {
            rPortrait.m_tail.bVisible = 0;
            rFrame.m_tail.bVisible = 0;
        }
        rPortrait.m_border.m_params.m_flags.nFillStyle = 2;
        rPortrait.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    }
    LayoutGamepadMenu(false);
}
#endif

#if KOTOR1
extern "C" int __android_log_print(int nPriority, const char* pTag, const char* pFormat, ...);

#endif

CSWGuiMainInterface::CSWGamepadMenuIos::CSWGamepadMenuIos(CSWGuiManager* pGuiManager,
                                                         CSWGuiMainInterface* pInterface)
    : CSWGuiPanel(pGuiManager), m_pInterface(pInterface) {
    m_nSelectedEntry = -1;
    m_bWasPausedByCombat = 0;
}
CSWGuiMainInterface::CSWGamepadMenuIos::~CSWGamepadMenuIos() {
}


void CSWGuiMainInterface::CSWGamepadMenuIos::UpdateStealth() {
    int bStealthCapable = g_pAppManager->m_pClientExoApp->GetPlayerCreature()->IsStealthCapable();
    m_bStealthCapable = (bStealthCapable != 0);
#if KOTOR1
    if (bStealthCapable) {
        m_menuButtons[GPAD_ENTRY_STEALTH].m_border.m_params.SetFillImage(CResRef("ios_gpad_ste"), 0);
        m_menuButtons[GPAD_ENTRY_STEALTH].m_border.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_STEALTH].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_ste2"), 0);
    } else {
        m_menuButtons[GPAD_ENTRY_STEALTH].m_border.m_params.SetFillImage(CResRef("ios_gpad_ste3"), 0);
        m_menuButtons[GPAD_ENTRY_STEALTH].m_border.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_STEALTH].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_ste3"), 0);
    }
    m_menuButtons[GPAD_ENTRY_STEALTH].m_borderHilight.m_params.m_flags.nFillStyle = 2;
#else
    if (bStealthCapable) {
        m_menuButtons[GPAD_ENTRY_STEALTH].m_border.m_params.SetFillImage(CResRef("cus_gpad_ste"), 0);
        m_menuButtons[GPAD_ENTRY_STEALTH].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_ste2"), 0);
    } else {
        m_menuButtons[GPAD_ENTRY_STEALTH].m_border.m_params.SetFillImage(CResRef("cus_gpad_ste3"), 0);
        m_menuButtons[GPAD_ENTRY_STEALTH].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_ste3"), 0);
    }
#endif
}

#if KOTOR1
void CSWGuiMainInterface::CSWGamepadMenuIos::LayoutGamepadMenu(bool bInitImages) {
    CSWGuiExtent extent(0, 0, 0, 0);
    if (bInitImages) {
        m_buttonBackground.m_border.m_params.SetFillImage(CResRef("ios_gpad_bg"), 0);
        m_buttonBackground.m_border.m_params.m_flags.nFillStyle = 2;
        m_buttonBackground.m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_bg"), 0);
        m_buttonBackground.m_borderHilight.m_params.m_flags.nFillStyle = 2;
        AddControl(&m_buttonBackground);
        for (int j = 0; j < 3; j++) {
            m_frameButtons[j].m_border.m_params.SetFillImage(CResRef("ios_gpad_gen"), 0);
            m_frameButtons[j].m_border.m_params.m_flags.nFillStyle = 2;
            m_frameButtons[j].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_gen2"), 0);
            m_frameButtons[j].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        }
        m_menuButtons[GPAD_ENTRY_MAP].m_border.m_params.SetFillImage(CResRef("ios_gpad_map"), 0);
        m_menuButtons[GPAD_ENTRY_MAP].m_border.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_MAP].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_map2"), 0);
        m_menuButtons[GPAD_ENTRY_MAP].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_SOLO].m_border.m_params.SetFillImage(CResRef("ios_gpad_solo"), 0);
        m_menuButtons[GPAD_ENTRY_SOLO].m_border.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_SOLO].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_solo2"), 0);
        m_menuButtons[GPAD_ENTRY_SOLO].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_STEALTH].m_border.m_params.SetFillImage(CResRef("ios_gpad_ste"), 0);
        m_menuButtons[GPAD_ENTRY_STEALTH].m_border.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_STEALTH].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_ste2"), 0);
        m_menuButtons[GPAD_ENTRY_STEALTH].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_QUICKSAVE].m_border.m_params.SetFillImage(CResRef("ios_gpad_save"), 0);
        m_menuButtons[GPAD_ENTRY_QUICKSAVE].m_border.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_QUICKSAVE].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_save2"), 0);
        m_menuButtons[GPAD_ENTRY_QUICKSAVE].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_HELP].m_border.m_params.SetFillImage(CResRef("ios_gpad_help"), 0);
        m_menuButtons[GPAD_ENTRY_HELP].m_border.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_HELP].m_borderHilight.m_params.SetFillImage(CResRef("ios_gpad_help2"), 0);
        m_menuButtons[GPAD_ENTRY_HELP].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        if (SDL_IsChromebook()) {
            m_menuButtons[GPAD_ENTRY_MAP].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                &CSWGuiMainInterface::CSWGamepadMenuIos::OnMenusButtonClicked));
            m_menuButtons[GPAD_ENTRY_SOLO].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                &CSWGuiMainInterface::CSWGamepadMenuIos::OnSoloButtonClicked));
            m_menuButtons[GPAD_ENTRY_STEALTH].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                &CSWGuiMainInterface::CSWGamepadMenuIos::OnStealthButtonClicked));
            m_menuButtons[GPAD_ENTRY_QUICKSAVE].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                &CSWGuiMainInterface::CSWGamepadMenuIos::QuickSave));
            m_menuButtons[GPAD_ENTRY_HELP].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                &CSWGuiMainInterface::CSWGamepadMenuIos::ShowHelp));
        }
        for (int i = 0; i < GPAD_ENTRY_COUNT; i++) {
            IosLabel& rLabel = m_iosLabels[i];
            rLabel.SetFilterNewline(0);
            rLabel.m_bCenterText = 0;
            rLabel.m_tail.bVisible = 1;
            rLabel.SetString(CExoString(GetLocalizedGamepadString(i)));
            rLabel.SetImage(nullptr);
            rLabel.SetFontSize(26);
            AddControl(&rLabel);
            AddControl(&m_labelButtons[i]);
            m_labelButtons[i].m_text.m_params.SetColor(Vector(0.0f, 0.66f, 0.98f));
            __android_log_print(6, "KOTOR", "text missing: %s", GetLocalizedGamepadString(i));
            m_labelButtons[i].m_text.m_params.SetText(CExoString(GetLocalizedGamepadString(i)));
            m_labelButtons[i].m_text.m_params.SetAlignment(SWGUIALIGN_BOTTOM | SWGUIALIGN_RIGHT);
        }
    }

    extent = CSWGuiExtent(285, 85, 600, 597);
    m_buttonBackground.CSWGuiButton::SetExtent(extent);
    m_buttonBackground.ScaleExtentForResolution();

    int nY = 87;
    for (int i = 0; i < GPAD_ENTRY_COUNT; i++) {
        extent = CSWGuiExtent(500, nY, 86, 98);
        if (i == GPAD_ENTRY_PARTY) {
            for (int j = 0; j < 3; j++) {
                CSWGuiControl* pFrame = &m_frameButtons[j];
                CSWGuiControl* pPortrait = &m_portraitButtons[j];
                extent = CSWGuiExtent(500 + 101 * j, nY, 86, 98);
                pFrame->SetExtent(extent);
                extent.x += 11;
                extent.y += 20;
                extent.w = 66;
                extent.h = 66;
                pPortrait->SetExtent(extent);
                pFrame->ScaleExtentForResolution();
                pPortrait->ScaleExtentForResolution();
                pFrame->SetActive(m_nSelectedEntry == GPAD_ENTRY_PARTY && m_nSelectedPartySlot == j);
                if (bInitImages) {
                    AddControl(pPortrait);
                    AddControl(pFrame);
                    SendControlToBack(pFrame);
                }
            }
            extent = CSWGuiExtent(500, nY, 86, 98);
        } else {
            CSWGuiControl* pEntry = &m_menuButtons[i];
            pEntry->SetExtent(extent);
            pEntry->SetActive(i == m_nSelectedEntry);
            if (bInitImages)
                AddControl(pEntry);
            extent = pEntry->m_extent;
            pEntry->ScaleExtentForResolution();
        }
        extent.x -= 257;
        extent.y += 13;
        extent.w = 250;
        extent.h = 40;
        CSWGuiControl* pIosLabel = &m_iosLabels[i];
        CSWGuiControl* pCaption = &m_labelButtons[i];
        pIosLabel->SetExtent(extent);
        pIosLabel->m_tail.bVisible = 0;
        pCaption->SetExtent(extent);
        pIosLabel->ScaleExtentForResolution();
        pCaption->ScaleExtentForResolution();
        nY += 98;
    }
}
#else
void CSWGuiMainInterface::CSWGamepadMenuIos::LayoutGamepadMenu(bool bInitImages) {
    short nScreenW = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
    short nScreenH = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight;
    CSWGuiExtent extentRoot;

    if (bInitImages) {
        m_buttonBackground.m_border.m_params.SetFillImage(CResRef("cus_gpad_bg"), 0);
        m_buttonBackground.m_border.m_params.m_flags.nFillStyle = 2;
        m_buttonBackground.m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_bg"), 0);
        m_buttonBackground.m_borderHilight.m_params.m_flags.nFillStyle = 2;
        AddControl(&m_buttonBackground);
        m_frameButtons[0].m_border.m_params.SetFillImage(CResRef("cus_gpad_gen"), 0);
        m_frameButtons[0].m_border.m_params.m_flags.nFillStyle = 2;
        m_frameButtons[0].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_gen2"), 0);
        m_frameButtons[0].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_frameButtons[1].m_border.m_params.SetFillImage(CResRef("cus_gpad_gen"), 0);
        m_frameButtons[1].m_border.m_params.m_flags.nFillStyle = 2;
        m_frameButtons[1].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_gen2"), 0);
        m_frameButtons[1].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_frameButtons[2].m_border.m_params.SetFillImage(CResRef("cus_gpad_gen"), 0);
        m_frameButtons[2].m_border.m_params.m_flags.nFillStyle = 2;
        m_frameButtons[2].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_gen2"), 0);
        m_frameButtons[2].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_menuButtons[GPAD_ENTRY_MAP].m_border.m_params.SetFillImage(CResRef("cus_gpad_map"), 0);
        m_menuButtons[GPAD_ENTRY_MAP].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_map2"), 0);
        m_menuButtons[GPAD_ENTRY_SOLO].m_border.m_params.SetFillImage(CResRef("cus_gpad_solo"), 0);
        m_menuButtons[GPAD_ENTRY_SOLO].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_solo2"), 0);
        m_menuButtons[GPAD_ENTRY_STEALTH].m_border.m_params.SetFillImage(CResRef("cus_gpad_ste"), 0);
        m_menuButtons[GPAD_ENTRY_STEALTH].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_ste2"), 0);
        m_menuButtons[GPAD_ENTRY_QUICKSAVE].m_border.m_params.SetFillImage(CResRef("cus_gpad_save"), 0);
        m_menuButtons[GPAD_ENTRY_QUICKSAVE].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_save2"), 0);
        m_menuButtons[GPAD_ENTRY_FREE_LOOK].m_border.m_params.SetFillImage(CResRef("cus_gpad_fper"), 0);
        m_menuButtons[GPAD_ENTRY_FREE_LOOK].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_fper2"), 0);
        m_menuButtons[GPAD_ENTRY_SWITCH_WEAPONS].m_border.m_params.SetFillImage(CResRef("cus_gpad_hand"), 0);
        m_menuButtons[GPAD_ENTRY_SWITCH_WEAPONS].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_hand2"), 0);
        m_menuButtons[GPAD_ENTRY_HELP].m_border.m_params.SetFillImage(CResRef("cus_gpad_help"), 0);
        m_menuButtons[GPAD_ENTRY_HELP].m_borderHilight.m_params.SetFillImage(CResRef("cus_gpad_help2"), 0);
        for (int i = 0; i < GPAD_ENTRY_COUNT; i++) {
            AddControl(&m_labelButtons[i]);
            m_labelButtons[i].m_text.m_params.SetColor(Vector(0.0f, 0.66f, 0.98f));
            m_labelButtons[i].m_text.m_params.SetText(CExoString(GetLocalizedGamepadString(i)));
            m_labelButtons[i].m_text.m_params.SetAlignment(SWGUIALIGN_BOTTOM | SWGUIALIGN_RIGHT);
            m_menuButtons[i].m_border.m_params.m_flags.nFillStyle = 2;
            m_menuButtons[i].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        }
    }

    // 1024x768 design space scaled to the current screen.
    float fSx = nScreenW * 0.0009765625f;
    float fSy = nScreenH / 768.0f;
    int nY87 = static_cast<int>(fSy * 87.0f);
    int nA = static_cast<int>(fSx * 500.0f);
    extentRoot.x = static_cast<int>(nA - fSx * 225.0f);
    extentRoot.y = static_cast<int>(nY87 - fSy * 2.0f);
    extentRoot.w = static_cast<int>(fSx * 600.0f);
    extentRoot.h = static_cast<int>(fSy * 597.0f);
    m_buttonBackground.CSWGuiButton::SetExtent(extentRoot);

    float fSxHalf = fSx * 0.75f;
    float fSyHalf = fSy * 0.75f;
    int nX86 = static_cast<int>(fSxHalf * 86.0f);
    int nY98 = static_cast<int>(fSyHalf * 98.0f);
    int nY40 = static_cast<int>(fSyHalf * 40.0f);
    int nX66 = static_cast<int>(fSxHalf * 66.0f);
    int nY66 = static_cast<int>(fSyHalf * 66.0f);
    int nX11 = static_cast<int>(fSxHalf * 11.0f);
    int nY20 = static_cast<int>(fSyHalf * 20.0f);
    int nX250 = static_cast<int>(fSxHalf * 250.0f);
    float fXj = fSxHalf * 15.0f + nX86;
    int nY = nY87;

    for (int i = 0; i < GPAD_ENTRY_COUNT; i++) {
        if (i == GPAD_ENTRY_PARTY) {
            for (int j = 0; j < 3; j++) {
                CSWGuiButton& rPortrait = m_portraitButtons[j];
                CSWGuiButton& rFrame = m_frameButtons[j];
                extentRoot.x = static_cast<int>(fXj * j + nA);
                extentRoot.y = nY;
                extentRoot.w = nX86;
                extentRoot.h = nY98;
                rFrame.SetExtent(extentRoot);
                extentRoot.x += nX11;
                extentRoot.y += nY20;
                extentRoot.w = nX66;
                extentRoot.h = nY66;
                rPortrait.SetExtent(extentRoot);
                rFrame.SetActive(m_nSelectedEntry == GPAD_ENTRY_PARTY && j == m_nSelectedPartySlot);
                if (bInitImages) {
                    AddControl(&rPortrait);
                    AddControl(&rFrame);
                    SendControlToBack(&rFrame);
                }
            }
        } else {
            CSWGuiButton& rEntry = m_menuButtons[i];
            extentRoot.x = nA;
            extentRoot.y = nY;
            extentRoot.w = nX86;
            extentRoot.h = nY98;
            rEntry.SetExtent(extentRoot);
            rEntry.SetActive(i == m_nSelectedEntry);
            if (bInitImages)
                AddControl(&rEntry);
        }
        extentRoot = (i == GPAD_ENTRY_PARTY) ? m_frameButtons[0].m_extent : m_menuButtons[i].m_extent;
        extentRoot.x = static_cast<int>(extentRoot.x - (fSxHalf * 7.0f + nX250));
        extentRoot.y = static_cast<int>(extentRoot.y + fSyHalf * ((3 - i) * 2.5f + 40.0f));
        extentRoot.w = nX250;
        extentRoot.h = nY40;
        m_labelButtons[i].SetExtent(extentRoot);
        nY += nY98;
    }
    SendControlToBack(&m_buttonBackground);
}
#endif

#if KOTOR1
            // party pair only while entry 1 is lit; no input-class reset and no key aliases
void CSWGuiMainInterface::CSWGamepadMenuIos::HandleInputEvent(int nType, int nVal) {
    K2_UNUSED(nVal);
    int nDir = -1;
    switch (nType) {
    case INPUTEVENT_GUI_Y:
        g_nOpenMapCountdown = 2;
        // fallthrough
    case INPUTEVENT_GUI_B:
    lCloseGamepadMenu:
        if (!m_bWasPausedByCombat)
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
    lRemoveGamepadMenu:
        g_pGuiMan->RemovePanel(this);
        break;
    case INPUTEVENT_GUI_A:
        switch (m_nSelectedEntry) {
        case GPAD_ENTRY_MAP:
            g_nOpenMapCountdown = 2;
            break;
        case GPAD_ENTRY_PARTY:
            break;
        case GPAD_ENTRY_SOLO:
            g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            g_pAppManager->m_pClientExoApp->TogglePartyFollow();
            break;
        case GPAD_ENTRY_STEALTH:
            if (!m_bWasPausedByCombat)
                g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
            if (g_pAppManager->m_pClientExoApp->GetPlayerCreature()->IsStealthCapable())
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter()
                    ->PartyMenuStealth(0, 0);
            break;
        case GPAD_ENTRY_QUICKSAVE:
            g_pAppManager->m_pClientExoApp->SendSaveGameRequest(0, CExoString("QUICKSAVE"),
                                                                CExoString("QUICKSAVE"));
            break;
        case GPAD_ENTRY_HELP: {
            static bool s_bHasTouchScreen = HasTouchScreen();
            ShowHelpInternal(!s_bHasTouchScreen);
            g_nHelpSavedPausedByCombat = m_bWasPausedByCombat;
            goto lRemoveGamepadMenu;
        }
        default:
            return;
        }
        goto lCloseGamepadMenu;
    case INPUTEVENT_GUI_DPAD_LEFT:
        if (m_nSelectedEntry == GPAD_ENTRY_PARTY && m_nSelectedPartySlot > 0) {
            --m_nSelectedPartySlot;
            g_pAppManager->m_pClientExoApp
                ->ChangeCharacter(m_nLastPartySlot == 2 ? 2 : 1, 1);
            UpdateStealth();
            LayoutGamepadMenu(false);
        }
        break;
    case INPUTEVENT_GUI_DPAD_RIGHT:
        if (m_nSelectedEntry == GPAD_ENTRY_PARTY && m_nSelectedPartySlot < m_nLastPartySlot) {
            ++m_nSelectedPartySlot;
            g_pAppManager->m_pClientExoApp->ChangeCharacter(1, 1);
            UpdateStealth();
            LayoutGamepadMenu(false);
        }
        break;
    case INPUTEVENT_GUI_DPAD_DOWN:
        nDir = 1;
        // fallthrough
    case INPUTEVENT_GUI_DPAD_UP: {
        int nPrev = m_nSelectedEntry;
        m_nSelectedEntry += nDir;
        if (m_nSelectedEntry == GPAD_ENTRY_STEALTH && !m_bStealthCapable)
            m_nSelectedEntry += nDir;
        m_nSelectedEntry = (m_nSelectedEntry + GPAD_ENTRY_COUNT) % GPAD_ENTRY_COUNT;
        if (m_nSelectedEntry != nPrev)
            LayoutGamepadMenu(false);
        break;
    }
    }
}
#else
void CSWGuiMainInterface::CSWGamepadMenuIos::HandleInputEvent(int nType, int nVal) {
    K2_UNUSED(nVal);
    int nDir = -1;
    switch (nType) {
    case INPUTEVENT_PAD_MENU:
        g_gameMenuCancelledActivateInGameGuiButton = 1;
        g_nGamepadMenuSuppressed = 2;
        // fallthrough
    case INPUTEVENT_GUI_B:
    case INPUTEVENT_GUI_Y:
    lCloseGamepadMenu:
        if (!m_bWasPausedByCombat)
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 0);
        g_pGuiMan->RemovePanel(this);
        break;
    case INPUTEVENT_GUI_A:
        switch (m_nSelectedEntry) {
        case GPAD_ENTRY_MAP:
            g_nGamepadMenuSuppressed = 2;
            break;
        case GPAD_ENTRY_PARTY:
            break;
        case GPAD_ENTRY_SOLO:
            g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            g_pAppManager->m_pClientExoApp->TogglePartyFollow();
            break;
        case GPAD_ENTRY_STEALTH: {
            CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            if (pPlayer && pPlayer->IsStealthCapable())
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter()
                    ->PartyMenuStealth(0, 0);
            break;
        }
        case GPAD_ENTRY_QUICKSAVE:
            g_pAppManager->m_pClientExoApp->SendSaveGameRequest(0, CExoString("QUICKSAVE"),
                                                                CExoString("QUICKSAVE"));
            break;
        case GPAD_ENTRY_FREE_LOOK:
            g_pAppManager->m_pClientExoApp->m_pInternal->m_pPlayerControl->ResetDriveAcceleration();
            g_bFreeLook = 1;
            break;
        case GPAD_ENTRY_SWITCH_WEAPONS: {
            CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
            if (pPlayer)
                pPlayer->ActionMenuSwitchWeapons(0, 0);
            break;
        }
        case GPAD_ENTRY_HELP:
            if (!HasTouchScreen())
                g_nHelpMode = 0;
            if (HasJoystick()) {
                g_nQueuedHelpPage = 0;
                g_bQueuedHelpGamepad = 0;
                g_nQueuedHelpDelay = 1;
            }
            g_nHelpSavedPausedByCombat = m_bWasPausedByCombat;
            break;
        default:
            return;
        }
        goto lCloseGamepadMenu;
    case INPUTEVENT_GUI_DPAD_LEFT:
    case INPUTEVENT_GUI_KEY_LEFT:
        if (m_nSelectedPartySlot > 0) {
            --m_nSelectedPartySlot;
            g_pAppManager->m_pClientExoApp
                ->ChangeCharacter(m_nLastPartySlot == 2 ? 2 : 1, 1);
            UpdateStealth();
            LayoutGamepadMenu(false);
        }
        break;
    case INPUTEVENT_GUI_DPAD_RIGHT:
    case INPUTEVENT_GUI_KEY_RIGHT:
        if (m_nSelectedPartySlot < m_nLastPartySlot) {
            ++m_nSelectedPartySlot;
            g_pAppManager->m_pClientExoApp->ChangeCharacter(1, 1);
            UpdateStealth();
            LayoutGamepadMenu(false);
        }
        break;
    case INPUTEVENT_GUI_DPAD_DOWN:
    case INPUTEVENT_GUI_KEY_DOWN:
        nDir = 1;
        // fallthrough
    case INPUTEVENT_GUI_DPAD_UP:
    case INPUTEVENT_GUI_KEY_UP: {
        int nPrev = m_nSelectedEntry;
        int nNext = nPrev + nDir;
        m_nSelectedEntry = nNext;
        if (nNext == GPAD_ENTRY_STEALTH && !m_bStealthCapable)
            m_nSelectedEntry = nNext + nDir;
        m_nSelectedEntry = (m_nSelectedEntry + GPAD_ENTRY_COUNT) % GPAD_ENTRY_COUNT;
        if (m_nSelectedEntry != nPrev)
            LayoutGamepadMenu(false);
        break;
    }
    }
}
#endif

void CSWGuiMainInterface::UpdateAndDrawFogOfWar(const CSWGuiExtent& rExtent) {
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    CSWSAreaMap* pAreaMap = pModule->m_pAreaMap;
    if (pAreaMap && pAreaMap->m_nSize) {
        unsigned int nDataSizeDwords;
        int nResX, nResY;
        unsigned int* pExploredBits = static_cast<unsigned int*>(pAreaMap->GetMapData(nDataSizeDwords, nResX, nResY));
        if (pExploredBits) {
#if KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
            float fSX = 440.0f * K1MiniMapZoom() / static_cast<float>(nResX);
            float fSY = 256.0f * K1MiniMapZoom() / static_cast<float>(nResY);
#elif KOTOR1
            float fSX = 440.0f / static_cast<float>(nResX);
            float fSY = 256.0f / static_cast<float>(nResY);
#else
            float fSX = static_cast<float>(m_nMapWidth) / static_cast<float>(nResX);
            float fSY = static_cast<float>(m_nMapHeight) / static_cast<float>(nResY);
#endif
            float fCellW = fSX * 4.0f;
            float fCellH = fSY * 4.0f;
            float fHalfCellW = fCellW * 0.5f;
            float fHalfCellH = fCellH * 0.5f;
            int nHalfCellCols = static_cast<int>(fHalfCellW / fSX);
            int nHalfCellRows = static_cast<int>(fHalfCellH / fSY);
            int nCellY = static_cast<int>(static_cast<float>(rExtent.y) / fSY) + nHalfCellRows;
            int nCellX = static_cast<int>(static_cast<float>(rExtent.x) / fSX) + nHalfCellCols;
            int nXEnd = (static_cast<int>(static_cast<float>(m_mapViewExtent.w) / fSX) - nCellX) + nHalfCellCols * 2;
            int nYEnd = (static_cast<int>(static_cast<float>(m_mapViewExtent.h) / fSY) - nCellY) + nHalfCellRows * 2;
            int nRow = -nCellY;
            float fY = fSY * nRow + static_cast<float>(rExtent.y);
            if (nRow < 0) {
                nRow = 0;
                fY = fSY * nCellY + fY;
            }
            int nRowEnd = nYEnd < nResY ? nYEnd : nResY;
            int nColEnd = nXEnd < nResX ? nXEnd : nResX;
            if (nRow <= nRowEnd) {
                int nColStart = -nCellX;
                nColStart = nColStart < 0 ? 0 : nColStart;
                int nRowBase = nRow * (nResX + 1);
                float fX = fSX * nColStart;
                do {
                    if (nColStart <= nColEnd) {
                        float fXCell = fSX * nColStart;
                        fXCell = fX + static_cast<float>(rExtent.x);
                        int nCol = nColStart;
                        do {
                            int nBit = nRowBase + nCol;
                            int nWord = nBit / 32;
                            if (!(pExploredBits[nWord] & (1 << nBit))) {
                                Vector vColor(-1.0f, -1.0f, -1.0f);
                                static_cast<CAurGUIImageInternal*>(m_pFogOfWarImage)->DrawBuffered(
                                    (fXCell - fHalfCellW) / static_cast<float>(m_mapViewExtent.w),
                                    (fY - fHalfCellH) / static_cast<float>(m_mapViewExtent.h),
                                    fCellW / static_cast<float>(m_mapViewExtent.w),
                                    fCellH / static_cast<float>(m_mapViewExtent.h),
                                    0, 0.0f, vColor, 1.0f);
                            }
                            fXCell = fSX + fXCell;
                        } while (nCol++ < nColEnd);
                    }
                    fY = fSY + fY;
                    nRowBase = nRowBase + (nResX + 1);
                    nRow++;
                } while (nRow <= nRowEnd);
            }
            CAurGUIImage::FlushBuffer(CAurGUIImage::cm_fAlpha);
        }
    }
}

void CSWGuiMainInterface::DrawMap(float fDelta) {
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (!pOptions || pOptions->m_bFeedback3) {
#if !KOTOR1
        CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
        if (pPartyTable->m_disableMap) {
            m_altMenuButton.m_tail.bVisible = 0;
            m_miniMapImage.m_tail.bVisible = 0;
            m_miniMap.m_tail.bVisible = 0;
        } else {
#endif
            CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
            CSWSAreaMap* pArea = pModule->m_pAreaMap;
            if (pArea && pArea->m_nSize) {
                if (m_miniMapImage.m_border.m_params.m_fillImage == const_cast<char*>(""))
                    InitializeMap();
                CSWGuiExtent extent = m_miniMapImage.m_extent;
                float fScale = pArea->m_fMapScale;
                int nMapX, nMapY;
                pArea->GetPartyMemberMapLocation(0, nMapX, nMapY);
#if !KOTOR1
                nMapX = static_cast<int>(static_cast<float>(nMapX) * static_cast<float>(m_nMapWidth) / 708.0f);
                nMapY = static_cast<int>(static_cast<float>(nMapY) * static_cast<float>(m_nMapHeight) / 354.0f);
#elif defined(K2_FIX_ENGINE_BUGS)
                nMapX = static_cast<int>(static_cast<float>(nMapX) * K1MiniMapZoom());
                nMapY = static_cast<int>(static_cast<float>(nMapY) * K1MiniMapZoom());
#endif
                extent.x = static_cast<int>(static_cast<float>(m_mapViewExtent.w / 2) - nMapX * fScale);
                extent.y = static_cast<int>(static_cast<float>(m_mapViewExtent.h / 2) - nMapY * fScale);
                m_miniMapImage.SetExtent(extent);
#if defined(K2_UI_XBOX) || defined(K2_FIX_ENGINE_BUGS)
                CSWCCreature* pPlayer =
                    g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
                if (pPlayer != 0) {
                    m_lblArrow.m_border.m_params.m_fRotate =
                        pArea->GetMapRotateCCWFromWorldOrientation(pPlayer->m_orientation);
                } else {
#endif
                CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
                Quaternion q = pCamera->GetOrientation();
                m_lblArrow.m_border.m_params.m_fRotate =
                    pArea->GetMapRotateCCWFromWorldOrientation(Vector(
#if KOTOR1
                        (q.x * q.y - q.w * q.z) * 2.0f,
                        1.0f - (q.x * q.x + q.z * q.z) * 2.0f,
                        (q.x * q.w + q.y * q.z) * 2.0f));
#else
                        (q.x * q.z + q.w * q.y) * -2.0f,
                        (q.z * q.y - q.x * q.w) * -2.0f,
                        -(1.0f - (q.x * q.x + q.y * q.y) * 2.0f)));
#endif
#if defined(K2_UI_XBOX) || defined(K2_FIX_ENGINE_BUGS)
                }
#endif
                AurGUIStartLayer();
#if KOTOR1
                if (AurGUISetupViewport(m_mapViewExtent.x, m_mapViewExtent.y, m_mapViewExtent.w, m_mapViewExtent.w,
                                        CGuiInGame::COLOR_BLACK, false,
                                        m_fAlpha)) {
#else
                if (AurGUISetupViewport(m_mapViewExtent.x, m_mapViewExtent.y, m_mapViewExtent.w, m_mapViewExtent.h,
                                        CGuiInGame::COLOR_BLACK, false,
                                        m_fAlpha)) {
#endif
                    m_miniMapImage.Draw(fDelta);
                    UpdateAndDrawFogOfWar(extent);
                    m_lblArrow.Draw(fDelta);
                    AurGUICloseViewport();
                }
                AurGUIStopLayer();
                m_altMenuButton.m_tail.bVisible = 1;
#if !KOTOR1
                m_miniMap.m_tail.bVisible = 1;
#endif
            } else {
                m_altMenuButton.m_tail.bVisible = 0;
            }
#if !KOTOR1
        }
#endif
    } else {
        m_altMenuButton.m_tail.bVisible = 0;
    }
}


void CSWGuiMainInterface::SelectNextPersonalAction(int nIndex) {
#if KOTOR1 && !defined(K1_UI_XBOX)  // K1: four personal boxes (the console bar has six)
    if (static_cast<unsigned int>(nIndex) > 3)
        return;
#else
    if (static_cast<unsigned int>(nIndex) > 5)
        return;
#endif
    if (m_personalActions[nIndex].m_count == 0)
        return;
    g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    CSWGuiInterfaceAction* pNext = 0;
    int nActionId = m_personalActionIds[nIndex];
    int nCount = m_personalActions[nIndex].m_count;
    if (nActionId != -1) {
        for (int i = 0; i < nCount; i++) {
            if (m_personalActions[nIndex].m_pArray[i].m_nActionId == static_cast<unsigned int>(nActionId)) {
                if (i + 1 < nCount)
                    pNext = &m_personalActions[nIndex].m_pArray[i + 1];
                else
                    pNext = &m_personalActions[nIndex].m_pArray[0];
                break;
            }
        }
    }
    if (nCount >= 2) {
        if (!pNext)
            pNext = &m_personalActions[nIndex].m_pArray[1];
        m_personalActionIds[nIndex] = pNext->m_nActionId;
        m_pGuiManager->PlayGuiSound(GUISOUND_ACTION_MENU_SCROLL);
    }
}

void CSWGuiMainInterface::SelectPrevPersonalAction(int nIndex) {
#if KOTOR1 && !defined(K1_UI_XBOX)  // K1: four personal boxes (the console bar has six)
    if (static_cast<unsigned int>(nIndex) > 3)
        return;
#else
    if (static_cast<unsigned int>(nIndex) > 5)
        return;
#endif
    if (m_personalActions[nIndex].m_count == 0)
        return;
    g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    CSWGuiInterfaceAction* pPrev = 0;
    int nActionId = m_personalActionIds[nIndex];
    int64_t nCount = m_personalActions[nIndex].m_count;
    if (nActionId != -1) {
        CSWGuiInterfaceAction* pActions = m_personalActions[nIndex].m_pArray;
        for (int i = 0; i < nCount; i++) {
            if (pActions[i].m_nActionId == static_cast<unsigned int>(nActionId)) {
                if (i == 0)
                    pPrev = &pActions[nCount - 1];
                else
                    pPrev = &pActions[i - 1];
                break;
            }
        }
    }
    if (nCount >= 2) {
        if (!pPrev)
            pPrev = &m_personalActions[nIndex].m_pArray[nCount - 1];
        m_personalActionIds[nIndex] = pPrev->m_nActionId;
        m_pGuiManager->PlayGuiSound(GUISOUND_ACTION_MENU_SCROLL);
    }
}

// Pulse the two "you have something new" menu buttons. Each arm re-reads its own params flag word
// so an already-pulsing button is left alone; the else arms clear the pulse unconditionally.
void CSWGuiMainInterface::UpdateGameMenu() {
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->m_nNewItemCount) {
        if (!m_btnInventory.m_border.m_params.m_flags.bPulseA)
            m_btnInventory.m_border.m_params.SetPulsingAlpha(1, 0, 1);
    } else {
        m_btnInventory.m_border.m_params.SetPulsingAlpha(0, 0, 0);
    }
    if (g_pAppManager->m_pClientExoApp->GetQuestJournal()->HasNewEntries()) {
        if (!m_btnJournal.m_border.m_params.m_flags.bPulseA)
            m_btnJournal.m_border.m_params.SetPulsingAlpha(1, 0, 1);
    } else {
        m_btnJournal.m_border.m_params.SetPulsingAlpha(0, 0, 0);
    }
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::OnPersonalActionExit(CSWGuiControl* pControl) {
    if (!pControl->m_tail.nCurrentEventArg)
        return;
    if (!m_bActionMenuLatch)
        return;
    int nAction = static_cast<int>(pControl->m_tail.m_nUserData);
    if (g_pGuiMan->PanelExists(&m_actionMenuIos)) {
        if (m_nFocusedAction == nAction) {
            m_bClearSelectionOnClose = 1;
            HandleButtonActionX(pControl);
        } else {
            ResetPartyMemberItemUse();
            if (nAction >= 0)
                PopupActionMenu(nAction);
        }
        m_nFocusedAction = nAction;
    }
#ifdef K2_FIX_ENGINE_BUGS
    // The target-action buttons register this handler too, with user data 6..8, and only a personal
    // slot has an action list to look in.
    if (nAction >= 0 && nAction < static_cast<int>(sizeof(m_personalActionIds) / sizeof(m_personalActionIds[0]))) {
#endif
    int nActionId = m_personalActionIds[nAction];
    if (nActionId != -1) {
        int64_t nCount = m_personalActions[nAction].m_count;
        if (nCount >= 1) {
            CSWGuiInterfaceAction* pActions = m_personalActions[nAction].m_pArray;
            CSWGuiInterfaceAction* pFound = 0;
            for (int64_t i = 0; i < nCount; i++) {
                if (pActions[i].m_nActionId == static_cast<unsigned int>(nActionId)) {
                    pFound = &pActions[i];
                    break;
                }
            }
            if (pFound && pFound->m_bChangesState)
                RevertActionBarStates();
        }
    }
#ifdef K2_FIX_ENGINE_BUGS
    }
#endif
    m_bActionMenuLatch = 0;
}
#endif // !KOTOR1

void CSWGuiMainInterfaceAction::SetSelected(bool bSelected) {
    m_bSelected = bSelected;
#if KOTOR1
#ifndef K2_UI_TOUCH
    // pc / xbox: the boxes keep the layout's own art (the PC engine has no selected skin).
    if (false)
#endif
    {
    if (bSelected)
        m_button.m_border.m_params.SetFillImage(CResRef("ios_self_box2"), 0);
    else
        m_button.m_border.m_params.SetFillImage(CResRef("ios_self_box"), 0);
    m_button.m_border.m_params.m_flags.nFillStyle = 2;
    }
#else
#ifdef K2_UI_XBOX
    if (m_btnUp.m_extent.w > 0) {
#ifdef K2_HOST_BUILD
        K2ConsoleSizeBox(*this, bSelected);
#endif
        m_button.m_border.m_params.m_color = bSelected
            ? m_button.m_borderHilight.m_params.m_color
            : m_btnUp.m_border.m_params.m_color;
        return;
    }
#endif
    if (bSelected) {
        m_button.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_WHITE);
    } else {
        static CSWGuiColor s_idleTint(0.4f, 0.4f, 0.4f);
        m_button.m_border.m_params.m_color = s_idleTint;
    }
#endif
}

void CSWGuiMainInterfaceAction::Pulse(int bPulse) {
    if (bPulse) {
        m_nPulseSteps = 2;
        m_fPulseTimer = 0.1f;
    } else {
        m_nPulseSteps = 0;
        m_icon.m_border.m_params.m_fAlpha = 1.0f;
        m_icon.m_borderHilight.m_params.m_fAlpha = 1.0f;
    }
}

void CSWGuiMainInterfaceAction::Update(float fDelta) {
    if (!m_nPulseSteps)
        return;
    m_fPulseTimer -= fDelta;
    if (m_fPulseTimer <= 0.0f) {
        --m_nPulseSteps;
        m_fPulseTimer = 0.1f;
    }
    float fAlpha;
    if (m_nPulseSteps == 0) {
        fAlpha = 1.0f;
    } else {
        float fT = (0.1f - m_fPulseTimer) / 0.1f;
        fAlpha = m_nPulseSteps == 2 ? 1.0f - fT : fT;
    }
    m_icon.m_border.m_params.m_fAlpha = fAlpha;
    m_icon.m_borderHilight.m_params.m_fAlpha = fAlpha;
}

// Installs the slot's icon, dimming the borders to 0.25 when the entry is greyed.
void CSWGuiMainInterfaceAction::SetIcon(const CResRef& resRef, const CExoString& str, int bEnabled,
                                        unsigned char bDimmed) {
    K2_UNUSED(str); K2_UNUSED(bDimmed);
    if (resRef != m_icon.m_border.m_params.m_fillImage) {
#if KOTOR1
        m_icon.m_border.m_params.SetFillImage(resRef, 0);
        m_icon.m_border.m_params.m_flags.nFillStyle = 2;
        m_icon.m_borderHilight.m_params.SetFillImage(resRef, 0);
        m_icon.m_borderHilight.m_params.m_flags.nFillStyle = 2;
#else
        m_icon.m_border.m_params.SetFillImage(resRef, 0);
        m_icon.m_borderHilight.m_params.SetFillImage(resRef, 0);
#endif
        m_nPulseSteps = 0;
    }
    float fAlpha = bEnabled == 0 ? 0.25f : 1.0f;
    m_icon.m_border.m_params.m_fAlpha = fAlpha;
    m_icon.m_borderHilight.m_params.m_fAlpha = fAlpha;
}

void CSWGuiMainInterfaceAction::Draw(float fDelta) {
    if (m_button.m_tail.bVisible)
        m_button.Draw(fDelta);
    if (m_btnUp.m_tail.bVisible)
        m_btnUp.Draw(fDelta);
    if (m_btnDown.m_tail.bVisible)
        m_btnDown.Draw(fDelta);
    if (m_icon.m_tail.bVisible)
        m_icon.Draw(fDelta);
}

CSWGuiControl* CSWGuiMainInterfaceAction::HitCheckMouse(int nX, int nY) {
    if (m_btnUp.HitCheckMouse(nX, nY))
        return &m_btnUp;
    if (m_btnDown.HitCheckMouse(nX, nY))
        return &m_btnDown;
    if (m_icon.HitCheckMouse(nX, nY))
        return &m_icon;
    if (m_button.HitCheckMouse(nX, nY))
        return &m_button;
    return 0;
}

#if KOTOR1
void CSWGuiMainInterfaceAction::Show(int bEmpty, int bShowArrows) {
#ifndef K2_UI_PC  // pc: only the mouse leaving the box deactivates it (as K2's macOS Show)
    m_button.SetActive(0);
#endif
    if (bEmpty) {
        CResRef resClear("");
        CExoString strClear("");
        if (resClear != m_icon.m_border.m_params.m_fillImage) {
            m_icon.m_border.m_params.SetFillImage(resClear, 0);
            m_icon.m_border.m_params.m_flags.nFillStyle = 2;
            m_icon.m_borderHilight.m_params.SetFillImage(resClear, 0);
            m_icon.m_borderHilight.m_params.m_flags.nFillStyle = 2;
            m_nPulseSteps = 0;
        }
        m_icon.m_border.m_params.m_fAlpha = 1.0f;
        m_icon.m_borderHilight.m_params.m_fAlpha = 1.0f;
    }
    float fAlpha = bEmpty ? 0.5f : 1.0f;
    m_button.m_border.m_params.m_fAlpha = fAlpha;
    m_button.m_borderHilight.m_params.m_fAlpha = fAlpha;
    m_bEmpty = bEmpty != 0;
    m_btnUp.m_tail.bVisible = bShowArrows;
    m_btnDown.m_tail.bVisible = bShowArrows;
}
#else
void CSWGuiMainInterfaceAction::Show(int bEmpty, int bShowArrows) {
#ifndef K2_UI_PC  // pc: only the mouse leaving the box deactivates it
    if (!g_bLeftMouseDown)
        m_button.SetActive(0);
#endif
    if (bEmpty) {
        CResRef resClear("");
        CExoString strClear("");
        if (resClear != m_icon.m_border.m_params.m_fillImage) {
            m_icon.m_border.m_params.SetFillImage(resClear, 0);
            m_icon.m_borderHilight.m_params.SetFillImage(resClear, 0);
            m_nPulseSteps = 0;
        }
        m_icon.m_border.m_params.m_fAlpha = 1.0f;
        m_icon.m_borderHilight.m_params.m_fAlpha = 1.0f;
    }
    m_button.m_border.m_params.m_fAlpha = 1.0f;
    m_button.m_borderHilight.m_params.m_fAlpha = 1.0f;
    m_bEmpty = bEmpty != 0;
    m_btnUp.m_tail.bVisible = bShowArrows;
    m_btnDown.m_tail.bVisible = bShowArrows;
}
#endif

#ifdef K2_UI_XBOX

static int ConsoleBoxPitch(CSWGuiMainInterface* pInterface, CSWGuiLabel& scratch,
                           const char* pPrefix) {
    CExoString sName;
    sName.Format("%s%d", pPrefix, 5);
    pInterface->InitControl(scratch, sName, 0, 1, 1, 0, 0);   // 0 = keep it out of the table
    int nLeft5 = scratch.m_extent.x;
    sName.Format("%s%d", pPrefix, 6);
    pInterface->InitControl(scratch, sName, 0, 1, 1, 0, 0);
    return scratch.m_extent.x - nLeft5;
}

static void ConsoleBind(CSWGuiMainInterface* pInterface, CSWGuiControl& control,
                        CSWGuiLabel& scratch, const char* pPrefix, int nBox) {
    CExoString sName;
    sName.Format("%s%dB", pPrefix, nBox);  // idiom-exempt: char* literal, Format's parameter type
    pInterface->InitControl(control, sName, 1, 1, 1, 0, 0);
    if (control.m_extent.w > 0)
        return;
    sName.Format("%s%d", pPrefix, nBox);
    pInterface->InitControl(control, sName, 1, 1, 1, 0, 0);
    if (control.m_extent.w <= 0 && nBox > 6) {
        int nPitch = ConsoleBoxPitch(pInterface, scratch, pPrefix);
        sName.Format("%s%d", pPrefix, 6);
        pInterface->InitControl(control, sName, 1, 1, 1, 0, 0);
        CSWGuiExtent extent = control.m_extent;
        extent.x += nPitch;
        control.SetExtent(extent);
        return;
    }
}

static void ConsoleRebind(CSWGuiMainInterface* pInterface, CSWGuiControl& control,
                          const char* pMobileTag, const char* pConsoleTag, int bOffsetToEdge) {
    pInterface->InitControl(control, CExoString(pMobileTag), 1, 1, bOffsetToEdge, 0, 0);
    if (control.m_extent.w > 0 && control.m_extent.h > 0)
        return;
    pInterface->InitControl(control, CExoString(pConsoleTag), 1, 1, bOffsetToEdge, 0, 0);
}

#ifdef K2_HOST_BUILD
// Returns the large twin of a bar control ConsoleBind bound small: the plain name, or for a seventh
// box the layout lacks, box 6's rect one pitch on.
static CSWGuiExtent ConsoleLargeExtent(CSWGuiMainInterface* pInterface, const char* pPrefix,
                                       int nBox) {
    CSWGuiLabel scratch;
    CExoString sName;
    sName.Format("%s%d", pPrefix, nBox);
    pInterface->InitControl(scratch, sName, 0, 1, 1, 0, 0);
    CSWGuiExtent extent = scratch.m_extent;
    if (extent.w <= 0 && nBox > 6) {
        CSWGuiLabel sixth;
        int nPitch = ConsoleBoxPitch(pInterface, sixth, pPrefix);   // leaves box 6's rect bound
        extent = sixth.m_extent;
        extent.x += nPitch;
    }
    return extent;
}

static void ConsoleRecordSizes(CSWGuiMainInterface* pInterface, CSWGuiMainInterfaceAction& widget,
                               int nIndex, int nBox) {
    if (static_cast<unsigned int>(nIndex) >= 9)
        return;
    K2ConsoleBoxSizes& s = s_k2Sizes[nIndex];
    s.aSmall[0] = widget.m_button.m_extent;
    s.aSmall[1] = widget.m_icon.m_extent;
    s.aSmall[2] = widget.m_btnUp.m_extent;
    s.aBig[0] = ConsoleLargeExtent(pInterface, "LBH_BORDER", nBox);
    s.aBig[1] = ConsoleLargeExtent(pInterface, "LBL_ICON", nBox);
    s.aBig[2] = ConsoleLargeExtent(pInterface, "LBH_ARROW", nBox);
    s.aBig[1].x = s.aBig[0].x + (s.aBig[0].w - s.aBig[1].w) / 2;   // centred, as the small icon
    s.aBig[1].y = s.aBig[0].y + (s.aBig[0].h - s.aBig[1].h) / 2;
    // The large box and its icon at 90%, about the box's centre, so the selected box clears its
    // neighbours; the chevron keeps its size and follows the box's edge.
    const CSWGuiExtent box = s.aBig[0];
    for (int k = 0; k < 2; k++) {
        CSWGuiExtent& e = s.aBig[k];
        const int nW = e.w * 9 / 10;
        const int nH = e.h * 9 / 10;
        e.x = box.x + box.w / 2 - (box.x + box.w / 2 - e.x) * 9 / 10;
        e.y = box.y + box.h / 2 - (box.y + box.h / 2 - e.y) * 9 / 10;
        e.w = nW;
        e.h = nH;
    }
    const int nInset = (box.h - s.aBig[0].h) / 2;
    if (s.aBig[2].y + s.aBig[2].h / 2 < box.y + box.h / 2)
        s.aBig[2].y += nInset;
    else
        s.aBig[2].y -= nInset;
    s.bValid = true;
    for (int k = 0; k < 3; k++) {
        if (s.aBig[k].w <= 0 || s.aBig[k].h <= 0 || s.aSmall[k].w <= 0)
            s.bValid = false;
    }
}
#endif  // K2_HOST_BUILD
#endif

#if KOTOR1
// K1: an action box is re-anchored with its slot -- in place for the target menu, to the
// bottom-right HUD corner for the personal bar.
static inline void ShiftActionControl(CSWGuiControl& control, int bTarget) {
    CSWGuiExtent extent = control.m_extent;
    if (bTarget)
        AndroidExtentShift(extent, 0, 0);
    else
        AndroidExtentShift(extent, -1, -1);
    control.SetExtent(extent);
}

void CSWGuiMainInterfaceAction::Initialize(CSWGuiMainInterface* pInterface, int nSlot, int nMode) {
    CExoString sName;
    CSWGuiLabel labelTmp;
    m_nMode = nMode;
#ifdef K1_UI_XBOX
    if (nMode == 0 && static_cast<unsigned int>(nSlot) < 6) {
        static const char* const s_apTag[3] = { "LBH_BORDER", "LBL_ICON", "LBH_ARROW" };
        CSWGuiControl* apControl[3] = { &m_button, &m_icon, &m_btnUp };
        for (int k = 0; k < 3; k++) {
            sName.Format("%s%dB", s_apTag[k], nSlot + 1);
            pInterface->InitControl(labelTmp, sName, 0);
            ShiftActionControl(labelTmp, 0);
            s_k1Boxes[nSlot].aSmall[k] = labelTmp.m_extent;
            sName.Format("%s%d", s_apTag[k], nSlot + 1);
            pInterface->InitControl(*apControl[k], sName, 1);
            ShiftActionControl(*apControl[k], 0);
            s_k1Boxes[nSlot].aBig[k] = apControl[k]->m_extent;
            apControl[k]->m_tail.bFocusOnHover = 0;
            apControl[k]->m_tail.m_nUserData = nSlot;
        }
        m_btnUp.m_tail.bVisible = 0;
        K1ConsoleScroll& s = s_k1Scroll[nSlot];
        if (!s.pOld)
            s.pOld = new CSWGuiLabel();
        sName.Format("LBL_ICON%d", nSlot + 1);
        pInterface->InitControl(*s.pOld, sName, 0);
        s.fTime = 0.0f;
        // The mouse: the box and its icon select and run on a click; the arrow strip's halves
        // (CSWGuiMainInterface::HitCheckMouse: m_btnUp above the icon, m_btnDown below it) step
        // the entries on a click; the wheel steps them anywhere on the box.
        typedef void (CSWGuiObject::*Handler)(CSWGuiControl*);
        const Handler pfnClick = static_cast<Handler>(&CSWGuiMainInterface::OnK1ConsoleBoxClicked);
        const Handler pfnUp = static_cast<Handler>(&CSWGuiMainInterface::OnK1ConsoleBoxWheelUp);
        const Handler pfnDown = static_cast<Handler>(&CSWGuiMainInterface::OnK1ConsoleBoxWheelDown);
        CSWGuiControl* apMouse[4] = { &m_button, &m_icon, &m_btnUp, &m_btnDown };
        const Handler apClick[4] = { pfnClick, pfnClick, pfnUp, pfnDown };
        for (int k = 0; k < 4; k++) {
            apMouse[k]->m_tail.bFocusOnHover = 0;
            apMouse[k]->m_tail.m_nUserData = nSlot;
            apMouse[k]->AddEvent(INPUTEVENT_GUI_A, pInterface, apClick[k]);
            apMouse[k]->AddEvent(SWGUIEVENT_WHEEL_UP, pInterface, pfnUp);
            apMouse[k]->AddEvent(SWGUIEVENT_WHEEL_DOWN, pInterface, pfnDown);
        }
        return;
    }
#endif
    CExoString sMode(nMode == 0 ? "ACTION" : "TARGET");
    sName.Format(const_cast<char*>("BTN_%s%d"), sMode.CStr(), nSlot);
    pInterface->InitControl(m_button, sName, nMode == 0);
    sName.Format(const_cast<char*>("LBL_%s%d"), sMode.CStr(), nSlot);
    pInterface->InitControl(m_icon, sName, nMode == 0);
    sName.Format(const_cast<char*>("BTN_%sUP%d"), sMode.CStr(), nSlot);
    pInterface->InitControl(m_btnUp, sName, nMode == 0);
    sName.Format(const_cast<char*>("BTN_%sDOWN%d"), sMode.CStr(), nSlot);
    pInterface->InitControl(m_btnDown, sName, nMode == 0);
    if (m_nMode != 0) {
        m_button.AddChildControl(&m_icon);
        m_icon.m_pParent = &m_button;
        m_button.AddChildControl(&m_btnUp);
        m_btnUp.m_pParent = &m_button;
        m_button.AddChildControl(&m_btnDown);
        m_btnDown.m_pParent = &m_button;
        nSlot += 4;
    }
    void (CSWGuiObject::*pfnUp)(CSWGuiControl*) = nMode != 0 ? static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnTargetUpArrowPressed)
        : static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnActionUpArrowPressed);
    void (CSWGuiObject::*pfnDown)(CSWGuiControl*) = nMode != 0 ? static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnTargetDownArrowPressed)
        : static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnActionDownArrowPressed);
    void (CSWGuiObject::*pfnFocus)(CSWGuiControl*) = nMode != 0 ? static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnTargetActionFocus)
        : static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnDefaultActionFocus);
    m_icon.m_tail.bFocusOnHover = 0;
    m_icon.AddEvent(INPUTEVENT_GUI_A, pInterface, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnDefaultActionLeft));
    m_icon.AddEvent(SWGUIEVENT_FOCUS, pInterface, pfnFocus);
    m_icon.AddEvent(SWGUIEVENT_WHEEL_UP, pInterface, pfnUp);
    m_icon.AddEvent(SWGUIEVENT_WHEEL_DOWN, pInterface, pfnDown);
    m_icon.m_tail.m_nUserData = nSlot;
    m_button.m_tail.bFocusOnHover = 0;
    m_button.AddEvent(SWGUIEVENT_FOCUS, pInterface, pfnFocus);
    m_button.AddEvent(SWGUIEVENT_WHEEL_UP, pInterface, pfnUp);
    m_button.AddEvent(SWGUIEVENT_WHEEL_DOWN, pInterface, pfnDown);
    m_button.m_tail.m_nUserData = nSlot;
    m_btnUp.m_tail.bFocusOnHover = 0;
    m_btnUp.AddEvent(INPUTEVENT_GUI_A, pInterface, pfnUp);
    m_btnUp.AddEvent(SWGUIEVENT_FOCUS, pInterface, pfnFocus);
    m_btnUp.AddEvent(SWGUIEVENT_WHEEL_UP, pInterface, pfnUp);
    m_btnUp.AddEvent(SWGUIEVENT_WHEEL_DOWN, pInterface, pfnDown);
    m_btnUp.m_tail.m_nUserData = nSlot;
    m_btnDown.m_tail.bFocusOnHover = 0;
    m_btnDown.AddEvent(INPUTEVENT_GUI_A, pInterface, pfnDown);
    m_btnDown.AddEvent(SWGUIEVENT_FOCUS, pInterface, pfnFocus);
    m_btnDown.AddEvent(SWGUIEVENT_WHEEL_UP, pInterface, pfnUp);
    m_btnDown.AddEvent(SWGUIEVENT_WHEEL_DOWN, pInterface, pfnDown);
    m_btnDown.m_tail.m_nUserData = nSlot;
    m_btnDown.m_border.m_params.m_fRotate = 180.0f;
    m_btnDown.m_borderHilight.m_params.m_fRotate = 180.0f;
    CSWGuiExtent sUp(0, 0, 0, 0);
    CSWGuiExtent sDown(0, 0, 0, 0);
    ShiftActionControl(m_button, m_nMode);
    ShiftActionControl(m_icon, m_nMode);
#ifndef K2_UI_TOUCH
    // pc / xbox: the layout's own arrows (both, on every box), moved with their box, the down one
    // turned over; no touch-sized expand arrow and no "ios_self" skins.
    ShiftActionControl(m_btnUp, m_nMode);
    ShiftActionControl(m_btnDown, m_nMode);
    (void)sUp;
    (void)sDown;
    if (false)
#endif
    {
    sDown = m_btnDown.m_extent;
    if (m_nMode != 0) {
        sDown = m_button.m_extent;
        m_btnDown.m_border.m_params.m_fRotate = 0.0f;
        m_btnDown.m_borderHilight.m_params.m_fRotate = 0.0f;
        sDown.y = static_cast<int>(static_cast<float>(sDown.y + sDown.h) + AndroidHudScaleFactor() * -16.0f);
        sDown.h = static_cast<int>(AndroidHudScaleFactor() * 64.0f);
        sUp.w = 0;
        sUp.h = 0;
    } else {
        sUp = m_button.m_extent;
        sUp.y = static_cast<int>(static_cast<float>(sUp.y) + AndroidHudScaleFactor() * -48.0f);
        sUp.h = static_cast<int>(AndroidHudScaleFactor() * 64.0f);
        sDown.w = 0;
        sDown.h = 0;
    }
    m_btnDown.SetExtent(sDown);
    m_btnUp.SetExtent(sUp);
    if (nMode == 0) {
        m_button.m_border.m_params.SetFillImage(CResRef("ios_self_box"), 0);
        m_button.m_border.m_params.m_flags.nFillStyle = 2;
        m_button.m_border.m_params.m_fAlpha = 1.0f;
        m_button.m_border.m_params.m_color.r = CGuiInGame::COLOR_WHITE.x;
        m_button.m_border.m_params.m_color.g = CGuiInGame::COLOR_WHITE.y;
        m_button.m_border.m_params.m_color.b = CGuiInGame::COLOR_WHITE.z;
        m_btnUp.m_border.m_params.SetFillImage(CResRef("ios_self_expand"), 0);
        m_btnUp.m_border.m_params.m_flags.nFillStyle = 2;
        m_btnUp.m_borderHilight.m_params.SetFillImage(CResRef("ios_self_expand"), 0);
        m_btnUp.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    }
    }
    m_btnUp.m_nToolTipStrRef = STRREF_K1_TIP_ACTION_MENU_UP;
    m_btnUp.m_nBoundInputEventId = INPUTEVENT_ACTION_MENU_UP;
    m_btnDown.m_nToolTipStrRef = STRREF_K1_TIP_ACTION_MENU_DOWN;
    m_btnDown.m_nBoundInputEventId = INPUTEVENT_ACTION_MENU_DOWN;
    static const unsigned long long s_aIconToolTip[7] = {
        STRREF_K1_TIP_FRIENDLY_POWER, STRREF_K1_TIP_MEDICAL_ITEM, STRREF_K1_TIP_NONMEDICAL_ITEM, STRREF_K1_TIP_MINE, STRREF_K1_TIP_LEFT_ACTION, STRREF_K1_TIP_MIDDLE_ACTION, STRREF_K1_TIP_RIGHT_ACTION };
    static const int s_aIconInputEvent[7] = { INPUTEVENT_PERSONAL_POWER_ACTION, INPUTEVENT_PERSONAL_MEDICAL_ACTION, INPUTEVENT_PERSONAL_OTHER_ACTION, INPUTEVENT_PERSONAL_MINES_ACTION, INPUTEVENT_TARGET_LEFT_ACTION, INPUTEVENT_TARGET_MIDDLE_ACTION, INPUTEVENT_TARGET_RIGHT_ACTION };
    if (static_cast<unsigned int>(nSlot) < 7) {
        m_icon.m_nToolTipStrRef = s_aIconToolTip[nSlot];
        m_icon.m_nBoundInputEventId = s_aIconInputEvent[nSlot];
    }
}
#endif

void CSWGuiMainInterfaceAction::Initialize(CSWGuiMainInterface* pInterface, int nSlot, int nMode, int bOffsetToEdge) {
    CExoString sName;
    CSWGuiLabel labelTmp;
    m_nMode = nMode;
    CExoString sMode(nMode == 0 ? "ACTION" : "TARGET");
#ifdef K2_UI_XBOX
    const int nConsoleBox = nMode == 0 ? nSlot + 2 : (nSlot == 0 ? 1 : 0);
    const bool bConsoleBar = (nConsoleBox != 0);
    if (bConsoleBar) {
        ConsoleBind(pInterface, m_button, labelTmp, "LBH_BORDER", nConsoleBox);
        ConsoleBind(pInterface, m_icon, labelTmp, "LBL_ICON", nConsoleBox);
#ifdef K2_HOST_BUILD
        // The selected box draws its icon at the large size from the highlight border, whose
        // layout fill tiles: stretch both, as the K1 bar does.
        m_icon.m_border.m_params.m_flags.nFillStyle = 2;
        m_icon.m_borderHilight.m_params.m_flags.nFillStyle = 2;
        // The scroll's outgoing icon uses the same layout row, not in the panel's table.
        K2ConsoleScroll& s = s_k2Scroll[nMode == 0 ? nSlot : ACTION_SLOT_TARGET_LEFT + nSlot];
        if (!s.pOld)
            s.pOld = new CSWGuiLabel();
        sName.Format("LBL_ICON%dB", nConsoleBox);
        pInterface->InitControl(*s.pOld, sName, 0, 1, 1, 0, 0);
        s.pOld->m_border.m_params.m_flags.nFillStyle = 2;
        s.fTime = 0.0f;
#endif
    } else {
#endif
    sName.Format(const_cast<char*>("BTN_%s%d"), sMode.CStr(), nSlot);
    pInterface->InitControl(m_button, sName, nMode == 0, 1, bOffsetToEdge, 0, 0);
    sName.Format(const_cast<char*>("LBL_%s%d"), sMode.CStr(), nSlot);
    pInterface->InitControl(m_icon, sName, nMode == 0, 1, bOffsetToEdge, 0, 0);
#ifdef K2_UI_XBOX
    }
#endif
    CSWGuiExtent extent = m_icon.m_extent;
    extent.x = m_button.m_extent.x + (m_button.m_extent.w - extent.w) / 2;
    extent.y = m_button.m_extent.y + (m_button.m_extent.h - extent.h) / 2;
    m_icon.SetExtent(extent);
#ifdef K2_UI_XBOX
    if (bConsoleBar) {
        // The `_x` bar has one chevron column per slot, drawn over the box. The PC bar likewise
        // ships BTN_ACTIONUP0-5 and no BTN_ACTIONDOWN*, so the down control stays unbound in both
        // layouts; give it a name that cannot resolve rather than a second arrow.
        ConsoleBind(pInterface, m_btnUp, labelTmp, "LBH_ARROW", nConsoleBox);
        m_btnUp.m_tail.bIgnoreMouse = 1;
#ifdef K2_HOST_BUILD
        ConsoleRecordSizes(pInterface, *this, nMode == 0 ? nSlot : ACTION_SLOT_TARGET_LEFT + nSlot, nConsoleBox);
#endif
        sName.Format("BTN_%sDOWN%d", sMode.CStr(), nSlot);
        pInterface->InitControl(m_btnDown, sName, 1, 1, bOffsetToEdge, 0, 0);
    } else {
#endif
    sName.Format(const_cast<char*>("BTN_%sUP%d"), sMode.CStr(), nSlot);
    pInterface->InitControl(m_btnUp, sName, nMode == 0, 1, bOffsetToEdge, 0, 0);
    sName.Format(const_cast<char*>("BTN_%sDOWN%d"), sMode.CStr(), nSlot);
    pInterface->InitControl(m_btnDown, sName, nMode == 0, 1, bOffsetToEdge, 0, 0);
#ifdef K2_UI_XBOX
    }
#endif
    if (m_nMode != 0) {
        m_button.AddChildControl(&m_icon);
        m_icon.m_pParent = &m_button;
        m_button.AddChildControl(&m_btnUp);
        m_btnUp.m_pParent = &m_button;
        m_button.AddChildControl(&m_btnDown);
        m_btnDown.m_pParent = &m_button;
        nSlot += ACTION_SLOT_TARGET_LEFT;
    }
    void (CSWGuiObject::*pfnUp)(CSWGuiControl*) = nMode != 0 ? static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnTargetUpArrowPressed)
        : static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnActionUpArrowPressed);
    void (CSWGuiObject::*pfnDown)(CSWGuiControl*) = nMode != 0 ? static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnTargetDownArrowPressed)
        : static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnActionDownArrowPressed);
    void (CSWGuiObject::*pfnFocus)(CSWGuiControl*) = nMode != 0 ? static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnTargetActionFocus)
        : static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnDefaultActionFocus);
    m_icon.m_tail.bFocusOnHover = 0;
    m_icon.AddEvent(INPUTEVENT_GUI_A, pInterface, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnDefaultActionLeft));
    m_icon.AddEvent(SWGUIEVENT_FOCUS, pInterface, pfnFocus);
    m_icon.AddEvent(SWGUIEVENT_WHEEL_UP, pInterface, pfnUp);
    m_icon.AddEvent(SWGUIEVENT_WHEEL_DOWN, pInterface, pfnDown);
    m_icon.m_tail.m_nUserData = nSlot;
    m_button.m_tail.bFocusOnHover = 0;
    m_button.AddEvent(SWGUIEVENT_FOCUS, pInterface, pfnFocus);
    m_button.AddEvent(SWGUIEVENT_WHEEL_UP, pInterface, pfnUp);
    m_button.AddEvent(SWGUIEVENT_WHEEL_DOWN, pInterface, pfnDown);
#if !KOTOR1  // K1 has no personal-action exit handler
    m_button.AddEvent(SWGUIEVENT_UNFOCUS, pInterface, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnPersonalActionExit));
#endif
    m_button.m_tail.m_nUserData = nSlot;
    m_btnUp.m_tail.bFocusOnHover = 0;
    m_btnUp.AddEvent(INPUTEVENT_GUI_A, pInterface, pfnUp);
    m_btnUp.AddEvent(SWGUIEVENT_FOCUS, pInterface, pfnFocus);
    m_btnUp.AddEvent(SWGUIEVENT_WHEEL_UP, pInterface, pfnUp);
    m_btnUp.AddEvent(SWGUIEVENT_WHEEL_DOWN, pInterface, pfnDown);
    m_btnUp.m_tail.m_nUserData = nSlot;
    m_btnDown.m_tail.bFocusOnHover = 0;
    m_btnDown.AddEvent(INPUTEVENT_GUI_A, pInterface, pfnDown);
    m_btnDown.AddEvent(SWGUIEVENT_FOCUS, pInterface, pfnFocus);
    m_btnDown.AddEvent(SWGUIEVENT_WHEEL_UP, pInterface, pfnUp);
    m_btnDown.AddEvent(SWGUIEVENT_WHEEL_DOWN, pInterface, pfnDown);
    m_btnDown.m_tail.m_nUserData = nSlot;
    m_btnUp.m_nToolTipStrRef = STRREF_TIP_ACTION_MENU_UP;
    m_btnUp.m_nBoundInputEventId = INPUTEVENT_ACTION_MENU_UP;
    m_btnDown.m_nToolTipStrRef = STRREF_TIP_ACTION_MENU_DOWN;
    m_btnDown.m_nBoundInputEventId = INPUTEVENT_ACTION_MENU_DOWN;
#ifndef K2_UI_TOUCH
    m_btnDown.m_border.m_params.m_fRotate = 180.0f;
    m_btnDown.m_borderHilight.m_params.m_fRotate = 180.0f;
#endif
    switch (nSlot) {
    case ACTION_SLOT_POWER: m_icon.m_nToolTipStrRef = STRREF_TIP_FRIENDLY_POWER; m_icon.m_nBoundInputEventId = INPUTEVENT_PERSONAL_POWER_ACTION; break;
    case ACTION_SLOT_MEDICAL: m_icon.m_nToolTipStrRef = STRREF_TIP_MEDICAL_ITEM; m_icon.m_nBoundInputEventId = INPUTEVENT_PERSONAL_MEDICAL_ACTION; break;
    case ACTION_SLOT_OTHER: m_icon.m_nToolTipStrRef = STRREF_TIP_NONMEDICAL_ITEM; m_icon.m_nBoundInputEventId = INPUTEVENT_PERSONAL_OTHER_ACTION; break;
    case ACTION_SLOT_MINES: m_icon.m_nToolTipStrRef = STRREF_TIP_MINE; m_icon.m_nBoundInputEventId = INPUTEVENT_PERSONAL_MINES_ACTION; break;
    case ACTION_SLOT_BEHAVIOR: m_icon.m_nToolTipStrRef = STRREF_TIP_SELECT_BEHAVIOR; break;
    case ACTION_SLOT_FORM: m_icon.m_nToolTipStrRef = STRREF_TIP_FORM; break;
    case ACTION_SLOT_TARGET_LEFT: m_icon.m_nToolTipStrRef = STRREF_TIP_LEFT_ACTION; m_icon.m_nBoundInputEventId = INPUTEVENT_TARGET_LEFT_ACTION; break;
    case ACTION_SLOT_TARGET_MIDDLE: m_icon.m_nToolTipStrRef = STRREF_TIP_MIDDLE_ACTION; m_icon.m_nBoundInputEventId = INPUTEVENT_TARGET_MIDDLE_ACTION; break;
    case ACTION_SLOT_TARGET_RIGHT: m_icon.m_nToolTipStrRef = STRREF_TIP_RIGHT_ACTION; m_icon.m_nBoundInputEventId = INPUTEVENT_TARGET_RIGHT_ACTION; break;
    }
}

void CSWGuiMainInterface::ClearSelectedAction() {
    g_nSelectedAction = -1;
    for (int i = 0; i < 9; i++)
        GetActionByIndex(i)->SetSelected(false);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::RemoveActionMenu() {
    if (m_nActionMenuGraceFrames > 0)
        return;
    if (m_bClearSelectionOnClose) {
        ClearSelectedAction();
        m_bClearSelectionOnClose = 0;
    }
    g_pGuiMan->RemovePanel(&m_actionMenuIos);
    IosActionMenuUp = 0;
}
#endif // !KOTOR1

static CSWGuiInterfaceAction* FindActionById(const CExoArrayList<CSWGuiInterfaceAction>& list, int nActionId);

static const uint64_t s_actionHintValues[6] = {
    0x96d5, 0x96d6, 0x96d7, 0xa5b6, 0xa602, 0xbb40
};

void CSWGuiTargetActionMenu::DoTargetAction(int nIndex, int nTarget) {
    K2_UNUSED(nTarget);
#if !KOTOR1
    if (m_pInterface)
        m_pInterface->ResetPartyMemberItemUse();
#endif
    if (m_actions[nIndex].m_count == 0)
        return;

    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
    CSWCCreature* pPlayer = pApp->GetSWParty()->GetPlayerCharacter();
    CSWGuiInterfaceAction* pAction =
        FindActionById(m_actions[nIndex], m_actionIds[static_cast<signed char>(m_actionRow)][nIndex]);
    if (!pPlayer)
        return;
    if (!pAction)
        return;

    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(pAction->m_nObjectId);
    if (!pObject)
        return;
    CSWCCreature* pTarget = static_cast<CSWCCreature*>(pObject->AsSWCObject());
    if (!pTarget)
        return;

    if ((pAction->m_nFlags & 1) && pAction->m_pfnAction) {
        int bResumeCombat;
        if (pPlayer->m_nStateFlags & 1) {
            if (pApp->GetAutoPaused())
                pApp->GetInGameGui()->SetPaused(1, 10);
            bResumeCombat = 0;
        } else {
            if (pApp->GetCombatMode())
                bResumeCombat = 0;
            else
                bResumeCombat = (m_actionRow == 3);
            pPlayer->ClearAllQueuedCombatActions();
            if (m_actionRow == 3 || (m_actionRow == 0 && pAction->m_nActionId == CSWGuiInterfaceAction::ACTION_ID_BASH)) {
                CSWGuiMainInterface* pInterface = m_pInterface;
                K2_ASSUME(pInterface != 0);   // the owning main interface, set by Initialize(this)
                pInterface->m_fCombatMessageTimer = -1.0f;
#if KOTOR1  // K1: the shared combat-message setter, K1's own string
                pInterface->SetCombatMessage(STRREF_COMBAT_MODE_ENGAGED);
#else
                pInterface->m_lblCombatModeMsg.m_text.m_params.SetColor(
                    CGuiInGame::COLOR_NEW_HIGHLIGHT);
                pInterface->m_combatMessage = STRREF_COMBAT_MODE_ENGAGED;
                pInterface->m_lblCombatModeMsg.m_text.m_params.SetText(
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_COMBAT_MODE_ENGAGED));
                pInterface->m_lblCombatModeMsg.m_text.m_params.m_fAlpha = 1.0f;
#endif
            }
        }
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PlayGuiSound(GUISOUND_ACTION_MENU_USE);
        (pTarget->*pAction->m_pfnAction)(pAction->m_nActionId, pPlayer);
        m_widgets[nIndex].m_nPulseSteps = 2;
        m_widgets[nIndex].m_fPulseTimer = 0.1f;
        if (!(pPlayer->m_nStateFlags & 1)) {
            if (bResumeCombat)
                pPlayer->SetCombatMode(1);
            if (pPlayer->GetServerCreature())
                pPlayer->GetServerCreature()->ClearAllActions(0);
        }
        return;
    }
    uint64_t nCategory = (pAction->m_nFlags >> 1) & 0xf;
    if (nCategory != 0) {
        if (nCategory - 1 <= 5) {
            K2_ASSUME(nCategory >= 1 && nCategory <= 6);   // same range as the test above
            m_nHintStrRef = s_actionHintValues[nCategory - 1];
        }
        m_fHintTimer = 5.0f;
    }
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PlayGuiSound(GUISOUND_ERROR_DEFAULT);
}

#ifdef K1_UI_XBOX
// xbox: the mouse on the console bar. Pointing at a box selects it, as the D-pad does; a click
// selects it and runs its action (A); the wheel steps through the box's entries (D-pad up/down).
// An empty box takes none of them.
bool CSWGuiMainInterface::K1ConsoleSelectBox(int nSlot) {
    if (nSlot < 0 || nSlot > 5 || m_personalActions[nSlot].m_count == 0)
        return false;
    int& rSel = s_k1Sel[s_k1Ctx];
    if (rSel != nSlot) {
        rSel = nSlot;
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
        PopulateMenus();
    }
    return true;
}

// Selects the box the pointer has moved onto. It polls the manager's hover control rather than
// taking the GUI focus, which would hand pad presses to the box as well as to HandleInputEvent;
// a pad press parks the pointer off screen, so only a real move gets here.
void CSWGuiMainInterface::K1ConsoleMouseHover() {
    static CSWGuiControl* s_pLast;
    CSWGuiControl* pOver = m_pGuiManager->m_pMouseOverControl;
    if (pOver == s_pLast)
        return;
    s_pLast = pOver;
    for (int i = 0; i < 6; i++)
        if (pOver == &m_personalActionWidgets[i].m_button || pOver == &m_personalActionWidgets[i].m_icon ||
            pOver == &m_personalActionWidgets[i].m_btnUp || pOver == &m_personalActionWidgets[i].m_btnDown)
            K1ConsoleSelectBox(i);
}

void CSWGuiMainInterface::OnK1ConsoleBoxClicked(CSWGuiControl* pControl) {
    if (K1ConsoleSelectBox(pControl->m_tail.m_nUserData))
        HandleInputEvent(INPUTEVENT_GUI_A, 1);
}

void CSWGuiMainInterface::OnK1ConsoleBoxWheelUp(CSWGuiControl* pControl) {
    if (K1ConsoleSelectBox(pControl->m_tail.m_nUserData))
        HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
}

void CSWGuiMainInterface::OnK1ConsoleBoxWheelDown(CSWGuiControl* pControl) {
    if (K1ConsoleSelectBox(pControl->m_tail.m_nUserData))
        HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
}
#endif

#if KOTOR1
void CSWGuiMainInterface::HandleInputEvent(int nType, int nValue) {
    K2_UNUSED(nValue);
#ifdef K1_UI_XBOX
    {
        CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
        if (!pPC)
            return;
        CSWSCreature* pPCServer = pPC->GetServerCreature();
        bool bBlocked = !pPCServer || pPCServer->GetDead() || pPCServer->GetDeadTemp();
        int& rSel = s_k1Sel[s_k1Ctx];
        switch (nType) {
        case INPUTEVENT_GUI_A:
        case INPUTEVENT_GUI_X:
            if (bBlocked || rSel < 0 || rSel > 5)
                return;
            if (nType == INPUTEVENT_GUI_A)
                pPC->ClearAllQueuedCombatActions();
            else if (s_k1Ctx == CSWCCreature::INTERFACE_TARGET_HOSTILE && !(pPC->m_nStateFlags & 1))
                pPC->SetCombatMode(1);
            m_pGuiManager->PlayGuiSound(nType == INPUTEVENT_GUI_X ? 8 : 6);
            DoPersonalAction(rSel, 0);
            return;
        case INPUTEVENT_GUI_B: {
            if (!pPCServer || !g_pAppManager->m_pClientExoApp->GetCombatMode())
                return;
            // With more than one action queued, B drops the last one and the fight goes on, as on the
            // Switch; dropping the last one leaves combat.
            CSWSCombatRound* pCombatRound = pPCServer->m_pCombatRound;
            if (pCombatRound && pCombatRound->m_pRoundActionList &&
                pCombatRound->m_pRoundActionList->m_pInternal->m_nCount >= 2) {
                pCombatRound->RemoveLastAction();
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                return;
            }
            if (pCombatRound)
                pCombatRound->RemoveLastAction();
            g_pAppManager->m_pClientExoApp->SetCombatMode(0);
            pPCServer->SetCombatMode(0, 0);
            pPCServer->ClearAllActions(1);
            m_fReticlePulseTimer = -1.0f;
            m_fChallengeRating = -1.0f;
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            return;
        }
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            pPC->ClearAllQueuedCombatActions();
            return;
        case INPUTEVENT_GUI_DPAD_LEFT:
        case INPUTEVENT_GUI_DPAD_RIGHT: {
            if (bBlocked || rSel < 0)
                return;
            int nDir = nType == INPUTEVENT_GUI_DPAD_RIGHT ? 1 : 5;
            for (int k = 1, n = (rSel + nDir) % 6; k < 6; k++, n = (n + nDir) % 6) {
                if (m_personalActions[n].m_count != 0) {
                    rSel = n;
                    m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                    PopulateMenus();
                    return;
                }
            }
            return;
        }
        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_DPAD_DOWN: {
            if (bBlocked || rSel < 0 || m_personalActions[rSel].m_count < 2)
                return;
            CSWGuiMainInterfaceAction& widget = m_personalActionWidgets[rSel];
            CResRef sOld = widget.m_icon.m_border.m_params.m_fillImage;
            float fOldAlpha = widget.m_icon.m_border.m_params.m_fAlpha;
            if (nType == INPUTEVENT_GUI_DPAD_UP)
                SelectNextPersonalAction(rSel);
            else
                SelectPrevPersonalAction(rSel);
            PopulateMenus();
            K1ConsoleStartScroll(widget, rSel, nType == INPUTEVENT_GUI_DPAD_UP, sOld, fOldAlpha);
            return;
        }
        default:
            return;
        }
    }
#endif
    CSWSCreature* pServer =
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter()->GetServerCreature();
    bool bInCombatRound = pServer && pServer->m_pCombatRound;
#ifdef K2_FIX_ENGINE_BUGS
    int nTarget = 0;
#else
    int nTarget;
#endif
    int nJump = 0;
    int nDir = 1;
    bool bClear = false;
    CSWGuiMainInterfaceAction* pAction;
    switch (nType) {
    case INPUTEVENT_GUI_A:
        if (ASLPlat_GetControllerPlugged() == 2) {
            if (g_nSelectedAction < 0)
                break;
            pAction = GetActionByIndex(g_nSelectedAction);
            if (!pAction->m_bSelected || pAction->m_bEmpty)
                return;
            if (g_nSelectedAction <= 3)
                DoPersonalAction(g_nSelectedAction, nTarget);
            else
                DoTargetAction(g_nSelectedAction - 4, nTarget);
            return;
        }
    lPopup:
        if (g_nSelectedAction < 0)
            break;
        pAction = GetActionByIndex(g_nSelectedAction);
        if (pAction->m_bSelected && !pAction->m_bEmpty) {
            PopupActionMenu(g_nSelectedAction);
            return;
        }
        break;
    case INPUTEVENT_GUI_B:
        if (g_nSelectedAction == -1) {
            if (!pServer)
                return;
            if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_IN_COMBAT_Y_BUTTON, OBJECT_INVALID, OBJECT_INVALID, 0))
                return;
            CSWSCombatRound* pCombatRound = pServer->m_pCombatRound;
            if (!bInCombatRound ||
                pCombatRound->m_pRoundActionList->m_pInternal->m_nCount < 2) {
                pCombatRound->RemoveLastAction();
                g_pAppManager->m_pClientExoApp->SetCombatMode(0);
                pServer->SetCombatMode(0, 0);
                pServer->ClearAllActions(1);
                m_fReticlePulseTimer = -1.0f;
                m_fChallengeRating = -1.0f;
                return;
            }
            pCombatRound->RemoveLastAction();
            return;
        }
        bClear = true;
        goto lDeselect;
    case INPUTEVENT_GUI_X:
        if (ASLPlat_GetControllerPlugged() != 2) {
            if (g_nSelectedAction != -1 || !bInCombatRound)
                return;
            nJump = 5;
            goto lDeselect;
        }
        if (g_nSelectedAction != -1)
            goto lPopup;
        nJump = 5;
        goto lDeselect;
    case INPUTEVENT_GUI_Y:
        PopupGamepadMenu();
        return;
    case INPUTEVENT_GUI_DPAD_LEFT:
        if (ASLPlat_GetControllerPlugged() != 2)
            return;
        nDir = -1;
        goto lDeselect;
    case INPUTEVENT_GUI_DPAD_RIGHT:
        if (ASLPlat_GetControllerPlugged() != 2)
            return;
        goto lDeselect;
    case INPUTEVENT_GUI_DPAD_UP:
        if (ASLPlat_GetControllerPlugged() != 2 || g_nSelectedAction >= 4)
            return;
        nJump = 4 - g_nSelectedAction;
        goto lDeselect;
    case INPUTEVENT_GUI_DPAD_DOWN:
        if (ASLPlat_GetControllerPlugged() != 2 || g_nSelectedAction <= 3)
            return;
        nJump = -g_nSelectedAction;
        goto lDeselect;
    default:
        return;
    }
    g_pAppManager->m_pClientExoApp->HandleInputEvent(INPUTEVENT_DEFAULT_ACTION, 1);
    return;

lDeselect:
    if (g_nSelectedAction >= 0) {
        if (CSWGuiMainInterfaceAction* pOld = GetActionByIndex(g_nSelectedAction))
            pOld->SetSelected(false);
    }
    if (bClear) {
        g_nSelectedAction = -1;
        return;
    }
    {
        int nIndex = (g_nSelectedAction + (nJump ? nJump : nDir) + 7) % 7;
        pAction = GetActionByIndex(nIndex);
        for (int i = 1; i < 7 && (!pAction || pAction->m_bEmpty); i++) {
            nIndex = (nIndex + nDir + 7) % 7;
            if (nIndex == 0) {
                g_nSelectedAction = -1;
                return;
            }
            pAction = GetActionByIndex(nIndex);
        }
        g_nSelectedAction = nIndex;
    }
    if (!pAction->m_bEmpty)
        pAction->SetSelected(true);
}
#else
void CSWGuiMainInterface::HandleInputEvent(int nType, int nValue) {
    K2_UNUSED(nValue);
    CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    if (!pPC)
        return;
    CSWSCreature* pServer = pPC->GetServerCreature();
    bool bInCombatRound = pServer && pServer->m_pCombatRound;
#ifdef K2_FIX_ENGINE_BUGS
    int nTarget = 0;
#else
    int nTarget;
#endif
    int nStep = -1;
    int nDir = -1;
    CSWGuiMainInterfaceAction* pAction = 0;
    bool bPanelWasOpen = false;
    bool bClearAll = false;
    switch (nType) {
    case INPUTEVENT_GUI_A:  // activate whatever the action bar points at
        if (g_pGuiMan->PanelExists(&m_actionMenuIos))
            return;
        g_initiatedByController = 1;
        if (g_nSelectedAction < 0) {
            g_pAppManager->m_pClientExoApp->HandleInputEvent(INPUTEVENT_DEFAULT_ACTION, 1);
            g_initiatedByController = 0;
            return;
        }
        pAction = GetActionByIndex(g_nSelectedAction);
        if (pAction->m_bSelected && !pAction->m_bEmpty) {
            if (g_nSelectedAction <= CSWGuiMainInterfaceAction::ACTION_SLOT_FORM)
                DoPersonalAction(g_nSelectedAction, nTarget);
            else
                DoTargetAction(g_nSelectedAction - CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT, nTarget);
        } else {
            ClearSelectedAction();
            g_pAppManager->m_pClientExoApp->HandleInputEvent(INPUTEVENT_DEFAULT_ACTION, 1);
        }
        g_initiatedByController = 0;
        return;
    case INPUTEVENT_GUI_B:  // back/clear: dropping the last queued action leaves combat mode
#ifdef K2_UI_XBOX
        // The console bar always keeps a box selected, so B only drops the last queued action or
        // leaves combat, as the Switch's does, and does nothing out of combat (K1's console bar
        // alike).
        if (!g_pAppManager->m_pClientExoApp->GetCombatMode())
            return;
        {
#else
        if (g_nSelectedAction == -1) {
#endif
            if (!pServer)
                return;
            CSWSCombatRound* pCombatRound = pServer->m_pCombatRound;
            if (!bInCombatRound ||
                pCombatRound->m_pRoundActionList->m_pInternal->m_nCount < 2) {
                pCombatRound->RemoveLastAction();
                g_pAppManager->m_pClientExoApp->SetCombatMode(0);
                pServer->SetCombatMode(0, 0);
                pServer->ClearAllActions(1);
                m_fReticlePulseTimer = -1.0f;
                m_fChallengeRating = -1.0f;
                return;
            }
            pCombatRound->RemoveLastAction();
            return;
        }
        if (g_pGuiMan->PanelExists(&m_actionMenuIos))
            return;
        nDir = 1;
        bClearAll = true;
        goto L_playSound;
    case INPUTEVENT_GUI_X:  // next/previous character
        g_pAppManager->m_pClientExoApp->ChangeCharacter(1, 1);
        return;
    case INPUTEVENT_GUI_Y:  // close the pop-up action menu, then the gamepad menu
        RemoveActionMenu();
        PopupGamepadMenu();
        return;
    case INPUTEVENT_GUI_BLACK:
    case INPUTEVENT_GUI_WHITE:
    case INPUTEVENT_GUI_START:
    case INPUTEVENT_GUI_BACK:
        return;
    case INPUTEVENT_GUI_DPAD_RIGHT:  // cycle the personal action selection forwards ...
        nDir = 1;
        // fall through
    case INPUTEVENT_GUI_DPAD_LEFT:  // ... or backwards
        bPanelWasOpen = g_pGuiMan->PanelExists(&m_actionMenuIos);
    L_playSound:
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->PlayGuiSound(GUISOUND_ACTION_MENU_SCROLL);
        if (CSWGuiMainInterfaceAction* pOld = GetActionByIndex(g_nSelectedAction))
            pOld->SetSelected(false);
        if (bClearAll) {
            ClearSelectedAction();
            SetActiveControl(0, 0);
        } else {
            if ((g_nSelectedAction & nDir) == -1) {
                g_nSelectedAction = 0;
            }
#pragma clang loop unroll_count(9)
            for (int i = 0; i < 9; i++) {
                g_nSelectedAction = (g_nSelectedAction + nDir + 9) % 9;
                pAction = GetActionByIndex(g_nSelectedAction);
                if (pAction && !pAction->m_bEmpty)
                    break;
            }
            if (!pAction->m_bEmpty) {
                pAction->SetSelected(true);
                SetActiveControl(&pAction->m_icon, 0);
            }
        }
        if (!bPanelWasOpen)
            return;
        nStep = 0;
        break;
    case INPUTEVENT_GUI_DPAD_UP:  // page the target menu ...
        if (g_pGuiMan->PanelExists(&m_actionMenuIos))
            return;
        break;
    case INPUTEVENT_GUI_DPAD_DOWN:  // ... forwards
        if (g_pGuiMan->PanelExists(&m_actionMenuIos))
            return;
        nStep = 1;
        break;
    default:
        return;
    }
    if (g_nSelectedAction < 0)
        return;
    pAction = GetActionByIndex(g_nSelectedAction);
    if (!pAction->m_bSelected || pAction->m_bEmpty)
        return;
    PopupActionMenu(g_nSelectedAction);
    SetActionMenuSelected(GetActiveActionIdx(g_nSelectedAction));
    UpdateActionMenuSelected(nStep);
}
#endif

#if KOTOR1  // K1: no left-mouse gate on the deactivate, and the new icon is stretched (fill style 2)
static __attribute__((always_inline)) void
SetupActionWidget(CSWGuiMainInterfaceAction& widget, CSWGuiInterfaceAction* pAction,
                  int64_t nCount, int bDimmed) {
#ifndef K2_UI_PC  // pc: only the mouse leaving the box deactivates it
    widget.m_button.SetActive(0);
#endif
    widget.m_button.m_border.m_params.m_fAlpha = 1.0f;
    widget.m_button.m_borderHilight.m_params.m_fAlpha = 1.0f;
    widget.m_btnUp.m_tail.bVisible = nCount > 1;
    widget.m_btnDown.m_tail.bVisible = nCount > 1;
    widget.m_bEmpty = 0;
    if (pAction->m_resRef != widget.m_icon.m_border.m_params.m_fillImage) {
        widget.m_icon.m_border.m_params.SetFillImage(pAction->m_resRef, 0);
        widget.m_icon.m_border.m_params.m_flags.nFillStyle = 2;
        widget.m_icon.m_borderHilight.m_params.SetFillImage(pAction->m_resRef, 0);
        widget.m_icon.m_borderHilight.m_params.m_flags.nFillStyle = 2;
        widget.m_nPulseSteps = 0;
    }
    float fAlpha = (pAction->m_nFlags & 1) == 0 || bDimmed ? 0.25f : 1.0f;
    widget.m_icon.m_border.m_params.m_fAlpha = fAlpha;
    widget.m_icon.m_borderHilight.m_params.m_fAlpha = fAlpha;
}
#else
static __attribute__((always_inline)) void
SetupActionWidget(CSWGuiMainInterfaceAction& widget, CSWGuiInterfaceAction* pAction,
                  int64_t nCount, int bDimmed) {
#ifndef K2_UI_PC  // pc: only the mouse leaving the box deactivates it
    if (!g_bLeftMouseDown)
        widget.m_button.SetActive(0);
#endif
    widget.m_button.m_border.m_params.m_fAlpha = 1.0f;
    widget.m_button.m_borderHilight.m_params.m_fAlpha = 1.0f;
    widget.m_btnUp.m_tail.bVisible = nCount > 1;
    widget.m_btnDown.m_tail.bVisible = nCount > 1;
    widget.m_bEmpty = 0;
    if (pAction->m_resRef != widget.m_icon.m_border.m_params.m_fillImage) {
        widget.m_icon.m_border.m_params.SetFillImage(pAction->m_resRef, 0);
        widget.m_icon.m_borderHilight.m_params.SetFillImage(pAction->m_resRef, 0);
        widget.m_nPulseSteps = 0;
    }
    float fAlpha = (pAction->m_nFlags & 1) == 0 || bDimmed ? 0.25f : 1.0f;
    widget.m_icon.m_border.m_params.m_fAlpha = fAlpha;
    widget.m_icon.m_borderHilight.m_params.m_fAlpha = fAlpha;
}
#endif

#ifdef K2_UI_XBOX
extern int g_aConsoleSlotList[6] = { 2, 3, 0, 1, 5, 4 };
#endif

#if KOTOR1
// K1: six personal lists and four action boxes; a leader change only records the new id (no
// sticky-slot revert); "dimmed" is GetDeadTemp; no selection reset at the end.
void CSWGuiMainInterface::PopulateMenus() {
    CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    CSWSCreature* pCreature = pPC ? pPC->GetServerCreature() : 0;
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_targetId);
    if (!pPC)
        return;
    int bDimmed = 1;
    if (pCreature && !pCreature->GetDead())
        bDimmed = pCreature->GetDeadTemp() != 0;
    if (pPC->m_objectId != m_lastPlayerId) {
        m_lastPlayerId = pPC->m_objectId;
#ifdef K1_UI_XBOX
        for (int c = 0; c < 5; c++)
            s_k1Sel[c] = 0;   // another party leader starts on box 1 for every kind
#endif
    }
    CSWCObject* pTarget = 0;
    if (pObject)
        pTarget = static_cast<CSWCObject*>(pObject->AsSWCObject());
#ifdef K1_UI_XBOX
    {
        K1ConsoleInitSticky();
        int nCtx = pPC->GetInterfaceTargetType(pTarget);
        if (static_cast<unsigned int>(nCtx) > 4)
            nCtx = CSWCCreature::INTERFACE_TARGET_NONE;
        if (nCtx != s_k1Ctx) {
            // leaving a hostile target ends combat mode unless the target is only changing; the kind
            // left comes back on box 1; each kind keeps the entries it last showed
            if (s_k1Ctx == CSWCCreature::INTERFACE_TARGET_HOSTILE && !g_pAppManager->m_pClientExoApp->GetTargetChanging())
                pPC->SetCombatMode(0);
            for (int i = 0; i < 6; i++)
                s_k1Sticky[s_k1Ctx][i] = m_personalActionIds[i];
            s_k1Sel[s_k1Ctx] = 0;
            s_k1Ctx = nCtx;
            for (int i = 0; i < 6; i++)
                m_personalActionIds[i] = s_k1Sticky[nCtx][i];
        }
        for (int nBox = 0; nBox < 6; nBox++) {
            CExoArrayList<CSWGuiInterfaceAction>& list = m_personalActions[nBox];
            for (int i = 0; i < list.m_count; i++)
                list.m_pArray[i].Clear();
            list.m_count = 0;
        }
        CSWCCreature* pTargetCreature = pTarget ? pTarget->AsSWCCreature() : 0;
        CSWCPlaceable* pTargetPlaceable = pTarget ? pTarget->AsSWCPlaceable() : 0;
        bool bHostilePlaceable = pTargetPlaceable != 0 && nCtx == CSWCCreature::INTERFACE_TARGET_NONHOSTILE &&
                                 pTargetPlaceable->IsHostile() != 0;
        // box 1
        if (nCtx == CSWCCreature::INTERFACE_TARGET_HOSTILE && pTargetCreature)
            pTargetCreature->GetHostileInterfaceActions(static_cast<CSWCObject*>(pPC), 0, m_personalActions[0]);
        else if (nCtx == CSWCCreature::INTERFACE_TARGET_NONHOSTILE && pTargetCreature)
            pTargetCreature->GetFriendlyInterfaceActions(static_cast<CSWCObject*>(pPC), 0, m_personalActions[0]);
        else if (nCtx != CSWCCreature::INTERFACE_TARGET_NONE)
            pPC->GetTargetActions(pTarget, 0, m_personalActions[0]);
        if (m_personalActions[0].m_count == 0 && pTarget != 0 && nCtx != CSWCCreature::INTERFACE_TARGET_NONE) {
            // an unlocked door, a friendly creature, an inventoryless placeable: the target's own
            // builders list only the extras (Bash, Security ...); its plain Open / Talk / Use is
            // the engine's default action (K2's console bar does the same, ConsoleDefaultAction)
            CClientExoAppInternal* pApp = g_pAppManager->m_pClientExoApp->m_pInternal;
            pApp->GetDefaultActions();
            CExoArrayList<CSWGuiInterfaceAction>& rDefault = pApp->m_actions;
            if (rDefault.m_count >= 1 && rDefault.m_pArray[0].m_nActionId != CSWGuiInterfaceAction::ACTION_ID_NONE) {
                m_personalActions[0].SetSize(rDefault.m_count);
                for (int i = 0; i < rDefault.m_count; i++)
                    m_personalActions[0].m_pArray[i] = rDefault.m_pArray[i];
            }
        }
        if (m_personalActions[0].m_count == 0) {
            m_personalActions[0].SetSize(1);
            CSWGuiInterfaceAction& rNone = m_personalActions[0].m_pArray[0];
            rNone.m_sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NO_ACTION);   // No Action
            rNone.m_nActionId = CSWGuiInterfaceAction::ACTION_ID_NONE;
            rNone.m_nObjectId = pPC->m_objectId;
            rNone.m_resRef = const_cast<char*>("i_noaction");
        }
        // box 2
        if (nCtx == CSWCCreature::INTERFACE_TARGET_HOSTILE && pTargetCreature) {
            pPC->GetCombatFeatsSkills(m_personalActions[1], pTargetCreature);
        } else {
            if (nCtx == CSWCCreature::INTERFACE_TARGET_OPENABLE || nCtx == CSWCCreature::INTERFACE_TARGET_TRIGGER)
                pPC->GetTargetActions(pTarget, 1, m_personalActions[1]);
            pPC->GetNonCombatFeatsSkills(m_personalActions[1]);
        }
        // box 3
        if (nCtx == CSWCCreature::INTERFACE_TARGET_HOSTILE && pTarget)
            pPC->GetHostileForcePowers(pTarget, m_personalActions[2]);
        else if (bHostilePlaceable)
            pPC->GetTargetActions(pTarget, 1, m_personalActions[2]);
        // box 4
        if (nCtx == CSWCCreature::INTERFACE_TARGET_HOSTILE && pTarget)
            pPC->GetTargetActions(pTarget, 2, m_personalActions[3]);
        pPC->GetUsableItems(m_personalActions[3], 2);
        // box 5, box 6
        pPC->GetSelfForcePowers(m_personalActions[4]);
        pPC->GetUsableItems(m_personalActions[5], 1 | 4);

        m_targetActionMenu.PopulateMenus(pPC, bDimmed, pTarget);

        int& rSel = s_k1Sel[nCtx];
        if (s_k1Pending != -1) {
            // a new target: the box that offers the action selected before it, else box 1
            rSel = 0;
            bool bFound = false;
            for (int b = 0; b < 6 && !bFound; b++) {
                for (int i = 0; i < m_personalActions[b].m_count; i++) {
                    if (m_personalActions[b].m_pArray[i].m_nActionId == static_cast<unsigned int>(s_k1Pending)) {
                        rSel = b;
                        m_personalActionIds[b] = s_k1Pending;
                        bFound = true;
                        break;
                    }
                }
            }
            s_k1Pending = -1;
        } else if (rSel < 0) {
            rSel = 0;
        }
        if (rSel < 0 || rSel > 5 || m_personalActions[rSel].m_count == 0) {
            rSel = -1;
            for (int i = 5; i >= 0; i--)
                if (m_personalActions[i].m_count != 0)
                    rSel = i;
        }
        for (int i = 0; i < 6; i++) {
            int64_t nCount = m_personalActions[i].m_count;
            CSWGuiInterfaceAction* pAction = 0;
            for (int64_t k = 0; k < nCount; k++) {
                if (m_personalActions[i].m_pArray[k].m_nActionId ==
                    static_cast<unsigned int>(m_personalActionIds[i])) {
                    pAction = &m_personalActions[i].m_pArray[k];
                    break;
                }
            }
            if (!pAction && nCount >= 1) {
                pAction = &m_personalActions[i].m_pArray[0];
                m_personalActionIds[i] = pAction->m_nActionId;
            }
            if (pAction)
                SetupActionWidget(m_personalActionWidgets[i], pAction, nCount, bDimmed);
            else
                m_personalActionWidgets[i].Show(1, 0);
            K1ConsoleSetBox(m_personalActionWidgets[i], i, i == rSel, nCount == 0);
        }
        m_nFocusedAction = rSel >= 0 ? rSel : 0;
        UpdateActionDescription();
        return;
    }
#endif
    for (int i = 0; i < m_personalActions[0].m_count; i++)
        m_personalActions[0].m_pArray[i].Clear();
    m_personalActions[0].m_count = 0;
    pPC->GetPersonalActions(0, m_personalActions[0]);
    for (int i = 0; i < m_personalActions[1].m_count; i++)
        m_personalActions[1].m_pArray[i].Clear();
    m_personalActions[1].m_count = 0;
    pPC->GetPersonalActions(1, m_personalActions[1]);
    for (int i = 0; i < m_personalActions[2].m_count; i++)
        m_personalActions[2].m_pArray[i].Clear();
    m_personalActions[2].m_count = 0;
    pPC->GetPersonalActions(2, m_personalActions[2]);
    for (int i = 0; i < m_personalActions[3].m_count; i++)
        m_personalActions[3].m_pArray[i].Clear();
    m_personalActions[3].m_count = 0;
    pPC->GetPersonalActions(3, m_personalActions[3]);
    for (int i = 0; i < m_personalActions[4].m_count; i++)
        m_personalActions[4].m_pArray[i].Clear();
    m_personalActions[4].m_count = 0;
    pPC->GetPersonalActions(4, m_personalActions[4]);
    for (int i = 0; i < m_personalActions[5].m_count; i++)
        m_personalActions[5].m_pArray[i].Clear();
    m_personalActions[5].m_count = 0;
    pPC->GetPersonalActions(5, m_personalActions[5]);
    m_targetActionMenu.PopulateMenus(pPC, bDimmed, pTarget);
    for (int i = 0; i < 4; i++) {
        int64_t nCount = m_personalActions[i].m_count;
        CSWGuiInterfaceAction* pAction = 0;
        for (int64_t k = 0; k < nCount; k++) {
            if (m_personalActions[i].m_pArray[k].m_nActionId ==
                static_cast<unsigned int>(m_personalActionIds[i])) {
                pAction = &m_personalActions[i].m_pArray[k];
                break;
            }
        }
        if (pAction)
            SetupActionWidget(m_personalActionWidgets[i], pAction, nCount, bDimmed);
        else if (nCount >= 1)
            SetupActionWidget(m_personalActionWidgets[i], &m_personalActions[i].m_pArray[0],
                              nCount, bDimmed);
        else
            m_personalActionWidgets[i].Show(1, 0);
    }
    UpdateActionDescription();
}
#endif

#if !KOTOR1  // K2-only (K1 has PopulateMenus() above)
void CSWGuiMainInterface::PopulateMenus(int nUnused) {
    K2_UNUSED(nUnused);
    CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    CSWSCreature* pCreature = pPC ? pPC->GetServerCreature() : 0;
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(m_targetId);
    if (!pPC)
        return;
    // Dim everything while the leader is dead or down: 1 == "no action is usable right now".
    int bDimmed = 1;
    if (pCreature && !pCreature->GetDead()) {
        if (pCreature->m_bInParty == 0 || pCreature->GetCurrentHitPoints(0) >= 1)
            bDimmed = 0;
    }
    if (pPC->m_objectId != m_lastPlayerId) {
        m_lastPlayerId = pPC->m_objectId;
        RevertActionBarStates();
    }
    CSWCObject* pTarget = 0;
    if (pObject)
        pTarget = static_cast<CSWCObject*>(pObject->AsSWCObject());

    for (int i = 0; i < m_personalActions[0].m_count; i++)
        m_personalActions[0].m_pArray[i].Clear();
    m_personalActions[0].m_count = 0;
    pPC->GetPersonalActions(0, m_personalActions[0]);
    for (int i = 0; i < m_personalActions[1].m_count; i++)
        m_personalActions[1].m_pArray[i].Clear();
    m_personalActions[1].m_count = 0;
    pPC->GetPersonalActions(1, m_personalActions[1]);
    for (int i = 0; i < m_personalActions[2].m_count; i++)
        m_personalActions[2].m_pArray[i].Clear();
    m_personalActions[2].m_count = 0;
    pPC->GetPersonalActions(2, m_personalActions[2]);
    for (int i = 0; i < m_personalActions[3].m_count; i++)
        m_personalActions[3].m_pArray[i].Clear();
    m_personalActions[3].m_count = 0;
    pPC->GetPersonalActions(3, m_personalActions[3]);
    for (int i = 0; i < m_personalActions[4].m_count; i++)
        m_personalActions[4].m_pArray[i].Clear();
    m_personalActions[4].m_count = 0;
    pPC->GetPersonalActions(4, m_personalActions[4]);
    for (int i = 0; i < m_personalActions[5].m_count; i++)
        m_personalActions[5].m_pArray[i].Clear();
    m_personalActions[5].m_count = 0;
    pPC->GetPersonalActions(5, m_personalActions[5]);
    for (int i = 0; i < m_personalActions[6].m_count; i++)
        m_personalActions[6].m_pArray[i].Clear();
    m_personalActions[6].m_count = 0;
    pPC->GetPersonalActions(6, m_personalActions[6]);

    m_targetActionMenu.PopulateMenus(pPC, bDimmed, pTarget);


    for (int i = 0; i < 6; i++) {
        int64_t nCount = m_personalActions[i].m_count;
        CSWGuiInterfaceAction* pAction = 0;
        for (int64_t k = 0; k < nCount; k++) {
            if (m_personalActions[i].m_pArray[k].m_nActionId ==
                static_cast<unsigned int>(m_personalActionIds[i])) {
                pAction = &m_personalActions[i].m_pArray[k];
                break;
            }
        }
        if (pAction)
            SetupActionWidget(m_personalActionWidgets[i], pAction, nCount, bDimmed);
        else if (nCount >= 1)
            SetupActionWidget(m_personalActionWidgets[i], &m_personalActions[i].m_pArray[0],
                              nCount, bDimmed);
        else
            m_personalActionWidgets[i].Show(1, 0);
    }

    UpdateActionDescription();
    if (g_nSelectedAction >= 0 && GetActionByIndex(g_nSelectedAction)->m_bEmpty)
        ClearSelectedAction();
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterface::RevertActionBarStates() {
    CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    if (!pPC)
        return;
    CSWSCreature* pCreature = pPC->GetServerCreature();
    if (!pCreature)
        return;
    char nImplant = 0;
    int nImplantId = -1;
    if (pCreature->m_pStats->HasFeat(FEAT_IMPLANT_SWITCHING)) {
        CSWGlobalVariableTable* pTable =
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
        pTable->GetValueNumber(CExoString("CANDEROUS_CUR_IMPLANT"), nImplant);
        nImplantId = nImplant >= 1 && nImplant <= 4 ? nImplant + CSWGuiInterfaceAction::ACTION_ID_IMPLANT_BASE : -1;
    }
    unsigned short nState = pCreature->m_pStats->m_aiState - 9;
    int nStateId = nState <= 4 ? nState + CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_AGGRESSIVE : -1;
    int nStanceId = pCreature->m_stanceId;
    if (nStanceId > 0)
        nStanceId &= ~CSWGuiInterfaceAction::ACTION_ID_SPELL;
#ifdef K2_UI_XBOX
    // console HUD (K2_UI_XBOX): these three sticky ids belong to lists 0 (the implant, off
    // GetSelfForcePowers), 4 (the AI state, off GetBehaviors) and 5 (the combat stance), so under
    // the console's slot permutation they have to land in whichever slot now carries each list.
    for (int nSlot = 0; nSlot < 6; nSlot++) {
        const int nList = g_aConsoleSlotList[nSlot];
        if (nList == 0 && nImplantId >= 1)
            m_personalActionIds[nSlot] = nImplantId;
        if (nList == 4 && nStateId >= 1)
            m_personalActionIds[nSlot] = nStateId;
        if (nList == 5 && nStanceId >= 1)
            m_personalActionIds[nSlot] = nStanceId;
    }
#else
    if (nImplantId >= 1)
        m_personalActionIds[0] = nImplantId;
    if (nStateId >= 1)
        m_personalActionIds[4] = nStateId;
    if (nStanceId >= 1)
        m_personalActionIds[5] = nStanceId;
#endif
    PopulateMenus(0);
}
#endif // !KOTOR1

void CSWGuiMainInterface::HandleButtonActionX(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
#if KOTOR1
    g_pGuiMan->RemovePanel(&m_actionMenuIos);
    IosActionMenuUp = 0;
#else
    RemoveActionMenu();
#endif
    if (!m_actionMenuIos.m_bWasPausedByCombat)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
}

static CSWGuiInterfaceAction* FindActionById(
        const CExoArrayList<CSWGuiInterfaceAction>& list, int nActionId) {
    CSWGuiInterfaceAction* pAction = 0;
    if (nActionId != -1) {
        int64_t nCount = list.m_count;
        if (nCount >= 1) {
            CSWGuiInterfaceAction* pActions = list.m_pArray;
            for (int64_t i = 0; i < nCount; i++) {
                if (pActions[i].m_nActionId == static_cast<unsigned int>(nActionId)) {
                    pAction = &pActions[i];
                    break;
                }
            }
        }
    }
    if (!pAction)
        pAction = list.m_pArray;
    return pAction;
}

void CSWGuiMainInterface::DoPersonalAction(int nIndex, int nTarget) {
    K2_UNUSED(nTarget);
#if KOTOR1
    if (static_cast<unsigned int>(nIndex) > 5)
        return;
#else
    if (static_cast<unsigned int>(nIndex) > 6)
        return;
#endif
    if (g_bShiftActionState) {
        CSWCCreature* pShiftPlayer =
            g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
        if (pShiftPlayer) {
            pShiftPlayer->SetCombatMode(0);
            pShiftPlayer->ClearAllActions();
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_fReticlePulseTimer = -1.0f;
            m_fChallengeRating = -1.0f;
        }
    }
#if !KOTOR1
    ResetPartyMemberItemUse();
#endif
    m_nFocusedAction = nIndex;
    if (m_personalActions[nIndex].m_count == 0)
        return;

    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
    CSWCCreature* pPlayer = pApp->GetSWParty()->GetPlayerCharacter();
    CSWGuiInterfaceAction* pAction =
        FindActionById(m_personalActions[nIndex], m_personalActionIds[nIndex]);
    if (!pPlayer)
        return;
    if (!pAction)
        return;

    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(pAction->m_nObjectId);
    if (!pObject)
        return;
    CSWCCreature* pTarget = static_cast<CSWCCreature*>(pObject->AsSWCObject());
    if (!pTarget)
        return;

    if ((pAction->m_nFlags & 1) && pAction->m_pfnAction) {
        if (pPlayer->m_nStateFlags & 1) {
            if (pApp->GetAutoPaused())
                pApp->GetInGameGui()->SetPaused(1, 10);
        } else {
            pPlayer->ClearAllQueuedCombatActions();
            m_pGuiManager->PlayGuiSound(GUISOUND_ACTION_MENU_USE);
        }
        (pTarget->*pAction->m_pfnAction)(pAction->m_nActionId, pPlayer);
        m_personalActionWidgets[nIndex].m_nPulseSteps = 2;
        m_personalActionWidgets[nIndex].m_fPulseTimer = 0.1f;
        if (!(pPlayer->m_nStateFlags & 1) && pPlayer->GetServerCreature())
            pPlayer->GetServerCreature()->ClearAllActions(0);
        return;
    }
    uint64_t nCategory = (pAction->m_nFlags >> 1) & 0xf;
    if (nCategory != 0) {
        if (nCategory - 1 <= 5) {
            K2_ASSUME(nCategory >= 1 && nCategory <= 6);   // same range as the test above
            m_nHintStrRef = s_actionHintValues[nCategory - 1];
        }
        m_fChallengeRating = 5.0f;
    }
    m_pGuiManager->PlayGuiSound(GUISOUND_ERROR_DEFAULT);
}

CSWGuiMainInterfaceAction::CSWGuiMainInterfaceAction() {
    m_nPulseSteps = 0;
    m_nMode = 0;
    m_bSelected = 0;
    m_bEmpty = 1;
}

#ifdef K2_UI_XBOX
namespace {
CSWGuiColor k2TargetFrameColor;
CSWGuiColor k2TargetBarColor;
bool k2TargetColorsSaved = false;
}  // namespace
#endif

#if KOTOR1
// K1: no trailing int, no SetSelected; the four banner widgets are re-anchored in HUD units and
// the name label is widened on an iPhone.
void CSWGuiTargetActionMenu::Initialize(CSWGuiMainInterface* pInterface) {
    m_pInterface = pInterface;
    m_widgets[0].Initialize(pInterface, 0, 1);
    m_widgets[1].Initialize(pInterface, 1, 1);
    m_widgets[2].Initialize(pInterface, 2, 1);

    pInterface->InitControl(m_lblName, CExoString("LBL_NAME"), 0);
    pInterface->InitControl(m_lblNameBg, CExoString("LBL_NAMEBG"), 0);
    pInterface->InitControl(m_lblHealthBg, CExoString("LBL_HEALTHBG"), 0);
    pInterface->InitControl(m_progressBar, CExoString("PB_HEALTH"), 0);
#ifdef K1_UI_XBOX
    // xbox: the console banner's plate is LBL_INDICATE (lbl_miindic01f, around the name and the
    // health bar); `maininterface` has no LBL_NAMEBG / LBL_HEALTHBG.
    if (m_lblNameBg.m_extent.w <= 0) {
        pInterface->InitControl(m_lblNameBg, CExoString("LBL_INDICATE"), 0);
        m_lblNameBg.m_text.m_params.SetText(CExoString(""));
    }
#endif
    ShiftControlExtent(m_lblName, 0, 0);
    ShiftControlExtent(m_lblNameBg, 0, 0);
    ShiftControlExtent(m_lblHealthBg, 0, 0);
    ShiftControlExtent(m_progressBar, 0, 0);

    m_aViewport[2] = m_lblName.m_extent.w;
    int nEdge = m_lblName.m_extent.y + m_lblName.m_extent.h;
    m_nHealthBarGap = static_cast<unsigned char>(m_progressBar.m_extent.y - nEdge);
    m_nActionRowGap = static_cast<unsigned char>(m_widgets[0].m_button.m_extent.y - nEdge);

    SetNameLabel(CExoString(""));

    int nWidth, nHeight;
    if (IosIsIphone(nWidth, nHeight)) {
        CSWGuiExtent extent = m_lblName.m_extent;
        extent.h += g_nIphoneLabelGrow;
        extent.y -= g_nIphoneLabelGrow;
        m_lblName.SetExtent(extent);
    }
}
#endif

void CSWGuiTargetActionMenu::Initialize(CSWGuiMainInterface* pInterface, int bOffsetToEdge) {
    m_pInterface = pInterface;
    m_widgets[0].Initialize(pInterface, 0, 1, bOffsetToEdge);
    m_widgets[0].SetSelected(false);
    m_widgets[1].Initialize(pInterface, 1, 1, bOffsetToEdge);
    m_widgets[1].SetSelected(false);
    m_widgets[2].Initialize(pInterface, 2, 1, bOffsetToEdge);
    m_widgets[2].SetSelected(false);

    pInterface->InitControl(m_lblName, CExoString("LBL_NAME"), 0, 1, bOffsetToEdge, 0, 0);
    pInterface->InitControl(m_lblNameBg, CExoString("LBL_NAMEBG"), 0, 1, bOffsetToEdge, 0, 0);
    pInterface->InitControl(m_lblHealthBg, CExoString("LBL_HEALTHBG"), 0, 1, bOffsetToEdge, 0, 0);
    pInterface->InitControl(m_progressBar, CExoString("PB_HEALTH"), 0, 1, bOffsetToEdge, 0, 0);

#ifdef K2_UI_XBOX
    if (m_lblNameBg.m_extent.w <= 0) {
        pInterface->InitControl(m_lblNameBg, CExoString("LBL_INDICATE"), 0, 1, bOffsetToEdge, 0, 0);
        // A `.gui` label ships a placeholder text for layout sizing; LBL_INDICATE's is empty, but
        // clear it anyway so a layout that does carry one cannot print it under the target name.
        m_lblNameBg.m_text.m_params.SetText(CExoString(""));
    }
    // LBL_HEALTHBG stays collapsed: LBL_INDICATE encloses the bar too.
    k2TargetFrameColor = m_lblNameBg.m_border.m_params.m_color;
    k2TargetBarColor = m_progressBar.m_borderProgress.m_params.m_color;
    k2TargetColorsSaved = true;
#endif

    m_aViewport[2] = m_lblName.m_extent.w;
    int nEdge = (m_lblName.m_extent.y + m_lblName.m_extent.h) & 0xff;
    // A byte in K1, an int in K2.
    m_nHealthBarGap = static_cast<decltype(m_nHealthBarGap)>(m_progressBar.m_extent.y - nEdge);
    m_nActionRowGap = static_cast<decltype(m_nActionRowGap)>(m_widgets[0].m_button.m_extent.y - nEdge);

    SetNameLabel(CExoString(""));
}

void CSWGuiTargetActionMenu::SetFriend(int bFriendly, int bPoisoned) {
// K2 pc takes K1's body, as the macOS build also has it: the box art only, the
// layout's highlight (lbl_miscroll_hi) kept, no touch art.
#if KOTOR1 || defined(K2_UI_PC)
    CExoString sMain;
    if (bFriendly) {
        if (bPoisoned)
            m_progressBar.m_borderProgress.m_params.SetFillImage(CResRef("POISON_BAR"), 0);
        else
            m_progressBar.m_borderProgress.m_params.SetFillImage(CResRef("FRIEND_BAR"), 0);
        m_progressBar.m_borderProgress.m_params.m_flags.nFillStyle = 2;
        sMain = "lbl_miscroll_f";
    } else {
        if (bPoisoned)
            m_progressBar.m_borderProgress.m_params.SetFillImage(CResRef("POISON_BAR"), 0);
        else
            m_progressBar.m_borderProgress.m_params.SetFillImage(CResRef("ENEMY_BAR"), 0);
        m_progressBar.m_borderProgress.m_params.m_flags.nFillStyle = 2;
        sMain = "lbl_miscroll_h";
    }
    for (int i = 0; i < 3; i++) {
        CExoString sImage(sMain);
#ifdef K2_UI_TOUCH
        if (m_widgets[i].m_bSelected)
            sImage = "ios_self_box2";
#endif
        m_widgets[i].m_button.m_border.m_params.SetFillImage(CResRef(sImage), 0);
        m_widgets[i].m_button.m_border.m_params.m_flags.nFillStyle = 2;
    }
#ifdef K1_UI_XBOX
    CResRef sPlate(bFriendly ? "lbl_miindic01f" : "lbl_miindic01e");
    if (m_lblNameBg.m_border.m_params.m_fillImage != sPlate)
        m_lblNameBg.m_border.m_params.SetFillImage(sPlate, 0);
#endif
#else
    CExoString sMain, sArrow0, sArrow1;
    int nBarMode;
    if (bFriendly) {
#ifdef K2_UI_XBOX
#else
        if (bPoisoned)
            m_progressBar.m_borderProgress.m_params.SetFillImage(CResRef("POISON_BAR"), 0);
        else
            m_progressBar.m_borderProgress.m_params.SetFillImage(CResRef("FRIEND_BAR"), 0);
#endif
        sMain    = "lbl_miscroll_f";
        sArrow0  = "lbl_miarr_1";
        sArrow1  = "lbl_miarr_2";
        nBarMode = 4;
    } else {
#ifndef K2_UI_XBOX
        if (bPoisoned)
            m_progressBar.m_borderProgress.m_params.SetFillImage(CResRef("POISON_BAR"), 0);
        else
            m_progressBar.m_borderProgress.m_params.SetFillImage(CResRef("ENEMY_BAR"), 0);
#endif
        sMain    = "ios_self_box_r";
        sArrow0  = "ios_self_expand";
        sArrow1  = "ios_self_expand2";
        nBarMode = 0;
    }
#ifdef K2_UI_XBOX
    if (k2TargetColorsSaved) {
        const CSWGuiColor kHostile(0.74f, 0.11f, 0.0f);
        const CSWGuiColor kPoisoned(0.28f, 0.92f, 0.11f);
        m_lblNameBg.m_border.m_params.m_color = bFriendly ? k2TargetFrameColor : kHostile;
        m_progressBar.m_borderProgress.m_params.m_color =
            !bFriendly ? kHostile : (bPoisoned ? kPoisoned : k2TargetBarColor);
    }
#else
    m_widgets[0].m_button.m_border.m_params.SetFillImage(CResRef(sMain), 0);
    m_widgets[0].m_button.m_borderHilight.m_params.SetFillImage(CResRef(sMain), 0);
#endif
#ifdef K2_UI_TOUCH  // pc / xbox: the layout's own arrows, turned over by ActionInit (as the macOS SetFriend)
    m_widgets[0].m_btnDown.m_border.m_params.SetFillImage(CResRef(sArrow0), 0);
    m_widgets[0].m_btnDown.m_borderHilight.m_params.SetFillImage(CResRef(sArrow1), 0);
    m_widgets[0].m_btnDown.m_border.m_params.m_flags.nFlipStyle = nBarMode;
    m_widgets[0].m_btnDown.m_borderHilight.m_params.m_flags.nFlipStyle = nBarMode;
#endif
    m_widgets[1].m_button.m_border.m_params.SetFillImage(CResRef(sMain), 0);
    m_widgets[1].m_button.m_borderHilight.m_params.SetFillImage(CResRef(sMain), 0);
#ifdef K2_UI_TOUCH
    m_widgets[1].m_btnDown.m_border.m_params.SetFillImage(CResRef(sArrow0), 0);
    m_widgets[1].m_btnDown.m_borderHilight.m_params.SetFillImage(CResRef(sArrow1), 0);
    m_widgets[1].m_btnDown.m_border.m_params.m_flags.nFlipStyle = nBarMode;
    m_widgets[1].m_btnDown.m_borderHilight.m_params.m_flags.nFlipStyle = nBarMode;
#endif
    m_widgets[2].m_button.m_border.m_params.SetFillImage(CResRef(sMain), 0);
    m_widgets[2].m_button.m_borderHilight.m_params.SetFillImage(CResRef(sMain), 0);
#ifdef K2_UI_TOUCH
    m_widgets[2].m_btnDown.m_border.m_params.SetFillImage(CResRef(sArrow0), 0);
    m_widgets[2].m_btnDown.m_borderHilight.m_params.SetFillImage(CResRef(sArrow1), 0);
    m_widgets[2].m_btnDown.m_border.m_params.m_flags.nFlipStyle = nBarMode;
    m_widgets[2].m_btnDown.m_borderHilight.m_params.m_flags.nFlipStyle = nBarMode;
#endif
#endif
}

#ifdef K2_UI_XBOX
static void ConsoleEmptyContainerSuffix(CSWCObject* pTarget, CExoString& sName) {
    CSWCPlaceable* pPlaceable = pTarget->AsSWCPlaceable();
    if (pPlaceable == 0)
        return;
    CSWSPlaceable* pServer = pPlaceable->GetServerPlaceable();
    if (pServer == 0 || pServer->m_bHasInventory == 0)
        return;
    if (pServer->AsSWSPlaceable()->GetItemCount(0) != 0)
        return;
    int bUseSearch = 0;
    g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(pPlaceable->m_nAppearanceType,
                                                      CExoString("UseSearch"), &bUseSearch);
    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
    sName = sName + CExoString(" (") + pApp->GetGUIString(bUseSearch ? STRREF_NOTHING : STRREF_EMPTY) +
            CExoString(")");
}
#endif

#if KOTOR1 && defined(K2_QOL_EMPTY_CONTAINERS)
// KOTOR II's suffix on a looted container. K1's placeables.2da has no UseSearch column and its
// the talk table has no entry for this strref: English reads "(Empty)", every other language a
// neutral "*".
static void K1QolEmptyContainerSuffix(CSWCObject* pTarget, CExoString& sName) {
    CSWCPlaceable* pPlaceable = pTarget->AsSWCPlaceable();
    if (pPlaceable == 0)
        return;
    CSWSPlaceable* pServer = pPlaceable->GetServerPlaceable();
    if (pServer == 0 || pServer->m_bHasInventory == 0)
        return;
    if (pServer->AsSWSPlaceable()->GetItemCount(0) != 0)
        return;
    sName = sName + CExoString(g_pTlkTable->GetLanguageVersion() == 0 ? " (Empty)" : " *");
}
#endif

void CSWGuiTargetActionMenu::UpdateNameLabel(CSWCObject* pTarget) {
    CExoString sName;
#ifdef K2_UI_XBOX
    float fAlpha = 1.0f;
#endif
    if (pTarget != 0) {
        CSWSObject* pServer = pTarget->GetServerObject();
        if (pServer != 0) {
            unsigned int nType;
            CExoString unused;  // constructed and destroyed, never read
            pTarget->GetFeedbackInfo(sName, nType);
#ifdef K2_UI_XBOX
            if (nType != 0) {
                float t = static_cast<float>(nType) / 5000.0f;
                fAlpha = t < 0.5f ? t + t : 1.0f;
            } else {
                // For the console's banner text, ask the engine what this object is called
                // (ConsoleEmptyContainerSuffix does the same for a container).
                g_pAppManager->m_pClientExoApp->GetObjectName(pTarget->m_objectId, sName);
                ConsoleEmptyContainerSuffix(pTarget, sName);
            }
#elif defined(K1_UI_XBOX)
            // xbox: the banner names the target (the feedback line while it runs, else the
            // object's own name); the focused entry's name is the caption over the bar's job.
            if (nType == 0) {
                g_pAppManager->m_pClientExoApp->GetObjectName(pTarget->m_objectId, sName);
#ifdef K2_QOL_EMPTY_CONTAINERS
                K1QolEmptyContainerSuffix(pTarget, sName);
#endif
            }
#else
            if (nType == 0) {
                if (m_fHintTimer > 0.0f) {
                    sName = g_pAppManager->m_pClientExoApp->GetGUIString(m_nHintStrRef);
                } else {
                    int nIndex = static_cast<signed char>(m_nFocusedAction);
                    if ((m_widgets[nIndex].m_icon.m_tail.bActive ||
                         m_widgets[nIndex].m_btnUp.m_tail.bActive ||
                         m_widgets[nIndex].m_btnDown.m_tail.bActive ||
                         m_widgets[nIndex].m_button.m_tail.bActive) &&
                        nIndex >= 0 && m_actions[nIndex].m_count >= 1) {
                        int nActionId = m_actionIds[static_cast<signed char>(m_actionRow)][nIndex];
                        CSWGuiInterfaceAction* pAction = 0;
                        if (nActionId != -1) {
                            for (int i = 0; i < m_actions[nIndex].m_count; i++)
                                if (m_actions[nIndex].m_pArray[i].m_nActionId ==
                                    static_cast<unsigned int>(nActionId))
                                    pAction = &m_actions[nIndex].m_pArray[i];
                        }
                        if (!pAction)
                            pAction = m_actions[nIndex].m_pArray;
                        sName = pAction->m_sName;
                        if (pAction->m_nCount >= 2)
                            sName.Format(const_cast<char*>("%s (%d)"), sName.CStr(), pAction->m_nCount);
                    } else {
                        sName = pTarget->GetName();
#if KOTOR1 && defined(K2_QOL_EMPTY_CONTAINERS)
                        K1QolEmptyContainerSuffix(pTarget, sName);
#endif
#if !KOTOR1
                        if (pTarget->AsSWCPlaceable() != 0) {
                            CSWSPlaceable* pServerPlaceable =
                                pTarget->AsSWCPlaceable()->GetServerPlaceable();
                            if (pServerPlaceable != 0 && pServerPlaceable->m_bHasInventory != 0 &&
                                pServerPlaceable->AsSWSPlaceable()->GetItemCount(0) == 0) {
                                // A searchable-but-empty container names itself through the
                                // placeables.2da "UseSearch" column; the two strrefs are the
                                // yes/no halves of the same suffix.
                                int bSearch = 0;
                                g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(
                                    pTarget->AsSWCPlaceable()->m_nAppearanceType,
                                    CExoString("UseSearch"), &bSearch);
                                if (bSearch)
                                    sName = sName + CExoString(" (") +
                                            g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NOTHING) +
                                            CExoString(")");
                                else
                                    sName = sName + CExoString(" (") +
                                            g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_EMPTY) +
                                            CExoString(")");
                            }
                        }
#endif
                    }
                }
            }
#endif
            m_progressBar.SetMaxValue(pServer->GetMaxHitPoints(1));
            m_progressBar.SetCurValue(pServer->GetCurrentHitPoints(0));
        }
    }
    SetNameLabel(sName);
#ifdef K2_UI_XBOX
    // The name is restored to opaque on every refresh; the console fades a feedback line.
    m_lblName.m_text.m_params.m_fAlpha = fAlpha;
#else
    m_lblName.m_text.m_params.m_fAlpha = 1.0f;
#endif
}

#ifdef K2_UI_XBOX
static void ConsoleDefaultAction(CExoArrayList<CSWGuiInterfaceAction>& rColumn,
                                 CSWCObject* pTarget) {
    if (pTarget == 0 || rColumn.m_count != 0)
        return;
    CClientExoAppInternal* pApp = g_pAppManager->m_pClientExoApp->m_pInternal;
    pApp->GetDefaultActions();
    CExoArrayList<CSWGuiInterfaceAction>& rSrc = pApp->m_actions;
    if (rSrc.m_count < 1)
        return;
    rColumn.SetSize(rSrc.m_count);
    for (int i = 0; i < rSrc.m_count; i++)
        rColumn.m_pArray[i] = rSrc.m_pArray[i];
}


// The slot that currently carries one GetPersonalActions list, under whatever permutation
// g_aConsoleSlotList holds (the debug property can rewrite it at run time, so it is read back
// rather than assumed). Box number = slot + 2.
static int ConsoleSlotForList(int nList) {
    for (int i = 0; i < 6; i++) {
        if (g_aConsoleSlotList[i] == nList)
            return i;
    }
    return -1;
}

static void ConsoleHostileItems(CSWGuiMainInterface* pInterface,
                                CExoArrayList<CSWGuiInterfaceAction>& rColumn) {
    if (pInterface == 0 || rColumn.m_count < 1)
        return;
    const int nSlot = ConsoleSlotForList(3);
    if (nSlot < 0)
        return;
    CExoArrayList<CSWGuiInterfaceAction>& rList = pInterface->m_personalActions[nSlot];
    const int nAdd = rColumn.m_count;
    const int nOld = rList.m_count;
    rList.SetSize(nOld + nAdd);
    for (int i = nOld - 1; i >= 0; i--)
        rList.m_pArray[i + nAdd] = rList.m_pArray[i];
    for (int i = 0; i < nAdd; i++)
        rList.m_pArray[i] = rColumn.m_pArray[i];
}

static void ConsoleTargetSkillColumn(CExoArrayList<CSWGuiInterfaceAction>* paColumns,
                                     int nTargetType) {
    if (nTargetType != CSWCCreature::INTERFACE_TARGET_OPENABLE && nTargetType != CSWCCreature::INTERFACE_TARGET_TRIGGER)
        return;                       // target types 3/4: column 1 already holds GetHostileForcePowers
    const int nAdd = paColumns[1].m_count;
    if (nAdd < 1)
        return;
    CExoArrayList<CSWGuiInterfaceAction>& rCol = paColumns[0];
    const int nOld = rCol.m_count;
    int nBash = nOld;
    for (int i = 0; i < nOld; i++) {
        if (rCol.m_pArray[i].m_nActionId == CSWGuiInterfaceAction::ACTION_ID_BASH) {
            nBash = i;
            break;
        }
    }
    int nSecurity = 0;
    for (int i = 0; i < nAdd; i++)
        if (paColumns[1].m_pArray[i].m_nActionId == CSWGuiInterfaceAction::ACTION_ID_SECURITY)
            nSecurity++;
    rCol.SetSize(nOld + nAdd);
    // Make room for the Security entries at the Bash slot; the rest of column 1 appends after.
    for (int i = nOld - 1; i >= nBash; i--)
        rCol.m_pArray[i + nSecurity] = rCol.m_pArray[i];
    int nFront = nBash;
    int nBack = nOld + nSecurity;
    for (int i = 0; i < nAdd; i++) {
        if (paColumns[1].m_pArray[i].m_nActionId == CSWGuiInterfaceAction::ACTION_ID_SECURITY)
            rCol.m_pArray[nFront++] = paColumns[1].m_pArray[i];
        else
            rCol.m_pArray[nBack++] = paColumns[1].m_pArray[i];
    }
}

// Empty the two columns the console has no box for, once their content has been routed.
static void ConsoleClearSpareColumns(CExoArrayList<CSWGuiInterfaceAction>* paColumns) {
    for (int nCol = 1; nCol < 3; nCol++) {
        for (int i = 0; i < paColumns[nCol].m_count; i++)
            paColumns[nCol].m_pArray[i].Clear();
        paColumns[nCol].m_count = 0;
    }
}

static void ConsoleNoAction(CExoArrayList<CSWGuiInterfaceAction>& rColumn) {
    if (rColumn.m_count != 0)
        return;
    rColumn.SetSize(1);
    CSWGuiInterfaceAction& rEntry = rColumn.m_pArray[0];
    rEntry.Clear();
    rEntry.m_sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NO_ACTION);
    rEntry.m_nActionId = CSWGuiInterfaceAction::ACTION_ID_NONE;
    rEntry.m_resRef = CResRef("i_noaction");
}

static const int kConsoleBox2Slot = 0;      // widget 0 == LBH_BORDER2

static void ConsolePermuteLists(CSWGuiMainInterface* pInterface) {
    if (pInterface == 0)
        return;
    CSWGuiInterfaceAction* apArray[6];
    int anCount[6];
    int anCapacity[6];
    for (int i = 0; i < 6; i++) {
        apArray[i] = pInterface->m_personalActions[i].m_pArray;
        anCount[i] = pInterface->m_personalActions[i].m_count;
        anCapacity[i] = pInterface->m_personalActions[i].m_capacity;
    }
    for (int nSlot = 0; nSlot < 6; nSlot++) {
        const int nList = g_aConsoleSlotList[nSlot];
        pInterface->m_personalActions[nSlot].m_pArray = apArray[nList];
        pInterface->m_personalActions[nSlot].m_count = anCount[nList];
        pInterface->m_personalActions[nSlot].m_capacity = anCapacity[nList];
    }
}

static bool ConsoleHostilePlaceable(CSWCCreature* pPC, CSWCObject* pTarget) {
    CSWCPlaceable* pPlaceable = pTarget->AsSWCPlaceable();
    if (pPlaceable == 0 || pPlaceable->IsHostile() == 0)
        return false;
    CSWSPlaceable* pServer = pPlaceable->GetServerPlaceable();
    if (pServer == 0)
        return false;
    return pServer->GetReputation(pPC->m_objectId, 0) <= 10;
}

static void ConsoleHostileForcePowers(CSWGuiMainInterface* pInterface, CSWCCreature* pPC,
                                      CSWCObject* pTarget, int nTargetType) {
    if (pInterface == 0 || pPC == 0)
        return;
    CExoArrayList<CSWGuiInterfaceAction>& rList = pInterface->m_personalActions[kConsoleBox2Slot];
    for (int i = 0; i < rList.m_count; i++)
        rList.m_pArray[i].Clear();
    rList.m_count = 0;
    if (pTarget == 0)
        return;
    if (nTargetType == CSWCCreature::INTERFACE_TARGET_HOSTILE || (nTargetType == CSWCCreature::INTERFACE_TARGET_NONHOSTILE && ConsoleHostilePlaceable(pPC, pTarget)))
        pPC->GetHostileForcePowers(pTarget, rList);
}

static void ConsoleNonCombatFeats(CExoArrayList<CSWGuiInterfaceAction>& rColumn,
                                  CSWCCreature* pPC, int nTargetType) {
    if (pPC == 0 || nTargetType == CSWCCreature::INTERFACE_TARGET_TRIGGER || nTargetType == CSWCCreature::INTERFACE_TARGET_HOSTILE)
        return;
    pPC->GetNonCombatFeatsSkills(rColumn);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
static void ConsoleSwitchWeapons(CExoArrayList<CSWGuiInterfaceAction>& rColumn,
                                 CSWCCreature* pPC) {
    if (pPC == 0)
        return;
    CSWSCreature* pServer = pPC->GetServerCreature();
    if (pServer == 0 || pServer->m_pInventory == 0)
        return;
    CSWInventory* pInventory = pServer->m_pInventory;
    CSWItem* pRight1 = pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON);
    CSWItem* pRight2 = pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON2);
    CSWItem* pLeft1 = pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON);
    CSWItem* pLeft2 = pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON2);
    if (pRight1 == 0 && pRight2 == 0)
        return;
    int nStrRef;
    if (pRight1 != 0 && pRight2 != 0)
        nStrRef = (pLeft1 != 0 || pLeft2 != 0) ? STRREF_SWITCH_WEAPONS : STRREF_SWITCH_WEAPON;
    else if (pRight1 != 0)
        nStrRef = pLeft1 != 0 ? STRREF_UNEQUIP_WEAPONS : STRREF_UNEQUIP_WEAPON;
    else
        nStrRef = pLeft2 != 0 ? STRREF_EQUIP_WEAPONS : STRREF_EQUIP_WEAPON;
    const int nAt = rColumn.m_count;
    rColumn.SetSize(nAt + 1);
    CSWGuiInterfaceAction& rEntry = rColumn.m_pArray[nAt];
    rEntry.Clear();
    rEntry.m_sName = g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef);
    rEntry.m_nActionId = CSWGuiInterfaceAction::ACTION_ID_SWITCH_WEAPONS;
    rEntry.m_pfnAction = &CSWCCreature::ActionMenuSwitchWeapons;
    rEntry.m_nObjectId = pPC->m_objectId;
    rEntry.m_resRef = CResRef("i_swtchwpn");
}
#endif // !KOTOR1

static void ConsoleUsableItems(CSWGuiMainInterface* pInterface, CSWCCreature* pPC) {
    if (pInterface == 0 || pPC == 0)
        return;
    const int nSlot = ConsoleSlotForList(1);
    if (nSlot < 0)
        return;
    CExoArrayList<CSWGuiInterfaceAction>& rList = pInterface->m_personalActions[nSlot];
    for (int i = 0; i < rList.m_count; i++)
        rList.m_pArray[i].Clear();
    rList.m_count = 0;
    pPC->GetUsableItems(rList, 1 | 4);
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    for (int i = 0; i < rList.m_count; i++) {
        CSWGuiInterfaceAction& rEntry = rList.m_pArray[i];
        // the same id resolution PartyMenuUseItemSelect does with the entry's m_nActionId
        int nServerId = static_cast<int>(pServer->ClientToServerObjectId(rEntry.m_nActionId & ~CSWGuiInterfaceAction::ACTION_ID_ITEM));
        CSWSItem* pItem = pServer->GetItemByGameObjectID(nServerId);
        if (pItem == 0 || pItem->GetBaseItem() == 0)
            continue;
        const unsigned char nBaseType = pItem->GetBaseItem()->m_nItemType;
        if (nBaseType == ITEM_TYPE_STIMULANT || nBaseType == ITEM_TYPE_MEDICAL_EQUIPMENT || nBaseType == ITEM_TYPE_SQUAD_RECOVERY_KIT || nBaseType == ITEM_TYPE_DROID_REPAIR_EQUIPMENT)
            rEntry.m_pfnAction = &CSWCCreature::PartyMenuUseItemSelect;
    }
}
#endif

void CSWGuiTargetActionMenu::PopulateMenus(CSWCCreature* pPC, int bDimmed, CSWCObject* pTarget) {
    m_actionRow = static_cast<char>(pPC->GetTargetInterfaceTargetType(pTarget));

    for (int64_t i = 0; i < m_actions[0].m_count; i++)
        m_actions[0].m_pArray[i].Clear();
    m_actions[0].m_count = 0;
    pPC->GetTargetActions(pTarget, 0, m_actions[0]);
    for (int64_t i = 0; i < m_actions[1].m_count; i++)
        m_actions[1].m_pArray[i].Clear();
    m_actions[1].m_count = 0;
    pPC->GetTargetActions(pTarget, 1, m_actions[1]);
    for (int64_t i = 0; i < m_actions[2].m_count; i++)
        m_actions[2].m_pArray[i].Clear();
    m_actions[2].m_count = 0;
    pPC->GetTargetActions(pTarget, 2, m_actions[2]);
#ifdef K2_UI_XBOX
    const int nTargetType = pPC->GetInterfaceTargetType(pTarget);
    ConsoleTargetSkillColumn(m_actions, nTargetType);
    ConsoleDefaultAction(m_actions[0], pTarget);
    ConsoleNonCombatFeats(m_actions[0], pPC, nTargetType);
#if !KOTOR1  // K1 has no weapon sets
    if (pTarget != 0 &&
        (nTargetType == CSWCCreature::INTERFACE_TARGET_HOSTILE || (nTargetType == CSWCCreature::INTERFACE_TARGET_NONHOSTILE && ConsoleHostilePlaceable(pPC, pTarget))))
        ConsoleSwitchWeapons(m_actions[0], pPC);
#endif
    ConsoleNoAction(m_actions[0]);
    // ...and the six personal boxes. This runs between the HUD filling m_personalActions and
    // building its six widgets from them, the only place with the target still in hand. Permute the
    // lists into the console's order first, as everything below addresses the slot a list was moved
    // into:
    //   box 2 <- GetHostileForcePowers, the seventh builder GetPersonalActions never calls, and only
    //            for target types 3/4, as the console leaves the column empty for a door or a
    //            container;
    //   box 3 <- the target's throwable items, in front of the mine list it already holds;
    //   box 5 <- the usable items rebuilt at mask 1|4, the console's single item list.
    ConsolePermuteLists(m_pInterface);
    ConsoleHostileForcePowers(m_pInterface, pPC, pTarget, nTargetType);
    ConsoleHostileItems(m_pInterface, m_actions[2]);
    ConsoleUsableItems(m_pInterface, pPC);
    ConsoleClearSpareColumns(m_actions);
#endif

    for (int64_t i = 0; i < 3; i++) {
        CSWGuiMainInterfaceAction& widget = m_widgets[i];
        int64_t nCount = m_actions[i].m_count;
        if (nCount >= 1) {
            CSWGuiInterfaceAction* pAction = 0;
#ifdef K2_UI_XBOX
            // console HUD: column 0 is never empty here (the "No Action" placeholder), so this
            // search also runs with no target, where m_actionRow is the -1 "none" row; read no
            // sticky id then rather than m_actionIds[-1].
            const int nWant = m_actionRow >= 0 ? m_actionIds[static_cast<int>(m_actionRow)][i] : -1;
#endif
            for (int64_t k = 0; k < nCount; k++) {
                if (m_actions[i].m_pArray[k].m_nActionId ==
#ifdef K2_UI_XBOX
                    static_cast<unsigned int>(nWant)) {
#else
                    static_cast<unsigned int>(m_actionIds[static_cast<signed char>(m_actionRow)][i])) {
#endif
                    pAction = &m_actions[i].m_pArray[k];
                    break;
                }
            }
            if (pAction)
                SetupActionWidget(widget, pAction, nCount, bDimmed);
            else
                SetupActionWidget(widget, &m_actions[i].m_pArray[0], nCount, bDimmed);
        } else {
            widget.Show(1, 0);
        }
    }
    UpdateNameLabel(pTarget);
    PositionMenu(pPC, pTarget);
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
    if (m_pInterface != 0 && m_pInterface->m_pGuiManager != 0) {
        m_aViewport[0] = 0;
        m_aViewport[1] = 0;
        m_aViewport[2] = m_pInterface->m_pGuiManager->m_nScreenWidth;
        m_aViewport[3] = m_pInterface->m_pGuiManager->m_nScreenHeight;
    }
#endif
}

static inline int MinInt(int a, int b) { return a < b ? a : b; }
static inline int MaxInt(int a, int b) { return a > b ? a : b; }

#ifdef K2_FIX_ENGINE_BUGS
// The target the menu was last placed for (a file static keeps the class layout fixed).
static unsigned long long s_nMenuTargetId = OBJECT_INVALID;
#endif

void CSWGuiTargetActionMenu::PositionMenu(CSWCCreature* pPC, CSWCObject* pTarget) {
    K2_UNUSED(pPC);
    unsigned char bFlags = m_nMenuFlags;
    if (pTarget == 0) {
        m_nMenuFlags = static_cast<unsigned char>(bFlags & ~1);
        return;
    }
#ifdef K2_FIX_ENGINE_BUGS
    // The menu holds still while the pointer is over it (bit 1) or fading off it, so it can be
    // clicked -- for any target, the PC engine's rule (macOS KOTOR's PositionMenu). Out of combat a
    // new target (the keyboard turning the camera under a still pointer) drops the hold and the
    // menu goes to that target; in combat the queue under the pointer stays put.
    if (pTarget->m_objectId != s_nMenuTargetId) {
        s_nMenuTargetId = pTarget->m_objectId;
        if (g_pAppManager->m_pClientExoApp->GetCombatMode() == 0) {
            bFlags = static_cast<unsigned char>(bFlags & ~2);
            m_fHoverReleaseTimer = -1.0f;
        }
    }
#endif
    m_nMenuFlags = static_cast<unsigned char>(bFlags | 1);

    int64_t nI = 0;
    int nCount;
    do {
        nCount = m_actions[nI].m_count;
        nI++;
    } while (nI < 3 && nCount == 0);

    // The bottom edge follows the row of real content, the progress bar when there is nothing to
    // show, the first widget's arrow button otherwise.
    int nOldBottom = m_aViewport[3];
#if KOTOR1
    if (nCount == 0) {
        m_aViewport[3] = m_progressBar.m_extent.y + m_progressBar.m_extent.h;
    } else {
        int nBoxBottom = m_widgets[0].m_button.m_extent.y + m_widgets[0].m_button.m_extent.h;
        int nArrowBottom = m_widgets[0].m_btnDown.m_extent.y + m_widgets[0].m_btnDown.m_extent.h;
        m_aViewport[3] = nBoxBottom > nArrowBottom ? nBoxBottom : nArrowBottom;
    }
#else
    m_aViewport[3] = (nCount == 0)
        ? m_progressBar.m_extent.y + m_progressBar.m_extent.h
        : m_widgets[0].m_btnDown.m_extent.y + m_widgets[0].m_btnDown.m_extent.h;
#endif

    if ((bFlags & 2) != 0 || m_fHoverReleaseTimer > 0.0f) {
        // Already up (bit 1) or still fading (the 0.5f re-arm): keep the origin and just slide it
        // by however much the bottom edge moved.
        m_aViewport[1] += nOldBottom - m_aViewport[3];
    } else {
        Vector vec;
        unsigned char nType;
        if (pTarget->AsSWCCreature() != 0)
            nType = static_cast<unsigned char>(pTarget->AsSWCCreature()->GetMenuPoint(vec));
        else
            nType = pTarget->GetReticlePoint(vec);

        int nScreenW = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
        int nScreenH = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight;

        // nType == 0 leaves the rect exactly where the last frame put it (the clamp below still
        // runs against it); nType == 2 is "above/below the reticle", everything else "beside it".
        if (nType != 0) {
            if (nType == 2) {
                // Hug the edge of the reticle's own half of the screen.
                if (vec.x < static_cast<float>(nScreenW / 2))
                    m_aViewport[0] = m_aViewport[6] + m_aViewport[4] - m_aViewport[2];
                else
                    m_aViewport[0] = m_aViewport[4];
                if (vec.y < static_cast<float>(m_aViewport[5] + 32))
                    m_aViewport[1] = (m_aViewport[5] + m_aViewport[7] - 32) - m_aViewport[3];
                else if (!(vec.y > static_cast<float>(m_aViewport[5] + m_aViewport[7] - 32))) {
                    if (vec.y < static_cast<float>(nScreenH / 2))
                        m_aViewport[1] = nScreenH - static_cast<int>(vec.y + 32.0f + static_cast<float>(m_aViewport[3]));
                    else
                        m_aViewport[1] = static_cast<int>(vec.y - 32.0f - static_cast<float>(m_aViewport[3]));
                } else
                    m_aViewport[1] = m_aViewport[5];
            } else {
                // Place it beside the reticle, centred unless that would push the menu off screen.
                int nHalfW = m_aViewport[2] / 2;
                if (vec.x - static_cast<float>(nHalfW) < static_cast<float>(m_aViewport[4])) {
                    m_aViewport[0] = m_aViewport[4];
                    if (vec.y < static_cast<float>(m_aViewport[5] + 32))
                        m_aViewport[1] = m_aViewport[5];
                    else if (!(vec.y > static_cast<float>(m_aViewport[5] + m_aViewport[7] - 32)))
                        m_aViewport[1] = static_cast<int>(vec.y - 32.0f - static_cast<float>(m_aViewport[3]));
                    else
                        m_aViewport[1] =
                            (m_aViewport[5] + m_aViewport[7] - 32) - m_aViewport[3];
                } else if (vec.x + static_cast<float>(nHalfW) > static_cast<float>(m_aViewport[6] + m_aViewport[4])) {
                    m_aViewport[0] = m_aViewport[6] + m_aViewport[4] - m_aViewport[2];
                    if (vec.y < static_cast<float>(m_aViewport[5] + 32))
                        m_aViewport[1] = m_aViewport[5];
                    else if (!(vec.y > static_cast<float>(m_aViewport[5] + m_aViewport[7] - 32)))
                        m_aViewport[1] = static_cast<int>(vec.y - 32.0f - static_cast<float>(m_aViewport[3]));
                    else
                        m_aViewport[1] =
                            (m_aViewport[5] + m_aViewport[7] - 32) - m_aViewport[3];
                } else {
                    if (vec.y < static_cast<float>(m_aViewport[5] + 32)) {
                        m_aViewport[0] = static_cast<int>(vec.x - static_cast<float>(nHalfW));
                        m_aViewport[1] = m_aViewport[5];
                    } else if (!(vec.y > static_cast<float>(m_aViewport[5] + m_aViewport[7] - 32))) {
                        m_aViewport[0] = static_cast<int>(vec.x) - nHalfW;
                        m_aViewport[1] = static_cast<int>(vec.y) - m_aViewport[3] - 32;
                    } else {
                        m_aViewport[0] = static_cast<int>(vec.x - static_cast<float>(nHalfW));
                        m_aViewport[1] =
                            (m_aViewport[5] + m_aViewport[7] - 32) - m_aViewport[3];
                    }
                }
            }
        }
        m_aViewport[0] = MaxInt(m_aViewport[4],
            MinInt(m_aViewport[6] + m_aViewport[4] - m_aViewport[2], m_aViewport[0]));
        m_aViewport[1] = MaxInt(m_aViewport[5],
            MinInt(m_aViewport[7] + m_aViewport[5] - m_aViewport[3], m_aViewport[1]));
    }

#if !KOTOR1 && !defined(K2_UI_PC)  // pc: the menu follows its target
    if (nCount != 0) {
        // With actions to show, the menu hangs under the tooltip banner instead: centred on it and
        // 20 px below its bottom edge.
        CSWGuiExtent& e = m_pInterface->m_lblCombatModeMsg.m_extent;
        m_aViewport[0] = e.x + e.w / 2 - m_aViewport[2] / 2;
        m_aViewport[1] = e.y + e.h + 20;
    }
#endif
}

void CSWGuiTargetActionMenu::SetNameLabel(const CExoString& sName) {
#if KOTOR1
    m_lblName.m_text.m_params.SetText(sName);
#else
    CExoString sText = sName;
    int nLt = sText.Find('<', 0);
    int nBr = sText.Find('{', 0);
    if ((nLt & nBr) != -1) {
        CSWTlkTable* pTlk = g_pSWTlkTable;
        pTlk->m_pSpeakerObj =
            g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
        pTlk->m_gender = pTlk->GetObjectGender();
        pTlk->m_bSuppressEnabled |= 1;
        pTlk->ParseStr(sText);
    }
    m_lblName.m_text.m_params.SetText(sText);
#endif
    CSWGuiText* pName = &m_lblName.m_text;
    CSWGuiExtent extent = m_lblName.m_extent;
#ifdef K2_QOL_IMPROVEMENTS
#ifdef K2_UI_XBOX
    // The console plate is fixed, so the name keeps to the room above the health bar inside it.
    K2QolFitText(*pName, m_progressBar.m_extent.y > extent.y ? m_progressBar.m_extent.y - extent.y
                                                             : extent.h);
#else
    // The pop-up banner re-flows around the name (the bar and the action row follow it down).
    K2QolFitText(*pName, g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight / 4);
#endif
#endif
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
    int nIdeal = pName->GetIdealHeight();
    if (nIdeal > 0)
        extent.h = nIdeal;
    m_lblName.SetExtent(extent);
#else
    extent.h = pName->GetIdealHeight();
    m_lblName.SetExtent(extent);
    m_lblNameBg.SetExtent(extent);
    int nBarY = extent.h + extent.y;
    extent = m_progressBar.m_extent;
#if KOTOR1  // K1: the bar keeps its layout gap below the name, and the row gets no extra 5
    extent.y = nBarY + m_nHealthBarGap;
    m_progressBar.SetExtent(extent);
    m_lblHealthBg.SetExtent(extent);
    int nRowH = nBarY + m_nActionRowGap - m_widgets[0].m_button.m_extent.y;
#else
    extent.y = nBarY;
    m_progressBar.SetExtent(extent);
    m_lblHealthBg.SetExtent(extent);
    int nRowH = nBarY + m_nActionRowGap - m_widgets[0].m_button.m_extent.y + 5;
#endif
    for (int i = 0; i < 3; i++) {
        CSWGuiButton* apButton[4] = { &m_widgets[i].m_icon, &m_widgets[i].m_button, &m_widgets[i].m_btnUp,
                                      &m_widgets[i].m_btnDown };
        for (int j = 0; j < 4; j++) {
            CSWGuiExtent eB = apButton[j]->m_extent;
            eB.y += nRowH;
            apButton[j]->SetExtent(eB);
        }
    }
#endif
}

CSWGuiTargetActionMenu::CSWGuiTargetActionMenu() : m_aViewport() {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++)
            m_actionIds[i][j] = -1;
    m_actionRow = -1;
    m_nFocusedAction = 0;
    m_nMenuFlags &= 0xfc;
    m_fHintTimer = -1.0f;
    m_fHoverReleaseTimer = -1.0f;
}


void CSWGuiMainInterface::DoTargetAction(int nIndex, int nTarget) {
    if (static_cast<unsigned int>(nIndex) <= 2) {
        if (g_bShiftActionState) {
            CSWCCreature* pPlayer =
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            if (pPlayer) {
                pPlayer->SetCombatMode(0);
                pPlayer->ClearAllActions();
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                m_fReticlePulseTimer = -1.0f;
                m_fChallengeRating = -1.0f;
            }
        }
        m_targetActionMenu.DoTargetAction(nIndex, nTarget);
    }
}

#if KOTOR1
void CSWGuiMainInterface::PopupActionMenu(int nIndex) {
    if (nIndex < 0) {
        IosActionMenuCount = 0;
        return;
    }
    int nCount;
    CSWGuiInterfaceAction* pActions = GetActionsFromActionIndex(nIndex, nCount);
    IosActionMenuCount = nCount;
    if (nCount == 0)
        return;
    m_actionMenuIos.m_nActionIndex = nIndex;
    if (!g_pGuiMan->PanelExists(&m_actionMenuIos)) {
        g_pGuiMan->AddPanel(&m_actionMenuIos, 1, 1);
        m_actionMenuIos.m_bWasPausedByCombat = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
        if (!m_actionMenuIos.m_bWasPausedByCombat)
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 0, 0);
    }
    RelayoutActionMenu(nCount);
    CSWGuiInterfaceAction* pAction = pActions;
    for (int64_t i = 0; i < 32; i++, ++pAction) {
        CSWGuiButton& slot = m_actionSlots[i];
        if (i < nCount) {
            slot.m_border.m_params.SetFillImage(pAction->m_resRef, 0);
            slot.m_border.m_params.m_flags.nFillStyle = 2;
            slot.m_borderHilight.m_params.SetFillImage(pAction->m_resRef, 0);
            slot.m_borderHilight.m_params.m_flags.nFillStyle = 2;
            CExoString strText;
            strText = pAction->m_sName;
            if (pAction->m_nCount >= 2)
                strText.Format(const_cast<char*>("%s (%d)"), strText.CStr(), pAction->m_nCount);
            slot.m_text.m_params.SetText(strText);
            slot.m_tail.m_nUserData = i;
            slot.m_tail.bVisible = true;
        } else {
            slot.m_tail.bVisible = false;
        }
        m_actionSlots[i].SetActive(false);
    }
    m_actionMenuHalves[0].m_tail.bVisible = nIndex < 4;
    m_actionMenuHalves[1].m_tail.bVisible = nIndex > 3;
    m_actionMenuCloseButtons[0].m_tail.bVisible = nIndex < 4;
    m_actionMenuCloseButtons[1].m_tail.bVisible = nIndex > 3;
    IosActionMenuUp = 1;
    if (ASLPlat_IsControllerPlugged())
        UpdateActionMenuSelected(1);
}
#else
void CSWGuiMainInterface::PopupActionMenu(int nIndex) {
    if (nIndex < 0) {
        IosActionMenuCount = 0;
        m_nActionMenuGraceFrames = 2;
        return;
    }
    int nCount;
    CSWGuiInterfaceAction* pActions = GetActionsFromActionIndex(nIndex, nCount, 0);
    IosActionMenuCount = nCount;
    m_nActionMenuGraceFrames = 2;
    if (nCount == 0)
        return;
    if (g_nSelectedAction != nIndex) {
        CSWGuiMainInterfaceAction* pWidget = GetActionByIndex(nIndex);
        if (pWidget && !pWidget->m_bEmpty) {
            ClearSelectedAction();
            g_nSelectedAction = nIndex;
            pWidget->SetSelected(true);
        }
    }
    m_actionMenuIos.m_nActionIndex = nIndex;
    if (!g_pGuiMan->PanelExists(&m_actionMenuIos)) {
        g_pGuiMan->AddPanel(&m_actionMenuIos, 4, 1);
        m_actionMenuIos.m_bWasPausedByCombat = g_pAppManager->m_pClientExoApp->GetPausedByCombat();
        if (!m_actionMenuIos.m_bWasPausedByCombat)
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(1, 0, 0);
    }
    RelayoutActionMenu(nCount, nIndex > 5);
    CSWGuiInterfaceAction* pAction = pActions;
    for (int64_t i = 0; i < 32; i++, ++pAction) {
        CSWGuiButton& slot = m_actionSlots[i];
        if (i < nCount) {
            slot.m_border.m_params.SetFillImage(pAction->m_resRef, 0);
            slot.m_borderHilight.m_params.SetFillImage(pAction->m_resRef, 0);
            slot.m_border.m_params.m_flags.nFillStyle = 2;
            slot.m_borderHilight.m_params.m_flags.nFillStyle = 2;
            CExoString strText;
            strText = pAction->m_sName;
            if (pAction->m_nCount >= 2)
                strText.Format(const_cast<char*>("%s (%d)"), strText.CStr(), pAction->m_nCount);
            slot.m_text.m_params.SetText(strText);
            slot.m_tail.m_nUserData = i;
            slot.m_tail.bVisible = true;
        } else {
            slot.m_tail.bVisible = false;
        }
        m_actionSlots[i].SetActive(false);
    }
    m_actionMenuHalves[0].m_tail.bVisible = nIndex < 6;
    m_actionMenuHalves[1].m_tail.bVisible = nIndex > 5;
    IosActionMenuUp = 1;
}
#endif

#if KOTOR1
// K1: one fixed column of 32 rows (box, icon, caption) under IOS_ACTION_MENU_SHIFT_X; an iPhone
// tightens the row pitch by 4 per entry past eleven.
void CSWGuiMainInterface::RelayoutActionMenu(int nCount) {
    int nScreenW;
    int nScreenH;
    float fShrink = 0.0f;
    if (IosIsIphone(nScreenW, nScreenH))
        fShrink = static_cast<float>((nCount > 11 ? nCount : 11) * 4 - 44);
    CSWGuiExtent sExtent;
    for (int i = 0; i < 32; i++) {
        CSWGuiButton& slot = m_actionSlots[i];
        sExtent.x = static_cast<int>(IOS_ACTION_MENU_SHIFT_X + 650.0f);
        sExtent.y = static_cast<int>(s_fIosActionMenuTop + (s_fIosActionMenuPitch - fShrink) * static_cast<float>(i));
        sExtent.w = 360;
        sExtent.h = 56;
        AndroidExtentShift(sExtent, -1, -1);
        slot.SetExtent(sExtent);
        sExtent.x = static_cast<int>(IOS_ACTION_MENU_SHIFT_X + 950.0f);
        sExtent.y = static_cast<int>(s_fIosActionMenuTop + (s_fIosActionMenuPitch - fShrink) * static_cast<float>(i));
        sExtent.w = 56;
        sExtent.h = 56;
        AndroidExtentShift(sExtent, -1, -1);
        CSWGuiBorder* pBorder = &slot.m_border;
        pBorder->SetExtent(sExtent);
        CSWGuiBorder* pHilight = &slot.m_borderHilight;
        pHilight->SetExtent(sExtent);
        sExtent.x = static_cast<int>(IOS_ACTION_MENU_SHIFT_X + 640.0f);
        sExtent.y = static_cast<int>(s_fIosActionMenuTop + (s_fIosActionMenuPitch - fShrink) * static_cast<float>(i));
        sExtent.w = 300;
        sExtent.h = 56;
        AndroidExtentShift(sExtent, -1, -1);
        CSWGuiText* pText = &slot.m_text;
        pText->SetExtent(sExtent);
    }
}
#endif

#if !KOTOR1  // K2-only (K1 has RelayoutActionMenu(int) above)
void CSWGuiMainInterface::RelayoutActionMenu(int nCount, bool bTarget) {
    float fScale = GetResolutionTextScalar();
    int nRowSpan = static_cast<int>(fScale * g_fActionMenuRowSpan);
    CSWGuiExtent sHalf = m_actionMenuHalves[bTarget].m_extent;
    int nHalfW = sHalf.w;
    float fA = static_cast<float>(g_nScreenHeight);
    fA = (fA - IOS_PADDING_START_RES) / IOS_PADDING_DOUBLE_RES;
    fA = !(fA <= 1.0f) ? fA : 1.0f;
    int nAlign;
    int nY0 = static_cast<int>(g_fActionMenuBottomPad * fA + (g_fActionMenuTopPad * fA + static_cast<float>(nRowSpan * nCount)));
    float f56 = fScale * 56.0f;
    float f10 = fScale * 10.0f;
    int nX;
    int nSlotX;
    int nYExtra;
    int nH;
    float fTextX;
    if (bTarget) {
        nYExtra = 0;
        nH = static_cast<int>(static_cast<float>(sHalf.x + sHalf.w) - IOS_ACTION_MENU_SHIFT_X - f56);
        nX = sHalf.x;
        fTextX = IOS_ACTION_MENU_SHIFT_X + static_cast<float>(sHalf.x);
        CSWGuiManager* pGui = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        if (pGui->m_nScreenHeight <= nY0) {
            CSWGuiManager* pGui2 = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
            nRowSpan -= (nY0 - pGui2->m_nScreenHeight) / nCount;
            nY0 = pGui2->m_nScreenHeight;
        }
        nSlotX = nH;
        nAlign = 20;
    } else {
        nX = static_cast<int>(IOS_ACTION_MENU_SHIFT_X + static_cast<float>(sHalf.x));
        int nT = static_cast<int>(static_cast<float>(m_personalActionWidgets[0].m_button.m_extent.y)
                       - IOS_ACTION_EXPANSION_PADDING);
        nYExtra = nT - nY0;
        fTextX = f10 + (f56 + (IOS_ACTION_MENU_SHIFT_X + static_cast<float>(sHalf.x)));
        nSlotX = nX;
        nAlign = 17;
        if (nYExtra < 0) {
            nRowSpan += nYExtra / nCount;
            nY0 = nT;
            nYExtra = 0;
        }
    }
    sHalf.y = nYExtra;
    sHalf.h = nY0;
    m_actionMenuHalves[bTarget].SetExtent(sHalf);
    float fRowH = static_cast<float>(nRowSpan);
    fRowH = fRowH < f56 ? fRowH : f56;
    int nSlotH = static_cast<int>(fRowH);
    CSWGuiExtent sSlot;
    sSlot.x = 0;
    sSlot.y = 0;
    sSlot.w = 0;
    sSlot.h = 0;
    if (nCount >= 1) {
        int nSlots = nCount < 32 ? nCount : 32;
        int nYRun = static_cast<int>(g_fActionMenuTopPad * fA + static_cast<float>(nYExtra));
        int nTextX = static_cast<int>(fTextX);
        float f20 = fScale * 20.0f;
        CSWGuiButton* pSlot = m_actionSlots;
        for (int64_t i = 0; i < nSlots; i++, ++pSlot) {
            sSlot.x = nX;
            sSlot.y = nYRun;
            sSlot.w = nHalfW;
            sSlot.h = nRowSpan;
            pSlot->SetExtent(sSlot);
            pSlot->m_text.m_params.SetAlignment(nAlign);
            CSWGuiBorder* pSub = &pSlot->m_border;
            sSlot.x = nSlotX;
            sSlot.w = nSlotH;
            sSlot.h = nSlotH;
            pSub->SetExtent(sSlot);
            pSub = &pSlot->m_borderHilight;
            pSub->SetExtent(sSlot);
            CSWGuiText* pText = &pSlot->m_text;
            sSlot.x = nTextX;
            sSlot.w = static_cast<int>(static_cast<float>(nHalfW) - f56 - IOS_ACTION_MENU_SHIFT_X - f10 - f20);
            sSlot.h = nRowSpan;
            pText->SetExtent(sSlot);
            nYRun += nRowSpan;
        }
    }
}
#endif // !KOTOR1

void CSWGuiMainInterface::HandleButtonAction(int nAction, int nTarget) {
    HandleButtonActionX(0);
    int nCount;
#if KOTOR1
    CSWGuiInterfaceAction* pActions = GetActionsFromActionIndex(nAction, nCount);
    if (nAction >= 0 && nTarget >= 0 && nTarget < nCount && pActions) {
    m_nFocusedAction = 0;
        int nActionId = pActions[nTarget].m_nActionId;
        if (nAction <= 3) {
            m_personalActionIds[nAction] = nActionId;
            DoPersonalAction(nAction, nTarget);
        } else {
            m_targetActionMenu.m_actionIds[static_cast<signed char>(m_targetActionMenu.m_actionRow)][nAction - 4] = nActionId;
            DoTargetAction(nAction - 4, nTarget);
        }
    }
#else
    CSWGuiInterfaceAction* pActions = GetActionsFromActionIndex(nAction, nCount, 0);
    if (nAction >= 0 && nTarget >= 0 && nTarget < nCount && pActions) {
    m_nFocusedAction = 0;
        int nActionId = pActions[nTarget].m_nActionId;
        if (nAction <= 5) {
            m_personalActionIds[nAction] = nActionId;
            DoPersonalAction(nAction, nTarget);
        } else {
            m_targetActionMenu.m_actionIds[static_cast<signed char>(m_targetActionMenu.m_actionRow)][nAction - 6] = nActionId;
            DoTargetAction(nAction - 6, nTarget);
        }
    }
#endif
}

void CSWGuiMainInterface::OnDefaultActionLeft(CSWGuiControl* pControl) {
    if (!pControl->m_tail.nCurrentEventArg)
        return;
    int nAction = static_cast<int>(pControl->m_tail.m_nUserData);
#if KOTOR1
    if (static_cast<unsigned int>(nAction) <= 3)
        DoPersonalAction(nAction, 0);
    else
        DoTargetAction(nAction - 4, 0);
#else
    if (g_pGuiMan->PanelExists(&m_actionMenuIos)) {
        m_bActionMenuLatch = 1;
        return;
    }
    if (static_cast<unsigned int>(nAction) <= 5)
        DoPersonalAction(nAction, 0);
    else
        DoTargetAction(nAction - 6, 0);
#endif
}


#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
// The port's override/custom.txt is 18 strings a language over 5 languages. A PC install's override
// has the Steam build's own custom.txt instead -- 11 a language, the same strings in a different
// order and without the port's -- or none at all. An index then answers with the Steam file's row
// for the same string where it has one, else the port's English.
static const char* const kCustomStringsEnglish[18] = {
    "Register Your Game",
    "Synchronizing Cloud Data",
    "No Data Available",
    "Cloud Sync Successful",
    "Cloud Sync Failed",
    "Force Speed Effects",
    "ON: Frame Buffer Effects are active when using the Force Speed power\n\nOFF: Frame Buffer "
    "Effects are not active when using the Force Speed power",
    "Framerate",
    "ON: Framerate is locked to 60 Frames Per Second.\n\nOFF: Framerate is unlocked.",
    "You have too many cloud saves. Please delete some old cloud saves before saving, or save a "
    "non-cloud save instead.",
    "More KOTOR",
    "Cloud Save",
    "This game needs Steam. Please make sure that Steam client is running and logged on with a Steam "
    "account that has this game in Library, then run the game again. The game will now exit.",
    "The Steam account you are logged on does not have this game in Library. Please switch Steam "
    "accounts to the correct one, or register the game on your Steam account using provided code. "
    "The game will now exit.",
    "Spacebar",
    "Escape",
    "Caps Lock",
    "%s has finished downloading.  You must restart the game to enable the mod.  Would you like to "
    "exit the game now?",
};
// The Steam file's row for each of the port's, -1 where it has none.
static const int kCustomStringsSteamRow[18] = { -1, -1, -1, -1, -1, 0, 1, -1, -1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

enum { kCustomPort, kCustomSteam, kCustomOther };

// Tells which file it is by its length, exactly 5 languages of 18, or of 11.
static int CustomStringsLayout(AspyrLocalizedStrings& strings)
{
    if (*strings.GetString(18 * 5 - 1, 0) && !*strings.GetString(18 * 5, 0))
        return kCustomPort;
    if (*strings.GetString(11 * 5 - 1, 0) && !*strings.GetString(11 * 5, 0))
        return kCustomSteam;
    return kCustomOther;
}
#endif

const char* GetLocalizedCustomString(int nIndex) {
    static AspyrLocalizedStrings s_customStrings("override/custom.txt", true);
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    static const int s_nLayout = CustomStringsLayout(s_customStrings);
    if (s_nLayout != kCustomPort && nIndex >= 0 && nIndex < 18) {
        if (s_nLayout == kCustomSteam && kCustomStringsSteamRow[nIndex] >= 0)
            return s_customStrings.GetString(kCustomStringsSteamRow[nIndex], 11);
        return kCustomStringsEnglish[nIndex];
    }
#endif
    return s_customStrings.GetString(nIndex, 18);
}

const char* GetLocalizedCustomMobileString(int nIndex) {
    static AspyrLocalizedStrings s_strings("override/mobile.txt", true);
    return s_strings.GetString(nIndex, 8);
}

#if KOTOR1
#include "port/ios/IosLocalizedStrings.h"
#endif

const char* GetLocalizedGamepadString(int nIndex) {
#if KOTOR1
    static IosLocalizedStrings s_strings("data/gamepad.txt", true);
    return s_strings.GetString(nIndex, 6);
#else
    static AspyrLocalizedStrings s_strings("override/gamepad.txt", true);
    return s_strings.GetString(nIndex, 8);
#endif
}

// Substitutes "" for a missing entry, unlike the three lookups above.
const char* GetLocalizedGmenuString(int nIndex) {
    static AspyrLocalizedStrings s_strings("override/gmenu.txt", true);
    const char* pResult = s_strings.GetString(nIndex, 3);
    if (!pResult)
        pResult = "";
    return pResult;
}

#if KOTOR1
static inline void OffsetControlX(CSWGuiControl& control, int nDelta) {
    CSWGuiExtent extent = control.m_extent;
    extent.x += nDelta;
    control.SetExtent(extent);
}

// An iPhone screen is not 1024x768: re-anchor a 1024x768 rectangle to the right/bottom...
static inline void IosAnchorRightBottom(CSWGuiExtent& extent, int nW, int nH) {
    extent.x += nW > 1023 ? nW - 1024 : -((1024 - nW) / 2);
    extent.y -= (768 - nH) / 2;
}

#ifdef K2_UI_TOUCH
// ...to the right/top...
static inline void IosAnchorRightTop(CSWGuiExtent& extent, int nW, int nH) {
    extent.x += nW > 1023 ? nW - 1024 : -((1024 - nW) / 2);
    extent.y += (768 - nH) / 2;
}

// ...or to the left/bottom.
static inline void IosAnchorLeftBottom(CSWGuiExtent& extent, int nW, int nH) {
    if (nW <= 1023)
        extent.x += (1024 - nW) / 2;
    extent.y -= (768 - nH) / 2;
}
#endif

#endif

#if KOTOR1
CSWGuiMainInterface::CSWGuiMainInterface(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager),
      m_gamepadMenu(pGuiManager, this),
      m_actionMenuIos(pGuiManager, this),
      m_helpPanel(pGuiManager, this)
{
    CExoString sName;
    CSWGuiLabel sScratch;

    CSWGuiFloatyText::m_pMainInterface = this;
#ifdef K2_UI_TOUCH
    StartLoadFromLayout(CResRef("ymipc210x7"));
#else
    // pc / xbox: `ymipc210x7` ships only as the phone variant; the PC HUD at the port's 1024x768
    // design size is the desktop layout `mipc210x7` (gui.bzf: the menu bar, the four action columns with
    // their arrows, the three target columns, TB_PAUSE / TB_SOLO / TB_STEALTH).
#ifdef K1_UI_XBOX
    // xbox: the console HUD, `maininterface` -- what the Switch loads at any width but 800
    // (mi8x6 is the same layout at 800x600): six LBH_BORDER boxes bottom left, the portraits
    // stacked bottom right, the minimap top right, no menu bar.
    StartLoadFromLayout(CResRef("maininterface"));
#else
    StartLoadFromLayout(CResRef("mipc210x7"));
#endif
#endif
    m_stateFlags |= 1;
    for (int i = 0; i < 6; i++)
        m_personalActionWidgets[i].Initialize(this, i, 0);
#pragma clang loop unroll(disable)
    for (int i = -3; i != 0; i++)
        m_aCharacters[i + 3].Initialize(this, i + 4);
    m_aCharacters[3].Initialize(this, 4);

    for (int i = 0; i < 4; i++) {
        sName.Format(const_cast<char*>("LBL_QUEUE%d"), i);  // idiom-exempt: char* literal, Format's parameter type
        InitControl(m_actionQueueIcons[i], sName, 1);
        ShiftControlExtent(m_actionQueueIcons[i], 0, -1);
    }
    for (int i = 0; i < 9; i++)
        m_statusIcons[i].Initialize(this, static_cast<unsigned char>(i));
    m_targetActionMenu.Initialize(this);
    sName.Format(const_cast<char*>("LBL_MOULDING%d"), 1);  // idiom-exempt: char* literal, Format's parameter type
    InitControl(m_lblMoulding1, sName, 1);
    sName.Format(const_cast<char*>("LBL_MOULDING%d"), 2);  // idiom-exempt: char* literal, Format's parameter type
    InitControl(m_lblMoulding2, sName, 1);
    sName.Format(const_cast<char*>("LBL_MOULDING%d"), 3);  // idiom-exempt: char* literal, Format's parameter type
    sName.Format(const_cast<char*>("LBL_COMBATBG%d"), 1);  // idiom-exempt: char* literal, Format's parameter type
    InitControl(m_lblCombatBg1, sName, 1);
    ShiftControlExtent(m_lblCombatBg1, 0, -1);
    sName.Format(const_cast<char*>("LBL_COMBATBG%d"), 2);  // idiom-exempt: char* literal, Format's parameter type
    InitControl(m_lblCombatBg2, sName, 1);
    ShiftControlExtent(m_lblCombatBg2, 0, -1);
    sName.Format(const_cast<char*>("LBL_COMBATBG%d"), 3);  // idiom-exempt: char* literal, Format's parameter type
    InitControl(m_lblCombatBg3, sName, 1);
    ShiftControlExtent(m_lblCombatBg3, 0, -1);
    InitControl(m_btnClearOne, CExoString("BTN_CLEARONE"), 1);
    InitControl(m_btnClearOne2, CExoString("BTN_CLEARONE2"), 1);
    InitControl(m_btnClearAll, CExoString("BTN_CLEARALL"), 1);
    ShiftControlExtent(m_btnClearOne, 0, -1);
    ShiftControlExtent(m_btnClearOne2, 0, -1);
    ShiftControlExtent(m_btnClearAll, 0, -1);
#ifdef K1_UI_XBOX
    // xbox: the console queue is the layout's (LBL_BG1 + LBL_HIGHLIGHT + LBL_QUEUE0..3, beside the
    // action box) and it has no BTN_CLEARALL to centre on.
    if (false)
#endif
    {
        // Centre BTN_CLEARALL on the screen and drag the whole combat-queue group along with it.
        CSWGuiExtent extent = m_btnClearAll.m_extent;
        int nOldX = extent.x;
        extent.x = (g_SDL_mode.w - extent.w) / 2;
        m_btnClearAll.SetExtent(extent);
        static bool s_bHasTouchScreen = HasTouchScreen();
        int nDelta = extent.x - nOldX;
        if (!s_bHasTouchScreen)
            m_btnClearAll.m_tail.bVisible = 0;
        OffsetControlX(m_actionQueueIcons[0], nDelta);
        OffsetControlX(m_actionQueueIcons[1], nDelta);
        OffsetControlX(m_actionQueueIcons[2], nDelta);
        OffsetControlX(m_actionQueueIcons[3], nDelta);
        OffsetControlX(m_lblCombatBg1, nDelta);
        OffsetControlX(m_lblCombatBg2, nDelta);
        OffsetControlX(m_lblCombatBg3, nDelta);
        OffsetControlX(m_actionQueueHilight, nDelta);
        OffsetControlX(m_btnClearOne, nDelta);
        OffsetControlX(m_btnClearOne2, nDelta);
    }
    InitControl(m_actionDescLabel, CExoString("LBL_ACTIONDESC"), 1);
    ShiftControlExtent(m_actionDescLabel, -1, -1);
    InitControl(m_actionDescLabel2, CExoString("LBL_ACTIONDESCBG"), 1);
    ShiftControlExtent(m_actionDescLabel2, -1, -1);
    InitControl(m_miniMap, CExoString("LBL_MAPBORDER"), 1);
    InitControl(m_lblArrow, CExoString("LBL_ARROW"), 0);
    InitControl(m_miniMapImage, CExoString("LBL_MAP"), 0);
    InitControl(sScratch, CExoString("LBL_MAPVIEW"), 0);
    InitControl(m_altMenuButton, CExoString("BTN_MINIMAP"), 1);
    InitControl(m_lblCombatModeMsg, CExoString("LBL_CMBTMODEMSG"), 1);
#ifdef K2_UI_TOUCH
    {
        CSWGuiExtent extent = m_lblCombatModeMsg.m_extent;
        extent.w -= 90;
        AndroidExtentShift(extent, 1, 0);
        m_lblCombatModeMsg.SetExtent(extent);
    }
    m_miniMap.m_border.m_params.SetFillImage(CResRef("ios_lbl_minimap"), 0);
    m_miniMap.m_border.m_params.m_flags.nFillStyle = 2;
#else
    ShiftControlExtent(m_lblCombatModeMsg, 0, 1);   // pc / xbox: the banner, top centre
#endif
    m_lblCombatModeMsg2.CSWGuiLabel::Initialize(m_lblCombatModeMsg.m_extent, &m_lblCombatModeMsg, 1.0f);
    InitControl(m_lblCombatMsgBg, CExoString("LBL_CMBTMSGBG"), 1);
#ifndef K2_UI_TOUCH
    ShiftControlExtent(m_lblCombatMsgBg, 0, 1);
#endif
    m_lblCombatModeMsg.m_tail.bVisible = 0;
    m_lblCombatMsgBg.m_tail.bVisible = 0;
    m_lblCombatModeMsg2.m_tail.bVisible = 0;
    m_lblCombatModeMsg2.m_text.m_params.SetText(g_pTlkTable->GetSimpleString(STRREF_HOTKEY_HINT));
    InitControl(m_btnEquip, CExoString("BTN_EQU"), 1);
    InitControl(m_btnInventory, CExoString("BTN_INV"), 1);
    InitControl(m_btnCharacter, CExoString("BTN_CHAR"), 1);
    InitControl(m_btnAbilities, CExoString("BTN_ABI"), 1);
    InitControl(m_btnMessages, CExoString("BTN_MSG"), 1);
    InitControl(m_btnJournal, CExoString("BTN_JOU"), 1);
    InitControl(m_btnMap, CExoString("BTN_MAP"), 1);
    InitControl(m_btnOptions, CExoString("BTN_OPT"), 1);
    InitControl(m_lblMenuBg, CExoString("LBL_MENUBG"), 1);
    InitControl(m_lblArrowMargin, CExoString("LBL_ARROW_MARGIN"), 1);
#ifdef K1_UI_XBOX
    // xbox: `maininterface` has no LBL_ARROW_MARGIN, and UpdateIndicator only puts the reticle on
    // the target inside this rectangle (inset 32) -- empty, every target got an edge arrow at the
    // left of the screen instead. The console's rectangle is the whole screen.
    if (m_lblArrowMargin.m_extent.w <= 0 || m_lblArrowMargin.m_extent.h <= 0)
        m_lblArrowMargin.SetExtent(
            CSWGuiExtent(0, 0, m_pGuiManager->m_nScreenWidth, m_pGuiManager->m_nScreenHeight));
#endif
#if defined(K2_UI_TOUCH) && defined(K2_FIX_ENGINE_BUGS)
    // touch: the layout's rectangle is in units of a 1024-high screen, so scale it into screen
    // pixels and span the whole width; the reticle then reaches the screen's edges on a larger one.
    {
        float fScale = static_cast<float>(m_pGuiManager->m_nScreenHeight) / 1024.0f;
        CSWGuiExtent sMargin = m_lblArrowMargin.m_extent;
        sMargin.x = static_cast<int>(fScale * sMargin.x);
        sMargin.w = m_pGuiManager->m_nScreenWidth - 2 * sMargin.x;
        sMargin.y = static_cast<int>(fScale * sMargin.y);
        sMargin.h = static_cast<int>(fScale * sMargin.h);
        m_lblArrowMargin.SetExtent(sMargin);
    }
#endif
    InitControl(m_tbPause, CExoString("TB_PAUSE"), 1);
    InitControl(m_tbSolo, CExoString("TB_SOLO"), 1);
    InitControl(m_tbStealth, CExoString("TB_STEALTH"), 1);
#ifdef K1_UI_XBOX
    // xbox: the console layout's own pieces in the slots its PC tags leave empty -- the portraits'
    // blue curve where a PC moulding was, the combat queue's backdrop and the ring over the
    // executing action (which no PC/touch tag names), and the caption plate's bottom cap
    // (LBL_ACTIONDESCBG2, under the box row; the plate above it is clipped to meet it in Draw).
    InitControl(m_lblMoulding1, CExoString("LBL_ACTIONDESCBG2"), 1);
    ShiftControlExtent(m_lblMoulding1, 0, 0);
    InitControl(m_lblMoulding2, CExoString("LBL_BG2"), 1);
    ShiftControlExtent(m_lblMoulding2, 0, 0);
    InitControl(m_lblCombatBg1, CExoString("LBL_BG1"), 1);
    ShiftControlExtent(m_lblCombatBg1, 0, -1);
    InitControl(m_actionQueueHilight, CExoString("LBL_HIGHLIGHT"), 1);
    ShiftControlExtent(m_actionQueueHilight, 0, -1);
    K1ConsoleSaveCaption(m_actionDescLabel.m_extent, m_actionDescLabel2.m_extent);
#endif
    StopLoadFromLayout();
#if !defined(K2_UI_TOUCH) && !defined(K1_UI_XBOX)
    // pc / xbox: the PC HUD's own pieces where its layout puts them -- the menu bar and its
    // backdrop top right, the three toggles and the mouldings bottom left -- and the target menu's
    // margin (read just below) in screen units.
    ShiftControlExtent(m_btnEquip, -1, 1);
    ShiftControlExtent(m_btnInventory, -1, 1);
    ShiftControlExtent(m_btnCharacter, -1, 1);
    ShiftControlExtent(m_btnAbilities, -1, 1);
    ShiftControlExtent(m_btnMessages, -1, 1);
    ShiftControlExtent(m_btnJournal, -1, 1);
    ShiftControlExtent(m_btnMap, -1, 1);
    ShiftControlExtent(m_btnOptions, -1, 1);
    ShiftControlExtent(m_lblMenuBg, -1, 1);
    ShiftControlExtent(m_tbPause, 1, -1);
    ShiftControlExtent(m_tbSolo, 1, -1);
    ShiftControlExtent(m_tbStealth, 1, -1);
    ShiftControlExtent(m_lblMoulding1, 1, -1);
    ShiftControlExtent(m_lblMoulding2, 1, -1);
    ShiftControlExtent(m_lblArrowMargin, 0, 0);
    ShiftControlExtent(m_altMenuButton, 1, 1);
    // The reticle's margin spans the screen's width; the layout's is the 4:3 composition's, which
    // ends three quarters across a 16:9 screen.
    {
        CSWGuiExtent sMargin = m_lblArrowMargin.m_extent;
        sMargin.w = m_pGuiManager->m_nScreenWidth - 2 * sMargin.x;
        m_lblArrowMargin.SetExtent(sMargin);
    }
#endif

    m_targetActionMenu.m_aViewport[4] = m_lblArrowMargin.m_extent.x;
    m_targetActionMenu.m_aViewport[5] = m_lblMenuBg.m_extent.y + m_lblMenuBg.m_extent.h;
    m_targetActionMenu.m_aViewport[6] = m_pGuiManager->m_nScreenWidth - m_targetActionMenu.m_aViewport[4] * 2;
    m_targetActionMenu.m_aViewport[7] = m_tbPause.m_extent.y - m_targetActionMenu.m_aViewport[5];
    AddControl(&m_lblCombatModeMsg2);
    if (m_pGuiManager->m_nScreenWidth >= 1024) {
        CSWGuiExtent sExtent;
        sExtent.x = m_extent.x;
        sExtent.y = m_extent.y;
        sExtent.w = g_nScreenWidth;
        sExtent.h = g_nScreenHeight;
        SetExtent(sExtent);
    }
    int nSafeMarginA;
    int nSafeMarginB;
    m_pGuiManager->GetSafeMargin(nSafeMarginA, nSafeMarginB);
    m_interfaceOffset = 0;
    m_fCombatMessageTimer = -1.0f;
    CSWGuiExtent sMap = m_miniMapImage.m_extent;
    m_miniMapImage.SetExtent(sMap);
    m_nMapWidth = m_miniMapImage.m_extent.w;
    m_nMapHeight = m_miniMapImage.m_extent.h;
#ifdef K1_UI_XBOX
    // xbox: LBL_MAP is the map texture's 512 in `maininterface`'s 640 units, which the layout scale
    // made 819; the image size and GetPartyMemberMapLocation's projection are in the texture's own.
    m_nMapWidth = m_nMapWidth * 640 / 1024;
    m_nMapHeight = m_nMapHeight * 640 / 1024;
#endif
    m_mapViewExtent = sScratch.m_extent;
    AndroidExtentShift(m_mapViewExtent, 1, 1);
    sScratch.SetExtent(m_mapViewExtent);
    CSWGuiExtent sArrow = m_lblArrow.m_extent;
    sArrow.w = static_cast<int>(AndroidHudScaleFactor() * sArrow.w);
    sArrow.h = static_cast<int>(AndroidHudScaleFactor() * sArrow.h);
    sArrow.x = (m_mapViewExtent.w - sArrow.w) / 2;
    sArrow.y = (m_mapViewExtent.h - sArrow.h) / 2;
    m_lblArrow.SetExtent(sArrow);
    CSWGuiExtent sMiniMap = m_miniMap.m_extent;
    AndroidExtentShift(sMiniMap, 1, 1);
    m_miniMap.SetExtent(sMiniMap);
#ifdef K1_UI_XBOX
    // xbox: no BTN_MINIMAP in `maininterface` either; the minimap's frame becomes the button that
    // opens the Map page (OnMenuButtonPressed's m_altMenuButton arm), as the touch HUD's does.
    if (m_altMenuButton.m_extent.w <= 0 || m_altMenuButton.m_extent.h <= 0) {
        m_altMenuButton.SetExtent(sMiniMap);
        bool bListed = false;
        for (int i = 0; i < m_controls.m_count; i++)
            if (m_controls.m_pArray[i] == &m_altMenuButton)
                bListed = true;
        if (!bListed)
            AddControl(&m_altMenuButton);
    }
#endif
    m_lblTargetIndicator.m_tail.bIgnoreMouse = 1;
    m_lblTargetIndicator.m_border.m_params.m_flags.nFillStyle = 2;
    AddControl(&m_lblTargetIndicator);
    CSWGuiExtent sReticle;
    sReticle.x = 0;
    sReticle.y = 0;
    sReticle.w = 16;
    sReticle.h = 16;
    m_reticleLabel.SetExtent(sReticle);
    m_reticleLabel.m_tail.bVisible = 0;
    m_reticleLabel.m_tail.bIgnoreMouse = 1;
    m_reticleLabel.m_border.m_params.m_flags.nFillStyle = 2;
    AddControl(&m_reticleLabel);
    m_mouseOverTargetId = OBJECT_INVALID;
    m_targetId = OBJECT_INVALID;
    m_lastPlayerId = OBJECT_INVALID;
    m_fReticlePulseTimer = -1.0f;
    m_fChallengeRating = -1.0f;
    for (int i = 0; i < 6; i++)
        m_personalActionIds[i] = -1;
    m_nInterfaceFlags |= 2;
    m_fRotationTimer = 0;
    ShowActionQueue(0);
    m_combatMessage = STRREF_COMBAT_MODE_ENGAGED;
    m_bToolTipMessageShown = 0;

    m_btnEquip.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnInventory.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnCharacter.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnAbilities.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnJournal.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnMessages.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnMap.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnOptions.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_altMenuButton.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnEquip.m_tail.bFocusOnHover = 0;
    m_btnInventory.m_tail.bFocusOnHover = 0;
    m_btnCharacter.m_tail.bFocusOnHover = 0;
    m_btnAbilities.m_tail.bFocusOnHover = 0;
    m_btnJournal.m_tail.bFocusOnHover = 0;
    m_btnMessages.m_tail.bFocusOnHover = 0;
    m_btnMap.m_tail.bFocusOnHover = 0;
    m_btnOptions.m_tail.bFocusOnHover = 0;
    m_altMenuButton.m_tail.bFocusOnHover = 0;
    m_btnClearOne.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnClearOneButtonPressed));
    m_btnClearOne2.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnClearOneButtonPressed));
    m_btnClearAll.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnClearAllButtonPressed));
    m_btnClearOne.m_tail.bFocusOnHover = 0;
    m_btnClearOne2.m_tail.bFocusOnHover = 0;
    m_btnClearAll.m_tail.bFocusOnHover = 0;
    m_lblArrowMargin.m_tail.bVisible = 0;
    m_lblArrowMargin.m_tail.bFocusOnHover = 0;
    m_tbPause.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnPauseButtonPressed));
    m_tbSolo.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnSoloButtonPressed));
    m_tbStealth.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnStealthButtonPressed));
    m_tbPause.m_tail.bFocusOnHover = 0;
    m_tbSolo.m_tail.bFocusOnHover = 0;
    m_tbStealth.m_tail.bFocusOnHover = 0;
    m_pFogOfWarImage = NewCAurGUIImage(const_cast<char*>("blackdot"));
    CSWGuiText* pDescText = &m_actionDescLabel.m_text;
    m_nFocusedAction = 0;
    m_lblCombatModeMsg.m_text.m_params.SetText(CExoString(""));
    m_nActionDescBottom = m_actionDescLabel.m_extent.y + m_actionDescLabel.m_extent.h;
    {
        CExoString sCaption("");
        m_actionDescLabel.m_text.m_params.SetText(sCaption);
        CSWGuiExtent sExtent = m_actionDescLabel.m_extent;
        sExtent.h = pDescText->GetIdealHeight();
        sExtent.y = m_nActionDescBottom - sExtent.h;
        m_actionDescLabel.SetExtent(sExtent);
        m_actionDescLabel2.SetExtent(sExtent);
    }
    UpdateActionDescription();
    m_btnEquip.m_nToolTipStrRef = STRREF_TIP_EQUIP;   m_btnEquip.m_nBoundInputEventId = INPUTEVENT_PANEL_EQUIP;
    m_btnInventory.m_nToolTipStrRef = STRREF_TIP_INVENTORY;   m_btnInventory.m_nBoundInputEventId = INPUTEVENT_PANEL_INVENTORY;
    m_btnCharacter.m_nToolTipStrRef = STRREF_TIP_CHARACTER;   m_btnCharacter.m_nBoundInputEventId = INPUTEVENT_PANEL_CHARACTER;
    m_btnMap.m_nToolTipStrRef = STRREF_TIP_MAP;   m_btnMap.m_nBoundInputEventId = INPUTEVENT_PANEL_MAP;
    m_altMenuButton.m_nToolTipStrRef = STRREF_TIP_MAP;    m_altMenuButton.m_nBoundInputEventId = INPUTEVENT_PANEL_MAP;
    m_btnAbilities.m_nToolTipStrRef = STRREF_TIP_ABILITIES;   m_btnAbilities.m_nBoundInputEventId = INPUTEVENT_PANEL_ABILITIES;
    m_btnJournal.m_nToolTipStrRef = STRREF_TIP_JOURNAL;   m_btnJournal.m_nBoundInputEventId = INPUTEVENT_PANEL_JOURNAL;
    m_btnOptions.m_nToolTipStrRef = STRREF_TIP_OPTIONS;    m_btnOptions.m_nBoundInputEventId = INPUTEVENT_PANEL_OPTIONS;
    m_btnMessages.m_nToolTipStrRef = STRREF_TIP_MESSAGES;  m_btnMessages.m_nBoundInputEventId = INPUTEVENT_PANEL_MESSAGES;
    m_tbPause.m_nToolTipStrRef = STRREF_TIP_PAUSE; m_tbPause.m_nBoundInputEventId = INPUTEVENT_PAUSE_ALT;
    m_tbSolo.m_nToolTipStrRef = STRREF_TIP_SOLO_MODE; m_tbSolo.m_nBoundInputEventId = INPUTEVENT_PARTY_ACTIVE;
    m_tbStealth.m_nToolTipStrRef = STRREF_SKILL_STEALTH;   m_tbStealth.m_nBoundInputEventId = INPUTEVENT_STEALTH;
    m_btnClearOne.m_nToolTipStrRef = STRREF_TIP_CLEAR_COMBAT_ACTION;     m_btnClearOne.m_nBoundInputEventId = INPUTEVENT_CLEAR_ONE_ACTION;
    m_btnClearOne2.m_nToolTipStrRef = STRREF_TIP_CLEAR_COMBAT_ACTION;     m_btnClearOne2.m_nBoundInputEventId = INPUTEVENT_CLEAR_ONE_ACTION;
    m_btnClearAll.m_nToolTipStrRef = STRREF_TIP_DISENGAGE;     m_btnClearAll.m_nBoundInputEventId = INPUTEVENT_CANCEL_COMBAT;
#ifndef K2_UI_PC  // pc: the caption sits on the bar, where the PC layout puts it
    m_nActionDescBottom = static_cast<int>(static_cast<float>(m_nActionDescBottom) + AndroidHudScaleFactor() * -40.0f);
#endif

    // The top-right menu buttons and the bottom-left moulding are K1 skins, hand-placed.
#ifdef K2_UI_TOUCH
    int nW;
    int nH;
    m_btnEquip.m_border.m_params.SetFillImage(CResRef("ios_lbl_miequ"), 0);
    m_btnEquip.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnEquip.m_borderHilight.m_params.SetFillImage(CResRef("ios_lbl_miequ2"), 0);
    m_btnEquip.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    CSWGuiExtent sExtent = { 927, 0, 96, 96 };
    if (IosIsIphone(nW, nH))
        IosAnchorRightTop(sExtent, nW, nH);
    AndroidExtentShift(sExtent, -1, 1);
    m_btnEquip.SetExtent(sExtent);
    m_btnInventory.m_border.m_params.SetFillImage(CResRef("ios_lbl_miinv"), 0);
    m_btnInventory.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnInventory.m_borderHilight.m_params.SetFillImage(CResRef("ios_lbl_miinv2"), 0);
    m_btnInventory.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    CSWGuiExtent sHidden = { 0, 10, 0, 0 };
    sExtent = sHidden;
    AndroidExtentShift(sExtent, 1, 1);
    m_btnInventory.SetExtent(sExtent);
    m_btnOptions.m_border.m_params.SetFillImage(CResRef("ios_lbl_miopt"), 0);
    m_btnOptions.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnOptions.m_borderHilight.m_params.SetFillImage(CResRef("ios_lbl_miopt2"), 0);
    m_btnOptions.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    sExtent = sHidden;
    AndroidExtentShift(sExtent, 1, 1);
    m_btnOptions.SetExtent(sExtent);
    sExtent = { 6, 512, 104, 148 };
    if (IosIsIphone(nW, nH))
        IosAnchorLeftBottom(sExtent, nW, nH);
    AndroidExtentShift(sExtent, 1, -1);
    m_lblMoulding1.m_border.m_params.SetFillImage(CResRef("ios_lower_left"), 0);
    m_lblMoulding1.m_border.m_params.m_flags.nFillStyle = 2;
    CSWGuiControl* pMoulding = &m_lblMoulding1;
    pMoulding->SetExtent(sExtent);

    // The three toggles sit bottom-left, white in both states.
    sExtent = { 12, 517, 92, 92 };
    if (IosIsIphone(nW, nH))
        IosAnchorLeftBottom(sExtent, nW, nH);
    AndroidExtentShift(sExtent, 1, -1);
    m_tbPause.SetExtent(sExtent);
    m_tbPause.m_border.m_params.m_color.r = CGuiInGame::COLOR_WHITE.x;
    m_tbPause.m_border.m_params.m_color.g = CGuiInGame::COLOR_WHITE.y;
    m_tbPause.m_border.m_params.m_color.b = CGuiInGame::COLOR_WHITE.z;
    m_tbPause.m_borderHilight.m_params.m_color.r = CGuiInGame::COLOR_WHITE.x;
    m_tbPause.m_borderHilight.m_params.m_color.g = CGuiInGame::COLOR_WHITE.y;
    m_tbPause.m_borderHilight.m_params.m_color.b = CGuiInGame::COLOR_WHITE.z;
    sExtent = { 10, 601, 46, 52 };
    if (IosIsIphone(nW, nH))
        IosAnchorLeftBottom(sExtent, nW, nH);
    AndroidExtentShift(sExtent, 1, -1);
    m_tbSolo.SetExtent(sExtent);
    m_tbSolo.m_border.m_params.m_color.r = CGuiInGame::COLOR_WHITE.x;
    m_tbSolo.m_border.m_params.m_color.g = CGuiInGame::COLOR_WHITE.y;
    m_tbSolo.m_border.m_params.m_color.b = CGuiInGame::COLOR_WHITE.z;
    m_tbSolo.m_borderHilight.m_params.m_color.r = CGuiInGame::COLOR_WHITE.x;
    m_tbSolo.m_borderHilight.m_params.m_color.g = CGuiInGame::COLOR_WHITE.y;
    m_tbSolo.m_borderHilight.m_params.m_color.b = CGuiInGame::COLOR_WHITE.z;
    sExtent = { 60, 601, 46, 52 };
    if (IosIsIphone(nW, nH))
        IosAnchorLeftBottom(sExtent, nW, nH);
    AndroidExtentShift(sExtent, 1, -1);
    m_tbStealth.SetExtent(sExtent);
    m_tbStealth.m_border.m_params.m_color.r = CGuiInGame::COLOR_WHITE.x;
    m_tbStealth.m_border.m_params.m_color.g = CGuiInGame::COLOR_WHITE.y;
    m_tbStealth.m_border.m_params.m_color.b = CGuiInGame::COLOR_WHITE.z;
    m_tbStealth.m_borderHilight.m_params.m_color.r = CGuiInGame::COLOR_WHITE.x;
    m_tbStealth.m_borderHilight.m_params.m_color.g = CGuiInGame::COLOR_WHITE.y;
    m_tbStealth.m_borderHilight.m_params.m_color.b = CGuiInGame::COLOR_WHITE.z;
#else
    CSWGuiExtent sExtent;
#endif

    // The pop-up action menu is full screen, two halves with a close button each, 32 slots.
    sExtent.x = 0;
    sExtent.y = 0;
    sExtent.w = g_SDL_mode.w;
    sExtent.h = g_SDL_mode.h;
    m_actionMenuIos.SetExtent(sExtent);
    m_actionMenuIos.m_vColor.x = -2.0f;
    m_actionMenuIos.m_vColor.y = -2.0f;
    m_actionMenuIos.m_vColor.z = -2.0f;
    m_actionMenuIos.m_stateFlags |= 1;
    int nScreenW;
    int nScreenH;
    if (IosIsIphone(nScreenW, nScreenH)) {
        s_fIosActionMenuTop = s_fIosActionMenuIphoneTop;
        s_fIosActionMenuPitch = s_fIosActionMenuIphonePitch;
        if (nScreenW > 1024) {
            IOS_ACTION_MENU_SHIFT_X = static_cast<float>(nScreenW - 1031);
            IOS_ACTION_MENU_GROW_X = 20.0f;
        }
    }
    const CSWGuiExtent sCloseExtent = { 720, 700, 230, 47 };
    for (int i = 0; i < 2; i++) {
        sExtent.x = static_cast<int>(IOS_ACTION_MENU_SHIFT_X + 614.0f);
        sExtent.y = 0;
        sExtent.w = static_cast<int>(IOS_ACTION_MENU_GROW_X + 410.0f);
        sExtent.h = 768;
        AndroidExtentShift(sExtent, -1, 1);
        CResRef sBg(i ? "ios_action_bg2" : "ios_action_bg");
        m_actionMenuHalves[i].m_border.m_params.SetFillImage(sBg, 0);
        m_actionMenuHalves[i].m_border.m_params.m_flags.nFillStyle = 2;
        m_actionMenuHalves[i].m_borderHilight.m_params.SetFillImage(sBg, 0);
        m_actionMenuHalves[i].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_actionMenuHalves[i].SetExtent(sExtent);
        m_actionMenuHalves[i].m_text.m_params.SetText(CExoString(""));
        m_actionMenuHalves[i].m_tail.bFocusOnHover = 0;
        m_actionMenuIos.AddControl(&m_actionMenuHalves[i]);
        SendControlToBack(&m_actionMenuHalves[i]);
        sExtent = sCloseExtent;
        int nCloseW;
        int nCloseH;
        if (IosIsIphone(nCloseW, nCloseH))
            IosAnchorRightBottom(sExtent, nCloseW, nCloseH);
        AndroidExtentShift(sExtent, -1, -1);
        CResRef sClose(i ? "ios_action_x2" : "ios_action_x");
        m_actionMenuCloseButtons[i].m_border.m_params.SetFillImage(sClose, 0);
        m_actionMenuCloseButtons[i].m_border.m_params.m_flags.nFillStyle = 2;
        m_actionMenuCloseButtons[i].m_borderHilight.m_params.SetFillImage(sClose, 0);
        m_actionMenuCloseButtons[i].m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_actionMenuCloseButtons[i].SetExtent(sExtent);
        m_actionMenuCloseButtons[i].m_text.m_params.SetText(CExoString(""));
        m_actionMenuCloseButtons[i].m_tail.bFocusOnHover = 0;
        m_actionMenuIos.AddControl(&m_actionMenuCloseButtons[i]);
        SendControlToBack(&m_actionMenuCloseButtons[i]);
        m_actionMenuCloseButtons[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HandleButtonActionX));
    }
    for (int64_t i = 0; i < 32; i++) {
        CSWGuiButton& slot = m_actionSlots[i];
        sExtent.x = static_cast<int>(IOS_ACTION_MENU_SHIFT_X + 650.0f);
        sExtent.y = static_cast<int>(s_fIosActionMenuPitch * static_cast<float>(i) + s_fIosActionMenuTop);
        sExtent.w = 360;
        sExtent.h = 56;
        AndroidExtentShift(sExtent, -1, 1);
        slot.m_text.m_params.SetAlignment(SWGUIALIGN_MIDDLE | SWGUIALIGN_RIGHT);
        slot.m_text.m_params.SetText(CExoString(""));
        slot.m_tail.m_nUserData = i;
        slot.SetExtent(sExtent);
        sExtent.x = static_cast<int>(IOS_ACTION_MENU_SHIFT_X + 950.0f);
        sExtent.y = static_cast<int>(s_fIosActionMenuPitch * static_cast<float>(i) + s_fIosActionMenuTop);
        sExtent.w = 56;
        sExtent.h = 56;
        AndroidExtentShift(sExtent, -1, 1);
        CSWGuiBorder* pBorder = &slot.m_border;
        pBorder->SetExtent(sExtent);
        CSWGuiBorder* pHilight = &slot.m_borderHilight;
        pHilight->SetExtent(sExtent);
        sExtent.x = static_cast<int>(IOS_ACTION_MENU_SHIFT_X + 640.0f);
        sExtent.y = static_cast<int>(s_fIosActionMenuPitch * static_cast<float>(i) + s_fIosActionMenuTop);
        sExtent.w = 300;
        sExtent.h = 56;
        AndroidExtentShift(sExtent, -1, 1);
        CSWGuiText* pText = &slot.m_text;
        pText->SetExtent(sExtent);
        m_actionMenuIos.AddControl(&slot);
        slot.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
            static_cast<void (CSWGuiMainInterface::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HandleButtonAction)));
    }
    RelayoutActionMenu(0);

    // The top-right corner plate.
#ifdef K2_UI_TOUCH  // pc / xbox: no corner plate, help or quicksave button (keys and menus do that)
    bool bIphone = IosIsIphone(nScreenW, nScreenH);
    sExtent.x = bIphone ? 786 : 800;
    sExtent.y = 0;
    sExtent.w = bIphone ? 238 : 225;
    sExtent.h = bIphone ? 98 : 99;
    if (IosIsIphone(nW, nH))
        IosAnchorRightTop(sExtent, nW, nH);
    AndroidExtentShift(sExtent, -1, 1);
    m_btnCornerPlate.SetExtent(sExtent);
    m_btnCornerPlate.m_text.m_params.SetText(CExoString(""));
    m_btnCornerPlate.m_border.m_params.SetFillImage(bIphone ? CResRef("ipho_upper_right") : CResRef("ios_upper_right"), 0);
    m_btnCornerPlate.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnCornerPlate.m_borderHilight.m_params.SetFillImage(bIphone ? CResRef("ipho_upper_right") : CResRef("ios_upper_right"), 0);
    m_btnCornerPlate.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    m_btnCornerPlate.m_tail.bFocusOnHover = 0;
    m_btnCornerPlate.SetEnabled(0);
    AddControl(&m_btnCornerPlate);
    SendControlToBack(&m_btnCornerPlate);
#endif

    // The gamepad menu covers the whole screen.
    const CSWGuiExtent sFullScreen = { 0, 0, 1024, 768 };
    sExtent = sFullScreen;
    if (IosIsIphone(nScreenW, nScreenH) && nScreenW > 1024) {
        sExtent.x = 0;
        sExtent.y = 0;
        sExtent.w = nScreenW;
        sExtent.h = 768;
    }
    sExtent.x = 0;
    sExtent.y = 0;
    sExtent.w = g_SDL_mode.w;
    sExtent.h = g_SDL_mode.h;
    m_gamepadMenu.SetExtent(sExtent);
    m_gamepadMenu.m_stateFlags |= 1;
    m_gamepadMenu.m_vColor.x = -2.0f;
    m_gamepadMenu.m_vColor.y = -2.0f;
    m_gamepadMenu.m_vColor.z = -2.0f;
    m_gamepadMenu.LayoutGamepadMenu(true);

    // Help and quicksave, under the corner plate.
#ifdef K2_UI_TOUCH
    int nTopY = bIphone ? 7 : 5;
    int nTopH = bIphone ? 70 : 46;
    sExtent.x = bIphone ? 861 : 872;
    sExtent.y = nTopY;
    sExtent.w = bIphone ? 90 : 76;
    sExtent.h = nTopH;
    if (IosIsIphone(nW, nH))
        IosAnchorRightTop(sExtent, nW, nH);
    AndroidExtentShift(sExtent, -1, 1);
    m_btnCornerHelp.SetExtent(sExtent);
    m_btnCornerHelp.m_text.m_params.SetText(CExoString(""));
    m_btnCornerHelp.m_border.m_params.SetFillImage(bIphone ? CResRef("ipho_help") : CResRef("ios_help"), 0);
    m_btnCornerHelp.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnCornerHelp.m_borderHilight.m_params.SetFillImage(bIphone ? CResRef("ipho_help2") : CResRef("ios_help2"), 0);
    m_btnCornerHelp.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    m_btnCornerHelp.m_tail.bFocusOnHover = 0;
    AddControl(&m_btnCornerHelp);
    m_btnCornerHelp.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::ShowHelp));
    sExtent.x = bIphone ? 792 : 807;
    sExtent.y = nTopY;
    sExtent.w = bIphone ? 90 : 69;
    sExtent.h = nTopH;
    if (IosIsIphone(nW, nH))
        IosAnchorRightTop(sExtent, nW, nH);
    AndroidExtentShift(sExtent, -1, 1);
    m_btnQuickSave.SetExtent(sExtent);
    m_btnQuickSave.m_text.m_params.SetText(CExoString(""));
    m_btnQuickSave.m_border.m_params.SetFillImage(bIphone ? CResRef("ipho_quicksave") : CResRef("ios_quicksave"), 0);
    m_btnQuickSave.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnQuickSave.m_borderHilight.m_params.SetFillImage(bIphone ? CResRef("ipho_quicksave2") : CResRef("ios_quicksave2"), 0);
    m_btnQuickSave.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    m_btnQuickSave.m_tail.bFocusOnHover = 0;
    AddControl(&m_btnQuickSave);
    m_btnQuickSave.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::Quicksave));
#endif

    // The help overlay puts the page image and five click-to-close buttons over the whole screen
    // (letterboxed on an iPhone, with the background stretched past the bars on a wide one).
    sExtent = sFullScreen;
    CSWGuiExtent sBackground = { 0, 0, 0, 0 };
    int bWide;
    int nHelpW;
    int nHelpH;
    if (IosIsIphone(nHelpW, nHelpH)) {
        if (nHelpW > 1023) {
            sBackground.x = -1;
            sBackground.y = 0;
            sBackground.w = nHelpW + 1;
            sBackground.h = nHelpH + 2;
            sExtent.x = 0;
            sExtent.y = (768 - nHelpH) / 2;
            sExtent.w = nHelpW;
            sExtent.h = nHelpH + 1;
            bWide = 1;
        } else {
            sExtent.x = (1024 - nHelpW) / 2;
            sExtent.y = (768 - nHelpH) / 2;
            sExtent.w = nHelpW;
            sExtent.h = nHelpH + 1;
            bWide = 0;
        }
    } else {
        bWide = 0;
    }
    AndroidExtentShift(sExtent, 1, 1);
    AndroidExtentShift(sBackground, 1, 1);
#ifdef K2_QOL_CONTROLLER
    // In the port, the overlay, its dimmed backdrop and its tap-to-close area cover the whole
    // screen, whatever its shape (IosIsIphone's 1136x640 page is 16:9 at the HUD scale).
    sExtent.x = 0;
    sExtent.y = 0;
    sExtent.w = g_SDL_mode.w;
    sExtent.h = g_SDL_mode.h;
    if (bWide) {
        sBackground.x = -1;
        sBackground.y = 0;
        sBackground.w = g_SDL_mode.w + 1;
        sBackground.h = g_SDL_mode.h + 2;
    }
#endif
    m_btnHelpClose.SetExtent(sExtent);
    m_btnHelpClose.m_text.m_params.SetText(CExoString(""));
    m_btnHelpClose.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpClose.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpClose.m_tail.bFocusOnHover = 0;
    m_btnHelpClose.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpPage.SetExtent(sExtent);
    m_btnHelpPage.m_text.m_params.SetText(CExoString(""));
    m_btnHelpPage.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpPage.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpPage.m_tail.bFocusOnHover = 0;
    m_btnHelpPage.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    GatherAndroidHelpPanelConstants();
    m_btnHelpTopLeft.SetExtent(sExtent);
    m_btnHelpTopLeft.m_text.m_params.SetText(CExoString(""));
    m_btnHelpTopLeft.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpTopLeft.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpTopLeft.m_tail.bFocusOnHover = 0;
    m_btnHelpTopLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpTopRight.SetExtent(sExtent);
    m_btnHelpTopRight.m_text.m_params.SetText(CExoString(""));
    m_btnHelpTopRight.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpTopRight.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpTopRight.m_tail.bFocusOnHover = 0;
    m_btnHelpTopRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpBottomLeft.SetExtent(sExtent);
    m_btnHelpBottomLeft.m_text.m_params.SetText(CExoString(""));
    m_btnHelpBottomLeft.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpBottomLeft.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpBottomLeft.m_tail.bFocusOnHover = 0;
    m_btnHelpBottomLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpBottomRight.SetExtent(sExtent);
    m_btnHelpBottomRight.m_text.m_params.SetText(CExoString(""));
    m_btnHelpBottomRight.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpBottomRight.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpBottomRight.m_tail.bFocusOnHover = 0;
    m_btnHelpBottomRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_helpPanel.SetExtent(sExtent);
    if (bWide) {
        sExtent.x = -1;
        m_helpPanel.m_pBackground->SetExtent(sBackground);
    }
    m_helpPanel.AddControl(&m_btnHelpPage);
    m_helpPanel.AddControl(&m_btnHelpTopLeft);
    m_helpPanel.AddControl(&m_btnHelpTopRight);
    m_helpPanel.AddControl(&m_btnHelpBottomLeft);
    m_helpPanel.AddControl(&m_btnHelpBottomRight);
    m_btnHelpHint.m_tail.bFocusOnHover = 0;
    m_btnHelpHint.m_text.m_params.SetText(CExoString(""));
    m_btnHelpHint.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpHint.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpHint.m_tail.bFocusOnHover = 0;
    m_helpPanel.AddControl(&m_btnHelpHint);
    m_btnHelpPage.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_helpPanel.AddControl(&m_btnHelpClose);
    m_helpPanel.m_vColor.x = -2.0f;
    m_helpPanel.m_vColor.y = -2.0f;
    m_helpPanel.m_vColor.z = -2.0f;
    m_helpPanel.m_stateFlags |= 1;
#ifdef K1_UI_XBOX
    // xbox: the caption and its plate keep the layout's rectangles (the ctor shrink-wraps both).
    K1ConsoleRestoreCaption(m_actionDescLabel, m_actionDescLabel2);
#endif
}
#else
CSWGuiMainInterface::CSWGuiMainInterface(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager),
      m_gamepadMenu(pGuiManager, this),
      m_actionMenuIos(pGuiManager, this),
      m_helpPanel(pGuiManager, this)
{
    CExoString sName;
    CSWGuiLabel sScratch;

    CSWGuiFloatyText::m_pMainInterface = this;
#ifdef K2_UI_XBOX

    const int nSavedGUIWidth = g_nGUIWidth;
    const int nSavedGUIHeight = g_nGUIHeight;
    g_nGUIWidth = 640;
    g_nGUIHeight = 480;
#endif
    m_scaleX = static_cast<float>(pGuiManager->m_nScreenWidth) / static_cast<float>(static_cast<unsigned int>(g_nGUIWidth));
    m_scaleY = static_cast<float>(pGuiManager->m_nScreenHeight) / static_cast<float>(static_cast<unsigned int>(g_nGUIHeight));
#ifdef K2_UI_XBOX
    g_scaleMipc = 3;
#else
    g_scaleMipc = 1;
#endif
#ifdef K2_UI_XBOX
    StartLoadFromLayout(CResRef("maininterface_x"), 1, 1, 0);
#elif defined(K2_UI_PC)
    StartLoadFromLayout(CResRef("mipc28x6_p"), 1, 1, 0);
#else
    if (!ASLIsDeviceWidescreen())
        StartLoadFromLayout(CResRef("mipc28x6_p"), 1, 1, 0);
    else
        StartLoadFromLayout(CResRef("miipho16x9"), 1, 1, 0);
#endif
#if !KOTOR1  // K2-only member: K1's CSWGuiPanel lacks it
    m_bIsMainInterface = 1;
#endif
    m_stateFlags |= 1;
#ifndef K2_UI_XBOX
    g_scaleMipc = 2;
#endif

    for (int i = 0; i < 6; i++) {
        m_personalActionWidgets[i].Initialize(this, i, 0, 1);
        m_personalActionWidgets[i].m_bSelected = 0;
#ifndef K2_UI_XBOX
        static const CSWGuiColor sColor(0.4f, 0.4f, 0.4f);
        m_personalActionWidgets[i].m_button.m_border.m_params.m_color = sColor;
#endif
    }
#ifdef K2_UI_XBOX
    g_scaleMipc = 3;
#else
    g_scaleMipc = 1;
#endif
#pragma clang loop unroll(disable)
    for (int i = -3; i != 0; i++)
        m_aCharacters[i + 3].Initialize(this, i + 4, 1);
    g_scaleMipc = 4;
    m_aCharacters[3].Initialize(this, 4, 1);

    sName.Format(const_cast<char*>("LBL_QUEUE%d"), 0);  // idiom-exempt: char* literal, Format's parameter type
    InitControl(m_actionQueueIcons[0], sName, 1, 1, 0, 0, 0);
    sName.Format(const_cast<char*>("LBL_QUEUE%d"), 1);  // idiom-exempt: char* literal, Format's parameter type
    InitControl(m_actionQueueIcons[1], sName, 1, 1, 0, 0, 0);
    sName.Format(const_cast<char*>("LBL_QUEUE%d"), 2);  // idiom-exempt: char* literal, Format's parameter type
    InitControl(m_actionQueueIcons[2], sName, 1, 1, 0, 0, 0);
    sName.Format(const_cast<char*>("LBL_QUEUE%d"), 3);  // idiom-exempt: char* literal, Format's parameter type
    InitControl(m_actionQueueIcons[3], sName, 1, 1, 0, 0, 0);
#ifdef K2_UI_XBOX
    g_scaleMipc = 3;
#else
    g_scaleMipc = 1;
#endif
    for (int i = 0; i < 13; i++)
        m_statusIcons[i].Initialize(this, static_cast<unsigned char>(i), 1);
    m_targetActionMenu.Initialize(this, 0);
#ifdef K2_UI_XBOX
    ConsoleRebind(this, m_lblMoulding1, "LBL_MOULDING1", "LBL_ACTIONDESCBG2", 1);
#else
    InitControl(m_lblMoulding1, CExoString("LBL_MOULDING1"), 1, 1, 1, 0, 0);
#endif
    InitControl(m_lblMoulding3, CExoString("LBL_MOULDING3"), 1, 1, 1, 0, 0);
    g_scaleMipc = 4;
#ifdef K2_UI_XBOX
    ConsoleRebind(this, m_lblCombatBg2, "LBL_COMBATBG2", "LBL_BG1", 0);
    ConsoleRebind(this, m_lblCombatBg3, "LBL_COMBATBG3", "LBL_BG2", 0);
    // LBL_BG2 badges the queue with the Xbox's Black button (uibut_black), which no modern pad has
    // and the Switch build does not draw: a zero extent keeps it hidden whatever its visible flag.
    {
        CSWGuiExtent kHidden(0, 0, 0, 0);
        m_lblCombatBg3.SetExtent(kHidden);
    }
    InitControl(m_actionQueueHilight, CExoString("LBL_HIGHLIGHT"), 1, 1, 0, 0, 0);
#else
    InitControl(m_lblCombatBg2, CExoString("LBL_COMBATBG2"), 1, 1, 0, 0, 0);
    InitControl(m_lblCombatBg3, CExoString("LBL_COMBATBG3"), 1, 1, 0, 0, 0);
#endif
    InitControl(m_btnClearOne, CExoString("BTN_CLEARONE"), 1, 1, 0, 0, 0);
    InitControl(m_btnClearOne2, CExoString("BTN_CLEARONE2"), 1, 1, 0, 0, 0);
    InitControl(m_btnClearAll, CExoString("BTN_CLEARALL"), 1, 1, 0, 0, 0);
#ifdef K2_UI_XBOX
    g_scaleMipc = 3;
#elif defined(K2_UI_PC)
    g_scaleMipc = 1;
#endif
    InitControl(m_actionDescLabel, CExoString("LBL_ACTIONDESC"), 1, 1, 1, 0, 0);
    InitControl(m_actionDescLabel2, CExoString("LBL_ACTIONDESCBG"), 1, 1, 1, 0, 0);
#ifdef K2_UI_XBOX
    // console HUD (K2_UI_XBOX): the plate's fill style 0 tiles uibit_brdr_main3 at its native size,
    // one copy at 640x480 but two side by side once the HUD is scaled up; stretch it once instead.
    m_actionDescLabel2.m_border.m_params.m_flags.nFillStyle = 2;
    if (m_lblMoulding1.m_extent.w > 0 && m_actionDescLabel2.m_extent.w > 0) {
        CSWGuiExtent sCap = m_lblMoulding1.m_extent;
        const int nCapBottom = sCap.y + sCap.h;
        const int nPlateBottom = m_actionDescLabel2.m_extent.y + m_actionDescLabel2.m_extent.h;
        if (nPlateBottom != sCap.y && nPlateBottom < nCapBottom) {
            sCap.y = nPlateBottom;
            sCap.h = nCapBottom - nPlateBottom;
            m_lblMoulding1.SetExtent(sCap);
        }
    }
    if (m_lblCombatBg2.m_extent.w > 0 && m_actionDescLabel2.m_extent.w > 0) {
        const int nPlateRight = m_actionDescLabel2.m_extent.x + m_actionDescLabel2.m_extent.w;
        const int nOverlap = (294 - 292) * m_lblCombatBg2.m_extent.w / 128;
        const int nShift = (nPlateRight - nOverlap) - m_lblCombatBg2.m_extent.x;
        if (nShift != 0) {
            CSWGuiLabel* apQueue[5] = { &m_actionQueueIcons[0], &m_actionQueueIcons[1],
                                        &m_actionQueueIcons[2], &m_actionQueueIcons[3],
                                        &m_lblCombatBg2 };
            for (int i = 0; i < 5; i++) {
                if (apQueue[i]->m_extent.w <= 0)
                    continue;
                CSWGuiExtent sExtent = apQueue[i]->m_extent;
                sExtent.x += nShift;
                apQueue[i]->SetExtent(sExtent);
            }
            if (m_actionQueueHilight.m_extent.w > 0) {
                CSWGuiExtent sRing = m_actionQueueHilight.m_extent;
                sRing.x += nShift;
                m_actionQueueHilight.SetExtent(sRing);
            }
        }
    }
#else
    g_scaleMipc = 1;
#endif
    InitControl(m_miniMap, CExoString("LBL_MAPBORDER"), 1, 1, 1, 0, 0);
    InitControl(m_lblArrow, CExoString("LBL_ARROW"), 0, 1, 1, 0, 0);
    InitControl(m_miniMapImage, CExoString("LBL_MAP"), 0, 1, 1, 0, 0);
    InitControl(sScratch, CExoString("LBL_MAPVIEW"), 0, 1, 1, 0, 0);
    InitControl(m_altMenuButton, CExoString("BTN_MINIMAP"), 1, 1, 1, 0, 0);
#ifdef K2_UI_XBOX
    if (m_altMenuButton.m_extent.w > 0 && sScratch.m_extent.w > 0)
#endif
#ifndef K2_UI_PC
    {
        CSWGuiExtent sExtent = m_miniMap.m_extent;
        sExtent.w = sScratch.m_extent.w;
        sExtent.x = m_altMenuButton.m_extent.x;
        m_miniMap.SetExtent(sExtent);
    }
#endif
#ifdef K2_UI_XBOX
    if (m_altMenuButton.m_extent.w <= 0 || m_altMenuButton.m_extent.h <= 0) {
        m_altMenuButton.SetExtent(m_miniMap.m_extent);
        bool bListed = false;
        for (int i = 0; i < m_controls.m_count; i++)
            if (m_controls.m_pArray[i] == &m_altMenuButton)
                bListed = true;
        if (!bListed)
            AddControl(&m_altMenuButton);
    }
#endif
    g_scaleMipc = 4;
    InitControl(m_lblCombatModeMsg, CExoString("LBL_CMBTMODEMSG"), 1, 1, 0, 0, 0);
    m_lblCombatModeMsg2.CSWGuiLabel::Initialize(m_lblCombatModeMsg.m_extent, &m_lblCombatModeMsg, 1.0f);
    InitControl(m_lblCombatMsgBg, CExoString("LBL_CMBTMSGBG"), 1, 1, 0, 0, 0);
    g_scaleMipc = 3;
    m_lblCombatModeMsg.m_tail.bVisible = 0;
    m_lblCombatMsgBg.m_tail.bVisible = 0;
    m_lblCombatModeMsg2.m_tail.bVisible = 0;
    m_lblCombatModeMsg2.m_text.m_params.SetText(g_pTlkTable->GetSimpleString(STRREF_HOTKEY_HINT));
    InitControl(m_btnEquip, CExoString("BTN_EQU"), 1, 1, 1, 0, 0);
    InitControl(m_btnInventory, CExoString("BTN_INV"), 1, 1, 1, 0, 0);
    InitControl(m_btnCharacter, CExoString("BTN_CHAR"), 1, 1, 1, 0, 0);
    InitControl(m_btnAbilities, CExoString("BTN_ABI"), 1, 1, 1, 0, 0);
    InitControl(m_btnMessages, CExoString("BTN_MSG"), 1, 1, 1, 0, 0);
    InitControl(m_btnJournal, CExoString("BTN_JOU"), 1, 1, 1, 0, 0);
    InitControl(m_btnMap, CExoString("BTN_MAP"), 1, 1, 1, 0, 0);
    InitControl(m_btnOptions, CExoString("BTN_OPT"), 1, 1, 1, 0, 0);
    InitControl(m_lblMenuBg, CExoString("LBL_MENUBG"), 1, 1, 1, 0, 0);
    InitControl(m_lblArrowMargin, CExoString("LBL_ARROW_MARGIN"), 1, 1, 1, 0, 0);
#ifdef K2_UI_XBOX
    if (m_lblArrowMargin.m_extent.w <= 0 || m_lblArrowMargin.m_extent.h <= 0) {
        CSWGuiExtent sMargin;
        sMargin.x = 0;
        sMargin.y = 0;
        sMargin.w = m_pGuiManager->m_nScreenWidth;
        sMargin.h = m_pGuiManager->m_nScreenHeight;
        m_lblArrowMargin.SetExtent(sMargin);
    }
#endif
#ifdef K2_UI_PC
    // For the PC HUD (K2_UI_PC), the macOS constructor binds the four bottom-right toggles with no
    // HasTouchScreen test and has none of the touch extras (TGuiLabel/TGuiLabel1, BTN_FLOURISH,
    // BTN_CAM, BTN_HELP, BTN_SAVE); those stay unbound, zero-extent and unlisted.
    InitControl(m_tbPause, CExoString("TB_PAUSE"), 1, 1, 1, 0, 0);
    InitControl(m_tbSolo, CExoString("TB_SOLO"), 1, 1, 1, 0, 0);
    InitControl(m_tbStealth, CExoString("TB_STEALTH"), 1, 1, 1, 0, 0);
    InitControl(m_btnSwapWeapons, CExoString("BTN_SWAPWEAPONS"), 1, 1, 1, 0, 0);
#else
    if (HasTouchScreen()) {
        InitControl(m_tbPause, CExoString("TB_PAUSE"), 1, 1, 1, 0, 0);
        InitControl(m_tbSolo, CExoString("TB_SOLO"), 1, 1, 1, 0, 0);
        InitControl(m_tbStealth, CExoString("TB_STEALTH"), 1, 1, 1, 0, 0);
        InitControl(m_btnSwapWeapons, CExoString("BTN_SWAPWEAPONS"), 1, 1, 1, 0, 0);
        InitControl(m_lblControlBg, CExoString("TGuiLabel"), 1, 1, 1, 0, 0);
        InitControl(m_btnFlourish, CExoString("BTN_FLOURISH"), 1, 1, 1, 0, 0);
    }
    InitControl(m_lblMenuKeyBg, CExoString("TGuiLabel1"), 1, 1, 1, 0, 0);
    InitControl(m_btnCamera, CExoString("BTN_CAM"), 1, 1, 1, 0, 0);
    InitControl(m_btnHelp, CExoString("BTN_HELP"), 1, 1, 1, 0, 0);
    InitControl(m_btnSave, CExoString("BTN_SAVE"), 1, 1, 1, 0, 0);
#endif
    m_btnCamera.m_tail.bFocusOnHover = 0;
    m_btnHelp.m_tail.bFocusOnHover = 0;
    m_btnSave.m_tail.bFocusOnHover = 0;
    m_btnCamera.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnFirstPersonButtonPressed));
    m_btnHelp.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::ShowHelp));
    m_btnSave.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::Quicksave));
    StopLoadFromLayout();
    g_scaleMipc = 0;
    float fTextScale = GetResolutionTextScalar();
#ifndef K2_UI_PC
    {
        CSWGuiExtent sExtent;
        sExtent.x = m_miniMap.m_extent.x + m_miniMap.m_extent.w + 2;
        sExtent.y = 2;
        sExtent.w = (m_lblMenuKeyBg.m_extent.x - sExtent.x - 2) / 2 - 4;
        sExtent.h = static_cast<int>(fTextScale * 70.0f);
        m_lblCombatModeMsg.CSWGuiLabel::SetExtent(sExtent);
    }
#else
    (void)fTextScale;
#endif

#ifdef K2_UI_PC
    // The reticle's margin ends where the bottom HUD row starts (TB_PAUSE), as every other PC
    // layout's does: mipc28x6_p keeps mipc8x6_p's rectangle, which stops 100 lines higher.
    {
        CSWGuiExtent sMargin = m_lblArrowMargin.m_extent;
        if (m_tbPause.m_extent.y > sMargin.y + sMargin.h)
            sMargin.h = m_tbPause.m_extent.y - sMargin.y;
        m_lblArrowMargin.SetExtent(sMargin);
    }
#endif
    m_targetActionMenu.m_aViewport[4] = m_lblArrowMargin.m_extent.x;
    m_targetActionMenu.m_aViewport[5] = m_lblMenuBg.m_extent.y + m_lblMenuBg.m_extent.h;
    m_targetActionMenu.m_aViewport[6] = m_pGuiManager->m_nScreenWidth - m_targetActionMenu.m_aViewport[4] * 2;
    m_targetActionMenu.m_aViewport[7] = m_tbPause.m_extent.y - m_targetActionMenu.m_aViewport[5];
#ifdef K2_UI_XBOX
    // console HUD (K2_UI_XBOX): the top and bottom of that clamp box come from LBL_MENUBG and TB_PAUSE,
    // two more tags no `_x` layout has, so the console HUD would leave it zero-height. Same
    // answer as the margin above.
    if (m_targetActionMenu.m_aViewport[7] <= 0) {
        m_targetActionMenu.m_aViewport[5] = 0;
        m_targetActionMenu.m_aViewport[7] = m_pGuiManager->m_nScreenHeight;
    }
#endif
    AddControl(&m_lblCombatModeMsg2);
    if (m_pGuiManager->m_nScreenWidth >= 1024) {
        CSWGuiExtent sExtent;
        sExtent.x = m_extent.x;
        sExtent.y = m_extent.y;
        sExtent.w = g_nScreenWidth;
        sExtent.h = g_nScreenHeight;
        SetExtent(sExtent);
    }
    int nSafeMarginA;
    int nSafeMarginB;
    m_pGuiManager->GetSafeMargin(nSafeMarginA, nSafeMarginB);  // both margins are unread here
    m_interfaceOffset = 0;
    m_nInterfaceFlags |= 2;
    m_fCombatMessageTimer = -1.0f;
    m_nMapWidth = m_miniMapImage.m_extent.w;
    m_nMapHeight = m_miniMapImage.m_extent.h;
    m_mapViewExtent = sScratch.m_extent;
    CSWGuiExtent sArrow = m_lblArrow.m_extent;
    sArrow.x = (m_mapViewExtent.w - sArrow.w) / 2;
    sArrow.y = (m_mapViewExtent.h - sArrow.h) / 2;
    m_lblArrow.SetExtent(sArrow);
    m_lblTargetIndicator.m_tail.bIgnoreMouse = 1;
    m_lblTargetIndicator.m_border.m_params.m_flags.nFillStyle = 2;
    AddControl(&m_lblTargetIndicator);
    CSWGuiExtent sReticle;
    sReticle.x = 0;
    sReticle.y = 0;
    sReticle.w = 16;
    sReticle.h = 16;
    m_reticleLabel.SetExtent(sReticle);
    m_reticleLabel.m_tail.bIgnoreMouse = 1;
    m_reticleLabel.m_border.m_params.m_flags.nFillStyle = 2;
    AddControl(&m_reticleLabel);
    m_mouseOverTargetId = OBJECT_INVALID;
    m_targetId = OBJECT_INVALID;
    m_lastPlayerId = OBJECT_INVALID;
    m_fReticlePulseTimer = -1.0f;
    m_fChallengeRating = -1.0f;
    for (int i = 0; i < 7; i++)
        m_personalActionIds[i] = -1;
    m_fRotationTimer = 0;
    ShowActionQueue(0);
    m_combatMessage = STRREF_COMBAT_MODE_ENGAGED;
    m_bToolTipMessageShown = 0;

    m_btnEquip.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnInventory.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnCharacter.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnAbilities.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnJournal.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnMessages.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnMap.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnOptions.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_altMenuButton.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnMenuButtonPressed));
    m_btnEquip.m_tail.bFocusOnHover = 0;
    m_btnInventory.m_tail.bFocusOnHover = 0;
    m_btnCharacter.m_tail.bFocusOnHover = 0;
    m_btnAbilities.m_tail.bFocusOnHover = 0;
    m_btnJournal.m_tail.bFocusOnHover = 0;
    m_btnMessages.m_tail.bFocusOnHover = 0;
    m_btnMap.m_tail.bFocusOnHover = 0;
    m_btnOptions.m_tail.bFocusOnHover = 0;
    m_altMenuButton.m_tail.bFocusOnHover = 0;
    m_btnClearOne.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnClearOneButtonPressed));
    m_btnClearOne2.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnClearOneButtonPressed));
    m_btnClearAll.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnClearAllButtonPressed));
    m_btnClearOne2.m_tail.bFocusOnHover = 0;
    m_btnClearOne.m_tail.bFocusOnHover = 0;
    m_btnClearAll.m_tail.bFocusOnHover = 0;
    m_lblArrowMargin.m_tail.bVisible = 0;
    m_lblArrowMargin.m_tail.bFocusOnHover = 0;
    m_tbPause.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnPauseButtonPressed));
    m_tbSolo.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnSoloButtonPressed));
    m_tbStealth.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnStealthButtonPressed));
    m_btnFlourish.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnFlourishButtonPressed));
    m_btnFlourish.m_tail.bFocusOnHover = 0;
    m_tbPause.m_tail.bFocusOnHover = 0;
    m_tbSolo.m_tail.bFocusOnHover = 0;
    m_tbStealth.m_tail.bFocusOnHover = 0;
#if !KOTOR1  // K1 has no weapon sets
    m_btnSwapWeapons.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnSwapWeaponsPressed));
#endif
    m_btnSwapWeapons.m_tail.bFocusOnHover = 0;
    m_pFogOfWarImage = NewCAurGUIImage(const_cast<char*>("blackdot"));
    CSWGuiText* pDescText = &m_actionDescLabel.m_text;
    m_nFocusedAction = 0;
    m_lblCombatModeMsg.m_text.m_params.SetText(CExoString(""));
    m_nActionDescBottom = m_actionDescLabel.m_extent.y + m_actionDescLabel.m_extent.h;
    {
        CExoString sCaption("");
        m_actionDescLabel.m_text.m_params.SetText(sCaption);
        CSWGuiExtent sExtent;
        sExtent.x = m_actionDescLabel.m_extent.x;
        sExtent.w = m_actionDescLabel.m_extent.w;
        sExtent.h = pDescText->GetIdealHeight();
        sExtent.y = m_nActionDescBottom - sExtent.h;
        m_actionDescLabel.SetExtent(sExtent);
#ifdef K2_UI_XBOX
        SizeActionDescPlate(m_actionDescLabel2, sExtent.y);   // the plate, not the caption
#else
        m_actionDescLabel2.SetExtent(sExtent);
#endif
    }
    UpdateActionDescription();
    m_btnEquip.m_nToolTipStrRef = STRREF_TIP_EQUIP;   m_btnEquip.m_nBoundInputEventId = INPUTEVENT_PANEL_EQUIP;
    m_btnInventory.m_nToolTipStrRef = STRREF_TIP_INVENTORY;   m_btnInventory.m_nBoundInputEventId = INPUTEVENT_PANEL_INVENTORY;
    m_btnCharacter.m_nToolTipStrRef = STRREF_TIP_CHARACTER;   m_btnCharacter.m_nBoundInputEventId = INPUTEVENT_PANEL_CHARACTER;
    m_btnMap.m_nToolTipStrRef = STRREF_TIP_MAP;   m_btnMap.m_nBoundInputEventId = INPUTEVENT_PANEL_MAP;
    m_altMenuButton.m_nToolTipStrRef = STRREF_TIP_MAP;    m_altMenuButton.m_nBoundInputEventId = INPUTEVENT_PANEL_MAP;
    m_btnAbilities.m_nToolTipStrRef = STRREF_TIP_ABILITIES;   m_btnAbilities.m_nBoundInputEventId = INPUTEVENT_PANEL_ABILITIES;
    m_btnJournal.m_nToolTipStrRef = STRREF_TIP_JOURNAL;   m_btnJournal.m_nBoundInputEventId = INPUTEVENT_PANEL_JOURNAL;
    m_btnOptions.m_nToolTipStrRef = STRREF_TIP_OPTIONS;    m_btnOptions.m_nBoundInputEventId = INPUTEVENT_PANEL_OPTIONS;
    m_btnMessages.m_nToolTipStrRef = STRREF_TIP_PARTY_SELECTION;  m_btnMessages.m_nBoundInputEventId = INPUTEVENT_PANEL_MESSAGES;
    m_lblArrowMargin.m_nToolTipStrRef = STRREF_TIP_PAUSE;  m_lblArrowMargin.m_nBoundInputEventId = INPUTEVENT_PAUSE_ALT;
    m_tbSolo.m_nToolTipStrRef = STRREF_TIP_SOLO_MODE; m_tbSolo.m_nBoundInputEventId = INPUTEVENT_PARTY_ACTIVE;
    m_tbStealth.m_nToolTipStrRef = STRREF_SKILL_STEALTH;   m_tbStealth.m_nBoundInputEventId = INPUTEVENT_STEALTH;
    m_btnClearAll.m_nBoundInputEventId = INPUTEVENT_CANCEL_COMBAT;
    m_btnClearOne.m_nToolTipStrRef = STRREF_TIP_CLEAR_COMBAT_ACTION;     m_btnClearOne2.m_nToolTipStrRef = STRREF_TIP_CLEAR_COMBAT_ACTION;
    m_btnClearOne.m_nBoundInputEventId = INPUTEVENT_CLEAR_ONE_ACTION;   m_btnClearOne2.m_nBoundInputEventId = INPUTEVENT_CLEAR_ONE_ACTION;
    m_btnClearAll.m_nToolTipStrRef = STRREF_TIP_CANCEL_COMBAT;
    m_btnSwapWeapons.m_nToolTipStrRef = STRREF_TIP_SWITCH_WEAPONS;    m_btnSwapWeapons.m_nBoundInputEventId = INPUTEVENT_SWITCH_WEAPONS;

    CSWGuiManager* pMgr = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    {
        int nW = pMgr->m_nScreenWidth;
        CSWGuiManager* pMgrB = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        int nH = pMgrB->m_nScreenHeight;
        CSWGuiExtent sExtent;
        sExtent.x = 0;
        sExtent.w = nW;
        sExtent.h = nH;
        m_actionMenuIos.SetExtent(sExtent);
    }
    m_actionMenuIos.m_vColor.x = -1.0f;
    m_actionMenuIos.m_vColor.y = -1.0f;
    m_actionMenuIos.m_vColor.z = -1.0f;
    m_actionMenuIos.m_stateFlags |= 1;

    pMgr = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    int nScreenW = pMgr->m_nScreenWidth;
    pMgr = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    int nScreenH = pMgr->m_nScreenHeight;
    int nEdgeL = 0;
    int nEdgeR = 0;
    if (g_bAddEdgeOffset) {
        pMgr = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        if (pMgr) {
            pMgr = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
            nEdgeR = pMgr->m_nUIEdgeInset;
            pMgr = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
            nEdgeL = -pMgr->m_nUIEdgeInset;
        }
    }
    int nThirdW = nScreenW / 3;
    {
        CSWGuiExtent sExtentL;
        CSWGuiExtent sExtentR;
        sExtentR.x = nEdgeR;
        sExtentR.y = 0;
        sExtentR.w = nThirdW;
        sExtentR.h = nScreenH;
        sExtentL.x = nEdgeL;
        sExtentL.y = 0;
        sExtentL.w = nThirdW;
        sExtentL.h = nScreenH;

    }
    const CResRef sHalfBg("ios_action_bg");
    m_actionMenuHalves[0].m_border.m_params.SetFillImage(sHalfBg, 0);
    m_actionMenuHalves[0].m_borderHilight.m_params.SetFillImage(sHalfBg, 0);
    m_actionMenuHalves[0].m_borderHilight.m_params.m_flags.nFillStyle = 2;
    m_actionMenuHalves[0].m_border.m_params.m_flags.nFillStyle = 2;
    {
        CSWGuiControl* pControl = &m_actionMenuHalves[0];
        CSWGuiExtent sExtentL;
        sExtentL.x = nEdgeR;
        sExtentL.y = 0;
        sExtentL.w = nThirdW;
        sExtentL.h = nScreenH;
        pControl->SetExtent(sExtentL);
    }
    m_actionMenuHalves[0].m_text.m_params.SetText(CExoString(""));
    m_actionMenuHalves[0].m_tail.bFocusOnHover = 0;
    m_actionMenuIos.AddControl(&m_actionMenuHalves[0]);
    SendControlToBack(&m_actionMenuHalves[0]);
    const CResRef sHalfBg2("ios_action_bg2");
    m_actionMenuHalves[1].m_border.m_params.SetFillImage(sHalfBg2, 0);
    m_actionMenuHalves[1].m_borderHilight.m_params.SetFillImage(sHalfBg2, 0);
    m_actionMenuHalves[1].m_borderHilight.m_params.m_flags.nFillStyle = 2;
    m_actionMenuHalves[1].m_border.m_params.m_flags.nFillStyle = 2;
    {
        CSWGuiControl* pControl = &m_actionMenuHalves[1];
        CSWGuiExtent sExtentR;
        sExtentR.x = nEdgeL;
        sExtentR.y = 0;
        sExtentR.w = nThirdW;
        sExtentR.h = nScreenH;
        pControl->SetExtent(sExtentR);
    }
    m_actionMenuHalves[1].m_text.m_params.SetText(CExoString(""));
    m_actionMenuHalves[1].m_tail.bFocusOnHover = 0;
    m_actionMenuIos.AddControl(&m_actionMenuHalves[1]);
    SendControlToBack(&m_actionMenuHalves[1]);
    for (int i = 0; i < 32; i++) {
        m_actionSlots[i].m_text.m_params.SetText(CExoString(""));
        m_actionSlots[i].m_tail.m_nUserData = i;
        m_actionMenuIos.AddControl(&m_actionSlots[i]);
        m_actionSlots[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
            static_cast<void (CSWGuiMainInterface::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HandleButtonAction)));
    }
    {
        CSWGuiExtent sExtent;
        sExtent.x = 0;
        sExtent.w = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
        sExtent.h = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight;
        m_gamepadMenu.SetExtent(sExtent);
    }
    m_gamepadMenu.m_stateFlags |= 1;
    m_gamepadMenu.m_vColor.x = -2.0f;
    m_gamepadMenu.m_vColor.y = -2.0f;
    m_gamepadMenu.m_vColor.z = -2.0f;
    m_gamepadMenu.LayoutGamepadMenu(true);
    CSWGuiExtent sFullScreen;
    sFullScreen.x = 0;
    sFullScreen.w = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
    sFullScreen.h = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight;
    m_btnHelpClose.SetExtent(sFullScreen);
    m_btnHelpClose.m_text.m_params.SetText(CExoString(""));
    m_btnHelpClose.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpClose.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpClose.m_tail.bFocusOnHover = 0;
    m_btnHelpClose.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpClose.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpClose.AddEvent(INPUTEVENT_GUI_B, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpPage.SetExtent(sFullScreen);
    m_btnHelpPage.m_text.m_params.SetText(CExoString(""));
    m_btnHelpPage.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpPage.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpPage.m_tail.bFocusOnHover = 0;
    m_btnHelpPage.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpPage.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpPage.AddEvent(INPUTEVENT_GUI_B, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_helpPanel.SetExtent(sFullScreen);
    m_helpPanel.AddControl(&m_btnHelpPage);
    m_btnHelpHint.m_tail.bFocusOnHover = 0;
    m_btnHelpHint.m_text.m_params.SetText(CExoString(""));
    m_btnHelpHint.m_border.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpHint.m_borderHilight.m_params.SetFillImage(CResRef(""), 0);
    m_btnHelpHint.m_tail.bFocusOnHover = 0;
    m_helpPanel.AddControl(&m_btnHelpHint);
    m_btnHelpPage.AddEvent(INPUTEVENT_GUI_BACK, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpPage.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_btnHelpPage.AddEvent(INPUTEVENT_GUI_B, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::HideHelp));
    m_helpPanel.AddControl(&m_btnHelpClose);
    m_helpPanel.m_vColor.x = -2.0f;
    m_helpPanel.m_vColor.y = -2.0f;
    m_helpPanel.m_vColor.z = -2.0f;
    m_btnGamepadHelp.m_tail.bVisible = 0;
    m_btnGamepadHelp.m_tail.bFocusOnHover = 0;
    m_helpPanel.m_stateFlags |= 1;
    m_btnGamepadHelp.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnGamepadHelp.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    AddControl(&m_btnGamepadHelp);
    m_bPartyItemUsePending = 0;
    m_nPartyItemUseTarget = 0;
    m_oidPartyItemUse = OBJECT_INVALID;
    m_feedbackTimer = 0.0f;
    m_feedbackStrRef = 0;
    CSWGuiExtent sCorner;
    {
        float fX = 0.0f;
        float fY = 0.0f;
        CSWGuiManager* pMgr2 = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
        if (pMgr2) {
            pMgr2 = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
            fX = static_cast<float>(pMgr2->m_nScreenWidth) / static_cast<float>(static_cast<unsigned int>(g_nGUIWidth));
            pMgr2 = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
            fY = static_cast<float>(pMgr2->m_nScreenHeight) / static_cast<float>(static_cast<unsigned int>(g_nGUIHeight));
        }
        int nX = 0;
        if (static_cast<int>(fX * 4.0f) + static_cast<int>(g_bAddEdgeOffset) != 0) {
            nX = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nUIEdgeInset;
        }
        sCorner.x = nX;
        sCorner.y = static_cast<int>(fY * 398.0f);
        sCorner.w = static_cast<int>(fX * 252.0f);
        sCorner.h = static_cast<int>(fY * 96.0f);
        m_btnFeedback.m_border.m_params.SetCornerImage(CResRef("uibit_brdr_16bct"), 0);
        m_btnFeedback.m_border.m_params.SetEdgeImage(CResRef("uibit_brdr_16bet"), 0);
        m_btnFeedback.m_border.m_params.SetFillImage(CResRef("uibit_fill_2bt"), 0);
        m_btnFeedback.m_border.m_params.m_nBorderDim = 16;
        m_btnFeedback.m_border.m_params.m_color.r = CGuiInGame::COLOR_NEW_GREEN.x;
        m_btnFeedback.m_border.m_params.m_color.g = CGuiInGame::COLOR_NEW_GREEN.y;
        m_btnFeedback.m_border.m_params.m_color.b = CGuiInGame::COLOR_NEW_GREEN.z;
        m_btnFeedback.m_border.m_params.m_nInnerOffset = 10;
#if !KOTOR1  // K2-only: K1's CSWGuiBorderParams lacks this field
        m_btnFeedback.m_border.m_params.m_nInnerOffsetY = 12;
#endif
        m_btnFeedback.m_borderHilight.m_params.SetCornerImage(CResRef("uibit_brdr_16bct"), 0);
        m_btnFeedback.m_borderHilight.m_params.SetEdgeImage(CResRef("uibit_brdr_16bet"), 0);
        m_btnFeedback.m_borderHilight.m_params.SetFillImage(CResRef("uibit_fill_2bt"), 0);
        m_btnFeedback.m_borderHilight.m_params.m_nBorderDim = 16;
        m_btnFeedback.m_borderHilight.m_params.m_color.r = CGuiInGame::COLOR_NEW_GREEN.x;
        m_btnFeedback.m_borderHilight.m_params.m_color.g = CGuiInGame::COLOR_NEW_GREEN.y;
        m_btnFeedback.m_borderHilight.m_params.m_color.b = CGuiInGame::COLOR_NEW_GREEN.z;
        m_btnFeedback.m_borderHilight.m_params.m_nInnerOffset = 10;
#if !KOTOR1  // K2-only: K1's CSWGuiBorderParams lacks this field
        m_btnFeedback.m_borderHilight.m_params.m_nInnerOffsetY = 12;
#endif
    }
#if !KOTOR1  // K1 has no party item-use selection to cancel
    m_btnFeedback.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnCancelPartyItemSelect));
#endif
    m_btnFeedback.m_tail.bFocusOnHover = 0;
    m_btnFeedback.SetExtent(sCorner);
    AddControl(&m_btnFeedback);
    m_bActionMenuLatch = 0;
#ifdef K2_UI_XBOX

    // console HUD (K2_UI_XBOX): close the 640x480 window opened at the top. Every panel built after this
    // point is a mobile `_p` layout and wants the desktop 800/600 back.
    g_nGUIWidth = nSavedGUIWidth;
    g_nGUIHeight = nSavedGUIHeight;
#endif
}
#endif

#if KOTOR1
// K1: no joystick gate, no gamepad flag and no one-tick delay -- the page alone is parked for Update.
void QueueHelpGui(int nPage) {
    g_nQueuedHelpPage = nPage;
}
#else
void QueueHelpGui(int nPage, bool bGamepad) {
    if (HasJoystick()) {
        g_nQueuedHelpPage = nPage;
        g_bQueuedHelpGamepad = bGamepad;
        g_nQueuedHelpDelay = 1;
    }
}
#endif

bool IosAnyMainPanelUp() {
    CSWGuiMainInterface* pMainInterface = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
    return g_pGuiMan->PanelExists(&pMainInterface->m_helpPanel) ||
           g_pGuiMan->PanelExists(&pMainInterface->m_gamepadMenu) ||
           g_pGuiMan->PanelExists(&pMainInterface->m_actionMenuIos);
}

#ifdef K2_HOST_BUILD
// A modal panel (a message or tutorial box) is up over the game.
bool K2HostModalPanelUp() {
    return g_pGuiMan != 0 && g_pGuiMan->m_modalPanels.m_count != 0;
}
#endif

static void FmtBuf(char* pBuf, unsigned int nSize, const char* szFmt, ...) {
    __builtin_va_list va;
    __builtin_va_start(va, szFmt);
    __vsprintf_chk(pBuf, 0, nSize, szFmt, va);
    __builtin_va_end(va);
}

#if KOTOR1
static const char* const s_aHelpLanguage[4] = { "fr", "it", "de", "es" };

// K1: measure the five localized help-overlay images once, so AddHelpPanel can place its buttons
// over them; each pair is (height, width) in image pixels.
void CSWGuiMainInterface::GatherAndroidHelpPanelConstants() {
    char szImage[256] = { 0 };
    int nLang = ASLPlat_GetCurrentLanguage();
    const char* szLang = nLang >= ASL_LANGUAGE_FRENCH && nLang <= ASL_LANGUAGE_SPANISH ? s_aHelpLanguage[nLang - ASL_LANGUAGE_FRENCH] : "";
    CAurGUIImageInternal* pImage;
    FmtBuf(szImage, 256, "%s%s%s", "ios_help_2", szLang, "_2");
    pImage = NewCAurGUIImage(szImage);
    AurMakeCurrent();
    if (pImage) {
        m_helpPanelConstants[1] = static_cast<float>(pImage->GetImageWidth());
        m_helpPanelConstants[0] = static_cast<float>(pImage->GetImageHeight());
        delete pImage;
    }
    FmtBuf(szImage, 256, "%s%s%s", "ios_help_2", szLang, "_1");
    pImage = NewCAurGUIImage(szImage);
    AurMakeCurrent();
    if (pImage) {
        m_helpPanelConstants[3] = static_cast<float>(pImage->GetImageWidth());
        m_helpPanelConstants[2] = static_cast<float>(pImage->GetImageHeight());
        delete pImage;
    }
    FmtBuf(szImage, 256, "%s%s%s", "ios_help_2", szLang, "_3");
    pImage = NewCAurGUIImage(szImage);
    AurMakeCurrent();
    if (pImage) {
        m_helpPanelConstants[5] = static_cast<float>(pImage->GetImageWidth());
        m_helpPanelConstants[4] = static_cast<float>(pImage->GetImageHeight());
        delete pImage;
    }
    FmtBuf(szImage, 256, "%s%s%s", "ios_help_2", szLang, "_4");
    pImage = NewCAurGUIImage(szImage);
    AurMakeCurrent();
    if (pImage) {
        m_helpPanelConstants[7] = static_cast<float>(pImage->GetImageWidth());
        m_helpPanelConstants[6] = static_cast<float>(pImage->GetImageHeight());
        delete pImage;
    }
    FmtBuf(szImage, 256, "%s%s%s", "ios_help_2", szLang, "_5");
    pImage = NewCAurGUIImage(szImage);
    AurMakeCurrent();
    if (pImage) {
        m_helpPanelConstants[9] = static_cast<float>(pImage->GetImageWidth());
        m_helpPanelConstants[8] = static_cast<float>(pImage->GetImageHeight());
        delete pImage;
    }
}
#endif

#if KOTOR1
// K1: one help/tutorial image skin, both states.
static inline void SetHelpButtonImage(CSWGuiButton& button, const CResRef& image) {
    button.m_borderHilight.m_params.SetFillImage(image, 0);
    button.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    button.m_border.m_params.SetFillImage(image, 0);
    button.m_border.m_params.m_flags.nFillStyle = 2;
}

// K1: pages 0-3 are the help screens (page 1 is five tiles placed from the measured image sizes),
// 4-7 the tutorial cards; a card is centred or pinned beside the widget it explains, and every
// tutorial page shows the "tap help" hint under the help button.
void CSWGuiMainInterface::AddHelpPanel(int nPage) {
#ifdef K2_QOL_FULLSCREEN
    float s_fScale = AndroidHudScaleFactor();
#else
    static float s_fScale = AndroidHudScaleFactor();
#endif
    char szImage[256];
    CSWGuiExtent extent(0, 0, g_SDL_mode.w, g_SDL_mode.h);
    const char* szName;
    int bCentred;
    int bTutorial;
    switch (nPage) {
    case 0: szName = "ios_help_1"; bCentred = 1; bTutorial = 0; break;
    case 1: szName = "ios_help_2"; bCentred = 1; bTutorial = 0; break;
    case 2: szName = "ios_help_3"; bCentred = 1; bTutorial = 0; break;
    case 3: szName = "ios_help_4x"; bCentred = 1; bTutorial = 0; break;
    case 4: szName = "ios_tut_1"; bCentred = 1; bTutorial = 1; break;
    case 5: szName = "ios_tut_2"; bCentred = 0; bTutorial = 1; break;
    case 6: szName = "ios_tut_3"; bCentred = 0; bTutorial = 1; break;
    case 7: szName = "ios_tut_4"; bCentred = 0; bTutorial = 1; break;
    default: szName = ""; bCentred = 0; bTutorial = 0; break;
    }
    int nLang = ASLPlat_GetCurrentLanguage();
    const char* szLang = nLang >= ASL_LANGUAGE_FRENCH && nLang <= ASL_LANGUAGE_SPANISH ? s_aHelpLanguage[nLang - ASL_LANGUAGE_FRENCH] : "";
    FmtBuf(szImage, 256, "%s%s", szName, szLang);
    if (nPage == 1) {
        FmtBuf(szImage, 256, "%s%s%s", szName, szLang, "_2");
        SetHelpButtonImage(m_btnHelpPage, CResRef(szImage));
        extent.w = static_cast<int>(m_helpPanelConstants[1] * s_fScale);
        extent.h = static_cast<int>(s_fScale * m_helpPanelConstants[0]);
        extent.x = static_cast<int>(static_cast<float>(g_SDL_mode.w - extent.w) * 0.5f);
        extent.y = 0;
        m_btnHelpPage.SetExtent(extent);

        FmtBuf(szImage, 256, "%s%s%s", szName, szLang, "_1");
        SetHelpButtonImage(m_btnHelpTopLeft, CResRef(szImage));
        extent.w = static_cast<int>(m_helpPanelConstants[3] * s_fScale);
        extent.x = 0;
        extent.y = 0;
        extent.h = static_cast<int>(s_fScale * m_helpPanelConstants[2]);
#if defined(K2P_IOS) && defined(K2_QOL_CONTROLLER)
        extent.x += K1PhoneEdgeInset();   // beside the HUD's edge buttons
#endif
        m_btnHelpTopLeft.SetExtent(extent);

        static bool s_bHasTouchScreen = HasTouchScreen();
        if (s_bHasTouchScreen) {
            FmtBuf(szImage, 256, "%s%s%s", szName, szLang, "_3");
            SetHelpButtonImage(m_btnHelpTopRight, CResRef(szImage));
            extent.w = static_cast<int>(m_helpPanelConstants[5] * s_fScale);
            extent.x = g_SDL_mode.w - extent.w;
            extent.y = 0;
            extent.h = static_cast<int>(s_fScale * m_helpPanelConstants[4]);
#if defined(K2P_IOS) && defined(K2_QOL_CONTROLLER)
            extent.x -= K1PhoneEdgeInset();   // beside the HUD's edge buttons
#endif
            m_btnHelpTopRight.SetExtent(extent);
        } else {
            m_btnHelpTopRight.m_tail.bVisible = 0;
        }

        FmtBuf(szImage, 256, "%s%s%s", szName, szLang, "_4");
        SetHelpButtonImage(m_btnHelpBottomLeft, CResRef(szImage));
        extent.w = static_cast<int>(m_helpPanelConstants[7] * s_fScale);
        extent.h = static_cast<int>(s_fScale * m_helpPanelConstants[6]);
        extent.x = 0;
        extent.y = g_SDL_mode.h - extent.h;
#if defined(K2P_IOS) && defined(K2_QOL_CONTROLLER)
        extent.x += K1PhoneEdgeInset();   // beside the HUD's edge buttons
#endif
        m_btnHelpBottomLeft.SetExtent(extent);

        FmtBuf(szImage, 256, "%s%s%s", szName, szLang, "_5");
        SetHelpButtonImage(m_btnHelpBottomRight, CResRef(szImage));
        extent.w = static_cast<int>(m_helpPanelConstants[9] * s_fScale);
        extent.x = g_SDL_mode.w - extent.w;
        extent.h = static_cast<int>(s_fScale * m_helpPanelConstants[8]);
        extent.y = g_SDL_mode.h - extent.h;
#if defined(K2P_IOS) && defined(K2_QOL_CONTROLLER)
        extent.x -= K1PhoneEdgeInset();   // beside the HUD's edge buttons
#endif
        m_btnHelpBottomRight.SetExtent(extent);
    } else {
        SetHelpButtonImage(m_btnHelpPage, CResRef(szImage));
        int nScreenW = g_SDL_mode.w;
        int nScreenH = g_SDL_mode.h;
        CAurGUIImageInternal* pImage = NewCAurGUIImage(szImage);
        AurMakeCurrent();
        if (pImage) {
            extent.w = static_cast<int>(s_fScale * static_cast<float>(pImage->GetImageWidth()));
            extent.h = static_cast<int>(s_fScale * static_cast<float>(pImage->GetImageHeight()));
            if (bCentred) {
                extent.x = (nScreenW - extent.w) / 2;
                extent.y = (nScreenH - extent.h) / 2;
            } else if (nPage == 5 || nPage == 7) {
                extent.x = m_btnEquip.m_extent.x - extent.w;
                extent.y = static_cast<int>(static_cast<float>(m_btnEquip.m_extent.y + m_btnEquip.m_extent.h) + s_fScale * 30.0f);
            } else if (nPage == 6) {
                CSWGuiExtent& anchor = m_aCharacters[0].m_btnChar.m_extent;
                extent.x = static_cast<int>(s_fScale * 35.0f + anchor.x + anchor.w);
                extent.y = static_cast<int>(anchor.y + s_fScale * -40.0f - extent.h);
            }
            delete pImage;
        }
        m_btnHelpPage.SetExtent(extent);
    }

    static bool s_bHintHasTouchScreen = HasTouchScreen();
    if (s_bHintHasTouchScreen) {
        FmtBuf(szImage, 256, "ios_tut_h1%s", szLang);
        SetHelpButtonImage(m_btnHelpHint, CResRef(szImage));
        CAurGUIImageInternal* pImage = NewCAurGUIImage(szImage);
        AurMakeCurrent();
        CSWGuiExtent hint(0, 0, 0, 0);
        if (pImage) {
            hint.w = static_cast<int>(s_fScale * static_cast<float>(pImage->GetImageWidth()));
            hint.h = static_cast<int>(s_fScale * static_cast<float>(pImage->GetImageHeight()));
            CSWGuiExtent& help = m_btnCornerHelp.m_extent;
            hint.x = help.x - hint.w + help.w / 2;
            hint.y = static_cast<int>(static_cast<float>(help.y + help.h) + s_fScale * -10.0f);
            delete pImage;
        }
        m_btnHelpHint.SetExtent(hint);
        m_btnHelpHint.m_tail.bVisible = bTutorial;
    } else {
        m_btnHelpHint.m_tail.bVisible = 0;
    }
    g_pGuiMan->AddPanel(&m_helpPanel, 1, 1);
}
#endif

#if !KOTOR1
void CSWGuiMainInterface::AddHelpPanel(int nPage, bool bGamepad) {
    int nScreenW = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
    int nScreenH = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight;

    float fAnchorY = 0.0f;
    float fScale = 1.0f;
    const char* pszImagePrefix;
    const char* pszAspect;
    bool bNumbered;
    int nLayout;
    float fAnchorX;
    float fAlignV;
    float fAlignH;
    switch (nPage) {
    case 0:
    case 1:
        nLayout = 3;
        bNumbered = true;
        pszImagePrefix = "ihlp";
        pszAspect = ASLIsDeviceWidescreen() ? "169" : "43";
        fAnchorX = 0.0f;
        fAlignV = 0.0f;
        fAlignH = 0.0f;
        break;
    case 2:
        nLayout = 2;
        bNumbered = true;
        pszImagePrefix = "ihlp";
        pszAspect = "";
        fAnchorX = 0.0f;
        fAlignV = 0.0f;
        fAlignH = 0.0f;
        break;
    case 3: {
        const CSWGuiExtent& rcExt = m_btnHelp.m_extent;
        fAnchorX = static_cast<float>(rcExt.x + rcExt.w / 2);
        fAnchorY = static_cast<float>(rcExt.h + rcExt.y + rcExt.h / -3);
        nLayout = 0;
        bNumbered = true;
        pszImagePrefix = "ihlp";
        pszAspect = "";
        fScale = 1.0f;
        fAlignV = 1.0f;
        fAlignH = -1.0f;
        break;
    }
    case 4: {
        const CSWGuiExtent& rcExt = m_aCharacters[2].m_btnChar.m_extent;
        fAnchorX = static_cast<float>(rcExt.x + rcExt.w / 2);
        fAnchorY = static_cast<float>(rcExt.y + rcExt.h / 3);
        nLayout = 0;
        bNumbered = true;
        pszImagePrefix = "ihlp";
        pszAspect = "";
        fAlignV = -1.0f;
        fAlignH = -1.0f;
        break;
    }
    case 5: {
        const CSWGuiExtent& rcExt = m_btnOptions.m_extent;
        fAnchorX = static_cast<float>(rcExt.x + rcExt.w / 2);
        fAnchorY = static_cast<float>(rcExt.h + rcExt.y + rcExt.h / -3);
        fScale = 0.9f;
        nLayout = 0;
        bNumbered = false;
        pszImagePrefix = "inv";
        pszAspect = "";
        fAlignV = 1.0f;
        fAlignH = -1.0f;
        break;
    }
    default:
        nLayout = 0;
        bNumbered = true;
        pszImagePrefix = "ihlp";
        pszAspect = "";
        fAnchorX = 0.0f;
        fAlignV = 0.0f;
        fAlignH = 0.0f;
        break;
    }

    const char* pszLang = GetLocalizationFolderName();
    if (strstr(pszLang, "rench"))
        pszLang = "fre";
    else if (strstr(pszLang, "talian"))
        pszLang = "ita";
    else if (strstr(pszLang, "erman"))
        pszLang = "deu";
    else if (strstr(pszLang, "panish"))
        pszLang = "esp";
    else
        pszLang = "eng";

    char szPageSfx[10];
    szPageSfx[0] = 0;
    for (int nI = 1; nI < 10; ++nI) szPageSfx[nI] = 0;
    if (bNumbered)
        FmtBuf(szPageSfx, 10, "_%d", nPage + 1);
    if (IsJoystickConnected() && nPage == 0) {
        pszImagePrefix = "ios_mfi";
        pszAspect = "";
        nLayout = 2;
        szPageSfx[0] = 0;
    }
    char szImage[256];
    FmtBuf(szImage, 256, "%s%s%s_%s", pszImagePrefix, pszAspect, szPageSfx, pszLang);
#if defined(K2_HOST_BUILD) && !defined(K2_UI_TOUCH)
    // pc / xbox: the overlays are the mobile ports' art; the PC installs have none of it, and a
    // missing texture draws a white box.
    {
        CResRef resArt(szImage);
        if (!g_pExoResMan->Exists(resArt, 3, static_cast<unsigned int*>(0)) &&
            !g_pExoResMan->Exists(resArt, 3007, static_cast<unsigned int*>(0)) &&
            !g_pExoResMan->Exists(resArt, 2033, static_cast<unsigned int*>(0)))
            return;
    }
#endif

    int nImgW = nScreenW;
    int nImgH = nScreenH;
    CAurGUIImageInternal* pImage = NewCAurGUIImage(szImage);
    AurMakeCurrent();
    if (pImage) {
        nImgW = pImage->GetImageWidth();
        nImgH = pImage->GetImageHeight();
    }

    float fW = static_cast<float>(nScreenW);
    CSWGuiExtent sExtent;
    sExtent.w = nScreenW;
    sExtent.h = nScreenH;
    if (nLayout == 3) {
        int nInset = 0;
        if (g_bAddEdgeOffset) {
            nInset = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nUIEdgeInset;
            fW -= static_cast<float>(static_cast<unsigned>(nInset) << 1);
        }
        float fOff;
#ifdef K2_FIX_ENGINE_BUGS
        if (pszAspect && strcmp(pszAspect, "43") == 0)
#else
        if (pszAspect == "43")
#endif
            fOff = 0.0f;
        else
            fOff = fW / static_cast<float>(static_cast<unsigned int>(g_nGUIWidth)) * static_cast<float>(m_btnHelp.m_originalExtent.w) -
                   static_cast<float>(m_btnHelp.m_extent.w);
        sExtent.x = static_cast<int>(static_cast<float>(sExtent.x + nInset) + fOff * -0.5f);
        sExtent.y = 0;
        sExtent.w = static_cast<int>(fW + fOff);
    } else if (nLayout == 2) {
        int nScaledW = static_cast<int>(static_cast<float>(nScreenH) / static_cast<float>(nImgH) * static_cast<float>(nImgW));
        sExtent.w = nScaledW;
        sExtent.x = static_cast<int>((fW - static_cast<float>(nScaledW)) * 0.5f);
    } else {
        float fBase = fW / 1024.0f * fScale;
        int nA = static_cast<int>(fBase * static_cast<float>(nImgW));
        int nB = static_cast<int>(fBase * static_cast<float>(nImgH));
        int nHalfA = nA / 2;
        int nHalfB = nB / 2;
        sExtent.w = nA;
        sExtent.h = nB;
        int nXcand = fAlignH < 0.0f ? nA : nHalfA;
        sExtent.x = static_cast<int>(fAnchorX - static_cast<float>(nXcand));
        int nY;
        if (fAlignV < 0.0f)
            nY = static_cast<int>(fAnchorY - static_cast<float>(nB));
        else if (!(fAlignV > 0.0f))
            nY = static_cast<int>(fAnchorY - static_cast<float>(nHalfB));
        else
            nY = static_cast<int>(fAnchorY);
        sExtent.y = nY;
    }
    if (pImage)
        delete pImage;

    if (bGamepad) {
        g_fGamepadHelpTimer = 5.0f;
        {
            CResRef res(szImage);
            m_btnGamepadHelp.m_border.m_params.SetFillImage(res, 0);
        }
        {
            CResRef res2(szImage);
            m_btnGamepadHelp.m_borderHilight.m_params.SetFillImage(res2, 0);
        }
        m_btnGamepadHelp.m_tail.bVisible = 1;
        m_btnGamepadHelp.SetExtent(sExtent);
    } else {
        {
            CResRef res(szImage);
            m_btnHelpPage.m_border.m_params.SetFillImage(res, 0);
        }
        {
            CResRef res2(szImage);
            m_btnHelpPage.m_borderHilight.m_params.SetFillImage(res2, 0);
        }
        m_btnHelpPage.m_border.m_params.m_flags.nFillStyle = 2;
        m_btnHelpPage.m_borderHilight.m_params.m_flags.nFillStyle = 2;
        m_btnHelpPage.SetExtent(sExtent);
        g_pGuiMan->AddPanel(&m_helpPanel, 1, 1);
    }
}
#endif

#if KOTOR1
void CSWGuiMainInterface::Draw(float fDelta) {
    if (!(m_stateFlags & PANEL_STATE_DRAW))  // K1: the panel state bits sit one higher (bit 3 inserted)
        return;
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    if (!pPlayer || pPlayer->m_objectId == OBJECT_INVALID)
        return;
    if (m_fChallengeRating != -1.0f) {
        m_fChallengeRating -= fDelta;
        if (m_fChallengeRating <= 0.0f)
            m_fChallengeRating = -1.0f;
    }
    UpdatePortraits();
    UpdateIndicator(m_pGuiManager->m_fDeltaTStore);
    m_targetActionMenu.UpdateTimers(fDelta);
#ifdef K1_UI_XBOX
    K1ConsoleMouseHover();
#endif
    PopulateMenus();

    for (int nI = 0; nI < 9; nI++)
        m_statusIcons[nI].Update(m_pGuiManager->m_fDeltaTStore);

    if (m_fRotationTimer > 0.0f) {
        m_fRotationTimer -= fDelta;
        if (m_fRotationTimer > 0.0f) {
            m_aCharacters[0].Adjust(m_fRotationTimer);
            m_aCharacters[1].Adjust(m_fRotationTimer);
            m_aCharacters[2].Adjust(m_fRotationTimer);
            m_aCharacters[3].Adjust(m_fRotationTimer);
        } else {
            m_aCharacters[0].Adjust(0.25f);
            m_aCharacters[1].Adjust(0.25f);
            m_aCharacters[2].Adjust(0.25f);
            m_aCharacters[3].Adjust(0.25f);
            m_aCharacters[3].m_aLabels[5].m_tail.bVisible = 0;
            m_aCharacters[3].m_aLabels[0].m_tail.bVisible = 0;
            m_aCharacters[3].m_aBars[0].m_tail.bVisible = 0;
            m_aCharacters[3].m_aBars[1].m_tail.bVisible = 0;
            m_aCharacters[3].m_aLabels[1].m_tail.bVisible = 0;
            m_aCharacters[3].m_aLabels[2].m_tail.bVisible = 0;
            m_aCharacters[3].m_aLabels[3].m_tail.bVisible = 0;
            m_aCharacters[3].m_aLabels[4].m_tail.bVisible = 0;
            m_aCharacters[3].m_nGoodEffectCount = 0;
            m_aCharacters[3].m_nBadEffectCount = 0;
            m_fRotationTimer = 0.0f;
            m_aCharacters[3].m_btnChar.m_tail.bVisible = 0;
#ifdef K1_UI_XBOX
            // xbox: the portraits above were filled in the rotation's slot mapping; back at their
            // home slots that would show the old order for a frame, so refill them now.
            UpdatePortraits();
#endif
        }
    }

    ShowActionQueue(g_pAppManager->m_pClientExoApp->GetCombatMode());

    for (int nI = 0; nI < 6; nI++) {
        CSWGuiMainInterfaceAction& rcAction = m_personalActionWidgets[nI];
        if (rcAction.m_nPulseSteps != 0) {
            rcAction.m_fPulseTimer -= fDelta;
            if (rcAction.m_fPulseTimer <= 0.0f) {
                rcAction.m_nPulseSteps -= 1;
                rcAction.m_fPulseTimer = 0.1f;
            }
            float fA;
            if (rcAction.m_nPulseSteps == 0) {
                fA = 1.0f;
            } else {
                float fT = (0.1f - rcAction.m_fPulseTimer) / 0.1f;
                fA = rcAction.m_nPulseSteps == 2 ? 1.0f - fT : fT;
            }
            rcAction.m_icon.m_border.m_params.m_fAlpha = fA;
            rcAction.m_icon.m_borderHilight.m_params.m_fAlpha = fA;
        }
    }

    if ((m_nInterfaceFlags & 2) != 0)
        UpdateActionQueue();

    if (g_bShowIndicator != 0) {
        DrawMap(fDelta);
#ifdef K1_UI_XBOX
        // xbox: the caption plate first, clipped (see K1ConsoleDrawPlate), then the rest without it.
        int bPlate = m_actionDescLabel2.m_tail.bVisible;
        if (bPlate) {
            AurGUIStartLayer();
            if (AurGUISetupViewport(m_extent.x, m_extent.y, m_extent.w, m_extent.h, m_vColor,
                                    false, m_fAlpha)) {
                K1ConsoleDrawPlate(m_actionDescLabel2, m_lblMoulding1, fDelta);
                AurGUICloseViewport();
            }
            AurGUIStopLayer();
        }
        m_actionDescLabel2.m_tail.bVisible = 0;
        int abIconVisible[6];
        K1ConsoleTickScroll(m_personalActionWidgets, fDelta, abIconVisible);
        int abStripActive[6];
        for (int i = 0; i < 6; i++)
            abStripActive[i] = ConsoleStripUnlit(m_personalActionWidgets[i]);
        CSWGuiPanel::Draw(fDelta);
        for (int i = 0; i < 6; i++)
            m_personalActionWidgets[i].m_btnUp.m_tail.bActive = abStripActive[i];
        m_actionDescLabel2.m_tail.bVisible = bPlate;
        AurGUIStartLayer();
        if (AurGUISetupViewport(m_extent.x, m_extent.y, m_extent.w, m_extent.h, m_vColor, false,
                                m_fAlpha)) {
            K1ConsoleDrawScroll(m_personalActionWidgets, fDelta, abIconVisible);
            for (int i = 0; i < 6; i++)
                ConsoleDrawArrowHover(m_personalActionWidgets[i], fDelta);
            AurGUICloseViewport();
        } else {
            for (int i = 0; i < 6; i++)
                m_personalActionWidgets[i].m_icon.m_tail.bVisible = abIconVisible[i];
        }
        AurGUIStopLayer();
#else
        CSWGuiPanel::Draw(fDelta);
#endif
        m_targetActionMenu.Draw(fDelta);
        AurGUIStartLayer();
        if (AurGUISetupViewport(m_extent.x, m_extent.y, m_extent.w, m_extent.h, m_vColor, false,
                                m_fAlpha)) {
            for (int nI = m_floatyTexts.m_count - 1; nI >= 0; nI--) {
                CSWGuiFloatyText* pFloaty = m_floatyTexts.m_pArray[nI];
                if (pFloaty)
                    pFloaty->Draw(fDelta);
            }
            AurGUICloseViewport();
        }
        if (m_aCharacters[0].m_aLabels[5].m_tail.bVisible) {
            if (m_aCharacters[0].m_nGoodEffectCount)
                m_aCharacters[0].DrawStack(fDelta, 1, 1);
            if (m_aCharacters[0].m_nBadEffectCount)
                m_aCharacters[0].DrawStack(fDelta, 1, 0);
        }
        if (m_aCharacters[1].m_aLabels[5].m_tail.bVisible) {
            if (m_aCharacters[1].m_nGoodEffectCount)
                m_aCharacters[1].DrawStack(fDelta, 0, 1);
            if (m_aCharacters[1].m_nBadEffectCount)
                m_aCharacters[1].DrawStack(fDelta, 0, 0);
        }
        if (m_aCharacters[2].m_aLabels[5].m_tail.bVisible) {
            if (m_aCharacters[2].m_nGoodEffectCount)
                m_aCharacters[2].DrawStack(fDelta, 0, 1);
            if (m_aCharacters[2].m_nBadEffectCount)
                m_aCharacters[2].DrawStack(fDelta, 0, 0);
        }
        if (m_aCharacters[3].m_aLabels[5].m_tail.bVisible) {
            if (m_aCharacters[3].m_nGoodEffectCount)
                m_aCharacters[3].DrawStack(fDelta, 0, 1);
            if (m_aCharacters[3].m_nBadEffectCount)
                m_aCharacters[3].DrawStack(fDelta, 0, 0);
        }
        AurGUIStopLayer();
    }
}
#else
void CSWGuiMainInterface::Draw(float fDelta) {
    if (!(m_stateFlags & PANEL_STATE_DRAW))
        return;
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    if (!pPlayer || pPlayer->m_objectId == OBJECT_INVALID)
        return;
    if (m_fChallengeRating != -1.0f) {
        m_fChallengeRating -= fDelta;
        if (m_fChallengeRating <= 0.0f)
            m_fChallengeRating = -1.0f;
    }
    UpdatePortraits();
    UpdateGameMenu();
    UpdateIndicator(m_pGuiManager->m_fDeltaTStore);
    m_targetActionMenu.UpdateTimers(fDelta);
    PopulateMenus(0);

    if (m_bPartyItemUsePending != 0) {
        CSWSCreature* pServer = pPlayer->GetServerCreature();
        if (pServer != 0 && (pServer->IsDebilitated(0) || pServer->GetDead() ||
                             (pServer->m_bInParty && pServer->GetCurrentHitPoints(0) <= 0)))
            m_bPartyItemUsePending = 0;
    }
    if (m_bPartyItemUsePending != 0) {
        m_feedbackTimer = 0.0f;
        m_feedbackStrRef = 0;
        CExoString sText;
        int nIdx = m_nPartyItemUseTarget;
        if (nIdx == 2)
            nIdx = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount == 3 ? 2 : 1;
        if (nIdx == -1) {
            sText = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_PARTY_ITEM_USE_CLICK_PORTRAIT);
            sText = sText + "\n" + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CLICK_PANEL_TO_CANCEL);
            m_btnFeedback.m_text.m_params.SetText(sText);
            m_btnFeedback.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        } else {
            CSWCCreature* pTarget = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nIdx);
            int nServerId = static_cast<int>(g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_oidPartyItemUse));
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(nServerId);
            if (pTarget->GetServerCreature()->CanUseItem(pItem, 0, 1)) {
                sText = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_USE_ITEM_ON);
                sText = sText + " " + pTarget->GetName();
                sText = sText + "\n" + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CLICK_PANEL_TO_CANCEL);
                m_btnFeedback.m_text.m_params.SetText(sText);
                m_btnFeedback.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
            } else {
                sText = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CANNOT_USE_ITEM_ON);
                sText = sText + " " + pTarget->GetName();
                sText = sText + "\n" + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CLICK_PANEL_TO_CANCEL);
                m_btnFeedback.m_text.m_params.SetText(sText);
                m_btnFeedback.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
            }
        }
#if defined(K2_UI_XBOX) || defined(K2_UI_PC)
        PlaceFeedbackAbovePlate(m_btnFeedback, m_actionDescLabel2.m_extent, m_pGuiManager->m_nScreenHeight);
#endif
        m_btnFeedback.SetEnabled(1);
        m_btnFeedback.m_tail.bVisible = 1;
        m_btnFeedback.Draw(fDelta);
    }
    if (m_bPartyItemUsePending == 0) {
        if (m_feedbackTimer > 0.0f) {
            m_feedbackTimer -= fDelta;
            if (m_feedbackTimer <= 0.0f) {
                m_feedbackTimer = 0.0f;
            } else {
                m_btnFeedback.m_text.m_params.SetText(
                    g_pAppManager->m_pClientExoApp->GetGUIString(m_feedbackStrRef));
                m_btnFeedback.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
#if defined(K2_UI_XBOX) || defined(K2_UI_PC)
                PlaceFeedbackAbovePlate(m_btnFeedback, m_actionDescLabel2.m_extent,
                                        m_pGuiManager->m_nScreenHeight);
#endif
                m_btnFeedback.SetEnabled(0);
                m_btnFeedback.m_tail.bVisible = 1;
                m_btnFeedback.Draw(fDelta);
            }
        } else {
            m_btnFeedback.m_tail.bVisible = 0;
        }
    }

    for (int nI = 0; nI < 13; nI++)
        m_statusIcons[nI].Update(m_pGuiManager->m_fDeltaTStore);

    // the portrait panel's hide countdown
    if (m_fRotationTimer > 0.0f) {
        m_fRotationTimer -= fDelta;
        if (m_fRotationTimer > 0.0f) {
            m_aCharacters[0].Adjust(m_fRotationTimer);
            m_aCharacters[1].Adjust(m_fRotationTimer);
            m_aCharacters[2].Adjust(m_fRotationTimer);
            m_aCharacters[3].Adjust(m_fRotationTimer);
        } else {
            m_aCharacters[0].Adjust(0.25f);
            m_aCharacters[1].Adjust(0.25f);
            m_aCharacters[2].Adjust(0.25f);
            m_aCharacters[3].Adjust(0.25f);
            m_aCharacters[3].m_aLabels[4].m_tail.bVisible = 0;
            m_aCharacters[3].m_aLabels[0].m_tail.bVisible = 0;
            m_aCharacters[3].m_aBars[0].m_tail.bVisible = 0;
            m_aCharacters[3].m_aBars[1].m_tail.bVisible = 0;
            m_aCharacters[3].m_aLabels[1].m_tail.bVisible = 0;
            m_aCharacters[3].m_aLabels[2].m_tail.bVisible = 0;
            m_aCharacters[3].m_nGoodEffectCount = 0;
            m_aCharacters[3].m_nBadEffectCount = 0;
            m_fRotationTimer = 0.0f;
            m_aCharacters[3].m_aLabels[3].m_tail.bVisible = 0;
            m_aCharacters[3].m_btnChar.m_tail.bVisible = 0;
        }
    }

    ShowActionQueue(g_pAppManager->m_pClientExoApp->GetCombatMode());

    // the personal action slots' dim pulse
    for (int nI = 0; nI < 7; nI++) {
        CSWGuiMainInterfaceAction& rcAction = m_personalActionWidgets[nI];
        if (rcAction.m_nPulseSteps != 0) {
            rcAction.m_fPulseTimer -= fDelta;
            float fA;
            if (rcAction.m_fPulseTimer <= 0.0f) {
                rcAction.m_nPulseSteps -= 1;
                rcAction.m_fPulseTimer = 0.1f;
                if (rcAction.m_nPulseSteps != 0) {
                    float fT = (0.1f - rcAction.m_fPulseTimer) / 0.1f;
                    fA = rcAction.m_nPulseSteps == 2 ? 1.0f - fT : fT;
                } else {
                    fA = 1.0f;
                }
            } else {
                float fT = (0.1f - rcAction.m_fPulseTimer) / 0.1f;
                fA = rcAction.m_nPulseSteps == 2 ? 1.0f - fT : fT;
            }
            rcAction.m_icon.m_border.m_params.m_fAlpha = fA;
            rcAction.m_icon.m_borderHilight.m_params.m_fAlpha = fA;
        }
    }

    if ((m_nInterfaceFlags & 2) != 0)
        UpdateActionQueue();

    // Draw the HUD over the world, in layers
    if (g_bShowIndicator != 0) {
        DrawMap(fDelta);
#if defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
        int abIconVisible[9];
        K2ConsoleTickScroll(this, fDelta, abIconVisible);
        int abStripActive[CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT + 1];
        for (int i = 0; i <= CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT; i++)
            abStripActive[i] = ConsoleStripUnlit(*GetActionByIndex(i));
#endif
        CSWGuiPanel::Draw(fDelta);
        m_targetActionMenu.Draw(fDelta);
#if defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
        for (int i = 0; i <= CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT; i++)
            GetActionByIndex(i)->m_btnUp.m_tail.bActive = abStripActive[i];
#endif
#if defined(K2_UI_XBOX) && defined(K2_HOST_BUILD)
        AurGUIStartLayer();
        if (AurGUISetupViewport(m_extent.x, m_extent.y, m_extent.w, m_extent.h, m_vColor, false,
                                m_fAlpha)) {
            K2ConsoleDrawScroll(this, fDelta, abIconVisible);
            for (int i = 0; i <= CSWGuiMainInterfaceAction::ACTION_SLOT_TARGET_LEFT; i++)
                ConsoleDrawArrowHover(*GetActionByIndex(i), fDelta);
            AurGUICloseViewport();
        } else {
            for (int i = 0; i < 9; i++)
                GetActionByIndex(i)->m_icon.m_tail.bVisible = abIconVisible[i];
        }
        AurGUIStopLayer();
#endif
        AurGUIStartLayer();
        if (AurGUISetupViewport(m_extent.x, m_extent.y, m_extent.w, m_extent.h, m_vColor, false,
                                m_fAlpha)) {
            for (int nI = m_floatyTexts.m_count - 1; nI >= 0; nI--) {
                CSWGuiFloatyText* pFloaty = m_floatyTexts.m_pArray[nI];
                if (pFloaty)
                    pFloaty->Draw(fDelta);
            }
            AurGUICloseViewport();
        }
        if (m_aCharacters[0].m_aLabels[4].m_tail.bVisible) {
            if (m_aCharacters[0].m_nGoodEffectCount)
                m_aCharacters[0].DrawStack(fDelta, 1, 1);
            if (m_aCharacters[0].m_nBadEffectCount)
                m_aCharacters[0].DrawStack(fDelta, 1, 0);
        }
        if (m_aCharacters[1].m_aLabels[4].m_tail.bVisible) {
            if (m_aCharacters[1].m_nGoodEffectCount)
                m_aCharacters[1].DrawStack(fDelta, 0, 1);
            if (m_aCharacters[1].m_nBadEffectCount)
                m_aCharacters[1].DrawStack(fDelta, 0, 0);
        }
        if (m_aCharacters[2].m_aLabels[4].m_tail.bVisible) {
            if (m_aCharacters[2].m_nGoodEffectCount)
                m_aCharacters[2].DrawStack(fDelta, 0, 1);
            if (m_aCharacters[2].m_nBadEffectCount)
                m_aCharacters[2].DrawStack(fDelta, 0, 0);
        }
        if (m_aCharacters[3].m_aLabels[4].m_tail.bVisible) {
            if (m_aCharacters[3].m_nGoodEffectCount)
                m_aCharacters[3].DrawStack(fDelta, 0, 1);
            if (m_aCharacters[3].m_nBadEffectCount)
                m_aCharacters[3].DrawStack(fDelta, 0, 0);
        }
        AurGUIStopLayer();
    }
}
#endif

#if KOTOR1
void CSWGuiMainInterface::UpdatePortraits() {
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CExoString sScratch;
    for (int nI = 0; nI < 3; nI++) {
        CSWCCreature* pChar = pParty->GetCharacter(nI);
        CSWSCreature* pServer = pChar ? pChar->GetServerCreature() : 0;
        bool bShow = pServer != 0;
        int nSlot = nI;
        if (m_fRotationTimer > 0.0f) {
            if (nI == 0) {
                nSlot = 1;
            } else if (nI == 1) {
                bool bShift = pParty->m_nCharacterCount != 2;
                bShow = bShow && bShift;
                nSlot = bShift ? 2 : 0;
            } else if (nI == 2) {
                nSlot = 0;
                bShow = bShow || pParty->m_nCharacterCount == 2;
            }
        } else if (pParty->m_nCharacterCount == 2) {
            if (nI == 1) {
                nSlot = 2;
                bShow = false;
            } else if (nI == 2) {
                nSlot = 2;
                bShow = true;
            }
        }
        if (pServer != 0) {
            int nState;
            int nEffA = 0;
            int nEffB = 0;
            m_aCharacters[nSlot].m_sFirstName = pChar->m_pStats->m_name;
            m_aCharacters[nSlot].m_sLastName = pChar->m_pStats->m_sLastName;
            m_aCharacters[nSlot].m_aLabels[5].m_border.m_params.SetFillImage(pChar->GetPortrait(1), 0);
            short nCurHP = pServer->GetCurrentHitPoints(0);
            short nMaxHP = pServer->GetMaxHitPoints(1);
            int bPoisoned = pServer->m_bPoisoned;
            m_aCharacters[nSlot].m_aBars[0].SetMaxValue(nMaxHP);
            m_aCharacters[nSlot].m_aBars[0].SetCurValue(nCurHP);
#ifdef K1_UI_XBOX
            CResRef sGreen("lbl_healthp");
            CResRef sRed("lbl_health");
#else
            CResRef sGreen("greenfill");
            CResRef sRed("redfill");
#endif
            m_aCharacters[nSlot].m_aBars[0].m_borderProgress.m_params.SetFillImage(bPoisoned ? sGreen : sRed, 0);
            m_aCharacters[nSlot].m_nMaxHP = nMaxHP;
            m_aCharacters[nSlot].m_nCurHP = nCurHP;
            m_aCharacters[nSlot].m_bToolTipDirty = 1;
            short nFPBonus = pServer->m_pStats->m_nTemporaryForcePoints;
            short nFPCur = pServer->m_pStats->m_nCurrentForcePoints;
            short nMaxFP = pServer->GetMaxForcePoints();
            short nFPCurTot = nFPBonus + nFPCur;
            if (nMaxFP == 0) {
                m_aCharacters[nSlot].m_aBars[1].SetMaxValue(1);
                m_aCharacters[nSlot].m_aBars[1].SetCurValue(0);
            } else {
                m_aCharacters[nSlot].m_aBars[1].SetMaxValue(nMaxFP);
                m_aCharacters[nSlot].m_aBars[1].SetCurValue(nFPCurTot);
            }
            m_aCharacters[nSlot].m_nMaxFP = nMaxFP;
            m_aCharacters[nSlot].m_nCurFP = nFPCurTot;
            m_aCharacters[nSlot].m_bToolTipDirty = 1;
            m_aCharacters[nSlot].m_nLevel = pServer->m_pStats->GetLevel(0) & 0xff;
            m_aCharacters[nSlot].m_bToolTipDirty = 1;
            int nExpDiff = pServer->m_pStats->GetExpNeededForLevelUp() - pServer->m_pStats->m_nExperience;
            if (nExpDiff < 1)
                nExpDiff = 0;
            m_aCharacters[nSlot].m_nXPToLevel = nExpDiff;
            m_aCharacters[nSlot].m_bToolTipDirty = 1;
            if (pServer->GetDead() || pServer->GetDeadTemp()) {
                nState = 1;
            } else if (pServer->m_pStats->CanLevelUp()) {
                if (pChar->m_nStateFlags & 2) {
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_LEVEL_UP_SOUND, 0);
                    pChar->m_nStateFlags &= ~2;
                }
                nState = 4;
            } else if (pServer->IsDebilitated(0)) {
                if (nI == 0)
                    SetCombatMessage(STRREF_PLAYER_DEBILITATED);
                nState = 2;
            } else {
                int nGood = 0;
                int nBad = 0;
                for (int nE = 0; nE < pServer->m_effectIcons.m_count; nE++) {
                    CEffectIconObject* pEffect = pServer->m_effectIcons.m_pArray[nE];
                    if (pEffect->m_nGood != 0) {
                        if (pEffect->m_bGood != 0)
                            nGood++;
                        else
                            nBad++;
                    }
                }
                nEffA = nGood;
                nEffB = nBad;
                nState = 0;
            }
            if (pServer->IsDebilitated(0) == 0 && m_combatMessage == STRREF_PLAYER_DEBILITATED)
                SetCombatMessage(STRREF_COMBAT_MODE_ENGAGED);
            m_aCharacters[nSlot].m_aLabels[5].m_tail.bVisible = 1;
            m_aCharacters[nSlot].m_aLabels[0].m_tail.bVisible = 1;
            m_aCharacters[nSlot].m_aBars[0].m_tail.bVisible = 1;
            m_aCharacters[nSlot].m_aBars[1].m_tail.bVisible = 1;
            m_aCharacters[nSlot].m_aLabels[1].m_tail.bVisible = (nState & 2) != 0;
            m_aCharacters[nSlot].m_aLabels[2].m_tail.bVisible = (nState & 1) != 0;
            m_aCharacters[nSlot].m_aLabels[3].m_tail.bVisible = (nState & 4) != 0;
            m_aCharacters[nSlot].m_aLabels[4].m_tail.bVisible = (nState & 4) != 0;
            m_aCharacters[nSlot].m_nGoodEffectCount = static_cast<unsigned char>(nState == 0 ? nEffA : 0);
            m_aCharacters[nSlot].m_nBadEffectCount = static_cast<unsigned char>(nState == 0 ? nEffB : 0);
            m_aCharacters[nSlot].m_btnChar.m_tail.bVisible = 1;
            if (nSlot == 0 && m_fRotationTimer > 0.0f) {
                m_aCharacters[3].m_sFirstName = pChar->m_pStats->m_name;
                m_aCharacters[3].m_sLastName = pChar->m_pStats->m_sLastName;
                m_aCharacters[3].m_aLabels[5].m_border.m_params.SetFillImage(pChar->GetPortrait(1), 0);
                short nCurHPMirror = pServer->GetCurrentHitPoints(0);
                short nMaxHPMirror = pServer->GetMaxHitPoints(1);
                int bPoisonedMirror = pServer->m_bPoisoned;
                m_aCharacters[3].m_aBars[0].SetMaxValue(nMaxHPMirror);
                m_aCharacters[3].m_aBars[0].SetCurValue(nCurHPMirror);
#ifdef K1_UI_XBOX
                CResRef sGreenMirror("lbl_healthp");
                CResRef sRedMirror("lbl_health");
#else
                CResRef sGreenMirror("greenfill");
                CResRef sRedMirror("redfill");
#endif
                m_aCharacters[3].m_aBars[0].m_borderProgress.m_params.SetFillImage(bPoisonedMirror ? sGreenMirror : sRedMirror, 0);
                m_aCharacters[3].m_nCurHP = nCurHPMirror;
                m_aCharacters[3].m_nMaxHP = nMaxHPMirror;
                m_aCharacters[3].m_bToolTipDirty = 1;
                short nFPBonusMirror = pServer->m_pStats->m_nTemporaryForcePoints;
                short nFPCurMirror = pServer->m_pStats->m_nCurrentForcePoints;
                short nMaxFPMirror = pServer->GetMaxForcePoints();
                short nFPCurTotMirror = nFPBonusMirror + nFPCurMirror;
                if (nMaxFPMirror == 0) {
                    m_aCharacters[3].m_aBars[1].SetMaxValue(1);
                    m_aCharacters[3].m_aBars[1].SetCurValue(0);
                } else {
                    m_aCharacters[3].m_aBars[1].SetMaxValue(nMaxFPMirror);
                    m_aCharacters[3].m_aBars[1].SetCurValue(nFPCurTotMirror);
                }
                m_aCharacters[3].m_nCurFP = nFPCurTotMirror;
                m_aCharacters[3].m_nMaxFP = nMaxFPMirror;
                m_aCharacters[3].m_bToolTipDirty = 1;
                m_aCharacters[3].m_nLevel = pServer->m_pStats->GetLevel(0) & 0xff;
                m_aCharacters[3].m_bToolTipDirty = 1;
                int nExpDiffMirror = pServer->m_pStats->GetExpNeededForLevelUp() - pServer->m_pStats->m_nExperience;
                if (nExpDiffMirror < 1)
                    nExpDiffMirror = 0;
                m_aCharacters[3].m_nXPToLevel = nExpDiffMirror;
                m_aCharacters[3].m_bToolTipDirty = 1;
                m_aCharacters[3].m_aLabels[5].m_tail.bVisible = 1;
                m_aCharacters[3].m_aLabels[0].m_tail.bVisible = 1;
                m_aCharacters[3].m_aBars[0].m_tail.bVisible = 1;
                m_aCharacters[3].m_aBars[1].m_tail.bVisible = 1;
                m_aCharacters[3].m_aLabels[1].m_tail.bVisible = (nState & 2) != 0;
                m_aCharacters[3].m_aLabels[2].m_tail.bVisible = (nState & 1) != 0;
                m_aCharacters[3].m_aLabels[3].m_tail.bVisible = (nState & 4) != 0;
                m_aCharacters[3].m_aLabels[4].m_tail.bVisible = (nState & 4) != 0;
                m_aCharacters[3].m_nGoodEffectCount = static_cast<unsigned char>(nState == 0 ? nEffA : 0);
                m_aCharacters[3].m_nBadEffectCount = static_cast<unsigned char>(nState == 0 ? nEffB : 0);
                m_aCharacters[3].m_btnChar.m_tail.bVisible = 1;
            }
        }
        if (!bShow) {
            m_aCharacters[nI].m_aLabels[5].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aLabels[0].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aBars[0].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aBars[1].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aLabels[1].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aLabels[2].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aLabels[3].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aLabels[4].m_tail.bVisible = 0;
            m_aCharacters[nI].m_nGoodEffectCount = 0;
            m_aCharacters[nI].m_nBadEffectCount = 0;
            m_aCharacters[nI].m_btnChar.m_tail.bVisible = 0;
        }
    }
}
#else
void CSWGuiMainInterface::UpdatePortraits() {
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    CExoString sScratch;
    for (int nI = 0;; nI++) {
        CSWCCreature* pChar = pParty->GetCharacter(nI);
        CSWSCreature* pServer = pChar ? pChar->GetServerCreature() : 0;
        bool bShow = pServer != 0;
        int nSlot = nI;
        if (m_fRotationTimer > 0.0f) {
            if (nI == 0) {
                nSlot = 1;
            } else if (nI == 1) {
                bool bShift = pParty->m_nCharacterCount != 2;
                bShow = bShow && bShift;
                nSlot = bShift ? 2 : 0;
            } else if (nI == 2) {
                nSlot = 0;
                bShow = bShow || pParty->m_nCharacterCount == 2;
            }
        } else if (pParty->m_nCharacterCount == 2) {
            if (nI == 1) {
                nSlot = 2;
                bShow = false;
            } else if (nI == 2) {
                nSlot = 2;
                bShow = true;
            }
        }
        int nState = 0;
        int nEffA = 0;
        int nEffB = 0;
        if (pServer != 0) {
            m_aCharacters[nSlot].m_sFirstName = pChar->m_pStats->GetFirstName();
            m_aCharacters[nSlot].m_sLastName = pChar->m_pStats->GetLastName();
            m_aCharacters[nSlot].m_aLabels[4].m_border.m_params.SetFillImage(pChar->GetPortrait(1), 0);
            int nCurHP = pServer->GetCurrentHitPoints(0);
            int nMaxHP = pServer->GetMaxHitPoints(1);
            int bPoisoned = pServer->m_bPoisoned;
            m_aCharacters[nSlot].m_aBars[0].SetMaxValue(nMaxHP);
            m_aCharacters[nSlot].m_aBars[0].SetCurValue(nCurHP);
            m_aCharacters[nSlot].m_aBars[0].m_borderProgress.m_params.m_color = bPoisoned ?
                reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_GREEN) : reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_RED);
            m_aCharacters[nSlot].m_nMaxHP = nMaxHP;
            m_aCharacters[nSlot].m_nCurHP = nCurHP;
            m_aCharacters[nSlot].m_bToolTipDirty = 1;
            short nFPBonus = pServer->m_pStats->m_nTemporaryForcePoints;
            short nFPCur = pServer->m_pStats->m_nCurrentForcePoints;
            short nMaxFP = pServer->GetMaxForcePoints();
            short nFPCurTot = nFPBonus + nFPCur;
            if (nMaxFP == 0) {
                m_aCharacters[nSlot].m_aBars[1].SetMaxValue(1);
                m_aCharacters[nSlot].m_aBars[1].SetCurValue(0);
            } else {
                m_aCharacters[nSlot].m_aBars[1].SetMaxValue(nMaxFP);
                m_aCharacters[nSlot].m_aBars[1].SetCurValue(nFPCurTot);
            }
            m_aCharacters[nSlot].m_nMaxFP = nMaxFP;
            m_aCharacters[nSlot].m_nCurFP = nFPCurTot;
            m_aCharacters[nSlot].m_bToolTipDirty = 1;
            m_aCharacters[nSlot].m_nLevel = pServer->m_pStats->GetLevel(0) & 0xff;
            m_aCharacters[nSlot].m_bToolTipDirty = 1;
            int nExpDiff;
            if (static_cast<int>(pServer->m_pStats->GetExpNeededForLevelUp() - pServer->m_pStats->m_nExperience) < 1)
                nExpDiff = 0;
            else
                nExpDiff = pServer->m_pStats->GetExpNeededForLevelUp() - pServer->m_pStats->m_nExperience;
            m_aCharacters[nSlot].m_nXPToLevel = nExpDiff;
            m_aCharacters[nSlot].m_bToolTipDirty = 1;
            if (pServer->GetDead()) {
                nState = 1;
            } else if (pServer->m_bInParty && pServer->GetCurrentHitPoints(0) <= 0) {
                nState = 1;
            } else if (pServer->m_pStats->CanLevelUp()) {
                CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
                if (pOptions->m_bAutoLevelUpNPCs && pChar->m_objectId !=
                    g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_objectId &&
                    pServer->m_bPlayerMadeCharacter == 0) {
                    pServer->m_pStats->AutoLevelup(1);
                    nState = 0;
                } else {
                    if (pChar->m_nStateFlags & 2) {
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(
                            CGuiInGame::GUI_STATUS_LEVEL_UP_SOUND, 0);
                        pChar->m_nStateFlags &= ~2;
                    }
                    nState = 4;
                }
            } else if (pServer->IsDebilitated(0)) {
                if (nI == 0 && m_fCombatMessageTimer == -1.0f) {
                    m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
                    m_combatMessage = STRREF_PLAYER_DEBILITATED;
                    m_lblCombatModeMsg.m_text.m_params.SetText(
                        g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_PLAYER_DEBILITATED));
                    m_lblCombatModeMsg.m_text.m_params.m_fAlpha = 1.0f;
                }
                nState = 2;
            } else {
                int nGood = 0;
                int nBad = 0;
                for (int nE = 0; nE < pServer->m_effectIcons.m_count; nE++) {
                    CEffectIconObject* pEffect = pServer->m_effectIcons.m_pArray[nE];
                    if (pEffect->m_nGood != 0) {
                        if (pEffect->m_bGood != 0)
                            nGood++;
                        else
                            nBad++;
                    }
                }
                nEffA = nGood;
                nEffB = nBad;
            }
            if (pServer->IsDebilitated(0) == 0 && m_combatMessage == STRREF_PLAYER_DEBILITATED &&
                m_fCombatMessageTimer == -1.0f) {
                m_lblCombatModeMsg.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
                m_combatMessage = STRREF_COMBAT_MODE_ENGAGED;
                m_lblCombatModeMsg.m_text.m_params.SetText(
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_COMBAT_MODE_ENGAGED));
                m_lblCombatModeMsg.m_text.m_params.m_fAlpha = 1.0f;
            }
            m_aCharacters[nSlot].m_aLabels[4].m_tail.bVisible = 1;
            m_aCharacters[nSlot].m_aLabels[0].m_tail.bVisible = 1;
            m_aCharacters[nSlot].m_aBars[0].m_tail.bVisible = 1;
            m_aCharacters[nSlot].m_aBars[1].m_tail.bVisible = 1;
            m_aCharacters[nSlot].m_aLabels[1].m_tail.bVisible = (nState & 2) != 0;
            m_aCharacters[nSlot].m_aLabels[2].m_tail.bVisible = (nState & 1) != 0;
            m_aCharacters[nSlot].m_aLabels[3].m_tail.bVisible = (nState & 4) != 0;
            m_aCharacters[nSlot].m_nGoodEffectCount = static_cast<unsigned char>(nState == 0 ? nEffA : 0);
            m_aCharacters[nSlot].m_nBadEffectCount = static_cast<unsigned char>(nState == 0 ? nEffB : 0);
            m_aCharacters[nSlot].m_btnChar.m_tail.bVisible = 1;
            if (nSlot == 0 && m_fRotationTimer > 0.0f) {
                m_aCharacters[3].m_sFirstName = pChar->m_pStats->GetFirstName();
                m_aCharacters[3].m_sLastName = pChar->m_pStats->GetLastName();
                m_aCharacters[3].m_aLabels[4].m_border.m_params.SetFillImage(pChar->GetPortrait(1), 0);
                int nCurHPMirror = pServer->GetCurrentHitPoints(0);
                int nMaxHPMirror = pServer->GetMaxHitPoints(1);
                int bPoisonedMirror = pServer->m_bPoisoned;
                m_aCharacters[3].m_aBars[0].SetMaxValue(nMaxHPMirror);
                m_aCharacters[3].m_aBars[0].SetCurValue(nCurHPMirror);
                m_aCharacters[3].m_aBars[0].m_borderProgress.m_params.m_color = bPoisonedMirror ?
                    reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_GREEN) : reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_RED);
                m_aCharacters[3].m_nCurHP = nCurHPMirror;
                m_aCharacters[3].m_nMaxHP = nMaxHPMirror;
                m_aCharacters[3].m_bToolTipDirty = 1;
                short nFPBonusMirror = pServer->m_pStats->m_nTemporaryForcePoints;
                short nFPCurMirror = pServer->m_pStats->m_nCurrentForcePoints;
                short nMaxFPMirror = pServer->GetMaxForcePoints();
                short nFPCurTotMirror = nFPBonusMirror + nFPCurMirror;
                if (nMaxFPMirror == 0) {
                    m_aCharacters[3].m_aBars[1].SetMaxValue(1);
                    m_aCharacters[3].m_aBars[1].SetCurValue(0);
                } else {
                    m_aCharacters[3].m_aBars[1].SetMaxValue(nMaxFPMirror);
                    m_aCharacters[3].m_aBars[1].SetCurValue(nFPCurTotMirror);
                }
                m_aCharacters[3].m_nCurFP = nFPCurTotMirror;
                m_aCharacters[3].m_nMaxFP = nMaxFPMirror;
                m_aCharacters[3].m_bToolTipDirty = 1;
                m_aCharacters[3].m_nLevel = pServer->m_pStats->GetLevel(0) & 0xff;
                m_aCharacters[3].m_bToolTipDirty = 1;
                int nExpDiffMirror;
                if (static_cast<int>(pServer->m_pStats->GetExpNeededForLevelUp() - pServer->m_pStats->m_nExperience) < 1)
                    nExpDiffMirror = 0;
                else
                    nExpDiffMirror = pServer->m_pStats->GetExpNeededForLevelUp() - pServer->m_pStats->m_nExperience;
                m_aCharacters[3].m_nXPToLevel = nExpDiffMirror;
                m_aCharacters[3].m_bToolTipDirty = 1;
                m_aCharacters[3].m_aLabels[4].m_tail.bVisible = 1;
                m_aCharacters[3].m_aLabels[0].m_tail.bVisible = 1;
                m_aCharacters[3].m_aBars[0].m_tail.bVisible = 1;
                m_aCharacters[3].m_aBars[1].m_tail.bVisible = 1;
                m_aCharacters[3].m_aLabels[1].m_tail.bVisible = (nState & 2) != 0;
                m_aCharacters[3].m_aLabels[2].m_tail.bVisible = (nState & 1) != 0;
                m_aCharacters[3].m_aLabels[3].m_tail.bVisible = (nState & 4) != 0;
                m_aCharacters[3].m_nGoodEffectCount = static_cast<unsigned char>(nState == 0 ? nEffA : 0);
                m_aCharacters[3].m_nBadEffectCount = static_cast<unsigned char>(nState == 0 ? nEffB : 0);
                m_aCharacters[3].m_btnChar.m_tail.bVisible = 1;
            }
        }
        if (!bShow) {
            m_aCharacters[nI].m_aLabels[4].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aLabels[0].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aBars[0].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aBars[1].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aLabels[1].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aLabels[2].m_tail.bVisible = 0;
            m_aCharacters[nI].m_aLabels[3].m_tail.bVisible = 0;
            m_aCharacters[nI].m_nGoodEffectCount = 0;
            m_aCharacters[nI].m_nBadEffectCount = 0;
            m_aCharacters[nI].m_btnChar.m_tail.bVisible = 0;
        }
        if (nI == 2)
            break;
    }
}
#endif

CSWGuiMainInterface::~CSWGuiMainInterface() {
    delete m_pFogOfWarImage;
}

bool AspyrUseExtendedController()
{
    return true;
}


#if KOTOR1
// K1: a sixth plate (LBL_LVLUPBG); every widget is re-anchored to the bottom-right HUD unit; the
// bar anchor is unscaled; no mouse enter/exit; the back plate takes a per-slot "ios" frame.
void CSWGuiMainInterfaceChar::Initialize(CSWGuiMainInterface* pInterface, int nSlot)
{
    CExoString sName;
    sName.Format(const_cast<char*>("LBL_CHAR%d"), nSlot);
    pInterface->InitControl(m_aLabels[5], sName, 1);
    sName.Format(const_cast<char*>("PB_VIT%d"), nSlot);
    pInterface->InitControl(m_aBars[0], sName, 1);
    sName.Format(const_cast<char*>("PB_FORCE%d"), nSlot);
    pInterface->InitControl(m_aBars[1], sName, 1);
    sName.Format(const_cast<char*>("LBL_LEVELUP%d"), nSlot);
    pInterface->InitControl(m_aLabels[3], sName, 1);
    sName.Format(const_cast<char*>("LBL_LVLUPBG%d"), nSlot);
    pInterface->InitControl(m_aLabels[4], sName, 1);
    sName.Format(const_cast<char*>("LBL_BACK%d"), nSlot);
    pInterface->InitControl(m_aLabels[0], sName, 1);
    sName.Format(const_cast<char*>("LBL_DEBILATATED%d"), nSlot);
    pInterface->InitControl(m_aLabels[1], sName, 1);
    sName.Format(const_cast<char*>("LBL_DISABLED%d"), nSlot);
    pInterface->InitControl(m_aLabels[2], sName, 1);
    sName.Format(const_cast<char*>("LBL_CMBTEFCTINC%d"), nSlot);
    pInterface->InitControl(m_lblCombatEffectInc, sName, 0);
    sName.Format(const_cast<char*>("LBL_CMBTEFCTRED%d"), nSlot);
    pInterface->InitControl(m_lblCombatEffectRed, sName, 0);
    sName.Format(const_cast<char*>("BTN_CHAR%d"), nSlot);
    pInterface->InitControl(m_btnChar, sName, 1);
    m_aLabels[3].m_border.m_params.SetPulsingAlpha(1, 0, 1);

    CSWGuiExtent portrait(0, 0, 0, 0);
    ShiftControlExtent(m_aBars[1], 1, -1);
    ShiftControlExtent(m_aLabels[3], 1, -1);
    ShiftControlExtent(m_aLabels[4], 1, -1);
    ShiftControlExtent(m_aLabels[1], 1, -1);
    ShiftControlExtent(m_aLabels[2], 1, -1);
    ShiftControlExtent(m_btnChar, 1, -1);
    CSWGuiControl* pPortrait = &m_aLabels[5];
    portrait = pPortrait->m_extent;
    AndroidExtentShift(portrait, 1, -1);
    pPortrait->SetExtent(portrait);
#ifdef K1_UI_XBOX
    // xbox: `maininterface` has no BTN_CHAR%d, so the portrait button takes the portrait's own
    // rectangle (no art of its own) and joins the panel's list, which InitControl only does for a
    // tag the layout has: tapping or clicking a portrait then works as on the touch HUD.
    if (m_btnChar.m_extent.w <= 0 || m_btnChar.m_extent.h <= 0) {
        m_btnChar.SetExtent(portrait);
        bool bListed = false;
        for (int i = 0; i < pInterface->m_controls.m_count; i++)
            if (pInterface->m_controls.m_pArray[i] == &m_btnChar)
                bListed = true;
        if (!bListed)
            pInterface->AddControl(&m_btnChar);
    }
#endif

    CSWGuiExtent extent = m_lblCombatEffectInc.m_extent;
    AndroidExtentShift(extent, 1, -1);
    extent.x -= portrait.x;
    extent.y -= portrait.y;
    m_lblCombatEffectInc.SetExtent(extent);
    extent = m_lblCombatEffectRed.m_extent;
    AndroidExtentShift(extent, 1, -1);
    extent.x -= portrait.x;
    extent.y -= portrait.y;
    m_lblCombatEffectRed.SetExtent(extent);

    CSWGuiControl* pBar = &m_aBars[0];
    extent = pBar->m_extent;
    AndroidExtentShift(extent, 1, -1);
    pBar->SetExtent(extent);
    m_nScaledBarX = static_cast<short>(extent.x);
    m_btnChar.m_tail.m_nUserData = nSlot;
    m_nScaledBarY = static_cast<short>(extent.y);

    m_btnChar.m_tail.bFocusOnHover = 0;
    m_btnChar.AddEvent(INPUTEVENT_GUI_A, pInterface, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnCharacterClicked));
    m_btnChar.AddEvent(INPUTEVENT_MOUSE_RIGHT, pInterface, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnCharacterRightClicked));

    m_btnChar.m_pChar = this;
    m_bToolTipDirty = 0;
    m_nCurFP = 0;
    m_nMaxFP = 0;
    m_nLevel = 0;
    m_nXPToLevel = 0;
    m_nCurHP = 0;
    m_nMaxHP = 0;

    CSWGuiControl* pBack = &m_aLabels[0];
    extent = pBack->m_extent;
#ifdef K2_UI_TOUCH  // pc / xbox: the layout's own portrait frames (lbl_miport1/3)
    switch (nSlot) {
    case 3:
        m_aLabels[0].m_border.m_params.SetFillImage(CResRef("ios_lbl_miport1"), 0);
        m_aLabels[0].m_border.m_params.m_flags.nFillStyle = 2;
        extent.h -= 1;
        break;
    case 2:
        m_aLabels[0].m_border.m_params.SetFillImage(CResRef("ios_lbl_miport3"), 0);
        m_aLabels[0].m_border.m_params.m_flags.nFillStyle = 2;
        extent.h -= 1;
        break;
    case 1:
        m_aLabels[0].m_border.m_params.SetFillImage(CResRef("ios_lbl_miport2"), 0);
        m_aLabels[0].m_border.m_params.m_flags.nFillStyle = 2;
        extent.y += 1;
        extent.w += 1;
        break;
    }
    extent.x -= 1;
#endif
    AndroidExtentShift(extent, 1, -1);
    pBack->SetExtent(extent);
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiMainInterfaceChar::Initialize(CSWGuiMainInterface* pInterface, int nSlot, int nFlag)
{
    CExoString sName;
    sName.Format(const_cast<char*>("LBL_CHAR%d"), nSlot);
#ifdef K2_UI_XBOX
    // console HUD (K2_UI_XBOX): read the layout's own coordinates first. bScale = 0 skips both the aspect
    // multipliers and the whole pc-layout block, so this is the .gui file's number, un-snapped.
    pInterface->InitControl(m_aLabels[4], sName, 0, 0, 0, 0, 0);
    const bool bParked = (m_aLabels[4].m_extent.x >= g_nGUIWidth ||
                          m_aLabels[4].m_extent.y >= g_nGUIHeight);
#endif
    pInterface->InitControl(m_aLabels[4], sName, 1, 1, nFlag, 0, 0);
    sName.Format(const_cast<char*>("PB_VIT%d"), nSlot);
    pInterface->InitControl(m_aBars[0], sName, 1, 1, nFlag, 0, 0);
    sName.Format(const_cast<char*>("PB_FORCE%d"), nSlot);
    pInterface->InitControl(m_aBars[1], sName, 1, 1, nFlag, 0, 0);
    sName.Format(const_cast<char*>("LBL_LEVELUP%d"), nSlot);
    pInterface->InitControl(m_aLabels[3], sName, 1, 1, nFlag, 0, 0);
    sName.Format(const_cast<char*>("LBL_BACK%d"), nSlot);
    pInterface->InitControl(m_aLabels[0], sName, 1, 1, nFlag, 0, 0);
    sName.Format(const_cast<char*>("LBL_DEBILATATED%d"), nSlot);
    pInterface->InitControl(m_aLabels[1], sName, 1, 1, nFlag, 0, 0);
    sName.Format(const_cast<char*>("LBL_DISABLED%d"), nSlot);
    pInterface->InitControl(m_aLabels[2], sName, 1, 1, nFlag, 0, 0);
    sName.Format(const_cast<char*>("LBL_CMBTEFCTINC%d"), nSlot);
    pInterface->InitControl(m_lblCombatEffectInc, sName, 0, 1, nFlag, 0, 0);
    sName.Format(const_cast<char*>("LBL_CMBTEFCTRED%d"), nSlot);
    pInterface->InitControl(m_lblCombatEffectRed, sName, 0, 1, nFlag, 0, 0);
    sName.Format(const_cast<char*>("BTN_CHAR%d"), nSlot);
    pInterface->InitControl(m_btnChar, sName, 1, 1, nFlag, 0, 0);
#ifdef K2_UI_XBOX
    if ((m_btnChar.m_extent.w <= 0 || m_btnChar.m_extent.h <= 0) && m_aLabels[4].m_extent.w > 0) {
        m_btnChar.SetExtent(m_aLabels[4].m_extent);
        bool bListed = false;
        for (int i = 0; i < pInterface->m_controls.m_count; i++)
            if (pInterface->m_controls.m_pArray[i] == &m_btnChar)
                bListed = true;
        if (!bListed)
            pInterface->AddControl(&m_btnChar);
    }
#endif

    m_aLabels[3].m_border.m_params.SetPulsingAlpha(1, 0, 1);

    CSWGuiExtent portrait = m_aLabels[4].m_extent;
    CSWGuiExtent extent = m_lblCombatEffectInc.m_extent;
    extent.x -= portrait.x;
    extent.y -= portrait.y;
    m_lblCombatEffectInc.SetExtent(extent);

    extent = m_lblCombatEffectRed.m_extent;
    extent.x -= portrait.x;
    extent.y -= portrait.y;
    m_lblCombatEffectRed.SetExtent(extent);

    extent = m_aBars[0].m_extent;
    m_nScaledBarX = static_cast<short>(pInterface->m_scaleX * extent.x);
    m_nScaledBarY = static_cast<short>(pInterface->m_scaleY * extent.y);
    m_btnChar.m_tail.m_nUserData = nSlot;

    m_btnChar.m_tail.bFocusOnHover = 0;
    m_btnChar.AddEvent(INPUTEVENT_GUI_A, pInterface, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnCharacterClicked));
    m_btnChar.AddEvent(INPUTEVENT_MOUSE_RIGHT, pInterface, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnCharacterRightClicked));
    m_btnChar.AddEvent(SWGUIEVENT_FOCUS, pInterface, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnCharacterEntered));
    m_btnChar.AddEvent(SWGUIEVENT_UNFOCUS, pInterface, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMainInterface::OnCharacterExited));

    m_btnChar.m_pChar = this;
    m_bToolTipDirty = 0;
    m_nCurFP = 0;
    m_nMaxFP = 0;
    m_nLevel = 0;
    m_nXPToLevel = 0;
    m_nCurHP = 0;
    m_nMaxHP = 0;
#ifdef K2_UI_XBOX

    // console HUD (K2_UI_XBOX): the parked-slot guard. Collapse every widget the slot owns to a
    // zero extent, which is exactly the state a slot whose tags are absent from the layout already
    // has -- and which every PC layout proves is invisible.
    if (bParked) {
        CSWGuiExtent kHidden(0, 0, 0, 0);
        for (int i = 0; i < 5; i++)
            m_aLabels[i].SetExtent(kHidden);
        m_aBars[0].SetExtent(kHidden);
        m_aBars[1].SetExtent(kHidden);
        m_lblCombatEffectInc.SetExtent(kHidden);
        m_lblCombatEffectRed.SetExtent(kHidden);
        m_btnChar.SetExtent(kHidden);
    }
#endif
}
#endif // !KOTOR1

void CSWGuiMainInterfaceChar::Show(int bVisible, unsigned char bDisabled, unsigned char a, unsigned char b)
{
    m_aLabels[0].m_tail.bVisible = bVisible;
    m_aLabels[1].m_tail.bVisible = bVisible && (bDisabled & 2);
    m_aLabels[2].m_tail.bVisible = bVisible && (bDisabled & 1);
    m_aLabels[3].m_tail.bVisible = bVisible && (bDisabled & 4);
#if KOTOR1
    m_aLabels[4].m_tail.bVisible = bVisible && (bDisabled & 4);
    m_aLabels[5].m_tail.bVisible = bVisible;
#else
    m_aLabels[4].m_tail.bVisible = bVisible;
#endif
    m_aBars[0].m_tail.bVisible = bVisible;
    m_aBars[1].m_tail.bVisible = bVisible;
    m_btnChar.m_tail.bVisible = bVisible;
    m_nGoodEffectCount = bDisabled ? 0 : a;
    m_nBadEffectCount = bDisabled ? 0 : b;
}

void CSWGuiMainInterfaceChar::SetupGoto(int a, int b, CSWGuiManager* pGuiManager)
{
#if KOTOR1 && defined(K1_UI_XBOX)
    // xbox: the insets are in the console layout's 640x480 units, and AndroidExtentShift draws
    // that layout at screen height / 480 pinned to the right edge, so the insets scale the same way.
    {
        float fUnit = AndroidHudScaleFactor() * (1024.0f / 640.0f);
        int nW = pGuiManager->m_nScreenWidth;
        int nH = pGuiManager->m_nScreenHeight;
        switch (a) {
        case 0:
            m_nTargetBarX = m_nScaledBarX;
            m_nTargetBarY = nH;
            break;
        case 1:
            m_nTargetBarX = nW - static_cast<int>(fUnit * 128);
            m_nTargetBarY = nH - static_cast<int>(fUnit * 105);
            break;
        case 2:
            m_nTargetBarX = nW - static_cast<int>(fUnit * (b == 2 ? 128 : 117));
            m_nTargetBarY = nH - static_cast<int>(fUnit * (b == 2 ? 105 : 141));
            break;
        case 3:
            m_nTargetBarX = nW - static_cast<int>(fUnit * 106);
            m_nTargetBarY = m_nScaledBarY;
            break;
        }
        return;
    }
#endif
    switch (a) {
    case 0:
        m_nTargetBarX = m_nScaledBarX;
        m_nTargetBarY = pGuiManager->m_nScreenHeight;
        break;
    case 1:
        m_nTargetBarX = pGuiManager->m_nScreenWidth - 128;
        m_nTargetBarY = pGuiManager->m_nScreenHeight - 105;
        break;
    case 2:
        if (b == 2) {
            m_nTargetBarX = pGuiManager->m_nScreenWidth - 128;
            m_nTargetBarY = pGuiManager->m_nScreenHeight - 105;
        } else {
            m_nTargetBarX = pGuiManager->m_nScreenWidth - 117;
            m_nTargetBarY = pGuiManager->m_nScreenHeight - 141;
        }
        break;
    case 3:
        m_nTargetBarX = pGuiManager->m_nScreenWidth - 106;
        m_nTargetBarY = m_nScaledBarY;
        break;
    }
}

void CSWGuiMainInterfaceChar::Adjust(float f)
{
    // Seeded from the bar itself: its origin is what the delta is measured against.
    CSWGuiExtent extent = m_aBars[0].m_extent;
    float fT = 1.0f - 4.0f * f;
    int nDX = static_cast<int>(static_cast<float>(m_nTargetBarX - m_nScaledBarX) * fT + static_cast<float>(m_nScaledBarX)
                    - static_cast<float>(extent.x));
    int nDY = static_cast<int>(static_cast<float>(m_nTargetBarY - m_nScaledBarY) * fT + static_cast<float>(m_nScaledBarY)
                    - static_cast<float>(extent.y));

    extent = m_aLabels[0].m_extent;
    extent.x += nDX;
    extent.y += nDY;
    m_aLabels[0].SetExtent(extent);

    extent = m_aLabels[1].m_extent;
    extent.x += nDX;
    extent.y += nDY;
    m_aLabels[1].SetExtent(extent);

    extent = m_aLabels[2].m_extent;
    extent.x += nDX;
    extent.y += nDY;
    m_aLabels[2].SetExtent(extent);

    extent = m_aLabels[3].m_extent;
    extent.x += nDX;
    extent.y += nDY;
    m_aLabels[3].SetExtent(extent);

    extent = m_aLabels[4].m_extent;
    extent.x += nDX;
    extent.y += nDY;
    m_aLabels[4].SetExtent(extent);
#if KOTOR1
    extent = m_aLabels[5].m_extent;
    extent.x += nDX;
    extent.y += nDY;
    m_aLabels[5].SetExtent(extent);
#endif

    extent = m_aBars[0].m_extent;
    extent.x += nDX;
    extent.y += nDY;
    m_aBars[0].SetExtent(extent);

    extent = m_aBars[1].m_extent;
    extent.x += nDX;
    extent.y += nDY;
    m_aBars[1].SetExtent(extent);

    extent = m_btnChar.m_extent;
    extent.x += nDX;
    extent.y += nDY;
    m_btnChar.SetExtent(extent);
}

void CSWGuiMainInterfaceChar::DrawEffects(float f, int a)
{
#if KOTOR1
    if (!m_aLabels[5].m_tail.bVisible)
        return;
#else
    if (!m_aLabels[4].m_tail.bVisible)
        return;
#endif
    if (m_nGoodEffectCount)
        DrawStack(f, a, 1);
    if (m_nBadEffectCount)
        DrawStack(f, a, 0);
}

float GetResolutionTextScalar();

void CSWGuiMainInterfaceChar::DrawStack(float f, int a, int b)
{
#if KOTOR1
    CSWGuiExtent plateExtent = m_aLabels[5].m_extent;
    AurGUIStartLayer();
    if (!AurGUISetupViewport(plateExtent.x, plateExtent.y, plateExtent.w, plateExtent.h,
                             CSWGuiDrawable::NO_COLOURING, false, 1.0f)) {
#else
    CSWGuiExtent plateExtent = m_aLabels[4].m_extent;
    bool bCenterOnNative = m_aLabels[4].m_bCenterOnNativeRect;
    AurGUIStartLayer();
    if (!AurGUISetupViewport((bCenterOnNative ? g_nScreenLeftOffset : 0) + plateExtent.x,
                             (bCenterOnNative ? g_nScreenTopOffset : 0) + plateExtent.y,
                             plateExtent.w, plateExtent.h,
                             CSWGuiDrawable::NO_COLOURING, false, 1.0f)) {
#endif
        AurGUIStopLayer();
        return;
    }

    CSWGuiLabel& label = b ? m_lblCombatEffectInc : m_lblCombatEffectRed;
    unsigned char nFlag = b ? m_nGoodEffectCount : m_nBadEffectCount;
    int nCount = a ? (nFlag < 9 ? nFlag : 9) : (nFlag < 5 ? nFlag : 5);

    CSWGuiExtent original = label.m_extent;
    CSWGuiExtent extent = label.m_extent;

    int nStep;
#if KOTOR1
#ifdef K2_FIX_ENGINE_BUGS
    // The arrows are drawn at the HUD's scale, so their steps and their start inside the plate are
    // too (at 2160 lines a 5-pixel step stacked three arrows into one).
    float fStackScale = AndroidHudScaleFactor();
#endif
    if (nCount < 5) {
        nStep = a ? 10 : 5;
#ifdef K2_FIX_ENGINE_BUGS
        nStep = static_cast<int>(static_cast<float>(nStep) * fStackScale);
#endif
        int nSpan = (nCount - 1) * (static_cast<unsigned>(nStep) >> 1);
        extent.y = original.y + (b ? -nSpan : nSpan);
    } else {
#ifdef K2_FIX_ENGINE_BUGS
        extent.y = b ? static_cast<int>(fStackScale) : (plateExtent.h - static_cast<int>((a ? 17.0f : 9.0f) * fStackScale));
#else
        extent.y = b ? 1 : (plateExtent.h + (a ? -17 : -9));
#endif
        nStep = 5;
        if (a) {
            switch (nCount) {
            case 5: nStep = 10; break;
            case 6: nStep = 8; break;
            case 7: nStep = 7; break;
            case 8: nStep = 6; break;
            case 9: break;
            default: nStep = 0; break;
            }
        }
#ifdef K2_FIX_ENGINE_BUGS
        nStep = static_cast<int>(static_cast<float>(nStep) * fStackScale);
#endif
    }
#else
    if (nCount < 5) {
        nStep = static_cast<int>(GetResolutionTextScalar() * (a ? 10.0f : 5.0f));
        int nSpan = (nCount - 1) * (static_cast<unsigned>(nStep) >> 1);
        extent.y = original.y + (b ? -nSpan : nSpan);
    } else {
        extent.y = b ? 1 : (plateExtent.h + (a ? -17 : -9));
        float fy = 5.0f;
        if (a) {
            switch (nCount) {
            case 5: fy = 10.0f; break;
            case 6: fy = 8.0f; break;
            case 7: fy = 7.0f; break;
            case 8: fy = 6.0f; break;
            case 9: break;
            default: fy = 0.0f; break;
            }
        }
        nStep = static_cast<int>(fy * GetResolutionTextScalar());
    }
#endif

    unsigned char nSeven = static_cast<unsigned char>(nCount - 7);

    for (int i = 0; i < nCount; i++) {
        label.SetExtent(extent);
        label.Draw(f);
        extent.y += b ? nStep : -nStep;
        if ((i & 0x7ffffffd) == 0 && nSeven < 2)
            extent.y += b ? -1 : 1;
    }

    label.SetExtent(original);
    AurGUICloseViewport();
    AurGUIStopLayer();
}


extern CTlkTable* g_pTlkTable;

void CSWGuiCharButton::SetCharPointer(CSWGuiMainInterfaceChar* pChar)
{
    m_pChar = pChar;
}

int CSWGuiCharButton::DisplayToolTip()
{
    if (!m_tail.bVisible)
        return 0;
    UpdateToolTipText();
    g_pGuiMan->DisplayToolTip(m_sToolTipText);
    m_tail.bToolTipShown = 1;
    return 1;
}

void CSWGuiCharButton::Draw(float f)
{
    if (m_tail.bToolTipShown) {
        if (m_pChar && m_pChar->m_bToolTipDirty) {
            UpdateToolTipText();
            g_pGuiMan->ChangeToolTipText(m_sToolTipText);
            m_pChar->m_bToolTipDirty = 0;
        }
    }
    CSWGuiButton::Draw(f);
}

void CSWGuiCharButton::UpdateToolTipText()
{
    if (!m_pChar)
        return;
#if KOTOR1
    if (m_pChar->m_nMaxFP) {
        m_sToolTipText.Format(const_cast<char*>("%s %s\n%s: %d/%d\n%s: %d/%d\n%s: %d\n%s: %d"),
                              m_pChar->m_sFirstName.CStr(), m_pChar->m_sLastName.CStr(),
                              g_pTlkTable->GetSimpleString(STRREF_VITALITY).CStr(),
                              m_pChar->m_nCurHP, m_pChar->m_nMaxHP,
                              g_pTlkTable->GetSimpleString(STRREF_FORCE).CStr(),
                              m_pChar->m_nCurFP, m_pChar->m_nMaxFP,
                              g_pTlkTable->GetSimpleString(STRREF_LEVEL).CStr(), m_pChar->m_nLevel,
                              g_pTlkTable->GetSimpleString(STRREF_XP_NEEDED).CStr(), m_pChar->m_nXPToLevel);
    } else {
        m_sToolTipText.Format(const_cast<char*>("%s %s\n%s: %d/%d\n%s: %d\n%s: %d"),
                              m_pChar->m_sFirstName.CStr(), m_pChar->m_sLastName.CStr(),
                              g_pTlkTable->GetSimpleString(STRREF_VITALITY).CStr(),
                              m_pChar->m_nCurHP, m_pChar->m_nMaxHP,
                              g_pTlkTable->GetSimpleString(STRREF_LEVEL).CStr(), m_pChar->m_nLevel,
                              g_pTlkTable->GetSimpleString(STRREF_XP_NEEDED).CStr(), m_pChar->m_nXPToLevel);
    }
#else

    if (m_pChar->m_nMaxFP) {
        m_sToolTipText.Format(const_cast<char*>("%s %s\n%s: %d/%d\n%s: %d/%d\n%s: %d\n%s: %d"),
                              m_pChar->m_sFirstName.CStr(), m_pChar->m_sLastName.CStr(),
                              g_pTlkTable->GetSimpleString(STRREF_VITALITY).CStr(),
                              m_pChar->m_nCurHP, m_pChar->m_nMaxHP,
                              g_pTlkTable->GetSimpleString(STRREF_FORCE).CStr(),
                              m_pChar->m_nCurFP, m_pChar->m_nMaxFP,
                              g_pTlkTable->GetSimpleString(STRREF_LEVEL).CStr(), m_pChar->m_nLevel,
                              g_pTlkTable->GetSimpleString(STRREF_XP_NEEDED).CStr(), m_pChar->m_nXPToLevel);
    } else {
        m_sToolTipText.Format(const_cast<char*>("%s %s\n%s: %d/%d\n%s: %d\n%s: %d"),
                              m_pChar->m_sFirstName.CStr(), m_pChar->m_sLastName.CStr(),
                              g_pTlkTable->GetSimpleString(STRREF_VITALITY).CStr(),
                              m_pChar->m_nCurHP, m_pChar->m_nMaxHP,
                              g_pTlkTable->GetSimpleString(STRREF_LEVEL).CStr(), m_pChar->m_nLevel,
                              g_pTlkTable->GetSimpleString(STRREF_XP_NEEDED).CStr(), m_pChar->m_nXPToLevel);
    }
#endif
}
