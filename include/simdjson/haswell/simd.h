#ifndef SIMDJSON_HASWELL_SIMD_H
#define SIMDJSON_HASWELL_SIMD_H

// Haswell rides the shared std::simd kernel: the 64-byte block is
// std::simd::vec<uint8_t,64> (split into 2x AVX2 registers), with AVX2 native gap
// fills (lookup_16/prev/compress). The hand-written AVX2 simd8 kernel this file
// used to contain was replaced by the unified std::simd kernel (branch stdioc).
// Fixed include order: block type, then the AVX2 gap fills, then the kernel ops.
#include "simdjson/generic/simd_block.h"
#include "simdjson/haswell/simd_gaps.h"
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_HASWELL_SIMD_H
