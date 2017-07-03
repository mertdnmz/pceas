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
 * VerifiableShare.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: m3r7
 */

#include "VerifiableShare.h"

namespace pceas {

VerifiableShare::VerifiableShare(fmpz_t const& mod) {
	fmpz_mod_poly_init(fkx, mod);
	fmpz_mod_poly_zero(fkx);
	k = 0;
}

VerifiableShare::~VerifiableShare() {
	fmpz_mod_poly_clear(fkx);
}

} /* namespace pceas */
