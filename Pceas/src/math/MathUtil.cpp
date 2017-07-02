/*
 * MathUtil.cpp
 *
 *  Created on: Mar 29, 2017
 *      Author: m3r7
 */

#include "MathUtil.h"
#include <random>

namespace pceas {

MathUtil::MathUtil(ulong seed) {
	flint_randinit(state);
	std::random_device r;
	ulong rngSeed1 = seed;//we use Party ID for seed 1
#ifdef NO_RANDOM
	ulong rngSeed2 = 0;
#else
	ulong rngSeed2 = r();
#endif
	flint_randseed(state, rngSeed1, rngSeed2);
}

MathUtil::~MathUtil() {
	flint_randclear(state);
}

/**
 * Prepare a polynomial such that :
 * 	1. Coefficient of ^0 term (the secret being shared) is 'coeffZero'
 * 	2. Other coefficients are random
 * Relies on Flint for random generation
 * (Flint uses a linear congruential generator)
 */
void MathUtil::sampleUnivariate(fmpz_mod_poly_t& poly, fmpz_t const& coeffZero, ulong degree) {
	//modulus : fmpz_mod_poly_modulus(poly)
	fmpz_t temp;
	fmpz_init(temp);
	fmpz_mod_poly_zero(poly);
	//use fmpz_mod_poly_randtest_monic to get random nonzero integers for coefficients
	fmpz_mod_poly_randtest_monic(poly, state, degree+1);//coeff. of x^T term = 1, others random
	fmpz_mod_poly_get_coeff_fmpz(temp, poly, 0);//temp = coeff. of x^0 term
	if (fmpz_is_zero(temp) == 1) {
		fmpz_add_ui(temp, temp, 1);
	}
	fmpz_mod_poly_set_coeff_fmpz(poly, degree, temp);//set coeff. of x^T term to temp
	fmpz_mod_poly_set_coeff_fmpz(poly, 0, coeffZero);//set coeff. of x^0 term to coeffZero
#ifdef VERBOSE
	flint_printf("Sampled univariate :\n");
	fmpz_mod_poly_print_pretty(poly, "x");
	flint_printf("\n");
#endif
	fmpz_clear(temp);
}

/**
 * Prepare a polynomial such that :
 * 	1. Coefficient of ^0 term (the secret being shared) is 'coeffZero'
 * 	2. Other coefficients are 0
 */
void MathUtil::zeroUnivariate(fmpz_mod_poly_t& poly, fmpz_t const& coeffZero) {
	std::lock_guard<std::mutex> guard(mut);
	fmpz_mod_poly_zero(poly);
	fmpz_mod_poly_set_coeff_fmpz(poly, 0, coeffZero);
}

std::string MathUtil::fmpzToStr(fmpz_t const& c) {
	std::lock_guard<std::mutex> guard(mut);
	const int BASE = 10;//fmpz_get_str accepts up to base 62, inclusive.
	char* temp = nullptr;
	temp = fmpz_get_str(temp, BASE, c);
	return std::string(temp);
}

bool MathUtil::degreeCheckEQ(fmpz_mod_poly_t const& poly, ulong requiredDegree) {
	std::lock_guard<std::mutex> guard(mut);
	slong degree = fmpz_mod_poly_degree(poly);
	bool dZero = (degree == -1 || degree == 0);
	return (dZero && requiredDegree == 0) || (degree == requiredDegree);//safe to compare signed/unsigned here because only neg. value fmpz_mod_poly_degree returns is -1(for 0-polynomial)
}

bool MathUtil::degreeCheckLTE(fmpz_mod_poly_t const& poly, ulong requiredDegree) {
	std::lock_guard<std::mutex> guard(mut);
	slong degree = fmpz_mod_poly_degree(poly);
	bool dZero = (degree == -1 || degree == 0);
	return (dZero && requiredDegree >= 0) || (requiredDegree >= degree);//safe to compare signed/unsigned here because only neg. value fmpz_mod_poly_degree returns is -1(for 0-polynomial)
}

flint_rand_t& MathUtil::getRandState() {
	return state;
}

std::mutex MathUtil::mut;

} /* namespace pceas */
