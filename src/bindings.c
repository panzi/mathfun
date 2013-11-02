#include "mathfun_intern.h"

static const mathfun_sig mathfun_bsig1 = {
	1, (mathfun_type[]){MATHFUN_NUMBER}, MATHFUN_BOOLEAN
};

static const mathfun_sig mathfun_bsig2 = {
	2, (mathfun_type[]){MATHFUN_NUMBER, MATHFUN_NUMBER}, MATHFUN_BOOLEAN
};

static const mathfun_sig mathfun_sig1 = {
	1, (mathfun_type[]){MATHFUN_NUMBER}, MATHFUN_NUMBER
};

static const mathfun_sig mathfun_sig2 = {
	2, (mathfun_type[]){MATHFUN_NUMBER, MATHFUN_NUMBER}, MATHFUN_NUMBER
};

static const mathfun_sig mathfun_sig3 = {
	3, (mathfun_type[]){MATHFUN_NUMBER, MATHFUN_NUMBER, MATHFUN_NUMBER}, MATHFUN_NUMBER
};

static mathfun_value mathfun_funct_isnan(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = isnan(args[0].number) };
}

static mathfun_value mathfun_funct_isfinite(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = isfinite(args[0].number) };
}

static mathfun_value mathfun_funct_isnormal(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = isnormal(args[0].number) };
}

static mathfun_value mathfun_funct_isinf(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = isinf(args[0].number) };
}

static mathfun_value mathfun_funct_isgreater(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = isgreater(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_isgreaterequal(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = isgreaterequal(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_isless(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = isless(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_islessequal(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = islessequal(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_islessgreater(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = islessgreater(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_isunordered(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = isunordered(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_signbit(const mathfun_value args[]) {
	return (mathfun_value){ .boolean = signbit(args[0].number) != 0 };
}

static mathfun_value mathfun_funct_acos(const mathfun_value args[]) {
	return (mathfun_value){ .number = acos(args[0].number) };
}

static mathfun_value mathfun_funct_acosh(const mathfun_value args[]) {
	return (mathfun_value){ .number = acosh(args[0].number) };
}

static mathfun_value mathfun_funct_asin(const mathfun_value args[]) {
	return (mathfun_value){ .number = asin(args[0].number) };
}

static mathfun_value mathfun_funct_asinh(const mathfun_value args[]) {
	return (mathfun_value){ .number = asinh(args[0].number) };
}

static mathfun_value mathfun_funct_atan(const mathfun_value args[]) {
	return (mathfun_value){ .number = atan(args[0].number) };
}

static mathfun_value mathfun_funct_atan2(const mathfun_value args[]) {
	return (mathfun_value){ .number = atan2(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_atanh(const mathfun_value args[]) {
	return (mathfun_value){ .number = atanh(args[0].number) };
}

static mathfun_value mathfun_funct_cbrt(const mathfun_value args[]) {
	return (mathfun_value){ .number = cbrt(args[0].number) };
}

static mathfun_value mathfun_funct_ceil(const mathfun_value args[]) {
	return (mathfun_value){ .number = ceil(args[0].number) };
}

static mathfun_value mathfun_funct_copysign(const mathfun_value args[]) {
	return (mathfun_value){ .number = copysign(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_cos(const mathfun_value args[]) {
	return (mathfun_value){ .number = cos(args[0].number) };
}

static mathfun_value mathfun_funct_cosh(const mathfun_value args[]) {
	return (mathfun_value){ .number = cosh(args[0].number) };
}

static mathfun_value mathfun_funct_erf(const mathfun_value args[]) {
	return (mathfun_value){ .number = erf(args[0].number) };
}

static mathfun_value mathfun_funct_erfc(const mathfun_value args[]) {
	return (mathfun_value){ .number = erfc(args[0].number) };
}

static mathfun_value mathfun_funct_exp(const mathfun_value args[]) {
	return (mathfun_value){ .number = exp(args[0].number) };
}

static mathfun_value mathfun_funct_exp2(const mathfun_value args[]) {
	return (mathfun_value){ .number = exp2(args[0].number) };
}

static mathfun_value mathfun_funct_expm1(const mathfun_value args[]) {
	return (mathfun_value){ .number = expm1(args[0].number) };
}

static mathfun_value mathfun_funct_abs(const mathfun_value args[]) {
	return (mathfun_value){ .number = fabs(args[0].number) };
}

static mathfun_value mathfun_funct_fdim(const mathfun_value args[]) {
	return (mathfun_value){ .number = fdim(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_floor(const mathfun_value args[]) {
	return (mathfun_value){ .number = floor(args[0].number) };
}

static mathfun_value mathfun_funct_fma(const mathfun_value args[]) {
	return (mathfun_value){ .number = fma(args[0].number, args[1].number, args[2].number) };
}

static mathfun_value mathfun_funct_fmod(const mathfun_value args[]) {
	return (mathfun_value){ .number = fmod(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_max(const mathfun_value args[]) {
	double x = args[0].number;
	double y = args[1].number;
	return (mathfun_value){ .number = (x >= y || isnan(x)) ? x : y };
//	return (mathfun_value){ .number = fmax(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_min(const mathfun_value args[]) {
	double x = args[0].number;
	double y = args[1].number;
	return (mathfun_value){ .number = (x <= y || isnan(y)) ? x : y };
//	return (mathfun_value){ .number = fmin(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_hypot(const mathfun_value args[]) {
	return (mathfun_value){ .number = hypot(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_j0(const mathfun_value args[]) {
	return (mathfun_value){ .number = j0(args[0].number) };
}

static mathfun_value mathfun_funct_j1(const mathfun_value args[]) {
	return (mathfun_value){ .number = j1(args[0].number) };
}

static mathfun_value mathfun_funct_jn(const mathfun_value args[]) {
	return (mathfun_value){ .number = jn((int)args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_ldexp(const mathfun_value args[]) {
	return (mathfun_value){ .number = ldexp(args[0].number, (int)args[1].number) };
}

static mathfun_value mathfun_funct_log(const mathfun_value args[]) {
	return (mathfun_value){ .number = log(args[0].number) };
}

static mathfun_value mathfun_funct_log10(const mathfun_value args[]) {
	return (mathfun_value){ .number = log10(args[0].number) };
}

static mathfun_value mathfun_funct_log1p(const mathfun_value args[]) {
	return (mathfun_value){ .number = log1p(args[0].number) };
}

static mathfun_value mathfun_funct_log2(const mathfun_value args[]) {
	return (mathfun_value){ .number = log2(args[0].number) };
}

static mathfun_value mathfun_funct_logb(const mathfun_value args[]) {
	return (mathfun_value){ .number = logb(args[0].number) };
}

static mathfun_value mathfun_funct_nearbyint(const mathfun_value args[]) {
	return (mathfun_value){ .number = nearbyint(args[0].number) };
}

static mathfun_value mathfun_funct_nextafter(const mathfun_value args[]) {
	return (mathfun_value){ .number = nextafter(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_nexttoward(const mathfun_value args[]) {
	return (mathfun_value){ .number = nexttoward(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_remainder(const mathfun_value args[]) {
	return (mathfun_value){ .number = remainder(args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_round(const mathfun_value args[]) {
	return (mathfun_value){ .number = round(args[0].number) };
}

static mathfun_value mathfun_funct_scalbln(const mathfun_value args[]) {
	return (mathfun_value){ .number = scalbln(args[0].number, (long)args[1].number) };
}

static mathfun_value mathfun_funct_sin(const mathfun_value args[]) {
	return (mathfun_value){ .number = sin(args[0].number) };
}

static mathfun_value mathfun_funct_sinh(const mathfun_value args[]) {
	return (mathfun_value){ .number = sinh(args[0].number) };
}

static mathfun_value mathfun_funct_sqrt(const mathfun_value args[]) {
	return (mathfun_value){ .number = sqrt(args[0].number) };
}

static mathfun_value mathfun_funct_tan(const mathfun_value args[]) {
	return (mathfun_value){ .number = tan(args[0].number) };
}

static mathfun_value mathfun_funct_tanh(const mathfun_value args[]) {
	return (mathfun_value){ .number = tanh(args[0].number) };
}

static mathfun_value mathfun_funct_gamma(const mathfun_value args[]) {
	return (mathfun_value){ .number = tgamma(args[0].number) };
}

static mathfun_value mathfun_funct_trunc(const mathfun_value args[]) {
	return (mathfun_value){ .number = trunc(args[0].number) };
}

static mathfun_value mathfun_funct_y0(const mathfun_value args[]) {
	return (mathfun_value){ .number = y0(args[0].number) };
}

static mathfun_value mathfun_funct_y1(const mathfun_value args[]) {
	return (mathfun_value){ .number = y1(args[0].number) };
}

static mathfun_value mathfun_funct_yn(const mathfun_value args[]) {
	return (mathfun_value){ .number = yn((int)args[0].number, args[1].number) };
}

static mathfun_value mathfun_funct_sign(const mathfun_value args[]) {
	const double x = args[0].number;
	return (mathfun_value){ .number = isnan(x) || x == 0.0 ? x : copysign(1.0, x) };
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
	if (!mathfun_context_define_funct(ctx, "isnan", mathfun_funct_isnan, &mathfun_bsig1, error) ||
		!mathfun_context_define_funct(ctx, "isfinite", mathfun_funct_isfinite, &mathfun_bsig1, error) ||
		!mathfun_context_define_funct(ctx, "isnormal", mathfun_funct_isnormal, &mathfun_bsig1, error) ||
		!mathfun_context_define_funct(ctx, "isinf", mathfun_funct_isinf, &mathfun_bsig1, error) ||
		!mathfun_context_define_funct(ctx, "isgreater", mathfun_funct_isgreater, &mathfun_bsig2, error) ||
		!mathfun_context_define_funct(ctx, "isgreaterequal", mathfun_funct_isgreaterequal, &mathfun_bsig2, error) ||
		!mathfun_context_define_funct(ctx, "isless", mathfun_funct_isless, &mathfun_bsig2, error) ||
		!mathfun_context_define_funct(ctx, "islessequal", mathfun_funct_islessequal, &mathfun_bsig2, error) ||
		!mathfun_context_define_funct(ctx, "islessgreater", mathfun_funct_islessgreater, &mathfun_bsig2, error) ||
		!mathfun_context_define_funct(ctx, "isunordered", mathfun_funct_isunordered, &mathfun_bsig2, error) ||
		!mathfun_context_define_funct(ctx, "signbit", mathfun_funct_signbit, &mathfun_bsig2, error) ||
		!mathfun_context_define_funct(ctx, "acos", mathfun_funct_acos, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "acosh", mathfun_funct_acosh, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "asin", mathfun_funct_asin, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "asinh", mathfun_funct_asinh, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "atan", mathfun_funct_atan, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "atan2", mathfun_funct_atan2, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "atanh", mathfun_funct_atanh, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "cbrt", mathfun_funct_cbrt, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "ceil", mathfun_funct_ceil, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "copysign", mathfun_funct_copysign, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "cos", mathfun_funct_cos, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "cosh", mathfun_funct_cosh, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "erf", mathfun_funct_erf, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "erfc", mathfun_funct_erfc, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "exp", mathfun_funct_exp, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "exp2", mathfun_funct_exp2, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "expm1", mathfun_funct_expm1, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "abs", mathfun_funct_abs, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "fdim", mathfun_funct_fdim, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "floor", mathfun_funct_floor, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "fma", mathfun_funct_fma, &mathfun_sig3, error) ||
		!mathfun_context_define_funct(ctx, "fmod", mathfun_funct_fmod, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "max", mathfun_funct_max, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "min", mathfun_funct_min, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "hypot", mathfun_funct_hypot, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "j0", mathfun_funct_j0, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "j1", mathfun_funct_j1, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "jn", mathfun_funct_jn, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "ldexp", mathfun_funct_ldexp, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "log", mathfun_funct_log, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "log10", mathfun_funct_log10, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "log1p", mathfun_funct_log1p, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "log2", mathfun_funct_log2, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "logb", mathfun_funct_logb, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "nearbyint", mathfun_funct_nearbyint, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "nextafter", mathfun_funct_nextafter, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "nexttoward", mathfun_funct_nexttoward, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "remainder", mathfun_funct_remainder, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "round", mathfun_funct_round, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "scalbln", mathfun_funct_scalbln, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "sin", mathfun_funct_sin, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "sinh", mathfun_funct_sinh, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "sqrt", mathfun_funct_sqrt, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "tan", mathfun_funct_tan, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "tanh", mathfun_funct_tanh, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "gamma", mathfun_funct_gamma, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "trunc", mathfun_funct_trunc, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "y0", mathfun_funct_y0, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "y1", mathfun_funct_y1, &mathfun_sig1, error) ||
		!mathfun_context_define_funct(ctx, "yn", mathfun_funct_yn, &mathfun_sig2, error) ||
		!mathfun_context_define_funct(ctx, "sign", mathfun_funct_sign, &mathfun_sig1, error)) {
		return false;
	}

	return true;
}
