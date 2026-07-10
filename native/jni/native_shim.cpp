#include "native_shim.h"
#include "log.h"
#include <dobby.h>
#include <dlfcn.h>
#include <sys/ptrace.h>
#include <sys/types.h>

namespace pairipfix {

using ptrace_t = long(*)(int, pid_t, void*, void*);
static ptrace_t o_ptrace = nullptr;
static long my_ptrace(int req, pid_t pid, void* addr, void* data) {
    if (req == PTRACE_TRACEME) return 0;
    return o_ptrace ? o_ptrace(req, pid, addr, data) : 0;
}

static void hook(const char* sym, void* replace, void** backup) {
    void* p = dlsym(RTLD_DEFAULT, sym);
    if (!p) { LOGD("no sym %s", sym); return; }
    if (DobbyHook(p, reinterpret_cast<dobby_dummy_func_t>(replace),
                  reinterpret_cast<dobby_dummy_func_t*>(backup)) == 0) {
        LOGD("shim hooked %s", sym);
    } else {
        LOGD("shim FAILED %s", sym);
    }
}

void InstallNativeShim() {
    static bool done = false;
    if (done) return;
    done = true;
    hook("ptrace", (void*)my_ptrace, (void**)&o_ptrace);
    LOGD("native shim installed");
}

} // namespace pairipfix
