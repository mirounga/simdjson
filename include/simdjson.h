#ifndef SIMDJSON_H
#define SIMDJSON_H

/**
 * @mainpage
 *
 * Check the [README.md](https://github.com/simdjson/simdjson/blob/master/README.md#simdjson--parsing-gigabytes-of-json-per-second).
 *
 * Sample code. See https://github.com/simdjson/simdjson/blob/master/doc/basics.md for more examples.

    #include "simdjson.h"

    int main(void) {
      // load from `twitter.json` file:
      simdjson::dom::parser parser;
      simdjson::dom::element tweets = parser.load("twitter.json");
      std::cout << tweets["search_metadata"]["count"] << " results." << std::endl;

      // Parse and iterate through an array of objects
      auto abstract_json = R"( [
        {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
        {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
        ] )"_padded;

      for (simdjson::dom::object obj : parser.parse(abstract_json)) {
        for(const auto key_value : obj) {
          cout << "key: " << key_value.key << " : ";
          simdjson::dom::object innerobj = key_value.value;
          cout << "a: " << double(innerobj["a"]) << ", ";
          cout << "b: " << double(innerobj["b"]) << ", ";
          cout << "c: " << int64_t(innerobj["c"]) << endl;
        }
      }
    }
 */

#include "simdjson/common_defs.h"

// This provides the public API for simdjson.
// DOM and ondemand are amalgamated separately, in simdjson.h
#include "simdjson/simdjson_version.h"

#include "simdjson/base.h"

#include "simdjson/error.h"
#include "simdjson/error-inl.h"
#include "simdjson/implementation.h"
#include "simdjson/minify.h"
#include "simdjson/padded_string.h"
#include "simdjson/padded_string-inl.h"
#include "simdjson/padded_string_view.h"
#include "simdjson/padded_string_view-inl.h"

#if SIMDJSON_HEADER_ONLY
// Header-only: instantiate the builtin backend's engine (and its concrete
// SIMDJSON_BUILTIN_IMPLEMENTATION::dom_parser_implementation) BEFORE dom.h / ondemand.h, whose
// parser::allocate() constructs it directly instead of dispatching through the (absent) library.
#include "simdjson/builtin.h"
namespace simdjson {
// The free-function entry points normally live in the fat library (src/implementation.cpp) and
// dispatch at runtime. Header-only has no library and no dispatch, so run the builtin engine
// directly here — mirroring implementation::minify / implementation::validate_utf8 in each backend.
simdjson_warn_unused inline error_code minify(const char *buf, size_t len, char *dst, size_t &dst_len) noexcept {
  return SIMDJSON_BUILTIN_IMPLEMENTATION::stage1::json_minifier::minify<SIMDJSON_STAGE1_STEP>(
      reinterpret_cast<const uint8_t *>(buf), len, reinterpret_cast<uint8_t *>(dst), dst_len);
}
simdjson_warn_unused inline bool validate_utf8(const char *buf, size_t len) noexcept {
  return SIMDJSON_BUILTIN_IMPLEMENTATION::stage1::generic_validate_utf8(buf, len);
}
} // namespace simdjson
#endif

#include "simdjson/dom.h"
#include "simdjson/builder.h"
#include "simdjson/ondemand.h"
#include "simdjson/convert.h"
#include "simdjson/convert-inl.h"

// Compile-time JSON parsing (C++26 P2996 reflection)
#include "simdjson/compile_time_json.h"
#include "simdjson/compile_time_json-inl.h"

#endif // SIMDJSON_H
