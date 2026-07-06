// prefix_xor (cumulative bitwise XOR) -- part of the shim surface. Uses the carry-less multiply
// (PCLMUL) accelerator when the compile target has it, else the portable SWAR fallback (correct on
// any target). generic/intrinsics.h (included before this by begin.h) provides the x86 intrinsic.
#ifndef SIMDJSON_GENERIC_BITMASK_H
#define SIMDJSON_GENERIC_BITMASK_H

#include "simdjson/base.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
// For example, prefix_xor(00100100) == 00011100
//
simdjson_inline uint64_t prefix_xor(uint64_t bitmask) {
#if defined(__PCLMUL__)
  // x86 carry-less multiply by all-ones.
  __m128i all_ones = _mm_set1_epi8('\xFF');
  __m128i result = _mm_clmulepi64_si128(_mm_set_epi64x(0ULL, bitmask), all_ones, 0);
  return (uint64_t)_mm_cvtsi128_si64(result);
#else
  // Portable SWAR prefix XOR (ISA-agnostic).
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
#endif
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_BITMASK_H
