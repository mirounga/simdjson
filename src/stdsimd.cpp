#ifndef SIMDJSON_SRC_STDSIMD_CPP
#define SIMDJSON_SRC_STDSIMD_CPP

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <simdjson/stdsimd.h>
#include <simdjson/stdsimd/implementation.h>

#include <simdjson/stdsimd/begin.h>
// Stage 5: the full stage-1 (structural indexer) and stage-2 (tape builder,
// string/number parsing) are compiled for this backend, reusing the generic
// algorithms unchanged on top of the std::simd simd8 primitives. DOM parsing,
// minify, and UTF-8 validation are all functional.
#include <generic/amalgamated.h>
#include <generic/stage1/amalgamated.h>
#include <generic/stage2/amalgamated.h>

//
// Stage 1
//
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

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

simdjson_inline bool is_ascii(const simd8x64<uint8_t>& input) {
  return input.reduce_or().is_ascii();
}

simdjson_inline simd8<uint8_t> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3) {
  simd8<uint8_t> is_third_byte  = prev2.saturating_sub(simd8<uint8_t>(uint8_t(0xe0u - 0x80))); // Only 111_____ will be >= 0x80
  simd8<uint8_t> is_fourth_byte = prev3.saturating_sub(simd8<uint8_t>(uint8_t(0xf0u - 0x80))); // Only 1111____ will be >= 0x80
  return is_third_byte | is_fourth_byte;
}

// Identifies structural characters (comma, colon, braces, brackets) and ASCII
// whitespace ('\r','\n','\t',' '). Mirrors the haswell classify but expressed via
// the std::simd simd8 lookup_16 (4-bit table lookup) + compare instead of raw
// pshufb. See the haswell backend for the table-design rationale.
simdjson_inline json_character_block json_character_block::classify(const simd::simd8x64<uint8_t>& in) {
  const simd8<uint8_t> c0 = in.chunks[0];
  const simd8<uint8_t> c1 = in.chunks[1];

  const simd8<uint8_t> ws0 = c0.lookup_16<uint8_t>(' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100);
  const simd8<uint8_t> ws1 = c1.lookup_16<uint8_t>(' ', 100, 100, 100, 17, 100, 113, 2, 100, '\t', '\n', 112, 100, '\r', 100, 100);
  const uint64_t whitespace =
      uint64_t(uint32_t((c0 == ws0).to_bitmask())) |
      (uint64_t(uint32_t((c1 == ws1).to_bitmask())) << 32);

  const simd8<uint8_t> op0 = c0.lookup_16<uint8_t>(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{', ',', '}', 0, 0);
  const simd8<uint8_t> op1 = c1.lookup_16<uint8_t>(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ':', '{', ',', '}', 0, 0);
  const simd8<uint8_t> curl0 = c0 | uint8_t(0x20);
  const simd8<uint8_t> curl1 = c1 | uint8_t(0x20);
  const uint64_t op =
      uint64_t(uint32_t((curl0 == op0).to_bitmask())) |
      (uint64_t(uint32_t((curl1 == op1).to_bitmask())) << 32);

  return { whitespace, op };
}

} // unnamed namespace

//
// Implementation-specific overrides
//
simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return stdsimd::stage1::json_minifier::minify<128>(buf, len, dst, dst_len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return stdsimd::stage1::json_structural_indexer::index<128>(_buf, _len, *this, streaming);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return stdsimd::stage1::generic_validate_utf8(buf,len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_NO_SANITIZE_MEMORY
simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool replacement_char) const noexcept {
  return stdsimd::stringparsing::parse_string(src, dst, replacement_char);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return stdsimd::stringparsing::parse_wobbly_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#include <simdjson/stdsimd/end.h>

#endif // SIMDJSON_SRC_STDSIMD_CPP
