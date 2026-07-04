#include <string>
#include "log.h"
#include "gate.h"
#include "java_hooks.h"
#include "lsplant_init.h"
#include "native_shim.h"
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;

static std::string g_self_package;

// Runs from Anchor.callback (Instrumentation.callApplicationOnCreate) before the
// app's own Application.onCreate. We must NOT unhook the anchor here: LSPlant docs
// state that invoking the backup after UnHook is undefined behavior, and
// Anchor.callback calls the backup right after this returns. The `done` guard makes
// re-entry cheap; the anchor stays hooked but inert (fires once per process).
static void Anchor_onCreate(JNIEnv* env, jclass, jobject application) {
    static bool done = false;
    if (done) return;
    done = true;

    jclass appClass = env->GetObjectClass(application);
    jmethodID getCl = env->GetMethodID(appClass, "getClassLoader",
        "()Ljava/lang/ClassLoader;");
    jobject appCl = env->NewGlobalRef(env->CallObjectMethod(application, getCl));
    if (!pairipfix::HasPairip(env, appCl)) {
        LOGD("not pairip, bail");
        return;
    }
    pairipfix::InstallJavaHooks(env, appCl, g_self_package.c_str());
}

class Module : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        // JNIEnv is provided at onLoad and stored as `env`; this API has no getEnv().
        const char* nice = env->GetStringUTFChars(args->nice_name, nullptr);
        g_self_package = nice ? nice : "";
        env->ReleaseStringUTFChars(args->nice_name, nice);
        LOGD("postAppSpecialize pid=%d pkg=%s", getpid(), g_self_package.c_str());

        if (!pairipfix::IsCandidatePackage(g_self_package.c_str())) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        pairipfix::InstallNativeShim();

        if (!pairipfix::InitLSPlant(env)) return;
        pairipfix::InstallAnchor(env, (void*)Anchor_onCreate);
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(Module)
