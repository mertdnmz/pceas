/*
 * Circuit.cpp
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#include <algorithm>
#include "Circuit.h"
#include "../core/PceasException.h"

namespace pceas {

Circuit::Circuit() {
}

Circuit::~Circuit() {
	for (auto& g : gates) {
		delete g;
	}
}

/**
 * Returns next gate to be computed :
 * Gate with smallest gate number, which has not been computed yet (output wire not assigned value yet),
 * but is computable (all input wires have values assigned)
 */
Gate* Circuit::getNext() {
	for (auto& g : gates) {
		if (!g->isProcessed() && g->isReady()) {
			return g;
		}
	}
	return nullptr;
}

void Circuit::addGate(Gate* g) {
	gates.push_back(g);
}

void Circuit::sortGates() {
	sort(gates.begin(), gates.end(), [](Gate* g1, Gate* g2){return g1->getGateNumber() < g2->getGateNumber();});
}

/**
 * returns the number of required inputs, which may be different
 * than number of open input wires (i.e. the number of arguments
 * for the function we want to evaluate), because one secret may
 * be assigned multiple times to different wires.
 */
unsigned long Circuit::getInputCount() const {
	return getLabels().size();
}

unordered_set<string> Circuit::getLabels() const {
	unordered_set<string> labels;
	for (auto const& g : gates) {
		auto const& l = g->getLabels();
		labels.insert(l.begin(), l.end());
	}
	return labels;
}

/**
 * returns number of open output wires,
 */
unsigned long Circuit::getOutputCount() const {
	unsigned long count = 0;
	for (auto const& g : gates) {
		count += g->getOutputCount();
	}
	return count;
}

/**
 * Sets value 'val' to an open input wire with matching label.
 */
void Circuit::assignInput(const fmpz_t& val, string label) {
	bool assigned = false;
	for (auto& g : gates) {
		if (g->assignInput(val, label)) {
			assigned = true;//continue, label might occur multiple times
		}
	}
	if (!assigned) {
		throw PceasException("Could not assign input.");
	}
}

const fmpz_t& Circuit::retrieveOutput() const {
	for (auto& g : gates) {
		if (!g->isProcessed()) {
			throw PceasException("There are unprocessed gates.");
		}
		//note:we assume single output gate (enforced in Party::sanityChecks())
		if (g->isOutputGate()) {
			return g->getLocalResult();
		}
	}
	throw PceasException("No output gate.");
}

/**
 * Sets commitment with ID 'cid' to an open input wire with matching label.
 */
void Circuit::assignInputCid(const CommitmentId& cid, string label) {
	bool assigned = false;
	for (auto& g : gates) {
		if (g->assignInput(cid, label)) {
			assigned = true;//continue, label might occur multiple times
		}
	}
	if (!assigned) {
		throw PceasException("Could not assign input.");
	}
}

const CommitmentId& Circuit::retrieveOutputCid() const {
	for (auto& g : gates) {
		if (!g->isProcessed()) {
			throw PceasException("There are unprocessed gates.");
		}
		//note:we assume single output gate (enforced in Party::sanityChecks())
		if (g->isOutputGate()) {
			return g->getOutputCid();
		}
	}
	throw PceasException("No output gate.");
}

Gate* Circuit::getOutputGate() {
	for (auto& g : gates) {
		if (g->isOutputGate()) {
			return g;
		}
	}
	return nullptr;
}

bool Circuit::hasGate(Gate* g) const {
	for (auto const& gg : gates) {
		if (g == gg) {
			return true;
		}
	}
	return false;
}

Gate* Circuit::getInputGateWithLabel(string label) {
	for (auto& g : gates) {
		if (g->getInputWireWithLabel(label)) {
			return g;
		}
	}
	throw PceasException("No gate with matching label.");
}

} /* namespace pceas */
