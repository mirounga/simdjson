#ifndef SIMDJSON_FALLBACK_BITMASK_H
#define SIMDJSON_FALLBACK_BITMASK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/fallback/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace fallback {
namespace {

//
// Perform a "cumulative bitwise xor," flipping bits each time a 1 is encountered.
//
// For example, prefix_xor(00100100) == 00011100
//
// Portable carry-less multiply by all-ones (no PCLMUL): the fallback backend targets
// no SIMD ISA, so this is a plain SWAR prefix XOR. Equivalent to the clmul-based
// prefix_xor the SIMD backends use.
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
} // namespace fallback
} // namespace simdjson

#endif // SIMDJSON_FALLBACK_BITMASK_H
