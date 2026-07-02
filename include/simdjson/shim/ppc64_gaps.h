// PPC64 (Altivec/VSX) native gap fills for the shared std::simd kernel (64-lane block).
//
// Implements the `native::` customization point the kernel delegates to
// (lookup_16, prev<N>, compress) with Altivec intrinsics. The 64-byte `block` is split
// into four 16-byte quarters (std::simd::chunk) bridged to `__vector unsigned char` via
// std::bit_cast; each Altivec op runs per quarter and the quarters are recombined with
// std::simd::cat. Included by ppc64/simd.h between simd_block.h and simd_kernel.h.

#ifndef SIMDJSON_PPC64_SIMD_GAPS_H
#define SIMDJSON_PPC64_SIMD_GAPS_H
#include "simdjson/generic/intrinsics.h"             // <altivec.h>
#include "simdjson/internal/simdprune_tables.h"    // thintable_epi8, ...

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {
namespace native {

using quarter = dp::vec<uint8_t, 16>; // one Altivec register
// The ppc64 bespoke simd.h aliases __m128i to __vector unsigned char.
using altivec_u8 = __vector unsigned char;

simdjson_inline altivec_u8 to_altivec(const quarter q) { return std::bit_cast<altivec_u8>(q); }
simdjson_inline quarter from_altivec(const altivec_u8 r) { return std::bit_cast<quarter>(r); }

// 16-entry byte lookup with x86 pshufb semantics: result = (b & 0x80) ? 0 : table[b & 0x0F].
// Altivec vec_perm indexes the full byte modulo 32 (wrapping across two source regs), so:
//   - mask the index to the low nibble (vec_and with splat 0x0F) for the in-range lookup
//   - use vec_perm(tbl, zero, idx) so that vec_perm uses elements [0..15] from tbl and
//     any index that reaches [16..31] would come from zero -- but since idx is already
//     masked to 0x0F (0..15) the second source argument is not consulted; we still pass
//     a zero vector so out-of-spec indexes are safe
//   - separately detect lanes where the original value had its high bit set, and zero those
//     lanes (vec_andc: res & ~mask)
// Note: vec_perm on little-endian PPC64 reverses the element order within each 16-byte
// source register but NOT across the two registers, so a separate vec_reve is needed on
// LE before using vec_perm as a table lookup.  However for a symmetric table (same 16
// bytes in both halves) the LE reversal cancels out: element [i] of the output is
// table[idx[i]] regardless of endianness.  The zero masking remains endian-neutral.
simdjson_inline altivec_u8 pshufb16(const altivec_u8 tbl, const altivec_u8 value) {
  const altivec_u8 zero = vec_splats((unsigned char)0);
  const altivec_u8 idx  = vec_and(value, vec_splats((unsigned char)0x0F));
  const altivec_u8 res  = vec_perm(tbl, zero, idx);
  // zero lanes where the high bit of the original value was set
  const altivec_u8 hi   = (altivec_u8)vec_cmpge(value, vec_splats((unsigned char)0x80));
  return vec_andc(res, hi);
}
simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  const altivec_u8 tbl = vec_vsx_ld(0, table);
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  return dp::cat(
      from_altivec(pshufb16(tbl, to_altivec(c0))),
      from_altivec(pshufb16(tbl, to_altivec(c1))),
      from_altivec(pshufb16(tbl, to_altivec(c2))),
      from_altivec(pshufb16(tbl, to_altivec(c3))));
}

// prev<N>: cross-64-byte byte shift built from four 16-byte vec_sld steps, each pulling the
// carry from the preceding 16-byte quarter (quarter -1 is the last quarter of prev_chunk).
// On little-endian PPC64, vec_sld operates on big-endian byte order, so the input must
// be byte-reversed with vec_reve before and after, mirroring the bespoke ppc64 prev<N>.
template <int N>
simdjson_inline altivec_u8 prev16(const altivec_u8 cur, const altivec_u8 prv) {
#ifdef __LITTLE_ENDIAN__
  altivec_u8 cur_be  = vec_reve(cur);
  altivec_u8 prv_be  = vec_reve(prv);
  altivec_u8 res_be  = vec_sld(prv_be, cur_be, 16 - N);
  return vec_reve(res_be);
#else
  return vec_sld(prv, cur, 16 - N);
#endif
}
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  auto [p0, p1, p2, p3] = dp::chunk<quarter>(prev_chunk);
  (void)p0; (void)p1; (void)p2;
  return dp::cat(
      from_altivec(prev16<N>(to_altivec(c0), to_altivec(p3))),
      from_altivec(prev16<N>(to_altivec(c1), to_altivec(c0))),
      from_altivec(prev16<N>(to_altivec(c2), to_altivec(c1))),
      from_altivec(prev16<N>(to_altivec(c3), to_altivec(c2))));
}

// One 16-byte compress step (thintable + two vec_perm), writing 16 - popcount(mask16) bytes.
// Mirrors the bespoke ppc64 simd8<uint8_t>::compress exactly: build shufmask from thintable,
// add 0x08 to the second 8 lanes, gather with vec_perm(v, v, shufmask), then apply the
// combine mask via vec_perm(pruned, zero, compactmask).
simdjson_inline int compress16(altivec_u8 v, uint16_t mask, uint8_t* output) {
  using simdjson::internal::thintable_epi8;
  using simdjson::internal::BitsSetTable256mul2;
  using simdjson::internal::pshufb_combine_table;
  uint8_t mask1 = uint8_t(mask), mask2 = uint8_t(mask >> 8);
#ifdef __LITTLE_ENDIAN__
  altivec_u8 shufmask = (altivec_u8)((__vector unsigned long long){
      thintable_epi8[mask1], thintable_epi8[mask2]});
#else
  altivec_u8 shufmask = (altivec_u8)((__vector unsigned long long){
      thintable_epi8[mask2], thintable_epi8[mask1]});
  shufmask = vec_reve(shufmask);
#endif
  // increment the second half of the shuffle mask by 0x08
  shufmask = (altivec_u8)((altivec_u8)shufmask +
             (altivec_u8)((__vector int){0, 0, 0x08080808, 0x08080808}));
  altivec_u8 pruned = vec_perm(v, v, shufmask);
  int pop1 = BitsSetTable256mul2[mask1];
  altivec_u8 compactmask = vec_vsx_ld(0, reinterpret_cast<const uint8_t*>(pshufb_combine_table + pop1 * 8));
  const altivec_u8 zero = vec_splats((unsigned char)0);
  altivec_u8 answer = vec_perm(pruned, zero, compactmask);
  vec_vsx_st(answer, 0, reinterpret_cast<altivec_u8*>(output));
  return 16 - __builtin_popcount(mask);
}

// Compact out bytes whose mask bit is 1, over the 64-byte block: four 16-byte steps.
simdjson_inline void compress(const block value, uint64_t mask, uint8_t* output) {
  auto [c0, c1, c2, c3] = dp::chunk<quarter>(value);
  uint8_t* out = output;
  out += compress16(to_altivec(c0), uint16_t(mask),       out);
  out += compress16(to_altivec(c1), uint16_t(mask >> 16), out);
  out += compress16(to_altivec(c2), uint16_t(mask >> 32), out);
  out += compress16(to_altivec(c3), uint16_t(mask >> 48), out);
}

} // namespace native
} // namespace simd
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_PPC64_SIMD_GAPS_H
