#include "k2stdint.h"
#include "port/sdlapi.h"
#define ASPYR_FLOATINGICON_CTOR_OUT_OF_LINE 1

#include "swguiingame.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "guiingame.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "swguimaininterfacepc2.h"
#include "partytables.h"
#include "nwsitem.h"
#include "swparty.h"
#include "nwcjournal.h"
#include "nwccreature.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwupdatestats.h"
#include "nwsrules.h"
#include "nwclass.h"
#include "nwtlktable.h"
#include "exoresman.h"
#include "exostring.h"
#include "swguicontrol.h"
#include "exotlktable.h"
#include "swguiexamine.h"
#include "CExoArrayListImpl.h"
#include "globalvartables.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#include "nwsobject.h"
#include "nwscript.h"
#include "swguisaveload.h"
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <string>
#include <vector>
#include "port/k2qolapi.h"
#include "tutorial.h"
#include "nwscriptconst.h"
#include "enginevfx.h"
#include "strref.h"
#include "exostringinline.h"


#if KOTOR1
extern bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
#endif

// The gamepad glyphs AspyrAddStringIcon prefixes, A through the right shoulder, in the fonts.
enum {
    PAD_ICON_GLYPH_FIRST = 0x10,
    PAD_ICON_GLYPH_LAST = 0x16,
};

// The message log's channel bits (nwchandlemessage.cpp: 1 talk, 2 tell, 4 shout, 8 whisper,
// 0x40 party); engine feedback lines use this one.
enum {
    MSG_CHANNEL_FEEDBACK = 0x80,
};

// The UTF-8 byte order mark, and the two-byte sequence bits the tool-tip text loader decodes.
enum {
    UTF8_BOM_1 = 0xef,
    UTF8_BOM_2 = 0xbb,
    UTF8_BOM_3 = 0xbf,
    UTF8_LEAD_BYTE_MIN = 0xc0,
    UTF8_CONTINUATION_BITS = 0x3f,
};

CSWGuiInGameMenu::CSWGuiInGameMenu(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: the "top" layout, plates and buttons only, no readout, portrait or party-change rows.
    StartLoadFromLayout(CResRef("top"));
    InitControl(m_lblhEquip, CExoString("LBLH_EQU"), 1);
    InitControl(m_lblhInv, CExoString("LBLH_INV"), 1);
    InitControl(m_lblhChar, CExoString("LBLH_CHA"), 1);
    InitControl(m_lblhMap, CExoString("LBLH_MAP"), 1);
    InitControl(m_lblhAbi, CExoString("LBLH_ABI"), 1);
    InitControl(m_lblhJou, CExoString("LBLH_JOU"), 1);
    InitControl(m_lblhOpt, CExoString("LBLH_OPT"), 1);
    InitControl(m_lblhMsg, CExoString("LBLH_MSG"), 1);
    InitControl(m_btnEquip, CExoString("BTN_EQU"), 1);
    InitControl(m_btnInv, CExoString("BTN_INV"), 1);
    InitControl(m_btnChar, CExoString("BTN_CHAR"), 1);
    InitControl(m_btnMap, CExoString("BTN_MAP"), 1);
    InitControl(m_btnAbi, CExoString("BTN_ABI"), 1);
    InitControl(m_btnJou, CExoString("BTN_JOU"), 1);
    InitControl(m_btnOpt, CExoString("BTN_OPT"), 1);
    InitControl(m_btnMsg, CExoString("BTN_MSG"), 1);
    StopLoadFromLayout();

    m_lblhEquip.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhInv.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhChar.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhMap.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhAbi.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhJou.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhOpt.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhMsg.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhEquip.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhInv.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhChar.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhMap.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhAbi.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhJou.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhOpt.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhMsg.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));

    m_btnEquip.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameMenu::OnEquipButtonPressed));
    m_btnInv.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnInvButtonPressed));
    m_btnChar.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnCharButtonPressed));
    m_btnMap.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnMapButtonPressed));
    m_btnAbi.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnAbiButtonPressed));
    m_btnJou.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnJouButtonPressed));
    m_btnOpt.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnOptButtonPressed));
    m_btnMsg.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnMsgButtonPressed));

    m_btnEquip.m_tail.bFocusOnHover = 0;
    m_btnInv.m_tail.bFocusOnHover = 0;
    m_btnChar.m_tail.bFocusOnHover = 0;
    m_btnMap.m_tail.bFocusOnHover = 0;
    m_btnAbi.m_tail.bFocusOnHover = 0;
    m_btnJou.m_tail.bFocusOnHover = 0;
    m_btnOpt.m_tail.bFocusOnHover = 0;
    m_btnMsg.m_tail.bFocusOnHover = 0;
    SetActiveControl(&m_btnEquip, 0);
    ScaleExtentForResolution();

    CSWGuiExtent extent = m_extent;
    extent.x += (g_SDL_mode.w - static_cast<int>(g_SDL_mode.h / 768.0f * 1024.0f)) / 2;
    SetExtent(extent);

    m_btnEquip.m_nToolTipStrRef = STRREF_MENU_EQUIP;
    m_btnEquip.m_nBoundInputEventId = INPUTEVENT_PANEL_EQUIP;
    m_btnInv.m_nToolTipStrRef = STRREF_MENU_INVENTORY;
    m_btnInv.m_nBoundInputEventId = INPUTEVENT_PANEL_INVENTORY;
    m_btnChar.m_nToolTipStrRef = STRREF_MENU_CHARACTER_SHEET;
    m_btnChar.m_nBoundInputEventId = INPUTEVENT_PANEL_CHARACTER;
    m_btnMap.m_nToolTipStrRef = STRREF_MENU_MAP;
    m_btnMap.m_nBoundInputEventId = INPUTEVENT_PANEL_MAP;
    m_btnAbi.m_nToolTipStrRef = STRREF_MENU_ABILITIES;
    m_btnAbi.m_nBoundInputEventId = INPUTEVENT_PANEL_ABILITIES;
    m_btnJou.m_nToolTipStrRef = STRREF_MENU_QUESTS;
    m_btnJou.m_nBoundInputEventId = INPUTEVENT_PANEL_JOURNAL;
    m_btnOpt.m_nToolTipStrRef = STRREF_MENU_OPTIONS;
    m_btnOpt.m_nBoundInputEventId = INPUTEVENT_PANEL_OPTIONS;
    m_btnMsg.m_nToolTipStrRef = STRREF_MENU_MESSAGES;
    m_btnMsg.m_nBoundInputEventId = INPUTEVENT_PANEL_MESSAGES;
#else
    StartLoadFromLayout(CResRef("top_p"), 1, 1, 1);
    InitControl(m_lblhEquip, CExoString("LBLH_EQU"), 1, 1, 0, 0, 0);
    InitControl(m_lblhInv, CExoString("LBLH_INV"), 1, 1, 0, 0, 0);
    InitControl(m_lblhChar, CExoString("LBLH_CHA"), 1, 1, 0, 0, 0);
    InitControl(m_lblhMap, CExoString("LBLH_MAP"), 1, 1, 0, 0, 0);
    InitControl(m_lblhAbi, CExoString("LBLH_ABI"), 1, 1, 0, 0, 0);
    InitControl(m_lblhJou, CExoString("LBLH_JOU"), 1, 1, 0, 0, 0);
    InitControl(m_lblhOpt, CExoString("LBLH_OPT"), 1, 1, 0, 0, 0);
    InitControl(m_lblhMsg, CExoString("LBLH_MSG"), 1, 1, 0, 0, 0);
    InitControl(m_btnEquip, CExoString("BTN_EQU"), 1, 1, 0, 0, 0);
    InitControl(m_btnInv, CExoString("BTN_INV"), 1, 1, 0, 0, 0);
    InitControl(m_btnChar, CExoString("BTN_CHAR"), 1, 1, 0, 0, 0);
    InitControl(m_btnMap, CExoString("BTN_MAP"), 1, 1, 0, 0, 0);
    InitControl(m_btnAbi, CExoString("BTN_ABI"), 1, 1, 0, 0, 0);
    InitControl(m_btnJou, CExoString("BTN_JOU"), 1, 1, 0, 0, 0);
    InitControl(m_btnOpt, CExoString("BTN_OPT"), 1, 1, 0, 0, 0);
    InitControl(m_btnMsg, CExoString("BTN_MSG"), 1, 1, 0, 0, 0);
    InitControl(m_lblTopClass1, CExoString("LBL_TOP_CLASS1"), 1, 1, 0, 0, 0);
    InitControl(m_lblTopClass2, CExoString("LBL_TOP_CLASS2"), 1, 1, 0, 0, 0);
    InitControl(m_lblTopClass1Level, CExoString("LBL_TOP_CLASS1LEVEL"), 1, 1, 0, 0, 0);
    InitControl(m_lblTopClass2Level, CExoString("LBL_TOP_CLASS2LEVEL"), 1, 1, 0, 0, 0);
    InitControl(m_lblCharName, CExoString("LBL_CHARNAME"), 1, 1, 0, 0, 0);
    // The readout is display only, so setting this flag takes the five labels out of mouse
    // hit-testing.
    m_lblTopClass1.m_tail.bIgnoreMouse = 1;
    m_lblTopClass2.m_tail.bIgnoreMouse = 1;
    m_lblTopClass1Level.m_tail.bIgnoreMouse = 1;
    m_lblTopClass2Level.m_tail.bIgnoreMouse = 1;
    m_lblCharName.m_tail.bIgnoreMouse = 1;
    // Initialize takes CSWGuiMainInterface*, and this panel is that class's sibling rather than a
    // subclass; the pointer is only ever used as the owner to add controls to.
    m_charSlot.Initialize(reinterpret_cast<CSWGuiMainInterface*>(this), 1, 0);  // idiom-exempt: the cast is the original's own, pinned by the callee's mangled name
    InitControl(m_lblChar2, CExoString("LBL_CHAR2"), 1, 1, 0, 0, 0);
    InitControl(m_lblLevelUp2, CExoString("LBL_LEVELUP2"), 1, 1, 0, 0, 0);
    InitControl(m_lblBack2, CExoString("LBL_BACK2"), 1, 1, 0, 0, 0);
    InitControl(m_btnChange2, CExoString("BTN_CHANGE2"), 1, 1, 0, 0, 0);
    InitControl(m_lblChar3, CExoString("LBL_CHAR3"), 1, 1, 0, 0, 0);
    InitControl(m_lblLevelUp3, CExoString("LBL_LEVELUP3"), 1, 1, 0, 0, 0);
    InitControl(m_lblBack3, CExoString("LBL_BACK3"), 1, 1, 0, 0, 0);
    InitControl(m_btnChange3, CExoString("BTN_CHANGE3"), 1, 1, 0, 0, 0);
    InitControl(m_lblSecTitle, CExoString("LBL_SECTITLE"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    m_btnChange2.m_tail.bFocusOnHover = 0;
    m_btnChange2.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameMenu::OnChangeCharacter));
    m_btnChange3.m_tail.bFocusOnHover = 0;
    m_btnChange3.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameMenu::OnChangeCharacter));
    m_bChangeButtonsDisabled = 0;

    // Every highlight plate pages the whole screen list, so the shoulder handlers go on all eight.
    m_lblhEquip.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhInv.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhChar.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhMap.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhAbi.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhJou.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhOpt.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhMsg.AddEvent(INPUTEVENT_GUI_LEFT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderLeft));
    m_lblhEquip.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhInv.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhChar.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhMap.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhAbi.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhJou.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhOpt.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));
    m_lblhMsg.AddEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnShoulderRight));

    m_btnEquip.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameMenu::OnEquipButtonPressed));
    m_btnInv.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnInvButtonPressed));
    m_btnChar.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameMenu::OnCharButtonPressed));
    m_btnMap.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnMapButtonPressed));
    m_btnAbi.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnAbiButtonPressed));
    m_btnJou.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnJouButtonPressed));
    m_btnOpt.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnOptButtonPressed));
    m_btnMsg.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameMenu::OnMsgButtonPressed));

    m_btnEquip.m_tail.bFocusOnHover = 0;
    m_btnInv.m_tail.bFocusOnHover = 0;
    m_btnChar.m_tail.bFocusOnHover = 0;
    m_btnMap.m_tail.bFocusOnHover = 0;
    m_btnAbi.m_tail.bFocusOnHover = 0;
    m_btnJou.m_tail.bFocusOnHover = 0;
    m_btnOpt.m_tail.bFocusOnHover = 0;
    m_btnMsg.m_tail.bFocusOnHover = 0;
    SetActiveControl(&m_btnEquip, 0);

    // Tooltip strref and the input event whose bound key name is appended to it.
    m_btnEquip.m_nToolTipStrRef = STRREF_MENU_EQUIP;
    m_btnEquip.m_nBoundInputEventId = INPUTEVENT_PANEL_EQUIP;
    m_btnInv.m_nToolTipStrRef = STRREF_MENU_INVENTORY;
    m_btnInv.m_nBoundInputEventId = INPUTEVENT_PANEL_INVENTORY;
    m_btnChar.m_nToolTipStrRef = STRREF_MENU_CHARACTER;
    m_btnChar.m_nBoundInputEventId = INPUTEVENT_PANEL_CHARACTER;
    m_btnMap.m_nToolTipStrRef = STRREF_MENU_MAP;
    m_btnMap.m_nBoundInputEventId = INPUTEVENT_PANEL_MAP;
    m_btnAbi.m_nToolTipStrRef = STRREF_MENU_ABILITIES;
    m_btnAbi.m_nBoundInputEventId = INPUTEVENT_PANEL_ABILITIES;
    m_btnJou.m_nToolTipStrRef = STRREF_MENU_JOURNAL;
    m_btnJou.m_nBoundInputEventId = INPUTEVENT_PANEL_JOURNAL;
    m_btnOpt.m_nToolTipStrRef = STRREF_MENU_OPTIONS;
    m_btnOpt.m_nBoundInputEventId = INPUTEVENT_PANEL_OPTIONS;
    m_btnMsg.m_nToolTipStrRef = STRREF_MENU_PARTY_SELECTION;
    m_btnMsg.m_nBoundInputEventId = INPUTEVENT_PANEL_MESSAGES;
    m_nTopMenuOverrideChar = -1;

#ifdef K2_UI_XBOX
    // Xbox: the glyph sits flush with the portrait's bottom-right corner.
    const CSWGuiExtent& portrait = m_charSlot.m_aLabels[4].m_extent;
    // RB cycles the party member.
#ifdef K2_HOST_BUILD
    // The glyph covers a third of the portrait's width, at any screen size.
    const int nGlyph = portrait.w / 3;
    m_iconPartySwap.InitAndAdd(this, 'r', portrait, portrait.w - nGlyph, portrait.h - nGlyph);
    m_iconSoloMode.InitAndAdd(this, 'r', portrait, portrait.w - nGlyph, portrait.h - nGlyph);
    CSWGuiExtent glyph = m_iconPartySwap.m_extent;
    glyph.w = nGlyph;
    glyph.h = nGlyph;
    m_iconPartySwap.SetExtent(glyph);
    m_iconSoloMode.SetExtent(glyph);
    m_iconPartySwap.m_border.m_params.m_flags.nFillStyle = 2;
    m_iconPartySwap.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    m_iconSoloMode.m_border.m_params.m_flags.nFillStyle = 2;
    m_iconSoloMode.m_borderHilight.m_params.m_flags.nFillStyle = 2;
#else
    m_iconPartySwap.InitAndAdd(this, 'r', portrait, portrait.w - 48, portrait.h - 48);
    m_iconSoloMode.InitAndAdd(this, 'r', portrait, portrait.w - 48, portrait.h - 48);
#endif
#else
    m_iconPartySwap.InitAndAdd(this, 'y', m_lblChar2.m_extent, 0, -50);
    m_iconSoloMode.InitAndAdd(this, 'y', m_lblChar2.m_extent, 0, -100);
#endif
#endif
}

#if !KOTOR1  // K1: implicit destructor
CSWGuiInGameMenu::~CSWGuiInGameMenu()
{
}

void CSWGuiInGameMenu::OnPanelAdded()
{
}
#endif

// nIndex addresses the panel's own control list, not a GFF id.
void CSWGuiInGameMenu::SetActiveControlID(int nIndex, int flag)
{
    if (nIndex >= 0 && nIndex < m_controls.m_count)
        SetActiveControl(m_controls.m_pArray[nIndex], flag);
}

#if !KOTOR1  // K1: no party-change buttons
void CSWGuiInGameMenu::ShowChangeButtons(int bShow)
{
    m_btnChange2.m_tail.bVisible = static_cast<unsigned char>(bShow);
    m_btnChange2.SetEnabled(bShow);
    m_btnChange3.m_tail.bVisible = static_cast<unsigned char>(bShow);
    m_btnChange3.SetEnabled(bShow);
}
#endif // !KOTOR1

// The shoulder buttons page the whole in-game screen list; re-entering this handler is what
// delivers them to the highlight plates.
void CSWGuiInGameMenu::HandleInputEvent(int nType, int nVal)
{
    if (nType == INPUTEVENT_PREV_MENU)
    {
        if (nVal & 1)
            HandleInputEvent(INPUTEVENT_GUI_LEFT_SHOULDER, 1);
    }
    else if (nType == INPUTEVENT_NEXT_MENU)
    {
        if (nVal & 1)
            HandleInputEvent(INPUTEVENT_GUI_RIGHT_SHOULDER, 1);
    }
    CSWGuiPanel::HandleInputEvent(nType, nVal);
}

#if !KOTOR1  // K1: no party-change buttons
void CSWGuiInGameMenu::OnChangeCharacter(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (m_bChangeButtonsDisabled != 0)
    {
        // Ask before swapping with the shared "leave solo mode?" box, OK only, no callback.
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->m_bAllowCancel = 0;
        pBox->m_nOpenSoundIndex = GUISOUND_ERROR_DEFAULT;
        pBox->m_btnOk.m_tail.bVisible = pBox->m_bAllowDismissal;
        pBox->m_btnCancel.m_tail.bVisible = pBox->m_bAllowCancel && pBox->m_bAllowDismissal;
        pBox->m_lblIcon.m_tail.bVisible = pBox->m_bHasIcon;
        pBox->m_pfnCallback = 0;
        pBox->m_pCallbackObject = 0;
        pBox->m_pCallbackControl = 0;
        pBox->SetMessage(STRREF_PARTY_ORDER_LOCKED);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    // The lower of the two rows steps two party members along instead of one.
    if (g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(
            0, 0, static_cast<unsigned char>(pControl == &m_btnChange3 ? 2 : 1)) != 0)
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateAbilitiesCharacter();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateInventory();
}
#endif // !KOTOR1

void CSWGuiInGameMenu::OnShoulderLeft(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->PrevSWInGameGui();
}

void CSWGuiInGameMenu::OnShoulderRight(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->NextSWInGameGui();
}

void CSWGuiInGameMenu::OnEquipButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SwitchToSWInGameGui(0);
}

void CSWGuiInGameMenu::OnInvButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SwitchToSWInGameGui(1);
}

void CSWGuiInGameMenu::OnCharButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SwitchToSWInGameGui(2);
}

void CSWGuiInGameMenu::OnMapButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SwitchToSWInGameGui(6);
}

void CSWGuiInGameMenu::OnAbiButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SwitchToSWInGameGui(3);
}

void CSWGuiInGameMenu::OnJouButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SwitchToSWInGameGui(5);
}

void CSWGuiInGameMenu::OnOptButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SwitchToSWInGameGui(7);
}

void CSWGuiInGameMenu::OnMsgButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SwitchToSWInGameGui(4);
}

#if !KOTOR1  // K1 has no Draw override (the base panel draws the bar)
void CSWGuiInGameMenu::Draw(float f)
{
    // Normally the party leader's portrait; CGuiInGame::SetTopMenuOverrideCharacter can pin it
    // to a party-table NPC.
    CSWCCreature* pCreature;
    if (m_nTopMenuOverrideChar != -1)
    {
        pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(
                    m_nTopMenuOverrideChar, 0, 1)));
        if (pCreature == 0)
            return;
    }
    else
    {
        pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    }

    CSWSCreature* pServer = pCreature->GetServerCreature();
    CSWSCreatureStats* pStats = pServer->m_pStats;
    m_charSlot.m_aLabels[4].m_border.m_params.SetFillImage(pCreature->GetPortrait(1), 0);

    short nCurHP = pServer->GetCurrentHitPoints(0);
    short nMaxHP = pServer->GetMaxHitPoints(1);
    m_charSlot.SetHealth(nCurHP, nMaxHP, pServer->m_bPoisoned);

    short nCurFP = static_cast<short>(pStats->m_nCurrentForcePoints + pStats->m_nTemporaryForcePoints);
    short nMaxFP = pServer->GetMaxForcePoints();
    m_charSlot.SetForce(nCurFP, nMaxFP);

    // The portrait's state overlay, and (only in the ordinary state) how many of the creature's
    // live effect icons are beneficial vs harmful.
    unsigned char nState;
    unsigned char nGoodIcons = 0;
    unsigned char nBadIcons = 0;
    if (pServer->GetDead() ||
        pServer->GetDeadTemp() != 0)
    {
        nState = 1;
    }
    else if (pStats->CanLevelUp())
    {
        nState = 4;
    }
    else if (pServer->IsDebilitated(0))
    {
        nState = 2;
    }
    else
    {
        nState = 0;
        for (int i = 0; i < pServer->m_effectIcons.m_count; i++)
        {
            if (pServer->m_effectIcons.m_pArray[i]->m_bGood != 0)
                nGoodIcons++;
            else
                nBadIcons++;
        }
    }

    CExoString sName = pCreature->GetName();
    int nTag = sName.Find('<', 0);
    int nToken = sName.Find('{', 0);
    if (nTag != -1 || nToken != -1)
    {
        g_pSWTlkTable->m_pSpeakerObj = pCreature;
        g_pSWTlkTable->m_gender = g_pSWTlkTable->GetObjectGender();
        g_pSWTlkTable->m_bSuppressEnabled |= 1;
        g_pSWTlkTable->ParseStr(sName);
    }
    m_lblCharName.m_text.m_params.SetText(sName);

    // The class/level readout. A single-class character leaves the second pair blank.
    if (pStats->m_numClasses == 1)
    {
        m_lblTopClass1.m_text.m_params.SetText(
            g_pAppManager->m_pClientExoApp->GetGUIString(
                g_pRules->m_paClasses[pStats->m_classInfo[0].m_class].m_nNameStrRef));
        m_lblTopClass1Level.m_text.m_params.SetText(CExoString(pStats->GetClassLevel(0, 0)));
        m_lblTopClass2.m_text.m_params.SetText(CExoString(""));
        m_lblTopClass2Level.m_text.m_params.SetText(CExoString(""));
    }
    else
    {
        m_lblTopClass1.m_text.m_params.SetText(
            g_pAppManager->m_pClientExoApp->GetGUIString(
                g_pRules->m_paClasses[pStats->GetClass(0)].m_nNameStrRef));
        m_lblTopClass1Level.m_text.m_params.SetText(CExoString(pStats->GetClassLevel(0, 0)));
        m_lblTopClass2.m_text.m_params.SetText(
            g_pAppManager->m_pClientExoApp->GetGUIString(
                g_pRules->m_paClasses[pStats->GetClass(1)].m_nNameStrRef));
        m_lblTopClass2Level.m_text.m_params.SetText(CExoString(pStats->GetClassLevel(1, 0)));
    }

    m_charSlot.Show(1, nState, nGoodIcons, nBadIcons);
    CSWGuiPanel::Draw(f);
    m_charSlot.DrawEffects(f, 1);

    // The two party-swap rows track the party size.
    unsigned int nPartySize = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
    if (nPartySize <= 1)
    {
        m_lblChar2.m_tail.bVisible = 0;
        m_lblBack2.m_tail.bVisible = 0;
        m_lblLevelUp2.m_tail.bVisible = 0;
        m_btnChange2.m_tail.bVisible = 0;
        m_lblChar3.m_tail.bVisible = 0;
        m_lblLevelUp3.m_tail.bVisible = 0;
        m_lblBack3.m_tail.bVisible = 0;
        m_btnChange3.m_tail.bVisible = 0;
    }
    else if (nPartySize == 2)
    {
        CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(1);
        CSWSCreature* pMemberServer = pMember->GetServerCreature();
        m_lblChar2.m_border.m_params.SetFillImage(pMember->GetPortrait(1), 0);
        m_lblChar2.m_tail.bVisible = 1;
        m_lblBack2.m_tail.bVisible = 1;
        if (pMemberServer->m_pStats->CanLevelUp())
            m_lblLevelUp2.m_tail.bVisible = 1;
        else
            m_lblLevelUp2.m_tail.bVisible = 0;
        m_btnChange2.m_tail.bVisible = 1;
        m_lblChar3.m_tail.bVisible = 0;
        m_lblLevelUp3.m_tail.bVisible = 0;
        m_lblBack3.m_tail.bVisible = 0;
        m_btnChange3.m_tail.bVisible = 0;
    }
    else if (nPartySize == 3)
    {
        CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(1);
        CSWSCreature* pMemberServer = pMember->GetServerCreature();
        m_lblChar2.m_border.m_params.SetFillImage(pMember->GetPortrait(1), 0);
        m_lblChar2.m_tail.bVisible = 1;
        m_lblBack2.m_tail.bVisible = 1;
        if (pMemberServer->m_pStats->CanLevelUp())
            m_lblLevelUp2.m_tail.bVisible = 1;
        else
            m_lblLevelUp2.m_tail.bVisible = 0;
        m_btnChange2.m_tail.bVisible = 1;
        pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(2);
        pMemberServer = pMember->GetServerCreature();
        m_lblChar3.m_border.m_params.SetFillImage(pMember->GetPortrait(1), 0);
        m_lblChar3.m_tail.bVisible = 1;
        m_lblBack3.m_tail.bVisible = 1;
        if (pMemberServer->m_pStats->CanLevelUp())
            m_lblLevelUp3.m_tail.bVisible = 1;
        else
            m_lblLevelUp3.m_tail.bVisible = 0;
        m_btnChange3.m_tail.bVisible = 1;
    }
}
#endif // !KOTOR1

// Pad-glyph helpers, declared here because no header carries them.
//
// A glyph prefix is a control byte followed by a space: 0x10 for A, 0x11 for B, 0x12 for X, 0x13
// for Y and 0x14 for D. The Add pair prepends one; the Strip pair removes it only when the string
// really starts with one.
void AspyrAddStringIcon(CExoString& s, char nIcon);
void AspyrStripStringIcon(CExoString& s, char nIcon);
void AspyrAddLabelIcon(CSWGuiLabel* pLabel, char nIcon);
void AspyrStripButtonIcon(CSWGuiButton* pButton, char nIcon);

// Every other letter falls through with no prefix added.
void AspyrAddStringIcon(CExoString& s, char nIcon)
{
#ifdef K2_UI_PC
    // pc: the fonts have no pad glyphs, so the prefix is a blank that pushes the caption off centre
    K2_UNUSED(s);
    K2_UNUSED(nIcon);
#else
    CExoString sIcon;
    switch (nIcon)
    {
    case 'a': sIcon = "\x10 "; break;
    case 'd': sIcon = "\x14 "; break;
    case 'b': sIcon = "\x11 "; break;
    case 'x': sIcon = "\x12 "; break;
    case 'y': sIcon = "\x13 "; break;
#ifdef K2_QOL_CONTROLLER
    case 'l': sIcon = "\x15 "; break;   // the shoulder buttons
    case 'r': sIcon = "\x16 "; break;
#endif
    }
    s = sIcon + s;
#endif
}

// nIcon is unused: the strip is unconditional on which glyph it was.
void AspyrStripStringIcon(CExoString& s, char nIcon)
{
    K2_UNUSED(nIcon);
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.CStr());
    if (p != 0 && *p != 0 && static_cast<unsigned int>(*p - PAD_ICON_GLYPH_FIRST) <= PAD_ICON_GLYPH_LAST - PAD_ICON_GLYPH_FIRST && p[1] != 0)
        s = CExoString(reinterpret_cast<const char*>(p + 2));
}

// Copy the label's caption out, prefix it, put it back.
void AspyrAddLabelIcon(CSWGuiLabel* pLabel, char nIcon)
{
    CExoString sText(pLabel->m_text.m_params.m_sText);
    AspyrAddStringIcon(sText, nIcon);
    pLabel->m_text.m_params.SetText(sText);
}

// Strips the icon prefix from a button's caption with the same guard as AspyrStripStringIcon, but
// on a copy of the caption so the control's text is untouched when no prefix is present.
void AspyrStripButtonIcon(CSWGuiButton* pButton, char nIcon)
{
    K2_UNUSED(nIcon);
    CExoString sText(pButton->m_text.m_params.m_sText);
    const unsigned char* p = reinterpret_cast<const unsigned char*>(sText.CStr());
    if (p != 0 && *p != 0 && static_cast<unsigned int>(*p - PAD_ICON_GLYPH_FIRST) <= PAD_ICON_GLYPH_LAST - PAD_ICON_GLYPH_FIRST && p[1] != 0)
        sText = CExoString(reinterpret_cast<const char*>(p + 2));
    pButton->m_text.m_params.SetText(sText);
}

AspyrFloatingButtonIcon::AspyrFloatingButtonIcon() {}

bool IsJoystickConnected();

void AspyrFloatingButtonIcon::Draw(float f)
{
    if (IsJoystickConnected())
        CSWGuiButton::Draw(f);
}

bool IsPsButtonIcons();

void AspyrFloatingButtonIcon::InitAndAdd(CSWGuiPanel* pPanel, char nIcon, CSWGuiExtent extent,
                                         int nOffsetX, int nOffsetY)
{
    CResRef resRef;
    if (IsPsButtonIcons())
    {
        switch (nIcon)
        {
        case 'a': resRef = CResRef("cus_button_aps"); break;
        case 'b': resRef = CResRef("cus_button_bps"); break;
        case 'x': resRef = CResRef("cus_button_xps"); break;
        case 'y': resRef = CResRef("cus_button_yps"); break;
#ifdef K2_QOL_CONTROLLER
        case 'l': resRef = CResRef("cus_button_lps"); break;
        case 'r': resRef = CResRef("cus_button_rps"); break;
#endif
        }
    }
    else
    {
        switch (nIcon)
        {
        case 'a': resRef = CResRef("cus_button_aan"); break;
        case 'b': resRef = CResRef("cus_button_ban"); break;
        case 'x': resRef = CResRef("cus_button_xan"); break;
        case 'y': resRef = CResRef("cus_button_yan"); break;
#ifdef K2_QOL_CONTROLLER
        case 'l': resRef = CResRef("cus_button_lan"); break;
        case 'r': resRef = CResRef("cus_button_ran"); break;
#endif
        }
    }

    // A fixed 48x48 glyph, offset from the extent it is anchored to.
    CSWGuiExtent iconExtent;
    iconExtent.x = extent.x + nOffsetX;
    iconExtent.y = extent.y + nOffsetY;
    iconExtent.w = 48;
    iconExtent.h = 48;
    SetExtent(iconExtent);
    m_text.m_params.SetText(CExoString(""));
    m_border.m_params.SetFillImage(resRef, 0);
    m_borderHilight.m_params.SetFillImage(resRef, 0);
    m_tail.bFocusOnHover = 0;
    SetEnabled(0);
    pPanel->AddControl(this);
    pPanel->BringControlToFront(this);
}

void CSWGuiInGameMenu::Update(float f)
{
    K2_UNUSED(f);
#if KOTOR1
    // K1: only the two plate pulses, no glyph hints.
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->m_nNewItemCount != 0)
    {
        if (!m_lblhInv.m_border.m_params.m_flags.bPulseA)
            m_lblhInv.m_border.m_params.SetPulsingAlpha(1, 0, 1);
    }
    else
    {
        m_lblhInv.m_border.m_params.SetPulsingAlpha(0, 0, 0);
    }

    if (g_pAppManager->m_pClientExoApp->GetQuestJournal()->HasNewEntries())
    {
        if (!m_lblhJou.m_border.m_params.m_flags.bPulseA)
            m_lblhJou.m_border.m_params.SetPulsingAlpha(1, 0, 1);
    }
    else
    {
        m_lblhJou.m_border.m_params.SetPulsingAlpha(0, 0, 0);
    }
#else
    // The Ebon Hawk ("003EBO") is the one module where the party bar's swap hint is suppressed.
    int bNotEbonHawk = 0;
    if (g_pAppManager != 0 && g_pAppManager->m_pServerExoApp != 0)
        bNotEbonHawk = g_pAppManager->m_pServerExoApp->GetModuleName().CompareNoCase("003EBO") != 0;

    int bPartyHasFollowers = 1;
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0 &&
        g_pAppManager->m_pClientExoApp->GetSWParty() != 0)
        bPartyHasFollowers = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount > 1;

    K2_ASSUME(g_pAppManager != 0);   // the app manager outlives every GUI panel
    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    int bSoloQuery = bNotEbonHawk & (pInGameGui->m_lastGuiPanel == CGuiInGame::SCREEN_EQUIP);
    m_iconPartySwap.m_tail.bVisible = bPartyHasFollowers & !bSoloQuery;
    m_iconSoloMode.m_tail.bVisible = static_cast<unsigned char>(bSoloQuery);

    // Pulse the inventory plate while the party store holds unseen items, and the journal plate
    // while the quest log does.
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->m_nNewItemCount != 0)
    {
        if (!m_lblhInv.m_border.m_params.m_flags.bPulseA)
            m_lblhInv.m_border.m_params.SetPulsingAlpha(1, 0, 1);
    }
    else
    {
        m_lblhInv.m_border.m_params.SetPulsingAlpha(0, 0, 0);
    }

    if (g_pAppManager->m_pClientExoApp->GetQuestJournal()->HasNewEntries())
    {
        if (!m_lblhJou.m_border.m_params.m_flags.bPulseA)
            m_lblhJou.m_border.m_params.SetPulsingAlpha(1, 0, 1);
    }
    else
    {
        m_lblhJou.m_border.m_params.SetPulsingAlpha(0, 0, 0);
    }
#endif
}

#include "port/libcstring.h"
void AspyrAddStringIcon(CExoString& s, char nIcon);
bool IsJoystickConnected();
float GetResolutionTextScalar();
extern int gDontMoveCursor_Mac;
extern CTlkTable* g_pTlkTable;

#if defined(K2_QOL_IMPROVEMENTS) && !defined(K2_UI_TOUCH)
// Mobile only: the datapad buttons are touch targets, so away from touch they take the datapad
// style's own button height and the panel gets the room back.
static void K2QolCompactMessageBoxButtons(CSWGuiMessageBox* pBox, float fScale)
{
    CSWGuiManager* pGui = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    float fScreenScale = static_cast<float>(pGui->m_nScreenHeight) / 720.0f;
    int nMax = static_cast<int>(28.0f * (fScale > fScreenScale ? fScale : fScreenScale));
    CSWGuiButton* apButtons[2] = {&pBox->m_btnOk, &pBox->m_btnCancel};
    for (int i = 0; i < 2; i++) {
        CSWGuiExtent extent = apButtons[i]->m_extent;
        if (extent.h <= nMax)
            continue;
        pBox->m_originalPanelExtent.h -= extent.h - nMax;
        extent.h = nMax;
        apButtons[i]->SetExtent(extent);
    }
}
#endif

#if KOTOR1 && defined(K1_UI_XBOX)
// Xbox: the console confirm box. A is OK and B is Cancel, so nothing takes focus and the pad
// answers directly; the PC layout's two buttons stay for touch and mouse, and each gets the pad's
// glyph beside its caption.
static void K1ConsoleAddBoxGlyphs(CSWGuiMessageBox* pBox)
{
    static const char* const s_apszGlyph[2] = { "ios_button_a", "ios_button_b" };
    for (int i = 0; i < 2; i++) {
        CSWGuiLabel* pGlyph = new CSWGuiLabel();
        pGlyph->m_border.m_params.SetFillImage(CResRef(s_apszGlyph[i]), 0);
        pGlyph->m_border.m_params.m_flags.nFillStyle = 2;
        pGlyph->m_tail.bVisible = 0;
        pGlyph->m_tail.bIgnoreMouse = 1;   // drawn, never hit
        pBox->AddControl(pGlyph);
        pBox->m_apConsoleGlyph[i] = pGlyph;
    }
}

// Place each glyph just left of its button, at the button's height, and show it with the button.
void CSWGuiMessageBox::Draw(float fDelta)
{
    CSWGuiButton* apButton[2] = { &m_btnOk, &m_btnCancel };
    for (int i = 0; i < 2; i++) {
        CSWGuiButton& button = *apButton[i];
        CSWGuiLabel* pGlyph = m_apConsoleGlyph[i];
        pGlyph->m_tail.bVisible = button.m_tail.bVisible && !m_bButtonsHidden;
        if (!pGlyph->m_tail.bVisible)
            continue;
        int nSize = button.m_extent.h;
        int nX = button.m_extent.x - nSize - nSize / 8;   // just left of the button's frame
        if (nX < 0)
            nX = button.m_extent.x + nSize / 8;
        CSWGuiExtent extent(nX, button.m_extent.y, nSize, nSize);
        const CSWGuiExtent& e = pGlyph->m_extent;
        if (e.x != extent.x || e.y != extent.y || e.w != extent.w || e.h != extent.h)
            pGlyph->SetExtent(extent);
    }
    CSWGuiPanel::Draw(fDelta);
}
#endif

CSWGuiMessageBox::CSWGuiMessageBox(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: the "confirm" layout, no gamepad caption icons, and the icon keeps its literal 32x32 box.
    m_bAllowCancel = 1;
    m_bAllowDismissal = 1;
    m_bButtonsHidden = 0;
    m_bHasIcon = 0;

    StartLoadFromLayout(CResRef("confirm"));
    InitControl(m_btnOk, CExoString("BTN_OK"), 1);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1);
    InitControl(m_lbMessage, CExoString("LB_MESSAGE"), 1);
    StopLoadFromLayout();

    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                     &CSWGuiMessageBox::OnOkButtonPressed));
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiMessageBox::OnCancelButtonPressed));
    m_lbMessage.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;

    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 10;
    extent.w = 32;
    extent.h = 32;
    m_lblIcon.SetExtent(extent);
    m_lblIcon.m_tail.bVisible = 0;
    AddControl(&m_lblIcon);
#ifdef K1_UI_XBOX
    K1ConsoleAddBoxGlyphs(this);
#endif
    ScaleExtentForResolution();
    if (m_lbMessage.m_pProtoItem)
        m_lblMessage.Initialize(m_lbMessage.m_pProtoItem->m_extent,
                                static_cast<CSWGuiLabel*>(m_lbMessage.m_pProtoItem), 1.0f);
    CenterPanel();

    m_originalPanelExtent = m_extent;
    m_originalListBoxExtent = m_lbMessage.m_extent;
    m_nOpenSoundIndex = GUISOUND_PROMPT;
    m_pfnCallback = 0;
    m_pCallbackObject = 0;
    m_pCallbackControl = 0;
    m_nSavedInputClass = 0;
    m_bAccepted = 0;
    m_bFocusOkButton = 0;
#else
    m_bAllowCancel = 1;
    m_bAllowDismissal = 1;
    m_bButtonsHidden = 0;
    m_bHasIcon = 0;
    m_bDatapadStyle = 0;

    StartLoadFromLayout(CResRef("confirm_p"), 1, 1, 1);
    InitControl(m_btnOk, CExoString("BTN_OK"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
    InitControl(m_lbMessage, CExoString("LB_MESSAGE"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                     &CSWGuiMessageBox::OnOkButtonPressed));
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiMessageBox::OnCancelButtonPressed));
    if (IsJoystickConnected())
    {
        {
            CExoString sText = m_btnOk.m_text.m_params.m_sText;
            AspyrAddStringIcon(sText, 'a');
            m_btnOk.m_text.m_params.SetText(sText);
        }
        {
            CExoString sText = m_btnCancel.m_text.m_params.m_sText;
            AspyrAddStringIcon(sText, 'b');
            m_btnCancel.m_text.m_params.SetText(sText);
        }
    }
    m_lbMessage.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;

    // The icon is not a layout control. It gets a literal 32x32 box under the title, is hidden
    // until SetIcon supplies an image, and is added to the panel by hand.
    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 10;
    extent.w = 32;
    extent.h = 32;
    m_lblIcon.SetExtent(extent);
    m_lblIcon.m_tail.bVisible = 0;
    AddControl(&m_lblIcon);
    if (m_lbMessage.m_pProtoItem)
        m_lblMessage.Initialize(m_lbMessage.m_pProtoItem->m_extent,
                                static_cast<CSWGuiLabel*>(m_lbMessage.m_pProtoItem), 1.0f);
    CenterPanel();

    float fScale = GetResolutionTextScalar();
    extent = m_lblIcon.m_extent;
    extent.w = static_cast<int>(extent.w * fScale);
    extent.h = static_cast<int>(extent.h * fScale);
    m_lblIcon.SetExtent(extent);

    m_originalPanelExtent = m_extent;
    m_originalListBoxExtent = m_lbMessage.m_extent;
#if defined(K2_QOL_IMPROVEMENTS) && !defined(K2_UI_TOUCH)
    K2QolCompactMessageBoxButtons(this, fScale);
#endif
    m_nOpenSoundIndex = GUISOUND_PROMPT;
    m_pfnCallback = 0;
    m_pCallbackObject = 0;
    m_pCallbackControl = 0;
    m_nSavedInputClass = 0;
    m_bAccepted = 0;
    m_bFocusOkButton = 0;
    m_bPausedCombat = 0;
#endif
}

void CSWGuiMessageBox::OnOkButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(SWGUIEVENT_MSGBOX_OK, m_btnOk.m_tail.nCurrentEventArg);
}

void CSWGuiMessageBox::OnCancelButtonPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(SWGUIEVENT_MSGBOX_CANCEL, m_btnCancel.m_tail.nCurrentEventArg);
}

#if !KOTOR1  // K1 has one ctor (no datapad style)
CSWGuiMessageBox::CSWGuiMessageBox(CSWGuiManager* pGuiManager, int bDatapadStyle)
    : CSWGuiPanel(pGuiManager)
{
    m_bAllowCancel = 1;
    m_bAllowDismissal = 1;
    m_bButtonsHidden = 0;
    m_bHasIcon = 0;
    m_bDatapadStyle = bDatapadStyle;

    StartLoadFromLayout(CResRef("datapad_p"), 1, 1, 1);
    // The datapad skin paints its own background; drop the layout's fill so only the frame shows.
    m_pBackground->m_params.SetFillImage(CResRef(""), 0);
    InitControl(m_btnOk, CExoString("BTN_OK"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
    InitControl(m_lbMessage, CExoString("LB_MESSAGE"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    m_lbMessage.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;

    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                     &CSWGuiMessageBox::OnOkButtonPressed));
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiMessageBox::OnCancelButtonPressed));
    if (IsJoystickConnected())
    {
        {
            CExoString sText = m_btnOk.m_text.m_params.m_sText;
            AspyrAddStringIcon(sText, 'a');
            m_btnOk.m_text.m_params.SetText(sText);
        }
        {
            CExoString sText = m_btnCancel.m_text.m_params.m_sText;
            AspyrAddStringIcon(sText, 'b');
            m_btnCancel.m_text.m_params.SetText(sText);
        }
    }

    CSWGuiExtent extent;
    extent.x = 0;
    extent.y = 10;
    extent.w = 32;
    extent.h = 32;
    m_lblIcon.SetExtent(extent);
    m_lblIcon.m_tail.bVisible = 0;
    AddControl(&m_lblIcon);
    if (m_lbMessage.m_pProtoItem)
        m_lblMessage.Initialize(m_lbMessage.m_pProtoItem->m_extent,
                                static_cast<CSWGuiLabel*>(m_lbMessage.m_pProtoItem), 1.0f);
    CenterPanel();

    float fScale = GetResolutionTextScalar();
    extent = m_lblIcon.m_extent;
    extent.w = static_cast<int>(extent.w * fScale);
    extent.h = static_cast<int>(extent.h * fScale);
    m_lblIcon.SetExtent(extent);

    m_originalPanelExtent = m_extent;
    m_originalListBoxExtent = m_lbMessage.m_extent;
#if defined(K2_QOL_IMPROVEMENTS) && !defined(K2_UI_TOUCH)
    K2QolCompactMessageBoxButtons(this, fScale);
#endif
    m_nOpenSoundIndex = GUISOUND_PROMPT;
    m_pfnCallback = 0;
    m_pCallbackObject = 0;
    m_pCallbackControl = 0;
    m_nSavedInputClass = 0;
    m_bAccepted = 0;
    m_bFocusOkButton = 0;
}
#endif // !KOTOR1

void CSWGuiMessageBox::HandleInputEvent(int nType, int nValue)
{
#if KOTOR1
#ifdef K1_UI_XBOX
    // Xbox: A answers OK whatever has focus; the panel must not also hand it to the focused
    // button.
    if (nType == INPUTEVENT_GUI_A && m_bAllowDismissal && !m_bButtonsHidden)
        nType = SWGUIEVENT_MSGBOX_OK;
#endif
    // K1: the escape keys and Cancel count as an accept when cancelling is not allowed, and
    // there is no combat un-pause.
    if (nValue)
    {
        switch (nType)
        {
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_BACK:
            case SWGUIEVENT_MSGBOX_OK:
            case SWGUIEVENT_MSGBOX_CANCEL:
                if (nType == SWGUIEVENT_MSGBOX_OK || !m_bAllowCancel)
                    m_bAccepted = 1;
                else
                    m_bAccepted = 0;
                m_bHasIcon = 0;
                m_bAllowCancel = 1;
                m_bAllowDismissal = 1;
                ResetButtonMessage();
                {
                    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
                    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                        nFlags = (m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
                    m_stateFlags = nFlags;
                }
                m_pGuiManager->PopModalPanel();
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                if (m_pCallbackObject && m_pfnCallback)
                    (m_pCallbackObject->*m_pfnCallback)(m_pCallbackControl);
                break;
            case INPUTEVENT_GUI_SCROLL_UP:
                m_lbMessage.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
            case INPUTEVENT_GUI_SCROLL_DOWN:
                m_lbMessage.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;
        }
    }
    CSWGuiPanel::HandleInputEvent(nType, nValue);
#else
    if (nValue)
    {
        switch (nType)
        {
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_BACK:
            case SWGUIEVENT_MSGBOX_OK:
            case SWGUIEVENT_MSGBOX_CANCEL:
            {
                // 502 is the OK button's own event, 503 the Cancel button's, and 40/46 are the
                // keyboard escapes, which count as an accept when the box refuses to be
                // cancelled at all.
                if (nType == SWGUIEVENT_MSGBOX_OK)
                    m_bAccepted = 1;
                else
                    m_bAccepted = !m_bAllowCancel;
                m_bHasIcon = 0;
                m_bAllowCancel = 1;
                m_bAllowDismissal = 1;
                ResetButtonMessage();
                // Request the panel's removal: one already marked for remove-and-delete only drops
                // its DEFERRED bit, anything else becomes a plain remove.
                unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
                if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                    nFlags = (m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
                m_stateFlags = nFlags;
                m_pGuiManager->PopModalPanel();
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                if (m_pCallbackObject && m_pfnCallback)
                    (m_pCallbackObject->*m_pfnCallback)(m_pCallbackControl);
#if !KOTOR1
                if (m_bDatapadStyle)
                    g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
#endif
                break;
            }
            case INPUTEVENT_GUI_SCROLL_UP:
                m_lbMessage.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
            case INPUTEVENT_GUI_SCROLL_DOWN:
                m_lbMessage.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;
        }
    }
    CSWGuiPanel::HandleInputEvent(nType, nValue);
#endif
}

void CSWGuiMessageBox::SetAllowCancel(int bAllow)
{
    // A cancellable box opens with the "question" sound, a forced one with the "alert" sound.
    m_nOpenSoundIndex = bAllow ? 15 : 2;
    m_bAllowCancel = static_cast<unsigned char>(bAllow);
    UpdateControls();
}

void CSWGuiMessageBox::UpdateControls()
{
    m_btnOk.m_tail.bVisible = m_bAllowDismissal;
    m_btnCancel.m_tail.bVisible = m_bAllowCancel && m_bAllowDismissal;
    m_lblIcon.m_tail.bVisible = m_bHasIcon;
}

void CSWGuiMessageBox::SetIcon(const CResRef& resRef)
{
    if (resRef.IsValid())
    {
        m_bHasIcon = 1;
        m_lblIcon.m_border.m_params.SetFillImage(resRef, 0);
#if !KOTOR1 || defined(K2_FIX_ENGINE_BUGS)  // K1 keeps a tiled fill; an invalid resref leaves the box as it was
        m_lblIcon.m_border.m_params.m_flags.nFillStyle = 2;
#endif
        UpdateControls();
    }
#if !KOTOR1
    else
    {
        m_bHasIcon = 0;
        UpdateControls();
    }
#endif
}

#if !KOTOR1  // K1 has no SetMessageColors (the box keeps its layout's colours)
void CSWGuiMessageBox::SetMessageColors(Vector vBorder, Vector vText)
{
    m_pBackground->m_params.m_color.r = vBorder.x;
    m_pBackground->m_params.m_color.g = vBorder.y;
    m_pBackground->m_params.m_color.b = vBorder.z;
    m_btnOk.m_border.m_params.m_color.r = vBorder.x;
    m_btnOk.m_border.m_params.m_color.g = vBorder.y;
    m_btnOk.m_border.m_params.m_color.b = vBorder.z;
    m_btnOk.m_text.m_params.SetDefaultColor(vText);
    m_btnCancel.m_border.m_params.m_color.r = vBorder.x;
    m_btnCancel.m_border.m_params.m_color.g = vBorder.y;
    m_btnCancel.m_border.m_params.m_color.b = vBorder.z;
    m_btnCancel.m_text.m_params.SetDefaultColor(vText);
    m_lblMessage.m_text.m_params.SetColor(vText);
}
#endif // !KOTOR1

void CSWGuiMessageBox::SetAllowDismissal(int bAllow)
{
    m_bAllowDismissal = static_cast<unsigned char>(bAllow);
    UpdateControls();
}

void CSWGuiMessageBox::SetMessage(CExoString sText)
{
    g_pTlkTable->ParseStr(sText);
    m_lblMessage.m_text.m_params.SetText(sText);
    m_lblMessage.m_text.m_params.ReSetFont();
#ifdef K2_QOL_IMPROVEMENTS
    K2QolFitText(m_lblMessage.m_text, 0x7fffffff);   // LB_MESSAGE scrolls its one row: no limit
#endif
    FixMessageLabel();
}

void CSWGuiMessageBox::FixMessageLabel()
{
#if KOTOR1
    // K1: the scale is the screen height over 768, the button captions may wrap to two lines, and
    // an iPhone screen trims the list by three scaled pixels. No datapad geometry.
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    CSWGuiExtent extentPanel = m_originalPanelExtent;
    CSWGuiExtent extentList = m_originalListBoxExtent;
    CSWGuiExtent extent;

    if (m_bHasIcon)
    {
        extentPanel.h = static_cast<int>(fScale * 32.0f + extentPanel.h);
        extentList.y = static_cast<int>(fScale * 32.0f + extentList.y);
        extentPanel.y = static_cast<int>(extentPanel.y + fScale * -16.0f);
    }

    if (m_bAllowDismissal && !m_bButtonsHidden)
    {
        if (!m_bAllowCancel)
        {
            extentPanel.h = static_cast<int>(extentPanel.h - (fScale + fScale + m_btnCancel.m_extent.h));
            extentPanel.y = static_cast<int>((fScale + fScale + m_btnCancel.m_extent.h) / 2 + extentPanel.y);
        }
    }
    else
    {
        extentPanel.h -= m_btnOk.m_extent.h;
        extentPanel.y += m_btnOk.m_extent.h / 2;
    }

    if (m_btnOk.m_tail.bVisible)
    {
        extent = m_btnOk.m_extent;
        extent.w = static_cast<int>(fScale * 100.0f);
        m_btnOk.SetExtent(extent);
        CSWGuiText* pText = &m_btnOk.m_text;
        while (pText->GetIdealHeight() > pText->GetFontHeight() * 2)
        {
            extent.w = static_cast<int>(fScale * 10.0f + extent.w);
            m_btnOk.SetExtent(extent);
        }
        extent.w = static_cast<int>(fScale * (m_btnOk.m_border.m_params.m_nInnerOffset * 2) + extent.w);
        m_btnOk.SetExtent(extent);
        extentPanel.w = static_cast<int>((extentPanel.w > extent.w - (fScale + fScale))
                                  ? extentPanel.w : extent.w - (fScale + fScale));
    }
    if (m_btnCancel.m_tail.bVisible)
    {
        extent = m_btnCancel.m_extent;
        extent.w = static_cast<int>(fScale * 100.0f);
        m_btnCancel.SetExtent(extent);
        CSWGuiText* pText = &m_btnCancel.m_text;
        while (pText->GetIdealHeight() > pText->GetFontHeight() * 2)
        {
            extent.w = static_cast<int>(fScale * 10.0f + extent.w);
            m_btnCancel.SetExtent(extent);
        }
        extent.w = static_cast<int>(fScale * (m_btnCancel.m_border.m_params.m_nInnerOffset * 2) + extent.w);
        m_btnCancel.SetExtent(extent);
        extentPanel.w = static_cast<int>((extentPanel.w > extent.w - (fScale + fScale))
                                  ? extentPanel.w : extent.w - (fScale + fScale));
    }

    extentList.w = extentPanel.w - m_originalPanelExtent.w + extentList.w;
    int nScreenWidth, nScreenHeight;
    if (IosIsIphone(nScreenWidth, nScreenHeight))
        extentList.w = static_cast<int>(extentList.w + fScale * -3.0f);
    SetExtent(extentPanel);
    m_lbMessage.SetExtent(extentList);
    m_lbMessage.SetScrollBarExtent(static_cast<int>(fScale * 16.0f), 1);
    AddMessageLabel();
    int nScrollBarWidth = m_lbMessage.m_scrollBar.m_extent.w;
    m_lbMessage.SetScrollBarExtent(0, 1);

    if (m_lbMessage.IsOversized())
    {
        int nFontHeight = m_lblMessage.m_text.GetFontHeight();
        while (m_lbMessage.IsOversized())
        {
            if (!(fScale * 440.0f > extentPanel.w) && !(fScale * 280.0f > extentPanel.h))
                break;
            if (extentPanel.h > fScale * 160.0f && fScale * 440.0f > extentPanel.w)
            {
                extentPanel.w = static_cast<int>(fScale * 40.0f + extentPanel.w);
                extentList.w = static_cast<int>(fScale * 40.0f + extentList.w);
                extentPanel.x = static_cast<int>(extentPanel.x - fScale * 40.0f / 2);
                m_lbMessage.SetExtent(extentList);
                AddMessageLabel();
            }
            if (!m_lbMessage.IsOversized())
                break;
            if (fScale * 280.0f > extentPanel.h)
            {
                extentPanel.h += nFontHeight;
                extentList.h += nFontHeight;
                extentPanel.y -= nFontHeight / 2;
                m_lbMessage.SetExtent(extentList);
                AddMessageLabel();
            }
        }
        SetExtent(extentPanel);
    }
    if (m_lbMessage.IsOversized())
    {
        m_lbMessage.SetScrollBarExtent(nScrollBarWidth, 1);
        AddMessageLabel();
    }

    int nCenterX = m_extent.w / 2;
    extent = m_lblIcon.m_extent;
    extent.x = nCenterX - extent.w / 2;
    m_lblIcon.SetExtent(extent);

    extent = m_btnOk.m_extent;
    extent.y = static_cast<int>(fScale * 4.0f + (m_lbMessage.m_extent.y + m_lbMessage.m_extent.h));
    extent.x = nCenterX - extent.w / 2;
    m_btnOk.SetExtent(extent);

    extent = m_btnCancel.m_extent;
    extent.y = static_cast<int>(fScale * 2.0f + (m_btnOk.m_extent.y + m_btnOk.m_extent.h));
    extent.x = nCenterX - extent.w / 2;
    m_btnCancel.SetExtent(extent);
    CenterPanel();
#else
    float fScale = GetResolutionTextScalar();
#ifdef K2_QOL_IMPROVEMENTS
    // The box's own pixel constants are multiplied by the integer text scalar, while its layout
    // controls scale with the screen. At the 1280x720 the mobile layout was tuned at the two agree;
    // at 1920x1080 the layout-scaled panel is already past the size the growth loop stops at, so
    // LB_MESSAGE never grew and the OK button took half the box. Both are made screen-relative
    // here: never smaller than the layout's, scaled from 1280x720.
    CSWGuiManager* pK2Gui = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    const float fK2ScaleW = static_cast<float>(pK2Gui->m_nScreenWidth) / 1280.0f;
    const float fK2ScaleH = static_cast<float>(pK2Gui->m_nScreenHeight) / 720.0f;
    const float fK2MaxW = fScale * 440.0f > 440.0f * fK2ScaleW ? fScale * 440.0f : 440.0f * fK2ScaleW;
    const float fK2MaxH = fScale * 280.0f > 280.0f * fK2ScaleH ? fScale * 280.0f : 280.0f * fK2ScaleH;
    const float fK2PadScale = fScale > fK2ScaleH ? fScale : fK2ScaleH;   // the datapad's frame
#define K2_MSGBOX_MAX_W fK2MaxW
#define K2_MSGBOX_MAX_H fK2MaxH
#define K2_DATAPAD_SCALE fK2PadScale
    // The button-widening loops below stop once the caption fits on one line. The string's size
    // factor is ignored by the font-height query, so a caption scaled above 100 % never satisfies
    // that test and the loop runs forever (seen on the Equip screen's "no items" box). Test the
    // line count instead, with the screen width as a hard stop.
#define K2_MSGBOX_CAPTION_WRAPS(pText) \
    ((pText)->m_pString && (pText)->m_pString->GetNumLines() > 1 && extent.w < pK2Gui->m_nScreenWidth)
#else
#define K2_MSGBOX_CAPTION_WRAPS(pText) ((pText)->GetIdealHeight() != (pText)->GetFontHeight())
#define K2_MSGBOX_MAX_W (fScale * 440.0f)
#define K2_MSGBOX_MAX_H (fScale * 280.0f)
#define K2_DATAPAD_SCALE fScale
#endif
    CSWGuiExtent extentPanel = m_originalPanelExtent;
    CSWGuiExtent extentList = m_originalListBoxExtent;
    CSWGuiExtent extent;

    // An icon pushes the whole box down and makes room for itself above the text.
    if (m_bHasIcon)
    {
        extentPanel.h = static_cast<int>(fScale * 32.0f + extentPanel.h);
        extentList.y = static_cast<int>(fScale * 32.0f + extentList.y);
        extentPanel.y = static_cast<int>(extentPanel.y + fScale * -16.0f);
    }

    if (m_bAllowDismissal && !m_bButtonsHidden)
    {
        // Only one button will be shown, so the box loses a whole button row.
        if (!m_bAllowCancel)
        {
            extentPanel.h = static_cast<int>(extentPanel.h - (fScale + fScale + m_btnCancel.m_extent.h));
            extentPanel.y = static_cast<int>(fScale * ((m_btnCancel.m_extent.h + 2) / 2) + extentPanel.y);
        }
    }
    else
    {
        extentPanel.h = static_cast<int>(extentPanel.h - fScale * m_btnOk.m_extent.h);
        extentPanel.y = static_cast<int>(fScale * (m_btnOk.m_extent.h / 2) + extentPanel.y);
    }

    // Grow each visible button until its caption fits on one line, then pad it by its own frame.
    if (m_btnOk.m_tail.bVisible)
    {
        extent = m_btnOk.m_extent;
        extent.w = static_cast<int>(fScale * 100.0f);
        m_btnOk.SetExtent(extent);
        CSWGuiText* pText = &m_btnOk.m_text;
        while (K2_MSGBOX_CAPTION_WRAPS(pText))
        {
            extent.w += 10;
            m_btnOk.SetExtent(extent);
        }
        extent.w = static_cast<int>(fScale * (m_btnOk.m_border.m_params.m_nInnerOffset * 2) + extent.w);
        m_btnOk.SetExtent(extent);
        extentPanel.w = (extentPanel.w > extent.w - 2) ? extentPanel.w : extent.w - 2;
    }
    if (m_btnCancel.m_tail.bVisible)
    {
        extent = m_btnCancel.m_extent;
        extent.w = static_cast<int>(fScale * 100.0f);
        m_btnCancel.SetExtent(extent);
        CSWGuiText* pText = &m_btnCancel.m_text;
        while (K2_MSGBOX_CAPTION_WRAPS(pText))
        {
            extent.w += 10;
            m_btnCancel.SetExtent(extent);
        }
        extent.w = static_cast<int>(fScale * (m_btnCancel.m_border.m_params.m_nInnerOffset * 2) + extent.w);
        m_btnCancel.SetExtent(extent);
        extentPanel.w = (extentPanel.w > extent.w - 2) ? extentPanel.w : extent.w - 2;
        // Match the two buttons' widths to the wider of the pair.
        if (m_btnOk.m_tail.bVisible)
        {
            CSWGuiExtent extentOk = m_btnOk.m_extent;
            if (extentOk.w < extent.w)
            {
                extentOk.w = extent.w;
                m_btnOk.SetExtent(extentOk);
            }
            else if (extentOk.w > extent.w)
            {
                extent.w = extentOk.w;
                m_btnCancel.SetExtent(extentOk);  // sic: the OK button's rectangle, not `extent`
            }
        }
    }

    if (m_bDatapadStyle)
    {
        extentPanel.w = static_cast<int>(400.0f * K2_DATAPAD_SCALE);
        extentPanel.h = static_cast<int>(300.0f * K2_DATAPAD_SCALE);
        extentPanel.x = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth / 2
                        - extentPanel.w / 2;
    }
    extentList.w = extentPanel.w - m_originalPanelExtent.w + extentList.w;
    SetExtent(extentPanel);

    if (m_bDatapadStyle)
    {
        extentList.x = static_cast<int>(33.0f * K2_DATAPAD_SCALE);
        extentList.y = static_cast<int>(27.0f * K2_DATAPAD_SCALE);
        extentList.w = static_cast<int>(334.0f * K2_DATAPAD_SCALE);
        extentList.h = static_cast<int>(208.0f * K2_DATAPAD_SCALE);
    }
    m_lbMessage.SetExtent(extentList);
    m_lbMessage.SetScrollBarExtent(static_cast<int>(fScale * 16.0f), 1);
    AddMessageLabel();
    int nScrollBarWidth = m_lbMessage.m_scrollBar.m_extent.w;
    m_lbMessage.SetScrollBarExtent(0, 1);

    // While the single row still overflows the list box, spend the space budget on width first
    // (up to 440 scaled pixels) and then on height (up to 280).
    if (m_lbMessage.IsOversized())
    {
        int nFontHeight = m_lblMessage.m_text.GetFontHeight();
        while (m_lbMessage.IsOversized())
        {
            // Out of budget in both directions -- the label will just have to scroll.
            if (!(K2_MSGBOX_MAX_W > static_cast<float>(extentPanel.w))
                && !(K2_MSGBOX_MAX_H > static_cast<float>(extentPanel.h)))
                break;
            if (extentPanel.h >= 161 && K2_MSGBOX_MAX_W > static_cast<float>(extentPanel.w))
            {
                extentPanel.w += 40;
                extentList.w += 40;
                extentPanel.x -= 20;
                m_lbMessage.SetExtent(extentList);
                AddMessageLabel();
            }
            if (!m_lbMessage.IsOversized())
                break;
            if (K2_MSGBOX_MAX_H > static_cast<float>(extentPanel.h))
            {
                extentPanel.h += nFontHeight;
                extentList.h += nFontHeight;
                extentPanel.y -= nFontHeight / 2;
                m_lbMessage.SetExtent(extentList);
                AddMessageLabel();
            }
        }
        SetExtent(extentPanel);
    }
    if (m_lbMessage.IsOversized())
    {
        // Still overflowing after every growth step -- give the scroll bar back.
        m_lbMessage.SetScrollBarExtent(nScrollBarWidth, 1);
        AddMessageLabel();
    }

    extent = m_lblIcon.m_extent;
    extent.x = m_extent.w / 2 - extent.w / 2;
    m_lblIcon.SetExtent(extent);

    if (m_bDatapadStyle)
    {
        extent.w = static_cast<int>(120.0f * K2_DATAPAD_SCALE);
        extent.h = static_cast<int>(28.0f * K2_DATAPAD_SCALE);
        extent.y = static_cast<int>(K2_DATAPAD_SCALE * 245.0f);
        extent.x = extentPanel.w / 2 - extent.w / 2;
    }
    else
    {
        extent = m_btnOk.m_extent;
        extent.y = m_lbMessage.m_extent.y + m_lbMessage.m_extent.h + 4;
        extent.x = m_extent.w / 2 - extent.w / 2;
    }
    m_btnOk.SetExtent(extent);

    extent = m_btnCancel.m_extent;
    extent.y = m_btnOk.m_extent.y + m_btnOk.m_extent.h + 2;
    extent.x = m_extent.w / 2 - extent.w / 2;
    m_btnCancel.SetExtent(extent);
    CenterPanel();
#undef K2_MSGBOX_MAX_W
#undef K2_MSGBOX_MAX_H
#undef K2_DATAPAD_SCALE
#undef K2_MSGBOX_CAPTION_WRAPS
#endif
}

void CSWGuiMessageBox::SetMessage(uint64_t nStrRef)
{
    SetMessage(g_pTlkTable->GetSimpleString(nStrRef));
}

void CSWGuiMessageBox::SetOkButtonMessage(uint64_t nStrRef)
{
    m_btnOk.m_text.m_params.SetStrRef(nStrRef);
#if !KOTOR1  // K1: no gamepad caption icon
    if (IsJoystickConnected())
    {
        CExoString sText = m_btnOk.m_text.m_params.m_sText;
        AspyrAddStringIcon(sText, 'a');
        m_btnOk.m_text.m_params.SetText(sText);
    }
#endif
}

void CSWGuiMessageBox::SetCancelButtonMessage(uint64_t nStrRef)
{
    m_btnCancel.m_text.m_params.SetStrRef(nStrRef);
#if !KOTOR1  // K1: no gamepad caption icon
    if (IsJoystickConnected())
    {
        CExoString sText = m_btnCancel.m_text.m_params.m_sText;
        AspyrAddStringIcon(sText, 'b');
        m_btnCancel.m_text.m_params.SetText(sText);
    }
#endif
}

void CSWGuiMessageBox::HideButtons()
{
    m_bButtonsHidden = 1;
    m_btnOk.m_tail.bVisible = 0;
    m_btnCancel.m_tail.bVisible = 0;
}

void CSWGuiMessageBox::ResetButtonMessage()
{
    m_btnOk.m_tail.bVisible = 1;
    m_btnCancel.m_tail.bVisible = 1;
    m_bButtonsHidden = 0;
    m_btnOk.m_text.m_params.SetStrRef(STRREF_OK);
    m_btnCancel.m_text.m_params.SetStrRef(STRREF_CANCEL);
#if !KOTOR1  // K1: no gamepad caption icons
    if (IsJoystickConnected())
    {
        {
            CExoString sText = m_btnOk.m_text.m_params.m_sText;
            AspyrAddStringIcon(sText, 'a');
            m_btnOk.m_text.m_params.SetText(sText);
        }
        {
            CExoString sText = m_btnCancel.m_text.m_params.m_sText;
            AspyrAddStringIcon(sText, 'b');
            m_btnCancel.m_text.m_params.SetText(sText);
        }
    }
#endif
}

void CSWGuiMessageBox::AddMessageLabel()
{
    CSWGuiExtent extent = m_lblMessage.m_extent;
    extent.w = m_lbMessage.m_innerExtent.w - 2 * m_lbMessage.m_nPadding;
    m_lblMessage.SetExtent(extent);
    CSWGuiText* pText = &m_lblMessage.m_text;
    if (m_lblMessage.m_text.m_params.m_sText.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblMessage.SetExtent(extent);

    CSWGuiControl* paRows[1];
    paRows[0] = &m_lblMessage;
    m_lbMessage.AddControls(paRows, 1, 0, 0, 0);
    m_lbMessage.SetSelectedControl(&m_lblMessage, 0);
}

void CSWGuiMessageBox::SetCallback(CSWGuiObject* pObject,
                                   void (CSWGuiObject::*pfn)(CSWGuiControl*),
                                   CSWGuiControl* pControl)
{
    m_pfnCallback = pfn;
    m_pCallbackObject = pObject;
    m_pCallbackControl = pControl;
}

void CSWGuiMessageBox::OnPanelAdded()
{
#if KOTOR1
    // K1: the active control is Cancel whenever cancelling is allowed, and the mouse is not moved.
    int nInputClass = g_pAppManager->m_pClientExoApp->GetInputClass();
    CSWGuiPanel::OnPanelAdded();
    m_nSavedInputClass = -1;
    if (nInputClass != 2)
    {
        m_nSavedInputClass = nInputClass;
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 0);
    }
    SetActiveControl(0, 0);
#ifdef K2_QOL_CONFIRM_PAD
    // A answers OK and B cancels (HandleInputEvent), so a box showing its buttons opens with
    // neither selected: the d-pad has nothing to move between.
    if (m_bAllowDismissal && !m_bButtonsHidden)
        return;
#endif
    if (m_bAllowCancel)
        SetActiveControl(&m_btnCancel, 0);
    else
        SetActiveControl(&m_btnOk, 0);
#else
    if (m_bDatapadStyle)
        m_pBackground->m_params.SetFillImage(CResRef("pnl_datapad"), 0);

    int nInputClass = g_pAppManager->m_pClientExoApp->GetInputClass();
    CSWGuiPanel::OnPanelAdded();
    m_nSavedInputClass = -1;
    if (nInputClass != 2)
    {
        m_nSavedInputClass = nInputClass;
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 0);
    }
    SetActiveControl(0, 0);
    SetActiveControl(&m_btnOk, 0);

    // Park the mouse over whichever button is the safe default, unless the port option says the
    // cursor is the player's to move or a pad is driving the UI anyway.
    if (!gDontMoveCursor_Mac && !IsJoystickConnected())
    {
        if (m_bAllowCancel && !m_bFocusOkButton)
        {
            g_pGuiMan->StoreCurrentMousePosition();
            g_pGuiMan->MoveMouseToPosition(
                m_extent.x + m_btnCancel.m_extent.x + m_btnCancel.m_extent.w / 2,
                m_extent.y + m_btnCancel.m_extent.y + m_btnCancel.m_extent.h / 2);
        }
        else
        {
            g_pGuiMan->StoreCurrentMousePosition();
            g_pGuiMan->MoveMouseToPosition(
                m_extent.x + m_btnOk.m_extent.x + m_btnOk.m_extent.w / 2,
                m_extent.y + m_btnOk.m_extent.y + m_btnOk.m_extent.h / 2);
        }
    }
#endif
}

void CSWGuiMessageBox::OnPanelRemoved()
{
    CSWGuiPanel::OnPanelRemoved();
#if KOTOR1
    // K1: the callback survives the removal and nothing is re-skinned.
    if (m_nSavedInputClass >= 0)
        g_pAppManager->m_pClientExoApp->SetInputClass(m_nSavedInputClass, 1);
    g_pGuiMan->RestoreOldMousePosition();
    m_bFocusOkButton = 0;
#else
    m_pfnCallback = 0;
    m_pCallbackObject = 0;
    m_pCallbackControl = 0;
    if (m_bDatapadStyle)
        m_pBackground->m_params.SetFillImage(CResRef("pnl_datapad"), 0);
    m_lblMessage.m_text.m_params.SetAlignment(10);
    m_lblMessage.m_text.m_params.SetFont(CResRef("dialogfont16x16"));
    if (m_nSavedInputClass >= 0)
        g_pAppManager->m_pClientExoApp->SetInputClass(m_nSavedInputClass, 1);
    g_pGuiMan->RestoreOldMousePosition();
    m_bFocusOkButton = 0;
    if (!m_bDatapadStyle)
        SetMessageColors(CGuiInGame::COLOR_NEW_GREEN, CGuiInGame::COLOR_NEW_HIGHLIGHT);
    if (m_bPausedCombat)
    {
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
        m_bPausedCombat = 0;
    }
#endif
}

void CSWGuiControllerLossBox::OnAccept(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->EndControllerPauseState();
}

void CSWGuiControllerLossBox::OnPanelAdded()
{
    SetAllowCancel(0);
#if !KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
    SetMessage(STRREF_CONTROLLER_DISCONNECTED_K2);
#else
    SetMessage(STRREF_CONTROLLER_DISCONNECTED);
#endif
    SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiControllerLossBox::OnAccept), 0);
    CSWGuiMessageBox::OnPanelAdded();
}

void CSWGuiMessageBox::OnLoadError(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
}

void CSWGuiMessageBox::DisplayLoadError(uint64_t nStrRef)
{
    K2_UNUSED(nStrRef);
}

void CSWGuiMessageBox::DisplayDeathMessage()
{
#if KOTOR1
    // K1: no death display class, so the death message is this non-cancellable box, dismissed
    // into OnDeath.
    SetAllowCancel(0);
    SetMessage(STRREF_PARTY_KILLED);
    SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiMessageBox::OnDeath), 0);
    m_pGuiManager->AddPanel(this, 1, 1);
#endif
}

void CSWGuiMessageBox::OnDeath(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->SetEndGamePending(1, 0.0f, 1);
}

void AspyrAddStringIcon(CExoString& s, char nIcon);
extern CTlkTable* g_pTlkTable;
extern int g_nHSMode;

// The easter-egg key sequence the log screen watches for, and how much of it has been typed. A
// file-scope counter, not a member, so the progress survives the panel being rebuilt.
#if KOTOR1  // K1: the sequence starts and ends on 41
static const int s_anEasterEggKeys[4] = { 41, 47, 39, 41 };
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#else
static const int s_anEasterEggKeys[4] = { 42, 47, 39, 42 };
#endif
static int s_nEasterEggProgress;

CSWGuiInGameMessages::CSWGuiInGameMessages(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    CSWGuiExtent extent;

#if KOTOR1
    // K1: "messages" with two pages and a BTN_SHOW toggle; the rows' highlight is yellow and the
    // feedback page clips partial rows.
    StartLoadFromLayout(CResRef("messages"));
    InitControl(m_lbFeedback, CExoString("LB_MESSAGES"), 1);
    InitControl(m_lblMessages, CExoString("LBL_MESSAGES"), 1);
    InitControl(m_lbDialog, CExoString("LB_DIALOG"), 1);
    InitControl(m_btnShow, CExoString("BTN_SHOW"), 1);
    InitControl(m_btnExit, CExoString("BTN_EXIT"), 1);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    CSWGuiLabelHilight* pProto = static_cast<CSWGuiLabelHilight*>(m_lbFeedback.m_pProtoItem);
    pProto->m_hilightBorder.m_params.m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_YELLOW);
    pProto->m_hilightBorder.m_params.SetPulsingAlpha(1, 1, 0);
    m_paFeedbackRows = new CSWGuiLabelHilight[64];
    for (int i = 0; i < 64; i++)
        m_paFeedbackRows[i].Initialize(extent, pProto);

    pProto = static_cast<CSWGuiLabelHilight*>(m_lbDialog.m_pProtoItem);
    pProto->m_hilightBorder.m_params.m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_YELLOW);
    pProto->m_hilightBorder.m_params.SetPulsingAlpha(1, 1, 0);
    m_paDialogRows = new CSWGuiLabelHilight[64];
    for (int i = 0; i < 64; i++)
        m_paDialogRows[i].Initialize(extent, pProto);

    m_lbFeedback.bClipPartialRows = 1;
    m_btnShow.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnXButtonPressed));
    m_btnShow.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnBButtonPressed));
    m_btnExit.m_tail.bFocusOnHover = 0;
    m_lbFeedback.bSelectOnLeftClick = 1;
    m_lbDialog.bSelectOnLeftClick = 1;
    SetActiveControl(&m_lbFeedback, 0);
    IosAddButtonIcon(&m_btnExit, 'b');
#else
    StartLoadFromLayout(CResRef("messages_p"), 1, 1, 1);
    InitControl(m_lbFeedback, CExoString("LB_MESSAGES"), 1, 1, 0, 0, 0);
    InitControl(m_lblMessages, CExoString("LBL_MESSAGES"), 1, 1, 0, 0, 0);
    InitControl(m_lbDialog, CExoString("LB_DIALOG"), 1, 1, 0, 0, 0);
    InitControl(m_lbCombat, CExoString("LB_COMBAT"), 1, 1, 0, 0, 0);
    InitControl(m_lbEffectsGood, CExoString("LB_EFFECTS_GOOD"), 1, 1, 0, 0, 0);
    InitControl(m_lbEffectsBad, CExoString("LB_EFFECTS_BAD"), 1, 1, 0, 0, 0);
    InitControl(m_lblEffectsGood, CExoString("LBL_EFFECTS_GOOD"), 1, 1, 0, 0, 0);
    InitControl(m_lblEffectsBad, CExoString("LBL_EFFECTS_BAD"), 1, 1, 0, 0, 0);
    InitControl(m_btnExit, CExoString("BTN_EXIT"), 1, 1, 0, 0, 0);
    AspyrAddButtonIcon(&m_btnExit, 'b');
    InitControl(m_aBars[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[4], CExoString("LBL_BAR5"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[5], CExoString("LBL_BAR6"), 1, 1, 0, 0, 0);
    InitControl(m_btnDialog, CExoString("BTN_DIALOG"), 1, 1, 0, 0, 0);
    InitControl(m_btnFeedback, CExoString("BTN_FEEDBACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnCombat, CExoString("BTN_COMBAT"), 1, 1, 0, 0, 0);
    InitControl(m_btnEffects, CExoString("BTN_EFFECTS"), 1, 1, 0, 0, 0);
    InitControl(m_lblFilter, CExoString("LBL_FILTER"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    // Each page's 64 row widgets are cloned from that page's list box prototype row, after the
    // prototype is re-tinted to the "new text" highlight and set pulsing.
    CSWGuiLabelHilight* pProto = static_cast<CSWGuiLabelHilight*>(m_lbFeedback.m_pProtoItem);
    pProto->m_hilightBorder.m_params.m_color =
        reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    pProto->m_hilightBorder.m_params.SetPulsingAlpha(1, 1, 0);
    m_paFeedbackRows = new CSWGuiLabelHilight[64];
    for (int i = 0; i < 64; i++)
        m_paFeedbackRows[i].Initialize(extent, pProto);

    pProto = static_cast<CSWGuiLabelHilight*>(m_lbDialog.m_pProtoItem);
    pProto->m_hilightBorder.m_params.m_color =
        reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    pProto->m_hilightBorder.m_params.SetPulsingAlpha(1, 1, 0);
    m_paDialogRows = new CSWGuiLabelHilight[64];
    for (int i = 0; i < 64; i++)
        m_paDialogRows[i].Initialize(extent, pProto);

    // The combat rows reuse the dialog page's prototype; m_lbCombat.m_pProtoItem is not read
    // here. // sic
    pProto->m_hilightBorder.m_params.m_color =
        reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    pProto->m_hilightBorder.m_params.SetPulsingAlpha(1, 1, 0);
    m_paCombatRows = new CSWGuiLabelHilight[64];
    for (int i = 0; i < 64; i++)
        m_paCombatRows[i].Initialize(extent, pProto);

    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnBButtonPressed));
    m_btnExit.m_tail.bFocusOnHover = 0;
    // One handler serves all four filter buttons; each carries its page index in the control's
    // user-data word, which is what ChangeMessageDisplay switches on.
    m_btnDialog.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameMessages::ChangeMessageDisplay));
    m_btnDialog.m_tail.m_nUserData = MESSAGE_PAGE_DIALOG;
    m_btnDialog.m_tail.bFocusOnHover = 0;
    m_btnFeedback.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                           &CSWGuiInGameMessages::ChangeMessageDisplay));
    m_btnFeedback.m_tail.m_nUserData = MESSAGE_PAGE_FEEDBACK;
    m_btnFeedback.m_tail.bFocusOnHover = 0;
    m_btnCombat.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameMessages::ChangeMessageDisplay));
    m_btnCombat.m_tail.m_nUserData = MESSAGE_PAGE_COMBAT;
    m_btnCombat.m_tail.bFocusOnHover = 0;
    m_btnEffects.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameMessages::ChangeMessageDisplay));
    m_btnEffects.m_tail.m_nUserData = MESSAGE_PAGE_EFFECTS;
    m_btnEffects.m_tail.bFocusOnHover = 0;

    m_nPendingFilter = MESSAGE_PAGE_DIALOG;
    m_lbFeedback.bSelectOnLeftClick = 1;
    m_lbDialog.bSelectOnLeftClick = 1;
    m_lbCombat.bSelectOnLeftClick = 1;
    SetActiveControl(&m_lbFeedback, 0);
    m_nPendingFilter = MESSAGE_PAGE_DIALOG;
#endif
}

// Prefix the button's caption with the on-screen glyph for gamepad button nIcon.
#if !KOTOR1  // K1 has only IosAddButtonIcon (IosWidgets.cpp)
void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon)
{
    CExoString sText(pButton->m_text.m_params.m_sText);
    AspyrAddStringIcon(sText, nIcon);
    pButton->m_text.m_params.SetText(sText);
}
#endif

// The four filter buttons all route their "pressed" event here; each carries its MessagePage in
// m_tail.m_nUserData, set up by the constructor.
#if !KOTOR1  // K1: no filter buttons
void CSWGuiInGameMessages::ChangeMessageDisplay(CSWGuiControl* pControl)
{
    switch (static_cast<int>(pControl->m_tail.m_nUserData))
    {
        case MESSAGE_PAGE_DIALOG:   ShowDialogMessages();   break;
        case MESSAGE_PAGE_FEEDBACK: ShowFeedbackMessages(); break;
        case MESSAGE_PAGE_COMBAT:   ShowCombatMessages();   break;
        case MESSAGE_PAGE_EFFECTS:  ShowEffects();          break;
    }
}
#endif

CSWGuiInGameMessages::~CSWGuiInGameMessages()
{
    m_lbDialog.ClearItems();
    m_lbFeedback.ClearItems();
#if !KOTOR1
    m_lbCombat.ClearItems();
    m_lbEffectsGood.ClearItems();
    m_lbEffectsBad.ClearItems();
#endif
    delete[] m_paDialogRows;
    delete[] m_paFeedbackRows;
#if !KOTOR1
    delete[] m_paCombatRows;
#endif
}

void CSWGuiInGameMessages::AddMessages(CGuiInGameMessage* pMessages, unsigned int nCount)
{
    CExoArrayList<CSWGuiControl*> aRows;
#if KOTOR1  // K1: rows start 10 high, no token pass, blue text
    CSWGuiExtent extent(0, 0, m_lbFeedback.m_innerExtent.w - m_lbFeedback.m_nPadding * 2, 10);
#else
    CSWGuiExtent extent(0, 0, m_lbFeedback.m_innerExtent.w - m_lbFeedback.m_nPadding * 2, 100);
#endif
    aRows.SetSize(nCount);

    for (unsigned int i = 0; i < nCount; i++)
    {
#if KOTOR1
        m_paFeedbackRows[i].m_text.m_params.SetText(pMessages[i].m_text);
#else
        CExoString sText(pMessages[i].m_text);
        // Only pay for the talk-table pass when the line actually carries a token.
        int nTag = sText.Find('<', 0);
        int nToken = sText.Find('{', 0);
        if (nTag != -1 || nToken != -1)
        {
            CSWTlkTable* pTlk;
            (pTlk = g_pSWTlkTable)->m_pSpeakerObj =
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            pTlk->m_gender = pTlk->GetObjectGender();
            pTlk->m_bSuppressEnabled |= 1;
            pTlk->ParseStr(sText);
            // Trim trailing blanks the token expansion left behind.
            int nLen = sText.m_sString ? static_cast<int>(strlen(sText.m_sString)) : 0;
            while (sText[nLen - 1] == ' ')
            {
                sText = sText.SubString(0, (sText.m_sString ? static_cast<int>(strlen(sText.m_sString)) : 0) - 1);
                nLen = sText.m_sString ? static_cast<int>(strlen(sText.m_sString)) : 0;
                if (nLen < 1)
                    break;
            }
        }

        m_paFeedbackRows[i].m_text.m_params.SetText(sText);
#endif
        m_paFeedbackRows[i].m_text.m_params.ReSetFont();

        int nParam = pMessages[i].m_param4;
        Vector vColor;
        if (nParam == 1)
            vColor = CGuiInGame::COLOR_RED;
        else
#if KOTOR1
            vColor = CGuiInGame::COLOR_BLUE;
#else
            vColor = CGuiInGame::COLOR_NEW_TEXT;
#endif
        m_paFeedbackRows[i].m_text.m_params.SetColor(vColor);

        m_paFeedbackRows[i].SetExtent(extent);
        extent.h = static_cast<CSWGuiDrawable*>(&m_paFeedbackRows[i].m_text)->GetIdealHeight()
                 + m_paFeedbackRows[i].m_hilightBorder.m_params.GetBorderDim() * 2;
        m_paFeedbackRows[i].SetExtent(extent);
        m_paFeedbackRows[i].SetActive(0);
        aRows[i] = &m_paFeedbackRows[i];
    }

    if (nCount != 0)
    {
        m_lbFeedback.AddControls(aRows, 0, 0, 1);
        m_lbFeedback.SetSelectedControl(nCount - 1, 0);
    }
}

// The dialog page's rows carry a "<speaker>: <line>" caption, so the speaker name gets the
// talk-table pass and the concatenation happens only when it survives it.
void CSWGuiInGameMessages::AddDialogMessages(CGuiInGameDialogMessage* pMessages,
                                             unsigned int nCount)
{
    CExoArrayList<CSWGuiControl*> aRows;
#if KOTOR1  // K1: rows start 10 high, no token pass on the speaker
    CSWGuiExtent extent(0, 0, m_lbDialog.m_innerExtent.w - m_lbDialog.m_nPadding * 2, 10);
#else
    CSWGuiExtent extent(0, 0, m_lbDialog.m_innerExtent.w - m_lbDialog.m_nPadding * 2, 100);
#endif
    CExoString sSpeaker;
    aRows.SetSize(nCount);

    for (unsigned int i = 0; i < nCount; i++)
    {
        sSpeaker = pMessages[i].m_speaker;
#if KOTOR1
        if (sSpeaker.GetLength() >= 1)
#else
        int nTag = sSpeaker.Find('<', 0);
        int nToken = sSpeaker.Find('{', 0);
        if (nTag != -1 || nToken != -1)
        {
            CSWTlkTable* pTlk;
            (pTlk = g_pSWTlkTable)->m_pSpeakerObj =
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            pTlk->m_gender = pTlk->GetObjectGender();
            pTlk->m_bSuppressEnabled |= 1;
            pTlk->ParseStr(sSpeaker);
            int nLen = sSpeaker.m_sString ? static_cast<int>(strlen(sSpeaker.m_sString)) : 0;
            while (sSpeaker[nLen - 1] == ' ')
            {
                sSpeaker = sSpeaker.SubString(0, (sSpeaker.m_sString ? static_cast<int>(strlen(sSpeaker.m_sString)) : 0) - 1);
                nLen = sSpeaker.m_sString ? static_cast<int>(strlen(sSpeaker.m_sString)) : 0;
                if (nLen < 1)
                    break;
            }
        }

        if ((sSpeaker.m_sString ? static_cast<int>(strlen(sSpeaker.m_sString)) : 0) >= 1)
#endif
            m_paDialogRows[i].m_text.m_params.SetText(sSpeaker + ": " + pMessages[i].m_text);
        else
            m_paDialogRows[i].m_text.m_params.SetText(pMessages[i].m_text);
        m_paDialogRows[i].m_text.m_params.ReSetFont();

        m_paDialogRows[i].SetExtent(extent);
        extent.h = static_cast<CSWGuiDrawable*>(&m_paDialogRows[i].m_text)->GetIdealHeight()
                 + m_paDialogRows[i].m_hilightBorder.m_params.GetBorderDim() * 2;
        m_paDialogRows[i].SetExtent(extent);
        m_paDialogRows[i].SetActive(0);
        aRows[i] = &m_paDialogRows[i];
    }

    if (nCount != 0)
    {
        m_lbDialog.AddControls(aRows, 0, 0, 1);
        m_lbDialog.SetSelectedControl(nCount - 1, 0);
    }
}

// Same shape as AddMessages, on the combat page's own row array, with an extra leading SetExtent
// before the caption is installed.
#if !KOTOR1  // K1: no combat log
void CSWGuiInGameMessages::AddCombatMessages(CGuiInGameMessage* pMessages, unsigned int nCount)
{
    CExoArrayList<CSWGuiControl*> aRows;
    CSWGuiExtent extent(0, 0, m_lbCombat.m_innerExtent.w - m_lbCombat.m_nPadding * 2, 100);
    aRows.SetSize(nCount);

    for (unsigned int i = 0; i < nCount; i++)
    {
        CExoString sText(pMessages[i].m_text);
        int nTag = sText.Find('<', 0);
        int nToken = sText.Find('{', 0);
        if (nTag != -1 || nToken != -1)
        {
            CSWTlkTable* pTlk;
            (pTlk = g_pSWTlkTable)->m_pSpeakerObj =
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            pTlk->m_gender = pTlk->GetObjectGender();
            pTlk->m_bSuppressEnabled |= 1;
            pTlk->ParseStr(sText);
            int nLen = sText.m_sString ? static_cast<int>(strlen(sText.m_sString)) : 0;
            while (sText[nLen - 1] == ' ')
            {
                sText = sText.SubString(0, (sText.m_sString ? static_cast<int>(strlen(sText.m_sString)) : 0) - 1);
                nLen = sText.m_sString ? static_cast<int>(strlen(sText.m_sString)) : 0;
                if (nLen < 1)
                    break;
            }
        }

        m_paCombatRows[i].SetExtent(extent);
        m_paCombatRows[i].m_text.m_params.SetText(sText);
        m_paCombatRows[i].m_text.m_params.ReSetFont();

        int nParam = pMessages[i].m_param4;
        Vector vColor;
        if (nParam == 1)
            vColor = CGuiInGame::COLOR_RED;
        else
            vColor = CGuiInGame::COLOR_NEW_TEXT;
        m_paCombatRows[i].m_text.m_params.SetColor(vColor);

        m_paCombatRows[i].SetExtent(extent);
        extent.h = static_cast<CSWGuiDrawable*>(&m_paCombatRows[i].m_text)->GetIdealHeight()
                 + m_paCombatRows[i].m_hilightBorder.m_params.GetBorderDim() * 2;
        m_paCombatRows[i].SetExtent(extent);
        m_paCombatRows[i].SetActive(0);
        aRows[i] = &m_paCombatRows[i];
    }

    if (nCount != 0)
    {
        m_lbCombat.AddControls(aRows, 0, 0, 1);
        m_lbCombat.SetSelectedControl(nCount - 1, 0);
    }
}
#endif

// Commits m_nPendingFilter. Every filter button drops back to the plain colour, the chosen page is
// shown, and the chosen button is re-highlighted.
#if !KOTOR1  // K1: no filter buttons
void CSWGuiInGameMessages::SetNewFilter()
{
    CSWGuiTextParams* pSelected = &m_btnDialog.m_text.m_params;
    pSelected->SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnFeedback.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnCombat.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnEffects.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    switch (m_nPendingFilter)
    {
        case MESSAGE_PAGE_DIALOG:
            ShowDialogMessages();
            break;
        case MESSAGE_PAGE_FEEDBACK:
            ShowFeedbackMessages();
            pSelected = &m_btnFeedback.m_text.m_params;
            break;
        case MESSAGE_PAGE_COMBAT:
            ShowCombatMessages();
            pSelected = &m_btnCombat.m_text.m_params;
            break;
        case MESSAGE_PAGE_EFFECTS:
            ShowEffects();
            pSelected = &m_btnEffects.m_text.m_params;
            break;
        default:
            return;
    }
    pSelected->SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
}
#endif

void CSWGuiInGameMessages::ShowDialogMessages()
{
#if KOTOR1
    // K1: BTN_SHOW offers the other page, and the title is built from two strrefs.
    m_lbFeedback.m_tail.bVisible = 0;
    m_lbDialog.m_tail.bVisible = 1;
    m_btnShow.m_text.m_params.SetStrRef(STRREF_SHOW_FEEDBACK);
    SetActiveControl(&m_lbDialog, 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay = MESSAGE_PAGE_DIALOG;
    CExoString sTitle;
    sTitle = g_pTlkTable->GetSimpleString(STRREF_MESSAGES) + CExoString(" - ") + g_pTlkTable->GetSimpleString(STRREF_DIALOG);
    m_lblMessages.m_text.m_params.SetText(sTitle);
    IosAddButtonIcon(&m_btnShow, 'x');
#else
    m_lbFeedback.m_tail.bVisible = 0;
    m_lbDialog.m_tail.bVisible = 1;
    m_lbCombat.m_tail.bVisible = 0;
    m_lbEffectsGood.m_tail.bVisible = 0;
    m_lbEffectsBad.m_tail.bVisible = 0;
    m_lblEffectsGood.m_tail.bVisible = 0;
    m_lblEffectsBad.m_tail.bVisible = 0;
    m_btnDialog.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    m_btnFeedback.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnCombat.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnEffects.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    SetActiveControl(&m_lbDialog, 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay = MESSAGE_PAGE_DIALOG;
#endif
}

void CSWGuiInGameMessages::ShowFeedbackMessages()
{
#if KOTOR1
    // K1: the title is built from two strrefs.
    m_lbFeedback.m_tail.bVisible = 1;
    m_lbDialog.m_tail.bVisible = 0;
    m_btnShow.m_text.m_params.SetStrRef(STRREF_SHOW_DIALOG);
    SetActiveControl(&m_lbFeedback, 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay = MESSAGE_PAGE_FEEDBACK;
    CExoString sTitle;
    sTitle = g_pTlkTable->GetSimpleString(STRREF_MESSAGES) + CExoString(" - ") + g_pTlkTable->GetSimpleString(STRREF_FEEDBACK);
    m_lblMessages.m_text.m_params.SetText(sTitle);
    IosAddButtonIcon(&m_btnShow, 'x');
#else
    m_lbFeedback.m_tail.bVisible = 1;
    m_lbDialog.m_tail.bVisible = 0;
    m_lbCombat.m_tail.bVisible = 0;
    m_lbEffectsGood.m_tail.bVisible = 0;
    m_lbEffectsBad.m_tail.bVisible = 0;
    m_lblEffectsGood.m_tail.bVisible = 0;
    m_lblEffectsBad.m_tail.bVisible = 0;
    m_btnDialog.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnFeedback.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    m_btnCombat.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnEffects.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    SetActiveControl(&m_lbFeedback, 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay = MESSAGE_PAGE_FEEDBACK;
#endif
}

#if !KOTOR1  // K1: no combat log
void CSWGuiInGameMessages::ShowCombatMessages()
{
    m_lbFeedback.m_tail.bVisible = 0;
    m_lbDialog.m_tail.bVisible = 0;
    m_lbEffectsGood.m_tail.bVisible = 0;
    m_lbEffectsBad.m_tail.bVisible = 0;
    m_lbCombat.m_tail.bVisible = 1;
    m_lblEffectsGood.m_tail.bVisible = 0;
    m_lblEffectsBad.m_tail.bVisible = 0;
    m_btnDialog.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnFeedback.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnCombat.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    m_btnEffects.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    SetActiveControl(&m_lbCombat, 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay = MESSAGE_PAGE_COMBAT;
}
#endif

// The effects page is the only one built from live game state rather than from a message ring
// buffer: it walks the party leader's server-side effect-icon strip and makes one row per icon,
// split into the two list boxes by the icon's "Good" column.
#if !KOTOR1  // K1: no effects page
void CSWGuiInGameMessages::ShowEffects()
{
    m_lbFeedback.m_tail.bVisible = 0;
    m_lbDialog.m_tail.bVisible = 0;
    m_lbEffectsGood.m_tail.bVisible = 1;
    m_lbEffectsBad.m_tail.bVisible = 1;
    m_lbCombat.m_tail.bVisible = 0;
    m_lblEffectsGood.m_tail.bVisible = 1;
    m_lblEffectsBad.m_tail.bVisible = 1;
    m_btnDialog.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnFeedback.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnCombat.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnEffects.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);

    m_lbEffectsGood.ClearItems();
    m_lbEffectsBad.ClearItems();

    CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pLeader == 0)
        return;
    CSWSCreature* pServerLeader = pLeader->GetServerCreature();
    if (pServerLeader == 0)
        return;

    CExoArrayList<CSWGuiControl*> aGoodRows;
    CExoArrayList<CSWGuiControl*> aBadRows;

    CSWGuiExtent extentGood;
    extentGood.w = m_lbEffectsGood.m_innerExtent.w - m_lbEffectsGood.m_nPadding * 2;
    extentGood.h = 10;
    CSWGuiLabelHilight* pProtoGood = static_cast<CSWGuiLabelHilight*>(m_lbEffectsGood.m_pProtoItem);

    CSWGuiExtent extentBad;
    extentBad.w = m_lbEffectsBad.m_innerExtent.w - m_lbEffectsBad.m_nPadding * 2;
    extentBad.h = 10;
    CSWGuiLabelHilight* pProtoBad = static_cast<CSWGuiLabelHilight*>(m_lbEffectsBad.m_pProtoItem);

    CExoString sName;
    for (int64_t i = 0; i < pServerLeader->m_effectIcons.m_count; i++)
    {
        CEffectIconObject* pIcon = pServerLeader->m_effectIcons.m_pArray[i];
        if (pIcon->m_nNameStrRef == 0)
            continue;
        sName = g_pTlkTable->GetSimpleString(pIcon->m_nNameStrRef);

        CSWGuiLabelHilight* pRow = new CSWGuiLabelHilight;
        if (pIcon->m_bGood)
        {
            pRow->Initialize(extentGood, pProtoGood);
            pRow->m_text.m_params.SetText(sName);
            pRow->m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
            CSWGuiText* pText = &pRow->m_text;
            extentGood.h = pText->GetIdealHeight()
                         + pRow->m_hilightBorder.m_params.GetBorderDim() * 2;
            pRow->SetExtent(extentGood);
            pRow->SetActive(0);
            aGoodRows.Add(pRow);
        }
        else
        {
            pRow->Initialize(extentBad, pProtoBad);
            pRow->m_text.m_params.SetText(sName);
            pRow->m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
            CSWGuiText* pText = &pRow->m_text;
            extentBad.h = pText->GetIdealHeight()
                        + pRow->m_hilightBorder.m_params.GetBorderDim() * 2;
            pRow->SetExtent(extentBad);
            pRow->SetActive(0);
            aBadRows.Add(pRow);
        }
    }

    if (aGoodRows.m_count > 0)
    {
        m_lbEffectsGood.AddControls(aGoodRows, 1, 1, 1);
        m_lbEffectsGood.SetSelectedControl(0, 0);
    }
    if (aBadRows.m_count >= 1)
    {
        m_lbEffectsBad.AddControls(aBadRows, 1, 1, 1);
        m_lbEffectsBad.SetSelectedControl(0, 0);
    }
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay = MESSAGE_PAGE_EFFECTS;
}
#endif

void CSWGuiInGameMessages::HandleInputEvent(int nType, int nValue)
{
    if (nValue)
    {
#if KOTOR1
        // K1: the close keys hide the screen; the flip key pages it.
        switch (nType)
        {
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_START:
            case INPUTEVENT_GUI_BACK:
            case INPUTEVENT_GUI:
                if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
                    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
                break;
            case INPUTEVENT_GUI_X:
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                ToggleMessages();
                break;
        }
#else
        switch (nType)
        {
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_X:
            case INPUTEVENT_GUI_START:
            case INPUTEVENT_GUI_BACK:
            case INPUTEVENT_GUI:
            {
                // Request the panel's removal: one already marked for remove-and-delete only drops
                // its DEFERRED bit, anything else becomes a plain remove. Both arms fold into one
                // select, so the test reads the already-masked local.
                unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
                if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                    nFlags = (m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
                m_stateFlags = nFlags;
                break;
            }
            case INPUTEVENT_GUI_DPAD_RIGHT:
            {
                unsigned char nPage =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay;
                m_nPendingFilter = (nPage > MESSAGE_PAGE_COMBAT) ? MESSAGE_PAGE_DIALOG : nPage + 1;
                SetNewFilter();
                break;
            }
            case INPUTEVENT_GUI_Y:
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                g_pAppManager->m_pClientExoApp
                    ->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
                break;
            case INPUTEVENT_GUI_DPAD_LEFT:
            {
                unsigned char nPage =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay;
                m_nPendingFilter = (nPage == MESSAGE_PAGE_DIALOG) ? MESSAGE_PAGE_EFFECTS : nPage - 1;
                SetNewFilter();
                break;
            }
        }
#endif

        // The easter egg is typing the four keys in order anywhere on the log screen.
        if (s_anEasterEggKeys[s_nEasterEggProgress] == nType)
        {
            if (++s_nEasterEggProgress == 4)
            {
                // alignas(16) is load-bearing: the constant load depends on it.
                alignas(16) char szMessage[] = "Punch it, Chewie!";
                s_nEasterEggProgress = 0;
                CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
                pInGameGui->AppendToMsgBuffer(CExoString(szMessage), MSG_CHANNEL_FEEDBACK, 0);
                g_nHSMode = 1;
#if KOTOR1
                g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateMessageGUI();
#else
                g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateMessageGUI(0);
#endif
            }
        }
        else
        {
#if KOTOR1
            s_nEasterEggProgress = (nType == INPUTEVENT_GUI_X);
#else
            s_nEasterEggProgress = (nType == INPUTEVENT_GUI_Y);
#endif
        }
    }
    CSWGuiPanel::HandleInputEvent(nType, nValue);
}

#if KOTOR1
void CSWGuiInGameMessages::ToggleMessages()
{
    if (m_lbFeedback.m_tail.bVisible)
        ShowDialogMessages();
    else
        ShowFeedbackMessages();
}
#endif

void CSWGuiInGameMessages::OnPanelAdded()
{
    CSWGuiPanel::OnPanelAdded();
#if KOTOR1
    // K1: two pages.
    g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateMessageGUI();
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay)
        ShowFeedbackMessages();
    else
        ShowDialogMessages();
#else
    g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateMessageGUI(this);
    switch (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nMessageDisplay)
    {
        case MESSAGE_PAGE_DIALOG:   ShowDialogMessages();   break;
        case MESSAGE_PAGE_FEEDBACK: ShowFeedbackMessages(); break;
        case MESSAGE_PAGE_COMBAT:   ShowCombatMessages();   break;
        case MESSAGE_PAGE_EFFECTS:  ShowEffects();          break;
    }
#endif
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_MESSAGES_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
}

float GetResolutionTextScalar();
extern CTlkTable* g_pTlkTable;
bool IsJoystickConnected();
extern int gDontMoveCursor_Mac;

// Builds the widget wall: layout "statussummary_p", the 26 label binds (indices 2..27 of
// m_aValueLabels/m_aDescLabels) plus BTN_OK, then the button's resolution-rescaled extent and its
// activate event, then the accumulators.
CSWGuiStatusSummary::CSWGuiStatusSummary(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: "statussummary", nine rows rescaled for the screen; the OK button keeps its layout
    // height.
    StartLoadFromLayout(CResRef("statussummary"));

    InitControl(m_aValueLabels[0], CExoString("LBL_JOURNAL"), 1);
    InitControl(m_aDescLabels[0], CExoString("LBL_JOURNAL_DESC"), 1);
    InitControl(m_aValueLabels[1], CExoString("LBL_CREDITS"), 1);
    InitControl(m_aDescLabels[1], CExoString("LBL_CREDITS_DESC"), 1);
    InitControl(m_aValueLabels[2], CExoString("LBL_XP"), 1);
    InitControl(m_aDescLabels[2], CExoString("LBL_XP_DESC"), 1);
    InitControl(m_aValueLabels[3], CExoString("LBL_STEALTH"), 1);
    InitControl(m_aDescLabels[3], CExoString("LBL_STEALTH_DESC"), 1);
    InitControl(m_aValueLabels[4], CExoString("LBL_DARKSIDE"), 1);
    InitControl(m_aDescLabels[4], CExoString("LBL_DARKSIDE_DESC"), 1);
    InitControl(m_aValueLabels[5], CExoString("LBL_LIGHTSIDE"), 1);
    InitControl(m_aDescLabels[5], CExoString("LBL_LIGHTSIDE_DESC"), 1);
    InitControl(m_aValueLabels[6], CExoString("LBL_NETSHIFT"), 1);
    InitControl(m_aDescLabels[6], CExoString("LBL_NETSHIFT_DESC"), 1);
    InitControl(m_aValueLabels[7], CExoString("LBL_RECEIVED"), 1);
    InitControl(m_aDescLabels[7], CExoString("LBL_RECEIVED_DESC"), 1);
    InitControl(m_aValueLabels[8], CExoString("LBL_LOST"), 1);
    InitControl(m_aDescLabels[8], CExoString("LBL_LOST_DESC"), 1);
    InitControl(m_btnAck,      CExoString("BTN_OK"), 1);

    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_btnAck.m_tail.bFocusOnHover = 0;
    m_btnAck.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStatusSummary::OnAButtonPressed));

    m_credits = 0;
    m_lightShift = 0;
    m_darkShift = 0;
    m_flags &= ~SUMMARY_ALL;
    m_nXP = 0;
    m_nStealthXP = 0;
    m_suppressFlag = 0;
    m_stateFlags |= 1;
#else
    StartLoadFromLayout(CResRef("statussummary_p"), 1, 1, 1);

    InitControl(m_aValueLabels[0], CExoString("LBL_JOURNAL"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[0], CExoString("LBL_JOURNAL_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[1], CExoString("LBL_CREDITS"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[1], CExoString("LBL_CREDITS_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[2], CExoString("LBL_XP"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[2], CExoString("LBL_XP_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[3], CExoString("LBL_STEALTH"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[3], CExoString("LBL_STEALTH_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[4], CExoString("LBL_DARKSIDE"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[4], CExoString("LBL_DARKSIDE_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[5], CExoString("LBL_LIGHTSIDE"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[5], CExoString("LBL_LIGHTSIDE_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[6], CExoString("LBL_NETSHIFT"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[6], CExoString("LBL_NETSHIFT_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[7], CExoString("LBL_RECEIVED"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[7], CExoString("LBL_RECEIVED_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[8], CExoString("LBL_LOST"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[8], CExoString("LBL_LOST_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[9], CExoString("LBL_INFLUENCE_RECV"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[9], CExoString("LBL_INFLUENCE_RECV_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[10], CExoString("LBL_INFLUENCE_LOST"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[10], CExoString("LBL_INFLUENCE_LOST_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[11], CExoString("LBL_MAX_FP_GAINED"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[11], CExoString("LBL_MAX_FP_GAINED_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_aValueLabels[12], CExoString("LBL_MAX_FP_LOST"), 1, 1, 0, 0, 0);
    InitControl(m_aDescLabels[12], CExoString("LBL_MAX_FP_LOST_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_btnAck,      CExoString("BTN_OK"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();

    // The OK button grows with the text-resolution scalar (the client resolutions above the
    // 1.0 baseline), preserving its layout height's share of the scale.
    float fScale = GetResolutionTextScalar();
    CSWGuiExtent extent = m_btnAck.m_extent;
    extent.h = static_cast<int>(((fScale - 1.0f) * 0.1f + 1.0f) * static_cast<float>(extent.h));
    m_btnAck.SetExtent(extent);

    m_btnAck.m_tail.bFocusOnHover = 0;
    m_btnAck.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStatusSummary::OnAButtonPressed));

    m_credits = 0;
    m_lightShift = 0;
    m_darkShift = 0;
    m_nAlignmentChanged = 0;
    m_flags &= ~SUMMARY_ALL;
    m_suppressFlag = 0;
    m_influenceGainedMask = 0;
    m_influenceLostMask = 0;
    m_maxFPGained = 0;
    m_nXP = 0;
    m_nStealthXP = 0;
    m_maxFPLost = 0;
    m_stateFlags |= 1;
#endif
}

// Applies what the character gained since the panel was last shown: re-lays each status row's
// value and description from the talk table, flashes the rows whose accumulator is set, then
// re-centres the panel, clears the accumulators and hands the cursor back.
void CSWGuiStatusSummary::OnPanelAdded()
{
#if KOTOR1
    // K1: nine rows sized by the screen height over 768; the cursor is not moved.
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    float fScale = s_fScale;
    CExoString sToken;
    CSWGuiExtent extent;
    int bShowJournal = 0;

    PlayPendingSounds();

    int nMaxWidth = static_cast<int>(fScale * 150.0f);
    int nRowY = static_cast<int>(fScale * 10.0f);

    for (int i = 0; i < 9; i++)
    {
        int bShow = 0;
        int bFlash = 0;
        int bSetText = 0;

        switch (i)
        {
        case SUMMARY_ROW_JOURNAL:
            bShowJournal = (m_flags & SUMMARY_JOURNAL) != 0;
            bShow = bShowJournal;
            bFlash = 1;
            break;

        case SUMMARY_ROW_CREDITS:
            if (m_credits != 0 || (m_flags & SUMMARY_CREDITS_NET) != 0)
            {
                if (m_credits < 0)
                {
                    g_pTlkTable->SetCustomToken(0, CExoString(-m_credits));
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CREDITS_LOST);
                }
                else
                {
                    g_pTlkTable->SetCustomToken(0, CExoString(m_credits));
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CREDITS_RECEIVED);
                }
                if (m_flags & SUMMARY_CREDITS_NET)
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NET_CHANGE) + sToken;
                bShow = 1;
                bFlash = 1;
                bSetText = 1;
            }
            break;

        case SUMMARY_ROW_XP:
            if (m_nXP != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(m_nXP));
                sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_XP_RECEIVED);
                bShow = 1;
                bFlash = 1;
                bSetText = 1;
            }
            break;

        case SUMMARY_ROW_STEALTH_XP:
            if (m_nStealthXP != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(m_nStealthXP));
                sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_STEALTH_XP_RECEIVED);
                bShow = 1;
                bFlash = 1;
                bSetText = 1;
            }
            break;

        case SUMMARY_ROW_DARK_SIDE:
            bShow = (m_darkShift != 0);
            bFlash = (m_darkShift > m_lightShift);
            break;

        case SUMMARY_ROW_LIGHT_SIDE:
            bShow = (m_lightShift != 0);
            bFlash = (m_lightShift > m_darkShift);
            break;

        case SUMMARY_ROW_NET_SHIFT:
            if (m_lightShift != 0 && m_lightShift != m_darkShift && m_darkShift != 0)
            {
                if (m_lightShift > m_darkShift)
                {
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NET_LIGHT_SIDE_SHIFT);
                    m_aValueLabels[i].m_border.m_params.SetFillImage(
                        m_aValueLabels[SUMMARY_ROW_LIGHT_SIDE].m_border.m_params.m_fillImage, 0);
                }
                else
                {
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NET_DARK_SIDE_SHIFT);
                    m_aValueLabels[i].m_border.m_params.SetFillImage(
                        m_aValueLabels[SUMMARY_ROW_DARK_SIDE].m_border.m_params.m_fillImage, 0);
                }
                bShow = 1;
                bSetText = 1;
            }
            break;

        case SUMMARY_ROW_ITEM_RECEIVED:
            bShow = (m_flags & SUMMARY_ITEM_RECEIVED) != 0;
            bFlash = 1;
            break;

        case SUMMARY_ROW_ITEM_LOST:
            bShow = (m_flags & SUMMARY_ITEM_LOST) != 0;
            bFlash = 1;
            break;
        }

        if (bShow)
        {
            m_aValueLabels[i].m_tail.bVisible = 1;
            m_aDescLabels[i].m_tail.bVisible = 1;

            if (bSetText)
            {
                m_aDescLabels[i].m_text.m_params.SetText(sToken);
                m_aDescLabels[i].m_text.m_params.ReSetFont();
            }

            extent = m_aValueLabels[i].m_extent;
            extent.y = nRowY;
            m_aValueLabels[i].SetExtent(extent);

            CSWGuiText* pText = &m_aDescLabels[i].m_text;

            extent = m_aDescLabels[i].m_extent;
            extent.y = nRowY - static_cast<int>(s_fScale);
            extent.w = nMaxWidth;
            m_aDescLabels[i].SetExtent(extent);

            if (m_aDescLabels[i].m_text.m_params.m_sText != "")
            {
                int nMaxDescWidth = static_cast<int>(ceilf(s_fScale * 440.0f));
                int bDone = 0;
                while (pText->GetIdealHeight() / pText->GetFontHeight() != 1 && !bDone)
                {
                    int nWidth = nMaxWidth + static_cast<int>(ceilf(s_fScale * 20.0f));
                    if (nWidth > nMaxDescWidth)
                    {
                        extent.h = pText->GetIdealHeight();
                        bDone = 1;
                    }
                    else
                    {
                        nMaxWidth = nWidth;
                        extent.w = nMaxWidth;
                    }
                    m_aDescLabels[i].SetExtent(extent);
                }
            }

            if (bFlash)
                g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowFlashingStatus(static_cast<unsigned char>(i));

            int nHeight = pText->GetIdealHeight();
            int nMinHeight = static_cast<int>(ceilf(s_fScale * 37.0f));
#ifndef K2_UI_TOUCH
            // pc / xbox: the PC layout's icons are 32 of 640 columns, taller than 37 of the
            // 768-line unit; the row holds its icon and the layout's 5-line gap.
            int nIconHeight = m_aValueLabels[i].m_extent.h + static_cast<int>(ceilf(s_fScale * 5.0f));
            if (nMinHeight < nIconHeight)
                nMinHeight = nIconHeight;
#endif
            nRowY = nRowY + static_cast<int>(ceilf(s_fScale * 10.0f)) + (nHeight > nMinHeight ? nHeight : nMinHeight);
        }
        else
        {
            m_aValueLabels[i].m_tail.bVisible = 0;
            m_aDescLabels[i].m_tail.bVisible = 0;
        }
    }

    int nButtonY = static_cast<int>(nRowY - s_fScale * 7.0f);
    extent.w = static_cast<int>(s_fScale * 10.0f + (m_aDescLabels[0].m_extent.x + nMaxWidth));
    extent.h = static_cast<int>(s_fScale * 62.0f + nButtonY);
    extent.y = (m_pGuiManager->m_nScreenHeight - extent.h) / 2;
    extent.x = (m_pGuiManager->m_nScreenWidth - extent.w) / 2;
    SetExtent(extent);

    int nPanelWidth = extent.w;
    extent = m_btnAck.m_extent;
    extent.x = (nPanelWidth - extent.w) / 2;
    extent.y = nButtonY;
    m_btnAck.SetExtent(extent);

    Clear();

    CSWGuiPanel::OnPanelAdded();

    if (bShowJournal)
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_RECEIVE_JOURNAL_ENTRY, OBJECT_INVALID, OBJECT_INVALID, 0);
#else
    float fScale = GetResolutionTextScalar();
    CExoString sToken;
    CSWGuiExtent extent;
    int bShowJournal = 0;

#ifdef K2_QOL_IMPROVEMENTS
    K2QolResetStatusSummary(*this);   // platforms/qol/k2qolpage.cpp: undo the last Text Size layout
#endif
    PlayPendingSounds();

    if (m_darkShift != 0 || m_lightShift != 0)
        m_nAlignmentChanged = 1;

    // Resolution-scaled layout constants (all ints except the row step, which stays a float).
    int nRowY = static_cast<int>(fScale * 1.8f * 10.0f);
    int nLabelSize = static_cast<int>(fScale * 32.0f);
    int nMaxWidth = static_cast<int>(fScale * 1.5f * 150.0f);
    int nTextStep = static_cast<int>(fScale * 1.5f * 20.0f);
    int nMaxHeight = static_cast<int>(fScale * 1.5f * 300.0f);
    float fRowStep = fScale * 37.0f;
#if defined(K2_HOST_BUILD) && defined(K2_UI_XBOX)
    // The panel fits its rows; the floor is the OK button, applied below.
    nMaxWidth = 0;
    // The console text scale grows the icons past a PC layout's (drawn at the PC scale), which
    // would close the gap to the text; the description column keeps the layout's gap in
    // proportion to the icon, measured once while the icons are still at the layout's size.
    static float s_fDescGap = -1.0f;
    if (s_fDescGap < 0.0f)
    {
        const CSWGuiExtent& icon = m_aValueLabels[0].m_extent;
        s_fDescGap = icon.w > 0 ? static_cast<float>(m_aDescLabels[0].m_extent.x - icon.x - icon.w) / static_cast<float>(icon.w) : 0.0f;
        if (s_fDescGap < 0.0f)
            s_fDescGap = 0.0f;
    }
    for (int i = 0; i < 13; i++)
    {
        extent = m_aDescLabels[i].m_extent;
        extent.x = m_aValueLabels[i].m_extent.x + static_cast<int>(static_cast<float>(nLabelSize) * (1.0f + s_fDescGap));
        m_aDescLabels[i].SetExtent(extent);
    }
#endif

    int* paInfluenceGained = &m_influenceGainedMask;
    int* paInfluenceLost = &m_influenceLostMask;
    CSWGuiText* pReceivedText = &m_aDescLabels[SUMMARY_ROW_ITEM_RECEIVED].m_text;

    for (int i = 0; i < 13; i++)
    {
        int bShow = 0;
        int bFlash = 0;
        int bSetText = 0;

        switch (i)
        {
        case SUMMARY_ROW_JOURNAL:
            bShowJournal = (m_flags & SUMMARY_JOURNAL) != 0;
            bShow = bShowJournal;
            bFlash = 1;
            break;

        case SUMMARY_ROW_CREDITS:
            if (m_credits != 0 || (m_flags & SUMMARY_CREDITS_NET) != 0)
            {
                if (m_credits < 0)
                {
                    g_pTlkTable->SetCustomToken(0, CExoString(-m_credits));
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CREDITS_LOST);
                }
                else
                {
                    g_pTlkTable->SetCustomToken(0, CExoString(m_credits));
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CREDITS_RECEIVED);
                }
                if (m_flags & SUMMARY_CREDITS_NET)
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NET_CHANGE) + sToken;
                bShow = 1;
                bFlash = 1;
                bSetText = 1;
            }
            break;

        case SUMMARY_ROW_XP:
            if (m_nXP != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(m_nXP));
                sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_XP_RECEIVED);
                bShow = 1;
                bFlash = 1;
                bSetText = 1;
            }
            break;

        case SUMMARY_ROW_STEALTH_XP:
            if (m_nStealthXP != 0)
            {
                g_pTlkTable->SetCustomToken(0, CExoString(m_nStealthXP));
                sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_STEALTH_XP_RECEIVED);
                bShow = 1;
                bFlash = 1;
                bSetText = 1;
            }
            break;

        case SUMMARY_ROW_DARK_SIDE:
            bShow = (m_darkShift != 0);
            bFlash = (m_darkShift > m_lightShift);
            break;

        case SUMMARY_ROW_LIGHT_SIDE:
            bShow = (m_lightShift != 0);
            bFlash = (m_lightShift > m_darkShift);
            break;

        case SUMMARY_ROW_NET_SHIFT:
            if (m_lightShift != 0 && m_lightShift != m_darkShift && m_darkShift != 0)
            {
                if (m_lightShift > m_darkShift)
                {
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NET_LIGHT_SIDE_SHIFT);
                    m_aValueLabels[i].m_border.m_params.SetFillImage(
                        m_aValueLabels[SUMMARY_ROW_LIGHT_SIDE].m_border.m_params.m_fillImage, 0);
                }
                else
                {
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NET_DARK_SIDE_SHIFT);
                    m_aValueLabels[i].m_border.m_params.SetFillImage(
                        m_aValueLabels[SUMMARY_ROW_DARK_SIDE].m_border.m_params.m_fillImage, 0);
                }
                bShow = 1;
                bSetText = 1;
            }
            break;

#if !KOTOR1  // K1 has no received-item summary row (no CGuiInGame item-name list)
        case SUMMARY_ROW_ITEM_RECEIVED:
        {
            CGuiInGame* pGuiInGame = g_pAppManager->m_pClientExoApp->GetInGameGui();
            CExoString sAccum;
            for (int j = 0; j < pGuiInGame->m_aReceivedItemNames.m_count; j++)
            {
                CExoLocString locString(pGuiInGame->m_aReceivedItemNames.m_pArray[j]);
                CExoString sOut;
                locString.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sOut, 0);
                if (sAccum == "")
                    sAccum = sOut + "\n";
                else
                    sAccum.Format(const_cast<char*>("%s%s\n"), sAccum.CStr(), sOut.CStr());
                int nLT = sAccum.Find('<', 0);
                int nBrace = sAccum.Find('{', 0);
                if (nLT != -1 || nBrace != -1)
                {
                    CSWTlkTable* pTlk = g_pSWTlkTable;
                    pTlk->m_pSpeakerObj =
                        g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
                    pTlk->m_gender = pTlk->GetObjectGender();
                    pTlk->m_bSuppressEnabled |= 1;
                    pTlk->ParseStr(sAccum);
                }
            }
            if (sAccum == "")
            {
                bShow = 0;
            }
            else
            {
                g_pTlkTable->SetCustomToken(0, sAccum);
                sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ITEMS_RECEIVED);
                bShow = 1;
                bSetText = 1;
            }
            bFlash = 1;
            break;
        }
#endif

        case SUMMARY_ROW_ITEM_LOST:
            bShow = (m_flags & SUMMARY_ITEM_LOST) != 0;
            bFlash = 1;
            break;

        case SUMMARY_ROW_INFLUENCE_GAINED:
        case SUMMARY_ROW_INFLUENCE_LOST:
        {
            CExoString sAccum;
            int nMask = (i == SUMMARY_ROW_INFLUENCE_GAINED) ? *paInfluenceGained : *paInfluenceLost;
            g_pAppManager->m_pServerExoApp->GetPartyTable();
            for (int j = 0; j < 12; j++)
            {
                if (nMask & (1 << j))
                {
                    CExoString sMember;
                    // inlined CSWTlkTable::Fetch(unsigned long, CGameObject*, int, int)
                    switch (j)
                    {
                    case NPC_ATTON:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTON);
                        break;
                    }
                    case NPC_BAO_DUR:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_BAO_DUR);
                        break;
                    }
                    case NPC_CANDEROUS:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MANDALORE);
                        break;
                    }
                    case NPC_G0T0:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_G0_T0);
                        break;
                    }
                    case NPC_HANDMAIDEN:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_HANDMAIDEN);
                        break;
                    }
                    case NPC_HK_47:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_HK_47);
                        break;
                    }
                    case NPC_KREIA:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_KREIA);
                        break;
                    }
                    case NPC_MIRA:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_MIRA);
                        break;
                    }
                    case NPC_T3_M4:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_T3_M4);
                        break;
                    }
                    case NPC_VISAS:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_VISAS);
                        break;
                    }
                    case NPC_HANHARR:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_HANHARR);
                        break;
                    }
                    case NPC_DISCIPLE:
                    {
                        CSWTlkTable* pTlk = g_pSWTlkTable;
                        pTlk->m_pSpeakerObj = 0;
                        pTlk->m_gender = pTlk->GetObjectGender();
                        pTlk->m_bSuppressEnabled &= ~1;
                        sMember = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DISCIPLE);
                        break;
                    }
                    }
                    if (sAccum == "")
                        sAccum = sMember;
                    else
                        sAccum.Format(const_cast<char*>("%s, %s"), sAccum.CStr(), sMember.CStr());
                }
            }
            if (sAccum == "")
            {
                bShow = 0;
            }
            else
            {
                g_pTlkTable->SetCustomToken(0, sAccum);
                if (i == SUMMARY_ROW_INFLUENCE_GAINED)
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_INFLUENCE_GAINED);
                else
                    sToken = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_INFLUENCE_LOST);
                bShow = 1;
                bSetText = 1;
            }
            bFlash = 1;
            break;
        }

        case SUMMARY_ROW_MAX_FP_GAINED:
            bShow = (m_maxFPGained != 0);
            bFlash = 1;
            break;

        case SUMMARY_ROW_MAX_FP_LOST:
            bShow = (m_maxFPLost != 0);
            bFlash = 1;
            break;
        }

        if (bShow)
        {
            m_aValueLabels[i].m_tail.bVisible = 1;
            m_aDescLabels[i].m_tail.bVisible = 1;

            if (bSetText)
            {
                m_aDescLabels[i].m_text.m_params.SetText(sToken);
                m_aDescLabels[i].m_text.m_params.ReSetFont();
            }

            extent.x = m_aValueLabels[i].m_extent.x;
            extent.y = nRowY;
            extent.w = nLabelSize;
            extent.h = nLabelSize;
            m_aValueLabels[i].SetExtent(extent);

            // The pointer keeps the call virtual.
            CSWGuiText* pText = &m_aDescLabels[i].m_text;

            extent = m_aDescLabels[i].m_extent;
            extent.y = nRowY - 1;
            extent.h = (i == SUMMARY_ROW_ITEM_RECEIVED) ? pReceivedText->GetIdealHeight() : 31;
            extent.w = pText->GetIdealWidthAndHeight().w;
            if (nMaxWidth < extent.w)
                nMaxWidth = extent.w;
            extent.h = static_cast<int>(extent.h * fScale);
            m_aDescLabels[i].SetExtent(extent);

            // A description that wraps widens (nMaxWidth, the running width the panel is sized
            // from) one step at a time until it is a single line; the received-items row is given
            // the larger of that width and nMaxHeight.
            if (i == 7)
            {
                if (m_aDescLabels[i].m_text.m_params.m_sText != "")
                {
#if !(defined(K2_HOST_BUILD) && defined(K2_UI_XBOX))   // xbox: the names keep their own width
                    if (nMaxWidth < nMaxHeight)
                        nMaxWidth = nMaxHeight;
#endif
                    extent.w = nMaxWidth;
                    m_aDescLabels[i].SetExtent(extent);
                }
            }
            else if (m_aDescLabels[i].m_text.m_params.m_sText != "")
            {
                while (nMaxWidth <= 439)
                {
                    if (pText->GetIdealHeight() == pText->GetFontHeight())
                        break;
                    nMaxWidth += nTextStep;
                    extent.w = nMaxWidth;
                    m_aDescLabels[i].SetExtent(extent);
                }
            }

            if (bFlash)
                g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowFlashingStatus(static_cast<unsigned char>(i));

            if (i == 7)
            {
                int nIdealHeight = pText->GetIdealHeight();
                if (nIdealHeight > 37)
                    nRowY += nIdealHeight;
                else
                    nRowY += 37;
            }
            else
            {
                nRowY = static_cast<int>(fRowStep + nRowY);
            }
        }
        else
        {
            m_aValueLabels[i].m_tail.bVisible = 0;
            m_aDescLabels[i].m_tail.bVisible = 0;
        }
    }

#if defined(K2_HOST_BUILD) && defined(K2_UI_XBOX)
    // At the console text scale the 30-unit margin outgrows the layout; the right edge keeps the
    // icons' inset from the left.
    extent.w = nMaxWidth + m_aValueLabels[0].m_extent.x + m_aDescLabels[0].m_extent.x;
    if (extent.w < m_btnAck.m_extent.w + 2 * m_aValueLabels[0].m_extent.x)
        extent.w = m_btnAck.m_extent.w + 2 * m_aValueLabels[0].m_extent.x;
#elif defined(K2_FIX_ENGINE_BUGS)
    extent.w = nMaxWidth + static_cast<int>(fScale * 1.5f * 20.0f) + m_aDescLabels[0].m_extent.x;
#else
    extent.w = nMaxWidth + static_cast<int>(fScale * 1.5f * 20.0f * fScale) + m_aDescLabels[0].m_extent.x;
#endif
    extent.h = nRowY + m_btnAck.m_extent.h + 20;
    extent.y = (m_pGuiManager->m_nScreenHeight - extent.h) / 2;
    extent.x = (m_pGuiManager->m_nScreenWidth - extent.w) / 2;
    SetExtent(extent);

    // The OK button is centred in the panel's width; the panel width is read before the local is
    // overwritten by the button extent.
    int nPanelWidth = extent.w;
    extent = m_btnAck.m_extent;
    extent.x = (nPanelWidth - extent.w) / 2;
    extent.y = nRowY;
    m_btnAck.SetExtent(extent);

#ifdef K2_QOL_IMPROVEMENTS
    K2QolLayoutStatusSummary(*this);   // the rows again at the Text Size
#endif
    Clear();

    CSWGuiPanel::OnPanelAdded();

    if (bShowJournal)
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_RECEIVE_JOURNAL_ENTRY, OBJECT_INVALID, OBJECT_INVALID, 0);

    if (IsJoystickConnected())
        return;
    if (gDontMoveCursor_Mac != 0)
        return;

    // All six extent words are read before the cursor position is stored.
    int nMouseX = m_extent.x + m_btnAck.m_extent.x + m_btnAck.m_extent.w / 2;
    int nMouseY = m_extent.y + m_btnAck.m_extent.y + m_btnAck.m_extent.h / 2;
    g_pGuiMan->StoreCurrentMousePosition();
    g_pGuiMan->MoveMouseToPosition(nMouseX, nMouseY);
#endif
}

// Leaving the status panel hands the cursor back.
void CSWGuiStatusSummary::OnPanelRemoved()
{
    g_pGuiMan->RestoreOldMousePosition();
}

void CSWGuiStatusSummary::Clear() {
#if KOTOR1  // K1: nine flag bits, no influence or max-FP accumulators
    m_lightShift = 0;
    m_darkShift = 0;
    m_flags &= ~SUMMARY_ALL;
    m_credits = 0;
    m_nXP = 0;
    m_nStealthXP = 0;
#else
    m_lightShift = 0;
    m_darkShift = 0;
    m_credits = 0;
    m_flags &= ~SUMMARY_ALL;
    m_nXP = 0;
    m_nStealthXP = 0;
    m_influenceGainedMask = 0;
    m_influenceLostMask = 0;
    m_maxFPGained = 0;
    m_maxFPLost = 0;
#endif
}

#if !KOTOR1  // K1: no influence or max-FP rows
void CSWGuiStatusSummary::AddInfluenceGained(char nMemberId) {
    m_influenceGainedMask |= (1 << nMemberId);
    m_flags |= 1;
}

void CSWGuiStatusSummary::AddInfluenceLost(char nMemberId) {
    m_influenceLostMask |= (1 << nMemberId);
    m_flags |= 1;
}

void CSWGuiStatusSummary::AddMaxFPGained(int nAmount) {
    m_maxFPGained += nAmount;
    m_flags |= 1;
}

void CSWGuiStatusSummary::AddMaxFPLost(int nAmount) {
    m_maxFPLost += nAmount;
    m_flags |= 1;
}
#endif

// Adds to the alignment accumulator, the light/dark shift byte pair and the two dominant-side flag
// bits. While the two bytes tie, both dominant-side bits are cleared, and the light-side one
// additionally only when the feedback bits are clear. Bit 0 is the "something changed" latch every
// accumulator here sets.
void CSWGuiStatusSummary::AddAlignmentShift(char shift) {
    if (shift == 0)
        return;
    if (shift > 0)
        m_lightShift += shift;
    else
        m_darkShift -= shift;

#if !KOTOR1  // K1: no dominant-side bits
    if (m_lightShift == m_darkShift) {
        m_flags &= ~(SUMMARY_DARK_ALIGN_SOUND | SUMMARY_LIGHT_ALIGN_SOUND);
        if ((m_flags & (SUMMARY_LEVEL_UP_SOUND | SUMMARY_NEW_QUEST_SOUND | SUMMARY_COMPLETE_SOUND)) == 0)
            m_flags = m_flags & ~(SUMMARY_DARK_ALIGN_SOUND | SUMMARY_SOUND_PENDING);
    } else if (m_lightShift > m_darkShift) {
        m_flags = (m_flags & ~(SUMMARY_DARK_ALIGN_SOUND | SUMMARY_LIGHT_ALIGN_SOUND | SUMMARY_SOUND_PENDING)) |
                  SUMMARY_LIGHT_ALIGN_SOUND | SUMMARY_SOUND_PENDING;
    } else {
        m_flags = (m_flags & ~(SUMMARY_DARK_ALIGN_SOUND | SUMMARY_LIGHT_ALIGN_SOUND | SUMMARY_SOUND_PENDING)) |
                  SUMMARY_DARK_ALIGN_SOUND | SUMMARY_SOUND_PENDING;
    }
#endif
    m_flags |= 1;
}

// Sets the flag bit meaning the credit change runs against the balance's sign: the money went the
// other way round from a positive/negative balance, and the popup shows a different line for it.
void CSWGuiStatusSummary::AddCredits(int credits) {
    if (credits == 0)
        return;
    if (credits < 0) {
        if (m_credits > 0)
            m_flags |= SUMMARY_CREDITS_NET;
    }
    if (credits > 0) {
        if (m_credits < 0)
            m_flags |= SUMMARY_CREDITS_NET;
    }
    m_credits += credits;
    m_flags |= 1;
}

#if !KOTOR1  // K1: no alignment movie or level-up dialog
// Fire the alignment cinematic's script once.
void CSWGuiStatusSummary::HandleAlignmentMovie() {
    CExoString sScript("k_align_movie");
    g_pVirtualMachine->RunScript(&sScript, 0, 0);
}

// Fires the level-up dialog once, only for a player-made PC that can still level, and only if the
// "000_Level_Dlg_Fired" global is still clear; that global is latched before the script runs.
void CSWGuiStatusSummary::HandleLevelUpDLG() {
    unsigned long long oidPC = g_pAppManager->m_pServerExoApp->GetFirstPCObject();
    CSWSCreature* pCreature =
        static_cast<CSWSCreature*>(g_pAppManager->m_pServerExoApp->GetGameObject(oidPC));
    if (pCreature == 0)
        return;
    if (pCreature->m_bPlayerMadeCharacter == 0)
        return;
    if (pCreature->m_pStats->CanLevelUp() == 0)
        return;

    char bFired = 0;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(
        CExoString("000_Level_Dlg_Fired"), bFired);
    if (bFired)
        return;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
        CExoString("000_Level_Dlg_Fired"), 1);

    CExoString sScript("k_level_dlg");
    g_pVirtualMachine->RunScript(&sScript, 0, 0);
}
#endif

// Flashes the icon of every status row whose accumulator is set, then clears the accumulators.
void CSWGuiStatusSummary::DisplayStatus() {
    PlayPendingSounds();
#if KOTOR1  // K1: nine rows
    for (int i = 0; i < 9; i++) {
#else
    for (int i = 0; i < 13; i++) {
#endif
        int bShow = 0;
        switch (i) {
        case SUMMARY_ROW_JOURNAL: bShow = (m_flags & SUMMARY_JOURNAL) != 0; break;
        case SUMMARY_ROW_CREDITS: bShow = (m_credits != 0); break;
        case SUMMARY_ROW_XP: bShow = (m_nXP != 0); break;
        case SUMMARY_ROW_STEALTH_XP: bShow = (m_nStealthXP != 0); break;
        case SUMMARY_ROW_DARK_SIDE: bShow = m_darkShift > m_lightShift; break;
        case SUMMARY_ROW_LIGHT_SIDE: bShow = m_lightShift > m_darkShift; break;
        case SUMMARY_ROW_ITEM_RECEIVED: bShow = (m_flags & SUMMARY_ITEM_RECEIVED) != 0; break;
        case SUMMARY_ROW_ITEM_LOST: bShow = (m_flags & SUMMARY_ITEM_LOST) != 0; break;
#if !KOTOR1
        case SUMMARY_ROW_INFLUENCE_GAINED: bShow = m_influenceGainedMask; break;
        case SUMMARY_ROW_INFLUENCE_LOST: bShow = m_influenceLostMask; break;
        case SUMMARY_ROW_MAX_FP_GAINED: bShow = m_maxFPGained; break;
        case SUMMARY_ROW_MAX_FP_LOST: bShow = m_maxFPLost; break;
#endif
        }
        if (bShow != 0)
            g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowFlashingStatus(static_cast<unsigned char>(i));
    }
    Clear();
}

// Plays the queued feedback sounds out of the flag word (the three progress blips), then turns
// the two alignment bits into sound plus a one-shot visual effect on the party leader's server
// creature.
void CSWGuiStatusSummary::PlayPendingSounds() {
    if (m_flags & SUMMARY_LEVEL_UP_SOUND)
        m_pGuiManager->PlayGuiSound(GUISOUND_LEVEL_UP_NOTIFY);
    if (m_flags & SUMMARY_NEW_QUEST_SOUND)
        m_pGuiManager->PlayGuiSound(GUISOUND_QUEST_NEW_NOTIFY);
    if (m_flags & SUMMARY_COMPLETE_SOUND)
        m_pGuiManager->PlayGuiSound(GUISOUND_QUEST_COMPLETE_NOTIFY);

#if !KOTOR1  // K1: no alignment sounds or effects
    CSWSCreature* pServer = 0;
    CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pMember != 0)
        pServer = pMember->GetServerCreature();

    if (m_flags & SUMMARY_DARK_ALIGN_SOUND) {
        m_pGuiManager->PlayGuiSound(GUISOUND_DARK_SIDE);
        m_flags &= ~SUMMARY_DARK_ALIGN_SOUND;
        if (pServer != 0 && pServer->m_bPlayerMadeCharacter != 0) {
            CGameEffect* pEffect = new CGameEffect(1);
            pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
            pEffect->SetInteger(0, VFX_IMP_DARK_SIDE);
            pEffect->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pEffect->SetCreator(pServer->m_objectId);
            pServer->ApplyEffect(pEffect, 0, 0);
        }
    }
    if (m_flags & SUMMARY_LIGHT_ALIGN_SOUND) {
        m_pGuiManager->PlayGuiSound(GUISOUND_LIGHT_SIDE);
        m_flags &= ~SUMMARY_LIGHT_ALIGN_SOUND;
        if (pServer != 0 && pServer->m_bPlayerMadeCharacter != 0) {
            CGameEffect* pEffect = new CGameEffect(1);
            pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
            pEffect->SetInteger(0, VFX_IMP_LIGHT_SIDE);
            pEffect->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pEffect->SetCreator(pServer->m_objectId);
            pServer->ApplyEffect(pEffect, 0, 0);
        }
    }
#endif
}

// The four activate/escape input classes hand the panel's state back to the manager. The panel
// state is re-armed for "layout loaded", the alignment movie re-runs, the level-up dialog is
// re-checked, the dialog-span cache is dropped without freeing, and the game is unpaused unless
// this frame already latched the pause.
void CSWGuiStatusSummary::HandleInputEvent(int type, int val) {
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_A:
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_START:
        case INPUTEVENT_GUI_BACK:
            unsigned short nNewState = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
            if ((m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                nNewState = (m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE;
            m_stateFlags = nNewState;
            g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
#if !KOTOR1  // K1: no alignment movie, level-up dialog or received-item list
            HandleAlignmentMovie();
            HandleLevelUpDLG();
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_aReceivedItemNames.m_count = 0;
#endif
            if ((m_bTogglePauseAtEndOfFrame & 1) == 0)
                g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

// CSWGuiDeathDisplay, the "you died" modal. Its layout and sizeof correction live in the header.









void AspyrAddStringIcon(CExoString& sText, char nIcon);

#if !KOTOR1  // K1 has no CSWGuiDeathDisplay (the death message is CSWGuiMessageBox::DisplayDeathMessage)
CSWGuiDeathDisplay::CSWGuiDeathDisplay(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("gameover_p"), 1, 1, 1);

    InitControl(m_lblMessage,  CExoString("LBL_MESSAGE"),  1, 1, 0, 0, 0);
#ifdef K2_QOL_IMPROVEMENTS
    // "Your entire party has been killed": the layout's text, fitted to its own box.
    K2QolMarkFitText(&m_lblMessage.m_text);
    K2QolFitText(m_lblMessage.m_text, m_lblMessage.m_text.m_extent.h);
#endif
    InitControl(m_btnLastSave, CExoString("BTN_LASTSAVE"), 1, 1, 0, 0, 0);
    InitControl(m_btnLoadGame, CExoString("BTN_LOADGAME"), 1, 1, 0, 0, 0);
    InitControl(m_btnQuit,     CExoString("BTN_QUIT"),     1, 1, 0, 0, 0);

    StopLoadFromLayout();

    // All three buttons bind the activate event; the handler disambiguates by reading the
    // control's own event argument.
    m_btnLastSave.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDeathDisplay::OnLastSavePressed));
    m_btnLoadGame.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDeathDisplay::OnLoadGamePressed));
    m_btnQuit.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiDeathDisplay::OnQuitPressed));

    // The three buttons start grey, with the border taking the disabled colour and the text the
    // new-green default. Draw repaints both once the countdown has expired.
    m_btnLastSave.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_TEXT);
    m_btnLastSave.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_GREEN);
    m_btnLoadGame.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_TEXT);
    m_btnLoadGame.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_GREEN);
    m_btnQuit.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_TEXT);
    m_btnQuit.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_GREEN);

    m_fButtonLockTimer = 0.5f;

    // 'a' on LastSave, 'x' on LoadGame and 'b' on Quit (LoadGame's arm is the X face).
    AspyrAddButtonIcon(&m_btnLastSave, 'a');
    AspyrAddButtonIcon(&m_btnLoadGame, 'x');
    AspyrAddButtonIcon(&m_btnQuit, 'b');
}

void CSWGuiDeathDisplay::OnLastSavePressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    // The shared "a load is already in flight" latch, tested by all three handlers.
    if (m_fButtonLockTimer > 0.0f) return;

    g_pAppManager->m_pClientExoApp->SetGameOver(0);
    g_pAppManager->m_pClientExoApp->SetEndGamePending(0, 0.0f, 1);
    g_pAppManager->m_pClientExoApp->SaveOptions();

    CSWGuiSaveLoad* pSaveLoad = new CSWGuiSaveLoad(m_pGuiManager, 0, 0, 1);
    pSaveLoad->SetEndGameMode();

    if (pSaveLoad->m_entries.m_count > 0)
    {
        int bLoaded = pSaveLoad->LaunchMostRecentSave();
        delete pSaveLoad;

        CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
        if (bLoaded != 0)
        {
            pInGameGui->RemoveDeathMessage(0);
            return;
        }

        // The load failed, so re-arm the shared "are you sure?" box and push it instead. The
        // SetAllowCancel body, expanded here, sets the open sound to the alert, the cancel flag to
        // off and the two button/label visibility flags, then clears the dismissal callback.
        CSWGuiMessageBox* pBox = pInGameGui->m_pSoloModeQueryBox;
        pBox->m_nOpenSoundIndex = GUISOUND_ERROR_DEFAULT;
        pBox->m_bAllowCancel = 0;
        pBox->m_btnOk.m_tail.bVisible = pBox->m_bAllowDismissal;
        pBox->m_btnCancel.m_tail.bVisible = pBox->m_bAllowCancel && pBox->m_bAllowDismissal;
        pBox->m_lblIcon.m_tail.bVisible = pBox->m_bHasIcon;
        pBox->m_pfnCallback = 0;
        pBox->m_pCallbackObject = 0;
        pBox->m_pCallbackControl = 0;
        pBox->SetMessage(STRREF_NO_SAVED_GAMES);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    delete pSaveLoad;
}

void CSWGuiDeathDisplay::OnLoadGamePressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (m_fButtonLockTimer > 0.0f) return;

    g_pAppManager->m_pClientExoApp->SetGameOver(0);
    g_pAppManager->m_pClientExoApp->SetEndGamePending(0, 0.0f, 1);
    g_pAppManager->m_pClientExoApp->SaveOptions();

    CSWGuiSaveLoad* pSaveLoad = new CSWGuiSaveLoad(m_pGuiManager, 0, 0, 1);
    pSaveLoad->SetEndGameMode();

    // This arm does not launch the most recent save; it drops the death panel and pushes the
    // picker instead.
    if (pSaveLoad->m_entries.m_count > 0)
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->RemoveDeathMessage(1);
        m_pGuiManager->AddPanel(pSaveLoad, 3, 1);
        return;
    }

    delete pSaveLoad;
}

void CSWGuiDeathDisplay::OnQuitPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (m_fButtonLockTimer > 0.0f) return;

    // The end-game flag is set before the game-over flag here, the opposite order from the two
    // save handlers.
    g_pAppManager->m_pClientExoApp->SetEndGamePending(1, 0.0f, 1);
    g_pAppManager->m_pClientExoApp->SetGameOver(0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->RemoveDeathMessage(1);
}

void CSWGuiDeathDisplay::OnPanelAdded()
{
    g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GUI, 0);

    // Centred by halving each side rather than subtracting first.
    CSWGuiExtent extent = m_extent;
    extent.x = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth / 2 - extent.w / 2;
    extent.y = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight / 2 - extent.h / 2;
    SetExtent(extent);

    g_pAppManager->m_pClientExoApp->ShowMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);
    g_pAppManager->m_pClientExoApp->SetGuiGameSlow(1);

    CSWGuiPanel::OnPanelAdded();
}

void CSWGuiDeathDisplay::OnPanelRemoved() {
    // The death screen going away is what un-slows the game clock again.
    g_pAppManager->m_pClientExoApp->SetGuiGameSlow(0);
}

void CSWGuiDeathDisplay::Draw(float f)
{
    m_fButtonLockTimer -= f;
    if (m_fButtonLockTimer < 0.0f)
    {
        // The countdown has expired, so the three buttons are re-armed. The text colour here is the
        // new-text default, while the ctor set the new-green one.
        m_btnLastSave.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        m_btnLastSave.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnLoadGame.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        m_btnLoadGame.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnQuit.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        m_btnQuit.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    }
    CSWGuiPanel::Draw(f);
}

void CSWGuiDeathDisplay::HandleInputEvent(int type, int val)
{
    bool bJoystick = IsJoystickConnected();
    if (val == 0) return;
    if (!bJoystick) return;

    switch (type)
    {
    case INPUTEVENT_GUI_A:
    case INPUTEVENT_GUI_START: OnLastSavePressed(0); break;
    case INPUTEVENT_GUI_X: OnLoadGamePressed(0); break;
    case INPUTEVENT_GUI_B: OnQuitPressed(0); break;
    }
}

#endif // !KOTOR1

// AspyrLocalizedStrings, the pimpl wrapper over AspyrLocalizedStringsImpl, plus the Impl's own
// bodies. The Impl's definition lives here rather than in the header so <string>/<vector> stay out
// of the TUs that include the header; the wrapper only holds a pointer.







class AspyrLocalizedStringsImpl
{
public:
    AspyrLocalizedStringsImpl(const char* pFileName, bool bDecodeUtf8);
    const char* GetString(int nIndex, int nStringsPerLanguage);

private:
    std::vector<std::string> m_aStrings;
};

const char* GetLocalizationFolderName();

// One flat run of strings per language inside the override file: the stride arrives as a
// parameter and the language index is recovered from the folder name. nStringsPerLanguage > 0 is
// what makes index 0 readable when no stride is given. A missing row falls back to the shared empty
// string.
const char* AspyrLocalizedStringsImpl::GetString(int nIndex, int nStringsPerLanguage)
{
    const char* pszFolder = GetLocalizationFolderName();

    int nLanguage;
    if (strstr(pszFolder, "rench") != 0)
        nLanguage = 1;
    else if (strstr(pszFolder, "talian") != 0)
        nLanguage = 2;
    else if (strstr(pszFolder, "erman") != 0)
        nLanguage = 3;
    else
        nLanguage = (strstr(pszFolder, "panish") != 0) << 2;

    int nBase = 0;
    if (nStringsPerLanguage > 0) nBase = nLanguage * nStringsPerLanguage;

    uint64_t nWhich = static_cast<uint64_t>(nBase + nIndex);
    if (nWhich >= m_aStrings.size()) return "";
    return m_aStrings[nWhich].c_str();
}

// std::fstream rather than std::ifstream; the file is opened read-only.
AspyrLocalizedStringsImpl::AspyrLocalizedStringsImpl(const char* pFileName, bool bDecodeUtf8)
{
    std::fstream file;
    file.open(pFileName, std::ios_base::in);
    if (!file.is_open()) return;

    std::string sLine;
    while (std::getline(file, sLine))
    {
        // The BOM strip runs in both arms, advancing the pointer past the three bytes of a UTF-8
        // BOM when they are present.
        const char* p = sLine.c_str();
        if (*p == static_cast<char>(UTF8_BOM_1)) p++;
        if (*p == static_cast<char>(UTF8_BOM_2)) p++;
        if (*p == static_cast<char>(UTF8_BOM_3)) p++;

        if (bDecodeUtf8)
        {
            // A hand-rolled 2-byte UTF-8 decoder. Any byte with the two top bits set starts a
            // two-byte sequence and the follower's low 6 bits are spliced in under it.
            char szDecoded[1024];
            char* pOut = szDecoded;
            while (*p != 0)
            {
                unsigned char c = static_cast<unsigned char>(*p);
                if (c >= UTF8_LEAD_BYTE_MIN)
                {
                    p++;
                    c = static_cast<unsigned char>((*p & UTF8_CONTINUATION_BITS) | (c << 6));
                }
                *pOut++ = static_cast<char>(c);
                p++;
            }
            *pOut = 0;
            int nLen = static_cast<int>(strlen(szDecoded));

            // A literal two-character `\n` in the override file means a real newline. The memmove
            // length is the string's length as an int.
            char* q = strstr(szDecoded, "\\n");
            while (q != 0)
            {
                memmove(q, q + 1, static_cast<int>(strlen(q)));  // idiom-exempt: runtime length (strlen-derived)
                *q = '\n';
                q = strstr(q, "\\n");
            }

            // The line's own trailing CR/LF, trimmed from the end only.
            nLen = static_cast<int>(strlen(szDecoded));
            while (nLen > 0)
            {
                if (szDecoded[nLen - 1] != '\r' && szDecoded[nLen - 1] != '\n') break;
                szDecoded[nLen - 1] = 0;
                nLen--;
            }

            m_aStrings.push_back(std::string(szDecoded));
        }
        else
        {
            m_aStrings.push_back(std::string(p));
        }
    }

    file.close();
}

// Frees the implementation, whose string vector is the class's only state.
AspyrLocalizedStrings::~AspyrLocalizedStrings()
{
    delete m_pImpl;
}

const char* AspyrLocalizedStrings::GetString(int nIndex, int nStringsPerLanguage)
{
    return m_pImpl->GetString(nIndex, nStringsPerLanguage);
}

AspyrLocalizedStrings::AspyrLocalizedStrings(const char* pFileName, bool bLoadNow)
{
    m_pImpl = new AspyrLocalizedStringsImpl(pFileName, bLoadNow);
}

// The tooltip box. Its layout lives in the class's own header.
void CSWGuiToolTipPanel::SetToolTipText(const CExoString& text)
{
    m_label.m_text.m_params.SetText(text);
}

CSWGuiToolTipPanel::CSWGuiToolTipPanel(CSWGuiManager* pGuiManager) : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: the layout is picked by screen height, then rescaled.
    if (g_nScreenHeight >= 1200)
        StartLoadFromLayout(CResRef("tooltip16X12"));
    else if (g_nScreenHeight >= 1024)
        StartLoadFromLayout(CResRef("tooltip12x10"));
    else if (g_nScreenHeight >= 960)
        StartLoadFromLayout(CResRef("tooltip12X9"));
    else if (g_nScreenHeight >= 768)
        StartLoadFromLayout(CResRef("tooltip10X8"));
    else
        StartLoadFromLayout(CResRef("tooltip8X6"));
    m_stateFlags |= 1;
    InitControl(m_label, CExoString("tooltip"), 1);
    StopLoadFromLayout();
    ScaleExtentForResolution();
#else
    StartLoadFromLayout(CResRef("tooltip8X6_p"), 1, 1, 1);
    InitControl(m_label, CExoString("tooltip"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#endif
}

// The text to measure is reached through the label's own CSWGuiTextParams back-pointer.
//
// Both axes resolve the same way. The box is dropped below and right of the cursor and flipped back
// inside the screen when cursor + 15 + ideal + 8 would overflow. The two clamps read the raw ideal
// width and height rather than the padded accept size, so the test and the clamp disagree by two
// pixels.
void CSWGuiToolTipPanel::PositionToolTip()
{
    CSWGuiExtent extent;

    CSWGuiText* pText = m_label.m_text.m_params.m_pTextObject;
    if (pText)
    {
#if KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
        // The panel is the box's clip rect and the wrap width is in screen pixels, so both follow
        // the screen as the font does (the screen height over 768 lines).
        m_extent = CSWGuiExtent(0, 0, g_nScreenWidth, g_nScreenHeight);
        pText->m_nWrapWidth = static_cast<int>(400.0f * static_cast<float>(g_SDL_mode.h) / 768.0f);
#endif
        CSWGuiExtent extentIdeal = pText->GetIdealWidthAndHeight();
#ifdef K2_FIX_ENGINE_BUGS
        // The box's padding, its offset from the pointer and its gap from the screen's edge grow
        // with the text (and the pointer, which the desktop scales with the screen too).
#if KOTOR1
        float fUnit = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
        float fUnit = GetResolutionTextScalar();
#endif
        if (fUnit < 1.0f)
            fUnit = 1.0f;
        int nPad = static_cast<int>(8.0f * fUnit);
        int nOffset = static_cast<int>(15.0f * fUnit);
        int nEdge = static_cast<int>(10.0f * fUnit);
#else
        const int nPad = 8, nOffset = 15, nEdge = 10;
#endif
        extent.w = extentIdeal.w + nPad;
        extent.h = extentIdeal.h + nPad;

        int nX = g_pGuiMan->m_currentMousePos.x + nOffset;
        int nY = g_pGuiMan->m_currentMousePos.y + nOffset;
        extent.x = (nX + extent.w < g_nScreenWidth) ? nX : g_nScreenWidth - extentIdeal.w - nEdge;
        extent.y = (nY + extent.h < g_nScreenHeight) ? nY : g_nScreenHeight - extentIdeal.h - nEdge;

        m_label.SetExtent(extent);
    }
}
