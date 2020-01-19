#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdint.h>
#include <stdlib.h>

/** A sequence of bits */
struct BitStrShort {uint8_t len; uint16_t value;};

/** A buffer to write bits to */
struct Bitstream {
	size_t bufferc;
	uint8_t* bufferv;
	size_t byte_offset;
	uint8_t bit_offset;
};

/** Reverses the order of the bits in a bitstring */
void bitstr_reverse(struct BitStrShort *const this);

/** Initializes an empty bitstream */
void bitstream_init(struct Bitstream *const this);

/** Frees the bitstream's internal data buffer */
void bitstream_deinit(struct Bitstream *const this);

/** reallocate the internal buffer to contain at least `min_capacity` bytes */
void bitstream_extend(struct Bitstream *const this, const size_t min_capacity);

/** Appends the bits to the end of the bitstream */
void bitstream_append(struct Bitstream *const this, const uint8_t bitsc, const uint32_t bitsv);

/** Appends the bits to the end of the bitstream */
void bitstream_append_struct(struct Bitstream *const this, const struct BitStrShort value);

/** Append zero bits to the stream until the stream reaches a byte boundary */
void bitstream_fillToByteBoundary(struct Bitstream *const this);

#endif // #ifndef BITSTREAM_H
