#include <errno.h>
#include <string.h>
#include <assert.h>

#include "mathfun_intern.h"

typedef bool (*mathfun_binary_op)(mathfun_value a, mathfun_value b, mathfun_value *res, mathfun_error_p *error);
typedef bool (*mathfun_cmp)(mathfun_value a, mathfun_value b);

static bool mathfun_opt_add(mathfun_value a, mathfun_value b, mathfun_value *res, mathfun_error_p *error) {
	(void)error;
	*res = a + b;
	return true;
}

static bool mathfun_opt_sub(mathfun_value a, mathfun_value b, mathfun_value *res, mathfun_error_p *error) {
	(void)error;
	*res = a - b;
	return true;
}

static bool mathfun_opt_mul(mathfun_value a, mathfun_value b, mathfun_value *res, mathfun_error_p *error) {
	(void)error;
	*res = a * b;
	return true;
}

static bool mathfun_opt_div(mathfun_value a, mathfun_value b, mathfun_value *res, mathfun_error_p *error) {
	(void)error;
	*res = a / b;
	return true;
}

static bool mathfun_opt_mod(mathfun_value a, mathfun_value b, mathfun_value *res, mathfun_error_p *error) {
	if (b == 0.0) {
		mathfun_raise_math_error(error, EDOM);
		return false;
	}
	else {
		mathfun_value mathfun_mod_result;
		MATHFUN_MOD(a,b);
		*res = mathfun_mod_result;
		return true;
	}
}

static bool mathfun_opt_pow(mathfun_value a, mathfun_value b, mathfun_value *res, mathfun_error_p *error) {
	errno = 0;
	*res = pow(a, b);
	if (errno != 0) {
		mathfun_raise_c_error(error);
		return false;
	}
	return true;
}

static bool mathfun_opt_eq(mathfun_value a, mathfun_value b) { return a == b; }
static bool mathfun_opt_ne(mathfun_value a, mathfun_value b) { return a != b; }
static bool mathfun_opt_gt(mathfun_value a, mathfun_value b) { return a >  b; }
static bool mathfun_opt_lt(mathfun_value a, mathfun_value b) { return a <  b; }
static bool mathfun_opt_ge(mathfun_value a, mathfun_value b) { return a >= b; }
static bool mathfun_opt_le(mathfun_value a, mathfun_value b) { return a <= b; }

static mathfun_expr *mathfun_expr_optimize_binary(mathfun_expr *expr,
	mathfun_binary_op op, bool has_neutral, mathfun_value neutral, bool commutative,
	mathfun_error_p *error) {

	expr->ex.binary.left = mathfun_expr_optimize(expr->ex.binary.left, error);
	if (!expr->ex.binary.left) {
		mathfun_expr_free(expr);
		return NULL;
	}

	expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right, error);
	if (!expr->ex.binary.right) {
		mathfun_expr_free(expr);
		return NULL;
	}

	if (expr->ex.binary.left->type  == EX_CONST &&
		expr->ex.binary.right->type == EX_CONST) {

		mathfun_value value = 0;
		if (!op(expr->ex.binary.left->ex.value.value.number, expr->ex.binary.right->ex.value.value.number, &value, error)) {
			mathfun_expr_free(expr);
			return NULL;
		}

		mathfun_expr_free(expr->ex.binary.left);
		mathfun_expr_free(expr->ex.binary.right);
		expr->type = EX_CONST;
		expr->ex.value.type = MATHFUN_NUMBER;
		expr->ex.value.value.number = value;
	}
	else if (has_neutral) {
		if (expr->ex.binary.right->type == EX_CONST && expr->ex.binary.right->ex.value.value.number == neutral) {
			mathfun_expr *left = expr->ex.binary.left;
			expr->ex.binary.left = NULL;
			mathfun_expr_free(expr);
			expr = left;
		}
		else if (commutative && expr->ex.binary.left->type == EX_CONST &&
			expr->ex.binary.left->ex.value.value.number == neutral) {
			mathfun_expr *right = expr->ex.binary.right;
			expr->ex.binary.right = NULL;
			mathfun_expr_free(expr);
			expr = right;
		}
	}
	return expr;
}

static mathfun_expr *mathfun_expr_optimize_comparison(mathfun_expr *expr,
	mathfun_cmp cmp, mathfun_error_p *error) {

	expr->ex.binary.left = mathfun_expr_optimize(expr->ex.binary.left, error);
	if (!expr->ex.binary.left) {
		mathfun_expr_free(expr);
		return NULL;
	}

	expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right, error);
	if (!expr->ex.binary.right) {
		mathfun_expr_free(expr);
		return NULL;
	}
	
	if (expr->ex.binary.left->type  == EX_CONST &&
		expr->ex.binary.right->type == EX_CONST) {

		bool value = cmp(expr->ex.binary.left->ex.value.value.number, expr->ex.binary.right->ex.value.value.number);

		mathfun_expr_free(expr->ex.binary.left);
		mathfun_expr_free(expr->ex.binary.right);
		expr->type = EX_CONST;
		expr->ex.value.type = MATHFUN_BOOLEAN;
		expr->ex.value.value.boolean = value;
	}

	return expr;
}

mathfun_expr *mathfun_expr_optimize(mathfun_expr *expr, mathfun_error_p *error) {
	switch (expr->type) {
		case EX_CONST:
		case EX_ARG:
			return expr;

		case EX_CALL:
		{
			bool allconst = true;
			const size_t argc = expr->ex.funct.sig->argc;
			for (size_t i = 0; i < argc; ++ i) {
				mathfun_expr *child = expr->ex.funct.args[i] =
					mathfun_expr_optimize(expr->ex.funct.args[i], error);
				if (!child) {
					mathfun_expr_free(expr);
					return NULL;
				}
				else if (child->type != EX_CONST) {
					allconst = false;
				}
			}
			if (allconst) {
				mathfun_reg *args = calloc(argc, sizeof(mathfun_reg));
				if (!args) return NULL;
				for (size_t i = 0; i < argc; ++ i) {
					args[i] = expr->ex.funct.args[i]->ex.value.value;
				}

				// math errors are communicated via errno
				// XXX: buggy. see NOTES in man math_error
				errno = 0;
				mathfun_reg value = expr->ex.funct.funct(args);

				free(args);
				expr->type = EX_CONST;
				expr->ex.value.type = expr->ex.funct.sig->rettype;
				expr->ex.value.value = value;

				if (errno != 0) {
					mathfun_raise_c_error(error);
					mathfun_expr_free(expr);
					return NULL;
				}
			}
			return expr;
		}

		case EX_NEG:
			expr->ex.unary.expr = mathfun_expr_optimize(expr->ex.unary.expr, error);
			if (!expr->ex.unary.expr) {
				mathfun_expr_free(expr);
				return NULL;
			}
			else if (expr->ex.unary.expr->type == EX_NEG) {
				mathfun_expr *child = expr->ex.unary.expr->ex.unary.expr;
				expr->ex.unary.expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				return child;
			}
			else if (expr->ex.unary.expr->type == EX_CONST) {
				mathfun_expr *child = expr->ex.unary.expr;
				expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				child->ex.value.value.number = -child->ex.value.value.number;
				return child;
			}
			return expr;

		case EX_ADD: return mathfun_expr_optimize_binary(expr, mathfun_opt_add, true,    0, true,  error);
		case EX_SUB: return mathfun_expr_optimize_binary(expr, mathfun_opt_sub, true,    0, false, error);
		case EX_MUL: return mathfun_expr_optimize_binary(expr, mathfun_opt_mul, true,    1, true,  error);
		case EX_DIV: return mathfun_expr_optimize_binary(expr, mathfun_opt_div, true,    1, false, error);
		case EX_MOD: return mathfun_expr_optimize_binary(expr, mathfun_opt_mod, false, NAN, false, error);
		case EX_POW: return mathfun_expr_optimize_binary(expr, mathfun_opt_pow, true,    1, false, error);

		case EX_NOT:
			expr->ex.unary.expr = mathfun_expr_optimize(expr->ex.unary.expr, error);
			if (!expr->ex.unary.expr) {
				mathfun_expr_free(expr);
				return NULL;
			}
			else if (expr->ex.unary.expr->type == EX_NOT) {
				mathfun_expr *child = expr->ex.unary.expr->ex.unary.expr;
				expr->ex.unary.expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				return child;
			}
			else if (expr->ex.unary.expr->type == EX_CONST) {
				mathfun_expr *child = expr->ex.unary.expr;
				expr->ex.unary.expr = NULL;
				mathfun_expr_free(expr);
				child->ex.value.value.boolean = !child->ex.value.value.boolean;
				return child;
			}
			return expr;

		case EX_EQ: return mathfun_expr_optimize_comparison(expr, mathfun_opt_eq, error);
		case EX_NE: return mathfun_expr_optimize_comparison(expr, mathfun_opt_ne, error);
		case EX_LT: return mathfun_expr_optimize_comparison(expr, mathfun_opt_lt, error);
		case EX_GT: return mathfun_expr_optimize_comparison(expr, mathfun_opt_gt, error);
		case EX_LE: return mathfun_expr_optimize_comparison(expr, mathfun_opt_le, error);
		case EX_GE: return mathfun_expr_optimize_comparison(expr, mathfun_opt_ge, error);

		case EX_AND:
		{
			expr->ex.binary.left = mathfun_expr_optimize(expr->ex.binary.left, error);
			if (!expr->ex.binary.left) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right, error);
			if (!expr->ex.binary.right) {
				mathfun_expr_free(expr);
				return NULL;
			}

			mathfun_expr *const_expr;
			mathfun_expr *other_expr;
			if (expr->ex.binary.left->type  == EX_CONST &&
				expr->ex.binary.right->type == EX_CONST) {
				bool value = expr->ex.binary.left->ex.value.value.boolean && expr->ex.binary.right->ex.value.value.boolean;

				mathfun_expr_free(expr->ex.binary.left);
				mathfun_expr_free(expr->ex.binary.right);
				expr->type = EX_CONST;
				expr->ex.value.type = MATHFUN_BOOLEAN;
				expr->ex.value.value.boolean = value;
			}
			else if (expr->ex.binary.left->type == EX_CONST) {
				const_expr = expr->ex.binary.left;
				other_expr = expr->ex.binary.right;
			}
			else if (expr->ex.binary.right->type == EX_CONST) {
				other_expr = expr->ex.binary.left;
				const_expr = expr->ex.binary.right;
			}
			else {
				return expr;
			}

			if (const_expr->ex.value.value.boolean) {
				expr->ex.binary.left  = NULL;
				expr->ex.binary.right = NULL;
				mathfun_expr_free(const_expr);
				mathfun_expr_free(expr);
				return other_expr;
			}
			else {
				mathfun_expr_free(expr->ex.binary.left);
				mathfun_expr_free(expr->ex.binary.right);
				expr->type = EX_CONST;
				expr->ex.value.type = MATHFUN_BOOLEAN;
				expr->ex.value.value.boolean = false;
				return expr;
			}
		}
		case EX_OR:
		{
			expr->ex.binary.left = mathfun_expr_optimize(expr->ex.binary.left, error);
			if (!expr->ex.binary.left) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.binary.right = mathfun_expr_optimize(expr->ex.binary.right, error);
			if (!expr->ex.binary.right) {
				mathfun_expr_free(expr);
				return NULL;
			}

			mathfun_expr *const_expr;
			mathfun_expr *other_expr;
			if (expr->ex.binary.left->type  == EX_CONST &&
				expr->ex.binary.right->type == EX_CONST) {
				bool value = expr->ex.binary.left->ex.value.value.boolean || expr->ex.binary.right->ex.value.value.boolean;

				mathfun_expr_free(expr->ex.binary.left);
				mathfun_expr_free(expr->ex.binary.right);
				expr->type = EX_CONST;
				expr->ex.value.type = MATHFUN_BOOLEAN;
				expr->ex.value.value.boolean = value;
			}
			else if (expr->ex.binary.left->type == EX_CONST) {
				const_expr = expr->ex.binary.left;
				other_expr = expr->ex.binary.right;
			}
			else if (expr->ex.binary.right->type == EX_CONST) {
				other_expr = expr->ex.binary.left;
				const_expr = expr->ex.binary.right;
			}
			else {
				return expr;
			}

			if (const_expr->ex.value.value.boolean) {
				mathfun_expr_free(expr->ex.binary.left);
				mathfun_expr_free(expr->ex.binary.right);
				expr->type = EX_CONST;
				expr->ex.value.type = MATHFUN_BOOLEAN;
				expr->ex.value.value.boolean = true;
				return expr;
			}
			else {
				expr->ex.binary.left  = NULL;
				expr->ex.binary.right = NULL;
				mathfun_expr_free(const_expr);
				mathfun_expr_free(expr);
				return other_expr;
			}
		}
		case EX_IIF:
		{
			expr->ex.iif.cond = mathfun_expr_optimize(expr->ex.iif.cond, error);
			if (!expr->ex.iif.cond) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.iif.then_expr = mathfun_expr_optimize(expr->ex.iif.then_expr, error);
			if (!expr->ex.iif.then_expr) {
				mathfun_expr_free(expr);
				return NULL;
			}

			expr->ex.iif.else_expr = mathfun_expr_optimize(expr->ex.iif.else_expr, error);
			if (!expr->ex.iif.else_expr) {
				mathfun_expr_free(expr);
				return NULL;
			}

			if (expr->ex.iif.cond->type == EX_CONST) {
				mathfun_expr *child;
				if (expr->ex.iif.cond->ex.value.value.boolean) {
					child = expr->ex.iif.then_expr;
					expr->ex.iif.then_expr = NULL;
				}
				else {
					child = expr->ex.iif.else_expr;
					expr->ex.iif.else_expr = NULL;
				}
				mathfun_expr_free(expr);
				return child;
			}

			return expr;
		}
	}

	return expr;
}

void mathfun_codegen_cleanup(mathfun_codegen *codegen) {
	free(codegen->code);
	codegen->code = NULL;
}

bool mathfun_codegen_ensure(mathfun_codegen *codegen, size_t n) {
	const size_t size = codegen->code_used + n;
	if (size > codegen->code_size) {
		mathfun_code *code = realloc(codegen->code, size * sizeof(mathfun_code));

		if (!code) {
			mathfun_raise_error(codegen->error, MATHFUN_OUT_OF_MEMORY);
			return false;
		}

		codegen->code = code;
		codegen->code_size = size;
	}
	return true;
}

bool mathfun_codegen_align(mathfun_codegen *codegen, size_t offset, size_t align) {
	for (;;) {
		uintptr_t ptr = (uintptr_t)(codegen->code + codegen->code_used + offset);
		uintptr_t aligned = ptr & ~(align - 1);
		if (aligned == ptr) break;
		if (!mathfun_codegen_ensure(codegen, 1)) return false;
		codegen->code[codegen->code_used ++] = NOP;
	}
	return true;
}

bool mathfun_codegen_val(mathfun_codegen *codegen, mathfun_reg value, mathfun_code target) {
	if (!mathfun_codegen_align(codegen, 1, sizeof(mathfun_value))) return false;
	if (!mathfun_codegen_ensure(codegen, MATHFUN_VALUE_CODES + 2)) return false;

	codegen->code[codegen->code_used ++] = VAL;
	*(mathfun_reg*)(codegen->code + codegen->code_used) = value;
	codegen->code_used += MATHFUN_VALUE_CODES;
	codegen->code[codegen->code_used ++] = target;

	return true;
}

bool mathfun_codegen_call(mathfun_codegen *codegen, mathfun_binding_funct funct, mathfun_code firstarg, mathfun_code target) {
	if (!mathfun_codegen_align(codegen, 1, sizeof(mathfun_binding_funct))) return false;
	if (!mathfun_codegen_ensure(codegen, MATHFUN_FUNCT_CODES + 3)) return false;

	codegen->code[codegen->code_used ++] = CALL;
	*(mathfun_binding_funct*)(codegen->code + codegen->code_used) = funct;
	codegen->code_used += MATHFUN_FUNCT_CODES;
	codegen->code[codegen->code_used ++] = firstarg;
	codegen->code[codegen->code_used ++] = target;

	return true;
}

bool mathfun_codegen_ins0(mathfun_codegen *codegen, enum mathfun_bytecode code) {
	if (!mathfun_codegen_ensure(codegen, 1)) return false;
	codegen->code[codegen->code_used ++] = code;
	return true;
}

bool mathfun_codegen_ins1(mathfun_codegen *codegen, enum mathfun_bytecode code, mathfun_code arg1) {
	if (!mathfun_codegen_ensure(codegen, 2)) return false;
	codegen->code[codegen->code_used ++] = code;
	codegen->code[codegen->code_used ++] = arg1;
	return true;
}

bool mathfun_codegen_ins2(mathfun_codegen *codegen, enum mathfun_bytecode code, mathfun_code arg1, mathfun_code arg2) {
	if (!mathfun_codegen_ensure(codegen, 3)) return false;
	codegen->code[codegen->code_used ++] = code;
	codegen->code[codegen->code_used ++] = arg1;
	codegen->code[codegen->code_used ++] = arg2;
	return true;
}

bool mathfun_codegen_ins3(mathfun_codegen *codegen, enum mathfun_bytecode code, mathfun_code arg1, mathfun_code arg2, mathfun_code arg3) {
	if (!mathfun_codegen_ensure(codegen, 4)) return false;
	codegen->code[codegen->code_used ++] = code;
	codegen->code[codegen->code_used ++] = arg1;
	codegen->code[codegen->code_used ++] = arg2;
	codegen->code[codegen->code_used ++] = arg3;
	return true;
}

bool mathfun_codegen_binary(
	mathfun_codegen *codegen,
	mathfun_expr *expr,
	enum mathfun_bytecode code,
	mathfun_code *ret) {
	mathfun_code leftret  = codegen->currstack;
	mathfun_code rightret;

	if (!mathfun_codegen_expr(codegen, expr->ex.binary.left, &leftret)) return false;

	if (leftret < codegen->currstack) {
		// returned an argument, can use unchanged currstack for right expression
		rightret = codegen->currstack;
		if (!mathfun_codegen_expr(codegen, expr->ex.binary.right, &rightret)) return false;
	}
	else {
		rightret = ++ codegen->currstack;
		if (!mathfun_codegen_expr(codegen, expr->ex.binary.right, &rightret)) return false;

		// doing this *after* the codegen for the right expression
		// optimizes the case where no extra register is needed (e.g. it
		// just accesses an argument register)
		if (codegen->maxstack < rightret) {
			codegen->maxstack = rightret;
		}

		-- codegen->currstack;
	}

	return mathfun_codegen_ins3(codegen, code, leftret, rightret, *ret);
}

bool mathfun_codegen_unary(mathfun_codegen *codegen, mathfun_expr *expr,
	enum mathfun_bytecode code, mathfun_code *ret) {
	mathfun_code unret = *ret;
	if (!mathfun_codegen_expr(codegen, expr->ex.unary.expr, &unret)) return false;
	return mathfun_codegen_ins2(codegen, code, unret, *ret);
}

bool mathfun_codegen_expr(mathfun_codegen *codegen, mathfun_expr *expr, mathfun_code *ret) {
	switch (expr->type) {
		case EX_CONST:
			if (expr->ex.value.type == MATHFUN_BOOLEAN) {
				return mathfun_codegen_ins1(codegen, expr->ex.value.value.boolean ? SETT : SETF, *ret);
			}
			else {
				return mathfun_codegen_val(codegen, expr->ex.value.value, *ret);
			}

		case EX_ARG:
			*ret = expr->ex.arg;
			return true;

		case EX_CALL:
		{
			mathfun_code oldstack = codegen->currstack;
			mathfun_code firstarg = oldstack;
			size_t i = 0;
			const size_t argc = expr->ex.funct.sig->argc;

			// check if args happen to be on the "stack"
			// This removes mov instructions when all arguments are already in the
			// correct order in registers or the leading arguments are in registers
			// directly before the current "stack pointer".
			if (argc > 0 && expr->ex.funct.args[0]->type == EX_ARG) {
				firstarg = expr->ex.funct.args[0]->ex.arg;
				for (i = 1; i < argc; ++ i) {
					mathfun_expr *arg = expr->ex.funct.args[i];
					if (arg->type != EX_ARG || arg->ex.arg != firstarg + i) {
						break;
					}
				}

				if (firstarg + i != codegen->currstack && i != argc) {
					// didn't work out
					firstarg = oldstack;
					i = 0;
				}
			}

			// codegen for the rest of the arguments
			for (; i < argc; ++ i) {
				mathfun_expr *arg = expr->ex.funct.args[i];
				mathfun_code argret = codegen->currstack;
				if (!mathfun_codegen_expr(codegen, arg, &argret)) return false;
				if (argret != codegen->currstack &&
					!mathfun_codegen_ins2(codegen, MOV, argret, codegen->currstack)) {
					return false;
				}
				if (i + 1 < argc) {
					++ codegen->currstack;
					if (codegen->currstack > codegen->maxstack) {
						codegen->maxstack = codegen->currstack;
					}
				}
			}
			codegen->currstack = oldstack;

			return mathfun_codegen_call(codegen, expr->ex.funct.funct, firstarg, *ret);
		}
		case EX_NEG:
			return mathfun_codegen_unary(codegen, expr, NEG, ret);

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
			
		case EX_NOT:
			return mathfun_codegen_unary(codegen, expr, NOT, ret);

		case EX_EQ:
			return mathfun_codegen_binary(codegen, expr, EQ, ret);

		case EX_NE:
			return mathfun_codegen_binary(codegen, expr, NE, ret);

		case EX_LT:
			return mathfun_codegen_binary(codegen, expr, LT, ret);

		case EX_GT:
			return mathfun_codegen_binary(codegen, expr, GT, ret);

		case EX_LE:
			return mathfun_codegen_binary(codegen, expr, LE, ret);

		case EX_GE:
			return mathfun_codegen_binary(codegen, expr, GE, ret);

		case EX_AND:
		{
			mathfun_code leftret = *ret;
			if (!mathfun_codegen_expr(codegen, expr->ex.binary.left, &leftret)) return false;
//			if (!mathfun_codegen_ins2(codegen, )) return false; TODO
		}
		case EX_OR:
		case EX_IIF:
		{
			// TODO
			assert(false);
		}
	}

	mathfun_raise_error(codegen->error, MATHFUN_INTERNAL_ERROR);
	return false;
}

bool mathfun_expr_codegen(mathfun_expr *expr, mathfun *mathfun, mathfun_error_p *error) {
	if (mathfun->argc > MATHFUN_REGS_MAX) {
		mathfun_raise_error(error, MATHFUN_TOO_MANY_ARGUMENTS);
		return false;
	}

	mathfun_codegen codegen;

	memset(&codegen, 0, sizeof(mathfun_codegen));

	codegen.argc  = codegen.currstack = codegen.maxstack = mathfun->argc;
	codegen.code_size = 16;
	codegen.code  = calloc(codegen.code_size, sizeof(mathfun_code));
	codegen.error = error;

	if (!codegen.code) {
		mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
		mathfun_codegen_cleanup(&codegen);
		return false;
	}

	mathfun_code ret = mathfun->argc;
	if (!mathfun_codegen_expr(&codegen, expr, &ret) ||
		!mathfun_codegen_ins1(&codegen, RET, ret)) {
		mathfun_codegen_cleanup(&codegen);
		return false;
	}

	if (codegen.maxstack >= MATHFUN_REGS_MAX) {
		mathfun_raise_error(error, MATHFUN_EXCEEDS_MAX_FRAME_SIZE);
		return false;
	}

	mathfun->framesize = codegen.maxstack + 1;
	mathfun->code      = codegen.code;

	codegen.code = NULL;
	mathfun_codegen_cleanup(&codegen);

	return true;
}

#define MATHFUN_DUMP(ARGS) \
	if (fprintf ARGS < 0) { \
		mathfun_raise_error(error, MATHFUN_IO_ERROR); \
		return false; \
	}

bool mathfun_dump(const mathfun *mathfun, FILE *stream, const mathfun_context *ctx, mathfun_error_p *error) {
	const mathfun_code *code = mathfun->code;

	for (;;) {
		MATHFUN_DUMP((stream, "0x%08"PRIuPTR": ", code - mathfun->code));
		switch (*code) {
			case NOP:
				MATHFUN_DUMP((stream, "nop\n"));
				++ code;
				break;

			case RET:
				MATHFUN_DUMP((stream, "ret %"PRIuPTR"\n", code[1]));
				return true;

			case MOV:
				MATHFUN_DUMP((stream, "mov %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2]));
				code += 3;
				break;

			case VAL:
				MATHFUN_DUMP((stream, "val %.22g, %"PRIuPTR"\n", *(mathfun_value*)(code + 1),
					code[1 + MATHFUN_VALUE_CODES]));
				code += 2 + MATHFUN_VALUE_CODES;
				break;

			case CALL:
			{
				mathfun_binding_funct funct = *(mathfun_binding_funct*)(code + 1);
				mathfun_code firstarg = code[MATHFUN_FUNCT_CODES + 1];
				mathfun_code ret = code[MATHFUN_FUNCT_CODES + 2];
				code += 3 + MATHFUN_FUNCT_CODES;

				if (ctx) {
					const char *name = mathfun_context_funct_name(ctx, funct);
					if (name) {
						MATHFUN_DUMP((stream, "call %s, %"PRIuPTR", %"PRIuPTR"\n", name, firstarg, ret));
						break;
					}
				}

				MATHFUN_DUMP((stream, "call 0x%"PRIxPTR", %"PRIuPTR", %"PRIuPTR"\n",
					(uintptr_t)funct, firstarg, ret));
				break;
			}

			case NEG:
				MATHFUN_DUMP((stream, "neg %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2]));
				code += 3;
				break;

			case ADD:
				MATHFUN_DUMP((stream, "add %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case SUB:
				MATHFUN_DUMP((stream, "sub %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case MUL:
				MATHFUN_DUMP((stream, "mul %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case DIV:
				MATHFUN_DUMP((stream, "div %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case MOD:
				MATHFUN_DUMP((stream, "mod %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case POW:
				MATHFUN_DUMP((stream, "pow %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case NOT:
				MATHFUN_DUMP((stream, "net %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2]));
				code += 3;
				break;

			case EQ:
				MATHFUN_DUMP((stream, "eq %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case NE:
				MATHFUN_DUMP((stream, "ne %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case LT:
				MATHFUN_DUMP((stream, "lt %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case GT:
				MATHFUN_DUMP((stream, "gt %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case LE:
				MATHFUN_DUMP((stream, "le %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case GE:
				MATHFUN_DUMP((stream, "ge %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case JMP:
				MATHFUN_DUMP((stream, "jmp 0x%"PRIxPTR"\n", code[1]));
				code += 2;
				break;

			case JMPT:
				MATHFUN_DUMP((stream, "jmpt %"PRIuPTR", 0x%"PRIxPTR"\n",
					code[1], code[2]));
				code += 3;
				break;

			case JMPF:
				MATHFUN_DUMP((stream, "jmpf %"PRIuPTR", 0x%"PRIxPTR"\n",
					code[1], code[2]));
				code += 3;
				break;

			case SETT:
				MATHFUN_DUMP((stream, "sett %"PRIuPTR"\n", code[1]));
				code += 2;
				break;

			case SETF:
				MATHFUN_DUMP((stream, "setf %"PRIuPTR"\n", code[1]));
				code += 2;
				break;

			default: // assert?
				mathfun_raise_error(error, MATHFUN_INTERNAL_ERROR);
				return false;
		}
	}
}
