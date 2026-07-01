#ifndef SIMDJSON_BUILTIN_IMPLEMENTATION_H
#define SIMDJSON_BUILTIN_IMPLEMENTATION_H

#include "simdjson/builtin/base.h"

#include "simdjson/generic/dependencies.h"

#define SIMDJSON_IMPLEMENTATION SIMDJSON_BUILTIN_IMPLEMENTATION
#include "simdjson/generic/implementation.h"
#undef SIMDJSON_IMPLEMENTATION

namespace simdjson {
  /**
   * Function which returns a pointer to an implementation matching the "builtin" implementation.
   * The builtin implementation is the best statically linked simdjson implementation that can be used by the compiling
   * program. If you compile with g++ -march=haswell, this will return the haswell implementation.
   * It is handy to be able to check what builtin was used: builtin_implementation()->name().
   */
  const implementation * builtin_implementation();
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_IMPLEMENTATION_H
