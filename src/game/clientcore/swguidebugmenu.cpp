// The developer and QA cheat menu's button callbacks. Each one self-dismisses the menu, gates on its
// event argument and flips a debug global.

#include "k2stdint.h"
#include "swguidebugmenu.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "guiingame.h"
#include "swguicontrol.h"
#include "clientoptions.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "nwchandlemessage.h"
#include "nwscreature.h"
#include "partytables.h"
#include "nwsarea.h"
#include "nwsdoor.h"
#include "globalvartables.h"
#include "nwccreature.h"
#include "AURORA/aurmodel.h"
#include "exoinput.h"
#include "exotlktable.h"
#include "swgui.h"
#include "exoresman.h"
#include "CExoArrayListImpl.h"
#include "exostring.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#include "nwscriptconst.h"
#include <stdarg.h>

extern int  RenderQA;
extern int  RenderQATriggers;
extern int  enablerendertriggers;
extern int  renderaabb;                    // flipped as `1 - x`, not `!x`
extern int  enablegrass;                   // also flipped as `1 - x`
extern bool g_bDisplayServicedResources;
extern int  FollowLeaderFullLogging;
extern int  g_bShowFogOfWar;
extern int  g_bDrawSafeRegion;
extern int  g_bLogConversations;
extern int  g_bShowIndicator;
extern int  g_bDisableDamage;
extern int  g_bExitProgram;
extern int  g_bInScreenshotMode;
extern int  g_bEnableAurPostString;
extern int  g_bEnableCombatDebugging;
int bTracesOn;

extern "C++" {
char* warpforward(char* s);
char* DebugMode(char* s);
char* trace(char* s);
char* untrace(char* s);
void AurEnableForceFury(short n);
}

extern CTlkTable* g_pTlkTable;

extern CExoString g_sBuildNumber;

#include "port/libcbase.h"
#include "nwcanimbase.h"
void AurPostString(char* pszString, int nX, int nY, float fLife);

#if !KOTOR1
static short s_nForceFuryEffect;
static bool  s_bHologramEnabled;
#endif

#if !KOTOR1
// The 100-byte bound is hard-coded, so callers pass char[100] buffers. The second parameter is never
// read.
static void DebugSprintf(char* pBuffer, void* pDead, const char* pFormat, ...)
{
    K2_UNUSED(pDead);
    va_list ap;
    va_start(ap, pFormat);
    __vsprintf_chk(pBuffer, 0, 100, pFormat, ap);
    va_end(ap);
}
#endif

// Loads the panel, stamps the build number and builds the whole 71-option list.

#if KOTOR1
// K1's menu: the debug layout scaled to the screen, with 38 options and no QA gate, so there are no
// Revan, attribute, alignment, movie, video-effect or influence rows.
CSWGuiDebugMenu::CSWGuiDebugMenu(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("debug"), 1, 1, 1);
    InitControl(m_listBox, CExoString("LB_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_label, CExoString("LBL_BUILD"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    ScaleExtentForResolution();
    CExoString sBuildString = CExoString("Build: ") + g_sBuildNumber;
    m_label.m_text.m_params.SetText(sBuildString);

    CExoArrayList<CSWGuiControl*> list;
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    list.SetSize(38);

    list.m_pArray[0] = CreateOption(CExoString("Heal"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::Heal), pTemplate);
    list.m_pArray[1] = CreateOption(CExoString("QA Display"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleQADisplay), pTemplate);
    list.m_pArray[2] = CreateOption(CExoString("QA - Display All Triggers"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleQADisplayAllTriggers), pTemplate);
    list.m_pArray[3] = CreateOption(CExoString("QA - Add any feat, power or skill"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::QAAllPowersFeatsSkills), pTemplate);
    list.m_pArray[4] = CreateOption(CExoString("QA - Warp"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::QAWarp), pTemplate);
    list.m_pArray[5] = CreateOption(CExoString("QA - Add 1000 experience"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::AddExperience1), pTemplate);
    list.m_pArray[6] = CreateOption(CExoString("QA - Add 5000 experience"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::AddExperience2), pTemplate);
    list.m_pArray[7] = CreateOption(CExoString("QA - Add 10000 experience"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::AddExperience3), pTemplate);
    list.m_pArray[8] = CreateOption(CExoString("QA - Toggle Log conversations"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::LogConversations), pTemplate);
    list.m_pArray[9] = CreateOption(CExoString("Turbo Speed"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleTurboSpeed), pTemplate);
    list.m_pArray[10] = CreateOption(CExoString("Create Item"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::CreateItem), pTemplate);
    list.m_pArray[11] = CreateOption(CExoString("Change Formation"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ChangeFormation), pTemplate);
    list.m_pArray[12] = CreateOption(CExoString("Toggle Invulnerability"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleDamage), pTemplate);
    list.m_pArray[13] = CreateOption(CExoString("Toggle All Damage"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleAllDamage), pTemplate);
    list.m_pArray[14] = CreateOption(CExoString("Toggle Engine Stats"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleTracing), pTemplate);
    list.m_pArray[15] = CreateOption(CExoString("Toggle Safe Region"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleSafeRegion), pTemplate);
    list.m_pArray[16] = CreateOption(CExoString("Toggle Object Indicator"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleObjectIndicator), pTemplate);
    list.m_pArray[17] = CreateOption(CExoString("Toggle Grass"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleGrass), pTemplate);
    list.m_pArray[18] = CreateOption(CExoString("Toggle AABB"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleAABB), pTemplate);
    list.m_pArray[19] = CreateOption(CExoString("Toggle Party Bar"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::TogglePartyBar), pTemplate);
    list.m_pArray[20] = CreateOption(CExoString("Restart Minigame"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::RestartMinigame), pTemplate);
    list.m_pArray[21] = CreateOption(CExoString("Log Heap Walk"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::LogHeapWalk), pTemplate);
    list.m_pArray[22] = CreateOption(CExoString("Print Heap Walk Stats"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::PrintHeapWalk), pTemplate);
    list.m_pArray[23] = CreateOption(CExoString("Mem Track to File"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleMemTrack), pTemplate);
    list.m_pArray[24] = CreateOption(CExoString("Mem Track Stats"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleMemTrackStats), pTemplate);
    list.m_pArray[25] = CreateOption(CExoString("KILL ME NOW!"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::KillActiveCharacter), pTemplate);
    list.m_pArray[26] = CreateOption(CExoString("Bright Sun and Moon"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::BrightSunAndMoon), pTemplate);
    list.m_pArray[27] = CreateOption(CExoString("Spawn Cheat Bot"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::SpawnCheatBot), pTemplate);
    list.m_pArray[28] = CreateOption(CExoString("Toggle Display Resource Loads"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleResourceLoads), pTemplate);
    list.m_pArray[29] = CreateOption(CExoString("Show Test Pattern"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ShowTestPattern), pTemplate);
    list.m_pArray[30] = CreateOption(CExoString("Warp Forward"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::WarpForward), pTemplate);
    list.m_pArray[31] = CreateOption(CExoString("Toggle Screenshot Mode"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleScreenshotMode), pTemplate);
    list.m_pArray[32] = CreateOption(CExoString("Toggle Combat Debugging"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleCombatDebugging), pTemplate);
    list.m_pArray[33] = CreateOption(CExoString("Toggle Party Follow Logging"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::TogglePartyFollowLogging), pTemplate);
    list.m_pArray[34] = CreateOption(CExoString("Toggle Flight Camera"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleNavigateCamera), pTemplate);
    list.m_pArray[35] = CreateOption(CExoString("DO NOT PUSH!!!"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::TestFunction), pTemplate);
    list.m_pArray[36] = CreateOption(CExoString("Toggle Show String Refs"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleShowStringRefs), pTemplate);
    list.m_pArray[37] = CreateOption(CExoString("Toggle Fog of War"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleFogOfWar), pTemplate);

    m_listBox.AddControls(list, 1, 0, 0);
    m_listBox.bLooping = 1;
    SetActiveControl(&m_listBox, 0);
    CenterPanel();
}
#else
// Builds the 71-option list. Each row uses the listbox's own template row rather than a member of
// this class. Only the show-all-movies row is always visible, so it alone survives the release gate.
CSWGuiDebugMenu::CSWGuiDebugMenu(CSWGuiManager* pGuiManager, int bHideQaOptions)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("debug_p"), 1, 1, 1);
    InitControl(m_listBox, CExoString("LB_OPTIONS"), 1, 1, 0, 0, 0);
    InitControl(m_label, CExoString("LBL_BUILD"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    CExoString sBuildString = CExoString("Build: ") + g_sBuildNumber;
    m_label.m_text.m_params.SetText(sBuildString);
    m_bHideQaOptions = bHideQaOptions;

    CExoArrayList<CSWGuiControl*> list;
    CSWGuiButton* pTemplate = static_cast<CSWGuiButton*>(m_listBox.m_pProtoItem);
    list.SetSize(71);

    list.m_pArray[0] = CreateOption(CExoString("Turbo + Kill all + Open Doors"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::TurboOpenKillAll), pTemplate, 0);
    list.m_pArray[1] = CreateOption(CExoString("Add Party Members"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::PartyDebugMenu), pTemplate, 0);
    list.m_pArray[2] = CreateOption(CExoString("Unlock/Open All Doors"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::OpenAllDoors), pTemplate, 0);
    list.m_pArray[3] = CreateOption(CExoString("Kill All Enemies"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::KillAllEnemies), pTemplate, 0);
    list.m_pArray[4] = CreateOption(CExoString("Heal"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::Heal), pTemplate, 0);
    list.m_pArray[5] = CreateOption(CExoString("QA - Add any feat, power or skill"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::QAAllPowersFeatsSkills), pTemplate, 0);
    list.m_pArray[6] = CreateOption(CExoString("QA - Warp"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::QAWarp), pTemplate, 0);
    list.m_pArray[7] = CreateOption(CExoString("QA Display"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleQADisplay), pTemplate, 0);
    list.m_pArray[8] = CreateOption(CExoString("Toggle Screenshot Mode"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleScreenshotMode), pTemplate, 0);
    list.m_pArray[9] = CreateOption(CExoString("Toggle Party Bar"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::TogglePartyBar), pTemplate, 0);
    list.m_pArray[10] = CreateOption(CExoString("Toggle Flight Camera"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleNavigateCamera), pTemplate, 0);
    list.m_pArray[11] = CreateOption(CExoString("QA - Add 1000 experience"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::AddExperience1), pTemplate, 0);
    list.m_pArray[12] = CreateOption(CExoString("QA - Add 5000 experience"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::AddExperience2), pTemplate, 0);
    list.m_pArray[13] = CreateOption(CExoString("QA - Add 10000 experience"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::AddExperience3), pTemplate, 0);
    list.m_pArray[14] = CreateOption(CExoString("QA - Toggle Log conversations"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::LogConversations), pTemplate, 0);
    list.m_pArray[15] = CreateOption(CExoString("Toggle Turbo Speed"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleTurboSpeed), pTemplate, 0);
    list.m_pArray[16] = CreateOption(CExoString("Create Item"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::CreateItem), pTemplate, 0);
    list.m_pArray[17] = CreateOption(CExoString("Change Formation"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ChangeFormation), pTemplate, 0);
    list.m_pArray[18] = CreateOption(CExoString("Toggle Invulnerability"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleDamage), pTemplate, 0);
    list.m_pArray[19] = CreateOption(CExoString("Toggle All Damage"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleAllDamage), pTemplate, 0);
    list.m_pArray[20] = CreateOption(CExoString("Toggle Engine Stats"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleTracing), pTemplate, 0);
    list.m_pArray[21] = CreateOption(CExoString("Toggle Safe Region"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleSafeRegion), pTemplate, 0);
    list.m_pArray[22] = CreateOption(CExoString("Toggle Object Indicator"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleObjectIndicator), pTemplate, 0);
    list.m_pArray[23] = CreateOption(CExoString("Toggle Grass"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleGrass), pTemplate, 0);
    list.m_pArray[24] = CreateOption(CExoString("Toggle AABB"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleAABB), pTemplate, 0);
    list.m_pArray[25] = CreateOption(CExoString("Restart Minigame"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::RestartMinigame), pTemplate, 0);
    list.m_pArray[26] = CreateOption(CExoString("Log Heap Walk"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::LogHeapWalk), pTemplate, 0);
    list.m_pArray[27] = CreateOption(CExoString("Print Heap Walk Stats"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::PrintHeapWalk), pTemplate, 0);
    list.m_pArray[28] = CreateOption(CExoString("Mem Track to File"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleMemTrack), pTemplate, 0);
    list.m_pArray[29] = CreateOption(CExoString("Mem Track Stats"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleMemTrackStats), pTemplate, 0);
    list.m_pArray[30] = CreateOption(CExoString("KILL ME NOW!"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::KillActiveCharacter), pTemplate, 0);
    list.m_pArray[31] = CreateOption(CExoString("Bright Sun and Moon"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::BrightSunAndMoon), pTemplate, 0);
    list.m_pArray[32] = CreateOption(CExoString("Spawn Cheat Bot"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::SpawnCheatBot), pTemplate, 0);
    list.m_pArray[33] = CreateOption(CExoString("Toggle Display Resource Loads"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleResourceLoads), pTemplate, 0);
    list.m_pArray[34] = CreateOption(CExoString("Show Test Pattern"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ShowTestPattern), pTemplate, 0);
    list.m_pArray[35] = CreateOption(CExoString("Warp Forward"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::WarpForward), pTemplate, 0);
    list.m_pArray[36] = CreateOption(CExoString("Toggle Combat Debugging"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleCombatDebugging), pTemplate, 0);
    list.m_pArray[37] = CreateOption(CExoString("Toggle Party Follow Logging"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::TogglePartyFollowLogging), pTemplate, 0);
    list.m_pArray[38] = CreateOption(CExoString("DO NOT PUSH!!!"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::TestFunction), pTemplate, 0);
    list.m_pArray[39] = CreateOption(CExoString("Toggle Show String Refs"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleShowStringRefs), pTemplate, 0);
    list.m_pArray[40] = CreateOption(CExoString("Toggle Fog of War"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleFogOfWar), pTemplate, 0);
    list.m_pArray[41] = CreateOption(CExoString("QA - Display All Triggers"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleQADisplayAllTriggers), pTemplate, 0);
    list.m_pArray[42] = CreateOption(CExoString("Increase Alignment by 5"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::IncreaseAlignment), pTemplate, 0);
    list.m_pArray[43] = CreateOption(CExoString("Decrease Alignment by 5"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::DecreaseAlignment), pTemplate, 0);
    list.m_pArray[44] = CreateOption(CExoString("Show All Movies"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ShowAllMovies), pTemplate, 1);
    list.m_pArray[45] = CreateOption(CExoString("Enable Clairvoyance"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::EnableClairvoyanceEffect), pTemplate, 0);
    list.m_pArray[46] = CreateOption(CExoString("Enable Fury"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::EnableFuryEffect), pTemplate, 0);
    list.m_pArray[47] = CreateOption(CExoString("Disable Video Effects"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::DisableVideoEffects), pTemplate, 0);
    list.m_pArray[48] = CreateOption(CExoString("Enable Force Sight"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleForceSight), pTemplate, 0);
    list.m_pArray[49] = CreateOption(CExoString("Toggle Hologram"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ToggleHologram), pTemplate, 0);
    list.m_pArray[50] = CreateOption(CExoString("Take Screenshot"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::TakeScreenShot), pTemplate, 0);
    list.m_pArray[51] = CreateOption(CExoString("Increase Strength"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::IncreaseStrength), pTemplate, 0);
    list.m_pArray[52] = CreateOption(CExoString("Increase Intelligence"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::IncreaseIntelligence), pTemplate, 0);
    list.m_pArray[53] = CreateOption(CExoString("Increase Wisdom"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::IncreaseWisdom), pTemplate, 0);
    list.m_pArray[54] = CreateOption(CExoString("Increase Dexterity"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::IncreaseDexterity), pTemplate, 0);
    list.m_pArray[55] = CreateOption(CExoString("Increase Constitution"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::IncreaseConstitution), pTemplate, 0);
    list.m_pArray[56] = CreateOption(CExoString("Increase Charisma"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::IncreaseCharisma), pTemplate, 0);
    list.m_pArray[57] = CreateOption(CExoString("Decrease Strength"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::DecreaseStrength), pTemplate, 0);
    list.m_pArray[58] = CreateOption(CExoString("Decrease Intelligence"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::DecreaseIntelligence), pTemplate, 0);
    list.m_pArray[59] = CreateOption(CExoString("Decrease Wisdom"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::DecreaseWisdom), pTemplate, 0);
    list.m_pArray[60] = CreateOption(CExoString("Decrease Dexterity"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::DecreaseDexterity), pTemplate, 0);
    list.m_pArray[61] = CreateOption(CExoString("Decrease Constitution"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::DecreaseConstitution), pTemplate, 0);
    list.m_pArray[62] = CreateOption(CExoString("Decrease Charisma"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::DecreaseCharisma), pTemplate, 0);
    list.m_pArray[63] = CreateOption(CExoString("Revan is Male"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::RevanMale), pTemplate, 0);
    list.m_pArray[64] = CreateOption(CExoString("Revan is Female"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::RevanFemale), pTemplate, 0);
    list.m_pArray[65] = CreateOption(CExoString("Revan is Good"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::RevanGood), pTemplate, 0);
    list.m_pArray[66] = CreateOption(CExoString("Revan is Evil"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::RevanEvil), pTemplate, 0);
    list.m_pArray[67] = CreateOption(CExoString("Change Global Variable"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ChangeGlobalVars), pTemplate, 0);
    list.m_pArray[68] = CreateOption(CExoString("Increase Infuence"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::IncreaseInfluence), pTemplate, 0);
    list.m_pArray[69] = CreateOption(CExoString("Decrease Infuence"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::DecreaseInfluence), pTemplate, 0);
    list.m_pArray[70] = CreateOption(CExoString("Show Infuence Stats"),
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDebugMenu::ShowInfluenceStats), pTemplate, 0);

    m_listBox.AddControls(list, 1, 0, 0);
    m_listBox.bLooping = 1;
    SetActiveControl(&m_listBox, 0);
    CenterPanel();
}
#endif

// Turbo plus kill-all plus open-doors turns turbo mode on, walks the area killing every enemy the
// faction test accepts, then walks it again to unlock and open every door.

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
void CSWGuiDebugMenu::TurboOpenKillAll(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bTurboSpeed = 1;
    unsigned long long playerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(playerId);
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea) {
            unsigned long long objId;
            int bMore = pArea->GetFirstObjectInArea(objId);
            while (bMore) {
                CSWSCreature* pVictim =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objId);
                if (pVictim && (pVictim->m_pStats->m_factionId | 2) == 3) {
                    CGameEffect* pEffect = new CGameEffect(1);
                    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
                    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                    pEffect->SetCreator(OBJECT_INVALID);
                    pEffect->SetInteger(1, 1);
                    pVictim->ApplyEffect(pEffect, 0, 0);
                }
                bMore = pArea->GetNextObjectInArea(objId);
            }
        }
        pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea) {
            unsigned long long objId;
            int bMore = pArea->GetFirstObjectInArea(objId);
            while (bMore) {
                CSWSDoor* pDoor = g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(objId);
                if (pDoor) {
                    pDoor->m_bLocked = 0;
                    pDoor->AddOpenDoorAction(objId, 0);
                }
                bMore = pArea->GetNextObjectInArea(objId);
            }
        }
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}
#endif  // !KOTOR1

// Kills every enemy in the area, the same walk as the kill phase above without the turbo flag and
// without the door loop.

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
void CSWGuiDebugMenu::KillAllEnemies(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    unsigned long long playerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(playerId);
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea) {
            unsigned long long objId;
            int bMore = pArea->GetFirstObjectInArea(objId);
            while (bMore) {
                CSWSCreature* pVictim =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objId);
                if (pVictim && (pVictim->m_pStats->m_factionId | 2) == 3) {
                    CGameEffect* pEffect = new CGameEffect(1);
                    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
                    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                    pEffect->SetCreator(OBJECT_INVALID);
                    pEffect->SetInteger(1, 1);
                    pVictim->ApplyEffect(pEffect, 0, 0);
                }
                bMore = pArea->GetNextObjectInArea(objId);
            }
            g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
        }
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}
#endif  // !KOTOR1

// Spawns an "npc_tempman" creature 1.5 units in front of the player along its facing. The spot is
// sanitized through ComputeSafeLocation first, in place, so the clamped coordinates are what reach
// AddToArea.

void CSWGuiDebugMenu::SpawnCheatBot(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    if (pPlayer) {
        Vector vPos = pPlayer->m_position + pPlayer->m_orientation * 1.5f;
        CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
        pArea->ComputeSafeLocation(vPos, 10.0f, pPlayer->m_pPathfindInfo, 1, &vPos);
        CSWSCreature* pBot = new CSWSCreature(OBJECT_INVALID, 0);
#if KOTOR1  // K1 uses the "g_CheatBot" template and adds the bot whether or not it loaded
        pBot->LoadFromTemplate(CResRef("g_CheatBot"), 0);
        {
#else
        if (pBot->LoadFromTemplate(CResRef("npc_tempman"), 0)) {
#endif
            pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
            pBot->AddToArea(pArea, vPos.x, vPos.y, vPos.z, 1, 1);
        }
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

// Party-influence buttons. Each walks the same guard ladder (player creature, party table, NPC id,
// influence recorded), posting its own on-screen line per failure, then checks the bound, 100 for
// the increase and 0 for the decrease. The formatted name is the creature's tag, not a display name.

#if !KOTOR1
void CSWGuiDebugMenu::IncreaseInfluence(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    if (pCreature) {
        CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
        if (pPartyTable) {
            int nNpcId = pPartyTable->GetNPCID(pCreature->m_objectId);
            if (nNpcId != -1) {
                int nInfluence = pPartyTable->GetInfluence(nNpcId);
                if (nInfluence != -1) {
                    if (nInfluence != 100) {
                        pPartyTable->SetInfluence(nNpcId, nInfluence + 1);
                        char szBuf[100];
                        DebugSprintf(szBuf, nullptr, "IncreaseInfluence(): New influence on %s = %d\n.",
                                     pCreature->m_sTag.CStr(), nInfluence + 1);
                        AurPostString(szBuf, 5, 10, 10.0f);
                        pCreature->HandleAlignmentInfluence();
                    } else {
                        AurPostString(const_cast<char*>("IncreaseInfluence(): Influence is already at maximum of 100."),
                                     5, 10, 10.0f);
                    }
                } else {
                    AurPostString(
                        const_cast<char*>("IncreaseInfluence(): Creature has never been in party. Cannot change influence."),
                        5, 10, 10.0f);
                }
            } else {
                AurPostString(const_cast<char*>("IncreaseInfluence(): Cannot modify influence on this creature."),
                             5, 10, 10.0f);
            }
        } else {
            AurPostString(const_cast<char*>("IncreaseInfluence(): Party Table not accessible."), 5, 10, 10.0f);
        }
    } else {
        AurPostString(const_cast<char*>("IncreaseInfluence(): Player Creature not accessible."), 5, 10, 10.0f);
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::DecreaseInfluence(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    if (pCreature) {
        CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
        if (pPartyTable) {
            int nNpcId = pPartyTable->GetNPCID(pCreature->m_objectId);
            if (nNpcId != -1) {
                int nInfluence = pPartyTable->GetInfluence(nNpcId);
                if (nInfluence != -1) {
                    if (nInfluence != 0) {
                        pPartyTable->SetInfluence(nNpcId, nInfluence - 1);
                        char szBuf[100];
                        DebugSprintf(szBuf, nullptr, "DecreaseInfluence(): New influence on %s = %d\n.",
                                     pCreature->m_sTag.CStr(), nInfluence - 1);
                        AurPostString(szBuf, 5, 10, 10.0f);
                        pCreature->HandleAlignmentInfluence();
                    } else {
                        AurPostString(const_cast<char*>("DecreaseInfluence(): Influence is already at minimum of 0."),
                                     5, 10, 10.0f);
                    }
                } else {
                    AurPostString(
                        const_cast<char*>("DecreaseInfluence(): Creature has never been in party. Cannot change influence."),
                        5, 10, 10.0f);
                }
            } else {
                AurPostString(const_cast<char*>("DecreaseInfluence(): Cannot modify influence on this creature."),
                             5, 10, 10.0f);
            }
        } else {
            AurPostString(const_cast<char*>("DecreaseInfluence(): Party Table not accessible."), 5, 10, 10.0f);
        }
    } else {
        AurPostString(const_cast<char*>("DecreaseInfluence(): Player Creature not accessible."), 5, 10, 10.0f);
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

// Prints a twelve-row on-screen table of the party table's per-companion influence: the companion
// name comes from a switch on the party-table index, and the two column headers go out before the
// loop.

void CSWGuiDebugMenu::ShowInfluenceStats(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable) {
        AurPostString(const_cast<char*>("Character"), 30, 5, 10.0f);
        AurPostString(const_cast<char*>("Influence"), 45, 5, 10.0f);
        for (int i = 0; i != 12; i++) {
            int nInfluence = pPartyTable->GetInfluence(i);
            CExoString sName;
            CExoString sValue;
            sValue.Format(const_cast<char*>("%d"), nInfluence);
            switch (i) {
            case NPC_ATTON:       sName = "Atton"; break;
            case NPC_BAO_DUR:     sName = "Bao-Dur"; break;
            case NPC_CANDEROUS:   sName = "Mand"; break;
            case NPC_G0T0:        sName = "G0-T0"; break;
            case NPC_HANDMAIDEN:  sName = "Handmaiden"; break;
            case NPC_HK_47:       sName = "HK-47"; break;
            case NPC_KREIA:       sName = "Kreia"; break;
            case NPC_MIRA:        sName = "Mira"; break;
            case NPC_T3_M4:       sName = "T3-M4"; break;
            case NPC_VISAS:       sName = "Visas Marr"; break;
            case NPC_HANHARR:     sName = "Hanharr"; break;
            case NPC_DISCIPLE:    sName = "Disciple"; break;
            }
            AurPostString(sName.CStr(), 30, i + 6, 10.0f);
            AurPostString(sValue.CStr(), 45, i + 6, 10.0f);
        }
    } else {
        AurPostString(const_cast<char*>("ShowInfluenceStats(): Party Table not accessible."), 5, 10, 10.0f);
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}
#endif

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
// Stubbed out in the shipped build (the party debug menu is opened elsewhere).
void CSWGuiDebugMenu::PartyDebugMenu(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
}

// Stubbed out in the shipped build.
void CSWGuiDebugMenu::ChangeGlobalVars(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
}
#endif  // !KOTOR1

void CSWGuiDebugMenu::WarpForward(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    warpforward(const_cast<char*>(""));
}

void CSWGuiDebugMenu::ChangeFormation(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->NextPartyFormation();
}

// These callbacks only dismiss, because the heap and memory-tracking buttons and the scratch test
// hook are compiled out of the shipped build, leaving nothing but the self-dismiss tail.

void CSWGuiDebugMenu::LogHeapWalk(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::PrintHeapWalk(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleMemTrack(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleMemTrackStats(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::TestFunction(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
#if KOTOR1  // K1: leave combat, drop the player's queue and restore the camera first
    g_pAppManager->m_pClientExoApp->SetCombatMode(0);
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    pServer->GetPlayerCreature()->ClearAllActions(1);
    g_pAppManager->m_pClientExoApp->GetModule()->RestoreCamera();
#endif
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

// Global-flag toggles. Most negate the flag; the two that subtract from one compute a new value
// instead of a boolean.

void CSWGuiDebugMenu::ToggleQADisplay(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    RenderQA = !RenderQA;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::LogConversations(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_bLogConversations = !g_bLogConversations;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleAllDamage(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_bDisableDamage = !g_bDisableDamage;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleSafeRegion(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_bDrawSafeRegion = !g_bDrawSafeRegion;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleObjectIndicator(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_bShowIndicator = !g_bShowIndicator;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleGrass(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    enablegrass = 1 - enablegrass;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleAABB(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    renderaabb = 1 - renderaabb;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleResourceLoads(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_bDisplayServicedResources = !g_bDisplayServicedResources;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::TogglePartyFollowLogging(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    FollowLeaderFullLogging = !FollowLeaderFullLogging;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleFogOfWar(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_bShowFogOfWar = !g_bShowFogOfWar;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleShowStringRefs(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pTlkTable->m_bShowStringRefs = !g_pTlkTable->m_bShowStringRefs;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
// Toggles g_bExitProgram, the one toggle that dismisses first. It tears the whole application down,
// so the menu has to be gone before the flag is set.
void CSWGuiDebugMenu::Toggle_g_bExitProgram(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
    g_bExitProgram = !g_bExitProgram;
}
#endif  // !KOTOR1


#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
void CSWGuiDebugMenu::DisableVideoEffects(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->DisableVideoEffect();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::EnableClairvoyanceEffect(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->EnableVideoEffect(6);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleForceSight(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->EnableVideoEffect(4);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::TakeScreenShot(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
    g_pAppManager->m_pClientExoApp->DoScreenShot();
}
#endif  // !KOTOR1

void CSWGuiDebugMenu::TogglePartyBar(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ToggleSWGui();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
void CSWGuiDebugMenu::ShowAllMovies(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetAllMoviesShown();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}
#endif  // !KOTOR1

// Screenshot mode and the on-screen debug-string overlay are exact opposites, so entering one turns
// the other off, which is why both results come from the same pre-flip read.
void CSWGuiDebugMenu::ToggleScreenshotMode(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_bInScreenshotMode = !g_bInScreenshotMode;
        g_bEnableAurPostString = !g_bInScreenshotMode;
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}


void CSWGuiDebugMenu::ToggleDamage(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    DebugMode(const_cast<char*>("1"));
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerCheat_Invulnerability();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleCombatDebugging(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    DebugMode(const_cast<char*>("1"));
    g_pAppManager->m_pClientExoApp->GetSWCMessage()
        ->SendPlayerToServerCheat_ToggleCombatDebugging(!g_bEnableCombatDebugging);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}


void CSWGuiDebugMenu::AddExperience1(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable)
        pPartyTable->DistributeExperience(1000, 1);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::AddExperience2(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable)
        pPartyTable->DistributeExperience(5000, 1);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::AddExperience3(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable)
        pPartyTable->DistributeExperience(10000, 1);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::KillActiveCharacter(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pServerExoApp->KillActiveCharacter();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
    }
}

// Sub-menu openers. Each hides this menu and shows the next one.

void CSWGuiDebugMenu::QAAllPowersFeatsSkills(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowPowersFeatsSkillsDebugMenu();
    }
}

void CSWGuiDebugMenu::QAWarp(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowLoadModuleDebugMenu();
    }
}

void CSWGuiDebugMenu::CreateItem(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowItemCreateMenu();
    }
}


// The only callback that null-checks the whole application chain, since it can be reached from the
// main menu before a module, and so an area, exists.
void CSWGuiDebugMenu::BrightSunAndMoon(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (g_pAppManager && g_pAppManager->m_pClientExoApp) {
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        if (pModule && pModule->m_pArea)
            pModule->m_pArea->ForceBrightSunAndMoon();
    }
    K2_ASSUME(g_pAppManager != 0);   // the app manager outlives every GUI panel
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::RestartMinigame(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pMiniGame) {
        g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->RestartMiniGame();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
    }
}

// Does nothing while a minigame is running: the minigame owns the player's state.
void CSWGuiDebugMenu::Heal(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pMiniGame == 0) {
        DebugMode(const_cast<char*>("1"));
        g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerCheat_Resurrect();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
    }
}


void CSWGuiDebugMenu::OnPanelAdded()
{
    g_pAppManager->m_pClientExoApp->SetGuiGameSlow(1);
    CSWGuiPanel::OnPanelAdded();
}

void CSWGuiDebugMenu::OnPanelRemoved()
{
    g_pAppManager->m_pClientExoApp->SetGuiGameSlow(0);
    SetVisible(1);
    CSWGuiPanel::OnPanelRemoved();
}

// Either cancel key closes the menu, and every event still reaches the base handler.
void CSWGuiDebugMenu::HandleInputEvent(int type, int val)
{
    if (val && (type == INPUTEVENT_GUI_BACK || type == INPUTEVENT_GUI_B))
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
    CSWGuiPanel::HandleInputEvent(type, val);
}

// The twelve ability-score callbacks are near-identical; each reads the current base off the
// player's server stats and pushes the value plus or minus one back through the matching setter.

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
void CSWGuiDebugMenu::IncreaseStrength(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nSTRBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetSTRBase(nScore + 1);
    }
}

void CSWGuiDebugMenu::DecreaseStrength(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nSTRBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetSTRBase(nScore - 1);
    }
}

void CSWGuiDebugMenu::IncreaseDexterity(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nDEXBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetDEXBase(nScore + 1);
    }
}

void CSWGuiDebugMenu::DecreaseDexterity(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nDEXBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetDEXBase(nScore - 1);
    }
}

// Constitution is the one score whose setter also adjusts hit points, hence the second argument.
void CSWGuiDebugMenu::IncreaseConstitution(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nCONBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetCONBase(nScore + 1, 1);
    }
}

void CSWGuiDebugMenu::DecreaseConstitution(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nCONBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetCONBase(nScore - 1, 1);
    }
}

void CSWGuiDebugMenu::IncreaseIntelligence(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nINTBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetINTBase(nScore + 1);
    }
}

void CSWGuiDebugMenu::DecreaseIntelligence(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nINTBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetINTBase(nScore - 1);
    }
}

void CSWGuiDebugMenu::IncreaseWisdom(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nWISBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetWISBase(nScore + 1);
    }
}

void CSWGuiDebugMenu::DecreaseWisdom(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nWISBase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetWISBase(nScore - 1);
    }
}

void CSWGuiDebugMenu::IncreaseCharisma(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nCHABase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetCHABase(nScore + 1);
    }
}

void CSWGuiDebugMenu::DecreaseCharisma(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        unsigned char nScore = g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->m_nCHABase;
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->SetCHABase(nScore - 1);
    }
}
#endif  // !KOTOR1

// Alignment. Unlike the ability scores these two have no event-argument gate, since they are safe to
// fire twice, and five points per click is the intended step.

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
void CSWGuiDebugMenu::IncreaseAlignment(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    if (pCreature)
        pCreature->m_pStats->ModifyAlignment(5, 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::DecreaseAlignment(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    if (pCreature)
        pCreature->m_pStats->ModifyAlignment(-5, 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}
#endif  // !KOTOR1

// The KOTOR1 import answers the sequel asks the player for. None of these callbacks dismisses the
// menu, so sex and ending can be set back to back.

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
void CSWGuiDebugMenu::RevanMale(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
            ->SetValueNumber(CExoString("101PER_Revan_Sex"), 0);
}

void CSWGuiDebugMenu::RevanFemale(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
            ->SetValueNumber(CExoString("101PER_Revan_Sex"), 1);
}

void CSWGuiDebugMenu::RevanGood(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
            ->SetValueNumber(CExoString("101PER_Revan_End"), 0);
}

void CSWGuiDebugMenu::RevanEvil(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg)
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()
            ->SetValueNumber(CExoString("101PER_Revan_End"), 1);
}
#endif  // !KOTOR1


void CSWGuiDebugMenu::ToggleTracing(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (bTracesOn) {
        untrace(0);
        bTracesOn = 0;
    } else {
#if KOTOR1  // K1: the three memory traces are switched on with the frame rate
        trace(const_cast<char*>("TexMem"));
        trace(const_cast<char*>("GfxMem"));
        trace(const_cast<char*>("XbxMem"));
#endif
        trace(const_cast<char*>("fps"));
        bTracesOn = 1;
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

void CSWGuiDebugMenu::ToggleTurboSpeed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    pOptions->m_bTurboSpeed = !g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bTurboSpeed;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

// Switches the camera mode three ways, leaving the free-fly camera, entering it from either mode
// that allows it, or returning without dismissing the menu.
void CSWGuiDebugMenu::ToggleNavigateCamera(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_NAVIGATE) {
        g_pAppManager->m_pClientExoApp->GetModule()->RestoreCamera();
#if KOTOR1  // K1: only the on-a-stick camera enters the free-fly camera
    } else if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK) {
#else
    } else if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_ON_A_STICK ||
               g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_COMBAT) {
#endif
        g_pAppManager->m_pClientExoApp->GetModule()->SetNavigateCamera();
    } else {
        return;
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

// Toggles two globals for one concept, the overlay flag and the renderer's own mirror of it.
void CSWGuiDebugMenu::ToggleQADisplayAllTriggers(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    RenderQATriggers = !RenderQATriggers;
    enablerendertriggers = RenderQATriggers;
    g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_pArea->DisplayAllTriggers(RenderQATriggers);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
// Cycles the force-fury visual through three settings and back to off. The camera shake and controller
// rumble only fire on the three real settings.
void CSWGuiDebugMenu::EnableFuryEffect(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    s_nForceFuryEffect++;
    if (s_nForceFuryEffect > 3)
        s_nForceFuryEffect = 0;
    AurEnableForceFury(s_nForceFuryEffect);
    if (s_nForceFuryEffect) {
        g_pAppManager->m_pClientExoApp->GetModule()->ShakeCamera(0, 5000);
        g_pExoInput->SetRumble(0, 1.0f, 1.0f, 15000);
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}
#endif  // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
// Flips every creature in the area between solid and hologram rendering. Each creature has a body
// model and a head model, and both are switched.
void CSWGuiDebugMenu::ToggleHologram(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWCArea* pArea = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea;
    if (pArea) {
        s_bHologramEnabled = !s_bHologramEnabled;
        for (int i = 0; i < pArea->m_objectIdList.m_count; i++) {
            CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
                pArea->m_objectIdList.m_pArray[i]);
            if (pCreature) {
                Gob* pGob = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
                if (pGob)
                    pGob->EnableHologram(s_bHologramEnabled);
                pGob = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1);
                if (pGob)
                    pGob->EnableHologram(s_bHologramEnabled);
            }
        }
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only debug-menu callback(s): K1's menu never binds them
// Walks every object in the player's area and, for each door, clears the lock and queues an open
// action on the door itself.
void CSWGuiDebugMenu::OpenAllDoors(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    unsigned long long playerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(playerId);
    if (pCreature) {
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea) {
            unsigned long long objId;
            int bMore = pArea->GetFirstObjectInArea(objId);
            while (bMore) {
                CSWSDoor* pDoor = g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(objId);
                if (pDoor) {
                    pDoor->m_bLocked = 0;
                    pDoor->AddOpenDoorAction(objId, 0);
                }
                bMore = pArea->GetNextObjectInArea(objId);
            }
        }
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideDebugMenu();
}
#endif  // !KOTOR1

// Drops a test-pattern overlay on top of everything. Unlike every other entry in this menu it does
// not dismiss the debug menu, since the pattern goes up over it.
void CSWGuiDebugMenu::ShowTestPattern(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWGuiTestPattern* pPanel = new CSWGuiTestPattern(m_pGuiManager);
    m_pGuiManager->AddPanel(pPanel, 3, 1);
}

// The test-pattern panel has no key function, so its vtable and virtual stubs are emitted into every
// translation unit that instantiates it, this one included.

#if KOTOR1
// K1: no build gate, and the option is sized to its own text.
CSWGuiButton* CSWGuiDebugMenu::CreateOption(const CExoString& sText,
                                            void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                                            CSWGuiButton* pTemplate)
{
    CSWGuiExtent extent = {0, 0, 0, 0};
    CSWGuiButton* pButton = new CSWGuiButton();
    pButton->Initialize(extent, pTemplate);
    pButton->m_text.m_params.SetText(sText);
    int nText = static_cast<CSWGuiDrawable*>(&pButton->m_text)->GetIdealHeight();
    int nHilight = pButton->m_borderHilight.m_params.GetBorderDim();
    int nBorder = pButton->m_border.m_params.GetBorderDim();
    extent.h = nText + (nHilight > nBorder ? nHilight : nBorder) * 2;
    pButton->SetExtent(extent);
    pButton->AddEvent(INPUTEVENT_GUI_A, this, pMethod);
    pButton->AddEvent(INPUTEVENT_GUI_START, this, pMethod);
    return pButton;
}
#else
// Builds an option as the family's builder does, plus this menu's release gate. With the hide-QA
// flag set, an option that is not always-visible is skipped and null returned, which is how the
// shipping build hides the QA-only entries.
CSWGuiButton* CSWGuiDebugMenu::CreateOption(const CExoString& sText,
                                            void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                                            CSWGuiButton* pTemplate, int bAlways)
{
    if (!bAlways && m_bHideQaOptions)
        return 0;
    CSWGuiExtent extent = pTemplate->m_extent;
    CSWGuiButton* pButton = new CSWGuiButton();
    pButton->Initialize(extent, pTemplate);
    pButton->m_text.m_params.SetText(sText);
    pButton->AddEvent(INPUTEVENT_GUI_A, this, pMethod);
    pButton->AddEvent(INPUTEVENT_GUI_START, this, pMethod);
    return pButton;
}
#endif

