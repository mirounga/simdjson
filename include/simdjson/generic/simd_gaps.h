// The gap-fill SHIM: the only ISA-specific code in the generic std::simd implementation.
//
// std::simd cannot (yet) express dynamic byte shuffle (lookup_16 / pshufb), cross-vector byte
// shift (prev<N>), or compaction (compress). This header selects a native implementation of
// native::lookup_16 / prev<N> / compress for the COMPILE TARGET, falling back to a portable
// std::simd version when no accelerated target is detected. Selection keys off the compiler's
// target macros, so the SAME header serves the header-only consumer (their -march) and each
// per-march fat-binary object (src/<isa>.cpp). The native branches are TEMPORARY: when std::simd
// gains dynamic permute + compress, delete them and the portable `#else` stands alone.
//
// Included by generic/simd.h between generic/simd_block.h (defines `block`) and
// generic/simd_kernel.h (the ops that call native::). SIMDJSON_IMPLEMENTATION selects the namespace.

#if defined(__AVX512VBMI2__)
  #include "simdjson/shim/icelake_gaps.h"
#elif defined(__AVX2__)
  #include "simdjson/shim/haswell_gaps.h"
#elif defined(__SSE4_2__)
  #include "simdjson/shim/westmere_gaps.h"
#elif defined(__ARM_FEATURE_SVE2)
  #include "simdjson/shim/sve_gaps.h"
#elif defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM64)
  #include "simdjson/shim/arm64_gaps.h"
#elif defined(__loongarch_asx)
  #include "simdjson/shim/lasx_gaps.h"
#elif defined(__loongarch_sx)
  #include "simdjson/shim/lsx_gaps.h"
#elif defined(__VSX__) || defined(__ALTIVEC__)
  #include "simdjson/shim/ppc64_gaps.h"
#elif defined(__riscv_v_intrinsic)
  #include "simdjson/shim/rvv_gaps.h"
#else
  #include "simdjson/shim/portable_gaps.h"
#endif
