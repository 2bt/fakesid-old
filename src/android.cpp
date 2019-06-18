#include <string>

#ifdef __ANDROID__
#include <SDL.h>
#include <sys/stat.h>
#include <jni.h>

namespace android {

std::string get_storage_dir() {

    JNIEnv* j_env = (JNIEnv*) SDL_AndroidGetJNIEnv();

    jclass j_env_class = j_env->FindClass("android/os/Environment");
    jmethodID j_mid = j_env->GetStaticMethodID(j_env_class, "getExternalStorageDirectory",  "()Ljava/io/File;");
    jobject j_file = j_env->CallStaticObjectMethod(j_env_class, j_mid);
    if (!j_file) return "";

    j_mid = j_env->GetMethodID(j_env->GetObjectClass(j_file), "mkdirs", "()Z");
    j_env->CallBooleanMethod(j_file, j_mid);

    j_mid = j_env->GetMethodID(j_env->GetObjectClass(j_file), "getAbsolutePath", "()Ljava/lang/String;");
    jstring j_path = (jstring) j_env->CallObjectMethod(j_file, j_mid);
    const char* str = j_env->GetStringUTFChars(j_path, nullptr);
    std::string root_dir = str;
    j_env->ReleaseStringUTFChars(j_path, str);

    return root_dir + "/fakesid";
}

} // namespace

#else

namespace android {
    std::string get_storage_dir() { return "."; }
} // namespace

#endif