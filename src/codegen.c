#include <string.h>

#include "mathfun_intern.h"

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
	const uintptr_t mask = ~(align - 1);
	for (;;) {
		uintptr_t ptr = (uintptr_t)(codegen->code + codegen->code_used + offset);
		uintptr_t aligned = ptr & mask;
		if (aligned == ptr) break;
		if (!mathfun_codegen_ensure(codegen, 1)) return false;
		codegen->code[codegen->code_used ++] = NOP;
	}
	return true;
}

bool mathfun_codegen_val(mathfun_codegen *codegen, mathfun_reg value, mathfun_code target) {
	if (!mathfun_codegen_align(codegen, 1, sizeof(mathfun_reg))) return false;
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

static bool mathfun_codegen_range(
	mathfun_codegen *codegen,
	mathfun_expr *expr,
	mathfun_code valuereg,
	mathfun_code *ret) {
	mathfun_code rngret = codegen->currstack;
	if (!mathfun_codegen_expr(codegen, expr->ex.binary.left, &rngret)) return false;

	const mathfun_code lowerret = codegen->currstack;
	if (!mathfun_codegen_ins3(codegen, GE, valuereg, rngret, lowerret)) return false;

	size_t adr = codegen->code_used + 2;
	if (!mathfun_codegen_ins2(codegen, JMPF, lowerret, 0)) return false;

	rngret = codegen->currstack;
	if (!mathfun_codegen_expr(codegen, expr->ex.binary.right, &rngret)) return false;

	const mathfun_code upperret = codegen->currstack;
	if (!mathfun_codegen_ins3(codegen, expr->type == EX_RNG_INCL ? LE : LT, valuereg, rngret, upperret)) return false;

	if (upperret != *ret) {
		if (!mathfun_codegen_ins2(codegen, MOV, upperret, *ret)) return false;
	}
	codegen->code[adr] = codegen->code_used;
	if (lowerret != *ret) {
		return mathfun_codegen_ins1(codegen, SETF, *ret);
	}

	return true;
}

static bool mathfun_codegen_in(
	mathfun_codegen *codegen,
	mathfun_expr *expr,
	mathfun_code *ret) {
	mathfun_code valueret = codegen->currstack;

	if (!mathfun_codegen_expr(codegen, expr->ex.binary.left, &valueret)) return false;

	if (valueret < codegen->currstack) {
		return mathfun_codegen_range(codegen, expr->ex.binary.right, valueret, ret);
	}
	else {
		++ codegen->currstack;
		if (!mathfun_codegen_range(codegen, expr->ex.binary.right, valueret, ret)) return false;

		// doing this *after* the codegen for the range expression
		// optimizes the case where no extra register is needed (e.g. it
		// just accesses an argument registers)
		if (codegen->maxstack < codegen->currstack) {
			codegen->maxstack = codegen->currstack;
		}
		-- codegen->currstack;

		return true;
	}
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

		case EX_IN:
			return mathfun_codegen_in(codegen, expr, ret);
		
		case EX_RNG_INCL:
		case EX_RNG_EXCL:
			mathfun_raise_error(codegen->error, MATHFUN_INTERNAL_ERROR);
			return false;

		case EX_BEQ:
			return mathfun_codegen_binary(codegen, expr, BEQ, ret);

		case EX_BNE:
			return mathfun_codegen_binary(codegen, expr, BNE, ret);

		case EX_AND:
		{
			mathfun_code leftret = *ret;
			if (!mathfun_codegen_expr(codegen, expr->ex.binary.left, &leftret)) return false;
			size_t adr = codegen->code_used + 2;
			if (!mathfun_codegen_ins2(codegen, JMPF, leftret, 0)) return false;
			mathfun_code rightret = *ret;
			if (!mathfun_codegen_expr(codegen, expr->ex.binary.right, &rightret)) return false;
			if (rightret != *ret) {
				if (!mathfun_codegen_ins2(codegen, MOV, rightret, *ret)) return false;
			}
			codegen->code[adr] = codegen->code_used;
			if (leftret != *ret) {
				return mathfun_codegen_ins1(codegen, SETF, *ret);
			}
			return true;
		}
		case EX_OR:
		{
			mathfun_code leftret = *ret;
			if (!mathfun_codegen_expr(codegen, expr->ex.binary.left, &leftret)) return false;
			size_t adr = codegen->code_used + 2;
			if (!mathfun_codegen_ins2(codegen, JMPT, leftret, 0)) return false;
			mathfun_code rightret = *ret;
			if (!mathfun_codegen_expr(codegen, expr->ex.binary.right, &rightret)) return false;
			if (rightret != *ret) {
				if (!mathfun_codegen_ins2(codegen, MOV, rightret, *ret)) return false;
			}
			codegen->code[adr] = codegen->code_used;
			if (leftret != *ret) {
				return mathfun_codegen_ins1(codegen, SETT, *ret);
			}
			return true;
		}
		case EX_IIF:
		{
			mathfun_code childret = *ret;
			if (!mathfun_codegen_expr(codegen, expr->ex.iif.cond, &childret)) return false;
			size_t adr1 = codegen->code_used + 2;
			if (!mathfun_codegen_ins2(codegen, JMPF, childret, 0)) return false;
			childret = *ret;
			if (!mathfun_codegen_expr(codegen, expr->ex.iif.then_expr, &childret)) return false;
			if (childret != *ret) {
				if (!mathfun_codegen_ins2(codegen, MOV, childret, *ret)) return false;
			}
			size_t adr2 = codegen->code_used + 1;
			if (!mathfun_codegen_ins1(codegen, JMP, 0)) return false;
			codegen->code[adr1] = codegen->code_used;
			childret = *ret;
			if (!mathfun_codegen_expr(codegen, expr->ex.iif.else_expr, &childret)) return false;
			if (childret != *ret) {
				if (!mathfun_codegen_ins2(codegen, MOV, childret, *ret)) return false;
			}
			codegen->code[adr2] = codegen->code_used;
			return true;
		}
	}

	mathfun_raise_error(codegen->error, MATHFUN_INTERNAL_ERROR);
	return false;
}

// shortcut unconditional jump chain to RET
static bool mathfun_code_shortcut_jmp_to_ret(mathfun_code *code, mathfun_code *ptr, mathfun_code *retptr) {
	switch (ptr[0]) {
	case RET:
		if (retptr) *retptr = ptr[1];
		return true;
	
	case JMP:
	{
		mathfun_code ret = 0;
		bool shortcut = mathfun_code_shortcut_jmp_to_ret(code, code + ptr[1], &ret);

		if (shortcut) {
			ptr[0] = RET;
			ptr[1] = ret;
			if (retptr) *retptr = ret;
		}
		return shortcut;
	}
	default:
		return false;
	}
}

// shortcut unconditional jump chain
static mathfun_code mathfun_code_shortcut_jmp(mathfun_code *code, mathfun_code *ptr) {
	if (ptr[0] == JMP) {
		return (ptr[1] = mathfun_code_shortcut_jmp(code, code + ptr[1]));
	}
	else {
		return ptr - code;
	}
}

// shortcut conditional jump chain on same condition register
static mathfun_code mathfun_code_shortcut_jmptf(mathfun_code *code, mathfun_code *ptr,
	enum mathfun_bytecode instr, mathfun_code reg) {
	if (ptr[0] == instr && ptr[1] == reg) {
		return (ptr[2] = mathfun_code_shortcut_jmptf(code, code + ptr[2], instr, reg));
	}
	else {
		return ptr - code;
	}
}

bool mathfun_expr_codegen(mathfun_expr *expr, mathfun *fun, mathfun_error_p *error) {
	if (fun->argc > MATHFUN_REGS_MAX) {
		mathfun_raise_error(error, MATHFUN_TOO_MANY_ARGUMENTS);
		return false;
	}

	mathfun_codegen codegen;

	memset(&codegen, 0, sizeof(struct mathfun_codegen));

	codegen.argc  = codegen.currstack = codegen.maxstack = fun->argc;
	codegen.code_size = 16;
	codegen.code  = calloc(codegen.code_size, sizeof(mathfun_code));
	codegen.error = error;

	if (!codegen.code) {
		mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
		mathfun_codegen_cleanup(&codegen);
		return false;
	}

	mathfun_code ret = fun->argc;
	if (!mathfun_codegen_expr(&codegen, expr, &ret) ||
		!mathfun_codegen_ins1(&codegen, RET, ret) ||
		!mathfun_codegen_ins0(&codegen, END)) {
		mathfun_codegen_cleanup(&codegen);
		return false;
	}

	if (codegen.maxstack >= MATHFUN_REGS_MAX) {
		mathfun_raise_error(error, MATHFUN_EXCEEDS_MAX_FRAME_SIZE);
		mathfun_codegen_cleanup(&codegen);
		return false;
	}

	// shortcut everything that just jumps to RET
	mathfun_code *ptr = codegen.code;

	while (*ptr != END) {
		switch (*ptr) {
		case JMP:
			if (!mathfun_code_shortcut_jmp_to_ret(codegen.code, ptr, NULL)) {
				mathfun_code_shortcut_jmp(codegen.code, ptr);
			}
			ptr += 2;
			break;

		case NOP:  ptr += 1; break;
		case MOV:
		case NEG:
		case NOT:  ptr += 3; break;
		case JMPF:
		case JMPT:
			mathfun_code_shortcut_jmptf(codegen.code, ptr, ptr[0], ptr[1]);
			ptr += 3;
			break;

		case VAL:  ptr += 2 + MATHFUN_VALUE_CODES; break;
		case CALL: ptr += 3 + MATHFUN_FUNCT_CODES; break;
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case MOD:
		case POW:
		case EQ:
		case NE:
		case LT:
		case GT:
		case LE:
		case GE:
		case BEQ:
		case BNE:  ptr += 4; break;
		case SETF:
		case SETT: ptr += 2; break;
		case RET:  ptr += 2; break;
		default:
			mathfun_raise_error(error, MATHFUN_INTERNAL_ERROR);
			mathfun_codegen_cleanup(&codegen);
			return false;
		}
	}

	fun->framesize = codegen.maxstack + 1;
	fun->code      = codegen.code;

	codegen.code = NULL;
	mathfun_codegen_cleanup(&codegen);

	return true;
}

#define MATHFUN_DUMP(ARGS) \
	if (fprintf ARGS < 0) { \
		mathfun_raise_error(error, MATHFUN_IO_ERROR); \
		return false; \
	}

bool mathfun_dump(const mathfun *fun, FILE *stream, const mathfun_context *ctx, mathfun_error_p *error) {
	const mathfun_code *start = fun->code;
	const mathfun_code *code  = start;

	MATHFUN_DUMP((stream, "argc = %"PRIzu", framesize = %"PRIzu"\n\n", fun->argc, fun->framesize));

	while (*code != END) {
		MATHFUN_DUMP((stream, "0x%08"PRIXPTR": ", code - start));
		switch (*code) {
			case NOP:
				MATHFUN_DUMP((stream, "nop\n"));
				++ code;
				break;

			case RET:
				MATHFUN_DUMP((stream, "ret %"PRIuPTR"\n", code[1]));
				code += 2;
				break;

			case MOV:
				MATHFUN_DUMP((stream, "mov %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2]));
				code += 3;
				break;

			case VAL:
				MATHFUN_DUMP((stream, "val %.22g, %"PRIuPTR"\n", *(double*)(code + 1),
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
				MATHFUN_DUMP((stream, "not %"PRIuPTR", %"PRIuPTR"\n", code[1], code[2]));
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

			case BEQ:
				MATHFUN_DUMP((stream, "beq %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case BNE:
				MATHFUN_DUMP((stream, "bne %"PRIuPTR", %"PRIuPTR", %"PRIuPTR"\n",
					code[1], code[2], code[3]));
				code += 4;
				break;

			case JMP:
				MATHFUN_DUMP((stream, "jmp 0x%"PRIXPTR"\n", code[1]));
				code += 2;
				break;

			case JMPT:
				MATHFUN_DUMP((stream, "jmpt %"PRIuPTR", 0x%"PRIXPTR"\n",
					code[1], code[2]));
				code += 3;
				break;

			case JMPF:
				MATHFUN_DUMP((stream, "jmpf %"PRIuPTR", 0x%"PRIXPTR"\n",
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

	return true;
}
