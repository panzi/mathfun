#include <errno.h>

#include "mathfun_intern.h"

// tree interpreter, for one time execution and debugging
mathfun_value mathfun_expr_exec(const mathfun_expr *expr, const double args[]) {
	switch (expr->type) {
		case EX_CONST:
			return expr->ex.value.value;

		case EX_ARG:
			return (mathfun_value){ .number = args[expr->ex.arg] };

		case EX_CALL:
		{
			const size_t argc = expr->ex.funct.sig->argc;
			mathfun_value *funct_args = malloc(argc * sizeof(mathfun_value));
			if (!funct_args) {
				if (errno == 0) errno = ENOMEM;
				return (mathfun_value){ .number = NAN };
			}
			for (size_t i = 0; i < argc; ++ i) {
				funct_args[i] = mathfun_expr_exec(expr->ex.funct.args[i], args);
			}
			mathfun_value value = expr->ex.funct.funct(funct_args);
			free(funct_args);

			return value;
		}
		case EX_NEG:
			return (mathfun_value){ .number =
				-mathfun_expr_exec(expr->ex.unary.expr, args).number };

		case EX_ADD:
			return (mathfun_value){ .number =
				mathfun_expr_exec(expr->ex.binary.left, args).number +
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_SUB:
			return (mathfun_value){ .number =
				mathfun_expr_exec(expr->ex.binary.left, args).number -
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_MUL:
			return (mathfun_value){ .number =
				mathfun_expr_exec(expr->ex.binary.left, args).number *
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_DIV:
			return (mathfun_value){ .number =
				mathfun_expr_exec(expr->ex.binary.left, args).number /
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_MOD:
			return (mathfun_value){ .number = mathfun_mod(
				mathfun_expr_exec(expr->ex.binary.left, args).number,
				mathfun_expr_exec(expr->ex.binary.right, args).number) };

		case EX_POW:
			return (mathfun_value){ .number = pow(
				mathfun_expr_exec(expr->ex.binary.left, args).number,
				mathfun_expr_exec(expr->ex.binary.right, args).number) };

		case EX_NOT:
			return (mathfun_value){ .boolean = !mathfun_expr_exec(expr->ex.unary.expr, args).boolean };

		case EX_EQ:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).number ==
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_NE:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).number !=
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_LT:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).number <
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_GT:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).number >
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_LE:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).number <=
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_GE:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).number >=
				mathfun_expr_exec(expr->ex.binary.right, args).number };

		case EX_BEQ:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).boolean ==
				mathfun_expr_exec(expr->ex.binary.right, args).boolean };

		case EX_BNE:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).boolean !=
				mathfun_expr_exec(expr->ex.binary.right, args).boolean };

		case EX_AND:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).boolean &&
				mathfun_expr_exec(expr->ex.binary.right, args).boolean };

		case EX_OR:
			return (mathfun_value){ .boolean =
				mathfun_expr_exec(expr->ex.binary.left, args).boolean ||
				mathfun_expr_exec(expr->ex.binary.right, args).boolean };

		case EX_IIF:
			return mathfun_expr_exec(expr->ex.iif.cond, args).boolean ?
				mathfun_expr_exec(expr->ex.iif.then_expr, args) :
				mathfun_expr_exec(expr->ex.iif.else_expr, args);

		case EX_IN:
		{
			double value = mathfun_expr_exec(expr->ex.binary.left, args).number;
			mathfun_expr *range = expr->ex.binary.right;
			return (mathfun_value){ .boolean = range->type == EX_RNG_INCL ?
				value >= mathfun_expr_exec(range->ex.binary.left, args).number &&
				value <= mathfun_expr_exec(range->ex.binary.right, args).number :

				value >= mathfun_expr_exec(range->ex.binary.left, args).number &&
				value <  mathfun_expr_exec(range->ex.binary.right, args).number};
		}

		case EX_RNG_INCL:
		case EX_RNG_EXCL:
			break;
	}
	errno = EINVAL;
	return (mathfun_value){ .number = NAN };
}

double mathfun_exec(const mathfun *fun, mathfun_value regs[]) {
	const mathfun_code *start = fun->code;
	const mathfun_code *code  = fun->code;

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wpointer-arith"
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
		/* BEQ  */ &&do_beq  - &&do_add,
		/* BNE  */ &&do_bne  - &&do_add,
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
				regs[code[3]].number = mathfun_mod(regs[code[1]].number, regs[code[2]].number);
				code += 4;
				DISPATCH;

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

			case BEQ:
do_beq:
				regs[code[3]].boolean = regs[code[1]].boolean == regs[code[2]].boolean;
				code += 4;
				DISPATCH;

			case BNE:
do_bne:
				regs[code[3]].boolean = regs[code[1]].boolean != regs[code[2]].boolean;
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

			default:
				errno = EINVAL;
				return NAN;
		}
	}
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
