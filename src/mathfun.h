#ifndef MATHFUN_H__
#define MATHFUN_H__

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>

#define _MATHFUN_STR(V) #V
#define MATHFUN_STR(V) _MATHFUN_STR(V)

#define MATHFUN_VERSION_MAJOR 1
#define MATHFUN_VERSION_MINOR 0
#define MATHFUN_VERSION_PATCH 0

#define MATHFUN_VERSION \
	MATHFUN_STR(MATHFUN_VERSION_MAJOR) "." \
	MATHFUN_STR(MATHFUN_VERSION_MINOR) "." \
	MATHFUN_STR(MATHFUN_VERSION_PATCH)

#define MATHFUN_IS_PARSER_ERROR(ERROR) ( \
	(ERROR) >= MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS && \
	(ERROR) <= MATHFUN_PARSER_TRAILING_GARBAGE)

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t mathfun_code;
typedef double mathfun_value;
typedef mathfun_value (*mathfun_binding_funct)(const mathfun_value args[]);

enum mathfun_error_type {
	MATHFUN_OK = 0,
	MATHFUN_IO_ERROR,
	MATHFUN_MEMORY_ERROR,
	MATHFUN_MATH_ERROR,
	MATHFUN_C_ERROR,
	MATHFUN_ILLEGAL_NAME,
	MATHFUN_NAME_EXISTS,
	MATHFUN_NO_SUCH_NAME,
	MATHFUN_TOO_MANY_ARGUMENTS,
	MATHFUN_EXCEEDS_MAX_FRAME_SIZE,
	MATHFUN_INTERNAL_ERROR,
	MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS,
	MATHFUN_PARSER_UNDEFINED_REFERENCE,
	MATHFUN_PARSER_NOT_A_FUNCTION,
	MATHFUN_PARSER_NOT_A_VARIABLE,
	MATHFUN_PARSER_ILLEGAL_NUMBER_OF_ARGUMENTS,
	MATHFUN_PARSER_EXPECTED_NUMBER,
	MATHFUN_PARSER_EXPECTED_IDENTIFIER,
	MATHFUN_PARSER_TRAILING_GARBAGE
};

struct mathfun_decl;

struct mathfun_context {
	struct mathfun_decl *decls;
	size_t decl_capacity;
	size_t decl_used;
};

struct mathfun {
	size_t        argc;
	size_t        framesize;
	mathfun_code *code;
};

bool mathfun_context_init(struct mathfun_context *ctx, bool define_default);
void mathfun_context_cleanup(struct mathfun_context *ctx);
bool mathfun_context_define_default(struct mathfun_context *ctx);
bool mathfun_context_define_const(struct mathfun_context *ctx, const char *name, mathfun_value value);
bool mathfun_context_define_funct(struct mathfun_context *ctx, const char *name, mathfun_binding_funct funct, size_t argc);
const char *mathfun_context_funct_name(const struct mathfun_context *ctx, mathfun_binding_funct funct);
bool mathfun_context_undefine(struct mathfun_context *ctx, const char *name);
bool mathfun_context_compile(const struct mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code,
	struct mathfun *mathfun);

void mathfun_cleanup(struct mathfun *mathfun);
bool mathfun_compile(struct mathfun *mathfun, const char *argnames[], size_t argc, const char *code);
mathfun_value mathfun_call(const struct mathfun *mathfun, ...);
mathfun_value mathfun_acall(const struct mathfun *mathfun, const mathfun_value args[]);
mathfun_value mathfun_vcall(const struct mathfun *mathfun, va_list ap);
bool mathfun_dump(const struct mathfun *mathfun, FILE *stream, const struct mathfun_context *ctx);

mathfun_value mathfun_run(const char *code, ...);
mathfun_value mathfun_arun(const char *argnames[], size_t argc, const char *code, const mathfun_value args[]);

enum mathfun_error_type mathfun_error_type();
int    mathfun_error_errno();
size_t mathfun_error_lineno();
size_t mathfun_error_column();
size_t mathfun_error_errpos();
size_t mathfun_error_errlen();
void   mathfun_error_clear();
void   mathfun_error_log(FILE *stream);

bool          mathfun_valid_name(const char *name);
mathfun_value mathfun_mod(mathfun_value x, mathfun_value y);

#ifdef __cplusplus
}
#endif

#endif
