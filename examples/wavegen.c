#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <mathfun.h>
#include "portable_endian.h"

#ifndef M_TAU
#	define M_TAU (2*M_PI)
#endif

static mathfun_value square_wave(const mathfun_value args[]) {
	return fmod(args[0], M_TAU) < M_PI ? 1 : -1;
}

static mathfun_value triangle_wave(const mathfun_value args[]) {
	const mathfun_value x = fmod((args[0] + M_PI_2), M_TAU);
	return x < M_PI ? x * M_2_PI - 1 : 3 - x * M_2_PI;
}

static mathfun_value sawtooth_wave(const mathfun_value args[]) {
	return fmod(args[0], M_TAU) * M_1_PI - 1;
}

static mathfun_value fadein(const mathfun_value args[]) {
	const mathfun_value t = args[0];
	if (t < 0) return 0;
	const mathfun_value x = t / args[1];
	return x < 1 ? x*x : 1;
}

static mathfun_value fadeout(const mathfun_value args[]) {
	const mathfun_value t = args[0];
	const mathfun_value duration = args[1];
	if (t > duration) return 0;
	const mathfun_value x = (t - duration) / duration;
	return x < 1 ? x*x : 1;
}

static mathfun_value mask(const mathfun_value args[]) {
	const mathfun_value t = args[0];
	return t >= 0 && t < args[1] ? 1 : 0;
}

#define RIFF_WAVE_HEADER_SIZE 44

#pragma pack(push, 1)
struct riff_wave_header {
	uint8_t  id[4];
	uint32_t size;
	uint8_t  format[4];
	uint8_t  fmt_chunk_id[4];
	uint32_t fmt_chunk_size;
	uint16_t audio_format;
	uint16_t channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	uint8_t  data_chunk_id[4];
	uint32_t data_chunk_size;
};
#pragma pack(pop)

static unsigned int to_full_byte(int bits) {
	int rem = bits % 8;
	return rem == 0 ? bits : bits + (8 - rem);
}

bool mathfun_wavegen(const char *filename, FILE *stream, uint32_t sample_rate, uint16_t bits_per_sample,
	uint16_t channels, uint32_t samples, const mathfun channel_functs[], bool write_header) {

	if (sample_rate == 0) {
		fprintf(stderr, "illegal sample rate: %u\n", sample_rate);
		return false;
	}

	if (bits_per_sample == 0) {
		fprintf(stderr, "illegal number of bits per sample: %u\n", bits_per_sample);
		return false;
	}

	if (channels == 0) {
		fprintf(stderr, "illegal number of channels: %u\n", channels);
		return false;
	}

	const int mid        = 1 << (bits_per_sample - 1);
	const int max_volume = ~(~0 << (bits_per_sample - 1));
	const unsigned int ceil_bits_per_sample = to_full_byte(bits_per_sample);
	const unsigned int shift                = ceil_bits_per_sample - bits_per_sample;
	const unsigned int bytes_per_sample     = ceil_bits_per_sample / 8;

	uint8_t *sample_buf = malloc(bytes_per_sample);

	if (!sample_buf) {
		perror("allocating sample buffer");
		return false;
	}

	// to boost performance even more pre-allocate a big enough frame instead of
	// allocating a new frame on each function call
	size_t maxframesize = 0;
	for (uint16_t channel = 0; channel < channels; ++ channel) {
		size_t framesize = channel_functs[channel].framesize;
		if (framesize > maxframesize) {
			maxframesize = framesize;
		}
	}

	mathfun_value *frame = calloc(maxframesize, sizeof(mathfun_value));

	if (!frame) {
		perror("allocating frame");
		free(frame);
		return false;
	}

	if (write_header) {
		const uint16_t block_align = channels * bytes_per_sample;
		const uint32_t data_size   = block_align * samples;

		const struct riff_wave_header header = {
			.id              = "RIFF",
			.size            = htole32(36 + data_size),
			.format          = "WAVE",
			.fmt_chunk_id    = "fmt ",
			.fmt_chunk_size  = htole32(16),
			.audio_format    = htole16(1), // PCM
			.channels        = htole16(channels),
			.sample_rate     = htole32(sample_rate),
			.byte_rate       = htole32(sample_rate * block_align),
			.block_align     = htole16(block_align),
			.bits_per_sample = htole16(bits_per_sample),
			.data_chunk_id   = "data",
			.data_chunk_size = htole32(data_size)
		};

		if (fwrite(&header, RIFF_WAVE_HEADER_SIZE, 1, stream) != 1) {
			perror(filename);
			free(frame);
			free(sample_buf);
			return false;
		}
	}

	for (size_t sample = 0; sample < samples; ++ sample) {
		const mathfun_value t = (mathfun_value)sample / (mathfun_value)sample_rate;
		for (size_t channel = 0; channel < channels; ++ channel) {
			const mathfun *funct = channel_functs + channel;
			// arguments are the first cells in frame:
			frame[0] = t;
			frame[1] = sample;
			frame[2] = channel;
			// ignore math errors here (would be in errno)
			int vol = (int)(max_volume * mathfun_exec(funct, frame)) << shift;
			if (bits_per_sample <= 8) {
				vol += mid;
			}
			for (size_t byte = 0; byte < bytes_per_sample; ++ byte) {
				sample_buf[byte] = (vol >> (byte * 8)) & 0xFF;
			}
			if (fwrite(sample_buf, bytes_per_sample, 1, stream) != 1) {
				perror(filename);
				free(frame);
				free(sample_buf);
				return false;
			}
		}
	}

	free(frame);
	free(sample_buf);
	
	return true;
}

bool wavegen(const char *filename, FILE *stream, uint32_t sample_rate, uint16_t bits_per_sample,
	uint16_t channels, uint32_t samples, const char *channel_functs[], bool write_header) {
	mathfun_context ctx;
	mathfun_error_p error = NULL;

	if (!mathfun_context_init(&ctx, true, &error) ||
		!mathfun_context_define_funct(&ctx, "sq",      square_wave,   1, &error) ||
		!mathfun_context_define_funct(&ctx, "tri",     triangle_wave, 1, &error) ||
		!mathfun_context_define_funct(&ctx, "saw",     sawtooth_wave, 1, &error) ||
		!mathfun_context_define_funct(&ctx, "fadein",  fadein,        2, &error) ||
		!mathfun_context_define_funct(&ctx, "fadeout", fadeout,       2, &error) ||
		!mathfun_context_define_funct(&ctx, "mask",    mask,          2, &error)) {
		mathfun_error_log_and_cleanup(&error, stderr);
		return false;
	}

	mathfun *functs = calloc(channels, sizeof(mathfun));
	// t ... time in seconds
	// s ... sample
	// c ... channel
	const char *argnames[] = { "t", "s", "c" };
	for (size_t i = 0; i < channels; ++ i) {
		if (!mathfun_context_compile(&ctx, argnames, 3, channel_functs[i], functs + i, &error)) {
			mathfun_error_log_and_cleanup(&error, stderr);
			for (; i > 0; -- i) {
				mathfun_cleanup(functs + i - 1);
			}
			free(functs);
			mathfun_context_cleanup(&ctx);
			return false;
		}
	}

	bool ok = mathfun_wavegen(filename, stream, sample_rate, bits_per_sample, channels,
		samples, functs, write_header);
	
	for (size_t i = channels; i > 0; -- i) {
		mathfun_cleanup(functs + i - 1);
	}
	free(functs);
	mathfun_context_cleanup(&ctx);

	return ok;
}

void usage(int argc, const char *argv[]) {
	printf(
		"Usage: %s <wave-filename> <sample-rate> <bits-per-sample> <samples> <wave-function>...\n",
		argc > 0 ? argv[0] : "wavegen");
}

int main(int argc, const char *argv[]) {
	if (argc < 6) {
		fprintf(stderr, "error: too few arguments\n");
		usage(argc, argv);
		return 1;
	}

	const char *filename = argv[1];
	char *endptr = NULL;

	unsigned long int sample_rate = strtoul(argv[2], &endptr, 10);
	if (!*argv[2] || *endptr || sample_rate > UINT32_MAX) {
		fprintf(stderr, "illegal value for sample rate: %s\n", argv[2]);
		return 1;
	}

	unsigned long int bits_per_sample = strtoul(argv[3], &endptr, 10);
	if (!*argv[3] || *endptr || bits_per_sample > UINT16_MAX) {
		fprintf(stderr, "illegal value for bits per sample: %s\n", argv[3]);
		return 1;
	}

	unsigned long int samples = strtoul(argv[4], &endptr, 10);
	if (!*argv[4] || *endptr || samples > UINT32_MAX) {
		fprintf(stderr, "illegal value for samples: %s\n", argv[4]);
		return 1;
	}

	const char **functs = argv + 5;
	int channels = argc - 5;
	if (channels > UINT16_MAX) {
		fprintf(stderr, "too many channels: %d\n", channels);
		return 1;
	}

	bool ok;
	if (strcmp(filename, "-") == 0) {
		ok = wavegen("<stdout>", stdout, sample_rate, bits_per_sample, channels, samples, functs, true);
	}
	else {
		FILE *stream = fopen(filename, "wb");

		if (!stream) {
			perror(filename);
			return 1;
		}
		
		ok = wavegen(filename, stream, sample_rate, bits_per_sample, channels, samples, functs, true);

		fclose(stream);
	}

	return ok ? 0 : 1;
}
