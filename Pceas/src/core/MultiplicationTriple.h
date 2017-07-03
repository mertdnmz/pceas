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
 * MultiplicationTriple.h
 *
 *  Created on: Mar 26, 2017
 *      Author: m3r7
 */

#ifndef MULTIPLICATIONTRIPLE_H_
#define MULTIPLICATIONTRIPLE_H_

#include "CommitmentRecord.h"

namespace pceas {

class MultiplicationTriple {
public:
	MultiplicationTriple();
	virtual ~MultiplicationTriple();

	CommitmentRecord* firstMult;
	CommitmentRecord* secontMult;
	CommitmentRecord* product;
	vector<CommitmentRecord*> receivedShares;

	static constexpr const char* M1 = "first_mul";
	static constexpr const char* M2 = "second_mul";
	static constexpr const char* PROD = "product";
	static constexpr const char* E = "e";
	static constexpr const char* D = "d";
};

} /* namespace pceas */

#endif /* MULTIPLICATIONTRIPLE_H_ */
