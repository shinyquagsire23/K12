#include "AndroidJNI.h"
#include "port/sdlapi.h"

#include <algorithm>
#include <string>

void SetCloudSynchStatus(CloudSynchStatus status);

// The status is bumped to 1, in flight, before the Java call; the native* cloud callbacks advance
// the same enum to 2 and 3, so this is the producer of that handshake.
void SynchCloudSave()
{
    SetCloudSynchStatus(CloudSynchStatus_Synchronizing);

    JNIEnv* env = Android_JNI_GetEnv();
    jclass cls = env->FindClass("com/aspyr/base/ASPYR");
    jmethodID mid = env->GetStaticMethodID(cls, "SynchCloudSave", "()V");
    env->CallStaticVoidMethod(cls, mid);
    env->DeleteLocalRef(cls);
}


// Writes several files with the same path idiom as the single-file writer below, wrapped in a loop
// over the files, with each path appended to one semicolon-separated list that is what reaches
// Java.
void IosCloudWriteFiles(const char** ppszFiles, int nCount, const char* pszData, unsigned int nLen)
{
    std::string strAll;

    for (int i = 0; i < nCount; i++)
    {
        std::string strPath(ppszFiles[i]);
        std::replace(strPath.begin(), strPath.end(), '\\', '/');

        std::string strFull =
            (std::string(SDL_AndroidGetExternalStoragePath()) + "/" + strPath).c_str();

        strAll.append(strFull);
        strAll.append(";", 1);
    }

    JNIEnv* env;
    jclass cls;
    jmethodID mid;
    jstring jstrAll;
    const char* pszAll;

    pszAll = strAll.c_str();
    env = Android_JNI_GetEnv();
    cls = env->FindClass("com/aspyr/base/ASPYR");
    mid = env->GetStaticMethodID(cls, "WriteMultiFileCloudSave",
                                 "(Ljava/lang/String;Ljava/lang/String;J)V");
    jstrAll = env->NewStringUTF(pszAll);
    env->CallStaticVoidMethod(cls, mid, jstrAll, env->NewStringUTF(pszData), static_cast<jlong>(nLen));
    env->DeleteLocalRef(jstrAll);
    env->DeleteLocalRef(cls);
}

// The cloud layer's writer. It normalises the path, glues it onto the external-storage root, and
// hands it to Java along with the payload and its length.
void IosCloudWrite(const char* pszPath, const char* pszData, unsigned int nLen)
{
    std::string strPath(pszPath);
    std::replace(strPath.begin(), strPath.end(), '\\', '/');

    std::string strFull =
        (std::string(SDL_AndroidGetExternalStoragePath()) + "/" + strPath).c_str();
    JNIEnv* env;
    jclass cls;
    jmethodID mid;
    jstring jstrPath;
    const char* pszFull;

    pszFull = strFull.c_str();
    env = Android_JNI_GetEnv();
    cls = env->FindClass("com/aspyr/base/ASPYR");
    mid = env->GetStaticMethodID(cls, "WriteCloudSave", "(Ljava/lang/String;Ljava/lang/String;J)V");
    jstrPath = env->NewStringUTF(pszFull);
    env->CallStaticVoidMethod(cls, mid, jstrPath, env->NewStringUTF(pszData), static_cast<jlong>(nLen));
    env->DeleteLocalRef(jstrPath);
    env->DeleteLocalRef(cls);
}

// The rest of the family does nothing on Android. The file is named for iOS and ships in the
// Android library, where every body is an empty shim or a read of the Google-Play flag. The real
// work happens in Java.
bool IsAmazon();

extern volatile bool g_GooglePlay;

void IosCloudWrite(const char* pszPath, const char* pszData, unsigned int nLen);
void IosCloudInit(const char* pszRoot, bool bSomething)
{
    K2_UNUSED(pszRoot); K2_UNUSED(bSomething);
}

bool IosCloudChanged()
{
    return false;
}

bool IosCloudAvailable()
{
    if (IsAmazon())
        return false;
    return g_GooglePlay;
}

void IosCloudDelete()
{
}

bool IosCloudIsAvailable()
{
    if (IsAmazon())
        return false;
    return g_GooglePlay;
}

void IosCloudWrite(const char* pszPath)
{
    IosCloudWrite(pszPath, 0, 0);
}
