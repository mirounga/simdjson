#ifndef SIMDJSON_SRC_LSX_CPP
#define SIMDJSON_SRC_LSX_CPP

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <simdjson/lsx.h>
#include <simdjson/lsx/implementation.h>

#include <simdjson/lsx/begin.h>
#include <generic/amalgamated.h>
#include <generic/stage1/amalgamated.h>
#include <generic/stage2/amalgamated.h>

//
// Stage 1
//
namespace simdjson {
namespace lsx {

simdjson_warn_unused error_code implementation::create_dom_parser_implementation(
  size_t capacity,
  size_t max_depth,
  std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  dst.reset( new (std::nothrow) dom_parser_implementation() );
  if (!dst) { return MEMALLOC; }
  if (auto err = dst->set_capacity(capacity))
    return err;
  if (auto err = dst->set_max_depth(max_depth))
    return err;
  return SUCCESS;
}

namespace {

using namespace simd;

// Identifies structural characters (comma, colon, braces, brackets) and ASCII
// whitespace ('\r','\n','\t',' '). Operates on the whole 64-byte block via the
// kernel's lookup_16 (LSX pshufb16 gap fill, pshufb semantics). The bespoke
// two-table 5-bit LSX classify is replaced by the shared classify (byte-identical
// to the x86 backends). See the haswell backend for the table-design rationale.
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
} // namespace lsx
} // namespace simdjson

//
// Stage 2
//

//
// Implementation-specific overrides
//
namespace simdjson {
namespace lsx {

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return lsx::stage1::json_minifier::minify<64>(buf, len, dst, dst_len);
}

simdjson_flatten simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return lsx::stage1::json_structural_indexer::index<64>(buf, len, *this, streaming);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return lsx::stage1::generic_validate_utf8(buf,len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_NO_SANITIZE_MEMORY
simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool allow_replacement) const noexcept {
  return lsx::stringparsing::parse_string(src, dst, allow_replacement);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return lsx::stringparsing::parse_wobbly_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace lsx
} // namespace simdjson

#include <simdjson/lsx/end.h>

#endif // SIMDJSON_SRC_LSX_CPP
