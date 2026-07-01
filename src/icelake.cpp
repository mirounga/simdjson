#ifndef SIMDJSON_SRC_ICELAKE_CPP
#define SIMDJSON_SRC_ICELAKE_CPP

#define SIMDJSON_IMPLEMENTATION icelake

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <simdjson/generic/umbrella.h>
#include <simdjson/generic/implementation.h>

// defining SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER allows us to provide our own bit_indexer::write
#define SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER

#include <simdjson/generic/begin.h>
#include <generic/amalgamated.h>
#include <generic/stage1/amalgamated.h>
#include <generic/stage2/amalgamated.h>

#undef SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER

//
// Stage 1
//

namespace simdjson {
namespace icelake {

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
// kernel's lookup_16 (AVX-512 vpshufb gap fill, high-bit->0). The [ ]/{ } pair is
// folded with | 0x20. See the haswell backend for the full table-design rationale.
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
} // namespace icelake
} // namespace simdjson

/**
 * We provide a custom version of bit_indexer::write using
 * naked intrinsics.
 * TODO: make this code more elegant.
 */
// Under GCC 12, the intrinsic _mm512_extracti32x4_epi32 may generate 'maybe uninitialized'.
// as a workaround, we disable warnings within the following function.
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
namespace simdjson { namespace icelake { namespace { namespace stage1 {
simdjson_inline void bit_indexer::write(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0) { return; }

    const __m512i indexes = _mm512_maskz_compress_epi8(bits, _mm512_set_epi32(
      0x3f3e3d3c, 0x3b3a3938, 0x37363534, 0x33323130,
      0x2f2e2d2c, 0x2b2a2928, 0x27262524, 0x23222120,
      0x1f1e1d1c, 0x1b1a1918, 0x17161514, 0x13121110,
      0x0f0e0d0c, 0x0b0a0908, 0x07060504, 0x03020100
    ));
    const __m512i start_index = _mm512_set1_epi32(idx);

    const auto count = count_ones(bits);
    __m512i t0 = _mm512_cvtepu8_epi32(_mm512_castsi512_si128(indexes));
    _mm512_storeu_si512(this->tail, _mm512_add_epi32(t0, start_index));

    if(count > 16) {
      const __m512i t1 = _mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(indexes, 1));
      _mm512_storeu_si512(this->tail + 16, _mm512_add_epi32(t1, start_index));
      if(count > 32) {
        const __m512i t2 = _mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(indexes, 2));
        _mm512_storeu_si512(this->tail + 32, _mm512_add_epi32(t2, start_index));
        if(count > 48) {
          const __m512i t3 = _mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(indexes, 3));
          _mm512_storeu_si512(this->tail + 48, _mm512_add_epi32(t3, start_index));
        }
      }
    }
    this->tail += count;
}
}}}}
SIMDJSON_POP_DISABLE_WARNINGS

//
// Stage 2
//

//
// Implementation-specific overrides
//
namespace simdjson {
namespace icelake {

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return icelake::stage1::json_minifier::minify<128>(buf, len, dst, dst_len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode streaming) noexcept {
  this->buf = _buf;
  this->len = _len;
  return icelake::stage1::json_structural_indexer::index<128>(_buf, _len, *this, streaming);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return icelake::stage1::generic_validate_utf8(buf,len);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_NO_SANITIZE_MEMORY
simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool replacement_char) const noexcept {
  return icelake::stringparsing::parse_string(src, dst, replacement_char);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return icelake::stringparsing::parse_wobbly_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace icelake
} // namespace simdjson

#include <simdjson/generic/end.h>

#endif // SIMDJSON_SRC_ICELAKE_CPP
