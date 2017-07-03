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
