// RVV-VLS native gap fills for the shared std::simd kernel (64-lane block).
//
// Implements the `native::` customization point the kernel delegates to
// (lookup_16, prev<N>, compress) with RISC-V Vector intrinsics. The 64-byte
// `block` is a single fixed-width 512-bit RVV register bridged via std::bit_cast
// to the bespoke fixed-width vuint8x64_t type, so every gap is one whole-register
// op: vrgather for lookup_16, vslidedown+vslideup for prev<N>, vcompress for
// compress.
//
// NOTE: GCC 16 std::simd has no RISC-V V backend, so the kernel's non-gap ops
// (eq/lteq/to_bitmask/is_ascii) lower to scalar -- accepted; this backend trades
// peak SIMD performance for a single unified code path. The gap fills here use
// native RVV intrinsics and run natively.
//
// Included by rvv-vls/simd.h between simd_block.h and simd_kernel.h.

#ifndef SIMDJSON_RVV_VLS_SIMD_GAPS_H
#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_RVV_VLS_SIMD_GAPS_H
#include "simdjson/rvv-vls/intrinsics.h"           // <riscv_vector.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {

// ---------------------------------------------------------------------------
// Fixed-width RVV type aliases for the 64-byte (512-bit) block.
//
// vuint8x64_t is always 512-bit regardless of __riscv_v_fixed_vlen:
//   VLEN 128: m4 (4 registers), VLEN 256: m2, VLEN 512+: m1.
// vboolx64_t correspondingly has 64 bits (1 bit per element at 512 bits / 8):
//   VLEN 128: vbool2_t, VLEN 256: vbool4_t, VLEN 512+: vbool8_t.
// ---------------------------------------------------------------------------
#if __riscv_v_fixed_vlen == 128
using vuint8x64_t = vuint8m4_t __attribute__((riscv_rvv_vector_bits(512)));
using vboolx64_t  = vbool2_t   __attribute__((riscv_rvv_vector_bits(512/8)));
#elif __riscv_v_fixed_vlen == 256
using vuint8x64_t = vuint8m2_t __attribute__((riscv_rvv_vector_bits(512)));
using vboolx64_t  = vbool4_t   __attribute__((riscv_rvv_vector_bits(512/8)));
#else
// VLEN >= 512 (including exactly 512, the canonical target)
using vuint8x64_t = vuint8m1_t __attribute__((riscv_rvv_vector_bits(512)));
using vboolx64_t  = vbool8_t   __attribute__((riscv_rvv_vector_bits(512/8)));
#endif

namespace native {

// VL for the 64-byte block: always 64 elements (512 bits / 8 bits per element).
static constexpr size_t VL = 64;

// Bridge the 64-byte std::simd block to/from the 512-bit fixed-width vuint8 type.
simdjson_inline vuint8x64_t to_rvv(const block v)   { return std::bit_cast<vuint8x64_t>(v); }
simdjson_inline block from_rvv(const vuint8x64_t r) { return std::bit_cast<block>(r); }

// ---------------------------------------------------------------------------
// lookup_16: 16-entry byte lookup with pshufb semantics.
//   result[i] = (value[i] & 0x80) ? 0 : table[value[i] & 0x0F]
//
// Implementation:
//   1. Mask low nibble: idx = value & 0x0F  (safe gather index 0..15).
//   2. Load the 16-byte table into a vuint8m1_t (single VLEN-wide register).
//   3. vrgather over the 64-element block -- for VLEN < 512 where the block
//      spans multiple m1 registers, use the simdutf macros from intrinsics.h.
//   4. Zero out lanes where the high bit was set (value >= 0x80).
// ---------------------------------------------------------------------------
simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  const vuint8x64_t v = to_rvv(value);
  const vuint8m1_t  tbl = __riscv_vle8_v_u8m1(table, __riscv_vsetvlmax_e8m1());

#if __riscv_v_fixed_vlen == 128
  // VLEN=128: block is vuint8m4_t; use the 4-chunk simdutf gather macro.
  const vuint8x64_t idx = __riscv_vand_vx_u8m4(v, 0x0F, VL);
  vuint8x64_t res       = simdutf_vrgather_u8m1x4(tbl, idx);
  // Zero lanes where high bit is set: vboolx64_t = vbool2_t here.
  vboolx64_t hibits = __riscv_vmsgeu_vx_u8m4_b2(v, 0x80, VL);
  res = __riscv_vmerge_vxm_u8m4(res, 0, hibits, VL);

#elif __riscv_v_fixed_vlen == 256
  // VLEN=256: block is vuint8m2_t; use the 2-chunk simdutf gather macro.
  const vuint8x64_t idx = __riscv_vand_vx_u8m2(v, 0x0F, VL);
  vuint8x64_t res       = simdutf_vrgather_u8m1x2(tbl, idx);
  // vboolx64_t = vbool4_t here.
  vboolx64_t hibits = __riscv_vmsgeu_vx_u8m2_b4(v, 0x80, VL);
  res = __riscv_vmerge_vxm_u8m2(res, 0, hibits, VL);

#else
  // VLEN>=512: block is vuint8m1_t; single-register vrgather works directly.
  const vuint8x64_t idx = __riscv_vand_vx_u8m1(v, 0x0F, VL);
  vuint8x64_t res       = __riscv_vrgather_vv_u8m1(tbl, idx, VL);
  // vboolx64_t = vbool8_t here.
  vboolx64_t hibits = __riscv_vmsgeu_vx_u8m1_b8(v, 0x80, VL);
  res = __riscv_vmerge_vxm_u8m1(res, 0, hibits, VL);
#endif

  return from_rvv(res);
}

// ---------------------------------------------------------------------------
// prev<N>: cross-64-byte byte shift, pulling N bytes from the tail of
// prev_chunk into the head of value.  Mirrors the bespoke simd8::prev<N>
// exactly, applied to the full 64-element block.
//
//   vslidedown(prv, 64-N, 64): expose the last N elements of prv at [0..N-1].
//   vslideup(slid, cur, N, 64): overlay cur starting at index N.
// Result: [prv[64-N..63], cur[0..63-N]]
// ---------------------------------------------------------------------------
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  static_assert(N >= 1 && N <= 3, "prev<N>: N must be 1, 2, or 3");
  const vuint8x64_t cur  = to_rvv(value);
  const vuint8x64_t prv  = to_rvv(prev_chunk);
  const vuint8x64_t slid = __riscv_vslidedown(prv, VL - N, VL);
  const vuint8x64_t res  = __riscv_vslideup(slid, cur, N, VL);
  return from_rvv(res);
}

// ---------------------------------------------------------------------------
// compress: compact out bytes whose mask bit is 1; write 64 - popcount(mask)
// bytes to output.  The count is computed by the kernel wrapper (simd_kernel.h)
// via __builtin_popcountll; this function only stores.
//
// Mirrors bespoke simd8x64::compress exactly:
//   1. Invert mask: keep = ~mask  (bit=1 now means "keep this byte").
//   2. Scalar-insert keep into element-0 of a u64m1 register.
//   3. Reinterpret as vboolx64_t (64 mask bits, 1 per element).
//   4. vcompress to gather the kept elements.
//   5. Store the result.
// ---------------------------------------------------------------------------
simdjson_inline void compress(const block value, uint64_t mask, uint8_t* output) {
  const vuint8x64_t v    = to_rvv(value);
  const uint64_t    keep = ~mask;  // bit=1 means keep this lane

#if __riscv_v_fixed_vlen == 128
  vboolx64_t m = __riscv_vreinterpret_b2(__riscv_vmv_s_x_u64m1(keep, 1));
#elif __riscv_v_fixed_vlen == 256
  vboolx64_t m = __riscv_vreinterpret_b4(__riscv_vmv_s_x_u64m1(keep, 1));
#else
  vboolx64_t m = __riscv_vreinterpret_b8(__riscv_vmv_s_x_u64m1(keep, 1));
#endif
  __riscv_vse8(output, __riscv_vcompress(v, m, VL), __builtin_popcountll(keep));
}

} // namespace native
} // namespace simd
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_RVV_VLS_SIMD_GAPS_H
