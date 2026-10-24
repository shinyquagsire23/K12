// The Android/SDL-Android symbols the engine imports, answered for a host build.
//
// These are the entry points the engine imports from libSDL2.so (Aspyr's fork of its Android
// backend) and liblog.so. Android_JNI_GetEnv() hands out a stub JNIEnv on every platform, the
// Android build included: every one of its ~230 functions is a harmless no-op returning zero
// (FindClass and the ID lookups return a dummy non-null handle, string getters return ""). Every
// Java-bridge call in AndroidJNI.cpp / MacMain / CSWGuiMainMenu / CSWGuiSaveLoad / AspyrCloud
// therefore does nothing, which on a desktop is the right answer ("show the window", "is a
// controller attached", cloud saves...). Set K2_JNI_TRACE=1 to log the class/method names looked
// up.
#include <stdint.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <jni.h>

namespace {

bool Trace()
{
    static int s = -1;
    if (s < 0) s = getenv("K2_JNI_TRACE") != 0;
    return s != 0;
}

int s_dummy;   // the one "object" every lookup returns
jobject Dummy() { return reinterpret_cast<jobject>(&s_dummy); }

// Generic slots: any arguments, integer/pointer result 0. Float and double results need their own
// slots, which return 0 too.
int64_t NullFn() { return 0; }
float NullFloat() { return 0.0f; }
double NullDouble() { return 0.0; }

jclass FindClass(JNIEnv*, const char* name)
{
    if (Trace()) fprintf(stderr, "k2: JNI FindClass %s\n", name);
    return static_cast<jclass>(Dummy());
}
// Every method id is an interned record of its name, so a Call*Method can answer per method:
// K2_JNI_<method>=<int> sets what the Boolean/Int calls on it return. Default 0, except the
// host-identity answers below.
struct MethodRec { char name[64]; int64_t value; };
jmethodID GetMethodID(JNIEnv*, jclass, const char* name, const char* sig)
{
    if (Trace()) fprintf(stderr, "k2: JNI method %s%s\n", name, sig);
    static MethodRec s_recs[256];
    static int s_n;
    for (int i = 0; i < s_n; ++i)
        if (strcmp(s_recs[i].name, name) == 0) return reinterpret_cast<jmethodID>(&s_recs[i]);
    if (s_n == 256) return reinterpret_cast<jmethodID>(Dummy());
    MethodRec* r = &s_recs[s_n++];
    snprintf(r->name, sizeof r->name, "%s", name);
    char key[96];
    snprintf(key, sizeof key, "K2_JNI_%s", name);
    const char* v = getenv(key);
    // HasTouchScreen: the Android build always has one, and the main interface shows the touch HUD
    // (top-right help/options/camera/quicksave, bottom-right party/stealth/pause/talk) only when it
    // says so. Only the touch UI build answers 1; pc and xbox answer 0, as a desktop or a console
    // does, so those widgets stay hidden (Esc and the keyboard/pad reach the same screens).
    // K2_JNI_HasTouchScreen=1 brings them back for a pc run.
#ifdef K2_UI_TOUCH
    const int64_t nTouchScreen = 1;
#else
    const int64_t nTouchScreen = 0;
#endif
    int64_t nDefault = strcmp(name, "HasTouchScreen") == 0 ? nTouchScreen : 0;
    r->value = v ? strtol(v, 0, 0) : nDefault;
    return reinterpret_cast<jmethodID>(r);
}
bool IsRec(jmethodID m) { return static_cast<void*>(m) != static_cast<void*>(&s_dummy); }
int64_t RecValue(jmethodID m) { return m && IsRec(m) ? reinterpret_cast<MethodRec*>(m)->value : 0; }
// The method id is the third argument on every Call*Method form (v, A and varargs alike)
jboolean CallBool(JNIEnv*, void*, jmethodID m, ...) { return RecValue(m) != 0; }
jint CallInt(JNIEnv*, void*, jmethodID m, ...) { return static_cast<jint>(RecValue(m)); }
// Fields the same way: K2_JNI_<field>=<number> is what Get(Static)FloatField answers for it
// (KOTOR 1's SDL_main reads DisplayMetrics.xdpi / ydpi for its tap tolerances); default 0.
struct FieldRec { char name[64]; float value; };
jfieldID GetFieldID(JNIEnv*, jclass, const char* name, const char*)
{
    if (Trace()) fprintf(stderr, "k2: JNI field %s\n", name);
    static FieldRec s_recs[64];
    static int s_n;
    for (int i = 0; i < s_n; ++i)
        if (strcmp(s_recs[i].name, name) == 0) return reinterpret_cast<jfieldID>(&s_recs[i]);
    if (s_n == 64) return reinterpret_cast<jfieldID>(Dummy());
    FieldRec* r = &s_recs[s_n++];
    snprintf(r->name, sizeof r->name, "%s", name);
    char key[96];
    snprintf(key, sizeof key, "K2_JNI_%s", name);
    const char* v = getenv(key);
    r->value = v ? strtof(v, 0) : 0.0f;
    return reinterpret_cast<jfieldID>(r);
}
float GetFloatField(JNIEnv*, void*, jfieldID f)
{
    return f && static_cast<void*>(f) != static_cast<void*>(&s_dummy) ? reinterpret_cast<FieldRec*>(f)->value : 0.0f;
}
jobject ReturnDummy() { return Dummy(); }
const char* EmptyUTF(JNIEnv*, jstring, jboolean* isCopy)
{
    if (isCopy) *isCopy = 0;
    return "";
}

JNINativeInterface* MakeTable()
{
    static JNINativeInterface t;
    void** slots = reinterpret_cast<void**>(&t);  // raw-offset: the JNI function table filled slot by slot
    for (size_t i = 0; i < sizeof t / sizeof(void*); ++i) slots[i] = reinterpret_cast<void*>(NullFn);
    t.FindClass = FindClass;
    t.GetMethodID = GetMethodID;
    t.GetStaticMethodID = GetMethodID;
    t.GetFieldID = GetFieldID;
    t.GetStaticFieldID = GetFieldID;
    t.GetObjectClass = reinterpret_cast<jclass (*)(JNIEnv*, jobject)>(ReturnDummy);
    t.NewStringUTF = reinterpret_cast<jstring (*)(JNIEnv*, const char*)>(ReturnDummy);
    t.NewGlobalRef = reinterpret_cast<jobject (*)(JNIEnv*, jobject)>(ReturnDummy);
    t.GetStringUTFChars = EmptyUTF;
#define B(name) *reinterpret_cast<void**>(&t.name) = reinterpret_cast<void*>(CallBool);
#define I(name) *reinterpret_cast<void**>(&t.name) = reinterpret_cast<void*>(CallInt);
    B(CallBooleanMethod) B(CallBooleanMethodV) B(CallBooleanMethodA) B(CallStaticBooleanMethod)
    B(CallStaticBooleanMethodV) B(CallStaticBooleanMethodA)
    I(CallIntMethod) I(CallIntMethodV) I(CallIntMethodA) I(CallStaticIntMethod)
    I(CallStaticIntMethodV) I(CallStaticIntMethodA)
#undef B
#undef I
#define F(name) *reinterpret_cast<void**>(&t.name) = reinterpret_cast<void*>(NullFloat);
#define D(name) *reinterpret_cast<void**>(&t.name) = reinterpret_cast<void*>(NullDouble);
    F(CallFloatMethod) F(CallFloatMethodV) F(CallFloatMethodA) F(CallNonvirtualFloatMethod)
    F(CallNonvirtualFloatMethodV) F(CallNonvirtualFloatMethodA) F(CallStaticFloatMethod)
    F(CallStaticFloatMethodV) F(CallStaticFloatMethodA)
    D(CallDoubleMethod) D(CallDoubleMethodV) D(CallDoubleMethodA) D(CallNonvirtualDoubleMethod)
    D(CallNonvirtualDoubleMethodV) D(CallNonvirtualDoubleMethodA) D(CallStaticDoubleMethod)
    D(CallStaticDoubleMethodV) D(CallStaticDoubleMethodA) D(GetDoubleField) D(GetStaticDoubleField)
#undef F
#undef D
    *reinterpret_cast<void**>(&t.GetFloatField) = reinterpret_cast<void*>(GetFloatField);
    *reinterpret_cast<void**>(&t.GetStaticFloatField) = reinterpret_cast<void*>(GetFloatField);
    return &t;
}

} // namespace

extern "C" JNIEnv* Android_JNI_GetEnv()
{
    static JNIEnv s_env = { MakeTable() };
    return &s_env;
}

#ifndef __ANDROID__
const char* k2shim_user_dir();

extern "C" const char* SDL_AndroidGetExternalStoragePath()
{
    // The OBB/save root on Android. Default to the working directory; K2_DATA_DIR overrides. With a
    // user directory the working directory is used too: the engine's paths under it stay relative,
    // which is what the shim's user layer reads and writes through.
    const char* p = getenv("K2_DATA_DIR");
    return p && !*k2shim_user_dir() ? p : ".";
}

#endif

// Aspyr's SDL-Android publishes the GL surface size here (stock SDL2 has no such globals); the engine
// reads it for its aspect decisions. The platform layer overwrites both once the window exists.
int g_SDL_BufferGeometry_w = 1280;
int g_SDL_BufferGeometry_h = 720;

#ifndef __ANDROID__
extern "C" int __android_log_write(int nPrio, const char* pszTag, const char* pszText)
{
    return fprintf(stderr, "[%d %s] %s\n", nPrio, pszTag ? pszTag : "", pszText ? pszText : "");
}

extern "C" int __android_log_print(int nPrio, const char* pszTag, const char* pszFmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, pszFmt);
    vsnprintf(buf, sizeof buf, pszFmt, ap);
    va_end(ap);
    return __android_log_write(nPrio, pszTag, buf);
}
#endif
