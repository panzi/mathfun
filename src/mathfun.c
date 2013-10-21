#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include "mathfun_intern.h"

bool mathfun_context_init(struct mathfun_context *ctx, bool define_default, mathfun_error_info *error) {
	ctx->decl_capacity = 256;
	ctx->decl_used     =   0;

	ctx->decls = calloc(ctx->decl_capacity, sizeof(struct mathfun_decl));

	if (!ctx->decls) {
		ctx->decl_capacity = 0;
		mathfun_raise_error(error, MATHFUN_MEMORY_ERROR);
		return false;
	}

	if (define_default) {
		return mathfun_context_define_default(ctx, error);
	}

	return true;
}

void mathfun_context_cleanup(struct mathfun_context *ctx) {
	free(ctx->decls);

	ctx->decls = NULL;
	ctx->decl_capacity = 0;
	ctx->decl_used     = 0;
}

bool mathfun_context_grow(struct mathfun_context *ctx, mathfun_error_info *error) {
	const size_t size = ctx->decl_capacity * 2;
	struct mathfun_decl *decls = realloc(ctx->decls, size * sizeof(struct mathfun_decl));

	if (!decls) {
		mathfun_raise_error(error, MATHFUN_MEMORY_ERROR);
		return false;
	}

	ctx->decls         = decls;
	ctx->decl_capacity = size;
	return true;
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

const struct mathfun_decl *mathfun_context_getn(const struct mathfun_context *ctx, const char *name, size_t n) {
	for (size_t i = 0; i < ctx->decl_used; ++ i) {
		const struct mathfun_decl *decl = &ctx->decls[i];
		if (strncmp(decl->name, name, n) == 0 && !decl->name[n]) {
			return decl;
		}
	}

	return NULL;
}

const char *mathfun_context_funct_name(const struct mathfun_context *ctx, mathfun_binding_funct funct) {
	for (size_t i = 0; i < ctx->decl_used; ++ i) {
		const struct mathfun_decl *decl = &ctx->decls[i];
		if (decl->type == DECL_FUNCT && decl->decl.funct.funct == funct) {
			return decl->name;
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
	return strcasecmp(name,"inf") != 0 && strcasecmp(name,"nan") != 0;
}

bool mathfun_validate_argnames(const char *argnames[], size_t argc, mathfun_error_info *error) {
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

bool mathfun_context_define_const(struct mathfun_context *ctx, const char *name, mathfun_value value,
	mathfun_error_info *error) {
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

	struct mathfun_decl *decl = ctx->decls + ctx->decl_used;
	decl->type = DECL_CONST;
	decl->name = name;
	decl->decl.value = value;

	++ ctx->decl_used;

	return true;
}

bool mathfun_context_define_funct(struct mathfun_context *ctx, const char *name, mathfun_binding_funct funct, size_t argc,
	mathfun_error_info *error) {
	if (!mathfun_valid_name(name)) {
		mathfun_raise_name_error(error, MATHFUN_ILLEGAL_NAME, name);
		return false;
	}

	if (argc > MATHFUN_REGS_MAX) {
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

	struct mathfun_decl *decl = ctx->decls + ctx->decl_used;
	decl->type = DECL_FUNCT;
	decl->name = name;
	decl->decl.funct.funct = funct;
	decl->decl.funct.argc  = argc;

	++ ctx->decl_used;

	return true;
}

bool mathfun_context_undefine(struct mathfun_context *ctx, const char *name, mathfun_error_info *error) {
	struct mathfun_decl *decl = (struct mathfun_decl*)mathfun_context_get(ctx, name);

	if (!decl) {
		mathfun_raise_name_error(error, MATHFUN_NO_SUCH_NAME, name);
		return false;
	}

	memmove(decl, decl + 1, (ctx->decl_used - (decl + 1 - ctx->decls)) * sizeof(struct mathfun_decl));

	++ ctx->decl_used;
	return true;
}

void mathfun_cleanup(struct mathfun *mathfun) {
	free(mathfun->code);
	mathfun->code = NULL;
}

mathfun_value mathfun_call(const struct mathfun *mathfun, mathfun_error_info *error, ...) {
	va_list ap;
	va_start(ap, error);

	mathfun_value value = mathfun_vcall(mathfun, ap, error);

	va_end(ap);

	return value;
}

mathfun_value mathfun_acall(const struct mathfun *mathfun, const mathfun_value args[], mathfun_error_info *error) {
	mathfun_value *regs = calloc(mathfun->framesize, sizeof(mathfun_value));

	if (!regs) {
		mathfun_raise_error(error, MATHFUN_MEMORY_ERROR);
		return NAN;
	}

	memcpy(regs, args, mathfun->argc * sizeof(mathfun_value));

	errno = 0;
	mathfun_value value = mathfun_exec(mathfun, regs);
	free(regs);

	if (errno != 0) {
		mathfun_raise_c_error(error);
	}

	return value;
}

mathfun_value mathfun_vcall(const struct mathfun *mathfun, va_list ap, mathfun_error_info *error) {
	mathfun_value *regs = calloc(mathfun->framesize, sizeof(mathfun_value));

	if (!regs) {
		mathfun_raise_error(error, MATHFUN_MEMORY_ERROR);
		return NAN;
	}

	for (size_t i = 0; i < mathfun->argc; ++ i) {
		regs[i] = va_arg(ap, mathfun_value);
	}

	errno = 0;
	mathfun_value value = mathfun_exec(mathfun, regs);
	free(regs);

	if (errno != 0) {
		mathfun_raise_c_error(error);
	}

	return value;
}

// usage:
// result = matfun_run(expr, errorptr, argnames..., NULL, args...);
//
// struct mathfun_error *error = NULL;
// mathfun_value value = mathfun_run("sin(x) * y", &error, "x", "y", NULL, 1.4, 2.5);
//
// or
// 
// mathfun_value value = mathfun_run("sin(x) * y", NULL, "x", "y", NULL, 1.4, 2.5);
mathfun_value mathfun_run(const char *code, mathfun_error_info *error, ...) {
	va_list ap;
	size_t argcap = 32;
	const char **argnames = calloc(argcap, sizeof(char*));
	size_t argc = 0;

	if (!argnames) {
		mathfun_raise_error(error, MATHFUN_MEMORY_ERROR);
		return NAN;
	}

	va_start(ap, error);

	for (;;) {
		const char *argname = va_arg(ap, const char *);

		if (!argname) break;

		if (argc == argcap) {
			size_t size = argcap * 2;
			const char **args = realloc(argnames, size * sizeof(char*));

			if (!args) {
				mathfun_raise_error(error, MATHFUN_MEMORY_ERROR);
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
		mathfun_raise_error(error, MATHFUN_MEMORY_ERROR);
		free(argnames);
		return NAN;
	}

	for (size_t i = 0; i < argc; ++ i) {
		args[i] = va_arg(ap, mathfun_value);
	}

	va_end(ap);

	mathfun_value value = mathfun_arun(argnames, argc, code, args, error);

	free(args);
	free(argnames);

	return value;
}

mathfun_value mathfun_arun(const char *argnames[], size_t argc, const char *code, const mathfun_value args[],
	mathfun_error_info *error) {
	struct mathfun_context ctx;

	if (!mathfun_context_init(&ctx, true, error)) return NAN;

	struct mathfun_expr *expr = mathfun_context_parse(&ctx, argnames, argc, code, error);

	if (!expr) {
		mathfun_context_cleanup(&ctx);
		return NAN;
	}

	// it's only executed once, so any optimizations and byte code
	// compilations would only add overhead
	mathfun_value value = mathfun_expr_exec(expr, args, error);

	mathfun_expr_free(expr);
	mathfun_context_cleanup(&ctx);

	return value;
}

bool mathfun_context_compile(const struct mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code,
	struct mathfun *mathfun, mathfun_error_info *error) {
	if (!mathfun_validate_argnames(argnames, argc, error)) return false;

	struct mathfun_expr *expr = mathfun_context_parse(ctx, argnames, argc, code, error);

	memset(mathfun, 0, sizeof(struct mathfun));
	if (!expr) return false;

	struct mathfun_expr *opt = mathfun_expr_optimize(expr, error);

	if (!opt) {
		// expr is freed by mathfun_expr_optimize on error
		return false;
	}

	mathfun->argc = argc;
	bool ok = mathfun_expr_codegen(opt, mathfun, error);

	// mathfun_expr_optimize reuses expr and frees discarded things,
	// so only opt has to be freed:
	mathfun_expr_free(opt);

	return ok;
}

bool mathfun_compile(struct mathfun *mathfun, const char *argnames[], size_t argc, const char *code,
	mathfun_error_info *error) {
	struct mathfun_context ctx;
	if (!mathfun_context_init(&ctx, true, error)) return false;

	bool ok = mathfun_context_compile(&ctx, argnames, argc, code, mathfun, error);
	mathfun_context_cleanup(&ctx);

	return ok;
}

struct mathfun_expr *mathfun_expr_alloc(enum mathfun_expr_type type, mathfun_error_info *error) {
	struct mathfun_expr *expr = calloc(1, sizeof(struct mathfun_expr));

	if (!expr) {
		mathfun_raise_error(error, MATHFUN_MEMORY_ERROR);
		return NULL;
	}

	expr->type = type;

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
	mathfun_value mathfun_mod_result;
	if (y == 0.0) {
		errno = EDOM;
		return NAN;
	}
	MATHFUN_MOD(x, y);
	return mathfun_mod_result;
}
