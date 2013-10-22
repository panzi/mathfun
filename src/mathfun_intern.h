#ifndef MATHFUN_INTERN_H__
#define MATHFUN_INTERN_H__

// this is a internal header, hence no MATHFUN_/mathfun_ prefixes or ifdef __cplusplus

#include "mathfun.h"

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Assumtions:
// sizeof(x) == 2 ** n and sizeof(x) == __alignof__(x)
// for x in {mathfun_value, mathfun_binding_funct, size_t}

#define MATHFUN_REGS_MAX UINTPTR_MAX
#define MATHFUN_FUNCT_CODES (1 + ((sizeof(mathfun_binding_funct) - 1) / sizeof(mathfun_code)))
#define MATHFUN_VALUE_CODES (1 + ((sizeof(mathfun_value) - 1) / sizeof(mathfun_code)))

#ifndef __GNUC__
#	define __attribute__(X)
#endif

#define MATHFUN_MOD(A,B) \
	mathfun_value __mathfun_mod_i = floor((mathfun_value)(A)/(mathfun_value)(B)); \
	mathfun_mod_result = (mathfun_value)(A) - __mathfun_mod_i * (mathfun_value)(B); \
	if (((mathfun_value)(A) < 0.0) != ((mathfun_value)(B) < 0.0)) { \
		mathfun_mod_result = mathfun_mod_result - (mathfun_value)(B); \
	}

#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)
#	if defined(_WIN64)
#		define PRIzu PRIu64
#		define PRIzx PRIx64
#	elif defined(_WIN32)
#		define PRIzu PRIu32
#		define PRIzx PRIx32
#	elif defined(_WIN16)
#		define PRIzu PRIu16
#		define PRIzx PRIx16
#	endif
#else
#	define PRIzu "zu"
#	define PRIzx "zx"
#endif

typedef struct mathfun_expr mathfun_expr;
typedef struct mathfun_error mathfun_error;
typedef struct mathfun_parser mathfun_parser;
typedef struct mathfun_codegen mathfun_codegen;

enum mathfun_decl_type {
	DECL_CONST,
	DECL_FUNCT
};

struct mathfun_decl {
	enum mathfun_decl_type type;
	const char *name;
	union {
		mathfun_value value;
		struct {
			mathfun_binding_funct funct;
			size_t argc;
		} funct;
	} decl;
};

enum mathfun_expr_type {
	EX_CONST,
	EX_ARG,
	EX_CALL,
	EX_NEG,
	EX_ADD,
	EX_SUB,
	EX_MUL,
	EX_DIV,
	EX_MOD,
	EX_POW
};

struct mathfun_expr {
	enum mathfun_expr_type type;
	union {
		mathfun_value value;

		mathfun_code arg;

		struct {
			mathfun_binding_funct funct;
			mathfun_expr **args;
			size_t argc;
		} funct;

		struct {
			mathfun_expr *expr;
		} unary;

		struct {
			mathfun_expr *left;
			mathfun_expr *right;
		} binary;
	} ex;
};

enum mathfun_bytecode {
	             // args           description
	NOP  =  0,   //                do nothing. used to align VAL and CALL
	RET  =  1,   // reg            return
	MOV  =  2,   // reg, reg       copy value
	VAL  =  3,   // val, reg       load an immediate value
	CALL =  4,   // ptr, reg, reg  call a function. parameters:
	             //                 * C function pointer
	             //                 * register of first argument
	             //                 * register for the return value

	NEG  =  5,   // reg, reg       negate
	ADD  =  6,   // reg, reg, reg  add
	SUB  =  7,   // reg, reg, reg  substract
	MUL  =  8,   // reg, reg, reg  multiply
	DIV  =  9,   // reg, reg, reg  divide
	MOD  = 10,   // reg, reg, reg  modulo division
	POW  = 11    // reg, reg, reg  power
};

struct mathfun_error {
	enum mathfun_error_type type;
	int         errnum;
	size_t      lineno;
	size_t      column;
	const char *str;
	const char *errpos;
	size_t      errlen;
	size_t      argc;
	size_t      expected_argc;
};

struct mathfun_parser {
	const struct mathfun_context *ctx;
	const char **argnames;
	size_t       argc;
	const char  *code;
	const char  *ptr;
	mathfun_error_p *error;
};

struct mathfun_codegen {
	size_t argc;
	size_t maxstack;
	size_t currstack;
	size_t code_size;
	size_t code_used;
	mathfun_code *code;
	mathfun_error_p *error;
};

bool mathfun_context_grow(mathfun_context *ctx, mathfun_error_p *error);
const mathfun_decl *mathfun_context_get(const mathfun_context *ctx, const char *name);
const mathfun_decl *mathfun_context_getn(const mathfun_context *ctx, const char *name, size_t n);
mathfun_expr *mathfun_context_parse(const mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code, mathfun_error_p *error);
bool mathfun_expr_codegen(mathfun_expr *expr, mathfun *mathfun, mathfun_error_p *error);

bool mathfun_codegen_expr(mathfun_codegen *codegen, mathfun_expr *expr, mathfun_code *ret);
bool mathfun_codegen_binary(mathfun_codegen *codegen, mathfun_expr *expr,
	enum mathfun_bytecode code, mathfun_code *ret);
mathfun_expr *mathfun_expr_alloc(enum mathfun_expr_type type, mathfun_error_p *error);
void                 mathfun_expr_free(mathfun_expr *expr);
mathfun_expr *mathfun_expr_optimize(mathfun_expr *expr, mathfun_error_p *error);
mathfun_value        mathfun_expr_exec(const mathfun_expr *expr, const mathfun_value args[],
	mathfun_error_p *error);

mathfun_value mathfun_exec(const mathfun *mathfun, mathfun_value regs[])
	__attribute__((__noinline__,__noclone__));

const char *mathfun_find_identifier_end(const char *str);

mathfun_error *mathfun_error_alloc(enum mathfun_error_type type);
void mathfun_raise_error(mathfun_error_p *error, enum mathfun_error_type type);
void mathfun_raise_name_error(mathfun_error_p *error, enum mathfun_error_type type, const char *name);
void mathfun_raise_math_error(mathfun_error_p *error, int errnum);
void mathfun_raise_c_error(mathfun_error_p *error);

void mathfun_raise_parser_error(const mathfun_parser *parser,
	enum mathfun_error_type type, const char *errpos);

void mathfun_raise_parser_argc_error(const mathfun_parser *parser,
	const char *errpos, size_t expected, size_t got);

bool mathfun_validate_argnames(const char *argnames[], size_t argc, mathfun_error_p *error);

#ifdef __cplusplus
}
#endif

#endif
