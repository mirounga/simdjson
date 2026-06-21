#ifndef SIMDJSON_LSX_STRINGPARSING_DEFS_H
#define SIMDJSON_LSX_STRINGPARSING_DEFS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/lsx/base.h"
#include "simdjson/lsx/simd.h"
#include "simdjson/lsx/bitmanipulation.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

using namespace simd;
namespace dp = std::simd;

// The string-parsing scan processes 32 bytes at a time with a std::simd vector
// (a half-block); no simd8 wrapper. Uses the kernel's free functions. (The old LSX
// 64-bit bitmask simd8x64 path is dropped in favor of the proven x86 std::simd path.)
using sp_vec = dp::vec<uint8_t, 32>;

struct backslash_and_quote {
public:
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_inline backslash_and_quote copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_quote_first() { return ((bs_bits - 1) & quote_bits) != 0; }
  simdjson_inline bool has_backslash() { return ((quote_bits - 1) & bs_bits) != 0; }
  simdjson_inline int quote_index() { return trailing_zeroes(quote_bits); }
  simdjson_inline int backslash_index() { return trailing_zeroes(bs_bits); }

  uint32_t bs_bits;
  uint32_t quote_bits;
}; // struct backslash_and_quote

simdjson_inline backslash_and_quote backslash_and_quote::copy_and_find(const uint8_t *src, uint8_t *dst) {
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "backslash and quote finder must process fewer than SIMDJSON_PADDING bytes");
  sp_vec v = dp::unchecked_load<sp_vec>(src, 32);
  dp::unchecked_store(v, dst, 32);
  return {
      uint32_t(to_bitmask(v == sp_vec(uint8_t('\\')))), // bs_bits
      uint32_t(to_bitmask(v == sp_vec(uint8_t('"')))),  // quote_bits
  };
}

struct escaping {
  static constexpr uint32_t BYTES_PROCESSED = 32;
  simdjson_inline static escaping copy_and_find(const uint8_t *src, uint8_t *dst);

  simdjson_inline bool has_escape() { return escape_bits != 0; }
  simdjson_inline int escape_index() { return trailing_zeroes(escape_bits); }

  uint64_t escape_bits;
}; // struct escaping

simdjson_inline escaping escaping::copy_and_find(const uint8_t *src, uint8_t *dst) {
  static_assert(SIMDJSON_PADDING >= (BYTES_PROCESSED - 1), "escaping finder must process fewer than SIMDJSON_PADDING bytes");
  sp_vec v = dp::unchecked_load<sp_vec>(src, 32);
  dp::unchecked_store(v, dst, 32);
  auto is_quote = (v == sp_vec(uint8_t('"')));
  auto is_backslash = (v == sp_vec(uint8_t('\\')));
  auto is_control = (v < sp_vec(uint8_t(32)));
  return { to_bitmask(is_backslash | is_quote | is_control) };
}

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_LSX_STRINGPARSING_DEFS_H
