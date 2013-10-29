#ifndef MATHFUN_INTERN_H__
#define MATHFUN_INTERN_H__

// This is a internal header so there might be no MATHFUN_/mathfun_ prefixes.

#include "mathfun.h"

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// assumtions:
// sizeof(x) == 2 ** n and sizeof(x) == __alignof__(x)
// for x in {double, mathfun_binding_funct}

#define MATHFUN_REGS_MAX UINTPTR_MAX
#define MATHFUN_FUNCT_CODES (1 + ((sizeof(mathfun_binding_funct) - 1) / sizeof(mathfun_code)))
#define MATHFUN_VALUE_CODES (1 + ((sizeof(double) - 1) / sizeof(mathfun_code)))

#ifndef __GNUC__
#	define __attribute__(X)
#endif

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
		double value;
		struct {
			mathfun_binding_funct funct;
			const mathfun_sig *sig;
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
	EX_POW,

	EX_NOT,
	EX_EQ,
	EX_NE,
	EX_LT,
	EX_GT,
	EX_LE,
	EX_GE,
	
	EX_BEQ,
	EX_BNE,

	EX_AND,
	EX_OR,

	EX_IIF
};

struct mathfun_expr {
	enum mathfun_expr_type type;
	union {
		struct {
			mathfun_type type;
			mathfun_reg value;
		} value;

		mathfun_code arg;

		struct {
			mathfun_binding_funct funct;
			const mathfun_sig *sig;
			mathfun_expr **args;
		} funct;

		struct {
			mathfun_expr *expr;
		} unary;

		struct {
			mathfun_expr *left;
			mathfun_expr *right;
		} binary;

		struct {
			mathfun_expr *cond;
			mathfun_expr *then_expr;
			mathfun_expr *else_expr;
		} iif;
	} ex;
};

enum mathfun_bytecode {
	             // arguments      description
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
	POW  = 11,   // reg, reg, reg  power

	NOT  = 12,   // reg, reg       logically negate
	EQ   = 13,   // reg, reg, reg  equals
	NE   = 14,   // reg, reg, reg  not equals
	LT   = 15,   // reg, reg, reg  lower than
	GT   = 16,   // reg, reg, reg  greater than
	LE   = 17,   // reg, reg, reg  lower or equal than
	GE   = 18,   // reg, reg, reg  greater or equal than

	BEQ  = 19,   // reg, reg, reg  boolean equals
	BNE  = 20,   // reg, reg, reg  boolean not equals

	JMP  = 21,   // adr            jumo to adr
	JMPT = 22,   // reg, adr       jump to adr if reg contains true
	JMPF = 23,   // reg, adr       jump to adr if reg contains false

	SETT = 24,   // reg            set reg to true
	SETF = 25,   // reg            set reg to false

	END  = 26    //                pseudo instruction. marks end of code.
};

struct mathfun_error {
	enum mathfun_error_type type;
	int         errnum;
	size_t      lineno;
	size_t      column;
	const char *str;
	const char *errpos;
	size_t      errlen;
	union {
		struct {
			size_t got;
			size_t expected;
		} argc;

		struct {
			mathfun_type got;
			mathfun_type expected;
		} type;
	} err;
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

MATHFUN_LOCAL bool mathfun_context_grow(mathfun_context *ctx, mathfun_error_p *error);

MATHFUN_LOCAL const mathfun_decl *mathfun_context_get(const mathfun_context *ctx, const char *name);

MATHFUN_LOCAL const mathfun_decl *mathfun_context_getn(const mathfun_context *ctx, const char *name, size_t n);

MATHFUN_LOCAL mathfun_expr *mathfun_context_parse(const mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code, mathfun_error_p *error);

MATHFUN_LOCAL bool mathfun_expr_codegen(mathfun_expr *expr, mathfun *mathfun, mathfun_error_p *error);

MATHFUN_LOCAL bool mathfun_codegen_expr(mathfun_codegen *codegen, mathfun_expr *expr, mathfun_code *ret);

MATHFUN_LOCAL bool mathfun_codegen_val(mathfun_codegen *codegen, mathfun_reg value, mathfun_code target);
MATHFUN_LOCAL bool mathfun_codegen_call(mathfun_codegen *codegen, mathfun_binding_funct funct, mathfun_code firstarg, mathfun_code target);

MATHFUN_LOCAL bool mathfun_codegen_ins0(mathfun_codegen *codegen, enum mathfun_bytecode code);
MATHFUN_LOCAL bool mathfun_codegen_ins1(mathfun_codegen *codegen, enum mathfun_bytecode code, mathfun_code arg1);
MATHFUN_LOCAL bool mathfun_codegen_ins2(mathfun_codegen *codegen, enum mathfun_bytecode code, mathfun_code arg1, mathfun_code arg2);
MATHFUN_LOCAL bool mathfun_codegen_ins3(mathfun_codegen *codegen, enum mathfun_bytecode code, mathfun_code arg1, mathfun_code arg2, mathfun_code arg3);

MATHFUN_LOCAL bool mathfun_codegen_binary(mathfun_codegen *codegen, mathfun_expr *expr,
	enum mathfun_bytecode code, mathfun_code *ret);

MATHFUN_LOCAL bool mathfun_codegen_unary(mathfun_codegen *codegen, mathfun_expr *expr,
	enum mathfun_bytecode code, mathfun_code *ret);

MATHFUN_LOCAL mathfun_expr *mathfun_expr_alloc(enum mathfun_expr_type type, mathfun_error_p *error);

MATHFUN_LOCAL void mathfun_expr_free(mathfun_expr *expr);

MATHFUN_LOCAL mathfun_expr *mathfun_expr_optimize(mathfun_expr *expr, mathfun_error_p *error);

MATHFUN_LOCAL mathfun_type mathfun_expr_type(const mathfun_expr *expr);

MATHFUN_LOCAL mathfun_reg mathfun_expr_exec(const mathfun_expr *expr, const double args[]);

MATHFUN_LOCAL const char *mathfun_find_identifier_end(const char *str);

MATHFUN_LOCAL mathfun_error *mathfun_error_alloc(enum mathfun_error_type type);
MATHFUN_LOCAL void mathfun_raise_error(mathfun_error_p *error, enum mathfun_error_type type);
MATHFUN_LOCAL void mathfun_raise_name_error(mathfun_error_p *error, enum mathfun_error_type type, const char *name);
MATHFUN_LOCAL void mathfun_raise_math_error(mathfun_error_p *error, int errnum);
MATHFUN_LOCAL void mathfun_raise_c_error(mathfun_error_p *error);

MATHFUN_LOCAL void mathfun_raise_parser_error(const mathfun_parser *parser,
	enum mathfun_error_type type, const char *errpos);

MATHFUN_LOCAL void mathfun_raise_parser_argc_error(const mathfun_parser *parser,
	const char *errpos, size_t expected, size_t got);

MATHFUN_LOCAL void mathfun_raise_parser_type_error(const mathfun_parser *parser,
	const char *errpos, mathfun_type expected, mathfun_type got);

MATHFUN_LOCAL bool mathfun_validate_argnames(const char *argnames[], size_t argc, mathfun_error_p *error);

MATHFUN_LOCAL const char *mathfun_type_name(mathfun_type type);

#ifdef __cplusplus
}
#endif

#endif
