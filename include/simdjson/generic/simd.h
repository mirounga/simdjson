// Generic SIMD wrapper: the 64-byte std::simd block, the target-dispatched gap shim, then the
// kernel ops. Same three includes every backend used; now ISA-agnostic (the shim is the only
// ISA-specific piece).
#ifndef SIMDJSON_GENERIC_SIMD_H
#define SIMDJSON_GENERIC_SIMD_H

#include "simdjson/generic/simd_block.h"
#include "simdjson/generic/simd_gaps.h"   // target-dispatched gap shim
#include "simdjson/generic/simd_kernel.h"

#endif // SIMDJSON_GENERIC_SIMD_H
