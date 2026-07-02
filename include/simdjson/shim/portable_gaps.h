// Portable gap fills for the shared std::simd kernel (fallback backend).
//
// The fallback backend rides the SAME std::simd kernel as the SIMD backends, but targets no
// SIMD ISA: the compiler scalarizes std::simd's 64-lane vector into plain scalar code. The three
// primitives std::simd can't express portably (lookup_16 / prev<N> / compress) are implemented
// here as straight C++ loops over the 64 bytes (bridged to/from `block` with unchecked_store /
// unchecked_load). Semantics are identical to the x86 gap fills, so classify/stage-1/stage-2 are
// byte-for-byte the same as the SIMD backends -- just scalarized. Selected by fallback/simd.h
// between simd_block.h and simd_kernel.h.

#ifndef SIMDJSON_FALLBACK_SIMD_GAPS_H
#define SIMDJSON_FALLBACK_SIMD_GAPS_H
#include "simdjson/base.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {
namespace native {

// 16-entry byte lookup, x86 pshufb semantics: out[i] = (b & 0x80) ? 0 : table[b & 0x0F].
simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  uint8_t in[64], out[64];
  dp::unchecked_store(value, in, 64);
  for (int i = 0; i < 64; i++) {
    const uint8_t b = in[i];
    out[i] = (b & 0x80) ? 0 : table[b & 0x0F];
  }
  return dp::unchecked_load<block>(out, 64);
}

// prev<N>: shift the 64-byte vector right by N bytes, pulling the last N bytes of prev_chunk
// into the front -- the window of {prev_chunk, value} starting at byte 64-N.
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  uint8_t cur[64], prv[64], out[64];
  dp::unchecked_store(value, cur, 64);
  dp::unchecked_store(prev_chunk, prv, 64);
  for (int i = 0; i < N; i++) { out[i] = prv[64 - N + i]; }
  for (int i = N; i < 64; i++) { out[i] = cur[i - N]; }
  return dp::unchecked_load<block>(out, 64);
}

// Compact out bytes whose mask bit is 1; writes the bytes whose mask bit is 0, in order.
// The kernel wrapper returns the count (64 - popcount(mask)).
simdjson_inline void compress(const block value, uint64_t mask, uint8_t* output) {
  uint8_t in[64];
  dp::unchecked_store(value, in, 64);
  uint8_t* out = output;
  for (int i = 0; i < 64; i++) {
    if (!((mask >> i) & 1)) { *out++ = in[i]; }
  }
}

} // namespace native
} // namespace simd
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_FALLBACK_SIMD_GAPS_H
