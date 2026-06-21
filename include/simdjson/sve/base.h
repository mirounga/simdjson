#ifndef SIMDJSON_SVE_BASE_H
#define SIMDJSON_SVE_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
/**
 * Implementation for ARM SVE2 (experimental, std::simd kernel).
 */
namespace sve {

class implementation;

} // namespace sve
} // namespace simdjson

#endif // SIMDJSON_SVE_BASE_H
