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
	const mathfun_decl decls[] = {

		// Constants
		{ MATHFUN_DECL_CONST, "e",         { .value = M_E } },
		{ MATHFUN_DECL_CONST, "log2e",     { .value = M_LOG2E } },
		{ MATHFUN_DECL_CONST, "log10e",    { .value = M_LOG10E } },
		{ MATHFUN_DECL_CONST, "ln2",       { .value = M_LN2 } },
		{ MATHFUN_DECL_CONST, "ln10",      { .value = M_LN10 } },
		{ MATHFUN_DECL_CONST, "pi",        { .value = M_PI } },
		{ MATHFUN_DECL_CONST, "tau",       { .value = M_TAU } },
		{ MATHFUN_DECL_CONST, "pi_2",      { .value = M_PI_2 } },
		{ MATHFUN_DECL_CONST, "pi_4",      { .value = M_PI_4 } },
		{ MATHFUN_DECL_CONST, "_1_pi",     { .value = M_1_PI } },
		{ MATHFUN_DECL_CONST, "_2_pi",     { .value = M_2_PI } },
		{ MATHFUN_DECL_CONST, "_2_sqrtpi", { .value = M_2_SQRTPI } },
		{ MATHFUN_DECL_CONST, "sqrt2",     { .value = M_SQRT2 } },
		{ MATHFUN_DECL_CONST, "sqrt1_2",   { .value = M_SQRT1_2 } },

		// Functions
		{ MATHFUN_DECL_FUNCT, "isnan",          { .funct = { mathfun_funct_isnan,          &mathfun_bsig1 } } },
		{ MATHFUN_DECL_FUNCT, "isfinite",       { .funct = { mathfun_funct_isfinite,       &mathfun_bsig1 } } },
		{ MATHFUN_DECL_FUNCT, "isnormal",       { .funct = { mathfun_funct_isnormal,       &mathfun_bsig1 } } },
		{ MATHFUN_DECL_FUNCT, "isinf",          { .funct = { mathfun_funct_isinf,          &mathfun_bsig1 } } },
		{ MATHFUN_DECL_FUNCT, "isgreater",      { .funct = { mathfun_funct_isgreater,      &mathfun_bsig2 } } },
		{ MATHFUN_DECL_FUNCT, "isgreaterequal", { .funct = { mathfun_funct_isgreaterequal, &mathfun_bsig2 } } },
		{ MATHFUN_DECL_FUNCT, "isless",         { .funct = { mathfun_funct_isless,         &mathfun_bsig2 } } },
		{ MATHFUN_DECL_FUNCT, "islessequal",    { .funct = { mathfun_funct_islessequal,    &mathfun_bsig2 } } },
		{ MATHFUN_DECL_FUNCT, "islessgreater",  { .funct = { mathfun_funct_islessgreater,  &mathfun_bsig2 } } },
		{ MATHFUN_DECL_FUNCT, "isunordered",    { .funct = { mathfun_funct_isunordered,    &mathfun_bsig2 } } },
		{ MATHFUN_DECL_FUNCT, "signbit",        { .funct = { mathfun_funct_signbit,        &mathfun_bsig2 } } },
		{ MATHFUN_DECL_FUNCT, "acos",           { .funct = { mathfun_funct_acos,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "acosh",          { .funct = { mathfun_funct_acosh,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "asin",           { .funct = { mathfun_funct_asin,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "asinh",          { .funct = { mathfun_funct_asinh,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "atan",           { .funct = { mathfun_funct_atan,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "atan2",          { .funct = { mathfun_funct_atan2,          &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "atanh",          { .funct = { mathfun_funct_atanh,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "cbrt",           { .funct = { mathfun_funct_cbrt,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "ceil",           { .funct = { mathfun_funct_ceil,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "copysign",       { .funct = { mathfun_funct_copysign,       &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "cos",            { .funct = { mathfun_funct_cos,            &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "cosh",           { .funct = { mathfun_funct_cosh,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "erf",            { .funct = { mathfun_funct_erf,            &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "erfc",           { .funct = { mathfun_funct_erfc,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "exp",            { .funct = { mathfun_funct_exp,            &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "exp2",           { .funct = { mathfun_funct_exp2,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "expm1",          { .funct = { mathfun_funct_expm1,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "abs",            { .funct = { mathfun_funct_abs,            &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "fdim",           { .funct = { mathfun_funct_fdim,           &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "floor",          { .funct = { mathfun_funct_floor,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "fma",            { .funct = { mathfun_funct_fma,            &mathfun_sig3 } } },
		{ MATHFUN_DECL_FUNCT, "fmod",           { .funct = { mathfun_funct_fmod,           &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "max",            { .funct = { mathfun_funct_max,            &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "min",            { .funct = { mathfun_funct_min,            &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "hypot",          { .funct = { mathfun_funct_hypot,          &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "j0",             { .funct = { mathfun_funct_j0,             &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "j1",             { .funct = { mathfun_funct_j1,             &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "jn",             { .funct = { mathfun_funct_jn,             &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "ldexp",          { .funct = { mathfun_funct_ldexp,          &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "log",            { .funct = { mathfun_funct_log,            &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "log10",          { .funct = { mathfun_funct_log10,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "log1p",          { .funct = { mathfun_funct_log1p,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "log2",           { .funct = { mathfun_funct_log2,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "logb",           { .funct = { mathfun_funct_logb,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "nearbyint",      { .funct = { mathfun_funct_nearbyint,      &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "nextafter",      { .funct = { mathfun_funct_nextafter,      &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "nexttoward",     { .funct = { mathfun_funct_nexttoward,     &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "remainder",      { .funct = { mathfun_funct_remainder,      &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "round",          { .funct = { mathfun_funct_round,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "scalbln",        { .funct = { mathfun_funct_scalbln,        &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "sin",            { .funct = { mathfun_funct_sin,            &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "sinh",           { .funct = { mathfun_funct_sinh,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "sqrt",           { .funct = { mathfun_funct_sqrt,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "tan",            { .funct = { mathfun_funct_tan,            &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "tanh",           { .funct = { mathfun_funct_tanh,           &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "gamma",          { .funct = { mathfun_funct_gamma,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "trunc",          { .funct = { mathfun_funct_trunc,          &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "y0",             { .funct = { mathfun_funct_y0,             &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "y1",             { .funct = { mathfun_funct_y1,             &mathfun_sig1 } } },
		{ MATHFUN_DECL_FUNCT, "yn",             { .funct = { mathfun_funct_yn,             &mathfun_sig2 } } },
		{ MATHFUN_DECL_FUNCT, "sign",           { .funct = { mathfun_funct_sign,           &mathfun_sig1 } } },

		{ -1, NULL, { .value = 0 } }
	};

	return mathfun_context_define(ctx, decls, error);
}
