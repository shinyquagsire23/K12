// Aspyr's platform layer (ASL / ASLPlat): preferences, language, paths, controller and on-screen
// keyboard queries, alerts. On Android these talk to Java; here they answer as a desktop would.
#include "k2shim.h"

#include <SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

namespace {

// K2_PREF_<key>=0|1 overrides a preference; otherwise the engine's default stands.
bool PrefFromEnv(const char* pszKey, bool bDefault)
{
    std::string var = std::string("K2_PREF_") + pszKey;
    const char* v = getenv(var.c_str());
    if (!v) return bDefault;
    return v[0] == '1' || v[0] == 't' || v[0] == 'T' || v[0] == 'y' || v[0] == 'Y';
}

void Alert(const std::string& title, const std::string& fmt, va_list ap)
{
    char msg[2048];
    vsnprintf(msg, sizeof msg, fmt.c_str(), ap);
    fprintf(stderr, "k2: ALERT [%s] %s\n", title.c_str(), msg);
}

} // namespace

// The host entry has already chdir'd to the data directory; Android resolves the OBB mount here.
void ASLPlat_SetWorkingDirectoryToResources() {}

// Translates a DOS path to a host path. Callers hand a 1024-byte destination, which is the size of
// CExoStreamingSoundSourceInternal's frame.
void DOS2MacPath(const char* sDosPath, char* sMacPath)
{
    k2shim_host_path(sDosPath, sMacPath, 0x400);
}

extern "C" int ASL_feof(FILE* f) { return feof(f); }
extern "C" int ASL_ferror(FILE* f) { return ferror(f); }

int ASLGetIPadVersion() { return 0; }
int ASLGetIPhoneVersion() { return 0; }

// Whether a game controller is attached; the class selection screen, the dialog replies and the pad
// glyphs read it. GetControllerPlugged's 2 ("game controller") is what puts the left stick's forward
// axis into the touch UI's walking, so the touch build answers 2 with a pad attached; the camera then
// reads ios_controller_left_right, which nothing here feeds, and falls back to the engine's
// right-stick binding. The pc and xbox builds keep 0, which is what they were built and tested
// around. The KOTOR 1 build has its own pair, read off gamepadConnected.
#ifndef K2_QOL_K1
extern "C" int k2shim_pad_attached(void);
bool ASLPlat_IsControllerPlugged() { return k2shim_pad_attached() != 0; }
#ifdef K2_UI_TOUCH
int ASLPlat_GetControllerPlugged() { return k2shim_pad_attached() ? 2 : 0; }
#else
int ASLPlat_GetControllerPlugged() { return 0; }
#endif
#endif
// The engine's Show starts SDL text input and sets a flag nothing ever clears. The editboxes call it
// on click/focus and SDL_StopTextInput on focus loss, so without the StartTextInput no SDL_TEXTINPUT
// (hence no WM_CHAR) reaches a save name after the first box.
static bool s_bVirtualKeyboardVisible;
void ASLPlat_ShowVirtualKeyboard(const char*, unsigned int)
{
#if !defined(K2_UI_XBOX) && !defined(K1_UI_XBOX)
    // The console UIs type through their own keyboard; on Android this call raises the system one.
    SDL_StartTextInput();
#endif
    s_bVirtualKeyboardVisible = true;
}
bool ASLPlat_IsVirtualKeyboardVisible() { return s_bVirtualKeyboardVisible; }

namespace ASL {

bool GetPrefAsBool(const char* pszKey, bool bDefault) { return PrefFromEnv(pszKey, bDefault); }

// Reads a numeric preference from K2_PREF_<key>, falling back to the default.
unsigned int GetPrefAsUInt32(const char* pszKey, unsigned int nDefault)
{
    std::string var = std::string("K2_PREF_") + pszKey;
    const char* v = getenv(var.c_str());
    return v ? static_cast<unsigned int>(strtoul(v, 0, 0)) : nDefault;
}

// Returns the device language, English (0) unless K2_LANGUAGE says otherwise. The engine's folder
// table counts up from French: 3, German 4, Italian 5, Spanish 9.
unsigned int GetLanguage()
{
    const char* v = getenv("K2_LANGUAGE");
    return v ? static_cast<unsigned int>(atoi(v)) : 0u;
}

void ErrorAlertStr(const std::string& strTitle, std::string strMsg, ...)
{
    va_list ap;
    va_start(ap, strMsg);
    Alert(strTitle, strMsg, ap);
    va_end(ap);
}

bool ErrorAlertYesNoStr(const std::string& strTitle, std::string strMsg, bool bDefaultYes, ...)
{
    // va_start after a `bool` is undefined (default promotion), so the format is shown unexpanded.
    fprintf(stderr, "k2: ALERT [%s] %s (answering %s)\n", strTitle.c_str(), strMsg.c_str(),
            bDefaultYes ? "yes" : "no");
    return bDefaultYes;
}

// The rest of the Android process entry (SDL_main). Each host has its own entry that does this
// bring-up and then calls WinMain, so nothing reaches these there.
class AppHooks;

// ASL's own string table; the engine's text comes from dialog.tlk and AspyrLocalizedStrings, which
// open their files themselves.
void RegisterStringFile(const std::string&) {}

// The host entry picks the GL context (a 2.1 compatibility one, or GLES 2 under gl4es).
namespace SDL { bool SetUseCoreProfile(bool) { return true; } }

// The host entry mounts the expansion files (k2shim_obb_mount) from where its platform keeps them.
namespace FsApi { namespace Obb { void initVfs() {} } }

// Runs WinMain with argv as its command line; the window is made when WinMain asks for one
// (CreateWindowExA).
int Main(int argc, char** argv, AppHooks&,
         int (*pfnWinMain)(HINSTANCE__*, HINSTANCE__*, char*, int))
{
    std::string cmdline;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) cmdline += ' ';
        cmdline += argv[i];
    }
    return pfnWinMain(0, 0, &cmdline[0], 1);
}

} // namespace ASL

namespace MSG_Mac {
// Captures the mouse for a mouse-look (the right button, or the Mouse Look option) and releases it
// after. On pc and xbox a captured mouse is SDL relative mode: the pointer stays put and hidden,
// and its motion reaches GetMouseDelta through the DirectInput mouse.
void CaptureMouse(bool bCapture)
{
#ifndef K2_UI_TOUCH
    SDL_SetRelativeMouseMode(bCapture ? SDL_TRUE : SDL_FALSE);
#else
    SDL_CaptureMouse(bCapture ? SDL_TRUE : SDL_FALSE);
#endif
}
}
