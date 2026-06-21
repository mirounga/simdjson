#ifndef SIMDJSON_LASX_SIMD_H
#define SIMDJSON_LASX_SIMD_H

// LASX rides the shared std::simd kernel: the 64-byte block is
// std::simd::vec<uint8_t,64> (split into 2x 256-bit registers), with LASX native gap
// fills (lookup_16/prev/compress). The hand-written LASX simd8 kernel this file used
// to contain was replaced by the unified std::simd kernel (branch stdioc).
// Fixed include order: block type, then the LASX gap fills, then the kernel ops.
#include "simdjson/generic/simd_block.h"
#include "simdjson/lasx/simd_gaps.h"
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_LASX_SIMD_H
