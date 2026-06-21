#ifndef SIMDJSON_SVE_IMPLEMENTATION_H
#define SIMDJSON_SVE_IMPLEMENTATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#include "simdjson/implementation.h"
#include "simdjson/internal/instruction_set.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace sve {

/**
 * @private
 */
class implementation final : public simdjson::implementation {
public:
  // NOTE (uncertainty): There is no instruction_set::SVE or instruction_set::SVE2 enum
  // value in internal/instruction_set.h.  We reuse instruction_set::NEON (0x1) because
  // SVE2 implies NEON on all real hardware.  The main agent should add a dedicated SVE2
  // flag if runtime dispatch is ever needed (e.g. 0x200000).
  simdjson_inline implementation() : simdjson::implementation(
    "sve",
    "ARM SVE2 (std::simd, experimental)",
    internal::instruction_set::NEON) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace sve
} // namespace simdjson

#endif // SIMDJSON_SVE_IMPLEMENTATION_H
