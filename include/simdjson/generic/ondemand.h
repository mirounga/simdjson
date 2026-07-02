// Generic ondemand umbrella (replaces per-ISA <isa>/ondemand.h). Includer sets SIMDJSON_IMPLEMENTATION
// and must have already pulled the DOM-side generic code (builtin/ondemand.h includes builtin.h first).
// Self-contained: non-generic ondemand deps + preamble + the ondemand generic code in dependency order.

// Non-generic dependencies (formerly generic/ondemand/dependencies.h).
#include "simdjson/dom/base.h" // for MINIMAL_DOCUMENT_CAPACITY
#include "simdjson/implementation.h"
#include "simdjson/base.h"
#include "simdjson/common_defs.h"
#include "simdjson/constevalutil.h"
#include "simdjson/padded_string.h"
#include "simdjson/padded_string_view.h"
#include "simdjson/internal/dom_parser_implementation.h"
#include "simdjson/jsonpathutil.h"
#include "simdjson/annotations.h"

#include "simdjson/generic/begin.h"

// ondemand generic code (formerly generic/ondemand/amalgamated.h), in dependency order.
// Stuff other things depend on
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/deserialize.h"
#include "simdjson/generic/ondemand/value_iterator.h"
#include "simdjson/generic/ondemand/value.h"
#include "simdjson/generic/ondemand/logger.h"
#include "simdjson/generic/ondemand/token_iterator.h"
#include "simdjson/generic/ondemand/json_iterator.h"
#include "simdjson/generic/ondemand/json_type.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#include "simdjson/generic/ondemand/parser.h"

// All other declarations
#include "simdjson/generic/ondemand/array.h"
#include "simdjson/generic/ondemand/array_iterator.h"
#include "simdjson/generic/ondemand/document.h"
#include "simdjson/generic/ondemand/document_stream.h"
#include "simdjson/generic/ondemand/field.h"
#include "simdjson/generic/ondemand/key_selector.h"
#include "simdjson/generic/ondemand/object.h"
#include "simdjson/generic/ondemand/object_iterator.h"
#include "simdjson/generic/ondemand/ranges.h"
#include "simdjson/generic/ondemand/serialization.h"

// Deserialization for standard types
#include "simdjson/generic/ondemand/std_deserialize.h"

// Inline definitions
#include "simdjson/generic/ondemand/array-inl.h"
#include "simdjson/generic/ondemand/array_iterator-inl.h"
#include "simdjson/generic/ondemand/value-inl.h"
#include "simdjson/generic/ondemand/document-inl.h"
#include "simdjson/generic/ondemand/document_stream-inl.h"
#include "simdjson/generic/ondemand/field-inl.h"
#include "simdjson/generic/ondemand/json_iterator-inl.h"
#include "simdjson/generic/ondemand/json_type-inl.h"
#include "simdjson/generic/ondemand/logger-inl.h"
#include "simdjson/generic/ondemand/object-inl.h"
#include "simdjson/generic/ondemand/object_iterator-inl.h"
#include "simdjson/generic/ondemand/ranges-inl.h"
#include "simdjson/generic/ondemand/parser-inl.h"
#include "simdjson/generic/ondemand/raw_json_string-inl.h"
#include "simdjson/generic/ondemand/token_iterator-inl.h"
#include "simdjson/generic/ondemand/value_iterator-inl.h"
#include "simdjson/generic/ondemand/serialization-inl.h"

// JSON path accessor (compile-time) - must be after inline definitions
#include "simdjson/generic/ondemand/compile_time_accessors.h"

#include "simdjson/generic/end.h"
