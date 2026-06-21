#ifndef SIMDJSON_WESTMERE_SIMD_H
#define SIMDJSON_WESTMERE_SIMD_H

// Westmere rides the shared std::simd kernel: the 64-byte block is
// std::simd::vec<uint8_t,64> (split into 4x SSE registers), with SSE native gap
// fills (lookup_16/prev/compress). The hand-written SSE simd8 kernel this file
// used to contain was replaced by the unified std::simd kernel (branch stdioc).
// Fixed include order: block type, then the SSE gap fills, then the kernel ops.
#include "simdjson/generic/simd_block.h"
#include "simdjson/westmere/simd_gaps.h"
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_WESTMERE_SIMD_H
