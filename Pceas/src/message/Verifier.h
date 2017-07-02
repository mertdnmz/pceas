/*
 * Verifier.h
 *
 *  Created on: Mar 8, 2017
 *      Author: m3r7
 */

#ifndef VERIFIER_H_
#define VERIFIER_H_

#include <fmpz.h>

namespace pceas {

struct Verifier {
public:
	Verifier();
	virtual ~Verifier();
	fmpz_t vf;
};

} /* namespace pceas */

#endif /* VERIFIER_H_ */
