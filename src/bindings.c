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

static mathfun_reg mathfun_funct_isnan(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = isnan(args[0].number) };
}

static mathfun_reg mathfun_funct_isfinite(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = isfinite(args[0].number) };
}

static mathfun_reg mathfun_funct_isnormal(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = isnormal(args[0].number) };
}

static mathfun_reg mathfun_funct_isinf(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = isinf(args[0].number) };
}

static mathfun_reg mathfun_funct_isgreater(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = isgreater(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_isgreaterequal(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = isgreaterequal(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_isless(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = isless(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_islessequal(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = islessequal(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_islessgreater(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = islessgreater(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_isunordered(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = isunordered(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_signbit(const mathfun_reg args[]) {
	return (mathfun_reg){ .boolean = signbit(args[0].number) != 0 };
}

static mathfun_reg mathfun_funct_acos(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = acos(args[0].number) };
}

static mathfun_reg mathfun_funct_acosh(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = acosh(args[0].number) };
}

static mathfun_reg mathfun_funct_asin(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = asin(args[0].number) };
}

static mathfun_reg mathfun_funct_asinh(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = asinh(args[0].number) };
}

static mathfun_reg mathfun_funct_atan(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = atan(args[0].number) };
}

static mathfun_reg mathfun_funct_atan2(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = atan2(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_atanh(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = atanh(args[0].number) };
}

static mathfun_reg mathfun_funct_cbrt(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = cbrt(args[0].number) };
}

static mathfun_reg mathfun_funct_ceil(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = ceil(args[0].number) };
}

static mathfun_reg mathfun_funct_copysign(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = copysign(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_cos(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = cos(args[0].number) };
}

static mathfun_reg mathfun_funct_cosh(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = cosh(args[0].number) };
}

static mathfun_reg mathfun_funct_erf(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = erf(args[0].number) };
}

static mathfun_reg mathfun_funct_erfc(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = erfc(args[0].number) };
}

static mathfun_reg mathfun_funct_exp(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = exp(args[0].number) };
}

static mathfun_reg mathfun_funct_exp2(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = exp2(args[0].number) };
}

static mathfun_reg mathfun_funct_expm1(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = expm1(args[0].number) };
}

static mathfun_reg mathfun_funct_abs(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = fabs(args[0].number) };
}

static mathfun_reg mathfun_funct_fdim(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = fdim(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_floor(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = floor(args[0].number) };
}

static mathfun_reg mathfun_funct_fma(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = fma(args[0].number, args[1].number, args[2].number) };
}

static mathfun_reg mathfun_funct_fmod(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = fmod(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_max(const mathfun_reg args[]) {
	double x = args[0].number;
	double y = args[1].number;
	return (mathfun_reg){ .number = (x >= y || isnan(x)) ? x : y };
//	return (mathfun_reg){ .number = fmax(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_min(const mathfun_reg args[]) {
	double x = args[0].number;
	double y = args[1].number;
	return (mathfun_reg){ .number = (x <= y || isnan(y)) ? x : y };
//	return (mathfun_reg){ .number = fmin(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_hypot(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = hypot(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_j0(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = j0(args[0].number) };
}

static mathfun_reg mathfun_funct_j1(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = j1(args[0].number) };
}

static mathfun_reg mathfun_funct_jn(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = jn((int)args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_ldexp(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = ldexp(args[0].number, (int)args[1].number) };
}

static mathfun_reg mathfun_funct_log(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = log(args[0].number) };
}

static mathfun_reg mathfun_funct_log10(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = log10(args[0].number) };
}

static mathfun_reg mathfun_funct_log1p(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = log1p(args[0].number) };
}

static mathfun_reg mathfun_funct_log2(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = log2(args[0].number) };
}

static mathfun_reg mathfun_funct_logb(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = logb(args[0].number) };
}

static mathfun_reg mathfun_funct_nearbyint(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = nearbyint(args[0].number) };
}

static mathfun_reg mathfun_funct_nextafter(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = nextafter(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_nexttoward(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = nexttoward(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_remainder(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = remainder(args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_round(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = round(args[0].number) };
}

static mathfun_reg mathfun_funct_scalbln(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = scalbln(args[0].number, (long)args[1].number) };
}

static mathfun_reg mathfun_funct_sin(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = sin(args[0].number) };
}

static mathfun_reg mathfun_funct_sinh(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = sinh(args[0].number) };
}

static mathfun_reg mathfun_funct_sqrt(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = sqrt(args[0].number) };
}

static mathfun_reg mathfun_funct_tan(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = tan(args[0].number) };
}

static mathfun_reg mathfun_funct_tanh(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = tanh(args[0].number) };
}

static mathfun_reg mathfun_funct_gamma(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = tgamma(args[0].number) };
}

static mathfun_reg mathfun_funct_trunc(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = trunc(args[0].number) };
}

static mathfun_reg mathfun_funct_y0(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = y0(args[0].number) };
}

static mathfun_reg mathfun_funct_y1(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = y1(args[0].number) };
}

static mathfun_reg mathfun_funct_yn(const mathfun_reg args[]) {
	return (mathfun_reg){ .number = yn((int)args[0].number, args[1].number) };
}

static mathfun_reg mathfun_funct_sign(const mathfun_reg args[]) {
	const double x = args[0].number;
	return (mathfun_reg){ .number = isnan(x) || x == 0.0 ? x : copysign(1.0, x) };
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
