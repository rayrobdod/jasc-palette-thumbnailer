#ifndef DEFLATE_H
#define DEFLATE_H

#include "bitstream.h"
#include <stdint.h>
#include <stdlib.h>

/** Encodes a direct value or length index using deflate's fixed huffman codes */
struct BitStrShort encode_fixed_huffman_code(uint16_t value);

/** Encodes an length using deflate's fixed huffman codes */
struct BitStrShort encode_length(uint16_t value);

/** Encodes an offset using deflate's fixed huffman codes */
struct BitStrShort encode_offset(uint16_t value);


/** The state of an adler checksum */
struct Adler32 {uint32_t s1; uint32_t s2;};

/** The initial state for an adler checksum */
#define ADLER32_INIT {.s1 = 1, .s2 = 0}

/** Adds the given byte to the checksum */
void adler32_push(struct Adler32 *const this, uint8_t value);

#endif // #ifndef DEFLATE_H
