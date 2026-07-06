// Generic, ISA-agnostic bit manipulation. Portable across all targets: the compiler emits the
// native tzcnt/lzcnt/popcnt/blsr at the object's -march. (No per-ISA intrinsic variants needed.)
#ifndef SIMDJSON_GENERIC_BITMANIPULATION_H
#define SIMDJSON_GENERIC_BITMANIPULATION_H

#include "simdjson/base.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

#if defined(_MSC_VER) && !defined(_M_ARM64) && !defined(_M_X64)
static inline unsigned char _BitScanForward64(unsigned long* ret, uint64_t x) {
  unsigned long x0 = (unsigned long)x, top, bottom;
  _BitScanForward(&top, (unsigned long)(x >> 32));
  _BitScanForward(&bottom, x0);
  *ret = x0 ? bottom : 32 + top;
  return x != 0;
}
static unsigned char _BitScanReverse64(unsigned long* ret, uint64_t x) {
  unsigned long x1 = (unsigned long)(x >> 32), top, bottom;
  _BitScanReverse(&top, x1);
  _BitScanReverse(&bottom, (unsigned long)x);
  *ret = x1 ? top + 32 : bottom;
  return x != 0;
}
#endif

/* result might be undefined when input_num is zero */
simdjson_inline int leading_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  unsigned long leading_zero = 0;
  if (_BitScanReverse64(&leading_zero, input_num))
    return (int)(63 - leading_zero);
  else
    return 64;
#else
  return __builtin_clzll(input_num);
#endif
}

SIMDJSON_NO_SANITIZE_UNDEFINED
SIMDJSON_NO_SANITIZE_MEMORY
simdjson_inline int trailing_zeroes(uint64_t input_num) {
#ifdef _MSC_VER
  unsigned long trailing_zero = 0;
  if (_BitScanForward64(&trailing_zero, input_num))
    return (int)trailing_zero;
  else
    return 64;
#else
  return __builtin_ctzll(input_num);
#endif
}

/* result might be undefined when input_num is zero */
simdjson_inline uint64_t clear_lowest_bit(uint64_t input_num) {
  return input_num & (input_num - 1);
}

simdjson_inline int count_ones(uint64_t input_num) {
#ifdef _MSC_VER
  return (int)__popcnt64(input_num);
#else
  return __builtin_popcountll(input_num);
#endif
}

simdjson_inline bool add_overflow(uint64_t value1, uint64_t value2, uint64_t *result) {
#ifdef _MSC_VER
  return _addcarry_u64(0, value1, value2, reinterpret_cast<unsigned __int64 *>(result));
#else
  return __builtin_uaddll_overflow(value1, value2, reinterpret_cast<unsigned long long *>(result));
#endif
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_BITMANIPULATION_H
