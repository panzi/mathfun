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

/** Test if given mathfun_error_type is a parser error.
 */
#define MATHFUN_IS_PARSER_ERROR(ERROR) ( \
	(ERROR) >= MATHFUN_PARSER_EXPECTED_CLOSE_PARENTHESIS && \
	(ERROR) <= MATHFUN_PARSER_TRAILING_GARBAGE)

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t mathfun_code;

/** Type used for the "registers" of the interpreter.
 */
typedef union mathfun_reg {
	double number;
	bool boolean;
} mathfun_reg;

/** Type enum.
 *
 * E.g. used in #mathfun_sig.
 */
typedef enum mathfun_type {
	MATHFUN_NUMBER,
	MATHFUN_BOOLEAN
} mathfun_type;

/** Function signature.
 *
 * @see mathfun_context_define_funct()
 */
typedef struct mathfun_sig {
	size_t argc;            ///< number of arguments
	mathfun_type *argtypes; ///< array of argument types
	mathfun_type rettype;   ///< return type
} mathfun_sig;

/** Function type for functions to be regstered with a #mathfun_context.
 */
typedef mathfun_reg (*mathfun_binding_funct)(const mathfun_reg args[]);

/** Error code as returned by mathfun_error_type(mathfun_error_p error)
 */
enum mathfun_error_type {
	MATHFUN_OK = 0,                 ///< no error occured
	MATHFUN_IO_ERROR,               ///< error while writing to file
	MATHFUN_OUT_OF_MEMORY,          ///< memory allocation failed
	MATHFUN_MATH_ERROR,             ///< a math error occured, like x % 0
	MATHFUN_C_ERROR,                ///< a C error occured (errno is set)
	MATHFUN_ILLEGAL_NAME,           ///< a illegal argument/function/constant name was used
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
	MATHFUN_PARSER_EXPECTED_COLON,              ///< expected ':' but got something else or end of input
	MATHFUN_PARSER_TYPE_ERROR,                  ///< expression with wrong type for this position
	MATHFUN_PARSER_TRAILING_GARBAGE             ///< garbage at the end of input
};

struct mathfun_decl;
struct mathfun_error;

typedef struct mathfun_decl mathfun_decl;

/** Object that holds function and constant definitions.
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
 * double value = mathfun_run("sin(x) + cos(y)", &error, "x", "y", NULL, 1.2, 3.4);
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
 * @param ctx A pointer to a #mathfun_context
 * @param define_default If true then a lot of default functions (mainly from math.h) and
 *        constans will be defined in the math_context. See mathfun_context_define_default() for more details.
 * @param error A pointer to an error handle.
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_init(mathfun_context *ctx, bool define_default, mathfun_error_p *error);

/** Frees allocated resources.
 * @param ctx A pointer to a #mathfun_context
 */
MATHFUN_EXPORT void mathfun_context_cleanup(mathfun_context *ctx);

/** Define default set of functions and constants.
 *
 * Functions:
 *
 *    - isnan(x)
 *    - isfinite(x)
 *    - isnormal(x)
 *    - isinf(x)
 *    - acos(x)
 *    - acosh(x)
 *    - asin(x)
 *    - asinh(x)
 *    - atan(x)
 *    - atan2(y, x)
 *    - atanh(x)
 *    - cbrt(x)
 *    - ceil(x)
 *    - copysign(x, y)
 *    - cos(x)
 *    - cosh(x)
 *    - erf(x)
 *    - erfc(x)
 *    - exp(x)
 *    - exp2(x)
 *    - expm1(x)
 *    - abs(x) unsing fabs(x)
 *    - fdim(x, y)
 *    - floor(x)
 *    - fma(x, y, z)
 *    - fmod(x, y)
 *    - max(x)
 *    - min(x)
 *    - hypot(x, y)
 *    - j0(x)
 *    - j1(x)
 *    - jn(n, x)
 *    - ldexp(x, exp)
 *    - log(x)
 *    - log10(x)
 *    - log1p(x)
 *    - log2(x)
 *    - logb(x)
 *    - nearbyint(x)
 *    - nextafter(x, y)
 *    - nexttoward(x, y)
 *    - remainder(x, y)
 *    - round(x)
 *    - scalbln(x, exp)
 *    - sin(x)
 *    - sinh(x)
 *    - sqrt(x)
 *    - tan(x)
 *    - tanh(x)
 *    - gamma(x) using tgamma(x)
 *    - trunc(x)
 *    - y0(x)
 *    - y1(x)
 *    - yn(n, x)
 *
 * See `man math.h` for a documentation on these functions.
 *
 * Constants:
 *
 *    - e ... value of e
 *    - log2e ... value of log2(e)
 *    - log10e ... value of log10(e)
 *    - ln2 ... value of ln(2)
 *    - ln10 ... value of ln(10)
 *    - pi ... value of pi
 *    - tau ... value of pi*2
 *    - pi_2 ... value of pi/2
 *    - pi_4 ... value of pi/4
 *    - _1_pi ... value of 1/pi
 *    - _2_pi ... value of 2/pi
 *    - _2_sqrtpi ... value of 2/sqrt(pi)
 *    - sqrt2 ... value of sqrt(2)
 *    - sqrt1_2 ... value of 1/sqrt(2)
 *
 * @param ctx A pointer to a #mathfun_context
 * @param error A pointer to an error handle.
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_define_default(mathfun_context *ctx, mathfun_error_p *error);

/** Define a constant value.
 * @param ctx A pointer to a #mathfun_context
 * @param name The name of the constant.
 * @param value The value of the constant.
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY and #MATHFUN_NAME_EXISTS
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_define_const(mathfun_context *ctx, const char *name, double value,
	mathfun_error_p *error);

/** Define a constant function.
 * @param ctx A pointer to a #mathfun_context
 * @param name The name of the function.
 * @param funct A function pointer.
 * @param sig The function signature. sig has to have a lifetime of at least as long as ctx.
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY and #MATHFUN_NAME_EXISTS
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_define_funct(mathfun_context *ctx, const char *name, mathfun_binding_funct funct, const mathfun_sig *sig,
	mathfun_error_p *error);

/** Find the name of a given function.
 * @return The name of the function or NULL if not found.
 */
MATHFUN_EXPORT const char *mathfun_context_funct_name(const mathfun_context *ctx, mathfun_binding_funct funct);

/** Removes a function/constant from the context.
 * @param ctx A pointer to a #mathfun_context
 * @param name The name of the function/constant.
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_NO_SUCH_NAME
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_undefine(mathfun_context *ctx, const char *name,
	mathfun_error_p *error);

/** Compile a function expression to byte code.
 *
 * @param ctx A pointer to a #mathfun_context
 * @param argnames Array of argument names of the function expression
 * @param argc Number of arguments
 * @param code The function expression
 * @param mathfun Target byte code object (will be initialized in any case)
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_ILLEGAL_NAME, #MATHFUN_DUPLICATE_ARGUMENT,
 *        #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR, #MATHFUN_TOO_MANY_ARGUMENTS, #MATHFUN_EXCEEDS_MAX_FRAME_SIZE,
 *        MATHFUN_PARSER_*
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_compile(const mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code,
	mathfun *mathfun, mathfun_error_p *error);

/** Frees allocated resources.
 *
 * @param mathfun A pointer to a #mathfun object
 */
MATHFUN_EXPORT void mathfun_cleanup(mathfun *mathfun);

/** Compile function expression to byte code using default function/constant definitions.
 *
 * @param mathfun Target byte code object (will be initialized in any case)
 * @param argnames Array of argument names of the function expression
 * @param argc Number of arguments
 * @param code The function expression
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_ILLEGAL_NAME, #MATHFUN_DUPLICATE_ARGUMENT,
 *        #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR, #MATHFUN_TOO_MANY_ARGUMENTS, #MATHFUN_EXCEEDS_MAX_FRAME_SIZE,
 *        MATHFUN_PARSER_*
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_compile(mathfun *mathfun, const char *argnames[], size_t argc, const char *code,
	mathfun_error_p *error);

/** Execute a compiled function expression.
 *
 * @param mathfun Byte code object to execute
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR,
 *        #MATHFUN_C_ERROR (depending on the functions called by the expression)
 * @return The result of the evaluation.
 */
MATHFUN_EXPORT double mathfun_call(const mathfun *mathfun, mathfun_error_p *error, ...);

/** Execute a compiled function expression.
 *
 * @param mathfun Byte code object to execute
 * @param args Array of argument values
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR,
 *        #MATHFUN_C_ERROR (depending on the functions called by the expression)
 * @return The result of the evaluation.
 */
MATHFUN_EXPORT double mathfun_acall(const mathfun *mathfun, const double args[], mathfun_error_p *error);

/** Execute a compiled function expression.
 *
 * @param mathfun Byte code object to execute
 * @param ap Variable argument list pointer
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR,
 *        #MATHFUN_C_ERROR (depending on the functions called by the expression)
 * @return The result of the evaluation
 */
MATHFUN_EXPORT double mathfun_vcall(const mathfun *mathfun, va_list ap, mathfun_error_p *error);

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
 * @param mathfun The compiled function expression
 * @param frame The functions execution frame
 * @return The result of the execution
 */
MATHFUN_EXPORT double mathfun_exec(const mathfun *mathfun, mathfun_reg frame[])
	__attribute__((__noinline__,__noclone__));

/** Dump text representation of byte code.
 * 
 * @param mathfun The compiled function expression
 * @param stream The output FILE
 * @param ctx A pointer to a #mathfun_context. Can be NULL.
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_IO_ERROR
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_dump(const mathfun *mathfun, FILE *stream, const mathfun_context *ctx,
	mathfun_error_p *error);

/** Parse and run function expression.
 * 
 * This doesn't optimize or compile the expression but instead directly runs on the abstract syntax tree.
 * Use this for one-time executions.
 *
 * @code
 * double mathfun_run(const char *code, mathfun_error_p *error, const char *argname..., NULL, double argvalue...);
 * @endcode
 *
 * @param code The function expression.
 * @param error A pointer to an error handle.
 * @return The result of the execution.
 */
MATHFUN_EXPORT double mathfun_run(const char *code, mathfun_error_p *error, ...);

/** Parse and run function expression.
 * 
 * This doesn't optimize or compile the expression but instead directly runs on the abstract syntax tree.
 * Use this for one-time executions.
 *
 * @param argnames Array of argument names.
 * @param argc Number of arguments.
 * @param code The function expression.
 * @param args The argument values.
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_ILLEGAL_NAME, #MATHFUN_DUPLICATE_ARGUMENT,
 *        #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR, #MATHFUN_TOO_MANY_ARGUMENTS, #MATHFUN_EXCEEDS_MAX_FRAME_SIZE,
 *        MATHFUN_PARSER_*
 * @return The result of the execution.
 */
MATHFUN_EXPORT double mathfun_arun(const char *argnames[], size_t argc, const char *code, const double args[],
	mathfun_error_p *error);

/** Get mathfun_error_type of error.
 *
 * If error is NULL, derive mathfun_error_type from errno.
 *
 * @param error Error handle
 * @return a mathfun_error_type
 */
MATHFUN_EXPORT enum mathfun_error_type mathfun_error_type(mathfun_error_p error);

/** Get C error number of error.
 *
 * If error is NULL, return errno.
 *
 * @param error Error handle
 * @return a C error number
 */
MATHFUN_EXPORT int mathfun_error_errno(mathfun_error_p error);

/** Get line number of a parser error.
 *
 * @param error Error handle
 * @return line number of parser error or 0 if error isn't a parser error.
 */
MATHFUN_EXPORT size_t mathfun_error_lineno(mathfun_error_p error);

/** Get column of a parser error.
 *
 * @param error Error handle
 * @return column of parser error or 0 if error isn't a parser error.
 */
MATHFUN_EXPORT size_t mathfun_error_column(mathfun_error_p error);

/** Get index of parser error in function expression code string.
 *
 * @param error Error handle
 * @return index of parser error or 0 if error isn't a parser error.
 */
MATHFUN_EXPORT size_t mathfun_error_errpos(mathfun_error_p error);

/** Get length of erroneous area in function expression code string.
 *
 * @param error Error handle
 * @return length of error or 0 if error isn't a parser error.
 */
MATHFUN_EXPORT size_t mathfun_error_errlen(mathfun_error_p error);

/** Print error message.
 * 
 * @param error Error handle
 * @param stream Output stream
 */
MATHFUN_EXPORT void mathfun_error_log(mathfun_error_p error, FILE *stream);

/** Print error message and free error object.
 * 
 * @param error Error handle
 * @param stream Output stream
 */
MATHFUN_EXPORT void mathfun_error_log_and_cleanup(mathfun_error_p *error, FILE *stream);

/** Free error object and set error handle to NULL.
 *
 * @param error Error handle
 */
MATHFUN_EXPORT void mathfun_error_cleanup(mathfun_error_p *error);

/** Test if argument is a valid name.
 *
 * Valid names start with a letter or '_' and then have an arbitrary number of more
 * letters, numbers or '_'. To test for letters isalpha() is used, to test for letters or
 * numbers isalnum() is used.
 *
 * @return true if argument is a valid name, false otherwise
 */
MATHFUN_EXPORT bool mathfun_valid_name(const char *name);

/** Modulo division using the Euclidean definition.
 * 
 * The remainder is always positive or 0. This is the way the % operator works
 * in the mathfun expression language.
 *
 * If y is 0, a NaN is returned and errno is set to EDOM (domain error).
 *
 * @param x The dividend
 * @param y The divisor
 * @return The remainder
 */
MATHFUN_EXPORT double mathfun_mod(double x, double y);

#ifdef __cplusplus
}
#endif

#endif
