# PATCH_COMMAND for SDL2 on iOS (ios/deps.cmake), run in its source dir. Idempotent.
#
# The UIScene lifecycle. An app built with the iOS 27 SDK that has no scene manifest is refused at
# launch (UIKit's _UIApplicationEvaluateRuntimeIssueForNoSceneLifecycleAdoption traps); SDL2 only knows
# the old app-delegate lifecycle (SDL3 adopted scenes). As SDL3 does it:
#   * SDLUIKitSceneDelegate (named in ios/Info.plist.in's UIApplicationSceneManifest) keeps the window
#     scene, and starts SDL_main once it has connected -- not from didFinishLaunching, when a scene-based
#     app has no scene yet;
#   * SDL's windows are made on that scene (initWithWindowScene:);
#   * no programmatic launch window in scene mode (it would have no scene to show on).
# The app-level lifecycle notifications SDL observes (UIApplicationWillResignActive... ) are still
# posted for a scene-based app.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/video/uikit/SDL_uikitappdelegate.h")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: scenes")
  set(_a "@interface SDLUIKitDelegate")
  string(FIND "${_s}" "${_a}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "sdl2_patch.cmake: SDL_uikitappdelegate.h anchor not found")
  endif()
  string(REPLACE "${_a}" "/* k2: scenes -- the connected window scene (nil before it connects, or without a scene manifest) */
UIWindowScene *UIKit_K2GetWindowScene(void) API_AVAILABLE(ios(13.0), tvos(13.0));

${_a}" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/video/uikit/SDL_uikitappdelegate.m")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: scenes")
  set(_a "@implementation SDLUIKitDelegate {")
  set(_b "    SDL_SetMainReady();\n    [self performSelector:@selector(postFinishLaunch) withObject:nil afterDelay:0.0];")
  set(_c "    if (vc.view) {\n        launchWindow = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];")
  foreach(_x _a _b _c)
    string(FIND "${_s}" "${${_x}}" _at)
    if(_at EQUAL -1)
      message(FATAL_ERROR "sdl2_patch.cmake: SDL_uikitappdelegate.m anchor ${_x} not found")
    endif()
  endforeach()
  string(REPLACE "${_a}" "/* k2: scenes (see platforms/ios/sdl2_patch.cmake) */
static id s_k2WindowScene;
static BOOL s_k2MainStarted;

static BOOL K2UsesScenes(void)
{
    if (@available(iOS 13.0, tvOS 13.0, *)) {
        return [[NSBundle mainBundle] objectForInfoDictionaryKey:@\"UIApplicationSceneManifest\"] != nil;
    }
    return NO;
}

UIWindowScene *UIKit_K2GetWindowScene(void)
{
    return s_k2WindowScene;
}

API_AVAILABLE(ios(13.0), tvos(13.0))
@interface SDLUIKitSceneDelegate : UIResponder <UIWindowSceneDelegate>
@end

@implementation SDLUIKitSceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions
{
    if ([scene isKindOfClass:[UIWindowScene class]]) {
        s_k2WindowScene = scene;
    }
    if (!s_k2MainStarted) {
        s_k2MainStarted = YES;
        [[SDLUIKitDelegate sharedAppDelegate] performSelector:@selector(postFinishLaunch) withObject:nil afterDelay:0.0];
    }
}

- (void)sceneDidDisconnect:(UIScene *)scene
{
    if (s_k2WindowScene == scene) {
        s_k2WindowScene = nil;
    }
}

@end

${_a}" _s "${_s}")
  string(REPLACE "${_b}" "    SDL_SetMainReady();
    if (K2UsesScenes()) {
        return YES;   /* k2: scenes -- SDLUIKitSceneDelegate starts SDL_main once the scene connects */
    }
    s_k2MainStarted = YES;
    [self performSelector:@selector(postFinishLaunch) withObject:nil afterDelay:0.0];" _s "${_s}")
  string(REPLACE "${_c}" "    if (vc.view && !K2UsesScenes()) {   /* k2: scenes -- no window outside a scene */
        launchWindow = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

# The audio session outlives SDL's own audio devices. The game's sound is miniaudio, on the same
# AVAudioSession; SDL opens a device only for a movie's sound, and closing it (the last SDL device)
# deactivated the session, which silenced the game until the app next came back from the background.
set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/coreaudio/SDL_coreaudio.m")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: session")
  set(_a "        } else if (!open_playback_devices && !open_capture_devices && session_active) {")
  string(FIND "${_s}" "${_a}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "sdl2_patch.cmake: SDL_coreaudio.m anchor not found")
  endif()
  string(REPLACE "${_a}" "        } else if (0 && !open_playback_devices && !open_capture_devices && session_active) {   /* k2: session -- miniaudio shares it */" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()

set(_f "${CMAKE_CURRENT_SOURCE_DIR}/src/video/uikit/SDL_uikitwindow.m")
file(READ "${_f}" _s)
if(NOT _s MATCHES "k2: scenes")
  set(_a "        uiwindow = [[SDL_uikitwindow alloc] initWithFrame:data.uiscreen.bounds];")
  string(FIND "${_s}" "${_a}" _at)
  if(_at EQUAL -1)
    message(FATAL_ERROR "sdl2_patch.cmake: SDL_uikitwindow.m anchor not found")
  endif()
  string(REPLACE "${_a}" "        uiwindow = nil;
        if (@available(iOS 13.0, tvOS 13.0, *)) {   /* k2: scenes -- the window lives on the scene */
            UIWindowScene *scene = UIKit_K2GetWindowScene();
            if (scene && data.uiscreen == [UIScreen mainScreen]) {
                uiwindow = [[SDL_uikitwindow alloc] initWithWindowScene:scene];
                uiwindow.frame = data.uiscreen.bounds;
            }
        }
        if (!uiwindow) {
            uiwindow = [[SDL_uikitwindow alloc] initWithFrame:data.uiscreen.bounds];
        }" _s "${_s}")
  file(WRITE "${_f}" "${_s}")
endif()
