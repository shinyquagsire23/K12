// k2console.cpp - the console action-bar input driver, host side (K2P_UI=xbox).
//
// The rules this driver follows (one producer per physical press, the `owner` column, the shim mask,
// the observed-duplicate sentinel, the modal conversation, the in-place entry cycle, A versus X, Y
// as the queue clear) are described where each is implemented below. The whole engine is ours, so
// what a hook would have reached by address is a named member or function of src/, and the pad
// arrives through the host's own DirectInput device, platforms/shim/shim_dinput.cpp, which asks
// k2console_dishim_mask() once per engine poll.
//
// The knobs are Android system properties, `debug.k2.console...`; on the desktop the same names are
// environment variables, `debug.` dropped, upper-cased, dots to underscores:
//         K2_CONSOLE=0|1|2|3     off / on (default) / on + log every dispatch / trace only
//         K2_CONSOLE_POPUP=1     keep the mobile pop-up action menu
//         K2_CONSOLE_MODAL=0     do not make a conversation modal
//         K2_CONSOLE_ENGINE=1    stand down where the engine produces (the one-producer rule)
//         K2_CONSOLE_AUTOSLOT=0  a new target does not move the bar to box 1
//         K2_CONSOLE_REPEAT=N    d-pad auto-repeat after N tenths of a second (default 0 = off)
//         K2_CONSOLE_SLOTS=230154  the bar's six list indices, box 2 first
//         K2_CONSOLE_INJECT=NAME[:n]  fire the named action once (kActionNames)
//         K2_DISHIM=1|2          the engine sees the whole pad (no claim) / no button at all
//     The OS half (logging, the property/environment read, the clock) is platforms/shim/
//     shim_console.cpp: this TU is compiled with the engine's flags so it can include src/'s
//     headers, and an engine TU may not include a system header (compat/k2host_prelude.h).
//
// It runs on the game thread, once per frame, from src/'s K2_UI_XBOX CSWGuiMainInterfaceStatus::Update
// (the HUD's update path keeps ticking under a modal; its draw does not) and, as a supplement, from
// UpdateActionDescription; the tick rate-limits itself to one run per frame.
//
// Reads input from every SDL game controller merged, only through SDL_GameController names (the
// fifteen buttons and the two trigger axes), the same view the shim hands the engine, plus the
// keyboard mirror of the pad layout for a machine with no pad: arrows = d-pad, Return = A,
// Backspace = B, Tab = X, Y = Y, L/R = the shoulders, Q/E = ZL/ZR, P = Plus, O = Minus, F = the
// right-stick click.
#include <stdint.h>
#include "swguimaininterfacepc2.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "ClientExoAppInternal.h"
#include "guiingame.h"
#include "swguidialog.h"
#include "clientoptions.h"
#include "swparty.h"
#include "nwccreature.h"
#include "nwscreature.h"
#include "nwscombatround.h"
#include "exolinkedlistinternal.h"
#include "exoinput.h"
#include "exoinputinternal.h"

bool IsJoystickConnected();   // src/game/clientcore/clientexoapp.cpp

// the OS half: platforms/shim/shim_console.cpp.
extern "C" {
void k2console_log(const char* pszLine);
int  k2console_prop(const char* pszName, char* pBuf, int nBuf);   // debug.k2.* -> length, 0 = unset
int64_t k2console_now_ms(void);
}

// SDL, declared here (an engine TU may not include SDL.h; opaque SDL_GameController*).
extern "C" {
int           k2shim_pad_index(void);   // shim/shim_dinput.cpp: the one the DirectInput pad reports
int           k2shim_pad_button(int nButton);   // ... and every pad merged, which it reads
int           k2shim_pad_axis(int nAxis);
int           SDL_NumJoysticks(void);
int           SDL_IsGameController(int nIndex);
void*         SDL_GameControllerOpen(int nIndex);
void*         SDL_GameControllerGetJoystick(void* pController);
int           SDL_JoystickNumButtons(void* pJoystick);
unsigned char SDL_JoystickGetButton(void* pJoystick, int nButton);
int           SDL_GameControllerGetAttached(void* pController);
const char*   SDL_GameControllerName(void* pController);
char*         SDL_GameControllerMapping(void* pController);
void          SDL_free(void* p);
const unsigned char* SDL_GetKeyboardState(int* pnKeys);
}

extern "C" unsigned int k2console_dishim_mask(unsigned int nDown);

#ifndef K2_UI_XBOX
// The pc / touch builds have no console HUD and nothing ticks a driver, so the pad is the engine's.
extern "C" unsigned int k2console_dishim_mask(unsigned int) { return 0; }
#else

// The HUD's selected bar slot, -1 = none. src/game/clientcore/swguimaininterfacepc2.cpp.
extern int g_nSelectedAction;
void K2ConsoleStartScroll(CSWGuiMainInterface* pMain, int nIndex, bool bNext, const CResRef& sOld,
                          float fOldAlpha);   // swguimaininterfacepc2.cpp (K2_UI_XBOX)
// The console bar's slot order (index = the widget, so box = index + 2; value = the
// CSWCCreature::GetPersonalActions nType whose list that box shows). Same TU.
extern int g_aConsoleSlotList[6];

namespace {

#define LOGI(...) k2log(__VA_ARGS__)
void k2log(const char* pszFmt, ...) __attribute__((format(printf, 1, 2)));
void k2log(const char* pszFmt, ...) {
    char sLine[600];
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    __builtin_vsnprintf(sLine, sizeof(sLine), pszFmt, ap);
    __builtin_va_end(ap);
    k2console_log(sLine);
}

const unsigned long long kObjectInvalid = OBJECT_INVALID;
const int kTargetSlot       = 6;   // console box 1: the folded target column
const int kTargetCaptionIdx = 6;   // box 1's control payload (Initialize's `nSlot += 6`)

// CExoInput's own device/key constants.
enum { DEV_KEYBOARD = 0 };
enum { KB_NONE = -1, KB_RETURN = 6, KB_LEFT_ARROW = 7, KB_RIGHT_ARROW = 8, KB_UP_ARROW = 9,
       KB_DOWN_ARROW = 10 };

// SDL_GameControllerButton, and the two trigger axes (SDL_CONTROLLER_AXIS_TRIGGERLEFT/RIGHT = 4/5).
enum { PAD_A = 0, PAD_B = 1, PAD_X = 2, PAD_Y = 3,
       PAD_BACK = 4, PAD_START = 6, PAD_LS = 7, PAD_RS = 8, PAD_L = 9, PAD_R = 10,
       PAD_UP = 11, PAD_DOWN = 12, PAD_LEFT = 13, PAD_RIGHT = 14,
       PAD_NONE = 0xff };
enum { PAD_AXIS_ZL = 0x80, PAD_AXIS_ZR = 0x81, PAD_AXIS_FIRST = 0x80 };
const int kTriggerOn = 12000;
// SDL_Scancode values for the keyboard mirror of the pad layout.
enum { KEY_E = 8, KEY_F = 9, KEY_L = 15, KEY_O = 18, KEY_P = 19, KEY_Q = 20, KEY_R = 21, KEY_Y = 28,
       KEY_RETURN = 40, KEY_BACKSPACE = 42, KEY_TAB = 43, KEY_RIGHT = 79, KEY_LEFT = 80,
       KEY_DOWN = 81, KEY_UP = 82 };

// The Switch's layout and the engine's own pad bindings it has to coexist with.
// `CExoInputInternal::GetEvents` ends with a hard-coded block that turns each DirectInput button
// into the mobile layout's events. On the HUD the Switch differs on A, X, Y, L, R, ZL, ZR, Plus,
// Minus and d-pad up/down: those rows are OWN_DRIVER, so while the HUD owns the input the shim
// reports them at rest and this driver is their only producer. B, d-pad left/right and the
// right-stick click are the same on both, so they stay the engine's (OWN_ENGINE). Off the HUD every
// input is the engine's.
enum Action {
    ACT_BOX_PREV,    // d-pad left: select the box to the left
    ACT_BOX_NEXT,    // d-pad right: the box to the right
    ACT_ENTRY_PREV,  // d-pad up: previous entry in the box (performed: cycle_entry)
    ACT_ENTRY_NEXT,  // d-pad down: next entry (performed)
    ACT_ACCEPT,      // A: wipe the queue, act now (ClearAllQueuedCombatActions, then the accept event)
    ACT_CANCEL,      // B: back / leave combat
    ACT_QUEUE_ADD,   // X: append to the round
    ACT_QUEUE_CLEAR, // Y: clear the player's queue (CSWSObject::ClearAllActions(1))
    ACT_NEXTCHAR,    // R: next living party member
    ACT_MENU,        // Plus: the in-game menu
    ACT_TGT_PREV,    // ZL: previous target
    ACT_TGT_NEXT,    // ZR: next target
    ACT_PAUSE,       // L: toggle the combat pause
    ACT_SOLO,        // Minus: the solo-mode query
    ACT_FREELOOK,    // R-stick click: first-person / free look
    ACT_COUNT
};
const char* const kActionNames[ACT_COUNT] = {
    "box-prev", "box-next", "entry-prev", "entry-next", "A/perform", "B/back", "X/queue",
    "Y/clear-queue", "R/next-char", "+/menu", "ZL/prev-target", "ZR/next-target", "L/pause",
    "-/solo", "RS/free-look" };

// `engineKey` / `engineClasses`: the keyboard key CClientExoAppInternal::SetEventDescriptions binds
// for this action and the input classes it binds it in (0-based). `repeat`: only the four
// directional actions may auto-repeat, since an auto-repeating A is a press the player never made.
#define CLS(n) (1u << (n))
enum { OWN_ENGINE = 0, OWN_DRIVER = 1 };
struct Binding { unsigned char act, pad, key, repeat; signed char engineKey;
                 unsigned char engineClasses; unsigned char owner; };
const Binding kBindings[ACT_COUNT] = {
    { ACT_BOX_PREV,    PAD_LEFT,    KEY_LEFT,      1, KB_LEFT_ARROW,  CLS(1)|CLS(2)|CLS(3), OWN_ENGINE },
    { ACT_BOX_NEXT,    PAD_RIGHT,   KEY_RIGHT,     1, KB_RIGHT_ARROW, CLS(1)|CLS(2)|CLS(3), OWN_ENGINE },
    { ACT_ENTRY_PREV,  PAD_UP,      KEY_UP,        1, KB_UP_ARROW,    CLS(1)|CLS(2)|CLS(3), OWN_DRIVER },
    { ACT_ENTRY_NEXT,  PAD_DOWN,    KEY_DOWN,      1, KB_DOWN_ARROW,  CLS(1)|CLS(2)|CLS(3), OWN_DRIVER },
    { ACT_ACCEPT,      PAD_A,       KEY_RETURN,    0, KB_RETURN,      CLS(1)|CLS(2)|CLS(3)|CLS(4), OWN_DRIVER },
    { ACT_CANCEL,      PAD_B,       KEY_BACKSPACE, 0, KB_NONE,        0, OWN_ENGINE },
    { ACT_QUEUE_ADD,   PAD_X,       KEY_TAB,       0, KB_NONE,        0, OWN_DRIVER },
    { ACT_QUEUE_CLEAR, PAD_Y,       KEY_Y,         0, KB_NONE,        0, OWN_DRIVER },
    { ACT_NEXTCHAR,    PAD_R,       KEY_R,         0, KB_NONE,        0, OWN_DRIVER },
    { ACT_MENU,        PAD_START,   KEY_P,         0, KB_NONE,        0, OWN_DRIVER },
    { ACT_TGT_PREV,    PAD_AXIS_ZL, KEY_Q,         0, KB_NONE,        0, OWN_DRIVER },
    { ACT_TGT_NEXT,    PAD_AXIS_ZR, KEY_E,         0, KB_NONE,        0, OWN_DRIVER },
    { ACT_PAUSE,       PAD_L,       KEY_L,         0, KB_NONE,        0, OWN_DRIVER },
    { ACT_SOLO,        PAD_BACK,    KEY_O,         0, KB_NONE,        0, OWN_DRIVER },
    { ACT_FREELOOK,    PAD_RS,      KEY_F,         0, KB_NONE,        0, OWN_ENGINE },
};
#undef CLS

int64_t g_nRepeatDelayMs = 0;     // K2_CONSOLE_REPEAT; off by default, since the auto-repeat is
const int64_t kRepeatRateMs = 110; //   itself a double-press source

bool g_held[ACT_COUNT];        // the physical state of the binding last frame
unsigned char g_padDisabled[ACT_COUNT];  // audit_mapping: this row's pad source is shared
bool g_suppress[ACT_COUNT];    // held, but must be released before it may fire again
int64_t g_nextFireMs[ACT_COUNT];
int  g_lastOwner = -1;         // (modal, conversation) packed -- who owned input last frame
int64_t g_lastTickMs = 0;
CSWGuiPanel* g_pPushedModal;   // the conversation panel this driver pushed onto the modal stack
int  g_bPopupSeen;

// For observed-duplicate suppression, the tick leaves this sentinel in
// CSWGuiManager::m_nCurrentInputEvent on its way out, so any other value there at the top of the
// next tick was handed to the manager by a producer that is not this driver.
int64_t g_engineEventMs[256];
const int kNoEventSentinel = 0;
const int64_t kDupWindowMs = 250;

// The shim mask holds the DirectInput bits of every live OWN_DRIVER row while the HUD owns the
// input, 0 otherwise. It is stamped, so the shim falls back to the engine's bindings once the tick
// stops.
unsigned g_dishimMask;
int64_t     g_dishimMaskMs;
const int64_t kDishimMaskTtlMs = 250;
int  g_nDishimProp;

int  g_mode = -1;              // -1 unread, 0 off, 1 on, 2 verbose, 3 trace only
int  g_bEngineRule = 0;        // off by default: measured on Android
int  g_bKeepPopup;
int  g_bDialogModal = 1;
int  g_bAutoSlot = 1;
int64_t g_nextModeCheckMs;

void* g_pad;                   // the opened SDL_GameController, if any
int64_t  g_nextPadScanMs;

int prop_int(const char* pszName, int nDefault) {
    char v[92] = {0};
    if (k2console_prop(pszName, v, sizeof(v)) <= 0 || !v[0]) return nDefault;
    return v[0] - '0';
}

// K2_CONSOLE_SLOTS / debug.k2.console.slots: a full permutation of 0..5, box 2 first.
void apply_slot_order(void) {
    char v[92] = {0};
    if (k2console_prop("debug.k2.console.slots", v, sizeof(v)) <= 0 || !v[0]) return;
    static char sLast[16];
    if (!__builtin_strncmp(sLast, v, sizeof(sLast) - 1)) return;
    for (unsigned i = 0; i + 1 < sizeof(sLast); ++i) sLast[i] = v[i];
    sLast[sizeof(sLast) - 1] = 0;

    int anOrder[6];
    unsigned nSeen = 0;
    int i = 0;
    for (; i < 6; ++i) {
        if (v[i] < '0' || v[i] > '5') break;
        anOrder[i] = v[i] - '0';
        nSeen |= 1u << anOrder[i];
    }
    if (i != 6 || v[6] != 0 || nSeen != 0x3f) {
        LOGI("slot order \"%s\" ignored: six digits, a permutation of 0..5, box 2 first", v);
        return;
    }
    for (i = 0; i < 6; ++i) g_aConsoleSlotList[i] = anOrder[i];
    LOGI("console bar slot order -> box2=%d box3=%d box4=%d box5=%d box6=%d box7=%d",
         anOrder[0], anOrder[1], anOrder[2], anOrder[3], anOrder[4], anOrder[5]);
}

// the mapping audit.
struct MappedName { const char* name; unsigned char pad; };
const MappedName kMappedNames[] = {
    { "a", PAD_A }, { "b", PAD_B }, { "x", PAD_X }, { "y", PAD_Y }, { "back", PAD_BACK },
    { "start", PAD_START }, { "leftstick", PAD_LS }, { "rightstick", PAD_RS },
    { "leftshoulder", PAD_L }, { "rightshoulder", PAD_R }, { "dpup", PAD_UP },
    { "dpdown", PAD_DOWN }, { "dpleft", PAD_LEFT }, { "dpright", PAD_RIGHT },
    { "lefttrigger", PAD_AXIS_ZL }, { "righttrigger", PAD_AXIS_ZR } };
const int kMappedNameCount = static_cast<int>(sizeof(kMappedNames) / sizeof(kMappedNames[0]));

const char* action_for_pad(unsigned char nPad) {
    for (int i = 0; i < ACT_COUNT; ++i)
        if (kBindings[i].pad == nPad) return kActionNames[i];
    return 0;
}

void audit_mapping(const char* pMapping) {
    if (!pMapping) return;
    char aSrc[kMappedNameCount][16];
    for (int i = 0; i < kMappedNameCount; ++i) aSrc[i][0] = 0;
    // "name:source," records; the first two fields are the GUID and the device name.
    const char* p = pMapping;
    int nField = 0;
    while (*p) {
        const char* q = p;
        while (*q && *q != ',') ++q;
        if (nField >= 2) {
            const char* c = p;
            while (c < q && *c != ':') ++c;
            if (c < q) {
                for (int i = 0; i < kMappedNameCount; ++i) {
                    uint64_t n = __builtin_strlen(kMappedNames[i].name);
                    if (static_cast<uint64_t>(c - p) == n && !__builtin_strncmp(p, kMappedNames[i].name, n)) {
                        uint64_t m = static_cast<uint64_t>(q - c - 1);
                        if (m >= sizeof(aSrc[i])) m = sizeof(aSrc[i]) - 1;
                        __builtin_memcpy(aSrc[i], c + 1, m);
                        aSrc[i][m] = 0;
                    }
                }
            }
        }
        nField++;
        p = *q ? q + 1 : q;
    }
    char line[512];
    int n = __builtin_snprintf(line, sizeof(line), "console driver: pad sources:");
    for (int i = 0; i < kMappedNameCount && n < static_cast<int>(sizeof(line)); ++i) {
        const char* pAct = action_for_pad(kMappedNames[i].pad);
        if (!pAct) continue;
        n += __builtin_snprintf(line + n, sizeof(line) - n, " %s=%s", pAct,
                                aSrc[i][0] ? aSrc[i] : "(unmapped)");
    }
    LOGI("%s", line);
    int nBad = 0;
    for (int i = 0; i < kMappedNameCount; ++i) {
        if (!aSrc[i][0]) {
            if (action_for_pad(kMappedNames[i].pad))
                LOGI("console driver: MAPPING GAP -- SDL maps nothing to '%s', so '%s' cannot fire "
                     "from the pad", kMappedNames[i].name, action_for_pad(kMappedNames[i].pad));
            continue;
        }
        for (int j = i + 1; j < kMappedNameCount; ++j) {
            if (!aSrc[j][0] || __builtin_strcmp(aSrc[i], aSrc[j])) continue;
            const char* a = action_for_pad(kMappedNames[i].pad);
            const char* b = action_for_pad(kMappedNames[j].pad);
            if (!a && !b) continue;
            nBad++;
            LOGI("console driver: MAPPING COLLISION -- SDL resolves '%s' AND '%s' to the same raw "
                 "source %s, so one press of it would fire '%s' and '%s'; the second is disabled",
                 kMappedNames[i].name, kMappedNames[j].name, aSrc[i], a ? a : "(nothing)",
                 b ? b : "(nothing)");
            for (int k = 0; k < ACT_COUNT; ++k)
                if (kBindings[k].pad == kMappedNames[j].pad) g_padDisabled[k] = 1;
        }
    }
    if (!nBad) LOGI("console driver: mapping audit clean -- every bound name has its own raw source");
}

// Open the first attached game controller (the one the shim's DirectInput device reads too),
// re-scanning at most twice a second: pads come and go.
void refresh_pad(int64_t tMs) {
    if (g_pad && SDL_GameControllerGetAttached(g_pad)) return;
    if (tMs < g_nextPadScanMs) return;
    g_nextPadScanMs = tMs + 500;
    g_pad = 0;
    int n = SDL_NumJoysticks();
    const int i = k2shim_pad_index();
    if (i >= 0)
        g_pad = SDL_GameControllerOpen(i);
    if (g_pad) {
        LOGI("console driver: controller %d opened (%s)", i, SDL_GameControllerName(g_pad));
        for (int k = 0; k < ACT_COUNT; ++k) g_padDisabled[k] = 0;
        char* pMapping = SDL_GameControllerMapping(g_pad);
        LOGI("console driver: SDL mapping = %s", pMapping ? pMapping : "(none)");
        audit_mapping(pMapping);
        if (pMapping) SDL_free(pMapping);
        return;
    }
    static int sSaidNone;
    if (!sSaidNone) {
        sSaidNone = 1;
        LOGI("console driver: no SDL game controller (%d joystick(s) present, none mapped) -- the "
             "keyboard mirror is the only pad", n);
    }
}

enum { SRC_PAD = 1, SRC_KEY = 2 };
bool pad_down(unsigned char nPad) {
    if (nPad == PAD_NONE || !g_pad) return false;
    if (nPad >= PAD_AXIS_FIRST)
        return k2shim_pad_axis(4 + (nPad - PAD_AXIS_ZL)) > kTriggerOn;
    return k2shim_pad_button(nPad) != 0;
}

void describe_source(const Binding& b, int nSrc, char* pOut, int nOut) {
    pOut[0] = 0;
    if (nSrc & SRC_PAD) {
        if (b.pad >= PAD_AXIS_FIRST)
            __builtin_snprintf(pOut, nOut, "pad axis %d=%d", 4 + (b.pad - PAD_AXIS_ZL),
                               g_pad ? k2shim_pad_axis(4 + (b.pad - PAD_AXIS_ZL)) : -1);
        else
            __builtin_snprintf(pOut, nOut, "pad button %d", b.pad);
    }
    if (nSrc & SRC_KEY) {
        int n = static_cast<int>(__builtin_strlen(pOut));
        __builtin_snprintf(pOut + n, nOut - n, "%skey scancode %d", n ? " + " : "", b.key);
    }
}

int source_of(const Binding& b, const unsigned char* keys, int nKeys) {
    int nSrc = 0;
    if (!g_padDisabled[b.act] && pad_down(b.pad)) nSrc |= SRC_PAD;
    if (keys && b.key < nKeys && keys[b.key]) nSrc |= SRC_KEY;
    return nSrc;
}

// Push one keyboard event into the engine's pending-event queue, exactly where a real key lands.
void inject_key(int nKey) {
    if (!g_pExoInput || !g_pExoInput->m_pInternal) return;
    g_pExoInput->m_pInternal->BufferEvent(DEV_KEYBOARD, static_cast<unsigned>(nKey), 1);
}

CClientExoAppInternal* app_internal() {
    if (!g_pAppManager || !g_pAppManager->m_pClientExoApp) return 0;
    return g_pAppManager->m_pClientExoApp->m_pInternal;
}

CGuiInGame* in_game_gui(CClientExoAppInternal* pInternal) {
    return pInternal ? pInternal->m_pInGameGui : 0;
}

// The live conversation panel (CSWGuiDialogCinematic or CSWGuiDialogComputer), or 0.
CSWGuiPanel* active_dialog(CGuiInGame* pGui) {
    if (!pGui || pGui->m_dialogInputActive == 0) return 0;
    return static_cast<CSWGuiPanel*>(pGui->m_pActiveDialogPanel);
}

// Every d-pad / A press during a conversation, under K2_CONSOLE=2: the state each engine gate
// on the way to the reply list reads (ProcessInput's GUI rule: toggle 1 or input class 2/3, and
// camera mode 5 diverts; the manager's top modal; CSWGuiDialog's line-up bit 0 drops the press).
// The next tick reports what the GUI manager was handed in between.
int g_nDialogPressPending = -1;   // the action id logged last tick, -1 none

void dialog_probe(Action act, CSWGuiPanel* pDialog, CSWGuiManager* pGuiManager,
                  CClientExoAppInternal* pInternal, int nClass, int nToggle) {
    if (!pDialog || g_mode < 2) return;
    if (act != ACT_ENTRY_PREV && act != ACT_ENTRY_NEXT && act != ACT_ACCEPT) return;
    CSWGuiDialog* p = static_cast<CSWGuiDialog*>(pDialog);
    const int nModal = pGuiManager->m_modalPanels.m_count;
    CSWGuiPanel* pTop = nModal > 0 ? pGuiManager->m_modalPanels.m_pArray[nModal - 1] : 0;
    CGuiInGame* pGui = in_game_gui(pInternal);
    LOGI("dialog press %s: cls=%d tgl=%d cam=%d gate1ac=%d modal=%d top=%p%s dlg=%p flags=0x%x "
         "cursor=%d replies=%d listSel=%d listVis=%d",
         kActionNames[act], nClass, nToggle,
         pInternal && pInternal->m_pClientOptions ? pInternal->m_pClientOptions->m_nCameraMode : -1,
         pGui ? pGui->m_bControllerPauseActive : -1, nModal, static_cast<void*>(pTop), pTop == pDialog ? "(=dlg)" : "",
         static_cast<void*>(pDialog), p->m_nMessageFlags, p->m_nSelectedReply, p->m_nReplyCount, p->m_listBox.m_nSelectedIndex,
         p->m_listBox.m_tail.bVisible);
    g_nDialogPressPending = act;
}

// A conversation owns the input: push it onto the modal stack, so CSWGuiManager::HandleInputEvent
// stops broadcasting the same press to the HUD underneath.
void ensure_dialog_modal(CSWGuiManager* pGuiManager, CGuiInGame* pGui) {
    CSWGuiPanel* pDialog = g_bDialogModal ? active_dialog(pGui) : 0;

    if (g_pPushedModal && g_pPushedModal != pDialog) {
        if (pGuiManager->PanelExists(g_pPushedModal) &&
            pGuiManager->GetPosInModalStack(g_pPushedModal) != -1) {
            if (pGuiManager->GetPosInModalStack(g_pPushedModal) == pGuiManager->m_modalPanels.m_count - 1)
                pGuiManager->PopModalPanel();
        }
        g_pPushedModal = 0;
    }
    if (!pDialog) return;
    if (!pGuiManager->PanelExists(pDialog)) {
        if (g_mode >= 2) LOGI("conversation %p is not a live panel yet", static_cast<void*>(pDialog));
        return;
    }
    if (pGuiManager->GetPosInModalStack(pDialog) != -1) {
        if (g_pPushedModal != pDialog) LOGI("conversation %p was already modal", static_cast<void*>(pDialog));
        g_pPushedModal = pDialog;
        return;
    }
    pGuiManager->PushModalPanel(pDialog);
    g_pPushedModal = pDialog;
    LOGI("conversation %p pushed modal (modal count now %d) -- the HUD underneath will no longer "
         "see the same press", static_cast<void*>(pDialog), pGuiManager->m_modalPanels.m_count);
}

// Suppresses the mobile pop-up action list, which has no place on the console HUD. While it is up
// the HUD's accept arm returns on its first line, so A stops selecting the entry the d-pad cycled
// to.
void suppress_action_menu(CSWGuiManager* pGuiManager, CSWGuiMainInterface* pMain) {
    if (!pMain) { g_bPopupSeen = 0; return; }
    int bUp = pGuiManager->PanelExists(&pMain->m_actionMenuIos) != 0;
    if (bUp && !g_bPopupSeen)
        LOGI("pop-up action menu appeared (manager's last event id 0x%x, sel=%d)%s",
             pGuiManager->m_nCurrentInputEvent, g_nSelectedAction, g_bKeepPopup ? "" : " -- removing");
    g_bPopupSeen = bUp;
    if (bUp && !g_bKeepPopup) {
        pMain->RemoveActionMenu();
        // PopupActionMenu paused the game unless it already was; its own close undoes that, and
        // RemoveActionMenu does not.
        if (!pMain->m_actionMenuIos.m_bWasPausedByCombat)
            g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);
    }
}

// Every pad edge, under K2_CONSOLE=2: both the SDL_GameController view this driver binds and the
// raw SDL_Joystick view.
void pad_trace(void) {
    if (g_mode != 2 || !g_pad) return;
    unsigned nBtns = 0;
    for (int i = 0; i < 15; ++i) if (k2shim_pad_button(i)) nBtns |= 1u << i;
    if (k2shim_pad_axis(4) > kTriggerOn) nBtns |= 1u << 15;   // ZL
    if (k2shim_pad_axis(5) > kTriggerOn) nBtns |= 1u << 16;   // ZR
    unsigned nRaw = 0;
    void* pJoy = SDL_GameControllerGetJoystick(g_pad);
    if (pJoy) {
        int n = SDL_JoystickNumButtons(pJoy);
        if (n > 32) n = 32;
        for (int i = 0; i < n; ++i) if (SDL_JoystickGetButton(pJoy, i)) nRaw |= 1u << i;
    }
    static unsigned sBtns, sRaw;
    if (nBtns == sBtns && nRaw == sRaw) return;
    sBtns = nBtns; sRaw = nRaw;
    LOGI("pad: controller=0x%05x raw=0x%08x   (controller bits: A0 B1 X2 Y3 Back4 Guide5 Start6 "
         "LS7 RS8 L9 R10 dU11 dD12 dL13 dR14 ZL15 ZR16)", nBtns, nRaw);
}

// the in-place entry cycle: the one action with no engine event id, so it is performed.
void cycle_entry(CSWGuiMainInterface* pMain, bool bNext) {
    if (!pMain) return;
    int nSel = g_nSelectedAction;
    if (nSel < 0)
        return;                       // nothing is selected: up/down has no box to work on
    CSWGuiMainInterfaceAction* pWidget = pMain->GetActionByIndex(nSel);
    CResRef sOld = pWidget->m_icon.m_border.m_params.m_fillImage;
    float fOldAlpha = pWidget->m_icon.m_border.m_params.m_fAlpha;
    if (nSel <= 5) {
        if (bNext) pMain->SelectNextPersonalAction(nSel);
        else       pMain->SelectPrevPersonalAction(nSel);
    } else {
        if (bNext) pMain->m_targetActionMenu.SelectNextAction(nSel - 6);
        else       pMain->m_targetActionMenu.SelectPrevAction(nSel - 6);
    }
    pMain->PopulateMenus(0);         // re-skin every widget from the sticky ids just moved
    K2ConsoleStartScroll(pMain, nSel, bNext, sOld, fOldAlpha);   // the Switch's scroll helper
}

// Selects box 1 on a new target, the Switch's auto-select, only on retarget.
void autoslot_on_retarget(CSWGuiMainInterface* pMain, bool bHudOwnsInput) {
    static unsigned long long sLastTarget = kObjectInvalid;
    if (!pMain) return;
    const unsigned long long nTarget = pMain->m_targetId;
    if (nTarget == sLastTarget) return;
    sLastTarget = nTarget;
    if (!g_bAutoSlot || !bHudOwnsInput || nTarget == kObjectInvalid) return;

    pMain->PopulateMenus(0);
    CSWGuiMainInterfaceAction* pWidget = pMain->GetActionByIndex(kTargetSlot);
    if (!pWidget || pWidget->m_bEmpty) {
        if (g_mode >= 2) LOGI("retarget 0x%llx: target column empty, sel stays %d", nTarget, g_nSelectedAction);
        return;
    }
    // The caption follows the caption index, not the selection. Selecting the widget here fires no
    // focus event, so without this the bar showed box 1 selected under the old box's name.
    pMain->m_nFocusedAction = kTargetCaptionIdx;
    if (g_nSelectedAction == kTargetSlot) return;
    const int nWas = g_nSelectedAction;
    pMain->ClearSelectedAction();
    g_nSelectedAction = kTargetSlot;
    pWidget->SetSelected(true);
    LOGI("retarget 0x%llx: sel %d -> %d", nTarget, nWas, kTargetSlot);
}

// The bar always keeps a box selected, as the Switch's and K1's console bars do. The HUD coming back
// after a gap in ticking (a load, an area transition) selects box 1, as K1's bar does after a load (a
// load also rebuilds the widgets unselected while the selected-slot index keeps its value, so the bar
// looked empty and A cleared instead of performing). With no box selected (the engine's
// ClearSelectedAction: PopulateMenus when the box emptied, A on a dimmed box) the selected box is
// shown again if it still has an entry, otherwise the first box with an entry is selected: box 1,
// which always holds at least "No Action". B never clears it.
bool g_bWantDefaultSlot = true;
bool g_bWantBox1 = true;

void select_slot(CSWGuiMainInterface* pMain, CSWGuiMainInterfaceAction* pWidget, int nIndex) {
    pMain->ClearSelectedAction();
    g_nSelectedAction = nIndex;
    pWidget->SetSelected(true);
    pMain->SetActiveControl(&pWidget->m_icon, 0);   // the caption's box, as a d-pad step sets it
    pMain->m_nFocusedAction = nIndex == kTargetSlot ? kTargetCaptionIdx : nIndex;
    g_bWantDefaultSlot = false;
    g_bWantBox1 = false;
}

void default_slot(CSWGuiMainInterface* pMain, bool bHudOwnsInput, bool bStalled) {
    if (!pMain) return;
    const int nSel = g_nSelectedAction;
    if (bStalled) g_bWantBox1 = true;
    if (g_bWantBox1 || nSel < 0) g_bWantDefaultSlot = true;
    if (!g_bWantDefaultSlot || !bHudOwnsInput) return;

    if (nSel >= 0 && !g_bWantBox1) {
        CSWGuiMainInterfaceAction* pWidget = pMain->GetActionByIndex(nSel);
        if (pWidget && !pWidget->m_bEmpty) {
            if (!pWidget->m_bSelected) {
                select_slot(pMain, pWidget, nSel);
                LOGI("default slot: sel %d shown again", nSel);
            }
            g_bWantDefaultSlot = false;
            return;
        }
    }
    static const int kBarOrder[7] = { kTargetSlot, 0, 1, 2, 3, 4, 5 };   // box 1, then boxes 2..7
    for (int k = 0; k < 7; ++k) {
        CSWGuiMainInterfaceAction* pWidget = pMain->GetActionByIndex(kBarOrder[k]);
        if (!pWidget || pWidget->m_bEmpty) continue;
        select_slot(pMain, pWidget, kBarOrder[k]);
        LOGI("default slot: sel %d -> %d", nSel, kBarOrder[k]);
        return;
    }
}

// The mouse on the bar: pointing at a box selects it, as a d-pad step does (the engine's own hover
// only moves the caption). Polled from the manager's hover control when it changes, so a pointer
// resting on a box does not undo d-pad steps, and a pad press parks the pointer off screen.
void hover_slot(CSWGuiMainInterface* pMain, CSWGuiManager* pGuiManager, bool bHudOwnsInput) {
    static CSWGuiControl* s_pLast;
    if (!pMain || !pGuiManager) return;
    CSWGuiControl* pOver = pGuiManager->m_pMouseOverControl;
    if (pOver == s_pLast) return;
    s_pLast = pOver;
    if (!pOver || !bHudOwnsInput) return;
    static const int kBarOrder[7] = { kTargetSlot, 0, 1, 2, 3, 4, 5 };
    for (int k = 0; k < 7; ++k) {
        CSWGuiMainInterfaceAction* pWidget = pMain->GetActionByIndex(kBarOrder[k]);
        if (!pWidget || pWidget->m_bEmpty) continue;
        if (pOver != &pWidget->m_button && pOver != &pWidget->m_icon && pOver != &pWidget->m_btnUp &&
            pOver != &pWidget->m_btnDown)
            continue;
        if (g_nSelectedAction != kBarOrder[k]) {
            select_slot(pMain, pWidget, kBarOrder[k]);
            pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            if (g_mode >= 2) LOGI("hover: sel -> %d", kBarOrder[k]);
        }
        return;
    }
}

bool engine_just_sent(int nId, int64_t tMs) {
    if (static_cast<unsigned>(nId) >= 256) return false;
    return g_engineEventMs[nId] != 0 && (tMs - g_engineEventMs[nId]) < kDupWindowMs;
}

// The selection and the nine widgets' skip/selected flags on both sides of a bar dispatch
// (K2_CONSOLE=2): names whether "the bar jumped two boxes" was two events or one skipped slot.
void bar_trace(CSWGuiMainInterface* pMain, const char* when) {
    if (!pMain || g_mode != 2) return;
    unsigned nSkip = 0, nSel = 0;
    for (int i = 0; i < 9; ++i) {
        CSWGuiMainInterfaceAction* pAction = pMain->GetActionByIndex(i);
        if (!pAction) continue;
        if (pAction->m_bEmpty)  nSkip |= 1u << i;
        if (pAction->m_bSelected) nSel  |= 1u << i;
    }
    LOGI("  bar %-6s sel=%d  skip=0x%03x sel'd=0x%03x", when, g_nSelectedAction, nSkip, nSel);
}

CSWCCreature* player_creature() {
    if (!g_pAppManager || !g_pAppManager->m_pClientExoApp) return 0;
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    return pParty ? pParty->GetPlayerCharacter() : 0;
}

// The DirectInput index the shim gives an SDL pad source (the bit k2console_dishim_mask publishes
// for it), 16/17 for the two trigger axes, -1 for none.
int di_bit_of(unsigned char nPad) {
    static const signed char kSdlToDi[15] = { 0, 1, 2, 3, 6, 10, 7, 8, 9, 4, 5, 12, 14, 15, 13 };
    if (nPad == PAD_AXIS_ZL) return 16;
    if (nPad == PAD_AXIS_ZR) return 17;
    if (nPad < 15) return kSdlToDi[nPad];
    return -1;
}

// Logs the player's pending round actions (the number A, X and Y are all about) and in-combat bit
// whenever they change.
void queue_probe(CSWGuiManager* pGuiManager) {
    int nQueue = -1, nCombat = -1;
    CSWCCreature* pPC = player_creature();
    if (pPC) {
        nCombat = pPC->m_nStateFlags & 1;
        CSWSCreature* pServer = pPC->GetServerCreature();
        CSWSCombatRound* pRound = pServer ? pServer->m_pCombatRound : 0;
        CExoLinkedList<CSWSCombatRoundAction>* pList = pRound ? pRound->m_pRoundActionList : 0;
        if (pList && pList->m_pInternal) nQueue = static_cast<int>(pList->m_pInternal->m_nCount);
    }
    static int sQueue = -2, sCombat = -2;
    if (nQueue == sQueue && nCombat == sCombat) return;
    LOGI("queue: %d -> %d  combat=%d  sel=%d cur=0x%x", sQueue, nQueue, nCombat, g_nSelectedAction,
         pGuiManager->m_nCurrentInputEvent);
    sQueue = nQueue;
    sCombat = nCombat;
}

void dispatch(Action act, int nSrc, bool bEngineOwns, bool bHudOwnsInput, int64_t tMs,
              CSWGuiMainInterface* pMain, CSWGuiManager* pGuiManager) {
    const Binding& b = kBindings[act];

    // the engine is already producing this press.
    if (bEngineOwns) {
        if (nSrc & SRC_KEY) return;            // ...from the real key: nothing to do, ever.
        inject_key(b.engineKey);               // ...but not from the pad: hand it the raw key.
        return;
    }

    // the engine is silent for this key in this class: we are the only producer.
    int nGuiId = -1;
    switch (act) {
    case ACT_BOX_PREV:   nGuiId = 0x2f; break;
    case ACT_BOX_NEXT:   nGuiId = 0x30; break;
    case ACT_ACCEPT:     nGuiId = 0x27; break;   // A and X are both 0x27; A clears the queue first
    case ACT_QUEUE_ADD:  nGuiId = 0x27; break;
    case ACT_CANCEL:     nGuiId = 0x28; break;
    default: break;
    }
    if (nGuiId >= 0) {
        if (engine_just_sent(nGuiId, tMs)) {
            LOGI("  suppressed 0x%x -- the engine just sent it", nGuiId);
            return;
        }
        const bool bBar = (nGuiId == 0x2f || nGuiId == 0x30 || nGuiId == 0x27);
        if (bBar) bar_trace(pMain, "before");
        // A = the Switch's perform body: ClearAllQueuedCombatActions first, then perform. Only on
        // the HUD: under a modal the accept event is that panel's.
        if (act == ACT_ACCEPT && bHudOwnsInput) {
            CSWCCreature* pPC = player_creature();
            if (pPC) pPC->ClearAllQueuedCombatActions();
            if (g_mode >= 2) LOGI("  A: ClearAllQueuedCombatActions on %p before 0x27", static_cast<void*>(pPC));
        }
        pGuiManager->HandleInputEvent(nGuiId, 1);
        LOGI("  gui 0x%x -> manager cur=0x%x sel=%d modal=%d", nGuiId,
             pGuiManager->m_nCurrentInputEvent, g_nSelectedAction, pGuiManager->m_modalPanels.m_count);
        if (bBar) bar_trace(pMain, "after");
        return;
    }
    switch (act) {
    // Y clears the queue with the engine's own calls. ClearAllActions(1) empties the action list
    // and the round's attacks but not the round's queued actions (m_pRoundActionList), which the
    // next X re-shows; ClearAllQueuedCombatActions (what A and the PC HUD call) empties those.
    case ACT_QUEUE_CLEAR: {
        if (!bHudOwnsInput) return;
        CSWCCreature* pPC = player_creature();
        if (!pPC) return;
        pPC->ClearAllQueuedCombatActions();
        CSWSCreature* pServer = pPC->GetServerCreature();
        if (pServer) pServer->ClearAllActions(1);
        pGuiManager->PlayGuiSound(0);
        if (g_mode >= 2)
            LOGI("  queue clear (ClearAllQueuedCombatActions + CSWSObject::ClearAllActions(1) on %p)",
                 static_cast<void*>(pServer));
        return;
    }

    // the app-level events: CClientExoApp::HandleInputEvent, not the GUI manager.
    //   1 free look, 2 the combat-pause toggle, 5/6 next/previous target (measured: 5 is next),
    //   9 next character, the solo-mode query, the in-game menu
    case ACT_MENU:
    case ACT_PAUSE:
    case ACT_TGT_PREV:
    case ACT_TGT_NEXT:
    case ACT_NEXTCHAR:
    case ACT_SOLO:
    case ACT_FREELOOK: {
        if (!g_pAppManager || !g_pAppManager->m_pClientExoApp) return;
        const int nId = act == ACT_PAUSE    ? 2
                      : act == ACT_MENU     ? 0xb
                      : act == ACT_TGT_NEXT ? 5
                      : act == ACT_TGT_PREV ? 6
                      : act == ACT_NEXTCHAR ? 9
                      : act == ACT_SOLO     ? 0xa
                      :                       1;
        g_pAppManager->m_pClientExoApp->HandleInputEvent(nId, 1);
        return;
    }

    case ACT_ENTRY_PREV:
    case ACT_ENTRY_NEXT:
        if (!bHudOwnsInput) return;
        cycle_entry(pMain, act == ACT_ENTRY_NEXT);
        return;

    default: return;
    }
}

// K2_CONSOLE_INJECT=NAME[:n] (debug.k2.console.inject): fire the named action once, as if the pad
// had pressed it, whenever the value changes (the ownership gate applies).
void inject_probe(bool bHudOwnsInput, int64_t tMs, CSWGuiMainInterface* pMain, CSWGuiManager* pGuiManager) {
    static int64_t sNextMs;
    static char sLast[92];
    if (tMs < sNextMs) return;
    sNextMs = tMs + 200;
    char v[92] = {0};
    if (k2console_prop("debug.k2.console.inject", v, sizeof(v)) <= 0 || !v[0]) return;
    if (!__builtin_strcmp(v, sLast)) return;
    __builtin_memcpy(sLast, v, sizeof(sLast));
    sLast[sizeof(sLast) - 1] = 0;
    char* pColon = __builtin_strchr(v, ':');
    if (pColon) *pColon = 0;
    for (int i = 0; i < ACT_COUNT; ++i) {
        if (__builtin_strcmp(v, kActionNames[i])) continue;
        const Binding& b = kBindings[i];
        const bool bOurs = bHudOwnsInput && b.owner == OWN_DRIVER;
        LOGI("inject %s (hud=%d owner=%s)%s", kActionNames[i], bHudOwnsInput ? 1 : 0,
             b.owner == OWN_DRIVER ? "driver" : "engine", bOurs ? "" : " -- the engine's, not sent");
        if (bOurs) dispatch(static_cast<Action>(i), SRC_PAD, false, bHudOwnsInput, tMs, pMain, pGuiManager);
        return;
    }
    LOGI("inject \"%s\": no such action", v);
}

// The pause-banner state, whenever any of it changes.
void pause_probe(CGuiInGame* pGui) {
    if (!pGui) return;
    const CSWGuiPanel* pPanel = pGui->m_pPauseGuiPanel;
    const CSWGuiExtent* e = pPanel ? &pPanel->m_extent : 0;
    static int sLast[7] = { -1, -1, -1, -1, -1, -1, -1 };
    const int now[7] = { pGui->m_bInitialized, pGui->m_swGuiToggleState, pGui->m_bPauseGuiShown,
                         pPanel ? 1 : 0, e ? e->x : 0, e ? e->y : 0, e ? e->w : 0 };
    for (int i = 0; i < 7; ++i) {
        if (now[i] == sLast[i]) continue;
        for (int k = 0; k < 7; ++k) sLast[k] = now[k];
        if (g_mode >= 2)
            LOGI("pause: gate(+0x1ec)=%d swGuiToggle(+0x58)=%d cached(+0x106c)=%d panel=%p "
                 "extent=(%d,%d %dx%d)", now[0], now[1], now[2], static_cast<const void*>(pPanel),
                 e ? e->x : -1, e ? e->y : -1, e ? e->w : -1, e ? e->h : -1);
        return;
    }
}

}  // namespace

// The mouse on a box's arrows or wheel (CSWGuiMainInterface's arrow handlers): selects the box and
// steps its entries, as the d-pad does.
void k2console_mouse_step(CSWGuiMainInterface* pMain, int nIndex, bool bNext) {
    CSWGuiMainInterfaceAction* pWidget = pMain ? pMain->GetActionByIndex(nIndex) : 0;
    if (!pWidget || pWidget->m_bEmpty) return;
    if (g_nSelectedAction != nIndex) select_slot(pMain, pWidget, nIndex);
    cycle_entry(pMain, bNext);
}

extern "C" void k2console_tick(void) {
    CSWGuiManager* pGuiManager = g_pGuiMan;
    CSWGuiMainInterface* pMainInterface = CSWGuiFloatyText::m_pMainInterface;
    if (!pGuiManager) return;

    int64_t tMs = k2console_now_ms();
    if (tMs - g_lastTickMs < 8 && tMs >= g_lastTickMs) return;   // once per frame is enough
    // After any gap in ticking (the stall guard), everything held must be released before it fires.
    const bool bStalled = (g_lastTickMs == 0) || (tMs - g_lastTickMs > 250);
    g_lastTickMs = tMs;
    g_dishimMask = 0;               // re-published below once the HUD's ownership is known
    g_dishimMaskMs = tMs;
    if (g_mode < 0 || tMs >= g_nextModeCheckMs) {
        g_mode = prop_int("debug.k2.console", 1);
        if (g_mode < 0 || g_mode > 3) g_mode = 1;
        g_bKeepPopup   = prop_int("debug.k2.console.popup", 0) != 0;
        g_bDialogModal = prop_int("debug.k2.console.modal", 1) != 0;
        g_bEngineRule  = prop_int("debug.k2.console.engine", 0) != 0;
        g_bAutoSlot    = prop_int("debug.k2.console.autoslot", 1) != 0;
        g_nDishimProp  = prop_int("debug.k2.dishim", 0);
        g_nRepeatDelayMs = 100 * prop_int("debug.k2.console.repeat", 0);
        apply_slot_order();
        g_nextModeCheckMs = tMs + 1000;
    }
    if (g_mode == 0) return;

    CClientExoAppInternal* pInternal = app_internal();
    CGuiInGame* pGui = in_game_gui(pInternal);
    pause_probe(pGui);
    {
        const int nJoy = IsJoystickConnected() ? 1 : 0;
        static int sJoy = -1;
        if (nJoy != sJoy) { sJoy = nJoy; LOGI("engine: IsJoystickConnected()=%d", nJoy); }
    }

    // The input trace treats anything but the sentinel in m_nCurrentInputEvent as handed to the GUI
    // manager by a producer that is not this driver.
    {
        const int nCur = pGuiManager->m_nCurrentInputEvent;
        if (nCur != kNoEventSentinel) {
            if (static_cast<unsigned>(nCur) < 256) g_engineEventMs[nCur] = tMs;
            if (g_mode >= 2) LOGI("gui manager saw event 0x%x (not ours)", nCur);
        }
        if (g_nDialogPressPending >= 0 && g_nDialogPressPending < ACT_COUNT) {
            if (nCur != kNoEventSentinel) LOGI("  -> gui manager got 0x%x", nCur);
            else LOGI("  -> gui manager got NOTHING since the press");
        }
        g_nDialogPressPending = -1;
        pGuiManager->m_nCurrentInputEvent = kNoEventSentinel;
    }

    // the engine state the one-producer rule is derived from.
    // ProcessInput hands a keyboard action key to the GUI manager only when
    // `m_swGuiToggleState != 1 && (m_inputClass & ~1) == 2`.
    const int nClass = pInternal ? pInternal->m_inputClass : 0;
    const int nToggle = pGui ? pGui->m_swGuiToggleState : 0;
    const bool bEngineGui = (nToggle != 1) && ((nClass & ~1) == 2);
    const bool bModal = pGuiManager->m_modalPanels.m_count != 0;
    CSWGuiPanel* pDialog = active_dialog(pGui);

    if (g_mode == 3) return;        // trace only: let the engine be the whole input system

    ensure_dialog_modal(pGuiManager, pGui);
    suppress_action_menu(pGuiManager, pMainInterface);

    // The action bar owns the input only when nothing is on top of it.
    const bool bHudOwnsInput = !bModal && (pDialog == 0) && (nToggle == 1);
    // An in-game menu screen (Equip .. Options) with nothing on top: the triggers page its tabs
    // and RB changes the party member, so the shoulders and triggers are this driver's there.
    const bool bMenuOwnsInput = pGui && !bModal && (pDialog == 0) && (nToggle != 1);

    // One press must not act on two screens: whatever is held when the owner changes is latched
    // off until it is released.
    const int nOwner = (bModal ? 1 : 0) | (pDialog ? 2 : 0);
    const bool bOwnerChanged = (g_lastOwner != nOwner);
    g_lastOwner = nOwner;

    refresh_pad(tMs);
    pad_trace();
    queue_probe(pGuiManager);

    // Publish the claim; the shim reads it on the engine's next poll.
    if (bHudOwnsInput) {
        unsigned nMask = 0;
        for (int i = 0; i < ACT_COUNT; ++i) {
            const Binding& b = kBindings[i];
            if (b.owner != OWN_DRIVER || g_padDisabled[i]) continue;
            const int nBit = di_bit_of(b.pad);
            if (nBit >= 0) nMask |= 1u << nBit;
        }
        g_dishimMask = nMask;
    } else if (bMenuOwnsInput) {
        g_dishimMask = (1u << di_bit_of(PAD_L)) | (1u << di_bit_of(PAD_R)) |
                       (1u << di_bit_of(PAD_AXIS_ZL)) | (1u << di_bit_of(PAD_AXIS_ZR));
    }
    {
        static unsigned sLastMask = ~0u;
        if (g_dishimMask != sLastMask) {
            sLastMask = g_dishimMask;
            if (g_mode >= 2) LOGI("shim mask -> 0x%05x (hud=%d)", g_dishimMask, bHudOwnsInput ? 1 : 0);
        }
    }

    int nKeys = 0;
    const unsigned char* keys = SDL_GetKeyboardState(&nKeys);

    for (int i = 0; i < ACT_COUNT; ++i) {
        const Binding& b = kBindings[i];
        const int nSrc = source_of(b, keys, nKeys);
        if (!nSrc) {                        // released: clear the latch and the suppression
            g_held[i] = false;
            g_suppress[i] = false;
            continue;
        }
        if (bStalled || bOwnerChanged)
            g_suppress[i] = true;
        const bool bWasHeld = g_held[i];
        g_held[i] = true;
        if (!bWasHeld) dialog_probe(static_cast<Action>(i), pDialog, pGuiManager, pInternal, nClass, nToggle);
        if (g_suppress[i]) continue;

        bool fire = false;
        if (!bWasHeld) {
            fire = true;
            g_nextFireMs[i] = tMs + g_nRepeatDelayMs;
        } else if (b.repeat && g_nRepeatDelayMs > 0 && tMs >= g_nextFireMs[i]) {
            fire = true;
            g_nextFireMs[i] = tMs + kRepeatRateMs;
        }
        if (!fire) continue;

        // In a menu screen LT / RT page the tabs, RB is the next party member, LB nothing.
        if (bMenuOwnsInput && !(nSrc & SRC_KEY) &&
            (b.pad == PAD_L || b.pad == PAD_R || b.pad == PAD_AXIS_ZL || b.pad == PAD_AXIS_ZR)) {
            const int nMenuId = b.pad == PAD_AXIS_ZL ? 0x35 : b.pad == PAD_AXIS_ZR ? 0x36
                              : b.pad == PAD_R ? 0xce : -1;
            LOGI("press %s in a menu -> gui 0x%x", kActionNames[i], nMenuId);
            if (nMenuId >= 0) pGuiManager->HandleInputEvent(nMenuId, 1);
            continue;
        }
        // For the pad half of the one-producer rule, this driver produces from the pad only what
        // the shim has taken away from the engine, an OWN_DRIVER row while the HUD owns the input.
        if (!(nSrc & SRC_KEY) && !(bHudOwnsInput && b.owner == OWN_DRIVER)) {
            if (g_mode >= 2) {
                char sSrc[64];
                describe_source(b, nSrc, sSrc, sizeof(sSrc));
                LOGI("press %s <- %s  -- the engine's (hud=%d owner=%s), not sent",
                     kActionNames[i], sSrc, bHudOwnsInput ? 1 : 0,
                     b.owner == OWN_DRIVER ? "driver" : "engine");
            }
            continue;
        }
        const bool bEngineOwns = g_bEngineRule && (b.engineKey != KB_NONE) &&
                                 ((b.engineClasses >> (nClass & 31)) & 1) && bEngineGui;
        {
            char sSrc[64];
            describe_source(b, nSrc, sSrc, sizeof(sSrc));
            if (g_mode >= 2)
                LOGI("press %s <- %s  (cls=%d tgl=%d engineGui=%d engine=%d hud=%d sel=%d dlg=%p)",
                     kActionNames[i], sSrc, nClass, nToggle, bEngineGui ? 1 : 0,
                     bEngineOwns ? 1 : 0, bHudOwnsInput ? 1 : 0, g_nSelectedAction, static_cast<void*>(pDialog));
            else
                LOGI("press %s <- %s", kActionNames[i], sSrc);
        }
        dispatch(static_cast<Action>(i), nSrc, bEngineOwns, bHudOwnsInput, tMs, pMainInterface, pGuiManager);
    }

    inject_probe(bHudOwnsInput, tMs, pMainInterface, pGuiManager);
    autoslot_on_retarget(pMainInterface, bHudOwnsInput);
    default_slot(pMainInterface, bHudOwnsInput, bStalled);
    hover_slot(pMainInterface, pGuiManager, bHudOwnsInput);

    // Re-arm, so the next tick does not mistake our own event for somebody else's.
    pGuiManager->m_nCurrentInputEvent = kNoEventSentinel;
}

// Answers the shim's question, once per engine poll of the pad: of the DirectInput inputs down
// right now, which does this driver produce itself? Latched per input from the frame it is first
// claimed until it is released, so a press that changes the HUD's ownership mid-way cannot reappear
// to the engine as a fresh press on the new screen.
//     K2_DISHIM=1   the engine sees the whole pad (mask 0 always)
//     K2_DISHIM=2   every button and trigger silent everywhere
extern "C" unsigned int k2console_dishim_mask(unsigned int nDown) {
    static unsigned sSticky;
    unsigned nMask;
    if (g_nDishimProp == 1) nMask = 0;
    else if (g_nDishimProp == 2) nMask = 0x3ffffu;
    else if (k2console_now_ms() - g_dishimMaskMs > kDishimMaskTtlMs) nMask = 0;   // not ticking
    else nMask = g_dishimMask;
    sSticky = (sSticky | nMask) & nDown;
    return sSticky;
}

#endif  // K2_UI_XBOX
