#include "lsplant_init.h"
#include "log.h"
#include <lsplant.hpp>
#include <dobby.h>
#include <xdl.h>

namespace pairipfix {

static void* inline_hooker(void* target, void* hooker) {
    void* backup = nullptr;
    if (DobbyHook(target, reinterpret_cast<dobby_dummy_func_t>(hooker), reinterpret_cast<dobby_dummy_func_t*>(&backup)) == 0) {
        return backup;
    }
    return nullptr;
}

static bool inline_unhooker(void* func) {
    return DobbyDestroy(func) == 0;
}

static void* symbol_resolver(const char* name) {
    static void* art = xdl_open("libart.so", XDL_TRY_FORCE_LOAD);
    if (!art) return nullptr;
    void* r = xdl_sym(art, name, nullptr);
    return r ? r : xdl_dsym(art, name, nullptr);
}

bool InitLSPlant(JNIEnv* env) {
    lsplant::InitInfo info{
        .inline_hooker = inline_hooker,
        .inline_unhooker = inline_unhooker,
        .art_symbol_resolver = symbol_resolver,
    };
    bool ok = lsplant::Init(env, info);
    LOGD("lsplant::Init -> %d", ok);
    return ok;
}

} // namespace pairipfix
