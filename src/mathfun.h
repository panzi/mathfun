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

/** Type used for the "registers" of the interpreter.
 */
typedef union mathfun_value {
	double number; ///< numeric value
	bool boolean;  ///< boolean value
} mathfun_value;

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

/** Function type for functions to be registered with a #mathfun_context.
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
	MATHFUN_PARSER_EXPECTED_DOTS,               ///< expected '..' or '...' but got something else or end of input
	MATHFUN_PARSER_TYPE_ERROR,                  ///< expression with wrong type for this position
	MATHFUN_PARSER_UNEXPECTED_END_OF_INPUT,     ///< unexpected end of input
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
@code
mathfun_error_p error = NULL;
double value = mathfun_run("sin(x) + cos(y)", &error, "x", "y", NULL, 1.2, 3.4);
if (error) {
    mathfun_error_log_and_cleanup(&error, stderr);
}
@endcode
 */
typedef const struct mathfun_error *mathfun_error_p;

struct mathfun_context {
	mathfun_decl *decls;
	size_t decl_capacity;
	size_t decl_used;
};

struct mathfun {
	size_t argc;
	size_t framesize;
	void  *code;
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
 * <strong>Functions:</strong>
 *
 *    - bool isnan(double x)
 *    - bool isfinite(double x)
 *    - bool isnormal(double x)
 *    - bool isinf(double x)
 *    - bool isgreater(double x, double y)
 *    - bool isgreaterequal(double x, double y)
 *    - bool isless(double x, double y)
 *    - bool islessequal(double x, double y)
 *    - bool islessgreater(double x, double y)
 *    - bool isunordered(double x, double y)
 *    - bool signbit(double x)
 *    - double acos(double x)
 *    - double acosh(double x)
 *    - double asin(double x)
 *    - double asinh(double x)
 *    - double atan(double x)
 *    - double atan2(double y, double x)
 *    - double atanh(double x)
 *    - double cbrt(double x)
 *    - double ceil(double x)
 *    - double copysign(double x, double y)
 *    - double cos(double x)
 *    - double cosh(double x)
 *    - double erf(double x)
 *    - double erfc(double x)
 *    - double exp(double x)
 *    - double exp2(double x)
 *    - double expm1(double x)
 *    - double abs(double x) unsing fabs(double x)
 *    - double fdim(double x, double y)
 *    - double floor(double x)
 *    - double fma(double x, double y, double z)
 *    - double fmod(double x, double y)
 *    - double max(double x, double y) equivalent to fmax(double x, double y)
 *    - double min(double x, double y) equivalent to fmin(double x, double y)
 *    - double hypot(double x, double y)
 *    - double j0(double x)
 *    - double j1(double x)
 *    - double jn(int n, double x)
 *    - double ldexp(double x, int exp)
 *    - double log(double x)
 *    - double log10(double x)
 *    - double log1p(double x)
 *    - double log2(double x)
 *    - double logb(double x)
 *    - double nearbyint(double x)
 *    - double nextafter(double x, double y)
 *    - double nexttoward(double x, double y)
 *    - double remainder(double x, double y)
 *    - double round(double x)
 *    - double scalbln(double x, long int exp)
 *    - double sin(double x)
 *    - double sinh(double x)
 *    - double sqrt(double x)
 *    - double tan(double x)
 *    - double tanh(double x)
 *    - double gamma(double x) using tgamma(double x)
 *    - double trunc(double x)
 *    - double y0(double x)
 *    - double y1(double x)
 *    - double yn(int n, double x)
 *    - double sign(double x)
 *
 * See `man math.h` for a documentation on these functions, except for sign().
 *
 * <strong>double sign(double x)</strong>
 *
 * Returns sign of x, idecating whether x is positive, negative or zero.
 *
 *    - If x is +NaN, the result is +NaN.
 *    - If x is -NaN, the result is -NaN.
 *    - If x is +0, the result is +0.
 *    - If x is -0, the result is -0.
 *    - If x is negative and not -0 or -NaN, the result is -1.
 *    - If x is positive and not +0 or +NaN, the result is +1.
 *
 * <strong>Constants:</strong>
 *
 *    - e = 2.7182818284590452354
 *    - log2e = log2(e)
 *    - log10e = log10(e)
 *    - ln2 = log(2)
 *    - ln10 = log(10)
 *    - pi = 3.14159265358979323846
 *    - tau = pi * 2
 *    - pi_2 = pi / 2
 *    - pi_4 = pi / 4
 *    - _1_pi = 1 / pi
 *    - _2_pi = 2 / pi
 *    - _2_sqrtpi = 2 / sqrt(pi)
 *    - sqrt2 = sqrt(2)
 *    - sqrt1_2 = 1 / sqrt(2)
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
 * @param fun Target byte code object (will be initialized in any case)
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_ILLEGAL_NAME, #MATHFUN_DUPLICATE_ARGUMENT,
 *        #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR, #MATHFUN_TOO_MANY_ARGUMENTS, #MATHFUN_EXCEEDS_MAX_FRAME_SIZE,
 *        MATHFUN_PARSER_*
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_context_compile(const mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code,
	mathfun *fun, mathfun_error_p *error);

/** Frees allocated resources.
 *
 * @param fun A pointer to a #mathfun object
 */
MATHFUN_EXPORT void mathfun_cleanup(mathfun *fun);

/** Compile function expression to byte code using default function/constant definitions.
 *
 * @param fun Target byte code object (will be initialized in any case)
 * @param argnames Array of argument names of the function expression
 * @param argc Number of arguments
 * @param code The function expression
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_ILLEGAL_NAME, #MATHFUN_DUPLICATE_ARGUMENT,
 *        #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR, #MATHFUN_TOO_MANY_ARGUMENTS, #MATHFUN_EXCEEDS_MAX_FRAME_SIZE,
 *        MATHFUN_PARSER_*
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_compile(mathfun *fun, const char *argnames[], size_t argc, const char *code,
	mathfun_error_p *error);

/** Execute a compiled function expression.
 *
 * @param fun Byte code object to execute
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR,
 *        #MATHFUN_C_ERROR (depending on the functions called by the expression)
 * @return The result of the evaluation.
 */
MATHFUN_EXPORT double mathfun_call(const mathfun *fun, mathfun_error_p *error, ...);

/** Execute a compiled function expression.
 *
 * @param fun Byte code object to execute
 * @param args Array of argument values
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR,
 *        #MATHFUN_C_ERROR (depending on the functions called by the expression)
 * @return The result of the evaluation.
 */
MATHFUN_EXPORT double mathfun_acall(const mathfun *fun, const double args[], mathfun_error_p *error);

/** Execute a compiled function expression.
 *
 * @param fun Byte code object to execute
 * @param ap Variable argument list pointer
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_OUT_OF_MEMORY, #MATHFUN_MATH_ERROR,
 *        #MATHFUN_C_ERROR (depending on the functions called by the expression)
 * @return The result of the evaluation
 */
MATHFUN_EXPORT double mathfun_vcall(const mathfun *fun, va_list ap, mathfun_error_p *error);

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
 * @param fun The compiled function expression
 * @param frame The functions execution frame
 * @return The result of the execution
 */
MATHFUN_EXPORT double mathfun_exec(const mathfun *fun, mathfun_value frame[])
	__attribute__((__noinline__,__noclone__));

/** Dump text representation of byte code.
 * 
 * @param fun The compiled function expression
 * @param stream The output FILE
 * @param ctx A pointer to a #mathfun_context. Can be NULL.
 * @param error A pointer to an error handle. Possible errors: #MATHFUN_IO_ERROR
 * @return true on success, false if an error occured.
 */
MATHFUN_EXPORT bool mathfun_dump(const mathfun *fun, FILE *stream, const mathfun_context *ctx,
	mathfun_error_p *error);

/** Parse and run function expression.
 * 
 * This doesn't optimize or compile the expression but instead directly runs on the abstract syntax tree.
 * Use this for one-time executions.
 *
@code
double mathfun_run(const char *code, mathfun_error_p *error, const char *argname..., NULL, double argvalue...);
@endcode
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
