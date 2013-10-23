#ifndef MATHFUN_H__
#define MATHFUN_H__

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <math.h>

#include "config.h"
#include "export.h"

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#	define MATHFUN_LOCAL
#else
#	if __GNUC__ >= 4
#		define MATHFUN_LOCAL __attribute__ ((visibility ("hidden")))
#	else
#		define MATHFUN_LOCAL
#	endif
#endif

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
	MATHFUN_OUT_OF_MEMORY,
	MATHFUN_MATH_ERROR,
	MATHFUN_C_ERROR,
	MATHFUN_ILLEGAL_NAME,
	MATHFUN_DUPLICATE_ARGUMENT,
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
struct mathfun_error;

typedef struct mathfun_decl mathfun_decl;
typedef struct mathfun_context mathfun_context;
typedef struct mathfun mathfun;
typedef const struct mathfun_error *mathfun_error_p;

struct mathfun_context {
	mathfun_decl *decls;
	size_t decl_capacity;
	size_t decl_used;
};

struct mathfun {
	size_t        argc;
	size_t        framesize;
	mathfun_code *code;
};

MATHFUN_EXPORT bool mathfun_context_init(mathfun_context *ctx, bool define_default, mathfun_error_p *error);
MATHFUN_EXPORT void mathfun_context_cleanup(mathfun_context *ctx);
MATHFUN_EXPORT bool mathfun_context_define_default(mathfun_context *ctx, mathfun_error_p *error);
MATHFUN_EXPORT bool mathfun_context_define_const(mathfun_context *ctx, const char *name, mathfun_value value,
	mathfun_error_p *error);
MATHFUN_EXPORT bool mathfun_context_define_funct(mathfun_context *ctx, const char *name, mathfun_binding_funct funct, size_t argc,
	mathfun_error_p *error);
MATHFUN_EXPORT const char *mathfun_context_funct_name(const mathfun_context *ctx, mathfun_binding_funct funct);
MATHFUN_EXPORT bool mathfun_context_undefine(mathfun_context *ctx, const char *name,
	mathfun_error_p *error);
MATHFUN_EXPORT bool mathfun_context_compile(const mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code,
	mathfun *mathfun, mathfun_error_p *error);

MATHFUN_EXPORT void mathfun_cleanup(mathfun *mathfun);
MATHFUN_EXPORT bool mathfun_compile(mathfun *mathfun, const char *argnames[], size_t argc, const char *code,
	mathfun_error_p *error);
MATHFUN_EXPORT mathfun_value mathfun_call(const mathfun *mathfun, mathfun_error_p *error, ...);
MATHFUN_EXPORT mathfun_value mathfun_acall(const mathfun *mathfun, const mathfun_value args[], mathfun_error_p *error);
MATHFUN_EXPORT mathfun_value mathfun_vcall(const mathfun *mathfun, va_list ap, mathfun_error_p *error);
MATHFUN_EXPORT mathfun_value mathfun_exec(const mathfun *mathfun, mathfun_value frame[])
	__attribute__((__noinline__,__noclone__));

MATHFUN_EXPORT bool mathfun_dump(const mathfun *mathfun, FILE *stream, const mathfun_context *ctx,
	mathfun_error_p *error);

MATHFUN_EXPORT mathfun_value mathfun_run(const char *code, mathfun_error_p *error, ...);
MATHFUN_EXPORT mathfun_value mathfun_arun(const char *argnames[], size_t argc, const char *code, const mathfun_value args[],
	mathfun_error_p *error);

MATHFUN_EXPORT enum mathfun_error_type mathfun_error_type(mathfun_error_p error);
MATHFUN_EXPORT int    mathfun_error_errno(mathfun_error_p error);
MATHFUN_EXPORT size_t mathfun_error_lineno(mathfun_error_p error);
MATHFUN_EXPORT size_t mathfun_error_column(mathfun_error_p error);
MATHFUN_EXPORT size_t mathfun_error_errpos(mathfun_error_p error);
MATHFUN_EXPORT size_t mathfun_error_errlen(mathfun_error_p error);
MATHFUN_EXPORT void   mathfun_error_log(mathfun_error_p error, FILE *stream);
MATHFUN_EXPORT void   mathfun_error_log_and_cleanup(mathfun_error_p *error, FILE *stream);
MATHFUN_EXPORT void   mathfun_error_cleanup(mathfun_error_p *error);

MATHFUN_EXPORT bool          mathfun_valid_name(const char *name);
MATHFUN_EXPORT mathfun_value mathfun_mod(mathfun_value x, mathfun_value y);

#ifdef __cplusplus
}
#endif

#endif
