// The in-game character and level-up screen.

#include "k2stdint.h"
#include "swguiingamecharacter.h"

#include "swguicontrol.h"
#include "exoresman.h"
#include "nwccreature.h"
#include "nwccreatureappearance.h"
#include "aurscene.h"
#include "aurcamera.h"
#include "AURORA/aurmodel.h"
#include "Vector.h"
#include "Quaternion.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swgui.h"
#include "swguilevelupchargen.h"
#include "swguiingame.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwseffectlist.h"
#include "nwclass.h"
#include "swparty.h"
#include "AURORA/aurbehavior.h"
#include "nwccreaturestats.h"
#include "nwsrules.h"
#include "exolocstringinternal.h"
#if KOTOR1
#include "serverexoapp.h"
#include "serverinfo.h"
#include "partytables.h"
#include "swguiscriptselect.h"
#include "nwupdatestats.h"

void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

extern int g_bDo3DGUIs;

#include "nwscriptconst.h"
#include "exoinput.h"
#include "strref.h"

#if !KOTOR1
// The Jedi and Sith classes, as bits of (class - CLASS_TYPE_JEDIGUARDIAN).
enum {
    FORCE_CLASS_BITS = (1 << (CLASS_TYPE_JEDIGUARDIAN - CLASS_TYPE_JEDIGUARDIAN)) |
                       (1 << (CLASS_TYPE_JEDICONSULAR - CLASS_TYPE_JEDIGUARDIAN)) |
                       (1 << (CLASS_TYPE_JEDISENTINEL - CLASS_TYPE_JEDIGUARDIAN)) |
                       (1 << (CLASS_TYPE_JEDIWEAPONMASTER - CLASS_TYPE_JEDIGUARDIAN)) |
                       (1 << (CLASS_TYPE_JEDIMASTER - CLASS_TYPE_JEDIGUARDIAN)) |
                       (1 << (CLASS_TYPE_JEDIWATCHMAN - CLASS_TYPE_JEDIGUARDIAN)) |
                       (1 << (CLASS_TYPE_SITHMARAUDER - CLASS_TYPE_JEDIGUARDIAN)) |
                       (1 << (CLASS_TYPE_SITHLORD - CLASS_TYPE_JEDIGUARDIAN)) |
                       (1 << (CLASS_TYPE_SITHASSASSIN - CLASS_TYPE_JEDIGUARDIAN)),
};
#endif

#if !KOTOR1
// Alignment strrefs, indexed by alignment class - 3.
static const uint64_t anForceGoodStrRefs[14] = {
    STRREF_LIGHT_MASTERY_STR, STRREF_LIGHT_MASTERY_WIS, STRREF_LIGHT_MASTERY_CON, STRREF_LIGHT_MASTERY_STR, STRREF_LIGHT_MASTERY_STR, STRREF_LIGHT_MASTERY_STR, STRREF_LIGHT_MASTERY_STR,
    STRREF_LIGHT_MASTERY_STR, STRREF_LIGHT_MASTERY_STR, STRREF_LIGHT_MASTERY_WIS, STRREF_LIGHT_MASTERY_CON, STRREF_LIGHT_MASTERY_STR, STRREF_LIGHT_MASTERY_WIS, STRREF_LIGHT_MASTERY_CON,
};
static const uint64_t anForceEvilStrRefs[14] = {
    STRREF_DARK_MASTERY_DAMAGE, STRREF_DARK_MASTERY_FORCE_POINTS, STRREF_DARK_MASTERY_DEX, STRREF_DARK_MASTERY_DAMAGE, STRREF_DARK_MASTERY_DAMAGE, STRREF_DARK_MASTERY_DAMAGE, STRREF_DARK_MASTERY_DAMAGE,
    STRREF_DARK_MASTERY_DAMAGE, STRREF_DARK_MASTERY_DAMAGE, STRREF_DARK_MASTERY_FORCE_POINTS, STRREF_DARK_MASTERY_DEX, STRREF_DARK_MASTERY_DAMAGE, STRREF_DARK_MASTERY_FORCE_POINTS, STRREF_DARK_MASTERY_DEX,
};
#endif

#if KOTOR1
// K1 loads the "character" layout and keeps an always-present scripts panel.
CSWGuiInGameCharacter::CSWGuiInGameCharacter(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    m_nCurrentNPC = -1;
    StartLoadFromLayout(CResRef("character"));

    InitControl(m_lblName, CExoString("LBL_NAME"), 1);
    InitControl(m_lblClass, CExoString("LBL_CLASS"), 1);
    InitControl(m_lblClass1, CExoString("LBL_CLASS1"), 1);
    InitControl(m_lblClass2, CExoString("LBL_CLASS2"), 1);
    InitControl(m_lblLevel, CExoString("LBL_LEVEL"), 1);
    InitControl(m_lblLevel1, CExoString("LBL_LEVEL1"), 1);
    InitControl(m_lblLevel2, CExoString("LBL_LEVEL2"), 1);
    InitControl(m_lblFortitude, CExoString("LBL_FORTITUDE"), 1);
    InitControl(m_lblReflex, CExoString("LBL_REFLEX"), 1);
    InitControl(m_lblWill, CExoString("LBL_WILL"), 1);
    InitControl(m_lblFortitudeStat, CExoString("LBL_FORTITUDE_STAT"), 1);
    InitControl(m_lblReflexStat, CExoString("LBL_REFLEX_STAT"), 1);
    InitControl(m_lblWillStat, CExoString("LBL_WILL_STAT"), 1);
    InitControl(m_lblExperience, CExoString("LBL_EXPERIENCE"), 1);
    InitControl(m_lblExperienceStat, CExoString("LBL_EXPERIENCE_STAT"), 1);
    InitControl(m_lblNextLevel, CExoString("LBL_NEXT_LEVEL"), 1);
    InitControl(m_lblNeededXp, CExoString("LBL_NEEDED_XP"), 1);
    InitControl(m_lblDefenseStat, CExoString("LBL_DEFENSE_STAT"), 1);
    InitControl(m_lblForceStat, CExoString("LBL_FORCE_STAT"), 1);
    InitControl(m_lblVitalityStat, CExoString("LBL_VITALITY_STAT"), 1);
    InitControl(m_lblDefense, CExoString("LBL_DEFENSE"), 1);
    InitControl(m_lblForce, CExoString("LBL_FORCE"), 1);
    InitControl(m_lblVitality, CExoString("LBL_VITALITY"), 1);
    InitControl(m_lblStr, CExoString("LBL_STR"), 1);
    InitControl(m_lblStrength, CExoString("LBL_STRENGTH"), 1);
    InitControl(m_lblStrMod, CExoString("LBL_STR_MOD"), 1);
    InitControl(m_lblWis, CExoString("LBL_WIS"), 1);
    InitControl(m_lblWisdom, CExoString("LBL_WISDOM"), 1);
    InitControl(m_lblWisMod, CExoString("LBL_WIS_MOD"), 1);
    InitControl(m_lblCha, CExoString("LBL_CHA"), 1);
    InitControl(m_lblCharisma, CExoString("LBL_CHARISMA"), 1);
    InitControl(m_lblChaMod, CExoString("LBL_CHA_MOD"), 1);
    InitControl(m_lblInt, CExoString("LBL_INT"), 1);
    InitControl(m_lblIntelligence, CExoString("LBL_INTELLIGENCE"), 1);
    InitControl(m_lblIntMod, CExoString("LBL_INT_MOD"), 1);
    InitControl(m_lblCon, CExoString("LBL_CON"), 1);
    InitControl(m_lblConstitution, CExoString("LBL_CONSTITUTION"), 1);
    InitControl(m_lblConMod, CExoString("LBL_CON_MOD"), 1);
    InitControl(m_lblDex, CExoString("LBL_DEX"), 1);
    InitControl(m_lblDexterity, CExoString("LBL_DEXTERITY"), 1);
    InitControl(m_lblDexMod, CExoString("LBL_DEX_MOD"), 1);
    InitControl(m_view3DChar, CExoString("LBL_3DCHAR"), 1);
    InitControl(m_lblBevel, CExoString("LBL_BEVEL"), 1);
    InitControl(m_lblBevel2, CExoString("LBL_BEVEL2"), 1);
    InitControl(m_sldAlign, CExoString("SLD_ALIGN"), 1);
    InitControl(m_lblAdorn, CExoString("LBL_ADORN"), 1);
    InitControl(m_lblMore, CExoString("LBL_MORE"), 1);
    InitControl(m_lblLight, CExoString("LBL_LIGHT"), 1);
    InitControl(m_lblDark, CExoString("LBL_DARK"), 1);
    InitControl(m_btnAuto, CExoString("BTN_AUTO"), 1);
    InitControl(m_btnLevelUp, CExoString("BTN_LEVELUP"), 1);
    InitControl(m_btnChange1, CExoString("BTN_CHANGE1"), 1);
    InitControl(m_btnChange2, CExoString("BTN_CHANGE2"), 1);
    InitControl(m_btnExit, CExoString("BTN_EXIT"), 1);
    InitControl(m_btnScripts, CExoString("BTN_SCRIPTS"), 1);
    InitControl(m_btn3DChar, CExoString("BTN_3DCHAR"), 1);
    InitControl(m_btnCharLeft, CExoString("BTN_CHARLEFT"), 1);
    InitControl(m_btnCharRight, CExoString("BTN_CHARRIGHT"), 1);
    CExoString sName;
    for (int i = 0; i < 10; i++) {
        sName.Format(const_cast<char*>("LBL_GOOD%d"), i + 1);
        InitControl(m_alblGood[i], sName, 1);
        m_alblGood[i].m_tail.bVisible = 0;
    }
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_btnCharLeft.m_tail.bFocusOnHover = 0;
    m_btnCharLeft.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnSwitchLeft));
    m_btnCharRight.m_tail.bFocusOnHover = 0;
    m_btnCharRight.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnSwitchRight));
    m_btnCharRight.m_border.m_params.m_fRotate = 180.0f;
    m_btnCharRight.m_borderHilight.m_params.m_fRotate = 180.0f;

    m_lblMore.m_tail.bVisible = 0;

    CAurCamera* pCamera = m_view3DChar.m_scene.m_pCamera;
    CAurScene* pScene = m_view3DChar.m_scene.m_pScene;

#ifdef K2_FIX_ENGINE_BUGS
    m_btn3DChar.AddEvent(SWGUIEVENT_SPEED_BUTTON_LEFT, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnRotateLeft));
    m_btn3DChar.AddEvent(SWGUIEVENT_SPEED_BUTTON_RIGHT, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnRotateRight));
#else
    m_btn3DChar.AddEvent(SWGUIEVENT_SPEED_BUTTON_LEFT, this,
        reinterpret_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::RotateCharacterLeft));
    m_btn3DChar.AddEvent(SWGUIEVENT_SPEED_BUTTON_RIGHT, this,
        reinterpret_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::RotateCharacterRight));
#endif

    m_btn3DChar.m_fStep = 0.1f;
    m_btn3DChar.m_fInitialDelay = 0.1f;

    if (g_bDo3DGUIs) {
        pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        CAurObject* pModel = m_view3DChar.m_scene.AddModel(CExoString("charrec_light"), -1);
        if (pModel) {
            pCamera->AttachToObject(pModel, const_cast<char*>("camerahook"), 1);
            pCamera->SetViewAngle(22.726f);
        }
    }

    m_pCreature = new CSWCCreature();
    m_pAppearanceCreature = 0;
    m_nAppearanceAlignment = -1;
    m_pScriptSelect = new CSWGuiScriptSelect(m_pGuiManager);

    m_btnLevelUp.m_text.m_params.bHilight = 1;
    m_btnAuto.m_text.m_params.bHilight = 1;

    m_btnAuto.m_tail.bFocusOnHover = 0;
    m_btnAuto.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
    m_btnLevelUp.m_tail.bFocusOnHover = 0;
    m_btnLevelUp.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnChange1.m_tail.bFocusOnHover = 0;
    m_btnChange1.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnChangeCharacter));
    m_btnChange2.m_tail.bFocusOnHover = 0;
    m_btnChange2.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnChangeCharacter));
    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnScripts.m_tail.bFocusOnHover = 0;
    m_btnScripts.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnXButtonPressed));
    m_sldAlign.m_tail.bFocusOnHover = 0;

    m_btnChange1.m_nToolTipStrRef = STRREF_CHANGE_CHARACTER;
    m_btnChange1.m_nBoundInputEventId = INPUTEVENT_CHANGE_CHARACTER;
    m_btnChange2.m_nToolTipStrRef = STRREF_CHANGE_CHARACTER;
#ifdef K1_UI_XBOX
    // A levels up and Y auto-levels: neither button takes the pad's focus.
    m_btn3DChar.m_tail.bFocusOnHover = 0;
#else
    m_btnAuto.m_tail.bFocusOnHover = 1;
    m_btnLevelUp.m_tail.bFocusOnHover = 1;
    m_btn3DChar.m_tail.bFocusOnHover = 0;

    m_btnAuto.SetMoveToControl(0, &m_btnLevelUp);
    m_btnAuto.SetMoveToControl(2, &m_btnLevelUp);
    m_btnLevelUp.SetMoveToControl(0, &m_btnAuto);
    m_btnLevelUp.SetMoveToControl(2, &m_btnAuto);
    SetActiveControl(&m_btnAuto, 0);
#endif

    m_btnAuto.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnIosAutoButton));
    m_btnLevelUp.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnIosLevelUpButton));
    IosAddButtonIcon(&m_btnScripts, 'x');
    IosAddButtonIcon(&m_btnExit, 'b');
#if defined(K1_UI_XBOX)
    IosAddButtonIcon(&m_btnLevelUp, 'a');
    IosAddButtonIcon(&m_btnAuto, 'y');
    m_iconChange.InitAndAddCorner(this, 'r', m_btnChange1.m_extent);   // RB: next party member
#elif !defined(K2_UI_TOUCH)
    m_iconChange.InitAndAddCorner(this, 'y', m_btnChange1.m_extent);
#else
    m_iconChange.InitAndAdd(this, 'y', m_btnChange1.m_extent, -30, -30);
#endif
}
#else
CSWGuiInGameCharacter::CSWGuiInGameCharacter(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("character_p"), 1, 1, 1);

    InitControl(m_lblForcemastery, CExoString("LBL_FORCEMASTERY"), 1, 1, 0, 0, 0);
    InitControl(m_lblFortitude, CExoString("LBL_FORTITUDE"), 1, 1, 0, 0, 0);
    InitControl(m_lblReflex, CExoString("LBL_REFLEX"), 1, 1, 0, 0, 0);
    InitControl(m_lblWill, CExoString("LBL_WILL"), 1, 1, 0, 0, 0);
    InitControl(m_lblFortitudeStat, CExoString("LBL_FORTITUDE_STAT"), 1, 1, 0, 0, 0);
    InitControl(m_lblReflexStat, CExoString("LBL_REFLEX_STAT"), 1, 1, 0, 0, 0);
    InitControl(m_lblWillStat, CExoString("LBL_WILL_STAT"), 1, 1, 0, 0, 0);
    InitControl(m_lblExperience, CExoString("LBL_EXPERIENCE"), 1, 1, 0, 0, 0);
    InitControl(m_lblExperienceStat, CExoString("LBL_EXPERIENCE_STAT"), 1, 1, 0, 0, 0);
    InitControl(m_lblNextLevel, CExoString("LBL_NEXT_LEVEL"), 1, 1, 0, 0, 0);
    InitControl(m_lblNeededXp, CExoString("LBL_NEEDED_XP"), 1, 1, 0, 0, 0);
    InitControl(m_lblDefenseStat, CExoString("LBL_DEFENSE_STAT"), 1, 1, 0, 0, 0);
    InitControl(m_lblForceStat, CExoString("LBL_FORCE_STAT"), 1, 1, 0, 0, 0);
    InitControl(m_lblVitalityStat, CExoString("LBL_VITALITY_STAT"), 1, 1, 0, 0, 0);
    InitControl(m_lblDefense, CExoString("LBL_DEFENSE"), 1, 1, 0, 0, 0);
    InitControl(m_lblForce, CExoString("LBL_FORCE"), 1, 1, 0, 0, 0);
    InitControl(m_lblVitality, CExoString("LBL_VITALITY"), 1, 1, 0, 0, 0);
    InitControl(m_lblStr, CExoString("LBL_STR"), 1, 1, 0, 0, 0);
    InitControl(m_lblStrength, CExoString("LBL_STRENGTH"), 1, 1, 0, 0, 0);
    InitControl(m_lblStrMod, CExoString("LBL_STR_MOD"), 1, 1, 0, 0, 0);
    InitControl(m_lblWis, CExoString("LBL_WIS"), 1, 1, 0, 0, 0);
    InitControl(m_lblWisdom, CExoString("LBL_WISDOM"), 1, 1, 0, 0, 0);
    InitControl(m_lblWisMod, CExoString("LBL_WIS_MOD"), 1, 1, 0, 0, 0);
    InitControl(m_lblCha, CExoString("LBL_CHA"), 1, 1, 0, 0, 0);
    InitControl(m_lblCharisma, CExoString("LBL_CHARISMA"), 1, 1, 0, 0, 0);
    InitControl(m_lblChaMod, CExoString("LBL_CHA_MOD"), 1, 1, 0, 0, 0);
    InitControl(m_lblInt, CExoString("LBL_INT"), 1, 1, 0, 0, 0);
    InitControl(m_lblIntelligence, CExoString("LBL_INTELLIGENCE"), 1, 1, 0, 0, 0);
    InitControl(m_lblIntMod, CExoString("LBL_INT_MOD"), 1, 1, 0, 0, 0);
    InitControl(m_lblCon, CExoString("LBL_CON"), 1, 1, 0, 0, 0);
    InitControl(m_lblConstitution, CExoString("LBL_CONSTITUTION"), 1, 1, 0, 0, 0);
    InitControl(m_lblConMod, CExoString("LBL_CON_MOD"), 1, 1, 0, 0, 0);
    InitControl(m_lblDex, CExoString("LBL_DEX"), 1, 1, 0, 0, 0);
    InitControl(m_lblDexterity, CExoString("LBL_DEXTERITY"), 1, 1, 0, 0, 0);
    InitControl(m_lblDexMod, CExoString("LBL_DEX_MOD"), 1, 1, 0, 0, 0);
    InitControl(m_view3DChar, CExoString("LBL_3DCHAR"), 1, 1, 0, 0, 0);
    InitControl(m_lblBevel, CExoString("LBL_BEVEL"), 1, 1, 0, 0, 0);
    InitControl(m_lblBevel2, CExoString("LBL_BEVEL2"), 1, 1, 0, 0, 0);
    InitControl(m_sldAlign, CExoString("SLD_ALIGN"), 1, 1, 0, 0, 0);
    InitControl(m_lblAdorn, CExoString("LBL_ADORN"), 1, 1, 0, 0, 0);
    InitControl(m_lblMore, CExoString("LBL_MORE"), 1, 1, 0, 0, 0);
    InitControl(m_lblLight, CExoString("LBL_LIGHT"), 1, 1, 0, 0, 0);
    InitControl(m_lblDark, CExoString("LBL_DARK"), 1, 1, 0, 0, 0);
    InitControl(m_btnAuto, CExoString("BTN_AUTO"), 1, 1, 0, 0, 0);
    InitControl(m_btnLevelUp, CExoString("BTN_LEVELUP"), 1, 1, 0, 0, 0);
    InitControl(m_btnExit, CExoString("BTN_EXIT"), 1, 1, 0, 0, 0);
    AspyrAddButtonIcon(&m_btnAuto, 'x');
    AspyrAddButtonIcon(&m_btnLevelUp, 'a');
    AspyrAddButtonIcon(&m_btnExit, 'b');
    InitControl(m_btn3DChar, CExoString("BTN_3DCHAR"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[4], CExoString("LBL_BAR5"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[5], CExoString("LBL_BAR6"), 1, 1, 0, 0, 0);
    InitControl(m_lblXpBack, CExoString("LBL_XP_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_lblMoreBack, CExoString("LBL_MORE_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_lblStatsBorder, CExoString("LBL_STATSBORDER"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    m_lblMore.m_tail.bVisible = 0;

    CAurCamera* pCamera = m_view3DChar.m_scene.m_pCamera;
    CAurScene* pScene = m_view3DChar.m_scene.m_pScene;

#ifdef K2_FIX_ENGINE_BUGS
    m_btn3DChar.AddEvent(SWGUIEVENT_SPEED_BUTTON_LEFT, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnRotateLeft));
    m_btn3DChar.AddEvent(SWGUIEVENT_SPEED_BUTTON_RIGHT, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::OnRotateRight));
#else
    m_btn3DChar.AddEvent(SWGUIEVENT_SPEED_BUTTON_LEFT, this,
        reinterpret_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::RotateCharacterLeft));
    m_btn3DChar.AddEvent(SWGUIEVENT_SPEED_BUTTON_RIGHT, this,
        reinterpret_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::RotateCharacterRight));
#endif

    m_btn3DChar.m_fStep = 0.1f;
    m_btn3DChar.m_fInitialDelay = 0.1f;

    if (g_bDo3DGUIs) {
        pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        CAurObject* pModel = m_view3DChar.m_scene.AddModel(CExoString("charrec_light"), -1);
        if (pModel) {
            pCamera->AttachToObject(pModel, const_cast<char*>("camerahook"), 1);
            pCamera->SetViewAngle(22.726f);
        }
    }

    m_pCreature = new CSWCCreature();
    m_pAppearanceCreature = 0;
    m_nAppearanceAlignment = -1;

    m_lblForcemastery.m_tail.bVisible = 0;
    m_btnLevelUp.m_text.m_params.bHilight = 1;
    m_btnAuto.m_text.m_params.bHilight = 1;

    m_btnAuto.m_tail.bFocusOnHover = 0;
    m_btnAuto.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnXButtonPressed));
    m_btnLevelUp.m_tail.bFocusOnHover = 0;
    m_btnLevelUp.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_sldAlign.m_tail.bFocusOnHover = 0;
}
#endif

CSWGuiInGameCharacter::~CSWGuiInGameCharacter()
{
#if KOTOR1  // K1 owns a scripts panel
    delete m_pScriptSelect;
#endif
    delete m_pCreature;
}


// The base call is qualified on purpose. The name is virtual, so an unqualified call would
// recurse.
void CSWGuiInGameCharacter::OnPanelRemoved()
{
    m_pAppearanceCreature = 0;
    CSWGuiPanel::OnPanelRemoved();
}

#if KOTOR1
void CSWGuiInGameCharacter::SetStats()
{
    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
    m_nPartyCount = pApp->GetSWParty()->m_nCharacterCount;

    CSWCCreature* pCreature;
    if (g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule
        && static_cast<unsigned char>(m_nCurrentNPC) != 0xff)
        pCreature = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(m_nCurrentNPC, 0, 1)));
    else
        pCreature = pApp->GetSWParty()->GetCharacter(0);

    CSWCCreatureStats* pStats = pCreature->m_pStats;
    CSWSCreature* pServer = pCreature->GetServerCreature();
    CSWSCreatureStats* pServerStats = pServer->m_pStats;
    int nClass = pStats->GetClassLeveledUpIn();

    m_lblName.m_text.m_params.SetText(pCreature->GetName());

    if (pStats->m_nNumClasses == 1) {
        m_lblClass1.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(
            g_pRules->m_paClasses[pStats->GetClass(0)].m_nNameStrRef));
        m_lblClass2.m_text.m_params.SetText(CExoString(""));
        m_lblLevel1.m_text.m_params.SetText(CExoString(static_cast<int>(pStats->GetClassLevel(0))));
        m_lblLevel2.m_text.m_params.SetText(CExoString(""));
    } else {
        m_lblClass1.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(
            g_pRules->m_paClasses[pStats->GetClass(1)].m_nNameStrRef));
        m_lblClass2.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(
            g_pRules->m_paClasses[pStats->GetClass(0)].m_nNameStrRef));
        m_lblLevel1.m_text.m_params.SetText(CExoString(static_cast<int>(pStats->GetClassLevel(1))));
        m_lblLevel2.m_text.m_params.SetText(CExoString(static_cast<int>(pStats->GetClassLevel(0))));
    }

    CExoString sText;

    sText.Format(const_cast<char*>("%d/%d"), pStats->m_nCurrentHitPoints, static_cast<short>(pStats->GetMaxHitPoints()));
    m_lblVitalityStat.m_text.m_params.SetText(sText);
    SetLabelBonusPenaltyColour(m_lblVitalityStat, pStats->m_nHitPointBonus);

    m_lblDefenseStat.m_text.m_params.SetText(CExoString(static_cast<int>(static_cast<short>(pStats->GetArmorClass(0)))));
    SetLabelBonusPenaltyColour(m_lblDefenseStat,
                               pCreature->GetServerCreature()->GetTotalACEffectBonus());

    if (CSWClass::IsJedi(static_cast<unsigned char>(nClass))) {
        sText.Format(const_cast<char*>("%d/%d"), static_cast<short>(pStats->m_nCurrentForcePoints + pStats->m_nTemporaryForcePoints),
                     static_cast<short>(pStats->m_nMaxForcePoints));
        m_lblForceStat.m_text.m_params.SetText(sText);
        m_lblForce.m_tail.bVisible = 1;
        m_lblForceStat.m_tail.bVisible = 1;
        SetLabelBonusPenaltyColour(m_lblForceStat, static_cast<short>(pStats->m_nTemporaryForcePoints));
    } else {
        m_lblForce.m_tail.bVisible = 0;
        m_lblForceStat.m_tail.bVisible = 0;
    }

    {
        unsigned char nOld = pServerStats->m_nSTRBase;
        unsigned char nNew = pServerStats->GetSTRStat();
        m_lblStr.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblStrMod, static_cast<signed char>(pServerStats->m_nSTRMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblStr, nDiff);
        SetLabelBonusPenaltyColour(m_lblStrMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nWISBase;
        unsigned char nNew = pServerStats->GetWISStat();
        m_lblWis.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblWisMod, static_cast<signed char>(pServerStats->m_nWISMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblWis, nDiff);
        SetLabelBonusPenaltyColour(m_lblWisMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nCHABase;
        unsigned char nNew = pServerStats->GetCHAStat();
        m_lblCha.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblChaMod, static_cast<signed char>(pServerStats->m_nCHAMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblCha, nDiff);
        SetLabelBonusPenaltyColour(m_lblChaMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nINTBase;
        unsigned char nNew = pServerStats->GetINTStat();
        m_lblInt.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblIntMod, static_cast<signed char>(pServerStats->m_nINTMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblInt, nDiff);
        SetLabelBonusPenaltyColour(m_lblIntMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nCONBase;
        unsigned char nNew = pServerStats->GetCONStat();
        m_lblCon.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblConMod, static_cast<signed char>(pServerStats->m_nCONMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblCon, nDiff);
        SetLabelBonusPenaltyColour(m_lblConMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nDEXBase;
        unsigned char nNew = pServerStats->GetDEXStat();
        m_lblDex.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblDexMod, static_cast<signed char>(pServerStats->GetDEXMod(1)));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblDex, nDiff);

        if (nOld == nNew) {
            int nModDiff = static_cast<signed char>(pServerStats->GetDEXMod(1))
                           - static_cast<signed char>(pServerStats->GetDEXMod(0));
            SetLabelBonusPenaltyColour(m_lblDexMod, nModDiff);
        } else {
            SetLabelBonusPenaltyColour(m_lblDexMod, nDiff);
        }
    }

    int nBonus = 0;
    m_lblFortitudeStat.m_text.m_params.SetText(
        CExoString(static_cast<int>(pServerStats->GetFortSavingThrow(0))));
    int nFortSave = pCreature->GetServerCreature()->GetTotalEffectBonus(
        CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nBonus, 0, 0, SAVING_THROW_FORT, 0,
        CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    SetLabelBonusPenaltyColour(m_lblFortitudeStat, nFortSave);

    m_lblReflexStat.m_text.m_params.SetText(
        CExoString(static_cast<int>(pServerStats->GetReflexSavingThrow(0))));
    int nReflexSave = pCreature->GetServerCreature()->GetTotalEffectBonus(
        CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nBonus, 0, 0, SAVING_THROW_REFLEX, 0,
        CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    SetLabelBonusPenaltyColour(m_lblReflexStat, nReflexSave);

    m_lblWillStat.m_text.m_params.SetText(
        CExoString(static_cast<int>(pServerStats->GetWillSavingThrow(0))));
    int nWillSave = pCreature->GetServerCreature()->GetTotalEffectBonus(
        CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nBonus, 0, 0, SAVING_THROW_WILL, 0,
        CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    SetLabelBonusPenaltyColour(m_lblWillStat, nWillSave);

    m_lblExperienceStat.m_text.m_params.SetText(CExoString(pServerStats->m_nExperience));
    m_lblNeededXp.m_text.m_params.SetText(
        CExoString(static_cast<int>(pServerStats->GetExpNeededForLevelUp())));
    m_lblNeededXp.m_tail.bVisible = (pServerStats->GetExpNeededForLevelUp() != static_cast<unsigned int>(-1));
    m_lblNextLevel.m_tail.bVisible = (pServerStats->GetExpNeededForLevelUp() != static_cast<unsigned int>(-1));

    int nCanLevelUp;
    if (static_cast<unsigned char>(m_nCurrentNPC) != 0xff
        && g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule)
        nCanLevelUp = 0;
    else
        nCanLevelUp = pServerStats->CanLevelUp() != 0;
    m_btnLevelUp.m_tail.bVisible = nCanLevelUp;
    m_btnAuto.m_tail.bVisible = nCanLevelUp;

    int i = 0;
    for (; i < pServer->m_effectIcons.m_count; i++) {
        if (i >= 10)
            break;
        m_alblGood[i].m_border.m_params.SetFillImage(pServer->m_effectIcons.m_pArray[i]->m_resIcon, 0);
        m_alblGood[i].m_tail.bVisible = 1;
    }
    m_lblMore.m_tail.bVisible = pServer->m_effectIcons.m_count > 10;
    for (; i < 10 && m_alblGood[i].m_tail.bVisible; i++)
        m_alblGood[i].m_tail.bVisible = 0;

    if (m_pAppearanceCreature == pCreature
        && m_nAppearanceAlignment == pServerStats->m_nAlignment)
        return;

    CCAppearanceInfo appearance = *reinterpret_cast<CCAppearanceInfo*>(pCreature->m_pCreatureAppearance);
    appearance.core.m_oidHeadItem = OBJECT_INVALID;

    m_pCreature->SetPortrait(pCreature->GetPortrait(0));

    if (g_bDo3DGUIs) {
        m_pCreature->SetAppearance(&appearance, 3, 1);
        m_pCreature->m_pCreatureAppearance->SetAppearanceAlignment(
            pCreature->m_pStats->m_alignmentGoodEvil);

        m_pCreature->AttachToScene(m_view3DChar.m_scene.m_pScene);
        Vector vDirection;
        m_pCreature->SetPosition(vDirection);
        vDirection = Vector(1.0f, 0.0f, 0.0f);
        m_pCreature->SetOrientation(vDirection);
        m_pCreature->Setup3DScene(m_view3DChar, pCreature, 0, 1);
    }

    m_pAppearanceCreature = pCreature;
    m_nAppearanceAlignment = pStats->m_alignmentGoodEvil;
    m_sldAlign.SetCurValue(100 - m_nAppearanceAlignment);
}
#else
void CSWGuiInGameCharacter::SetStats()
{
    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
    m_nPartyCount = pApp->GetSWParty()->m_nCharacterCount;

    CSWCCreature* pCreature = pApp->GetSWParty()->GetCharacter(0);
    CSWCCreatureStats* pStats = pCreature->m_pStats;
    CSWSCreature* pServer = pCreature->GetServerCreature();
    CSWSCreatureStats* pServerStats = pServer->m_pStats;
    int nClass = pStats->GetClassLeveledUpIn();

    CExoString sText;

    sText.Format(const_cast<char*>("%d/%d"), pStats->m_nCurrentHitPoints, static_cast<short>(pStats->GetMaxHitPoints()));
    m_lblVitalityStat.m_text.m_params.SetText(sText);
    SetLabelBonusPenaltyColour(m_lblVitalityStat, pStats->m_nHitPointBonus);

    m_lblDefenseStat.m_text.m_params.SetText(CExoString(static_cast<int>(static_cast<short>(pStats->GetArmorClass(1)))));
    m_lblDefenseStat.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);

    if (CSWClass::IsJedi(static_cast<unsigned char>(nClass))) {
        short nMaxForce = static_cast<short>(pStats->m_nMaxForcePoints);
        int nCurForce = pStats->m_nCurrentForcePoints + pStats->m_nTemporaryForcePoints;
        if (nMaxForce < 1)
            nCurForce = 0;
        sText.Format(const_cast<char*>("%d/%d"), static_cast<short>(nCurForce), nMaxForce);
        m_lblForceStat.m_text.m_params.SetText(sText);
        m_lblForce.m_tail.bVisible = 1;
        m_lblForceStat.m_tail.bVisible = 1;
        SetLabelBonusPenaltyColour(m_lblForceStat, static_cast<short>(pStats->m_nTemporaryForcePoints));
    } else {
        m_lblForce.m_tail.bVisible = 0;
        m_lblForceStat.m_tail.bVisible = 0;
    }

    {
        unsigned char nOld = pServerStats->m_nSTRBase;
        unsigned char nNew = pServerStats->GetSTRStat();
        m_lblStr.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblStrMod, static_cast<signed char>(pServerStats->m_nSTRMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblStr, nDiff);
        SetLabelBonusPenaltyColour(m_lblStrMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nWISBase;
        unsigned char nNew = pServerStats->GetWISStat();
        m_lblWis.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblWisMod, static_cast<signed char>(pServerStats->m_nWISMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblWis, nDiff);
        SetLabelBonusPenaltyColour(m_lblWisMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nCHABase;
        unsigned char nNew = pServerStats->GetCHAStat();
        m_lblCha.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblChaMod, static_cast<signed char>(pServerStats->m_nCHAMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblCha, nDiff);
        SetLabelBonusPenaltyColour(m_lblChaMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nINTBase;
        unsigned char nNew = pServerStats->GetINTStat();
        m_lblInt.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblIntMod, static_cast<signed char>(pServerStats->m_nINTMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblInt, nDiff);
        SetLabelBonusPenaltyColour(m_lblIntMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nCONBase;
        unsigned char nNew = pServerStats->GetCONStat();
        m_lblCon.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblConMod, static_cast<signed char>(pServerStats->m_nCONMod));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblCon, nDiff);
        SetLabelBonusPenaltyColour(m_lblConMod, nDiff);
    }
    {
        unsigned char nOld = pServerStats->m_nDEXBase;
        unsigned char nNew = pServerStats->GetDEXStat();
        m_lblDex.m_text.m_params.SetText(CExoString(static_cast<int>(nNew)));
        SetModText(m_lblDexMod, static_cast<signed char>(pServerStats->GetDEXMod(1)));
        int nDiff = static_cast<int>(nNew) - static_cast<int>(nOld);
        SetLabelBonusPenaltyColour(m_lblDex, nDiff);

        if (nOld == nNew) {
            int nModDiff = static_cast<signed char>(pServerStats->GetDEXMod(1))
                           - static_cast<signed char>(pServerStats->GetDEXMod(0));
            SetLabelBonusPenaltyColour(m_lblDexMod, nModDiff);
        } else {
            SetLabelBonusPenaltyColour(m_lblDexMod, nDiff);
        }
    }

    int nBonus = 0;
    m_lblFortitudeStat.m_text.m_params.SetText(
        CExoString(static_cast<int>(pServerStats->GetFortSavingThrow(0))));
    int nFortSave = pCreature->GetServerCreature()->GetTotalEffectBonus(
        CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nBonus, 0, 0, SAVING_THROW_FORT, 0,
        CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    SetLabelBonusPenaltyColour(m_lblFortitudeStat, nFortSave);

    m_lblReflexStat.m_text.m_params.SetText(
        CExoString(static_cast<int>(pServerStats->GetReflexSavingThrow(0))));
    int nReflexSave = pCreature->GetServerCreature()->GetTotalEffectBonus(
        CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nBonus, 0, 0, SAVING_THROW_REFLEX, 0,
        CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    SetLabelBonusPenaltyColour(m_lblReflexStat, nReflexSave);

    m_lblWillStat.m_text.m_params.SetText(
        CExoString(static_cast<int>(pServerStats->GetWillSavingThrow(0))));
    int nWillSave = pCreature->GetServerCreature()->GetTotalEffectBonus(
        CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nBonus, 0, 0, SAVING_THROW_WILL, 0,
        CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    SetLabelBonusPenaltyColour(m_lblWillStat, nWillSave);

    m_lblExperienceStat.m_text.m_params.SetText(CExoString(pServerStats->m_nExperience));
    m_lblNeededXp.m_text.m_params.SetText(
        CExoString(static_cast<int>(pServerStats->GetExpNeededForLevelUp())));
    m_lblNeededXp.m_tail.bVisible = (pServerStats->GetExpNeededForLevelUp() != static_cast<unsigned int>(-1));
    m_lblNextLevel.m_tail.bVisible = (pServerStats->GetExpNeededForLevelUp() != static_cast<unsigned int>(-1));
    int nCanLevelUp = pServerStats->CanLevelUp();
    m_btnLevelUp.m_tail.bVisible = nCanLevelUp;
    m_btnAuto.m_tail.bVisible = nCanLevelUp;

    if (m_pAppearanceCreature == pCreature
        && m_nAppearanceAlignment == pServerStats->m_nAlignment)
        return;

    CCAppearanceInfo appearance = *reinterpret_cast<CCAppearanceInfo*>(pCreature->m_pCreatureAppearance);
    appearance.core.m_oidHeadItem = OBJECT_INVALID;

    CResRef resPortrait = pCreature->GetPortrait(0);
    m_pCreature->SetPortrait(resPortrait);

    if (g_bDo3DGUIs) {
        m_pCreature->SetAppearance(&appearance, 3, 1, 0);
        m_pCreature->m_pCreatureAppearance->SetAppearanceAlignment(
            pStats->m_alignmentGoodEvil);

        m_pCreature->AttachToScene(m_view3DChar.m_scene.m_pScene);
        Vector vDirection;
        m_pCreature->SetPosition(vDirection);
        vDirection = Vector(1.0f, 0.0f, 0.0f);
        m_pCreature->SetOrientation(vDirection);
        m_pCreature->Setup3DScene(m_view3DChar, pCreature, 0, 1);
    }

    m_pAppearanceCreature = pCreature;
    m_nAppearanceAlignment = pStats->m_alignmentGoodEvil;
    m_sldAlign.SetCurValue(100 - m_nAppearanceAlignment);

    if (pServer->HasEffect(CSWSEffectListHandler::EFFECT_PUREGOODPOWERS)) {
        int nIdx = nClass - CLASS_TYPE_JEDIGUARDIAN;
        if (static_cast<unsigned char>(nIdx) <= CLASS_TYPE_SITHASSASSIN - CLASS_TYPE_JEDIGUARDIAN &&
            ((FORCE_CLASS_BITS >> nIdx) & 1)) {
            m_lblForcemastery.m_tail.bVisible = 1;
            m_lblForcemastery.m_text.m_params.SetStrRef(anForceGoodStrRefs[static_cast<signed char>(nIdx)]);
        } else {
            m_lblForcemastery.m_tail.bVisible = 0;
        }
    } else if (pServer->HasEffect(CSWSEffectListHandler::EFFECT_PUREEVILPOWERS)) {
        int nIdx = nClass - CLASS_TYPE_JEDIGUARDIAN;
        if (static_cast<unsigned char>(nIdx) <= CLASS_TYPE_SITHASSASSIN - CLASS_TYPE_JEDIGUARDIAN &&
            ((FORCE_CLASS_BITS >> nIdx) & 1)) {
            m_lblForcemastery.m_tail.bVisible = 1;
            m_lblForcemastery.m_text.m_params.SetStrRef(anForceEvilStrRefs[static_cast<signed char>(nIdx)]);
        } else {
            m_lblForcemastery.m_tail.bVisible = 0;
        }
    } else {
        m_lblForcemastery.m_tail.bVisible = 0;
    }
}
#endif

#if KOTOR1
// Refreshes the party portraits on the two change-character buttons.
void CSWGuiInGameCharacter::UpdatePortraits()
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
        nStart = m_nCurrentNPC + 1;

    int nFound = 0;
    for (int i = nStart; i < 9; i++) {
        if (i == m_nCurrentNPC)
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

    if (static_cast<unsigned char>(m_nCurrentNPC) != 0xff) {
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
        if (i == m_nCurrentNPC)
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
    m_btnChange1.m_tail.bVisible = nFound;
    m_btnChange2.m_tail.bVisible = 0;
    goto cyclers;
both:
    m_btnChange1.m_tail.bVisible = nFound;
    m_btnChange2.m_tail.bVisible = 1;
cyclers:
    m_btnCharLeft.m_tail.bVisible =
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNumNPCAvailable() > 2;
    m_btnCharRight.m_tail.bVisible =
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNumNPCAvailable() > 2;
}
#else
void CSWGuiInGameCharacter::UpdatePortraits()
{
}
#endif

void CSWGuiInGameCharacter::OnPanelAdded()
{
#if KOTOR1
    m_nCurrentNPC = -1;
    m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-2);
    m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-2);
    SetStats();
    UpdatePortraits();
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
    SetStats();
    CSWGuiPanel::OnPanelAdded();
#endif
}

void CSWGuiInGameCharacter::OnChangeCharacter(CSWGuiControl* pControl)
{
#if KOTOR1
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (!g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
        unsigned char nMember = pControl == &m_btnChange2 ? 2 : 1;
        if (!g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, nMember))
            return;
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
    } else {
        signed char nNPC = static_cast<signed char>(pControl->m_tail.m_nUserData);
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        CSWCCreature* pNPC;
        if (nNPC == -1)
            pNPC = pApp->GetPlayerCreature();
        else
            pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nNPC, 0, 1)));
        if (pNPC) {
            if (!m_btnCharRight.m_tail.bVisible)
                m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-2);
            else if (pControl == &m_btnChange1)
                m_btnChange1.m_tail.m_nUserData = m_btnChange2.m_tail.m_nUserData;
            m_nCurrentNPC = nNPC;
        }
    }
    m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
    UpdatePortraits();
#else
    K2_UNUSED(pControl);
#endif
}

void CSWGuiInGameCharacter::SetLabelBonusPenaltyColour(CSWGuiLabel& lbl, int n)
{
#if KOTOR1
    if (n > 0)
        lbl.m_text.m_params.SetColor(CGuiInGame::COLOR_GREEN);
    else if (n >= 0)
        lbl.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
    else
        lbl.m_text.m_params.SetColor(CGuiInGame::COLOR_RED);
#else
    if (n > 0)
        lbl.m_text.m_params.SetColor(*reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS));
    else if (n >= 0)
        lbl.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
    else
        lbl.m_text.m_params.SetColor(CGuiInGame::COLOR_RED);
#endif
}

__attribute__((weak)) void CSWGuiInGameCharacter::SetModText(CSWGuiLabel& lbl, int n)
{
    lbl.m_text.m_params.SetText(n >= 1 ? CExoString("+") + CExoString(n) : CExoString(n));
}

extern int quaternionidentityoptimization;

// Rotates a vector by a unit quaternion; the identity rotation is short-circuited.
static inline Vector RotateVector(Quaternion q, const Vector& v)
{
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == 1.0f || w == -1.0f))
        return v;
    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, wx = x * w;
    float yz = y * z, wy = y * w;
    float wz = z * w;
    Vector r;
    float sx = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz));
    r.x = v.z * ((xz + wy) + (xz + wy)) + sx;
    float sy = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz)));
    r.y = v.z * ((yz - wx) + (yz - wx)) + sy;
    float sz = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx));
    r.z = v.z * (1.0f - ((xx + yy) + (xx + yy))) + sz;
    return r;
}

void CSWGuiInGameCharacter::RotateCharacterLeft(int nAmount)
{
    K2_UNUSED(nAmount);
    m_pCreature->SetOrientation(
        RotateVector(YawPitchRoll(-10.0f, 0.0f, 0.0f), m_pCreature->m_orientation));
}

void CSWGuiInGameCharacter::RotateCharacterRight(int nAmount)
{
    K2_UNUSED(nAmount);
    m_pCreature->SetOrientation(
        RotateVector(YawPitchRoll(10.0f, 0.0f, 0.0f), m_pCreature->m_orientation));
}

#ifdef K2_FIX_ENGINE_BUGS
void CSWGuiInGameCharacter::OnRotateLeft(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    RotateCharacterLeft(0);
}

void CSWGuiInGameCharacter::OnRotateRight(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    RotateCharacterRight(0);
}
#endif

void CSWGuiInGameCharacter::RotateCharacter(int nAmount)
{
    m_pCreature->SetOrientation(
        RotateVector(YawPitchRoll(nAmount == 0 ? 10.0f : -10.0f, 0.0f, 0.0f),
                     m_pCreature->m_orientation));
}

#if KOTOR1
// Handles the bindings this panel owns, then forwards to the base.
void CSWGuiInGameCharacter::HandleInputEvent(int type, int val)
{
    g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);

    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B: case INPUTEVENT_GUI_START: case INPUTEVENT_GUI_BACK: case INPUTEVENT_GUI:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0) != 0)
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            break;

        case INPUTEVENT_GUI_X: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
            if (pLeader) {
                unsigned long long nLeaderId =
                    g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pLeader->m_objectId);
                m_pScriptSelect->SetCreatureID(nLeaderId);
                m_pGuiManager->AddPanel(m_pScriptSelect, 3, 1);
            }
            break;
        }

#if defined(K1_UI_XBOX)
        case INPUTEVENT_GUI_A:
            OnIosLevelUpButton(0);
            return;

        case INPUTEVENT_GUI_Y:
            OnIosAutoButton(0);
            return;

        case INPUTEVENT_GUI_BLACK:   // RB in menus
        case INPUTEVENT_CHANGE_CHARACTER:
#else
        case INPUTEVENT_GUI_Y: case INPUTEVENT_CHANGE_CHARACTER:
#endif
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            if (!g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
                g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
            } else {
                int i;
                for (i = m_nCurrentNPC + 1; i < 9; i++) {
                    if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
                        continue;
                    CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
                    if (pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
                            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)))) {
                        m_nCurrentNPC = static_cast<char>(i);
                        break;
                    }
                }
                if (i >= 9)
                    m_nCurrentNPC = -1;
            }
            UpdatePortraits();
            break;

        case INPUTEVENT_GUI_SCROLL_LEFT:
            RotateCharacter(1);
            break;

        case INPUTEVENT_GUI_SCROLL_RIGHT:
            RotateCharacter(0);
            break;

        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}
#else
// Handles the bindings this panel owns, then forwards to the base.
void CSWGuiInGameCharacter::HandleInputEvent(int type, int val)
{
    g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);

    if (val != 0) {
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        switch (type) {
        case INPUTEVENT_GUI_A:
            if (pApp->GetInGameGui()->m_levelUpMode != 0
                && g_pAppManager->m_pClientExoApp->GetCharacterChangeInProgress() == 0) {
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                ShowLevelUpGUI(0);
            }
            break;

        case INPUTEVENT_GUI_B: case INPUTEVENT_GUI_START: case INPUTEVENT_GUI_BACK: case INPUTEVENT_GUI:
            if (pApp->GetInGameGui()->HideSWInGameGui(0) != 0)
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            break;

#ifndef K2_UI_XBOX  // xbox: RB alone changes the character
        case INPUTEVENT_GUI_Y:
#endif
        case INPUTEVENT_CHANGE_CHARACTER:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            pApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
            break;

        case INPUTEVENT_GUI_X:
            if (pApp->GetInGameGui()->m_levelUpMode != 0
                && g_pAppManager->m_pClientExoApp->GetCharacterChangeInProgress() == 0) {
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                StartAutoLevelup();
            }
            break;

        case INPUTEVENT_GUI_SCROLL_LEFT:
            RotateCharacter(1);
            break;

        case INPUTEVENT_GUI_SCROLL_RIGHT:
            RotateCharacter(0);
            break;

        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}
#endif

// Reads the next experience-level threshold out of the rules table and asks the player to confirm
// the automatic level-up.
void CSWGuiInGameCharacter::StartAutoLevelup()
{
    CSWCCreature* pCreature =
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    CSWCCreatureStats* pStats = pCreature->m_pStats;
    int nExperience = pStats->m_nExperience;
    unsigned char nLevel = pStats->GetLevel();

    CExoString sText;
    unsigned char nLevelNext = nLevel;
    while (static_cast<unsigned int>(nExperience) >= static_cast<unsigned int>(g_pRules->m_aExpTable[++nLevelNext])) {
    }

    g_pTlkTable->SetCustomToken(0, CExoString(static_cast<int>(nLevel)));
    g_pTlkTable->SetCustomToken(1, CExoString(static_cast<int>(nLevelNext)));
    sText = g_pTlkTable->GetSimpleString(STRREF_CONFIRM_AUTO_LEVEL_UP);
    g_pTlkTable->ParseStr(sText);

    CSWGuiMessageBox* pBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(1);
    pBox->SetMessage(sText);
    pBox->SetCallback(this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameCharacter::DoAutoLevelup), 0);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}

bool CSWGuiInGameCharacter::ShowLevelUpGUI(int nMode)
{
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_levelUpMode == 0
        || g_pAppManager->m_pClientExoApp->GetCharacterChangeInProgress() != 0)
        return false;

    SetStats();

    CSWGuiLevelUpCharGen* pGui = new CSWGuiLevelUpCharGen(m_pGuiManager,
        g_pAppManager->m_pClientExoApp->GetPlayerCreature()->m_objectId, m_pCreature);
#if !KOTOR1  // K2 adds a background replacement panel
    pGui->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
    pGui->m_pInGameChar = this;
    pGui->m_nInputClass = (nMode == 0) ? 2 : 0;
    m_pGuiManager->AddPanel(pGui, 3, 1);
    return true;
}

void CSWGuiInGameCharacter::Draw(float f)
{
    SetStats();
#if !KOTOR1  // K2 resyncs the alignment slider
    m_sldAlign.SetCurValue(m_sldAlign.m_nCurValue);
#endif
    CSWGuiPanel::Draw(f);
}

// Puts the creature on the preview scene and makes the camera translate behavior follow its head
// height.
void CSWGuiInGameCharacter::AttachModel()
{
    if (!g_bDo3DGUIs)
        return;

    m_pCreature->AttachToScene(m_view3DChar.m_scene.m_pScene);

    CAurBehaviorAttachTranslate* pBehavior =
        static_cast<CAurBehaviorAttachTranslate*>(m_view3DChar.m_scene.m_pCamera->GetBehavior(kBehaviorAttachTranslate));
    if (pBehavior != 0) {
        pBehavior->m_bUnrotatedTranslation = 1;
        float fHeadHeight = m_pCreature->GetHeadHeightOffset();
        pBehavior->m_vTranslation.x = 0.0f;
        pBehavior->m_vTranslation.y = 0.0f;
        pBehavior->m_vTranslation.z = fHeadHeight;
    }
}

void CSWGuiInGameCharacter::DoAutoLevelup(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted) {
        CSWSCreature* pServer = g_pAppManager->m_pClientExoApp->GetSWParty()
                                    ->GetPlayerCharacter()
                                    ->GetServerCreature();
        if (pServer != 0)
            pServer->m_pStats->AutoLevelup(0);
    }
}

#if KOTOR1
void CSWGuiInGameCharacter::OnSwitchLeft(CSWGuiControl* pControl)
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
        if (i == m_nCurrentNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pNPC)
            goto found;
    }
    if (static_cast<unsigned char>(m_nCurrentNPC) != 0xff && static_cast<char>(m_btnChange2.m_tail.m_nUserData) != -1) {
        m_btnChange1.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-1);
        return;
    }
    for (i = 8; i > nCur; i--) {
        if (i == m_nCurrentNPC)
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

void CSWGuiInGameCharacter::OnSwitchRight(CSWGuiControl* pControl)
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
        if (i == m_nCurrentNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        pNPC = pApp->GetCreatureByGameObjectID(pApp->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pNPC)
            goto found;
    }
    if (static_cast<unsigned char>(m_nCurrentNPC) != 0xff) {
        m_btnChange2.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange2.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-1);
        return;
    }
    for (i = 0; i < nCur; i++) {
        if (i == m_nCurrentNPC)
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

void CSWGuiInGameCharacter::OnIosAutoButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_levelUpMode != 0
        && g_pAppManager->m_pClientExoApp->GetCharacterChangeInProgress() == 0) {
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        StartAutoLevelup();
    }
}

void CSWGuiInGameCharacter::OnIosLevelUpButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_levelUpMode != 0
        && g_pAppManager->m_pClientExoApp->GetCharacterChangeInProgress() == 0) {
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        ShowLevelUpGUI(0);
    }
}
#endif

// Draws the text four times at one-pixel offsets to fake a black outline, then restores the
// colour and draws the label on top.
void CSWGuiBlackenedLabel::Draw(float fAlpha)
{
    CSWGuiExtent extent = m_text.m_extent;
    Vector color = m_text.m_params.m_color;
    m_text.m_params.SetColor(CGuiInGame::COLOR_BLACK);

    extent.x -= 1;
    extent.y += 1;
    m_text.SetExtent(extent);
    m_text.Draw(fAlpha);

    extent.x += 2;
    m_text.SetExtent(extent);
    m_text.Draw(fAlpha);

    extent.y -= 2;
    m_text.SetExtent(extent);
    m_text.Draw(fAlpha);

    extent.x -= 2;
    m_text.SetExtent(extent);
    m_text.Draw(fAlpha);

    m_text.m_params.SetColor(color);
    extent.y += 1;
    extent.x += 1;
    m_text.SetExtent(extent);
    CSWGuiLabel::Draw(fAlpha);
}

// Same outline trick as CSWGuiBlackenedLabel::Draw, finishing with the button draw.
void CSWGuiBlackenedButton::Draw(float fAlpha)
{
    CSWGuiExtent extent = m_text.m_extent;
    Vector color = m_text.m_params.m_color;
    m_text.m_params.SetColor(CGuiInGame::COLOR_BLACK);

    extent.x -= 1;
    extent.y += 1;
    m_text.SetExtent(extent);
    m_text.Draw(fAlpha);

    extent.x += 2;
    m_text.SetExtent(extent);
    m_text.Draw(fAlpha);

    extent.y -= 2;
    m_text.SetExtent(extent);
    m_text.Draw(fAlpha);

    extent.x -= 2;
    m_text.SetExtent(extent);
    m_text.Draw(fAlpha);

    m_text.m_params.SetColor(color);
    extent.y += 1;
    extent.x += 1;
    m_text.SetExtent(extent);
    CSWGuiButton::Draw(fAlpha);
}

