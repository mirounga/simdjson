#ifndef SIMDJSON_GENERIC_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_BASE_H
#include "simdjson/base.h"
// If we haven't got an implementation yet, we're in the editor, editing a generic file! Just
// pick the compile-time builtin so the most possible stuff can be tested. (In a real build
// SIMDJSON_IMPLEMENTATION is always already defined by src/<isa>.cpp or builtin, so this is skipped.)
#ifndef SIMDJSON_IMPLEMENTATION
#include "simdjson/implementation_detection.h"
#define SIMDJSON_IMPLEMENTATION SIMDJSON_BUILTIN_IMPLEMENTATION
#include "simdjson/generic/begin.h"
#endif // SIMDJSON_IMPLEMENTATION
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

struct open_container;
class dom_parser_implementation;

/**
 * The type of a JSON number
 */
enum class number_type {
    floating_point_number=1, /// a binary64 number
    signed_integer,          /// a signed integer that fits in a 64-bit word using two's complement
    unsigned_integer,        /// a positive integer larger or equal to 1<<63
    big_integer              /// a big integer that does not fit in a 64-bit word
};

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_BASE_H
