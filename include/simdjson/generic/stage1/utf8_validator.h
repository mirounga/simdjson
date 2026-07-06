#ifndef SIMDJSON_GENERIC_STAGE1_UTF8_VALIDATOR_H

#define SIMDJSON_GENERIC_STAGE1_UTF8_VALIDATOR_H
#include "simdjson/generic/stage1/base.h"
#include "simdjson/generic/stage1/buf_block_reader.h"
#include "simdjson/generic/stage1/utf8_lookup4_algorithm.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

/**
 * Validates that the string is actual UTF-8.
 */
template<class checker>
bool generic_validate_utf8(const uint8_t * input, size_t length) {
    checker c{};
    buf_block_reader<64> reader(input, length);
    while (reader.has_full_block()) {
      simd::block in = simd::load_block(reader.full_block());
      c.check_next_input(in);
      reader.advance();
    }
    uint8_t block_buf[64]{};
    reader.get_remainder(block_buf);
    simd::block in = simd::load_block(block_buf);
    c.check_next_input(in);
    reader.advance();
    c.check_eof();
    return c.errors() == error_code::SUCCESS;
}

inline bool generic_validate_utf8(const char * input, size_t length) {
    return generic_validate_utf8<utf8_checker>(reinterpret_cast<const uint8_t *>(input),length);
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_STAGE1_UTF8_VALIDATOR_H