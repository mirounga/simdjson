// Header-only definitions of the dom_parser_implementation vtable overrides + the shared classify /
// must_be_2_3_continuation helpers. `inline` so both the fat-library backend objects and a header-only
// (builtin) consumer define them in their own SIMDJSON_IMPLEMENTATION namespace. Include AFTER the
// stage1/stage2 engine headers.
//
// NOTE: the `implementation` dispatch-class virtuals (create_dom_parser_implementation / minify /
// validate_utf8) are deliberately NOT here — they stay non-inline in src/<isa>.cpp so that class's
// vtable is emitted at -march for the fat library. The header-only path never uses `implementation`
// (it constructs dom_parser_implementation directly), so it only needs these inline bodies.
#ifndef SIMDJSON_GENERIC_DOM_PARSER_IMPLEMENTATION_INL_H
#define SIMDJSON_GENERIC_DOM_PARSER_IMPLEMENTATION_INL_H
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/generic/json_character_block.h"

// Bytes indexed per structural-indexer iteration (perf tuning; any multiple of the 64-byte block is
// valid). Fat-library backends set this before including the umbrella (128 for haswell/icelake/fallback,
// 64 for the rest); a header-only builtin consumer gets the 128 default.
#ifndef SIMDJSON_STAGE1_STEP
#define SIMDJSON_STAGE1_STEP 128
#endif

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

namespace {

using namespace simd;

// Identifies structural characters (comma, colon, braces, brackets) and ASCII whitespace over the
// whole 64-byte block via the shared kernel's lookup_16 (native gap fill per backend, high-bit->0).
// Byte-identical across all backends. The [ ]/{ } pair is folded with | 0x20.
simdjson_inline json_character_block json_character_block::classify(const simd::block& in) {
  static const uint8_t whitespace_table[16] = {
    ' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100
  };
  static const uint8_t op_table[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{', ',', '}', 0, 0
  };
  const block ws = lookup_16(in, whitespace_table);
  const uint64_t whitespace = to_bitmask(in == ws);
  const block opl = lookup_16(in, op_table);
  const block curl = in | block(uint8_t(0x20));
  const uint64_t op = to_bitmask(curl == opl);
  return { whitespace, op };
}

simdjson_inline simd::block must_be_2_3_continuation(const simd::block prev2, const simd::block prev3) {
  block is_third_byte  = saturating_sub(prev2, block(uint8_t(0xe0u-0x80))); // Only 111_____ will be >= 0x80
  block is_fourth_byte = saturating_sub(prev3, block(uint8_t(0xf0u-0x80))); // Only 1111____ will be >= 0x80
  return is_third_byte | is_fourth_byte;
}

} // unnamed namespace

inline simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return SIMDJSON_IMPLEMENTATION::stage1::json_structural_indexer::index<SIMDJSON_STAGE1_STEP>(_buf, _len, *this, streaming);
}

inline simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

inline simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_NO_SANITIZE_MEMORY
inline simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool allow_replacement) const noexcept {
  return SIMDJSON_IMPLEMENTATION::stringparsing::parse_string(src, dst, allow_replacement);
}

inline simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return SIMDJSON_IMPLEMENTATION::stringparsing::parse_wobbly_string(src, dst);
}

inline simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_DOM_PARSER_IMPLEMENTATION_INL_H
