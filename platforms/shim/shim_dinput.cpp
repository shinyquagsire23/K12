// DirectInput 8 on SDL. It provides the keyboard and mouse devices CExoRawInputInternal polls, with
// the semantics of the macOS port's IDirectInputDeviceA_Mac* classes:
//   keyboard  GetDeviceState copies a 256-byte DIK state (the high bit set means down);
//             GetDeviceData reports the difference against the state at the previous GetDeviceData
//             call, one record per changed key (dwOfs = DIK code, dwData = the high bit on press)
//   mouse     GetDeviceState returns the motion accumulated since the last call (then zeroes it)
//             and the four button bytes; GetDeviceData drains a queue of axis (ofs 0/4, delta),
//             wheel (ofs 8) and button (ofs 12 + n, the high bit on press) records
//   joystick  every SDL game controller merged into one, the way the port's joystick class presents
//             it (see Joystick below): one device, a DIJOYSTATE2 snapshot per GetDeviceState
//
// The engine has its own pad bindings, so a pad reaching it here drives every screen by itself:
// `CExoInputInternal::GetEvents` ends with a hard-coded table that turns each changed DirectInput
// button / trigger / POV into GUI and APP event ids (A, B and the d-pad among them; the whole table
// is in platforms/console/k2console.cpp). That is the mobile layout, and it is what an Android
// player's pad does on every menu. The console HUD wants the Switch's layout on some of those
// buttons instead, so under K2P_UI=xbox the action-bar driver claims them while the HUD owns the
// input (`k2console_dishim_mask`) and they are reported at rest here; everywhere else the engine's
// bindings run untouched.
//
// K2_PAD=0 hides the pad from the engine (EnumDevices reports none), K2_PAD=1 exposes it. It
// defaults to on in the touch and xbox UI builds (a controller attached to the Android port) and
// off in the pc build, because IsJoystickConnected() flips the PC lineage onto console strings and
// the console name keyboard, which that UI was never built around.
#include <stdint.h>
#include "k2shim.h"

#include "exorawinputinternal.h"
#include "port/k2qolapi.h"

#include <SDL.h>
#include <stdlib.h>
#include <string.h>

#include <deque>
#include <mutex>
#include <vector>

extern int gResetController_Mac;   // src/port/Mac/MacMain.cpp: GetEvents' re-enumerate latch
// platforms/console/k2console.cpp: the DirectInput inputs the console action-bar driver produces
// itself right now (bit n = rgbButtons[n], 16 = lZ, 17 = lRz), given the ones that are down.
extern "C" unsigned int k2console_dishim_mask(unsigned int nDown);

namespace {

enum : int64_t {
    DI_OK = 0,
    DI_BUFFEROVERFLOW = 1,
    DIERR_NOTACQUIRED = static_cast<int64_t>(0x8007000c),
    DIERR_INPUTLOST = static_cast<int64_t>(0x8007001e),
    DIERR_INVALIDPARAM = static_cast<int64_t>(0x80070057),
    DIERR_DEVICENOTREG = static_cast<int64_t>(0x80040154),
    E_NOTIMPL_ = static_cast<int64_t>(0x80004001),
};

struct Guid { uint32_t a; uint16_t b, c; uint8_t d[8]; };
// The pad's instance GUID is 'K2PJ' + the SDL joystick instance id. CreateDevice is handed back
// what EnumDevices reported, and nothing else reads it.
const uint32_t kGuidPadTag = 0x4A50324B;

struct DiObjectData {                // DIDEVICEOBJECTDATA, 0x18 bytes on LP64
    uint32_t dwOfs, dwData, dwTimeStamp, dwSequence;
    uint64_t uAppData;
};
static_assert(sizeof(DiObjectData) == 0x18, "the engine passes cbObjectData = 0x18");

// SDL scancode -> DirectInput DIK code (the PC set-1 scancodes).
unsigned char DikFromScancode(SDL_Scancode s)
{
    switch (s) {
    case SDL_SCANCODE_ESCAPE: return 0x01;
    case SDL_SCANCODE_1: return 0x02; case SDL_SCANCODE_2: return 0x03; case SDL_SCANCODE_3: return 0x04;
    case SDL_SCANCODE_4: return 0x05; case SDL_SCANCODE_5: return 0x06; case SDL_SCANCODE_6: return 0x07;
    case SDL_SCANCODE_7: return 0x08; case SDL_SCANCODE_8: return 0x09; case SDL_SCANCODE_9: return 0x0A;
    case SDL_SCANCODE_0: return 0x0B; case SDL_SCANCODE_MINUS: return 0x0C; case SDL_SCANCODE_EQUALS: return 0x0D;
    case SDL_SCANCODE_BACKSPACE: return 0x0E; case SDL_SCANCODE_TAB: return 0x0F;
    case SDL_SCANCODE_Q: return 0x10; case SDL_SCANCODE_W: return 0x11; case SDL_SCANCODE_E: return 0x12;
    case SDL_SCANCODE_R: return 0x13; case SDL_SCANCODE_T: return 0x14; case SDL_SCANCODE_Y: return 0x15;
    case SDL_SCANCODE_U: return 0x16; case SDL_SCANCODE_I: return 0x17; case SDL_SCANCODE_O: return 0x18;
    case SDL_SCANCODE_P: return 0x19; case SDL_SCANCODE_LEFTBRACKET: return 0x1A; case SDL_SCANCODE_RIGHTBRACKET: return 0x1B;
    case SDL_SCANCODE_RETURN: return 0x1C; case SDL_SCANCODE_LCTRL: return 0x1D;
    case SDL_SCANCODE_A: return 0x1E; case SDL_SCANCODE_S: return 0x1F; case SDL_SCANCODE_D: return 0x20;
    case SDL_SCANCODE_F: return 0x21; case SDL_SCANCODE_G: return 0x22; case SDL_SCANCODE_H: return 0x23;
    case SDL_SCANCODE_J: return 0x24; case SDL_SCANCODE_K: return 0x25; case SDL_SCANCODE_L: return 0x26;
    case SDL_SCANCODE_SEMICOLON: return 0x27; case SDL_SCANCODE_APOSTROPHE: return 0x28; case SDL_SCANCODE_GRAVE: return 0x29;
    case SDL_SCANCODE_LSHIFT: return 0x2A; case SDL_SCANCODE_BACKSLASH: return 0x2B;
    case SDL_SCANCODE_Z: return 0x2C; case SDL_SCANCODE_X: return 0x2D; case SDL_SCANCODE_C: return 0x2E;
    case SDL_SCANCODE_V: return 0x2F; case SDL_SCANCODE_B: return 0x30; case SDL_SCANCODE_N: return 0x31;
    case SDL_SCANCODE_M: return 0x32; case SDL_SCANCODE_COMMA: return 0x33; case SDL_SCANCODE_PERIOD: return 0x34;
    case SDL_SCANCODE_SLASH: return 0x35; case SDL_SCANCODE_RSHIFT: return 0x36; case SDL_SCANCODE_KP_MULTIPLY: return 0x37;
    case SDL_SCANCODE_LALT: return 0x38; case SDL_SCANCODE_SPACE: return 0x39; case SDL_SCANCODE_CAPSLOCK: return 0x3A;
    case SDL_SCANCODE_F1: return 0x3B; case SDL_SCANCODE_F2: return 0x3C; case SDL_SCANCODE_F3: return 0x3D;
    case SDL_SCANCODE_F4: return 0x3E; case SDL_SCANCODE_F5: return 0x3F; case SDL_SCANCODE_F6: return 0x40;
    case SDL_SCANCODE_F7: return 0x41; case SDL_SCANCODE_F8: return 0x42; case SDL_SCANCODE_F9: return 0x43;
    case SDL_SCANCODE_F10: return 0x44; case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45; case SDL_SCANCODE_SCROLLLOCK: return 0x46;
    case SDL_SCANCODE_KP_7: return 0x47; case SDL_SCANCODE_KP_8: return 0x48; case SDL_SCANCODE_KP_9: return 0x49;
    case SDL_SCANCODE_KP_MINUS: return 0x4A; case SDL_SCANCODE_KP_4: return 0x4B; case SDL_SCANCODE_KP_5: return 0x4C;
    case SDL_SCANCODE_KP_6: return 0x4D; case SDL_SCANCODE_KP_PLUS: return 0x4E; case SDL_SCANCODE_KP_1: return 0x4F;
    case SDL_SCANCODE_KP_2: return 0x50; case SDL_SCANCODE_KP_3: return 0x51; case SDL_SCANCODE_KP_0: return 0x52;
    case SDL_SCANCODE_KP_PERIOD: return 0x53; case SDL_SCANCODE_NONUSBACKSLASH: return 0x56;
    case SDL_SCANCODE_F11: return 0x57; case SDL_SCANCODE_F12: return 0x58;
    case SDL_SCANCODE_KP_ENTER: return 0x9C; case SDL_SCANCODE_RCTRL: return 0x9D;
    case SDL_SCANCODE_KP_DIVIDE: return 0xB5; case SDL_SCANCODE_PRINTSCREEN: return 0xB7; case SDL_SCANCODE_RALT: return 0xB8;
    case SDL_SCANCODE_PAUSE: return 0xC5; case SDL_SCANCODE_HOME: return 0xC7; case SDL_SCANCODE_UP: return 0xC8;
    case SDL_SCANCODE_PAGEUP: return 0xC9; case SDL_SCANCODE_LEFT: return 0xCB; case SDL_SCANCODE_RIGHT: return 0xCD;
    case SDL_SCANCODE_END: return 0xCF; case SDL_SCANCODE_DOWN: return 0xD0; case SDL_SCANCODE_PAGEDOWN: return 0xD1;
    case SDL_SCANCODE_INSERT: return 0xD2; case SDL_SCANCODE_DELETE: return 0xD3;
    case SDL_SCANCODE_LGUI: return 0xDB; case SDL_SCANCODE_RGUI: return 0xDC; case SDL_SCANCODE_APPLICATION: return 0xDD;
    default: return 0;
    }
}

// Input state, fed from the SDL event pump (k2shim_dinput_event) on the main thread and read by
// whichever thread polls the devices.
std::mutex g_lock;
unsigned char g_keys[256];
int g_mouseDx, g_mouseDy, g_mouseDz;
unsigned char g_mouseButtons[8];
std::deque<DiObjectData> g_mouseQueue;
uint32_t g_sequence;

void QueueMouse(uint32_t ofs, uint32_t data)
{
    if (g_mouseQueue.size() >= 1024) g_mouseQueue.pop_front();
    DiObjectData d = { ofs, data, static_cast<uint32_t>(SDL_GetTicks()), ++g_sequence, 0 };
    g_mouseQueue.push_back(d);
}

class Keyboard : public IDirectInputDeviceA {
public:
    unsigned char m_prev[256];
    bool m_acquired = false;
    uint64_t m_ref = 1;
    Keyboard() { memset(m_prev, 0, sizeof m_prev); }

    uint64_t AddRef() override { return ++m_ref; }
    uint64_t Release() noexcept override { return --m_ref; }   // a process-lifetime singleton
    int64_t Acquire() override { m_acquired = true; return DI_OK; }
    int64_t Unacquire() noexcept override { m_acquired = false; return DI_OK; }
    int64_t SetDataFormat(const void*) override { return DI_OK; }
    int64_t SetCooperativeLevel(void*, unsigned int) override { return DI_OK; }
    int64_t SetProperty(unsigned int, const void*) override { return DI_OK; }

    // DIPROP_KEYNAME is the only property the engine reads. It answers the name of the key with DIK
    // code dwObj, as a 32-bit (K2WChar) string after the property header (CExoRawInputInternal::
    // GetDisplayCharacterDirectInputValue, which `stringtokens.2da` key tokens like <ActionUp> print
    // through). The macOS port's keyboard answers it with GetKeyNameTextW and 0; the name here is
    // SDL's for the key at that position in the current layout. The base class's E_NOTIMPL_ is not a
    // failure to the engine: it tests the sign of a 64-bit `long`, E_NOTIMPL is positive, so the
    // engine went on and converted the uninitialised wsz instead.
    int64_t GetProperty(unsigned int nProp, void* pHeader) override
    {
        struct Prop { uint32_t dwSize, dwHeaderSize, dwObj, dwHow; char32_t wsz[(0x420 - 0x10) / sizeof(char32_t)]; };
        static_assert(sizeof(Prop) == 0x420, "DIPROPSTRING, as the engine's DIPropStringLocal");
        Prop* p = static_cast<Prop*>(pHeader);
        if (!p || nProp != 0x14 || p->dwSize != 0x420 || p->dwHow != 1) return DIERR_INVALIDPARAM;
        p->wsz[0] = 0;
        for (int s = 0; p->dwObj && s < SDL_NUM_SCANCODES; ++s) {
            if (DikFromScancode(static_cast<SDL_Scancode>(s)) != p->dwObj) continue;
            const char* pszName = SDL_GetKeyName(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(s)));
            if (!*pszName) pszName = SDL_GetScancodeName(static_cast<SDL_Scancode>(s));
            // UTF-8 -> UTF-32 by hand, since SDL_iconv_utf8_ucs4 gives big-endian code points on
            // macOS.
            const unsigned char* q = reinterpret_cast<const unsigned char*>(pszName);
            size_t n = 0;
            while (*q && n + 1 < sizeof p->wsz / sizeof p->wsz[0]) {
                unsigned int c = *q++, nMore = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : c >= 0xc0 ? 1 : 0;
                if (nMore) c &= 0x3f >> nMore;
                for (; nMore && (*q & 0xc0) == 0x80; --nMore) c = (c << 6) | (*q++ & 0x3f);
                p->wsz[n++] = static_cast<char32_t>(c);
            }
            p->wsz[n] = 0;
            break;
        }
        return DI_OK;
    }

    int64_t GetDeviceState(unsigned int nSize, void* pData) override
    {
        if (!m_acquired) return DIERR_NOTACQUIRED;
        if (nSize != 0x100 || !pData) return DIERR_INVALIDPARAM;
        std::lock_guard<std::mutex> g(g_lock);
        memcpy(pData, g_keys, 0x100);
        return DI_OK;
    }

    int64_t GetDeviceData(unsigned int nObjSize, void* paData, unsigned int* pnCount, unsigned int) override
    {
        if (!nObjSize || !pnCount || !*pnCount) return DIERR_INVALIDPARAM;
        std::lock_guard<std::mutex> g(g_lock);
        DiObjectData* out = static_cast<DiObjectData*>(paData);
        if (out) memset(out, 0, static_cast<size_t>(nObjSize) * *pnCount);
        unsigned int n = 0;
        for (int k = 1; k < 256 && n < *pnCount; ++k) {
            if (g_keys[k] == m_prev[k]) continue;
            if (out) {
                out[n].dwOfs = static_cast<uint32_t>(k);
                out[n].dwData = g_keys[k] ? 0x80 : 0;
                out[n].dwTimeStamp = static_cast<uint32_t>(SDL_GetTicks());
                out[n].dwSequence = ++g_sequence;
            }
            ++n;
        }
        *pnCount = n;
        if (out) memcpy(m_prev, g_keys, sizeof m_prev);   // a peek (NULL buffer) does not consume
        return DI_OK;
    }
};

class Mouse : public IDirectInputDeviceA {
public:
    bool m_acquired = false;
    uint64_t m_ref = 1;

    uint64_t AddRef() override { return ++m_ref; }
    uint64_t Release() noexcept override { return --m_ref; }
    int64_t Acquire() override { m_acquired = true; return DI_OK; }
    int64_t Unacquire() noexcept override { m_acquired = false; return DI_OK; }
    int64_t SetDataFormat(const void*) override { return DI_OK; }
    int64_t SetCooperativeLevel(void*, unsigned int) override { return DI_OK; }
    int64_t SetProperty(unsigned int, const void*) override { return DI_OK; }

    int64_t GetDeviceState(unsigned int nSize, void* pData) override
    {
        if (!m_acquired) return DIERR_NOTACQUIRED;
        if (nSize < 0x10 || !pData) return DIERR_INVALIDPARAM;
        std::lock_guard<std::mutex> g(g_lock);
        memset(pData, 0, nSize);
        int32_t* p = static_cast<int32_t*>(pData);
        p[0] = g_mouseDx; p[1] = g_mouseDy; p[2] = g_mouseDz;
        g_mouseDx = g_mouseDy = g_mouseDz = 0;
        memcpy(static_cast<unsigned char*>(pData) + 12, g_mouseButtons, nSize >= 0x14 ? 8 : 4);
        return DI_OK;
    }

    int64_t GetDeviceData(unsigned int nObjSize, void* paData, unsigned int* pnCount, unsigned int) override
    {
        if (!m_acquired) return DIERR_NOTACQUIRED;
        if (!pnCount) return DIERR_INVALIDPARAM;
        std::lock_guard<std::mutex> g(g_lock);
        DiObjectData* out = static_cast<DiObjectData*>(paData);
        unsigned int n = 0;
        if (out) memset(out, 0, static_cast<size_t>(nObjSize) * *pnCount);
        while (n < *pnCount && !g_mouseQueue.empty()) {
            if (out) out[n] = g_mouseQueue.front();
            if (out) g_mouseQueue.pop_front();
            ++n;
        }
        int64_t r = (!g_mouseQueue.empty() && out) ? DI_BUFFEROVERFLOW : DI_OK;
        *pnCount = n;
        return r;
    }
};

// The pad is the port's IDirectInputDeviceA_MacJoystick, in the parts the engine calls:
// CExoRawInputInternal::InitJoysticks (CreateDevice, QueryInterface, GetCapabilities bit 0 =
// attached), ReinitJoysticks (SetDataFormat), CheckControllerType (GetDeviceInfo: the product name
// picks the DualShock glyphs) and GetJoystickBuffer (Poll + GetDeviceState every frame; a failure
// there zeroes m_nNumJoysticks, i.e. IsJoystickConnected()).
//
// One pad, always. GetJoystickBuffer writes `m_nNumJoysticks = 1` on every successful poll, so the
// engine is built around exactly one; EnumDevices reports the first SDL game controller only (its
// name picks the glyphs), and its state, like the console driver's, is every pad merged.
//
// GetDeviceState fills a DIJOYSTATE2 the same way the macOS port's does: six SDL axes into
// lX/lY/lZ/lRx/lRy/lRz, the fifteen SDL buttons into the Xbox DirectInput order, then the console
// driver's claim zeroed out.
bool PadEnabled()
{
#if defined(K2_UI_TOUCH) || defined(K2_UI_XBOX)
    const bool bDefault = true;
#else
    const bool bDefault = false;
#endif
    static const bool s_bPad = [bDefault] {
        const char* v = getenv("K2_PAD");
        return v ? (*v && strcmp(v, "0") != 0) : bDefault;
    }();
    return s_bPad;
}

// Steam Input. A game Steam launches sees Steam's virtual gamepad (Valve 0x28de/0x11ff on Linux and
// Windows; on macOS an Xbox 360 pad of version 0, as SDL tells them apart), an Xbox 360 pad Steam
// drives from the physical controller through the player's Steam Input layout. Steam hides that
// physical controller from SDL through the game's environment (SDL_GAMECONTROLLER_IGNORE_DEVICES);
// a controller the list misses is listed too -- the Steam Deck's built-in controls through SDL's
// HIDAPI driver -- and each press then arrives twice, the second time without the layout's
// remapping. Valve's own controllers are always Steam's while Steam runs, so while a virtual
// gamepad is listed they are not read.
bool IsSteamVirtualPad(int nIndex)
{
    const Uint16 nVendor = SDL_JoystickGetDeviceVendor(nIndex);
    const Uint16 nProduct = SDL_JoystickGetDeviceProduct(nIndex);
#ifdef __APPLE__
    return nVendor == 0x045e && nProduct == 0x028e && SDL_JoystickGetDeviceProductVersion(nIndex) == 0;
#else
    return nVendor == 0x28de && nProduct == 0x11ff;
#endif
}

bool SteamOwnsPad(int nIndex)
{
    if (SDL_JoystickGetDeviceVendor(nIndex) != 0x28de || IsSteamVirtualPad(nIndex)) return false;
    for (int i = 0, n = SDL_NumJoysticks(); i < n; ++i)
        if (i != nIndex && IsSteamVirtualPad(i)) return true;
    return false;
}

// Answers the device index of the first game controller SDL knows the type of (Xbox, PS, Switch,
// ...), else of the first game controller at all, or -1. An Android key device with d-pad keys
// (the emulator's "qwerty2") also lists as a game controller, ahead of a real pad.
int FirstPadIndex()
{
    int nFirst = -1;
    for (int i = 0, n = SDL_NumJoysticks(); i < n; ++i) {
        if (!SDL_IsGameController(i) || SteamOwnsPad(i)) continue;
        if (nFirst < 0) nFirst = i;
        if (SDL_GameControllerTypeForIndex(i) != SDL_CONTROLLER_TYPE_UNKNOWN) return i;
    }
    return nFirst;
}

// ---- the merged pad ------------------------------------------------------------------------------
// Every attached game controller is read as one, so a button is down if it is down on any pad and
// an axis is the value pushed furthest from rest. A handheld lists its built-in controls as a pad of
// its own (the AYN Thor's "Xbox Wireless Controller"), ahead of a Bluetooth pad whose type SDL
// cannot name, so reading only the device FirstPadIndex picks leaves the second pad's sticks and
// d-pad dead.
std::recursive_mutex g_mergeLock;
std::vector<SDL_GameController*> g_merged;
int g_nMergedJoysticks = -1;

void RefreshMerged()
{
    std::lock_guard<std::recursive_mutex> g(g_mergeLock);
    bool bStale = SDL_NumJoysticks() != g_nMergedJoysticks;
    for (SDL_GameController* p : g_merged)
        if (!SDL_GameControllerGetAttached(p)) bStale = true;
    if (!bStale) return;
    static bool s_bSaidSteam;
    if (!s_bSaidSteam) {
        s_bSaidSteam = true;
        const char* pAllow = getenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD");
        const char* pIgnore = getenv("SDL_GAMECONTROLLER_IGNORE_DEVICES");
        const char* pExcept = getenv("SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT");
        if (pAllow || pIgnore || pExcept)
            fprintf(stderr, "k2: pad: Steam Input: SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD=%s "
                    "SDL_GAMECONTROLLER_IGNORE_DEVICES=%s SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=%s\n",
                    pAllow ? pAllow : "(unset)", pIgnore ? pIgnore : "(unset)", pExcept ? pExcept : "(unset)");
    }
    for (SDL_GameController* p : g_merged) SDL_GameControllerClose(p);
    g_merged.clear();
    g_nMergedJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < g_nMergedJoysticks; ++i) {
        if (!SDL_IsGameController(i)) {
            char guid[33];
            SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), guid, sizeof guid);
            const char* pName = SDL_JoystickNameForIndex(i);
            fprintf(stderr, "k2: pad: SDL joystick %d (%s, guid %s) has no game controller mapping\n", i,
                    pName ? pName : "?", guid);
            continue;
        }
        if (SteamOwnsPad(i)) {
            const char* pName = SDL_JoystickNameForIndex(i);
            fprintf(stderr, "k2: pad: SDL controller %d (%s) not read: Steam Input reads it and "
                    "presents it as its virtual gamepad\n", i, pName ? pName : "?");
            continue;
        }
        SDL_GameController* p = SDL_GameControllerOpen(i);
        if (!p) continue;
        char* pMapping = SDL_GameControllerMapping(p);
        fprintf(stderr, "k2: pad: merging SDL controller %d (%s, %04x:%04x) mapping=%s\n", i,
                SDL_GameControllerName(p), SDL_JoystickGetDeviceVendor(i), SDL_JoystickGetDeviceProduct(i),
                pMapping ? pMapping : "(none)");
        if (pMapping) SDL_free(pMapping);
        g_merged.push_back(p);
    }
}

bool MergedButton(int nButton)
{
#ifdef K2_QOL_CONTROLLER
    // A <-> B, X <-> Y (SDL_CONTROLLER_BUTTON_A..Y = 0..3), for every reader of the pad.
    if (nButton >= 0 && nButton < 4 && k2qol_swap_face_buttons()) nButton ^= 1;
#endif
    std::lock_guard<std::recursive_mutex> g(g_mergeLock);
    RefreshMerged();
    for (SDL_GameController* p : g_merged)
        if (SDL_GameControllerGetButton(p, static_cast<SDL_GameControllerButton>(nButton))) return true;
    return false;
}

int MergedAxis(int nAxis)
{
    std::lock_guard<std::recursive_mutex> g(g_mergeLock);
    RefreshMerged();
    int nBest = 0;
    for (SDL_GameController* p : g_merged) {
        const int v = SDL_GameControllerGetAxis(p, static_cast<SDL_GameControllerAxis>(nAxis));
        if ((v < 0 ? -v : v) > (nBest < 0 ? -nBest : nBest)) nBest = v;
    }
    return nBest;
}

class Joystick : public IDirectInputDeviceA {
public:
    SDL_JoystickID m_id;
    SDL_GameController* m_pController;
    bool m_acquired = false;

    Joystick(SDL_JoystickID id, SDL_GameController* p) : m_id(id), m_pController(p) {}

    int64_t QueryInterface(const void*, void** ppv) override
    {
        if (!ppv) return DIERR_INVALIDPARAM;
        *ppv = this;                     // the one interface it has (IID_IDirectInputDevice8A)
        return DI_OK;
    }
    uint64_t AddRef() override { return 1; }
    uint64_t Release() noexcept override { return 0; }   // cached per SDL instance, see g_pads
    int64_t Acquire() override { m_acquired = true; return DI_OK; }
    int64_t Unacquire() noexcept override { m_acquired = false; return DI_OK; }
    int64_t SetDataFormat(const void*) override { return DI_OK; }
    int64_t SetCooperativeLevel(void*, unsigned int) override { return DI_OK; }
    int64_t SetProperty(unsigned int, const void*) override { return DI_OK; }
    int64_t Poll() override { return DI_OK; }

    bool Attached() const { return m_pController && SDL_GameControllerGetAttached(m_pController); }

    int64_t GetCapabilities(void* pCaps) override
    {
        if (!pCaps) return DIERR_INVALIDPARAM;
        static_cast<uint32_t*>(pCaps)[1] = Attached() ? 1 : 0;      // DIDEVCAPS::dwFlags, DIDC_ATTACHED
        return DI_OK;
    }

    int64_t GetDeviceInfo(void* pInfo) override
    {
        if (!pInfo) return DIERR_INVALIDPARAM;
        const char* pName = m_pController ? SDL_GameControllerName(m_pController) : 0;
        char* pProduct = static_cast<char*>(pInfo) + 0x140;           // tszProductName, 0x104 ANSI chars
        memset(pProduct, 0, 0x104);
        if (pName) strncpy(pProduct, pName, 0x103);
        return DI_OK;
    }

    int64_t GetDeviceState(unsigned int nSize, void* pState) override
    {
        if (!m_acquired) return DIERR_NOTACQUIRED;
        if (!pState || (nSize != 0x50 && nSize != 0x110)) return DIERR_INVALIDPARAM;
        if (!Attached()) return DIERR_INPUTLOST;         // the hot-plug latch re-enumerates
        memset(pState, 0, nSize);
        int32_t* pAxes = static_cast<int32_t*>(pState);
        pAxes[0] = MergedAxis(SDL_CONTROLLER_AXIS_LEFTX);
        pAxes[1] = MergedAxis(SDL_CONTROLLER_AXIS_LEFTY);
        pAxes[2] = MergedAxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        pAxes[3] = MergedAxis(SDL_CONTROLLER_AXIS_RIGHTX);
        pAxes[4] = MergedAxis(SDL_CONTROLLER_AXIS_RIGHTY);
        pAxes[5] = MergedAxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        // The POV words stay 0, since the d-pad is buttons 12..15, and GetJoystickBuffer only
        // decodes a hat whose value changed, which 0 -> 0 never does.
        static const unsigned char kSdlToDi[15] = { 0, 1, 2, 3, 6, 10, 7, 8, 9, 4, 5, 12, 14, 15, 13 };
        unsigned char* pButtons = static_cast<unsigned char*>(pState) + 0x30;
        unsigned int nDown = 0;
        for (int i = 0; i < 15; ++i) {
            if (!MergedButton(i)) continue;
            pButtons[kSdlToDi[i]] = 0x80;
            nDown |= 1u << kSdlToDi[i];
        }
        // A trigger is down for the engine once `value - 300 >= 1` (GetJoystickBuffer's latch).
        if (pAxes[2] >= 301) nDown |= 1u << 16;
        if (pAxes[5] >= 301) nDown |= 1u << 17;
        // K2_PAD_TRACE=1: one line per change of what is down, and what the console driver claims.
        static const bool s_bTrace = getenv("K2_PAD_TRACE") != 0;
        static unsigned int s_nLastDown = ~0u;
        if (s_bTrace && nDown != s_nLastDown) {
            s_nLastDown = nDown;
            fprintf(stderr, "k2: pad: DI down=0x%05x claimed=0x%05x\n", nDown, k2console_dishim_mask(nDown));
        }
        if (const unsigned int nSilenced = k2console_dishim_mask(nDown)) {
            for (int di = 0; di < 16; ++di)
                if (nSilenced & (1u << di)) pButtons[di] = 0;
            if (nSilenced & (1u << 16)) pAxes[2] = 0;
            if (nSilenced & (1u << 17)) pAxes[5] = 0;
        }
        return DI_OK;
    }
};

// One device per SDL joystick instance, kept for the process, since ReinitJoysticks drops its array
// without releasing what it held, and a re-plugged pad gets a new instance id anyway.
std::mutex g_padLock;
std::deque<Joystick*> g_pads;

Joystick* PadForInstance(SDL_JoystickID id)
{
    std::lock_guard<std::mutex> g(g_padLock);
    for (Joystick* p : g_pads)
        if (p->m_id == id) return p;
    for (int i = 0, n = SDL_NumJoysticks(); i < n; ++i) {
        if (SDL_JoystickGetDeviceInstanceID(i) != id || !SDL_IsGameController(i)) continue;
        SDL_GameController* pController = SDL_GameControllerOpen(i);
        if (!pController) return 0;
        fprintf(stderr, "k2: pad: DirectInput joystick on SDL controller %d (%s)\n", i,
                SDL_GameControllerName(pController));
        Joystick* p = new Joystick(id, pController);
        g_pads.push_back(p);
        return p;
    }
    return 0;
}

Keyboard* g_keyboard;
Mouse* g_mouse;

class DirectInput : public IDirectInputA {
public:
    uint64_t AddRef() override { return 1; }
    uint64_t Release() noexcept override { return 0; }
    int64_t CreateDevice(const void* rguid, IDirectInputDeviceA** ppDevice, void*) override
    {
        if (!rguid || !ppDevice) return DIERR_INVALIDPARAM;
        if (!memcmp(rguid, &GUID_SysMouse, sizeof(GUID))) {
            if (!g_mouse) g_mouse = new Mouse;
            *ppDevice = g_mouse;
            return DI_OK;
        }
        if (!memcmp(rguid, &GUID_SysKeyboard, sizeof(GUID))) {
            if (!g_keyboard) g_keyboard = new Keyboard;
            *ppDevice = g_keyboard;
            return DI_OK;
        }
        const Guid* pGuid = static_cast<const Guid*>(rguid);
        if (pGuid->a == kGuidPadTag) {
            Joystick* p = PadForInstance(static_cast<SDL_JoystickID>(pGuid->b) | static_cast<SDL_JoystickID>(pGuid->c) << 16);
            if (!p) return DIERR_DEVICENOTREG;
            *ppDevice = p;
            return DI_OK;
        }
        return DIERR_DEVICENOTREG;
    }
    // nDevType 4 (DI8DEVCLASS_GAMECTRL) is the only class the engine enumerates. The callback is the
    // engine's own `int (*)(const DIDEVICEINSTANCEA*, void*)` (CountJoysticksExt / InitJoysticksExt),
    // and InitJoysticks reads the instance GUID at +8.
    int64_t EnumDevices(unsigned int nDevType, void* pCallback, void* pRef, unsigned int) override
    {
        if (nDevType != 4 || !pCallback || !PadEnabled()) return DI_OK;
        const int nIndex = FirstPadIndex();
        if (nIndex < 0) return DI_OK;
        const SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(nIndex);
        unsigned char aInstance[0x268] = {};
        Guid guid = { kGuidPadTag, static_cast<uint16_t>(id & 0xffff), static_cast<uint16_t>(static_cast<uint32_t>(id) >> 16), {} };
        memcpy(aInstance + 8, &guid, sizeof guid);
        (reinterpret_cast<int (*)(const void*, void*)>(pCallback))(aInstance, pRef);
        return DI_OK;
    }
    int64_t GetDeviceStatus(const void*) override { return DI_OK; }
    int64_t Initialize(void*, unsigned int) override { return DI_OK; }
};

} // namespace

// Writes SDL's name for the key at DIK code nDik in the current layout, for KOTOR 1's key names
// (CExoRawInputInternal::GetDisplayCharacterDirectInputValue, which has no DirectInput keyboard to
// ask).
extern "C" int k2host_key_name(unsigned int nDik, char* pszOut, int nOut)
{
    for (int s = 0; nDik && nOut > 0 && s < SDL_NUM_SCANCODES; ++s) {
        if (DikFromScancode(static_cast<SDL_Scancode>(s)) != nDik) continue;
        const char* pszName = SDL_GetKeyName(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(s)));
        if (!*pszName) pszName = SDL_GetScancodeName(static_cast<SDL_Scancode>(s));
        if (!*pszName) return 0;
        SDL_strlcpy(pszOut, pszName, static_cast<size_t>(nOut));
        return 1;
    }
    return 0;
}

// Answers whether a movement key (WASD, the arrows) is down now, the trigger for the engine's
// "why the player will not move" log (clientinput.cpp, K2_HOST_BUILD).
extern "C" int k2host_move_keys_held(void)
{
    static const SDL_Scancode s_aKeys[] = { SDL_SCANCODE_W, SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D,
                                            SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT };
    const Uint8* pState = SDL_GetKeyboardState(0);
    for (SDL_Scancode s : s_aKeys)
        if (pState && pState[s]) return 1;
    return 0;
}

// A game controller the pad device would report is attached (shim_asl.cpp's
// ASLPlat_IsControllerPlugged).
extern "C" int k2shim_pad_attached(void)
{
    return PadEnabled() && FirstPadIndex() >= 0;
}

// The device index the pad uses, for console/k2console.cpp (which cannot include SDL.h).
extern "C" int k2shim_pad_index(void)
{
    return FirstPadIndex();
}

// The physical controller behind Steam's virtual gamepad, which is not read (SteamOwnsPad), for
// k1/k1sdl.cpp.
extern "C" int k2shim_pad_steam_owned(int nIndex)
{
    return SteamOwnsPad(nIndex) ? 1 : 0;
}

// The merged pad's SDL_GameControllerButton / SDL_GameControllerAxis, for console/k2console.cpp.
extern "C" int k2shim_pad_button(int nButton)
{
    return MergedButton(nButton) ? 1 : 0;
}

extern "C" int k2shim_pad_axis(int nAxis)
{
    return MergedAxis(nAxis);
}

// the DirectInput constants the engine passes by address (exorawinputinternal.h).
// The DIDATAFORMATs are only handed back to SetDataFormat, which ignores them; the objects just have
// to exist.
const GUID IID_IDirectInput8A       = { 0xBF798030, 0x483A, 0x4DA2, { 0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00 } };
const GUID IID_IDirectInputDevice8A = { 0x54D41080, 0xDC15, 0x4833, { 0xA4, 0x1B, 0x74, 0x8F, 0x73, 0xA3, 0x81, 0x79 } };
const GUID GUID_SysKeyboard         = { 0x6F1D2B61, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };
const GUID GUID_SysMouse            = { 0x6F1D2B60, 0xD5A0, 0x11CF, { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };
DIDATAFORMAT c_dfDIKeyboard  = { 0x20, 0x18, 0, 0, 0, nullptr };
DIDATAFORMAT c_dfDIMouse     = { 0x20, 0x18, 0, 0, 0, nullptr };
DIDATAFORMAT c_dfDIJoystick2 = { 0x20, 0x18, 0, 0, 0, nullptr };

// ---- the COM base classes: every slot needs a body for the vtables to exist ---------------------
int64_t IDirectInputA::QueryInterface(const void*, void**) { return E_NOTIMPL_; }
uint64_t IDirectInputA::AddRef() { return 1; }
uint64_t IDirectInputA::Release() noexcept { return 0; }
int64_t IDirectInputA::CreateDevice(const void*, IDirectInputDeviceA**, void*) { return E_NOTIMPL_; }
int64_t IDirectInputA::EnumDevices(unsigned int, void*, void*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputA::GetDeviceStatus(const void*) { return E_NOTIMPL_; }
int64_t IDirectInputA::RunControlPanel(void*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputA::Initialize(void*, unsigned int) { return E_NOTIMPL_; }
IDirectInputA::~IDirectInputA() {}

int64_t IDirectInputDeviceA::QueryInterface(const void*, void**) { return E_NOTIMPL_; }
uint64_t IDirectInputDeviceA::AddRef() { return 1; }
uint64_t IDirectInputDeviceA::Release() noexcept { return 0; }
int64_t IDirectInputDeviceA::GetCapabilities(void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::EnumObjects(void*, void*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::GetProperty(unsigned int, void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::SetProperty(unsigned int, const void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::Acquire() { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::Unacquire() noexcept { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::GetDeviceState(unsigned int, void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::GetDeviceData(unsigned int, void*, unsigned int*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::SetDataFormat(const void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::SetEventNotification(void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::SetCooperativeLevel(void*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::GetObjectInfo(void*, unsigned int, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::GetDeviceInfo(void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::RunControlPanel(void*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::Initialize(void*, unsigned int, const void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::CreateEffect(const void*, const void*, void**, void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::EnumEffects(void*, void*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::GetEffectInfo(void*, const void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::GetForceFeedbackState(unsigned int*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::SendForceFeedbackCommand(unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::EnumCreatedEffectObjects(void*, void*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::Escape(void*) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::Poll() { return DI_OK; }
int64_t IDirectInputDeviceA::SendDeviceData(unsigned int, const void*, unsigned int*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::EnumEffectsInFile(const char*, void*, void*, unsigned int) { return E_NOTIMPL_; }
int64_t IDirectInputDeviceA::WriteEffectToFile(const char*, unsigned int, void*, unsigned int) { return E_NOTIMPL_; }
IDirectInputDeviceA::~IDirectInputDeviceA() {}

extern "C" int64_t DirectInput8Create(HINSTANCE__*, unsigned int, const void* riid, void** ppvOut, void*)
{
    if (!ppvOut) return DIERR_INVALIDPARAM;
    static DirectInput s_di;
    *ppvOut = &s_di;
    (void)riid;
    return DI_OK;
}

// The scancode table for KOTOR 1's pc UI keyboard (k1/k1sdl.cpp), which has no DirectInput device.
extern "C" unsigned char k2shim_dik_from_scancode(int nScancode)
{
    return DikFromScancode(static_cast<SDL_Scancode>(nScancode));
}

// Called by the window shim for every SDL event it pumps.
void k2shim_dinput_event(const SDL_Event* e)
{
    std::lock_guard<std::mutex> g(g_lock);
    switch (e->type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        if (unsigned char dik = DikFromScancode(e->key.keysym.scancode))
            g_keys[dik] = e->type == SDL_KEYDOWN ? 0x80 : 0;
        break;
    case SDL_MOUSEMOTION:
        g_mouseDx += e->motion.xrel;
        g_mouseDy += e->motion.yrel;
        if (e->motion.xrel) QueueMouse(0, static_cast<uint32_t>(e->motion.xrel));
        if (e->motion.yrel) QueueMouse(4, static_cast<uint32_t>(e->motion.yrel));
        break;
    case SDL_MOUSEWHEEL:
        g_mouseDz += e->wheel.y * 120;
        QueueMouse(8, static_cast<uint32_t>(e->wheel.y * 120));
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        // DirectInput order: left, right, middle, then the extras
        static const int kMap[] = { -1, 0, 2, 1, 3, 4 };
        int b = e->button.button < 6 ? kMap[e->button.button] : -1;
        if (b < 0) break;
        unsigned char v = e->type == SDL_MOUSEBUTTONDOWN ? 0x80 : 0;
        g_mouseButtons[b] = v;
        QueueMouse(12 + static_cast<uint32_t>(b), v);
        break;
    }
    case SDL_JOYDEVICEADDED: {
        const char* pName = SDL_JoystickNameForIndex(e->jdevice.which);
        fprintf(stderr, "k2: pad: SDL joystick %d attached (%s)%s%s\n", e->jdevice.which, pName ? pName : "?",
                SDL_IsGameController(e->jdevice.which) ? "" : ", no game controller mapping",
                PadEnabled() ? "" : ", hidden from the engine (K2_PAD=0)");
        break;
    }
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
        // What MacMain.cpp's PreProcessEvent does with these (the window shim does not route pad
        // hot-plug through it): the next GetEvents re-runs ReinitJoysticks, and pauses the game
        // when the pad count dropped.
        if (PadEnabled()) gResetController_Mac = 1;
        break;
    case SDL_WINDOWEVENT:
        if (e->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {   // no stuck keys across alt-tab
            memset(g_keys, 0, sizeof g_keys);
            memset(g_mouseButtons, 0, sizeof g_mouseButtons);
        }
        break;
    default:
        break;
    }
}
