#ifndef SIMDJSON_BUILTIN_H
#define SIMDJSON_BUILTIN_H

#include "simdjson/builtin/base.h"
#include "simdjson/builtin/implementation.h"


// ISA-agnostic: the builtin implementation is the generic std::simd code compiled at the
// keeps the amalgamated headers in dependency order (dependencies.h above pre-declares them).
#define SIMDJSON_IMPLEMENTATION SIMDJSON_BUILTIN_IMPLEMENTATION
#include "simdjson/generic/umbrella.h"
#undef SIMDJSON_IMPLEMENTATION

#endif // SIMDJSON_BUILTIN_H
