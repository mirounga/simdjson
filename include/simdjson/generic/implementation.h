// Generic per-implementation `implementation` class (dispatch entry). NO include guard: it is
// re-expanded once per implementation (implementation.cpp declares several with different
// SIMDJSON_IMPLEMENTATION values, each a distinct namespace). The constructor is declared here and
// DEFINED out-of-line in src/implementation.cpp per variant (baseline TU, so the ctor + its
// name/description/required-instruction-set metadata run safely on any host); the virtual method
// bodies are defined in src/<isa>.cpp (compiled at that variant's -march).
#include "simdjson/generic/impl_fwd.h"
#include "simdjson/implementation.h"
#include "simdjson/internal/dom_parser_implementation.h"

#include <memory>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

class implementation final : public simdjson::implementation {
public:
  implementation();
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
