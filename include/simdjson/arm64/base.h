#ifndef SIMDJSON_ARM64_BASE_H
#define SIMDJSON_ARM64_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
/**
 * Implementation for NEON (ARMv8).
 */
namespace arm64 {

class implementation;

} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_BASE_H
