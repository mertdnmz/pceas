/*
 * ConstantMultGate.cpp
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#include "ConstantMultGate.h"

namespace pceas {

ConstantMultGate::ConstantMultGate(GateNumber gateNumber, fmpz_t const& c) : Gate(gateNumber) {
	fmpz_init(constant);
	fmpz_set(constant, c);
}

ConstantMultGate::ConstantMultGate(GateNumber gateNumber, long c) : Gate(gateNumber) {
	fmpz_init(constant);
	fmpz_set_si(constant, c);
}

ConstantMultGate::~ConstantMultGate() {
	fmpz_clear(constant);
}

void ConstantMultGate::localCompute() {
	fmpz_mul(localResult, inputs.at(0)->getValue(), constant);
}

CommitmentId ConstantMultGate::getInputCid() const {
	return inputs.at(0)->getCid();
}

} /* namespace pceas */
