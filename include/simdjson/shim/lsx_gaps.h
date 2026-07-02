// LSX (LoongArch 128-bit SIMD) native gap fills for the shared std::simd kernel (64-lane block).
//
// Implements the `native::` customization point the kernel delegates to
// (lookup_16, prev<N>, compress) with LSX intrinsics. The 64-byte `block` is split
// into four 16-byte quarters (std::simd::chunk) bridged to `__m128i` via
// std::bit_cast; each LSX op runs per quarter and the quarters are recombined with
// std::simd::cat. Included by lsx/simd.h between simd_block.h and simd_kernel.h.

#ifndef SIMDJSON_LSX_SIMD_GAPS_H
#define SIMDJSON_LSX_SIMD_GAPS_H
#include "simdjson/generic/intrinsics.h"                // <lsxintrin.h>
#include "simdjson/internal/simdprune_tables.h"     // thintable_epi8, ...

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {
namespace native {

using quarter = dp::vec<uint8_t, 16>; // one LSX register

simdjson_inline __m128i to_lsx(const quarter q) { return std::bit_cast<__m128i>(q); }
simdjson_inline quarter from_lsx(const __m128i r) { return std::bit_cast<quarter>(r); }

// 16-entry byte lookup with x86 pshufb semantics: result = (b & 0x80) ? 0 : table[b & 0x0F].
// LSX __lsx_vshuf_b uses all 8 bits of the index (result is 0 for index >= 32), so we mask to
// the low nibble and then zero lanes whose high bit was set, to match the kernel's contract.
// Operand order mirrors the bespoke lsx lookup_16: __lsx_vshuf_b(tbl, tbl, idx).
simdjson_inline __m128i pshufb16(const __m128i tbl, const __m128i value) {
  const __m128i idx = __lsx_vand_v(value, __lsx_vreplgr2vr_b(0x0F));
  const __m128i res = __lsx_vshuf_b(tbl, tbl, idx);
  // Zero lanes where the high bit of the original value is set (pshufb semantics).
  // __lsx_vmskltz_b gives 0xFF per lane where the signed byte < 0 (i.e. bit7 set);
  // use bitwise-and-not: res & ~hi_mask.
  const __m128i hi  = __lsx_vmskltz_b(value);           // 0xFF where high bit set
  // hi from vmskltz_b is a 16-bit mask in lane 0; need per-byte mask instead.
  // Use saturating shift: srai to broadcast bit7 to all bits of each byte.
  const __m128i sign_broadcast = __lsx_vsrai_b(value, 7); // 0xFF where hi-bit set, else 0x00
  return __lsx_vandn_v(sign_broadcast, res);             // res & ~sign_broadcast
}
simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  const __m128i tbl = __lsx_vld(reinterpret_cast<const __m128i*>(table), 0);
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  return dp::cat(
      from_lsx(pshufb16(tbl, to_lsx(c0))),
      from_lsx(pshufb16(tbl, to_lsx(c1))),
      from_lsx(pshufb16(tbl, to_lsx(c2))),
      from_lsx(pshufb16(tbl, to_lsx(c3))));
}

// prev<N>: cross-64-byte byte shift built from four 16-byte steps, each pulling the
// carry from the preceding 16-byte quarter (quarter -1 is the last quarter of prev_chunk).
// Mirrors bespoke lsx prev: __lsx_vor_v(__lsx_vbsll_v(cur, N), __lsx_vbsrl_v(prv, 16-N)).
// __lsx_vbsll_v shifts the whole vector left by N bytes (zeroing from the right);
// __lsx_vbsrl_v shifts the whole vector right by (16-N) bytes (zeroing from the left).
template <int N>
simdjson_inline __m128i prev16(const __m128i cur, const __m128i prv) {
  return __lsx_vor_v(__lsx_vbsll_v(cur, N), __lsx_vbsrl_v(prv, 16 - N));
}
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  auto [p0, p1, p2, p3] = dp::chunk<quarter>(prev_chunk);
  (void)p0; (void)p1; (void)p2;
  return dp::cat(
      from_lsx(prev16<N>(to_lsx(c0), to_lsx(p3))),
      from_lsx(prev16<N>(to_lsx(c1), to_lsx(c0))),
      from_lsx(prev16<N>(to_lsx(c2), to_lsx(c1))),
      from_lsx(prev16<N>(to_lsx(c3), to_lsx(c2))));
}

// One 16-byte compress step (thintable + two __lsx_vshuf_b), writing 16 - popcount(mask16) bytes.
// Mirrors bespoke lsx simd8::compress operand order: __lsx_vshuf_b(src, src, shuf).
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

// Compact out bytes whose mask bit is 1, over the 64-byte block: four 16-byte steps.
simdjson_inline void compress(const block value, uint64_t mask, uint8_t* output) {
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  uint8_t* out = output;
  out += compress16(to_lsx(c0), uint16_t(mask),       out);
  out += compress16(to_lsx(c1), uint16_t(mask >> 16), out);
  out += compress16(to_lsx(c2), uint16_t(mask >> 32), out);
  out += compress16(to_lsx(c3), uint16_t(mask >> 48), out);
}

} // namespace native
} // namespace simd
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_LSX_SIMD_GAPS_H
