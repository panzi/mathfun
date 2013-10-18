#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include "mathfun_intern.h"

int mathfun_context_init(struct mathfun_context *ctx, bool define_default) {
	ctx->decl_capacity = 256;
	ctx->decl_used     =   0;

	ctx->decls = calloc(ctx->decl_capacity, sizeof(struct mathfun_decl));

	if (!ctx->decls) {
		ctx->decl_capacity = 0;
		return ENOMEM;
	}

	if (define_default) {
		return mathfun_context_define_default(ctx);
	}

	return 0;
}

void mathfun_context_cleanup(struct mathfun_context *ctx) {
	free(ctx->decls);

	ctx->decls = NULL;
	ctx->decl_capacity = 0;
	ctx->decl_used     = 0;
}

int mathfun_context_grow(struct mathfun_context *ctx) {
	const size_t size = ctx->decl_capacity * 2;
	struct mathfun_decl *decls = realloc(ctx->decls, size * sizeof(struct mathfun_decl));

	if (!decls) return ENOMEM;

	ctx->decls         = decls;
	ctx->decl_capacity = size;
	return 0;
}

const struct mathfun_decl *mathfun_context_get(const struct mathfun_context *ctx, const char *name) {
	for (size_t i = 0; i < ctx->decl_used; ++ i) {
		const struct mathfun_decl *decl = &ctx->decls[i];
		if (strcmp(decl->name, name) == 0) {
			return decl;
		}
	}

	return NULL;
}

bool mathfun_valid_name(const char *name) {
	if (!isalpha(*name) && *name != '_') {
		return false;
	}
	++ name;
	while (*name) {
		if (!isalnum(*name) && *name != '_') {
			return false;
		}
		++ name;
	}
	return true;
}

int mathfun_context_define_const(struct mathfun_context *ctx, const char *name, mathfun_value value) {
	if (!mathfun_valid_name(name)) return errno = EINVAL;
	if (mathfun_context_get(ctx, name)) return errno = EINVAL;

	if (ctx->decl_used == ctx->decl_capacity) {
		int errnum = mathfun_context_grow(ctx);
		if (errnum != 0) return errnum;
	}

	struct mathfun_decl *decl = ctx->decls + ctx->decl_used;
	decl->type = DECL_CONST;
	decl->name = name;
	decl->decl.value = value;

	++ ctx->decl_used;

	return 0;
}

int mathfun_context_define_funct(struct mathfun_context *ctx, const char *name, mathfun_binding_funct funct, size_t argc) {
	if (!mathfun_valid_name(name)) return errno = EINVAL;
	if (argc > MATHFUN_REGS_MAX) return errno = ERANGE;
	if (mathfun_context_get(ctx, name)) return errno = EINVAL;

	if (ctx->decl_used == ctx->decl_capacity) {
		int errnum = mathfun_context_grow(ctx);
		if (errnum != 0) return errnum;
	}

	struct mathfun_decl *decl = ctx->decls + ctx->decl_used;
	decl->type = DECL_FUNCT;
	decl->name = name;
	decl->decl.funct.funct = funct;
	decl->decl.funct.argc  = argc;

	++ ctx->decl_used;

	return 0;
}

int mathfun_context_undefine(struct mathfun_context *ctx, const char *name) {
	struct mathfun_decl *decl = (struct mathfun_decl*)mathfun_context_get(ctx, name);

	if (!decl) return errno = EINVAL;

	memmove(decl, decl + 1, (ctx->decl_used - (decl + 1 - ctx->decls)) * sizeof(struct mathfun_decl));

	++ ctx->decl_used;
	return 0;
}

void mathfun_cleanup(struct mathfun *mathfun) {
	free(mathfun->code);
	mathfun->code = NULL;
}

mathfun_value mathfun_call(const struct mathfun *mathfun, ...) {
	va_list ap;
	va_start(ap, mathfun);

	mathfun_value value = mathfun_vcall(mathfun, ap);

	va_end(ap);

	return value;
}

mathfun_value mathfun_acall(const struct mathfun *mathfun, const mathfun_value args[]) {
	mathfun_value *regs = calloc(mathfun->framesize, sizeof(mathfun_value));
	memcpy(regs, args, mathfun->argc * sizeof(mathfun_value));

	mathfun_value value = mathfun_exec(mathfun, regs);

	free(regs);

	return value;
}

mathfun_value mathfun_vcall(const struct mathfun *mathfun, va_list ap) {
	mathfun_value *regs = calloc(mathfun->framesize, sizeof(mathfun_value));

	for (size_t i = 0; i < mathfun->argc; ++ i) {
		regs[i] = va_arg(ap, mathfun_value);
	}

	mathfun_value value = mathfun_exec(mathfun, regs);

	free(regs);

	return value;
}

// usage:
// mathfun_value value = mathfun_run("sin(x) * y", "x", "y", NULL, 1.4, 2.5);
mathfun_value mathfun_run(const char *code, ...) {
	va_list ap;
	size_t argcap = 32;
	const char **argnames = calloc(argcap, sizeof(char*));
	size_t argc = 0;

	if (!argnames) return NAN;

	va_start(ap, code);

	for (;;) {
		const char *argname = va_arg(ap, const char *);

		if (!argname) break;

		if (argc == argcap) {
			size_t size = argcap * 2;
			const char **args = realloc(argnames, size * sizeof(char*));

			if (!args) {
				free(argnames);
				return NAN;
			}

			argnames = args;
			argcap   = size;
		}

		argnames[argc] = argname;
		++ argc;
	}
	
	mathfun_value *args = malloc(argc * sizeof(mathfun_value));

	if (!args) {
		free(argnames);
		return NAN;
	}

	for (size_t i = 0; i < argc; ++ i) {
		args[i] = va_arg(ap, mathfun_value);
	}

	va_end(ap);

	mathfun_value value = mathfun_arun(argnames, argc, code, args);

	free(args);
	free(argnames);

	return value;
}

mathfun_value mathfun_arun(const char *argnames[], size_t argc, const char *code, const mathfun_value args[]) {
	struct mathfun_context ctx;

	int errnum = mathfun_context_init(&ctx, true);
	if (errnum != 0) return NAN;

	struct mathfun_expr *expr = mathfun_context_parse(&ctx, argnames, argc, code);

	if (!expr) {
		mathfun_context_cleanup(&ctx);
		return NAN;
	}

	// it's only executed once, so any optimizations and byte code
	// compilations would only add overhead
	mathfun_value value = mathfun_expr_exec(expr, args);

	mathfun_context_cleanup(&ctx);

	return value;
}

int mathfun_context_compile(const struct mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code,
	struct mathfun *mathfun) {
	errno = 0;
	struct mathfun_expr *expr = mathfun_context_parse(ctx, argnames, argc, code);

	memset(mathfun, 0, sizeof(struct mathfun));
	if (!expr) return errno ? errno : EINVAL;

	struct mathfun_expr *opt = mathfun_expr_optimize(expr);

	if (!opt) {
		mathfun_expr_free(expr);
		return errno ? errno : EINVAL;
	}

	mathfun->argc = argc;
	int errnum = mathfun_expr_codegen(opt, mathfun);

	// mathfun_expr_optimize reuses expr and frees discarded things,
	// so only opt has to be freed:
	mathfun_expr_free(opt);

	return errnum;
}

int mathfun_compile(struct mathfun *mathfun, const char *argnames[], size_t argc, const char *code) {
	struct mathfun_context ctx;
	int errnum = mathfun_context_init(&ctx, true);
	if (errnum != 0) return errnum;

	errnum = mathfun_context_compile(&ctx, argnames, argc, code, mathfun);
	mathfun_context_cleanup(&ctx);

	return errnum;
}

struct mathfun_expr *mathfun_expr_alloc(enum mathfun_expr_type type) {
	struct mathfun_expr *expr = malloc(sizeof(struct mathfun_expr));

	if (!expr) return NULL;

	expr->type = type;

	switch (type) {
		case EX_CONST: expr->ex.value = 0; break;
		case EX_ARG:   expr->ex.arg   = 0; break;
		case EX_CALL:  expr->ex.funct.funct = NULL;
		               expr->ex.funct.args  = NULL;
		               expr->ex.funct.argc  = 0;    break;
		case EX_NEG:   expr->ex.unary.expr  = NULL; break;
		case EX_ADD:
		case EX_SUB:
		case EX_MUL:
		case EX_DIV:
		case EX_MOD:
		case EX_POW:
			expr->ex.binary.left  = NULL;
			expr->ex.binary.right = NULL;
			break;
	}

	return expr;
}

void mathfun_expr_free(struct mathfun_expr *expr) {
	if (!expr) return;

	switch (expr->type) {
		case EX_CONST:
		case EX_ARG:
			break;

		case EX_CALL:
			if (expr->ex.funct.args) {
				for (size_t i = 0; i < expr->ex.funct.argc; ++ i) {
					mathfun_expr_free(expr->ex.funct.args[i]);
				}
				free(expr->ex.funct.args);
				expr->ex.funct.args = NULL;
			}
			break;

		case EX_NEG:
			mathfun_expr_free(expr->ex.unary.expr);
			expr->ex.unary.expr = NULL;
			break;

		case EX_ADD:
		case EX_SUB:
		case EX_MUL:
		case EX_DIV:
		case EX_MOD:
		case EX_POW:
			mathfun_expr_free(expr->ex.binary.left);
			mathfun_expr_free(expr->ex.binary.right);
			expr->ex.binary.left  = NULL;
			expr->ex.binary.right = NULL;
			break;
	}
	free(expr);
}

mathfun_value mathfun_mod(mathfun_value x, mathfun_value y) {
	MATHFUN_MOD(x, y);
	return mathfun_mod_result;
}
