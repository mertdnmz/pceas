/*
 * Circuit.h
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#ifndef CIRCUIT_H_
#define CIRCUIT_H_

#include <vector>
#include "Gate.h"

using namespace std;

namespace pceas {

class Circuit {
	friend class CircuitGenerator;
public:
	Circuit();
	virtual ~Circuit();

	Gate* getNext();
	void sortGates();
	unsigned long getInputCount() const;
	unordered_set<string> getLabels() const;
	unsigned long getOutputCount() const;
	void assignInput(fmpz_t const& val, string label);
	fmpz_t const& retrieveOutput() const;
	void assignInputCid(CommitmentId const& cid, string label);
	CommitmentId const& retrieveOutputCid() const;
	const vector<Gate*>& getGates() const {
		return gates;
	}
	void addGate(Gate* g);
private:
	vector<Gate*> gates;
	Gate* getOutputGate();
	bool hasGate(Gate* g) const;
	Gate* getInputGateWithLabel(string label);
};

} /* namespace pceas */

#endif /* CIRCUIT_H_ */
