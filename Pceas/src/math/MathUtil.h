/**************************************************************************************
**
** Copyright (C) 2017 Mert Dönmez
**
** This file is part of PCEAS
**
** PCEAS is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** PCEAS is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with PCEAS.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
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
