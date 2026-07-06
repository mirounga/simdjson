// Header-only acceptance test.
//
// This translation unit includes ONLY <simdjson.h> and is compiled with
// -D SIMDJSON_HEADER_ONLY=1, linking NO simdjson library. It exercises both the
// ondemand and DOM front-ends plus the free minify / validate_utf8 entry points to
// prove that a header-only consumer needs nothing to link: the builtin backend's
// engine + the inline constexpr tables are compiled straight into this TU.
#include "simdjson.h"

#include <cstdio>
#include <cstring>
#include <string>

using namespace simdjson;

static int fails = 0;
static void check(bool ok, const char *what) {
  std::printf("%-28s %s\n", what, ok ? "OK" : "FAIL");
  if (!ok) fails++;
}

int main() {
  // ondemand: escaped string + double
  {
    ondemand::parser parser;
    padded_string json = R"({"a":["hello\nworld", 3.14159, 12345], "b":true})"_padded;
    ondemand::document doc = parser.iterate(json);
    std::string_view s;
    check(!doc["a"].at(0).get_string().get(s) && std::string(s) == "hello\nworld", "ondemand escaped string");
    double d;
    check(!doc["a"].at(1).get_double().get(d) && d > 3.14 && d < 3.15, "ondemand double");
  }
  // DOM: double + escaped string
  {
    dom::parser parser;
    padded_string json = R"([1,2,3,"x\ty",2.71828])"_padded;
    dom::element doc = parser.parse(json);
    double e = doc.at(4);
    std::string_view sv = doc.at(3);
    check(e > 2.71 && e < 2.72, "dom double");
    check(std::string(sv) == "x\ty", "dom escaped string");
  }
  // Free functions: minify + validate_utf8
  {
    const char *in = "{ \"x\" : 1 , \"y\" : [ 2 , 3 ] }";
    char out[64];
    size_t outlen = 0;
    check(!simdjson::minify(in, std::strlen(in), out, outlen) &&
              std::string(out, outlen) == "{\"x\":1,\"y\":[2,3]}",
          "minify");
    check(simdjson::validate_utf8(in, std::strlen(in)), "validate_utf8");
  }

  std::printf("HEADER-ONLY ACCEPTANCE %s (impl=%s)\n",
              fails == 0 ? "PASS" : "FAIL",
              SIMDJSON_STRINGIFY(SIMDJSON_BUILTIN_IMPLEMENTATION));
  return fails == 0 ? 0 : 1;
}
