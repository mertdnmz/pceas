/*
 * Verifier.cpp
 *
 *  Created on: Mar 8, 2017
 *      Author: m3r7
 */

#include "Verifier.h"

namespace pceas {

Verifier::Verifier() {
	fmpz_init(vf);
}

Verifier::~Verifier() {
	fmpz_clear(vf);
}

} /* namespace pceas */
