// Icelake (AVX-512) native gap fills for the shared std::simd kernel (64-lane block).
//
// Implements the `native::` customization point the kernel delegates to
// (lookup_16, prev<N>, compress) with AVX-512 intrinsics. The 64-byte `block` is
// a single __m512i (bridged via std::bit_cast), so every gap is one register op:
// vpshufb for lookup_16, valignr+vpermt2q for the cross-lane prev<N>, and the
// native vpcompressb for compress. Included by icelake/simd.h between simd_block.h and simd_kernel.h.

#ifndef SIMDJSON_ICELAKE_SIMD_GAPS_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_ICELAKE_SIMD_GAPS_H
#include "simdjson/icelake/intrinsics.h"           // <immintrin.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {
namespace native {

simdjson_inline __m512i to_m512(const block v) { return std::bit_cast<__m512i>(v); }
simdjson_inline block from_m512(const __m512i r) { return std::bit_cast<block>(r); }

// 16-entry byte lookup: AVX-512 vpshufb (table broadcast to all four 128-bit
// lanes; pshufb gives low-nibble-select / high-bit-zero semantics).
simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  const __m128i t128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(table));
  const __m512i tbl = _mm512_broadcast_i32x4(t128);
  return from_m512(_mm512_shuffle_epi8(tbl, to_m512(value)));
}

// prev<N>: cross-512-bit-lane byte shift. valignr is per-128-bit-lane, so we first
// build [last 16 bytes of prev_chunk | first 48 bytes of value] with a qword
// permute, then alignr against value. (Mirrors the bespoke icelake simd8 prev.)
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  const __m512i cur = to_m512(value), prv = to_m512(prev_chunk);
  constexpr int shift = 16 - N;
  const __m512i stitched = _mm512_permutex2var_epi64(prv, _mm512_set_epi64(13, 12, 11, 10, 9, 8, 7, 6), cur);
  return from_m512(_mm512_alignr_epi8(cur, stitched, shift));
}

// Compact out bytes whose mask bit is 1; writes 64 - popcount(mask) bytes.
// Native vpcompressb (we avoid _mm512_mask_compressstoreu_epi8 — broken/slow on Zen4).
simdjson_inline void compress(const block value, uint64_t mask, uint8_t* output) {
  __m512i compressed = _mm512_maskz_compress_epi8(~mask, to_m512(value));
  _mm512_storeu_si512(reinterpret_cast<__m512i*>(output), compressed);
}

} // namespace native
} // namespace simd
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_ICELAKE_SIMD_GAPS_H
