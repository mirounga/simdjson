#ifndef SIMDJSON_STDSIMD_BITMASK_H
#define SIMDJSON_STDSIMD_BITMASK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/stdsimd/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace stdsimd {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
// For example, prefix_xor(00100100) == 00011100.
//
// This is a scalar uint64_t bit-trick, NOT a std::simd operation: carry-less
// multiply by all-ones is equivalent to this shift-XOR ladder. We use the
// portable ladder (the same approach as the arm64 backend) rather than the x86
// CLMUL intrinsic, keeping the backend ISA-independent. (Not a std::simd gap.)
//
simdjson_inline uint64_t prefix_xor(uint64_t bitmask) {
  bitmask ^= bitmask << 1;
  bitmask ^= bitmask << 2;
  bitmask ^= bitmask << 4;
  bitmask ^= bitmask << 8;
  bitmask ^= bitmask << 16;
  bitmask ^= bitmask << 32;
  return bitmask;
}

} // unnamed namespace
} // namespace stdsimd
} // namespace simdjson

#endif // SIMDJSON_STDSIMD_BITMASK_H
