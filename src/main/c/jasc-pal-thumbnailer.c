#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "bitstream.h"
#include "deflate.h"

#ifdef _MSC_VER
	uint32_t htonl(uint32_t in) {
		// MSVC only does i686, x86_64 or ARMEL, all little endian
		return
			((in & 0xFF000000) >> 24) |
			((in & 0x00FF0000) >> 8) |
			((in & 0x0000FF00) << 8) |
			((in & 0x000000FF) << 24);
	}
#else
	#include <arpa/inet.h>
#endif

const char PROGRAM_NAME[] = "jasc-pal-thumbnailer";
const char PROGRAM_VERSION[] = "0000.00.00";
const char PROGRAM_HOMEPAGE[] = "https://rayrobdod.name/programming/programs/jascPaletteThumbnailer/";
const char PROGRAM_DESCRIPTION[] = "A thumbnailer for JASC-PAL files";

#define PAL_MAGIC "JASC-PAL"
const char PNG_MAGIC[8] = "\x89PNG\r\n\x1a\n";
const uint32_t PNG_CRC_POLYNOMIAL = 0xEDB88320;

/** Deserialized program options */
struct Options {
	enum {FREE, SIZE, INPUT, OUTPUT, FORCE_POSITIONAL} next_argument;
	bool help;
	bool version;
	int size;
	const char *program_name;
	const char *input;
	const char *output;
};

/** A 24-bit color */
struct RGB {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};
/** A set of colors */
struct Palette {
	size_t count;
	struct RGB *colors;
};
/** The payload of a png header chunk */
struct IHDR {
	uint32_t width;
	uint32_t height;
	uint8_t bitDepth;
	uint8_t colorType;
	uint8_t compressionMethod;
	uint8_t filterMethod;
	uint8_t interlaceMethod;
};

/** Parse a jasc-pal file, and put the contents into a Palette */
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

/** Write a png chunk to the given file */
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

/** Writes a png file, with the given pixel dimension, representing the given palette, to the given file */
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
	 * Turns out, GNOME ThumbnailFactory rencodes the thumbnailer's output anyway,
	 * which means figuring out the optimal compression doesn't actually reduce the size of the file in the thumbnail cache
	 */
	/// deflate data
	for (size_t i = 0; i < swatchesY; i++) {
		for (size_t u = 0; u < swatchHeight; u++) {
			bitstream_append_struct(&idat, encode_fixed_huffman_code(0));
			adler32_push(&adler, 0);
			for (size_t j = 0; j < swatchesX; j++) {
				if (swatchWidth >= 4) {
					bitstream_append_struct(&idat, encode_fixed_huffman_code(j + swatchesX * i));
					bitstream_append_struct(&idat, encode_length(swatchWidth - 1));
					bitstream_append_struct(&idat, encode_offset(0));
				} else {
					for (size_t v = 0; v < swatchWidth; v++) {
						bitstream_append_struct(&idat, encode_fixed_huffman_code(j + swatchesX * i));
					}
				}
				for (size_t v = 0; v < swatchWidth; v++) {
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


/** Print a usage statement */
void print_usage(const char *const program_name) {
	printf("  %s -s outdim [-i infile] [-o outfile]\n", program_name);
	printf("  %s --help|--version\n", program_name);
	printf("\n");
	printf("%s\n", PROGRAM_DESCRIPTION);
	printf("\n");
	printf("  %-3s %-20s %-s\n", "-?,", "--help", "Display this help message");
	printf("  %-3s %-20s %-s\n", "", "--version", "Display program version");
	printf("  %-3s %-20s %-s\n", "-s,", "--size", "The dimensions of the output file.");
	printf("  %-3s %-20s %-s\n", "-i,", "--input", "The input file. stdin if not specified.");
	printf("  %-3s %-20s %-s\n", "-o,", "--output", "The output file. stdout if not specified.");
}

/**
 * Append the given arg to the options struct
 *
 * This should be called in a loop for each argv in order from zeroth to argc-minus-oneth
 */
void options_push(struct Options *const options, const char* const arg) {
	char arg_zeroth_char = (arg ? arg[0] : '\0');
	switch (options->next_argument) {
	case SIZE:
		options->size = atoi(arg);
		options->next_argument = FREE;
		break;
	case INPUT:
		options->input = arg;
		options->next_argument = FREE;
		break;
	case OUTPUT:
		options->output = arg;
		options->next_argument = FREE;
		break;
	case FORCE_POSITIONAL:
		goto positional;
	case FREE:
		if (arg_zeroth_char != '-') {
			goto positional;
		} else if (0 == strcmp(arg, "--")) {
			options->next_argument = FORCE_POSITIONAL;
		} else if (0 == strcmp(arg, "--size") || 0 == strcmp(arg, "-s")) {
			options->next_argument = SIZE;
		} else if (0 == strcmp(arg, "--input") || 0 == strcmp(arg, "-i")) {
			options->next_argument = INPUT;
		} else if (0 == strcmp(arg, "--output") || 0 == strcmp(arg, "-o")) {
			options->next_argument = OUTPUT;
		} else if (0 == strcmp(arg, "--version")) {
			options->version = true;
		} else if (0 == strcmp(arg, "--help") || 0 == strcmp(arg, "-?") || 0 == strcmp(arg, "-?")) {
			options->help = true;
		} else {
			fprintf(stderr, "Unknown flag\n");
			exit(1);
		}
	}
	return;

positional:
	if (NULL == options->program_name) {
		options->program_name = arg;
	} else {
		fprintf(stderr, "Too many positional arguments\n");
		exit(1);
	}
}


int main(int argc, char** argv) {
	struct Options Options = {0};
	for (int i = 0; i < argc; i++) {
		options_push(&Options, argv[i]);
	}

	if (Options.help) {
		print_usage(argv[0]);
		return 0;
	}
	if (Options.version) {
		printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
		printf("%s\n", PROGRAM_HOMEPAGE);
		return 0;
	}
	if (Options.size == 0) {
		print_usage(argv[0]);
		return 0;
	}

	struct Palette palette = {0};
	parse_input(&palette, Options.input);
	write_output(&palette, Options.output, Options.size);
	free(palette.colors);

	return 0;
}
