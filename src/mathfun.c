#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "mathfun_intern.h"

bool mathfun_context_init(mathfun_context *ctx, bool define_default, mathfun_error_p *error) {
	ctx->decl_capacity = 256;
	ctx->decl_used     =   0;

	ctx->decls = calloc(ctx->decl_capacity, sizeof(mathfun_decl));

	if (!ctx->decls) {
		ctx->decl_capacity = 0;
		mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
		return false;
	}

	if (define_default) {
		return mathfun_context_define_default(ctx, error);
	}

	return true;
}

void mathfun_context_cleanup(mathfun_context *ctx) {
	free(ctx->decls);

	ctx->decls = NULL;
	ctx->decl_capacity = 0;
	ctx->decl_used     = 0;
}

bool mathfun_context_grow(mathfun_context *ctx, mathfun_error_p *error) {
	const size_t size = ctx->decl_capacity * 2;
	mathfun_decl *decls = realloc(ctx->decls, size * sizeof(mathfun_decl));

	if (!decls) {
		mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
		return false;
	}

	ctx->decls         = decls;
	ctx->decl_capacity = size;
	return true;
}

const mathfun_decl *mathfun_context_get(const mathfun_context *ctx, const char *name) {
	for (size_t i = 0; i < ctx->decl_used; ++ i) {
		const mathfun_decl *decl = &ctx->decls[i];
		if (strcmp(decl->name, name) == 0) {
			return decl;
		}
	}

	return NULL;
}

const mathfun_decl *mathfun_context_getn(const mathfun_context *ctx, const char *name, size_t n) {
	for (size_t i = 0; i < ctx->decl_used; ++ i) {
		const mathfun_decl *decl = &ctx->decls[i];
		if (strncmp(decl->name, name, n) == 0 && !decl->name[n]) {
			return decl;
		}
	}

	return NULL;
}

const char *mathfun_context_funct_name(const mathfun_context *ctx, mathfun_binding_funct funct) {
	for (size_t i = 0; i < ctx->decl_used; ++ i) {
		const mathfun_decl *decl = &ctx->decls[i];
		if (decl->type == DECL_FUNCT && decl->decl.funct.funct == funct) {
			return decl->name;
		}
	}

	return NULL;
}

bool mathfun_valid_name(const char *name) {
	const char *ptr = name;
	if (!isalpha(*ptr) && *ptr != '_') {
		return false;
	}
	++ ptr;
	while (*ptr) {
		if (!isalnum(*ptr) && *ptr != '_') {
			return false;
		}
		++ ptr;
	}
	return
		strcasecmp(name, "inf")   != 0 &&
		strcasecmp(name, "nan")   != 0 &&
		strcasecmp(name, "true")  != 0 &&
		strcasecmp(name, "false") != 0 &&
		strcasecmp(name, "in")    != 0;
}

bool mathfun_validate_argnames(const char *argnames[], size_t argc, mathfun_error_p *error) {
	for (size_t i = 0; i < argc; ++ i) {
		const char *argname = argnames[i];
		if (!mathfun_valid_name(argname)) {
			mathfun_raise_name_error(error, MATHFUN_ILLEGAL_NAME, argname);
			return false;
		}
		for (size_t j = 0; j < i; ++ j) {
			if (strcmp(argname, argnames[j]) == 0) {
				mathfun_raise_name_error(error, MATHFUN_DUPLICATE_ARGUMENT, argname);
				return false;
			}
		}
	}
	return true;
}

bool mathfun_context_define_const(mathfun_context *ctx, const char *name, double value,
	mathfun_error_p *error) {
	if (!mathfun_valid_name(name)) {
		mathfun_raise_name_error(error, MATHFUN_ILLEGAL_NAME, name);
		return false;
	}

	if (mathfun_context_get(ctx, name)) {
		mathfun_raise_name_error(error, MATHFUN_NAME_EXISTS, name);
		return false;
	}

	if (ctx->decl_used == ctx->decl_capacity && !mathfun_context_grow(ctx, error)) {
		return false;
	}

	mathfun_decl *decl = ctx->decls + ctx->decl_used;
	decl->type = DECL_CONST;
	decl->name = name;
	decl->decl.value = value;

	++ ctx->decl_used;

	return true;
}

bool mathfun_context_define_funct(mathfun_context *ctx, const char *name, mathfun_binding_funct funct,
	const mathfun_sig *sig, mathfun_error_p *error) {
	if (!mathfun_valid_name(name)) {
		mathfun_raise_name_error(error, MATHFUN_ILLEGAL_NAME, name);
		return false;
	}

	if (sig->argc > MATHFUN_REGS_MAX) {
		mathfun_raise_error(error, MATHFUN_TOO_MANY_ARGUMENTS);
		return false;
	}

	if (mathfun_context_get(ctx, name)) {
		mathfun_raise_name_error(error, MATHFUN_NAME_EXISTS, name);
		return false;
	}

	if (ctx->decl_used == ctx->decl_capacity && !mathfun_context_grow(ctx, error)) {
		return false;
	}

	mathfun_decl *decl = ctx->decls + ctx->decl_used;
	decl->type = DECL_FUNCT;
	decl->name = name;
	decl->decl.funct.funct = funct;
	decl->decl.funct.sig   = sig;

	++ ctx->decl_used;

	return true;
}

bool mathfun_context_undefine(mathfun_context *ctx, const char *name, mathfun_error_p *error) {
	mathfun_decl *decl = (mathfun_decl*)mathfun_context_get(ctx, name);

	if (!decl) {
		mathfun_raise_name_error(error, MATHFUN_NO_SUCH_NAME, name);
		return false;
	}

	memmove(decl, decl + 1, (ctx->decl_used - (decl + 1 - ctx->decls)) * sizeof(mathfun_decl));

	++ ctx->decl_used;
	return true;
}

void mathfun_cleanup(mathfun *fun) {
	free(fun->code);
	fun->code = NULL;
}

double mathfun_call(const mathfun *fun, mathfun_error_p *error, ...) {
	va_list ap;
	va_start(ap, error);

	double value = mathfun_vcall(fun, ap, error);

	va_end(ap);

	return value;
}

double mathfun_acall(const mathfun *fun, const double args[], mathfun_error_p *error) {
	mathfun_reg *regs = calloc(fun->framesize, sizeof(mathfun_reg));

	if (!regs) {
		mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
		return NAN;
	}

	for (size_t i = 0; i < fun->argc; ++ i) {
		regs[i].number = args[i];
	}

	errno = 0;
	double value = mathfun_exec(fun, regs);
	free(regs);

	if (errno != 0) {
		mathfun_raise_c_error(error);
	}

	return value;
}

double mathfun_vcall(const mathfun *fun, va_list ap, mathfun_error_p *error) {
	mathfun_reg *regs = calloc(fun->framesize, sizeof(mathfun_reg));

	if (!regs) {
		mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
		return NAN;
	}

	for (size_t i = 0; i < fun->argc; ++ i) {
		regs[i].number = va_arg(ap, double);
	}

	errno = 0;
	double value = mathfun_exec(fun, regs);
	free(regs);

	if (errno != 0) {
		mathfun_raise_c_error(error);
	}

	return value;
}

double mathfun_run(const char *code, mathfun_error_p *error, ...) {
	va_list ap;

	va_start(ap, error);

	size_t argc = 0;
	while (va_arg(ap, const char *)) {
		++ argc;
	}

	va_end(ap);

	const char **argnames = NULL;
	double *args = NULL;

	if (argc > 0) {
		argnames = calloc(argc, sizeof(char*));

		if (!argnames) {
			mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
			return NAN;
		}

		args = calloc(argc, sizeof(double));

		if (!args) {
			mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
			free(argnames);
			return NAN;
		}
	}

	va_start(ap, error);

	for (size_t i = 0; i < argc; ++ i) {
		argnames[i] = va_arg(ap, const char *);
	}

	va_arg(ap, const char *);

	for (size_t i = 0; i < argc; ++ i) {
		args[i] = va_arg(ap, double);
	}

	va_end(ap);

	double value = mathfun_arun(argnames, argc, code, args, error);

	free(args);
	free(argnames);

	return value;
}

double mathfun_arun(const char *argnames[], size_t argc, const char *code, const double args[],
	mathfun_error_p *error) {
	mathfun_context ctx;

	if (!mathfun_validate_argnames(argnames, argc, error)) return NAN;

	if (!mathfun_context_init(&ctx, true, error)) return NAN;

	mathfun_expr *expr = mathfun_context_parse(&ctx, argnames, argc, code, error);

	if (!expr) {
		mathfun_context_cleanup(&ctx);
		return NAN;
	}

	// it's only executed once, so any optimizations and byte code
	// compilations would only add overhead
	errno = 0;
	double value = mathfun_expr_exec(expr, args).number;

	if (errno != 0) {
		mathfun_raise_c_error(error);
		mathfun_expr_free(expr);
		mathfun_context_cleanup(&ctx);
		return NAN;
	}

	mathfun_expr_free(expr);
	mathfun_context_cleanup(&ctx);

	return value;
}

bool mathfun_context_compile(const mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code,
	mathfun *fun, mathfun_error_p *error) {
	if (!mathfun_validate_argnames(argnames, argc, error)) return false;

	mathfun_expr *expr = mathfun_context_parse(ctx, argnames, argc, code, error);

	memset(fun, 0, sizeof(struct mathfun));
	if (!expr) return false;

	mathfun_expr *opt = mathfun_expr_optimize(expr, error);

	if (!opt) {
		// expr is freed by mathfun_expr_optimize on error
		return false;
	}

	fun->argc = argc;
	bool ok = mathfun_expr_codegen(opt, fun, error);

	// mathfun_expr_optimize reuses expr and frees discarded things,
	// so only opt has to be freed:
	mathfun_expr_free(opt);

	return ok;
}

bool mathfun_compile(mathfun *fun, const char *argnames[], size_t argc, const char *code,
	mathfun_error_p *error) {
	mathfun_context ctx;
	memset(fun, 0, sizeof(struct mathfun));
	if (!mathfun_context_init(&ctx, true, error)) return false;

	bool ok = mathfun_context_compile(&ctx, argnames, argc, code, fun, error);
	mathfun_context_cleanup(&ctx);

	return ok;
}

mathfun_expr *mathfun_expr_alloc(enum mathfun_expr_type type, mathfun_error_p *error) {
	mathfun_expr *expr = calloc(1, sizeof(mathfun_expr));

	if (!expr) {
		mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
		return NULL;
	}

	expr->type = type;

	return expr;
}

void mathfun_expr_free(mathfun_expr *expr) {
	if (!expr) return;

	switch (expr->type) {
		case EX_CONST:
		case EX_ARG:
			break;

		case EX_CALL:
			if (expr->ex.funct.args) {
				const size_t argc = expr->ex.funct.sig->argc;
				for (size_t i = 0; i < argc; ++ i) {
					mathfun_expr_free(expr->ex.funct.args[i]);
				}
				free(expr->ex.funct.args);
				expr->ex.funct.args = NULL;
			}
			break;

		case EX_NEG:
		case EX_NOT:
			mathfun_expr_free(expr->ex.unary.expr);
			expr->ex.unary.expr = NULL;
			break;

		case EX_ADD:
		case EX_SUB:
		case EX_MUL:
		case EX_DIV:
		case EX_MOD:
		case EX_POW:
		case EX_EQ:
		case EX_NE:
		case EX_LT:
		case EX_GT:
		case EX_LE:
		case EX_GE:
		case EX_BEQ:
		case EX_BNE:
		case EX_AND:
		case EX_OR:
		case EX_IN:
		case EX_RNG_INCL:
		case EX_RNG_EXCL:
			mathfun_expr_free(expr->ex.binary.left);
			mathfun_expr_free(expr->ex.binary.right);
			expr->ex.binary.left  = NULL;
			expr->ex.binary.right = NULL;
			break;

		case EX_IIF:
			mathfun_expr_free(expr->ex.iif.cond);
			mathfun_expr_free(expr->ex.iif.then_expr);
			mathfun_expr_free(expr->ex.iif.else_expr);
			expr->ex.iif.cond      = NULL;
			expr->ex.iif.then_expr = NULL;
			expr->ex.iif.else_expr = NULL;
			break;
	}
	free(expr);
}

mathfun_type mathfun_expr_type(const mathfun_expr *expr) {
	switch (expr->type) {
		case EX_CONST:
			return expr->ex.value.type;

		case EX_CALL:
			return expr->ex.funct.sig->rettype;

		case EX_IIF:
			return mathfun_expr_type(expr->ex.iif.then_expr);

		case EX_ARG:
		case EX_NEG:
		case EX_ADD:
		case EX_SUB:
		case EX_MUL:
		case EX_DIV:
		case EX_MOD:
		case EX_POW:
			return MATHFUN_NUMBER;

		case EX_NOT:
		case EX_EQ:
		case EX_NE:
		case EX_LT:
		case EX_GT:
		case EX_LE:
		case EX_GE:
		case EX_BEQ:
		case EX_BNE:
		case EX_AND:
		case EX_OR:
		case EX_IN:
			return MATHFUN_BOOLEAN;

		case EX_RNG_INCL:
		case EX_RNG_EXCL:
			return -1;

		default:
			assert(false);
			return -1;
	}
}

const char *mathfun_type_name(mathfun_type type) {
	switch (type) {
		case MATHFUN_NUMBER:  return "number";
		case MATHFUN_BOOLEAN: return "boolean";
		default:              fprintf(stderr, "illegal type: %d\n", type);
		
		return NULL;
	}
}

double mathfun_mod(double x, double y) {
	if (y == 0.0) {
		errno = EDOM;
		return NAN;
	}
	double mod = fmod(x, y);
	if (mod) {
		if ((y < 0.0) != (mod < 0.0)) {
			mod += y;
		}
	}
	else {
		mod = copysign(0.0, y);
	}
	return mod;
}
