#ifndef SIMDJSON_GENERIC_JSON_CHARACTER_BLOCK_H

#define SIMDJSON_GENERIC_JSON_CHARACTER_BLOCK_H
#include "simdjson/generic/base.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

struct json_character_block {
  static simdjson_inline json_character_block classify(const simd::block& in);

  simdjson_inline uint64_t whitespace() const noexcept { return _whitespace; }
  simdjson_inline uint64_t op() const noexcept { return _op; }
  simdjson_inline uint64_t scalar() const noexcept { return ~(op() | whitespace()); }

  uint64_t _whitespace;
  uint64_t _op;
};

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_JSON_CHARACTER_BLOCK_H