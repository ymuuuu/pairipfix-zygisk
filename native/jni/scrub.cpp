#include "scrub.h"
#include <cctype>
#include <cstring>

namespace pairipfix {

static const char* kTokens[] = {
    "frida", "gum", "frida-agent", "frida-gadget", "xposed",
    "lsposed", "magisk", "zygisk", "riru", "substrate", "27042"
};

static bool line_blocked(const char* s, size_t len) {
    for (const char* tok : kTokens) {
        size_t tl = strlen(tok);
        if (tl > len) continue;
        for (size_t i = 0; i + tl <= len; i++) {
            size_t j = 0;
            for (; j < tl; j++)
                if (std::tolower(static_cast<unsigned char>(s[i+j])) !=
                    std::tolower(static_cast<unsigned char>(tok[j]))) break;
            if (j == tl) return true;
        }
    }
    return false;
}

size_t scrub_lines(const char* in, size_t in_len, char* out, size_t out_cap) {
    size_t w = 0, i = 0;
    while (i < in_len) {
        size_t j = i;
        while (j < in_len && in[j] != '\n') j++;
        size_t line_len = (j < in_len) ? (j - i + 1) : (j - i);
        if (!line_blocked(in + i, j - i)) {
            if (w + line_len <= out_cap) {
                std::memcpy(out + w, in + i, line_len);
                w += line_len;
            }
        }
        i = (j < in_len) ? j + 1 : j;
    }
    return w;
}

} // namespace pairipfix
