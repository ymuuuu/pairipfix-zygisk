#include "lsplant_init.h"
#include "log.h"
#include <lsplant.hpp>
#include <dobby.h>
#include <xdl.h>
#include <cstdlib>
#include <string>
#include <string_view>

namespace pairipfix {

static std::string rand_ident(size_t n) {
    static const char first[] = "abcdefghijklmnopqrstuvwxyz";
    static const char rest[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string s;
    s.reserve(n);
    s += first[arc4random_uniform(sizeof(first) - 1)];
    for (size_t i = 1; i < n; i++) s += rest[arc4random_uniform(sizeof(rest) - 1)];
    return s;
}

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

static void* symbol_resolver(std::string_view name) {
    static void* art = xdl_open("libart.so", XDL_TRY_FORCE_LOAD);
    if (!art) return nullptr;
    std::string n(name);
    void* r = xdl_sym(art, n.c_str(), nullptr);
    return r ? r : xdl_dsym(art, n.c_str(), nullptr);
}

bool InitLSPlant(JNIEnv* env) {
    // Idempotent: lsplant::Init hooks ART internals and must run once per process.
    // Called from both postAppSpecialize and InstallJavaHooks; cache the result.
    static bool initialized = false;
    static bool result = false;
    if (initialized) return result;
    static const std::string cls = "a." + rand_ident(8);
    static const std::string src = "SourceFile";
    static const std::string fld = rand_ident(4);
    lsplant::InitInfo info{
        .inline_hooker = inline_hooker,
        .inline_unhooker = inline_unhooker,
        .art_symbol_resolver = symbol_resolver,
    };
    info.generated_class_name = cls;
    info.generated_source_name = src;
    info.generated_field_name = fld;
    result = lsplant::Init(env, info);
    initialized = true;
    LOGD("lsplant::Init -> %d", result);
    return result;
}

} // namespace pairipfix
