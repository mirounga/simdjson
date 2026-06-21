#ifndef SIMDJSON_SRC_GENERIC_DOM_PARSER_IMPLEMENTATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_DOM_PARSER_IMPLEMENTATION_H
#include <generic/base.h>
#include <simdjson/generic/dom_parser_implementation.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// Interface a dom parser implementation must fulfill
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

// The UTF-8 multibyte-continuation helper is provided per-backend (e.g. haswell.cpp,
// westmere.cpp, icelake.cpp). is_ascii(block) comes from the shared kernel.
simdjson_inline simd::block must_be_2_3_continuation(const simd::block prev2, const simd::block prev3);

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_DOM_PARSER_IMPLEMENTATION_H
