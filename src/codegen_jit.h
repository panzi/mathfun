#ifndef MATHFUN_CODEGEN_JIT_H__
#define MATHFUN_CODEGEN_JIT_H__

#pragma once

#include "mathfun_intern.h"

#include <jit/jit.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mathfun_jit mathfun_jit;
typedef struct mathfun_jit_code mathfun_jit_code;

struct mathfun_jit {
	size_t           argc;
	jit_function_t   funct;
	jit_type_t       mathfun_value_fields[2];
	jit_type_t       mathfun_value;
	jit_type_t       mathfun_binding_funct_t;
	jit_type_t       mathfun_binding_funct_params[1];
};

struct mathfun_jit_code {
	jit_context_t  context;
	jit_function_t funct;
};

MATHFUN_LOCAL jit_value_t mathfun_jit_expr(mathfun_jit *jit, mathfun_expr *expr);

#ifdef __cplusplus
}
#endif

#endif
