// Android/SDL port glue: the JNI bridges and the device/account queries.

#include "k2stdint.h"
#include "port/sdlapi.h"
#include "port/sdlconst.h"
#include "AndroidJNI.h"

#include "Achievements.h"

#include <cstdarg>
#include <string>
#include <algorithm>
#if KOTOR1
#include "port/k1obb.h"
#endif

CloudSynchStatus g_CloudSynchStatus = CloudSynchStatus_Idle;
SDL_mutex* g_UI_mutex;
SDL_cond* g_UI_cond;
#if KOTOR1
volatile bool g_done;
#else
bool g_done;
#endif

bool g_AmazonFlavorChecked;
bool g_PrivacyChecked;
bool g_PrivacyConsentGiven;
bool g_IsAmazon;
volatile bool g_HasTouchScreen;
bool g_TouchScreenStatusChecked;
#if !KOTOR1
volatile bool g_GooglePlay;
bool g_cloudSaveChanged;
#endif

const char* GetLocalizedCustomString(int nIndex);

bool HasJoystick()
{
    return SDL_NumJoysticks() != 0;
}

bool IsTablet()
{
    return SDL_IsTablet() != 0;
}

void SetCloudSynchStatus(CloudSynchStatus status)
{
    g_CloudSynchStatus = status;
}

CloudSynchStatus GetCloudSynchStatus()
{
    return g_CloudSynchStatus;
}

const char* GetCloudStatusString()
{
    int nStringId;
    switch (g_CloudSynchStatus) {
    case CloudSynchStatus_Succeeded:
        nStringId = 3;
        break;
    case CloudSynchStatus_Synchronizing:
        nStringId = 1;
        break;
    case CloudSynchStatus_Failed:
        nStringId = 4;
        break;
    case CloudSynchStatus_NoData:
        nStringId = 2;
        break;
    default:
        nStringId = 4;
        break;
    }
    return GetLocalizedCustomString(nStringId);
}

bool IsAmazon()
{
    if (!g_AmazonFlavorChecked)
        CheckFlavor();
    return g_IsAmazon;
}

void CheckFlavor()
{
    SDL_LockMutex(g_UI_mutex);
    if (!g_AmazonFlavorChecked) {
        g_AmazonFlavorChecked = true;

        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/base/ASPYR");
        jmethodID mid = env->GetStaticMethodID(cls, "isAmazon", "()Z");
        jboolean bIsAmazon = env->CallStaticBooleanMethod(cls, mid);
        env->DeleteLocalRef(cls);

        g_IsAmazon = bIsAmazon != 0;
    }
    SDL_UnlockMutex(g_UI_mutex);
}

#if KOTOR1
// K1 queries fresh every call and always accepts a Chromebook.
bool HasTouchScreen()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "HasTouchScreen", "()Z");
    jboolean bHasTouch = env->CallStaticBooleanMethod(cls, mid);
    env->DeleteLocalRef(cls);
    int bChromebook = SDL_IsChromebook();
    return bHasTouch != 0 || bChromebook != 0;
}
#else
bool HasTouchScreen()
{
    if (!g_TouchScreenStatusChecked) {
        g_TouchScreenStatusChecked = true;

        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/base/ASPYR");
        jmethodID mid = env->GetStaticMethodID(cls, "HasTouchScreen", "()Z");
        jboolean bHasTouch = env->CallStaticBooleanMethod(cls, mid);
        env->DeleteLocalRef(cls);

        g_HasTouchScreen = bHasTouch != 0;
    }
    return g_HasTouchScreen;
}
#endif

void CheckTouchScreenAvailable()
{
    if (!g_TouchScreenStatusChecked) {
        g_TouchScreenStatusChecked = true;

        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/base/ASPYR");
        jmethodID mid = env->GetStaticMethodID(cls, "HasTouchScreen", "()Z");
        jboolean bHasTouch = env->CallStaticBooleanMethod(cls, mid);
        env->DeleteLocalRef(cls);

        g_HasTouchScreen = bHasTouch != 0;
    }
}

bool GetHighResolution()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "GetHighResolution", "()Z");
    jboolean bHigh = env->CallStaticBooleanMethod(cls, mid);
    env->DeleteLocalRef(cls);
    return bHigh != 0;
}

void SetHighResolution(bool bHigh)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "SetHighResolution", "(Z)V");
    env->CallStaticVoidMethod(cls, mid, bHigh);
    env->DeleteLocalRef(cls);
}

#if KOTOR1
// K1 has no cache.
float GetScreenHeightInch()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "GetScreenHeightInch", "()F");
    jfloat fHeight = env->CallStaticFloatMethod(cls, mid);
    env->DeleteLocalRef(cls);
    return fHeight;
}
#else
float GetScreenHeightInch()
{
    static float fScreenHeightInch = 0.0f;
    if (fScreenHeightInch > 0.0f)
        return fScreenHeightInch;

    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "GetScreenHeightInch", "()F");
    jfloat fHeight = env->CallStaticFloatMethod(cls, mid);
    env->DeleteLocalRef(cls);

    fScreenHeightInch = fHeight;
    return fScreenHeightInch;
}
#endif

int GetScreenHeightPixel()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "GetScreenHeightPixel", "()I");
    jint nHeight = env->CallStaticIntMethod(cls, mid);
    env->DeleteLocalRef(cls);
    return nHeight;
}

#if KOTOR1
// K1 asks the Java side whether this device must skip SDL_GL_SetSwapInterval.
bool GetDisableSetSwapIntervalCall()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "GetDisableSetSwapIntervalCall", "()Z");
    jboolean bDisable = env->CallStaticBooleanMethod(cls, mid);
    env->DeleteLocalRef(cls);
    return bDisable != 0;
}
#endif

bool ASLIsDeviceWidescreen()
{
    return static_cast<float>(g_SDL_BufferGeometry_w) / static_cast<float>(g_SDL_BufferGeometry_h) >= 1.4f;
}

bool ASLIsDeviceSizeMedium()
{
    return false;
}

bool ASLIsDeviceSizeLarge()
{
    return true;
}

bool ASLIsDeviceSizeSmall()
{
    return GetScreenHeightInch() < 4.0f;
}

void ToggleRenderOverNotch(bool bToggle)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("org/libsdl/app/SDLActivity");
    jmethodID mid = env->GetStaticMethodID(cls, "ToggleRenderOverNotch", "(Z)V");
    env->CallStaticVoidMethod(cls, mid, bToggle);
    env->DeleteLocalRef(cls);
}

int ASLPlat_GetCurrentLanguage()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "getCurrentLanguage", "()I");
    const jint nLanguage = env->CallStaticIntMethod(cls, mid);
    env->DeleteLocalRef(cls);
    return nLanguage;
}

long long ASLPlat_GetDiskSpaceLeft()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "getDiskSpaceLeft", "()D");
    const jdouble fSpaceLeft = env->CallStaticDoubleMethod(cls, mid);
    env->DeleteLocalRef(cls);
    return static_cast<long long>(fSpaceLeft);
}

static int s_nDimmerRefCount;

void AcquireCinematicScreenLock()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "AcquireWakeLock", "()V");
    env->CallStaticVoidMethod(cls, mid);
    env->DeleteLocalRef(cls);
}

void ReleaseCinematicScreenLock()
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "ReleaseWakeLock", "()V");
    env->CallStaticVoidMethod(cls, mid);
    env->DeleteLocalRef(cls);
}

void ASLPlat_ASLEnableDimmer()
{
    if (--s_nDimmerRefCount <= 0) {
        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/base/ASPYR");
        jmethodID mid = env->GetStaticMethodID(cls, "ReleaseWakeLock", "()V");
        env->CallStaticVoidMethod(cls, mid);
        env->DeleteLocalRef(cls);
        s_nDimmerRefCount = 0;
    }
}

void ASLPlat_ASLDisableDimmer()
{
    if (s_nDimmerRefCount == 0) {
        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/base/ASPYR");
        jmethodID mid = env->GetStaticMethodID(cls, "AcquireWakeLock", "()V");
        env->CallStaticVoidMethod(cls, mid);
        env->DeleteLocalRef(cls);
    }
    ++s_nDimmerRefCount;
}

namespace ASL {

void ErrorAlertStr(const std::string& strTitle, std::string strMsg, ...);

bool ErrorAlertYesNoStr(const std::string& strTitle, std::string strMsg, bool bDefaultYes, ...);

}  // namespace ASL

class IosShowYesNoDialogHandler {
public:
    virtual void OnYesNoResult(bool bYes) = 0;
};

void IosShowDialog(const char* pszTitle, const char* pszMsg)
{
    ASL::ErrorAlertStr(std::string(pszTitle), std::string(pszMsg));
}

void IosShowYesNoDialog(const char* pszTitle, const char* pszMsg,
                        IosShowYesNoDialogHandler* pHandler)
{
    const bool bYes = ASL::ErrorAlertYesNoStr(std::string(pszTitle), std::string(pszMsg), false);
    pHandler->OnYesNoResult(bYes);
}

bool Manufacturer(const char* pszMatch)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("android/os/Build");
    jfieldID fid = env->GetStaticFieldID(cls, "MANUFACTURER", "Ljava/lang/String;");
    jstring str = static_cast<jstring>(env->GetStaticObjectField(cls, fid));
    const jsize nLen = env->GetStringUTFLength(str);
    char* pszBuf = new char[nLen + 1];
    env->GetStringUTFRegion(str, 0, nLen, pszBuf);
    env->DeleteLocalRef(str);
    pszBuf[nLen] = '\0';
    __android_log_print(ANDROID_LOG_INFO, "ASL_DEBUG", "Manufacturer: %s", pszBuf);
    const bool bMatch = strcasestr(pszBuf, pszMatch) != 0;
    delete[] pszBuf;
    env->DeleteLocalRef(cls);
    return bMatch;
}

bool Model(const char* pszMatch)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("android/os/Build");
    jfieldID fid = env->GetStaticFieldID(cls, "MODEL", "Ljava/lang/String;");
    jstring str = static_cast<jstring>(env->GetStaticObjectField(cls, fid));
    const jsize nLen = env->GetStringUTFLength(str);
    char* pszBuf = new char[nLen + 1];
    env->GetStringUTFRegion(str, 0, nLen, pszBuf);
    env->DeleteLocalRef(str);
    pszBuf[nLen] = '\0';
    __android_log_print(ANDROID_LOG_INFO, "ASL_DEBUG", "Model: %s", pszBuf);
    const bool bMatch = strcasestr(pszBuf, pszMatch) != 0;
    delete[] pszBuf;
    env->DeleteLocalRef(cls);
    return bMatch;
}

bool Brand(const char* pszMatch)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("android/os/Build");
    jfieldID fid = env->GetStaticFieldID(cls, "BRAND", "Ljava/lang/String;");
    jstring str = static_cast<jstring>(env->GetStaticObjectField(cls, fid));
    const jsize nLen = env->GetStringUTFLength(str);
    char* pszBuf = new char[nLen + 1];
    env->GetStringUTFRegion(str, 0, nLen, pszBuf);
    env->DeleteLocalRef(str);
    pszBuf[nLen] = '\0';
    __android_log_print(ANDROID_LOG_INFO, "ASL_DEBUG", "Brand: %s", pszBuf);
    const bool bMatch = strcasestr(pszBuf, pszMatch) != 0;
    delete[] pszBuf;
    env->DeleteLocalRef(cls);
    return bMatch;
}

bool Display(const char* pszMatch)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("android/os/Build");
    jfieldID fid = env->GetStaticFieldID(cls, "DISPLAY", "Ljava/lang/String;");
    jstring str = static_cast<jstring>(env->GetStaticObjectField(cls, fid));
    const jsize nLen = env->GetStringUTFLength(str);
    char* pszBuf = new char[nLen + 1];
    env->GetStringUTFRegion(str, 0, nLen, pszBuf);
    env->DeleteLocalRef(str);
    pszBuf[nLen] = '\0';
    __android_log_print(ANDROID_LOG_INFO, "ASL_DEBUG", "DISPLAY: %s", pszBuf);
    const bool bMatch = strcasestr(pszBuf, pszMatch) != 0;
    delete[] pszBuf;
    env->DeleteLocalRef(cls);
    return bMatch;
}

bool Device(const char* pszMatch)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("android/os/Build");
    jfieldID fid = env->GetStaticFieldID(cls, "DEVICE", "Ljava/lang/String;");
    jstring str = static_cast<jstring>(env->GetStaticObjectField(cls, fid));
    const jsize nLen = env->GetStringUTFLength(str);
    char* pszBuf = new char[nLen + 1];
    env->GetStringUTFRegion(str, 0, nLen, pszBuf);
    env->DeleteLocalRef(str);
    pszBuf[nLen] = '\0';
    __android_log_print(ANDROID_LOG_INFO, "ASL_DEBUG", "DEVICE: %s", pszBuf);
    const bool bMatch = strcasestr(pszBuf, pszMatch) != 0;
    delete[] pszBuf;
    env->DeleteLocalRef(cls);
    return bMatch;
}

// Formats into a fixed stack buffer; a longer result is retried with a growing heap buffer (up to
// 999999 bytes) before falling back to "?".
#include "port/libcbase.h"
#include "port/libcstdlib.h"
#include "port/crtapi.h"
#include "port/win32api.h"

std::string vformat(const char* pszFmt, va_list args)
{
    char szBuf[160];
    va_list ap;
    va_copy(ap, args);
    int n = vsnprintf(szBuf, sizeof(szBuf), pszFmt, ap);
    if (n >= 0 && n < static_cast<int>(sizeof(szBuf))) {
        szBuf[n] = '\0';
        return std::string(szBuf);
    }

    char* pszBuf = 0;
    int nCap = sizeof(szBuf);
    do {
        va_copy(ap, args);
#ifdef K2_FIX_ENGINE_BUGS
        char* pszGrown = static_cast<char*>(realloc(pszBuf, nCap * 2 + 1));
        if (pszGrown == 0) {
            free(pszBuf);
            return std::string("?");
        }
        pszBuf = pszGrown;
#else
        pszBuf = static_cast<char*>(realloc(pszBuf, nCap * 2 + 1));
#endif
        n = vsnprintf(pszBuf, nCap * 2, pszFmt, ap);
        nCap *= 2;
    } while ((n == -1 || n >= nCap) && static_cast<unsigned int>(nCap) <= 999999u);
#ifdef K2_FIX_ENGINE_BUGS
    if (n == -1 || n >= nCap) {
#else
    if (n == -1) {
#endif
        free(pszBuf);
        return std::string("?");
    }
    pszBuf[n] = '\0';
    std::string strResult(pszBuf);
    free(pszBuf);
    return strResult;
}

void OutputDebugPrint(const char* pszFmt, ...)
{
    va_list args;
    va_start(args, pszFmt);
    std::string sMsg = vformat(pszFmt, args);
    va_end(args);
    __android_log_write(ANDROID_LOG_DEBUG, "Debug", sMsg.c_str());
}


extern bool g_bIsInBackground;
#if !KOTOR1
bool g_bIsInBackground;
#endif
extern bool g_facebookLoggedIn;
bool g_facebookLoggedIn;
extern volatile bool g_GooglePlay;
extern bool g_cloudSaveChanged;
std::string gVendorId = "";

#if KOTOR1
extern "C" void Java_com_aspyr_kotor_KOTOR_nativeCreateMutex(JNIEnv*, jobject)
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativeCreateMutex(JNIEnv*, jobject)
#endif
{
    g_UI_mutex = SDL_CreateMutex();
    g_UI_cond  = SDL_CreateCond();
}

#if KOTOR1
extern "C" void Java_com_aspyr_kotor_KOTOR_nativeSignIn(JNIEnv*, jobject)
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativeSignIn(JNIEnv*, jobject)
#endif
{
    SDL_LockMutex(g_UI_mutex);
    g_GooglePlay = true;
    SDL_CondSignal(g_UI_cond);
    SDL_UnlockMutex(g_UI_mutex);
}

#if KOTOR1
extern "C" void Java_com_aspyr_kotor_KOTOR_nativeSignOut(JNIEnv*, jobject)
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativeSignOut(JNIEnv*, jobject)
#endif
{
    SDL_LockMutex(g_UI_mutex);
    g_GooglePlay = false;
    SDL_CondSignal(g_UI_cond);
    SDL_UnlockMutex(g_UI_mutex);
}

#if KOTOR1
extern "C" void Java_com_aspyr_kotor_KOTOR_nativePopupClosed(JNIEnv*, jobject)
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativePopupClosed(JNIEnv*, jobject)
#endif
{
    SDL_LockMutex(g_UI_mutex);
    SDL_CondSignal(g_UI_cond);
    SDL_UnlockMutex(g_UI_mutex);
}

#if KOTOR1
extern "C" void Java_com_aspyr_kotor_KOTOR_nativeQuitOk(JNIEnv*, jobject)
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativeQuitOk(JNIEnv*, jobject)
#endif
{
    SDL_LockMutex(g_UI_mutex);
    g_done = true;
    SDL_CondSignal(g_UI_cond);
    SDL_UnlockMutex(g_UI_mutex);
}

extern "C" void Java_com_aspyr_base_ASPYR_nativePrivacyChecked(JNIEnv*, jobject, jboolean bConsent)
{
    SDL_LockMutex(g_UI_mutex);
    g_PrivacyChecked = true;
    g_PrivacyConsentGiven = bConsent != 0;
    SDL_CondSignal(g_UI_cond);
    SDL_UnlockMutex(g_UI_mutex);
}

#if KOTOR1
extern "C" void Java_com_aspyr_kotor_KOTOR_nativeOnResume(JNIEnv*, jobject)
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativeOnResume(JNIEnv*, jobject)
#endif
{
    g_bIsInBackground = false;
}

#if KOTOR1
// K1 raises the corrupt-cloud-save popup and blocks until it is answered.
extern "C" void Java_com_aspyr_kotor_KOTOR_nativeOnSynchCloudSaveFailed(JNIEnv* env, jobject)
{
    SDL_LockMutex(g_UI_mutex);
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID mid = env->GetStaticMethodID(cls, "showCSCorruptPopup", "()V");
    env->CallStaticVoidMethod(cls, mid);
    env->DeleteLocalRef(cls);
    SDL_CondWait(g_UI_cond, g_UI_mutex);
    SDL_UnlockMutex(g_UI_mutex);
}

extern "C" void Java_com_aspyr_kotor_KOTOR_nativeProfilerClicked(JNIEnv*, jobject)
{
}
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativeOnSynchCloudSaveFailed(JNIEnv*, jobject)
{
}
#endif

extern "C" void Java_com_aspyr_base_ASPYR_nativeOnCloudSynchFailed(JNIEnv*, jobject)
{
    if (g_CloudSynchStatus == CloudSynchStatus_Synchronizing) {
        g_CloudSynchStatus = CloudSynchStatus_Failed;
        g_cloudSaveChanged = true;
    }
}

extern "C" void Java_com_aspyr_base_ASPYR_nativeOnCloudSynchNoData(JNIEnv*, jobject)
{
    if (g_CloudSynchStatus == CloudSynchStatus_Synchronizing) {
        g_CloudSynchStatus = CloudSynchStatus_NoData;
        g_cloudSaveChanged = true;
    }
}

#if KOTOR1
extern "C" void Java_com_aspyr_kotor_KOTOR_nativeExitApplication(JNIEnv*, jobject)
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativeExitApplication(JNIEnv*, jobject)
#endif
{
    exit(0);
}

extern "C" void Java_com_aspyr_kotor_KOTOR_OnLogUserLoggedIn(JNIEnv*, jobject, jboolean bLoggedIn)
{
    g_facebookLoggedIn = bLoggedIn != 0;
}

#if KOTOR1
// K1 sets the background flag only; no event is pushed.
extern "C" void Java_com_aspyr_kotor_KOTOR_nativeOnPause(JNIEnv*, jobject)
{
    g_bIsInBackground = true;
}
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativeOnPause(JNIEnv*, jobject)
{
    g_bIsInBackground = true;

    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.data1 = const_cast<char*>("ON_PAUSE");
    SDL_PushEvent(&event);
}
#endif

extern "C" void Java_com_aspyr_base_ASPYR_nativeCloudSaveService(JNIEnv*, jobject)
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    event.user.data1 = const_cast<char*>("CLOUD_PRESS");
    SDL_PushEvent(&event);
}

// Normalizes path separators, makes both paths absolute under the external-storage dir, and
// renames through libc.
#if !KOTOR1
int _rename(const char* pszFrom, const char* pszTo)
{
    std::string sFrom(pszFrom);
    std::replace(sFrom.begin(), sFrom.end(), '\\', '/');

    std::string sTo(pszTo);
    std::replace(sTo.begin(), sTo.end(), '\\', '/');

    std::string sFromAbs = std::string(SDL_AndroidGetExternalStoragePath()) + "/" + sFrom;
    std::string sToAbs   = std::string(SDL_AndroidGetExternalStoragePath()) + "/" + sTo;

    return rename(sFromAbs.c_str(), sToAbs.c_str());
}
#endif

extern "C" void Java_com_aspyr_base_ASPYR_nativeCloudSynchMoveFile(JNIEnv* env, jobject, jstring strFrom, jstring strTo)
{
    const char* pszFrom = env->GetStringUTFChars(strFrom, 0);
    const char* pszTo   = env->GetStringUTFChars(strTo, 0);

    SDL_LockMutex(g_UI_mutex);
    _rename(pszFrom, pszTo);
    SDL_UnlockMutex(g_UI_mutex);

    env->ReleaseStringUTFChars(strFrom, pszFrom);
    env->ReleaseStringUTFChars(strTo, pszTo);
}

#if KOTOR1
extern "C" void Java_com_aspyr_kotor_KOTOR_nativeOnSynchCloudSaveOver(JNIEnv* env, jobject, jstring strFrom, jstring strTo)
#else
extern "C" void Java_com_aspyr_base_ASPYR_nativeOnSynchCloudSaveOver(JNIEnv* env, jobject, jstring strFrom, jstring strTo)
#endif
{
    const char* pszFrom = env->GetStringUTFChars(strFrom, 0);
    const char* pszTo   = env->GetStringUTFChars(strTo, 0);

    SDL_LockMutex(g_UI_mutex);

    std::string sCmd = "rm -rf \"" + std::string(SDL_AndroidGetExternalStoragePath())
                       + "/" + std::string(pszTo) + "\"";
    system(sCmd.c_str());

    g_cloudSaveChanged = _rename(pszFrom, pszTo) == 0;
#if !KOTOR1
    g_CloudSynchStatus = CloudSynchStatus_Succeeded;
#endif

    SDL_UnlockMutex(g_UI_mutex);

    env->ReleaseStringUTFChars(strFrom, pszFrom);
    env->ReleaseStringUTFChars(strTo, pszTo);
}

extern "C" void Java_com_aspyr_base_ASPYR_nativeCloudSynchRemoveTempFolder(JNIEnv* env, jobject, jstring strFolder)
{
    const char* pszFolder = env->GetStringUTFChars(strFolder, 0);

    SDL_LockMutex(g_UI_mutex);

    std::string sCmd = "rm -rf \"" + std::string(SDL_AndroidGetExternalStoragePath())
                       + "/" + std::string(pszFolder) + "\"";
    system(sCmd.c_str());

    g_cloudSaveChanged = true;

    SDL_UnlockMutex(g_UI_mutex);

    env->ReleaseStringUTFChars(strFolder, pszFolder);
}

static std::string s_mainObbFileName;
static std::string s_patchObbFileName;

extern "C" void Java_com_aspyr_base_ASPYR_mainObbFileName(JNIEnv* env, jobject, jstring strObbName)
{
    const char* pszName = env->GetStringUTFChars(strObbName, 0);
    s_mainObbFileName.assign(pszName, strlen(pszName));
}

extern "C" void Java_com_aspyr_base_ASPYR_patchObbFileName(JNIEnv* env, jobject, jstring strObbName)
{
    const char* pszName = env->GetStringUTFChars(strObbName, 0);
    s_patchObbFileName.assign(pszName, strlen(pszName));
}

#if KOTOR1
// K1 mounts the two expansion archives itself, each opened twice: once for the resource layer and
// once for the audio system's streams.
extern ObbFile* g_fmod_mainObb;
extern ObbFile* g_fmod_patchObb;

extern "C" void Java_com_aspyr_kotor_KOTOR_mountObb(JNIEnv* env, jobject, jstring strPath)
{
    const char* pszPath = env->GetStringUTFChars(strPath, 0);
    g_mainObb = new ObbFile(pszPath, false);
    g_fmod_mainObb = new ObbFile(pszPath, true);
    env->ReleaseStringUTFChars(strPath, pszPath);
    g_obbMounted = true;
}

extern "C" void Java_com_aspyr_kotor_KOTOR_mountPatchObb(JNIEnv* env, jobject, jstring strPath)
{
    const char* pszPath = env->GetStringUTFChars(strPath, 0);
    g_patchObb = new ObbFile(pszPath, false);
    g_fmod_patchObb = new ObbFile(pszPath, true);
    env->ReleaseStringUTFChars(strPath, pszPath);
    g_patchObbMounted = true;
}

extern "C" void Java_com_aspyr_kotor_KOTOR_unmountObbs(JNIEnv*, jobject)
{
    if (g_patchObbMounted) {
        g_patchObbMounted = false;
        delete g_fmod_patchObb;
        g_fmod_patchObb = NULL;
        delete g_patchObb;
        g_patchObb = NULL;
    }
    if (g_obbMounted) {
        g_obbMounted = false;
        delete g_fmod_mainObb;
        g_fmod_mainObb = NULL;
        delete g_mainObb;
        g_mainObb = NULL;
    }
}
#endif

extern "C" void Java_com_aspyr_base_ASPYR_nativeOnVenderIdFetched(JNIEnv* env, jobject, jstring strVendorId)
{
    gVendorId = std::string(env->GetStringUTFChars(strVendorId, 0));
}

void AspyrSetupSDKs(unsigned char bPrivacyConsentGiven)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "AspyrSetupSDKs", "(Z)V");
    env->CallStaticVoidMethod(cls, mid, bPrivacyConsentGiven);
    env->DeleteLocalRef(cls);
}

void AndroidLaunchUrl(const char* pszUrl)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "launchURL", "(Ljava/lang/String;)V");
    jstring strUrl = env->NewStringUTF(pszUrl);
    env->CallStaticVoidMethod(cls, mid, strUrl);
    env->DeleteLocalRef(strUrl);
    env->DeleteLocalRef(cls);
}

void ASLAnalyticsTrackScreen(const char* pszScreen)
{
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "AspyrAnalyticsTrackScreen",
                                          "(Ljava/lang/String;)V");
    jstring strScreen = env->NewStringUTF(pszScreen);
    env->CallStaticVoidMethod(cls, mid, strScreen);
    env->DeleteLocalRef(strScreen);
    env->DeleteLocalRef(cls);
}

void SubmitScore(const char* pszLeaderboard, int64_t nScore)
{
    JNIEnv* env = Android_JNI_GetEnv();
#if KOTOR1
    jclass cls = env->FindClass("com/aspyr/kotor/KOTOR");
#else
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
#endif
    jmethodID mid = env->GetStaticMethodID(cls, "submitScore", "(Ljava/lang/String;J)V");
    jstring strLeaderboard = env->NewStringUTF(pszLeaderboard);
    env->CallStaticVoidMethod(cls, mid, strLeaderboard, nScore);
    env->DeleteLocalRef(strLeaderboard);
    env->DeleteLocalRef(cls);
}

#ifdef K2_HOST_BUILD
extern "C" void k2host_unlock_achievement(const char* pszTag);   // platforms/shim/shim_achievements.cpp
#endif

bool Achievements::UnlockAchievement(const char* pTag)
{
#ifdef K2_HOST_BUILD
    // Steam, else the host's own record and notice: there is no Java side to hand it to.
    k2host_unlock_achievement(pTag);
#else
    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "unlockAchievement", "(Ljava/lang/String;)V");
    jstring strTag = env->NewStringUTF(pTag);
    env->CallStaticVoidMethod(cls, mid, strTag);
    env->DeleteLocalRef(strTag);
    env->DeleteLocalRef(cls);
#endif
    return true;
}


void OnAndroidQuit()
{
    SDL_LockMutex(g_UI_mutex);
    {
        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/base/ASPYR");
        jmethodID mid = env->GetStaticMethodID(cls, "showQuitPopup", "()V");
        env->CallStaticVoidMethod(cls, mid);
        env->DeleteLocalRef(cls);
    }
    SDL_CondWait(g_UI_cond, g_UI_mutex);
    if (g_done)
        PostQuitMessage(0);
    SDL_UnlockMutex(g_UI_mutex);
}

bool LoginGooglePlay()
{
    SDL_LockMutex(g_UI_mutex);
    {
        JNIEnv* env = Android_JNI_GetEnv();
        jclass cls = env->FindClass("com/aspyr/base/ASPYR");
        jmethodID mid = env->GetStaticMethodID(cls, "openGameServiceFragment", "()V");
        env->CallStaticVoidMethod(cls, mid);
        env->DeleteLocalRef(cls);
    }
    SDL_CondWait(g_UI_cond, g_UI_mutex);
    SDL_UnlockMutex(g_UI_mutex);
    return g_GooglePlay;
}

#if !KOTOR1  // K1 imports it
SDL_DisplayMode g_SDL_mode;
#endif


// SDL is not carried as a header here, so the enumerators are spelled locally, like
// AndroidLogPriority in AndroidJNI.h.
enum SdlGlAttribute {
    SDL_GL_RED_SIZE   = 0,
    SDL_GL_GREEN_SIZE = 1,
    SDL_GL_BLUE_SIZE  = 2,
    SDL_GL_DEPTH_SIZE = 6,
};

extern int GAME_OPTION_DEFAULT_SHADOWS;
extern int GAME_OPTION_DEFAULT_GRASS;
extern int GAME_OPTION_DEFAULT_FRAMEBUFFER;
extern int GAME_OPTION_DEFAULT_SCREENRES;
extern int GAME_OPTION_DEFAULT_HIDEINGAMEGUI;
extern int SWGUI_NUM_FEEDBACK_OPTIONS;
volatile int resolutionCap = -1;

void SetupAndroidHardwareScaler()
{
    JNIEnv* env = Android_JNI_GetEnv();

    Manufacturer("print out");
    Model("print out");
    Brand("print out");
    Display("print out");
    Device("print out");

    if (Manufacturer("NVIDIA") && Model("SHIELD")) {
        GAME_OPTION_DEFAULT_SHADOWS = 1;
        GAME_OPTION_DEFAULT_GRASS = 1;
        GAME_OPTION_DEFAULT_FRAMEBUFFER = 1;
        GAME_OPTION_DEFAULT_SCREENRES = 2;
    }
    GAME_OPTION_DEFAULT_SHADOWS = 1;
    GAME_OPTION_DEFAULT_GRASS = 1;

    jclass cls = env->FindClass("org/libsdl/app/SDLActivity");
    jmethodID mid = env->GetStaticMethodID(cls, "getContext", "()Landroid/content/Context;");
    jobject context = env->CallStaticObjectMethod(cls, mid);
    env->DeleteLocalRef(cls);

    cls = env->FindClass("android/app/Activity");
    mid = env->GetMethodID(cls, "getWindowManager", "()Landroid/view/WindowManager;");
    env->DeleteLocalRef(cls);
    jobject windowManager = env->CallObjectMethod(context, mid);
    env->DeleteLocalRef(context);

    cls = env->FindClass("android/view/WindowManager");
    mid = env->GetMethodID(cls, "getDefaultDisplay", "()Landroid/view/Display;");
    env->DeleteLocalRef(cls);
    jobject display = env->CallObjectMethod(windowManager, mid);
    env->DeleteLocalRef(windowManager);

    cls = env->FindClass("android/util/DisplayMetrics");
    mid = env->GetMethodID(cls, "<init>", "()V");
    jobject metrics = env->NewObject(cls, mid);

    // The DisplayMetrics class must stay alive across both GetFieldID calls below, so the Display
    // class cannot reuse `cls`.
    jclass clsDisplay = env->FindClass("android/view/Display");
    jmethodID midGetMetrics =
        env->GetMethodID(clsDisplay, "getMetrics", "(Landroid/util/DisplayMetrics;)V");
    env->DeleteLocalRef(clsDisplay);
    env->CallVoidMethod(display, midGetMetrics, metrics);
    env->DeleteLocalRef(display);

    jfieldID fidXdpi = env->GetFieldID(cls, "xdpi", "F");
    jfieldID fidYdpi = env->GetFieldID(cls, "ydpi", "F");
    env->DeleteLocalRef(cls);
    env->GetFloatField(metrics, fidXdpi);  // read and discarded
    const float fYdpi = env->GetFloatField(metrics, fidYdpi);
    env->DeleteLocalRef(metrics);

    const bool bHighRes = GetHighResolution();
    const int nPixelHeight = GetScreenHeightPixel();
    const float fScreenHeightInch = GetScreenHeightInch();

    if (!HasTouchScreen()) {
        GAME_OPTION_DEFAULT_HIDEINGAMEGUI = 1;
        SWGUI_NUM_FEEDBACK_OPTIONS = 6;
    }

    __android_log_print(ANDROID_LOG_INFO, "ASL_DEBUG", "screen height inch: %f ydpi: %f",
                        fScreenHeightInch, fYdpi);

    if (bHighRes) {
        resolutionCap = nPixelHeight;
    } else {
        resolutionCap = fScreenHeightInch < 3.0f ? 640 : 720;
        if (resolutionCap >= nPixelHeight)
            resolutionCap = 480;
    }

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    if (SDL_GetDisplayMode(0, 0, &g_SDL_mode) != 0) {
        __android_log_print(ANDROID_LOG_INFO, "ASL_DEBUG", "SDL_GetDisplayMode Error: %s",
                            SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    g_SDL_BufferGeometry_w = g_SDL_mode.w;
    g_SDL_BufferGeometry_h = g_SDL_mode.h;
    if (g_SDL_BufferGeometry_h > resolutionCap) {
        g_SDL_BufferGeometry_w = resolutionCap * g_SDL_BufferGeometry_w / g_SDL_BufferGeometry_h;
        g_SDL_BufferGeometry_h = resolutionCap;
        __android_log_print(ANDROID_LOG_INFO, "ASL_DEBUG", "resolution cap! setting %d by %d",
                            g_SDL_BufferGeometry_w, g_SDL_BufferGeometry_h);
    }
    __android_log_print(ANDROID_LOG_INFO, "ASL_DEBUG", "Resolution finalized to %d by %d",
                        g_SDL_BufferGeometry_w, g_SDL_BufferGeometry_h);
}
