#ifndef SIMDJSON_ICELAKE_SIMD_H
#define SIMDJSON_ICELAKE_SIMD_H

// Icelake rides the shared std::simd kernel: the 64-byte block is
// std::simd::vec<uint8_t,64> (a single AVX-512 register), with AVX-512 native gap
// fills (lookup_16/prev/compress). The hand-written AVX-512 simd8 kernel this file
// used to contain was replaced by the unified std::simd kernel (branch stdioc).
// Fixed include order: block type, then the AVX-512 gap fills, then the kernel ops.
#include "simdjson/generic/simd_block.h"
#include "simdjson/icelake/simd_gaps.h"
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_ICELAKE_SIMD_H
