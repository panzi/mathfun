#include <errno.h>

#include "mathfun_intern.h"

#define OP_ADD(A,B) ((A) + (B))
#define OP_SUB(A,B) ((A) - (B))
#define OP_MUL(A,B) ((A) * (B))
#define OP_DIV(A,B) ((A) / (B))
#define OP_MOD(A,B) ((A) < 0 ? (B) + (A) % (B) : (A) % (B))

typedef mathfun_value (*mathfun_binary_op)(mathfun_value a, mathfun_value b);

math_value op_add(math_value a, math_value b) { return a + b; }
math_value op_sub(math_value a, math_value b) { return a - b; }
math_value op_mul(math_value a, math_value b) { return a * b; }
math_value op_div(math_value a, math_value b) { return a / b; }
math_value op_mod(math_value a, math_value b) { return a < 0 ? b + a % b : a % b; }

static struct mathfun_expr *mathfun_expr_optimize_binary(struct mathfun_expr *expr, mathfun_binary_op op, bool has_neutral, mathfun_value neutral, bool commutative) {
	expr->ex.binary.left  = mathfun_expr_optimize(expr->ex.binary.left);
	expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right);

	if (expr->ex.binary.left->type  == EX_CONST &&
		expr->ex.binary.right->type == EX_CONST) {
		mathfun_value value = op(
			expr->ex.binary.left->ex.value,
			expr->ex.binary.right->ex.value);
		mathfun_expr_free(expr->ex.binary.left);
		mathfun_expr_free(expr->ex.binary.right);
		expr->type = EX_CONST;
		expr->ex.value = value;
	}
	else if (has_neutral) {
		if (expr->ex.binary.right->type == EX_CONST && expr->ex.binary.right->ex.value == neutral) {
			struct mathfun_expr *left = expr->ex.binary.left;
			expr->ex.binary.left = NULL;
			mathfun_expr_free(expr);
			expr = left;
		}
		else if (commutative && expr->ex.binary.left->type == EX_CONST && expr->ex.binary.left->ex.value == neutral) {
			struct mathfun_expr *right = expr->ex.binary.right;
			expr->ex.binary.right = NULL;
			mathfun_expr_free(expr);
			expr = right;
		}
	}
	return expr;
}


struct mathfun_expr *mathfun_expr_optimize(struct mathfun_expr *expr) {
	switch (expr->type) {
		case EX_CONST:
		case EX_ARG:
			return expr;

		case EX_CALL:
			bool allconst = true;
			for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
				struct mathfun_expr *child = expr->ex.funct.args[i] = mathfun_expr_optimize(expr->ex.funct.args[i]);
				if (child->type != EX_CONST) {
					allconst = false;
				}
			}
			if (allconst) {
				mathfun_value *args = calloc(expr->ex.funct.argc, sizeof(mathfun_value));
				if (!args) return NULL;
				mathfun_value value = expr->ex.funct(args);
				free(args);
				expr->type = EX_CONST;
				expr->ex.value = value;
			}
			return expr;

		case EX_NEG:
			expr->ex.unary.expr = mathfun_expr_optimize(expr->ex.unary.expr);
			if (expr->ex.unary.expr->type == EX_NEG) {
				struct mathfun_expr *child = expr->ex.unary.expr->ex.unary.expr;
				expr->ex.unary.expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				return child;
			}
			else if (expr->ex.unary.expr->type == EX_CONST) {
				struct mathfun_expr *child = expr->ex.unary.expr;
				expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				child->ex.value = -child->ex.value;
				return child;
			}
			return expr;

		case EX_ADD: return mathfun_expr_optimize_binary(expr, op_add, true,    0, true);
		case EX_SUB: return mathfun_expr_optimize_binary(expr, op_sub, true,    0, false);
		case EX_MUL: return mathfun_expr_optimize_binary(expr, op_mul, true,    1, true);
		case EX_DIV: return mathfun_expr_optimize_binary(expr, op_div, true,    1, false);
		case EX_MOD: return mathfun_expr_optimize_binary(expr, op_mod, false, NAN, false);
		case EX_POW: return mathfun_expr_optimize_binary(expr, pow,    true,    1, false);
	}

	return expr;
}

struct mathfun_codegen {
	size_t argc;
	size_t constc;
	size_t maxstack;
	size_t currstack;

	size_t regs_size;
	mathfun_value *regs;
	struct mathfun_expr **lastaccess;
	bool *reusable;

	size_t code_size;
	size_t code_used;
	mathfun_code_t *code;

	size_t functs_size;
	size_t functs_used;
	mathfun_binding_funct *functs;
};

void mathfun_codegen_cleanup(struct mathfun_codegen *codegen) {
	free(codegen->regs);
	free(codegen->code);
	free(codegen->functs);
	free(codegen->lastaccess);

	codegen->regs       = NULL;
	codegen->code       = NULL;
	codegen->functs     = NULL;
	codegen->lastaccess = NULL;
}

int mathfun_codegen_append(struct mathfun_codegen *codegen, enum mathfun_bytecode code, ...) {
	va_list ap;
	size_t argc = 0;
	switch (code) {
		case RET: argc = 1; break;
		case MOV:
		case CALL:
		case NEG: argc = 2; break;
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case MOD:
		case POW: argc = 3; break;
	}

	if (codegen->code_used + 1 + argc > codegen->code_size) {
		size_t  size = codegen->code_used + 1 + argc;
		mathfun_code_t code = realloc(codegen->code, size);

		if (!code) return ENOMEM;

		codegen->code = code;
		codegen->code_size = size;
	}

	va_start(ap, code);

	codegen->code[codegen->code_used ++] = code;

	for (size_t i = 0; i < argc; ++ i) {
		codegen->code[codegen->code_used ++] = va_arg(ap, mathfun_code_t);
	}

	va_end(ap);
}

int mathfun_codegen_add_const(struct mathfun_codegen *codegen, struct mathfun_expr *expr) {
	mathfun_value value = expr->ex.value;
	for (size_t reg = codegen->argc, end = codegen->argc + codegen->constc; reg < end; ++ reg) {
		if (codegen->regs[reg] == value) {
			return 0;
		}
	}

	size_t constind = codegen->argc + codegen->constc;
	size_t size = constind + 1;
	if (size > MATHFUN_REGS_MAX) {
		return errno = ERANGE;
	}
	else if (size > codegen->regs_size) {
		mathfun_value *regs = realloc(codegen->regs, size * sizeof(mathfun_value));
		if (!regs) return ENOMEM;

		struct mathfun_expr **lastaccess = realloc(codegen->lastaccess, size * sizeof(struct mathfun_expr*));

		if (!lastaccess) {
			free(regs);
			return ENOMEM;
		}

		codegen->regs = regs;
		codegen->lastaccess = lastaccess;
		codegen->regs_size = size;
	}
	codegen->regs[constind] = value;
	codegen->lastaccess[constind] = expr;
	++ codegen->constc;
	return 0;
}

int mathfun_codegen_add_funct(struct mathfun_codegen *codegen, mathfun_binding_funct funct) {
	for (size_t i = 0; i < codegen->functs_used; ++ i) {
		if (codegen->functs[i] == funct) {
			return 0;
		}
	}

	size_t size = codegen->functs_used + 1;
	if (size > MATHFUN_REGS_MAX) {
		return errno = ERANGE;
	}
	else if (size > codegen->functs_size) {
		mathfun_binding_funct *functs = realloc(codegen->functs, size * sizeof(mathfun_binding_funct));
		if (!functs) return ENOMEM;
		codegen->functs = functs;
		codegen->functs_size = size;
	}
	codegen->functs[constind] = value;
	++ codegen->functs_used;
	return 0;
}

int mathfun_codegen_binary(
	struct mathfun_codegen *codegen,
	struct mathfun_expr *expr,
	enum mathfun_bytecode code,
	mathfun_code_t *ret) {
	int errnum = 0;
	size_t oldstack = codegen->currstack;
	mathfun_code_t leftret  = *ret;
	mathfun_code_t rightret = ++ oldstack;

	errnum = mathfun_codegen(codegen, expr->ex.binary.left, &leftret);
	if (errnum != 0) return errnum;

	if (expr->ex.binary.right->type != EX_CONST && expr->ex.binary.right->type != EX_ARG) {
		codegen->currstack = rightret;
		if (rightret > codegen->maxstack) {
			codegen->maxstack = rightret;
		}
	}

	errnum = mathfun_codegen(codegen, expr->ex.binary.right, &rightret);
	if (errnum != 0) return errnum;

	codegen->currstack = oldstack;
	return mathfun_codegen_append(codegen, code, leftret, rightret, *ret);
}

int mathfun_codegen_alloc_reg(struct mathfun_codegen *codegen, mathfun_code_t *reg) {
	// TODO
}

void mathfun_codegen_free_reg(struct mathfun_codegen *codegen, mathfun_code_t reg) {
	// TODO: do I even need this? I think not
}

// TODO: rewrite register allocation.
// build table that determins when which arg/const register is used the last time and
// use it as general purpose register after that point to save space
//
// and don't pass ret hint. it's not needed because mathfun_codegen_alloc_reg should
// be more intelligent once implemented.
int mathfun_codegen(struct mathfun_codegen *codegen, struct mathfun_expr *expr, mathfun_code_t *ret) {
	int errnum = 0;

	switch (expr->type) {
		case EX_CONST:
			for (size_t reg = codegen->argc, end = codegen->argc + codegen->constc; reg < end; ++ reg) {
				if (codegen->regs[reg] == expr->ex.value) {
					if (codegen->lastaccess[reg] == expr) {
						codegen->reusable[reg] = true;
					}
					*ret = reg;
					return 0;
				}
			}
			return errno = EINVAL; // internal error

		case EX_ARG:
			if (codegen->lastaccess[reg] == expr) {
				codegen->reusable[reg] = true;
			}
			*ret = expr->ex.arg;
			return 0;

		case EX_CALL:
			size_t oldstack = codegen->currstack;
			size_t funct_ret = ++ oldstack;
			if (codegen->maxstack < funct_ret) codegen->maxstack = funct_ret;
			for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
				struct mathfun_expr *arg = expr->ex.funct.args[i];
				mathfun_code_t argret = codegen->currstack;
				errnum = mathfun_codegen(codegen, arg, &argret);
				if (errnum != 0) return errnum;
				if (argret != codegen->currstack &&
					(errnum = mathfun_codegen_append(codegen, MOV, argret, codegen->currstack)) != 0) {
					return errnum;
				}
				if (i + 1 < expr->ex.funct.argc) {
					++ codegen->currstack;
					if (codegen->currstack > codegen->maxstack) {
						codegen->maxstack = codegen->currstack;
					}
				}
			}
			codegen->currstack = oldstack;
			mathfun_code_t findex = 0;
			bool found = false;
			for (mathfun_code_t i = 0; i < codegen->functs_used; ++ i) {
				if (codegen->functs[i] == expr->ex.funct.funct) {
					found = true;
					findex = i;
					break;
				}
			}
			if (!found) return errno = EINVAL; // internal error
			return mathfun_codegen_append(codegen, CALL, findex, *ret);
			
		case EX_NEG:
			mathfun_code_t negret = *ret;
			errnum = mathfun_codegen(codegen, expr->ex.unary.expr, &negret);
			if (errnum != 0) return errnum;
			return mathfun_codegen_append(codegen, NEG, negret, *ret);

		case EX_ADD:
			return mathfun_codegen_binary(codegen, expr, ADD, ret);

		case EX_SUB:
			return mathfun_codegen_binary(codegen, expr, SUB, ret);

		case EX_MUL:
			return mathfun_codegen_binary(codegen, expr, MUL, ret);

		case EX_DIV:
			return mathfun_codegen_binary(codegen, expr, DIV, ret);

		case EX_MOD:
			return mathfun_codegen_binary(codegen, expr, MOD, ret);

		case EX_POW:
			return mathfun_codegen_binary(codegen, expr, POW, ret);
	}

	return EINVAL;
}

// gathers const/inline values and functions
int mathfun_gather_const(struct mathfun_codegen *codegen, struct mathfun_expr *expr) {
	int errnum = 0;
	switch (expr->type) {
		case EX_CONST:
			return mathfun_codegen_add_const(codegen, expr);
			
		case EX_ARG:
			return 0;

		case EX_CALL:
			errnum = mathfun_codegen_add_funct(codegen, expr->ex.funct.funct);
			if (errnum != 0) return errnum;

			for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
				errnum = mathfun_gather_const(codegen, expr->ex.funct.args[i]);
				if (errnum != 0) return errnum;
			}
			return 0;

		case EX_NEG:
			return mathfun_gather_const(codegen, expr->ex.unary.expr);

		case EX_ADD:
		case EX_SUB:
		case EX_MUL:
		case EX_DIV:
		case EX_MOD:
		case EX_POW:
			if ((errnum = mathfun_gather_const(codegen, expr->ex.binary.left)) == 0) {
				return mathfun_gather_const(codegen, expr->ex.binary.right);
			}
			return errnum;
	}
	return 0;
}

int mathfun_context_codegen(const struct mathfun_context *ctx,
	struct mathfun_expr *expr,
	struct mathfun *mathfun) {

	if (mathfun.argc > MATHFUN_REGS_MAX) return errno = ERANGE;

	struct mathfun_codegen codegen;

	memset(&codegen, 0, sizeof(struct mathfun_codegen));

	codegen.argc      = mathfun->argc;
	codegen.regs_size = mathfun->argc > 0 ? mathfun->argc : 1;
	codegen.regs = calloc(codegen.regs_size, sizeof(mathfun_value));

	if (!codegen.regs) {
		mathfun_codegen_cleanup(&codegen);
		return ENOMEM;
	}

	if (mathfun->argc > 0) {
		codegen.regsfree = calloc(mathfun->argc, sizeof(bool));

		if (!codegen.regsfree) {
			mathfun_codegen_cleanup(&codegen);
			return ENOMEM;
		}
	}

	codegen.code_size = 16;
	codegen.code = malloc(code_size);

	if (!codegen.code) {
		mathfun_codegen_cleanup(&codegen);
		return ENOMEM;
	}

	// TODO...
	mathfun_code_t ret = 0; // at the end all registers are free to use
	// ...
	int errnum = mathfun_gather_const(&codegen, expr);
	
	if (errnum != 0) {
		mathfun_codegen_cleanup(&codegen);
		return errnum;
	}

	if (codegen.regs_size > MATHFUN_REGS_MAX || codegen.functs_used > MATHFUN_REGS_MAX) {
		return errno = ERANGE;
	}

	if (codegen.argc + codegen.constc > 0) {
		codegen.reusable = calloc(codegen.argc + codegen.constc, sizeof(bool));

		if (!codegen.reusable) {
			mathfun_codegen_cleanup(&codegen);
			return ENOMEM;
		}
	}

	if ((errnum = mathfun_codegen(&codegen, ctx, expr, &ret)) != 0 ||
		(errnum = mathfun_codegen_append(&codegen, RET, ret)) != 0) {
		mathfun_codegen_cleanup(&codegen);
		return errnum;
	}

	mathfun.regs = codegen.regs;
	mathfun.code = codegen.code;
	mathfun.funct_map = codegen.functs;

	return 0;
}
