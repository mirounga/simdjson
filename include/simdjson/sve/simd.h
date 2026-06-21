#ifndef SIMDJSON_SVE_SIMD_H
#define SIMDJSON_SVE_SIMD_H

// ARM SVE2 rides the shared std::simd kernel: the 64-byte block is
// std::simd::vec<uint8_t,64> (requires -msve-vector-bits=512 so it maps to one
// Z register).  Bulk ops (eq, lteq, to_bitmask, is_ascii, saturating_sub) lower
// through std::simd's SVE backend and emit wide SVE instructions.  The gap fills
// (lookup_16/prev/compress) that std::simd does not cover are supplied by
// sve/simd_gaps.h using native SVE intrinsics (svtbl/svext/svcompact) over the
// whole 64-byte vector.
//
// Fixed include order: block type, then the SVE gap fills, then the kernel ops.
#include "simdjson/generic/simd_block.h"
#include "simdjson/sve/simd_gaps.h"
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_SVE_SIMD_H
