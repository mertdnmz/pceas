/*
 * MultiplicationTriple.h
 *
 *  Created on: Mar 26, 2017
 *      Author: m3r7
 */

#ifndef MULTIPLICATIONTRIPLE_H_
#define MULTIPLICATIONTRIPLE_H_

#include "CommitmentRecord.h"

namespace pceas {

class MultiplicationTriple {
public:
	MultiplicationTriple();
	virtual ~MultiplicationTriple();

	CommitmentRecord* firstMult;
	CommitmentRecord* secontMult;
	CommitmentRecord* product;
	vector<CommitmentRecord*> receivedShares;

	static constexpr const char* M1 = "first_mul";
	static constexpr const char* M2 = "second_mul";
	static constexpr const char* PROD = "product";
	static constexpr const char* E = "e";
	static constexpr const char* D = "d";
};

} /* namespace pceas */

#endif /* MULTIPLICATIONTRIPLE_H_ */
