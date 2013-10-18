#include <errno.h>
#include <string.h>

#include "mathfun_intern.h"

typedef mathfun_value (*mathfun_binary_op)(mathfun_value a, mathfun_value b);

static mathfun_value mathfun_add(mathfun_value a, mathfun_value b) { return a + b; }
static mathfun_value mathfun_sub(mathfun_value a, mathfun_value b) { return a - b; }
static mathfun_value mathfun_mul(mathfun_value a, mathfun_value b) { return a * b; }
static mathfun_value mathfun_div(mathfun_value a, mathfun_value b) { return a / b; }

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
		{
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
				for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
					args[i] = expr->ex.funct.args[i]->ex.value;
				}
				mathfun_value value = expr->ex.funct.funct(args);
				free(args);
				expr->type = EX_CONST;
				expr->ex.value = value;
			}
			return expr;
		}

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

		case EX_ADD: return mathfun_expr_optimize_binary(expr, mathfun_add, true,    0, true);
		case EX_SUB: return mathfun_expr_optimize_binary(expr, mathfun_sub, true,    0, false);
		case EX_MUL: return mathfun_expr_optimize_binary(expr, mathfun_mul, true,    1, true);
		case EX_DIV: return mathfun_expr_optimize_binary(expr, mathfun_div, true,    1, false);
		case EX_MOD: return mathfun_expr_optimize_binary(expr, mathfun_mod, false, NAN, false);
		case EX_POW: return mathfun_expr_optimize_binary(expr, pow,         true,    1, false);
	}

	return expr;
}

void mathfun_codegen_cleanup(struct mathfun_codegen *codegen) {
	free(codegen->code);
	codegen->code = NULL;
}

int mathfun_codegen_ensure(struct mathfun_codegen *codegen, size_t n) {
	const size_t size = codegen->code_used + n;
	if (size > codegen->code_size) {
		mathfun_code *code = realloc(codegen->code, size * sizeof(mathfun_code));

		if (!code) return ENOMEM;

		codegen->code = code;
		codegen->code_size = size;
	}
	return 0;
}

int mathfun_codegen_align(struct mathfun_codegen *codegen, size_t offset, size_t align) {
	for (;;) {
		uintptr_t ptr = (uintptr_t)(codegen->code + codegen->code_used + offset);
		uintptr_t aligned = ptr & ~(align - 1);
		if (aligned == ptr) break;
		int errnum = mathfun_codegen_ensure(codegen, 1);
		if (errnum != 0) return errnum;
		codegen->code[codegen->code_used ++] = NOP;
	}
	return 0;
}

int mathfun_codegen_append(struct mathfun_codegen *codegen, enum mathfun_bytecode code, ...) {
	va_list ap;
	size_t argc = 0;
	int errnum = 0;

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
			errnum = mathfun_codegen_align(codegen, 1, sizeof(mathfun_value));
			if (errnum != 0) return errnum;

			errnum = mathfun_codegen_ensure(codegen, MATHFUN_VALUE_CODES + 2);
			if (errnum != 0) return errnum;

			va_start(ap, code);
			mathfun_value value = va_arg(ap, mathfun_value);

			codegen->code[codegen->code_used ++] = code;
			*(mathfun_value*)(codegen->code + codegen->code_used) = value;
			codegen->code_used += MATHFUN_VALUE_CODES;
			codegen->code[codegen->code_used ++] = va_arg(ap, mathfun_code);

			va_end(ap);
			return 0;

		case CALL:
			errnum = mathfun_codegen_align(codegen, 1, sizeof(mathfun_binding_funct));
			if (errnum != 0) return errnum;

			errnum = mathfun_codegen_ensure(codegen, MATHFUN_FUNCT_CODES + 3);
			if (errnum != 0) return errnum;

			va_start(ap, code);
			mathfun_binding_funct fptr = va_arg(ap, mathfun_binding_funct);

			codegen->code[codegen->code_used ++] = code;
			*(mathfun_binding_funct*)(codegen->code + codegen->code_used) = fptr;
			codegen->code_used += MATHFUN_FUNCT_CODES;
			codegen->code[codegen->code_used ++] = va_arg(ap, mathfun_code);
			codegen->code[codegen->code_used ++] = va_arg(ap, mathfun_code);
			va_end(ap);

			return 0;
	}

	errnum = mathfun_codegen_ensure(codegen, argc + 1);
	if (errnum != 0) return errnum;

	va_start(ap, code);

	codegen->code[codegen->code_used ++] = code;

	for (size_t i = 0; i < argc; ++ i) {
		codegen->code[codegen->code_used ++] = va_arg(ap, mathfun_code);
	}

	va_end(ap);

	return 0;
}

int mathfun_codegen_binary(
	struct mathfun_codegen *codegen,
	struct mathfun_expr *expr,
	enum mathfun_bytecode code,
	mathfun_code *ret) {
	int errnum = 0;
	mathfun_code leftret  = codegen->currstack;
	mathfun_code rightret;

	errnum = mathfun_codegen(codegen, expr->ex.binary.left, &leftret);
	if (errnum != 0) return errnum;

	if (leftret < codegen->currstack) {
		// returned an argument, can use unchanged currstack for right expression
		rightret = codegen->currstack;
		errnum = mathfun_codegen(codegen, expr->ex.binary.right, &rightret);
		if (errnum != 0) return errnum;
	}
	else {
		rightret = ++ codegen->currstack;
		errnum = mathfun_codegen(codegen, expr->ex.binary.right, &rightret);
		if (errnum != 0) return errnum;

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

int mathfun_codegen(struct mathfun_codegen *codegen, struct mathfun_expr *expr, mathfun_code *ret) {
	int errnum = 0;

	switch (expr->type) {
		case EX_CONST:
			return mathfun_codegen_append(codegen, VAL, expr->ex.value, *ret);

		case EX_ARG:
			*ret = expr->ex.arg;
			return 0;

		case EX_CALL:
		{
			const size_t firstarg = codegen->currstack;
			for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
				struct mathfun_expr *arg = expr->ex.funct.args[i];
				mathfun_code argret = codegen->currstack;
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
			codegen->currstack = firstarg;
			return mathfun_codegen_append(codegen, CALL, expr->ex.funct.funct, firstarg, *ret);
		}	
		case EX_NEG:
		{
			mathfun_code negret = *ret;
			errnum = mathfun_codegen(codegen, expr->ex.unary.expr, &negret);
			if (errnum != 0) return errnum;
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

	return EINVAL;
}

int mathfun_expr_codegen(struct mathfun_expr *expr, struct mathfun *mathfun) {

	if (mathfun->argc > MATHFUN_REGS_MAX) return errno = ERANGE;

	struct mathfun_codegen codegen;

	memset(&codegen, 0, sizeof(struct mathfun_codegen));

	codegen.argc = codegen.currstack = codegen.maxstack = mathfun->argc;

	codegen.code_size = 16;
	codegen.code = calloc(codegen.code_size, sizeof(mathfun_code));

	if (!codegen.code) {
		mathfun_codegen_cleanup(&codegen);
		return ENOMEM;
	}

	mathfun_code ret = mathfun->argc;
	int errnum = 0;
	if ((errnum = mathfun_codegen(&codegen, expr, &ret)) != 0 ||
		(errnum = mathfun_codegen_append(&codegen, RET, ret)) != 0) {
		mathfun_codegen_cleanup(&codegen);
		return errnum;
	}

	if (codegen.maxstack >= MATHFUN_REGS_MAX) {
		return errno = ERANGE;
	}

	mathfun->framesize = codegen.maxstack + 1;
	mathfun->code      = codegen.code;

	codegen.code = NULL;
	mathfun_codegen_cleanup(&codegen);

	return 0;
}

int mathfun_dump(const struct mathfun *mathfun, FILE *stream) {
	const mathfun_code *code = mathfun->code;

	for (;;) {
		fprintf(stream, "0x%08"PRIuPTR": ", code - mathfun->code);
		switch (*code) {
			case NOP:
				if (fprintf(stream, "nop\n") < 0)
					return errno;
				++ code;
				break;

			case RET:
				if (fprintf(stream, "ret %"PRIuPTR"\n", code[1]) < 0)
					return errno;
				return 0;

			case MOV:
				if (fprintf(stream, "mov %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2]) < 0)
					return errno;
				code += 3;
				break;

			case VAL:
				if (fprintf(stream, "val %.22g, %"PRIuPTR"\n", *(mathfun_value*)(code + 1), code[1 + MATHFUN_VALUE_CODES]) < 0)
					return errno;
				code += 2 + MATHFUN_VALUE_CODES;
				break;

			case CALL:
			{
				mathfun_binding_funct funct = *(mathfun_binding_funct*)(code + 1);
				mathfun_code firstarg = code[MATHFUN_FUNCT_CODES + 1];
				mathfun_code ret = code[MATHFUN_FUNCT_CODES + 2];
				// TODO: use optional mathfun_context to get function name
				if (fprintf(stream, "call 0x%"PRIxPTR", %"PRIuPTR", %"PRIuPTR"\n", (uintptr_t)funct, firstarg, ret) < 0) {
					return errno;
				}
				code += 3 + MATHFUN_FUNCT_CODES;
				break;
			}

			case NEG:
				if (fprintf(stream, "neg %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2]) < 0)
					return errno;
				code += 3;
				break;

			case ADD:
				if (fprintf(stream, "add %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2], code[3]) < 0)
					return errno;
				code += 4;
				break;

			case SUB:
				if (fprintf(stream, "sub %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2], code[3]) < 0)
					return errno;
				code += 4;
				break;

			case MUL:
				if (fprintf(stream, "add %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2], code[3]) < 0)
					return errno;
				code += 4;
				break;

			case DIV:
				if (fprintf(stream, "div %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2], code[3]) < 0)
					return errno;
				code += 4;
				break;

			case MOD:
				if (fprintf(stream, "mod %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2], code[3]) < 0)
					return errno;
				code += 4;
				break;

			case POW:
				if (fprintf(stream, "pow %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2], code[3]) < 0)
					return errno;
				code += 4;
				break;

			default:
				return EINVAL;
		}
	}
}
