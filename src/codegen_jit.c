#include "codegen_jit.h"

// TODO: error handling
bool mathfun_jit_expr(mathfun_jit *jit, mathfun_expr *expr) {
	switch (expr->type) {
		case EX_CONST:
		case EX_ARG:
		case EX_CALL:
		case EX_NEG:
		case EX_ADD:
		case EX_SUB:
		case EX_MUL:
		case EX_DIV:
		case EX_MOD:
		case EX_POW:
		case EX_NOT:
		case EX_EQ:
		case EX_NE:
		case EX_LT:
		case EX_GT:
		case EX_LE:
		case EX_GE:
		case EX_IN:
			break;
		
		case EX_RNG_INCL:
		case EX_RNG_EXCL:
			mathfun_raise_error(jit->error, MATHFUN_INTERNAL_ERROR);
			return false;
			
		case EX_BEQ:
		case EX_BNE:
		case EX_AND:
		case EX_OR:
		case EX_IIF:
			break;
	}

	mathfun_raise_error(jit->error, MATHFUN_INTERNAL_ERROR);
	return false;
}

bool mathfun_expr_codegen(mathfun_expr *expr, mathfun *fun, mathfun_error_p *error) {
	jit_context_t context;
	jit_type_t *params;
	jit_type_t signature;
	mathfun_jit jit;
	mathfun_jit_code *code;

	jit.argc  = fun->argc;
	jit.funct = NULL;
	jit.error = error;

	context = jit_context_create();

	jit_context_build_start(context);

	code = malloc(sizeof(mathfun_jit_code));
	params = calloc(fun->argc, sizeof(jit_type_t));

	for (size_t i = 0; i < fun->argc; ++ i) {
		params[i] = jit_type_sys_double;
	}

	signature = jit_type_create_signature(jit_abi_cdecl, jit_type_sys_double, params, fun->argc, 1);

	jit.funct = jit_function_create(context, signature);

	mathfun_jit_expr(&jit, expr);

	jit_context_build_end(context);

	code->context = context;
	code->funct   = jit.funct;

	fun->framesize = fun->argc + fun->argc * (1 + (sizeof(void *) - 1) / sizeof(mathfun_value));
	fun->code      = code;

	return true;
}

bool mathfun_dump(const mathfun *fun, FILE *stream, const mathfun_context *ctx, mathfun_error_p *error) {
	(void)fun;
	(void)ctx;
	fprintf(stream, "mathfun_dump not yet implemented when using libjit\n");
	mathfun_raise_error(error, MATHFUN_INTERNAL_ERROR);
	return false;
}
