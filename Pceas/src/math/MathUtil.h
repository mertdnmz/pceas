/*
 * MathUtil.h
 *
 *  Created on: Mar 29, 2017
 *      Author: m3r7
 */

#ifndef MATH_MATHUTIL_H_
#define MATH_MATHUTIL_H_

#include <fmpz.h>
#include <fmpz_mod_poly.h>
#include <mutex>
#include "../core/Pceas.h"

namespace pceas {

class MathUtil {
public:
	MathUtil(ulong seed);
	virtual ~MathUtil();

	void sampleUnivariate(fmpz_mod_poly_t& poly, fmpz_t const& coeffZero, ulong degree);
	static void zeroUnivariate(fmpz_mod_poly_t& poly, fmpz_t const& coeffZero);
	static std::string fmpzToStr(fmpz_t const& c);
	static bool degreeCheckEQ(fmpz_mod_poly_t const& poly, ulong requiredDegree);
	static bool degreeCheckLTE(fmpz_mod_poly_t const& poly, ulong requiredDegree);
	flint_rand_t& getRandState();

private:
	flint_rand_t state;
	static std::mutex mut;
};

} /* namespace pceas */

#endif /* MATH_MATHUTIL_H_ */
