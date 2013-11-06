#ifndef MATHFUN_CODEGEN_H__
#define MATHFUN_CODEGEN_H__

#pragma once

#include "mathfun_intern.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mathfun_codegen mathfun_codegen;

struct mathfun_codegen {
	size_t argc;
	size_t maxstack;
	size_t currstack;
	size_t code_size;
	size_t code_used;
	mathfun_code *code;
	mathfun_error_p *error;
};

MATHFUN_LOCAL bool mathfun_codegen_expr(mathfun_codegen *codegen, mathfun_expr *expr, mathfun_code *ret);

MATHFUN_LOCAL bool mathfun_codegen_val(mathfun_codegen *codegen, mathfun_value value, mathfun_code target);
MATHFUN_LOCAL bool mathfun_codegen_call(mathfun_codegen *codegen, mathfun_binding_funct funct, mathfun_code firstarg, mathfun_code target);

MATHFUN_LOCAL bool mathfun_codegen_ins0(mathfun_codegen *codegen, enum mathfun_bytecode code);
MATHFUN_LOCAL bool mathfun_codegen_ins1(mathfun_codegen *codegen, enum mathfun_bytecode code, mathfun_code arg1);
MATHFUN_LOCAL bool mathfun_codegen_ins2(mathfun_codegen *codegen, enum mathfun_bytecode code, mathfun_code arg1, mathfun_code arg2);
MATHFUN_LOCAL bool mathfun_codegen_ins3(mathfun_codegen *codegen, enum mathfun_bytecode code, mathfun_code arg1, mathfun_code arg2, mathfun_code arg3);

MATHFUN_LOCAL bool mathfun_codegen_binary(mathfun_codegen *codegen, mathfun_expr *expr,
	enum mathfun_bytecode code, mathfun_code *ret);

MATHFUN_LOCAL bool mathfun_codegen_unary(mathfun_codegen *codegen, mathfun_expr *expr,
	enum mathfun_bytecode code, mathfun_code *ret);

#ifdef __cplusplus
}
#endif

#endif
