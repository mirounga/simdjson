#ifndef SIMDJSON_PPC64_SIMD_H
#define SIMDJSON_PPC64_SIMD_H

// ppc64 rides the shared std::simd kernel: the 64-byte block is
// std::simd::vec<uint8_t,64> (split into 4x Altivec/VSX registers), with VSX native gap
// fills (lookup_16/prev/compress). The hand-written Altivec simd8 kernel this file used
// to contain was replaced by the unified std::simd kernel (branch stdioc).
// Fixed include order: block type, then the VSX gap fills, then the kernel ops.
#include "simdjson/generic/simd_block.h"
#include "simdjson/ppc64/simd_gaps.h"
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_PPC64_SIMD_H
