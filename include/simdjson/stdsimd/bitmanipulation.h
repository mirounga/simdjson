#ifndef SIMDJSON_STDSIMD_BITMANIPULATION_H
#define SIMDJSON_STDSIMD_BITMANIPULATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/stdsimd/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace stdsimd {
namespace {

// Portable 64-bit bit-manipulation helpers (GCC builtins). These are scalar
// integer ops, not std::simd; inside the AVX2 target region the builtins lower
// to tzcnt/lzcnt/popcnt/blsr.
SIMDJSON_NO_SANITIZE_UNDEFINED
SIMDJSON_NO_SANITIZE_MEMORY
simdjson_inline int trailing_zeroes(uint64_t input_num) {
  return __builtin_ctzll(input_num);
}

/* result might be undefined when input_num is zero */
simdjson_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num - 1);
}

/* result might be undefined when input_num is zero */
simdjson_inline int leading_zeroes(uint64_t input_num) {
  return __builtin_clzll(input_num);
}

simdjson_inline long long int count_ones(uint64_t input_num) {
  return __builtin_popcountll(input_num);
}

simdjson_inline bool add_overflow(uint64_t value1, uint64_t value2, uint64_t *result) {
  return __builtin_uaddll_overflow(value1, value2,
                                   reinterpret_cast<unsigned long long *>(result));
}

} // unnamed namespace
} // namespace stdsimd
} // namespace simdjson

#endif // SIMDJSON_STDSIMD_BITMANIPULATION_H
