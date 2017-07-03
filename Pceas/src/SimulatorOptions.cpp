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
 * SimulatorOptions.cpp
 *
 *  Created on: Apr 29, 2017
 *      Author: m3r7
 */

#ifndef SIMULATOROPTIONS_CPP_
#define SIMULATOROPTIONS_CPP_

#include <sstream>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <iomanip>
#include <vector>
#include "core/Pceas.h"

using namespace std;

namespace pceas {

class SimulatorOptions {
public:
	SimulatorOptions() {
		N = 0;
		T = 0;
		FIELD_PRIME = 0;
		prot = PROT_NONE;
		dataUser = NOPARTY;
		comparator = false;
		circuitDescString = "";
		sequentialRun = false;

		loadOptionsFromFile();
	}
	virtual ~SimulatorOptions() {}

	struct Input {
		PartyId p;
		string label;
		ulong value;
	};

	ulong N;//Number of computing parties
	ulong T;//Shamir's threshold
	ulong FIELD_PRIME;//Must be chosen so that evaluation result fits in.

	Protocol prot;

	vector<Input> secrets;
	vector<PartyId> activelyCorrupted;
	PartyId dataUser;

	bool comparator;
	uint bitlength;
	string labelA;
	string labelB;
	string labelOne;

	/*
	 * String representation of the function to be securely evaluated.
	 * Function is to be expressed as an arithmetic circuit.
	 *
	 * '+' : Addition Gate
	 * '.' : Constant Multiplication Gate
	 * '*' : Multiplication Gate
	 *
	 * Input labels used in the expression are expected to match the labels
	 * provided in calls to 'addSecret'.
	 */
	string circuitDescString;

	bool sequentialRun;
	string labelPrevRunResult;
	string nextRunCircuitDescString;

private:
	const string OPTIONS_FILE_PATH = "./options/opt";
	static constexpr const char* DELIMITER = "@";
	static const char STATE_INDICATOR = '#';

	enum ReadState {
		START_,
		N_,
		T_,
		FIELD_PRIME_,
		PROTOCOL_,
		INPUTS_,
		CORRUPT_,
		DATA_USER_,
		COMPARATOR_,
		CIRCUIT_DESC_,
		SEQ_RUN_,
		FINISH_
	};

	ReadState getNextState(ReadState current) {
		switch (current) {
		case START_:
			return N_;
			break;
		case N_:
			return T_;
			break;
		case T_:
			return FIELD_PRIME_;
			break;
		case FIELD_PRIME_:
			return PROTOCOL_;
			break;
		case PROTOCOL_:
			return INPUTS_;
			break;
		case INPUTS_:
			return CORRUPT_;
			break;
		case CORRUPT_:
			return DATA_USER_;
			break;
		case DATA_USER_:
			return COMPARATOR_;
			break;
		case COMPARATOR_:
			return CIRCUIT_DESC_;
			break;
		case CIRCUIT_DESC_:
			return SEQ_RUN_;
			break;
		case SEQ_RUN_:
			return FINISH_;
			break;
		default:
			return FINISH_;
			break;
		}
	}

	void loadOptionsFromFile() {
		ifstream infile(OPTIONS_FILE_PATH);
		if (infile.is_open()) {
			string line;
	    	boost::char_separator<char> sep(DELIMITER);
			ReadState state = START_;
			while (getline(infile, line)) {
			    istringstream iss(line);
			    boost::algorithm::trim(line);
			    boost::algorithm::erase_all(line, " ");
			    boost::algorithm::erase_all(line, "\t");
			    if (line.empty()) {
			    	continue;
			    } else if (line.at(0) == STATE_INDICATOR) {
			    	state = getNextState(state);
			    	continue;
			    } else if (line.at(0) == DELIMITER[0]) {
			    	boost::tokenizer<boost::char_separator<char>> tokens(line, sep);
			    	boost::tokenizer<boost::char_separator<char>>::iterator it = tokens.begin();
				    switch (state) {
				    case N_:
				    	N = atol((*it).c_str());
				    	break;
				    case T_:
				    	T = atol((*it).c_str());
				    	break;
				    case FIELD_PRIME_:
				    	FIELD_PRIME = atol((*it).c_str());
				    	break;
				    case PROTOCOL_:
				    	prot = static_cast<Protocol>(atoi((*it).c_str()));
				    	break;
				    case INPUTS_:
				    {
				    	Input in;
				    	in.p = atol((*it).c_str());
				    	in.label = *++it;
				    	in.value = atol((*++it).c_str());
				    	secrets.push_back(in);
				    	break;
				    }
				    case CORRUPT_:
				    {
				    	PartyId corrupt = atol((*it).c_str());
				    	activelyCorrupted.push_back(corrupt);
				    	break;
				    }
				    case DATA_USER_:
				    	dataUser = atol((*it).c_str());
				    	break;
				    case COMPARATOR_:
				    {
				    	const string TRUE = "TRUE";
				    	string comp = *it;
				    	boost::to_upper(comp);
				    	comparator = (comp == TRUE);
				    	if (comparator) {
				    		bitlength = atoi((*++it).c_str());
				    		labelA = *++it;
				    		labelB = *++it;
				    		labelOne = *++it;
				    	}
				    	break;
				    }
				    case CIRCUIT_DESC_:
				    	if (!comparator) {
				    		circuitDescString = *it;
				    	}
				    	break;
				    case SEQ_RUN_:
				    	if (!comparator) {
					    	const string TRUE = "TRUE";
					    	string seq = *it;
					    	boost::to_upper(seq);
					    	sequentialRun = (seq == TRUE);
					    	if (sequentialRun) {
					    		labelPrevRunResult = *++it;
					    		nextRunCircuitDescString = *++it;
					    	}
				    	}
				    	break;
				    default:
				    	throw runtime_error("Bad options file.");
				    	break;
				    }
			    }
			}
			infile.close();
		} else {
			throw runtime_error("Could not open options file.");
		}
	}
};

} /* namespace pceas */

#endif /* SIMULATOROPTIONS_CPP_ */
