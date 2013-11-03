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

bool mathfun_context_ensure(mathfun_context *ctx, size_t n, mathfun_error_p *error) {
	size_t size = ctx->decl_capacity + n;
	size_t rem  = size % 256;
	if (rem) size += 256 - rem;
	mathfun_decl *decls = realloc(ctx->decls, size * sizeof(mathfun_decl));

	if (!decls) {
		mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
		return false;
	}

	ctx->decls         = decls;
	ctx->decl_capacity = size;
	return true;
}

// first string is NUL terminated, second string has a defined length
static int strn2cmp(const char *s1, const char *s2, size_t n2) {
	const char *s2end = s2 + n2;
	for (; *s1 == (s2 == s2end ? 0 : *s2); ++ s1, ++ s2) {
		if (*s1 == 0) {
			return 0;
		}
	}
	return (*(const unsigned char *)s1 < (s2 == s2end ? 0 : *(const unsigned char *)s2)) ? -1 : +1;
}

static bool mathfun_context_find(const mathfun_context *ctx, const char *name, size_t n, size_t *index) {
	// binary search
	// exclusive range: [lower, upper)
	size_t lower = 0;
	size_t upper = ctx->decl_used;
	const mathfun_decl *decls = ctx->decls;

	while (lower < upper) {
		const size_t mid = lower + (upper - lower) / 2;
		const mathfun_decl *decl = decls + mid;
		int cmp = strn2cmp(decl->name, name, n);

		if (cmp < 0) {
			lower = mid + 1;
		}
		else if (cmp > 0) {
			upper = mid;
		}
		else {
			*index = mid;
			return true;
		}
	}
	
	*index = lower;
	return false;
}

const mathfun_decl *mathfun_context_get(const mathfun_context *ctx, const char *name) {
	size_t index = 0;
	if (mathfun_context_find(ctx, name, strlen(name), &index)) {
		return ctx->decls + index;
	}
	return NULL;
}

const mathfun_decl *mathfun_context_getn(const mathfun_context *ctx, const char *name, size_t n) {
	size_t index = 0;
	if (mathfun_context_find(ctx, name, n, &index)) {
		return ctx->decls + index;
	}
	return NULL;
}

const char *mathfun_context_funct_name(const mathfun_context *ctx, mathfun_binding_funct funct) {
	for (size_t i = 0; i < ctx->decl_used; ++ i) {
		const mathfun_decl *decl = &ctx->decls[i];
		if (decl->type == MATHFUN_DECL_FUNCT && decl->decl.funct.funct == funct) {
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

static int mathfun_decl_cmp(const void *a, const void *b) {
	return strcmp(((const mathfun_decl*)a)->name, ((const mathfun_decl*)b)->name);
}

bool mathfun_context_define(mathfun_context *ctx, const mathfun_decl decls[], mathfun_error_p *error) {
	size_t new_count = 0;
	const mathfun_decl *ptr = decls;
	while (ptr->name) {
		if (mathfun_context_get(ctx, ptr->name)) {
			mathfun_raise_name_error(error, MATHFUN_NAME_EXISTS, ptr->name);
			return false;
		}
		++ ptr;
		++ new_count;
	}
	size_t old_count = ctx->decl_used;
	size_t size = old_count + new_count;
	size_t rem  = size % 256;
	if (rem) size += 256 - rem;

	mathfun_decl *merged = calloc(size, sizeof(mathfun_decl));

	if (!merged) {
		mathfun_raise_error(error, MATHFUN_OUT_OF_MEMORY);
		return false;
	}

	mathfun_decl *old_decls = ctx->decls;
	mathfun_decl *new_decls = merged + old_count;

	// copy new list to new buffer where I can sort it
	memcpy(new_decls, decls, new_count * sizeof(mathfun_decl));
	qsort(new_decls, new_count, sizeof(mathfun_decl), mathfun_decl_cmp);

	// merge old and new lists
	size_t i = 0, j = 0, index = 0;
	while (i < old_count && j < new_count) {
		int cmp = strcmp(old_decls[i].name, new_decls[j].name);
		if (cmp < 0) {
			merged[index] = old_decls[i ++];
		}
		else if (cmp > 0) {
			merged[index] = new_decls[j ++];
		}
		else {
			mathfun_raise_name_error(error, MATHFUN_NAME_EXISTS, old_decls[i].name);
			free(merged);
			return false;
		}

		if (index > 0 && strcmp(merged[index].name, merged[index - 1].name) == 0) {
			mathfun_raise_name_error(error, MATHFUN_NAME_EXISTS, merged[index].name);
			free(merged);
			return false;
		}

		++ index;
	}

	// add rest of old list
	while (i < old_count) {
		merged[index] = old_decls[i];

		if (index > 0 && strcmp(merged[index].name, merged[index - 1].name) == 0) {
			mathfun_raise_name_error(error, MATHFUN_NAME_EXISTS, merged[index].name);
			free(merged);
			return false;
		}

		++ i;
		++ index;
	}

	// new list should be in place, just validate uniquness of names
	assert(merged + index == new_decls + j);
	while (j < new_count) {
		if (index > 0 && strcmp(merged[index].name, merged[index - 1].name) == 0) {
			mathfun_raise_name_error(error, MATHFUN_NAME_EXISTS, merged[index].name);
			free(merged);
			return false;
		}

		++ j;
		++ index;
	}

	// use merged lists
	free(ctx->decls);
	ctx->decls         = merged;
	ctx->decl_used     = old_count + new_count;
	ctx->decl_capacity = size;

	return true;
}

bool mathfun_context_define_const(mathfun_context *ctx, const char *name, double value,
	mathfun_error_p *error) {
	if (!mathfun_valid_name(name)) {
		mathfun_raise_name_error(error, MATHFUN_ILLEGAL_NAME, name);
		return false;
	}

	size_t index = 0;
	if (mathfun_context_find(ctx, name, strlen(name), &index)) {
		mathfun_raise_name_error(error, MATHFUN_NAME_EXISTS, name);
		return false;
	}

	if (ctx->decl_used == ctx->decl_capacity && !mathfun_context_ensure(ctx, 1, error)) {
		return false;
	}

	memmove(ctx->decls + index + 1, ctx->decls + index, (ctx->decl_used - index) * sizeof(mathfun_decl));

	mathfun_decl *decl = ctx->decls + index;
	decl->type = MATHFUN_DECL_CONST;
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

	size_t index = 0;
	if (mathfun_context_find(ctx, name, strlen(name), &index)) {
		mathfun_raise_name_error(error, MATHFUN_NAME_EXISTS, name);
		return false;
	}

	if (ctx->decl_used == ctx->decl_capacity && !mathfun_context_ensure(ctx, 1, error)) {
		return false;
	}

	memmove(ctx->decls + index + 1, ctx->decls + index, (ctx->decl_used - index) * sizeof(mathfun_decl));

	mathfun_decl *decl = ctx->decls + index;
	decl->type = MATHFUN_DECL_FUNCT;
	decl->name = name;
	decl->decl.funct.funct = funct;
	decl->decl.funct.sig   = sig;

	++ ctx->decl_used;

	return true;
}

bool mathfun_context_undefine(mathfun_context *ctx, const char *name, mathfun_error_p *error) {
	size_t index = 0;
	if (!mathfun_context_find(ctx, name, strlen(name), &index)) {
		mathfun_raise_name_error(error, MATHFUN_NO_SUCH_NAME, name);
		return false;
	}

	memmove(ctx->decls + index, ctx->decls + index + 1, (ctx->decl_used - index - 1) * sizeof(mathfun_decl));

	-- ctx->decl_used;
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
	mathfun_value *regs = calloc(fun->framesize, sizeof(mathfun_value));

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
	mathfun_value *regs = calloc(fun->framesize, sizeof(mathfun_value));

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
		va_arg(ap, double);
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
		args[i]     = va_arg(ap, double);
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
