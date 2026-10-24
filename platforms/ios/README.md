# ios/ — the iOS app (iPhone and iPad)

KOTOR II and KOTOR 1 from the transcribed engine, as an iOS app: the macOS build's engine, shim and
backends, with Android's GL (gl4es over GLES 2) and SDL2 built from source.

```sh
./build_run_ios.sh [--k1] [--touch|--xbox] [--sim]        # build, install, run (repo root)
cmake --preset k2-cross-ios-sim-pc && cmake --build --preset k2-cross-ios-sim-pc   # by hand
cmake --build --preset k2-cross-ios-sim-pc --target run-sim            # Simulator: install, OBBs, launch
cmake --build --preset k2-cross-ios-pc --target ipa                    # platforms/out/k2-cross-ios-pc/k2port-pc.ipa (device builds)
```

Presets: `k2-cross-ios-pc` (pc), `k2-cross-ios-touch`, `k2-cross-ios-xbox`, `k1-cross-ios-pc`, `k1-cross-ios-touch`, `k1-cross-ios-xbox`, and the same
with `-sim` after `k2-cross-ios-pc` for the Simulator (`k2-cross-ios-sim-pc`, `k1-cross-ios-sim-touch`, ...). Bundle ids
`com.k2port.swkotorii.<ui>` / `com.k2port.swkotor.<ui>` (`-DK2I_BUNDLE_ID=`), so the six apps install
side by side.

## Game data

The Android expansion files, unchanged: KOTOR II `main.*.com.aspyr.swkotorii.obb` and
`patch.*.com.aspyr.swkotorii.obb`, KOTOR 1 `main.53.com.aspyr.swkotor.obb` and
`patch.53.com.aspyr.swkotor.obb`. They go in the app's **Documents** folder (`UIFileSharingEnabled`:
Finder's device pane > Files, or the Files app's On My iPhone), which is also the working directory:
saves, `swkotor2.ini` / `swkotor.ini`, `k2.log` (stdout/stderr, the previous run's in `k2.prev.log`)
and `k2env.txt` (one `KEY=VALUE` per line: the `K2_*` switches of `platforms/README.md`) live there.
`build_run_ios.sh` / `run-sim` copy the OBBs from `decomp_bins/` for you. KOTOR 1's dialog font and
GLSL shaders (the original APK's assets) are bundled into the app.

## How it differs from macOS

* **Entry:** `main_ios.cpp` defines `main` and hands it to `SDL_UIKitRunApp` (no SDL2main), so KOTOR
  1's engine-owned `SDL_main` (`src/port/Mac/MacMain.cpp`) still runs as on Android.
* **SDL2 is a dylib** in the app's `Frameworks/`: `k1/k1sdl.cpp` defines `SDL_CreateWindow`,
  `SDL_GL_SetAttribute` ... in front of SDL's and reaches the real ones with `dlsym(RTLD_NEXT)`, which a
  static SDL could not answer.
* **GL:** gl4es (`android/gl4es_glue.cpp`, the Android patches plus one for Darwin: gl4es's public `gl*`
  names are `__attribute__((alias))`, which Mach-O lacks, so on Apple it defined none of them; they are
  assembler `.set` aliases now). Three iOS specifics in the glue: gl4es looks the driver up in
  OpenGLES.framework itself (`SDL_GL_GetProcAddress` is `dlsym(RTLD_DEFAULT)`, which finds our own
  exported `gl*` first); EAGL has no default framebuffer, so the driver's `glBindFramebuffer(0)` binds
  SDL's view framebuffer; and SDL's swap presents whatever renderbuffer is bound, so the view's colour
  renderbuffer is bound around each swap (else the engine's depth buffer is presented: black).
* **The window is the screen:** fullscreen, never hidden, one only (as Android's SDL allows — the
  movies' black "Knights Cinematic Backdrop" window would otherwise cover the game), HiDPI with the
  screen's `nativeScale` (`shim_display.cpp k2shim_pixel_scalef`; the shim's points-to-pixels scale is
  a float for the 2.88x / 2.61x iPhones), and display mode 0 is the screen in pixels.
  `K2_HIDPI=0` renders at points instead (the Simulator's speed).
* **UIScene lifecycle:** iOS 27 will not launch an app built with its SDK that has no scene
  manifest; SDL2 2.32 predates scenes, so `ios/sdl2_patch.cmake` adds SDL3's approach (a scene
  delegate that starts SDL_main once the window scene connects, windows made on that scene).
* **Landscape only:** the display mode is forced landscape (a scene can still report the portrait
  screen at first), and WM_SIZE carries pixels, not SDL's points.
* **Limited NPOT:** Apple's GLES 2 samples a non-power-of-two texture with mipmap filtering as
  black; gl4es runs with `LIBGL_FORCENPOT=1` (android/gl4es_glue.cpp). The Simulator has the same
  limit, so it shows there too.
* **App icon:** resources/icons/<game>_icon_1024.png, compiled into an asset catalog by `actool`
  (`k2ipa.py sign --icon`).
* **No `system()`** on iOS: the Android cloud-save JNI's `rm -rf` gets a stub (`ios_nosystem.h`).
* **miniaudio** is compiled as Objective-C (its iOS backend uses AVAudioSession).
* **Signing** (`tools/k2ipa.py`): the build signs ad hoc (`K2I_CODESIGN_IDENTITY=-`, what the Simulator
  runs); `run-device` re-signs with a development identity and a profile that covers the device (a
  wildcard profile's `application-identifier` gets the bundle id), or `-DK2I_CODESIGN_IDENTITY=...
  -DK2I_PROVISIONING_PROFILE=...` signs at build time.

## The Simulator

Its OpenGL ES is a CPU rasterizer (`GLRendererFloat`): the main menu takes ~2.5 s a frame at 1x and
~10 s at 3x. Use it to check that things start and draw (`K2_HIDPI=0`, `K2_SHOT=<frame>` writes the
back buffer into Documents), not to play.
