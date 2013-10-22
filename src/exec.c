#include <errno.h>

#include "mathfun_intern.h"

// tree interpreter, for one time execution and debugging
mathfun_value mathfun_expr_exec(const mathfun_expr *expr, const mathfun_value args[],
	mathfun_error_p *error) {
	switch (expr->type) {
		case EX_CONST:
			return expr->ex.value;

		case EX_ARG:
			return args[expr->ex.arg];

		case EX_CALL:
		{
			mathfun_value *funct_args = malloc(expr->ex.funct.argc * sizeof(mathfun_value));
			if (!funct_args) {
				mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
				return NAN;
			}
			for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
				funct_args[i] = mathfun_expr_exec(expr->ex.funct.args[i], args, error);
			}
			errno = 0;
			mathfun_value value = expr->ex.funct.funct(funct_args);
			free(funct_args);

			switch (errno) {
				case 0: break; // good
				case ERANGE:
				case EDOM:
					mathfun_raise_math_error(error, errno);
					break;

				default:
					mathfun_raise_error(error, MATHFUN_C_ERROR);
					break;
			}

			return value;
		}
		case EX_NEG:
			return -mathfun_expr_exec(expr->ex.unary.expr, args, error);

		case EX_ADD:
			return mathfun_expr_exec(expr->ex.binary.left, args, error) +
			       mathfun_expr_exec(expr->ex.binary.right, args, error);

		case EX_SUB:
			return mathfun_expr_exec(expr->ex.binary.left, args, error) -
			       mathfun_expr_exec(expr->ex.binary.right, args, error);

		case EX_MUL:
			return mathfun_expr_exec(expr->ex.binary.left, args, error) *
			       mathfun_expr_exec(expr->ex.binary.right, args, error);

		case EX_DIV:
			return mathfun_expr_exec(expr->ex.binary.left, args, error) /
			       mathfun_expr_exec(expr->ex.binary.right, args, error);

		case EX_MOD:
		{
			mathfun_value left  = mathfun_expr_exec(expr->ex.binary.left, args, error);
			mathfun_value right = mathfun_expr_exec(expr->ex.binary.right, args, error);

			if (right == 0.0) {
				mathfun_raise_math_error(error, EDOM);
				return NAN;
			}

			mathfun_value mathfun_mod_result;
			MATHFUN_MOD(left, right);
			return mathfun_mod_result;
		}
		case EX_POW:
			return pow(mathfun_expr_exec(expr->ex.binary.left, args, error),
		               mathfun_expr_exec(expr->ex.binary.right, args, error));
	}
	mathfun_raise_error(error, MATHFUN_INTERNAL_ERROR);
	return NAN;
}

#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wunused-label"
mathfun_value mathfun_exec(const mathfun *mathfun, mathfun_value regs[]) {
	const mathfun_code *code = mathfun->code;

#ifdef __GNUC__
	// http://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html
	// use offsets instead of absolute addresses to reduce the number of
	// dynamic relocations for code in shared libraries
	//
	// TODO: same for llvm clang
	// http://blog.llvm.org/2010/01/address-of-label-and-indirect-branches.html
	static const intptr_t jump_table[] = {
		/* NOP  */ &&do_nop  - &&do_add,
		/* RET  */ &&do_ret  - &&do_add,
		/* MOV  */ &&do_mov  - &&do_add,
		/* VAL  */ &&do_val  - &&do_add,
		/* CALL */ &&do_call - &&do_add,
		/* NEG  */ &&do_neg  - &&do_add,
		/* ADD  */ &&do_add  - &&do_add,
		/* SUB  */ &&do_sub  - &&do_add,
		/* MUL  */ &&do_mul  - &&do_add,
		/* DIV  */ &&do_div  - &&do_add,
		/* MOD  */ &&do_mod  - &&do_add,
		/* POW  */ &&do_pow  - &&do_add
	};

#	define DISPATCH goto *(&&do_add + jump_table[*code]);
	DISPATCH;
#else
#	define DISPATCH break;
#endif

	for (;;) {
		switch (*code) {
			case ADD:
do_add:
				regs[code[3]] = regs[code[1]] + regs[code[2]];
				code += 4;
				DISPATCH;

			case SUB:
do_sub:
				regs[code[3]] = regs[code[1]] - regs[code[2]];
				code += 4;
				DISPATCH;

			case MUL:
do_mul:
				regs[code[3]] = regs[code[1]] * regs[code[2]];
				code += 4;
				DISPATCH;

			case DIV:
do_div:
				regs[code[3]] = regs[code[1]] / regs[code[2]];
				code += 4;
				DISPATCH;

			case MOD:
do_mod:
			{
				mathfun_value left  = regs[code[1]];
				mathfun_value right = regs[code[2]];
				mathfun_value mathfun_mod_result;
				
				if (right == 0.0) {
					// consistenly just act c-like here
					// caller has to handle everything
					errno = EDOM;
					return NAN;
				}

				MATHFUN_MOD(left, right);
				regs[code[3]] = mathfun_mod_result;
				code += 4;
				DISPATCH;
			}
			case POW:
do_pow:
				regs[code[3]] = pow(regs[code[1]], regs[code[2]]);
				code += 4;
				DISPATCH;

			case NEG:
do_neg:
				regs[code[2]] = -regs[code[1]];
				code += 3;
				DISPATCH;

			case VAL:
do_val:
				regs[code[1 + MATHFUN_VALUE_CODES]] = *(mathfun_value*)(code + 1);
				code += 2 + MATHFUN_VALUE_CODES;
				DISPATCH;

			case CALL:
do_call:
			{
				mathfun_binding_funct funct = *(mathfun_binding_funct*)(code + 1);
				code += 1 + MATHFUN_FUNCT_CODES;
				mathfun_code firstarg = *(code ++);
				mathfun_code ret      = *(code ++);
				regs[ret] = funct(regs + firstarg);
				DISPATCH;
			}	
			case MOV:
do_mov:
				regs[code[2]] = regs[code[1]];
				code += 3;
				DISPATCH;

			case RET:
do_ret:
				return regs[code[1]];

			case NOP:
do_nop:
				++ code;
				DISPATCH;
		}
	}
}
#pragma GCC diagnostic pop
