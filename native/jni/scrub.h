#pragma once
#include <cstddef>

namespace pairipfix {
size_t scrub_lines(const char* in, size_t in_len, char* out, size_t out_cap);
}
