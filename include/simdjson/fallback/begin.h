#define SIMDJSON_IMPLEMENTATION fallback

// Fallback rides the shared std::simd kernel (compiler scalarizes); see fallback/simd.h. The
// string-parsing finder arrives via simd.h -> simd_kernel.h, so no stringparsing include here.
#include "simdjson/fallback/base.h"
#include "simdjson/fallback/bitmanipulation.h"
#include "simdjson/fallback/numberparsing_defs.h"
#include "simdjson/fallback/simd.h"
