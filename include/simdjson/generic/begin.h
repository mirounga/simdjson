// Generic per-implementation preamble. The includer (src/<isa>.cpp for the fat binary, or
// builtin.h for the header-only consumer) must `#define SIMDJSON_IMPLEMENTATION <name>` first; this
// file does NOT set it. Pulls the target intrinsics + the (ISA-agnostic) bit/number helpers + the
// SIMD kernel (whose only ISA-specific piece is the gap shim). No target-region pragmas: each object
// is compiled with a global -march (or the consumer's -march).
#include "simdjson/generic/impl_fwd.h"
#include "simdjson/generic/intrinsics.h"
#include "simdjson/generic/bitmanipulation.h"
#include "simdjson/generic/bitmask.h"
#include "simdjson/generic/numberparsing_defs.h"
#include "simdjson/generic/simd.h"
