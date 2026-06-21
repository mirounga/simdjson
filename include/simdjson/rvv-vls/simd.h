#ifndef SIMDJSON_RVV_VLS_SIMD_H
#define SIMDJSON_RVV_VLS_SIMD_H

// rvv-vls rides the shared std::simd kernel: the 64-byte block is
// std::simd::vec<uint8_t,64> (a single fixed-width 512-bit RVV register), with RVV
// native gap fills (lookup_16/prev/compress). The hand-written RVV simd8/simd8x64
// kernel this file used to contain was replaced by the unified std::simd kernel
// (branch stdioc). NOTE: GCC 16 std::simd has no RISC-V V backend, so the kernel's
// non-gap ops lower to scalar -- this backend trades peak performance for a single
// unified code path.
// Fixed include order: block type, then the RVV gap fills, then the kernel ops.
#include "simdjson/generic/simd_block.h"
#include "simdjson/rvv-vls/simd_gaps.h"
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_RVV_VLS_SIMD_H
