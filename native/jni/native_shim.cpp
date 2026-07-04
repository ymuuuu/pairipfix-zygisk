#include "native_shim.h"
#include "scrub.h"
#include "log.h"
#include <dobby.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <set>
#include <mutex>

namespace pairipfix {

// /proc fds we are scrubbing, shared across the openat/read/close hooks which run
// on arbitrary app threads. Guarded because std::set is not thread-safe.
static std::set<int>& scrub_fds() { static std::set<int> s; return s; }
static std::mutex& scrub_mtx() { static std::mutex m; return m; }

using ptrace_t = long(*)(int, pid_t, void*, void*);
static ptrace_t o_ptrace = nullptr;
static long my_ptrace(int req, pid_t pid, void* addr, void* data) {
    if (req == PTRACE_TRACEME) return 0;
    return o_ptrace ? o_ptrace(req, pid, addr, data) : 0;
}

static bool sensitive_path(const char* p) {
    if (!p) return false;
    return std::strstr(p, "/proc/self/maps") || std::strstr(p, "/proc/self/status") ||
           std::strstr(p, "/proc/self/task")  || std::strstr(p, "/proc/self/mounts") ||
           std::strstr(p, "/proc/net/tcp");
}

using openat_t = int(*)(int, const char*, int, ...);
static openat_t o_openat = nullptr;
static int my_openat(int dirfd, const char* path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = static_cast<mode_t>(va_arg(ap, int));
        va_end(ap);
    }
    int fd = o_openat(dirfd, path, flags, mode);
    if (fd >= 0 && sensitive_path(path)) {
        std::lock_guard<std::mutex> lk(scrub_mtx());
        scrub_fds().insert(fd);
    }
    return fd;
}

using read_t = ssize_t(*)(int, void*, size_t);
static read_t o_read = nullptr;
static ssize_t my_read(int fd, void* buf, size_t count) {
    ssize_t n = o_read(fd, buf, count);
    bool scrub = false;
    if (n > 0) {
        std::lock_guard<std::mutex> lk(scrub_mtx());
        scrub = scrub_fds().count(fd) != 0;
    }
    if (scrub) {
        char* tmp = (char*)std::malloc(static_cast<size_t>(n));
        if (tmp) {
            size_t w = scrub_lines(static_cast<const char*>(buf), static_cast<size_t>(n), tmp, static_cast<size_t>(n));
            std::memcpy(buf, tmp, w);
            std::free(tmp);
            return static_cast<ssize_t>(w);
        }
    }
    return n;
}

using close_t = int(*)(int);
static close_t o_close = nullptr;
static int my_close(int fd) {
    {
        std::lock_guard<std::mutex> lk(scrub_mtx());
        scrub_fds().erase(fd);
    }
    return o_close(fd);
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
    hook("openat", (void*)my_openat, (void**)&o_openat);
    hook("read",   (void*)my_read,   (void**)&o_read);
    hook("close",  (void*)my_close,  (void**)&o_close);
    LOGD("native shim installed");
}

} // namespace pairipfix
