#include "bit_parser.h"
#include "logger.h"   // ✅ For logging warnings and info
#include <string.h>
#include <stdio.h>

/**
 * Initializes a BitParser with the given data and bit length.
 * Sets the parser to the start of the bitstream.
 */
void bit_parser_init(BitParser *parser, const uint8_t *data, size_t bit_length) {
    parser->data = data;
    parser->bit_length = bit_length;
    parser->bit_pos = 0;

    log_info("BitParser initialized: %zu bits total", bit_length);
}

/**
 * Reads up to 32 bits from the stream.
 * If num_bits is invalid or exceeds available data, logs a warning.
 *
 * @param parser    Pointer to active BitParser
 * @param num_bits  Number of bits to read (1-32)
 * @return          Extracted bits as uint32_t, or 0 if error
 */
uint32_t bit_parser_read(BitParser *parser, size_t num_bits) {
    if (num_bits == 0 || num_bits > 32) {
        log_warning("bit_parser_read: Invalid bit request (%zu bits)", num_bits);
        return 0;
    }

    if (parser->bit_pos + num_bits > parser->bit_length) {
        log_warning(
            "bit_parser_read: Attempt to read beyond stream (pos=%zu, requested=%zu, total=%zu)",
            parser->bit_pos, num_bits, parser->bit_length
        );
        return 0;
    }

    uint32_t value = 0;

    // Sequentially extract bits, MSB first
    for (size_t i = 0; i < num_bits; i++) {
        size_t byte_idx = (parser->bit_pos + i) / 8;
        size_t bit_idx  = 7 - ((parser->bit_pos + i) % 8); // MSB-first indexing
        uint8_t bit     = (parser->data[byte_idx] >> bit_idx) & 0x01;
        value = (value << 1) | bit;
    }

    parser->bit_pos += num_bits;

    log_info("bit_parser_read: Read %zu bits -> 0x%X (new pos=%zu)",
             num_bits, value, parser->bit_pos);

    return value;
}

/**
 * Skips a number of bits in the stream.
 * If skipping past the end, logs a warning and clamps to the max length.
 */
void bit_parser_skip(BitParser *parser, size_t num_bits) {
    if (parser->bit_pos + num_bits <= parser->bit_length) {
        parser->bit_pos += num_bits;
        log_info("bit_parser_skip: Skipped %zu bits (new pos=%zu)",
                 num_bits, parser->bit_pos);
    } else {
        log_warning("bit_parser_skip: Attempted to skip past end (pos=%zu, skip=%zu, total=%zu)",
                    parser->bit_pos, num_bits, parser->bit_length);
        parser->bit_pos = parser->bit_length;
    }
}

/**
 * Resets the parser to the start of the bitstream.
 */
void bit_parser_reset(BitParser *parser) {
    parser->bit_pos = 0;
    log_info("bit_parser_reset: Position reset to 0");
}
