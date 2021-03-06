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
 * SymmBivariatePoly.h
 *
 *  Created on: Feb 18, 2017
 *      Author: m3r7
 */

#ifndef SYMMBIVARIATEPOLY_H_
#define SYMMBIVARIATEPOLY_H_

#include <fmpz.h>
#include <fmpz_mod_poly.h>
#include <mutex>
#include "../core/Pceas.h"

namespace pceas {

/**
 * Implements symmetric bivariate polynomials over Z/nZ
 * on top of Flint library's 'polynomials over Z/nZ'
 *
 * We will need symmetric bivariate polynomials for redundant sharing,
 * which in turn is used for achieving (perfect) commitment.
 */
class SymmBivariatePoly {
public:
	SymmBivariatePoly(fmpz_t const& mod, ulong degree, ulong seed);
	virtual ~SymmBivariatePoly();

	void evaluateAtZero(fmpz_mod_poly_t& fk_x);
	void evaluate(fmpz_mod_poly_t& fk_x, fmpz_t const& k);
	void evaluate(fmpz_mod_poly_t& fk_x, ulong k);
	void evaluate(fmpz_t& fkl, fmpz_t const& k, fmpz_t const& l);
	void evaluate(fmpz_t& fkl, ulong k, ulong l);
	void sampleBivariate(fmpz_t const& coeffZero, flint_rand_t& randState);

private:
	fmpz_t n; // Z/nZ
	ulong t; // degree of the polynomial
	/**
	 * A 2-dim array to hold the coefficients.
	 */
	fmpz** coeff;

	void setCoeff(ulong row, ulong column, fmpz_t const& val);
	void getCoeff(ulong row, ulong column, fmpz_t& val) const;

#ifdef VERBOSE
	void printCoeff() const;
	std::mutex mut;
#endif
};

} /* namespace pceas */

#endif /* SYMMBIVARIATEPOLY_H_ */
