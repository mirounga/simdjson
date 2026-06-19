#ifndef SIMDJSON_STDSIMD_BASE_H
#define SIMDJSON_STDSIMD_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
/**
 * Portable implementation built on C++26 std::simd (<simd>).
 *
 * This backend reuses simdjson's generic algorithms unchanged; only the SIMD
 * primitive layer (simd8 / simd8x64 in simd.h) is expressed with std::simd
 * instead of target-specific intrinsics. See docs/stdsimd-migration/.
 */
namespace stdsimd {

class implementation;

namespace {
namespace simd {
template <typename T> struct simd8;
template <typename T> struct simd8x64;
} // namespace simd
} // unnamed namespace

} // namespace stdsimd
} // namespace simdjson

#endif // SIMDJSON_STDSIMD_BASE_H
