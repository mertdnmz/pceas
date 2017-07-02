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
