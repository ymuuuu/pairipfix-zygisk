#include <cassert>
#include <cstdio>
#include <cstring>
#include "../jni/scrub.h"

int main() {
    const char* in =
        "1000-2000 r-xp lib.so\n"
        "3000-4000 r-xp frida-agent-64.so\n"
        "5000-6000 r-xp normal.so\n";
    char out[512];
    size_t n = pairipfix::scrub_lines(in, strlen(in), out, sizeof(out));
    out[n] = 0;
    assert(strstr(out, "frida") == nullptr);
    assert(strstr(out, "lib.so") != nullptr);
    assert(strstr(out, "normal.so") != nullptr);
    printf("OK\n");
    return 0;
}
