#ifndef BIT_PARSER_H
#define BIT_PARSER_H

#include <stddef.h>
#include <stdint.h>

/**
 * BitParser
 * ----------------------
 * Struct for parsing arbitrary bit-width payloads.
 */
typedef struct {
    const uint8_t *data;  // Pointer to the byte array
    size_t bit_length;    // Total number of bits in the data
    size_t bit_pos;       // Current position in bits
} BitParser;

/**
 * Initializes a BitParser with the given data.
 *
 * @param parser      Pointer to BitParser struct
 * @param data        Pointer to data buffer
 * @param bit_length  Length in bits
 */
void bit_parser_init(BitParser *parser, const uint8_t *data, size_t bit_length);

/**
 * Reads `num_bits` bits from the stream and returns the value.
 *
 * @param parser      Pointer to BitParser struct
 * @param num_bits    Number of bits to read (up to 32)
 * @return            Extracted bits as uint32_t
 */
uint32_t bit_parser_read(BitParser *parser, size_t num_bits);

/**
 * Skips `num_bits` bits in the stream.
 *
 * @param parser      Pointer to BitParser struct
 * @param num_bits    Number of bits to skip
 */
void bit_parser_skip(BitParser *parser, size_t num_bits);

/**
 * Resets parser to the beginning of the data stream.
 */
void bit_parser_reset(BitParser *parser);

#endif // BIT_PARSER_H
