// Shared std::simd block type (no include guard of its own; emitted once per
// implementation namespace, like the other generic/*.h bodies).
//
// The 64-byte JSON block is `std::simd::vec<uint8_t, 64>` used directly -- there
// is no simd8 / simd8x64 wrapper type. std::simd splits the 64-lane vector into
// native registers (4x SSE on westmere, 2x AVX2 on haswell, 1x AVX-512 on icelake).
//
// Include order within each <isa>/simd.h: this header (defines `block`), then the
// per-backend <isa>/simd_gaps.h (defines `native::` gap fills over `block`), then
// generic/simd_kernel.h (the free-function ops that call `native::`).

// Make sure SIMDJSON_STD_SIMD_AVAILABLE (and the implementation-set macros) are defined
// before we test it. When a backend is compiled as its own translation unit (the
// separate-object build), <isa>/begin.h does not pull in implementation_detection.h
// ahead of this header the way the old amalgamated TU did. The header carries its own
// include guard, so this is idempotent.
#include "simdjson/implementation_detection.h"

// C++26 data-parallel types + std::bit_cast. Guarded by the std::simd availability
// macro so the system headers never reach a pre-C++26 TU (e.g. the C++11/14/17
// single-header builds, where this whole region is compiled out anyway). System
// headers carry their own include guards, so this is included unconditionally here.
#if SIMDJSON_STD_SIMD_AVAILABLE
#include <simd>
#include <bit>
#endif

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {

namespace dp = std::simd;

// The 64-byte block. Everything in stage 1 is a 64-lane vector now.
using block = dp::vec<uint8_t, 64>;

} // namespace simd
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
