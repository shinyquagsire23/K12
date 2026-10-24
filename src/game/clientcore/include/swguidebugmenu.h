#pragma once

#ifndef CSWGUIDEBUGMENU_H
#define CSWGUIDEBUGMENU_H

#include "swgui.h"
#include "swguicontrol.h"

class CExoString;
class CSWGuiButton;
class CSWGuiControl;
class CSWGuiObject;

// In-game developer/QA cheat menu, a scrolling list of debug buttons whose handlers live in
// swguidebugmenu.cpp.
class CSWGuiDebugMenu : public CSWGuiPanel {
public:

#if KOTOR1
    CSWGuiDebugMenu(CSWGuiManager* pGuiManager);
#else
    CSWGuiDebugMenu(CSWGuiManager* pGuiManager, int bHideQaOptions);
#endif

    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();
    virtual void OnPanelRemoved();

    CSWGuiListBox m_listBox;
    CSWGuiLabel   m_label;
#if !KOTOR1  // K1 has no QA gate
    int  m_bHideQaOptions;
#endif

#if KOTOR1
    CSWGuiButton* CreateOption(const CExoString& sText,
                               void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                               CSWGuiButton* pTemplate);
#else
    CSWGuiButton* CreateOption(const CExoString& sText,
                               void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                               CSWGuiButton* pTemplate, int bAlways);
#endif

    void TurboOpenKillAll(CSWGuiControl* pControl);
    void KillAllEnemies(CSWGuiControl* pControl);
    void PartyDebugMenu(CSWGuiControl* pControl);
    void OpenAllDoors(CSWGuiControl* pControl);
    void Heal(CSWGuiControl* pControl);
    void QAAllPowersFeatsSkills(CSWGuiControl* pControl);
    void QAWarp(CSWGuiControl* pControl);
    void ToggleQADisplay(CSWGuiControl* pControl);
    void ToggleScreenshotMode(CSWGuiControl* pControl);
    void TogglePartyBar(CSWGuiControl* pControl);
    void ToggleNavigateCamera(CSWGuiControl* pControl);
    void AddExperience1(CSWGuiControl* pControl);
    void AddExperience2(CSWGuiControl* pControl);
    void AddExperience3(CSWGuiControl* pControl);
    void LogConversations(CSWGuiControl* pControl);
    void ToggleTurboSpeed(CSWGuiControl* pControl);
    void CreateItem(CSWGuiControl* pControl);
    void ChangeFormation(CSWGuiControl* pControl);
    void ToggleDamage(CSWGuiControl* pControl);
    void ToggleAllDamage(CSWGuiControl* pControl);
    void ToggleTracing(CSWGuiControl* pControl);
    void ToggleSafeRegion(CSWGuiControl* pControl);
    void ToggleObjectIndicator(CSWGuiControl* pControl);
    void ToggleGrass(CSWGuiControl* pControl);
    void ToggleAABB(CSWGuiControl* pControl);
    void RestartMinigame(CSWGuiControl* pControl);
    void LogHeapWalk(CSWGuiControl* pControl);
    void PrintHeapWalk(CSWGuiControl* pControl);
    void ToggleMemTrack(CSWGuiControl* pControl);
    void ToggleMemTrackStats(CSWGuiControl* pControl);
    void KillActiveCharacter(CSWGuiControl* pControl);
    void BrightSunAndMoon(CSWGuiControl* pControl);
    void SpawnCheatBot(CSWGuiControl* pControl);
    void ToggleResourceLoads(CSWGuiControl* pControl);
    void ShowTestPattern(CSWGuiControl* pControl);
    void WarpForward(CSWGuiControl* pControl);
    void ToggleCombatDebugging(CSWGuiControl* pControl);
    void TogglePartyFollowLogging(CSWGuiControl* pControl);
    void TestFunction(CSWGuiControl* pControl);
    void ToggleShowStringRefs(CSWGuiControl* pControl);
    void ToggleFogOfWar(CSWGuiControl* pControl);
    void ToggleQADisplayAllTriggers(CSWGuiControl* pControl);
    void IncreaseAlignment(CSWGuiControl* pControl);
    void DecreaseAlignment(CSWGuiControl* pControl);
    void ShowAllMovies(CSWGuiControl* pControl);
    void EnableClairvoyanceEffect(CSWGuiControl* pControl);
    void EnableFuryEffect(CSWGuiControl* pControl);
    void DisableVideoEffects(CSWGuiControl* pControl);
    void ToggleForceSight(CSWGuiControl* pControl);
    void ToggleHologram(CSWGuiControl* pControl);
    void TakeScreenShot(CSWGuiControl* pControl);
    void IncreaseStrength(CSWGuiControl* pControl);
    void IncreaseIntelligence(CSWGuiControl* pControl);
    void IncreaseWisdom(CSWGuiControl* pControl);
    void IncreaseDexterity(CSWGuiControl* pControl);
    void IncreaseConstitution(CSWGuiControl* pControl);
    void IncreaseCharisma(CSWGuiControl* pControl);
    void DecreaseStrength(CSWGuiControl* pControl);
    void DecreaseIntelligence(CSWGuiControl* pControl);
    void DecreaseWisdom(CSWGuiControl* pControl);
    void DecreaseDexterity(CSWGuiControl* pControl);
    void DecreaseConstitution(CSWGuiControl* pControl);
    void DecreaseCharisma(CSWGuiControl* pControl);
    void RevanMale(CSWGuiControl* pControl);
    void RevanFemale(CSWGuiControl* pControl);
    void RevanGood(CSWGuiControl* pControl);
    void RevanEvil(CSWGuiControl* pControl);
    void ChangeGlobalVars(CSWGuiControl* pControl);
#if !KOTOR1
    void IncreaseInfluence(CSWGuiControl* pControl);
    void DecreaseInfluence(CSWGuiControl* pControl);
    void ShowInfluenceStats(CSWGuiControl* pControl);
#endif
    void Toggle_g_bExitProgram(CSWGuiControl* pControl);
};

#endif // CSWGUIDEBUGMENU_H

#ifndef CSWGUITESTPATTERN_H
#define CSWGUITESTPATTERN_H

#include "swgui.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "exoresman.h"

// The test-pattern overlay shown by the debug menu's test-pattern entry.
class CSWGuiTestPattern : public CSWGuiPanel {
public:
    virtual void HandleInputEvent(int type, int val)
    {
        CSWGuiPanel::HandleInputEvent(type, val);
        if (val)
        {
            g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
#if KOTOR1
            // K1 has no panel chain: one state-4 stamp.
            m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
            CSWGuiPanel* pPanel = this;
            do
            {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel != 0);
#endif
        }
    }

    CSWGuiTestPattern(CSWGuiManager* pGuiManager) : CSWGuiPanel(pGuiManager)
    {
#if KOTOR1
        // K1: the same layout, then a rescale for the resolution.
        StartLoadFromLayout(CResRef("ntscpat"));
        StopLoadFromLayout();
        ScaleExtentForResolution();
#else
        StartLoadFromLayout(CResRef("ntscpat_p"), 1, 1, 1);
        StopLoadFromLayout();
#endif
    }
};

#endif // CSWGUITESTPATTERN_H
