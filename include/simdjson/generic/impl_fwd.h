// Forward declaration of the per-implementation `implementation` class, in the namespace named
// by SIMDJSON_IMPLEMENTATION (declared by the includer). No include guard: it is re-expanded once
// per implementation (e.g. implementation.cpp declares several with different SIMDJSON_IMPLEMENTATION
// values). A repeated forward declaration in a distinct namespace is legal.
#include "simdjson/base.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
class implementation;
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
