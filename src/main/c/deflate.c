#include "deflate.h"
/*
   "DEFLATE Compressed Data Format Specification" <http://www.w3.org/Graphics/PNG/RFC-1951>
   "ZLIB Compressed Data Format Specification" <https://www.ietf.org/rfc/rfc1950.txt>
 */

#include <stdio.h>

/// The extra bits following a length code to store
/// the actual value of the code
const size_t LENGTH_EXTRA_BITS[29] = {
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1,
	0x1, 0x1, 0x2, 0x2, 0x2, 0x2, 0x3, 0x3, 0x3, 0x3,
	0x4, 0x4, 0x4, 0x4, 0x5, 0x5, 0x5, 0x5, 0x0
};

/// The extra bits following a distance code to store
/// the actual value of the code
const size_t DISTANCE_EXTRA_BITS[30] = {
	0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x2, 0x2, 0x3, 0x3,
	0x4, 0x4, 0x5, 0x5, 0x6, 0x6, 0x7, 0x7, 0x8, 0x8,
	0x9, 0x9, 0xA, 0xA, 0xB, 0xB, 0xC, 0xC, 0xD, 0xD,
};


struct BitStrShort encode_fixed_huffman_code(uint16_t value) {
	if (value <= 143) {
		struct BitStrShort retval = {8, value + 0b00110000};
		bitstr_reverse(&retval);
		return retval;
	} else if (value <= 255) {
		struct BitStrShort retval = {9, value - 144 + 0b110010000};
		bitstr_reverse(&retval);
		return retval;
	} else if (value <= 279) {
		struct BitStrShort retval = {7, value - 256};
		bitstr_reverse(&retval);
		return retval;
	} else if (value <= 285) {
		struct BitStrShort retval = {8, value - 280 + 0b11000000};
		bitstr_reverse(&retval);
		return retval;
	} else {
		fprintf(stderr, "Value error: encode_fixed_huffman_codes value exceeded 285");
		exit(1);
	}
}

struct BitStrShort encode_length(uint16_t value) {
	if (value < 3) {
		fprintf(stderr, "Value error: encode_length value less than 3");
		exit(1);
	}
	if (value > 258) {
		fprintf(stderr, "Value error: encode_length value greater than 258");
		exit(1);
	}
	
	uint16_t code = 0;
	value -= 3;
	while (value >= (1 << LENGTH_EXTRA_BITS[code])) {
		value -= 1 << LENGTH_EXTRA_BITS[code];
		code++;
	}
	struct BitStrShort hufcode = encode_fixed_huffman_code(257 + code);
	struct BitStrShort extra = {.len = LENGTH_EXTRA_BITS[code], .value = value};
	struct BitStrShort retval = {
		.value = hufcode.value + (extra.value << hufcode.len),
		.len = hufcode.len + extra.len,
	};
	return retval;
}

struct BitStrShort encode_offset(uint16_t value) {
	if (value > 32768) {
		fprintf(stderr, "Value error: encode_offset value greater than 32768");
		exit(1);
	}
	uint16_t code = 0;
	while (value >= (1 << DISTANCE_EXTRA_BITS[code])) {
		value -= 1 << DISTANCE_EXTRA_BITS[code];
		code++;
	}
	struct BitStrShort hufcode = {
		.len = 5,
		.value = code,
	};
	bitstr_reverse(&hufcode);
	struct BitStrShort extra = {
		.len = DISTANCE_EXTRA_BITS[code],
		.value = value,
	};
	struct BitStrShort retval = {
		.value = hufcode.value + (extra.value << hufcode.len),
		.len = hufcode.len + extra.len,
	};
	return retval;
}


const uint32_t ADLER32_DIVISOR = 65521;
void adler32_push(struct Adler32 *const this, uint8_t value) {
	this->s1 += value;
	this->s1 %= ADLER32_DIVISOR;
	this->s2 += this->s1;
	this->s2 %= ADLER32_DIVISOR;
}

void adler32_pushZeroRepeatedly(struct Adler32 *const this, size_t numZeros) {
	for (size_t i = 0; i < numZeros; i++) {
		this->s2 += this->s1;
		this->s2 %= ADLER32_DIVISOR;
	}
}
