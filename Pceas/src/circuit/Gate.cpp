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
 * Gate.cpp
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#include "Gate.h"

namespace pceas {

Gate::Gate(GateNumber gateNumber) : gateNumber(gateNumber) {
	if (gateNumber == NO_GATE) {
		throw std::runtime_error("Bad gate number.");
	}
	fmpz_init_set_ui(localResult, 0);
}

Gate::~Gate() {
	for (auto& w : outputs) {
		delete w;
	}
	for (auto& w : inputs) {
		delete w;
	}
	fmpz_clear(localResult);
}

bool Gate::isReady() const {
	for (auto& w : inputs) {
		if (!w->isAssigned()) {
			return false;
		}
	}
	return true;
}

bool Gate::isProcessed() const {
	return outputs.back()->isAssigned(); //we check only one of the outputs wires. all outputs wires are assigned values, or none.
}

void Gate::assignResult(fmpz_t const& result) {
	for (auto& w : outputs) {
		w->setValue(result);
		if (w->getNext() != nullptr) {
			for (auto& iw : w->getNext()->inputs) {
				if (iw->getPrev() == this) {
					iw->setValue(result);
				}
			}
		}
	}
}

void Gate::assignResult(CommitmentId const& result) {
	for (auto& w : outputs) {
		w->setCid(result);
		if (w->getNext() != nullptr) {
			for (auto& iw : w->getNext()->inputs) {
				if (iw->getPrev() == this) {
					iw->setCid(result);
				}
			}
		}
	}
}

bool Gate::assignInput(fmpz_t const& val, std::string label) {
	bool assigned = false;
	for (auto& w : inputs) {
		if (w->getPrev() == nullptr && !w->isAssigned() && w->getInputLabel() == label) {
			w->setValue(val);
			assigned = true;//continue, label might occur multiple times
		}
	}
	return assigned;
}

bool Gate::assignInput(CommitmentId const& cid, std::string label) {
	bool assigned = false;
	for (auto& w : inputs) {
		if (w->getPrev() == nullptr && !w->isAssigned() && w->getInputLabel() == label) {
			w->setCid(cid);
			assigned = true;//continue, label might occur multiple times
		}
	}
	return assigned;
}

void Gate::addInputWire(Wire* w) {
	inputs.push_back(w);
	w->setNext(this);
	if (getType() == ADD || getType() == MULT) {
		if (inputs.size() > 2) {
			throw std::runtime_error("More input wires than allowed.");
		}
	} else if (getType() == CONST_MULT) {
		if (inputs.size() > 1) {
			throw std::runtime_error("More input wires than allowed.");
		}
	}
}

void Gate::addOutputWire(Wire* w) {
	outputs.push_back(w);
	w->setPrev(this);
}

Wire* Gate::getEmptyInputWire() {
	for (auto const& w : inputs) {
		if (w->getPrev() == nullptr && w->getInputLabel().empty()) {
			return w;
		}
	}
	return nullptr;
}

Wire* Gate::getEmptyOutputWire() {
	for (auto const& w : outputs) {
		if (w->getNext() == nullptr) {
			return w;
		}
	}
	return nullptr;
}

Wire* Gate::getInputWireWithLabel(std::string label) {
	for (auto& w : inputs) {
		if (w->getPrev() == nullptr && w->getInputLabel() == label) {
			return w;
		}
	}
	return nullptr;
}

bool Gate::isInputGate() const {
	for (auto const& w : inputs) {
		if (w->getPrev() == nullptr) {
			return true;
		}
	}
	return false;
}

bool Gate::isOutputGate() const {
	for (auto const& w : outputs) {
		if (w->getNext() == nullptr) {
			return true;
		}
	}
	return false;
}

unsigned long Gate::getInputCount() const {
	return getLabels().size();
}

unsigned long Gate::getOutputCount() const {
	unsigned long count = 0;
	for (auto const& w : outputs) {
		if (w->getNext() == nullptr) {
			count++;
		}
	}
	return count;
}

std::unordered_set<std::string> Gate::getLabels() const {
	std::unordered_set<std::string> labels;
	for (auto const& w : inputs) {
		if (w->getPrev() == nullptr) {
			labels.insert(w->getInputLabel());
		}
	}
	return labels;
}

CommitmentId const& Gate::getOutputCid() const {
	for (auto const& w : outputs) {
		if (w->getNext() == nullptr) {
			return w->getCid();
		}
	}
	throw std::runtime_error("Gate is not an output gate.");
}

} /* namespace pceas */

