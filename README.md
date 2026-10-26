# K12

Byte-matching decompilation of the Android ports of the first and second Knights of the Unc Representative Democracy games.

## Platform Status

All platforms work with original assets, as well as the Android OBBs. The desktop builds find an installed Steam/GOG/disc copy on their own and share its saves (they can also take environment variables `K2_ASSET_DIR=<game folder>` / `K2_USER_DIR=<saves folder>`).

To play using the OBBs instead, set `K2_OBB_DIR=<folder with the OBBs>` (both games pick its own `main.*` / `patch.*` files by package name, so one folder can hold K1's and K2's), and run from a writable folder (or `K2_DATA_DIR=<dir>`) where the ini and saves will go. Android reads the OBBs from the usual `Android/obb/<package>/` folder, iOS from the app's Documents folder.

| Configuration | Graphics | Testing Status |
| --- | --- | --- |
| ARM64 Android | OpenGL ES 2.0 + gl4es | K2 has been tested with TSLRCM from start to credits. K1 has been completed through Dantooine. |
| 64-bit Windows (MinGW Cross) | OpenGL 2.1 (compatibility context) | Only basic manual tests performed. |
| 64-bit Windows (VS2022) | OpenGL 2.1 (compatibility context) | Automated map rotation burn-in tests with ASan; K1 has been completed through Dantooine, K2 has not been tested beyond early portions of Peragus, minigames, and some save load smell tests. |
| MacOS x86_64/AArch64 | OpenGL 2.1 (legacy context) | Primarily used for regression tests and 4K PC UI tests. |
| 64-bit Linux | OpenGL 2.1 (compatibility context) | Only basic tests performed. |
| ARM64 iOS | OpenGL ES 2.0 + gl4es | Only basic tests performed. |

### Original Assets

You can purchase the games from one of these links. You **must** own a valid license for the game you are compiling for, no assets are provided.

**K1**:
 - [Steam](https://store.steampowered.com/app/32370/STAR_WARS_Knights_of_the_Old_Republic/)
 - [GOG](https://www.gog.com/en/game/star_wars_knights_of_the_old_republic)
 - [Google Play](https://play.google.com/store/apps/details?id=com.aspyr.swkotor)

**K2**:
 - [Steam](https://store.steampowered.com/app/208580/STAR_WARS_Knights_of_the_Old_Republic_II__The_Sith_Lords/)
 - [GOG](https://www.gog.com/en/game/star_wars_knights_of_the_old_republic_ii_the_sith_lords)
 - [Google Play](https://play.google.com/store/apps/details?id=com.aspyr.swkotorii)

The final binary requires one of the following:
 * an installed PC/Mac copy (Steam, GOG, disc). The desktop builds find it on their own, or take `K2_ASSET_DIR=<folder with chitin.key>`.
 * the Android OBB files: K2's `main.213.com.aspyr.swkotorii.obb` + `patch.14.com.aspyr.swkotorii.obb`, or K1's `main.53.com.aspyr.swkotor.obb` + `patch.53.com.aspyr.swkotor.obb`. These are required for the touch UI, and on Android and iOS. The desktop builds take the folder they are in as `K2_OBB_DIR=<folder>`.
 * K1's touch UI also needs the original K1 APK's `assets/iosdialog.otf`.

## K1/K2 interoperation

Both games build from the one `src/` tree, which is K2's engine. Wherever K1 differs (structs, signatures, whole function bodies, K1-only functions) the code branches on `#if KOTOR1`. 

For matching, `tools/cc.sh` with `EXTRA_CFLAGS=-DKOTOR1` builds the K1 objects with K1 CFLAGS and `tools/k1/k1match.py --all --build` scores them against `libKOTOR.so`.

## Restored Engine Features

Things the Android builds cut or never had, brought back from the PC, Xbox or Switch versions:

- **PC UI** (`K2P_UI=pc`): the PC layouts and art from the game archives instead of the touch re-layouts, the PC HUD with its menu bar and per-box arrows, the PC main menu with Quit, the PC conversation and computer screens, mouse hover, hardware `.cur` cursors and right-drag mouse look.
- **Xbox UI** (`K2P_UI=xbox`): the console HUD and action bar, the console Pazaak and swoop-upgrade screens and the console name keyboard. Some creative liberties were taken, such as saves still having named slots.
- **Controller rumble**: `rumble.2da` is loaded and drives SDL rumble or haptics (or the phone's vibrator on Android).
- **Framebuffer effects for K1**: the PC's Force Push / stealth distortion and Force Speed motion blur, all cut on the Android port.
- **Soft shadows** (both games): the PC's image-space soft-shadow pass, also cut on both Android ports.
- **Anti-aliasing**: uses SSAA, MSAA/other AA is not implemented currently.
- **Mobile optimizations**: Optimizations specific to the mobile ports (size-over-distance culls past 15m, dangly mesh physics removal, forced 720p, reduced particle birthrates, reduced draw distances and added fog) are behind ifdefs.
- **Display options**: Screen Resolution and a Fullscreen setting (native resolution, notch-aware on macOS) on the desktop.
- **K1 Achievements on Desktop**: Shows the Google Play/Game Center achievements on desktop K1 as a corner bark bubble.

New additions:

- **Controller options page**: button prompts (Automatic / Xbox / PlayStation / Switch / GameCube), Swap A/B and X/Y, Disable Right Stick Pitch (K2), Faster Movement (K1 Android's 1.3x run speed), and Achievements on/off (K1).
- **Text Size** (K2): 100%-250% for conversations, menus and the HUD text.
- **Steam integration**: finds Steam/GOG installs and shares their saves, uses K2's Steam Cloud saves folder, mounts K2 Steam Workshop mods, and reports K2 achievements to Steam (local achievements otherwise).
- **HUD scaling**: target reticles and area-map markers scale with the screen.

## Replaced Dependencies

- **FMOD** -> [miniaudio](https://github.com/mackron/miniaudio) 0.11.25 (Unlicense or MIT-0), behind the engine's FMOD API (`platforms/audio/`). Steam Audio is planned for occlusion/HRTF, not integrated yet.
- **RAD Bink** -> k2bink, a C99 port of [unbikit](https://github.com/blennies/unbikit) (MIT or Apache-2.0)
- **gles2-bc** -> [gl4es](https://github.com/ptitSeb/gl4es) (MIT) on Android and iOS; the desktops use OpenGL 2.1.
- **JNI stuff, Google Play, Firebase and Crashlytics** -> rewritten
- **Steamworks SDK** -> none: a `steam_api` library is used if one sits beside the game, otherwise the running Steam client's `steamclient` is loaded directly.

## Building

See [BUILDING.md](BUILDING.md)

## Contributing

The only contributions accepted are playtesting, issue submissions, and matching improvements; monetary contributions cannot be accepted.

## Progress

`python3 tools/progress.py`

```
engine          : 14,337 funcs / 5,009,668 bytes  (of 20,881/6,121,816 raw dynsym)
transcribed     : 13990 funcs / 4,960,228 bytes  (99.0131% of engine)
exact-matched   : 12040 funcs / 2,584,908 bytes  (51.5984% of engine)  <- COVERAGE
match rate      : 12040/13990 funcs (86.1%), 2,584,908/4,960,228 bytes (52.1%)  <- of what we transcribed
```
