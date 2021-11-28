#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <mathfun.h>
#include "portable_endian.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef M_TAU
#	define M_TAU (2*M_PI)
#endif

static mathfun_value square_wave(const mathfun_value args[]) {
	return (mathfun_value){ .number = fmod(args[0].number, M_TAU) < M_PI ? 1 : -1 };
}

static mathfun_value triangle_wave(const mathfun_value args[]) {
	const double x = fmod((args[0].number + M_PI_2), M_TAU);
	return (mathfun_value){ .number = x < M_PI ? x * M_2_PI - 1 : 3 - x * M_2_PI };
}

static mathfun_value sawtooth_wave(const mathfun_value args[]) {
	return (mathfun_value){ .number = fmod(args[0].number, M_TAU) * M_1_PI - 1 };
}

static mathfun_value fadein(const mathfun_value args[]) {
	const double t = args[0].number;
	if (t < 0) return (mathfun_value){ .number = 0.0 };
	const double duration = args[1].number;
	if (duration < t) return (mathfun_value){ .number = 1.0 };
	const double x = t / duration;
	return (mathfun_value){ .number = x*x };
}

static mathfun_value fadeout(const mathfun_value args[]) {
	double t = args[0].number;
	const double duration = args[1].number;
	if (t > duration) return (mathfun_value){ .number = 0.0 };
	if (t < 0.0) return (mathfun_value){ .number = 1.0 };
	const double x = (t - duration) / duration;
	return (mathfun_value){ .number = x*x };
}

static mathfun_value mask(const mathfun_value args[]) {
	const double t = args[0].number;
	return (mathfun_value){ .number = t >= 0 && t < args[1].number ? 1.0 : 0.0 };
}

static mathfun_value clamp(const mathfun_value args[]) {
	const double x = args[0].number;
	const double min = args[1].number;
	const double max = args[2].number;
	return (mathfun_value){ .number = x < min ? min : x > max ? max : x };
}

static mathfun_value pop(const mathfun_value args[]) {
	const double t = args[0].number;
	const double wavelength = args[1].number;
	const double half_wavelength = wavelength * 0.5;
	const double amplitude = args[2].number;

	return (mathfun_value){ .number =
		t >= 0.0 && t < half_wavelength ? amplitude : t >= half_wavelength && t < wavelength ? -amplitude : 0.0
	};
}

static mathfun_value drop(const mathfun_value args[]) {
	return (mathfun_value){ .number = args[0].number == 0.0 ? 0.0 : 1.0 };
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
	const int max_volume = ~(~((uint32_t)0) << (bits_per_sample - 1));
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
		free(sample_buf);
		return false;
	}

	if (write_header) {
		const uint16_t block_align = channels * bytes_per_sample;

		const struct riff_wave_header header = {
			.id              = "RIFF",
			.size            = htole32(UINT32_MAX),
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
			.data_chunk_size = htole32(UINT32_MAX - 36)
		};

		if (fwrite(&header, RIFF_WAVE_HEADER_SIZE, 1, stream) != 1) {
			perror(filename);
			free(frame);
			free(sample_buf);
			return false;
		}
	}

	for (size_t sample = 0; sample < samples; ++ sample) {
		const double t = (double)sample / (double)sample_rate;
		const double r = t * M_TAU;
		for (size_t channel = 0; channel < channels; ++ channel) {
			const mathfun *funct = channel_functs + channel;
			// arguments are the first cells in frame:
			frame[0].number = t;
			frame[1].number = r;
			frame[2].number = sample;
			frame[3].number = channel;
			// ignore math errors here (would be in errno)
			double value = mathfun_exec(funct, frame);
			if (value > 1.0) value = 1.0;
			else if (value < -1.0) value = -1.0;
			int vol = (int)(max_volume * value) << shift;
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

bool livewave(const char *filename, uint32_t sample_rate, uint16_t bits_per_sample, uint32_t samples) {
	FILE *fp = fopen(filename, "r");

	if (!fp) {
		perror(filename);
		return 1;
	}

	const mathfun_sig sig1 = {1, (mathfun_type[]){MATHFUN_NUMBER}, MATHFUN_NUMBER};
	const mathfun_sig sig2 = {2, (mathfun_type[]){MATHFUN_NUMBER, MATHFUN_NUMBER}, MATHFUN_NUMBER};
	const mathfun_sig sig3 = {3, (mathfun_type[]){MATHFUN_NUMBER, MATHFUN_NUMBER, MATHFUN_NUMBER}, MATHFUN_NUMBER};

	mathfun_context ctx;
	mathfun_error_p error = NULL;

	if (!mathfun_context_init(&ctx, true, &error) ||
		!mathfun_context_define_const(&ctx, "C0",  16.35, &error) ||
		!mathfun_context_define_const(&ctx, "Cs0", 17.32, &error) ||
		!mathfun_context_define_const(&ctx, "D0",  18.35, &error) ||
		!mathfun_context_define_const(&ctx, "Ds0", 19.45, &error) ||
		!mathfun_context_define_const(&ctx, "E0",  20.60, &error) ||
		!mathfun_context_define_const(&ctx, "F0",  21.83, &error) ||
		!mathfun_context_define_const(&ctx, "Fs0", 23.12, &error) ||
		!mathfun_context_define_const(&ctx, "G0",  24.50, &error) ||
		!mathfun_context_define_const(&ctx, "Gs0", 25.96, &error) ||
		!mathfun_context_define_const(&ctx, "A0",  27.50, &error) ||
		!mathfun_context_define_const(&ctx, "As0", 29.14, &error) ||
		!mathfun_context_define_const(&ctx, "B0",  30.87, &error) ||
		!mathfun_context_define_const(&ctx, "C1",  32.70, &error) ||
		!mathfun_context_define_const(&ctx, "Cs1", 34.65, &error) ||
		!mathfun_context_define_const(&ctx, "D1",  36.71, &error) ||
		!mathfun_context_define_const(&ctx, "Ds1", 38.89, &error) ||
		!mathfun_context_define_const(&ctx, "E1",  41.20, &error) ||
		!mathfun_context_define_const(&ctx, "F1",  43.65, &error) ||
		!mathfun_context_define_const(&ctx, "Fs1", 46.25, &error) ||
		!mathfun_context_define_const(&ctx, "G1",  49.00, &error) ||
		!mathfun_context_define_const(&ctx, "Gs1", 51.91, &error) ||
		!mathfun_context_define_const(&ctx, "A1",  55.00, &error) ||
		!mathfun_context_define_const(&ctx, "As1", 58.27, &error) ||
		!mathfun_context_define_const(&ctx, "B1",  61.74, &error) ||
		!mathfun_context_define_const(&ctx, "C2",  65.41, &error) ||
		!mathfun_context_define_const(&ctx, "Cs2", 69.30, &error) ||
		!mathfun_context_define_const(&ctx, "D2",  73.42, &error) ||
		!mathfun_context_define_const(&ctx, "Ds2", 77.78, &error) ||
		!mathfun_context_define_const(&ctx, "E2",  82.41, &error) ||
		!mathfun_context_define_const(&ctx, "F2",  87.31, &error) ||
		!mathfun_context_define_const(&ctx, "Fs2", 92.50, &error) ||
		!mathfun_context_define_const(&ctx, "G2",  98.00, &error) ||
		!mathfun_context_define_const(&ctx, "Gs2", 103.83, &error) ||
		!mathfun_context_define_const(&ctx, "A2",  110.00, &error) ||
		!mathfun_context_define_const(&ctx, "As2", 116.54, &error) ||
		!mathfun_context_define_const(&ctx, "B2",  123.47, &error) ||
		!mathfun_context_define_const(&ctx, "C3",  130.81, &error) ||
		!mathfun_context_define_const(&ctx, "Cs3", 138.59, &error) ||
		!mathfun_context_define_const(&ctx, "D3",  146.83, &error) ||
		!mathfun_context_define_const(&ctx, "Ds3", 155.56, &error) ||
		!mathfun_context_define_const(&ctx, "E3",  164.81, &error) ||
		!mathfun_context_define_const(&ctx, "F3",  174.61, &error) ||
		!mathfun_context_define_const(&ctx, "Fs3", 185.00, &error) ||
		!mathfun_context_define_const(&ctx, "G3",  196.00, &error) ||
		!mathfun_context_define_const(&ctx, "Gs3", 207.65, &error) ||
		!mathfun_context_define_const(&ctx, "A3",  220.00, &error) ||
		!mathfun_context_define_const(&ctx, "As3", 233.08, &error) ||
		!mathfun_context_define_const(&ctx, "B3",  246.94, &error) ||
		!mathfun_context_define_const(&ctx, "C4",  261.63, &error) ||
		!mathfun_context_define_const(&ctx, "Cs4", 277.18, &error) ||
		!mathfun_context_define_const(&ctx, "D4",  293.66, &error) ||
		!mathfun_context_define_const(&ctx, "Ds4", 311.13, &error) ||
		!mathfun_context_define_const(&ctx, "E4",  329.63, &error) ||
		!mathfun_context_define_const(&ctx, "F4",  349.23, &error) ||
		!mathfun_context_define_const(&ctx, "Fs4", 369.99, &error) ||
		!mathfun_context_define_const(&ctx, "G4",  392.00, &error) ||
		!mathfun_context_define_const(&ctx, "Gs4", 415.30, &error) ||
		!mathfun_context_define_const(&ctx, "A4",  440.00, &error) ||
		!mathfun_context_define_const(&ctx, "As4", 466.16, &error) ||
		!mathfun_context_define_const(&ctx, "B4",  493.88, &error) ||
		!mathfun_context_define_const(&ctx, "C5",  523.25, &error) ||
		!mathfun_context_define_const(&ctx, "Cs5", 554.37, &error) ||
		!mathfun_context_define_const(&ctx, "D5",  587.33, &error) ||
		!mathfun_context_define_const(&ctx, "Ds5", 622.25, &error) ||
		!mathfun_context_define_const(&ctx, "E5",  659.25, &error) ||
		!mathfun_context_define_const(&ctx, "F5",  698.46, &error) ||
		!mathfun_context_define_const(&ctx, "Fs5", 739.99, &error) ||
		!mathfun_context_define_const(&ctx, "G5",  783.99, &error) ||
		!mathfun_context_define_const(&ctx, "Gs5", 830.61, &error) ||
		!mathfun_context_define_const(&ctx, "A5",  880.00, &error) ||
		!mathfun_context_define_const(&ctx, "As5", 932.33, &error) ||
		!mathfun_context_define_const(&ctx, "B5",  987.77, &error) ||
		!mathfun_context_define_const(&ctx, "C6",  1046.50, &error) ||
		!mathfun_context_define_const(&ctx, "Cs6", 1108.73, &error) ||
		!mathfun_context_define_const(&ctx, "D6",  1174.66, &error) ||
		!mathfun_context_define_const(&ctx, "Ds6", 1244.51, &error) ||
		!mathfun_context_define_const(&ctx, "E6",  1318.51, &error) ||
		!mathfun_context_define_const(&ctx, "F6",  1396.91, &error) ||
		!mathfun_context_define_const(&ctx, "Fs6", 1479.98, &error) ||
		!mathfun_context_define_const(&ctx, "G6",  1567.98, &error) ||
		!mathfun_context_define_const(&ctx, "Gs6", 1661.22, &error) ||
		!mathfun_context_define_const(&ctx, "A6",  1760.00, &error) ||
		!mathfun_context_define_const(&ctx, "As6", 1864.66, &error) ||
		!mathfun_context_define_const(&ctx, "B6",  1975.53, &error) ||
		!mathfun_context_define_const(&ctx, "C7",  2093.00, &error) ||
		!mathfun_context_define_const(&ctx, "Cs7", 2217.46, &error) ||
		!mathfun_context_define_const(&ctx, "D7",  2349.32, &error) ||
		!mathfun_context_define_const(&ctx, "Ds7", 2489.02, &error) ||
		!mathfun_context_define_const(&ctx, "E7",  2637.02, &error) ||
		!mathfun_context_define_const(&ctx, "F7",  2793.83, &error) ||
		!mathfun_context_define_const(&ctx, "Fs7", 2959.96, &error) ||
		!mathfun_context_define_const(&ctx, "G7",  3135.96, &error) ||
		!mathfun_context_define_const(&ctx, "Gs7", 3322.44, &error) ||
		!mathfun_context_define_const(&ctx, "A7",  3520.00, &error) ||
		!mathfun_context_define_const(&ctx, "As7", 3729.31, &error) ||
		!mathfun_context_define_const(&ctx, "B7",  3951.07, &error) ||
		!mathfun_context_define_const(&ctx, "C8",  4186.01, &error) ||
		!mathfun_context_define_const(&ctx, "Cs8", 4434.92, &error) ||
		!mathfun_context_define_const(&ctx, "D8",  4698.63, &error) ||
		!mathfun_context_define_const(&ctx, "Ds8", 4978.03, &error) ||
		!mathfun_context_define_const(&ctx, "E8",  5274.04, &error) ||
		!mathfun_context_define_const(&ctx, "F8",  5587.65, &error) ||
		!mathfun_context_define_const(&ctx, "Fs8", 5919.91, &error) ||
		!mathfun_context_define_const(&ctx, "G8",  6271.93, &error) ||
		!mathfun_context_define_const(&ctx, "Gs8", 6644.88, &error) ||
		!mathfun_context_define_const(&ctx, "A8",  7040.00, &error) ||
		!mathfun_context_define_const(&ctx, "As8", 7458.62, &error) ||
		!mathfun_context_define_const(&ctx, "B8",  7902.13, &error) ||

		!mathfun_context_define_funct(&ctx, "sq",      square_wave,   &sig1, &error) ||
		!mathfun_context_define_funct(&ctx, "tri",     triangle_wave, &sig1, &error) ||
		!mathfun_context_define_funct(&ctx, "saw",     sawtooth_wave, &sig1, &error) ||
		!mathfun_context_define_funct(&ctx, "fadein",  fadein,        &sig2, &error) ||
		!mathfun_context_define_funct(&ctx, "fadeout", fadeout,       &sig2, &error) ||
		!mathfun_context_define_funct(&ctx, "mask",    mask,          &sig2, &error) ||
		!mathfun_context_define_funct(&ctx, "clamp",   clamp,         &sig3, &error) ||
		!mathfun_context_define_funct(&ctx, "pop",     pop,           &sig3, &error) ||
		!mathfun_context_define_funct(&ctx, "drop",    drop,          &sig1, &error)) {
		mathfun_error_log_and_cleanup(&error, stderr);
		fclose(fp);
		return false;
	}

	mathfun funct = MATHFUN_INIT;

	// t ... time in seconds
	// r ... t * 2 * pi
	// s ... sample
	// c ... channel
	const char *argnames[] = { "t", "r", "s", "c" };
	char *buffer = NULL;
	size_t buffer_size = 0;
	time_t last_mtime = 0;

	bool ok = true;
	bool first = true;
	while (ok) {
		struct stat meta;

		if (fstat(fileno(fp), &meta) != 0) {
			perror(filename);
			mathfun_cleanup(&funct);
			mathfun_context_cleanup(&ctx);
			fclose(fp);
			free(buffer);
			return false;
		}

		if (meta.st_ctime > last_mtime + 1) {
			fprintf(stderr, "loading: ");
			const size_t needed_size = (size_t)meta.st_size + 1;
			if (buffer_size < needed_size) {
				free(buffer);
				buffer = malloc(meta.st_size + 1);
				if (!buffer) {
					perror("allocating source buffer");
					mathfun_cleanup(&funct);
					mathfun_context_cleanup(&ctx);
					fclose(fp);
					free(buffer);
					return false;
				}
				buffer_size = needed_size;
			}

			fclose(fp); // why do I need to re-open??
			fp = fopen(filename, "r");
			if (!fp) {
				perror(filename);
				mathfun_cleanup(&funct);
				mathfun_context_cleanup(&ctx);
				fclose(fp);
				free(buffer);
				return false;
			}

			if (fread(buffer, meta.st_size, 1, fp) != 1) {
				perror(filename);
			} else {
				buffer[meta.st_size] = 0;
				if (meta.st_size == 0 || buffer[meta.st_size - 1] != '\n') {
					fprintf(stderr, "%s\n", buffer);
				} else {
					fprintf(stderr, "%s", buffer);
				}
				mathfun tmp = MATHFUN_INIT;
				if (mathfun_context_compile(&ctx, argnames, 4, buffer, &tmp, &error)) {
					mathfun_cleanup(&funct);
					funct = tmp;
					last_mtime = meta.st_ctime;
				} else {
					mathfun_error_log_and_cleanup(&error, stderr);
					mathfun_cleanup(&tmp);
				}
			}
		}

		ok = mathfun_wavegen("<stdout>", stdout, sample_rate, bits_per_sample, 1, samples, &funct, first);
		first = false;
	}

	mathfun_cleanup(&funct);
	mathfun_context_cleanup(&ctx);
	fclose(fp);
	free(buffer);

	return ok;
}

static void usage(int argc, const char *argv[]) {
	printf(
		"Usage: %s <function-filename> <sample-rate> <bits-per-sample> <samples>\n",
		argc > 0 ? argv[0] : "livewave");
}

static bool parse_uint16(const char *str, uint16_t *valueptr) {
	char *endptr = NULL;
	unsigned long int value = strtoul(str, &endptr, 10);
	if (endptr == str || value > UINT16_MAX) return false;
	while (isspace(*endptr)) ++ endptr;
	if (*endptr) return false;
	*valueptr = value;
	return true;
}

static bool parse_uint32(const char *str, uint32_t *valueptr) {
	char *endptr = NULL;
	unsigned long int value = strtoul(str, &endptr, 10);
	if (endptr == str || value > UINT32_MAX) return false;
	while (isspace(*endptr)) ++ endptr;
	if (*endptr) return false;
	*valueptr = value;
	return true;
}

static bool parse_samples(const char *str, uint32_t sample_rate, uint32_t *samplesptr) {
	if (!*str) return false;
	const char *ptr = strrchr(str,':');
	const char *endptr = NULL;
	if (ptr) {
		const double sec = strtod(++ptr, (char**)&endptr);
		unsigned long int min = 0;
		unsigned long int hours = 0;
		if (ptr == endptr) return false;
		while (isspace(*endptr)) ++ endptr;
		if (*endptr) return false;
		ptr -= 2;
		while (ptr > str && *ptr != ':') -- ptr;
		if (ptr >= str) {
			min = strtoul(*ptr == ':' ? ptr+1 : ptr, (char**)&endptr, 10);
			if (endptr == ptr || *endptr != ':') return false;
			if (ptr > str) {
				hours = strtoul(str, (char**)&endptr, 10);
				if (str == endptr || endptr != ptr) return false;
			}
		}
		unsigned long int samples = (((hours * 60 + min) * 60) + sec) * sample_rate;
		if (samples > UINT32_MAX) return false;
		*samplesptr = samples;
		return true;
	}
	endptr = str + strlen(str) - 1;
	while (endptr > str && isspace(*endptr)) -- endptr;
	ptr = endptr;
	++ endptr;
	while (ptr > str && isalpha(*ptr)) -- ptr;
	++ ptr;
	size_t sufflen = endptr - ptr;
	if ((sufflen == 2 && strncmp("ms",ptr,sufflen) == 0) || (sufflen == 4 && strncmp("msec",ptr,sufflen) == 0)) {
		const double msec = strtod(str, (char**)&endptr);
		if (endptr == str || msec < 0) return false;
		while (isspace(*endptr)) ++ endptr;
		if (endptr != ptr) return false;
		const double samples = sample_rate * msec / 1000.0;
		if (samples > UINT32_MAX) return false;
		*samplesptr = (uint32_t)samples;
	}
	else if ((sufflen == 1 && strncmp("s",ptr,sufflen) == 0) || (sufflen == 3 && strncmp("sec",ptr,sufflen) == 0)) {
		const double sec = strtod(str, (char**)&endptr);
		if (endptr == str || sec < 0) return false;
		while (isspace(*endptr)) ++ endptr;
		if (endptr != ptr) return false;
		const double samples = sample_rate * sec;
		if (samples > UINT32_MAX) return false;
		*samplesptr = (uint32_t)samples;
	}
	else if ((sufflen == 1 && strncmp("m",ptr,sufflen) == 0) || (sufflen == 3 && strncmp("min",ptr,sufflen) == 0)) {
		const double min = strtod(str, (char**)&endptr);
		if (endptr == str || min < 0) return false;
		while (isspace(*endptr)) ++ endptr;
		if (endptr != ptr) return false;
		const double samples = sample_rate * min * 60;
		if (samples > UINT32_MAX) return false;
		*samplesptr = (uint32_t)samples;
	}
	else {
		return parse_uint32(str, samplesptr);
	}
	return true;
}

int main(int argc, const char *argv[]) {
	if (argc != 5) {
		fprintf(stderr, "error: too few arguments\n");
		usage(argc, argv);
		return 1;
	}

	const char *filename = argv[1];

	uint32_t sample_rate = 0;
	if (!parse_uint32(argv[2], &sample_rate) || sample_rate == 0) {
		fprintf(stderr, "illegal value for sample rate: %s\n", argv[2]);
		return 1;
	}

	uint16_t bits_per_sample = 0;
	if (!parse_uint16(argv[3], &bits_per_sample) || bits_per_sample == 0) {
		fprintf(stderr, "illegal value for bits per sample: %s\n", argv[3]);
		return 1;
	}

	uint32_t samples = 0;
	if (!parse_samples(argv[4], sample_rate, &samples)) {
		fprintf(stderr, "illegal value for samples: %s\n", argv[4]);
		return 1;
	}

	bool ok = livewave(filename, sample_rate, bits_per_sample, samples);

	return ok ? 0 : 1;
}
