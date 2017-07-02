/*
 * SymmBivariatePoly.cpp
 *
 *  Created on: Feb 18, 2017
 *      Author: m3r7
 */

#include "SymmBivariatePoly.h"
#include "MathUtil.h"

namespace pceas {

SymmBivariatePoly::SymmBivariatePoly(fmpz_t const& mod, ulong degree, ulong pid) {
	fmpz_init(n);
	fmpz_set(n, mod);
	if (fmpz_cmp_ui(mod, 1) < 0) {//mod < 1
		throw std::runtime_error("Invalid parameter.");
	}
	t = degree;
	coeff = new fmpz*[t+1];//t+1 rows of varying length. Due to symmetry, we only need (t+1)*(t+2) / 2 coefficients. (Lower half & diagonal of a symmetric (t+1)x(t+1) matrix)
	for (ulong i = 0; i <= t; ++i) {//allocate rows
		/**
		 * First row will have 1 element, second row 2 elements, ...
		 */
		coeff[i] = _fmpz_vec_init(1+i);
	}
}

SymmBivariatePoly::~SymmBivariatePoly() {
	fmpz_clear(n);
	for (ulong i = 0; i <= t; ++i) {//deallocate rows
		_fmpz_vec_clear(coeff[i], 1+i);
	}
	delete[] coeff;
}

/**
 * Set fk_x to f(x, k)
 */
void SymmBivariatePoly::evaluate(fmpz_mod_poly_t& fk_x, fmpz_t const& k) {
	fmpz_mod_poly_zero(fk_x);
	fmpz_t temp;
	fmpz_init(temp);
	for (ulong i = 0; i <= t; ++i) {
		/**
		 * When evaluating a bivariate polynomial with one variable fixed,
		 * it can be regarded as a single variable polynomial, where each
		 * coefficient is another single variable polynomial evaluated at
		 * the fixed value :
		 * ƩƩ coeff_i_j x^i y^j = (Ʃcoeff_0_j k^j) x^0 + (Ʃcoeff_1_j k^j) x^1 + ...
		 */
		fmpz_mod_poly_t poly;//polynomial to be evaluated at k
		fmpz_mod_poly_init(poly, n);
		for (ulong j = 0; j <= t; ++j) {
			getCoeff(i,j, temp);
			fmpz_mod_poly_set_coeff_fmpz(poly, j, temp); //set coeff of y^j (or k^j) term
		}
		fmpz_mod_poly_evaluate_fmpz(temp, poly, k);
		fmpz_mod_poly_set_coeff_fmpz(fk_x, i, temp);
		fmpz_mod_poly_clear(poly);
	}
	fmpz_clear(temp);

#ifdef VERBOSE
	std::lock_guard<std::mutex> guard(mut);
	flint_printf("Bivariate evaluated at k = ");
	flint_printf(MathUtil::fmpzToStr(k).c_str());
	flint_printf(" : ");
	fmpz_mod_poly_print_pretty(fk_x, "x");
	flint_printf("\n");
#endif
}

void SymmBivariatePoly::evaluate(fmpz_mod_poly_t& fk_x, ulong k) {
	fmpz_t temp;
	fmpz_init(temp);
	fmpz_set_ui(temp, k);
	evaluate(fk_x, temp);
	fmpz_clear(temp);
}

void SymmBivariatePoly::evaluateAtZero(fmpz_mod_poly_t& fk_x) {
	evaluate(fk_x, 0);
}

/**
 * Set fkl to f(k, l)
 */
void SymmBivariatePoly::evaluate(fmpz_t& fkl, fmpz_t const& k, fmpz_t const& l) {
	fmpz_mod_poly_t poly;
	fmpz_mod_poly_init(poly, n);
	evaluate(poly, l); // first evaluate f(x, l)
	fmpz_mod_poly_evaluate_fmpz(fkl, poly, k); // evaluate f(k, l)
	fmpz_mod_poly_clear(poly);

#ifdef VERBOSE
	std::lock_guard<std::mutex> guard(mut);
	flint_printf("Bivariate evaluated at (k , l) = ");
	flint_printf(MathUtil::fmpzToStr(k).c_str());
	flint_printf(" , ");
	flint_printf(MathUtil::fmpzToStr(k).c_str());
	flint_printf(" : ");
	flint_printf(MathUtil::fmpzToStr(fkl).c_str());
	flint_printf("\n");
#endif
}

void SymmBivariatePoly::evaluate(fmpz_t& fkl, ulong k, ulong l) {
	fmpz_t temp1, temp2;
	fmpz_init(temp1);
	fmpz_init(temp2);
	fmpz_set_ui(temp1, k);
	fmpz_set_ui(temp2, l);
	evaluate(fkl, temp1, temp2);
	fmpz_clear(temp1);
	fmpz_clear(temp2);
}

/**
 * Generates random coefficients for a
 * degree t symmetric bivariate polynomial.
 * coeff_0_0 will be 'coeffZero'.
 * coeff_t_t will be nonzero
 * Relies on Flint for random generation
 * (Flint uses a linear congruential generator)
 */
void SymmBivariatePoly::sampleBivariate(fmpz_t const& coeffZero, flint_rand_t& randState) {
	const ulong COEFF_COUNT = (t+1)*(t+2)/2;
	fmpz_t temp;
	fmpz_init(temp);
	fmpz_mod_poly_t poly;
	fmpz_mod_poly_init(poly, n);
	//use fmpz_mod_poly_randtest_monic to get random nonzero integers for coefficients
	fmpz_mod_poly_randtest_monic(poly, randState, COEFF_COUNT+1);//coeff. of x^COEFF_COUNT term = 1, other 'COEFF_COUNT' coefficients are random
	ulong term = 0;
	for (ulong row = 0; row <= t; ++row) {
		for (ulong column = 0; column <= row; ++column) {
			fmpz_mod_poly_get_coeff_fmpz(temp, poly, term++);
			if (row == t && column == t) {
				if (fmpz_is_zero(temp) == 1) {
					fmpz_add_ui(temp, temp, 1);//ensure that degree is 2T
				}
			}
			setCoeff(row, column, temp);
		}
	}
	setCoeff(0, 0, coeffZero);
	fmpz_clear(temp);
	fmpz_mod_poly_clear(poly);

#ifdef VERBOSE
	std::lock_guard<std::mutex> guard(mut);
	printCoeff();
#endif
}

void SymmBivariatePoly::setCoeff(ulong row, ulong column, fmpz_t const& val) {
	if (column <= row) {
		fmpz_set(coeff[row] + column, val);
	} else {
		fmpz_set(coeff[column] + row, val);
	}
}

void SymmBivariatePoly::getCoeff(ulong row, ulong column, fmpz_t& val) const {
	if (column <= row) {
		fmpz_set(val, coeff[row] + column);
	} else {
		fmpz_set(val, coeff[column] + row);
	}
}

#ifdef VERBOSE
void SymmBivariatePoly::printCoeff() const {
	fmpz_t temp;
	fmpz_init(temp);
	for (ulong row = 0; row <= t; ++row) {
		for (ulong column = 0; column <= t; ++column) {
			getCoeff(row, column, temp);
			std::cout << MathUtil::fmpzToStr(temp) << "\t";
		}
		std::cout << std::endl;
	}
	fmpz_clear(temp);
}
#endif

} /* namespace pceas */
