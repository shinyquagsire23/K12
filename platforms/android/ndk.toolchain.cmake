# NDK toolchain for the platforms/ Android presets (<k1|k2>-cross-android-<ui>), on macOS, Linux or
# Windows.
#
# Not the matching NDK: hybrid/ pins r21d (toolchain/android-ndk-r21d) because its objects must
# match the original byte for byte. This build is a port, compiled like the macOS host build, so it
# takes a current NDK (clang 17+): $ANDROID_NDK_HOME, else the newest <sdk>/ndk/<version> with
# <sdk> = $ANDROID_HOME / $ANDROID_SDK_ROOT / Android Studio's default (~/Library/Android/sdk,
# ~/Android/Sdk, %LOCALAPPDATA%\Android\Sdk).
if(NOT K2_NDK)
  file(TO_CMAKE_PATH "$ENV{ANDROID_NDK_HOME}" _k2_ndk_env)
  if(_k2_ndk_env AND EXISTS "${_k2_ndk_env}/build/cmake/android.toolchain.cmake")
    set(K2_NDK "${_k2_ndk_env}")
  else()
    foreach(_sdk "$ENV{ANDROID_HOME}" "$ENV{ANDROID_SDK_ROOT}" "$ENV{HOME}/Library/Android/sdk" "$ENV{HOME}/Android/Sdk"
                 "$ENV{LOCALAPPDATA}/Android/Sdk")
      file(TO_CMAKE_PATH "${_sdk}" _sdk)
      if(_sdk AND IS_DIRECTORY "${_sdk}/ndk")
        file(GLOB _ndks LIST_DIRECTORIES true "${_sdk}/ndk/*")
        list(SORT _ndks COMPARE NATURAL ORDER DESCENDING)
        foreach(_n IN LISTS _ndks)
          if(EXISTS "${_n}/build/cmake/android.toolchain.cmake")
            set(K2_NDK "${_n}")
            break()
          endif()
        endforeach()
      endif()
      if(K2_NDK)
        break()
      endif()
    endforeach()
  endif()
  if(NOT K2_NDK)
    message(FATAL_ERROR "No Android NDK found: set ANDROID_NDK_HOME (r25 or newer)")
  endif()
  set(K2_NDK "${K2_NDK}" CACHE PATH "Android NDK used by the platforms/ Android build")
endif()

set(ANDROID_ABI "arm64-v8a" CACHE STRING "")        # the original APK is arm64-only, and so is this
set(ANDROID_PLATFORM "android-23" CACHE STRING "")  # the original's minSdkVersion
set(ANDROID_STL "c++_static" CACHE STRING "")
include("${K2_NDK}/build/cmake/android.toolchain.cmake")
