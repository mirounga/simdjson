#ifndef SIMDJSON_FALLBACK_SIMD_H
#define SIMDJSON_FALLBACK_SIMD_H

// Fallback rides the shared std::simd kernel like every other backend: the 64-byte block is
// std::simd::vec<uint8_t,64>. It targets no SIMD ISA, so the compiler scalarizes the kernel ops;
// the gap fills (lookup_16/prev/compress) are portable C++ loops in fallback/simd_gaps.h.
//
// Fixed include order: block type, then the portable gap fills, then the kernel ops (which also
// pull in the shared string-parsing finder).
#include "simdjson/generic/simd_block.h"
#include "simdjson/fallback/simd_gaps.h"
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_FALLBACK_SIMD_H
