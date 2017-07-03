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
 * VerifiableShare.h
 *
 *  Created on: Feb 21, 2017
 *      Author: m3r7
 */

#ifndef VERIFIABLESHARE_H_
#define VERIFIABLESHARE_H_

#include <memory>
#include "../core/Pceas.h"
#include <fmpz_mod_poly.h>

namespace pceas {

struct VerifiableShare {
public:
	VerifiableShare(fmpz_t const& mod);
	virtual ~VerifiableShare();
	fmpz_mod_poly_t fkx;
	PartyId k;
};

typedef std::shared_ptr<VerifiableShare> VerifiableSharePtr;

} /* namespace pceas */

#endif /* VERIFIABLESHARE_H_ */
