#ifndef SIMDJSON_GENERIC_STAGE1_BASE_H

#define SIMDJSON_GENERIC_STAGE1_BASE_H
#include "simdjson/generic/base.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

class bit_indexer;
template<size_t STEP_SIZE>
struct buf_block_reader;
struct json_block;
class json_minifier;
class json_scanner;
struct json_string_block;
class json_string_scanner;
class json_structural_indexer;

} // namespace stage1

namespace utf8_validation {
struct utf8_checker;
} // namespace utf8_validation

using utf8_validation::utf8_checker;

// Per-backend UTF-8 multibyte-continuation helper (defined in the backend TU / engine bodies);
// forward-declared here so utf8_lookup4_algorithm.h can reference it before its definition. Lives in
// the engine-only stage1 tree so DOM/ondemand-only TUs never see this undefined free function.
simdjson_inline simd::block must_be_2_3_continuation(const simd::block prev2, const simd::block prev3);

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_STAGE1_BASE_H
