// Quick-start: use simdjson from C++ via the header-only On-Demand API.
//
// This is the drop-in path for C++26 consumers: include "simdjson.h" and link the prebuilt
// library for the small runtime (dispatch table, number formatting, error strings). The SIMD
// kernel is header-only and compiles at the consumer's own -march.
#include <iostream>
#include "simdjson.h"
using namespace simdjson;

int main(void) {
  auto json = "{\"search_metadata\":{\"count\":100}}"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  uint64_t count = doc["search_metadata"]["count"];
  std::cout << count << " results." << std::endl;
  return count == 100 ? 0 : 1;
}
