#include <errno.h>

#include "mathfun_intern.h"

// tree interpreter, for one time execution and debugging
mathfun_value mathfun_expr_exec(const struct mathfun_expr *expr, mathfun_value args[]) {
	switch (expr->type) {
		case EX_CONST:
			return expr->ex.value;

		case EX_ARG:
			return args[expr->ex.arg];

		case EX_CALL:
		{
			mathfun_value *funct_args = malloc(expr->ex.funct.argc * sizeof(mathfun_value));
			if (!funct_args) {
				return NAN; // ENOMEM
			}
			for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
				funct_args[i] = mathfun_expr_exec(expr->ex.funct.args[i], args);
			}
			mathfun_value value = expr->ex.funct.funct(funct_args);
			free(funct_args);
			return value;
		}
		case EX_NEG:
			return -mathfun_expr_exec(expr->ex.unary.expr, args);

		case EX_ADD:
			return mathfun_expr_exec(expr->ex.binary.left, args) +
			       mathfun_expr_exec(expr->ex.binary.right, args);

		case EX_SUB:
			return mathfun_expr_exec(expr->ex.binary.left, args) -
			       mathfun_expr_exec(expr->ex.binary.right, args);

		case EX_MUL:
			return mathfun_expr_exec(expr->ex.binary.left, args) *
			       mathfun_expr_exec(expr->ex.binary.right, args);

		case EX_DIV:
			return mathfun_expr_exec(expr->ex.binary.left, args) /
			       mathfun_expr_exec(expr->ex.binary.right, args);

		case EX_MOD:
		{
			mathfun_value left  = mathfun_expr_exec(expr->ex.binary.left, args);
			mathfun_value right = mathfun_expr_exec(expr->ex.binary.right, args);
			MATHFUN_MOD(left, right);
			return mathfun_mod_result;
		}
		case EX_POW:
			return pow(mathfun_expr_exec(expr->ex.binary.left, args),
		               mathfun_expr_exec(expr->ex.binary.right, args));
	}
	// internal error
	return NAN;
}

mathfun_value mathfun_exec(const struct mathfun *mathfun, mathfun_value regs[]) {
#ifdef __GNUC__
	static const void *jump_table[] = {
		/* NOP  */ &&do_nop,
		/* RET  */ &&do_ret,
		/* MOV  */ &&do_mov,
		/* VAL  */ &&do_val,
		/* CALL */ &&do_call,
		/* NEG  */ &&do_neg,
		/* ADD  */ &&do_add,
		/* SUB  */ &&do_sub,
		/* MUL  */ &&do_mul,
		/* DIV  */ &&do_div,
		/* MOD  */ &&do_mod,
		/* POW  */ &&do_pow
	};
#endif
	const mathfun_code *code = mathfun->code;

	for (;;) {
#ifdef __GNUC__
		goto *jump_table[*code];
#endif

		switch (*code) {
			case ADD:
do_add:
				regs[code[3]] = regs[code[1]] + regs[code[2]];
				code += 4;
				break;

			case SUB:
do_sub:
				regs[code[3]] = regs[code[1]] - regs[code[2]];
				code += 4;
				break;

			case MUL:
do_mul:
				regs[code[3]] = regs[code[1]] * regs[code[2]];
				code += 4;
				break;

			case DIV:
do_div:
				regs[code[3]] = regs[code[1]] / regs[code[2]];
				code += 4;
				break;

			case MOD:
do_mod:
			{
				mathfun_value left  = regs[code[1]];
				mathfun_value right = regs[code[2]];
				MATHFUN_MOD(left, right);
				regs[code[3]] = mathfun_mod_result;
				code += 4;
				break;
			}
			case POW:
do_pow:
				regs[code[3]] = pow(regs[code[1]], regs[code[2]]);
				code += 4;
				break;

			case NEG:
do_neg:
				regs[code[2]] = -regs[code[1]];
				code += 3;
				break;

			case VAL:
do_val:
				regs[code[1 + MATHFUN_VALUE_CODES]] = *(mathfun_value*)(code + 1);
				code += 2 + MATHFUN_VALUE_CODES;
				break;

			case CALL:
do_call:
			{
				mathfun_binding_funct funct = *(mathfun_binding_funct)(code + 1);
				code += MATHFUN_FUNCT_CODES;
				mathfun_code firstarg = *(code ++);
				mathfun_code ret      = *(code ++);
				regs[ret] = funct(regs + firstarg);
				break;
			}	
			case MOV:
do_mov:
				regs[code[2]] = regs[code[1]];
				code += 3;
				break;

			case RET:
do_ret:
				return regs[code[1]];

			case NOP:
do_nop:
				++ code;
				break;
		}
	}
}
