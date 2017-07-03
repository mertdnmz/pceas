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
 * CircuitGenerator.cpp
 *
 *  Created on: Mar 27, 2017
 *      Author: m3r7
 */

#ifndef CIRCUITGENERATOR_CPP_
#define CIRCUITGENERATOR_CPP_

#include <string>
#include <vector>
#include "Circuit.h"
#include "ConstantMultGate.h"
#include "AdditionGate.h"
#include "MultiplicationGate.h"

using namespace std;

namespace pceas {

class CircuitGenerator {
public:
	CircuitGenerator() {
		c = nullptr;
		gn = Gate::NO_GATE;
		inputWire = nullptr;
		circuitExpression = nullptr;
	};
	virtual ~CircuitGenerator() {};
	/**
	 * Generates a circuit from a description string
	 * and returns a pointer to the generated circuit.
	 * Examples for description string :
	 * (a+b)*(c.2)  (hard-coded below, in method named 'makeTestCurcuit')
	 * (foo+b).2*c*d*(foo*c)
	 */
	Circuit* generate(string circuitDescription) {
		if (circuitDescription.empty()) {
			throw runtime_error("Empty description string.");
		}
		for(char const& ch : circuitDescription) {
			check(ch);
		}
		c = new Circuit();
		gn = 1;
		circuitExpression = circuitDescription.c_str();
	    expression();
	    return c;
	}
private:
	Circuit* c;
	GateNumber gn;
	Wire* inputWire;//temporarily holds a reference to an input wire
	const char* circuitExpression;

	const char ADD = '+';//add
	const char MUL = '*';//multiply
	const char CMUL = '.';//multiply with constant

	const char MINUS = '-';
	const char OPEN_PAR = '(';
	const char CLOSE_PAR = ')';

	/*
	 * BEGIN Simple recursive descent parser.
	 * Rules :
	 * 1. Numbers only occur after a '.' or inside labels
	 * 2. Labels start with an alphabetic lowercase character a-z and may also contain numeric characters. (For ex. 'a', 'abc1', 'abc2')
	 * 3. Can use same label multiple times (to have some input assigned to multiple wires,
	 *    possibly at different gates).
	 * 4. Symbols are consumed from left to right. Use paranthesis to order MUL and CMUL gates.
	 *    For example, '(a+b)*(c.2)' represents a curcuit different than '(a+b)*c.2' (but they
	 *    yield the same result).
	 * 5. Do not use whitespace characters.
	 */
	char peek() {
		return *circuitExpression;
	}

	char get() {
		return *circuitExpression++;
	}

	long number() { // extracts a numeric value (multiplier of a scalar multiplication gate)
		bool negative = (peek() == MINUS);
		if (negative) {
			get();// eat '-'
		}
		long result = get() - '0';
		while (peek() >= '0' && peek() <= '9') {
			result = 10 * result + get() - '0';
		}
		return negative ? -result : result;
	}

	Wire* label() { // extracts an input label
		string label = "";
		while ((peek() >= 'a' && peek() <= 'z') || (peek() >= '0' && peek() <= '9')) {
			label += get();
		}
		Wire* w = new Wire();
		w->setInputLabel(label);
		return w;
	}

	Gate* factor() {
		if (peek() >= '0' && peek() <= '9') {
			throw runtime_error("numbers can only follow .");
		} else if (peek() >= 'a' && peek() <= 'z') {
			inputWire = label();
			return nullptr;
		} else if (peek() == OPEN_PAR) {
			get();// eat '('
			Gate* g = expression();
			get();// eat ')'
			return g;
		}
		throw runtime_error("Unexpected character.");
	}

	Gate* term() {
		Gate* g1 = factor();
		Gate* g = nullptr;
		while (peek() == MUL || peek() == CMUL) {
			if (get() == MUL) {
				MultiplicationGate* mg = new MultiplicationGate(getNextGateNum());
				c->addGate(mg);
				g = mg;
				g->addOutputWire(new Wire());
				if (!g1) {//assign input wire
					g->addInputWire(getSavedInputWire());
				} else {//connect gates
					connectGates(g1, g);
				}
				Gate* g2 = factor();
				if (!g2) {//assign input wire
					g->addInputWire(getSavedInputWire());
				} else {//connect gates
					connectGates(g2, g);
				}
			} else {
				long scalar = number();
				ConstantMultGate* cmg = new ConstantMultGate(getNextGateNum(), scalar);
				c->addGate(cmg);
				g = cmg;
				g->addOutputWire(new Wire());
				if (!g1) {//assign input wire
					g->addInputWire(getSavedInputWire());
				} else {//connect gates
					connectGates(g1, g);
				}
				//we already 'got' the second factor via number()
			}
			g1 = g;
		}
		return g1;
	}

	Gate* expression() {
		Gate* g1 = term();
		Gate* g = nullptr;
		while (peek() == ADD) {
			if (get() == ADD) {
				AdditionGate* ag = new AdditionGate(getNextGateNum());
				c->addGate(ag);
				g = ag;
				g->addOutputWire(new Wire());
			}
			if (!g1) {//assign input wire
				g->addInputWire(getSavedInputWire());
			} else {//connect gates
				connectGates(g1, g);
			}
			Gate* g2 = term();
			if (!g2) {//assign input wire
				g->addInputWire(getSavedInputWire());
			} else {//connect gates
				connectGates(g2, g);
			}
			g1 = g;
		}
		return g1;
	}
	/* END */

	/**
	 * G1 --> G2
	 */
	void connectGates(Gate* g1, Gate* g2) {
		if (g1->getGateNumber() == g2->getGateNumber()) {
			throw runtime_error("Tried to connect gate to self.");
		}
		Wire* wg1 = g1->getEmptyOutputWire();
		if (!wg1) {
			wg1 = new Wire();
			g1->addOutputWire(wg1);
		}
		Wire* wg2 = g2->getEmptyInputWire();
		if (!wg2) {
			wg2 = new Wire();
			g2->addInputWire(wg2);
		}
		wg1->setNext(g2);
		wg2->setPrev(g1);
	}

	GateNumber getNextGateNum() {
		return gn++;
	}

	Wire* getSavedInputWire() {
		if (!inputWire) {
			throw runtime_error("No saved wire.");
		}
		Wire* ret = inputWire;
		inputWire = nullptr;//make sure we use only once
		return ret;
	}

	void check(char ch) {
		bool ok = ((ch >= '0' && ch <= '9')
				|| (ch >= 'a' && ch <= 'z')
				|| (ch == ADD || ch == MUL || ch == CMUL)
				|| (ch == MINUS || ch == OPEN_PAR || ch == CLOSE_PAR));
		if (!ok) {
			throw runtime_error(string("Unexpected character : ") + ch);
		}
	}

	/* BEGIN Generating the comparator circuit */
public:
	/**
	 * Generates a circuit for comparing bit representations of two
	 * values 'a' and 'b', and returns a pointer to the generated circuit.
	 * Evaluation of the circuit will yield :
	 * 	1, if a > b
	 * 	0, otherwise
	 * Parameter 'bitlength' specifies length of the bit representations.
	 * Parameter labelForOne will be the input label of the generated circuit
	 *  , which must be fed with a share of one for correct functionality.
	 * Example :
	 * For parameters bitlength = 2, labelForA = "a", labelForB = "b", labelForOne = "one"
	 * labels for the generated circuit will be "a0", "a1", "b0", "b1", "one".
	 */
	Circuit* generateComparator(uint bitlength, string labelForA, string labelForB, string labelForOne) {
		if (bitlength == 0) {
			throw runtime_error("Bad param : bitlength");
		}
		vector<Circuit*> cv;
		Circuit* dfArr[bitlength];//these circuits will connect MS1 to ƩXY.
		//Labels below can be arbitrarily chosen. (Not input labels. Used internally to mark welding points of circuits.)
		const string LABEL_C = "c";
		const string LABEL_FIP1 = "fiplusone";
		const string LABEL_FI = "fi";
		const string LABEL_OMC_MUL = "omcmul";
		const string LABEL_D = "d";
		const string LABEL_ADD_UP = "addup";
		const string LABEL_ADD_DOWN = "adddown";
		const int l = bitlength - 1;
		/*
		 * First we generate XOR's and MS1.
		 * MS1 finds the index of most significant 1
		 */
		Circuit* prevMul = nullptr;
		for (int i = l; i >= 0; --i) {
			string labelA = labelForA + to_string(i);
			string labelB = labelForB + to_string(i);
			Circuit* xor_ = getXorCircuit(labelA, labelB);// these are input gates and labels matter
			Circuit* omc = getSubCircuit(labelForOne, LABEL_C); // 1 - c
			Circuit* df;
			Circuit* mul;
			if (i == l) {//we have an open input wire for i = l
				df = getSubCircuit(labelForOne, LABEL_FI);// 1 - f_l
				mul = getMulCircuit(labelForOne, LABEL_OMC_MUL);
			} else {
				df = getSubCircuit(LABEL_FIP1, LABEL_FI);// f_i+1 - f_i
				mul = getMulCircuit(LABEL_FIP1, LABEL_OMC_MUL);
				connectCircuits(prevMul, mul, LABEL_FIP1);
				connectCircuits(prevMul, df, LABEL_FIP1);
			}
			connectCircuits(xor_, omc, LABEL_C);
			connectCircuits(omc, mul, LABEL_OMC_MUL);
			connectCircuits(mul, df, LABEL_FI);
			prevMul = mul;
			dfArr[i] = df;
			cv.push_back(xor_);
			cv.push_back(omc);
			cv.push_back(df);
			cv.push_back(mul);
		}
		/*
		 * Now we generate ƩXY.
		 * ƩXY yields the sum over products x_i.y_i
		 */
		Circuit* prevAdd = nullptr;//this will end up as the output gate for the whole comparator circuit
		for (int i = l; i >= 0; --i) {
			string labelA = labelForA + to_string(i);
			Circuit* mul = getMulCircuit(labelA, LABEL_D);
			cv.push_back(mul);
			connectCircuits(dfArr[i], mul, LABEL_D);
			if (!prevAdd) {
				prevAdd = getAddCircuit(LABEL_ADD_UP, LABEL_ADD_DOWN);
				cv.push_back(prevAdd);
				connectCircuits(mul, prevAdd, LABEL_ADD_UP);
			} else {
				connectCircuits(mul, prevAdd, LABEL_ADD_DOWN);
				if (i != 0) {
					Circuit* add = getAddCircuit(LABEL_ADD_UP, LABEL_ADD_DOWN);
					cv.push_back(add);
					connectCircuits(prevAdd, add, LABEL_ADD_UP);
					prevAdd = add;
				}
			}
		}
		c = new Circuit();
		gn = 1;
		for (auto& cPart : cv) {
			combine(c, cPart);
		}
		return c;
	}
private:
	/**
	 * C1 --> C2
	 * Connect output wire of C1, to input wire of C2
	 * with given label.
	 */
	void connectCircuits(Circuit* c1, Circuit* c2, string label) {
		Gate* g1 = c1->getOutputGate();
		if (!g1) {
			g1 = locateOutputGate(c1);
		}
		connectGateToCircuit(g1, c2, label);
	}

	/**
	 * If we already connected a generated circuit to somewhere,
	 * we can no longer count on the dangling wire to identify output gate.
	 * We use the following method instead.
	 */
	Gate* locateOutputGate(Circuit* c) {
		if (c->gates.size() == 1) {//trivial case
			return c->gates[0];
		}
		//if it has a connection to a gate that does not belong to its circuit, it is the output gate
		for (auto& g : c->gates) {
			for (auto& w : g->outputs) {
				if (!c->hasGate(w->next)) {
					return g;
				}
			}
		}
		throw runtime_error("Could not locate output gate.");
	}

	/**
	 * G1 --> C2
	 * Connect output wire of G1, to input wire of C2
	 * with given label.
	 */
	void connectGateToCircuit(Gate* g1, Circuit* c2, string label) {
		Wire* wg1 = g1->getEmptyOutputWire();
		if (!wg1) {
			wg1 = new Wire();
			g1->addOutputWire(wg1);
		}
		Gate* g2 = c2->getInputGateWithLabel(label);
		Wire* wg2 = g2->getInputWireWithLabel(label);
		if (!wg2) {
			throw runtime_error("No wire with matching label.");
		}
		wg1->setNext(g2);
		wg2->setPrev(g1);
	}

	/**
	 * Adds gates of cPart to c.
	 * cPart is destroyed in the process (gates will survive).
	 * This operation does not connect gates. See connectCircuits for that.
	 */
	void combine(Circuit* c, Circuit* cPart) {
		for (auto& g : cPart->getGates()) {
			c->addGate(g);
			g->gateNumber = getNextGateNum();
		}
		cPart->gates.clear();
		delete cPart;
		cPart = nullptr;
	}

	/**
	 * 'XOR'
	 * a+b-2ab --> (a+b+(a*b).-2)
	 */
	Circuit* getXorCircuit(string a, string b) {
		string desc = "(";
		desc += a;
		desc += "+";
		desc += b;
		desc += "+(";
		desc += a;
		desc += "*";
		desc += b;
		desc += ").-2)";
		return generate(desc);
	}
	/**
	 * 'SUB'
	 * x-y --> (x+(y.-1))
	 */
	Circuit* getSubCircuit(string x, string y) {
		string desc = "(";
		desc += x;
		desc += "+(";
		desc += y;
		desc += ".-1))";
		return generate(desc);
	}
	/**
	 * 'MUL'
	 * x*y --> (x*y)
	 */
	Circuit* getMulCircuit(string x, string y) {
		string desc = "(";
		desc += x;
		desc += "*";
		desc += y;
		desc += ")";
		return generate(desc);
	}
	/**
	 * 'ADD'
	 * x+y --> (x+y)
	 */
	Circuit* getAddCircuit(string x, string y) {
		string desc = "(";
		desc += x;
		desc += "+";
		desc += y;
		desc += ")";
		return generate(desc);
	}
	/* END Generating the comparator circuit */

//	/**
//	 * A hard-coded arithmetic circuit.
//	 * Evaluated function : (a + b) * (c . 2)
//	 */
//	Circuit* makeTestCircuit() {
//		ConstantMultGate* gCmul = new ConstantMultGate(1, 2); // gateNumber = 1, constant multiplier = 2
//		AdditionGate* gAdd = new AdditionGate(2);
//		MultiplicationGate* gMul = new MultiplicationGate(3);
//		//Add input wire
//		Wire* g1i = new Wire();
//		gCmul->addInputWire(g1i);
//		g1i->setInputLabel("c");
//		//Add output wire
//		Wire* g1o = new Wire();
//		gCmul->addOutputWire(g1o);
//		//connect gates
//		g1o->setNext(gMul);
//		//Add input wires
//		Wire* g2i1 = new Wire();
//		gAdd->addInputWire(g2i1);
//		g2i1->setInputLabel("a");
//		Wire* g2i2 = new Wire();
//		gAdd->addInputWire(g2i2);
//		g2i2->setInputLabel("b");
//		//Add output wire
//		Wire* g2o = new Wire();
//		gAdd->addOutputWire(g2o);
//		//connect gates
//		g2o->setNext(gMul);
//		//Add input wires
//		Wire* g3i1 = new Wire();
//		gMul->addInputWire(g3i1);
//		Wire* g3i2 = new Wire();
//		gMul->addInputWire(g3i2);
//		//connect gates
//		g3i1->setPrev(gAdd);
//		g3i2->setPrev(gCmul);
//		//Add output wire
//		Wire* g3o = new Wire();
//		gMul->addOutputWire(g3o);
//		//Add gates to the curcuit
//		Circuit* c = new Circuit();
//		c->addGate(gCmul);
//		c->addGate(gAdd);
//		c->addGate(gMul);
//		return c;
//	}
};

} /* namespace pceas */

#endif /* CIRCUITGENERATOR_CPP_ */
