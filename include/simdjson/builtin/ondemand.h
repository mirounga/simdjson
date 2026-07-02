#ifndef SIMDJSON_BUILTIN_ONDEMAND_H
#define SIMDJSON_BUILTIN_ONDEMAND_H

#include "simdjson/builtin.h"
#include "simdjson/builtin/base.h"


#define SIMDJSON_IMPLEMENTATION SIMDJSON_BUILTIN_IMPLEMENTATION
#include "simdjson/generic/ondemand.h"
#undef SIMDJSON_IMPLEMENTATION

namespace simdjson {
  /**
   * @copydoc simdjson::SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand
   */
  namespace ondemand = SIMDJSON_BUILTIN_IMPLEMENTATION::ondemand;
} // namespace simdjson

#endif // SIMDJSON_BUILTIN_ONDEMAND_H
