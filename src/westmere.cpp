#ifndef SIMDJSON_SRC_WESTMERE_CPP
#define SIMDJSON_SRC_WESTMERE_CPP

#define SIMDJSON_IMPLEMENTATION westmere
#define SIMDJSON_STAGE1_STEP 64

#include <base.h>

// The class declaration (before the umbrella defines its inline members) + the full generic engine +
// the inline dom_parser_implementation bodies. Compiled at this object's global -march.
#include <simdjson/generic/implementation.h>
#define SIMDJSON_BUILDING_IMPLEMENTATION 1
#include <simdjson/generic/umbrella.h>

// The 'implementation' dispatch-class virtuals stay non-inline here so this class's vtable is emitted
// at -march for the fat library (the header-only path never uses 'implementation').
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

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return stage1::json_minifier::minify<SIMDJSON_STAGE1_STEP>(buf, len, dst, dst_len);
}

simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return stage1::generic_validate_utf8(buf, len);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_WESTMERE_CPP
