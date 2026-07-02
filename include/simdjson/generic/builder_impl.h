// Generic builder umbrella (replaces per-ISA <isa>/builder.h). Includer sets SIMDJSON_IMPLEMENTATION
// and must have already pulled the DOM-side generic code (builtin/builder.h includes builtin.h first).
// Named builder_impl.h to avoid clashing with the generic/builder/ directory. Self-contained.

// Non-generic dependencies (formerly generic/builder/dependencies.h).
#include "simdjson/concepts.h"
#include "simdjson/dom/fractured_json.h"
#include "simdjson/annotations.h"

#include "simdjson/generic/begin.h"

// builder generic code (formerly generic/builder/amalgamated.h), in dependency order.
#include "simdjson/generic/builder/json_string_builder.h"
#include "simdjson/generic/builder/json_builder.h"
#include "simdjson/generic/builder/fractured_json_builder.h"
#include "simdjson/generic/builder/json_string_builder-inl.h"

#include "simdjson/generic/end.h"
