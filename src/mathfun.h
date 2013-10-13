#ifndef MATHFUN_H__
#define MATHFUN_H__

#include <stdarg.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double mathfun_value;
typedef mathfun_value (*mathfun_binding_funct)(const mathfun_value *args);

struct mathfun_context;
struct mathfun_decl;
struct mathfun;

int mathfun_context_init(struct mathfun_context *ctx, bool define_default);
void mathfun_context_cleanup(struct mathfun_context *ctx);
int mathfun_context_define_default(struct mathfun_context *ctx);
int mathfun_context_define_const(struct mathfun_context *ctx, const char *name, mathfun_value value);
int mathfun_context_define_funct(struct mathfun_context *ctx, const char *name, mathfun_binding_funct funct, size_t argc);
int mathfun_context_undefine(struct mathfun_context *ctx, const char *name);
int mathfun_context_compile(const struct mathfun_context *ctx,
	const char *argnames[], size_t argc, const char *code,
	struct mathfun *mathfun);

void mathfun_cleanup(struct mathfun *mathfun);
mathfun_value mathfun_call(struct mathfun *mathfun, ...);
mathfun_value mathfun_acall(struct mathfun *mathfun, const mathfun_value args[]);
mathfun_value mathfun_vcall(struct mathfun *mathfun, va_list ap);

mathfun_value mathfun_run(const char *code, ...);

#ifdef __cplusplus
}
#endif

#endif
