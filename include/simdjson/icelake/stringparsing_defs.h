#ifndef SIMDJSON_ICELAKE_STRINGPARSING_DEFS_H
#define SIMDJSON_ICELAKE_STRINGPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/icelake/base.h"
#include "simdjson/icelake/simd.h"
#include "simdjson/icelake/bitmanipulation.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

using namespace simd;
namespace dp = std::simd;

// Icelake processes a full 64-byte block per string-parsing step (one AVX-512
// register): the kernel `block` directly, no simd8 wrapper. bs/quote/escape masks
// are 64-bit. Uses the kernel free functions (load_block / to_bitmask).

struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 64;
  simdjson_inline backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_inline bool has_backslash() { return ((quote_bits - 1) & bs_bits) != 0; }
  simdjson_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint64_t bs_bits;
  uint64_t quote_bits;
}; // struct backslash_and_quote

simdjson_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  block v = load_block(src);
  dp::unchecked_store(v, dst, 64);
  return {
      to_bitmask(v == block(uint8_t('\\'))), // bs_bits
      to_bitmask(v == block(uint8_t('"'))),  // quote_bits
  };
}

struct escaping {
  static constexpr uint32_t BYTES_PROCESSED = 64;
  simdjson_inline static escaping copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_escape() { return escape_bits != 0; }
  simdjson_inline int escape_index() { return trailing_zeroes(escape_bits); }

  uint64_t escape_bits;
}; // struct escaping

simdjson_inline escaping escaping::copy_and_find(const uint8_t *src, uint8_t *dst) {
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "escaping finder must process fewer than SIMDJSON_PADDING bytes");
  block v = load_block(src);
  dp::unchecked_store(v, dst, 64);
  auto is_quote = (v == block(uint8_t('"')));
  auto is_backslash = (v == block(uint8_t('\\')));
  auto is_control = (v < block(uint8_t(32)));
  return { to_bitmask(is_backslash | is_quote | is_control) };
}

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_ICELAKE_STRINGPARSING_DEFS_H
