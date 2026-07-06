// Westmere (SSE4.2) native gap fills for the shared std::simd kernel (64-lane block).
//
// Implements the `native::` customization point the kernel delegates to
// (lookup_16, prev<N>, compress) with SSE intrinsics. The 64-byte `block` is split
// into four 16-byte quarters (std::simd::chunk) bridged to `__m128i` via
// std::bit_cast; each SSE op runs per quarter and the quarters are recombined with
// std::simd::cat. Included by westmere/simd.h between simd_block.h and simd_kernel.h.

#ifndef SIMDJSON_WESTMERE_SIMD_GAPS_H
#define SIMDJSON_WESTMERE_SIMD_GAPS_H
#include "simdjson/generic/intrinsics.h"          // <emmintrin.h> etc.
#include "simdjson/internal/simdprune_tables.h"    // thintable_epi8, ...

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace simd {
namespace native {

using quarter = dp::vec<uint8_t, 16>; // one SSE register

simdjson_inline __m128i to_m128(const quarter q) { return std::bit_cast<__m128i>(q); }
simdjson_inline quarter from_m128(const __m128i r) { return std::bit_cast<quarter>(r); }

// 16-entry byte lookup: SSE vpshufb per 16-byte quarter (low-nibble-select /
// high-bit-zero semantics).
simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  const __m128i tbl = _mm_loadu_si128(reinterpret_cast<const __m128i*>(table));
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  return dp::cat(
      from_m128(_mm_shuffle_epi8(tbl, to_m128(c0))),
      from_m128(_mm_shuffle_epi8(tbl, to_m128(c1))),
      from_m128(_mm_shuffle_epi8(tbl, to_m128(c2))),
      from_m128(_mm_shuffle_epi8(tbl, to_m128(c3))));
}

// prev<N>: cross-64-byte byte shift built from four 16-byte alignr steps, each
// pulling the carry from the preceding 16-byte quarter (quarter -1 is the last
// quarter of prev_chunk).
template <int N>
simdjson_inline __m128i prev16(const __m128i cur, const __m128i prv) {
  return _mm_alignr_epi8(cur, prv, 16 - N);
}
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  auto [p0, p1, p2, p3] = dp::chunk<quarter>(prev_chunk);
  (void)p0; (void)p1; (void)p2;
  return dp::cat(
      from_m128(prev16<N>(to_m128(c0), to_m128(p3))),
      from_m128(prev16<N>(to_m128(c1), to_m128(c0))),
      from_m128(prev16<N>(to_m128(c2), to_m128(c1))),
      from_m128(prev16<N>(to_m128(c3), to_m128(c2))));
}

// One 16-byte compress step (thintable/pshufb), writing 16 - popcount(mask16) bytes.
simdjson_inline int compress16(__m128i v, uint16_t mask, uint8_t* output) {
  using simdjson::internal::thintable_epi8;
  using simdjson::internal::BitsSetTable256mul2;
  using simdjson::internal::pshufb_combine_table;
  uint8_t mask1 = uint8_t(mask), mask2 = uint8_t(mask >> 8);
  __m128i shufmask = _mm_set_epi64x(thintable_epi8[mask2], thintable_epi8[mask1]);
  shufmask = _mm_add_epi8(shufmask, _mm_set_epi32(0x08080808, 0x08080808, 0, 0));
  __m128i pruned = _mm_shuffle_epi8(v, shufmask);
  int pop1 = BitsSetTable256mul2[mask1];
  __m128i compactmask = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pshufb_combine_table + pop1 * 8));
  __m128i answer = _mm_shuffle_epi8(pruned, compactmask);
  _mm_storeu_si128(reinterpret_cast<__m128i*>(output), answer);
  return 16 - __builtin_popcount(mask);
}

// Compact out bytes whose mask bit is 1, over the 64-byte block: four 16-byte
// thintable compress steps.
simdjson_inline void compress(const block value, uint64_t mask, uint8_t* output) {
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  uint8_t* out = output;
  out += compress16(to_m128(c0), uint16_t(mask),       out);
  out += compress16(to_m128(c1), uint16_t(mask >> 16), out);
  out += compress16(to_m128(c2), uint16_t(mask >> 32), out);
  out += compress16(to_m128(c3), uint16_t(mask >> 48), out);
}

} // namespace native
} // namespace simd
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_WESTMERE_SIMD_GAPS_H
