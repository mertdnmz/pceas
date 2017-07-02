/*
 * AdditionGate.h
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#ifndef ADDITIONGATE_H_
#define ADDITIONGATE_H_

#include "Gate.h"

namespace pceas {

class AdditionGate: public Gate {
public:
	AdditionGate(GateNumber gateNumber);
	virtual ~AdditionGate();

	GateType getType() const {
		return ADD;
	}
	void localCompute();
	CommitmentId getInputCid1() const;
	CommitmentId getInputCid2() const;
};

} /* namespace pceas */

#endif /* ADDITIONGATE_H_ */
