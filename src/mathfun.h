/**
 * @file   mathfun.h
 * @author Mathias Panzenböck
 * @date   October, 2013
 * @brief  Evaluate simple mathematical functions.
 */

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

/** Test if given enum mathfun_error_type is a parser error.
 */
#define MATHFUN_IS_PARSER_ERROR(ERROR) ( \
	(ERROR) >= MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS && \
	(ERROR) <= MATHFUN_PARSER_TRAILING_GARBAGE)

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t mathfun_code;

/** TODO
 */
typedef double mathfun_value;

/** TODO
 */
typedef mathfun_value (*mathfun_binding_funct)(const mathfun_value args[]);

/** Error code as returned by mathfun_error_type(mathfun_error_p error)
 */
enum mathfun_error_type {
	MATHFUN_OK = 0,                 ///< no error occured
	MATHFUN_IO_ERROR,               ///< error while writing to file
	MATHFUN_OUT_OF_MEMORY,          ///< memory allocation failed
	MATHFUN_MATH_ERROR,             ///< a math error occured, like x % 0
	MATHFUN_C_ERROR,                ///< a C error occured (errno is set)
	MATHFUN_ILLEGAL_NAME,           ///< a illegal argumen/function/constant name was used
	MATHFUN_DUPLICATE_ARGUMENT,     ///< a argument name occured more than once in the list of arguments
	MATHFUN_NAME_EXISTS,            ///< a constant/function with given name already exists
	MATHFUN_NO_SUCH_NAME,           ///< no constant/function with given name exists
	MATHFUN_TOO_MANY_ARGUMENTS,     ///< number of arguments to big
	MATHFUN_EXCEEDS_MAX_FRAME_SIZE, ///< frame size of compiled function exceeds maximum
	MATHFUN_INTERNAL_ERROR,         ///< internal error (e.g. unknown bytecode)
	MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS,  ///< expected ')' but got something else or end of input
	MATHFUN_PARSER_UNDEFINED_REFERENCE,         ///< undefined reference
	MATHFUN_PARSER_NOT_A_FUNCTION,              ///< reference does not define a function (but a constant or argument)
	MATHFUN_PARSER_NOT_A_VARIABLE,              ///< reference does not define a constant or argument (but a function)
	MATHFUN_PARSER_ILLEGAL_NUMBER_OF_ARGUMENTS, ///< function called with an illegal number of arguments
	MATHFUN_PARSER_EXPECTED_NUMBER,             ///< expected a number but got something else or end of input
	MATHFUN_PARSER_EXPECTED_IDENTIFIER,         ///< expected an identifier but got something else or end of input
	MATHFUN_PARSER_TRAILING_GARBAGE             ///< garbage at the end of input
};

struct mathfun_decl;
struct mathfun_error;

typedef struct mathfun_decl mathfun_decl;

/** Context that holds function and constant definitions.
 */
typedef struct mathfun_context mathfun_context;

/** Compiled matfun function expression.
 */
typedef struct mathfun mathfun;

/** Error handle.
 *
 * A pointer to this type (so a pointer to a pointer) is used as argument type of
 * every function that can cause an error. If the function failes the refered pointer
 * will be set to an error object, describing the error. This object might be newly
 * allocated or a constant global object (e.g. in case of #MATHFUN_OUT_OF_MEMORY).
 * In case you don't care about errors you can alyways pass NULL instead.
 *
 * Pelase initialize error handles with NULL.
 * 
 * To free the object use mathfun_error_cleanup() or mathfun_error_log_and_cleanup(),
 * which also sets the refered error handle to NULL again.
 *
 * @code
 * mathfun_error_p error = NULL;
 * mathfun_value value = mathfun_run("sin(x)", &error, "x", NULL, 1.2);
 * if (error) {
 *     mathfun_error_log_and_cleanup(&error, stderr);
 * }
 * @endcode
 */
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

/** Initialize a mathfun_context.
 *
 * @param ctx A pointer to a mathfun_context
 * @param define_default If true then a lot of default functions (mainly from math.h) and
 *        constans will be defined in the math_context. See mathfun_context_define_default() for more details.
 * @param error A pointer to an error handle.
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_init(mathfun_context *ctx, bool define_default, mathfun_error_p *error);

/** Frees allocated resources.
 * @param ctx A pointer to a mathfun_context
 */
MATHFUN_EXPORT void mathfun_context_cleanup(mathfun_context *ctx);

/** TODO
 */
MATHFUN_EXPORT bool mathfun_context_define_default(mathfun_context *ctx, mathfun_error_p *error);

/** Define a constant value.
 * @param ctx A pointer to a mathfun_context
 * @param name The name of the constant.
 * @param value The value of the constant.
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY and #MATHFUN_NAME_EXISTS
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_define_const(mathfun_context *ctx, const char *name, mathfun_value value,
	mathfun_error_p *error);

/** Define a constant function.
 * @param ctx A pointer to a mathfun_context
 * @param name The name of the function.
 * @param funct A function pointer.
 * @param argc The number of the arguments of the function.
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY and #MATHFUN_NAME_EXISTS
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_define_funct(mathfun_context *ctx, const char *name, mathfun_binding_funct funct, size_t argc,
	mathfun_error_p *error);

/** Find the name of a given function.
 * @return The name of the function or NULL if not found.
 */
MATHFUN_EXPORT const char *mathfun_context_funct_name(const mathfun_context *ctx, mathfun_binding_funct funct);

/** Removes a function/constant from the context.
 * @param ctx A pointer to a mathfun_context
 * @param name The name of the function/constant.
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_NO_SUCH_NAME
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_undefine(mathfun_context *ctx, const char *name,
	mathfun_error_p *error);

/** TODO
 */
MATHFUN_EXPORT bool mathfun_context_compile(const mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code,
	mathfun *mathfun, mathfun_error_p *error);

/** Frees allocated resources.
 */
MATHFUN_EXPORT void mathfun_cleanup(mathfun *mathfun);

/** TODO
 */
MATHFUN_EXPORT bool mathfun_compile(mathfun *mathfun, const char *argnames[], size_t argc, const char *code,
	mathfun_error_p *error);

/** TODO
 */
MATHFUN_EXPORT mathfun_value mathfun_call(const mathfun *mathfun, mathfun_error_p *error, ...);

/** TODO
 */
MATHFUN_EXPORT mathfun_value mathfun_acall(const mathfun *mathfun, const mathfun_value args[], mathfun_error_p *error);

/** TODO
 */
MATHFUN_EXPORT mathfun_value mathfun_vcall(const mathfun *mathfun, va_list ap, mathfun_error_p *error);

/** Execute a compiled function expression.
 *
 * This is a low-level function used by mathfun_call(), mathfun_acall() and mathfun_vcall(). Use it if you need
 * high speed and allocate the execution frame yourself (e.g. to re-use it over many calls).
 *
 * The frame has to have mathfun->framesize elements. The first mathfun->argc elements shall be initialized
 * with the arguments of the function expression. There are no guarantees about what elements will be overwritten
 * after the execution.
 *
 * Sets errno when a math error occurs.
 *
 * @param mathfun The compiled function expression.
 * @param frame The functions execution frame.
 * @return The result of the execution.
 */
MATHFUN_EXPORT mathfun_value mathfun_exec(const mathfun *mathfun, mathfun_value frame[])
	__attribute__((__noinline__,__noclone__));

/** TODO
 */
MATHFUN_EXPORT bool mathfun_dump(const mathfun *mathfun, FILE *stream, const mathfun_context *ctx,
	mathfun_error_p *error);

/** Parse and run function expression.
 * 
 * This doesn't optimize or compile the expression but instead directly runs on the abstract syntax tree.
 * Use this for one-time executions.
 *
 * @code
 * mathfun_value mathfun_run(const char *code, mathfun_error_p *error, const char *argname..., NULL, mathfun_value argvalue...);
 * @endcode
 *
 * @param code The function expression.
 * @param error A pointer to an error handle.
 * @return The result of the execution.
 */
MATHFUN_EXPORT mathfun_value mathfun_run(const char *code, mathfun_error_p *error, ...);

/** Parse and run function expression.
 * 
 * This doesn't optimize or compile the expression but instead directly runs on the abstract syntax tree.
 * Use this for one-time executions.
 *
 * @param argnames Array of argument names.
 * @param argc Number of arguments.
 * @param code The function expression.
 * @param args The argument values.
 * @param error A pointer to an error handle.
 * @return The result of the execution.
 */
MATHFUN_EXPORT mathfun_value mathfun_arun(const char *argnames[], size_t argc, const char *code, const mathfun_value args[],
	mathfun_error_p *error);

/** TODO
 */
MATHFUN_EXPORT enum mathfun_error_type mathfun_error_type(mathfun_error_p error);

/** TODO
 */
MATHFUN_EXPORT int mathfun_error_errno(mathfun_error_p error);

/** TODO
 */
MATHFUN_EXPORT size_t mathfun_error_lineno(mathfun_error_p error);

/** TODO
 */
MATHFUN_EXPORT size_t mathfun_error_column(mathfun_error_p error);

/** TODO
 */
MATHFUN_EXPORT size_t mathfun_error_errpos(mathfun_error_p error);

/** TODO
 */
MATHFUN_EXPORT size_t mathfun_error_errlen(mathfun_error_p error);

/** TODO
 */
MATHFUN_EXPORT void mathfun_error_log(mathfun_error_p error, FILE *stream);

/** TODO
 */
MATHFUN_EXPORT void mathfun_error_log_and_cleanup(mathfun_error_p *error, FILE *stream);

/** TODO
 */
MATHFUN_EXPORT void mathfun_error_cleanup(mathfun_error_p *error);

/** TODO
 */
MATHFUN_EXPORT bool mathfun_valid_name(const char *name);

/** Modulo division using the Euclidean definition.
 * 
 * The remainder is always positive or 0. This is the way the % operator works
 * in the mathfun expression language.
 *
 * If y is 0, a NaN is returned and errno is set to EDOM (domain error).
 *
 * @param x The dividend.
 * @param y The divisor.
 * @return The remainder.
 */
MATHFUN_EXPORT mathfun_value mathfun_mod(mathfun_value x, mathfun_value y);

#ifdef __cplusplus
}
#endif

#endif
