#ifndef SIMDJSON_GENERIC_STAGE1_UTF8_LOOKUP4_ALGORITHM_H

#define SIMDJSON_GENERIC_STAGE1_UTF8_LOOKUP4_ALGORITHM_H
#include "simdjson/generic/stage1/base.h"
#include "simdjson/generic/dom_parser_implementation.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace utf8_validation {

using namespace simd;

  // The UTF-8 validator stays in the vector-mask domain: it operates on the
  // 64-byte `block` directly (std::simd splits it into native registers) and
  // only reduces to a bool at the very end via any_set(error). lookup_16 takes
  // a 16-byte table; prev<N> / saturating_sub are the kernel free functions.
  simdjson_inline block check_special_cases(const block input, const block prev1) {
// Bit 0 = Too Short (lead byte/ASCII followed by lead byte/ASCII)
// Bit 1 = Too Long (ASCII followed by continuation)
// Bit 2 = Overlong 3-byte
// Bit 4 = Surrogate
// Bit 5 = Overlong 2-byte
// Bit 7 = Two Continuations
    constexpr const uint8_t TOO_SHORT   = 1<<0; // 11______ 0_______
                                                // 11______ 11______
    constexpr const uint8_t TOO_LONG    = 1<<1; // 0_______ 10______
    constexpr const uint8_t OVERLONG_3  = 1<<2; // 11100000 100_____
    constexpr const uint8_t SURROGATE   = 1<<4; // 11101101 101_____
    constexpr const uint8_t OVERLONG_2  = 1<<5; // 1100000_ 10______
    constexpr const uint8_t TWO_CONTS   = 1<<7; // 10______ 10______
    constexpr const uint8_t TOO_LARGE   = 1<<3; // 11110100 1001____
                                                // 11110100 101_____
                                                // 11110101 1001____
                                                // 11110101 101_____
                                                // 1111011_ 1001____
                                                // 1111011_ 101_____
                                                // 11111___ 1001____
                                                // 11111___ 101_____
    constexpr const uint8_t TOO_LARGE_1000 = 1<<6;
                                                // 11110101 1000____
                                                // 1111011_ 1000____
                                                // 11111___ 1000____
    constexpr const uint8_t OVERLONG_4  = 1<<6; // 11110000 1000____

    constexpr const uint8_t CARRY = TOO_SHORT | TOO_LONG | TWO_CONTS; // These all have ____ in byte 1 .
    static const uint8_t byte_1_high_table[16] = {
      // 0_______ ________ <ASCII in byte 1>
      TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
      TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
      // 10______ ________ <continuation in byte 1>
      TWO_CONTS, TWO_CONTS, TWO_CONTS, TWO_CONTS,
      // 1100____ ________ <two byte lead in byte 1>
      TOO_SHORT | OVERLONG_2,
      // 1101____ ________ <two byte lead in byte 1>
      TOO_SHORT,
      // 1110____ ________ <three byte lead in byte 1>
      TOO_SHORT | OVERLONG_3 | SURROGATE,
      // 1111____ ________ <four+ byte lead in byte 1>
      TOO_SHORT | TOO_LARGE | TOO_LARGE_1000 | OVERLONG_4
    };
    static const uint8_t byte_1_low_table[16] = {
      // ____0000 ________
      CARRY | OVERLONG_3 | OVERLONG_2 | OVERLONG_4,
      // ____0001 ________
      CARRY | OVERLONG_2,
      // ____001_ ________
      CARRY,
      CARRY,
      // ____0100 ________
      CARRY | TOO_LARGE,
      // ____0101 ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      // ____011_ ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      // ____1___ ________
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      // ____1101 ________
      CARRY | TOO_LARGE | TOO_LARGE_1000 | SURROGATE,
      CARRY | TOO_LARGE | TOO_LARGE_1000,
      CARRY | TOO_LARGE | TOO_LARGE_1000
    };
    static const uint8_t byte_2_high_table[16] = {
      // ________ 0_______ <ASCII in byte 2>
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
      // ________ 1000____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE_1000 | OVERLONG_4,
      // ________ 1001____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE,
      // ________ 101_____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,
      // ________ 11______
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT
    };
    const block byte_1_high = lookup_16(prev1 >> 4, byte_1_high_table);
    const block byte_1_low  = lookup_16(prev1 & block(uint8_t(0x0F)), byte_1_low_table);
    const block byte_2_high = lookup_16(input >> 4, byte_2_high_table);
    return (byte_1_high & byte_1_low & byte_2_high);
  }
  simdjson_inline block check_multibyte_lengths(const block input,
      const block prev_input, const block sc) {
    block prev2 = prev<2>(input, prev_input);
    block prev3 = prev<3>(input, prev_input);
    block must23 = must_be_2_3_continuation(prev2, prev3);
    block must23_80 = must23 & block(uint8_t(0x80));
    return must23_80 ^ sc;
  }

  //
  // Return nonzero if there are incomplete multibyte characters at the end of the block:
  // e.g. if there is a 4-byte character, but it's 3 bytes from the end.
  //
  simdjson_inline block is_incomplete(const block input) {
    // If the previous input's last 3 bytes match this, they're too short (they ended at EOF):
    // ... 1111____ 111_____ 11______
    static const uint8_t max_array[64] = {
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 0xf0u-1, 0xe0u-1, 0xc0u-1
    };
    const block max_value = load_block(max_array);
    // gt_bits: nonzero exactly where input > max_value (== saturating_sub).
    return saturating_sub(input, max_value);
  }

  struct utf8_checker {
    // If this is nonzero, there has been a UTF-8 error. (zero-initialized)
    block error = block(uint8_t(0));
    // The last input we received
    block prev_input_block = block(uint8_t(0));
    // Whether the last input we received was incomplete (used for ASCII fast path)
    block prev_incomplete = block(uint8_t(0));

    //
    // Check whether the current bytes are valid UTF-8.
    //
    simdjson_inline void check_utf8_bytes(const block input, const block prev_input) {
      // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+ lead bytes
      // (2, 3, 4-byte leads become large positive numbers instead of small negative numbers)
      block prev1 = prev<1>(input, prev_input);
      block sc = check_special_cases(input, prev1);
      this->error = this->error | check_multibyte_lengths(input, prev_input, sc);
    }

    // The only problem that can happen at EOF is that a multibyte character is too short
    // or a byte value too large in the last bytes: check_special_cases only checks for bytes
    // too large in the first of two bytes.
    simdjson_inline void check_eof() {
      // If the previous block had incomplete UTF-8 characters at the end, an ASCII block can't
      // possibly finish them.
      this->error = this->error | this->prev_incomplete;
    }

    // The 64-byte block is a single std::simd vector now: no NUM_CHUNKS loop.
    // prev<N> handles the cross-block carry over the whole 64 bytes at once.
    simdjson_inline void check_next_input(const block& input) {
      if(simdjson_likely(is_ascii(input))) {
        this->error = this->error | this->prev_incomplete;
      } else {
        this->check_utf8_bytes(input, this->prev_input_block);
        this->prev_incomplete = is_incomplete(input);
        this->prev_input_block = input;
      }
    }
    // do not forget to call check_eof!
    simdjson_warn_unused simdjson_inline error_code errors() {
      return any_set(this->error) ? error_code::UTF8_ERROR : error_code::SUCCESS;
    }

  }; // struct utf8_checker
} // namespace utf8_validation

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_STAGE1_UTF8_LOOKUP4_ALGORITHM_H
