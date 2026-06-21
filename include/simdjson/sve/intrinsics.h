#ifndef SIMDJSON_SVE_INTRINSICS_H
#define SIMDJSON_SVE_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/sve/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// SVE2 bulk ops come through std::simd's SVE backend; the gap fills (simd_gaps.h)
// use native SVE (svtbl / svext / svcompact) over the whole 64-byte vector. NEON is
// also included because numberparsing_defs.h (cloned from arm64) parses digits with
// NEON — SVE2 implies NEON on all real hardware.
#include <arm_sve.h>
#include <arm_neon.h>

static_assert(sizeof(uint8x16_t) <= simdjson::SIMDJSON_PADDING, "insufficient padding for sve");

#endif // SIMDJSON_SVE_INTRINSICS_H
