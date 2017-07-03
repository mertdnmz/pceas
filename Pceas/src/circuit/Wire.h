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
 * Wire.h
 *
 *  Created on: Feb 14, 2017
 *      Author: m3r7
 */

#ifndef WIRE_H_
#define WIRE_H_

#include <stdexcept>
#include "../core/Pceas.h"

namespace pceas {

class Gate;
class Wire {
	friend class CircuitGenerator;
public:
	Wire();
	virtual ~Wire();

	bool isAssigned() const {
		return assigned;
	}

	const Gate* getNext() const {
		return next;
	}

	void setNext(Gate* next) {
		this->next = next;
	}

	const Gate* getPrev() const {
		return prev;
	}

	void setPrev(Gate* prev) {
		this->prev = prev;
	}

	fmpz_t const& getValue() {
		return value;
	}

	void setValue(const fmpz_t val) {
		fmpz_set(value, val);
		assigned = true;
	}

	void setValue(const ulong val) {
		fmpz_set_ui(value, val);
		assigned = true;
	}

	const CommitmentId& getCid() const {
		return cid;
	}

	void setCid(const CommitmentId& cid) {
		this->cid = cid;
		assigned = true;
	}

	const std::string& getInputLabel() const {
		return inputLabel;
	}

	void setInputLabel(const std::string& inputLabel) {
		if (prev != nullptr) {
			throw std::runtime_error("Tried to assign label to non-input wire.");
		}
		if (inputLabel.empty()) {
			throw std::runtime_error("Empty label not allowed.");
		}
		this->inputLabel = inputLabel;
	}

private:

	/*
	 * A wire is assigned either values as in Protocol 'CEPS' (Circuit Evaluation with Passive Security) implementation,
	 * or commitments as in Protocol 'CEAS' (Circuit Evaluation with Active Security) implementation.
	 */
	fmpz_t value;
	CommitmentId cid;

	bool assigned;
	Gate* prev; // pointer to gate, which outputs on this wire
	Gate* next; // pointer to gate, which take input from this wire

	std::string inputLabel;// used to match inputs to input wires (alternative to matching via ordering)
};

} /* namespace pceas */

#endif /* WIRE_H_ */
