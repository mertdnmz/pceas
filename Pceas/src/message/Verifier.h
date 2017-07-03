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
 * Verifier.h
 *
 *  Created on: Mar 8, 2017
 *      Author: m3r7
 */

#ifndef VERIFIER_H_
#define VERIFIER_H_

#include <fmpz.h>

namespace pceas {

struct Verifier {
public:
	Verifier();
	virtual ~Verifier();
	fmpz_t vf;
};

} /* namespace pceas */

#endif /* VERIFIER_H_ */
