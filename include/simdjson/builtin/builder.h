#ifndef SIMDJSON_BUILTIN_BUILDER_H
#define SIMDJSON_BUILTIN_BUILDER_H

#include "simdjson/builtin.h"
#include "simdjson/builtin/base.h"

#include "simdjson/generic/builder/dependencies.h"

#define SIMDJSON_IMPLEMENTATION SIMDJSON_BUILTIN_IMPLEMENTATION
#define SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/generic/builder_impl.h"
#undef SIMDJSON_CONDITIONAL_INCLUDE
#undef SIMDJSON_IMPLEMENTATION

namespace simdjson {
  /**
   * @copydoc simdjson::SIMDJSON_BUILTIN_IMPLEMENTATION::builder
   */
  namespace builder = SIMDJSON_BUILTIN_IMPLEMENTATION::builder;
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_BUILDER_H
