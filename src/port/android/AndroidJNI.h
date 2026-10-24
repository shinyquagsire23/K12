#ifndef ANDROIDJNI_H
#define ANDROIDJNI_H

#include <jni.h>
#include "port/sdlapi.h"
#include "port/asllanguage.h"

extern "C" JNIEnv* Android_JNI_GetEnv();


extern SDL_mutex* g_UI_mutex;
extern SDL_cond*  g_UI_cond;
#if KOTOR1  // K1 reloads it at every use
extern volatile bool       g_done;
#else
extern bool       g_done;
#endif



bool HasJoystick();
bool IsTablet();

extern bool g_IsAmazon;
extern bool g_AmazonFlavorChecked;

// The privacy prompt's state, from the Java side or the ini.
extern bool g_PrivacyChecked;
extern bool g_PrivacyConsentGiven;

enum CloudSynchStatus {
    CloudSynchStatus_Idle = -1,
    CloudSynchStatus_Succeeded = 0,
    CloudSynchStatus_Synchronizing = 1,
    CloudSynchStatus_Failed = 2,
    CloudSynchStatus_NoData = 3,
    CloudSynchStatus_Reported = 4,
};
extern CloudSynchStatus g_CloudSynchStatus;

void CheckFlavor();

extern bool g_TouchScreenStatusChecked;
// volatile: the flag is re-read after the probe.
extern volatile bool g_HasTouchScreen;

extern int g_SDL_BufferGeometry_w;
extern int g_SDL_BufferGeometry_h;

bool HasTouchScreen();
void CheckTouchScreenAvailable();
bool GetHighResolution();
void SetHighResolution(bool bHigh);
float GetScreenHeightInch();
int GetScreenHeightPixel();
bool ASLIsDeviceWidescreen();
bool ASLIsDeviceSizeMedium();
bool ASLIsDeviceSizeLarge();
bool ASLIsDeviceSizeSmall();

enum AndroidLogPriority { ANDROID_LOG_INFO = 4, ANDROID_LOG_DEBUG = 3 };
extern "C" int __android_log_print(int nPrio, const char* pszTag, const char* pszFmt, ...);
extern "C" int __android_log_write(int nPrio, const char* pszTag, const char* pszText);

// Each returns whether the device's corresponding Build field contains pszMatch,
// case-insensitively.
bool Manufacturer(const char* pszMatch);
bool Model(const char* pszMatch);
bool Brand(const char* pszMatch);
bool Display(const char* pszMatch);
bool Device(const char* pszMatch);

void ToggleRenderOverNotch(bool bToggle);
// Narrowed from the Java double to a signed 64-bit integer.
long long ASLPlat_GetDiskSpaceLeft();

void OutputDebugPrint(const char* pszFmt, ...);

#endif // ANDROIDJNI_H
