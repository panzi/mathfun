#include <errno.h>
#include <string.h>

#include "mathfun_intern.h"

typedef bool (*mathfun_binary_op)(mathfun_value a, mathfun_value b, mathfun_value *res, mathfun_error_p *error);

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
		if (!op(expr->ex.binary.left->ex.value, expr->ex.binary.right->ex.value, &value, error)) {
			mathfun_expr_free(expr);
			return NULL;
		}

		mathfun_expr_free(expr->ex.binary.left);
		mathfun_expr_free(expr->ex.binary.right);
		expr->type = EX_CONST;
		expr->ex.value = value;
	}
	else if (has_neutral) {
		if (expr->ex.binary.right->type == EX_CONST && expr->ex.binary.right->ex.value == neutral) {
			mathfun_expr *left = expr->ex.binary.left;
			expr->ex.binary.left = NULL;
			mathfun_expr_free(expr);
			expr = left;
		}
		else if (commutative && expr->ex.binary.left->type == EX_CONST &&
			expr->ex.binary.left->ex.value == neutral) {
			mathfun_expr *right = expr->ex.binary.right;
			expr->ex.binary.right = NULL;
			mathfun_expr_free(expr);
			expr = right;
		}
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
			for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
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
				mathfun_value *args = calloc(expr->ex.funct.argc, sizeof(mathfun_value));
				if (!args) return NULL;
				for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
					args[i] = expr->ex.funct.args[i]->ex.value;
				}

				// math errors are communicated via errno
				// XXX: buggy. see NOTES in man math_error
				errno = 0;
				mathfun_value value = expr->ex.funct.funct(args);

				free(args);
				expr->type = EX_CONST;
				expr->ex.value = value;

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
				child->ex.value = -child->ex.value;
				return child;
			}
			return expr;

		case EX_ADD: return mathfun_expr_optimize_binary(expr, mathfun_opt_add, true,    0, true,  error);
		case EX_SUB: return mathfun_expr_optimize_binary(expr, mathfun_opt_sub, true,    0, false, error);
		case EX_MUL: return mathfun_expr_optimize_binary(expr, mathfun_opt_mul, true,    1, true,  error);
		case EX_DIV: return mathfun_expr_optimize_binary(expr, mathfun_opt_div, true,    1, false, error);
		case EX_MOD: return mathfun_expr_optimize_binary(expr, mathfun_opt_mod, false, NAN, false, error);
		case EX_POW: return mathfun_expr_optimize_binary(expr, mathfun_opt_pow, true,    1, false, error);
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
			mathfun_raise_error(codegen->error, MATHFUN_MEMORY_ERROR);
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

bool mathfun_codegen_append(mathfun_codegen *codegen, enum mathfun_bytecode code, ...) {
	va_list ap;
	size_t argc = 0;

	switch (code) {
		case NOP: argc = 0; break;
		case RET: argc = 1; break;
		case MOV:
		case NEG: argc = 2; break;
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case MOD:
		case POW: argc = 3; break;
		case VAL:
			if (!mathfun_codegen_align(codegen, 1, sizeof(mathfun_value))) return false;
			if (!mathfun_codegen_ensure(codegen, MATHFUN_VALUE_CODES + 2)) return false;

			va_start(ap, code);
			mathfun_value value = va_arg(ap, mathfun_value);

			codegen->code[codegen->code_used ++] = code;
			*(mathfun_value*)(codegen->code + codegen->code_used) = value;
			codegen->code_used += MATHFUN_VALUE_CODES;
			codegen->code[codegen->code_used ++] = va_arg(ap, mathfun_code);
			va_end(ap);

			return true;

		case CALL:
			if (!mathfun_codegen_align(codegen, 1, sizeof(mathfun_binding_funct))) return false;
			if (!mathfun_codegen_ensure(codegen, MATHFUN_FUNCT_CODES + 3)) return false;

			va_start(ap, code);
			mathfun_binding_funct fptr = va_arg(ap, mathfun_binding_funct);

			codegen->code[codegen->code_used ++] = code;
			*(mathfun_binding_funct*)(codegen->code + codegen->code_used) = fptr;
			codegen->code_used += MATHFUN_FUNCT_CODES;
			codegen->code[codegen->code_used ++] = va_arg(ap, mathfun_code);
			codegen->code[codegen->code_used ++] = va_arg(ap, mathfun_code);
			va_end(ap);

			return true;
	}

	if (!mathfun_codegen_ensure(codegen, argc + 1)) return false;

	va_start(ap, code);

	codegen->code[codegen->code_used ++] = code;

	for (size_t i = 0; i < argc; ++ i) {
		codegen->code[codegen->code_used ++] = va_arg(ap, mathfun_code);
	}

	va_end(ap);

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

	return mathfun_codegen_append(codegen, code, leftret, rightret, *ret);
}

bool mathfun_codegen_expr(mathfun_codegen *codegen, mathfun_expr *expr, mathfun_code *ret) {
	switch (expr->type) {
		case EX_CONST:
			return mathfun_codegen_append(codegen, VAL, expr->ex.value, *ret);

		case EX_ARG:
			*ret = expr->ex.arg;
			return true;

		case EX_CALL:
		{
			const size_t firstarg = codegen->currstack;
			for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
				mathfun_expr *arg = expr->ex.funct.args[i];
				mathfun_code argret = codegen->currstack;
				if (!mathfun_codegen_expr(codegen, arg, &argret)) return false;
				if (argret != codegen->currstack &&
					!mathfun_codegen_append(codegen, MOV, argret, codegen->currstack)) {
					return false;
				}
				if (i + 1 < expr->ex.funct.argc) {
					++ codegen->currstack;
					if (codegen->currstack > codegen->maxstack) {
						codegen->maxstack = codegen->currstack;
					}
				}
			}
			codegen->currstack = firstarg;
			return mathfun_codegen_append(codegen, CALL, expr->ex.funct.funct, firstarg, *ret);
		}	
		case EX_NEG:
		{
			mathfun_code negret = *ret;
			if (!mathfun_codegen_expr(codegen, expr->ex.unary.expr, &negret)) return false;
			return mathfun_codegen_append(codegen, NEG, negret, *ret);
		}
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
		mathfun_raise_error(error, MATHFUN_MEMORY_ERROR);
		mathfun_codegen_cleanup(&codegen);
		return false;
	}

	mathfun_code ret = mathfun->argc;
	if (!mathfun_codegen_expr(&codegen, expr, &ret) ||
		!mathfun_codegen_append(&codegen, RET, ret)) {
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

			default: // assert?
				mathfun_raise_error(error, MATHFUN_INTERNAL_ERROR);
				return false;
		}
	}
}
