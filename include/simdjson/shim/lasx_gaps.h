// LASX (LoongArch 256-bit ASX) native gap fills for the shared std::simd kernel (64-lane block).
//
// Implements the `native::` customization point the kernel delegates to
// (lookup_16, prev<N>, compress) with LASX intrinsics. The 64-byte `block` is
// split into two 32-byte halves (std::simd::chunk) bridged to `__m256i` via
// std::bit_cast; each LASX op runs per half and the halves are recombined with
// std::simd::cat. Included by lasx/simd.h between simd_block.h and simd_kernel.h.
//
// NOTE: GCC 16 std::simd has no LoongArch backend, so non-gap kernel ops are
// scalar on this architecture -- accepted. The gap fills here are still native LASX.

#ifndef SIMDJSON_LASX_SIMD_GAPS_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_LASX_SIMD_GAPS_H
#include "simdjson/generic/intrinsics.h"              // <lsxintrin.h> + <lasxintrin.h>
#include "simdjson/internal/simdprune_tables.h"    // thintable_epi8, BitsSetTable256mul2, pshufb_combine_table
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {
namespace native {

using half = dp::vec<uint8_t, 32>; // one LASX register (256-bit)

simdjson_inline __m256i to_lasx(const half h) { return std::bit_cast<__m256i>(h); }
simdjson_inline half from_lasx(const __m256i r) { return std::bit_cast<half>(r); }

// Broadcast a 16-byte table into both 128-bit lanes of a 256-bit register.
// Mirrors the bespoke lasx repeat_16 approach: store the 16 bytes twice into a
// 32-byte aligned buffer and reload as a 256-bit vector so both 128-bit lanes
// hold the same table. No direct __m128i -> __m256i lane-insert intrinsic exists
// in the public LASX header, so store+reload is the most portable option.
simdjson_inline __m256i broadcast_table(const uint8_t table[16]) {
  const __m128i t128 = __lsx_vld(reinterpret_cast<const __m128i*>(table), 0);
  alignas(32) uint8_t buf[32];
  __lsx_vst(t128, reinterpret_cast<__m128i*>(buf),      0);
  __lsx_vst(t128, reinterpret_cast<__m128i*>(buf + 16), 0);
  return __lasx_xvld(reinterpret_cast<const __m256i*>(buf), 0);
}

// 16-entry byte lookup with x86 pshufb semantics: result = (b & 0x80) ? 0 : table[b & 0x0F].
// LASX __lasx_xvshuf_b uses all 8 bits of the index (result is 0 for index >= 32 within
// each 128-bit lane), so we mask to the low nibble, shuffle, then zero lanes whose high bit
// was set, matching the kernel's contract. The 16-byte table is broadcast into both 128-bit
// lanes. Operand order mirrors bespoke lasx: __lasx_xvshuf_b(tbl, tbl, idx).
simdjson_inline __m256i pshufb32(const __m256i tbl, const __m256i value) {
  const __m256i idx = __lasx_xvand_v(value, __lasx_xvreplgr2vr_b(0x0F));
  const __m256i res = __lasx_xvshuf_b(tbl, tbl, idx);
  // Zero lanes where the high bit of the original value is set (pshufb semantics).
  // xvsrai_b broadcasts bit7 to all bits of each byte: 0xFF where hi-bit set, else 0x00.
  const __m256i sign_broadcast = __lasx_xvsrai_b(value, 7);
  return __lasx_xvandn_v(sign_broadcast, res); // res & ~sign_broadcast
}

simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  const __m256i tbl = broadcast_table(table);
  auto [lo, hi] = dp::chunk<half>(value);
  return dp::cat(
      from_lasx(pshufb32(tbl, to_lasx(lo))),
      from_lasx(pshufb32(tbl, to_lasx(hi))));
}

// prev32<N>: shift a 256-bit half left by N bytes, bringing in the last N bytes of `prv`
// as the carry. Mirrors the bespoke simd8::prev<N> logic exactly:
//   hi = xvbsll_v(cur, N)           -- shift cur left N bytes within each 128-bit lane
//   lo = xvbsrl_v(cur, 16-N)        -- shift cur right (16-N) bytes within each 128-bit lane
//   tmp = xvbsrl_v(prv, 16-N)       -- shift prv right (16-N) bytes within each 128-bit lane
//   lo = xvpermi_q(lo, tmp, 0x21)   -- lo.lo128 = tmp.hi128, lo.hi128 = lo.lo128
//   result = xvor_v(hi, lo)
// This stitches the cross-128-lane carry within the 256-bit register using the previous
// 256-bit chunk as the incoming bytes.
template <int N>
simdjson_inline half prev32(const half cur, const half prv) {
  const __m256i c = to_lasx(cur), p = to_lasx(prv);
  __m256i hi  = __lasx_xvbsll_v(c, N);
  __m256i lo  = __lasx_xvbsrl_v(c, 16 - N);
  __m256i tmp = __lasx_xvbsrl_v(p, 16 - N);
  lo = __lasx_xvpermi_q(lo, tmp, 0x21);
  return from_lasx(__lasx_xvor_v(hi, lo));
}

// prev<N>: cross-64-byte byte shift over the 2-chunk block.
//   new_lo = prev32<N>(value.lo, prev_chunk.hi)  -- lo chunk's carry comes from prev_chunk's hi
//   new_hi = prev32<N>(value.hi, value.lo)        -- hi chunk's carry comes from value's lo
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  auto [lo, hi]   = dp::chunk<half>(value);
  auto [plo, phi] = dp::chunk<half>(prev_chunk);
  (void)plo;
  return dp::cat(prev32<N>(lo, phi), prev32<N>(hi, lo));
}

// One 16-byte (half-register) compress step (thintable/pshufb), writing
// 16 - popcount(mask16) bytes. Uses LSX 128-bit intrinsics (mirrors lsx/simd_gaps.h).
simdjson_inline int compress16(__m128i v, uint16_t mask, uint8_t* output) {
  using simdjson::internal::thintable_epi8;
  using simdjson::internal::BitsSetTable256mul2;
  using simdjson::internal::pshufb_combine_table;
  uint8_t mask1 = uint8_t(mask), mask2 = uint8_t(mask >> 8);
  __m128i shufmask = {int64_t(thintable_epi8[mask1]), int64_t(thintable_epi8[mask2]) + 0x0808080808080808};
  __m128i pruned = __lsx_vshuf_b(v, v, shufmask);
  int pop1 = BitsSetTable256mul2[mask1];
  __m128i compactmask = __lsx_vldx(reinterpret_cast<void*>(reinterpret_cast<unsigned long>(pshufb_combine_table)), pop1 * 8);
  __m128i answer = __lsx_vshuf_b(pruned, pruned, compactmask);
  __lsx_vst(answer, reinterpret_cast<uint8_t*>(output), 0);
  return 16 - __builtin_popcount(mask);
}

// Compact out bytes whose mask bit is 1, over the 64-byte block: four 16-byte
// thintable compress steps. The block is stored to a local buffer, then processed
// in 16-byte pieces using LSX 128-bit ops (mirrors haswell/simd_gaps.h compress).
simdjson_inline void compress(const block value, uint64_t mask, uint8_t* output) {
  uint8_t buf[64];
  dp::unchecked_store(value, buf, 64);
  uint8_t* out = output;
  for (int q = 0; q < 4; q++) {
    __m128i v = __lsx_vld(reinterpret_cast<const __m128i*>(buf + q * 16), 0);
    out += compress16(v, uint16_t(mask >> (q * 16)), out);
  }
}

} // namespace native
} // namespace simd
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_LASX_SIMD_GAPS_H
