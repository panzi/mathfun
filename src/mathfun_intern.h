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

#define _MATHFUN_MOD(A,B) \
	mathfun_value __mathfun_mod_i = floor((mathfun_value)(A)/(mathfun_value)(B)); \
	mathfun_mod_result = (mathfun_value)(A) - __mathfun_mod_i * (mathfun_value)(B); \
	if (((mathfun_value)(A) < 0.0) != ((mathfun_value)(B) < 0.0)) { \
		mathfun_mod_result = mathfun_mod_result - (mathfun_value)(B); \
	}

#define MATHFUN_MOD(A,B) \
	mathfun_value mathfun_mod_result; \
	if ((B) == 0.0) { \
		mathfun_raise_error(MATHFUN_DOMAIN_ERROR); \
		mathfun_mod_result = NAN; \
	} \
	else { \
		_MATHFUN_MOD(A,B) \
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

		size_t arg;

		struct {
			mathfun_binding_funct funct;
			struct mathfun_expr **args;
			size_t argc;
		} funct;

		struct {
			struct mathfun_expr *expr;
		} unary;

		struct {
			struct mathfun_expr *left;
			struct mathfun_expr *right;
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

struct strbuf {
	char  *data;
	size_t size;
	size_t used;
};

struct mathfun_parser {
	const struct mathfun_context *ctx;
	const char **argnames;
	size_t argc;
	const char *code;
	const char *ptr;
	struct strbuf buf;
};

struct mathfun_codegen {
	size_t argc;
	size_t maxstack;
	size_t currstack;
	size_t code_size;
	size_t code_used;
	mathfun_code *code;
};

bool mathfun_context_grow(struct mathfun_context *ctx);
const struct mathfun_decl *mathfun_context_get(const struct mathfun_context *ctx, const char *name);
struct mathfun_expr *mathfun_context_parse(const struct mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code);
bool mathfun_expr_codegen(struct mathfun_expr *expr, struct mathfun *mathfun);

bool mathfun_codegen(struct mathfun_codegen *codegen, struct mathfun_expr *expr, mathfun_code *ret);
bool mathfun_codegen_binary(struct mathfun_codegen *codegen, struct mathfun_expr *expr,
	enum mathfun_bytecode code, mathfun_code *ret);
struct mathfun_expr *mathfun_expr_alloc(enum mathfun_expr_type type);
void                 mathfun_expr_free(struct mathfun_expr *expr);
struct mathfun_expr *mathfun_expr_optimize(struct mathfun_expr *expr);
mathfun_value        mathfun_expr_exec(const struct mathfun_expr *expr, const mathfun_value args[]);

mathfun_value mathfun_exec(const struct mathfun *mathfun, mathfun_value regs[])
	__attribute__((__noinline__,__noclone__));

const char *mathfun_find_identifier_end(const char *str);

void mathfun_raise_error(enum mathfun_error_type type);
void mathfun_raise_name_error(enum mathfun_error_type type, const char *name);
void mathfun_raise_parser_error(const struct mathfun_parser *parser,
	enum mathfun_error_type type, const char *errpos);
void mathfun_raise_parser_argc_error(const struct mathfun_parser *parser,
	const char *errpos, size_t expected, size_t got);

#ifdef __cplusplus
}
#endif

#endif
