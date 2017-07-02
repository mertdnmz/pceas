/*
 * MultiplicationGate.cpp
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#include "MultiplicationGate.h"

namespace pceas {

MultiplicationGate::MultiplicationGate(GateNumber gateNumber) : Gate(gateNumber) {
}

MultiplicationGate::~MultiplicationGate() {
}

void MultiplicationGate::localCompute() {
	fmpz_mul(localResult, inputs.at(0)->getValue(), inputs.at(1)->getValue());
}

CommitmentId MultiplicationGate::getInputCid1() const {
	return inputs.at(0)->getCid();
}

CommitmentId MultiplicationGate::getInputCid2() const {
	return inputs.at(1)->getCid();
}

} /* namespace pceas */
