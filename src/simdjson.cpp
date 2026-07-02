#define SIMDJSON_SRC_SIMDJSON_CPP

#include <base.h>

SIMDJSON_PUSH_DISABLE_UNUSED_WARNINGS



#include <to_chars.cpp>
#include <from_chars.cpp>
#include <internal/error_tables.cpp>
#include <internal/jsoncharutils_tables.cpp>
#include <internal/numberparsing_tables.cpp>
#include <internal/simdprune_tables.cpp>
#include <implementation.cpp>

// NOTE: the per-ISA backends (src/<isa>.cpp) are NO LONGER amalgamated into this TU.
// Each is compiled as its own object with its own global -march (the std::simd kernel
// needs a global target, not per-function attributes); CMake adds them to the library
// and the runtime dispatcher in implementation.cpp selects among them. This TU is the
// baseline aggregate: shared tables (above) + the dispatcher (implementation.cpp).

SIMDJSON_POP_DISABLE_UNUSED_WARNINGS

