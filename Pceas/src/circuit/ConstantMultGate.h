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
 * ConstantMultGate.h
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#ifndef CONSTANTMULTGATE_H_
#define CONSTANTMULTGATE_H_

#include "Gate.h"

namespace pceas {

class ConstantMultGate: public Gate {
public:
	ConstantMultGate(GateNumber gateNumber, fmpz_t const& c);
	ConstantMultGate(GateNumber gateNumber, long c);
	virtual ~ConstantMultGate();

	GateType getType() const {
		return CONST_MULT;
	}
	void localCompute();
	fmpz_t const& getConstant() const {
		return constant;
	}
	CommitmentId getInputCid() const;

private:
	fmpz_t constant;
};

} /* namespace pceas */

#endif /* CONSTANTMULTGATE_H_ */
