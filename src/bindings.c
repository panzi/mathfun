#include "mathfun_intern.h"

static mathfun_value mathfun_funct_acos(const mathfun_value args[]) {
	return acos(args[0]);
}

static mathfun_value mathfun_funct_acosh(const mathfun_value args[]) {
	return acosh(args[0]);
}

static mathfun_value mathfun_funct_asin(const mathfun_value args[]) {
	return asin(args[0]);
}

static mathfun_value mathfun_funct_asinh(const mathfun_value args[]) {
	return asinh(args[0]);
}

static mathfun_value mathfun_funct_atan(const mathfun_value args[]) {
	return atan(args[0]);
}

static mathfun_value mathfun_funct_atan2(const mathfun_value args[]) {
	return atan2(args[0], args[1]);
}

static mathfun_value mathfun_funct_atanh(const mathfun_value args[]) {
	return atanh(args[0]);
}

static mathfun_value mathfun_funct_cbrt(const mathfun_value args[]) {
	return cbrt(args[0]);
}

static mathfun_value mathfun_funct_ceil(const mathfun_value args[]) {
	return ceil(args[0]);
}

static mathfun_value mathfun_funct_copysign(const mathfun_value args[]) {
	return copysign(args[0], args[1]);
}

static mathfun_value mathfun_funct_cos(const mathfun_value args[]) {
	return cos(args[0]);
}

static mathfun_value mathfun_funct_cosh(const mathfun_value args[]) {
	return cosh(args[0]);
}

static mathfun_value mathfun_funct_erf(const mathfun_value args[]) {
	return erf(args[0]);
}

static mathfun_value mathfun_funct_erfc(const mathfun_value args[]) {
	return erfc(args[0]);
}

static mathfun_value mathfun_funct_exp(const mathfun_value args[]) {
	return exp(args[0]);
}

static mathfun_value mathfun_funct_exp2(const mathfun_value args[]) {
	return exp2(args[0]);
}

static mathfun_value mathfun_funct_expm1(const mathfun_value args[]) {
	return expm1(args[0]);
}

static mathfun_value mathfun_funct_abs(const mathfun_value args[]) {
	return fabs(args[0]);
}

static mathfun_value mathfun_funct_fdim(const mathfun_value args[]) {
	return fdim(args[0], args[1]);
}

static mathfun_value mathfun_funct_floor(const mathfun_value args[]) {
	return floor(args[0]);
}

static mathfun_value mathfun_funct_fma(const mathfun_value args[]) {
	return fma(args[0], args[1], args[2]);
}

static mathfun_value mathfun_funct_fmod(const mathfun_value args[]) {
	return fmod(args[0], args[1]);
}

static mathfun_value mathfun_funct_max(const mathfun_value args[]) {
	mathfun_value x = args[0];
	mathfun_value y = args[1];
	return (x >= y || isnan(x)) ? x : y;
//	return fmax(args[0], args[1]);
}

static mathfun_value mathfun_funct_min(const mathfun_value args[]) {
	mathfun_value x = args[0];
	mathfun_value y = args[1];
	return (x <= y || isnan(y)) ? x : y;
//	return fmin(args[0], args[1]);
}

static mathfun_value mathfun_funct_hypot(const mathfun_value args[]) {
	return hypot(args[0], args[1]);
}

static mathfun_value mathfun_funct_j0(const mathfun_value args[]) {
	return j0(args[0]);
}

static mathfun_value mathfun_funct_j1(const mathfun_value args[]) {
	return j1(args[0]);
}

static mathfun_value mathfun_funct_jn(const mathfun_value args[]) {
	return jn((int)args[0], args[1]);
}

static mathfun_value mathfun_funct_ldexp(const mathfun_value args[]) {
	return ldexp(args[0], (int)args[1]);
}

static mathfun_value mathfun_funct_log(const mathfun_value args[]) {
	return log(args[0]);
}

static mathfun_value mathfun_funct_log10(const mathfun_value args[]) {
	return log10(args[0]);
}

static mathfun_value mathfun_funct_log1p(const mathfun_value args[]) {
	return log1p(args[0]);
}

static mathfun_value mathfun_funct_log2(const mathfun_value args[]) {
	return log2(args[0]);
}

static mathfun_value mathfun_funct_logb(const mathfun_value args[]) {
	return logb(args[0]);
}

static mathfun_value mathfun_funct_nearbyint(const mathfun_value args[]) {
	return nearbyint(args[0]);
}

static mathfun_value mathfun_funct_nextafter(const mathfun_value args[]) {
	return nextafter(args[0], args[1]);
}

static mathfun_value mathfun_funct_nexttoward(const mathfun_value args[]) {
	return nexttoward(args[0], args[1]);
}

static mathfun_value mathfun_funct_remainder(const mathfun_value args[]) {
	return remainder(args[0], args[1]);
}

static mathfun_value mathfun_funct_round(const mathfun_value args[]) {
	return round(args[0]);
}

static mathfun_value mathfun_funct_scalbln(const mathfun_value args[]) {
	return scalbln(args[0], (long)args[1]);
}

static mathfun_value mathfun_funct_sin(const mathfun_value args[]) {
	return sin(args[0]);
}

static mathfun_value mathfun_funct_sinh(const mathfun_value args[]) {
	return sinh(args[0]);
}

static mathfun_value mathfun_funct_sqrt(const mathfun_value args[]) {
	return sqrt(args[0]);
}

static mathfun_value mathfun_funct_tan(const mathfun_value args[]) {
	return tan(args[0]);
}

static mathfun_value mathfun_funct_tanh(const mathfun_value args[]) {
	return tanh(args[0]);
}

static mathfun_value mathfun_funct_gamma(const mathfun_value args[]) {
	return tgamma(args[0]);
}

static mathfun_value mathfun_funct_trunc(const mathfun_value args[]) {
	return trunc(args[0]);
}

static mathfun_value mathfun_funct_y0(const mathfun_value args[]) {
	return y0(args[0]);
}

static mathfun_value mathfun_funct_y1(const mathfun_value args[]) {
	return y1(args[0]);
}

static mathfun_value mathfun_funct_yn(const mathfun_value args[]) {
	return yn((int)args[0], args[1]);
}

bool mathfun_context_define_default(mathfun_context *ctx, mathfun_error_p *error) {
	// Constants
	if (!mathfun_context_define_const(ctx, "e", M_E, error) ||
		!mathfun_context_define_const(ctx, "log2e", M_LOG2E, error) ||
		!mathfun_context_define_const(ctx, "log10e", M_LOG10E, error) ||
		!mathfun_context_define_const(ctx, "ln2", M_LN2, error) ||
		!mathfun_context_define_const(ctx, "ln10", M_LN10, error) ||
		!mathfun_context_define_const(ctx, "pi", M_PI, error) ||
		!mathfun_context_define_const(ctx, "tau", 2*M_PI, error) ||
		!mathfun_context_define_const(ctx, "pi_2", M_PI_2, error) ||
		!mathfun_context_define_const(ctx, "pi_4", M_PI_4, error) ||
		!mathfun_context_define_const(ctx, "_1_pi", M_1_PI, error) ||
		!mathfun_context_define_const(ctx, "_2_pi", M_2_PI, error) ||
		!mathfun_context_define_const(ctx, "_2_sqrtpi", M_2_SQRTPI, error) ||
		!mathfun_context_define_const(ctx, "sqrt2", M_SQRT2, error) ||
		!mathfun_context_define_const(ctx, "sqrt1_2", M_SQRT1_2, error)) {
		return false;
	}

	// Functions
	if (!mathfun_context_define_funct(ctx, "acos", mathfun_funct_acos, 1, error) ||
		!mathfun_context_define_funct(ctx, "acosh", mathfun_funct_acosh, 1, error) ||
		!mathfun_context_define_funct(ctx, "asin", mathfun_funct_asin, 1, error) ||
		!mathfun_context_define_funct(ctx, "asinh", mathfun_funct_asinh, 1, error) ||
		!mathfun_context_define_funct(ctx, "atan", mathfun_funct_atan, 1, error) ||
		!mathfun_context_define_funct(ctx, "atan2", mathfun_funct_atan2, 2, error) ||
		!mathfun_context_define_funct(ctx, "atanh", mathfun_funct_atanh, 1, error) ||
		!mathfun_context_define_funct(ctx, "cbrt", mathfun_funct_cbrt, 1, error) ||
		!mathfun_context_define_funct(ctx, "ceil", mathfun_funct_ceil, 1, error) ||
		!mathfun_context_define_funct(ctx, "copysign", mathfun_funct_copysign, 2, error) ||
		!mathfun_context_define_funct(ctx, "cos", mathfun_funct_cos, 1, error) ||
		!mathfun_context_define_funct(ctx, "cosh", mathfun_funct_cosh, 1, error) ||
		!mathfun_context_define_funct(ctx, "erf", mathfun_funct_erf, 1, error) ||
		!mathfun_context_define_funct(ctx, "erfc", mathfun_funct_erfc, 1, error) ||
		!mathfun_context_define_funct(ctx, "exp", mathfun_funct_exp, 1, error) ||
		!mathfun_context_define_funct(ctx, "exp2", mathfun_funct_exp2, 1, error) ||
		!mathfun_context_define_funct(ctx, "expm1", mathfun_funct_expm1, 1, error) ||
		!mathfun_context_define_funct(ctx, "abs", mathfun_funct_abs, 1, error) ||
		!mathfun_context_define_funct(ctx, "fdim", mathfun_funct_fdim, 2, error) ||
		!mathfun_context_define_funct(ctx, "floor", mathfun_funct_floor, 1, error) ||
		!mathfun_context_define_funct(ctx, "fma", mathfun_funct_fma, 3, error) ||
		!mathfun_context_define_funct(ctx, "fmod", mathfun_funct_fmod, 2, error) ||
		!mathfun_context_define_funct(ctx, "max", mathfun_funct_max, 2, error) ||
		!mathfun_context_define_funct(ctx, "min", mathfun_funct_min, 2, error) ||
		!mathfun_context_define_funct(ctx, "hypot", mathfun_funct_hypot, 2, error) ||
		!mathfun_context_define_funct(ctx, "j0", mathfun_funct_j0, 1, error) ||
		!mathfun_context_define_funct(ctx, "j1", mathfun_funct_j1, 1, error) ||
		!mathfun_context_define_funct(ctx, "jn", mathfun_funct_jn, 2, error) ||
		!mathfun_context_define_funct(ctx, "ldexp", mathfun_funct_ldexp, 2, error) ||
		!mathfun_context_define_funct(ctx, "log", mathfun_funct_log, 1, error) ||
		!mathfun_context_define_funct(ctx, "log10", mathfun_funct_log10, 1, error) ||
		!mathfun_context_define_funct(ctx, "log1p", mathfun_funct_log1p, 1, error) ||
		!mathfun_context_define_funct(ctx, "log2", mathfun_funct_log2, 1, error) ||
		!mathfun_context_define_funct(ctx, "logb", mathfun_funct_logb, 1, error) ||
		!mathfun_context_define_funct(ctx, "nearbyint", mathfun_funct_nearbyint, 1, error) ||
		!mathfun_context_define_funct(ctx, "nextafter", mathfun_funct_nextafter, 2, error) ||
		!mathfun_context_define_funct(ctx, "nexttoward", mathfun_funct_nexttoward, 2, error) ||
		!mathfun_context_define_funct(ctx, "remainder", mathfun_funct_remainder, 2, error) ||
		!mathfun_context_define_funct(ctx, "round", mathfun_funct_round, 1, error) ||
		!mathfun_context_define_funct(ctx, "scalbln", mathfun_funct_scalbln, 2, error) ||
		!mathfun_context_define_funct(ctx, "sin", mathfun_funct_sin, 1, error) ||
		!mathfun_context_define_funct(ctx, "sinh", mathfun_funct_sinh, 1, error) ||
		!mathfun_context_define_funct(ctx, "sqrt", mathfun_funct_sqrt, 1, error) ||
		!mathfun_context_define_funct(ctx, "tan", mathfun_funct_tan, 1, error) ||
		!mathfun_context_define_funct(ctx, "tanh", mathfun_funct_tanh, 1, error) ||
		!mathfun_context_define_funct(ctx, "gamma", mathfun_funct_gamma, 1, error) ||
		!mathfun_context_define_funct(ctx, "trunc", mathfun_funct_trunc, 1, error) ||
		!mathfun_context_define_funct(ctx, "y0", mathfun_funct_y0, 1, error) ||
		!mathfun_context_define_funct(ctx, "y1", mathfun_funct_y1, 1, error) ||
		!mathfun_context_define_funct(ctx, "yn", mathfun_funct_yn, 2, error)) {
		return false;
	}

	return true;
}
