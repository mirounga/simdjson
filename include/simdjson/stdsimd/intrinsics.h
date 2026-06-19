#ifndef SIMDJSON_STDSIMD_INTRINSICS_H
#define SIMDJSON_STDSIMD_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/stdsimd/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// C++26 data-parallel types. In GCC 16 these live in namespace std::simd.
#include <simd>
#include <bit> // std::bit_cast (for the optional native-pshufb escape hatch)

// Optional, non-portable performance escape hatch: when enabled on x86-64, the
// lookup_16 primitive uses native AVX2 vpshufb (via std::bit_cast to __m256i)
// instead of the portable std::simd generator-gather. This exists only because
// GCC 16's dynamic std::simd::permute is unimplemented (see GAPS.md). Default
// OFF to keep the backend ISA-independent; define to 1 to measure/recover perf.
#ifndef SIMDJSON_STDSIMD_NATIVE_PSHUFB
#define SIMDJSON_STDSIMD_NATIVE_PSHUFB 0
#endif
// Same kind of escape hatch for compress (whitespace compaction in minify): use
// the AVX2 thintable/pshufb compaction instead of the portable scalar gather.
#ifndef SIMDJSON_STDSIMD_NATIVE_COMPRESS
#define SIMDJSON_STDSIMD_NATIVE_COMPRESS 0
#endif
// And for prev<N> (cross-chunk byte shift, used by UTF-8 validation): use the
// AVX2 alignr/permute2x128 idiom instead of the portable store-to-memory-and-
// reload-at-offset. x86-64 only; default OFF.
#ifndef SIMDJSON_STDSIMD_NATIVE_ALIGNR
#define SIMDJSON_STDSIMD_NATIVE_ALIGNR 0
#endif
#if (SIMDJSON_STDSIMD_NATIVE_PSHUFB || SIMDJSON_STDSIMD_NATIVE_COMPRESS || SIMDJSON_STDSIMD_NATIVE_ALIGNR) && SIMDJSON_IS_X86_64
#include <immintrin.h>
#endif

namespace simdjson {
namespace stdsimd {
namespace {
namespace simd {
// Short alias for the std::simd namespace used throughout this backend.
namespace dp = std::simd;
} // namespace simd
} // unnamed namespace
} // namespace stdsimd
} // namespace simdjson

#endif // SIMDJSON_STDSIMD_INTRINSICS_H
