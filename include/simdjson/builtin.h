#ifndef SIMDJSON_BUILTIN_H
#define SIMDJSON_BUILTIN_H

#include "simdjson/builtin/base.h"
#include "simdjson/builtin/implementation.h"

#include "simdjson/generic/dependencies.h"

// ISA-agnostic: the builtin implementation is the generic std::simd code compiled at the
// consumer's own -march, in the SIMDJSON_BUILTIN_IMPLEMENTATION namespace. SIMDJSON_CONDITIONAL_INCLUDE
// keeps the amalgamated headers in dependency order (dependencies.h above pre-declares them).
#define SIMDJSON_IMPLEMENTATION SIMDJSON_BUILTIN_IMPLEMENTATION
#define SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/generic/umbrella.h"
#undef SIMDJSON_CONDITIONAL_INCLUDE
#undef SIMDJSON_IMPLEMENTATION

#endif // SIMDJSON_BUILTIN_H
