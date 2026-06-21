// NOTE: This backend requires the target hardware to expose a fixed SVE vector
// length at build time (compile with -msve-vector-bits=N, e.g. N=512).  Without
// a fixed VL the std::simd vec<uint8_t,64> type cannot be lowered to a single
// SVE register and will fall back to multiple NEON operations, defeating the
// purpose.  The SIMDJSON_CAN_ALWAYS_RUN_SVE guard below mirrors the pattern used
// by the haswell backend for SIMDJSON_CAN_ALWAYS_RUN_HASWELL.

#define SIMDJSON_IMPLEMENTATION sve

#include "simdjson/sve/base.h"
#include "simdjson/sve/intrinsics.h"

// NOTE (uncertainty): The correct GCC target attribute string for SVE2 is "sve2".
// If the toolchain or std::simd uses a different feature string (e.g. "arch=armv9-a")
// consult GCC documentation and adjust accordingly.
#if !SIMDJSON_CAN_ALWAYS_RUN_SVE
SIMDJSON_TARGET_REGION("sve2")
#endif

#include "simdjson/sve/bitmanipulation.h"
#include "simdjson/sve/bitmask.h"
#include "simdjson/sve/numberparsing_defs.h"
#include "simdjson/sve/simd.h"
#include "simdjson/sve/stringparsing_defs.h"
