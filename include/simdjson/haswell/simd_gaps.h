// Haswell (AVX2) native gap fills for the shared std::simd kernel (64-lane block).
//
// Implements the `native::` customization point the kernel delegates to
// (lookup_16, prev<N>, compress) with AVX2 intrinsics. The 64-byte `block` is
// split into two 32-byte halves (std::simd::chunk) bridged to `__m256i` via
// std::bit_cast; each AVX2 op runs per half and the halves are recombined with
// std::simd::cat. Included by haswell/simd.h between simd_block.h and simd_kernel.h.

#ifndef SIMDJSON_HASWELL_SIMD_GAPS_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_HASWELL_SIMD_GAPS_H
#include "simdjson/haswell/intrinsics.h"           // <immintrin.h>
#include "simdjson/internal/simdprune_tables.h"    // thintable_epi8, ...
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {
namespace native {

using half = dp::vec<uint8_t, 32>; // one AVX2 register

simdjson_inline __m256i to_m256(const half h) { return std::bit_cast<__m256i>(h); }
simdjson_inline half from_m256(const __m256i r) { return std::bit_cast<half>(r); }

// 16-entry byte lookup: AVX2 vpshufb per 32-byte half (table broadcast to both
// 128-bit lanes; pshufb gives low-nibble-select / high-bit-zero semantics).
simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  const __m128i t128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(table));
  const __m256i tbl = _mm256_broadcastsi128_si256(t128);
  auto [lo, hi] = dp::chunk<half>(value);
  return dp::cat(
      from_m256(_mm256_shuffle_epi8(tbl, to_m256(lo))),
      from_m256(_mm256_shuffle_epi8(tbl, to_m256(hi))));
}

// prev<N>: cross-64-byte-lane byte shift. Done as two chained 32-byte alignr:
//   hi' = alignr(in.hi, in.lo, 16-N) lanes ; lo' = alignr(in.lo, prev.hi, 16-N)
// using the haswell permute2x128+alignr idiom per 32-byte register.
template <int N>
simdjson_inline half prev32(const half cur, const half prv) {
  const __m256i c = to_m256(cur), p = to_m256(prv);
  return from_m256(_mm256_alignr_epi8(c, _mm256_permute2x128_si256(p, c, 0x21), 16 - N));
}
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  auto [lo, hi]   = dp::chunk<half>(value);
  auto [plo, phi] = dp::chunk<half>(prev_chunk);
  (void)plo;
  return dp::cat(prev32<N>(lo, phi), prev32<N>(hi, lo));
}

// One 16-byte (half-register) compress step (thintable/pshufb), writing
// 16 - popcount(mask16) bytes. `m128` is the 16 source bytes.
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
  uint8_t buf[64];
  dp::unchecked_store(value, buf, 64);
  uint8_t* out = output;
  for (int q = 0; q < 4; q++) {
    __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buf + q * 16));
    out += compress16(v, uint16_t(mask >> (q * 16)), out);
  }
}

} // namespace native
} // namespace simd
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_HASWELL_SIMD_GAPS_H
