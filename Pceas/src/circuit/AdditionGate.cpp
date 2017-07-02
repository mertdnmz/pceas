/*
 * AdditionGate.cpp
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#include "AdditionGate.h"

namespace pceas {

AdditionGate::AdditionGate(GateNumber gateNumber) : Gate(gateNumber) {
}

AdditionGate::~AdditionGate() {
}

void AdditionGate::localCompute() {
	fmpz_add(localResult, inputs.at(0)->getValue(), inputs.at(1)->getValue());
}

CommitmentId AdditionGate::getInputCid1() const {
	return inputs.at(0)->getCid();
}

CommitmentId AdditionGate::getInputCid2() const {
	return inputs.at(1)->getCid();
}

} /* namespace pceas */
