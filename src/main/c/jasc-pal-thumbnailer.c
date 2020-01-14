#include <arpa/inet.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "bitstream.c"
#include "deflate.c"


const struct option long_options[] = {
	{"help", no_argument, 0, 'h'},
	{"size", required_argument, 0, 's'},
	{"input", required_argument, 0, 'i'},
	{"output", required_argument, 0, 'o'},
	{0}
};
#define PAL_MAGIC "JASC-PAL"
const char PNG_MAGIC[8] = "\x89PNG\r\n\x1a\n";
const uint32_t PNG_CRC_POLYNOMIAL = 0xEDB88320;

struct Options {
	bool help;
	int size;
	char *input;
	char *output;
};

struct RGB {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};
struct Palette {
	size_t count;
	struct RGB *colors;
};
struct IHDR {
	uint32_t width;
	uint32_t height;
	uint8_t bitDepth;
	uint8_t colorType;
	uint8_t compressionMethod;
	uint8_t filterMethod;
	uint8_t interlaceMethod;
};

void parse_input(struct Palette *const retval, const char *const filename) {
	FILE *f;
	if (filename) {
		f = fopen(filename, "rb");
		if (!f) {
			fprintf(stderr, "Could not open file %s", filename);
			exit(1);
		}
	} else {
		f = stdin;
		exit(1);
	}
	int result;
	size_t count;
	struct RGB color;
	result = fscanf(f, PAL_MAGIC "\r\n0100\r\n%zd", &count);
	if (result != 1) {
		fprintf(stderr, "Could not parse file %s\n", filename);
		exit(1);
	}
	retval->colors = calloc(sizeof(struct RGB), count);

	for (size_t i = 0; i < count; i++) {
		result = fscanf(f, "%hhd %hhd %hhd\r\n", &(color.red), &(color.green), &(color.blue));
		if (result != 3) {
			fprintf(stderr, "Could not parse file %s\n", filename);
			exit(1);
		}
		retval->colors[i] = color;
	}
	retval->count = count;
	fclose(f);
}

void fwrite_png_chunk(const uint8_t typ[4], const size_t datac, const uint8_t *const datav, FILE *const f) {
	uint32_t datac_writable = htonl((uint32_t) datac);
	fwrite(&datac_writable, 4, 1, f);
	fwrite(typ, 1, 4, f);
	fwrite(datav, 1, datac, f);

	uint32_t crc = 0xFFFFFFFF;
	for (size_t i = 0; i < 4; i++) {
		crc ^= ((uint32_t) typ[i]);
		for (size_t j = 0; j < 8; j++) {
			crc = ((crc & 1) != 0 ? PNG_CRC_POLYNOMIAL : 0) ^ (crc >> 1);
		}
	}
	for (size_t i = 0; i < datac; i++) {
		crc ^= ((uint32_t) datav[i]);
		for (size_t j = 0; j < 8; j++) {
			crc = ((crc & 1) != 0 ? PNG_CRC_POLYNOMIAL : 0) ^ (crc >> 1);
		}
	}
	crc ^= 0xFFFFFFFF;
	crc = htonl(crc);
	fwrite(&crc, 4, 1, f);
}

void write_output(const struct Palette *const palette, const char *const filename, const size_t dimension) {
	FILE *f;
	if (filename) {
		f = fopen(filename, "wb");
		if (!f) {
			fprintf(stderr, "Could not open file %s", filename);
			exit(1);
		}
	} else {
		f = stdout;
	}
	size_t swatchesX = (size_t) floor(sqrt((double) palette->count));
	size_t swatchesY = palette->count / swatchesX + (0 == palette->count % swatchesX ? 0 : 1);
	size_t swatchWidth = dimension / swatchesX;
	size_t swatchHeight = dimension / swatchesY;
	//bool needs_transparent = (palette->count != swatchesX * swatchesY);
	size_t post_dim_x = swatchesX * swatchWidth;
	size_t post_dim_y = swatchesY * swatchHeight;

	const uint8_t PNG_COLOR_TYPE_PAL = 3;
	fwrite(PNG_MAGIC, 1, 8, f);
	struct IHDR ihdr = {htonl(post_dim_x), htonl(post_dim_y), 8, PNG_COLOR_TYPE_PAL, 0, 0, 0};
	fwrite_png_chunk((uint8_t*) "IHDR", 0x0d, (uint8_t*) &ihdr, f);
	fwrite_png_chunk((uint8_t*) "PLTE", 3 * palette->count, (uint8_t*) palette->colors, f);
	struct Bitstream idat = {0};
	struct Adler32 adler = ADLER32_INIT;
	bitstream_init(&idat);
	 /// zlib header
	bitstream_append(&idat, 8, 0x78);
	bitstream_append(&idat, 8, 1);
	/// deflate header
	bitstream_append(&idat, 1, 1); // last chunk
	bitstream_append(&idat, 2, 1); // used fixed codes
	/*
	Turns out, natulus rencodes the thumbnailer's output anyway, which means figuring out out the optional compression doesn't actually reduce the size of the file in nautilus's cache

	And since I have to figure out the raw stream to figure out the checksum anyway, writing out an approximation of the raw stream is easiest.

	So, why do the huffman encoding instead of raw? So that I don't have to split the stream into 0xFFFF-sized chunks. And because I still kinda want to figure out how to write the deflated data, and this will be an easier jumping-off point than the other.
	*/
	/// deflate data
	for (size_t i = 0; i < swatchesY; i++) {
		for (size_t u = 0; u < swatchHeight; u++) {
			bitstream_append_struct(&idat, encode_fixed_huffman_code(0));
			adler32_push(&adler, 0);
			for (size_t j = 0; j < swatchesX; j++) {
				for (size_t v = 0; v < swatchWidth; v++) {
					bitstream_append_struct(&idat, encode_fixed_huffman_code(j + swatchesX * i));
					adler32_push(&adler, j + swatchesX * i);
				}
			}
		}
	}
	bitstream_append_struct(&idat, encode_fixed_huffman_code(256));
	// zlib checksum
	bitstream_fillToByteBoundary(&idat);
	bitstream_append(&idat, 8, adler.s2 >> 8);
	bitstream_append(&idat, 8, adler.s2);
	bitstream_append(&idat, 8, adler.s1 >> 8);
	bitstream_append(&idat, 8, adler.s1);
	fwrite_png_chunk((uint8_t*) "IDAT", idat.byte_offset, idat.bufferv, f);
	bitstream_deinit(&idat);
	fwrite_png_chunk((uint8_t*) "IEND", 0, (uint8_t*) "", f);

	fclose(f);
}

void print_usage() {
	printf("thumbnailer -i infile -o outfile -s outdim\n");
}

int main(int argc, char** argv) {
	int __getopt_long_i__;
	struct Options Options = {0};
	while (1) {
		int opt = getopt_long(argc, argv, "hs:i:o:", long_options, &__getopt_long_i__);
		if (opt == -1) {
			break;
		}
		switch (opt) {
		case 'h':
			Options.help = true;
			break;
		case 's':
			Options.size = atoi(optarg);
			break;
		case 'i':
			Options.input = optarg;
			break;
		case 'o':
			Options.output = optarg;
			break;
		default:
			print_usage();
			exit(1);
			break;
		}
	}

	if (Options.help || Options.size == 0) {
		print_usage();
		return 0;
	}

	struct Palette palette = {0};
	parse_input(&palette, Options.input);
	write_output(&palette, Options.output, Options.size);
	free(palette.colors);

	return 0;
}
