// ARM64 (NEON) native gap fills for the shared std::simd kernel (64-lane block).
//
// Implements the `native::` customization point the kernel delegates to
// (lookup_16, prev<N>, compress) with NEON intrinsics. The 64-byte `block` is split
// into four 16-byte quarters (std::simd::chunk) bridged to `uint8x16_t` via
// std::bit_cast; each NEON op runs per quarter and the quarters are recombined with
// std::simd::cat. Included by arm64/simd.h between simd_block.h and simd_kernel.h.

#ifndef SIMDJSON_ARM64_SIMD_GAPS_H
#define SIMDJSON_ARM64_SIMD_GAPS_H
#include "simdjson/generic/intrinsics.h"             // <arm_neon.h>
#include "simdjson/internal/simdprune_tables.h"    // thintable_epi8, ...

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace simd {
namespace native {

using quarter = dp::vec<uint8_t, 16>; // one NEON register

simdjson_inline uint8x16_t to_neon(const quarter q) { return std::bit_cast<uint8x16_t>(q); }
simdjson_inline quarter from_neon(const uint8x16_t r) { return std::bit_cast<quarter>(r); }

// 16-entry byte lookup with x86 pshufb semantics: result = (b & 0x80) ? 0 : table[b & 0x0F].
// NEON vqtbl1q_u8 indexes the full byte (0 for index >= 16), so we mask to the low nibble
// and then zero the lanes whose high bit was set, to match the kernel's contract.
simdjson_inline uint8x16_t pshufb16(const uint8x16_t tbl, const uint8x16_t value) {
  const uint8x16_t idx = vandq_u8(value, vdupq_n_u8(0x0F));
  const uint8x16_t res = vqtbl1q_u8(tbl, idx);
  const uint8x16_t hi  = vcgeq_u8(value, vdupq_n_u8(0x80)); // 0xFF where high bit set
  return vbicq_u8(res, hi);                                  // res & ~hi
}
simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  const uint8x16_t tbl = vld1q_u8(table);
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  return dp::cat(
      from_neon(pshufb16(tbl, to_neon(c0))),
      from_neon(pshufb16(tbl, to_neon(c1))),
      from_neon(pshufb16(tbl, to_neon(c2))),
      from_neon(pshufb16(tbl, to_neon(c3))));
}

// prev<N>: cross-64-byte byte shift built from four 16-byte vext steps, each pulling the
// carry from the preceding 16-byte quarter (quarter -1 is the last quarter of prev_chunk).
template <int N>
simdjson_inline uint8x16_t prev16(const uint8x16_t cur, const uint8x16_t prv) {
  return vextq_u8(prv, cur, 16 - N);
}
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  auto [p0, p1, p2, p3] = dp::chunk<quarter>(prev_chunk);
  (void)p0; (void)p1; (void)p2;
  return dp::cat(
      from_neon(prev16<N>(to_neon(c0), to_neon(p3))),
      from_neon(prev16<N>(to_neon(c1), to_neon(c0))),
      from_neon(prev16<N>(to_neon(c2), to_neon(c1))),
      from_neon(prev16<N>(to_neon(c3), to_neon(c2))));
}

// One 16-byte compress step (thintable + two vqtbl1q_u8), writing 16 - popcount(mask16) bytes.
simdjson_inline int compress16(uint8x16_t v, uint16_t mask, uint8_t* output) {
  using simdjson::internal::thintable_epi8;
  using simdjson::internal::BitsSetTable256mul2;
  using simdjson::internal::pshufb_combine_table;
  uint8_t mask1 = uint8_t(mask), mask2 = uint8_t(mask >> 8);
  uint64x2_t shufmask64 = {thintable_epi8[mask1], thintable_epi8[mask2]};
  uint8x16_t shufmask = vreinterpretq_u8_u64(shufmask64);
  const uint8x16_t inc = {0, 0, 0, 0, 0, 0, 0, 0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};
  shufmask = vaddq_u8(shufmask, inc);
  uint8x16_t pruned = vqtbl1q_u8(v, shufmask);
  int pop1 = BitsSetTable256mul2[mask1];
  uint8x16_t compactmask = vld1q_u8(reinterpret_cast<const uint8_t*>(pshufb_combine_table + pop1 * 8));
  uint8x16_t answer = vqtbl1q_u8(pruned, compactmask);
  vst1q_u8(reinterpret_cast<uint8_t*>(output), answer);
  return 16 - __builtin_popcount(mask);
}

// Compact out bytes whose mask bit is 1, over the 64-byte block: four 16-byte steps.
simdjson_inline void compress(const block value, uint64_t mask, uint8_t* output) {
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  uint8_t* out = output;
  out += compress16(to_neon(c0), uint16_t(mask),       out);
  out += compress16(to_neon(c1), uint16_t(mask >> 16), out);
  out += compress16(to_neon(c2), uint16_t(mask >> 32), out);
  out += compress16(to_neon(c3), uint16_t(mask >> 48), out);
}

} // namespace native
} // namespace simd
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_ARM64_SIMD_GAPS_H
