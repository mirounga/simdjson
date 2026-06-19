#define SIMDJSON_IMPLEMENTATION stdsimd

#include "simdjson/stdsimd/base.h"
#include "simdjson/stdsimd/intrinsics.h"

// Compile the SIMD region for AVX2 (matching the haswell width/ISA) so the
// std::simd<uint8_t,32> ops lower to AVX2, for apples-to-apples benchmarking.
#if !SIMDJSON_CAN_ALWAYS_RUN_STDSIMD
#if defined(__clang__)
SIMDJSON_TARGET_REGION("avx2,bmi,bmi2,pclmul,lzcnt,popcnt")
#else
SIMDJSON_TARGET_REGION("avx2,bmi,pclmul,lzcnt,popcnt")
#endif
#endif

#include "simdjson/stdsimd/bitmanipulation.h"
#include "simdjson/stdsimd/bitmask.h"
#include "simdjson/stdsimd/numberparsing_defs.h"
#include "simdjson/stdsimd/simd.h"
#include "simdjson/stdsimd/stringparsing_defs.h"
