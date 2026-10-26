#include "k2stdint.h"
#include "port/sdlapi.h"
#define CSWGUIPAZAAKCARD_CTOR_DECLARED_ONLY 1
#define CSWGUITUTORIAL_CTOR_DECLARED_ONLY 1
#include "swguipazaak.h"
#if KOTOR1  // K1: FlashArrow's one-element growth inlines Allocate
#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE
#include "CExoArrayListImpl.h"
#endif

#include "guiingame.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "swguiingame.h"
#include "swguidrawable.h"
#include "swpazaak.h"
#include "exostring.h"
#include "exoresman.h"
#include "appmanager.h"
#include "Achievements.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwtlktable.h"
#include "exosound.h"
#include "partytables.h"
#include "swparty.h"
#include "nwccreature.h"
#include "exolocstringinternal.h"
#include "soundprioritygroup.h"
#ifdef K2_UI_XBOX
#include <new>
#endif
extern CExoSound* g_pExoSound;
#if KOTOR1
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif
#include "port/libcbase.h"
#include "port/libcmath.h"
#include "port/k2qolapi.h"
#include "exoinput.h"
#include "strref.h"

#if KOTOR1 && !defined(K2_UI_TOUCH)
// The side panels beside the 4:3 table are the phone data's art (and_paz*); a PC install has none,
// and an image with no texture draws as a white sheet, so it is tinted black there.
static Vector K1PazaakSideColor(const char* pszName)
{
    CResRef res(pszName);
    bool bExists = g_pExoResMan->Exists(res, 3, static_cast<unsigned int*>(0)) ||
                   g_pExoResMan->Exists(res, 3007, static_cast<unsigned int*>(0));
    return bExists ? Vector(1.0f, 1.0f, 1.0f) : Vector(0.0f, 0.0f, 0.0f);
}
#endif

// The "this tutorial page was shown" bits of m_anTutorialState[0].
enum PazaakTutorialShown {
    TUTORIAL_SHOWN_FLIP = 0x01,
    TUTORIAL_SHOWN_FIRST_DRAW = 0x02,
    TUTORIAL_SHOWN_OPPONENT_RULES = 0x04,
    TUTORIAL_SHOWN_INTRO = 0x08,
    TUTORIAL_SHOWN_EXACTLY_20 = 0x10,
    TUTORIAL_SHOWN_CHANGE = 0x20,
};

enum { PAZAAK_WINNING_TOTAL = 20 };

int g_bPazaakShowNPCSideDeck;

// Formatter shared by both Pazaak panels' layout binds.

static void FormatInto(char* pBuf, uint64_t nLen, const char* pszFmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, pszFmt);
    // The va_list is passed by value: `&args` is not an equivalent spelling and aborts on Darwin,
    // where va_list is a plain pointer rather than a by-reference array object.
    __vsprintf_chk(pBuf, 0, nLen, pszFmt, args);
    __builtin_va_end(args);
}

// Control-name templates, of which only the digit slot is rewritten in place; the trailing spaces
// are layout-table padding.
namespace {
const char s_achBtnPlr[16]    = "BTN_PLRx       ";
const char s_achLblPlr[16]    = "LBL_PLRx       ";
const char s_achBtnNpc[16]    = "BTN_NPCx       ";
const char s_achLblNpc[16]    = "LBL_NPCx       ";
const char s_achBtnPlrSD[16]  = "BTN_PLRSIDEx   ";
const char s_achLblPlrSD[16]  = "LBL_PLRSIDEx   ";
const char s_achBtnNpcSD[16]  = "BTN_NPCSIDEx   ";
const char s_achLblNpcSD[16]  = "LBL_NPCSIDEx   ";
const char s_achBtnFlip[16]   = "BTN_FLIPx      ";
#if !KOTOR1 && !defined(K2_UI_XBOX)
const char s_achBtnChange[16] = "BTN_CHANGEx    ";
#endif
}  // namespace

CSWGuiPazaakGame::CSWGuiPazaakGame(CSWGuiManager* pManager, CSWPazaak* pPazaak,
                                   int bTutorialsEnabled, unsigned long long oidOpponent)
#if KOTOR1
    : CSWGuiPanel(pManager), m_dialog(pManager), m_tutorial(pManager) {
    m_apszCueSounds[0] = const_cast<char*>("mgs_startturn");
    m_apszCueSounds[1] = const_cast<char*>("mgs_drawmain");
    m_apszCueSounds[2] = const_cast<char*>("mgs_playside");
    m_apszCueSounds[3] = const_cast<char*>("mgs_warnbust");
    m_pszPendingSound = 0;
    m_pPazaak = pPazaak;
    m_nGameState = GAME_STATE_START_SET;
    m_fSequenceDelay = -1.0f;
    m_anTutorialState[0] = 0;
    m_anTutorialState[1] = -1;
    m_nHandPickMode = 0;
#else
    : CSWGuiPanel(pManager), m_dialog(pManager), m_tutorial(pManager), m_pPazaak(pPazaak) {
    m_pszPendingSound = 0;
#endif
    m_tutorial.SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleTutorialClose), 0);
#if !KOTOR1
    m_tutorial.m_pPazaakGame = this;
#endif
    m_tutorial.m_bEnabled    = bTutorialsEnabled;
    m_dialog.SetAllowCancel(0);

#ifdef K2_UI_XBOX
    // In the console layout, pazaakgame_x is authored at 640x480 while g_nGUIWidth/g_nGUIHeight are
    // pinned at 800x600 for the mobile layouts, and the load and every bind scale off those
    // globals, so the work runs inside a sized window. The console table has no End Turn, Stand or
    // flip/change buttons: its legend is five captions plus face-button badges.
    const int nSavedGUIWidth  = g_nGUIWidth;
    const int nSavedGUIHeight = g_nGUIHeight;
    g_nGUIWidth  = 640;
    g_nGUIHeight = 480;
    {
        CResRef res("pazaakgame_x");
        CSWGuiPanel::StartLoadFromLayout(res, 1, 1, 1);
    }
#else
    // Load the table layout; the binds below use its control tags.
    {
#if KOTOR1
        CResRef res("pazaakgame");
#else
        CResRef res("pazaakgame_p");
#endif
        CSWGuiPanel::StartLoadFromLayout(res, 1, 1, 1);
    }
#endif

    {
        CExoString s("LBL_PLRNAME");
        CSWGuiPanel::InitControl(m_lbl0, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_NPCNAME");
        CSWGuiPanel::InitControl(m_lbl1, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_PLRTOTAL");
        CSWGuiPanel::InitControl(m_lbl2, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_NPCTOTAL");
        CSWGuiPanel::InitControl(m_lbl3, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_PLRSIDEDECK");
        CSWGuiPanel::InitControl(m_lbl4, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_NPCSIDEDECK");
        CSWGuiPanel::InitControl(m_lbl5, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_PLRTURN");
        CSWGuiPanel::InitControl(m_lbl6, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_NPCTURN");
        CSWGuiPanel::InitControl(m_lbl7, s, 1, 1, 0, 0, 0);
    }

#if KOTOR1  // K1: the two turn indicators start hidden
    m_lbl6.m_tail.bVisible = 0;
    m_lbl7.m_tail.bVisible = 0;
#else
    // The turn-indicator card art goes into both turn labels' fill image.
    {
        CResRef res("PZ_PLAYERLITEOFF");
        m_lbl6.m_border.m_params.SetFillImage(res, 0);
    }
    {
        CResRef res("PZ_PLAYERLITEOFF");
        m_lbl7.m_border.m_params.SetFillImage(res, 0);
    }
#endif
    {
        CExoString s("LBL_PLRSCORE0");
        CSWGuiPanel::InitControl(m_aPlrScore[0], s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_PLRSCORE1");
        CSWGuiPanel::InitControl(m_aPlrScore[1], s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_PLRSCORE2");
        CSWGuiPanel::InitControl(m_aPlrScore[2], s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_NPCSCORE0");
        CSWGuiPanel::InitControl(m_aNpcScore[0], s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_NPCSCORE1");
        CSWGuiPanel::InitControl(m_aNpcScore[1], s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_NPCSCORE2");
        CSWGuiPanel::InitControl(m_aNpcScore[2], s, 1, 1, 0, 0, 0);
    }
#ifndef K2_UI_XBOX  // pazaakgame_x has none of these: its legend is bound after the card loops
    {
        CExoString s("LBL_FLIPICON");
        CSWGuiPanel::InitControl(m_blbl0, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_FLIPLEGEND");
        CSWGuiPanel::InitControl(m_blbl1, s, 1, 1, 0, 0, 0);
    }
#if !KOTOR1
    {
        CExoString s("LBL_CHANGEICON");
        CSWGuiPanel::InitControl(m_blbl2, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_CHANGELEGEND");
        CSWGuiPanel::InitControl(m_blbl3, s, 1, 1, 0, 0, 0);
    }
    {
        // The forfeit button is the ninth card button.
        CExoString s("BTN_FORFEITGAME");
        CSWGuiPanel::InitControl(m_btnForfeitGame, s, 1, 1, 0, 0, 0);
    }
#endif
#endif

    // The player-hand card slots each rebind the prebuilt card widget to its layout button and
    // label, then gate it.
    {
        // Patch the slot digit into the name templates.
        char achBtnPlr[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achBtnPlr, s_achBtnPlr, 16);
        char achLblPlr[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achLblPlr, s_achLblPlr, 16);
        char achBtnNpc[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achBtnNpc, s_achBtnNpc, 16);
        char achLblNpc[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achLblNpc, s_achLblNpc, 16);
#pragma clang loop unroll(disable)
        for (int n = 0; n < 9; n++) {
            FormatInto(achBtnPlr + 7, 9, "%d", n);
            FormatInto(achLblPlr + 7, 9, "%d", n);
            FormatInto(achBtnNpc + 7, 9, "%d", n);
            FormatInto(achLblNpc + 7, 9, "%d", n);
            CSWGuiPanel::InitControl(m_aPlrBoard[n], achBtnPlr, 1, 1, 0, 0, 0);
            CSWGuiPanel::InitControl(m_aPlrBoard[n].m_label, achLblPlr, 1, 1, 0, 0, 0);
            CSWGuiPanel::InitControl(m_aNpcBoard[n], achBtnNpc, 1, 1, 0, 0, 0);
            CSWGuiPanel::InitControl(m_aNpcBoard[n].m_label, achLblNpc, 1, 1, 0, 0, 0);
            m_aNpcBoard[n].m_tail.bIgnoreMouse = 1;
            m_aPlrBoard[n].SetEnabled(0);
            m_aPlrBoard[n].m_bDragLatch = 0;
        }
    }
    // For the side-deck slots and the flip/change button pairs, four iterations cover cards 9..12
    // of the player's side deck and 22..25 of the NPC's (nine hand plus four side per side), plus
    // the four flip and four change controls. The hand-card events bind to the player's side-deck
    // cards, not to any action button.
    {
        char achBtnPlrSD[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achBtnPlrSD, s_achBtnPlrSD, 16);
        char achLblPlrSD[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achLblPlrSD, s_achLblPlrSD, 16);
        char achBtnNpcSD[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achBtnNpcSD, s_achBtnNpcSD, 16);
        char achLblNpcSD[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achLblNpcSD, s_achLblNpcSD, 16);
#ifndef K2_UI_XBOX  // pazaakgame_x has no BTN_FLIPx / BTN_CHANGEx buttons
        char achBtnFlip[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achBtnFlip, s_achBtnFlip, 16);
#if !KOTOR1
        char achBtnChange[16];
        __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achBtnChange, s_achBtnChange, 16);
#endif
#endif
#pragma clang loop unroll(disable)
        for (int n = 0; n < 4; n++) {
            FormatInto(achBtnPlrSD + 11, 5, "%d", n);
            FormatInto(achBtnNpcSD + 11, 5, "%d", n);
            FormatInto(achLblPlrSD + 11, 5, "%d", n);
            FormatInto(achLblNpcSD + 11, 5, "%d", n);
#ifndef K2_UI_XBOX
            FormatInto(achBtnFlip + 8, 8, "%d", n);
#if !KOTOR1
            FormatInto(achBtnChange + 10, 6, "%d", n);
#endif
#endif
            CSWGuiPanel::InitControl(m_aPlrHand[n], achBtnPlrSD, 1, 1, 0, 0, 0);
            CSWGuiPanel::InitControl(m_aPlrHand[n].m_label, achLblPlrSD, 1, 1, 0, 0, 0);
#ifndef K2_UI_XBOX
            CSWGuiPanel::InitControl(m_aCardButtonsA[n], achBtnFlip, 1, 1, 0, 0, 0);
#if KOTOR1  // K1: on a phone the flip buttons double in size about their centre
            {
                int nScreenW, nScreenH;
                if (IosIsIphone(nScreenW, nScreenH)) {
                    CSWGuiExtent extent = m_aCardButtonsA[n].m_extent;
                    int nSize = extent.w;
                    extent.x -= nSize / 2;
                    extent.y -= nSize / 2;
                    extent.w = nSize * 2;
                    extent.h = nSize * 2;
                    m_aCardButtonsA[n].SetExtent(extent);
                    if (n == 1)
                        m_aPlrHand[1].m_border.m_params.m_flags.nFillStyle = 2;
                }
            }
#else
            CSWGuiPanel::InitControl(m_aCardButtonsB[n], achBtnChange, 1, 1, 0, 0, 0);
#endif
#endif
            m_aPlrHand[n].m_tail.nControlId       = n;
#ifndef K2_UI_XBOX
            m_aCardButtonsA[n].m_tail.nControlId     = n;
#if !KOTOR1
            m_aCardButtonsB[n].m_tail.nControlId = n;
#endif
#endif
            CSWGuiPanel::InitControl(m_aNpcHand[n], achBtnNpcSD, 1, 1, 0, 0, 0);
            CSWGuiPanel::InitControl(m_aNpcHand[n].m_label, achLblNpcSD, 1, 1, 0, 0, 0);
            // On the console table the side cards take no click event: A ends the turn there, and
            // the base panel would otherwise route A to the focused side card and play it. The
            // drag, double-click and right-click events stay, so touch can still drag or
            // double-tap a card onto the table.
#ifndef K2_UI_XBOX
            m_aPlrHand[n].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleHandcardAction));
#endif
            m_aPlrHand[n].AddEvent(SWGUIEVENT_CARD_DROPPED, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleHandCardDrop));
            m_aPlrHand[n].AddEvent(SWGUIEVENT_DOUBLE_CLICK, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleHandCardDoubleClick));
            m_aPlrHand[n].AddEvent(INPUTEVENT_MOUSE_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleHandCardRightClick));
#if !KOTOR1  // K1 leaves bit 2 of the hand cards, flip buttons and action buttons set
            m_aPlrHand[n].m_tail.bFocusOnHover = 0;
#endif
            m_aNpcHand[n].m_tail.bIgnoreMouse = 1;
#ifndef K2_UI_XBOX
            m_aCardButtonsA[n].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleFlipButtons));
#if !KOTOR1
            m_aCardButtonsB[n].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleChangeButtons));
            m_aCardButtonsA[n].m_tail.bFocusOnHover     = 0;
            m_aCardButtonsB[n].m_tail.bFocusOnHover = 0;
#endif
#endif
        }
    }

#ifdef K2_UI_XBOX
    // The console legend is ten controls where the PC table binds the flip/change buttons,
    // BTN_XTEXT/BTN_YTEXT and BTN_FORFEITGAME. The layout tags do not name the button they show, so
    // the caption text and badge fill carry the mapping. The four captions the PC table already has
    // buttons for keep their PC roles in m_aActionButtons; the fifth caption and the five badges
    // reuse members the console table leaves idle (the four flip/change icon labels and
    // m_aCardButtons[7]/[8]).
    {
        CExoString s("LBL_XBUTTON");
        CSWGuiPanel::InitControl(m_blbl0, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_XTEXT");
        CSWGuiPanel::InitControl(m_actbtn0, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_YBUTTON");
        CSWGuiPanel::InitControl(m_blbl1, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_YTEXT");
        CSWGuiPanel::InitControl(m_actbtn1, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_ABUTTON");
        CSWGuiPanel::InitControl(m_blbl2, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_ATEXT");
        CSWGuiPanel::InitControl(m_actbtn2, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_BBUTTON");
        CSWGuiPanel::InitControl(m_blbl3, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_BTEXT");
        CSWGuiPanel::InitControl(m_actbtn3, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_WBUTTON");
        CSWGuiPanel::InitControl(m_btnForfeitGame, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("LBL_WTEXT");
        CSWGuiPanel::InitControl(m_aCardButtonsB[3], s, 1, 1, 0, 0, 0);
    }
#else
    {
        CExoString s("BTN_XTEXT");
        CSWGuiPanel::InitControl(m_actbtn0, s, 1, 1, 0, 0, 0);
    }
    {
        CExoString s("BTN_YTEXT");
        CSWGuiPanel::InitControl(m_actbtn1, s, 1, 1, 0, 0, 0);
    }
#endif
    CSWGuiPanel::StopLoadFromLayout();
#if KOTOR1  // K1: rescale to the window, then hang the two side panels beside the 4:3 table
    {
#ifdef K2_UI_TOUCH
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
        float s_fScale = static_cast<float>(g_SDL_mode.h) / 720.0f;
#else
        static float s_fScale = static_cast<float>(g_SDL_mode.h) / 720.0f;
#endif
#else
        // pc / xbox: the 640x480 layout fills the height, between the side panels
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
        float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#else
        static float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#endif
#endif
        ScaleExtentForResolution(s_fScale);
        CSWGuiExtent extent;
        extent.w = static_cast<int>(ceilf(static_cast<float>(g_SDL_mode.h) * 204.0f / 512.0f));
        extent.h = g_SDL_mode.h;
        extent.x = static_cast<int>((static_cast<float>(g_SDL_mode.w) - ceilf(static_cast<float>(g_SDL_mode.h) * 4.0f / 3.0f)) * 0.5f -
                         static_cast<float>(extent.w));
        extent.y = 0;
        m_imgSideLeft.Initialize(extent, CSWGuiImageParams(CResRef("and_paz_left"), 0, 0, 0.0f, SWGUIALIGN_MIDDLE | SWGUIALIGN_CENTER,
#ifdef K2_UI_TOUCH
                                                           1.0f, Vector(1.0f, 1.0f, 1.0f)));
#else
                                                           1.0f, K1PazaakSideColor("and_paz_left")));
#endif
        extent.x = g_SDL_mode.w - extent.x - extent.w;
        m_imgSideRight.Initialize(extent, CSWGuiImageParams(CResRef("and_paz_right"), 0, 0, 0.0f, SWGUIALIGN_MIDDLE | SWGUIALIGN_CENTER,
#ifdef K2_UI_TOUCH
                                                            1.0f, Vector(1.0f, 1.0f, 1.0f)));
#else
                                                            1.0f, K1PazaakSideColor("and_paz_right")));
#endif
    }
#endif
#ifdef K2_UI_XBOX
    g_nGUIWidth  = nSavedGUIWidth;
    g_nGUIHeight = nSavedGUIHeight;

    // The White/Black buttons do not exist on a modern pad: Flip and Change are driven by the
    // bumper events, so the badges show the left and right trigger glyphs.
    {
        CResRef res("uibut_LTrig");
        m_blbl3.m_border.m_params.SetFillImage(res, 0);
    }
    {
        CResRef res("uibut_RTrig");
        m_btnForfeitGame.m_border.m_params.SetFillImage(res, 0);
    }

    // Every caption is also a tap target. End Turn and Stand keep the PC handlers; Play, Flip and
    // Change go to HandleXButton, which dispatches on the caption tapped and acts on the side card
    // Draw last saw focused (recorded in m_aActionButtons[2].m_tail.nControlId).
#endif

    m_actbtn0.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleContinue));
    m_actbtn1.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleStand));
    m_actbtn2.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleXButton));
#ifdef K2_UI_XBOX
    m_actbtn3.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleXButton));
    m_aCardButtonsB[3].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleXButton));
#else
    m_actbtn3.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleBlackButton));
#if !KOTOR1
    m_btnForfeitGame.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleForfeitButton));
#endif
#endif
#if !KOTOR1
    m_actbtn0.m_tail.bFocusOnHover = 0;
    m_actbtn1.m_tail.bFocusOnHover = 0;
    m_actbtn2.m_tail.bFocusOnHover = 0;
    m_actbtn3.m_tail.bFocusOnHover = 0;
#ifdef K2_UI_XBOX
    m_aCardButtonsB[3].m_tail.bFocusOnHover   = 0;
    m_actbtn2.m_tail.nControlId = -1;
#else
    m_btnForfeitGame.m_tail.bFocusOnHover   = 0;
#endif
#endif

    // Fill the opponent's name plate, gender-parsing the name through the TLK singleton. The
    // player's own plate is filled by OnPanelAdded.
    CSWSCreature* pOpponent =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidOpponent);
    if (pOpponent) {
        CExoString strName = pOpponent->m_pStats->GetFullName();
#if !KOTOR1  // K1: the name is shown as is, with no gender-token parse
        CSWTlkTable* pTlk  = g_pSWTlkTable;
        pTlk->m_pSpeakerObj = pOpponent;
        unsigned char nGender = pTlk->GetObjectGender();
        pTlk->m_gender = nGender;
        pTlk->m_bSuppressEnabled |= 1;
        pTlk->ParseStr(strName);
#endif
        m_lbl1.m_text.m_params.SetText(strName);
    }
#if KOTOR1 && defined(K1_UI_XBOX)
    // xbox: X ends the turn and Y stands (HandleInputEvent); A stays on the focused control.
    IosAddButtonIcon(&m_actbtn0, 'x');
    IosAddButtonIcon(&m_actbtn1, 'y');
#endif
    // When a caption cannot fit its button's extent height, center it instead of top-aligning.
#ifndef K2_UI_XBOX  // the console captions keep pazaakgame_x's own alignment
    CSWGuiText* pText = &m_actbtn0.m_text;
    if (pText->GetIdealHeight() > m_actbtn0.m_extent.h)
        m_actbtn0.m_text.m_params.SetAlignment(10);
    pText = &m_actbtn1.m_text;
    if (pText->GetIdealHeight() > m_actbtn1.m_extent.h)
        m_actbtn1.m_text.m_params.SetAlignment(10);
#endif
    m_nFullScreenBGIndex = 2;
#if !KOTOR1
    m_bPlayerStarts         = 1;
#ifndef K2_UI_XBOX  // console: m_aCardButtons[8] is the RB badge, and B is Forfeit
    AspyrAddButtonIcon(&m_btnForfeitGame, 'b');
#endif
#endif
}

CSWGuiPazaakGame::~CSWGuiPazaakGame() {
#if KOTOR1  // K1: the clear sits inside the null guard
    if (m_pPazaak) {
        delete m_pPazaak;
        m_pPazaak = 0;
    }
#else
    delete m_pPazaak;
    m_pPazaak = 0;
#endif
}

void CSWGuiPazaakGame::HandleHandCardDoubleClick(CSWGuiControl* pControl) {
    if (pControl && m_nGameState == GAME_STATE_PLAYER_PLAY)
        HandlePlayHandCard(pControl->m_tail.nControlId);
}

void CSWGuiPazaakGame::HandleXButton(CSWGuiControl* pControl) {
#ifdef K2_UI_XBOX
    // Console: the PC body entered the "pick a hand card" mode of the flip/change button row;
    // the console table has no such row, so this is the touch entry for the legend captions that
    // act on a hand card, and it acts on the side card Draw last saw focused.
    if (!pControl)
        return;
    const int n = m_actbtn2.m_tail.nControlId;
    if (n < 0 || n > 3)
        return;
    CSWGuiControl* pSaved = m_pActiveControl;
    m_pActiveControl = &m_aPlrHand[n];
    if (pControl == &m_actbtn2)
        HandlePlayHandCard(n);
    else if (pControl == &m_actbtn3)
        HandleFlipHandCard();
    else if (pControl == &m_aCardButtonsB[3])
        HandleChangeHandCard();
    if (m_pActiveControl == &m_aPlrHand[n])
        m_pActiveControl = pSaved;
#else
    if (!pControl || pControl->m_tail.nCurrentEventArg) {
        m_nHandPickMode = 1;
        SetHandEnabled(1);
    }
#endif
}

void CSWGuiPazaakGame::HandleBlackButton(CSWGuiControl* pControl) {
    if (!pControl || pControl->m_tail.nCurrentEventArg) {
        m_nHandPickMode = 2;
        SetHandEnabled(1);
    }
}

void CSWGuiPazaakGame::HandleHandcardAction(CSWGuiControl* pControl) {
    HandlePlayHandCard(pControl->m_tail.nControlId);
    m_nHandPickMode = 0;
    SetHandEnabled(0);
}

#if !KOTOR1
void CSWGuiPazaakGame::HandleTutorialStand() {
    if (!m_pPazaak->m_players[0].m_bStanding &&
        (m_nGameState == GAME_STATE_PLAYER_PLAY || m_nGameState == GAME_STATE_PLAYER_PLAYED_CARD)) {
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        m_pPazaak->m_players[0].m_bStanding = 1;
        m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
        m_fSequenceDelay     = 0.4f;
        RefreshDisplay();
    }
}
#endif

// Local copy of CSWGuiTutorial::PlaceAbove; the explicit tutorial parameter keeps SetExtent a
// virtual dispatch.
static void PlaceBubbleAbove(CSWGuiTutorial* pTutorial, CSWGuiControl* pControl) {
    pTutorial->m_pAnchorControl = pControl;
    CSWGuiExtent extent = pControl->m_extent;
    int nX = extent.x;
    int nY = extent.y;
    if (pControl->m_pContainer) {
        CSWGuiPanel* pPanel = pControl->m_pContainer->AsSWGuiPanel();
        if (pPanel)
            pPanel->GetExtentAccountingForPanelOffset(&extent);
        nX = extent.x + nX;
        nY = extent.y + nY;
    }
    extent = pTutorial->m_extent;
    extent.x = nX;
    extent.y = nY - extent.h;
    pTutorial->SetExtent(extent);
}

// The tutorial bubble's OK callback. The action depends on the page the bubble was showing when
// it closed: the intro pages chain into the next explain-a-rule page, the stand and side-deck pages
// perform that action, and the end-of-game pages walk the dialog chain and tear the match down.
// With m_bAccepted clear only the paired end-of-game strrefs latch GAME_STATE_END_MATCH.
void CSWGuiPazaakGame::HandleTutorialClose(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    uint64_t nHelp = 0;
    if (m_tutorial.m_nCurrentHelp < m_tutorial.m_helpText.m_count)
        nHelp = m_tutorial.m_helpText.m_pData[m_tutorial.m_nCurrentHelp];
    if (m_tutorial.m_bAccepted) {
        switch (nHelp) {
        case STRREF_TUT_FIRST_DRAW: {
            const uint64_t anEndRefs[5] = {STRREF_TUT_HAND, STRREF_TUT_PLAY_HAND_CARD, STRREF_TUT_NEGATIVE_CARD_TIP, STRREF_TUT_FLIP_CARDS, STRREF_TUT_DECLARE_SIGN};
            m_tutorial.ShowMultipleHelp(anEndRefs, 5);
            PlaceBubbleAbove(&m_tutorial, &m_aPlrHand[0]);
            break;
        }
        case STRREF_TUT_NEGATIVE_CARD_TIP:
        case STRREF_TUT_DECLARE_SIGN: {
            const uint64_t anDeckRefs[3] = {STRREF_TUT_TURN_OVER, STRREF_TUT_STAND, STRREF_TUT_CHOOSE_ACTION};
            m_tutorial.ShowMultipleHelp(anDeckRefs, 3);
            PlaceBubbleAbove(&m_tutorial, &m_actbtn0);
            break;
        }
        case STRREF_TUT_STAND_CONFIRM:
            m_pPazaak->m_players[0].m_bStanding = 1;
            m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
            m_fSequenceDelay     = 0.4f;
            RefreshDisplay();
            break;
        case STRREF_TUT_END_TURN:
            m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
            m_fSequenceDelay     = 0.4f;
            RefreshDisplay();
            break;
        case STRREF_TUT_ONE_HAND_CARD:
            m_pPazaak->UsePlayerSidedeckCard(m_anTutorialState[1]);
            m_anTutorialState[1] = -1;
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[2]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            m_nGameState = GAME_STATE_PLAYER_PLAYED_CARD;
            RefreshDisplay();
            ReconnectSidedeckButtons();
            break;
        case STRREF_TUT_MATCH_OVER: {
#if KOTOR1
            // K1: no panel chain, just state 4.
            SetToBeDeleted(0);
            g_pAppManager->m_pClientExoApp->EndPazaakGame(
                m_pPazaak->m_players[0].m_nRoundsWon > m_pPazaak->m_players[1].m_nRoundsWon, m_pPazaak->m_nWager, 0,
                m_pPazaak->m_oidOpponent);
#else
            SetToBeDeleted(0);
            int bPlayerAhead = m_pPazaak->m_players[0].m_nRoundsWon > m_pPazaak->m_players[1].m_nRoundsWon;
            g_pAppManager->m_pClientExoApp->EndPazaakGame(bPlayerAhead, m_pPazaak->m_nWager, 0,
                                                         m_pPazaak->m_oidOpponent);
#endif
            break;
        }
        }
    } else if ((nHelp & ~1UL) == STRREF_TUT_MATCH_RULES) {
        m_nGameState = GAME_STATE_END_MATCH;
    }
}

void CSWGuiPazaakGame::HandleEndDialog(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    if (m_pPazaak->m_players[0].m_nRoundsWon > 2 || m_pPazaak->m_players[1].m_nRoundsWon >= 3) {
        m_tutorial.ShowHelp(STRREF_TUT_MATCH_OVER, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
        m_nGameState = GAME_STATE_END_MATCH;
    } else {
        m_tutorial.ShowHelp(STRREF_TUT_MATCH_RULES, static_cast<unsigned>(-1), STRREF_QUIT_TUTORIAL);
        m_nGameState = GAME_STATE_NEXT_SET;
    }
}

void CSWGuiPazaakGame::Draw(float f) {
#ifdef K2_UI_XBOX
    // Console: remember the last focused side card for the touch captions (HandleXButton).
    for (int n = 0; n < 4; n++)
        if (m_pActiveControl == &m_aPlrHand[n])
            m_actbtn2.m_tail.nControlId = n;
#endif
#if KOTOR1  // K1: the side panels are drawn first, in a window-sized viewport
    AurGUIStartLayer();
    if (AurGUISetupViewport(0, 0, g_SDL_mode.w, g_SDL_mode.h, Vector(0.09f, 0.082f, 0.125f), false, 1.0f)) {
        m_imgSideLeft.Draw(f);
        m_imgSideRight.Draw(f);
        AurGUICloseViewport();
    }
    AurGUIStopLayer();
#endif
    if (m_fSequenceDelay >= 0.0f) {
        m_fSequenceDelay -= f;
        if (m_fSequenceDelay < 0.0f)
            RefreshDisplay();
    } else {
        while (m_pGuiManager->IsOnTop(this) && DoGameSequence()) {
        }
    }
    CSWGuiPanel::Draw(f);
}

void CSWGuiPazaakGame::HandleQuitDialog(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted) {
        SetToBeDeleted(0);
        g_pAppManager->m_pClientExoApp->EndPazaakGame(0, m_pPazaak->m_nWager, 0,
                                                      m_pPazaak->m_oidOpponent);
    }
}

#if !KOTOR1
void CSWGuiPazaakGame::HandleForfeitButton(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(1);
    pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleQuitDialog), 0);
    pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
    pBox->SetMessage(STRREF_CONFIRM_FORFEIT);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}
#endif

void CSWGuiPazaakGame::StartNewGame() {
    m_pPazaak->ClearGameBoard();
    m_nGameState = GAME_STATE_START_SET;
    RefreshDisplay();
    int nFlags = m_anTutorialState[0];
    m_anTutorialState[0] = nFlags & ~TUTORIAL_SHOWN_EXACTLY_20;
    if (!(nFlags & TUTORIAL_SHOWN_INTRO)) {
        m_anTutorialState[0] |= TUTORIAL_SHOWN_INTRO;
        const uint64_t anHelpStrRefs[] = {STRREF_TUT_OBJECT, STRREF_TUT_BUST};
        m_tutorial.ShowMultipleHelp(anHelpStrRefs, 2);
    }
    m_nHandPickMode = 0;
#if KOTOR1
    // K1: the hand starts disabled and focus goes to the End Turn button (xbox: to the hand,
    // SetHandEnabled).
    SetHandEnabled(0);
#ifndef K1_UI_XBOX
    SetActiveControl(&m_actbtn0, 0);
#endif
#else
    SetHandEnabled(1);
#endif
}

// Rebuilds the hand row's navigation links after a board change: the up/down links are cleared,
// the held cards are ringed left/right, the row ends link to the action buttons, and a live card
// takes focus when the focused slot was emptied while a dialog choice is pending.
void CSWGuiPazaakGame::ReconnectSidedeckButtons() {
    int nSel = -1;
    for (int i = 3; i >= 0; i--) {
        m_aPlrHand[i].SetMoveToControl(0, 0);
#ifdef K2_UI_XBOX
        // Console: no flip/change button row and no End Turn or Stand buttons to link to, so the
        // d-pad only walks the live side cards, left and right (the ring below).
        m_aPlrHand[i].SetMoveToControl(1, 0);
#endif
        m_aPlrHand[i].SetMoveToControl(2, 0);
#ifdef K2_UI_XBOX
        m_aPlrHand[i].SetMoveToControl(3, 0);
        m_aPlrHand[i].m_tail.bFocusOnHover = 1;
        m_aPlrHand[i].m_bEnabled        = 1;
#endif
        if (m_aPlrHand[i].m_card.m_nCardType >= 0)
            nSel = i;
    }
    int bFocus = 0;
    for (int i = 3; i >= 0; i--) {
        if (m_aPlrHand[i].m_card.m_nCardType >= 0) {
            if (nSel >= 0) {
                m_aPlrHand[i].SetMoveToControl(3, &m_aPlrHand[nSel]);
                m_aPlrHand[nSel].SetMoveToControl(1, &m_aPlrHand[i]);
            }
            nSel = i;
        } else {
            bFocus = m_pActiveControl == &m_aPlrHand[i] ? 1 : bFocus;
        }
    }
#ifdef K2_UI_XBOX
    // With no buttons a played, now empty, focused slot always hands focus to the nearest live
    // card.
    if (bFocus != 0 || m_pActiveControl == 0)
#elif defined(K1_UI_XBOX)
    if (bFocus != 0)   // xbox: the focus stays on the hand (SetHandEnabled)
#else
    if (bFocus != 0 && m_nHandPickMode != 0)
#endif
        SetActiveControl(nSel >= 0 ? &m_aPlrHand[nSel] : 0, 0);
#ifndef K2_UI_XBOX
#if KOTOR1  // K1: no left/right links between the hand cards
    for (int i = 0; i < 4; i++) {
        m_aPlrHand[i].m_tail.bFocusOnHover = 1;
        m_aPlrHand[i].m_bEnabled        = 1;
        m_aCardButtonsA[i].SetMoveToControl(0, &m_aPlrHand[i]);
        m_aPlrHand[i].SetMoveToControl(2, &m_aCardButtonsA[i]);
#ifndef K1_UI_XBOX  // xbox: End Turn / Stand are X / Y; the d-pad keeps to the ring above
        if (i == 0) {
            m_aCardButtonsA[0].SetMoveToControl(1, &m_actbtn1);
            m_aPlrHand[0].SetMoveToControl(1, &m_actbtn1);
            m_actbtn1.SetMoveToControl(3, &m_aPlrHand[0]);
            m_actbtn1.SetMoveToControl(3, &m_aPlrHand[0]);
        }
        if (i == 3) {
            m_aCardButtonsA[3].SetMoveToControl(3, &m_actbtn0);
            m_aPlrHand[3].SetMoveToControl(3, &m_actbtn0);
            m_actbtn0.SetMoveToControl(1, &m_aPlrHand[3]);
            m_actbtn0.SetMoveToControl(1, &m_aPlrHand[3]);
        }
#endif
    }
#else
#pragma clang loop unroll(disable)
    for (int i = 0; i < 4; i++) {
        m_aCardButtonsA[i].SetMoveToControl(0, &m_aPlrHand[i]);
        m_aPlrHand[i].SetMoveToControl(2, &m_aCardButtonsA[i]);
        m_aPlrHand[i].m_tail.bFocusOnHover = 1;
        m_aPlrHand[i].m_bEnabled        = 1;
        if (i == 0) {
            m_aCardButtonsA[0].SetMoveToControl(1, &m_actbtn1);
            m_aPlrHand[0].SetMoveToControl(1, &m_actbtn1);
            m_actbtn1.SetMoveToControl(3, &m_aPlrHand[0]);
            m_actbtn1.SetMoveToControl(3, &m_aPlrHand[0]);
        }
        if (i == 3) {
            m_aCardButtonsA[3].SetMoveToControl(3, &m_actbtn0);
            m_aPlrHand[3].SetMoveToControl(3, &m_actbtn0);
            m_actbtn0.SetMoveToControl(1, &m_aPlrHand[3]);
            m_actbtn0.SetMoveToControl(1, &m_aPlrHand[3]);
        }
        if (i != 0)
            m_aPlrHand[i].SetMoveToControl(1, &m_aPlrHand[i - 1]);
        if (i < 3)
            m_aPlrHand[i].SetMoveToControl(3, &m_aPlrHand[i + 1]);
    }
#endif
#endif
}

#if !KOTOR1
void CSWGuiPazaakGame::HandleChangeHandCard() {
    CSWGuiControl* pActive = m_pActiveControl;
    if (pActive) {
        int64_t n = pActive->m_tail.nControlId;
        if (n >= 0 && n <= 3 && pActive == &m_aPlrHand[n]) {
            CPazaakCard* pCard = &m_pPazaak->m_players[0].m_hand[n];
            if (pCard->m_nCardType == PAZAAK_CARD_PLUS_MINUS_1_2) {
                pCard->m_nValue = static_cast<unsigned int>(pCard->m_nValue) < 2 ? 2 : 1;
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                RefreshDisplay();
            }
        }
    }
}
#endif

#if !KOTOR1
void CSWGuiPazaakGame::HandleChangeButtons(CSWGuiControl* pControl) {
    CSWGuiControl* pSaved = m_pActiveControl;
    int n = pControl->m_tail.nControlId;
    CSWGuiPazaakCard* pNew = &m_aPlrHand[n];
    m_pActiveControl = pNew;
    int n2 = pNew->m_tail.nControlId;
    if (n2 >= 0 && n2 <= 3 && n2 == n) {
        CPazaakCard* pCard = &m_pPazaak->m_players[0].m_hand[n2];
        if (pCard->m_nCardType == PAZAAK_CARD_PLUS_MINUS_1_2) {
            pCard->m_nValue = static_cast<unsigned int>(pCard->m_nValue) < 2 ? 2 : 1;
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            RefreshDisplay();
        }
    }
    m_pActiveControl = pSaved;
}
#endif

#if !KOTOR1  // K2 only
// Routes the help bubble's A-press to one of the table's tutorial actions, keyed on the OK button's
// current strref, i.e. the page's own label.
void CSWGuiTutorial::HandlePazaakTutorialDismissButton()
{
    if (m_pPazaakGame == 0 || m_bEnabled == 0)
        return;

    if (m_btnOk.m_text.m_params.m_nStrRef == STRREF_END_TURN)
        m_pPazaakGame->HandleTutorialContinue();
    else if (m_btnOk.m_text.m_params.m_nStrRef == STRREF_PLAY_CARD)
        m_pPazaakGame->HandleTutorialPlayHandCard();
    else if (m_btnOk.m_text.m_params.m_nStrRef == STRREF_STAND)
        m_pPazaakGame->HandleTutorialStand();
}
#endif

#if !KOTOR1
void CSWGuiPazaakGame::HandleTutorialContinue() {
    if (!m_pPazaak->m_players[0].m_bStanding &&
        (m_nGameState == GAME_STATE_PLAYER_PLAY || m_nGameState == GAME_STATE_PLAYER_PLAYED_CARD) && m_nHandPickMode == 0) {
        m_actbtn0.m_fFlashTimer  = 0.8f;
        m_actbtn0.m_labelPos = CGuiInGame::COLOR_CYAN;
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
        m_fSequenceDelay    = 0.4f;
        RefreshDisplay();
    }
}
#endif

void CSWGuiPazaakGame::HandleStand(CSWGuiControl* pControl) {
    if ((!pControl || pControl->m_tail.nCurrentEventArg) &&
        !m_pPazaak->m_players[0].m_bStanding &&
        (m_nGameState == GAME_STATE_PLAYER_PLAY || m_nGameState == GAME_STATE_PLAYER_PLAYED_CARD)) {
#if KOTOR1  // K1: the Stand caption flashes cyan
        m_actbtn1.m_fFlashTimer = 0.8f;
        m_actbtn1.m_labelPos = CGuiInGame::COLOR_CYAN;
#endif
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        if (m_tutorial.m_bEnabled != 0 && m_pPazaak->m_players[0].GetTotal() <= 13)
            m_tutorial.ShowHelp(STRREF_TUT_STAND_CONFIRM, STRREF_STAND, STRREF_PAZAAK_CANCEL);
        else {
            m_pPazaak->m_players[0].m_bStanding = 1;
            m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
            m_fSequenceDelay     = 0.4f;
            RefreshDisplay();
        }
    }
}

void CSWGuiPazaakGame::HandleHandCardRightClick(CSWGuiControl* pControl) {
    if (pControl) {
        CSWGuiControl* pSaved = m_pActiveControl;
        m_pActiveControl = pControl;
        HandleFlipHandCard();
        m_pActiveControl = pSaved;
    }
}

void CSWGuiPazaakGame::HandleFlipHandCard() {
    CSWGuiControl* pActive = m_pActiveControl;
    if (pActive) {
        int64_t n = pActive->m_tail.nControlId;
        if (n >= 0 && n <= 3 && pActive == &m_aPlrHand[n] &&
            m_pPazaak->m_players[0].m_hand[n].m_nCardType >= PAZAAK_CARD_PLUS_MINUS_1 &&
#if KOTOR1  // K1: only the six flippable cards flip
            m_pPazaak->m_players[0].m_hand[n].m_nCardType < PAZAAK_CARD_MAIN_1) {
#else
            (m_pPazaak->m_players[0].m_hand[n].m_nCardType < PAZAAK_CARD_TIEBREAKER ||
             (m_pPazaak->m_players[0].m_hand[n].m_nCardType | 4) == PAZAAK_CARD_PLUS_MINUS_1_2)) {
#endif
            m_actbtn3.m_fFlashTimer = 0.8f;
            m_actbtn3.m_labelPos = CGuiInGame::COLOR_CYAN;
            m_pPazaak->m_players[0].m_hand[n].m_bIsNegative =
                m_pPazaak->m_players[0].m_hand[n].m_bIsNegative == 0;
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            RefreshDisplay();
        }
    }
}

void CSWGuiPazaakGame::HandleFlipButtons(CSWGuiControl* pControl) {
    CSWGuiControl* pSaved = m_pActiveControl;
    int64_t n = pControl->m_tail.nControlId;
    CSWGuiPazaakCard* pNew = &m_aPlrHand[n];
    m_pActiveControl = pNew;
    int n2 = pNew->m_tail.nControlId;
    if (n2 >= 0 && n2 <= 3 && n == n2 &&
        m_pPazaak->m_players[0].m_hand[n2].m_nCardType >= PAZAAK_CARD_PLUS_MINUS_1 &&
#if KOTOR1  // K1: only the six flippable cards flip
        m_pPazaak->m_players[0].m_hand[n2].m_nCardType < PAZAAK_CARD_MAIN_1) {
#else
        (m_pPazaak->m_players[0].m_hand[n2].m_nCardType < PAZAAK_CARD_TIEBREAKER ||
         (m_pPazaak->m_players[0].m_hand[n2].m_nCardType | 4) == PAZAAK_CARD_PLUS_MINUS_1_2)) {
#endif
        m_actbtn3.m_fFlashTimer = 0.8f;
        m_actbtn3.m_labelPos = CGuiInGame::COLOR_CYAN;
        m_pPazaak->m_players[0].m_hand[n2].m_bIsNegative =
            m_pPazaak->m_players[0].m_hand[n2].m_bIsNegative == 0;
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        RefreshDisplay();
    }
    m_pActiveControl = pSaved;
}

void CSWGuiPazaakGame::HandleContinue(CSWGuiControl* pControl) {
    if ((!pControl || pControl->m_tail.nCurrentEventArg) &&
        !m_pPazaak->m_players[0].m_bStanding &&
        (m_nGameState == GAME_STATE_PLAYER_PLAY || m_nGameState == GAME_STATE_PLAYER_PLAYED_CARD) && m_nHandPickMode == 0) {
        m_actbtn0.m_fFlashTimer = 0.8f;
        m_actbtn0.m_labelPos = CGuiInGame::COLOR_CYAN;
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        if (m_tutorial.m_bEnabled != 0 && m_pPazaak->m_players[0].GetTotal() >= 16) {
            m_tutorial.ShowHelp(STRREF_TUT_END_TURN, STRREF_END_TURN, STRREF_PAZAAK_CANCEL);
            return;
        }
        m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
        m_fSequenceDelay     = 0.4f;
        RefreshDisplay();
    }
}

// Enables or disables the four hand-action buttons. Empty main-deck slots lose their selectable
// flag, and a disabled button's label drops to the disabled tint.
__attribute__((noinline)) void CSWGuiPazaakGame::SetHandEnabled(int bEnabled) {
#if KOTOR1  // K1: the focus moves to End Turn, or to the first live hand card
    int nFocus = -1;
    if (m_aPlrHand[3].m_card.m_nCardType < 0)
        m_aPlrHand[3].m_bEnabled = 0;
    else
        nFocus = 3;
    if (m_aPlrHand[2].m_card.m_nCardType < 0)
        m_aPlrHand[2].m_bEnabled = 0;
    else
        nFocus = 2;
    if (m_aPlrHand[1].m_card.m_nCardType < 0)
        m_aPlrHand[1].m_bEnabled = 0;
    else
        nFocus = 1;
    if (m_aPlrHand[0].m_card.m_nCardType < 0)
        m_aPlrHand[0].m_bEnabled = 0;
    else
        nFocus = 0;
    int bDisabled = (bEnabled == 0);
    m_actbtn0.CSWGuiActivatedButton::SetEnabled(bDisabled);
    m_actbtn1.CSWGuiActivatedButton::SetEnabled(bDisabled);
    m_actbtn2.CSWGuiActivatedButton::SetEnabled(bDisabled);
    m_actbtn3.CSWGuiActivatedButton::SetEnabled(bDisabled);
#ifdef K1_UI_XBOX
    // xbox: X / Y are End Turn / Stand, so the focus stays on the hand: a focused live card keeps
    // it, else the first live card has it.
    int bOnHand = 0;
    for (int i = 0; i < 4; i++)
        if (m_pActiveControl == &m_aPlrHand[i] && m_aPlrHand[i].m_card.m_nCardType >= 0)
            bOnHand = 1;
    if (!bOnHand)
        SetActiveControl(nFocus >= 0 ? &m_aPlrHand[nFocus] : 0, 0);
#else
    if (!bEnabled)
        SetActiveControl(&m_actbtn0, 0);
    else if (nFocus >= 0)
        SetActiveControl(&m_aPlrHand[nFocus], 0);
#endif
    RefreshDisplay();
#else
    if (m_aPlrHand[3].m_card.m_nCardType < 0)
        m_aPlrHand[3].m_bEnabled = 0;
    if (m_aPlrHand[2].m_card.m_nCardType < 0)
        m_aPlrHand[2].m_bEnabled = 0;
    if (m_aPlrHand[1].m_card.m_nCardType < 0)
        m_aPlrHand[1].m_bEnabled = 0;
    if (m_aPlrHand[0].m_card.m_nCardType < 0)
        m_aPlrHand[0].m_bEnabled = 0;
    int bDisabled = (bEnabled == 0);
    m_actbtn0.CSWGuiActivatedButton::SetEnabled(bDisabled);
    m_actbtn1.CSWGuiActivatedButton::SetEnabled(bDisabled);
    m_actbtn2.CSWGuiActivatedButton::SetEnabled(bDisabled);
    m_actbtn3.CSWGuiActivatedButton::SetEnabled(bDisabled);
    RefreshDisplay();
#endif
}

// Recolors every label and button for the current game state and pushes the card images, scores
// and round pips onto the widgets. The compare guards keep SetCard off unchanged slots so their
// drag state survives.
#if KOTOR1
// K1 compares the two fields as one value.
static inline bool CardsDiffer(CPazaakCard a, CPazaakCard b)
{
    return a.m_nCardType != b.m_nCardType || a.m_bIsNegative != b.m_bIsNegative;
}
#endif
void CSWGuiPazaakGame::RefreshDisplay() {
    // The played rows are tinted full white once that side stands or setup is over, dim grey
    // otherwise.
    const int bFull0 = (m_nGameState > GAME_STATE_SHOW_RESULT) | (m_pPazaak->m_players[0].m_bStanding == 0);
    const int bFull1 = (m_nGameState > GAME_STATE_SHOW_RESULT) | (m_pPazaak->m_players[1].m_bStanding == 0);
    const float fTint0 = bFull0 ? 1.0f : 0.667f;
    const float fTint1 = bFull1 ? 1.0f : 0.667f;

    // The played rows, the player's on m_aPlrBoard and the NPC's on m_aNpcBoard.
    int bDirty0 = 0;
    int bDirty1 = 0;
#pragma clang loop unroll(disable)
    for (int n = 0; n < 9; n++) {
#if KOTOR1
        const CPazaakCard rPlayed0 = m_pPazaak->m_players[0].m_cardsPlayed[n];
        if (CardsDiffer(m_aPlrBoard[n].m_card, rPlayed0)) {
#else
        const CPazaakCard& rPlayed0 = m_pPazaak->m_players[0].m_cardsPlayed[n];
        if (m_aPlrBoard[n].m_card.m_nCardType != rPlayed0.m_nCardType ||
            m_aPlrBoard[n].m_card.m_bIsNegative != rPlayed0.m_bIsNegative
            || m_aPlrBoard[n].m_card.m_nValue != rPlayed0.m_nValue
            ) {
#endif
            m_aPlrBoard[n].SetCard(rPlayed0);
            bDirty0 = 1;
        }
        m_aPlrBoard[n].m_border.m_params.m_color = CSWGuiColor(fTint0, fTint0, fTint0);
        const CPazaakCard& rPlayed1 = m_pPazaak->m_players[1].m_cardsPlayed[n];
#if KOTOR1
        if (CardsDiffer(m_aNpcBoard[n].m_card, rPlayed1)) {
#else
        if (m_aNpcBoard[n].m_card.m_nCardType != rPlayed1.m_nCardType ||
            m_aNpcBoard[n].m_card.m_bIsNegative != rPlayed1.m_bIsNegative
            || m_aNpcBoard[n].m_card.m_nValue != rPlayed1.m_nValue
            ) {
#endif
            m_aNpcBoard[n].SetCard(rPlayed1);
            bDirty1 = 1;
        }
        m_aNpcBoard[n].m_border.m_params.m_color = CSWGuiColor(fTint1, fTint1, fTint1);
    }

    // Hand rows. The player's hand also drives a button enable and two visibility mirrors, keyed
    // off the hand card being a playable side-deck card or the +/-1/2 card. Hidden NPC hand cards
    // render as the empty sentinel.
    bool bHandAction = false;  // any hand card is playable
#if !KOTOR1 || defined(K2_UI_XBOX)
    bool bHandHasPlusMinus12 = false;  // any hand card is the +/-1/2 card
#endif
    bool bRowLive = false;     // any hand-row widget shows a real card
#pragma clang loop unroll(disable)
    for (int n = 0; n < 4; n++) {
        const CPazaakCard rHand0 = m_pPazaak->m_players[0].m_hand[n];
#if KOTOR1  // K1: only the six flippable cards act
        const int bAction = static_cast<unsigned int>(rHand0.m_nCardType - PAZAAK_CARD_PLUS_MINUS_1) < 6;
#else
        const int bAction = static_cast<unsigned int>(rHand0.m_nCardType - PAZAAK_CARD_PLUS_MINUS_1) < 7 || rHand0.m_nCardType == PAZAAK_CARD_PLUS_MINUS_1_2;
#endif
        bHandAction = (bHandAction != 0) | (bAction != 0);
        const CPazaakCard& rShown0 = m_aPlrHand[n].m_card;
        if (rShown0.m_nCardType != rHand0.m_nCardType ||
            rShown0.m_bIsNegative != rHand0.m_bIsNegative
#if !KOTOR1
            || rShown0.m_nValue != rHand0.m_nValue
#endif
            ) {
            m_aPlrHand[n].SetCard(rHand0);
#ifndef K2_UI_XBOX  // console: no flip/change button row ([7]/[8] are the Change caption + badge)
            m_aCardButtonsA[n].SetEnabled(bAction);
            m_aCardButtonsA[n].m_tail.bVisible = static_cast<unsigned char>(bAction);
#if !KOTOR1
            m_aCardButtonsB[n].m_tail.bVisible = rHand0.m_nCardType == PAZAAK_CARD_PLUS_MINUS_1_2;
#endif
#endif
        }
        const float fTint2 = (m_nGameState == GAME_STATE_PLAYER_PLAY) ? 1.0f : 0.667f;
        m_aPlrHand[n].m_border.m_params.m_color = CSWGuiColor(fTint2, fTint2, fTint2);
#if !KOTOR1
        bHandHasPlusMinus12 = (bHandHasPlusMinus12 != 0) | (rHand0.m_nCardType == PAZAAK_CARD_PLUS_MINUS_1_2);
#endif
        bRowLive = (bRowLive != 0) | (m_aPlrHand[n].m_card.m_nCardType >= 0);

        const CPazaakCard rHand1 = m_pPazaak->m_players[1].m_hand[n];
        if (g_bPazaakShowNPCSideDeck != 0) {
            const CPazaakCard& rShown1 = m_aNpcHand[n].m_card;
            if (rShown1.m_nCardType != rHand1.m_nCardType ||
                rShown1.m_bIsNegative != rHand1.m_bIsNegative
#if !KOTOR1
                || rShown1.m_nValue != rHand1.m_nValue
#endif
                ) {
                m_aNpcHand[n].SetCard(rHand1);
            }
        } else {
            const int bHidden = rHand1.m_nCardType >= 0;
            CPazaakCard cardShown;
            cardShown.m_nCardType = bHidden ? PAZAAK_CARD_FACE_DOWN : rHand1.m_nCardType;
            cardShown.m_bIsNegative = bHidden ? 0 : rHand1.m_bIsNegative;
#if !KOTOR1
            cardShown.m_nValue = bHidden ? 0 : rHand1.m_nValue;
#endif
            const CPazaakCard& rShown1b = m_aNpcHand[n].m_card;
            if (rShown1b.m_nCardType != cardShown.m_nCardType ||
                rShown1b.m_bIsNegative != cardShown.m_bIsNegative
#if !KOTOR1
                || rShown1b.m_nValue != cardShown.m_nValue
#endif
                ) {
                m_aNpcHand[n].SetCard(cardShown);
            }
        }
    }
#ifndef K2_UI_XBOX  // console: m_bLabels are the always-shown legend badges
    m_blbl0.m_tail.bVisible = bHandAction;
    m_blbl1.m_tail.bVisible = bHandAction;
#if !KOTOR1
    m_blbl2.m_tail.bVisible = bHandHasPlusMinus12;
    m_blbl3.m_tail.bVisible = bHandHasPlusMinus12;
#endif
#endif

    // Refresh a score label only when that player's played row changed this pass.
    if (bDirty0) {
        CExoString strTotal(m_pPazaak->m_players[0].GetTotal());
        m_lbl2.m_text.m_params.SetText(strTotal);
    }
    if (bDirty1) {
        CExoString strTotal(m_pPazaak->m_players[1].GetTotal());
        m_lbl3.m_text.m_params.SetText(strTotal);
    }

    // Once a hand resolved, seed each side's next won-round pip slot from its won count.
    if (m_nGameState == GAME_STATE_RESULT_SHOWN) {
        const int nW0 = m_pPazaak->m_players[0].m_nRoundsWon;
        if (static_cast<unsigned int>(nW0 - 1) < 3) {
#if KOTOR1
            CResRef resOn("lbl_winmark02");
#else
            CResRef resOn("pz_rounds_on");
#endif
            m_aPlrScore[nW0 - 1].m_border.m_params.SetFillImage(resOn, 0);
        }
        const int nW1 = m_pPazaak->m_players[1].m_nRoundsWon;
        if (static_cast<unsigned int>(nW1 - 1) < 3) {
#if KOTOR1
            CResRef resOn2("lbl_winmark02");
#else
            CResRef resOn2("pz_rounds_on");
#endif
            m_aNpcScore[nW1 - 1].m_border.m_params.SetFillImage(resOn2, 0);
        }
    }

    // For the hand buttons, the menu waits for the reveal query at a live hand row, flip and change
    // follow a live row, and stand/cancel share the standing-window gate.
#ifdef K2_UI_XBOX
    // Console: the captions take the same enable rules as the PC action buttons (disabled text
    // against the gold tint), and the fifth caption, Change, is coloured by hand the same way.
    const int bTurn = m_pPazaak->m_players[0].m_bStanding == 0 && static_cast<unsigned int>(m_nGameState - GAME_STATE_PLAYER_PLAY) < 2;
    const int bPlay = m_nGameState == GAME_STATE_PLAYER_PLAY && bRowLive;
    m_actbtn0.CSWGuiActivatedButton::SetEnabled(bTurn);
    m_actbtn1.CSWGuiActivatedButton::SetEnabled(bTurn);
    m_actbtn2.CSWGuiActivatedButton::SetEnabled(bPlay);
    m_actbtn3.CSWGuiActivatedButton::SetEnabled(bHandAction && bRowLive);
    // SetEnabled ends in SetActive, and SetActive(0) restores the layout's caption colour, so
    // re-apply the intended tint afterwards unless a flash is running.
    for (int i = 0; i < 4; i++) {
        CSWGuiActivatedButton& rCaption = (&m_actbtn0)[i];
        if (rCaption.m_fFlashTimer < 0.0f)
            rCaption.m_text.m_params.SetColor(rCaption.m_tail.bEnabled ? CGuiInGame::COLOR_PAZAAK_GOLD
                                                                        : CGuiInGame::DISABLED_TEXT);
    }
    {
        const Vector& rColor = (bHandHasPlusMinus12 && bRowLive) ? CGuiInGame::COLOR_PAZAAK_GOLD
                                                      : CGuiInGame::DISABLED_TEXT;
        m_aCardButtonsB[3].m_text.m_params.SetColor(rColor);
    }
#else
    bool bG2 = false;
    if (m_nGameState == GAME_STATE_PLAYER_PLAY && m_pActiveControl != 0 && m_nHandPickMode == 0) {
        bG2 = bRowLive;
    }
    m_actbtn2.CSWGuiActivatedButton::SetEnabled(bG2);
    m_actbtn3.CSWGuiActivatedButton::SetEnabled(bRowLive && m_nHandPickMode == 0);
    bool bG01 = false;
    if (m_pPazaak->m_players[0].m_bStanding == 0 && static_cast<unsigned int>(m_nGameState - GAME_STATE_PLAYER_PLAY) < 2 &&
        m_nHandPickMode == 0) {
        bG01 = true;
    }
    m_actbtn0.CSWGuiActivatedButton::SetEnabled(bG01);
    m_actbtn1.CSWGuiActivatedButton::SetEnabled(bG01);

    // As a de-focus kludge, while button0 holds focus outside the opponent's turn, bounce it off and
    // back.
    if (GetActiveControl() == &m_actbtn0 && (m_nGameState & ~1) != GAME_STATE_OPPONENT_TURN) {
        SetActiveControl(0, 0);
        SetActiveControl(&m_actbtn0, 0);
    }
#endif
}

void CSWGuiPazaakGame::HandleInputEvent(int nType, int nValue) {
#ifdef K2_UI_XBOX
    // Console input goes through a jump table over the face-button events on a press, then the base
    // call for every event. Each arm is the PC table's own handler with the console's guards, so
    // the handlers are called rather than re-spelled, and the d-pad walks the links that
    // ReconnectSidedeckButtons sets. The bumpers arrive as menu events and drive Flip/Change.
    if (nValue != 0) {
        switch (nType) {
        case INPUTEVENT_GUI_A:
        case INPUTEVENT_GUI_START:
            HandleContinue(0);
            break;
        case INPUTEVENT_GUI_Y:
            if (!m_pPazaak->m_players[0].m_bStanding &&
                (m_nGameState == GAME_STATE_PLAYER_PLAY || m_nGameState == GAME_STATE_PLAYER_PLAYED_CARD)) {
                m_actbtn1.m_fFlashTimer = 0.8f;
                m_actbtn1.m_labelPos = CGuiInGame::COLOR_CYAN;
            }
            HandleStand(0);
            break;
        case INPUTEVENT_GUI_X: {
            CSWGuiControl* pActive = m_pActiveControl;
            if (m_nGameState == GAME_STATE_PLAYER_PLAY && pActive) {
                const int n = pActive->m_tail.nControlId;
                if (n >= 0 && n <= 3 && pActive == &m_aPlrHand[n])
                    HandlePlayHandCard(n);
            }
            break;
        }
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pBox->SetAllowCancel(1);
            pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleQuitDialog), 0);
            pBox->SetMessage(STRREF_CONFIRM_FORFEIT);
            pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
            m_pGuiManager->AddPanel(pBox, 1, 1);
            break;
        }
        case INPUTEVENT_GUI_BLACK:
        case INPUTEVENT_GUI_LEFT_SHOULDER:
        case INPUTEVENT_GUI_WHITE:
        case INPUTEVENT_GUI_RIGHT_SHOULDER: {
            const int bFlip = nType == INPUTEVENT_GUI_BLACK || nType == INPUTEVENT_GUI_LEFT_SHOULDER;
            // One "help shown" bit per action lives in this table's flags word.
            const int nBit = bFlip ? TUTORIAL_SHOWN_FLIP : TUTORIAL_SHOWN_CHANGE;
            if (!(m_anTutorialState[0] & nBit)) {
                m_anTutorialState[0] |= nBit;
                const uint64_t anHelpRefs[2] = {STRREF_TUT_FLIP_CARDS, STRREF_TUT_DECLARE_SIGN};
                m_tutorial.ShowMultipleHelp(anHelpRefs, 2);
            }
            if (bFlip)
                HandleFlipHandCard();
            else
                HandleChangeHandCard();
            break;
        }
        }
    }
#else
#if KOTOR1 && defined(K1_UI_XBOX)
    if (nValue != 0 && nType == INPUTEVENT_GUI_X)
        HandleContinue(0);
    else if (nValue != 0 && nType == INPUTEVENT_GUI_Y)
        HandleStand(0);
#endif
    if (nValue != 0 && (nType == INPUTEVENT_GUI_BACK || nType == INPUTEVENT_GUI_B)) {
        if (m_nHandPickMode != 0) {
            m_nHandPickMode = 0;
            SetHandEnabled(0);
        } else {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pBox->SetAllowCancel(1);
            pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleQuitDialog), 0);
            pBox->SetMessage(STRREF_CONFIRM_FORFEIT);
#if !KOTOR1  // K1: the box keeps its own colours
            pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
            m_pGuiManager->AddPanel(pBox, 1, 1);
        }
    }
#endif
    CSWGuiPanel::HandleInputEvent(nType, nValue);
}


void CSWGuiPazaakGame::OnPanelAdded() {
    StartNewGame();
#if !KOTOR1  // K1: StartNewGame has already placed the focus
    SetActiveControl(&m_aPlrHand[3], 0);
#endif
    m_lbl0.m_text.m_params.SetText(
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()->m_pStats->GetFullName());
    ReconnectSidedeckButtons();
    CSWGuiPanel::OnPanelAdded();
}

#if !KOTOR1
// Runs the tutorial's Play Card action, with the same guard and auto-stand as HandlePlayHandCard
// plus the tutorial caption flash.
void CSWGuiPazaakGame::HandleTutorialPlayHandCard() {
    if (m_nGameState == GAME_STATE_PLAYER_PLAY && m_anTutorialState[1] >= 0 && m_anTutorialState[1] <= 3 &&
        m_pPazaak->CanUsePlayerSidedeckCard(m_anTutorialState[1])) {
        m_actbtn2.m_fFlashTimer = 0.8f;
        m_actbtn2.m_labelPos = CGuiInGame::COLOR_CYAN;
        m_pPazaak->UsePlayerSidedeckCard(m_anTutorialState[1]);
        int nPlayerTotal = m_pPazaak->m_players[0].GetTotal();
        int nOpponentStanding = m_pPazaak->m_players[1].m_bStanding;
        int nOpponentTotal = m_pPazaak->m_players[1].GetTotal();
        if (nOpponentTotal <= 19 && nPlayerTotal <= 19 && nOpponentStanding != 0 &&
            nPlayerTotal > nOpponentTotal && nOpponentTotal >= -999999)
            m_pPazaak->m_players[0].m_bStanding = 1;
        g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[2]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
        m_nGameState = GAME_STATE_PLAYER_PLAYED_CARD;
        m_fSequenceDelay     = 0.4f;
        m_aPlrHand[m_anTutorialState[1]].m_bEnabled = 0;
        m_aPlrHand[m_anTutorialState[1]].SetActive(0);
        RefreshDisplay();
        ReconnectSidedeckButtons();
    }
}
#endif

void CSWGuiPazaakGame::HandleHandCardDrop(CSWGuiControl* pControl) {
    CSWGuiControl* pDragged = g_pGuiMan->m_pMouseOverControl;
    if (!pDragged)
        return;
    if (pDragged == &m_actbtn3) {
        CSWGuiControl* pSaved = m_pActiveControl;
        m_pActiveControl = pControl;
        if (pControl) {
            int64_t n = pControl->m_tail.nControlId;
            if (n >= 0 && n <= 3 && pControl == &m_aPlrHand[n] &&
                m_pPazaak->m_players[0].m_hand[n].m_nCardType >= PAZAAK_CARD_PLUS_MINUS_1 &&
#if KOTOR1
                m_pPazaak->m_players[0].m_hand[n].m_nCardType < PAZAAK_CARD_MAIN_1) {
#else
                (m_pPazaak->m_players[0].m_hand[n].m_nCardType < PAZAAK_CARD_TIEBREAKER ||
                 (m_pPazaak->m_players[0].m_hand[n].m_nCardType | 4) == PAZAAK_CARD_PLUS_MINUS_1_2)) {
#endif
                m_actbtn3.m_fFlashTimer = 0.8f;
                m_actbtn3.m_labelPos = CGuiInGame::COLOR_CYAN;
                m_pPazaak->m_players[0].m_hand[n].m_bIsNegative =
                    m_pPazaak->m_players[0].m_hand[n].m_bIsNegative == 0;
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                RefreshDisplay();
            }
        }
        m_pActiveControl = pSaved;
    } else if (m_nGameState == GAME_STATE_PLAYER_PLAY &&
               (pDragged == &m_aPlrBoard[7] || pDragged == &m_aPlrBoard[8] ||
                pDragged == &m_aPlrBoard[6] || pDragged == &m_aPlrBoard[5] ||
                pDragged == &m_aPlrBoard[4] || pDragged == &m_aPlrBoard[3] ||
                pDragged == &m_aPlrBoard[2] || pDragged == &m_aPlrBoard[1] ||
                pDragged == &m_aPlrBoard[0] || pDragged == &m_actbtn2)) {
        HandlePlayHandCard(pControl->m_tail.nControlId);
    }
}

// Plays the side-deck card in hand slot nSlot, with the auto-stand rule for a winning card.
void CSWGuiPazaakGame::HandlePlayHandCard(int nSlot) {
    if (m_nGameState == GAME_STATE_PLAYER_PLAY && nSlot >= 0 && nSlot <= 3 &&
        m_pPazaak->CanUsePlayerSidedeckCard(nSlot)) {
        m_actbtn2.m_fFlashTimer = 0.8f;
        m_actbtn2.m_labelPos = CGuiInGame::COLOR_CYAN;
        if (m_tutorial.m_bEnabled != 0 && m_pPazaak->m_players[0].GetTotal() <= 12) {
            m_anTutorialState[1] = nSlot;
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_tutorial.ShowHelp(STRREF_TUT_ONE_HAND_CARD, STRREF_PLAY_CARD, STRREF_PAZAAK_CANCEL);
            return;
        }
        m_pPazaak->UsePlayerSidedeckCard(nSlot);
#if !KOTOR1  // K1: no auto-stand, and the played slot stays live
        int nPlayerTotal = m_pPazaak->m_players[0].GetTotal();
        int nOpponentStanding = m_pPazaak->m_players[1].m_bStanding;
        int nOpponentTotal = m_pPazaak->m_players[1].GetTotal();
        if (nOpponentTotal <= 19 && nPlayerTotal <= 19 && nOpponentStanding != 0 &&
            nPlayerTotal > nOpponentTotal && nOpponentTotal >= -999999)
            m_pPazaak->m_players[0].m_bStanding = 1;
#endif
        g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[2]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
        m_nGameState = GAME_STATE_PLAYER_PLAYED_CARD;
        m_fSequenceDelay     = 0.4f;
#if !KOTOR1
        m_aPlrHand[nSlot].m_bEnabled = 0;
        m_aPlrHand[nSlot].SetActive(0);
#endif
        RefreshDisplay();
        ReconnectSidedeckButtons();
    }
}

int CSWGuiPazaakGame::DoGameSequence()
{
#if KOTOR1  // K1: the player always opens, no auto-stand, turn lights are label visibility
    switch (m_nGameState) {
    case GAME_STATE_START_SET:
        m_nGameState = GAME_STATE_PLAYER_TURN;
        return 1;
    case GAME_STATE_END_MATCH:
        SetToBeDeleted(0);
        g_pAppManager->m_pClientExoApp->EndPazaakGame(
            m_pPazaak->m_players[0].m_nRoundsWon > m_pPazaak->m_players[1].m_nRoundsWon, m_pPazaak->m_nWager, 0,
            m_pPazaak->m_oidOpponent);
        return 0;
    case GAME_STATE_NEXT_SET:
        StartNewGame();
        return 0;
    case GAME_STATE_PLAYER_DRAW: {
        m_pPazaak->m_players[0].AddGameCard(m_pPazaak->DrawCard());
        g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[1]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
        if ((m_anTutorialState[0] & TUTORIAL_SHOWN_FIRST_DRAW) == 0) {
            m_anTutorialState[0] |= TUTORIAL_SHOWN_FIRST_DRAW;
            m_tutorial.ShowHelp(STRREF_TUT_FIRST_DRAW, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
            m_tutorial.PlaceRightOf(&m_aPlrBoard[0]);
        }
        if (m_pPazaak->m_players[0].m_bStanding != 0) {
            m_actbtn1.m_fFlashTimer = 0.8f;
            m_actbtn1.m_labelPos = CGuiInGame::COLOR_CYAN;
            m_fSequenceDelay = 0.4f;
        }
        if (m_pPazaak->m_players[0].GetTotal() > PAZAAK_WINNING_TOTAL)
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[3]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
        m_nGameState = GAME_STATE_PLAYER_PLAY;
        CSWGuiControl* pActive = GetActiveControl();
        SetActiveControl(0, 0);
        SetActiveControl(pActive, 0);
        RefreshDisplay();
        return 0;
    }
    case GAME_STATE_PLAYER_TURN:
        if (m_pPazaak->m_players[0].m_bStanding == 0) {
            m_lbl6.m_tail.bVisible = 1;
            m_lbl7.m_tail.bVisible = 0;
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[0]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            m_nGameState = GAME_STATE_PLAYER_DRAW;
            m_fSequenceDelay = 0.4f;
            return 0;
        }
        m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
        return 1;
    case GAME_STATE_PLAYER_PLAY:
    case GAME_STATE_PLAYER_PLAYED_CARD:
        if (m_pPazaak->m_players[0].m_bStanding != 0) {
            m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
            return 1;
        }
        return 0;
    case GAME_STATE_PLAYER_TURN_OVER: {
        int nMyTotal = m_pPazaak->m_players[0].GetTotal();
        if (nMyTotal >= 21) {
            m_tutorial.ShowHelp(STRREF_TUT_OVER_20, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
        } else if (nMyTotal == PAZAAK_WINNING_TOTAL && (m_anTutorialState[0] & TUTORIAL_SHOWN_EXACTLY_20) == 0) {
            m_anTutorialState[0] |= TUTORIAL_SHOWN_EXACTLY_20;
            m_tutorial.ShowHelp(STRREF_TUT_EXACTLY_20, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
        }
        if (m_pPazaak->DetermineWinner() == CSWPazaak::WINNER_NONE) {
            m_nGameState = GAME_STATE_OPPONENT_TURN;
            if (m_pPazaak->m_players[1].m_bStanding != 0) {
                m_nGameState = GAME_STATE_SET_OVER;
                return 1;
            }
        } else {
            m_nGameState = GAME_STATE_SET_OVER;
            return 1;
        }
    }
    case GAME_STATE_OPPONENT_TURN:
        if (m_pPazaak->m_players[1].m_bStanding != 0) {
            m_nGameState = GAME_STATE_SET_OVER;
            return 1;
        }
        m_lbl6.m_tail.bVisible = 0;
        m_lbl7.m_tail.bVisible = 1;
        g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[0]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
        m_nGameState = GAME_STATE_OPPONENT_PLAY;
        m_fSequenceDelay = 0.4f;
        if ((m_anTutorialState[0] & TUTORIAL_SHOWN_OPPONENT_RULES) == 0) {
            m_anTutorialState[0] |= TUTORIAL_SHOWN_OPPONENT_RULES;
            m_tutorial.ShowHelp(STRREF_TUT_OPPONENT_RULES, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
        }
        return 0;
    case GAME_STATE_OPPONENT_PLAY: {
        CSWPazaak* pPazaak = m_pPazaak;
        int nResult = pPazaak->PlayAITurn(pPazaak->m_players[1], pPazaak->m_players[0],
                                          static_cast<CSWPazaak::EAIState>(pPazaak->m_nAIState));
        pPazaak->m_nAIState = nResult;
        switch (nResult) {
        case CSWPazaak::EAISTATE_START_TURN:
            m_nGameState = GAME_STATE_SET_OVER;
            m_fSequenceDelay = 0.4f;
            break;
        case CSWPazaak::EAISTATE_DREW_CARD:
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[1]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            if (m_pPazaak->m_players[1].GetTotal() > PAZAAK_WINNING_TOTAL)
                g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[3]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            break;
        case CSWPazaak::EAISTATE_PLAYED_SIDE_CARD:
            m_actbtn2.m_fFlashTimer = 0.8f;
            m_actbtn2.m_labelPos = CGuiInGame::COLOR_RED;
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[2]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            break;
        case CSWPazaak::EAISTATE_STOOD:
            m_actbtn1.m_fFlashTimer = 0.8f;
            m_actbtn1.m_labelPos = CGuiInGame::COLOR_RED;
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            RefreshDisplay();
            return 0;
        case CSWPazaak::EAISTATE_ENDED_TURN:
            m_actbtn0.m_fFlashTimer = 0.8f;
            m_actbtn0.m_labelPos = CGuiInGame::COLOR_RED;
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            RefreshDisplay();
            return 0;
        default:
            RefreshDisplay();
            return 0;
        }
        if (nResult != CSWPazaak::EAISTATE_START_TURN) {
            m_fSequenceDelay = 0.8f;
            if (m_pPazaak->m_players[1].GetTotal() == PAZAAK_WINNING_TOTAL && (m_anTutorialState[0] & TUTORIAL_SHOWN_EXACTLY_20) == 0) {
                m_anTutorialState[0] |= TUTORIAL_SHOWN_EXACTLY_20;
                m_tutorial.ShowHelp(STRREF_TUT_EXACTLY_20, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
            }
            RefreshDisplay();
            return 0;
        }
    }
    case GAME_STATE_SET_OVER: {
        int nWinner = m_pPazaak->DetermineWinner();
        if (nWinner == CSWPazaak::WINNER_NONE) {
            m_nGameState = GAME_STATE_PLAYER_TURN;
            RefreshDisplay();
            return 0;
        }
        m_dialog.SetAllowCancel(0);
        if (nWinner == CSWPazaak::WINNER_PLAYER) {
            m_pPazaak->m_players[0].m_nRoundsWon++;
            if (m_pPazaak->m_players[0].m_nRoundsWon >= 3) {
                m_dialog.SetMessage(STRREF_YOU_WIN_MATCH);
                m_pszPendingSound = reinterpret_cast<unsigned long long>("mgs_winmatch");
            } else {
                m_dialog.SetMessage(STRREF_YOU_WIN_SET);
                m_pszPendingSound = reinterpret_cast<unsigned long long>("mgs_winset");
            }
        } else if (nWinner == CSWPazaak::WINNER_OPPONENT) {
            m_pPazaak->m_players[1].m_nRoundsWon++;
            if (m_pPazaak->m_players[1].m_nRoundsWon >= 3) {
                m_dialog.SetMessage(STRREF_YOU_LOSE_MATCH);
                m_pszPendingSound = reinterpret_cast<unsigned long long>("mgs_losematch");
            } else {
                m_dialog.SetMessage(STRREF_OPPONENT_WINS_SET);
                m_pszPendingSound = reinterpret_cast<unsigned long long>("mgs_loseset");
            }
        } else {
            m_dialog.SetMessage(STRREF_SET_TIED);
        }
        m_nGameState = GAME_STATE_SHOW_RESULT;
        RefreshDisplay();
        return 0;
    }
    case GAME_STATE_SHOW_RESULT:
        if (m_pszPendingSound != 0) {
            g_pExoSound->PlayOneShotSound(CResRef(reinterpret_cast<char*>(m_pszPendingSound)), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            m_pszPendingSound = 0;
        }
        m_dialog.SetCallback(this,
                             static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleEndDialog), 0);
        m_pGuiManager->AddPanel(&m_dialog, 1, 1);
        m_nGameState = GAME_STATE_RESULT_SHOWN;
        RefreshDisplay();
        return 0;
    case GAME_STATE_RESULT_SHOWN:
        break;
    }
    return 0;
#else
    switch (m_nGameState) {
    case GAME_STATE_START_SET:
        m_nGameState = m_bPlayerStarts ? GAME_STATE_PLAYER_TURN : GAME_STATE_OPPONENT_PLAY;
        m_bPlayerStarts = !m_bPlayerStarts;
        return 1;
    case GAME_STATE_END_MATCH:
        SetToBeDeleted(0);
        g_pAppManager->m_pClientExoApp->EndPazaakGame(
            m_pPazaak->m_players[0].m_nRoundsWon > m_pPazaak->m_players[1].m_nRoundsWon, m_pPazaak->m_nWager, 0,
            m_pPazaak->m_oidOpponent);
        return 0;
    case GAME_STATE_NEXT_SET:
        StartNewGame();
        return 0;
    case GAME_STATE_PLAYER_DRAW: {
        m_pPazaak->DrawPlayerCard();
        g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[1]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
        if ((m_anTutorialState[0] & TUTORIAL_SHOWN_FIRST_DRAW) == 0) {
            m_anTutorialState[0] |= TUTORIAL_SHOWN_FIRST_DRAW;
            m_tutorial.ShowHelp(STRREF_TUT_FIRST_DRAW, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
            m_tutorial.PlaceRightOf(&m_aPlrBoard[0]);
        }
        if (m_pPazaak->m_players[0].m_bStanding != 0)
            m_fSequenceDelay = 0.4f;
        int nMyTotal = m_pPazaak->m_players[0].GetTotal();
        if (nMyTotal > PAZAAK_WINNING_TOTAL)
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[3]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
        int nOppStanding = m_pPazaak->m_players[1].m_bStanding;
        int nOppTotal = m_pPazaak->m_players[1].GetTotal();
        if (nOppStanding != 0 && nOppTotal < PAZAAK_WINNING_TOTAL && nMyTotal < PAZAAK_WINNING_TOTAL &&
            nMyTotal > nOppTotal && nOppTotal > -1000000)
            m_pPazaak->m_players[0].m_bStanding = 1;
        m_nGameState = GAME_STATE_PLAYER_PLAY;
        CSWGuiControl* pActive = GetActiveControl();
        SetActiveControl(0, 0);
        SetActiveControl(pActive, 0);
        RefreshDisplay();
        return 0;
    }
    case GAME_STATE_PLAYER_TURN:
        if (m_pPazaak->m_players[0].m_bStanding == 0) {
            CResRef resOn("pz_playerliteon");
            m_lbl6.m_border.m_params.SetFillImage(resOn, 0);
            CResRef resOff("PZ_PLAYERLITEOFF");
            m_lbl7.m_border.m_params.SetFillImage(resOff, 0);
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[0]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            m_nGameState = GAME_STATE_PLAYER_DRAW;
            m_fSequenceDelay = 0.1f;
            return 0;
        }
        m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
        return 1;
    case GAME_STATE_PLAYER_PLAY: {
        int nOppStanding = m_pPazaak->m_players[1].m_bStanding;
        int nOppTotal = m_pPazaak->m_players[1].GetTotal();
        int nMyTotal  = m_pPazaak->m_players[0].GetTotal();
        if (nOppStanding != 0 && nOppTotal < PAZAAK_WINNING_TOTAL && nMyTotal < PAZAAK_WINNING_TOTAL && nOppTotal > -1000000 &&
            nMyTotal > nOppTotal)
            m_pPazaak->m_players[0].m_bStanding = 1;
    }
        // fallthrough: the shared already-stood check
    case GAME_STATE_PLAYER_PLAYED_CARD:
        if (m_pPazaak->m_players[0].m_bStanding != 0) {
            m_nGameState = GAME_STATE_PLAYER_TURN_OVER;
            return 1;
        }
        return 0;
    case GAME_STATE_PLAYER_TURN_OVER: {
        int nMyTotal = m_pPazaak->m_players[0].GetTotal();
        if (nMyTotal >= 21) {
            m_tutorial.ShowHelp(STRREF_TUT_OVER_20, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
        } else if (nMyTotal == PAZAAK_WINNING_TOTAL && (m_anTutorialState[0] & TUTORIAL_SHOWN_EXACTLY_20) == 0) {
            m_anTutorialState[0] |= TUTORIAL_SHOWN_EXACTLY_20;
            m_tutorial.ShowHelp(STRREF_TUT_EXACTLY_20, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
        }
        if (m_pPazaak->DetermineWinner() == CSWPazaak::WINNER_NONE) {
            m_nGameState = GAME_STATE_OPPONENT_TURN;
            if (m_pPazaak->m_players[1].m_bStanding != 0) {
                m_nGameState = GAME_STATE_SET_OVER;
                return 1;
            }
        } else {
            m_nGameState = GAME_STATE_SET_OVER;
            return 1;
        }
    }
    case GAME_STATE_OPPONENT_TURN:
        if (m_pPazaak->m_players[1].m_bStanding != 0) {
            m_nGameState = GAME_STATE_SET_OVER;
            return 1;
        }
        {
            CResRef resOff("pz_playerliteoff");
            m_lbl6.m_border.m_params.SetFillImage(resOff, 0);
            CResRef resOn("pz_playerliteon");
            m_lbl7.m_border.m_params.SetFillImage(resOn, 0);
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[0]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
        }
        m_nGameState = GAME_STATE_OPPONENT_PLAY;
        m_fSequenceDelay = 0.1f;
        if ((m_anTutorialState[0] & TUTORIAL_SHOWN_OPPONENT_RULES) == 0) {
            m_anTutorialState[0] |= TUTORIAL_SHOWN_OPPONENT_RULES;
            m_tutorial.ShowHelp(STRREF_TUT_OPPONENT_RULES, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
        }
        return 0;
    case GAME_STATE_OPPONENT_PLAY: {
        int nResult = m_pPazaak->AIPlayNPCTurn();
        switch (nResult) {
        case CSWPazaak::EAISTATE_START_TURN:
            m_nGameState = GAME_STATE_SET_OVER;
            m_fSequenceDelay = 0.4f;
            break;
        case CSWPazaak::EAISTATE_DREW_CARD:
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[1]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            if (m_pPazaak->m_players[1].GetTotal() > PAZAAK_WINNING_TOTAL)
                g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[3]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            break;
        case CSWPazaak::EAISTATE_PLAYED_SIDE_CARD:
            m_actbtn2.m_fFlashTimer = 0.8f;
            m_actbtn2.m_labelPos = CGuiInGame::COLOR_RED;
            g_pExoSound->PlayOneShotSound(CResRef(m_apszCueSounds[2]), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            break;
        case CSWPazaak::EAISTATE_ENDED_TURN:
            m_actbtn0.m_fFlashTimer = 0.8f;
            m_actbtn0.m_labelPos = CGuiInGame::COLOR_RED;
            // fallthrough
        case CSWPazaak::EAISTATE_STOOD:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            RefreshDisplay();
            return 0;
        default:
            RefreshDisplay();
            return 0;
        }
        if (nResult != CSWPazaak::EAISTATE_START_TURN) {
            m_fSequenceDelay = 0.2f;
            if (m_pPazaak->m_players[1].GetTotal() == PAZAAK_WINNING_TOTAL && (m_anTutorialState[0] & TUTORIAL_SHOWN_EXACTLY_20) == 0) {
                m_anTutorialState[0] |= TUTORIAL_SHOWN_EXACTLY_20;
                m_tutorial.ShowHelp(STRREF_TUT_EXACTLY_20, static_cast<unsigned>(-1), STRREF_PAZAAK_CANCEL);
            }
            RefreshDisplay();
            return 0;
        }
        // EAISTATE_START_TURN means the AI finished the round; resolve it as GAME_STATE_SET_OVER does
    }
    case GAME_STATE_SET_OVER: {
        int nWinner = m_pPazaak->DetermineWinner();
        if (nWinner == CSWPazaak::WINNER_NONE) {
            m_nGameState = GAME_STATE_PLAYER_TURN;
            RefreshDisplay();
            return 0;
        }
        m_dialog.SetAllowCancel(0);
        if (nWinner == CSWPazaak::WINNER_OPPONENT) {
            m_pPazaak->m_players[1].m_nRoundsWon++;
            if (m_pPazaak->m_players[1].m_nRoundsWon >= 3) {
                m_dialog.SetMessage(STRREF_YOU_LOSE_MATCH);
                m_pszPendingSound = reinterpret_cast<unsigned long long>("mgs_losematch");
            } else {
                m_dialog.SetMessage(STRREF_OPPONENT_WINS_SET);
                m_pszPendingSound = reinterpret_cast<unsigned long long>("mgs_loseset");
            }
        } else if (nWinner == CSWPazaak::WINNER_PLAYER) {
#if !KOTOR1  // K1: no nine-card achievement
            if (m_pPazaak->m_players[0].GetNumCardsPlayed() == 9)
                Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_PAZAAK_NINE_CARDS);
#endif
            m_pPazaak->m_players[0].m_nRoundsWon++;
            if (m_pPazaak->m_players[0].m_nRoundsWon >= 3) {
                m_dialog.SetMessage(STRREF_YOU_WIN_MATCH);
                m_pszPendingSound = reinterpret_cast<unsigned long long>("mgs_winmatch");
            } else {
                m_dialog.SetMessage(STRREF_YOU_WIN_SET);
                m_pszPendingSound = reinterpret_cast<unsigned long long>("mgs_winset");
            }
        } else {
            m_dialog.SetMessage(STRREF_SET_TIED);
        }
        m_nGameState = GAME_STATE_SHOW_RESULT;
        RefreshDisplay();
        return 0;
    }
    case GAME_STATE_SHOW_RESULT:
        if (m_pszPendingSound != 0) {
            g_pExoSound->PlayOneShotSound(CResRef(reinterpret_cast<char*>(m_pszPendingSound)), PRIORITY_GROUP_GUI, 0, 0, 0.0f, 0.0f);
            m_pszPendingSound = 0;
        }
        m_dialog.SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        m_dialog.SetCallback(this,
                             static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakGame::HandleEndDialog), 0);
        m_pGuiManager->AddPanel(&m_dialog, 1, 1);
        m_nGameState = GAME_STATE_RESULT_SHOWN;
        RefreshDisplay();
        return 0;
    case GAME_STATE_RESULT_SHOWN:
        break;
    }
    return 0;
#endif
}










// True when the active control is one of the ten chosen cards.
bool CSWGuiPazaakStart::IsRightSideActive()
{
    int n = m_pActiveControl->m_tail.nControlId;
    return static_cast<unsigned int>(n) <= 9 ? m_pActiveControl == &m_aChosen[n] : false;
}

// The left-side counterpart.
bool CSWGuiPazaakStart::IsLeftSideActive()
{
    int n = m_pActiveControl->m_tail.nControlId;
#if KOTOR1
    if (static_cast<unsigned>(n) > 17)
        return false;
    return m_pActiveControl == &m_aAvail[n];
#else
    return static_cast<unsigned int>(n) <= 23 ? m_pActiveControl == &m_aAvail[n] : false;
#endif
}

// Records the wager when the wager dialog closes and puts focus back on the first Avail-card slot.
void CSWGuiPazaakStart::HandleWagerExit(int nWager)
{
    m_pPazaak->m_nWager = nWager;
    SetActiveControl(&m_aAvail[0], 0);
}

void CSWGuiPazaakStart::OnChosenCardEntered(CSWGuiControl* pControl)
{
#if KOTOR1
    K2_UNUSED(pControl);
#endif
#if !KOTOR1  // K1: no help line
    UpdateHelpText(pControl->m_tail.nControlId);
#endif
    ShowDisabledButtons();
}

namespace {
const char s_achBtnChosen[16]   = "BTN_CHOSENx    ";
const char s_achLblChosen[16]   = "LBL_CHOSENx    ";
const char s_achBtnAvail[16]    = "BTN_AVAILxx    ";
const char s_achLblAvail[16]    = "LBL_AVAILxx    ";
const char s_achLblAvailNum[16] = "LBL_AVAILNUMxx ";
}  // namespace

#ifdef K2_UI_XBOX
// Console setup screen. The PC layout has a Play button, a Clear Cards button with a floating
// icon, and d-pad links that run across the two card panes; the console layout has none of those
// and is driven by the face buttons and the bumpers instead.
namespace {
// Storage for the setup screen's legend controls, which the class has no member for: raw bytes and
// placement new, since static constructors do not run in this build. Only one setup screen exists
// at a time, and each one rebuilds its own legend.
enum { LEGEND_YTEXT, LEGEND_YBUTTON, LEGEND_ABUTTON, LEGEND_XBUTTON, LEGEND_LBUTTON, LEGEND_RBUTTON,
       LEGEND_COUNT };
alignas(16) unsigned char k2PazaakSetupLegendStore[LEGEND_COUNT * sizeof(CSWGuiLabel)];
int k2PazaakSetupLegendLive;

CSWGuiLabel* Legend() { return reinterpret_cast<CSWGuiLabel*>(k2PazaakSetupLegendStore); }

CSWGuiLabel* RebuildLegend() {
    CSWGuiLabel* p = Legend();
    if (k2PazaakSetupLegendLive)
        for (int i = 0; i < LEGEND_COUNT; i++)
            p[i].~CSWGuiLabel();
    for (int i = 0; i < LEGEND_COUNT; i++)
        new (&p[i]) CSWGuiLabel();
    k2PazaakSetupLegendLive = 1;
    return p;
}
}  // namespace
#endif

//
// Deck-builder screen. Builds the match state and the wager popup when one is at stake, preloads
// the opponent's side deck, then pulls the layout and binds every control by name. OnPanelAdded
// does the runtime half of the setup.
CSWGuiPazaakStart::CSWGuiPazaakStart(CSWGuiManager* pManager, int nWager, int nOpponentDeck,
                                     int bTutorialsEnabled, unsigned long long oidPCCreature)
    : CSWGuiPanel(pManager), m_dialog(pManager), m_tutorial(pManager) {
    m_pDragChosen = m_aChosen;
    m_pDragAvail  = m_aAvail;
    m_pWagerPopup = 0;
    m_nSideDeckComplete = 0;
    m_tutorial.m_bEnabled = bTutorialsEnabled;

#if KOTOR1
    for (int i = 0; i < 18; i++)
        m_anCopies[i] = 0;
#else
    for (int i = 0; i < 24; i++)
        m_anCopies[i] = 0;
#endif

    for (int i = 0; i < 10; i++) {
        m_aSidedeck[i].m_nCardType   = PAZAAK_CARD_NONE;
        m_aSidedeck[i].m_bIsNegative = 0;
#if !KOTOR1
        m_aSidedeck[i].m_nValue      = 0;
#endif
    }

    m_pPazaak = new CSWPazaak;
    m_pPazaak->m_nWager = nWager;
    m_pPazaak->m_oidOpponent = oidPCCreature;
    if (nWager >= 1) {
        m_pPazaak->m_nWager = 1;
        m_pWagerPopup = new CSWGuiWagerPopup(m_pGuiManager, this, nWager);
    }

    CPazaakCard anInitial[10];
    for (int i = 0; i < 10; i++) {
        anInitial[i].m_bIsNegative = 0;
#if !KOTOR1
        anInitial[i].m_nValue      = 0;
#endif
    }
    m_pPazaak->LoadInitialSideDeck(nOpponentDeck, anInitial);
    m_pPazaak->m_players[1].ChooseSidedeck(anInitial);

#ifdef K2_UI_XBOX
    // Console: pazaaksetup_x is a 640x480 composition, so the load and the binds run inside a
    // sized window.
    const int nSavedGUIWidth  = g_nGUIWidth;
    const int nSavedGUIHeight = g_nGUIHeight;
    g_nGUIWidth  = 640;
    g_nGUIHeight = 480;
    StartLoadFromLayout(CResRef("pazaaksetup_x"), 1, 1, 1);
#elif KOTOR1
    StartLoadFromLayout(CResRef("pazaaksetup"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("pazaaksetup_p"), 1, 1, 1);
#endif

#ifdef K2_UI_XBOX
    // In the console legend, the PC's Play and Clear Cards buttons become the A and X captions,
    // still tap targets with the PC handlers; the rest are labels.
    CSWGuiLabel* pLegend = RebuildLegend();
#endif
    CSWGuiPanel::InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
#ifdef K2_UI_XBOX
    CSWGuiPanel::InitControl(m_btnAccept, CExoString("LBL_ATEXT"), 1, 1, 0, 0, 0);
    CSWGuiPanel::InitControl(m_btnClearCards, CExoString("LBL_XTEXT"), 1, 1, 0, 0, 0);
#else
    CSWGuiPanel::InitControl(m_btnAccept, CExoString("BTN_ATEXT"), 1, 1, 0, 0, 0);
#if !KOTOR1
    CSWGuiPanel::InitControl(m_btnClearCards, CExoString("BTN_CLEARCARDS"), 1, 1, 0, 0, 0);
#elif defined(K1_UI_XBOX)
    // xbox: the move button takes the PC layout's slot between the panes, as the Y legend.
    CSWGuiPanel::InitControl(m_btnMoveCard, CExoString("BTN_YTEXT"), 1, 1, 0, 0, 0);
    IosAddButtonIcon(&m_btnMoveCard, 'y');
    IosAddButtonIcon(&m_btnAccept, 'a');
#endif
#endif
    CSWGuiPanel::InitControl(m_bLabels[0], CExoString("LBL_LTEXT"), 1, 1, 0, 0, 0);
    CSWGuiPanel::InitControl(m_lblRText, CExoString("LBL_RTEXT"), 1, 1, 0, 0, 0);
#ifdef K2_UI_XBOX
    CSWGuiPanel::InitControl(pLegend[LEGEND_YTEXT], CExoString("LBL_YTEXT"), 1, 1, 0, 0, 0);
    CSWGuiPanel::InitControl(pLegend[LEGEND_YBUTTON], CExoString("LBL_YBUTTON"), 1, 1, 0, 0, 0);
    CSWGuiPanel::InitControl(pLegend[LEGEND_ABUTTON], CExoString("LBL_ABUTTON"), 1, 1, 0, 0, 0);
    CSWGuiPanel::InitControl(pLegend[LEGEND_XBUTTON], CExoString("LBL_XBUTTON"), 1, 1, 0, 0, 0);
    CSWGuiPanel::InitControl(pLegend[LEGEND_LBUTTON], CExoString("LBL_LBUTTON"), 1, 1, 0, 0, 0);
    CSWGuiPanel::InitControl(pLegend[LEGEND_RBUTTON], CExoString("LBL_RBUTTON"), 1, 1, 0, 0, 0);
#endif

    // For the chosen-side slots, name the card and its caption per slot, then wire the
    // pick/drop/move events.
    char achBtn[16];
    __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achBtn, s_achBtnChosen, 16);
    char achLbl[16];
    __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achLbl, s_achLblChosen, 16);
#pragma clang loop unroll(disable)
    for (int n = 0; n < 10; n++) {
        FormatInto(achBtn + 10, 6, "%d", n);
        FormatInto(achLbl + 10, 6, "%d", n);
        CSWGuiPanel::InitControl(m_aChosen[n], achBtn, 1, 1, 0, 0, 0);
        CSWGuiPanel::InitControl(m_aChosen[n].m_label, achLbl, 1, 1, 0, 0, 0);
        m_aChosen[n].m_tail.nControlId = n;
        m_aChosen[n].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::OnChosenCardEntered));
        m_aChosen[n].AddEvent(SWGUIEVENT_CARD_DROPPED, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleChosenCardDrop));
        m_aChosen[n].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleMoveCard));
    }

    // In the avail-side grid, the card, its caption label and its count label take row/col-patched
    // names and get the same event trio with the avail handlers, plus the count label's highlight
    // bit.
    char achAvailBtn[16];
    __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achAvailBtn, s_achBtnAvail, 16);
    char achAvailLbl[16];
    __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achAvailLbl, s_achLblAvail, 16);
    char achNum[16];
    __builtin_memcpy(/* idiom-exempt: fixed 16-byte name template */achNum, s_achLblAvailNum, 16);
#pragma clang loop unroll(disable)
#if KOTOR1
    for (int n = 0; n < 18; n++) {
#else
    for (int n = 0; n < 24; n++) {
#endif
        FormatInto(achAvailBtn + 9, 7, "%d%d", n / 6, n % 6);
        FormatInto(achAvailLbl + 9, 7, "%d%d", n / 6, n % 6);
        FormatInto(achNum + 12, 4, "%d%d", n / 6, n % 6);
        CSWGuiPanel::InitControl(m_aAvail[n], achAvailBtn, 1, 1, 0, 0, 0);
        CSWGuiPanel::InitControl(m_aAvail[n].m_label, achAvailLbl, 1, 1, 0, 0, 0);
        CSWGuiPanel::InitControl(m_aLabels[n], achNum, 1, 1, 0, 0, 0);
        m_aAvail[n].m_tail.nControlId = n;
        m_aAvail[n].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::OnAvailCardEntered));
        m_aAvail[n].AddEvent(SWGUIEVENT_CARD_DROPPED, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleAvailCardDrop));
        m_aAvail[n].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleMoveCard));
        m_aLabels[n].m_tail.bIgnoreMouse = 1;
    }

#if !KOTOR1
    CSWGuiPanel::InitControl(m_bLabels[1], CExoString("LBL_HELP"), 1, 1, 0, 0, 0);
#endif
    StopLoadFromLayout();
#if KOTOR1  // K1: rescale to the window, then hang the two side panels beside the 4:3 screen
    {
#ifdef K2_UI_TOUCH
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
        float s_fScale = static_cast<float>(g_SDL_mode.h) / 624.0f;
#else
        static float s_fScale = static_cast<float>(g_SDL_mode.h) / 624.0f;
#endif
#else
        // pc / xbox: the 640x480 layout fills the height, between the side panels
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
        float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#else
        static float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#endif
#endif
        ScaleExtentForResolution(s_fScale);
        CSWGuiExtent extent;
        extent.w = static_cast<int>(ceilf(static_cast<float>(g_SDL_mode.h) * 204.0f / 512.0f));
        extent.h = g_SDL_mode.h;
        extent.x = static_cast<int>((static_cast<float>(g_SDL_mode.w) - ceilf(static_cast<float>(g_SDL_mode.h) * 4.0f / 3.0f)) * 0.5f -
                         static_cast<float>(extent.w));
        extent.y = 0;
        m_imgSideLeft.Initialize(extent, CSWGuiImageParams(CResRef("and_paz01_left"), 0, 0, 0.0f, SWGUIALIGN_MIDDLE | SWGUIALIGN_CENTER,
#ifdef K2_UI_TOUCH
                                                           1.0f, Vector(1.0f, 1.0f, 1.0f)));
#else
                                                           1.0f, K1PazaakSideColor("and_paz01_left")));
#endif
        extent.x = g_SDL_mode.w - extent.x - extent.w;
        m_imgSideRight.Initialize(extent, CSWGuiImageParams(CResRef("and_paz01_right"), 0, 0, 0.0f, SWGUIALIGN_MIDDLE | SWGUIALIGN_CENTER,
#ifdef K2_UI_TOUCH
                                                            1.0f, Vector(1.0f, 1.0f, 1.0f)));
#else
                                                            1.0f, K1PazaakSideColor("and_paz01_right")));
#endif
    }
#endif
#ifdef K2_UI_XBOX
    g_nGUIWidth  = nSavedGUIWidth;
    g_nGUIHeight = nSavedGUIHeight;
#endif
#ifdef K2_QOL_IMPROVEMENTS
    // Fit the title and the pane captions into their layout boxes; the two button captions fit
    // themselves and LBL_HELP is fitted by UpdateHelpText.
    K2QolFitText(m_lblTitle.m_text, m_lblTitle.m_text.m_extent.h);
    K2QolFitText(m_bLabels[0].m_text, m_bLabels[0].m_text.m_extent.h);
    K2QolFitText(m_lblRText.m_text, m_lblRText.m_text.m_extent.h);
#ifdef K2_UI_XBOX
    K2QolFitText(pLegend[LEGEND_YTEXT].m_text, pLegend[LEGEND_YTEXT].m_text.m_extent.h);
#endif
#endif

#if KOTOR1  // K1: the move button carries HandleMoveCard, and there is no clear-cards button
    m_btnMoveCard.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleMoveCard));
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleAcceptButton));
    m_btnMoveCard.m_tail.bFocusOnHover = 0;
#else
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleAcceptButton));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnClearCards.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::ClearSideDeck));
    m_btnClearCards.m_tail.bFocusOnHover = 0;
#endif
    m_nFullScreenBGIndex = 2;
}

#if KOTOR1
void CSWGuiPazaakStart::Draw(float f)
{
    AurGUIStartLayer();
    if (AurGUISetupViewport(0, 0, g_SDL_mode.w, g_SDL_mode.h, Vector(0.09f, 0.082f, 0.125f), false, 1.0f)) {
        m_imgSideLeft.Draw(f);
        m_imgSideRight.Draw(f);
        AurGUICloseViewport();
    }
    AurGUIStopLayer();
    CSWGuiPanel::Draw(f);
}
#endif

CSWGuiPazaakStart::~CSWGuiPazaakStart()
{
    if (m_pWagerPopup)
    {
        delete m_pWagerPopup;
        m_pWagerPopup = 0;
    }
    if (m_pPazaak)
    {
        delete m_pPazaak;
        m_pPazaak = 0;
    }
}

void CSWGuiPazaakStart::OnAvailCardEntered(CSWGuiControl* pControl)
{
#if KOTOR1
    K2_UNUSED(pControl);
#endif
#if !KOTOR1  // K1: no help line
    UpdateHelpText(pControl->m_tail.nControlId);
#endif
    ShowDisabledButtons();
}

// Tears down the wager popup once the shared query box confirms the quit, restores the panel
// chain's state, and hands the match's bookkeeping back to the app.
void CSWGuiPazaakStart::HandleQuitDialog(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;
    if (m_pGuiManager->PanelExists(m_pWagerPopup))
        m_pGuiManager->RemovePanel(m_pWagerPopup);
#if KOTOR1
    // K1: no panel chain, just state 4, and K1 passes 0 rather than the table's m_nWager as
    // EndPazaakGame's second argument.
    SetToBeDeleted(0);
    g_pAppManager->m_pClientExoApp->EndPazaakGame(0, 0, 0, m_pPazaak->m_oidOpponent);
#else
    SetToBeDeleted(0);
    g_pAppManager->m_pClientExoApp->EndPazaakGame(0, m_pPazaak->m_nWager, 0, m_pPazaak->m_oidOpponent);
#endif
}

// Handles a deck card dropped onto the grid. The drop target is the control under the mouse. Only a
// drop onto one of the ten Chosen slots is meaningful, swapping that slot's card out for the
// dragged one (id = m_tail.nControlId of the dropped control).
void CSWGuiPazaakStart::HandleAvailCardDrop(CSWGuiControl* pControl)
{
    CSWGuiControl* pOver = g_pGuiMan->m_pMouseOverControl;
    if (!pOver)
        return;
    int nSlot = pOver->m_tail.nControlId;
    if (static_cast<unsigned>(nSlot) > 9)
        return;
    if (pOver != &m_aChosen[nSlot])
        return;
    int nId = pControl->m_tail.nControlId;
    RemoveChosenCard(nSlot);
    AddChosenCard(nId, nSlot);
}

#if !KOTOR1
void CSWGuiPazaakStart::ClearSideDeck(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    for (int n = 0; n < 10; n++)
        RemoveChosenCard(n);
}
#endif

// Accept (build deck) confirms through the in-panel message box when one is armed, else through the
// game's shared query box; either way the box is added as a modal-ish panel.
void CSWGuiPazaakStart::HandleAcceptButton(CSWGuiControl* pControl)
{
    if (!pControl || !pControl->m_tail.nCurrentEventArg)
        return;
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    CSWGuiMessageBox* pMsg;
    if (m_nSideDeckComplete) {
        m_dialog.SetMessage(STRREF_CONFIRM_SIDEDECK);
        m_dialog.SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleStartDialog), 0);
        pMsg = &m_dialog;
    } else {
        pMsg = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pMsg->SetAllowCancel(0);
        pMsg->SetCallback(0, 0, 0);
        pMsg->SetMessage(STRREF_TUT_SIDEDECK_REQUIRED);
    }
#if !KOTOR1  // K1: the boxes keep their own colours
    pMsg->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
    m_pGuiManager->AddPanel(pMsg, 1, 1);
}

// Enables the accept button and tints it according to whether the side deck is complete.
void CSWGuiPazaakStart::ShowDisabledButtons()
{
    m_btnAccept.SetEnabled(m_nSideDeckComplete);
#if KOTOR1  // K1: the move button is live when the focused card can move
    if (m_pActiveControl && IsLeftSideActive())
        m_btnMoveCard.SetEnabled((m_nSideDeckComplete == 0) &
                                 (m_anCopies[m_pActiveControl->m_tail.nControlId] > 0));
    else if (m_pActiveControl && IsRightSideActive())
        m_btnMoveCard.SetEnabled(m_aSidedeck[m_pActiveControl->m_tail.nControlId].m_nCardType >= 0);
    else
        m_btnMoveCard.SetEnabled(0);
#else
    const Vector* pColor;
    if (m_nSideDeckComplete)
        pColor = &CGuiInGame::COLOR_PAZAAK_GOLD;
    else
        pColor = &CGuiInGame::COLOR_DARK_TAN;
    m_btnAccept.m_text.m_params.SetDefaultColor(*pColor);
    m_btnAccept.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(pColor);
#endif
}

// Handles a chosen card being dropped. Onto another chosen slot the two swap through the drag-row
// scratch (a -1 id means an empty side); onto the avail grid the dragged card just leaves the
// chosen side.
void CSWGuiPazaakStart::HandleChosenCardDrop(CSWGuiControl* pControl)
{
    CSWGuiControl* pOver = g_pGuiMan->m_pMouseOverControl;
    if (!pOver)
        return;
    int nOver = pOver->m_tail.nControlId;
    int nId = pControl->m_tail.nControlId;
    if (static_cast<unsigned>(nOver) <= 9 && pOver == &m_aChosen[nOver]) {
        int nIdCard = m_aSidedeck[nId].m_nCardType;
        RemoveChosenCard(nId);
        int nOverCard = m_aSidedeck[nOver].m_nCardType;
        RemoveChosenCard(nOver);
        if (nOverCard >= 0)
            AddChosenCard(nOverCard, nId);
        if (nIdCard >= 0)
            AddChosenCard(nIdCard, nOver);
#if KOTOR1
    } else if (static_cast<unsigned>(nOver) <= 17 && pOver == &m_aAvail[nOver]) {
#else
    } else if (static_cast<unsigned>(nOver) <= 23 && pOver == &m_aAvail[nOver]) {
#endif
        RemoveChosenCard(nId);
    }
}

// Empties side-deck slot nSlot and returns its card to the avail side, relabelling both tiles.
// The complete flag clears first, so the slot-state buttons grey out with one fewer card placed.
int CSWGuiPazaakStart::RemoveChosenCard(int nSlot)
{
    int nCard = m_aSidedeck[nSlot].m_nCardType;
    if (nCard < 0)
        return false;
    m_nSideDeckComplete = 0;
    ShowDisabledButtons();
#if KOTOR1
    CPazaakCard empty;
    empty.m_nCardType = PAZAAK_CARD_NONE;
    empty.m_bIsNegative = 0;
    m_aSidedeck[nSlot] = empty;
#else
    CPazaakCard& card = m_aSidedeck[nSlot];
    card.m_nCardType = PAZAAK_CARD_NONE;
    card.m_bIsNegative = 0;
    card.m_nValue = 0;
#endif
    ++m_anCopies[nCard];
    m_aLabels[nCard].m_text.m_params.SetText(CExoString(m_anCopies[nCard]));
    CSWGuiPazaakCard& rAvail = m_aAvail[nCard];
    rAvail.m_border.m_params.m_color = CSWGuiColor(1.0f, 1.0f, 1.0f);
    rAvail.m_bDragLatch = 1;
    CSWGuiPazaakCard& rChosen = m_aChosen[nSlot];
    rChosen.m_border.m_params.SetFillImage(CResRef(""), 0);
    rChosen.m_label.m_text.m_params.SetText(CExoString(""));
    return 1;
}
// Places nCard on chosen slot nSlotPref (0..9 taken verbatim, otherwise the first empty slot, none
// -> false) and relabels both tiles.
int CSWGuiPazaakStart::AddChosenCard(int nCard, int nSlotPref)
{
    if (m_anCopies[nCard] < 1)
        return false;
    int nSlot = nSlotPref;
    if (nSlot < 0) {
        if (m_aSidedeck[0].m_nCardType < 0)
            nSlot = 0;
        else if (m_aSidedeck[1].m_nCardType < 0)
            nSlot = 1;
        else if (m_aSidedeck[2].m_nCardType < 0)
            nSlot = 2;
        else if (m_aSidedeck[3].m_nCardType < 0)
            nSlot = 3;
        else if (m_aSidedeck[4].m_nCardType < 0)
            nSlot = 4;
        else if (m_aSidedeck[5].m_nCardType < 0)
            nSlot = 5;
        else if (m_aSidedeck[6].m_nCardType < 0)
            nSlot = 6;
        else if (m_aSidedeck[7].m_nCardType < 0)
            nSlot = 7;
        else if (m_aSidedeck[8].m_nCardType < 0)
            nSlot = 8;
        else if (m_aSidedeck[9].m_nCardType < 0)
            nSlot = 9;
        else
            return false;
    } else if (nSlot > 9) {
        return false;
    }
#if KOTOR1
    CPazaakCard added;
    added.m_nCardType = nCard;
    added.m_bIsNegative = 0;
    m_aSidedeck[nSlot] = added;
#else
    CPazaakCard& card = m_aSidedeck[nSlot];
    card.m_nCardType = nCard;
    card.m_bIsNegative = 0;
    card.m_nValue = 0;
#endif
    --m_anCopies[nCard];
    m_aLabels[nCard].m_text.m_params.SetText(CExoString(m_anCopies[nCard]));
    CSWGuiPazaakCard& rChosen = m_aChosen[nSlot];
    CSWGuiPazaakCard& rAvail = m_aAvail[nCard];
    CResRef cFillImage(rAvail.m_border.m_params.m_fillImage);
    rChosen.m_border.m_params.SetFillImage(cFillImage, 0);
    if (m_anCopies[nCard] == 0) {
        rAvail.m_border.m_params.m_color = CSWGuiColor(0.666999996f, 0.666999996f, 0.666999996f);
        rAvail.m_bDragLatch = 0;
    }
    CExoString sCaption(rAvail.m_label.m_text.m_params.m_sText);
    rChosen.m_label.m_text.m_params.SetText(sCaption);
    m_nSideDeckComplete = 1;
    int i;
    for (i = 0; i < 10; i++) {
        if (m_aSidedeck[i].m_nCardType < 0)
            break;
    }
    if (i < 10)
        m_nSideDeckComplete = 0;
    ShowDisabledButtons();
    return 1;
}

// Handles a card tap. Tapping a chosen card returns it to the avail side, tapping an avail card
// places it on the first empty chosen slot. The tapped card must also be the panel's active
// control; once every chosen slot is occupied again the accept button takes active-control duty.
void CSWGuiPazaakStart::HandleMoveCard(CSWGuiControl* pControl)
{
    if (!pControl || !pControl->m_tail.nCurrentEventArg)
        return;
    int nSlot = pControl->m_tail.nControlId;
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    CSWGuiControl* pDrag = m_pActiveControl;
    int nDrag = pDrag->m_tail.nControlId;
    if (static_cast<unsigned>(nDrag) <= 9 && pDrag == &m_aChosen[nDrag] && &m_aChosen[nSlot] == pDrag) {
        if (RemoveChosenCard(nSlot))
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
#if KOTOR1
    } else if (static_cast<unsigned>(nDrag) <= 17 && pDrag == &m_aAvail[nDrag] && &m_aAvail[nSlot] == pDrag) {
#else
    } else if (static_cast<unsigned>(nDrag) <= 23 && pDrag == &m_aAvail[nDrag] && &m_aAvail[nSlot] == pDrag) {
#endif
        if (AddChosenCard(nSlot, -1)) {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            ShowDisabledButtons();
            for (int i = 0; i < 10; i++)
                if (m_aSidedeck[i].m_nCardType < 0)
                    return;
            SetActiveControl(&m_btnAccept, 0);
            return;
        }
    }
    ShowDisabledButtons();
}

// Per-slot help strrefs for avail slots 18..22, in the engine's own permuted order.
#if !KOTOR1
static const int s_anHelpStrrefs[5] = {STRREF_HELP_TIEBREAKER_CARD, STRREF_HELP_DOUBLE_CARD, STRREF_HELP_FLIP_2_4_CARD,
                                        STRREF_HELP_FLIP_3_6_CARD, STRREF_HELP_PLUS_MINUS_1_2_CARD};
#endif

// Pumps the panel's input events. A nonzero nVal gates everything, and the switch handles the
// panel's private messages, which move focus between the two card panes.
void CSWGuiPazaakStart::HandleInputEvent(int nType, int nVal)
{
#ifdef K2_UI_XBOX
    // Console: A and Start return without the base call, which would otherwise route the click to
    // the focused card and move it.
    if (nVal != 0) {
        CSWGuiControl* pActive = m_pActiveControl;
        const int nId = pActive ? pActive->m_tail.nControlId : -1;
        const bool bChosen = pActive && static_cast<unsigned>(nId) <= 9 && pActive == &m_aChosen[nId];
        const bool bAvail  = pActive && static_cast<unsigned>(nId) <= 23 && pActive == &m_aAvail[nId];
        switch (nType) {
        case INPUTEVENT_GUI_A:
        case INPUTEVENT_GUI_START: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            CSWGuiMessageBox* pMsg;
            if (m_nSideDeckComplete) {
                m_dialog.SetMessage(STRREF_CONFIRM_SIDEDECK);
                m_dialog.SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleStartDialog), 0);
                pMsg = &m_dialog;
            } else {
                pMsg = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
                pMsg->SetAllowCancel(0);
                pMsg->SetCallback(0, 0, 0);
                pMsg->SetMessage(STRREF_TUT_SIDEDECK_REQUIRED);
            }
            pMsg->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
            m_pGuiManager->AddPanel(pMsg, 1, 1);
            return;
        }
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pBox->SetAllowCancel(1);
            pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleQuitDialog), 0);
            pBox->SetMessage(STRREF_QUIT_IS_FORFEIT);
            pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
            m_pGuiManager->AddPanel(pBox, 1, 1);
            break;
        }
        case INPUTEVENT_GUI_X:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            ClearSideDeck(0);
            break;
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (bAvail) {
                if (AddChosenCard(nId, -1))
                    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            } else if (bChosen) {
                if (RemoveChosenCard(nId))
                    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            }
            ShowDisabledButtons();
            if (bAvail || bChosen)
                UpdateHelpText(nId);
            break;
        case INPUTEVENT_GUI_LEFT_SHOULDER:  // L: to the Available pane
            if (bChosen) {
                m_pDragChosen = static_cast<CSWGuiPazaakCard*>(pActive);
                SetActiveControl(m_pDragAvail, 0);
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                UpdateHelpText(m_pDragAvail->m_tail.nControlId);
            }
            break;
        case INPUTEVENT_GUI_RIGHT_SHOULDER:  // R: to the Chosen pane
            if (bAvail) {
                m_pDragAvail = static_cast<CSWGuiPazaakCard*>(pActive);
                SetActiveControl(m_pDragChosen, 0);
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                UpdateHelpText(m_pDragChosen->m_tail.nControlId);
            }
            break;
        }
    }
#else
    if (nVal != 0) {
        unsigned nMsg = nType;
        if (nType == INPUTEVENT_KEY_SPACE) {
            CSWGuiControl* pActive = m_pActiveControl;
            int nId = pActive->m_tail.nControlId;
            nMsg = (static_cast<unsigned>(nId) > 9 || pActive != &m_aChosen[nId]) ? INPUTEVENT_GUI_RIGHT_SHOULDER : INPUTEVENT_GUI_LEFT_SHOULDER;
        }
        switch (nMsg) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pBox->SetAllowCancel(1);
            pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleQuitDialog), 0);
            pBox->SetMessage(STRREF_QUIT_IS_FORFEIT);
#if !KOTOR1  // K1: the box keeps its own colours
            pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
            m_pGuiManager->AddPanel(pBox, 1, 1);
        } break;
#if KOTOR1  // K1: the third button moves the focused card
        case INPUTEVENT_GUI_Y:
            HandleMoveCard(m_pActiveControl);
            break;
#else
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            ClearSideDeck(0);
            break;
#endif
        // The two bumper events are two different list switches: each remembers the focused tile of
        // one list and moves focus to the remembered tile of the other, skipping the move when the
        // focus is already on that list.
        case INPUTEVENT_GUI_LEFT_SHOULDER: {
            CSWGuiControl* pActive = m_pActiveControl;
            int nId = pActive->m_tail.nControlId;
            if (static_cast<unsigned>(nId) <= 9 && pActive == &m_aChosen[nId])
                m_pDragChosen = static_cast<CSWGuiPazaakCard*>(pActive);
#if KOTOR1  // K1: the move button's caption follows the list
            if (static_cast<unsigned>(nId) > 17 || pActive != &m_aAvail[nId]) {
                SetActiveControl(static_cast<CSWGuiControl*>(m_pDragAvail), 0);
                m_btnMoveCard.m_text.m_params.SetStrRef(STRREF_ADD_CARD);
#ifdef K1_UI_XBOX
                IosAddButtonIcon(&m_btnMoveCard, 'y');
#endif
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            }
#else
            if (static_cast<unsigned>(nId) > 23 || pActive != &m_aAvail[nId]) {
                SetActiveControl(static_cast<CSWGuiControl*>(m_pDragAvail), 0);
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            }
#endif
        } break;
        case INPUTEVENT_GUI_RIGHT_SHOULDER: {
            CSWGuiControl* pActive = m_pActiveControl;
            int nId = pActive->m_tail.nControlId;
#if KOTOR1
            if (static_cast<unsigned>(nId) <= 17 && pActive == &m_aAvail[nId])
                m_pDragAvail = static_cast<CSWGuiPazaakCard*>(pActive);
            if (static_cast<unsigned>(nId) > 9 || pActive != &m_aChosen[nId]) {
                SetActiveControl(static_cast<CSWGuiControl*>(m_pDragChosen), 0);
                m_btnMoveCard.m_text.m_params.SetStrRef(STRREF_REMOVE_CARD);
#ifdef K1_UI_XBOX
                IosAddButtonIcon(&m_btnMoveCard, 'y');
#endif
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            }
#else
            if (static_cast<unsigned>(nId) <= 23 && pActive == &m_aAvail[nId])
                m_pDragAvail = static_cast<CSWGuiPazaakCard*>(pActive);
            if (static_cast<unsigned>(nId) > 9 || pActive != &m_aChosen[nId]) {
                SetActiveControl(static_cast<CSWGuiControl*>(m_pDragChosen), 0);
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            }
#endif
        } break;
        }
    }
#endif
    CSWGuiPanel::HandleInputEvent(nType, nVal);
}

// Accepts the start dialog: copies the assembled side deck into the party table (the stored per-PC
// deck snapshot), deals 4 of the 10 cards into player 0's hand, hides this panel chain (state 4),
// then swaps itself for the live CSWGuiPazaakGame panel, which takes over the m_pPazaak ownership
// (the field is nulled before AddPanel).
void CSWGuiPazaakStart::HandleStartDialog(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!m_dialog.m_bAccepted)
        return;

    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    for (int i = 0; i < 10; i++)
        pPartyTable->m_pazaakSideDeck[i] = m_aSidedeck[i].m_nCardType;

    m_pPazaak->m_players[0].ChooseSidedeck(m_aSidedeck);
    m_pGuiManager->PopModalPanel();
    SetToBeDeleted(0);

    CSWGuiPazaakGame* pGame = new CSWGuiPazaakGame(m_pGuiManager, m_pPazaak, m_tutorial.m_bEnabled,
                                                  m_pPazaak->m_oidOpponent);
#if !KOTOR1
    pGame->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
    m_pPazaak = 0;
    m_pGuiManager->AddPanel(pGame, 3, 1);
}

#if !KOTOR1
// The bottom help line mirrors the hovered card: the focused slot's help or tutorial strref, or
// blank for a chosen card or an exhausted avail slot.
void CSWGuiPazaakStart::UpdateHelpText(int nId)
{
    int nStr;
    unsigned u = nId;
    if (nId == -1)
        nStr = -1;
    else if (u < PAZAAK_CARD_MINUS_1)
        nStr = STRREF_HELP_PLUS_CARD;
    else if (u - PAZAAK_CARD_MINUS_1 < 6)
        nStr = STRREF_HELP_MINUS_CARD;
    else if (u - PAZAAK_CARD_PLUS_MINUS_1 < 6)
        nStr = STRREF_HELP_FLIP_CARD;
    else if (u - PAZAAK_CARD_TIEBREAKER < 5)
        nStr = s_anHelpStrrefs[nId - PAZAAK_CARD_TIEBREAKER];
    else
        nStr = -1;
    CSWGuiControl* pActive = m_pActiveControl;
    int nActive = pActive->m_tail.nControlId;
#ifdef K2_FIX_ENGINE_BUGS
    if (static_cast<unsigned>(nActive) <= 23 && pActive == &m_aAvail[nActive] &&
        static_cast<unsigned>(nId) < sizeof(m_anCopies) / sizeof(m_anCopies[0]) && m_anCopies[nId] == -1)
#else
    if (static_cast<unsigned>(nActive) <= 23 && pActive == &m_aAvail[nActive] && m_anCopies[nId] == -1)
#endif
        nStr = -1;
    CSWGuiTextParams& rHelp = m_bLabels[1].m_text.m_params;
    if ((static_cast<unsigned>(nActive) <= 9 && pActive == &m_aChosen[nActive]) || nStr == -1) {
        rHelp.SetText(CExoString(""));
    } else {
        rHelp.SetStrRef(nStr);
    }
#ifdef K2_QOL_IMPROVEMENTS
    // Fit the card description into LBL_HELP's box.
    K2QolFitText(m_bLabels[1].m_text, m_bLabels[1].m_text.m_extent.h);
#endif
#ifdef K2_UI_XBOX
    // Console: the Y caption follows the focused pane (the Add card / Remove card strrefs).
    if (k2PazaakSetupLegendLive) {
        const bool bChosen = static_cast<unsigned>(nActive) <= 9 && pActive == &m_aChosen[nActive];
        Legend()[LEGEND_YTEXT].m_text.m_params.SetStrRef(bChosen ? STRREF_REMOVE_CARD : STRREF_ADD_CARD);
#ifdef K2_QOL_IMPROVEMENTS
        CSWGuiText& rYText = Legend()[LEGEND_YTEXT].m_text;
        K2QolFitText(rYText, rYText.m_extent.h);
#endif
    }
#endif
}
#endif

// Sets the panel up: replays the wager popup, snapshots the party's card counts into m_anCopies (a
// zero-count type becomes the "unavailable" sentinel with a blanked avail tile), rebuilds the
// side-deck rows from the saved per-PC deck, mirrors the counts into the avail captions, then wires
// the navigation graph and adds the quit-button icon.
void CSWGuiPazaakStart::OnPanelAdded()
{
    if (m_pWagerPopup) {
        SetActiveControl(0, 0);
        m_pGuiManager->AddPanel(m_pWagerPopup, 1, 1);
    }

    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();

#if KOTOR1
    for (int i = 0; i < 18; i++) {
        m_anCopies[i] = pPartyTable->m_pazaakCards[i];
        if (m_anCopies[i] == 0) {
            m_anCopies[i] = -1;
            CPazaakCard empty;
            empty.m_nCardType = PAZAAK_CARD_NONE;
            empty.m_bIsNegative = 0;
            m_aAvail[i].SetCard(empty);
        }
    }
#else
#pragma clang loop unroll(disable)
    for (int i = 0; i < 24; i++) {
        m_anCopies[i] = pPartyTable->m_pazaakCards[i];
        if (m_anCopies[i] == 0) {
            m_anCopies[i] = -1;
            m_aAvail[i].SetCard(CPazaakCard{PAZAAK_CARD_NONE, 0, 0});
        }
    }
#endif

    m_nSideDeckComplete = 1;

#if !KOTOR1  // K1 unrolls it
#pragma clang loop unroll(disable)
#endif
    for (int nSlot = 0; nSlot < 10; nSlot++) {
        CPazaakCard& card = m_aSidedeck[nSlot];
#if KOTOR1  // K1: whole-card copies
        CPazaakCard saved;
        saved.m_nCardType = pPartyTable->m_pazaakSideDeck[nSlot];
        saved.m_bIsNegative = 0;
        card = saved;
        if (card.m_nCardType >= 0 && m_anCopies[card.m_nCardType] > 0) {
            --m_anCopies[card.m_nCardType];
        } else {
            CPazaakCard none;
            none.m_nCardType = PAZAAK_CARD_NONE;
            none.m_bIsNegative = 0;
            card = none;
        }
#else
        card.m_nCardType = pPartyTable->m_pazaakSideDeck[nSlot];
        card.m_bIsNegative = 0;
        card.m_nValue = 0;
        if (card.m_nCardType >= 0 && m_anCopies[card.m_nCardType] > 0) {
            --m_anCopies[card.m_nCardType];
        } else {
            card.m_nCardType = PAZAAK_CARD_NONE;
            card.m_bIsNegative = 0;
            card.m_nValue = 0;
        }
#endif
        m_aChosen[nSlot].SetCard(m_aSidedeck[nSlot]);
        if (m_aSidedeck[nSlot].m_nCardType < 0)
            m_nSideDeckComplete = 0;
    }

#if KOTOR1  // K1: an unavailable card's count is left blank
    for (int i = 0; i < 18; i++) {
        if (m_anCopies[i] < 0)
            continue;
        m_aLabels[i].m_text.m_params.SetText(CExoString(m_anCopies[i]));
        if (m_anCopies[i] == 0) {
#else
#pragma clang loop unroll(disable)
    for (int i = 0; i < 24; i++) {
        if (m_anCopies[i] < 0)
            m_aLabels[i].m_text.m_params.SetText(CExoString("0"));
        else
            m_aLabels[i].m_text.m_params.SetText(CExoString(m_anCopies[i]));
        if (m_anCopies[i] == 0) {
#endif
            m_aAvail[i].m_border.m_params.m_color = CSWGuiColor(0.667f, 0.667f, 0.667f);
            m_aAvail[i].m_bDragLatch = 0;
        }
    }

#pragma clang loop unroll(disable)
    for (int n = 1; n < 11; n++) {
        m_aChosen[n - 1].SetMoveToControl(1, &m_aChosen[(n + 8) % 10]);
        m_aChosen[n - 1].SetMoveToControl(3, &m_aChosen[n % 10]);
        m_aChosen[n - 1].SetMoveToControl(0, &m_aChosen[(n + 7) % 10]);
        m_aChosen[n - 1].SetMoveToControl(2, &m_aChosen[(n + 1) % 10]);
    }

#if KOTOR1  // K1: a full 6x3 grid, wrapping both ways
    for (int e = 0; e < 18; e++) {
        m_aAvail[e].SetMoveToControl(1, &m_aAvail[(e + 12) % 18]);
        m_aAvail[e].SetMoveToControl(3, &m_aAvail[(e + 6) % 18]);
        m_aAvail[e].SetMoveToControl(0, &m_aAvail[e - e % 6 + (e + 5) % 6]);
        m_aAvail[e].SetMoveToControl(2, &m_aAvail[e - e % 6 + (e + 1) % 6]);
    }
#else
#pragma clang loop unroll(disable)
    for (int e = 0; e < 24; e++) {
        m_aAvail[e].SetMoveToControl(0, &m_aAvail[(e + 18) % 24]);
        m_aAvail[e].SetMoveToControl(2, &m_aAvail[(e + 6) % 24]);
        m_aAvail[e].SetMoveToControl(1, &m_aAvail[e - e % 6 + (e + 5) % 6]);
        m_aAvail[e].SetMoveToControl(3, &m_aAvail[e - e % 6 + (e + 1) % 6]);
    }

    m_aAvail[23].m_tail.bVisible = 0;
    m_aAvail[23].m_label.m_tail.bVisible = 0;
    m_aLabels[23].m_tail.bVisible = 0;

    m_aAvail[17].SetMoveToControl(2, &m_aAvail[5]);
    m_aAvail[5].SetMoveToControl(0, &m_aAvail[17]);
    m_aAvail[22].SetMoveToControl(3, &m_aAvail[18]);
    m_aAvail[18].SetMoveToControl(1, &m_aAvail[22]);
#endif

    if (!m_pWagerPopup)
        SetActiveControl(&m_aAvail[0], 0);

    ShowDisabledButtons();
    const uint64_t anHelpIds[3] = {STRREF_TUT_SIDEDECK_INTRO, STRREF_TUT_BASIC_DECK, STRREF_TUT_SELECT_SIDEDECK};
    m_tutorial.ShowMultipleHelp(anHelpIds, 3);
    CSWGuiPanel::OnPanelAdded();

#ifdef K2_UI_XBOX
    // Console: no cross-pane or Play links and no Aspyr icon; the d-pad wraps inside each pane and
    // each pane starts remembering its first card.
    m_pDragAvail  = &m_aAvail[0];
    m_pDragChosen = &m_aChosen[0];
    if (m_pActiveControl)
        UpdateHelpText(m_pActiveControl->m_tail.nControlId);
#elif KOTOR1  // K1: the chosen column sits beside the grid's rows
    m_aChosen[0].SetMoveToControl(1, &m_aAvail[12]);
    m_aChosen[1].SetMoveToControl(3, &m_aAvail[0]);
    m_aAvail[12].SetMoveToControl(3, &m_aChosen[0]);
    m_aAvail[0].SetMoveToControl(1, &m_aChosen[1]);
    m_aChosen[2].SetMoveToControl(1, &m_aAvail[13]);
    m_aChosen[3].SetMoveToControl(3, &m_aAvail[1]);
    m_aAvail[13].SetMoveToControl(3, &m_aChosen[2]);
    m_aAvail[1].SetMoveToControl(1, &m_aChosen[3]);
    m_aChosen[4].SetMoveToControl(1, &m_aAvail[14]);
    m_aChosen[5].SetMoveToControl(3, &m_aAvail[2]);
    m_aAvail[14].SetMoveToControl(3, &m_aChosen[4]);
    m_aAvail[2].SetMoveToControl(1, &m_aChosen[5]);
    m_aChosen[6].SetMoveToControl(1, &m_aAvail[15]);
    m_aChosen[7].SetMoveToControl(3, &m_aAvail[3]);
    m_aAvail[15].SetMoveToControl(3, &m_aChosen[6]);
    m_aAvail[3].SetMoveToControl(1, &m_aChosen[7]);
    m_aChosen[8].SetMoveToControl(1, &m_aAvail[16]);
    m_aChosen[9].SetMoveToControl(3, &m_aAvail[4]);
    m_aAvail[16].SetMoveToControl(3, &m_aChosen[8]);
    m_aAvail[4].SetMoveToControl(1, &m_aChosen[9]);
    m_aAvail[17].SetMoveToControl(3, &m_aChosen[8]);
    m_aAvail[5].SetMoveToControl(1, &m_aChosen[9]);
    m_aAvail[5].SetMoveToControl(2, &m_btnAccept);
    m_aAvail[0].SetMoveToControl(0, &m_btnAccept);
    m_aAvail[11].SetMoveToControl(2, &m_btnAccept);
    m_aAvail[6].SetMoveToControl(0, &m_btnAccept);
    m_aAvail[17].SetMoveToControl(2, &m_btnAccept);
    m_aAvail[12].SetMoveToControl(0, &m_btnAccept);
    m_aChosen[0].SetMoveToControl(0, &m_btnAccept);
    m_aChosen[8].SetMoveToControl(2, &m_btnAccept);
    m_aChosen[1].SetMoveToControl(0, &m_btnAccept);
    m_aChosen[9].SetMoveToControl(2, &m_btnAccept);
    m_btnAccept.SetMoveToControl(2, &m_aAvail[0]);
    m_btnAccept.SetMoveToControl(0, &m_aAvail[5]);
#else
    m_aAvail[0].SetMoveToControl(1, &m_aChosen[1]);
    m_aChosen[1].SetMoveToControl(3, &m_aAvail[0]);
    m_aAvail[5].SetMoveToControl(3, &m_aChosen[0]);
    m_aChosen[0].SetMoveToControl(1, &m_aAvail[5]);
    m_aAvail[6].SetMoveToControl(1, &m_aChosen[3]);
    m_aChosen[3].SetMoveToControl(3, &m_aAvail[6]);
    m_aAvail[11].SetMoveToControl(3, &m_aChosen[2]);
    m_aChosen[2].SetMoveToControl(1, &m_aAvail[11]);
    m_aAvail[12].SetMoveToControl(1, &m_aChosen[5]);
    m_aChosen[5].SetMoveToControl(3, &m_aAvail[12]);
    m_aAvail[17].SetMoveToControl(3, &m_aChosen[4]);
    m_aChosen[4].SetMoveToControl(1, &m_aAvail[17]);
    m_aAvail[18].SetMoveToControl(1, &m_aChosen[7]);
    m_aChosen[7].SetMoveToControl(3, &m_aAvail[18]);
    m_aAvail[22].SetMoveToControl(3, &m_aChosen[6]);
    m_aChosen[6].SetMoveToControl(1, &m_aAvail[22]);
    m_aAvail[0].SetMoveToControl(0, &m_btnAccept);
    m_aAvail[22].SetMoveToControl(2, &m_btnAccept);
    m_aAvail[1].SetMoveToControl(0, &m_btnAccept);
    m_aAvail[21].SetMoveToControl(2, &m_btnAccept);
    m_aAvail[2].SetMoveToControl(0, &m_btnAccept);
    m_aAvail[20].SetMoveToControl(2, &m_btnAccept);
    m_aAvail[3].SetMoveToControl(0, &m_btnAccept);
    m_aAvail[19].SetMoveToControl(2, &m_btnAccept);
    m_aAvail[4].SetMoveToControl(0, &m_btnAccept);
    m_aAvail[18].SetMoveToControl(2, &m_btnAccept);
    m_aAvail[5].SetMoveToControl(0, &m_btnAccept);
    m_aChosen[0].SetMoveToControl(0, &m_btnAccept);
    m_aChosen[8].SetMoveToControl(2, &m_btnAccept);
    m_aChosen[1].SetMoveToControl(0, &m_btnAccept);
    m_aChosen[9].SetMoveToControl(2, &m_btnAccept);
    m_aChosen[8].SetMoveToControl(1, &m_aChosen[9]);
    m_aChosen[9].SetMoveToControl(3, &m_aChosen[8]);
    m_btnAccept.SetMoveToControl(2, &m_aAvail[0]);
    m_btnAccept.SetMoveToControl(0, &m_aAvail[22]);

    AspyrAddButtonIcon(&m_btnClearCards, 'y');
#endif
}



// The on-card caption strings, indexed by card type. The first six flippable types are read at
// [type-6], the second six at [type-12], and the rest at [type]. The replacement glyphs reproduce
// the bytes the shipped data actually carries rather than the intended codepoints. The K1 table has
// 28 entries and spells the sign as a Latin-1 byte.
#if KOTOR1  // K1: 28 card types, with the +/- glyph as Latin-1
static const char* const s_pszCardLabels[] = {
    "+1", "+2", "+3", "+4", "+5", "+6",
    "-1", "-2", "-3", "-4", "-5", "-6",
    "\xb1" "1", "\xb1" "2", "\xb1" "3", "\xb1" "4", "\xb1" "5", "\xb1" "6",
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"
};
#else
static const char* const s_pszCardLabels[] = {
    "+1", "+2", "+3", "+4", "+5", "+6",
    "-1", "-2", "-3", "-4", "-5", "-6",
    "\xef\xbf\xbd" "1", "\xef\xbf\xbd" "2", "\xef\xbf\xbd" "3", "\xef\xbf\xbd" "4",
    "\xef\xbf\xbd" "5", "\xef\xbf\xbd" "6",
    "\xef\xbf\xbd" "1T", "D", "2|4", "3|6", "\xef\xbf\xbd" "1|2",
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"
};
#endif

CSWGuiPazaakCard::~CSWGuiPazaakCard() {
    if (g_pGuiMan->m_pDragDropControl == this)
        g_pGuiMan->m_pDragDropControl = 0;
}

void CSWGuiPazaakCard::Load(CResGFF* pGFF, CResStruct& resStruct) {
    CSWGuiButton::Load(pGFF, resStruct);
    // The highlight border's "pulsing" is turned on for cards.
    m_borderHilight.m_params.SetPulsingAlpha(1, 0, 1);
}

void CSWGuiPazaakCard::SetCard(CPazaakCard card) {
    m_card = card;
#if KOTOR1  // K1: no value cards, caption by type
    if (static_cast<unsigned int>(card.m_nCardType) <= PAZAAK_CARD_MAIN_10) {
        if (static_cast<unsigned int>(card.m_nCardType - PAZAAK_CARD_PLUS_MINUS_1) <= 5u) {
            CSWGuiTextParams* pParams = &m_label.m_text.m_params;
            if (card.m_bIsNegative) {
                CExoString s(s_pszCardLabels[card.m_nCardType - 6]);
                pParams->SetText(s);
            } else {
                CExoString s(s_pszCardLabels[card.m_nCardType - PAZAAK_CARD_PLUS_MINUS_1]);
                pParams->SetText(s);
            }
        } else {
            CExoString s(s_pszCardLabels[card.m_nCardType]);
            m_label.m_text.m_params.SetText(s);
        }
    } else {
        CExoString s = "";
        m_label.m_text.m_params.SetText(s);
    }

    CResRef res;
    switch (card.m_nCardType + 1) {
        case PAZAAK_CARD_NONE + 1:
            res = const_cast<char*>("");
            break;
        case PAZAAK_CARD_PLUS_1 + 1: case PAZAAK_CARD_PLUS_2 + 1: case PAZAAK_CARD_PLUS_3 + 1:
        case PAZAAK_CARD_PLUS_4 + 1: case PAZAAK_CARD_PLUS_5 + 1: case PAZAAK_CARD_PLUS_6 + 1:
            res = const_cast<char*>("lbl_cardmpos");
            break;
        case PAZAAK_CARD_MINUS_1 + 1: case PAZAAK_CARD_MINUS_2 + 1: case PAZAAK_CARD_MINUS_3 + 1:
        case PAZAAK_CARD_MINUS_4 + 1: case PAZAAK_CARD_MINUS_5 + 1: case PAZAAK_CARD_MINUS_6 + 1:
            res = const_cast<char*>("lbl_cardmneg");
            break;
        case PAZAAK_CARD_PLUS_MINUS_1 + 1: case PAZAAK_CARD_PLUS_MINUS_2 + 1: case PAZAAK_CARD_PLUS_MINUS_3 + 1:
        case PAZAAK_CARD_PLUS_MINUS_4 + 1: case PAZAAK_CARD_PLUS_MINUS_5 + 1: case PAZAAK_CARD_PLUS_MINUS_6 + 1:
            res = card.m_bIsNegative ? const_cast<char*>("lbl_cardraref") : const_cast<char*>("lbl_cardrarem");
            break;
        case PAZAAK_CARD_MAIN_1 + 1: case PAZAAK_CARD_MAIN_2 + 1: case PAZAAK_CARD_MAIN_3 + 1:
        case PAZAAK_CARD_MAIN_4 + 1: case PAZAAK_CARD_MAIN_5 + 1: case PAZAAK_CARD_MAIN_6 + 1:
        case PAZAAK_CARD_MAIN_7 + 1: case PAZAAK_CARD_MAIN_8 + 1: case PAZAAK_CARD_MAIN_9 + 1:
        case PAZAAK_CARD_MAIN_10 + 1:
            res = const_cast<char*>("lbl_cardstand");
            break;
        default:
            res = const_cast<char*>("lbl_cardback");
            break;
    }
    m_border.m_params.SetFillImage(res, 0);
#else

    if (static_cast<unsigned int>(card.m_nCardType) <= PAZAAK_CARD_MAIN_10) {
        if (static_cast<unsigned int>(card.m_nCardType - PAZAAK_CARD_TIEBREAKER) <= 4u) {
            // Types 18..22 are the five special display rules. These captions are not localized.
            CExoString s;
            int v;
            switch (card.m_nCardType) {
                case PAZAAK_CARD_TIEBREAKER:  // flips sign
                    if (card.m_bIsNegative)
                        s = "-1T";
                    else
                        s = "+1T";
                    break;
                case PAZAAK_CARD_DOUBLE:  // blank ("D") unless a value was copied
                    if (card.GetCardValue())
                        s.Format(const_cast<char*>("%d"), card.GetCardValue());
                    else
                        s = "D";
                    break;
                case PAZAAK_CARD_FLIP_2_4:  // "flip 2&4"  (the two captions are reversed from their names)
                    if (card.m_nValue)
                        s = "0";
                    else
                        s = "2&4";
                    break;
                case PAZAAK_CARD_FLIP_3_6:  // "flip 3&6"
                    if (card.m_nValue)
                        s = "0";
                    else
                        s = "3&6";
                    break;
                case PAZAAK_CARD_PLUS_MINUS_1_2:  // ±1/±2: ceil(|value|) to at least 1, apply the sign
                    // inlined CPazaakCard::GetCardValue()
                    v = card.m_nValue ? card.m_nValue : 1;
                    s.Format(const_cast<char*>("%d"), card.m_bIsNegative ? -v : v);
                    break;
            }
            m_label.m_text.m_params.SetText(s);
        } else if (card.m_nValue != 0) {
            // Types 23..32 with a value set show the number itself.
            CExoString s;
            s.Format(const_cast<char*>("%d"), card.m_nValue);
            m_label.m_text.m_params.SetText(s);
        } else if (static_cast<unsigned int>(card.m_nCardType - PAZAAK_CARD_PLUS_MINUS_1) <= 5u) {
            // Flippable cards with no value use the static list, sign-adjusted.
            CSWGuiTextParams* pParams = &m_label.m_text.m_params;
            if (card.m_bIsNegative) {
                CExoString s(s_pszCardLabels[card.m_nCardType - 6]);
                pParams->SetText(s);
            } else {
                CExoString s(s_pszCardLabels[card.m_nCardType - PAZAAK_CARD_PLUS_MINUS_1]);
                pParams->SetText(s);
            }
        } else {
            // Everything else with no value gets the type's own plain label.
            CExoString s(s_pszCardLabels[card.m_nCardType]);
            m_label.m_text.m_params.SetText(s);
        }
    } else {
        // Beyond the last type, and the -1 empty sentinel, get a blank caption.
        CExoString s = "";
        m_label.m_text.m_params.SetText(s);
    }

    // The border key is (type + 1).
    CResRef res;
    switch (card.m_nCardType + 1) {
        case PAZAAK_CARD_NONE + 1:
            res = const_cast<char*>("");
            break;
        case PAZAAK_CARD_PLUS_1 + 1: case PAZAAK_CARD_PLUS_2 + 1: case PAZAAK_CARD_PLUS_3 + 1:
        case PAZAAK_CARD_PLUS_4 + 1: case PAZAAK_CARD_PLUS_5 + 1: case PAZAAK_CARD_PLUS_6 + 1:
            res = const_cast<char*>("PCARDS_POS_P");
            break;
        case PAZAAK_CARD_MINUS_1 + 1: case PAZAAK_CARD_MINUS_2 + 1: case PAZAAK_CARD_MINUS_3 + 1:
        case PAZAAK_CARD_MINUS_4 + 1: case PAZAAK_CARD_MINUS_5 + 1: case PAZAAK_CARD_MINUS_6 + 1:
            res = const_cast<char*>("PCARDS_NEG_P");
            break;
        case PAZAAK_CARD_PLUS_MINUS_1 + 1: case PAZAAK_CARD_PLUS_MINUS_2 + 1: case PAZAAK_CARD_PLUS_MINUS_3 + 1:
        case PAZAAK_CARD_PLUS_MINUS_4 + 1: case PAZAAK_CARD_PLUS_MINUS_5 + 1: case PAZAAK_CARD_PLUS_MINUS_6 + 1:
            // The positive artwork is selected when the card is not negative.
            res = card.m_bIsNegative ? const_cast<char*>("PCARDS_DBLNEG_P") : const_cast<char*>("PCARDS_DBLPOS_P");
            break;
        case PAZAAK_CARD_TIEBREAKER + 1: case PAZAAK_CARD_DOUBLE + 1: case PAZAAK_CARD_FLIP_2_4 + 1:
        case PAZAAK_CARD_FLIP_3_6 + 1: case PAZAAK_CARD_PLUS_MINUS_1_2 + 1:
            res = const_cast<char*>("PCARDS_GOLD_P");
            break;
        case PAZAAK_CARD_MAIN_1 + 1: case PAZAAK_CARD_MAIN_2 + 1: case PAZAAK_CARD_MAIN_3 + 1:
        case PAZAAK_CARD_MAIN_4 + 1: case PAZAAK_CARD_MAIN_5 + 1: case PAZAAK_CARD_MAIN_6 + 1:
        case PAZAAK_CARD_MAIN_7 + 1: case PAZAAK_CARD_MAIN_8 + 1: case PAZAAK_CARD_MAIN_9 + 1:
        case PAZAAK_CARD_MAIN_10 + 1:
            res = const_cast<char*>("PCARDS_GENERIC_P");
            break;
        default:
            res = const_cast<char*>("PCARDS_BACK_P");
            break;
    }
    m_border.m_params.SetFillImage(res, 0);
#endif
}

void CSWGuiPazaakCard::Draw(float f) {
    if (m_tail.bActive) {
        // Active cards paint both chrome borders themselves...
        m_border.Draw(f);
        m_borderHilight.Draw(f);
    } else {
        // ...inactive cards fall back to the button's own Draw, which paints only the main border.
        CSWGuiButton::Draw(f);
    }
}

void CSWGuiPazaakCard::DrawDragDrop(int x, int y) {
    if (g_pGuiMan->m_pMouseCaptureObject == this && g_pGuiMan->m_nCaptureFlags == 1) {
        // Reposition the card and its caption for the drag, then put both back.
        int ow = m_border.m_extent.w;
        int oh = m_border.m_extent.h;
        int ox = m_border.m_extent.x;
        int oy = m_border.m_extent.y;
        int nx = x - ow / 2;
        int ny = y - oh / 2;
        m_border.m_extent.x = nx;
        m_border.m_extent.y = ny;
        m_border.Draw(0.0f);

        CSWGuiExtent saved = m_label.m_extent;
        CSWGuiExtent cur = m_label.m_extent;
        cur.x += nx - ox;
        cur.y += ny - oy;
        m_label.SetExtent(cur);
        m_label.Draw(0.0f);
        m_label.SetExtent(saved);

        m_border.m_extent.x = ox;
        m_border.m_extent.y = oy;
        m_border.m_extent.w = ow;
        m_border.m_extent.h = oh;
    } else {
        // When this is not the captured drag source, nothing is dragging, so clear the ghost latch.
        g_pGuiMan->m_pDragDropControl = 0;
    }
}

void CSWGuiPazaakCard::HandleInputEvent(int type, int val) {
    if (type == INPUTEVENT_GUI_A)
        CSWGuiNavigable::HandleInputEvent(type, val);
    else
        CSWGuiButton::HandleInputEvent(type, val);
}

void CSWGuiPazaakCard::HandleLMouseDown() {
    m_dragStartPos = g_pGuiMan->m_currentMousePos;
    CSWGuiControl::HandleLMouseDown();
}

int CSWGuiPazaakCard::HandleMouseCapturedMovement(int x, int y) {
    if (!m_bDragLatch)
        return 0;
    if (g_pGuiMan->m_nCaptureFlags != 1)
        return 0;
    if (g_pGuiMan->m_pDragDropControl == this)
        return 0;
    // No fill image yet => nothing to drag (an unskinned card).
    if (m_border.m_params.m_fillImage.GetResRef()[0] == 0)
        return 0;
    // Latch as the drag ghost once the pointer leaves the dead zone.
    int dx = x - m_dragStartPos.x;
    int dy = y - m_dragStartPos.y;
    if ((dx < 0 ? -dx : dx) > 12 || (dy < 0 ? -dy : dy) >= 13)
        g_pGuiMan->m_pDragDropControl = this;
    return 0;
}

void CSWGuiPazaakCard::HandleLMouseUp() {
    if (g_pGuiMan->m_pDragDropControl == this) {
        // We were the drag ghost, so clear the latch, tell the input system the drag ended, release
        // the mouse and drop focus.
        g_pGuiMan->m_pDragDropControl = 0;
        HandleInputEvent(SWGUIEVENT_CARD_DROPPED, 1);
        g_pGuiMan->ReleaseMouse(this);
        HandleLoseMouseFocus();
    } else {
        CSWGuiControl::HandleLMouseUp();
    }
}

int CSWGuiPazaakCard::GetIsSelectable() {
    // Selectable only while enabled and the inherited control is enabled too.
    return m_bEnabled & m_tail.bEnabled;
}

CSWGuiPazaakCard::CSWGuiPazaakCard() {
#if KOTOR1
    m_bDragLatch = 1;
    m_bEnabled   = 1;
    CPazaakCard empty;
    empty.m_nCardType = PAZAAK_CARD_NONE;
    empty.m_bIsNegative = 0;
    m_card = empty;
    m_label.m_tail.bIgnoreMouse = 1;
#else
    m_bDragLatch = 1;
    m_bEnabled   = 1;
    m_label.m_tail.bIgnoreMouse = 1;
    m_card.m_nCardType    = PAZAAK_CARD_NONE;
    m_card.m_bIsNegative  = 0;
    m_card.m_nValue       = 0;
#endif
}







void CSWGuiTutorial::ShowHelp(uint64_t nHelpStrRef, uint64_t nOkStrRef,
                              uint64_t nCancelStrRef)
{
    m_helpText.m_count = 0;
    m_nCurrentHelp = 0;
    m_pAnchorControl = 0;
    // If it is already on screen, take it down first so AddPanel below re-pushes it on top.
#if KOTOR1
    // K1: the same test and clear on the K1 state field.
    if (m_stateFlags & PANEL_REMOVE_MASK)
    {
        m_stateFlags &= ~PANEL_REMOVE_STATE_MASK;
        m_pGuiManager->RemovePanel(this);
    }
#else
    if (m_stateFlags & PANEL_REMOVE_MASK)
    {
        m_stateFlags &= ~PANEL_REMOVE_STATE_MASK;
        m_pGuiManager->RemovePanel(this);
    }
#endif
    if (!m_bEnabled)
        return;

    m_helpText.Add(nHelpStrRef);
    // For the single-page spelling, hide cancel and caption the remaining button "Next".
    if (nOkStrRef == 0xffffffff && nCancelStrRef == STRREF_PAZAAK_CANCEL)
    {
        SetAllowCancel(0);
        m_btnOk.m_text.m_params.SetStrRef(STRREF_CONTINUE);
    }
    else
    {
        SetAllowCancel(1);
        m_btnOk.m_text.m_params.SetStrRef(nOkStrRef);
        m_btnCancel.m_text.m_params.SetStrRef(nCancelStrRef);
    }
    SetMessage(nHelpStrRef);
    m_pGuiManager->AddPanel(this, 1, 1);
}

void CSWGuiTutorial::SetBButtonEnabled(int bEnabled)
{
    SetAllowCancel(bEnabled);
}

void CSWGuiTutorial::ShowMultipleHelp(const uint64_t* panStrRefs, int nCount)
{
    if (!m_bEnabled)
        return;
    // The first page goes through ShowHelp (which resets the list and pushes the panel); the rest
    // are appended behind it, and the cancel button becomes "Back".
    ShowHelp(panStrRefs[0], 0xffffffff, STRREF_PAZAAK_CANCEL);
    for (int i = 1; i < nCount; i++)
        m_helpText.Add(panStrRefs[i]);
    m_btnCancel.m_text.m_params.SetStrRef(STRREF_PAZAAK_BACK);
}

void CSWGuiTutorial::PlaceRightOf(CSWGuiControl* pControl)
{
    CSWGuiExtent extent = pControl->m_extent;
    int nX = extent.x + extent.w;
    int nY = extent.y + extent.h / 2;
    // The control's extent is panel-relative, so add the owning panel's own offset back in.
    if (pControl->m_pContainer)
    {
        CSWGuiPanel* pPanel = pControl->m_pContainer->AsSWGuiPanel();
        if (pPanel)
            pPanel->GetExtentAccountingForPanelOffset(&extent);
        nX = extent.x + nX;
        nY = extent.y + nY;
    }
    extent = m_extent;
    extent.x = nX;
    extent.y = nY - extent.h / 2;
    SetExtent(extent);
}

void CSWGuiTutorial::PlaceAbove(CSWGuiControl* pControl)
{
    m_pAnchorControl = pControl;
    CSWGuiExtent extent = pControl->m_extent;
    int nX = extent.x;
    int nY = extent.y;
    if (pControl->m_pContainer)
    {
        CSWGuiPanel* pPanel = pControl->m_pContainer->AsSWGuiPanel();
        if (pPanel)
            pPanel->GetExtentAccountingForPanelOffset(&extent);
        nX = extent.x + nX;
        nY = extent.y + nY;
    }
    extent = m_extent;
    extent.x = nX;
    extent.y = nY - extent.h;
    SetExtent(extent);
}

uint64_t CSWGuiTutorial::GetHelpText()
{
    if (m_nCurrentHelp >= m_helpText.m_count)
        return 0;
    return m_helpText.m_pData[m_nCurrentHelp];
}

void CSWGuiTutorial::HandleInputEvent(int nType, int nValue)
{
    if (nValue)
    {
        switch (nType)
        {
            // A / next: step one page towards the back, which also makes "Back" available.
            case INPUTEVENT_GUI_A:
            case SWGUIEVENT_MSGBOX_OK:
#if !KOTOR1  // K1: the pazaak tutorial is not interactive
                HandlePazaakTutorialDismissButton();
#endif
                if (m_nCurrentHelp < m_helpText.m_count - 1)
                {
                    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                    m_nCurrentHelp++;
                    SetAllowCancel(1);
                    int nIndex = m_nCurrentHelp;
                    SetMessage(m_helpText.m_pData[nIndex]);
                    if (m_pAnchorControl)
                        PlaceAbove(m_pAnchorControl);
                    return;
                }
                break;
            // B / back: step one page towards the front. Reaching page 0 takes the cancel button
            // away again, since there is nothing left to go back to.
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_BACK:
            case SWGUIEVENT_MSGBOX_CANCEL:
                if (m_nCurrentHelp >= 1)
                {
                    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                    m_nCurrentHelp--;
                    if (!m_nCurrentHelp)
                        SetAllowCancel(0);
                    SetMessage(m_helpText.m_pData[m_nCurrentHelp]);
                    if (m_pAnchorControl)
                        PlaceAbove(m_pAnchorControl);
                    return;
                }
                break;
        }
    }
    CSWGuiMessageBox::HandleInputEvent(nType, nValue);
}


CSWGuiTutorial::CSWGuiTutorial(CSWGuiManager* pGuiManager) : CSWGuiMessageBox(pGuiManager) {}

#ifdef K2_UI_XBOX
// The console wager popup loads pazaakwager_x in a sized window. The console popup has no buttons:
// the two arrows and the two captions bind into the PC's four button members so they stay tap
// targets with the PC handlers, and the two badges live in k2PazaakWagerBadgeStore.
namespace {
// Storage for the two badge labels the class has no member for (LBL_A, LBL_B): raw bytes and
// placement new, since static constructors do not run in this build. One popup exists at a time and
// each one rebuilds them.
alignas(16) unsigned char k2PazaakWagerBadgeStore[2 * sizeof(CSWGuiLabel)];
int k2PazaakWagerBadgeLive;

CSWGuiLabel* RebuildBadges() {
    CSWGuiLabel* p = reinterpret_cast<CSWGuiLabel*>(k2PazaakWagerBadgeStore);
    if (k2PazaakWagerBadgeLive)
        for (int i = 0; i < 2; i++)
            p[i].~CSWGuiLabel();
    for (int i = 0; i < 2; i++)
        new (&p[i]) CSWGuiLabel();
    k2PazaakWagerBadgeLive = 1;
    return p;
}
}  // namespace
#endif

CSWGuiWagerPopup::CSWGuiWagerPopup(CSWGuiManager* pManager, CSWGuiPazaakStart* pOwner, int nWager)
    : CSWGuiPanel(pManager), m_nWager(nWager), m_nWagerMax(nWager), m_aArrowFlashes(), m_pOwner(pOwner) {
#ifdef K2_UI_XBOX
    // Console: pazaakwager_x in a sized window; the background and bar bind first, then the
    // captions, the arrows and the two badges.
    const int nSavedGUIWidth  = g_nGUIWidth;
    const int nSavedGUIHeight = g_nGUIHeight;
    g_nGUIWidth  = 640;
    g_nGUIHeight = 480;
    StartLoadFromLayout(CResRef("pazaakwager_x"), 1, 1, 1);

    CSWGuiLabel* pBadges = RebuildBadges();
    InitControl(m_lblBg, CExoString("LBL_BG"), 1, 1, 0, 0, 0);
    InitControl(m_lblBar1, CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_lblMaximum, CExoString("LBL_MAXIMUM"), 1, 1, 0, 0, 0);
    InitControl(m_lblWagerVal, CExoString("LBL_WAGERVAL"), 1, 1, 0, 0, 0);
    InitControl(m_btnLess, CExoString("LBL_LESS"), 1, 1, 0, 0, 0);
    InitControl(m_btnMore, CExoString("LBL_MORE"), 1, 1, 0, 0, 0);
    InitControl(m_btnWager, CExoString("LBL_WAGER"), 1, 1, 0, 0, 0);
    InitControl(m_btnQuit, CExoString("LBL_QUIT"), 1, 1, 0, 0, 0);
    InitControl(pBadges[0], CExoString("LBL_A"), 1, 1, 0, 0, 0);
    InitControl(pBadges[1], CExoString("LBL_B"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
    g_nGUIWidth  = nSavedGUIWidth;
    g_nGUIHeight = nSavedGUIHeight;
#elif KOTOR1
    StartLoadFromLayout(CResRef("pazaakwager"), 1, 1, 1);

    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_lblMaximum, CExoString("LBL_MAXIMUM"), 1, 1, 0, 0, 0);
    InitControl(m_lblWagerVal, CExoString("LBL_WAGERVAL"), 1, 1, 0, 0, 0);
    InitControl(m_btnLess, CExoString("BTN_LESS"), 1, 1, 0, 0, 0);
    InitControl(m_btnMore, CExoString("BTN_MORE"), 1, 1, 0, 0, 0);
    InitControl(m_btnWager, CExoString("BTN_WAGER"), 1, 1, 0, 0, 0);
    InitControl(m_btnQuit, CExoString("BTN_QUIT"), 1, 1, 0, 0, 0);
    InitControl(m_lblBg, CExoString("LBL_BG"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    // K1: rescale, widen the panel to the window and shift every control by half the gain.
    {
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
        float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#else
        static float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#endif
        ScaleExtentForResolution(s_fScale);
        CSWGuiExtent extent = m_extent;
        int nShift = (g_SDL_mode.w - extent.w) / 2;
        extent.w = g_SDL_mode.w;
        SetExtent(extent);
        CSWGuiControl* apControls[8] = {&m_lblTitle, &m_lblMaximum, &m_lblWagerVal, &m_btnLess,
                                        &m_btnMore,  &m_btnWager,   &m_btnQuit,     &m_lblBg};
        for (int i = 0; i < 8; i++) {
            CSWGuiExtent ext = apControls[i]->m_extent;
            ext.x += nShift;
            apControls[i]->SetExtent(ext);
        }
    }
#else
    StartLoadFromLayout(CResRef("pazaakwager_p"), 1, 1, 1);

    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_lblMaximum, CExoString("LBL_MAXIMUM"), 1, 1, 0, 0, 0);
    InitControl(m_lblWagerVal, CExoString("LBL_WAGERVAL"), 1, 1, 0, 0, 0);
    InitControl(m_btnLess, CExoString("BTN_LESS"), 1, 1, 0, 0, 0);
    InitControl(m_btnMore, CExoString("BTN_MORE"), 1, 1, 0, 0, 0);
    InitControl(m_btnWager, CExoString("BTN_WAGER"), 1, 1, 0, 0, 0);
    InitControl(m_btnQuit, CExoString("BTN_QUIT"), 1, 1, 0, 0, 0);
    InitControl(m_lblBar1, CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_lblBg, CExoString("LBL_BG"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#endif


    m_btnLess.AddEvent(SWGUIEVENT_SPEED_BUTTON_LEFT, this,
                       static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiWagerPopup::HandleButtonLess));
    m_btnMore.AddEvent(SWGUIEVENT_SPEED_BUTTON_LEFT, this,
                       static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiWagerPopup::HandleButtonMore));
    m_btnWager.AddEvent(INPUTEVENT_GUI_A, this,
                        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiWagerPopup::HandleButtonWager));
    m_btnQuit.AddEvent(INPUTEVENT_GUI_A, this,
                       static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiWagerPopup::HandleButtonQuit));
    m_btnLess.m_tail.bFocusOnHover = 0;
    m_btnMore.m_tail.bFocusOnHover = 0;
    m_btnWager.m_tail.bFocusOnHover = 0;
    m_btnQuit.m_tail.bFocusOnHover = 0;
    m_btnLess.m_fStep = 0.06666667f;
    m_btnMore.m_fStep = 0.06666667f;

    // The gold readout for the title bar follows client PC -> server id -> server creature ->
    // m_nGold, and is 0 when the lookup comes up empty.
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    uint64_t oidServer = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pPlayer->m_objectId);
    CSWSCreature* pPC = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidServer);
    int nGold = 0;
    if (pPC)
        nGold = pPC->GetGold();

    // The wager readout, then the stakes banner, which is the first TLK strref, the maximum, a
    // newline, the second TLK strref and the gold.
    UpdateWagerText();
    CExoString sMaximum = g_pTlkTable->GetSimpleString(STRREF_MAXIMUM_WAGER) + CExoString(" ") +
                          CExoString(m_nWagerMax) + CExoString("\n") +
                          g_pTlkTable->GetSimpleString(STRREF_CREDITS_LABEL) + CExoString(" ") + CExoString(nGold);
    m_lblMaximum.m_text.m_params.SetText(sMaximum);
#if KOTOR1
    IosAddButtonIcon(&m_btnQuit, 'b');
    IosAddButtonIcon(&m_btnWager, 'a');
#endif
#ifdef K2_QOL_IMPROVEMENTS
    // Fit the title, the wager and the stakes banner into their layout boxes; the two buttons fit
    // themselves. The readout starts at the maximum, its widest value, and keeps this size as the
    // wager steps down and back.
    K2QolFitText(m_lblTitle.m_text, m_lblTitle.m_text.m_extent.h);
    K2QolFitText(m_lblWagerVal.m_text, m_lblWagerVal.m_text.m_extent.h);
    K2QolFitText(m_lblMaximum.m_text, m_lblMaximum.m_text.m_extent.h);
#endif
}

void CSWGuiWagerPopup::UpdateWagerText()
{
    m_lblWagerVal.m_text.m_params.SetText(CExoString(m_nWager));
}

void CSWGuiWagerPopup::HandleInputEvent(int nEvent, int nParam)
{
    if (nParam != 0) {
        switch (nEvent) {
        case INPUTEVENT_GUI_DPAD_LEFT:
        case INPUTEVENT_GUI_DPAD_DOWN:
        case INPUTEVENT_GUI_SCROLL_DOWN:
        case INPUTEVENT_GUI_SCROLL_LEFT:
        case INPUTEVENT_GUI_KEY_DOWN:
        case INPUTEVENT_GUI_KEY_LEFT:  // wager down (keys + the Less button's code)
            if (m_nWager >= 2) {
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                --m_nWager;
                m_lblWagerVal.m_text.m_params.SetText(CExoString(m_nWager));
            }
            break;
        case INPUTEVENT_GUI_DPAD_RIGHT:
        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_SCROLL_UP:
        case INPUTEVENT_GUI_SCROLL_RIGHT:
        case INPUTEVENT_GUI_KEY_UP:
        case INPUTEVENT_GUI_KEY_RIGHT:  // wager up (keys + the More button's code)
            if (m_nWager < m_nWagerMax) {
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                ++m_nWager;
                m_lblWagerVal.m_text.m_params.SetText(CExoString(m_nWager));
            }
            break;
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {  // the Quit button / its key equivalent: confirm through the shared box
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            CSWGuiMessageBox* pMsg = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pMsg->SetAllowCancel(1);
            pMsg->SetCallback(m_pOwner, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPazaakStart::HandleQuitDialog), 0);
            pMsg->SetMessage(STRREF_QUIT_IS_FORFEIT);
#if !KOTOR1  // K1: the box keeps its own colours
            pMsg->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
            m_pGuiManager->AddPanel(pMsg, 1, 1);
        } break;
        case INPUTEVENT_GUI_A:
        case INPUTEVENT_GUI_START:  // the Wager button / its key equivalent: commit and leave
            m_pOwner->m_pPazaak->m_nWager = m_nWager;
            m_pOwner->SetActiveControl(&m_pOwner->m_aAvail[0], 0);
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
            m_pGuiManager->RemovePanel(this);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(nEvent, nParam);
}

void CSWGuiWagerPopup::Draw(float f)
{
    CSWGuiPanel::Draw(f);
}

#if KOTOR1
// Light an arrow button yellow for a quarter of a second; an arrow already lit is only re-armed.
void CSWGuiWagerPopup::FlashArrow(CSWGuiButton& button)
{
    int nCount = m_aArrowFlashes.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_aArrowFlashes[i].m_pButton == &button) {
            m_aArrowFlashes[i].m_fTimer = 0.25f;
            return;
        }
    }
    m_aArrowFlashes.SetSize(nCount + 1);
    m_aArrowFlashes[nCount].m_pButton = &button;
    m_aArrowFlashes[nCount].m_fTimer = 0.25f;
    button.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    button.m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
}
#endif


void CSWGuiActivatedButton::ActivateLabel(const Vector& pos) {
    m_fFlashTimer = 0.8f;
    m_labelPos = pos;
}

void CSWGuiActivatedButton::Draw(float dt) {
    float t = m_fFlashTimer;
    if (t >= 0.0f) {
        Vector color = CGuiInGame::DISABLED_TEXT;
        t -= dt;
        m_fFlashTimer = t;
        if (t < 0.0f) {
            // countdown finished: settle on the resting tint
            if (m_tail.bEnabled)
#if KOTOR1  // K1: blue, and the border colour is not touched
                color = CGuiInGame::COLOR_BLUE;
#else
                color = CGuiInGame::COLOR_PAZAAK_GOLD;
#endif
        } else {
            // 3 full sine pulses across the 0.8 s activation window
            float k = (sinf((t / 0.8f) * 2.0f * 3.14159265f * 3.0f) + 1.0f) * 0.5f;
            float s = 1.0f - k;
            color = m_labelPos * k + color * s;
        }
        m_text.m_params.SetColor(color);
#if !KOTOR1
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&color);
#endif
    }
    CSWGuiButton::Draw(dt);
}
