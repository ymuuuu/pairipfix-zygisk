#include "java_hooks.h"
#include "helper_dex.h"
#include "lsplant_init.h"
#include "log.h"
#include <lsplant.hpp>
#include <string>

namespace pairipfix {

static jobject LoadHelper(JNIEnv* env, jobject appCl) {
    jobject buf = env->NewDirectByteBuffer((void*)kHelperDex, (jlong)kHelperDexLen);
    jclass cl = env->FindClass("dalvik/system/InMemoryDexClassLoader");
    jmethodID ctor = env->GetMethodID(cl, "<init>",
        "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    jobject helperCl = env->NewObject(cl, ctor, buf, appCl);
    return env->NewGlobalRef(helperCl);
}

static jclass HelperClass(JNIEnv* env, jobject helperCl, const char* dotName) {
    jclass clcl = env->FindClass("java/lang/ClassLoader");
    jmethodID load = env->GetMethodID(clcl, "loadClass",
        "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring n = env->NewStringUTF(dotName);
    jclass c = (jclass)env->CallObjectMethod(helperCl, load, n);
    env->DeleteLocalRef(n);
    return c;
}

static jobject ResolveMethod(JNIEnv* env, jobject helperCl, jclass resolver,
                             jobject appCl, const char* cls, const char* name,
                             jobjectArray paramTypes) {
    jmethodID m = env->GetStaticMethodID(resolver, "method",
        "(Ljava/lang/ClassLoader;Ljava/lang/String;Ljava/lang/String;"
        "[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
    jstring jc = env->NewStringUTF(cls);
    jstring jn = env->NewStringUTF(name);
    jobject res = env->CallStaticObjectMethod(resolver, m, appCl, jc, jn, paramTypes);
    env->DeleteLocalRef(jc); env->DeleteLocalRef(jn);
    return res;
}

static jobject NewHooker(JNIEnv* env, jclass hookerCls, jint mode,
                         jobject constant, const char* selfPackage) {
    jmethodID ctor = env->GetMethodID(hookerCls, "<init>", "()V");
    jobject h = env->NewObject(hookerCls, ctor);
    env->SetIntField(h, env->GetFieldID(hookerCls, "mode", "I"), mode);
    if (constant)
        env->SetObjectField(h, env->GetFieldID(hookerCls, "constant", "Ljava/lang/Object;"), constant);
    if (selfPackage)
        env->SetObjectField(h, env->GetFieldID(hookerCls, "selfPackage", "Ljava/lang/String;"),
                            env->NewStringUTF(selfPackage));
    return h;
}

static bool HookOne(JNIEnv* env, jobject helperCl, jclass resolver, jclass hookerCls,
                    jobject appCl, const char* cls, const char* name,
                    jobjectArray paramTypes, jint mode, jobject constant,
                    const char* selfPackage) {
    jobject target = ResolveMethod(env, helperCl, resolver, appCl, cls, name, paramTypes);
    if (!target) { LOGD("resolve fail %s.%s", cls, name); return false; }
    jobject hooker = NewHooker(env, hookerCls, mode, constant, selfPackage);
    jmethodID cbId = env->GetMethodID(hookerCls, "callback",
        "([Ljava/lang/Object;)Ljava/lang/Object;");
    jobject cbMethod = env->ToReflectedMethod(hookerCls, cbId, JNI_FALSE);
    jobject backup = lsplant::Hook(env, target, hooker, cbMethod);
    if (!backup) { LOGD("Hook fail %s.%s", cls, name); return false; }
    env->SetObjectField(hooker, env->GetFieldID(hookerCls, "backup",
        "Ljava/lang/reflect/Method;"), backup);
    env->NewGlobalRef(hooker);
    LOGD("hooked %s.%s", cls, name);
    return true;
}

static jobjectArray Params0(JNIEnv* env) {
    return env->NewObjectArray(0, env->FindClass("java/lang/Class"), nullptr);
}

static jobjectArray Params1(JNIEnv* env, jclass resolver, jobject appCl, const char* t) {
    jmethodID tm = env->GetStaticMethodID(resolver, "type",
        "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;");
    jstring s = env->NewStringUTF(t);
    jclass c = (jclass)env->CallStaticObjectMethod(resolver, tm, appCl, s);
    jobjectArray a = env->NewObjectArray(1, env->FindClass("java/lang/Class"), c);
    env->DeleteLocalRef(s);
    return a;
}

static jobjectArray Params2(JNIEnv* env, jclass resolver, jobject appCl,
                            const char* t0, const char* t1) {
    jmethodID tm = env->GetStaticMethodID(resolver, "type",
        "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;");
    jclass classCls = env->FindClass("java/lang/Class");
    jobjectArray a = env->NewObjectArray(2, classCls, nullptr);
    const char* ts[2] = {t0, t1};
    for (int i = 0; i < 2; i++) {
        jstring s = env->NewStringUTF(ts[i]);
        env->SetObjectArrayElement(a, i,
            env->CallStaticObjectMethod(resolver, tm, appCl, s));
        env->DeleteLocalRef(s);
    }
    return a;
}

static jobjectArray Params4(JNIEnv* env, jclass resolver, jobject appCl,
                            const char* t0, const char* t1,
                            const char* t2, const char* t3) {
    jmethodID tm = env->GetStaticMethodID(resolver, "type",
        "(Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/Class;");
    jclass classCls = env->FindClass("java/lang/Class");
    jobjectArray a = env->NewObjectArray(4, classCls, nullptr);
    const char* ts[4] = {t0, t1, t2, t3};
    for (int i = 0; i < 4; i++) {
        jstring s = env->NewStringUTF(ts[i]);
        env->SetObjectArrayElement(a, i,
            env->CallStaticObjectMethod(resolver, tm, appCl, s));
        env->DeleteLocalRef(s);
    }
    return a;
}

static bool ClassExists(JNIEnv* env, jclass resolver, jobject appCl, const char* name) {
    jmethodID m = env->GetStaticMethodID(resolver, "classExists",
        "(Ljava/lang/ClassLoader;Ljava/lang/String;)Z");
    jstring s = env->NewStringUTF(name);
    jboolean r = env->CallStaticBooleanMethod(resolver, m, appCl, s);
    env->DeleteLocalRef(s);
    return r;
}

static void SetStateOk(JNIEnv* env, jclass resolver, jobject appCl) {
    jmethodID m = env->GetStaticMethodID(resolver, "setStaticStateOk",
        "(Ljava/lang/ClassLoader;)Z");
    jboolean r = env->CallStaticBooleanMethod(resolver, m, appCl);
    LOGD("setStaticStateOk -> %d", r);
}

// Anchor state for unhooking
static jobject g_anchor_target = nullptr;
static jobject g_anchor_obj = nullptr;

void InstallAnchor(JNIEnv* env, void* onCreateFnPtr) {
    jclass clCl = env->FindClass("java/lang/ClassLoader");
    jmethodID getSys = env->GetStaticMethodID(clCl, "getSystemClassLoader",
        "()Ljava/lang/ClassLoader;");
    jobject sysCl = env->CallStaticObjectMethod(clCl, getSys);

    jobject helperCl = LoadHelper(env, sysCl);
    jclass anchorCls = HelperClass(env, helperCl, "io.github.pairipfix.helper.Anchor");

    JNINativeMethod nativeMethod = { "onCreate", "(Ljava/lang/Object;)V", onCreateFnPtr };
    env->RegisterNatives(anchorCls, &nativeMethod, 1);

    jclass instr = env->FindClass("android/app/Instrumentation");
    jmethodID mid = env->GetMethodID(instr, "callApplicationOnCreate",
        "(Landroid/app/Application;)V");
    jobject target = env->ToReflectedMethod(instr, mid, JNI_FALSE);
    g_anchor_target = env->NewGlobalRef(target);

    jmethodID ctor = env->GetMethodID(anchorCls, "<init>", "()V");
    g_anchor_obj = env->NewGlobalRef(env->NewObject(anchorCls, ctor));

    jmethodID cbId = env->GetMethodID(anchorCls, "callback",
        "([Ljava/lang/Object;)Ljava/lang/Object;");
    jobject cbMethod = env->ToReflectedMethod(anchorCls, cbId, JNI_FALSE);

    jobject backup = lsplant::Hook(env, target, g_anchor_obj, cbMethod);
    env->SetObjectField(g_anchor_obj, env->GetFieldID(anchorCls, "backup",
        "Ljava/lang/reflect/Method;"), backup);
    LOGD("anchor installed");
}

void UninstallAnchor(JNIEnv* env) {
    if (g_anchor_target) {
        lsplant::UnHook(env, g_anchor_target);
        env->DeleteGlobalRef(g_anchor_target);
        g_anchor_target = nullptr;
    }
    if (g_anchor_obj) {
        env->DeleteGlobalRef(g_anchor_obj);
        g_anchor_obj = nullptr;
    }
}

void InstallJavaHooks(JNIEnv* env, jobject appCl, const char* selfPackage) {
    if (!InitLSPlant(env)) { LOGD("lsplant init failed"); return; }
    jobject helperCl = LoadHelper(env, appCl);
    jclass resolver = HelperClass(env, helperCl, "io.github.pairipfix.helper.Resolver");
    jclass hookerCls = HelperClass(env, helperCl, "io.github.pairipfix.helper.Hooker");

    jobject TRUE_ = env->NewGlobalRef(env->GetStaticObjectField(
        env->FindClass("java/lang/Boolean"),
        env->GetStaticFieldID(env->FindClass("java/lang/Boolean"), "TRUE", "Ljava/lang/Boolean;")));

    const int DO_NOTHING = 0, RETURN_CONST = 1, INSTALLER_SPOOF = 2;

    HookOne(env, helperCl, resolver, hookerCls, appCl,
            "com.pairip.SignatureCheck", "verifyIntegrity",
            Params1(env, resolver, appCl, "android.content.Context"),
            DO_NOTHING, nullptr, selfPackage);
    HookOne(env, helperCl, resolver, hookerCls, appCl,
            "com.pairip.SignatureCheck", "verifySignatureMatches",
            Params1(env, resolver, appCl, "java.lang.String"),
            RETURN_CONST, TRUE_, selfPackage);

    HookOne(env, helperCl, resolver, hookerCls, appCl,
            "com.pairip.licensecheck.LicenseClient", "initializeLicenseCheck",
            Params0(env), DO_NOTHING, nullptr, selfPackage);
    HookOne(env, helperCl, resolver, hookerCls, appCl,
            "com.pairip.licensecheck.LicenseClient", "performLocalInstallerCheck",
            Params0(env), RETURN_CONST, TRUE_, selfPackage);
    SetStateOk(env, resolver, appCl);

    if (ClassExists(env, resolver, appCl, "com.pairip.licensecheck3.LicenseClientV3")) {
        HookOne(env, helperCl, resolver, hookerCls, appCl,
            "com.pairip.licensecheck3.LicenseClientV3", "processResponse",
            Params2(env, resolver, appCl, "int", "android.os.Bundle"),
            DO_NOTHING, nullptr, selfPackage);
    }

    const char* LA = "com.pairip.licensecheck.LicenseActivity";
    for (const char* mth : {"closeApp","exitApp","showErrorDialog","showPaywallAndCloseApp"}) {
        HookOne(env, helperCl, resolver, hookerCls, appCl, LA, mth,
                Params0(env), DO_NOTHING, nullptr, selfPackage);
    }
    HookOne(env, helperCl, resolver, hookerCls, appCl, LA, "logAndShowErrorDialog",
            Params1(env, resolver, appCl, "java.lang.String"),
            DO_NOTHING, nullptr, selfPackage);
    HookOne(env, helperCl, resolver, hookerCls, appCl, LA, "logAndShowErrorDialog",
            Params2(env, resolver, appCl, "java.lang.String", "java.lang.Exception"),
            DO_NOTHING, nullptr, selfPackage);

    const char* LRH = "com.pairip.licensecheck.LicenseResponseHelper";
    HookOne(env, helperCl, resolver, hookerCls, appCl, LRH, "validateResponse",
            Params2(env, resolver, appCl, "android.os.Bundle", "java.lang.String"),
            DO_NOTHING, nullptr, selfPackage);
    HookOne(env, helperCl, resolver, hookerCls, appCl, LRH, "getRepeatedCheckMetadata",
            Params1(env, resolver, appCl, "android.os.Bundle"),
            RETURN_CONST, nullptr, selfPackage);
    HookOne(env, helperCl, resolver, hookerCls, appCl, LRH, "verifySignature",
            Params4(env, resolver, appCl, "java.lang.String","java.lang.String",
                    "java.lang.String","java.security.PublicKey"),
            DO_NOTHING, nullptr, selfPackage);

    if (ClassExists(env, resolver, appCl, "com.pairip.licensecheck.ResponseValidator")) {
        HookOne(env, helperCl, resolver, hookerCls, appCl,
            "com.pairip.licensecheck.ResponseValidator", "validateResponse",
            Params2(env, resolver, appCl, "android.os.Bundle", "java.lang.String"),
            DO_NOTHING, nullptr, selfPackage);
        HookOne(env, helperCl, resolver, hookerCls, appCl,
            "com.pairip.licensecheck.ResponseValidator", "verifySignature",
            Params4(env, resolver, appCl, "java.lang.String","java.lang.String",
                    "java.lang.String","java.security.PublicKey"),
            DO_NOTHING, nullptr, selfPackage);
    }
    if (ClassExists(env, resolver, appCl, "com.pairip.licensecheck3.ResponseValidator")) {
        HookOne(env, helperCl, resolver, hookerCls, appCl,
            "com.pairip.licensecheck3.ResponseValidator", "validateResponse",
            Params2(env, resolver, appCl, "android.os.Bundle", "java.lang.String"),
            DO_NOTHING, nullptr, selfPackage);
    }

    HookOne(env, helperCl, resolver, hookerCls, appCl,
            "android.app.ApplicationPackageManager", "getInstallerPackageName",
            Params1(env, resolver, appCl, "java.lang.String"),
            INSTALLER_SPOOF, nullptr, selfPackage);

    LOGD("Engine A installed");
}

} // namespace pairipfix
