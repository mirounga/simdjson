// ARM SVE2 native gap fills for the shared std::simd kernel (64-lane block).
//
// Proper SVE engineering: the three primitives the kernel delegates to native::
// (lookup_16 / prev<N> / compress) are implemented over the WHOLE 64-byte vector
// with native SVE instructions -- no per-128-bit NEON chunking:
//   lookup_16 -> svtbl     (cross-vector 16-entry table lookup, pshufb semantics)
//   prev<N>   -> svext     (cross-vector byte splice)
//   compress  -> svcompact (SVE has no byte compact, so widen u8->u32, compact, narrow)
//
// Requires a FIXED SVE width of 512 bits (-msve-vector-bits=512) so the 64-byte
// `block` is exactly one Z register; the static_assert below enforces it. Selected
// by sve/simd.h between simd_block.h and simd_kernel.h.

#ifndef SIMDJSON_SVE_SIMD_GAPS_H
#define SIMDJSON_SVE_SIMD_GAPS_H
#include "simdjson/generic/intrinsics.h"   // <arm_sve.h>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace simd {
namespace native {

// Fixed-length SVE type (needs -msve-vector-bits=512): sized, hence bit_cast-able to/from
// the 64-byte `block`. The sizeless svuint8_t is accepted wherever a fixed_u8 is passed.
typedef svuint8_t fixed_u8 __attribute__((arm_sve_vector_bits(512)));
static_assert(sizeof(fixed_u8) == 64,
  "sve2 backend requires a fixed 512-bit SVE width (-msve-vector-bits=512): block is 64 bytes");

simdjson_inline svuint8_t to_sve(const block v) { return std::bit_cast<fixed_u8>(v); }
simdjson_inline block from_sve(const svuint8_t v) { return std::bit_cast<block>(fixed_u8(v)); }

// 16-entry byte lookup, x86 pshufb semantics: result = (b & 0x80) ? 0 : table[b & 0x0F].
// One svtbl over all 64 lanes (svld1rq replicates the 16-byte table into each 128-bit
// segment; svtbl indexes lanes 0..15 globally). Indices are masked to the low nibble, then
// the high-bit lanes are zeroed to match the kernel contract (it passes raw bytes).
simdjson_inline block lookup_16(const block value, const uint8_t table[16]) {
  const svbool_t pg = svptrue_b8();
  const svuint8_t v = to_sve(value);
  const svuint8_t tbl = svld1rq_u8(pg, table);            // table -> lanes 0..15
  const svuint8_t idx = svand_n_u8_x(pg, v, 0x0F);
  svuint8_t res = svtbl_u8(tbl, idx);                     // table[idx], idx in 0..15
  const svbool_t keep = svcmplt_n_u8(pg, v, 0x80);        // true where high bit clear
  res = svsel_u8(keep, res, svdup_n_u8(0));               // zero the high-bit lanes
  return from_sve(res);
}

// prev<N>: shift the 64-byte vector right by N bytes, pulling the last N bytes of prev_chunk
// into the front -- the VL-length window of {prev_chunk, value} starting at byte 64-N.
template <int N>
simdjson_inline block prev(const block value, const block prev_chunk) {
  return from_sve(svext_u8(to_sve(prev_chunk), to_sve(value), 64 - N));
}

// Compact out bytes whose mask bit is 1; writes 64 - popcount(mask) bytes. SVE has no byte
// compact, so for each 16-byte group: widen u8->u32, svcompact_u32 keeping the mask-0 lanes,
// then truncating-store the kept u32->u8. Four groups cover the 64-byte block.
simdjson_inline void compress(const block value, uint64_t mask, uint8_t* output) {
  const svbool_t p32 = svptrue_b32();                     // 16 active u32 lanes at VL=512
  const svuint32_t iota = svindex_u32(0, 1);              // 0,1,2,...,15
  const svuint32_t bitsel = svlsl_u32_x(p32, svdup_n_u32(1), iota); // 1<<i
  const svuint8_t v = to_sve(value);
  // widen all 64 bytes to u32 in four 16-lane groups (zero-extending unpacks)
  const svuint16_t lo16 = svunpklo_u16(v);               // bytes 0..31
  const svuint16_t hi16 = svunpkhi_u16(v);               // bytes 32..63
  const svuint32_t g[4] = {
    svunpklo_u32(lo16),                                   // bytes 0..15
    svunpkhi_u32(lo16),                                   // bytes 16..31
    svunpklo_u32(hi16),                                   // bytes 32..47
    svunpkhi_u32(hi16),                                   // bytes 48..63
  };
  uint8_t* out = output;
  for (int i = 0; i < 4; i++) {
    const uint32_t m = uint32_t((mask >> (16 * i)) & 0xFFFFu);
    // keep lane j when mask bit j is 0
    const svbool_t keep = svcmpeq_n_u32(p32, svand_u32_x(p32, svdup_n_u32(m), bitsel), 0);
    const svuint32_t packed = svcompact_u32(keep, g[i]);  // kept lanes -> front
    const int kept = 16 - __builtin_popcount(m);
    svst1b_u32(svwhilelt_b32(0, kept), out, packed);      // store `kept` bytes (u32->u8)
    out += kept;
  }
}

} // namespace native
} // namespace simd
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SVE_SIMD_GAPS_H
