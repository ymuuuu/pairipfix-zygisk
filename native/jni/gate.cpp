#include "gate.h"
#include "log.h"

namespace pairipfix {

bool IsCandidatePackage(const char* nicePackage) {
    (void)nicePackage;
    return true;
}

bool HasPairip(JNIEnv* env, jobject appCl) {
    jclass clcl = env->FindClass("java/lang/ClassLoader");
    jmethodID load = env->GetMethodID(clcl, "loadClass",
        "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring n = env->NewStringUTF("com.pairip.VMRunner");
    jobject c = env->CallObjectMethod(appCl, load, n);
    env->DeleteLocalRef(n);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return false; }
    bool present = (c != nullptr);
    if (c) env->DeleteLocalRef(c);
    LOGD("HasPairip -> %d", present);
    return present;
}

} // namespace pairipfix
