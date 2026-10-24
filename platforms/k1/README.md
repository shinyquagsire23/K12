# platforms/k1 — KOTOR 1 smoke builds (`K2P_GAME=k1`)

The same `src/` tree compiled with `KOTOR1` (docs/K1_PORT.md) as a playable host / Android build.

```sh
./build_run_macos.sh --k1 [--window 1920x1080] [--chromebook]   # preset k1-host-touch
./build_run_macos.sh --k1 --ui pc                               # preset k1-host-pc   (first pass)
./build_run_macos.sh --k1 --xbox                                # preset k1-host-xbox (first pass)
./build_run_android.sh --k1 [--xbox]                            # preset k1-cross-android-touch / k1-cross-android-xbox
```

Presets are named `<k1|k2>-host-<ui>` (this computer) and `<k1|k2>-cross-<target>-<ui>`
(`CMakePresets.json`); each has its own build dir, `platforms/out/<preset>/`.

* **Android package:** `com.aspyr.swkotor.hybrid` (versionCode 93), the same `<original>.hybrid` scheme as
  KOTOR II's port, so it installs beside Aspyr's `com.aspyr.swkotor` and the KOTOR II port and replaces
  neither. `device.py install --orig-package com.aspyr.swkotor` copies the OBBs from the installed
  original (or pushes `decomp_bins/swkotor_obb/`).
* **Data:** K1's two OBBs are stored-only zips; the engine reads them in place through `ObbFile`
  (`k1port.cpp` + `k1zip.cpp`, miniz). The `.bzf` BIFs are LZMA (`k1lzma.c`, `MacDecompress`). The
  desktops: `K2_OBB_DIR=<folder>`, as for KOTOR II (the older `K1_OBB_DIR` still works); the
  build_run scripts' working dir is `platforms/out/gamedir-k1/` (swkotor.ini, saves; the touch UI also
  `override/iosdialog.otf` from the original APK's assets, which the OBBs lack). Android: the app's
  obb dir and external files dir; `k1_main` installs the font from the APK's assets (iOS: from the
  app bundle). The GLSL shaders are compiled in everywhere (`resources/shaders/`).
* **No OBBs** (the desktop `main`): with no `K2_OBB_DIR` and no `main.53` / `patch.53` in the
  game's folder, but a `chitin.key` there -- an installed copy `macos/gamedirs.cpp` found (the Steam
  Mac bundle's `Contents/Assets`, the Windows build under Proton ...), or symlinks to one -- that folder,
  the working directory, stands in for both archives (`k1port.cpp`: an `ObbFile`
  on a directory has no zip entries and opens loose files, case-blind), so `SDL_main`'s wait for both
  mounts ends and the streamed sounds, which read only through the main OBB, play. The plain `.bif`s
  load where the device has `.bzf`s (the engine tries the compressed twin first).
* **Entry:** K1's engine owns `SDL_main` (src/port/Mac/MacMain.cpp). macOS `main` and Android `k1_main`
  (K1Activity) mount the OBBs, answer SDL_main's Java queries through the stub JNIEnv
  (`K2_JNI_<method>` / `K2_JNI_<field>`), then call it.

## UI builds (`K2P_UI`): touch, pc, xbox

`touch` is the shipped Android UI. `pc` and `xbox` are a **first pass** (2026-10-14): K1's data has no `_p` /
`_x` layouts — `data/gui.bzf` (+ `patch.erf`) holds the original PC/console layouts under their bare names, and
the port's `override/` shadows 61 of them with touch re-layouts. Those re-layouts are the PC layouts scaled x1.6
(640x480 -> 1024x768: the EXTENTs are the difference), and every K1 body positions in that 1024x768 space.

| | touch | pc (`K2_UI_PC`) | xbox (`K1_UI_XBOX`) |
|---|---|---|---|
| layouts | override/ first | the archives first (`CExoResMan::GetKeyEntry`), no phone `x%s` variants | same |
| GUI art | override/ first | override/'s touch re-draws of PC textures lose to `swpc_tex_gui.erf` (fonts excepted) | same |
| scaling | — | an archived layout's EXTENTs are scaled into 1024x768 as they load (`K1ScaleLoadedExtent`, swgui.cpp: 1024 / the design width its name ends in, 640 for a bare name); list PROTOITEMs are not | same |
| main menu | the port's code-built menu | `mainmenu8x6` (Malak, Quit = `QueryExitProgram`) | `mainmenu` (640x480 console menu) |
| HUD | `ymipc210x7` + iOS widgets | `mipc210x7`: menu bar, four action columns with arrows (cycle in place), toggles, minimap; `AndroidExtentShift` pins the PC pieces to their edge; no corner plate / help / quicksave buttons | `maininterface`: the console bar (six boxes, the selected one large; box contents and pad as the Switch's HUD), portraits stacked bottom right, minimap top right, a fixed target banner; one scale, each rectangle pinned to its screen half |
| conversation | `ydialog` (phone `xydialog`) | `dialog`, the band kept on screen | `dialog` |
| lists | phone padding / captions | the layout's padding and the default caption size | same |
| input | touch (mouse = finger) | `SDL_IsChromebook` on; the keyboard reaches the key map as DirectInput codes (`k1sdl.cpp` key watch -> `GetKeyboardBuffer`); a PC mouse (MacMain.cpp: left/right buttons as their own messages, the pointer always reported, right-drag looks in SDL relative mode, left+right walks; the PC's hardware cursors from the Steam `.cur` set, `shim/shim_cursor.cpp`, else the software cursor `gui_mp_*` drawn by `CSWGuiManager::Draw`) | the Switch's pad table (`CExoInputInternal`, the K1 pad block): A act, X queue, Y clear, B disengage, D-pad boxes/entries, LB pause, RB character, LT/RT target, Start menu, Back solo, RS free look; the mouse as in pc (MacMain.cpp: hover, both buttons, the wheel, right-drag looks, left+right walks; the PC cursors), with no screen-edge turn; the bar's boxes select on hover, run on a click and step on the wheel; a pad press hides the pointer and parks it off screen until it moves; a press whose release the engine dropped is released after half a second, as its latch would otherwise keep the look or the walk on or the right stick from turning |

`K1_UI_XBOX` is NOT `K2_UI_XBOX`: KOTOR II's console code (the seven-box `maininterface_x` bar and its driver
`console/k2console.cpp`, the `_x` Pazaak) is written against layouts and classes K1 does not have, so a K1 xbox
build compiles none of it. The one exception is the console name keyboard (`console/k2namekeyboard.cpp`): its
K1 branch builds the Switch's 13x5 key grid in code from K1's PC `name` layout and APK art, and it is pushed by
the chargen Name step and the save-name dialog. What is still open is in `platforms/BUGS.md`
("K1 pc / xbox UI"). A host pad is read through SDL's GameController API so its numbering is Android's
(`k1sdl.cpp`). Scripted input for these builds: `click:X,Y`, `move:X,Y`, `ldown:`/`lup:`/`rdown:`/`rup:X,Y`,
`rel:DX,DY`, `keydown:` / `keyup:SCANCODE`, `joy:N`, `jaxis:AXIS,VALUE` (below).

## What is here — the port of libandroid_port.so

libKOTOR.so imports ~270 symbols from Aspyr's port library. Stubs return what the shipped stub
returns (read off `decomp_bins/swkotor_extracted/lib/arm64-v8a/libandroid_port.so` with llvm-objdump).

| file | what |
|---|---|
| `k1port.cpp` | engine flags + KOTOR1: Win32 shims (C++ linkage), port globals, `ObbFile`, `MacDecompress`, the viewport record, Ios* glue |
| `k1gl.gen.cpp` | `android_port_glX` forwarders, generated from `src/port/k1gl.h` by `tools/gen_k1gl.py` (4-byte `ret` imports stay no-ops) |
| `k1kotorgl.cpp` | the port's GLSL renderer (libandroid_port.so 0x9b830..0x9d0b8): fragment-program ids -> `KotorProgram` (APK assets `kotor.vert`/`kotor.frag`), env params -> uniforms, fog / alpha as shader data -- KOTOR II's `shim_kotorgl.cpp` at an earlier revision |
| `k1sdl.cpp` | in front of SDL: desktop GL 2.1 + window on macOS, gl4es hooks + cwd-first `SDL_RWFromFile` on Android, `Android_Window`, `K2_SHOT`, scripted input (`K2_INPUT` / `K2_CTL`: `tap:X,Y`, `click:X,Y` / `move:X,Y` (the mouse), `ldown:`/`lup:`/`rdown:`/`rup:X,Y`, `rel:DX,DY` (relative motion), `key:SCANCODE`, `keydown:` / `keyup:SCANCODE`, `text:STR`, `joy:N` (raw joystick button), `jaxis:AXIS,VALUE`, `rstick:X` (right stick x, -1..1; 0 releases), `shot:NAME`, `quit:0`); the pc UI's keyboard queue |
| `k1movie.cpp` | movies: `MacPlayBinkGL` (k1port.cpp) finds the file as the original does (as given, backslashes to slashes, else the patch OBB, else the main OBB) and k1movie.cpp plays that stream through k2movie (`k2movie_play_io`, platforms/video). The original's rules: Bink volume 0..0x8000, a skip only after the first second and never while `bSound` is off (the evil ending), the out-flag never written, `g_RenderSkip = 1` after a movie that played. `K2_SKIP_MOVIES=1` skips them all |
| `k1zip.cpp`, `k1lzma.c`, `deps.cmake` | miniz 3.0.2 + LZMA SDK 24.09 (pinned, `platforms/out/_deps-k1`) |

The FMOD wrapper (`FModAudioSystem`, also in libandroid_port.so) is the tree's own
`src/port/fmod/FModAudioSystem.cpp`, compiled into the host K1 engine (`K2_HOST_BUILD`).

## Port behaviours the engine depends on

* `ObbFile` stream seeks past the end FAIL (-1): IosLocalizedStrings' line reader (a K1 bug: its read
  buffer is never NUL-terminated) only stops on that failure -- otherwise the in-game HUD's
  constructor loops forever on the first module load.
* `glGetIntegerv(GL_VIEWPORT)` answers the port's own record (AurGUIStartLayer sizes the GUI from it).

## Gaps

* Movie subtitles are not drawn (the original's `BinkSubtitles` reads `<movie>.bik.<lang>sub.txt`, which exist for
  de/es/fr/it only). Skip input is k2movie's (any release, keys and mouse included) rather than the original's
  finger / joystick-button presses.
* The Movies menu lists Credits (the `CSWGuiInGameCredits` text sequence, not a Bink file) plus only the movies the
  profile has seen (`[Movies Shown]` in swkotor.ini) or that movies.2da marks `AlwaysShow`, as the original does. The
  intro logos play on the first run only (no `Disable Sound` entry in swkotor.ini yet).
* No engine FUNCTION is unresolved (session 15). The ~1,100 "engine" symbols in `report/linkreport.md` are DATA
  (globals, cvars, tables, shader text) whose initializers the stub file lifts from libKOTOR.so; an unexported
  (hidden) K1 global that src/ declares but never defines comes out ZERO, so define it with its K1 value. Any
  unresolved function would be a soft stub logging `k2: unimplemented …` (`--hard` aborts).
* No ARB program ever reaches GL, as on the device: `k1kotorgl.cpp` maps the engine's fragment-program
  ids onto the port's own GLSL programs (`ProgramType`: 6 bump, 19/20 cube map, 22/39 lightmap + cube
  map, 24/29 bump + cube map, 26 lightmap, 40 post, 41 bloom, 43 flat cube map; every other id the plain
  program; the port adds 0x100 and 0x101, the PC's force-distortion and Force Speed blur passes, 0x102,
  a plain copy, and 0x103, the PC's soft-shadow blur, whose fragment shaders are built in) and feeds them the vertex-program env parameters (bones 0x12-0x44, lights 0x49-0x57, camera
  0x59-0x5c, material 0x5d-0x5f). Draws with no fragment program bound are fixed-function (gles2-bc on
  the device; desktop GL / gl4es here). The shaders come from the APK's assets on Android and from the
  working directory on macOS (`build_run_macos.sh` copies them into `gamedir-k1/`).
