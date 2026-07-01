// Target-dispatched intrinsics include (part of the shim surface). Pulls the ISA header needed by
// the gap shim / prefix_xor for the COMPILE TARGET. Permanent guard (system headers, idempotent).
#ifndef SIMDJSON_GENERIC_INTRINSICS_H
#define SIMDJSON_GENERIC_INTRINSICS_H

#include "simdjson/base.h"

#if defined(__AVX2__) || defined(__SSE4_2__) || defined(__AVX512F__)
  // x86
  #if SIMDJSON_VISUAL_STUDIO
  #include <intrin.h>
  #else
  #include <x86intrin.h>
  #endif
  #if SIMDJSON_CLANG_VISUAL_STUDIO
  // clang-cl only pulls these when the matching feature macro is set; we include <x86intrin.h>
  // (or <intrin.h>) first to unlock them. See the historical haswell/intrinsics.h note.
  #include <bmiintrin.h>
  #include <lzcntintrin.h>
  #include <immintrin.h>
  #include <smmintrin.h>
  #include <tmmintrin.h>
  #include <wmmintrin.h> // _mm_clmulepi64_si128
  #ifndef _blsr_u64
  #define _blsr_u64(n) ((n - 1) & n)
  #endif
  #endif
#elif defined(__ARM_FEATURE_SVE2)
  #include <arm_sve.h>
  #include <arm_neon.h>
#elif defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM64)
  #include <arm_neon.h>
#elif defined(__VSX__) || defined(__ALTIVEC__)
  #include <altivec.h>
#elif defined(__loongarch_asx)
  #include <lsxintrin.h>
  #include <lasxintrin.h>
#elif defined(__loongarch_sx)
  #include <lsxintrin.h>
#elif defined(__riscv_v_intrinsic)
  #include <riscv_vector.h>
#endif
// else: portable target, no intrinsics header.

#endif // SIMDJSON_GENERIC_INTRINSICS_H
