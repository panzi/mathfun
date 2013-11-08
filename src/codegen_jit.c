#include "codegen_jit.h"

#include <jit/jit-dump.h>
#include <errno.h>

static void mathfun_set_errno(int errnum) {
	errno = errnum;
}

static jit_value_t mathfun_jit_mod(jit_function_t funct, jit_value_t left, jit_value_t right) {
	jit_value_t zero = jit_value_create_float64_constant(funct, jit_type_sys_double, 0.0);
	if (!zero) return NULL;
	jit_value_t cmp = jit_insn_eq(funct, right, zero);
	if (!cmp) return NULL;
	jit_label_t right_not_zero = jit_label_undefined;
	if (!jit_insn_branch_if_not(funct, cmp, &right_not_zero)) return NULL;

	jit_type_t params[] = {jit_type_sys_int, jit_type_void};
	jit_value_t args[] = {jit_value_create_nint_constant(funct, jit_type_sys_int, EDOM), NULL};
	if (!args[0]) return NULL;
	jit_type_t sig = jit_type_create_signature(jit_abi_cdecl, jit_type_void, params, 1, 1);
	if (!sig) return NULL;

	// TODO: how to dectet error here?
#pragma GCC diagnostic ignored "-pedantic"
	jit_insn_call_native(funct, "mathfun_set_errno", mathfun_set_errno, sig, args, 1, JIT_CALL_NOTHROW);
#pragma GCC diagnostic pop
	
	if (!jit_insn_label(funct, &right_not_zero)) return NULL;

	params[0] = jit_type_sys_double;
	params[1] = jit_type_sys_double;
	args[0] = left;
	args[1] = right;
	sig = jit_type_create_signature(jit_abi_cdecl, jit_type_sys_double, params, 2, 1);
	if (!sig) return NULL;

#pragma GCC diagnostic ignored "-pedantic"
	jit_value_t mod = jit_insn_call_native(funct, "fmod", fmod, sig, args, 2, JIT_CALL_NOTHROW);
#pragma GCC diagnostic pop
	if (!mod) return NULL;

	jit_label_t mod_zero = jit_label_undefined;
	cmp = jit_insn_ne(funct, mod, zero);
	if (!cmp) return NULL;
	if (!jit_insn_branch_if_not(funct, cmp, &right_not_zero)) return NULL;

	jit_value_t right_neg = jit_insn_lt(funct, right, zero);
	jit_value_t mod_neg   = jit_insn_lt(funct, mod, zero);
	if (!right_neg || !mod_neg) return NULL;
	cmp = jit_insn_eq(funct, right_neg, mod_neg);
	if (!cmp) return NULL;

	jit_label_t not_eq = jit_label_undefined;
	if (!jit_insn_branch_if_not(funct, cmp, &not_eq)) return NULL;
	jit_value_t tmp = jit_insn_add(funct, mod, right);
	if (!tmp) return NULL;
	jit_insn_store(funct, mod, tmp);

	if (!jit_insn_label(funct, &not_eq)) return NULL;

	args[0] = zero;
	args[1] = right;
#pragma GCC diagnostic ignored "-pedantic"
	tmp = jit_insn_call_native(funct, "copysign", copysign, sig, args, 2, JIT_CALL_NOTHROW);
#pragma GCC diagnostic pop
	if (!tmp) return NULL;

	jit_insn_store(funct, mod, tmp);

	if (!jit_insn_label(funct, &mod_zero)) return NULL;

	return mod;
}

typedef jit_value_t (*jit_insn1)(jit_function_t funct, jit_value_t arg1);
typedef jit_value_t (*jit_insn2)(jit_function_t funct, jit_value_t arg1, jit_value_t arg2);

static jit_value_t mathfun_jit_builtin1(mathfun_jit *jit, mathfun_expr *expr, jit_insn1 jit_insn) {
	jit_value_t arg = mathfun_jit_expr(jit, expr->ex.funct.args[0]);
	if (!arg) return NULL;
	return jit_insn(jit->funct, arg);
}

static jit_value_t mathfun_jit_builtin2(mathfun_jit *jit, mathfun_expr *expr, jit_insn2 jit_insn) {
	jit_value_t arg1 = mathfun_jit_expr(jit, expr->ex.funct.args[0]);
	if (!arg1) return NULL;

	jit_value_t arg2 = mathfun_jit_expr(jit, expr->ex.funct.args[1]);
	if (!arg2) return NULL;
	
	return jit_insn(jit->funct, arg1, arg2);
}

jit_value_t mathfun_jit_expr(mathfun_jit *jit, mathfun_expr *expr) {
	switch (expr->type) {
		case EX_CONST:
			if (expr->ex.value.type == MATHFUN_NUMBER) {
				return jit_value_create_float64_constant(jit->funct, jit_type_sys_double, expr->ex.value.value.number);
			}
			else {
				return jit_value_create_nint_constant(jit->funct, jit_type_sys_bool, expr->ex.value.value.boolean);
			}

		case EX_ARG:
			return jit_value_get_param(jit->funct, expr->ex.arg);

		case EX_CALL:
			if (expr->ex.funct.native_funct) {
				mathfun_native_funct native_funct = expr->ex.funct.native_funct;
				// use possibly optimized buliltin versions of some known functions
				if      (native_funct == (mathfun_native_funct)acos)  return mathfun_jit_builtin1(jit, expr, jit_insn_acos);
				else if (native_funct == (mathfun_native_funct)asin)  return mathfun_jit_builtin1(jit, expr, jit_insn_asin);
				else if (native_funct == (mathfun_native_funct)atan)  return mathfun_jit_builtin1(jit, expr, jit_insn_atan);
				else if (native_funct == (mathfun_native_funct)atan2) return mathfun_jit_builtin2(jit, expr, jit_insn_atan2);
				else if (native_funct == (mathfun_native_funct)cos)   return mathfun_jit_builtin1(jit, expr, jit_insn_cos);
				else if (native_funct == (mathfun_native_funct)cosh)  return mathfun_jit_builtin1(jit, expr, jit_insn_cosh);
				else if (native_funct == (mathfun_native_funct)exp)   return mathfun_jit_builtin1(jit, expr, jit_insn_exp);
				else if (native_funct == (mathfun_native_funct)log)   return mathfun_jit_builtin1(jit, expr, jit_insn_log);
				else if (native_funct == (mathfun_native_funct)log10) return mathfun_jit_builtin1(jit, expr, jit_insn_log10);
				else if (native_funct == (mathfun_native_funct)pow)   return mathfun_jit_builtin2(jit, expr, jit_insn_pow);
				else if (native_funct == (mathfun_native_funct)sin)   return mathfun_jit_builtin1(jit, expr, jit_insn_sin);
				else if (native_funct == (mathfun_native_funct)sinh)  return mathfun_jit_builtin1(jit, expr, jit_insn_sinh);
				else if (native_funct == (mathfun_native_funct)sqrt)  return mathfun_jit_builtin1(jit, expr, jit_insn_sqrt);
				else if (native_funct == (mathfun_native_funct)tan)   return mathfun_jit_builtin1(jit, expr, jit_insn_tan);
				else if (native_funct == (mathfun_native_funct)tanh)  return mathfun_jit_builtin1(jit, expr, jit_insn_tanh);
				else if (native_funct == (mathfun_native_funct)ceil)  return mathfun_jit_builtin1(jit, expr, jit_insn_ceil);
				else if (native_funct == (mathfun_native_funct)floor) return mathfun_jit_builtin1(jit, expr, jit_insn_floor);
				else if (native_funct == (mathfun_native_funct)round) return mathfun_jit_builtin1(jit, expr, jit_insn_round);
				else if (native_funct == (mathfun_native_funct)trunc) return mathfun_jit_builtin1(jit, expr, jit_insn_trunc);
				else if (native_funct == (mathfun_native_funct)fabs)  return mathfun_jit_builtin1(jit, expr, jit_insn_abs);
				else if (native_funct == (mathfun_native_funct)fmin)  return mathfun_jit_builtin2(jit, expr, jit_insn_min);
				else if (native_funct == (mathfun_native_funct)fmax)  return mathfun_jit_builtin2(jit, expr, jit_insn_max);

				jit_type_t *params = calloc(expr->ex.funct.sig->argc, sizeof(jit_type_t));
				if (!params) {
					return NULL;
				}
				jit_value_t *args = calloc(expr->ex.funct.sig->argc, sizeof(jit_value_t));
				if (!args) {
					free(params);
					return NULL;
				}
				for (size_t i = 0; i < expr->ex.funct.sig->argc; ++ i) {
					mathfun_type type = expr->ex.funct.sig->argtypes[i];
					params[i] = type == MATHFUN_NUMBER ? jit_type_sys_double : jit_type_sys_bool;
					jit_value_t arg = args[i] = mathfun_jit_expr(jit, expr->ex.funct.args[i]);

					if (arg == NULL) {
						free(params);
						free(args);
						return NULL;
					}
				}
				jit_type_t sig = jit_type_create_signature(jit_abi_cdecl,
					expr->ex.funct.sig->rettype == MATHFUN_NUMBER ? jit_type_sys_double : jit_type_sys_bool,
					params, expr->ex.funct.sig->argc, 1);
				jit_value_t value = NULL;
				
				if (sig) {
#pragma GCC diagnostic ignored "-pedantic"
					value = jit_insn_call_native(jit->funct, expr->ex.funct.name, native_funct,
						 sig, args, expr->ex.funct.sig->argc, JIT_CALL_NOTHROW);
#pragma GCC diagnostic pop
					jit_type_free(sig);
				}

				free(params);
				free(args);

				return value;
			}
			else {
				if (!jit->mathfun_value) {
					jit->mathfun_value = jit_type_create_union(jit->mathfun_value_fields, 2, 1);
					if (!jit->mathfun_value) return NULL;
				}

				if (!jit->mathfun_binding_funct_t) {
					if (!jit->mathfun_binding_funct_params[0]) {
						jit->mathfun_binding_funct_params[0] = jit_type_create_pointer(jit->mathfun_value, 1);
						if (!jit->mathfun_binding_funct_params[0]) return NULL;
					}

					jit->mathfun_binding_funct_t = jit_type_create_signature(
						jit_abi_cdecl, jit->mathfun_value, jit->mathfun_binding_funct_params, 1, 1);
				}

				jit_type_t *array_fields = calloc(expr->ex.funct.sig->argc, sizeof(jit_type_t));
				if (!array_fields) return NULL;
				
				for (size_t i = 0; i < expr->ex.funct.sig->argc; ++ i) {
					array_fields[i] = jit->mathfun_value;
				}

				jit_type_t args_array_type = jit_type_create_struct(array_fields, expr->ex.funct.sig->argc, 1);
				
				if (!args_array_type) {
					free(array_fields);
					return NULL;
				}

				jit_value_t args_array = jit_value_create(jit->funct, args_array_type);

				if (!args_array) {
					jit_type_free(args_array_type);
					free(array_fields);
					return NULL;
				}

				jit_value_t args[] = { args_array };
				jit_value_t array_adr = jit_insn_address_of(jit->funct, args_array);

				if (!array_adr) {
					jit_type_free(args_array_type);
					free(array_fields);
					return NULL;
				}
				
				for (size_t i = 0; i < expr->ex.funct.sig->argc; ++ i) {
					jit_value_t arg = mathfun_jit_expr(jit, expr->ex.funct.args[i]);
					jit_value_t index = jit_value_create_nint_constant(jit->funct, jit_type_sys_ulong, i);

					if (!arg || !index || !jit_insn_store_elem(jit->funct, array_adr, index, arg)) {
						free(array_fields);
						return NULL;
					}
				}

				jit_value_t value = NULL;

#pragma GCC diagnostic ignored "-pedantic"
				value = jit_insn_call_native(jit->funct, expr->ex.funct.name, expr->ex.funct.funct,
					 jit->mathfun_binding_funct_t, args, 1, JIT_CALL_NOTHROW);
#pragma GCC diagnostic pop
				
				free(array_fields);
				jit_type_free(args_array_type);

				return value;
			}

		case EX_NEG:
		{
			jit_value_t value = mathfun_jit_expr(jit, expr->ex.unary.expr);
			return value ? jit_insn_neg(jit->funct, value) : NULL;
		}

		case EX_ADD:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_add(jit->funct, left, right);
		}

		case EX_SUB:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_sub(jit->funct, left, right);
		}

		case EX_MUL:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_mul(jit->funct, left, right);
		}

		case EX_DIV:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_div(jit->funct, left, right);
		}

		case EX_MOD:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return mathfun_jit_mod(jit->funct, left, right);
		}

		case EX_POW:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_pow(jit->funct, left, right);
		}

		case EX_NOT:
		{
			jit_value_t value = mathfun_jit_expr(jit, expr->ex.unary.expr);
			return value ? jit_insn_to_not_bool(jit->funct, value) : NULL;
		}

		case EX_EQ:
		case EX_BEQ:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_eq(jit->funct, left, right);
		}

		case EX_NE:
		case EX_BNE:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_ne(jit->funct, left, right);
		}

		case EX_LT:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_lt(jit->funct, left, right);
		}

		case EX_GT:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_gt(jit->funct, left, right);
		}

		case EX_LE:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_le(jit->funct, left, right);
		}

		case EX_GE:
		{
			jit_value_t left  = mathfun_jit_expr(jit, expr->ex.binary.left);
			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);

			if (left == NULL || right == NULL) {
				return NULL;
			}

			return jit_insn_ge(jit->funct, left, right);
		}

		case EX_IN:
		{
			jit_value_t value = jit_value_create(jit->funct, jit_type_sys_bool);
			if (!value) return NULL;

			jit_value_t num_value = mathfun_jit_expr(jit, expr->ex.binary.left);
			if (!num_value) return NULL;

			mathfun_expr *range = expr->ex.binary.right;
			jit_value_t lower = mathfun_jit_expr(jit, range->ex.binary.left);
			if (!lower) return NULL;

			jit_value_t cmp = jit_insn_ge(jit->funct, num_value, lower);
			if (!cmp) return NULL;
			
			jit_label_t ret_false = jit_label_undefined;
			if (!jit_insn_branch_if_not(jit->funct, cmp, &ret_false)) return NULL;

			jit_value_t upper = mathfun_jit_expr(jit, range->ex.binary.left);
			if (!upper) return NULL;

			cmp = range->type == EX_RNG_INCL ?
				jit_insn_le(jit->funct, num_value, upper) :
				jit_insn_lt(jit->funct, num_value, upper);
			if (!cmp) return NULL;

			jit_insn_store(jit->funct, value, cmp);

			if (!jit_insn_label(jit->funct, &ret_false)) return NULL;

			jit_value_t val_false = jit_value_create_nint_constant(jit->funct, jit_type_sys_bool, false);
			if (!val_false) return NULL;

			jit_insn_store(jit->funct, value, val_false);

			return value;
		}
		
		case EX_RNG_INCL:
		case EX_RNG_EXCL:
			return NULL;

		case EX_AND:
		{
			jit_value_t value = jit_value_create(jit->funct, jit_type_sys_bool);
			if (!value) return NULL;

			jit_value_t left = mathfun_jit_expr(jit, expr->ex.binary.left);
			if (!left) return NULL;

			jit_label_t ret_false = jit_label_undefined;
			if (!jit_insn_branch_if_not(jit->funct, left, &ret_false)) return NULL;

			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);
			if (!right) return NULL;

			jit_insn_store(jit->funct, value, right);

			jit_label_t done = jit_label_undefined;

			if (!jit_insn_branch(jit->funct, &done)) return NULL;

			if (!jit_insn_label(jit->funct, &ret_false)) return NULL;

			jit_value_t val_false = jit_value_create_nint_constant(jit->funct, jit_type_sys_bool, false);
			if (!val_false) return NULL;

			jit_insn_store(jit->funct, value, val_false);

			if (!jit_insn_label(jit->funct, &done)) return NULL;

			return value;
		}

		case EX_OR:
		{
			jit_value_t value = jit_value_create(jit->funct, jit_type_sys_bool);
			if (!value) return NULL;

			jit_value_t left = mathfun_jit_expr(jit, expr->ex.binary.left);
			if (!left) return NULL;

			jit_label_t ret_true = jit_label_undefined;
			if (!jit_insn_branch_if(jit->funct, left, &ret_true)) return NULL;

			jit_value_t right = mathfun_jit_expr(jit, expr->ex.binary.right);
			if (!right) return NULL;

			jit_insn_store(jit->funct, value, right);

			jit_label_t done = jit_label_undefined;
			if (!jit_insn_branch(jit->funct, &done)) return NULL;

			if (!jit_insn_label(jit->funct, &ret_true)) return NULL;

			jit_value_t val_true = jit_value_create_nint_constant(jit->funct, jit_type_sys_bool, true);
			if (!val_true) return NULL;

			jit_insn_store(jit->funct, value, val_true);

			if (!jit_insn_label(jit->funct, &done)) return NULL;

			return value;
		}

		case EX_IIF:
		{
			jit_value_t value = jit_value_create(jit->funct,
				mathfun_expr_type(expr) == MATHFUN_NUMBER ?
				jit_type_sys_double : jit_type_sys_bool);

			jit_value_t cond = mathfun_jit_expr(jit, expr->ex.iif.cond);
			if (!cond) return NULL;
			
			jit_label_t do_else = jit_label_undefined;
			if (!jit_insn_branch_if_not(jit->funct, cond, &do_else)) return NULL;

			jit_value_t then_value = mathfun_jit_expr(jit, expr->ex.iif.then_expr);
			if (!then_value) return NULL;

			jit_insn_store(jit->funct, value, then_value);

			jit_label_t done = jit_label_undefined;
			if (!jit_insn_branch(jit->funct, &done)) return NULL;

			jit_value_t else_value = mathfun_jit_expr(jit, expr->ex.iif.else_expr);
			if (!else_value) return NULL;
			
			jit_insn_store(jit->funct, value, else_value);

			if (!jit_insn_label(jit->funct, &done)) return NULL;

			return value;
		}
	}

	return NULL;
}

bool mathfun_expr_codegen(mathfun_expr *expr, mathfun *fun, mathfun_error_p *error) {
	jit_context_t context = NULL;
	jit_type_t *params = NULL;
	jit_type_t signature = NULL;
	mathfun_jit jit;
	mathfun_jit_code *code = NULL;

	jit.argc  = fun->argc;
	jit.funct = NULL;
	jit.mathfun_value_fields[0] = jit_type_sys_double;
	jit.mathfun_value_fields[1] = jit_type_sys_bool;
	jit.mathfun_value = NULL;
	jit.mathfun_binding_funct_params[0] = NULL;
	jit.mathfun_binding_funct_t = NULL;

	fun->framesize = fun->argc + (1 + ((sizeof(void *) * fun->argc) - 1) / sizeof(mathfun_value));

	code = malloc(sizeof(mathfun_jit_code));
	if (!code) goto onerror;

	params = calloc(fun->argc, sizeof(jit_type_t));
	if (!params) goto onerror;

	context = jit_context_create();
	if (!context) goto onerror;

	jit_context_build_start(context);

	for (size_t i = 0; i < fun->argc; ++ i) {
		params[i] = jit_type_sys_double;
	}

	signature = jit_type_create_signature(jit_abi_cdecl, jit_type_sys_double, params, fun->argc, 1);
	if (!signature) goto onerror;

	jit.funct = jit_function_create(context, signature);
	if (!jit.funct) goto onerror;

	jit_value_t ret = mathfun_jit_expr(&jit, expr);
	if (!ret) goto onerror;

	jit_insn_return(jit.funct, ret);

	jit_context_build_end(context);

	if (!jit_function_compile(jit.funct)) goto onerror;

	free(params);
	params = NULL;

	if (jit.mathfun_value) jit_type_free(jit.mathfun_value);
	jit.mathfun_value = NULL;

	if (jit.mathfun_binding_funct_t) jit_type_free(jit.mathfun_binding_funct_t);
	jit.mathfun_binding_funct_t = NULL;

	if (jit.mathfun_binding_funct_params[0]) jit_type_free(jit.mathfun_binding_funct_params[0]);
	jit.mathfun_binding_funct_params[0] = NULL;

	code->context = context;
	code->funct   = jit.funct;

	fun->code = code;

	return true;

onerror:
	if (errno == 0) {
		mathfun_raise_error(error, MATHFUN_INTERNAL_ERROR);
	}
	else {
		mathfun_raise_c_error(error);
	}

	if (jit.funct) jit_function_abandon(jit.funct);
	if (signature) jit_type_free(signature);
	if (jit.mathfun_binding_funct_t) jit_type_free(jit.mathfun_binding_funct_t);
	if (jit.mathfun_binding_funct_params[0]) jit_type_free(jit.mathfun_binding_funct_params[0]);
	if (jit.mathfun_value) jit_type_free(jit.mathfun_value);
	if (context)   jit_context_destroy(context);

	free(params);
	free(code);

	return false;
}

void mathfun_cleanup(mathfun *fun) {
	mathfun_jit_code *code = fun->code;
	if (code && code->context) {
		jit_context_destroy(code->context);
	}

	free(fun->code);
	fun->code = NULL;
}

bool mathfun_dump(const mathfun *fun, FILE *stream, const mathfun_context *ctx, mathfun_error_p *error) {
	(void)ctx;
	(void)error;
	
	mathfun_jit_code *code = fun->code;
	jit_dump_function(stream, code->funct, NULL);

	return true;
}
