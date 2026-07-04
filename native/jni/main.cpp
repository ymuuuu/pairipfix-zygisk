#include <cstdlib>
#include <string>
#include <jni.h>
#include "log.h"
#include "gate.h"
#include "java_hooks.h"
#include "native_shim.h"
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;

static std::string g_self_package;

static void Anchor_onCreate(JNIEnv* env, jclass, jobject application) {
    static bool done = false;
    if (done) return;

    jclass appClass = env->GetObjectClass(application);
    jmethodID getCl = env->GetMethodID(appClass, "getClassLoader",
        "()Ljava/lang/ClassLoader;");
    jobject appCl = env->NewGlobalRef(env->CallObjectMethod(application, getCl));
    if (!pairipfix::HasPairip(env, appCl)) {
        LOGD("not pairip, bail");
        pairipfix::UninstallAnchor(env);
        done = true;
        return;
    }
    pairipfix::InstallJavaHooks(env, appCl, g_self_package.c_str());
    pairipfix::UninstallAnchor(env);
    done = true;
}

class Module : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        const char* nice = env->GetStringUTFChars(args->nice_name, nullptr);
        g_self_package = nice ? nice : "";
        env->ReleaseStringUTFChars(args->nice_name, nice);
        LOGD("postAppSpecialize pid=%d pkg=%s", getpid(), g_self_package.c_str());

        if (!pairipfix::IsCandidatePackage(g_self_package.c_str())) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        pairipfix::InstallNativeShim();

        JNIEnv* env = api->getEnv();
        if (!pairipfix::InitLSPlant(env)) return;
        pairipfix::InstallAnchor(env, (void*)Anchor_onCreate);
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(Module)
