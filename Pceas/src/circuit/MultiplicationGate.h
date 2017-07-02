/*
 * MultiplicationGate.h
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#ifndef MULTIPLICATIONGATE_H_
#define MULTIPLICATIONGATE_H_

#include "Gate.h"

namespace pceas {

class MultiplicationGate: public Gate {
public:
	MultiplicationGate(GateNumber gateNumber);
	virtual ~MultiplicationGate();

	GateType getType() const {
		return MULT;
	}
	void localCompute();
	CommitmentId getInputCid1() const;
	CommitmentId getInputCid2() const;
};

} /* namespace pceas */

#endif /* MULTIPLICATIONGATE_H_ */
