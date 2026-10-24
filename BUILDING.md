## Building K12

Building is currently tested primarily on Ubuntu 24.04, Windows 11 (VS2022), and MacOS 27. Windows releases can also be cross-compiled using MinGW.

Every build is a CMake preset in `CMakePresets.json`, `cmake --list-presets` shows the ones your host can build:

| preset | builds | output |
| --- | --- | --- |
| `k2-host-pc`, `k2-host-touch`, `k2-host-xbox` (and `k1-host-*`) | using the host machine compiler | `platforms/out/<preset>/{macos,linux,windows}/k2` (or `k1`) |
| `k2-host-pc-asan`, `k1-host-pc-asan` | with AddressSanitizer | same as above |
| `<game>-cross-windows-<ui>` | Win64 with llvm-mingw, from any host | `platforms/out/<preset>/windows/k2.exe` |
| `<game>-cross-windows-msvc-pc`, `<game>-cross-windows-cl-pc` | Win64 with clang-cl / with `cl.exe` under Wine (macOS, Linux) | `platforms/out/<preset>/windows/k2.exe` |
| `<game>-cross-ios-<ui>`, `<game>-cross-ios-sim-<ui>` | the iOS app (macOS only) | `platforms/out/<preset>/ios/` |
| `<game>-cross-android-touch`, `<game>-cross-android-xbox` | the Android APK (any host) | `platforms/out/<preset>/<game>-android-<ui>.apk` |

- `<game>` is `k2` or `k1`. 
- `<ui>` is `pc` (keyboard and mouse), `touch` (the Android touch UI, which needs the Android data) or `xbox` (the console UI, for a controller). 

### Original Assets

None of the original files are required to compile the desktop builds, but some build steps and all of the matching tools use them.

You can purchase the games from one of these links. You **must** own a valid license for the game you are compiling for, no assets are provided.

**K1**:
 - [Steam](https://store.steampowered.com/app/32370/STAR_WARS_Knights_of_the_Old_Republic/)
 - [GOG](https://www.gog.com/en/game/star_wars_knights_of_the_old_republic)
 - [Google Play](https://play.google.com/store/apps/details?id=com.aspyr.swkotor)

**K2**:
 - [Steam](https://store.steampowered.com/app/208580/STAR_WARS_Knights_of_the_Old_Republic_II__The_Sith_Lords/)
 - [GOG](https://www.gog.com/en/game/star_wars_knights_of_the_old_republic_ii_the_sith_lords)
 - [Google Play](https://play.google.com/store/apps/details?id=com.aspyr.swkotorii)

Other things to pull in:

* **`decomp_bins/` assets**, used by the helper scripts and tools:
  * `swkotorii_obb/`, `swkotor_obb/`: the OBBs above. The `build_run_*.sh` scripts run off them, the Android/iOS install steps copy them to the device.
  * `swkotorii_extracted/`: `swkotorii.apk`, `apktool x`. `lib/arm64-v8a/libkotor2.so` is the matching target.
  * `swkotor_extracted/`: `swkotor.apk`, unzipped. `lib/arm64-v8a/libKOTOR.so` is K1's matching target, and `assets/iosdialog.otf` (Aspyr's font for K1's touch-UI widgets, which the K1 OBBs lack, so it is not in the repo) is bundled into the K1 Android and iOS apps and used by the desktop K1 touch UI. Only the font comes from there: the GLSL shaders the original APK also carried are vendored in `resources/shaders/` and compiled into every build.
  * `KOTOR2_MacOS/KOTOR2sub` (optional): the macOS Steam executable, a symbolized reference for the link report and some tools.
* **`toolchain/android-ndk-r21d/`** (matching build only): NDK r21d, the compiler the original was built with.

```
K12/
├── decomp_bins/
│   ├── KOTOR2_MacOS/
│   │   └── KOTOR2sub
│   ├── swkotor_extracted/
│   │   ├── assets/
│   │   │   ├── iosdialog.otf
│   │   │   ├── kotor.frag
│   │   │   ├── kotor.vert
│   │   │   └── ...
│   │   └── lib/arm64-v8a/
│   │       └── libKOTOR.so
│   ├── swkotor_obb/
│   │   ├── main.53.com.aspyr.swkotor.obb
│   │   └── patch.53.com.aspyr.swkotor.obb
│   ├── swkotorii_extracted/
│   │   └── lib/arm64-v8a/
│   │       └── libkotor2.so
│   └── swkotorii_obb/
│       ├── main.213.com.aspyr.swkotorii.obb
│       └── patch.14.com.aspyr.swkotorii.obb
├── toolchain/
│   └── android-ndk-r21d/
├── platforms/
└── src/
```

### 1) Install dependencies

<details>
  <summary>Arch Linux Dependencies</summary>

These package names have not been tested yet, let me know if they work.

```
# All
pacman -S git clang lld llvm libc++ sdl2-compat freetype2 libglvnd glu zlib ninja cmake python librsvg

# Win32/MinGW (platforms/windows/get_llvm_mingw.sh download)
pacman -S curl xz
```
</details>

<details>
  <summary>Ubuntu Dependencies</summary>

```
# Make sure everything is up to date before installing new dependencies
sudo apt-get update
sudo apt-get upgrade

# All
sudo apt install git clang lld llvm libc++-dev libc++abi-dev libsdl2-dev libfreetype-dev libgl-dev libglu1-mesa-dev zlib1g-dev ninja-build cmake python3 librsvg2-bin

# Win32/MinGW (platforms/windows/get_llvm_mingw.sh download)
sudo apt install curl xz-utils
```
</details>

<details>
  <summary>MacOS Dependencies</summary>

Before starting, install Xcode. This is required for OpenGL headers, among other things.
```
# All
brew install cmake ninja sdl2-compat freetype llvm librsvg

# Win32/MinGW building: nothing from Homebrew, platforms/windows/get_llvm_mingw.sh downloads llvm-mingw
```
</details>

<details>
  <summary>Windows Dependencies</summary>

Download and install [Python 3](https://www.python.org/downloads/), 3.9 or newer (be sure it is added to your PATH when installing).

Download and install Visual Studio 2022 with the "Desktop development with C++" workload, which includes CMake and Ninja ("C++ CMake tools for Windows").

For the MinGW presets (`*-cross-windows-*`), also unpack `llvm-mingw-<ver>-ucrt-x86_64.zip` from the [llvm-mingw releases](https://github.com/mstorsjo/llvm-mingw/releases) and set `K2_LLVM_MINGW` to its folder (or put its `bin\` on `PATH`).

</details>

<details>
  <summary>Android Dependencies (any host)</summary>

* An NDK r25 or newer (`ANDROID_NDK_HOME`, else the newest under the SDK's `ndk/`).
* The SDK's build-tools 34 and `platforms;android-34` (`ANDROID_HOME`, else Android Studio's default SDK folder).
* A JDK (`JAVA_HOME`, e.g. Android Studio's `jbr`, else `javac` on `PATH`), Python 3, CMake and Ninja.

Android Studio's SDK Manager installs all of these, or with the command-line tools:

```
sdkmanager "ndk;26.3.11579264" "build-tools;34.0.0" "platforms;android-34" "platform-tools"
```

On Windows, build from a Visual Studio developer prompt (for Ninja), or pick the preset in Visual Studio.
</details>

### 2) Compile the desired target

<details>
  <summary>64-bit Linux (x86_64, ARM64)</summary>

```
git clone https://github.com/shinyquagsire23/K12.git
cd K12

cmake --preset k2-host-pc                # or k2-host-xbox / k1-host-pc / ...
cmake --build --preset k2-host-pc        # -> platforms/out/k2-host-pc/linux/k2
./platforms/out/k2-host-pc/linux/k2                # finds an installed copy (Steam, GOG)
```

`./build_run_linux.sh [--k1] [--touch|--xbox] [--obb-dir DIR]` at the repo root builds and runs off the OBBs instead (`K2_OBB_DIR`, below). libc++ is linked statically, so the result also runs on SteamOS. See below for details on compiling on-device.


### Steam Deck (SteamOS)

SteamOS has no compiler and a read-only root, so build in a **distrobox Ubuntu 24.04** container and
run the result on SteamOS itself: libc++ is linked in, the other libraries the executable
needs are part of SteamOS.

In Desktop Mode, in Konsole (distrobox and podman ship with SteamOS 3.5 and later):

```sh
distrobox create --name kotor --image ubuntu:24.04
distrobox enter kotor
# inside the container (your home folder is the same one SteamOS sees):
sudo apt update
sudo apt install git clang lld llvm libc++-dev libc++abi-dev libsdl2-dev libfreetype-dev libgl-dev \
                 libglu1-mesa-dev zlib1g-dev ninja-build cmake python3 librsvg2-bin
git clone https://github.com/shinyquagsire23/K12.git ~/K12 && cd ~/K12
cmake --preset k2-host-xbox && cmake --build --preset k2-host-xbox
cmake --preset k1-host-xbox && cmake --build --preset k1-host-xbox
exit
```

</details>

<details>
  <summary>64-bit Windows, using MinGW</summary>

64-bit Windows can be cross-compiled from Linux or MacOS.

```
git clone https://github.com/shinyquagsire23/K12.git
cd K12

platforms/windows/get_llvm_mingw.sh      # unpacks llvm-mingw into platforms/out/_llvm-mingw
cmake --preset k2-cross-windows-pc && cmake --build --preset k2-cross-windows-pc   # -> platforms/out/k2-cross-windows-pc/windows/k2.exe
cmake --preset k1-cross-windows-pc && cmake --build --preset k1-cross-windows-pc   # -> platforms/out/k1-cross-windows-pc/windows/k1.exe
```

The .exe can be swapped in place for `swkotor.exe` and/or `swkotor2.exe`, or run on its own (it will automatically find the game assets, but Steam Input and achievements may not be functional unless the game is run through Steam).
</details>

<details>
  <summary>MacOS ARM64</summary>

```
git clone https://github.com/shinyquagsire23/K12.git
cd K12

cmake --preset k2-host-pc                # or k2-host-xbox / k1-host-pc / ...
cmake --build --preset k2-host-pc        # -> platforms/out/k2-host-pc/macos/k2
./platforms/out/k2-host-pc/macos/k2                # finds an installed copy (Steam, GOG, the Mac app bundles)
```

`./build_run_macos.sh [--k1] [--touch|--xbox]` at the repo root builds and runs off the OBBs in `decomp_bins/` instead (`--help` lists the options).
</details>

<details>
  <summary>iOS</summary>

```
git clone https://github.com/shinyquagsire23/K12.git
cd K12

cmake --preset k2-cross-ios-pc && cmake --build --preset k2-cross-ios-pc   # -> platforms/out/k2-cross-ios-pc/ios/
cmake --build --preset k2-cross-ios-pc --target ipa                         # -> platforms/out/k2-cross-ios-pc/k2port-pc.ipa
```

Installing and running in the Simulator:

```
./build_run_ios.sh --sim [--k1] [--touch|--xbox]
# or by hand:
cmake --preset k2-cross-ios-sim-pc && cmake --build --preset k2-cross-ios-sim-pc
cmake --build --preset k2-cross-ios-sim-pc --target run-sim   # installs, copies the OBBs, launches
```

The Simulator renders on the CPU, so expect poor performance.

Installing assets and running on-device:

```
./build_run_ios.sh [--k1] [--touch|--xbox]   # the first paired iPhone / iPad (--device NAME|UDID)
```

This re-signs the app with an "Apple Development" identity and a development profile from Xcode that covers the device, installs it with `xcrun devicectl`, copies the OBBs from `decomp_bins/` into the app's Documents folder (once) and launches it with the console attached. Without the script, put the two OBBs in the app's Documents folder yourself (Finder's device pane > Files, or the Files app).

The deployment target is **iOS 15.0**, and the build is signed ad hoc by default (what the Simulator runs). To sign a device build at build time, configure with `-DK2I_CODESIGN_IDENTITY="Apple Development: you@example.com (XXXXXXXXXX)"`, `-DK2I_PROVISIONING_PROFILE=<file.mobileprovision>` and, if the profile is not a wildcard, a matching `-DK2I_BUNDLE_ID=` (the default is `com.k2port.swkotorii.<ui>` / `com.k2port.swkotor.<ui>`). The entitlements come from the profile. `build_run_ios.sh` takes `--profile FILE` and `--bundle-id ID` for the same.

</details>


<details>
  <summary>Android</summary>

```
git clone https://github.com/shinyquagsire23/K12.git
cd K12

cmake --preset k2-cross-android-touch && cmake --build --preset k2-cross-android-touch   # -> platforms/out/k2-cross-android-touch/k2-android-touch.apk
```

Use `k2-cross-android-xbox` for the console UI and `k1-cross-android-touch` / `k1-cross-android-xbox` for K1. The K2 APK installs as `com.aspyr.swkotorii.hybrid` and K1 as `com.aspyr.swkotor.hybrid`, beside Aspyr's ports.

Installing and running in the emulator:

```
./build_run_android.sh --avd <AVD name> [--k1] [--xbox]   # boots the AVD if no device is attached
```

This builds, installs, copies the OBBs, launches and follows logcat. `--env "K2_STUBS=soft;K2_SKIP_MOVIES=1"` passes `K2_*` switches to the launch.

Installing assets and running on-device:

```
cmake --build --preset k2-cross-android-touch --target install-device   # adb install, + the OBBs if missing
cmake --build --preset k2-cross-android-touch --target run-device       # launch + follow logcat
```

The OBBs go to `/sdcard/Android/obb/<package>/`, renamed for the package. They are copied from the installed original game if it is on the device, else pushed from `decomp_bins/swkotorii_obb/` (`swkotor_obb/` for K1).

</details>

<details>
  <summary>Visual Studio 2022 Project</summary>

Clone the repository using git. Then open VS 2022 and select `Open a local folder`. Select and open the repository's root folder. Then, select the CMake target you want (usually `k1-host-pc` or `k2-host-pc`), select either k1.exe or k2.exe as the startup item, and launch/debug it.

The `k2-host-*` / `k1-host-*` presets build with RelWithDebInfo, and `k2-host-pc-asan` adds AddressSanitizer. The `*-cross-windows-*` presets use llvm-mingw instead (set `K2_LLVM_MINGW` first). From a shell, run `vcvars64.bat` first, then the same `cmake --preset` / `cmake --build --preset` commands as above.

Sometimes if there are significant CMake changes, you may need to right-click `CMakeLists.txt` and select `Delete Cache and Reconfigure`.
</details>

### Build options

<details>
  <summary>Click To See Details</summary>

Pass these to `cmake --preset <name> -D<OPTION>=<value>`:

| option | default | what |
| --- | --- | --- |
| `K2P_GAME` | from the preset | `k2` or `k1` (defines `KOTOR1`) |
| `K2P_UI` | from the preset | `pc`, `touch` or `xbox` |
| `K2P_MOBILE_OPT` | `OFF` | put the mobile port's cuts back (culling, particle caps, no dangly physics...), for an A/B or a weak GPU |
| `K2P_FIX_ENGINE_BUGS` | `ON` | fixes for bugs in the original engine; `OFF` reproduces them |
| `K2P_QOL_IMPROVEMENTS` | `ON` | the Controller options page and the other new features |
| `K2P_ASAN` | `OFF` | AddressSanitizer (the `-asan` presets) |
| `K2P_AUDIO` | `miniaudio` | `null` for silent audio |
| `K2P_BINK_SUBTITLES` | `OFF` | draw movie subtitles |
| `K2P_COV` | `OFF` | per-function runtime coverage for `tools/covrank.py` |
| `K2P_MSVC_WARN` | `OFF` | `cl.exe` only: `/W4 /analyze` instead of no warnings |
| `K2P_LINKREPORT` | `ON` if `decomp_bins/` has the original `.so` | classify every symbol the engine leaves to the platform (`tools/linkreport.py`); the build does not need it |
| `K2P_STATIC_LIBCXX` | `ON` | Linux: link libc++ statically |
| `K2P_OBB_DIR`, `K2P_GAME_DATA_DIR` | `decomp_bins/<game>_obb/`, then Steam's copy | the game's glyph-font atlases the button-prompt art is laid into. The art itself is `resources/*.svg` (rendered through `resources/padglyph_renders.zip`); the atlases also hold the text characters, so they come from the game's data: the OBBs first (K2: the Switch dump's and the extracted APK's loose `override/` ahead of them), else the installed PC game |

Run-time switches (`K2_*` environment variables: data folders, window size, screenshots, scripted input, logging) are set using environment variables. On Android they go in the launch's `--env`, on iOS in `Documents/k2env.txt`. The one for game data that is the same everywhere: `K2_OBB_DIR=<folder>` runs either game off the Android OBBs in that folder (each game mounts its own `main.<version>.<package>.obb` and `patch.<version>.<package>.obb`, the highest version of each, so one folder can hold both games'). The `build_run_*.sh` scripts' `--obb-dir` sets it; Android sets it to the app's obb folder, iOS defaults it to Documents. The older `K2_OBB=<main.obb>;<patch.obb>` (`;` on every platform, `:` also accepted off Windows) and `K1_OBB_DIR=<folder>` still work.
</details>

### Byte-matching build (decompilation work)

<details>
  <summary>Click To See Details</summary>

This is separate from everything above: it compiles each `src/` file with the original's compiler and compares the result against the shipped library, function by function. It needs:

* macOS (`tools/cc.sh` runs NDK r21d's `darwin-x86_64` clang, under Rosetta 2 on Apple silicon),
* NDK r21d unpacked into `toolchain/android-ndk-r21d/` (or `NDK_DIR=<path>`),
* Python 3 with `pyelftools` (`pip3 install pyelftools`),
* `decomp_bins/swkotorii_extracted/lib/arm64-v8a/libkotor2.so` (and `decomp_bins/swkotor_extracted/lib/arm64-v8a/libKOTOR.so` for K1).

From the repo root:

```
tools/cc.sh src/<path>/<TU>.cpp                 # -> build/<TU>.o
python3 tools/match.py src/<path>/<TU>.cpp      # MATCH / DIFF per function in that file
python3 tools/progress.py                       # the whole tree: K2's headline numbers
python3 tools/k1/k1match.py --all --build       # K1's count (-DKOTOR1 -O3 objects in build/k1obj)
```
</details>