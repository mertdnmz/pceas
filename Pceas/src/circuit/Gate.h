/*
 * Gate.h
 *
 *  Created on: Feb 9, 2017
 *      Author: m3r7
 */

#ifndef GATE_H_
#define GATE_H_

#include <vector>
#include <unordered_set>
#include "Wire.h"

namespace pceas {

enum GateType {
	ADD,
	CONST_MULT,
	MULT
};
class Gate {
	friend class CircuitGenerator;
	friend class Circuit;
public:
	Gate(GateNumber gateNumber);
	virtual ~Gate();

	virtual GateType getType() const = 0;
	virtual void localCompute() = 0;

	/**
	 * Return true if the gate is computable, i.e. if all input wires have values assigned.
	 */
	bool isReady() const;

	/**
	 * If output wire(s) is/are set a value, return true.
	 */
	bool isProcessed() const;

	/**
	 * Result is assigned to output wires, and input wires of following gates
	 */
	void assignResult(fmpz_t const& result);
	/**
	 * Result is assigned to output wires, and input wires of following gates
	 */
	void assignResult(CommitmentId const& result);

	bool assignInput(fmpz_t const& val, std::string label);
	bool assignInput(CommitmentId const& cid, std::string label);

	fmpz_t& getLocalResult() {
		return localResult;
	}
	CommitmentId const& getOutputCid() const;

	bool isInputGate() const;
	bool isOutputGate() const;
	unsigned long getInputCount() const;
	unsigned long getOutputCount() const;
	std::unordered_set<std::string> getLabels() const;

	GateNumber getGateNumber() const {
		return gateNumber;
	}

	static constexpr const GateNumber NO_GATE = 0;//reserved

private:
	GateNumber gateNumber;
	void addInputWire(Wire* w);//circuit generation
	void addOutputWire(Wire* w);//circuit generation
	Wire* getEmptyInputWire();//circuit generation
	Wire* getEmptyOutputWire();//circuit generation
	Wire* getInputWireWithLabel(std::string label);//circuit generation
protected:
	fmpz_t localResult;
	// A gate has at least one input wire
	std::vector<Wire*> inputs;
	// and one or more output wires
	std::vector<Wire*> outputs;
};

} /* namespace pceas */

#endif /* GATE_H_ */
