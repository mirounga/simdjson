// Shared std::simd kernel ops (no include guard of its own; emitted once per impl).
//
// The free-function layer over the 64-byte `block` (from generic/simd_block.h),
// delegating the three primitives std::simd can't express well (lookup_16, prev<N>,
// compress) to the per-backend `native::` gap fills (from <isa>/simd_gaps.h). Both
// of those must be included BEFORE this header -- see generic/simd_block.h for the
// fixed include order each <isa>/simd.h follows. The generic stage-1/stage-2
// algorithms operate on `block` via native vec operators plus these free functions.

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {

// Load a 64-byte block.
simdjson_inline block load_block(const uint8_t* p) { return dp::unchecked_load<block>(p, 64); }

// std::simd mask -> integer bitset (movemask). Works for any width.
template <typename M>
simdjson_inline uint64_t to_bitmask(const M m) { return m.to_ullong(); }

// Element == / <= against a splat value, as a bitmask. Works for any vec<uint8_t,N>.
template <typename V>
simdjson_inline uint64_t eq(const V v, uint8_t c) { return to_bitmask(v == V(c)); }
template <typename V>
simdjson_inline uint64_t lteq(const V v, uint8_t c) { return to_bitmask(v <= V(c)); }

// Saturating subtract emulated with min (a - min(a,b)); also serves gt_bits.
simdjson_inline block saturating_sub(const block a, const block b) { return a - dp::min(a, b); }

// True iff every byte is ASCII (high bit clear everywhere).
simdjson_inline bool is_ascii(const block v) {
  return dp::none_of((v & block(uint8_t(0x80))) != block(uint8_t(0)));
}
// True iff any byte is nonzero.
simdjson_inline bool any_set(const block v) { return dp::any_of(v != block(uint8_t(0))); }

// 16-entry byte lookup (pshufb semantics), cross-block byte shift, and masked
// compaction -- delegated to the per-backend native gap fills.
simdjson_inline block lookup_16(const block v, const uint8_t table[16]) { return native::lookup_16(v, table); }
template <int N>
simdjson_inline block prev(const block cur, const block prev_chunk) { return native::prev<N>(cur, prev_chunk); }
// Writes 64 - popcount(mask) significant bytes; returns that count.
simdjson_inline uint64_t compress(const block v, uint64_t mask, uint8_t* output) {
  native::compress(v, mask, output);
  return 64 - __builtin_popcountll(mask);
}

} // namespace simd

// Make `block` and the ops visible unqualified to the generic stage-1/2 code.
using namespace simd;

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson
