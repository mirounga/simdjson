#ifndef SIMDJSON_LSX_SIMD_H
#define SIMDJSON_LSX_SIMD_H

// lsx rides the shared std::simd kernel: the 64-byte block is
// std::simd::vec<uint8_t,64> (split into 4x LSX registers), with LSX native gap
// fills (lookup_16/prev/compress). The hand-written LSX simd8 kernel this file used
// to contain was replaced by the unified std::simd kernel (branch stdioc).
// Fixed include order: block type, then the LSX gap fills, then the kernel ops.
#include "simdjson/generic/simd_block.h"
#include "simdjson/lsx/simd_gaps.h"
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_LSX_SIMD_H
