#ifndef SIMDJSON_STDSIMD_SIMD_H
#define SIMDJSON_STDSIMD_SIMD_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/stdsimd/base.h"
#include "simdjson/stdsimd/intrinsics.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace stdsimd {
namespace {
namespace simd {

// 32-byte vectors (AVX2 class), so a 64-byte block is NUM_CHUNKS == 2, matching
// the haswell backend for apples-to-apples benchmarking.
static constexpr int VEC_BYTES = 32;
using u8x32  = dp::vec<uint8_t, VEC_BYTES>;
using i8x32  = dp::vec<int8_t,  VEC_BYTES>;
using msk32  = u8x32::mask_type;

template <typename T> struct simd8;

// SIMD byte-mask type (returned by comparisons such as > ).
template <>
struct simd8<bool> {
  msk32 value;
  simdjson_inline simd8() : value{} {}
  simdjson_inline simd8(msk32 v) : value(v) {}
  // 32-bit movemask equivalent (std::simd mask -> integer bitset).
  simdjson_inline uint32_t to_bitmask() const { return uint32_t(value.to_ullong()); }
  simdjson_inline simd8<bool> operator|(const simd8<bool> o) const { return simd8<bool>(value | o.value); }
};

// Unsigned bytes. This is the workhorse used by UTF-8 validation.
template <>
struct simd8<uint8_t> {
  u8x32 value;

  simdjson_inline simd8() : value{} {}
  simdjson_inline simd8(u8x32 v) : value(v) {}
  // Splat constructor
  simdjson_inline simd8(uint8_t v) : value(v) {}
  // Load from a 32-byte buffer
  simdjson_inline simd8(const uint8_t* p) : value(dp::unchecked_load<u8x32>(p, VEC_BYTES)) {}

  static simdjson_inline simd8<uint8_t> splat(uint8_t v) { return simd8<uint8_t>(v); }
  static simdjson_inline simd8<uint8_t> zero() { return simd8<uint8_t>(u8x32{}); }
  static simdjson_inline simd8<uint8_t> load(const uint8_t* p) { return simd8<uint8_t>(p); }

  simdjson_inline void store(uint8_t* dst) const { dp::unchecked_store(value, dst, VEC_BYTES); }

  // Bitwise
  simdjson_inline simd8<uint8_t> operator|(const simd8<uint8_t> o) const { return value | o.value; }
  simdjson_inline simd8<uint8_t> operator&(const simd8<uint8_t> o) const { return value & o.value; }
  simdjson_inline simd8<uint8_t> operator^(const simd8<uint8_t> o) const { return value ^ o.value; }
  simdjson_inline simd8<uint8_t>& operator|=(const simd8<uint8_t> o) { value = value | o.value; return *this; }

  // Element-wise equality / less-than -> byte mask (unsigned).
  simdjson_inline simd8<bool> operator==(const simd8<uint8_t> o) const { return simd8<bool>(value == o.value); }
  simdjson_inline simd8<bool> operator<(const simd8<uint8_t> o) const { return simd8<bool>(value < o.value); }

  // Arithmetic
  simdjson_inline simd8<uint8_t> operator+(const simd8<uint8_t> o) const { return value + o.value; }
  simdjson_inline simd8<uint8_t> operator-(const simd8<uint8_t> o) const { return value - o.value; }

  // Saturating subtract, emulated with min (a - min(a,b)).
  // GAP:: (future standard, not GCC-only) C++26 std::simd has no saturating
  // arithmetic. P2956 "Allow std::simd overloads for saturating operations" adds
  // std::sub_sat/add_sat/saturate_cast for basic_simd, but it targets C++29 and
  // is not in GCC 16. Replace the emulation with dp::sub_sat(...) once available.
  // (C++26 only adds *scalar* std::sub_sat in <numeric>, which would not vectorize
  // here.) See docs/stdsimd-migration/GAPS.md.
  simdjson_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> o) const {
    return value - dp::min(value, o.value);
  }

  // gt_bits: nonzero where *this > other (used by is_incomplete).
  simdjson_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> o) const { return this->saturating_sub(o); }

  // Logical shift right of each byte by a compile-time amount.
  template <int N>
  simdjson_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(value >> N); }

  // 16-entry byte lookup with pshufb semantics: index's low nibble selects the
  // table entry; if the high bit (0x80) is set the result is 0.
  // GAP:: waiting for GCC std::simd support. C++26 specifies the dynamic
  // (simd-indexed) permute -- std::simd::permute(table, indices) / table[indices]
  // -- but GCC 16's <simd> declares it without implementing operator[](simd
  // indices), so we fall back to a generator-lambda scalar gather (correct, but
  // ~25x slower; dominant cost of the UTF-8 regression). Replace with
  // dp::permute(...) once GCC implements it. See docs/stdsimd-migration/GAPS.md.
  template <typename L>
  simdjson_inline simd8<L> lookup_16(
      L v0,  L v1,  L v2,  L v3,  L v4,  L v5,  L v6,  L v7,
      L v8,  L v9,  L v10, L v11, L v12, L v13, L v14, L v15) const {
    const L table[16] = { v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15 };
#if SIMDJSON_STDSIMD_NATIVE_PSHUFB && SIMDJSON_IS_X86_64
    // Escape hatch: native AVX2 vpshufb. pshufb is lane-wise (per 16 bytes) with
    // exactly the desired semantics (low nibble selects; high bit -> 0), so we
    // broadcast the 16-byte table to both lanes. Only valid for byte tables.
    if constexpr (sizeof(L) == 1) {
      const __m128i t128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(table));
      const __m256i tbl256 = _mm256_broadcastsi128_si256(t128);
      const __m256i res = _mm256_shuffle_epi8(tbl256, std::bit_cast<__m256i>(value));
      return simd8<L>(std::bit_cast<u8x32>(res));
    }
#endif
    // Portable path (GAP:: see above): generator-lambda scalar gather.
    const u8x32 idx = value;
    return simd8<L>(dp::vec<L, VEC_BYTES>([&](int i) {
      uint8_t b = idx[i];
      return (b & 0x80) ? L(0) : table[b & 0x0F];
    }));
  }

  // prev<N>: bytes from the boundary of the previous chunk shifted in.
  // result[i] = (i >= N) ? this[i-N] : prev_chunk[VEC_BYTES-N+i].
  // GAP:: waiting for GCC std::simd support. With a working dynamic permute (or a
  // two-input cross-lane shift) this could be done in-register; GCC 16 lacks it,
  // so we store both chunks to memory and reload at an offset. See GAPS.md.
  template <int N = 1>
  simdjson_inline simd8<uint8_t> prev(const simd8<uint8_t> prev_chunk) const {
#if SIMDJSON_STDSIMD_NATIVE_ALIGNR && SIMDJSON_IS_X86_64
    // Escape hatch: AVX2 alignr/permute2x128 idiom (same as haswell), via bit_cast.
    const __m256i cur = std::bit_cast<__m256i>(value);
    const __m256i prv = std::bit_cast<__m256i>(prev_chunk.value);
    return simd8<uint8_t>(std::bit_cast<u8x32>(
        _mm256_alignr_epi8(cur, _mm256_permute2x128_si256(prv, cur, 0x21), 16 - N)));
#else
    uint8_t buf[2 * VEC_BYTES];
    dp::unchecked_store(prev_chunk.value, buf, VEC_BYTES);
    dp::unchecked_store(value, buf + VEC_BYTES, VEC_BYTES);
    return simd8<uint8_t>(dp::unchecked_load<u8x32>(buf + VEC_BYTES - N, VEC_BYTES));
#endif
  }

  // Copy to `output` all bytes corresponding to a 0 bit in `mask` (i.e. compact
  // out the masked bytes). Writes 32 - popcount(mask) significant bytes.
  // GAP:: (future standard, not GCC-only) C++26 std::simd has no compaction;
  // P2956 adds compress(mask, v) for basic_simd but targets C++29 and is not in
  // GCC 16. Until then this is a scalar gather loop (correct, slow). See GAPS.md.
  template <typename L>
  simdjson_inline void compress(uint32_t mask, L* output) const {
#if SIMDJSON_STDSIMD_NATIVE_COMPRESS && SIMDJSON_IS_X86_64
    // Escape hatch: AVX2 thintable/pshufb compaction (same algorithm as the
    // haswell backend), via std::bit_cast. Only valid for byte output.
    if constexpr (sizeof(L) == 1) {
      using simdjson::internal::thintable_epi8;
      using simdjson::internal::BitsSetTable256mul2;
      using simdjson::internal::pshufb_combine_table;
      uint8_t mask1 = uint8_t(mask), mask2 = uint8_t(mask >> 8);
      uint8_t mask3 = uint8_t(mask >> 16), mask4 = uint8_t(mask >> 24);
      __m256i shufmask = _mm256_set_epi64x(thintable_epi8[mask4], thintable_epi8[mask3],
                                           thintable_epi8[mask2], thintable_epi8[mask1]);
      shufmask = _mm256_add_epi8(shufmask, _mm256_set_epi32(0x18181818, 0x18181818,
                                 0x10101010, 0x10101010, 0x08080808, 0x08080808, 0, 0));
      __m256i pruned = _mm256_shuffle_epi8(std::bit_cast<__m256i>(value), shufmask);
      int pop1 = BitsSetTable256mul2[mask1];
      int pop3 = BitsSetTable256mul2[mask3];
      __m256i v256 = _mm256_castsi128_si256(
          _mm_loadu_si128(reinterpret_cast<const __m128i *>(pshufb_combine_table + pop1 * 8)));
      __m256i compactmask = _mm256_insertf128_si256(v256,
          _mm_loadu_si128(reinterpret_cast<const __m128i *>(pshufb_combine_table + pop3 * 8)), 1);
      __m256i almostthere = _mm256_shuffle_epi8(pruned, compactmask);
      __m128i v128;
      v128 = _mm256_castsi256_si128(almostthere);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(output), v128);
      v128 = _mm256_extractf128_si256(almostthere, 1);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(output + 16 - __builtin_popcount(mask & 0xFFFF)), v128);
      return;
    }
#endif
    // Portable path (GAP:: compaction; see compress/saturating gap in GAPS.md).
    int pos = 0;
    for (int i = 0; i < VEC_BYTES; i++) {
      if (!((mask >> i) & 1u)) { output[pos++] = L(value[i]); }
    }
  }

  // True iff every byte is ASCII (high bit clear everywhere).
  simdjson_inline bool is_ascii() const { return dp::none_of((value & u8x32(uint8_t(0x80))) != u8x32(uint8_t(0))); }
  simdjson_inline bool any_bits_set_anywhere() const { return dp::any_of(value != u8x32(uint8_t(0))); }
};

// Signed bytes (only used by the unused must_be_continuation helper, but must compile).
template <>
struct simd8<int8_t> {
  i8x32 value;
  simdjson_inline simd8() : value{} {}
  simdjson_inline simd8(i8x32 v) : value(v) {}
  // Reinterpret the bits of an unsigned vector as signed (per-element static_cast).
  simdjson_inline simd8(const simd8<uint8_t> u)
    : value(i8x32([&](int i){ return int8_t(u.value[i]); })) {}
  simdjson_inline simd8<bool> operator>(const int8_t v) const {
    return simd8<bool>(value > i8x32(v));
  }
};

template <typename T>
struct simd8x64 {
  static constexpr int NUM_CHUNKS = 64 / VEC_BYTES; // == 2
  static_assert(NUM_CHUNKS == 2, "stdsimd backend uses two 32-byte registers per 64-byte block.");
  const simd8<T> chunks[NUM_CHUNKS];

  simd8x64(const simd8x64<T>&) = delete;
  simd8x64<T>& operator=(const simd8<T>&) = delete;
  simd8x64() = delete;

  simdjson_inline simd8x64(const simd8<T> c0, const simd8<T> c1) : chunks{c0, c1} {}
  simdjson_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr + VEC_BYTES)} {}

  template <int idx>
  simdjson_inline simd8<uint8_t> get() const { return chunks[idx]; }

  simdjson_inline void store(T ptr[64]) const {
    chunks[0].store(ptr);
    chunks[1].store(ptr + VEC_BYTES);
  }

  simdjson_inline simd8<T> reduce_or() const { return chunks[0] | chunks[1]; }

  // Element-wise equality against a splat value, returned as a 64-bit mask.
  simdjson_inline uint64_t eq(const T m) const {
    const simd8<uint8_t> mask = simd8<uint8_t>::splat(m);
    uint64_t lo = uint32_t((chunks[0] == mask).to_bitmask());
    uint64_t hi = uint32_t((chunks[1] == mask).to_bitmask());
    return lo | (hi << 32);
  }

  // Element-wise <= against a splat value, returned as a 64-bit mask.
  simdjson_inline uint64_t lteq(const T m) const {
    const u8x32 mask = u8x32(m);
    uint64_t lo = uint32_t(simd8<bool>(chunks[0].value <= mask).to_bitmask());
    uint64_t hi = uint32_t(simd8<bool>(chunks[1].value <= mask).to_bitmask());
    return lo | (hi << 32);
  }

  // Compact out bytes whose corresponding mask bit is 1; returns bytes written.
  simdjson_inline uint64_t compress(uint64_t mask, T* output) const {
    uint32_t mask1 = uint32_t(mask);
    uint32_t mask2 = uint32_t(mask >> 32);
    chunks[0].compress(mask1, output);
    chunks[1].compress(mask2, output + VEC_BYTES - __builtin_popcount(mask1));
    return 64 - __builtin_popcountll(mask);
  }
};

} // namespace simd

// Make simd8 / simd8x64 visible unqualified to the generic stage-1 code, which
// refers to them without the `simd::` prefix. (The ISA backends get this via
// their stringparsing_defs.h, which this minimal backend does not include.)
using namespace simd;

} // unnamed namespace
} // namespace stdsimd
} // namespace simdjson

#endif // SIMDJSON_STDSIMD_SIMD_H
