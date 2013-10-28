#include <errno.h>

#include "mathfun_intern.h"

// tree interpreter, for one time execution and debugging
mathfun_reg mathfun_expr_exec(const mathfun_expr *expr, const double args[],
	mathfun_error_p *error) {
	switch (expr->type) {
		case EX_CONST:
			return expr->ex.value.value;

		case EX_ARG:
			return (mathfun_reg){ .number = args[expr->ex.arg] };

		case EX_CALL:
		{
			const size_t argc = expr->ex.funct.sig->argc;
			mathfun_reg *funct_args = malloc(argc * sizeof(mathfun_reg));
			if (!funct_args) {
				mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
				return (mathfun_reg){ .number = NAN };
			}
			for (size_t i = 0; i < argc; ++ i) {
				funct_args[i] = mathfun_expr_exec(expr->ex.funct.args[i], args, error);
			}
			errno = 0;
			mathfun_reg value = expr->ex.funct.funct(funct_args);
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
			return (mathfun_reg){ .number =
				-mathfun_expr_exec(expr->ex.unary.expr, args, error).number };

		case EX_ADD:
			return (mathfun_reg){ .number =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number +
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_SUB:
			return (mathfun_reg){ .number =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number -
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_MUL:
			return (mathfun_reg){ .number =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number *
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_DIV:
			return (mathfun_reg){ .number =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number /
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_MOD:
		{
			double left  = mathfun_expr_exec(expr->ex.binary.left, args, error).number;
			double right = mathfun_expr_exec(expr->ex.binary.right, args, error).number;

			if (right == 0.0) {
				mathfun_raise_math_error(error, EDOM);
				return (mathfun_reg){ .number = NAN };
			}

			double mathfun_mod_result;
			MATHFUN_MOD(left, right);
			return (mathfun_reg){ .number = mathfun_mod_result };
		}
		case EX_POW:
			return (mathfun_reg){ .number = pow(
				mathfun_expr_exec(expr->ex.binary.left, args, error).number,
				mathfun_expr_exec(expr->ex.binary.right, args, error).number) };

		case EX_NOT:
			return (mathfun_reg){ .boolean = !mathfun_expr_exec(expr->ex.unary.expr, args, error).boolean };

		case EX_EQ:
			return (mathfun_reg){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number ==
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_NE:
			return (mathfun_reg){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number !=
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_LT:
			return (mathfun_reg){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number <
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_GT:
			return (mathfun_reg){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number >
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_LE:
			return (mathfun_reg){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number <=
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_GE:
			return (mathfun_reg){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args, error).number >=
				mathfun_expr_exec(expr->ex.binary.right, args, error).number };

		case EX_AND:
			return (mathfun_reg){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args, error).boolean &&
				mathfun_expr_exec(expr->ex.binary.right, args, error).boolean };

		case EX_OR:
			return (mathfun_reg){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args, error).boolean ||
				mathfun_expr_exec(expr->ex.binary.right, args, error).boolean };

		case EX_IIF:
			return mathfun_expr_exec(expr->ex.iif.cond, args, error).boolean ?
				mathfun_expr_exec(expr->ex.iif.then_expr, args, error) :
				mathfun_expr_exec(expr->ex.iif.else_expr, args, error);
	}
	mathfun_raise_error(error, MATHFUN_INTERNAL_ERROR);
	return (mathfun_reg){ .number = NAN };
}

#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wunused-label"
double mathfun_exec(const mathfun *mathfun, mathfun_reg regs[]) {
	const mathfun_code *start = mathfun->code;
	const mathfun_code *code  = mathfun->code;

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
		/* POW  */ &&do_pow  - &&do_add,
		/* NOT  */ &&do_not  - &&do_add,
		/* EQ   */ &&do_eq   - &&do_add,
		/* NE   */ &&do_ne   - &&do_add,
		/* LT   */ &&do_lt   - &&do_add,
		/* GT   */ &&do_gt   - &&do_add,
		/* LE   */ &&do_le   - &&do_add,
		/* GE   */ &&do_ge   - &&do_add,
		/* JMP  */ &&do_jmp  - &&do_add,
		/* JMPT */ &&do_jmpt - &&do_add,
		/* JMPF */ &&do_jmpf - &&do_add,
		/* SETT */ &&do_sett - &&do_add,
		/* SETF */ &&do_setf - &&do_add
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
				regs[code[3]].number = regs[code[1]].number + regs[code[2]].number;
				code += 4;
				DISPATCH;

			case SUB:
do_sub:
				regs[code[3]].number = regs[code[1]].number - regs[code[2]].number;
				code += 4;
				DISPATCH;

			case MUL:
do_mul:
				regs[code[3]].number = regs[code[1]].number * regs[code[2]].number;
				code += 4;
				DISPATCH;

			case DIV:
do_div:
				regs[code[3]].number = regs[code[1]].number / regs[code[2]].number;
				code += 4;
				DISPATCH;

			case MOD:
do_mod:
			{
				double left  = regs[code[1]].number;
				double right = regs[code[2]].number;
				double mathfun_mod_result;
				
				if (right == 0.0) {
					// consistenly just act c-like here
					// caller has to handle everything
					errno = EDOM;
					return NAN;
				}

				MATHFUN_MOD(left, right);
				regs[code[3]].number = mathfun_mod_result;
				code += 4;
				DISPATCH;
			}
			case POW:
do_pow:
				regs[code[3]].number = pow(regs[code[1]].number, regs[code[2]].number);
				code += 4;
				DISPATCH;

			case NEG:
do_neg:
				regs[code[2]].number = -regs[code[1]].number;
				code += 3;
				DISPATCH;

			case VAL:
do_val:
				regs[code[1 + MATHFUN_VALUE_CODES]].number = *(double*)(code + 1);
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

			case NOT:
do_not:
				regs[code[2]].boolean = !regs[code[1]].boolean;
				code += 3;
				DISPATCH;

			case EQ:
do_eq:
				regs[code[3]].boolean = regs[code[1]].number == regs[code[2]].number;
				code += 4;
				DISPATCH;

			case NE:
do_ne:
				regs[code[3]].boolean = regs[code[1]].number != regs[code[2]].number;
				code += 4;
				DISPATCH;

			case LT:
do_lt:
				regs[code[3]].boolean = regs[code[1]].number < regs[code[2]].number;
				code += 4;
				DISPATCH;

			case GT:
do_gt:
				regs[code[3]].boolean = regs[code[1]].number > regs[code[2]].number;
				code += 4;
				DISPATCH;

			case LE:
do_le:
				regs[code[3]].boolean = regs[code[1]].number <= regs[code[2]].number;
				code += 4;
				DISPATCH;

			case GE:
do_ge:
				regs[code[3]].boolean = regs[code[1]].number >= regs[code[2]].number;
				code += 4;
				DISPATCH;

			case JMP:
do_jmp:
				code = start + code[1];
				DISPATCH;

			case JMPT:
do_jmpt:
				if (regs[code[1]].boolean) {
					code = start + code[2];
				}
				else {
					code += 3;
				}
				DISPATCH;

			case JMPF:
do_jmpf:
				if (regs[code[1]].boolean) {
					code += 3;
				}
				else {
					code = start + code[2];
				}
				DISPATCH;

			case SETT:
do_sett:
				regs[code[1]].boolean = true;
				code += 2;
				DISPATCH;

			case SETF:
do_setf:
				regs[code[1]].boolean = false;
				code += 2;
				DISPATCH;

			case RET:
do_ret:
				return regs[code[1]].number;

			case NOP:
do_nop:
				++ code;
				DISPATCH;
		}
	}
}
#pragma GCC diagnostic pop
