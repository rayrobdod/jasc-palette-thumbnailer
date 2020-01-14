struct BitStrShort {uint8_t len; uint16_t value;};

struct Bitstream {
	size_t bufferc;
	char* bufferv;
	size_t byte_offset;
	uint8_t bit_offset;
};


void bitstr_reverse(struct BitStrShort *const this) {
	uint16_t newval = 0;
	for (size_t i = 0; i < this->len; i++) {
		newval <<= 1;
		newval |= (this->value & 1);
		this->value >>= 1;
	}
	this->value = newval;
}


void bitstream_init(struct Bitstream *const this) {
	this->bit_offset = 0;
	this->byte_offset = 0;
	this->bufferc = 128;
	this->bufferv = calloc(1, this->bufferc);
	if (! (this->bufferv)) {
		fprintf(stderr, "memory allocation failure");
		exit(1);
	}
}

void bitstream_deinit(struct Bitstream *const this) {
	free(this->bufferv);
	this->bufferv = NULL;
	this->bufferc = 0;
}

void bitstream_extend(struct Bitstream *const this, const size_t min_capacity) {
	if (this->bufferc < min_capacity) {
		this->bufferv = realloc(this->bufferv, min_capacity);
		if (! (this->bufferv)) {
			fprintf(stderr, "memory allocation failure");
			exit(1);
		}
		for (size_t i = this->bufferc; i < min_capacity; i++) {
			this->bufferv[i] = 0;
		}
		this->bufferc = min_capacity;
	}
}

void bitstream_append(struct Bitstream *const this, const uint8_t bitsc, const uint32_t bitsv) {
	bitstream_extend(this, this->byte_offset + 1);
	int avaliableBitsInByte = 8 - this->bit_offset % 8;
	if (avaliableBitsInByte > bitsc) {
		this->bufferv[this->byte_offset] |= ((bitsv & ((1 << bitsc) - 1)) << this->bit_offset);
		this->bit_offset += bitsc;
	} else {
		this->bufferv[this->byte_offset] |= ((bitsv & ((1 << avaliableBitsInByte) - 1)) << this->bit_offset);
		this->byte_offset += 1;
		this->bit_offset = 0;
		bitstream_append(this, bitsc - avaliableBitsInByte, bitsv >> avaliableBitsInByte);
	}
}

void bitstream_append_struct(struct Bitstream *const this, const struct BitStrShort value) {
	bitstream_append(this, value.len, value.value);
}

void bitstream_fillToByteBoundary(struct Bitstream *const this) {
	if (this->bit_offset) {
		this->byte_offset += 1;
		this->bit_offset = 0;
	}
}
