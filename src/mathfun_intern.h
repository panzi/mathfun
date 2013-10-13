#ifndef MATHFUN_INTERN_H__
#define MATHFUN_INTERN_H__

// this is a internal header, hence no MATHFUN_/mathfun_ prefixes or ifdef __cplusplus

#include <stdint.h>

#include "mathfun.h"

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

struct mathfun_context {
	struct mathfun_decl *decls;
	size_t decl_capacity;
	size_t decl_used;
};

struct mathfun {
	size_t         argc;
	uint8_t       *code;
	mathfun_value *regs;
	mathfun_binding_funct funct_map[256];
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
			struct expr *expr;
		} unary;

		struct {
			struct expr *left;
			struct expr *right;
		} binary;
	} ex;
};

enum mathfun_bytecode {
	       // args           description
	RET,   // reg            return
	MOV,   // reg, reg       copy value
	CALL,  // index, reg     call a function. index points into funct_map.

	NEG,   // reg, reg       negate
	ADD,   // reg, reg, reg  add
	SUB,   // reg, reg, reg  substract
	MUL,   // reg, reg, reg  multiply
	DIV,   // reg, reg, reg  divide
	MOD,   // reg, reg, reg  modulo
	POW    // reg, reg, reg  power

/* maybe later
	EQ,    // reg, reg, reg  equal
	NEQ,   // reg, reg, reg  not equal
	BEQ,   // reg, reg, reg  boolean values equal
	BNEQ,  // reg, reg, reg  boolean values not equal
	LT,    // reg, reg, reg  lower than
	GT,    // reg, reg, reg  greater than
	LTE,   // reg, reg, reg  lower than or equal
	GTE,   // reg, reg, reg  greater than or equal

	ISNAN, // reg, reg
	ISINF, // reg, reg
	ISFIN, // reg, reg

	BR,    // codeptr        unconditional branch
	CBR,   // codeptr, reg   conditional branch (branch if true)
	NBR    // codeptr, reg   conditional branch (branch if false)
*/
};

int                  mathfun_context_grow(struct mathfun_context *ctx);
struct mathfun_decl *mathfun_context_get(struct mathfun_context *ctx, const char *name);
struct mathfun_expr *mathfun_context_parse(const struct mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code);
int mathfun_context_codegen(const struct mathfun_context *ctx,
	struct mathfun_expr *expr,
	struct mathfun *mathfun);

struct mathfun_expr *mathfun_expr_alloc(enum mathfun_expr_type type);
void                 mathfun_expr_free(struct mathfun_expr *expr);
struct mathfun_expr *mathfun_expr_optimize(struct mathfun_expr *expr);

mathfun_value mathfun_exec(struct mathfun *mathfun);

#endif
