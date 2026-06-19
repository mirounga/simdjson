#ifndef SIMDJSON_STDSIMD_IMPLEMENTATION_H
#define SIMDJSON_STDSIMD_IMPLEMENTATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/stdsimd/base.h"
#include "simdjson/implementation.h"
#include "simdjson/internal/instruction_set.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace stdsimd {

/**
 * @private
 * Portable std::simd backend. Stage 1 of the migration implements only
 * validate_utf8 via std::simd; parsing/minify currently delegate to the
 * fallback path by returning UNSUPPORTED_ARCHITECTURE. We compile the SIMD
 * region for AVX2 (mirroring haswell) for apples-to-apples benchmarking.
 */
class implementation final : public simdjson::implementation {
public:
  simdjson_inline implementation() : simdjson::implementation(
      "stdsimd",
      "Portable C++26 std::simd (AVX2 width)",
      internal::instruction_set::AVX2
  ) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace stdsimd
} // namespace simdjson

#endif // SIMDJSON_STDSIMD_IMPLEMENTATION_H
